#include "windows.ch"
#include "HbClass.ch"
#include "debug.ch"


CLASS TRadio FROM TControl
   METHOD New() CONSTRUCTOR
ENDCLASS

METHOD New( oParent, cCaption, nId, nLeft, nTop, nWidth, nHeight ) CLASS TRadio
   ::id        := nId
   ::lRegister := .F.
   ::lControl  := .T.
   ::Msgs      := IFNIL( ::Msgs, {WM_DESTROY}, ::Msgs )
   ::WndProc   := IFNIL( ::WndProc, 'FormProc', ::WndProc )
   ::Caption   := cCaption
   ::Left      := nLeft
   ::Top       := nTop
   ::Width     := nWidth
   ::Height    := IFNIL( nHeight, IFNIL( ::height, 20, ::height), nHeight)
   ::Name      := 'button'
   ::Style     := WS_CHILD + WS_VISIBLE + WS_TABSTOP + BS_AUTORADIOBUTTON
return( super:new( oParent  ) )

