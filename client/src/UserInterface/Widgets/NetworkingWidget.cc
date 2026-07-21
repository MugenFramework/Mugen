#include <Mugen/Mugen.hpp>
#include <global.hpp>

#include <UserInterface/Widgets/NetworkingWidget.hpp>
#include <UserInterface/Widgets/TeamserverTabSession.h>
#include <UserInterface/Widgets/SessionTable.hpp>
#include <UserInterface/Widgets/DemonInteracted.h>
#include <Mugen/Service.hpp>
#include <Util/Base.hpp>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QSpacerItem>

using namespace MugenNamespace::UserInterface::Widgets;

// ── Helpers ───────────────────────────────────────────────────────────────────

static QString sessionOS( const QString& agentID )
{
    for ( const auto& s : MugenX::Teamserver.Sessions )
        if ( s.Name == agentID ) return s.OS;
    return {};
}

static QTableWidgetItem* osIconItem( const QString& agentID )
{
    auto* item = new QTableWidgetItem();
    item->setIcon( WinVersionIcon( sessionOS( agentID ), false ) );
    item->setTextAlignment( Qt::AlignCenter );
    item->setFlags( item->flags() & ~Qt::ItemIsEditable );
    return item;
}

static QTableWidgetItem* cell( const QString& text )
{
    auto* item = new QTableWidgetItem( text );
    item->setFlags( item->flags() & ~Qt::ItemIsEditable );
    return item;
}

static void sendToAgent( const QString& agentID, const QString& cmd, const QString& teamserverName )
{
    for ( auto& s : MugenX::Teamserver.Sessions )
    {
        if ( s.Name != agentID ) continue;
        if ( s.InteractedWidget == nullptr )
        {
            s.InteractedWidget                 = new UserInterface::Widgets::DemonInteracted;
            s.InteractedWidget->SessionInfo    = s;
            s.InteractedWidget->TeamserverName = teamserverName;
            s.InteractedWidget->setupUi( new QWidget );
        }
        s.InteractedWidget->AppendText( cmd );
        break;
    }
}

// ── NetworkingWidget ──────────────────────────────────────────────────────────

NetworkingWidget::NetworkingWidget( QWidget* parent ) : QWidget( parent ) {}

