#include "includes.h"
#include "wiitdbwindow.h"
#include "ui_wiitdbwindow.h"
#include "tools.h"

WiiTDBWindow::WiiTDBWindow(QWidget *parent) : QWidget(parent), ui(new Ui::WiiTDBWindow)
{
    setAttribute( Qt::WA_DeleteOnClose );
    ui->setupUi(this);
    ClearGui();

    ui->graphicsView->setScene( &gv );
    ui->graphicsView->setAlignment( Qt::AlignRight );
    ui->graphicsView->setRenderHint( QPainter::Antialiasing );


    QFontMetrics fm( fontMetrics() );
    //search tab
    int min = fm.width( ui->label_searchPlayers->text() );
    min = MAX( min, fm.width( ui->label_searchRating->text() ) );
    min = MAX( min, fm.width( ui->label_searchTitle->text() ) );
    min = MAX( min, fm.width( ui->label_searchID->text() ) );
    min = MAX( min, fm.width( ui->label_searchType->text() ) );
    min = MAX( min, fm.width( ui->label_searchWifiPlayers->text() ) );
    min = MAX( min, fm.width( ui->label_searchSynopsis->text() ) );
    min += 20;
    ui->label_searchPlayers->setMinimumWidth( min );
    ui->label_searchWifiPlayers->setMinimumWidth( min );
    ui->label_searchType->setMinimumWidth( min );
    ui->label_searchID->setMinimumWidth( min );
    ui->label_searchTitle->setMinimumWidth( min );
    ui->label_searchRating->setMinimumWidth( min );
    ui->label_searchSynopsis->setMinimumWidth( min );


    //search result list
    ui->treeWidget->sortItems( 1, Qt::AscendingOrder );//sort the name column in alphabetical order
    ui->treeWidget->header()->resizeSection( 0, fm.width( "WWWWWWW" ) );//id
    ui->treeWidget->header()->resizeSection( 1, fm.width( QString( 22, 'W' ) ) );//name
    ui->treeWidget->header()->resizeSection( 2, fm.width( "WW" ) );//#p
    ui->treeWidget->header()->resizeSection( 3, fm.width( "WW" ) );//#wifi
    ui->treeWidget->header()->resizeSection( 4, fm.width( "WWWWWWWW" ) );//rating
    ui->treeWidget->header()->resizeSection( 5, fm.width( "WWWWWWW" ) );//type
    ui->treeWidget->header()->resizeSection( 6, fm.width( QString( 22, 'W' ) ) );//accessories

	connect( wiiTDB, SIGNAL( SendError( const QString&, const QString &) ),
			 this, SLOT( ReceiveErrorFromWiiTDB( const QString&, const QString &) ) );

    //skip to the last viewed tab
    QSettings s( settingsPath, QSettings::IniFormat );
    ui->tabWidget->setCurrentIndex( s.value( "wiitdb/tab", 0 ).toInt() );
}

WiiTDBWindow::~WiiTDBWindow()
{
    QSettings s( settingsPath, QSettings::IniFormat );
    s.setValue( "wiitdb/tab", ui->tabWidget->currentIndex() );

    emit Destroyed();
    delete ui;
}

QString WiiTDBWindow::GetVersion()
{
    return wiiTDB->xmlDate.toString( "dd/MM/yyyy" ) + " : " + wiiTDB->xmlTime.toString( "hh:mm:ss" );
}

int WiiTDBWindow::GameCount()
{
    return wiiTDB->xmlGameNum;
}

void WiiTDBWindow::SetFile( const QString &path )
{
    wiiTDB->LoadFile( path );
}

void WiiTDBWindow::AddImage( const QPixmap &pm )
{
    QGraphicsPixmapItem *pmi = new QGraphicsPixmapItem( pm );
    gv.addItem( pmi );
}

void WiiTDBWindow::AddImage( const QString &str )
{
    if( !QFile::exists( str ) )
    {
	qDebug() << "bad filename:" << ui->label_id->text() << str;
        return;
    }
    QGraphicsPixmapItem *pmi = new QGraphicsPixmapItem( QPixmap( str ) );
    gv.addItem( pmi );
}

