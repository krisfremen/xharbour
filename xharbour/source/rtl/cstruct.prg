/*
 * $Id: cstruct.prg,v 1.21 2003/03/26 03:33:51 ronpinkas Exp $
 */

/*
 * xHarbour Project source code:
 * C Structure Support.
 *
 * Copyright 2000 Ron Pinkas <ronpinkas@profit-master.com>
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

#include "hboo.ch"
#include "cstruct.ch"
#include "error.ch"

#define CLASS_PROPERTIES 5

static s_aActiveStructure
static s_aClasses := {}
static s_aArrayClasses := {}

//---------------------------------------------------------------------------//
Function __ActiveStructure( cStructure, nAlign )

   LOCAL oErr
   LOCAL acMembers, aCTypes, hClass, Counter, cMember


   IF PCount() == 2
      cStructure := Upper( cStructure )

      IF aScan( s_aClasses, { | aClassInfo | aClassInfo[1] == cStructure } ) > 0
         /* In most cases we can simply ignore the reduefinition, by returning a FAKED Structure Array!
         oErr := ErrorNew()
         oErr:Args := { cStructure, nAlign }
         oErr:CanDefault    := .F.
         oErr:CanRetry      := .F.
         oErr:CanSubstitute := .T.
         oErr:Description  := "Structure already defined."
         oErr:Operation     := "__ActiveStructure()"
         oErr:Severity      := ES_ERROR
         oErr:SubCode       := 1
         oErr:SubSystem     := "C Structure"

         RETURN Eval( ErrorBlock(), oErr )
         */

         // In most cases we can simply ignore the redefinition, by returning a FAKED Structure Array!
         //TraceLog( "Redefinition of C Structure: " + cStructure )
         RETURN ( s_aActiveStructure := { cStructure, NIL, {}, {}, IIF( ValType( nAlign ) == "N", nAlign, 8 ) } )
      END

      aAdd( s_aClasses, { cStructure, NIL, {}, {}, IIF( ValType( nAlign ) == "N", nAlign, 8 ) } )
      //TraceLog( "Registered: " + cStructure, s_aClasses[-1][5] )

      s_aActiveStructure := s_aClasses[-1]
   ELSE
      //TraceLog( "Created: " + Str( nId ) )

      acMembers := s_aActiveStructure[3]
      aCTypes   := s_aActiveStructure[4]
      nAlign    := s_aActiveStructure[5]

      hClass := __clsNew( "C Structure " + s_aActiveStructure[1] , Len( aCTypes ) + CLASS_PROPERTIES, 8 )

      s_aActiveStructure[2] := hClass

      __clsAddMsg( hClass,  "Reset"     , @Reset()      , HB_OO_MSG_METHOD )
      __clsAddMsg( hClass,  "Buffer"    , @Buffer()     , HB_OO_MSG_METHOD )
      __clsAddMsg( hClass,  "Value"     , @Value()      , HB_OO_MSG_METHOD )
      __clsAddMsg( hClass,  "DeValue"   , @DeValue()    , HB_OO_MSG_METHOD )
      __clsAddMsg( hClass,  "Array"     , @ArrayMethod(), HB_OO_MSG_METHOD )
      __clsAddMsg( hClass,  "SayMembers", @SayMembers() , HB_OO_MSG_METHOD )
      __clsAddMsg( hClass,  "Init"      , @Init()       , HB_OO_MSG_METHOD )
      __clsAddMsg( hClass,  "Pointer"   , @Pointer()    , HB_OO_MSG_METHOD )

      FOR EACH cMember IN acMembers
         __clsAddMsg( hClass,       cMember, HB_EnumIndex(), HB_OO_MSG_PROPERTY )
      NEXT

      Counter := Len( acMembers ) + 1
      __clsAddMsg( hClass,  "aCTypes"       , Counter, HB_OO_MSG_PROPERTY, acTypes )

      Counter++
      __clsAddMsg( hClass,  "nAlign"        , Counter, HB_OO_MSG_PROPERTY, nAlign, HB_OO_CLSTP_READONLY )

      Counter++
      __clsAddMsg( hClass,  "SizeOf"        , Counter, HB_OO_MSG_PROPERTY, HB_SizeOfCStructure( aCTypes, nAlign ), HB_OO_CLSTP_READONLY )

      Counter++
      __clsAddMsg( hClass,  "nID"           , Counter, HB_OO_MSG_PROPERTY, Len( s_aClasses ) )

      // WARNING InternalBuffer *MUST* remain the *LAST* Property!!!
      Counter++
      __clsAddMsg( hClass,  "InternalBuffer", Counter, HB_OO_MSG_PROPERTY, , HB_OO_CLSTP_READONLY )

      //TraceLog( Len( aCTypes ), aCTypes[1], aCTypes )
      RETURN hClass
   ENDIF

