/*
 * $Id$
 */

/*
 * xHarbour Project source code:
 * hb_RegexReplace( cRegex, cString, cReplace, lCaseSensitive, lNewLine, nMaxMatches, nGetMatch ) --> cReturn
 *
 * Copyright 2006 Francesco Saverio Giudice <info/at/fsgiudice.com>
 * www - http://www.xharbour.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * <text>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * </text>
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA (or visit the web site http://www.gnu.org/).
 * <text>
 * As a special exception, xHarbour license gives permission for
 * additional uses of the text contained in its release of xHarbour.
 * </text>
 * The exception is that, if you link the xHarbour libraries with other
 * files to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the xHarbour library code into it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 *
 * This exception applies only to the code released with this xHarbour
 * explicit exception.  If you add/copy code from other sources,
 * as the General Public License permits, the above exception does
 * not apply to the code that you add in this way.  To avoid misleading
 * anyone as to the status of such modified files, you must delete
 * this exception notice from them.
 *
 * If you write modifications of your own for xHarbour, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice.
 *
 */

//--------------------------------------------------------------//

#define MATCH_STRING 1
#define MATCH_START  2
#define MATCH_END    3

FUNCTION hb_RegexReplace( cRegex, cString, cReplace, lCaseSensitive, lNewLine, nMaxMatches, nGetMatch )

   LOCAL pRegex
   LOCAL aMatches, aMatch
   LOCAL cReturn
   LOCAL nOffSet := 0
   LOCAL cSearch, nStart, nEnd, nLenSearch, nLenReplace

   IF HB_ISREGEXSTRING( cRegex )
     pRegex := cRegEx
   ELSE
     pRegex := HB_RegExComp( cRegEx, lCaseSensitive, lNewLine )
   ENDIF

   cReturn := cString

   // lCaseSensitive and lNewLine already defined by HB_RegExComp()!
   aMatches := HB_RegExAll( pRegEx, cString, /* lCaseSensitive */, /*lNewLine*/, nMaxMatches, nGetMatch, .F. )

   IF ! ( aMatches == NIL )
      FOR EACH aMatch IN aMatches
          //TraceLog( "ValToPrg( aMatch ), cReturn", ValToPrg( aMatch ), cReturn )
          IF Len( aMatch ) == 1 .AND. Len( aMatch[1] ) == 3 // if regex matches I must have an array of 3 elements
             cSearch := aMatch[1][ MATCH_STRING ]
             nStart  := aMatch[1][ MATCH_START ]
             nEnd    := aMatch[1][ MATCH_END ]

             nLenSearch  := Len( cSearch ) //nEnd - nStart + 1
             nLenReplace := Len( cReplace )
             //TraceLog( "SubStr( cString, nStart, nLenSearch )", ;
             //          SubStr( cString, nStart - nOffSet, nLenSearch ) )

             cReturn := Stuff( cReturn, nStart - nOffSet, nLenSearch, cReplace )
             nOffSet += nLenSearch - nLenReplace
             //TraceLog( "cSearch, nStart, nEnd, nLenSearch, nLenReplace, nOffSet, cReturn",;
             //          cSearch, nStart, nEnd, nLenSearch, nLenReplace, nOffSet, cReturn )
          ENDIF
      NEXT
   ENDIF

RETURN cReturn

//--------------------------------------------------------------//

