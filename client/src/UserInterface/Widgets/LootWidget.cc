#include <global.hpp>
#include <spdlog/spdlog.h>

#include <UserInterface/Widgets/LootWidget.h>
#include <UserInterface/Widgets/TeamserverTabSession.h>
#include <Mugen/DBManager/DBManager.hpp>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QLabel>
#include <QFile>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QScrollBar>
#include <QKeyEvent>
#include <QGraphicsSceneWheelEvent>
#include <QDialog>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QDateTime>

// imagelabel.cpp
ImageLabel::ImageLabel( QWidget* parent ) : QWidget( parent )
{
    label      = new QLabel;
    scrollArea = new QScrollArea( this );

    label->setBackgroundRole( QPalette::Base );
    label->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
    label->setScaledContents( true );
    label->setStyleSheet( "background-color: #111118;\n"
                          "    color: #f0f0ee;" );
    label->setPixmap( QPixmap() );

    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidget( label );
}

void ImageLabel::resizeEvent( QResizeEvent* event )
{
    QWidget::resizeEvent( event );
    resizeImage();
}

const QPixmap* ImageLabel::pixmap() const
{
    return label->pixmap();
}

bool ImageLabel::event( QEvent* e )
{
    if ( e->type() == e->KeyPress )
    {
        auto eventKey = dynamic_cast<QKeyEvent*>( e );

        if ( eventKey->key() == Qt::Key_Control )
        {
            // spdlog::info( "Key_Control pressed" );
            key_ctrl = false;
        }
    }

    return QWidget::event( e );
}

void ImageLabel::keyReleaseEvent( QKeyEvent* event )
{
    if ( event->key() == Qt::Key_Control )
    {
        // spdlog::info( "Key_Control released" );
        key_ctrl = true;
    }

    QWidget::keyReleaseEvent( event );
}

void ImageLabel::wheelEvent( QWheelEvent* ev )
{
    // spdlog::info( "wheelEvent: {}", ev->angleDelta().y() );

    QWidget::wheelEvent( ev );
}

void ImageLabel::setPixmap( const QPixmap &pixmap )
{
    label->setPixmap( pixmap );
    scrollArea->setWidget( label );
    resizeImage();
}

void ImageLabel::resizeImage()
{
    label->setMinimumSize( size() );
    label->adjustSize();
    scrollArea->resize( size() );
}