RETURN s_aActiveStructure

//---------------------------------------------------------------------------//
Procedure HB_Member( cMember, CType )

  LOCAL nLen, nAt

  IF cMember[-1] == "]"
     nAt := At( "[", cMember )
     //nLen := Val( SubStr( cMember, nAt + 1, Len( cMember ) ) )
     // Support expressions like x + y, x - y, x * y
     nLen := &( SubStr( cMember, nAt + 1, Len( cMember ) - nAt - 1 ) )

     aAdd( s_aActiveStructure[3], Left( cMember, nAt - 1 ) )
     aAdd( s_aActiveStructure[4], HB_CTypeArrayID( CType, nLen ) )
  ELSE
     aAdd( s_aActiveStructure[3], cMember )
     aAdd( s_aActiveStructure[4], CType )
  ENDIF

Return

//---------------------------------------------------------------------------//
Function HB_CStructureID( cStructure, lInplace )

   cStructure := Upper( cStructure )

RETURN aScan( s_aClasses, { | aClassInfo | aClassInfo[1] == cStructure } ) + IIF( lInplace, CTYPE_STRUCTURE, CTYPE_STRUCTURE_PTR )

//---------------------------------------------------------------------------//
Procedure HB_CStructureCSyntax( cStructure, aDefinitions, cTag, cSynonList, nAlign )

   LOCAL cElem, nAt, nIndex := 1
   LOCAL nLen, Counter, CType
   LOCAL oErr

   FOR EACH cElem IN aDefinitions
       // *** PP bug - remove when possible! ***
       IF cElem == NIL
          aSize( aDefinitions, nIndex - 1 )
          EXIT
       ENDIF

       IF ( nAt := At( "*", cElem ) ) > 1
          IF nIndex < Len( aDefinitions )
             aIns( aDefinitions, nIndex + 1, SubStr( cElem, nAt + 1 ), .T. )
          ELSE
             aAdd( aDefinitions, SubStr( cElem, nAt + 1 ) )
          ENDIF

          aDefinitions[nIndex] := StrTran( Left( cElem, nAt ), " ", "" )
       ELSEIF ( nAt := At( "-", cElem ) ) > 1
          IF nIndex < Len( aDefinitions )
             aIns( aDefinitions, nIndex + 1, SubStr( cElem, nAt ), .T. )
          ELSE
             aAdd( aDefinitions, SubStr( cElem, nAt ) )
          ENDIF

          aDefinitions[nIndex] := RTrim( Left( cElem, nAt - 1 ) )
       ENDIF

       nIndex++
   NEXT

   __ActiveStructure( cStructure, nAlign )
   nLen := Len( aDefinitions )

   FOR Counter := 1 TO nLen STEP 2
      //TraceLog( "Member: " + aDefinitions[Counter + 1], "Type: " + aDefinitions[Counter] )
      CType := aDefinitions[Counter]
      IF Val( CType ) != 0
         HB_Member( aDefinitions[Counter + 1], Val( aDefinitions[Counter] ) )
      ELSE
         IF CType[-1] == '*'
            CType := HB_CStructureID( Left( CType, Len( CType ) - 1 ), .F. )
         ELSE
            CType := HB_CStructureID( CType, .T. )

            IF CType == CTYPE_STRUCTURE .OR. CType == CTYPE_STRUCTURE_PTR
               oErr := ErrorNew()
               oErr:Args          := { cStructure, aDefinitions, cTag, cSynonList, nAlign }
               oErr:CanDefault    := .F.
               oErr:CanRetry      := .F.
               oErr:CanSubstitute := .T.
               oErr:Description   := "Undefined CType: '" + aDefinitions[Counter] + "'"
               oErr:Operation     := "HB_CStructureCSyntax()"
               oErr:Severity      := ES_ERROR
               oErr:SubCode       := 2
               oErr:SubSystem     := "C Structure"

               CType := Eval( ErrorBlock(), oErr )
            ENDIF
         ENDIF

         HB_Member( aDefinitions[Counter + 1], CType )
      ENDIF
   NEXT

