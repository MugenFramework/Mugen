#include <Mugen/Mugen.hpp>
#include <global.hpp>

#include <UserInterface/Widgets/TasksWidget.hpp>
#include <UserInterface/Widgets/TeamserverTabSession.h>
#include <Mugen/Packager.hpp>
#include <Util/ColorText.h>
#include <Util/Base.hpp>

#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QJsonDocument>
#include <QDateTime>

using namespace MugenNamespace::UserInterface::Widgets;

static QTableWidgetItem* cell( const QString& text )
{
    auto* item = new QTableWidgetItem( text );
    item->setFlags( item->flags() & ~Qt::ItemIsEditable );
    return item;
}

static QColor statusColor( const QString& status )
{
    auto s = status.toLower();
    if ( s == "queued" )     return QColor( Util::ColorText::Colors::Hex::Yellow );
    if ( s == "sent" )       return QColor( Util::ColorText::Colors::Hex::Cyan );
    if ( s == "processing" ) return QColor( Util::ColorText::Colors::Hex::Orange );
    if ( s == "error" )      return QColor( Util::ColorText::Colors::Hex::Red );
    return QColor( Util::ColorText::Colors::Hex::Green );
}

static QDateTime parseTaskTime( const QString& ts )
{
    auto dt = QDateTime::fromString( ts, "dd/MM/yyyy HH:mm:ss" );
    dt.setTimeSpec( Qt::UTC );
    return dt;
}

static QString formatDuration( qint64 secs )
{
    if ( secs < 0 ) secs = 0;
    if ( secs < 60 )
        return QString( "%1s" ).arg( secs );
    if ( secs < 3600 )
        return QString( "%1m %2s" ).arg( secs / 60 ).arg( secs % 60 );
    return QString( "%1h %2m" ).arg( secs / 3600 ).arg( ( secs % 3600 ) / 60 );
}

TasksWidget::TasksWidget( QWidget* parent ) : QWidget( parent ) {}

