/*
 * $Id: traceprg.prg,v 1.11 2003/11/10 00:59:32 fsgiudice Exp $
 */

/*
 * xHarbour Project source code:
 * PRG Tracing System
 *
 * Copyright 2001 Ron Pinkas <ron@@ronpinkas.com>
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
 * As a special exception, xHarbour license gives permission for
 * additional uses of the text contained in its release of xHarbour.
 *
 * The exception is that, if you link the xHarbour libraries with other
 * files to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the xHarbour library code into it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 *
 * This exception applies only to the code released with this xHarbour
 * explicit exception.  If you add/copy code from other sources,
 * as the General Public License permits, the above exception does
 * not apply to the code that you add in this way.  To avoid misleading
 * anyone as to the status of such modified files, you must delete
 * this exception notice from them.
 *
 * If you write modifications of your own for xHarbour, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice.
 *
 */

#include "set.ch"

#DEFINE  CRLF HB_OsNewLine()
#xtranslate Write( <cString> ) => FWrite( FileHandle, <cString> ) //;HB_OutDebug( <cString> )

//--------------------------------------------------------------//
FUNCTION TraceLog( ... )

   LOCAL cFile := SET( _SET_TRACEFILE ), FileHandle, nLevel := SET( _SET_TRACESTACK ), ProcName, xParam

   IF ! SET( _SET_TRACE )
      RETURN .T.
   ENDIF

   IF File( cFile )
      FileHandle := FOpen( cFile, 1 )
   ELSE
      FileHandle := FCreate( cFile )
   ENDIF

   FSeek( FileHandle, 0, 2 )

   IF nLevel > 0
      Write( '[' + ProcFile(1) + "->" + ProcName( 1 ) + '] (' + LTrim( Str( Procline(1) ) ) + ')' )
   ENDIF

   IF nLevel > 1 .AND. ! ( ProcName( 2 ) == '' )
      Write( ' Called from: '  + CRLF )
      nLevel := 1
      DO WHILE ! ( ( ProcName := ProcName( ++nLevel ) ) == '' )
         Write( space(30) + ProcFile( nLevel ) + "->" + ProcName + '(' + LTrim( Str( Procline( nLevel ) ) ) + ')' + CRLF )
      ENDDO
   ELSE
      Write( CRLF )
   ENDIF

   FOR EACH xParam IN HB_aParams()
      Write( 'Type: ' + ValType( xParam ) + ' >>>' + CStr( xParam ) + '<<<' + CRLF )
   NEXT

   Write( CRLF )

   FClose(FileHandle)

RETURN .T.
//--------------------------------------------------------------//


