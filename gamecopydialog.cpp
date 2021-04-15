#include "gamecopydialog.h"
#include "ui_gamecopydialog.h"
#include "includes.h"
#include "filefolderdialog.h"
#include "tools.h"
#include "withandler.h"

GameCopyDialog::GameCopyDialog( QWidget *parent, const QList<QTreeWidgetItem *> &partitions, const QStringList &games,
								const QString &current, const QStringList &patchArgs ) : QDialog(parent), ui(new Ui::GameCopyDialog)
{
    //qDebug() << "GameCopyDialog" << games;
    ui->setupUi(this);
    setWindowTitle( tr( "Copy & Convert Parameters" ) );
    currentPart = current;
    gameList = games;
    singleGame = games.size() == 1;

    currentPathIswbfs = false;
    //add partitions to the combobox
    int size = partitions.size();
    for( int i = 0; i < size; i++ )
    {
	if( current == partitions.at( i )->text( 0 ) && partitions.at( i )->text( 5 ) == "WBFS" )//the current game is on a WBFS partition, dont add this partition to the list
	{
	    currentPathIswbfs = true;
	    continue;
	}
	if( partitions.at( i )->text( 5 ) == "WBFS" && partitions.at( i )->text( 6 ) == "busy" )//the WBFS partition is being written to by some other process, dont allow it to be written to right now
	{
	    continue;
	}
	ui->comboBox_dest->addItem( partitions.at( i )->text( 0 ) );
    }

    if( currentPathIswbfs )//if the current games are on a wbfs partition, using the filename as part of the output escape sequence will cause it to use the partition path as the filename
    {			    //so just remove these items from the choices
	while( ui->comboBox_esc->count() > 8 )
	    ui->comboBox_esc->removeItem( 8 );
    }

    partList = partitions;
    patchArgList = patchArgs;

    LoadSettings();

    UpdateGuiWithCurrentSettings();
}

GameCopyDialog::~GameCopyDialog()
{
    delete ui;
}

QStringList GameCopyDialog::WitCopyCommand( QWidget *parent, const QString &currentPartition, const QList<QTreeWidgetItem *> &partitions,
											const QStringList &games, const QStringList &patchArgs )
{
    GameCopyDialog d( parent, partitions, games, currentPartition, patchArgs );
    if( d.exec() )
	return d.ret;

    return QStringList();
}


//different partition selected
void GameCopyDialog::on_comboBox_dest_currentIndexChanged( const QString &part )
{
    Q_UNUSED( part );
    //bool other = part == tr( "Other" );
    //ui->lineEdit_destOther->setEnabled( other );
    //ui->pushButton_destOther->setEnabled( other );

    UpdateGuiWithCurrentSettings();
}

