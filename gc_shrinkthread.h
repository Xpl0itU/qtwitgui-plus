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

#ifndef GC_SHRINKTHREAD_H
#define GC_SHRINKTHREAD_H

#include "includes.h"
#define GC_FATAL 5555

typedef struct bannerText
{
    char  shTitle[ 0x20 ];
    char  shMaker[ 0x20 ];
    char  title  [ 0x40 ];
    char  maker  [ 0x40 ];
    char  comment[ 0x80 ];
} bannerText;

typedef struct GCBanner
{
    qint32 type;
    quint8  padding    [ 0x1c ];
    quint8  image      [ 0x1800 ];
    bannerText text[ 6 ];
} GCBanner;

struct FEntry
{
	union
	{
		struct
		{
			unsigned int Type		:8;
			unsigned int NameOffset	:24;
		};
		unsigned int TypeName;
	};
	union
	{
		struct		// File Entry
		{
			unsigned int FileOffset;
			unsigned int FileLength;
		};
		struct		// Dir Entry
		{
			unsigned int ParentOffset;
			unsigned int NextOffset;
		};
		unsigned int entry[2];
	};
};


class GC_Game
{

public:
	GC_Game( const QString &path = QString() );
    ~GC_Game();

    //get the text from the banner.  pal banners can have different languages
	QString GetSHTitle ( int lang ){ return QString( opening_bnr.text[ lang ].shTitle ); }
	QString GetTitle   ( int lang ){ return QString( opening_bnr.text[ lang ].title ); }
	QString GetSHMaker ( int lang ){ return QString( opening_bnr.text[ lang ].shMaker ); }
	QString GetMaker   ( int lang ){ return QString( opening_bnr.text[ lang ].maker ); }
	QString GetComment ( int lang ){ return QString( opening_bnr.text[ lang ].comment ); }
	QString GetName() { return QString( hdrname ); }
	QString GetID(){ return QString( id ); }
	QPixmap BannerImage( int height );

    int   GetLangCnt()      { return langCnt; }
    bool fileOk;
protected:
    GCBanner opening_bnr;
    QString pathstr;
    QFile file;

    QByteArray fst_bin;
    quint32 fstSize;
    quint32 fstOffset;
    quint32 dolOffset;
    quint32 dolSize;
    quint32 apploaderSize;
    quint32 dolReused; //aquaman & some other games mix the main.dol in with the fst files
    QByteArray newBootBin;

    int langCnt;
    QString id;
    QString hdrname;

    quint32 ReadU32();

	void FatalError( const QString &str );//not used in this class
};

class GC_ShrinkThread : public QThread , public GC_Game
 {
    Q_OBJECT

 public:
	 GC_ShrinkThread( QObject *parent = 0, const QString &path = QString() );
     ~GC_ShrinkThread();

	 void ShrinkTo( const QString &destPath, bool over );

 protected:
     void run();

 signals:
     void SendText( const QString & ); // not used here, but left in for copy/paste thread creating later
     void SendProgress( int );
     void SendDone();
	 void SendFatalError( const QString &, int );


 private:
     QString newPath;
     bool overwrite;
     QMutex mutex;
     QWaitCondition condition;

     bool abort;

	 void FatalError( const QString &str );

 };

#endif // GC_SHRINKTHREAD_H
