/*
 * $Id: tclass.prg,v 1.14 2004/07/29 23:55:59 ronpinkas Exp $
 */

/*
 * Harbour Project source code:
 * Base Class for internal handling of class creation
 *
 * Copyright 1999 Antonio Linares <alinares@fivetech.com>
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

/*
 * The following parts are Copyright of the individual authors.
 * www - http://www.harbour-project.org
 *
 * Copyright 2000 J. Lefebvre <jfl@mafact.com> & RA. Cuylen <rac@mafact.com>
 *    Multiple inheritance
 *    Support shared class DATA
 *    scoping (hidden, protected, readOnly)
 *    Use of __cls_param function to allow multiple superclass declaration
 *    Suppress of SetType and SetInit not more nedded
 *    Delegation and forwarding
 *    Preparing the InitClass class method (not working !! )
 *
 * Copyright 1999 Eddie Runia <eddie@runia.com>
 *    Support for inheritance
 *    Support for default DATA values
 *
 * See doc/license.txt for licensing terms.
 *
 */

// Harbour Class HBClass to build classes

#include "common.ch"
#include "hboo.ch"

#include "hbclass.ch"

STATIC s_nDataId

REQUEST HBObject

FUNCTION HBClass()

   STATIC s_hClass /* NOTE: Automatically default to NIL */

   IF s_hClass == NIL
      s_hClass := __clsNew( "HBCLASS", 11, 17 )
      __ClsSetModule( s_hClass )

      __clsAddMsg( s_hClass, "New"            , @New()            , HB_OO_MSG_METHOD )
      __clsAddMsg( s_hClass, "Create"         , @Create()         , HB_OO_MSG_METHOD )
      __clsAddMsg( s_hClass, "AddData"        , @AddData()        , HB_OO_MSG_METHOD )
      __clsAddMsg( s_hClass, "AddMultiData"   , @AddMultiData()   , HB_OO_MSG_METHOD )
      __clsAddMsg( s_hClass, "AddClassData"   , @AddClassData()   , HB_OO_MSG_METHOD )
      __clsAddMsg( s_hClass, "AddMultiClsData", @AddMultiClsData(), HB_OO_MSG_METHOD )
      __clsAddMsg( s_hClass, "AddInline"      , @AddInline()      , HB_OO_MSG_METHOD )
      __clsAddMsg( s_hClass, "AddMethod"      , @AddMethod()      , HB_OO_MSG_METHOD )
      __clsAddMsg( s_hClass, "AddClsMethod"   , @AddClsMethod()   , HB_OO_MSG_METHOD )
      __clsAddMsg( s_hClass, "AddVirtual"     , @AddVirtual()     , HB_OO_MSG_METHOD )
      __clsAddMsg( s_hClass, "Instance"       , @Instance()       , HB_OO_MSG_METHOD )
      __clsAddMsg( s_hClass, "SetOnError"     , @SetOnError()     , HB_OO_MSG_METHOD )
      __clsAddMsg( s_hClass, "SetDestructor"  , @SetDestructor()  , HB_OO_MSG_METHOD )
      __clsAddMsg( s_hClass, "InitClass"      , @InitClass()      , HB_OO_MSG_METHOD )
      __clsAddMsg( s_hClass, "ConstructorCall", @ConstructorCall(), HB_OO_MSG_METHOD )
      __clsAddMsg( s_hClass, "cSuper"         , {| Self | IIF( ::acSuper == NIL .OR. Len( ::acSuper ) == 0, NIL, ::acSuper[ 1 ] ) }, HB_OO_MSG_INLINE )
      __clsAddMsg( s_hClass, "_cSuper"        , {| Self, xVal | IIF( ::acSuper == NIL .OR. Len( ::acSuper ) == 0, ( ::acSuper := { xVal } ), ::acSuper[ 1 ] := xVal ), xVal }, HB_OO_MSG_INLINE )

      __clsAddMsg( s_hClass, "hClass"         ,  1, HB_OO_MSG_PROPERTY )
      __clsAddMsg( s_hClass, "cName"          ,  2, HB_OO_MSG_PROPERTY )
      __clsAddMsg( s_hClass, "aDatas"         ,  3, HB_OO_MSG_PROPERTY )
      __clsAddMsg( s_hClass, "aMethods"       ,  4, HB_OO_MSG_PROPERTY )
      __clsAddMsg( s_hClass, "aClsDatas"      ,  5, HB_OO_MSG_PROPERTY )
      __clsAddMsg( s_hClass, "aClsMethods"    ,  6, HB_OO_MSG_PROPERTY )
      __clsAddMsg( s_hClass, "aInlines"       ,  7, HB_OO_MSG_PROPERTY )
      __clsAddMsg( s_hClass, "aVirtuals"      ,  8, HB_OO_MSG_PROPERTY )
      __clsAddMsg( s_hClass, "acSuper"        ,  9, HB_OO_MSG_PROPERTY )
      __clsAddMsg( s_hClass, "nOnError"       , 10, HB_OO_MSG_PROPERTY )
      __clsAddMsg( s_hClass, "nDestructor"    , 11, HB_OO_MSG_PROPERTY )

   ENDIF

