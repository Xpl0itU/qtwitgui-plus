#include "includes.h"

#include "svnrev.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "hddselectdialog.h"
#include "settingsdialog.h"
#include "partitionwindow.h"
#include "passworddialog.h"
#include "filefolderdialog.h"
#include "gamewindow.h"
#include "aboutdialog.h"
#include "fsinfo.h"
#include "tools.h"
#include "formatwindow.h"
#include "dvdselectdialog.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    pFlowDirection = 0;
    wiiTDBisOpen = false;
    pFlowIsOpen = false;
    alreadyAskingForPassword = false;
    ui->setupUi( this );
#ifdef Q_OS_MAC
    ui->actionOpen_DVD_r->setEnabled( false );//needs the listing of DVD drives fixed in fsinfo.cpp
#endif

    setWindowTitle( PROGRAM_NAME );

    LoadSettings();
    CheckWit();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent( QCloseEvent * closeEvent )
{
    //tell all the subwindows to close to trigger the stuff to save their position
    QList<QMdiSubWindow *>list = ui->mdiArea->subWindowList();

    //store these variables, as they are set to false when we call "close"
    bool wiitdb = wiiTDBisOpen;
    bool covers = pFlowIsOpen;
    bool logOpen = logIsOpen;
    foreach( QMdiSubWindow *window, list )
		window->close();

    wiiTDBisOpen = wiitdb;
    pFlowIsOpen = covers;
    logIsOpen = logOpen;

    SaveSettings();
    QWidget::closeEvent( closeEvent );
}

//create the WiiTDB window and display it in the previous location
void MainWindow::CreateWiiTDBSubWindow()
{
    wiiTDBwindow = new WiiTDBWindow();

    subWiiTDB = new CustomMdiItem( ui->mdiArea );
    subWiiTDB->type = mdiWiiTDB;
    subWiiTDB->setWidget( wiiTDBwindow );
    subWiiTDB->setWindowTitle( "WiiTDB" );
    subWiiTDB->setWindowFlags( Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint );
    //subWiiTDB->setWindowIcon( QIcon( ":/images/wiitdbIcon.png" ) );

    subWiiTDB->show();

    QSettings settings( settingsPath, QSettings::IniFormat );
    subWiiTDB->move( settings.value( "wiitdb/pos", QPoint( 0, 0 ) ).toPoint() );
    subWiiTDB->resize( settings.value( "wiitdb/size", QSize( 610, 375 ) ).toSize() );


	connect( subWiiTDB, SIGNAL( AboutToClose( const QString &, const QPoint &, const QSize &, int ) ),
			 this, SLOT( MdiItemDestroyed( const QString &, const QPoint &, const QSize &, int ) ) );

    if( pFlowIsOpen )
		connect( pFlow, SIGNAL( NewIDInFocus( const QString & ) ),
				 wiiTDBwindow, SLOT( LoadGameFromID( const QString & ) ) );

    //connect the window to any open partitions
    foreach( QMdiSubWindow *window, ui->mdiArea->subWindowList() )
    {
		CustomMdiItem *mdiChild = qobject_cast<CustomMdiItem *>( window );
		if( mdiChild->type != mdiPartition )
			continue;
		foreach( QTreeWidgetItem* item, partList )
		{
			if( mdiChild->GetTitle() == item->text( 0 ) )
			{
				PartitionWindow *w = qobject_cast<PartitionWindow *>( mdiChild->widget() );
				connect( w, SIGNAL( GameClicked( const QString & ) ),
						 wiiTDBwindow, SLOT( LoadGameFromID( const QString & ) ) );
			}
		}
    }
}

