#ifndef MUGEN_RESOURCEMANAGERWIDGET_HPP
#define MUGEN_RESOURCEMANAGERWIDGET_HPP

#include <global.hpp>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QJsonArray>

class MugenNamespace::UserInterface::Widgets::ResourceManagerWidget : public QWidget
{
    Q_OBJECT

public:
    QWidget*      ResourceView  = nullptr;
    QTableWidget* ResourceTable = nullptr;
    QPushButton*  UploadButton   = nullptr;
    QPushButton*  DeleteButton   = nullptr;
    QPushButton*  DownloadButton = nullptr;
    QLineEdit*    SearchBar      = nullptr;

    QString TeamserverName;
    QString Username;

    explicit ResourceManagerWidget( QWidget* parent = nullptr );
    void setupUi( QWidget* widget );
    void Refresh( const QString& resourcesJson );

private:
    QJsonArray m_entries;

    void rebuildTable( const QJsonArray& entries );
    void applyFilter( const QString& query );
};

#endif
