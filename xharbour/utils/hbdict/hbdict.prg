*****************************************************
* HB I18N dictionary editor
*
* $Id$
*
* Usage: hbdict <infile> <outfile>
*
* The outfile must be a HIT table file, while the input
* can be an HIL or a HIT.
* Input and output can be the same; in this case, the
* target dictionary will be overwritten on program
* exit.
*
* NOTICE: this program is currently a WIP and is
* being developed for testing purposes. This notice
* will be removed when the program will begin to assume
* more functionality.
*
#include "inkey.ch"

PROCEDURE Main( cInput, cOutput )
   LOCAL aInput
   LOCAL recpos
   LOCAL nKey

   SET COLOR TO W+/B
   CLEAR SCREEN

   @0,18 SAY i18n( "X H A R B O U R - Dictionary Editor (preview)" )

   DisplayMask()

   IF cInput == NIL .or. cOutput == NIL
      Popup( i18n( "Incorrect format" ) )
      @12,12 say i18n( "Usage:" )  + " hbdict <input> <output>"
      DoQuit()
   ENDIF

   aInput := HB_I18nLoadTable( cInput )

   IF aInput == NIL
      Popup( i18n( "Error in loading file" ) )
      @12,12 SAY i18n( "Can't load file " ) + cInput
      DoQuit()
   ENDIF

   // Sorting records
   IF SubStr( aInput[1][1], 2 ) == "HIL"
      ASort( aInput[2],,,{| x, y | UPPER(x[1]) < UPPER(y[1])} )
   ENDIF

   // Informing user
   ShowHeader( aInput[1] )

   @6,3 SAY i18n("Saving to: ") + cOutput
   @3,40 SAY i18n("Press 'S' to save")
   @4,40 SAY i18n("Press 'E' to edit header")
   recpos := 1

   DO WHILE .T.
      DisplayMask()
      DrawPos( recpos, aInput[2] )
      ShowRecord( recpos, aInput[2] )
      nKey := Inkey(0)
      IF nKey == K_ESC
         EXIT
      ENDIF

      SWITCH nKey

         CASE K_LEFT
         CASE K_DOWN
            IF recpos > 1
               recpos--
            ENDIF
         EXIT

         CASE K_RIGHT
         CASE K_UP
            IF recpos < Len ( aInput[2] )
               recpos++
            ENDIF
         EXIT

         CASE K_ENTER
            EditEntry( recpos, aInput[2] )
         EXIT

         DEFAULT
            IF Upper( Chr( nKey ) ) == 'S'
               SaveTable( cOutput, aInput )
            ELSEIF Upper( Chr( nKey ) ) == 'E'
               ReadHeader( aInput[1] )
            ENDIF

      END

   ENDDO

RETURN

PROCEDURE DoQuit()
   @24, 20 say i18n( "Program terminated. Press any key" )
   Inkey( 0 )
   CLEAR SCREEN
   QUIT
RETURN


PROCEDURE DisplayMask()
   MakeBox( 8,5, 13,75 )
   MakeBox( 15,5, 21,75 )
   @8,8 SAY " " + i18n("International string") + " "
   @15,8 SAY " " + i18n("Local language string") + " "
   @22,12 SAY i18n("Press ENTER to edit entry and then CTRL+W to save it")
RETURN


PROCEDURE MakeBox( nRow, nCol, nRowTo, nColTo )
   @nRow, nCol, nRowTo, nColTo ;
        BOX( Chr( 201 ) + Chr( 205 ) + Chr( 187 ) + Chr( 186 ) +;
        Chr( 188 ) + Chr( 205 ) + Chr( 200 ) + Chr( 186 ) + Space( 1 ) )
RETURN

PROCEDURE PopUp( cMessage )
   SET COLOR TO W+/N
   @11,11 clear to 16, 71
   SET COLOR TO W+/b
   @10,10 clear to 15, 70
   MakeBox( 10,10, 15, 70 )
   @10,20 SAY " " +cMessage + " "

RETURN

PROCEDURE DrawPos( recpos, aTable )
   @7,60 SAY AllTrim( Str( recpos ) ) +" "+ I18n("of") + " " +  ;
      AllTrim( Str( Len(aTable ) ) )+"     "
RETURN

PROCEDURE ShowRecord( nPos, aTable )
   LOCAL cInter := aTable[nPos][1]
   LOCAL cLocal := aTable[nPos][2]

   IF cInter != NIL
      @9,7 SAY cInter
   ELSE
      @9,7 SAY i18n( "<Nothing>" )
   ENDIF

   IF cLocal != NIL
      @16,7 SAY cLocal
   ELSE
      @16,7 SAY i18n( "<Not Yet translated>" )
   ENDIF

RETURN


PROCEDURE EditEntry( nPos, aTable )
   Local cInput := aTable[nPos][2]
   IF cInput == NIL
      cInput := ""
   ENDIF

   cInput := MemoEdit( cInput, 16, 7, 20,73 )

   IF Len( cInput ) > 0
      aTable[nPos][2] := cInput
   ENDIF

RETURN

PROCEDURE SaveTable( cFileName, aTable )
   LOCAL aHeader

   Popup( i18n( "Saving file" ) )
   @12,12 SAY i18n( "Saving file to " ) + cFileName

   // Saving the new header
   aHeader := Aclone( aTable[1] )
   // I can only save HIT types
   aHeader[1] := Chr( 3 ) + "HIT"
   aHeader[6] := Len( aTable[2] )

   IF HB_I18nSaveTable( cFileName, aHeader, aTable[2] )
      Popup( i18n( "Success" ) )
      @12,12 SAY i18n( "The file has been saved" )
   ELSE
      Popup( i18n( "Failure" ) )
      @12,12 SAY i18n( "The File has not been saved" )
   ENDIF

   @13,13 SAY i18n( "(Press any key)" )
   Inkey(0)
   @10,10 clear to 16, 71
RETURN

PROCEDURE ReadHeader( aHeader )
   LOCAL GetList := {}

   @3,3 SAY i18n("Author:    ") GET aHeader[2]
   @4,3 SAY i18n("Language:  ") GET aHeader[3]
   @5,3 SAY i18n("Code:      ") GET aHeader[5]
   READ

   ShowHeader( aHeader )
RETURN

PROCEDURE ShowHeader( aHeader )
   * Due to a flaw In GET, I have to add a space here...
   @2,3 SAY i18n("File Type: ") +" "+ SubStr( aHeader[1], 2 )
   @3,3 SAY i18n("Author:    ") +" "+ aHeader[2]
   @4,3 SAY i18n("Language:  ") +" "+ aHeader[3]
   @5,3 SAY i18n("Code:      ") +" "+ aHeader[5]
RETURN