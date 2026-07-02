#include <Mugen/PythonApi/PythonApi.h>
#include <UserInterface/MugenUI.hpp>

#include <Mugen/PythonApi/UI/PyWidgetClass.hpp>
#include <Mugen/PythonApi/UI/PyDialogClass.hpp>
#include <Mugen/PythonApi/UI/PyLoggerClass.hpp>
#include <Mugen/PythonApi/UI/PyTreeClass.hpp>

#include <QFile>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QColorDialog>
#include <QProgressDialog>
#include <QTimer>
#include <QErrorMessage>

namespace PythonAPI::MugenUI
{
    PyMethodDef PyMethode_MugenUI[] = {
            { "messagebox", PythonAPI::MugenUI::Core::MessageBox, METH_VARARGS, "Python interface for Mugen Messagebox" },
            { "errormessage", PythonAPI::MugenUI::Core::ErrorMessage, METH_VARARGS, "Python interface for Mugen Error Message" },
            { "createtab", PythonAPI::MugenUI::Core::CreateTab, METH_VARARGS, "Python interface for Mugen Tabs" },
            { "inputdialog", PythonAPI::MugenUI::Core::InputDialog, METH_VARARGS, "Python interface for Mugen InputDialog" },
            { "openfiledialog", PythonAPI::MugenUI::Core::OpenFileDialog, METH_VARARGS, "Python interface for Mugen InputDialog" },
            { "savefiledialog", PythonAPI::MugenUI::Core::SaveFileDialog, METH_VARARGS, "Python interface for Mugen InputDialog" },
            { "questiondialog", PythonAPI::MugenUI::Core::QuestionDialog, METH_VARARGS, "Python interface for Mugen InputDialog" },
            { "colordialog", PythonAPI::MugenUI::Core::ColorDialog, METH_VARARGS, "Python interface for Mugen ColorDialog" },
            { "progressdialog", PythonAPI::MugenUI::Core::ProgressDialog, METH_VARARGS, "Python interface for Mugen ColorDialog" },

            { NULL, NULL, 0, NULL }
    };

    namespace PyModule
    {
        struct PyModuleDef mugenui = {
                PyModuleDef_HEAD_INIT,
                "mugenui",
                "Python module for Mugen Interface UI",
                -1,
                PyMethode_MugenUI
        };
    }
}

PyObject* PythonAPI::MugenUI::Core::CreateTab(PyObject *self, PyObject *args)
{
    const char *title = nullptr;
    const char *in_menu = nullptr;

    if ( !MugenX::MugenUserInterface || !MugenX::MugenUserInterface->menubar )
    {
        Py_RETURN_NONE;
    }
    Py_ssize_t tuple_size = PyTuple_Size(args);
    title = (const char *)PyUnicode_AsUTF8(PyTuple_GetItem(args, 0));
    auto *menubar= MugenX::MugenUserInterface->menubar;
    auto tab = menubar->addMenu(title);
    if ( !tab )
    {
        Py_RETURN_NONE;
    }
    for (Py_ssize_t i = 1; i < tuple_size; i+=2) {
        const char * string_obj = PyUnicode_AsUTF8(PyTuple_GetItem(args, i));
        PyObject* callable_obj = PyTuple_GetItem(args, i + 1);
        if ( !PyCallable_Check(callable_obj) )
        {
            PyErr_SetString(PyExc_TypeError, "parameter must be callable");
            return NULL;
        }
        auto tupleCallback = new QAction( MugenX::MugenUserInterface->MugenWindow );

        tupleCallback->setObjectName(QString::fromUtf8(string_obj));
        tupleCallback->setText(string_obj);
        tab->addAction(tupleCallback);
        QMainWindow::connect( tupleCallback, &QAction::triggered, MugenX::MugenUserInterface->MugenWindow, [callable_obj]() {
            PyObject_CallFunctionObjArgs(callable_obj, nullptr);
        });
    }
    Py_RETURN_NONE;
}


PyObject* PythonAPI::MugenUI::Core::MessageBox(PyObject *self, PyObject *args)
{
    char *title = nullptr, *content = nullptr;

    if( !PyArg_ParseTuple( args, "ss", &title, &content ) )
    {
        Py_RETURN_NONE;
    }

    QFile messageBoxStyleSheets(":/stylesheets/MessageBox");
    QMessageBox messageBox;

    messageBoxStyleSheets.open(QIODevice::ReadOnly);

    messageBox.setWindowTitle(title);
    messageBox.setText(content);
    messageBox.setIcon(QMessageBox::Information);
    messageBox.setStyleSheet(messageBoxStyleSheets.readAll());

    messageBox.exec();

    Py_RETURN_NONE;
}

PyObject* PythonAPI::MugenUI::Core::ErrorMessage(PyObject *self, PyObject *args)
{
    char *message = nullptr;

    if( !PyArg_ParseTuple( args, "s", &message ) )
    {
        Py_RETURN_NONE;
    }

    QErrorMessage* errorMessage = new QErrorMessage(MugenX::MugenUserInterface->MugenWindow);
    errorMessage->showMessage(message);

    Py_RETURN_NONE;
}