//create the cover window and display it in the previous location
void MainWindow::CreatePFlowSubWindow()
{
    pFlow = new CoverManagerWindow;
    pFlow->SetGameLists( gameMap );
    PictureFlow *pf = pFlow->PFlowObject();
    pf->setSlideSize( QSize( 3 * 40, 5 * 40 ) );
    pf->setFixedHeight( 270 );
    pf->setMinimumWidth( 200 );
    pf->setFocusPolicy( Qt::ClickFocus );

    subPFlow = new CustomMdiItem( ui->mdiArea );
    subPFlow->type = mdiCovers;
    subPFlow->setWidget( pFlow );
    subPFlow->setWindowTitle( tr( "CoverFlow" ) );
    //subPFlow->setWindowIcon( QIcon( ":/images/coverFlowIcon.png" ) );
    subPFlow->show();

    QSettings settings( settingsPath, QSettings::IniFormat );
    subPFlow->move( settings.value( "pflow/pos", QPoint( 0, 0 ) ).toPoint() );
    subPFlow->resize( settings.value( "pflow/size", QSize( 500, 295 ) ).toSize() );

    //pf->setCenterIndex( pf->slideCount() / 2 );

	connect( this, SIGNAL( SendListsToCoverManager( const QMap<QString, QList<QTreeWidgetItem *> > & ) ), \
			 pFlow, SLOT( SetGameLists( const QMap<QString, QList<QTreeWidgetItem *> > & ) ) );

    //connect( pf, SIGNAL( centerIndexChanged( int ) ), this, SLOT( UpdateInfoFromPFlow( int ) ) );
    if( wiiTDBisOpen )
		connect( pFlow, SIGNAL( NewIDInFocus( const QString & ) ),
				 wiiTDBwindow, SLOT( LoadGameFromID( const QString & ) ) );

	connect( subPFlow, SIGNAL( AboutToClose( const QString &, const QPoint &, const QSize &, int ) ),
			 this, SLOT( MdiItemDestroyed( const QString &, const QPoint &, const QSize &, int ) ) );
	connect( this, SIGNAL( TellOpenWindowsThatTheSettingsAreChanged() ),
			 pFlow, SLOT( ReloadSettings() ) );
}

//help->about
void MainWindow::on_actionAbout_triggered()
{
    QString thisProgVersion = "r" + QString( SVN_REV_STR ) + "  :  " + tr( "Built %1").arg( __DATE__ );
    QString witVers = WitHandler::GetVersionString();
    if( witVers.isEmpty() )
		witVers = tr( "Missing" );
    else if( !WitHandler::VersionIsOk() )
		witVers += "   " + tr( "Unsupported" );

    QString wwtVers = WwtHandler::GetVersionString();
    if( wwtVers.isEmpty() )
		wwtVers = tr( "Missing" );
    else if( !WwtHandler::VersionIsOk() )
		wwtVers += "   " + tr( "Unsupported" );

    QString wiitdbVers = wiiTDBwindow->GetVersion();
    if( wiitdbVers.length() < 10 )
		wiitdbVers = tr( "Not Loaded" );

    QString wiitdbGames = QString( "%1" ).arg( wiiTDBwindow->GameCount() );
    AboutDialog dialog( this, thisProgVersion, witVers, wwtVers,wiitdbVers, wiitdbGames );
    dialog.exec();
}

//check that the wit path in the settings leads to a supported version of the program
void MainWindow::CheckWit()
{
    setAcceptDrops( false );
    setWindowTitle( PROGRAM_NAME );
    bool witOk = false;
    if( WitHandler::ReadVersion() )
    {
		if( WitHandler::VersionIsOk() )
		{
			if( WitHandler::ReadAttributes() )
			{
#ifdef Q_OS_WIN
                if( !FsInfo::Check() )
                {
                    setAcceptDrops( false );
                    ui->statusBar->showMessage( tr( "Error while checking wmic & cygpath" ) );
                }
                else
                {
#endif
                    witOk = true;
                    setWindowTitle( QString( PROGRAM_NAME ) + "  :  " + WitHandler::GetVersionString() );
                    ui->statusBar->showMessage( tr( "Ready" ) );
                    setAcceptDrops( true );
#ifdef Q_OS_WIN
                }
#endif
			}
			else
			{
				ui->statusBar->showMessage( tr( "Error reading \"wit info\"" ) );
			}
		}
		else
		{
			ui->statusBar->showMessage( tr( "Wit version is too low" ) );
		}
    }
    else
    {
		ui->statusBar->showMessage( tr( "Error getting wit version" ) );
    }

    WwtHandler::ReadVersion();//dont bother checking return value here, as wwt is not necessary for most of what this program does.

    ui->actionOpenGame->setEnabled( witOk );
    ui->actionOpen_Partition->setEnabled( witOk );

}

