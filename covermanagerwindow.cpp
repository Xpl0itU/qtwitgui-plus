#include "covermanagerwindow.h"
#include "ui_covermanagerwindow.h"
#include "tools.h"

//todo, allow adjustable paths
//#define COVERPATH   "covers"
#define PATHDISC    "disc"
#define PATH2D	    "2d"
#define PATH3D	    "3d"
#define PATHFULL    "full"
#define PATHFULL_HQ "full_HiRes"


CoverManagerWindow::CoverManagerWindow(QWidget *parent) : QWidget(parent), ui(new Ui::CoverManagerWindow)
{
    ui->setupUi(this);
    reloadCoversAfterDownload = false;
    downloading = false;
    threadRunning = false;
    ShowTextAndProgress( true );

    ui->progressBar_loading->setVisible( false );
    ui->label_info->setVisible( false );

    //hide the disc images for now
    ui->groupBox_sideBox->setVisible( false );
    ReloadSettings();

    /*pathOK = true;
    coverDir = COVERPATH;
    QDir cur = QDir::current();
    if( !cur.exists( coverDir ) )
    {
        if( !cur.mkdir( coverDir ) )
	{
	    qDebug() << "cant create the cover folder";
	    pathOK = false;
	}
    }
    coverDir = cur.absoluteFilePath( coverDir );*/

    QString localeStr = QLocale::system().name();
    localeStr.resize( 2 );
    localeStr = localeStr.toUpper();
    manager.SetLocale( localeStr );
    //manager.SetBasePath( coverDir );

    connect( &manager, SIGNAL( SendProgress( int ) ), ui->progressBar_loading, SLOT( setValue( int ) ) );
	connect( &manager, SIGNAL( SendText( const QString & ) ), this, SLOT( GetText( const QString & ) ) );
    connect( &manager, SIGNAL( finished() ), this, SLOT( DoneDownloading() ) );

    connect( &loader, SIGNAL( SendProgress( int ) ), ui->progressBar_loading, SLOT( setValue( int ) ) );
	connect( &loader, SIGNAL( SendDone( const QList< QImage > &, bool ) ), this, SLOT( ReceiveCovers( const QList< QImage > &, bool ) ) );
	connect( &loader, SIGNAL( CoverIsMissing( const QString &, int ) ), this, SLOT( ReceiveMissingCover( const QString &, int ) ) );

    connect( ui->pictureFlow, SIGNAL( centerIndexChanged( int ) ), this, SLOT( CoverHasBeenSelected( int ) ) );
    connect( ui->pictureFlow, SIGNAL( ImageClicked( int ) ), this, SLOT( CoverHasBeenSelected( int ) ) );

}

CoverManagerWindow::~CoverManagerWindow()
{
    delete ui;
}

//expose the pictureflow widget so the main window can directly connect signals and shit to it
PictureFlow *CoverManagerWindow::PFlowObject()
{
    return ui->pictureFlow;
}

void CoverManagerWindow::ShowSidePane( bool show )
{
    ui->groupBox_sideBox->setVisible( show );
}

void CoverManagerWindow::ShowTextAndProgress( bool show )
{
    ui->groupBox_sideBox->setVisible( show );
    ui->label_info->setVisible( show );
    //ui->progressBar_download->setVisible( show );

    resize( width(), 302 );
}

void CoverManagerWindow::SetGameLists( const QMap<QString, QList<QTreeWidgetItem *> > &gameMap )
{
    //qDebug() << "CoverManagerWindow::SetGameLists";
    gameLists.clear();
	QMap<QString, QList<QTreeWidgetItem *> >::ConstIterator i = gameMap.constBegin();
    while( i != gameMap.constEnd() )
    {
		QStringList ids;
		QString partName = i.key();
		int size = i.value().size();
		for( int j = 0; j < size; j++ )
		{
			ids << gameID( i.value().at( j ) );
		}

		if( size )
			gameLists.insert( partName, ids );

		i++;
    }
    //check if the gamelist being shown needs reloaded
    i = gameMap.find( currentPartition );
    if( i != gameMap.constEnd() )
    {
		bool needToReload = false;
		int size = i.value().size();
		for( int j = 0; j < size; j++ )
		{
			QTreeWidgetItem *item = i.value().at( j );
			if( !loadedList.contains( item->text( 0 ) ) )
			{
				needToReload = true;
				break;
			}
		}
		if( needToReload )
		{
			qDebug() << "gamelist has changed, reloading";
			LoadCoversForPartition( currentPartition );
		}
    }

    //not synced with any partition yet, load one
    else if( currentPartition.isEmpty() )
    {
		i = gameMap.begin();
		if( i != gameMap.constEnd() )
			LoadCoversForPartition( i.key() );
    }
    //debug shit
    /*QMap<QString, QStringList >::iterator p = gameLists.begin();
    while( p != gameLists.constEnd() )
    {
	qDebug() << p.key() << p.value().size();
	p++;
    }*/

}

