/*
 * $Id: tbrowse.prg,v 1.16 2002/11/13 14:42:45 walito Exp $
 */

/*
 * Harbour Project source code:
 * TBrowse Class
 *
 * Copyright 1999 Antonio Linares <alinares@fivetech.com>
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
 * Copyright 2000, '01, '02 Maurilio Longo <maurilio.longo@libero.it>
 * Cursor movement handling, stabilization loop, multi-line headers and footers support
 * ::PageUp(), ::PageDown(), ::Down(), ::Up(), ::GoBottom(), ::GoTop(), ::Stabilize()
 * ::GotoXY(), ::DispCell(), ::WriteMLineText(), ::RedrawHeaders(),
 * ::SetFrozenCols(), ::SetColumnWidth()
 *
 * Copyright 2001 Manu Exposito <maex14@dipusevilla.es>
 * Activate data PICTURE DispCell(nColumn, nColor)
 *
 */


/* NOTE: Don't use SAY in this module, use DispOut(), DispOutAt() instead,
         otherwise it will not be CA-Cl*pper compatible. [vszakats] */

/* TODO: :firstScrCol() --> nScreenCol
         Determines screen column where the first table column is displayed.
         Xbase++ compatible method */

/* TODO: :viewArea() --> aViewArea
         Determines the coordinates for the data area of a TBrowse object.
         Xbase++ compatible method */

//-------------------------------------------------------------------//

#include "common.ch"
#include "hbclass.ch"
#include "color.ch"
#include "inkey.ch"
#include "setcurs.ch"
#include "button.ch"
#include "tbrowse.ch"

//-------------------------------------------------------------------//
//
//                         Pritpal Bedi
//
//                 Constants to access ::aColsInfo
//
#define o_Obj          1    // Object Column 
#define o_Type         2    // Type of Data in Column
#define o_Width        3    // Column Width
#define o_Heading      4    // Column Headings
#define o_Pict         5    // Column Picture 
#define o_WidthCell    6    // Width of the Cell
#define o_ColSep       7    // Column Seperator
#define o_SepWidth     8    // Width of the Separator

//-------------------------------------------------------------------//

CLASS TBrowse

   DATA autoLite              // Logical value to control highlighting
   DATA cargo                 // User-definable variable
   DATA colorSpec             // Color table for the TBrowse display
   DATA colPos                // Current cursor column position
   DATA goBottomBlock         // Code block executed by TBrowse:goBottom()
   DATA goTopBlock            // Code block executed by TBrowse:goTop()
   DATA hitBottom             // Indicates the end of available data
   DATA hitTop                // Indicates the beginning of available data
   DATA leftVisible           // Indicates position of leftmost unfrozen column in display
   DATA nBottom               // Bottom row number for the TBrowse display
   DATA nLeft                 // Leftmost column for the TBrowse display
   DATA nRight                // Rightmost column for the TBrowse display
   DATA nTop                  // Top row number for the TBrowse display
   DATA rightVisible          // Indicates position of rightmost unfrozen column in display
   DATA rowCount              // Number of visible data rows in the TBrowse display
   DATA rowPos                // Current cursor row position
   DATA skipBlock             // Code block used to reposition data source
   DATA stable                // Indicates if the TBrowse object is stable

#ifdef HB_COMPAT_C53
   DATA nRow                  // Row number for the actual cell
   DATA nCol                  // Col number for the actual cell
   DATA aKeys
   DATA mColpos,mrowPos,message
#endif

   ACCESS colSep  INLINE ::cColSep                          // Column separator character
   ASSIGN colSep( cColSep ) INLINE ::cColSep := cColSep, ::Configure()
   ACCESS footSep INLINE ::cFootSep                         // Footing separator character
   ASSIGN footSep( cFootSep ) INLINE ::cFootSep := cFootSep, ::Configure()
   ACCESS headSep INLINE ::cHeadSep                         // Head separator character
   ASSIGN headSep( cHeadSep ) INLINE ::cHeadSep := cHeadSep, ::Configure()

   ACCESS freeze INLINE ::nFrozenCols                       // Number of columns to freeze/frozen
   ASSIGN freeze( nHowMany ) INLINE ::SetFrozenCols( nHowMany ), ::Configure()

   METHOD New( nTop, nLeft, nBottom, nRight )               // Constructor
   METHOD Down()                                            // Moves the cursor down one row
   METHOD End()                                             // Moves the cursor to the rightmost visible data column
   METHOD GoBottom()                                        // Repositions the data source to the bottom of file
   METHOD GoTop()                                           // Repositions the data source to the top of file
   METHOD Home()                                            // Moves the cursor to the leftmost visible data column
   MESSAGE Left() METHOD _Left()                            // Moves the cursor left one column
   METHOD PageDown()                                        // Repositions the data source downward
   METHOD PageUp()                                          // Repositions the data source upward
   METHOD PanEnd()                                          // Moves the cursor to the rightmost data column
   METHOD PanHome()                                         // Moves the cursor to the leftmost visible data column
   METHOD PanLeft()                                         // Pans left without changing the cursor position
   METHOD PanRight()                                        // Pans right without changing the cursor position
   MESSAGE Right() METHOD _Right()                          // Moves the cursor right one column
   METHOD Up()                                              // Moves the cursor up one row

   METHOD AddColumn( oCol )
   METHOD DelColumn( nPos )                                 // Delete a column object from a browse
   METHOD InsColumn( nPos, oCol )                           // Insert a column object in a browse
   METHOD GetColumn( nColumn )                              // Gets a specific TBColumn object
   METHOD SetColumn( nColumn, oCol )                        // Replaces one TBColumn object with another
   METHOD ColWidth( nColumn )                               // Returns the display width of a particular column
   METHOD ColCount() INLINE ::nColumns
   METHOD ColorRect()                                       // Alters the color of a rectangular group of cells
   METHOD Configure( nMode )                                // Reconfigures the internal settings of the TBrowse object
                                                            // nMode is an undocumented parameter in CA-Cl*pper
   METHOD DeHilite()                                        // Dehighlights the current cell
   METHOD ForceStable()                                     // Performs a full stabilization
   METHOD Hilite()                                          // Highlights the current cell
   METHOD Invalidate()                                      // Forces entire redraw during next stabilization
   METHOD RefreshAll()                                      // Causes all data to be recalculated during the next stabilize
   METHOD RefreshCurrent() INLINE;                          // Causes the current row to be refilled and repainted on next stabilize
          ::aRedraw[ ::RowPos ] := .T., ::stable := .F., Self
   METHOD Stabilize()                                       // Performs incremental stabilization

#ifdef HB_COMPAT_C53
   METHOD SetKey( nKey, bBlock )
   METHOD ApplyKey( nKey )
   METHOD InitKeys( Self )
   METHOD TApplyKey( nKey, o )
   METHOD HitTest( nMouseRow,nMouseCol )
   METHOD SetStyle( nMode,lSetting )
