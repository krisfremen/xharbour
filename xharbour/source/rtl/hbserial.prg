/*
 * $Id: hbserial.prg,v 1.11 2004/06/23 18:14:08 paultucker Exp $
 */

/*
 * xHarbour Project source code:
 * Remote Procedure Call code
 * xHarbour part
 *
 * Copyright 2003 Giancarlo Niccolai <giancarlo@niccolai.ws>
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
 
#include "error.ch" 
 
FUNCTION HB_Serialize( xValue )
   LOCAL cSerial
   LOCAL xElement, aPropertiesAndValues, aPropertyAndValue
   LOCAL nPos

   SWITCH ValType( xValue )
      CASE "A"
         cSerial :="A" + HB_CreateLen8( Len( xValue ) )

         FOR EACH xElement IN xValue
            cSerial += HB_Serialize( xElement )
         NEXT

         EXIT

      CASE "H"
         cSerial := "H" + HB_CreateLen8( Len( xValue ) )

         FOR nPos := 1 TO Len( xValue )
            cSerial += HB_Serialize( HGetKeyAt( xValue, nPos ) )
            cSerial += HB_Serialize( HGetValueAt( xValue, nPos ) )
         NEXT

         EXIT

      CASE "O"  
      	 if __objDerivedFrom(xValue,"HBPersistent")
            cSerial:= HB_SerializeSimple( xValue:ClassName )
            cSerial+= xValue:SaveToText()
            cSerial:= "Q" + HB_CreateLen8( Len( cSerial ) )+ cSerial
      	 
      	 else
            aPropertiesAndValues := __ClsGetPropertiesAndValues( xValue )
            cSerial := "O" + HB_CreateLen8( Len( aPropertiesAndValues ) )
            cSerial += HB_SerializeSimple( xValue:ClassName )

            FOR EACH aPropertyAndValue in aPropertiesAndValues
               // saves name and content
               cSerial += HB_Serialize( aPropertyAndValue[1] )
               cSerial += HB_Serialize( aPropertyAndValue[2] )
            NEXT
         end if
         EXIT

      DEFAULT
         cSerial := HB_SerializeSimple( xValue )
   END

RETURN cSerial

FUNCTION HB_Deserialize( cSerial, nMaxLen )
   LOCAL oObject
   LOCAL oElem, cClassName, aProperties, oVal
   LOCAL nLen, nClassID  
   LOCAL nLenBytes,nClassNameLen,oErr

   IF Len( cSerial ) < 2 // note
      RETURN NIL
   ENDIF

   SWITCH cSerial[1]
      CASE "A"
         oObject := Array(0)
         nLen := HB_GetLen8( Substr( cSerial, 2 ) )
         cSerial := Substr( cSerial, 10 )
         DO WHILE nLen > 0
            oElem := HB_Deserialize( cSerial, nMaxLen )
            Aadd( oObject, oElem )
            cSerial := Substr( cSerial, HB_SerialNext( cSerial )+1 )
            nLen --
         ENDDO
      EXIT

      CASE "H"
         oObject := Hash()
         nLen := HB_GetLen8( Substr( cSerial, 2 ) )
         cSerial := Substr( cSerial, 10 )
         HAllocate( oObject, nLen )

         DO WHILE nLen > 0
            oElem := HB_Deserialize( cSerial, nMaxLen )
            cSerial :=  Substr( cSerial, HB_SerialNext( cSerial )+1 )
            oVal := HB_Deserialize( cSerial, nMaxLen )
            HSet( oObject, oElem, oVal )
            cSerial := Substr( cSerial, HB_SerialNext( cSerial )+1 )
            nLen --
         ENDDO
      EXIT

      CASE "O"
         nLen := HB_GetLen8( Substr( cSerial, 2 ) )
         cSerial := Substr( cSerial, 10 )
         cClassName := HB_DeserializeSimple( cSerial, 128 )
         IF cClassName == NIL
            RETURN NIL
         ENDIF
         cSerial :=  Substr( cSerial, HB_SerialNext( cSerial )+1 )
         // Let's create a new instance of this class, if possible
         nClassID := __ClsGetHandleFromName( cClassName )
         IF nClassID == 0
            oErr := ErrorNew()
            oErr:Args          := { cSerial, nMaxLen }
            oErr:CanDefault    := .F.
            oErr:CanRetry      := .F.
            oErr:CanSubstitute := .F.
            oErr:Description   := "Cannot find class implementation: '" + cClassName + "'"
            oErr:Operation     := "HB_Deserialize()"
            oErr:Severity      := ES_ERROR
            oErr:SubCode       := 3
            oErr:SubSystem     := "Serialization"
            RETURN Eval( ErrorBlock(), oErr )
         ENDIF
         oObject := __ClsInst( nClassId )

         DO WHILE nLen > 0
            // retreives name
            oElem := HB_DeserializeSimple( cSerial, 128 )
            cSerial := Substr( cSerial, HB_SerialNext( cSerial )+1 )
            // then the value
            oVal := HB_Deserialize( cSerial, nMaxLen )

            __objSendMsgCase( oObject, "_" + oElem, oVal )
            cSerial := Substr( cSerial, HB_SerialNext( cSerial )+1 )

            nLen--
         ENDDO
      EXIT
      
      CASE "Q"                               // Object inherited from HBPersistent
         nLenBytes := HB_GetLen8( Substr( cSerial, 2 ) )
         cSerial := Substr( cSerial, 10 )
                  
         cClassName := HB_DeserializeSimple( cSerial, 128 )
         IF cClassName == NIL
            RETURN NIL
         ENDIF                       
         nClassNameLen:=HB_SerialNext( cSerial )
         cSerial :=  Substr( cSerial, nClassNameLen+1 )
         nLenBytes:=nLenBytes-nClassNameLen
         // Let's create a new instance of this class, if possible
         nClassID := __ClsGetHandleFromName( cClassName )
         IF nClassID == 0
            oErr := ErrorNew()
            oErr:Args          := { cSerial, nMaxLen }
            oErr:CanDefault    := .F.
            oErr:CanRetry      := .F.
            oErr:CanSubstitute := .F.
            oErr:Description   := "Cannot find class implementation: '" + cClassName + "'"
            oErr:Operation     := "HB_Deserialize()"
            oErr:Severity      := ES_ERROR
            oErr:SubCode       := 1
            oErr:SubSystem     := "Serialization"
            RETURN Eval( ErrorBlock(), oErr )
         ENDIF           
         
         oObject := __ClsInst( nClassId )  
         if ! __objDerivedFrom(oObject,"HBPersistent")
            oErr := ErrorNew()
            oErr:Args          := { cSerial, nMaxLen }
            oErr:CanDefault    := .F.
            oErr:CanRetry      := .F.
            oErr:CanSubstitute := .F.
            oErr:Description   := "Class is not derived from HBPersistent: '" + cClassName + "'"
            oErr:Operation     := "HB_Deserialize()"
            oErr:Severity      := ES_ERROR
            oErr:SubCode       := 2
            oErr:SubSystem     := "Serialization"
            RETURN Eval( ErrorBlock(), oErr )
         end if
 
       oObject:LoadFromText(substr(cSerial,1,nLenBytes))
       cSerial :=  Substr( cSerial,nLenBytes+1 )
         
      EXIT
      
      DEFAULT
         oObject := HB_DeserializeSimple( cSerial, nMaxLen )
   END

RETURN oObject


FUNCTION HB_DeserialNext( cSerial, nMaxLen )
   LOCAL oObject, cStr
   LOCAL nPos

   nPos := HB_GetLen8( cSerial )
   IF nPos < 0 .or. nPos >= Len( cSerial )
      RETURN NIL
   ENDIF

   cStr := Substr(cSerial, nPos)
   oObject := HB_Deserialize( cStr, nMaxLen )
   IF oObject != NIL
      nPos += HB_SerialNext( cStr )
      HB_CreateLen8( @cSerial, nPos )
   ENDIF

RETURN oObject
