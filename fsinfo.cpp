#include "fsinfo.h"

#ifdef Q_WS_LINUX
#include <sys/vfs.h>
#define	    HFS_SUPER_MAGIC       0x4244
#define	    HPFS_SUPER_MAGIC      0xF995E849
#define	    MSDOS_SUPER_MAGIC     0x4d44
#define	    NTFS_SB_MAGIC         0x5346544e
#define	    NTFS_PUNE_MAGIC       0x65735546//all my NTFS return this ( created on XP x64 )
#endif

#ifdef Q_WS_WIN//keep a list of the dvd drive letters to exclude them from the partition-auto search
static QStringList dvdLetters;
#endif

FsInfo::FsInfo(QObject *parent) :
		QObject(parent)
{
}
#ifdef Q_WS_WIN
bool FsInfo::Check()
{
    QProcess p;

    //check that we have the program to convert windows paths to proper cygwin paths
    p.start( "cygpath -v" );
    if( !p.waitForStarted() )
    {
		qCritical() << "failed to start cygpath ( check )";
		return false;
    }

    if( !p.waitForFinished() )
    {
		qCritical() << "!p.waitForFinished() ( winfs stuff )";
		return false;
    }
    if( p.exitCode() != QProcess::NormalExit )
    {
		qCritical() << "exit status ( winfs stuff )" << p.exitCode() << QProcess::NormalExit;
		return false;
    }

    QString output = p.readAll();
    if( !output.contains( "Cygwin pathconv" ) && !output.contains( "Path Conversion Utility" ) )//LITE and full versions
    {
		qCritical() << "cygpath text output wrong ( winfs stuff ):" << output;
		return false;
    }

    //check that we can access wmic to query the filesystems
    //just query the operating system for a test
    p.start( "wmic", QStringList() << "/output:stdout" << "/interactive:off" << "/locale:ms_409" <<\
			 "OS" << "GET" << "caption" );
    if( !p.waitForStarted() )
    {
		qCritical() << "failed to start wmic ( check )";
		return false;
    }
    p.closeWriteChannel();

    if( !p.waitForFinished() )
    {
		qCritical() << "!p.waitForFinished() wmic ( winfs stuff )";
		return false;
    }
    if( p.exitCode() != QProcess::NormalExit )
    {
		qCritical() << "exit status wmic ( winfs stuff )";
		return false;
    }

    output = p.readAll();
    if( !output.contains( "Windows" ) )
    {
		qCritical() << "wmic text output wrong ( winfs stuff ):" << output;
		return false;
    }

    //build a list of the did drive letters so we can skip them later
    for( int j = 0; j < 15; j++ )
    {
        bool ok = false;
        QString tmp = ToWinPath( QString( "/dev/sr%1" ).arg( j ), &ok );
        if( !tmp.endsWith( ":" ) )
            break;

        dvdLetters << tmp;
    }
    return true;
}

bool FsInfo::IsDVDLetter( const QString &path )
{
    QString p = path;
    p.resize( 2 );
    return dvdLetters.contains( p, Qt::CaseInsensitive );
}

QString FsInfo::ToWinPath( const QString &cygPath, bool *ok )
{
    *ok = false;
    QProcess p;
    p.start( "cygpath", QStringList() << "-w" << cygPath );
    if( !p.waitForStarted() )
    {
		qCritical() << "failed to start cygpath ( towinpath )";
		return QString();
    }

    if( !p.waitForFinished() )
    {
		qCritical() << "!p.waitForFinished() ( winfs stuff )";
		return QString();
    }
    if( p.exitCode() != QProcess::NormalExit )
    {
		qCritical() << "exit status ( winfs stuff )";
		return QString();
    }

    QString output = p.readAll();
    output.remove( "\r" );
    output.remove( "\n" );
    output.remove( "\\\\.\\" );//if the path maps to a drive letter, it will have these badboys on it.  just remove them
    //qDebug() << "FsInfo::ToWinPath:" << cygPath << output;
    *ok = true;
    return output;
}

QString FsInfo::ToCygPath( const QString &winPath, bool *ok )
{
    *ok = false;
    QProcess p;
    p.start( "cygpath", QStringList() << "-u" << winPath );
    if( !p.waitForStarted() )
    {
		qCritical() << "failed to start cygpath ( tocygpath )";
		return QString();
    }

    if( !p.waitForFinished() )
    {
		qCritical() << "!p.waitForFinished() ( winfs stuff )";
		return QString();
    }
    if( p.exitCode() != QProcess::NormalExit )
    {
		qCritical() << "exit status ( winfs stuff )";
		return QString();
    }

    QString output = p.readAll();
    output.remove( "\r" );
    output.remove( "\n" );
    //qDebug() << "FsInfo::ToCygPath:" << winPath << output;
    *ok = true;
    return output;
}
#endif

