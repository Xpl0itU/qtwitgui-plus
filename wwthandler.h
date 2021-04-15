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

#ifndef WWTHANDLER_H
#define WWTHANDLER_H

#include "includes.h"

enum
{
    wwtNoJob = 2000,
    wwtGetVersion,
    wwtFind,
    wwtFind_long,
    wwtAdd,
    wwtRemove,
    wwtFormat
};

class WwtHandler : public QObject
{
    Q_OBJECT

public:
    WwtHandler( QObject *parent = 0, bool root = false );
    ~WwtHandler();

    void SetRunAsRoot( bool root = true );
    void GetPartitions( bool verbose = false );

    //block the current thread and wait for wit to finish
    bool Wait( int msecs = 30000 );
    void Kill();

	void RunJob( QStringList args, int jobType );

    //static functions to mirror the ones in the WitHandler
    static bool ReadVersion();
    static bool VersionIsOk();
    static QString GetVersionString();

private:
    QProcess *process;
    int wwtJob;

    QString errStr;
    QString stdStr;

    QString currJobText;

    bool runAsRoot;
    bool requestedPassword;

    QString GetWwtPath();

signals:
    void KillProcess();
	void SendFatalErr( const QString &, int );
    //void SendStdErr( QString );
	void SendStdOut( const QString & );
    void SendProgress( int );
	void SendMessageForStatusBar( const QString & );
	void SendPartitionList( const QStringList & );
    void RequestPassword();
    void SendJobDone( int );

public slots:
    void PasswordIsEntered();

private slots:
    void ReadyReadStdOutSlot();
    void ReadyReadStdErrSlot();
    void ProcessFinishedSlot( int, QProcess::ExitStatus );

};



#endif // WWTHANDLER_H
