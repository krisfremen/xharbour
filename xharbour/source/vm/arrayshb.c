/*
 * $Id: arrayshb.c,v 1.60 2005/09/22 01:12:00 druzus Exp $
 */

/*
 * Harbour Project source code:
 * The Array API (Harbour level)
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
 * The following parts are Copyright of the individual authors.
 * www - http://www.xharbour.org
 *
 * Copyright 2001 Ron Pinkas <ron@profit-master.com>
 * HB_FUNC( HB_APARAMS )
 * HB_FUNC( HB_AEXPRESSIONS )
 *
 */

#include <ctype.h>

#include "hbvmopt.h"
#include "hbapi.h"
#include "hbvm.h"
#include "hbfast.h"
#include "hbstack.h"
#include "hbapiitm.h"
#include "hbapierr.h"
#include "hbapilng.h"
#include "cstruct.ch"


/* This function creates an array item using 'iDimension' as an index
 * to retrieve the number of elements from the parameter list.
 */
static void hb_arrayNewRagged( PHB_ITEM pArray, int iDimension )
{
   ULONG ulElements;

   HB_TRACE(HB_TR_DEBUG, ("hb_arrayNewRagged(%p, %d)", pArray, iDimension));

   ulElements = ( ULONG ) hb_parnl( iDimension );

   /* create an array */
   hb_arrayNew( pArray, ulElements );

   if( ++iDimension <= hb_pcount() )
   {
      /* call self recursively to create next dimensions
       */
      while( ulElements )
      {
         hb_arrayNewRagged( hb_arrayGetItemPtr( pArray, ulElements-- ), iDimension );
      }
   }
}

HB_FUNC( ARRAY )
{
   int iPCount = hb_pcount();

   if( iPCount > 0 )
   {
      BOOL bError = FALSE;
      int iParam;

      for( iParam = 1; iParam <= iPCount; iParam++ )
      {

         if ( ! hb_param( iParam, HB_IT_NUMERIC ) )
         // if( ! ISNUM( iParam ) )
         {
            bError = TRUE;
            break;
         }

         if( hb_parnl( iParam ) < 0 ) /* || hb_parnl( iParam ) <= 4096 */
         {
            hb_errRT_BASE( EG_BOUND, 1131, NULL, hb_langDGetErrorDesc( EG_ARRDIMENSION ), 1, hb_paramError( 1 ) );
            bError = TRUE;
            break;
         }
      }

      if( ! bError )
      {
         hb_arrayNewRagged( &(HB_VM_STACK.Return), 1 );
      }
   }
}

HB_FUNC( AADD )
{
   PHB_ITEM pArray = hb_param( 1, HB_IT_ARRAY );

   if( pArray )
   {
      PHB_ITEM pValue = hb_param( 2, HB_IT_ANY );

      if( pValue && hb_arrayAdd( pArray, pValue ) )
      {
         if( hb_stackItemFromBase( 2 )->type & HB_IT_BYREF )
         {
            hb_itemCopy( &(HB_VM_STACK.Return), pValue );
         }
         else
         {
            hb_itemForwardValue( &(HB_VM_STACK.Return), pValue );
         }
      }
      else
      {
         hb_errRT_BASE( EG_BOUND, 1187, NULL, "AADD", HB_MIN( hb_pcount(), 2 ), hb_paramError( 1 ), hb_paramError( 2 ) );
      }
   }
   else
   {
      hb_errRT_BASE_SubstR( EG_ARG, 1123, NULL, "AADD", HB_MIN( hb_pcount(), 2 ), hb_paramError(1), hb_paramError( 2 ) );
   }
}

HB_FUNC( HB_ARRAYID )  /* for debugging: returns the array's "address" so dual references to same array can be seen */
{
   PHB_ITEM pArray = hb_param( 1, HB_IT_ARRAY );

   if( pArray )
   {
      hb_retptr( ( void * ) pArray->item.asArray.value );
   }
   else
   {
      hb_retptr( NULL );
   }
}

/* NOTE: CA-Cl*pper 5.3 and older will return NIL on bad parameter, 5.3a,b
         will throw a runtime error. [vszakats] */

HB_FUNC( ASIZE )
{
   PHB_ITEM pArray = hb_param( 1, HB_IT_ARRAY );

   if( pArray && ISNUM( 2 ) )
   {
      LONG lSize = hb_parnl( 2 );

      hb_arraySize( pArray, HB_MAX( lSize, 0 ) );

      /* ASize() returns the array itself */
      if( hb_stackItemFromBase( 1 )->type & HB_IT_BYREF )
      {
         hb_itemCopy( &(HB_VM_STACK.Return), pArray );
      }
      else
      {
         hb_itemForwardValue( &(HB_VM_STACK.Return), pArray );
      }
   }
#ifdef HB_COMPAT_C53 /* From CA-Cl*pper 5.3a */
   else
   {
#if 0
      hb_errRT_BASE( EG_ARG, 2023, NULL, "ASIZE", HB_MIN( hb_pcount(), 2 ), hb_paramError( 1 ), hb_paramError( 2 ) );
#else
      if ( hb_pcount() == 0 )
      {
         HB_ITEM_NEW( Err1 );
         HB_ITEM_NEW( Err2 );

         hb_errRT_BASE( EG_ARG, 2023, NULL, "ASIZE", 2, &Err1, &Err2 );
      }
      else
      {
         hb_errRT_BASE( EG_ARG, 2023, NULL, "ASIZE", 2, hb_paramError( 1 ), hb_paramError( 2 ) );
      }
#endif
   }
#endif
}

HB_FUNC( ATAIL )
{
   PHB_ITEM pArray = hb_param( 1, HB_IT_ARRAY );

   if( pArray )
   {
      hb_arrayLast( pArray, &(HB_VM_STACK.Return) );
   }
}

HB_FUNC( AINS )
{
   PHB_ITEM pArray = hb_param( 1, HB_IT_ARRAY );

   if( pArray )
   {
    #ifndef HB_C52_STRICT
      PHB_ITEM pExtend = hb_param( 4, HB_IT_LOGICAL );

      if( pExtend && pExtend->item.asLogical.value )
      {
         hb_arraySize( pArray, pArray->item.asArray.value->ulLen + 1 );
      }
    #endif

      if( ISNUM( 2 ) )
      {
         hb_arrayIns( pArray, hb_parnl( 2 ) );
      }

    #ifndef HB_C52_STRICT
      if( hb_pcount() >= 3 )
      {
         hb_arraySet( pArray, hb_parnl( 2 ), hb_param( 3, HB_IT_ANY ) );
      }
    #endif

      /* AIns() returns the array itself */
      if( hb_stackItemFromBase( 1 )->type & HB_IT_BYREF )
      {
         hb_itemCopy( &(HB_VM_STACK.Return), pArray );
      }
      else
      {
         hb_itemForwardValue( &(HB_VM_STACK.Return), pArray );
      }
   }
}

HB_FUNC( ADEL )
{
   PHB_ITEM pArray = hb_param( 1, HB_IT_ARRAY );

   if( pArray )
   {
    #ifndef HB_C52_STRICT
      PHB_ITEM pShrink = hb_param( 3, HB_IT_LOGICAL );
    #endif

      if( pArray->item.asArray.value->ulLen )
      {
         if( ISNUM( 2 ) && hb_parnl( 2 ) )
         {
            hb_arrayDel( pArray, hb_parnl( 2 ) );
         }
         else
         {
            hb_arrayDel( pArray, 1 );
         }

         #ifndef HB_C52_STRICT
            if( pShrink && pShrink->item.asLogical.value )
            {
               hb_arraySize( pArray, pArray->item.asArray.value->ulLen - 1 );
            }
         #endif
      }

      /* ADel() returns the array itself */
      if( hb_stackItemFromBase( 1 )->type & HB_IT_BYREF )
      {
         hb_itemCopy( &(HB_VM_STACK.Return), pArray );
      }
      else
      {
         hb_itemForwardValue( &(HB_VM_STACK.Return), pArray );
      }
   }
}