QString FsInfo::GetFilesystem( QString path )
{
    QProcess p;
#ifdef Q_WS_WIN
    bool ok = false;
    QString drive = ToWinPath( path, &ok );
    int colon = drive.indexOf( ":" );
    if( colon != 1 || !ok )
    {
		qCritical() << "FsInfo::GetFilesystem() colon != 1" << colon << ok;
		return QString();
    }
    drive.resize( 1 );

    p.start( "wmic", QStringList() << "/output:stdout" << "/interactive:off" << "/locale:ms_409" <<\
			 "logicaldisk" << "where" << "DeviceID=\'" + drive + ":\'" << "get" << "filesystem" );
#elif defined Q_WS_MAC
    p.start( "diskutil", QStringList() << "info" << path );
#else
    //try statfs first as it is the fastest.  but its descriptors are less than descriptive for ext variants
    struct statfs fInfo;
    int r = statfs( path.toLatin1().data(), &fInfo );
    if( !r )
    {
		switch( fInfo.f_type )
		{
		case MSDOS_SUPER_MAGIC:
			return "FAT";
			break;
		case NTFS_SB_MAGIC:
		case NTFS_PUNE_MAGIC:
			return "NTFS";
			break;
		case HFS_SUPER_MAGIC:
		case HPFS_SUPER_MAGIC:
			return "HPFS";
			break;
		default:
			break;
		}
    }
    p.start( "df", QStringList() << "-T" << path );
#endif
    if( !p.waitForStarted( 5000 ) )
    {
		qCritical() << "FsInfo::GetFilesystem failed to start";
		return QString();
    }
    p.closeWriteChannel();

    if( !p.waitForFinished() )
    {
		qCritical() << "!p.waitForFinished() ( getfs )";
		return QString();
    }
    if( p.exitCode() != QProcess::NormalExit )
    {
		qCritical() << "exit status ( getfs )" << p.exitCode();
		return QString();
    }

    QString output = p.readAll();
    output.remove( "\r" );
    QStringList list = output.split( "\n", QString::SkipEmptyParts );
#ifdef Q_WS_WIN
    if( !list.contains( "FileSystem  " ) || list.size() != 2 )
    {
		qCritical() << "wrong output ( getfs )" << list;
		return QString();
    }
    QString fs = list.at( 1 );
    fs = fs.simplified();
    return fs;
#elif defined Q_WS_MAC
    int size = list.size();
    for( int i = 0; i < size; i++ )
    {
		if( !list.at( i ).contains( "File System:" ) )
			continue;

		QString fs = list.at( i );
		fs.remove( 0, fs.indexOf( "File System:" ) + 12 );
		fs = fs.trimmed();
		int space = fs.indexOf( " " );
		if( space > 0 )
		{
			fs.remove( 0, space + 1 );
		}
		//qDebug() << fs;
		return fs;
    }
    return  QString();
#else
    int size = list.size();
    if( size != 2 )
    {
		qCritical() << "size != 2 ( getfs )" << list;
		return QString();
    }
    QString fs = list.at( 1 );
    fs = fs.simplified();
    if( fs.count( " " ) < 2 )
    {
		qCritical() << "spaces < 2 ( getfs )" << fs;
		return QString();
    }
    fs.remove( 0, fs.indexOf( " ") + 1 );
    fs.resize( fs.indexOf( " " ) );
    if( fs == "devtmpfs" )//this is what it shows if if has permission to read a block device -- ie,  WBFS partition at /dev/sdc2
    {
		qCritical() << "fs == devtmpfs ( getfs )";
		return QString();
    }
    return fs;
#endif
}

QStringList FsInfo::GetDvdDrives()
{

#ifdef Q_WS_WIN
    QProcess p;
    p.start( "listDvd" );
    if( !p.waitForStarted( 5000 ) )
    {
		qCritical() << "FsInfo::GetDvdDrives() failed to start";
        return QStringList();
    }

    if( !p.waitForFinished() )
    {
		qCritical() << "!p.waitForFinished() ( FsInfo::GetDvdDrives() )";
        return QStringList();
    }
    if( p.exitCode() != QProcess::NormalExit )
    {
		qCritical() << "exit status ( FsInfo::GetDvdDrives() )" << p.exitCode();
        return QStringList();
    }
    QString output = p.readAll();
    output.remove( "\r" );
    QStringList list = output.split( "\n", QString::SkipEmptyParts );
    return list;
#elif defined Q_WS_MAC//TODO...
    //vmware + osx + my dvd drives + burned wii games dont really work out to well.
    //for now, just don't do anything.  somebody that knows macs can fix this and submit a patch
    return QStringList();
#else
    QDir dir( "/dev" );
    QStringList dvds = dir.entryList( QStringList() << "sr*", QDir::AllEntries | QDir::System );
    QStringList ret;
    foreach( QString str, dvds )
		ret << "/dev/" + str;
    return ret;
#endif
}
