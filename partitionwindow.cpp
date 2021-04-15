#include "partitionwindow.h"
#include "gamecopydialog.h"
#include "ui_partitionwindow.h"
#include "tools.h"
#include "filefolderdialog.h"
#include "fsinfo.h"

PartitionWindow::PartitionWindow( QWidget *parent ) : QMainWindow( parent ), ui( new Ui::PartitionWindow )
{
    partition = NULL;
    shrinkingGc = false;
    busy = false;
    needToReload = false;

    ui->setupUi( this );
    ui->progressBar->setVisible( false );
    ui->statusbar->addPermanentWidget( ui->progressBar, 0 );
    ui->statusbar->addPermanentWidget( ui->label_status, 0 );
    ui->label_status->setText( "blabla" );
    ui->menubar->hide();
    ui->treeWidget->sortItems( GAME_NAME_COLUMN, Qt::AscendingOrder );

    QFontMetrics fm( fontMetrics() );
    ui->treeWidget->header()->resizeSection( GAME_ID_COLUMN, fm.width( "WWWWWWWWW" ) );//id
    ui->treeWidget->header()->resizeSection( GAME_NAME_COLUMN, fm.width( QString( 22, 'W' ) ) );//name
    ui->treeWidget->header()->resizeSection( GAME_SIZE_COLUMN, fm.width( "WWWWW" ) );//size
    ui->treeWidget->header()->resizeSection( GAME_REGION_COLUMN, fm.width( "WWWWW" ) );//region
    ui->treeWidget->header()->resizeSection( GAME_TYPE_COLUMN, fm.width( "WWWWWWW" ) );//type
    ui->treeWidget->header()->resizeSection( GAME_PARTITIONS_COLUMN, fm.width( "WWWWW" ) );//# partitions


    QSettings s( settingsPath, QSettings::IniFormat );
    bool root = s.value( "root/enabled" ).toBool();
    wit.SetRunAsRoot( root );
    wwt.SetRunAsRoot( root );
    wit.SetTitlesTxtPath( s.value( "paths/titlesTxt", "" ).toString() );
    ignoreFst = s.value( "ignoreFst", false ).toBool();

    connect( ui->treeWidget, SIGNAL( customContextMenuRequested( const QPoint& ) ), this, SLOT( CustomTreeWidgetContentmenu( const QPoint& ) ) );

    connect( &wwt, SIGNAL( RequestPassword() ), this, SLOT( NeedToAskForPassword() ) );
    connect( this, SIGNAL( UserEnteredPassword() ), &wwt, SLOT( PasswordIsEntered() ) );

    connect( &wit, SIGNAL( RequestPassword() ), this, SLOT( NeedToAskForPassword() ) );
    connect( this, SIGNAL( UserEnteredPassword() ), &wit, SLOT( PasswordIsEntered() ) );

    connect( &wit, SIGNAL( SendProgress( int ) ), ui->progressBar, SLOT( setValue( int ) ) );
    connect( &wwt, SIGNAL( SendProgress( int ) ), ui->progressBar, SLOT( setValue( int ) ) );

    connect( &wwt, SIGNAL( SendJobDone( int ) ), this, SLOT( HideProgressBar( int ) ) );
    connect( &wit, SIGNAL( SendJobDone( int ) ), this, SLOT( HideProgressBar( int ) ) );
	connect( &wit, SIGNAL( SendMessageForStatusBar( const QString &) ), this, SLOT( GetStatusTextFromWiimms( const QString &) ) );
	connect( &wwt, SIGNAL( SendMessageForStatusBar( const QString &) ), this, SLOT( GetStatusTextFromWiimms( const QString &) ) );

	connect( &wwt, SIGNAL( SendFatalErr( const QString&, int ) ), this, SLOT( HandleWiimmsErrors( const QString&, int ) ) );
	connect( &wit, SIGNAL( SendFatalErr( const QString&, int ) ), this, SLOT( HandleWiimmsErrors( const QString&, int ) ) );
	connect( &wit, SIGNAL( SendListLLL( const QList<QTreeWidgetItem *>&, const QString &) ),
			 this, SLOT( GetPartitionInfo( const QList<QTreeWidgetItem *>&, const QString &) ) );

	connect( ui->treeWidget, SIGNAL( currentItemChanged( QTreeWidgetItem *, QTreeWidgetItem *)  ),
			 this, SLOT( TreeSelectionChanged( QTreeWidgetItem *, QTreeWidgetItem *) ) );

    //
}

