/*
 * $Id: philes.c,v 1.29 2005/05/19 02:20:18 druzus Exp $
 */

/*
 * Harbour Project source code:
 * The FileSys API (Harbour level)
 *
 * Copyright 1999 Manuel Ruiz <mrt@joca.es>
 * www - http://www.harbour-project.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version, with one exception:
 *
 * The exception is that if you link the Harbour Runtime Library (HRL)
 * and/or the Harbour Virtual Machine (HVM) with other files to produce
 * an executable, this does not by itself cause the resulting executable
 * to be covered by the GNU General Public License. Your use of that
 * executable is in no way restricted on account of linking the HRL
 * and/or HVM code into it.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA (or visit
 * their web site at http://www.gnu.org/).
 *
 */

/*
 * The following parts are Copyright of the individual authors.
 * www - http://www.harbour-project.org
 *
 * Copyright 1999-2001 Viktor Szakats <viktor.szakats@syenar.hu>
 *    CURDIR()
 *
 * Copyright 2000 David G. Holm <dholm@jsd-llc.com>
 *    HB_F_EOF()
 *
 * Copyright 2003 Giancarlo Niccolai <gian@niccolai.ws>
 *    HB_OSPATHSEPARATOR()
 *    HB_OSPATHLISTSEPARATOR()
 *    HB_OSPATHDELIMITERS()
 *    HB_OSDRIVESEPARATOR()
 *    HB_OPENPROCESS()
 *    HB_PROCESSVALUE()
 *    HB_CLOSEPROCESS()
 *
 * See doc/license.txt for licensing terms.
 *
 */

#include <ctype.h>

#if !defined(ERROR_SHARING_VIOLATION)
   #define ERROR_SHARING_VIOLATION 32L
#endif
#if !defined(ERROR_ACCESS_DENIED)
   #define ERROR_ACCESS_DENIED 5L
#endif

#include "hbapi.h"
#include "hbfast.h"
#include "hbapifs.h"
#include "hbapierr.h"
#include "hbset.h"


HB_FUNC( FOPEN )
{
   if( ISCHAR( 1 ) )
      hb_retnl( hb_fsOpen( ( BYTE * ) hb_parcx( 1 ),
                           ISNUM( 2 ) ? hb_parni( 2 ) : FO_READ | FO_COMPAT ) );
   else
      hb_errRT_BASE( EG_ARG, 2021, NULL, "FOPEN", 2, hb_paramError( 1 ), hb_paramError( 2 ) ); /* NOTE: Undocumented but existing Clipper Run-time error */
}

HB_FUNC( FCREATE )
{
   if( ISCHAR( 1 ) )
      hb_retnl( hb_fsCreate( ( BYTE * ) hb_parcx( 1 ),
                             ISNUM( 2 ) ? hb_parni( 2 ) : FC_NORMAL ) );
   else
      hb_retnl( FS_ERROR );
}

#ifdef HB_EXTENSION

HB_FUNC( HB_FCREATE )
{
   if( ISCHAR( 1 ) )
      hb_retni( hb_fsCreateEx( ( BYTE * ) hb_parcx( 1 ),
                               ISNUM( 2 ) ? hb_parni( 2 ) : FC_NORMAL,
                               ISNUM( 3 ) ? hb_parni( 3 ) : FO_COMPAT ) );
   else
      hb_retni( FS_ERROR );
}

#endif

/*
  xHarbour extension allows for a 4th optional parameter indicating
  offset into buffer.
 */
HB_FUNC( FREAD )
{
   PHB_ITEM pBuffer = hb_param( 2, HB_IT_STRING );
   ULONG ulRead;

   if( ISNUM( 1 ) && pBuffer && ISBYREF( 2 ) && ISNUM( 3 ) )
   {
      ULONG ulOffset = hb_parnl( 4 );

      ulRead = hb_parnl( 3 );
      /* NOTE: CA-Clipper determines the maximum size by calling _parcsiz()
               instead of _parclen(), this means that the maximum read length
               will be one more than the length of the passed buffer, because
               the terminating zero could be used if needed. [vszakats] */

      if( ulOffset + ulRead <= hb_parcsiz( 2 ) )
      {
         /* NOTE: Warning, the read buffer will be directly modified,
                  this is normal here ! [vszakats] */

         /* Unshare the item to avoid GPF on static buffers and changing
            other items which shares this buffer. [druzus] */
         pBuffer = hb_itemUnShare( pBuffer );

         ulRead = hb_fsReadLarge( hb_parnl( 1 ),
                     ( BYTE * ) hb_itemGetCPtr( pBuffer ) + ulOffset, ulRead );
      }
      else
      {
         ulRead = 0;
      }
   }
   else
   {
      ulRead = 0;
   }

   hb_retnl( ulRead );
}

