/*
 * $Id: hbver.c,v 1.12 2004/01/18 03:51:35 paultucker Exp $
 */

/*
 * Harbour Project source code:
 * Version detection functions
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
 * Copyright 1999 Luiz Rafael Culik <culik@sl.conex.net>
 *    hb_verPlatform() (support for determining the windows version)
 *
 * Copyright 1999 Jose Lalin <dezac@corevia.com>
 *    hb_verPlatform() (support for determining many windows flavours)
 *    hb_verCompiler() (support for determining some compiler version/revision)
 *
 * Copyright 2000-2001 Viktor Szakats <viktor.szakats@syenar.hu>
 *    hb_verPlatform() (support for detecting Windows NT on DOS)
 *    hb_verPlatform() (rearrangment and cleanup)
 *
 * See doc/license.txt for licensing terms.
 *
 */

/* NOTE: For OS/2. Must be ahead of any and all #include statements */
#define INCL_DOSMISC

#define HB_OS_WIN_32_USED

#include "hbapi.h"
#include "hbver.h"
#include "hbmemory.ch"

#if defined(HB_OS_WIN_32)

   #include <ctype.h>
   #include "hbwbase.h"

#elif defined(HB_OS_UNIX)

   #include <sys/utsname.h>

#endif

/* NOTE: OS() function, as a primary goal will detect the version number
         of the target platform. As an extra it may also detect the host OS.
         The latter is mainly an issue in DOS, where the host OS can be OS/2
         WinNT/2K, Win3x, Win9x, DOSEMU, Desqview, etc. [vszakats] */

/* NOTE: The caller must free the returned buffer. [vszakats] */

