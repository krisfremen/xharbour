/*
 * $Id: ttable.prg,v 1.1 2002/12/24 00:42:47 lculik Exp $
 */

/*
 * Harbour Project source code:
 * Table,Record and Field Class
 *
 * Copyright 2000-2003 Manos Aspradakis maspr@otenet.gr
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
 *
 * Copyright 2000 -2002 Luiz Rafael Culik
 * Methods CreateTable(),Gentable(),AddField()
 * Plus optimization for Xharbour
 *
 */


#include "hbclass.ch"
#include "ttable.ch"
#include "set.ch"
#include "ord.ch"
#include "common.ch"
#include "inkey.ch"
#include "dbinfo.ch"
#define COMPILE(c) &("{||" + c + "}")

//request DBFCDX
STATIC saTables := {}
/* NetWork Functions */
STATIC snNetDelay    := 30
STATIC slNetOk       := .F.
STATIC scNetMsgColor := "GR+/R"

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
FUNCTION NetDbUse( cDataBase, cAlias, nSeconds, cDriver, ;
                      lNew, lOpenMode, lReadOnly )
   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   LOCAL nKey       := 0
   LOCAL lForever
   LOCAL cOldScreen := SAVESCREEN( MAXROW(), 0, MAXROW(), MAXCOL() + 1 )
   LOCAL lFirstPass := .T.

   DEFAULT cDriver := "DBFCDX"
   DEFAULT lNew := .T.
   DEFAULT lOpenMode := NET_OPEN_MODE
   DEFAULT lReadOnly := .F.
   DEFAULT nSeconds := snNetDelay

   slNetOk  := .F.
   nSeconds *= 1.00
   lforever := ( nSeconds = 0 )

   KEYBOARD CHR( 255 )
   INKEY()

   DO WHILE ( lforever .or. nSeconds > 0 ) .and. LASTKEY() != K_ESC
      IF !lfirstPass
         DISPOUTAT( MAXROW(), 0, ;
                    PADC( "Network retry ³ " + ;
                    LTRIM( STR( nSeconds, 4, 1 ) ) + " ³ ESCape = Exit ", ;
                    MAXCOL() + 1 ), ;
                    scNetMsgColor )
         lFirstPass := .F.
      ENDIF

      DBUSEAREA( lNew, ;
                 ( cDriver ), ( cDatabase ), ( cAlias ), ;
                 lOpenMode, ;
                 .F. )

      IF !NETERR()  // USE SUCCEEDS
         RESTSCREEN( MAXROW(), 0, MAXROW(), MAXCOL() + 1, cOldScreen )
         slNetOk := .T.
      ELSE
         lFirstPass := .F.
      ENDIF

      IF !slNetOK
         nKey     := INKEY( .5 )        // WAIT 1 SECOND
         nSeconds -= .5
      ELSE
         RESTSCREEN( MAXROW(), 0, MAXROW(), MAXCOL() + 1, cOldScreen )
         EXIT
      ENDIF

      IF nKey == K_ESC
         RESTSCREEN( MAXROW(), 0, MAXROW(), MAXCOL() + 1, cOldScreen )
         EXIT
      ENDIF

   ENDDO

   RESTSCREEN( MAXROW(), 0, MAXROW(), MAXCOL() + 1, cOldScreen )

RETURN ( slNetOk )

// ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
FUNCTION NetLock( nType, lReleaseLocks, nSeconds )

   // ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   LOCAL cSave       := SAVESCREEN( MAXROW(), 0, MAXROW(), MAXCOL() + 1 )
   LOCAL lContinue   := .T.
   LOCAL lSuccess    := .F.
   LOCAL nWaitTime
   LOCAL bOperation
   LOCAL xIdentifier
   LOCAL nKey        := 0
   LOCAL nCh
   LOCAL cWord

   IF .not. ( VALTYPE( nType ) == "N" ) .or. ;
              ( ( .not. ( nType == 1 ) ) .and. ;
              ( .not. ( nType == 2 ) ) .and. ;
              ( .not. ( nType == 3 ) ) )
      ALERT( "Invalid Argument passed to NETLOCK()" )
      RETURN ( lSuccess )
   ENDIF

   DEFAULT lReleaseLocks := .F.
   DEFAULT nSeconds := snNetDelay

   nWaitTime := nSeconds

   SWITCH nType
   CASE NET_RECLOCK                        // 1 = Record Lock...
      xIdentifier := IF( lReleaseLocks, NIL, RECNO() )
      bOperation  := { | x | DBRLOCK( x ) }
      exit
   CASE NET_FILELOCK                       // 2 = File Lock...
      bOperation := { | x | FLOCK() }
      exit
   CASE NET_APPEND                         // 3 = Append Blank...
      xIdentifier := lReleaseLocks
      bOperation  := { | x | DBAPPEND( x ), !NETERR() }
      exit
   END

   slNetOk := .F.

   WHILE lContinue == .T.
      /*
   IF (nKey := INKEY()) == K_ESC
      RestScreen( maxrow(),0,maxrow(),maxcol()+1, cSave)
      EXIT
   ENDIF
   */
      WHILE nSeconds > 0 .and. lContinue == .T.
         IF EVAL( bOperation, xIdentifier )
            nSeconds  := 0
            lSuccess  := .T.
            lContinue := .F.
            slNetOK   := .T.
            EXIT
         ELSE
            IF nType == 1
               cWord := "( " + DBINFO( 33 ) + " - Record Lock )"
            ELSEIF nType == 1
               cWord := "( " + DBINFO( 33 ) + " - File Lock )"
            ELSEIF nType == 3
               cWord := "( " + DBINFO( 33 ) + " - File Append )"
            ELSE
               cWord := "( " + DBINFO( 33 ) + " -  ??? "
            ENDIF

            DISPOUTAT( MAXROW(), 0, ;
                       PADC( "Network Retry " + cWord + " ³ " + STR( nSeconds, 3 ) + " ³ ESC Exit", MAXCOL() + 1 ), ;
                       scNetMsgColor )

            nKey := INKEY( 1 )          //TONE( 1,1 )
            nSeconds --                 //.5
            IF nKey == K_ESC
               RESTSCREEN( MAXROW(), 0, MAXROW(), MAXCOL() + 1, cSave )
               EXIT
            ENDIF
         ENDIF
      ENDDO

      IF ( nKey := LASTKEY() ) == K_ESC
         RESTSCREEN( MAXROW(), 0, MAXROW(), MAXCOL() + 1, cSave )
         EXIT
      ENDIF

      IF !lSuccess
         nSeconds := nWaitTime
         nCh      := ALERT( RETRY_MSG, { "  YES  ", "  NO  " } )

         IF nCh == 1
            lContinue := .T.
         ELSE
            lContinue := .F.
         ENDIF

         IF lContinue == .F.
            //EXIT
            RESTSCREEN( MAXROW(), 0, MAXROW(), MAXCOL() + 1, cSave )
            RETURN ( lSuccess )
         ENDIF

      ENDIF
   ENDDO

   RESTSCREEN( MAXROW(), 0, MAXROW(), MAXCOL() + 1, cSave )

