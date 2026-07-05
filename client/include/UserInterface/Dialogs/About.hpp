#ifndef MUGEN_ABOUTDIALOG_H
#define MUGEN_ABOUTDIALOG_H

#include <global.hpp>

class About : public QDialog
{
private:
    QPushButton* pushButton;

public:
    QDialog* AboutDialog;

    void setupUi();
    About( QDialog* );

public slots:
    void onButtonClose();
};

#endif
