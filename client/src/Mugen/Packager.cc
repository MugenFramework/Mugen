#include <global.hpp>

#include <QSystemTrayIcon>
#include <Mugen/Mugen.hpp>
#include <Mugen/Packager.hpp>
#include <Mugen/DemonCmdDispatch.h>
#include <Mugen/Connector.hpp>

#include <UserInterface/Widgets/TeamserverTabSession.h>
#include <UserInterface/SmallWidgets/EventViewer.hpp>
#include <Mugen/DBManager/DBManager.hpp>
#include <UserInterface/Widgets/DemonInteracted.h>
#include <UserInterface/Widgets/ScriptManager.h>

#include <Util/ColorText.h>
#include <Util/Base.hpp>

#include <sstream>

#include <QScrollBar>
#include <QByteArray>
#include <QJsonArray>
#include <QDir>
#include <QFile>
#include <QFileDialog>

const int Util::Packager::InitConnection::Type      = 0x1;
const int Util::Packager::InitConnection::Success   = 0x1;
const int Util::Packager::InitConnection::Error     = 0x2;
const int Util::Packager::InitConnection::Login     = 0x3;

const int Util::Packager::Listener::Type            = 0x2;
const int Util::Packager::Listener::Add             = 0x1;
const int Util::Packager::Listener::Edit            = 0x2;
const int Util::Packager::Listener::Remove          = 0x3;
const int Util::Packager::Listener::Mark            = 0x4;
const int Util::Packager::Listener::Error           = 0x5;

const int Util::Packager::Chat::Type                = 0x4;
const int Util::Packager::Chat::NewMessage          = 0x1;
const int Util::Packager::Chat::NewListener         = 0x2;
const int Util::Packager::Chat::NewSession          = 0x3;
const int Util::Packager::Chat::NewUser             = 0x4;
const int Util::Packager::Chat::UserDisconnect      = 0x5;

const int Util::Packager::Gate::Type                = 0x5;
const int Util::Packager::Gate::Staged              = 0x1;
const int Util::Packager::Gate::Stageless           = 0x2;

const int Util::Packager::Session::Type             = 0x7;
const int Util::Packager::Session::NewSession       = 0x1;
const int Util::Packager::Session::Remove           = 0x2;
const int Util::Packager::Session::SendCommand      = 0x3;
const int Util::Packager::Session::ReceiveCommand   = 0x4;
const int Util::Packager::Session::MarkAs           = 0x5;
const int Util::Packager::Session::UpdateSession    = 0x6;
const int Util::Packager::Session::SetAlias         = 0x7;

const int Util::Packager::Service::Type             = 0x9;
const int Util::Packager::Service::AgentRegister    = 0x1;
const int Util::Packager::Service::ListenerRegister = 0x2;

const int Util::Packager::Teamserver::Type          = 0x10;
const int Util::Packager::Teamserver::Logger        = 0x1;
const int Util::Packager::Teamserver::Profile       = 0x2;

const int Util::Packager::Resource::Type            = 0xA;
const int Util::Packager::Resource::List            = 0x1;
const int Util::Packager::Resource::Add             = 0x2;
const int Util::Packager::Resource::Remove          = 0x3;
const int Util::Packager::Resource::Download        = 0x4;

const int Util::Packager::TaskHistory::Type         = 0xB;
const int Util::Packager::TaskHistory::List         = 0x1;
const int Util::Packager::TaskHistory::SetComment   = 0x2;
const int Util::Packager::TaskHistory::Delete       = 0x3;
const int Util::Packager::TaskHistory::Sync         = 0x4;

using MugenNamespace::UserInterface::Widgets::ScriptManager;

Util::Packager::PPackage Packager::DecodePackage( const QString& Package )
{
    auto FullPackage    = new Util::Packager::Package;
    auto PackageObject  = QJsonObject();
    auto JsonData       = QJsonDocument::fromJson( Package.toUtf8() );

    if ( JsonData.isEmpty() )
    {
        spdlog::critical( "Invalid json" );
        return nullptr;
    }

    if ( JsonData.isObject() )
    {
        PackageObject = JsonData.object();

        auto HeadObject = PackageObject[ "Head" ].toObject();
        auto BodyObject = PackageObject[ "Body" ].toObject();

        FullPackage->Head.Event = HeadObject[ "Event" ].toInt();
        FullPackage->Head.Time = HeadObject[ "Time" ].toString().toStdString();
        FullPackage->Head.User = HeadObject[ "User" ].toString().toStdString();

        FullPackage->Body.SubEvent = BodyObject[ "SubEvent" ].toInt();

        if ( BodyObject[ "Info" ].isObject() )
        {
            foreach( const QString& key, BodyObject[ "Info" ].toObject().keys() )
            {
                FullPackage->Body.Info[ key.toStdString() ] = BodyObject[ "Info" ].toObject().value( key ).toString().toStdString();
            }
        }

    }
    else
    {
        auto object = QJsonDocument( JsonData ).toJson().toStdString();
        spdlog::critical( "Is not an Object: {}", object );
    }

    return FullPackage;
}

QJsonDocument Packager::EncodePackage( Util::Packager::Package Package )
{
    auto JsonPackage = QJsonObject();
    auto Head        = QJsonObject();
    auto Body        = QJsonObject();
    auto Map         = QVariantMap();
    auto Iterator    = QMapIterator<string, string>( Package.Body.Info );

    while ( Iterator.hasNext() )
    {
        Iterator.next();
        Map.insert( Iterator.key().c_str(), Iterator.value().c_str() );
    }

    Head.insert( "Event", QJsonValue::fromVariant( Package.Head.Event ) );
    Head.insert( "User", QJsonValue::fromVariant( Package.Head.User.c_str() ) );
    Head.insert( "Time", QJsonValue::fromVariant( Package.Head.Time.c_str() ) );
    Head.insert( "OneTime", QJsonValue::fromVariant( Package.Head.OneTime.c_str() ) );

    Body.insert( "SubEvent", QJsonValue::fromVariant( Package.Body.SubEvent ) );
    Body.insert( "Info", QJsonValue::fromVariant( Map ) );

    JsonPackage.insert( "Body", Body );
    JsonPackage.insert( "Head", Head );

    return QJsonDocument( JsonPackage );
}


