#ifndef MUGEN_NETWORKINGWIDGET_HPP
#define MUGEN_NETWORKINGWIDGET_HPP

#include <global.hpp>
#include <QTableWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>

class MugenNamespace::UserInterface::Widgets::NetworkingWidget : public QWidget
{
    Q_OBJECT

public:
    QWidget*      NetworkingView  = nullptr;
    QTabWidget*   Tabs            = nullptr;
    QTableWidget* Socks5Table     = nullptr;
    QTableWidget* RportfwdTable   = nullptr;
    QPushButton*  StopButton      = nullptr;
    QPushButton*  RemoveButton    = nullptr;

    QString TeamserverName;

    explicit NetworkingWidget( QWidget* parent = nullptr );
    void setupUi( QWidget* widget );
    void Refresh();

private:
    void rebuildSocks5Table();
    void rebuildRportfwdTable();
};

#endif
