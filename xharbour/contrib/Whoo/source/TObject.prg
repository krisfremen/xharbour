/*
*-----------------------------------------------------------------------------
* WoopGUI for Harbour - Win32 OOP GUI library source code
* Copyright 2002 Francesco Saverio Giudice <info@fsgiudice.com>
*
*-----------------------------------------------------------------------------
* Parts of this project come from:
* "Harbour MiniGUI"
*                   Copyright 2002 Roberto Lopez <roblez@ciudad.com.ar>
*                   http://www.geocities.com/harbour_minigui/
* "Harbour GUI framework for Win32"
*                   Copyright 2001 Alexander S.Kresin <alex@belacy.belgorod.su>
*                   Copyright 2001 Antonio Linares <alinares@fivetech.com>
*                   http://www.harbour-project.org
*-----------------------------------------------------------------------------
*
* Base Class - All windows and controls derive from this class
* NOT USE THIS CLASS DIRECTLY !!!
*/

#include "woopgui.ch"
#include "common.ch"
#include "hbclass.ch"
//#include "windows.ch"

CLASS TObject  // FROM HBObject

    DATA cargo                          // cargo as in Clipper

    METHOD New() CONSTRUCTOR

    /*
    // This data and methods came from standard [x]Harbour object class

    DATA    ClassName               // Class Name
    DATA    ClassH                  // Class
    DATA    ClassSel

    METHOD New()                   INLINE Self
    METHOD Init()                  INLINE Self

    METHOD GetMessageList( lDataMethod, nClassType ) ;
                                   INLINE __objGetMsgList( Self, lDataMethod, nClassType )
    METHOD GetMethodList()         INLINE __objGetMethodList( Self )
    METHOD GetValueList( aExcept ) INLINE __objGetValueList( Self, aExcept )
    METHOD SetValueList( aData )   INLINE __ObjSetValueList( Self, aData )
    METHOD AddMethod( cSymbol, nFuncPtr ) ;
                                   INLINE __objAddMethod( Self, cSymbol, nFuncPtr )
    METHOD AddInline( cSymbol, bInline );
                                   INLINE __objAddInline( Self, cSymbol, bInline )
    METHOD AddData( cSymbol)       INLINE __objAddData( Self, cSymbol )
    METHOD ModifyMethod( cSymbol, nFuncPtr ) ;
                                   INLINE __objModMethod( Self, cSymbol, nFuncPtr )
    METHOD ModifyInline( cSymbol, bInline ) ;
                                   INLINE __objModInline( Self, cSymbol, bInline )
    METHOD DeleteMethod( cSymbol ) INLINE __objDelMethod( Self, cSymbol )
    METHOD DeleteInline( cSymbol ) INLINE __objDelInline( Self, cSymbol )
    METHOD DeleteData( cSymbol )   INLINE __objDelData( Self, cSymbol )
    METHOD IsDerivedFrom( cGrandFather AS STRING ) ;
                                    INLINE __objDerivedFrom( Self, Upper( cGrandFather ) )
    */

    // FSG - as Delphi
    METHOD AfterConstruction()           VIRTUAL
	METHOD BeforeDestruction()           VIRTUAL
	METHOD ClassInfo()                   VIRTUAL  // FSG - to be implemented
	//METHOD ClassName                            // FSG - already have from harbour
	METHOD ClassNameIs( cName )          INLINE  Upper( cName ) == ::ClassName
	METHOD ClassParent()                 INLINE  ::Super
	METHOD ClassType                     VIRTUAL  // FSG - to be implemented
	METHOD CleanupInstance               VIRTUAL  // FSG - to be implemented
	METHOD Create                        VIRTUAL  // FSG - to be implemented
	METHOD DefaultHandler                VIRTUAL
	METHOD Destroy                       VIRTUAL
	METHOD Dispatch                      VIRTUAL
	METHOD FieldAddress                  VIRTUAL  // FSG - to be implemented
	METHOD Free                          INLINE  IIF( Self <> NIL, ::Destroy(), NIL ), Self := NIL
	METHOD FreeInstance                  VIRTUAL
	METHOD GetInterface                  VIRTUAL  // FSG - to be implemented
	METHOD GetInterfaceEntry             VIRTUAL  // FSG - to be implemented
    METHOD GetInterfaceTable             VIRTUAL  // FSG - to be implemented
	METHOD InheritsFrom( cGrandFather )  INLINE  ::IsDerivedFrom( cGrandFather AS STRING )
	METHOD InitInstance                  VIRTUAL  // FSG - to be implemented
	METHOD InstanceSize                  VIRTUAL  // FSG - to be implemented
	METHOD MethodAddress                 VIRTUAL  // FSG - to be implemented
	METHOD MethodName                    VIRTUAL  // FSG - to be implemented
	METHOD NewInstance                   VIRTUAL
	METHOD SafeCallException             VIRTUAL  // FSG - to be implemented


    // Mine
    METHOD DisplayData()
    METHOD DisplayMethods()
    METHOD DisplayArray()

ENDCLASS

METHOD New() CLASS TObject

RETURN Self

// ---------------------------------------------------------------------------------------

METHOD DisplayArray( aData AS ARRAY, lMessageBox AS LOGICAL ) CLASS TObject
   local cString := "", i

      DEFAULT lMessageBox TO TRUE

      FOR i = 1 to len ( aData )
          cString += Str( i ) + " - " + cStr( aData[i] )
          cString += CRLF
      NEXT

   IF lMessageBox
      MessageBox(, cString, "Array Data" )
   ENDIF

RETURN cString

METHOD DisplayData( cText AS STRING, lMessageBox AS LOGICAL ) CLASS TObject
   local cString := "", i, aData
   local oB := Self

      DEFAULT cText       TO ""
      DEFAULT lMessageBox TO TRUE

      cString += "Object Name: " + oB:ClassName + CRLF

      aData := __objGetValueList( oB )
      FOR i = 1 to len ( aData )
          cString += "DATA name: " + Pad( aData[ i, HB_OO_DATA_SYMBOL ], 25 )
          cString += " - type: " + ValType( aData[ i, HB_OO_DATA_VALUE  ] )
          cString += " - value: " + Pad( cStr( aData[ i, HB_OO_DATA_VALUE  ] ), 30 )
          cString += CRLF
      NEXT

   IF lMessageBox
      MessageBox(, cString, "Object Data" + ;
                            IIF( Len( cText ) > 0, " - " + cText, "" ) )
   ENDIF

RETURN cString

METHOD DisplayMethods( cText AS STRING, lMessageBox AS LOGICAL ) CLASS TObject
   local cString := "", i, aData
   local oB := Self

      DEFAULT cText       TO ""
      DEFAULT lMessageBox TO TRUE

      cString += "Object Name: " + oB:ClassName + CRLF

      aData := __objGetMethodList( oB )
      FOR i = 1 to len ( aData )
          cString += "Method name: " + Pad( aData[ i ], 25 )
          //cString += " - type: " + ValType( aData[ i, HB_OO_DATA_VALUE  ] )
          //cString += " - value: " + Pad( cStr( aData[ i, HB_OO_DATA_VALUE  ] ), 30 )
          cString += CRLF
      NEXT

   IF lMessageBox
      MessageBox(, cString, "Object Methods" + ;
                            IIF( Len( cText ) > 0, " - " + cText, "" ) )
   ENDIF

RETURN cString