void NetworkingWidget::setupUi( QWidget* widget )
{
    NetworkingView = widget ? widget : new QWidget();
    NetworkingView->setObjectName( "NetworkingView" );

    auto* root = new QGridLayout( NetworkingView );
    root->setObjectName( "gridLayout" );
    root->setContentsMargins( 0, 0, 0, 3 );

    // ── Tabs ──────────────────────────────────────────────────────────────
    Tabs = new QTabWidget( NetworkingView );
    Tabs->setObjectName( "tabWidget" );
    root->addWidget( Tabs, 0, 0, 1, 4 );

    // ── SOCKS5 tab ────────────────────────────────────────────────────────
    auto* socksPage   = new QWidget();
    auto* socksLayout = new QGridLayout( socksPage );
    socksLayout->setContentsMargins( 0, 0, 0, 3 );

    Socks5Table = new QTableWidget( 0, 3, socksPage );
    Socks5Table->setHorizontalHeaderItem( 0, new QTableWidgetItem( "" )               );
    Socks5Table->setHorizontalHeaderItem( 1, new QTableWidgetItem( "Agent"          ) );
    Socks5Table->setHorizontalHeaderItem( 2, new QTableWidgetItem( "Local Endpoint" ) );
    Socks5Table->horizontalHeader()->setSectionResizeMode( 0, QHeaderView::Fixed );
    Socks5Table->horizontalHeader()->setSectionResizeMode( 1, QHeaderView::Stretch );
    Socks5Table->horizontalHeader()->setSectionResizeMode( 2, QHeaderView::ResizeToContents );
    Socks5Table->horizontalHeader()->resizeSection( 0, 48 );
    Socks5Table->horizontalHeader()->setHighlightSections( false );
    Socks5Table->verticalHeader()->setVisible( false );
    Socks5Table->verticalHeader()->setDefaultSectionSize( 42 );
    Socks5Table->setShowGrid( false );
    Socks5Table->setFocusPolicy( Qt::NoFocus );
    Socks5Table->setSortingEnabled( false );
    Socks5Table->setSelectionBehavior( QAbstractItemView::SelectRows );
    Socks5Table->setSelectionMode( QAbstractItemView::SingleSelection );
    Socks5Table->setIconSize( QSize( 26, 26 ) );

    socksLayout->addWidget( Socks5Table, 0, 0, 1, 4 );

    auto* socks5Spacer1 = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    socksLayout->addItem( socks5Spacer1, 1, 0 );

    StopButton = new QPushButton( "Stop", socksPage );
    StopButton->setObjectName( "stopButton" );
    StopButton->setEnabled( false );
    socksLayout->addWidget( StopButton, 1, 1 );

    auto* socks5Spacer2 = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    socksLayout->addItem( socks5Spacer2, 1, 2 );

    Tabs->addTab( socksPage, "SOCKS5" );

    // ── Port Forwards tab ─────────────────────────────────────────────────
    auto* pfPage   = new QWidget();
    auto* pfLayout = new QGridLayout( pfPage );
    pfLayout->setContentsMargins( 0, 0, 0, 3 );

    RportfwdTable = new QTableWidget( 0, 4, pfPage );
    RportfwdTable->setHorizontalHeaderItem( 0, new QTableWidgetItem( "" )          );
    RportfwdTable->setHorizontalHeaderItem( 1, new QTableWidgetItem( "Agent"     ) );
    RportfwdTable->setHorizontalHeaderItem( 2, new QTableWidgetItem( "Bind Port" ) );
    RportfwdTable->setHorizontalHeaderItem( 3, new QTableWidgetItem( "Remote"    ) );
    RportfwdTable->horizontalHeader()->setSectionResizeMode( 0, QHeaderView::Fixed );
    RportfwdTable->horizontalHeader()->setSectionResizeMode( 1, QHeaderView::Stretch );
    RportfwdTable->horizontalHeader()->setSectionResizeMode( 2, QHeaderView::ResizeToContents );
    RportfwdTable->horizontalHeader()->setSectionResizeMode( 3, QHeaderView::Stretch );
    RportfwdTable->horizontalHeader()->resizeSection( 0, 48 );
    RportfwdTable->horizontalHeader()->setHighlightSections( false );
    RportfwdTable->verticalHeader()->setVisible( false );
    RportfwdTable->verticalHeader()->setDefaultSectionSize( 42 );
    RportfwdTable->setShowGrid( false );
    RportfwdTable->setFocusPolicy( Qt::NoFocus );
    RportfwdTable->setSortingEnabled( false );
    RportfwdTable->setSelectionBehavior( QAbstractItemView::SelectRows );
    RportfwdTable->setSelectionMode( QAbstractItemView::SingleSelection );
    RportfwdTable->setIconSize( QSize( 26, 26 ) );

    pfLayout->addWidget( RportfwdTable, 0, 0, 1, 4 );

    auto* pfSpacer1 = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    pfLayout->addItem( pfSpacer1, 1, 0 );

    RemoveButton = new QPushButton( "Remove", pfPage );
    RemoveButton->setObjectName( "removeButton" );
    RemoveButton->setEnabled( false );
    pfLayout->addWidget( RemoveButton, 1, 1 );

    auto* pfSpacer2 = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    pfLayout->addItem( pfSpacer2, 1, 2 );

    Tabs->addTab( pfPage, "Port Forwards" );

    // ── Connections ───────────────────────────────────────────────────────
    connect( Socks5Table, &QTableWidget::itemSelectionChanged, this, [this]() {
        StopButton->setEnabled( !Socks5Table->selectedItems().isEmpty() );
    } );

    connect( RportfwdTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        RemoveButton->setEnabled( !RportfwdTable->selectedItems().isEmpty() );
    } );

    connect( StopButton, &QPushButton::clicked, this, [this]() {
        int row = Socks5Table->currentRow();
        if ( row < 0 ) return;
        auto* agentItem = Socks5Table->item( row, 1 );
        if ( !agentItem ) return;
        sendToAgent( agentItem->text(), "socks5 stop", TeamserverName );
    } );

    connect( RemoveButton, &QPushButton::clicked, this, [this]() {
        int row = RportfwdTable->currentRow();
        if ( row < 0 ) return;
        auto* agentItem    = RportfwdTable->item( row, 1 );
        auto* bindPortItem = RportfwdTable->item( row, 2 );
        if ( !agentItem || !bindPortItem ) return;
        sendToAgent( agentItem->text(),
                     "rportfwd rm " + bindPortItem->text(),
                     TeamserverName );
    } );
}

