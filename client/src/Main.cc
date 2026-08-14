#include <global.hpp>
#include <Mugen/Mugen.hpp>
#include <QTimer>

#include <iostream>
#include <unistd.h>

static void printBanner()
{
    const bool color = isatty( STDOUT_FILENO );
    const char* pink  = color ? "\033[38;2;255;107;157m" : "";
    const char* blue  = color ? "\033[34m" : "";
    const char* reset = color ? "\033[0m" : "";

    std::cout << pink << R"(     _______           _______  _______  _
    (       )│\     /│(  ____ \(  ____ \( (    /│
    │ () () ││ )   ( ││ (    \/│ (    \/│  \  ( │
    │ ││ ││ ││ │   │ ││ │      │ (__    │   \ │ │
    │ │(_)│ ││ │   │ ││ │ ____ │  __)   │ (\ \) │
    │ │   │ ││ │   │ ││ │ \_  )│ (      │ │ \   │
    │ )   ( ││ (___) ││ (___) ││ (____/\│ )  \  │
    │/     \│(_______)(_______)(_______/│/    )_)
)" << reset
              << "  \t" << pink << "無限" << reset
              << " - " << blue << "infinite" << reset
              << " - open source, no limits\n\n"
              << std::flush;
}

auto main(
    int    argc,
    char** argv
) -> int {
    auto MugenApp = QApplication( argc, argv );
    auto Status   = 0;

    printBanner();

    QGuiApplication::setWindowIcon( QIcon( ":/Mugen.ico" ) );

    MugenNamespace::MugenApplication = new MugenNamespace::MugenSpace::Mugen( new QMainWindow );
    MugenNamespace::MugenApplication->Init( argc, argv );

    Status = QApplication::exec();

    spdlog::info( "Mugen Application status: {}", Status );

    return Status;
}