HB_FUNC( AFILL )
{
   PHB_ITEM pArray = hb_param( 1, HB_IT_ARRAY );
   PHB_ITEM pValue = NULL;

   if( pArray )
   {
      pValue = hb_param( 2, HB_IT_ANY );

      if( pValue )
      {
         LONG ulStart = hb_parnl( 3 );
         LONG ulCount = hb_parnl( 4 );

         /* Explicy ulCount of 0 - Nothing to do! */
         if( ISNUM(4) && ulCount == 0 )
         {
            /* AFill() returns the array itself */
            if( hb_stackItemFromBase( 1 )->type & HB_IT_BYREF )
            {
               hb_itemCopy( &(HB_VM_STACK.Return), pArray );
            }
            else
            {
               hb_itemForwardValue( &(HB_VM_STACK.Return), pArray );
            }

            return;
         }

         if( ulStart == 0 )
         {
            /* Clipper allows Start to be of wrong type, or 0, and corrects it to 1. */
            ulStart = 1;
         }
         /* Clipper aborts if negative start. */
         else if( ulStart < 0 )
         {
            /* AFill() returns the array itself */
            if( hb_stackItemFromBase( 1 )->type & HB_IT_BYREF )
            {
               hb_itemCopy( &(HB_VM_STACK.Return), pArray );
            }
            else
            {
               hb_itemForwardValue( &(HB_VM_STACK.Return), pArray );
            }

            return;
         }

         if( ulCount == 0 )
         {
            /* Clipper allows the Count to be of wrong type, and corrects it to maximum elements. */
            ulCount = pArray->item.asArray.value->ulLen;
         }
         else if( ulCount < 0 )
         {
            if( ulStart == 1 )
            {
               /* Clipper allows the Count to be negative, if start is 1, and corrects it to maximum elements. */
               ulCount = pArray->item.asArray.value->ulLen;
            }
            /* Clipper aborts if negative count and start is not at 1. */
            else
            {
               /* AFill() returns the array itself */
               if( hb_stackItemFromBase( 1 )->type & HB_IT_BYREF )
               {
                  hb_itemCopy( &(HB_VM_STACK.Return), pArray );
               }
               else
               {
                  hb_itemForwardValue( &(HB_VM_STACK.Return), pArray );
               }

               return;
            }
         }

         hb_arrayFill( pArray, pValue, (ULONG) ulStart, (ULONG) ulCount );
      }

      /* AFill() returns the array itself */
      if( hb_stackItemFromBase( 1 )->type & HB_IT_BYREF )
      {
         hb_itemCopy( &(HB_VM_STACK.Return), pArray );
      }
      else
      {
         hb_itemForwardValue( &(HB_VM_STACK.Return), pArray );
      }
   }
   else
   {
      #ifdef HB_C52_STRICT
        /* NOTE: In CA-Cl*pper AFILL() is written in a manner that it will
               call AEVAL() to do the job, so the error (if any) will also be
               thrown by AEVAL().  [vszakats] */
        hb_errRT_BASE( EG_ARG, 2017, NULL, "AEVAL", HB_MIN( hb_pcount(), 4 ), hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError( 3 ), hb_paramError( 4 ) );
      #else
#if 0
        hb_errRT_BASE( EG_ARG, 9999, NULL, "AFILL", HB_MIN( hb_pcount(), 4 ), hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError( 3 ), hb_paramError( 4 ) );
#else
        HB_ITEM_NEW( Err1 );
        HB_ITEM_NEW( Err2 );
        HB_ITEM_NEW( Err3 );
        HB_ITEM_NEW( Err4 );

        hb_errRT_BASE( EG_ARG, 9999, NULL, "AFILL", 4, pArray ? hb_paramError( 1 ) : &Err1, pValue ? hb_paramError( 2 ) : &Err2, &Err3, &Err4 );
      #endif
#endif
   }
}

HB_FUNC( ASCAN )
{
   PHB_ITEM pArray = hb_param( 1, HB_IT_ARRAY );
   PHB_ITEM pValue = hb_param( 2, HB_IT_ANY );

   if( pArray && pValue )
   {
      ULONG ulStart   = hb_parnl( 3 );
      ULONG ulCount   = hb_parnl( 4 );
      BOOL bExact     = hb_parl( 5 );
      BOOL bAllowChar = hb_parl( 6 );

      hb_retnl( hb_arrayScan( pArray, pValue, ISNUM( 3 ) ? &ulStart : NULL, ISNUM( 4 ) ? &ulCount : NULL, bExact, bAllowChar ) );
   }
   else
   {
      hb_retnl( 0 );
   }
}

/* TODO: In Xbase++ fifth parameter determines whether array elements
         are passed by reference to the code block. [vszakats] */

HB_FUNC( AEVAL )
{
   PHB_ITEM pArray = hb_param( 1, HB_IT_ARRAY );
   PHB_ITEM pBlock = hb_param( 2, HB_IT_BLOCK );

   if( pArray && pBlock )
   {
      ULONG ulStart = hb_parnl( 3 );
      ULONG ulCount = hb_parnl( 4 );

      hb_arrayEval( pArray,
                    pBlock,
                    ISNUM( 3 ) ? &ulStart : NULL,
                    ISNUM( 4 ) ? &ulCount : NULL );

      /* AEval() returns the array itself */
      if( hb_stackItemFromBase( 1 )->type & HB_IT_BYREF )
      {
         hb_itemCopy( &(HB_VM_STACK.Return), pArray );
      }
      else
      {
         hb_itemForwardValue( &(HB_VM_STACK.Return), pArray );
      }
   }
   else
   {
      HB_ITEM_NEW( Err1 );
      HB_ITEM_NEW( Err2 );

      hb_errRT_BASE( EG_ARG, 2017, NULL, "AEVAL", 4, pArray ? hb_paramError( 1 ) : &Err1, pBlock ? hb_paramError( 2 ) : &Err2, hb_paramError( 3 ), hb_paramError( 4 ) );
   }
}

HB_FUNC( ACOPY )
{
   PHB_ITEM pSrcArray = hb_param( 1, HB_IT_ARRAY );
   PHB_ITEM pDstArray = hb_param( 2, HB_IT_ARRAY );

   if( pSrcArray && pDstArray )
   {
      /* CA-Cl*pper works this way. */
      if( ! hb_arrayIsObject( pSrcArray ) && ! hb_arrayIsObject( pDstArray ) )
      {
         ULONG ulStart = hb_parnl( 3 );
         ULONG ulCount = hb_parnl( 4 );
         ULONG ulTarget = hb_parnl( 5 );

         hb_arrayCopy( pSrcArray,
                       pDstArray,
                       ISNUM( 3 ) ? &ulStart : NULL,
                       ISNUM( 4 ) ? &ulCount : NULL,
                       ISNUM( 5 ) ? &ulTarget : NULL );
      }

      /* ACopy() returns the target array */
      if( hb_stackItemFromBase( 2 )->type & HB_IT_BYREF )
      {
         hb_itemCopy( &(HB_VM_STACK.Return), pDstArray );
      }
      else
      {
         hb_itemForwardValue( &(HB_VM_STACK.Return), pDstArray );
      }
   }
}

/* NOTE: Clipper will return NIL if the parameter is not an array. [vszakats] */

HB_FUNC( ACLONE )
{
   PHB_ITEM pSrcArray = hb_param( 1, HB_IT_ARRAY );

   if( pSrcArray && ! hb_arrayIsObject( pSrcArray ) )
   {
      hb_itemRelease( hb_itemReturnForward( hb_arrayClone( pSrcArray, NULL ) ) ); /* AClone() returns the new array */
   }
}

HB_FUNC( HB_APARAMS )
{
   PHB_ITEM * pBase = HB_VM_STACK.pBase;

   pBase = HB_VM_STACK.pItems + ( *pBase )->item.asSymbol.stackbase;

   hb_itemRelease( hb_itemReturnForward( hb_arrayFromParams( pBase ) ) );
}

HB_FUNC( HB_AEXPRESSIONS )
{
   PHB_ITEM pLine  = hb_param( 1, HB_IT_STRING );
   size_t i, iOffset = 0;
   int iParans = 0, iArrays = 0, iIndexs = 0;
   BOOL bArray = FALSE;

   if( pLine == NULL )
   {
      hb_errRT_BASE_SubstR( EG_ARG, 1123, NULL, "HB_AEXPRESSIONS", 1, hb_paramError(1) );
      return;
   }

   HB_VM_STACK.Return.type = HB_IT_NIL;
   hb_arrayNew( &(HB_VM_STACK.Return), 0 );

   for( i = 0; i < pLine->item.asString.length; i++ )
   {
      switch( pLine->item.asString.value[i] )
      {
         case '(' :
            iParans++;
            bArray = FALSE;
            break;

         case ')' :
            iParans--;
            bArray = TRUE;
            break;

         case '{' :
            iArrays++;
            bArray = FALSE;
            break;

         case '}' :
            iArrays--;
            bArray = TRUE;
            break;

         case '[' :
            if( bArray || ( i && isalnum( ( BYTE ) pLine->item.asString.value[i - 1] ) ) )
            {
               iIndexs++;
            }
            else
            {
               while( ++i < pLine->item.asString.length  && pLine->item.asString.value[i] != ']'  ) {}
            }
            bArray = FALSE;
            break;

         case ']' :
            iIndexs--;
            bArray = TRUE;
            break;

         case '"' :
            while( ++i < pLine->item.asString.length && pLine->item.asString.value[i] != '"'  ) {}
            bArray = FALSE;
            break;

         case '\'' :
            while( ++i < pLine->item.asString.length && pLine->item.asString.value[i] != '\''  ) {}
            bArray = FALSE;
            break;

         case ',' :
            if( iParans == 0 && iArrays == 0 && iIndexs == 0 )
            {
               HB_ITEM_NEW ( Exp );

               hb_arrayAddForward( &(HB_VM_STACK).Return, hb_itemPutCL( &Exp, pLine->item.asString.value + iOffset, i - iOffset ) );
               iOffset = i + 1;

            }
            bArray = FALSE;
            break;

         default :
            bArray = FALSE;
            break;
      }
   }

   if( iOffset < pLine->item.asString.length - 1 )
   {
      HB_ITEM_NEW ( Exp );

      hb_arrayAddForward( &(HB_VM_STACK).Return, hb_itemPutCL( &Exp, pLine->item.asString.value + iOffset, pLine->item.asString.length - iOffset ) );

   }
}

HB_FUNC( HB_ATOKENS )
{
   PHB_ITEM pLine  = hb_param( 1, HB_IT_STRING );
   PHB_ITEM pDelim = hb_param( 2, HB_IT_STRING );

   if( pLine )
   {
      HB_ITEM_NEW ( Token );
      char cDelimiter = pDelim ? pDelim->item.asString.value[0] : 32;
      size_t i, iOffset = 0;
      BOOL bSkipStrings = hb_parl( 3 );
      BOOL bDoubleQuoteOnly = hb_parl( 4 );

      HB_VM_STACK.Return.type = HB_IT_NIL;
      hb_arrayNew( &(HB_VM_STACK.Return), 0 );

      for( i = 0; i < pLine->item.asString.length; i++ )
      {
         if( bSkipStrings && ( pLine->item.asString.value[i] == '"'
                               || ( bDoubleQuoteOnly == FALSE && pLine->item.asString.value[i] == '\'' ) ) )
         {
            char cTerminator = pLine->item.asString.value[i];

            while( ++i < pLine->item.asString.length && pLine->item.asString.value[i] != cTerminator )
            {
            }
         }
         else if( pLine->item.asString.value[i] == cDelimiter )
         {
            hb_arrayAddForward( &(HB_VM_STACK.Return), hb_itemPutCL( &Token, pLine->item.asString.value + iOffset, i - iOffset ) );

            iOffset = i + 1;
         }
      }

      if( iOffset < pLine->item.asString.length )
      {
         hb_arrayAddForward( &(HB_VM_STACK.Return), hb_itemPutCL( &Token, pLine->item.asString.value + iOffset, pLine->item.asString.length - iOffset ) );
      }

   }
   else
   {
      hb_errRT_BASE_SubstR( EG_ARG, 1123, NULL, "HB_ATOKENS", 3, hb_paramError(1), hb_paramError(2), hb_paramError(3) );
      return;
   }
}

