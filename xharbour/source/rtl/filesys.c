/*
 * $Id: filesys.c,v 1.139 2005/01/28 12:16:41 alexstrickland Exp $
 */

/*
 * Harbour Project source code:
 * The FileSys API (C level)
 *
 * Copyright 1999 {list of individual authors and e-mail addresses}
 * www - http://www.harbour-project.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA (or visit the web site http://www.gnu.org/).
 *
 * As a special exception, the Harbour Project gives permission for
 * additional uses of the text contained in its release of Harbour.
 *
 * The exception is that, if you link the Harbour libraries with other
 * files to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the Harbour library code into it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 *
 * This exception applies only to the code released by the Harbour
 * Project under the name Harbour.  If you copy code from other
 * Harbour Project or Free Software Foundation releases into a copy of
 * Harbour, as the General Public License permits, the exception does
 * not apply to the code that you add in this way.  To avoid misleading
 * anyone as to the status of such modified files, you must delete
 * this exception notice from them.
 *
 * If you write modifications of your own for Harbour, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice.
 *
 */

/*
 * The following parts are Copyright of the individual authors.
 * www - http://www.harbour-project.org
 *
 * Copyright 1999-2001 Viktor Szakats <viktor.szakats@syenar.hu>
 *    hb_fsSetError()
 *    hb_fsSetDevMode()
 *    hb_fsReadLarge()
 *    hb_fsWriteLarge()
 *    hb_fsCurDirBuff()
 *
 * Copyright 1999 Jose Lalin <dezac@corevia.com>
 *    hb_fsChDrv()
 *    hb_fsCurDrv()
 *    hb_fsIsDrv()
 *    hb_fsIsDevice()
 *
 * Copyright 2000 Luiz Rafael Culik <culik@sl.conex.net>
 *            and David G. Holm <dholm@jsd-llc.com>
 *    hb_fsEof()
 *
 * Copyright 2001 Jose Gimenez (JFG) <jfgimenez@wanadoo.es>
 *                                   <tecnico.sireinsa@ctv.es>
 *    Added __WIN32__ check for any compiler to use the Win32
 *    API calls to allow openning an unlimited number of files
 *    simultaneously.
 *
 * Copyright 2003 Giancarlo Niccolai <gian@niccolai.ws>
 *    hb_fsOpenProcess()
 *    hb_fsProcessValue()
 *    hb_fsCloseProcess()
 *    Unix sensible creation flags
 *    Thread safing and optimization
 *
 * See doc/license.txt for licensing terms.
 *
 */

/* NOTE: In DOS/DJGPP under WinNT4 hb_fsSeek( fhnd, offset < 0, FS_SET) will
         set the file pointer to the passed negative value, and the subsequent
         hb_fsWrite() call will fail. In CA-Clipper hb_fsSeek() will fail,
         the pointer will not be moved, and thus the hb_fsWrite() call will
         successfully write the buffer to the current file position. [vszakats]

   This has been corrected by ptucker
 */


#if defined(HB_OS_LINUX)
#  define _LARGEFILE64_SOURCE
#endif

#ifndef HB_OS_WIN_32_USED
   #define HB_OS_WIN_32_USED
#endif
#define HB_THREAD_OPTIMIZE_STACK

#include <ctype.h>

#include "hbapi.h"
#include "hbapifs.h"
#include "hb_io.h"
#include "hbset.h"

#if defined(OS_UNIX_COMPATIBLE)
   #include <unistd.h>
   #include <signal.h>
   #include <sys/types.h>
   #include <sys/wait.h>
   #if !defined( HB_OS_DARWIN )
      extern char **environ;
   #else
      #include <crt_externs.h>
      #define environ (*_NSGetEnviron())
   #endif
#endif

#if ( defined(__BORLANDC__) || defined(__IBMCPP__) || defined(_MSC_VER) || \
      defined(__MINGW32__) || defined(__WATCOMC__) ) && !defined( HB_OS_UNIX )
   #include <sys/stat.h>
   #include <share.h>
   #include <fcntl.h>
   #include <errno.h>
   #include <direct.h>
   #include <process.h>
   #if defined(__BORLANDC__)
      #include <dir.h>
      #include <dos.h>
   #elif defined(__WATCOMC__)
      #include <dos.h>
   #endif

   #if defined(_MSC_VER) || defined(__MINGW32__)
      #include <sys/locking.h>
      #define ftruncate _chsize
      #if defined(__MINGW32__) && !defined(_LK_UNLCK)
         #define _LK_UNLCK _LK_UNLOCK
      #endif
   #else
      #define ftruncate chsize
   #endif
   #if !defined(HAVE_POSIX_IO)
      #define HAVE_POSIX_IO
   #endif
#elif defined(__GNUC__) || defined( HB_OS_UNIX )
   #include <sys/types.h>
   #include <sys/stat.h>
   #include <fcntl.h>
   #include <errno.h>
   #if defined(__CYGWIN__)
      #include <io.h>
   #elif defined(__DJGPP__)
      #include <dir.h>
   #endif
   #if !defined(HAVE_POSIX_IO)
      #define HAVE_POSIX_IO
   #endif
#endif

#if defined(__MPW__)
   #include <fcntl.h>
#endif

#if defined(HB_OS_HPUX)
   extern int fdatasync(int fildes);
#endif

#if defined(HB_OS_DOS)
   #include <dos.h>
#endif

#if defined( HB_WIN32_IO )
   #include <windows.h>

   #if ( defined( _MSC_VER ) || defined( __LCC__ ) ) && !defined( INVALID_SET_FILE_POINTER )
      #define INVALID_SET_FILE_POINTER ((DWORD)-1)
   #endif
#endif

#ifdef HB_OS_OS2
   #include <sys/signal.h>
   #include <sys/process.h>
   #include <sys/wait.h>
#endif

/* 27/08/2004 - <maurilio.longo@libero.it>
                HB_FS_GETDRIVE() should return a number in the range 0..25 ('A'..'Z')
                HB_FS_SETDRIVE() should accept a number inside same range.

                If a particular platform/compiler returns/accepts different ranges of
                values, simply define a branch for that platform.

                NOTE: There is not an implicit "current disk", ALWAYS use

                        my_func( hb_fsCurDrv(), ...)

                      to refer to current disk
*/

#if defined( __DJGPP__ )
   #define HB_FS_GETDRIVE(n) ( n = getdisk() )
   #define HB_FS_SETDRIVE(n) ( setdisk( n ) )

#elif defined( __WATCOMC__ )
   #define HB_FS_GETDRIVE(n) ( _dos_getdrive( &( n ) ), --( n ) )
   #define HB_FS_SETDRIVE(n) \
   do { \
      UINT uiDummy; \
      _dos_setdrive( ( n ) + 1, &uiDummy ); \
   } while( 0 );

#elif defined(HB_OS_OS2)
   #define HB_FS_GETDRIVE(n) ( n = _getdrive() - 65 )
   #define HB_FS_SETDRIVE(n) ( _chdrive( ( n ) + 65 ) )

#else
   #define HB_FS_GETDRIVE(n) ( ( ( n = _getdrive() ) < 65 ) ? --( n ) : ( (n) -= 65 ) )
   #define HB_FS_SETDRIVE(n) ( _chdrive( ( n ) + 1 ) )

#endif

#ifndef O_BINARY
   #define O_BINARY     0       /* O_BINARY not defined on Linux */
#endif

#ifndef O_LARGEFILE
   #define O_LARGEFILE  0       /* O_LARGEFILE is used for LFS in 32-bit Linux */
#endif

#ifndef S_IEXEC
   #define S_IEXEC      0x0040  /* owner may execute <directory search> */
#endif

#ifndef S_IRWXU
   #define S_IRWXU      (S_IRUSR | S_IWUSR | S_IXUSR)
#endif

#ifndef S_IRUSR
   #define S_IRUSR      0x0100  /* owner may read */
#endif

#ifndef S_IWUSR
   #define S_IWUSR      0x0080  /* owner may write */
#endif

#ifndef S_IXUSR
   #define S_IXUSR      0x0040  /* owner may execute <directory search> */
#endif

#ifndef S_IRWXG
    #define S_IRWXG     (S_IRGRP | S_IWGRP | S_IXGRP)
#endif

#ifndef     S_IRGRP
    #define     S_IRGRP 0x00020 /* read permission, group */
#endif

#ifndef     S_IWGRP
    #define     S_IWGRP 0x00010 /* write permission, grougroup */
#endif

#ifndef     S_IXGRP
    #define     S_IXGRP 0x00008/* execute/search permission, group */
#endif

#ifndef     S_IRWXO
    #define     S_IRWXO    (S_IROTH | S_IWOTH | S_IXOTH)
#endif

#ifndef     S_IROTH
    #define     S_IROTH 0x00004 /* read permission, other */
#endif

#ifndef     S_IWOTH
    #define     S_IWOTH 0x00002 /* write permission, other */
#endif

#ifndef     S_IXOTH
    #define     S_IXOTH 0x00001/* execute/search permission, other */
#endif

#ifndef SH_COMPAT
   #define SH_COMPAT    0x00    /* Compatibility */
#endif

#ifndef SH_DENYRW
   #define SH_DENYRW    0x10    /* Deny read/write */
#endif

#ifndef SH_DENYWR
   #define SH_DENYWR    0x20    /* Deny write */
#endif

#ifndef SH_DENYRD
   #define SH_DENYRD    0x30    /* Deny read */
#endif

#ifndef SH_DENYNO
   #define SH_DENYNO    0x40    /* Deny nothing */
#endif

#if defined(HAVE_POSIX_IO) || defined(_MSC_VER) || defined(__MINGW32__) || defined(__LCC__)
/* Only compilers with Posix or Posix-like I/O support are supported */
   #define HB_FS_FILE_IO
#endif

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__IBMCPP__) || defined(__WATCOMC__) || defined(HB_OS_OS2)
/* These compilers use sopen() rather than open(), because their
   versions of open() do not support combined O_ and SH_ flags */
   #define HB_FS_SOPEN
#endif

#if UINT_MAX == USHRT_MAX
   #define LARGE_MAX ( UINT_MAX - 1L )
#else
   #define HB_FS_LARGE_OPTIMIZED
#endif

#if defined(HB_FS_FILE_IO)

#if defined(HB_WIN32_IO)

   #if defined( __LCC__ )
      __inline void * LongToHandle( const long h )
      {
          return((void *) (INT_PTR) h );
      }
   #endif

   #if defined( _MSC_VER ) && ( _MSC_VER >= 1010 ) && ( ! defined( _BASETSD_H_) || ! defined( HandleToLong ) || defined(__USE_INLINE__) )

      __inline void * LongToHandle( const long h )
      {
          return((void *) (INT_PTR) h );
      }

      __inline long HandleToLong( const void *h )
      {
         return((long) h );
      }
   #endif

HANDLE DostoWinHandle( FHANDLE fHandle)
{
   HANDLE hHandle = LongToHandle( fHandle );

   switch (fHandle)
   {
      case 0:
        return GetStdHandle(STD_INPUT_HANDLE);

      case 1:
        return GetStdHandle(STD_OUTPUT_HANDLE);

      case 2:
        return GetStdHandle(STD_ERROR_HANDLE);

      default :
        return hHandle;
   }
}

static void convert_create_flags( BOOL fCreate, USHORT uiAttr, USHORT uiFlags,
                                  DWORD *dwMode, DWORD *dwShare, DWORD *dwAttr )
{
   if ( fCreate )
   {
      *dwMode = GENERIC_READ | GENERIC_WRITE;

      *dwAttr = FILE_ATTRIBUTE_ARCHIVE;
      if( uiAttr & FC_READONLY )
         *dwAttr |= FILE_ATTRIBUTE_READONLY;
      if( uiAttr & FC_HIDDEN )
         *dwAttr |= FILE_ATTRIBUTE_HIDDEN;
      if( uiAttr & FC_SYSTEM )
         *dwAttr |= FILE_ATTRIBUTE_SYSTEM;
   }
   else
   {
      *dwMode = 0;
      switch( uiFlags & 3 )
      {
         case  FO_READWRITE :
            *dwMode |= GENERIC_READ | GENERIC_WRITE;
            break;
         case FO_WRITE:
            *dwMode |= GENERIC_WRITE;
            break;
         case FO_READ :
            *dwMode |= GENERIC_READ;
            break;
      }
      *dwAttr = FILE_ATTRIBUTE_NORMAL;
   }

   /* shared flags */
   *dwShare = FILE_SHARE_WRITE | FILE_SHARE_READ;

   switch ( uiFlags & ( FO_DENYREAD | FO_DENYWRITE | FO_EXCLUSIVE | FO_DENYNONE ) )
   {
      case FO_DENYREAD :
         *dwShare = FILE_SHARE_WRITE;
         break;
      case FO_DENYWRITE :
         *dwShare = FILE_SHARE_READ;
         break;
      case FO_EXCLUSIVE:
         *dwShare = 0;
         break;
   }
}

#else

