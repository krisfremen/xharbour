/*
 * $Id: TCProgBar.prg,v 1.16 2003/03/07 15:17:29 what32 Exp $
 */
/*
 * xHarbour Project source code:
 *
 * Whoo.lib TProgressBar CLASS
 *
 * Copyright 2002 Augusto Infante [augusto@2vias.com.ar]
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
 */

#include "winuser.ch"
#Include "hbclass.ch"
#include "what32.ch"
#Include "debug.ch"
#INCLUDE "WinGdi.ch"
#INCLUDE "commctrl.ch"
#INCLUDE "classex.ch"

CLASS TProgressBar FROM TCustomControl

   DATA FLeft       PROTECTED   INIT    0
   DATA FTop        PROTECTED   INIT    0
   DATA FWidth      PROTECTED   INIT  160
   DATA FHeight     PROTECTED   INIT   16

   PROPERTY Position READ FPosition WRITE SetPosition DEFAULT 0

   DATA Max         INIT  100
   DATA Step        INIT  1

   DATA bkColor     INIT GetSysColor(COLOR_BTNFACE)
   DATA BarColor    INIT GetSysColor(COLOR_HIGHLIGHT)

   DATA lRegister   PROTECTED INIT .F.
   DATA lControl    PROTECTED INIT .T.
   DATA Msgs        PROTECTED INIT {WM_DESTROY,WM_SIZE,WM_MOVE}
   DATA WndProc     PROTECTED INIT "ControlProc"

   DATA Id          PROTECTED
   DATA Icon        PROTECTED
   DATA Style       PROTECTED INIT  WS_CHILD+WS_VISIBLE
   DATA ExStyle     PROTECTED INIT  0
   DATA Color       PROTECTED

   DATA WinClass    PROTECTED INIT PROGRESS_CLASS
   DATA ControlName EXPORTED INIT "ProgressBar"

   METHOD SetPosition()
   METHOD DrawText()
   METHOD SetBkColor()
   METHOD SetBarColor()
   METHOD StepIt()   INLINE HB_QSelf():SendMessage( PBM_STEPIT, 0, 0 )
   METHOD WMCreate()
ENDCLASS



METHOD WMCreate() CLASS TProgressBar

   view ::FHandle

   SendMessage( ::FHandle, PBM_SETRANGE32, 0, ::Max )
   SendMessage( ::FHandle, PBM_SETSTEP, ::Step, 0)
   ::SetPosition( ::FPosition )

RETURN Self



METHOD SetPosition( n ) CLASS TProgressBar

   DEFAULT n TO 0

   ::FPosition := n

   SendMessage( ::FHandle, PBM_SETPOS, n, 0 )

   IF ::FCaption!="" .AND. AND( GetWindowLong( ::FHandle, GWL_STYLE ), PBS_SMOOTH ) > 0
      ::DrawText()
   END

   UpdateWindow( ::FHandle )

RETURN self


METHOD DrawText() CLASS TProgressBar

   LOCAL hDC,aMetrics,nTMWidth,nTMHeight,nX,nY,aClip,nWidth,aRect,hBrush

   aRect := GetClientRect(::handle)
   hDC   := GetDC(::handle)

   SelectObject(hDC, SendMessage(::Parent:handle, WM_GETFONT ))

   aMetrics  := GetTextExtentPoint32(hDC, ::Caption)
   nTMWidth  := aMetrics[1]
   nTMHeight := aMetrics[2]

   nX := (aRect[3] - nTMWidth )/ 2
   nY :=((aRect[4] - nTMHeight)/ 2)+(aRect[2]/2)

   nWidth := aRect[1] + ( ( ( aRect[3] - aRect[1] ) * ::position ) / 100 )

   aClip:={nX,aRect[2],nX+nTMWidth,aRect[4]}
   SetTextColor(hDC, ::barColor)
   SetBkColor(hDC, ::bkColor)

   ExtTextOut(hDC, nX, nY, ETO_CLIPPED , aClip, ::Caption)

   IF nX<=nWidth
      SetTextColor(hDC, ::bkColor)
      SetBkColor(hDC, ::barColor)
      aClip := { nX, aRect[2], nWidth,aRect[4] }
      ExtTextOut(hDC, nX, nY, ETO_CLIPPED , aClip, ::Caption)
   END

   ReleaseDC(::handle,hDC)

RETURN self


METHOD SetBkColor(nColor) CLASS TProgressBar
   ::bkColor:=nColor
   SendMessage(::handle,PBM_SETBKCOLOR,0,nColor)
RETURN self


METHOD SetBarColor(nColor) CLASS TProgressBar
   ::BarColor:=nColor
   SendMessage(::handle,PBM_SETBARCOLOR,0,nColor)
RETURN self