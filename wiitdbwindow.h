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

#ifndef WIITDBWINDOW_H
#define WIITDBWINDOW_H


#include "includes.h"
#include "wiitdb.h"

namespace Ui {
    class WiiTDBWindow;
}

class WiiTDBWindow : public QWidget
{
    Q_OBJECT

public:
    explicit WiiTDBWindow( QWidget *parent = 0 );
    ~WiiTDBWindow();
    void SetFile( const QString &path );

    QString GetVersion();
    int GameCount();



private:
    Ui::WiiTDBWindow *ui;
    QGraphicsScene gv;

    void AddImage( const QPixmap &pm );
    void AddImage( const QString &str );
    //WiiTDB wiiTDB;
    void ClearGui();

public slots:
    void LoadGameFromID( const QString &id );

private slots:
    void on_comboBox_searchSynopsisLang_currentIndexChanged(int index);
    void on_comboBox_searchRatingType_currentIndexChanged(int index);
    void on_checkBox_search_LFF_stateChanged(int );
    void on_checkBox_searchDS_stateChanged(int );
    void on_checkBox_searchWS_stateChanged(int );
    void on_checkBox_searchKB_stateChanged(int );
    void on_checkBox_searchGBA_stateChanged(int );
    void on_checkBox_searchMic_stateChanged(int );
    void on_checkBox_search_Drums_stateChanged(int );
    void on_checkBox_searchGuitar_stateChanged(int );
    void on_checkBox_searchGCN_stateChanged(int );
    void on_checkBox_searchDDR_stateChanged(int );
    void on_checkBox_searchWheel_stateChanged(int );
    void on_checkBox_searchCC_stateChanged(int );
    void on_checkBox_searchMP_stateChanged(int );
    void on_checkBox_searchBongos_stateChanged(int );
    void on_checkBox_searchBB_stateChanged(int );
    void on_checkBox_search_7_stateChanged(int );
    void on_checkBox_searchNunchuk_stateChanged(int );
    void on_checkBox_searchWiimote_stateChanged(int );
    void on_treeWidget_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
    void on_treeWidget_itemClicked(QTreeWidgetItem* item, int column);
    void on_pushButton_search_clicked();
    void on_pushButton_searchBack_clicked();
	void ReceiveErrorFromWiiTDB( const QString &title, const QString &detail );

signals:
    void Destroyed();
};

#endif // WIITDBWINDOW_H
