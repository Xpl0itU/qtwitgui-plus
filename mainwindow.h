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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "includes.h"

#include "wiitdbwindow.h"
#include "custommdiitem.h"
#include "covermanagerwindow.h"


namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow( QWidget *parent = 0 );
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    int pFlowDirection;
    int pFlowDelay;
    bool alreadyAskingForPassword;

    QMap<QString, QList<QTreeWidgetItem *> > gameMap;//holds partition name, games
    QList<QTreeWidgetItem *> partList;		    //holds partition paths and their settings
    QList<QTreeWidgetItem *> ReadPartitionSettings();

    //keep track of the wiitdb subwindow
    bool wiiTDBisOpen;
    WiiTDBWindow *wiiTDBwindow;
    CustomMdiItem *subWiiTDB;
    void CreateWiiTDBSubWindow();

    //keep track of the coverflow subwindow
    bool pFlowIsOpen;
    CoverManagerWindow *pFlow;
    CustomMdiItem *subPFlow;
    void CreatePFlowSubWindow();

    //functions for detecting which type of mdi window is active
    CustomMdiItem *findMdiChild( const QString &name, int type );
    CustomMdiItem *CurrentMdiSubWindow();
    int CurrentMidSubwindowType();

    //logwindow stuff
    bool logIsOpen;
    void ShowLogWindow();

    void LoadSettings();
    void SaveSettings();

    void DisableEnablePartitionWindows();
    void CheckWit();


private slots:
    void on_actionLog_triggered(bool checked);
    void on_actionOpen_DVD_r_triggered();
    void on_actionFormat_WBFS_triggered();
    void on_actionRefresh_Current_Window_triggered();
    void on_actionSave_As_triggered();
    void on_actionSave_triggered();
    void on_actionOpenGame_triggered();
    void on_actionCovers_triggered(bool checked);
    void on_actionWiiTDB_triggered(bool checked);
    void on_actionSettings_triggered();
    void on_actionOpen_Partition_triggered();
    void on_actionAbout_triggered();

	void MdiItemDestroyed( const QString &name, const QPoint &pos, const QSize &size, int type );

	void ReceiveListFor_1_Partition( const QString &, const QList<QTreeWidgetItem *> & );
    void RecieveUpdatedPartitionInfo( QTreeWidgetItem * );
	void ReceiveAllPartitionInfo( const QList<QTreeWidgetItem *> & );
	void OpenSelectedPartitions( const QList<QTreeWidgetItem *> &list );
    void NeedToAskForPassword();
	void OpenGames( const QStringList & );
	void ReactToInvalidPartionReport( const QString &part );
	void ReceiveDirtyPartition( const QString &part );

    void on_menuFile_aboutToShow();
    void on_menuView_aboutToShow();



protected:
    void closeEvent( QCloseEvent * closeEvent );
    void dropEvent( QDropEvent *event );
    void dragEnterEvent( QDragEnterEvent *event );

signals:
    void TellOpenWindowsThatTheSettingsAreChanged();
	void SendNewPartitionListToSubWindows( const QList<QTreeWidgetItem *> & );
    void UserEnteredPassword();
	void SendListsToCoverManager( const QMap<QString, QList<QTreeWidgetItem *> > & );//send all the current gamelists and partition names to the covermanager
};

#endif // MAINWINDOW_H