PartitionWindow::~PartitionWindow()
{
    //qDebug() << "PartitionWindow::~PartitionWindow()";
    emit SendGamelistFor_1_Partition( partition->text( 0 ), ui->treeWidget->invisibleRootItem()->takeChildren() );
    delete ui;
}

void PartitionWindow::TreeSelectionChanged( QTreeWidgetItem * current, QTreeWidgetItem * previous )
{
    if( !current )
		return;

    if( previous )
		if( current->text( 0 ) == previous->text( 0 ) )
			return;

    emit GameClicked( current->text( 0 ) );
}

void PartitionWindow::closeEvent( QCloseEvent * closeEvent )
{
    //qDebug() << "PartitionWindow::closeEvent()";
    if( ui->treeWidget->topLevelItemCount() )
    {
		QList<QTreeWidgetItem *> list( ui->treeWidget->invisibleRootItem()->takeChildren() );
		emit SendGamelistFor_1_Partition( partition->text( 0 ), list );
    }
    QWidget::closeEvent( closeEvent );
}

void PartitionWindow::SetPartition( QTreeWidgetItem *part )
{
    //qDebug() << "PartitionWindow::SetPartition" << part->text( 0 ) << part->text( 3 ) << part->text( 5 );
    setWindowTitle( part->text( 0 ) );
    //TODO:  still need to somehow change the name of the parent custommdi window for its GetTitle()
    if( partition )
		delete partition;

    partition = part->clone();

    QList<QTreeWidgetItem *> oldList = ui->treeWidget->invisibleRootItem()->takeChildren();
    while( !oldList.isEmpty() )
    {
		QTreeWidgetItem *item = oldList.takeFirst();
		delete item;
    }
    UpdateFlagText();
}

void PartitionWindow::SetPartitionAndGameList( QTreeWidgetItem *part, const QList<QTreeWidgetItem *> &gameList )
{
    SetPartition( part );
    //qDebug() << __FUNCTION__ << gameList.size();
    if( gameList.size() )
    {
		foreach( QTreeWidgetItem *game, gameList )
		{
			QTreeWidgetItem *newGame = game->clone();//make a deep copy of the items so when this window is destroyed, the original list is not touched

			if( !newGame->text( 2 ).contains( "." ) )//if the size contains a '.' then we must have already converted the size to GiB.  no need to do it again
				newGame->setText( 2, SizeTextGiB( newGame->text( 2 ) ) );
			ui->treeWidget->addTopLevelItem( newGame );
		}
    }
    else
    {
		//we werent given a gamelist, but only a partition.  ask wit for the games
		on_actionRefresh_List_triggered();
    }

    //ui->treeWidget->addTopLevelItems( gameList );
}

//respond to fatal errors from the processes
void PartitionWindow::HandleWiimmsErrors( const QString &err, int id )
{
    qWarning() << "PartitionWindow::HandleWiimmsErrors" << err << id;
    unsetCursor();
    ui->progressBar->setVisible( false );
    busy = false;
    switch( id )
    {
    case witListLLLHDD:
		QMessageBox::critical( this, tr( "Error reading partition data" ), err );
		emit ReportInvalidPartition( partition->text( 0 ) );
		break;
    case wwtAdd:
    case witCopy:
		//case witEdit:
		emit PartitionIsDirty( dirtyPartition );
		QMessageBox::critical( this, tr( "Error writing games" ), err );
		if( writingToWBFS )
			SetPartitionEnabled( busyWBFSPath, true );
		ui->statusbar->showMessage( tr( "Error writing games" ) );
		break;
    case GC_FATAL:
		if( shrinkingGc )
		{
			delete gcGame;
			gcGame = NULL;
			shrinkingGc = false;
		}
		QMessageBox::critical( this, tr( "Error shrinking games" ), err );
		ui->statusbar->showMessage( tr( "Error shrinking games" ) );
		break;

    default:
		qDebug() << "unhandled error in PartitionWindow::HandleWiimmsErrors" << err << id;
		break;
    }
}

