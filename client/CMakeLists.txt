cmake_minimum_required( VERSION 3.15 )
project( Mugen )

##
## cmake sets
##
set( QT_VERSION 5 )
set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_AUTOMOC ON )
set( CMAKE_AUTORCC ON )
set( CMAKE_AUTOUIC ON )
set( CMAKE_CPP_COMPILER /usr/bin/x86_64-w64-mingw32-g++ )
set( CMAKE_C_COMPILER /usr/bin/x86_64-w64-mingw32-gcc )
set( CMAKE_CXX_FLAGS "-fpermissive" )
set( REQUIRED_LIBS Core Gui Widgets Network WebSockets Sql )
set( REQUIRED_LIBS_QUALIFIED Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network Qt5::WebSockets Qt5::Sql )
set( APP_ICON_RESOURCE_WINDOWS "data/Mugen.qrc" )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY .. )
set( CMAKE_INCLUDE_CURRENT_DIR ON )

##
## Ensure 3.10 is used if present.
## If not, modules will not work within the client.
##
set( Python_ADDITIONAL_VERSIONS 3.10 )

##
## import includes and external libraries
##
include_directories( include  )
include_directories( external/spdlog/include )
include_directories( external/json/include )
include_directories( external/toml )

##
## find qt packages
##
find_package( Qt${QT_VERSION} COMPONENTS ${REQUIRED_LIBS} REQUIRED )

##
## apple support
##
if(APPLE)
  find_package( Python 3 COMPONENTS Interpreter Development REQUIRED )
  set( PYTHON_MAJOR $ENV{Python_VERSION_MAJOR} )
  set( PYTHON_MINOR $ENV{Python_VERSION_MINOR} )
  set( PYTHONLIBS_VERSION_STRING ${Python_VERSION} )
  set( PYTHON_INCLUDE_DIR ${Python_INCLUDE_DIRS} )
  set( PYTHON_LIBRARIES ${Python_LIBRARIES} )
  message( "Apple - Using Python:${Python_VERSION_MAJOR} - Libraries:${PYTHON_LIBRARIES} - includeDirs: ${PYTHON_INCLUDE_DIR}" )
elseif(UNIX)
  find_package( PythonLibs 3 REQUIRED )
else()
  set( PYTHONLIBS_VERSION_STRING $ENV{PY_VERSION} )
endif()



if(APPLE)
    execute_process( COMMAND brew --prefix OUTPUT_VARIABLE BREW_PREFIX ) #this because brew install location differs Intel/Apple Silicon macs
    string( STRIP ${BREW_PREFIX} BREW_PREFIX ) #for some reason this happens: https://gitlab.kitware.com/cmake/cmake/-/issues/22404
    include_directories( "${BREW_PREFIX}/bin/python3.10" )
    include_directories( "${BREW_PREFIX}/Frameworks/Python.framework/Headers" )
elseif(UNIX)
    include_directories( ${PYTHON_INCLUDE_DIRS} )
endif()

set( MUGEN_INCLUDE

        # Misc
        include/Mugen/Connector.hpp
        include/Mugen/DBManager/DBManager.hpp
        include/Mugen/DemonCmdDispatch.h
        include/Mugen/Packager.hpp
        include/Mugen/Mugen.hpp
        include/Mugen/Service.hpp
        include/Mugen/CmdLine.hpp

        # Python Api
        include/Mugen/PythonApi/PythonApi.h
        include/Mugen/PythonApi/Event.h
        include/Mugen/PythonApi/MugenUi.h
        include/Mugen/PythonApi/PyAgentClass.hpp
        include/Mugen/PythonApi/PyTenguClass.h
        include/Mugen/PythonApi/UI/PyDialogClass.hpp
        include/Mugen/PythonApi/UI/PyLoggerClass.hpp
        include/Mugen/PythonApi/UI/PyTreeClass.hpp
        include/Mugen/PythonApi/UI/PyWidgetClass.hpp

        # Dialogs
        include/UserInterface/Dialogs/Payload.hpp
        include/UserInterface/Dialogs/About.hpp
        include/UserInterface/Dialogs/Connect.hpp
        include/UserInterface/Dialogs/Listener.hpp

        # small widgets
        include/UserInterface/SmallWidgets/EventViewer.hpp
        include/UserInterface/SmallWidgets/EventViewer.hpp

        # widgets
        include/UserInterface/Widgets/Chat.hpp
        include/UserInterface/Widgets/DemonInteracted.h
        include/UserInterface/Widgets/ListenerTable.hpp
        include/UserInterface/Widgets/ProcessList.hpp
        include/UserInterface/Widgets/PythonScript.hpp
        include/UserInterface/Widgets/SessionTable.hpp
        include/UserInterface/Widgets/SessionGraph.hpp
        include/UserInterface/Widgets/TeamserverTabSession.h
        include/UserInterface/Widgets/SplitConsoleWidget.hpp
        include/UserInterface/Widgets/ScriptManager.h
        include/UserInterface/Widgets/LootWidget.h
        include/Mugen/DBManager/DBManager.hpp
        include/UserInterface/Widgets/FileBrowser.hpp
        include/UserInterface/Widgets/Teamserver.hpp
        include/UserInterface/Widgets/Store.hpp
        include/UserInterface/Widgets/DashboardWidget.hpp
        include/UserInterface/Widgets/NetworkingWidget.hpp
        include/UserInterface/Widgets/ResourceManagerWidget.hpp
        include/UserInterface/Widgets/TasksWidget.hpp

        include/UserInterface/MugenUI.hpp
)