LootWidget::LootWidget()
{
    if ( objectName().isEmpty() )
        setObjectName( QString::fromUtf8( "LootWidget" ) );

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

    gridLayout = new QGridLayout( this );
    gridLayout->setContentsMargins( 0, 0, 0, 0 );
    gridLayout->setObjectName( QString::fromUtf8( "gridLayout" ) );
    
    LabelShow = new QLabel( this );
    LabelShow->setObjectName( QString::fromUtf8( "LabelShow" ) );

    gridLayout->addWidget( LabelShow, 0, 3, 1, 1 );

    ComboShow = new QComboBox( this );
    ComboShow->addItem( QString( "Screenshots" ) );
    ComboShow->addItem( QString( "Downloads" ) );
    ComboShow->addItem( QString( "Credentials" ) );
    ComboShow->setObjectName( QString::fromUtf8( "ComboShow" ) );
    ComboShow->setMinimumSize( QSize( 150, 0 ) );

    gridLayout->addWidget( ComboShow, 0, 4, 1, 1 );

    LabelAgentID = new QLabel( this );
    LabelAgentID->setObjectName( QString::fromUtf8( "LabelAgentID" ) );
    LabelAgentID->setText( "AgentID: " );
    gridLayout->addWidget( LabelAgentID, 0, 1, 1, 1 );

    ComboAgentID = new QComboBox( this );
    ComboAgentID->setObjectName( QString::fromUtf8( "ComboAgentID" ) );
    ComboAgentID->setMinimumSize( QSize( 150, 0 ) );
    gridLayout->addWidget( ComboAgentID, 0, 2, 1, 1 );

    horizontalSpacer = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    gridLayout->addItem( horizontalSpacer, 0, 0, 1, 1 );

    StackWidget = new QStackedWidget( this );
    StackWidget->setObjectName( QString::fromUtf8( "StackWidget" ) );
    StackWidget->setContentsMargins( 0, 0, 0, 0 );

    Screenshots = new QWidget();
    Screenshots->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );

    Screenshots->setObjectName( QString::fromUtf8( "Screenshots" ) );
    gridLayout_2 = new QGridLayout( Screenshots );
    gridLayout_2->setObjectName( QString::fromUtf8( "gridLayout_2" ) );

    splitter = new QSplitter( Screenshots );
    splitter->setObjectName( QString::fromUtf8( "splitter" ) );
    splitter->setOrientation( Qt::Horizontal );
    splitter->setSizes( QList<int>() << 10 << 200 );

    ScreenshotTable = new QTableWidget( splitter );
    if ( ScreenshotTable->columnCount() < 2 )
        ScreenshotTable->setColumnCount( 2 );

    ScreenshotTable->setEnabled( true );
    ScreenshotTable->setShowGrid( false );
    ScreenshotTable->setSortingEnabled( false );
    ScreenshotTable->setWordWrap( true );
    ScreenshotTable->setCornerButtonEnabled( true );
    ScreenshotTable->horizontalHeader()->setVisible( true );
    ScreenshotTable->setSelectionBehavior( QAbstractItemView::SelectRows );
    ScreenshotTable->setContextMenuPolicy( Qt::CustomContextMenu );
    ScreenshotTable->horizontalHeader()->setSectionResizeMode( QHeaderView::Stretch );
    ScreenshotTable->verticalHeader()->setVisible(false);
    ScreenshotTable->verticalHeader()->setStretchLastSection( false );
    ScreenshotTable->verticalHeader()->setDefaultSectionSize( 28 );
    ScreenshotTable->setAlternatingRowColors( true );
    ScreenshotTable->setFocusPolicy( Qt::NoFocus );
    ScreenshotTable->setObjectName( QString::fromUtf8( "ScreenshotTable" ) );

    splitter->addWidget( ScreenshotTable );

    ScreenshotImage = new ImageLabel( splitter );
    ScreenshotImage->setObjectName( QString::fromUtf8( "ScreenshotImage" ) );

    splitter->addWidget( ScreenshotImage );

    gridLayout_2->addWidget(splitter, 0, 0, 1, 1);

    StackWidget->addWidget( Screenshots );
    Downloads = new QWidget();
    Downloads->setObjectName(QString::fromUtf8("Downloads"));
    gridLayout_3 = new QGridLayout( Downloads );
    gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));

    DownloadTable = new QTableWidget( Downloads );
    if ( DownloadTable->columnCount() < 3 )
        DownloadTable->setColumnCount( 3 );

    DownloadTable->setEnabled( true );
    DownloadTable->setShowGrid( false );
    DownloadTable->setSortingEnabled( false );
    DownloadTable->setWordWrap( true );
    DownloadTable->setCornerButtonEnabled( true );
    DownloadTable->horizontalHeader()->setVisible( true );
    DownloadTable->setSelectionBehavior( QAbstractItemView::SelectRows );
    DownloadTable->setContextMenuPolicy( Qt::CustomContextMenu );
    DownloadTable->horizontalHeader()->setSectionResizeMode( QHeaderView::Stretch );
    DownloadTable->verticalHeader()->setVisible(false);
    DownloadTable->verticalHeader()->setStretchLastSection( false );
    DownloadTable->verticalHeader()->setDefaultSectionSize( 28 );
    DownloadTable->setAlternatingRowColors( true );
    DownloadTable->setFocusPolicy( Qt::NoFocus );
    DownloadTable->setObjectName( QString::fromUtf8( "DownloadsTable" ) );

    gridLayout_3->addWidget( DownloadTable, 0, 0, 1, 1 );

    StackWidget->addWidget( Downloads );

    // Credentials tab
    Credentials = new QWidget();
    Credentials->setObjectName( QString::fromUtf8( "Credentials" ) );
    gridLayout_4 = new QGridLayout( Credentials );
    gridLayout_4->setObjectName( QString::fromUtf8( "gridLayout_4" ) );

    CredentialTable = new QTableWidget( Credentials );
    CredentialTable->setColumnCount( 7 );

    CredentialTable->setEnabled( true );
    CredentialTable->setShowGrid( false );
    CredentialTable->setSortingEnabled( false );
    CredentialTable->setWordWrap( true );
    CredentialTable->setCornerButtonEnabled( true );
    CredentialTable->horizontalHeader()->setVisible( true );
    CredentialTable->setSelectionBehavior( QAbstractItemView::SelectRows );
    CredentialTable->horizontalHeader()->setSectionResizeMode( QHeaderView::Stretch );
    CredentialTable->verticalHeader()->setVisible( false );
    CredentialTable->verticalHeader()->setDefaultSectionSize( 28 );
    CredentialTable->setFocusPolicy( Qt::NoFocus );
    CredentialTable->setContextMenuPolicy( Qt::CustomContextMenu );
    CredentialTable->setAlternatingRowColors( true );
    CredentialTable->setObjectName( QString::fromUtf8( "CredentialTable" ) );

    CredentialTable->setHorizontalHeaderItem( 0, new QTableWidgetItem( "TYPE"      ) );
    CredentialTable->setHorizontalHeaderItem( 1, new QTableWidgetItem( "USERNAME"  ) );
    CredentialTable->setHorizontalHeaderItem( 2, new QTableWidgetItem( "SECRET"    ) );
    CredentialTable->setHorizontalHeaderItem( 3, new QTableWidgetItem( "DOMAIN"    ) );
    CredentialTable->setHorizontalHeaderItem( 4, new QTableWidgetItem( "SOURCE"    ) );
    CredentialTable->setHorizontalHeaderItem( 5, new QTableWidgetItem( "AGENT"     ) );
    CredentialTable->setHorizontalHeaderItem( 6, new QTableWidgetItem( "DATE"      ) );

    gridLayout_4->addWidget( CredentialTable, 0, 0, 1, 1 );
    StackWidget->addWidget( Credentials );

    gridLayout->addWidget( StackWidget, 1, 0, 1, 6 );

    horizontalSpacer_2 = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    gridLayout->addItem( horizontalSpacer_2, 0, 5, 1, 1 );

    // Bottom credential action bar (hidden by default, shown only on Credentials tab)
    CredentialBar = new QWidget( this );
    auto credBarLayout = new QHBoxLayout( CredentialBar );
    credBarLayout->setContentsMargins( 4, 4, 4, 4 );
    credBarLayout->setSpacing( 4 );

    BtnAddCredential    = new QPushButton( "+ Add",    CredentialBar );
    BtnEditCredential   = new QPushButton( "Edit",     CredentialBar );
    BtnRemoveCredential = new QPushButton( "Remove",   CredentialBar );

    credBarLayout->addStretch();
    credBarLayout->addWidget( BtnAddCredential );
    credBarLayout->addWidget( BtnEditCredential );
    credBarLayout->addWidget( BtnRemoveCredential );
    credBarLayout->addStretch();

    CredentialBar->setVisible( false );
    gridLayout->addWidget( CredentialBar, 2, 0, 1, 6 );

    StackWidget->setCurrentIndex( 0 );

    ScreenshotTable->setHorizontalHeaderItem( 0, new QTableWidgetItem( "Name" ) );
    ScreenshotTable->setHorizontalHeaderItem( 1, new QTableWidgetItem( "Date" ) );

    DownloadTable->setHorizontalHeaderItem( 0, new QTableWidgetItem( "Name" ) );
    DownloadTable->setHorizontalHeaderItem( 1, new QTableWidgetItem( "Size" ) );
    DownloadTable->setHorizontalHeaderItem( 2, new QTableWidgetItem( "Date" ) );

    LabelShow->setText( "Show: " );

    ScreenshotMenu           = new QMenu( this );
    ScreenshotActionDownload = new QAction( "Download" );

    ScreenshotMenu->setStyleSheet( MenuStyle );
    ScreenshotMenu->addAction( ScreenshotActionDownload );

    connect( this, &QTableWidget::customContextMenuRequested, this, &LootWidget::onScreenshotTableCtx );
    connect( ScreenshotTable, &QTableWidget::clicked, this, &LootWidget::onScreenshotTableClick );
    connect( DownloadTable, &QTableWidget::clicked, this, &LootWidget::onDownloadTableClick );
    connect( splitter, &QSplitter::splitterMoved, ScreenshotImage, &ImageLabel::resizeImage );
    connect( ComboAgentID, &QComboBox::currentTextChanged, this, &LootWidget::onAgentChange );
    connect( ComboShow, &QComboBox::currentTextChanged, this, &LootWidget::onShowChange );
    connect( BtnAddCredential,    &QPushButton::clicked, this, &LootWidget::onAddCredential    );
    connect( BtnEditCredential,   &QPushButton::clicked, this, &LootWidget::onEditCredential   );
    connect( BtnRemoveCredential, &QPushButton::clicked, this, &LootWidget::onRemoveCredential );
    connect( CredentialTable, &QTableWidget::customContextMenuRequested, this, &LootWidget::onCredentialTableCtx );

    Reload();

    QMetaObject::connectSlotsByName( this );
}

