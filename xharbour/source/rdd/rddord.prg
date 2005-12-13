/*
 * $Id: rddord.prg,v 1.5 2005/12/13 12:03:36 druzus Exp $
 */

/*
 * Harbour Project source code:
 * Old style order management functions
 *
 * Copyright 1999 {list of individual authors and e-mail addresses}
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

#include "hbsetup.ch"

#include "common.ch"
#include "dbinfo.ch"

/* NOTE: The fifth parameters (cOrderName) is undocumented. */

FUNCTION dbCreateIndex( cOrderBagName, cKeyExpr, bKeyExpr, lUnique, cOrderName )
   RETURN ordCreate( cOrderBagName, cOrderName, cKeyExpr, bKeyExpr, lUnique )

FUNCTION dbSetIndex( cIndexName )
   RETURN ordListAdd( cIndexName )

FUNCTION dbClearIndex()
   RETURN ordListClear()

FUNCTION dbReindex()
   RETURN ordListRebuild()

FUNCTION dbSetOrder( nOrderNum )

   IF ISCHARACTER( nOrderNum ) .AND. !Empty( Val( nOrderNum ) )
      nOrderNum := Val( nOrderNum )
   ENDIF

   ordSetFocus( nOrderNum )

   RETURN NIL

FUNCTION IndexExt()
   RETURN ordBagExt()

FUNCTION IndexKey( nOrder )

   IF !ISNUMBER( nOrder )
      RETURN ordKey( nOrder )
   ENDIF

   IF Used()
      RETURN ordKey( nOrder )
   ENDIF

   RETURN ""

FUNCTION OrdSetRelation( xArea, bRelation, cRelation )
   RETURN dbSetRelation( xArea, bRelation, cRelation, .T. )

/* short (10 chars long) version of some ord* functions for compatibility */
FUNCTION ORDLISTCLE()
   RETURN ORDLISTCLEAR()

FUNCTION ORDLISTREB()
   RETURN ORDLISTREBUILD()

FUNCTION ORDSETFOCU( xOrder, cFile )
   RETURN ORDSETFOCUS( xOrder, cFile )

FUNCTION ORDSETRELA( xArea, bRelation, cRelation )
   RETURN ORDSETRELATION( xArea, bRelation, cRelation )

#ifdef HB_COMPAT_XPP

FUNCTION ORDWILDSEEK( cPattern, lCont, lBack )
   LOCAL lFound := .F.
   LOCAL xScopeTop, xScopeBottom, cFixed
   LOCAL nFixed := 0, i
   LOCAL c

   IF VALTYPE( lCont ) != "L"
      lCont := .F.
   ENDIF
   IF VALTYPE( lBack ) != "L"
      lBack := .F.
   ENDIF

   FOR EACH c IN cPattern
      IF c $ "?*"
         EXIT
      ENDIF
      ++nFixed
   NEXT
   IF nFixed > 0
      cFixed := LEFT( cPattern, nFixed )
      xScopeTop := ordScope( 0 )
      xScopeBottom := ordScope( 1 )
      IF !VALTYPE( xScopeTop ) == "C" .or. cFixed >= xScopeTop
         ordScope( 0, cFixed )
      ENDIF
      IF !VALTYPE( xScopeBottom ) == "C" .or. cFixed <= xScopeBottom
         ordScope( 1, cFixed )
      ENDIF
   ENDIF
   IF ! lCont
      IF lBack
         DBGOBOTTOM()
      ELSE
         DBGOTOP()
      ENDIF
#ifdef __XHARBOUR__
      lFound := WildMatch( cPattern, ORDKEYVAL() )
#else
      lFound := HB_WildMatch( cPattern, ORDKEYVAL() )
#endif
   ENDIF

   IF ! lFound
      lFound := DBORDERINFO( IIF( lBack, DBOI_SKIPWILDBACK, DBOI_SKIPWILD ),,, cPattern )
   ENDIF

   IF nFixed > 0
      ordScope( 0, xScopeTop )
      ordScope( 1, xScopeBottom )
   ENDIF

   RETURN lFound

#endif
