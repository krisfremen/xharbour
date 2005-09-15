/*
 * $Id: extend.c,v 1.51 2005/04/11 01:46:35 druzus Exp $
 */

/*
 * Harbour Project source code:
 * The Extend API
 *
 * Copyright 1999 Antonio Linares <alinares@fivetech.com>
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
 *    hb_retnlen()
 *    hb_retnilen()
 *    hb_retnllen()
 *    hb_retndlen()
 *    hb_retdl()
 *
 * Copyright 2000 Jose Lalin <dezac@corevia.com>
 *    hb_retd()
 *
 * Copyright 2002 Marek Paliwoda <paliwoda@inetia.pl>
 *    hb_parptr()
 *    hb_retptr()
 *
 * Copyright 2003 Giancarlo Niccolai <antispam (at) niccolai [dot] ws>
 *    hb_parpointer()
 *
 * See doc/license.txt for licensing terms.
 *
 */

#define HB_THREAD_OPTIMIZE_STACK

#include "hbapi.h"
#include "hbapiitm.h"
#include "hbset.h"
#include "hbdate.h"
#include "thread.h"

/* NOTE: iParam = -1 can be used to access the return value. */
/* NOTE: iParam = 0 can be used to access the SELF object. */

HB_EXTERN_BEGIN

PHB_ITEM HB_EXPORT hb_param( int iParam, int iMask )
{
   HB_THREAD_STUB

   HB_TRACE(HB_TR_DEBUG, ("hb_param(%d, %d)", iParam, iMask));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam  );

      if( pItem->type & HB_IT_BYREF )
      {
         pItem = hb_itemUnRef( pItem );

         if( iMask == HB_IT_BYREF )
         {
            return pItem;
         }
      }

      if( ( USHORT ) iMask == HB_IT_ANY || pItem->type & ( USHORT ) iMask )
      {
         return pItem;
      }
      else
      {
         if( iMask == HB_IT_NUMERIC && HB_IS_NUMERIC( pItem ) )
         {
            return pItem;
         }
      }
   }

   return NULL;
}

PHB_ITEM  HB_EXPORT hb_paramError( int iParam )
{
   static HB_ITEM s_NIL;

   PHB_ITEM pParam = hb_param( iParam, HB_IT_ANY );

   if( pParam == NULL )
   {
      ( &s_NIL )->type = HB_IT_NIL;
      pParam = &s_NIL;
   }

   return pParam;
}

/* function to be called from pcode DLLs to detect if the extend system
 * is going to use an array item */

BOOL HB_EXPORT hb_extIsArray( int iParam )
{
   HB_THREAD_STUB

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );

      return HB_IS_ARRAY( pItem );
   }
   else
      return FALSE;
}

/* NOTE: Caller should not modify the buffer returned by this function.
         [vszakats] */

char HB_EXPORT * hb_parc( int iParam, ... )
{
   HB_THREAD_STUB

   HB_TRACE(HB_TR_DEBUG, ("hb_parc(%d, ...)", iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );

      if( HB_IS_BYREF( pItem ) )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_STRING( pItem ) )
      {
         return pItem->item.asString.value;
      }
      else if( HB_IS_ARRAY( pItem ) )
      {
         va_list va;
         ULONG ulArrayIndex;

         va_start( va, iParam );
         ulArrayIndex = va_arg( va, ULONG );
         va_end( va );

         return hb_arrayGetCPtr( pItem, ulArrayIndex );
      }
   }

   return NULL;
}

char HB_EXPORT * hb_parcx( int iParam, ... )
{
   HB_THREAD_STUB

   HB_TRACE(HB_TR_DEBUG, ("hb_parcx(%d, ...)", iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );

      if( HB_IS_BYREF( pItem ) )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_STRING( pItem ) )
      {
         return pItem->item.asString.value;
      }
      else if( HB_IS_ARRAY( pItem ) )
      {
         va_list va;
         ULONG ulArrayIndex;

         va_start( va, iParam );
         ulArrayIndex = va_arg( va, ULONG );
         va_end( va );

         return hb_arrayGetCPtr( pItem, ulArrayIndex );
      }
   }

   return "";
}

