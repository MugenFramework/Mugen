#include <global.hpp>
#include <QTableWidget>

#include <Mugen/DBManager/DBManager.hpp>
#include <Mugen/Packager.hpp>

class MugenNamespace::UserInterface::Widgets::ListenersTable : public QWidget
{
private:
    QGridLayout *gridLayout;
    QSpacerItem *horizontalSpacer_2;
    QSpacerItem *horizontalSpacer;
    QTableWidget *tableWidget;
    MugenSpace::DBManager* dbManager;
    MugenSpace::Packager* Packager;

    QPushButton* buttonAdd;
    QPushButton* buttonEdit;
    QPushButton* buttonRemove;

public:
    QString TeamserverName;
    QWidget* ListenerWidget;

    void setupUi( QWidget* widget );
    void ButtonsInit();
    void setDBManager( MugenSpace::DBManager* dbManager );

    Util::Packager::Package CreateNewPackage( int EventID, MapStrStr ) const;

    void ListenerAdd( Util::ListenerItem item ) const;
    void ListenerEdit( Util::ListenerItem item ) const;
    void ListenerRemove( QString ListenerName ) const;
    void ListenerError( QString ListenerName, QString Error ) const;
};