RETURN

//---------------------------------------------------------------------------//
Function HB_CStructure( cStructure, nAlign )

   LOCAL hClass
   LOCAL oStructure
   LOCAL nID
   LOCAL aMemberDefinition
   LOCAL aStructure
   LOCAL oErr

   cStructure := Upper( cStructure )
   nID        := aScan( s_aClasses, { | aClassInfo | aClassInfo[1] == cStructure } )

   IF nID == 0
      oErr := ErrorNew()
      oErr:Args          := { cStructure, nAlign }
      oErr:CanDefault    := .F.
      oErr:CanRetry      := .F.
      oErr:CanSubstitute := .T.
      oErr:Description   := "Structure not initialized with __ActiveStructure()"
      oErr:Operation     := "HB_CStructure()"
      oErr:Severity      := ES_ERROR
      oErr:SubCode       := 3
      oErr:SubSystem     := "C Structure"

      RETURN Eval( ErrorBlock(), oErr )
   ENDIF

   hClass := s_aClasses[nId][2]

   oStructure := __clsInst( hClass )

   AllocateMembers( oStructure )

RETURN oStructure

//---------------------------------------------------------------------------//
static Procedure AllocateMembers( oStructure )

   LOCAL aCTypes := oStructure:aCTypes, CType

   //TraceLog( "Scaning: " + oStructure:ClassName )

   FOR EACH CType IN aCTypes
      IF CType > CTYPE_STRUCTURE .AND. CType < CTYPE_STRUCTURE_PTR
         oStructure[ HB_EnumIndex() ] := HB_CStructureFromID( CType, , .F. )
         AllocateMembers( oStructure[ HB_EnumIndex() ] )
      ENDIF
   NEXT

   //TraceLog( "Finished: " + oStructure:ClassName )

Return

//---------------------------------------------------------------------------//
Function HB_CStructureFromID( nID, nAlign )

   LOCAL hClass, oStructure, lInplace
   LOCAL CType
   LOCAL oErr

   //TraceLog( nId, s_aClasses )

   IF nID > CTYPE_STRUCTURE_PTR
      lInplace := .F.
      nID -= CTYPE_STRUCTURE_PTR
   ELSEIF nID > CTYPE_STRUCTURE
      lInplace := .T.
      nID -= CTYPE_STRUCTURE
   ELSE
      oErr := ErrorNew()
      oErr:Args          := { nID, nAlign }
      oErr:CanDefault    := .F.
      oErr:CanRetry      := .F.
      oErr:CanSubstitute := .T.
      oErr:Description  := "ID out of range."
      oErr:Operation     := "HB_CStructureFromID()"
      oErr:Severity      := ES_ERROR
      oErr:SubCode       := 4
      oErr:SubSystem     := "C Structure"

      RETURN Eval( ErrorBlock(), oErr )
   ENDIF

   IF s_aClasses[nID][2] == NIL
      // Meta class was not created yet.
      RETURN HB_CStructure( s_aClasses[nId][1] )
   ELSE
      hClass := s_aClasses[nId][2]

      oStructure := __clsInst( hClass )
   ENDIF