PyObject* PythonAPI::MugenUI::Core::QuestionDialog(PyObject *self, PyObject *args)
{
    char *title = nullptr, *content = nullptr;

    if( !PyArg_ParseTuple( args, "ss", &title, &content ) )
    {
        Py_RETURN_NONE;
    }

    QMessageBox::StandardButton result = QMessageBox::question(MugenX::MugenUserInterface->MugenWindow, title, content, QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

PyObject* PythonAPI::MugenUI::Core::InputDialog(PyObject *self, PyObject *args)
{
    char *title = nullptr, *content = nullptr;

    if( !PyArg_ParseTuple( args, "ss", &title, &content ) )
    {
        Py_RETURN_NONE;
    }
    QString data = QInputDialog::getText(
                    MugenX::MugenUserInterface->MugenWindow, title, content);
    return PyBytes_FromString(data.toStdString().c_str());
}

PyObject* PythonAPI::MugenUI::Core::OpenFileDialog(PyObject *self, PyObject *args)
{
    char *title = nullptr;

    if( !PyArg_ParseTuple( args, "s", &title) )
    {
        Py_RETURN_NONE;
    }
    QString data = QFileDialog::getOpenFileName(
                    MugenX::MugenUserInterface->MugenWindow, title, QDir::homePath());
    return PyBytes_FromString(data.toStdString().c_str());
}

PyObject* PythonAPI::MugenUI::Core::SaveFileDialog(PyObject *self, PyObject *args)
{
    char *title = nullptr;

    if( !PyArg_ParseTuple( args, "s", &title) )
    {
        Py_RETURN_NONE;
    }
    QString data = QFileDialog::getSaveFileName(
                    MugenX::MugenUserInterface->MugenWindow, title, QDir::homePath());
    return PyBytes_FromString(data.toStdString().c_str());
}

PyObject* PythonAPI::MugenUI::Core::ColorDialog(PyObject *self, PyObject *args)
{
    QColorDialog data = QColorDialog(MugenX::MugenUserInterface->MugenWindow);
    QColor sel = data.getColor();
    if (sel.isValid()) {
        QString colorHex = sel.name();
        return PyBytes_FromString(colorHex.toStdString().c_str());
    } else {
        Py_RETURN_NONE;
    }
}

PyObject* PythonAPI::MugenUI::Core::ProgressDialog(PyObject *self, PyObject *args)
{
    char *title = nullptr;
    char *text= nullptr;
    int max_num = 0;
    PyObject* callable_obj = nullptr;

    if( !PyArg_ParseTuple( args, "ssOi", &title, &text, &callable_obj, &max_num) )
    {
        Py_RETURN_NONE;
    }
    if ( !PyCallable_Check(callable_obj) )
    {
        PyErr_SetString(PyExc_TypeError, "parameter must be callable");
        return NULL;
    }
    QProgressDialog* dialog = new QProgressDialog(title, text, 0, max_num, MugenX::MugenUserInterface->MugenWindow);
    dialog->setAutoClose(false);
    QTimer* timer = new QTimer();

    QMainWindow::connect( timer, &QTimer::timeout, MugenX::MugenUserInterface->MugenWindow, [callable_obj, dialog, timer]() {
        PyObject *pResult = PyObject_CallFunctionObjArgs(callable_obj, nullptr);

        if (pResult != NULL) {
            if (PyLong_Check(pResult)) {
                long resultInt = PyLong_AsLong(pResult);
                dialog->setValue(resultInt);
                if (resultInt < 0) {
                    dialog->close();
                    timer->stop();
                }
            }
        } else {
            PyErr_SetString(PyExc_TypeError, "Function needs to return an int");
            dialog->close();
            timer->stop();
        }
    });
    QPushButton *cancelButton = dialog->findChild<QPushButton *>();
    QMainWindow::connect( cancelButton, &QPushButton::clicked, MugenX::MugenUserInterface->MugenWindow, [dialog, timer]() {
        dialog->close();
        timer->stop();
    });
    timer->start(max_num);
    dialog->exec();

    Py_RETURN_NONE;
}

PyMODINIT_FUNC PythonAPI::MugenUI::PyInit_MugenUI(void)
{
    PyObject* Module = PyModule_Create2( &PythonAPI::MugenUI::PyModule::mugenui, PYTHON_API_VERSION );

    if ( PyType_Ready( &PyWidgetClass_Type ) < 0 )
        spdlog::error( "Couldn't check if WidgetClass is ready" );
    else
        PyModule_AddObject( Module, "Widget", (PyObject*) &PyWidgetClass_Type );

    if ( PyType_Ready( &PyDialogClass_Type ) < 0 )
        spdlog::error( "Couldn't check if DialogClass is ready" );
    else
        PyModule_AddObject( Module, "Dialog", (PyObject*) &PyDialogClass_Type );

    if ( PyType_Ready( &PyLoggerClass_Type ) < 0 )
        spdlog::error( "Couldn't check if LoggerClass is ready" );
    else
        PyModule_AddObject( Module, "Logger", (PyObject*) &PyLoggerClass_Type );

    if ( PyType_Ready( &PyTreeClass_Type ) < 0 )
        spdlog::error( "Couldn't check if TreeClass is ready" );
    else
        PyModule_AddObject( Module, "Tree", (PyObject*) &PyTreeClass_Type );

    return Module;
}
