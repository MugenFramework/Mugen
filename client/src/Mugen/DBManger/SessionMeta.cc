#include <Mugen/DBManager/DBManager.hpp>

using namespace MugenNamespace::MugenSpace;

bool DBManager::SetSessionMeta( const QString& AgentID, const QString& Tags, const QString& Notes )
{
    auto query = QSqlQuery();

    query.prepare(
        "INSERT OR REPLACE INTO SessionMeta (AgentID, Tags, Notes) "
        "VALUES (:AgentID, :Tags, :Notes)"
    );
    query.bindValue( ":AgentID", AgentID );
    query.bindValue( ":Tags",    Tags );
    query.bindValue( ":Notes",   Notes );

    if ( ! query.exec() ) {
        spdlog::error( "[DB] Failed to set session meta: {}", query.lastError().text().toStdString() );
        return false;
    }

    return true;
}

bool DBManager::GetSessionMeta( const QString& AgentID, QString& Tags, QString& Notes )
{
    auto query = QSqlQuery();

    query.prepare( "SELECT Tags, Notes FROM SessionMeta WHERE AgentID = :AgentID" );
    query.bindValue( ":AgentID", AgentID );

    if ( ! query.exec() ) {
        spdlog::error( "[DB] Failed to get session meta: {}", query.lastError().text().toStdString() );
        return false;
    }

    if ( query.next() ) {
        Tags  = query.value( "Tags"  ).toString();
        Notes = query.value( "Notes" ).toString();
        return true;
    }

    return false;
}
