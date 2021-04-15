#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include "includes.h"

AboutDialog::AboutDialog(QWidget *parent, const QString &qtwitguiVersion, const QString &witVersion\
			, const QString &wwtVersion, const QString &wiitdbVersion, const QString &wiitdbGames ) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    //setWindowTitle( tr( "About" ) );
    QString unk = tr( "Unknown" );
    ui->label_qtwitgui_version->setText( qtwitguiVersion.isEmpty() ? unk : qtwitguiVersion );
    ui->label_wit_text->setText( witVersion.isEmpty() ? unk : witVersion );
    ui->label_wwt_text->setText( wwtVersion.isEmpty() ? unk : wwtVersion );
    ui->label_wiitdb_v_text->setText( wiitdbVersion.isEmpty() ? unk : wiitdbVersion );
    ui->label_wiitdb_games_text->setText( wiitdbGames.isEmpty() ? unk : wiitdbGames );

    ui->label_qtwitgui_url->setOpenExternalLinks( true );
    ui->label_qtwitgui_url->setText( "<a href=\"http://code.google.com/p/qtwitgui/\">http://code.google.com/p/qtwitgui/</a>" );

    ui->label_wiimms_url->setOpenExternalLinks( true );
    ui->label_wiimms_url->setText( "<a href=\"http://wit.wiimm.de/\">http://wit.wiimm.de/</a>" );

    ui->label_wiitdb_url->setOpenExternalLinks( true );
    ui->label_wiitdb_url->setText( "<a href=\"http://wiitdb.com/\">http://wiitdb.com/</a>" );
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::on_pushButton_clicked()
{
    QDialog::accept();
}