unsigned int SizeOfCStructure( PHB_ITEM aDef, unsigned int uiAlign )
{
   PHB_BASEARRAY pBaseDef = aDef->item.asArray.value;
   unsigned long ulLen = pBaseDef->ulLen;
   unsigned long ulIndex;
   unsigned int uiSize = 0, uiMemberSize;
   BYTE cShift;
   unsigned int uiPad;

   for( ulIndex = 0; ulIndex < ulLen; ulIndex++ )
   {
      if( ( pBaseDef->pItems + ulIndex )->type != HB_IT_INTEGER )
      {
         hb_errRT_BASE( EG_ARG, 2023, NULL, "SizeOfCStructure", 1, hb_paramError( 1 ) );
         return 0;
      }

      switch( ( pBaseDef->pItems + ulIndex )->item.asInteger.value )
      {
         case CTYPE_CHAR : // char
         case CTYPE_UNSIGNED_CHAR : // unsigned char
            uiMemberSize = sizeof( char );
            break;

         case CTYPE_CHAR_PTR : // char *
         case CTYPE_UNSIGNED_CHAR_PTR : // unsigned char *
            uiMemberSize = sizeof( char * );
            break;

         case CTYPE_SHORT : // short
         case CTYPE_UNSIGNED_SHORT : // unsigned short
            uiMemberSize = sizeof( short );
            break;

         case CTYPE_SHORT_PTR : // short
         case CTYPE_UNSIGNED_SHORT_PTR : // unsigned short
            uiMemberSize = sizeof( short * );
            break;

         case CTYPE_INT : // int
         case CTYPE_UNSIGNED_INT : // unsigned int
            uiMemberSize = sizeof( int );
            break;

         case CTYPE_INT_PTR : // int *
         case CTYPE_UNSIGNED_INT_PTR : // unsigned int *
            uiMemberSize = sizeof( int * );
            break;

         case CTYPE_LONG : // long
         case CTYPE_UNSIGNED_LONG : // unsigned long
            uiMemberSize = sizeof( long );
            break;

         case CTYPE_LONG_PTR : // long *
         case CTYPE_UNSIGNED_LONG_PTR : // unsigned long *
            uiMemberSize = sizeof( long * );
            break;

         case CTYPE_FLOAT : // float
            uiMemberSize = sizeof( float );
            break;

         case CTYPE_FLOAT_PTR : // float *
            uiMemberSize = sizeof( float * );
            break;

         case CTYPE_DOUBLE : // double
            uiMemberSize = sizeof( double );
            break;

         case CTYPE_DOUBLE_PTR : // double *
            uiMemberSize = sizeof( double * );
            break;

         case CTYPE_VOID_PTR : // void * (pointer)
            uiMemberSize = sizeof( void * );
            break;

         default:
         {
            HB_ITEM_NEW ( ID );

            hb_itemPutNI( &ID, ( pBaseDef->pItems + ulIndex )->item.asInteger.value );

            if( ( pBaseDef->pItems + ulIndex )->item.asInteger.value >= CTYPE_STRUCTURE_PTR )
            {
               uiMemberSize = sizeof( void * );
            }
            else if( ( pBaseDef->pItems + ulIndex )->item.asInteger.value >= CTYPE_STRUCTURE )
            {
               PHB_ITEM pStructure = hb_itemDoC( "HB_CSTRUCTUREFROMID", 1, &ID );

               if( HB_IS_OBJECT( pStructure ) )
               {
                  hb_objSendMsg( pStructure, "SizeOf", 0 );
                  uiMemberSize = (unsigned int) hb_itemGetNL( &HB_VM_STACK.Return );
                  hb_itemRelease( pStructure );
               }
               else
               {
                  hb_itemRelease( pStructure );
                  hb_errRT_BASE( EG_ARG, 2023, NULL, "SizeOfCStructure", 1, hb_paramError( 1 ) );
                  return 0;
               }
            }
            else
            {
               hb_errRT_BASE( EG_ARG, 2023, NULL, "SizeOfCStructure", 1, hb_paramError( 1 ) );
               return 0;
            }
         }
      }

      if( uiSize )
      {
         uiPad = ( ( uiMemberSize < uiAlign ) ? uiMemberSize : uiAlign );

         if( ( cShift = ( uiSize % uiPad ) ) > 0 )
         {
            uiSize += ( uiPad - cShift );
         }
      }

      uiSize += uiMemberSize;

      //printf( "#%lu Size: %u Align: %u Pad: %u Shift %i Size: %u\n", ulIndex, uiMemberSize, uiAlign, uiPad, cShift, uiSize );

   }

   if( ( cShift = ( uiSize % uiAlign ) ) > 0 )
   {
      uiSize += ( uiAlign - cShift );
   }

   //printf( "#%lu Size: %u Align: %u Pad: %u Shift %i Size: %u\n", ulIndex, uiMemberSize, uiAlign, uiPad, cShift, uiSize );

   return uiSize;
}

HB_FUNC( HB_SIZEOFCSTRUCTURE )
{
   PHB_ITEM aDef = hb_param( 1, HB_IT_ARRAY );
   PHB_ITEM pAlign = hb_param( 2, HB_IT_INTEGER );
   unsigned int uiAlign;

   if( aDef )
   {
      if( pAlign )
      {
         uiAlign = (BYTE) pAlign->item.asInteger.value;
      }
      else
      {
         uiAlign = 8;
      }

      hb_retni( SizeOfCStructure( aDef, uiAlign ) );
   }
   else
   {
      hb_errRT_BASE( EG_ARG, 2023, NULL, "SizeOfCStructure", 2, hb_paramError( 1 ), hb_paramError( 2 ) );
   }
}

