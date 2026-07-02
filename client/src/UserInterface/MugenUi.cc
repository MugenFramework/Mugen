#include <global.hpp>

// Headers for UserInterface
#include <Mugen/Mugen.hpp>
#include <Mugen/PythonApi/PythonApi.h>

#include <UserInterface/MugenUI.hpp>
#include <Mugen/Connector.hpp>
#include <UserInterface/Widgets/DemonInteracted.h>
#include <UserInterface/Widgets/TeamserverTabSession.h>
#include <UserInterface/SmallWidgets/EventViewer.hpp>
#include <UserInterface/Widgets/PythonScript.hpp>
#include <UserInterface/Widgets/ScriptManager.h>
#include <UserInterface/Widgets/LootWidget.h>
#include <UserInterface/Widgets/DashboardWidget.hpp>

#include <Util/ColorText.h>

#include <Mugen/Packager.hpp>

#include <QPixmap>
#include <QProcess>
#include <QToolButton>
#include <QShortcut>
#include <QTimer>
#include <QLabel>
#include <QDir>
#include <QFileInfo>

using namespace MugenNamespace::MugenSpace;

void MugenNamespace::UserInterface::MugenUi::setupUi(QMainWindow *Window)
{
    MugenWindow = Window;

    if ( MugenWindow->objectName().isEmpty() ) {
        MugenWindow->setObjectName( QString::fromUtf8( "MugenWindow " ) );
    }

    MugenWindow->resize( 1399, 821 );
    MugenWindow->setStyleSheet( FileRead( ":/stylesheets/Mugen" ) );

    actionNew_Client = new QAction( MugenWindow );
    actionNew_Client->setObjectName( QString::fromUtf8( "NewClient" ) );

    actionChat = new QAction( MugenWindow );
    actionChat->setObjectName( QString::fromUtf8( "actionChat" ) );

    actionPreferences = new QAction( MugenWindow );
    actionPreferences->setObjectName( QString::fromUtf8( "actionPreferences" ) );
    actionPreferences->setIcon( QIcon( ":/icons/settings" ) );
    actionPreferences->setShortcut( QKeySequence( "Ctrl+Alt+s" ) );

    actionDisconnect = new QAction( MugenWindow );
    actionDisconnect->setObjectName( QString::fromUtf8( "actionDisconnect" ) );

    actionExit = new QAction( MugenWindow );
    actionExit->setObjectName( QString::fromUtf8( "actionExit" ) );

    actionTeamserver = new QAction( MugenWindow );
    actionTeamserver->setObjectName( QString::fromUtf8( "actionTeamserver" ) );

    actionStore = new QAction( MugenWindow );
    actionStore->setObjectName( QString::fromUtf8( "actionStore" ) );

    actionGeneratePayload = new QAction( MugenWindow );
    actionGeneratePayload->setObjectName( QString::fromUtf8( "actionGeneratePayload" ) );

    actionLoad_Script = new QAction( MugenWindow );
    actionLoad_Script->setObjectName( QString::fromUtf8( "actionLoad_Script" ) );

    actionPythonConsole = new QAction( MugenWindow );
    actionPythonConsole->setObjectName( QString::fromUtf8( "actionPythonConsole" ) );

    actionAbout = new QAction( MugenWindow );
    actionAbout->setObjectName( QString::fromUtf8( "actionAbout" ) );

    actionOpen_Help_Documentation = new QAction( MugenWindow );
    actionOpen_Help_Documentation->setObjectName( QString::fromUtf8( "actionOpen_Help_Documentation" ) );

    actionOpen_API_Reference = new QAction( MugenWindow );
    actionOpen_API_Reference->setObjectName( QString::fromUtf8( "actionOpen_API_Reference" ) );

    actionGithub_Repository = new QAction( MugenWindow );
    actionGithub_Repository->setObjectName( QString::fromUtf8( "actionGithub_Repository" ) );

    actionListeners = new QAction( MugenWindow );
    actionListeners->setObjectName( QString::fromUtf8( "actionListeners" ) );

    actionSessionsTable = new QAction( MugenWindow );
    actionSessionsTable->setObjectName( QString::fromUtf8( "actionSessionsTable" ) );

    actionSessionsGraph = new QAction( MugenWindow );
    actionSessionsGraph->setObjectName( QString::fromUtf8( "actionSessionsGraph" ) );

    actionSessionsMap = new QAction( MugenWindow );
    actionSessionsMap->setObjectName( QString::fromUtf8( "actionSessionsMap" ) );

    actionDashboard = new QAction( MugenWindow );
    actionDashboard->setObjectName( QString::fromUtf8( "actionDashboard" ) );

    actionLogs = new QAction( MugenWindow );
    actionLogs->setObjectName( QString::fromUtf8( "actionLogs" ) );

    actionLoot = new QAction( MugenWindow );
    actionLoot->setObjectName( QString::fromUtf8( "actionLoot" ) );

    centralwidget = new QWidget( MugenWindow );
    centralwidget->setObjectName( QString::fromUtf8( "centralwidget" ) );
    gridLayout_3 = new QGridLayout( centralwidget );
    gridLayout_3->setObjectName( QString::fromUtf8( "gridLayout_3" ) );
    gridLayout_3->setContentsMargins( 0, 0, 0, 0 );

    TeamserverTabWidget = new QTabWidget( centralwidget );
    TeamserverTabWidget->setObjectName( QString::fromUtf8( "TeamserverTabWidget" ) );
    TeamserverTabWidget->setStyleSheet( FileRead( ":/stylesheets/teamserverTab" ) );
    TeamserverTabWidget->setTabBarAutoHide( true );
    TeamserverTabWidget->setTabsClosable( true );

    /* TODO: refactor this. */
    MugenX::Teamserver.TabSession = new UserInterface::Widgets::TeamserverTabSession;
    MugenX::Teamserver.TabSession->setupUi( new QWidget, MugenX::Teamserver.Name );
    TeamserverTabWidget->setCurrentIndex(
        TeamserverTabWidget->addTab(
            MugenX::Teamserver.TabSession->PageWidget,
            MugenX::Teamserver.Name
        )
    );

    gridLayout_3->addWidget( TeamserverTabWidget, 0, 0, 1, 1 );

    menubar = new QMenuBar( this->MugenWindow );
    menubar->setObjectName( QString::fromUtf8( "menubar" ) );
    menubar->setGeometry( QRect( 0, 0, 1143, 20 ) );

    menubar->setStyleSheet( FileRead( ":/stylesheets/menubar" ) );

    menuMugen   = new QMenu( menubar );
    menuView    = new QMenu( menubar );
    menuAttack  = new QMenu( menubar );
    MenuSession = new QMenu( menubar );
    menuTheme   = new QMenu( menubar );
    menuScripts = new QMenu( menubar );
    menuHelp    = new QMenu( menubar );

    menuMugen->setObjectName( QString::fromUtf8( "menuMugen" ) );
    menuView->setObjectName( QString::fromUtf8( "menuView" ) );
    menuAttack->setObjectName( QString::fromUtf8( "menuAttack" ) );
    menuTheme->setObjectName( QString::fromUtf8( "menuTheme" ) );

    actionThemeMugen = new QAction( MugenWindow );
    actionThemeMugen->setObjectName( QString::fromUtf8( "actionThemeMugen" ) );
    actionThemeMugen->setCheckable( true );

    actionThemeHavoc = new QAction( MugenWindow );
    actionThemeHavoc->setObjectName( QString::fromUtf8( "actionThemeHavoc" ) );
    actionThemeHavoc->setCheckable( true );

    MugenWindow->setMenuBar( menubar );

    menuScripts->setObjectName( QString::fromUtf8( "menuScripts" ) );
    menuHelp->setObjectName( QString::fromUtf8( "menuHelp" ) );

    statusbar = new QStatusBar( MugenWindow );
    statusbar->setObjectName( QString::fromUtf8( "statusbar" ) );
    statusbar->setLayoutDirection( Qt::LayoutDirection::LeftToRight );
    statusbar->setSizeGripEnabled( false );
    statusbar->setVisible( true );
    MugenWindow->setStatusBar( statusbar );

    {
        auto lblConn = new QLabel(
            "<span style='color:#50fa7b'>&#9679;</span>"
            " <span style='color:#8888aa'>connected to</span>"
            " <span style='color:#f0f0ee'>" + MugenX::Teamserver.Name + "</span>"
        );
        lblConn->setContentsMargins( 8, 0, 0, 0 );
        statusbar->addPermanentWidget( lblConn, 1 );

        auto lblSep = new QLabel( "<span style='color:#2a2a3f'>|</span>" );
        statusbar->addPermanentWidget( lblSep );

        auto lblOp = new QLabel(
            "<span style='color:#8888aa'>operator</span>"
            " <span style='color:#ff6b9d'>" + MugenX::Teamserver.User + "</span>"
        );
        lblOp->setContentsMargins( 0, 0, 8, 0 );
        statusbar->addPermanentWidget( lblOp );
    }

    menubar->addAction( menuMugen->menuAction() );
    menubar->addAction( menuView->menuAction() );
    menubar->addAction( menuAttack->menuAction() );
    menubar->addAction( menuScripts->menuAction() );
    menubar->addAction( menuTheme->menuAction() );
    menubar->addAction( menuHelp->menuAction() );

    menuMugen->addAction( actionNew_Client );
    menuMugen->addSeparator();
    menuMugen->addAction( actionDisconnect );
    menuMugen->addAction( actionExit );

    MenuSession->addAction( actionSessionsTable );
    MenuSession->addAction( actionSessionsGraph );
    MenuSession->addAction( actionSessionsMap );

    menuView->addAction( actionListeners );
    menuView->addSeparator();
    menuView->addAction( MenuSession->menuAction() );
    menuView->addAction( actionDashboard );
    menuView->addSeparator();
    menuView->addAction( actionChat );
    menuView->addAction( actionLoot );
    menuView->addSeparator();
    menuView->addAction( actionLogs );
    menuView->addAction( actionTeamserver );

    menuAttack->addAction( actionGeneratePayload );
    menuAttack->addAction( actionStore );

    menuScripts->addAction( actionLoad_Script );
    menuScripts->addAction( actionPythonConsole );

    menuTheme->addAction( actionThemeMugen );
    menuTheme->addAction( actionThemeHavoc );

    menuHelp->addAction( actionAbout );
    menuHelp->addSeparator();
    menuHelp->addAction( actionOpen_Help_Documentation );
    menuHelp->addAction( actionOpen_API_Reference );
    menuHelp->addSeparator();
    menuHelp->addAction( actionGithub_Repository );

    /* prepare python interpreter */
    PythonPrepare();

    /* connect events & buttons */
    ConnectEvents();

    /* set text for each action, button and widget */
    retranslateUi( MugenWindow );

    // apply saved theme
    {
        QSettings settings( "Mugen", "Mugen" );
        auto saved = settings.value( "theme", "mugen" ).toString();
        applyTheme( saved );
    }

    // System tray icon for agent check-in notifications
    if ( QSystemTrayIcon::isSystemTrayAvailable() )
    {
        TrayIcon = new QSystemTrayIcon( MugenWindow );
        TrayIcon->setIcon( MugenWindow->windowIcon() );
        TrayIcon->setToolTip( "Mugen C2" );
        TrayIcon->show();
    }

    MugenWindow->setFocus();
    MugenWindow->showMaximized();

    QMetaObject::connectSlotsByName( MugenWindow );
}