RETURN oStructure

//---------------------------------------------------------------------------//
Function HB_CTypeArrayID( CType, nLen )

   LOCAL hClass
   LOCAL Counter
   LOCAL nID
   LOCAL aCTypes, acMembers, cMember
   LOCAL cArrayClassName := "C Array of [" + LTrim( Str( nLen ) ) + "] CType: " + Str( CType )

   nID := aScan( s_aArrayClasses, { | aArrayDefinitions | aArrayDefinitions[1] == CType .AND. aArrayDefinitions[2] == nLen } )

   IF nID == 0
      hClass := __clsNew( "C Structure " + cArrayClassName, nLen + CLASS_PROPERTIES, 8 )
      __ClsSetModule( hClass )

      aAdd( s_aClasses, { cArrayClassName, hClass, Array( nLen ), Array( nLen ), 1 } )
      nID := Len( s_aClasses )

      acMembers := s_aClasses[nID][3]
      aCTypes   := s_aClasses[nID][4]

      // Sames as s_aClasses[nID][4]
      aFill( aCTypes, CType )

      aAdd( s_aArrayClasses, { CType, nLen, nID } )

      __clsAddMsg( hClass,  "Reset"     , @Reset()      , HB_OO_MSG_METHOD )
      __clsAddMsg( hClass,  "Buffer"    , @Buffer()     , HB_OO_MSG_METHOD )
      __clsAddMsg( hClass,  "Value"     , @Value()      , HB_OO_MSG_METHOD )
      __clsAddMsg( hClass,  "DeValue"   , @DeValue()    , HB_OO_MSG_METHOD )
      __clsAddMsg( hClass,  "Array"     , @ArrayMethod(), HB_OO_MSG_METHOD )
      __clsAddMsg( hClass,  "SayMembers", @SayMembers() , HB_OO_MSG_METHOD )
      __clsAddMsg( hClass,  "Init"      , @Init()       , HB_OO_MSG_METHOD )
      __clsAddMsg( hClass,  "Pointer"   , @Pointer()    , HB_OO_MSG_METHOD )

      FOR Counter := 1 TO nLen
         cMember := LTrim( Str( Counter ) )

         acMembers[Counter] := cMember

         __clsAddMsg( hClass,       cMember, Counter, HB_OO_MSG_PROPERTY )
      NEXT

      __clsAddMsg( hClass,  "aCTypes"       , Counter, HB_OO_MSG_PROPERTY, aCTypes )

      Counter++
      __clsAddMsg( hClass,  "nAlign"        , Counter, HB_OO_MSG_PROPERTY, 1, HB_OO_CLSTP_READONLY )

      Counter++
      __clsAddMsg( hClass,  "SizeOf"        , Counter, HB_OO_MSG_PROPERTY, HB_SizeOfCStructure( aCTypes, 1 ), HB_OO_CLSTP_READONLY )

      Counter++
      __clsAddMsg( hClass,  "nID", Counter, HB_OO_MSG_PROPERTY, nID )

      // WARNING InternalBuffer *MUST* remain the *LAST* Property!!!
      Counter++
      __clsAddMsg( hClass,  "InternalBuffer", Counter, HB_OO_MSG_PROPERTY, , HB_OO_CLSTP_READONLY )

      //TraceLog( "Registered: " + cArrayClassName, nID, Len( s_aArrayClasses ) )
   ELSE
      nID := s_aArrayClasses[nID][3]
      //TraceLog( "Reused: " + s_aClasses[nID][1], nID )
   ENDIF

RETURN nID + CTYPE_STRUCTURE

//---------------------------------------------------------------------------//
Function HB_IS_CStructure( x )