char * hb_verPlatform( void )
{

   char *szPlatform = ( char * ) hb_xgrab( 256 );

   HB_TRACE(HB_TR_DEBUG, ("hb_verPlatform()"));

#if defined(HB_OS_DOS)

   {
      /* NOTE: Must be larger than 128, which is the maximum size of
               osVer.szCSDVersion (Win32). [vszakats] */
      char szName[128];
      union REGS regs;

      regs.h.ah = 0x30;
      HB_DOS_INT86( 0x21, &regs, &regs );

      sprintf( szPlatform, "DOS %d.%02d", regs.h.al, regs.h.ah );

      /* Host OS detection: Windows 2.x, 3.x, 95/98 */

      {
         regs.HB_XREGS.ax = 0x1600;
         HB_DOS_INT86( 0x2F, &regs, &regs );

         if( regs.h.al != 0x00 && regs.h.al != 0x80 )
         {
            if( regs.h.al == 0x01 || regs.h.al == 0xFF )
            {
               sprintf( szName, " (Windows 2.x)" );
            }
            else
            {
               sprintf( szName, " (Windows %d.%02d)", regs.h.al, regs.h.ah );
            }

            strcat( szPlatform, szName );
         }
      }

      /* Host OS detection: Windows NT/2000 */

      {
         regs.HB_XREGS.ax = 0x3306;
         HB_DOS_INT86( 0x21, &regs, &regs );

         if( regs.HB_XREGS.bx == 0x3205 )
         {
            strcat( szPlatform, " (Windows NT/2000)" );
         }
      }

      /* Host OS detection: OS/2 */

      {
         regs.h.ah = 0x30;
         HB_DOS_INT86( 0x21, &regs, &regs );

         if( regs.h.al >= 10 )
         {
            if( regs.h.al == 20 && regs.h.ah > 20 )
            {
               sprintf( szName, " (OS/2 %d.%02d)", regs.h.ah / 10, regs.h.ah % 10 );
            }
            else
            {
               sprintf( szName, " (OS/2 %d.%02d)", regs.h.al / 10, regs.h.ah );
            }

            strcat( szPlatform, szName );
         }
      }
   }

#elif defined(HB_OS_OS2)

   {
      ULONG aulQSV[ QSV_MAX ] = { 0 };
      APIRET rc;

      rc = DosQuerySysInfo( 1L, QSV_MAX, ( void * ) aulQSV, sizeof( ULONG ) * QSV_MAX );

      if( rc == 0 )
      {
         /* is this OS/2 2.x ? */
         if( aulQSV[ QSV_VERSION_MINOR - 1 ] < 30 )
         {
            sprintf( szPlatform, "OS/2 %ld.%02ld",
               aulQSV[ QSV_VERSION_MAJOR - 1 ] / 10,
               aulQSV[ QSV_VERSION_MINOR - 1 ] );
         }
         else
         {
            sprintf( szPlatform, "OS/2 %2.2f",
               ( float ) aulQSV[ QSV_VERSION_MINOR - 1 ] / 10 );
         }
      }
      else
      {
         sprintf( szPlatform, "OS/2" );
      }
   }

#elif defined(HB_OS_WIN_32)

   {
      /* NOTE: Must be larger than 128, which is the maximum size of
               osVer.szCSDVersion (Win32). [vszakats] */
      char szName[128];
      OSVERSIONINFO osVer;

      osVer.dwOSVersionInfoSize = sizeof( osVer );

      if( GetVersionEx( &osVer ) )
      {
         strcpy( szName, "Windows" );

         switch( osVer.dwPlatformId )
         {
            case VER_PLATFORM_WIN32_WINDOWS:

               if( osVer.dwMajorVersion == 4 && osVer.dwMinorVersion < 10 )
               {
                  strcat( szName, " 95" );
               }
               else if( osVer.dwMajorVersion == 4 && osVer.dwMinorVersion == 10 )
               {
                  strcat( szName, " 98" );
               }
               else
               {
                  strcat( szName, " ME" );
               }

               break;

            case VER_PLATFORM_WIN32_NT:

               if( osVer.dwMajorVersion == 5 && osVer.dwMinorVersion == 2 )
               {
                  strcat( szName, " Server 2003" );
               }
               else if( osVer.dwMajorVersion == 5 && osVer.dwMinorVersion == 1 )
               {
                  strcat( szName, " XP" );
               }
               else if( osVer.dwMajorVersion == 5 )
               {
                  strcat( szName, " 2000" );
               }
               else
               {
                  strcat( szName, " NT" );
               }

               /* test for specific product on Windows NT 4.0 SP6 and later */

               {
                  HBOSVERSIONINFOEX osVerEx;  /* NOTE */

                  osVerEx.dwOSVersionInfoSize = sizeof( osVerEx );

                                    /* Windows decl error? */
                  if( GetVersionEx( ( LPOSVERSIONINFOA ) &osVerEx ) )
                  {
                     /* workstation type */

                     if( osVerEx.wProductType == VER_NT_WORKSTATION )
                     {
                        if( osVerEx.dwMajorVersion == 4 )
                        {
                           strcat( szName, " Workstation 4.0" );
                        }
                        else if( osVerEx.wSuiteMask & VER_SUITE_PERSONAL )
                        {
                           strcat( szName, " Home Edition" );
                        }
                        else
                        {
                           strcat( szName, " Professional" );
                        }
                     }

                     /* server type */

                     else if( osVerEx.wProductType == VER_NT_SERVER )
                     {
                        if( osVerEx.dwMajorVersion == 5 && osVerEx.dwMinorVersion == 2 )
                        {
                           if( osVerEx.wSuiteMask & VER_SUITE_DATACENTER )
                           {
                              strcat( szName, " Datacenter Edition" );
                           }
                           else if( osVerEx.wSuiteMask & VER_SUITE_ENTERPRISE )
                           {
                              strcat( szName, " Enterprise Edition" );
                           }
                           else if( osVerEx.wSuiteMask == VER_SUITE_BLADE )
                           {
                              strcat( szName, " Web Edition" );
                           }
                           else
                           {
                              strcat( szName, " Standard Edition" );
                           }
                        }

                        else if( osVerEx.dwMajorVersion == 5 && osVerEx.dwMinorVersion == 0 )
                        {
                           if( osVerEx.wSuiteMask & VER_SUITE_DATACENTER )
                           {
                              strcat( szName, " Datacenter Server" );
                           }
                           else if( osVerEx.wSuiteMask & VER_SUITE_ENTERPRISE )
                           {
                              strcat( szName, " Advanced Server" );
                           }
                           else
                           {
                              strcat( szName, " Server" );
                           }
                        }

                        else
                        {
                           if( osVerEx.wSuiteMask & VER_SUITE_ENTERPRISE )
                           {
                              strcat( szName, " Server 4.0, Enterprise Edition" );
                           }
                           else
                           {
                              strcat( szName, " Server 4.0" );
                           }
                        }
                     }
                  }
               }

               break;

            case VER_PLATFORM_WIN32s:
               strcat( szName, " 32s" );
               break;

            case VER_PLATFORM_WIN32_CE:
               strcat( szName, " CE" );
               break;
         }

         sprintf( szPlatform, "%s %lu.%02lu.%04d", szName,
                              ( ULONG ) osVer.dwMajorVersion,
                              ( ULONG ) osVer.dwMinorVersion,
                              ( USHORT ) LOWORD( osVer.dwBuildNumber ) );

         /* Add service pack/other info */

         if( osVer.szCSDVersion )
         {
            int i;

            /* Skip the leading spaces (Win95B, Win98) */
            for( i = 0; osVer.szCSDVersion[ i ] != '\0' && isspace( ( int ) osVer.szCSDVersion[ i ] ); i++ );

            if( osVer.szCSDVersion[ i ] != '\0' )
            {
               strcat( szPlatform, " " );
               strcat( szPlatform, osVer.szCSDVersion + i );
            }
         }
      }
      else
      {
         sprintf( szPlatform, "Windows" );
      }
   }

#elif defined(HB_OS_UNIX)

   {
      struct utsname un;

      uname( &un );
      sprintf( szPlatform, "%s %s", un.sysname, un.release );
   }

#elif defined(HB_OS_MAC)

   {
      strcpy( szPlatform, "MacOS compatible" );
   }

#else

   {
      strcpy( szPlatform, "(unknown)" );
   }

#endif

   return szPlatform;
}

