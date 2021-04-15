#ifndef FSINFO_H
#define FSINFO_H


#include "includes.h"
class FsInfo : public QObject
{
Q_OBJECT
public:
    explicit FsInfo(QObject *parent = 0);

    static QString GetFilesystem( QString path );
    static QStringList GetDvdDrives();
#ifdef Q_WS_WIN
    //only used for windows stuff
    static bool Check();
	static QString ToWinPath( const QString &cygPath, bool *ok );
	static QString ToCygPath( const QString &winPath, bool *ok );
    static bool IsDVDLetter( const QString &path );
#endif
signals:

public slots:

};

#endif // FSINFO_H