void LootWidget::AddScreenshot( const QString& DemonID, const QString& Name, const QString& Date, const QByteArray& Data )
{
    spdlog::info( "Add Screenshot" );

    auto Item = LootData{
        .Type       = LOOT_IMAGE,
        .AgentID    = DemonID,
        .Data       = {
            .Name   = Name,
            .Date   = Date,
            .Data   = Data,
        },
    };

    LootItems.push_back( Item );

    if ( ComboAgentID->currentText().compare( DemonID ) == 0 || ComboAgentID->currentText().compare( "[ All ]" ) == 0 )
        ScreenshotTableAdd( Name, Date );
}

void LootWidget::AddDownload( const QString &DemonID, const QString &Name, const QString& Size, const QString &Date, const QByteArray &Data )
{
    auto Item = LootData{
        .Type       = LOOT_FILE,
        .AgentID    = DemonID,
        .Data       = {
            .Name = Name,
            .Date = Date,
            .Size = Size,
            .Data = Data,
        },
    };

    LootItems.push_back( Item );

    if ( ComboAgentID->currentText().compare( DemonID ) == 0 || ComboAgentID->currentText().compare( "[ All ]" ) == 0 )
        DownloadTableAdd( Name, Size, Date );
}

void LootWidget::Reload()
{
    ComboAgentID->clear();
    ComboAgentID->addItem( "[ All ]" );

    for ( auto& Session : MugenX::Teamserver.Sessions )
        ComboAgentID->addItem( Session.Name );

    // TODO: iterate over table items and free memory
    ScreenshotTable->setRowCount( 0 );
    DownloadTable->setRowCount( 0 );
    CredentialTable->setRowCount( 0 );
}

