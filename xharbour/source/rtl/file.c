/*
 * $Id: file.c,v 1.16 2004/03/03 20:34:52 andijahja Exp $
 */

/*
 * Harbour Project source code:
 * hb_fsFile() function
 *
 * Copyright 1999-2002 Viktor Szakats <viktor.szakats@syenar.hu>
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
* hb_fsIsDirectory( BYTE * pFilename ) -- determine if a given file is a directory
*     Copyright 2004 Giancarlo Niccolai
*/

#include "hbapi.h"
#include "hbapifs.h"
#include "hbapiitm.h"
#include "hbset.h"
BOOL HB_EXPORT hb_fsFile( BYTE * pFilename )
{
   PHB_FFIND ffind;
   BOOL bResult = FALSE;
   int iFileName ;

   pFilename = hb_fileNameConv( hb_strdup( ( char * ) pFilename ) );

   iFileName = strlen( (char*) pFilename ) ;
   if ( iFileName && pFilename[iFileName-1] != OS_PATH_DELIMITER ) // A directory cannot possibly be a FILE
   {                                                               // so only do this is the last char is not
                                                                   // a directory separator character
     if( ( ffind = hb_fsFindFirst( ( char * ) pFilename, HB_FA_ALL ) ) != NULL )
     {
       if (( ffind->attr & HB_FA_DIRECTORY ) != HB_FA_DIRECTORY ) // If it's not a directory it's a file
       {
          bResult = TRUE;
       }
       else if ( strchr( pFilename, '*' ) || strchr( pFilename, '?' ) ) // Clipper compatibility
       {                                                                // FindFirst may have found a directory first
       	 while( !bResult && hb_fsFindNext( ffind ) )
       	 {
           bResult = (( ffind->attr & HB_FA_DIRECTORY ) != HB_FA_DIRECTORY ) ;
         }
       }
       hb_fsFindClose( ffind );
     }
   }

   hb_fsSetError( 0 );
   hb_xfree(pFilename);

   return bResult;
}

BOOL HB_EXPORT hb_fsIsDirectory( BYTE * pFilename )
{
   PHB_FFIND ffind;
   BOOL bResult = FALSE;
   int iFileName ;

   pFilename = hb_filecase( hb_strdup( ( char * ) pFilename ) );

   iFileName = strlen( (char*) pFilename ) ;

   if ( iFileName )
   {
     if( ( ffind = hb_fsFindFirst( ( char * ) pFilename, HB_FA_ALL ) ) != NULL )
     {
       if (( ffind->attr & HB_FA_DIRECTORY ) == HB_FA_DIRECTORY )
       {
         bResult = TRUE;
       }
       else if ( pFilename[iFileName-1] == OS_PATH_DELIMITER )
       {
         bResult = TRUE;
       }
       hb_fsFindClose( ffind );
     }
   }
   hb_fsSetError( 0 );
   hb_xfree(pFilename);

   return bResult;
}
