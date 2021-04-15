#include "savedialog.h"
#include "ui_savedialog.h"
#include "tools.h"

SaveDialog::SaveDialog( QWidget *parent, const QString &boldText, const QString &lightText, const QString &details
			, const QString &saveText, const QString &cancelText, bool showCancel ) : QDialog( parent ), ui(new Ui::SaveDialog)
{
    ui->setupUi(this);
    setWindowTitle( PROGRAM_NAME );
    detailsVisible = false;

    smallSize = minimumSize();
    bigSize = QSize( smallSize.width(), smallSize.height() * 2 );
    QSettings s( settingsPath, QSettings::IniFormat );
    s.beginGroup( "saveDialog" );
    bool det = s.value( "details", false ).toBool();
    s.endGroup();
    ui->textEdit_details->setVisible( false );
    resize( smallSize );

    if( det )
	ToggleDetails();

    //ToggleDetails();
    if( !boldText.isEmpty() )
	ui->label_header->setText( boldText );

    if( !lightText.isEmpty() )
	ui->label_message->setText( lightText );

    if( !details.isEmpty() )
	ui->textEdit_details->setText( details );
    else
    {
	ui->pushButton_details->setVisible( false );
    }

    if( !saveText.isEmpty() )
	ui->pushButton_ok->setText( saveText );

    if( !cancelText.isEmpty() )
	ui->pushButton_cancel->setText( cancelText );

    ui->pushButton_cancel->setVisible( showCancel );
}

SaveDialog::~SaveDialog()
{
    delete ui;
}

void SaveDialog::on_pushButton_ok_clicked()
{
    SaveSettings();
    QDialog::accept();
}

void SaveDialog::on_pushButton_cancel_clicked()
{
    SaveSettings();
    QDialog::reject();
}

void SaveDialog::SaveSettings()
{
    QSettings s( settingsPath, QSettings::IniFormat );
    s.beginGroup( "saveDialog" );
    s.setValue( "details", detailsVisible );
    s.endGroup();
}

void SaveDialog::on_pushButton_details_clicked()
{
    ToggleDetails();
}

void SaveDialog::ToggleDetails()
{
    if( detailsVisible )
    {
	bigSize = size();
	detailsVisible = false;
	ui->textEdit_details->setVisible( false );
	ui->pushButton_details->setText( tr( "Show Details >>" ) );
	resize( smallSize );
    }
    else
    {
	smallSize = size();
	detailsVisible = true;
	ui->textEdit_details->setVisible( true );
	ui->pushButton_details->setText( tr( "Hide Details <<" ) );
	resize( bigSize );
    }
}
