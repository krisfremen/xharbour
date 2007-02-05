/*
 * $Id: encoder.prg,v 1.1 2004/08/05 12:21:16 lf_sfnet Exp $
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
                                                                      /*
 Internet Messaging: http://www.ietf.org/rfc/rfc2045.txt

 */

#include "hbclass.ch"
#include "fileio.ch"
#include "tip.ch"

CLASS TIPEncoder
   DATA cName

   METHOD New( cModel )
   METHOD Encode( cData )
   METHOD Decode( cData )
ENDCLASS


METHOD New( cModel ) class TIPEncoder
   cModel := Lower( cModel )
   IF cModel == "base64"
         RETURN TIPEncoderBase64():New()

   ELSEIF cModel == "quoted-printable"
      RETURN TIPEncoderQP():New()

   ELSEIF cModel == "url" .or. cModel == "urlencoded"
      RETURN TIPEncoderURL():New()

   ELSEIF  cModel == "7bit" .or. cModel == "8bit"
      ::cName := cModel
      RETURN Self

   ELSEIF cModel == "text" .or. cModel == "plain";
             .or. cModel == "text/plain" .or. cModel == "as-is";
             .or. cModel == "7-bit" .or. cModel == "8-bit"
      ::cName := "as-is"
      RETURN Self
   ENDIF

RETURN NIL

METHOD Encode( cData ) class TIPEncoder
RETURN cData

METHOD Decode( cData ) class TIPEncoder
RETURN cData

