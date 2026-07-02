#ifndef MUGEN_DEMONINTERACTED_H
#define MUGEN_DEMONINTERACTED_H

#include <global.hpp>
#include <Mugen/DemonCmdDispatch.h>
#include <Mugen/Tengu/TenguCmdDispatch.h>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>

namespace MugenNamespace::UserInterface::Widgets
{
    class DemonInteracted : public QWidget
    {
    private:
        QGridLayout* gridLayout;
        QLabel*      label;
        QLabel*      label_2;

    public:
        QWidget*                    DemonInteractedWidget;
        MugenSpace::DemonCommands*  DemonCommands;
        MugenSpace::TenguCommands*  TenguCmds  = nullptr;
        QString                     TeamserverName;
        Util::SessionItem           SessionInfo;
        QTextEdit*                  Console;
        QCompleter*                 CommandCompleter;
        QStringList                 CompleterCommands;
        QString                     AgentTypeName = "Demon";

        class DemonInput : public QLineEdit
        {
        public:
            int CommandHistoryIndex;
            QStringList CommandHistory;
            explicit DemonInput(QWidget *parent = nullptr);

            void AddCommand( const QString& Command );

        protected:
            bool event(QEvent *) override;

        private:
            bool handleKeyPress(QKeyEvent* eventKey);
            void handleTabKey();
            void handleUpKey();
            void handleDownKey();
        };
        DemonInput* lineEdit;

        // search bar (Ctrl+F)
        QWidget*    SearchBar    = nullptr;
        QLineEdit*  SearchInput  = nullptr;
        QLabel*     SearchCount  = nullptr;
        QList<QTextEdit::ExtraSelection> SearchHighlights;
        int         SearchIndex  = -1;

        void setupUi( QWidget* Form );
        void AppendText( const QString& text );
        void AppendRaw( const QString& text = "" );
        void AppendNoNL( const QString& test );

        QString TaskInfo( bool Show, QString TaskID, const QString& text ) const;
        QString TaskError( const QString& text ) const;

        void AutoCompleteAdd( QString text );
        void AutoCompleteAddList( QStringList list );
        void AutoCompleteClear();

    private slots:
        void AppendFromInput();
        void showSearch();
        void hideSearch();
        void performSearch( const QString& query );
        void searchNext();
        void searchPrev();

    };
}

#endif
