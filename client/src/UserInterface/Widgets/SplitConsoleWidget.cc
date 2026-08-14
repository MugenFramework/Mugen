#include <global.hpp>
#include <UserInterface/Widgets/SplitConsoleWidget.hpp>
#include <UserInterface/Widgets/TeamserverTabSession.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QSplitter>

using namespace MugenNamespace::UserInterface::Widgets;
using namespace MugenNamespace::Util;

static QLabel* makeSessionLabel( const SessionItem& session )
{
    auto* label = new QLabel( session.ConsoleTabTitle() );
    label->setStyleSheet(
        "background:#111118; color:#ff6b9d; font-family:monospace; font-size:11px;"
        "padding:3px 8px; border-bottom:1px solid #2a2a3f;"
    );
    return label;
}

static QWidget* makePanel( DemonInteracted* console, const SessionItem& session )
{
    auto* wrapper = new QWidget();
    auto* layout  = new QVBoxLayout( wrapper );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->setSpacing( 0 );
    layout->addWidget( makeSessionLabel( session ) );
    layout->addWidget( console->DemonInteractedWidget );
    console->DemonInteractedWidget->show();
    return wrapper;
}

SplitConsoleWidget::~SplitConsoleWidget()
{
    cleanup();
}

void SplitConsoleWidget::setupUi( SessionItem& left, SessionItem& right )
{
    leftConsole  = left.InteractedWidget;
    rightConsole = right.InteractedWidget;
    if ( ! leftConsole || ! leftConsole->DemonInteractedWidget ||
         ! rightConsole || ! rightConsole->DemonInteractedWidget )
        return;

    auto* mainLayout = new QVBoxLayout( this );
    mainLayout->setContentsMargins( 0, 0, 0, 0 );
    mainLayout->setSpacing( 0 );

    auto* splitter = new QSplitter( Qt::Horizontal, this );
    splitter->setHandleWidth( 3 );
    splitter->addWidget( makePanel( leftConsole, left ) );
    splitter->addWidget( makePanel( rightConsole, right ) );
    splitter->setSizes( { 1, 1 } );

    mainLayout->addWidget( splitter );
}

void SplitConsoleWidget::park( DemonInteracted* panel )
{
    if ( ! panel || ! panel->DemonInteractedWidget )
        return;

    auto* w = panel->DemonInteractedWidget;
    if ( w->parent() != this && ! isAncestorOf( w ) )
        return;

    w->hide();
    auto* host = MugenX::Teamserver.TabSession;
    w->setParent( host ? static_cast<QWidget*>( host ) : nullptr );
}

void SplitConsoleWidget::cleanup()
{
    park( leftConsole );
    park( rightConsole );
}
