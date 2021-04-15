#include <QtWidgets/QApplication>
#include "mainwindow.h"
#include "includes.h"
#include "wiitdb.h"
#include "tools.h"
#include "svnrev.h"

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE( resources );

    QApplication a(argc, argv);
    settingsPath = QStandardPaths::StandardLocation( QStandardPaths::HomeLocation ) + "/QtWitGui.ini";
    SetupLog();
    qDebug().nospace() << "<b><text style=\"color:green\">" << PROGRAM_NAME << " r" << SVN_REV_STR << "</text></b>";

    QTranslator translator;
    if( !translator.load( QString( "language/witGuiLang_" ) + QLocale::system().name() ) )
		translator.load( QString( ":/language/witGuiLang_" ) + QLocale::system().name() );

    a.installTranslator( &translator );

    wiiTDB = new WiiTDB;

    QApplication::setWindowIcon( QIcon( ":images/icon.ico" ) );

    MainWindow w;
    w.showMaximized();

    QStringList args = QApplication::arguments();
    if( args.contains( "--greyness") )
    {
		QFile file( ":/darkTheme.qss" );
		file.open( QFile::ReadOnly );
		QString styleSheet = QLatin1String( file.readAll() );
		w.setStyleSheet( styleSheet );
    }
    w.show();

    int ret = a.exec();
    delete wiiTDB;
    delete logWindow;
#ifdef Q_OS_WIN    //on windows XP, wmic leaves behind a temp file ( 0 bytes ): just delete it
    if( QFile::exists( "./TempWmicBatchFile.bat" ) )
        QFile::remove( "./TempWmicBatchFile.bat" );
#endif
    return ret;
}