RETURN ( lSuccess )

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
FUNCTION NetFunc( bBlock, nSeconds )

   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   LOCAL lForever      // Retry forever?

   DEFAULT nSeconds := snNetDelay
   lForever := ( nSeconds == 0 )

   // Keep trying as long as specified or default
   DO WHILE ( lForever .or. ( nSeconds > 0 ) )

      IF EVAL( bBlock )
         RETURN ( .T. )                 // NOTE
      ENDIF

      INKEY( 1 )    // Wait 0.5 seconds
      nSeconds -= 0.5
   ENDDO

RETURN ( .F. )      // Not locked

// { DBFName, Alias, { idx Names } }
// Returns:   0   All Ok
//           -1   DBF File not found
//           -2   DBF File open Error
//           -3   Index File open Error

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
FUNCTION NetOpenFiles( aFiles )

   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   LOCAL i
   LOCAL n
   LOCAL nRet := 0

   FOR i := 1 TO LEN( aFiles )

      IF !FILE( aFiles[ i, 1 ] )
         nRet := - 1
         EXIT
      ENDIF

      IF NetDbUse( aFiles[ i, 1 ], aFiles[ i, 2 ], snNetDelay, "DBFCDX" )
         IF VALTYPE( aFiles[ i, 3 ] ) == "A"
            FOR n := 1 TO LEN( aFiles[ i, 3 ] )
               IF FILE( aFiles[ i, 3, n ] )
                  ORDLISTADD( aFiles[ i, 3, n ] )
               ELSE
                  nRet := - 3
                  EXIT
               ENDIF
            NEXT
         ENDIF
      ELSE
         nRet := - 2
         EXIT
      ENDIF
   NEXT

   RETURN nRet

   /*
ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
³  NETWORK METHODS                                                 ³
ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
*/

   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
FUNCTION NetDelete()

   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   slNetOK := .F.

   IF NetLock( NET_RECLOCK ) == .T.
      DBDELETE()
      slNetOK := .T.
   ENDIF

   IF !NETERR()
      DBSKIP( 0 ) 
      DBCOMMIT()
   ELSE
      slNetOK := .T.
      ALERT( " Failed to DELETE Record -> " + STR( RECNO() ) )
   ENDIF
RETURN ( slNetOk )

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
FUNCTION NetReCall()

   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   slNetOk := .F.

   IF NetLock( NET_RECLOCK ) == .T.
      DBRECALL()
      slNetOk := .T.
   ENDIF

   IF !NETERR()
      DBSKIP( 0 ) 
      DBCOMMIT()
   ELSE
      slNetOK := .T.
      ALERT( " Failed to RECALL Record -> " + STR( RECNO() ) )
   ENDIF

RETURN ( slNetOk )

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
FUNCTION NetRecLock( nSeconds )

   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   DEFAULT nSeconds := snNetDelay

   slNetOK := .F.

   IF NetLock( NET_RECLOCK,, nSeconds )                     // 1
      slNetOK := .T.
   ENDIF

RETURN ( slNetOK )

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
FUNCTION NetFileLock( nSeconds )

   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   slNetOK := .F.
   DEFAULT nSeconds := snNetDelay

   IF NetLock( NET_FILELOCK,, nSeconds )
      slNetOK := .T.
   ENDIF

RETURN ( slNetOK )

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
FUNCTION NetAppend( nSeconds, lReleaseLocks )

   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   LOCAL nOrd := 0
   DEFAULT lReleaseLocks := .T.
   DEFAULT nSeconds := snNetDelay
   slNetOK := .F.
   nOrd    := ORDSETFOCUS( 0 )          // --> set order to 0 to append ???

   IF NetLock( NET_APPEND,, nSeconds )
      //DbGoBottom()
      slNetOK := .T.
   ENDIF

   ORDSETFOCUS( nOrd )

RETURN ( slNetOK )

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
PROCEDURE NetFlush()

   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   DBCOMMITALL()
   DBUNLOCKALL()
   DBSKIP( 0 )
RETURN

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
FUNCTION NetCommitAll()

   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   LOCAL i
   LOCAL n

   FOR n := 1 TO MAX_TABLE_AREAS
      IF !EMPTY( ALIAS( n ) )
         ( ALIAS( n ) )->( DBCOMMIT(), DBUNLOCK() )
      ENDIF
   NEXT

RETURN n

// ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
FUNCTION IsLocked( nRecId )

   // ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
RETURN ( ASCAN( DBRLOCKLIST(), { | n | n == nRecID } ) > 0 )

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
FUNCTION NetError()

   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
RETURN !slNetOK

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
FUNCTION SetNetDelay( nSecs )

   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   LOCAL nTemp := snNetDelay
   IF nSecs != NIL
      snNetDelay := nSecs
   ENDIF
RETURN ( nTemp )

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
FUNCTION SetNetMsgColor( cColor )

   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   LOCAL cTemp := scNetMsgColor
   IF cColor != NIL
      scNetmsgColor := cColor
   ENDIF
RETURN ( cTemp )


/****
*     Utility functions
*
*     TableNew()
*
*     getTable()
*/

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
FUNCTION TableNew( cDBF, cALIAS, cOrderBag, cDRIVER, ;
                      lNET, cPATH, lNEW, lREADONLY )
   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   LOCAL nPos
   LOCAL lAuto
   LOCAL oDB
   LOCAL o
   DEFAULT lNET TO .T.
   DEFAULT lNEW TO .T.
   DEFAULT lREADONLY TO .F.
   DEFAULT cDRIVER TO "DBFCDX"
   DEFAULT cPATH TO SET( _SET_DEFAULT )
   DEFAULT cAlias TO FixExt( cDbf )
   DEFAULT cOrderBag TO FixExt( cDbf )  //+".CDX"

   lAuto := SET( _SET_AUTOPEN, .F. )

   IF ( nPos := ASCAN( saTables, { | e | e[ 1 ] == UPPER( cALIAS ) } ) ) > 0

      oDB := saTables[ nPos, 2 ]

   ELSE
      o := Table():New( cDBF, cALIAS, cOrderBag, cDRIVER, ;
                      lNET, cPATH, lNEW, lREADONLY )
      IF o:Open()
         oDB := o:FldInit()
      ENDIF

      AADD( saTables, { UPPER( cAlias ), oDB } )

   ENDIF

   SET( _SET_AUTOPEN, lAuto )