void WiiTDBWindow::ClearGui()
{
    ui->label_date->clear();
    ui->label_developer->clear();
    ui->label_genre->clear();
    ui->label_id->clear();
    ui->label_languages->clear();
    ui->label_publisher->clear();
    ui->label_region->clear();
    ui->label_title->clear();
    ui->label_type->clear();
    ui->label_wifiFeatures->clear();
    ui->textBrowser_synopsis->clear();
    foreach( QGraphicsItem *i, gv.items() )
    {
	gv.removeItem( i );
	delete( i );
    }
}

void WiiTDBWindow::LoadGameFromID( const QString &id )
{
    if( ui->label_id->text() == id )//dont bother to load a game thats already loaded
	return;

    ClearGui();
    ui->label_id->setText( id );

    if( !wiiTDB->LoadGameFromID( id ) )
    {
	return;
    }

    ui->label_date->setText( wiiTDB->releaseDate.toString( "ddd MMMM d yyyy" ) );
    ui->label_developer->setText( wiiTDB->developer );
    ui->label_genre->setText( wiiTDB->genre );
    ui->label_languages->setText( wiiTDB->languages );
    ui->label_publisher->setText( wiiTDB->publisher );
    ui->label_region->setText( wiiTDB->region );
    ui->label_title->setText( wiiTDB->title );
    ui->label_type->setText( wiiTDB->type );
    ui->label_wifiFeatures->setText( wiiTDB->wifiFeatures );
    ui->textBrowser_synopsis->setText( wiiTDB->synopsis );


    //add all the pretty icons

    //wiimote & num players
    QString str;
    if( wiiTDB->type == "GameCube" )
	QTextStream( &str ) << ":/images/gc" << wiiTDB->inputPlayers << ".png";
    else
	QTextStream( &str ) << ":/images/Wiimote" << wiiTDB->inputPlayers << ".png";
    AddImage( str );

    //other accessories
    QMap<QString, bool> map = wiiTDB->inputControlers;
    QMapIterator<QString, bool> i( map );
    while( i.hasNext() )
    {
	 i.next();
	 str = i.key();
	 if( str == "wiimote" )
	     continue;
	 else if( str == "nunchuk" )
	 {
	     AddImage( i.value() ? ":/images/nunchukR.png" : ":/images/nunchuk.png" );
	 }
	 else if( str == "motionplus" )
	 {
	     AddImage( i.value() ? ":/images/motionplusR.png" : ":/images/motionplus.png" );
	 }
	 else if( str == "gamecube" && wiiTDB->type != "GameCube" )
	 {
	     AddImage( i.value() ? ":/images/gamecubeR.png" : ":/images/gamecube.png" );
	 }
	 else if( str == "nintendods" )
	 {
	     AddImage( i.value() ? ":/images/nintendodsR.png" : ":/images/nintendods.png" );
	 }
	 else if( str == "classiccontroller" )
	 {
	     AddImage( i.value() ? ":/images/classiccontrollerR.png" : ":/images/classiccontroller.png" );
	 }
	 else if( str == "logitech force feedback" )
	 {
	     AddImage( i.value() ? ":/images/forceFeedbackR.png" : ":/images/forceFeedback.png" );
	 }
	 else if( str == "wheel" )
	 {
	     AddImage( i.value() ? ":/images/wheelR.png" : ":/images/wheel.png" );
	 }
	 else if( str == "zapper" )
	 {
	     AddImage( i.value() ? ":/images/zapperR.png" : ":/images/zapper.png" );
	 }
	 else if( str == "balanceboard" )
	 {
	     AddImage( i.value() ? ":/images/balanceboardR.png" : ":/images/balanceboard.png" );
	 }
	 else if( str == "wiispeak" )
	 {
	     AddImage( i.value() ? ":/images/wiispeakR.png" : ":/images/wiispeak.png" );
	 }
	 else if( str == "microphone" )
	 {
	     AddImage( i.value() ? ":/images/microphoneR.png" : ":/images/microphone.png" );
	 }
	 else if( str == "guitar" )
	 {
	     AddImage( i.value() ? ":/images/guitarR.png" : ":/images/guitar.png" );
	 }
	 else if( str == "drums" )
	 {
	     AddImage( i.value() ? ":/images/drumsR.png" : ":/images/drums.png" );
	 }
	 else if( str == "keyboard" )//same image for rockband3 keyboard and USB one for monsterhunter
	 {
	     AddImage( i.value() ? ":/images/keyboardR.png" : ":/images/keyboard.png" );
	 }
	 else if( str == "dancepad" )
	 {
	     AddImage( i.value() ? ":/images/dancepadR.png" : ":/images/dancepad.png" );
	 }
	 else if( str == "gameboy advance" )
	 {
	     AddImage( ":/images/gba.png" );//not using the gbaR.png.  i dont think there should be any games that actually require it
	 }
	 else if( str == "dk bongo" )
	 //else if( str == "dk bongo" || str == "bongo dk" )
	 {
	     AddImage( i.value() ? ":/images/bongosR.png" : ":/images/bongos.png" );
	 }
	 else if( str != "gamecube" )
	     qDebug() << "unhandled shit::" << id << i.key() << ": " << i.value();
    }
    //wi-fi players
    int n = wiiTDB->wifiPlayers;
    if( n )
    {
	str.clear();
	QTextStream( &str ) << ":/images/wifi" << n << ".png";
	AddImage( str );
    }
    //rating
    str = wiiTDB->ratingType;
    if( !str.isEmpty() )
    {
	AddImage( ":/images/" + str.toLower() + "_" + wiiTDB->ratingValue.toLower() + ".png" );
    }
    else AddImage( ":/images/norating.png" );


    // align all the images right to left, starting with the last one
    int hspace = 4;
    int ho = ui->graphicsView->width();
    for( int j = 0; j < gv.items().size(); j++ )
    {
	QGraphicsItem *i = gv.items().at( j );
	ho -= i->boundingRect().width() + hspace;

	i->setX( ho );
    }
}