void MugenNamespace::UserInterface::MugenUi::OneSecondTick()
{
    if ( MugenX::Teamserver.TabSession && MugenX::Teamserver.TabSession->Dashboard )
        MugenX::Teamserver.TabSession->Dashboard->Refresh();
}

void MugenNamespace::UserInterface::MugenUi::MarkSessionAs(MugenNamespace::Util::SessionItem Session, QString Mark)
{
    for ( int i = 0; i <  MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->rowCount(); i++ )
    {
        auto AgentID = MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->item( i, 0 )->text();

        if ( Session.Name.compare( AgentID ) == 0 )
        {
            auto Package = new Util::Packager::Package;
            QString Marked;

            if ( Mark.compare( "Alive" ) == 0 )
            {
                Marked = "Alive";
                Session.Marked = Marked;

                auto Icon = ( Session.Elevated.compare( "true" ) == 0 ) ?
                            WinVersionIcon( Session.OS, true ) :
                            WinVersionIcon( Session.OS, false );

                MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->item( i, 0 )->setIcon( Icon );

                for ( int j = 0; j < MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->columnCount(); j++ )
                {
                    MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->item( i, j )->setBackground( QColor( Util::ColorText::Colors::Hex::Background ) );
                    MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->item( i, j )->setForeground( QColor( Util::ColorText::Colors::Hex::Foreground ) );
                }
            }
            else if ( Mark.compare( "Dead" ) == 0 )
            {
                Marked = "Dead";
                Session.Marked = Marked;

                MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->item( i, 0 )->setIcon( QIcon( ":/icons/DeadWhite" ) );

                for ( int j = 0; j < MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->columnCount(); j++ )
                {
                    MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->item( i, j )->setBackground( QColor( Util::ColorText::Colors::Hex::CurrentLine ) );
                    MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->item( i, j )->setForeground( QColor( Util::ColorText::Colors::Hex::Comment ) );
                }
            }

            Package->Body = Util::Packager::Body_t {
                .SubEvent = Util::Packager::Session::MarkAs,
                .Info = {
                        { "AgentID", AgentID.toStdString() },
                        { "Marked",  Marked.toStdString() },
                }
            };

            MugenX::Connector->SendPackage( Package );

            break;
        }
    }
}


