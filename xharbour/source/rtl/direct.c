/*
 * $Id: direct.c,v 1.15 2004/02/27 16:01:11 andijahja Exp $
 */

/*
 * Harbour Project source code:
 * DIRECTORY() function
 *
 * Copyright 1999 Leslee Griffith <les.griffith@vantagesystems.ca>
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
 * Notes from the fringe... <ptucker@sympatico.ca>
 *
 * Clipper is a bit schizoid with the treatment of file attributes, but we've
 * emulated that weirdness here for your viewing amusement.
 *
 * In Clippers' homeworld of DOS, there are 5 basic attributes: 'A'rchive,
 * 'H'idden, 'S'ystem, 'R'eadonly and 'D'irectory.  In addition, a file can
 * have no attributes, and only 1 file per physical partition can have the
 * 'V'olume label.
 *
 * For a given file request, it is implied that the attribute mask includes
 * all attributes except 'H'idden, 'S'ystem, 'D'irectory and 'V'olume.
 * The returned file list will always include (for instance) 'R'eadOnly files
 * unless they also happen to be 'H'idden and that attribute was not requested.
 *
 * "V" is a special case - you will get back the entry that describes the
 * volume label for the drive implied by the filemask.
 *
 * Differences from the 'standard' (where supported):
 * - Filenames will be returned in the same case as they are stored in the
 *   directory.  Clipper (and VO too) will convert the names to upper case
 * - Filenames will be the full filename as supported by the OS in use.
 * - There are a number of additional file attributes returned.
 *   They are:
 *       'I' - DEVICE      File is a device
 *       'T' - TEMPORARY   File is a Temporary file
 *       'P' - SPARSE      File is Sparse
 *       'L' - REPARSE     File/Dir is a reparse point
 *       'C' - COMPRESSED  File/Dir is compressed
 *       'O' - OFFLINE     File/Dir is not online
 *       'X' - NOTINDEXED  Exclude File/Dir from Indexing Service
 *       'E' - ENCRYPTED   File/Dir is Encrypted
 *       'M' - VOLCOMP     Volume Supports Compression
 * - Clipper can sometimes drop the ReadOnly indication of directories.
 *   Harbour detects this correctly.
 *
 * TODO: - Under an MS Windows implimentation, an optional 3rd parameter to
 *         Directory to allow you to receive the compatible '8.3' filename.
 *       - check that path support vis stat works on all platforms
 *       - UNC Support? ie: dir \\myserver\root
 *
 * TOFIX:- Volume label support
 *
 */

#include "hbapi.h"
#include "hbapifs.h"
#include "hbapiitm.h"
#include "hbfast.h"

#include "directry.ch"

#if defined( HB_OS_UNIX )
   #define HB_DIR_ALL_FILES_MASK        "*"
#else
   #define HB_DIR_ALL_FILES_MASK        "*.*"
#endif

#include "hbapi.h"
#include "hbapifs.h"
#include "hbapiitm.h"
#include "hbfast.h"

#include "directry.ch"

#if defined( HB_OS_UNIX )
   #define HB_DIR_ALL_FILES_MASK        "*"
#else
   #define HB_DIR_ALL_FILES_MASK        "*.*"
#endif

