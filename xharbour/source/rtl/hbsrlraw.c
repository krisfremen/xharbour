/*
 * $Id: hbsrlraw.c,v 1.23 2004/02/14 21:01:17 andijahja Exp $
 */

/*
 * Harbour Project source code:
 * Remote Procedure Call code
 *
 * Copyright 2003 Giancarlo Niccolai <giancarlo@niccolai.ws>
 * www - http://www.xharbour.org
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
#include "hbapiitm.h"
#include "hbstack.h"
#include "hbapierr.h"
#include "inet.h"

/* Returns a string containing 8 characters in network byte order
* HB_CreateLen8( nLen ) --> returns the bytes containing the code
*/

#ifndef HB_LONG_LONG_OFF
void hb_createlen8( BYTE *ret, ULONGLONG uRet )
#else
void hb_createlen8( BYTE *ret, ULONG uRet )
#endif
{
   int i;
   for( i = 7; i >= 0; i -- )
   {
      ret[i] = (BYTE) (uRet % 256 );
      uRet /= 256l;
   }
}

HB_FUNC( HB_CREATELEN8 )
{
   BYTE ret[8];

   if( ISNUM(1) )
   {
      #ifndef HB_LONG_LONG_OFF
      ULONGLONG uRet = (ULONGLONG) hb_parnll(1 );
      #else
      ULONG uRet = (ULONG) hb_parnl( 1 );
      #endif
      hb_createlen8( ret, uRet );
      hb_retclen( ( char *) ret, 8 );
   }
   else if( ISCHAR(1) )
   {
      PHB_ITEM pItem = hb_param(1, HB_IT_STRING);
      #ifndef HB_LONG_LONG_OFF
      ULONGLONG uRet = (ULONGLONG) hb_parnll( 2 );
      #else
      ULONG uRet = (ULONG) hb_parnl( 2 );
      #endif
      if( pItem->item.asString.length >= 8)
      {
         hb_createlen8( ( BYTE *) pItem->item.asString.value, uRet );
      }
   }

}


/* Returns a numeric length using the first 4 bytes of the given string
* HB_GetLen8( cStr ) --> nLength
*/
#ifndef HB_LONG_LONG_OFF
ULONGLONG hb_getlen8( BYTE *cStr )
{
   int i;
   ULONGLONG lFact = 1;
   ULONGLONG ulRet = 0;

   for (i = 7; i >= 0; i-- )
   {
      ulRet += (( ULONGLONG ) cStr[i]) * lFact;
      lFact *= 256l;
   }
   return ulRet;
}
#else
ULONG hb_getlen8( BYTE *cStr )
{
   int i;
   ULONG lFact = 1;
   ULONG ulRet = 0;

   for (i = 7; i >= 4; i-- )
   {
      ulRet += (( ULONG ) cStr[i]) * lFact;
      lFact *= 256l;
   }
   return ulRet;
}
#endif



HB_FUNC( HB_GETLEN8 )
{
   BYTE *cStr = (BYTE *) hb_parcx( 1 );
   if ( cStr == NULL || hb_parclen( 1 ) < 4 )
   {
      hb_retnl( -1l );
   }
   else
   {
#ifndef HB_LONG_LONG_OFF
      hb_retnll( hb_getlen8( cStr ) );
#else
      hb_retnl( hb_getlen8( cStr ) );
#endif
   }
}


