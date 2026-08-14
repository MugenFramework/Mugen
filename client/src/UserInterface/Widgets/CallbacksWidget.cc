#include <Mugen/Connector.hpp>
#include <Mugen/Mugen.hpp>
#include <global.hpp>

#include <UserInterface/Widgets/CallbacksWidget.hpp>
#include <UserInterface/Widgets/TeamserverTabSession.h>
#include <Mugen/Packager.hpp>
#include <Util/ColorText.h>
#include <Util/Base.hpp>

#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMenu>
#include <QAbstractItemView>
#include <QComboBox>
#include <QPainter>
#include <QPainterPath>
#include <QDateTime>
#include <QFrame>

using namespace MugenNamespace::UserInterface::Widgets;

static QTableWidgetItem* cbCell( const QString& text )
{
    auto* item = new QTableWidgetItem( text );
    item->setFlags( item->flags() & ~Qt::ItemIsEditable );
    item->setTextAlignment( Qt::AlignCenter );
    return item;
}

static QIcon iconEye( bool hidden )
{
    QPixmap pm( 18, 18 );
    pm.fill( Qt::transparent );
    QPainter p( &pm );
    p.setRenderHint( QPainter::Antialiasing );
    const QColor c( hidden ? "#ff6b9d" : "#8888aa" );
    p.setPen( QPen( c, 1.4 ) );
    p.setBrush( Qt::NoBrush );

    QPainterPath eye;
    eye.moveTo( 2, 9 );
    eye.cubicTo( 6, 3.5, 12, 3.5, 16, 9 );
    eye.cubicTo( 12, 14.5, 6, 14.5, 2, 9 );
    p.drawPath( eye );

    if ( hidden )
    {
        p.drawLine( 4, 14, 14, 4 );
    }
    else
    {
        p.setBrush( c );
        p.drawEllipse( QPointF( 9, 9 ), 2.1, 2.1 );
    }
    return QIcon( pm );
}

static QIcon iconInfo( bool active )
{
    QPixmap pm( 18, 18 );
    pm.fill( Qt::transparent );
    QPainter p( &pm );
    p.setRenderHint( QPainter::Antialiasing );
    const QColor c( active ? "#ff6b9d" : "#8888aa" );
    p.setPen( QPen( c, 1.4 ) );
    p.setBrush( Qt::NoBrush );
    p.drawEllipse( QRectF( 2.4, 2.4, 13.2, 13.2 ) );
    p.drawLine( 9, 8.2, 9, 13.2 );
    p.setBrush( c );
    p.setPen( Qt::NoPen );
    p.drawEllipse( QPointF( 9, 6.2 ), 1.15, 1.15 );
    return QIcon( pm );
}

static QToolButton* iconButton( QWidget* parent, const QIcon& icon, const QString& tip )
{
    auto* btn = new QToolButton( parent );
    btn->setIcon( icon );
    btn->setIconSize( QSize( 16, 16 ) );
    btn->setToolTip( tip );
    btn->setCursor( Qt::PointingHandCursor );
    btn->setAutoRaise( true );
    btn->setStyleSheet(
        "QToolButton { background:transparent; border:none; border-radius:3px; padding:2px; }"
        "QToolButton:hover { background:#2a2a3f; }"
    );
    return btn;
}

static QString dash( const QString& v )
{
    return v.trimmed().isEmpty() ? QString( "—" ) : v;
}

static QString formatKillDate( quint64 ticks )
{
    if ( ticks == 0 )
        return QString( "—" );
    const quint64 UNIX_TIME_START  = 0x019DB1DED53E8000ULL;
    const quint64 TICKS_PER_SECOND = 10000000ULL;
    if ( ticks < UNIX_TIME_START )
        return QString( "—" );
    auto epoch = ( ticks - UNIX_TIME_START ) / TICKS_PER_SECOND;
    return QDateTime::fromSecsSinceEpoch( (qint64)epoch, Qt::UTC ).toString( "yyyy-MM-dd HH:mm UTC" );
}

