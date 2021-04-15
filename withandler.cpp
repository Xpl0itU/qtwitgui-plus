#include "withandler.h"
#include "passworddialog.h"
#include "includes.h"
#include "wiitdb.h"
#include "tools.h"
#include "fsinfo.h"

//store the sudo strings here so we dont have to look at the settings file every time we need to check them
QString rootAskStr;
QString rootWrongStr;
QString rootFailStr;

//only ask the user 1 time for their password unless they give a wrong one
QString rootPass;

//keep track of which formats wit can read/write/extend...
struct WitOptionAttr
{
    QString name;
    QString option;
    QStringList extensions;
    QStringList attributes;
};

static QList<WitOptionAttr> optionAttributes;
static int witVersion = 0;
static QString witVersionString;

//if there is a setting for titles.txt, remember it here, so we dont always have to look it up
static QString titlesTxtPath;

WitHandler::WitHandler( QObject *parent, bool root ) :QObject( parent )
{
    runAsRoot = root;
    witJob = witNoJob;
    namesFromWiiTDB = false;
    //create the pointer to the process used to run wit
    process = new QProcess( this );

#ifdef Q_OS_WIN
    // Add an environment variable to shut up the cygwin warning in windows
    //QStringList env = QProcess::systemEnvironment();//depreciated
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("CYGWIN", "nodosfilewarning");
    process->setProcessEnvironment( env );
#endif

    //connect output and input signals between the process and the main window so we can get information from it
    //and also send a "kill" message if this object is destroyed while the process is running
    connect( process, SIGNAL( readyReadStandardOutput() ), this, SLOT( ReadyReadStdOutSlot() ) );
    connect( process, SIGNAL( readyReadStandardError() ), this, SLOT( ReadyReadStdErrSlot() ) );
    connect( process, SIGNAL( finished( int, QProcess::ExitStatus ) ), this, SLOT( ProcessFinishedSlot(  int, QProcess::ExitStatus ) ) );
    connect( this, SIGNAL( KillProcess() ), process, SLOT( kill() ) );
}

WitHandler::~WitHandler()
{
    if( witJob != witNoJob )
    {
	emit KillProcess();
    }
    delete process;
    process = 0;
}

//replace names from wit with ones from WiiTDB
void WitHandler::SetNamesFromWiiTDB( bool wiitdb )
{
    namesFromWiiTDB = wiitdb;
}

void WitHandler::SetTitlesTxtPath( const QString &path )
{
    titlesTxtPath = path;
}

QString WitHandler::GetTitlesTxtPath()
{
    return titlesTxtPath;
}

void WitHandler::SetRunAsRoot( bool root )
{
    runAsRoot = root;
}

void WitHandler::ReadyReadStdOutSlot()
{
    //read text from wit
    QString curRead = process->readAllStandardOutput();
#ifdef Q_OS_WIN
    //get rid of stupid windows new lines
    curRead.replace( "\r\n", "\n" );
#endif
    stdStr += curRead;
    //since we are reading more stdout, assume that any existing stderr is non-fatal and clear it
    errStr.clear();

    //qDebug() << "gotmessage out" << curRead;

    switch( witJob )
    {
    case witDump:
	emit SendStdOut( curRead );
	break;
    case witCopy:
    case witEdit:
	if( curRead.contains( "* wit SCRUB" ) )
	{
	    currJobText = curRead;
	    currJobText.remove( 0, curRead.indexOf( "* wit SCRUB" ) + 12 );
	    currJobText.resize( currJobText.indexOf( " " ) );
	    emit SendMessageForStatusBar( currJobText );
	    break;
	}
	//turn the % message into a int and pass it to the progress bar
	if( curRead.contains( "%" ) )
	{
	    //qDebug() << curRead;
	    QString str = curRead.simplified(); //remove extra whitespace
	    QString numText;
	    int perChar = str.indexOf( "%" );
	    int num = 0;
	    if( perChar < 4 && perChar > 0)
	    {
		while( str.at( 0 ) != '%' )
		{
		    numText += str.at( 0 );
		    str.remove( 0, 1 );
		}

		num = numText.toInt();//convert to int
		if( num < 101 && num > -1 )
		    emit SendProgress( num );
	    }

	    QString rate;
	    num = str.indexOf( "(", 0 );
	    if( num > 1 )
	    {
		int clParen = str.indexOf( ")", num );
		if( clParen > 1 )
		{
		    num += 1;
		    rate = str.mid( num, clParen - num );
		}
	    }

	    num = str.indexOf( "ETA", 0 );
	    if( num > 1 )
	    {
		str.remove( 0, num );
	    }
	}
	break;
    default:
	break;
    }


}