/* Serializes a variable into a serialization stream, socket or string
* HB_SERIALIZE( oVariuous )--> cData
*/
HB_FUNC( HB_SERIALIZESIMPLE )
{
   PHB_ITEM pItem = hb_param( 1, HB_IT_ANY );

   BYTE *cRet;
   ULONG ulRet;

   if( pItem == NULL )
   {
      // TODO: error code
      hb_ret();
      return;
   }

   if( HB_IS_BYREF( pItem ) )
   {
      hb_itemUnRef( pItem );
   }

   if( HB_IS_MEMVAR( pItem ) )
   {
      HB_VALUE_PTR pValue;

      pValue = *( pItem->item.asMemvar.itemsbase ) + pItem->item.asMemvar.offset +
                  pItem->item.asMemvar.value;
      pItem = &pValue->item;
   }

   switch( pItem->type )
   {
      case HB_IT_STRING:
         ulRet = (ULONG) (pItem->item.asString.length + 9);
         cRet = (BYTE *) hb_xgrab( ulRet );
         cRet[0] = (BYTE) 'C';
         hb_createlen8( cRet + 1, pItem->item.asString.length );
         memcpy( cRet + 9, pItem->item.asString.value, pItem->item.asString.length );
      break;

      case HB_IT_LOGICAL:
         ulRet = 2;
         cRet = (BYTE *)hb_xgrab( 2 );
         cRet[0] = (BYTE)'L';
         cRet[1] = (BYTE) ( pItem->item.asLogical.value ? 'T' : 'F' );
      break;

      case HB_IT_INTEGER:
         ulRet = 10;
         cRet = (BYTE *) hb_xgrab( ulRet );
         cRet[0] = (BYTE)'N';
         cRet[1] = (BYTE)'I';
         hb_createlen8( cRet + 2, pItem->item.asInteger.value );
      break;

      case HB_IT_LONG:
         ulRet = 10;
         cRet = (BYTE *) hb_xgrab( ulRet );
         cRet[0] = (BYTE)'N';
         cRet[1] = (BYTE)'L';
         hb_createlen8( cRet + 2, pItem->item.asLong.value );
      break;

#ifndef HB_LONG_LONG_OFF
      case HB_IT_LONGLONG:
         ulRet = 10;
         cRet = (BYTE *) hb_xgrab( ulRet );
         cRet[0] = (BYTE)'N';
         cRet[1] = (BYTE)'X';
         hb_createlen8( cRet + 2, pItem->item.asLongLong.value );
      break;
#endif

      case HB_IT_DOUBLE:
         ulRet = 2 + sizeof( double );
         cRet = (BYTE *)hb_xgrab( ulRet );
         cRet[0] = (BYTE)'N';
         cRet[1] = (BYTE)'D';
         memcpy( cRet + 2, &(pItem->item.asDouble.value), sizeof( double ) );
      break;

      case HB_IT_DATE:
         ulRet = 9;
         cRet = (BYTE *)hb_xgrab( ulRet );
         cRet[0] = (BYTE)'D';
         hb_createlen8( cRet + 1, pItem->item.asDate.value );
      break;

      case HB_IT_NIL:
         ulRet = 1;
         cRet = (BYTE *)hb_xgrab( ulRet );
         cRet[0] = (BYTE)'Z';
      break;

      /* not implemented ? */
      default:
         hb_ret();
      return;
   }

   hb_retclenAdoptRaw( (char *)cRet, ulRet );
}

/* Deserializes a variable and get the value back
*/
HB_FUNC( HB_DESERIALIZESIMPLE )
{
   PHB_ITEM pItem = hb_param( 1, HB_IT_STRING );
   LONG ulMaxlen;
   ULONG ulData;
   char *cBuf;

   if ( ISNUM( 2 ) )
   {
      ulMaxlen = hb_parnl( 2 );
   }
   else
   {
      ulMaxlen = -1;
   }

   if( pItem == NULL )
   {
      // TODO: error code
      hb_ret();
      return;
   }

   cBuf = pItem->item.asString.value;

   switch( cBuf[0] )
   {
      case 'C':
         ulData = (ULONG) hb_getlen8( ( BYTE * )cBuf + 1 );
         if ( ulMaxlen > 0 && ulData > (ULONG) ulMaxlen )
         {
            hb_ret();
         }
         else
         {
            hb_retclen( cBuf + 9, ulData );
         }
      break;

      case 'L':
         hb_retl( cBuf[1] == 'T' );
      break;

      case 'N':
         if( cBuf[1] == 'I' )
         {
            ulData = (ULONG) hb_getlen8( ( BYTE * )cBuf + 2 );
            hb_retni( (int) ulData );
         }
         else if( cBuf[1] == 'L' )
         {
            ulData = (ULONG) hb_getlen8( ( BYTE * )cBuf + 2 );
            hb_retnl( (LONG) ulData );
         }
#ifndef HB_LONG_LONG_OFF
         else if( cBuf[1] == 'X' )
         {
            ulData = (ULONG) hb_getlen8( ( BYTE * )cBuf + 2 );
            hb_retnll( (LONGLONG) ulData );
         }
#endif
         else
         {
            hb_retnd( *((double *) (cBuf +2) ) );
         }
      break;

      case 'D':
         ulData = (ULONG) hb_getlen8( (BYTE *)(cBuf + 1) );
         hb_retdl( ulData );
      break;

      case 'Z':
         // ulData = 1;
         hb_ret();

      break;
   }
}