RETURN __clsInst( s_hClass )

//----------------------------------------------------------------------------//

// xSuper is used here as the new preprocessor file (HBCLASS.CH) send here
// always an array (if no superclass, this will be an empty one)
// In case of direct class creation (without the help of preprocessor) xSuper can be
// either NIL or contain the name of the superclass.

STATIC FUNCTION New( cClassName, xSuper )

   LOCAL Self := QSelf()
   LOCAL nSuper, i
   LOCAL cSuper

   IF ISARRAY( xSuper ) .AND. Len( xSuper ) >= 1
      ::acSuper := xSuper
      nSuper := Len( xSuper )
   ELSEIF ISCHARACTER( xSuper ) .AND. ! empty( xSuper )
      ::acSuper := { xSuper }
      nSuper := 1
   ELSE
      ::acSuper := {}
      nSuper := 0
   ENDIF

   ::cName       := Upper( cClassName )

   ::aDatas      := {}
   ::aMethods    := {}
   ::aClsDatas   := {}
   ::aClsMethods := {}
   ::aInlines    := {}
   ::aVirtuals   := {}

   i := -1
   FOR EACH cSuper IN ::acSuper
      IF ! ISCHARACTER( cSuper )
         i := HB_EnumIndex()
         EXIT
      ENDIF
   NEXT

   IF i > 0
      nSuper := i - 1
      ASize( ::acSuper, nSuper )
   ENDIF

RETURN QSelf()

//----------------------------------------------------------------------------//