//settings save and load
void MainWindow::LoadSettings()
{
    QSettings settings( settingsPath, QSettings::IniFormat );
    int version = settings.value( "settingsversion", 0 ).toInt();
    if( version != SETTINGS_VERSION )
    {
		QMessageBox::critical( this, tr( "Settings Version" ),\
							   tr( "The format of the settings file does not match what is expected by this version of %1.").arg( PROGRAM_NAME )\
							   + "  " + tr( "Default settings will be used.") );
		settings.remove( "" );

		settings.setValue( "settingsversion", SETTINGS_VERSION );

    }

    settings.beginGroup("MainWindow");
    resize( settings.value("size", QSize( 800, 700 ) ).toSize() );
    move( settings.value("pos", QPoint( 200, 200 ) ).toPoint() );
    settings.endGroup();

    wiiTDBisOpen = settings.value( "wiitdb/visible", false ).toBool();
    ui->actionWiiTDB->setChecked( wiiTDBisOpen );
    if( wiiTDBisOpen )
		CreateWiiTDBSubWindow();

    pFlowIsOpen = settings.value( "pflow/visible", false ).toBool();
    ui->actionCovers->setChecked( pFlowIsOpen );
    if( pFlowIsOpen )
		CreatePFlowSubWindow();

    logIsOpen = settings.value( "logwindow/visible", true ).toBool();
    ui->actionLog->setChecked( logIsOpen );
    if( logIsOpen )
		ShowLogWindow();


    //read partition list and settings

    while( !partList.isEmpty() )//delete all known partitions from the list
    {
		QTreeWidgetItem *item = partList.takeFirst();
		delete item;
    }

    partList = ReadPartitionSettings();

    settings.beginGroup( "root" );
    rootAskStr = settings.value( "requestString", "[sudo] password for" ).toString();
    rootWrongStr = settings.value( "wrongPwString", "Sorry, try again." ).toString();
    rootFailStr = settings.value( "failString", "sudo: 3 incorrect password attempts" ).toString();
    settings.endGroup();

    //try to load their wiitdb.zip.  this is happening before and signals/slots are connected, so any error messages wont be shown
    //just fall back to the built-in one if not successful
    QString path = settings.value( "paths/wiitdb", "" ).toString();
    if( path.isEmpty() || !wiiTDB->LoadFile( path ) )
        wiiTDB->LoadFile( ":/wiitdb.xml" );
}

void MainWindow::SaveSettings()
{
    QSettings settings( settingsPath, QSettings::IniFormat );
    settings.setValue( "settingsversion", SETTINGS_VERSION );

    settings.beginGroup("MainWindow");
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    settings.endGroup();

    //special windows
    settings.setValue( "wiitdb/visible", wiiTDBisOpen );
    settings.setValue( "pflow/visible", pFlowIsOpen );
    settings.setValue( "logwindow/visible", logIsOpen );

    //partition options
    settings.beginGroup( "partitionOptions" );
    settings.remove( "" );			//remove all existing settings
    settings.endGroup();

    int size = partList.size();

    settings.beginWriteArray("partitionOptions");
    for( int i = 0; i < size; i++ )
    {
		QTreeWidgetItem* item = partList.at( i );
		settings.setArrayIndex( i );
		settings.setValue("path", item->text( 0 ) );
		settings.setValue("split", item->text( 3 ) );
		settings.setValue("source", item->text( 4 ) );
		settings.setValue("filesystem", item->text( 5 ) );
    }
    settings.endArray();
}

//respond to items in the mdi being closed
void MainWindow::MdiItemDestroyed( const QString &title, const QPoint &pos, const QSize &size, int type )
{
    //qDebug() << "MdiItemDestroyed()" << title << pos << size;
    bool max = size == QSize( 999, 999 );
    QSettings settings( settingsPath, QSettings::IniFormat );
    switch( type )
    {
    case mdiWiiTDB:
		{
			if( !max )
			{
				settings.setValue( "wiitdb/size", size );
				settings.setValue( "wiitdb/pos", pos );
			}
			ui->actionWiiTDB->setChecked( false );
			wiiTDBisOpen = false;
			return;
		}
		break;
    case mdiCovers:
		{
			if( !max )
			{
				settings.setValue( "pflow/size", size );
				settings.setValue( "pflow/pos", pos );
			}
			ui->actionCovers->setChecked( false );
			pFlowIsOpen = false;
			return;
		}
		break;
    case mdiPartition:
        {
            QString key = title;
			key.replace( "/", "__SLSH__" );
			settings.beginGroup( key );
			if( !max )
			{
				settings.setValue( "size", size );
				settings.setValue( "pos", pos );
			}
			settings.setValue( "maximized", max );
			settings.endGroup();
			settings.sync();
		}
		break;
    case mdiGame:
		if( !max )
		{
			settings.setValue( "gamewindow/size", size );
			settings.setValue( "gamewindow/pos", pos );
		}
		settings.setValue( "gamewindow/maximized", max );
		break;
    case mdiLog:
		if( !max )
		{
			settings.setValue( "logwindow/size", size );
			settings.setValue( "logwindow/pos", pos );
		}
		settings.setValue( "logwindow/maximized", max );
		logIsOpen = false;
		ui->actionLog->setChecked( false );
		break;
    }

}