void NetworkingWidget::Refresh()
{
    rebuildSocks5Table();
    rebuildRportfwdTable();
}

void NetworkingWidget::rebuildSocks5Table()
{
    if ( !Socks5Table ) return;

    Socks5Table->clearSpans();
    Socks5Table->setRowCount( 0 );
    StopButton->setEnabled( false );

    auto* tab = MugenX::Teamserver.TabSession;
    if ( !tab || !tab->SessionTableWidget ) return;

    const auto& tunnels = tab->SessionTableWidget->Socks5Tunnels;

    if ( tunnels.isEmpty() )
    {
        Socks5Table->insertRow( 0 );
        auto* empty = new QTableWidgetItem( "No active SOCKS5 tunnels" );
        empty->setTextAlignment( Qt::AlignCenter );
        empty->setForeground( QColor( "#3a3a5a" ) );
        empty->setFlags( Qt::ItemIsEnabled );
        Socks5Table->setItem( 0, 0, empty );
        Socks5Table->setSpan( 0, 0, 1, 3 );
        Socks5Table->setRowHeight( 0, 60 );
        return;
    }

    for ( int i = 0; i < tunnels.size(); i++ )
    {
        const auto& e = tunnels[i];
        Socks5Table->insertRow( i );
        Socks5Table->setItem( i, 0, osIconItem( e.agentID ) );
        Socks5Table->setItem( i, 1, cell( e.agentID ) );
        Socks5Table->setItem( i, 2, cell( QString( "127.0.0.1:%1" ).arg( e.port ) ) );
    }
}

void NetworkingWidget::rebuildRportfwdTable()
{
    if ( !RportfwdTable ) return;

    RportfwdTable->clearSpans();
    RportfwdTable->setRowCount( 0 );
    RemoveButton->setEnabled( false );

    auto* tab = MugenX::Teamserver.TabSession;
    if ( !tab || !tab->SessionTableWidget ) return;

    const auto& fwds = tab->SessionTableWidget->PortForwards;

    if ( fwds.isEmpty() )
    {
        RportfwdTable->insertRow( 0 );
        auto* empty = new QTableWidgetItem( "No active port forwards" );
        empty->setTextAlignment( Qt::AlignCenter );
        empty->setForeground( QColor( "#3a3a5a" ) );
        empty->setFlags( Qt::ItemIsEnabled );
        RportfwdTable->setItem( 0, 0, empty );
        RportfwdTable->setSpan( 0, 0, 1, 4 );
        RportfwdTable->setRowHeight( 0, 60 );
        return;
    }

    for ( int i = 0; i < fwds.size(); i++ )
    {
        const auto& e = fwds[i];
        RportfwdTable->insertRow( i );
        RportfwdTable->setItem( i, 0, osIconItem( e.agentID ) );
        RportfwdTable->setItem( i, 1, cell( e.agentID ) );
        RportfwdTable->setItem( i, 2, cell( QString::number( e.bindPort ) ) );
        RportfwdTable->setItem( i, 3, cell( e.remoteHost + ":" + QString::number( e.remotePort ) ) );
    }
}
