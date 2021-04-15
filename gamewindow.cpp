#include "gamewindow.h"
#include "ui_gamewindow.h"
#include "includes.h"
#include "gamecopydialog.h"
#include "tools.h"
#include "savedialog.h"
#include "gc_shrinkthread.h"


GameWindow::GameWindow( QWidget *parent, const QString &game, const QList<QTreeWidgetItem *> &pList )
	: QMainWindow( parent ), ui( new Ui::GameWindow )
{
    ui->setupUi(this);
    ui->menubar->hide();
    ui->progressBar->setVisible( false );
    ui->statusbar->addPermanentWidget( ui->progressBar, 0 );
    setAcceptDrops( true );
    busy = false;
    SetPartitionList( pList );

    QSettings set( settingsPath, QSettings::IniFormat );
    bool root = set.value( "root/enabled" ).toBool();
    wit.SetRunAsRoot( root );
    wwt.SetRunAsRoot( root );
    wit.SetTitlesTxtPath( set.value( "paths/titlesTxt", "" ).toString() );

    //lockTextOutput = false;

    //set a font for the output window.
#ifdef Q_WS_WIN
    QFont monoFont = QFont( "Courier New", QApplication::font().pointSize(), 55 );
#else
    QFont monoFont = QFont( "Courier New", QApplication::font().pointSize() - 1, 55 );
#endif
    ui->textEdit->setFont( monoFont );

    wiithread = new WiiTreeThread;

    //create stuff for the tree window
	//icons
    groupIcon.addPixmap( style()->standardPixmap( QStyle::SP_DirClosedIcon ), QIcon::Normal, QIcon::Off );
    groupIcon.addPixmap( style()->standardPixmap( QStyle::SP_DirOpenIcon ), QIcon::Normal, QIcon::On );
    keyIcon.addPixmap( style()->standardPixmap( QStyle::SP_FileIcon ) );

	//make sure the name column is wide enough
    ui->treeWidget->header()->resizeSection( 0, 300 );
    ui->treeWidget->header()->resizeSection( 1, QFontMetrics( monoFont ).width( "** 0xf820000 **" ) + 10 );

	//connect the wiitreethread to this main window so we can read the output
    connect( wiithread , SIGNAL( SendProgress( int ) ), ui->progressBar, SLOT( setValue( int ) ) );
    connect( wiithread , SIGNAL( SendDone( QTreeWidgetItem * ) ), this, SLOT( ThreadIsDoneRunning( QTreeWidgetItem * ) ) );

	connect( &wit, SIGNAL( SendStdOut( const QString &) ), ui->textEdit, SLOT( insertPlainText( const QString &) ) );
    //TODO: maybe make this a bright color so it is easy to find.
    //but fatal errors are shown in a dialog box, so it really isnt necessary
	connect( &wit, SIGNAL( SendStdErr( const QString &) ), ui->textEdit, SLOT( insertPlainText( const QString &) ) );


    connect( &wit, SIGNAL( RequestPassword() ), this, SLOT( NeedToAskForPassword() ) );
    connect( this, SIGNAL( UserEnteredPassword() ), &wit, SLOT( PasswordIsEntered() ) );
	connect( &wit, SIGNAL( SendFatalErr( const QString &, int ) ), this, SLOT( HandleThreadErrors( const QString &, int ) ) );
	connect( &wit, SIGNAL( SendGameInfo( const QString &, const QString &, const QString &, int, int, const QStringList &, const QStringList &, bool ) ), \
		 this, SLOT( ReceiveGameInfo( const QString &, const QString &, const QString &, int, int, const QStringList &, const QStringList &, bool ) ) );

    connect( &wwt, SIGNAL( RequestPassword() ), this, SLOT( NeedToAskForPassword() ) );
    connect( this, SIGNAL( UserEnteredPassword() ), &wwt, SLOT( PasswordIsEntered() ) );
	connect( &wwt, SIGNAL( SendFatalErr( const QString &, int ) ), this, SLOT( HandleThreadErrors( const QString &, int ) ) );

    connect( &wwt, SIGNAL( SendJobDone( int ) ), this, SLOT( HideProgressBar( int ) ) );
    connect( &wit, SIGNAL( SendJobDone( int ) ), this, SLOT( HideProgressBar( int ) ) );
	connect( &wit, SIGNAL( SendMessageForStatusBar( const QString & ) ), this, SLOT( GetStatusTextFromWiimms( const QString & ) ) );
	connect( &wwt, SIGNAL( SendMessageForStatusBar( const QString & ) ), this, SLOT( GetStatusTextFromWiimms( const QString & ) ) );
    connect( &wit, SIGNAL( SendProgress( int ) ), ui->progressBar, SLOT( setValue( int ) ) );
    connect( &wwt, SIGNAL( SendProgress( int ) ), ui->progressBar, SLOT( setValue( int ) ) );

    if( !game.isEmpty() )
	LoadGame( game );
}