//file->open partition
void MainWindow::on_actionOpen_Partition_triggered()
{
    //QList<QTreeWidgetItem *> pList;

    HDDSelectDialog dialog( this );
    dialog.AddPartitionsToList( partList );

	connect( &dialog, SIGNAL( SendGamelistFor_1_Partition( const QString &, const QList<QTreeWidgetItem *> & ) ),
			 this, SLOT( ReceiveListFor_1_Partition( const QString &, const QList<QTreeWidgetItem *> & ) ) );

	connect( &dialog, SIGNAL( SendSelectedPartition( const QList<QTreeWidgetItem *> & ) ),
			 this, SLOT( OpenSelectedPartitions( const QList<QTreeWidgetItem *> & ) ) );

	connect( &dialog, SIGNAL( SendHDDList( const QList<QTreeWidgetItem *> & ) ),
			 this, SLOT( ReceiveAllPartitionInfo( const QList<QTreeWidgetItem *> & ) ) );

    dialog.exec();
}

//view -> settings
void MainWindow::on_actionSettings_triggered()
{
    SettingsDialog dialog( this );
    if( dialog.exec() )
    {
		//qDebug() << "mainwindow: settings changed";

		//set the log colors
		QPalette p = logWindow->palette();
		QSettings s( settingsPath, QSettings::IniFormat );
		s.beginGroup( "log" );
		p.setColor( QPalette::Base, QColor( s.value( "bgColor", "#ffffff" ).toString() ) );
		p.setColor( QPalette::Text, QColor( s.value( "txtColor", "#000000" ).toString() ) );
		warningColor = s.value( "wrnColor", "#0000ff" ).toString();
		criticalColor = s.value( "crtColor", "#ff0000" ).toString();
		s.endGroup();
		logWindow->setPalette( p );

		//check that the external processes are working
		CheckWit();

		emit TellOpenWindowsThatTheSettingsAreChanged();
    }
}

//view->wiitdb toggled
void MainWindow::on_actionWiiTDB_triggered( bool checked )
{
    wiiTDBisOpen = checked;
    if( !checked )
    {
		subWiiTDB->close();
    }
    else CreateWiiTDBSubWindow();
}

//view->covers toggled
void MainWindow::on_actionCovers_triggered( bool checked )
{
    pFlowIsOpen = checked;
    if( !checked )
    {
		subPFlow->close();
    }
    else CreatePFlowSubWindow();
}

//view -> log triggered
void MainWindow::on_actionLog_triggered( bool checked )
{
    logIsOpen = checked;
    if( !checked )
    {
		CustomMdiItem *found = findMdiChild( tr( "Log" ), mdiLog );
		if( found )
		{
			found->close();
		}
    }
    else ShowLogWindow();
}

void MainWindow::ShowLogWindow()
{
    CustomMdiItem *found = findMdiChild( tr( "Log" ), mdiLog );
    if( found )
    {
		ui->mdiArea->setActiveSubWindow( found );
		found->show();
		return;
    }

    CustomMdiItem *mdiItem = new CustomMdiItem( ui->mdiArea, Qt::Widget, tr( "Log" ), false );
    mdiItem->type = mdiLog;
    mdiItem->setWidget( logWindow );
    mdiItem->show();

    QSettings settings( settingsPath, QSettings::IniFormat );
    mdiItem->move( settings.value( "logwindow/pos", QPoint( 0, 0 ) ).toPoint() );
    mdiItem->resize( settings.value( "logwindow/size", QSize( 610, 375 ) ).toSize() );

	connect( mdiItem, SIGNAL( AboutToClose( const QString &, const QPoint &, const QSize &, int ) ),
			 this, SLOT( MdiItemDestroyed( const QString &, const QPoint &, const QSize &, int ) ) );
}

//respond to the signal containing a partition path and its gameList
void MainWindow::ReceiveListFor_1_Partition( const QString &path, const QList<QTreeWidgetItem *> &list )
{
    qDebug() << "ReceiveListFor_1_Partition:" << path << list.size();
    //check if partition is already known
    QMap<QString, QList<QTreeWidgetItem *> >::iterator i = gameMap.find( path );
    if( i == gameMap.end() )
    {
		gameMap.insert( path, list );//not known, just add this one
    }
    else//partition is known
    {
		while( !i.value().isEmpty() )//delete old gamelist
		{
			QTreeWidgetItem *item = i.value().takeFirst();
			delete item;
		}
		i.value() = list;//set pointer to the new list
    }
    emit SendListsToCoverManager( gameMap );
}