RETURN oDB

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
FUNCTION GetTable( cAlias )

   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   LOCAL nPos
   LOCAL oDB
   IF ( nPos := ASCAN( saTables, { | e | e[ 1 ] == UPPER( cALIAS ) } ) ) > 0
      oDB := saTables[ nPos, 2 ]
   ENDIF
RETURN oDB

/****
*
*     CLASS oField()
*
*
*
*/

CLASS oField

   DATA Alias INIT ALIAS()
   DATA Name INIT ""
   DATA Type INIT "C"
   DATA Len INIT 0
   DATA Dec INIT 0
   DATA order INIT 0
   DATA Value

   METHOD GET() INLINE ::value := ( ::alias )->( FIELDGET( ::order ) )
   METHOD Put( x ) INLINE ::value := x ,;
          ( ::alias )->( FIELDPUT( ::order, x ) )

ENDCLASS

   /****
*
*     CLASS Record()
*
*
*
*/

CLASS RECORD

   DATA Buffer INIT {}
   DATA Alias INIT ALIAS()
   DATA Number INIT 0
   DATA aFields INIT {}

   METHOD New()
   METHOD GET()
   METHOD Put()

ENDCLASS

   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD NEW( cAlias ) CLASS RECORD

   LOCAL i
   LOCAL oFld
   LOCAL aStruc
   LOCAL aItem

   DEFAULT cAlias TO ALIAS()

   ::Alias   := cAlias
   ::Buffer  := {}
   ::aFields := ARRAY( ( ::alias )->( FCOUNT() ) )

   aStruc := ( ::alias )->( DBSTRUCT() )
#ifdef __XHARBOUR__

   FOR EACH aItem in ::aFields
      i          := HB_EnumIndex()
      oFld       := oField()
      oFld:order := i
      oFld:Name  := ( ::alias )->( FIELDNAME( i ) )
      oFld:Type  := aStruc[ i, 2 ]
      oFld:LEN   := aStruc[ i, 3 ]
      oFld:Dec   := aStruc[ i, 4 ]
      oFld:Alias := ::alias
      aItem      := oFld
   NEXT
#else
   FOR i := 1 TO LEN( ::aFields )
      oFld           := oField()
      oFld:order     := i
      oFld:Name      := ( ::alias )->( FIELDNAME( i ) )
      oFld:Type      := aStruc[ i, 2 ]
      oFld:LEN       := aStruc[ i, 3 ]
      oFld:Dec       := aStruc[ i, 4 ]
      oFld:Alias     := ::alias
      ::aFields[ i ] := oFld
   NEXT
#endif

RETURN Self

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD GET() CLASS RECORD

   LOCAL i
   FOR i := 1 TO LEN( ::aFields )
      ::aFields[ i ]:GET()
      ::buffer[ i ] := ::aFields[ i ] :value
   NEXT

RETURN Self

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD Put() CLASS RECORD

   LOCAL i
   FOR i := 1 TO LEN( ::aFields )
      IF ::aFields[ i ] :Value <> ::buffer[ i ]
         ::aFields[ i ]:PUT( ::buffer[ i ] )
         ::buffer[ i ] := ::aFields[ i ] :value
      ENDIF
   NEXT

RETURN Self

/****
*
*     CLASS Table()
*
*
*
*/

   //METHOD SetFocus()    INLINE (::Alias)->(Select( ::Area ))
   // ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   // ³  Info...
   // ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   //ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   //³ encapsulated methods
   //ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   // ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   // ³  Methods
   // ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   // ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   // ³ table movement
   // ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   // ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   // ³  RELATION
   // ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   // ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   // ³  ORDER Management
   // ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
