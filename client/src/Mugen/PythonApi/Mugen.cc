#include <global.hpp>
#include <Mugen/Packager.hpp>
#include <Mugen/Connector.hpp>
#include <UserInterface/Widgets/LootWidget.h>
#include <UserInterface/Widgets/TeamserverTabSession.h>
#include <Mugen/DBManager/DBManager.hpp>
#include <QFile>
#include <QDateTime>

#include <Mugen/PythonApi/PythonApi.h>

#include <Mugen/PythonApi/PyDemonClass.h>
#include <Mugen/PythonApi/PyAgentClass.hpp>
#include <Mugen/PythonApi/Event.h>

#include <UserInterface/Widgets/DemonInteracted.h>

#include <QCompleter>

using namespace MugenNamespace::Util;

namespace PythonAPI::Mugen
{
    PyMethodDef PyMethode_Mugen[] = {
            { "LoadScript",       PythonAPI::Mugen::Core::Load,                            METH_VARARGS,                 "load python script"       },
            { "GetDemons",        PythonAPI::Mugen::Core::GetDemons,                       METH_VARARGS,                 "get list of demon ID's"   },
            { "GetListeners",     PythonAPI::Mugen::Core::GetListeners,                    METH_VARARGS,                 "get list of Listeners"   },
            { "GetAgents",        PythonAPI::Mugen::Core::GetAgents,                       METH_VARARGS,                 "get list of Agents"   },
            { "GeneratePayload",  ( PyCFunction ) PythonAPI::Mugen::Core::GeneratePayload, METH_VARARGS | METH_KEYWORDS, "Generate a payload and get the base64 bytestring" },
            { "RegisterCommand",       ( PyCFunction ) PythonAPI::Mugen::Core::RegisterCommand,       METH_VARARGS | METH_KEYWORDS, "register a command/alias"        },
            { "RegisterTenguCommand",  ( PyCFunction ) PythonAPI::Mugen::Core::RegisterTenguCommand,  METH_VARARGS | METH_KEYWORDS, "register a Tengu command/alias"  },
            { "RegisterModule",        PythonAPI::Mugen::Core::RegisterModule,                        METH_VARARGS,                 "register a module"               },
            { "RegisterCallback", PythonAPI::Mugen::Core::RegisterCallback,                METH_VARARGS,                 "register a callback"      },
            { "AddCredential",    ( PyCFunction ) PythonAPI::Mugen::Core::AddCredential,   METH_VARARGS | METH_KEYWORDS, "store a captured credential in the loot manager" },

            { NULL, NULL, 0, NULL }
    };

    namespace PyModule
    {
        struct PyModuleDef mugen = {
                PyModuleDef_HEAD_INIT,
                "mugen",
                "Python module to interact with Mugen",
                -1,
                PyMethode_Mugen
        };
    }
}

PyMODINIT_FUNC PythonAPI::Mugen::PyInit_Mugen( void )
{
    PyObject* Module = PyModule_Create2( &PythonAPI::Mugen::PyModule::mugen, PYTHON_API_VERSION );

    if ( PyType_Ready( &PyDemonClass_Type ) < 0 )
        spdlog::error( "Couldn't check if DemonClass is ready" );
    else
        PyModule_AddObject( Module, "Demon", (PyObject*) &PyDemonClass_Type );

    if ( PyType_Ready( &PyAgentClass_Type ) < 0 )
        spdlog::error( "Couldn't check if AgentClass is ready" );
    else
        PyModule_AddObject( Module, "Agent", (PyObject*) &PyAgentClass_Type );

    if ( PyType_Ready( &PyEventClass_Type ) < 0 )
        spdlog::error( "Couldn't check if Event class is ready" );
    else
        PyModule_AddObject( Module, "Event", (PyObject*) &PyEventClass_Type );

    return Module;
}

