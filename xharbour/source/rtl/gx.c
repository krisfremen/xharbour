/*
 * $Id: gx.c,v 1.1.1.1 2001/12/21 10:41:43 ronpinkas Exp $
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
   hb_retl( hb_gtSetMode( ISNUM( 1 ) ? hb_parni( 1 ) : ( hb_gtMaxRow() + 1 ),
                          ISNUM( 2 ) ? hb_parni( 2 ) : ( hb_gtMaxCol() + 1 ) ) == 0 );
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
