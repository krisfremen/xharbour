/*
 * $Id: tmenuitm.prg,v 1.2 2003/01/27 04:03:12 walito Exp $
 */

/*
 * Harbour Project source code:
 * MENUITEM class
 *
 * Copyright 2000 Jose Lalin <dezac@corevia.com>
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

#include "hbclass.ch"
#include "common.ch"
#include "button.ch"

#ifdef HB_COMPAT_C53

//--------------------------------------------------------------------------//
function MenuItem( cCaption, boData, nShortcut, cMsg, nID )

   local oMenuItem := HBMenuItem():New( cCaption, boData, nShortcut, cMsg, nID )

return oMenuItem

//--------------------------------------------------------------------------//
CLASS HBMenuItem

   DATA ClassName init "MENUITEM"
   DATA caption   init ""
   DATA cargo
   DATA checked   init FALSE
   DATA column    init 0
   DATA data
   DATA enabled
   DATA id
   DATA message
   DATA row       init 0
   DATA shortcut
   DATA style     init HB_TMENUITEM_STYLE

   METHOD New( cCaption, boData, nShortcut, cMsg, nID )
   METHOD isPopUp()

ENDCLASS

//--------------------------------------------------------------------------//
METHOD New( cCaption, boData, nShortcut, cMsg, nID ) CLASS HBMenuItem

   if ISBLOCK( boData ) .or. ISOBJECT( boData )
      boData := iif( cCaption != MENU_SEPARATOR, boData, nil )
   endif

   DEFAULT cCaption  TO ""
   DEFAULT boData    TO nil
   DEFAULT nShortcut TO 0
   DEFAULT cMsg      TO ""
   DEFAULT nID       TO 0

   ::caption  := cCaption
   ::checked  := FALSE
   ::column   := 0
   ::data     := boData
   ::enabled  := iif( cCaption != MENU_SEPARATOR, TRUE, FALSE )
   ::id       := nID
   ::message  := cMsg
   ::row      := 0
   ::shortcut := nShortcut
   ::style    := HB_TMENUITEM_STYLE

return Self

//--------------------------------------------------------------------------//
METHOD isPopUp() CLASS HBMenuItem

   if ISOBJECT( ::data ) .and. ::data:ClassName() == "POPUPMENU"
      return TRUE
   endif

return FALSE

//--------------------------------------------------------------------------//

#endif
