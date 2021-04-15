#include "includes.h"
#include "gc_shrinkthread.h"
#include "tools.h"


#include <stdio.h>
#include <stdlib.h>
#ifndef Q_WS_MAC
#include <malloc.h>
#else//windows and osx doesnt have memalign, and the image conversion API says it wants aligned memory
void *memalign( size_t size, int align )
{
    void *mem = malloc( size + (align-1) + sizeof(void*) );

    char *amem = ((char*)mem) + sizeof(void*);
    amem += align - ((quint64)amem & (align - 1));

    ((void**)amem)[-1] = mem;
    return amem;
}

void aligned_free( void *mem )
{
    free( ((void**)mem)[-1] );
}
#endif
#ifdef Q_WS_WIN
extern "C" void * _aligned_malloc( size_t size, size_t alignment );
extern "C" void _aligned_free( void *memblock );
#endif

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
#error "This file has not been tested on a big endian machine."
#error "You can remove this error and compile the program, but be careful while using the"
#error "functions in this file - shrinking and aligning gamecube games."
#error "If you would like to help out, please test a few gamecube games and"
#error "let me know if everything is working properly or not."
#endif



GC_ShrinkThread::GC_ShrinkThread( QObject *parent, const QString &path ) : QThread( parent ), GC_Game( path )
{
    abort = false;
}

GC_ShrinkThread::~GC_ShrinkThread()
{
//    qDebug( "Thread destruct" );
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();

    wait();
}

void GC_ShrinkThread::FatalError( const QString &str )
{
    qDebug() << str;
    emit SendFatalError( str, GC_FATAL );
}

void GC_ShrinkThread::ShrinkTo( const QString &destPath, bool over )
{
    if( !fileOk )
    {
	emit SendFatalError( "File is not a valid gamecube game", GC_FATAL );
	return;
    }
    newPath = destPath;
    overwrite = over;
    QMutexLocker locker(&mutex);
    if ( !isRunning() )
    {
	start( NormalPriority );
    }
    else
    {
	emit SendFatalError( "already running", GC_FATAL );
	return;
    }
}

