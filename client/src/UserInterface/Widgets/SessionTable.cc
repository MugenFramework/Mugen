#include <Mugen/Mugen.hpp>
#include <global.hpp>

#include <UserInterface/Widgets/SessionTable.hpp>
#include <UserInterface/Widgets/TeamserverTabSession.h>
#include <UserInterface/SmallWidgets/EventViewer.hpp>
#include <UserInterface/Widgets/DemonInteracted.h>

#include <QHeaderView>
#include <QItemSelectionModel>
#include <QWidgetAction>
#include <QCheckBox>
#include <QDialog>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QLabel>
#include <QInputDialog>
#include <QStyledItemDelegate>
#include <QPainter>
#include <Util/ColorText.h>

using namespace MugenNamespace::UserInterface::Widgets;
using namespace MugenNamespace::Util;

static QColor sessionAccentFromName( const QString& name )
{
    if ( name.compare( "Red",    Qt::CaseInsensitive ) == 0 ) return QColor( ColorText::Colors::Hex::Red );
    if ( name.compare( "Blue",   Qt::CaseInsensitive ) == 0 ) return QColor( ColorText::Colors::Hex::Cyan );
    if ( name.compare( "Pink",   Qt::CaseInsensitive ) == 0 ) return QColor( ColorText::Colors::Hex::Pink );
    if ( name.compare( "Yellow", Qt::CaseInsensitive ) == 0 ) return QColor( ColorText::Colors::Hex::Yellow );
    if ( name.compare( "Green",  Qt::CaseInsensitive ) == 0 ) return QColor( ColorText::Colors::Hex::Green );
    if ( name.compare( "Purple", Qt::CaseInsensitive ) == 0 ) return QColor( ColorText::Colors::Hex::Purple );
    if ( name.compare( "Orange", Qt::CaseInsensitive ) == 0 ) return QColor( ColorText::Colors::Hex::Orange );
    return QColor();
}

/* QTableWidget::item in Mugen.qss swallows QTableWidgetItem::setBackground.
 * Paint a translucent wash + left accent on top of the styled item instead. */
class SessionColorDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const override
    {
        QStyledItemDelegate::paint( painter, option, index );

        const QColor accent = index.model()->index( index.row(), 0 ).data( Qt::UserRole ).value<QColor>();
        if ( ! accent.isValid() )
            return;

        painter->save();
        QColor wash = accent;
        wash.setAlpha( 38 );
        painter->fillRect( option.rect, wash );
        if ( index.column() == 0 )
            painter->fillRect( QRect( option.rect.x(), option.rect.y(), 3, option.rect.height() ), accent );
        painter->restore();
    }
};

