/*
 * $Id: popcln.prg,v 1.1 2004/08/05 12:21:16 lf_sfnet Exp $
 */

/*
 * xHarbour Project source code:
 * TIP Class oriented Internet protocol library
 *
 * Copyright 2003 Giancarlo Niccolai <gian@niccolai.ws>
 *
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

#include "hbclass.ch"

/**
* Inet service manager: pop3
*/

CLASS tIPClientPOP FROM tIPClient

   METHOD New()
   METHOD Open()
   METHOD Close()
   METHOD Read( iLen )
   METHOD Stat()
   METHOD List()
   METHOD Retreive( nMsgId )
   METHOD Delete()
   METHOD Quit()
   METHOD Noop()                 // Can be called repeatedly to keep-alive the connection
   METHOD Top( nMsgId )          // Get Headers of mail (no body) to be able to quickly handle a message
   METHOD UIDL( nMsgId )         // Returns Unique ID of message n or list of unique IDs of all message inside maildrop

   METHOD GetOK()

ENDCLASS


METHOD New() CLASS tIPClientPOP
   ::nDefaultPort := 110
   ::nConnTimeout := 10000
RETURN Self


METHOD Open() CLASS tIPClientPOP
   IF .not. ::super:Open()
      RETURN .F.
   ENDIF

   IF Empty ( ::oUrl:cUserid ) .or. Empty ( ::oUrl:cPassword )
      RETURN .F.
   ENDIF

   InetSetTimeout( ::SocketCon, ::nConnTimeout )
   IF ::GetOk()
      InetSendAll( ::SocketCon, "USER " + ::oUrl:cUserid + ::cCRLF )
      IF ::GetOK()
         InetSendAll( ::SocketCon, "PASS " + ::oUrl:cPassword + ::cCRLF )
         IF ::GetOK()
            RETURN .T.
         ENDIF
      ENDIF
   ENDIF
RETURN .F.


METHOD GetOk() CLASS tIPClientPOP
   LOCAL nLen

   ::cReply := InetRecvLine( ::SocketCon, @nLen, 128 )
   IF InetErrorCode( ::SocketCon ) != 0 .or. ::cReply[1] != '+'
      RETURN .F.
   ENDIF
RETURN .T.


METHOD Noop() CLASS tIPClientPOP
   InetSendAll( ::SocketCon, "NOOP" + ::cCRLF )
RETURN ::GetOk()


METHOD Close() CLASS tIPClientPOP
   InetSetTimeOut( ::SocketCon, ::nConnTimeout )
   ::Quit()
RETURN ::super:Close()


METHOD Quit() CLASS tIPClientPOP
   InetSendAll( ::SocketCon, "QUIT" + ::cCRLF )
RETURN ::GetOk()


METHOD Stat() CLASS tIPClientPOP
   LOCAL nRead
   InetSendAll( ::SocketCon, "STAT" + ::cCRLF )
RETURN InetRecvLine( ::SocketCon, @nRead, 128)


METHOD Read( nLen ) CLASS tIPClientPOP
   /** Set what to read for */
   IF Empty( ::oUrl:cFile )
      RETURN ::List()
   ENDIF

   IF Val (::oUrl:cFile ) < 0
      IF ::Delete( -  Val (::oUrl:cFile ) )
         RETURN ::Quit()
      ELSE
         RETURN .F.
      ENDIF
   ENDIF

RETURN ::Retreive( Val (::oUrl:cFile ), nLen )



METHOD Top( nMsgId ) CLASS tIPClientPOP
   LOCAL nPos
   LOCAL cStr, cRet

   InetSendAll( ::SocketCon, "TOP " + Str( nMsgId ) + " 0 " + ::cCRLF )
   IF .not. ::GetOk()
      RETURN NIL
   ENDIF

   cRet := ""
   DO WHILE cStr != "." .and. InetErrorCode( ::SocketCon ) == 0
      cStr := InetRecvLine( ::SocketCon, @nPos, 256 )
      IF cStr != "."
         cRet += cStr + ::cCRLF
      ELSE
         ::bEof := .T.
      ENDIF

   ENDDO

   IF InetErrorCode( ::SocketCon ) != 0
      RETURN NIL
   ENDIF

RETURN cRet



