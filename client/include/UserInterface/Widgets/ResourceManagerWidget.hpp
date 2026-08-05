#ifndef MUGEN_RESOURCEMANAGERWIDGET_HPP
#define MUGEN_RESOURCEMANAGERWIDGET_HPP

#include <global.hpp>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QFrame>
#include <QJsonArray>
#include <QJsonObject>

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
    QFrame*       DetailPanel    = nullptr;

    // detail panel labels
    QLabel*       dl_name        = nullptr;
    QLabel*       dl_kind        = nullptr;
    QLabel*       dl_size        = nullptr;
    QLabel*       dl_uploader    = nullptr;
    QLabel*       dl_added       = nullptr;
    QLabel*       dl_hash        = nullptr;
    QLabel*       dl_path        = nullptr;
    QPushButton*  dl_copyHash    = nullptr;

    QString TeamserverName;
    QString Username;

    explicit ResourceManagerWidget( QWidget* parent = nullptr );
    void setupUi( QWidget* widget );
    void Refresh( const QString& resourcesJson );

private:
    QJsonArray m_entries;

    void rebuildTable( const QJsonArray& entries );
    void applyFilter( const QString& query );
    void showDetail( const QJsonObject& obj );
    void hideDetail();
};

#endif
