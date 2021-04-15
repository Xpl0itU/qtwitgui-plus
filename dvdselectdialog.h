#ifndef DVDSELECTDIALOG_H
#define DVDSELECTDIALOG_H

#include "includes.h"

namespace Ui {
    class DvdSelectDialog;
}

class DvdSelectDialog : public QDialog {
    Q_OBJECT
public:
    DvdSelectDialog(QWidget *parent = 0);
    ~DvdSelectDialog();

    static QStringList GetDvdToOpen( QWidget *parent = 0 );

private:
    QStringList ret;

protected:
    void changeEvent(QEvent *e);

private:
    Ui::DvdSelectDialog *ui;

private slots:
    void on_pushButton_refresh_clicked();
    void on_buttonBox_accepted();
};

#endif // DVDSELECTDIALOG_H