void GC_ShrinkThread::run()
{
	if ( abort )
	{
	    qDebug( "Thread abort" );
	    return;
	}
	mutex.lock();

	QFileInfo f1( file );
	QFileInfo f2( newPath );
	QFile newFile;
	if( f1.absoluteFilePath() == f2.absoluteFilePath() )//destination and source are the same
	{
	    QString ppp = newPath;
	    newFile.setFileName( ppp );//file a filename that doesnt exist
	    while( newFile.exists() )
	    {
		ppp += "_";
		newFile.setFileName( ppp );
	    }
	}
	else
	    newFile.setFileName( newPath );

	mutex.unlock();


	if( newFile.exists() )
	{
	    if( !overwrite )
	    {
		emit SendFatalError( tr( "File already exists.<br>Choose another filename or enable file overwriting in the settings." ), GC_FATAL );
		return;
	    }
	    if( !newFile.remove() )
	    {
		emit SendFatalError( tr( "Can\'t delete file" ), GC_FATAL );
		return;
	    }
	}
	if( !newFile.open( QIODevice::WriteOnly ) || !file.open( QIODevice::ReadOnly ) )
	{
	    emit SendFatalError( tr( "Can\'t open file" ), GC_FATAL );
	    return;
	}
	//qDebug() << "Shrinking:" << oldFile.fileName() << "to" << newFile.fileName();

	int zeros;
	float outSize = 0;
	FEntry *fe = (FEntry*)(fst_bin.data());
	quint32 Entries = qFromBigEndian( fe[ 0 ].FileLength );

	QByteArray nFst( fst_bin );
	FEntry *newFst = (FEntry *)nFst.data();

	//shift all the offsets in the new fst down
	quint32 offset = RU( ALIGN, fstOffset + fstSize );
	int totalFiles = 0;
	for( quint32 i = 0; i < Entries; i++ )
	{
	    if( !fe[ i ].Type )
	    {
		newFst[ i ].FileOffset = qFromBigEndian( offset );
		offset += qFromBigEndian( fe[ i ].FileLength );
		offset = RU( ALIGN, offset );
		totalFiles++;

		if( i == dolReused )
		{
		    quint32 *nBootBin = (quint32*)newBootBin.data();
		    nBootBin[ 264 ] = newFst[ i ].FileOffset;
		    dolOffset = qFromBigEndian( newFst[ i ].FileOffset );
		    //qDebug() << "reusing main dol as fst[" << i << "]" << hex << qFromBigEndian( nBootBin[ 264 ] );
		    //hexdump( (void*)( ( (char*)newBootBin.data() ) + 0x420 ), 0x20 );
		}
	    }
	}
	outSize = offset;
	//qDebug() << "outSize" << (float)( offset / _MiB_ ) << "MiB";


	//make sure aligning the files does make this game bigger
	quint64 maxGCSize = 0x57058000ULL;
	if( offset >= maxGCSize )
	{
	    emit SendFatalError( tr( "File alignment will make this iso larger" ), GC_FATAL );
	    file.close();
	    newFile.close();
	    newFile.remove();
	    return;
	}
	file.seek( 0 );

	if( dolReused )
	{
	    //write new boot.bin with new dol offset
	    newFile.write( newBootBin );

	    //copy everything else up to fst
	    file.seek( 0x440 );
	    newFile.write( file.read( fstOffset - 0x440 ) );
	}
	else
	{
	    //copy everything up till through the main.dol
	    int toRead = dolOffset + dolSize;
	    newFile.write( file.read( toRead ) );

	    //pad until fst
	    zeros = fstOffset - newFile.pos();
	    if( zeros < 0 )
	    {
		emit SendFatalError( "Padding is < 0", GC_FATAL );
		file.close();
		newFile.close();
		newFile.remove();
		return;
	    }
	    if( zeros > 0 )
		newFile.write( QByteArray( zeros, '\0' ) );
	}

	//write the new fst
	newFile.write( nFst );

	//copy all the files
	//quint32 curr = 0;
	for( quint32 i = 0; i < Entries; i++ )
	{
	    if( abort )
	    {
		qDebug() << "aborting gc thread";
		file.close();
		newFile.close();
		newFile.remove();
		return;
	    }
	    if( !fe[ i ].Type )
	    {
		zeros = qFromBigEndian( newFst[ i ].FileOffset ) - newFile.pos();
		if( zeros < 0 )
		{
		    emit SendFatalError( "Padding is < 0 [2]", GC_FATAL );
		    file.close();
		    newFile.close();
		    newFile.remove();
		    return;
		}
		if( zeros > 0 )//pad wth 0s between files
		{
		    newFile.write( QByteArray( zeros, '\0' ) );
		}
		file.seek( qFromBigEndian( fe[ i ].FileOffset ) );
		QByteArray shit = file.read( qFromBigEndian( fe[ i ].FileLength ) );
		if( (quint32)shit.size() != qFromBigEndian( fe[ i ].FileLength ) )
		{
		    emit SendFatalError( "Read eror", GC_FATAL );
		    file.close();
		    newFile.close();
		    newFile.remove();
		    return;
		}
		newFile.write( shit );

		emit SendProgress( (float)( (float)newFile.pos() / (float)outSize ) * 100 );
	    }
	}

	zeros = offset - newFile.pos();
	if( zeros > 0 )//final padding for wode/sd boot
	{
	    newFile.write( QByteArray( zeros, '\0' ) );
	}
	newFile.flush();
	file.close();
	newFile.close();

	//remove old file and rename new one
	if( f1.absoluteFilePath() == f2.absoluteFilePath() )//destination and source are the same
	{
	    file.remove();
	    newFile.rename( f1.absoluteFilePath() );
	}
	emit SendProgress( 100 );
	emit SendDone();
	//qDebug() << "shrink done";


}

quint32 swap24( quint32 i )
{
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    return i & 0xffffff;
#else
    return qFromBigEndian( i ) >> 8;
#endif
}

quint32 get_dol_size( const quint32 *header )
{
    quint8 i;
    quint32 offset, size;
    quint32 max = 0;

    for( i = 0; i < 18; i++ )
    {
	offset = qFromBigEndian( header[ i ] );
	size = qFromBigEndian( header[ 0x24 + i ] );
	if( offset + size > max )
	    max = offset + size;
	//qDebug() << i << "off:" << hex << offset << "size:" <<hex << size;
    }
    return( max );
}

