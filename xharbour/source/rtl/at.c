/*
 * $Id: at.c,v 1.6 2004/04/17 23:27:21 jonnymind Exp $
 */

/*
 * Harbour Project source code:
 * AT() function
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

#include "hbapi.h"
#include "hbapiitm.h"
#include "hbapierr.h"

/* locates a substring in a string */

#ifdef HB_C52_STRICT

    HB_FUNC( AT )
    {
       PHB_ITEM pSub = hb_param( 1, HB_IT_STRING );
       PHB_ITEM pText = hb_param( 2, HB_IT_STRING );

       if( pText && pSub )
       {
          hb_retnl( hb_strAt( pSub->item.asString.value, pSub->item.asString.length, pText->item.asString.value, pText->item.asString.length ) );
       }
       else
       {
          hb_errRT_BASE_SubstR( EG_ARG, 1108, NULL, "AT", 2, hb_paramError( 1 ), hb_paramError( 2 ) );
       }
    }

#else

    HB_FUNC( AT )
    {
       PHB_ITEM pSub = hb_param( 1, HB_IT_STRING );
       PHB_ITEM pText = hb_param( 2, HB_IT_STRING );
       PHB_ITEM pStart = hb_param( 3, HB_IT_NUMERIC );
       PHB_ITEM pEnd = hb_param( 4, HB_IT_NUMERIC );

       if( pText && pSub )
       {
          ULONG ulLength = pText->item.asString.length;
          ULONG ulStart = pStart ? (ULONG) hb_itemGetNL( pStart ) : 1;
          ULONG ulEnd = pEnd ? (ULONG) hb_itemGetNL( pEnd ) : ulLength;
          ULONG ulPos;

          // SANITIZATION
          // JC1: Don't know if it is clipper compliant, but anyway something
          // about that must be done.
          if ( ulStart < 1 )
          {
             ulStart = 1;
          }

          // Stop searching if starting past beyond end.
          if ( ulStart > ulEnd || ulStart > ulLength )
          {
             hb_retnl( 0 );
             return;
          }

          if ( ulEnd < ulStart )
          {
             ulEnd = ulStart;
          }

          if ( ulEnd > ulLength )
          {
             ulEnd = ulLength;
          }
          // END OF SANITIZATION

          ulPos = hb_strAt( pSub->item.asString.value, pSub->item.asString.length, pText->item.asString.value + ulStart - 1, ulEnd - ulStart + 1 );

          hb_retnl( ulPos ? ulPos + ( ulStart - 1 ) : 0 );
       }
       else
       {
          hb_errRT_BASE_SubstR( EG_ARG, 1108, NULL, "AT", 2, hb_paramError( 1 ), hb_paramError( 2 ) );
       }
    }

#endif

ULONG HB_EXPORT hb_AtSkipStrings( const char * szSub, ULONG ulSubLen, const char * szText, ULONG ulLen )
{
   char cLastChar = ' ';

   HB_TRACE(HB_TR_DEBUG, ("hb_strAtSkipStrings(%s, %lu, %s, %lu)", szSub, ulSubLen, szText, ulLen));

   if( ulSubLen > 0 && ulLen >= ulSubLen )
   {
      ULONG ulPos = 0;
      ULONG ulSubPos = 0;

      while( ulPos < ulLen && ulSubPos < ulSubLen )
      {
         if( szText[ ulPos ] == '"' && szSub[0] != '"' )
         {
            while( ++ulPos < ulLen && szText[ ulPos ] != '"' )
            {
               // Skip.
            }

            ulPos++;
            ulSubPos = 0;
            continue;
         }

         if( szText[ ulPos ] == '\'' && szSub[0] != '\'' )
         {
            while( ++ulPos < ulLen && szText[ ulPos ] != '\'' )
            {
               // Skip.
            }

            ulPos++;
            ulSubPos = 0;
            continue;
         }

         if( szText[ ulPos ] == '[' && szSub[0] != '[' )
         {
            if( ! ( isalpha( cLastChar  ) || isdigit( cLastChar ) || strchr( "])}_.", cLastChar ) ) )
            {
               while( ++ulPos < ulLen && szText[ ulPos ] != ']' )
               {
                  // Skip.
               }

               ulPos++;
               ulSubPos = 0;
               continue;
            }
         }

         if( szText[ ulPos ] == szSub[ ulSubPos ] )
         {
            ulSubPos++;
            ulPos++;
         }
         else if( ulSubPos )
         {
            /* Go back to the first character after the first match,
               or else tests like "22345" $ "012223456789" will fail. */
            ulPos -= ( ulSubPos - 1 );
            ulSubPos = 0;
         }
         else
         {
            cLastChar = szText[ ulPos ];
            ulPos++;
         }
      }

      return ( ulSubPos < ulSubLen ) ? 0 : ( ulPos - ulSubLen + 1 );
   }
   else
   {
      return 0;
   }
}

HB_FUNC( ATSKIPSTRINGS ) // cFind, cWhere, nStart
{
   PHB_ITEM pFind = hb_param( 1, HB_IT_STRING ), pWhere = hb_param( 2, HB_IT_STRING );

   if( pFind && pWhere )
   {
      unsigned long ulStart = (unsigned long) hb_parnl(3);

      if( ulStart > 0 )
      {
         ulStart--;
      }

      if( ulStart < pWhere->item.asString.length )
      {
         unsigned long ulRet;

         ulRet = hb_AtSkipStrings( pFind->item.asString.value, pFind->item.asString.length,
                                      pWhere->item.asString.value + ulStart, pWhere->item.asString.length - ulStart );

         if( ulRet )
         {
            hb_retnl( ulRet + ulStart );
            return;
         }
      }
   }

   hb_retnl( 0 );
}