static QString formatWorkingHours( quint32 packed )
{
    if ( ( ( packed >> 22 ) & 1 ) != 1 )
        return QString( "—" );
    auto sh = ( packed >> 17 ) & 0b011111;
    auto sm = ( packed >> 11 ) & 0b111111;
    auto eh = ( packed >>  6 ) & 0b011111;
    auto em = ( packed >>  0 ) & 0b111111;
    return QString( "%1:%2–%3:%4" )
        .arg( sh, 2, 10, QChar( '0' ) )
        .arg( sm, 2, 10, QChar( '0' ) )
        .arg( eh, 2, 10, QChar( '0' ) )
        .arg( em, 2, 10, QChar( '0' ) );
}

static void addSection( QVBoxLayout* lay, const QString& title )
{
    auto* h = new QLabel( title.toUpper() );
    h->setStyleSheet(
        "color:#ff6b9d; font-size:10px; font-weight:bold; letter-spacing:1px;"
        "padding:10px 0 4px 0;"
    );
    lay->addWidget( h );
}

static void addField( QFormLayout* form, const QString& label, const QString& value )
{
    auto* lab = new QLabel( label );
    lab->setStyleSheet( "color:#8888aa; font-size:11px;" );
    auto* val = new QLabel( dash( value ) );
    val->setTextInteractionFlags( Qt::TextSelectableByMouse );
    val->setWordWrap( true );
    val->setStyleSheet( "color:#f0f0ee; font-size:11px; font-family:monospace;" );
    form->addRow( lab, val );
}

CallbacksWidget::CallbacksWidget( QWidget* parent ) : QWidget( parent ) {}

