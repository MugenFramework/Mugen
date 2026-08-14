#include <Mugen/Mugen.hpp>
#include <global.hpp>

#include <UserInterface/Widgets/ResourceManagerWidget.hpp>
#include <Mugen/Packager.hpp>
#include <Util/Base.hpp>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QLineEdit>
#include <QSpacerItem>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMessageBox>
#include <QLabel>
#include <QFrame>
#include <QGridLayout>
#include <QJsonObject>
#include <QApplication>
#include <QClipboard>

using namespace MugenNamespace::UserInterface::Widgets;

// ── helpers ──────────────────────────────────────────────────────────────────

static QTableWidgetItem* cell( const QString& text )
{
    auto* item = new QTableWidgetItem( text );
    item->setFlags( item->flags() & ~Qt::ItemIsEditable );
    return item;
}

static QString guessKind( const QString& name )
{
    auto ext = QFileInfo( name ).suffix().toLower();
    if ( ext == "exe" || ext == "dll" ) return "exe";
    if ( ext == "o"  || ext == "obj" ) return "bof";
    if ( ext == "py" || ext == "js"
      || ext == "ps1"|| ext == "sh"  ) return "script";
    return "other";
}

// ── ResourceManagerWidget ─────────────────────────────────────────────────────

ResourceManagerWidget::ResourceManagerWidget( QWidget* parent ) : QWidget( parent ) {}

