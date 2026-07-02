#include <Mugen/DBManager/DBManager.hpp>

using namespace MugenNamespace::MugenSpace;

bool DBManager::AddCredential( const CredentialEntry& entry )
{
    auto query = QSqlQuery();

    query.prepare(
        "INSERT INTO Credentials (AgentID, Type, Username, Secret, Domain, Source, Timestamp) "
        "VALUES (:AgentID, :Type, :Username, :Secret, :Domain, :Source, :Timestamp)"
    );
    query.bindValue( ":AgentID",    entry.AgentID );
    query.bindValue( ":Type",       entry.Type );
    query.bindValue( ":Username",   entry.Username );
    query.bindValue( ":Secret",     entry.Secret );
    query.bindValue( ":Domain",     entry.Domain );
    query.bindValue( ":Source",     entry.Source );
    query.bindValue( ":Timestamp",  entry.Timestamp );

    if ( ! query.exec() ) {
        spdlog::error( "[DB] Failed to add credential: {}", query.lastError().text().toStdString() );
        return false;
    }

    return true;
}

vector<DBManager::CredentialEntry> DBManager::GetCredentials()
{
    auto list  = vector<CredentialEntry>();
    auto query = QSqlQuery();

    query.prepare( "SELECT AgentID, Type, Username, Secret, Domain, Source, Timestamp FROM Credentials" );

    if ( ! query.exec() ) {
        spdlog::error( "[DB] Failed to query credentials: {}", query.lastError().text().toStdString() );
        return list;
    }

    while ( query.next() ) {
        CredentialEntry e;
        e.AgentID   = query.value( "AgentID"   ).toString();
        e.Type      = query.value( "Type"       ).toString();
        e.Username  = query.value( "Username"   ).toString();
        e.Secret    = query.value( "Secret"     ).toString();
        e.Domain    = query.value( "Domain"     ).toString();
        e.Source    = query.value( "Source"     ).toString();
        e.Timestamp = query.value( "Timestamp"  ).toString();
        list.push_back( e );
    }

    return list;
}

bool DBManager::DeleteCredential( const QString& AgentID, const QString& Username, const QString& Timestamp )
{
    auto query = QSqlQuery();

    query.prepare( "DELETE FROM Credentials WHERE AgentID = :AgentID AND Username = :Username AND Timestamp = :Timestamp" );
    query.bindValue( ":AgentID",   AgentID );
    query.bindValue( ":Username",  Username );
    query.bindValue( ":Timestamp", Timestamp );

    if ( ! query.exec() ) {
        spdlog::error( "[DB] Failed to delete credential: {}", query.lastError().text().toStdString() );
        return false;
    }

    return true;
}