void CallbacksWidget::setupUi( QWidget* widget )
{
    CallbacksView = widget ? widget : new QWidget();
    CallbacksView->setObjectName( "CallbacksView" );

    auto* root = new QVBoxLayout( CallbacksView );
    root->setContentsMargins( 0, 0, 0, 3 );
    root->setSpacing( 4 );

    auto* filterRow = new QWidget( CallbacksView );
    auto* filterLayout = new QHBoxLayout( filterRow );
    filterLayout->setContentsMargins( 0, 0, 0, 0 );
    filterLayout->setSpacing( 4 );

    SearchBar = new QLineEdit( filterRow );
    SearchBar->setPlaceholderText( "Filter: type:tg  user:root  host:dc  hidden:yes  (space = AND)" );
    SearchBar->setMaximumHeight( 24 );
    SearchBar->setStyleSheet(
        "QLineEdit { background: #12121f; color: #f8f8f2; border: 1px solid #2a2d4a;"
        " border-radius: 3px; padding: 0 6px; font-size: 11px; }"
    );

    FilterCombo = new QComboBox( filterRow );
    FilterCombo->addItem( "All",     "all" );
    FilterCombo->addItem( "Visible", "visible" );
    FilterCombo->addItem( "Hidden",  "hidden" );
    FilterCombo->addItem( "Dead",    "dead" );
    FilterCombo->setMaximumHeight( 24 );
    FilterCombo->setStyleSheet(
        "QComboBox { background: #12121f; color: #f8f8f2; border: 1px solid #2a2d4a;"
        " border-radius: 3px; padding: 0 8px; font-size: 11px; }"
    );

    filterLayout->addWidget( SearchBar, 1 );
    filterLayout->addWidget( FilterCombo );

    Splitter = new QSplitter( Qt::Horizontal, CallbacksView );
    Splitter->setHandleWidth( 3 );
    Splitter->setChildrenCollapsible( false );

    Table = new QTableWidget( Splitter );
    Table->setColumnCount( 10 );
    Table->setHorizontalHeaderLabels( {
        "", "ID", "Alias", "Type", "User", "Host", "Internal", "PID", "Last", ""
    } );
    Table->verticalHeader()->setVisible( false );
    Table->setSelectionBehavior( QAbstractItemView::SelectRows );
    Table->setSelectionMode( QAbstractItemView::SingleSelection );
    Table->setEditTriggers( QAbstractItemView::NoEditTriggers );
    Table->setContextMenuPolicy( Qt::CustomContextMenu );
    Table->setSortingEnabled( false );
    Table->setShowGrid( false );
    Table->setStyleSheet(
        "QTableWidget { background:#0a0a0f; color:#f0f0ee; gridline-color:#2a2a3f; }"
        "QHeaderView::section { background:#111118; color:#8888aa; border:none;"
        " padding:4px 8px; font-size:11px; }"
        "QTableWidget::item:selected { background:#2a2a3f; }"
    );
    auto* header = Table->horizontalHeader();
    header->setDefaultAlignment( Qt::AlignCenter );
    header->setSectionResizeMode( 0, QHeaderView::Fixed );
    header->setSectionResizeMode( 9, QHeaderView::Fixed );
    for ( int c = 1; c <= 8; ++c )
        header->setSectionResizeMode( c, QHeaderView::Stretch );
    Table->setColumnWidth( 0, 36 );
    Table->setColumnWidth( 9, 36 );
    Table->setMinimumWidth( 420 );

    DetailPane = new QWidget( Splitter );
    DetailPane->setObjectName( "CallbackDetail" );
    DetailPane->setMinimumWidth( 300 );
    DetailPane->setMaximumWidth( 440 );
    DetailPane->setStyleSheet(
        "QWidget#CallbackDetail { background:#111118; border-left:1px solid #2a2a3f; }"
    );
    auto* detailRoot = new QVBoxLayout( DetailPane );
    detailRoot->setContentsMargins( 0, 0, 0, 0 );
    detailRoot->setSpacing( 0 );

    auto* detailHead = new QWidget( DetailPane );
    detailHead->setStyleSheet( "background:#111118; border-bottom:1px solid #2a2a3f;" );
    auto* headLay = new QHBoxLayout( detailHead );
    headLay->setContentsMargins( 10, 6, 6, 6 );
    DetailTitle = new QLabel( "Callback", detailHead );
    DetailTitle->setStyleSheet( "color:#ff6b9d; font-weight:bold; font-size:12px;" );
    auto* closeBtn = iconButton( detailHead, iconInfo( true ), "Close" );
    closeBtn->setText( "✕" );
    closeBtn->setIcon( QIcon() );
    closeBtn->setToolTip( "Close" );
    headLay->addWidget( DetailTitle, 1 );
    headLay->addWidget( closeBtn );

    DetailBody = new QScrollArea( DetailPane );
    DetailBody->setWidgetResizable( true );
    DetailBody->setFrameShape( QFrame::NoFrame );
    DetailBody->setStyleSheet( "QScrollArea { background:#111118; border:none; }" );

    detailRoot->addWidget( detailHead );
    detailRoot->addWidget( DetailBody, 1 );

    Splitter->addWidget( Table );
    Splitter->addWidget( DetailPane );
    Splitter->setStretchFactor( 0, 1 );
    Splitter->setStretchFactor( 1, 0 );
    DetailPane->hide();

    root->addWidget( filterRow );
    root->addWidget( Splitter, 1 );

    connect( closeBtn, &QToolButton::clicked, this, [this]() { closeDetails(); } );
    connect( SearchBar, &QLineEdit::textChanged, this, [this]( const QString& ) { rebuildTable(); } );
    connect( FilterCombo, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, [this]( int ) { rebuildTable(); } );

    connect( Table, &QTableWidget::itemSelectionChanged, this, [this]() {
        if ( Rebuilding || ! DetailPane || ! DetailPane->isVisible() )
            return;
        auto* idItem = Table->item( Table->currentRow(), 1 );
        if ( ! idItem )
            return;
        if ( idItem->text() == DetailAgentID )
            return;
        openDetails( idItem->text() );
    } );

    connect( Table, &QTableWidget::doubleClicked, this, [this]( const QModelIndex& index ) {
        if ( index.column() == 0 || index.column() == 9 )
            return;
        auto* idItem = Table->item( index.row(), 1 );
        if ( ! idItem )
            return;
        for ( const auto& s : MugenX::Teamserver.Sessions ) {
            if ( s.Name == idItem->text() && MugenX::Teamserver.TabSession ) {
                MugenX::Teamserver.TabSession->OpenConsoleTab( s );
                break;
            }
        }
    } );

    connect( Table, &QTableWidget::customContextMenuRequested, this, [this]( const QPoint& pos ) {
        auto* idItem = Table->item( Table->indexAt( pos ).row(), 1 );
        if ( ! idItem )
            return;

        auto* session = findSession( idItem->text() );
        if ( ! session )
            return;

        auto menu = QMenu();
        menu.setStyleSheet(
            "QMenu { background-color:#111118; color:#f0f0ee; border:1px solid #2a2a3f; }"
            "QMenu::item:selected { background:#2a2a3f; }"
        );
        menu.addAction( "Interact" );
        menu.addSeparator();
        if ( session->Marked == "Dead" )
            menu.addAction( "Mark as Alive" );
        else
            menu.addAction( "Mark as Dead" );
        menu.addAction( session->Hidden ? "Show" : "Hide" );
        menu.addSeparator();
        menu.addAction( "Details" );

        auto* action = menu.exec( Table->viewport()->mapToGlobal( pos ) );
        if ( ! action )
            return;

        if ( action->text() == "Interact" && MugenX::Teamserver.TabSession )
            MugenX::Teamserver.TabSession->OpenConsoleTab( *session );
        else if ( action->text() == "Hide" )
            sendHidden( session->Name, true );
        else if ( action->text() == "Show" )
            sendHidden( session->Name, false );
        else if ( action->text() == "Mark as Dead" )
            sendMarked( session->Name, "Dead" );
        else if ( action->text() == "Mark as Alive" )
            sendMarked( session->Name, "Alive" );
        else if ( action->text() == "Details" )
            toggleDetails( session->Name );
    } );
}

