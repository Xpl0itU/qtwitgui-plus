#include "wiitdb.h"
#ifndef Q_WS_MAC
#include "unzip/unzip.h"
#endif
#include "includes.h"

#include <QIODevice>

WiiTDB *wiiTDB;

WiiTDB::WiiTDB( QObject *parent ) : QObject( parent )
{
    //grab the language code to use for localized text
    localeStr = QLocale::system().name();
    if( localeStr.startsWith( "zh" ) )//some sort of chinese
    {
	if( localeStr == "zh_TW" )//taiwan
	    localeStr = "ZHTW";

	else// make all other chinese use the china version
	    localeStr = "ZHCH";
    }
    else // all other languages
    {
	localeStr.resize( 2 );
	localeStr = localeStr.toUpper();
    }

    //load the default wiitdb.xml
    //LoadFile( ":/wiitdb.xml" );
}

WiiTDB::~WiiTDB()
{
    if( file.isOpen() )
	file.close();

    if( buf.isOpen() )
	buf.close();
}

//load the wiitdb.xml file
bool WiiTDB::LoadFile( const QString &name )
{
    //qDebug() << "WiiTDB::LoadFile" << name;
    if( file.isOpen() )
	file.close();

    if( buf.isOpen() )
	buf.close();

    QString errorStr;
    int errorLine;
    int errorColumn;

    title = tr( "No WiiTDB available" );
    if( name.endsWith( ".xml", Qt::CaseInsensitive ) )
    {
	//open the file
	file.setFileName( name );
	if( !file.open(QFile::ReadOnly | QFile::Text ) )
	{
	    emit SendError( tr("WiiTDB Error"), tr("Cannot read file.") );
	    return false;
	}
	//point the dom to the file
	if( !domDocument.setContent( &file, true, &errorStr, &errorLine, &errorColumn ) )
	{
	    emit SendError( tr("WiiTDB Error"), tr("Parse error at line %1, column %2:\n%3").arg( errorLine ).arg( errorColumn ).arg( errorStr ) );
	    return false;
	}
    }
    else if( name.endsWith( ".zip", Qt::CaseInsensitive ) )
    {
#ifdef Q_WS_MAC
        emit SendError( tr("WiiTDB Error"), tr("Zip files are not supported in the OSx version of this program yet") );
        return false;
#else
	if( !QFile::exists( name ) )
	{
		emit SendError( tr("WiiTDB Error"), tr("Cannot find %1.").arg( name ) );
		return false;
	}

	UnZip::ErrorCode ec;
	UnZip uz;

	//try to open the zip archize
	ec = uz.openArchive( name );
	if (ec != UnZip::Ok)
	{
		emit SendError( tr("WiiTDB Error"), QString( "Failed to open archive: " ) + uz.formatError(ec).toAscii() );
		return false;
	}

	//see if it contains the correct file
	if( !uz.contains( "wiitdb.xml" ) )
	{
	    emit SendError( tr("WiiTDB Error"), tr( "wiitdb.xml not found in the archive." ) );
	    return false;
	}

	//open the memory oidevice
	if( !buf.open( QIODevice::ReadWrite ) )
	{
	    emit SendError( tr("WiiTDB Error"), tr( "Memory error." ) );
	    return false;
	}

	//extract that file to our memory buffer
	ec = uz.extractFile( "wiitdb.xml", &buf);
	if (ec != UnZip::Ok)
	{
		emit SendError( tr("WiiTDB Error"), QString( "Extraction failed: " ) + uz.formatError(ec).toAscii() );
		uz.closeArchive();
		return false;
	}

	//rewind back to the beginning
	buf.seek( 0 );

	//point the dom to the buffer
	if( !domDocument.setContent( &buf, true, &errorStr, &errorLine, &errorColumn ) )
	{
	    emit SendError( tr("WiiTDB Error"), tr("Parse error at line %1, column %2:\n%3").arg( errorLine ).arg( errorColumn ).arg( errorStr ) );
	    return false;
	}
#endif
    }
    else
    {
	qDebug() << name;
	emit SendError( tr("WiiTDB Error"), tr("Only .zip and .xml files are supported.") );
	return false;
    }
    //find and check root tag
    QDomElement root = domDocument.documentElement();
    if( root.tagName() != "datafile" )
    {
	emit SendError( tr("WiiTDB Error"), tr("The file is missing the \"datafile\" root tag.") );
	return false;
    }

    QDomElement child = root.firstChildElement( "WiiTDB" );
    if( child.isNull() || !child.hasAttribute( "version" ) || !child.hasAttribute( "games" ) )
    {
	emit SendError( tr("WiiTDB Error"), tr("Invalid \"WiiTDB\" tag.") );
	return false;
    }

    bool ok;
    xmlGameNum = child.attribute( "games" ).toInt( &ok );
    if( !ok )
    {
	emit SendError( tr("WiiTDB Error"), tr("Invalid attribute in the \"WiiTDB\" tag.") );
	return false;
    }
    title.clear();
    ClearGame();

    QString ver = child.attribute( "version" );
    ver.resize( 8 );
    xmlDate = QDate::fromString( ver, "yyyyMMdd");

    ver = child.attribute( "version" );
    ver.remove( 0, 8 );
    xmlTime = QTime::fromString( ver, "hhmmss" );

    return true;
}