//update gui elements based on settings
void GameCopyDialog::UpdateGuiWithCurrentSettings( bool skipPartitionFlags )
{
    //qDebug() << "GameCopyDialog::UpdateGuiWithCurrentSettings" << skipPartitionFlags;
    if( ui->groupBox_partition->isChecked() || !singleGame )//use an existing partition
    {
	//qDebug() << "ui->groupBox_partition->isChecked() || !singleGame";

	int size = partList.size();
	QTreeWidgetItem * partition;
	bool found = false;
	bool other = !ui->comboBox_dest->currentIndex();
	bool wbfs = false;
	for( int i = 0; i < size; i++ )
	{
	    if( ui->comboBox_dest->currentText() == partList.at( i )->text( 0 ) )
	    {
		found = true;
		partition = partList.at( i );
		break;
	    }
	}
	if( !found && !other )
	{
	    qDebug() << "GameCopyDialog::UpdateGuiWithCurrentSettings() !found";
	    return;
	}
	if( !other && !skipPartitionFlags )//try to be smart and guess what the output container will be
	{
	    wbfs = partition->text( 5 ) == "WBFS";
	    ui->checkBox_partitionisWBFS->setChecked( wbfs );
	    if( wbfs )
		ui->comboBox_container->setCurrentIndex( 0 );

	    ui->checkBox_split->setChecked( partition->text( 3 ) == tr( "Yes" ) );

	    if( partition->text( 5 ) == "SNEEK" )
	    {
		ui->comboBox_container->setCurrentIndex( 2 );
		ui->checkBox_fst_sneek->setChecked( true );
	    }
	    else if( partition->text( 0 ).endsWith( "/wbfs" ) )
	    {
		ui->comboBox_container->setCurrentIndex( 0 );
	    }
	    else if( partition->text( 0 ).endsWith( "/iso" ) )
	    {
		ui->comboBox_container->setCurrentIndex( 1 );
	    }
	}
	//ui->checkBox_partitionisWBFS->setEnabled( other );
	wbfs = ui->checkBox_partitionisWBFS->isChecked();

	ui->stackedWidget->setEnabled( !wbfs );
	ui->comboBox_esc->setEnabled( !wbfs );
	ui->label_naming->setEnabled( !wbfs );
	ui->checkBox_split->setEnabled( !wbfs );
	ui->comboBox_container->setEnabled( !wbfs );
	ui->label_container->setEnabled( !wbfs );

	ui->pushButton_destOther->setEnabled( false );
	ui->lineEdit_destOther->setEnabled( false );

	//set the example text
	QString text;
	if( wbfs )
	    text = "";
	else
	{
	    QString ID = "RSPE01";
	    QString fileName = "baseFilename";
	    QString gameTitle = "Wii Sports";
	    QString ext = ui->comboBox_container->currentText();
	    if( ext == "fst" )
		ext = " " + tr( "[Directory]" );
	    else
		ext.prepend( "." );

	    text = ui->comboBox_dest->currentText();
	    text += "/";

	    switch( ui->comboBox_esc->currentIndex() )
	    {
	    case 1:
	    default:
		text += gameTitle + ext;
		break;
	    case 8:
		text += fileName + ext;
		break;
	    case 0:
		text += ID + ext;
		break;
	    case 10:
		text += fileName + "_[" + ID + "]" + ext;
		break;
	    case 9:
		text += fileName + " [" + ID + "]" + ext;
		break;
	    case 11:
		text += ID + "_" + fileName + ext;
		break;
	    case 13:
		text += fileName + "_[" + ID + "]/" + ID + ext;
		break;
	    case 12:
		text += fileName + " [" + ID + "]/" + ID + ext;
		break;
	    case 14:
		text += ID + "_" + fileName + "/" + ID + ext;
		break;
	    case 3:
		text += gameTitle + "_[" + ID + "]" + ext;
		break;
	    case 2:
		text += gameTitle + " [" + ID + "]" + ext;
		break;
	    case 4:
		text += ID + "_" + gameTitle + ext;
		break;
	    case 6:
		text += gameTitle + "_[" + ID + "]/" + ID + ext;
		break;
	    case 5:
		text += gameTitle + " [" + ID + "]/" + ID + ext;
		break;
	    case 7:
		text += ID + "_" + gameTitle + "/" + ID + ext;
		break;
	    }
	    /*
    1"/GameName.ext"
    8"/FileName.ext"
    0"/ID.ext"
    10"/FileName_[ID].ext"
    9"/FileName [ID].ext"
    11"/ID_FileName.ext"
    13"/FileName_[ID]/ID.ext"
    12"/FileName [ID]/ID.ext"
    14"/ID_FileName/ID.ext"
    3"/GameName_[ID].ext"
    2"/GameName [ID].ext"
    4"/ID_GameName.ext"
    6"/GameName_[ID]/ID.ext"
    5"/GameName [ID]/ID.ext"
    7"/ID_GameName/ID.ext"
    */
	}

	ui->label_example->setText( text );
    }
    else//specify a filename
    {
	ui->pushButton_destOther->setEnabled( true );
	ui->lineEdit_destOther->setEnabled( true );
	ui->stackedWidget->setEnabled( true );
	ui->checkBox_split->setEnabled( true );
	ui->comboBox_container->setEnabled( true );
	ui->label_container->setEnabled( true );

	ui->label_example->setText( ui->lineEdit_destOther->text() );
    }
    bool canHazPsel = !( ui->comboBox_container->currentIndex() == 2 && ui->checkBox_fst_sneek->isChecked() );
    ui->groupBox_partitions->setEnabled( canHazPsel );
}