void PartitionWindow::GetPartitionInfo( const QList<QTreeWidgetItem *> &games, const QString &MibUsed )
{
    //qDebug() << "PartitionWindow::GetPartitionInfo" << games.size() << MibUsed;
    unsetCursor();
    QString gameCntStr;
    QTextStream( &gameCntStr ) << games.size();
    busy = false;

    //add new games from the list
    partition->setText( 1, gameCntStr );
    partition->setText( 2, MibUsed );

    //delete old gamelist
    QList<QTreeWidgetItem *> oldList = ui->treeWidget->invisibleRootItem()->takeChildren();
    while( !oldList.isEmpty() )
    {
		QTreeWidgetItem *item = oldList.takeFirst();
		delete item;
    }

    //make a copy for the main window
    QList<QTreeWidgetItem *> gamesCopy;
    for( int i = 0; i < games.size(); i++ )
    {
		gamesCopy << games.at( i )->clone();
		games.at( i )->setText( 2, SizeTextGiB( games.at( i )->text( 2 ) ) );
    }

    if( games.size() )
    {	ui->treeWidget->addTopLevelItems( games );
    }

    ui->statusbar->showMessage( "" );
    UpdateFlagText();
    emit SendGamelistFor_1_Partition( partition->text( 0 ), gamesCopy );
    emit SendUpdatedPartitionInfo( partition );
}

//forward password request from wit/wwt to main window to avoid multiple windows asking for it at the same time
void PartitionWindow::NeedToAskForPassword()
{
    emit AskMainWindowForPassword();
}

//tell wit/wwt that the user entered a password
void PartitionWindow::GetPasswordFromMainWindow()
{
    emit UserEnteredPassword();
}

//tools->refresh
void PartitionWindow::on_actionRefresh_List_triggered()
{
    if( !partition )
		return;

    if( !busy )
    {
		//qDebug() << "PartitionWindow::Reload() - reloading now";
		needToReload = false;
		busy = true;
		setCursor( Qt::BusyCursor );
		ui->statusbar->showMessage( tr( "Reading game information..." ) );
		QSettings s( settingsPath, QSettings::IniFormat );
		int rDepth = s.value( "wit_wwt/rdepth", 10 ).toInt();
		wit.ListLLL_HDD( partition->text( 0 ), rDepth, ignoreFst );
    }
    else
    {
		//qDebug() << "PartitionWindow::Reload() - reloading later";
		needToReload = true;
    }
}

//respond to message that the user has edited the settings
void PartitionWindow::SettingsHaveChanged()
{
    //qDebug() << "PartitionWindow::SettingsHaveChanged()" << partition->text( 0 );
    QSettings s( settingsPath, QSettings::IniFormat );
    bool root = s.value( "root/enabled" ).toBool();
    wit.SetRunAsRoot( root );
    wwt.SetRunAsRoot( root );
    wit.SetTitlesTxtPath( s.value( "paths/titlesTxt", "" ).toString() );
    ignoreFst = s.value( "ignoreFst", false ).toBool();
}

//get a copy of the partition list from the main window
void PartitionWindow::SetPartitionList( const QList<QTreeWidgetItem *> &pList )
{
    //qDebug() << "PartitionWindow::SetPartitionList" << pList.size();
    while( !partList.isEmpty() )//delete all known partitions from the list
    {
		QTreeWidgetItem *item = partList.takeFirst();
		delete item;
    }

    int size = pList.size();
    for( int i = 0; i < size; i++ )
    {
		if( pList.at( i )->text( 0 ) == partition->text( 0 ) )
		{
			delete partition;
			partition = pList.at( i )->clone();
			UpdateFlagText();
		}
		partList << pList.at( i )->clone();
    }
}

