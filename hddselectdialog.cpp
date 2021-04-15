#include "ui_hddselectdialog.h"
#include "hddselectdialog.h"
#include "passworddialog.h"

#include "includes.h"
#include "tools.h"
#include "fsinfo.h"

HDDSelectDialog::HDDSelectDialog( QWidget *parent ) : QDialog( parent ), ui( new Ui::HDDSelectDialog )
{
    ui->setupUi( this );
    ui->treeWidget->header()->resizeSection( 0, 300 );
    ui->treeWidget->setHeaderHidden( false );

    QFontMetrics fm( fontMetrics() );
    ui->treeWidget->header()->resizeSection( 0, fm.width( QString( 24, 'W' ) ) );//path
    ui->treeWidget->header()->resizeSection( 1, fm.width( "WWWWW" ) );//#games
    ui->treeWidget->header()->resizeSection( 2, fm.width( "WWWWWWW" ) );//size
    ui->treeWidget->header()->resizeSection( 3, fm.width( "WWWWWWW" ) );//split
    ui->treeWidget->header()->resizeSection( 4, fm.width( "WWWWW" ) );//source

    oktoRequestNextLIST_LLL = true;
    alreadyAskingForPassword = false;
    //setWindowTitle( tr( "Partition Selection" ) );

    connect( ui->treeWidget, SIGNAL( customContextMenuRequested( const QPoint& ) ), this, SLOT( CustomTreeWidgetContentmenu( const QPoint& ) ) );

    QSettings s( settingsPath, QSettings::IniFormat );
    wit.SetTitlesTxtPath( s.value( "paths/titlesTxt", "" ).toString() );

    int size = s.beginReadArray( "ignoreFolders" );
    for( int i = 0; i < size; i++ )
    {
		s.setArrayIndex( i );
		ignorePaths << s.value( "path" ).toString();
    }
    s.endArray();

    ignoreFst = s.value( "ignoreFst", false ).toBool();

#ifndef Q_WS_WIN
    bool root = s.value( "root/enabled" ).toBool();
    wit.SetRunAsRoot( root );
    wwt.SetRunAsRoot( root );

    connect( &wwt, SIGNAL( RequestPassword() ), this, SLOT( NeedToAskForPassword() ) );
    connect( this, SIGNAL( UserEnteredPassword() ), &wwt, SLOT( PasswordIsEntered() ) );

    connect( &wit, SIGNAL( RequestPassword() ), this, SLOT( NeedToAskForPassword() ) );
    connect( this, SIGNAL( UserEnteredPassword() ), &wit, SLOT( PasswordIsEntered() ) );

#endif

	connect( &wwt, SIGNAL( SendPartitionList( const QStringList &) ), this, SLOT( GetWBFSPartitionList( const QStringList &) ) );
	connect( &wwt, SIGNAL( SendFatalErr( const QString&, int ) ), this, SLOT( HandleWiimmsErrors( const QString&, int ) ) );
	connect( &wit, SIGNAL( SendFatalErr( const QString&, int ) ), this, SLOT( HandleWiimmsErrors( const QString&, int ) ) );
	connect( &wit, SIGNAL( SendListLLL( const QList<QTreeWidgetItem *>&, const QString &) ),
			 this, SLOT( GetPartitionInfo( const QList<QTreeWidgetItem *>&, const QString &) ) );
}

HDDSelectDialog::~HDDSelectDialog()
{
    delete ui;
}

void HDDSelectDialog::DestroyProcessesAndWait()
{
    setCursor( Qt::BusyCursor );
    wit.Kill();
    wwt.Kill();
    wit.Wait();
    wwt.Wait();
    unsetCursor();
}

void HDDSelectDialog::closeEvent( QCloseEvent * closeEvent )
{
    DestroyProcessesAndWait();
    emit SendHDDList( ui->treeWidget->invisibleRootItem()->takeChildren() );
    QWidget::closeEvent( closeEvent );
}