void ResourceManagerWidget::setupUi( QWidget* widget )
{
    ResourceView = widget ? widget : new QWidget();
    ResourceView->setObjectName( "ResourceView" );

    auto* root = new QGridLayout( ResourceView );
    root->setObjectName( "gridLayout" );
    root->setContentsMargins( 0, 0, 0, 3 );

    // ── Search bar ─────────────────────────────────────────────────────────
    SearchBar = new QLineEdit( ResourceView );
    SearchBar->setObjectName( "searchBar" );
    SearchBar->setPlaceholderText( "Search resources..." );
    SearchBar->setClearButtonEnabled( true );
    root->addWidget( SearchBar, 0, 0, 1, 5 );

    // ── Table ──────────────────────────────────────────────────────────────
    ResourceTable = new QTableWidget( 0, 5, ResourceView );
    ResourceTable->setHorizontalHeaderItem( 0, new QTableWidgetItem( "Name"     ) );
    ResourceTable->setHorizontalHeaderItem( 1, new QTableWidgetItem( "Uploader" ) );
    ResourceTable->setHorizontalHeaderItem( 2, new QTableWidgetItem( "Kind"     ) );
    ResourceTable->setHorizontalHeaderItem( 3, new QTableWidgetItem( "Size"     ) );
    ResourceTable->setHorizontalHeaderItem( 4, new QTableWidgetItem( "Added"    ) );
    ResourceTable->horizontalHeader()->setSectionResizeMode( 0, QHeaderView::Stretch );
    ResourceTable->horizontalHeader()->setSectionResizeMode( 1, QHeaderView::ResizeToContents );
    ResourceTable->horizontalHeader()->setSectionResizeMode( 2, QHeaderView::ResizeToContents );
    ResourceTable->horizontalHeader()->setSectionResizeMode( 3, QHeaderView::ResizeToContents );
    ResourceTable->horizontalHeader()->setSectionResizeMode( 4, QHeaderView::ResizeToContents );
    ResourceTable->horizontalHeader()->setHighlightSections( false );
    ResourceTable->verticalHeader()->setVisible( false );
    ResourceTable->verticalHeader()->setDefaultSectionSize( 42 );
    ResourceTable->setShowGrid( false );
    ResourceTable->setFocusPolicy( Qt::NoFocus );
    ResourceTable->setSortingEnabled( false );
    ResourceTable->setSelectionBehavior( QAbstractItemView::SelectRows );
    ResourceTable->setSelectionMode( QAbstractItemView::SingleSelection );

    root->addWidget( ResourceTable, 1, 0, 1, 5 );

    // ── Detail panel ──────────────────────────────────────────────────────
    DetailPanel = new QFrame( ResourceView );
    DetailPanel->setObjectName( "detailPanel" );
    DetailPanel->setFrameShape( QFrame::NoFrame );
    DetailPanel->setStyleSheet(
        "QFrame#detailPanel {"
        "  background: #111118;"
        "  border-top: 1px solid #2a2a3f;"
        "  border-bottom: 1px solid #2a2a3f;"
        "}"
    );
    DetailPanel->setVisible( false );
    DetailPanel->setFixedHeight( 72 );

    auto* dp = new QVBoxLayout( DetailPanel );
    dp->setContentsMargins( 12, 8, 12, 8 );
    dp->setSpacing( 4 );

    // ── Ligne 1 : nom + badge kind + métadonnées ──
    auto* row1 = new QHBoxLayout;
    row1->setSpacing( 10 );

    dl_name = new QLabel( "-" );
    {
        auto f = dl_name->font();
        f.setBold( true );
        f.setPointSize( f.pointSize() + 1 );
        dl_name->setFont( f );
    }
    dl_name->setTextInteractionFlags( Qt::TextSelectableByMouse );

    dl_kind = new QLabel( "-" );
    dl_kind->setStyleSheet(
        "QLabel { background: #1a1a27; color: #ff6b9d;"
        "  border: 1px solid #ff6b9d; border-radius: 3px;"
        "  padding: 1px 6px; font-size: 10px; letter-spacing: 1px; }"
    );
    dl_kind->setFixedHeight( 18 );

    auto makeMetaBlock = []( const QString& key ) -> QWidget* {
        auto* w  = new QWidget;
        auto* vb = new QVBoxLayout( w );
        vb->setContentsMargins( 0, 0, 0, 0 );
        vb->setSpacing( 1 );
        auto* k = new QLabel( key );
        k->setStyleSheet( "color: #44475a; font-size: 9px; letter-spacing: 1px;" );
        vb->addWidget( k );
        auto* v = new QLabel( "-" );
        v->setTextInteractionFlags( Qt::TextSelectableByMouse );
        vb->addWidget( v );
        return w;
    };

    auto* sizeBlock      = makeMetaBlock( "SIZE" );
    auto* uploaderBlock  = makeMetaBlock( "UPLOADER" );
    auto* addedBlock     = makeMetaBlock( "ADDED" );

    dl_size     = sizeBlock->findChildren<QLabel*>().last();
    dl_uploader = uploaderBlock->findChildren<QLabel*>().last();
    dl_added    = addedBlock->findChildren<QLabel*>().last();

    row1->addWidget( dl_name );
    row1->addWidget( dl_kind );
    row1->addSpacing( 8 );
    row1->addWidget( sizeBlock );
    row1->addWidget( uploaderBlock );
    row1->addWidget( addedBlock );
    row1->addStretch();

    // ── Ligne 2 : SHA-256 + bouton copy + path ──
    auto* row2 = new QHBoxLayout;
    row2->setSpacing( 6 );

    auto* hashKey = new QLabel( "SHA-256" );
    hashKey->setStyleSheet( "color: #44475a; font-size: 9px; letter-spacing: 1px; min-width: 46px;" );

    dl_hash = new QLabel( "-" );
    {
        auto f = dl_hash->font();
        f.setFamily( "monospace" );
        f.setPointSize( f.pointSize() - 1 );
        dl_hash->setFont( f );
    }
    dl_hash->setTextInteractionFlags( Qt::TextSelectableByMouse );
    dl_hash->setStyleSheet( "color: #f0f0ee;" );

    dl_copyHash = new QPushButton( "Copy" );
    dl_copyHash->setFixedSize( 38, 18 );
    dl_copyHash->setStyleSheet(
        "QPushButton { background: #1a1a27; color: #44475a; border: 1px solid #2a2a3f;"
        "  border-radius: 3px; font-size: 9px; letter-spacing: 1px; }"
        "QPushButton:hover { color: #ff6b9d; border-color: #ff6b9d; }"
    );
    connect( dl_copyHash, &QPushButton::clicked, this, [this]() {
        if ( dl_hash ) QApplication::clipboard()->setText( dl_hash->text() );
    } );

    auto* pathKey = new QLabel( "PATH" );
    pathKey->setStyleSheet( "color: #44475a; font-size: 9px; letter-spacing: 1px; min-width: 28px;" );

    dl_path = new QLabel( "-" );
    dl_path->setStyleSheet( "color: #44475a;" );
    dl_path->setTextInteractionFlags( Qt::TextSelectableByMouse );

    row2->addWidget( hashKey );
    row2->addWidget( dl_hash );
    row2->addWidget( dl_copyHash );
    row2->addSpacing( 16 );
    row2->addWidget( pathKey );
    row2->addWidget( dl_path, 1 );

    dp->addLayout( row1 );
    dp->addLayout( row2 );

    root->addWidget( DetailPanel, 2, 0, 1, 5 );

    // ── Buttons ────────────────────────────────────────────────────────────
    auto* spacer1 = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    root->addItem( spacer1, 3, 0 );

    UploadButton = new QPushButton( "Upload", ResourceView );
    UploadButton->setObjectName( "uploadButton" );
    root->addWidget( UploadButton, 3, 1 );

    DeleteButton = new QPushButton( "Delete", ResourceView );
    DeleteButton->setObjectName( "deleteButton" );
    DeleteButton->setEnabled( false );
    root->addWidget( DeleteButton, 3, 2 );

    DownloadButton = new QPushButton( "Download", ResourceView );
    DownloadButton->setObjectName( "downloadButton" );
    DownloadButton->setEnabled( false );
    root->addWidget( DownloadButton, 3, 3 );

    auto* spacer2 = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    root->addItem( spacer2, 3, 4 );

    // ── Connections ────────────────────────────────────────────────────────
    connect( SearchBar, &QLineEdit::textChanged, this, [this]( const QString& text ) {
        applyFilter( text );
    } );

    connect( ResourceTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        int row = ResourceTable->currentRow();
        bool sel = row >= 0 && !ResourceTable->selectedItems().isEmpty();
        DeleteButton->setEnabled( sel );
        DownloadButton->setEnabled( sel );

        if ( sel && row < (int)m_entries.size() )
        {
            // retrouver l'entrée correspondante dans m_entries via le nom de la row
            auto* nameItem = ResourceTable->item( row, 0 );
            if ( nameItem )
            {
                auto name = nameItem->text();
                for ( const auto& v : m_entries )
                {
                    auto obj = v.toObject();
                    if ( obj[ "Name" ].toString() == name )
                    {
                        showDetail( obj );
                        break;
                    }
                }
            }
        }
        else
        {
            hideDetail();
        }
    } );

    connect( UploadButton, &QPushButton::clicked, this, [this]() {
        auto path = ThemedOpenFileDialog( "Upload Resource", "All files (*.*)" );
        if ( path.isEmpty() ) return;

        QFile f( path );
        if ( !f.open( QIODevice::ReadOnly ) ) {
            QMessageBox::critical( nullptr, "Error", "Cannot open file: " + path );
            return;
        }
        auto data = f.readAll();
        f.close();

        auto name = QFileInfo( path ).fileName();

        // Vérification overwrite
        for ( const auto& v : m_entries )
        {
            if ( v.toObject()[ "Name" ].toString() == name )
            {
                auto ans = QMessageBox::question(
                    nullptr, "Overwrite?",
                    "\"" + name + "\" already exists in resources.\nOverwrite it?",
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No
                );
                if ( ans != QMessageBox::Yes ) return;
                break;
            }
        }

        auto kind = guessKind( name );

        Util::Packager::Body_t body;
        body.SubEvent              = Util::Packager::Resource::Add;
        body.Info[ "Name"    ]     = name.toStdString();
        body.Info[ "Kind"    ]     = kind.toStdString();
        body.Info[ "Content" ]     = data.toBase64().toStdString();
        body.Info[ "User"    ]     = Username.toStdString();

        NewPackageResource( TeamserverName, body );
    } );

    connect( DeleteButton, &QPushButton::clicked, this, [this]() {
        int row = ResourceTable->currentRow();
        if ( row < 0 ) return;
        auto* nameItem = ResourceTable->item( row, 0 );
        if ( !nameItem ) return;

        Util::Packager::Body_t body;
        body.SubEvent          = Util::Packager::Resource::Remove;
        body.Info[ "Name" ]    = nameItem->text().toStdString();

        NewPackageResource( TeamserverName, body );
    } );

    // ── Context menu ──────────────────────────────────────────────────────
    ResourceTable->setContextMenuPolicy( Qt::CustomContextMenu );
    connect( ResourceTable, &QTableWidget::customContextMenuRequested, this,
             [this]( const QPoint& pos )
    {
        auto* item = ResourceTable->itemAt( pos );
        if ( !item ) return;

        int row = ResourceTable->row( item );
        auto* nameItem = ResourceTable->item( row, 0 );
        if ( !nameItem ) return;

        QMenu menu( ResourceTable );

        auto* actDownload = menu.addAction( "Download" );
        auto* actDelete   = menu.addAction( "Delete"   );

        auto* chosen = menu.exec( ResourceTable->viewport()->mapToGlobal( pos ) );

        if ( chosen == actDownload )
        {
            Util::Packager::Body_t body;
            body.SubEvent               = Util::Packager::Resource::Download;
            body.Info[ "Name"        ]  = nameItem->text().toStdString();
            body.Info[ "RequestUser" ]  = Username.toStdString();
            NewPackageResource( TeamserverName, body );
        }
        else if ( chosen == actDelete )
        {
            Util::Packager::Body_t body;
            body.SubEvent          = Util::Packager::Resource::Remove;
            body.Info[ "Name" ]    = nameItem->text().toStdString();
            NewPackageResource( TeamserverName, body );
        }
    } );

    connect( DownloadButton, &QPushButton::clicked, this, [this]() {
        int row = ResourceTable->currentRow();
        if ( row < 0 ) return;
        auto* nameItem = ResourceTable->item( row, 0 );
        if ( !nameItem ) return;

        Util::Packager::Body_t body;
        body.SubEvent               = Util::Packager::Resource::Download;
        body.Info[ "Name"        ]  = nameItem->text().toStdString();
        body.Info[ "RequestUser" ]  = Username.toStdString();

        NewPackageResource( TeamserverName, body );
    } );

    // Demande la liste courante au serveur dès l'ouverture de l'onglet
    // (l'event envoyé au connect est perdu si le widget n'existe pas encore)
    Util::Packager::Body_t listReq;
    listReq.SubEvent = Util::Packager::Resource::List;
    NewPackageResource( TeamserverName, listReq );
}