void MugenNamespace::UserInterface::MugenUi::UpdateSessionsHealth()
{
    for ( auto& session : MugenX::Teamserver.Sessions )
    {
        if ( session.Marked.compare( "Dead" ) == 0 )
            continue;

        auto Now  = QDateTime::currentDateTimeUtc();
        auto diff = session.LastUTC.secsTo( Now );

        auto seconds = QDateTime::fromTime_t( diff ).toUTC().toString("s");
        auto minutes = QDateTime::fromTime_t( diff ).toUTC().toString("m");
        auto hours   = QDateTime::fromTime_t( diff ).toUTC().toString("h");
        auto days    = QDateTime::fromTime_t( diff ).toUTC().toString("d");

        if ( diff < 60 )
        {
            session.Last = QString("%1s").arg(seconds);
            MugenX::Teamserver.TabSession->SessionTableWidget->ChangeSessionValue(session.Name, 8, session.Last.toStdString().c_str());
        }
        else if ( diff < 60 * 60 )
        {
            session.Last = QString("%1m %2s").arg(minutes, seconds);
            MugenX::Teamserver.TabSession->SessionTableWidget->ChangeSessionValue(session.Name, 8, session.Last.toStdString().c_str());
        }
        else if ( diff < 24 * 60 * 60 )
        {
            session.Last = QString("%1h %2m").arg(hours, minutes);
            MugenX::Teamserver.TabSession->SessionTableWidget->ChangeSessionValue(session.Name, 8, session.Last.toStdString().c_str());
        }
        else
        {
            session.Last = QString("%1d %2h").arg(days, hours);
            MugenX::Teamserver.TabSession->SessionTableWidget->ChangeSessionValue(session.Name, 8, session.Last.toStdString().c_str());
        }

        // it is very normal for agents to delay three second due to network latency
        auto AllowedDiff = 3;

        if ( session.KillDate > 0 )
        {
            auto UNIX_TIME_START  = 0x019DB1DED53E8000; //January 1, 1970 (start of Unix epoch) in "ticks"
            auto TICKS_PER_SECOND = 10000000; //a tick is 100ns
            auto KillDateInEpoch  = ( session.KillDate - UNIX_TIME_START ) / TICKS_PER_SECOND;

            if ( Now.secsTo( QDateTime::fromSecsSinceEpoch( KillDateInEpoch, Qt::UTC ) ) <= 0 )
            {
                // agent reached its killdate
                session.Health = "killdate";
                session.Marked = "Dead";
                MugenX::Teamserver.TabSession->SessionTableWidget->SetHealthCell( session.Name, "● killdate", QColor( 0x66, 0x66, 0x77 ) );
                MarkSessionAs( session, QString( "Dead") );
                continue;
            }
        }

        if ( ( ( session.WorkingHours >> 22 ) & 1 ) == 1 )
        {
            uint32_t StartHour   = ( session.WorkingHours >> 17 ) & 0b011111;
            uint32_t StartMinute = ( session.WorkingHours >> 11 ) & 0b111111;
            uint32_t EndHour     = ( session.WorkingHours >>  6 ) & 0b011111;
            uint32_t EndMinute   = ( session.WorkingHours >>  0 ) & 0b111111;
            bool isOffHours = false;

            if ( StartHour < Now.time().hour() || EndHour > Now.time().hour() ) {
                isOffHours = true;
            }

            if ( StartHour == Now.time().hour() && StartMinute < Now.time().minute() ) {
                isOffHours = true;
            }

            if ( EndHour == Now.time().hour() && EndMinute > Now.time().minute() ) {
                isOffHours = true;
            }

            if ( isOffHours ) {
                // agent is offhours
                session.Health = "offhours";
                MugenX::Teamserver.TabSession->SessionTableWidget->SetHealthCell( session.Name, "● off-hours", QColor( 0xf1, 0xfa, 0x8c ) );
                continue;
            }
        }

        auto expected = session.SleepDelay + ( session.SleepDelay * 0.01 * session.SleepJitter );
        if ( diff - AllowedDiff < expected ) {
            session.Health = "healthy";
            auto remaining = (qint64)expected - diff;
            QString countdown;
            if ( remaining >= 60 )
                countdown = QString("● next %1m %2s").arg( remaining / 60 ).arg( remaining % 60 );
            else
                countdown = QString("● next %1s").arg( remaining );
            MugenX::Teamserver.TabSession->SessionTableWidget->SetHealthCell(
                session.Name, countdown, QColor( 0x50, 0xfa, 0x7b ) );
            continue;
        } else {
            session.Health = "unresponsive";
            auto overdue = diff - (qint64)expected;
            QString lateText;
            if ( overdue >= 60 )
                lateText = QString("● late +%1m %2s").arg( overdue / 60 ).arg( overdue % 60 );
            else
                lateText = QString("● late +%1s").arg( overdue );
            MugenX::Teamserver.TabSession->SessionTableWidget->SetHealthCell(
                session.Name, lateText, QColor( 0xff, 0x55, 0x55 ) );
            continue;
        }
    }
}

