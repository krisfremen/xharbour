/*
 * $Id: hbstr.c,v 1.7 2004/02/22 22:16:22 andijahja Exp $
 */

/*
 * Harbour Project source code:
 * Harbour common string functions (accessed from standalone utilities and the RTL)
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
 * Copyright 1999 David G. Holm <dholm@jsd-llc.com>
 *    hb_stricmp()
 *
 * See doc/license.txt for licensing terms.
 *
 */


#include <ctype.h> /* Needed by hb_strupr() */

#include "hbapi.h"

ULONG HB_EXPORT hb_strAt( const char * szSub, ULONG ulSubLen, const char * szText, ULONG ulLen )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_strAt(%s, %lu, %s, %lu)", szSub, ulSubLen, szText, ulLen));

   if( ulSubLen > 0 && ulLen >= ulSubLen )
   {
      ULONG ulPos = 0;
      ULONG ulSubPos = 0;

      while( ulPos < ulLen && ulSubPos < ulSubLen )
      {
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
            ulPos++;
      }

      return ( ulSubPos < ulSubLen ) ? 0 : ( ulPos - ulSubLen + 1 );
   }
   else
      return 0;
}

char HB_EXPORT * hb_strupr( char * pszText )
{
   char * pszPos;

   HB_TRACE(HB_TR_DEBUG, ("hb_strupr(%s)", pszText));

   for( pszPos = pszText; *pszPos; pszPos++ )
      *pszPos = toupper( *pszPos );

   return pszText;
}

char HB_EXPORT * hb_strdup( const char * pszText )
{
   char * pszDup;
   int iLen = strlen( pszText ) + 1;

   HB_TRACE(HB_TR_DEBUG, ("hb_strdup(%s, %i)", pszText, iLen));

   pszDup = ( char * ) hb_xgrab( iLen );
   memcpy( pszDup, pszText, iLen );

   return pszDup;
}

HB_EXPORT int hb_stricmp( const char * s1, const char * s2 )
{
   int rc = 0;
   ULONG l1;
   ULONG l2;
   ULONG count;

   HB_TRACE(HB_TR_DEBUG, ("hb_stricmp(%s, %s)", s1, s2));

   l1 = strlen( s1 );
   l2 = strlen( s2 );
   count = ( l1 < l2 ? l1 : l2 );

   while( rc == 0 && count > 0 )
   {
      char c1 = toupper( *s1 );
      char c2 = toupper( *s2 );

      s1++;
      s2++;

      if( c1 != c2 )
         rc = ( c1 < c2 ? -1 : 1 );

      count--;
   }

   if( rc == 0 && l1 != l2 )
      rc = ( l1 < l2 ? -1 : 1 );

   return rc;
}

/*
AJ: 2004-02-23
Concatenates multiple strings into a single result.
Eg. hb_xstrcat (buffer, "A", "B", NULL) stores "AB" in buffer.
*/
char HB_EXPORT * hb_xstrcat ( char *szDest, const char *szSrc, ... )
{
   char *szResult = szDest;
   va_list va;

   HB_TRACE(HB_TR_DEBUG, ("hb_xstrcat(%p, %p, ...)", szDest, szSrc));

   while( *szDest )
      szDest++;

   va_start(va, szSrc);

   while( szSrc )
   {
      while ( *szSrc )
         *szDest++ = *szSrc++;
      szSrc = va_arg ( va, char* );
   }

   *szDest = '\0';
   va_end ( va );
   return ( szResult );
}

/*
AJ: 2004-02-23
Concatenates multiple strings into a single result.
Eg. hb_xstrcpy (buffer, "A", "B", NULL) stores "AB" in buffer.
Returns szDest.
Any existing contents of szDest are cleared. If the szDest buffer is NULL,
allocates a new buffer with the required length and returns that. The
buffer is allocated using hb_xgrab(), and should eventually be freed
using hb_xfree().
*/
char HB_EXPORT * hb_xstrcpy ( char *szDest, const char *szSrc, ...)
{
   const char *szSrc_Ptr;
   va_list va;
   size_t dest_size;

   HB_TRACE(HB_TR_DEBUG, ("hb_xstrcpy(%p, %p, ...)", szDest, szSrc));

   if (szDest == NULL)
   {
       va_start (va, szSrc);
       szSrc_Ptr = szSrc;
       dest_size = 1;
       while (szSrc_Ptr)
       {
          dest_size += strlen (szSrc_Ptr);
          szSrc_Ptr = va_arg (va, char *);
       }
       va_end (va);

       szDest = (char *) hb_xgrab( dest_size );
   }

   va_start (va, szSrc);
   szSrc_Ptr  = szSrc;
   szDest [0] = '\0';
   while (szSrc_Ptr)
   {
      hb_xstrcat (szDest, szSrc_Ptr, NULL );
      szSrc_Ptr = va_arg (va, char *);
   }
   va_end (va);
   return (szDest);
}