BYTE * ArrayToStructure( PHB_ITEM aVar, PHB_ITEM aDef, unsigned int uiAlign, unsigned int * puiSize )
{
   PHB_BASEARRAY pBaseVar = aVar->item.asArray.value;
   PHB_BASEARRAY pBaseDef = aDef->item.asArray.value;
   unsigned long ulLen = pBaseDef->ulLen;
   unsigned long ulIndex;
   BYTE  *Buffer;
   unsigned int uiOffset = 0, uiMemberSize;
   BYTE cShift;

   *puiSize = SizeOfCStructure( aDef, uiAlign ) ;

   //printf( "Size: %i\n", *puiSize );

   Buffer = (BYTE *) hb_xgrab( *puiSize );

   for( ulIndex = 0; ulIndex < ulLen; ulIndex++ )
   {
      //printf( "#: %i\n", ulIndex );

      switch( ( pBaseDef->pItems + ulIndex )->item.asInteger.value )
      {
         case CTYPE_CHAR : // char
         case CTYPE_UNSIGNED_CHAR : // unsigned char
            if( ( pBaseVar->pItems + ulIndex  )->type && ! HB_IS_NUMERIC( pBaseVar->pItems + ulIndex  ) )
            {
               hb_errRT_BASE( EG_ARG, 2023, NULL, "ArrayToStructure", 3, hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError(3) );
               return NULL;
            }

            uiMemberSize = sizeof( char );
            break;

         case CTYPE_CHAR_PTR : // char *
         case CTYPE_UNSIGNED_CHAR_PTR : // unsigned char *
            if( ( pBaseVar->pItems + ulIndex  )->type && ( pBaseVar->pItems + ulIndex  )->type != HB_IT_STRING
                                                      && ( pBaseVar->pItems + ulIndex  )->type != HB_IT_POINTER
                                                      && ( pBaseVar->pItems + ulIndex  )->type != HB_IT_LONG )
            {
               hb_errRT_BASE( EG_ARG, 2023, NULL, "ArrayToStructure", 3, hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError(3) );
               return NULL;;
            }

            uiMemberSize = sizeof( char * );
            break;

         case CTYPE_SHORT : // short
         case CTYPE_UNSIGNED_SHORT : // unsigned short
            // Type check performed in actual translation...
            uiMemberSize = sizeof( short );
            break;

         case CTYPE_SHORT_PTR : // short *
         case CTYPE_UNSIGNED_SHORT_PTR : // unsigned short *
            if( ( pBaseVar->pItems + ulIndex  )->type && ( pBaseVar->pItems + ulIndex  )->type != HB_IT_POINTER
                                                      && ( pBaseVar->pItems + ulIndex  )->type != HB_IT_LONG )
            {
               hb_errRT_BASE( EG_ARG, 2023, NULL, "ArrayToStructure", 3, hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError(3) );
               return NULL;;
            }

            uiMemberSize = sizeof( short * );
            break;

         case CTYPE_INT : // int
         case CTYPE_UNSIGNED_INT : // unsigned int
            // Type check performed in actual translation...
            uiMemberSize = sizeof( int );
            break;

         case CTYPE_INT_PTR : // int *
         case CTYPE_UNSIGNED_INT_PTR : // unsigned int *
            if( ( pBaseVar->pItems + ulIndex  )->type && ( pBaseVar->pItems + ulIndex  )->type != HB_IT_POINTER
                                                      && ( pBaseVar->pItems + ulIndex  )->type != HB_IT_LONG )
            {
               hb_errRT_BASE( EG_ARG, 2023, NULL, "ArrayToStructure", 3, hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError(3) );
               return NULL;;
            }

            uiMemberSize = sizeof( int * );
            break;

         case CTYPE_LONG : // long
         case CTYPE_UNSIGNED_LONG : // unsigned long
            // Type check performed in actual translation...
            uiMemberSize = sizeof( long );
            break;

         case CTYPE_LONG_PTR : // long *
         case CTYPE_UNSIGNED_LONG_PTR : // unsigned long *
            if( ( pBaseVar->pItems + ulIndex  )->type && ( pBaseVar->pItems + ulIndex  )->type != HB_IT_POINTER
                                                      && ( pBaseVar->pItems + ulIndex  )->type != HB_IT_LONG )
            {
               hb_errRT_BASE( EG_ARG, 2023, NULL, "ArrayToStructure", 3, hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError(3) );
               return NULL;;
            }

            uiMemberSize = sizeof( long * );
            break;

         case CTYPE_FLOAT : // float
            if( ( pBaseVar->pItems + ulIndex  )->type && ( pBaseVar->pItems + ulIndex  )->type != HB_IT_DOUBLE )
            {
               hb_errRT_BASE( EG_ARG, 2023, NULL, "ArrayToStructure", 3, hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError(3) );
               return NULL;
            }

            uiMemberSize = sizeof( float );
            break;

         case CTYPE_FLOAT_PTR : // float *
            if( ( pBaseVar->pItems + ulIndex  )->type && ( pBaseVar->pItems + ulIndex  )->type != HB_IT_DOUBLE
                                                      && ( pBaseVar->pItems + ulIndex  )->type != HB_IT_POINTER
                                                      && ( pBaseVar->pItems + ulIndex  )->type != HB_IT_LONG )
            {
               hb_errRT_BASE( EG_ARG, 2023, NULL, "ArrayToStructure", 3, hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError(3) );
               return NULL;
            }

            uiMemberSize = sizeof( float * );
            break;

         case CTYPE_DOUBLE : // double
            if( ( pBaseVar->pItems + ulIndex  )->type && ( pBaseVar->pItems + ulIndex  )->type != HB_IT_DOUBLE )
            {
               hb_errRT_BASE( EG_ARG, 2023, NULL, "ArrayToStructure", 3, hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError(3) );
               return NULL;
            }

            uiMemberSize = sizeof( double );
            break;

         case CTYPE_DOUBLE_PTR : // double *
            if( ( pBaseVar->pItems + ulIndex  )->type && ( pBaseVar->pItems + ulIndex  )->type != HB_IT_DOUBLE
                                                      && ( pBaseVar->pItems + ulIndex  )->type != HB_IT_POINTER
                                                      && ( pBaseVar->pItems + ulIndex  )->type != HB_IT_LONG )
            {
               hb_errRT_BASE( EG_ARG, 2023, NULL, "ArrayToStructure", 3, hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError(3) );
               return NULL;
            }

            uiMemberSize = sizeof( double * );
            break;

         case CTYPE_VOID_PTR : // void * (pointer)
            if( ( pBaseVar->pItems + ulIndex  )->type && ( pBaseVar->pItems + ulIndex  )->type != HB_IT_POINTER
                                                      && ( pBaseVar->pItems + ulIndex  )->type != HB_IT_LONG
                                                      && ( pBaseVar->pItems + ulIndex  )->type != HB_IT_STRING )
            {
               hb_errRT_BASE( EG_ARG, 2023, NULL, "ArrayToStructure", 3, hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError(3) );
               return NULL;
            }

            uiMemberSize = sizeof( void * );
            break;

         default:
         {
            HB_ITEM_NEW ( ID );

            hb_itemPutNI( &ID, ( pBaseDef->pItems + ulIndex )->item.asInteger.value );

            if( ( pBaseDef->pItems + ulIndex )->item.asInteger.value >= CTYPE_STRUCTURE_PTR )
            {
               uiMemberSize = sizeof( void * );
            }
            else if( ( pBaseDef->pItems + ulIndex )->item.asInteger.value >= CTYPE_STRUCTURE )
            {
               PHB_ITEM pStructure = hb_itemDoC( "HB_CSTRUCTUREFROMID", 1, &ID );

               if( HB_IS_OBJECT( pStructure ) )
               {
                  hb_objSendMsg( pStructure, "SizeOf", 0 );
                  uiMemberSize = (unsigned int) hb_itemGetNL( &HB_VM_STACK.Return );
                  hb_itemRelease( pStructure );
               }
               else
               {
                  hb_itemRelease( pStructure );
                  hb_errRT_BASE( EG_ARG, 2023, NULL, "ArrayToStructure", 3, hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError(3) );
                  return NULL;
               }
            }
            else
            {
               hb_errRT_BASE( EG_ARG, 2023, NULL, "ArrayToStructure", 3, hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError(3) );
               return NULL;
            }
         }
      }

      if( uiOffset )
      {
         unsigned int uiPad = ( ( uiMemberSize < uiAlign ) ? uiMemberSize : uiAlign );

         if( ( cShift = ( uiOffset % uiPad ) ) > 0 )
         {
            uiOffset += ( uiPad - cShift );
         }
      }

      //printf( "* Size: %i Offset: %i\n", uiMemberSize, uiOffset );

      switch( ( pBaseDef->pItems + ulIndex )->item.asInteger.value )
      {
         case CTYPE_CHAR : // char
            if( ( pBaseVar->pItems + ulIndex  )->type )
            {
               *( (char *) ( Buffer + uiOffset ) ) = (char) ( ( pBaseVar->pItems + ulIndex  )->item.asInteger.value );
            }
            else
            {
               *( (char *) ( Buffer + uiOffset ) ) = 0;
            }
            break;

         case CTYPE_UNSIGNED_CHAR : // unsigned char
            if( ( pBaseVar->pItems + ulIndex  )->type )
            {
               *( (BYTE*) ( Buffer + uiOffset ) ) = (BYTE) ( ( pBaseVar->pItems + ulIndex  )->item.asInteger.value );
            }
            else
            {
               *( (BYTE*) ( Buffer + uiOffset ) ) = 0;
            }
            break;

         case CTYPE_CHAR_PTR : // char *
            switch( ( pBaseVar->pItems + ulIndex  )->type )
            {
               case HB_IT_STRING:
                  *( (char **) ( Buffer + uiOffset ) ) = ( pBaseVar->pItems + ulIndex  )->item.asString.value;
                  break;

               case HB_IT_POINTER:
                  *( (char **) ( Buffer + uiOffset ) ) = (char *) ( pBaseVar->pItems + ulIndex  )->item.asPointer.value;
                  break;
#if UINT_MAX == ULONG_MAX
               case HB_IT_INTEGER:
                  *( (char **) ( Buffer + uiOffset ) ) = (char *) ( HB_PTRDIFF ) ( pBaseVar->pItems + ulIndex  )->item.asInteger.value;
                  break;
#endif
               case HB_IT_LONG:
                  *( (char **) ( Buffer + uiOffset ) ) = (char *) ( HB_PTRDIFF ) ( pBaseVar->pItems + ulIndex  )->item.asLong.value;
                  break;

               default:
                 *( (char **) ( Buffer + uiOffset ) ) = NULL;
                  break;
            }
            break;

         case CTYPE_UNSIGNED_CHAR_PTR : // unsigned char *
            switch( ( pBaseVar->pItems + ulIndex  )->type )
            {
               case HB_IT_STRING:
                  *( (BYTE **) ( Buffer + uiOffset ) ) = (BYTE *) ( ( pBaseVar->pItems + ulIndex  )->item.asString.value );
                  break;

               case HB_IT_POINTER:
                  *( (BYTE **) ( Buffer + uiOffset ) ) = (BYTE *) ( ( pBaseVar->pItems + ulIndex  )->item.asPointer.value );
                  break;

#if UINT_MAX == ULONG_MAX
               case HB_IT_INTEGER:
                  *( (BYTE **) ( Buffer + uiOffset ) ) = (BYTE *) ( HB_PTRDIFF ) ( ( pBaseVar->pItems + ulIndex  )->item.asInteger.value );
                  break;
#endif
               case HB_IT_LONG:
                  *( (BYTE **) ( Buffer + uiOffset ) ) = (BYTE *) ( HB_PTRDIFF ) ( ( pBaseVar->pItems + ulIndex  )->item.asLong.value );
                  break;

               default:
                 *( (BYTE **) ( Buffer + uiOffset ) ) = NULL;
                  break;
            }
            break;

         case CTYPE_SHORT : // short
            if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_INTEGER )
            {
               *( (short *) ( Buffer + uiOffset ) ) = (short) ( ( pBaseVar->pItems + ulIndex  )->item.asInteger.value );
            }
            else if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_LONG )
            {
               *( (short *) ( Buffer + uiOffset ) ) = (short) ( ( pBaseVar->pItems + ulIndex  )->item.asLong.value );
            }
            else if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_DOUBLE )
            {
               *( (short *) ( Buffer + uiOffset ) ) = (short) ( ( pBaseVar->pItems + ulIndex  )->item.asDouble.value );
            }
            else if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_NIL )
            {
               *( (short *) ( Buffer + uiOffset ) ) = 0;
            }
            else
            {
               hb_errRT_BASE( EG_ARG, 2023, NULL, "ArrayToStructure", 3, hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError(3) );
               return NULL;
            }
            break;

         case CTYPE_UNSIGNED_SHORT : // unsigned short
            if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_INTEGER )
            {
               *( (unsigned short *) ( Buffer + uiOffset ) ) = (unsigned short) ( ( pBaseVar->pItems + ulIndex  )->item.asInteger.value );
            }
            else if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_LONG )
            {
               *( (unsigned short *) ( Buffer + uiOffset ) ) = (unsigned short) ( ( pBaseVar->pItems + ulIndex  )->item.asLong.value );
            }
            else if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_DOUBLE )
            {
               *( (unsigned short *) ( Buffer + uiOffset ) ) = (unsigned short) ( ( pBaseVar->pItems + ulIndex  )->item.asDouble.value );
            }
            else if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_NIL )
            {
               *( (unsigned short *) ( Buffer + uiOffset ) ) = 0;
            }
            else
            {
               hb_errRT_BASE( EG_ARG, 2023, NULL, "ArrayToStructure", 3, hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError(3) );
               return NULL;
            }
            break;

         case CTYPE_SHORT_PTR : // short *
            switch( ( pBaseVar->pItems + ulIndex  )->type )
            {
               case HB_IT_POINTER:
                  *( (short **) ( Buffer + uiOffset ) ) = (short *) ( ( pBaseVar->pItems + ulIndex  )->item.asPointer.value );
                  break;

#if UINT_MAX == ULONG_MAX
               case HB_IT_INTEGER:
                  *( (short **) ( Buffer + uiOffset ) ) = (short *) ( HB_PTRDIFF ) ( ( pBaseVar->pItems + ulIndex  )->item.asInteger.value );
                  break;
#endif
               case HB_IT_LONG:
                  *( (short **) ( Buffer + uiOffset ) ) = (short *) ( HB_PTRDIFF ) ( ( pBaseVar->pItems + ulIndex  )->item.asLong.value );
                  break;

               default:
                 *( (short **) ( Buffer + uiOffset ) ) = NULL;
                  break;
            }
            break;

         case CTYPE_UNSIGNED_SHORT_PTR : // unsigned short *
            switch( ( pBaseVar->pItems + ulIndex  )->type )
            {
               case HB_IT_POINTER:
                  *( (unsigned short **) ( Buffer + uiOffset ) ) = (unsigned short *) ( ( pBaseVar->pItems + ulIndex  )->item.asPointer.value );
                  break;

#if UINT_MAX == ULONG_MAX
               case HB_IT_INTEGER:
                  *( (unsigned short **) ( Buffer + uiOffset ) ) = (unsigned short *) ( HB_PTRDIFF ) ( ( pBaseVar->pItems + ulIndex  )->item.asInteger.value );
                  break;
#endif
               case HB_IT_LONG:
                  *( (unsigned short **) ( Buffer + uiOffset ) ) = (unsigned short *) ( HB_PTRDIFF ) ( ( pBaseVar->pItems + ulIndex  )->item.asLong.value );
                  break;

               default:
                 *( (unsigned short **) ( Buffer + uiOffset ) ) = NULL;
                  break;
            }
            break;

         case CTYPE_INT : // int
            if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_INTEGER )
            {
               *( (int *) ( Buffer + uiOffset ) ) = (int) ( ( pBaseVar->pItems + ulIndex  )->item.asInteger.value );
            }
            else if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_LONG )
            {
               *( (int *) ( Buffer + uiOffset ) ) = (int) ( ( pBaseVar->pItems + ulIndex  )->item.asLong.value );
            }
            else if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_DOUBLE )
            {
               *( (int *) ( Buffer + uiOffset ) ) = (int) ( ( pBaseVar->pItems + ulIndex  )->item.asDouble.value );
            }
            else if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_NIL )
            {
               *( (int *) ( Buffer + uiOffset ) ) = 0;
            }
            else
            {
               hb_errRT_BASE( EG_ARG, 2023, NULL, "ArrayToStructure", 3, hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError(3) );
               return NULL;
            }
            break;

         case CTYPE_UNSIGNED_INT : // unsigned int
            if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_INTEGER )
            {
               *( (unsigned int *) ( Buffer + uiOffset ) ) = (unsigned int) ( ( pBaseVar->pItems + ulIndex  )->item.asInteger.value );
            }
            else if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_LONG )
            {
               *( (unsigned int *) ( Buffer + uiOffset ) ) = (unsigned int) ( ( pBaseVar->pItems + ulIndex  )->item.asLong.value );
            }
            else if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_DOUBLE )
            {
               *( (unsigned int *) ( Buffer + uiOffset ) ) = (unsigned int) ( ( pBaseVar->pItems + ulIndex  )->item.asDouble.value );
            }
            else if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_NIL )
            {
               *( (unsigned int *) ( Buffer + uiOffset ) ) = 0;
            }
            else
            {
               hb_errRT_BASE( EG_ARG, 2023, NULL, "ArrayToStructure", 3, hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError(3) );
               return NULL;
            }

            break;

         case CTYPE_INT_PTR : // int *
            switch( ( pBaseVar->pItems + ulIndex  )->type )
            {
               case HB_IT_POINTER:
                  *( (int **) ( Buffer + uiOffset ) ) = (int *) ( ( pBaseVar->pItems + ulIndex  )->item.asPointer.value );
                  break;

#if UINT_MAX == ULONG_MAX
               case HB_IT_INTEGER:
                  *( (int **) ( Buffer + uiOffset ) ) = (int *) ( HB_PTRDIFF ) ( ( pBaseVar->pItems + ulIndex  )->item.asInteger.value );
                  break;
#endif
               case HB_IT_LONG:
                  *( (int **) ( Buffer + uiOffset ) ) = (int *) ( HB_PTRDIFF ) ( ( pBaseVar->pItems + ulIndex  )->item.asLong.value );
                  break;

               default:
                 *( (int **) ( Buffer + uiOffset ) ) = NULL;
                  break;
            }
            break;

         case CTYPE_UNSIGNED_INT_PTR : // unsigned int *
            switch( ( pBaseVar->pItems + ulIndex  )->type )
            {
               case HB_IT_POINTER:
                  *( (unsigned int **) ( Buffer + uiOffset ) ) = (unsigned int *) ( ( pBaseVar->pItems + ulIndex  )->item.asPointer.value );
                  break;

#if UINT_MAX == ULONG_MAX
               case HB_IT_INTEGER:
                  *( (unsigned int **) ( Buffer + uiOffset ) ) = (unsigned int *) ( HB_PTRDIFF ) ( ( pBaseVar->pItems + ulIndex  )->item.asInteger.value );
                  break;
#endif
               case HB_IT_LONG:
                  *( (unsigned int **) ( Buffer + uiOffset ) ) = (unsigned int *) ( HB_PTRDIFF ) ( ( pBaseVar->pItems + ulIndex  )->item.asLong.value );
                  break;

               default:
                 *( (unsigned int **) ( Buffer + uiOffset ) ) = NULL;
                  break;
            }
            break;

         case CTYPE_LONG : // long
            if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_INTEGER )
            {
               *( (long *) ( Buffer + uiOffset ) ) = (long) ( ( pBaseVar->pItems + ulIndex  )->item.asInteger.value );
            }
            else if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_LONG )
            {
               *( (long *) ( Buffer + uiOffset ) ) = (long) ( ( pBaseVar->pItems + ulIndex  )->item.asLong.value );
            }
            else if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_DOUBLE )
            {
               *( (long *) ( Buffer + uiOffset ) ) = (long) ( ( pBaseVar->pItems + ulIndex  )->item.asDouble.value );
            }
            else if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_NIL )
            {
               *( (long *) ( Buffer + uiOffset ) ) = 0;
            }
            else
            {
               hb_errRT_BASE( EG_ARG, 2023, NULL, "ArrayToStructure", 3, hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError(3) );
               return NULL;
            }
            break;

         case CTYPE_UNSIGNED_LONG : // unsigned long
            if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_INTEGER )
            {
               *( (unsigned long *) ( Buffer + uiOffset ) ) = (unsigned long) ( ( pBaseVar->pItems + ulIndex  )->item.asInteger.value );
            }
            else if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_LONG )
            {
               *( (unsigned long *) ( Buffer + uiOffset ) ) = (unsigned long) ( ( pBaseVar->pItems + ulIndex  )->item.asLong.value );
            }
            else if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_DOUBLE )
            {
               *( (unsigned long *) ( Buffer + uiOffset ) ) = (unsigned long) ( ( pBaseVar->pItems + ulIndex  )->item.asDouble.value );
            }
            else if( ( pBaseVar->pItems + ulIndex  )->type == HB_IT_NIL )
            {
               *( (unsigned long *) ( Buffer + uiOffset ) ) = 0;
            }
            else
            {
               hb_errRT_BASE( EG_ARG, 2023, NULL, "ArrayToStructure", 3, hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError(3) );
               return NULL;
            }
            break;

         case CTYPE_LONG_PTR : // long *
            switch( ( pBaseVar->pItems + ulIndex  )->type )
            {
               case HB_IT_POINTER:
                  *( (long **) ( Buffer + uiOffset ) ) = (long *) ( ( pBaseVar->pItems + ulIndex  )->item.asPointer.value );
                  break;

#if UINT_MAX == ULONG_MAX
               case HB_IT_INTEGER:
                  *( (long **) ( Buffer + uiOffset ) ) = (long *) ( HB_PTRDIFF ) ( ( pBaseVar->pItems + ulIndex  )->item.asInteger.value );
                  break;
#endif
               case HB_IT_LONG:
                  *( (long **) ( Buffer + uiOffset ) ) = (long *) ( HB_PTRDIFF ) ( ( pBaseVar->pItems + ulIndex  )->item.asLong.value );
                  break;

               default:
                 *( (long **) ( Buffer + uiOffset ) ) = NULL;
                  break;
            }
            break;

         case CTYPE_UNSIGNED_LONG_PTR : // unsigned long *
            switch( ( pBaseVar->pItems + ulIndex  )->type )
            {
               case HB_IT_POINTER:
                  *( (unsigned long **) ( Buffer + uiOffset ) ) = (unsigned long *) ( ( pBaseVar->pItems + ulIndex  )->item.asPointer.value );
                  break;

#if UINT_MAX == ULONG_MAX
               case HB_IT_INTEGER:
                  *( (unsigned long **) ( Buffer + uiOffset ) ) = (unsigned long *) ( HB_PTRDIFF ) ( ( pBaseVar->pItems + ulIndex  )->item.asInteger.value );
                  break;
#endif
               case HB_IT_LONG:
                  *( (unsigned long **) ( Buffer + uiOffset ) ) = (unsigned long *) ( HB_PTRDIFF ) ( ( pBaseVar->pItems + ulIndex  )->item.asLong.value );
                  break;

               default:
                 *( (unsigned long **) ( Buffer + uiOffset ) ) = NULL;
                  break;
            }
            break;

         case CTYPE_FLOAT : // float
            if( ( pBaseVar->pItems + ulIndex  )->type )
            {
               *( (float *) ( Buffer + uiOffset ) ) = (float) ( pBaseVar->pItems + ulIndex  )->item.asDouble.value;
            }
            else
            {
               *( (float *) ( Buffer + uiOffset ) ) = 0;
            }
            break;

         case CTYPE_FLOAT_PTR : // float *
            switch( ( pBaseVar->pItems + ulIndex  )->type )
            {
               case HB_IT_POINTER:
                  *( (float **) ( Buffer + uiOffset ) ) = (float *) ( ( pBaseVar->pItems + ulIndex  )->item.asPointer.value );
                  break;

#if UINT_MAX == ULONG_MAX
               case HB_IT_INTEGER:
                  *( (float **) ( Buffer + uiOffset ) ) = (float *) ( HB_PTRDIFF ) ( ( pBaseVar->pItems + ulIndex  )->item.asInteger.value );
                  break;
#endif
               case HB_IT_LONG:
                  *( (float **) ( Buffer + uiOffset ) ) = (float *) ( HB_PTRDIFF ) ( ( pBaseVar->pItems + ulIndex  )->item.asLong.value );
                  break;

               /* Is this correct??? IMHO It's a bug */
               case HB_IT_DOUBLE:
                  **( (float **) ( Buffer + uiOffset ) ) = (float) ( ( pBaseVar->pItems + ulIndex  )->item.asDouble.value );
                  break;

               default:
                 *( (float **) ( Buffer + uiOffset ) ) = NULL;
                  break;
            }
            break;

         case CTYPE_DOUBLE : // double
            if( ( pBaseVar->pItems + ulIndex  )->type )
            {
               *( (double *) ( Buffer + uiOffset ) ) = ( pBaseVar->pItems + ulIndex  )->item.asDouble.value;
            }
            else
            {
               *( (double *) ( Buffer + uiOffset ) ) = 0;
            }
            break;

         case CTYPE_DOUBLE_PTR : // double *
            switch( ( pBaseVar->pItems + ulIndex  )->type )
            {
               case HB_IT_POINTER:
                  *( (double **) ( Buffer + uiOffset ) ) = (double *) ( ( pBaseVar->pItems + ulIndex  )->item.asPointer.value );
                  break;

#if UINT_MAX == ULONG_MAX
               case HB_IT_INTEGER:
                  *( (double **) ( Buffer + uiOffset ) ) = (double *) ( HB_PTRDIFF ) ( ( pBaseVar->pItems + ulIndex  )->item.asInteger.value );
                  break;
#endif
               case HB_IT_LONG:
                  *( (double **) ( Buffer + uiOffset ) ) = (double *) ( HB_PTRDIFF ) ( ( pBaseVar->pItems + ulIndex  )->item.asLong.value );
                  break;

               /* Is this correct??? IMHO It's a bug */
               case HB_IT_DOUBLE:
                  **( (double **) ( Buffer + uiOffset ) ) = ( ( pBaseVar->pItems + ulIndex  )->item.asDouble.value );
                  break;

               default:
                 *( (double **) ( Buffer + uiOffset ) ) = NULL;
                  break;
            }
            break;

         case CTYPE_VOID_PTR : // void *
            switch( ( pBaseVar->pItems + ulIndex  )->type )
            {
               case HB_IT_POINTER:
                  *( (void **) ( Buffer + uiOffset ) ) = (void *) ( ( pBaseVar->pItems + ulIndex  )->item.asPointer.value );
                  break;

#if UINT_MAX == ULONG_MAX
               case HB_IT_INTEGER:
                  *( (void **) ( Buffer + uiOffset ) ) = (void *) ( HB_PTRDIFF ) ( ( pBaseVar->pItems + ulIndex  )->item.asInteger.value );
                  break;
#endif
               case HB_IT_LONG:
                  *( (void **) ( Buffer + uiOffset ) ) = (void *) ( HB_PTRDIFF ) ( ( pBaseVar->pItems + ulIndex  )->item.asLong.value );
                  break;

               default:
                 *( (void **) ( Buffer + uiOffset ) ) = NULL;
                  break;
            }
            break;

         default:
         {
            if( ( pBaseDef->pItems + ulIndex )->item.asInteger.value > CTYPE_STRUCTURE )
            {
               PHB_ITEM pStructure = pBaseVar->pItems + ulIndex;

               if( HB_IS_LONG( pStructure ) )
               {
                  if( ( pBaseDef->pItems + ulIndex )->item.asInteger.value > CTYPE_STRUCTURE_PTR )
                  {
                     *( (void **) ( Buffer + uiOffset ) ) = (void *) ( HB_PTRDIFF ) pStructure->item.asLong.value;
                  }
                  else
                  {
                     memcpy( (void *) ( Buffer + uiOffset ), (void *) ( HB_PTRDIFF ) pStructure->item.asLong.value, uiMemberSize );
                  }
               }
#if UINT_MAX == ULONG_MAX
               else if( HB_IS_INTEGER( pStructure ) )
               {
                  if( ( pBaseDef->pItems + ulIndex )->item.asInteger.value > CTYPE_STRUCTURE_PTR )
                  {
                     *( (void **) ( Buffer + uiOffset ) ) = (void *) ( HB_PTRDIFF ) pStructure->item.asInteger.value;
                  }
                  else
                  {
                     memcpy( (void *) ( Buffer + uiOffset ), (void *) ( HB_PTRDIFF ) pStructure->item.asInteger.value, uiMemberSize );
                  }
               }
#endif
               else if( HB_IS_NIL( pStructure ) )
               {
                  if( ( pBaseDef->pItems + ulIndex )->item.asInteger.value > CTYPE_STRUCTURE_PTR )
                  {
                     *( (void **) ( Buffer + uiOffset ) ) = NULL;
                  }
                  else
                  {
                     TraceLog( NULL,"ArrayToStructure() - Empty Inplace\n" );
                     memset( (void *) ( Buffer + uiOffset ), 0, uiMemberSize );
                  }
               }
               else if( strncmp( hb_objGetClsName( pStructure ), "C Structure", 11 ) == 0 )
               {
                  PHB_BASEARRAY pBaseStructure = pStructure->item.asArray.value;
                  PHB_ITEM pInternalBuffer = pBaseStructure->pItems + pBaseStructure->ulLen - 1;

                  hb_objSendMsg( pStructure, "VALUE", 0 );

                  if( ( pBaseDef->pItems + ulIndex )->item.asInteger.value > CTYPE_STRUCTURE_PTR )
                  {
                     *( (void **) ( Buffer + uiOffset ) ) = (void *) pInternalBuffer->item.asString.value;
                  }
                  else
                  {
                     memcpy( (void *) ( Buffer + uiOffset ), (void *) pInternalBuffer->item.asString.value, uiMemberSize );
                  }
               }
               else
               {
                  hb_errRT_BASE( EG_ARG, 2023, NULL, "ArrayToStructure", 3, hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError(3) );
               }
            }
            else
            {
               hb_errRT_BASE( EG_ARG, 2023, NULL, "ArrayToStructure", 3, hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError(3) );
            }
         }
      }

      //printf( "Wrote %i bytes at Offset %i\n", uiMemberSize, uiOffset );

      uiOffset += uiMemberSize;
   }

   return Buffer;
}