void ResourceManagerWidget::Refresh( const QString& resourcesJson )
{
    auto doc = QJsonDocument::fromJson( resourcesJson.toUtf8() );
    m_entries = doc.isArray() ? doc.array() : QJsonArray{};
    applyFilter( SearchBar ? SearchBar->text() : QString{} );
}

void ResourceManagerWidget::applyFilter( const QString& query )
{
    auto filtered = QJsonArray{};
    if ( query.trimmed().isEmpty() )
    {
        filtered = m_entries;
    }
    else
    {
        auto q = query.trimmed().toLower();
        for ( const auto& v : m_entries )
        {
            auto obj = v.toObject();
            if ( obj[ "Name"    ].toString().toLower().contains( q ) ||
                 obj[ "Kind"    ].toString().toLower().contains( q ) ||
                 obj[ "AddedAt" ].toString().toLower().contains( q ) )
            {
                filtered.append( obj );
            }
        }
    }
    rebuildTable( filtered );
}

void ResourceManagerWidget::showDetail( const QJsonObject& obj )
{
    if ( !DetailPanel ) return;

    auto val = [&]( const QString& key ) -> QString {
        auto s = obj[ key ].toString().trimmed();
        return s.isEmpty() ? "-" : s;
    };

    if ( dl_name )     dl_name->setText( val( "Name" ) );
    if ( dl_kind )     dl_kind->setText( val( "Kind" ) );
    if ( dl_size )     dl_size->setText( val( "Size" ) );
    if ( dl_uploader ) dl_uploader->setText( val( "User" ) );
    if ( dl_added )    dl_added->setText( val( "AddedAt" ) );
    if ( dl_hash )     dl_hash->setText( val( "Hash" ) );
    if ( dl_path )     dl_path->setText( val( "Path" ) );

    DetailPanel->setVisible( true );
}