bool WiiTDB::LoadGameFromID( const QString &id )
{
    if( !file.isOpen() && !buf.isOpen() )
	return false;
    if( id == id_loaded )
	return true;
    ClearGame();
    QDomElement e = GameFromID( id );
    if( e.isNull() )
    {
	if( id.at( 0 ) == QChar( '0' ) || id.at( 0 ) == QChar( '1' ) )//if this game starts with 0 or 1 then it is an autoboot game.  try to change it to R or S and see if the game exists
	{
	    QString i = id;
	    i[ 0 ] = QChar( 'R' );
	    e = GameFromID( i );
	    if( e.isNull() )
	    {
		i[ 0 ] = QChar( 'S' );
		e = GameFromID( i );
		if( e.isNull() )
		{
		    return false;
		}
		return false;
	    }
	}
	return false;
    }
    id_loaded = id;
    title = NameFromGameElement( e );
    synopsis = SynopsisFromGameElement( e );
    type = TypeFromGameElement( e );
    region = RegionFromGameElement( e );
    languages = LanguagesFromGameElement( e );
    developer = DeveloperFromGameElement( e );
    publisher = PublisherFromGameElement( e );
    releaseDate = DateFromGameElement( e );
    genre = GenreFromGameElement( e );
    ratingType = RatingTypeFromGameElement( e );
    ratingValue = RatingValueFromGameElement( e );
    wifiPlayers = WifiPlayersGameElement( e );
    wifiFeatures = WifiFeaturesFromGameElement( e );
    inputPlayers = InputPlayersFromGameElement( e );
    inputControlers = InputControllersFromGameElement( e );

    return true;
}

void WiiTDB::ClearGame()
{
    id_loaded.clear();
    title.clear();
    synopsis.clear();
    type.clear();
    region.clear();
    languages.clear();
    developer.clear();
    publisher.clear();
    releaseDate = QDate();
    genre.clear();
    ratingType.clear();
    ratingValue.clear();
    wifiPlayers = 0;
    wifiFeatures.clear();
    inputPlayers = 0;
    inputControlers.clear();
}

//get the "<game>" tag for a give ID
QDomElement WiiTDB::GameFromID( const QString &id )
{
    QDomElement root = domDocument.documentElement();
    QDomElement child = root.firstChildElement( "game" );
    if( child.isNull() )
    {
	emit SendError( tr("WiiTDB Error"), tr("No \"game\" tag found.") );
	return QDomElement();
    }

    while( !child.isNull() )
    {
	QDomElement idNode = child.firstChildElement( "id" );
	if( !idNode.isNull() )
	{
	    if( idNode.text() == id )
		return child;
	}
	child = child.nextSiblingElement( "game" );
    }

    return QDomElement();
}

//get a game from an ID
QString WiiTDB::NameFromID( const QString &id )
{
    if( !file.isOpen() )
	return QString();

    QDomElement e = GameFromID( id );
    if( e.isNull() )
	return QString();

    return NameFromGameElement( e );
}

