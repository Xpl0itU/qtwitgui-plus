#include "includes.h"
#include "wwthandler.h"
#include "tools.h"

extern QString rootPass;
extern QString rootAskStr;
extern QString rootWrongStr;
extern QString rootFailStr;

static int wwtVersion = 0;
static QString wwtVersionString;

WwtHandler::WwtHandler( QObject *parent, bool root ) :QObject( parent )
{
    runAsRoot = root;
    //requestedPassword = false;
    wwtJob = wwtNoJob;
    //create the pointer to the process used to run wit
    process = new QProcess( this );

#ifdef Q_WS_WIN
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

WwtHandler::~WwtHandler()
{
    if( wwtJob != wwtNoJob )
    {
		emit KillProcess();
    }
    delete process;
    process = 0;
}

void WwtHandler::SetRunAsRoot( bool root )
{
    runAsRoot = root;
}

void WwtHandler::ReadyReadStdOutSlot()
{
    //read text from wwt
    QString curRead = process->readAllStandardOutput();
#ifdef Q_WS_WIN
    //get rid of stupid windows new lines
    curRead.replace( "\r\n", "\n" );
#endif
    stdStr += curRead;

    //qDebug() << "gotmessage out" << curRead;

    switch ( wwtJob )
    {
	case wwtAdd:
	    //curRead.remove( "\n" );
	    //turn the % message into a int and pass it to the progress bar
	    if( curRead.contains( "%" ) )
	    {
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
				if( num < 101 )
					//ui->progressBar->setValue( num );
					emit SendProgress( num );
			}

			num = str.indexOf( "ETA", 0 );
			if( num > 1 )
			{
				str.remove( 0, num );
				emit SendMessageForStatusBar( currJobText.isEmpty() ? str :\
											  currJobText + " : " + str );
			}
			break;
	    }
	    else//text doesnt contain a "%" sign
	    {
			if( curRead.contains( "already exists ->" ) )//show ignored games
			{
				int start  = curRead.indexOf( " - DISC");
				int end = curRead.lastIndexOf( "ignore" );
				if( start < end )
				{
					end += 7;
					QString stuff = curRead.mid( start, end - start );
					stuff = stuff.trimmed();
					stuff.replace( "\n", "<br>" );
					qWarning() << qPrintable( stuff );
				}
			}
			if( curRead.contains( "- REMOVE" ) )
			{
				int start = curRead.indexOf( " - REMOVE " );
				int end = curRead.indexOf( "]", start ) + 1;
				if( start < end )
					qWarning() << qPrintable( curRead.mid( start, end - start ) );//show what game is being deleted

			}
			if( curRead.contains( " - ADD " ) )
			{
				int start = curRead.indexOf( " - ADD " );
				int start2 = start + 7;
				int end = curRead.indexOf( " ",start2 );
				currJobText = curRead.mid( start2, end - start2 );
				qWarning() << qPrintable( curRead.mid( start ) );//show what game is being written
				emit SendMessageForStatusBar( currJobText );
				emit SendProgress( 0 );
			}
			if( curRead.contains( "copied in" ) )//game is done writing
			{
				currJobText.clear();
				qWarning() << qPrintable( curRead.trimmed() );
				emit SendProgress( 100 );
			}
			if( curRead.contains( "disc added." ) || curRead.contains( "discs added." ) )//job done
			{
				int start = curRead.lastIndexOf( "* WBFS" );
				qWarning() << qPrintable( curRead.mid( start ).trimmed() );
			}

	    }
	    //qDebug() << "unhandled wwtAdd text:" << curRead;
	    break;
	    case wwtRemove:
	    {
			int start = curRead.indexOf( "WBFSv" );
			if( start >= 0 )
			{
				qWarning() << qPrintable( curRead.mid( start ).trimmed().replace( "\n", "<br>" ) );
			}
	    }
	    break;
	default:
	    break;
    }


}