void WitHandler::ReadyReadStdErrSlot()
{
    //read text from wit
    errStr += process->readAllStandardError();

    //qDebug() << "gotmessage err" << errStr;
#ifdef Q_OS_WIN
    //get rid of stupid windows new lines
    errStr.replace( "\r\n", "\n" );
#endif
    if( errStr.startsWith( rootWrongStr ) )
    {
	errStr.clear();
	rootPass.clear();
    }

    else if( errStr.startsWith( rootAskStr ) )
    {
	errStr.clear();
	//requestedPassword = true;
	emit RequestPassword();
	return;
    }
    else if( errStr == rootFailStr )
    {
	emit KillProcess();
	process->waitForFinished();//force kill the thing
	qDebug() << "killed sudo";
	emit SendFatalErr( errStr, witJob );
	errStr.clear();
	rootPass.clear();
	witJob = witNoJob;
    }
}

//triggered once the user entered their password
void WitHandler::PasswordIsEntered()
{
    //qDebug() << "wit: password entered" << witJob;
    if( witJob == witNoJob )
	return;

    //requestedPassword = false;
    QString pass = rootPass.isEmpty() ? "iLikeDicks" : rootPass; //crashes with an empty password
    process->write( QByteArray( pass.toLatin1() ) );
}

void WitHandler::ProcessFinishedSlot( int i, QProcess::ExitStatus s )
{
    //qDebug() << "wit finished:" << i << s << "job:" << witJob;
    if( i || s )
    {
#ifdef Q_OS_WIN
	if( i == 128 )
	{
	    emit SendFatalErr( tr( "Maybe cygwin1.dll is missing" ), witJob );
	    return;
	}
#endif
	if( s )
	{
	    emit SendFatalErr( tr( "Wit appears to have crashed" ), witJob );
	    return;
	}

	if( !errStr.isEmpty() )
	{
	    emit SendFatalErr( errStr, witJob );
	    return;
	}

	//clear old errors
	errStr.clear();
	witJob = witNoJob;
    }

    SendProgress( 100 );
    int witJobcopy = witJob;//make a copy and reset the original so we can emit signals and allow other processes to run
    witJob = witNoJob;
    switch( witJobcopy )
    {
	case witListLLLHDD:
	{
        QStringList parts = stdStr.split( "\n", Qt::SkipEmptyParts );

	    bool ok = false;
	    QList<QTreeWidgetItem *> games = StringListToGameList( parts, &ok );
	    if( ok )
	    {
		QString used;
		QTreeWidgetItem *size = games.takeLast();//the used space is saved as the last item in the list
		used = size->text( 0 );
		delete size;
		emit SendListLLL( games, used );
	    }
	}
	break;
	case witDump:
	{
	    QString idStr;
	    QString nameStr;
	    QStringList partitionOffsets;
	    QString type;
	    bool trucha = false;
	    //gather information about the game
	    QString gameInfo = stdStr;
	    gameInfo.remove( 0, gameInfo.indexOf( "Real path" ) );
	    gameInfo.resize( gameInfo.indexOf( "directories with") - 8 );
	    //qDebug() << "gameInfo:" << gameInfo;


	    int regionInt = -1;
	    bool isDataPartition = false;
	    int foundIos = 0;

        QStringList list = gameInfo.split("\n", Qt::SkipEmptyParts );
	    foreach( QString str, list )
	    {
		str = str.trimmed();
		if( str.contains( "ID & file type:" ) )
		{
		    str.remove( 0, 15 );
		    str = str.trimmed();
		    idStr = str;
		    idStr.resize( 6 );
		    str = str.trimmed();
		    str.remove( 0, 8 );
		    type = str.simplified();
		    if( !type.contains( "WII" ) && !type.contains( "GC" ))
			emit SendFatalErr( tr( "Unknown game type: %1" ).arg( type ), witDump );
		    //qDebug() << "id: " << idStr;
		    //qDebug() << "type:" << type;
		}
		else if( str.contains( "Disc name:" ) )
		{
		    str.remove( 0, 10 );
		    str = str.trimmed();
		    if( str.isEmpty() )
			str = "No Name";
		    nameStr = str;
		    //qDebug() << "name:" << nameStr;
		}
		else if( str.contains( "Region setting:" ) || str.contains( "BI2 Region:" ) )
		{
		    str.remove( 0, 15 );
		    str = str.trimmed();
		    str.resize( 1 );
		    //qDebug() << "region: " << str;
		    bool ok;
		    int v = str.toInt( &ok, 10 );
		    if( ok && v < 5 )
		    {
			regionInt = v;
		    }
		    //qDebug() << "region: " << regionInt;
		}
		else if( str.contains( "Partition table #0, " ) && str.contains( "type 0 [DATA]:" ) )
		{
		    isDataPartition = true;
		}
		else if( str.contains( "fake signed." ) && isDataPartition && !type.contains( "GC" ) )
		{
		    trucha = true;
		}
		else if( str.contains( "System version:" ) && isDataPartition && !type.contains( "GC" ) )
		{
		    isDataPartition = false;
		    str = str.simplified();
		    //str.remove( 0, 39 );
		    str.remove( 0, str.lastIndexOf( "IOS " ) + 4 );

		    bool ok;
		    int v = str.toInt( &ok, 10 );
		    if( ok && v < 255 && v > 3 )
		    {
			foundIos = v;
		    }
		    //qDebug() << "ios:" << foundIos;
		}
		else if( str.startsWith( "Data:" ) && !type.contains( "GC" ) )
		{
		    //not really needed, but just to be safe, simplify all the whitespace in the string
		    str = str.simplified();

		    //split the string into parts at each space
            QStringList parts = str.split(" ", Qt::SkipEmptyParts );

		    partitionOffsets << "** 0x" + parts.at( 5 ) +" **";
		}
	    }
	    //we found all the different info we need about the game, so change to that game
	    if( ( !( type.contains( "WII" ) && !idStr.isEmpty() && regionInt >= 0 && foundIos && !nameStr.isEmpty() )
		&& !( type.contains( "GC" ) && !idStr.isEmpty() && !nameStr.isEmpty() ) )
		|| type.isEmpty() )
	    {
		qDebug()<< "Invalid game listing" << type << idStr << nameStr << foundIos << regionInt;
		emit SendFatalErr( tr( "Invalid game listing" ), witDump );
		return;
	    }

	    int dashes = stdStr.indexOf( "-------------" );
	    int start = stdStr.indexOf( "\n", dashes ) + 1;
	    QString fileListing = stdStr.mid( start, stdStr.indexOf( "ISO Memory Map:" ) - start );
        QStringList files = fileListing.split( "\n", Qt::SkipEmptyParts );

	    emit SendGameInfo( type, idStr, nameStr, foundIos, regionInt, files, partitionOffsets, trucha );
	}
	break;
	case witCopy:
	case witEdit:
	    emit SendJobDone( witJobcopy );
	default:
	break;





    }

    //clear old errors
    errStr.clear();
}