//get info about all partitions ( sent when partition selector window is closed )
void MainWindow::ReceiveAllPartitionInfo( const QList<QTreeWidgetItem *> &list )
{
    //qDebug() << "got New List Of partitions:" << list.size();
    while( !partList.isEmpty() )//delete all known partitions from the list
    {
		QTreeWidgetItem *item = partList.takeFirst();
		//qDebug() << "deleting:" << item->text( 0 );
		delete item;
    }
    if( partList.size() )
    {
		foreach( QTreeWidgetItem *item, partList )
		{
			qDebug() << "MEM LEAK - not deleted:" << item->text( 0 );
		}
    }
#ifdef Q_OS_WIN
    int size = list.size();
    for( int i = 0; i < size; i++ )
    {
        list[ i ]->setText( 0, RemoveDriveLetter( list[ i ]->text( 0 ) ) );
    }
#endif
    partList = list;
    emit SendNewPartitionListToSubWindows( partList );
    //DisableEnablePartitionWindows();
}

//sent whenever one of the mdisubwindows receives information about a partition ( mib used, game count... )
void MainWindow::RecieveUpdatedPartitionInfo( QTreeWidgetItem *partInfo )
{
    //qDebug() << "MainWindow::RecieveUpdatedPartitionInfo" << partInfo->text( 0 ) << partInfo->text( 1 ) << partInfo->text( 2 ) ;
    int size = partList.size();
    if( !size )
		partList << partInfo;
    else
    {
		QList<QTreeWidgetItem *> newList;
		newList << partInfo->clone();
		if( partList.size() )
		{
			foreach( QTreeWidgetItem *item, partList )
			{
				if( item->text( 0 ) == partInfo->text( 0 ) )
					continue;

				newList << item->clone();
			}
		}
		ReceiveAllPartitionInfo( newList );
    }
    //DisableEnablePartitionWindows();
}

//disable/enable flagged WBFS partitions.  only allow access to them by 1 process at a time.
//currently not used.  instead just disable adding these partitions to the save dialog dropdown
void MainWindow::DisableEnablePartitionWindows()
{
    if( partList.size() )
    {
		foreach( QTreeWidgetItem *item, partList )
		{
			if( item->text( 5 ) == "WBFS" )
			{
				bool busy = item->text( 6 ) == "busy";
				CustomMdiItem *w = findMdiChild( item->text( 0 ), mdiPartition );
				if( !w )
					continue;
				w->widget()->setDisabled( busy );
			}
		}
    }
}

//if a partition window tries list-lll and gets an error, it will trigger this
void MainWindow::ReactToInvalidPartionReport( const QString &part )
{
    qDebug() << "MainWindow::ReactToInvalidPartionReport" << part;
    //remove it first from the list of partitions
    //this is kinda ghetto, but QList::takeAt() didnt seem to work right
    QList<QTreeWidgetItem *> newList;
    while( !partList.isEmpty() )
    {
		QTreeWidgetItem *i = partList.takeFirst();
		if( i->text( 0 ) == part )
			delete i;
		else newList << i;
    }
    partList = newList;
    emit SendNewPartitionListToSubWindows( partList );//send the current list to the subwindows

    //and now remove the list of games associated with this partition if there is one
    QMap<QString, QList<QTreeWidgetItem *> >::iterator i = gameMap.find( part );
    if( i == gameMap.end() )
    {
		return;//not found, nothing to do
    }
    else//partition has a gamelist associated with it
    {
		while( !i.value().isEmpty() )//delete old gamelist
		{
			QTreeWidgetItem *item = i.value().takeFirst();
			delete item;
		}
		gameMap.remove( part );//remove key of this partition from the map
    }
}

//a subwindow has asked for the root password
void MainWindow::NeedToAskForPassword()
{
    if( alreadyAskingForPassword )
		return;

    alreadyAskingForPassword = true;
    PasswordDialog dialog( this );
    dialog.exec();
    alreadyAskingForPassword = false;
    emit UserEnteredPassword();
}