ULONG  HB_EXPORT hb_parclen( int iParam, ... )
{
   HB_THREAD_STUB

   HB_TRACE(HB_TR_DEBUG, ("hb_parclen(%d, ...)", iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );

      if( HB_IS_BYREF( pItem ) )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_STRING( pItem ) )
      {
         return pItem->item.asString.length;
      }
      else if( HB_IS_ARRAY( pItem ) )
      {
         va_list va;
         ULONG ulArrayIndex;

         va_start( va, iParam );
         ulArrayIndex = va_arg( va, ULONG );
         va_end( va );

         return hb_arrayGetCLen( pItem, ulArrayIndex );
      }
   }

   return 0;
}

/* NOTE: Similar to _parclen() but returns the length including the
         terminating zero byte, and it only works for parameters passed by
         reference. [vszakats] */

ULONG  HB_EXPORT hb_parcsiz( int iParam, ... )
{
   HB_THREAD_STUB

   HB_TRACE(HB_TR_DEBUG, ("hb_parcsiz(%d, ...)", iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );

      /* NOTE: hb_parcsiz() will only work for strings passed by reference.
               CA-Cl*pper works like this. [vszakats] */

      if( HB_IS_BYREF( pItem ) )
      {
         pItem = hb_itemUnRef( pItem );

         if( HB_IS_STRING( pItem ) )
         {
            return pItem->item.asString.length + 1;
         }
         else if( HB_IS_ARRAY( pItem ) )
         {
            va_list va;
            ULONG ulArrayIndex;

            va_start( va, iParam );
            ulArrayIndex = va_arg( va, ULONG );
            va_end( va );

            return hb_arrayGetCLen( pItem, ulArrayIndex ) + 1;
         }
      }
   }

   return 0;
}

/* NOTE: Using HB_VM_STACK.szDate as a temporary date buffer guaranties
         good behavior when multithreading. */

char  HB_EXPORT * hb_pards( int iParam, ... )
{
   HB_THREAD_STUB

   HB_TRACE(HB_TR_DEBUG, ("hb_pards(%d, ...)", iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );

      if( HB_IS_BYREF( pItem ) )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_DATE( pItem ) )
      {
         return hb_dateDecStr( HB_VM_STACK.szDate, pItem->item.asDate.value );
      }
      else if( HB_IS_ARRAY( pItem ) )
      {
         va_list va;
         ULONG ulArrayIndex;

         va_start( va, iParam );
         ulArrayIndex = va_arg( va, ULONG );
         va_end( va );

         return hb_arrayGetDS( pItem, ulArrayIndex, HB_VM_STACK.szDate );
      }
   }

   return hb_dateDecStr( HB_VM_STACK.szDate, 0 );
}

/* NOTE: szDate must be a 9 chars wide buffer. [vszakats] */

char  HB_EXPORT * hb_pardsbuff( char * szDate, int iParam, ... )
{
   HB_THREAD_STUB

   HB_TRACE(HB_TR_DEBUG, ("hb_pardsbuff(%p, %d, ...)", szDate, iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );

      if( HB_IS_BYREF( pItem ) )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_DATE( pItem ) )
      {
         return hb_dateDecStr( szDate, pItem->item.asDate.value );
      }
      else if( HB_IS_ARRAY( pItem ) )
      {
         va_list va;
         ULONG ulArrayIndex;

         va_start( va, iParam );
         ulArrayIndex = va_arg( va, ULONG );
         va_end( va );

         return hb_arrayGetDS( pItem, ulArrayIndex, szDate );
      }
   }

   return hb_dateDecStr( szDate, 0 );
}

/* retrieve a date as long integer - number of days from Julian's day */

LONG  HB_EXPORT hb_pardl( int iParam, ... )
{
   HB_THREAD_STUB

   HB_TRACE(HB_TR_DEBUG, ("hb_pardl(%d, ...)", iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );

      if( HB_IS_BYREF( pItem ) )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_DATE( pItem ) )
      {
         return pItem->item.asDate.value;
      }
      else if( HB_IS_ARRAY( pItem ) )
      {
         va_list va;
         ULONG ulArrayIndex;

         va_start( va, iParam );
         ulArrayIndex = va_arg( va, ULONG );
         va_end( va );

         return hb_arrayGetDL( pItem, ulArrayIndex );
      }
   }

   return hb_itemGetDL( NULL );
}

