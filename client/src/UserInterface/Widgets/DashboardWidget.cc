#include <global.hpp>
#include <Mugen/Service.hpp>
#include <UserInterface/Widgets/DashboardWidget.hpp>
#include <UserInterface/Widgets/LootWidget.h>
#include <UserInterface/Widgets/TeamserverTabSession.h>

#include <QPainter>
#include <QHeaderView>
#include <QFont>

DashboardWidget::DashboardWidget( QWidget* parent )
    : QWidget( parent )
{
    auto root = new QVBoxLayout( this );
    root->setContentsMargins( 12, 12, 12, 12 );
    root->setSpacing( 12 );

    // ── stats row ──────────────────────────────────────────────────────────
    auto statsRow = new QHBoxLayout();
    statsRow->setSpacing( 10 );

    statsRow->addWidget( makeCard( &statLiveTengu, "TENGU LIVE",  QColor("#ff6b9d") ) );
    statsRow->addWidget( makeCard( &statLiveDemon, "DEMON LIVE",  QColor("#8be9fd") ) );
    statsRow->addWidget( makeCard( &statDead,      "DEAD",        QColor("#555566") ) );
    statsRow->addWidget( makeCard( &statTotal,     "TOTAL",       QColor("#bd93f9") ) );
    statsRow->addStretch();

    root->addLayout( statsRow );

    // ── bottom grid: creds left, downloads right ───────────────────────────
    auto bottomRow = new QHBoxLayout();
    bottomRow->setSpacing( 12 );

    auto mkTable = []( const QStringList& headers ) -> QTableWidget* {
        auto t = new QTableWidget( 0, headers.size() );
        t->setHorizontalHeaderLabels( headers );
        t->setEditTriggers( QAbstractItemView::NoEditTriggers );
        t->setSelectionBehavior( QAbstractItemView::SelectRows );
        t->setSelectionMode( QAbstractItemView::SingleSelection );
        t->verticalHeader()->setVisible( false );
        t->horizontalHeader()->setStretchLastSection( true );
        t->setShowGrid( false );
        t->setAlternatingRowColors( true );
        t->setMaximumHeight( 220 );
        return t;
    };

    auto leftBox  = new QVBoxLayout();
    auto lblCreds = new QLabel( "Recent Credentials" );
    lblCreds->setStyleSheet( "color:#ff6b9d; font-weight:bold; font-size:11px; letter-spacing:1px;" );
    recentCreds = mkTable( { "Type", "User", "Domain", "Source", "Agent" } );
    leftBox->addWidget( lblCreds );
    leftBox->addWidget( recentCreds );

    auto rightBox    = new QVBoxLayout();
    auto lblDownloads = new QLabel( "Recent Downloads" );
    lblDownloads->setStyleSheet( "color:#bd93f9; font-weight:bold; font-size:11px; letter-spacing:1px;" );
    recentDownloads = mkTable( { "File", "Size", "Date", "Agent" } );
    rightBox->addWidget( lblDownloads );
    rightBox->addWidget( recentDownloads );

    bottomRow->addLayout( leftBox,  1 );
    bottomRow->addLayout( rightBox, 1 );

    root->addLayout( bottomRow );
    root->addStretch();
}

QFrame* DashboardWidget::makeCard( QLabel** label, const QString& title, const QColor& accent )
{
    auto card = new QFrame();
    card->setFixedSize( 140, 72 );
    card->setStyleSheet( QString(
        "QFrame { background: #1a1a2e; border: 1px solid %1; border-radius: 4px; }"
    ).arg( accent.name() ) );

    auto lay = new QVBoxLayout( card );
    lay->setContentsMargins( 10, 8, 10, 8 );
    lay->setSpacing( 2 );

    auto titleLbl = new QLabel( title );
    titleLbl->setStyleSheet( QString("color:%1; font-size:9px; letter-spacing:1px; font-weight:bold;").arg( accent.name() ) );

    *label = new QLabel( "0" );
    (*label)->setStyleSheet( "color:#f8f8f2; font-size:26px; font-weight:bold;" );

    lay->addWidget( titleLbl );
    lay->addWidget( *label );
    return card;
}

void DashboardWidget::Refresh()
{
    int liveTengu = 0, liveDemon = 0, dead = 0;

    for ( const auto& s : MugenX::Teamserver.Sessions )
    {
        bool isDead = ( s.Marked == "Dead" || s.Health == "dead" || s.Health == "killdate" );
        if ( isDead ) { dead++; continue; }

        if ( s.MagicValue == TenguMagicValue )
            liveTengu++;
        else
            liveDemon++;
    }

    statLiveTengu->setText( QString::number( liveTengu ) );
    statLiveDemon->setText( QString::number( liveDemon ) );
    statDead->setText     ( QString::number( dead ) );
    statTotal->setText    ( QString::number( liveTengu + liveDemon + dead ) );

    // recent credentials (last 8)
    auto loot = MugenX::Teamserver.TabSession->LootWidget;
    if ( loot )
    {
        recentCreds->setRowCount( 0 );
        int shown = 0;
        for ( int i = (int)loot->LootItems.size() - 1; i >= 0 && shown < 8; i-- )
        {
            const auto& item = loot->LootItems[i];
            if ( item.Type != LootWidget::LOOT_CREDENTIAL )
                continue;

            int row = recentCreds->rowCount();
            recentCreds->insertRow( row );
            recentCreds->setItem( row, 0, new QTableWidgetItem( item.Cred.CredType ) );
            recentCreds->setItem( row, 1, new QTableWidgetItem( item.Cred.Username ) );
            recentCreds->setItem( row, 2, new QTableWidgetItem( item.Cred.Domain ) );
            recentCreds->setItem( row, 3, new QTableWidgetItem( item.Cred.Source ) );
            recentCreds->setItem( row, 4, new QTableWidgetItem( item.AgentID ) );
            shown++;
        }

        // recent downloads (last 8)
        recentDownloads->setRowCount( 0 );
        shown = 0;
        for ( int i = (int)loot->LootItems.size() - 1; i >= 0 && shown < 8; i-- )
        {
            const auto& item = loot->LootItems[i];
            if ( item.Type != LootWidget::LOOT_FILE )
                continue;

            int row = recentDownloads->rowCount();
            recentDownloads->insertRow( row );
            recentDownloads->setItem( row, 0, new QTableWidgetItem( item.Data.Name ) );
            recentDownloads->setItem( row, 1, new QTableWidgetItem( item.Data.Size ) );
            recentDownloads->setItem( row, 2, new QTableWidgetItem( item.Data.Date ) );
            recentDownloads->setItem( row, 3, new QTableWidgetItem( item.AgentID ) );
            shown++;
        }
    }
}
