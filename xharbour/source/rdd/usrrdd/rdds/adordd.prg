/*
 * $Id: adordd.prg,v 1.2 2007/05/07 10:22:55 marchuet Exp $
 */

/*
 * Harbour Project source code:
 * ADORDD - RDD to automatically manage Microsoft ADO
 *
 * Copyright 2007 Fernando Mancera <fmancera@viaopen.com> and
 * Antonio Linares <alinares@fivetechsoft.com>
 * www - http://www.harbour-project.org
 *
 * Copyright 2007 Miguel Angel Marchuet <miguelangel@marchuet.net>
 *  ADO_GOTOID( nWA, nRecord )
 *  ADO_GOTO( nWA, nRecord )
 *  ADO_OPEN( nWA, aOpenInfo ) some modifications
 *
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

#include "rddsys.ch"
#include "fileio.ch"
#include "error.ch"
#include "adordd.ch"
#include "common.ch"
#include "dbstruct.ch"

#ifndef __XHARBOUR__
   #include "hbusrrdd.ch"
   #xcommand TRY              => cbErr := errorBlock( {|oErr| break( oErr ) } ) ;;
                                 BEGIN SEQUENCE
   #xcommand CATCH [<!oErr!>] => errorBlock( cbErr ) ;;
                                 RECOVER [USING <oErr>] <-oErr-> ;;
                                 errorBlock( cbErr )
   #command FINALLY          =>  ALWAYS
#else   
   #include "usrrdd.ch"
#endif

ANNOUNCE ADORDD

static s_cTableName, s_cEngine, s_cServer, s_cUserName, s_cPassword
static s_cQuery := "SELECT * FROM ", s_cLocateFor
static s_aConnections[ 255 ], s_aCatalogs[ 255 ]
static s_aTableNames[ 255 ], s_aScopeInfo[ 255 ]
static s_aSQLStruct[ 255 ], cbErr

#ifdef __XHARBOUR__

static function HB_TokenGet( cText, nPos, cSep )

   local aTokens := HB_ATokens( cText, cSep )

return If( nPos <= Len( aTokens ), aTokens[ nPos ], "" )

#endif

STATIC FUNCTION ADO_INIT( nRDD )

   LOCAL aRData := ARRAY( 10 )

   AFILL( aRData, -1 )
   USRRDD_RDDDATA( nRDD, aRData )

RETURN SUCCESS

STATIC FUNCTION ADO_NEW( pWA )

   LOCAL aWData := { -1, .F., .F. }

   USRRDD_AREADATA( pWA, aWData )

RETURN SUCCESS

STATIC FUNCTION ADO_CREATE( nWA, aOpenInfo )


   local cDataBase  := HB_TokenGet( aOpenInfo[ UR_OI_NAME ], 1, ";" )
   local cTableName := HB_TokenGet( aOpenInfo[ UR_OI_NAME ], 2, ";" )
   local cDbEngine  := HB_TokenGet( aOpenInfo[ UR_OI_NAME ], 3, ";" )
   local cServer    := HB_TokenGet( aOpenInfo[ UR_OI_NAME ], 4, ";" )
   local cUserName  := HB_TokenGet( aOpenInfo[ UR_OI_NAME ], 5, ";" )
   local cPassword  := HB_TokenGet( aOpenInfo[ UR_OI_NAME ], 6, ";" )
   local oConnection := TOleAuto():New( "ADODB.Connection" )
   local oCatalog   := TOleAuto():New( "ADOX.Catalog" )

   do case
      case Upper( Right( cDataBase, 4 ) ) == ".MDB"
           if ! File( cDataBase )
              oCatalog:Create( "Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" +  + cDataBase )
           endif
           oConnection:Open( "Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" + cDataBase )

      case Upper( cDbEngine ) == "MYSQL"     
           oConnection:Open( "DRIVER={MySQL ODBC 3.51 Driver};" + ;
                             "server=" + cServer + ;
                             ";database=" + cDataBase + ;
                             ";uid=" + cUserName + ;
                             ";pwd=" + cPassword )

   endcase

   TRY
      oConnection:Execute( "DROP TABLE " + cTableName )
   CATCH
   FINALLY

      oConnection:Execute( "CREATE TABLE [" + cTableName + "] (" + s_aSQLStruct[ nWA ] + ")" )
      oConnection:Close()

   END TRY

	/*
   LOCAL oError := ErrorNew()

   oError:GenCode     := EG_CREATE
   oError:SubCode     := 1004
   oError:Description := HB_LANGERRMSG( EG_CREATE ) + " (" + ;
                         HB_LANGERRMSG( EG_UNSUPPORTED ) + ")"
   oError:FileName    := aOpenInfo[ UR_OI_NAME ]
   oError:CanDefault  := .T.
   UR_SUPER_ERROR( nWA, oError )
   */

