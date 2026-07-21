#include <global.hpp>

#include <UserInterface/Widgets/TeamserverTabSession.h>
#include <UserInterface/Widgets/SessionTable.hpp>
#include <UserInterface/Widgets/SessionGraph.hpp>
#include <UserInterface/Widgets/SessionMap.hpp>
#include <UserInterface/Widgets/DashboardWidget.hpp>
#include <UserInterface/Widgets/TeamserverTabSession.h>
#include <UserInterface/Widgets/DemonInteracted.h>
#include <UserInterface/Widgets/ProcessList.hpp>
#include <UserInterface/Widgets/Chat.hpp>
#include <UserInterface/Widgets/LootWidget.h>
#include <UserInterface/Widgets/FileBrowser.hpp>
#include <Mugen/Tengu/TenguCmdDispatch.h>

#include <UserInterface/SmallWidgets/EventViewer.hpp>

#include <Util/ColorText.h>
#include <Mugen/Packager.hpp>
#include <Mugen/Connector.hpp>

#include <QFile>
#include <QToolButton>
#include <QHeaderView>
#include <QByteArray>
#include <QKeyEvent>
#include <QShortcut>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QInputDialog>
#include <Mugen/DBManager/DBManager.hpp>

using namespace UserInterface::Widgets;

void MugenNamespace::UserInterface::Widgets::TeamserverTabSession::setupUi( QWidget* Page, QString TeamserverName )
{
    TeamserverName = TeamserverName;
    PageWidget = Page;

    SmallAppWidgets = new SmallAppWidgets_t;
    SmallAppWidgets->EventViewer = new UserInterface::SmallWidgets::EventViewer;
    SmallAppWidgets->EventViewer->setupUi( new QWidget );
    SmallAppWidgets->EventViewer->AppendText( CurrentDateTime(), "Mugen Framework [Version: " + QString( Version.c_str() ) + "] [CodeName: " + QString( CodeName.c_str() ) + "]" );

    auto MenuStyle = QString(
        "QMenu {"
        "    background-color: #111118;"
        "    color: #f0f0ee;"
        "    border: 1px solid #2a2a3f;"
        "}"
        "QMenu::separator {"
        "    background: #2a2a3f;"
        "}"
        "QMenu::item:selected {"
        "    background: #2a2a3f;"
        "}"
        "QAction {"
        "    background-color: #111118;"
        "    color: #f0f0ee;"
        "}"
    );

    gridLayout = new QGridLayout(PageWidget);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    gridLayout->setContentsMargins(0, 0, 0, 0);

    splitter_TopBot = new QSplitter(PageWidget);
    splitter_TopBot->setOrientation(Qt::Vertical);
    splitter_TopBot->setContentsMargins(0, 0, 0, 0);

    layoutWidget = new QWidget( splitter_TopBot );

    verticalLayout  = new QVBoxLayout( layoutWidget );
    verticalLayout->setContentsMargins( 3, 3, 3, 3 );

    MainViewWidget   = new QStackedWidget( );
    SessionTablePage = new QWidget( );

    gridLayout_2 = new QGridLayout( SessionTablePage );
    gridLayout_2->setObjectName( QString::fromUtf8( "gridLayout_2" ) );
    gridLayout_2->setContentsMargins( 0, 0, 0, 0 );

    splitter_SessionAndTabs = new QSplitter( layoutWidget );
    splitter_SessionAndTabs->setOrientation( Qt::Horizontal );

    SessionTableWidget = new MugenNamespace::UserInterface::Widgets::SessionTable;
    SessionTableWidget->setupUi( new QWidget(), TeamserverName );
    SessionTableWidget->setFocusPolicy( Qt::NoFocus );

    SessionGraphWidget = new GraphWidget( MainViewWidget );
    SessionMapWidget   = new SessionMap( MainViewWidget );

    // Session Table (index 0), Graph (index 1), Map (index 2)
    MainViewWidget->addWidget( SessionTableWidget );
    MainViewWidget->addWidget( SessionGraphWidget );
    MainViewWidget->addWidget( SessionMapWidget );
    MainViewWidget->setCurrentIndex( 0 );

    splitter_SessionAndTabs->addWidget( MainViewWidget );

    tabWidgetSmall = new QTabWidget( splitter_SessionAndTabs );
    tabWidgetSmall->setObjectName( QString::fromUtf8( "tabWidgetSmall" ) );
    tabWidgetSmall->setMovable( false );

    splitter_SessionAndTabs->addWidget( tabWidgetSmall );

    gridLayout_2->addWidget( splitter_SessionAndTabs, 0, 0, 1, 1 );

    verticalLayout->addWidget( SessionTablePage );

    splitter_TopBot->addWidget( layoutWidget );
    tabWidget = new QTabWidget( splitter_TopBot );
    tabWidget->setObjectName( QString::fromUtf8( "tabWidget" ) );
    splitter_TopBot->addWidget( tabWidget );

    gridLayout->addWidget(splitter_TopBot, 0, 0, 1, 1);

    TeamserverChat = new UserInterface::Widgets::Chat;
    TeamserverChat->TeamserverName = MugenX::Teamserver.Name;
    TeamserverChat->setupUi( new QWidget );

    NewBottomTab( TeamserverChat->ChatWidget, "Teamserver Chat", ":/icons/users" );
    tabWidget->setCurrentIndex( 0 );
    tabWidget->setMovable( false );

    LootWidget = new ::LootWidget;

    Dashboard = new DashboardWidget();
    NewWidgetTab( Dashboard, "Dashboard" );

    NewWidgetTab( SmallAppWidgets->EventViewer->EventViewer, "Event Viewer" );

    connect( SessionTableWidget->SessionTableWidget, &QTableWidget::customContextMenuRequested, this, &TeamserverTabSession::handleDemonContextMenu );
    connect( tabWidget->tabBar(), &QTabBar::tabCloseRequested, this, [&]( int index )
    {
        if ( index == -1 )
            return;

        // if closing a split view tab, unregister mirrors before removing
        if ( auto* split = dynamic_cast<UserInterface::Widgets::SplitConsoleWidget*>( tabWidget->widget( index ) ) )
            split->cleanup();

        tabWidget->removeTab( index );

        if ( tabWidget->count() == 0 )
        {
            splitter_TopBot->setSizes( QList<int>() << 0 );
            splitter_TopBot->setStyleSheet( "QSplitter::handle {  image: url(images/notExists.png); }" );
        }
        else if ( tabWidget->count() == 1 )
        {
            tabWidget->setMovable( false );
        }
    } );

    connect( tabWidgetSmall->tabBar(), &QTabBar::tabCloseRequested, this, &TeamserverTabSession::removeTabSmall );

    connect( SessionTableWidget->SessionTableWidget, &QTableWidget::doubleClicked, this, [&]( const QModelIndex &index ) {
        auto SessionID = SessionTableWidget->SessionTableWidget->item( index.row(), 0 )->text();

        for ( const auto& Session : MugenX::Teamserver.Sessions )
        {
            if ( Session.Name.compare( SessionID ) == 0 )
            {
                auto tabName = "[" + Session.Name + "] " + Session.User + "/" + Session.Computer;
                for ( int i = 0 ; i < MugenX::Teamserver.TabSession->tabWidget->count(); i++ )
                {
                    if ( MugenX::Teamserver.TabSession->tabWidget->tabText( i ) == tabName )
                    {
                        MugenX::Teamserver.TabSession->tabWidget->setCurrentIndex( i );
                        return;
                    }
                }

                MugenX::Teamserver.TabSession->NewBottomTab( Session.InteractedWidget->DemonInteractedWidget, tabName.toStdString() );
                Session.InteractedWidget->lineEdit->setFocus();
            }
        }
    } );
}

