// Augusto Infante
// Whoo.lib

#include "windows.ch"
#include "HbClass.ch"
#include "what32.ch"
#include "debug.ch"

*------------------------------------------------------------------------------*

CLASS TButton FROM TControl

   METHOD New() CONSTRUCTOR

ENDCLASS

*------------------------------------------------------------------------------*

METHOD New( oParent, cCaption, nId, nLeft, nTop, nWidth, nHeight ) CLASS TButton
      
   ::Name      := 'button'
   ::id        := nId
   ::lRegister := .F.
   ::lControl  := .T.
   ::Msgs      := IFNIL( ::Msgs, {WM_DESTROY}, ::Msgs )
   ::WndProc   := IFNIL( ::WndProc, 'FormProc', ::WndProc )
   ::Caption   := cCaption
   ::Left      := nLeft
   ::Top       := nTop
   ::Width     := IFNIL( nWidth , IFNIL( ::Width , 80, ::Width ), nWidth )
   ::Height    := IFNIL( nHeight, IFNIL( ::height, 24, ::height), nHeight)
   ::Style     := WS_CHILD + WS_VISIBLE + WS_TABSTOP + BS_PUSHBUTTON
 
   RETURN( super:new( oParent ) )

*------------------------------------------------------------------------------*