RETURN SUCCESS

STATIC FUNCTION ADO_CREATEFIELDS( nWA, aStruct )

  local n

  s_aSQLStruct[ nWA ] = ""

  for n = 1 to Len( aStruct )
     if n > 1
        s_aSQLStruct[ nWA ] += ", "
     endif
     s_aSQLStruct[ nWA ] += "[" + aStruct[ n ][ DBS_NAME ] + "]"
     do case
        case aStruct[ n ][ DBS_TYPE ] $ "C,Character"
             s_aSQLStruct[ nWA ] += " CHAR(" + AllTrim( Str( aStruct[ n ][ DBS_LEN ] ) ) + ") NULL"

        case aStruct[ n ][ DBS_TYPE ] == "N"
             s_aSQLStruct[ nWA ] += " NUMERIC(" + AllTrim( Str( aStruct[ n ][ DBS_LEN ] ) ) + ")"

        case aStruct[ n ][ DBS_TYPE ] == "L"
             s_aSQLStruct[ nWA ] += " LOGICAL"
     endcase
  next

RETURN SUCCESS

STATIC FUNCTION ADO_OPEN( nWA, aOpenInfo )

   LOCAL cName, nMode, nSlot, nHandle, aRData, aWData, aField, oError, nResult
   LOCAL oADO, nTotalFields, n

   // When there is no ALIAS we will create new one using file name
   IF aOpenInfo[ UR_OI_ALIAS ] == NIL
      HB_FNAMESPLIT( aOpenInfo[ UR_OI_NAME ], , @cName )
      aOpenInfo[ UR_OI_ALIAS ] := cName
   ENDIF

   nMode := If( aOpenInfo[ UR_OI_SHARED ], FO_SHARED , FO_EXCLUSIVE ) + ;
            If( aOpenInfo[ UR_OI_READONLY ], FO_READ, FO_READWRITE )

   aRData := USRRDD_RDDDATA( USRRDD_ID( nWA ) )
   aWData := USRRDD_AREADATA( nWA )
   nSlot := ASCAN( aRData, -1 )

   IF nSlot == 0
      oError := ErrorNew()
      oError:GenCode     := EG_OPEN
      oError:SubCode     := 1000
      oError:Description := HB_LANGERRMSG( EG_OPEN ) + ", no free slots"
      oError:FileName    := aOpenInfo[ UR_OI_NAME ]
      oError:CanDefault  := .T.
      UR_SUPER_ERROR( nWA, oError )
      RETURN FAILURE
   ENDIF

   IF Empty( aOpenInfo[ UR_OI_CONNECT ] )
      s_aConnections[ nWA ] = TOleAuto():New( "ADODB.Connection" )

      do case
          case Lower( Right( aOpenInfo[ UR_OI_NAME ], 4 ) ) == ".mdb"
               IF Empty( s_cPassword )
                  s_aConnections[ nWA ]:Open( "Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" + aOpenInfo[ UR_OI_NAME ] )
               ELSE
                  s_aConnections[ nWA ]:Open( "Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" + aOpenInfo[ UR_OI_NAME ] + ";Jet OLEDB:Database Password=" + AllTrim( s_cPassword ) )
               ENDIF

          case Lower( Right( aOpenInfo[ UR_OI_NAME ], 4 ) ) == ".xls"
               s_aConnections[ nWA ]:Open( "Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" + aOpenInfo[ UR_OI_NAME ] + ";Extended Properties='Excel 8.0;HDR=YES';Persist Security Info=False" )

          case Lower( Right( aOpenInfo[ UR_OI_NAME ], 3 ) ) == ".db"
               s_aConnections[ nWA ]:Open( "Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" + aOpenInfo[ UR_OI_NAME ] + ";Extended Properties='Paradox 3.x';" )

          case s_cEngine == "MYSQL"
               s_aConnections[ nWA ]:Open( "DRIVER={MySQL ODBC 3.51 Driver};" + ;
                                           "server=" + s_cServer + ;
                                           ";database=" + aOpenInfo[ UR_OI_NAME ] + ;
                                           ";uid=" + s_cUserName + ;
                                           ";pwd=" + s_cPassword )

          case s_cEngine == "SQL"
               s_aConnections[ nWA ]:Open( "Provider=SQLOLEDB;" + ;
                                           "server=" + s_cServer + ;
                                           ";database=" + aOpenInfo[ UR_OI_NAME ] + ;
                                           ";uid=" + s_cUserName + ;
                                           ";pwd=" + s_cPassword )

          case s_cEngine == "ORACLE"
               s_aConnections[ nWA ]:Open( "Provider=MSDAORA.1;" + ;
                                           "Persist Security Info=False" + ;
                                           If( s_cServer == NIL .OR. s_cServer == "",;
                                               "", ";Data source=" + s_cServer ) + ;
                                           ";User ID=" + s_cUserName + ;
                                           + ";Password=" + s_cPassword )

      endcase
   ELSE
      s_aConnections[ nWA ] = TOleAuto():New( aOpenInfo[ UR_OI_CONNECT ], "ADODB.Connection" )
   ENDIF

   oADO := TOleAuto():New( "ADODB.Recordset" )

   IF oADO == NIL
      oError := ErrorNew()
      oError:GenCode     := EG_OPEN
      oError:SubCode     := 1001
      oError:Description := HB_LANGERRMSG( EG_OPEN )
      oError:FileName    := aOpenInfo[ UR_OI_NAME ]
      oError:OsCode      := fError()
      oError:CanDefault  := .T.

      UR_SUPER_ERROR( nWA, oError )
      RETURN FAILURE
   ENDIF

   oAdo:CursorType     = adOpenDynamic
   oAdo:CursorLocation = adUseClient
   oAdo:LockType       = adLockPessimistic
   oAdo:Open( s_cQuery + s_cTableName, s_aConnections[ nWA ] )

   s_aCatalogs[ nWA ] = TOleAuto():New( "ADOX.Catalog" )
   s_aCatalogs[ nWA ]:ActiveConnection = s_aConnections[ nWA ]

   s_aTableNames[ nWA ] = s_cTableName

   aRData[ nSlot ] := oADO
   aWData[ 1 ] := oADO
   aWData[ 2 ] := aWData[ 3 ] := .F.

   nTotalFields := oADO:Fields:Count

   UR_SUPER_SETFIELDEXTENT( nWA, oADO:Fields:Count )

   FOR n = 1 TO nTotalFields
   		aField := ARRAY( UR_FI_SIZE )
   		aField[ UR_FI_NAME ]    := oADO:Fields( n - 1 ):Name
   		aField[ UR_FI_TYPE ]    := ADO_GETFIELDTYPE( oADO:Fields( n - 1 ):Type )
   		aField[ UR_FI_TYPEEXT ] := 0
   		aField[ UR_FI_LEN ]     := ADO_GETFIELDSIZE( aField[ UR_FI_TYPE ], oADO:Fields( n - 1 ):DefinedSize )
   		aField[ UR_FI_DEC ]     := 0
   		UR_SUPER_ADDFIELD( nWA, aField )
   NEXT

   nResult := UR_SUPER_OPEN( nWA, aOpenInfo )

   IF nResult == SUCCESS
      ADO_GOTOP( nWA )
   ENDIF