#endif

   PROTECTED:     /* P R O T E C T E D */

   METHOD MGotoYX( nRow, nCol )           // Given screen coordinates nRow, nCol sets TBrowse cursor on underlaying cell
                                          // _M_GotoXY because this method will mostly be called to handle mouse requests

   HIDDEN:         /* H I D D E N */

   METHOD PosCursor()                     // Positions the cursor to the beginning of the call, used only when autolite==.F.
   METHOD LeftDetermine()                 // Determine leftmost unfrozen column in display
   METHOD DispCell( nColumn, nColor, lSetPos )  // Displays a single cell and returns cell type as a single letter like Valtype()
   METHOD HowManyCol()                    // Counts how many cols can be displayed
   METHOD RedrawHeaders( nWidth )         // Repaints TBrowse Headers
   METHOD Moved()                         // Every time a movement key is issued I need to reset certain properties
                                          // of TBrowse, I do these settings inside this method

   METHOD WriteMLineText( cStr, nPadLen, lHeader, cColor ) // Writes a multi-line text where ";" is a line break, lHeader
                                                           // is .T. if it is a header and not a footer
   METHOD SetFrozenCols( nHowMany )       // Handles freezing of columns
   METHOD SetColumnWidth( oCol )          // Calcs width of given column

   DATA aRect                             // The rectangle specified with ColorRect()
   DATA aRectColor                        // The color positions to use in the rectangle specified with ColorRect()
   DATA aRedraw                           // Array of logical items indicating, is appropriate row need to be redraw
   DATA lHeaders                          // Internal variable which indicates whether there are column footers to paint
   DATA lFooters                          // Internal variable which indicates whether there are column footers to paint
   DATA lRedrawFrame                      // True if I need to redraw Headers/Footers
   DATA nColsWidth                        // Total width of visible columns plus ColSep
   DATA nColsVisible                      // Number of columns that fit on the browse width
   DATA lHitTop                           // Internal Top/Bottom reached flag
   DATA lHitBottom
   DATA nRecsToSkip                       // Recs to skip on next Stabilize()
   DATA nNewRowPos                        // Next position of data source (after first phase of stabilization)
   DATA nLastRetrieved                    // Position, relative to first row, of last retrieved row (with an Eval(::SkipBlock, n))
   DATA nRowData                          // Row, first row of data

   DATA cColSep                           // Column separator character
   DATA cFootSep                          // Footing separator character
   DATA cHeadSep                          // Head separator character

   DATA nHeaderHeight                     // How many lines is highest Header/Footer and so how many lines of
   DATA nFooterHeight                     // screen space I have to reserve
   DATA nFrozenWidth                      // How many screen column are not available on the left side of TBrowse display
                                          // > 0 only when there are frozen columns
   DATA nFrozenCols                       // Number of frozen columns on left side of TBrowse
   DATA nColumns                          // Number of columns added to TBrowse
   DATA lNeverDisplayed                   // .T. if TBrowse has never been stabilized()
   DATA lDispBegin

   DATA aColsInfo                         // Columns configuration array
   DATA nVisWidth                         // Visible width of Browser
   DATA lConfigured                       // Specifies whether tBrowse is already configured or not

#ifdef HB_COMPAT_C53
   DATA rect
   DATA aVisibleCols
   DATA aSetStyle
#endif

ENDCLASS

//-------------------------------------------------------------------//

METHOD New( nTop, nLeft, nBottom, nRight ) CLASS TBrowse

   default  nTop    to 0
   default  nLeft   to 0
   default  nBottom to MaxRow()
   default  nRight  to MaxCol()

   ::nTop            := nTop
   ::nLeft           := nLeft
   ::nBottom         := nBottom
   ::nRight          := nRight

   ::nRowData        := nTop
   ::lDispBegin      := .F.
   ::AutoLite        := .T.
   ::leftVisible     := 1
   ::ColPos          := 1
   ::HitBottom       := .F.
   ::HitTop          := .F.
   ::lHitTop         := .F.
   ::lHitBottom      := .F.
   ::ColorSpec       := SetColor()
   ::cColSep         := " "
   ::cFootSep        := ""
   ::cHeadSep        := ""
   ::RowPos          := 1
   ::nNewRowPos      := 1
   ::stable          := .F.
   ::nLastRetrieved  := 1
   ::nRecsToSkip     := 0
   ::aRedraw         := {}
   ::lHeaders        := .F.
   ::lFooters        := .F.
   ::lRedrawFrame    := .T.
   ::aRect           := {}
   ::aRectColor      := {}
   ::nColsWidth      := 0
   ::nColsVisible    := 0
   ::nHeaderHeight   := 0   // 1
   ::nFooterHeight   := 0   // 1
   ::nFrozenWidth    := 0
   ::nFrozenCols     := 0
   ::nColumns        := 0
   ::lNeverDisplayed := .T.

   ::aColsInfo       := {}     //  Pritpal Bedi
   ::nVisWidth       := ::nRight - ::nLeft + 1
   ::lConfigured     := .f.

 #ifdef HB_COMPAT_C53
   ::mColPos         := 0
   ::mRowPos         := 0
   ::rect            := { nTop, nLeft, nBottom, nRight }
   ::aVisibleCols    := {}
   ::message         := ''
   ::nRow            := 0
   ::nCol            := 0
   ::aSetStyle       := ARRAY( 5 )

   ::aSetStyle[ TBR_APPEND    ] := .f.
   ::aSetStyle[ TBR_APPENDING ] := .f.
   ::aSetStyle[ TBR_MODIFY    ] := .f.
   ::aSetStyle[ TBR_MOVE      ] := .f.
   ::aSetStyle[ TBR_SIZE      ] := .f.

 #endif

return Self

//-------------------------------------------------------------------//

METHOD Invalidate() CLASS TBrowse

   AFill( ::aRedraw, .T. )

   ::stable       := .F.
   ::lRedrawFrame := .T.

return Self

//-------------------------------------------------------------------//

METHOD RefreshAll() CLASS TBrowse

   AFill( ::aRedraw, .T. )
   ::stable := .F.

return Self

//-------------------------------------------------------------------//

