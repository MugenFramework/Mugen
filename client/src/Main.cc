#include <global.hpp>
#include <Mugen/Mugen.hpp>
#include <QTimer>

auto main(
    int    argc,
    char** argv
) -> int {
    auto MugenApp = QApplication( argc, argv );
    auto Status   = 0;

    QGuiApplication::setWindowIcon( QIcon( ":/Mugen.ico" ) );

    MugenNamespace::MugenApplication = new MugenNamespace::MugenSpace::Mugen( new QMainWindow );
    MugenNamespace::MugenApplication->Init( argc, argv );

    Status = QApplication::exec();

    spdlog::info( "Mugen Application status: {}", Status );

    return Status;
}