RETURN nResult

STATIC FUNCTION ADO_CLOSE( nWA )

   LOCAL oADO := USRRDD_AREADATA( nWA )[ 1 ]

   // oADO:Close()

RETURN UR_SUPER_CLOSE( nWA )

STATIC FUNCTION ADO_GETVALUE( nWA, nField, xValue )

   LOCAL aWData := USRRDD_AREADATA( nWA )
   LOCAL oADO := USRRDD_AREADATA( nWA )[ 1 ]

   IF aWData[ 3 ]
      xValue := ""
   ELSE
      xValue := oADO:Fields( nField - 1 ):Value
   ENDIF

RETURN SUCCESS

STATIC FUNCTION ADO_GOTO( nWA, nRecord )

   LOCAL nRecNo
   LOCAL oADO := USRRDD_AREADATA( nWA )[ 1 ]

   WITH OBJECT oADO
      IF :RecordCount > 0
         :MoveFirst()
         :Move( nRecord - 1, 0 )
      ENDIF
      ADO_RECID( nWA, @nRecNo )
      RETURN If( nRecord == nRecNo, SUCCESS, FAILURE )
   END WITH


RETURN SUCCESS

STATIC FUNCTION ADO_GOTOID( nWA, nRecord )
   LOCAL nRecNo
   LOCAL oADO := USRRDD_AREADATA( nWA )[ 1 ]

   WITH OBJECT oADO
      IF :RecordCount > 0
         :MoveFirst()
         :Move( nRecord - 1, 0 )
      ENDIF
      ADO_RECID( nWA, @nRecNo )
      RETURN If( nRecord == nRecNo, SUCCESS, FAILURE )
   END WITH