auto Packager::DispatchPackage( Util::Packager::PPackage Package ) -> bool
{
    switch ( Package->Head.Event )
    {
        case Util::Packager::InitConnection::Type:
            return DispatchInitConnection( Package );

        case Util::Packager::Listener::Type:
            return DispatchListener( Package );

        case Util::Packager::Chat::Type:
            return DispatchChat( Package );

        case Util::Packager::Gate::Type:
            return DispatchGate( Package );

        case Util::Packager::Session::Type:
            return DispatchSession( Package );

        case Util::Packager::Service::Type:
            return DispatchService( Package );

        case Util::Packager::Teamserver::Type:
            return DispatchTeamserver( Package );

        case Util::Packager::Resource::Type:
            return DispatchResource( Package );

        case Util::Packager::TaskHistory::Type:
            return DispatchTaskHistory( Package );

        default:
            spdlog::info( "[PACKAGE] Event Id not found" );
            return false;
    }
}

bool Packager::DispatchInitConnection( Util::Packager::PPackage Package )
{
    MugenX::MugenUserInterface = &MugenApplication->MugenAppUI;
    switch ( Package->Body.SubEvent )
    {
        case Util::Packager::InitConnection::Success:
        {
            if ( MugenApplication->ClientInitConnect ) {
                if ( ! MugenApplication->MugenAppUI.isVisible() ) {
                    MugenApplication->MugenAppUI.setupUi( MugenApplication->MugenMainWindow );
                    MugenApplication->MugenAppUI.setDBManager( MugenApplication->dbManager );
                }

                const auto  scripts = toml::find( MugenApplication->Config, "scripts" );
                const auto& files   = toml::find<std::vector<std::string>>( scripts, "files" );

                for ( const auto& file : files ) {
                    ScriptManager::AddScript( file.c_str() );
                }

                MugenApplication->Start();

                if ( !MugenX::Teamserver.TLSFingerprint.isEmpty() && MugenX::Teamserver.TabSession )
                    MugenX::Teamserver.TabSession->SmallAppWidgets->EventViewer->AppendText(
                        CurrentTime(),
                        "TLS SHA-256: " + MugenX::Teamserver.TLSFingerprint
                    );
            } else {
                MugenApplication->MugenAppUI.NewTeamserverTab( this->TeamserverName );

                if ( !MugenX::Teamserver.TLSFingerprint.isEmpty() && MugenX::Teamserver.TabSession )
                    MugenX::Teamserver.TabSession->SmallAppWidgets->EventViewer->AppendText(
                        CurrentTime(),
                        "TLS SHA-256: " + MugenX::Teamserver.TLSFingerprint
                    );
            }

            return true;
        }

        case Util::Packager::InitConnection::Error:
        {
            if ( Package->Body.Info[ "Message" ] == "" ) {
                MessageBox( "Teamserver Error", QString( "Couldn't connect to Teamserver:" + QString( Package->Body.Info[ "Message" ].c_str() ) ), QMessageBox::Critical );
            } else {
                MessageBox( "Teamserver Error", "Couldn't connect to Teamserver", QMessageBox::Critical );
            }

            return true;
        }

        case 0x5:
        {
            auto TeamserverIPs = QString( Package->Body.Info[ "TeamserverIPs" ].c_str() );
            for ( auto& Ip : TeamserverIPs.split( ", " ) ) {
                MugenX::Teamserver.IpAddresses << Ip;
            }

            MugenX::Teamserver.DemonConfig = QJsonDocument::fromJson( Package->Body.Info[ "Demon" ].c_str() );
        }

        default:
            return false;
    }
}

