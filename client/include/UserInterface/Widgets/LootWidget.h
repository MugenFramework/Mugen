#ifndef MUGEN_LOOTWIDGET_H
#define MUGEN_LOOTWIDGET_H

#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QWidget>
#include <QLabel>
#include <QScrollArea>
#include <QPushButton>
#include <Mugen/DBManager/DBManager.hpp>

class ImageLabel : public QWidget
{
public:
    QLabel*      label;
    QScrollArea* scrollArea;
    bool         key_ctrl = false;

    explicit ImageLabel(QWidget *parent = 0);
    const QPixmap* pixmap() const;

public slots:
    void setPixmap(const QPixmap&);

protected:
    void resizeEvent(QResizeEvent *);
    void keyReleaseEvent( QKeyEvent* event );
    bool event(QEvent *) override;
    void wheelEvent(QWheelEvent *ev);

public slots:
    void resizeImage();

};

class LootWidget : public QWidget
{
public:
    enum {
        LOOT_IMAGE,
        LOOT_FILE,
        LOOT_CREDENTIAL,
    };

    typedef struct
    {
        int     Type;
        QString AgentID;

        struct
        {

        } File;

        struct
        {
            QString     Name;
            QString     Date;
            QString     Size;
            QByteArray  Data;
        } Data;

        struct
        {
            QString CredType;
            QString Username;
            QString Secret;
            QString Domain;
            QString Source;
            QString Timestamp;
        } Cred;

    } LootData;
    std::vector<LootData> LootItems;

    QGridLayout*    gridLayout;

    QLabel*         LabelShow;
    QLabel*         LabelAgentID;

    QComboBox*      ComboShow;
    QComboBox*      ComboAgentID;

    QTableWidget*   ScreenshotTable;
    QTableWidget*   DownloadTable;

    QSpacerItem*    horizontalSpacer;
    QStackedWidget* StackWidget;
    QWidget*        Screenshots;
    QGridLayout*    gridLayout_2;
    QSplitter*      splitter;
    ImageLabel*     ScreenshotImage;
    QWidget*        Downloads;
    QGridLayout*    gridLayout_3;

    QWidget*        Credentials;
    QGridLayout*    gridLayout_4;
    QTableWidget*   CredentialTable;

    QWidget*        CredentialBar;
    QPushButton*    BtnAddCredential;
    QPushButton*    BtnEditCredential;
    QPushButton*    BtnRemoveCredential;
    QWidget*        FileBar;
    QPushButton*    BtnDownloadFile;
    QPushButton*    BtnDeleteFile;
    QSpacerItem*    horizontalSpacer_2;

    LootWidget();
    void Reload();

    void AddSessionSection( const QString& DemonID );
    void AddScreenshot( const QString& DemonID, const QString& Name, const QString& Date, const QByteArray& Data );
    void AddDownload( const QString &DemonID, const QString &Name, const QString& Size, const QString &Date, const QByteArray &Data );
    void AddText( const QString& DemonID, const QString& Name, const QByteArray& Data );
    void AddCredential( const QString& DemonID, const QString& CredType, const QString& Username,
                        const QString& Secret, const QString& Domain, const QString& Source,
                        const QString& Timestamp );

    void ScreenshotTableAdd( const QString& AgentID, const QString& Name, const QString& Date );
    void DownloadTableAdd( const QString& AgentID, const QString& Name, const QString& Size, const QString& Date );
    void UpdateDownloadProgress( const QString& AgentID, const QString& Name, const QString& Size );
    void CredentialTableAdd( const QString& CredType, const QString& Username, const QString& Secret,
                             const QString& Domain, const QString& Source, const QString& AgentID,
                             const QString& Timestamp );
    void LoadCredentialsFromDB( MugenNamespace::MugenSpace::DBManager* db );
    void ShowKind( const QString& kind );
    void RemoveFile( const QString& agentID, const QString& name, int type );

private:
    bool agentVisible( const QString& agentID ) const;
    void refreshLootTables();
    bool hasLoot( int type, const QString& agentID, const QString& name ) const;
    bool selectedFile( QString& agentID, QString& name, int& type ) const;
    void requestFileDownload();
    void requestFileDelete();
    void updateFileButtons();
    void sendLootRequest( int subEvent, const QString& agentID, const QString& name, int type );

private Q_SLOTS:
    void onAgentChange( const QString& text );
    void onShowChange( const QString& text );
    void onScreenshotTableClick( const QModelIndex &index );
    void onScreenshotTableCtx( const QPoint &pos );
    void onDownloadTableCtx( const QPoint &pos );
    void onAddCredential();
    void onEditCredential();
    void onRemoveCredential();
    void onCredentialTableCtx( const QPoint& pos );
};


#endif