STATIC PROCEDURE Create( MetaClass )

   LOCAL Self := QSelf()
   LOCAL n
   LOCAL nLen := Len( ::acSuper )
   LOCAL nLenDatas := Len( ::aDatas ) //Datas local to the class !!
   LOCAL nDataBegin := 0
   LOCAL nClassBegin := 0
   LOCAL hClass
   LOCAL ahSuper := Array( nLen )
   LOCAL nExtraMsgs := Len( ::aMethods ) +  ( 2 * Len( ::aClsDatas ) ) + Len( ::aInlines ) + Len( ::aVirtuals )
   LOCAL cDato
   LOCAL hSuper

   IF nLen == 0
      hClass := __ClsNew( ::cName, nLenDatas, nExtraMsgs )
   ELSE                                         // Multi inheritance
      FOR EACH cDato IN ::acSuper
         hSuper := __ClsGetHandleFromName( cDato )
         IF hSuper == 0
            ahSuper[ HB_EnumIndex() ] := __clsInstSuper( Upper( cDato ) )
         ELSE
            ahSuper[ HB_EnumIndex() ] := hSuper
         ENDIF

         IF ahSuper[ HB_EnumIndex() ] == 0
            Throw( ErrorNew( "TClass", 0, 1003, ProcName(), "Could not locate super: " + cDato, HB_aParams() ) )
         ENDIF
      NEXT

      hClass := __ClsNew( ::cName, nLenDatas + nlen, nExtraMsgs, ahSuper )

      FOR EACH cDato IN ahSuper
         nDataBegin   += __cls_CntData( cDato )        // Get offset for new Datas
         nClassBegin  += __cls_CntClsData( cDato )     // Get offset for new ClassData
      NEXT

      __clsAddMsg( hClass, Upper( ::acSuper[ 1 ] ), ++nDataBegin, HB_OO_MSG_SUPER, ahSuper[ 1 ], HB_OO_CLSTP_CLASS + 1 )
      // nData begin stay here the same so as, SUPER and __SUPER will share the same pointer to super object with the first one.
      __clsAddMsg( hClass, "SUPER"                , nDataBegin, HB_OO_MSG_SUPER, ahSuper[ 1 ], 1 )
      __clsAddMsg( hClass, "__SUPER"              , nDataBegin, HB_OO_MSG_SUPER, ahSuper[ 1 ], 1 )

      FOR n := 2 TO nLen
         __clsAddMsg( hClass, Upper( ::acSuper[ n ] ), ++nDataBegin, HB_OO_MSG_SUPER, ahSuper[ n ], HB_OO_CLSTP_CLASS + 1 )
      NEXT
   ENDIF

   ::hClass := hClass

   // We will work here on the MetaClass object to add the Class Method
   // as needed
   FOR EACH cDato IN ::aClsMethods
       IF !( __objHasMsg( Self, cDato[ HB_OO_MTHD_SYMBOL ] ) )
          __clsAddMsg( hClass, cDato[ HB_OO_MTHD_SYMBOL ], cDato[ HB_OO_MTHD_PFUNCTION ], HB_OO_MSG_METHOD, NIL, cDato[ HB_OO_MTHD_SCOPE ] )
       ENDIF
   NEXT

   FOR EACH cDato IN ::aDatas
      __clsAddMsg( hClass, cDato[ HB_OO_DATA_SYMBOL ]       , HB_EnumIndex() + nDataBegin, ;
                   HB_OO_MSG_PROPERTY, cDato[ HB_OO_DATA_VALUE ], cDato[ HB_OO_DATA_SCOPE ],;
                   cDato[ HB_OO_DATA_PERSISTENT ] )
   NEXT

   FOR EACH cDato IN ::aMethods
      __clsAddMsg( hClass, cDato[ HB_OO_MTHD_SYMBOL ], cDato[ HB_OO_MTHD_PFUNCTION ], HB_OO_MSG_METHOD, NIL, cDato[ HB_OO_MTHD_SCOPE ],;
                   cDato[ HB_OO_MTHD_PERSISTENT ] )
   NEXT

   FOR EACH cDato IN ::aClsDatas
      __clsAddMsg( hClass, cDato[ HB_OO_CLSD_SYMBOL ]      , HB_EnumIndex() + nClassBegin,;
                   HB_OO_MSG_CLASSPROPERTY, cDato[ HB_OO_CLSD_VALUE ], cDato[ HB_OO_CLSD_SCOPE ] )
   NEXT

   FOR EACH cDato IN ::aInlines
      __clsAddMsg( hClass, cDato[ HB_OO_MTHD_SYMBOL ], cDato[ HB_OO_MTHD_PFUNCTION ],;
                   HB_OO_MSG_INLINE, NIL, cDato[ HB_OO_MTHD_SCOPE ],;
                   cDato[ HB_OO_MTHD_PERSISTENT ] )
   NEXT

   FOR EACH cDato IN ::aVirtuals
      __clsAddMsg( hClass, cDato, HB_EnumIndex(), HB_OO_MSG_VIRTUAL )
   NEXT

   IF ::nOnError != NIL
      __clsAddMsg( hClass, "__OnError", ::nOnError, HB_OO_MSG_ONERROR )
   ENDIF

   IF ::nDestructor != NIL
      __clsAddMsg( hClass, "__Destructor", ::nDestructor, HB_OO_MSG_DESTRUCTOR )
   ENDIF

RETURN

//----------------------------------------------------------------------------//

STATIC FUNCTION Instance()

    LOCAL Self := QSelf()
    Local oInstance := __clsInst( ::hClass )
    /*oInstance:Class := Self:Class*/

RETURN oInstance

//----------------------------------------------------------------------------//

STATIC PROCEDURE AddData( cData, xInit, cType, nScope, lNoinit, lPersistent )

   LOCAL Self := QSelf()

   if lNoInit==NIL;lNoInit:=.F.;endif
   if lPersistent == nil; lpersistent := .f.; endif

   // Default Init for Logical and numeric
   IF ! lNoInit .AND. cType != NIL .AND. xInit == NIL
      IF Upper( Left( cType, 1 ) ) == "L"
         xInit := .F.
      ELSEIF Upper( Left( cType, 1 ) ) IN "NI"   /* Numeric Int */
         xInit := 0
      ENDIF
   ENDIF

   AAdd( ::aDatas, { cData, xInit, cType, nScope, lPersistent } )