//find shit from the game element
QString WiiTDB::NameFromGameElement( const QDomElement &parent )
{
    QDomElement localeElement = parent.firstChildElement( "locale" );
    while( !localeElement.isNull() )
    {
	if( localeElement.hasAttribute( "lang" ) && localeElement.attribute( "lang" ) == localeStr )
	{
	    QDomElement titleElement = localeElement.firstChildElement( "title" );
	    if( !titleElement.isNull() && !titleElement.text().isEmpty() )
	    {
		return titleElement.text();
	    }
	}
	localeElement = localeElement.nextSiblingElement( "locale" );
    }
    //localized language not found.  look for generic one
    if( parent.hasAttribute( "name") && !parent.attribute( "name" ).isEmpty() )
	return parent.attribute( "name" );

    return QString();
}

QString WiiTDB::SynopsisFromGameElement( const QDomElement &parent, const QString &locale )
{
    QDomElement localeElement = parent.firstChildElement( "locale" );
    QString enSynopsis;
    QString loc = locale.isEmpty() ? localeStr : locale; //if no language is given, use the one set in this PC settings
    while( !localeElement.isNull() )
    {
	if( localeElement.hasAttribute( "lang" ) )
	{
	    if( localeElement.attribute( "lang" ) == loc )
	    {
		QDomElement titleElement = localeElement.firstChildElement( "synopsis" );
		if( !titleElement.isNull() && !titleElement.text().isEmpty() )
		{
		    return titleElement.text();
		}
	    }
	    if( localeElement.attribute( "lang" ) == "EN" )
	    {
		QDomElement titleElement = localeElement.firstChildElement( "synopsis" );
		if( !titleElement.isNull() && !titleElement.text().isEmpty() )
		{
		    enSynopsis = titleElement.text();
		}
	    }
	}
	localeElement = localeElement.nextSiblingElement( "locale" );
    }
    //localized language not found.  return the english one
    return enSynopsis;
}

QString WiiTDB::TypeFromGameElement( const QDomElement &parent )
{
    QString ret;
    QDomElement e = parent.firstChildElement( "type" );
    if( !e.isNull() )
	ret = e.text();

    //as of nov-9-2010, wii games dont have a type string. they have an empty tag
    return ret;
}

QString WiiTDB::RegionFromGameElement( const QDomElement &parent )
{
    QString ret;
    QDomElement e = parent.firstChildElement( "region" );
    if( !e.isNull() )
	ret = e.text();

    return ret;
}

QString WiiTDB::LanguagesFromGameElement( const QDomElement &parent )
{
    QString ret;
    QDomElement e = parent.firstChildElement( "languages" );
    if( !e.isNull() )
	ret = e.text();

    return ret;
}

QString WiiTDB::DeveloperFromGameElement( const QDomElement &parent )
{
    QString ret;
    QDomElement e = parent.firstChildElement( "developer" );
    if( !e.isNull() )
	ret = e.text();

    return ret;
}

QString WiiTDB::PublisherFromGameElement( const QDomElement &parent )
{
    QString ret;
    QDomElement e = parent.firstChildElement( "publisher" );
    if( !e.isNull() )
	ret = e.text();

    return ret;
}

QDate WiiTDB::DateFromGameElement( const QDomElement &parent )
{
    QDate ret;
    QString str;
    QDomElement e = parent.firstChildElement( "date" );
    if( !e.isNull() )
    {
	if( e.hasAttribute( "year" ) )
	    str += e.attribute( "year" );
	else// no release year, just bail out
	    return ret;

	if( e.hasAttribute( "month" ) )
	    str += e.attribute( "month" ).rightJustified( 2, '0' );
	else//no month.  just make it january
	    str += "01";

	if( e.hasAttribute( "day" ) )
	    str += e.attribute( "day" ).rightJustified( 2, '0' );
	else//no day.  default to the first of the month
	    str += "01";
    }
    ret = QDate::fromString( str, "yyyyMMdd" );

    return ret;
}

QString WiiTDB::GenreFromGameElement( const QDomElement &parent )
{
    QString ret;
    QDomElement e = parent.firstChildElement( "genre" );
    if( !e.isNull() )
	ret = e.text();

    return ret;
}

