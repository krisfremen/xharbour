// Augusto Infante
// Whoo.lib

#include "hbclass.ch"
#include "windows.ch"
#include "what32.ch"

#Define RCF_DIALOG     0
#Define RCF_WINDOW     1
#Define RCF_MDIFRAME   2
#Define RCF_MDICHILD   4

*-----------------------------------------------------------------------------*

CLASS TFrame FROM TWindow
   DATA Controls INIT {}
   METHOD New()
   METHOD Add()
   METHOD SetLink()
   METHOD GetObj()
ENDCLASS

*-----------------------------------------------------------------------------*

METHOD New( oParent ) CLASS TFrame

   ::WndProc   := 'FormProc'
   ::Msgs      := -1
   ::FrameWnd  := .T.
   ::Style     := WS_OVERLAPPEDWINDOW
   ::ExStyle   := WS_EX_APPWINDOW
   ::FormType  := RCF_WINDOW
   ::lRegister := .T.
   InitCommonControls()
//   InitCommonControlsEx(ICC_BAR_CLASSES)

   return( super:New( oParent ) )

*-----------------------------------------------------------------------------*

METHOD Add( cName, oObj, lCreate ) CLASS TFrame
   
   DEFAULT lCreate TO .T.
   oObj:propname := cName
   ::SetLink( cName, oObj )
   IF lCreate
      oObj:Create()
   endif
   AADD( ::Controls, oObj )

   return( oObj )

*-----------------------------------------------------------------------------*

METHOD SetLink( cName, oObj ) CLASS TFrame
   __objAddData( self, cName )
   __ObjSetValueList( self, { { cName, oObj } } )
return( oObj )

*-----------------------------------------------------------------------------*

METHOD GetObj( cName ) CLASS TFrame
   local n:= ASCAN( ::Controls,{|o|o:propname==cName} )
   if n>0
      return( ::Controls[n] )
   endif
return(nil)