//wbfs checkbox ticked
void GameCopyDialog::on_checkBox_partitionisWBFS_clicked( bool )
{
    //qDebug() << "GameCopyDialog::on_checkBox_partitionisWBFS_clicked";
    UpdateGuiWithCurrentSettings( true );
}

void GameCopyDialog::on_comboBox_esc_currentIndexChanged( const QString & )
{
    UpdateGuiWithCurrentSettings( true );
}

//ok button clicked
void GameCopyDialog::on_buttonBox_accepted()
{
    //qDebug() << "GameCopyDialog::on_buttonBox_accepted()";
    SaveSettings();
    if( ui->comboBox_dest->currentText() == tr( "/<Enter a Directory>" ) &&
        ( ui->groupBox_partition->isChecked() || !singleGame ) )//they didnt enter a destination, and it is left at the default one
        return;

    bool wbfs = ui->checkBox_partitionisWBFS->isChecked();
    if( wbfs )//writing to a WBFS partition
    {
	QStringList args;
	args << "wwt" << "ADD" << "--part";
	args << ui->comboBox_dest->currentText();

	int size = gameList.size();// add games to the args
	for( int i = 0; i < size; i++ )
	    args << gameList.at( i );

	if( ui->checkBox_overwrite->isChecked() )
	    args << "--overwrite";

	QString psel = "--psel=";
	bool p = false;
	if( !ui->checkBox_part_update->isChecked() )
	{
	    psel += "-UPDATE";
	    p = true;
	}
	if( !ui->checkBox_part_data->isChecked() )
	{
	    if( p )psel += ",";
	    psel += "-DATA";
	    p = true;
	}
	if( !ui->checkBox_part_installer->isChecked() )
	{
	    if( p )psel += ",";
	    psel += "-CHANNEL";
	    p = true;
	}
	if( !ui->checkBox_part_id->isChecked() )
	{
	    if( p )psel += ",";
	    psel += "-ID";
	    p = true;
	}

	if( p )
	    args << psel;

	args << "--progress";

	foreach( QString pArg, patchArgList )
	    args << pArg;

	ret = args;
	return;
    }
    QStringList args;
    QString dirtyPart = ( ui->groupBox_partition->isChecked() || !singleGame ) ? ui->comboBox_dest->currentText() : "_NO_PART_";
    args << "wit" << dirtyPart << "COPY" << "--DEST";//add the actual partition name as an argument here just to be able to refresh later, this arg will be removed before wit gets them
    if( ui->groupBox_partition->isChecked() || !singleGame )//use an existing partition
    {
	QString text = ui->comboBox_dest->currentText() + "/";
	QString ext = ui->comboBox_container->currentIndex() == 2 ? "" : ".%E";//no extension for --fst
	switch( ui->comboBox_esc->currentIndex() )
	{
	case 1:
	default:
	    text += "%T" + ext;
	    break;
	case 8:
	    text += "%F" + ext;
	    break;
	case 0:
            text += "%I" + ext;
	    break;
	case 10:
	    text += "%F_[%I]" + ext;
	    break;
	case 9:
	    text += "%F [%I]" + ext;
	    break;
	case 11:
	    text += "%I_%F" + ext;
	    break;
	case 13:
            text += "%F_[%I]/%I" + ext;
	    break;
	case 12:
            text += "%F [%I]/%I" + ext;
	    break;
	case 14:
            text += "%I_%F/%I" + ext;
	    break;
	case 3:
	    text += "%T_[%I]" + ext;
	    break;
	case 2:
	    text += "%X";
	    break;
	case 4:
	    text += "%I_%T" + ext;
	    break;
	case 6:
            text += "%N_[%I]/%I" + ext;
	    break;
	case 5:
            text += "%Y/%I" + ext;
	    break;
	case 7:
            text += "%I_%T/%I" + ext;
	    break;
	}
	args << text;
	/*
1"/GameName.ext"
8"/FileName.ext"
0"/ID.ext"
10"/FileName_[ID].ext"
9"/FileName [ID].ext"
11"/ID_FileName.ext"
13"/FileName_[ID]/ID.ext"
12"/FileName [ID]/ID.ext"
14"/ID_FileName/ID.ext"
3"/GameName_[ID].ext"
2"/GameName [ID].ext"
4"/ID_GameName.ext"
6"/GameName_[ID]/ID.ext"
5"/GameName [ID]/ID.ext"
7"/ID_GameName/ID.ext"
*/
    }
    else //single game & specific filename given
	args << ui->lineEdit_destOther->text();

    args << QString( "--" + ui->comboBox_container->currentText() );//container

    int size = gameList.size();// add games to the args
    for( int i = 0; i < size; i++ )
	args << gameList.at( i );

    if( ui->checkBox_split->isChecked() )
	args << "--split";

    if( ui->checkBox_overwrite->isChecked() )
	args << "--overwrite";

    bool sneek = false;
    switch( ui->comboBox_container->currentIndex() )//different args depending on the output container
    {
    case 1://iso
	if( ui->checkBox_iso_trunc->isChecked() )
	    args << "--trunc";
	break;
    case 2://sneek
	if( ui->checkBox_fst_sneek->isChecked() )
	{
	    args << "--sneek";
	    sneek = true;
	}
	break;
    case 3://wia
	{
	    if( ui->groupBox_wia_presets->isChecked() )//wia preset
	    {
		switch( ui->comboBox_wia_preset->currentIndex() )
		{
		case 0://fast
		    args << "--compression=FAST";
		    break;
		case 2:
		    args << "--compression=BEST";
		    break;
		default:
		case 1://good - default, no need for any args
		    break;
		}
	    }
	    else//wia advanced mode
	    {
		QString wiaArg = "--compression=" + ui->comboBox_wia_method->currentText();
		if( ui->comboBox_wia_method->currentIndex() > 1 && ui->groupBox_wia_level->isChecked() )//"none" and "purge" not selected and level is specified
		{
		    QTextStream( &wiaArg ) << "." << ui->comboBox_wia_level->currentIndex();
		}
		if( ui->groupBox_wia_chunk->isChecked() )//specified a chunk factor
		    QTextStream( &wiaArg ) << "@" << ui->spinBox_wia_chunk->value();

		args << wiaArg;
	    }
	}
	break;
    default:
    case 0://wbfs
	break;
    }
    if( !sneek )
    {
	QString psel = "--psel=";
	bool p = false;
	if( !ui->checkBox_part_update->isChecked() )
	{
	    psel += "-UPDATE";
	    p = true;
	}
	if( !ui->checkBox_part_data->isChecked() )
	{
	    if( p )psel += ",";
	    psel += "-DATA";
	    p = true;
	}
	if( !ui->checkBox_part_installer->isChecked() )
	{
	    if( p )psel += ",";
	    psel += "-CHANNEL";
	    p = true;
	}
	if( !ui->checkBox_part_id->isChecked() )
	{
	    if( p )psel += ",";
	    psel += "-ID";
	    p = true;
	}

	if( p )
	    args << psel;
    }

    args << "--progress";

    QString titlesTxtPath = WitHandler::GetTitlesTxtPath();
    if( !titlesTxtPath.isEmpty() )
	args << "--titles=" + titlesTxtPath;

    foreach( QString pArg, patchArgList )
	args << pArg;

    ret = args;

    //qDebug() << args;
    return;

}