bool Packager::DispatchListener( Util::Packager::PPackage Package )
{
    switch ( Package->Body.SubEvent )
    {
        case Util::Packager::Listener::Add:
        {
            auto TeamserverTab = MugenX::Teamserver.TabSession;

            // check if this comes from the Teamserver or operator. if from operator then ignore it
            if ( ! Package->Head.User.empty() )
                return false;

            auto ListenerInfo = Util::ListenerItem {
                .Name     = Package->Body.Info[ "Name" ],
                .Protocol = Package->Body.Info[ "Protocol" ],
                .Status   = Package->Body.Info[ "Status" ],
            };

            if ( ListenerInfo.Protocol == Listener::PayloadHTTP.toStdString() )
            {
                auto Headers = QStringList();
                for ( auto& header : QString( Package->Body.Info[ "Headers" ].c_str() ).split( ", " ) ) {
                    Headers << header;
                }

                auto Uris = QStringList();
                for ( auto& uri : QString( Package->Body.Info[ "Uris" ].c_str() ).split( ", " ) ) {
                    Uris << uri;
                }

                auto Hosts = QStringList();
                for ( auto& host : QString( Package->Body.Info[ "Hosts" ].c_str() ).split( ", " ) ) {
                    Hosts << host;
                }

                ListenerInfo.Info = Listener::HTTP {
                    .Hosts          = Hosts,
                    .HostBind       = Package->Body.Info[ "HostBind" ].c_str(),
                    .HostRotation   = Package->Body.Info[ "HostRotation" ].c_str(),
                    .PortBind       = Package->Body.Info[ "PortBind" ].c_str(),
                    .PortConn       = Package->Body.Info[ "PortConn" ].c_str(),
                    .UserAgent      = Package->Body.Info[ "UserAgent" ].c_str(),
                    .Headers        = Headers,
                    .Uris           = Uris,
                    .HostHeader     = Package->Body.Info[ "HostHeader" ].c_str(),
                    .Secure         = Package->Body.Info[ "Secure" ].c_str(),

                    // proxy configuration
                    .ProxyEnabled   = Package->Body.Info[ "Proxy Enabled" ].c_str(),
                    .ProxyType      = Package->Body.Info[ "Proxy Type" ].c_str(),
                    .ProxyHost      = Package->Body.Info[ "Proxy Host" ].c_str(),
                    .ProxyPort      = Package->Body.Info[ "Proxy Port" ].c_str(),
                    .ProxyUsername  = Package->Body.Info[ "Proxy Username" ].c_str(),
                    .ProxyPassword  = Package->Body.Info[ "Proxy Password" ].c_str(),
                };

                if ( Package->Body.Info[ "Secure" ] == "true" ) {
                    ListenerInfo.Protocol = Listener::PayloadHTTPS.toStdString();
                }
            }
            else if ( ListenerInfo.Protocol == Listener::PayloadSMB.toStdString() )
            {
                ListenerInfo.Info = Listener::SMB {
                    .PipeName = Package->Body.Info[ "PipeName" ].c_str(),
                };
            }
            else if ( ListenerInfo.Protocol == Listener::PayloadExternal.toStdString() )
            {
                ListenerInfo.Info = Listener::External {
                    .Endpoint = Package->Body.Info[ "Endpoint" ].c_str(),
                };
            }
            else if ( ListenerInfo.Protocol == Listener::PayloadDNS.toStdString() )
            {
                ListenerInfo.Info = Listener::DNS {
                    .BindHost = Package->Body.Info[ "BindHost" ].c_str(),
                    .BindPort = Package->Body.Info[ "BindPort" ].c_str(),
                    .Domain   = Package->Body.Info[ "Domain" ].c_str(),
                };
            }
            else if ( ListenerInfo.Protocol == Listener::PayloadDoH.toStdString() )
            {
                ListenerInfo.Info = Listener::DoH {
                    .BindHost = Package->Body.Info[ "BindHost" ].c_str(),
                    .PortBind = Package->Body.Info[ "PortBind" ].c_str(),
                    .Domain   = Package->Body.Info[ "Domain" ].c_str(),
                    .Secure   = Package->Body.Info[ "Secure" ].c_str(),
                };
            }
            else if ( ListenerInfo.Protocol == Listener::PayloadTcp.toStdString() )
            {
                ListenerInfo.Info = Listener::Tcp {
                    .Name      = Package->Body.Info[ "Name" ].c_str(),
                    .PivotHost = Package->Body.Info[ "PivotHost" ].c_str(),
                    .PivotPort = Package->Body.Info[ "PivotPort" ].c_str(),
                };
            }
            else
            {
                // We assume it's a service listener.
                auto found = false;

                for ( const auto& listener : MugenX::Teamserver.RegisteredListeners )
                {
                    if ( ListenerInfo.Protocol == listener[ "Name" ].get<std::string>() )
                    {
                        found = true;

                        ListenerInfo.Info = Listener::Service {
                                { "Host",     Package->Body.Info[ "Host" ].c_str() },
                                { "PortBind", Package->Body.Info[ "Port" ].c_str() },
                                { "PortConn", Package->Body.Info[ "Port" ].c_str() },
                                { "Info",     Package->Body.Info[ "Info" ].c_str() } // NOTE: this is json string.
                        };

                        break;
                    }
                }

                if ( ! found  )
                {
                    spdlog::error( "Listener protocol type not found: {} ", ListenerInfo.Protocol );

                    MessageBox(
                        "Listener Error",
                        QString( ( "Listener protocol type not found: {} " + ListenerInfo.Protocol ).c_str() ),
                        QMessageBox::Critical
                    );

                    return false;
                }
            }

            if ( TeamserverTab->ListenerTableWidget == nullptr )
            {
                TeamserverTab->ListenerTableWidget = new UserInterface::Widgets::ListenersTable;
                TeamserverTab->ListenerTableWidget->setupUi( new QWidget );
                TeamserverTab->ListenerTableWidget->TeamserverName = this->TeamserverName;
            }

            TeamserverTab->ListenerTableWidget->ListenerAdd( ListenerInfo );

            if ( ListenerInfo.Status.compare( "Online" ) == 0 )
            {
                auto MsgStr = "[" + Util::ColorText::Cyan( "*" ) + "]" + " Started " + Util::ColorText::Green( "\"" + QString( ListenerInfo.Name.c_str() ) + "\"" ) + " listener";
                auto Time   = QString( Package->Head.Time.c_str() );

                MugenX::Teamserver.TabSession->SmallAppWidgets->EventViewer->AppendText( Time, MsgStr );

                spdlog::info( "Started \"{}\" listener", ListenerInfo.Name );
            }
            else if ( ListenerInfo.Status.compare( "Offline" ) == 0 )
            {
                if ( ! Package->Body.Info[ "Error" ].empty() )
                {
                    auto Error = QString( Package->Body.Info[ "Error" ].c_str() );
                    auto Name  = QString( ListenerInfo.Name.c_str() );

                    TeamserverTab->ListenerTableWidget->ListenerError( Name, Error );
                }
            }

            break;
        }

        case Util::Packager::Listener::Remove:
        {

            MugenX::Teamserver.TabSession->ListenerTableWidget->ListenerRemove( Package->Body.Info[ "Name" ].c_str() );

            break;
        }

        case Util::Packager::Listener::Edit:
        {
            auto ListenerInfo = Util::ListenerItem {
                    .Name     = Package->Body.Info[ "Name" ],
                    .Protocol = Package->Body.Info[ "Protocol" ],
                    .Status   = Package->Body.Info[ "Status" ],
            };

            if ( ListenerInfo.Protocol == Listener::PayloadHTTP.toStdString() )
            {
                auto Headers = QStringList();
                for ( auto& header : QString( Package->Body.Info[ "Headers" ].c_str() ).split( ", " ) )
                    Headers << header;

                auto Uris = QStringList();
                for ( auto& uri : QString( Package->Body.Info[ "Uris" ].c_str() ).split( ", " ) )
                    Uris << uri;

                auto Hosts = QStringList();
                for ( auto& host : QString( Package->Body.Info[ "Hosts" ].c_str() ).split( ", " ) )
                    Hosts << host;


                ListenerInfo.Info = Listener::HTTP {
                        .Hosts          = Hosts,
                        .HostBind       = Package->Body.Info[ "HostBind" ].c_str(),
                        .HostRotation   = Package->Body.Info[ "HostRotation" ].c_str(),
                        .PortBind       = Package->Body.Info[ "PortBind" ].c_str(),
                        .PortConn       = Package->Body.Info[ "PortConn" ].c_str(),
                        .UserAgent      = Package->Body.Info[ "UserAgent" ].c_str(),
                        .Headers        = Headers,
                        .Uris           = Uris,
                        .HostHeader     = Package->Body.Info[ "HostHeader" ].c_str(),
                        .Secure         = Package->Body.Info[ "Secure" ].c_str(),

                        .ProxyEnabled   = Package->Body.Info[ "Proxy Enabled" ].c_str(),
                        .ProxyType      = Package->Body.Info[ "Proxy Type" ].c_str(),
                        .ProxyHost      = Package->Body.Info[ "Proxy Host" ].c_str(),
                        .ProxyPort      = Package->Body.Info[ "Proxy Port" ].c_str(),
                        .ProxyUsername  = Package->Body.Info[ "Proxy Username" ].c_str(),
                        .ProxyPassword  = Package->Body.Info[ "Proxy Password" ].c_str(),
                };

                if ( Package->Body.Info[ "Secure" ] == "true" )
                {
                    ListenerInfo.Protocol = Listener::PayloadHTTPS.toStdString();
                }

            }
            else if ( ListenerInfo.Protocol == Listener::PayloadSMB.toStdString() )
            {
                ListenerInfo.Info = Listener::SMB {
                        .PipeName = Package->Body.Info[ "PipeName" ].c_str(),
                };
            }
            else if ( ListenerInfo.Protocol == Listener::PayloadExternal.toStdString() )
            {
                ListenerInfo.Info = Listener::External {
                        .Endpoint = Package->Body.Info[ "Endpoint" ].c_str(),
                };
            }

            MugenX::Teamserver.TabSession->ListenerTableWidget->ListenerEdit( ListenerInfo );

            break;
        }

        case Util::Packager::Listener::Mark:
        {
            break;
        }

        case Util::Packager::Listener::Error:
        {
            auto Error = Package->Body.Info[ "Error" ];
            auto Name  = Package->Body.Info[ "Name" ];

            if ( Package->Head.User.compare( MugenX::Teamserver.User.toStdString() ) == 0 )
            {
                if ( ! Name.empty() )
                {
                    if ( ! Error.empty() )
                    {
                        MessageBox( "Listener Error", QString( Error.c_str() ), QMessageBox::Critical );
                        MugenX::Teamserver.TabSession->ListenerTableWidget->ListenerError( QString( Name.c_str() ), QString( Error.c_str() ) );

                        auto MsgStr = "[" + Util::ColorText::Red( "-" ) + "]" + " Failed to start " + Util::ColorText::Green( "\"" + QString( Name.c_str() ) + "\"" ) + " listener: " + Util::ColorText::Red( Error.c_str() );
                        auto Time   = QString( Package->Head.Time.c_str() );

                        MugenX::Teamserver.TabSession->SmallAppWidgets->EventViewer->AppendText( Time, MsgStr );
                    }
                }
            }
            else if ( Package->Head.User.empty() )
            {
                if ( ! Name.empty() )
                {
                    if ( ! Error.empty() )
                    {
                        MugenX::Teamserver.TabSession->ListenerTableWidget->ListenerError( QString( Name.c_str() ), QString( Error.c_str() ) );

                        auto MsgStr = "[" + Util::ColorText::Red( "-" ) + "]" + " Failed to start " + Util::ColorText::Green( "\"" + QString( Name.c_str() ) + "\"" ) + " listener: " + Util::ColorText::Red( Error.c_str() );
                        auto Time   = QString( Package->Head.Time.c_str() );

                        MugenX::Teamserver.TabSession->SmallAppWidgets->EventViewer->AppendText( Time, MsgStr );
                    }
                }
            }

            break;
        }
    }
    return true;
}