void MugenNamespace::UserInterface::Widgets::SessionTable::setupUi(QWidget *Form, QString TeamserverName)
{
    this->TeamserverName = TeamserverName;

    gridLayout = new QGridLayout( this );
    gridLayout->setObjectName( QString::fromUtf8( "gridLayout" ) );
    gridLayout->setContentsMargins( 0, 0, 0, 0 );
    gridLayout->setSpacing( 2 );
    SessionTableWidget = new QTableWidget( this );

    if ( SessionTableWidget->columnCount() < 12 )
        SessionTableWidget->setColumnCount( 12 );

    TitleAgentID   = new QTableWidgetItem( "ID"       );
    TitleExternal  = new QTableWidgetItem( "EXTERNAL" );
    TitleInternal  = new QTableWidgetItem( "INTERNAL" );
    TitleUser      = new QTableWidgetItem( "USER"     );
    TitleComputer  = new QTableWidgetItem( "COMPUTER" );
    TitleOperating = new QTableWidgetItem( "OS"       );
    TitleProcess   = new QTableWidgetItem( "PROCESS"  );
    TitleProcessId = new QTableWidgetItem( "PID"      );
    TitleLast      = new QTableWidgetItem( "LAST"     );
    TitleHealth    = new QTableWidgetItem( "HEALTH"   );
    TitleTags      = new QTableWidgetItem( "TAGS"     );
    TitleAlias     = new QTableWidgetItem( "ALIAS"    );

    SessionTableWidget->setHorizontalHeaderItem( 0,  TitleAgentID   );
    SessionTableWidget->setHorizontalHeaderItem( 1,  TitleExternal  );
    SessionTableWidget->setHorizontalHeaderItem( 2,  TitleInternal  );
    SessionTableWidget->setHorizontalHeaderItem( 3,  TitleUser      );
    SessionTableWidget->setHorizontalHeaderItem( 4,  TitleComputer  );
    SessionTableWidget->setHorizontalHeaderItem( 5,  TitleOperating );
    SessionTableWidget->setHorizontalHeaderItem( 6,  TitleProcess   );
    SessionTableWidget->setHorizontalHeaderItem( 7,  TitleProcessId );
    SessionTableWidget->setHorizontalHeaderItem( 8,  TitleLast      );
    SessionTableWidget->setHorizontalHeaderItem( 9,  TitleHealth    );
    SessionTableWidget->setHorizontalHeaderItem( 10, TitleTags      );
    SessionTableWidget->setHorizontalHeaderItem( 11, TitleAlias     );
    SessionTableWidget->horizontalHeader()->resizeSection( 5, 150 );

    SessionTableWidget->setEnabled( true );
    SessionTableWidget->setShowGrid( false );
    SessionTableWidget->setSortingEnabled( false );
    SessionTableWidget->setWordWrap( true );
    SessionTableWidget->setCornerButtonEnabled( true );
    SessionTableWidget->horizontalHeader()->setVisible( true );
    SessionTableWidget->setSelectionBehavior( QAbstractItemView::SelectRows );
    SessionTableWidget->setSelectionMode( QAbstractItemView::ExtendedSelection );
    SessionTableWidget->setContextMenuPolicy( Qt::CustomContextMenu );
    SessionTableWidget->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeMode::Stretch );
    SessionTableWidget->horizontalHeader()->setStretchLastSection( true );
    SessionTableWidget->verticalHeader()->setVisible( false );
    SessionTableWidget->verticalHeader()->setDefaultSectionSize( 28 );
    SessionTableWidget->setFocusPolicy( Qt::NoFocus );
    SessionTableWidget->setAlternatingRowColors( true );
    SessionTableWidget->setItemDelegate( new SessionColorDelegate( SessionTableWidget ) );

    SessionTableWidget->horizontalHeaderItem( 0 )->setSizeHint( QSize( 0, 0 ) );

    SessionTableWidget->horizontalHeader()->setSectionsMovable( true );
    SessionTableWidget->horizontalHeader()->setContextMenuPolicy( Qt::CustomContextMenu );

    restoreColumnLayout();

    auto colMenuStyle = QString(
        "QMenu { background:#111118; color:#f0f0ee; border:1px solid #2a2a3f; padding:2px; }"
        "QCheckBox { color:#f0f0ee; background:transparent; padding:3px 8px; spacing:6px; }"
        "QCheckBox:hover { color:#ff6b9d; }"
        "QCheckBox::indicator { width:13px; height:13px; border-radius:2px; }"
        "QCheckBox::indicator:checked { background:#ff6b9d; border:1px solid #ff6b9d; }"
        "QCheckBox::indicator:unchecked { background:#1a1a27; border:1px solid #44475a; }"
    );

    connect( SessionTableWidget->horizontalHeader(), &QHeaderView::customContextMenuRequested,
             this, [this, colMenuStyle]( const QPoint& pos ) {
        auto* menu = new QMenu( SessionTableWidget );
        menu->setStyleSheet( colMenuStyle );
        for ( int i = 0; i < SessionTableWidget->columnCount(); i++ ) {
            auto* wa = new QWidgetAction( menu );
            auto* cb = new QCheckBox( SessionTableWidget->horizontalHeaderItem( i )->text(), menu );
            cb->setChecked( !SessionTableWidget->isColumnHidden( i ) );
            wa->setDefaultWidget( cb );
            menu->addAction( wa );
            connect( cb, &QCheckBox::toggled, this, [this, i]( bool checked ) {
                SessionTableWidget->setColumnHidden( i, !checked );
                saveColumnLayout();
            } );
        }
        menu->exec( SessionTableWidget->horizontalHeader()->mapToGlobal( pos ) );
        menu->deleteLater();
    } );

    connect( SessionTableWidget->horizontalHeader(), &QHeaderView::sectionMoved,
             this, [this]( int, int, int ) { saveColumnLayout(); } );

    connect( SessionTableWidget, &QTableWidget::itemSelectionChanged, this, &MugenNamespace::UserInterface::Widgets::SessionTable::updateRow );

    // Filter bar + Actions button
    auto* filterRow = new QWidget( this );
    auto* filterLayout = new QHBoxLayout( filterRow );
    filterLayout->setContentsMargins( 0, 0, 0, 0 );
    filterLayout->setSpacing( 4 );

    FilterInput = new QLineEdit( filterRow );
    FilterInput->setPlaceholderText( "Filter: type:tg  health:healthy  user:root  listener:C2  ip:10.0  (space = AND)" );
    FilterInput->setObjectName( "FilterInput" );
    FilterInput->setMaximumHeight( 24 );
    FilterInput->setStyleSheet( "QLineEdit { background: #12121f; color: #f8f8f2; border: 1px solid #2a2d4a; border-radius: 3px; padding: 0 6px; font-size: 11px; }" );
    connect( FilterInput, &QLineEdit::textChanged, this, &MugenNamespace::UserInterface::Widgets::SessionTable::applyFilter );

    ActionsButton = new QPushButton( "Actions  ▾", filterRow );
    ActionsButton->setFixedSize( 90, 24 );
    ActionsButton->setStyleSheet(
        "QPushButton {"
        "    background:#1a1a27; color:#f0f0ee; border:1px solid #2a2a3f;"
        "    border-radius:3px; font-size:11px; padding:0 6px;"
        "}"
        "QPushButton:hover { background:#2a2a3f; border-color:#ff6b9d; color:#ff6b9d; }"
        "QPushButton:pressed { background:#ff6b9d22; }"
    );

    filterLayout->addWidget( FilterInput );
    filterLayout->addWidget( ActionsButton );

    auto menuStyle = QString(
        "QMenu { background:#111118; color:#f0f0ee; border:1px solid #2a2a3f; }"
        "QMenu::separator { background:#2a2a3f; }"
        "QMenu::item:selected { background:#2a2a3f; }"
        "QMenu::item:disabled { color:#44475a; }"
    );

    connect( ActionsButton, &QPushButton::clicked, this, [this, menuStyle]() {
        auto* menu = new QMenu( ActionsButton );
        menu->setStyleSheet( menuStyle );

        // ── Beacon Builder ────────────────────────────────────────────────
        auto* builderAct = menu->addAction( "Beacon Builder" );
        connect( builderAct, &QAction::triggered, this, []() {
            if ( MugenX::Teamserver.TabSession )
                MugenX::Teamserver.TabSession->OpenPayloadBuilder();
        } );

        // ── Bulk Dispatch ─────────────────────────────────────────────────
        auto* bulkAct = menu->addAction( "Bulk Dispatch" );
        connect( bulkAct, &QAction::triggered, this, [this]() {
            QVector<Util::SessionItem> liveSessions;
            for ( const auto& s : MugenX::Teamserver.Sessions ) {
                if ( s.Marked != "Dead" ) liveSessions << s;
            }
            if ( liveSessions.isEmpty() ) return;

            auto dlgStyle = QString(
                "QDialog { background:#0a0a0f; color:#f0f0ee; }"
                "QLabel { color:#f0f0ee; font-size:11px; }"
                "QCheckBox { color:#f0f0ee; font-size:11px; spacing:6px; }"
                "QCheckBox::indicator { width:13px; height:13px; border-radius:2px; }"
                "QCheckBox::indicator:checked { background:#ff6b9d; border:1px solid #ff6b9d; }"
                "QCheckBox::indicator:unchecked { background:#1a1a27; border:1px solid #44475a; }"
                "QLineEdit { background:#12121f; color:#f8f8f2; border:1px solid #2a2d4a; border-radius:3px; padding:2px 6px; font-size:11px; }"
                "QPushButton { background:#1a1a27; color:#f0f0ee; border:1px solid #2a2a3f; border-radius:3px; font-size:11px; padding:3px 14px; }"
                "QPushButton:hover { background:#2a2a3f; border-color:#ff6b9d; color:#ff6b9d; }"
                "QPushButton#execBtn { background:#ff6b9d22; border-color:#ff6b9d; color:#ff6b9d; }"
                "QPushButton#execBtn:hover { background:#ff6b9d44; }"
            );

            auto* dlg = new QDialog( this->window() );
            dlg->setWindowTitle( "Bulk Dispatch" );
            dlg->setMinimumWidth( 420 );
            dlg->setStyleSheet( dlgStyle );

            auto* layout = new QVBoxLayout( dlg );
            layout->setSpacing( 8 );
            layout->setContentsMargins( 14, 14, 14, 14 );

            auto* headerRow = new QHBoxLayout();
            auto* selectAll = new QCheckBox( "Select all", dlg );
            selectAll->setChecked( true );
            headerRow->addWidget( selectAll );
            headerRow->addStretch();
            layout->addLayout( headerRow );

            auto* scroll     = new QScrollArea( dlg );
            auto* listWgt    = new QWidget();
            auto* listLayout = new QVBoxLayout( listWgt );
            listLayout->setSpacing( 2 );
            listLayout->setContentsMargins( 0, 0, 0, 0 );
            scroll->setWidget( listWgt );
            scroll->setWidgetResizable( true );
            scroll->setMaximumHeight( 180 );
            scroll->setStyleSheet( "QScrollArea { border:1px solid #2a2a3f; background:#0d0d14; }" );

            QVector<QCheckBox*> boxes;
            for ( const auto& s : liveSessions ) {
                auto label = QString( "[%1]  %2 @ %3" ).arg( s.Name ).arg( s.User ).arg( s.Computer );
                auto* cb   = new QCheckBox( label, listWgt );
                cb->setChecked( true );
                listLayout->addWidget( cb );
                boxes << cb;
            }
            layout->addWidget( scroll );

            connect( selectAll, &QCheckBox::toggled, dlg, [&boxes]( bool checked ) {
                for ( auto* cb : boxes ) cb->setChecked( checked );
            } );

            layout->addWidget( new QLabel( "Command:", dlg ) );
            auto* cmdInput = new QLineEdit( dlg );
            cmdInput->setPlaceholderText( "shell whoami" );
            layout->addWidget( cmdInput );

            auto* btnRow    = new QHBoxLayout();
            auto* execBtn   = new QPushButton( "Execute", dlg );
            auto* cancelBtn = new QPushButton( "Cancel", dlg );
            execBtn->setObjectName( "execBtn" );
            btnRow->addStretch();
            btnRow->addWidget( cancelBtn );
            btnRow->addWidget( execBtn );
            layout->addLayout( btnRow );

            connect( cancelBtn, &QPushButton::clicked, dlg, &QDialog::reject );
            connect( execBtn, &QPushButton::clicked, dlg, [&]() {
                auto cmd = cmdInput->text().trimmed();
                if ( cmd.isEmpty() ) return;
                for ( int i = 0; i < boxes.size(); i++ ) {
                    if ( !boxes[i]->isChecked() ) continue;
                    auto sid = liveSessions[i].Name;
                    for ( auto& s : MugenX::Teamserver.Sessions ) {
                        if ( s.Name != sid ) continue;
                        if ( s.InteractedWidget == nullptr ) {
                            s.InteractedWidget                 = new UserInterface::Widgets::DemonInteracted;
                            s.InteractedWidget->SessionInfo    = s;
                            s.InteractedWidget->TeamserverName = this->TeamserverName;
                            s.InteractedWidget->setupUi( new QWidget );
                        }
                        s.InteractedWidget->AppendText( cmd );
                        break;
                    }
                }
                dlg->accept();
            } );

            dlg->exec();
            delete dlg;
        } );

        menu->addSeparator();

        // ── Process List submenu ──────────────────────────────────────────
        auto* procMenu = menu->addMenu( "Process List" );
        procMenu->setStyleSheet( menuStyle );
        bool anyLive = false;
        for ( auto& s : MugenX::Teamserver.Sessions ) {
            if ( s.Marked == "Dead" ) continue;
            anyLive = true;
            auto label = QString( "[%1]  %2 @ %3" ).arg( s.Name ).arg( s.User ).arg( s.Computer );
            auto* act = procMenu->addAction( label );
            QString sid = s.Name;
            connect( act, &QAction::triggered, this, [sid]() {
                if ( MugenX::Teamserver.TabSession )
                    MugenX::Teamserver.TabSession->OpenProcessList( sid );
            } );
        }
        if ( ! anyLive ) procMenu->setEnabled( false );

        // ── File Explorer submenu ─────────────────────────────────────────
        auto* fileMenu = menu->addMenu( "File Explorer" );
        fileMenu->setStyleSheet( menuStyle );
        anyLive = false;
        for ( auto& s : MugenX::Teamserver.Sessions ) {
            if ( s.Marked == "Dead" ) continue;
            anyLive = true;
            auto label = QString( "[%1]  %2 @ %3" ).arg( s.Name ).arg( s.User ).arg( s.Computer );
            auto* act = fileMenu->addAction( label );
            QString sid = s.Name;
            connect( act, &QAction::triggered, this, [sid]() {
                if ( MugenX::Teamserver.TabSession )
                    MugenX::Teamserver.TabSession->OpenFileBrowser( sid );
            } );
        }
        if ( ! anyLive ) fileMenu->setEnabled( false );

        menu->exec( ActionsButton->mapToGlobal( QPoint( 0, ActionsButton->height() ) ) );
        menu->deleteLater();
    } );

    gridLayout->addWidget( filterRow,           0, 0, 1, 1 );
    gridLayout->addWidget( SessionTableWidget,  1, 0, 1, 1 );
    gridLayout->setRowStretch( 1, 1 );

    QMetaObject::connectSlotsByName( Form );
}