//respond to signal to open some partitions
void MainWindow::OpenSelectedPartitions( const QList<QTreeWidgetItem *> &list )
{
    //qDebug() << "OpenSelectedPartitions" << list.size();
    if( !list.size() )
		return;

    foreach( QTreeWidgetItem *item, list )
    {
#ifdef Q_OS_WIN
        item->setText( 0, RemoveDriveLetter( item->text( 0 ) ) );
#endif
        CustomMdiItem *found = findMdiChild( item->text( 0 ), mdiPartition );

		if( found )		    //partition is already open in a window.  just make this window active
		{
			//qDebug() << item->text( 0 ) << "is already open";
			ui->mdiArea->setActiveSubWindow( found );
			continue;
		}

		QMap<QString, QList<QTreeWidgetItem *> >::iterator i = gameMap.find( item->text( 0 ) );
		if( i == gameMap.end() )
		{
			//qDebug() << " partition doesnt match:" << item->text( 0 );
			gameMap.insert( item->text( 0 ), QList<QTreeWidgetItem *>() );//not known, just add this one
			i = gameMap.find( item->text( 0 ) );
		}

		CustomMdiItem *mdiItem = new CustomMdiItem( ui->mdiArea );
		mdiItem->type = mdiPartition;
		mdiItem->setWindowTitle( item->text( 0 ) );
		PartitionWindow * w = new PartitionWindow();
		mdiItem->setWidget( w );
		w->SetPartitionAndGameList( item, i.value() );

		connect( this, SIGNAL( SendNewPartitionListToSubWindows( const QList<QTreeWidgetItem *> & ) ),
				 w, SLOT( SetPartitionList( const QList<QTreeWidgetItem *> & ) ) );

		connect( this, SIGNAL( TellOpenWindowsThatTheSettingsAreChanged() ), w, SLOT( SettingsHaveChanged() ) );

		connect( w, SIGNAL( SendUpdatedPartitionInfo( QTreeWidgetItem * ) ),
				 this, SLOT( RecieveUpdatedPartitionInfo( QTreeWidgetItem * ) ) );

		connect( w, SIGNAL( PartitionIsDirty( const QString &) ),
				 this, SLOT( ReceiveDirtyPartition( const QString &) ) );

		connect( w, SIGNAL( ReportInvalidPartition( const QString & ) ),
				 this, SLOT( ReactToInvalidPartionReport( const QString & ) ) );

		connect( this, SIGNAL( UserEnteredPassword() ), w, SLOT( GetPasswordFromMainWindow() ) );
		connect( w, SIGNAL( AskMainWindowForPassword() ), this, SLOT( NeedToAskForPassword() ) );

		connect( w, SIGNAL( BrowseGames( const QStringList & ) ), this, SLOT( OpenGames( const QStringList & ) ) );

		connect( w, SIGNAL( SendGamelistFor_1_Partition( const QString &, const QList<QTreeWidgetItem *> & ) ),
				 this, SLOT( ReceiveListFor_1_Partition( const QString &, const QList<QTreeWidgetItem *> & ) ) );

		connect( mdiItem, SIGNAL( AboutToClose( const QString &, const QPoint &, const QSize &, int ) ),
				 this, SLOT( MdiItemDestroyed( const QString &, const QPoint &, const QSize &, int ) ) );

		if( findMdiChild( "WiiTDB", mdiWiiTDB ) )
			connect( w, SIGNAL( GameClicked( const QString & ) ), wiiTDBwindow, SLOT( LoadGameFromID( const QString & ) ) );

		//mdiItem->show();
#ifdef Q_OS_WIN
        QString key = RemoveDriveLetter( item->text( 0 ) );
#else
        QString key = item->text( 0 );
#endif
		key.replace( "/", "__SLSH__" );

		QSettings settings( settingsPath, QSettings::IniFormat );
		settings.beginGroup( key );
		bool max = settings.value( "maximized", false ).toBool();
		if( !max )
			mdiItem->show();
		mdiItem->resize( settings.value( "size", QSize( 745, 425 ) ).toSize() );
		mdiItem->move( settings.value( "pos", QPoint( 0, 0 ) ).toPoint() );
		if( max )
			mdiItem->showMaximized();

		settings.endGroup();
    }
}

//respond to request to open games in a browser
void MainWindow::OpenGames( const QStringList &games )
{
	//    qDebug() << "MainWindow::OpenGames" << games;
    for( int i = 0; i < games.size(); i++ )
    {
		CustomMdiItem *mdiItem = new CustomMdiItem( ui->mdiArea );
		mdiItem->type = mdiGame;
		mdiItem->setWindowTitle( games.at( i ) );
		GameWindow *w = new GameWindow( this, games.at( i ), partList );
		mdiItem->setWidget( w );



		connect( this, SIGNAL( SendNewPartitionListToSubWindows( const QList<QTreeWidgetItem *> & ) ),
				 w, SLOT( SetPartitionList( const QList<QTreeWidgetItem *> & ) ) );

		connect( mdiItem, SIGNAL( AboutToClose( const QString &, const QPoint &, const QSize &, int ) ),
				 this, SLOT( MdiItemDestroyed( const QString &, const QPoint &, const QSize &, int ) ) );

		connect( this, SIGNAL( UserEnteredPassword() ), w, SLOT( GetPasswordFromMainWindow() ) );
		connect( w, SIGNAL( AskMainWindowForPassword() ), this, SLOT( NeedToAskForPassword() ) );

		connect( w, SIGNAL( SendUpdatedPartitionInfo( QTreeWidgetItem * ) ),
				 this, SLOT( RecieveUpdatedPartitionInfo( QTreeWidgetItem * ) ) );

		connect( w, SIGNAL( PartitionIsDirty( const QString & ) ), this, SLOT( ReceiveDirtyPartition( const QString & ) ) );
		//mdiItem->show();

		QSettings settings( settingsPath, QSettings::IniFormat );
		settings.beginGroup( "gamewindow" );
		bool max = settings.value( "maximized", false ).toBool();
		if( !max )
			mdiItem->show();
		mdiItem->resize( settings.value( "size", QSize( 745, 425 ) ).toSize() );
		mdiItem->move( settings.value( "pos", QPoint( 0, 0 ) ).toPoint() );
		if( max )
			mdiItem->showMaximized();


		settings.endGroup();
    }
}