void WwtHandler::ReadyReadStdErrSlot()
{
    //read text from wwt
    errStr += process->readAllStandardError();

    //qDebug() << "gotmessage err" << errStr;
#ifdef Q_WS_WIN
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
		if( !rootPass.isEmpty() )
		{
			process->write( QByteArray( rootPass.toLatin1() ) );
		}
		else
		{
			emit RequestPassword();
		}
		return;
    }
    else if( errStr == rootFailStr )
    {
		emit KillProcess();
		process->waitForFinished();//force kill the thing
		qDebug() << "killed sudo";
		emit SendFatalErr( errStr, wwtJob );
		errStr.clear();
		rootPass.clear();
		wwtJob = wwtNoJob;
    }
}

void WwtHandler::ProcessFinishedSlot( int i, QProcess::ExitStatus s )
{
    //qDebug() << "wwt finished:" << i << s << "job:" << wwtJob;
    if( i || s )
    {
#ifdef Q_WS_WIN
		if( i == 128 )
		{
			emit SendFatalErr( tr( "Maybe cygwin1.dll is missing" ), wwtJob );
			errStr.clear();
			wwtJob = wwtNoJob;
			return;
		}
#endif
		if( s )
		{
			emit SendFatalErr( tr( "Wwt appears to have crashed" ), wwtJob );
			errStr.clear();
			wwtJob = wwtNoJob;
			return;
		}

		if( !errStr.isEmpty() )
		{
			emit SendFatalErr( errStr, wwtJob );
			errStr.clear();
			wwtJob = wwtNoJob;
			return;
		}

		if( i == 4 )
		{
			emit SendFatalErr( tr( "At least 1 operation was ignored.<br>Maybe you didn't set the \"overwrite\" option?" ), wwtJob );
			errStr.clear();
			wwtJob = wwtNoJob;
			return;
		}
    }

    SendProgress( 100 );
    switch( wwtJob )
    {
	case wwtFind:
		{
			QStringList parts = stdStr.split( "\n", QString::SkipEmptyParts );
#ifdef Q_WS_MAC//mac version prints out more than just the list we want, so remove all entries that dont start with "/dev/"
			int s = parts.size();
			QStringList filtered;
			for( int i = 0; i < s; i++ )
			{
				if( parts.at( i ).startsWith( "/dev/") )
					filtered << parts.at( i );
			}
			parts = filtered;
#endif
			emit SendPartitionList( parts );
		}
		break;
	case wwtFind_long:
		{
			QStringList parts = stdStr.split( "\n", QString::SkipEmptyParts );
#ifdef Q_WS_MAC//mac version prints out more than just the list we want, so remove all entries that dont start with "CHAR"
			int s = parts.size();
			QStringList filtered;
			for( int i = 0; i < s; i++ )
			{
				if( parts.at( i ).startsWith( "CHAR") )
					filtered << parts.at( i );
			}
			parts = filtered;
#endif
			emit SendPartitionList( parts );
		}
		break;

	case wwtAdd:
	case wwtRemove:
		{
			emit SendJobDone( wwtJob );
		}
		break;
	case wwtFormat:
		{
			stdStr.remove( 0xC );//IDK where this comes from, but there is a byte in the wwt output that makes no sense to me.  just delete it
			emit SendStdOut( stdStr );
			//emit SendJobDone( wwtFormat );
		}
		break;

	default:
		break;
    }

    //clear old errors
    errStr.clear();
    wwtJob = wwtNoJob;
}

//triggered once the user entered their password
void WwtHandler::PasswordIsEntered()
{
    //qDebug() << "wwt: password entered" << requestedPassword;
    if( wwtJob == wwtNoJob )
		return;

    //requestedPassword = false;
    QString pass = rootPass.isEmpty() ? "iLikeDicks" : rootPass; //crashes with an empty password
    process->write( QByteArray( pass.toLatin1() ) );
}