/* NOTE: The caller must free the returned buffer. [vszakats] */

char * hb_verCompiler( void )
{
   char * pszCompiler;
   char * szName;
   int iVerMajor;
   int iVerMinor;

   HB_TRACE(HB_TR_DEBUG, ("hb_verCompiler()"));

   pszCompiler = ( char * ) hb_xgrab( 80 );

#if defined(__IBMC__) || defined(__IBMCPP__)

   #if defined(__IBMC__)
      iVerMajor = __IBMC__;
   #else
      iVerMajor = __IBMCPP__;
   #endif

   if( iVerMajor >= 300 )
      szName = "IBM Visual Age C++";
   else
      szName = "IBM C++";

   iVerMajor /= 100;
   iVerMinor = iVerMajor % 100;

#elif defined(_MSC_VER)

   #if (_MSC_VER >= 800)
      szName = "Microsoft Visual C/C++";
   #else
      szName = "Microsoft C/C++";
   #endif

   iVerMajor = _MSC_VER / 100;
   iVerMinor = _MSC_VER % 100;

#elif defined(__BORLANDC__)

   szName = "Borland C++";
   #if (__BORLANDC__ == 1040) /* Version 3.1 */
      iVerMajor = 3;
      iVerMinor = 1;
   #elif (__BORLANDC__ >= 1280) /* Version 5.x */
      iVerMajor = __BORLANDC__ >> 8;
      iVerMinor = ( __BORLANDC__ & 0xFF ) >> 4;
   #else /* Version 4.x */
      iVerMajor = __BORLANDC__ >> 8;
      iVerMinor = ( __BORLANDC__ - 1 & 0xFF ) >> 4;
   #endif

#elif defined(__TURBOC__)

   szName = "Borland Turbo C";
   iVerMajor = __TURBOC__ >> 8;
   iVerMinor = __TURBOC__ & 0xFF;

#elif defined(__MPW__)

   szName = "MPW C";
   iVerMajor = __MPW__ / 100;
   iVerMinor = __MPW__ % 100;

#elif defined(__WATCOMC__)

   szName = "Watcom C/C++";
   iVerMajor = __WATCOMC__ / 100;
   iVerMinor = __WATCOMC__ % 100;

#elif defined(__GNUC__)

   #if defined(__DJGPP__)
      szName = "Delorie GNU C";
   #elif defined(__CYGWIN__)
      szName = "Cygnus Cygwin GNU C";
   #elif defined(__MINGW32__)
      szName = "Cygnus MinGW GNU C";
   #elif defined(__RSX32__)
      szName = "EMX/RSXNT/DOS GNU C";
   #elif defined(__RSXNT__)
      szName = "EMX/RSXNT/Win32 GNU C";
   #elif defined(__EMX__)
      szName = "EMX GNU C";
   #else
      szName = "GNU C";
   #endif

   iVerMajor = __GNUC__;
   iVerMinor = __GNUC_MINOR__;

#else

   szName = ( char * ) NULL;
   iVerMajor = 0;
   iVerMinor = 0;

#endif

   if( szName )
      sprintf( pszCompiler, "%s %hd.%hd", szName, iVerMajor, iVerMinor );
   else
      strcpy( pszCompiler, "(unknown)" );

#if defined(__DJGPP__)

   {
      char szSub[ 32 ];
      sprintf( szSub, " (DJGPP %i.%02i)", ( int ) __DJGPP__, ( int ) __DJGPP_MINOR__ );
      strcat( pszCompiler, szSub );
   }

#elif defined(__BORLANDC__)

   {
      char szSub[ 32 ];
      /* QUESTION: Is there any better, safer, more official way to detect
                   the bit depth of the C compiler ? [vszakats] */
      sprintf( szSub, " (%i bit)", ( int ) ( sizeof( int ) * 8 ) );
      strcat( pszCompiler, szSub );
   }

#endif

   return pszCompiler;
}