HB_FUNC( HB_ARRAYTOSTRUCTURE )
{
   PHB_ITEM aVar = hb_param( 1, HB_IT_ARRAY );
   PHB_ITEM aDef = hb_param( 2, HB_IT_ARRAY );
   PHB_ITEM pAlign = hb_param( 3, HB_IT_INTEGER );

   if( aVar && aDef )
   {
      unsigned int uiSize;
      unsigned int uiAlign;
      BYTE *Buffer;

      if( pAlign )
      {
         uiAlign = (BYTE) pAlign->item.asInteger.value;
      }
      else
      {
         uiAlign = 8;
      }

      Buffer = ArrayToStructure( aVar, aDef, uiAlign, &uiSize );

      hb_itemPutCRaw( &(HB_VM_STACK.Return), (char *) Buffer, uiSize );
   }
   else
   {
      hb_errRT_BASE( EG_ARG, 2023, NULL, "ArrayToStructure", 3, hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError(3) );
   }
}

PHB_ITEM StructureToArray( BYTE* Buffer, PHB_ITEM aDef, unsigned int uiAlign, BOOL bAdoptNested, PHB_ITEM pRet )
{
   PHB_BASEARRAY pBaseDef = aDef->item.asArray.value;
   unsigned long ulLen = pBaseDef->ulLen;
   unsigned long ulIndex;
   unsigned int uiOffset, uiMemberSize;
   BYTE cShift;
   //PHB_ITEM pRet = hb_itemNew( NULL );
   PHB_BASEARRAY pBaseVar;

   //TraceLog( NULL, "StructureToArray(%p, %p, %u, %i) ->%u\n", Buffer, aDef, uiAlign, bAdoptNested, ulLen );

   //hb_arrayNew( pRet, ulLen );
   pBaseVar = pRet->item.asArray.value;

   uiOffset = 0;
   for( ulIndex = 0; ulIndex < ulLen; ulIndex++ )
   {
      switch( ( pBaseDef->pItems + ulIndex )->item.asInteger.value )
      {
         case CTYPE_CHAR : // char
         case CTYPE_UNSIGNED_CHAR : // unsigned char
            uiMemberSize = sizeof( char );
            break;

         case CTYPE_CHAR_PTR : // char *
         case CTYPE_UNSIGNED_CHAR_PTR : // unsigned char *
            uiMemberSize = sizeof( char * );
            break;

         case CTYPE_SHORT : // short
         case CTYPE_UNSIGNED_SHORT : // unsigned short
            uiMemberSize = sizeof( short );
            break;

         case CTYPE_SHORT_PTR : // short *
         case CTYPE_UNSIGNED_SHORT_PTR : // unsigned short *
            uiMemberSize = sizeof( short * );
            break;

         case CTYPE_INT : // int
         case CTYPE_UNSIGNED_INT : // unsigned int
            uiMemberSize = sizeof( int );
            break;

         case CTYPE_INT_PTR : // int *
         case CTYPE_UNSIGNED_INT_PTR : // unsigned int *
            uiMemberSize = sizeof( int * );
            break;

         case CTYPE_LONG : // long
         case CTYPE_UNSIGNED_LONG : // unsigned long
            uiMemberSize = sizeof( long );
            break;

         case CTYPE_LONG_PTR : // long *
         case CTYPE_UNSIGNED_LONG_PTR : // unsigned long *
            uiMemberSize = sizeof( long * );
            break;

         case CTYPE_FLOAT : // float
            uiMemberSize = sizeof( float );
            break;

         case CTYPE_FLOAT_PTR : // float *
            uiMemberSize = sizeof( float * );
            break;

         case CTYPE_DOUBLE : // double
            uiMemberSize = sizeof( double );
            break;

         case CTYPE_DOUBLE_PTR : // double *
            uiMemberSize = sizeof( double * );
            break;

         case CTYPE_VOID_PTR : // void * (pointer)
            uiMemberSize = sizeof( void * );
            break;

         default:
         {
            HB_ITEM_NEW ( ID );

            hb_itemPutNI( &ID, ( pBaseDef->pItems + ulIndex )->item.asInteger.value );

            if( ( pBaseDef->pItems + ulIndex )->item.asInteger.value > CTYPE_STRUCTURE_PTR )
            {
               uiMemberSize = sizeof( void * );
            }
            else if( ( pBaseDef->pItems + ulIndex )->item.asInteger.value > CTYPE_STRUCTURE )
            {
               PHB_ITEM pStructure = hb_itemDoC( "HB_CSTRUCTUREFROMID", 1, &ID );

               if( HB_IS_OBJECT( pStructure ) )
               {
                  hb_objSendMsg( pStructure, "SizeOf", 0 );
                  uiMemberSize = (unsigned int) hb_itemGetNL( &HB_VM_STACK.Return );
                  hb_itemRelease( pStructure );
               }
               else
               {
                  hb_itemRelease( pStructure );
                  hb_errRT_BASE( EG_ARG, 2023, NULL, "StructureToArray", 1, hb_paramError( 1 ) );
                  return pRet;;
               }
            }
            else
            {
               hb_errRT_BASE( EG_ARG, 2023, NULL, "StructureToArray", 3, hb_paramError( 1 ), hb_paramError( 2 ), hb_paramError(3) );
               return pRet;;
            }
         }
      }

      if( uiOffset )
      {
         unsigned int uiPad = ( ( uiMemberSize < uiAlign ) ? uiMemberSize : uiAlign );

         if( ( cShift = ( uiOffset % uiPad ) ) > 0 )
         {
            uiOffset += ( uiPad - cShift );
         }

         //TraceLog( NULL, "* Size: %i Offset: %i Pad: %i\n", uiMemberSize, uiOffset, uiPad );
      }
      else
      {
         //TraceLog( NULL, "* Size: %i Offset: %i\n", uiMemberSize, uiOffset );
      }

      switch( ( pBaseDef->pItems + ulIndex )->item.asInteger.value )
      {
         case CTYPE_CHAR : // char
            hb_itemPutNI( pBaseVar->pItems + ulIndex , (int) *( (char *) ( Buffer + uiOffset ) ) );
            break;

         case CTYPE_UNSIGNED_CHAR : // unsigned char
            hb_itemPutNI( pBaseVar->pItems + ulIndex , (int) *( (BYTE *) ( Buffer + uiOffset ) ) );
            break;

         case CTYPE_CHAR_PTR : // char *
            if( HB_IS_STRING( pBaseVar->pItems + ulIndex ) && ( pBaseVar->pItems + ulIndex )->item.asString.value == *( (char **) ( Buffer + uiOffset ) ) )
            {
               //TraceLog( NULL, "IDENTICAL: %s\n", *( (char **) ( Buffer + uiOffset ) ) );
            }
            else if( bAdoptNested )
            {
               //TraceLog( NULL, "Adopt: %s\n", *( (char **) ( Buffer + uiOffset ) ) );
               hb_itemPutC( pBaseVar->pItems + ulIndex , *( (char **) ( Buffer + uiOffset ) ) );
            }
            else
            {
               //TraceLog( NULL, "Static: %s\n", *( (char **) ( Buffer + uiOffset ) ) );
               hb_itemPutCStatic( pBaseVar->pItems + ulIndex , *( (char **) ( Buffer + uiOffset ) ) );
            }
            break;

         case CTYPE_UNSIGNED_CHAR_PTR : // unsigned char *
            if( HB_IS_STRING( pBaseVar->pItems + ulIndex ) && ( pBaseVar->pItems + ulIndex )->item.asString.value == *( (char **) ( Buffer + uiOffset ) ) )
            {
               //TraceLog( NULL, "IDENTICAL: %s\n", *( (char **) ( Buffer + uiOffset ) ) );
            }
            else if( bAdoptNested )
            {
               //TraceLog( NULL, "Adopt: %s\n", *( (char **) ( Buffer + uiOffset ) ) );
               hb_itemPutC( pBaseVar->pItems + ulIndex , *( (char **) ( Buffer + uiOffset ) ) );
            }
            else
            {
               //TraceLog( NULL, "Static: %s\n", *( (char **) ( Buffer + uiOffset ) ) );
               hb_itemPutCStatic( pBaseVar->pItems + ulIndex , *( (char **) ( Buffer + uiOffset ) ) );
            }
            break;

         case CTYPE_SHORT : // short
            hb_itemPutNI( pBaseVar->pItems + ulIndex , *( (short *) ( Buffer + uiOffset ) ) );
            break;

         case CTYPE_UNSIGNED_SHORT : // unsigned short
            hb_itemPutNI( pBaseVar->pItems + ulIndex , (short) *( (unsigned short *) ( Buffer + uiOffset ) ) );
            break;

         case CTYPE_SHORT_PTR : // short *
         case CTYPE_UNSIGNED_SHORT_PTR : // unsigned short *
            hb_itemPutPtr( pBaseVar->pItems + ulIndex , (void *) ( Buffer + uiOffset ) );
            break;

         case CTYPE_INT : // int
            hb_itemPutNI( pBaseVar->pItems + ulIndex , *( (int *) ( Buffer + uiOffset ) ) );
            break;

         case CTYPE_UNSIGNED_INT : // unsigned int
            hb_itemPutNI( pBaseVar->pItems + ulIndex , (int) *( (unsigned int *) ( Buffer + uiOffset ) ) );
            break;

         case CTYPE_INT_PTR : // int *
         case CTYPE_UNSIGNED_INT_PTR : // unsigned int *
            hb_itemPutPtr( pBaseVar->pItems + ulIndex , (void *) ( Buffer + uiOffset ) );
            break;

         case CTYPE_LONG : // long
            hb_itemPutNL( pBaseVar->pItems + ulIndex , *( (long *) ( Buffer + uiOffset ) ) );
            break;

         case CTYPE_UNSIGNED_LONG : // unsigned long
            hb_itemPutNL( pBaseVar->pItems + ulIndex , (long) *( (unsigned long  *) ( Buffer + uiOffset ) ) );
            break;

         case CTYPE_LONG_PTR : // long *
         case CTYPE_UNSIGNED_LONG_PTR : // unsigned long *
            hb_itemPutPtr( pBaseVar->pItems + ulIndex , (void *) ( Buffer + uiOffset ) );
            break;

         case CTYPE_FLOAT : // float
            hb_itemPutND( pBaseVar->pItems + ulIndex , (double) *( (float *) ( Buffer + uiOffset ) ) );
            break;

         case CTYPE_FLOAT_PTR : // float *
            hb_itemPutPtr( pBaseVar->pItems + ulIndex , (void *) ( Buffer + uiOffset ) );
            break;

         case CTYPE_DOUBLE : // double
            hb_itemPutND( pBaseVar->pItems + ulIndex , *( (double *) ( Buffer + uiOffset ) ) );
            break;

         case CTYPE_DOUBLE_PTR : // double *
         case CTYPE_VOID_PTR : // void *
            hb_itemPutPtr( pBaseVar->pItems + ulIndex , (void *) ( Buffer + uiOffset ) );
            break;

         default:
         {
            HB_ITEM_NEW ( ID );
            PHB_ITEM pStructure;
            unsigned int uiNestedSize /*, uiNestedAlign */ ;

            hb_itemPutNI( &ID, ( pBaseDef->pItems + ulIndex )->item.asInteger.value );

            pStructure = hb_itemDoC( "HB_CSTRUCTUREFROMID", 1, &ID );

            if( ! HB_IS_OBJECT( pStructure ) )
            {
               hb_itemRelease( pStructure );
               hb_errRT_BASE( EG_ARG, 2023, NULL, "StructureToArray", 2, hb_paramError( 1 ), hb_paramError( 2 ) );
               return pRet;
            }

            hb_objSendMsg( pStructure, "NALIGN", 0 );
            // uiNestedAlign = ( &(HB_VM_STACK.Return) )->item.asInteger.value;

            hb_objSendMsg( pStructure, "SizeOf", 0 );
            uiNestedSize = (unsigned int) hb_itemGetNL( &HB_VM_STACK.Return );

            //TraceLog( NULL, "* NestedSize: %i Offset: %i\n", uiNestedSize, uiOffset );

            if( ( pBaseDef->pItems + ulIndex )->item.asInteger.value > CTYPE_STRUCTURE_PTR )
            {
               //printf( "Offset %i Pointer: %p\n", uiOffset, *(char **) ( (long ** )( Buffer + uiOffset ) ) );

               if( *(char **) ( (long ** )( Buffer + uiOffset ) ) )
               {
                  PHB_BASEARRAY pBaseStructure = pStructure->item.asArray.value;
                  PHB_ITEM pInternalBuffer = pBaseStructure->pItems + pBaseStructure->ulLen - 1;

                  if( bAdoptNested )
                  {
                     hb_itemPutCRaw( pInternalBuffer, *(char **) ( (long **)( Buffer + uiOffset ) ), uiNestedSize );
                  }
                  else
                  {
                     hb_itemPutCRawStatic( pInternalBuffer, *(char **) ( (long **)( Buffer + uiOffset ) ), uiNestedSize );
                  }

                  hb_objSendMsg( pStructure, "DEVALUE", 0 );
               }
               else
               {
                  //hb_objSendMsg( pStructure, "RESET", 0 );
                  hb_itemClear( pStructure );
               }
            }
            else
            {
               PHB_BASEARRAY pBaseStructure = pStructure->item.asArray.value;
               PHB_ITEM pInternalBuffer = pBaseStructure->pItems + pBaseStructure->ulLen - 1;
               HB_ITEM Adopt;

               Adopt.type = HB_IT_LOGICAL;
               Adopt.item.asLogical.value = bAdoptNested;

               //TraceLog( NULL, "Before Devalue\n" );

               hb_itemPutCRawStatic( pInternalBuffer, (char *) (BYTE *)( Buffer + uiOffset ), uiNestedSize );

               hb_objSendMsg( pStructure, "DEVALUE", 1, &Adopt );

               //TraceLog( NULL, "After Devalue\n" );
            }

            hb_itemForwardValue( pBaseVar->pItems + ulIndex, pStructure );

            hb_itemRelease( pStructure );
         }
      }

      uiOffset += uiMemberSize;

      //TraceLog( NULL, "AFTER Size: %i Offset: %i\n", uiMemberSize, uiOffset );
   }

   return pRet;
}