/* Convert HARBOUR flags to IO subsystem flags */
static int convert_open_flags( USHORT uiFlags )
{
   /* by default FO_READ + FO_COMPAT is set */
   int result_flags = 0;

   HB_TRACE(HB_TR_DEBUG, ("convert_open_flags(%hu)", uiFlags));

   result_flags |= O_BINARY | O_LARGEFILE;
   HB_TRACE(HB_TR_INFO, ("convert_open_flags: added O_BINARY | O_LARGEFILE\n"));

#if defined(HB_FS_SOPEN)
   if( ( uiFlags & ( FO_WRITE | FO_READWRITE ) ) == FO_READ )
   {
      result_flags |= O_RDONLY;
      HB_TRACE(HB_TR_INFO, ("convert_open_flags: added O_RDONLY\n"));
   }
#else
   if( ( uiFlags & ( FO_WRITE | FO_READWRITE ) ) == FO_READ )
   {
      result_flags |= ( O_RDONLY | SH_COMPAT );
      HB_TRACE(HB_TR_INFO, ("convert_open_flags: added O_RDONLY SH_COMPAT\n"));
   }
#endif

   /* read & write flags */
   if( uiFlags & FO_WRITE )
   {
      result_flags |= O_WRONLY;
      HB_TRACE(HB_TR_INFO, ("convert_open_flags: added O_WRONLY\n"));
   }

   if( uiFlags & FO_READWRITE )
   {
      result_flags |= O_RDWR;
      HB_TRACE(HB_TR_INFO, ("convert_open_flags: added O_RDWR\n"));
   }

#if ! defined(HB_FS_SOPEN) && ! defined( HB_OS_UNIX )
   /* shared flags */
   if( ( uiFlags & FO_DENYREAD ) == FO_DENYREAD )
   {
      result_flags |= SH_DENYRD;
      HB_TRACE(HB_TR_INFO, ("convert_open_flags: added SH_DENYRD\n"));
   }

   else if( uiFlags & FO_EXCLUSIVE )
   {
      result_flags |= SH_DENYRW;
      HB_TRACE(HB_TR_INFO, ("convert_open_flags: added SH_DENYRW\n"));
   }

   else if( uiFlags & FO_DENYWRITE )
   {
      result_flags |= SH_DENYWR;
      HB_TRACE(HB_TR_INFO, ("convert_open_flags: added SH_DENYWR\n"));
   }

   if( uiFlags & FO_DENYNONE )
   {
      result_flags |= SH_DENYNO;
      HB_TRACE(HB_TR_INFO, ("convert_open_flags: added SH_DENYNO\n"));
   }

   if( uiFlags & FO_SHARED )
   {
      result_flags |= SH_DENYNO;
      HB_TRACE(HB_TR_INFO, ("convert_open_flags: added SH_DENYNO\n"));
   }
#endif

   if( uiFlags & FO_CREAT )
   {
      result_flags |= O_CREAT;
      HB_TRACE(HB_TR_INFO, ("convert_open_flags: added O_CREAT\n"));
   }

   if( uiFlags & FO_TRUNC )
   {
      result_flags |= O_TRUNC;
      HB_TRACE(HB_TR_INFO, ("convert_open_flags: added O_TRUNC\n"));
   }

   if( uiFlags & FO_EXCL )
   {
      result_flags |= O_EXCL;
      HB_TRACE(HB_TR_INFO, ("convert_open_flags: added O_EXCL\n"));
   }

   HB_TRACE(HB_TR_INFO, ("convert_open_flags: result is 0x%04x\n", result_flags));

   return result_flags;
}

static unsigned convert_pmode_flags( USHORT uiFlags )
{
   unsigned result_pmode;

   HB_TRACE(HB_TR_DEBUG, ("convert_pmode_flags(%hu)", uiFlags));

   /* by default FC_NORMAL is set */

   //JC1: sorry, but at least under unix is more sensible to
   // create files without the X attribute

   if( uiFlags & FC_HIDDEN )
   {
      result_pmode = S_IRUSR;
   }
   else
   {
      result_pmode = S_IRUSR | S_IRGRP |  S_IROTH;
   }

   if( !( uiFlags & FC_READONLY) )
   {
      if( result_pmode & S_IRUSR )  result_pmode |= S_IWUSR;
      if( result_pmode & S_IRGRP )  result_pmode |= S_IWGRP;
      if( result_pmode & S_IROTH )  result_pmode |= S_IWOTH;
   }


   // JC1: give executable attribute where read is available
   if( uiFlags & FC_SYSTEM )
   {
      if( result_pmode & S_IRUSR )  result_pmode |= S_IXUSR;
      if( result_pmode & S_IRGRP )  result_pmode |= S_IXGRP;
      if( result_pmode & S_IROTH )  result_pmode |= S_IXOTH;
   }

   HB_TRACE(HB_TR_INFO, ("convert_pmode_flags: 0x%04x\n", result_pmode));

   return result_pmode;
}

static void convert_create_flags( USHORT uiFlags, int * result_flags, unsigned * result_pmode )
{
   HB_TRACE(HB_TR_DEBUG, ("convert_create_flags(%hu, %p, %p)", uiFlags, result_flags, result_pmode));

   /* by default FC_NORMAL is set */
   *result_flags = O_BINARY | O_CREAT | O_TRUNC | O_RDWR | O_LARGEFILE;
   *result_pmode = convert_pmode_flags( uiFlags );

   HB_TRACE(HB_TR_INFO, ("convert_create_flags: 0x%04x, 0x%04x\n", *result_flags, *result_pmode));
}

static void convert_create_flags_ex( USHORT uiAttr, USHORT uiFlags, int * result_flags, unsigned * result_pmode )
{
   HB_TRACE(HB_TR_DEBUG, ("convert_create_flags_ex(%hu, %hu, %p, %p)", uiAttr, uiFlags, result_flags, result_pmode));

   convert_create_flags( uiAttr, result_flags, result_pmode );
   *result_flags |= convert_open_flags( uiFlags );

   HB_TRACE(HB_TR_INFO, ("convert_create_flags_ex: 0x%04x, 0x%04x\n", *result_flags, *result_pmode));
}

#endif

static int convert_seek_flags( USHORT uiFlags )
{
   /* by default FS_SET is set */
   int result_flags = SEEK_SET;

   HB_TRACE(HB_TR_DEBUG, ("convert_seek_flags(%hu)", uiFlags));

   if( uiFlags & FS_RELATIVE )
      result_flags = SEEK_CUR;

   if( uiFlags & FS_END )
      result_flags = SEEK_END;

   return result_flags;
}

#endif


/*
 * FILESYS.API FUNCTIONS --
 */

FHANDLE HB_EXPORT hb_fsPOpen( BYTE * pFilename, BYTE * pMode )
{
#if defined(OS_UNIX_COMPATIBLE)
   HB_THREAD_STUB
#endif

   FHANDLE hFileHandle;

   HB_TRACE(HB_TR_DEBUG, ("hb_fsPOpen(%p, %s)", pFilename, pMode));

   //JC1: Defaulting hFileHandle so compilers are happy (and code is more solid)
   hFileHandle = FS_ERROR;

#if defined(OS_UNIX_COMPATIBLE)
   {
      FHANDLE hPipeHandle[2], hNullHandle;
      pid_t pid;
      BYTE * pbyTmp;
      BOOL bRead;
      ULONG ulLen;
      int iMaxFD;

      //JC1: unlocking the stack to allow cancelation points
      HB_STACK_UNLOCK;

      ulLen = strlen( ( char * ) pFilename );
      if( pMode && ( *pMode == 'r' || *pMode == 'w' ) )
         bRead = ( *pMode == 'r' );
      else
      {
         if( pFilename[0] == '|' )
            bRead = FALSE;
         else if( pFilename[ ulLen - 1 ] == '|' )
            bRead = TRUE;
         else
            bRead = FALSE;
      }

      if( pFilename[0] == '|' )
      {
          ++pFilename;
          --ulLen;
      }
      if( pFilename[ ulLen - 1 ] == '|' )
      {
          pbyTmp = hb_strdup( ( char * ) pFilename );
          pbyTmp[--ulLen] = 0;
          pFilename = pbyTmp;
      } else
          pbyTmp = NULL;

      if( pipe( hPipeHandle ) == 0 ) {
         if( ( pid = fork() ) != -1 ) {
            if( pid != 0 ) {
               if( bRead ) {
                  close( hPipeHandle[ 1 ] );
                  hFileHandle = hPipeHandle[ 0 ];
               } else {
                  close( hPipeHandle[ 0 ] );
                  hFileHandle = hPipeHandle[ 1 ];
               }
            } else {
               char *argv[4];
               argv[0] = "sh";
               argv[1] = "-c";
               argv[2] = ( char * ) pFilename;
               argv[3] = ( char * ) 0;
               hNullHandle = open("/dev/null", O_RDWR);
               if( bRead ) {
                  close( hPipeHandle[ 0 ] );
                  dup2( hPipeHandle[ 1 ], 1 );
                  dup2( hNullHandle, 0 );
                  dup2( hNullHandle, 2 );
               } else {
                  close( hPipeHandle[ 1 ] );
                  dup2( hPipeHandle[ 0 ], 0 );
                  dup2( hNullHandle, 1 );
                  dup2( hNullHandle, 2 );
               }
               iMaxFD = sysconf( _SC_OPEN_MAX );
               if ( iMaxFD < 3 )
                  iMaxFD = 1024;
               for( hNullHandle = 3; hNullHandle < iMaxFD; ++hNullHandle )
                  close(hNullHandle);
               setuid(getuid());
               setgid(getgid());
               execve("/bin/sh", argv, environ);
               exit(1);
            }
         }
         else
         {
            close( hPipeHandle[0] );
            close( hPipeHandle[1] );
         }
      }
      hb_fsSetIOError( hFileHandle != FS_ERROR, 0 );

      if( pbyTmp )
      {
         hb_xfree( pbyTmp );
      }

      HB_STACK_LOCK;
   }
#else

   HB_SYMBOL_UNUSED( pFilename );
   HB_SYMBOL_UNUSED( pMode );

   hb_fsSetError( (USHORT) FS_ERROR );

#endif

   return hFileHandle;
}

#ifndef HB_WIN32_IO

int s_parametrize( char *out, char *in )
{
   int count = 0;  // we'll have at least one token

   // removes leading spaces
   while ( *in && isspace( (BYTE) *in) )
      in++;
   if (! *in ) return 0;

   while ( *in ) {
      if ( *in == '\"' || *in == '\'')
      {
         char quote = *in;
         in++;
         while ( *in && *in != quote ) {
            if ( *in == '\\' ) {
               in++;
            }
            if ( *in ) {
               *out = *in;
               out ++;
               in++;
            }
         }
         if (*in) {
            in++;
         }
         if ( *in ) {
            *out = '\0';
         }
         // out++ will be done later; if in is done,
         // '\0' will be added at loop exit.
      }
      else if (! isspace( (BYTE) *in ) ) {
         *out = *in;
         in++;
         out++;
      }
      else {
         *out = '\0';
         count ++;
         while (*in && isspace( (BYTE) *in ) )
            in++;
         out++;
      }
   }
   *out = '\0';
   count ++;

   return count;
}

char **s_argvize( char *params, int size )
{
   int i;
   char **argv = (char **) hb_xgrab( sizeof( char * ) * ( size + 1 ) );

   for( i = 0; i < size; i ++ )
   {
      argv[i] = params;
      while (*params) params++;
      params++;
   }
   return argv;
}

#endif

/*
JC1: Process Control functions
hb_fsOpenProcess creates a process and get the control of the 4 main
standard handlers. The handlers are returned in FHANDLE pointers;
each of them can be 0 if the owner process don't want to get
the ownership of that specific handler.

The process return a resource identificator that allows to
wait for the child process to be stopped. Incidentally, the
type of the process resource identificator is the same as
a file handle in all the systems were are using this functions:
in windows it is a HANDLE, while in unix is a PID_T (that is
an integer == file handle in all the GNU derived unces).

On success, a valid FHandle is returned, and FError returns
zero. On error, -1 is returned and FError() returns nonzero.
*/

