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

#ifndef CUSTOMMDIITEM_H
#define CUSTOMMDIITEM_H

#include <QObject>
#include <QMdiSubWindow>

enum
{
    mdiAny = -1,
    mdiNone = 0,
    mdiPartition,
    mdiGame,
    mdiWiiTDB,
    mdiCovers,
    mdiLog
};

class CustomMdiItem : public QMdiSubWindow
{
    Q_OBJECT
public:
	CustomMdiItem( QWidget * parent = 0, Qt::WindowFlags flags = 0, const QString &windowTitle = QString(), bool deleteOnClose = true );

    //provide a way to distinguish mdi windows
    QString GetTitle();
	void setWindowTitle( const QString &windowTitle );
    int type;

private:
    QString title;

signals:
    //provide a way for other stuff to know this window is closing
	void AboutToClose( const QString &title, const QPoint &pos, const QSize &size, int type );

public slots:

protected:
    void closeEvent( QCloseEvent * closeEvent );

};

#endif // CUSTOMMDIITEM_H