METHOD Configure( nMode ) CLASS TBrowse

   local n, nHeight, aCol, xVal

   ::nVisWidth    := ::nRight - ::nLeft + 1

   ::lHeaders     := .F.
   ::lFooters     := .F.
   ::lRedrawFrame := .T.

   // Are there column headers to paint ?
   FOR EACH aCol IN ::aColsInfo
      if !Empty( aCol[ o_Obj ]:Heading )
         ::lHeaders := .T.
         exit
      endif
   NEXT

   // Are there column footers to paint ?
   FOR EACH aCol IN ::aColsInfo
      if !Empty( aCol[ o_Obj ]:Footing )
         ::lFooters := .T.
         exit
      endif
   NEXT

   ::nHeaderHeight := 0 
   ::nFooterHeight := 0 

   // Find out highest header and footer
   FOR EACH aCol IN ::aColsInfo

      xVal := Eval( aCol[ o_Obj ]:block )

      aCol[ o_Type      ] := valtype( xVal )
      aCol[ o_Width     ] := ::SetColumnWidth( aCol[ o_Obj ] )
      aCol[ o_Heading   ] := aCol[ o_Obj ]:heading 
      aCol[ o_Pict      ] := iif( Empty( aCol[ o_Obj ]:Picture ), "", aCol[ o_Obj ]:Picture )
      aCol[ o_WidthCell ] := LenVal( xVal, aCol[ o_Type ], aCol[ o_Obj ]:Picture )
      aCol[ o_ColSep    ] := iif( aCol[ o_Obj ]:ColSep != NIL,;
                                          aCol[ o_Obj ]:ColSep, ::ColSep )
      aCol[ o_SepWidth  ] := len( aCol[ o_ColSep ] )

      if aCol[ o_Type ] == 'D' .and. empty( aCol[ o_Pict ] )
         aCol[ o_Pict ] := '@D'
      endif
                        
      if ::lHeaders .AND. !Empty( aCol[ o_Heading ] )
         nHeight := Len( aCol[ o_Heading ] ) - Len( StrTran( aCol[ o_Heading ], ";" ) ) + 1

         if nHeight > ::nHeaderHeight
            ::nHeaderHeight := nHeight
         endif

      endif

      if ::lFooters .AND. !Empty( aCol[ o_Obj ]:Footing )
         nHeight := Len( aCol[ o_Obj ]:Footing ) - Len( StrTran( aCol[ o_Obj ]:Footing, ";" ) ) + 1

         if nHeight > ::nFooterHeight
            ::nFooterHeight := nHeight
         endif

      endif
   next

   // 20/nov/2000 - maurilio.longo@libero.it
   // If I add (or remove) header or footer (separator) I have to change number
   // of available rows
   //
   ::RowCount := ::nBottom - ::nTop + 1 - iif( ::lHeaders, ::nHeaderHeight, 0 ) - ;
                  iif( ::lFooters, ::nFooterHeight, 0 ) - iif( Empty( ::HeadSep ), 0, 1 ) - ;
                  iif( Empty( ::FootSep ), 0, 1 )

   //   Starting position of data rows excluding headers . Pritpal Bedi
   //
   ::nRowData := ::nTop + iif( ::lHeaders, ::nHeaderHeight, 0 ) + iif( Empty( ::HeadSep ), 0, 1 ) - 1

   if Len( ::aRedraw ) <> ::RowCount
      ::aRedraw := Array( ::RowCount )
   endif

   ::Invalidate()

   // Force re-evaluation of space occupied by frozen columns
   if ::freeze > 0
      ::SetFrozenCols( ::freeze )
   endif

#ifdef HB_COMPAT_C53
   ::Rect := { ::nTop + ::nHeaderHeight, ::nLeft, ::nBottom - ::nHeaderHeight, ::nRight }
   for n := ::nLeft to ::nRight
      aadd( ::aVisibleCols, n )
   next
#endif

   //   Flag that browser has been configured properly
   ::lConfigured := .t.

return Self

//-------------------------------------------------------------------//
//-------------------------------------------------------------------//
//-------------------------------------------------------------------//
//
//                        Columns Management
//
//-------------------------------------------------------------------//
//
//   Adds a TBColumn object to the TBrowse object
//
METHOD AddColumn( oCol ) CLASS TBrowse

   ::nColumns++
   
   aadd( ::aColsInfo, { oCol, '', 0, '', '', 0, '', 0 } )

   ::lConfigured := .f.

return Self

//-------------------------------------------------------------------//
//
//   Inserts a column object in a browse
//
METHOD InsColumn( nPos, oCol )

   ::nColumns++

   aIns( ::aColsInfo, nPos, { oCol, '', 0, '', '', 0, '', 0 }, .t. )

   ::lConfigured := .f.

return oCol

//-------------------------------------------------------------------//
//
//   Gets a specific TBColumn object
//
METHOD GetColumn( nColumn )

return iif( 0 < nColumn .AND. nColumn <= ::nColumns, ;
                                      ::aColsInfo[ nColumn, o_Obj ], NIL )

//-------------------------------------------------------------------//
//
//   Replaces one TBColumn object with another
//
METHOD SetColumn( nColumn, oCol )

   LOCAL oOldCol

   if 0 < nColumn .AND. nColumn <= ::nColumns

      oOldCol := ::aColsInfo[ nColumn, o_Obj ]

      ::aColsInfo[ nColumn ] := { oCol, '', 0, '', '', 0, '', 0 }

      ::lConfigured := .f.

   endif

return oOldCol

//-------------------------------------------------------------------//
//
//  Delete a column given the column position
//
METHOD DelColumn( nPos ) CLASS TBrowse

   local oCol := ::aColsInfo[ nPos, o_Obj ]

   ::nColumns--

   aDel( ::aColsInfo, nPos, .t. )

   ::lConfigured := .f.

return oCol

//-------------------------------------------------------------------//
//
//   Returns the display width of a particular column
//
METHOD ColWidth( nColumn )

return iif( 0 < nColumn .AND. nColumn <= ::nColumns, ;
                                 ::aColsInfo[ nColumn, o_Width ], NIL )

//-------------------------------------------------------------------//

METHOD SetFrozenCols( nHowMany ) CLASS TBrowse

   LOCAL nCol, aCol

   ::nFrozenCols  := Min( nHowMany, ::nColumns )

   // Space inside TBrowse window reserved for frozen columns
   ::nFrozenWidth := 0

   // If I've never displayed this TBrowse before I cannot calc occupied space since
   // columns:width is not yet set, ::Stabilize() will call me later
   //
   if ! ::lNeverDisplayed

      if nHowMany > 0
         for nCol := 1 TO nHowMany
            ::nFrozenWidth += ::aColsInfo[ nCol, o_Width ]
            if nCol < ::nColumns
               ::nFrozenWidth += ::aColsInfo[ nCol + 1, o_SepWidth ]
            endif
         next
      endif

      FOR EACH aCol IN ::aColsInfo
         if nHowMany > 0
            // If there are columns which are larger than TBrowse display width minus
            // frozen columns reserved space, shrihnk them to fit
            //
            if ::nFrozenWidth + aCol[ o_Width ] > ::nVisWidth
               aCol[ o_Width ] := ::nVisWidth - ::nFrozenWidth
            endif

         else
            // Reset column widths
            //
            aCol[ o_Width ] := ::SetColumnWidth( aCol[ o_Obj ] )
         endif
      NEXT
   endif

return nHowMany

//-------------------------------------------------------------------//

METHOD SetColumnWidth( oCol ) CLASS TBrowse

   LOCAL xRes, cType, nTokenPos := 0, nL, cHeading, cFooting
   LOCAL nWidth := 0, nHeadWidth := 0, nFootWidth := 0, nLen := 0

   // if oCol has :Width property set I use it
   //
   if oCol:Width <> nil
      nWidth := Min( oCol:Width, ::nVisWidth )

   else
      if ISBLOCK( oCol:block )

         cType    := Valtype( xRes := Eval( oCol:block ) )
         nLen     := LenVal( xRes, cType )

         cHeading := oCol:Heading + ";"
         while ( nL := Len( __StrTkPtr( @cHeading, @nTokenPos, ";" ) ) ) > 0
            nHeadWidth += nL
         enddo

         nTokenPos := 0

         cFooting  := oCol:Footing + ";"
         while ( nL := Len( __StrTkPtr( @cFooting, @nTokenPos, ";" ) ) ) > 0
            nFootWidth += nL
         enddo
      endif

      nWidth := Min( Max( nHeadWidth, Max( nFootWidth, nLen ) ), ::nVisWidth )

   endif

return nWidth

//-------------------------------------------------------------------//
//-------------------------------------------------------------------//
//-------------------------------------------------------------------//
//
//                         Up-down movements
//
//-------------------------------------------------------------------//

METHOD Down() CLASS TBrowse

   ::Moved()
   ::nRecsToSkip := 1

return Self

//-------------------------------------------------------------------//

METHOD Up() CLASS TBrowse

   ::Moved()
   ::nRecsToSkip := -1