void UserInterface::Widgets::TeamserverTabSession::handleDemonContextMenu( const QPoint &pos )
{
    if ( ! SessionTableWidget->SessionTableWidget->itemAt( pos ) ) {
        return;
    }

    auto MenuStyle = QString(
        "QMenu {"
        "    background-color: #111118;"
        "    color: #f0f0ee;"
        "    border: 1px solid #2a2a3f;"
        "}"
        "QMenu::separator {"
        "    background: #2a2a3f;"
        "}"
        "QMenu::item:selected {"
        "    background: #2a2a3f;"
        "}"
    );

    // ── Multi-select bulk actions ─────────────────────────────────────────────
    auto selectedRows = SessionTableWidget->SessionTableWidget->selectionModel()->selectedRows();
    if ( selectedRows.size() > 1 )
    {
        // Collect selected agent IDs and their corresponding sessions
        QVector<QString> selectedIDs;
        for ( const auto& idx : selectedRows ) {
            auto* cell = SessionTableWidget->SessionTableWidget->item( idx.row(), 0 );
            if ( cell ) selectedIDs << cell->text();
        }

        auto bulkMenu = QMenu();
        bulkMenu.setStyleSheet( MenuStyle );

        auto title = bulkMenu.addAction( QString( "Bulk Actions (%1 agents)" ).arg( selectedIDs.size() ) );
        title->setEnabled( false );
        bulkMenu.addSeparator();
        bulkMenu.addAction( "Shell" );
        bulkMenu.addAction( "Sleep" );
        bulkMenu.addSeparator();
        bulkMenu.addAction( "Kill" );

        auto* action = bulkMenu.exec(
            SessionTableWidget->SessionTableWidget->horizontalHeader()->viewport()->mapToGlobal( pos )
        );

        if ( !action ) return;

        auto ensureWidget = [&]( Util::SessionItem& s ) {
            if ( s.InteractedWidget == nullptr ) {
                s.InteractedWidget                 = new UserInterface::Widgets::DemonInteracted;
                s.InteractedWidget->SessionInfo    = s;
                s.InteractedWidget->TeamserverName = MugenX::Teamserver.Name;
                s.InteractedWidget->setupUi( new QWidget );
            }
        };

        if ( action->text() == "Shell" )
        {
            bool ok = false;
            auto cmd = QInputDialog::getText(
                this, "Bulk Shell",
                QString( "Command to run on %1 agents:" ).arg( selectedIDs.size() ),
                QLineEdit::Normal, QString(), &ok
            );
            if ( !ok || cmd.trimmed().isEmpty() ) return;

            for ( auto& s : MugenX::Teamserver.Sessions ) {
                if ( selectedIDs.contains( s.Name ) ) {
                    ensureWidget( s );
                    s.InteractedWidget->AppendText( "shell " + cmd.trimmed() );
                }
            }
        }
        else if ( action->text() == "Sleep" )
        {
            bool ok = false;
            auto val = QInputDialog::getText(
                this, "Bulk Sleep",
                QString( "Sleep interval for %1 agents (seconds):" ).arg( selectedIDs.size() ),
                QLineEdit::Normal, "60", &ok
            );
            if ( !ok || val.trimmed().isEmpty() ) return;

            for ( auto& s : MugenX::Teamserver.Sessions ) {
                if ( selectedIDs.contains( s.Name ) ) {
                    ensureWidget( s );
                    s.InteractedWidget->AppendText( "sleep " + val.trimmed() );
                }
            }
        }
        else if ( action->text() == "Kill" )
        {
            for ( auto& s : MugenX::Teamserver.Sessions ) {
                if ( selectedIDs.contains( s.Name ) ) {
                    ensureWidget( s );
                    if ( s.MagicValue == DemonMagicValue )
                        s.InteractedWidget->AppendText( "exit process" );
                    else
                        s.InteractedWidget->AppendText( "exit" );
                }
            }
        }

        return;
    }

    auto SessionID = SessionTableWidget->SessionTableWidget->item( SessionTableWidget->SessionTableWidget->currentRow(), 0 )->text();
    auto Agent     = Util::SessionItem{};

    for ( auto s : MugenX::Teamserver.Sessions )
    {
        if ( s.Name.compare( SessionID ) == 0 )
        {
            Agent = s;
            break;
        }
    }

    auto separator  = new QAction();
    auto separator2 = new QAction();
    auto separator3 = new QAction();
    auto separator4 = new QAction();

    separator->setSeparator( true );
    separator2->setSeparator( true );
    separator3->setSeparator( true );
    separator4->setSeparator( true );

    auto SessionMenu        = QMenu();
    auto SessionExplorer    = QMenu( "Explorer" );
    auto TenguExplorer      = QMenu( "Explorer" );
    auto ExitMenu           = QMenu( "Exit" );
    auto ColorMenu          = QMenu( "Color" );
    auto SplitMenu          = QMenu( "Split with..." );
    auto NetworkingMenu     = QMenu( "Networking" );
    auto Socks5Menu         = QMenu( "SOCKS5" );
    auto PortFwdMenu        = QMenu( "Port Forward" );

    ColorMenu.addAction( "Reset" );
    ColorMenu.addAction( "Red" );
    ColorMenu.addAction( "Blue" );
    ColorMenu.addAction( "Yellow" );
    ColorMenu.addAction( "Pink" );
    ColorMenu.addAction( "Green" );
    ColorMenu.addAction( "Purple" );
    ColorMenu.addAction( "Orange" );

    ColorMenu.setStyleSheet( MenuStyle );

    SessionExplorer.addAction( "Process List" );
    SessionExplorer.addAction( "File Explorer" );
    SessionExplorer.setStyleSheet( MenuStyle );

    TenguExplorer.addAction( "Process List" );
    TenguExplorer.addAction( "File Explorer" );
    TenguExplorer.setStyleSheet( MenuStyle );

    ExitMenu.addAction( "Thread" );
    ExitMenu.addAction( "Process" );
    ExitMenu.setStyleSheet( MenuStyle );

    Socks5Menu.addAction( "Start..." );
    Socks5Menu.addAction( "Stop" );
    Socks5Menu.setStyleSheet( MenuStyle );

    PortFwdMenu.addAction( "Add..." );
    PortFwdMenu.addAction( "Remove..." );
    PortFwdMenu.setStyleSheet( MenuStyle );

    NetworkingMenu.addMenu( &Socks5Menu );
    NetworkingMenu.addMenu( &PortFwdMenu );
    NetworkingMenu.setStyleSheet( MenuStyle );

    // populate "Split with..." with all other live sessions
    for ( const auto& s : MugenX::Teamserver.Sessions )
    {
        if ( s.Name != SessionID && s.Marked != "Dead" )
            SplitMenu.addAction( "[" + s.Name + "] " + s.User + "/" + s.Computer );
    }
    SplitMenu.setStyleSheet( MenuStyle );
    SplitMenu.setEnabled( ! SplitMenu.isEmpty() );

    SessionMenu.addAction( "Interact" );
    SessionMenu.addAction( SplitMenu.menuAction() );
    SessionMenu.addAction( separator );

    if ( Agent.MagicValue == DemonMagicValue )
    {
        SessionMenu.addAction( SessionExplorer.menuAction() );
        SessionMenu.addAction( NetworkingMenu.menuAction() );
        SessionMenu.addAction( separator2 );
    }
    else if ( Agent.MagicValue == TenguMagicValue )
    {
        SessionMenu.addAction( TenguExplorer.menuAction() );
        SessionMenu.addAction( NetworkingMenu.menuAction() );
        SessionMenu.addAction( separator2 );
    }

    if ( Agent.Marked.compare( "Dead" ) != 0 )
        SessionMenu.addAction( "Mark as Dead" );
    else
        SessionMenu.addAction( "Mark as Alive" );

    SessionMenu.addAction( ColorMenu.menuAction() );
    SessionMenu.addAction( "Notes & Tags" );
    SessionMenu.addAction( "Export" );
    SessionMenu.addAction( separator3 );
    SessionMenu.addAction( "Remove" );

    if ( Agent.MagicValue == DemonMagicValue )
    {
        SessionMenu.addAction( ExitMenu.menuAction() );
    }
    else
    {
        SessionMenu.addAction( "Exit" );
    }

    SessionMenu.setStyleSheet( MenuStyle );

    auto *action = SessionMenu.exec( SessionTableWidget->SessionTableWidget->horizontalHeader()->viewport()->mapToGlobal( pos ) );

    if ( action )
    {
        for ( auto& Session : MugenX::Teamserver.Sessions )
        {
            // TODO: make that on Session receive
            if ( Session.InteractedWidget == nullptr )
            {
                Session.InteractedWidget                 = new UserInterface::Widgets::DemonInteracted;
                Session.InteractedWidget->SessionInfo    = Session;
                Session.InteractedWidget->TeamserverName = MugenX::Teamserver.Name;
                Session.InteractedWidget->setupUi( new QWidget );
            }

            if ( Session.Name.compare( SessionID ) == 0 )
            {
                if ( action->text().compare( "Interact" ) == 0 )
                {
                    auto tabName = "[" + Session.Name + "] " + Session.User + "/" + Session.Computer;
                    for ( int i = 0 ; i < MugenX::Teamserver.TabSession->tabWidget->count(); i++ )
                    {
                        if ( MugenX::Teamserver.TabSession->tabWidget->tabText( i ) == tabName )
                        {
                            MugenX::Teamserver.TabSession->tabWidget->setCurrentIndex( i );
                            return;
                        }
                    }

                    MugenX::Teamserver.TabSession->NewBottomTab( Session.InteractedWidget->DemonInteractedWidget, tabName.toStdString() );
                    Session.InteractedWidget->lineEdit->setFocus();
                }
                else if ( action->text().startsWith( "[" ) && SplitMenu.actions().contains( action ) )
                {
                    // find the second session from the action text "[ID] user/host"
                    auto secondID = action->text().section( ']', 0, 0 ).mid( 1 ).trimmed();
                    Util::SessionItem* secondSession = nullptr;
                    for ( auto& s : MugenX::Teamserver.Sessions )
                    {
                        if ( s.Name == secondID )
                        {
                            if ( s.InteractedWidget == nullptr )
                            {
                                s.InteractedWidget                 = new UserInterface::Widgets::DemonInteracted;
                                s.InteractedWidget->SessionInfo    = s;
                                s.InteractedWidget->TeamserverName = MugenX::Teamserver.Name;
                                s.InteractedWidget->setupUi( new QWidget );
                            }
                            secondSession = &s;
                            break;
                        }
                    }

                    if ( secondSession )
                    {
                        auto* splitWidget = new UserInterface::Widgets::SplitConsoleWidget;
                        splitWidget->setupUi( Session, *secondSession, MugenX::Teamserver.Name );

                        auto splitTabName = QString( "Split [%1] [%2]" ).arg( Session.Name ).arg( secondSession->Name );
                        MugenX::Teamserver.TabSession->NewBottomTab( splitWidget, splitTabName.toStdString() );
                        splitWidget->leftConsole->lineEdit->setFocus();
                    }
                }
                else if ( action->text().compare( "Red" ) == 0 || action->text().compare( "Blue" ) == 0 || action->text().compare( "Pink" ) == 0 || action->text().compare( "Yellow" ) == 0 || action->text().compare( "Green" ) == 0 || action->text().compare( "Purple" ) == 0 || action->text().compare( "Orange" ) == 0 || action->text().compare( "Reset" ) == 0 ){

                    for ( int i = 0; i < MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->rowCount(); ++i ){
                        auto AgentID = MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->item(i, 0)->text();

                        if(AgentID.compare( SessionID ) == 0 )
                        {

                            QColor CurrentColor;

                            if( action->text().compare("Red") == 0 )
                                CurrentColor = QColor( Util::ColorText::Colors::Hex::SessionRed );
                            else if( action->text().compare("Blue") == 0 )
                                CurrentColor = QColor( Util::ColorText::Colors::Hex::SessionCyan );
                            else if( action->text().compare("Pink") == 0 )
                                CurrentColor = QColor( Util::ColorText::Colors::Hex::SessionPink );
                            else if( action->text().compare("Yellow") == 0 )
                                CurrentColor = QColor( Util::ColorText::Colors::Hex::SessionYellow );
                            else if( action->text().compare("Green") == 0 )
                                CurrentColor = QColor( Util::ColorText::Colors::Hex::SessionGreen );
                            else if( action->text().compare("Purple") == 0 )
                                CurrentColor = QColor( Util::ColorText::Colors::Hex::SessionPurple );
                            else if( action->text().compare("Orange") == 0 )
                                CurrentColor = QColor( Util::ColorText::Colors::Hex::SessionOrange );
                            else
                                CurrentColor = QColor( Util::ColorText::Colors::Hex::Background );

                            for ( int j = 0; j < MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->columnCount(); j++ )
                            {
                                MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->item(i, j)->setBackground( CurrentColor );
                            }

                        }
                    }
                    
                }
                else if ( action->text().compare( "Mark as Dead" ) == 0 || action->text().compare( "Mark as Alive" ) == 0 )
                {
                    for ( int i = 0; i <  MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->rowCount(); i++ )
                    {
                        auto AgentID = MugenX::Teamserver.TabSession->SessionTableWidget->SessionTableWidget->item( i, 0 )->text();

                        if ( AgentID.compare( SessionID ) == 0 )
                        {
                            auto Package = new Util::Packager::Package;

                            Package->Head = Util::Packager::Head_t {
                                    .Event= Util::Packager::Session::Type,
                                    .User = MugenX::Teamserver.User.toStdString(),
                                    .Time = CurrentTime().toStdString(),
                            };

                            auto Marked = QString();

                            if ( action->text().compare( "Mark as Alive" ) == 0 )
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
                            else if ( action->text().compare( "Mark as Dead" ) == 0 )
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
                                    .SubEvent = 0x5,
                                    .Info = {
                                        { "AgentID", AgentID.toStdString() },
                                        { "Marked",  Marked.toStdString() },
                                    }
                            };

                            MugenX::Connector->SendPackage( Package );
                        }
                    }
                }
                else if ( action->text().compare( "Export" ) == 0 )
                {
                    Session.Export();
                }
                else if ( action->text().compare( "Notes & Tags" ) == 0 )
                {
                    auto dialog = new QDialog( this->window() );
                    dialog->setWindowTitle( "Notes & Tags - " + Session.Name );
                    dialog->setMinimumSize( 480, 320 );

                    auto layout       = new QVBoxLayout( dialog );
                    auto labelTags    = new QLabel( "Tags (comma-separated):" );
                    auto editTags     = new QLineEdit( Session.Tags );
                    auto labelNotes   = new QLabel( "Notes:" );
                    auto editNotes    = new QTextEdit( Session.Notes );
                    auto btnLayout    = new QHBoxLayout();
                    auto btnSave      = new QPushButton( "Save" );
                    auto btnCancel    = new QPushButton( "Cancel" );

                    editTags->setPlaceholderText( "e.g. domain-admin, web-server, pivoted" );
                    editNotes->setPlaceholderText( "Operator notes..." );

                    btnLayout->addStretch();
                    btnLayout->addWidget( btnSave );
                    btnLayout->addWidget( btnCancel );

                    layout->addWidget( labelTags );
                    layout->addWidget( editTags );
                    layout->addWidget( labelNotes );
                    layout->addWidget( editNotes );
                    layout->addLayout( btnLayout );

                    auto agentID  = Session.Name;
                    auto tabPtr   = this;

                    QObject::connect( btnSave, &QPushButton::clicked, dialog, [=]() {
                        auto tags  = editTags->text().trimmed();
                        auto notes = editNotes->toPlainText().trimmed();

                        for ( auto& s : MugenX::Teamserver.Sessions ) {
                            if ( s.Name == agentID ) {
                                s.Tags  = tags;
                                s.Notes = notes;
                                break;
                            }
                        }

                        tabPtr->SessionTableWidget->SetSessionTags( agentID, tags );

                        if ( tabPtr->dbManager )
                            tabPtr->dbManager->SetSessionMeta( agentID, tags, notes );

                        dialog->accept();
                    } );

                    QObject::connect( btnCancel, &QPushButton::clicked, dialog, &QDialog::reject );

                    dialog->exec();
                    delete dialog;
                }
                else if ( action->text().compare( "Remove" ) == 0 )
                {
                    auto SessionID = SessionTableWidget->SessionTableWidget->item( SessionTableWidget->SessionTableWidget->currentRow(), 0 )->text();

                    for ( auto & Session : MugenX::Teamserver.Sessions )
                    {
                        if ( SessionID.compare( Session.Name ) == 0 )
                        {
                            SessionTableWidget->SessionTableWidget->removeRow( SessionTableWidget->SessionTableWidget->currentRow() );
                            MugenX::Teamserver.TabSession->SessionGraphWidget->GraphNodeRemove( Session );
                        }
                    }
                }
                else if ( action->text().compare( "Thread" ) == 0 || action->text().compare( "Process" ) == 0 )
                {
                    Session.InteractedWidget->AppendText( "exit " + action->text().toLower() );
                }

                if ( Session.MagicValue == DemonMagicValue )
                {
                    if ( action->text().compare( "Process List" ) == 0 )
                    {
                        auto TabName = QString( "[" + SessionID + "] Process List" );

                        if ( Session.ProcessList == nullptr )
                        {
                            Session.ProcessList = new UserInterface::Widgets::ProcessList;
                            Session.ProcessList->setupUi( new QWidget );
                            Session.ProcessList->Session = Session;
                            Session.ProcessList->Teamserver = MugenX::Teamserver.Name;

                            MugenX::Teamserver.TabSession->NewBottomTab( Session.ProcessList->ProcessListWidget, TabName.toStdString() );
                            Session.InteractedWidget->DemonCommands->Execute.ProcList( Util::gen_random( 8 ).c_str(), true );
                        }
                        else
                        {
                            MugenX::Teamserver.TabSession->NewBottomTab( Session.ProcessList->ProcessListWidget, TabName.toStdString() );
                        }
                    }
                    else if ( action->text().compare( "File Explorer" ) == 0 )
                    {
                        auto TabName = QString( "[" + SessionID + "] File Explorer" );

                        if ( Session.FileBrowser == nullptr )
                        {
                            Session.FileBrowser = new FileBrowser;
                            Session.FileBrowser->setupUi( new QWidget );
                            Session.FileBrowser->SessionID = Session.Name;

                            MugenX::Teamserver.TabSession->NewBottomTab( Session.FileBrowser->FileBrowserWidget, TabName.toStdString(), "" );
                            Session.InteractedWidget->DemonCommands->Execute.FS( Util::gen_random( 8 ).c_str(), "dir;ui", "." );
                        }
                        else
                        {
                            MugenX::Teamserver.TabSession->NewBottomTab( Session.FileBrowser->FileBrowserWidget, TabName.toStdString(), "" );
                        }
                    }
                }
                else if ( Session.MagicValue == TenguMagicValue )
                {
                    if ( action->text().compare( "Process List" ) == 0 )
                    {
                        auto TabName = QString( "[" + SessionID + "] Process List" );

                        if ( Session.ProcessList == nullptr )
                        {
                            Session.ProcessList = new UserInterface::Widgets::ProcessList;
                            Session.ProcessList->setupUi( new QWidget );
                            Session.ProcessList->Session = Session;
                            Session.ProcessList->Teamserver = MugenX::Teamserver.Name;

                            MugenX::Teamserver.TabSession->NewBottomTab( Session.ProcessList->ProcessListWidget, TabName.toStdString() );
                        }
                        else
                        {
                            MugenX::Teamserver.TabSession->NewBottomTab( Session.ProcessList->ProcessListWidget, TabName.toStdString() );
                        }

                        auto TaskID = QString( Util::gen_random( 8 ).c_str() );
                        if ( Session.InteractedWidget->TenguCmds )
                            Session.InteractedWidget->TenguCmds->SendTask( TaskID, "ps;ui" );
                    }
                    else if ( action->text().compare( "File Explorer" ) == 0 )
                    {
                        auto TabName = QString( "[" + SessionID + "] File Explorer" );

                        if ( Session.FileBrowser == nullptr )
                        {
                            Session.FileBrowser = new FileBrowser;
                            Session.FileBrowser->setupUi( new QWidget );
                            Session.FileBrowser->SessionID = Session.Name;
                            Session.FileBrowser->IsTengu   = true;

                            MugenX::Teamserver.TabSession->NewBottomTab( Session.FileBrowser->FileBrowserWidget, TabName.toStdString(), "" );

                            auto TaskID = QString( Util::gen_random( 8 ).c_str() );
                            if ( Session.InteractedWidget->TenguCmds )
                                Session.InteractedWidget->TenguCmds->SendTask( TaskID, "ls;ui ." );
                        }
                        else
                        {
                            MugenX::Teamserver.TabSession->NewBottomTab( Session.FileBrowser->FileBrowserWidget, TabName.toStdString(), "" );
                        }
                    }
                }

                // ── Networking ─────────────────────────────────────────────
                if ( action->text().compare( "Start..." ) == 0 && Socks5Menu.actions().contains( action ) )
                {
                    bool ok = false;
                    auto portStr = QInputDialog::getText(
                        this, "SOCKS5 Start", "Listen port:", QLineEdit::Normal, "1080", &ok );
                    if ( ok && !portStr.isEmpty() )
                    {
                        Session.InteractedWidget->AppendText( "socks5 start " + portStr );
                        int port = portStr.toInt();
                        if ( SessionTableWidget )
                        {
                            for ( int i = SessionTableWidget->Socks5Tunnels.size() - 1; i >= 0; --i )
                                if ( SessionTableWidget->Socks5Tunnels[i].agentID == SessionID )
                                    SessionTableWidget->Socks5Tunnels.removeAt( i );
                            SessionTableWidget->Socks5Tunnels.append( { SessionID, port } );
                        }
                    }
                }
                else if ( action->text().compare( "Stop" ) == 0 && Socks5Menu.actions().contains( action ) )
                {
                    Session.InteractedWidget->AppendText( "socks5 stop" );
                    if ( SessionTableWidget )
                        for ( int i = SessionTableWidget->Socks5Tunnels.size() - 1; i >= 0; --i )
                            if ( SessionTableWidget->Socks5Tunnels[i].agentID == SessionID )
                                SessionTableWidget->Socks5Tunnels.removeAt( i );
                }
                else if ( action->text().compare( "Add..." ) == 0 && PortFwdMenu.actions().contains( action ) )
                {
                    bool ok1 = false, ok2 = false, ok3 = false;
                    auto bindPortStr = QInputDialog::getText(
                        this, "Port Forward Add", "Bind port (on teamserver):", QLineEdit::Normal, "8080", &ok1 );
                    if ( !ok1 || bindPortStr.isEmpty() ) return;
                    auto remoteHost = QInputDialog::getText(
                        this, "Port Forward Add", "Remote host:", QLineEdit::Normal, "192.168.1.1", &ok2 );
                    if ( !ok2 || remoteHost.isEmpty() ) return;
                    auto remotePortStr = QInputDialog::getText(
                        this, "Port Forward Add", "Remote port:", QLineEdit::Normal, "80", &ok3 );
                    if ( !ok3 || remotePortStr.isEmpty() ) return;
                    Session.InteractedWidget->AppendText(
                        "rportfwd add " + bindPortStr + " " + remoteHost + " " + remotePortStr );
                    if ( SessionTableWidget )
                        SessionTableWidget->PortForwards.append(
                            { SessionID, bindPortStr.toInt(), remoteHost, remotePortStr.toInt() } );
                }
                else if ( action->text().compare( "Remove..." ) == 0 && PortFwdMenu.actions().contains( action ) )
                {
                    bool ok = false;
                    auto bindPortStr = QInputDialog::getText(
                        this, "Port Forward Remove", "Bind port to remove:", QLineEdit::Normal, "", &ok );
                    if ( ok && !bindPortStr.isEmpty() )
                    {
                        Session.InteractedWidget->AppendText( "rportfwd rm " + bindPortStr );
                        if ( SessionTableWidget )
                        {
                            int bp = bindPortStr.toInt();
                            for ( int i = SessionTableWidget->PortForwards.size() - 1; i >= 0; --i )
                                if ( SessionTableWidget->PortForwards[i].agentID == SessionID &&
                                     SessionTableWidget->PortForwards[i].bindPort == bp )
                                    SessionTableWidget->PortForwards.removeAt( i );
                        }
                    }
                }
            }
        }

    }

    delete separator;
    delete separator2;
    delete separator3;
    delete separator4;
}


