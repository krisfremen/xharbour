/*
 * $Id: gx.c,v 1.17 2004/10/22 14:29:06 paultucker Exp $
 */

/*
 * Harbour Project source code:
 * NOSNOW(), SETMODE(), ISCOLOR() functions
 *
 * Copyright 1999-2001 Viktor Szakats <viktor.szakats@syenar.hu>
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
 * Copyright 1999 Paul Tucker <ptucker@sympatico.ca>
 *    SETMODE()
 *
 * Copyright 2004 Giancarlo Niccolai <antispam at niccolai dot ws >
 *    SETGTCLOSEHANDLER()
 *    GETGTCLOSEHANDLER()

 * See doc/license.txt for licensing terms.
 *
 */

#include "hbapi.h"
#include "hbapigt.h"
#include "hbapiitm.h"
#include "hbapierr.h"

HB_FUNC( ISCOLOR )
{
   hb_retl( hb_gtIsColor() );
}

HB_FUNC( NOSNOW )
{
   if( ISLOG( 1 ) )
      hb_gtSetSnowFlag( hb_parl( 1 ) );
}

HB_FUNC( SETMODE )
{
   int nRow = -1, nCol = -1;

   if( ISNUM( 1 ) && ISNUM( 2 ) )
   {
      nRow = hb_parni( 1 );
      nCol = hb_parni( 2 );
   }

   if( nRow == -1 || nCol == -1 )
   {
      nRow = hb_gtMaxRow() + 1;
      nCol = hb_gtMaxCol() + 1;
   }

   hb_retl( hb_gtSetMode( nRow, nCol ) == 0 );

}

HB_FUNC( SETGTCLOSEHANDLER )
{
   if ( hb_gtSetCloseHandler( hb_param(1, HB_IT_ANY ) ) == FALSE )
   {
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, "SETGTCLOSEHANDLER", 1, hb_paramError( 1 ) );
   }
}

HB_FUNC( GETGTCLOSEHANDLER )
{
   PHB_ITEM pi = hb_gtGetCloseHandler();
   if( pi == 0 )
   {
      hb_ret();
   }
   else {
      hb_itemReturnCopy( pi );
   }
}

HB_FUNC( SETCLOSEEVENT )
{
   PHB_ITEM pEvent = hb_param( 1, HB_IT_NUMERIC );
   if ( pEvent == NULL )
   {
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, "SETCLOSEEVENT", 1, hb_paramError( 1 ) );
   }
   else {
      hb_gtSetCloseEvent( hb_itemGetNL( pEvent ) );
   }
}

HB_FUNC( SETSHUTDOWNEVENT )
{
   PHB_ITEM pEvent = hb_param( 1, HB_IT_NUMERIC );
   if ( pEvent == NULL )
   {
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, "SETSHUTDOWNEVENT", 1, hb_paramError( 1 ) );
   }
   else {
      hb_gtSetShutdownEvent( hb_itemGetNL( pEvent ) );
   }
}

HB_FUNC( SETGTRESIZEEVENT )
{
   PHB_ITEM pEvent = hb_param( 1, HB_IT_NUMERIC );
   if ( pEvent == NULL )
   {
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, "SETRESIZEEVENT", 1, hb_paramError( 1 ) );
   }
   else {
      hb_gtSetResizeEvent( hb_itemGetNL( pEvent ) );
   }
}

HB_FUNC( GTSETCLIPBOARD )
{
   PHB_ITEM pData = hb_param( 1, HB_IT_STRING );
   if ( pData == NULL )
   {
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, "GTSETCLIPBOARD", 1, hb_paramError( 1 ) );
   }
   else
   {
      hb_gtSetClipboard( pData->item.asString.value, pData->item.asString.length );
   }
}