/* NOTE: The caller must free the returned buffer. [vszakats] */

char * hb_verHarbour( void )
{
   char * pszVersion;

   HB_TRACE(HB_TR_DEBUG, ("hb_verHarbour()"));

   pszVersion = ( char * ) hb_xgrab( 80 );

   sprintf( pszVersion, "xHarbour build %d.%d.%d Intl. (%s)", HB_VER_MAJOR, HB_VER_MINOR, HB_VER_REVISION, HB_VER_LEX );

   return pszVersion;
}

char * hb_verPCode( void )
{
   char * pszVersion;

   HB_TRACE(HB_TR_DEBUG, ("hb_verPCode()"));

   pszVersion = ( char * ) hb_xgrab( 32 );

   sprintf( pszVersion, "PCode Version: %d", HB_PCODE_VER );

   return pszVersion;

}

void hb_verBuildInfo( void )
{
   hb_conOutErr( "Harbour Build Info", 0 );
   hb_conOutErr( hb_conNewLine(), 0 );
   hb_conOutErr( "---------------------------", 0 );
   hb_conOutErr( hb_conNewLine(), 0 );

   {
      char * pszVersion = hb_verHarbour();
      hb_conOutErr( "Version: ", 0 );
      hb_conOutErr( pszVersion, 0 );
      hb_conOutErr( hb_conNewLine(), 0 );
      hb_xfree( pszVersion );
   }

   {
      char * pszVersion = hb_verPCode();
      hb_conOutErr( pszVersion, 0 );
      hb_conOutErr( hb_conNewLine(), 0 );
      hb_xfree( pszVersion );
   }

   {
      char * pszVersion = hb_verCompiler();
      hb_conOutErr( "Compiler: ", 0 );
      hb_conOutErr( pszVersion, 0 );
      hb_conOutErr( hb_conNewLine(), 0 );
      hb_xfree( pszVersion );
   }

   {
      char * pszVersion = hb_verPlatform();
      hb_conOutErr( "Platform: ", 0 );
      hb_conOutErr( pszVersion, 0 );
      hb_conOutErr( hb_conNewLine(), 0 );
      hb_xfree( pszVersion );
   }

   hb_conOutErr( hb_conNewLine(), 0 );

   hb_conOutErr( "Built on: ", 0 );
   hb_conOutErr( __DATE__, 0 );
   hb_conOutErr( " ", 0 );
   hb_conOutErr( __TIME__, 0 );
   hb_conOutErr( hb_conNewLine(), 0 );

   hb_conOutErr( "Last ChangeLog entry: ", 0 );
   hb_conOutErr( HB_VER_LENTRY, 0 );
   hb_conOutErr( hb_conNewLine(), 0 );

   hb_conOutErr( "ChangeLog CVS version: ", 0 );
   hb_conOutErr( HB_VER_CHLCVS, 0 );
   hb_conOutErr( hb_conNewLine(), 0 );

   if( strlen( HB_VER_C_USR ) )
   {
      hb_conOutErr( "Harbour compiler switches: ", 0 );
      hb_conOutErr( HB_VER_C_USR, 0 );
      hb_conOutErr( hb_conNewLine(), 0 );
   }

   if( strlen( HB_VER_L_USR ) )
   {
      hb_conOutErr( "C compiler switches: ", 0 );
      hb_conOutErr( HB_VER_L_USR, 0 );
      hb_conOutErr( hb_conNewLine(), 0 );
   }

   if( strlen( HB_VER_PRG_USR ) )
   {
      hb_conOutErr( "Linker switches: ", 0 );
      hb_conOutErr( HB_VER_PRG_USR, 0 );
      hb_conOutErr( hb_conNewLine(), 0 );
   }

   hb_conOutErr( hb_conNewLine(), 0 );

   hb_conOutErr( "Harbour extensions: ", 0 );
#if defined( HB_EXTENSION )
   hb_conOutErr( "Yes", 0 );
#else
   hb_conOutErr( "No", 0 );
#endif
   hb_conOutErr( hb_conNewLine(), 0 );

   hb_conOutErr( "CA-Clipper 5.2e undocumented: ", 0 );
#if defined( HB_C52_UNDOC )
   hb_conOutErr( "Yes", 0 );
#else
   hb_conOutErr( "No", 0 );
#endif
   hb_conOutErr( hb_conNewLine(), 0 );

   hb_conOutErr( "CA-Clipper 5.2e strict compatibility: ", 0 );
#if defined( HB_C52_STRICT )
   hb_conOutErr( "Yes", 0 );
#else
   hb_conOutErr( "No", 0 );
#endif
   hb_conOutErr( hb_conNewLine(), 0 );

   hb_conOutErr( "CA-Clipper 5.3x compatible extensions: ", 0 );
#if defined( HB_COMPAT_C53 )
   hb_conOutErr( "Yes", 0 );
#else
   hb_conOutErr( "No", 0 );
#endif
   hb_conOutErr( hb_conNewLine(), 0 );

   hb_conOutErr( "Alaska Xbase++ compatible extensions: ", 0 );
#if defined( HB_COMPAT_XPP )
   hb_conOutErr( "Yes", 0 );
#else
   hb_conOutErr( "No", 0 );
#endif
   hb_conOutErr( hb_conNewLine(), 0 );

   hb_conOutErr( "CA-Visual Objects compatible extensions: ", 0 );
#if defined( HB_COMPAT_VO )
   hb_conOutErr( "Yes", 0 );
#else
   hb_conOutErr( "No", 0 );
#endif
   hb_conOutErr( hb_conNewLine(), 0 );

   hb_conOutErr( "Multisoft Flagship compatible extensions: ", 0 );
#if defined( HB_COMPAT_FLAGSHIP )
   hb_conOutErr( "Yes", 0 );
#else
   hb_conOutErr( "No", 0 );
#endif
   hb_conOutErr( hb_conNewLine(), 0 );

   hb_conOutErr( "Microsoft FoxPro compatible extensions: ", 0 );
#if defined( HB_COMPAT_FOXPRO )
   hb_conOutErr( "Yes", 0 );
#else
   hb_conOutErr( "No", 0 );
#endif
   hb_conOutErr( hb_conNewLine(), 0 );

   hb_conOutErr( "dBase compatible extensions: ", 0 );
#if defined( HB_COMPAT_DBASE )
   hb_conOutErr( "Yes", 0 );
#else
   hb_conOutErr( "No", 0 );
#endif
   hb_conOutErr( hb_conNewLine(), 0 );

   hb_conOutErr( "Object file generation support: ", 0 );
#if defined( HARBOUR_OBJ_GENERATION ) || defined( HB_BACK_END )
   hb_conOutErr( "Yes", 0 );
#else
   hb_conOutErr( "No", 0 );
#endif
   hb_conOutErr( hb_conNewLine(), 0 );

   hb_conOutErr( "ANSI C usage: ", 0 );
#if defined( HARBOUR_STRICT_ANSI_C )
   hb_conOutErr( "Strict", 0 );
#else
   hb_conOutErr( "Non strict", 0 );
#endif
   hb_conOutErr( hb_conNewLine(), 0 );

   hb_conOutErr( "C++ mode: ", 0 );
#if defined(__cplusplus)
   hb_conOutErr( "On", 0 );
#else
   hb_conOutErr( "Off", 0 );
#endif
   hb_conOutErr( hb_conNewLine(), 0 );

   hb_conOutErr( "Compiler YACC debug mode: ", 0 );
#if defined( HARBOUR_YYDEBUG )
   hb_conOutErr( "On", 0 );
#else
   hb_conOutErr( "Off", 0 );
#endif
   hb_conOutErr( hb_conNewLine(), 0 );

   hb_conOutErr( "Memory tracing and statistics: ", 0 );
   hb_conOutErr( hb_xquery( HB_MEM_USEDMAX ) != 0 ? "On" : "Off", 0 );
/*
#if defined( HB_FM_STATISTICS )
   hb_conOutErr( "On", 0 );
#else
   hb_conOutErr( "Off", 0 );
#endif
*/
   hb_conOutErr( hb_conNewLine(), 0 );

   {
      char buffer[ 64 ];
      sprintf( buffer, "Maximum symbol name length: %i", HB_SYMBOL_NAME_LEN );
      hb_conOutErr( buffer, 0 );
      hb_conOutErr( hb_conNewLine(), 0 );
   }

   hb_conOutErr( "---------------------------", 0 );
   hb_conOutErr( hb_conNewLine(), 0 );
}