void MugenNamespace::UserInterface::Widgets::SessionTable::NewSessionItem( Util::SessionItem item ) const
{
    /* check if the session already exists */
    for ( auto& session : MugenX::Teamserver.Sessions ) {
        if ( session.Name.compare( item.Name ) == 0 ) {
            return;
        }
    }

    MugenX::Teamserver.Sessions.push_back( item );

    if ( SessionTableWidget->rowCount() < 1 ) {
        SessionTableWidget->setRowCount( 1 );
    } else {
        SessionTableWidget->setRowCount( SessionTableWidget->rowCount() + 1 );
    }

    auto isSortingEnabled = SessionTableWidget->isSortingEnabled();

    SessionTableWidget->setSortingEnabled( false );

    auto item_ID        = new QTableWidgetItem();
    auto item_External  = new QTableWidgetItem();
    auto item_Internal  = new QTableWidgetItem();
    auto item_User      = new QTableWidgetItem();
    auto item_Computer  = new QTableWidgetItem();
    auto item_OS        = new QTableWidgetItem();
    auto item_Process   = new QTableWidgetItem();
    auto item_ProcessID = new QTableWidgetItem();
    auto item_Last      = new QTableWidgetItem();
    auto item_Health    = new QTableWidgetItem();
    auto Icon           = QIcon();

    if ( item.Elevated.compare( "true" ) == 0 ) {
        item_ID->setForeground( QColor( 255, 85, 85 ) );
        Icon = WinVersionIcon( item.OS, true );
    } else {
        Icon = WinVersionIcon( item.OS, false );
    }

    item_ID->setText( item.Name );
    item_ID->setIcon( Icon );
    item_ID->setTextAlignment( Qt::AlignCenter );
    item_ID->setFlags( item_ID->flags() ^ Qt::ItemIsEditable );
    SessionTableWidget->setItem( SessionTableWidget->rowCount() - 1, 0, item_ID );

    item_External->setText( item.External );
    item_External->setTextAlignment( Qt::AlignCenter );
    item_External->setFlags( item_External->flags() ^ Qt::ItemIsEditable );
    SessionTableWidget->setItem( SessionTableWidget->rowCount()-1, 1, item_External );

    item_Internal->setText( item.Internal );
    item_Internal->setTextAlignment( Qt::AlignCenter );
    item_Internal->setFlags( item_Internal->flags() ^ Qt::ItemIsEditable );
    SessionTableWidget->setItem( SessionTableWidget->rowCount()-1, 2, item_Internal );

    item_User->setText( item.User );
    item_User->setTextAlignment( Qt::AlignCenter );
    item_User->setFlags( item_User->flags() ^ Qt::ItemIsEditable );
    SessionTableWidget->setItem( SessionTableWidget->rowCount()-1, 3, item_User );

    item_Computer->setText( item.Computer );
    item_Computer->setTextAlignment( Qt::AlignCenter );
    item_Computer->setFlags( item_Computer->flags() ^ Qt::ItemIsEditable );
    SessionTableWidget->setItem( SessionTableWidget->rowCount()-1, 4, item_Computer );

    item_OS->setText( item.OS );
    item_OS->setTextAlignment( Qt::AlignCenter );
    item_OS->setFlags( item_OS->flags() ^ Qt::ItemIsEditable );
    SessionTableWidget->setItem( SessionTableWidget->rowCount()-1, 5, item_OS );

    item_Process->setText( item.Process );
    item_Process->setTextAlignment( Qt::AlignCenter );
    item_Process->setFlags( item_Process->flags() ^ Qt::ItemIsEditable );
    SessionTableWidget->setItem( SessionTableWidget->rowCount()-1, 6, item_Process );

    item_ProcessID->setText( item.PID );
    item_ProcessID->setTextAlignment( Qt::AlignCenter );
    item_ProcessID->setFlags( item_ProcessID->flags() ^ Qt::ItemIsEditable );
    SessionTableWidget->setItem( SessionTableWidget->rowCount()-1, 7, item_ProcessID );

    item_Last->setText( item.Last );
    item_Last->setTextAlignment( Qt::AlignCenter );
    item_Last->setFlags( item_Last->flags() ^ Qt::ItemIsEditable );
    SessionTableWidget->setItem( SessionTableWidget->rowCount()-1, 8, item_Last );

    item_Health->setText( item.Health == "healthy" ? "● live" : "● dead" );
    item_Health->setTextAlignment( Qt::AlignCenter );
    item_Health->setFlags( item_Health->flags() ^ Qt::ItemIsEditable );
    item_Health->setForeground( item.Health == "healthy" ? QColor( 0x50, 0xfa, 0x7b ) : QColor( 0xff, 0x55, 0x55 ) );
    SessionTableWidget->setItem( SessionTableWidget->rowCount()-1, 9, item_Health );

    auto item_Tags = new QTableWidgetItem( item.Tags );
    item_Tags->setTextAlignment( Qt::AlignCenter );
    item_Tags->setFlags( item_Tags->flags() ^ Qt::ItemIsEditable );
    item_Tags->setForeground( QColor( 0x50, 0xfa, 0x7b ) ); // green accent for tags
    SessionTableWidget->setItem( SessionTableWidget->rowCount()-1, 10, item_Tags );

    auto item_Alias = new QTableWidgetItem( item.Alias );
    item_Alias->setTextAlignment( Qt::AlignCenter );
    item_Alias->setFlags( item_Alias->flags() ^ Qt::ItemIsEditable );
    item_Alias->setForeground( QColor( 0xff, 0x6b, 0x9d ) ); // sakura accent for the alias
    SessionTableWidget->setItem( SessionTableWidget->rowCount()-1, 11, item_Alias );

    if ( ! item.Color.isEmpty() ) {
        auto* idItem = SessionTableWidget->item( SessionTableWidget->rowCount() - 1, 0 );
        QColor accent = sessionAccentFromName( item.Color );
        if ( idItem && accent.isValid() )
            idItem->setData( Qt::UserRole, accent );
    }

    SessionTableWidget->setSortingEnabled( isSortingEnabled );

    for ( auto & Session : MugenX::Teamserver.Sessions )
    {
        // TODO: make that on Session receive
        if ( Session.InteractedWidget == nullptr )
        {
            auto AgentMessageInfo = QString();
            auto prev_cursor      = QTextCursor();
            auto PivotStream      = QString();

            Session.InteractedWidget                 = new UserInterface::Widgets::DemonInteracted;
            Session.InteractedWidget->SessionInfo    = Session;
            Session.InteractedWidget->TeamserverName = this->TeamserverName;
            Session.InteractedWidget->setupUi( new QWidget );

            if ( item.PivotParent.size() > 0 ) {
                PivotStream = "[Pivot: " + item.PivotParent + Util::ColorText::Cyan( "-<>-<>-" ) + item.Name + "]";
                MugenX::Teamserver.TabSession->SessionGraphWidget->GraphPivotNodeAdd( item.PivotParent, item );
            } else {
                PivotStream = "[Pivot: "+ Util::ColorText::Cyan( "Direct" ) +"]";
                MugenX::Teamserver.TabSession->SessionGraphWidget->GraphNodeAdd( item );
            }

            AgentMessageInfo =
                    Util::ColorText::Comment( item.First ) + " Agent " + Util::ColorText::Red( item.Name.toUpper() ) + " authenticated as "+ Util::ColorText::Purple( item.Computer + "\\" + item.User ) +
                    " :: [Internal: " + Util::ColorText::Cyan( item.Internal ) + "] [Process: " + Util::ColorText::Red( item.Process + "\\" + item.PID ) + "] [Arch: " + Util::ColorText::Pink( item.Arch ) + "] " + PivotStream;

            prev_cursor = Session.InteractedWidget->Console->textCursor();

            Session.InteractedWidget->ConsoleInitialMessage = AgentMessageInfo;

            Session.InteractedWidget->Console->moveCursor( QTextCursor::End );
            Session.InteractedWidget->Console->insertHtml( AgentMessageInfo );

            Session.InteractedWidget->Console->setTextCursor( prev_cursor );
        }
    }
}