RETURN FAILURE

STATIC FUNCTION ADO_GOTOP( nWA )

   local oADO := USRRDD_AREADATA( nWA )[ 1 ]

   if oADO:RecordCount != 0
      oADO:MoveFirst()
   endif
   USRRDD_AREADATA( nWA )[ 2 ] = .f.
   USRRDD_AREADATA( nWA )[ 3 ] = .f.

RETURN SUCCESS

STATIC FUNCTION ADO_GOBOTTOM( nWA )

   local oADO := USRRDD_AREADATA( nWA )[ 1 ]

   oADO:MoveLast()
   USRRDD_AREADATA( nWA )[ 2 ] = .f.
   USRRDD_AREADATA( nWA )[ 3 ] = .f.

RETURN SUCCESS

STATIC FUNCTION ADO_SKIPRAW( nWA, nRecords )

   LOCAL aWData, oADO

   IF nRecords != 0
      aWData := USRRDD_AREADATA( nWA )
      oADO := aWData[ 1 ]
      IF aWData[ 3 ]
         IF nRecords > 0
            RETURN SUCCESS
         ENDIF
         ADO_GOBOTTOM( nWA )
         ++nRecords
       ENDIF
       IF nRecords < 0 .AND. oADO:AbsolutePosition <= -nRecords
          oADO:MoveFirst()
          aWData[ 2 ] := .T.
          aWData[ 3 ] := oADO:EOF
       ELSEIF nRecords != 0
          oADO:Move( nRecords )
          aWData[ 2 ] := .F.
          aWData[ 3 ] := oADO:EOF
       ENDIF
   ENDIF

RETURN SUCCESS

STATIC FUNCTION ADO_BOF( nWA, lBof )

   LOCAL aWData := USRRDD_AREADATA( nWA )

   lBof := aWData[ 2 ]

RETURN SUCCESS

STATIC FUNCTION ADO_EOF( nWA, lEof )

   local oADO := USRRDD_AREADATA( nWA )[ 1 ]

   lEof := ( oADO:AbsolutePosition == -3 ) // lEof := aWData[ 3 ]

RETURN SUCCESS

STATIC FUNCTION ADO_DELETED( nWA, lDeleted )

	local oADO := USRRDD_AREADATA( nWA )[ 1 ]

	IF oADO:Status == adRecDeleted // To be checked, ACCESS does not uses it
	   lDeleted := .T.
	ELSE
	   lDeleted := .F.
	ENDIF

RETURN SUCCESS

STATIC FUNCTION ADO_DELETE( nWA )

	local oADO := USRRDD_AREADATA( nWA )[ 1 ]

   oADO:Delete()

   ADO_SKIPRAW( nWA, 1 )

RETURN SUCCESS

STATIC FUNCTION ADO_RECID( nWA, nRecNo )

   local oADO := USRRDD_AREADATA( nWA )[ 1 ]

	 nRecno := If( oADO:AbsolutePosition == -3, oAdo:RecordCount + 1, oAdo:AbsolutePosition )

RETURN SUCCESS

STATIC FUNCTION ADO_RECCOUNT( nWA, nRecords )

   local oADO := USRRDD_AREADATA( nWA )[ 1 ]

   nRecords := oADO:RecordCount

RETURN SUCCESS

