#include "formatwindow.h"
#include "ui_formatwindow.h"
#include "passworddialog.h"
#include "tools.h"
#include "savedialog.h"
#include "fsinfo.h"

FormatWindow::FormatWindow(QWidget *parent) : QDialog(parent), ui(new Ui::FormatWindow)
{
    ui->setupUi(this);
    alreadyAskingForPassword = false;


    QFontMetrics fm( fontMetrics() );
    ui->treeWidget->header()->resizeSection( 0, fm.width( QString( 18, 'W' ) ) );//location
    ui->treeWidget->header()->resizeSection( 1, fm.width( "WWWWW" ) );//GiB
    ui->treeWidget->header()->resizeSection( 2, fm.width( "WWWWWWW" ) );//WBFS status

#ifndef Q_WS_WIN
    QSettings s( settingsPath, QSettings::IniFormat );
    bool root = s.value( "root/enabled" ).toBool();
    wwt.SetRunAsRoot( root );
    connect( &wwt, SIGNAL( RequestPassword() ), this, SLOT( NeedToAskForPassword() ) );
    connect( this, SIGNAL( UserEnteredPassword() ), &wwt, SLOT( PasswordIsEntered() ) );
#endif
	connect( &wwt, SIGNAL( SendStdOut( const QString & ) ), this, SLOT( FormatDone( const QString & ) ) );
	connect( &wwt, SIGNAL( SendPartitionList( const QStringList & ) ), this, SLOT( GetPartitionList( const QStringList & ) ) );
	connect( &wwt, SIGNAL( SendFatalErr( const QString &, int ) ), this, SLOT( HandleWiimmsErrors( const QString &, int ) ) );

    //get the initial HDD listing
    wwt.GetPartitions( true );
}

FormatWindow::~FormatWindow()
{
    wwt.Kill();
    wwt.Wait();
    delete ui;
}

//close the window
void FormatWindow::on_pushButton_done_clicked()
{
    QDialog::accept();
}

#ifndef Q_WS_WIN
//wwt has asked for a password
void FormatWindow::NeedToAskForPassword()
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

//ask the list of partitions from wwt
void FormatWindow::on_pushButton_refresh_clicked()
{
    ui->pushButton_format->setEnabled( false );
    wwt.GetPartitions( true );
}

//receive partitions from wwt
void FormatWindow::GetPartitionList( const QStringList &list )
{
    qDebug() << "FormatWindow::GetPartitionList";
    QList<QTreeWidgetItem* > oldItems = ui->treeWidget->invisibleRootItem()->takeChildren();
    while( !oldItems.isEmpty() )
    {
        QTreeWidgetItem* i = oldItems.takeFirst();
        delete i;
    }
    unsetCursor();
    foreach( QString part, list )
    {
        AddItemToTree( part );
    }
}

//get a line from wwt's output and figure out where it goes in the tree
void FormatWindow::AddItemToTree( const QString &part )
{
    QStringList sections = part.split( " ", QString::SkipEmptyParts );
    if( sections.size() != 5 )
    {
        qDebug() << "sections.size() != 5" << sections;
        return;
    }
    QString path = sections.at( 4 );
    QString sizeStr = sections.at( 3 );
    if( ( !path.startsWith( "/dev/s" )
#ifdef Q_WS_MAC
        && !path.startsWith( "/dev/rdisk" )
#endif
        ) || sizeStr == "0" )
        {
        qDebug() << "FormatWindow::AddItemToTree -> skipping" << path << sizeStr;
        return;
    }
    QString location;

    QTreeWidgetItem* parent = ui->treeWidget->invisibleRootItem();
    int size = ui->treeWidget->topLevelItemCount();
    for( int i = 0; i < size; i++ )
    {
        QString hdd = ui->treeWidget->topLevelItem( i )->text( 3 );
        if( path.startsWith( hdd ) )
        {
            parent = ui->treeWidget->topLevelItem( i );
            location = tr( "Partition %1" ).arg( ui->treeWidget->topLevelItem( i )->childCount() + 1 );
#ifdef Q_WS_WIN//add the drive letter in windows
            bool ok = false;
            QString letter = FsInfo::ToWinPath( path, &ok );
            if( ok && letter.endsWith( ":" ) )
                location += "\t[ " + letter + " ]";
#endif
        }
    }
    ui->pushButton_format->setEnabled( true );

    QTreeWidgetItem* item = new QTreeWidgetItem( parent );

    //not found, this must be a HDD and not a partition
    if( location.isEmpty() )
        location = tr( "Disc %1" ).arg( ui->treeWidget->topLevelItemCount() );

    item->setTextAlignment( 1, Qt::AlignRight | Qt::AlignVCenter );
    item->setTextAlignment( 2, Qt::AlignHCenter | Qt::AlignVCenter );

    item->setText( 0, location );
    if( sections.at( 1 ) == "WBFS" )
    {
        parent->setText( 2, "*" );
        item->setText( 2, "WBFS" );
    }
    item->setText( 1, SizeTextGiB( sizeStr ) );
    item->setText( 3, path );

}