void CallbacksWidget::Refresh()
{
    rebuildTable();
    if ( ! DetailAgentID.isEmpty() )
    {
        if ( auto* s = findSession( DetailAgentID ) )
            fillDetails( *s );
        else
            closeDetails();
    }
}

Util::SessionItem* CallbacksWidget::findSession( const QString& agentID )
{
    for ( auto& s : MugenX::Teamserver.Sessions )
        if ( s.Name == agentID )
            return &s;
    return nullptr;
}

bool CallbacksWidget::rowVisible( const Util::SessionItem& s, const QString& query, const QString& filter ) const
{
    if ( filter == "hidden" && ! s.Hidden )
        return false;
    if ( filter == "visible" && s.Hidden )
        return false;
    if ( filter == "dead" && s.Marked != "Dead" )
        return false;

    auto q = query.trimmed().toLower();
    if ( q.isEmpty() )
        return true;

    auto typeName = ( s.MagicValue == TenguMagicValue ) ? QString( "tengu" ) : QString( "demon" );
    auto hay = ( s.Name + " " + s.Alias + " " + s.User + " " + s.Computer + " " +
                 s.Internal + " " + s.External + " " + typeName + " " +
                 ( s.Hidden ? "hidden yes" : "hidden no" ) ).toLower();

    const auto tokens = q.split( ' ', Qt::SkipEmptyParts );
    for ( const auto& tok : tokens )
    {
        if ( tok.startsWith( "type:" ) ) {
            if ( ! typeName.startsWith( tok.mid( 5 ) ) )
                return false;
        } else if ( tok.startsWith( "user:" ) ) {
            if ( ! s.User.toLower().contains( tok.mid( 5 ) ) )
                return false;
        } else if ( tok.startsWith( "host:" ) ) {
            if ( ! s.Computer.toLower().contains( tok.mid( 5 ) ) )
                return false;
        } else if ( tok.startsWith( "hidden:" ) ) {
            auto v = tok.mid( 7 );
            bool want = ( v == "yes" || v == "true" || v == "1" );
            if ( s.Hidden != want )
                return false;
        } else if ( ! hay.contains( tok ) ) {
            return false;
        }
    }
    return true;
}