PyObject* PythonAPI::Mugen::Core::Load( PyObject *self, PyObject *args )
{
    char* FilePath = NULL;
    int   Return   = 0;

    if ( ! PyArg_ParseTuple( args, "s", &FilePath ) )
        Py_RETURN_NONE;

    auto script = FileRead( FilePath );

    spdlog::info( "Load Script: {}", FilePath );

    Return = PyRun_SimpleStringFlags( script.toStdString().c_str(), NULL );

    if ( Return == -1 ) {
        spdlog::error( "Failed to load script" );
        Py_RETURN_FALSE;
    }

    Py_RETURN_TRUE;
}

PyObject* PythonAPI::Mugen::Core::GetListeners( PyObject *self, PyObject *args )
{
    auto      Listeners        = MugenX::Teamserver.Listeners;
    uint32_t  NumberOfSessions = Listeners.size();
    PyObject* ListenerObjects  = PyList_New( NumberOfSessions );
    PyObject* ListenerID       = NULL;

    for ( int i = 0; i < NumberOfSessions; ++i )
    {
        ListenerID = Py_BuildValue( "s", Listeners[ i ].Name.c_str() );
        PyList_SetItem( ListenerObjects, i, ListenerID );
    }

    return ListenerObjects;
}

PyObject* PythonAPI::Mugen::Core::GetAgents( PyObject *self, PyObject *args )
{
    auto      Agents           = MugenX::Teamserver.ServiceAgents;
    uint32_t  NumberOfSessions = Agents.size();
    PyObject* AgentsObjects  = PyList_New( NumberOfSessions + 1);
    PyObject* AgentsID       = NULL;

    AgentsID = Py_BuildValue( "s", "Demon" );
    PyList_SetItem( AgentsObjects, 0, AgentsID );
    for ( int i = 1; i < NumberOfSessions; ++i )
    {
        AgentsID = Py_BuildValue( "s", Agents[ i ].Name.toStdString().c_str() );
        PyList_SetItem( AgentsObjects, i, AgentsID );
    }

    return AgentsObjects;
}

PyObject* PythonAPI::Mugen::Core::GetDemons( PyObject *self, PyObject *args )
{
    auto      DemonSessions    = MugenX::Teamserver.Sessions;
    uint32_t  NumberOfSessions = DemonSessions.size();
    PyObject* DemonObjects     = PyList_New( NumberOfSessions );
    PyObject* DemonID          = NULL;

    for ( int i = 0; i < NumberOfSessions; ++i )
    {
        DemonID = Py_BuildValue( "s", DemonSessions[ i ].Name.toStdString().c_str() );
        PyList_SetItem( DemonObjects, i, DemonID );
    }

    return DemonObjects;
}

PyObject* PythonAPI::Mugen::Core::GeneratePayload( PyObject *self, PyObject *args, PyObject* kwargs )
{
    PyObject*   callbackGate = nullptr;
    char*       agent = nullptr;
    char*       listener = nullptr;
    char*       arch = nullptr;
    char*       format_string = nullptr;
    char*       config = nullptr;
    const char* KeyWords[] = { "callback", "agent", "listener", "arch", "format", "config", NULL };

    if ( ! PyArg_ParseTupleAndKeywords( args, kwargs, "Osssss", const_cast<char**>(KeyWords), &callbackGate, &agent, &listener, &arch, &format_string, &config) )
        Py_RETURN_NONE;
    if ( !PyCallable_Check(callbackGate) )
    {
        PyErr_SetString(PyExc_TypeError, "parameter must be callable");
        return NULL;
    }
    MugenX::callbackGate = callbackGate;

    auto Package = new Util::Packager::Package;

    auto Head = Util::Packager::Head_t {
            .Event   = Util::Packager::Gate::Type,
            .User    = MugenX::Teamserver.User.toStdString(),
            .Time    = CurrentTime().toStdString(),
            .OneTime = "true",
    };

    auto Body = Util::Packager::Body_t {
            .SubEvent = Util::Packager::Gate::Stageless,
            .Info = {
                { "AgentType", std::string(agent) },
                { "Listener",  std::string(listener) },
                { "Arch",      std::string(arch) },
                { "Format",    std::string(format_string) },
                { "Config",    std::string(config) },
            },
    };


    Package->Head = Head;
    Package->Body = Body;

    MugenX::Connector->SendPackage( Package );

    Py_RETURN_NONE;
}