//add items to the list if no partition with the same name is already there
void HDDSelectDialog::AddPartitionsToList( const QList<QTreeWidgetItem *> &list )
{
    for( int i = 0; i < list.size(); i++ )
    {
		bool found = false;
		for( int j = 0; j < ui->treeWidget->topLevelItemCount(); j++ )
		{
#ifdef Q_WS_WIN
            if( list.at( i )->text( 0 ) == RemoveDriveLetter( ui->treeWidget->topLevelItem( j )->text( 0 ) ) )
#else
				if( list.at( i )->text( 0 ) == ui->treeWidget->topLevelItem( j )->text( 0 ) )
#endif
					found = true;
		}
		if( !found )
		{
			QTreeWidgetItem *item = list.at( i )->clone();
#ifdef Q_WS_WIN
            if( item->text( 4 ) == "wwt" )
                item->setText( 0, AddDriveLetter( item->text( 0 ) ) );
#endif
			if( !item->text( 2 ).contains( "." ) )//this size text doesnt contain a ".", so assume we havent already converted it from bytes to GiB
				item->setText( 2, SizeTextGiB( item->text( 2 ) ) );
			ui->treeWidget->addTopLevelItem( item );
		}
    }
}

//they clicked "ok" button
void HDDSelectDialog::accept()
{
    DestroyProcessesAndWait();

    QList<QTreeWidgetItem *> selected = ui->treeWidget->selectedItems();
    if( !selected.isEmpty() )
		emit SendSelectedPartition( selected );

    emit SendHDDList( ui->treeWidget->invisibleRootItem()->takeChildren() );
    QDialog::accept();
}

//they clicked "cancel"
void HDDSelectDialog::reject()
{
    DestroyProcessesAndWait();
    emit SendHDDList( ui->treeWidget->invisibleRootItem()->takeChildren() );
    QDialog::reject();
}

#ifndef Q_WS_WIN
//wit/wwt has asked for a password
void HDDSelectDialog::NeedToAskForPassword()
{
    if( alreadyAskingForPassword )
		return;

    alreadyAskingForPassword = true;
    PasswordDialog dialog( this );
    dialog.exec();
    alreadyAskingForPassword = false;
    emit UserEnteredPassword();
}
#endif
//manually add partition / folder
void HDDSelectDialog::on_pushButton_manualADD_clicked()
{
    QString str = QFileDialog::getExistingDirectory( this, tr( "Select a folder" ) );
    if( str.isEmpty() )
		return;

    AddNewPartitionToList( str, tr( "Manual" ) );
}

//auto add partition / folders
void HDDSelectDialog::on_pushButton_find_clicked()
{
    ui->buttonBox->setEnabled( false );
    oktoRequestNextLIST_LLL = true;
    QList<QTreeWidgetItem *> lst = ui->treeWidget->invisibleRootItem()->takeChildren();
    for( int j = 0; j < lst.size(); j++ )
    {
		QTreeWidgetItem *it = lst.at( j );
		delete it;
    }
#ifdef Q_WS_WIN
    QFileInfoList list = QDir::drives();

#else
#ifdef Q_WS_MAC
    QDir dir( "/Volumes" );
#else
    QDir dir( "/media" );
#endif

    dir.setFilter( QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::NoSymLinks );
    QFileInfoList list = dir.entryInfoList();

#endif//#ifdef Q_WS_WIN
    for( int i = 0; i < list.size(); ++i )
    {
		QFileInfo fileInfo = list.at( i );
        if( PathIsIgnored( fileInfo.absoluteFilePath() )
#ifdef Q_WS_WIN
            || FsInfo::IsDVDLetter( fileInfo.absoluteFilePath() )
#endif
            )
			continue;

		QDir subDir( fileInfo.absoluteFilePath() );
		//QDir subDir( dir.absoluteFilePath( fileInfo.fileName() ) );

		if( subDir.exists( "wbfs" ) )
		{
			if( PathIsIgnored( subDir.absoluteFilePath( "wbfs" ) ) )
				continue;
			AddNewPartitionToList( subDir.absoluteFilePath( "wbfs" ), tr( "Auto" ) );
		}

		if( subDir.exists( "iso" ) )
		{
			if( PathIsIgnored( subDir.absoluteFilePath( "iso" ) ) )
				continue;
			AddNewPartitionToList( subDir.absoluteFilePath( "iso" ), tr( "Auto" ) );
		}

		if( subDir.exists( "games" ) )
		{
			if( PathIsIgnored( subDir.absoluteFilePath( "games" ) ) )
				continue;
			AddNewPartitionToList( subDir.absoluteFilePath( "games" ), tr( "Auto" ) );
		}
    }
    wwt.GetPartitions();
}

//check if a path is ignored, or is a subfolder of an ignored path
bool HDDSelectDialog::PathIsIgnored( const QString &path )
{
    int size  = ignorePaths.size();
    for( int i = 0; i < size; i++ )
    {
		if( path.startsWith( ignorePaths.at( i ) ) )
		{
			//qDebug() << "ignoring" << path << "based on ignore rule" << ignorePaths.at( i );
			return true;
		}
    }
    return false;
}

