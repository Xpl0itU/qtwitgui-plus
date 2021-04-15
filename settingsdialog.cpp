
#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "includes.h"
#include "tools.h"
#include "wiitdb.h"

extern QString rootAskStr;
extern QString rootWrongStr;
extern QString rootFailStr;
/*
 page order

 paths
 root previleges
 wit/wwt
 partition windows
 game browsers
 coverflow
 wiitdb

 */

SettingsDialog::SettingsDialog( QWidget *parent ) : QDialog( parent ), ui( new Ui::SettingsDialog )
{
    ui->setupUi( this );
    setWindowTitle( tr( "Settings" ) );
#ifdef Q_WS_WIN//remove root settings for windows
    QWidget *p = ui->Root;
    ui->stackedWidget->removeWidget( p );
    delete( p );

    QListWidgetItem *q = ui->listWidget->takeItem( 1 );
    delete( q );
#endif
    ResizeButtons();

    QSettings s( settingsPath, QSettings::IniFormat );
    s.beginGroup( "paths" );
    ui->lineEdit_wit->setText( s.value( "wit" ,"" ).toString() );
    ui->lineEdit_wwt->setText( s.value( "wwt" ,"" ).toString() );
    ui->lineEdit_coverPath->setText( s.value( "covers" ,"" ).toString() );
    ui->lineEdit_wiitdbPath->setText( s.value( "wiitdb" ,"" ).toString() );
    ui->lineEdit_titlesTxt->setText( s.value( "titlesTxt" ,"" ).toString() );
    s.endGroup();
#ifndef Q_WS_WIN
    s.beginGroup( "root" );
    ui->checkBox_runasRoot->setChecked( s.value( "enabled", false ).toBool() );
    ui->groupBox_rootMessages->setEnabled( ui->checkBox_runasRoot->isChecked() );
    ui->lineEdit_rootReqStr->setText( s.value( "requestString",
#ifdef Q_WS_MAC
                                               "Password:"
#else
                                               "[sudo] password for"
#endif

                                               ).toString() );
    ui->lineEdit_rootWrongStr->setText( s.value( "wrongPwString", "Sorry, try again." ).toString() );
    ui->lineEdit_rootFail->setText( s.value( "failString", "sudo: 3 incorrect password attempts" ).toString() );
    s.endGroup();
#endif
    s.beginGroup( "wit_wwt" );
    ui->checkBox_ignoreSvn->setChecked( s.value( "ignoreSvn" ).toBool() );
    ui->comboBox_key->setCurrentIndex( s.value( "encKey", 0 ).toInt() );
    ui->spinBox_wiimms_recurse->setValue( s.value( "rdepth", 10 ).toInt() );
    s.endGroup();

    s.beginGroup( "log" );
    ui->label_logBG->setText( s.value( "bgColor", "#ffffff" ).toString() );
    ui->label_logTxt->setText( s.value( "txtColor", "#000000" ).toString() );
    ui->label_logWrn->setText( s.value( "wrnColor", "#0000ff" ).toString() );
    ui->label_logCrt->setText( s.value( "crtColor", "#ff0000" ).toString() );
    s.endGroup();

    int size = s.beginReadArray( "ignoreFolders" );
    for( int i = 0; i < size; i++ )
    {
	 s.setArrayIndex( i );
	 ui->listWidget_ignore->insertItem( ui->listWidget_ignore->count(), s.value( "path" ).toString() );
    }
    s.endArray();

    ui->checkBox_ignoreFst->setChecked( s.value( "ignoreFst", false ).toBool() );

    QFont monoFont;
#ifdef Q_WS_WIN
    monoFont = QFont( "Courier New", QApplication::font().pointSize() );
#else
    monoFont = QFont( "Courier New", QApplication::font().pointSize() - 1 );
#endif
    ui->textEdit->setFont( monoFont );


    ResetLogPage();
    ui->label_logBG->setPalette( QPalette( QColor( ui->label_logBG->text() ) ) );
    ui->label_logBG->setAutoFillBackground( true );

    ui->label_logTxt->setPalette( QPalette( QColor( ui->label_logTxt->text() ) ) );
    ui->label_logTxt->setAutoFillBackground( true );

    ui->label_logWrn->setPalette( QPalette( QColor( ui->label_logWrn->text() ) ) );
    ui->label_logWrn->setAutoFillBackground( true );

    ui->label_logCrt->setPalette( QPalette( QColor( ui->label_logCrt->text() ) ) );
    ui->label_logCrt->setAutoFillBackground( true );
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::on_pushButton_ok_clicked()
{
    //save settings
    QSettings s( settingsPath, QSettings::IniFormat );
    s.beginGroup( "paths" );
    QString thisProgram = QCoreApplication::instance()->arguments().at( 0 );//avoid an endless loop if this program is used as the path for a process
    if( ui->lineEdit_wit->text() != thisProgram )
	s.setValue( "wit", ui->lineEdit_wit->text() );
    if( ui->lineEdit_wwt->text() != thisProgram )
	s.setValue( "wwt", ui->lineEdit_wwt->text() );
    s.setValue( "covers", ui->lineEdit_coverPath->text() );
    s.setValue( "wiitdb", ui->lineEdit_wiitdbPath->text() );
    s.setValue( "titlesTxt", ui->lineEdit_titlesTxt->text() );
    s.endGroup();
#ifndef Q_WS_WIN
    s.beginGroup( "root" );
    s.setValue( "enabled", ui->checkBox_runasRoot->isChecked() );
    s.setValue( "requestString", ui->lineEdit_rootReqStr->text() );
    s.setValue( "wrongPwString", ui->lineEdit_rootWrongStr->text() );
    s.setValue( "failString", ui->lineEdit_rootFail->text() );
    rootAskStr = ui->lineEdit_rootReqStr->text();
    rootWrongStr = ui->lineEdit_rootWrongStr->text();
    rootFailStr = ui->lineEdit_rootFail->text();
    s.endGroup();
#endif
    s.beginGroup( "wit_wwt" );
    s.setValue( "ignoreSvn", ui->checkBox_ignoreSvn->isChecked() );
    s.setValue( "encKey", ui->comboBox_key->currentIndex() );
    s.setValue( "rdepth", ui->spinBox_wiimms_recurse->value() );
    s.endGroup();

    s.beginGroup( "log" );
    s.setValue( "bgColor", ui->label_logBG->text() );
    s.setValue( "txtColor", ui->label_logTxt->text() );
    s.setValue( "wrnColor", ui->label_logWrn->text() );
    s.setValue( "crtColor", ui->label_logCrt->text() );
    s.endGroup();

    int size = ui->listWidget_ignore->count();
    s.beginWriteArray("ignoreFolders");
    for( int i = 0; i < size; i++ )
    {
	QListWidgetItem * item = ui->listWidget_ignore->takeItem( 0 );
	s.setArrayIndex( i );
	s.setValue("path", item->text() );
	delete item;
    }
    s.endArray();
    s.setValue( "ignoreFst", ui->checkBox_ignoreFst->isChecked() );

    s.sync();

    QDialog::accept();
}

void SettingsDialog::on_pushButton_wit_clicked()
{
    QString wit = QFileDialog::getOpenFileName( this, tr( "Where is wit?" ) );
    if( wit.isEmpty() )
	return;
    ui->lineEdit_wit->setText( wit );
}

void SettingsDialog::on_pushButton_wwt_clicked()
{
    QString wwt = QFileDialog::getOpenFileName( this, tr( "Where is wwt?" ) );
    if( wwt.isEmpty() )
	return;
    ui->lineEdit_wwt->setText( wwt );
}
void SettingsDialog::on_pushButton_wiitdbPath_clicked()
{
    QString p = QFileDialog::getOpenFileName( this, tr( "Where is wiitdb.zip / .xml?" ) );
    if( p.isEmpty() )
    {
        wiiTDB->LoadFile( ":/wiitdb.xml" );
	return;
    }
    ui->lineEdit_wiitdbPath->setText( p );

    if( !wiiTDB->LoadFile( p ) )//try to load theirs.  if not, fall back to the one we have compiled in the program
    {
	ui->lineEdit_wiitdbPath->clear();
        wiiTDB->LoadFile( ":/wiitdb.xml" );
    }
}

//get the width of the widest text in the "paths" buttons and resize all buttons to match that one
void SettingsDialog::ResizeButtons()
{
    QFontMetrics fm( fontMetrics() );
    int max = fm.width( ui->pushButton_coverPath->text() );
    max = MAX( max, fm.width( ui->pushButton_titlesPath->text() ) );
    max = MAX( max, fm.width( ui->pushButton_wiitdbPath->text() ) );
    max = MAX( max, fm.width( ui->pushButton_wit->text() ) );
    max = MAX( max, fm.width( ui->pushButton_wwt->text() ) );

    max += 30;
    ui->pushButton_coverPath->setMinimumWidth( max );
    ui->pushButton_titlesPath->setMinimumWidth( max );
    ui->pushButton_wiitdbPath->setMinimumWidth( max );
    ui->pushButton_wit->setMinimumWidth( max );
    ui->pushButton_wwt->setMinimumWidth( max );
}

void SettingsDialog::on_pushButton_titlesPath_clicked()
{
    QString p = QFileDialog::getOpenFileName( this, tr( "Where is titles.txt?" ) );
    ui->lineEdit_titlesTxt->setText( p );
}

void SettingsDialog::on_pushButton_coverPath_clicked()
{
    QString p = QFileDialog::getExistingDirectory( this, tr( "Select a Base Path for Covers" ) );
    ui->lineEdit_coverPath->setText( p );
}

//add item to the ignore path list
void SettingsDialog::on_pushButton_ignore_add_clicked()
{
    //if there is text entered in the lineEdit, use that text, otherwise open a window to allow searching for a path
    QString p = ui->lineEdit_ignore_entry->text();

    if( p.isEmpty() )
	p = QFileDialog::getExistingDirectory( this, tr( "Select a folder to ignore" ) );
    else
	ui->lineEdit_ignore_entry->clear();

    if( !p.isEmpty() && !ui->listWidget_ignore->findItems( p, Qt::MatchExactly ).count() )
	ui->listWidget_ignore->insertItem( ui->listWidget_ignore->count(), p );
}

//remove items from the ignore path list
void SettingsDialog::on_pushButton_ignore_minus_clicked()
{
    QList<QListWidgetItem *> items = ui->listWidget_ignore->selectedItems();
    if( !items.size() )
	return;

    foreach( QListWidgetItem *item, items )
    {
	ui->listWidget_ignore->removeItemWidget( item );
	delete item;
    }
}

void SettingsDialog::ResetLogPage()
{
    QPalette p = ui->textEdit->palette();
    p.setColor( QPalette::Base, QColor( ui->label_logBG->text() ) );
    p.setColor( QPalette::Text, QColor( ui->label_logTxt->text() ) );
    ui->textEdit->setPalette( p );

    ui->textEdit->clear();
    QString htmlString = QString( "%1<br><b><text style=\"color:" + ui->label_logWrn->text() + "\"> %2 </text></b><br>"\
				  "<b><text style=\"color:" + ui->label_logCrt->text() + "\"> %3 </text></b><br>" ).\
			 arg( tr( "This is text" ) ).\
			 arg( tr( "This is example warning text" ) ).\
			 arg( tr( "This is example critical text" ) );

    ui->textEdit->insertHtml( htmlString );
}

//search for log colors
//BG
void SettingsDialog::on_pushButton_logBg_clicked()
{
    QColor color = QColorDialog::getColor( QColor( ui->label_logBG->text() ), this );

    if( color.isValid() )
    {
	ui->label_logBG->setText( color.name() );
	ui->label_logBG->setPalette( QPalette( color ) );
	ui->label_logBG->setAutoFillBackground( true );
    }
    ResetLogPage();
}
//text
void SettingsDialog::on_pushButton_logTxt_clicked()
{
    QColor color = QColorDialog::getColor( QColor( ui->label_logTxt->text() ), this );

    if( color.isValid() )
    {
	ui->label_logTxt->setText( color.name() );
	ui->label_logTxt->setPalette( QPalette( color ) );
	ui->label_logTxt->setAutoFillBackground( true );
    }
    ResetLogPage();
}
//warning
void SettingsDialog::on_pushButton_logWrn_clicked()
{
    QColor color = QColorDialog::getColor( QColor( ui->label_logWrn->text() ), this );

    if( color.isValid() )
    {
	ui->label_logWrn->setText( color.name() );
	ui->label_logWrn->setPalette( QPalette( color ) );
	ui->label_logWrn->setAutoFillBackground( true );
    }
    ResetLogPage();
}

void SettingsDialog::on_pushButton_logCrt_clicked()
{
    QColor color = QColorDialog::getColor( QColor( ui->label_logCrt->text() ), this );

    if( color.isValid() )
    {
	ui->label_logCrt->setText( color.name() );
	ui->label_logCrt->setPalette( QPalette( color ) );
	ui->label_logCrt->setAutoFillBackground( true );
    }
    ResetLogPage();
}