void CoverManagerWindow::on_frame_customContextMenuRequested( const QPoint &pos )
{
    //qDebug() << "CoverManagerWindow::on_frame_customContextMenuRequested" << pos;
    QPoint globalPos = ui->frame->mapToGlobal( pos );
    //create the context menu based on the games selected
    QMenu myMenu( this );
    //QAction cvrCheck( tr( "Check for missing images"), &myMenu );

    //submenu for download options
    bool haveMissingCovers = false;
    bool needMenu = false;
    QMenu dlM( tr( "Download Missing Images" ), &myMenu );
    QAction cvrFlat( &dlM );
    QAction cvrFull( &dlM );
    QAction cvrFullH( &dlM );
    QAction cvr3d( &dlM );
    QAction cvrDvd( &dlM );
    QMap<int, QStringList >::iterator i = missingCovers.begin();
    while( i != missingCovers.constEnd() )
    {
		switch( i.key() )
		{
		case coverType2d:
			cvrFlat.setText( tr( "Front %1  ( %2 )" ).arg( "160x224" ).arg( i.value().size() ) );
			dlM.addAction( &cvrFlat );
			haveMissingCovers = true;
			break;
		case coverType3d:
			cvr3d.setText( tr( "3-D %1  ( %2 )").arg( "176x248" ).arg( i.value().size() ) );
			dlM.addAction( &cvr3d );
			haveMissingCovers = true;
			break;
		case coverTypeFull:
			cvrFull.setText( tr( "Full %1  ( %2 )").arg( "512X340" ).arg( i.value().size() ) );
			dlM.addAction( &cvrFull );
			haveMissingCovers = true;
			break;
		case coverTypeFullHQ:
			cvrFullH.setText( tr( "Full %1  ( %2 )").arg( "1024x680" ).arg( i.value().size() ) );
			dlM.addAction( &cvrFullH );
			haveMissingCovers = true;
			break;
		case coverTypeDisc:
			cvrDvd.setText( tr( "Disc %1  ( %2 )").arg( "160x160" ).arg( i.value().size() ) );
			dlM.addAction( &cvrDvd );
			haveMissingCovers = true;
			break;
		default:
			qDebug() << "unknown cover type in missing cover map" << i.key();
			break;
		}
		i++;
    }

    if( haveMissingCovers )
    {
		needMenu = true;
		myMenu.addMenu( &dlM );
    }

    QMenu partMenu( tr( "Sync With Partition" ), &myMenu );
    QList<QAction*> partActions;
    QMap<QString, QStringList >::iterator p = gameLists.begin();
    while( p != gameLists.constEnd() )
    {
		QAction* newAct = new QAction( p.key(), &partMenu );
		if( p.key() == currentPartition )
		{
			newAct->setCheckable( true );
			newAct->setChecked( true );
		}
		partActions << newAct;
		p++;
    }
    if( partActions.size() )
    {
		partMenu.addActions( partActions );
		myMenu.addMenu( &partMenu );
		needMenu = true;
    }

    if( !needMenu )//there are no items in the menu, just return
		return;
    //execute the menu
    QAction* selectedItem = myMenu.exec( globalPos );
    //respond to what was selected
    if( selectedItem )
    {
		//download mising flat covers
		if( selectedItem == &cvrFlat )
		{
			QMap<int, QStringList >::iterator i = missingCovers.find( coverType2d );
			if( i == missingCovers.end() )
			{
				qDebug() << "i == missingCovers.end()";
				return;
			}
			downloading = true;
			reloadCoversAfterDownload = true;
			ui->progressBar_loading->setVisible( true );
			ui->label_info->setVisible( true );
			manager.append( i.value(), coverType2d );
		}
		//download mising full covers
		else if( selectedItem == &cvrFull )
		{
			QMap<int, QStringList >::iterator i = missingCovers.find( coverTypeFull );
			if( i == missingCovers.end() )
			{
				qDebug() << "i == missingCovers.end()";
				return;
			}
			reloadCoversAfterDownload = true;
			downloading = true;
			ui->progressBar_loading->setVisible( true );
			ui->label_info->setVisible( true );
			manager.append( i.value(), coverTypeFull );
		}
		//download mising full covers ( hi-res )
		else if( selectedItem == &cvrFullH )
		{
			QMap<int, QStringList >::iterator i = missingCovers.find( coverTypeFullHQ );
			if( i == missingCovers.end() )
			{
				qDebug() << "i == missingCovers.end()";
				return;
			}
			reloadCoversAfterDownload = true;
			downloading = true;
			ui->progressBar_loading->setVisible( true );
			ui->label_info->setVisible( true );
			manager.append( i.value(), coverTypeFullHQ );
		}
		//download mising 3D covers
		else if( selectedItem == &cvr3d )
		{
			QMap<int, QStringList >::iterator i = missingCovers.find( coverType3d );
			if( i == missingCovers.end() )
			{
				qDebug() << "i == missingCovers.end()";
				return;
			}
			downloading = true;
			ui->progressBar_loading->setVisible( true );
			ui->label_info->setVisible( true );
			manager.append( i.value(), coverType3d );
		}
		//download mising disc images
		else if( selectedItem == &cvrDvd )
		{
			QMap<int, QStringList >::iterator i = missingCovers.find( coverTypeDisc );
			if( i == missingCovers.end() )
			{
				qDebug() << "i == missingCovers.end()";
				return;
			}
			downloading = true;
			ui->progressBar_loading->setVisible( true );
			ui->label_info->setVisible( true );
			manager.append( i.value(), coverTypeDisc );
		}
		// load covers for selected partition
		int size = partActions.size();
		for( int i = 0; i < size; i++ )
		{
			if( selectedItem != partActions.at( i ) )
				continue;

			LoadCoversForPartition( selectedItem->text() );
			break;
		}
    }
}