//context menu for the partition tree window
void PartitionWindow::CustomTreeWidgetContentmenu( const QPoint& pos )
{
    QPoint globalPos = ui->treeWidget->viewport()->mapToGlobal(pos);
    QTreeWidgetItem* selected = ui->treeWidget->itemAt( pos );
    if( !selected )//right-clicked in the partition window, but not on a game
    {
		qDebug() << "no item";
		return;
    }

    //gather information about which games are selected
    int selectedCount = ui->treeWidget->selectedItems().count();
    bool allSelectedGamesAreWii = true;
    bool allSelectedGamesAreGC = true;
    bool allGamesAreGcIso = true;
    foreach( QTreeWidgetItem* item, ui->treeWidget->selectedItems() )
    {
		QString typeStr = item->text( 4 );
		if( typeStr.contains( "GC" ) ) // all GC types contain this and none of the wii ones do
		{
            if( typeStr != "ISO/GC" )
                allGamesAreGcIso = false;

			allSelectedGamesAreWii = false;
        }
		else
		{
			allSelectedGamesAreGC = false;
            allGamesAreGcIso = false;
		}
    }
    if( allSelectedGamesAreWii )
		qDebug() << "allSelectedGamesAreWii";
    if( allSelectedGamesAreGC )
		qDebug() << "allSelectedGamesAreGC";
    if( allSelectedGamesAreWii && allSelectedGamesAreGC )
		qDebug() << "WTF";

    //create the context menu based on the games selected
    QMenu myMenu( this );
    QAction infoA( tr( "%1 %2").arg( selectedCount ).arg( selectedCount > 1 ? tr( "Games Selected" ) : tr( "Game Selected" ) ), &myMenu );
    infoA.setEnabled( false );
    QAction cpA( tr( "Copy / Convert" ), &myMenu );
    QAction rmA( tr( "Delete" ), &myMenu );
    QAction verifyA( tr( "Verify" ), &myMenu );
    QAction browseA( tr( "Open in Game Browser" ), &myMenu );
    QAction gcAlA( tr( "Align Files && Shrink" ), &myMenu );//built-in GC shrinker, doesnt use wiimms' tools


    myMenu.addAction( &infoA );
    myMenu.addSeparator();
    if( !busy )
    {
		myMenu.addAction( &cpA );
		if( partition->text( 5 ) == "WBFS" )//WBFS remove
			myMenu.addAction( &rmA );
		if( allGamesAreGcIso )//align GC games
			myMenu.addAction( &gcAlA );
		if( selectedCount == 1 && allSelectedGamesAreWii )//verify wii games
			myMenu.addAction( &verifyA );
    }
    myMenu.addAction( &browseA );

    //execute the menu
    QAction* selectedItem = myMenu.exec( globalPos );
    //respond to what was selected
    if( selectedItem )
    {
		// something was chosen, do stuff
		if( selectedItem == &cpA )
		{
			QStringList games;
			foreach( QTreeWidgetItem* item, ui->treeWidget->selectedItems() )
				games << item->text( 6 );

			QStringList args = GameCopyDialog::WitCopyCommand( this, partition->text( 0 ), partList, games, QStringList() );
			//qDebug() << args;
			if( args.size() < 2 )//brief sanity check
				return;

			QString prog = args.takeFirst();
			//qDebug() << args;
			ui->progressBar->setVisible( true );
			setCursor( Qt::BusyCursor );
			if( prog == "wwt" )
			{
				busy = true;
				SetPartitionEnabled( args.at( 2 ), false );
				dirtyPartition = args.at( 2 );
				wwt.RunJob( args, wwtAdd );
			}
			else if( prog == "wit" )
			{
				busy = true;
				dirtyPartition = args.takeFirst();
				wit.RunJob( args, witCopy );
			}
			else
				qDebug() << "WFT invalid job" << prog;
		}
		else if( selectedItem == &rmA )//should only happen if the current games are on a wbfs partition
		{
			int ok = QMessageBox::critical( this, tr( "Are you sure?" ), tr( "You are about to remove %1 games from %2.  This is your last chance to back out.")\
											.arg( ui->treeWidget->selectedItems().count() ).arg( partition->text( 0 ) ), QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel );

			if( ok != QMessageBox::Ok )
				return;

			busy = true;
			QStringList args = QStringList() << "REMOVE" << "--part=" + partition->text( 0 );
			foreach( QTreeWidgetItem* item, ui->treeWidget->selectedItems() )
				args << item->text( 0 );
			qDebug() << args;
			wwt.RunJob( args, wwtRemove );
		}
		else if( selectedItem == &browseA )
		{
			QStringList games;
			foreach( QTreeWidgetItem* item, ui->treeWidget->selectedItems() )
			{
				games << gamePath( item );
			}
			emit BrowseGames( games );
		}
		else if( selectedItem == &gcAlA )//shrick GC games
		{
			gcGameList.clear();
			switch( selectedCount )
			{
			case 1:
				{
					gcDestination = QFileDialog::getSaveFileName( this, tr( "New File Name" ) );
					gcSingleFile = true;
				}
				break;
			default:
				{
					gcDestination = QFileDialog::getExistingDirectory( this, tr( "Select destination folder" ) );
					gcSingleFile = false;
				}
				break;
			}
			if( gcDestination.isEmpty() )
				return;
			busy = true;
			gcTotalgames = selectedCount;
			foreach( QTreeWidgetItem* item, ui->treeWidget->selectedItems() )
			{
#ifdef Q_WS_WIN
                bool ok = false;
                QString path = FsInfo::ToWinPath( item->text( 6 ), &ok );
                if( !ok )
                {
                    qDebug() << "error converting path to windows path" << item->text( 6 );
                    continue;
                }
                gcGameList << path;
#else
                gcGameList << item->text( 6 );
#endif
			}
			qDebug() << gcDestination;
			QTimer::singleShot( 500, this, SLOT( ShrinkNextGame() ) );
		}
    }
    else
    {
		// nothing was chosen
    }
}

