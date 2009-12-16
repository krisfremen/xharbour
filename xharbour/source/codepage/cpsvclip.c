/*
 * $Id: cpsvclip.c,v 1.5 2009/05/07 09:29:35 likewolf Exp $
 */

/*
 * Harbour Project source code:
 * National Collation Support Module (SVCLIP - Clipper compatible)
 *
 * Copyright 2006 Klas Engwall <klas dot engwall at engwall dot com>
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

/* Language name: Swedish */
/* ISO language code (2 chars): SV */
/* Codepage: 437 */

#include "hbapi.h"
#include "hbapicdp.h"

#define NUMBER_OF_CHARACTERS  31    /* The number of single characters in the
                                       alphabet, two-as-one aren't considered
                                       here, accented - are considered. */
#define IS_LATIN               1    /* Should be 1, if the national alphabet
                                       is based on Latin */
#define ACCENTED_EQUAL         0    /* Should be 1, if accented character
                                       has the same weight as appropriate
                                       unaccented. */
#define ACCENTED_INTERLEAVED   0    /* Should be 1, if accented characters
                                       sort after their unaccented counterparts
                                       only if the unaccented versions of all
                                       characters being compared are the same
                                       ( interleaving ) */

/* If ACCENTED_EQUAL or ACCENTED_INTERLEAVED is 1, you need to mark the
   accented characters with the symbol '~' before each of them, for example:
      a~�
   If there is two-character sequence, which is considered as one, it should
   be marked with '.' before and after it, for example:
      ... h.ch.i ...

   The Upper case string and the Lower case string should be absolutely the
   same excepting the characters case, of course.
 */

/* NOTE: The collation sequence below is almost compatible with Clipper's
   NTXSWE.OBJ which does NOT provide a correct Swedish collation. The most
   notable error in NTXSWE.OBJ is the uppercase E with an acute accent
   versus the lowercase e with a grave accent. All other accented characters
   are ignored and show up according to their ASCII values. Another oddity
   in NTXSWE.OBJ is that the Danish "AE" and "ae" compound characters
   (ASCII values 146 and 145 respectively) turn up between ASCII 135 and
   136, a rather pointless relocation IMHO. I found no way to replicate
   that behaviour in the character strings below, so if you allow chr(146)
   and chr(145) to be saved in indexed fields there WILL be index corruption
   if data is shared between Clipper and xHarbour. Upper()/Lower() converson
   of those characters as well as all accented characters must be done
   programatically just like in Clipper.

   For sharing data with Clipper, assuming that the chr(146) and chr(145)
   problem is properly taken care of in your code, this codepage version
   must be used. For correct collation according to the book "Svenska
   skrivregler" (Swedish Writing Rules) by Svenska Spr�kn�mnden (the Swedish
   Language Council), use the SV850 version instead. That will of course
   not be Clipper compatible.
 */

static HB_CODEPAGE s_codepage = { "SVCLIP",
    HB_CPID_437,HB_UNITB_437,NUMBER_OF_CHARACTERS,
    "ABCDE�FGHIJKLMNOPQRSTUVWXY�Z���",
    "abcde�fghijklmnopqrstuvwxy�z���",
    IS_LATIN, ACCENTED_EQUAL, ACCENTED_INTERLEAVED, 0, 0, NULL, NULL, NULL, NULL, 0, NULL };

#define HB_CP_ID SVCLIP
#include "hbcdpreg.h"
