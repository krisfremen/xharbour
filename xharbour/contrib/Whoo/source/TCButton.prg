#include "windows.ch"
#include "HbClass.ch"
#include "what32.ch"
#include "debug.ch"


CLASS TButton FROM TControl
   METHOD New() CONSTRUCTOR
ENDCLASS

METHOD New( oParent, cCaption, nId, nLeft, nTop, nWidth, nHeight ) CLASS TButton

   ::Name      := 'button'
   ::id        := nId
   ::lRegister := .F.
   ::lControl  := .T.
   ::Msgs      := {WM_DESTROY}
   ::WndProc   := 'FormProc'
   ::Caption   := cCaption
   ::Left      := nLeft
   ::Top       := nTop
   ::Width     := nWidth
   ::Height    := nHeight 
   ::Style     := WS_CHILD + WS_VISIBLE + WS_TABSTOP + BS_PUSHBUTTON

return( super:new( oParent ) )