QString WiiTDB::RatingTypeFromGameElement( const QDomElement &parent )
{
    QString ret;
    QDomElement e = parent.firstChildElement( "rating" );
    if( !e.isNull() && e.hasAttribute( "type" ) )
	ret = e.attribute( "type" );

    return ret;
}

QString WiiTDB::RatingValueFromGameElement( const QDomElement &parent )
{
    QString ret;
    QDomElement e = parent.firstChildElement( "rating" );
    if( !e.isNull() && e.hasAttribute( "value" ) )
	ret = e.attribute( "value" );

    return ret;
}

int WiiTDB::WifiPlayersGameElement( const QDomElement &parent )
{
    int ret = 0;
    bool ok;
    QString str;
    QDomElement e = parent.firstChildElement( "wi-fi" );
    if( !e.isNull() && e.hasAttribute( "players" ) )
    {
	str = e.attribute( "players" );
	ret = str.toInt( &ok, 10 );
	if( ok )
	    return ret;
    }

    return 0;
}

QString WiiTDB::WifiFeaturesFromGameElement( const QDomElement &parent )
{
    QString ret;
    QDomElement e = parent.firstChildElement( "wi-fi" );
    if( !e.isNull() )
    {
	QDomElement f = e.firstChildElement( "feature" );
	while( 1 )
	{
	    ret += f.text();
	    f = f.nextSiblingElement( "feature" );
	    if( !f.isNull() )
		ret += ", ";
	    else
		break;
	}
    }
    return ret;
}

int WiiTDB::InputPlayersFromGameElement( const QDomElement &parent )
{
    int ret = 0;
    bool ok;
    QString str;
    QDomElement e = parent.firstChildElement( "input" );
    if( !e.isNull() && e.hasAttribute( "players" ) )
    {
	str = e.attribute( "players" );
	ret = str.toInt( &ok, 10 );
	if( ok )
	    return ret;
    }

    return 1;
}

QMap<QString, bool> WiiTDB::InputControllersFromGameElement( const QDomElement &parent )
{
    QMap<QString, bool> ret;
    QDomElement e = parent.firstChildElement( "input" );
    if( !e.isNull() )
    {
	QDomElement c = e.firstChildElement( "control" );
	while( !c.isNull() )
	{
	    bool required = false;
	    QString type;
	    if( c.hasAttribute( "type") )
	    {
		type = c.attribute( "type" );
		if( c.hasAttribute( "required" ) )
		    required = c.attribute( "required" ) == "true";

		ret.insert( type, required );
	    }
	    c = c.nextSiblingElement( "control" );
	}
    }
    return ret;
}

