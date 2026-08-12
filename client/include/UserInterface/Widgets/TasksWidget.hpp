#ifndef MUGEN_TASKSWIDGET_HPP
#define MUGEN_TASKSWIDGET_HPP

#include <global.hpp>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QJsonArray>
#include <QJsonObject>

class MugenNamespace::UserInterface::Widgets::TasksWidget : public QWidget
{
    Q_OBJECT

public:
    QWidget*      TasksView   = nullptr;
    QTableWidget* TasksTable  = nullptr;
    QLineEdit*    SearchBar   = nullptr;
    QComboBox*    FilterCombo = nullptr;

    QString TeamserverName;

    explicit TasksWidget( QWidget* parent = nullptr );
    void setupUi( QWidget* widget );
    void LoadSnapshot( const QString& tasksJson );
    void UpsertTask( const QJsonObject& task );
    void RefreshDurations();
    void RequestSnapshot();

private:
    QJsonArray m_tasks;

    void rebuildTable();
    bool rowVisible( const QJsonObject& task, const QString& query, const QString& filter ) const;
    QString durationText( const QJsonObject& task ) const;
};

#endif