//receive WBFS partitions from wwt
void HDDSelectDialog::GetWBFSPartitionList( const QStringList &list )
{
    qDebug() << "HDDSelectDialog::GetWBFSPartitionList" << list;
    unsetCursor();
    foreach( QString part, list )
    {
        AddNewPartitionToList( part, "wwt" );
    }
}

//respond somehow to fatal errors
void HDDSelectDialog::HandleWiimmsErrors( const QString &err, int id )
{
    qDebug() << "wiimms error" << err << id;
    unsetCursor();
    ui->buttonBox->setEnabled( true );//probably shouldnt enable both of these right here every time.
    ui->pushButton_reScan->setEnabled( true );
    switch( id )
    {
	case witListLLLHDD:
		{
			QTreeWidgetItem * item = PartitionBeingRead();
			if( item )
			{
				item->setText( 1, "0" );
				item->setText( 2, tr( "Wit Error" ) + ": " + err );
				break;
			}

			break;
		}

    }
}

//which partition is currently being read
QTreeWidgetItem *HDDSelectDialog::PartitionBeingRead()
{
    for( int i = 0; i < ui->treeWidget->topLevelItemCount(); i++ )
    {
		QTreeWidgetItem * item = ui->treeWidget->topLevelItem( i );
		if( item->text( 1 ) == tr( "Reading" ) )
		{
			return item;
		}
    }
    qDebug() << "error @ partition being read";
    return 0;
}

//check a path and if it doesn't mach anything already on our list, add it.  trigger the "LIST-LLL" chain to start
void HDDSelectDialog::AddNewPartitionToList( const QString &path, const QString &source )
{
    QList<QTreeWidgetItem *> found = ui->treeWidget->findItems( path, Qt::MatchFixedString );

    if( !found.isEmpty() )
    {
		qDebug() << "partition already exists" << path;
		return;
    }

    QTreeWidgetItem * item = new QTreeWidgetItem( QStringList() << path );
    item->setText( 4, source );
    if( source == "wwt" )
    {
#ifdef Q_WS_WIN
        item->setText( 0, AddDriveLetter( item->text( 0 ) ) );
#endif
		item->setText( 5, "WBFS" );
    }
    ui->treeWidget->addTopLevelItem( item );

    //read partition settings
    QSettings settings( settingsPath, QSettings::IniFormat );
    int size = settings.beginReadArray( "partitionOptions" );
    for( int i = 0; i < size; i++ )
    {
		settings.setArrayIndex( i );
		if( settings.value( "path" ).toString() == item->text( 0 ) )
			item->setText( 3, settings.value( "split" ).toString() );
    }
    settings.endArray();

    RequestNextLIST_LLLL();
}

//receive info about a specific partition from wit ( response to list-lll )
void HDDSelectDialog::GetPartitionInfo( const QList<QTreeWidgetItem *> &games, const QString &MibUsed )
{
    //qDebug() << MibUsed;
    QTreeWidgetItem *item = PartitionBeingRead();
    if( !item )
    {
		qDebug() << "!PartitionBeingRead()";
		return;
    }
    oktoRequestNextLIST_LLL = true;
    RequestNextLIST_LLLL();
    QString count;
    QTextStream( &count ) << games.size();
    item->setText( 1, count );
    item->setText( 2, SizeTextGiB( MibUsed ) );

    if( item->text( 4 ) != "wwt" )//if the partition came from wwt, assume it is WBFS.  otherwise, get the filesystem and set some flags
    {
		QString fs = FsInfo::GetFilesystem( item->text( 0 ) );
		if( fs.isEmpty() )
			fs = tr( "Unknown" );

		if( fs.contains( "FAT", Qt::CaseInsensitive ) )
		{
			if( item->text( 0 ).endsWith( "\\games" ) || item->text( 0 ).endsWith( "/games" ) )//FAT partition ending with a folder called "games"  flag it as SNEEK
			{
				fs = "SNEEK";
			}
			else//no "games" folder, set the flag to split large files
			{
				item->setText( 3, tr( "Yes" ) );
			}
		}
		item->setText( 5, fs );
    }

    if( item == ui->treeWidget->topLevelItem( ui->treeWidget->topLevelItemCount() - 1 ) )
    {
		ui->pushButton_reScan->setEnabled( true );
		ui->buttonBox->setEnabled( true );
		unsetCursor();
    }
#ifdef Q_WS_WIN
    emit SendGamelistFor_1_Partition( RemoveDriveLetter( item->text( 0 ) ), games );
#else
    emit SendGamelistFor_1_Partition( item->text( 0 ), games );
#endif
}