FHANDLE HB_EXPORT hb_fsOpenProcess( char *pFilename,
      FHANDLE *fhStdin,
      FHANDLE *fhStdout,
      FHANDLE *fhStderr,
      BOOL bBackground,
      ULONG *ProcessID
      )
{
   FHANDLE hRet = FS_ERROR;
   HB_TRACE(HB_TR_DEBUG, ("hb_fsOpenProcess(%s, %p, %p, %p )",
      pFilename, fhStdin, fhStdout, fhStderr));

#if defined(OS_UNIX_COMPATIBLE) || ( defined( HB_OS_WIN_32 ) && ! defined( HB_WIN32_IO) ) || defined (HB_OS_OS2)
{
#ifndef MAXFD
   #define MAXFD       1024
#endif
   FHANDLE hPipeIn[2], hPipeOut[2], hPipeErr[2];
   FHANDLE hNull;
   char **argv;
   int size;
   char *command;

   #ifdef HB_OS_WIN_32
   int pid;
   #define pipe(x)   _pipe( x, 2048, _O_BINARY )
   #else
   pid_t pid;
   #endif

   if( fhStdin != 0 && pipe( hPipeIn ) != 0 )
   {
      hb_fsSetIOError( FALSE, 0 );
      return (FHANDLE) -1;
   #ifdef HB_OS_OS2
   } else {
      setmode( hPipeIn[0], O_BINARY );
      setmode( hPipeIn[1], O_BINARY );
   #endif
   }

   if( fhStdout != 0 && pipe( hPipeOut ) != 0 )
   {
      hb_fsSetIOError( FALSE, 0 );
      if ( fhStdin != 0 )
      {
         close( hPipeIn[0] );
         close( hPipeIn[1] );
      }
      return (FHANDLE) -1;
   #ifdef HB_OS_OS2
   } else {
      setmode( hPipeOut[0], O_BINARY );
      setmode( hPipeOut[1], O_BINARY );
   #endif
   }


   if( fhStderr != 0 )
   {
      if( fhStderr != fhStdout )
      {
         if ( pipe( hPipeErr ) != 0)
         {
            hb_fsSetIOError( FALSE, 0 );
            if ( fhStdin != 0 )
            {
               close( hPipeIn[0] );
               close( hPipeIn[1] );
            }
            if ( fhStdout != 0 )
            {
               close( hPipeOut[0] );
               close( hPipeOut[1] );
            }
            return (FHANDLE) -1;
         #ifdef HB_OS_OS2
         } else {
            setmode( hPipeErr[0], O_BINARY );
            setmode( hPipeErr[1], O_BINARY );
         #endif
         }
      }
   }


   #if defined(HB_OS_WIN_32) || defined(HB_OS_OS2)
   {
      int oldstdin, oldstdout, oldstderr;
      int iFlags;

      hNull = open("NUL:", O_RDWR);

      oldstdin = dup( 0 );
      oldstdout = dup( 1 );
      oldstderr = dup( 2 );

      if ( fhStdin != 0 )
      {
         dup2( hPipeIn[ 0 ], 0 );
      }
      else if ( bBackground )
      {
         dup2( hNull, 0 );
      }

      if ( fhStdout != 0 )
      {
         dup2( hPipeOut[ 1 ], 1 );
      }
      else if ( bBackground )
      {
         dup2( hNull, 1 );
      }

      if ( fhStderr != 0 )
      {
         if ( fhStderr != fhStdout )
         {
            dup2( hPipeErr[ 1 ], 2 );
         }
         else
         {
            dup2( hPipeOut[ 1 ], 2 );
         }
      }
      else if ( bBackground )
      {
         dup2( hNull, 2 );
      }

      command = ( char * )hb_xgrab( strlen(pFilename) + 2 );
      size = s_parametrize( command, pFilename );
      argv = s_argvize( command, size );
      argv[size] = 0;

      #if defined(__BORLANDC__) || defined(__WATCOMC__) || defined(__GNUC__)
      iFlags = P_NOWAIT;
      pid = spawnvp( iFlags, argv[0], argv );
      #else
      iFlags = _P_NOWAIT;
      pid = _spawnvp( iFlags, argv[0], argv );
      #endif

      #ifdef HB_OS_OS2
      *ProcessID = (ULONG) pid;
      #else
      *ProcessID = (DWORD) pid;
      #endif
      hb_xfree( command );
      hb_xfree( argv );

      dup2( oldstdin, 0 );
      dup2( oldstdout, 1 );
      dup2( oldstderr, 2 );
   }
   if ( pid < 0 )

   #else
   if( ( pid = fork() ) == -1 )
   #endif
   {
      hb_fsSetIOError( FALSE, 0 );
      // closing unused handles should be nice
      // TODO: check fs_Popen to close handles.
      if ( fhStdin != 0 )
      {
         close( hPipeIn[0] );
         close( hPipeIn[1] );
      }

      if ( fhStdout != 0 )
      {
         close( hPipeOut[0] );
         close( hPipeOut[1] );
      }

      if ( fhStderr != 0 && fhStderr != fhStdout )
      {
         close( hPipeErr[0] );
         close( hPipeErr[1] );
      }
      return (FHANDLE) -1;
   }

   if( pid != 0 ) {
      *ProcessID = (ULONG) pid;
      // I am the father
      if ( fhStdin != NULL )
      {
         *fhStdin = hPipeIn[1];
         close( hPipeIn[0] );
      }

      if ( fhStdout != NULL )
      {
         *fhStdout = hPipeOut[0];
         close( hPipeOut[1] );
      }

      if ( fhStderr != NULL && fhStderr != fhStdout )
      {
         *fhStderr = hPipeErr[0];
         close( hPipeErr[1] );
      }

      // father is done.
      hb_fsSetError( 0 );
      hRet = (FHANDLE) pid;

   }
   // I am che child
   #ifndef HB_OS_WIN_32
   else
   {
      command = ( char * ) hb_xgrab( strlen( pFilename ) + 2 );
      size = s_parametrize( command, pFilename );
      argv = s_argvize( command, size );
      argv[size] = NULL;

/*
      // temporary solution
      char *argv[4];
      argv[0] = "sh";
      argv[1] = "-c";
      argv[2] = ( char * ) pFilename;
      argv[3] = ( char * ) 0; */
      // drop uncontrolled streams

      /* Initialize hNull to make compiler happy ;-) */
      hNull = bBackground ? open("/dev/null", O_RDWR) : FS_ERROR;

      // does father wants to control us?
      if ( fhStdin != NULL )
      {
         close( hPipeIn[ 1 ] ); // we don't write to stdin
         dup2( hPipeIn[ 0 ], 0 );
      }
      else if ( bBackground )
      {
         dup2( hNull, 0 );
      }

      if ( fhStdout != NULL )
      {
         close( hPipeOut[0] );
         dup2( hPipeOut[ 1 ], 1 );
      }
      else if ( bBackground )
      {
         dup2( hNull, 1 );
      }

      if ( fhStderr != NULL )
      {
         if ( fhStdout != fhStderr )
         {
            close( hPipeErr[0] );
            dup2( hPipeErr[ 1 ], 2 );
         }
         else
         {
            dup2( 1 , 2 );
         }
      }
      else if ( bBackground )
      {
         dup2( hNull, 2 );
      }

      if ( bBackground )
      {
         close( hNull );
      }

      /*
      for( hNull = 3; hNull < MAXFD; ++hNull )
         close(hNull);
      */

      // ????
      /*
       * This disable SUID and SGID, I added it for security to hb_fsPOpen
       * Just simply program can work using seteuid()/setegid() to access
       * database file what can cause that users cannot access database
       * file directly - only from program. When you run external program
       * with setuid(geteuid())/setgid(getegid()) then it inherits UID and
       * GID so is able to operate with their privileges. If this external
       * program is called without absolute path (beginning from "/") then
       * user can set his own PATH variable or change directory before
       * running xHarbour binaries to take control over EUID/EGID resources
       * Take a decision is it's important - maybe it should be set
       * by a parameter? Druzus.
       */
      /*
      setuid(getuid());
      setgid(getgid());*/

      execvp(argv[0], argv );
   }
   #endif
}
#elif defined( HB_WIN32_IO )
{
   STARTUPINFO si;
   PROCESS_INFORMATION proc;
   ULONG ulSize;
   // int iSize;
   // DWORD iRet;
   DWORD iFlags=0;
   char fullCommand[1024], cmdName[256];
   char *completeCommand, *pos;
   char *filePart;
   SECURITY_ATTRIBUTES secatt;

   HANDLE hPipeInRd=INVALID_HANDLE_VALUE, hPipeInWr=INVALID_HANDLE_VALUE;
   HANDLE hPipeOutRd=INVALID_HANDLE_VALUE, hPipeOutWr=INVALID_HANDLE_VALUE;
   HANDLE hPipeErrRd=INVALID_HANDLE_VALUE, hPipeErrWr=INVALID_HANDLE_VALUE;

   // prepare security attributes
   secatt.nLength = sizeof( secatt );
   secatt.lpSecurityDescriptor = NULL;
   secatt.bInheritHandle = TRUE;

   hb_fsSetError( 0 ); // reset error

   if ( fhStdin != NULL )
   {
      if ( !CreatePipe( &hPipeInRd, &hPipeInWr, &secatt, 0 ) )
      {
         hb_fsSetIOError( FALSE, 0 );
         return FS_ERROR;
      }
   }

   if ( fhStdout != NULL )
   {
      if ( !CreatePipe( &hPipeOutRd, &hPipeOutWr, &secatt, 0 ) )
      {
         hb_fsSetIOError( FALSE, 0 );
         hRet = FS_ERROR;
         goto ret_close_1;
      }
   }

   if ( fhStderr != NULL )
   {
      if ( fhStderr == fhStdout )
      {
         hPipeErrRd = hPipeOutRd;
         hPipeErrWr = hPipeOutWr;
      }

      if ( !CreatePipe( &hPipeErrRd, &hPipeErrWr, &secatt, 0 ) )
      {
         hb_fsSetIOError( FALSE, 0 );
         hRet = FS_ERROR;
         goto ret_close_2;
      }
   }

   // parameters are included in the command string
   pos = (char *) pFilename;
   while( *pos && *pos != ' ' && *pos != '\\' )
   {
      pos++;
   }

   ulSize = (unsigned) (pos - (char *)pFilename );
   if ( ulSize > 254 || *pos == '\\' )
   {
      // absolute path. We are ok
      strncpy( fullCommand, pFilename, 1023);
      fullCommand[1023] = '\0';
   }
   else
   {
      memcpy( cmdName, pFilename, ulSize );
      cmdName[ulSize+1] = 0;
      // find the command in the path
      if ( ! SearchPath( NULL, cmdName, NULL, 1024, fullCommand, &filePart ) )
      {
         strcpy( fullCommand, cmdName );
      }

   }

   if ( *pos && *pos != '\\')
   {
      completeCommand = (char *) hb_xgrab( strlen( fullCommand ) + strlen( pos ) +2);
      sprintf( completeCommand, "%s %s",  fullCommand, pos+1);
   }
   else
   {
      completeCommand = (char *) hb_xgrab( strlen( fullCommand ) + 1);
      strcpy( completeCommand, fullCommand);
   }

   memset( &si, 0, sizeof( si ) );
   si.cb = sizeof( si );

   if ( bBackground )
   {
      // using show_hide AND using invalid handlers for unused streams
      si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
      si.wShowWindow = SW_HIDE;

      si.hStdInput = hPipeInRd;
      si.hStdOutput = hPipeOutWr;
      si.hStdError = hPipeErrWr;

      iFlags |= DETACHED_PROCESS;
   }
   else
   {
      si.dwFlags = STARTF_USESTDHANDLES;

      if ( fhStdin != NULL )
      {
         si.hStdInput = hPipeInRd;
      }
      else
      {
         si.hStdInput = GetStdHandle( STD_INPUT_HANDLE );
      }

      if ( fhStdout != NULL )
      {
         si.hStdOutput = hPipeOutWr;
      }
      else
      {
         si.hStdOutput = GetStdHandle( STD_OUTPUT_HANDLE );
      }

      if ( fhStderr != NULL )
      {
         si.hStdError = hPipeErrWr;
      }
      else
      {
         si.hStdError = GetStdHandle( STD_ERROR_HANDLE );
      }
   }

   if ( !CreateProcess( NULL,
      completeCommand,
      NULL,
      NULL,
      TRUE, //Inerhit handles!
      iFlags,
      NULL,
      NULL,
      &si,
      &proc
      ) )
   {
      hb_fsSetIOError( FALSE, 0 );
      hRet = FS_ERROR;
      hb_xfree( completeCommand );
      goto ret_close_3;
   }
   else
   {
      hb_xfree( completeCommand );
      hb_fsSetError( 0 );
      hRet = HandleToLong( proc.hProcess );
      *ProcessID = proc.dwProcessId;

      if ( fhStdin != NULL )
      {
         *fhStdin = HandleToLong( hPipeInWr );
         CloseHandle( hPipeInRd );
      }
      if ( fhStdout != NULL )
      {
         *fhStdout = HandleToLong( hPipeOutRd );
         CloseHandle( hPipeOutWr );
      }
      if ( fhStderr != NULL )
      {
         *fhStderr = HandleToLong( hPipeErrRd );
         CloseHandle( hPipeErrWr );
      }

      CloseHandle( proc.hThread ); // unused
   }

   return hRet;

ret_close_3:
   if ( hPipeErrRd != INVALID_HANDLE_VALUE )
   {
      CloseHandle( hPipeErrRd );
   }
   if ( hPipeErrWr != INVALID_HANDLE_VALUE )
   {
      CloseHandle( hPipeErrWr );
   }

ret_close_2:
   if ( hPipeOutRd != INVALID_HANDLE_VALUE )
   {
      CloseHandle( hPipeOutRd );
   }
   if ( hPipeOutWr != INVALID_HANDLE_VALUE )
   {
      CloseHandle( hPipeOutWr );
   }

ret_close_1:
   if ( hPipeInRd != INVALID_HANDLE_VALUE )
   {
      CloseHandle( hPipeInRd );
   }
   if ( hPipeInWr != INVALID_HANDLE_VALUE )
   {
      CloseHandle( hPipeInWr );
   }
}

#else

   HB_SYMBOL_UNUSED( pFilename );
   HB_SYMBOL_UNUSED( fhStdin );
   HB_SYMBOL_UNUSED( fhStdout );
   HB_SYMBOL_UNUSED( fhStderr );
   HB_SYMBOL_UNUSED( bBackground );
   HB_SYMBOL_UNUSED( ProcessID );

   hb_fsSetError( FS_ERROR );

#endif

   return hRet;
}

/*
   See if a process is still being executed. If bWait is true,
   the function will wait for the process to finish  before
   to return. When the process is terminated
   with a signal, the signal is returned as -signum. Notice that
   this does not apply to Windows.
*/