HB_FUNC( GTGETCLIPBOARD )
{
   PHB_ITEM pData = hb_param( 1, HB_IT_STRING );
   ULONG ulMaxLen;
   char *szData, cExtra = 0;
   int pCount = hb_pcount();

   if ( (pData == NULL && pCount == 1) || pCount > 1 )
   {
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, "GTGETCLIPBOARD", 1, hb_paramError( 1 ) );
      return;
   }

   if ( pCount == 1 )
   {
      ulMaxLen = pData->item.asString.length;
      if ( ulMaxLen == 0 )
      {
         hb_retc("");
         return;
      }
      szData = pData->item.asString.value;
      if ( hb_gtGetClipboardSize() < ulMaxLen )
      {
         cExtra = szData[ hb_gtGetClipboardSize() ];
      }
   }
   else
   {
      ulMaxLen = hb_gtGetClipboardSize();
      if ( ulMaxLen == 0 )
      {
         hb_retc("");
         return;
      }
      szData = (char *) hb_xgrab( ulMaxLen+1 );
   }

   hb_gtGetClipboard( szData , &ulMaxLen );

   if ( cExtra != 0 )
   {
      szData[ ulMaxLen ] = cExtra;
   }

   if ( pCount == 0 )
   {
      hb_retclenAdoptRaw( szData , ulMaxLen );
   }
   else
   {
      hb_itemReturn( pData );
   }
}

HB_FUNC( GTGETCLIPBOARDSIZE )
{
   hb_retnl(hb_gtGetClipboardSize());
}

HB_FUNC( GTPASTECLIPBOARD )
{
   hb_gtPasteFromClipboard( hb_parnl(1) );
}

/************************************************************************************/

HB_FUNC( GTINFO )
{
   PHB_ITEM pInfo = hb_param( 1, HB_IT_NUMERIC );
   PHB_ITEM pSet = hb_param( 2, HB_IT_NUMERIC );

   /* Parameter error */
   if ( pInfo == NULL )
   {
      hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, "GTINFO", 3,
         hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError( 3 ) );
      return;
   }

   /* Is it a query? */
   if ( pSet == NULL && !ISCHAR(2) )
   {
      /* Parameter void * still unused, for future developement. */
      hb_retni( HB_GT_FUNC( gt_info( hb_itemGetNI( pInfo ), FALSE, 0, NULL ) ) );
   }
   else
   {
      /* Parameter void * still unused, for future developement. */
      char *param = ISCHAR(2) ? hb_parc(2) : hb_parc(3);

      hb_retni( HB_GT_FUNC( gt_info( hb_itemGetNI( pInfo ), TRUE,
                hb_itemGetNI( pSet ), param ) ) );
   }

}

HB_FUNC( GFXPRIMITIVE )
{
   PHB_ITEM pType   = hb_param( 1, HB_IT_NUMERIC );
   PHB_ITEM pTop    = hb_param( 2, HB_IT_NUMERIC );
   PHB_ITEM pLeft   = hb_param( 3, HB_IT_NUMERIC );
   PHB_ITEM pBottom = hb_param( 4, HB_IT_NUMERIC );
   PHB_ITEM pRight  = hb_param( 5, HB_IT_NUMERIC );
   PHB_ITEM pColor  = hb_param( 6, HB_IT_NUMERIC );

   hb_retni( HB_GT_FUNC( gt_gfxPrimitive( hb_itemGetNI(pType), hb_itemGetNI(pTop), hb_itemGetNI(pLeft), hb_itemGetNI(pBottom), hb_itemGetNI(pRight), hb_itemGetNI(pColor) ) ) );
}

HB_FUNC( GFXTEXT )
{
   PHB_ITEM pTop    = hb_param( 1, HB_IT_NUMERIC );
   PHB_ITEM pLeft   = hb_param( 2, HB_IT_NUMERIC );
   char *cText      = hb_parc(3);
   PHB_ITEM pColor  = hb_param( 4, HB_IT_NUMERIC );
   PHB_ITEM pSize   = hb_param( 5, HB_IT_NUMERIC );
   PHB_ITEM pWidth  = hb_param( 6, HB_IT_NUMERIC );

   HB_GT_FUNC( gt_gfxText( hb_itemGetNI(pTop), hb_itemGetNI(pLeft), cText, hb_itemGetNI(pColor), hb_itemGetNI(pSize), hb_itemGetNI(pWidth) ) );
}
