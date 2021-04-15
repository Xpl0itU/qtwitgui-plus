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

#ifndef TOOLS_H
#define TOOLS_H

#include "includes.h"

#define SETTINGS_VERSION 2

#define _KiB_ ( (float)0x40000 )
#define _MiB_ ( (float)0x100000 )
#define _GiB_ ( (float)0x40000000 )

#define MIN( x, y ) ( ( x ) < ( y ) ? ( x ) : ( y ) )
#define MAX( x, y ) ( ( x ) > ( y ) ? ( x ) : ( y ) )

#define RU(x,n)	(-(-(x) & -(n)))    //round up
#define ALIGN 0x8000		    //alignment for GC files


#define WIT_MINIMUM_VERSION 1904
#define WWT_MINIMUM_VERSION 1904

extern QString settingsPath;

QString SizeTextGiB( QString bytes );

#define GAME_ID_COLUMN		    0
#define GAME_NAME_COLUMN	    1
#define GAME_SIZE_COLUMN	    2
#define GAME_REGION_COLUMN	    3
#define GAME_TYPE_COLUMN	    4
#define GAME_PARTITIONS_COLUMN	    5
#define GAME_PATH_COLUMN	    6

QString gameID( QTreeWidgetItem* item );
QString gameName( QTreeWidgetItem* item );
QString gameSize( QTreeWidgetItem* item );
QString gameRegion( QTreeWidgetItem* item );
QString gameType( QTreeWidgetItem* item );
QString gamePartitions( QTreeWidgetItem* item );
QString gamePath( QTreeWidgetItem* item );

void SetGameID( QTreeWidgetItem*, const QString & );
void SetGameName( QTreeWidgetItem* item, const QString &s );
void SetGameSize( QTreeWidgetItem* item, const QString &s );
void SetGameRegion( QTreeWidgetItem* item, const QString &s );
void SetGameType( QTreeWidgetItem* item, const QString &s );
void SetGamePartitions( QTreeWidgetItem* item, const QString &s );
void SetGamePath( QTreeWidgetItem* item, const QString &s );

void hexdump( void *d, int len );

extern QPlainTextEdit *logWindow;

extern QString warningColor;
extern QString criticalColor;

void SetupLog();
void DebugHandler( QtMsgType type, const char *msg );

#ifdef Q_WS_WIN//add the drive letter in windows
QString RemoveDriveLetter( const QString &path );
QString AddDriveLetter( const QString &path );
#endif
#endif // TOOLS_H