//find an open window in the mdi area
CustomMdiItem *MainWindow::findMdiChild( const QString &name, int type )
{
    foreach( QMdiSubWindow *window, ui->mdiArea->subWindowList() )
    {
		CustomMdiItem *mdiChild = qobject_cast<CustomMdiItem *>( window );
		if( mdiChild->type != type && type != mdiAny )
			continue;
		if( mdiChild->GetTitle() == name )
			return mdiChild;
    }
    return 0;
}

//get a message that a parttion needs to be reloaded ( from a different window ) and request for that partition to refresh its gamelist
void MainWindow::ReceiveDirtyPartition( const QString &part )
{
    qDebug() << "MainWindow::ReceiveDirtyPartition(" << part << ")";
    CustomMdiItem *subW = findMdiChild( part, mdiPartition );
    if( !subW )
		return;

    PartitionWindow *w = qobject_cast<PartitionWindow *>( subW->widget() );
    QTimer::singleShot( 0, w, SLOT( on_actionRefresh_List_triggered() ) );
}

//read settings for each partition and get them in the form of a treewidgetitemlist
QList<QTreeWidgetItem *> MainWindow::ReadPartitionSettings()
{
    QList<QTreeWidgetItem *> ret;
    QSettings settings( settingsPath, QSettings::IniFormat );
    int size = settings.beginReadArray( "partitionOptions" );
    for( int i = 0; i < size; i++ )
    {
		settings.setArrayIndex( i );
		QString path = settings.value( "path" ).toString();
		if( !QFile::exists( path ) )//dont add partitions that dont exist any more
			continue;
		QTreeWidgetItem *item = new QTreeWidgetItem;
		item->setText( 0, path );
		item->setText( 3, settings.value( "split" ).toString() );
		item->setText( 4, settings.value( "source" ).toString() );
		item->setText( 5, settings.value( "filesystem" ).toString() );

		ret << item;
    }
    settings.endArray();

    return ret;
}

//file->open game
void MainWindow::on_actionOpenGame_triggered()
{
    FileFolderDialog dialog( this );
    dialog.setNameFilter( "*.iso *.wbfs *.ciso *.wdf *.wia" );

    if ( !dialog.exec() )
		return;

    QStringList games = dialog.selectedFiles();

    OpenGames( games );
}

//drag & drop
//triggered on dropping a file in the main window
void MainWindow::dropEvent( QDropEvent *event )
{
    int size = event->mimeData()->urls().size();
    if( !size )
    {
		event->ignore();
		return;
    }

    QStringList urls;
    foreach( QUrl url, event->mimeData()->urls() )
		urls << url.toLocalFile();

    QStringList types = WitHandler::FileType( urls );
    if( types.size() != urls.size() )
    {
        qDebug() << "MainWindow::dropEvent: wrong size" << types.size() << urls.size();
		event->ignore();
		return;
    }
    int cnt = types.size();
    for( int i = 0; i < cnt; i++ )
    {
		if( types.at( i ) == "OTHER" || types.at( i ) == "NO-FILE" )//do nothing
			continue;

		if( types.at( i ) == "DIR" )//folder.  expect that it is a partition.  an extracted GC game will throw a false positive here as composing them is not supported
		{
			int pcnt = partList.size();
			bool found = false;
			for( int j = 0; j < pcnt  && !found; j++ )
			{
				if( partList.at( j )->text( 0 ) == urls.at( i ) )//do we already know this partition
				{
					OpenSelectedPartitions( QList<QTreeWidgetItem *>() << partList.at( j ) );
					found = true;
				}
			}
			//partition not known, add it to the list and open it
			QTreeWidgetItem *newPart = new QTreeWidgetItem;
			newPart->setText( 0, urls.at( i ) );
			partList << newPart;
			OpenSelectedPartitions( QList<QTreeWidgetItem *>() << newPart );

			continue;
		}
		if( types.at( i ).contains( "/WII" ) || types.at( i ).contains( "/GC" ) )
		{
			//must be a game
			CustomMdiItem *w = findMdiChild( urls.at( i ), mdiGame );
			if( w )		    //game is already open in a window.  just make this window active
			{
				ui->mdiArea->setActiveSubWindow( w );
				continue;
			}
			OpenGames( QStringList() << urls.at( i ) );
			continue;
		}
		else
			qDebug() << "unhandled wit filetype:" << types.at( i );

    }
}