STATIC FUNCTION ADO_PUTVALUE( nWA, nField, xValue )

   LOCAL aWData := USRRDD_AREADATA( nWA )
   LOCAL oADO := USRRDD_AREADATA( nWA )[ 1 ]

   IF aWData[ 3 ]
       xValue := ""
   ELSE
      oADO:Fields( nField - 1 ):Value := xValue
      oADO:Update()
   ENDIF

RETURN SUCCESS

STATIC FUNCTION ADO_APPEND( nWA, lUnLockAll )

	local oADO := USRRDD_AREADATA( nWA )[ 1 ]

	oADO:AddNew()

	TRY
	   oADO:Update() // keep it here, or there is an ADO error
	CATCH
   END TRY

RETURN SUCCESS

STATIC FUNCTION ADO_FLUSH( nWA )

	local oADO := USRRDD_AREADATA( nWA )[ 1 ]

	oADO:Update()

RETURN SUCCESS

STATIC FUNCTION ADO_ORDINFO( nWA, nIndex, aOrderInfo )

	local oADO := USRRDD_AREADATA( nWA )[ 1 ]

	do case
	   case nIndex == UR_ORI_TAG
	        if aOrderInfo[ UR_ORI_TAG ] < s_aCatalogs[ nWA ]:Tables( s_aTableNames[ nWA ] ):Indexes:Count
             aOrderInfo[ UR_ORI_RESULT ] = s_aCatalogs[ nWA ]:Tables( s_aTableNames[ nWA ] ):Indexes( aOrderInfo[ UR_ORI_TAG ] ):Name
          else
             aOrderInfo[ UR_ORI_RESULT ] = ""
          endif
	endcase

RETURN SUCCESS

STATIC FUNCTION ADO_ORDLSTFOCUS( nWA, aOrderInfo )
/*
   LOCAL nRecNo

   WITH OBJECT USRRDD_AREADATA( nWA )[ 1 ]
      ADO_RECID( nWA, @nRecNo )

      :Close()
      __OutDebug( aOrderInfo )
      __outDebug("SELECT * FROM " + s_aTableNames[ nWA ] )
      IF aOrderInfo[ UR_ORI_TAG ] = 0
          :Open( "SELECT * FROM " + s_aTableNames[ nWA ] , HB_QWith(), adOpenDynamic, adLockPessimistic )
      ELSE
          //:Open( "SELECT * FROM " + ::oTabla:cTabla + ' ORDER BY ' + ::OrdKey( uTag ) , QWith(), adOpenDynamic, adLockPessimistic, adCmdUnspecified )
          :Open( "SELECT * FROM " + s_aTableNames[ nWA ], HB_QWith(), adOpenDynamic, adLockPessimistic )
      ENDIF
      aOrderInfo[ UR_ORI_RESULT ] = aOrderInfo[ UR_ORI_TAG ]

      ADO_GOTOP( nWA )
      ADO_GOTO( nWA, nRecNo )
   END WITH
*/
RETURN SUCCESS

STATIC FUNCTION ADO_PACK( nWA )

	local oADO := USRRDD_AREADATA( nWA )[ 1 ]

RETURN SUCCESS

STATIC FUNCTION ADO_RAWLOCK( nWA, nAction, nRecNo )

	local oADO := USRRDD_AREADATA( nWA )[ 1 ]

RETURN SUCCESS

STATIC FUNCTION ADO_LOCK( nWA, aLockInfo  )

	LOCAL oADO := USRRDD_AREADATA( nWA )[ 1 ]

	aLockInfo[ UR_LI_METHOD ] := DBLM_MULTIPLE
   aLockInfo[ UR_LI_RECORD ] := RECNO()
   aLockInfo[ UR_LI_RESULT ] := .T.

RETURN SUCCESS

STATIC FUNCTION ADO_UNLOCK( nWA, xRecID )

	LOCAL oADO := USRRDD_AREADATA( nWA )[ 1 ]

RETURN SUCCESS

STATIC FUNCTION ADO_SETFILTER( nWA, aFilterInfo )

	LOCAL oADO := USRRDD_AREADATA( nWA )[ 1 ]

	oADO:Filter = SQLTranslate( aFilterInfo[ UR_FRI_CEXPR ] )

RETURN SUCCESS

STATIC FUNCTION ADO_CLEARFILTER( nWA )

	LOCAL oADO := USRRDD_AREADATA( nWA )[ 1 ]

   TRY
	   oADO:Filter = ""
	CATCH
   END TRY