//respond somehow to fatal errors
void FormatWindow::HandleWiimmsErrors( const QString &err, int id )
{
    //qDebug() << "wiimms error" << err << id;
    unsetCursor();
    ui->pushButton_format->setEnabled( true );
    ui->pushButton_refresh->setEnabled( true );
    ui->pushButton_done->setEnabled( true );
    switch( id )
    {
    case wwtFormat:
        {
            QMessageBox::critical( this, tr( "Formatting Error" ), err, QMessageBox::Ok, QMessageBox::Ok );
        }
        break;
    case wwtFind_long:
        {
            QMessageBox::critical( this, tr( "Error getting partition list" ), err, QMessageBox::Ok, QMessageBox::Ok );
        }
        break;
    default:
        break;
    }
}

//formatting is done, text is all the stdout from wwt
void FormatWindow::FormatDone( const QString &text )
{
    ui->pushButton_format->setEnabled( true );
    ui->pushButton_refresh->setEnabled( true );
    ui->pushButton_done->setEnabled( true );

    //just recycle the save dialog.  its good for the environment
    SaveDialog w( this, tr( "Partition Formatted" ), tr( "Here is the output from wwt" ), text, tr( "Ok" ), QString(), false );
    w.exec();

    QTimer::singleShot( 500, this, SLOT( on_pushButton_refresh_clicked() ) );
}

void FormatWindow::on_pushButton_format_clicked()
{
    int x = ui->spinBox_hss->value();
    if( ui->checkBox_hss->isChecked() && ( !x || ( ( x & ( x - 1 ) ) ) ) )//if the user typed in a bad value and clicked format before the 1337 spinbox had time to fix it for them
    {
        QMessageBox::warning( this, tr( "Bad value" ), tr( "The sector size should be a power of 2" ), QMessageBox::Ok );
        return;
    }
    QList< QTreeWidgetItem * > selected = ui->treeWidget->selectedItems();
    if( selected.size() != 1 )
        return;
    QTreeWidgetItem* item = selected.at( 0 );
    int button;
    bool recover = ui->checkBox_recover->isChecked();
    if( !item->parent() )//top level item, must be a physical drive
    {
        button = QMessageBox::question( this, tr( "Are you sure?" ), \
                                        tr( "You have chosen to format %1.  This is a physical device, not a partition.  If you aren\'t sure what you are doing, you probably want to format a partition.  "  \
                                            "If you continue, all data on this device will be destroyed, possibly irreparably.%2" ).arg( item->text( 3 ) )\
                                        .arg( recover ? tr( "<br><br>Formatting is set to run in recover mode." ) : "" ),\
                                        QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel );
    }
    else if( !item->parent()->parent() )//second level item, must be a partition
    {
        button = QMessageBox::question( this, tr( "Are you sure?" ), \
                                        tr( "You are about to format %1 (%2 GiB).  "  \
                                            "If you continue, all data on this partition will be destroyed, possibly irreparably.%3" ).arg( item->text( 3 ) ).arg( item->text( 1 ) )\
                                        .arg( recover ? tr( "<br><br>Formatting is set to run in recover mode." ) : "" ),\
                                        QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel );
    }
    else//what the fuck is this?  caused by a really broken partition or an error parsing the output text from wwt.
    {
        qDebug() << "z0mg.  something went wrong in the format window";
        return;
    }
    if( button != QMessageBox::Ok )
        return;

    QStringList args = QStringList() << "FORMAT" << item->text( 3 ) << "--force";
    if( ui->checkBox_recover->isChecked() )
        args << "--recover";

    if( ui->checkBox_hss->isChecked() && ui->spinBox_hss->value() != 512 )//defined hss value and not default
        args << QString( "--hss=%1").arg( ui->spinBox_hss->value() );

    ui->pushButton_format->setEnabled( false );
    ui->pushButton_refresh->setEnabled( false );
    ui->pushButton_done->setEnabled( false );
    wwt.RunJob( args, wwtFormat );
}

QSpinBoxPowerOf2::QSpinBoxPowerOf2( QWidget *parent ) : QSpinBox( parent )
{
    lastGoodValue = 512;
}

void QSpinBoxPowerOf2::stepBy( int steps )
{
    int v = value();
    if( v < 512 || ( ( v & ( v - 1 ) ) ) )//should only get here if the user typed in a bad value.  fix it for them
    {
        v = lastGoodValue;
        setValue( lastGoodValue );
    }
    else
        lastGoodValue = v;

    if( steps < 0 )
        QAbstractSpinBox::stepBy( -( v / 2 ) );
    else if( steps > 0 )
        QAbstractSpinBox::stepBy( v );
}