HB_FUNC( FWRITE )
{
   if( ISNUM( 1 ) && ISCHAR( 2 ) )
   {
      ULONG ulOffset = hb_parnl( 4 );

      hb_retnl( hb_fsWriteLarge( hb_parnl( 1 ), ( BYTE * ) hb_parcx( 2 ) + ulOffset, ISNUM( 3 ) ? (ULONG) hb_parnl( 3 ) : hb_parclen( 2 ) - ulOffset ) );
   }
   else
   {
      hb_retnl( 0 );
   }
}

HB_FUNC( FERROR )
{
// For clipper compatibility MSC return 32 for file open in share mode !
   #if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__WATCOMC__) || defined(__MINGW32__) || defined(__DMC__)
   USHORT uError = hb_fsError();
   if( uError == ERROR_SHARING_VIOLATION )
   {
      uError = ERROR_ACCESS_DENIED;
   }
   hb_retni( uError );
   #else
   hb_retni( hb_fsError() );
   #endif
}

HB_FUNC( HB_OSERROR )
{
   hb_retni( hb_fsOsError() );
}

HB_FUNC( FCLOSE )
{
   hb_fsSetError( 0 );

   if( ISNUM( 1 ) )
   {
      hb_fsClose( hb_parnl( 1 ) );
      hb_retl( hb_fsError() == 0 );
   }
   else
      hb_retl( FALSE );
}

HB_FUNC( FERASE )
{
   hb_fsSetError( 3 );

   hb_retni( ( ISCHAR( 1 ) &&
               hb_fsDelete( ( BYTE * ) hb_parcx( 1 ) ) ) ? 0 : -1 );
}

HB_FUNC( FRENAME )
{
   hb_fsSetError( 2 );

   hb_retni( ( ISCHAR( 1 ) && ISCHAR( 2 ) &&
               hb_fsRename( ( BYTE * ) hb_parcx( 1 ), ( BYTE * ) hb_parcx( 2 ) ) ) ? 0 : -1 );
}

HB_FUNC( FSEEK )
{
   if( ISNUM( 1 ) && ISNUM( 2 ) )
      hb_retnint( hb_fsSeekLarge( hb_parnl( 1 ),
                                  hb_parnint( 2 ),
                                  ISNUM( 3 ) ? hb_parni( 3 ) : FS_SET ) );
   else
      hb_retnl( 0 );
}

HB_FUNC( FREADSTR )
{
   if( ISNUM( 1 ) && ISNUM( 2 ) )
   {
      ULONG ulToRead = ( ULONG ) hb_parnl( 2 );

      if( ulToRead > 0 )
      {
         FHANDLE fhnd = ( FHANDLE ) hb_parnl( 1 );
         BYTE * buffer = ( BYTE * ) hb_xgrab( ulToRead + 1 );
         ULONG ulRead;

         ulRead = hb_fsReadLarge( fhnd, buffer, ulToRead );
         buffer[ ulRead ] = '\0';

         /* NOTE: Clipper will not return zero chars from this functions. */

         hb_retcAdopt( ( char * ) buffer );
      }
      else
      {
         hb_retc( "" );
      }
   }
   else
   {
      hb_retc( "" );
   }
}

/* NOTE: This function should not return the leading and trailing */
/*       (back)slashes. [vszakats] */

/* TODO: Xbase++ is able to change to the specified directory. */

