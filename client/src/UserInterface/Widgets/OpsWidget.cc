#include <UserInterface/Widgets/OpsWidget.hpp>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QAbstractButton>

using namespace MugenNamespace::UserInterface::Widgets;

OpsWidget::OpsWidget( QWidget* parent ) : QWidget( parent ) {}

void OpsWidget::setupUi()
{
    setObjectName( "OpsWidget" );

    auto* root = new QVBoxLayout( this );
    root->setContentsMargins( 0, 0, 0, 0 );
    root->setSpacing( 0 );

    NavBar = new QWidget( this );
    NavBar->setObjectName( "OpsNavBar" );
    NavBar->setStyleSheet(
        "QWidget#OpsNavBar { background:#111118; border-bottom:1px solid #2a2a3f; }"
        "QPushButton {"
        "    background:transparent; color:#8888aa; border:none;"
        "    border-bottom:2px solid transparent; border-radius:0px;"
        "    padding:8px 18px; font-weight:bold; font-size:11px;"
        "    letter-spacing:1px; margin:0px;"
        "}"
        "QPushButton:hover { background:transparent; color:#f0f0ee; border:none;"
        "    border-bottom:2px solid #2a2a3f; }"
        "QPushButton:pressed { background:transparent; color:#ff6b9d; border:none;"
        "    border-bottom:2px solid #ff6b9d; }"
        "QPushButton:checked {"
        "    background:transparent; color:#ff6b9d; border:none;"
        "    border-bottom:2px solid #ff6b9d;"
        "}"
    );

    auto* navLayout = new QHBoxLayout( NavBar );
    navLayout->setContentsMargins( 8, 0, 8, 0 );
    navLayout->setSpacing( 0 );

    Buttons = new QButtonGroup( this );
    Buttons->setExclusive( true );

    navLayout->addWidget( addNavButton( "screenshots",  "Screenshots" ) );
    navLayout->addWidget( addNavButton( "credentials",  "Credentials" ) );
    navLayout->addWidget( addNavButton( "downloads",    "Downloads" ) );
    navLayout->addWidget( addNavButton( "resources",    "Resources" ) );
    navLayout->addWidget( addNavButton( "tasks",        "Tasks" ) );
    navLayout->addWidget( addNavButton( "networking",   "Networking" ) );
    navLayout->addStretch();

    Stack = new QStackedWidget( this );
    Stack->setObjectName( "OpsStack" );

    root->addWidget( NavBar );
    root->addWidget( Stack, 1 );
}

QPushButton* OpsWidget::addNavButton( const QString& id, const QString& label )
{
    auto* btn = new QPushButton( label, NavBar );
    btn->setCheckable( true );
    btn->setCursor( Qt::PointingHandCursor );
    btn->setProperty( "opsId", id );
    Buttons->addButton( btn );
    NavButtons.insert( id, btn );

    connect( btn, &QPushButton::clicked, this, [this, id]() {
        SetPage( id );
    } );

    return btn;
}

void OpsWidget::Attach( const QString& id, QWidget* page )
{
    if ( ! page || Pages.contains( id ) )
        return;

    int idx = Stack->indexOf( page );
    if ( idx < 0 )
        idx = Stack->addWidget( page );

    Pages.insert( id, idx );
}

void OpsWidget::SetPage( const QString& id )
{
    if ( ! Pages.contains( id ) )
        return;

    Stack->setCurrentIndex( Pages.value( id ) );

    if ( auto* btn = NavButtons.value( id, nullptr ) )
        btn->setChecked( true );

    emit pageChanged( id );
}