void UserInterface::Widgets::TeamserverTabSession::NewBottomTab( QWidget* TabWidget, const string& TitleName, const QString IconPath ) const
{
    int id = 0;
    if ( tabWidget->count() == 0 )
    {
        splitter_TopBot->setSizes( QList<int>() << 100 << 200 );
        splitter_TopBot->setStyleSheet( "" );
    }
    else if ( tabWidget->count() == 1 )
        tabWidget->setMovable( true );

    tabWidget->setTabsClosable( true );

    id = tabWidget->addTab( TabWidget, TitleName.c_str() );

    tabWidget->setIconSize( QSize( 15, 15 ) );
    tabWidget->setCurrentIndex( id );
}

void UserInterface::Widgets::TeamserverTabSession::NewWidgetTab( QWidget *TabWidget, const std::string &TitleName ) const
{
    if ( tabWidgetSmall->count() == 0 ) {
        splitter_SessionAndTabs->setSizes( QList<int>() << 200 << 10 );
        splitter_SessionAndTabs->setStyleSheet( "" );
        splitter_SessionAndTabs->handle( 1 )->setEnabled( true );
        splitter_SessionAndTabs->handle( 1 )->setCursor( Qt::SplitHCursor );
    } else if ( tabWidgetSmall->count() == 1 ) {
        tabWidgetSmall->setMovable( true );
    }

    tabWidgetSmall->setTabsClosable( true );

    tabWidget->setCurrentIndex(
        tabWidgetSmall->addTab(
            TabWidget,
            TitleName.c_str()
        )
    );
}

