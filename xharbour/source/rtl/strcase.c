/*
 * $Id: strcase.c,v 1.23 2007/11/13 21:34:11 likewolf Exp $
 */

/*
 * Harbour Project source code:
 * Uppercase/lowercase string functions
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

#include <ctype.h>

#include "hbapi.h"
#include "hbapiitm.h"
#include "hbfast.h"
#include "hbapierr.h"

#ifndef HB_CDP_SUPPORT_OFF
#include "hbapicdp.h"
#endif

/* converts szText to lower case. Does not create a new string! */
char * hb_strLower( char * szText, ULONG ulLen )
{
   ULONG i;

   HB_TRACE(HB_TR_DEBUG, ("hb_strLower(%s, %lu)", szText, ulLen));

#ifndef HB_CDP_SUPPORT_OFF
   if( hb_cdp_page->nChars )
      for( i = 0; i < ulLen; i++ )
         szText[ i ] = (char) hb_cdp_page->s_lower[ (BYTE) szText[ i ] ];
   else
#endif
      for( i = 0; i < ulLen; i++ )
         szText[ i ] = tolower( (BYTE) szText[ i ] );

   return szText;
}

char * hb_strLowerCopy( char * szText, ULONG ulLen )
{
   char *szCopy = (char*) hb_xgrab( ulLen + 1 );
   ULONG i;

   HB_TRACE(HB_TR_DEBUG, ("hb_strLowerCopy(%s, %lu)", szText, ulLen));

#ifndef HB_CDP_SUPPORT_OFF
   if( hb_cdp_page->nChars )
      for( i = 0; i < ulLen; i++ )
         szCopy[ i ] = (char) hb_cdp_page->s_lower[ (BYTE) szText[ i ] ];
   else
#endif
      for( i = 0; i < ulLen; i++ )
         szCopy[ i ] = tolower( (BYTE) szText[ i ] );
   szCopy[ i ] = '\0';

   return szCopy;
}

char * hb_strUpperCopy( char * szText, ULONG ulLen )
{
   char *szCopy = (char*) hb_xgrab( ulLen + 1 );
   ULONG i;

   HB_TRACE(HB_TR_DEBUG, ("hb_strUpperCopy(%s, %lu)", szText, ulLen));

#ifndef HB_CDP_SUPPORT_OFF
   if( hb_cdp_page->nChars )
      for( i = 0; i < ulLen; i++ )
         szCopy[ i ] = (char) hb_cdp_page->s_upper[ (BYTE) szText[ i ] ];
   else
#endif
      for( i = 0; i < ulLen; i++ )
         szCopy[ i ] = toupper( (BYTE) szText[ i ] );
   szCopy[ i ] = '\0';

   return szCopy;
}

/* converts szText to upper case. Does not create a new string! */
char * hb_strUpper( char * szText, ULONG ulLen )
{
   ULONG i;

   HB_TRACE(HB_TR_DEBUG, ("hb_strUpper(%s, %lu)", szText, ulLen));

#ifndef HB_CDP_SUPPORT_OFF
   if( hb_cdp_page->nChars )
      for( i = 0; i < ulLen; i++ )
         szText[ i ] = (char) hb_cdp_page->s_upper[ (BYTE) szText[ i ] ];
   else
#endif
      for( i = 0; i < ulLen; i++ )
         szText[ i ] = toupper( (BYTE) szText[ i ] );

   return szText;
}

/* converts iChar to upper case */
int hb_charUpper( int iChar )
{
#ifndef HB_CDP_SUPPORT_OFF
   if( hb_cdp_page->nChars )
      return (unsigned char) hb_cdp_page->s_upper[ (unsigned char) iChar ];
   else
#endif
      return toupper( (unsigned char) iChar );
}

/* converts iChar to lower case */
int hb_charLower( int iChar )
{
#ifndef HB_CDP_SUPPORT_OFF
   if( hb_cdp_page->nChars )
      return (unsigned char) hb_cdp_page->s_lower[ (unsigned char) iChar ];
   else
#endif
      return tolower( (unsigned char) iChar );
}

/* converts string to lower case */
HB_FUNC( LOWER )
{
   PHB_ITEM pText = hb_param( 1, HB_IT_STRING );

   if( pText )
   {
      char * pszBuffer = pText->item.asString.value;
      ULONG ulLen = pText->item.asString.length;

      hb_retclenAdopt( hb_strLowerCopy( pszBuffer, ulLen ), ulLen );
   }
   else
   {
      hb_errRT_BASE_SubstR( EG_ARG, 1103, NULL, "LOWER", 1, hb_paramError( 1 ) );
   }
}

/* converts string to upper case */
HB_FUNC( UPPER )
{
   PHB_ITEM pText = hb_param( 1, HB_IT_STRING );

   if( pText )
   {
      char * pszBuffer = pText->item.asString.value;
      ULONG ulLen = pText->item.asString.length;

      hb_retclenAdopt( hb_strUpperCopy( pszBuffer, ulLen ), ulLen );
   }
   else
   {
      hb_errRT_BASE_SubstR( EG_ARG, 1102, NULL, "UPPER", 1, hb_paramError( 1 ) );
   }
}