HB_FUNC( HB_STRUCTURETOARRAY )
{
   PHB_ITEM Structure = hb_param( 1, HB_IT_STRING );
   PHB_ITEM aDef      = hb_param( 2, HB_IT_ARRAY );
   PHB_ITEM pAlign    = hb_param( 3, HB_IT_INTEGER );
   PHB_ITEM pAdopt    = hb_param( 4, HB_IT_LOGICAL );
   PHB_ITEM pRet      = hb_param( 5, HB_IT_ARRAY );
   BOOL bAdopt;

   if( Structure && aDef )
   {
      BYTE  *Buffer = (BYTE *) Structure->item.asString.value;
      unsigned int uiAlign;

      if( pAlign )
      {
         uiAlign = (BYTE) pAlign->item.asInteger.value;
      }
      else
      {
         uiAlign = 8;
      }

      if( pAdopt )
      {
         bAdopt = pAdopt->item.asLogical.value;
      }
      else
      {
         bAdopt = FALSE;
      }

      hb_itemForwardValue( &(HB_VM_STACK.Return), StructureToArray( Buffer, aDef, uiAlign, bAdopt, pRet ) );
   }
   else
   {
      hb_errRT_BASE( EG_ARG, 2023, NULL, "StructureToArray", 2, hb_paramError( 1 ), hb_paramError( 2 ) );
   }
}