CLASS Table

   DATA Buffer INIT {}                  // 1
   DATA Alias INIT ALIAS()              // 2
   DATA Area INIT 0 // 3

   DATA oRec
   DATA aStruc INIT {}
   DATA nRecno INIT 0
   DATA cDBF INIT ""
   DATA cOrderBag INIT ""
   DATA cOrderFile INIT ""
   DATA cPATH INIT ""
   DATA Driver INIT "DBFCDX"
   DATA IsNew INIT .T.
   DATA IsReadOnly INIT .F.
   DATA IsNet INIT .T.
   DATA aSaveState INIT {}
   DATA lMonitor INIT .F.
   DATA ReadBuffers INIT {}
   DATA WriteBuffers INIT {}
   DATA DeleteBuffers INIT {}
   DATA nDataOffset INIT 0
   DATA BlankBuffer INIT {}
   DATA aOrders INIT {}
   DATA aChildren INIT {}
   DATA oParent

   METHOD EOF() INLINE ( ::Alias )->( EOF() )
   METHOD BOF() INLINE ( ::Alias )->( BOF() )
   METHOD RECNO() INLINE ( ::Alias )->( RECNO() )
   METHOD LASTREC() INLINE ( ::Alias )->( LASTREC() )
   METHOD SKIP( n ) INLINE ( ::Alias )->( DBSKIP( n ) ),;
   ::nRecno := ( ::Alias )->( RECNO() )

   METHOD GOTO( n ) INLINE ( ::Alias )->( DBGOTO( n ) )
   METHOD goTop() INLINE ( ::Alias )->( DBGOTOP() )
   METHOD goBottom() INLINE ( ::Alias )->( DBGOBOTTOM() )
   METHOD SetFocus() INLINE ( ::Alias )->( SELECT( ::ALias ) )
   METHOD Append( l ) INLINE IF( ::isNet, ( ::Alias )->( NetAppend( l ) ), ;
   ( ::alias )->( DBAPPEND() ) )
   METHOD RECALL( l ) INLINE ( ::Alias )->( NetRecall( l ) )

   METHOD LOCATE( bFor, bWhile, nNext, nRec, lRest ) INLINE ;
   ( ::Alias )->( __dbLocate( bFor, bWhile, ;
   nNext, nRec, lRest ) )
   METHOD CONTINUE() INLINE ( ::Alias )->( __dbContinue() )
   METHOD FOUND() INLINE ( ::Alias )->( FOUND() )
   METHOD Kill() INLINE ( ::Alias )->( DBCOMMIT() ),;
          ( ::Alias )->( DBUNLOCK() ) ,;
          ( ::Alias )->( DBCLOSEAREA() ),;
          ::ClearBuffers()
   METHOD ClearBuffers() INLINE ::ReadBuffers := {},;
         ::WriteBuffers := {},;
         ::DeleteBuffers := {}

   METHOD dbIsShared() INLINE ( ::Alias )->( DBINFO( DBI_SHARED ) )

   METHOD dbIsFLocked( n ) INLINE ( ::Alias )->( DBINFO( DBI_ISFLOCK ) )

   METHOD dbLockCount() INLINE ( ::Alias )->( DBINFO( DBI_LOCKCOUNT ) )

   METHOD DBINFO( n, x ) INLINE ( ::Alias )->( DBINFO( n, x ) )

   METHOD dbGetAlias() INLINE ( ::Alias )->( DBINFO( DBI_ALIAS ) )

   METHOD dbFullPath() INLINE ( ::Alias )->( DBINFO( DBI_FULLPATH ) )

   METHOD IsRLocked( n ) INLINE ( ::Alias )->( DBRECORDINFO( DBRI_LOCKED, n ) )

   METHOD IsRUpdated( n ) INLINE ( ::Alias )->( DBRECORDINFO( DBRI_UPDATED, n ) )

   METHOD DBRECORDINFO( n, x ) INLINE ( ::Alias )->( DBRECORDINFO( n,, x ) )

   METHOD DBORDERINFO( n, x, u ) INLINE ( ::Alias )->( DBORDERINFO( n, ::cOrderFile, x, u ) )

   METHOD OrderCount() INLINE ;
   ( ::Alias )->( DBORDERINFO( DBOI_ORDERCOUNT, ::cOrderFile ) )

   METHOD AutoOpen( l ) INLINE ;
   ( ::Alias )->( DBORDERINFO( DBOI_AUTOOPEN, ::cOrderFile,, l ) )

   METHOD AutoShare( l ) INLINE ;
   ( ::Alias )->( DBORDERINFO( DBOI_AUTOSHARE, ::cOrderFile,, l ) )

   METHOD USED() INLINE SELECT( ::Alias ) > 0

   METHOD ORDSETFOCUS( ncTag ) INLINE ( ::Alias )->( ORDSETFOCUS( ncTag ) )
   METHOD ORDNAME( nOrder ) INLINE ;
   ( ::Alias )->( ORDNAME( nOrder, ::cOrderBag ) ) ;

   METHOD ORDNUMBER( cOrder ) INLINE ;
   ( ::Alias )->( ORDNUMBER( cOrder, ::cOrderBag ) ) ;

   METHOD ORDSCOPE( n, u ) INLINE ( ::Alias )->( ORDSCOPE( n, u ) )

   METHOD ORDISUNIQUE( nc ) INLINE ( ::Alias )->( ORDISUNIQUE( nc, ;
   ::cOrderBag ) ) ;

   METHOD ORDSKIPUNIQUE( n ) INLINE ( ::Alias )->( ORDSKIPUNIQUE( n ) )
   METHOD ORDSETRELATION( n, b, c ) INLINE ( ::Alias )->( ORDSETRELATION( n, b, c ) )

   METHOD SetTopScope( xScope ) INLINE ;
   ( ::alias )->( ORDSCOPE( TOPSCOPE, xScope ) )
   METHOD SetBottomScope( xScope ) INLINE ;
   ( ::alias )->( ORDSCOPE( BOTTOMSCOPE, xScope ) )
   METHOD KillScope() INLINE ( ::alias )->( ORDSCOPE( TOPSCOPE, NIL ) )  ,;
          ( ::alias )->( ORDSCOPE( BOTTOMSCOPE, NIL ) )

   METHOD New( cDBF, cALIAS, cOrderBag, cDRIVER, ;
   lNET, cPATH, lNEW, lREADONLY )

   METHOD OPEN()

   METHOD dbMove( n )
   METHOD FldInit()
   METHOD READ( l )
   METHOD ReadBLANK( l )
   METHOD Write( l )
   METHOD BufWrite( l )
   MESSAGE DELETE() METHOD __oTDelete() // reserved word - *HAS* to be renamed...
   METHOD SetMonitor( l )
   METHOD Undo( a, b, c )

   METHOD DBSKIP( n ) INLINE ( ::Alias )->( DBSKIP( n ) ),;
          ::nRecno := ( ::alias )->( RECNO() )

   METHOD DBGOTO( n ) INLINE ( ::Alias )->( DBGOTO( n ) )

   METHOD DBEVAL( a, b, c, d, e, f ) INLINE ( ::Alias )->( DBEVAL( a, b, c, d, e, f ) )
   METHOD DBSEEK( a, b, c ) INLINE ( ::Alias )->( DBSEEK( a, b, c ) )
   METHOD LOCATE( bFor, bWhile, ;
   nNext, nRec, ;
   lRest ) INLINE ;
   ( ::Alias )->( __dbLocate( bFor, bWhile, ;
   nNext, nRec, lRest ) ) ;

   METHOD CONTINUE() INLINE ( ::Alias )->( __dbContinue() )
   METHOD FOUND() INLINE ( ::Alias )->( FOUND() )

   METHOD DBFILTER() INLINE ( ::Alias )->( DBFILTER() )
   METHOD SetFilter( c ) INLINE ;
   IF( c != NIL, ( ::Alias )->( DBSETFILTER( COMPILE( c ), c ) ), ;
   ( ::Alias )->( DBCLEARFILTER() ) )

   METHOD AddChild( oChild, cKey )

   METHOD AddOrder( cTag, cKey, cLabel, ;
   cFor, cWhile, ;
   lUnique, ;
   bEval, nInterval, cOrderFile )
   METHOD GetOrderLabels()
   METHOD SetOrder( xTag )
   METHOD GetOrder( xOrder )
   METHOD FastReindex()
   METHOD REINDEX()
   METHOD CreateTable( cFile )
   METHOD AddField( f, t, l, d )
   METHOD Gentable()