void CallbacksWidget::rebuildTable()
{
    auto query  = SearchBar ? SearchBar->text() : QString();
    auto filter = FilterCombo ? FilterCombo->currentData().toString() : QString( "all" );
    auto keepID = DetailAgentID;

    Rebuilding = true;
    Table->setRowCount( 0 );

    for ( const auto& s : MugenX::Teamserver.Sessions )
    {
        if ( ! rowVisible( s, query, filter ) )
            continue;

        int row = Table->rowCount();
        Table->insertRow( row );
        Table->setRowHeight( row, 28 );

        auto typeName = ( s.MagicValue == TenguMagicValue ) ? QString( "Tengu" ) : QString( "Demon" );
        const auto id = s.Name;

        auto* hideBtn = iconButton( Table, iconEye( s.Hidden ), s.Hidden ? "Show" : "Hide" );
        Table->setCellWidget( row, 0, hideBtn );
        connect( hideBtn, &QToolButton::clicked, this, [this, id, hidden = s.Hidden]() {
            sendHidden( id, ! hidden );
        } );

        Table->setItem( row, 1, cbCell( s.Name ) );
        Table->setItem( row, 2, cbCell( s.Alias ) );
        Table->setItem( row, 3, cbCell( typeName ) );
        Table->setItem( row, 4, cbCell( s.User ) );
        Table->setItem( row, 5, cbCell( s.Computer ) );
        Table->setItem( row, 6, cbCell( s.Internal ) );
        Table->setItem( row, 7, cbCell( s.PID ) );
        Table->setItem( row, 8, cbCell( s.Last ) );

        auto* infoBtn = iconButton( Table, iconInfo( DetailAgentID == s.Name ), "Details" );
        Table->setCellWidget( row, 9, infoBtn );
        connect( infoBtn, &QToolButton::clicked, this, [this, id]() {
            toggleDetails( id );
        } );

        auto comment = QColor( Util::ColorText::Colors::Hex::Comment );
        auto pink    = QColor( Util::ColorText::Colors::Hex::Pink );
        if ( Table->item( row, 2 ) )
            Table->item( row, 2 )->setForeground( pink );
        if ( s.Hidden )
        {
            for ( int c = 1; c <= 8; ++c )
            {
                if ( auto* it = Table->item( row, c ) )
                    it->setForeground( comment );
            }
            if ( Table->item( row, 2 ) && ! s.Alias.isEmpty() )
                Table->item( row, 2 )->setForeground( pink );
        }
        if ( s.Marked == "Dead" )
        {
            auto red = QColor( Util::ColorText::Colors::Hex::Red );
            if ( auto* it = Table->item( row, 1 ) )
                it->setForeground( red );
        }
    }

    if ( ! keepID.isEmpty() )
        selectAgentRow( keepID );
    Rebuilding = false;
}

void CallbacksWidget::updateInfoIcons()
{
    for ( int r = 0; r < Table->rowCount(); ++r )
    {
        auto* btn = qobject_cast<QToolButton*>( Table->cellWidget( r, 9 ) );
        auto* id  = Table->item( r, 1 );
        if ( btn && id )
            btn->setIcon( iconInfo( DetailAgentID == id->text() ) );
    }
}

void CallbacksWidget::selectAgentRow( const QString& agentID )
{
    for ( int r = 0; r < Table->rowCount(); ++r )
    {
        auto* id = Table->item( r, 1 );
        if ( id && id->text() == agentID )
        {
            Table->selectRow( r );
            return;
        }
    }
}

void CallbacksWidget::toggleDetails( const QString& agentID )
{
    if ( DetailAgentID == agentID && DetailPane->isVisible() )
    {
        closeDetails();
        return;
    }
    openDetails( agentID );
}

void CallbacksWidget::openDetails( const QString& agentID )
{
    auto* s = findSession( agentID );
    if ( ! s )
        return;

    DetailAgentID = agentID;
    fillDetails( *s );
    DetailPane->show();
    const int total  = qMax( Splitter->width(), 800 );
    const int detail = qBound( 320, total / 3, 400 );
    Splitter->setSizes( { total - detail, detail } );
    updateInfoIcons();
    if ( ! Rebuilding )
        selectAgentRow( agentID );
}

void CallbacksWidget::closeDetails()
{
    DetailAgentID.clear();
    DetailPane->hide();
    updateInfoIcons();
}