void WitHandler::RunJob( QStringList args, int jobType )
{
    errStr.clear();
    stdStr.clear();
    if( witJob != witNoJob )
    {
	qDebug() <<  tr( "Wit is already running" );
	emit SendFatalErr( tr( "Wit is already running" ), jobType );
	return;
    }
    QString witPath = GetWitPath();
    if( witPath.isEmpty() )
    {
	qDebug() <<  tr( "Job aborted.");
	emit SendFatalErr( tr( "Job aborted."), jobType );
	return;
    }
    emit SendProgress( 0 );
    QString dbgtxt = witPath;
        foreach( QString arg, args )
        {
        if( arg.contains( " " ) )
            dbgtxt += " \'" + arg + "\'";
        else
            dbgtxt += " " + arg;
        }

        qDebug() << "<b>CMD:</b>" << dbgtxt;

    QString command = runAsRoot ? "sudo" : witPath;
    if( runAsRoot )
    {
	QStringList newArgs = QStringList() << "-S" << witPath;
	foreach( QString arg, args )
	    newArgs << arg;

	args = newArgs;
    }

    witJob = jobType;
    process->start( command, args );
    if( !process->waitForStarted() )//default timeout 30,000 msecs
    {
	qDebug() << "failed to start wit";
	SendFatalErr( tr( "Error starting wit!" ), jobType );
	return;
    }

    //qDebug() <<  tr( "Job started.");

}