void MugenNamespace::UserInterface::MugenUi::retranslateUi(QMainWindow* Window ) const
{
    Window->setWindowTitle( "Mugen" );

    actionNew_Client->setText( "New Client" );
    actionChat->setText( "Teamserver Chat" );
    actionDisconnect->setText( "Disconnect" );
    actionExit->setText( "Exit" );
    actionTeamserver->setText( "Teamserver" );
    actionStore->setText( "Extentions" );
    actionGeneratePayload->setText( "Payload" );
    actionLoad_Script->setText(  "Scripts Manager" );
    actionPythonConsole->setText( "Script Console" );
    actionAbout->setText( "About" );
    actionOpen_Help_Documentation->setText( "Open Documentation" );
    actionOpen_API_Reference->setText( "Open API Reference" );
    actionGithub_Repository->setText( "Github Repository" );
    actionListeners->setText( "Listeners" );
    actionSessionsTable->setText( "Table" );
    actionSessionsGraph->setText( "Graph" );
    actionSessionsMap->setText( "Map" );
    actionDashboard->setText( "Dashboard" );
    actionLogs->setText( "Event Viewer" );
    actionLoot->setText( "Loot" );
    menuMugen->setTitle( "Mugen" );
    menuView->setTitle( "View" );
    menuAttack->setTitle( "Attack" );
    menuScripts->setTitle( "Scripts" );
    menuTheme->setTitle( "Theme" );
    menuHelp->setTitle( "Help" );
    MenuSession->setTitle( "Session View" );
    actionThemeMugen->setText( "Mugen (Default)" );
    actionThemeHavoc->setText( "Havoc (Classic)" );
}

