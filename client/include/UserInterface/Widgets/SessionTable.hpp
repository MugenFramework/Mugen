#ifndef MUGEN_SESSIONTABLE_HPP
#define MUGEN_SESSIONTABLE_HPP

#include <global.hpp>
#include <QTableWidget>
#include <QLineEdit>
#include <QVBoxLayout>

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

public:
    QTableWidget*   SessionTableWidget = nullptr;
    QLineEdit*      FilterInput        = nullptr;

    void setupUi( QWidget* widget, QString TeamserverName );
    void NewSessionItem( Util::SessionItem item ) const;
    void ChangeSessionValue( QString DemonID, int key, QString value );
    void SetHealthCell( const QString& agentId, const QString& text, const QColor& color );
    void SetSessionTags( const QString& AgentID, const QString& Tags );
    void updateRow();
    void applyFilter( const QString& query );
};

#endif