quint32 GC_Game::ReadU32()
{
    quint32 ret;
    file.read( (char*)&ret, 4 );
    return qFromBigEndian( ret );
}

int TPL_ConvertRGB5A3ToBitMap(quint8* tplbuf, quint8** bitmapdata, qint32 width, qint32 height)
{
	qint32 x, y;
	qint32 x1, y1;
	qint32 iv;
	//tplpoint -= width;
#ifdef Q_WS_WIN
	*bitmapdata = (quint8*)_aligned_malloc( width * height * 4, 32 );
#else
	*bitmapdata = (quint8*)memalign( 32, width * height * 4 );
#endif
	if(*bitmapdata == NULL)
		return -1;
	quint32 outsz = width * height * 4;
	for(iv = 0, y1 = 0; y1 < height; y1 += 4) {
		for(x1 = 0; x1 < width; x1 += 4) {
			for(y = y1; y < (y1 + 4); y++) {
				for(x = x1; x < (x1 + 4); x++) {
					quint16 oldpixel = *(quint16*)(tplbuf + ((iv++) * 2));
					if((x >= width) || (y >= height))
						continue;
					oldpixel = qFromBigEndian(oldpixel);
					if(oldpixel & (1 << 15)) {
						// RGB5
						quint8 b = (((oldpixel >> 10) & 0x1F) * 255) / 31;
						quint8 g = (((oldpixel >> 5)  & 0x1F) * 255) / 31;
						quint8 r = (((oldpixel >> 0)  & 0x1F) * 255) / 31;
						quint8 a = 255;
						quint32 rgba = (r << 0) | (g << 8) | (b << 16) | (a << 24);
						(*(quint32**)bitmapdata)[x + (y * width)] = rgba;
					}else{
						// RGB4A3
						/*quint8 b = (((oldpixel >> 12) & 0xF) * 255) / 15;
						quint8 g = (((oldpixel >> 8)  & 0xF) * 255) / 15;
						quint8 r = (((oldpixel >> 4)  & 0xF) * 255) / 15;
						quint8 a = (((oldpixel >> 0)  & 0x7) * 64) / 7;
						quint32 rgba = (r << 0) | (g << 8) | (b << 16) | (a << 24);
						(*(quint32**)bitmapdata)[x + (y * width)] = rgba;*/

					    quint8 a = (((oldpixel >> 12) & 0x7) * 255) / 7;
					    quint8 b = (((oldpixel >> 8)  & 0xF) * 255) / 15;
					    quint8 g = (((oldpixel >> 4)  & 0xF) * 255) / 15;
					    quint8 r = (((oldpixel >> 0)  & 0xF) * 255) / 15;
					    quint32 rgba = (r << 0) | (g << 8) | (b << 16) | (a << 24);
					    (*(quint32**)bitmapdata)[x + (y * width)] = rgba;
					}
				}
			}
		}
	}
	return outsz;
}