void MugenNamespace::UserInterface::MugenUi::ConnectEvents()
{
    auto OneSecondTimer = new QTimer( this );

    QMainWindow::connect( OneSecondTimer, &QTimer::timeout, this, [&]() {
        UpdateSessionsHealth();
        OneSecondTick();
    } );
    OneSecondTimer->start(1000);

    QMainWindow::connect( actionNew_Client, &QAction::triggered, this, []() {
        QProcess::startDetached( QCoreApplication::applicationFilePath(), QStringList{""} );
    } );

    QMainWindow::connect( actionChat, &QAction::triggered, this, [&](){
        auto Teamserver = MugenX::Teamserver.TabSession;
        if ( Teamserver->TeamserverChat == nullptr ) {
            Teamserver->TeamserverChat = new Widgets::Chat;
            Teamserver->TeamserverChat->setupUi(new QWidget);
            Teamserver->TeamserverName = MugenX::Teamserver.Name;
        }

        NewBottomTab(
            Teamserver->TeamserverChat->ChatWidget,
            "Teamserver Chat"
        );
    } );

    QMainWindow::connect( actionDisconnect, &QAction::triggered, this, []() {
        if ( MugenX::Connector != nullptr ) {
            MugenX::Connector->Disconnect();
            MessageBox( "Disconnected", "Disconnected from " + MugenX::Teamserver.Name, QMessageBox::Information );
        } else {
            MessageBox( "Error", "Couldn't disconnect from " + MugenX::Teamserver.Name, QMessageBox::Critical );
        }
    } );

    QMainWindow::connect( actionExit, &QAction::triggered, this, []() {
        Mugen::Exit();
    } );

    QMainWindow::connect( actionThemeMugen, &QAction::triggered, this, &MugenUi::onThemeMugen );
    QMainWindow::connect( actionThemeHavoc, &QAction::triggered, this, &MugenUi::onThemeHavoc );

    QMainWindow::connect( actionSessionsTable, &QAction::triggered, this, []() {
        MugenX::Teamserver.TabSession->MainViewWidget->setCurrentIndex( 0 );
    } );

    QMainWindow::connect( actionListeners, &QAction::triggered, this, [&](){
        auto Teamserver = MugenX::Teamserver.TabSession;

        if ( Teamserver->ListenerTableWidget == nullptr ) {
            Teamserver->ListenerTableWidget = new Widgets::ListenersTable;
            Teamserver->ListenerTableWidget->setupUi(new QWidget);
            Teamserver->ListenerTableWidget->setDBManager(this->dbManager);
            Teamserver->ListenerTableWidget->TeamserverName = MugenX::Teamserver.Name;
        }

        NewBottomTab(
            Teamserver->ListenerTableWidget->ListenerWidget,
            "Listeners"
        );
    } );

    QMainWindow::connect( actionTeamserver, &QAction::triggered, this, [&](){
        if ( MugenX::Teamserver.TabSession->Teamserver == nullptr ) {
            MugenX::Teamserver.TabSession->Teamserver = new Teamserver;
            MugenX::Teamserver.TabSession->Teamserver->setupUi( new QDialog );
        }

        NewBottomTab(
            MugenX::Teamserver.TabSession->Teamserver->TeamserverWidget,
            "Teamserver"
        );
    } );

    QMainWindow::connect( actionStore, &QAction::triggered, this, [&](){
        if ( MugenX::Teamserver.TabSession->Store == nullptr ) {
            MugenX::Teamserver.TabSession->Store = new Store;
            MugenX::Teamserver.TabSession->Store->setupUi( new QDialog );
        }

        NewBottomTab(
            MugenX::Teamserver.TabSession->Store->StoreWidget,
            "Extentions"
        );
    } );

    QMainWindow::connect( actionSessionsGraph, &QAction::triggered, this, [&]() {
        MugenX::Teamserver.TabSession->MainViewWidget->setCurrentIndex( 1 );
    } );

    QMainWindow::connect( actionSessionsMap, &QAction::triggered, this, [&]() {
        auto tab = MugenX::Teamserver.TabSession;
        tab->SessionMapWidget->UpdateMap( MugenX::Teamserver.Sessions );
        tab->MainViewWidget->setCurrentIndex( 2 );
    } );

    QMainWindow::connect( actionDashboard, &QAction::triggered, this, [&]() {
        auto tab = MugenX::Teamserver.TabSession;
        if ( !tab || !tab->Dashboard ) return;
        int idx = tab->tabWidget->indexOf( tab->Dashboard );
        if ( idx >= 0 ) {
            tab->tabWidget->setCurrentIndex( idx );
        } else {
            tab->NewWidgetTab( tab->Dashboard, "Dashboard" );
        }
        tab->Dashboard->Refresh();
    } );

    QMainWindow::connect( actionLogs, &QAction::triggered, this, [&]() {
        auto Teamserver = MugenX::Teamserver.TabSession;

        if ( Teamserver->SmallAppWidgets->EventViewer == nullptr )
        {
            Teamserver->SmallAppWidgets->EventViewer = new SmallWidgets::EventViewer;
            Teamserver->SmallAppWidgets->EventViewer->setupUi( new QWidget );
        }

        NewSmallTab(
            Teamserver->SmallAppWidgets->EventViewer->EventViewer,
            "Event Viewer"
        );
    } );

    QMainWindow::connect( actionLoot, &QAction::triggered, this, [&]() {
        NewBottomTab( MugenX::Teamserver.TabSession->LootWidget, "Loot Collection" );
    } );

    QMainWindow::connect( actionGeneratePayload, &QAction::triggered, this, [this]() {
        if ( MugenX::Teamserver.TabSession->PayloadDialog == nullptr ) {
            MugenX::Teamserver.TabSession->PayloadDialog = new Payload;
            MugenX::Teamserver.TabSession->PayloadDialog->setupUi( new QDialog( this->MugenWindow ) );
            MugenX::Teamserver.TabSession->PayloadDialog->TeamserverName = MugenX::Teamserver.Name;
        }

        MugenX::Teamserver.TabSession->PayloadDialog->Start();
    } );

    QMainWindow::connect( actionPythonConsole, &QAction::triggered, this, [&]() {
        auto Teamserver = MugenX::Teamserver.TabSession;

        if ( Teamserver->PythonScriptWidget == nullptr ) {
            Teamserver->PythonScriptWidget = new Widgets::PythonScriptInterpreter;
            Teamserver->PythonScriptWidget->setupUi( new QWidget );
        }

        NewBottomTab(
            Teamserver->PythonScriptWidget->PythonScriptInterpreterWidget,
            "Scripting Console"
        );
    } );

    QMainWindow::connect( actionLoad_Script, &QAction::triggered, this, [&]() {
        auto Teamserver = MugenX::Teamserver.TabSession;

        if ( Teamserver->PythonScriptWidget == nullptr ) {
            Teamserver->PythonScriptWidget = new Widgets::PythonScriptInterpreter;
            Teamserver->PythonScriptWidget->setupUi( new QWidget );
        }

        if ( Teamserver->ScriptManagerWidget == nullptr ) {
            Teamserver->ScriptManagerWidget = new Widgets::ScriptManager;
            Teamserver->ScriptManagerWidget->SetupUi( new QWidget );
        }

        NewBottomTab(
            Teamserver->ScriptManagerWidget->ScriptManagerWidget,
            "Script Manager"
        );
    } );

    QMainWindow::connect( actionAbout, &QAction::triggered, this, [&]() {
        if ( AboutDialog == nullptr ) {
            AboutDialog = new About( new QDialog(MugenX::MugenUserInterface->MugenWindow) );
            AboutDialog->setupUi();
        }

        this->AboutDialog->AboutDialog->exec();
    } );

    QMainWindow::connect( actionGithub_Repository, &QAction::triggered, this, []() {
        QDesktopServices::openUrl( QUrl( "https://github.com/MugenFramework/Mugen" ) );
    } );

    QMainWindow::connect( actionOpen_Help_Documentation, &QAction::triggered, this, []() {
        QDesktopServices::openUrl( QUrl( "https://github.com/MugenFramework/Mugen/wiki" ) );
    } );
}