HB_FUNC( RASCAN )  // Reverse AScan... no hashes supported :(
{
   PHB_ITEM pArray = hb_param( 1, HB_IT_ARRAY );
   PHB_ITEM pValue = hb_param( 2, HB_IT_ANY );

   if( pArray && pValue )
   {
      PHB_ITEM pItems;
      ULONG ulLen;
      ULONG ulStart;
      ULONG ulCount;
      BOOL bExact = hb_parl( 5 );
      BOOL bAllowChar = hb_parl( 6 );

      pItems = pArray->item.asArray.value->pItems;
      ulLen = pArray->item.asArray.value->ulLen;

      /* sanitize scan range */
      if( ISNUM( 3 ) && hb_parni( 3 ) >= 1 )
      {
         ulStart = hb_parni( 3 );
      }
      else
      {
         ulStart = ulLen;
      }

      if( ulStart > ulLen )
      {
         hb_retnl( 0 );
         return;
      }

      if( ISNUM( 4 ) && ( ( ULONG ) hb_parni( 4 ) <= ulStart ) )
      {
         ulCount = hb_parni( 4 );
      }
      else
      {
         ulCount = ulStart;
      }

      if( ulCount > ulStart )
      {
         ulCount = ulStart;
      }

      /* Make separate search loops for different types to find, so that
         the loop can be faster. */

      if( HB_IS_BLOCK( pValue ) )
      {

         for( ulStart--; ulCount > 0; ulCount--, ulStart-- )
         {
            hb_vmPushSymbol( &hb_symEval );
            hb_vmPush( pValue );

            hb_vmPush( pItems + ulStart );
            hb_vmPushLong( ulStart + 1 );
            hb_vmSend( 2 );

            if( HB_IS_LOGICAL( &(HB_VM_STACK.Return) ) && HB_VM_STACK.Return.item.asLogical.value )
            {
               hb_retnl( ulStart + 1 );             // arrays start from 1
               return;
            }
         }
      }
      else if( HB_IS_STRING( pValue ) ) // Must precede HB_IS_NUMERIC()
      {
         if( ! bAllowChar || ! HB_IS_NUMERIC( pValue ) )
         {
            for( ulStart--; ulCount > 0; ulCount--, ulStart-- )
            {
               PHB_ITEM pItem = pItems + ulStart;

               /* NOTE: The order of the pItem and pValue parameters passed to
                        hb_itemStrCmp() is significant, please don't change it. [vszakats] */
               if( HB_IS_STRING( pItem ) && hb_itemStrCmp( pItem, pValue, bExact ) == 0 )
               {
                  hb_retnl( ulStart + 1 );             // arrays start from 1
                  return;
               }
            }
         }
         else
         {
            double dValue = hb_itemGetND( pValue );

            for( ulStart--; ulCount > 0; ulCount--, ulStart-- )
            {
               PHB_ITEM pItem = pItems + ulStart;

               /* NOTE: The order of the pItem and pValue parameters passed to
                        hb_itemStrCmp() is significant, please don't change it. [vszakats] */
               if( HB_IS_STRING( pItem ) )
               {
                  if( hb_itemStrCmp( pItem, pValue, bExact ) == 0 )
                  {
                     hb_retnl( ulStart + 1 );          // arrays start from 1
                     return;
                  }
               }
               else if( HB_IS_NUMERIC( pItem ) && hb_itemGetND( pItem ) == dValue )
               {
                  hb_retnl( ulStart + 1 );             // arrays start from 1
                  return;
               }
            }
         }
      }
      else if( pValue->type == HB_IT_DATE ) // Must precede HB_IS_NUMERIC()
      {
         LONG lValue = pValue->item.asDate.value;

         for( ulStart--; ulCount > 0; ulCount--, ulStart-- )
         {
            PHB_ITEM pItem = pItems + ulStart;

            if( pItem->type == HB_IT_DATE && pItem->item.asDate.value == lValue )
            {
               hb_retnl( ulStart + 1 );             // arrays start from 1
               return;
            }
         }
      }
      else if( HB_IS_NUMERIC( pValue ) )
      {
         double dValue = hb_itemGetND( pValue );

         for( ulStart--; ulCount > 0; ulCount--, ulStart-- )
         {
            PHB_ITEM pItem = pItems + ulStart;

            if( HB_IS_NUMERIC( pItem ) && hb_itemGetND( pItem ) == dValue && ( bAllowChar || ! HB_IS_STRING( pItem ) ) )
            {
               hb_retnl( ulStart + 1 );             // arrays start from 1
               return;
            }
         }
      }
      else if( HB_IS_LOGICAL( pValue ) )
      {
         BOOL bValue = hb_itemGetL( pValue );

         for( ulStart--; ulCount > 0; ulCount--, ulStart-- )
         {
            PHB_ITEM pItem = pItems + ulStart;

            if( HB_IS_LOGICAL( pItem ) && hb_itemGetL( pItem ) == bValue )
            {
               hb_retnl( ulStart + 1 );             // arrays start from 1
               return;
            }
         }
      }
      else if( HB_IS_NIL( pValue ) )
      {
         for( ulStart--; ulCount > 0; ulCount--, ulStart-- )
         {
            if( HB_IS_NIL( pItems + ulStart ) )
            {
               hb_retnl( ulStart + 1 );             // arrays start from 1
               return;
            }
         }
      }
      else if( bExact && pValue->type == HB_IT_ARRAY )
      {
         for( ulStart--; ulCount > 0; ulCount--, ulStart-- )
         {
            PHB_ITEM pItem = pItems + ulStart;

            if( pItem->type == HB_IT_ARRAY && pItem->item.asArray.value == pValue->item.asArray.value )
            {
               hb_retnl( ulStart + 1 );             // arrays start from 1
               return;
            }
         }
      }
   }

   hb_retnl( 0 );
}