void LootWidget::onScreenshotTableClick( const QModelIndex &index )
{
    auto DemonID  = ComboAgentID->currentText();
    auto FileName = ScreenshotTable->item( index.row(), 0 )->text();

    for ( auto& item : LootItems )
    {
        if ( DemonID.compare( "[ All ]" ) == 0 || DemonID.compare( item.AgentID ) == 0 )
        {
            if ( item.Type == LOOT_IMAGE )
            {
                if ( item.Data.Name.compare( FileName ) == 0 )
                {
                    auto image = QPixmap();
                    if ( image.loadFromData( item.Data.Data ) )
                    {
                        ScreenshotImage->setPixmap( image );
                    }
                }
            }
        }
    }
}

void LootWidget::onDownloadTableClick( const QModelIndex &index )
{
    auto FileName = DownloadTable->item( index.row(), 0 )->text();
    auto lower    = FileName.toLower();
    if ( !lower.endsWith(".png") && !lower.endsWith(".jpg") &&
         !lower.endsWith(".jpeg") && !lower.endsWith(".bmp") )
        return;

    auto DemonID = ComboAgentID->currentText();
    for ( auto& item : LootItems )
    {
        if ( item.Type != LOOT_FILE ) continue;
        if ( item.Data.Name.compare( FileName ) != 0 ) continue;
        if ( DemonID.compare( "[ All ]" ) != 0 && DemonID.compare( item.AgentID ) != 0 ) continue;

        // Switch to Screenshots tab and display the image
        ComboShow->setCurrentText( "Screenshots" );

        QPixmap px;
        if ( item.Data.Data.isEmpty() ) {
            // load from disk if data wasn't kept in memory
            // savePath matches the teamserver convention
        } else if ( px.loadFromData( item.Data.Data ) ) {
            ScreenshotImage->setPixmap( px );
        }
        break;
    }
}