void MugenNamespace::UserInterface::MugenUi::NewBottomTab(QWidget* TabWidget, const std::string& TitleName, const QString IconPath ) const
{
    MugenX::Teamserver.TabSession->NewBottomTab( TabWidget, TitleName );
}

void MugenNamespace::UserInterface::MugenUi::setDBManager(MugenSpace::DBManager* dbManager)
{
    this->dbManager = dbManager;
    if ( MugenX::Teamserver.TabSession ) {
        MugenX::Teamserver.TabSession->dbManager = dbManager;
        if ( MugenX::Teamserver.TabSession->LootWidget )
            MugenX::Teamserver.TabSession->LootWidget->LoadCredentialsFromDB( dbManager );
    }
}

void UserInterface::MugenUi::NewTeamserverTab(MugenNamespace::Util::ConnectionInfo* Connection )
{
    Connection->TabSession = new UserInterface::Widgets::TeamserverTabSession;
    Connection->TabSession->setupUi( new QWidget, Connection->Name );
    Connection->TabSession->dbManager = this->dbManager;
    if ( Connection->TabSession->LootWidget )
        Connection->TabSession->LootWidget->LoadCredentialsFromDB( this->dbManager );

    int id = TeamserverTabWidget->addTab( Connection->TabSession->PageWidget, Connection->Name );
    TeamserverTabWidget->setCurrentIndex( id );
    MugenX::Teamserver = *Connection;
}

