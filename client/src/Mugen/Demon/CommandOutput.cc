#include <QJsonDocument>
#include <QJsonArray>

#include <Mugen/DemonCmdDispatch.h>

#include <UserInterface/Widgets/DemonInteracted.h>
#include <UserInterface/Widgets/TeamserverTabSession.h>
#include <UserInterface/Widgets/ProcessList.hpp>

#include <Util/ColorText.h>
#include <QFile>

using namespace MugenNamespace::MugenSpace;

void DispatchOutput::MessageOutput( QString JsonString, const QString& Date = "" ) const
{
    auto JsonDocument = QJsonDocument::fromJson( QByteArray::fromBase64( JsonString.toLocal8Bit( ) ) );
    auto TaskID       = JsonDocument[ "TaskID" ].toString();
    auto MessageType  = JsonDocument[ "Type" ].toString();
    auto Message      = JsonDocument[ "Message" ].toString();
    auto Output       = JsonDocument[ "Output" ].toString();
    auto Operator     = JsonDocument[ "Operator" ].toString();

    auto opTag = Operator.isEmpty()
        ? QString()
        : Util::ColorText::Comment( "[" + Operator + "]" ) + " ";

    if ( Message.length() > 0 )
    {
        if ( MessageType == "Error" || MessageType == "Erro" )
            this->DemonCommandInstance->DemonConsole->AppendRaw( Util::ColorText::Red( "[!]" ) + " " + opTag + Message );
        else if ( MessageType == "Good" )
            this->DemonCommandInstance->DemonConsole->AppendRaw( Util::ColorText::Green( "[+]" ) + " " + opTag + Message );
        else if ( MessageType == "Info" )
            this->DemonCommandInstance->DemonConsole->AppendRaw( Util::ColorText::Cyan( "[*]" ) + " " + opTag + Message );
        else if ( MessageType == "Warning" || MessageType == "Warn" )
            this->DemonCommandInstance->DemonConsole->AppendRaw( Util::ColorText::Yellow( "[!]" ) + " " + opTag + Message );
        else
            this->DemonCommandInstance->DemonConsole->AppendRaw( Util::ColorText::Purple( "[^]" ) + " " + opTag + Message );
    }

    if ( ! Output.isEmpty() )
    {
        //printf("task: %s\n", TaskID.toUtf8().constData());
        if (MugenX::callbackMessage)
        {
            PyObject *arglist = Py_BuildValue( "s", Output.toUtf8().constData() );
            PyObject_CallFunctionObjArgs( MugenX::callbackMessage, arglist, NULL );
            Py_XDECREF( MugenX::callbackMessage );
            MugenX::callbackMessage = NULL;
        }
        this->DemonCommandInstance->DemonConsole->AppendRaw( Output );
    }

    if ( JsonDocument[ "MiscType" ].toString().compare( "" ) != 0 )
    {
        auto Type = JsonDocument[ "MiscType" ].toString();
        auto Data = JsonDocument[ "MiscData" ].toString();

        if ( Type.compare( "screenshot" ) == 0 )
        {
            auto DecodedData = QByteArray::fromBase64( Data.toLocal8Bit() );
            auto Name        = JsonDocument[ "MiscData2" ].toString();

            MugenX::Teamserver.TabSession->LootWidget->AddScreenshot( DemonCommandInstance->DemonID, Name, Date, DecodedData );
        }
        else if ( Type.compare( "download" ) == 0 )
        {
            auto MiscDataInfo = JsonDocument[ "MiscData2" ].toString().split( ";" );
            auto Name         = QByteArray::fromBase64( MiscDataInfo[ 0 ].toLocal8Bit() );
            auto Size         = ( MiscDataInfo[ 1 ] );

            MugenX::Teamserver.TabSession->LootWidget->AddDownload( DemonCommandInstance->DemonID, Name, Size, Date, nullptr );
        }
        else if ( Type.compare( "ProcessUI" ) == 0 )
        {
            for ( auto& Session : MugenX::Teamserver.Sessions )
            {
                if ( Session.Name == DemonCommandInstance->DemonID )
                {
                    if ( Session.ProcessList )
                    {
                        auto Decoded = QByteArray::fromBase64( Data.toLocal8Bit() );
                        Session.ProcessList->UpdateProcessListJson( QJsonDocument::fromJson( Decoded ) );
                    }
                }
            }
        }
        else if ( Type.compare( "FileExplorer" ) == 0 )
        {
            for ( auto& Session : MugenX::Teamserver.Sessions )
            {
                if ( Session.Name == DemonCommandInstance->DemonID )
                {
                    if ( Session.FileBrowser )
                    {
                        auto Decoded = QByteArray::fromBase64( Data.toLocal8Bit() );
                        Session.FileBrowser->AddData( QJsonDocument::fromJson( Decoded ) );
                    }
                }
            }
        }
        else if ( Type.compare( "disconnect" ) == 0 )
        {
            MugenX::Teamserver.TabSession->SessionGraphWidget->GraphPivotNodeDisconnect( Data );
        }
        else if ( Type.compare( "reconnect" ) == 0 )
        {
            auto Split = Data.split( ";" );

            MugenX::Teamserver.TabSession->SessionGraphWidget->GraphPivotNodeReconnect( Split[ 0 ], Split[ 1 ] );
        }
    }
}