ULONG hb_serialNextRaw( char *cBuf )
{
   ULONG ulData, ulNext;
   ULONG ulCount;

   switch( cBuf[0] )
   {
      case 'C':
         ulData = (ULONG) hb_getlen8( ( BYTE * )cBuf + 1 );
      return ulData + 9;

      case 'L':
      return 2;

      case 'N':
         if( cBuf[1] == 'I' || cBuf[1] == 'X' || cBuf[1] == 'L' )
         {
            return 10;
         }
      return 2 + sizeof(double);

      case 'D':
      return 9;

      case 'A':
         ulData = ulNext = 9;
         ulCount = (ULONG) hb_getlen8( ( BYTE *) (cBuf + 1) );

         while ( ulCount > 0 )
         {
            cBuf += ulNext;
            ulNext = hb_serialNextRaw( cBuf );
            ulData += ulNext;
            ulCount --;
         }
      return ulData;

      case 'H':
         ulData = ulNext = 9;
         ulCount = (ULONG) hb_getlen8( ( BYTE *) (cBuf + 1) );

         while ( ulCount > 0 )
         {
            cBuf += ulNext;
            ulNext = hb_serialNextRaw( cBuf );
            cBuf += ulNext;
            ulData += ulNext;
            ulNext = hb_serialNextRaw( cBuf );
            ulData += ulNext;
            ulCount --;
         }
      return ulData;

      case 'O':
         ulNext = 9;
         ulCount = (ULONG) hb_getlen8( ( BYTE *) (cBuf + 1) );
         // remove class name
         ulNext += hb_serialNextRaw( ( char *) ( cBuf + 9 )  );
         ulData = ulNext;

         while ( ulCount > 0 )
         {
            // remove property name
            cBuf += ulNext;
            ulNext = hb_serialNextRaw( cBuf );
            ulData += ulNext;
            // remove property value
            cBuf += ulNext;
            ulNext = hb_serialNextRaw( cBuf );
            ulData += ulNext;
            ulCount --;
         }
      return ulData;

       case 'Q':
         /* ulNext = 9; */
         ulCount = (ULONG) hb_getlen8( ( BYTE *) (cBuf + 1) );
         return ulCount+9;

      case 'Z': return 1;
    }
    return 0;
}

HB_FUNC( HB_SERIALNEXT )
{
   PHB_ITEM pItem = hb_param( 1, HB_IT_STRING );
   char *cBuf;

   if( pItem == NULL )
   {
      // TODO: error code
      hb_ret();
      return;
   }

   cBuf = pItem->item.asString.value;

   hb_retnl( hb_serialNextRaw( cBuf ) );
}


HB_FUNC( HB_DESERIALBEGIN )
{
   BYTE *cBuf;
   PHB_ITEM pItem = hb_param( 1, HB_IT_STRING );
   if( pItem == NULL )
   {
      // TODO: error code
      hb_ret();
      return;
   }

   cBuf = (BYTE *) hb_xgrab( pItem->item.asString.length + 8 );
   hb_createlen8( cBuf, 9 );
   memcpy( cBuf+8, pItem->item.asString.value, pItem->item.asString.length );
   hb_retclenAdoptRaw( ( char *) cBuf, 8 + pItem->item.asString.length );

}


HB_FUNC( HB_DESERIALRESET )
{
   PHB_ITEM pItem = hb_param( 1, HB_IT_STRING );
   if( pItem == NULL )
   {
      // TODO: error code
      hb_ret();
      return;
   }
   hb_createlen8( ( BYTE *) pItem->item.asString.value, 9 );
}
