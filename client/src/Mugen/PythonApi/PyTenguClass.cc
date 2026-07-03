
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

#include <Mugen/PythonApi/PythonApi.h>
#include <Mugen/PythonApi/PyTenguClass.h>
#include <UserInterface/Widgets/DemonInteracted.h>
#include <Mugen/Service.hpp>
#include <Util/ColorText.h>

PyMemberDef PyTenguClass_members[] = {

    { "TenguID",      T_STRING, offsetof( PyTenguClass, TenguID ),    0, "Agent ID"     },
    { "Listener",     T_STRING, offsetof( PyTenguClass, Listener ),   0, "Listener"     },
    { "ExternalIP",   T_STRING, offsetof( PyTenguClass, ExternalIP ), 0, "External IP"  },
    { "InternalIP",   T_STRING, offsetof( PyTenguClass, InternalIP ), 0, "Internal IP"  },
    { "User",         T_STRING, offsetof( PyTenguClass, User ),       0, "Username"     },
    { "Hostname",     T_STRING, offsetof( PyTenguClass, Hostname ),   0, "Hostname"     },
    { "OS",           T_STRING, offsetof( PyTenguClass, OS ),         0, "OS string"    },
    { "Arch",         T_STRING, offsetof( PyTenguClass, Arch ),       0, "Architecture" },

    { "CONSOLE_INFO",  T_INT, offsetof( PyTenguClass, CONSOLE_INFO ),  0, "Console info type"  },
    { "CONSOLE_ERROR", T_INT, offsetof( PyTenguClass, CONSOLE_ERROR ), 0, "Console error type" },
    { "CONSOLE_TASK",  T_INT, offsetof( PyTenguClass, CONSOLE_TASK ),  0, "Console task type"  },

    { NULL },
};

PyMethodDef PyTenguClass_methods[] = {

    { "ConsoleWrite", ( PyCFunction ) TenguClass_ConsoleWrite, METH_VARARGS, "Write to the Tengu session console" },
    { "Command",      ( PyCFunction ) TenguClass_Command,      METH_VARARGS, "Dispatch a command to the Tengu session" },

    { NULL },
};

PyTypeObject PyTenguClass_Type = {
    PyVarObject_HEAD_INIT( &PyType_Type, 0 )

    "mugen.Tengu",                              /* tp_name */
    sizeof( PyTenguClass ),                     /* tp_basicsize */
    0,                                          /* tp_itemsize */
    ( destructor ) TenguClass_dealloc,          /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags */
    "Tengu Session Object",                     /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    PyTenguClass_methods,                       /* tp_methods */
    PyTenguClass_members,                       /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    ( initproc ) TenguClass_init,               /* tp_init */
    0,                                          /* tp_alloc */
    TenguClass_new,                             /* tp_new */
};

#define AllocMov( des, src, size )                          \
    if ( size > 0 )                                         \
    {                                                       \
        des = ( char* ) malloc( size * sizeof( char ) );    \
        memset( des, 0, size );                             \
        std::strcpy( des, src );                            \
    }

void TenguClass_dealloc( PPyTenguClass self )
{
    free( self->TenguID );
    free( self->Listener );
    free( self->ExternalIP );
    free( self->InternalIP );
    free( self->User );
    free( self->Hostname );
    free( self->OS );
    free( self->Arch );

    Py_TYPE( self )->tp_free( ( PyObject* ) self );
}

PyObject* TenguClass_new( PyTypeObject* type, PyObject* args, PyObject* kwds )
{
    PPyTenguClass self = ( PPyTenguClass ) PyType_Type.tp_alloc( type, 0 );
    return ( PyObject* ) self;
}