HB_FUNC( CURDIR )
{
   USHORT uiErrorOld = hb_fsError();
   BYTE * pbyBuffer = ( BYTE * ) hb_xgrab( _POSIX_PATH_MAX + 1 );
   PHB_ITEM pDrv = hb_param( 1, HB_IT_STRING );
   BYTE cCurDrv = hb_fsCurDrv();
   BYTE cDrv;

   if( pDrv && hb_parclen( 1 ) > 0 )
   {
      cDrv = (BYTE) ( toupper( pDrv->item.asString.value[0] ) - 'A');
      if( cDrv != cCurDrv )
      {
         hb_fsChDrv( cDrv );
      }
   }
   else
   {
      cDrv = cCurDrv;
   }

   hb_fsCurDirBuff( cDrv, pbyBuffer, _POSIX_PATH_MAX + 1 );

   hb_retcAdopt( ( char * ) pbyBuffer );

   hb_fsChDrv( cCurDrv );

   hb_fsSetError( uiErrorOld );
}

#ifdef HB_EXTENSION

HB_FUNC( HB_F_EOF )
{
   if( ISNUM( 1 ) )
   {
      hb_retl( hb_fsEof( hb_parni( 1 ) ) );
   }
   else
   {
      hb_fsSetError( (USHORT) FS_ERROR );
      hb_retl( TRUE );
   }
}

HB_FUNC( CURDIRX )
{
   USHORT uiErrorOld = hb_fsError();
   BYTE * pbyBuffer = ( BYTE * ) hb_xgrab( _POSIX_PATH_MAX + 1 );
   PHB_ITEM pDrv = hb_param( 1, HB_IT_STRING );
   BYTE cCurDrv = hb_fsCurDrv();
   BYTE cDrv;

   if( pDrv && hb_parclen( 1 ) > 0 )
   {
      cDrv = (BYTE) ( toupper( pDrv->item.asString.value[0] ) - 'A');
      if( cDrv != cCurDrv )
      {
         hb_fsChDrv( cDrv );
      }
   }
   else
   {
      cDrv = cCurDrv;
   }

   hb_fsCurDirBuffEx( cDrv, pbyBuffer, _POSIX_PATH_MAX + 1 );

   hb_retcAdopt( ( char * ) pbyBuffer );

   hb_fsChDrv( cCurDrv );

   hb_fsSetError( uiErrorOld );
}

/** Added by Giancarlo Niccolai */
HB_FUNC( HB_OSPATHSEPARATOR )
{
   char ret[2];
   ret[1] = 0;
   ret[0] = OS_PATH_DELIMITER;

   hb_retc( ret );
}

HB_FUNC( HB_OSPATHLISTSEPARATOR )
{
   char ret[2];
   ret[1] = 0;
   ret[0] = OS_PATH_LIST_SEPARATOR;

   hb_retc( ret );
}

HB_FUNC( HB_OSPATHDELIMITERS )
{
   hb_retc( OS_PATH_DELIMITER_LIST );
}

HB_FUNC( HB_OSDRIVESEPARATOR )
{
#ifdef OS_DRIVE_DELIMITER
   char ret[2];
   ret[0] = OS_DRIVE_DELIMITER;
   ret[1] = 0;

   hb_retc( ret );
#else
   hb_retc( "" );
#endif
}