void UserInterface::Widgets::SessionTable::SetHealthCell( const QString& agentId, const QString& text, const QColor& color )
{
    for ( int i = 0; i < SessionTableWidget->rowCount(); i++ ) {
        if ( SessionTableWidget->item( i, 0 )->text() == agentId ) {
            auto cell = SessionTableWidget->item( i, 9 );
            cell->setText( text );
            cell->setForeground( color );
            return;
        }
    }
}

void UserInterface::Widgets::SessionTable::ChangeSessionValue( QString DemonID, int key, QString value )
{
    for ( int i = 0; i < SessionTableWidget->rowCount(); i++ ) {
        if ( SessionTableWidget->item( i, 0 )->text() == DemonID ) {
            auto cell = SessionTableWidget->item( i, key );
            if ( key == 9 ) {
                cell->setText( value == "healthy" ? "● live" : "● dead" );
                cell->setForeground( value == "healthy" ? QColor( 0x50, 0xfa, 0x7b ) : QColor( 0xff, 0x55, 0x55 ) );
            } else {
                cell->setText( value );
            }
        }
    }
}

void MugenNamespace::UserInterface::Widgets::SessionTable::updateRow()
{
    bool selected = false;

    for ( int count = 0; count < SessionTableWidget->rowCount(); count++ )
        if ( SessionTableWidget->item( count, 0 )->isSelected() )
            selected = true;

    if ( ! selected )
        SessionTableWidget->clearFocus();
}

