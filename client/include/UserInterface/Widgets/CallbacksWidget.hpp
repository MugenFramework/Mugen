#ifndef MUGEN_CALLBACKSWIDGET_HPP
#define MUGEN_CALLBACKSWIDGET_HPP

#include <global.hpp>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSplitter>
#include <QScrollArea>
#include <QLabel>
#include <QToolButton>

class MugenNamespace::UserInterface::Widgets::CallbacksWidget : public QWidget
{
    Q_OBJECT

public:
    QWidget*      CallbacksView = nullptr;
    QTableWidget* Table         = nullptr;
    QLineEdit*    SearchBar     = nullptr;
    QComboBox*    FilterCombo   = nullptr;

    explicit CallbacksWidget( QWidget* parent = nullptr );
    void setupUi( QWidget* widget );
    void Refresh();

private:
    QSplitter*   Splitter   = nullptr;
    QWidget*     DetailPane = nullptr;
    QScrollArea* DetailBody = nullptr;
    QLabel*      DetailTitle = nullptr;
    QString      DetailAgentID;
    bool         Rebuilding = false;

    void rebuildTable();
    void updateInfoIcons();
    void selectAgentRow( const QString& agentID );
    bool rowVisible( const Util::SessionItem& s, const QString& query, const QString& filter ) const;
    void sendHidden( const QString& agentID, bool hidden );
    void sendMarked( const QString& agentID, const QString& marked );
    void toggleDetails( const QString& agentID );
    void openDetails( const QString& agentID );
    void fillDetails( const Util::SessionItem& s );
    void closeDetails();
    Util::SessionItem* findSession( const QString& agentID );
};

#endif
