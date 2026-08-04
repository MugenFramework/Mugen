#ifndef MUGEN_TEAMSERVERTABSESSION_H
#define MUGEN_TEAMSERVERTABSESSION_H

#include <global.hpp>
#include <QStackedWidget>
#include <QSplitter>

#include <UserInterface/Widgets/LootWidget.h>
#include <UserInterface/Widgets/SessionGraph.hpp>
#include <UserInterface/Widgets/SessionMap.hpp>
#include <UserInterface/Widgets/Teamserver.hpp>
#include <UserInterface/Widgets/NetworkingWidget.hpp>
#include <UserInterface/Widgets/ResourceManagerWidget.hpp>

class DashboardWidget;
#include <UserInterface/Widgets/Store.hpp>
#include <UserInterface/Widgets/SplitConsoleWidget.hpp>

#include <UserInterface/Dialogs/Payload.hpp>

using namespace MugenNamespace;

class MugenNamespace::UserInterface::Widgets::TeamserverTabSession : public QWidget
{
    typedef struct
    {
        UserInterface::SmallWidgets::EventViewer* EventViewer;
    } SmallAppWidgets_t;

public:
    QGridLayout* gridLayout              = {};
    QGridLayout* gridLayout_2            = {};
    QWidget*     layoutWidget            = {};
    QSplitter*   splitter_TopBot         = {};
    QSplitter*   splitter_SessionAndTabs = {};
    QVBoxLayout* verticalLayout          = {};
    QTabWidget*  tabWidget               = {};
    QTabWidget*  tabWidgetSmall          = {};

public:
    Widgets::Chat*                    TeamserverChat      = {};
    class Teamserver*                 Teamserver          = {};
    class Store*                      Store               = {};
    Widgets::SessionTable*            SessionTableWidget  = {};
    GraphWidget*                      SessionGraphWidget  = {};
    SessionMap*                       SessionMapWidget    = {};
    DashboardWidget*                  Dashboard           = {};
    Widgets::ListenersTable*          ListenerTableWidget = {};
    Widgets::NetworkingWidget*        NetworkingView      = {};
    Widgets::ResourceManagerWidget*   ResourceManager     = {};
    Widgets::PythonScriptInterpreter* PythonScriptWidget  = {};
    Widgets::ScriptManager*           ScriptManagerWidget = {};
    Payload*                          PayloadDialog       = {};
    class LootWidget*                 LootWidget          = {};
    QStackedWidget*                   MainViewWidget      = {};
    QWidget*                          SessionTablePage    = {};
    MugenSpace::DBManager*            dbManager           = {};
    QString                           TeamserverName      = {};
    QWidget*                          PageWidget          = {};
    SmallAppWidgets_t*                SmallAppWidgets     = {};

    void setupUi( QWidget* Page, QString TeamserverName );
    void NewBottomTab( QWidget* TabWidget, const std::string& TitleName, QString IconPath = "" ) const;
    void NewWidgetTab( QWidget* TabWidget, const std::string& TitleName ) const;

    void OpenProcessList( const QString& sessionID );
    void OpenFileBrowser( const QString& sessionID );
    void OpenPayloadBuilder();

protected slots:
    void handleDemonContextMenu( const QPoint& pos );
    void removeTabSmall( int ) const;
};

#endif
