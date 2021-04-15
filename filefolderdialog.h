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
#ifndef FILEFOLDERDIALOG_H
#define FILEFOLDERDIALOG_H
#include "includes.h"

class FileFolderDialog: public QFileDialog
{
    Q_OBJECT
public:
    FileFolderDialog( QWidget * parent, Qt::WindowFlags flags ):QFileDialog( parent,flags ){}
    FileFolderDialog ( QWidget * parent = 0, const QString & caption = QString(), const QString & directory = QString(), const QString & filter = QString() ):
    QFileDialog ( parent ,caption,directory,filter ){}
public slots:
    void accept();
};

#endif // FILEFOLDERDIALOG_H