RETURN

//----------------------------------------------------------------------------//

STATIC PROCEDURE AddMultiData( cType, xInit, nScope, aData, lNoInit, lPersistent )

   LOCAL Self := QSelf()
   LOCAL i
   LOCAL nParam // := Len( aData )
   LOCAL cData

   i := -1
   FOR EACH cData IN aData
      IF ! ISCHARACTER( cData )
         i := HB_EnumIndex()
         EXIT
      ENDIF
//      i++
   NEXT

   IF i > 0
      nParam := i - 1
      ASize( aData, nParam )
   ENDIF

   FOR EACH cData IN aData
      ::AddData( cData, xInit, cType, nScope, lNoInit, lPersistent )
   NEXT

RETURN

//----------------------------------------------------------------------------//

STATIC PROCEDURE AddClassData( cData, xInit, cType, nScope, lNoInit )

   LOCAL Self := QSelf()

   if lNoInit==NIL;lNoInit:=.F.;endif

   // Default Init for Logical and numeric
   IF ! lNoInit .AND. cType != NIL .AND. xInit == NIL
      IF Upper( Left( cType, 1 ) ) == "L"
         xInit := .F.
      ELSEIF Upper( Left( cType, 1 ) ) IN "NI"  /* Numeric Int */
         xInit := 0
      ENDIF
   ENDIF

   AAdd( ::aClsDatas, { cData, xInit, cType, nScope } )

RETURN

//----------------------------------------------------------------------------//

STATIC PROCEDURE AddMultiClsData( cType, xInit, nScope, aData, lNoInit )

   LOCAL Self := QSelf()
   LOCAL i
   LOCAL nParam // := Len( aData )
   LOCAL cData

   i := -1
   FOR EACH cData IN aData
      IF ! ISCHARACTER( cData )
         i := HB_EnumIndex()
         EXIT
      ENDIF
//      i++
   NEXT

   IF i > 0
      nParam := i - 1
      ASize( aData, nParam )
   ENDIF

   FOR EACH cData IN aData
      ::AddClassData( cData, xInit, cType, nScope, lNoInit )
   NEXT

RETURN

//----------------------------------------------------------------------------//

STATIC PROCEDURE AddInline( cMethod, bCode, nScope, lPersistent )

   LOCAL Self := QSelf(), nAt

   /* Remove possible ( <x,...> )*/
   IF ( nAt := At( "(", cMethod ) ) > 0
      cMethod := RTrim( Left( cMethod, nAt - 1 ) )
   ENDIF

   AAdd( ::aInlines, { cMethod, bCode, nScope, lPersistent } )

RETURN

//----------------------------------------------------------------------------//

STATIC PROCEDURE AddMethod( cMethod, nFuncPtr, nScope, lPersistent )

   LOCAL Self := QSelf(), nAt

   /* Remove possible ( <x,...> )*/
   IF ( nAt := At( "(", cMethod ) ) > 0
      cMethod := RTrim( Left( cMethod, nAt - 1 ) )
   ENDIF

   AAdd( ::aMethods, { cMethod, nFuncPtr, nScope, lPersistent } )

RETURN

//----------------------------------------------------------------------------//

STATIC PROCEDURE AddClsMethod( cMethod, nFuncPtr, nScope )

   LOCAL Self := QSelf(), nAt

   /* Remove possible ( <x,...> )*/
   IF ( nAt := At( "(", cMethod ) ) > 0
      cMethod := RTrim( Left( cMethod, nAt - 1 ) )
   ENDIF

   AAdd( ::aClsMethods, { cMethod, nFuncPtr, nScope } )

RETURN

//----------------------------------------------------------------------------//
STATIC PROCEDURE AddVirtual( cMethod )

   LOCAL Self := QSelf(), nAt

   /* Remove possible ( <x,...> )*/
   IF ( nAt := At( "(", cMethod ) ) > 0
      cMethod := RTrim( Left( cMethod, nAt - 1 ) )
   ENDIF

   AAdd( ::aVirtuals, cMethod )