void ResourceManagerWidget::hideDetail()
{
    if ( DetailPanel ) DetailPanel->setVisible( false );
}

void ResourceManagerWidget::rebuildTable( const QJsonArray& entries )
{
    if ( !ResourceTable ) return;

    ResourceTable->clearSpans();
    ResourceTable->setRowCount( 0 );
    DeleteButton->setEnabled( false );
    hideDetail();

    if ( entries.isEmpty() )
    {
        ResourceTable->insertRow( 0 );
        auto* empty = new QTableWidgetItem( "No resources uploaded yet" );
        empty->setTextAlignment( Qt::AlignCenter );
        empty->setForeground( QColor( "#3a3a5a" ) );
        empty->setFlags( Qt::ItemIsEnabled );
        ResourceTable->setItem( 0, 0, empty );
        ResourceTable->setSpan( 0, 0, 1, 5 );
        ResourceTable->setRowHeight( 0, 60 );
        return;
    }

    for ( int i = 0; i < entries.size(); i++ )
    {
        auto obj  = entries[ i ].toObject();
        auto hash = obj[ "Hash" ].toString();

        auto* nameItem = cell( obj[ "Name" ].toString() );
        if ( !hash.isEmpty() )
            nameItem->setToolTip( "SHA-256: " + hash );

        ResourceTable->insertRow( i );
        ResourceTable->setItem( i, 0, nameItem );
        ResourceTable->setItem( i, 1, cell( obj[ "User"    ].toString() ) );
        ResourceTable->setItem( i, 2, cell( obj[ "Kind"    ].toString() ) );
        ResourceTable->setItem( i, 3, cell( obj[ "Size"    ].toString() ) );
        ResourceTable->setItem( i, 4, cell( obj[ "AddedAt" ].toString() ) );
    }
}