GameWindow::~GameWindow()
{
    wiithread->ForceQuit();
    delete wiithread;
    delete ui;
}

void GameWindow::LoadGame( const QString &path )
{
    if( !wit.VersionIsOk() || path.isEmpty() )
	return;

    tmpPath = path;
    ui->textEdit->clear();

    //clear the old list of partition offsets
    partitionOffsets.clear();

    QStringList args;
    args << "DUMP";
    args << path;

    QString titlesTxtPath = wit.GetTitlesTxtPath();
    if( !titlesTxtPath.isEmpty() )
	args << "--titles=" + titlesTxtPath;

    QString showString = "--show=intro,tmd,P-Map,P-Info,D-Map,files,offset,hex,size";

    args << showString;

    ui->textEdit->clear();
    ui->statusbar->showMessage( tr( "Getting game contents from wit..." ) );

    wit.RunJob( args, witDump );
}

//triggered by veiw->refresh
void GameWindow::ReloadGame()
{
    if( lastPathLoadedCorrectly.isEmpty() )
	return;
    LoadGame( lastPathLoadedCorrectly );
}

void GameWindow::ReceiveGameInfo( const QString &type, const QString &id, const QString &name, int ios, int region, const QStringList &files,
								  const QStringList &partitionOffsets, bool fakesigned )
{
    //qDebug() << "GameWindow::ReceiveGameInfo:" << id << name << type << ( fakesigned ? "" : "Not" ) << "fakesigned";
    ui->statusbar->showMessage( tr( "Parsing game contents..." ) );
    bool wii = !type.contains( "GC" );
    gameTypeStr = type;
    ui->lineEdit_id->setText( id );
    ui->lineEdit_name->setText( name );
    gameType = wii;
    lastPathLoadedCorrectly = tmpPath;
    oldName = name;
    oldId = id;

    ui->progressBar->setVisible( true );
    QSettings s( settingsPath, QSettings::IniFormat );
    bool skipSvn = s.value( "wit_wwt/ignoreSvn", true).toBool();
    switch( region )
    {
    case 4:
	ui->comboBox_region->setCurrentIndex( 4 );
	break;
    default:
	ui->comboBox_region->setCurrentIndex( region + 1 );
	break;
    }

    switch( wii )
    {
    case 0://gamegube
	{
	    GC_Game gc( lastPathLoadedCorrectly );//this is blocking.  the program freezes while it tries to load the game
	    ui->label_sigtype->setText( "" );
	    ui->label_trucha_icon->setPixmap( gc.BannerImage( ui->lineEdit_id->height() + 10 ) );
	}
	break;

    default:
    case 1://wii
	gameIos = ios;
	gameRegion = region;
	ui->spinBox_ios->setValue( ios );
	QImage img = QImage( fakesigned ? ":images/trucha.png" : ":images/no_trucha.png" ).scaledToHeight( ui->lineEdit_id->height(), Qt::SmoothTransformation );
	QPixmap truchaIcon = QPixmap::fromImage( img );
	ui->label_sigtype->setText( ( fakesigned ? tr( "Fakesigned" ) : tr( "Officially signed" ) ) );
	ui->label_trucha_icon->setPixmap( truchaIcon );

	break;
    }

    ui->label_ios->setVisible( wii );
    ui->spinBox_ios->setVisible( wii );
    //ui->label_trucha_icon->setVisible( wii );
    ui->checkBox_tikTmd->setVisible( wii );
    ui->label_sigtype->setVisible( wii );

    wiithread->DoCommand( files, skipSvn, keyIcon, groupIcon, wii, partitionOffsets );
    EnableDisableStuff();
}

