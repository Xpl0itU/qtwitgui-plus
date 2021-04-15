/************************************************************************************
*
*   - QtWitGui -				2010 giantpune
*
*   the multilingual, multiplatform, multiformat gui for messing with
*   Wii game images.
*
*   This software comes to you with a GPLv3 license.
*   http://www.gnu.org/licenses/gpl-3.0.html
*
*   Basically you are free to modify this code, distribute it, use it in
*   other projects, or anything along those lines.  Just make sure that any
*   derivative work gets the same license.  And don't remove this notice from
*   the derivative work.
*
*   And please, don't be a douche.  If you borrow code from here, don't claim
*   you wrote it.  Share your source code with others.  Even if you are
*   charging a fee for the binaries, let others read the code as somebody has
*   done for you.
*
*************************************************************************************/

#ifndef PARTITIONWINDOW_H
#define PARTITIONWINDOW_H

#include "includes.h"
#include "withandler.h"
#include "wwthandler.h"
#include "gc_shrinkthread.h"

namespace Ui {
    class PartitionWindow;
}

class PartitionWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit PartitionWindow( QWidget *parent = 0 );
    ~PartitionWindow();

    //set the window title and path of this window
    void SetPartition( QTreeWidgetItem *part );
	void SetPartitionAndGameList( QTreeWidgetItem * part, const QList<QTreeWidgetItem *> &gameList );

private:
    Ui::PartitionWindow *ui;
    QTreeWidgetItem *partition;
    WitHandler wit;
    WwtHandler wwt;
    bool ignoreFst;

    bool writingToWBFS;//keep track of a WBFS partition if we are writing to it so we can enable it when done
    QString busyWBFSPath;
	void SetPartitionEnabled( const QString &part, bool enabled );

    QList<QTreeWidgetItem *> partList;//holds a copy of the partition list

    //stuff for aligning gamecube games
    GC_ShrinkThread *gcGame;
    int gcTotalgames;
    bool shrinkingGc;
    bool gcSingleFile;
    QStringList gcGameList;
    QString gcDestination;

    QString dirtyPartition;//remember which partition we are writing to. on successful write, tell the main window that the gamelist needs to be reloaded

    bool needToReload;
    bool busy;

    void UpdateFlagText();


private slots:
    void on_actionRefresh_List_triggered();
	void HandleWiimmsErrors( const QString &err, int id );
	void GetPartitionInfo( const QList<QTreeWidgetItem *> &games, const QString &MibUsed );
    void TreeSelectionChanged( QTreeWidgetItem * current, QTreeWidgetItem * previous );
    void CustomTreeWidgetContentmenu( const QPoint& pos );
    void HideProgressBar( int job );
    void NeedToAskForPassword();
	void GetStatusTextFromWiimms( const QString &text );
    void ShrinkNextGame();

public slots:
    void SettingsHaveChanged();
	void SetPartitionList( const QList<QTreeWidgetItem *> &pList );
    void GetPasswordFromMainWindow();

protected:
    void closeEvent( QCloseEvent * closeEvent );

signals:
	void SendGamelistFor_1_Partition( const QString &, const QList<QTreeWidgetItem *> & );
    void UserEnteredPassword();
	void GameClicked( const QString &id );
    void SendUpdatedPartitionInfo( QTreeWidgetItem * );
    void AskMainWindowForPassword();
	void BrowseGames( const QStringList & );
	void ReportInvalidPartition( const QString & );
	void PartitionIsDirty( const QString & );
};

#endif // PARTITIONWINDOW_H