return Self

//-------------------------------------------------------------------//

METHOD PageDown() CLASS TBrowse

   ::Moved()
   ::nRecsToSkip := ( ::RowCount - ::RowPos ) + ::RowCount
   
return Self

//-------------------------------------------------------------------//

METHOD PageUp() CLASS TBrowse

   ::Moved()
   ::nRecsToSkip := - ( ( ::RowPos - 1 ) + ::RowCount )

return Self

//-------------------------------------------------------------------//

METHOD GoBottom() CLASS TBrowse

   local nToTop

   ::Moved()

   Eval( ::goBottomBlock )

   //   Skip back from last record as many records as TBrowse can hold
   nToTop := Abs( Eval( ::SkipBlock, - ( ::RowCount - 1 ) ) )

   //   From top of TBrowse new row position is nToTop + 1 records away
   ::nNewRowPos := nToTop + 1

   //   Last read record is first record inside TBrowse
   ::nLastRetrieved := 1
   ::RefreshAll()

return Self

//-------------------------------------------------------------------//

METHOD GoTop() CLASS TBrowse

   ::Moved()

   Eval( ::goTopBlock )
   ::nLastRetrieved := 1
   ::nNewRowPos     := 1
   ::RefreshAll()

return Self

//-------------------------------------------------------------------//
//-------------------------------------------------------------------//
//-------------------------------------------------------------------//
//
//                        Left-right movemets
//
//-------------------------------------------------------------------//

METHOD Home() CLASS TBrowse

   ::Moved()

   if ::ColPos != ::leftVisible
      ::ColPos := ::leftVisible
      ::lRedrawFrame := .T.
      ::RefreshCurrent()
   endif

return Self

//-------------------------------------------------------------------//

METHOD End() CLASS TBrowse

   ::Moved()

   if ::ColPos < ::rightVisible
      ::ColPos := ::rightVisible
      ::lRedrawFrame := .T.
      ::RefreshCurrent()
   endif

return Self

//-------------------------------------------------------------------//

METHOD _Right() CLASS TBrowse

   ::Moved()

   if ::ColPos < ::rightVisible
      ::ColPos++
   else
      if ::ColPos < ::nColumns
         ::rightVisible++
         ::leftVisible := ::LeftDetermine()
         ::ColPos++
         ::lRedrawFrame := .T.
         ::RefreshAll()
      endif
   endif

return Self

//-------------------------------------------------------------------//

METHOD _Left() CLASS TBrowse

   local leftVis := ::leftVisible

   ::Moved()

   if ::ColPos > ::leftVisible .or.;
         ( ::ColPos <= ::nFrozenCols + 1 .and. ::ColPos > 1 )
      ::ColPos--
   else
      if ::ColPos <= Max( ::leftVisible, ::nFrozenCols ) .AND. ::ColPos > 1
         while leftVis == ::leftVisible
            ::rightVisible--
            ::leftVisible := ::LeftDetermine()
         end
         ::ColPos--
         ::lRedrawFrame := .T.
         ::RefreshAll()
      endif
   endif

return Self

//-------------------------------------------------------------------//

METHOD PanEnd() CLASS TBrowse

   ::Moved()

   if ::ColPos < ::nColumns
      if ::rightVisible <  ::nColumns
         ::rightVisible := ::nColumns
         ::leftVisible  := ::LeftDetermine()
         ::ColPos       := ::RightVisible
         ::lRedrawFrame := .T.
         ::RefreshAll()
      else
         ::ColPos       := ::RightVisible
         ::RefreshCurrent()
      endif
   endif

return Self

//-------------------------------------------------------------------//

METHOD PanHome() CLASS TBrowse

   ::Moved()

   if ::ColPos > 1
      if ::leftVisible > ::nFrozenCols + 1
         ::leftVisible  := ::nFrozenCols + 1
         ::ColPos       := 1
         ::RefreshAll()
         ::lRedrawFrame := .T.
      else
         ::ColPos       := 1
         ::RefreshCurrent()
      endif
   endif

return Self

//-------------------------------------------------------------------//

METHOD PanLeft() CLASS TBrowse

   local n := ::ColPos - ::leftVisible
   local leftVis := ::leftVisible

   ::Moved()

   if ::leftVisible > ::nFrozenCols + 1
      while leftVis == ::leftVisible
         ::rightVisible--
         ::leftVisible := ::LeftDetermine()
      end
      ::ColPos := Min( ::leftVisible + n, ::rightVisible )
      ::lRedrawFrame := .T.
      ::RefreshAll()
   endif

return Self

//-------------------------------------------------------------------//

METHOD PanRight() CLASS TBrowse

   local n := ::ColPos - ::leftVisible

   ::Moved()

   if ::rightVisible < ::nColumns
      ::rightVisible++
      ::leftVisible  := ::LeftDetermine()
      ::ColPos       := Min( ::leftVisible + n, ::rightVisible )
      ::lRedrawFrame := .T.
      ::RefreshAll()
   endif

return Self

//-------------------------------------------------------------------//
//-------------------------------------------------------------------//
//-------------------------------------------------------------------//
//
//          Movement keys cause TBrowse to become unstable
//
//-------------------------------------------------------------------//

METHOD Moved() CLASS TBrowse

   // No need to Dehilite() current cell more than once
   if ::stable
      // Internal flags used to set ::HitTop/Bottom during next stabilization
      ::lHitTop    := .F.
      ::lHitBottom := .F.

      if ::AutoLite
         ::DeHilite()
      else
         ::PosCursor()
      endif
      ::stable := .F.
   endif

return Self

//-------------------------------------------------------------------//
//-------------------------------------------------------------------//
//-------------------------------------------------------------------//
//
//                         Utility Routines 
//
//-------------------------------------------------------------------//

METHOD LeftDetermine() CLASS TBrowse

   local nWidth := ::nFrozenWidth
   local nCol   := ::rightVisible

   while nWidth < ::nVisWidth .and. nCol > ::nFrozenCols

      nWidth += ::aColsInfo[ nCol, o_Width ] 

      if nWidth < ::nVisWidth
         nCol--
         if nCol < ::nColumns
            nWidth += ::aColsInfo[ nCol + 1, o_SepWidth ]
         endif
      endif
   enddo

return Min( nCol + 1, ::nColumns )

//-------------------------------------------------------------------//

METHOD DeHilite() CLASS TBrowse

   SetPos( ( ::RowPos + ::nRowData ), ::aColsInfo[ ::colPos, o_Obj ]:colPos )

   ::DispCell( ::ColPos, CLR_STANDARD )
   
   SetPos( ( ::RowPos + ::nRowData ), ::aColsInfo[ ::colPos, o_Obj ]:colPos )

return Self

//-------------------------------------------------------------------//

METHOD Hilite() CLASS TBrowse

   local nRow := ::RowPos + ::nRowData

   // Start of cell
   SetPos( nRow, ::aColsInfo[ ::ColPos, o_Obj ]:ColPos )

   ::DispCell( ::ColPos, CLR_ENHANCED )

   SetPos( nRow, ::aColsInfo[ ::ColPos, o_Obj ]:ColPos )

 #ifdef HB_COMPAT_C53
   ::nRow := nRow
   ::nCol := col()
 #endif

return Self

//-------------------------------------------------------------------//

