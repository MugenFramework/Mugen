#ifndef MUGEN_DEMONINTERACTED_H
#define MUGEN_DEMONINTERACTED_H

#include <global.hpp>
#include <Mugen/DemonCmdDispatch.h>
#include <Mugen/Tengu/TenguCmdDispatch.h>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QJsonArray>
#include <QToolButton>
#include <functional>

// ── ConsoleTextEdit ─────────────────────────────────────────────────────────
// QTextEdit subclass that overlays a ⋯ button per task block.
// No Q_OBJECT: signals replaced by std::function callbacks.
class ConsoleTextEdit : public QTextEdit
{
public:
    struct TaskBlock {
        int     startBlock; // block number in document
        QString taskID;
        QString comment;
    };

    explicit ConsoleTextEdit(QWidget* parent = nullptr);

    // Call just before appending the task prompt line
    void beginTask(const QString& taskID);

    // Clear all task tracking and console content (called on history reload)
    void resetBlocks();

    // Update stored comment for a task (after server confirms)
    void updateComment(const QString& taskID, const QString& comment);

    // Callbacks set by DemonInteracted
    std::function<void()>                                    onSearch;
    std::function<void(const QString& taskID, const QString& current)> onComment;
    std::function<void(const QString& taskID)>               onDelete;

    const QVector<TaskBlock>& blocks() const { return taskBlocks; }

protected:
    void mouseMoveEvent(QMouseEvent* e) override;
    void leaveEvent(QEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;

private:
    QVector<TaskBlock> taskBlocks;
    QToolButton*       menuBtn     = nullptr;
    int                hoveredTask = -1;

    int  taskAtBlock(int blockNumber) const;
    void repositionBtn(int taskIdx);
    void showMenu(int taskIdx);
};

// ── DemonInteracted ──────────────────────────────────────────────────────────
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
        ConsoleTextEdit*            Console;
        QCompleter*                 CommandCompleter;
        QStringList                 CompleterCommands;
        QString                     AgentTypeName   = "Demon";
        bool                        HistoryFetched  = false;

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

        // secondary consoles that mirror this console's output (split view)
        QList<QTextEdit*> MirrorConsoles;
        void AddMirror( QTextEdit* c );
        void RemoveMirror( QTextEdit* c );

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
        void replayHistory( const QJsonArray& tasks );

        QString TaskInfo( bool Show, QString TaskID, const QString& text ) const;
        QString TaskError( const QString& text ) const;

        void AutoCompleteAdd( QString text );
        void AutoCompleteAddList( QStringList list );
        void AutoCompleteClear();

    protected:
        void showEvent( QShowEvent* e ) override;

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