void CallbacksWidget::fillDetails( const Util::SessionItem& s )
{
    auto typeName = ( s.MagicValue == TenguMagicValue ) ? QString( "Tengu" ) : QString( "Demon" );
    auto title = s.Alias.trimmed().isEmpty() ? s.Name : s.Alias.trimmed();
    DetailTitle->setText( title + "  ·  " + typeName );

    auto* page = new QWidget();
    page->setStyleSheet( "background:#111118;" );
    auto* lay = new QVBoxLayout( page );
    lay->setContentsMargins( 12, 4, 12, 16 );
    lay->setSpacing( 0 );

    addSection( lay, "Callback" );
    auto* cb = new QFormLayout();
    cb->setHorizontalSpacing( 16 );
    cb->setVerticalSpacing( 4 );
    cb->setLabelAlignment( Qt::AlignRight | Qt::AlignVCenter );
    addField( cb, "ID",           s.Name );
    addField( cb, "Alias",        s.Alias );
    addField( cb, "Type",         typeName );
    addField( cb, "Visibility",   s.Hidden ? "Hidden" : "Visible" );
    addField( cb, "Health",       s.Health );
    addField( cb, "Elevation",    s.Elevated == "true" ? "High (elevated)" : "User" );
    lay->addLayout( cb );

    addSection( lay, "Host" );
    auto* host = new QFormLayout();
    host->setHorizontalSpacing( 16 );
    host->setVerticalSpacing( 4 );
    host->setLabelAlignment( Qt::AlignRight | Qt::AlignVCenter );
    addField( host, "User",        s.User );
    addField( host, "Host",        s.Computer );
    addField( host, "Domain",      s.Domain );
    addField( host, "Internal IP", s.Internal );
    addField( host, "External IP", s.External );
    addField( host, "OS",          s.OS );
    addField( host, "OS Build",    s.OSBuild );
    addField( host, "OS Arch",     s.OSArch );
    lay->addLayout( host );

    addSection( lay, "Process" );
    auto* proc = new QFormLayout();
    proc->setHorizontalSpacing( 16 );
    proc->setVerticalSpacing( 4 );
    proc->setLabelAlignment( Qt::AlignRight | Qt::AlignVCenter );
    addField( proc, "Name", s.Process );
    addField( proc, "PID",  s.PID );
    addField( proc, "Arch", s.Arch );
    lay->addLayout( proc );

    addSection( lay, "C2" );
    auto* c2 = new QFormLayout();
    c2->setHorizontalSpacing( 16 );
    c2->setVerticalSpacing( 4 );
    c2->setLabelAlignment( Qt::AlignRight | Qt::AlignVCenter );
    addField( c2, "Listener",      s.Listener );
    addField( c2, "First Checkin", s.First );
    addField( c2, "Last Checkin",  s.Last );
    addField( c2, "Sleep",         QString( "%1s  jitter %2%" ).arg( s.SleepDelay ).arg( s.SleepJitter ) );
    addField( c2, "Kill Date",     formatKillDate( s.KillDate ) );
    addField( c2, "Working Hours", formatWorkingHours( s.WorkingHours ) );
    addField( c2, "Pivot Parent",  s.PivotParent );
    lay->addLayout( c2 );

    addSection( lay, "Operator" );
    auto* op = new QFormLayout();
    op->setHorizontalSpacing( 16 );
    op->setVerticalSpacing( 4 );
    op->setLabelAlignment( Qt::AlignRight | Qt::AlignVCenter );
    addField( op, "Tags",  s.Tags );
    addField( op, "Notes", s.Notes );
    addField( op, "Color", s.Color );
    lay->addLayout( op );

    lay->addStretch();
    DetailBody->setWidget( page );
}

void CallbacksWidget::sendHidden( const QString& agentID, bool hidden )
{
    auto Package = new Util::Packager::Package;
    Package->Head = Util::Packager::Head_t {
        .Event = Util::Packager::Session::Type,
        .User  = MugenX::Teamserver.User.toStdString(),
        .Time  = CurrentTime().toStdString(),
    };
    Package->Body.SubEvent = Util::Packager::Session::SetHidden;
    Package->Body.Info[ "AgentID" ] = agentID.toStdString();
    Package->Body.Info[ "Hidden" ]  = hidden ? "true" : "false";
    MugenX::Connector->SendPackage( Package );
}

void CallbacksWidget::sendMarked( const QString& agentID, const QString& marked )
{
    auto Package = new Util::Packager::Package;
    Package->Head = Util::Packager::Head_t {
        .Event = Util::Packager::Session::Type,
        .User  = MugenX::Teamserver.User.toStdString(),
        .Time  = CurrentTime().toStdString(),
    };
    Package->Body.SubEvent = Util::Packager::Session::MarkAs;
    Package->Body.Info[ "AgentID" ] = agentID.toStdString();
    Package->Body.Info[ "Marked" ]  = marked.toStdString();
    MugenX::Connector->SendPackage( Package );
}