METHOD PosCursor() CLASS TBrowse

   local nRow := ::RowPos + ::nRowData
   local nCol

   nCol := ::aColsInfo[ ::ColPos, o_Obj ]:ColPos + ;
                  iif( ::aColsInfo[ ::colPos, o_Type ] == "L", ;
                           ::aColsInfo[ ::ColPos, o_Width ] / 2, 0 )

   // Put cursor on first char of cell value
   SetPos( nRow, nCol )

 #ifdef HB_COMPAT_C53
   ::nRow := nRow
   ::nCol := nCol
 #endif

return Self

//-------------------------------------------------------------------//
//
//  Calculate how many columns fit on the browse width including ColSeps
//
METHOD HowManyCol() CLASS TBrowse

   local nToAdd := 0

   // They were locals, so now I need to clear them (should fix this)
   //
   ::nColsWidth   := 0
   ::nColsVisible := 0

   if ::nFrozenCols > 0
      if ::leftVisible <= ::nFrozenCols
         ::leftVisible := ::nFrozenCols + 1
      endif

      ::nColsVisible := 0
      while ::nColsVisible < ::nFrozenCols
         nToAdd := ::aColsInfo[ ::nColsVisible + 1, o_Width ]

         if ::nColsVisible >= 1 .and. ::nColsVisible < ::nColumns
            nToAdd += ::aColsInfo[ ::nColsVisible + 1, o_SepWidth ]
         endif

         if ::nColsWidth + nToAdd > ::nVisWidth
            exit
         endif

         ::nColsWidth += nToAdd
         ::nColsVisible++
      enddo

      if ::nColsWidth > ::nVisWidth
         /* NOTE: Why do I change frozen columns here? */
         ::Freeze     := 0
         ::nColsWidth := 0
      endif
   endif

   ::nColsVisible := ::leftVisible - 1

   while ::nColsVisible < ::nColumns
      nToAdd := ::aColsInfo[ ::nColsVisible + 1, o_Width ]

      if ::nColsVisible >= ::leftVisible .or. ::nFrozenCols > 0
         nToAdd += ::aColsInfo[ ::nColsVisible + 1, o_SepWidth ]
      endif

      if ::nColsWidth + nToAdd > ::nVisWidth
         exit
      endif

      ::nColsWidth += nToAdd
      ::nColsVisible++
   enddo

   ::rightVisible := ::nColsVisible

return Self

//-------------------------------------------------------------------//
//
// Gets TBrowse width and width of displayed columns plus colsep
//
METHOD RedrawHeaders( nWidth ) CLASS TBrowse

   local n, nTPos, nBPos
   local cBlankBox := Space( 9 )
   local nScreenRowT, nScreenRowB
   local nLCS 
   local aCol, nCol
   local nColFrom

   aCol := ARRAY( ::rightVisible )
   nCol := ::nLeft + iif( ::nFrozenCols > 0, 0, ( nWidth - ::nColsWidth ) / 2 )

   nColFrom := iif( ::nFrozenCols > 0, 1, ::leftVisible )

   for n := nColFrom to ::rightVisible

      aCol[ n ] := nCol

      nCol += ::aColsInfo[ n, o_Width ]

      if ::nFrozenCols > 0 .and. n == ::nFrozenCols
         n    := ::leftVisible - 1
         nCol += ( nWidth - ::nColsWidth ) / 2
      endif

      if n <= ::rightVisible
         nCol += ::aColsInfo[ n, o_SepWidth ]
      endif

   next

   if ::lHeaders          // Drawing headers

      // Clear area of screen occupied by headers
      DispBox( ::nTop, ::nLeft, ::nTop + ::nHeaderHeight - 1, ::nRight, cBlankBox, ::ColorSpec )

      for n := nColFrom to ::rightVisible
         if ::nFrozenCols > 0 .and. n == ::nFrozenCols + 1
            n := ::leftVisible
         endif
         DevPos( ::nTop, aCol[ n ] )

         ::WriteMLineText( ::aColsInfo[ n, o_Heading ], ::aColsInfo[ n, o_Width ], .T., ::ColorSpec )

      next
   endif

   if ! Empty( ::HeadSep )  //Draw horizontal heading separator line
      DispOutAt( ( nScreenRowT := ::nRowData ), ::nLeft,;
                Replicate( Right( ::HeadSep, 1 ), nWidth ), ::ColorSpec )
   endif

   if ! Empty( ::FootSep )  //Draw horizontal footing separator line
      DispOutAt( ( nScreenRowB := ::nBottom - iif( ::lFooters, ::nFooterHeight, 0 ) ), ::nLeft,;
                Replicate( Right( ::FootSep, 1 ), nWidth ), ::ColorSpec )
   endif

   nTPos := nBPos := aCol[ iif( ::nFrozenCols > 0, 1, ::leftVisible ) ]

   // Draw headin/footing column separator
   for n := iif( ::nFrozenCols > 0, 1, ::leftVisible ) to ::rightVisible
      if ::nFrozenCols > 0 .and. n == ::nFrozenCols + 1
         n     := ::leftVisible
         nTPos += ( nWidth - ::nColsWidth ) / 2
         nBPos += ( nWidth - ::nColsWidth ) / 2
      endif

      if n < ::rightVisible
         nLCS := ::aColsInfo[ n + 1, o_SepWidth ]

         if ! Empty( ::HeadSep )
            DispOutAT( nScreenRowT, ( nTPos += ::aColsInfo[ n, o_Width ] ), ::HeadSep, ::ColorSpec )
            nTPos += nLCS           
         endif

         if ! Empty( ::FootSep )
            DispOutAT( nScreenRowB, ( nBPos += ::aColsInfo[ n, o_Width ] ), ::FootSep, ::ColorSpec )
            nBPos += nLCS
         endif

      endif
   next

   if ::lFooters                // Drawing footers

      // Clear area of screen occupied by footers
      DispBox( ::nBottom - ::nFooterHeight + 1, ::nLeft, ::nBottom, ::nRight, cBlankBox, ::ColorSpec )

      for n := iif( ::nFrozenCols > 0, 1, ::leftVisible ) to ::rightVisible
         if ::nFrozenCols > 0 .and. n == ::nFrozenCols + 1
            n := ::leftVisible
         endif
         DevPos( ::nBottom, aCol[ n ] )

         ::WriteMLineText( ::aColsInfo[ n, o_Obj ]:Footing, ::aColsInfo[ n, o_Width ], .F., ::ColorSpec )

      next
   endif

return Self

//-------------------------------------------------------------------//
//-------------------------------------------------------------------//
//-------------------------------------------------------------------//
//
//                         Stabilization
//
//-------------------------------------------------------------------//

