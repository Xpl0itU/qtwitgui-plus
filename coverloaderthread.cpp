#include "coverloaderthread.h"

CoverLoaderThread::CoverLoaderThread(QObject *parent) : QThread(parent)
{
    abort = false;
	qRegisterMetaType< QList<QImage> >();
}

void CoverLoaderThread::ForceQuit()
{
    mutex.lock();
    abort = true;
    mutex.unlock();
}

CoverLoaderThread::~CoverLoaderThread()
{
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();
    wait();
}
void CoverLoaderThread::CheckCovers( const QStringList &s, const QString &baseFolder, const QString &path2, const QString &path3, \
									 const QString &pathF, const QString &pathH, const QString &pathD, int m, bool r )
{
    basePath = baseFolder;
    ids = s;
    path2D = path2;
    path3D = path3;
    pathFull = pathF;
    pathDisc = pathD;
    pathHQ = pathH;
    mode = m;
    reload = r;

    if ( !isRunning() )
    {
		start( NormalPriority );

    }
    else //not used since this is a 1 time use thread.  just leaving it here for reference material
    {
		condition.wakeOne();
		//QString str = "Thread already running\n";
		//emit SendText( str );
    }
}

void CoverLoaderThread::run()
{
	if ( abort )
	{
	    qDebug( "Thread abort" );
	    return;
	}
	mutex.lock();
	if( !ids.size() )
	{
	    qDebug() << "its empty";
	    return;
	}
	mutex.unlock();

	//mutex.unlock();

	QList< QImage > returns;


	int size = ids.size();
	for( int i = 0; i < size; i++ )
	{
	    //qDebug() << "id:" << ids.at( i );
	    if( mode == mode_load )
	    {
			returns << Get( ids.at( i ) );
			emit SendProgress( (int)( ( (float)( i + 1 ) / (float)size ) * (float)100 ) );
	    }
	    else if( mode == mode_check )
	    {
			Check( ids.at( i ) );
	    }
	    else
	    {
			qDebug() << "coverthread: unknown mode" << mode;
			break;
	    }

	}
	if( mode == mode_load )
	{
	    emit SendProgress( 100 );
	    emit SendDone( returns, reload );
	}

}
//only supports 2d and Full covers for now
QImage CoverLoaderThread::Get( const QString &id )
{
    QImage ret;
    QDir dir( basePath );
    if( !dir.exists() )
    {
		qDebug() << dir << "doesnt exist";
		return ret;
    }
    QDir subDir;
    subDir.setPath( dir.filePath( path2D ) );
    if( ret.load( subDir.absolutePath() + "/" + id + ".png" ) )
    {
		ret = To160x224( ret );
		if( ret != QImage() )
			return ret;
    }
    subDir.setPath( dir.filePath( pathFull ) );
    if( ret.load( subDir.absolutePath() + "/" + id + ".png" ) )
    {
		ret = To160x224( ret );
		if( ret != QImage() )
			return ret;
    }
    subDir.setPath( dir.filePath( pathHQ ) );
    if( ret.load( subDir.absolutePath() + "/" + id + ".png" ) )
    {
		ret = To160x224( ret );
		if( ret != QImage() )
			return ret;
    }
    //qDebug() << "couldnt load" << subDir.absolutePath() + "/" + id + ".png";
    if( ret.load( dir.absolutePath() + "/nocover.png" ) )//user defined no-cover
    {
		return To160x224( ret );
    }
    ret.load( ":/images/nocover.png" );//no-cover compiled in this program
    return ret;

}

void CoverLoaderThread::Check( const QString &id )
{
    QDir dir( basePath );
    if( !dir.exists() )
    {
		qDebug() << dir << "doesnt exist";
		return;
    }
    QDir subDir;
    subDir.setPath( dir.filePath( path2D ) );
    if( !QFile::exists( subDir.absolutePath() + "/" + id + ".png" ) )
		emit CoverIsMissing( id, coverType2d );

    subDir.setPath( dir.filePath( pathFull ) );
    if( !QFile::exists( subDir.absolutePath() + "/" + id + ".png" ) )
		emit CoverIsMissing( id, coverTypeFull );

    subDir.setPath( dir.filePath( pathHQ ) );
    if( !QFile::exists( subDir.absolutePath() + "/" + id + ".png" ) )
		emit CoverIsMissing( id, coverTypeFullHQ );

    subDir.setPath( dir.filePath( path3D ) );
    if( !QFile::exists( subDir.absolutePath() + "/" + id + ".png" ) )
		emit CoverIsMissing( id, coverType3d );

    subDir.setPath( dir.filePath( pathDisc ) );
    if( !QFile::exists( subDir.absolutePath() + "/" + id + ".png" ) )
		emit CoverIsMissing( id, coverTypeDisc );

}

QImage CoverLoaderThread::To160x224( const QImage &i )
{
    if( i.size() == QSize( 160, 224 ) )//flat 2d image
		return i;

    if( i.size() == QSize( 512, 340 ) )//full cover
    {
		int fStart = 271;
		QImage front = i.copy( fStart, 0, 512 - fStart, 340 );
		return front.scaledToHeight( 224 );
    }

    if( i.size() == QSize( 1024, 680 ) )//full cover HQ ( wiiflow style )
    {
		int fStart = 535;
		QImage front = i.copy( fStart, 0, 1024 - fStart, 680 );
		return front.scaledToHeight( 224 );
    }

    //TODO: i have no clue how to impliment this.  it should be possible with qtransform
    /*if( i.size() == QSize( 176, 248 ) )//3d covers		16,7   162,15  231,7   162, 216
    {

    }*/

    return QImage();
}
