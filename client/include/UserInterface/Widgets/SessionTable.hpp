#ifndef MUGEN_SESSIONTABLE_HPP
#define MUGEN_SESSIONTABLE_HPP

#include <global.hpp>
#include <QTableWidget>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMenu>
#include <QSettings>

struct Socks5TunnelEntry {
    QString agentID;
    int     port;
};

struct RportfwdEntry {
    QString agentID;
    int     bindPort;
    QString remoteHost;
    int     remotePort;
};

class MugenNamespace::UserInterface::Widgets::SessionTable : public QWidget
{
private:
    QGridLayout*        gridLayout       = nullptr;
    QString             TeamserverName   = nullptr;

    QTableWidgetItem*   TitleAgentID     = nullptr;
    QTableWidgetItem*   TitleInternal    = nullptr;
    QTableWidgetItem*   TitleExternal    = nullptr;
    QTableWidgetItem*   TitleUser        = nullptr;
    QTableWidgetItem*   TitleComputer    = nullptr;
    QTableWidgetItem*   TitleOperating   = nullptr;
    QTableWidgetItem*   TitleProcess     = nullptr;
    QTableWidgetItem*   TitleProcessId   = nullptr;
    QTableWidgetItem*   TitleArch        = nullptr;
    QTableWidgetItem*   TitleLast        = nullptr;
    QTableWidgetItem*   TitleHealth      = nullptr;
    QTableWidgetItem*   TitleTags        = nullptr;
    QTableWidgetItem*   TitleAlias       = nullptr;

public:
    QTableWidget*   SessionTableWidget = nullptr;
    QLineEdit*      FilterInput        = nullptr;
    QPushButton*    ActionsButton      = nullptr;

    QList<Socks5TunnelEntry> Socks5Tunnels;
    QList<RportfwdEntry>     PortForwards;

    void setupUi( QWidget* widget, QString TeamserverName );
    void NewSessionItem( Util::SessionItem item ) const;
    void ChangeSessionValue( QString DemonID, int key, QString value );
    void SetHealthCell( const QString& agentId, const QString& text, const QColor& color );
    void SetSessionTags( const QString& AgentID, const QString& Tags );
    void SetSessionAlias( const QString& AgentID, const QString& Alias );
    void SetSessionMeta( const QString& AgentID, const QString& Tags, const QString& Notes );
    void SetSessionColor( const QString& AgentID, const QString& Color );
    void SetSessionHidden( const QString& AgentID, bool Hidden );
    void RemoveSessionRow( const QString& AgentID );
    void updateRow();
    void applyFilter( const QString& query );

private:
    void saveColumnLayout();
    void restoreColumnLayout();
    void addVisibleRow( const Util::SessionItem& item ) const;
};

#endif