METHOD ForceStable() CLASS TBrowse

   // If ForceStable() is called after movement of data source
   // (in simple words, the record pointer is moved) where the
   // movement was not effected by TBrowse (e.g user set scope)
   // then TBrowse has to evaluate if painting should start
   // from relative to current cursor row.  If not it should know
   // how many rows of data is available for painting prior
   // to paiting relative to current cursor row.  If sufficient data is not
   // available then reset position to row 1.

   Local nAvail           // How many records are avaialble?
   Local lReset := .F.    // Reposition to row 1 required?

   If ::nRecsToSkip == 0  // Ensure that you do not reset the painting
                          // of a movement caused by TBrowse methods

      If ::RowPos # 1     // No repositioning is required if current
                          // cursor row is one

         nAvail := Eval( ::SkipBlock, 0 - ::RowPos - 1 )

         // You should reposition only if there are too few records
         // available or there are more than sufficient records
         // available.  If there are exact number of records
         // leave it.

         lReset := Abs( nAvail ) + 1 # ::RowPos

         // Place back the data source pointer to where it was

         Eval( ::SkipBlock, 0 - nAvail )

      EndIf

   EndIf

   If lReset   // So repositioning was required !

      // nNewRowPos and nLastRetrieved have to be updated
      // as we will entering phase 2 of stabilization

      ::RowPos := ::nNewRowPos := ::nLastRetrieved := ;
                  If( Abs( nAvail ) + 1 > ::RowPos, ::RowPos, Abs( nAvail ) + 1 )
      ::Moved()

      // To ensure phase 1 is skipped
      ::nRecsToSkip := 0

   EndIf

   while !::Stabilize()
   end
   
return Self

//-------------------------------------------------------------------//

METHOD Stabilize() CLASS TBrowse

   local nRow, n
   local lDisplay                      // Is there something to show inside current cell?
   local nRecsSkipped                  // How many records do I really skipped?
   local nFirstRow                     // Where is on screen first row of TBrowse?
   local nOldCursor                    // Current shape of cursor (which I remove before stabilization)
   local nColFrom
   local nSpacePre

   // Configure the browse if not configured . Pritpal Bedi
   //
   if !::lConfigured
      ::configure()
   endif

   // I need to set columns width If TBrowse was never displayed before
   if ::lNeverDisplayed
      //AEVal(::aColumns, {|oCol| ::SetColumnWidth(oCol)} )

      // NOTE: It must be before call to ::SetFrozenCols() since this call
      //       tests this iVar value, and I set it to .F. since I'm going to display TBrowse
      //       for first time
      //
      ::lNeverDisplayed := .F.

      // Force re-evaluation of frozen space since I could not calc it before
      // being columns width not set
      //
      if ::freeze > 0
         ::SetFrozenCols( ::freeze )
      endif
   endif

   nOldCursor := SetCursor( SC_NONE )

   if ::lRedrawFrame
      // How may columns fit on TBrowse width?
      //
      ::HowManyCol()
      ::RedrawHeaders( ::nVisWidth )

      // Now that browser frame has been redrawn we don't need to redraw it unless
      // displayed columns change
      //
      ::lRedrawFrame := .F.

   endif

   // From this point there is stabilization of rows which is made up of three phases
   // 1st repositioning of data source
   // 2nd redrawing of rows, after each row we exit stabilization loop with .F.
   // 3rd if all rows have been redrawn we set ::stable state to .T.
   //
   if !::stable

      // NOTE: I can enter here because of a movement key or a ::RefreshAll():ForceStable() call

      // If I have a requested movement still to handle
      //
      if ::nRecsToSkip <> 0

         // If I'm not under cursor 
         // maybe I've interrupted an ongoing stabilization
         // I have to set data source to cursor position
         //
         if ::nLastRetrieved <> ::nNewRowPos
            Eval( ::SkipBlock, ::nNewRowPos - ::nLastRetrieved )
            ::nLastRetrieved := ::nNewRowPos
         endif

         nRecsSkipped := Eval( ::SkipBlock, ::nRecsToSkip )

         // I've tried to move past top or bottom margin
         //
         if nRecsSkipped == 0

            if ::nRecsToSkip > 0
               ::lHitBottom := .T.

            elseif ::nRecsToSkip < 0
               ::lHitTop := .T.

            endif

         elseif nRecsSkipped == ::nRecsToSkip

            // If after movement I'm still inside present TBrowse
            //
            if ( ::nNewRowPos + nRecsSkipped >= 1 ) .AND. ( ::nNewRowPos + nRecsSkipped <= ::RowCount )
               ::nNewRowPos     += nRecsSkipped
               ::nLastRetrieved := ::nNewRowPos
               
               // This is needed since present TBrowse has no cache, so I need to repaint current row
               // rereading it from data source and to force rereading from data source I have to mark
               // row as invalid
               //
               ::aRedraw[ ::nNewRowPos ] := .T.

            else
               // It was K_PGDN or K_PGUP
               //
               if Abs( nRecsSkipped ) >= ::RowCount

                  // K_PGDN
                  //
                  if nRecsSkipped > 0
                     ::nLastRetrieved := ::RowCount

                  else // K_PGUP
                     ::nLastRetrieved := 1

                  endif
                  ::RefreshAll()

               else // K_DN or K_UP

                  // Where does really start first TBrowse row?
                  //
                  nFirstRow := ::nRowData + 1

                  // I'm at top or bottom of TBrowse so I can scroll
                  //
                  if ::nNewRowPos == ::RowCount
                     Scroll( nFirstRow + nRecsSkipped - 1, ::nLeft, nFirstRow + ::RowCount - 1, ::nRight, nRecsSkipped )
                     ::nLastRetrieved := ::RowCount
                     DispOutAt( ::nRowData + ::RowCount, ::nLeft, space( ::nVisWidth ), ::colorSpec )

                  else
                     Scroll( nFirstRow, ::nLeft, nFirstRow + ::RowCount + nRecsSkipped, ::nRight, nRecsSkipped )
                     ::nLastRetrieved := 1
                     DispOutAt( ::nRowData+1, ::nLeft, space( ::nVisWidth ), ::colorSpec )

                  endif

                  // I've scrolled on screen rows, now I need to scroll ::aRedraw array as well!
                  //
                  if nRecsSkipped > 0
                     ACopy( ::aRedraw, ::aRedraw, 2,, 1 )

                  else
                     // Cannot use ACopy() here
                     //
                     for nRow := ::RowCount - 1 to 1 step -1
                        ::aRedraw[ nRow + 1 ] := ::aRedraw[ nRow ]
                     next

                  endif

                  ::aRedraw[ ::nNewRowPos ] := .T.
               endif
            endif

         else // I couldn't move as far as requested

            // I need to refresh all rows if I go past current top or bottom row
            //
            if ( ::nNewRowPos + nRecsSkipped < 1 ) .OR. ( ::nNewRowPos + nRecsSkipped > ::RowCount )
               // don't go past boundaries
               //
               ::nNewRowPos := iif( nRecsSkipped > 0, ::RowCount, 1 )
               ::RefreshAll()

            else
               ::nNewRowPos += nRecsSkipped
               ::aRedraw[ ::nNewRowPos ] := .T.

            endif

            ::nLastRetrieved := ::nNewRowPos

         endif

         // Data source moved, so next time I won't enter this stage of stabilization
         //
         ::nRecsToSkip := 0

         // Exit first stage of stabilization
         //
         SetCursor( nOldCursor )
         return .F.

      endif

      // Data source is alredy at correct record number, now we need
      // to repaint browser accordingly.
      //
      for nRow := 1 to ::RowCount

         // if there is a row to repaint
         if ::aRedraw[ nRow ]

            // NOTE: If my TBrowse has 20 rows but I have only 3 recs, clipper clears
            //       remaining 17 rows in a single operation, I will, instead, try to skip
            //       17 times. Should be made more clever.
            //
            if nRow <> ::nLastRetrieved
               if lDisplay := Eval( ::SkipBlock, nRow - ::nLastRetrieved ) == ( nRow - ::nLastRetrieved )
                  ::nLastRetrieved := nRow
               endif
            else
               lDisplay := .T.
            endif

            nColFrom   := iif( ::nFrozenCols > 0, 1, ::leftVisible )
            nSpacePre  := ( ::nVisWidth - ::nColsWidth ) / 2 

				//  This if statement is just to speed up the refresh process
				//  It is almost identical, but please do not try to merge it in one.
				//
            if ::nFrozenCols == 0
               DispOutAt( nRow + ::nRowData, ::nLeft, space( nSpacePre ), ::ColorSpec )

               for n := nColFrom to ::rightVisible

                  if nRow == 1
                     ::aColsInfo[ n, o_Obj ]:ColPos := Col()
                  endif

                  if lDisplay
                     ::DispCell( @n, CLR_STANDARD )

                  else
                     // Clear cell
                     //
                     DispOut( Space( ::aColsInfo[ n, o_Width ] ), ::ColorSpec )

                  endif

                  if n < ::rightVisible
                     DispOut( ::aColsInfo[ n + 1, o_ColSep ], ::ColorSpec )

                  endif
               next

            else      //  ::nFrozenCols > 0
               DevPos( nRow + ::nRowData, ::nLeft )

               for n := nColFrom to ::rightVisible

                  if n == ::nFrozenCols + 1
                     n := ::leftVisible
                     DispOut( space( nSpacePre ), ::ColorSpec )
                  endif

                  if nRow == 1
                     ::aColsInfo[ n, o_Obj ]:ColPos := Col()
                  endif

                  if lDisplay
                     ::DispCell( @n, CLR_STANDARD )

                  else
                     // Clear cell
                     //
                     DispOut( Space( ::aColsInfo[ n, o_Width ] ), ::ColorSpec )

                  endif

                  if n < ::rightVisible
                     DispOut( ::aColsInfo[ n + 1, o_ColSep ], ::ColorSpec )

                  endif
               next
            endif

            DispOut( Space( Int( Round( ( ::nVisWidth - ::nColsWidth ) / 2, 0 ) ) ), ::ColorSpec )

            // doesn't need to be redrawn
            ::aRedraw[ nRow ] := .F.

            // Exit incremental row stabilization
            SetCursor( nOldCursor )
            return .F.

         endif
      next        // nRow

      // If I reach this point I've repainted all rows so I can set ::stable state
      if nRow > ::RowCount

         // If I have fewer records than available TBrowse rows, cursor cannot be lower than
         // last record (note ::lHitBottom is set only during a movement)
         //
         if ::nLastRetrieved < ::nNewRowPos
            ::nNewRowPos := ::nLastRetrieved
         endif

         // If I'm not already under cursor I have to set data source to cursor position
         //
         if ::nLastRetrieved <> ::nNewRowPos
            Eval( ::SkipBlock, ::nNewRowPos - ::nLastRetrieved )
            ::nLastRetrieved := ::nNewRowPos
         endif

         // new cursor position
         //
         ::RowPos    := ::nNewRowPos

         ::HitTop    := ::lHitTop
         ::HitBottom := ::lHitBottom

         if ::AutoLite
            ::Hilite()
         else
            ::PosCursor()
         endif
         SetCursor( nOldCursor )

         ::stable := .T.

         return .T.
      endif

   else
      /* NOTE: DBU relies upon current cell being reHilited() even if already stable */
      if ::AutoLite
         ::Hilite()
      else
         ::PosCursor()
      endif
      SetCursor( nOldCursor )

      return .T.

   endif