RETURN

//----------------------------------------------------------------------------//

STATIC PROCEDURE SetOnError( nFuncPtr )

   LOCAL Self := QSelf()

   ::nOnError := nFuncPtr

RETURN


//----------------------------------------------------------------------------//

STATIC PROCEDURE SetDestructor( nFuncPtr )

   LOCAL Self := QSelf()

   ::nDestructor := nFuncPtr

RETURN

//----------------------------------------------------------------------------//

STATIC FUNCTION InitClass()

   LOCAL Self := QSelf()

RETURN Self

//----------------------------------------------------------------------------//

/*
 * (C) 2002 - Francesco Saverio Giudice
 *
 * Used to autoinitialize a class
 *
 * FSG 2003/11/05 - Fixed with right check of class constructor method
*/
STATIC FUNCTION ConstructorCall( oClass, aParams )
   LOCAL Self := QSelf()
   LOCAL aConstrMethods
   LOCAL lClassAutoInit := __SetClassAutoInit()
   LOCAL lOldScope, nPos

   IF lClassAutoInit .AND. Len( aParams ) > 0
     // Set class scoping off
     lOldScope := __SetClassScope( .F. )

       // Get method full list but limited to those with class type as constructor
       aConstrMethods  := __objGetMsgFullList( oClass, .F., HB_MSGLISTALL, HB_OO_CLSTP_CTOR )

       // Search the constructor which is not derived from a parent class
       //aEval( aConstrMethods, {|aMth| TraceLog( "aScan",  aMth[HB_OO_DATA_SYMBOL], aMth[HB_OO_DATA_SCOPE], ;
       //                                         hb_BitAnd( aMth[HB_OO_DATA_SCOPE], HB_OO_CLSTP_SUPER ) ) } )
       nPos := aScan( aConstrMethods, {|aMth| hb_BitAnd( aMth[HB_OO_DATA_SCOPE], HB_OO_CLSTP_SUPER ) == 0 } )

     // Revert class scoping
     __SetClassScope( lOldScope )

     IF nPos > 0
        // Exec method - i have found the constructor in this class
        //TraceLog( "Search this class constructor:", aConstrMethods[ nPos ][HB_OO_DATA_SYMBOL] )
        HB_ExecFromArray( oClass, aConstrMethods[ nPos ][ HB_OO_DATA_SYMBOL ], aParams )
     ELSE
        // Get LAST constructor from parent (NOTE: this can be a default and faster way,
        // but i prefer check rightly before)
        IF !Empty( aConstrMethods )
           //TraceLog( "Search parent class constructor:", aTail( aConstrMethods )[HB_OO_DATA_SYMBOL] )
           HB_ExecFromArray( oClass, aTail( aConstrMethods )[ HB_OO_DATA_SYMBOL ], aParams )
        ELSE
           //TraceLog( "Call new default constructor:", "NEW" )
           // If i have no constructor i call NEW method that is defined is HBOBJECT class
           HB_ExecFromArray( oClass, "NEW", aParams )
        ENDIF
     ENDIF

   ENDIF

RETURN Self

//----------------------------------------------------------------------------//

FUNCTION __ClassNew( cName, nDatas )

    LOCAL hClass := __ClsNew( cName, nDatas )

    __ClsSetModule( hClass )

    s_nDataId := 0

RETURN hClass

//----------------------------------------------------------------------------//

FUNCTION __ClassAdd( hClass, cProperty, cFunction )

   cProperty := Upper( cProperty )

   IF cProperty[1] == '_'
      RETURN __ClsAddMsg( hClass, cProperty, ++s_nDataId, HB_OO_MSG_DATA )
   ENDIF

RETURN __ClsAddMsg( hClass, cProperty, HB_FuncPtr( cFunction ), HB_OO_MSG_METHOD )

//----------------------------------------------------------------------------//

FUNCTION __ClassIns( hClass )

RETURN __ClsInst( hClass )

//----------------------------------------------------------------------------//

CLASS SCALAROBJECT
   METHOD IsScalar() INLINE .T.
ENDCLASS

//----------------------------------------------------------------------------//