# src for UI
set(
        MUGEN_UI

        src/UserInterface/MugenUi.cc
        src/global.cc

        # Dialogs
        src/UserInterface/Dialogs/About.cc
        src/UserInterface/Dialogs/Connect.cc
        src/UserInterface/Dialogs/Listener.cc
        src/UserInterface/Dialogs/Payload.cc

        # Widgets
        src/UserInterface/Widgets/SessionTable.cc
        src/UserInterface/Widgets/SessionGraph.cc
        src/UserInterface/Widgets/Chat.cc
        src/UserInterface/Widgets/ListenersTable.cc
        src/UserInterface/Widgets/DemonInteracted.cc
        src/UserInterface/Widgets/TeamserverTabSession.cc
        src/UserInterface/Widgets/SplitConsoleWidget.cc
        src/UserInterface/Widgets/PythonScript.cc
        src/UserInterface/Widgets/ScriptManager.cc
        src/UserInterface/Widgets/LootWidget.cc
        src/UserInterface/Widgets/FileBrowser.cc
        src/UserInterface/Widgets/Teamserver.cc
        src/UserInterface/Widgets/Store.cc
        src/UserInterface/Widgets/ProcessList.cc
        src/UserInterface/Widgets/DashboardWidget.cc
        src/UserInterface/Widgets/NetworkingWidget.cc
        src/UserInterface/Widgets/ResourceManagerWidget.cc
        src/UserInterface/Widgets/TasksWidget.cc

        # SmallWidgets
        src/UserInterface/SmallWidgets/EventViewer.cc
)

set(
        MUGEN_UTIL

        src/Util/ColorText.cpp
        src/Util/Base64.cpp
        src/Util/Base.cpp
)

# src for Mugen
set( MUGEN_SOURCE
        src/Main.cc

        src/Mugen/Packager.cc
        src/Mugen/Connector.cc
        src/Mugen/Service.cc

        src/Mugen/DBManger/DBManager.cc
        src/Mugen/DBManger/Teamserver.cc
        src/Mugen/DBManger/Scripts.cc
        src/Mugen/DBManger/Credentials.cc
        src/Mugen/DBManger/SessionMeta.cc

        src/Mugen/Demon/ConsoleInput.cc
        src/Mugen/Demon/CommandSend.cc
        src/Mugen/Demon/CommandOutput.cc
        src/Mugen/Demon/Commands.cc

        src/Mugen/Tengu/TenguConsoleInput.cc

        src/Mugen/PythonApi/PythonApi.cc
        src/Mugen/PythonApi/Mugen.cc
        src/Mugen/PythonApi/MugenUi.cc
        src/Mugen/PythonApi/UI/PyDialogClass.cc
        src/Mugen/PythonApi/UI/PyLoggerClass.cc
        src/Mugen/PythonApi/UI/PyTreeClass.cc
        src/Mugen/PythonApi/UI/PyWidgetClass.cc
        src/Mugen/PythonApi/PyDemonClass.cc
        src/Mugen/PythonApi/PyTenguClass.cc
        src/Mugen/PythonApi/Event.cc
        src/Mugen/PythonApi/PyAgentClass.cc

        src/Mugen/Mugen.cc

    ${MUGEN_UI}
)

add_executable( ${PROJECT_NAME} ${MUGEN_INCLUDE} ${MUGEN_SOURCE} ${MUGEN_UTIL} ${APP_ICON_RESOURCE_WINDOWS} data/Mugen.rc )
add_definitions( -DQT_NO_DEBUG_OUTPUT )

target_link_libraries(
    ${PROJECT_NAME}
    ${REQUIRED_LIBS_QUALIFIED}
    ${PYTHON_LIBRARIES}
    # spdlog::spdlog
    # spdlog::spdlog_header_only
)