void TasksWidget::setupUi( QWidget* widget )
{
    TasksView = widget ? widget : new QWidget();
    TasksView->setObjectName( "TasksView" );

    auto* root = new QVBoxLayout( TasksView );
    root->setContentsMargins( 0, 0, 0, 3 );
    root->setSpacing( 4 );

    auto* filterRow = new QWidget( TasksView );
    auto* filterLayout = new QHBoxLayout( filterRow );
    filterLayout->setContentsMargins( 0, 0, 0, 0 );
    filterLayout->setSpacing( 4 );

    SearchBar = new QLineEdit( filterRow );
    SearchBar->setPlaceholderText( "Filter: status:queued  agent:TU-  user:alice  type:Tengu  (space = AND)" );
    SearchBar->setMaximumHeight( 24 );
    SearchBar->setStyleSheet(
        "QLineEdit { background: #12121f; color: #f8f8f2; border: 1px solid #2a2d4a;"
        " border-radius: 3px; padding: 0 6px; font-size: 11px; }"
    );

    FilterCombo = new QComboBox( filterRow );
    FilterCombo->addItem( "All",        "all" );
    FilterCombo->addItem( "In progress","active" );
    FilterCombo->addItem( "Completed",  "completed" );
    FilterCombo->addItem( "Error",      "error" );
    FilterCombo->setFixedHeight( 24 );
    FilterCombo->setStyleSheet(
        "QComboBox { background:#1a1a27; color:#f0f0ee; border:1px solid #2a2a3f;"
        " border-radius:3px; padding:0 8px; font-size:11px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background:#111118; color:#f0f0ee; selection-background-color:#2a2a3f; }"
    );

    filterLayout->addWidget( SearchBar, 1 );
    filterLayout->addWidget( FilterCombo );

    TasksTable = new QTableWidget( 0, 8, TasksView );
    TasksTable->setHorizontalHeaderLabels( {
        "STATUS", "AGENT", "ALIAS", "TYPE", "OPERATOR", "COMMAND", "TIME", "DURATION"
    } );
    TasksTable->setShowGrid( false );
    TasksTable->setSortingEnabled( false );
    TasksTable->setSelectionBehavior( QAbstractItemView::SelectRows );
    TasksTable->setSelectionMode( QAbstractItemView::SingleSelection );
    TasksTable->setEditTriggers( QAbstractItemView::NoEditTriggers );
    TasksTable->verticalHeader()->setVisible( false );
    TasksTable->verticalHeader()->setDefaultSectionSize( 26 );
    TasksTable->setAlternatingRowColors( true );
    TasksTable->setFocusPolicy( Qt::NoFocus );
    TasksTable->horizontalHeader()->setSectionResizeMode( 0, QHeaderView::ResizeToContents );
    TasksTable->horizontalHeader()->setSectionResizeMode( 1, QHeaderView::ResizeToContents );
    TasksTable->horizontalHeader()->setSectionResizeMode( 2, QHeaderView::ResizeToContents );
    TasksTable->horizontalHeader()->setSectionResizeMode( 3, QHeaderView::ResizeToContents );
    TasksTable->horizontalHeader()->setSectionResizeMode( 4, QHeaderView::ResizeToContents );
    TasksTable->horizontalHeader()->setSectionResizeMode( 5, QHeaderView::Stretch );
    TasksTable->horizontalHeader()->setSectionResizeMode( 6, QHeaderView::ResizeToContents );
    TasksTable->horizontalHeader()->setSectionResizeMode( 7, QHeaderView::ResizeToContents );

    root->addWidget( filterRow );
    root->addWidget( TasksTable, 1 );

    connect( SearchBar, &QLineEdit::textChanged, this, [this]( const QString& ) { rebuildTable(); } );
    connect( FilterCombo, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, [this]( int ) { rebuildTable(); } );
    connect( TasksTable, &QTableWidget::doubleClicked, this, [this]( const QModelIndex& index ) {
        auto* idItem = TasksTable->item( index.row(), 1 );
        if ( !idItem ) return;
        auto agentID = idItem->data( Qt::UserRole ).toString();
        if ( agentID.isEmpty() ) agentID = idItem->text();
        for ( const auto& s : MugenX::Teamserver.Sessions ) {
            if ( s.Name == agentID && MugenX::Teamserver.TabSession ) {
                MugenX::Teamserver.TabSession->OpenConsoleTab( s );
                return;
            }
        }
    } );
}

void TasksWidget::RequestSnapshot()
{
    Util::Packager::Body_t body;
    body.SubEvent = Util::Packager::TaskHistory::ListAll;
    NewPackageTaskHistory( TeamserverName, body );
}

void TasksWidget::LoadSnapshot( const QString& tasksJson )
{
    auto doc = QJsonDocument::fromJson( tasksJson.toUtf8() );
    m_tasks = doc.array();
    rebuildTable();
}

void TasksWidget::UpsertTask( const QJsonObject& task )
{
    auto id = task["TaskID"].toString();
    if ( id.isEmpty() ) return;

    bool found = false;
    for ( int i = 0; i < m_tasks.size(); i++ ) {
        if ( m_tasks[i].toObject()["TaskID"].toString() == id ) {
            auto merged = m_tasks[i].toObject();
            for ( auto it = task.begin(); it != task.end(); ++it )
                merged.insert( it.key(), it.value() );
            m_tasks[i] = merged;
            found = true;
            break;
        }
    }
    if ( !found )
        m_tasks.prepend( task );

    rebuildTable();
}

void TasksWidget::RefreshDurations()
{
    if ( !TasksTable ) return;
    for ( int row = 0; row < TasksTable->rowCount(); row++ ) {
        auto* idItem = TasksTable->item( row, 1 );
        if ( !idItem ) continue;
        auto taskID = idItem->data( Qt::UserRole + 1 ).toString();
        for ( const auto& val : m_tasks ) {
            auto obj = val.toObject();
            if ( obj["TaskID"].toString() == taskID ) {
                if ( auto* dur = TasksTable->item( row, 7 ) )
                    dur->setText( durationText( obj ) );
                break;
            }
        }
    }
}