//get an error from the wiitdb and display it as an error window
void WiiTDBWindow::ReceiveErrorFromWiiTDB( const QString &title, const QString &detail )
{
    QMessageBox::information( this, title, detail );
}

/**************
*
* Search stuff
*
**************/

//search "back" button clicked
void WiiTDBWindow::on_pushButton_searchBack_clicked()
{
    ui->stackedWidget->setCurrentIndex( 0 );
}

// "search" button clicked
void WiiTDBWindow::on_pushButton_search_clicked()
{
    //qDebug() << "WiiTDBWindow::on_pushButton_search_clicked()";
    int cnt = ui->treeWidget->topLevelItemCount();
    while( cnt-- > -1 )
    {
	QTreeWidgetItem *item = ui->treeWidget->takeTopLevelItem( cnt );
	delete item;
    }
    //add requested accessories to some lists to search
    QStringList acc;
    QStringList accReq;
    switch( ui->checkBox_searchBB->checkState() )//balanceboard
    {
    case 1: acc << "balanceboard";
	break;
    case 2: accReq << "balanceboard";
	break;
    default:break;
    }
    switch( ui->checkBox_searchBongos->checkState() )//bongos
    {
    case 1: acc << "dk bongo";
	break;
    case 2: accReq << "dk bongo";
	break;
    default:break;
    }
    switch( ui->checkBox_searchCC->checkState() )//classic control
    {
    case 1: acc << "classiccontroller";
	break;
    case 2: accReq << "classiccontroller";
	break;
	default:break;
    }
    switch( ui->checkBox_searchDDR->checkState() )//dance mat
    {
    case 1: acc << "dancepad";
	break;
    case 2: accReq << "dancepad";
	break;
	default:break;
    }
    switch( ui->checkBox_searchDS->checkState() )//gameboy DS
    {
    case 1: acc << "nintendods";
	break;
    case 2: accReq << "nintendods";
	break;
	default:break;
    }
    switch( ui->checkBox_searchGBA->checkState() )//gameboy advance
    {
    case 1: acc << "gameboy advance";
	break;
    case 2: accReq << "gameboy advance";
	break;
	default:break;
    }
    switch( ui->checkBox_searchGCN->checkState() )//gamecube controller
    {
    case 1: acc << "gamecube";
	break;
    case 2: accReq << "gamecube";
	break;
	default:break;
    }
    switch( ui->checkBox_searchGuitar->checkState() )//guitar
    {
    case 1: acc << "guitar";
	break;
    case 2: accReq << "guitar";
	break;
	default:break;
    }
    switch( ui->checkBox_searchKB->checkState() )//keyboard
    {
    case 1: acc << "keyboard";
	break;
    case 2: accReq << "keyboard";
	break;
	default:break;
    }
    switch( ui->checkBox_searchMic->checkState() )//microphone
    {
    case 1: acc << "microphone";
	break;
    case 2: accReq << "microphone";
	break;
	default:break;
    }
    switch( ui->checkBox_searchMP->checkState() )//motion plus
    {
    case 1: acc << "motionplus";
	break;
    case 2: accReq << "motionplus";
	break;
	default:break;
    }
    switch( ui->checkBox_searchNunchuk->checkState() )//nunchuk
    {
    case 1: acc << "nunchuk";
	break;
    case 2: accReq << "nunchuk";
	break;
	default:break;
    }
    switch( ui->checkBox_searchWheel->checkState() )//wii wheel
    {
    case 1: acc << "wheel";
	break;
    case 2: accReq << "wheel";
	break;
	default:break;
    }
    switch( ui->checkBox_searchWiimote->checkState() )//wiimote
    {
    case 1: acc << "wiimote";
	break;
    case 2: accReq << "wiimote";
	break;
	default:break;
    }
    switch( ui->checkBox_searchWS->checkState() )//wii speak
    {
    case 1: acc << "wiispeak";
	break;
    case 2: accReq << "wiispeak";
	break;
	default:break;
    }
    switch( ui->checkBox_search_7->checkState() )//zapper
    {
    case 1: acc << "zapper";
	break;
    case 2: accReq << "zapper";
	break;
	default:break;
    }
    switch( ui->checkBox_search_Drums->checkState() )//drums
    {
    case 1: acc << "drums";
	break;
    case 2: accReq << "drums";
	break;
	default:break;
    }
    switch( ui->checkBox_search_LFF->checkState() )//force feedback
    {
    case 1: acc << "logitech force feedback";
	break;
    case 2: accReq << "logitech force feedback";
	break;
	default:break;
    }

    QList< QTreeWidgetItem * >games = wiiTDB->Search( ui->lineEdit_search_id->text(), ui->lineEdit_searchTitle->text(), \
			    ui->lineEdit_searchPlayers->text(), ui->comboBox_searchPlayers->currentIndex(),\
			    ui->lineEdit_searchWiFiPlayers->text(), ui->comboBox_searchWiFiPlayers->currentIndex(),\
			    ui->comboBox_searchType->currentText(), acc, accReq, ui->comboBox_searchRatingType->currentText(),\
			    ui->comboBox_searchRatingCmp->currentIndex(), ui->comboBox_searchRatingValue->currentText(),\
			    ui->lineEdit_searchSynopsis->text(), ui->comboBox_searchSynopsisLang->currentText() );
    int size = games.size();
    if( !size )
	ui->label_searchResultInfo->setText( tr( "No entries matched the search terms" ) );
    else
    {
	if( size == 1 )
	    ui->label_searchResultInfo->setText( tr( "%1 search result" ).arg( 1 ) );
	else
	    ui->label_searchResultInfo->setText( tr( "%1 search results" ).arg( size ) );

	ui->treeWidget->addTopLevelItems( games );
    }
    ui->stackedWidget->setCurrentIndex( 1 );
}