return .F.

//---------------------------------------------------------------------//
//
//                              Display 
//
//---------------------------------------------------------------------//

METHOD ColorRect( aRect, aRectColor ) CLASS TBrowse

   ::aRect       := aRect
   ::aRectColor  := aRectColor

return Self

//-------------------------------------------------------------------//

METHOD DispCell( nColumn, nColor ) CLASS TBrowse

   LOCAL ftmp  := Eval( ::aColsInfo[ nColumn, o_Obj ]:block )
   
   //  Just to prevent accessing object property in do case loop several times
   //
   LOCAL cType := ::aColsInfo[ nColumn, o_Type ]

   // NOTE: When nColor is used as an array index we need to increment 
   // it by one since CLR_STANDARD is 0
   //
   LOCAL cColor := iif( ::aColsInfo[ nColumn, o_Obj ]:ColorBlock != NIL,;
                        hb_ColorIndex( ::ColorSpec, Eval( ::aColsInfo[ nColumn, o_Obj ]:ColorBlock, ftmp )[ nColor + 1 ] - 1 ),;
                        hb_ColorIndex( ::ColorSpec, nColor ) )

   do case
   case cType $ "CM"
      DispOut( PadR( Transform( ftmp, ::aColsInfo[ nColumn, o_Pict ] ), ;
                          ::aColsInfo[ nColumn, o_Width ] ), cColor )

   case cType == "N"
      DispOut( PadL( Transform( ftmp, ::aColsInfo[ nColumn, o_Pict ] ), ;
                          ::aColsInfo[ nColumn, o_Width ] ), cColor )

   case cType == "D"
      DispOut( PadR( Transform( ftmp, ::aColsInfo[ nColumn, o_Pict ] ), ;
                          ::aColsInfo[ nColumn, o_Width ] ), cColor )

   case cType == "L"
      ftmp := padc( if( ftmp, 'T', 'F' ), ::aColsInfo[ nColumn, o_Width ] )
      DispOut( ftmp, cColor )

   otherwise
      DispOut( Space( ::aColsInfo[ nColumn, o_Width ] ), cColor )

   endcase

return self

//-------------------------------------------------------------------//
//
//   NOTE: Not tested, could be broken
//
METHOD MGotoYX( nRow, nCol ) CLASS TBrowse

   local nColsLen, nI, nNewRow

   // Am I inside TBrowse display area ?
   //
   if nRow > ::nTop  .AND. nRow < ::nBottom .AND. ;
      nCol > ::nLeft .AND. nCol < ::nRight

      // if not stable force repositioning of data source; maybe this is not first Stabilize() call after
      // TBrowse became unstable, but we need to call Stabilize() al least one time before moving again to be sure
      // data source is under cursor position
      //
      if ! ::stable
         ::Stabilize()

      else
         ::Moved()

      endif

      // Set new row position
//      nNewRow := nRow - ::nTop + iif( ::lHeaders, ::nHeaderHeight, 0 ) + iif( Empty( ::HeadSep ), 0, 1 ) - 1
      nNewRow       := nRow - ::nRowData
      ::nRecsToSkip := nNewRow - ::nNewRowPos

      // move data source accordingly
      //
      ::Stabilize()

      // Now move to column under nCol
      //
      nColsLen := 0

      // NOTE: I don't think it is correct, have to look up docs
      //
      nI := iif( ::nFrozenCols > 0, ::nFrozenCols, ::leftVisible )

      while nColsLen < nCol .AND. nI < ::rightVisible

         nColsLen += ::aColsInfo[ nI, o_Width ]
         if nI >= 1 .AND. nI < ::nColumns
            nColsLen += ::aColsInfo[ nI, o_SepWidth ] 
         endif

         nI++

      enddo

      ::ColPos := nI

      // Force redraw of current row with new cell position
      //
      ::RefreshCurrent()

   endif

return Self

//-------------------------------------------------------------------//

