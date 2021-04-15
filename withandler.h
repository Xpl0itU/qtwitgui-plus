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

#ifndef WITHANDLER_H
#define WITHANDLER_H

#include "includes.h"
#include "tools.h"

extern QString rootAskStr;
extern QString rootWrongStr;
extern QString rootFailStr;

enum
{
    witNoJob = 1000,
    witGetVersion,
    witCopy,
    witIlist,
    witDump,
    witEdit,
    witListLLLHDD
};

class WitHandler : public QObject
{
    Q_OBJECT


public:
    WitHandler( QObject *parent = 0, bool root = false );
    ~WitHandler();

    void SetRunAsRoot( bool root = true );
    void SetTitlesTxtPath( const QString &path );
    void ListLLL_HDD( const QString &path, int recurse = 10, bool ignoreFst = false );

    void SetNamesFromWiiTDB( bool wiitdb = true );

    //block the current thread and wait for wit to finish
    bool Wait( int msecs = 30000 );
    void Kill();


    void RunJob( QStringList args, int jobType );

    //static functions are blocking, and will halt the current thread until the process finishes
    //static functions dont respect the "root" settings
    static bool ReadAttributes();
    static bool NameSupportsAttribute( QString name, QString attr );
    static bool OptionSupportsAttribute( QString opt, QString attr );
    static bool ExtensionSupportsAttribute( QString ext, QString attr );
    static bool ReadVersion();
    static bool VersionIsOk();
    static QString GetVersionString();
    static QStringList FileType( QStringList files );
    static QString GetTitlesTxtPath();
    //static bool OptionSupports( QString option, QString attr );

private:
    QProcess *process;
    int witJob;

    QString stdStr;
    QString errStr;

    QString currJobText;

    bool runAsRoot;
    bool requestedPassword;

    bool namesFromWiiTDB;

    QString GetWitPath();

    QList<QTreeWidgetItem *> StringListToGameList( QStringList list, bool *ok );

signals:
    void KillProcess();
    void SendFatalErr( QString, int );
    void SendStdErr( QString );
    void SendStdOut( QString );
    void SendProgress( int );
    void RequestPassword();
    void SendJobDone( int );//sent when done copying & editing.  not sent ofter every joy ( yet )
    void SendMessageForStatusBar( QString );

    void SendListLLL( QList<QTreeWidgetItem *> games, QString MibUsed );

    void SendGameInfo( QString type, QString id, QString name, int ios, int region, QStringList files, QStringList partitionOffsets, bool fakesigned );

public slots:
    void PasswordIsEntered();

private slots:
    void ReadyReadStdOutSlot();
    void ReadyReadStdErrSlot();
    void ProcessFinishedSlot( int, QProcess::ExitStatus );

};

#endif // WITHANDLER_H
