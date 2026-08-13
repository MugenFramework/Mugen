#ifndef MUGEN_OPSWIDGET_HPP
#define MUGEN_OPSWIDGET_HPP

#include <global.hpp>
#include <QStackedWidget>
#include <QPushButton>
#include <QButtonGroup>
#include <QMap>

class MugenNamespace::UserInterface::Widgets::OpsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OpsWidget( QWidget* parent = nullptr );

    void setupUi();
    void Attach( const QString& id, QWidget* page );
    void SetPage( const QString& id );

signals:
    void pageChanged( const QString& id );

private:
    QWidget*        NavBar  = nullptr;
    QStackedWidget* Stack   = nullptr;
    QButtonGroup*   Buttons = nullptr;
    QMap<QString, int>          Pages;
    QMap<QString, QPushButton*> NavButtons;

    QPushButton* addNavButton( const QString& id, const QString& label );
};

#endif