RETURN SUCCESS

STATIC FUNCTION ADO_ZAP( nWA )

	local oADO := USRRDD_AREADATA( nWA )[ 1 ]

  if s_aConnections[ nWA ] != NIL .and. s_aTableNames[ nWA ] != nil
     s_aConnections[ nWA ]:Execute( "DELETE * FROM " + s_aTableNames[ nWA ] )
     oADO:Requery()
  endif

RETURN SUCCESS

STATIC FUNCTION ADO_SETLOCATE( nWA, aScopeInfo )

   aScopeInfo[ UR_SI_CFOR ] = SQLTranslate( s_cLocateFor )

   s_aScopeInfo[ nWA ] = aScopeInfo

return SUCCESS

STATIC FUNCTION ADO_LOCATE( nWA, lContinue )

	local oADO := USRRDD_AREADATA( nWA )[ 1 ]

  oADO:Find( s_aScopeInfo[ nWA ][ UR_SI_CFOR ], If( lContinue, 1, 0 ) )

return SUCCESS

static function ADO_CLEARREL( nWA )

   local nKeys := s_aCatalogs[ nWA ]:Tables( s_aTableNames[ nWA ] ):Keys:Count
   local cKeyName

   if nKeys > 0 
      cKeyName = s_aCatalogs[ nWA ]:Tables( s_aTableNames[ nWA ] ):Keys( nKeys - 1 ):Name
      if Upper( cKeyName ) != "PRIMARYKEY"
         s_aCatalogs[ nWA ]:Tables( s_aTableNames[ nWA ] ):Keys:Delete( cKeyName )
      endif   
   endif   

RETURN SUCCESS

STATIC FUNCTION ADO_SETREL( nWA, aRelInfo )

  local cParent := Alias( aRelInfo[ UR_RI_PARENT ] )
  local cChild := Alias( aRelInfo[ UR_RI_CHILD ] )
  local cKeyName := cParent + "_" + cChild

	s_aCatalogs[ nWA ]:Tables( s_aTableNames[ nWA ] ):Keys:Append( cKeyName, adKeyForeign,;
	                           aRelInfo[ UR_RI_CEXPR ], cChild, aRelInfo[ UR_RI_CEXPR ] )  

RETURN SUCCESS

STATIC FUNCTION ADO_ORDLSTADD( nWA, aOrderInfo )

	local oADO := USRRDD_AREADATA( nWA )[ 1 ]
	
  TRY 
	   oADO:Index = aOrderInfo[ UR_ORI_BAG ]
	CATCH
	END   
	
RETURN SUCCESS

STATIC FUNCTION ADO_ORDLSTCLEAR( nWA )

	local oADO := USRRDD_AREADATA( nWA )[ 1 ]

  TRY 
	   oADO:Index = ""
	CATCH
	END   
	
return SUCCESS

STATIC FUNCTION ADO_ORDCREATE( nWA, aOrderCreateInfo )

   local oIndex
   
   if s_aCatalogs[ nWA ]:Tables( s_aTableNames[ nWA ] ):Indexes( aOrderCreateInfo[ UR_ORCR_BAGNAME ] ) == nil
      oIndex = TOleAuto():New( "ADOX.Index" )
      oIndex:Name = aOrderCreateInfo[ UR_ORCR_BAGNAME ]
      oIndex:PrimaryKey = .F.
      oIndex:Unique = aOrderCreateInfo[ UR_ORCR_UNIQUE ]
      oIndex:Columns:Append( aOrderCreateInfo[ UR_ORCR_CKEY ] )
      s_aCatalogs[ nWA ]:Tables( s_aTableNames[ nWA ] ):Indexes:Append( oIndex )
   endif   

RETURN SUCCESS