//get the wit path from the settings and check that it exists
QString WitHandler::GetWitPath()
{
    QSettings settings( settingsPath, QSettings::IniFormat );
    QString ret = settings.value( "paths/wit" ).toString();
    if( ret.isEmpty() )
    {
	emit SendStdErr( tr("No path is set for wit.  Look in the settings.") );
    }
    if( !QFile::exists( ret ) )
    {
	emit SendStdErr( tr("Invalid path is set for wit.  Look in the settings.") );
    }

    return ret;
}

//LIST-LLL
void WitHandler::ListLLL_HDD( const QString &path, int recurse, bool ignoreFst )
{
    QString rDepth;
    QTextStream( &rDepth ) << "--rdepth=" << recurse;
#ifdef Q_OS_WIN//remove the drive letter in windows
    QStringList args = QStringList() << "LIST-LLL" << "--recurse" << RemoveDriveLetter( path ) << rDepth << "--sections";
#else
    QStringList args = QStringList() << "LIST-LLL" << "--recurse" << path << rDepth << "--sections";
#endif
    if( !titlesTxtPath.isEmpty() )
        args << "--titles=" + titlesTxtPath;

    if( ignoreFst )
        args << "--ignore-fst";

    //ignore uloader settings garbage
    args << "--exclude=__CFG_";

    //qDebug() << args;
    RunJob( args, witListLLLHDD );
}

//wait for wit to finish
bool WitHandler::Wait( int msecs )
{
    process->closeReadChannel( QProcess::StandardOutput );
    process->closeReadChannel( QProcess::StandardError );
    process->closeWriteChannel();
    return process->waitForFinished( msecs );
}
//send a kill message
void WitHandler::Kill()
{
    emit KillProcess();
}

