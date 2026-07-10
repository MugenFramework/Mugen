#ifndef MUGEN_SPLITCONSOLEWIDGET_HPP
#define MUGEN_SPLITCONSOLEWIDGET_HPP

#include <global.hpp>
#include <UserInterface/Widgets/DemonInteracted.h>
#include <QSplitter>

namespace MugenNamespace::UserInterface::Widgets
{
    class SplitConsoleWidget : public QWidget
    {
    public:
        DemonInteracted* leftConsole  = nullptr;
        DemonInteracted* rightConsole = nullptr;

        void setupUi( Util::SessionItem& left, Util::SessionItem& right, const QString& teamserver );
        void cleanup();
    };
}

#endif