GC_Game::GC_Game( const QString &path )
{
    fileOk = false;
    pathstr = path;
    dolReused = 0;

    if( !pathstr.contains( ".iso", Qt::CaseInsensitive ) && !pathstr.contains( ".gcm", Qt::CaseInsensitive ) )
    {
	FatalError( "Invalid filename" );
	return;
    }
    file.setFileName( pathstr );
    if( !file.exists() || !file.open( QIODevice::ReadOnly ) )
    {
	FatalError( "Can\t open file" );
	return;
    }

    //check stuff in the file header
    if( !file.seek( 0x1c ) ){ FatalError( "Seek error" ); file.close(); return; }
    quint32 magic;
    file.read( (char*)&magic, 4 );
    if( magic != qFromBigEndian( 0xC2339F3D ) ){ FatalError( "Wrong magic number" ); file.close(); return; }

    file.seek( 0 );
    id = QString( file.read( 6 ) );
    //qDebug() << "id:" << id;

    file.seek( 0x20 );
    hdrname = QString( file.read( 0x30 ) );
    //qDebug() << "hdrname:" << hdrname;

    file.seek( 0x420 );
    //QByteArray shit = file.peek( 0x20 );
    //hexdump( (void*)shit.data(), 0x20 );
    dolOffset = ReadU32();
    fstOffset = ReadU32();
    fstSize = ReadU32();
    //qDebug() << "dolOffset:" << hex << dolOffset << "fstOffset:" << hex << fstOffset << "fstSize:" << hex << fstSize;

    file.seek( dolOffset );
    QByteArray dolbuf = file.read( 0x100 );
    dolSize = get_dol_size( (quint32*)dolbuf.data() );
    //qDebug() << "dolSize:" << hex << dolSize << hex << get_dol_size2( (quint32*)dolbuf.data() );

    file.seek( 0x2454 );
    quint32 s1 = ReadU32();
    quint32 s2 = ReadU32();
    apploaderSize = s1 + s2 + 0x20;
    //qDebug() << "apploaderSize:" << hex << apploaderSize;

    quint32 bnrSize = 0;
    quint32 bnrOff  = 0;

    file.seek( fstOffset );
    fst_bin = file.read( fstSize );

    FEntry* fst = (FEntry*)fst_bin.data();
    quint32 cnt = qFromBigEndian( fst[0].FileLength );
    quint32 NameOff = cnt * 0x0C;

    //qDebug() << "cnt:" << hex << cnt << "NameOff:" << hex << NameOff;

    bool found = false;
    for (quint32 i = 0; i < cnt; i++)
    {
	if( !fst[ i ].Type )
	{
	    int nameStart = swap24( fst[ i ].NameOffset ) + NameOff;
	    //QByteArray n = fst_bin.mid( nameStart, fst_bin.size() - nameStart );
	    QString name = QString( fst_bin.mid( nameStart, fst_bin.size() - nameStart ) );
	    //qDebug() << name;
	    if( !name.compare( "opening.bnr", Qt::CaseInsensitive ) )
	    {
		bnrSize = qFromBigEndian( fst[ i ].FileLength );
		bnrOff  = qFromBigEndian( fst[ i ].FileOffset );
		//qDebug() << "banner off:" << hex << bnrOff << "bnrSize:" << hex << bnrSize;
		//printf("opening.bnr @ %d\nsize: %08x\t offset: %08x\n", i, bnrSize, bnrOff );
		found = true;
		//break;
	    }
	    //is the main.dol reused in the fst?
	    if( qFromBigEndian( fst[ i ].FileOffset ) == dolOffset )
	    {
		if( qFromBigEndian( fst[ i ].FileLength ) != dolSize )
		{
		    qDebug() << "huston, we have a problem";
		    FatalError( "dol re-use / mismatch" ); file.close(); return;
		}
		dolReused = i;
	    }
	}
    }

    //if the main.dol is reused, copy the boot.bin
    if( dolReused )
    {
	file.seek( 0 );
	newBootBin = file.read( 0x440 );
    }

    if( !found )
    {
	FatalError( "No opening.bnr found"); file.close(); return;
    }

    file.seek( bnrOff );
    file.read( (char*)&opening_bnr, bnrSize );
    file.close();

    //does this banner contain multiple languages
    langCnt = ( opening_bnr.type == qFromBigEndian( 0x424E5232 ) ? 6 : 1 );
    fileOk = true;

    //qDebug() << "GC opened ok";
}

GC_Game::~GC_Game()
{
    if( file.isOpen() )
	file.close();
}

void GC_Game::FatalError( const QString &str )
{
    Q_UNUSED( str );
}

QPixmap GC_Game::BannerImage( int height )
{
    if( !fileOk )
	return QPixmap();
    quint8* bitmapdata;
    int r = TPL_ConvertRGB5A3ToBitMap( (quint8*)&opening_bnr.image, &bitmapdata, 96, 32);
    if( r != 12288 )
    {
	qDebug() << "converted:" << r;
	return QPixmap();
    }
    QImage im = QImage( (const uchar*) bitmapdata, 96, 32, QImage::Format_ARGB32 );
    QImage im2 = im.scaledToHeight( height, Qt::SmoothTransformation );
#ifdef Q_WS_LINUX
    //qDebug() << "deleting buffer with free()";
    free( bitmapdata );
#elif defined Q_WS_MAC
    //qDebug() << "deleting buffer with aligned_free()";
    aligned_free( bitmapdata );
#else//not linux or mac.  must be windows
    //qDebug() << "deleting buffer with _aligned_free()";
    _aligned_free( bitmapdata );
#endif

    return QPixmap::fromImage( im2 );
}