//triggered on dragging a file around the mainwindow
void MainWindow::dragEnterEvent( QDragEnterEvent *event )
{
    if( event->mimeData()->hasUrls() )
    {
		event->acceptProposedAction();
    }
    else
    {
		event->ignore();
    }
}

//right before the "file" menu will be displayed
void MainWindow::on_menuFile_aboutToShow()
{
    //qDebug() << "MainWindow::UserClickedFileMenu()";

    int type = CurrentMidSubwindowType();
    switch( type )
    {
    case mdiGame:
		{
			CustomMdiItem *subW = CurrentMdiSubWindow();
			GameWindow *gw = qobject_cast<GameWindow *>( subW->widget() );
			QStringList actions = gw->AvailableActions();

			ui->actionSave->setEnabled( actions.contains( "save" ) );
			ui->actionSave_As->setEnabled( actions.contains( "save as" ) );
		}

		break;
    default:
		ui->actionSave->setEnabled( false );
		ui->actionSave_As->setEnabled( false );
		//qDebug() << "mdi:" << type;
		break;
    }

}

//right before the "view" menu will be displayed
void MainWindow::on_menuView_aboutToShow()
{
    bool canRefresh = false;
    int type = CurrentMidSubwindowType();
    switch( type )
    {
    case mdiGame:
    case mdiPartition:
    case mdiCovers:
		{
			canRefresh = true;
		}
		break;
    default:
		break;
    }
    ui->actionRefresh_Current_Window->setEnabled( canRefresh );
}

//see what type of window is active
int MainWindow::CurrentMidSubwindowType()
{
    QMdiSubWindow *subW = ui->mdiArea->activeSubWindow();
    if( !subW )
		return mdiNone;

    CustomMdiItem *custom = qobject_cast<CustomMdiItem *>( subW );
    return custom->type;
}

CustomMdiItem *MainWindow::CurrentMdiSubWindow()
{
    QMdiSubWindow *subW = ui->mdiArea->activeSubWindow();
    if( !subW )
		return 0;

    CustomMdiItem *custom = qobject_cast<CustomMdiItem *>( subW );
    return custom;
}

//file->save
void MainWindow::on_actionSave_triggered()
{
    CustomMdiItem *subW = CurrentMdiSubWindow();
    if( !subW || subW->type != mdiGame )
		return;

    GameWindow *gw = qobject_cast<GameWindow *>( subW->widget() );
    QTimer::singleShot( 0, gw, SLOT( on_actionSave_triggered() ) );
}

//file ->save as
void MainWindow::on_actionSave_As_triggered()
{
    CustomMdiItem *subW = CurrentMdiSubWindow();
    if( !subW || subW->type != mdiGame )
		return;

    GameWindow *gw = qobject_cast<GameWindow *>( subW->widget() );
    QTimer::singleShot( 0, gw, SLOT( on_actionSave_As_triggered() ) );
}

//view ->refresh
void MainWindow::on_actionRefresh_Current_Window_triggered()
{
    CustomMdiItem *subW = CurrentMdiSubWindow();
    if( !subW )
		return;

    switch( subW->type )
    {
    case mdiPartition:
		{
			PartitionWindow *w = qobject_cast<PartitionWindow *>( subW->widget() );
			QTimer::singleShot( 0, w, SLOT( on_actionRefresh_List_triggered() ) );
		}
		break;
    case mdiGame:
		{
			GameWindow *w = qobject_cast<GameWindow *>( subW->widget() );
			QTimer::singleShot( 0, w, SLOT( ReloadGame() ) );
		}
		break;
    case mdiCovers:
		{
			CoverManagerWindow *w = qobject_cast<CoverManagerWindow *>( subW->widget() );
			QTimer::singleShot( 0, w, SLOT( Refresh() ) );
		}
		break;
    default://other window types not supported by "refresh"
		break;
    }
}

//tools -> format WBFS
void MainWindow::on_actionFormat_WBFS_triggered()
{
    FormatWindow w( this );
    w.exec();
}

void MainWindow::on_actionOpen_DVD_r_triggered()
{
    QStringList drives = DvdSelectDialog::GetDvdToOpen( this );
    if( drives.size() )
        OpenGames( drives );
}
