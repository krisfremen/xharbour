/*
 * $Id: hbapicdp.h,v 1.8 2004/01/28 22:02:37 druzus Exp $
 */

/*
 * Harbour Project source code:
 * Header file for the CodePages API
 *
 * Copyright 2002 Alexander S.Kresin <alex@belacy.belgorod.su>
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

#ifndef HB_CDP_SUPPORT_OFF

#ifndef HB_APICDP_H_
#define HB_APICDP_H_

#include <ctype.h>
#include "hbapi.h"
#include "hbinit.h"

HB_EXTERN_BEGIN

/* This hack is needed to force preprocessing if id is also a macro */
#define HB_CODEPAGE_REQUEST( id )      HB_CODEPAGE_REQUEST_( id )
#define HB_CODEPAGE_REQUEST_( id )     extern HB_FUNC( HB_CODEPAGE_##id ); \
                                       void hb_codepage_ForceLink_##id( void ) \
                                       { \
                                          HB_FUNCNAME( HB_CODEPAGE_##id )(); \
                                       }
#define HB_CODEPAGE_ANNOUNCE( id )     HB_FUNC( HB_CODEPAGE_##id ) {}

typedef struct _HB_UNITABLE
{
   char     *uniID;
   int      nChars;
   BOOL     lMulti;
   USHORT   *uniCodes;
} HB_UNITABLE, * PHB_UNITABLE;

typedef struct _HB_MULTICHAR
{
   char  cLast[2];
   char  cFirst[2];
   int   nCode;
} HB_MULTICHAR, * PHB_MULTICHAR;

typedef struct _HB_CODEPAGE
{
   char *        id;
   char *        uniID;
   PHB_UNITABLE  uniTable;
   int           nChars;
   char *        CharsUpper;
   char *        CharsLower;
   BOOL          lLatin;
   BOOL          lAccEqual;
   BOOL          lAccInterleave;
   BOOL          lSort;
   BYTE *        s_chars;
   BYTE *        s_upper;
   BYTE *        s_lower;
   BYTE *        s_accent;
   int           nMulti;
   PHB_MULTICHAR multi;
} HB_CODEPAGE, *PHB_CODEPAGE;

extern BOOL HB_EXPORT hb_cdpRegister( PHB_CODEPAGE );
extern char HB_EXPORT * hb_cdpSelectID( char * );
extern PHB_CODEPAGE HB_EXPORT hb_cdpSelect( PHB_CODEPAGE );
extern PHB_CODEPAGE HB_EXPORT hb_cdpFind( char * );
extern void HB_EXPORT hb_cdpTranslate( char*, PHB_CODEPAGE, PHB_CODEPAGE );
extern void HB_EXPORT hb_cdpnTranslate( char*, PHB_CODEPAGE, PHB_CODEPAGE, UINT );
extern int HB_EXPORT hb_cdpcmp( char*, char*, ULONG, PHB_CODEPAGE, ULONG* );
extern int HB_EXPORT hb_cdpchrcmp( char, char, PHB_CODEPAGE );

extern USHORT HB_EXPORT hb_cdpGetU16( PHB_CODEPAGE, BYTE );
extern ULONG HB_EXPORT hb_cdpStrnToUTF( PHB_CODEPAGE, BYTE*, ULONG, BYTE* );
extern ULONG HB_EXPORT hb_cdpStrnToU16( PHB_CODEPAGE, BYTE*, ULONG, BYTE* );

#define CPID_437     "cp437"
#define CPID_850     "cp850"
#define CPID_852     "cp852"
#define CPID_866     "cp866"
#define CPID_1250    "cp1250"
#define CPID_1251    "cp1251"
#define CPID_1257    "cp1257"
#define CPID_8859_1  "iso8859-1"
#define CPID_8859_2  "iso8859-2"
#define CPID_KOI_8   "koi-8"
#define CPID_MAZ     "plmaz"
#define UNITB_437    &hb_uniTbl_437
#define UNITB_850    &hb_uniTbl_850
#define UNITB_852    &hb_uniTbl_852
#define UNITB_866    &hb_uniTbl_866
#define UNITB_1250   &hb_uniTbl_1250
#define UNITB_1251   &hb_uniTbl_1251
#define UNITB_1257   &hb_uniTbl_1257
#define UNITB_8859_1 &hb_uniTbl_8859_1
#define UNITB_8859_2 &hb_uniTbl_8859_2
#define UNITB_KOI_8  &hb_uniTbl_KOI_8
#define UNITB_MAZ    &hb_uniTbl_mazovia
#define UNITB_UNDEF  NULL /* ((PHB_UNITABLE) (-1)) */

extern HB_UNITABLE hb_uniTbl_437;
extern HB_UNITABLE hb_uniTbl_850;
extern HB_UNITABLE hb_uniTbl_852;
extern HB_UNITABLE hb_uniTbl_866;
extern HB_UNITABLE hb_uniTbl_1250;
extern HB_UNITABLE hb_uniTbl_1251;
extern HB_UNITABLE hb_uniTbl_1257;
extern HB_UNITABLE hb_uniTbl_8859_1;
extern HB_UNITABLE hb_uniTbl_8859_2;
extern HB_UNITABLE hb_uniTbl_KOI_8;
extern HB_UNITABLE hb_uniTbl_mazovia;

HB_EXTERN_END

#endif /* HB_APICDP_H_ */

#else

#define PHB_CODEPAGE void*

#endif /* HB_CDP_SUPPORT_OFF */