int  HB_EXPORT hb_parl( int iParam, ... )
{
   HB_THREAD_STUB

   HB_TRACE(HB_TR_DEBUG, ("hb_parl(%d, ...)", iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );

      if( HB_IS_BYREF( pItem ) )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_LOGICAL( pItem ) )
      {
         return pItem->item.asLogical.value ? 1 : 0;
      }
      else if( HB_IS_INTEGER( pItem ) )
      {
         return pItem->item.asInteger.value != 0 ? 1 : 0;
      }
      else if( HB_IS_LONG( pItem ) )
      {
         return pItem->item.asLong.value != 0 ? 1 : 0;
      }
      else if( HB_IS_DOUBLE( pItem ) )
      {
         return pItem->item.asDouble.value != 0.0 ? 1 : 0;
      }
      else if( HB_IS_STRING( pItem ) && pItem->item.asString.length == 1 )
      {
         return ( BYTE ) pItem->item.asString.value[0];
      }
      else if( HB_IS_ARRAY( pItem ) )
      {
         va_list va;
         ULONG ulArrayIndex;

         va_start( va, iParam );
         ulArrayIndex = va_arg( va, ULONG );
         va_end( va );

         return hb_arrayGetL( pItem, ulArrayIndex ) ? 1 : 0;
      }
   }

   return 0;
}

double  HB_EXPORT hb_parnd( int iParam, ... )
{
   HB_THREAD_STUB

   HB_TRACE(HB_TR_DEBUG, ("hb_parnd(%d, ...)", iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );

      if( HB_IS_BYREF( pItem ) )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_DOUBLE( pItem ) )
      {
         return pItem->item.asDouble.value;
      }
      else if( HB_IS_INTEGER( pItem ) )
      {
         return ( double ) pItem->item.asInteger.value;
      }
      else if( HB_IS_LONG( pItem ) )
      {
         return ( double ) pItem->item.asLong.value;
      }
      else if( HB_IS_LOGICAL( pItem ) )
      {
         return ( double ) pItem->item.asLogical.value;
      }
      else if( HB_IS_STRING( pItem ) && pItem->item.asString.length == 1 )
      {
         return ( double ) ( BYTE ) pItem->item.asString.value[0];
      }
      else if( HB_IS_ARRAY( pItem ) )
      {
         va_list va;
         ULONG ulArrayIndex;

         va_start( va, iParam );
         ulArrayIndex = va_arg( va, ULONG );
         va_end( va );

         return hb_arrayGetND( pItem, ulArrayIndex );
      }
   }

   return 0;
}

int  HB_EXPORT hb_parni( int iParam, ... )
{
   HB_THREAD_STUB

   HB_TRACE(HB_TR_DEBUG, ("hb_parni(%d, ...)", iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );

      if( HB_IS_BYREF( pItem ) )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_INTEGER( pItem ) )
      {
         return pItem->item.asInteger.value;
      }
      else if( HB_IS_LONG( pItem ) )
      {
         return ( int ) pItem->item.asLong.value;
      }
      else if( HB_IS_DOUBLE( pItem ) )
      {
         return ( int ) pItem->item.asDouble.value;
      }
      else if( HB_IS_LOGICAL( pItem ) )
      {
         return ( int ) pItem->item.asLogical.value;
      }
      else if( HB_IS_STRING( pItem ) && pItem->item.asString.length == 1 )
      {
         return ( int ) ( BYTE ) pItem->item.asString.value[0];
      }
      else if( HB_IS_ARRAY( pItem ) )
      {
         va_list va;
         ULONG ulArrayIndex;

         va_start( va, iParam );
         ulArrayIndex = va_arg( va, ULONG );
         va_end( va );

         return hb_arrayGetNI( pItem, ulArrayIndex );
      }
   }

   return 0;
}

LONG  HB_EXPORT hb_parnl( int iParam, ... )
{
   HB_THREAD_STUB

   HB_TRACE(HB_TR_DEBUG, ("hb_parnl(%d, ...)", iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );

      if( HB_IS_BYREF( pItem ) )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_LONG( pItem ) )
      {
         return ( LONG ) pItem->item.asLong.value;
      }
      else if( HB_IS_INTEGER( pItem ) )
      {
         return ( LONG ) pItem->item.asInteger.value;
      }
      else if( HB_IS_DOUBLE( pItem ) )
      {
#ifdef __GNUC__
         return ( LONG ) ( ULONG ) pItem->item.asDouble.value;
#else
         return ( LONG ) pItem->item.asDouble.value;
#endif
      }
      else if( HB_IS_DATE( pItem ) )
      {
         return pItem->item.asDate.value;
      }
      else if( HB_IS_LOGICAL( pItem ) )
      {
         return ( LONG ) pItem->item.asLogical.value;
      }
      else if( HB_IS_STRING( pItem ) && pItem->item.asString.length == 1 )
      {
         return ( LONG ) ( BYTE ) pItem->item.asString.value[0];
      }
      else if( HB_IS_ARRAY( pItem ) )
      {
         va_list va;
         ULONG ulArrayIndex;

         va_start( va, iParam );
         ulArrayIndex = va_arg( va, ULONG );
         va_end( va );

         return hb_arrayGetNL( pItem, ulArrayIndex );
      }
   }

   return 0;
}