ENDCLASS

   //---------------------
   //  Constructor...
   //---------------------

   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD New( cDBF, cALIAS, cOrderBag, cDRIVER, ;
               lNET, cPATH, lNEW, lREADONLY ) CLASS Table
   DEFAULT lNET TO .F.
   DEFAULT lNEW TO .T.
   DEFAULT lREADONLY TO .F.
   DEFAULT cDRIVER TO "DBFCDX"
   DEFAULT cPATH TO SET( _SET_DEFAULT )
   DEFAULT cAlias TO FixExt( cDbf )
   DEFAULT cOrderBag TO FixExt( cDbf )  //+".CDX"

   ::IsNew      := lNEW
   ::IsNet      := lNET
   ::IsReadOnly := lREADONLY
   ::cDBF       := cDBF
   ::cPath      := cPATH
   ::cOrderBag  := FixExt( cOrderBag )
   ::cOrderFile := ::cOrderBag + ORDBAGEXT()                //".CDX"

   ::Driver      := cDRIVER
   ::aOrders     := {}
   ::Area        := 0
   ::Alias       := cALIAS
   ::nDataOffset := LEN( self )         //66

RETURN self

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD OPEN() CLASS Table

   LOCAL lSuccess := .T.

   DBUSEAREA( ::IsNew, ::Driver, ::cDBF, ::Alias, ::IsNET, ::IsREADONLY )

   IF ::IsNET == .T.
      IF NETERR()
         ALERT( _NET_USE_FAIL_MSG )
         lSuccess := .F.
         RETURN ( lSuccess )
      ENDIF
   ENDIF

   SELECT( ::Alias )
   ::Area := SELECT()
   IF ::cOrderBag != NIL .and. FILE( ::cPath + ::cOrderFile )

      SET INDEX TO ( ::cPath + ::cOrderBag )
      ( ::Alias )->( ORDSETFOCUS( 1 ) )

   ENDIF

   ::Buffer := ARRAY( ( ::Alias )->( FCOUNT() ) )
   ::aStruc := ( ::Alias )->( DBSTRUCT() )

   ::dbMove( _DB_TOP )

RETURN ( lSuccess )

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD DBMove( nDirection ) CLASS Table

   LOCAL nRec := ( ::Alias )->( RECNO() )
   DEFAULT nDirection TO 0

   DO CASE
   CASE nDirection == 0
      ( ::Alias )->( DBSKIP( 0 ) )
   CASE nDirection == _DB_TOP
      ( ::Alias )->( DBGOTOP() )
   CASE nDirection == _DB_BOTTOM
      ( ::Alias )->( DBGOBOTTOM() )
   CASE nDirection == _DB_BOF
      ( ::Alias )->( DBGOTOP() ) 
      ( ::Alias )->( DBSKIP( - 1 ) )
   CASE nDirection == _DB_EOF
      ( ::Alias )->( DBGOBOTTOM() ) 
      ( ::Alias )->( DBSKIP( 1 ) )
   OTHERWISE
      ( ::Alias )->( DBGOTO( nDirection ) )
   ENDCASE

RETURN self

// -->
// -->
// --> Insert field definitions and generate virtual child class...
// -->
// -->

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD FldInit() CLASS Table

   LOCAL i
   LOCAL bBlock
   LOCAL cFldName
   LOCAL nCount    := ( ::Alias )->( FCOUNT() )
   LOCAL aDb
   LOCAL oDb
   LOCAL hNewClass
   LOCAL oNew
   LOCAL n
   LOCAL nScope    := 1
   LOCAL hNew

   ::nDataOffset := LEN( self ) - 1

   ::Buffer := ARRAY( ( ::Alias )->( FCOUNT() ) )
   IF EMPTY( ::Buffer )
      ::Read()
   ENDIF

   // --> create new oObject class from this one...

   adb := hbclass():new( ::alias, __CLS_PARAM( "table" ) )

   FOR i := 1 TO FCOUNT()
      adb:AddData( ( ::Alias )->( FIELDNAME( i ) ),,, nScope )
   NEXT

   aDB:create()

   oNew := adb:Instance()

   oNew:IsNew       := ::IsNew
   oNew:IsNet       := ::IsNet
   oNew:IsReadOnly  := ::IsReadOnly
   oNew:cDBF        := ::cDBF
   oNew:cPath       := ::cPath
   oNew:cOrderBag   := ::cOrderBag
   oNew:cOrderFile  := ::cOrderFile
   oNew:Driver      := ::Driver
   oNew:Area        := ::Area
   oNew:Alias       := ::Alias
   oNew:aStruc      := ::aStruc
   oNew:BlankBuffer := ::BlankBuffer
   oNew:aOrders     := ::aOrders
   oNew:oParent     := ::oParent
   oNew:Buffer      := ::buffer

   SELECT( oNew:Alias )
   oNew:Area := SELECT()

   oNew:Read()

   IF oNew:cOrderBag != NIL .and. FILE( oNew:cPath + oNew:cOrderFile )
      SET INDEX TO ( oNew:cPath + oNew:cOrderBag )
      ( oNew:Alias )->( ORDSETFOCUS( 1 ) )
   ENDIF

   oNew:buffer := ARRAY( ( oNew:alias )->( FCOUNT() ) )
   oNew:aStruc := ( oNew:alias )->( DBSTRUCT() )

   IF oNew:Used()
      oNew:dbMove( _DB_TOP ) 
      oNew:Read()
   ENDIF

RETURN oNew

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD READ( lKeepBuffer ) CLASS Table

   LOCAL i
   LOCAL nSel   := SELECT( ::Alias )
   LOCAL adata  := ARRAY( 1, 2 )
   LOCAL Buffer
   DEFAULT lKeepBuffer TO .F.

   //? len( ::Buffer )
#ifdef __XHARBOUR__
   FOR Each Buffer in ::Buffer
      
      i      := HB_EnumIndex()
      Buffer := ( ::Alias )->( FIELDGET( i ) )

      adata[ 1, 1 ] := ( ::Alias )->( FIELDNAME( i ) )
      adata[ 1, 2 ] := ( ::Alias )->( FIELDGET( i ) )
      __ObjSetValueList( Self, aData )

   NEXT

#else

   FOR i := 1 TO LEN( ::Buffer )
      ::Buffer[ i ] := ( ::Alias )->( FIELDGET( i ) )

      adata[ 1, 1 ] := ( ::Alias )->( FIELDNAME( i ) )
      adata[ 1, 2 ] := ( ::Alias )->( FIELDGET( i ) )
      __ObjSetValueList( Self, aData )

   NEXT