RETURN Left( x:ClassName(), 11 ) == "C Structure"

//---------------------------------------------------------------------------//
Static Function SayMembers( cPad )

   LOCAL xProperty

   IF cPad == NIL
      cPad := ""
   ENDIF

   QOut( cPad + SubStr( QSelf():ClassName, 13 ) )
   QOut( cPad + Replicate( "-", Len( SubStr( QSelf():ClassName, 13 ) ) ) )

   FOR EACH xProperty IN QSelf():Array
      IF HB_IS_CStructure( xProperty )
         xProperty:SayMembers( cPad + cPad )
      ELSE
         QOut( cPad + ":", xProperty )
      END
   NEXT

RETURN QSelf()

//---------------------------------------------------------------------------//
STATIC Function Reset()

   aFill( QSelf(), NIL, 1, Len( QSelf() ) - CLASS_PROPERTIES )

RETURN QSelf()

//---------------------------------------------------------------------------//
STATIC Function Buffer( Buffer, lAdopt )

   IF ValType( Buffer ) == "C"
      QSelf():InternalBuffer := Buffer
      QSelf():DeValue( lAdopt )
   ENDIF

   IF ValType( QSelf():InternalBuffer ) != "C"
      QSelf():InternalBuffer := QSelf():Value()
   ENDIF

RETURN QSelf():InternalBuffer

//---------------------------------------------------------------------------//
STATIC Function Value()

   LOCAL aValues := {}

   aEval( QSelf(), {|xVal| aAdd( aValues, xVal ) }, 1, Len( QSelf() ) - CLASS_PROPERTIES )

   QSelf():InternalBuffer := HB_ArrayToStructure( aValues, QSelf():aCTypes, QSelf():nAlign )

RETURN QSelf():InternalBuffer

//---------------------------------------------------------------------------//
STATIC Function DeValue( lAdopt )

   LOCAL aValues
   LOCAL xProperty, nLen := Len( QSelf() ) - CLASS_PROPERTIES
   LOCAL Buffer := QSelf():InternalBuffer

   //TraceLog( QSelf():ClassName(), QSelf():nAlign, Buffer, Len( Buffer ) )

   IF ValType( Buffer ) != "C" .OR. Len( Buffer ) == 0
      aValues := Array( nLen )
   ELSE
      aValues := HB_StructureToArray( Buffer, QSelf():aCTypes, QSelf():nAlign, lAdopt  )
   ENDIF

   FOR EACH xProperty IN QSelf()
      IF HB_EnumIndex() > nLen
         EXIT
      ENDIF

      xProperty := aValues[ HB_EnumIndex() ]
   NEXT

RETURN aValues

//---------------------------------------------------------------------------//
STATIC Function ArrayMethod()

   LOCAL aValues := {}

   aEval( QSelf(), {|xVal| aAdd( aValues, xVal ) }, 1, Len( QSelf() ) - CLASS_PROPERTIES )

RETURN aValues

//---------------------------------------------------------------------------//
STATIC Function Init( aValues )

   LOCAL xProperty, Counter, nLen := Len( aValues )

   FOR EACH xProperty IN QSelf()
      IF HB_EnumIndex() > nLen
         EXIT
      ENDIF

      IF Left( xProperty:ClassName, 11 ) == "C Structure"
         xProperty:Init( aValues[ HB_EnumIndex() ] )
      ELSE
         xProperty := aValues[ HB_EnumIndex() ]
      ENDIF
   NEXT

RETURN QSelf()

//---------------------------------------------------------------------------//
STATIC Function Pointer( nNewPointer, lAdopt )

   LOCAL nPointer := HB_String2Pointer( QSelf():Buffer() )

   IF nNewPointer != NIL
      QSelf():Buffer( HB_Pointer2String( nNewPointer ), QSelf():SizeOf(), lAdopt )
   ENDIF

RETURN nPointer
//---------------------------------------------------------------------------//