HB_LONG  HB_EXPORT hb_parnint( int iParam, ... )
{
   HB_THREAD_STUB

   HB_TRACE(HB_TR_DEBUG, ("hb_parnl(%d, ...)", iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );

      if( HB_IS_BYREF( pItem ) )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_LONG( pItem ) )
      {
         return ( HB_LONG ) pItem->item.asLong.value;
      }
      else if( HB_IS_INTEGER( pItem ) )
      {
         return ( HB_LONG ) pItem->item.asInteger.value;
      }
      else if( HB_IS_DOUBLE( pItem ) )
      {
#ifdef __GNUC__
         return ( HB_LONG ) ( HB_ULONG ) pItem->item.asDouble.value;
#else
         return ( HB_LONG ) pItem->item.asDouble.value;
#endif
      }
      else if( HB_IS_DATE( pItem ) )
      {
         return ( HB_LONG ) pItem->item.asDate.value;
      }
      else if( HB_IS_LOGICAL( pItem ) )
      {
         return ( HB_LONG ) pItem->item.asLogical.value;
      }
      else if( HB_IS_STRING( pItem ) && pItem->item.asString.length == 1 )
      {
         return ( HB_LONG ) ( BYTE ) pItem->item.asString.value[0];
      }
      else if( HB_IS_ARRAY( pItem ) )
      {
         va_list va;
         ULONG ulArrayIndex;

         va_start( va, iParam );
         ulArrayIndex = va_arg( va, ULONG );
         va_end( va );

         return hb_arrayGetNInt( pItem, ulArrayIndex );
      }
   }

   return 0;
}

/* NEW function - to retrieve a pointer from a harbour level */
void *hb_parptr( int iParam, ... )
{
   HB_THREAD_STUB

   HB_TRACE(HB_TR_DEBUG, ("hb_parptr(%d, ...)", iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );

      if( HB_IS_BYREF( pItem ) )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_POINTER( pItem ) )
      {
         return pItem->item.asPointer.value;
      }
      else if( HB_IS_ARRAY( pItem ) )
      {
         va_list va;
         ULONG ulArrayIndex;

         va_start( va, iParam );
         ulArrayIndex = va_arg( va, ULONG );
         va_end( va );

         return hb_arrayGetPtr( pItem, ulArrayIndex );
      }
   }

   return ( void * )0;
}

void *hb_parpointer( int iParam )
{
#ifdef HB_API_MACROS
   HB_THREAD_STUB
#endif

   HB_TRACE(HB_TR_DEBUG, ("hb_parptr(%d, ...)", iParam));

   if( iParam >= 1 && iParam <= hb_pcount()  )
   {
      PHB_ITEM pItem = hb_stackItemFromBase( iParam );

      if( HB_IS_BYREF( pItem ) )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_POINTER( pItem ) )
      {
         return pItem->item.asPointer.value;
      }
   }

   return NULL;
}

ULONG  HB_EXPORT hb_parinfa( int iParamNum, ULONG uiArrayIndex )
{
   PHB_ITEM pArray;

   HB_TRACE(HB_TR_DEBUG, ("hb_parinfa(%d, %lu)", iParamNum, uiArrayIndex));

   pArray = hb_param( iParamNum, HB_IT_ARRAY );

   if( pArray )
   {
      if( uiArrayIndex == 0 )
         return pArray->item.asArray.value->ulLen;
      else
         return ( ULONG ) hb_arrayGetType( pArray, uiArrayIndex );
   }
   else
      return 0;
}