QString TasksWidget::durationText( const QJsonObject& task ) const
{
    auto start = parseTaskTime( task["Timestamp"].toString() );
    if ( !start.isValid() ) return {};

    auto status = task["Status"].toString().toLower();
    QDateTime end;
    if ( status == "completed" || status == "error" ) {
        end = parseTaskTime( task["CompletedAt"].toString() );
        if ( !end.isValid() )
            end = start;
    } else {
        end = QDateTime::currentDateTimeUtc();
    }
    return formatDuration( start.secsTo( end ) );
}

bool TasksWidget::rowVisible( const QJsonObject& task, const QString& query, const QString& filter ) const
{
    auto status = task["Status"].toString().toLower();
    if ( status.isEmpty() ) status = "completed";

    if ( filter == "active" && ( status == "completed" || status == "error" ) )
        return false;
    if ( filter == "completed" && status != "completed" )
        return false;
    if ( filter == "error" && status != "error" )
        return false;

    auto tokens = query.trimmed().split( ' ', Qt::SkipEmptyParts );
    if ( tokens.isEmpty() ) return true;

    auto agent  = task["AgentID"].toString().toLower();
    auto alias  = task["Alias"].toString().toLower();
    auto type   = task["AgentType"].toString().toLower();
    auto op     = task["Operator"].toString().toLower();
    auto cmd    = task["CommandLine"].toString().toLower();

    for ( const auto& token : tokens ) {
        bool matched = false;
        if ( token.contains( ':' ) ) {
            int sep = token.indexOf( ':' );
            auto field = token.left( sep ).toLower();
            auto val   = token.mid( sep + 1 ).toLower();
            if ( val.isEmpty() ) { matched = true; }
            else if ( field == "status" ) matched = status.contains( val );
            else if ( field == "agent" || field == "id" ) matched = agent.contains( val );
            else if ( field == "alias" ) matched = alias.contains( val );
            else if ( field == "type" ) matched = type.contains( val );
            else if ( field == "user" || field == "op" || field == "operator" ) matched = op.contains( val );
            else matched = true;
        } else {
            auto v = token.toLower();
            matched = status.contains( v ) || agent.contains( v ) || alias.contains( v )
                   || type.contains( v ) || op.contains( v ) || cmd.contains( v );
        }
        if ( !matched ) return false;
    }
    return true;
}

void TasksWidget::rebuildTable()
{
    if ( !TasksTable ) return;

    auto query  = SearchBar ? SearchBar->text() : QString();
    auto filter = FilterCombo ? FilterCombo->currentData().toString() : QString( "all" );

    TasksTable->setRowCount( 0 );
    for ( const auto& val : m_tasks ) {
        if ( !val.isObject() ) continue;
        auto task = val.toObject();
        if ( !rowVisible( task, query, filter ) ) continue;

        int row = TasksTable->rowCount();
        TasksTable->insertRow( row );

        auto status = task["Status"].toString();
        if ( status.isEmpty() ) status = "completed";

        auto* statusItem = cell( status );
        statusItem->setForeground( statusColor( status ) );
        TasksTable->setItem( row, 0, statusItem );

        auto* agentItem = cell( task["AgentID"].toString() );
        agentItem->setData( Qt::UserRole, task["AgentID"].toString() );
        agentItem->setData( Qt::UserRole + 1, task["TaskID"].toString() );
        TasksTable->setItem( row, 1, agentItem );

        auto* aliasItem = cell( task["Alias"].toString() );
        aliasItem->setForeground( QColor( 0xff, 0x6b, 0x9d ) );
        TasksTable->setItem( row, 2, aliasItem );

        TasksTable->setItem( row, 3, cell( task["AgentType"].toString() ) );
        TasksTable->setItem( row, 4, cell( task["Operator"].toString() ) );
        TasksTable->setItem( row, 5, cell( task["CommandLine"].toString() ) );
        TasksTable->setItem( row, 6, cell( task["Timestamp"].toString() ) );
        TasksTable->setItem( row, 7, cell( durationText( task ) ) );
    }
}