// RegisterCommand( PyFunction: func, Module: str, Command: str, Description: str, Behavior: int, Usage: str, Example: str )
PyObject* PythonAPI::Mugen::Core::RegisterCommand( PyObject *self, PyObject *args, PyObject* kwargs )
{
    RegisteredCommand RCommand = { };

    PVOID Function         = nullptr;
    PCHAR Agent            = nullptr;
    PCHAR Module           = nullptr;
    PCHAR Command          = nullptr;
    PCHAR Description      = nullptr;
    PCHAR Usage            = nullptr;
    PCHAR Example          = nullptr;
    u32   Behavior         = 0;
    auto  CompleteText     = QString();
    auto  Path             = MugenX::Teamserver.LoadingScript;
    const char* KeyWords[] = { "function", "module", "command", "description", "behavior", "usage", "example", "agent", NULL };
    const char* format     = "Osssiss|s";

    if ( ! PyArg_ParseTupleAndKeywords( args, kwargs, format, const_cast<char**>(KeyWords), &Function, &Module, &Command, &Description, &Behavior, &Usage, &Example, &Agent ) )
        Py_RETURN_NONE;

    if ( Agent != nullptr )
        RCommand.Agent = Agent;
    else
        RCommand.Agent = "Demon"; /* if the 'agent' keyword hasn't been specified then use the demon agent by default */

    RCommand.Function  = Function;
    RCommand.Module    = Module;
    RCommand.Command   = Command;
    RCommand.Help      = Description;
    RCommand.Behaviour = Behavior;
    RCommand.Usage     = Usage;
    RCommand.Example   = Example;
    RCommand.Path      = Path.substr( 0, Path.find_last_of( "\\/" ) );

    if ( QString( RCommand.Module.c_str() ).length() > 0 ) {
        spdlog::debug( "Registered command: {} {}", Module, Command );
    } else {
        spdlog::debug( "Registered command: {}", Command );
    }

    // Check if command already exists... if it is already existing then replace it with new one.
    for ( u32 i = 0; i < MugenX::Teamserver.RegisteredCommands.size(); i++ )
    {
        auto c = MugenX::Teamserver.RegisteredCommands[ i ];

        if ( ( c.Command == RCommand.Command ) && ( c.Module == RCommand.Module ) && ( c.Agent == RCommand.Agent ) )
        {
            spdlog::debug( "Command already exists: [Module: {}] [Command: {}]", RCommand.Module, RCommand.Command );
            MugenX::Teamserver.RegisteredCommands[ i ] = RCommand;

            Py_RETURN_NONE;
        }
    }

    if ( ! RCommand.Module.empty() )
        CompleteText = QString( RCommand.Module.c_str() ) + " " + QString( RCommand.Command.c_str() );
    else
        CompleteText = QString( RCommand.Command.c_str() );

    // Add autocomplete to sessions of the matching agent type.
    auto Sessions = MugenX::Teamserver.Sessions;
    for ( u32 i = 0; i < Sessions.size(); i++ )
    {
        auto w = Sessions[ i ].InteractedWidget;
        if ( w == nullptr ) continue;

        bool isTengu = ( w->TenguCmds != nullptr );
        if ( ( RCommand.Agent == "Tengu" ) == isTengu )
        {
            w->AutoCompleteAdd( CompleteText );
            w->AutoCompleteAdd( "help " + CompleteText );
        }
    }

    MugenX::Teamserver.AddedCommands << CompleteText;

    // Add new command
    MugenX::Teamserver.RegisteredCommands.push_back( RCommand );
    Py_XINCREF( RCommand.Function );

    Py_RETURN_NONE;
}

