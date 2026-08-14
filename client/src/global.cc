#include <global.hpp>
#include <random>

#include <Mugen/Connector.hpp>

#include <QFile>

using namespace std;
using namespace MugenNamespace;
using namespace MugenNamespace::MugenSpace;

string MugenNamespace::Version  = "0.2";
string MugenNamespace::CodeName = "My Dress-Up Darling";

// Global Variables in the Mugen Namespace
MugenSpace::Mugen* MugenNamespace::MugenApplication;

Util::ConnectionInfo                    MugenX::Teamserver;
MugenNamespace::Connector*              MugenX::Connector;
MugenNamespace::UserInterface::MugenUi* MugenX::MugenUserInterface;
bool                                    MugenX::DebugMode = false;
bool                                    MugenX::GateGUI = false;
PyObject*                               MugenX::callbackGate = nullptr;
PyObject*                               MugenX::callbackMessage = nullptr;

QString MugenSpace::Listener::PayloadHTTPS    = "Https";
QString MugenSpace::Listener::PayloadHTTP     = "Http";
QString MugenSpace::Listener::PayloadSMB      = "Smb";
QString MugenSpace::Listener::PayloadExternal = "External";
QString MugenSpace::Listener::PayloadDNS      = "Dns";
QString MugenSpace::Listener::PayloadDoH      = "DoH";
QString MugenSpace::Listener::PayloadTcp = "Tcp";

std::string Util::gen_random( const int len )
{
    auto str = std::string( "0123456789ABCDEF" );
    auto rd  = std::random_device();
    auto gen = std::mt19937( rd() );

    std::shuffle( str.begin(), str.end(), gen );

    return str.substr( 0, len );
}

void Util::SessionItem::Export()
{
    auto savePath = ThemedSaveFileDialog( "Export Session", "Session_data_" + Name + ".json" );
    if ( savePath.isEmpty() )
        return;

    auto file       = QFile( savePath );
    auto messageBox = QMessageBox();

    if ( file.open( QIODevice::ReadWrite ) ) {
        auto SessionData = QJsonObject();

        SessionData.insert( "AgentID",          QJsonValue::fromVariant( Name ) );
        SessionData.insert( "Alias",            QJsonValue::fromVariant( Alias ) );
        SessionData.insert( "Tags",             QJsonValue::fromVariant( Tags ) );
        SessionData.insert( "Notes",            QJsonValue::fromVariant( Notes ) );
        SessionData.insert( "Color",            QJsonValue::fromVariant( Color ) );
        SessionData.insert( "MagicValue",       QJsonValue::fromVariant( (int) MagicValue ) );
        SessionData.insert( "ExternalIP",       QJsonValue::fromVariant( External ) );
        SessionData.insert( "InternalIP",       QJsonValue::fromVariant( Internal ) );
        SessionData.insert( "Listener",         QJsonValue::fromVariant( Listener ) );
        SessionData.insert( "User",             QJsonValue::fromVariant( User ) );
        SessionData.insert( "Computer",         QJsonValue::fromVariant( Computer ) );
        SessionData.insert( "Domain",           QJsonValue::fromVariant( Domain ) );
        SessionData.insert( "OS",               QJsonValue::fromVariant( OS ) );
        SessionData.insert( "OSBuild",          QJsonValue::fromVariant( OSBuild ) );
        SessionData.insert( "OSArch",           QJsonValue::fromVariant( OSArch ) );
        SessionData.insert( "ProcessName",      QJsonValue::fromVariant( Process ) );
        SessionData.insert( "ProcessID",        QJsonValue::fromVariant( PID ) );
        SessionData.insert( "ProcessArch",      QJsonValue::fromVariant( Arch ) );
        SessionData.insert( "ProcessElevated",  QJsonValue::fromVariant( Elevated ) );
        SessionData.insert( "PivotParent",      QJsonValue::fromVariant( PivotParent ) );
        SessionData.insert( "First Callback",   QJsonValue::fromVariant( First ) );
        SessionData.insert( "Last Callback",    QJsonValue::fromVariant( Last ) );

        file.write( QJsonDocument( SessionData ).toJson( QJsonDocument::Indented ) );
    }
    else {
        spdlog::error("Couldn't write to file {}", savePath.toStdString() );
    }

    file.close();

    messageBox.setWindowTitle( "Session Exported" );
    messageBox.setText( "Path: " + savePath );
    messageBox.setIcon( QMessageBox::Information );
    messageBox.setStyleSheet( FileRead( ":/stylesheets/MessageBox" ) );
    messageBox.exec();
}