METHOD List() CLASS tIPClientPOP
   LOCAL nPos
   LOCAL cStr, cRet

   InetSendAll( ::SocketCon, "LIST" + ::cCRLF )
   IF .not. ::GetOk()
      RETURN NIL
   ENDIF

   cRet := ""
   DO WHILE cStr != "." .and. InetErrorCode( ::SocketCon ) == 0
      cStr := InetRecvLine( ::SocketCon, @nPos, 256 )
      IF cStr != "."
         cRet += cStr + ::cCRLF
      ELSE
         ::bEof := .T.
      ENDIF

   ENDDO

   IF InetErrorCode( ::SocketCon ) != 0
      RETURN NIL
   ENDIF

RETURN cRet



METHOD UIDL( nMsgId ) CLASS tIPClientPOP

   LOCAL nPos
   LOCAL cStr, cRet

   IF ! Empty( nMsgId )
      InetSendAll( ::SocketCon, "UIDL " + Str( nMsgId ) + ::cCRLF )
   ELSE
      InetSendAll( ::SocketCon, "UIDL" + ::cCRLF )
   ENDIF

   IF .not. ::GetOk()
      RETURN NIL
   ENDIF

   IF ! Empty( nMsgId )

      // +OK Space(1) nMsg Space(1) UID
      RETURN SubStr(::cReply, Rat(Space(1), ::cReply) + 1)

   ELSE

      cRet := ""
      DO WHILE cStr != "." .and. InetErrorCode( ::SocketCon ) == 0
         cStr := InetRecvLine( ::SocketCon, @nPos, 256 )
         IF cStr != "."
            cRet += cStr + ::cCRLF
         ELSE
            ::bEof := .T.
         ENDIF

      ENDDO

   ENDIF

   IF InetErrorCode( ::SocketCon ) != 0
      RETURN NIL
   ENDIF

RETURN cRet


METHOD Retreive( nId, nLen ) CLASS tIPClientPOP
   LOCAL nPos
   LOCAL cStr, cRet, nRetLen, cBuffer, nRead

   IF .not. ::bInitialized
      InetSendAll( ::SocketCon, "RETR "+ Str( nId ) + ::cCRLF )
      IF .not. ::GetOk()
         ::bEof := .T.
         RETURN NIL
      ENDIF
      ::bInitialized := .T.
   ENDIF

   /* old code, one char at a time, slow
   cRet := ""
   DO WHILE InetErrorCode( ::SocketCon ) == 0
      cStr := InetRecvLine( ::SocketCon, @nPos, 1024 )
      IF cStr != NIL
         IF cStr == "."
            ::bEof := .T.
            EXIT
         ELSE
            cRet += cStr + ::cCRLF
            IF .not. Empty( nLen ) .and. nLen < Len( cRet )
               EXIT
            ENDIF
         ENDIF
      ENDIF
   ENDDO*/

   cRet := ""
   nRetLen := 1
   nRead := 0

   /* 04/05/2004 - <maurilio.longo@libero.it>
      Instead of receiving a single char at a time until after we have the full mail, let's receive as
      much as we can and stop when we reach EOM (end of mail :)) sequence. This way is _a lot_ faster
   */
   DO WHILE InetErrorCode( ::SocketCon ) == 0 .AND. ! ::bEof

      cBuffer := Space(1024)

      nRead := InetRecv( ::SocketCon, @cBuffer, 1024 )

      cRet += Left(cBuffer, nRead)

      IF At(::cCRLF + "." + ::cCRLF, cRet, nRetLen) <> 0
         // Remove ".CRLF"
         cRet := Left(cRet, Len(cRet) - 3)
         ::bEof := .T.

      ELSEIF ! Empty( nLen ) .AND. nLen < Len( cRet )
         EXIT

      ELSE
         nRetLen += nRead

      ENDIF

   ENDDO

   IF InetErrorCode( ::SocketCon ) != 0
      RETURN NIL
   ENDIF

   // Remove byte-stuffed termination octet(s) if any
   cRet := StrTran(cRet, ::cCRLF + "..", ::cCRLF + ".")

RETURN cRet


METHOD Delete( nId ) CLASS tIPClientPOP
   InetSendAll( ::SocketCon, "DELE " + AllTrim( Str( nId ) ) +  ::cCRLF )
RETURN ::GetOk()