void LootWidget::onAgentChange( const QString& text )
{
    ScreenshotImage->setPixmap( QPixmap() );

    // todo: free columns items
    for ( int i = ScreenshotTable->rowCount(); i >= 0; i-- )
        ScreenshotTable->removeRow( i );

    for ( auto& item : LootItems )
    {
        if ( item.AgentID.compare( text ) == 0 || text.compare( "[ All ]" ) == 0 )
        {
            switch ( item.Type )
            {
                case LOOT_IMAGE:
                {
                    ScreenshotTableAdd( item.Data.Name, item.Data.Date );
                    break;
                }

                case LOOT_FILE:
                {
                    DownloadTableAdd( item.Data.Name, item.Data.Size, item.Data.Date );
                    break;
                }

                case LOOT_CREDENTIAL:
                {
                    CredentialTableAdd( item.Cred.CredType, item.Cred.Username, item.Cred.Secret,
                                        item.Cred.Domain, item.Cred.Source, item.AgentID, item.Cred.Timestamp );
                    break;
                }
            }
        }
    }
}

void LootWidget::AddCredential( const QString& DemonID, const QString& CredType, const QString& Username,
                                const QString& Secret, const QString& Domain, const QString& Source,
                                const QString& Timestamp )
{
    auto Item = LootData{};
    Item.Type       = LOOT_CREDENTIAL;
    Item.AgentID    = DemonID;
    Item.Cred.CredType  = CredType;
    Item.Cred.Username  = Username;
    Item.Cred.Secret    = Secret;
    Item.Cred.Domain    = Domain;
    Item.Cred.Source    = Source;
    Item.Cred.Timestamp = Timestamp;

    LootItems.push_back( Item );

    if ( ComboAgentID->currentText().compare( DemonID ) == 0 || ComboAgentID->currentText().compare( "[ All ]" ) == 0 )
        CredentialTableAdd( CredType, Username, Secret, Domain, Source, DemonID, Timestamp );
}

void LootWidget::CredentialTableAdd( const QString& CredType, const QString& Username, const QString& Secret,
                                     const QString& Domain, const QString& Source, const QString& AgentID,
                                     const QString& Timestamp )
{
    auto mkItem = []( const QString& text ) -> QTableWidgetItem* {
        auto i = new QTableWidgetItem( text );
        i->setTextAlignment( Qt::AlignCenter );
        i->setFlags( i->flags() ^ Qt::ItemIsEditable );
        return i;
    };

    int row = CredentialTable->rowCount();
    CredentialTable->setRowCount( row + 1 );

    CredentialTable->setItem( row, 0, mkItem( CredType  ) );
    CredentialTable->setItem( row, 1, mkItem( Username  ) );
    CredentialTable->setItem( row, 2, mkItem( Secret    ) );
    CredentialTable->setItem( row, 3, mkItem( Domain    ) );
    CredentialTable->setItem( row, 4, mkItem( Source    ) );
    CredentialTable->setItem( row, 5, mkItem( AgentID   ) );
    CredentialTable->setItem( row, 6, mkItem( Timestamp ) );
}

void LootWidget::AddSessionSection( const QString& AgentID )
{
    for ( int index = 0; index < ComboAgentID->count(); index++ )
    {
        if ( ComboAgentID->itemText( index ).compare( AgentID ) == 0 )
        {
            return;
        }
    }

    ComboAgentID->addItem( AgentID );
}

void LootWidget::onShowChange( const QString& text )
{
    if ( text.compare( "Screenshots" ) == 0 )
    {
        StackWidget->setCurrentIndex( 0 );
        CredentialBar->setVisible( false );
    }
    else if ( text.compare( "Downloads" ) == 0 )
    {
        StackWidget->setCurrentIndex( 1 );
        CredentialBar->setVisible( false );
    }
    else if ( text.compare( "Credentials" ) == 0 )
    {
        StackWidget->setCurrentIndex( 2 );
        CredentialBar->setVisible( true );
    }
}