void WwtHandler::RunJob( QStringList args, int jobType )
{
    errStr.clear();
    stdStr.clear();
    if( wwtJob != wwtNoJob )
    {
		emit SendFatalErr( tr( "Wwt is already running" ), jobType );
		return;
    }
    QString wwtPath = GetWwtPath();
    if( wwtPath.isEmpty() )
    {
		emit SendFatalErr( tr( "Job aborted."), jobType );
		return;
    }
    emit SendProgress( 0 );

    QString dbgtxt = wwtPath;
    foreach( QString arg, args )
    {
		if( arg.contains( " " ) )
			dbgtxt += " \'" + arg + "\'";
		else
			dbgtxt += " " + arg;
    }

    qDebug() << "<b>CMD:</b>" << dbgtxt;

    QString command = runAsRoot ? "sudo" : wwtPath;
    if( runAsRoot )
    {
		QStringList newArgs = QStringList() << "-S" << wwtPath;
		foreach( QString arg, args )
			newArgs << arg;

		args = newArgs;
    }

    wwtJob = jobType;
    process->start( command, args );
    if( !process->waitForStarted() )//default timeout 30,000 msecs
    {
		qCritical() << "failed to start wwt";
		SendFatalErr( tr( "Error starting wwt!" ), jobType );
    }

}

void WwtHandler::GetPartitions( bool verbose )
{
    //qDebug() << "WwtHandler::GetPartitions" << verbose;
    QStringList args = QStringList() << "FIND";
    if( !verbose )
    {
		RunJob( args, wwtFind );
		return;
    }
    else
    {
		args << "-ll" << "--no-header";
		RunJob( args, wwtFind_long );
    }
}

//get the wwt path from the settings and check that it exists
QString WwtHandler::GetWwtPath()
{
    QSettings settings( settingsPath, QSettings::IniFormat );
    QString ret = settings.value( "paths/wwt" ).toString();
    if( ret.isEmpty() )
    {
		emit SendFatalErr( tr("No path is set for wwt.  Look in the settings."), wwtJob );

    }
    else if( !QFile::exists( ret ) )
    {
		emit SendFatalErr( tr("Invalid path is set for wwt.  Look in the settings."), wwtJob );
    }

    return ret;
}

//wait for wit to finish
bool WwtHandler::Wait( int msecs )
{
    process->closeReadChannel( QProcess::StandardOutput );
    process->closeReadChannel( QProcess::StandardError );
    process->closeWriteChannel();
    return process->waitForFinished( msecs );
}

//send a kill message
void WwtHandler::Kill()
{
    emit KillProcess();
}

//static public functions
bool WwtHandler::ReadVersion()
{
    wwtVersion = 0;
    wwtVersionString.clear();
    WwtHandler w;
    QString wwtPath = w.GetWwtPath();
    if( wwtPath.isEmpty() )
    {
		qWarning() << "path is empty" << __FUNCTION__;
		return false;
    }

    QProcess p;
    p.start( wwtPath, QStringList() << "VERSION" << "--sections" );
    if( !p.waitForStarted() )
    {
		qWarning() << "failed to start wwt" << __FUNCTION__;
		return false;
    }

    if( !p.waitForFinished() )
    {
		qWarning() << "!p.waitForFinished()" << __FUNCTION__;
		return false;
    }

    QString output = p.readAll();
#ifdef Q_WS_WIN
    output.remove( "\r" );
#endif
    QStringList list = output.split( "\n", QString::SkipEmptyParts );
    if( list.isEmpty() )
    {
		qWarning() << "list.isEmpty()" << __FUNCTION__;
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
			if( !str.endsWith( "=wwt" ) )
			{
				qWarning() << "wrong program" << __FUNCTION__;
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
			wwtVersion = rev.toInt( &ok );
			if( !ok )
				return false;

			wwtVersionString = name + " " + version + " r" + rev + " " + sys;
			return true;
		}
    }
    qWarning() << "wtf" << __FUNCTION__;
    return false;
}

bool WwtHandler::VersionIsOk()
{
    return wwtVersion >= WWT_MINIMUM_VERSION;
}

QString WwtHandler::GetVersionString()
{
    return wwtVersionString;
}