QList< QTreeWidgetItem * >WiiTDB::Search( const QString &id, const QString &name, const QString &players, int playerCmpType, \
	const QString &wifiPlayers, int wifiCmpType, const QString &type, const QStringList &accessories, const QStringList &required,\
	const QString &ratingType, int ratingCmp, const QString &ratingVal, const QString &synopsisFilter, const QString &synopsisLang )
{
    QDomElement root = domDocument.documentElement();
    QDomElement child = root.firstChildElement( "game" );
    if( child.isNull() )
    {
	emit SendError( tr("WiiTDB Error"), tr("No \"game\" tag found.") );
	return QList< QTreeWidgetItem * >();
    }

    QList< QTreeWidgetItem * >ret;

    //only do these conversions 1 time
    QRegExp rxTitle( name, Qt::CaseInsensitive );
    QRegExp rxID( id, Qt::CaseInsensitive );
    QRegExp rxSynopsis( synopsisFilter, Qt::CaseInsensitive );
    bool okPlr = true;
    int plr = -1;
    if( !players.isEmpty() )
	plr = players.toInt( &okPlr );
    if( !okPlr )
	return ret;
    int plrWiFi = -1;
    if( !wifiPlayers.isEmpty() )
	plrWiFi = wifiPlayers.toInt( &okPlr );
    if( !okPlr )
	return ret;

    while( !child.isNull() )
    {
	QDomElement c = child;
	child = child.nextSiblingElement( "game" );

	QDomElement idNode = c.firstChildElement( "id" );
	if( idNode.isNull() )
	    continue;

	int curPly = InputPlayersFromGameElement( c );//check the int stuff first as it is quicker and move on to the slower checks
	if( !( players.isEmpty() || CheckPlayerRule( curPly, playerCmpType, plr ) ) )
	    continue;

	int curWPly = WifiPlayersGameElement( c );
	if( !( wifiPlayers.isEmpty() || CheckPlayerRule( curWPly, wifiCmpType, plrWiFi ) ) )
	    continue;

	QString curType = TypeFromGameElement( c );
	if( !( type.isEmpty() || type == curType ) )
	    continue;

	QString curID = idNode.text();
	if( !( id.isEmpty() || CheckRegEx( curID, rxID ) ) )
	    continue;

	QString title = NameFromGameElement( c );
	if( !( name.isEmpty() || CheckRegEx( title, rxTitle ) ) )
	    continue;

	QMap<QString, bool>acc = InputControllersFromGameElement( c );
	if( !CheckAccessories( acc, accessories, required ) )
	    continue;

	QString rType = RatingTypeFromGameElement( c );
	QString rVal = RatingValueFromGameElement( c );
	if( !( ratingType.isEmpty() || CheckRating( rType, rVal, ratingCmp, ratingType, ratingVal ) ) )
	    continue;

	if( !( synopsisLang.isEmpty() || CheckRegEx( SynopsisFromGameElement( c, synopsisLang ), rxSynopsis ) ) )//the synopsis should probably be the last check.  it is a regex check done on the longest string on every game
	    continue;

	QString plyStr = QString( "%1" ).arg( curPly );
	QString plyWStr = QString( "%1" ).arg( curWPly );
	QString accStr;
	QMapIterator<QString, bool> i( acc );//turn all the accessories into a string
	while( i.hasNext() )
	{
	    i.next();
	    accStr += i.key();
	    if( i.hasNext() )
		accStr += ", ";
	}

	//make a new item and add all the text to it
	QTreeWidgetItem *item = new QTreeWidgetItem( QStringList() << curID << title << plyStr << plyWStr );
	if( !rType.isEmpty() && !rVal.isEmpty())
	    item->setText( 4, rType + " : " + rVal );
	item->setText( 5, curType );
	item->setText( 6, accStr );
	ret << item;
    }
    return ret;
}

bool WiiTDB::CheckRegEx( const QString &text, const QRegExp &rx )
{
    if( text.isEmpty() )
	return false;

    if( !rx.isValid() )
	return text.contains( rx.pattern(), Qt::CaseInsensitive );

    return text.contains( rx );
}

bool WiiTDB::CheckPlayerRule( int num, int cmpType, int cmpval )
{
    switch( cmpType )
    {
    case 0:
	return num < cmpval;
	break;
    case 1:
	return num <= cmpval;
	break;
    case 2:
	return num == cmpval;
	break;
    case 3:
	return num >= cmpval;
	break;
    case 4:
	return num > cmpval;
	break;
    case 5:
	return num != cmpval;
	break;
    }
    //shouldnt happen
    return false;
}

bool WiiTDB::CheckAccessories( const QMap<QString, bool> &tags, const QStringList &acc, const QStringList &accReq )
{
    //check the supported accessories first
    foreach( QString str, acc )
    {
	if( tags.constFind( str ) == tags.constEnd() )//game doesnt support this accessory
	    return false;
    }

    //check the required accessories
    foreach( QString str, accReq )
    {
	QMap<QString, bool>::const_iterator it = tags.constFind( str );
	if( it == tags.constEnd() )//game doesnt support this accessory
	    return false;
	if( !it.value() )//accessory is not required to play this game
	    return false;
    }
    return true;
}

static int ConvertRatingToIndex( const QString &ratingtext )
{
    if( ratingtext == "CERO" )
	return 0;
    if( ratingtext == "ESRB" )
	return  1;
    if( ratingtext == "PEGI" )
	return  2;
    if( ratingtext == "GRB" )
	return  3;
    return -1;
}