void LootWidget::ScreenshotTableAdd( const QString &Name, const QString &Date )
{
    for ( int i = 0; i < ScreenshotTable->rowCount(); i++ )
    {
        if ( ScreenshotTable->item( i, 0 )->text().compare( Name ) == 0 )
        {
            return;
        }
    }

    auto item_Name = new QTableWidgetItem( Name );
    auto item_Date = new QTableWidgetItem( Date );

    item_Name->setTextAlignment( Qt::AlignCenter );
    item_Name->setFlags( item_Name->flags() ^ Qt::ItemIsEditable );

    item_Date->setTextAlignment( Qt::AlignCenter );
    item_Date->setFlags( item_Date->flags() ^ Qt::ItemIsEditable );

    ScreenshotTable->rowCount() < 1 ? ScreenshotTable->setRowCount( 1 ) : ScreenshotTable->setRowCount( ScreenshotTable->rowCount() + 1 );

    ScreenshotTable->setItem( ScreenshotTable->rowCount() - 1, 0, item_Name );
    ScreenshotTable->setItem( ScreenshotTable->rowCount() - 1, 1, item_Date );
}

void LootWidget::DownloadTableAdd( const QString &Name, const QString &Size, const QString &Date )
{
    auto item_Name = new QTableWidgetItem( Name );
    auto item_Size = new QTableWidgetItem( Size );
    auto item_Date = new QTableWidgetItem( Date );

    item_Name->setTextAlignment( Qt::AlignCenter );
    item_Name->setFlags( item_Name->flags() ^ Qt::ItemIsEditable );

    item_Size->setTextAlignment( Qt::AlignCenter );
    item_Size->setFlags( item_Size->flags() ^ Qt::ItemIsEditable );

    item_Date->setTextAlignment( Qt::AlignCenter );
    item_Date->setFlags( item_Date->flags() ^ Qt::ItemIsEditable );

    DownloadTable->rowCount() < 1 ? DownloadTable->setRowCount( 1 ) : DownloadTable->setRowCount( DownloadTable->rowCount() + 1 );

    DownloadTable->setItem( DownloadTable->rowCount() - 1, 0, item_Name );
    DownloadTable->setItem( DownloadTable->rowCount() - 1, 1, item_Size );
    DownloadTable->setItem( DownloadTable->rowCount() - 1, 2, item_Date );
}

void LootWidget::onScreenshotTableCtx( const QPoint &pos )
{
    if ( ! ScreenshotTable->itemAt( pos ) )
        return;

    ScreenshotMenu->popup( ScreenshotTable->horizontalHeader()->viewport()->mapToGlobal( pos ) );
}

void LootWidget::onCredentialTableCtx( const QPoint& pos )
{
    auto item = CredentialTable->itemAt( pos );
    if ( ! item )
        return;

    int row = CredentialTable->currentRow();
    if ( row < 0 )
        return;

    auto menu       = QMenu( this );
    auto actDelete  = menu.addAction( "Delete" );
    auto chosen     = menu.exec( CredentialTable->viewport()->mapToGlobal( pos ) );

    if ( chosen != actDelete )
        return;

    // Identify credential by Agent (col 5) + Username (col 1) + Date (col 6)
    auto agentID   = CredentialTable->item( row, 5 ) ? CredentialTable->item( row, 5 )->text() : QString();
    auto username  = CredentialTable->item( row, 1 ) ? CredentialTable->item( row, 1 )->text() : QString();
    auto timestamp = CredentialTable->item( row, 6 ) ? CredentialTable->item( row, 6 )->text() : QString();

    // Remove from DB
    if ( MugenX::Teamserver.TabSession && MugenX::Teamserver.TabSession->dbManager )
        MugenX::Teamserver.TabSession->dbManager->DeleteCredential( agentID, username, timestamp );

    // Remove from in-memory list
    LootItems.erase( std::remove_if( LootItems.begin(), LootItems.end(), [&]( const LootData& d ) {
        return d.Type == LOOT_CREDENTIAL
            && d.AgentID          == agentID
            && d.Cred.Username    == username
            && d.Cred.Timestamp   == timestamp;
    } ), LootItems.end() );

    // Remove from table
    CredentialTable->removeRow( row );
}

