#include <Mugen/Connector.hpp>
#include <Mugen/Mugen.hpp>
#include <UserInterface/Widgets/TeamserverTabSession.h>
#include <UserInterface/SmallWidgets/EventViewer.hpp>
#include <QCryptographicHash>
#include <QMap>
#include <QBuffer>

Connector::Connector( Util::ConnectionInfo* ConnectionInfo )
{
    Teamserver   = ConnectionInfo;
    Socket       = new QWebSocket();
    auto Server  = "wss://" + Teamserver->Host + ":" + this->Teamserver->Port + "/mugen/";
    auto SslConf = Socket->sslConfiguration();

    Socket->setSslConfiguration( SslConf );

    QObject::connect( Socket, QOverload<const QList<QSslError>&>::of( &QWebSocket::sslErrors ),
                      this, [&]( const QList<QSslError>& errors ) {
        for ( auto& err : errors ) {
            auto cert = err.certificate();
            if ( !cert.isNull() ) {
                auto raw = cert.digest( QCryptographicHash::Sha256 );
                QStringList parts;
                for ( auto b : raw )
                    parts << QString( "%1" ).arg( static_cast<quint8>( b ), 2, 16, QChar( '0' ) ).toUpper();
                MugenX::Teamserver.TLSFingerprint = parts.join( ":" );
                break;
            }
        }
        Socket->ignoreSslErrors();
    } );

    QObject::connect( Socket, &QWebSocket::binaryMessageReceived, this, [&]( const QByteArray& Message )
    {
        auto Package = MugenSpace::Packager::DecodePackage( Message );

        if ( Package != nullptr )
        {
            if ( ! Packager )
                return;

            Packager->DispatchPackage( Package );

            return;
        }

        spdlog::critical( "Got Invalid json" );
    } );

    QObject::connect( Socket, &QWebSocket::connected, this, [&]()
    {
        if ( MugenX::Teamserver.TLSFingerprint.isEmpty() ) {
            auto cert = Socket->sslConfiguration().peerCertificate();
            if ( !cert.isNull() ) {
                auto raw = cert.digest( QCryptographicHash::Sha256 );
                QStringList parts;
                for ( auto b : raw )
                    parts << QString( "%1" ).arg( static_cast<quint8>( b ), 2, 16, QChar( '0' ) ).toUpper();
                MugenX::Teamserver.TLSFingerprint = parts.join( ":" );
            }
        }

        this->Packager = new MugenSpace::Packager;
        this->Packager->setTeamserver( this->Teamserver->Name );

        SendLogin();
    } );

    QObject::connect( Socket, &QWebSocket::disconnected, this, [&]()
    {
        MessageBox( "Teamserver error", Socket->errorString(), QMessageBox::Critical );

        Socket->close();

        Mugen::Exit();
    } );

    Socket->open( QUrl( Server ) );
}

bool Connector::Disconnect()
{
    if ( this->Socket != nullptr )
    {
        this->Socket->disconnect();
        return true;
    }

    return false;
}

Connector::~Connector() noexcept
{
    delete this->Socket;
}

void Connector::SendLogin()
{
    Util::Packager::Package Package;

    Util::Packager::Head_t Head;
    Util::Packager::Body_t Body;

    Head.Event              = Util::Packager::InitConnection::Type;
    Head.User               = this->Teamserver->User.toStdString();
    Head.Time               = CurrentTime().toStdString();

    Body.SubEvent           = Util::Packager::InitConnection::Login;
    Body.Info[ "User" ]     = this->Teamserver->User.toStdString();
    Body.Info[ "Password" ] = QCryptographicHash::hash( this->Teamserver->Password.toLocal8Bit(), QCryptographicHash::Sha3_256 ).toHex().toStdString();

    Package.Head = Head;
    Package.Body = Body;

    SendPackage( &Package );
}

void Connector::SendPackage( Util::Packager::PPackage Package )
{
    Socket->sendBinaryMessage( Packager->EncodePackage( *Package ).toJson( QJsonDocument::Compact ) );
}
