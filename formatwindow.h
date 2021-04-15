#ifndef FORMATWINDOW_H
#define FORMATWINDOW_H

#include "includes.h"
#include "wwthandler.h"

namespace Ui {
    class FormatWindow;
}

class FormatWindow : public QDialog
{
    Q_OBJECT

public:
    explicit FormatWindow(QWidget *parent = 0);
    ~FormatWindow();

private:
    Ui::FormatWindow *ui;
    WwtHandler wwt;

    bool alreadyAskingForPassword;
    void AddItemToTree( const QString &part );

private slots:
    void on_pushButton_format_clicked();
    void on_pushButton_refresh_clicked();
    void on_pushButton_done_clicked();
	void HandleWiimmsErrors( const QString &err, int id );
	void GetPartitionList( const QStringList &list );
	void FormatDone( const QString &text );
#ifndef Q_WS_WIN
    void NeedToAskForPassword();
#endif

signals:
#ifndef Q_WS_WIN
    void UserEnteredPassword();
#endif
};

//spinbox to use for HSS, should only allow values that are a power of 2
class QSpinBoxPowerOf2 : public QSpinBox
{
    Q_OBJECT
public:
    QSpinBoxPowerOf2( QWidget *parent = 0 );
    virtual void stepBy( int steps );
private:
    int lastGoodValue;
};

#endif // FORMATWINDOW_H
