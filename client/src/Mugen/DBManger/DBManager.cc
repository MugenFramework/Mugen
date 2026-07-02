#include <Mugen/DBManager/DBManager.hpp>
#include <QFileInfo>

using namespace MugenNamespace::MugenSpace;

int DBManager::OpenSqlFile = 1;
int DBManager::CreateSqlFile = 2;

DBManager::DBManager( const QString& FilePath, int OpenFlag )
{
    auto exists = false;
    if ( QFileInfo::exists( FilePath ) ) {
        exists = true;
    }

    this->DB = QSqlDatabase::addDatabase( "QSQLITE" );
    this->DB.setDatabaseName( FilePath );

    if ( this->DB.open() ) {
        if ( OpenFlag == DBManager::CreateSqlFile && ! exists ) {
            if ( this->createNewDatabase() ) {
                spdlog::info( "Successful created database" );
            } else {
                spdlog::error( "Failed to create a new database" );
            }
        } else if ( OpenFlag == DBManager::CreateSqlFile && exists ) {
            // Migrate existing databases by creating new tables if they don't exist yet.
            auto q = QSqlQuery();
            q.exec(
                "CREATE TABLE IF NOT EXISTS \"Credentials\" ( "
                "\"ID\" INTEGER PRIMARY KEY, \"AgentID\" TEXT, \"Type\" TEXT, "
                "\"Username\" TEXT, \"Secret\" TEXT, \"Domain\" TEXT, \"Source\" TEXT, \"Timestamp\" TEXT );"
            );
            q.exec(
                "CREATE TABLE IF NOT EXISTS \"SessionMeta\" ( "
                "\"AgentID\" TEXT PRIMARY KEY, \"Tags\" TEXT, \"Notes\" TEXT );"
            );
        }
    } else {
        spdlog::error( "[DB] Failed to open database" );
    }
}

bool DBManager::createNewDatabase()
{
    auto query = QSqlQuery();
    auto error = std::string();

    /* check if the db file is opened */
    if ( ! DB.isOpen() ) {
        return false;
    }

    query.prepare(
        "CREATE TABLE \"Teamservers\" ( "
        "\"ID\" INTEGER PRIMARY KEY, "
        "\"ProfileName\" TEXT, "
        "\"Host\" TEXT, "
        "\"Port\" INTEGER, "
        "\"User\" TEXT, "
        "\"Password\" TEXT "
        ");"
    );
    if ( ! query.exec() ) {
        error = query.lastError().text().toStdString();
        spdlog::error( "[DB] Couldn't create Teamserver table: ", error );
    }

    query.prepare(
        "CREATE TABLE \"Scripts\" ( "
        "\"ID\" INTEGER PRIMARY KEY, "
        "\"Path\" TEXT "
        ");"
    );

    if ( ! query.exec() ) {
        error = query.lastError().text().toStdString();
        spdlog::error( "[DB] Couldn't create Scripts table: ", error );
        return false;
    }

    query.prepare(
        "CREATE TABLE IF NOT EXISTS \"Credentials\" ( "
        "\"ID\" INTEGER PRIMARY KEY, "
        "\"AgentID\" TEXT, "
        "\"Type\" TEXT, "
        "\"Username\" TEXT, "
        "\"Secret\" TEXT, "
        "\"Domain\" TEXT, "
        "\"Source\" TEXT, "
        "\"Timestamp\" TEXT "
        ");"
    );

    if ( ! query.exec() ) {
        error = query.lastError().text().toStdString();
        spdlog::error( "[DB] Couldn't create Credentials table: {}", error );
    }

    query.prepare(
        "CREATE TABLE IF NOT EXISTS \"SessionMeta\" ( "
        "\"AgentID\" TEXT PRIMARY KEY, "
        "\"Tags\" TEXT, "
        "\"Notes\" TEXT "
        ");"
    );

    if ( ! query.exec() ) {
        error = query.lastError().text().toStdString();
        spdlog::error( "[DB] Couldn't create SessionMeta table: {}", error );
    }

    return true;
}