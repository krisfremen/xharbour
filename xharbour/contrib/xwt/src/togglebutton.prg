/*
   XWT - xHarbour Windowing Toolkit

   (C) 2003 Rafa Carmona ( Thefull )

   $Id: togglebutton.prg,v 1.0 2003/05/12 02:30:15 jonnymind Exp $

   Widget class - basic widget & event management
*/

#include "hbclass.ch"
#include "xwt.ch"

CLASS XWTToggleButton FROM XWTWidget
   METHOD New( cText, bStatus, nX, nY )
   METHOD SetStatus( bStatus )
   METHOD GetStatus()
ENDCLASS

METHOD New( cText, bStatus, nX, nY ) CLASS XWTToggleButton
   ::Super:New()
   ::nWidgetType := XWT_TYPE_TOGGLEBUTTON
   ::oRawWidget := XWT_Create( Self, XWT_TYPE_TOGGLEBUTTON )
   IF .not. Empty( cText )
      XWT_SetProperty( ::oRawWidget, XWT_PROP_TEXT, cText )
   ENDIF
   IF ValType( bStatus ) == "L" .and. bStatus
      XWT_SetProperty( ::oRawWidget, XWT_PROP_STATUS, 1 )
   ENDIF
   IF ValType( nX ) == "N" .and. ValType( nY ) == "N"
      ::Move( nX, nY )
   ENDIF
RETURN Self

METHOD SetStatus( bStatus ) CLASS XWTToggleButton
   IF ValType( bStatus ) == "L" .and. bStatus
      RETURN XWT_SetProperty( ::oRawWidget, XWT_PROP_STATUS, 1 )
   ENDIF
RETURN XWT_SetProperty( ::oRawWidget, XWT_PROP_STATUS, 0 )

METHOD GetStatus() CLASS XWTToggleButton
   LOCAL bRet

   IF XWT_GetProperty( ::oRawWidget, XWT_PROP_STATUS, @bRet )
      RETURN bRet
   ENDIF

RETURN .F.