void MugenNamespace::UserInterface::Widgets::SessionTable::SetSessionTags( const QString& AgentID, const QString& Tags )
{
    for ( auto& s : MugenX::Teamserver.Sessions ) {
        if ( s.Name == AgentID ) {
            s.Tags = Tags;
            break;
        }
    }

    for ( int i = 0; i < SessionTableWidget->rowCount(); i++ ) {
        if ( SessionTableWidget->item( i, 0 )->text() == AgentID ) {
            auto item = SessionTableWidget->item( i, 10 );
            if ( item ) {
                item->setText( Tags );
            }
            break;
        }
    }
}

void MugenNamespace::UserInterface::Widgets::SessionTable::SetSessionAlias( const QString& AgentID, const QString& Alias )
{
    for ( auto& s : MugenX::Teamserver.Sessions ) {
        if ( s.Name == AgentID ) {
            s.Alias = Alias;
            if ( s.InteractedWidget )
                s.InteractedWidget->SessionInfo.Alias = Alias;
            if ( MugenX::Teamserver.TabSession )
                MugenX::Teamserver.TabSession->UpdateConsoleTabTitle( s );
            break;
        }
    }

    for ( int i = 0; i < SessionTableWidget->rowCount(); i++ ) {
        if ( SessionTableWidget->item( i, 0 )->text() == AgentID ) {
            auto item = SessionTableWidget->item( i, 11 );
            if ( item ) {
                item->setText( Alias );
            }
            break;
        }
    }
}