int  HB_EXPORT hb_parinfo( int iParam )
{
   HB_THREAD_STUB

   HB_TRACE(HB_TR_DEBUG, ("hb_parinfo(%d)", iParam));

   if( iParam == 0 )
   {
      return ( int ) hb_pcount();
   }
   else
   {
      if( ( iParam > 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
      {
         USHORT uiType = ( iParam == -1 ) ? HB_VM_STACK.Return.type : ( hb_stackItemFromBase( iParam ) )->type;

         if( uiType & HB_IT_BYREF )
         {
            PHB_ITEM pItem = hb_itemUnRef( ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam ) );

            if( pItem )
            {
               uiType |= pItem->type;
            }
         }

         return ( int ) uiType;
      }
      else
         return 0;
   }
}

#undef hb_pcount
int  HB_EXPORT hb_pcount( void )
{
   HB_THREAD_STUB

   HB_TRACE(HB_TR_DEBUG, ("hb_pcount()"));

   return ( int )( ( *( HB_VM_STACK.pBase ) )->item.asSymbol.paramcnt < 255 ? ( *( HB_VM_STACK.pBase ) )->item.asSymbol.paramcnt : ( *( HB_VM_STACK.pBase ) )->item.asSymbol.paramcnt - 256 );
}

#undef hb_ret
void  HB_EXPORT hb_ret( void )
{
   HB_THREAD_STUB

   HB_TRACE(HB_TR_DEBUG, ("hb_ret()"));

   if( HB_IS_COMPLEX( &(HB_VM_STACK.Return) ) )
   {
      hb_itemClear( &(HB_VM_STACK.Return) );
   }
   else
   {
      ( &(HB_VM_STACK.Return) )->type = HB_IT_NIL;
   }
}

/* JC1: tunrning off optimization; from now on, HB_VM_STACK is referenced just once */
#if defined( HB_THREAD_SUPPORT )
   #undef HB_VM_STACK
   #define HB_VM_STACK (* hb_threadGetCurrentStack() )
#endif

#undef hb_reta
void  HB_EXPORT hb_reta( ULONG ulLen )  /* undocumented hb_reta() */
{
   HB_TRACE(HB_TR_DEBUG, ("hb_reta(%lu)", ulLen));

   hb_arrayNew( &(HB_VM_STACK.Return), ulLen );
}

#undef hb_retc
void HB_EXPORT  hb_retc( const char * szText )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_retc(%s)", szText));

   hb_itemPutC( &(HB_VM_STACK.Return), szText );
}

#undef hb_retclen
void  HB_EXPORT hb_retclen( const char * szText, ULONG ulLen )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_retclen(%s, %lu)", szText, ulLen));

   hb_itemPutCL( &(HB_VM_STACK.Return), szText, ulLen );
}

/* szDate must have YYYYMMDD format */

#undef hb_retds
void HB_EXPORT hb_retds( const char * szDate )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_retds(%s)", szDate));

   hb_itemPutDS( &(HB_VM_STACK.Return), szDate );
}

#undef hb_retd
void HB_EXPORT hb_retd( int iYear, int iMonth, int iDay )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_retd(%04i, %02i, %02i)", iYear, iMonth, iDay));

   hb_itemPutD( &(HB_VM_STACK.Return), iYear, iMonth, iDay );
}

#undef hb_retdl
void HB_EXPORT hb_retdl( LONG lJulian )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_retdl(%ld)", lJulian));

   hb_itemPutDL( &(HB_VM_STACK.Return), lJulian );
}

#undef hb_retl
void HB_EXPORT hb_retl( int iLogical )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_retl(%d)", iLogical));

   hb_itemPutL( &(HB_VM_STACK.Return), iLogical ? TRUE : FALSE );
}

#undef hb_retnd
void HB_EXPORT hb_retnd( double dNumber )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_retnd(%lf)", dNumber));

   hb_itemPutND( &(HB_VM_STACK.Return), dNumber );
}

#undef hb_retni
void HB_EXPORT hb_retni( int iNumber )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_retni(%d)", iNumber));

   hb_itemPutNI( &(HB_VM_STACK.Return), iNumber );
}

#undef hb_retnl
void HB_EXPORT hb_retnl( LONG lNumber )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_retnl(%ld)", lNumber));

   hb_itemPutNL( &(HB_VM_STACK.Return), lNumber );
}

#undef hb_retnlen
void HB_EXPORT hb_retnlen( double dNumber, int iWidth, int iDec )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_retnlen(%lf, %d, %d)", dNumber, iWidth, iDec));

   hb_itemPutNLen( &(HB_VM_STACK.Return), dNumber, iWidth, iDec );
}