bool Packager::DispatchChat( Util::Packager::PPackage Package)
{
    switch (Package->Body.SubEvent) {
        case Util::Packager::Chat::NewMessage:
        {
            auto TeamserverUser = MugenX::Teamserver.User;

            for ( const auto& e : Package->Body.Info.toStdMap() )
            {
                auto Time = QString( Package->Head.Time.c_str() );

                MugenX::Teamserver.TabSession->TeamserverChat->AddUserMessage( Time, string( e.first ).c_str(), QByteArray::fromBase64( string( e.second ).c_str() ) );
            }
            break;
        }

        case Util::Packager::Chat::NewListener:
        {
            break;
        }

        case Util::Packager::Chat::NewSession:
        {
            break;
        }

        case Util::Packager::Chat::NewUser:
        {
            auto user = QString( Package->Body.Info.toStdMap()[ "User" ].c_str() );
            auto Time = QString( Package->Head.Time.c_str() );

            MugenX::Teamserver.TabSession->SmallAppWidgets->EventViewer->AppendText( Time,  "[" + Util::ColorText::Green( "+" ) + "] " + Util::ColorText::Green( user + " connected to teamserver" ) );

            break;
        }

        case Util::Packager::Chat::UserDisconnect:
        {
            auto user = QString( Package->Body.Info.toStdMap()[ "User" ].c_str() );
            auto Time = QString( Package->Head.Time.c_str() );

            MugenX::Teamserver.TabSession->SmallAppWidgets->EventViewer->AppendText( Time, "[" + Util::ColorText::Red( "-" ) + "] " + Util::ColorText::Red( user + " disconnected from teamserver" ) );

            break;
        }
    }
    return true;
}

