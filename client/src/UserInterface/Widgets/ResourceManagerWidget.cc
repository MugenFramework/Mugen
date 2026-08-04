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
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

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
    ResourceTable = new QTableWidget( 0, 4, ResourceView );
    ResourceTable->setHorizontalHeaderItem( 0, new QTableWidgetItem( "Name"    ) );
    ResourceTable->setHorizontalHeaderItem( 1, new QTableWidgetItem( "Kind"    ) );
    ResourceTable->setHorizontalHeaderItem( 2, new QTableWidgetItem( "Size"    ) );
    ResourceTable->setHorizontalHeaderItem( 3, new QTableWidgetItem( "Added"   ) );
    ResourceTable->horizontalHeader()->setSectionResizeMode( 0, QHeaderView::Stretch );
    ResourceTable->horizontalHeader()->setSectionResizeMode( 1, QHeaderView::ResizeToContents );
    ResourceTable->horizontalHeader()->setSectionResizeMode( 2, QHeaderView::ResizeToContents );
    ResourceTable->horizontalHeader()->setSectionResizeMode( 3, QHeaderView::ResizeToContents );
    ResourceTable->horizontalHeader()->setHighlightSections( false );
    ResourceTable->verticalHeader()->setVisible( false );
    ResourceTable->verticalHeader()->setDefaultSectionSize( 42 );
    ResourceTable->setShowGrid( false );
    ResourceTable->setFocusPolicy( Qt::NoFocus );
    ResourceTable->setSortingEnabled( false );
    ResourceTable->setSelectionBehavior( QAbstractItemView::SelectRows );
    ResourceTable->setSelectionMode( QAbstractItemView::SingleSelection );

    root->addWidget( ResourceTable, 1, 0, 1, 5 );

    // ── Buttons ────────────────────────────────────────────────────────────
    auto* spacer1 = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    root->addItem( spacer1, 2, 0 );

    UploadButton = new QPushButton( "Upload", ResourceView );
    UploadButton->setObjectName( "uploadButton" );
    root->addWidget( UploadButton, 2, 1 );

    DeleteButton = new QPushButton( "Delete", ResourceView );
    DeleteButton->setObjectName( "deleteButton" );
    DeleteButton->setEnabled( false );
    root->addWidget( DeleteButton, 2, 2 );

    DownloadButton = new QPushButton( "Download", ResourceView );
    DownloadButton->setObjectName( "downloadButton" );
    DownloadButton->setEnabled( false );
    root->addWidget( DownloadButton, 2, 3 );

    auto* spacer2 = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    root->addItem( spacer2, 2, 4 );

    // ── Connections ────────────────────────────────────────────────────────
    connect( SearchBar, &QLineEdit::textChanged, this, [this]( const QString& text ) {
        applyFilter( text );
    } );

    connect( ResourceTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        bool sel = !ResourceTable->selectedItems().isEmpty();
        DeleteButton->setEnabled( sel );
        DownloadButton->setEnabled( sel );
    } );

    connect( UploadButton, &QPushButton::clicked, this, [this]() {
        auto path = QFileDialog::getOpenFileName(
            nullptr, "Upload Resource", QDir::homePath(),
            "All files (*.*)"
        );
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

void ResourceManagerWidget::rebuildTable( const QJsonArray& entries )
{
    if ( !ResourceTable ) return;

    ResourceTable->clearSpans();
    ResourceTable->setRowCount( 0 );
    DeleteButton->setEnabled( false );

    if ( entries.isEmpty() )
    {
        ResourceTable->insertRow( 0 );
        auto* empty = new QTableWidgetItem( "No resources uploaded yet" );
        empty->setTextAlignment( Qt::AlignCenter );
        empty->setForeground( QColor( "#3a3a5a" ) );
        empty->setFlags( Qt::ItemIsEnabled );
        ResourceTable->setItem( 0, 0, empty );
        ResourceTable->setSpan( 0, 0, 1, 4 );
        ResourceTable->setRowHeight( 0, 60 );
        return;
    }

    for ( int i = 0; i < entries.size(); i++ )
    {
        auto obj = entries[ i ].toObject();
        ResourceTable->insertRow( i );
        ResourceTable->setItem( i, 0, cell( obj[ "Name"    ].toString() ) );
        ResourceTable->setItem( i, 1, cell( obj[ "Kind"    ].toString() ) );
        ResourceTable->setItem( i, 2, cell( obj[ "Size"    ].toString() ) );
        ResourceTable->setItem( i, 3, cell( obj[ "AddedAt" ].toString() ) );
    }
}