void CoverManagerWindow::ReloadSettings()
{
    //qDebug() << "CoverManagerWindow::ReloadSettings()";
    QSettings s( settingsPath, QSettings::IniFormat );
    coverDir = s.value( "paths/covers", "" ).toString();
    pathOK = false;
    if( coverDir.isEmpty() )
    {
		qDebug() << "no path set for covers";
		return;
    }

    QDir cur = QDir::current();
    if( !cur.exists( coverDir ) )
    {
		if( !cur.mkdir( coverDir ) )
		{
			qDebug() << "cant create the cover folder";
			return;
		}
    }
    pathOK = true;
    manager.SetBasePath( coverDir );
    coverDir = cur.absoluteFilePath( coverDir );
}

void CoverManagerWindow::LoadCoversForPartition( const QString &part )
{
    if( !pathOK || threadRunning )
    {
		qDebug() << "!pathOK || threadRunning" << threadRunning;
		return;
    }
    QMap<QString, QStringList >::iterator p = gameLists.find( part );
    if( p == gameLists.end() )
    {
		qDebug() << "CoverManagerWindow::LoadCoversForPartition : part not found" << part;
		return;
    }

    loadedList.clear();
    int size = p.value().size();
    if( !size )
    {
		qDebug() << "CoverManagerWindow::LoadCoversForPartition : no games" << part;
		return;
    }
    currentPartition = part;
    //dont add duplicates to the coverflow.
    //with several thousand games, this will slow down a lot, but for a partition with 500 games, it happens in < 1 second for me
    for( int j = 0; j < size; j++ )
    {
		if( !loadedList.contains( p.value().at( j ) ) )
			loadedList << p.value().at( j );
		//else
	    //qDebug() << "CoverManagerWindow::LoadCoversForPartition: duplicate ID" << p.value().at( j );
    }

    threadRunning = true;
    ui->progressBar_loading->setValue( 0 );
    ui->progressBar_loading->setVisible( true );

    //forget all old missing covers
    missingCovers.clear();
    //if( missingCovers.size() )
	//qDebug() << "missingCovers.size()";
    loader.CheckCovers( loadedList, coverDir, PATH2D, PATH3D, PATHFULL, PATHFULL_HQ, PATHDISC, mode_load, true );
}

void CoverManagerWindow::ReceiveCovers( const QList< QImage > &covers, bool reload )
{
    threadRunning = false;
    //qDebug() << "CoverManagerWindow::ReceiveCovers";
    if( !downloading )
    {
		ui->progressBar_loading->setVisible( false );
		resize( width(), 302 );
    }

    if( reload )//dont reload the coverflow if we just get a list of disc images
    {
		ui->pictureFlow->clear();

		int size = covers.size();
		for( int i = 0; i < size; i++ )
		{
			ui->pictureFlow->addSlide( covers.at( i ) );
		}
		if( ui->pictureFlow->centerIndex() >= size )
			ui->pictureFlow->setCenterIndex( 0 );
    }

    //go ahead and request a list of all missing covers for this partition - this is a fast operation.  it should not be noticed
    if( !loadedList.isEmpty() )
		loader.CheckCovers( loadedList, coverDir, PATH2D, PATH3D, PATHFULL, PATHFULL_HQ, PATHDISC, mode_check, false );

}