void GameWindow::HandleThreadErrors( const QString &err, int id )
{
    qDebug() << "GameWindow::HandleThreadErrors" << id << err;
    ui->progressBar->setVisible( false );
    unsetCursor();
    switch( id )
    {
    case witDump:
	{
	    QMessageBox::critical( this, tr( "Error Getting game Data" ), err );
	    ui->statusbar->showMessage( tr( "Error getting game information" ) );
	}
	break;
    case wwtAdd:
    case witCopy:
    case witEdit:
	QMessageBox::critical( this, tr( "Error writing games" ), err );
	ui->statusbar->showMessage( tr( "Error writing games" ) );
	break;
    }
    if( writingToWBFS )
    {
	qDebug() << "GameWindow::HandleThreadErrors -> about to enable the wbfs partition";
	SetPartitionEnabled( busyWBFSPath, true );
    }

}

void GameWindow::ThreadIsDoneRunning( QTreeWidgetItem *i )
{
    //qDebug() << "GameWindow::ThreadIsDoneRunning";
    //delete old entries
    ClearTreeView();

    //move the file tree
    ui->treeWidget->addTopLevelItems( i->takeChildren() );

    ui->progressBar->setVisible( false );

    //tell the user that we are ready to do something else
    ui->statusbar->showMessage( tr( "Ready" ) );
}

void GameWindow::EnableDisableStuff()
{
    ui->label_ios->setEnabled( gameType );
    ui->spinBox_ios->setEnabled( gameType );
    ui->comboBox_region->setEnabled( gameType );

    bool ch = ui->checkBox_id->isChecked() || ui->checkBox_title->isChecked();

    ui->checkBox_discHdr->setEnabled( ch );
    ui->checkBox_partHdr->setEnabled( ch );
    ui->checkBox_tikTmd->setEnabled( gameType && ui->checkBox_id->isChecked() );

    QString container = gameTypeStr;
    container.resize( container.indexOf( "/" ) );
    bool canEdit = wit.NameSupportsAttribute( container, "modify" );
    ui->actionSave->setEnabled( canEdit );
}

//forward password request from wit/wwt to main window to avoid multiple windows asking for it at the same time
void GameWindow::NeedToAskForPassword()
{
    emit AskMainWindowForPassword();
}

//tell wit/wwt that the user entered a password
void GameWindow::GetPasswordFromMainWindow()
{
    emit UserEnteredPassword();
}

void GameWindow::on_checkBox_title_clicked()
{
    EnableDisableStuff();
}

void GameWindow::on_checkBox_id_clicked()
{
    EnableDisableStuff();
}

void GameWindow::on_actionSave_As_triggered()
{
    QStringList patches = GetPatchArgs();
    QString currentPart;
    int size = partList.size();
    for( int i = 0; i < size; i++ )
    {
	if( lastPathLoadedCorrectly.startsWith( partList.at( i )->text( 0 ) ) )
	    currentPart = partList.at( i )->text( 0 );
    }
    QStringList args = GameCopyDialog::WitCopyCommand( this, currentPart, partList, QStringList() << lastPathLoadedCorrectly, patches );
    //qDebug() << args.size() << args;
    if( args.size() < 2 )
	return;
    //for testing...
    //return;

    QString prog = args.takeFirst();
    //qDebug() << args;
    ui->progressBar->setVisible( true );
    setCursor( Qt::BusyCursor );
    //not exactly true, but 99.999% of the time this will be displayed only while wit is trying to open up the file and read it,
    ui->statusbar->showMessage( tr( "Waking up your HDD..." ) );
    if( prog == "wwt" )
    {
	SetPartitionEnabled( args.at( 2 ), false );
	dirtyPartition = args.at( 2 );
	wwt.RunJob( args, wwtAdd );
    }
    else if( prog == "wit" )
    {
	writingToWBFS = false;
	dirtyPartition = args.takeFirst();
	wit.RunJob( args, witCopy );
    }
    else
	qDebug() << "WFT invalid job" << prog;
}