//this is connected to "wwt ADD", "wwt REMOVE", "wit COPY", and "wit EDIT" -> done
void PartitionWindow::HideProgressBar( int job )
{
    ui->progressBar->setVisible( false );
    unsetCursor();
    busy = false;
    switch( job )
    {
    case wwtAdd:
		emit PartitionIsDirty( dirtyPartition );
		ui->statusbar->showMessage( tr( "Done adding games to WBFS partition" ) );
		break;
    case wwtRemove://refresh the game list
		{
			ui->statusbar->showMessage( tr( "Games successfully deleted from WBFS partition" ) );
			needToReload = true;
		}
		break;
    case witCopy:
		emit PartitionIsDirty( dirtyPartition );
		ui->statusbar->showMessage( tr( "Done copying & converting games" ) );
		break;
		//case witEdit:
		//ui->statusbar->showMessage( tr( "Game patched OK" ) );
		//break;
    default:
		ui->statusbar->showMessage( QString( "You shouldn\'t see this ( %1 )" ).arg( job ) );
		break;
    }

    if( writingToWBFS )
    {
		qDebug() << "GameWindow::HideProgressBar -> about to enable the wbfs partition";
		SetPartitionEnabled( busyWBFSPath, true );
    }

    if( needToReload )
		on_actionRefresh_List_triggered();
}

//get status message and append it to the status bar
void PartitionWindow::GetStatusTextFromWiimms( const QString &text )
{
    ui->statusbar->showMessage( text );
}

//flag a partition as "busy" and send that to the main window so it can tell all other windows not to mess with that partition
void PartitionWindow::SetPartitionEnabled( const QString &part, bool enabled )
{
    qDebug() << "PartitionWindow::SetPartitionEnabled :" << part << enabled;
    for( int i = 0; i < partList.size(); i++ )
    {
		if( partList.at( i )->text( 0 ) == part )
		{
			partList.at( i )->setText( 6, enabled ? "" : "busy" );
			emit SendUpdatedPartitionInfo( partList.at( i ) );

			writingToWBFS = !enabled;
			if( !enabled && !busyWBFSPath.isEmpty() )
				qDebug() << "PartitionWindow::SetPartitionEnabled :!enabled && !busyWBFSPath.isEmpty()";
			busyWBFSPath = enabled ? "" : part;
			return;
		}
    }
    qDebug() << "PartitionWindow::SetPartitionEnabled :no partition matched" << part;
}