void LootWidget::onAddCredential()
{
    auto dialog   = new QDialog( this );
    dialog->setWindowTitle( "Add Credential" );
    dialog->setMinimumWidth( 420 );

    auto form     = new QFormLayout();
    auto editType = new QComboBox();
    auto editUser = new QLineEdit();
    auto editSec  = new QLineEdit();
    auto editDom  = new QLineEdit();
    auto editSrc  = new QLineEdit();
    auto editAgent= new QComboBox();

    editType->addItems( { "plaintext", "ntlm", "hash", "kerberos", "ssh-key", "api-key", "other" } );
    editSec->setEchoMode( QLineEdit::Normal );
    editSrc->setPlaceholderText( "command, module..." );

    // Populate agent list from current sessions
    editAgent->addItem( "" );
    for ( auto& s : MugenX::Teamserver.Sessions )
        editAgent->addItem( s.Name );

    // Pre-select current agent filter if set
    auto currentAgent = ComboAgentID->currentText();
    if ( currentAgent != "[ All ]" )
        editAgent->setCurrentText( currentAgent );

    form->addRow( "Type:",     editType  );
    form->addRow( "Username:", editUser  );
    form->addRow( "Secret:",   editSec   );
    form->addRow( "Domain:",   editDom   );
    form->addRow( "Source:",   editSrc   );
    form->addRow( "Agent:",    editAgent );

    auto btnRow    = new QHBoxLayout();
    auto btnSave   = new QPushButton( "Add" );
    auto btnCancel = new QPushButton( "Cancel" );
    btnRow->addStretch();
    btnRow->addWidget( btnSave );
    btnRow->addWidget( btnCancel );

    auto vbox = new QVBoxLayout( dialog );
    vbox->addLayout( form );
    vbox->addLayout( btnRow );

    connect( btnCancel, &QPushButton::clicked, dialog, &QDialog::reject );
    connect( btnSave, &QPushButton::clicked, dialog, [=]() {
        auto typ   = editType->currentText();
        auto usr   = editUser->text().trimmed();
        auto sec   = editSec->text().trimmed();
        auto dom   = editDom->text().trimmed();
        auto src   = editSrc->text().trimmed();
        auto agent = editAgent->currentText().trimmed();
        auto now   = QDateTime::currentDateTime().toString( "yyyy-MM-dd hh:mm:ss" );

        if ( usr.isEmpty() && sec.isEmpty() ) {
            dialog->reject();
            return;
        }

        // Persist to DB
        if ( MugenX::Teamserver.TabSession && MugenX::Teamserver.TabSession->dbManager ) {
            MugenNamespace::MugenSpace::DBManager::CredentialEntry entry{ agent, typ, usr, sec, dom, src, now };
            MugenX::Teamserver.TabSession->dbManager->AddCredential( entry );
        }

        AddSessionSection( agent );
        AddCredential( agent, typ, usr, sec, dom, src, now );

        dialog->accept();
    } );

    dialog->exec();
    delete dialog;
}

void LootWidget::LoadCredentialsFromDB( MugenNamespace::MugenSpace::DBManager* db )
{
    if ( !db ) return;

    LootItems.erase( std::remove_if( LootItems.begin(), LootItems.end(), []( const LootData& d ) {
        return d.Type == LOOT_CREDENTIAL;
    } ), LootItems.end() );
    CredentialTable->setRowCount( 0 );

    for ( auto& e : db->GetCredentials() ) {
        AddSessionSection( e.AgentID );
        AddCredential( e.AgentID, e.Type, e.Username, e.Secret, e.Domain, e.Source, e.Timestamp );
    }
}