int HB_EXPORT hb_fsProcessValue( FHANDLE fhProc, BOOL bWait )
{
   HB_THREAD_STUB
   int iRetStatus = -1;

   HB_TRACE(HB_TR_DEBUG, ("hb_fsProcessValue(%d, %d )", fhProc, bWait));

   hb_fsSetError( 0 );

#if defined(OS_UNIX_COMPATIBLE) || defined(HB_OS_OS2)
{
   int iStatus;

   if ( fhProc > 0 )
   {
      if ( ! bWait )
      {
         iRetStatus = waitpid( (pid_t) fhProc, &iStatus, WNOHANG );
      }
      else
      {
         HB_STACK_UNLOCK;
         iRetStatus = waitpid( (pid_t) fhProc, &iStatus, 0 );
         HB_STACK_LOCK;
      }
   }
   else
   {
      iRetStatus = 0;
   }

#ifdef ERESTARTSYS
   if ( iRetStatus < 0 && errno != ERESTARTSYS)
#else
   if ( iRetStatus < 0 )
#endif
   {
      hb_fsSetIOError( FALSE, 0 );
      iRetStatus = -2;
   }
   else if ( iRetStatus == 0 )
   {
      iRetStatus = -1;
   }
   else
   {
      if( WIFEXITED( iStatus ) )
      {
         iRetStatus = WEXITSTATUS( iStatus );
      }
      else
      {
         iRetStatus = 0;
      }
   }
}
#elif defined( HB_OS_WIN_32 ) && ! defined( HB_WIN32_IO )
{
   int iPid;

   HB_SYMBOL_UNUSED( bWait );

   HB_STACK_UNLOCK
   HB_TEST_CANCEL_ENABLE_ASYN
   #ifdef __BORLANDC__
      iPid = cwait( &iRetStatus, (int) fhProc, 0 );
   #else
      iPid = _cwait( &iRetStatus, (int) fhProc, 0 );
   #endif
   HB_DISABLE_ASYN_CANC
   HB_STACK_LOCK;

   if ( iPid != (int) fhProc )
   {
      iRetStatus = -1;
   }
}
#elif defined( HB_WIN32_IO )
{
   DWORD dwTime;
   DWORD dwResult;

   if ( ! bWait )
   {
      dwTime = 0;
   }
   else
   {
      dwTime = INFINITE;
   }

   HB_STACK_UNLOCK
   HB_TEST_CANCEL_ENABLE_ASYN
   dwResult = WaitForSingleObject( DostoWinHandle(fhProc), dwTime );
   HB_DISABLE_ASYN_CANC
   HB_STACK_LOCK;

   if ( dwResult == WAIT_OBJECT_0 )
   {
      BOOL fResult = GetExitCodeProcess( DostoWinHandle(fhProc), &dwResult );

      hb_fsSetIOError( fResult, 0 );
      iRetStatus = fResult ? (int) dwResult : -2;
   }
   else
   {
      iRetStatus = -1;
   }
}
#else

   HB_SYMBOL_UNUSED( fhProc );
   HB_SYMBOL_UNUSED( bWait );

   hb_fsSetError( FS_ERROR );

#endif

   return iRetStatus;
}

/*
   Closes a process (that is, kill the application running the
   process); the handle is still valid until you
   catch it with hb_fsProcessValue. If bGentle is true, then
   a kind termination request is sent to the process, else
   the process is just killed.
   Retiurn
*/

BOOL HB_EXPORT hb_fsCloseProcess( FHANDLE fhProc, BOOL bGentle )
{
   BOOL bRet = FALSE;
   HB_TRACE(HB_TR_DEBUG, ("hb_fsCloseProcess(%d, %d )", fhProc, bGentle));

#if defined(OS_UNIX_COMPATIBLE) || defined(HB_OS_OS2)
   if ( fhProc > 0 )
   {
      int iSignal = bGentle ? SIGTERM : SIGKILL;
      bRet = (kill( (pid_t) fhProc, iSignal ) == 0);
      hb_fsSetIOError( bRet, 0 );
   }
   else
   {
      hb_fsSetError( FS_ERROR );
   }
#elif defined( HB_WIN32_IO )
   bRet = (TerminateProcess( DostoWinHandle( fhProc ), bGentle ? 0:1 ) != 0);
   hb_fsSetIOError( bRet, 0 );
#elif defined( HB_OS_WIN_32 )
{
   HANDLE hProc;

   hProc = OpenProcess( PROCESS_TERMINATE, FALSE, fhProc );

   if ( hProc != NULL )
   {
      bRet = (TerminateProcess( hProc, bGentle ? 0:1 ) != 0);
   }
   hb_fsSetIOError( bRet, 0 );
}
#else

   HB_SYMBOL_UNUSED( fhProc );
   HB_SYMBOL_UNUSED( bGentle );
   hb_fsSetError( FS_ERROR );

#endif
   return bRet;
}


FHANDLE HB_EXPORT hb_fsOpen( BYTE * pFilename, USHORT uiFlags )
{
   HB_THREAD_STUB

   FHANDLE hFileHandle;

   HB_TRACE(HB_TR_DEBUG, ("hb_fsOpen(%p, %hu)", pFilename, uiFlags));

   pFilename = (BYTE*)hb_fileNameConv( hb_strdup( ( char * )pFilename) );

   // Unlocking stack to allow cancelation points
   HB_STACK_UNLOCK
   // allowing async cancelation here
   HB_TEST_CANCEL_ENABLE_ASYN

#if defined(HB_WIN32_IO)
   {
      DWORD dwMode, dwShare, dwAttr;
      HANDLE hFile;

      convert_create_flags( FALSE, 0, uiFlags, &dwMode, &dwShare, &dwAttr );

      hFile = ( HANDLE ) CreateFile( ( char * ) pFilename, dwMode, dwShare,
                                     NULL, OPEN_EXISTING, dwAttr, NULL );

      hb_fsSetIOError( hFile != ( HANDLE ) INVALID_HANDLE_VALUE, 0 );

      hFileHandle = HandleToLong(hFile);
   }
#elif defined(_MSC_VER)
   {
      int iShare = _SH_DENYNO;

      if( ( uiFlags & FO_DENYREAD ) == FO_DENYREAD )
      {
         iShare = _SH_DENYRD;
      }
      else if( uiFlags & FO_EXCLUSIVE )
      {
         iShare = _SH_DENYRW;
      }
      else if( uiFlags & FO_DENYWRITE )
      {
         iShare = _SH_DENYWR;
      }

      if( iShare )
      {
         hFileHandle = _sopen( ( char * ) pFilename, convert_open_flags( uiFlags ), iShare );
      }
      else
      {
         hFileHandle = _open( ( char * ) pFilename, convert_open_flags( uiFlags ) );
      }
      hb_fsSetIOError( hFileHandle != FS_ERROR, 0 );
   }
#elif defined(HB_FS_SOPEN)
   {
      int iShare = SH_DENYNO;

      if( ( uiFlags & FO_DENYREAD ) == FO_DENYREAD )
      {
         iShare = SH_DENYRD;
      }
      else if( uiFlags & FO_EXCLUSIVE )
      {
         iShare = SH_DENYRW;
      }
      else if( uiFlags & FO_DENYWRITE )
      {
         iShare = SH_DENYWR;
      }

      if( iShare )
      {
         hFileHandle = sopen( ( char * ) pFilename, convert_open_flags( uiFlags ), iShare );
      }
      else
      {
         hFileHandle = open( ( char * ) pFilename, convert_open_flags( uiFlags ) );
      }
      hb_fsSetIOError( hFileHandle != FS_ERROR, 0 );
   }
#elif defined(HAVE_POSIX_IO)

   hFileHandle = open( ( char * ) pFilename, convert_open_flags( uiFlags ),
                                             convert_pmode_flags( 0 ) );
   hb_fsSetIOError( hFileHandle != FS_ERROR, 0 );

#else

   hFileHandle = FS_ERROR;
   hb_fsSetError( FS_ERROR );

#endif

   HB_DISABLE_ASYN_CANC
   HB_STACK_LOCK

   hb_xfree( pFilename );

   return hFileHandle;
}

FHANDLE HB_EXPORT hb_fsCreate( BYTE * pFilename, USHORT uiAttr )
{
   HB_THREAD_STUB
   FHANDLE hFileHandle;

   #if defined(HB_FS_FILE_IO) && !defined(HB_WIN32_IO)
      int oflag;
      unsigned pmode;
   #endif

   HB_TRACE(HB_TR_DEBUG, ("hb_fsCreate(%p, %hu)", pFilename, uiAttr));

   pFilename = (BYTE*)hb_fileNameConv( hb_strdup( ( char * ) pFilename ) );

   HB_STACK_UNLOCK
   // allowing async cancelation here
   HB_TEST_CANCEL_ENABLE_ASYN

#if defined(HB_WIN32_IO)
   {
      DWORD dwMode, dwShare, dwAttr;
      HANDLE hFile;

      convert_create_flags( TRUE, uiAttr, FO_EXCLUSIVE, &dwMode, &dwShare, &dwAttr );

      hFile = ( HANDLE ) CreateFile( ( char * ) pFilename, dwMode, dwShare,
                                     NULL, CREATE_ALWAYS, dwAttr, NULL );

      hb_fsSetIOError( hFile != ( HANDLE ) INVALID_HANDLE_VALUE, 0 );

      hFileHandle = HandleToLong(hFile);
   }
#elif defined(HB_FS_FILE_IO)

   convert_create_flags( uiAttr, &oflag, &pmode );

   hFileHandle = open( ( char * ) pFilename, oflag, pmode );
   hb_fsSetIOError( hFileHandle != FS_ERROR, 0 );

#else

   hFileHandle = FS_ERROR;
   hb_fsSetError( FS_ERROR );

#endif

   HB_DISABLE_ASYN_CANC
   HB_STACK_LOCK

   hb_xfree( pFilename );

   return hFileHandle;
}

/* Derived from hb_fsCreate()

   NOTE: The default opening mode differs from the one used in hb_fsCreate()
         [vszakats]
 */

FHANDLE HB_EXPORT hb_fsCreateEx( BYTE * pFilename, USHORT uiAttr, USHORT uiFlags )
{
   HB_THREAD_STUB
   FHANDLE hFileHandle;

   HB_TRACE(HB_TR_DEBUG, ("hb_fsCreateEx(%p, %hu, %hu)", pFilename, uiAttr, uiFlags));

   pFilename = hb_filecase( hb_strdup( ( char * ) pFilename ) );

   HB_STACK_UNLOCK
   // allowing async cancelation here
   HB_TEST_CANCEL_ENABLE_ASYN

#if defined( HB_WIN32_IO )
   {
      DWORD dwMode, dwShare, dwAttr;
      HANDLE hFile;

      convert_create_flags( TRUE, uiAttr, uiFlags, &dwMode, &dwShare, &dwAttr );

      hFile = ( HANDLE ) CreateFile( ( char * ) pFilename, dwMode, dwShare,
                                     NULL, CREATE_ALWAYS, dwAttr, NULL );

      hb_fsSetIOError( hFile != ( HANDLE ) INVALID_HANDLE_VALUE, 0 );

      hFileHandle = HandleToLong(hFile);
   }
#elif defined(HB_FS_FILE_IO)
   {
      int oflag;
      unsigned pmode;

      convert_create_flags_ex( uiAttr, uiFlags, &oflag, &pmode );

      hFileHandle = open( ( char * ) pFilename, oflag, pmode );
      hb_fsSetIOError( hFileHandle != FS_ERROR, 0 );
   }
#else

   hFileHandle = FS_ERROR;
   hb_fsSetError( FS_ERROR );

#endif

   HB_DISABLE_ASYN_CANC
   HB_STACK_LOCK

   hb_xfree( pFilename );
   return hFileHandle;
}

void    HB_EXPORT hb_fsClose( FHANDLE hFileHandle )
{
   HB_THREAD_STUB
   HB_TRACE(HB_TR_DEBUG, ("hb_fsClose(%p)", hFileHandle));


#if defined(HB_FS_FILE_IO)

   HB_STACK_UNLOCK
   // allowing async cancelation here
   HB_TEST_CANCEL_ENABLE_ASYN

   #if defined(HB_WIN32_IO)
      hb_fsSetIOError( CloseHandle( DostoWinHandle( hFileHandle ) ), 0 );
   #else
      hb_fsSetIOError( close( hFileHandle ) == 0, 0 );
   #endif

   HB_DISABLE_ASYN_CANC
   HB_STACK_LOCK

#else

   hb_fsSetError( FS_ERROR );

#endif
}

void    HB_EXPORT hb_fsSetDevMode( FHANDLE hFileHandle, USHORT uiDevMode )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_fsSetDevMode(%p, %hu)", hFileHandle, uiDevMode));

   /* TODO: HB_WIN32_IO support */

#if defined(__BORLANDC__) || defined(__IBMCPP__) || defined(__DJGPP__) || defined(__CYGWIN__) || defined(__WATCOMC__) || defined(HB_OS_OS2)
{
   int iRet = 0;

   switch( uiDevMode )
   {
      case FD_BINARY:
         iRet = setmode( hFileHandle, O_BINARY );
         break;

      case FD_TEXT:
         iRet = setmode( hFileHandle, O_TEXT );
         break;
   }
   hb_fsSetIOError( iRet != -1, 0 );
}
#elif defined(_MSC_VER) || defined(__MINGW32__)
{
   int iRet = 0;

   switch( uiDevMode )
   {
      case FD_BINARY:
         iRet = _setmode( hFileHandle, _O_BINARY );
         break;

      case FD_TEXT:
         iRet = _setmode( hFileHandle, _O_TEXT );
         break;
   }
   hb_fsSetIOError( iRet != -1, 0 );
}
#elif defined( HB_OS_UNIX )

   HB_SYMBOL_UNUSED( hFileHandle );
   HB_SYMBOL_UNUSED( uiDevMode );
   hb_fsSetError( FS_ERROR );

#else

   hb_fsSetError( FS_ERROR );

#endif

}