void CoverManagerWindow::Refresh()
{
    if( !currentPartition.isEmpty() )
        LoadCoversForPartition( currentPartition );
}

//convert a number emited from the pictureflow widget to an ID for the WiiTDB widget
void CoverManagerWindow::CoverHasBeenSelected( int i )
{
    if( i < 0 || i >= loadedList.size() )
		return;

    emit NewIDInFocus( loadedList.at( i ) );
}

void CoverManagerWindow::ReceiveMissingCover( const QString &id, int type )
{
    //qDebug() << "CoverManagerWindow::ReceiveMissingCover" << type << id;
    //manager.append( id, type );
    QMap< int, QStringList >::iterator p = missingCovers.find( type );
    if( p == missingCovers.end() )//no list for this type yet, just add one
    {
		missingCovers.insert( type, QStringList() << id );
		return;
    }

    if( !p.value().contains( id ) )
		p.value() << id;
}

void CoverManagerWindow::GetText( const QString &text )
{
    ui->label_info->setText( text );
}

void CoverManagerWindow::DoneDownloading()
{
    //ui->progressBar_loading->setVisible( false );
    ui->label_info->setVisible( false );
    resize( width(), minimumSizeHint().height() );

    if( reloadCoversAfterDownload )
		LoadCoversForPartition( currentPartition );
    else
    {
		ui->progressBar_loading->setVisible( false );
    }
    reloadCoversAfterDownload = false;
    downloading = false;
}



DownloadManager::DownloadManager( QObject *parent, const QString &base, const QString &loc)
    : QObject(parent), downloadedCount(0), totalCount(0), baseCoverPath( base ), locale( loc )
{
}
DownloadManager::~DownloadManager()
{
    if( output.isOpen() )
    {
		output.close();
		output.remove();
    }
}

void DownloadManager::SetLocale( const QString &loc )
{
    //these are the ones supported by wiitdb
    if( loc == "US" || loc == "JA" || loc == "EN" || loc == "FR" || loc == "DE" || loc == "ES" || loc == "IT" ||
		loc == "NL" || loc == "PT" || loc == "AU" || loc == "RU" || loc == "KO" || loc == "ZH" || loc == "SE" )
		locale = loc;

    else locale = "EN";
}

void DownloadManager::SetBasePath( const QString &p )
{
    baseCoverPath = p;
}

void DownloadManager::append( const QStringList &idList, int t )
{
    foreach( QString id, idList )
		append( id, t );

    if (downloadQueue.isEmpty())
    {
		downloadedCount = 0;
		totalCount = 0;
		QTimer::singleShot(0, this, SIGNAL(finished()));
    }
}

void DownloadManager::append( const QString &url, int t )
{
    if( downloadQueue.isEmpty() )
		QTimer::singleShot( 0, this, SLOT( startNextDownload()));

    QMap< QString, int > dl;
    dl.insert( url, t );
    downloadQueue.enqueue( dl );
    ++totalCount;
}

QString DownloadManager::saveFileName( const QString &id, int t )
{
    QDir dir( baseCoverPath );
    QString subDirPath;
    switch( t )
    {
    case coverType2d:
		subDirPath = PATH2D;
		break;
    case coverType3d:
		subDirPath = PATH3D;
		break;
    case coverTypeFull:
		subDirPath = PATHFULL;
		break;
    case coverTypeFullHQ:
		subDirPath = PATHFULL_HQ;
		break;
    case coverTypeDisc:
		subDirPath = PATHDISC;
		break;
    default:
		return QString();
		break;
    }

    QDir subDir( dir.filePath( subDirPath ) );
    if( !subDir.exists() )
    {
		if( !subDir.mkpath( subDir.absolutePath() ) )
		{
			qDebug() << "cant create" << subDir.absolutePath();
			return QString();
		}
    }
    QString ret = subDir.absolutePath() + "/" + id + ".png";
    if( QFile::exists( ret ) )
    {
		if( !QFile::remove( ret ) )
		{
			qDebug() << "error deleting" << ret;
			return QString();
		}
    }
    return ret;
}

