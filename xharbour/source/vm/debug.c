/*
 * $Id: debug.c,v 1.14 2004/04/14 10:32:14 druzus Exp $
 */

/*
 * Harbour Project source code:
 * Debugging functions for LOCAL, STATIC variables and the stack
 *
 * Copyright 1999 Eddie Runia <eddie@runia.com>
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
#include "hbfast.h"
#include "hbapiitm.h"
#include "hbstack.h"
#include "hbapierr.h"

/* $Doc$
 * $FuncName$     AddToArray( <pItem>, <pReturn>, <uiPos> )
 * $Description$  Add <pItem> to array <pReturn> at pos <uiPos>
 * $End$ */
static void AddToArray( PHB_ITEM pItem, PHB_ITEM pReturn, ULONG ulPos )
{
   HB_ITEM Temp;

   HB_TRACE(HB_TR_DEBUG, ("AddToArray(%p, %p, %lu)", pItem, pReturn, ulPos));

   if( pItem->type == HB_IT_SYMBOL )
   {                                            /* Symbol is pushed as text */
	 (&Temp)->type = HB_IT_STRING;
      (&Temp)->item.asString.length = strlen( pItem->item.asSymbol.value->szName ) + 2;
      (&Temp)->item.asString.value = ( char * ) hb_xgrab( (&Temp)->item.asString.length + 1 );

      sprintf( (&Temp)->item.asString.value, "[%s]", pItem->item.asSymbol.value->szName );

      (&Temp)->item.asString.pulHolders      = ( HB_COUNTER * ) hb_xgrab( sizeof( HB_COUNTER ) );
      *( (&Temp)->item.asString.pulHolders ) = 1;
      (&Temp)->item.asString.bStatic         = FALSE;

      hb_arraySetForward( pReturn, ulPos, &Temp );
      // hb_itemRelease( pTemp );               /* Get rid of temporary str.*/
   }
   else                                         /* Normal types             */
   {
      hb_arraySet( pReturn, ulPos, pItem );
   }
}

/* $Doc$
 * $FuncName$     <nVars> hb_dbg_vmStkGCount()
 * $Description$  Returns the length of the global stack
 * $End$ */
static USHORT hb_stackLenGlobal( void )
{
   PHB_ITEM * pItem;
   USHORT uiCount = 0;

   HB_TRACE(HB_TR_DEBUG, ("hb_stackLenGlobal()"));

   for( pItem = HB_VM_STACK.pItems; pItem <= HB_VM_STACK.pPos; uiCount++ )
   {
      pItem++;
   }

   return uiCount;
}

HB_FUNC( HB_DBG_VMSTKGCOUNT )
{
   hb_retni( hb_stackLenGlobal() );
}

/* $Doc$
 * $FuncName$     <aStack> hb_dbg_vmStkGList()
 * $Description$  Returns the global stack
 * $End$ */
HB_FUNC( HB_DBG_VMSTKGLIST )
{
   HB_ITEM Return;
   PHB_ITEM * pItem;

   USHORT uiLen = hb_stackLenGlobal();
   USHORT uiPos = 1;

   Return.type = HB_IT_NIL;
   hb_arrayNew( &Return, uiLen );           /* Create a transfer array  */

   for( pItem = HB_VM_STACK.pItems; pItem <= HB_VM_STACK.pPos; pItem++ )
   {
      AddToArray( *pItem, &Return, uiPos++ );
   }

   hb_itemReturn( &Return );
}

/* $Doc$
 * $FuncName$     <nVars> hb_dbg_vmStkLCount( <nProcLevel> )
 * $Description$  Returns params plus locals amount of the nProcLevel function
 * $End$ */
static USHORT hb_stackLen( int iLevel )
{
   PHB_ITEM * pBase = HB_VM_STACK.pBase;
   USHORT uiCount = 0;

   HB_TRACE(HB_TR_DEBUG, ("hb_stackLen()"));

   while( ( iLevel-- > 0 ) && pBase != HB_VM_STACK.pItems )
   {
      uiCount = pBase - ( HB_VM_STACK.pItems + ( *pBase )->item.asSymbol.stackbase ) - 2;
      pBase = HB_VM_STACK.pItems + ( *pBase )->item.asSymbol.stackbase;
   }

   return uiCount;
}

HB_FUNC( HB_DBG_VMSTKLCOUNT )
{
   int iLevel = hb_parni( 1 ) + 1;

   hb_retni( hb_stackLen( iLevel ) );
}

/* $Doc$
 * $FuncName$     <aStack> hb_dbg_vmStkLList()
 * $Description$  Returns the stack of the calling function
 *                "[<symbol>]"  Means symbol.
 *
 *                [1]        Symbol of current function
 *                [2]        Self | NIL
 *                [3 .. x]   Parameters
 *                [x+1 .. y] Locals
 *                [y+1 ..]   Pushed data
 * $End$ */
