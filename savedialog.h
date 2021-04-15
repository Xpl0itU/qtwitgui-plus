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

#ifndef SAVEDIALOG_H
#define SAVEDIALOG_H

#include "includes.h"

namespace Ui {
    class SaveDialog;
}

class SaveDialog : public QDialog
{
    Q_OBJECT

public:
	explicit SaveDialog( QWidget *parent = 0, const QString &boldText = QString(), const QString &lightText = QString(), const QString &details = QString()
			, const QString &saveText = QString(), const QString &cancelText = QString(), bool showCancel = true );
    ~SaveDialog();

private:
    Ui::SaveDialog *ui;
    void ToggleDetails();
    QSize bigSize;
    QSize smallSize;
    bool detailsVisible;
    void SaveSettings();

private slots:
    void on_pushButton_details_clicked();
    void on_pushButton_cancel_clicked();
    void on_pushButton_ok_clicked();
};

#endif // SAVEDIALOG_H