//search result is clicked
void WiiTDBWindow::on_treeWidget_itemClicked( QTreeWidgetItem* item, int column )
{
    Q_UNUSED( column );
    if( !item )
	return;
    LoadGameFromID( item->text( 0 ) );
}

//different search result highlited
void WiiTDBWindow::on_treeWidget_currentItemChanged( QTreeWidgetItem* current, QTreeWidgetItem* previous )
{
    Q_UNUSED( previous );
    if( !current )
	return;
    LoadGameFromID( current->text( 0 ) );
}

//search checkboxes
//wiimote
void WiiTDBWindow::on_checkBox_searchWiimote_stateChanged( int i )
{
    QString p = i == 2 ? ":/images/WiimoteR.png" : ":/images/Wiimote.png";
    ui->checkBox_searchWiimote->setIcon( QIcon( p ) );
}
//nunchuk
void WiiTDBWindow::on_checkBox_searchNunchuk_stateChanged( int i )
{
    QString p = i == 2 ? ":/images/nunchukR.png" : ":/images/nunchuk.png";
    ui->checkBox_searchNunchuk->setIcon( QIcon( p ) );
}
//zapper
void WiiTDBWindow::on_checkBox_search_7_stateChanged( int i )
{
    QString p = i == 2 ? ":/images/zapperR.png" : ":/images/zapper.png";
    ui->checkBox_search_7->setIcon( QIcon( p ) );
}
//balanceboard
void WiiTDBWindow::on_checkBox_searchBB_stateChanged( int i )
{
    QString p = i == 2 ? ":/images/balanceboardR.png" : ":/images/balanceboard.png";
    ui->checkBox_searchBB->setIcon( QIcon( p ) );
}
//bongos
void WiiTDBWindow::on_checkBox_searchBongos_stateChanged( int i )
{
    QString p = i == 2 ? ":/images/bongosR.png" : ":/images/bongos.png";
    ui->checkBox_searchBongos->setIcon( QIcon( p ) );
}
//motionplus
void WiiTDBWindow::on_checkBox_searchMP_stateChanged( int i )
{
    QString p = i == 2 ? ":/images/motionplusR.png" : ":/images/motionplus.png";
    ui->checkBox_searchMP->setIcon( QIcon( p ) );
}
//classic controller
void WiiTDBWindow::on_checkBox_searchCC_stateChanged( int i )
{
    QString p = i == 2 ? ":/images/classiccontrollerR.png" : ":/images/classiccontroller.png";
    ui->checkBox_searchCC->setIcon( QIcon( p ) );
}
//wheel
void WiiTDBWindow::on_checkBox_searchWheel_stateChanged( int i )
{
    QString p = i == 2 ? ":/images/wheelR.png" : ":/images/wheel.png";
    ui->checkBox_searchWheel->setIcon( QIcon( p ) );
}
//DDR mat
void WiiTDBWindow::on_checkBox_searchDDR_stateChanged( int i )
{
    QString p = i == 2 ? ":/images/dancepadR.png" : ":/images/dancepad.png";
    ui->checkBox_searchDDR->setIcon( QIcon( p ) );
}
//gamecube controller
void WiiTDBWindow::on_checkBox_searchGCN_stateChanged( int i )
{
    QString p = i == 2 ? ":/images/gamecubeR.png" : ":/images/gamecube.png";
    ui->checkBox_searchGCN->setIcon( QIcon( p ) );
}
//guitar
void WiiTDBWindow::on_checkBox_searchGuitar_stateChanged( int i )
{
    QString p = i == 2 ? ":/images/guitarR.png" : ":/images/guitar.png";
    ui->checkBox_searchGuitar->setIcon( QIcon( p ) );
}
//drums
void WiiTDBWindow::on_checkBox_search_Drums_stateChanged( int i )
{
    QString p = i == 2 ? ":/images/drumsR.png" : ":/images/drums.png";
    ui->checkBox_search_Drums->setIcon( QIcon( p ) );
}
//microphone
void WiiTDBWindow::on_checkBox_searchMic_stateChanged( int i )
{
    QString p = i == 2 ? ":/images/microphoneR.png" : ":/images/microphone.png";
    ui->checkBox_searchMic->setIcon( QIcon( p ) );
}
//gameboy advance
void WiiTDBWindow::on_checkBox_searchGBA_stateChanged( int i )
{
    QString p = i == 2 ? ":/images/gbaR.png" : ":/images/gba.png";
    ui->checkBox_searchGBA->setIcon( QIcon( p ) );
}
//keyboard
void WiiTDBWindow::on_checkBox_searchKB_stateChanged( int i )
{
    QString p = i == 2 ? ":/images/keyboardR.png" : ":/images/keyboard.png";
    ui->checkBox_searchKB->setIcon( QIcon( p ) );
}
//wiispeak
void WiiTDBWindow::on_checkBox_searchWS_stateChanged( int i )
{
    QString p = i == 2 ? ":/images/wiispeakR.png" : ":/images/wiispeak.png";
    ui->checkBox_searchWS->setIcon( QIcon( p ) );
}
//gameboy DS
void WiiTDBWindow::on_checkBox_searchDS_stateChanged( int i )
{
    QString p = i == 2 ? ":/images/nintendodsR.png" : ":/images/nintendods.png";
    ui->checkBox_searchDS->setIcon( QIcon( p ) );
}
//gamecube wheel
void WiiTDBWindow::on_checkBox_search_LFF_stateChanged( int i )
{
    QString p = i == 2 ? ":/images/forceFeedbackR.png" : ":/images/forceFeedback.png";
    ui->checkBox_search_LFF->setIcon( QIcon( p ) );
}