bool Packager::DispatchGate( Util::Packager::PPackage Package )
{
    switch ( Package->Body.SubEvent )
    {
        case Util::Packager::Gate::Staged:
        {
            break;
        }

        case Util::Packager::Gate::Stageless:
        {

            if ( Package->Body.Info[ "PayloadArray" ].size() > 0 )
            {
                auto PayloadArray = QString( Package->Body.Info[ "PayloadArray" ].c_str() ).toLocal8Bit();
                auto FileName     = QString( Package->Body.Info[ "FileName" ].c_str() );

                if (MugenX::GateGUI)
                {
                    MugenX::Teamserver.TabSession->PayloadDialog->ReceivedImplantAndSave( FileName, QByteArray::fromBase64( PayloadArray ) );
                    MugenX::GateGUI = false;
                }
                else
                {
                    if ( MugenX::callbackGate )
                    {
                        PyObject* pyByteArray= PyUnicode_DecodeFSDefault(Package->Body.Info[ "PayloadArray" ].c_str());
                        PyObject_CallFunctionObjArgs(MugenX::callbackGate, pyByteArray, nullptr);
                    }
                    else
                    {
                        break; // quit if there is no callback
                    }
                }
            }
            else if ( Package->Body.Info[ "MessageType" ].size() > 0  && MugenX::GateGUI)
            {
                auto MessageType = QString( Package->Body.Info[ "MessageType" ].c_str() );
                auto Message     = QString( Package->Body.Info[ "Message" ].c_str() );

                MugenX::Teamserver.TabSession->PayloadDialog->addConsoleLog( MessageType, Message );
            }

            break;
        }
    }
    return true;
}