// RegisterTenguCommand( func, module, command, description, behavior, usage, example )
// Convenience wrapper - identical to RegisterCommand but defaults agent to "Tengu".
PyObject* PythonAPI::Mugen::Core::RegisterTenguCommand( PyObject *self, PyObject *args, PyObject* kwargs )
{
    RegisteredCommand RCommand = { };

    PVOID Function         = nullptr;
    PCHAR Module           = nullptr;
    PCHAR Command          = nullptr;
    PCHAR Description      = nullptr;
    PCHAR Usage            = nullptr;
    PCHAR Example          = nullptr;
    u32   Behavior         = 0;
    auto  CompleteText     = QString();
    auto  Path             = MugenX::Teamserver.LoadingScript;
    const char* KeyWords[] = { "function", "module", "command", "description", "behavior", "usage", "example", NULL };
    const char* format     = "Osssiss";

    if ( ! PyArg_ParseTupleAndKeywords( args, kwargs, format, const_cast<char**>(KeyWords), &Function, &Module, &Command, &Description, &Behavior, &Usage, &Example ) )
        Py_RETURN_NONE;

    RCommand.Agent     = "Tengu";
    RCommand.Function  = Function;
    RCommand.Module    = Module;
    RCommand.Command   = Command;
    RCommand.Help      = Description;
    RCommand.Behaviour = Behavior;
    RCommand.Usage     = Usage;
    RCommand.Example   = Example;
    RCommand.Path      = Path.substr( 0, Path.find_last_of( "\\/" ) );

    spdlog::debug( "Registered Tengu command: {} {}", Module, Command );

    for ( u32 i = 0; i < MugenX::Teamserver.RegisteredCommands.size(); i++ )
    {
        auto c = MugenX::Teamserver.RegisteredCommands[ i ];
        if ( ( c.Command == RCommand.Command ) && ( c.Module == RCommand.Module ) && ( c.Agent == "Tengu" ) )
        {
            MugenX::Teamserver.RegisteredCommands[ i ] = RCommand;
            Py_RETURN_NONE;
        }
    }

    if ( ! RCommand.Module.empty() )
        CompleteText = QString( RCommand.Module.c_str() ) + " " + QString( RCommand.Command.c_str() );
    else
        CompleteText = QString( RCommand.Command.c_str() );

    auto Sessions = MugenX::Teamserver.Sessions;
    for ( u32 i = 0; i < Sessions.size(); i++ )
    {
        auto w = Sessions[ i ].InteractedWidget;
        if ( w != nullptr && w->TenguCmds != nullptr )
        {
            w->AutoCompleteAdd( CompleteText );
            w->AutoCompleteAdd( "help " + CompleteText );
        }
    }

    MugenX::Teamserver.AddedCommands << CompleteText;

    MugenX::Teamserver.RegisteredCommands.push_back( RCommand );
    Py_XINCREF( RCommand.Function );

    Py_RETURN_NONE;
}

