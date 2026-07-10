#include <global.hpp>
#include <UserInterface/Widgets/SplitConsoleWidget.hpp>

#include <QVBoxLayout>
#include <QLabel>

using namespace MugenNamespace::UserInterface::Widgets;
using namespace MugenNamespace::Util;

void SplitConsoleWidget::setupUi( SessionItem& left, SessionItem& right, const QString& teamserver )
{
    auto* mainLayout = new QVBoxLayout( this );
    mainLayout->setContentsMargins( 0, 0, 0, 0 );
    mainLayout->setSpacing( 0 );

    auto* splitter = new QSplitter( Qt::Horizontal, this );
    splitter->setHandleWidth( 3 );

    // left panel wrapper
    auto* leftWrapper  = new QWidget();
    auto* leftLayout   = new QVBoxLayout( leftWrapper );
    leftLayout->setContentsMargins( 0, 0, 0, 0 );
    leftLayout->setSpacing( 0 );

    auto* leftLabel    = new QLabel( "[" + left.Name + "] " + left.User + "@" + left.Computer );
    leftLabel->setStyleSheet(
        "background:#111118; color:#ff6b9d; font-family:monospace; font-size:11px;"
        "padding:3px 8px; border-bottom:1px solid #2a2a3f;"
    );

    leftConsole = new DemonInteracted;
    leftConsole->SessionInfo    = left;
    leftConsole->TeamserverName = teamserver;
    leftConsole->setupUi( new QWidget );

    leftLayout->addWidget( leftLabel );
    leftLayout->addWidget( leftConsole->DemonInteractedWidget );

    // right panel wrapper
    auto* rightWrapper = new QWidget();
    auto* rightLayout  = new QVBoxLayout( rightWrapper );
    rightLayout->setContentsMargins( 0, 0, 0, 0 );
    rightLayout->setSpacing( 0 );

    auto* rightLabel   = new QLabel( "[" + right.Name + "] " + right.User + "@" + right.Computer );
    rightLabel->setStyleSheet(
        "background:#111118; color:#ff6b9d; font-family:monospace; font-size:11px;"
        "padding:3px 8px; border-bottom:1px solid #2a2a3f;"
    );

    rightConsole = new DemonInteracted;
    rightConsole->SessionInfo    = right;
    rightConsole->TeamserverName = teamserver;
    rightConsole->setupUi( new QWidget );

    rightLayout->addWidget( rightLabel );
    rightLayout->addWidget( rightConsole->DemonInteractedWidget );

    splitter->addWidget( leftWrapper );
    splitter->addWidget( rightWrapper );
    splitter->setSizes( { 1, 1 } );

    mainLayout->addWidget( splitter );

    // register mirrors: server output for each session is mirrored to the split panel
    if ( left.InteractedWidget )
        left.InteractedWidget->AddMirror( leftConsole->Console );

    if ( right.InteractedWidget )
        right.InteractedWidget->AddMirror( rightConsole->Console );
}

void SplitConsoleWidget::cleanup()
{
    for ( auto& s : MugenX::Teamserver.Sessions )
    {
        if ( leftConsole && s.Name == leftConsole->DemonCommands->DemonID )
        {
            if ( s.InteractedWidget )
                s.InteractedWidget->RemoveMirror( leftConsole->Console );
        }
        if ( rightConsole && s.Name == rightConsole->DemonCommands->DemonID )
        {
            if ( s.InteractedWidget )
                s.InteractedWidget->RemoveMirror( rightConsole->Console );
        }
    }
}