void UserInterface::Widgets::TeamserverTabSession::OpenProcessList( const QString& sessionID )
{
    for ( auto& Session : MugenX::Teamserver.Sessions )
    {
        if ( Session.Name != sessionID ) continue;

        if ( Session.InteractedWidget == nullptr ) {
            Session.InteractedWidget                 = new UserInterface::Widgets::DemonInteracted;
            Session.InteractedWidget->SessionInfo    = Session;
            Session.InteractedWidget->TeamserverName = MugenX::Teamserver.Name;
            Session.InteractedWidget->setupUi( new QWidget );
        }

        auto tabName = QString( "[%1] Process List" ).arg( sessionID );

        if ( Session.ProcessList == nullptr ) {
            Session.ProcessList = new UserInterface::Widgets::ProcessList;
            Session.ProcessList->setupUi( new QWidget );
            Session.ProcessList->Session   = Session;
            Session.ProcessList->Teamserver = MugenX::Teamserver.Name;
            NewBottomTab( Session.ProcessList->ProcessListWidget, tabName.toStdString() );

            if ( Session.MagicValue == DemonMagicValue ) {
                Session.InteractedWidget->DemonCommands->Execute.ProcList( Util::gen_random( 8 ).c_str(), true );
            } else if ( Session.MagicValue == TenguMagicValue ) {
                auto taskID = QString( Util::gen_random( 8 ).c_str() );
                if ( Session.InteractedWidget->TenguCmds )
                    Session.InteractedWidget->TenguCmds->SendTask( taskID, "ps;ui" );
            }
        } else {
            NewBottomTab( Session.ProcessList->ProcessListWidget, tabName.toStdString() );
        }
        break;
    }
}

