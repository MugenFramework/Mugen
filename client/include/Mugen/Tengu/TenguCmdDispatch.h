#ifndef MUGEN_TENGUCMDDISPATCH_H
#define MUGEN_TENGUCMDDISPATCH_H

#include <global.hpp>
#include <Mugen/Packager.hpp>
#include <Util/ColorText.h>

#include <QMap>
#include <QStringList>

// Forward declaration
namespace MugenNamespace::UserInterface::Widgets {
    class DemonInteracted;
}

namespace MugenNamespace::MugenSpace {

class TenguCommands
{
public:
    UserInterface::Widgets::DemonInteracted* TenguConsole = nullptr;

    QString Teamserver;
    QString AgentID;
    u64     MagicValue  = 0;
    QString AgentTypeName = "Tengu";
    QString Prompt;

    QMap<QString, QString> CommandInputList;

    auto SetTenguConsole( UserInterface::Widgets::DemonInteracted* pInteracted ) -> void;
    auto DispatchCommand( bool Send, QString TaskID, const QString& commandline ) -> bool;
    auto SendTask( const QString& TaskID, const QString& command ) -> void;

private:
    auto ConsoleInfo( bool Send, QString TaskID, const QString& text ) -> QString;
    auto ConsoleError( const QString& text ) -> void;
};

} // namespace MugenNamespace::MugenSpace

#endif // MUGEN_TENGUCMDDISPATCH_H