#undef hb_retndlen
void HB_EXPORT hb_retndlen( double dNumber, int iWidth, int iDec )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_retndlen(%lf, %d, %d)", dNumber, iWidth, iDec));

   hb_itemPutNDLen( &(HB_VM_STACK.Return), dNumber, iWidth, iDec );
}

#undef hb_retnilen
void HB_EXPORT hb_retnilen( int iNumber, int iWidth )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_retnilen(%d, %d)", iNumber, iWidth));

   hb_itemPutNILen( &(HB_VM_STACK.Return), iNumber, iWidth );
}

#undef hb_retnllen
void HB_EXPORT hb_retnllen( LONG lNumber, int iWidth )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_retnllen(%ld, %d)", lNumber, iWidth));

   hb_itemPutNLLen( &(HB_VM_STACK.Return), lNumber, iWidth );
}

/* NEW function - to return a pointer to a harbour level */
#undef hb_retptr
void HB_EXPORT hb_retptr( void * voidPtr )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_retptr(%p)", voidPtr));

   hb_itemPutPtr( &(HB_VM_STACK.Return), voidPtr );
}

#undef hb_retptrGC
void HB_EXPORT hb_retptrGC( void * voidPtr )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_retptrGC(%p)", voidPtr));

   hb_itemPutPtrGC( &(HB_VM_STACK.Return), voidPtr );
}

#undef hb_retnint
void HB_EXPORT hb_retnint( HB_LONG lNumber )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_retnint(%" PFHL "d)", lNumber));

   hb_itemPutNInt( &(HB_VM_STACK.Return), lNumber );
}

#undef hb_retnintlen
void HB_EXPORT hb_retnintlen( HB_LONG lNumber, int iWidth )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_retnintlen(%" PFHL "d, %d)", lNumber, iWidth));

   hb_itemPutNIntLen( &(HB_VM_STACK.Return), lNumber, iWidth );
}

void HB_EXPORT hb_storc( char * szText, int iParam, ... )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_storc(%s, %d, ...)", szText, iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );
      BOOL bByRef = HB_IS_BYREF( pItem );

      if( bByRef  )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_ARRAY( pItem ) )
      {
         PHB_ITEM pDstItem;
         va_list va;
         va_start( va, iParam );
         pDstItem = hb_arrayGetItemPtr( pItem, va_arg( va, ULONG ));
         if( pDstItem )
         {
            hb_itemPutC( pDstItem, szText );
         }
         va_end( va );
      }
      else if( bByRef || iParam == -1 )
      {
         hb_itemPutC( pItem, szText );
      }
   }
}

void HB_EXPORT hb_storclen( char * szText, ULONG ulLen, int iParam, ... )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_storclen(%s, %lu, %d, ...)", szText, ulLen, iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );
      BOOL bByRef = HB_IS_BYREF( pItem );

      if( bByRef  )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_ARRAY( pItem ) )
      {
         PHB_ITEM pDstItem;
         va_list va;
         va_start( va, iParam );
         pDstItem = hb_arrayGetItemPtr( pItem, va_arg( va, ULONG ));
         if( pDstItem )
         {
            hb_itemPutCL( pDstItem, szText, ulLen );
         }
         va_end( va );
      }
      else if( bByRef || iParam == -1 )
      {
         hb_itemPutCL( pItem, szText, ulLen );
      }
   }
}

void HB_EXPORT hb_storclenAdopt( char * szText, ULONG ulLen, int iParam, ... )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_storclen(%s, %lu, %d, ...)", szText, ulLen, iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );
      BOOL bByRef = HB_IS_BYREF( pItem );

      if( bByRef  )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_ARRAY( pItem ) )
      {
         PHB_ITEM pDstItem;
         va_list va;
         va_start( va, iParam );
         pDstItem = hb_arrayGetItemPtr( pItem, va_arg( va, ULONG ));
         if( pDstItem )
         {
            hb_itemPutCPtr( pDstItem, szText, ulLen );
         }
         va_end( va );
      }
      else if( bByRef || iParam == -1 )
      {
         hb_itemPutCPtr( pItem, szText, ulLen );
      }
   }
}

/* szDate must have YYYYMMDD format */

