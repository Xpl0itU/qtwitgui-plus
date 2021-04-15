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

#ifndef HDDSELECTDIALOG_H
#define HDDSELECTDIALOG_H

#include "wwthandler.h"
#include "withandler.h"
#include "includes.h"

namespace Ui {
    class HDDSelectDialog;
}

class HDDSelectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HDDSelectDialog(QWidget *parent = 0);
    ~HDDSelectDialog();

	void AddPartitionsToList( const QList<QTreeWidgetItem *> &list );

private:
    Ui::HDDSelectDialog *ui;

    WitHandler wit;
    WwtHandler wwt;
    bool oktoRequestNextLIST_LLL;

    bool alreadyAskingForPassword;

	void AddNewPartitionToList( const QString &path, const QString &source = QString() );
    QTreeWidgetItem *PartitionBeingRead();

    void DestroyProcessesAndWait();
    void RequestFsTypes();

    //paths to ignore - set in the settings dialog
    QStringList ignorePaths;
    bool ignoreFst;
    bool PathIsIgnored( const QString &path );

public slots:
    void accept();
	void HandleWiimmsErrors( const QString &err, int id );
	void GetWBFSPartitionList( const QStringList &list );
	void GetPartitionInfo( const QList<QTreeWidgetItem *> &games, const QString &MibUsed );
#ifndef Q_WS_WIN
    void NeedToAskForPassword();
#endif

signals:
	void SendHDDList( const QList<QTreeWidgetItem *> & );//send all hdds
	void SendSelectedPartition( const QList<QTreeWidgetItem *> &);//send selected hdds
	void SendGamelistFor_1_Partition( const QString &, const QList<QTreeWidgetItem *> & );
#ifndef Q_WS_WIN
    void UserEnteredPassword();
#endif

private slots:
    void on_pushButton_reScan_clicked();
    void on_pushButton_find_clicked();
    void on_pushButton_manualADD_clicked();
    void RequestNextLIST_LLLL();
    void CustomTreeWidgetContentmenu( const QPoint& pos );

protected:
    void closeEvent( QCloseEvent * closeEvent );
    void reject();
};

#endif // HDDSELECTDIALOG_H