USHORT  HB_EXPORT hb_fsRead( FHANDLE hFileHandle, BYTE * pBuff, USHORT uiCount )
{
   HB_THREAD_STUB
   USHORT uiRead;

   HB_TRACE(HB_TR_DEBUG, ("hb_fsRead(%p, %p, %hu)", hFileHandle, pBuff, uiCount));

#if defined(HB_FS_FILE_IO)

   HB_STACK_UNLOCK

   #if defined(HB_WIN32_IO)
      {
         DWORD dwRead ;
         BOOL fResult;
         // allowing async cancelation here
         HB_TEST_CANCEL_ENABLE_ASYN

         fResult = ReadFile( DostoWinHandle(hFileHandle), pBuff, (DWORD)uiCount, &dwRead, NULL );
         hb_fsSetIOError( fResult, 0 );

         HB_DISABLE_ASYN_CANC

         uiRead = fResult ? ( USHORT ) dwRead : 0;
      }
   #else
      // allowing async cancelation here
      HB_TEST_CANCEL_ENABLE_ASYN
      uiRead = read( hFileHandle, pBuff, uiCount );
      hb_fsSetIOError( uiRead != ( USHORT ) -1, 0 );
      HB_DISABLE_ASYN_CANC
   #endif

   if( uiRead == ( USHORT ) -1 )
      uiRead = 0;

   HB_STACK_LOCK
#else

   uiRead = 0;
   hb_fsSetError( FS_ERROR );

#endif

   return uiRead;
}

USHORT  HB_EXPORT hb_fsWrite( FHANDLE hFileHandle, BYTE * pBuff, USHORT uiCount )
{
   HB_THREAD_STUB
   USHORT uiWritten;

   HB_TRACE(HB_TR_DEBUG, ("hb_fsWrite(%p, %p, %hu)", hFileHandle, pBuff, uiCount));

#if defined(HB_FS_FILE_IO)

   HB_STACK_UNLOCK

   #if defined(HB_WIN32_IO)
      {
         DWORD dwWritten = 0;
         BOOL fResult;
         // allowing async cancelation here
         HB_TEST_CANCEL_ENABLE_ASYN

         if ( uiCount )
         {
             fResult = WriteFile( DostoWinHandle(hFileHandle), pBuff, uiCount, &dwWritten, NULL );
         }
         else
         {
             dwWritten = 0;
             fResult = SetEndOfFile( DostoWinHandle(hFileHandle) );
         }
         hb_fsSetIOError( fResult, 0 );

         HB_DISABLE_ASYN_CANC

         uiWritten = fResult ? ( USHORT ) dwWritten : 0;
      }
   #else

      // allowing async cancelation here
      HB_TEST_CANCEL_ENABLE_ASYN

      if( uiCount )
      {
         uiWritten = write( hFileHandle, pBuff, uiCount );
         hb_fsSetIOError( uiWritten != ( USHORT ) -1, 0 );
         if( uiWritten == ( USHORT ) -1 )
            uiWritten = 0;
      }
      else
      {
#if defined(HB_OS_LINUX) && defined(__USE_LARGEFILE64)
         /*
          * The macro: __USE_LARGEFILE64 is set when _LARGEFILE64_SOURCE is
          * define and efectively enables lseek64/flock64/ftruncate64 functions
          * on 32bit machines.
          */
         hb_fsSetIOError( ftruncate64( hFileHandle, lseek64( hFileHandle, 0L, SEEK_CUR ) ) != -1, 0 );
#else
         hb_fsSetIOError( ftruncate( hFileHandle, lseek( hFileHandle, 0L, SEEK_CUR ) ) != -1, 0 );
#endif
         uiWritten = 0;
      }

      HB_DISABLE_ASYN_CANC

   #endif

   HB_STACK_LOCK

#else

   uiWritten = 0;
   hb_fsSetError( FS_ERROR );

#endif

   return uiWritten;
}

ULONG   HB_EXPORT hb_fsReadLarge( FHANDLE hFileHandle, BYTE * pBuff, ULONG ulCount )
{
   HB_THREAD_STUB
   ULONG ulRead;

   HB_TRACE(HB_TR_DEBUG, ("hb_fsReadLarge(%p, %p, %lu)", hFileHandle, pBuff, ulCount));

#if defined(HB_FS_FILE_IO)

   HB_STACK_UNLOCK

   #if defined(HB_WIN32_IO)
   {
      // allowing async cancelation here
      HB_TEST_CANCEL_ENABLE_ASYN

      hb_fsSetIOError( ReadFile( DostoWinHandle(hFileHandle),
                                 pBuff, ulCount, &ulRead, NULL ), 0 );

      HB_DISABLE_ASYN_CANC
   }
   #elif defined(HB_FS_LARGE_OPTIMIZED)
   {
      // allowing async cancelation here
      HB_TEST_CANCEL_ENABLE_ASYN
      ulRead = read( hFileHandle, pBuff, ulCount );
      hb_fsSetIOError( ulRead != (ULONG) -1, 0 );
      HB_DISABLE_ASYN_CANC
   }
   #else
   {
      ULONG ulLeftToRead = ulCount;
      USHORT uiToRead;
      USHORT uiRead;
      BYTE * pPtr = pBuff;

      ulRead = 0;

      HB_TEST_CANCEL_ENABLE_ASYN
      while( ulLeftToRead )
      {
         /* Determine how much to read this time */
         if( ulLeftToRead > ( ULONG ) INT_MAX )
         {
            uiToRead = INT_MAX;
            ulLeftToRead -= ( ULONG ) uiToRead;
         }
         else
         {
            uiToRead = ( USHORT ) ulLeftToRead;
            ulLeftToRead = 0;
         }

         // allowing async cancelation here

         uiRead = read( hFileHandle, pPtr, uiToRead );
         /* -1 on bad hFileHandle
             0 on disk full
          */

         if( uiRead == 0 )
            break;

         if( uiRead == ( USHORT ) -1 )
         {
            uiRead = 0;
            break;
         }

         ulRead += ( ULONG ) uiRead;
         pPtr += uiRead;
      }
      hb_fsSetIOError( ulLeftToRead == 0, 0 );
      HB_DISABLE_ASYN_CANC
   }
   #endif

   HB_STACK_LOCK

#else

   ulRead = 0;
   hb_fsSetError( FS_ERROR );

#endif

   return ulRead;
}

ULONG   HB_EXPORT hb_fsWriteLarge( FHANDLE hFileHandle, BYTE * pBuff, ULONG ulCount )
{
   HB_THREAD_STUB
   ULONG ulWritten;

   HB_TRACE(HB_TR_DEBUG, ("hb_fsWriteLarge(%p, %p, %lu)", hFileHandle, pBuff, ulCount));

#if defined(HB_FS_FILE_IO)

   #if defined(HB_WIN32_IO)
   {
      HB_STACK_UNLOCK

      ulWritten = 0;
      if( ulCount )
      {
         hb_fsSetIOError( WriteFile( DostoWinHandle( hFileHandle), pBuff, ulCount, &ulWritten, NULL ), 0 );
      }
      else
      {
         hb_fsSetIOError( SetEndOfFile( DostoWinHandle(hFileHandle) ), 0 );
      }

      HB_STACK_LOCK
   }
   #else
      HB_STACK_UNLOCK
      // allowing async cancelation here
      HB_TEST_CANCEL_ENABLE_ASYN

      if( ulCount )
      #if defined(HB_FS_LARGE_OPTIMIZED)
         {
            ulWritten = write( hFileHandle, pBuff, ulCount );
            hb_fsSetIOError( ulWritten != ( ULONG ) -1, 0 );
            if( ulWritten == ( ULONG ) -1 )
               ulWritten = 0;
         }
      #else
         {
            ULONG ulLeftToWrite = ulCount;
            USHORT uiToWrite;
            USHORT uiWritten;
            BYTE * pPtr = pBuff;

            ulWritten = 0;

            while( ulLeftToWrite )
            {
               /* Determine how much to write this time */
               if( ulLeftToWrite > ( ULONG ) INT_MAX )
               {
                  uiToWrite = INT_MAX;
                  ulLeftToWrite -= ( ULONG ) uiToWrite;
               }
               else
               {
                  uiToWrite = ( USHORT ) ulLeftToWrite;
                  ulLeftToWrite = 0;
               }

               uiWritten = write( hFileHandle, pPtr, uiToWrite );

               /* -1 on bad hFileHandle
                   0 on disk full
                */

               if( uiWritten == 0 )
                  break;

               if( uiWritten == ( USHORT ) -1 )
               {
                  uiWritten = 0;
                  break;
               }

               ulWritten += ( ULONG ) uiWritten;
               pPtr += uiWritten;
            }
            hb_fsSetIOError( ulLeftToWrite == 0, 0 );
         }
      #endif
      else
      {
#if defined(HB_OS_LINUX) && defined(__USE_LARGEFILE64)
         /*
          * The macro: __USE_LARGEFILE64 is set when _LARGEFILE64_SOURCE is
          * define and efectively enables lseek64/flock64/ftruncate64 functions
          * on 32bit machines.
          */
         hb_fsSetIOError( ftruncate64( hFileHandle, lseek64( hFileHandle, 0L, SEEK_CUR ) ) != -1, 0 );
#else
         hb_fsSetIOError( ftruncate( hFileHandle, lseek( hFileHandle, 0L, SEEK_CUR ) ) != -1, 0 );
#endif
         ulWritten = 0;
      }

      HB_DISABLE_ASYN_CANC
      HB_STACK_LOCK

   #endif

#else

   hb_fsSetError( FS_ERROR );

#endif

   return ulWritten;
}

void HB_EXPORT    hb_fsCommit( FHANDLE hFileHandle )
{
   HB_THREAD_STUB
   HB_TRACE(HB_TR_DEBUG, ("hb_fsCommit(%p)", hFileHandle));

   HB_STACK_UNLOCK

#if defined(HB_OS_WIN_32)
   {
      // allowing async cancelation here
      HB_TEST_CANCEL_ENABLE_ASYN
      #if defined(HB_WIN32_IO)
         hb_fsSetIOError( FlushFileBuffers( ( HANDLE ) DostoWinHandle( hFileHandle ) ), 0 );
      #else
         #if defined(__WATCOMC__)
            hb_fsSetIOError( fsync( hFileHandle ) == 0, 0 );
         #else
            hb_fsSetIOError( _commit( hFileHandle ) == 0, 0 );
         #endif
      #endif
      HB_DISABLE_ASYN_CANC
   }

#elif defined(HB_OS_OS2)

   {
      errno = 0;
      /* TODO: what about error code from DosResetBuffer() call? */
      DosResetBuffer( hFileHandle );
      hb_fsSetIOError( errno == 0, 0 );
   }

#elif defined(HB_OS_UNIX)

   /* NOTE: close() functions releases all lock regardles if it is an
    * original or duplicated file handle
   */
   #if defined(_POSIX_SYNCHRONIZED_IO)
      /* faster - flushes data buffers only, without updating directory info
      */
      hb_fsSetIOError( fdatasync( hFileHandle ) == 0, 0 );
   #else
      /* slower - flushes all file data buffers and i-node info
      */
      hb_fsSetIOError( fsync( hFileHandle ) == 0, 0 );
   #endif

#elif defined(__WATCOMC__)

   hb_fsSetIOError( fsync( hFileHandle ) == 0, 0 );

#elif defined(HB_FS_FILE_IO) && !defined(HB_OS_OS2) && !defined(HB_OS_UNIX)

   /* This hack is very dangerous. POSIX standard define that if _ANY_
      file handle is closed all locks set by the process on the file
      pointed by this descriptor are removed. It doesn't matter they
      were done using different descriptor. It means that we now clean
      all locks on hFileHandle with the code below if the OS is POSIX
      compilant. I vote to disable it.
    */
   {
      int dup_handle;
      BOOL fResult = FALSE;

      dup_handle = dup( hFileHandle );
      if( dup_handle != -1 )
      {
         close( dup_handle );
         fResult = TRUE;
      }
      hb_fsSetIOError( fResult, 0 );
   }

#else

   hb_fsSetError( FS_ERROR );

#endif

   HB_STACK_LOCK
}