#endif
   IF ( lKeepBuffer == .T. ) .or. ( ::lMonitor == .T. )
      AADD( ::ReadBuffers, { ( ::Alias )->( RECNO() ), ::Buffer } )
   ENDIF

   SELECT( nSel )

RETURN ( Self )

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD ReadBlank( lKeepBuffer ) CLASS Table

   LOCAL i
   LOCAL nSel   := SELECT( ::Alias )
   LOCAL nRec   := ( ::Alias )->( RECNO() )
   LOCAL adata  := ARRAY( 1, 2 )
   LOCAL Buffer
   DEFAULT lKeepBuffer TO .F.

   ( ::Alias )->( DBGOBOTTOM() )
   ( ::Alias )->( DBSKIP( 1 ) )         // go EOF
#ifdef __XHARBOUR__
   FOR each Buffer in ::Buffer
      i      := HB_EnumIndex()
      Buffer := ( ::Alias )->( FIELDGET( i ) )

      adata[ 1, 1 ] := ( ::Alias )->( FIELDNAME( i ) )
      adata[ 1, 2 ] := ( ::Alias )->( FIELDGET( i ) )
      __ObjSetValueList( Self, aData )

   NEXT
#else
   FOR i := 1 TO LEN( ::Buffer )
      ::Buffer[ i ] := ( ::Alias )->( FIELDGET( i ) )

      adata[ 1, 1 ] := ( ::Alias )->( FIELDNAME( i ) )
      adata[ 1, 2 ] := ( ::Alias )->( FIELDGET( i ) )
      __ObjSetValueList( Self, aData )

   NEXT

#endif

   IF ( lKeepBuffer == .T. ) .or. ( ::lMonitor == .T. )
      AADD( ::ReadBuffers, { ( ::Alias )->( RECNO() ), ::Buffer } )
   ENDIF

   ( ::Alias )->( DBGOTO( nRec ) )
   SELECT( nSel )

RETURN ( Self )

// ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD Write( lKeepBuffer ) CLASS Table

   LOCAL i
   LOCAL aOldBuffer := ARRAY( ( ::Alias )->( FCOUNT() ) )
   LOCAL nSel       := SELECT( ::Alias )
   LOCAL nOrd       := ( ::Alias )->( ORDSETFOCUS() )
   LOCAL aData      := __objGetValueList( Self )
   LOCAL n
   DEFAULT lKeepBuffer TO .F.

   IF ( lKeepBuffer == .T. ) .or. ( ::lMonitor == .T. )

      // --> save old record in temp buffer
      FOR i := 1 TO ( ::Alias )->( FCOUNT() )
         aOldBuffer[ i ] := ( ::Alias )->( FIELDGET( i ) )
      NEXT

      AADD( ::WriteBuffers, { ( ::Alias )->( RECNO() ), aOldBuffer } )

   ENDIF

   IF ::isNet
      IF !( ::Alias )->( NetRecLock() )
         RETURN .F.
      ENDIF
   ENDIF

   ( ::Alias )->( ORDSETFOCUS( 0 ) )

   FOR i := 1 TO ( ::Alias )->( FCOUNT() )
      n := ASCAN( adata, { | a, b | a[ 1 ] == ( ::Alias )->( FIELDNAME( i ) ) } )
      ( ::Alias )->( FIELDPUT( i, adata[ n, 2 ] ) )
   NEXT

   ( ::Alias )->( DBSKIP( 0 ) )         // same as commit
   IF ::isNet
      ( ::Alias )->( DBRUNLOCK() )
   ENDIF
   ( ::Alias )->( ORDSETFOCUS( nOrd ) )
   SELECT( nSel )

RETURN ( .T. )

// ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD BUFWrite( aBuffer ) CLASS Table

   LOCAL i
   LOCAL aOldBuffer := ARRAY( ( ::Alias )->( FCOUNT() ) )
   LOCAL nSel       := SELECT( ::Alias )
   LOCAL nOrd       := ( ::Alias )->( ORDSETFOCUS() )
   LOCAL Buffer
   DEFAULT aBuffer TO ::Buffer

   IF ::isNet
      IF !( ::Alias )->( NetRecLock() )
         RETURN .F.
      ENDIF
   ENDIF

   ( ::Alias )->( ORDSETFOCUS( 0 ) )
#ifdef __XHARBOUR__
   FOR each Buffer in aBuffer
      i := HB_EnumIndex()
      ( ::Alias )->( FIELDPUT( i, Buffer ) )
   NEXT

#else

   FOR i := 1 TO LEN( aBuffer )
      ( ::Alias )->( FIELDPUT( i, aBuffer[ i ] ) )
   NEXT
#endif
   ( ::Alias )->( DBSKIP( 0 ) )
   IF ::isNet
      ( ::Alias )->( DBRUNLOCK() )
   ENDIF
   ( ::Alias )->( ORDSETFOCUS( nOrd ) )
   SELECT( nSel )

RETURN ( .T. )

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD __oTDelete( lKeepBuffer )        // ::Delete()

   LOCAL lRet
   LOCAL lDeleted := SET( _SET_DELETED, .F. )                  // make deleted records visible
   // temporarily...
   DEFAULT lKeepBuffer TO .F.

   ::Read()

   IF ::isNet
      lRet := IF( ( ::Alias )->( NetDelete() ), .T., .F. )
   ELSE
      ( ::alias )->( DBDELETE() ) ; lRet := .T.
   ENDIF

   IF ( ( lKeepBuffer == .T. ) .or. ( ::lMonitor == .T. ) ) .and. ;
          ( lRet == .T. )
      AADD( ::DeleteBuffers, { ( ::Alias )->( RECNO() ), ::Buffer } )
   ENDIF

   IF ::isNet
      ( ::Alias )->( DBUNLOCK() )
   ENDIF

   SET( _SET_DELETED, lDeleted )

RETURN ( lRet )

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD SetMonitor( lOnOff ) CLASS Table

   LOCAL lTemp := ::lMonitor
   ::lMonitor := !( ::lMonitor )
RETURN lTemp