void UserInterface::MugenUi::NewTeamserverTab(QString Name )
{
    MugenX::Teamserver.TabSession = new UserInterface::Widgets::TeamserverTabSession;
    MugenX::Teamserver.TabSession->setupUi( new QWidget, MugenX::Teamserver.Name );
    MugenX::Teamserver.TabSession->dbManager = this->dbManager;
    if ( MugenX::Teamserver.TabSession->LootWidget )
        MugenX::Teamserver.TabSession->LootWidget->LoadCredentialsFromDB( this->dbManager );

    int id = TeamserverTabWidget->addTab( MugenX::Teamserver.TabSession->PageWidget, MugenX::Teamserver.Name );
    TeamserverTabWidget->setCurrentIndex( id );
}

void UserInterface::MugenUi::NewSmallTab(QWidget *TabWidget, const string &TitleName ) const
{
    auto Teamserver = MugenX::Teamserver.TabSession;
    Teamserver->NewWidgetTab( TabWidget, TitleName );
}

void UserInterface::MugenUi::PythonPrepare()
{
    PyImport_AppendInittab( "emb", emb::PyInit_emb );
    PyImport_AppendInittab( "mugenui", PythonAPI::MugenUI::PyInit_MugenUI );
    PyImport_AppendInittab( "mugen", PythonAPI::Mugen::PyInit_Mugen );

    Py_Initialize();

    PyImport_ImportModule( "emb" );

    // Register "havoc" as an alias for "mugen" in sys.modules so that
    // HavocFramework/Modules scripts that do `from havoc import ...` work
    // from any directory without needing a local havoc.py shim.
    PyRun_SimpleString(
        "import sys, mugen\n"
        "sys.modules['havoc'] = mugen\n"
    );

    // Inject the Packer class into the global namespace. Most HavocFramework
    // modules use Packer() without importing it, expecting it to be globally
    // available after loading Packer/packer.py first. We provide it for free.
    PyRun_SimpleString(
        "from struct import pack, calcsize\n"
        "class Packer:\n"
        "    def __init__(self):\n"
        "        self.buffer = b''\n"
        "        self.size = 0\n"
        "    def getbuffer(self):\n"
        "        return pack('<L', self.size) + self.buffer\n"
        "    def addstr(self, s):\n"
        "        if s is None: s = ''\n"
        "        if isinstance(s, str): s = s.encode('utf-8')\n"
        "        fmt = '<L{}s'.format(len(s) + 1)\n"
        "        self.buffer += pack(fmt, len(s) + 1, s)\n"
        "        self.size += calcsize(fmt)\n"
        "    def addWstr(self, s):\n"
        "        if s is None: s = ''\n"
        "        s = s.encode('utf-16_le')\n"
        "        fmt = '<L{}s'.format(len(s) + 2)\n"
        "        self.buffer += pack(fmt, len(s) + 2, s)\n"
        "        self.size += calcsize(fmt)\n"
        "    def addbytes(self, b):\n"
        "        if b is None: b = b''\n"
        "        fmt = '<L{}s'.format(len(b))\n"
        "        self.buffer += pack(fmt, len(b), b)\n"
        "        self.size += calcsize(fmt)\n"
        "    def addbool(self, b):\n"
        "        self.buffer += pack('<I', 1 if b else 0)\n"
        "        self.size += 4\n"
        "    def adduint32(self, n):\n"
        "        self.buffer += pack('<I', n)\n"
        "        self.size += 4\n"
        "    def addint(self, n):\n"
        "        self.buffer += pack('<i', n)\n"
        "        self.size += 4\n"
        "    def addshort(self, n):\n"
        "        self.buffer += pack('<h', n)\n"
        "        self.size += 2\n"
        "import builtins\n"
        "builtins.Packer = Packer\n"
    );

    for ( auto& ScriptPath : dbManager->GetScripts() ) {
        Widgets::ScriptManager::AddScript( ScriptPath );
    }

    // Auto-load all .py from ~/.mugen/modules/
    auto modulesDir = QDir( QDir::homePath() + "/.mugen/modules" );
    if ( modulesDir.exists() ) {
        for ( auto& entry : modulesDir.entryInfoList( { "*.py" }, QDir::Files, QDir::Name ) ) {
            Widgets::ScriptManager::AddScript( entry.absoluteFilePath() );
        }
    }
}