//parse the text from LIST-LLL
QList<QTreeWidgetItem *> WitHandler::StringListToGameList( QStringList list, bool *okRet )
{

    int lines = list.size();
    bool ok = false;
    int gameCnt = -1;
    QString used;
    qint64 totalHddUsed = 0;
    int mode = 0;// 0 = hdd info, 1 = game info
    QList<QTreeWidgetItem *> games;

    QString id;
    QString name;
    QString sizeStr;
    QString region;
    QString type;
    QString partitionInfo;
    QString path;

    for( int j = 0; j < lines; j++ )
    {
	QString p = list.at( j );
	switch( mode )
	{
	case 0://get partition info
	    {
		if( p.startsWith( "total-discs=" ) )//gamecount
		{
		    p.remove( 0, p.indexOf( "=" ) + 1 );
		    gameCnt = p.toInt( &ok );
		    if( !ok || gameCnt < 0 )
		    {
			qDebug() << "count" << p;
			goto abort;
		    }
                    if( !gameCnt )//just a HDD with no games
                    {
                        QTreeWidgetItem *size = new QTreeWidgetItem();//just tack the size onto the end of the list for now
                        size->setText( 0, "0" );
                        games << size;
                        *okRet = true;
                        return games;
                    }
		    continue;
		}
		if( p.startsWith( "total-size=" ) )//total size
		{
		    p.remove( 0, p.indexOf( "=" ) + 1 );
		    totalHddUsed = p.toDouble( &ok );
		    if( !ok )
		    {
			qDebug() << "size";
			goto abort;
		    }
		    used = p;
		    if( gameCnt >= 0 )
			mode = 1;
		    continue;
		}
	    }
	    break;
	case 1://game info
	    {
		if( p.startsWith( "id=" ) )
		{
		    id = p;
		    id.remove( 0, id.indexOf( "=" ) + 1 );

		    if( namesFromWiiTDB )name = wiiTDB->NameFromID( id );
		    continue;
		}
		if( p.startsWith( "name=" ) && ( !namesFromWiiTDB || name.isEmpty() ) )
		{
		    name = p;
		    name.remove( 0, name.indexOf( "=" ) + 1 );
		    continue;
		}
		if( p.startsWith( "region=" ) )
		{
		    p.remove( 0, p.indexOf( "=" ) + 1 );
		    region = p;
		    continue;
		}
		if( p.startsWith( "size=" ) )
		{
		    p.remove( 0, p.indexOf( "=" ) + 1 );
		    sizeStr = p;
		    //qDebug() << "list-lll" << sizeStr << name;
		    continue;
		}
		if( p.startsWith( "filetype=" ) )
		{
		    p.remove( 0, p.indexOf( "=" ) + 1 );
		    type = p;
		    continue;
		}
		if( p.startsWith( "partition-info=" ) )
		{
		    p.remove( 0, p.indexOf( "=" ) + 1 );
		    partitionInfo = p;
		    continue;
		}
		if( p.startsWith( "source=" ) )
		{
		    p.remove( 0, p.indexOf( "=" ) + 1 );
		    path = p;

		    if( !id.isEmpty() && !name.isEmpty() && !sizeStr.isEmpty() && !region.isEmpty() && !type.isEmpty() && !partitionInfo.isEmpty() && !path.isEmpty() )
		    {
			QTreeWidgetItem *item = new QTreeWidgetItem();
			SetGameID( item, id );
			SetGameName( item, name );
			SetGameSize( item, sizeStr );
			SetGameRegion( item, region );
			SetGameType( item, type );
			SetGamePartitions( item, partitionInfo );
			SetGamePath( item, path );

			games << item;

			id.clear();
			name.clear();
			sizeStr.clear();
			region.clear();
			type.clear();
			partitionInfo.clear();
			path.clear();

			if( games.size() == gameCnt )
			{
			    QTreeWidgetItem *size = new QTreeWidgetItem();//just tack the size onto the end of the list for now
			    size->setText( 0, used );
			    games << size;
			    *okRet = true;
			    return games;
			}
		    }
		}
	    }
	    break;
	default:
	    qDebug() << "list-lll case";
	    goto abort;
	    break;
	}
    }
    qDebug() << "list-lll wtf";
abort:
    while( !games.isEmpty() )
    {
	QTreeWidgetItem *shit = games.takeFirst();
	delete shit;
    }
    *okRet = false;
    emit SendFatalErr( tr( "wit LIST-LLL returned invalid list of games" ), witListLLLHDD );
    return games;

}