BOOL HB_EXPORT    hb_fsLock   ( FHANDLE hFileHandle, ULONG ulStart,
                                ULONG ulLength, USHORT uiMode )
{
   HB_THREAD_STUB
   BOOL bResult;

   HB_TRACE(HB_TR_DEBUG, ("hb_fsLock(%p, %lu, %lu, %hu)", hFileHandle, ulStart, ulLength, uiMode));

   HB_STACK_UNLOCK

#if defined(HB_WIN32_IO)

   // allowing async cancelation here
   HB_TEST_CANCEL_ENABLE_ASYN
   {
     static BOOL s_bInit = 0, s_bWinNt ;
     if ( !s_bInit )
     {
        s_bInit = TRUE ;
        s_bWinNt = hb_iswinnt() ;

     }
     switch( uiMode & FL_MASK )
     {
        case FL_LOCK:
        {
           if ( s_bWinNt )
           {
              OVERLAPPED sOlap ;
              DWORD dwFlags ;
              memset( &sOlap, 0, sizeof( OVERLAPPED ) ) ;
              sOlap.Offset = ( ULONG ) ulStart ;
              dwFlags = ( uiMode & FLX_SHARED ) ? 0 : LOCKFILE_EXCLUSIVE_LOCK ;
              if ( !( uiMode & FLX_WAIT ) )
              {
                 dwFlags |= LOCKFILE_FAIL_IMMEDIATELY ;
              }
              bResult = LockFileEx( DostoWinHandle( hFileHandle ), dwFlags, 0, ulLength, 0, &sOlap );
           }
           else
           {
               bResult = LockFile( DostoWinHandle( hFileHandle ), ulStart, 0, ulLength,0 );
           }
           break;
        }
        case FL_UNLOCK:
        {
           if ( s_bWinNt )
           {
              OVERLAPPED sOlap ;
              memset( &sOlap, 0, sizeof( OVERLAPPED ) ) ;
              sOlap.Offset = ( ULONG ) ulStart ;
              bResult = UnlockFileEx( DostoWinHandle( hFileHandle ), 0, ulLength,0, &sOlap );
           }
           else
           {
              bResult = UnlockFile( DostoWinHandle( hFileHandle ), ulStart, 0, ulLength,0 );
           }
           break;

        }
        default:
           bResult = FALSE;
     }
   }
   hb_fsSetIOError( bResult, 0 );
   HB_DISABLE_ASYN_CANC

#elif defined(HB_OS_OS2)
   {
      struct _FILELOCK fl, ful;

      switch( uiMode & FL_MASK )
      {
      case FL_LOCK:

         fl.lOffset = ulStart;
         fl.lRange = ulLength;
         ful.lOffset = 0;
         ful.lRange = 0;

         /* lock region, 2 seconds timeout, exclusive access - no atomic */
         bResult = ( DosSetFileLocks( hFileHandle, &ful, &fl, 2000L, 0L ) == 0 );
         break;

      case FL_UNLOCK:

         fl.lOffset = 0;
         fl.lRange = 0;
         ful.lOffset = ulStart;
         ful.lRange = ulLength;

         /* unlock region, 2 seconds timeout, exclusive access - no atomic */
         bResult = ( DosSetFileLocks( hFileHandle, &ful, &fl, 2000L, 0L ) == 0 );
         break;

      default:
         bResult = FALSE;
      }
      hb_fsSetIOError( bResult, 0 );
   }
#elif defined(_MSC_VER)
   {
      ULONG ulOldPos = lseek( hFileHandle, 0L, SEEK_CUR );

      // allowing async cancelation here
      HB_TEST_CANCEL_ENABLE_ASYN

      lseek( hFileHandle, ulStart, SEEK_SET );
      switch( uiMode & FL_MASK )
      {
         case FL_LOCK:
            bResult = ( locking( hFileHandle, _LK_NBLCK, ulLength ) == 0 );
            break;

         case FL_UNLOCK:
            bResult = ( locking( hFileHandle, _LK_UNLCK, ulLength ) == 0 );
            break;

         default:
            bResult = FALSE;
      }
      hb_fsSetIOError( bResult, 0 );
      lseek( hFileHandle, ulOldPos, SEEK_SET );
      HB_DISABLE_ASYN_CANC
   }
#elif defined(__MINGW32__)
   {
      ULONG ulOldPos = lseek( hFileHandle, 0L, SEEK_CUR );

      // allowing async cancelation here
      HB_TEST_CANCEL_ENABLE_ASYN

      lseek( hFileHandle, ulStart, SEEK_SET );
      switch( uiMode & FL_MASK )
      {
         case FL_LOCK:
            bResult = ( _locking( hFileHandle, _LK_LOCK, ulLength ) == 0 );
            break;

         case FL_UNLOCK:
            bResult = ( _locking( hFileHandle, _LK_UNLCK, ulLength ) == 0 );
            break;

         default:
            bResult = FALSE;
      }
      hb_fsSetIOError( bResult, 0 );
      lseek( hFileHandle, ulOldPos, SEEK_SET );
      HB_DISABLE_ASYN_CANC
   }
#elif defined(HB_OS_UNIX)
   {
      /* TODO: check for append locks (SEEK_END)
       */
      struct flock lock_info;

      switch( uiMode & FL_MASK )
      {
         case FL_LOCK:

            lock_info.l_type   = (uiMode & FLX_SHARED) ? F_RDLCK : F_WRLCK;
            lock_info.l_start  = ulStart;
            lock_info.l_len    = ulLength;
            lock_info.l_whence = SEEK_SET;   /* start from the beginning of the file */
            lock_info.l_pid    = getpid();

            bResult = ( fcntl( hFileHandle,
                               (uiMode & FLX_WAIT) ? F_SETLKW: F_SETLK,
                               &lock_info ) >= 0 );
            break;

         case FL_UNLOCK:

            lock_info.l_type   = F_UNLCK;   /* unlock */
            lock_info.l_start  = ulStart;
            lock_info.l_len    = ulLength;
            lock_info.l_whence = SEEK_SET;
            lock_info.l_pid    = getpid();

            bResult = ( fcntl( hFileHandle, F_SETLK, &lock_info ) >= 0 );
            break;

         default:
            bResult = FALSE;
      }
      hb_fsSetIOError( bResult, 0 );
   }
#elif defined(HAVE_POSIX_IO) && !defined(__IBMCPP__) && ( !defined(__GNUC__) || defined(__DJGPP__) )

   // allowing async cancelation here
   HB_TEST_CANCEL_ENABLE_ASYN
   switch( uiMode & FL_MASK )
   {
      case FL_LOCK:
         bResult = ( lock( hFileHandle, ulStart, ulLength ) == 0 );
         break;

      case FL_UNLOCK:
         bResult = ( unlock( hFileHandle, ulStart, ulLength ) == 0 );
         break;

      default:
         bResult = FALSE;
   }
   hb_fsSetIOError( bResult, 0 );
   HB_DISABLE_ASYN_CANC

#else

   bResult = FALSE;
   hb_fsSetError( FS_ERROR );

#endif

   HB_STACK_LOCK

   return bResult;
}

BOOL HB_EXPORT hb_fsLockLarge( FHANDLE hFileHandle, HB_FOFFSET ulStart,
                               HB_FOFFSET ulLength, USHORT uiMode )
{
   BOOL bResult;

   HB_TRACE(HB_TR_DEBUG, ("hb_fsLockLarge(%p, %" PFHL "u, %" PFHL "u, %hu)", hFileHandle, ulStart, ulLength, uiMode));

#if defined(HB_WIN32_IO)
   {
      HB_THREAD_STUB

      DWORD dwOffsetLo = ( DWORD ) ( ulStart & 0xFFFFFFFF ),
            dwOffsetHi = ( DWORD ) ( ulStart >> 32 ),
            dwLengthLo = ( DWORD ) ( ulLength & 0xFFFFFFFF ),
            dwLengthHi = ( DWORD ) ( ulLength >> 32 );

      static BOOL s_bInit = 0, s_bWinNt ;

      if ( !s_bInit )
      {
         s_bInit = TRUE ;
         s_bWinNt = hb_iswinnt() ;
      }

      HB_STACK_UNLOCK
      HB_TEST_CANCEL_ENABLE_ASYN

      switch( uiMode & FL_MASK )
      {
         case FL_LOCK:
            if ( s_bWinNt )
            {
               OVERLAPPED sOlap ;
               DWORD dwFlags ;

               dwFlags = ( ( uiMode & FLX_SHARED ) ? 0 : LOCKFILE_EXCLUSIVE_LOCK ) |
                         ( ( uiMode & FLX_WAIT ) ? 0 : LOCKFILE_FAIL_IMMEDIATELY );

               memset( &sOlap, 0, sizeof( OVERLAPPED ) );
               sOlap.Offset = dwOffsetLo;
               sOlap.OffsetHigh = dwOffsetHi;

               bResult = LockFileEx( DostoWinHandle( hFileHandle ), dwFlags, 0,
                                     dwLengthLo, dwLengthHi, &sOlap );
            }
            else
            {
               bResult = LockFile( DostoWinHandle( hFileHandle ),
                                   dwOffsetLo, dwOffsetHi,
                                   dwLengthLo, dwLengthHi );
            }
            break;

         case FL_UNLOCK:
            if ( s_bWinNt )
            {
               OVERLAPPED sOlap ;

               memset( &sOlap, 0, sizeof( OVERLAPPED ) );
               sOlap.Offset = dwOffsetLo;
               sOlap.OffsetHigh = dwOffsetHi;

               bResult = UnlockFileEx( DostoWinHandle( hFileHandle ), 0,
                                       dwLengthLo, dwLengthHi, &sOlap );
            }
            else
            {
               bResult = UnlockFile( DostoWinHandle( hFileHandle ),
                                     dwOffsetLo, dwOffsetHi,
                                     dwLengthLo, dwLengthHi );
            }
            break;

         default:
            bResult = FALSE;
      }
      hb_fsSetIOError( bResult, 0 );

      HB_DISABLE_ASYN_CANC
      HB_STACK_LOCK
   }
#elif defined(HB_OS_LINUX) && defined(__USE_LARGEFILE64)
   /*
    * The macro: __USE_LARGEFILE64 is set when _LARGEFILE64_SOURCE is
    * define and efectively enables lseek64/flock64/ftruncate64 functions
    * on 32bit machines.
    */
   {
      HB_THREAD_STUB

      struct flock64 lock_info;

      HB_STACK_UNLOCK
      HB_TEST_CANCEL_ENABLE_ASYN

      switch( uiMode & FL_MASK )
      {
         case FL_LOCK:

            lock_info.l_type   = (uiMode & FLX_SHARED) ? F_RDLCK : F_WRLCK;
            lock_info.l_start  = ulStart;
            lock_info.l_len    = ulLength;
            lock_info.l_whence = SEEK_SET;   /* start from the beginning of the file */
            lock_info.l_pid    = getpid();

            bResult = ( fcntl( hFileHandle,
                               (uiMode & FLX_WAIT) ? F_SETLKW64: F_SETLK64,
                               &lock_info ) != -1 );
            break;

         case FL_UNLOCK:

            lock_info.l_type   = F_UNLCK;   /* unlock */
            lock_info.l_start  = ulStart;
            lock_info.l_len    = ulLength;
            lock_info.l_whence = SEEK_SET;
            lock_info.l_pid    = getpid();

            bResult = ( fcntl( hFileHandle, F_SETLK64, &lock_info ) != -1 );
            break;

         default:
            bResult = FALSE;
      }
      hb_fsSetIOError( bResult, 0 );

      HB_DISABLE_ASYN_CANC
      HB_STACK_LOCK
   }
#else
   bResult = hb_fsLock( hFileHandle, (ULONG) ulStart, (ULONG) ulLength, uiMode );
#endif

   return bResult;
}

ULONG   HB_EXPORT hb_fsSeek( FHANDLE hFileHandle, LONG lOffset, USHORT uiFlags )
{
   HB_THREAD_STUB
   /* Clipper compatibility: under clipper, ulPos is returned as it was
      before; on error it is not changed. This is not thread compliant,
      but does not cares as MT prg are required to test FError(). */
   /* This is nothing compilat, this static var is not bound with file
      handle and it will cause overwriting database files and many other
      bad side effect if seek fails - it's one of the most serious bug
      I've found so far in filesys.c - ulPos _CANNOT_BE_ static, Druzus */
   ULONG ulPos;

   HB_TRACE(HB_TR_DEBUG, ("hb_fsSeek(%p, %ld, %hu)", hFileHandle, lOffset, uiFlags));

   HB_STACK_UNLOCK
   // allowing async cancelation here
   HB_TEST_CANCEL_ENABLE_ASYN

#if defined(HB_FS_FILE_IO)
{
   USHORT Flags = convert_seek_flags( uiFlags );

   #if defined(HB_OS_OS2)
   {
      APIRET ret;

      /* This DOS hack creates 2GB file size limit, Druzus */
      if( lOffset < 0 && Flags == SEEK_SET )
      {
         ret = 1;
         hb_fsSetError( 25 ); /* 'Seek Error' */
      }
      else
      {
         ret = DosSetFilePtr( hFileHandle, lOffset, Flags, &ulPos );
         /* TODO: what we should do with this error code? Is it DOS compatible? */
         hb_fsSetError(( USHORT ) ret );
      }
      if( ret != 0 )
      {
         /* FIXME: it should work if DosSetFilePtr is lseek compatible
            but maybe OS2 has DosGetFilePtr too, if not then remove this
            comment, Druzus */
         if ( DosSetFilePtr( hFileHandle, 0, SEEK_CUR, &ulPos ) != 0 )
         {
            ulPos = 0;
         }
      }
   }
   #elif defined(HB_WIN32_IO)
      /* This DOS hack creates 2GB file size limit, Druzus */
      if( lOffset < 0 && Flags == SEEK_SET )
      {
         ulPos = (ULONG) INVALID_SET_FILE_POINTER;
         hb_fsSetError( 25 ); /* 'Seek Error' */
      }
      else
      {
         ulPos = (DWORD) SetFilePointer( DostoWinHandle(hFileHandle), lOffset, NULL, (DWORD)Flags );
         hb_fsSetIOError( (DWORD) ulPos != INVALID_SET_FILE_POINTER, 0 );
      }

      if ( (DWORD) ulPos == INVALID_SET_FILE_POINTER )
      {
         ulPos = (DWORD) SetFilePointer( DostoWinHandle(hFileHandle), 0, NULL, SEEK_CUR );
      }
   #else
      /* This DOS hack creates 2GB file size limit, Druzus */
      if( lOffset < 0 && Flags == SEEK_SET )
      {
         ulPos = (ULONG) -1;
         hb_fsSetError( 25 ); /* 'Seek Error' */
      }
      else
      {
         ulPos = lseek( hFileHandle, lOffset, Flags );
         hb_fsSetIOError( ulPos != (ULONG) -1, 0 );
      }
      if ( ulPos == (ULONG) -1 )
      {
         ulPos = lseek( hFileHandle, 0L, SEEK_CUR );
         if ( ulPos == (ULONG) -1 )
         {
            ulPos = 0;
         }
      }
   #endif
   }
#else
   hb_fsSetError( 25 );
   ulPos = 0;
#endif

   HB_DISABLE_ASYN_CANC
   HB_STACK_LOCK

   return ulPos;
}