void GameCopyDialog::on_checkBox_file_clicked(bool checked)
{
    //qDebug() << "GameCopyDialog::on_checkBox_file_toggled";
    ui->groupBox_partition->setChecked( !checked );
    UpdateGuiWithCurrentSettings();
}

void GameCopyDialog::on_groupBox_partition_clicked( bool checked )
{
    //qDebug() << "GameCopyDialog::on_groupBox_partition_toggled";
    ui->checkBox_file->setChecked( !checked );
    UpdateGuiWithCurrentSettings();
}

//search button clicked
void GameCopyDialog::on_pushButton_destOther_clicked()
{
    FileFolderDialog dialog( this );
    dialog.setNameFilter( "*.iso *.wbfs *.ciso *.wdf *.wia" );

#if defined (Q_WS_MAC) || defined (Q_WS_WIN)
    dialog.setOption( QFileDialog::DontUseNativeDialog );
#endif

    if ( !dialog.exec() || !dialog.selectedFiles().size() )
	return;

    QString path = dialog.selectedFiles().at( 0 );

    ui->lineEdit_destOther->setText( path );

    QString ext = path;
    ext.remove( 0, ext.lastIndexOf( "." ) + 1 );
    //qDebug() << "ext:" << ext;
    if( ext.length() == 3 || ext.length() == 4 )
    {
	int idx = ui->comboBox_container->findText( ext );
	if( idx >= 0 )
	    ui->comboBox_container->setCurrentIndex( idx );
    }
    else
    {
	ui->comboBox_container->setCurrentIndex( 2 );
    }
}