CLASS ARRAY FROM SCALAROBJECT FUNCTION _ARRAY
   METHOD _Size( nLen )         INLINE aSize( Self, nLen ), nLen
   METHOD Add( xValue )         INLINE aAdd( Self, xValue ), Self
   METHOD AddAll( oCollection )
   METHOD AtIndex( nPos )       INLINE Self[nPos]
   METHOD AtPut( nPos, xValue ) INLINE Self[ nPos ] := xValue
   METHOD Append( xValue )      INLINE aAdd( Self, xValue ), Self
   METHOD AsString()            INLINE ValToPrgExp( HB_QSelf() )
   METHOD Collect( bCollect )
   METHOD Copy()                INLINE aCopy( Self, Array( Len( Self ) ) )
   METHOD DeleteAt( nPos )
   METHOD Do( bBlock )
   METHOD IndexOf( xValue )
   METHOD Init( nLen )          INLINE ::Size := IIF( nLen == NIL, 0, nLen ), Self
   METHOD InsertAt( nPos, xValue )
   METHOD Remove( xValue )
   METHOD Scan( bScan )         INLINE aScan( Self, bScan )

ENDCLASS

METHOD AddAll( oCollection )
   oCollection:Do( { |xValue| ::Add(xValue) } )
RETURn Self

METHOD Collect( bCollect ) CLASS ARRAY

   LOCAL xElement, aResult[0]

   FOR EACH xElement IN Self
      IF Eval( bCollect, UnRef( xElement ) )
          aAdd( aResult, UnRef( xElement ) )
      END
   NEXT

RETURN aResult

METHOD DeleteAt( nPos ) CLASS ARRAY

   IF nPos > 0 .AND. nPos <= Len( Self )
      aDel( Self, nPos, .T. )
   ENDIF

RETURN Self

METHOD Do( bEval ) CLASS ARRAY
   LOCAL xElement

   FOR EACH xElement IN Self
      bEval:Eval( UnRef( xElement ), HB_EnumIndex() )
   NEXT

RETURN Self

METHOD IndexOf( xValue ) CLASS ARRAY
   LOCAL xElement, cType := ValType( xValue )

   FOR EACH xElement IN Self
      IF ValType( xElement ) == cType .AND. xElement == xValue
         RETURN HB_EnumIndex()
      END
   NEXT

RETURN 0

METHOD InsertAt( nPos, xValue ) CLASS ARRAY

   IF nPos > Len( self )
      aSize( Self, nPos )
      Self[ nPos ] := xValue
   ELSEIF nPos > 0
      aIns( Self, nPos, xValue, .T. )
   ENDIF

RETURN Self

METHOD Remove( xValue ) CLASS ARRAY
   ::DeleteAt( ::IndexOf( xValue ) )
RETURN Self

//----------------------------------------------------------------------------//
CLASS BLOCK FROM SCALAROBJECT FUNCTION _BLOCK
   METHOD AsString() INLINE "{||...}"
ENDCLASS

//----------------------------------------------------------------------------//

CLASS CHARACTER FROM SCALAROBJECT FUNCTION _CHARACTER
   METHOD AsString INLINE HB_QSelf()
ENDCLASS

//----------------------------------------------------------------------------//

CLASS DATE FROM SCALAROBJECT FUNCTION _DATE
   METHOD AsString INLINE DToc( HB_QSelf() )
ENDCLASS

//----------------------------------------------------------------------------//

CLASS LOGICAL FROM SCALAROBJECT FUNCTION _LOGICAL
   METHOD AsString INLINE IIF( HB_QSelf(), ".T.", ".F." )
ENDCLASS

//----------------------------------------------------------------------------//

CLASS NIL FROM SCALAROBJECT FUNCTION _NIL
   METHOD AsString INLINE "NIL"
ENDCLASS

//----------------------------------------------------------------------------//

CLASS NUMERIC FROM SCALAROBJECT FUNCTION _NUMERIC
   METHOD AsString INLINE LTrim( Str( ( HB_QSelf() ) ) )
ENDCLASS

//----------------------------------------------------------------------------//

CLASS POINTER FROM SCALAROBJECT FUNCTION _POINTER
   METHOD AsString INLINE "0x" + HB_NumToHex( HB_QSelf() )
ENDCLASS

FUNCTION UnRef( xValue )
RETURN xValue