//rating type combobox
void WiiTDBWindow::on_comboBox_searchRatingType_currentIndexChanged( int index )
{
    ui->comboBox_searchRatingValue->clear();
    switch( index )
    {
    case 1://esrb
	ui->comboBox_searchRatingValue->addItem( "EC" );
	ui->comboBox_searchRatingValue->addItem( "E" );
	ui->comboBox_searchRatingValue->addItem( "E 10+" );
	ui->comboBox_searchRatingValue->addItem( "T" );
	ui->comboBox_searchRatingValue->addItem( "M" );
	ui->comboBox_searchRatingValue->addItem( "AO" );

	ui->comboBox_searchRatingCmp->setEnabled( true );
	ui->comboBox_searchRatingValue->setEnabled( true );
	break;
    case 2://pegi
	ui->comboBox_searchRatingValue->addItem( "3" );
	ui->comboBox_searchRatingValue->addItem( "7" );
	ui->comboBox_searchRatingValue->addItem( "12" );
	ui->comboBox_searchRatingValue->addItem( "15" );
	ui->comboBox_searchRatingValue->addItem( "16" );
	ui->comboBox_searchRatingValue->addItem( "18" );

	ui->comboBox_searchRatingCmp->setEnabled( true );
	ui->comboBox_searchRatingValue->setEnabled( true );
	break;
    case 3://cero
	ui->comboBox_searchRatingValue->addItem( "A" );
	ui->comboBox_searchRatingValue->addItem( "B" );
	ui->comboBox_searchRatingValue->addItem( "C" );
	ui->comboBox_searchRatingValue->addItem( "D" );
	ui->comboBox_searchRatingValue->addItem( "Z" );

	ui->comboBox_searchRatingCmp->setEnabled( true );
	ui->comboBox_searchRatingValue->setEnabled( true );
	break;
    case 4://grb
	ui->comboBox_searchRatingValue->addItem( "ALL" );
	ui->comboBox_searchRatingValue->addItem( "12" );
	ui->comboBox_searchRatingValue->addItem( "15" );
	ui->comboBox_searchRatingValue->addItem( "18" );

	ui->comboBox_searchRatingCmp->setEnabled( true );
	ui->comboBox_searchRatingValue->setEnabled( true );
	break;

    default:
	ui->comboBox_searchRatingCmp->setEnabled( false );
	ui->comboBox_searchRatingValue->setEnabled( false );
	break;
    }
}
//missing images
/*

bad filename: "SMNC01" ":/images/cero_e.png"//wiitdb errors
bad filename: "GANP7U" ":/images/esrb_3.png"
bad filename: "RUHX52" ":/images/esrb_3.png"
bad filename: "RQPZ52" ":/images/esrb_3.png"
bad filename: "RGHI52" ":/images/esrb_3.png"
bad filename: "RU5Y4Q" ":/images/esrb_3.png"
unhandled shit:: "GB4E51" "logitec force feedback" :  false
bad filename: "RC7E20" ":/images/esrb_e 10+.png"
bad filename: "R6FERS" ":/images/esrb_e 10+.png"
bad filename: "R48E7D" ":/images/esrb_e 10+.png"
unhandled shit:: "SWBE52" "turn table" :  false//only 1 game uses this
bad filename: "GFXE5D" ":/images/gc9.png"
unhandled shit:: "R8OX54" "vitalitysensor" :  true //this hasnt even been invented yet
unhandled shit:: "STYP52" "board controller" :  false //tony hawk board
*/

//synopsis combobox index changed
void WiiTDBWindow::on_comboBox_searchSynopsisLang_currentIndexChanged( int index )
{
    ui->lineEdit_searchSynopsis->setEnabled( index );
}