void GameCopyDialog::on_lineEdit_destOther_textChanged( const QString & )
{
    UpdateGuiWithCurrentSettings();
}

void GameCopyDialog::on_comboBox_container_currentIndexChanged(int index)
{
    //qDebug() << "GameCopyDialog::on_comboBox_container_currentIndexChanged:" << index;
    switch( index )
    {
    case 1:
    case 2:
    case 3:
	ui->stackedWidget->setCurrentIndex( index );
	break;
    default://no special options for these containers
	ui->stackedWidget->setCurrentIndex( 0 );
	break;
    }
    /*
     wbfs = 0
     iso
     fst
     wia
     wdf
     ciso = 5
    */
    UpdateGuiWithCurrentSettings( true );
}


//wia option logic
void GameCopyDialog::on_groupBox_wia_presets_toggled( bool checked )
{
    ui->groupBox_wia_advanced->setChecked( !checked );
}

void GameCopyDialog::on_groupBox_wia_advanced_toggled( bool checked )
{
    ui->groupBox_wia_presets->setChecked( !checked );
}

void GameCopyDialog::on_comboBox_wia_method_currentIndexChanged(int index)
{
    switch( index )
    {
    case 0:
    case 1:
	ui->groupBox_wia_level->setEnabled( false );
	break;
    default:
	ui->groupBox_wia_level->setEnabled( true );
	break;

    }
}

//fst option logic
void GameCopyDialog::on_checkBox_fst_sneek_toggled(bool)
{
    UpdateGuiWithCurrentSettings( true );
}