//look for the next HDD in the list that hasn't been scanned and ask the data from wit
void HDDSelectDialog::RequestNextLIST_LLLL()
{
    ui->pushButton_reScan->setEnabled( false );
    if( !oktoRequestNextLIST_LLL )//only list-lll 1 hdd at a time
    {
		//qDebug() << "!oktoRequestNextLIST_LLL";
		return;
    }

    for( int i = 0; i < ui->treeWidget->topLevelItemCount(); i++ )
    {
		QTreeWidgetItem * item = ui->treeWidget->topLevelItem( i );
		if( item->text( 1 ).isEmpty() )
		{
			item->setText( 1, tr( "Reading" ) );
			oktoRequestNextLIST_LLL = false;
			setCursor( Qt::BusyCursor );
			QSettings s( settingsPath, QSettings::IniFormat );
			int rDepth = s.value( "wit_wwt/rdepth", 10 ).toInt();
			wit.ListLLL_HDD( item->text( 0 ), rDepth, ignoreFst );
			return;
		}
    }
    // all HDDs have been scanned at this point
}

//clear the gamecounts & Mib used and rescan them
void HDDSelectDialog::on_pushButton_reScan_clicked()
{
    ui->buttonBox->setEnabled( false );
    if( !oktoRequestNextLIST_LLL )//dont allow this button to be clicked until this dialog has "settled"
		return;

    ui->pushButton_reScan->setEnabled( false );

    for( int i = 0; i < ui->treeWidget->topLevelItemCount(); i++ )
    {
		QTreeWidgetItem * item = ui->treeWidget->topLevelItem( i );
		item->setText( 1, QString() );
		item->setText( 2, QString() );
    }

    oktoRequestNextLIST_LLL = true;
    RequestNextLIST_LLLL();
}

//context menu for the partition tree window
void HDDSelectDialog::CustomTreeWidgetContentmenu( const QPoint& pos )
{
    //qDebug() << "HDDSelectDialog::CustomTreeWidgetContentmenu" << pos;
    QPoint globalPos = ui->treeWidget->viewport()->mapToGlobal(pos);
    QTreeWidgetItem* selected = ui->treeWidget->itemAt( pos );
    if( !selected )
    {
		//no item is at the spot clicked
		return;
    }

    //bool checked = ui->treeWidget->selectedItems().at( 0 )->text( 3 ) == tr( "Yes" );
    bool checked = selected->text( 3 ) == tr( "Yes" );

    QMenu myMenu( this );

    QAction changeSplitAct( tr( "Split Games When Writing to This Partition"), &myMenu );
    changeSplitAct.setCheckable( true );
    changeSplitAct.setChecked( checked );

    QMenu fsMenu( tr( "Filesystem"), &myMenu );
    QAction wbfsFsAct( "WBFS", &fsMenu );
    QAction fatFsAct( "FAT", &fsMenu );
    QAction sneekFsAct( "SNEEK", &fsMenu );
    QAction ntfsFsAct( "NTFS", &fsMenu );
    QAction extFsAct( "EXT", &fsMenu );
    QAction hpfsFsAct( "HPFS", &fsMenu );

    myMenu.addAction( &changeSplitAct );
    fsMenu.addAction( &wbfsFsAct );
    fsMenu.addAction( &fatFsAct );
    fsMenu.addAction( &sneekFsAct );
    fsMenu.addAction( &ntfsFsAct );
    fsMenu.addAction( &extFsAct );
    fsMenu.addAction( &hpfsFsAct );

    myMenu.addMenu( &fsMenu );

    QAction* selectedAct = myMenu.exec( globalPos );
    if( selectedAct )
    {
		// something was chosen, do stuff
		if( selectedAct == &changeSplitAct )//split games
		{
			foreach( QTreeWidgetItem *item, ui->treeWidget->selectedItems() )
			{
				item->setText( 3, checked ? QString() : tr( "Yes" ) );
			}
		}
		else if( selectedAct == &wbfsFsAct || selectedAct == &fatFsAct || selectedAct == &ntfsFsAct //change FS type
				 || selectedAct == &extFsAct || selectedAct == &hpfsFsAct || selectedAct == &sneekFsAct )
		{
			foreach( QTreeWidgetItem *item, ui->treeWidget->selectedItems() )
			{
				item->setText( 5, selectedAct->text() );
			}
		}
    }
    else
    {
		// nothing was chosen
    }
}