void HB_EXPORT hb_stords( char * szDate, int iParam, ... )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_stords(%s, %d, ...)", szDate, iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );
      BOOL bByRef = HB_IS_BYREF( pItem );

      if( bByRef  )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_ARRAY( pItem ) )
      {
         PHB_ITEM pDstItem;
         va_list va;
         va_start( va, iParam );
         pDstItem = hb_arrayGetItemPtr( pItem, va_arg( va, ULONG ));
         if( pDstItem )
         {
            hb_itemPutDS( pDstItem, szDate );
         }
         va_end( va );
      }
      else if( bByRef || iParam == -1 )
      {
         hb_itemPutDS( pItem, szDate );
      }
   }
}

void HB_EXPORT hb_stordl( LONG lJulian, int iParam, ... )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_stordl(%ld, %d, ...)", lJulian, iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );
      BOOL bByRef = HB_IS_BYREF( pItem );

      if( bByRef  )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_ARRAY( pItem ) )
      {
         PHB_ITEM pDstItem;
         va_list va;
         va_start( va, iParam );
         pDstItem = hb_arrayGetItemPtr( pItem, va_arg( va, ULONG ));
         if( pDstItem )
         {
            hb_itemPutDL( pDstItem, lJulian );
         }
         va_end( va );
      }
      else if( bByRef || iParam == -1 )
      {
         hb_itemPutDL( pItem, lJulian );
      }
   }
}

void HB_EXPORT hb_storl( int iLogical, int iParam, ... )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_storl(%d, %d, ...)", iLogical, iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );
      BOOL bByRef = HB_IS_BYREF( pItem );

      if( bByRef  )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_ARRAY( pItem ) )
      {
         PHB_ITEM pDstItem;
         va_list va;
         va_start( va, iParam );
         pDstItem = hb_arrayGetItemPtr( pItem, va_arg( va, ULONG ));
         if( pDstItem )
         {
            hb_itemPutL( pDstItem, iLogical ? TRUE : FALSE );
         }
         va_end( va );
      }
      else if( bByRef || iParam == -1 )
      {
         hb_itemPutL( pItem, iLogical ? TRUE : FALSE );
      }
   }
}

void HB_EXPORT hb_storni( int iValue, int iParam, ... )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_storni(%d, %d, ...)", iValue, iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );
      BOOL bByRef = HB_IS_BYREF( pItem );

      if( bByRef  )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_ARRAY( pItem ) )
      {
         PHB_ITEM pDstItem;
         va_list va;
         va_start( va, iParam );
         pDstItem = hb_arrayGetItemPtr( pItem, va_arg( va, ULONG ));
         if( pDstItem )
         {
            hb_itemPutNI( pDstItem, iValue );
         }
         va_end( va );
      }
      else if( bByRef || iParam == -1 )
      {
         hb_itemPutNI( pItem, iValue );
      }
   }
}

void HB_EXPORT hb_stornl( LONG lValue, int iParam, ... )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_stornl(%ld, %d, ...)", lValue, iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );
      BOOL bByRef = HB_IS_BYREF( pItem );

      if( bByRef  )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_ARRAY( pItem ) )
      {
         PHB_ITEM pDstItem;
         va_list va;
         va_start( va, iParam );
         pDstItem = hb_arrayGetItemPtr( pItem, va_arg( va, ULONG ));
         if( pDstItem )
         {
            hb_itemPutNL( pDstItem, lValue );
         }
         va_end( va );
      }
      else if( bByRef || iParam == -1 )
      {
         hb_itemPutNL( pItem, lValue );
      }
   }
}

void HB_EXPORT hb_stornint( HB_LONG lValue, int iParam, ... )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_stornint(%" PFHL "d, %d, ...)", lValue, iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &hb_stack.Return : hb_stackItemFromBase( iParam );
      BOOL bByRef = HB_IS_BYREF( pItem );

      if( bByRef  )
         pItem = hb_itemUnRef( pItem );

      if( HB_IS_ARRAY( pItem ) )
      {
         va_list va;
         PHB_ITEM pItemNew = hb_itemPutNInt( NULL, lValue );
         va_start( va, iParam );
         hb_arraySet( pItem, va_arg( va, ULONG ), pItemNew );
         va_end( va );
         hb_itemRelease( pItemNew );
      }
      else if( bByRef || iParam == -1 )
      {
         hb_itemPutNInt( pItem, lValue );
      }
   }
}