//
//   Transaction control subsystem...
//

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD Undo( nBuffer, nLevel ) CLASS Table

   LOCAL i
   LOCAL nLen
   LOCAL lRet      := .F.
   LOCAL lDelState := SET( _SET_DELETED )
   LOCAL nRec      :=::RECNO()

   DEFAULT nBuffer TO _WRITE_BUFFER

   IF nLevel == NIL
      nLevel := 0
   ENDIF

   SWITCH nBuffer

   CASE _DELETE_BUFFER

      IF !EMPTY( ::DeleteBuffers )

         SET( _SET_DELETED, .F. )       // make deleted records visible temporarily...

         DEFAULT nLevel TO LEN( ::DeleteBuffers )

         nLen := LEN( ::deleteBuffers )

         IF nLevel == 0                 // DO ALL...
            FOR i := 1 TO LEN( ::deleteBuffers )

               ( ::Alias )->( DBGOTO( ::deleteBuffers[ i, 1 ] ) )

               IF ( ::Alias )->( NetRecall() )
                  lRet := .T.
               ELSE
                  lRet := .F.
               ENDIF

            NEXT

            IF lRet == .T.
               FOR i := 1 TO LEN( ::deleteBuffers )
                  ADEL( ::DeleteBuffers, i )
                  ASIZE( ::DeleteBuffers, LEN( ::DeleteBuffers ) - 1 )
               NEXT
            ENDIF

         ELSE       // DO CONTROLLED...

            FOR i := nLen TO ( nLen - nLevel ) + 1

               ( ::Alias )->( DBGOTO( ::deleteBuffers[ i, 1 ] ) )

               IF ( ::Alias )->( NetRecall() )
                  lRet := .T.
               ELSE
                  lRet := .F.
               ENDIF

            NEXT

            IF lRet == .T.
               FOR i := nLen TO ( nLen - nLevel ) + 1
                  ADEL( ::DeleteBuffers, i )
                  ASIZE( ::DeleteBuffers, LEN( ::DeleteBuffers ) - 1 )
               NEXT
            ENDIF

         ENDIF

         SET( _SET_DELETED, lDelState )

      ENDIF

   CASE _WRITE_BUFFER
      IF !EMPTY( ::WriteBuffers )

         DEFAULT nLevel TO LEN( ::WriteBuffers )
         nLen := LEN( ::WriteBuffers )

         IF nLevel == 0                 // Do All...

            FOR i := 1 TO LEN( ::writeBuffers )             //nLen

               ( ::Alias )->( DBGOTO( ::WriteBuffers[ i, 1 ] ) )

               IF ::BufWrite( ::WriteBuffers[ i, 2 ] )
                  lRet := .T.
               ELSE
                  ALERT( "Rollback Failed..." )
                  lRet := .F.
               ENDIF
            NEXT

            IF lRet == .t.

               // erase entries
               FOR i := 1 TO LEN( ::WriteBuffers )
                  ADEL( ::WriteBuffers, i )
                  ASIZE( ::WriteBuffers, LEN( ::WriteBuffers ) - 1 )
               NEXT

            ENDIF

         ELSE       // do controlled...

            FOR i := nLen TO ( nLen - nLevel ) + 1

               ( ::Alias )->( DBGOTO( ::WriteBuffers[ i, 1 ] ) )

               IF ::BufWrite( ::WriteBuffers[ i, 2 ] )
                  lRet := .T.
               ELSE
                  ALERT( "Rollback Failed..." )
                  lRet := .F.
               ENDIF
            NEXT

            // erase entries
            IF lRet == .t.

               FOR i := nLen TO ( nLen - nLevel ) + 1
                  ADEL( ::WriteBuffers, i )
                  ASIZE( ::WriteBuffers, LEN( ::WriteBuffers ) - 1 )
               NEXT

            ENDIF

         ENDIF

      ENDIF

   DEFAULT

   END

   ( ::Alias )->( DBUNLOCK() )
   ( ::Alias )->( DBGOTO( nRec ) )
   ::Read()

RETURN ( lRet )

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
//   ORDER MANAGEMENT
//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD AddOrder( cTag, cKey, cLabel, ;
                    cFor, cWhile, ;
                    lUnique, ;
                    bEval, nInterval, cOrderFile ) CLASS Table
   LOCAL oOrd
   DEFAULT cOrderFile TO ::cOrderBag

   oOrd := oOrder():New( cTag, cKey, cLabel, ;
                       cFor, cWhile, ;
                       lUnique, ;
                       bEval, nInterval )

   oOrd:oTable    := Self
   oOrd:cOrderBag := ::cOrderBag

   AADD( ::aOrders, oOrd )

RETURN oOrd

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD REINDEX() CLASS Table

   LOCAL i
   LOCAL lRet := .F.
   LOCAL nSel := SELECT( ::Alias )
   LOCAL nOrd := ( ::Alias )->( ORDSETFOCUS( 0 ) )
   LOCAL nRec := ( ::Alias )->( RECNO() )

   IF LEN( ::aOrders ) > 0

      IF ::USED()
         ::Kill()
      ENDIF

      ::Isnet := .F.

      IF FILE( ::cPath + ::cOrderFile )
         IF FERASE( ::cPath + ::cOrderFile ) != 0
            // --> ALERT(".CDX *NOT* Deleted !!!" )
         ENDIF
      ENDIF

      IF !::Open()
         lRet := .F.
         RETURN ( lRet )
      ENDIF

      AEVAL( ::aOrders, { | o | o:Create() } )

      ::Kill()
      ::IsNet := .T.

      IF !::Open()
         lRet := .F.
         RETURN ( lRet )
      ENDIF

   ENDIF

   lRet := .T.
   ( ::Alias )->( DBSETINDEX( ::cOrderBag ) ) 
   ( ::Alias )->( ORDSETFOCUS( nOrd ) )
   ( ::Alias )->( DBGOTOP() ) 
   ( ::Alias )->( DBUNLOCK() )
   SELECT( nSel )

RETURN ( lRet )

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD FastReindex() CLASS Table

   LOCAL i
   LOCAL lRet := .F.
   LOCAL nSel := SELECT( ::Alias )
   LOCAL nOrd := ( ::Alias )->( ORDSETFOCUS( 0 ) )
   LOCAL nRec := ( ::Alias )->( RECNO() )

   IF LEN( ::aOrders ) > 0

      ::Kill()

      ::Isnet := .F.
      IF FILE( ::cPath + ::cOrderFile )
         IF FERASE( ::cPath + ::cOrderFile ) != 0
            // --> ALERT(".CDX *NOT* Deleted !!!" )
         ENDIF
      ENDIF

      IF !::Open()
         lRet := .F.
         RETURN ( lRet )
      ENDIF

      ( ::Alias )->( ORDLISTREBUILD() )

      ::Kill()
      ::IsNet := .T.

      IF !::Open()
         lRet := .F.
         RETURN ( lRet )
      ENDIF

   ENDIF

   lRet := .T.
   ( ::Alias )->( DBSETINDEX( ::cOrderBag ) ) 
   ( ::Alias )->( ORDSETFOCUS( nOrd ) )
   ( ::Alias )->( DBGOTOP() ) 
   ( ::Alias )->( DBUNLOCK() )
   SELECT( nSel )