HB_FUNC( DIRECTORY )
{
   PHB_ITEM  pDirSpec = hb_param( 1, HB_IT_STRING );
   PHB_ITEM  pAttributes = hb_param( 2, HB_IT_STRING );
   BYTE *    szDirSpec ;
   USHORT    uiMask;
   BOOL      bDirOnly = hb_parl(3);
   BOOL      bFullPath = hb_parl(4);

/*
#if defined(__MINGW32__) || ( defined(_MSC_VER) && _MSC_VER >= 910 )
   PHB_ITEM pEightDotThree = hb_param( 3, HB_IT_LOGICAL );
   BOOL     bEightDotThree;

   // Do we want 8.3 support?
   bEightDotThree = ( pEightDotThree ? hb_itemGetL( pEightDotThree ) : FALSE );
#endif
*/

   HB_ITEM pDir;
   PHB_FFIND ffind;
   BOOL bAlloc = FALSE;
   PHB_FNAME fDirSpec = NULL;
   BYTE *pCurDir;
   char cCurDsk;

   /* Get the passed attributes and convert them to Harbour Flags */

   uiMask = HB_FA_ARCHIVE
          | HB_FA_READONLY
          | HB_FA_NORMAL
          | HB_FA_DEVICE
          | HB_FA_TEMPORARY
          | HB_FA_SPARSE
          | HB_FA_REPARSE
          | HB_FA_COMPRESSED
          | HB_FA_OFFLINE
          | HB_FA_NOTINDEXED
          | HB_FA_ENCRYPTED
          | HB_FA_VOLCOMP;

   hb_arrayNew( &pDir, 0 );

   if ( bDirOnly )
   {
      if( !pAttributes )
      {
         pAttributes = hb_itemNew( NULL );
         hb_itemPutC( pAttributes, "D" );
         bAlloc = TRUE;
      }
      else
      {
         if ( pAttributes->item.asString.value != "D" )
            pAttributes->item.asString.value = "D";
      }
   }

   if( pAttributes && pAttributes->item.asString.length > 0 )
      if ( ( uiMask |= hb_fsAttrEncode( pAttributes->item.asString.value ) ) & HB_FA_LABEL )
      {
         /* NOTE: This is Clipper Doc compatible. (not operationally) */
         uiMask = HB_FA_LABEL;
      }


   szDirSpec = pDirSpec ?
               hb_fileNameConv( hb_strdup( ( char * ) pDirSpec->item.asString.value ) ) :
               (BYTE *) HB_DIR_ALL_FILES_MASK;

   if ( bDirOnly || bFullPath )
   {
      cCurDsk = hb_fsCurDrv() ;
      pCurDir = hb_fsCurDir( cCurDsk ) ;

      if ( (fDirSpec = hb_fsFNameSplit( (char*) szDirSpec )) !=NULL )
      {
         if( fDirSpec->szDrive )
         {
            hb_fsChDrv( ( BYTE ) fDirSpec->szDrive );
         }

         if( fDirSpec->szPath )
         {
            hb_fsChDir( ( BYTE *) fDirSpec->szPath );
         }
      }
   }

   /* Get the file list */
   if( ( ffind = hb_fsFindFirst( (const char *) szDirSpec, uiMask ) ) != NULL )
   {
      HB_ITEM pFilename;
      HB_ITEM pSize;
      HB_ITEM pDate;
      HB_ITEM pTime;
      HB_ITEM pAttr;

      pFilename.type = HB_IT_NIL ;
      pSize.type = HB_IT_NIL ;
      pDate.type = HB_IT_NIL ;
      pTime.type = HB_IT_NIL ;
      pAttr.type = HB_IT_NIL ;

      do
      {
         if( !( ( ( uiMask & HB_FA_HIDDEN    ) == 0 && ( ffind->attr & HB_FA_HIDDEN    ) != 0 ) ||
                ( ( uiMask & HB_FA_SYSTEM    ) == 0 && ( ffind->attr & HB_FA_SYSTEM    ) != 0 ) ||
                ( ( uiMask & HB_FA_LABEL     ) == 0 && ( ffind->attr & HB_FA_LABEL     ) != 0 ) ||
                ( ( uiMask & HB_FA_DIRECTORY ) == 0 && ( ffind->attr & HB_FA_DIRECTORY ) != 0 ) ))
         {
            HB_ITEM pSubarray;
            char buffer[ 32 ];
            BOOL bAddEntry;

            hb_arrayNew( &pSubarray, 0 );

            if ( bFullPath )
            {
               char *szFullName = hb_xstrcpy(NULL,fDirSpec->szPath?fDirSpec->szPath:"",ffind->szName,NULL);
               hb_arrayAddForward( &pSubarray, hb_itemPutC( &pFilename, szFullName ) );
               hb_xfree( szFullName );
            }
            else
            {
               hb_arrayAddForward( &pSubarray, hb_itemPutC( &pFilename, ffind->szName ) );
            }
         #ifndef HB_LONG_LONG_OFF
            hb_arrayAddForward( &pSubarray, hb_itemPutNLL( &pSize, ffind->size ) );
         #else
            hb_arrayAddForward( &pSubarray, hb_itemPutNL( &pSize, ffind->size ) );
         #endif
            hb_arrayAddForward( &pSubarray, hb_itemPutDL( &pDate, ffind->lDate ) );
            hb_arrayAddForward( &pSubarray, hb_itemPutC( &pTime, ffind->szTime ) );
            hb_arrayAddForward( &pSubarray, hb_itemPutC( &pAttr, hb_fsAttrDecode( ffind->attr, buffer ) ) );

            /* Don't exit when array limit is reached */
            bAddEntry = bDirOnly ? hb_fsIsDirectory( ( BYTE * ) ffind->szName ) : TRUE;

            if( bAddEntry )
            {
               hb_arrayAddForward( &pDir, &pSubarray );
            }
            else
            {
               if ( bDirOnly )
               {
                  hb_itemClear( &pSubarray );
               }
            }
         }
      }
      while( hb_fsFindNext( ffind ) );

      hb_fsFindClose( ffind );

   }

   hb_itemForwardValue( &(HB_VM_STACK).Return, &pDir );

   if ( bDirOnly || bFullPath )
   {
      hb_fsChDrv( (BYTE ) cCurDsk );
      hb_fsChDir( pCurDir );
   }

   if ( pDirSpec )
   {
      hb_xfree( szDirSpec );
   }

   if ( bAlloc )
   {
      hb_itemRelease( pAttributes );
   }

   if ( fDirSpec != NULL )
   {
      hb_xfree( fDirSpec );
   }
}