METHOD WriteMLineText( cStr, nPadLen, lHeader, cColor ) CLASS TBrowse

   local n, cS
   local nCol := Col()
   local nRow := Row()

   // Do I have to write an header or a footer?
   //
   if lHeader

      // Simple case, write header as usual
      //
      if ::nHeaderHeight == 1
         DispOut( PadR( cStr, nPadLen ), cColor )

      else
         // __StrToken needs that even last token be ended with token separator
         //
         cS := cStr + ";"

         for n := ::nHeaderHeight to 1 step -1
            DevPos( nRow + n - 1, nCol )
            DispOut( PadR( __StrToken( @cS, n, ";" ), nPadLen ), cColor )

         next

         DevPos( nRow, nCol + nPadLen )

      endif

   // footer
   //
   else

      // Simple case, write footer as usual
      //
      if ::nFooterHeight == 1
         DispOut( PadR( cStr, nPadLen ), cColor )

      else
         // __StrToken needs that even last token be ended with token separator
         //
         cS := cStr + ";"

         for n := 0 to ( ::nFooterHeight - 1 )
            DevPos( nRow - n, nCol )
            DispOut( PadR( __StrToken( @cS, ::nFooterHeight - n, ";" ), nPadLen ), cColor )
         next

         DevPos( nRow, nCol + nPadLen )

      endif

   endif

return Self

//---------------------------------------------------------------------//
//
//                      Clipper 5.3b Compatibility
//
//---------------------------------------------------------------------//
#ifdef HB_COMPAT_C53
//---------------------------------------------------------------------//

METHOD ApplyKey( nKey )  CLASS TBrowse

return ::TApplyKey( nKey, self )

//-------------------------------------------------------------------//

METHOD InitKeys( o ) CLASS TBROWSE

   Default o:aKeys to {;
              { K_DOWN,        {| Ob, nKey | Ob:Down()    , 0 } },;
              { K_END,         {| Ob, nKey | Ob:End()     , 0 } },;
              { K_CTRL_PGDN,   {| Ob, nKey | Ob:GoBottom(), 0 } },;
              { K_CTRL_PGUP,   {| Ob, nKey | Ob:GoTop()   , 0 } },;
              { K_HOME,        {| Ob, nKey | Ob:Home()    , 0 } },;
              { K_LEFT,        {| Ob, nKey | Ob:Left()    , 0 } },;
              { K_PGDN,        {| Ob, nKey | Ob:PageDown(), 0 } },;
              { K_PGUP,        {| Ob, nKey | Ob:PageUp()  , 0 } },;
              { K_CTRL_END,    {| Ob, nKey | Ob:PanEnd()  , 0 } },;
              { K_CTRL_HOME,   {| Ob, nKey | Ob:PanHome() , 0 } },;
              { K_CTRL_LEFT,   {| Ob, nKey | Ob:PanLeft() , 0 } },;
              { K_CTRL_RIGHT,  {| Ob, nKey | Ob:PanRight(), 0 } },;
              { K_RIGHT,       {| Ob, nKey | Ob:Right()   , 0 } },;
              { K_UP,          {| Ob, nKey | Ob:Up()      , 0 } },;
              { K_ESC,         {| Ob, nKey | -1 } },;
              { K_LBUTTONDOWN, {| Ob, nKey | tbmouse( ob,mrow(),mcol() ) } } }
return o

//-------------------------------------------------------------------//

METHOD SetKey( nKey,bBlock ) CLASS TBrowse

   local bReturn,nPos

   ::InitKeys( self )

   if ( nPos := ascan( ::aKeys,{| x | x[ 1 ] == nkey } ) ) == 0
      if ISBLOCK( bBlock )
         bReturn := bBlock
         aadd( ::aKeys, { nKey, bBlock } )
      endif
      bReturn := bBlock

   elseif ISBLOCK( bBlock )
      ::aKeys[ npos ][ 2 ] := bBlock
      bReturn := bBlock

   elseif PCOUNT() == 1
      bReturn := ::aKeys[ npos ][ 2 ]

   elseif ( bReturn := ::aKeys[ nPos ][ 2 ], PCount() == 2 .AND. ;
                                 ISNIL( bBlock ) .AND. nKey != 0 )
      adel( ::aKeys, nPos, .T. )

   endif

return bReturn

//-------------------------------------------------------------------//

METHOD TApplyKey( nKey, oBrowse ) CLASS tBrowse

   local bBlock := oBrowse:setkey( nKey ), nReturn := TBR_CONTINUE  // 0

   default bBlock to oBrowse:setkey( 0 )

   if ISNIL( bBlock )
      nReturn := TBR_EXCEPTION  // 1
   else
      nReturn := eval( bBlock, oBrowse, nKey )
   endif

return nReturn

//-------------------------------------------------------------------//

Method hitTest( mrow,mcol ) CLASS TBROWSE
  Local nVisCol
  ::mRowPos := ::rowPos
  ::mColPos := ::colPos


  if mRow< ::rect[ 1 ] .or. mRow > ::rect[ 3 ]
     return HTNOWHERE
  endif

  if mCol < ::rect[ 2 ] .or. mCol > ::rect[ 4 ]
     return HTNOWHERE
  endif

  ::mRowPos := mRow - ::rect[ 1 ] + 1

  for each nVisCol in ::aVisibleCols
     if nVisCol > mcol
        ::mColpos := nVisCol
        exit
     endif
  next

return HTCELL

//-------------------------------------------------------------------//

METHOD SetStyle( nMode, lSetting ) CLASS TBROWSE
  LOCAL lRet := .F.

  IF nMode > LEN( ::aSetStyle )
     RETURN .F.
  ENDIF

  lRet := ::aSetStyle[ nMode ]

  IF ISLOGICAL( lSetting )
     ::aSetStyle[ nMode ] := lSetting
  ENDIF

RETURN lRet

//-------------------------------------------------------------------//

function TBMOUSE( oBrowse, nMouseRow, nMouseCol )

   local n
   if oBrowse:hittest( nMouseRow, nMouseCol ) == -5121

      n := oBrowse:mrowpos - oBrowse:rowpos


      do while ( n < 0 )
         n++
         oBrowse:up()
      enddo

      do while ( n > 0 )
         n--
         oBrowse:down()
      enddo

      n := oBrowse:mcolpos - oBrowse:colpos

      do while ( n < 0 )
         n++
         oBrowse:left()
      enddo

      do while ( n > 0 )
         n--
         oBrowse:right()
      enddo

      return 0
   endif

   return 1

//---------------------------------------------------------------------//
#endif
//---------------------------------------------------------------------//

static function LenVal( xVal, cType, cPict )
   LOCAL nLen

   if !ISCHARACTER( cType )
      cType := Valtype( xVal )
   endif

   do case
      case cType == "L"
         nLen := 1

      case !Empty( cPict ) .and. cType $ "NCD"
         nLen := Len( Transform( xVal, cPict ) )

      case cType == "N"
         nLen := Len( Str( xVal ) )

      case cType == "C"
         nLen := Len( xVal )

      case cType == "D"
         nLen := Len( DToC( xVal ) )

      otherwise
         nLen := 0

   endcase

return nLen

//-------------------------------------------------------------------//
//
//                   Function to Activate TBrowse
//
//-------------------------------------------------------------------//

function TBrowseNew( nTop, nLeft, nBottom, nRight )

return TBrowse():New( nTop, nLeft, nBottom, nRight )

//-------------------------------------------------------------------//

