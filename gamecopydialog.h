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

#ifndef GAMECOPYDIALOG_H
#define GAMECOPYDIALOG_H

#include "includes.h"

namespace Ui {
    class GameCopyDialog;
}

class GameCopyDialog : public QDialog
{
    Q_OBJECT

public:
	explicit GameCopyDialog(QWidget *parent = 0, const QList<QTreeWidgetItem *> &partitions = QList<QTreeWidgetItem *>(),
							const QStringList &games = QStringList(), const QString &current = QString(),
							const QStringList &patchArgs = QStringList() );
    ~GameCopyDialog();

	static QStringList WitCopyCommand( QWidget *parent, const QString &currentPartition, const QList<QTreeWidgetItem *> &partitions,
									   const QStringList &games, const QStringList &patchArgs );
private:
    Ui::GameCopyDialog *ui;
    QList<QTreeWidgetItem *> partList;
    QStringList gameList;
    QStringList ret;
    QString currentPart;
    QStringList patchArgList;

    bool singleGame;
    bool currentPathIswbfs;

    void UpdateGuiWithCurrentSettings( bool skipPartitionFlags = false );
    void SaveSettings();
    void LoadSettings();

private slots:
    void on_checkBox_file_clicked(bool checked);
    void on_checkBox_fst_sneek_toggled(bool checked);
    void on_comboBox_wia_method_currentIndexChanged(int index);
    void on_groupBox_wia_advanced_toggled(bool );
    void on_groupBox_wia_presets_toggled(bool checked);
    void on_comboBox_container_currentIndexChanged(int index);
	void on_lineEdit_destOther_textChanged( const QString &);
    void on_pushButton_destOther_clicked();
    void on_groupBox_partition_clicked(bool );
    void on_buttonBox_accepted();
	void on_comboBox_esc_currentIndexChanged(const QString &);
    void on_checkBox_partitionisWBFS_clicked(bool checked);
	void on_comboBox_dest_currentIndexChanged(const QString &);
};

#endif // GAMECOPYDIALOG_H