HB_FOFFSET HB_EXPORT hb_fsSeekLarge( FHANDLE hFileHandle, HB_FOFFSET llOffset, USHORT uiFlags )
{
   HB_FOFFSET llPos;

   HB_TRACE(HB_TR_DEBUG, ("hb_fsSeekLarge(%p, %" PFHL "u, %hu)", hFileHandle, llOffset, uiFlags));

#if defined(HB_WIN32_IO)
   {
      HB_THREAD_STUB

      USHORT Flags = convert_seek_flags( uiFlags );

      ULONG ulOffsetLow  = ( ULONG ) ( llOffset & ULONG_MAX ),
            ulOffsetHigh = ( ULONG ) ( llOffset >> 32 );

      HB_STACK_UNLOCK
      HB_TEST_CANCEL_ENABLE_ASYN

      if( llOffset < 0 && Flags == SEEK_SET )
      {
         llPos = ( HB_FOFFSET ) INVALID_SET_FILE_POINTER;
         hb_fsSetError( 25 ); /* 'Seek Error' */
      }
      else
      {
         ulOffsetLow = SetFilePointer( DostoWinHandle( hFileHandle ),
                                       ulOffsetLow, (PLONG) &ulOffsetHigh,
                                       ( DWORD ) Flags );
         llPos = ( ( HB_FOFFSET ) ulOffsetHigh << 32 ) | ulOffsetLow;
         hb_fsSetIOError( llPos != ( HB_FOFFSET ) INVALID_SET_FILE_POINTER, 0 );
      }

      if ( llPos == ( HB_FOFFSET ) INVALID_SET_FILE_POINTER )
      {
         ulOffsetHigh = 0;
         ulOffsetLow = SetFilePointer( DostoWinHandle( hFileHandle ),
                                       0, (PLONG) &ulOffsetHigh, SEEK_CUR );
         llPos = ( ( HB_FOFFSET ) ulOffsetHigh << 32 ) | ulOffsetLow;
      }

      HB_DISABLE_ASYN_CANC
      HB_STACK_LOCK
   }
#elif defined(HB_OS_LINUX) && defined(__USE_LARGEFILE64)
   /*
    * The macro: __USE_LARGEFILE64 is set when _LARGEFILE64_SOURCE is
    * define and efectively enables lseek64/flock64/ftruncate64 functions
    * on 32bit machines.
    */
   {
      HB_THREAD_STUB

      USHORT Flags = convert_seek_flags( uiFlags );

      HB_STACK_UNLOCK
      HB_TEST_CANCEL_ENABLE_ASYN

      if( llOffset < 0 && Flags == SEEK_SET )
      {
         llPos = (HB_FOFFSET) -1;
         hb_fsSetError( 25 ); /* 'Seek Error' */
      }
      else
      {
         llPos = lseek64( hFileHandle, llOffset, Flags );
         hb_fsSetIOError( llPos != (HB_FOFFSET) -1, 0 );
      }

      if ( llPos == (HB_FOFFSET) -1 )
      {
         llPos = lseek64( hFileHandle, 0L, SEEK_CUR );
      }

      HB_DISABLE_ASYN_CANC
      HB_STACK_LOCK
   }
#else
   llPos = (HB_FOFFSET) hb_fsSeek( hFileHandle, (LONG) llOffset, uiFlags );
#endif

   return llPos;
}

ULONG   HB_EXPORT hb_fsTell( FHANDLE hFileHandle )
{
   HB_THREAD_STUB
   ULONG ulPos;

   HB_TRACE(HB_TR_DEBUG, ("hb_fsTell(%p)", hFileHandle));

   HB_STACK_UNLOCK

#if defined(HB_FS_FILE_IO)

   #if defined(HB_WIN32_IO)
      // allowing async cancelation here
      HB_TEST_CANCEL_ENABLE_ASYN
      ulPos = (DWORD) SetFilePointer( DostoWinHandle(hFileHandle), 0, NULL, FILE_CURRENT );
      hb_fsSetIOError( (DWORD) ulPos != INVALID_SET_FILE_POINTER, 0 );
      HB_DISABLE_ASYN_CANC
   #else
      // allowing async cancelation here
      HB_TEST_CANCEL_ENABLE_ASYN
      ulPos = lseek( hFileHandle, 0L, SEEK_CUR );
      hb_fsSetIOError( ulPos != (ULONG) -1, 0 );
      HB_DISABLE_ASYN_CANC
   #endif


#else

   ulPos = (ULONG) -1;
   hb_fsSetError( FS_ERROR );

#endif

   HB_STACK_LOCK

   return ulPos;
}

BOOL HB_EXPORT hb_fsDelete( BYTE * pFilename )
{
   HB_THREAD_STUB
   BOOL bResult;

   HB_TRACE(HB_TR_DEBUG, ("hb_fsDelete(%s)", (char*) pFilename));

   pFilename = ( BYTE *) hb_fileNameConv( hb_strdup( ( char * ) pFilename) );

   HB_STACK_UNLOCK


#if defined(HB_OS_WIN_32)

   HB_TEST_CANCEL_ENABLE_ASYN
   bResult = DeleteFile( ( char * ) pFilename );
   hb_fsSetIOError( bResult, 0 );
   HB_DISABLE_ASYN_CANC

#elif defined(HAVE_POSIX_IO)

   bResult = ( remove( ( char * ) pFilename ) == 0 );
   hb_fsSetIOError( bResult, 0 );

#elif defined(_MSC_VER) || defined(__MINGW32__)

   // allowing async cancelation here
   HB_TEST_CANCEL_ENABLE_ASYN
   bResult = ( remove( ( char * ) pFilename ) == 0 );
   hb_fsSetIOError( bResult, 0 );
   HB_DISABLE_ASYN_CANC

#else

   bResult = FALSE;
   hb_fsSetError( FS_ERROR );

#endif

   HB_STACK_LOCK

   hb_xfree( pFilename ) ;

   return bResult;
}

BOOL HB_EXPORT hb_fsRename( BYTE * pOldName, BYTE * pNewName )
{
   HB_THREAD_STUB
   BOOL bResult;

   HB_TRACE(HB_TR_DEBUG, ("hb_fsRename(%s, %s)", (char*) pOldName, (char*) pNewName));

   pOldName = ( BYTE *) hb_fileNameConv( hb_strdup( ( char * )pOldName) );
   pNewName = ( BYTE *) hb_fileNameConv( hb_strdup( ( char * )pNewName) );

   HB_STACK_UNLOCK


#if defined(HB_OS_WIN_32)

   // allowing async cancelation here
   HB_TEST_CANCEL_ENABLE_ASYN
   bResult = MoveFile( ( char * ) pOldName, ( char * ) pNewName );
   hb_fsSetIOError( bResult, 0 );
   HB_DISABLE_ASYN_CANC

#elif defined(HB_FS_FILE_IO)

   bResult = ( rename( ( char * ) pOldName, ( char * ) pNewName ) == 0 );
   hb_fsSetIOError( bResult, 0 );

#else

   bResult = FALSE;
   hb_fsSetError( FS_ERROR );

#endif


   HB_STACK_LOCK

   hb_xfree( pOldName ) ;
   hb_xfree( pNewName ) ;

   return bResult;
}

BOOL HB_EXPORT    hb_fsMkDir( BYTE * pDirname )
{
   HB_THREAD_STUB
   BOOL bResult;
   pDirname = (BYTE*)hb_fileNameConv( hb_strdup( ( char * )pDirname) );

   HB_TRACE(HB_TR_DEBUG, ("hb_fsMkDir(%s)", (char*) pDirname));

   HB_STACK_UNLOCK

#if defined(HB_OS_WIN_32)

   // allowing async cancelation here
   HB_TEST_CANCEL_ENABLE_ASYN
   bResult = CreateDirectory( ( char * ) pDirname, NULL );
   hb_fsSetIOError( bResult, 0 );
   HB_DISABLE_ASYN_CANC

#elif defined(HAVE_POSIX_IO) || defined(__MINGW32__)

#  if ! defined(HB_OS_UNIX) && \
      ( defined(__WATCOMC__) || defined(__BORLANDC__) || \
        defined(__IBMCPP__) || defined(__MINGW32__) )
      bResult = ( mkdir( ( char * ) pDirname ) == 0 );
#  else
      bResult = ( mkdir( ( char * ) pDirname, S_IRWXU | S_IRWXG | S_IRWXO ) == 0 );
#  endif
   hb_fsSetIOError( bResult, 0 );

#else

   bResult = FALSE;
   hb_fsSetError( FS_ERROR );

#endif

   HB_STACK_LOCK
   hb_xfree(pDirname) ;

   return bResult;
}

BOOL HB_EXPORT    hb_fsChDir( BYTE * pDirname )
{
   HB_THREAD_STUB
   BOOL bResult;

   pDirname = (BYTE*)hb_fileNameConv( hb_strdup( ( char * )pDirname) );
   HB_TRACE(HB_TR_DEBUG, ("hb_fsChDir(%s)", (char*) pDirname));

   HB_STACK_UNLOCK

#if defined(HB_OS_WIN_32)

   // allowing async cancelation here
   HB_TEST_CANCEL_ENABLE_ASYN
   bResult = SetCurrentDirectory( ( char * ) pDirname );
   hb_fsSetIOError( bResult, 0 );
   HB_DISABLE_ASYN_CANC

#elif defined(HAVE_POSIX_IO) || defined(__MINGW32__)

   bResult = ( chdir( ( char * ) pDirname ) == 0 );
   hb_fsSetIOError( bResult, 0 );

#else

   bResult = FALSE;
   hb_fsSetError( FS_ERROR );

#endif

   HB_STACK_LOCK
   hb_xfree( pDirname );
   return bResult;
}

BOOL HB_EXPORT    hb_fsRmDir( BYTE * pDirname )
{
   HB_THREAD_STUB
   BOOL bResult;
   pDirname = (BYTE*)hb_fileNameConv( hb_strdup( ( char * )pDirname) );

   HB_TRACE(HB_TR_DEBUG, ("hb_fsRmDir(%s)", (char*) pDirname));

   HB_STACK_LOCK

#if defined(HB_OS_WIN_32)

   HB_TEST_CANCEL_ENABLE_ASYN
   bResult = RemoveDirectory( ( char * ) pDirname );
   hb_fsSetIOError( bResult, 0 );
   HB_DISABLE_ASYN_CANC

#elif defined(HAVE_POSIX_IO) || defined(__MINGW32__)

   bResult = ( rmdir( ( char * ) pDirname ) == 0 );
   hb_fsSetIOError( bResult, 0 );

#else

   bResult = FALSE;
   hb_fsSetError( FS_ERROR );

#endif

   HB_STACK_UNLOCK
   hb_xfree( pDirname ) ;
   return bResult;
}

/* NOTE: This is not thread safe function, it's there for compatibility. */
/* NOTE: 0 = A, 1 = B, 2 = C, etc. */

BYTE HB_EXPORT * hb_fsCurDir( USHORT uiDrive )
{
   static BYTE s_byDirBuffer[ _POSIX_PATH_MAX + 1 ];

   HB_TRACE(HB_TR_DEBUG, ("hb_fsCurDir(%hu)", uiDrive));

   hb_fsCurDirBuff( uiDrive, s_byDirBuffer, _POSIX_PATH_MAX + 1 );

   return ( BYTE * ) s_byDirBuffer;
}

/* NOTE: Thread safe version of hb_fsCurDir() */
/* NOTE: 0 = A, 1 = B, 2 = C, etc. */

USHORT HB_EXPORT  hb_fsCurDirBuff( USHORT uiDrive, BYTE * pbyBuffer, ULONG ulLen )
{
   HB_THREAD_STUB
   BOOL fResult;
   HB_TRACE(HB_TR_DEBUG, ("hb_fsCurDirBuff(%hu)", uiDrive));

   HB_SYMBOL_UNUSED( uiDrive );

   pbyBuffer[ 0 ] = '\0';

   HB_STACK_UNLOCK

#if defined(HB_OS_WIN_32)

   HB_TEST_CANCEL_ENABLE_ASYN
   fResult = GetCurrentDirectory( ulLen, ( char * ) pbyBuffer );
   hb_fsSetIOError( fResult, 0 );
   HB_DISABLE_ASYN_CANC

#elif defined(HB_OS_OS2)

   fResult = ( _getcwd1( (char *) pbyBuffer, uiDrive + 'A' ) == 0 );
   hb_fsSetIOError( fResult, 0 );

#elif defined(HAVE_POSIX_IO)

   fResult = ( getcwd( ( char * ) pbyBuffer, ulLen ) != NULL );
   hb_fsSetIOError( fResult, 0 );

#elif defined(__MINGW32__)

   fResult = ( _getdcwd( uiDrive, pbyBuffer, ulLen ) != NULL );
   hb_fsSetIOError( fResult, 0 );

#else

   fResult = FALSE;
   hb_fsSetError( FS_ERROR );

#endif

   HB_STACK_LOCK

   if ( hb_fsError() != 0 )
   {
      return hb_fsError();
   }

   /* Strip the leading drive spec, and leading backslash if there's one. */

   {
      BYTE * pbyStart = pbyBuffer;

      /* NOTE: A trailing underscore is not returned on this platform,
               so we don't need to strip it. [vszakats] */

#if defined(OS_HAS_DRIVE_LETTER)
      if( pbyStart[ 1 ] == OS_DRIVE_DELIMITER )
         pbyStart += 2;
#endif
      if( strchr( OS_PATH_DELIMITER_LIST, pbyStart[ 0 ] ) )
      {
         pbyStart++;
      }

      if( pbyBuffer != pbyStart )
      {
         memmove( pbyBuffer, pbyStart, ulLen );
      }
   }

   /* Strip the trailing (back)slash if there's one */

   {
      ULONG ulLen = strlen( ( char * ) pbyBuffer );

      if( strchr( OS_PATH_DELIMITER_LIST, pbyBuffer[ ulLen - 1 ] ) )
      {
         pbyBuffer[ ulLen - 1 ] = '\0';
      }
   }

   return 0; // correct if it arrives here
}

/* NOTE: 0=A:, 1=B:, 2=C:, 3=D:, ... */