bool Packager::DispatchSession( Util::Packager::PPackage Package )
{
    switch ( Package->Body.SubEvent )
    {
        case Util::Packager::Session::NewSession:
        {
            auto TeamserverTab = MugenX::Teamserver.TabSession;
            auto MagicValue    = uint64_t( 0 );
            auto StringStream  = std::stringstream();

            StringStream << std::hex << Package->Body.Info[ "MagicValue" ].c_str();
            StringStream >> MagicValue;

            auto Agent = Util::SessionItem {
                    .Name         = Package->Body.Info[ "NameID" ].c_str(),
                    .Alias        = Package->Body.Info[ "Alias" ].c_str(),
                    .MagicValue   = MagicValue,
                    .External     = Package->Body.Info[ "ExternalIP" ].c_str(),
                    .Internal     = Package->Body.Info[ "InternalIP" ].c_str(),
                    .Listener     = Package->Body.Info[ "Listener" ].c_str(),
                    .User         = Package->Body.Info[ "Username" ].c_str(),
                    .Computer     = Package->Body.Info[ "Hostname" ].c_str(),
                    .Domain       = Package->Body.Info[ "DomainName" ].c_str(),
                    .OS           = Package->Body.Info[ "OSVersion" ].c_str(),
                    .OSBuild      = Package->Body.Info[ "OSBuild" ].c_str(),
                    .OSArch       = Package->Body.Info[ "OSArch" ].c_str(),
                    .Process      = Package->Body.Info[ "ProcessName" ].c_str(),
                    .PID          = Package->Body.Info[ "ProcessPID" ].c_str(),
                    .Arch         = Package->Body.Info[ "ProcessArch" ].c_str(),
                    .First        = Package->Body.Info[ "FirstCallIn" ].c_str(),
                    .Last         = Package->Body.Info[ "LastCallIn" ].c_str(),
                    .Elevated     = Package->Body.Info[ "Elevated" ].c_str(),
                    .PivotParent  = Package->Body.Info[ "PivotParent" ].c_str(),
                    .Marked       = Package->Body.Info[ "Active" ].c_str(),
                    .SleepDelay   = (uint32_t)strtoul(Package->Body.Info[ "SleepDelay" ].c_str(), NULL, 0),
                    .SleepJitter  = (uint32_t)strtoul(Package->Body.Info[ "SleepJitter" ].c_str(), NULL, 0),
                    .KillDate     = (uint64_t)strtoull(Package->Body.Info[ "KillDate" ].c_str(), NULL, 0),
                    .WorkingHours = (uint32_t)strtoul(Package->Body.Info[ "WorkingHours" ].c_str(), NULL, 0),
            };

            Agent.LastUTC = QDateTime::fromString(Agent.Last, "dd-MM-yyyy HH:mm:ss");

            if ( Agent.Marked == "true" )
            {
                Agent.Marked = "Alive";
                Agent.Health = "healthy";
            }
            else if ( Agent.Marked == "false" )
            {
                Agent.Marked = "Dead";
                Agent.Health = "dead";
            }

            for ( auto& session : MugenX::Teamserver.Sessions )
                if ( session.Name.compare( Agent.Name ) == 0 )
                    return false;

            // Load any persisted tags/notes for this agent before adding to table
            if ( TeamserverTab->dbManager ) {
                QString tags, notes;
                if ( TeamserverTab->dbManager->GetSessionMeta( Agent.Name, tags, notes ) ) {
                    Agent.Tags  = tags;
                    Agent.Notes = notes;
                }
            }

            TeamserverTab->SessionTableWidget->NewSessionItem( Agent );
            if ( TeamserverTab->LootWidget )
                TeamserverTab->LootWidget->AddSessionSection( Agent.Name );

            // Desktop notification for new agent check-in
            if ( Agent.Marked.compare( "Alive" ) == 0 )
            {
                auto ui = MugenX::MugenUserInterface;
                if ( ui && ui->TrayIcon )
                {
                    auto agentType = ( Agent.MagicValue == TenguMagicValue ) ? "Tengu" : "Demon";
                    ui->TrayIcon->showMessage(
                        QString( "New %1 Agent" ).arg( agentType ),
                        Agent.User + "@" + Agent.Computer + " (" + Agent.External + ")",
                        QSystemTrayIcon::Information,
                        5000
                    );
                }
            }

            auto Time    = QString( Package->Head.Time.c_str() );
            auto Message = "[" + Util::ColorText::Cyan( "*" ) + "]" + " Initialized " + Util::ColorText::Cyan( Agent.Name ) + " :: " + Util::ColorText::Yellow( Agent.User + "@" + Agent.Internal ) + Util::ColorText::Cyan( " (" ) + Util::ColorText::Red( Agent.Computer ) + Util::ColorText::Cyan( ")" );

            MugenX::Teamserver.TabSession->SmallAppWidgets->EventViewer->AppendText( Time, Message );

            if ( Agent.Marked.compare( "Alive" ) == 0 )
            {
                for ( auto& Callback : MugenX::Teamserver.RegisteredCallbacks )
                {
                    if ( PyCallable_Check( Callback ) )
                    {
                        PyObject* arglist = Py_BuildValue( "s", Agent.Name.toStdString().c_str() );
                        PyObject* Return  = PyObject_CallFunctionObjArgs( Callback, arglist, NULL );
                        if ( Return == NULL && PyErr_Occurred() )
                        {
                            spdlog::error( "Error calling callback" );
                            PyErr_PrintEx(0);
                            PyErr_Clear();
                        }
                    } else {
                        spdlog::error( "Callback is not callable" );
                    }
                }
            }

            break;
        }

        case Util::Packager::Session::SendCommand:
        {
            for ( auto& Session : MugenX::Teamserver.Sessions )
            {
                if ( Session.Name.compare( Package->Body.Info[ "DemonID" ].c_str() ) == 0 )
                {
                    auto AgentType = QString( Package->Body.Info[ "AgentType" ].c_str() );

                    if ( ! Package->Body.Info[ "CommandLine" ].empty() )
                    {
                        auto TaskID = QString( Package->Body.Info[ "TaskID" ].c_str() );

                        if ( AgentType.isEmpty() )
                            AgentType = "Demon";

                        Session.InteractedWidget->DemonCommands->Prompt = QString (
                                Util::ColorText::Comment( QString( Package->Head.Time.c_str() ) + " [" + QString( Package->Head.User.c_str() ) + "]" ) +
                                " " + Util::ColorText::UnderlinePink( AgentType ) +
                                Util::ColorText::Cyan(" » ") + QString( Package->Body.Info[ "CommandLine" ].c_str() )
                        );

                        if ( ! Package->Body.Info[ "TaskMessage" ].empty() )
                        {
                            Session.InteractedWidget->DemonCommands->CommandTaskInfo[ TaskID ] = Package->Body.Info[ "TaskMessage" ].c_str();
                        }
                        else
                        {
                            // Mark this task in the console before printing the prompt
                            Session.InteractedWidget->Console->beginTask( TaskID );
                            Session.InteractedWidget->AppendRaw();
                            Session.InteractedWidget->AppendRaw( Session.InteractedWidget->DemonCommands->Prompt );
                        }

                        Session.InteractedWidget->lineEdit->AddCommand( QString( Package->Body.Info[ "CommandLine" ].c_str() ) );
                        if ( Session.InteractedWidget->TenguCmds == nullptr ) {
                            Session.InteractedWidget->DemonCommands->DispatchCommand( false, TaskID, Package->Body.Info[ "CommandLine" ].c_str() );
                        }
                    }
                }
            }
            break;
        }

        case Util::Packager::Session::ReceiveCommand:
        {
            for ( auto & Session : MugenX::Teamserver.Sessions )
            {
                if ( Session.Name.compare( Package->Body.Info[ "DemonID" ].c_str() ) == 0 )
                {
                    Session.InteractedWidget->DemonCommands->OutputDispatch.DemonCommandInstance = Session.InteractedWidget->DemonCommands;

                    int CommandID = QString( Package->Body.Info[ "CommandID" ].c_str() ).toInt();
                    auto Output   = QString( Package->Body.Info[ "Output" ].c_str() );

                    switch ( CommandID )
                    {
                        case ( int ) Commands::CONSOLE_MESSAGE:

                            if ( QByteArray::fromBase64( Output.toLocal8Bit() ).length() > 5 )
                            {
                                Session.InteractedWidget->DemonCommands->OutputDispatch.MessageOutput(
                                        Output,
                                        QString( Package->Head.Time.c_str() )
                                );
                                Session.InteractedWidget->Console->verticalScrollBar()->setValue(
                                        Session.InteractedWidget->Console->verticalScrollBar()->maximum()
                                );
                            }

                            break;

                        case ( int ) Commands::BOF_CALLBACK:

                            if ( QByteArray::fromBase64( Output.toLocal8Bit() ).length() > 5 )
                            {
                                auto JsonDocument  = QJsonDocument::fromJson( QByteArray::fromBase64( Output.toLocal8Bit( ) ) );
                                auto Worked        = JsonDocument[ "Worked" ].toString();
                                auto Output        = JsonDocument[ "Output" ].toString();
                                auto Error         = JsonDocument[ "Error"  ].toString();
                                auto TaskID        = JsonDocument[ "TaskID" ].toString();
                                PyObject* Callback = nullptr;

                                auto it = Session.TaskIDToPythonCallbacks.find( TaskID );
                                if ( it != Session.TaskIDToPythonCallbacks.end() ) {
                                    Callback = it->second;
                                    if ( PyCallable_Check( Callback ) )
                                    {
                                        PyObject *arglist = Py_BuildValue( "ssOss", Session.Name.toStdString().c_str(), TaskID.toStdString().c_str(), Worked == "true" ? Py_True : Py_False, Output.toStdString().c_str(), Error.toStdString().c_str() );
                                        PyObject_CallObject( Callback, arglist );
                                        Py_XDECREF( Callback );
                                    } else {
                                        spdlog::error( "Callback is not callable" );
                                    }

                                    Session.TaskIDToPythonCallbacks.erase( TaskID );

                                    // print messages from the python the module
                                    Session.InteractedWidget->DemonCommands->PrintModuleCachedMessages();
                                } else {
                                    auto taskId = TaskID.toStdString();
                                    spdlog::error( "[PACKAGE] TaskID not found: {}", taskId );
                                }
                            }

                            break;

                        case ( int ) Commands::CALLBACK:
                        {
                            // update the "Last" field on this session
                            auto LastTime     = QString( QByteArray::fromBase64( Output.toLocal8Bit() ) );
                            auto LastTimeJson = QJsonDocument::fromJson( LastTime.toLocal8Bit() );

                            Session.Last         = LastTimeJson["Last"].toString();
                            Session.LastUTC      = QDateTime::fromString(Session.Last, "dd-MM-yyyy HH:mm:ss");
                            Session.SleepDelay   = (uint32_t)strtoul(LastTimeJson["Sleep"].toString().toStdString().c_str(), NULL, 0);
                            Session.SleepJitter  = (uint32_t)strtoul(LastTimeJson["Jitter"].toString().toStdString().c_str(), NULL, 0);
                            Session.KillDate     = (uint64_t)strtoull(LastTimeJson["KillDate"].toString().toStdString().c_str(), NULL, 0);
                            Session.WorkingHours = (uint32_t)strtoul(LastTimeJson["WorkingHours"].toString().toStdString().c_str(), NULL, 0);
                            break;
                        }

                        default:
                            spdlog::error( "[PACKAGE] Command not found" );
                            break;
                    }

                    break;
                }
            }

            break;
        }

        case Util::Packager::Session::Remove:
        {
            break;
        }

        case Util::Packager::Session::MarkAs:
        {
            auto AgentID = Package->Body.Info[ "AgentID" ];
            auto Marked  = Package->Body.Info[ "Marked" ];

            for ( auto& session : MugenX::Teamserver.Sessions )
            {
                if ( session.Name.toStdString() == AgentID )
                {
                    session.Marked = Marked.c_str();
                    break;
                }
            }

            for ( int i = 0; i < MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->rowCount(); i++ )
            {
                auto Row = MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->item( i, 0 )->text();

                if ( Row.compare( QString( AgentID.c_str() ) ) == 0 )
                {
                    if ( Marked.compare( "Alive" ) == 0 )
                    {
                        for ( auto& session : MugenX::Teamserver.Sessions )
                        {
                            if ( session.Name.toStdString() == AgentID )
                            {
                                auto Icon = ( session.Elevated.compare( "true" ) == 0 ) ?
                                        WinVersionIcon( session.OS, true ) :
                                        WinVersionIcon( session.OS, false );

                                MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->item( i, 0 )->setIcon( Icon );

                                break;
                            }
                        }

                        for ( int j = 0; j < MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->columnCount(); j++ )
                        {
                            MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->item( i, j )->setBackground( QColor( Util::ColorText::Colors::Hex::Background ) );
                            MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->item( i, j )->setForeground( QColor( Util::ColorText::Colors::Hex::Foreground ) );
                        }
                    }
                    else if ( Marked.compare( "Dead" ) == 0 )
                    {
                        MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->item( i, 0 )->setIcon( QIcon( ":/icons/DeadWhite" ) );

                        for ( int j = 0; j < MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->columnCount(); j++ )
                        {
                            MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->item( i, j )->setBackground( QColor( Util::ColorText::Colors::Hex::CurrentLine ) );
                            MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->item( i, j )->setForeground( QColor( Util::ColorText::Colors::Hex::Comment ) );
                        }
                    }

                    break;
                }
            }

            break;
        }

        case Util::Packager::Session::UpdateSession:
        {
            auto AgentID  = Package->Body.Info[ "AgentID" ];
            auto Username = Package->Body.Info[ "Username" ];

            for ( auto& session : MugenX::Teamserver.Sessions )
            {
                if ( session.Name.toStdString() == AgentID )
                {
                    session.User = Username.c_str();
                    break;
                }
            }

            auto* table = MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget;
            for ( int i = 0; i < table->rowCount(); i++ )
            {
                if ( table->item( i, 0 )->text().compare( QString( AgentID.c_str() ) ) == 0 )
                {
                    table->item( i, 3 )->setText( QString( Username.c_str() ) );
                    break;
                }
            }

            MugenX::Teamserver.TabSession->SessionMapWidget->UpdateMap( MugenX::Teamserver.Sessions );

            auto* node = MugenX::Teamserver.TabSession->SessionGraphWidget->GraphNodeGet( QString( AgentID.c_str() ) );
            if ( node )
            {
                for ( auto& session : MugenX::Teamserver.Sessions )
                {
                    if ( session.Name.toStdString() == AgentID )
                    {
                        node->setLabel( session.Name + " " + session.Process + "\\" + session.PID +
                                        " [" + session.Computer + "\\" + session.User + "]" );
                        node->Session   = session;
                        node->update();
                        break;
                    }
                }
            }

            break;
        }

        case Util::Packager::Session::SetAlias:
        {
            auto AgentID = QString( Package->Body.Info[ "AgentID" ].c_str() );
            auto Alias   = QString( Package->Body.Info[ "Alias" ].c_str() );

            MugenX::Teamserver.TabSession->SessionTableWidget->SetSessionAlias( AgentID, Alias );

            break;
        }
    }
    return true;
}

