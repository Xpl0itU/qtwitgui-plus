#include "tools.h"
#include "fsinfo.h"

QString settingsPath;

QString SizeTextGiB( QString bytes )
{
    QString ret;
    bool ok = false;
    qint64 i = bytes.toDouble( &ok );
    if( !ok )
	return ret;

    float f = (float)( (float)i / _GiB_ );
    QTextStream t( &ret );
    t.setRealNumberNotation( QTextStream::FixedNotation );
    t.setRealNumberPrecision( 2 );
    t << f;

    return ret;
}

QString gameID( QTreeWidgetItem* item )
{
    return item->text( GAME_ID_COLUMN );
}
QString gameName( QTreeWidgetItem* item )
{
    return item->text( GAME_NAME_COLUMN );
}
QString gameSize( QTreeWidgetItem* item )
{
    return item->text( GAME_SIZE_COLUMN );
}
QString gameRegion( QTreeWidgetItem* item )
{
    return item->text( GAME_REGION_COLUMN );
}
QString gameType( QTreeWidgetItem* item )
{
    return item->text( GAME_TYPE_COLUMN );
}
QString gamePartitions( QTreeWidgetItem* item )
{
    return item->text( GAME_PARTITIONS_COLUMN );
}
QString gamePath( QTreeWidgetItem* item )
{
    return item->text( GAME_PATH_COLUMN );
}
void SetGameID( QTreeWidgetItem* item, const QString &s )
{
    item->setText( GAME_ID_COLUMN , s );
}
void SetGameName( QTreeWidgetItem* item, const QString &s )
{
    item->setText( GAME_NAME_COLUMN , s );
}
void SetGameSize( QTreeWidgetItem* item, const QString &s )
{
    item->setText( GAME_SIZE_COLUMN , s );
}
void SetGameRegion( QTreeWidgetItem* item, const QString &s )
{
    item->setText( GAME_REGION_COLUMN , s );
}
void SetGameType( QTreeWidgetItem* item, const QString &s )
{
    item->setText( GAME_TYPE_COLUMN , s );
}
void SetGamePartitions( QTreeWidgetItem* item, const QString &s )
{
    item->setText( GAME_PARTITIONS_COLUMN , s );
}
void SetGamePath( QTreeWidgetItem* item, const QString &s )
{
    item->setText( GAME_PATH_COLUMN , s );
}

char ascii( char s ) {
    if ( s < 0x20 ) return '.';
    if ( s > 0x7E ) return '.';
    return s;
}
void hexdump( void *d, int len ) {
    unsigned char *data;
    int i, off;
    data = (unsigned char*)d;
    printf("\n");
    for ( off = 0; off < len; off += 16 ) {
	printf( "%08x  ", off );
	for ( i=0; i<16; i++ )
	    if ( ( i + off ) >= len ) printf("   ");
	    else printf("%02x ",data[ off + i ]);

	printf(" ");
	for ( i = 0; i < 16; i++ )
	    if ( ( i + off) >= len ) printf(" ");
	    else printf("%c", ascii( data[ off + i ]));
	printf("\n");
    }
    fflush( stdout );
}

QString warningColor = "#0000ff";
QString criticalColor = "#ff0000";
QPlainTextEdit *logWindow;
void SetupLog()
{
    logWindow = new QPlainTextEdit;
    logWindow->setReadOnly( true );
    QFont monoFont;
#ifdef Q_WS_WIN
    monoFont = QFont( "Courier New", QApplication::font().pointSize() );
#else
    monoFont = QFont( "Courier New", QApplication::font().pointSize() - 1 );
#endif
    logWindow->setFont( monoFont );
    QPalette p = logWindow->palette();

    QSettings s( settingsPath, QSettings::IniFormat );
    s.beginGroup( "log" );
    p.setColor( QPalette::Base, QColor( s.value( "bgColor", "#ffffff" ).toString() ) );
    p.setColor( QPalette::Text, QColor( s.value( "txtColor", "#000000" ).toString() ) );
    warningColor = s.value( "wrnColor", "#0000ff" ).toString();
    criticalColor = s.value( "crtColor", "#ff0000" ).toString();
    s.endGroup();

    logWindow->setPalette( p );
    qInstallMsgHandler( DebugHandler );
}

void DebugHandler( QtMsgType type, const char *msg )
{
    bool needToScroll = logWindow->verticalScrollBar()->value() == logWindow->verticalScrollBar()->maximum();//if the text window is already showing the last line
    switch( type )
    {
    case QtDebugMsg:
	logWindow->appendHtml( QString( msg ) );
	break;
    case QtWarningMsg:
	{
	    QString htmlString = "<b><text style=\"color:" + warningColor + "\">" + QString( msg ) + "</text></b>";
	    logWindow->appendHtml( htmlString );
	}
	break;
    case QtCriticalMsg:
	{
	    QString htmlString = "<b><text style=\"color:" + criticalColor + "\">" + QString( msg ) + "</text></b>";
	    logWindow->appendHtml( htmlString );
	}
	break;
    case QtFatalMsg:
	fprintf(stderr, "Fatal: %s\n", msg);
	abort();
	break;
    }
    if( needToScroll )//scroll to the new bottom of the screen
	logWindow->verticalScrollBar()->setValue( logWindow->verticalScrollBar()->maximum() );
}

#ifdef Q_WS_WIN//add the drive letter in windows
QString RemoveDriveLetter( const QString &path )
{
    QString ret = path;
    if( ret.contains( "\t" ) )
        ret.resize( ret.indexOf( "\t" ) );

    return ret;
}

QString AddDriveLetter( const QString &path )
{
    QString ret = path;
    bool ok = false;
    QString letter = FsInfo::ToWinPath( ret, &ok );
    if( ok && letter.endsWith( ":" ) )
        ret += "\t[ " + letter + " ]";

    return ret;

}

#endif
