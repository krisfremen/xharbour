/*
 * $Id: dbftools.c,v 1.2 2003/10/01 17:41:47 paultucker Exp $
 */

/*
 * Harbour Project source code:
 * Some dbf structure related functions
 *
 * Copyright 2000 Alexander Kresin <alex@belacy.belgorod.su>
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

#include "hbapi.h"
#include "hbapirdd.h"

extern HB_FUNC( FIELDPOS );
extern HB_FUNC( FIELDLEN );
extern HB_FUNC( FIELDDEC );

HB_FUNC( FIELDSIZE )
{
   HB_FUNCNAME( FIELDLEN )();
}

HB_FUNC( FIELDDECI )
{
   HB_FUNCNAME( FIELDDEC )();
}

HB_FUNC( FIELDNUM )
{
   HB_FUNCNAME( FIELDPOS )();
}

HB_FUNC( DBFSIZE )
{
   ULONG ulSize = 0;
   AREAP pArea;

   if( (pArea = ( AREAP ) hb_rddGetCurrentWorkAreaPointer()) != NULL )
   {
      HB_ITEM pSize ;
      ULONG ulRecSize, ulRecCount;

      pSize.type = HB_IT_NIL;
      SELF_INFO( pArea, DBI_GETHEADERSIZE, &pSize );
      ulSize = hb_itemGetNL( &pSize ) + 1;
      SELF_INFO( pArea, DBI_GETRECSIZE, &pSize );
      ulRecSize = hb_itemGetNL( &pSize );
      SELF_RECCOUNT( pArea, &ulRecCount );
      ulSize += ulRecCount * ulRecSize;
   }

   hb_retnl( ulSize );
}

/*
 These are now part of xHarbour rdd, or aren't needed
 pt

static LPFIELD _hb_get_field( void )
{
   LPFIELD pField = NULL;

   if( hb_pcount() > 0 && ISNUM(1) )
   {
      USHORT uiField;

      if( (uiField = hb_parni( 1 )) > 0 )
      {
         AREAP pArea;

         if( (pArea = ( AREAP ) hb_rddGetCurrentWorkAreaPointer()) != NULL )
         {
            USHORT uiFieldsNO;

            if( SELF_FIELDCOUNT( pArea, &uiFieldsNO ) == SUCCESS &&
                uiField <= uiFieldsNO )
               pField = pArea->lpFields + uiField - 1;
         }
      }
   }

   return pField;
}

HB_FUNC( FIELDTYPE )
{
   char *pszType = "";
   LPFIELD pField = _hb_get_field();

   if( pField )
   {
      switch( pField->uiType )
      {
         case HB_IT_STRING:
            pszType = "C";
            break;
         case HB_IT_LONG:
            pszType = "N";
            break;
         case HB_IT_DATE:
            pszType = "D";
            break;
         case HB_IT_LOGICAL:
            pszType = "L";
            break;
         case HB_IT_MEMO:
            pszType = "M";
            break;
      }
   }
   hb_retc( pszType );
}

HB_FUNC( FIELDSIZE )
{
   UINT uiLen = 0;
   LPFIELD pField = _hb_get_field();

   if( pField )
      uiLen = pField->uiLen;

   hb_retni( uiLen );
}

HB_FUNC( FIELDDECI )
{
   UINT uiDec = 0;
   LPFIELD pField = _hb_get_field();

   if( pField )
      uiDec = pField->uiDec;

   hb_retni( uiDec );
}

*/

