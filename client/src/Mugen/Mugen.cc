#include <Mugen/Mugen.hpp>
#include <Mugen/Connector.hpp>
#include <Mugen/CmdLine.hpp>

#include <QTimer>

MugenSpace::Mugen::Mugen( QMainWindow* w )
{
    w->setVisible( false );

    spdlog::set_pattern( "[%T] [%^%l%$] %v" );
    spdlog::info(
        "Mugen Framework [Version: {}] [CodeName: {}]",
        MugenNamespace::Version,
        MugenNamespace::CodeName
    );

    this->MugenMainWindow = w;
    this->dbManager = new MugenSpace::DBManager( "data/client.db", DBManager::CreateSqlFile );
}

void MugenSpace::Mugen::Init( int argc, char** argv )
{
    auto List      = std::vector<Util::ConnectionInfo>();
    auto Connect   = new MugenNamespace::UserInterface::Dialogs::Connect;
    auto Arguments = cmdline::parser();
    auto Path      = std::string();

    Arguments.add( "debug",  '\0', "debug mode" );
    Arguments.add( "config", '\0', "toml config path" );
    Arguments.parse_check( argc, argv );


    if ( Arguments.exist( "debug" ) ) {
        spdlog::set_level( spdlog::level::debug );
        spdlog::debug( "Debug mode enabled" );
    }

    if ( Arguments.exist( "config" ) ) {
        Path = Arguments.get<std::string>( "config" );

        if ( ! QFile::exists( Path.c_str() ) ) {
            Path = std::string();
        }
    }

    if ( Path.empty() ) {
        Path = "client/config.toml";
    }

    if ( ! QFile::exists( Path.c_str() ) ) {
        Path = "config.toml";

        if ( ! QFile::exists( Path.c_str() ) ) {
            spdlog::error( "couldn't find config file" );
            Exit();
        }
    }

    Config = toml::parse( Path );
    spdlog::info( "loaded config file: {}", Path );

    /* TODO: handle any kind of error */
    const auto& font   = toml::find( Config, "font" );
    const auto  family = toml::find<std::string>( font, "family" );
    const auto  size   = toml::find<int>( font, "size" );

    QTextCodec::setCodecForLocale( QTextCodec::codecForName( "UTF-8" ) );
    QApplication::setFont( QFont( family.c_str(), size ) );
        QTimer::singleShot( 10, [&]() {
        QApplication::setFont( QFont( family.c_str(), size ) );
    } );

    this->MugenMainWindow->setVisible( false );

    Connect->TeamserverList = dbManager->listTeamservers();
    Connect->passDB( this->dbManager );
    Connect->setupUi( new QDialog );

    MugenX::Teamserver = Connect->StartDialog( false );

    delete Connect;
}

void MugenSpace::Mugen::Start()
{
    this->ClientInitConnect = false;
    this->MugenMainWindow->setVisible( true );
    this->MugenMainWindow->setCentralWidget( this->MugenAppUI.centralwidget );
    this->MugenMainWindow->show();
}

void MugenSpace::Mugen::Exit()
{
    spdlog::critical( "Exit Program" );
    MugenApplication->MugenMainWindow->close();

    exit( 0 );
}

Mugen::~Mugen()
{
    delete this->dbManager;
    delete this->MugenMainWindow;
}
