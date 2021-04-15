#include "includes.h"
#include "wiitreethread.h"

WiiTreeThread::WiiTreeThread( QObject *parent ) : QThread( parent )
{
    abort = false;
    returnItem = new QTreeWidgetItem;

    nameFont = QApplication::font();
#ifdef Q_WS_WIN
    monoFont = QFont( "Courier New", QApplication::font().pointSize() );
#else
    #ifdef Q_WS_MAC
	monoFont = QFont( "Courier New", QApplication::font().pointSize() - 1 );
    #else
	monoFont = QFont( "Courier New", QApplication::font().pointSize() - 1, QFont::Bold );
    #endif
    nameFont.setPointSize( nameFont.pointSize() - 1 );
#endif
}

WiiTreeThread::~WiiTreeThread()
{
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();
    delete returnItem;

    wait();
}

//sets the variables and then starts the thread.  this is a 1 time run thread.
//! s is a long QString af paths to turn into a tree.  separated by \n
//! b is weather or not to skip entries with "/.svn" in them
//! ficon is the folder icon to use
//! dicon is the icon to use for directories
void WiiTreeThread::DoCommand( const QStringList &s, bool b, const QIcon &ficon, const QIcon &dicon, int type, const QStringList &list )
{
    //qDebug() << "WiiTreeThread::DoCommand";
    QMutexLocker locker(&mutex);
    inCommand = s;
    skipSvn = b;
    groupIcon = dicon;
    keyIcon = ficon;
    gameType = type;
    encryptedOffsets = list;

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
void WiiTreeThread::ForceQuit()
{
    mutex.lock();
    abort = true;
    mutex.unlock();
}

void WiiTreeThread::run()
{
	if ( abort )
	{
	    qDebug( "Thread abort" );
	    return;
	}
	mutex.lock();
	if( !inCommand.size() )
	{
	    qDebug() << "its empty";
	    return;
	}
	mutex.unlock();


	emit SendProgress( 0 );
	int cnt = inCommand.size();

	//add each full path to the free view
	for ( int i = 0; i < cnt; i++ )
	{
	    //qDebug() << inCommand.at( i );
	    if ( abort )
	    {
		qDebug( "Thread abort called" );
		return;
	    }
	    if( ( skipSvn && inCommand.at( i ).contains( "/.svn/" ) )
		|| inCommand.at( i ).size() < 15
			|| inCommand.at( i ).contains( "** Partition #" )) continue;
	    if( !inCommand.at( i ).isEmpty() )
	    {
		AddItemToTree( inCommand.at( i ), returnItem  );
		emit SendProgress( (int)( ( (float)( i + 1 ) / (float)cnt ) * (float)100 ) );
	    }
	}
	//qDebug() << "thread is done";
	emit SendProgress( 100 );
	emit SendDone( returnItem );

}

//adds an item to the tree view given a full path
void WiiTreeThread::AddItemToTree( const QString &s, QTreeWidgetItem *parent )
{
    QString path = s;
    path = path.simplified();
    QString sizeText;
    QString offsetText = "0x";

    bool isSysFolder = s.endsWith( "sys/" ) && s.count( "/" ) == 2;

    //get the offset from the start of the string
    while( !path.startsWith( " " ) )
    {
	offsetText += path.at( 0 );
	path.remove( 0, 1 );
    }
    path.remove( 0, 1 );

    //get the size from the start of the string
    while( !path.startsWith( " " ) )
    {
	sizeText += path.at( 0 );
	path.remove( 0, 1 );
    }
    path.remove( 0, 1 );

    if( sizeText.startsWith( "N=" ) )
    {
	sizeText.remove( 0, 2 );
	sizeText = tr( "%1 files" ).arg( sizeText );
    }
    else sizeText.prepend( "0x" );

    //change text for VC partitions
    if( gameType && path.startsWith( "P-" ) )
    {
	//qDebug() << "!gameType" << gameType;
	path.remove( 0, 2 );
	path.prepend( "[ VC ] " );
    }

    // start at the beginning of the full path and add each folder to the tree if it needs to be added
    // change the pointer to the new folder and continue down the string till we get to the end
    while( !path.isEmpty() )
    {
	QString string;
	bool isFolder = false;

	int slash = path.indexOf( "/" );
	if( slash >= 0 )
	{
	    string = path.left( slash );
	    path.remove( 0, slash + 1 );
	    isFolder = true;
	}
	else
	{
	    string = path;
	    path.clear();
	}
	int found = -1;

	if( isFolder )//assume that there can never be a file and folder wth the exact same name ( greatly speeds up parsing )
	    found = findItem( string, parent );

	if( found == -1 )
	{
	    parent = new QTreeWidgetItem( parent, QStringList() << string );
	    if( isFolder )
		parent->setIcon( 0, groupIcon );
	    else
	    {
		parent->setIcon( 0, keyIcon );
		parent->setText( 1, offsetText );
	    }
	    parent->setText( 2, sizeText );
	    parent->setFont( 0, nameFont );
	    parent->setFont( 1, monoFont );
	    parent->setFont( 2, monoFont );
	    parent->setTextAlignment( 1, Qt::AlignRight | Qt::AlignVCenter );

	    if( isSysFolder && gameType == 1 )//only for wii games
	    {
		parent->setText( 1, encryptedOffsets.at( 0 ) );
		encryptedOffsets.removeAt( 0 );
	    }
	}
	else
	{
	    parent = parent->child( found );
	}
    }
}

//returns the index of the child named "s" of the given parent or -1 if the child is not found
int WiiTreeThread::findItem( const QString &s, QTreeWidgetItem *parent )
{
    int ret = -1;
    int cnt = parent->childCount();
    for( int i = cnt - 1; i >= 0; i-- )
    {
	if( parent->child( i )->text( 0 ) == s )
	{
	    ret = i;
	    break;
	}
    }
    return ret;
}



