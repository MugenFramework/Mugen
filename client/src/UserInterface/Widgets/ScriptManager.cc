#include <UserInterface/Widgets/ScriptManager.h>
#include <UserInterface/Widgets/TeamserverTabSession.h>

#include <Mugen/DBManager/DBManager.hpp>

#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <Util/Base.hpp>

using namespace MugenNamespace::UserInterface::Widgets;

void ScriptManager::SetupUi( QWidget *Form )
{
    QString MenuStyle = "QMenu {\n"
                        "    background-color: #111118;\n"
                        "    color: #f0f0ee;\n"
                        "    border: 1px solid #2a2a3f;\n"
                        "}\n"
                        "\n"
                        "QMenu::separator {\n"
                        "    background: #2a2a3f;\n"
                        "}\n"
                        "\n"
                        "QMenu::item:selected {\n"
                        "    background: #2a2a3f;\n"
                        "}\n"
                        "\n"
                        "QAction {\n"
                        "    background-color: #111118;\n"
                        "    color: #f0f0ee;\n"
                        "}";


    this->ScriptManagerWidget = Form;

    if ( Form->objectName().isEmpty() )
        Form->setObjectName( QString::fromUtf8("Form") );

    Form->resize( 1417, 626 );

    gridLayout = new QGridLayout( Form );
    gridLayout->setObjectName( QString::fromUtf8( "gridLayout" ) );

    buttonLoadScript = new QPushButton( Form );
    buttonLoadScript->setObjectName( QString::fromUtf8( "buttonLoadScript" ) );
    gridLayout->addWidget( buttonLoadScript, 2, 1, 1, 1 );

    horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    gridLayout->addItem( horizontalSpacer, 2, 0, 1, 1 );

    horizontalSpacer_2 = new QSpacerItem( 40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    gridLayout->addItem( horizontalSpacer_2, 2, 2, 1, 1 );

    tableLoadedScripts = new QTableWidget( Form );

    tableLoadedScripts->setObjectName( QString::fromUtf8( "tableLoadedScripts" ) );
    tableLoadedScripts->setEnabled( true );
    tableLoadedScripts->setShowGrid( false );
    tableLoadedScripts->setSortingEnabled( false );
    tableLoadedScripts->setWordWrap( true );
    tableLoadedScripts->setCornerButtonEnabled( true );
    tableLoadedScripts->horizontalHeader()->setVisible( true );
    tableLoadedScripts->setSelectionBehavior( QAbstractItemView::SelectRows );
    tableLoadedScripts->setContextMenuPolicy( Qt::CustomContextMenu );
    tableLoadedScripts->horizontalHeader()->setSectionResizeMode( QHeaderView::Stretch );
    tableLoadedScripts->verticalHeader()->setVisible( false );
    tableLoadedScripts->horizontalHeader()->setStretchLastSection( true );
    tableLoadedScripts->verticalHeader()->setDefaultSectionSize( 28 );
    tableLoadedScripts->setAlternatingRowColors( true );
    tableLoadedScripts->setFocusPolicy( Qt::NoFocus );

    actionReload = new QAction( "Reload" );
    actionRemove = new QAction( "Remove" );

    menuScripts = new QMenu( this );
    menuScripts->setStyleSheet( MenuStyle );
    menuScripts->addAction( actionReload );
    menuScripts->addAction( actionRemove );

    if ( tableLoadedScripts->columnCount() < 1 )
        tableLoadedScripts->setColumnCount( 1 );

    tableLoadedScripts->setHorizontalHeaderItem(0, new QTableWidgetItem());

    gridLayout->addWidget( tableLoadedScripts, 0, 0, 1, 3 );
    gridLayout->setMargin( 0 );

    QObject::connect( buttonLoadScript, &QPushButton::clicked, this, &ScriptManager::b_LoadScript );

    QObject::connect( tableLoadedScripts, &QTableWidget::customContextMenuRequested, this, &ScriptManager::menu_ScriptMenu );
    QObject::connect( actionReload, &QAction::triggered, this, &ScriptManager::ReloadScript );
    QObject::connect( actionRemove, &QAction::triggered, this, &ScriptManager::RemoveScript );

    RetranslateUi();

    for ( auto& Script : MugenX::Teamserver.TabSession->dbManager->GetScripts() )
    {
        AddScriptTable( Script );
    }

    QMetaObject::connectSlotsByName(Form);
} // setupUi

void ScriptManager::RetranslateUi( )
{
    ScriptManagerWidget->setWindowTitle(QCoreApplication::translate("Form", "Script Manager", nullptr));
    buttonLoadScript->setText(QCoreApplication::translate("Form", "Load Script", nullptr));
    tableLoadedScripts->horizontalHeaderItem(0)->setText(QCoreApplication::translate("Form", "Path", nullptr));
}

bool ScriptManager::AddScript( QString Path )
{
    auto Script = FileRead( Path );
    auto path   = Path.toStdString();
    int  Return = 0;

    MugenX::Teamserver.LoadingScript = Path.toStdString();

    // Add the script's directory to sys.path so sibling modules (e.g. havoc.py) are importable.
    auto dir = QFileInfo( Path ).absolutePath().replace( "'", "\\'" );
    PyRun_SimpleString( QString( "import sys; sys.path.insert(0, '%1')" ).arg( dir ).toStdString().c_str() );

    if ( Script != nullptr ) {
        if ( ! Script.isEmpty() ) {
            Return = PyRun_SimpleStringFlags( Script.toStdString().c_str(), NULL );
            if ( Return == -1 ) {
                spdlog::error( "Failed to run script: {}", path );
            } else {
                return true;
            }
        }
        else {
            spdlog::error( "Script path not found: {}", path );
        }
    } else {
        spdlog::error( "Failed to load script: {}", path );
    }

    MugenX::Teamserver.LoadingScript = "";

    return false;
}

void ScriptManager::AddScriptTable( QString Path )
{
    if ( tableLoadedScripts->rowCount() < 1 )
        tableLoadedScripts->setRowCount( 1 );
    else
        tableLoadedScripts->setRowCount( tableLoadedScripts->rowCount() + 1 );

    tableLoadedScripts->setItem( tableLoadedScripts->rowCount() - 1, 0, new QTableWidgetItem( Path ) );

    // add to database
    if ( ! MugenX::Teamserver.TabSession->dbManager->CheckScript( Path ) )
        MugenX::Teamserver.TabSession->dbManager->AddScript( Path );
}


void ScriptManager::b_LoadScript()
{
    auto Filename = ThemedOpenFileDialog( "Load Script" );
    if ( Filename.isEmpty() )
        return;

    if ( AddScript( Filename ) ) {
        AddScriptTable( Filename );
    } else {
        auto messageBox = QMessageBox();
        messageBox.setWindowTitle( "Failed to import script" );
        messageBox.setText( "The script " + Filename + " could not be imported due to an error." );
        messageBox.setIcon( QMessageBox::Critical );
        messageBox.setStyleSheet( FileRead( ":/stylesheets/MessageBox" ) );
        messageBox.exec();
    }
}

void ScriptManager::menu_ScriptMenu( const QPoint &pos ) const
{
    auto DemonSelected = tableLoadedScripts->itemAt( pos );
    if ( ! DemonSelected )
        return;

    menuScripts->popup( tableLoadedScripts->horizontalHeader()->viewport()->mapToGlobal( pos ) );
}

void ScriptManager::ReloadScript() const
{
    auto Path = tableLoadedScripts->item( tableLoadedScripts->currentRow(), 0 )->text();

    // Just rerun the script
    AddScript( Path );
}

// TODO: clear python interpreter and reload every script except the one that got removed
void ScriptManager::RemoveScript() const
{
    auto Path = tableLoadedScripts->item( tableLoadedScripts->currentRow(), 0 )->text();

    tableLoadedScripts->removeRow( tableLoadedScripts->currentRow() );

    MugenX::Teamserver.TabSession->dbManager->RemoveScript( Path );
}
