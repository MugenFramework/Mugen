#ifndef MUGEN_DBMANAGER_HPP
#define MUGEN_DBMANAGER_HPP

#include <global.hpp>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

using namespace std;

class MugenNamespace::MugenSpace::DBManager
{
private:
    QSqlDatabase DB;

    bool createNewDatabase();

public:
    static string DBFilePath;

    static int OpenSqlFile;
    static int CreateSqlFile;

    DBManager(const QString& FilePath, int OpenFlag = OpenSqlFile);

    bool addTeamserverInfo( const Util::ConnectionInfo& );
    bool checkTeamserverExists( const QString& ProfileName );
    bool removeTeamserverInfo( const QString& ProfileName );
    bool removeAllTeamservers();
    vector<Util::ConnectionInfo> listTeamservers();

    bool AddScript( QString Path );
    bool RemoveScript( QString Path );
    bool CheckScript( QString Path );
    vector<QString> GetScripts();

    struct CredentialEntry {
        QString AgentID;
        QString Type;
        QString Username;
        QString Secret;
        QString Domain;
        QString Source;
        QString Timestamp;
    };

    bool AddCredential( const CredentialEntry& entry );
    vector<CredentialEntry> GetCredentials();
    bool DeleteCredential( const QString& AgentID, const QString& Username, const QString& Timestamp );

    bool SetSessionMeta( const QString& AgentID, const QString& Tags, const QString& Notes );
    bool GetSessionMeta( const QString& AgentID, QString& Tags, QString& Notes );
};

#endif