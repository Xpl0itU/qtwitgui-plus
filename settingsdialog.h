/************************************************************************************
*
*   - QtWitGui -				2010 giantpune
*
*   the multilingual, multiplatform, multiformat gui for messing with
*   Wii game images.
*
*   This software comes to you with a GPLv3 license.
*   http://www.gnu.org/licenses/gpl-3.0.html
*
*   Basically you are free to modify this code, distribute it, use it in
*   other projects, or anything along those lines.  Just make sure that any
*   derivative work gets the same license.  And don't remove this notice from
*   the derivative work.
*
*   And please, don't be a douche.  If you borrow code from here, don't claim
*   you wrote it.  Share your source code with others.  Even if you are
*   charging a fee for the binaries, let others read the code as somebody has
*   done for you.
*
*************************************************************************************/

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
    class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog( QWidget *parent = 0 );
    ~SettingsDialog();

private:
    Ui::SettingsDialog *ui;
    void ResizeButtons();
    void ResetLogPage();

private slots:
    void on_pushButton_logCrt_clicked();
    void on_pushButton_logWrn_clicked();
    void on_pushButton_logTxt_clicked();
    void on_pushButton_logBg_clicked();
    void on_pushButton_ignore_minus_clicked();
    void on_pushButton_ignore_add_clicked();
    void on_pushButton_coverPath_clicked();
    void on_pushButton_titlesPath_clicked();
    void on_pushButton_wiitdbPath_clicked();
    void on_pushButton_wwt_clicked();
    void on_pushButton_wit_clicked();
    void on_pushButton_ok_clicked();
};

#endif // SETTINGSDIALOG_H