//static public members
bool WitHandler::ReadAttributes()
{
    optionAttributes.clear();
    WitHandler w;
    QString witPath = w.GetWitPath();
    if( witPath.isEmpty() )
	return false;

    QProcess p;
    p.start( witPath, QStringList() << "INFO" << "--sections" );
    if( !p.waitForStarted() )
    {
	qDebug() << "failed to start wit ( read attributes )";
	return false;
    }

    if( !p.waitForFinished() )
    {
	qDebug() << "!p.waitForFinished() ( read attributes )";
	return false;
    }

    QString output = p.readAll();
#ifdef Q_OS_WIN
    output.remove( "\r" );
#endif
    QStringList list = output.split( "\n", Qt::SkipEmptyParts );
    if( list.isEmpty() )
    {
	qDebug() << "list.isEmpty() ( read attributes )";
	return false;
    }

    int num = 0;
    int mode = 0;
    QString opt;
    QString attr;
    QString ext;
    QString name;
    for( int i = 0; i < list.size(); i++ )
    {
	QString str = list.at( i );
	switch( mode )
	{
	case 0:
	    if( str.startsWith( "n=" ) )
	    {
		str.remove( 0, 2 );
		bool ok;
		num = str.toInt( &ok );
		if( !ok )
		    return false;

		mode = 1;
	    }
	    break;
	case 1:
	    if( str.startsWith( "name=" ) )
	    {
		name = str;
		name.remove( 0, 5 );
	    }
	    else if( str.startsWith( "option=" ) )
	    {
		opt = str;
		opt.remove( 0, 7 );
	    }
	    else if( str.startsWith( "extensions=" ) )
	    {
		ext = str;
		ext.remove( 0, 11 );
	    }
	    else if( str.startsWith( "attributes=" ) )
	    {
		attr = str;
		attr.remove( 0, 11 );
	    }
	    if( !opt.isEmpty() && !attr.isEmpty() && !ext.isEmpty() && !name.isEmpty() )
	    {
		WitOptionAttr newAttr;
		newAttr.name = name;
        newAttr.attributes = attr.split( " ", Qt::SkipEmptyParts );;
		newAttr.option = opt;
        newAttr.extensions = ext.split( " ", Qt::SkipEmptyParts );
		if( newAttr.extensions.isEmpty() )
		    newAttr.extensions << QString();

		optionAttributes << newAttr;
		opt.clear();
		attr.clear();
		ext.clear();
		name.clear();
	    }
	}
    }
    if( num != optionAttributes.size() )
    {
	qDebug() << "num != optionAttributes.size()";
	optionAttributes.clear();
	return false;
    }
    /*foreach( WitOptionAttr a, optionAttributes )
    {
	qDebug() << a.name << a.option << a.extensions << a.attributes;
    }*/

    return true;

}
bool WitHandler::NameSupportsAttribute( QString name, QString attr )
{
    //qDebug() << "WitHandler::NameSupportsAttribute" << name << attr;
    foreach( WitOptionAttr a, optionAttributes )
    {
	if( a.name != name )
	    continue;
	return a.attributes.contains( attr, Qt::CaseInsensitive );
    }
    return false;
}
bool WitHandler::OptionSupportsAttribute( QString opt, QString attr )
{
    foreach( WitOptionAttr a, optionAttributes )
    {
	if( a.option != opt )
	    continue;
	return a.attributes.contains( attr, Qt::CaseInsensitive );
    }
    return false;
}
bool WitHandler::ExtensionSupportsAttribute( QString ext, QString attr )
{
    foreach( WitOptionAttr a, optionAttributes )
    {
	if( !a.extensions.contains( ext, Qt::CaseInsensitive ) )
	    continue;
	return a.attributes.contains( attr, Qt::CaseInsensitive );
    }
    return false;
}
bool WitHandler::ReadVersion()
{
    witVersion = 0;
    witVersionString.clear();
    WitHandler w;
    QString witPath = w.GetWitPath();
    if( witPath.isEmpty() )
	return false;

    QProcess p;
    p.start( witPath, QStringList() << "VERSION" << "--sections" );
    if( !p.waitForStarted() )
    {
	qDebug() << "failed to start wit" << __FUNCTION__;
	return false;
    }

    if( !p.waitForFinished() )
    {
	qDebug() << "!p.waitForFinished()" << __FUNCTION__;
	return false;
    }

    QString output = p.readAll();
#ifdef Q_OS_WIN
    output.remove( "\r" );
#endif
    QStringList list = output.split( "\n", Qt::SkipEmptyParts );
    if( list.isEmpty() )
    {
	qDebug() << "list.isEmpty()" << __FUNCTION__;
	return false;
    }

    QString name;
    QString version;
    QString rev;
    QString sys;
    foreach( QString str, list )
    {
	if( str.startsWith( "prog=") )
	{
	    if( !str.endsWith( "=wit" ) )
	    {
                qDebug() << "wrong program" << __FUNCTION__ << str;
		return false;
	    }
	}
	else if( str.startsWith( "name=") )
	{
	    name = str;
	    name.remove( 0, 5 );
	    name.remove( "\"" );
	}
	else if( str.startsWith( "version=") )
	{
	    version = str;
	    version.remove( 0, 8 );
	}
	else if( str.startsWith( "revision=") )
	{
	    rev = str;
	    rev.remove( 0, 9 );
	}
	else if( str.startsWith( "system=") )
	{
	    sys = str;
	    sys.remove( 0, 7 );
	}
	if( !name.isEmpty() && !version.isEmpty() && !rev.isEmpty() && !sys.isEmpty() )
	{
	    bool ok;
	    witVersion = rev.toInt( &ok );
	    if( !ok )
		return false;

	    witVersionString = name + " " + version + " r" + rev + " " + sys;
        qDebug() << "<b><text style=\"color:green\">" << witVersionString << "</text></b>";
	    return true;
	}
    }
    qDebug() << "wtf" << __FUNCTION__;
    return false;
}
bool WitHandler::VersionIsOk()
{
    return witVersion >= WIT_MINIMUM_VERSION;
}
QString WitHandler::GetVersionString()
{
    return witVersionString;
}
QStringList WitHandler::FileType( QStringList files )
{
    if( !VersionIsOk() || !files.size() )
	return QStringList();

    WitHandler w;
    QString witPath = w.GetWitPath();
    if( witPath.isEmpty() )
	return QStringList();

    QProcess p;
    QStringList args = QStringList() << "FILETYPE" << "--no-header" << "--no-expand";
    foreach( QString file, files )
    {
	args << file;
    }

    p.start( witPath, args );
    if( !p.waitForStarted() )
    {
	qDebug() << "failed to start wit ( FileType )";
	return QStringList();
    }

    if( !p.waitForFinished() )
    {
	qDebug() << "!p.waitForFinished() ( FileType )";
	return QStringList();
    }

    QString output = p.readAll();
#ifdef Q_OS_WIN
    output.remove( "\r" );
#endif
    QStringList list = output.split( "\n", Qt::SkipEmptyParts );
    if( list.size() != files.size() )
    {
	qDebug() << "wrong size ( FileType )";
	return QStringList();
    }
    QStringList ret;
    for( int i = 0; i < files.size(); i++ )
    {
#ifdef Q_OS_WIN
        bool ok = false;
        QString cygPath = FsInfo::ToCygPath( files.at( i ), &ok );
        if( !ok || !list.at( i ).endsWith( cygPath ) )
#else
	if( !list.at( i ).endsWith( files.at( i ) ) )
#endif
	{
	    qDebug() << "( FileType ) mismatch"  <<  list.at( i ) << files.at( i );
	    return QStringList();
	}
	QString s = list.at( i );
	s.resize( s.indexOf( " " ) );
	ret << s;
    }
    return ret;
}