void MugenNamespace::UserInterface::Widgets::SessionTable::SetSessionMeta( const QString& AgentID, const QString& Tags, const QString& Notes )
{
    for ( auto& s : MugenX::Teamserver.Sessions ) {
        if ( s.Name == AgentID ) {
            s.Tags  = Tags;
            s.Notes = Notes;
            if ( s.InteractedWidget ) {
                s.InteractedWidget->SessionInfo.Tags  = Tags;
                s.InteractedWidget->SessionInfo.Notes = Notes;
            }
            break;
        }
    }

    SetSessionTags( AgentID, Tags );
}

void MugenNamespace::UserInterface::Widgets::SessionTable::SetSessionColor( const QString& AgentID, const QString& Color )
{
    for ( auto& s : MugenX::Teamserver.Sessions ) {
        if ( s.Name == AgentID ) {
            s.Color = Color;
            if ( s.InteractedWidget )
                s.InteractedWidget->SessionInfo.Color = Color;
            break;
        }
    }

    for ( int i = 0; i < SessionTableWidget->rowCount(); i++ ) {
        if ( SessionTableWidget->item( i, 0 )->text() == AgentID ) {
            auto* idItem = SessionTableWidget->item( i, 0 );
            QColor accent = sessionAccentFromName( Color );
            if ( accent.isValid() )
                idItem->setData( Qt::UserRole, accent );
            else
                idItem->setData( Qt::UserRole, QVariant() );
            SessionTableWidget->viewport()->update();
            break;
        }
    }
}

