/*
  This file is part of QtWitGui.  This is just a small helper program.
  Wiimms' ISO tools use cygwin to access the drives.  This way, I can
  get available DVD drives using the correct path for his tools
  without having to add a cygwin dependency to build my entire project.
  I assume all of the DVD drives will be "/dev/sr*".

  just build this in cygwin with
  gcc listDvd.c -o listDvd
*/
#include <stdio.h>

#define PROGRAM "listDvd"
#define VERSION "1.0"
int main( int argc, char ** argv )
{
    if( argc > 1 && argv[ 1 ] && !strcmp( argv[ 1 ], "--version" ) )
    {
        printf( PROGRAM" "VERSION" \n" );
        return 0;
    }
    FILE*f;
    int i;
    for( i = 0; i < 15; i++ )
    {
        char path[ 15 ];
        sprintf( path, "/dev/sr%d", i );
        f = fopen( path, "r" );
        if( f )
        {
            printf( "%s\n", path );
            fclose( f );
        }
    }
    return 0;
}
