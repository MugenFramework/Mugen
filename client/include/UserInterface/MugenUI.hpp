#ifndef MUGEN_UI_HPP
#define MUGEN_UI_HPP

#include <global.hpp>

#include <UserInterface/Dialogs/About.hpp>
#include <UserInterface/Dialogs/Connect.hpp>
#include <UserInterface/Dialogs/Listener.hpp>
#include <UserInterface/Dialogs/Payload.hpp>

#include <UserInterface/Widgets/SessionTable.hpp>
#include <UserInterface/Widgets/Chat.hpp>
#include <UserInterface/Widgets/ListenerTable.hpp>

#include <Mugen/DBManager/DBManager.hpp>

// QT libraries
#include <QDesktopServices>
#include <QShortcut>
#include <QStatusBar>
#include <QSystemTrayIcon>
#include <QDockWidget>
#include <QHeaderView>
#include <QSplitter>
#include <QTableWidget>
#include <QFile>
#include <QStackedWidget>
#include <QSettings>

class MugenNamespace::UserInterface::MugenUi : public QMainWindow
{
public:
    QWidget*               centralwidget                 = {};
    QAction*               actionNew_Client              = {};
    QAction*               actionChat                    = {};
    QAction*               actionPreferences             = {};
    QAction*               actionDisconnect              = {};
    QAction*               actionExit                    = {};
    QAction*               actionTeamserver              = {};
    QAction*               actionStore                   = {};
    QAction*               actionGeneratePayload         = {};
    QAction*               actionLoad_Script             = {};
    QAction*               actionPythonConsole           = {};
    QAction*               actionAbout                   = {};
    QAction*               actionOpen_Help_Documentation = {};
    QAction*               actionOpen_API_Reference      = {};
    QAction*               actionGithub_Repository       = {};
    QAction*               actionListeners               = {};
    QAction*               actionSessionsTable           = {};
    QAction*               actionSessionsGraph           = {};
    QAction*               actionSessionsMap             = {};
    QAction*               actionDashboard               = {};
    QAction*               actionLogs                    = {};
    QSystemTrayIcon*       TrayIcon                      = {};
    QAction*               actionLoot                    = {};
    QGridLayout*           gridLayout                    = {};
    QGridLayout*           gridLayout_3                  = {};
    QTabWidget*            TeamserverTabWidget           = {};
    QMenuBar*              menubar                       = {};
    QMenu*                 menuMugen                     = {};
    QMenu*                 menuView                      = {};
    QMenu*                 menuAttack                    = {};
    QMenu*                 menuScripts                   = {};
    QMenu*                 menuHelp                      = {};
    QMenu*                 MenuSession                   = {};
    QMenu*                 menuTheme                     = {};
    QAction*               actionThemeMugen              = {};
    QAction*               actionThemeHavoc              = {};
    QStatusBar*            statusbar                     = {};
    Dialogs::Connect*      ConnectDialog                 = {};
    About*                 AboutDialog                   = {};
    QMainWindow*           MugenWindow                   = {};
    MugenSpace::DBManager* dbManager                     = {};

public:
    void MarkSessionAs( MugenNamespace::Util::SessionItem session, QString Mark );
    void UpdateSessionsHealth();
    void setupUi( QMainWindow *Window );
    void retranslateUi( QMainWindow *Window ) const;
    void setDBManager( MugenSpace::DBManager* dbManager );
    void NewTeamserverTab( MugenNamespace::Util::ConnectionInfo* );
    void NewTeamserverTab( QString Name );
    void NewBottomTab( QWidget* TabWidget, const std::string& TitleName, const QString IconPath = "" ) const;
    void NewSmallTab( QWidget* TabWidget, const std::string& TitleName ) const;
    void ConnectEvents();
    void PythonPrepare();
    void applyTheme( const QString& theme );

public slots:
    void OneSecondTick();
    void onThemeMugen();
    void onThemeHavoc();
};

#endif
