#include <Mugen/Mugen.hpp>
#include <global.hpp>

#include <UserInterface/Widgets/SessionTable.hpp>
#include <UserInterface/Widgets/TeamserverTabSession.h>
#include <UserInterface/SmallWidgets/EventViewer.hpp>
#include <UserInterface/Widgets/DemonInteracted.h>

#include <QHeaderView>
#include <QItemSelectionModel>
#include <Util/ColorText.h>

using namespace MugenNamespace::UserInterface::Widgets;
using namespace MugenNamespace::Util;

void MugenNamespace::UserInterface::Widgets::SessionTable::setupUi(QWidget *Form, QString TeamserverName)
{
    this->TeamserverName = TeamserverName;

    gridLayout = new QGridLayout( this );
    gridLayout->setObjectName( QString::fromUtf8( "gridLayout" ) );
    gridLayout->setContentsMargins( 0, 0, 0, 0 );
    gridLayout->setSpacing( 2 );
    SessionTableWidget = new QTableWidget( this );

    if ( SessionTableWidget->columnCount() < 11 )
        SessionTableWidget->setColumnCount( 11 );

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
    SessionTableWidget->horizontalHeader()->resizeSection( 5, 150 );

    SessionTableWidget->setEnabled( true );
    SessionTableWidget->setShowGrid( false );
    SessionTableWidget->setSortingEnabled( false );
    SessionTableWidget->setWordWrap( true );
    SessionTableWidget->setCornerButtonEnabled( true );
    SessionTableWidget->horizontalHeader()->setVisible( true );
    SessionTableWidget->setSelectionBehavior( QAbstractItemView::SelectRows );
    SessionTableWidget->setContextMenuPolicy( Qt::CustomContextMenu );
    SessionTableWidget->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeMode::Stretch );
    SessionTableWidget->horizontalHeader()->setStretchLastSection( true );
    SessionTableWidget->verticalHeader()->setVisible( false );
    SessionTableWidget->verticalHeader()->setDefaultSectionSize( 28 );
    SessionTableWidget->setFocusPolicy( Qt::NoFocus );
    SessionTableWidget->setAlternatingRowColors( true );

    SessionTableWidget->horizontalHeaderItem( 0 )->setSizeHint( QSize( 0, 0 ) );

    connect( SessionTableWidget, &QTableWidget::itemSelectionChanged, this, &MugenNamespace::UserInterface::Widgets::SessionTable::updateRow );

    // Filter bar
    FilterInput = new QLineEdit( this );
    FilterInput->setPlaceholderText( "Filter: type:tg  health:healthy  user:root  listener:C2  ip:10.0  (space = AND)" );
    FilterInput->setObjectName( "FilterInput" );
    FilterInput->setMaximumHeight( 24 );
    FilterInput->setStyleSheet( "QLineEdit { background: #12121f; color: #f8f8f2; border: 1px solid #2a2d4a; border-radius: 3px; padding: 0 6px; font-size: 11px; }" );
    connect( FilterInput, &QLineEdit::textChanged, this, &MugenNamespace::UserInterface::Widgets::SessionTable::applyFilter );

    gridLayout->addWidget( FilterInput,         0, 0, 1, 1 );
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

// Query language: space-separated tokens, each "field:value" or plain text.
// Fields: type, health, user, listener, ip, id, os, proc, computer
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

        QString listener, agentType;
        auto id = getCol(0);
        for ( const auto& s : MugenX::Teamserver.Sessions ) {
            if ( s.Name.toLower() == id ) {
                listener  = s.Listener.toLower();
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
                if ( !matched ) matched = listener.contains(v) || agentType.contains(v);
            }

            if ( !matched ) { visible = false; break; }
        }

        SessionTableWidget->setRowHidden( row, !visible );
    }
}