RETURN ( lRet )

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD GetOrder( xOrder ) CLASS Table

   LOCAL nPos  := 0
   LOCAL xType := VALTYPE( xOrder )

   IF xType == "C"
      nPos := ASCAN( ::aOrders, { | e | e:Tag == xOrder } )
   ELSEIF xType == "N" .and. xOrder > 0
      nPos := xOrder
   ELSE
      nPos := 0
   ENDIF

   IF nPos == 0
      nPos := 1
   ENDIF

RETURN ::aOrders[ nPos ]                // returns oOrder

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD SetOrder( xTag ) CLASS Table

   LOCAL xType   := VALTYPE( xTag )
   LOCAL nOldOrd := ( ::Alias )->( ORDSETFOCUS() )

   SWITCH xType
   CASE "C"                    // we have an Order-TAG
      ( ::Alias )->( ORDSETFOCUS( xTag ) )
      EXIT
   CASE "N"                    // we have an Order-Number
      IF xTag <= 0
         ( ::Alias )->( ORDSETFOCUS( 0 ) )
      ELSE
         ::Getorder( xTag ):SetFocus()
      ENDIF
      EXIT
   CASE "O"                    // we have an Order-Object
      xTag:SetFocus()
      EXIT
   DEFAULT
      ( ::Alias )->( ORDSETFOCUS( 0 ) )
   END
RETURN nOldOrd

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD GetOrderLabels() CLASS Table

   LOCAL aRet := {}
   IF !EMPTY( ::aOrders )
      AEVAL( ::aOrders, { | e | AADD( aRet, e:Label ) } )
   ENDIF
RETURN aRet

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
// Relation Methods
//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD AddChild( oChild, cKey ) CLASS Table                 // ::addChild()

   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   AADD( ::aChildren, { oChild, cKey } )
   oChild:oParent := Self
   ( ::Alias )->( ORDSETRELATION( oChild:Alias, COMPILE( cKey ), cKey ) )
RETURN Self

/****
*     FixExt( cFileName )
*     extract .CDX filename from .DBF filename
*/
//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
STATIC FUNCTION FixExt( cFileName )

   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   LOCAL nLeft := AT( ".", cFilename )
RETURN ( LEFT( cFileName, IF( nLeft == 0, ;
         LEN( cFilename ), ;
         nLeft - 1 ) ) )

METHOD CreateTable( cFile ) CLASS Table

   ::cDbf := cFile
   IF LEN( ::aStruc ) > 0
      ::aStruc  := {}
      ::aOrders := {}
   ENDIF
RETURN Self

METHOD AddField( f, t, l, d ) CLASS Table

   AADD( ::aStruc, { f, t, l, d } )
RETURN Self

METHOD Gentable() CLASS Table

   DBCREATE( ::cDbf, ::aStruc, ::Driver )
RETURN Self

CLASS oOrder

   DATA oTable
   DATA cOrderBag
   DATA Label, TAG
   DATA cKey, bKey
   DATA cFor, bFor
   DATA cWhile, bWhile
   DATA Unique INIT .F.
   DATA bEval
   DATA nInterval
   METHOD ALIAS() INLINE ::oTable:Alias

   METHOD New( cTag, cKey, cLabel, cFor, cWhile, lUnique, bEval, nInterval, cOrderBag )
   METHOD Create()

   METHOD SetFocus() INLINE ( ::alias )->( ORDSETFOCUS( ::Tag, ::cOrderBag ) )
   METHOD Destroy() INLINE ( ::alias )->( ORDDESTROY( ::Tag, ::cOrderBag ) )
   METHOD ORDDESTROY() INLINE ( ::alias )->( ORDDESTROY( ::Tag, ::cOrderBag ) )
   METHOD ORDBAGEXT() INLINE ( ::alias )->( ORDBAGEXT() )
   METHOD ORDKEYCOUNT() INLINE ( ::alias )->( ORDKEYCOUNT( ::Tag, ::cOrderBag ) )
   METHOD ORDFOR() INLINE ( ::alias )->( ORDFOR( ::Tag, ::cOrderBag ) )
   METHOD ORDISUNIQUE() INLINE ( ::alias )->( ORDISUNIQUE( ::Tag, ::cOrderBag ) )
   METHOD ORDKEY() INLINE ( ::alias )->( ORDKEY( ::Tag, ::cOrderBag ) )
   METHOD ORDKEYCOUNT() INLINE ( ::alias )->( ORDKEYCOUNT( ::Tag, ::cOrderBag ) )
   METHOD ORDKEYNO() INLINE ( ::alias )->( ORDKEYNO( ::Tag, ::cOrderBag ) )
   METHOD ORDKEYVAL() INLINE ( ::alias )->( ORDKEYVAL( ::Tag, ::cOrderBag ) )

ENDCLASS

   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD New( cTag, cKey, cLabel, cFor, cWhile, lUnique, bEval, nInterval, cOrderBag ) CLASS oOrder

   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
   DEFAULT cKey TO ".T."
   DEFAULT lUnique TO .F.
   DEFAULT cFor TO ".T."
   DEFAULT cWhile TO ".T."
   DEFAULT bEval TO { || .T. }
   DEFAULT nInterval TO 1
   DEFAULT cLabel TO cTag
   ::cOrderBag := cOrderBag
   ::Tag       := cTag
   ::cKey      := cKey
   ::cFor      := cFor
   ::cWhile    := cWhile
   ::bKey      := COMPILE( cKey )
   ::bFor      := COMPILE( cFor )
   ::bWhile    := COMPILE( cWhile )
   ::bEval     := bEval
   ::nInterval := nInterval
   ::Label     := cLabel
RETURN self

//ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
METHOD Create() CLASS oOrder

   //ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

   DEFAULT ::cOrderBag TO ::oTable:cOrderBag
   //? "<<<",::alias, ::cOrderBag
   ( ::alias )->( ORDCONDSET( ::cFor, ::bFor, ;
     .T., ;
     ::bWhile, ;
     ::bEval, ::nInterval ) )

   ( ::alias )->( ORDCREATE( ::cOrderBag, ::Tag, ::cKey, ;
     ::bKey, ::Unique ) )
RETURN self