FUNCTION ADORDD_GETFUNCTABLE( pFuncCount, pFuncTable, pSuperTable, nRddID )

   LOCAL cSuperRDD := NIL     /* NO SUPER RDD */
   LOCAL aMyFunc[ UR_METHODCOUNT ]

   aMyFunc[ UR_INIT ]      := ( @ADO_INIT() )
   aMyFunc[ UR_NEW ]       := ( @ADO_NEW() )
   aMyFunc[ UR_CREATE ]    := ( @ADO_CREATE() )
   aMyFunc[ UR_CREATEFIELDS ] := ( @ADO_CREATEFIELDS() )
   aMyFunc[ UR_OPEN ]      := ( @ADO_OPEN() )
   aMyFunc[ UR_CLOSE ]     := ( @ADO_CLOSE() )
   aMyFunc[ UR_BOF  ]      := ( @ADO_BOF() )
   aMyFunc[ UR_EOF  ]      := ( @ADO_EOF() )
   aMyFunc[ UR_DELETED ]   := ( @ADO_DELETED() )
   aMyFunc[ UR_SKIPRAW ]   := ( @ADO_SKIPRAW() )
   aMyFunc[ UR_GOTO ]      := ( @ADO_GOTO() )
   aMyFunc[ UR_GOTOID ]    := ( @ADO_GOTOID() )
   aMyFunc[ UR_GOTOP ]     := ( @ADO_GOTOP() )
   aMyFunc[ UR_GOBOTTOM ]  := ( @ADO_GOBOTTOM() )
   aMyFunc[ UR_RECID ]     := ( @ADO_RECID() )
   aMyFunc[ UR_RECCOUNT ]  := ( @ADO_RECCOUNT() )
   aMyFunc[ UR_GETVALUE ]  := ( @ADO_GETVALUE() )
   aMyFunc[ UR_PUTVALUE ]  := ( @ADO_PUTVALUE() )
   aMyFunc[ UR_DELETE ]  	:= ( @ADO_DELETE() )
   aMyFunc[ UR_APPEND ]    := ( @ADO_APPEND() )
   aMyFunc[ UR_FLUSH ]     := ( @ADO_FLUSH() )
   aMyFunc[ UR_ORDINFO ]   := ( @ADO_ORDINFO() )
   aMyFunc[ UR_ORDLSTFOCUS ] := ( @ADO_ORDLSTFOCUS() )
   aMyFunc[ UR_PACK ]      := ( @ADO_PACK() )
   aMyFunc[ UR_RAWLOCK ]   := ( @ADO_RAWLOCK() )
   aMyFunc[ UR_LOCK ]      := ( @ADO_LOCK() )
   aMyFunc[ UR_UNLOCK ]    := ( @ADO_UNLOCK() )
   aMyFunc[ UR_SETFILTER ] := ( @ADO_SETFILTER() )
   aMyFunc[ UR_CLEARFILTER ] := ( @ADO_CLEARFILTER() )
   aMyFunc[ UR_ZAP ]       := ( @ADO_ZAP() )
   aMyFunc[ UR_SETLOCATE ] := ( @ADO_SETLOCATE() )
   aMyFunc[ UR_LOCATE ]  	:= ( @ADO_LOCATE() )
   aMyFunc[ UR_CLEARREL ]  := ( @ADO_CLEARREL() )
   aMyFunc[ UR_SETREL ]    := ( @ADO_SETREL() )
   aMyFunc[ UR_ORDCREATE ] := ( @ADO_ORDCREATE() )
   aMyFunc[ UR_ORDLSTADD ] := ( @ADO_ORDLSTADD() )
   aMyFunc[ UR_ORDLSTCLEAR ] := ( @ADO_ORDLSTCLEAR() )

RETURN USRRDD_GETFUNCTABLE( pFuncCount, pFuncTable, pSuperTable, nRddID, ;
                                                    cSuperRDD, aMyFunc )

INIT PROC ADORDD_INIT()
   rddRegister( "ADORDD", RDT_FULL )
RETURN

STATIC FUNCTION ADO_GETFIELDSIZE( nDBFTypeField, nADOFielSize )

	LOCAL nDBFFieldSize := 0

   DO CASE

			CASE nDBFTypeField == HB_FT_STRING
           nDBFFieldSize := nADOFielSize

			CASE nDBFTypeField == HB_FT_INTEGER
           nDBFFieldSize := nADOFielSize

			CASE nDBFTypeField == HB_FT_DATE
           nDBFFieldSize := 8

			CASE nDBFTypeField == HB_FT_LOGICAL
           nDBFFieldSize := 1

   ENDCASE

RETURN nDBFFieldSize