//this is connected to "wwt ADD", "wit COPY", and "wit EDIT" -> done
void GameWindow::HideProgressBar( int job )
{
    ui->progressBar->setVisible( false );
    unsetCursor();
    switch( job )
    {
    case wwtAdd:
	emit PartitionIsDirty( dirtyPartition );
	ui->statusbar->showMessage( tr( "Done adding game to WBFS partition" ) );
	break;
    case witCopy:
	emit PartitionIsDirty( dirtyPartition );
	ui->statusbar->showMessage( tr( "Done copying & converting game" ) );
	break;
    case witEdit:
	oldName = ui->lineEdit_name->text();
	oldId = ui->lineEdit_id->text();
	gameIos = ui->spinBox_ios->value();
	switch( ui->comboBox_region->currentIndex() )
	{
	case 4:
	    gameRegion = 4;
	    break;
	case 0:
	    break;
	default:
	    gameRegion = ui->comboBox_region->currentIndex() - 1;
	    break;
	}
	ui->statusbar->showMessage( tr( "Game patched OK" ) );
	break;
    default:
	ui->statusbar->showMessage( QString( "You shouldn\'t see this: error = GameWindow::HideProgressBar( %1 )" ).arg( job ) );
	break;
    }

    if( writingToWBFS )
    {
	qDebug() << "GameWindow::HideProgressBar -> about to enable the wbfs partition";
	SetPartitionEnabled( busyWBFSPath, true );
    }
}


//give the available actions to the mainwindow for the "file" menu
QStringList GameWindow::AvailableActions()
{
    QStringList ret = QStringList() << "save as";//this one is always available

    //assume this is a game on DVD-r.  these are read-only, so dont allow "save"
    if( lastPathLoadedCorrectly.startsWith( "/dev/sr" ) )
	return ret;

    QString current = gameTypeStr;
    current.resize( current.indexOf( "/" ) );
    if( wit.NameSupportsAttribute( current, "modify") )
	ret << "save";
    return ret;
}

void GameWindow::on_actionSave_triggered()
{
    //qDebug() << "GameWindow::on_actionSave_triggered()";
    QString details = GetChangeList();
    if( details.isEmpty() )//removed for testing
    {
	QMessageBox::information( this, tr( "Nothing to do" ), tr( "There aren\'t any changes to save to this game." ) );
	return;
    }
    SaveDialog msgBox( this, tr( "Save your changes and overwrite?" ), lastPathLoadedCorrectly, details );

    if( !msgBox.exec() )
	return;

    QStringList args = QStringList() << "EDIT" << lastPathLoadedCorrectly << GetPatchArgs();
    qDebug() << args;
    setCursor( Qt::BusyCursor );
    writingToWBFS = false;


    //not exactly true, but 99.999% of the time this will be displayed only while wit is trying to open up the file and read it,
    ui->statusbar->showMessage( tr( "Waking up your HDD..." ) );

    wit.RunJob( args, witEdit );


}

//respond to message that the user has edited the settings
void GameWindow::SettingsHaveChanged()
{
    //qDebug() << "GameWindow::SettingsHaveChanged()" << partition->text( 0 );
    QSettings s( settingsPath, QSettings::IniFormat );
    bool root = s.value( "root/enabled" ).toBool();
    wit.SetRunAsRoot( root );
    wit.SetTitlesTxtPath( s.value( "paths/titlesTxt", "" ).toString() );
}