void DownloadManager::startNextDownload()
{
    notFound = false;
    int progress = (int)( ( (float)( totalCount - downloadQueue.size() ) / (float)totalCount ) * (float)100 );
    emit SendProgress( progress );
    if( downloadQueue.isEmpty() )
    {
		//qDebug() <<  downloadedCount << "of" << totalCount << "files downloaded successfully";
		downloadedCount = 0;
		totalCount = 0;
		emit finished();
		return;
    }

    QMap< QString, int > p = downloadQueue.dequeue();
    QMap< QString, int >::iterator dl = p.begin();

    //qDebug() << dl;
    currentType = dl.value();
    currentId = dl.key();
    QString dlUrl;
    QString filename = saveFileName( currentId, currentType );
    //qDebug() << id << type;
    if( filename.isEmpty() )
    {
		qDebug() << "empty filename" << currentId << currentType;
		startNextDownload();
		return;
    }
    output.setFileName( filename );
    if( !output.open( QIODevice::WriteOnly ) )
    {
		qDebug() << "cant open" << filename;
		startNextDownload();
		return;
    }
    switch( currentType )
	{
	case coverType2d:
	    dlUrl = "http://wiitdb.com/wiitdb/artwork/cover/";
	    break;
	case coverType3d:
	    dlUrl = "http://wiitdb.com/wiitdb/artwork/cover3D/";
	    break;
	case coverTypeFull:
	    dlUrl = "http://wiitdb.com/wiitdb/artwork/coverfull/";
	    break;
	case coverTypeFullHQ:
	    dlUrl = "http://wiitdb.com/wiitdb/artwork/coverfullHQ/";
	    break;
	case coverTypeDisc:
	    dlUrl = "http://wiitdb.com/wiitdb/artwork/disc/";
	    break;
	default:
	    startNextDownload();
	    break;
	}
    QChar regCode = currentId.at( 3 );
    if( regCode == 'E' )
		dlUrl += "US/";
    else if( regCode == 'J' )
		dlUrl += "JA/";
    else if( regCode == 'W' )
		dlUrl += "ZH/";
    else if( regCode == 'K' )
		dlUrl += "KO/";
    else
		dlUrl += locale + "/";

    dlUrl += currentId + ".png";
    qDebug() << "trying to DL" << dlUrl;
    currentJobText = dlUrl;

    QUrl url( dlUrl );

    QNetworkRequest request( url );
    //request.setRawHeader("User-Agent", PROGRAM_NAME );

    currentDownload = manager.get( request );
    connect( currentDownload, SIGNAL( downloadProgress( qint64, qint64 ) ), this, SLOT( downloadProgress( qint64, qint64 ) ), Qt::UniqueConnection );
    connect( currentDownload, SIGNAL( finished() ), this, SLOT( downloadFinished() ), Qt::UniqueConnection );
    connect( currentDownload, SIGNAL( readyRead() ), this, SLOT( downloadReadyRead() ), Qt::UniqueConnection );

    // prepare the output
    //printf("Downloading %s...\n", url.toEncoded().constData());
    downloadTime.start();
}

void DownloadManager::downloadProgress( qint64 bytesReceived, qint64 bytesTotal )
{
    Q_UNUSED( bytesTotal );
    // calculate the download speed
    double speed = bytesReceived * 1000.0 / downloadTime.elapsed();
    QString unit;
    if (speed < 1024) {
		unit = "bytes/sec";
    } else if (speed < 1024*1024) {
		speed /= 1024;
		unit = "kB/s";
    } else {
		speed /= 1024*1024;
		unit = "MB/s";
    }

    emit SendText( currentJobText + "   " + QString::fromLatin1( "%1 %2" ).arg( speed, 3, 'f', 1 ).arg( unit ) );
}


void DownloadManager::downloadFinished()
{
    output.flush();
    output.close();

    if( currentDownload->error() || notFound )
    {
		output.remove();
		if( currentType == coverTypeFullHQ || currentType == coverTypeFull )//can't get the full cover, try to get a front cover only
		{
			//qDebug() << "cant get a full cover for" << currentId << ". trying to get a front";
			if( !QFile::exists( baseCoverPath + "/" + PATH2D + "/" + currentId + ".png" ) )
				append( currentId, coverType2d );
			//else
			//qDebug() << "front cover already exists";
		}
		//qDebug() << "currentDownload->error()";
    }
    else
    {
		qDebug() << "Downloaded" << currentJobText;
		downloadedCount++;
    }

    currentDownload->deleteLater();
    startNextDownload();
}

void DownloadManager::downloadReadyRead()
{
    QByteArray data = currentDownload->readAll();
    if( data.startsWith( "<!DOCTYPE HTML PUBLIC" ) && data.contains( "<a href=\"http://wiitdb.com/404.php\"" ) )
    {
		qWarning() << "404" << currentJobText;
		notFound = true;
		return;
    }
    else if( !data.size() )
    {
		//qDebug() << "download error - !data.size() @" << currentJobText;
		notFound = true;
		return;
    }
    output.write( data );
}