USHORT HB_EXPORT  hb_fsChDrv( BYTE nDrive )
{
#if defined(OS_HAS_DRIVE_LETTER)
   HB_THREAD_STUB
#endif

   USHORT uiResult;

   HB_TRACE(HB_TR_DEBUG, ("hb_fsChDrv(%d)", (int) nDrive));

#if defined(OS_HAS_DRIVE_LETTER)
   HB_STACK_UNLOCK

   {
      /* 'unsigned int' _have to_ be used in Watcom */
      UINT uiSave, uiNewDrive;

      /* allowing async cancelation here */
      HB_TEST_CANCEL_ENABLE_ASYN

      HB_FS_GETDRIVE( uiSave );
      HB_FS_SETDRIVE( nDrive );
      HB_FS_GETDRIVE( uiNewDrive );

      if ( nDrive == uiNewDrive )
      {
         uiResult = 0;
         hb_fsSetError( 0 );
      }
      else
      {
         HB_FS_SETDRIVE( uiSave );

         uiResult = (USHORT) FS_ERROR;
         hb_fsSetError( (USHORT) FS_ERROR );
      }
      HB_DISABLE_ASYN_CANC
    }

    HB_STACK_UNLOCK

#else

   HB_SYMBOL_UNUSED( nDrive );
   uiResult = FS_ERROR;
   hb_fsSetError( FS_ERROR );

#endif

   return uiResult;
}

/* NOTE: 0=A:, 1=B:, 2=C:, 3=D:, ... */

/* TOFIX: This isn't fully compliant because CA-Cl*pper doesn't access
          the drive before checking. hb_fsIsDrv only returns TRUE
          if there is a disk in the drive. */

USHORT HB_EXPORT  hb_fsIsDrv( BYTE nDrive )
{
#if defined(OS_HAS_DRIVE_LETTER)
   HB_THREAD_STUB
#endif

   USHORT uiResult;

   HB_TRACE(HB_TR_DEBUG, ("hb_fsIsDrv(%d)", (int) nDrive));

#if defined(OS_HAS_DRIVE_LETTER)
   HB_STACK_UNLOCK

   {
      /* 'unsigned int' _have to_ be used in Watcom
       */
      UINT uiSave, uiNewDrive;

      HB_FS_GETDRIVE( uiSave );
      HB_FS_SETDRIVE( nDrive );
      HB_FS_GETDRIVE( uiNewDrive );
      if( nDrive != uiNewDrive )
      {
         uiResult = FS_ERROR;
         hb_fsSetError( FS_ERROR );
      }
      else
      {
         uiResult = 0;
         hb_fsSetError( 0 );
      }
      HB_FS_SETDRIVE( uiSave );
   }

   HB_STACK_LOCK

#else

   HB_SYMBOL_UNUSED( nDrive );
   uiResult = FS_ERROR;
   hb_fsSetError( FS_ERROR );

#endif

   return uiResult;
}

BOOL   HB_EXPORT  hb_fsIsDevice( FHANDLE hFileHandle )
{
   BOOL bResult;

   HB_TRACE(HB_TR_DEBUG, ("hb_fsIsDevice(%p)", hFileHandle));

#if defined(HB_FS_FILE_IO)

   bResult = ( isatty( hFileHandle ) != 0 );
   hb_fsSetIOError( bResult, 0 );

#else

   bResult = FALSE;
   hb_fsSetError( FS_ERROR );
   HB_SYMBOL_UNUSED( hFileHandle );

#endif

   return bResult;
}

/* NOTE: 0=A:, 1=B:, 2=C:, 3=D:, ... */

BYTE   HB_EXPORT  hb_fsCurDrv( void )
{
   /* 'unsigned int' _have to_ be used in Watcom */
   UINT uiResult;

   HB_TRACE(HB_TR_DEBUG, ("hb_fsCurDrv()"));

#if defined(OS_HAS_DRIVE_LETTER)

   HB_FS_GETDRIVE( uiResult );

#else

   uiResult = 0;

#endif

   /* 27/08/04 - <maurilio.longo@libero.it>
                 This is wrong, IMHO, should set 0 if HB_FS_GETDRIVE() returned something */
   hb_fsSetError( FS_ERROR );
   return ( BYTE ) uiResult; /* Return the drive number, base 0. */
}

/* TODO: Implement hb_fsExtOpen */

FHANDLE HB_EXPORT  hb_fsExtOpen( BYTE * pFilename, BYTE * pDefExt,
                      USHORT uiFlags, BYTE * pPaths, PHB_ITEM pError )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_fsExtOpen(%s, %s, %hu, %p, %p)", (char*) pFilename, (char*) pDefExt, uiFlags, pPaths, pError));

   hb_fsSetError( (USHORT) FS_ERROR );

   HB_SYMBOL_UNUSED( pFilename );
   HB_SYMBOL_UNUSED( pDefExt );
   HB_SYMBOL_UNUSED( uiFlags );
   HB_SYMBOL_UNUSED( pPaths );
   HB_SYMBOL_UNUSED( pError );

   return hb_fsError();
}

BOOL HB_EXPORT hb_fsEof( FHANDLE hFileHandle )
{
#if defined(__DJGPP__) || defined(__CYGWIN__) || defined(OS_UNIX_COMPATIBLE)
   HB_THREAD_STUB
   LONG curPos;
   LONG endPos;
   LONG newPos;
   BOOL fResult = FALSE;

   HB_STACK_UNLOCK

   // allowing async cancelation here
   HB_TEST_CANCEL_ENABLE_ASYN

   curPos = lseek( hFileHandle, 0L, SEEK_CUR );
   if ( curPos != -1 )
   {
      endPos = lseek( hFileHandle, 0L, SEEK_END );
      newPos = lseek( hFileHandle, curPos, SEEK_SET );
      fResult = ( endPos != -1 && newPos == curPos );
   }
   else
   {
      endPos = -1;
   }
   hb_fsSetIOError( fResult, 0 );

   HB_DISABLE_ASYN_CANC

   HB_STACK_LOCK

   return ( !fResult || curPos == endPos );
#else
   //JC1: Should not cause system lock
   return eof( hFileHandle ) != FS_ERROR;
#endif
}

BYTE HB_EXPORT * hb_fsCurDirEx( USHORT uiDrive )
{
   static BYTE s_byDirBuffer[ _POSIX_PATH_MAX + 1 ];

   HB_TRACE(HB_TR_DEBUG, ("hb_fsCurDir(%hu)", uiDrive));

   hb_fsCurDirBuffEx( uiDrive, s_byDirBuffer, _POSIX_PATH_MAX + 1 );

   return ( BYTE * ) s_byDirBuffer;
}

USHORT HB_EXPORT  hb_fsCurDirBuffEx( USHORT uiDrive, BYTE * pbyBuffer, ULONG ulLen )
{
#if defined(HB_OS_WIN_32)
   HB_THREAD_STUB
#endif

   HB_TRACE(HB_TR_DEBUG, ("hb_fsCurDirBuff(%hu)", uiDrive));

   HB_SYMBOL_UNUSED( uiDrive );

   pbyBuffer[ 0 ] = '\0';

#if defined(HB_OS_WIN_32)
{
   DWORD dwResult;

   HB_TEST_CANCEL_ENABLE_ASYN
   dwResult = GetCurrentDirectory( ulLen, ( char * ) pbyBuffer );
   hb_fsSetIOError( dwResult != 0, 0 );
   HB_DISABLE_ASYN_CANC
   HB_STACK_LOCK
}

#elif defined(HB_OS_OS2)

   hb_fsSetIOError( ( _getcwd1( (char *) pbyBuffer, uiDrive + 'A' ) != 0 ), 0 );

#elif defined(HAVE_POSIX_IO)

   hb_fsSetIOError( getcwd( ( char * ) pbyBuffer, ulLen ) != NULL, 0 );

#elif defined(__MINGW32__)

   hb_fsSetIOError( _getdcwd( uiDrive, pbyBuffer, ulLen ) != NULL, 0 );

#else

   hb_fsSetError( FS_ERROR );
   return FS_ERROR;

#endif

   /* Strip the leading drive spec, and leading backslash if there's one. */

   if ( hb_fsError() != 0 )
   {
      return hb_fsError();
   }
   else
   {
      BYTE * pbyStart = pbyBuffer;
      ULONG ulPthLen;

      /* NOTE: A trailing underscore is not returned on this platform,
               so we don't need to strip it. [vszakats] */

#if defined(OS_HAS_DRIVE_LETTER)
      if( pbyStart[ 1 ] == OS_DRIVE_DELIMITER )
      {
         pbyStart += 2;
      }
#endif

      if( pbyBuffer != pbyStart )
      {
         memmove( pbyBuffer, pbyStart, ulLen );
      }

      /* Strip the trailing (back)slash if there's one */
      ulPthLen = strlen( ( char * ) pbyBuffer );

      if( strchr( OS_PATH_DELIMITER_LIST, pbyBuffer[ ulPthLen - 1 ] ) )
      {
         pbyBuffer[ ulPthLen  ] = '\0';
     }
      else
     {
         strcat( (char *) pbyBuffer, OS_PATH_DELIMITER_STRING);
     }

      return 0; // if it reaches here, it is right.
   }
}

/* TODO: remove it */
char HB_EXPORT * hb_fileTrim( BYTE * szFile)  /* Caller must free the buffer returned */
{
   char * szFileTrim ;
   ULONG ulLen;

   ulLen =  hb_strRTrimLen( (char*) szFile, strlen( (char*) szFile ), FALSE );
   szFile = (BYTE*) hb_strLTrim( (char*) szFile, &ulLen );
   szFileTrim = (char * ) hb_xgrab( ulLen + 1 );
   memcpy( (char*) szFileTrim, (char*) szFile, ulLen );
   szFileTrim[ulLen] = '\0';

   return szFileTrim;
}

/* TODO: remove it */
BYTE HB_EXPORT * hb_filecase(char *str) {
   // Convert file and dir case. The allowed SET options are:
   // LOWER - Convert all caracters of file to lower
   // UPPER - Convert all caracters of file to upper
   // MIXED - Leave as is

   // The allowed environment options are:
   // FILECASE - define the case of file
   // DIRCASE - define the case of path
   // DIRSEPARATOR - define separator of path (Ex. "/")

   size_t a;
   char *filename;
   char *dirname=str;
   size_t dirlen;

   // Look for filename (Last "\" or DIRSEPARATOR)
   if( hb_set.HB_SET_DIRSEPARATOR != '\\' ) {
      for(a=0;a<strlen(str);a++)
         if( str[a] == '\\' )
            str[a] = hb_set.HB_SET_DIRSEPARATOR;
   }
   if(( filename = strrchr( str, hb_set.HB_SET_DIRSEPARATOR )) != NULL)
      filename++;
   else
      filename=str;
   dirlen=filename-str;

   // FILECASE
   if( hb_set.HB_SET_FILECASE == HB_SET_CASE_LOWER )
      hb_strLower( filename, strlen(filename) );
   else if( hb_set.HB_SET_FILECASE == HB_SET_CASE_UPPER )
      hb_strUpper( filename, strlen(filename) );

   // DIRCASE
   if( hb_set.HB_SET_DIRCASE == HB_SET_CASE_LOWER )
      hb_strLower(dirname,dirlen);
   else if( hb_set.HB_SET_DIRCASE == HB_SET_CASE_UPPER )
      hb_strUpper(dirname,dirlen);
   return (( BYTE * ) str);
}

BYTE HB_EXPORT * hb_fileNameConv( char *str ) {
   // Convert file and dir case. The allowed SET options are:
   // LOWER - Convert all caracters of file to lower
   // UPPER - Convert all caracters of file to upper
   // MIXED - Leave as is

   // The allowed environment options are:
   // FILECASE - define the case of file
   // DIRCASE - define the case of path
   // DIRSEPARATOR - define separator of path (Ex. "/")

   char *filename;
   ULONG ulDirLen, ulFileLen;

   if ( hb_set.HB_SET_TRIMFILENAME )
   {
      char *szFileTrim;
      ULONG ulLen;

      ulLen = hb_strRTrimLen( str, strlen( str ), FALSE );
      szFileTrim = hb_strLTrim( str, &ulLen );
      if ( str != szFileTrim )
      {
         memmove( str, szFileTrim, ulLen );
      }
      str[ulLen] = '\0';
   }

   // Look for filename (Last "\" or DIRSEPARATOR)
   if( hb_set.HB_SET_DIRSEPARATOR != '\\' )
   {
      char *p = str;
      while ( *p )
      {
         if( *p == '\\' )
         {
            *p = hb_set.HB_SET_DIRSEPARATOR;
         }
         p++;
      }
   }

   if ( ( filename = strrchr( str, hb_set.HB_SET_DIRSEPARATOR ) ) != NULL )
   {
      filename++;
   }
   else
   {
      filename = str;
   }
   ulFileLen = strlen( filename );
   ulDirLen = filename - str;

   // FILECASE
   if ( ulFileLen > 0 )
   {
      if( hb_set.HB_SET_FILECASE == HB_SET_CASE_LOWER )
         hb_strLower( filename, strlen(filename) );
      else if( hb_set.HB_SET_FILECASE == HB_SET_CASE_UPPER )
         hb_strUpper( filename, strlen(filename) );
   }

   // DIRCASE
   if ( ulDirLen > 0 )
   {
      if ( hb_set.HB_SET_DIRCASE == HB_SET_CASE_LOWER )
         hb_strLower( str, ulDirLen );
      else if( hb_set.HB_SET_DIRCASE == HB_SET_CASE_UPPER )
         hb_strUpper( str, ulDirLen );
   }
   return (( BYTE * ) str);
}
