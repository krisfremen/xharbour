/*
 * $Id: dattime2.prg,v 1.2 2005/01/13 23:00:00 ptsarenko Exp $
 */

/*
 * Harbour Project source code:
 *   CT3 Date & Time functions, part II: - ADDMONTH()
 *                                       - CTODOW()
 *                                       - CTOMONTH()
 *                                       - DAYSINMONTH() 
 *                                       - DAYSTOMONTH() 
 *                                       - DMY() 
 *                                       - DOY() 
 *                                       - ISLEAP() 
 *                                       - LASTDAYOM() 
 *                                       - MDY() 
 *                                       - NTOCDOW() 
 *                                       - NTOCMONTH() 
 *                                       - QUARTER() 
 *                                       - WEEK() 
 *
 * Copyright 2002 Alan Secker <alansecker@globalnet.co.uk>
 * Copyright 2003 Martin Vogel <vogel@inttec.de>: Enhancements, internationalization, documentation headers
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


#include "set.ch"
#include "hblang.ch"



FUNCTION addmonth ( ddate, nmth )
local nDay
// local ndays
// local sev
// local dnew
// local dEnd
// local nOldday
local nMonth
local nYear
local nLDOM

//   if nmth > 70 
//      return ctod ("  /  /    ")
//   endif
//
//   nOldday := day (ddate)
//   ndays   := nmth * 30
//
//   dnew    := ddate + ndays
//
//   nMonth  := month ( dnew )
//   nNyear  := year  ( dnew )
//
//   dEnd    := ctod (str (nOldday) + "/" + str(nMonth) + "/" + str (nNyear))
//
//   return dEnd  

   if !(valtype (ddate) $ "DN")
      return ctod("")
   endif

   if valtype (ddate) == "N"
      nmth := ddate
      ddate := date()
   endif

   nmth = int (nmth)

   nDay = day (ddate)
   nMonth = month (ddate)
   nYear = year (ddate)

   nMonth += nmth

   if nMonth <= 0
      do while nMonth <= 0
	 nMonth += 12
	 nYear--
      enddo
   endif

   if nMonth > 12
      do while nMonth > 12
	 nMonth -= 12
	 nYear++
      enddo
   endif

   // correction for different end of months
   if nDay > (nLDOM := lastdayom (nMonth))
     nDay := nLDOM
   endif

   ddate := stod (strzero (nYear, 4) + strzero (nMonth, 2) + strzero (nDay, 2))
   return (ddate)

  

FUNCTION ctodow ( cDow )
//local cWeek  := "SUNMONTUEWEDTHUFRISAT "
//local nWk    := len (cWeek)
//local cMatch := left (upper ( Alltrim (cDow)), 3)
//local n 
//local nDay   := 0
//
//   for n = 1 to nWk step 3
//        if RTRIM (substr (cWeek, n, 3)) == cMatch
//           nDay := INT (((n-1) / 3) + 1)
//           exit
//        endif
//   next
//
//   return nDay

local nOrdinal := 0
local bExact

  if valtype (cDow) != "C"
     return (0)
  endif
  
  bExact = set (_SET_EXACT, .F.)
  cDow = upper (alltrim (cDow))

  do while nOrdinal < 7
     if upper (alltrim (hb_langmessage (HB_LANG_ITEM_BASE_DAY + nOrdinal))) = cDow
	set (_SET_EXACT, bExact)
	return (nOrdinal+1)
     endif
     nOrdinal++
  enddo

  set (_SET_EXACT, bExact)
  return (0)



FUNCTION ctomonth ( cDom )
//local cMonth := "JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC "
//local nMnth  := len (cMonth)
//local cMatch := left (upper ( Alltrim (cDom)), 3)
//local n 
//local nDay   := 0

local nOrdinal := 0
local bExact

//   for n = 1 to nMnth step 3
//        if RTRIM (substr (cMonth, n, 3)) == cMatch
//           nDay := INT (((n-1) / 3) + 1)
//           exit
//        endif
//   next
//
//   return nDay

  if valtype (cDom) != "C"
     return (0)
  endif
  
  bExact = set (_SET_EXACT, .F.)
  cDom = upper (alltrim (cDom))

  do while nOrdinal < 12
     if upper (alltrim (hb_langmessage (HB_LANG_ITEM_BASE_MONTH + nOrdinal))) = cDom
	set (_SET_EXACT, bExact)
	return (nOrdinal+1)
     endif
     nOrdinal++
  enddo

  set (_SET_EXACT, bExact)
  return (0)


FUNCTION dmy ( ddate, lmode )

//local nMonth  := month (dDate)
//local nDay    :=   day (dDate)
//local nYear   :=  year (dDate)
local nMonth, nDay, nYear 

local cPeriod := ""
local cDate
local cMonth 

local cYear
local lSetCentury := __SETCENTURY()

   if valtype (ddate) != "D"
      ddate := date ()
   endif

   nMonth  := month (ddate)
   nDay    :=   day (ddate)
   nYear   :=  year (ddate)

   cMonth := ntocmonth ( nMonth )
   cYear := str (nYear, iif (lSetCentury, 4, 2))

   cPeriod := if (lmode == .T., ".", "")


   cDate := ltrim ( str ( nDay )) + cPeriod + " " + cMonth + " " + ;
            ltrim ( cYear )
//            ltrim ( str ( nYear ))
   return cDate


#pragma BEGINDUMP

#include "hbapi.h"
#include "hbdate.h"


HB_FUNC( DOY )
{
   int iYear, iMonth, iDay;
   LONG lFirst, lCurrent;
   PHB_ITEM pDate = hb_param( 1, HB_IT_DATE );

   if( pDate )
   {
      lCurrent = pDate->item.asDate.value;
      hb_dateDecode( pDate->item.asDate.value, &iYear, &iMonth, &iDay );
   }
   else
   {
      hb_dateToday( &iYear, &iMonth, &iDay );
      lCurrent = hb_dateEncode(iYear, iMonth, iDay);
   }

   lFirst = hb_dateEncode(iYear, 1, 1);

   hb_retnl( lCurrent - lFirst + 1);
}

HB_FUNC( ISLEAP )
{
   int iYear, iMonth, iDay;
   PHB_ITEM pDate = hb_param( 1, HB_IT_DATE );

   if( pDate && pDate->item.asDate.value )
   {
      hb_dateDecode( pDate->item.asDate.value, &iYear, &iMonth, &iDay );
   }
   else
   {
      hb_dateToday( &iYear, &iMonth, &iDay );
   }

   hb_retl( ( ( iYear % 4 == 0 && iYear % 100 != 0 ) || iYear % 400 == 0 ) );
}


HB_FUNC( DAYSTOMONTH )
{
   int iMonth = (ISNUM( 1 ) ? hb_parni(1) : 0 );
   BOOL bLeap = (ISLOG( 2 ) ? hb_parl(2) : 0 );
   int iMonthes[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

   hb_retni( (iMonth < 1 && iMonth > 12) ? 0 : iMonthes[iMonth - 1] +
              ((bLeap && iMonth > 2) ? 1 : 0) );
}

HB_FUNC( DAYSINMONTH )
{
   int iMonth = (ISNUM( 1 ) ? hb_parni(1) : 0 );
   BOOL bLeap = (ISLOG( 2 ) ? hb_parl(2) : 0 );

   if( iMonth == 2)
   {
      hb_retni( bLeap ? 29 : 28 );
   }
   else if(iMonth == 4 || iMonth == 6 || iMonth == 9 || iMonth == 11)
   {
      hb_retni( 30 );
   }
   else
   {
      hb_retni( 31 );
   }

}

HB_FUNC( QUARTER )
{
   int iYear, iMonth, iDay;
   PHB_ITEM pDate = hb_param( 1, HB_IT_DATE );

   if( pDate && pDate->item.asDate.value )
   {
      hb_dateDecode( pDate->item.asDate.value, &iYear, &iMonth, &iDay );
   }
   else
   {
      hb_dateToday( &iYear, &iMonth, &iDay );
   }

   hb_retni( (iMonth+2) / 3);
}


#pragma ENDDUMP

   
FUNCTION lastdayom ( xDate )

local nMonth := 0
local nDays  := 0
local lleap  := .F.

     do case
        case empty ( xDate)
             nMonth := month ( date() )

        case valtype ( xDate ) == "D"
             nMonth  := month (xdate)
             lleap := isleap ( xdate)

        case valtype (xDate ) == "N"
             if xdate > 12
                nmonth := 0
             else
                nMonth := xDate
             endif
     endcase

     if nmonth != 0
        ndays := daysInmonth ( nMonth, lleap )
     endif

     return ndays

 

FUNCTION mdy ( dDate )

local nMonth
local nDay
local nYear
local cDate
local cMonth 

local lSetCentury := __SETCENTURY()
local cYear

//   default dDate to date()
   if valtype (ddate) != "D"
      ddate := date ()
   endif

   nMonth  := month (dDate)
   nDay    :=   day (dDate)
   nYear   :=  year (dDate)
   cMonth  := ntocmonth ( nMonth )

   cYear := str (nYear, iif (lSetCentury, 4, 2))

   cDate := cMonth + " " + ;
            ltrim ( str ( nDay )) + " " + ;
            ltrim ( cYear )
   //            ltrim ( str ( nYear ))
   
   return cDate



FUNCTION ntocdow ( nDay )
   
local cDay := ""
   
   if nDay >= 1 .AND. nDay <= 7
     cDay := hb_langmessage (HB_LANG_ITEM_BASE_DAY + (nDay-1))
   endif

   return cDay



FUNCTION ntocmonth ( nMonthNum )
   
local cMonth := ""
   
   if nMonthNum >= 1 .AND. nMonthNum <= 12
     cMonth := hb_langmessage (HB_LANG_ITEM_BASE_MONTH + (nMonthNum-1))
   endif

//  do case
//     case nMonthNum == 1
//          cMonth := "January"
//     case nMonthNum == 2
//          cMonth := "February"
//     case nMonthNum == 3
//          cMonth := "March"
//     case nMonthNum == 4
//          cMonth := "April"
//     case nMonthNum == 5
//          cMonth := "May"
//     case nMonthNum == 6
//          cMonth := "June"
//     case nMonthNum == 7
//          cMonth := "July"
//     case nMonthNum == 8
//          cMonth := "August"
//     case nMonthNum == 9
//          cMonth := "September"
//     case nMonthNum == 10
//          cMonth := "October"
//    case nMonthNum == 11
//          cMonth := "November"
//     case nMonthNum == 12
//          cMonth := "December"
//  endcase

    return cMonth



FUNCTION week ( dDate, lSWN )

local nMonth
local nDays
local nDay
local nYear
local nWeek
local nPart
local dDate2
// local nleap

//       do case
//	  case valtype (dDate) == "D" .and. empty ( dDate)
//             return nDays
//	     
//	  case empty (dDate)
//             dDate := date()
//       endcase

     if valtype (dDate) == "D" .and. empty (dDate)
	return 0
     endif

     if empty (dDate)
	dDate := date()
     endif

     nMonth  := month (dDate)
     nDay    :=   day (dDate)
     nYear   :=  year (dDate)

     if valtype (lSWN) != "L"
	lSWN := .F.
     endif

     if lSWN
       // simple week number

       //     nleap := if (isleap (dDate), 1, nleap)
       //     ndays := daystomonth ( nMonth, nleap ) + nday
       nDays := daystomonth ( nMonth, isleap (dDate)) + nDay
       
       nPart := nDays % 7
       nWeek := INT (nDays / 7)
       
       nWeek := INT (if ( nPart > 0, ++ nWeek, nWeek))

     else
	// ISO8601 week number
	dDate2 := dDate + 3 - ((dow(dDate)+5) % 7)
	nWeek := 1 + int ((dDate2 - boy (dDate2)) / 7) 
	
     endif

     return nWeek






