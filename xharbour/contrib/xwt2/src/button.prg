/*
   XWT - xHarbour Windowing Toolkit

   (C) 2003 Giancarlo Niccolai

   $Id$

   Widget class - basic widget & event management
*/

#include "hbclass.ch"
#include "xwt.ch"

CLASS XWTButton FROM XWTWidget
   METHOD New( xProps, oParent )    CONSTRUCTOR
ENDCLASS

METHOD New( xProps, oParent ) CLASS XWTButton
   ::nWidgetType := XWT_TYPE_BUTTON
   ::oRawWidget := XWT_Create( Self, XWT_TYPE_BUTTON )
   ::Super:New( xProps )

   IF oParent != NIL
      oParent:Add( Self )
   ENDIF

RETURN Self
