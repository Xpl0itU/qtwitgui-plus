#include "passworddialog.h"
#include "ui_passworddialog.h"

extern QString rootPass;
PasswordDialog::PasswordDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PasswordDialog)
{
    ui->setupUi(this);
    //this->setWindowTitle( tr( "Password Requested" ) );
}

PasswordDialog::~PasswordDialog()
{
    delete ui;
}

void PasswordDialog::on_pushButton_ok_clicked()
{
    rootPass =  ui->lineEdit->text() + "\n";
    QDialog::accept();
}
