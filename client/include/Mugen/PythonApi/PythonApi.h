#ifndef MUGEN_PYTHONAPI_H
#define MUGEN_PYTHONAPI_H

#include <global.hpp>
#pragma push_macro("slots")
#undef slots
#include <Python.h>
#pragma pop_macro("slots")

#define PY_FUNCTION( x )    PyObject* x( PyObject *self, PyObject *args );
#define PY_FUNCTION_KW( x ) PyObject* x( PyObject *self, PyObject *args, PyObject* kwargs );

namespace PythonAPI
{
    namespace Mugen
    {
        extern PyMethodDef PyMethode_Mugen[];

        namespace Core
        {
            PY_FUNCTION( Load )
            PY_FUNCTION( GetDemons )
            PY_FUNCTION( GetListeners )
            PY_FUNCTION( GetAgents )
            PY_FUNCTION_KW( GeneratePayload )
            PY_FUNCTION_KW( RegisterCommand )
            PY_FUNCTION_KW( RegisterTenguCommand )
            PY_FUNCTION( RegisterModule )
            PY_FUNCTION( RegisterCallback )
            PY_FUNCTION_KW( AddCredential )
        }

        namespace PyModule
        {
            extern struct PyModuleDef mugen;
        }

        PyMODINIT_FUNC PyInit_Mugen(void);
    }

    namespace MugenUI
    {
        extern PyMethodDef PyMethode_MugenUI[];

        namespace Core
        {
            PY_FUNCTION( MessageBox )
            PY_FUNCTION( ErrorMessage )
            PY_FUNCTION( CreateTab )
            PY_FUNCTION( InputDialog )
            PY_FUNCTION( OpenFileDialog )
            PY_FUNCTION( SaveFileDialog )
            PY_FUNCTION( QuestionDialog )
            PY_FUNCTION( ColorDialog )
            PY_FUNCTION( ProgressDialog )
        }

        namespace PyModule
        {
            extern struct PyModuleDef mugenui;
        }

        PyMODINIT_FUNC PyInit_MugenUI(void);

    }
}

namespace emb
{
    typedef std::function<void(std::string)> stdout_write_type;

    struct Stdout
    {
        PyObject_HEAD
        stdout_write_type write;
    };

    PyObject* Stdout_write(PyObject* self, PyObject* args);
    PyObject* Stdout_flush(PyObject* self, PyObject* args);
    PyMODINIT_FUNC PyInit_emb(void);
    void set_stdout(stdout_write_type write);
    void reset_stdout();
};

#endif
