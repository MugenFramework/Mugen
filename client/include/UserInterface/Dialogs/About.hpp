#ifndef MUGEN_ABOUTDIALOG_H
#define MUGEN_ABOUTDIALOG_H

#include <global.hpp>
#include <QTextBrowser>

class About : public QDialog
{
private:
    QGridLayout*    gridLayout;
    QLabel*         label;
    QPushButton*    pushButton;
    QSpacerItem*    horizontalSpacer;
    QTextBrowser*   textBrowser;

public:
    QDialog *AboutDialog;

    void setupUi();
    About( QDialog* );

public slots:
    void onButtonClose();
};

#endif