// RegisterModule( Name: str, Description: str, Behavior: str, Usage: str, Example: str, Options: str )
PyObject* PythonAPI::Mugen::Core::RegisterModule( PyObject *self, PyObject *args )
{
    spdlog::debug( "PythonAPI::Mugen::Core::RegisterModule" );
    RegisteredModule Module = {};

    PCHAR Name         = nullptr;
    PCHAR Description  = nullptr;
    PCHAR Behavior     = nullptr;
    PCHAR Usage        = nullptr;
    PCHAR Example      = nullptr;
    PCHAR Options      = nullptr;
    auto  CompleteText = QString();

    if( ! PyArg_ParseTuple( args, "ssssss", &Name, &Description, &Behavior, &Usage, &Example, &Options ) )
        Py_RETURN_NONE;

    Module.Name         = Name;
    Module.Description  = Description;
    Module.Behavior     = Behavior;
    Module.Usage        = Usage;
    Module.Example      = Example;

    // Check if module already exists... if it is already existing then replace it with new one.
    for ( u32 i = 0; i < MugenX::Teamserver.RegisteredModules.size(); i++ )
    {
        auto c = MugenX::Teamserver.RegisteredModules[ i ];

        if ( ( c.Name == Module.Name ) && ( c.Agent == Module.Agent ) )
        {
            spdlog::debug( "Module already exists: [Module: {}]", Module.Name );
            MugenX::Teamserver.RegisteredModules[ i ] = Module;

            Py_RETURN_NONE;
        }
    }

    CompleteText = QString( Module.Name.c_str() );

    // TODO: further test this. Reload or load new scripts that make use of RegisterCommand
    auto Sessions = MugenX::Teamserver.Sessions;
    for ( u32 i = 0; i < Sessions.size(); i++ )
    {
        Sessions[ i ].InteractedWidget->AutoCompleteAdd( CompleteText );
        Sessions[ i ].InteractedWidget->AutoCompleteAdd( "help " + CompleteText );
    }

    MugenX::Teamserver.AddedCommands << CompleteText;

    // Add new command
    MugenX::Teamserver.RegisteredModules.push_back( Module );

    spdlog::debug( "Registered module: {}", Module.Name );

    Py_RETURN_NONE;
}

PyObject* PythonAPI::Mugen::Core::RegisterCallback( PyObject *self, PyObject *args )
{
    spdlog::debug( "PythonAPI::Mugen::Core::RegisterCallback" );

    PyObject* Callback = nullptr;

    if ( ! PyArg_ParseTuple( args, "O", &Callback ) )
    {
        spdlog::error( "Invalid parameters on RegisterCallback" );
        return nullptr;
    }

    if ( ! PyCallable_Check( Callback ) )
    {
        spdlog::error( "The callback is not callable" );
        return nullptr;
    }

    MugenX::Teamserver.RegisteredCallbacks.push_back( Callback );
    Py_XINCREF( Callback );

    Py_RETURN_NONE;
}

// AddCredential(agent_id, cred_type, username, secret, domain="", source="")
// Stores a credential in the loot manager and persists it to the DB.
PyObject* PythonAPI::Mugen::Core::AddCredential( PyObject* self, PyObject* args, PyObject* kwargs )
{
    char* agent_id  = nullptr;
    char* cred_type = nullptr;
    char* username  = nullptr;
    char* secret    = nullptr;
    char* domain    = const_cast<char*>( "" );
    char* source    = const_cast<char*>( "" );

    const char* kw[] = { "agent_id", "cred_type", "username", "secret", "domain", "source", nullptr };

    if ( ! PyArg_ParseTupleAndKeywords( args, kwargs, "ssss|ss", const_cast<char**>( kw ),
                                        &agent_id, &cred_type, &username, &secret, &domain, &source ) )
        Py_RETURN_NONE;

    auto ts  = QString( agent_id );
    auto typ = QString( cred_type );
    auto usr = QString( username );
    auto sec = QString( secret );
    auto dom = QString( domain );
    auto src = QString( source );
    auto now = QDateTime::currentDateTime().toString( "yyyy-MM-dd hh:mm:ss" );

    // Persist to DB
    if ( MugenX::Teamserver.TabSession && MugenX::Teamserver.TabSession->dbManager ) {
        MugenNamespace::MugenSpace::DBManager::CredentialEntry entry{ ts, typ, usr, sec, dom, src, now };
        MugenX::Teamserver.TabSession->dbManager->AddCredential( entry );
    }

    // Add to live loot widget
    if ( MugenX::Teamserver.TabSession && MugenX::Teamserver.TabSession->LootWidget ) {
        MugenX::Teamserver.TabSession->LootWidget->AddSessionSection( ts );
        MugenX::Teamserver.TabSession->LootWidget->AddCredential( ts, typ, usr, sec, dom, src, now );
    }

    Py_RETURN_NONE;
}
