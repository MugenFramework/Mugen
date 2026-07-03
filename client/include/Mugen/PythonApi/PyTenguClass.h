#ifndef MUGEN_PYTENGUCLASS_H
#define MUGEN_PYTENGUCLASS_H

#include <global.hpp>

typedef struct
{
    PyObject_HEAD

    char* TenguID;
    char* Listener;
    char* ExternalIP;
    char* InternalIP;
    char* User;
    char* Hostname;
    char* OS;
    char* Arch;

    u32 CONSOLE_INFO;
    u32 CONSOLE_ERROR;
    u32 CONSOLE_TASK;

} PyTenguClass, *PPyTenguClass;

extern PyTypeObject PyTenguClass_Type;

void        TenguClass_dealloc( PPyTenguClass self );
PyObject*   TenguClass_new( PyTypeObject* type, PyObject* args, PyObject* kwds );
int         TenguClass_init( PPyTenguClass self, PyObject* args, PyObject* kwds );

PyObject*   TenguClass_ConsoleWrite( PPyTenguClass self, PyObject* args );
PyObject*   TenguClass_Command( PPyTenguClass self, PyObject* args );

#endif // MUGEN_PYTENGUCLASS_H