//get a copy of the partition list from the main window
void GameWindow::SetPartitionList( QList<QTreeWidgetItem *> pList )
{
    //qDebug() << "GameWindow::SetPartitionList" << pList.size();
    while( !partList.isEmpty() )//delete all known partitions from the list
    {
	QTreeWidgetItem *item = partList.takeFirst();
	delete item;
    }

    int size = pList.size();
    for( int i = 0; i < size; i++ )
	partList << pList.at( i )->clone();
}

//look at the variables the game started out with and the current gui state and build a list of args to patch this game
QStringList GameWindow::GetPatchArgs()
{
    QStringList args;
    if( !gameType )//gamecube game
    {
    }
    else//wii game
    {
	if( ui->spinBox_ios->value() != gameIos )
	{
	    QString ios;
	    QTextStream( &ios ) << "--ios=" << ui->spinBox_ios->value();
	    args << ios;
	}
	int reg = ui->comboBox_region->currentIndex();
	switch( reg )
	{
	case 4:
	    break;
	default:
	    reg--;
	    break;
	}
	if( reg >= 0 && reg != gameRegion )
	{
	    QString regString;
	    QTextStream( &regString ) << "--region=" << reg;
	    args << regString;
	}
    }

    bool needToModify = false;
    QString mod1;
    QString mod2;
    if( ui->checkBox_id->isChecked() //dont patch teh ID if it already matches the old one and we are only modifying the disc header ( this is where the old one came from )
	&& ( ui->lineEdit_id->text() != oldId || ( ui->checkBox_tikTmd->isChecked() || ui->checkBox_partHdr->isChecked() ) ) )
    {
	needToModify = true;
	mod1 = "--id=" + ui->lineEdit_id->text();
    }

    if( ui->checkBox_title->isChecked() && //only patch the name if it is different from the original one, or if we are patching the partition header
	( ui->lineEdit_name->text() != oldName || ui->checkBox_partHdr->isChecked() ) )
    {
	needToModify = true;
	mod2 = "--name=" + ui->lineEdit_name->text();
    }

    QString titlesTxtPath = wit.GetTitlesTxtPath();
    if( !titlesTxtPath.isEmpty() )
	args << "--titles=" + titlesTxtPath;

    bool modify = ( ui->checkBox_id->isChecked() || ui->checkBox_title->isChecked() ) && needToModify;
    if( modify )
    {
	QString mod = "--modify=";
	quint8 checked = 0;

	if( ui->checkBox_discHdr->isChecked() )
	{
	    checked++;
	    mod += "DISC";
	}
	if( ui->checkBox_partHdr->isChecked() )
	{
	    if( checked )mod += ",";
	    mod += "BOOT";
	    checked++;
	}
	if( ui->checkBox_tikTmd->isChecked() && gameType && ui->checkBox_id->isChecked() )
	{
	    if( checked )mod += ",";
	    mod += "TMD,TICKET";
	    checked++;
	}
	if( checked )
	{
	    if( !mod1.isEmpty() )args << mod1;
	    if( !mod2.isEmpty() )args << mod2;
	    args << mod;
	}
    }
    return args;
}