void HB_EXPORT hb_stornd( double dNumber, int iParam, ... )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_stornd(%lf, %d, ...)", dNumber, iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );
      BOOL bByRef = HB_IS_BYREF( pItem );

      if( bByRef  )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_ARRAY( pItem ) )
      {
         PHB_ITEM pDstItem;
         va_list va;
         va_start( va, iParam );
         pDstItem = hb_arrayGetItemPtr( pItem, va_arg( va, ULONG ));
         if( pDstItem )
         {
            hb_itemPutND( pDstItem, dNumber );
         }
         va_end( va );
      }
      else if( bByRef || iParam == -1 )
      {
         hb_itemPutND( pItem, dNumber );
      }
   }
}

void HB_EXPORT hb_storptr( void * pointer, int iParam, ... )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_storptr(%p, %d, ...)", pointer, iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &hb_stack.Return : hb_stackItemFromBase( iParam );
      BOOL bByRef = HB_IS_BYREF( pItem );

      if( bByRef  )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_ARRAY( pItem ) )
      {
         PHB_ITEM pDstItem;
         va_list va;
         va_start( va, iParam );
         pDstItem = hb_arrayGetItemPtr( pItem, va_arg( va, ULONG ));
         if( pDstItem )
         {
            hb_itemPutPtr( pDstItem, pointer );
         }
         va_end( va );
      }
      else if( bByRef || iParam == -1 )
      {
         hb_itemPutPtr( pItem, pointer );
      }
   }
}

#ifndef HB_LONG_LONG_OFF
/* LONGLONG support */
void HB_EXPORT hb_stornll( LONGLONG llNumber, int iParam, ... )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_stornll(%" PFLL "d, %d, ...)", llNumber, iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );
      BOOL bByRef = HB_IS_BYREF( pItem );

      if( bByRef  )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_ARRAY( pItem ) )
      {
         PHB_ITEM pDstItem;
         va_list va;
         va_start( va, iParam );
         pDstItem = hb_arrayGetItemPtr( pItem, va_arg( va, ULONG ));
         if( pDstItem )
         {
            hb_itemPutNLL( pDstItem, llNumber );
         }
         va_end( va );
      }
      else if( bByRef || iParam == -1 )
      {
        hb_itemPutNLL( pItem, llNumber );
      }
   }
}

LONGLONG  HB_EXPORT hb_parnll( int iParam, ... )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_parnll(%d, ...)", iParam));

   if( ( iParam >= 0 && iParam <= hb_pcount() ) || ( iParam == -1 ) )
   {
      PHB_ITEM pItem = ( iParam == -1 ) ? &(HB_VM_STACK.Return) : hb_stackItemFromBase( iParam );

      if( HB_IS_BYREF( pItem ) )
      {
         pItem = hb_itemUnRef( pItem );
      }

      if( HB_IS_DOUBLE( pItem ) )
      {
#ifdef __GNUC__
         return ( LONGLONG ) ( ULONGLONG ) pItem->item.asDouble.value;
#else
         return ( LONGLONG ) pItem->item.asDouble.value;
#endif
      }
      else if( HB_IS_INTEGER( pItem ) )
      {
         return ( LONGLONG ) pItem->item.asInteger.value;
      }
      else if( HB_IS_LONG( pItem ) )
      {
         return ( LONGLONG ) pItem->item.asLong.value;
      }
      else if( HB_IS_LOGICAL( pItem ) )
      {
         return ( LONGLONG ) pItem->item.asLogical.value;
      }
      else if( HB_IS_DATE( pItem ) )
      {
         return ( LONGLONG ) pItem->item.asDate.value;
      }
      else if( HB_IS_ARRAY( pItem ) )
      {
         va_list va;
         ULONG ulArrayIndex;

         va_start( va, iParam );
         ulArrayIndex = va_arg( va, ULONG );
         va_end( va );

         return hb_arrayGetNLL( pItem, ulArrayIndex );
      }
   }

   return 0;
}

#undef hb_retnll
void HB_EXPORT hb_retnll( LONGLONG llNumber )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_retnll(%" PFLL "d)", llNumber));

   hb_itemPutNLL( &(HB_VM_STACK.Return), llNumber );
}

#undef hb_retnlllen
void HB_EXPORT hb_retnlllen( LONGLONG llNumber, int iWidth)
{
   HB_TRACE(HB_TR_DEBUG, ("hb_retnlllen(%" PFLL "d, %d)", llNumber, iWidth));

   hb_itemPutNLLLen( &(HB_VM_STACK.Return), llNumber, iWidth);
}
#endif

HB_EXTERN_END
