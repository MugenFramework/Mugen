#ifndef MUGEN_STAGELESSDIALOG_H
#define MUGEN_STAGELESSDIALOG_H

#include <global.hpp>

#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>

#include <QCheckBox>
#include <QGroupBox>
#include <QComboBox>
#include <QTextEdit>
#include <QThread>
#include <QMetaObject>
#include <QVariant>
#include <QFile>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QClipboard>
#include <QApplication>

class Payload : public QDialog
{
    bool            Closed = false;
public:
    QDialog*        PayloadDialog;

    QGridLayout*    gridLayout;
    QGridLayout*    gridLayout_2;
    QGridLayout*    gridLayout_3;

    QGroupBox*      OptionsBox;
    QGroupBox*      BuildConsoleBox;

    QTextEdit*      ConsoleText;

    QComboBox*      ComboAgentType;
    QComboBox*      ComboListener;
    QComboBox*      ComboFormat;
    QComboBox*      ComboArch;

    QTreeWidget*    TreeConfig;

    QLabel*         LabelListener;
    QLabel*         LabelArch;
    QLabel*         LabelFormat;
    QLabel*         LabelAgentType;

    QPushButton*    ButtonGenerate;
    QLineEdit*      OnelineEdit  = nullptr;
    QPushButton*    CopyButton   = nullptr;

    QSpacerItem*    horizontalSpacer;
    QSpacerItem*    horizontalSpacer_2;
    QSpacerItem*    horizontalSpacer_3;
    QSpacerItem*    horizontalSpacer_4;
    QSpacerItem*    horizontalSpacer_5;
    QSpacerItem*    horizontalSpacer_6;
    QSpacerItem*    horizontalSpacer_7;

    QString         TeamserverName;

    auto setupUi( QDialog* StagelessDialog ) -> void;
    auto retranslateUi() -> void;
    auto Start() -> void;

    auto Clear() -> void;
    auto ReceivedImplantAndSave( QString Format, QByteArray ImplantArray )  -> void;

    auto AddConfigFromJson( QJsonDocument JsonConfig ) -> void;
    auto DefaultConfig() -> void;
    auto TenguDefaultConfig() -> void;

    auto GetConfigAsJson() -> QJsonDocument;

private:
    auto buildOneliner() -> QString;
    auto updateOneliner() -> void;

public slots:
    auto buttonGenerate() -> void;
    auto addConsoleLog( QString MsgType, QString Message ) -> void;
    auto CtxAgentPayloadChange( const QString& AgentType ) -> void;
};


#endif
