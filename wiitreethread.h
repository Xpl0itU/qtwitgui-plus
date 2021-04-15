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

#ifndef WIITREETHREAD_H
#define WIITREETHREAD_H
// about 95% of this class is pieced together from the Qt examples, 2% I wrote on purpose, and 3% I accidentally wrote and it worked so it stayed


#include "includes.h"


class WiiTreeThread : public QThread
 {
    Q_OBJECT

 public:
     WiiTreeThread( QObject *parent = 0 );
     ~WiiTreeThread();

	 void DoCommand( const QStringList &s, bool b, const QIcon &ficon, const QIcon &dicon, int type, const QStringList &list );
     void ForceQuit();

 protected:
     void run();

 signals:
     void SendText( const QString & ); // not used here, but left in for copy/paste thread creating later
     void SendProgress( int );
     void SendDone( QTreeWidgetItem * );


 //public slots:

 private:
     QMutex mutex;
     QWaitCondition condition;
     QStringList inCommand;
     QTreeWidgetItem *returnItem;

     QIcon groupIcon;
     QIcon keyIcon;
     QFont nameFont;
     QFont monoFont;
     QStringList encryptedOffsets;

     bool abort;
     bool skipSvn;
     int gameType;
     int findItem(  const QString &s, QTreeWidgetItem *parent );
     void AddItemToTree( const QString &s, QTreeWidgetItem *parent );
 };

#endif // WIITREETHREAD_H
