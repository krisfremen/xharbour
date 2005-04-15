/*
 * $Id: dbdelim.prg,v 1.14 2005/04/14 06:14:39 ronpinkas Exp $
 */

/*
 * Harbour Project source code:
 * Copies the contents of a database to a delimited text file.
 * Appends the contents of a delimited text file to a database.
 *
 * Copyright 2001-2002 David G. Holm <dholm@jsd-llc.com>
 * www - http://www.harbour-project.org
 * APPEND FROM code submitted by Marco Braida <marcobra@elart.it>
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

#include "hbcommon.ch"
#include "fileio.ch"
#include "error.ch"

HB_FILE_VER( "$Id: dbdelim.prg,v 1.14 2005/04/14 06:14:39 ronpinkas Exp $" )

PROCEDURE __dbDelim( lExport, cFileName, cDelimArg, aFields, bFor, bWhile, nNext, nRecord, lRest )

   local index, handle, nStart, nCount
   local cSeparator := ",", cDelim := CHR( 34 )
   local aTextContent

   // Process the delimiter argument.
   IF !EMPTY( cDelimArg )
      IF UPPER( cDelimArg ) == "BLANK"
         cDelim := ""
         cSeparator := " "
      ELSE
         cDelim := LEFT( cDelimArg, 1 )
      ENDIF
   ENDIF

   // Process the file name argument.
   index := RAT( ".", cFileName )
   IF index > 0
      // The file name might include a file extension.
      IF RAT( "/", cFileName ) > index ;
      .OR. RAT( "\", cFileName ) > index
         // No, the file extension is in a directory name.
         index := 0
      ENDIF
   ENDIF
   IF index <= 0
      // No file name extension, so provide the default.
      cFileName += ".txt"
   ENDIF

   // Determine where to start and how many records to process.
   IF nRecord != NIL
      // The RECORD clause has the highest priority.
      nStart := nRecord
      nCount := 1
   ELSEIF nNext != NIL
      // The NEXT clause has the next highest priority.
      nStart := -1
      nCount := nNext
   ELSEIF bWhile != NIL .OR. (lRest != NIL .and. lRest)
      // The WHILE and REST clauses have equal priority.
      nStart := -1
      nCount := -1
   ELSE
      // Followed by the FOR clause or the ALL clause.
      nStart := 0
      nCount := -1
   ENDIF

   IF lExport
      // COPY TO DELIMITED
      handle := FCREATE( cFileName )
      IF handle == F_ERROR
         Eval( ErrorBlock(), __dbDelimErr( EG_CREATE, 1002, cFileName ) )
      ELSE
         IF nStart > -1
            // Only reposition if a starting record was specified or implied.
            IF nStart == 0
               GO TOP
            ELSE
               GO (nStart)
            ENDIF
         ENDIF

         DBF2TEXT( bWhile, bFor, aFields, cDelim, handle, cSeparator, nCount )

         FClose( handle )
      ENDIF
   ELSE
      // APPEND FROM DELIMITED
      handle := FOPEN( cFileName )
      IF handle == F_ERROR
         Eval( ErrorBlock(), __dbDelimErr( EG_OPEN, 1001, cFileName ) )
      ELSE

         FClose( handle )
         AppendToDb( cFileName, cSeparator )

      endif
   endif
RETURN

STATIC FUNCTION __dbDelimErr( genCode, subCode, cFileName )

   local oErr := ErrorNew()

   oErr:severity   := ES_ERROR
   oErr:genCode    := genCode
   oErr:subSystem  := "DELIM"
   oErr:subCode    := subCode
   oErr:description:= HB_LANGERRMSG( oErr:genCode )
   oErr:canRetry   := .T.
   oErr:canDefault := .T.
   oErr:fileName   := cFileName
   oErr:osCode     := FERROR()

RETURN oErr

PROCEDURE AppendToDb( cFile, cDelimiter )

   LOCAL hFile := FOpen( cFile, FO_READ )
   LOCAL sLine, aEol := { Chr(13) + Chr(10), Chr(10) }
   LOCAL aValues, nFields, cValue

   WHILE HB_FReadLine( hFile, @sLine, aEol ) == 0
      aValues := HB_aTokens( sLine, cDelimiter, .T., .T. )
      nFields := Min( Len( aValues ), FCount() )

      IF nFields > 0
         APPEND BLANK
      ELSE
         LOOP
      ENDIF

      FOR EACH cValue IN aValues
         IF cValue[1] == '"'
            cValue := SubStr( cValue, 2, Len( cValue ) - 2 )
         ENDIF

         SWITCH ValType( FieldGet( HB_EnumIndex() ) )
            CASE 'D'
               FieldPut( HB_EnumIndex(), STOD( cValue ) )
               EXIT

            CASE 'N'
               FieldPut( HB_EnumIndex(), Val( cValue ) )
               EXIT

            CASE 'L'
               FieldPut( HB_EnumIndex(), Upper( cValue[1] ) == 'T' )
               EXIT

            CASE 'C'
               FieldPut( HB_EnumIndex(), cValue )
               EXIT
         END
      NEXT
   END

RETURN