void UserInterface::Widgets::TeamserverTabSession::OpenFileBrowser( const QString& sessionID )
{
    for ( auto& Session : MugenX::Teamserver.Sessions )
    {
        if ( Session.Name != sessionID ) continue;

        if ( Session.InteractedWidget == nullptr ) {
            Session.InteractedWidget                 = new UserInterface::Widgets::DemonInteracted;
            Session.InteractedWidget->SessionInfo    = Session;
            Session.InteractedWidget->TeamserverName = MugenX::Teamserver.Name;
            Session.InteractedWidget->setupUi( new QWidget );
        }

        auto tabName = QString( "[%1] File Explorer" ).arg( sessionID );

        if ( Session.FileBrowser == nullptr ) {
            Session.FileBrowser = new FileBrowser;
            Session.FileBrowser->setupUi( new QWidget );
            Session.FileBrowser->SessionID = Session.Name;
            Session.FileBrowser->IsTengu   = ( Session.MagicValue == TenguMagicValue );
            NewBottomTab( Session.FileBrowser->FileBrowserWidget, tabName.toStdString(), "" );

            if ( Session.MagicValue == DemonMagicValue ) {
                Session.InteractedWidget->DemonCommands->Execute.FS( Util::gen_random( 8 ).c_str(), "dir;ui", "." );
            } else if ( Session.MagicValue == TenguMagicValue ) {
                auto taskID = QString( Util::gen_random( 8 ).c_str() );
                if ( Session.InteractedWidget->TenguCmds )
                    Session.InteractedWidget->TenguCmds->SendTask( taskID, "ls;ui ." );
            }
        } else {
            NewBottomTab( Session.FileBrowser->FileBrowserWidget, tabName.toStdString(), "" );
        }
        break;
    }
}

void UserInterface::Widgets::TeamserverTabSession::OpenPayloadBuilder()
{
    if ( PayloadDialog == nullptr ) {
        PayloadDialog = new Payload;
        PayloadDialog->setupUi( new QDialog( PageWidget->window() ) );
        PayloadDialog->TeamserverName = MugenX::Teamserver.Name;
    }
    PayloadDialog->Start();
}

void UserInterface::Widgets::TeamserverTabSession::removeTabSmall( int index ) const
{
    if ( index == -1 ) {
        return;
    }

    tabWidgetSmall->removeTab( index );

    if ( tabWidgetSmall->count() == 0 ) {
        splitter_SessionAndTabs->setSizes( QList<int>() << 0 );
        splitter_SessionAndTabs->setStyleSheet( "QSplitter::handle { image: url(images/notExists.png); }" );
        splitter_SessionAndTabs->handle( 1 )->setEnabled( false );
        splitter_SessionAndTabs->handle( 1 )->setCursor( Qt::ArrowCursor );
    } else if ( tabWidgetSmall->count() == 1 ) {
        tabWidgetSmall->setMovable( false );
    }
}