int TenguClass_init( PPyTenguClass self, PyObject* args, PyObject* kwds )
{
    if ( PyType_Type.tp_init( ( PyObject* ) self, args, kwds ) < 0 )
        return -1;

    char*       TenguID         = NULL;
    auto        Sessions        = MugenX::Teamserver.Sessions;
    uint32_t    NumSessions     = Sessions.size();
    const char* kwdlist[]       = { "TenguID", NULL };

    if ( ! PyArg_ParseTupleAndKeywords( args, kwds, "s", const_cast<char**>( kwdlist ), &TenguID ) )
        return -1;

    for ( int i = 0; i < NumSessions; ++i )
    {
        if ( Sessions[ i ].Name.compare( TenguID ) != 0 )
            continue;

        if ( Sessions[ i ].MagicValue != TenguMagicValue )
        {
            spdlog::error( "[PyApi] specified id is not a Tengu agent" );
            PyErr_SetString( PyExc_TypeError, "specified id is not a Tengu agent" );
            return -1;
        }

        AllocMov( self->TenguID,    Sessions[ i ].Name.toStdString().c_str(),     Sessions[ i ].Name.size()     );
        AllocMov( self->Listener,   Sessions[ i ].Listener.toStdString().c_str(), Sessions[ i ].Listener.size() );
        AllocMov( self->ExternalIP, Sessions[ i ].External.toStdString().c_str(), Sessions[ i ].External.size() );
        AllocMov( self->InternalIP, Sessions[ i ].Internal.toStdString().c_str(), Sessions[ i ].Internal.size() );
        AllocMov( self->User,       Sessions[ i ].User.toStdString().c_str(),     Sessions[ i ].User.size()     );
        AllocMov( self->Hostname,   Sessions[ i ].Computer.toStdString().c_str(), Sessions[ i ].Computer.size() );
        AllocMov( self->OS,         Sessions[ i ].OS.toStdString().c_str(),       Sessions[ i ].OS.size()       );
        AllocMov( self->Arch,       Sessions[ i ].Arch.toStdString().c_str(),     Sessions[ i ].Arch.size()     );

        self->CONSOLE_INFO  = 1;
        self->CONSOLE_ERROR = 2;
        self->CONSOLE_TASK  = 3;

        break;
    }

    return 0;
}

// Tengu.ConsoleWrite( type: int, message: str ) -> str | None
// CONSOLE_TASK returns a generated TaskID string.
// CONSOLE_INFO / CONSOLE_ERROR append directly to the session console.
PyObject* TenguClass_ConsoleWrite( PPyTenguClass self, PyObject* args )
{
    u32   Type    = 0;
    char* Message = NULL;

    if ( ! PyArg_ParseTuple( args, "is", &Type, &Message ) )
        Py_RETURN_NONE;

    for ( auto& s : MugenX::Teamserver.Sessions )
    {
        if ( s.Name.compare( self->TenguID ) != 0 )
            continue;

        if ( ! s.InteractedWidget || ! s.InteractedWidget->TenguCmds )
            break;

        if ( Type == self->CONSOLE_TASK )
        {
            auto TaskID = QString( Util::gen_random( 8 ).c_str() );
            auto header = Util::ColorText::Cyan( "[*]" ) + " " +
                          Util::ColorText::Comment( "[" + TaskID + "]" ) + " " +
                          Util::ColorText::Cyan( QString( Message ).toHtmlEscaped() );

            s.InteractedWidget->Console->append( header );
            return PyUnicode_FromString( TaskID.toStdString().c_str() );
        }
        else if ( Type == self->CONSOLE_INFO )
        {
            s.InteractedWidget->Console->append(
                Util::ColorText::Green( "[+]" ) + " " + QString( Message ).toHtmlEscaped()
            );
        }
        else if ( Type == self->CONSOLE_ERROR )
        {
            s.InteractedWidget->Console->append(
                Util::ColorText::Red( "[!]" ) + " " + QString( Message ).toHtmlEscaped()
            );
        }

        break;
    }

    Py_RETURN_NONE;
}

// Tengu.Command( taskID: str, command: str )
// Dispatches a raw command line to the Tengu session (same path as operator console input).
PyObject* TenguClass_Command( PPyTenguClass self, PyObject* args )
{
    char* TaskID  = NULL;
    char* Command = NULL;

    if ( ! PyArg_ParseTuple( args, "ss", &TaskID, &Command ) )
        return NULL;

    for ( auto& s : MugenX::Teamserver.Sessions )
    {
        if ( s.Name.compare( self->TenguID ) != 0 )
            continue;

        if ( s.InteractedWidget && s.InteractedWidget->TenguCmds )
            s.InteractedWidget->TenguCmds->DispatchCommand( true, QString( TaskID ), QString( Command ) );

        break;
    }

    Py_RETURN_NONE;
}