QString WiiTDB::ConvertRating( const QString &fromType, const QString &fromVal, const QString &toType )
{
    if( fromType == toType )
    {
	return fromVal;
    }

    //set a default return value
    QString ret;
    if( toType == "CERO" )
	ret = "A";
    else if( toType == "ESRB" )
	ret = "E";
    else if( toType == "GRB" )
	ret = "ALL";
    else ret = "3";



    int type = -1;
    int desttype = -1;

    type = ConvertRatingToIndex( fromType );
    desttype = ConvertRatingToIndex( toType );
    if( type == -1 || desttype == -1 )
	return ret;

    /* rating conversion table */
    /* the list is ordered to pick the most likely value first: */
    /* EC and AO are less likely to be used so they are moved down to only be picked up when converting ESRB to PEGI or CERO */
    /* the conversion can never be perfect because ratings can differ between regions for the same game */
    char ratingtable[ 12 ][ 4 ][ 5 ] =
    { { { "A" }, { "E" }, { "3" }, { "ALL" } },
      { { "A" }, { "E" }, { "4" }, { "ALL" } },
      { { "A" }, { "E" }, { "6" }, { "ALL" } },
      { { "A" }, { "E" }, { "7" }, { "ALL" } },
      { { "A" }, { "EC" }, { "3" }, { "ALL" } },
      { { "A" }, { "E10+" }, { "7" }, { "ALL" } },
      { { "B" }, { "T" }, { "12" }, { "12" } },
      { { "D" }, { "M" }, { "18" }, { "18" } },
      { { "D" }, { "M" }, { "16" }, { "18" } },
      { { "C" }, { "T" }, { "16" }, { "15" } },
      { { "C" }, { "T" }, { "15" }, { "15" } },
      { { "Z" }, { "AO" }, { "18" }, { "18" } } };

    for( int i = 0; i < 12; i++ )
    {
	if( fromVal == ratingtable[ i ][ type ] )
	{
	    return ratingtable[ i ][ desttype ];
	}
    }
    return ret;

}

bool WiiTDB::CheckRating( const QString &testType, const QString &testVal, int oper, const QString &cmpType, const QString &cmpVal )
{
    if(  ( testType.isEmpty() || testVal.isEmpty() ) &&  oper != 2 )
    {
	return true;
    }
    QString convVal = ConvertRating( testType, testVal, cmpType );

    //test "==" and "!=" here before bothering to do any un-necessary stuff
    if( oper == 2 )
    {
	return convVal == cmpVal;
    }
    if( oper == 5 )
    {
	return convVal != cmpVal;
    }

    QStringList cmpVals;
    if( cmpType == "CERO" )
	cmpVals << "A" << "B" << "C" << "D" << "Z";
    else if( cmpType == "ESRB" )
	cmpVals << "EC" << "E" << "E10+" << "T" << "M" << "AO";
    else if( cmpType == "PEGI" )
	cmpVals << "3" << "4" << "5" << "6" << "7" << "12" << "15" << "16" << "18";
    else if( cmpType == "GRB" )
	cmpVals << "ALL" << "12" << "15" << "18";
    else
	return false;

    int size = cmpVals.size();
    switch( oper )
    {
    case 0:// <
	{
	    for( int i = 0; i < size; i++ )
	    {
		if( cmpVals.at( i ) == cmpVal )
		    return false;

		if( cmpVals.at( i ) == convVal )
		    return true;
	    }
	}
	break;
    case 1:// <=
	{
	    if( convVal == cmpVal )
		return true;

	    for( int i = 0; i < size; i++ )
	    {
		if( cmpVals.at( i ) == cmpVal )
		    return false;

		if( cmpVals.at( i ) == convVal )
		    return true;
	    }
	}
	break;
    case 3:// >=
	{
	    if( convVal == cmpVal )
		return true;

	    for( int i = size - 1 ; i >= 0; i-- )
	    {
		if( cmpVals.at( i ) == cmpVal )
		    return false;

		if( cmpVals.at( i ) == convVal )
		    return true;
	    }
	}
	break;
    case 4:// >
	{
	    for( int i = size - 1; i >= 0; i-- )
	    {
		if( cmpVals.at( i ) == cmpVal )
		    return false;

		if( cmpVals.at( i ) == convVal )
		    return true;
	    }
	}
	break;
    default:
	return false;
    }
    return false;

}