STATIC FUNCTION ADO_GETFIELDTYPE( nADOFielfType )

	LOCAL nDBFTypeField := 0

	DO CASE

		CASE nADOFielfType == adEmpty						// 0
		CASE nADOFielfType == adTinyInt 					// 16
		CASE nADOFielfType == adSmallInt 					// 2
		CASE nADOFielfType == adInteger 					// 3
         nDBFTypeField := HB_FT_INTEGER

		CASE nADOFielfType == adBigInt 						// 20
		CASE nADOFielfType == adUnsignedTinyInt 	    // 17
		CASE nADOFielfType == adUnsignedSmallInt 	// 18
		CASE nADOFielfType == adUnsignedInt 			// 19
		CASE nADOFielfType == adUnsignedBigInt 		// 21
		CASE nADOFielfType == adSingle 						// 4
		CASE nADOFielfType == adDouble 						// 5
		CASE nADOFielfType == adCurrency 					// 6
		CASE nADOFielfType == adDecimal 					// 14
		CASE nADOFielfType == adNumeric 					// 131
		CASE nADOFielfType == adBoolean 					// 11
         nDBFTypeField := HB_FT_LOGICAL

		CASE nADOFielfType == adError 						// 10
		CASE nADOFielfType == adUserDefined 			    // 132
		CASE nADOFielfType == adVariant 					// 12
		CASE nADOFielfType == adIDispatch 				    // 9
		CASE nADOFielfType == adIUnknown 				// 13
		CASE nADOFielfType == adGUID 						// 72
		CASE nADOFielfType == adDate 						// 7
         nDBFTypeField := HB_FT_DATE

		CASE nADOFielfType == adDBDate 					// 133
		CASE nADOFielfType == adDBTime 					// 134
		CASE nADOFielfType == adDBTimeStamp 			// 135
		CASE nADOFielfType == adBSTR 						// 8
		CASE nADOFielfType == adChar 						// 129
         nDBFTypeField := HB_FT_STRING

		CASE nADOFielfType == adVarChar 					// 200
         nDBFTypeField := HB_FT_STRING

		CASE nADOFielfType == adLongVarChar 			// 201
         nDBFTypeField := HB_FT_STRING

		CASE nADOFielfType == adWChar 						// 130
         nDBFTypeField := HB_FT_STRING

		CASE nADOFielfType == adVarWChar 				// 202
         nDBFTypeField := HB_FT_STRING

		CASE nADOFielfType == adLongVarWChar 			// 203
         nDBFTypeField := HB_FT_STRING

		CASE nADOFielfType == adBinary 						// 128
		CASE nADOFielfType == adVarBinary 				// 204
		CASE nADOFielfType == adLongVarBinary 		    // 205
		CASE nADOFielfType == adChapter 					// 136
		CASE nADOFielfType == adFileTime 					// 64
		CASE nADOFielfType == adPropVariant 			    // 138
		CASE nADOFielfType == adVarNumeric 				// 139
		// CASE nADOFielfType == adArray &H2000

   ENDCASE

RETURN nDBFTypeField

function HB_AdoSetTable( cTableName )

   s_cTableName = cTableName

return nil

function HB_AdoSetEngine( cEngine )

   s_cEngine = cEngine

return nil

function HB_AdoSetServer( cServer )

   s_cServer = cServer

return nil

function HB_AdoSetUser( cUser )

   s_cUserName = cUser

return nil

function HB_AdoSetPassword( cPassword )

   s_cPassword = cPassword

return nil

function HB_AdoSetQuery( cQuery )

   DEFAULT cQuery TO "SELECT * FROM "

   s_cQuery = cQuery

return nil

function HB_AdoSetLocateFor( cLocateFor )

   s_cLocateFor = cLocateFor

return nil

static function SQLTranslate( cExpr )

  if Left( cExpr, 1 ) == '"' .and. Right( cExpr, 1 ) == '"'
     cExpr = SubStr( cExpr, 2, Len( cExpr ) - 2 )
  endif

  cExpr = StrTran( cExpr, '""', "" )
  cExpr = StrTran( cExpr, '"', "'" )
  cExpr = StrTran( cExpr, "''", "'" )
  cExpr = StrTran( cExpr, "==", "=" )
  cExpr = StrTran( cExpr, ".and.", "AND" )
  cExpr = StrTran( cExpr, ".or.", "OR" )
  cExpr = StrTran( cExpr, ".AND.", "AND" )
  cExpr = StrTran( cExpr, ".OR.", "OR" )

return cExpr