//save and load the window state from last time it was used
void GameCopyDialog::SaveSettings()
{
    QSettings s( settingsPath, QSettings::IniFormat );
    s.beginGroup( "copydialog" );
    s.setValue( "partition", ui->comboBox_dest->currentText() );
    s.setValue( "escape", ui->comboBox_esc->currentIndex() );
    s.setValue( "split", ui->checkBox_split->isChecked() );
    s.setValue( "overwrite", ui->checkBox_overwrite->isChecked() );
    s.setValue( "part_update", ui->checkBox_part_update->isChecked() );
    s.setValue( "part_game", ui->checkBox_part_data->isChecked() );
    s.setValue( "part_installer", ui->checkBox_part_installer->isChecked() );
    s.setValue( "part_vc", ui->checkBox_part_id->isChecked() );

    s.setValue( "container", ui->comboBox_container->currentIndex() );
    if( gameList.size() == 1 )
	s.setValue( "exactFilename", ui->checkBox_file->isChecked() );
    switch( ui->comboBox_container->currentIndex() )
    {
    case 1:
	s.setValue( "iso_trunc", ui->checkBox_iso_trunc->isChecked() );
	break;
    case 2:
	 s.setValue( "fst_sneek", ui->checkBox_fst_sneek->isChecked() );
	 break;
    case 3:
	 s.setValue( "wia_use_presets", ui->groupBox_wia_presets->isChecked() );
	 s.setValue( "wia_preset", ui->comboBox_wia_preset->currentIndex() );
	 s.setValue( "wia_method", ui->comboBox_wia_method->currentIndex() );
	 s.setValue( "wia_use_level", ui->groupBox_wia_level->isChecked() );
	 s.setValue( "wia_level", ui->comboBox_wia_level->currentIndex() );
	 s.setValue( "wia_use_chunk", ui->groupBox_wia_chunk->isChecked() );
	 s.setValue( "wia_chunk", ui->spinBox_wia_chunk->value() );
	 break;
    default:
	 break;
    }
    s.endGroup();
    s.sync();
}

void GameCopyDialog::LoadSettings()
{
    QSettings s( settingsPath, QSettings::IniFormat );
    s.beginGroup( "copydialog" );
    QString lastPart = s.value( "partition", tr("/<Enter a Directory>") ).toString();
    int p = ui->comboBox_dest->findText( lastPart );
    //qDebug() << lastPart << p;
    if( p >= 0 )
	ui->comboBox_dest->setCurrentIndex( p );

    ui->comboBox_esc->setCurrentIndex( s.value( "escape", 0 ).toInt() );
    ui->checkBox_split->setChecked( s.value( "split", false ).toBool() );
    ui->checkBox_overwrite->setChecked( s.value( "overwrite", false ).toBool() );
    ui->checkBox_part_update->setChecked( s.value( "part_update", false ).toBool() );
    ui->checkBox_part_data->setChecked( s.value( "part_game", true ).toBool() );
    ui->checkBox_part_installer->setChecked( s.value( "part_installer", false ).toBool() );
    ui->checkBox_part_id->setChecked( s.value( "part_vc", true ).toBool() );

    ui->comboBox_container->setCurrentIndex( s.value( "container", 0 ).toInt() );

    if( gameList.size() == 1 )
    {
	bool f = s.value( "exactFilename", false ).toBool();
	ui->checkBox_file->setChecked( f );
	ui->groupBox_partition->setChecked( !f );
    }
    else
    {
	//dont allow entering an exact output name for more than 1 game
	ui->checkBox_file->setVisible( false );
	ui->pushButton_destOther->setVisible( false );
	ui->lineEdit_destOther->setVisible( false );
	ui->groupBox_partition->setCheckable( false );

    }
    ui->checkBox_iso_trunc->setChecked( s.value( "iso_trunc", false ).toBool() );
    ui->checkBox_fst_sneek->setChecked( s.value( "fst_sneek", false ).toBool() );

    ui->groupBox_wia_presets->setChecked( s.value( "wia_use_presets", false ).toBool() );
    ui->comboBox_wia_preset->setCurrentIndex( s.value( "wia_preset", 0 ).toInt() );
    ui->comboBox_wia_method->setCurrentIndex( s.value( "wia_method", 0 ).toInt() );
    ui->groupBox_wia_level->setChecked( s.value( "wia_use_level", false ).toBool() );
    ui->comboBox_wia_level->setCurrentIndex( s.value( "wia_level", 0 ).toInt() );
    ui->groupBox_wia_chunk->setChecked( s.value( "wia_use_chunk", false ).toBool() );
    ui->spinBox_wia_chunk->setValue( s.value( "wia_chunk", 20 ).toInt() );

    s.endGroup();
}