//look at the variables the game started out with and the current gui state and build a string of text to show the changes
QString GameWindow::GetChangeList()
{
    QString ret;
    QTextStream ts( &ret );
    bool fakesigned = false;
    if( !gameType )//gamecube game
    {
    }
    else//wii game
    {
	if( ui->spinBox_ios->value() != gameIos )
	{
	    ts << "IOS:\t" << gameIos << " -> " << ui->spinBox_ios->value() << "\n";
	    fakesigned = true;
	}
	int reg = ui->comboBox_region->currentIndex();
	switch( reg )
	{
	case 4:
	    break;
	default:
	    reg--;
	    break;
	}
	if( reg >= 0 && reg != gameRegion )
	{
	    ts << tr( "Region" ) << ":\t" << gameRegion << " -> " << reg << "\n";
	}
    }

    bool needToModify = false;
    QString maybeModify;
    if( ui->checkBox_id->isChecked() //dont patch teh ID if it already matches the old one and we are only modifying the disc header ( this is where the old one came from )
	&& ( ui->lineEdit_id->text() != oldId || ( ui->checkBox_tikTmd->isChecked() || ui->checkBox_partHdr->isChecked() ) ) )
    {
	needToModify = true;
	QTextStream( &maybeModify ) << "ID:\t" << oldId << " -> " << ui->lineEdit_id->text() << "\n";
    }

    if( ui->checkBox_title->isChecked() && //only patch the name if it is different from the original one, or if we are patching the partition header
	( ui->lineEdit_name->text() != oldName || ui->checkBox_partHdr->isChecked() ) )
    {
	needToModify = true;
	QTextStream( &maybeModify ) << tr( "Name" ) << ":\t\"" << oldName << "\" -> \"" << ui->lineEdit_name->text() << "\"\n";
    }

    bool modify = ( ui->checkBox_id->isChecked() || ui->checkBox_title->isChecked() ) && needToModify;
    if( modify )
    {
	QString mod = tr( "Modified" ) + ":\t";
	quint8 checked = 0;

	if( ui->checkBox_discHdr->isChecked() )
	{
	    checked++;
	    mod += tr( "Disc Header" );
	}
	if( ui->checkBox_partHdr->isChecked() )
	{
	    if( checked )mod += ",";
	    mod += tr( "Partition Header" );
	    fakesigned = true;
	    checked++;
	}
	if( ui->checkBox_tikTmd->isChecked() && gameType && ui->checkBox_id->isChecked() )
	{
	    if( checked )mod += ",";
	    mod += tr( "TMD + Ticket" );
	    fakesigned = true;
	    checked++;
	}
	mod += "\n";
	if( checked )
	{
	    ts << maybeModify << mod;
	}
    }
    if( fakesigned )
	ts << "\n\n" << tr( "The partition will be fakesigned.  This game can only be read by IOS with the signature checking bug." ) + "\n";
    ts.flush();
    return ret;
}

//flag a partition as "busy" and send that to the main window so it can tell all other windows not to mess with that partition
//only really needed for WBFS partitions to prevent tring to write 2 games at the same time.
//reading & writing at the same time by 2 different processes should be ok, but im not sure
void GameWindow::SetPartitionEnabled( const QString &part, bool enabled )
{
    qDebug() << "GameWindow::SetPartitionEnabled :" << part << enabled;
    for( int i = 0; i < partList.size(); i++ )
    {
	if( partList.at( i )->text( 0 ) == part )
	{
	    partList.at( i )->setText( 6, enabled ? "" : "busy" );
	    emit SendUpdatedPartitionInfo( partList.at( i ) );

	    writingToWBFS = !enabled;
	    if( !enabled && !busyWBFSPath.isEmpty() )
		qDebug() << "GameWindow::SetPartitionEnabled :!enabled && !busyWBFSPath.isEmpty()";
	    busyWBFSPath = enabled ? "" : part;
	    return;
	}
    }
    qDebug() << "GameWindow::SetPartitionEnabled :no partition matched" << part;
}

//get status message and append it to the status bar
void GameWindow::GetStatusTextFromWiimms( const QString &text )
{
    ui->statusbar->showMessage( text );
}

void GameWindow::ClearTreeView()
{
    QList<QTreeWidgetItem* > oldItems = ui->treeWidget->invisibleRootItem()->takeChildren();
    while( !oldItems.isEmpty() )
    {
	QTreeWidgetItem* i = oldItems.takeFirst();
	delete i;
    }
}

//insert text int the ui->plaintext
//! s is the text
//! c is a color
void GameWindow::InsertText( const QString &s, const QString &c )
{
    //copy the string so we can alter it and leave the original alone
    QString textCopy = s;

    //replace all \r\n and \n with <br>
#ifdef Q_WS_WIN
    textCopy.replace( "\r\n", "<br>" );
#endif
    textCopy.replace( "\n", "<br>" );

    QString htmlString = "<b><text style=\"color:" + c + "\">" + textCopy + "</text></b>";

    ui->textEdit->insertPlainText( htmlString );

}