//start shrink the next gamecube game in the list
void PartitionWindow::ShrinkNextGame()
{
    if( shrinkingGc )
    {
		delete gcGame;
		gcGame = NULL;
		shrinkingGc = false;
    }
    if( !gcGameList.size() )//no games to shrink
    {
		ui->progressBar->setVisible( false );
		unsetCursor();
		ui->statusbar->showMessage( tr( "Ready" ) );
		return;
    }
    setCursor( Qt::BusyCursor );
    if( gcDestination.isEmpty() )
    {
		qDebug() << "PartitionWindow::ShrinkNextGame error" << gcDestination << gcGameList.size();
        QMessageBox::critical( this, tr( "Error In GameCube Destination" ), tr( "Destination filename is empty" ) );
		ui->progressBar->setVisible( false );
		unsetCursor();
		return;
    }
    //QFileInfo fi( gcDestination );

    QString dest;
    QString source = gcGameList.takeFirst();
    //qDebug() << gcDestination;

    if( gcSingleFile )
    {
		if( gcGameList.size() > 1 )
		{
			qDebug() << "PartitionWindow::ShrinkNextGame error: !dir";
            QMessageBox::critical( this, tr( "Error shrinking game" ), tr( "Unable to parse the game correctly.") );
			ui->progressBar->setVisible( false );
			unsetCursor();
			return;
		}
		dest = gcDestination;
    }
    else
    {
		QFileInfo fi2( source );
		dest = gcDestination + "/" + fi2.fileName();
    }
    //qDebug() << dest << source;

    gcGame = new GC_ShrinkThread( this, source );
    if( !gcGame->fileOk )
    {
		qDebug() << "PartitionWindow::ShrinkNextGame error: !fileOk" << source;
		delete gcGame;
		ui->progressBar->setVisible( false );
		unsetCursor();
		return;
    }
    shrinkingGc = true;
    ui->progressBar->setVisible( true );
    QString cur;
    int thisGame = ( gcTotalgames - ( gcGameList.size() ) );
    QTextStream( &cur ) << " [ " << thisGame << " / " << gcTotalgames<< " ] ";
    ui->statusbar->showMessage( tr( "Aligning" ) + cur );

    connect( gcGame, SIGNAL( SendProgress( int ) ), ui->progressBar, SLOT( setValue( int ) ) );
    connect( gcGame, SIGNAL( SendDone() ), this, SLOT( ShrinkNextGame() ) );
	connect( gcGame, SIGNAL( SendFatalError( const QString&, int ) ), this, SLOT( HandleWiimmsErrors( const QString&, int ) ) );

    QSettings s( settingsPath, QSettings::IniFormat );
    bool overwrite = s.value( "wit_wwt/overWriteFiles" ).toBool();
    gcGame->ShrinkTo( dest, overwrite );

}

//update the text showing this partitions # of games and its flags
void PartitionWindow::UpdateFlagText()
{
    QString fs = partition->text( 5 );
    QString flags;
#ifdef Q_WS_WIN
    if( partition->text( 5 ) == "WBFS" )
    {
        bool ok = false;
        QString letter = FsInfo::ToWinPath( partition->text( 0 ), &ok );
        if( ok && letter.endsWith( ":" ) )
            flags += letter + " | ";
    }
#endif
    int cnt = ui->treeWidget->topLevelItemCount();
    QString games = cnt == 1 ? tr( "%1 Game" ).arg( cnt ) : tr( "%1 Games" ).arg( cnt );
    flags += fs;
    if( partition->text( 3 ) == tr( "Yes" ) )
		flags += " | " + tr( "Split" );

    flags += " | " + games;

    ui->label_status->setText( flags );
}