void LootWidget::onEditCredential()
{
    int row = CredentialTable->currentRow();
    if ( row < 0 )
        return;

    auto getCol = [&]( int col ) -> QString {
        auto it = CredentialTable->item( row, col );
        return it ? it->text() : QString();
    };

    auto oldType  = getCol( 0 );
    auto oldUser  = getCol( 1 );
    auto oldSec   = getCol( 2 );
    auto oldDom   = getCol( 3 );
    auto oldSrc   = getCol( 4 );
    auto oldAgent = getCol( 5 );
    auto oldTs    = getCol( 6 );

    auto dialog   = new QDialog( this );
    dialog->setWindowTitle( "Edit Credential" );
    dialog->setMinimumWidth( 420 );

    auto form     = new QFormLayout();
    auto editType = new QComboBox();
    auto editUser = new QLineEdit( oldUser );
    auto editSec  = new QLineEdit( oldSec  );
    auto editDom  = new QLineEdit( oldDom  );
    auto editSrc  = new QLineEdit( oldSrc  );
    auto editAgent= new QComboBox();

    editType->addItems( { "plaintext", "ntlm", "hash", "kerberos", "ssh-key", "api-key", "other" } );
    editType->setCurrentText( oldType );
    editSrc->setPlaceholderText( "command, module..." );

    editAgent->addItem( "" );
    for ( auto& s : MugenX::Teamserver.Sessions )
        editAgent->addItem( s.Name );
    editAgent->setCurrentText( oldAgent );

    form->addRow( "Type:",     editType  );
    form->addRow( "Username:", editUser  );
    form->addRow( "Secret:",   editSec   );
    form->addRow( "Domain:",   editDom   );
    form->addRow( "Source:",   editSrc   );
    form->addRow( "Agent:",    editAgent );

    auto btnRow    = new QHBoxLayout();
    auto btnSave   = new QPushButton( "Save" );
    auto btnCancel = new QPushButton( "Cancel" );
    btnRow->addStretch();
    btnRow->addWidget( btnSave );
    btnRow->addWidget( btnCancel );

    auto vbox = new QVBoxLayout( dialog );
    vbox->addLayout( form );
    vbox->addLayout( btnRow );

    connect( btnCancel, &QPushButton::clicked, dialog, &QDialog::reject );
    connect( btnSave, &QPushButton::clicked, dialog, [=]() {
        auto typ   = editType->currentText();
        auto usr   = editUser->text().trimmed();
        auto sec   = editSec->text().trimmed();
        auto dom   = editDom->text().trimmed();
        auto src   = editSrc->text().trimmed();
        auto agent = editAgent->currentText().trimmed();

        if ( usr.isEmpty() && sec.isEmpty() ) {
            dialog->reject();
            return;
        }

        auto db = MugenX::Teamserver.TabSession ? MugenX::Teamserver.TabSession->dbManager : nullptr;

        // Delete old entry from DB
        if ( db )
            db->DeleteCredential( oldAgent, oldUser, oldTs );

        // Remove from in-memory list
        LootItems.erase( std::remove_if( LootItems.begin(), LootItems.end(), [&]( const LootData& d ) {
            return d.Type == LOOT_CREDENTIAL
                && d.AgentID        == oldAgent
                && d.Cred.Username  == oldUser
                && d.Cred.Timestamp == oldTs;
        } ), LootItems.end() );

        // Add updated entry
        auto now = QDateTime::currentDateTime().toString( "yyyy-MM-dd hh:mm:ss" );
        if ( db ) {
            MugenNamespace::MugenSpace::DBManager::CredentialEntry entry{ agent, typ, usr, sec, dom, src, now };
            db->AddCredential( entry );
        }

        // Remove the old table row, then re-add via AddCredential (avoids duplicate)
        CredentialTable->removeRow( row );

        AddSessionSection( agent );
        AddCredential( agent, typ, usr, sec, dom, src, now );

        dialog->accept();
    } );

    dialog->exec();
    delete dialog;
}

void LootWidget::onRemoveCredential()
{
    int row = CredentialTable->currentRow();
    if ( row < 0 )
        return;

    auto agentID   = CredentialTable->item( row, 5 ) ? CredentialTable->item( row, 5 )->text() : QString();
    auto username  = CredentialTable->item( row, 1 ) ? CredentialTable->item( row, 1 )->text() : QString();
    auto timestamp = CredentialTable->item( row, 6 ) ? CredentialTable->item( row, 6 )->text() : QString();

    if ( MugenX::Teamserver.TabSession && MugenX::Teamserver.TabSession->dbManager )
        MugenX::Teamserver.TabSession->dbManager->DeleteCredential( agentID, username, timestamp );

    LootItems.erase( std::remove_if( LootItems.begin(), LootItems.end(), [&]( const LootData& d ) {
        return d.Type == LOOT_CREDENTIAL
            && d.AgentID        == agentID
            && d.Cred.Username  == username
            && d.Cred.Timestamp == timestamp;
    } ), LootItems.end() );

    CredentialTable->removeRow( row );
}
