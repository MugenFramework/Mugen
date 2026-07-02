#ifndef MUGEN_PYLOGGERCLASS_H
#define MUGEN_PYLOGGERCLASS_H

#include <UserInterface/MugenUI.hpp>
#include <global.hpp>

#include <QGridLayout>
#include <QTextEdit>
#include <QDialog>

typedef struct
{

    QWidget* window;
    QGridLayout* layout;
    QTextEdit* LogSection;

} PyLoggerQWindow, *PPyLoggerQWindow;

typedef struct
{
    PyObject_HEAD

    // Demon Info
    char* title;
    PPyLoggerQWindow LoggerWindow;

} PyLoggerClass, *PPyLoggerClass;

extern PyTypeObject PyLoggerClass_Type;

void        LoggerClass_dealloc( PPyLoggerClass self );
PyObject*   LoggerClass_new( PyTypeObject *type, PyObject *args, PyObject *kwds );
int         LoggerClass_init( PPyLoggerClass self, PyObject *args, PyObject *kwds );

// Methods

PyObject* LoggerClass_setBottomTab( PPyLoggerClass self, PyObject *args );
PyObject* LoggerClass_setSmallTab( PPyLoggerClass self, PyObject *args );
PyObject* LoggerClass_addText( PPyLoggerClass self, PyObject *args );
PyObject* LoggerClass_clear( PPyLoggerClass self, PyObject *args );

#endif
