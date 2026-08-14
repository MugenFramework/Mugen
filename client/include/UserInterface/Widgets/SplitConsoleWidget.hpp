#ifndef MUGEN_SPLITCONSOLEWIDGET_HPP
#define MUGEN_SPLITCONSOLEWIDGET_HPP

#include <global.hpp>
#include <UserInterface/Widgets/DemonInteracted.h>

namespace MugenNamespace::UserInterface::Widgets
{
    class SplitConsoleWidget : public QWidget
    {
    public:
        DemonInteracted* leftConsole  = nullptr;
        DemonInteracted* rightConsole = nullptr;

        ~SplitConsoleWidget() override;

        void setupUi( Util::SessionItem& left, Util::SessionItem& right );
        void cleanup();

        bool hosts( const DemonInteracted* console ) const
        {
            return console && ( console == leftConsole || console == rightConsole );
        }

        DemonInteracted* otherOf( const DemonInteracted* console ) const
        {
            if ( console == leftConsole )  return rightConsole;
            if ( console == rightConsole ) return leftConsole;
            return nullptr;
        }

    private:
        void park( DemonInteracted* panel );
    };
}

#endif
