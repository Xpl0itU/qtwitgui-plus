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

#ifndef COVERLOADERTHREAD_H
#define COVERLOADERTHREAD_H

#include "includes.h"

enum
{
    coverType2d = 777,
    coverType3d,	//not supported in the coverflow crap, but used only to get the list of missing covers
    coverTypeFull,
    coverTypeFullHQ,
    coverTypeDisc
};
enum
{
    mode_check,
    mode_load
};

class CoverLoaderThread : public QThread
{
    Q_OBJECT

 public:
     CoverLoaderThread( QObject *parent = 0 );
     ~CoverLoaderThread();

     void CheckCovers( const QStringList &s, const QString &baseFolder, const QString &path2, const QString &path3, const QString &pathF, \
		       const QString &pathH, const QString &pathD, int m, bool r );
     void ForceQuit();

 protected:
     void run();

 signals:
     void SendProgress( int );
	 void SendDone( const QList< QImage > &, bool reload );
	 void CoverIsMissing( const QString &id, int t );


 //public slots:

 private:
     QMutex mutex;
     QWaitCondition condition;
     QStringList ids;
     QString basePath;
     QString path2D;
     QString path3D;
     QString pathFull;
     QString pathHQ;
     QString pathDisc;
     int mode;
     bool reload;
     //QStringList subDirs;

	 QImage Get( const QString &id );
	 void Check( const QString &id );

	 QImage To160x224( const QImage &i );

     int type;

     bool abort;
 };

Q_DECLARE_METATYPE(QList<QImage>)

#endif // COVERLOADERTHREAD_H