void MugenNamespace::UserInterface::MugenUi::applyTheme( const QString& theme )
{
    if ( theme == "havoc" )
    {
        MugenWindow->setStyleSheet( FileRead( ":/stylesheets/Havoc" ) );
        menubar->setStyleSheet( FileRead( ":/stylesheets/menubar-havoc" ) );
        TeamserverTabWidget->setStyleSheet( FileRead( ":/stylesheets/teamserverTab-havoc" ) );
        actionThemeMugen->setChecked( false );
        actionThemeHavoc->setChecked( true );
    }
    else
    {
        MugenWindow->setStyleSheet( FileRead( ":/stylesheets/Mugen" ) );
        menubar->setStyleSheet( FileRead( ":/stylesheets/menubar" ) );
        TeamserverTabWidget->setStyleSheet( FileRead( ":/stylesheets/teamserverTab" ) );
        actionThemeMugen->setChecked( true );
        actionThemeHavoc->setChecked( false );
    }

    QSettings settings( "Mugen", "Mugen" );
    settings.setValue( "theme", theme );
}

void MugenNamespace::UserInterface::MugenUi::onThemeMugen()
{
    applyTheme( "mugen" );
}

void MugenNamespace::UserInterface::MugenUi::onThemeHavoc()
{
    applyTheme( "havoc" );
}