bool Packager::DispatchService( Util::Packager::PPackage Package )
{
    switch ( Package->Body.SubEvent )
    {
        case Util::Packager::Service::AgentRegister:
        {
            auto JsonObject     = QJsonDocument::fromJson( Package->Body.Info[ "Agent" ].c_str() ).object();
            auto OSArray        = QStringList();
            auto Arch           = QStringList();
            auto Formats        = std::vector<AgentFormat>();
            auto Commands       = std::vector<AgentCommands>();
            auto MagicValue     = uint64_t( 0 );
            auto StringStream   = std::stringstream();
            auto AgentName      = std::string();

            for ( const auto& item : JsonObject[ "Arch" ].toArray() )
                Arch << item.toString();

            for ( const auto& item : JsonObject[ "Formats" ].toArray() )
            {
                Formats.push_back( AgentFormat {
                        .Name = item.toObject()[ "Name" ].toString(),
                        .Extension = item.toObject()[ "Extension" ].toString(),
                } );
            }

            for ( const auto& item : JsonObject[ "SupportedOS" ].toArray() )
                OSArray << item.toString();

            for ( const auto& command : JsonObject[ "Commands" ].toArray() )
            {
                auto Mitr   = QStringList();
                auto Params = std::vector<CommandParam>();

                for ( const auto& param : command.toObject()[ "Params" ].toArray() )
                {
                    Params.push_back( CommandParam {
                        .Name       = param.toObject()[ "Name" ].toString(),
                        .IsFilePath = param.toObject()[ "IsFilePath" ].toBool(),
                        .IsOptional = param.toObject()[ "IsOptional" ].toBool(),
                    } );
                }

                for ( const auto& i : command.toObject()[ "Mitr" ].toArray() )
                    Mitr << i.toString();

                Commands.push_back( AgentCommands{
                    .Name        = command.toObject()[ "Name" ].toString(),
                    .Description = command.toObject()[ "Description" ].toString(),
                    .Help        = command.toObject()[ "Help" ].toString(),
                    .NeedAdmin   = command.toObject()[ "NeedAdmin" ].toBool(),
                    .Mitr        = Mitr,
                    .Params      = Params,
                    .Anonymous   = command.toObject()[ "Anonymous" ].toBool(),
                } );
            }

            StringStream << std::hex << JsonObject[ "MagicValue" ].toString().toStdString();
            StringStream >> MagicValue;

            MugenX::Teamserver.ServiceAgents.push_back( ServiceAgent{
                .Name           = JsonObject[ "Name" ].toString(),
                .Description    = JsonObject[ "Description" ].toString(),
                .Version        = JsonObject[ "Version" ].toString(),
                .MagicValue     = MagicValue,
                .Arch           = Arch,
                .Formats        = Formats,
                .SupportedOS    = OSArray,
                .Commands       = Commands,
                .BuildingConfig = QJsonDocument( JsonObject[ "BuildingConfig" ].toObject() ),
            } );

            AgentName = JsonObject[ "Name" ].toString().toStdString();

            spdlog::info( "Added service agent to client: {}", AgentName );

            return true;
        }

        case Util::Packager::Service::ListenerRegister:
        {
            auto listener = json::parse( Package->Body.Info[ "Listener" ].c_str() );
            auto name     = listener[ "Name" ].get<std::string>();

            MugenX::Teamserver.RegisteredListeners.push_back( listener );

            spdlog::info( "Added service listener to client: {}", name );

            return true;
        }

        default: break;
    }
    return false;
}