// Query language: space-separated tokens, each "field:value" or plain text.
// Fields: type, health, user, listener, ip, id, alias, tag, notes, os, proc, computer
// All tokens must match (implicit AND).
void MugenNamespace::UserInterface::Widgets::SessionTable::applyFilter( const QString& query )
{
    auto tokens = query.trimmed().split( ' ', Qt::SkipEmptyParts );

    for ( int row = 0; row < SessionTableWidget->rowCount(); row++ )
    {
        auto getCol = [&]( int c ) -> QString {
            auto* it = SessionTableWidget->item( row, c );
            return it ? it->text().toLower() : QString();
        };

        QString listener, agentType, notes;
        auto id = getCol(0);
        for ( const auto& s : MugenX::Teamserver.Sessions ) {
            if ( s.Name.toLower() == id ) {
                listener  = s.Listener.toLower();
                notes     = s.Notes.toLower();
                agentType = s.Name.startsWith("TU-", Qt::CaseInsensitive) ? "tu" : "dn";
                break;
            }
        }

        bool visible = true;
        for ( const auto& token : tokens )
        {
            if ( token.isEmpty() ) continue;
            bool matched = false;
            if ( token.contains(':') )
            {
                int    sep   = token.indexOf(':');
                auto   field = token.left(sep).toLower();
                auto   val   = token.mid(sep + 1).toLower();
                if ( val.isEmpty() ) { matched = true; continue; }

                if      ( field == "type"     ) matched = agentType.contains(val);
                else if ( field == "health"   ) matched = getCol(9).contains(val);
                else if ( field == "user"     ) matched = getCol(3).contains(val);
                else if ( field == "listener" ) matched = listener.contains(val);
                else if ( field == "ip"       ) matched = getCol(1).contains(val) || getCol(2).contains(val);
                else if ( field == "id"       ) matched = getCol(0).contains(val);
                else if ( field == "alias"    ) matched = getCol(11).contains(val);
                else if ( field == "tag" || field == "tags" ) matched = getCol(10).contains(val);
                else if ( field == "note" || field == "notes" ) matched = notes.contains(val);
                else if ( field == "os"       ) matched = getCol(5).contains(val);
                else if ( field == "proc"     ) matched = getCol(6).contains(val);
                else if ( field == "computer" ) matched = getCol(4).contains(val);
                else matched = true;
            }
            else
            {
                auto v = token.toLower();
                for ( int c = 0; c < SessionTableWidget->columnCount(); c++ )
                    if ( getCol(c).contains(v) ) { matched = true; break; }
                if ( !matched ) matched = listener.contains(v) || agentType.contains(v) || notes.contains(v);
            }

            if ( !matched ) { visible = false; break; }
        }

        SessionTableWidget->setRowHidden( row, !visible );
    }
}

// The key is versioned: a state saved by an older client has a different
// column count and would be rejected by restoreState() anyway.
static const char* SessionTableHeaderKey = "SessionTable/headerStateV2";

void MugenNamespace::UserInterface::Widgets::SessionTable::saveColumnLayout()
{
    QSettings s( "MugenFramework", "Mugen" );
    s.setValue( SessionTableHeaderKey, SessionTableWidget->horizontalHeader()->saveState() );
}

void MugenNamespace::UserInterface::Widgets::SessionTable::restoreColumnLayout()
{
    QSettings s( "MugenFramework", "Mugen" );
    auto  state  = s.value( SessionTableHeaderKey ).toByteArray();
    auto* header = SessionTableWidget->horizontalHeader();

    if ( ! state.isEmpty() && header->restoreState( state ) )
        return;

    // no usable layout yet: ALIAS is the last logical column, show it right
    // after ID so both identifiers of an agent sit next to each other
    header->moveSection( header->visualIndex( 11 ), 1 );
}