HB_FUNC( HB_DBG_VMSTKLLIST )
{
   HB_ITEM Return;
   PHB_ITEM * pItem;
   PHB_ITEM * pBase = HB_VM_STACK.pItems + hb_stackBaseItem()->item.asSymbol.stackbase;

   USHORT uiLen = hb_stackLen( 1 );
   USHORT uiPos = 1;

   Return.type = HB_IT_NIL;
   hb_arrayNew( &Return, uiLen );           /* Create a transfer array  */

   for( pItem = pBase; pItem < HB_VM_STACK.pBase; pItem++ )
   {
      AddToArray( *pItem, &Return, uiPos++ );
   }

   hb_itemReturn( &Return );
}

/* $Doc$
 * $FuncName$     <aParam> hb_dbg_vmParLGet()
 * $Description$  Returns the passed parameters of the calling function
 * $End$ */
               /* TODO : put bLocals / bParams      */
               /* somewhere for declared parameters */
               /* and locals                        */
HB_FUNC( HB_DBG_VMPARLLIST )
{
   int iLevel = hb_parni( 1 ) + 1;
   PHB_ITEM * pBase = HB_VM_STACK.pBase;
   HB_ITEM Return;
   PHB_ITEM * pItem;
   USHORT uiLen, uiPos = 1;

   while( iLevel-- > 0 && pBase != HB_VM_STACK.pItems )
   {
      pBase = HB_VM_STACK.pItems + ( *pBase )->item.asSymbol.stackbase;
   }

   uiLen = ( * pBase )->item.asSymbol.paramcnt;
   if( uiLen > 255 )
   {
      uiLen -= 256;
   }

   Return.type = HB_IT_NIL;
   hb_arrayNew( &Return, uiLen );           /* Create a transfer array  */

   for( pItem = pBase + 2; uiLen--; pItem++ )
   {
      AddToArray( *pItem, &Return, uiPos++ );
   }

   hb_itemReturn( &Return );
}

static void hb_dbgStop(void)
{
}

HB_FUNC( HB_DBG_VMVARLGET )
{
   int iLevel = hb_parni( 1 ) + 1;
   int iLocal = hb_parni( 2 );
   PHB_ITEM * pBase = HB_VM_STACK.pBase;

   while( ( iLevel-- > 0 ) && pBase != HB_VM_STACK.pItems )
   {
      pBase = HB_VM_STACK.pItems + ( *pBase )->item.asSymbol.stackbase;
   }

   if( iLocal > SHRT_MAX )
   {
      hb_dbgStop();
      iLocal -= USHRT_MAX;
      iLocal--;
   }

   if( iLocal >= 0 )
   {
     hb_itemCopy( &(HB_VM_STACK.Return), hb_itemUnRef( *(pBase + 1 + iLocal) ) );
   }
   else
   {
     if( HB_IS_BLOCK( *(pBase+1) ) )
     {
        hb_itemCopy( &(HB_VM_STACK.Return), hb_codeblockGetVar( *(pBase+1), ( LONG ) iLocal ) );
     }
     else
     {
        hb_errRT_BASE( EG_ARG, 9999, NULL, "HB_DBG_VMVARLGET", 2, hb_paramError( 1 ), hb_paramError( 2 ) );
     }
   }
}

HB_FUNC( HB_DBG_VMVARLSET )
{
   int iLevel = hb_parni( 1 ) + 1;
   int iLocal = hb_parni( 2 );
   PHB_ITEM * pBase = HB_VM_STACK.pBase;
   PHB_ITEM pLocal;

   while( ( iLevel-- > 0 ) && pBase != HB_VM_STACK.pItems )
   {
      pBase = HB_VM_STACK.pItems + ( *pBase )->item.asSymbol.stackbase;
   }

   if( iLocal > SHRT_MAX )
   {
      iLocal -= USHRT_MAX;
      iLocal--;
   }
   if( iLocal >= 0 )
     pLocal = *(pBase + 1 + iLocal);
   else
     pLocal = hb_codeblockGetVar( *(pBase+1), ( LONG ) iLocal );

   hb_itemCopy( hb_itemUnRef(pLocal), *(HB_VM_STACK.pBase + 4) );
}

HB_FUNC( __VMSTKLCOUNT )
{
   HB_FUNCNAME(HB_DBG_VMSTKLCOUNT)();
}

HB_FUNC( __VMPARLLIST )
{
   HB_FUNCNAME(HB_DBG_VMPARLLIST)();
}

HB_FUNC( __VMSTKLLIST )
{
   HB_FUNCNAME(HB_DBG_VMSTKLLIST)();
}

HB_FUNC( __VMVARLGET )
{
   HB_FUNCNAME(HB_DBG_VMVARLGET)();
}

HB_FUNC( __VMVARLSET )
{
   HB_FUNCNAME(HB_DBG_VMVARLSET)();
}

HB_FUNC( __VMSTKGLIST)
{
   HB_FUNCNAME(HB_DBG_VMSTKGLIST)();
}

HB_FUNC( __VMSTKGCOUNT )
{
   HB_FUNCNAME(HB_DBG_VMSTKGCOUNT)();
}