bool Packager::DispatchTeamserver( Util::Packager::PPackage Package )
{
    switch ( Package->Body.SubEvent )
    {
        case Util::Packager::Teamserver::Logger:
        {
            auto Text = QString( Package->Body.Info[ "Text" ].c_str() );

            if ( MugenX::Teamserver.TabSession->Teamserver == nullptr )
            {
                MugenX::Teamserver.TabSession->Teamserver = new Teamserver;
                MugenX::Teamserver.TabSession->Teamserver->setupUi( new QDialog );
            }

            MugenX::Teamserver.TabSession->Teamserver->AddLoggerText( Text );
        }

        case Util::Packager::Teamserver::Profile:
        {

        }
    }
    return true;
}


void Packager::setTeamserver( QString Name )
{
    this->TeamserverName = Name;
}

bool Packager::DispatchResource( Util::Packager::PPackage Package )
{
    switch ( Package->Body.SubEvent )
    {
        case Util::Packager::Resource::List:
        {
            auto TeamserverTab = MugenX::Teamserver.TabSession;
            if ( !TeamserverTab ) return false;
            if ( !TeamserverTab->ResourceManager ) return false;

            auto resourcesJson = QString( Package->Body.Info[ "Resources" ].c_str() );
            TeamserverTab->ResourceManager->Refresh( resourcesJson );
            break;
        }

        case Util::Packager::Resource::Download:
        {
            // seul le client qui a demandé le download traite la réponse
            auto requestUser = QString( Package->Body.Info[ "RequestUser" ].c_str() );
            if ( requestUser != MugenX::Teamserver.User ) break;

            auto name    = QString( Package->Body.Info[ "Name"    ].c_str() );
            auto content = QString( Package->Body.Info[ "Content" ].c_str() );

            auto savePath = QFileDialog::getSaveFileName(
                nullptr, "Save Resource", QDir::homePath() + "/" + name
            );
            if ( savePath.isEmpty() ) break;

            auto data = QByteArray::fromBase64( content.toUtf8() );
            QFile f( savePath );
            if ( f.open( QIODevice::WriteOnly ) )
            {
                f.write( data );
                f.close();
            }
            break;
        }
    }
    return true;
}

auto NewPackageResource( const QString& TeamserverName, Util::Packager::Body_t Body ) -> void
{
    auto Package    = new Util::Packager::Package;
    auto Head       = Util::Packager::Head_t {
        .Event = Util::Packager::Resource::Type,
    };
    Package->Head   = Head;
    Package->Body   = Body;
    MugenX::Connector->SendPackage( Package );
}

bool Packager::DispatchTaskHistory( Util::Packager::PPackage Package )
{
    switch ( Package->Body.SubEvent )
    {
        case Util::Packager::TaskHistory::Sync:
        {
            auto agentID  = QString( Package->Body.Info[ "AgentID" ].c_str() );
            auto tasksStr = QString( Package->Body.Info[ "Tasks"   ].c_str() );

            auto doc   = QJsonDocument::fromJson( tasksStr.toUtf8() );
            auto tasks = doc.array();

            for ( auto& session : MugenX::Teamserver.Sessions )
            {
                if ( session.Name.compare( agentID ) == 0 )
                {
                    if ( session.InteractedWidget )
                        session.InteractedWidget->replayHistory( tasks );
                    break;
                }
            }
            break;
        }
    }
    return true;
}

auto NewPackageTaskHistory( const QString& TeamserverName, Util::Packager::Body_t Body ) -> void
{
    auto Package    = new Util::Packager::Package;
    auto Head       = Util::Packager::Head_t {
        .Event = Util::Packager::TaskHistory::Type,
    };
    Package->Head   = Head;
    Package->Body   = Body;
    MugenX::Connector->SendPackage( Package );
}