HB_FUNC( HB_OPENPROCESS )
{
   char *szName = hb_parcx( 1 );
   PHB_ITEM pIn = hb_param( 2, HB_IT_BYREF );
   PHB_ITEM pOut = hb_param( 3, HB_IT_BYREF );
   PHB_ITEM pErr = hb_param( 4, HB_IT_BYREF );
   PHB_ITEM pBackground = hb_param( 5, HB_IT_LOGICAL );
   PHB_ITEM pProcID = hb_param( 6, HB_IT_BYREF );

   FHANDLE fhIn, fhOut, fhErr;
   FHANDLE *pfhIn, *pfhOut, *pfhErr;
   FHANDLE fhProcess;
   BOOL bBackground;
   ULONG pid;

   if ( szName == NULL )
   {
      hb_errRT_BASE( EG_ARG, 4001,
         "First parameter (process name) is mandatory",
         "HB_OPENPROCESS", 6,
            hb_paramError( 1 ), hb_paramError( 2 ),  hb_paramError( 3 ),
            hb_paramError( 4 ), hb_paramError( 5 ), hb_paramError( 6 ) );
      return;
   }

   if ( (pIn != NULL && !ISBYREF(2) ) ||
        (pOut != NULL && !ISBYREF(3) ) ||
        (pErr != NULL && !ISBYREF(4) ) ||
        (pProcID != NULL && !ISBYREF(6) ) )
   {
      hb_errRT_BASE( EG_ARG, 4001,
         "All the given file handle parameters must be by reference",
         "HB_OPENPROCESS", 6,
            hb_paramError( 1 ), hb_paramError( 2 ),  hb_paramError( 3 ),
            hb_paramError( 4 ), hb_paramError( 5 ), hb_paramError( 6 ) );
      return;
   }

   if ( pIn != NULL && ( pIn == pOut || pIn == pErr ) )
   {
      hb_errRT_BASE( EG_ARG, 4001,
         "Input stream must differ from output and error stream",
         "HB_OPENPROCESS", 6,
            hb_paramError( 1 ), hb_paramError( 2 ),  hb_paramError( 3 ),
            hb_paramError( 4 ), hb_paramError( 5 ), hb_paramError( 6 ) );
      return;
   }

   if ( pBackground == NULL )
   {
      bBackground = FALSE;
   }
   else
   {
      bBackground = hb_itemGetL( pBackground );
   }

   if ( pIn != NULL )
   {
      pfhIn = &fhIn;
   }
   else
   {
      pfhIn = NULL;
   }

   if ( pOut != NULL )
   {
      pfhOut = &fhOut;
   }
   else
   {
      pfhOut = NULL;
   }

   if ( pErr != NULL )
   {
      if ( pErr != pOut )
      {
         pfhErr = &fhErr;
      }
      else
      {
         pfhErr = pfhOut;
      }
   }
   else
   {
      pfhErr = NULL;
   }
   fhProcess = hb_fsOpenProcess( szName, pfhIn, pfhOut, pfhErr, bBackground, &pid );

   if ( fhProcess != FS_ERROR )
   {
      if ( pIn != NULL )
      {
         hb_itemPutNL( pIn, fhIn );
      }

      if ( pOut != NULL )
      {
         hb_itemPutNL( pOut, fhOut );
      }

      if ( pErr != NULL && pErr != pOut)
      {
         hb_itemPutNL( pErr, fhErr );
      }

      if ( pProcID != NULL )
      {
         hb_itemPutNL( pProcID, (LONG) pid );
      }
   }

   hb_retnl( fhProcess );
}

HB_FUNC( HB_CLOSEPROCESS )
{
   FHANDLE fhProc = hb_parnl( 1 );
   PHB_ITEM pGentle = hb_param( 2, HB_IT_LOGICAL );

   if ( fhProc == 0 || hb_pcount() > 2 ||
      ( hb_pcount() == 2 && pGentle == NULL) )
   {
      hb_errRT_BASE( EG_ARG, 4001,
         "Wrong parameter count/type",
         "HB_CLOSEPROCESS", 2,
            hb_paramError( 1 ), hb_paramError( 2 ));
      return;
   }

   if ( pGentle == NULL || pGentle->item.asLogical.value )
   {
      hb_retl( hb_fsCloseProcess( fhProc, TRUE ) );
   }
   else
   {
      hb_retl( hb_fsCloseProcess( fhProc, FALSE ) );
   }

}

HB_FUNC( HB_PROCESSVALUE )
{
   FHANDLE fhProc = hb_parnl( 1 );
   PHB_ITEM pWait = hb_param( 2, HB_IT_LOGICAL );

   if ( fhProc == 0 || hb_pcount() > 2 ||
      ( hb_pcount() == 2 && pWait == NULL) )
   {
      hb_errRT_BASE( EG_ARG, 4001,
         "Wrong parameter count/type",
         "HB_CLOSEPROCESS", 2,
            hb_paramError( 1 ), hb_paramError( 2 ));
      return;
   }

   if ( pWait == NULL || pWait->item.asLogical.value )
   {
      hb_retni( hb_fsProcessValue( fhProc, TRUE ) );
   }
   else
   {
      hb_retni( hb_fsProcessValue( fhProc, FALSE ) );
   }

}

HB_FUNC( HB_FCOMMIT )
{
   hb_fsCommit( hb_parni(1) );
}

HB_FUNC( DISABLEWAITLOCKS )
{
   hb_retl( hb_fsDisableWaitLocks( ISLOG( 1 ) ? ( hb_parl( 1 ) ? 1 : 0 ) : -1 ) );
}

#endif
