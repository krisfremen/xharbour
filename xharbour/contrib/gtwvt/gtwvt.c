/*
 * $Id: gtwvt.c,v 1.141 2005/01/13 10:30:31 bdj Exp $
 */

/*
 * Harbour Project source code:
 * Video subsystem for Win32 using GUI windows instead of Console
 *     Copyright 2003 Peter Rees <peter@rees.co.nz>
 *                    Rees Software & Systems Ltd
 * based on
 *   Bcc ConIO Video subsystem by
 *     Copyright 2002 Marek Paliwoda <paliwoda@inteia.pl>
 *     Copyright 2002 Przemyslaw Czerpak <druzus@polbox.com>
 *   Video subsystem for Win32 compilers
 *     Copyright 1999-2000 Paul Tucker <ptucker@sympatico.ca>
 *     Copyright 2002 Przemysław Czerpak <druzus@polbox.com>
 *
 * The following parts are Copyright of the individual authors.
 * www - http://www.harbour-project.org
 *
 *
 * Copyright 1999 David G. Holm <dholm@jsd-llc.com>
 *    hb_gt_Tone()
 *
 * See doc/license.txt for licensing terms.
 *
 * www - http://www.harbour-project.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option )
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.   If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA (or visit the web site http://www.gnu.org/ ).
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
 * not apply to the code that you add in this way.   To avoid misleading
 * anyone as to the status of such modified files, you must delete
 * this exception notice from them.
 *
 * If you write modifications of your own for Harbour, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice.
 *
 */

//-------------------------------------------------------------------//

/*
* Individual authors:
* (C) 2003-2004 Giancarlo Niccolai <gc at niccolai dot ws>
*         Standard xplatform GT Info system,
*         Graphical object system and event system.
*         GTINFO() And GTO_* implementation.
*
* (C) 2004 Mauricio Abre <maurifull@datafull.com>
*         Cross-GT, multiplatform Graphics API
*
*/

//-------------------------------------------------------------------//

#define HB_OS_WIN_32_USED

#ifndef _WIN32_IE
   #define _WIN32_IE 0x0400
#endif

#include "hbgtwvt.h"

#ifndef WM_MOUSEWHEEL
   #define WM_MOUSEWHEEL 0x020A
#endif

#ifndef INVALID_FILE_SIZE
   #define INVALID_FILE_SIZE (DWORD)0xFFFFFFFF
#endif

#ifndef CC_ANYCOLOR
   #define CC_ANYCOLOR 0x00000100
#endif

#ifndef IDC_HAND
   #define IDC_HAND MAKEINTRESOURCE(32649)
#endif

//-------------------------------------------------------------------//

static TCHAR szAppName[] = TEXT( "xHarbour WVT" );

static GLOBAL_DATA _s;

static COLORREF _COLORS[] = {
   BLACK,
   BLUE,
   GREEN,
   CYAN,
   RED,
   MAGENTA,
   BROWN,
   WHITE,
   LIGHT_GRAY,
   BRIGHT_BLUE,
   BRIGHT_GREEN,
   BRIGHT_CYAN,
   BRIGHT_RED,
   BRIGHT_MAGENTA,
   YELLOW,
   BRIGHT_WHITE
};

#ifdef WVT_DEBUG
static int nCountPuts=0,nCountScroll=0, nCountPaint=0, nSetFocus=0, nKillFocus=0;
#endif

static int K_Ctrl[] = {
  K_CTRL_A, K_CTRL_B, K_CTRL_C, K_CTRL_D, K_CTRL_E, K_CTRL_F, K_CTRL_G, K_CTRL_H,
  K_CTRL_I, K_CTRL_J, K_CTRL_K, K_CTRL_L, K_CTRL_M, K_CTRL_N, K_CTRL_O, K_CTRL_P,
  K_CTRL_Q, K_CTRL_R, K_CTRL_S, K_CTRL_T, K_CTRL_U, K_CTRL_V, K_CTRL_W, K_CTRL_X,
  K_CTRL_Y, K_CTRL_Z
  };

//-------------------------------------------------------------------//
//
//                  private functions declaration
//
HB_EXTERN_BEGIN
static void    gt_hbInitStatics( void );
static HWND    hb_wvt_gtCreateWindow( HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow );
static BOOL    hb_wvt_gtInitWindow( HWND hWnd, USHORT col, USHORT row );
static void    hb_wvt_gtResetWindowSize( HWND hWnd );
static LRESULT CALLBACK hb_wvt_gtWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
static BOOL    hb_wvt_gtAllocSpBuffer( USHORT col, USHORT row );
static DWORD   hb_wvt_gtProcessMessages( void );
static BOOL    hb_wvt_gtValidWindowSize( int rows, int cols, HFONT hFont, int width );
static void    hb_wvt_gtSetCaretOn( BOOL bOn );
static BOOL    hb_wvt_gtSetCaretPos( void );
static void    hb_wvt_gtValidateCaret( void );

static USHORT  hb_wvt_gtGetMouseX( void );
static USHORT  hb_wvt_gtGetMouseY( void );
static void    hb_wvt_gtSetMouseX( USHORT ix );
static void    hb_wvt_gtSetMouseY( USHORT iy );
static BOOL    hb_wvt_gtGetCharFromInputQueue( int * c );

static void    hb_wvt_gtTranslateKey( int key, int shiftkey, int altkey, int controlkey );

static void    hb_wvt_gtSetInvalidRect( USHORT left, USHORT top, USHORT right, USHORT bottom );
static void    hv_wvt_gtDoInvalidateRect( void );

static void    hb_wvt_gtHandleMenuSelection( int );

static POINT   hb_wvt_gtGetColRowFromXY( USHORT x, USHORT y );
static RECT    hb_wvt_gtGetColRowFromXYRect( RECT xy );
static POINT   hb_wvt_gtGetColRowForTextBuffer( USHORT index );

static void    hb_wvt_gtValidateCol( void );
static void    hb_wvt_gtValidateRow( void );

static USHORT  hb_wvt_gtCalcPixelHeight( void );
static USHORT  hb_wvt_gtCalcPixelWidth( void );
static BOOL    hb_wvt_gtSetColors( HDC hdc, BYTE attr );
static HFONT   hb_wvt_gtGetFont( char * pszFace, int iHeight, int iWidth, int iWeight, int iQuality, int iCodePage );

static BOOL    hb_wvt_gtTextOut( HDC hdc, USHORT col, USHORT row, LPCTSTR lpString,  USHORT cbString  );
static void    hb_wvt_gtSetStringInTextBuffer( USHORT col, USHORT row, BYTE attr, BYTE *sBuffer, USHORT length );
static USHORT  hb_wvt_gtGetIndexForTextBuffer( USHORT col, USHORT row );
static RECT    hb_wvt_gtGetXYFromColRowRect( RECT colrow );
static void    hb_wvt_gtCreateObjects( void );
static void    hb_wvt_gtKillCaret( void );
static void    hb_wvt_gtCreateCaret( void );
static void    hb_wvt_gtMouseEvent( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
static void    hb_wvt_gtCreateToolTipWindow( void );
static void    hb_wvt_gtRestGuiState( LPRECT rect );
static void    hb_wvt_gtInitGui( void );

//-------------------------------------------------------------------//
//
// mouse initialization was made in cmdarg.c
//

// set in mainwin.c
//
extern HANDLE  hb_hInstance;
extern HANDLE  hb_hPrevInstance;
extern int     hb_iCmdShow;

static USHORT  s_uiDispCount;
static USHORT  s_usCursorStyle;
static USHORT  s_usOldCurStyle;

static int s_iStdIn, s_iStdOut, s_iStdErr;

/* last updated GT object */
HB_GT_GOBJECT *last_gobject;

HB_EXTERN_END

extern BOOL    b_MouseEnable;

#define _GetScreenHeight()  (_s.ROWS)
#define _GetScreenWidth()   (_s.COLS)

//-------------------------------------------------------------------//
//-------------------------------------------------------------------//
//-------------------------------------------------------------------//
//
//                     GT Specific Functions
//
//-------------------------------------------------------------------//
//-------------------------------------------------------------------//
//-------------------------------------------------------------------//

void HB_GT_FUNC( gt_Init( int iFilenoStdin, int iFilenoStdout, int iFilenoStderr ) )
{
    /* FSG: filename var for application name */
   PHB_FNAME pFileName;

    HB_TRACE( HB_TR_DEBUG, ( "hb_gt_Init()" ) );

    /* stdin && stdout && stderr */
    s_iStdIn  = iFilenoStdin;
    s_iStdOut = iFilenoStdout;
    s_iStdErr = iFilenoStderr;

    last_gobject = NULL;

    s_usOldCurStyle = s_usCursorStyle = SC_NORMAL;

    gt_hbInitStatics();

    _s.hWnd = hb_wvt_gtCreateWindow( ( HINSTANCE ) hb_hInstance, ( HINSTANCE ) hb_hPrevInstance,  "", hb_iCmdShow );
    if ( !_s.hWnd )
    {
      //  Runtime error
      //
      hb_errRT_TERM( EG_CREATE, 10001, "WINAPI CreateWindow() failed", "hb_gt_Init()", 0, 0 );
    }
    pFileName = hb_fsFNameSplit( hb_cmdargARGV()[0] );
    hb_wvt_gtSetWindowTitle( pFileName->szName );
    hb_xfree( pFileName );

    hb_wvt_gtCreateObjects();
    _s.hdc        = GetDC( _s.hWnd );
    _s.hCompDC    = CreateCompatibleDC( _s.hdc );
    hb_wvt_gtInitGui();

    hb_wvt_wvtCore();
    hb_wvt_wvtUtils();

    if( b_MouseEnable )
    {
      HB_GT_FUNC( mouse_Init() );
    }

    if( b_MouseEnable )
    {
      hb_wvt_gtCreateToolTipWindow();
    }
}

//-------------------------------------------------------------------//

void HB_GT_FUNC( gt_Exit( void ) )
{
    int i;

    HB_TRACE( HB_TR_DEBUG, ( "hb_gt_Exit()" ) );

    if ( _s.hWnd )
    {
      for ( i = 0; i < WVT_DLGML_MAX; i++ )
      {
         if ( _s.hDlgModeless[ i ] )
         {
            SendMessage( _s.hDlgModeless[ i ], WM_CLOSE, 0, 0 );
         }
      }

      DeleteObject( ( HPEN   ) _s.penWhite      );
      DeleteObject( ( HPEN   ) _s.penWhiteDim   );
      DeleteObject( ( HPEN   ) _s.penBlack      );
      DeleteObject( ( HPEN   ) _s.penDarkGray   );
      DeleteObject( ( HPEN   ) _s.penGray       );
      DeleteObject( ( HPEN   ) _s.penNull       );
      DeleteObject( ( HPEN   ) _s.currentPen    );
      DeleteObject( ( HBRUSH ) _s.currentBrush  );
      DeleteObject( ( HBRUSH ) _s.diagonalBrush );
      DeleteObject( ( HBRUSH ) _s.solidBrush    );
      DeleteObject( ( HBRUSH ) _s.wvtWhiteBrush );

      if ( _s.hdc )
      {
         ReleaseDC( _s.hWnd, _s.hdc );
      }

      if ( _s.hCompDC )
      {
         DeleteDC( _s.hCompDC );
      }
      if ( _s.hGuiDC )
      {
         DeleteDC( _s.hGuiDC );
      }
      if ( _s.hGuiBmp )
      {
         DeleteObject( _s.hGuiBmp );
      }


      for ( i = 0; i < WVT_PICTURES_MAX; i++ )
      {
         if ( _s.iPicture[ i ] )
         {
            hb_wvt_gtDestroyPicture( _s.iPicture[ i ] );
         }
      }
      for ( i = 0; i < WVT_FONTS_MAX; i++ )
      {
         if ( _s.hUserFonts[ i ] )
         {
            DeleteObject( _s.hUserFonts[ i ] );
         }
      }
      for ( i = 0; i < WVT_PENS_MAX; i++ )
      {
         if ( _s.hUserPens[ i ] )
         {
            DeleteObject( _s.hUserPens[ i ] );
         }
      }
      if ( _s.hMSImg32 )
      {
         FreeLibrary( _s.hMSImg32 );
      }

      DestroyWindow( _s.hWnd );
    }
    UnregisterClass( szAppName,( HINSTANCE ) hb_hInstance );

    if( b_MouseEnable )
    {
       HB_GT_FUNC( mouse_Exit() );
    }
}

//-------------------------------------------------------------------//
//
//   returns the number of displayable columns
//
USHORT HB_GT_FUNC( gt_GetScreenWidth( void ) )
{
  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_GetScreenWidth()" ) );

  return _GetScreenWidth();
}

//-------------------------------------------------------------------//
//
//   returns the number of displayable rows
//
USHORT HB_GT_FUNC( gt_GetScreenHeight( void ) )
{
  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_GetScreenHeight()"));

  return _GetScreenHeight();
}

//-------------------------------------------------------------------//

SHORT HB_GT_FUNC( gt_Col( void ) )
{
  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_Col()" ) );

  return( (SHORT) _s.caretPos.x );
}

//-------------------------------------------------------------------//

SHORT HB_GT_FUNC( gt_Row( void ) )
{
  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_Row()" ) );

  return( (SHORT) _s.caretPos.y );
}

//-------------------------------------------------------------------//

void HB_GT_FUNC( gt_SetPos( SHORT sRow, SHORT sCol, SHORT sMethod ) )
{
  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_SetPos( %hd, %hd, %hd )", sRow, sCol, sMethod ) );

  HB_SYMBOL_UNUSED( sMethod );

  if ( sRow >= 0 && sRow< _GetScreenHeight() && sCol>=0 && sCol <= _GetScreenWidth() )
  {
    _s.caretPos.x = sCol;
    _s.caretPos.y = sRow;
    hb_wvt_gtValidateCaret();
  }
}

//-------------------------------------------------------------------//

BOOL HB_GT_FUNC( gt_AdjustPos( BYTE * pStr, ULONG ulLen ) )
{
  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_AdjustPos( %s, %lu )", pStr, ulLen ) );

  HB_SYMBOL_UNUSED( pStr );
  HB_SYMBOL_UNUSED( ulLen );

  return( FALSE );
}

//-------------------------------------------------------------------//

BOOL HB_GT_FUNC( gt_IsColor( void ) )
{
  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_IsColor()" ) );

  return( TRUE );
}

//-------------------------------------------------------------------//

USHORT HB_GT_FUNC( gt_GetCursorStyle( void ) )
{
  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_GetCursorStyle()" ) );

  return( s_usCursorStyle );
}

//-------------------------------------------------------------------//

void HB_GT_FUNC( gt_SetCursorStyle( USHORT usStyle ) )
{
  BOOL bCursorOn= TRUE;

  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_SetCursorStyle( %hu )", usStyle ) );

  s_usCursorStyle = usStyle;
  switch( usStyle )
  {
    case SC_NONE:
      _s.CaretSize = 0 ;
      bCursorOn= FALSE;
      break ;
    case SC_INSERT:
      _s.CaretSize = ( _s.PTEXTSIZE.y / 2 ) ;
      break;
    case SC_SPECIAL1:
      _s.CaretSize = _s.PTEXTSIZE.y ;
      break;
    case SC_SPECIAL2:
      _s.CaretSize = -( _s.PTEXTSIZE.y / 2 ) ;
      break;
    case SC_NORMAL:
    default:
      _s.CaretSize = 4 ;
      break;
  }
  if ( bCursorOn )
  {
    _s.CaretExist = CreateCaret( _s.hWnd, ( HBITMAP ) NULL, _s.PTEXTSIZE.x, _s.CaretSize );
  }
  hb_wvt_gtSetCaretOn( bCursorOn );
}

//-------------------------------------------------------------------//

void HB_GT_FUNC( gt_DispBegin( void ) )
{
  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_DispBegin()" ) );

  ++s_uiDispCount;
}

//-------------------------------------------------------------------//

void HB_GT_FUNC( gt_DispEnd() )
{
  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_DispEnd()" ) );

  if ( s_uiDispCount > 0 )
  {
    --s_uiDispCount;
  }
  if ( s_uiDispCount<= 0 )
  {
    hv_wvt_gtDoInvalidateRect();
  }
}

//-------------------------------------------------------------------//

USHORT HB_GT_FUNC( gt_DispCount() )
{
  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_DispCount()" ) );

  return( s_uiDispCount );
}

//-------------------------------------------------------------------//

void HB_GT_FUNC( gt_Puts( USHORT usRow, USHORT usCol, BYTE byAttr, BYTE *pbyStr, ULONG ulLen ) )
{
  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_Puts( %hu, %hu, %d, %p, %lu )", usRow, usCol, ( int ) byAttr, pbyStr, ulLen ) );
  hb_wvt_gtSetStringInTextBuffer( (SHORT) usCol, (SHORT) usRow, byAttr, pbyStr, (SHORT) ulLen );
#ifdef WVT_DEBUG
  nCountPuts++;
#endif
}

//-------------------------------------------------------------------//

void HB_GT_FUNC( gt_Replicate( USHORT usRow, USHORT usCol, BYTE byAttr, BYTE byChar, ULONG ulLen ) )
{
  BYTE  ucBuff[ WVT_CHAR_BUFFER ], *byChars;
  ULONG i;
  BOOL  bMalloc = FALSE;

  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_Replicate( %hu, %hu, %i, %i, %lu )", usRow, usCol, byAttr, byChar, ulLen ) );

  if ( ulLen > WVT_CHAR_BUFFER )
  {  // Avoid allocating memory if possible
    byChars = ( BYTE* ) hb_xgrab( ulLen );
    bMalloc= TRUE;
  }
  else
  {
    byChars = ucBuff ;
  }

  for ( i = 0; i < ulLen; i++ )
  {
    *( byChars+i ) = byChar;
  }

  hb_wvt_gtSetStringInTextBuffer( (SHORT) usCol, (SHORT) usRow, byAttr, byChars, (SHORT) ulLen );
  if ( bMalloc )
  {
    hb_xfree( byChars );
  }
}

//-------------------------------------------------------------------//

int HB_GT_FUNC( gt_RectSize( USHORT rows, USHORT cols ) )
{
  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_RectSize()" ) );

  return( rows * cols * 2 );
}

//-------------------------------------------------------------------//

void HB_GT_FUNC( gt_GetText( USHORT top, USHORT left, USHORT bottom, USHORT right, BYTE * sBuffer ) )
{
  USHORT irow, icol, index, j;

  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_GetText( %hu, %hu, %hu, %hu, %p )", top, left, bottom, right, sBuffer ) );

  j = 0;
  for ( irow = top; irow <= bottom; irow++ )
  {
    index = hb_wvt_gtGetIndexForTextBuffer( left, irow );
    for ( icol = left; icol <= right; icol++ )
    {
      if ( index >= _s.BUFFERSIZE )
      {
        break;
      }
      else
      {
        sBuffer[ j++ ] = _s.pBuffer[ index ];
        sBuffer[ j++ ] = _s.pAttributes[ index ];
        index++;
      }
    }
  }
}

//-------------------------------------------------------------------//

void HB_GT_FUNC( gt_PutText( USHORT top, USHORT left, USHORT bottom, USHORT right, BYTE * sBuffer ) )
{
  USHORT irow, icol, index, j;

  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_PutText( %hu, %hu, %hu, %hu, %p )", top, left, bottom, right, sBuffer ) );

  j = 0;
  for ( irow = top; irow <= bottom; irow++ )
  {
    index = hb_wvt_gtGetIndexForTextBuffer( left, irow );
    for ( icol = left; icol <= right; icol++ )
    {
      if ( index >= _s.BUFFERSIZE )
      {
        break;
      }
      else
      {
        _s.pBuffer[ index ] = sBuffer[ j++ ];
        _s.pAttributes[ index ] = sBuffer[ j++ ];
        index++;
      }
    }
  }
  hb_wvt_gtSetInvalidRect( left, top, right, bottom );
}

//-------------------------------------------------------------------//

void HB_GT_FUNC( gt_SetAttribute( USHORT rowStart, USHORT colStart, USHORT rowStop, USHORT colStop, BYTE attr ) )
{
  USHORT irow, icol, index;

  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_SetAttribute( %hu, %hu, %hu, %hu, %d", rowStart, colStart, rowStop, colStop, ( int ) attr ) );

  for ( irow = rowStart; irow <=rowStop; irow++ )
  {
    index = hb_wvt_gtGetIndexForTextBuffer( colStart, irow );
    for ( icol = colStart; icol <= colStop; icol++ )
    {
      if ( index >= _s.BUFFERSIZE )
      {
        break;
      }
      else
      {
        _s.pAttributes[ index++ ] = attr;
      }
    }
  }
  hb_wvt_gtSetInvalidRect( colStart, rowStart, colStop, rowStop );
}

//-------------------------------------------------------------------//
//
//    copied from gtwin...
//
void HB_GT_FUNC( gt_Scroll( USHORT usTop, USHORT usLeft, USHORT usBottom, USHORT usRight, BYTE byAttr, SHORT iRows, SHORT iCols ) )
{
  SHORT         usSaveRow, usSaveCol;
  // UINT          uiSize;
  BYTE ucBlank[ WVT_CHAR_BUFFER ], ucBuff[ WVT_CHAR_BUFFER * 2 ] ;
  BYTE * fpBlank ;
  BYTE * fpBuff  ;
  int           iLength = ( usRight - usLeft ) + 1;
  int           iCount, iColOld, iColNew, iColSize;
  BOOL          bMalloc = FALSE;

  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_Scroll( %hu, %hu, %hu, %hu, %d, %hd, %hd )", usTop, usLeft, usBottom, usRight, ( int ) byAttr, iRows, iCols ) );

  if ( iLength > WVT_CHAR_BUFFER )
  { // Avoid allocating memory if possible
    fpBlank = ( BYTE * ) hb_xgrab( iLength );
    fpBuff  = ( BYTE * ) hb_xgrab( iLength * 2 );  //*2 room for attribs
    bMalloc = TRUE;
  }
  else
  {
    fpBlank = ucBlank ;
    fpBuff  = ucBuff  ;
  }

  memset( fpBlank, hb_ctGetClearB(), iLength );

  iColOld = iColNew = usLeft;
  iColSize = iLength -1;
  if( iCols >= 0 )
  {
    iColOld += iCols;
    iColSize -= iCols;
  }
  else
  {
    iColNew -= iCols;
    iColSize += iCols;
  }
  // use the ScrollWindowEx() where possible ( Optimised for Terminal Server )
  // if both iCols & iRows are ZERO then the entire area is to be cleared and
  // there is no advantage in using ScrollWindowEx()
  //
  _s.InvalidateWindow = HB_GT_FUNC( gt_DispCount() ) > 0 || ( !iRows && !iCols ) ;

  // if _s.InvalidateWindow is FALSE it is used to stop
  //   HB_GT_FUNC( gt_Puts() ) & HB_GT_FUNC( gt_PutText() )
  //   from actually updating the screen. ScrollWindowEx() is used
  //
  if ( _s.InvalidateWindow )
  {
    HB_GT_FUNC( gt_DispBegin() );
  }

  usSaveCol = HB_GT_FUNC( gt_Col() ) ;
  usSaveRow = HB_GT_FUNC( gt_Row() ) ;
  for( iCount = ( iRows >= 0 ? usTop : usBottom );
       ( iRows >= 0 ? iCount <= usBottom : iCount >= usTop );
       ( iRows >= 0 ? iCount++ : iCount-- ) )
  {
      int iRowPos = iCount + iRows;


      /* Read the text to be scrolled into the current row */
      if( ( iRows || iCols ) && iRowPos <= usBottom && iRowPos >= usTop )
      {
        HB_GT_FUNC( gt_GetText( iRowPos, iColOld, iRowPos, iColOld + iColSize, fpBuff ) );
      }

      /* Blank the scroll region in the current row */
      HB_GT_FUNC( gt_Puts( iCount, usLeft, byAttr, fpBlank, iLength ) );

      /* Write the scrolled text to the current row */
      if( ( iRows || iCols ) && iRowPos <= usBottom && iRowPos >= usTop )
      {
        HB_GT_FUNC( gt_PutText( iCount, iColNew, iCount, iColNew + iColSize, fpBuff ) );
      }
  }
  HB_GT_FUNC( gt_SetPos( usSaveRow, usSaveCol, HB_GT_SET_POS_AFTER ) );

  if ( _s.InvalidateWindow )
  {
    HB_GT_FUNC( gt_DispEnd() );
  }
  else
  {
    RECT cr, crInvalid;

    cr.left   = usLeft   + ( iCols>0 ? 1 : 0 ) ;
    cr.top    = usTop    + ( iRows>0 ? 1 : 0 ) ;
    cr.right  = usRight  - ( iCols<0 ? 1 : 0 ) ;
    cr.bottom = usBottom - ( iRows<0 ? 1 : 0 ) ;

    cr = hb_wvt_gtGetXYFromColRowRect( cr );
    ScrollWindowEx( _s.hWnd, -iCols * _s.PTEXTSIZE.x, -iRows *_s.PTEXTSIZE.y, &cr, NULL, NULL, &crInvalid, 0 ) ;
    InvalidateRect( _s.hWnd, &crInvalid, FALSE );
    _s.InvalidateWindow = TRUE ;
  }
  if ( bMalloc )
  {
    hb_xfree( fpBlank );
    hb_xfree( fpBuff );
  }
#ifdef WVT_DEBUG
  nCountScroll++;
#endif
}

//-------------------------------------------------------------------//
//
//    resize the ( existing ) window
//
BOOL HB_GT_FUNC( gt_SetMode( USHORT row, USHORT col ) )
{
   BOOL bResult= FALSE;
   HFONT hFont;

   HB_TRACE( HB_TR_DEBUG, ( "hb_gt_SetMode( %hu, %hu )", row, col ) );

   if ( row<= WVT_MAX_ROWS && col<= WVT_MAX_COLS )
   {
      // Is the window already open
      if ( _s.hWnd )
      {
         hFont = hb_wvt_gtGetFont( _s.fontFace, _s.fontHeight, _s.fontWidth, _s.fontWeight, _s.fontQuality, _s.CodePage );
         if ( hFont )
         {
            // make sure that the mode selected along with the current
            // font settings will fit in the window
            if ( hb_wvt_gtValidWindowSize( row,col, hFont, _s.fontWidth ) )
            {
                bResult = hb_wvt_gtInitWindow( _s.hWnd, col, row );
            }
            DeleteObject( hFont );
         }
      }
      else
      {
         hb_wvt_gtAllocSpBuffer( row, col );
      }
   }
   return( bResult );
}

//-------------------------------------------------------------------//

BOOL HB_GT_FUNC( gt_GetBlink() )
{
  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_GetBlink()" ) );
  return( TRUE );
}

//-------------------------------------------------------------------//

void HB_GT_FUNC( gt_SetBlink( BOOL bBlink ) )
{
  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_SetBlink( %d )", ( int ) bBlink ) );
  HB_SYMBOL_UNUSED( bBlink );
}

//-------------------------------------------------------------------//

char * HB_GT_FUNC(gt_Version( int iType ))
{
   HB_TRACE( HB_TR_DEBUG, ( "hb_gt_Version()" ) );

   if ( iType == 0 )
      return HB_GT_DRVNAME( HB_GT_NAME );

  return "xHarbour Terminal: Win32 buffered WVT";
}

//-------------------------------------------------------------------//

static void HB_GT_FUNC( gt_xPutch( USHORT iRow, USHORT iCol, BYTE bAttr, BYTE bChar ) )
{
  USHORT index;
  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_xPutch( %hu, %hu, %d, %i )", iRow, iCol, ( int ) bAttr, bChar ) );

  index = hb_wvt_gtGetIndexForTextBuffer( iCol, iRow );
  if ( index < _s.BUFFERSIZE )
  {
    _s.pBuffer[ index ]     = bChar;
    _s.pAttributes[ index ] = bAttr;

    //  determine bounds of rect around character to refresh
    //
    hb_wvt_gtSetInvalidRect( iCol, iRow, iCol, iRow );
  }
}

//-------------------------------------------------------------------//
//
//    copied from gtwin
//
USHORT HB_GT_FUNC( gt_Box( SHORT Top, SHORT Left, SHORT Bottom, SHORT Right,
                          BYTE * szBox, BYTE byAttr ) )
{
    USHORT ret = 1;
    SHORT Row;
    SHORT Col;
    SHORT Height;
    SHORT Width;
    USHORT sWidth = _GetScreenWidth(),
          sHeight = _GetScreenHeight();

    if( ( Left   >= 0 && Left   < sWidth  ) ||
        ( Right  >= 0 && Right  < sWidth  ) ||
        ( Top    >= 0 && Top    < sHeight ) ||
        ( Bottom >= 0 && Bottom < sHeight ) )
    {
        /* Ensure that box is drawn from top left to bottom right. */
        if( Top > Bottom )
        {
            Row = Top;
            Top = Bottom;
            Bottom = Row;
        }

        if( Left > Right )
        {
            Row = Left;
            Left = Right;
            Right = Row;
        }

        /* Draw the box or line as specified */
        Height = Bottom - Top + 1;
        Width  = Right - Left + 1;

        HB_GT_FUNC( gt_DispBegin() );

        if( Height > 1 && Width > 1 && Top >= 0 && Top < sHeight && Left >= 0 && Left < sWidth )
        {
           HB_GT_FUNC( gt_xPutch( Top, Left, byAttr, szBox[ 0 ] ) ); /* Upper left corner */
        }

        Col = ( Width > 1 ? Left + 1 : Left );

        if( Col < 0 )
        {
            Width += Col;
            Col = 0;
        }

        if( Right >= sWidth )
        {
            Width -= Right - sWidth;
        }

        if( Col < Right && Col < sWidth && Top >= 0 && Top < sHeight )
        {
            HB_GT_FUNC( gt_Replicate( Top, Col, byAttr, szBox[ 1 ], Width + ( (Right - Left) > 1 ? -2 : 0 ) )); /* Top line */
        }

        if( Height > 1 && (Right - Left) > 0 && Right < sWidth && Top >= 0 && Top < sHeight )
        {
            HB_GT_FUNC( gt_xPutch( Top, Right, byAttr, szBox[ 2 ] ) ); /* Upper right corner */
        }

        if( szBox[ 8 ] && Height > 2 && Width > 2 )
        {
            for( Row = Top + 1; Row < Bottom; Row++ )
            {
                if( Row >= 0 && Row < sHeight )
                {
                    Col = Left;

                    if( Col < 0 )
                    {
                        Col = 0; /* The width was corrected earlier. */
                    }
                    else
                    {
                        HB_GT_FUNC( gt_xPutch( Row, Col++, byAttr, szBox[ 7 ] ) ); /* Left side */
                    }

                    HB_GT_FUNC( gt_Replicate( Row, Col, byAttr, szBox[ 8 ], Width - 2 ) ); /* Fill */

                    if( Right < sWidth )
                    {
                       HB_GT_FUNC(gt_xPutch( Row, Right, byAttr, szBox[ 3 ] )); /* Right side */
                    }
                }
            }
        }
        else
        {
            for( Row = ( Width > 1 ? Top + 1 : Top ); Row < ( (Right - Left ) > 1 ? Bottom : Bottom + 1 ); Row++ )
            {
                if( Row >= 0 && Row < sHeight )
                {
                    if( Left >= 0 && Left < sWidth )
                    {
                        HB_GT_FUNC(gt_xPutch( Row, Left, byAttr, szBox[ 7 ] )); /* Left side */
                    }

                    if( ( Width > 1 || Left < 0 ) && Right < sWidth )
                    {
                        HB_GT_FUNC(gt_xPutch( Row, Right, byAttr, szBox[ 3 ] )); /* Right side */
                    }
                }
            }
        }

        if( Height > 1 && Width > 1 )
        {
            if( Left >= 0 && Bottom < sHeight )
            {
                HB_GT_FUNC(gt_xPutch( Bottom, Left, byAttr, szBox[ 6 ] )); /* Bottom left corner */
            }

            Col = Left + 1;

            if( Col < 0 )
            {
                Col = 0; /* The width was corrected earlier. */
            }

            if( Col <= Right && Bottom < sHeight )
            {
                HB_GT_FUNC(gt_Replicate( Bottom, Col, byAttr, szBox[ 5 ], Width - 2 )); /* Bottom line */
            }

            if( Right < sWidth && Bottom < sHeight )
            {
                HB_GT_FUNC(gt_xPutch( Bottom, Right, byAttr, szBox[ 4 ] )); /* Bottom right corner */
            }
        }

        HB_GT_FUNC(gt_DispEnd());

        ret = 0;
    }

    return ret;
}

//-------------------------------------------------------------------//
//
//   copied from gtwin
//
USHORT HB_GT_FUNC( gt_BoxD( SHORT Top, SHORT Left, SHORT Bottom, SHORT Right, BYTE * pbyFrame, BYTE byAttr ) )
{
    return( HB_GT_FUNC( gt_Box( Top, Left, Bottom, Right, pbyFrame, byAttr ) ) );
}

//-------------------------------------------------------------------//
//
//   copied from gtwin
//
USHORT HB_GT_FUNC( gt_BoxS( SHORT Top, SHORT Left, SHORT Bottom, SHORT Right, BYTE * pbyFrame, BYTE byAttr ) )
{
    return( HB_GT_FUNC( gt_Box( Top, Left, Bottom, Right, pbyFrame, byAttr ) ) );
}

//-------------------------------------------------------------------//
//
//   copied from gtwin
//
USHORT HB_GT_FUNC( gt_HorizLine( SHORT Row, SHORT Left, SHORT Right, BYTE byChar, BYTE byAttr ) )
{
  USHORT ret    = 1;
  USHORT sWidth = _GetScreenWidth();

  if( Row >= 0 && Row < sWidth )
  {
      if( Left < 0 )
      {
          Left = 0;
      }
      else if( Left >= sWidth )
      {
          Left = sWidth - 1;
      }
      if( Right < 0 )
      {
          Right = 0;
      }
      else if( Right >= sWidth )
      {
          Right = sWidth - 1;
      }
      if( Left < Right )
      {
          HB_GT_FUNC( gt_Replicate( Row, Left, byAttr, byChar, Right - Left + 1 ) );
      }
      else
      {
          HB_GT_FUNC( gt_Replicate( Row, Right, byAttr, byChar, Left - Right + 1 ) );
      }
      ret = 0;
  }
  return( ret );
}

//-------------------------------------------------------------------//
//
//   copied from gtwin
//
USHORT HB_GT_FUNC( gt_VertLine( SHORT Col, SHORT Top, SHORT Bottom, BYTE byChar, BYTE byAttr ) )
{
    USHORT ret     = 1;
    USHORT sWidth  = _GetScreenWidth();
    USHORT sHeight = _GetScreenHeight();
    SHORT  Row;

    if( Col >= 0 && Col < sWidth )
    {
        if( Top < 0 )
        {
            Top = 0;
        }
        else if( Top >= sHeight )
        {
            Top = sHeight - 1;
        }
        if( Bottom < 0 )
        {
            Bottom = 0;
        }
        else if( Bottom >= sHeight )
        {
            Bottom = sHeight - 1;
        }
        if( Top <= Bottom )
        {
            Row = Top;
        }
        else
        {
            Row    = Bottom;
            Bottom = Top;
        }

        HB_GT_FUNC( gt_DispBegin() );

        while( Row <= Bottom )
        {
            HB_GT_FUNC( gt_xPutch( Row++, Col, byAttr, byChar ) );
        }
        HB_GT_FUNC( gt_DispEnd() );

        ret = 0;
    }
    return( ret );
}

//-------------------------------------------------------------------//
//
//    like gtwin
//
BOOL HB_GT_FUNC( gt_Suspend() )
{
  return( TRUE );
}

//-------------------------------------------------------------------//
//
//   like gtwin
//
BOOL HB_GT_FUNC( gt_Resume() )
{
  return( TRUE );
}

//-------------------------------------------------------------------//
//
//   like gtwin
//
BOOL HB_GT_FUNC( gt_PreExt() )
{
  return( TRUE );
}

//-------------------------------------------------------------------//
//
//   like gtwin
//
BOOL HB_GT_FUNC( gt_PostExt() )
{
  return( TRUE );
}

//-------------------------------------------------------------------//

void HB_GT_FUNC( gt_OutStd( BYTE * pbyStr, ULONG ulLen ) )
{
  hb_fsWriteLarge( s_iStdOut, ( BYTE * ) pbyStr, ulLen );
}

//-------------------------------------------------------------------//

void HB_GT_FUNC( gt_OutErr( BYTE * pbyStr, ULONG ulLen ) )
{
  hb_fsWriteLarge( s_iStdErr, ( BYTE * ) pbyStr, ulLen );
}

//-------------------------------------------------------------------//

int HB_GT_FUNC( gt_ExtendedKeySupport() )
{
    return( FALSE );  // Only use standard Clipper hey handling
}

//-------------------------------------------------------------------//

int HB_GT_FUNC( gt_ReadKey( HB_inkey_enum eventmask ) )
{
  int  c = 0;
  BOOL bKey;

  HB_TRACE( HB_TR_DEBUG, ( "hb_gt_ReadKey( %d )", ( int ) eventmask ) );

  HB_SYMBOL_UNUSED( eventmask );                  // we ignore the eventmask!

  hb_wvt_gtProcessMessages() ;
  bKey = hb_wvt_gtGetCharFromInputQueue( &c );

  return( bKey ? c : 0 );
}

//-------------------------------------------------------------------//
//
//   Copied from gtwin
//
#if defined( __BORLANDC__ ) || defined( _MSC_VER )
static int hb_Inp9x( USHORT usPort )
{
  USHORT usVal;

  HB_TRACE( HB_TR_DEBUG, ( "hb_Inp9x( %hu )", usPort ) );

  #if defined( __BORLANDC__ )
     _DX = usPort;
     __emit__( 0xEC );        /* ASM  IN AL, DX */
     __emit__( 0x32,0xE4 );   /* ASM XOR AH, AH */
     usVal = _AX;
  #else

     usVal = _inp( usPort );
  #endif

  return( usVal );
}

//-------------------------------------------------------------------//
//
//    Copied from gtwin
//
static int hb_Outp9x( USHORT usPort, USHORT usVal )
{
  HB_TRACE( HB_TR_DEBUG, ( "hb_Outp9x( %hu, %hu )", usPort, usVal ) );

  #if defined( __BORLANDC__ )
    _DX = usPort;
    _AL = usVal;
    __emit__( 0xEE );        /* ASM OUT DX, AL */
    __emit__( 0x32,0xE4 );   /* ASM XOR AH, AH */
    usVal = _AX;
  #else
     _outp( usPort, usVal );
  #endif

  return( usVal );
}

//-------------------------------------------------------------------//
//
//    Copied from gtwin
//
/* dDurat is in seconds */
static void HB_GT_FUNC(gt_w9xTone( double dFreq, double dDurat ))
{
    INT   uLSB,uMSB;
    ULONG lAdjFreq;

    HB_TRACE( HB_TR_DEBUG, ("hb_gt_w9xtone(%lf, %lf)", dFreq, dDurat ) );

    /* sync with internal clock with very small time period */
    hb_idleSleep( 0.01 );

    /* Clipper ignores Tone() requests (but delays anyway) if Frequency is
       less than < 20 hz (and so should we) to maintain compatibility .. */

    if ( dFreq >= 20.0 )
    {
      /* Setup Sound Control Port Registers and timer channel 2 */
      hb_Outp9x( 67, 182 ) ;

      lAdjFreq = ( ULONG ) ( 1193180 / dFreq ) ;

      if( ( LONG ) lAdjFreq < 0 )
         uLSB = lAdjFreq + 65536;
      else
         uLSB = lAdjFreq % 256;

      if( ( LONG ) lAdjFreq < 0 )
         uMSB = lAdjFreq + 65536;
      else
         uMSB = lAdjFreq / 256;


      /* set the frequency (LSB,MSB) */

      hb_Outp9x( 66, uLSB );
      hb_Outp9x( 66, uMSB );

      /* Get current Port setting */
      /* enable Speaker Data & Timer gate bits */
      /* (00000011B is bitmask to enable sound) */
      /* Turn on Speaker - sound Tone for duration.. */

      hb_Outp9x( 97, hb_Inp9x( 97 ) | 3 );

      hb_idleSleep( dDurat );

      /* Read back current Port value for Reset */
      /* disable Speaker Data & Timer gate bits */
      /* (11111100B is bitmask to disable sound) */
      /* Turn off the Speaker ! */

      hb_Outp9x( 97, hb_Inp9x( 97 ) & 0xFC );

    }
    else
    {
       hb_idleSleep( dDurat );
    }
}
#endif

//-------------------------------------------------------------------//
//
/* dDurat is in seconds */
//
static void HB_GT_FUNC( gt_wNtTone( double dFreq, double dDurat ) )
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_wNtTone(%lf, %lf)", dFreq, dDurat ) );

    /* Clipper ignores Tone() requests (but delays anyway) if Frequency is
       less than < 20 hz.  Windows NT minimum is 37... */

    /* sync with internal clock with very small time period */
    hb_idleSleep( 0.01 );

    if ( dFreq >= 37.0 )
    {
       Beep( (ULONG) dFreq, (ULONG) ( dDurat * 1000 ) ); /* Beep wants Milliseconds */
    }
    else
    {
       hb_idleSleep( dDurat );
    }
}

//-------------------------------------------------------------------//
//
/* dDuration is in 'Ticks' (18.2 per second) */
//
void HB_GT_FUNC( gt_Tone( double dFrequency, double dDuration ) )
{
    OSVERSIONINFO osv;

    HB_TRACE(HB_TR_DEBUG, ("hb_gt_Tone(%lf, %lf)", dFrequency, dDuration));

    /*
      According to the Clipper NG, the duration in 'ticks' is truncated to the
      interger portion  ... Depending on the platform, xHarbour allows a finer
      resolution, but the minimum is 1 tick (for compatibility)
     */
    /* Convert from ticks to seconds */
    dDuration  = ( HB_MIN( HB_MAX( 1.0, dDuration ), ULONG_MAX ) ) / 18.2;

    /* keep the frequency in an acceptable range */
    dFrequency =   HB_MIN( HB_MAX( 0.0, dFrequency ), 32767.0 );

    /* What version of Windows are you running? */
    osv.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
    GetVersionEx( &osv );

    /* If Windows 95 or 98, use w9xTone for BCC32, MSVC */
    if ( osv.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
    {
       #if defined( __BORLANDC__ ) || defined( _MSC_VER )
          HB_GT_FUNC( gt_w9xTone( dFrequency, dDuration ) );
       #else
          HB_GT_FUNC( gt_wNtTone( dFrequency, dDuration ) );
       #endif
    }

    /* If Windows NT or NT2k, use wNtTone, which provides TONE()
       reset sequence support (new) */
    else if ( osv.dwPlatformId == VER_PLATFORM_WIN32_NT )
    {
      HB_GT_FUNC( gt_wNtTone( dFrequency, dDuration ) );
    }
}

//-------------------------------------------------------------------//

void HB_GT_FUNC( mouse_Init( void ) )
{
  hb_wvt_gtSetMouseX( 0 );
  hb_wvt_gtSetMouseY( 0 );
}

//-------------------------------------------------------------------//

void HB_GT_FUNC( mouse_Exit( void ) )
{
}

//-------------------------------------------------------------------//

BOOL HB_GT_FUNC( mouse_IsPresent( void ) )
{
   return( TRUE );
}

//-------------------------------------------------------------------//

void HB_GT_FUNC( mouse_Show( void ) )
{
}

//-------------------------------------------------------------------//

void HB_GT_FUNC( mouse_Hide( void ) )
{
}

//-------------------------------------------------------------------//

int HB_GT_FUNC( mouse_Col( void ) )
{
  return( hb_wvt_gtGetMouseX() );
}

//-------------------------------------------------------------------//

int HB_GT_FUNC( mouse_Row( void ) )
{
  return( hb_wvt_gtGetMouseY() );
}

//-------------------------------------------------------------------//

void HB_GT_FUNC( mouse_SetPos( int iRow, int iCol ) )
{
  hb_wvt_gtSetMouseY( iRow );
  hb_wvt_gtSetMouseX( iCol );
}

//-------------------------------------------------------------------//

BOOL HB_GT_FUNC( mouse_IsButtonPressed( int iButton ) )
{
  BOOL bReturn = FALSE;

  if ( iButton == 0 )
  {
    bReturn = GetKeyState( VK_LBUTTON ) & 0x8000;
  }
  else if ( iButton== 1 )
  {
    bReturn = GetKeyState( VK_RBUTTON ) & 0x8000;
  }
  else if ( iButton == 2 )
  {
    bReturn = GetKeyState( VK_MBUTTON ) & 0x8000;
  }

  return( bReturn );
}

//-------------------------------------------------------------------//

int HB_GT_FUNC( mouse_CountButton( void ) )
{
  return( GetSystemMetrics( SM_CMOUSEBUTTONS ) ) ;
}

//-------------------------------------------------------------------//

void HB_GT_FUNC( mouse_SetBounds( int iTop, int iLeft, int iBottom, int iRight ) )
{
   HB_SYMBOL_UNUSED( iTop    );
   HB_SYMBOL_UNUSED( iLeft   );
   HB_SYMBOL_UNUSED( iBottom );
   HB_SYMBOL_UNUSED( iRight  );
}

//-------------------------------------------------------------------//

void HB_GT_FUNC( mouse_GetBounds( int * piTop, int * piLeft, int * piBottom, int * piRight ) )
{
   HB_SYMBOL_UNUSED( piTop    );
   HB_SYMBOL_UNUSED( piLeft   );
   HB_SYMBOL_UNUSED( piBottom );
   HB_SYMBOL_UNUSED( piRight  );
}

//-------------------------------------------------------------------//
//-------------------------------------------------------------------//
//-------------------------------------------------------------------//
//
//                    WVT specific functions
//
//-------------------------------------------------------------------//
//-------------------------------------------------------------------//
//-------------------------------------------------------------------//

static void hb_wvt_gtCreateObjects( void )
{
   LOGBRUSH lb;

   _s.penWhite     = CreatePen( PS_SOLID, 0, ( COLORREF ) RGB( 255,255,255 ) );
   _s.penBlack     = CreatePen( PS_SOLID, 0, ( COLORREF ) RGB(   0,  0,  0 ) );
   _s.penWhiteDim  = CreatePen( PS_SOLID, 0, ( COLORREF ) RGB( 205,205,205 ) );
   _s.penDarkGray  = CreatePen( PS_SOLID, 0, ( COLORREF ) RGB( 150,150,150 ) );
   _s.penGray      = CreatePen( PS_SOLID, 0, ( COLORREF ) _COLORS[ 7 ] );
   _s.penNull      = CreatePen( PS_NULL , 0, ( COLORREF ) _COLORS[ 7 ] );

   _s.currentPen   = CreatePen( PS_SOLID, 0, ( COLORREF ) RGB(   0,  0,  0 ) );

   lb.lbStyle      = BS_NULL;
   lb.lbColor      = RGB( 198,198,198 );
   lb.lbHatch      = 0;
   _s.currentBrush = CreateBrushIndirect( &lb );

   lb.lbStyle      = BS_HATCHED;
   lb.lbColor      = RGB( 210,210,210 );
   lb.lbHatch      = HS_DIAGCROSS; // HS_BDIAGONAL;
   _s.diagonalBrush = CreateBrushIndirect( &lb );

   lb.lbStyle      = BS_SOLID;
   lb.lbColor      = 0; // NULL;  // RGB( 0,0,0 );
   lb.lbHatch      = 0;
   _s.solidBrush = CreateBrushIndirect( &lb );

   lb.lbStyle      = BS_SOLID;
   lb.lbColor      = _COLORS[ 7 ];
   lb.lbHatch      = 0;
   _s.wvtWhiteBrush= CreateBrushIndirect( &lb );

}

//-------------------------------------------------------------------//

static USHORT hb_wvt_gtCalcPixelHeight( void )
{
  return( _s.PTEXTSIZE.y*_GetScreenHeight() );
}

//-------------------------------------------------------------------//

static USHORT hb_wvt_gtCalcPixelWidth( void )
{
  return( _s.PTEXTSIZE.x*_GetScreenWidth() );
}

//-------------------------------------------------------------------//

static BOOL hb_wvt_gtAllocSpBuffer( USHORT col, USHORT row )
{
  BOOL bRet = TRUE;

  _s.COLS        = col;
  _s.ROWS        = row;
  _s.BUFFERSIZE  = col * row * sizeof( char );
  _s.pBuffer     = _s.byBuffer ;
  _s.pAttributes = _s.byAttributes;
  memset( _s.pBuffer, ' ', _s.BUFFERSIZE );
  memset( _s.pAttributes,_s.background, _s.BUFFERSIZE );

  return( bRet );
}

//-------------------------------------------------------------------//

static BOOL hb_wvt_gtInitWindow( HWND hWnd, USHORT col, USHORT row )
{
  BOOL bRet = hb_wvt_gtAllocSpBuffer( col, row );

  hb_wvt_gtResetWindowSize( hWnd );

  return( bRet );
}

//-------------------------------------------------------------------//

static BOOL hb_wvt_gtValidWindowSize( int rows, int cols, HFONT hFont, int iWidth )
{
  HDC        hdc;
  HFONT      hOldFont ;
  USHORT     width, height, maxWidth, maxHeight;
  TEXTMETRIC tm;
  RECT       rcWorkArea;

  SystemParametersInfo( SPI_GETWORKAREA,0, &rcWorkArea, 0 );

  maxWidth  = (SHORT) ( rcWorkArea.right - rcWorkArea.left );
  maxHeight = (SHORT) ( rcWorkArea.bottom - rcWorkArea.top );

  hdc       = GetDC( _s.hWnd );
  hOldFont  = ( HFONT ) SelectObject( hdc, hFont );
  GetTextMetrics( hdc, &tm );
  SelectObject( hdc, hOldFont ); // Put old font back
  ReleaseDC( _s.hWnd, hdc );

  width     = iWidth < 0 ? -iWidth : tm.tmAveCharWidth * cols ;  // Total pixel width this setting would take
  height    = tm.tmHeight * rows;         // Total pixel height this setting would take

  return( ( width <= maxWidth ) && ( height <= maxHeight ) );
}

//-------------------------------------------------------------------//

static void hb_wvt_gtResetWindowSize( HWND hWnd )
{
  HDC        hdc;
  HFONT      hFont, hOldFont ;
  USHORT     diffWidth, diffHeight;
  USHORT     height, width;
  RECT       wi, ci;
  TEXTMETRIC tm;
  // HMENU      hMenu;
  RECT       rcWorkArea;
  int        n;

  // set the font and get it's size to determine the size of the client area
  // for the required number of rows and columns
  //
  hdc      = GetDC( hWnd );
  hFont    = hb_wvt_gtGetFont( _s.fontFace, _s.fontHeight, _s.fontWidth, _s.fontWeight, _s.fontQuality, _s.CodePage );
  _s.hFont = hFont ;
  hOldFont = ( HFONT ) SelectObject( hdc, hFont );
  if ( hOldFont )
  {
    DeleteObject( hOldFont );
  }
  GetTextMetrics( hdc, &tm );
  SetTextCharacterExtra( hdc,0 ); // do not add extra char spacing even if bold
  ReleaseDC( hWnd, hdc );

  // we will need to use the font size to handle the transformations from
  // row column space in the future, so we keep it around in a static!
  //

  _s.PTEXTSIZE.x = _s.fontWidth<0 ? -_s.fontWidth : tm.tmAveCharWidth; // For fixed FONT should == tm.tmMaxCharWidth
  _s.PTEXTSIZE.y = tm.tmHeight;       //     but seems to be a problem on Win9X so
                                      //     assume proportional fonts always for Win9X
  if (_s.fontWidth < 0 || _s.Win9X || ( tm.tmPitchAndFamily & TMPF_FIXED_PITCH ) || ( _s.PTEXTSIZE.x != tm.tmMaxCharWidth ) )
  {
    _s.FixedFont = FALSE;
  }
  else
  {
    _s.FixedFont = TRUE ;
  }

  for( n = 0 ; n < _GetScreenWidth() ; n++ ) // _s.FixedSize[] is used by ExtTextOut() to emulate
  {                             //          fixed font when a proportional font is used
    _s.FixedSize[ n ] = _s.PTEXTSIZE.x;
  }

  // resize the window to get the specified number of rows and columns
  //
  height = hb_wvt_gtCalcPixelHeight();
  width  = hb_wvt_gtCalcPixelWidth();

  GetWindowRect( hWnd, &wi );
  GetClientRect( hWnd, &ci );

  diffWidth  = ( SHORT )( ( wi.right  - wi.left ) - ( ci.right  ) );
  diffHeight = ( SHORT )( ( wi.bottom - wi.top  ) - ( ci.bottom ) );
  width      += diffWidth ;
  height     += diffHeight;

  // Centre the window within the CLIENT area on the screen
  //                   but only if _s.CentreWindow == TRUE
  //
  if ( _s.CentreWindow && SystemParametersInfo( SPI_GETWORKAREA,0, &rcWorkArea, 0 ) )
  {
    wi.left = rcWorkArea.left + ( ( ( rcWorkArea.right-rcWorkArea.left ) - ( width  ) ) / 2 ) ;
    wi.top  = rcWorkArea.top  + ( ( ( rcWorkArea.bottom-rcWorkArea.top ) - ( height ) ) / 2 ) ;
  }
  SetWindowPos( hWnd, NULL, wi.left, wi.top, width, height, SWP_NOZORDER );

  if ( _s.bGui )
  {
     hb_wvt_gtInitGui();
  }
}
//-------------------------------------------------------------------//

static int hb_wvt_key_ansi_to_oem( int c )
{
   char pszAnsi[4];
   char pszOem[4];

   sprintf( pszAnsi, "%c", c );
   CharToOemBuff( ( LPCSTR ) pszAnsi, ( LPTSTR ) pszOem, 1 );
   c = (BYTE) * pszOem;

   return c;
}

static void hb_wvt_gtInitGui( void )
{
  _s.iGuiWidth  = _GetScreenWidth() * _s.PTEXTSIZE.x ;
  _s.iGuiHeight = _GetScreenHeight() * _s.PTEXTSIZE.y ;

  if ( _s.hGuiDC )
  {
     DeleteDC( _s.hGuiDC );
  }
  _s.hGuiDC = CreateCompatibleDC( _s.hdc );

  if ( _s.hGuiBmp )
  {
     DeleteObject( _s.hGuiBmp );
  }
  _s.hGuiBmp = CreateCompatibleBitmap( _s.hdc, _s.iGuiWidth, _s.iGuiHeight );

  SelectObject( _s.hGuiDC, _s.hGuiBmp );
  SetTextCharacterExtra( _s.hGuiDC,0 );
  SelectObject( _s.hGuiDC, _s.hFont );
}

//-------------------------------------------------------------------//
//
/* JC1: rendering of graphical objects */
//
static void s_wvt_paintGraphicObjects( HDC hdc, RECT *updateRect )
{
   HB_GT_GOBJECT *pObj;
   COLORREF      color;
   HPEN          hPen, hOldPen;
   HBRUSH        hBrush, hOldBrush;

   pObj = hb_gt_gobjects;

   while ( pObj )
   {
      /* Check if pObj boundaries are inside the area to be updated */
      if ( hb_gtGobjectInside( pObj, updateRect->left, updateRect->top,
                              updateRect->right, updateRect->bottom ) )
      {
         color     = RGB( pObj->color.usRed >> 8, pObj->color.usGreen >> 8, pObj->color.usBlue >> 8 );
         hPen      = CreatePen( PS_SOLID, 1, color );
         hOldPen   = ( HPEN   ) SelectObject( hdc, hPen );
         hBrush    = ( HBRUSH ) CreateSolidBrush( color );
         hOldBrush = ( HBRUSH ) SelectObject( hdc, hBrush );

         switch( pObj->type )
         {
            case GTO_POINT:
               MoveToEx( hdc, pObj->x, pObj->y, NULL );
               LineTo( hdc, pObj->x, pObj->y );
            break;

            case GTO_LINE:
               /* For lines, width and height represent X2, Y2 */
               MoveToEx( hdc, pObj->x, pObj->y, NULL );
               LineTo( hdc, pObj->width, pObj->height );
            break;

            case GTO_SQUARE:
            {
               RECT r;
               r.left  = pObj->x;
               r.top   = pObj->y;
               r.right = pObj->x + pObj->width;
               r.bottom= pObj->y + pObj->height;

               FrameRect( hdc, &r, hBrush );
            }
            break;

            case GTO_RECTANGLE:
               /* For lines, width and height represent X2, Y2 */
               Rectangle( hdc,
                  pObj->x, pObj->y,
                  pObj->x + pObj->width, pObj->y + pObj->height );
            break;


            case GTO_CIRCLE:
               Arc( hdc,
                  pObj->x, pObj->y,
                  pObj->x + pObj->width, pObj->y + pObj->height,
                  0,0,0,0 );
            break;

            case GTO_DISK:
               Ellipse( hdc,
                  pObj->x, pObj->y,
                  pObj->x + pObj->width, pObj->y + pObj->height );
            break;

            case GTO_TEXT:
               TextOut( hdc,
                  pObj->x, pObj->y,
                  pObj->data, pObj->data_len );
            break;
         }
         SelectObject( hdc, hOldPen );
         SelectObject( hdc, hOldBrush );
         DeleteObject( hBrush );
         DeleteObject( hPen );
      }

      pObj = pObj->next;
   }
}

//-------------------------------------------------------------------//

static LRESULT CALLBACK hb_wvt_gtWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
  static BOOL bIgnoreWM_SYSCHAR = FALSE ;
  static BOOL bPaint     = FALSE;
  static BOOL bGetFocus  = FALSE;
  static BOOL bSetFocus  = FALSE;
  static BOOL bKillFocus = FALSE;

  BOOL        bRet;

  switch ( message )
  {
    case WM_CREATE:
    {
      bRet = hb_wvt_gtInitWindow( hWnd, WVT_DEFAULT_COLS, WVT_DEFAULT_ROWS );
      return( bRet );
    }

    case WM_COMMAND:
    {
      hb_wvt_gtHandleMenuSelection( ( int ) LOWORD( wParam ) );
      return( 0 );
    }

    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC         hdc;
      USHORT      irow;
      RECT        updateRect, rcRect;

      GetUpdateRect( hWnd, &updateRect, FALSE );
      /* WARNING!!!
       * the GetUpdateRect call MUST be made BEFORE the BeginPaint call, since
       * BeginPaint resets the update rectangle - don't move it or nothing is drawn!
       */
      if ( _s.bGui && bKillFocus )
      {
         hb_wvt_gtRestGuiState( &updateRect );
         ValidateRect( hWnd, &updateRect );
         return( 0 );
      }
      if ( _s.bGui && bSetFocus )
      {
         bSetFocus  = FALSE;
         hb_wvt_gtRestGuiState( &updateRect );
         ValidateRect( hWnd, &updateRect );
         return( 0 );
      }

      hdc = BeginPaint( hWnd, &ps );
      SelectObject( hdc, _s.hFont );
      if ( _s.bGui )
      {
         SelectObject( _s.hGuiDC, _s.hFont );
      }

      if ( _s.pBuffer != NULL && _s.pAttributes != NULL )
      {
        rcRect = hb_wvt_gtGetColRowFromXYRect( updateRect );

        _s.rowStart = rcRect.top    ;
        _s.rowStop  = rcRect.bottom ;
        _s.colStart = rcRect.left   ;
        _s.colStop  = rcRect.right  ;

        for ( irow = _s.rowStart; irow <=  _s.rowStop; irow++ )
        {
          USHORT icol, index, startIndex, startCol, len;
          BYTE   oldAttrib, attrib;

          icol       = _s.colStart;
          index      = hb_wvt_gtGetIndexForTextBuffer( icol, irow );
          startIndex = index;
          startCol   = icol;
          len        = 0;
          oldAttrib  = *( _s.pAttributes + index );

          /* attribute may change mid line...
          * so buffer up text with same attrib, and output it
          * then do next section with same attrib, etc
          */
          while ( icol <= _s.colStop )
          {
            if ( index >= _s.BUFFERSIZE )
            {
              break;
            }
            attrib = *( _s.pAttributes + index );
            if ( attrib != oldAttrib )
            {
              hb_wvt_gtSetColors( hdc, oldAttrib );
              hb_wvt_gtTextOut( hdc, startCol, irow, ( char const * ) _s.pBuffer + startIndex, len );
              if ( _s.bGui )
              {
                hb_wvt_gtSetColors( _s.hGuiDC, oldAttrib );
                hb_wvt_gtTextOut( _s.hGuiDC, startCol, irow, ( char const * ) _s.pBuffer + startIndex, len );
              }
              oldAttrib  = attrib;
              startIndex = index;
              startCol   = icol;
              len        = 0;
            }
            icol++;
            len++;
            index++;
          }
          hb_wvt_gtSetColors( hdc, oldAttrib );
          hb_wvt_gtTextOut( hdc, startCol, irow, ( char const * ) _s.pBuffer + startIndex, len );
          if ( _s.bGui )
          {
            hb_wvt_gtSetColors( _s.hGuiDC, oldAttrib );
            hb_wvt_gtTextOut( _s.hGuiDC, startCol, irow, ( char const * ) _s.pBuffer + startIndex, len );
          }
        }
      }

      if ( hb_gt_gobjects != NULL )
      {
         s_wvt_paintGraphicObjects( hdc, &updateRect );
      }

      EndPaint( hWnd, &ps );

      if ( bPaint )
      {
        if ( _s.pSymWVT_PAINT )
        {
           hb_vmPushSymbol( _s.pSymWVT_PAINT->pSymbol );
           hb_vmPushNil();
           hb_vmDo( 0 );
           hb_itemGetNL( ( PHB_ITEM ) &HB_VM_STACK.Return );
        }
      }
      else
      {
        bPaint = TRUE;
      }
#ifdef WVT_DEBUG
  printf( "\nPuts( %d ), Scroll( %d ), Paint( %d ), SetFocus( %d ), KillFocus( %d ) ",nCountPuts, nCountScroll, ++nCountPaint, nSetFocus, nKillFocus ) ;
#endif
      return( 0 );
    }

    case WM_MY_UPDATE_CARET:
    {
      hb_wvt_gtSetCaretPos();
      return( 0 );
    }

    case WM_SETFOCUS:
    {
#ifdef WVT_DEBUG
  nSetFocus++;
#endif
      if ( _s.bGui )
      {
         bSetFocus  = TRUE ;
         bKillFocus = FALSE;
      }

      hb_wvt_gtCreateCaret() ;

      if ( bGetFocus )
      {
        if ( _s.pSymWVT_SETFOCUS )
        {
          hb_vmPushSymbol( _s.pSymWVT_SETFOCUS->pSymbol );
          hb_vmPushNil();
          hb_vmPushLong( ( LONG ) hWnd    );
          hb_vmDo( 1 );
          hb_itemGetNL( ( PHB_ITEM ) &HB_VM_STACK.Return );
        }
      }
      else
      {
        bGetFocus = TRUE;
      }
      return( 0 );
    }

    case WM_KILLFOCUS:
    {
#ifdef WVT_DEBUG
  nKillFocus++;
#endif
      if ( _s.bGui )
      {
         bKillFocus = TRUE;
      }
      hb_wvt_gtKillCaret();

      if ( _s.pSymWVT_KILLFOCUS )
      {
        hb_vmPushSymbol( _s.pSymWVT_KILLFOCUS->pSymbol );
        hb_vmPushNil();
        hb_vmPushLong( ( LONG ) hWnd );
        hb_vmDo( 1 );
        hb_itemGetNL( ( PHB_ITEM ) &HB_VM_STACK.Return );
      }
      return( 0 );
    }

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
      BOOL bAlt         = GetKeyState( VK_MENU ) & 0x8000;
      bIgnoreWM_SYSCHAR = FALSE;

      switch ( wParam )
      {

        case VK_LEFT:
          hb_wvt_gtTranslateKey( K_LEFT , K_SH_LEFT , K_ALT_LEFT , K_CTRL_LEFT  );
          break;
        case VK_RIGHT:
          hb_wvt_gtTranslateKey( K_RIGHT, K_SH_RIGHT, K_ALT_RIGHT, K_CTRL_RIGHT );
          break;
        case VK_UP:
          hb_wvt_gtTranslateKey( K_UP   , K_SH_UP   , K_ALT_UP   , K_CTRL_UP    );
          break;
        case VK_DOWN:
          hb_wvt_gtTranslateKey( K_DOWN , K_SH_DOWN , K_ALT_DOWN , K_CTRL_DOWN  );
          break;
        case VK_HOME:
          hb_wvt_gtTranslateKey( K_HOME , K_SH_HOME , K_ALT_HOME , K_CTRL_HOME  );
          break;
        case VK_END:
          hb_wvt_gtTranslateKey( K_END  , K_SH_END  , K_ALT_END  , K_CTRL_END   );
          break;
        case VK_DELETE:
          hb_wvt_gtTranslateKey( K_DEL  , K_SH_DEL  , K_ALT_DEL  , K_CTRL_DEL   );
          break;
        case VK_INSERT:
          hb_wvt_gtTranslateKey( K_INS  , K_SH_INS  , K_ALT_INS  , K_CTRL_INS   );
          break;
        case VK_PRIOR:
          hb_wvt_gtTranslateKey( K_PGUP , K_SH_PGUP , K_ALT_PGUP , K_CTRL_PGUP  );
          break;
        case VK_NEXT:
          hb_wvt_gtTranslateKey( K_PGDN , K_SH_PGDN , K_ALT_PGDN , K_CTRL_PGDN  );
          break;

        case VK_F1:
          hb_wvt_gtTranslateKey( K_F1   , K_SH_F1, K_ALT_F1   , K_CTRL_F1    );
          break;
        case VK_F2:
          hb_wvt_gtTranslateKey( K_F2   , K_SH_F2, K_ALT_F2   , K_CTRL_F2    );
          break;
        case VK_F3:
          hb_wvt_gtTranslateKey( K_F3   , K_SH_F3, K_ALT_F3   , K_CTRL_F3    );
          break;
        case VK_F4:
        {
          if ( _s.AltF4Close && bAlt )
          {
            return( DefWindowProc( hWnd, message, wParam, lParam ) );
          }
          else
          {
            hb_wvt_gtTranslateKey( K_F4 , K_SH_F4, K_ALT_F4   , K_CTRL_F4    );
          }
          break;
        }
        case VK_F5:
          hb_wvt_gtTranslateKey( K_F5   , K_SH_F5, K_ALT_F5   , K_CTRL_F5    );
          break;
        case VK_F6:
          hb_wvt_gtTranslateKey( K_F6   , K_SH_F6, K_ALT_F6   , K_CTRL_F6    );
          break;
        case VK_F7:
          hb_wvt_gtTranslateKey( K_F7   , K_SH_F7, K_ALT_F7   , K_CTRL_F7    );
          break;
        case VK_F8:
          hb_wvt_gtTranslateKey( K_F8   , K_SH_F8, K_ALT_F8   , K_CTRL_F8    );
          break;
        case VK_F9:
          hb_wvt_gtTranslateKey( K_F9   , K_SH_F9, K_ALT_F9   , K_CTRL_F9    );
          break;
        case VK_F10:
          hb_wvt_gtTranslateKey( K_F10  , K_SH_F10,K_ALT_F10  , K_CTRL_F10   );
          break;
        case VK_F11:
          hb_wvt_gtTranslateKey( K_F11  , K_SH_F11,K_ALT_F11  , K_CTRL_F11   );
          break;
        case VK_F12:
          hb_wvt_gtTranslateKey( K_F12  , K_SH_F12,K_ALT_F12  , K_CTRL_F12   );
          break;
        default:
        {
          BOOL bCtrl     = GetKeyState( VK_CONTROL ) & 0x8000;
          BOOL bShift    = GetKeyState( VK_SHIFT ) & 0x8000;
          int  iScanCode = HIWORD( lParam ) & 0xFF ;

          if ( bCtrl && iScanCode == 76 ) // CTRL_VK_NUMPAD5 )
          {
            hb_wvt_gtAddCharToInputQueue( KP_CTRL_5 );
          }
          else if ( bCtrl && wParam == VK_TAB ) // K_CTRL_TAB
          {
             if ( bShift )
             {
                hb_wvt_gtAddCharToInputQueue( K_CTRL_SH_TAB );
             }
             else
             {
                hb_wvt_gtAddCharToInputQueue( K_CTRL_TAB );
             }
          }
          else if ( iScanCode == 70 ) // Ctrl_Break key OR Scroll Lock Key
          {
            if ( bCtrl )  // Not scroll lock
            {
              hb_wvt_gtAddCharToInputQueue( HB_BREAK_FLAG ); // Pretend Alt+C pressed
              bIgnoreWM_SYSCHAR = TRUE;
            }
            else
            {
              DefWindowProc( hWnd, message, wParam, lParam ) ;  // Let windows handle ScrollLock
            }
          }
          else if ( bCtrl && iScanCode == 53 && bShift )
          {
            hb_wvt_gtAddCharToInputQueue( K_CTRL_QUESTION );
          }
          else if ( ( bAlt || bCtrl ) && (
              wParam == VK_MULTIPLY || wParam == VK_ADD || wParam == VK_SUBTRACT || wParam == VK_DIVIDE ) )
          {
            if ( bAlt )
            {
              bIgnoreWM_SYSCHAR = TRUE;
            }
            switch ( wParam )
            {
              case VK_MULTIPLY:
                hb_wvt_gtTranslateKey( '*','*', KP_ALT_ASTERISK, KP_CTRL_ASTERISK );
                break;
              case VK_ADD:
                hb_wvt_gtTranslateKey( '+','+', KP_ALT_PLUS, KP_CTRL_PLUS );
                break;
              case VK_SUBTRACT:
                hb_wvt_gtTranslateKey( '-','-', KP_ALT_MINUS, KP_CTRL_MINUS );
                break;
              case VK_DIVIDE:
                hb_wvt_gtTranslateKey( '/','/', KP_ALT_SLASH, KP_CTRL_SLASH );
                break;
            }
          }
          else if ( _s.EnableShortCuts )
          {
            return( DefWindowProc( hWnd, message, wParam, lParam ) );
          }
        }
      }
      return( 0 );
    }

    case WM_CHAR:
    {
      BOOL bCtrl     = GetKeyState( VK_CONTROL ) & 0x8000;
      int  iScanCode = HIWORD( lParam ) & 0xFF ;
      int  c = ( int ) wParam;

      if ( !bIgnoreWM_SYSCHAR )
      {
        if ( bCtrl && iScanCode == 28 )  // K_CTRL_RETURN
        {
          hb_wvt_gtAddCharToInputQueue( K_CTRL_RETURN );
        }
        else if ( bCtrl && ( c >= 1 && c <= 26 ) )  // K_CTRL_A - Z
        {
          hb_wvt_gtAddCharToInputQueue( K_Ctrl[c-1]  );
        }
        else
        {
          switch ( c )
          {
            // handle special characters
            case VK_BACK:
              hb_wvt_gtTranslateKey( K_BS, K_SH_BS, K_ALT_BS, K_CTRL_BS );
              break;
            case VK_TAB:
              hb_wvt_gtTranslateKey( K_TAB, K_SH_TAB, K_ALT_TAB, K_CTRL_TAB );
              break;
            case VK_RETURN:
              hb_wvt_gtTranslateKey( K_RETURN, K_SH_RETURN, K_ALT_RETURN, K_CTRL_RETURN );
              break;
            case VK_ESCAPE:
              hb_wvt_gtAddCharToInputQueue( K_ESC );
              break;
            default:
              if( _s.CodePage == OEM_CHARSET )
              {
                 c = hb_wvt_key_ansi_to_oem( c );
              }
              hb_wvt_gtAddCharToInputQueue( c );
              break;
          }
        }
      }
      bIgnoreWM_SYSCHAR = FALSE;    // As Suggested by Peter
      return( 0 );
    }

    case WM_SYSCHAR:
    {
      if ( !bIgnoreWM_SYSCHAR )
      {
        int c, iScanCode = HIWORD( lParam ) & 0xFF ;
        switch ( iScanCode )
        {
          case  2:
            c = K_ALT_1 ;
            break;
          case  3:
            c = K_ALT_2 ;
            break;
          case  4:
            c = K_ALT_3 ;
            break;
          case  5:
            c = K_ALT_4 ;
            break;
          case  6:
            c = K_ALT_5 ;
            break;
          case  7:
            c = K_ALT_6 ;
            break;
          case  8:
            c = K_ALT_7 ;
            break;
          case  9:
            c = K_ALT_8 ;
            break;
          case 10:
            c = K_ALT_9 ;
            break;
          case 11:
            c = K_ALT_0 ;
            break;
          case 13:
            c = K_ALT_EQUALS ;
            break;
          case 14:
            c = K_ALT_BS ;
            break;
          case 16:
            c = K_ALT_Q ;
            break;
          case 17:
            c = K_ALT_W ;
            break;
          case 18:
            c = K_ALT_E ;
            break;
          case 19:
            c = K_ALT_R ;
            break;
          case 20:
            c = K_ALT_T ;
            break;
          case 21:
            c = K_ALT_Y ;
            break;
          case 22:
            c = K_ALT_U ;
            break;
          case 23:
            c = K_ALT_I ;
            break;
          case 24:
            c = K_ALT_O ;
            break;
          case 25:
            c = K_ALT_P ;
            break;
          case 30:
            c = K_ALT_A ;
            break;
          case 31:
            c = K_ALT_S ;
            break;
          case 32:
            c = K_ALT_D ;
            break;
          case 33:
            c = K_ALT_F ;
            break;
          case 34:
            c = K_ALT_G ;
            break;
          case 35:
            c = K_ALT_H ;
            break;
          case 36:
            c = K_ALT_J ;
            break;
          case 37:
            c = K_ALT_K ;
            break;
          case 38:
            c = K_ALT_L ;
            break;
          case 44:
            c = K_ALT_Z ;
            break;
          case 45:
            c = K_ALT_X ;
            break;
          case 46:
            c = K_ALT_C ;
            break;
          case 47:
            c = K_ALT_V ;
            break;
          case 48:
            c = K_ALT_B ;
            break;
          case 49:
            c = K_ALT_N ;
            break;
          case 50:
            c = K_ALT_M ;
            break;
          default:
            c = ( int ) wParam ;
            break;
        }
        hb_wvt_gtAddCharToInputQueue( c );

      }
      bIgnoreWM_SYSCHAR = FALSE;
      return( 0 );
    }

    case WM_QUERYENDSESSION: // Closing down computer
    {
      /* if we have set a shutdown command return false,
       * so windows ( and our app )doesn't shutdown
       * otherwise let the default handler take it
       */
      if ( hb_gtHandleShutdown() )
      {
         return 0;
      }
      break;
    }

    case WM_CLOSE:  // Clicked 'X' on system menu
    {
      /* if an event has been set then return it otherwise
         fake an Alt+C
      */
      hb_gtHandleClose();
      return( 0 );
    }

    case WM_QUIT:
    case WM_DESTROY:
      return( 0 );

    case WM_RBUTTONDOWN:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_LBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
    case WM_MOUSEMOVE:
    case WM_MOUSEWHEEL:
    case WM_NCMOUSEMOVE:
    {
       hb_wvt_gtMouseEvent( hWnd, message, wParam, lParam );
       return( 0 );
    }
/*
    case WM_TIMER:
    {
       if ( _s.pSymWVT_TIMER )
       {
          hb_vmPushSymbol( _s.pSymWVT_TIMER->pSymbol );
          hb_vmPushNil();
          hb_vmDo( 0 );
          hb_itemGetNL( ( PHB_ITEM ) &HB_VM_STACK.Return );
       }
       return( 0 );
    }
*/
  }
  return( DefWindowProc( hWnd, message, wParam, lParam ) );
}

//-------------------------------------------------------------------//

static void hb_wvt_gtRestGuiState( LPRECT rect )
{
   BitBlt( _s.hdc, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top,
                                                 _s.hGuiDC, rect->left, rect->top, SRCCOPY );
}

//-------------------------------------------------------------------//

static HWND hb_wvt_gtCreateWindow( HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow )
{
   HWND     hWnd;
  WNDCLASS wndclass;

  HB_SYMBOL_UNUSED( hPrevInstance );
  HB_SYMBOL_UNUSED( szCmdLine );

  InitCommonControls();

  wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS ;
  wndclass.lpfnWndProc   = hb_wvt_gtWndProc;
  wndclass.cbClsExtra    = 0;
  wndclass.cbWndExtra    = 0;
  wndclass.hInstance     = hInstance;
  wndclass.hIcon         = NULL;
  wndclass.hCursor       = LoadCursor( NULL, IDC_ARROW );
  wndclass.hbrBackground = NULL;
  wndclass.lpszMenuName  = NULL;
  wndclass.lpszClassName = szAppName;

  if ( ! RegisterClass( &wndclass ) )
  {
    MessageBox( NULL, TEXT( "Failed to register class." ),
                szAppName, MB_ICONERROR );
    return( 0 );
  }

  hWnd = CreateWindow( szAppName,                         //classname
     TEXT( "XHARBOUR_WVT" ),                              //window name
     WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,  //style
     0,                                                   //x
     0,                                                   //y
     CW_USEDEFAULT,                                       //width
     CW_USEDEFAULT,                                       //height
     NULL,                                                //window parent
     NULL,                                                //menu
     hInstance,                                           //instance
     NULL );                                              //lpParam


  if ( hWnd == NULL )
  {
    MessageBox( NULL, TEXT( "Failed to create window." ),
                  TEXT( "XHARBOUR_WVT" ), MB_ICONERROR );
  }

  // If you wish to show window the way you want, put somewhere in your application
  // ANNOUNCE HB_NOSTARTUPWINDOW
  // If so compiled, then you need to issue Wvt_ShowWindow( SW_RESTORE )
  // at the point you desire in your code.
  //

  if ( hb_dynsymFind( "HB_NOSTARTUPWINDOW" ) != NULL )
  {
     iCmdShow = SW_HIDE;
  }

  ShowWindow( hWnd, iCmdShow );
  UpdateWindow( hWnd );

  return( hWnd ) ;
}

//-------------------------------------------------------------------//

static void hb_wvt_gtCreateToolTipWindow( void )
{
   INITCOMMONCONTROLSEX icex;
   HWND                 hwndTT;
   TOOLINFO             ti;

   // Load the tooltip class from the DLL.
   //
   icex.dwSize = sizeof( icex );
   icex.dwICC  = ICC_BAR_CLASSES;

   if( !InitCommonControlsEx( &icex ) )
   {
      return;
   }

   // Create the tooltip control.
   //
   hwndTT = CreateWindow( TOOLTIPS_CLASS, TEXT( "" ),
                          WS_POPUP | TTS_ALWAYSTIP ,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          NULL,
                          ( HMENU ) NULL,
                          ( HINSTANCE ) hb_hInstance,
                          NULL );

   SetWindowPos( hwndTT,
                 HWND_TOPMOST,
                 0,
                 0,
                 0,
                 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );

   // Prepare TOOLINFO structure for use as tracking tooltip.
   //
   ti.cbSize    = sizeof( TOOLINFO );
   ti.uFlags    = TTF_SUBCLASS;
   ti.hwnd      = _s.hWnd;
   ti.uId       = 100000;
   ti.hinst     = ( HINSTANCE ) hb_hInstance;
   ti.lpszText  = "";
   ti.rect.left = ti.rect.top = ti.rect.bottom = ti.rect.right = 0;

   // Add the tool to the control, displaying an error if needed.
   //
   if( ! SendMessage( hwndTT, TTM_ADDTOOL, 0, ( LPARAM ) &ti ) )
   {
      return ;
   }

   _s.hWndTT = hwndTT;
}

//-------------------------------------------------------------------//

static DWORD hb_wvt_gtProcessMessages( void )
{
   MSG  msg;
   int  iIndex;
   BOOL bProcessed = FALSE;

   /* See if we have some graphic object to draw */
   if ( hb_gt_gobjects == NULL )
   {
      last_gobject = NULL;
   }
   else if( hb_gt_gobjects_end != last_gobject )
   {
      last_gobject = hb_gt_gobjects_end;
      InvalidateRect( _s.hWnd, NULL, FALSE );
   }

   while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
   {
      bProcessed = FALSE;

      for ( iIndex = 0; iIndex < WVT_DLGML_MAX; iIndex++ )
      {
         if ( _s.hDlgModeless[ iIndex ] != 0 )
         {
            if ( IsDialogMessage( _s.hDlgModeless[ iIndex ], &msg ) )
            {
               bProcessed = TRUE;
               break;
            }
         }
      }

      if ( bProcessed == FALSE )
      {
         TranslateMessage( &msg );
         DispatchMessage( &msg );
      }
   }
  return( msg.wParam );
}

//-------------------------------------------------------------------//

POINT HB_EXPORT hb_wvt_gtGetXYFromColRow( USHORT col, USHORT row )
{
  POINT xy;

  xy.x = ( col ) * _s.PTEXTSIZE.x;
  xy.y = ( row ) * _s.PTEXTSIZE.y;

  return( xy );
}

//-------------------------------------------------------------------//
/*
 * get the row and column from xy pixel client coordinates
 * This works because we are using the FIXED system font
 *
 */
static POINT hb_wvt_gtGetColRowFromXY( USHORT x, USHORT y )
{
  POINT colrow;

  colrow.x = ( x/_s.PTEXTSIZE.x );
  colrow.y = ( y/_s.PTEXTSIZE.y );

  return( colrow );
}

//-------------------------------------------------------------------//
/*
 * return a rectangle with row and column data, corresponding to the XY pixel
 * coordinates
 * This works because we are using the FIXED system font
 *
 */
static RECT hb_wvt_gtGetColRowFromXYRect( RECT xy )
{
  RECT colrow;

  colrow.left   = ( xy.left   / _s.PTEXTSIZE.x );
  colrow.top    = ( xy.top    / _s.PTEXTSIZE.y );                                           // 23/07/2004 9:02a.m.
  colrow.right  = ( xy.right  / _s.PTEXTSIZE.x - ( xy.right  % _s.PTEXTSIZE.x ? 0 : 1 ) );  // Adjust for when rectangle
  colrow.bottom = ( xy.bottom / _s.PTEXTSIZE.y - ( xy.bottom % _s.PTEXTSIZE.y ? 0 : 1 ) );  // EXACTLY overlaps characters

  return( colrow );
}

//-------------------------------------------------------------------//
/*
 * return a rectangle with the XY pixel coordinates corresponding to
 * the row and column data
 * This works because we are using the FIXED system font
 *
 */
static RECT hb_wvt_gtGetXYFromColRowRect( RECT colrow )
{
  RECT xy;

  xy.left   = ( colrow.left     ) * _s.PTEXTSIZE.x;
  xy.top    = ( colrow.top      ) * _s.PTEXTSIZE.y;
  xy.right  = ( colrow.right +1 ) * _s.PTEXTSIZE.x;
  xy.bottom = ( colrow.bottom+1 ) * _s.PTEXTSIZE.y;

  return( xy );
}

//-------------------------------------------------------------------//

static void hb_wvt_gtCreateCaret()
{
   // create and show the caret
   // create an underline caret of height - _s.CaretSize
   //
   _s.CaretExist = CreateCaret( _s.hWnd, ( HBITMAP ) NULL, _s.PTEXTSIZE.x, _s.CaretSize );
   if ( _s.CaretExist && _s.displayCaret )
   {
      hb_wvt_gtSetCaretPos();
      ShowCaret( _s.hWnd );
   }
}

//-------------------------------------------------------------------//

static void hb_wvt_gtKillCaret()
{
   if ( _s.CaretExist )
   {
      DestroyCaret();
      _s.CaretExist = FALSE ;
   }
}

//-------------------------------------------------------------------//
/*
 * hb_wvt_gtSetCaretPos converts col and row to x and y ( pixels ) and calls
 * the Windows function SetCaretPos ( with the expected coordinates )
 */
static BOOL hb_wvt_gtSetCaretPos()
{
  POINT xy;
  xy = hb_wvt_gtGetXYFromColRow( (SHORT) _s.caretPos.x, (SHORT) _s.caretPos.y );
  if ( _s.CaretSize > 0 )
  {
    xy.y += ( _s.PTEXTSIZE.y - _s.CaretSize );
  }
  if ( _s.CaretExist )
  {
    SetCaretPos( xy.x, xy.y );
  }
  return( TRUE );
}

//-------------------------------------------------------------------//
/*
 * hb_wvt_gtValidateRow checks the row bounds for the caret, wrapping if indicated
 */
static void hb_wvt_gtValidateRow( void )
{
  if ( _s.caretPos.y < 0 )
  {
    _s.caretPos.y = _GetScreenHeight()-1;
    if ( _s.caretPos.x > 0 )
    {
      _s.caretPos.x--;
    }
    else
    {
      _s.caretPos.x = _GetScreenWidth()-1;
    }
  }
  else if ( _s.caretPos.y >= _GetScreenHeight() )
  {
    _s.caretPos.y = 0;
    if ( _s.caretPos.x < _GetScreenWidth()-1 )
    {
      _s.caretPos.x++;
    }
    else
    {
       _s.caretPos.x = 0;
    }
  }
}

//-------------------------------------------------------------------//
/*
 * hb_wvt_gtValidateCol checks the column bounds for the caret, wrapping if indicated
 */
static void hb_wvt_gtValidateCol( void )
{
  if ( _s.caretPos.x < 0 )
  {
    _s.caretPos.x = _GetScreenWidth()-1;
    if ( _s.caretPos.y > 0 )
    {
      _s.caretPos.y--;
    }
    else
    {
      _s.caretPos.y = _GetScreenHeight()-1;
    }
  }
  else if ( _s.caretPos.x >= _GetScreenWidth() )
  {
    _s.caretPos.x = 0;
    if ( _s.caretPos.y < _GetScreenHeight()-1 )
    {
      _s.caretPos.y++;
    }
    else
    {
      _s.caretPos.y = 0;
    }
  }
}

//-------------------------------------------------------------------//
/*
 * hb_wvt_gtValidateCaret checks the bounds for the caret, wrapping if indicated
 * before setting the caret position on the screen
 */
static void hb_wvt_gtValidateCaret( void )
{
  hb_wvt_gtValidateCol();
  hb_wvt_gtValidateRow();

  // send message to window to display updated caret
  //
  SendMessage( _s.hWnd, WM_MY_UPDATE_CARET, 0, 0 );
}

//-------------------------------------------------------------------//
/*
 * hb_wvt_gtGetIndexForTextBuffer takes a row and column, and returns the appropriate
 * index into the screen Text buffer
 */
static USHORT hb_wvt_gtGetIndexForTextBuffer( USHORT col, USHORT row )
{
  return( row * _GetScreenWidth() + col );
}

//-------------------------------------------------------------------//
 /*
  * hb_wvt_gtGetColRowForTextBuffer takes an index into the screen Text buffer
  * and returns the corresponding row and column
  */
static POINT hb_wvt_gtGetColRowForTextBuffer( USHORT index )
{
  POINT colrow;

  colrow.x = index % _GetScreenWidth();
  colrow.y = index / _GetScreenWidth();

  return( colrow );
}

//-------------------------------------------------------------------//
/*
 * hb_wvt_gtTextOut converts col and row to x and y ( pixels ) and calls
 * the Windows function TextOut with the expected coordinates
 */
static BOOL hb_wvt_gtTextOut( HDC hdc,  USHORT col, USHORT row, LPCTSTR lpString, USHORT cbString  )
{
  BOOL  Result ;
  POINT xy;
  RECT  rClip;
//  long  nFontCX = _s.PTEXTSIZE.x;
//  long  nFontCY = _s.PTEXTSIZE.y;

  if ( cbString > _GetScreenWidth() ) // make sure string is not too long
  {
    cbString = _GetScreenWidth();
  }
  xy = hb_wvt_gtGetXYFromColRow( col, row );

//  SetRect( &rClip, xy.x, xy.y, xy.x + cbString * nFontCX, xy.y + nFontCY );
  SetRect( &rClip, xy.x, xy.y, xy.x + cbString * _s.PTEXTSIZE.x, xy.y + _s.PTEXTSIZE.y );

  if ( _s.FixedFont )
  {
     Result = ExtTextOut( hdc, xy.x, xy.y, ETO_CLIPPED|ETO_OPAQUE, &rClip, lpString, cbString, NULL );
  }
  else
  {
     Result = ExtTextOut( hdc, xy.x, xy.y, ETO_CLIPPED|ETO_OPAQUE, &rClip, lpString, cbString, _s.FixedSize );
  }
  return( Result ) ;
}

//-------------------------------------------------------------------//
//
/* get for and background colours from attribute and set them for window
*/
static BOOL hb_wvt_gtSetColors( HDC hdc, BYTE attr )
{
  int fore = attr & 0x000F;
  int back = ( attr & 0x00F0 )>>4;

  _s.foreground = _COLORS[ fore ];
  _s.background = _COLORS[ back ];

  SetTextColor( hdc, _s.foreground );
  SetBkColor( hdc, _s.background );
  SetTextAlign( hdc, TA_LEFT );

  return( TRUE );
}

//-------------------------------------------------------------------//
//
/* compute invalid rect in pixels, from row and col
*/
static void hb_wvt_gtSetInvalidRect( USHORT left, USHORT top, USHORT right, USHORT bottom )
{
  RECT rect;

  if ( _s.InvalidateWindow )
  {
    rect.left   = left;
    rect.top    = top;
    rect.right  = right;
    rect.bottom = bottom;

    rect = hb_wvt_gtGetXYFromColRowRect( rect );

    // check for wrapping
    //
    rect.left   = min( rect.left, rect.right );
    rect.top    = min( rect.top, rect.bottom );

    rect.right  = max( rect.left, rect.right );
    rect.bottom = max( rect.top, rect.bottom );

    if ( _s.RectInvalid.left < 0 )
    {
      memcpy( &_s.RectInvalid, &rect, sizeof( RECT ) );
    }
    else
    {
      _s.RectInvalid.left   = min( _s.RectInvalid.left  , rect.left   );
      _s.RectInvalid.top    = min( _s.RectInvalid.top   , rect.top    );
      _s.RectInvalid.right  = max( _s.RectInvalid.right , rect.right  );
      _s.RectInvalid.bottom = max( _s.RectInvalid.bottom, rect.bottom );
   }

    hv_wvt_gtDoInvalidateRect() ;
  }
}

//-------------------------------------------------------------------//

static void hv_wvt_gtDoInvalidateRect( void )
{
  if ( HB_GT_FUNC( gt_DispCount() ) <= 0 && ( _s.RectInvalid.left != -1 ) )
  {
    // InvalidateRect( _s.hWnd, &_s.RectInvalid, TRUE );
    InvalidateRect( _s.hWnd, &_s.RectInvalid, FALSE );
    _s.RectInvalid.left = -1 ;
    hb_wvt_gtProcessMessages();
  }
}

//-------------------------------------------------------------------//

static void hb_wvt_gtTranslateKey( int key, int shiftkey, int altkey, int controlkey )
{
  int nVirtKey = GetKeyState( VK_MENU );
  if ( nVirtKey & 0x8000 ) // alt + key
  {
    hb_wvt_gtAddCharToInputQueue( altkey );
  }
  else
  {
    nVirtKey = GetKeyState( VK_CONTROL );
    if ( nVirtKey & 0x8000 ) // control + key
    {
      hb_wvt_gtAddCharToInputQueue( controlkey );
    }
    else
    {
      nVirtKey = GetKeyState( VK_SHIFT );
      if ( nVirtKey & 0x8000 ) // shift + key
      {
        hb_wvt_gtAddCharToInputQueue( shiftkey );
      }
      else //just key
      {
        hb_wvt_gtAddCharToInputQueue( key );
      }
    }
  }
}

//-------------------------------------------------------------------//
//
// font stuff
/* use the standard fixed oem font, unless the caller has requested set size fonts
*/
static HFONT hb_wvt_gtGetFont( char * pszFace, int iHeight, int iWidth, int iWeight, int iQuality, int iCodePage )
{
  HFONT hFont;
  if ( iHeight > 0 )
  {
    LOGFONT logfont;

    logfont.lfEscapement     = 0;
    logfont.lfOrientation    = 0;
    logfont.lfWeight         = iWeight ;
    logfont.lfItalic         = 0;
    logfont.lfUnderline      = 0;
    logfont.lfStrikeOut      = 0;
    logfont.lfCharSet        = iCodePage;             // OEM_CHARSET;
    logfont.lfOutPrecision   = 0;
    logfont.lfClipPrecision  = 0;
    logfont.lfQuality        = iQuality;              // DEFAULT_QUALITY, DRAFT_QUALITY or PROOF_QUALITY
    logfont.lfPitchAndFamily = FIXED_PITCH+FF_MODERN; // all mapping depends on fixed width fonts!
    logfont.lfHeight         = iHeight;
    logfont.lfWidth          = iWidth < 0 ? -iWidth : iWidth ;

    strcpy( logfont.lfFaceName,pszFace );

    hFont = CreateFontIndirect( &logfont );
  }
  else
  {
//    hFont = GetStockObject( SYSTEM_FIXED_FONT );
    hFont = ( HFONT ) GetStockObject( OEM_FIXED_FONT );
  }
  return( hFont );

}

//-------------------------------------------------------------------//

static void gt_hbInitStatics( void )
{
  OSVERSIONINFO osvi ;
  HINSTANCE     h;
  int           iIndex;

  _s.ROWS             = WVT_DEFAULT_ROWS;
  _s.COLS             = WVT_DEFAULT_COLS;
  _s.foreground       = WHITE;
  _s.background       = BLACK;
  _s.BUFFERSIZE       = 0;
  _s.pAttributes      = NULL;
  _s.pBuffer          = NULL;
  _s.caretPos.x       = 0;
  _s.caretPos.y       = 0;
  _s.CaretExist       = FALSE;
  _s.CaretSize        = 4;
  _s.mousePos.x       = 0;
  _s.mousePos.y       = 0;
  _s.MouseMove        = FALSE ;
  _s.hWnd             = NULL;
  _s.keyPointerIn     = 1;
  _s.keyPointerOut    = 0;
  _s.displayCaret     = TRUE;
  _s.RectInvalid.left = -1 ;

  // THEESE are the default font parameters, if not changed by user
  _s.PTEXTSIZE.x      = 8;
  _s.PTEXTSIZE.y      = 12;
  _s.fontHeight       = 20;
  _s.fontWidth        = 10;
  _s.fontWeight       = FW_NORMAL;
  _s.fontQuality      = DEFAULT_QUALITY;
  strcpy( _s.fontFace,"Courier New" );

  _s.LastMenuEvent    = 0;
  _s.MenuKeyEvent     = 1024;
  _s.CentreWindow     = TRUE;       // Default is to always display window in centre of screen
  _s.CodePage         = GetACP() ;  // Set code page to default system

  osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
  GetVersionEx ( &osvi );
  _s.Win9X            = ( osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS );
  _s.AltF4Close       = FALSE;
  _s.InvalidateWindow = TRUE;
  _s.EnableShortCuts  = FALSE;
  _s.pSymWVT_PAINT    = hb_dynsymFind( "WVT_PAINT"     ) ;
  _s.pSymWVT_SETFOCUS = hb_dynsymFind( "WVT_SETFOCUS"  ) ;
  _s.pSymWVT_KILLFOCUS= hb_dynsymFind( "WVT_KILLFOCUS" ) ;
  _s.pSymWVT_MOUSE    = hb_dynsymFind( "WVT_MOUSE"     ) ;
  _s.pSymWVT_TIMER    = hb_dynsymFind( "WVT_TIMER"     ) ;
  _s.rowStart         = 0;
  _s.rowStop          = 0;
  _s.colStart         = 0;
  _s.colStop          = 0;
  _s.bToolTipActive   = FALSE;

  h = LoadLibraryEx( "msimg32.dll", NULL, 0 );
  if ( h )
  {
    _s.pfnGF = ( wvtGradientFill ) GetProcAddress( h, "GradientFill" );
    if ( _s.pfnGF )
    {
      _s.hMSImg32 = h;
    }
  }

  for ( iIndex = 0; iIndex < WVT_DLGML_MAX; iIndex++ )
  {
     _s.hDlgModeless[ iIndex ]        = NULL;
//     _s.pSymDlgProcModeless[ iIndex ] = NULL;
     _s.pFunc[ iIndex ]               = NULL;
     _s.iType[ iIndex ]               = ( int ) NULL;
  }
  for ( iIndex = 0; iIndex < WVT_DLGMD_MAX; iIndex++ )
  {
     _s.hDlgModal[ iIndex ]           = NULL;
     _s.pFuncModal[ iIndex ]          = NULL;
     _s.iTypeModal[ iIndex ]          = ( int ) NULL;
  }

  _s.bGui             = FALSE;
}

//-------------------------------------------------------------------//
/*
 *  functions for handling the input queues for the mouse and keyboard
 */
void HB_EXPORT hb_wvt_gtAddCharToInputQueue ( int data )
{
  int iNextPos;
  iNextPos = ( _s.keyPointerIn >= WVT_CHAR_QUEUE_SIZE ) ? 0 : _s.keyPointerIn+1 ;
  if ( iNextPos != _s.keyPointerOut ) // Stop accepting characters once the buffer is full
  {
    _s.Keys[ _s.keyPointerIn ] = data ;
    _s.keyPointerIn = iNextPos ;
  }
}

//-------------------------------------------------------------------//

static BOOL hb_wvt_gtGetCharFromInputQueue ( int *c )
{
  int iNextPos;
  BOOL bRet = FALSE;
  *c = 0;
  iNextPos = ( _s.keyPointerOut >= WVT_CHAR_QUEUE_SIZE ) ? 0 : _s.keyPointerOut+1 ;
  if ( iNextPos != _s.keyPointerIn )  // No more events in queue ??
  {
    *c = _s.Keys[ iNextPos ] ;
    _s.keyPointerOut = iNextPos ;
    bRet =  TRUE;
  }
  return( bRet );
}

//-------------------------------------------------------------------//

static USHORT hb_wvt_gtGetMouseX ( void )
{
  return( (SHORT) _s.mousePos.x );
}

//-------------------------------------------------------------------//

static USHORT hb_wvt_gtGetMouseY ( void )
{
  return( (SHORT) _s.mousePos.y );
}

//-------------------------------------------------------------------//

static void hb_wvt_gtSetMouseX ( USHORT ix )
{
  _s.mousePos.x = ix;
}

//-------------------------------------------------------------------//

static void hb_wvt_gtSetMouseY ( USHORT iy )
{
  _s.mousePos.y = iy;
}

//-------------------------------------------------------------------//
/*
 * hb_wvt_gtSetStringInTextBuffer puts the string of the specified length into the TextBuffer at
 * the specified caret position
 * It then determines the invalid rectangle, so the string will be displayed
 */
static void hb_wvt_gtSetStringInTextBuffer( USHORT col, USHORT row, BYTE attr, BYTE *sBuffer, USHORT length )
{
  POINT end;
  USHORT index;

  // determine the index and put the string into the TextBuffer
  //
  index = hb_wvt_gtGetIndexForTextBuffer( col, row );
  if ( length + index <= _s.BUFFERSIZE )
  {
    memcpy( ( _s.pBuffer+index ), sBuffer, length );
//    if ( attr != ' ' ) // if no attribute, don't overwrite
//    {
    memset( ( _s.pAttributes+index ), attr, length );
//    }

    //  determine bounds of rect around character to refresh
    //
    end = hb_wvt_gtGetColRowForTextBuffer( index + ( length -1 ) ); //location of last char
    hb_wvt_gtSetInvalidRect( (SHORT) col, (SHORT) row, (SHORT) end.x, (SHORT) end.y );
  }
}

//-------------------------------------------------------------------//

static void hb_wvt_gtSetCaretOn( BOOL bOn )
{
  if ( _s.CaretExist )
  {
    if ( bOn )
    {
      hb_wvt_gtSetCaretPos();
      ShowCaret( _s.hWnd );
    }
    else
    {
      HideCaret( _s.hWnd );
    }
  }
  _s.displayCaret = bOn;
}

//-------------------------------------------------------------------//

static void hb_wvt_gtHandleMenuSelection( int menuIndex )
{
  _s.LastMenuEvent = menuIndex ;
  hb_wvt_gtAddCharToInputQueue( _s.MenuKeyEvent );
}

//-------------------------------------------------------------------//

static void hb_wvt_gtMouseEvent( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
  POINT xy, colrow ;
  SHORT keyCode = 0;
  SHORT keyState = 0;
  ULONG lPopupRet ;

  HB_SYMBOL_UNUSED( hWnd );
  HB_SYMBOL_UNUSED( wParam );

  if ( !b_MouseEnable )
  {
    return;
  }
  else
  {
    if ( message == WM_MOUSEMOVE || message == WM_NCMOUSEMOVE )
    {
      if ( ! _s.MouseMove )
      {
        return;
      }
    }

    xy.x   = LOWORD( lParam );
    xy.y   = HIWORD( lParam );

    colrow = hb_wvt_gtGetColRowFromXY( ( SHORT ) xy.x, ( SHORT ) xy.y );

    hb_wvt_gtSetMouseX( ( SHORT ) colrow.x );
    hb_wvt_gtSetMouseY( ( SHORT ) colrow.y );

    switch( message )
    {
      case WM_LBUTTONDBLCLK:
        keyCode = K_LDBLCLK;
        break;

      case WM_RBUTTONDBLCLK:
        keyCode = K_RDBLCLK;
        break;

      case WM_LBUTTONDOWN:
        keyCode = K_LBUTTONDOWN;
        break;

      case WM_RBUTTONDOWN:
        keyCode = K_RBUTTONDOWN;
        break;

      case WM_RBUTTONUP:
        if ( _s.hPopup )
        {
           GetCursorPos( &xy );
           lPopupRet = TrackPopupMenu( _s.hPopup, TPM_CENTERALIGN + TPM_RETURNCMD, xy.x, xy.y, 0, hWnd, NULL );
           if ( lPopupRet )
           {
              hb_wvt_gtAddCharToInputQueue( lPopupRet );
           }
          return;
        }
        else
        {
          keyCode = K_RBUTTONUP;
          break;
        }

      case WM_LBUTTONUP:
        keyCode = K_LBUTTONUP;
        break;

      case WM_MBUTTONDOWN:
        keyCode = K_MBUTTONDOWN;
        break;

      case WM_MBUTTONUP:
        keyCode = K_MBUTTONUP;
        break;

      case WM_MBUTTONDBLCLK:
        keyCode = K_MDBLCLK;
        break;

      case WM_MOUSEMOVE:
        keyState = wParam;

        if      ( keyState == MK_LBUTTON )
        {
           keyCode = K_MMLEFTDOWN;
        }
        else if ( keyState == MK_RBUTTON )
        {
           keyCode = K_MMRIGHTDOWN;
        }
        else if ( keyState == MK_MBUTTON )
        {
           keyCode = K_MMMIDDLEDOWN;
        }
        else
        {
           keyCode = K_MOUSEMOVE;
        }
        break;

      case WM_MOUSEWHEEL:
        keyState = HIWORD( wParam );

        if ( keyState > 0 )
        {
           keyCode = K_MWFORWARD;
        }
        else
        {
           keyCode = K_MWBACKWARD;
        }
        break;

      case WM_NCMOUSEMOVE:
         {
            keyCode = K_NCMOUSEMOVE;
         }
         break;
    }

    if ( _s.pSymWVT_MOUSE && keyCode != 0 )
    {
      hb_vmPushSymbol( _s.pSymWVT_MOUSE->pSymbol );
      hb_vmPushNil();
      hb_vmPushLong( ( SHORT ) keyCode  );
      hb_vmPushLong( ( SHORT ) colrow.y );
      hb_vmPushLong( ( SHORT ) colrow.x );
      hb_vmPushLong( ( SHORT ) keyState );
      hb_vmDo( 4 );
      hb_itemGetNL( ( PHB_ITEM ) &HB_VM_STACK.Return );
    }

    hb_wvt_gtAddCharToInputQueue( keyCode );
  }
}

//-------------------------------------------------------------------//
//-------------------------------------------------------------------//
//-------------------------------------------------------------------//
//
//               Exported functions for API calls
//
//-------------------------------------------------------------------//
//-------------------------------------------------------------------//
//-------------------------------------------------------------------//

BOOL HB_EXPORT hb_wvt_gtSetMenuKeyEvent( int iMenuKeyEvent )
{
  int iOldEvent;
  iOldEvent = _s.MenuKeyEvent ;
  if ( iMenuKeyEvent )
  {
    _s.MenuKeyEvent = iMenuKeyEvent;
  }
  return( iOldEvent );
}

//-------------------------------------------------------------------//

BOOL HB_EXPORT hb_wvt_gtSetCentreWindow( BOOL bCentre, BOOL bPaint )
{
  BOOL bWasCentre;
  bWasCentre = _s.CentreWindow ;
  _s.CentreWindow = bCentre;
  if ( bPaint )
  {
    hb_wvt_gtResetWindowSize( _s.hWnd ) ;
  }
  return( bWasCentre );
}

//-------------------------------------------------------------------//

void HB_EXPORT hb_wvt_gtResetWindow( void )
{
  hb_wvt_gtResetWindowSize( _s.hWnd ) ;
}

//-------------------------------------------------------------------//

BOOL HB_EXPORT hb_wvt_gtSetCodePage( int iCodePage )
{
  int iOldCodePage;
  iOldCodePage = _s.CodePage ;
  if ( iCodePage )
  {
    _s.CodePage = iCodePage;
  }
  if ( iOldCodePage != iCodePage )
  {
    hb_wvt_gtResetWindow();
  }
  return( iOldCodePage );
}

//-------------------------------------------------------------------//

int HB_EXPORT hb_wvt_gtGetLastMenuEvent( void )
{
  return( _s.LastMenuEvent );
}

//-------------------------------------------------------------------//

void HB_EXPORT hb_wvt_gtSetWindowTitle( char * title )
{
  SetWindowText( _s.hWnd, title );
}

//-------------------------------------------------------------------//

DWORD HB_EXPORT hb_wvt_gtSetWindowIcon( int icon, char *lpIconName )
{
  HICON hIcon;

  if( lpIconName == NULL )
{
   hIcon = LoadIcon( ( HINSTANCE ) hb_hInstance, MAKEINTRESOURCE( icon ) );
  }
  else
  {
    hIcon = LoadIcon( ( HINSTANCE ) hb_hInstance, lpIconName );
  }

  if ( hIcon )
  {
    SendMessage( _s.hWnd, WM_SETICON, ICON_SMALL, ( LPARAM )hIcon ); // Set Title Bar ICON
    SendMessage( _s.hWnd, WM_SETICON, ICON_BIG, ( LPARAM )hIcon ); // Set Task List Icon
  }
  return( ( DWORD ) hIcon ) ;
}

//-------------------------------------------------------------------//

DWORD HB_EXPORT hb_wvt_gtSetWindowIconFromFile( char *icon )
{
  HICON hIcon = (HICON) LoadImage( ( HINSTANCE ) NULL, icon, IMAGE_ICON, 0, 0, LR_LOADFROMFILE );

  if ( hIcon )
  {
    SendMessage( _s.hWnd, WM_SETICON, ICON_SMALL, ( LPARAM ) hIcon ); // Set Title Bar ICON
    SendMessage( _s.hWnd, WM_SETICON, ICON_BIG  , ( LPARAM ) hIcon ); // Set Task List Icon
  }

  return( ( DWORD ) hIcon ) ;
}

//-------------------------------------------------------------------//

int HB_EXPORT hb_wvt_gtGetWindowTitle( char *title, int length )
{
  return( GetWindowText( _s.hWnd, title, length ) );
}

//-------------------------------------------------------------------//

BOOL HB_EXPORT hb_wvt_gtSetFont( char *fontFace, int height, int width, int Bold, int Quality )
{
  int   size;
  BOOL  bResult = TRUE ;
  HFONT hFont   = hb_wvt_gtGetFont( fontFace, height, width, Bold, Quality, _s.CodePage );

  // make sure the font could actually be created
  //
  if ( hFont )
  {
    // make sure that the font  will fit inside the
    // window with the current _s.ROWS and _s.COLS setting
//    if ( hb_wvt_gtValidWindowSize( _GetScreenHeight(),_GetScreenWidth(), hFont, width ) )
    {
      _s.fontHeight  = height;
      _s.fontWidth   = width;
      _s.fontWeight  = Bold;
      _s.fontQuality = Quality;

      size = strlen( fontFace );
      if ( ( size > 0 ) && ( size < LF_FACESIZE-1 ) )
      {
        strcpy( _s.fontFace, fontFace );
      }
      if ( _s.hWnd )
      {
        // resize the window based on new fonts
        //
        hb_wvt_gtResetWindowSize( _s.hWnd );

        // force resize of caret
        //
        hb_wvt_gtKillCaret();
        hb_wvt_gtCreateCaret();
      }
      bResult= TRUE;
    }
    DeleteObject( hFont );
  }
  return( bResult );
}

//-------------------------------------------------------------------//

HWND HB_EXPORT hb_wvt_gtGetWindowHandle( void )
{
  return( _s.hWnd );
}

//-------------------------------------------------------------------//

void HB_EXPORT hb_wvt_gtPostMessage( int message )
{
  SendMessage( _s.hWnd, WM_CHAR,message, 0 );
}

//-------------------------------------------------------------------//

BOOL HB_EXPORT hb_wvt_gtSetWindowPos( int left, int top )
{
  RECT wi;
  GetWindowRect( _s.hWnd, &wi );
  return( SetWindowPos( _s.hWnd, NULL, left, top, ( wi.right-wi.left )+1, ( wi.bottom-wi.top )+1, SWP_NOZORDER ) );
}

//-------------------------------------------------------------------//

BOOL HB_EXPORT hb_wvt_gtSetAltF4Close( BOOL bCanClose )
{
  BOOL bWas;
  bWas = _s.AltF4Close;
  _s.AltF4Close = bCanClose;
  return( bWas );
}

//-------------------------------------------------------------------//

void HB_EXPORT hb_wvt_gtDoProcessMessages( void )
{
  hb_wvt_gtProcessMessages();
}

//-------------------------------------------------------------------//

BOOL HB_EXPORT hb_wvt_gtSetMouseMove( BOOL bHandleEvent )
{
  BOOL bWas = _s.MouseMove;
  _s.MouseMove = bHandleEvent;
  return( bWas );
}

//-------------------------------------------------------------------//

BOOL HB_EXPORT hb_wvt_gtEnableShortCuts( BOOL bEnable )
{
  BOOL bWas = _s.EnableShortCuts;
  _s.EnableShortCuts = bEnable;
  return( bWas );
}

//-------------------------------------------------------------------//

IPicture * HB_EXPORT hb_wvt_gtLoadPicture( char * image )
{
  IStream   *iStream;
  IPicture  *iPicture;
  HGLOBAL   hGlobal;
  HANDLE    hFile;
  DWORD     nFileSize;
  DWORD     nReadByte;
  IPicture  *Result = NULL;

  hFile = CreateFile( image, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
  if ( hFile != INVALID_HANDLE_VALUE )
  {
    nFileSize = GetFileSize( hFile, NULL );

    if ( nFileSize != INVALID_FILE_SIZE )
    {
      hGlobal = GlobalAlloc( GPTR, nFileSize );

      if ( hGlobal )
      {
        if ( ReadFile( hFile, hGlobal, nFileSize, &nReadByte, NULL ) )
        {
          CreateStreamOnHGlobal( hGlobal, TRUE, &iStream );
          OleLoadPicture( iStream, nFileSize, TRUE, &IID_IPicture, ( LPVOID* ) &iPicture );
          if ( iPicture )
          {
            Result = iPicture;
          }
        }
        GlobalFree( hGlobal );
      }
    }
    CloseHandle( hFile );
  }
  return( Result );
}

//-------------------------------------------------------------------//

BOOL HB_EXPORT hb_wvt_gtRenderPicture( int x1, int y1, int wd, int ht, IPicture * iPicture )
{
  LONG     lWidth,lHeight;
  int      x,y,xe,ye;
  int      c   = x1 ;
  int      r   = y1 ;
  int      dc  = wd ;
  int      dr  = ht ;
  int      tor =  0 ;
  int      toc =  0 ;
  HRGN     hrgn1;
  POINT    lpp;
  BOOL     bResult = FALSE;

  if ( iPicture )
  {
    iPicture->lpVtbl->get_Width( iPicture,&lWidth );
    iPicture->lpVtbl->get_Height( iPicture,&lHeight );

    if ( dc  == 0 )
    {
      dc = ( int ) ( ( float ) dr * lWidth  / lHeight );
    }
    if ( dr  == 0 )
    {
      dr = ( int ) ( ( float ) dc * lHeight / lWidth  );
    }
    if ( tor == 0 )
    {
      tor = dr;
    }
    if ( toc == 0 )
    {
      toc = dc;
    }
    x  = c;
    y  = r;
    xe = c + toc - 1;
    ye = r + tor - 1;

    GetViewportOrgEx( _s.hdc, &lpp );

    hrgn1 = CreateRectRgn( c+lpp.x, r+lpp.y, xe+lpp.x, ye+lpp.y );
    SelectClipRgn( _s.hdc, hrgn1 );

    while ( x < xe )
    {
      while ( y < ye )
      {
        iPicture->lpVtbl->Render( iPicture, _s.hdc, x, y, dc, dr, 0, lHeight, lWidth, -lHeight, NULL );
        y += dr;
      }
      y =  r;
      x += dc;
    }

    SelectClipRgn( _s.hdc, NULL );
    DeleteObject( hrgn1 );

    bResult = TRUE ;
  }

  return( bResult );
}

//-------------------------------------------------------------------//

BOOL HB_EXPORT hb_wvt_gtDestroyPicture( IPicture * iPicture )
{
   BOOL bResult = FALSE;

   if ( iPicture )
   {
      iPicture->lpVtbl->Release( iPicture );
      bResult = TRUE;
   }
   return bResult;
}

//-------------------------------------------------------------------//

HB_EXPORT GLOBAL_DATA * hb_wvt_gtGetGlobalData( void )
{
   return &_s;
}

//-------------------------------------------------------------------//

HB_EXPORT COLORREF hb_wvt_gtGetColorData( int iIndex )
{
   return _COLORS[ iIndex ];
}

//-------------------------------------------------------------------//

HB_EXPORT BOOL hb_wvt_gtSetColorData( int iIndex, COLORREF ulCr )
{
   BOOL bResult = FALSE;

   if ( iIndex >= 0 && iIndex < 16 )
   {
      _COLORS[ iIndex ] = ulCr;
      bResult = TRUE;
   }
   return bResult;
}

//-------------------------------------------------------------------//
//                         Clipboard support
//-------------------------------------------------------------------//

void HB_GT_FUNC( gt_GetClipboard( char *szData, ULONG *pulMaxSize ) )
{
   HGLOBAL hglb;
   LPTSTR  lptstr;
   UINT    uFormat = ( _s.CodePage == OEM_CHARSET ) ? CF_OEMTEXT : CF_TEXT;

   if ( ! IsClipboardFormatAvailable( uFormat ) )
   {
     *pulMaxSize = 0;
     return;
   }

   if ( !OpenClipboard( NULL ) )
   {
     *pulMaxSize = 0;
     return;
   }

   hglb = GetClipboardData( uFormat );
   if ( hglb != NULL )
   {
      lptstr = ( LPSTR ) GlobalLock( hglb );
      if ( lptstr != NULL )
      {
         ULONG iLen = strlen( lptstr );
         if ( *pulMaxSize == 0 || *pulMaxSize > iLen )
         {
            *pulMaxSize = iLen;
         }

         //  still nothing ?
         //
         if ( *pulMaxSize == 0 )
         {
            return;
         }

         memcpy( szData, lptstr, *pulMaxSize );
         szData[ *pulMaxSize ] = '\0';

         GlobalUnlock( hglb );
      }
   }
   CloseClipboard();
}

//-------------------------------------------------------------------//

void HB_GT_FUNC( gt_SetClipboard( char *szData, ULONG ulSize ) )
{
   LPTSTR  lptstrCopy;
   HGLOBAL hglbCopy;
   UINT    uFormat = ( _s.CodePage == OEM_CHARSET ) ? CF_OEMTEXT : CF_TEXT;

/*  This poses problems when some other application copies a bitmap on the
    clipboard. The only way to set text to clipboard is made possible
    only if another application copies some text on the clipboard.

   if ( !IsClipboardFormatAvailable( CF_TEXT ) )
   {
     return;
   }
*/

   if ( ! OpenClipboard( NULL ) )
   {
     return;
   }
   EmptyClipboard();

   // Allocate a global memory object for the text.
   //
   hglbCopy = GlobalAlloc( GMEM_MOVEABLE, ( ulSize+1 ) * sizeof( TCHAR ) );
   if ( ! hglbCopy )
   {
      CloseClipboard();
      return;
   }

   // Lock the handle and copy the text to the buffer.
   //
   lptstrCopy = ( LPSTR ) GlobalLock( hglbCopy );

   memcpy( lptstrCopy, szData, ( ulSize+1 ) * sizeof( TCHAR ) );

   lptstrCopy[ ulSize+1 ] = ( TCHAR ) 0;    // null character

   GlobalUnlock( hglbCopy );

   // Place the handle on the clipboard.
   //
   SetClipboardData( uFormat, hglbCopy );

   CloseClipboard();
}

//-------------------------------------------------------------------//

ULONG HB_GT_FUNC( gt_GetClipboardSize( void ) )
{
   HGLOBAL hglb;
   LPTSTR  lptstr;
   UINT    uFormat = ( _s.CodePage == OEM_CHARSET ) ? CF_OEMTEXT : CF_TEXT;
   int     ret;

   if ( !IsClipboardFormatAvailable( uFormat ) )
   {
     return 0;
   }

   if ( ! OpenClipboard( NULL ) )
   {
     return 0;
   }

   hglb = GetClipboardData( uFormat );
   ret  = 0;
   if ( hglb != NULL )
   {
      lptstr = ( LPSTR ) GlobalLock( hglb );
      if ( lptstr != NULL )
      {
         ret = strlen( lptstr );
         GlobalUnlock( hglb );
      }
   }
   CloseClipboard();
   return ret;
}

//-------------------------------------------------------------------//

void HB_GT_FUNC( gt_ProcessMessages( void ) )
{
  hb_wvt_gtProcessMessages();
}

//-------------------------------------------------------------------//

int HB_GT_FUNC( gt_info( int iMsgType, BOOL bUpdate, int iParam, void *vpParam ) )
{
   int iOldValue;

   switch ( iMsgType )
   {
      case GTI_ISGRAPHIC:
         return ( int ) TRUE;

      case GTI_MOUSESTATUS:
         iOldValue = ( int ) b_MouseEnable;
         if ( bUpdate )
         {
            b_MouseEnable = iParam;
         }
         return iOldValue;

      case GTI_FONTNAME:
         if ( bUpdate )
         {
            strcpy( _s.fontFace, (char *) vpParam );
         }
         return ( int ) TRUE;

      case GTI_FONTSIZE:
         iOldValue = ( int ) _s.PTEXTSIZE.y;
         if ( bUpdate )
         {
            HFONT hFont = hb_wvt_gtGetFont( _s.fontFace, iParam, _s.fontWidth, _s.fontWeight, _s.fontQuality, _s.CodePage );
            // make sure the font could actually be created
            if ( hFont )
            {
               _s.fontHeight = iParam;
               // is the window already opened?
               if ( _s.hWnd )
               {
                 // resize the window based on new fonts
                 //
                 hb_wvt_gtResetWindowSize( _s.hWnd );

                 // force resize of caret
                 //
                 hb_wvt_gtKillCaret();
                 hb_wvt_gtCreateCaret();
               }
               DeleteObject( hFont );
            }
         }
         return iOldValue;

      case GTI_FONTWIDTH:
         iOldValue = ( int ) _s.PTEXTSIZE.x;
         if ( bUpdate )
         {
            // store font status for next operation on fontsize
            _s.fontWidth = iParam;
         }
         return iOldValue;

      case GTI_FONTWEIGHT:
         switch( _s.fontWeight )
         {
            case FW_THIN:
            case FW_EXTRALIGHT:
            case FW_LIGHT:
               iOldValue = GTI_FONTW_THIN;
            break;

            case FW_DONTCARE:
            case FW_NORMAL:
            case FW_MEDIUM:
               iOldValue = GTI_FONTW_NORMAL;
            break;

            case FW_SEMIBOLD:
            case FW_BOLD:
            case FW_EXTRABOLD:
            case FW_HEAVY:
               iOldValue = GTI_FONTW_BOLD;
            break;

            default:
               iOldValue = 0;
            break;
         }

         if ( bUpdate )
         {
            // store font status for next operation on fontsize
            switch( iParam )
            {
               case GTI_FONTW_THIN:
                  _s.fontWeight = FW_LIGHT;

               case GTI_FONTW_NORMAL:
                  _s.fontWeight = FW_NORMAL;
               break;

               case GTI_FONTW_BOLD:
                  _s.fontWeight = FW_BOLD;
            }
         }
         return iOldValue;

      case GTI_FONTQUALITY:
         switch( _s.fontQuality )
         {
            case ANTIALIASED_QUALITY:
               iOldValue = GTI_FONTQ_HIGH;
            break;

            case DEFAULT_QUALITY:
            case DRAFT_QUALITY:
               iOldValue = GTI_FONTQ_NORMAL;
            break;

            case NONANTIALIASED_QUALITY:
            case PROOF_QUALITY:
               iOldValue = GTI_FONTQ_DRAFT;
            break;

            default:
               iOldValue = 0;
            break;
         }

         if ( bUpdate )
         {
            switch( iParam )
            {
               case GTI_FONTQ_HIGH:
                  _s.fontQuality = ANTIALIASED_QUALITY;
               break;

               case GTI_FONTQ_NORMAL:
                  _s.fontQuality = DEFAULT_QUALITY;
               break;

               case GTI_FONTQ_DRAFT:
                  _s.fontQuality = DRAFT_QUALITY;
            }
         }
         return iOldValue;

      case GTI_SCREENHEIGHT:
         iOldValue = _s.PTEXTSIZE.y * _GetScreenHeight();
         if ( bUpdate )
         {
            HB_GT_FUNC( gt_SetMode( (USHORT) (iParam/_s.PTEXTSIZE.y), _GetScreenWidth() ) );
         }
         return iOldValue;

      case GTI_SCREENWIDTH:
         iOldValue = _s.PTEXTSIZE.x * _GetScreenWidth();
         if ( bUpdate )
         {
            HB_GT_FUNC( gt_SetMode( _GetScreenHeight(), (USHORT) (iParam/_s.PTEXTSIZE.x) ) );
         }
         return iOldValue;

      case GTI_DESKTOPWIDTH:
      {
         RECT rDesk;
         HWND hDesk;

         hDesk = GetDesktopWindow();
         GetWindowRect( hDesk, &rDesk );
         return rDesk.right - rDesk.left;
      }

      case GTI_DESKTOPHEIGHT:
      {
         RECT rDesk;
         HWND hDesk = GetDesktopWindow();
         GetWindowRect( hDesk, &rDesk );
         return rDesk.bottom - rDesk.top;
      }

      case GTI_DESKTOPCOLS:
      {
         RECT rDesk;
         HWND hDesk;

         hDesk = GetDesktopWindow();
         GetClientRect( hDesk, &rDesk );

         return (rDesk.right - rDesk.left) / _s.PTEXTSIZE.x;
      }


      case GTI_DESKTOPROWS:
      {
         RECT rDesk;
         HWND hDesk;

         hDesk = GetDesktopWindow();
         GetClientRect( hDesk, &rDesk );

         return (rDesk.bottom - rDesk.top) / _s.PTEXTSIZE.y;
      }

      case GTI_INPUTFD:
         return (int) GetStdHandle( STD_INPUT_HANDLE );

      case GTI_OUTPUTFD:
         return (int) GetStdHandle( STD_INPUT_HANDLE );

      case GTI_ERRORFD:
         return (int) GetStdHandle( STD_ERROR_HANDLE );

      case GTI_WINTITLE:
         {
            hb_wvt_gtSetWindowTitle( (char *) vpParam );
            return 1;   // 0 1.119
         }

      case GTI_CODEPAGE:
         return (int) hb_wvt_gtSetCodePage( iParam );

      case GTI_ICONFILE:
         return (long) hb_wvt_gtSetWindowIconFromFile( (char *) vpParam  );

      case GTI_ICONRES:
         return (long) hb_wvt_gtSetWindowIcon( iParam, (char *) vpParam );


      case GTI_VIEWMAXWIDTH:
         return _GetScreenWidth();

      case GTI_VIEWMAXHEIGHT:
         return _GetScreenHeight();

   }

   // DEFAULT: there's something wrong if we are here.
   return -1;
}

//-------------------------------------------------------------------//

/* ********** Graphics API ********** */
/*
 * NOTE:
 *      gfxPrimitive() parameters may have different meanings
 *      ie: - Desired color is 'iBottom' for PUTPIXEL and 'iRight' for CIRCLE
 *          - Red is iTop, Green iLeft and Blue is iBottom for MAKECOLOR
 *
 */

#define SetGFXContext() hPen=CreatePen(PS_SOLID,1,color); hOldPen=(HPEN) SelectObject(hdc,hPen); hBrush=(HBRUSH) CreateSolidBrush(color); hOldBrush=(HBRUSH) SelectObject(hdc,hBrush); bOut=TRUE

int HB_GT_FUNC( gt_gfxPrimitive( int iType, int iTop, int iLeft, int iBottom, int iRight, int iColor ) )
{
COLORREF      color;
HPEN          hPen, hOldPen;
HBRUSH        hBrush, hOldBrush;
HDC           hdc;
BOOL          bOut = FALSE;
int           iRet = 0;

   hdc = GetDC( _s.hWnd );

   switch ( iType )
   {
      case GFX_ACQUIRESCREEN:
      case GFX_RELEASESCREEN:
         return 1;
      case GFX_MAKECOLOR:
         return (int) ( iTop << 16 | iLeft << 8 | iBottom );
      case GFX_PUTPIXEL:
         color = RGB( iBottom >> 16, ( iBottom & 0xFF00 ) >> 8, iBottom & 0xFF );
         SetGFXContext();

         MoveToEx( hdc, iLeft, iTop, NULL );
         LineTo( hdc, iLeft, iTop );

         iRet = 1;
         break;
      case GFX_LINE:
         color = RGB( iColor >> 16, ( iColor & 0xFF00 ) >> 8, iColor & 0xFF );
         SetGFXContext();

         MoveToEx( hdc, iLeft, iTop, NULL );
         LineTo( hdc, iRight, iBottom );

         iRet = 1;
         break;
      case GFX_RECT:
      {
         RECT r;
         r.left = iLeft;
         r.top = iTop;
         r.right = iRight;
         r.bottom = iBottom;

         color = RGB( iColor >> 16, ( iColor & 0xFF00 ) >> 8, iColor & 0xFF );
         SetGFXContext();

         FrameRect( hdc, &r, hBrush );

         iRet = 1;
      }
      break;
      case GFX_FILLEDRECT:
         color = RGB( iColor >> 16, ( iColor & 0xFF00 ) >> 8, iColor & 0xFF );
         SetGFXContext();

         Rectangle( hdc, iLeft, iTop, iRight, iBottom );

         iRet = 1;
         break;
      case GFX_CIRCLE:
         color = RGB( iRight >> 16, ( iRight & 0xFF00 ) >> 8, iRight & 0xFF );
         SetGFXContext();

         Arc( hdc, iLeft - iBottom / 2, iTop - iBottom / 2, iLeft + iBottom / 2, iTop + iBottom / 2, 0, 0, 0, 0 );

         iRet = 1;
         break;
      case GFX_FILLEDCIRCLE:
         color = RGB( iRight >> 16, ( iRight & 0xFF00 ) >> 8, iRight & 0xFF );
         SetGFXContext();

         Ellipse( hdc, iLeft - iBottom / 2, iTop - iBottom / 2, iLeft + iBottom / 2, iTop + iBottom / 2 );

         iRet = 1;
         break;
      case GFX_ELLIPSE:
         color = RGB( iColor >> 16, ( iColor & 0xFF00 ) >> 8, iColor & 0xFF );
         SetGFXContext();

         Arc( hdc, iLeft - iRight / 2, iTop - iBottom / 2, iLeft + iRight / 2, iTop + iBottom / 2, 0, 0, 0, 0 );

         iRet = 1;
         break;
      case GFX_FILLEDELLIPSE:
         color = RGB( iColor >> 16, ( iColor & 0xFF00 ) >> 8, iColor & 0xFF );
         SetGFXContext();

         Ellipse( hdc, iLeft - iRight / 2, iTop - iBottom / 2, iLeft + iRight / 2, iTop + iBottom / 2 );

         iRet = 1;
         break;
      case GFX_FLOODFILL:
         color = RGB( iBottom >> 16, ( iBottom & 0xFF00 ) >> 8, iBottom & 0xFF );
         SetGFXContext();

         FloodFill( hdc, iLeft, iTop, iColor );

         iRet = 1;
         break;
  }

  if ( bOut )
  {
     SelectObject( hdc, hOldPen );
     SelectObject( hdc, hOldBrush );
     DeleteObject( hBrush );
     DeleteObject( hPen );
  }

  return iRet;
}

void HB_GT_FUNC( gt_gfxText( int iTop, int iLeft, char *cBuf, int iColor, int iSize, int iWidth ) )
{
  HB_SYMBOL_UNUSED( iTop );
  HB_SYMBOL_UNUSED( iLeft );
  HB_SYMBOL_UNUSED( cBuf );
  HB_SYMBOL_UNUSED( iColor );
  HB_SYMBOL_UNUSED( iSize );
  HB_SYMBOL_UNUSED( iWidth );
}

/* ******** Graphics API end ******** */

//-------------------------------------------------------------------//

#ifdef HB_MULTI_GT

static void HB_GT_FUNC( gtFnInit( PHB_GT_FUNCS gt_funcs ) )
{
    HB_TRACE( HB_TR_DEBUG, ( "hb_gtFnInit( %p )", gt_funcs ) );

    gt_funcs->Init                  = HB_GT_FUNC( gt_Init               );
    gt_funcs->Exit                  = HB_GT_FUNC( gt_Exit               );
    gt_funcs->GetScreenWidth        = HB_GT_FUNC( gt_GetScreenWidth     );
    gt_funcs->GetScreenHeight       = HB_GT_FUNC( gt_GetScreenHeight    );
    gt_funcs->Col                   = HB_GT_FUNC( gt_Col                );
    gt_funcs->Row                   = HB_GT_FUNC( gt_Row                );
    gt_funcs->SetPos                = HB_GT_FUNC( gt_SetPos             );
    gt_funcs->AdjustPos             = HB_GT_FUNC( gt_AdjustPos          );
    gt_funcs->IsColor               = HB_GT_FUNC( gt_IsColor            );
    gt_funcs->GetCursorStyle        = HB_GT_FUNC( gt_GetCursorStyle     );
    gt_funcs->SetCursorStyle        = HB_GT_FUNC( gt_SetCursorStyle     );
    gt_funcs->DispBegin             = HB_GT_FUNC( gt_DispBegin          );
    gt_funcs->DispEnd               = HB_GT_FUNC( gt_DispEnd            );
    gt_funcs->DispCount             = HB_GT_FUNC( gt_DispCount          );
    gt_funcs->Puts                  = HB_GT_FUNC( gt_Puts               );
    gt_funcs->Replicate             = HB_GT_FUNC( gt_Replicate          );
    gt_funcs->RectSize              = HB_GT_FUNC( gt_RectSize           );
    gt_funcs->GetText               = HB_GT_FUNC( gt_GetText            );
    gt_funcs->PutText               = HB_GT_FUNC( gt_PutText            );
    gt_funcs->SetAttribute          = HB_GT_FUNC( gt_SetAttribute       );
    gt_funcs->Scroll                = HB_GT_FUNC( gt_Scroll             );
    gt_funcs->SetMode               = HB_GT_FUNC( gt_SetMode            );
    gt_funcs->GetBlink              = HB_GT_FUNC( gt_GetBlink           );
    gt_funcs->SetBlink              = HB_GT_FUNC( gt_SetBlink           );
    gt_funcs->Version               = HB_GT_FUNC( gt_Version            );
    gt_funcs->Box                   = HB_GT_FUNC( gt_Box                );
    gt_funcs->BoxD                  = HB_GT_FUNC( gt_BoxD               );
    gt_funcs->BoxS                  = HB_GT_FUNC( gt_BoxS               );
    gt_funcs->HorizLine             = HB_GT_FUNC( gt_HorizLine          );
    gt_funcs->VertLine              = HB_GT_FUNC( gt_VertLine           );
    gt_funcs->Suspend               = HB_GT_FUNC( gt_Suspend            );
    gt_funcs->Resume                = HB_GT_FUNC( gt_Resume             );
    gt_funcs->PreExt                = HB_GT_FUNC( gt_PreExt             );
    gt_funcs->PostExt               = HB_GT_FUNC( gt_PostExt            );
    gt_funcs->OutStd                = HB_GT_FUNC( gt_OutStd             );
    gt_funcs->OutErr                = HB_GT_FUNC( gt_OutErr             );
    gt_funcs->Tone                  = HB_GT_FUNC( gt_Tone               );
    gt_funcs->ExtendedKeySupport    = HB_GT_FUNC( gt_ExtendedKeySupport );
    gt_funcs->ReadKey               = HB_GT_FUNC( gt_ReadKey            );
    gt_funcs->info                  = HB_GT_FUNC( gt_info               );
    gt_funcs->SetClipboard          = HB_GT_FUNC( gt_SetClipboard       );
    gt_funcs->GetClipboard          = HB_GT_FUNC( gt_GetClipboard       );
    gt_funcs->GetClipboardSize      = HB_GT_FUNC( gt_GetClipboardSize   );
    gt_funcs->ProcessMessages       = HB_GT_FUNC( gt_ProcessMessages    );

    /* Graphics API */
    gt_funcs->gfxPrimitive          = HB_GT_FUNC( gt_gfxPrimitive );
}

//-------------------------------------------------------------------//

static void HB_GT_FUNC( mouseFnInit( PHB_GT_FUNCS gt_funcs ) )
{
    HB_TRACE( HB_TR_DEBUG, ( "hb_mouseFnInit( %p )", gt_funcs ) );

    gt_funcs->mouse_Init            = HB_GT_FUNC( mouse_Init            );
    gt_funcs->mouse_Exit            = HB_GT_FUNC( mouse_Exit            );
    gt_funcs->mouse_IsPresent       = HB_GT_FUNC( mouse_IsPresent       );
    gt_funcs->mouse_Show            = HB_GT_FUNC( mouse_Show            );
    gt_funcs->mouse_Hide            = HB_GT_FUNC( mouse_Hide            );
    gt_funcs->mouse_Col             = HB_GT_FUNC( mouse_Col             );
    gt_funcs->mouse_Row             = HB_GT_FUNC( mouse_Row             );
    gt_funcs->mouse_SetPos          = HB_GT_FUNC( mouse_SetPos          );
    gt_funcs->mouse_IsButtonPressed = HB_GT_FUNC( mouse_IsButtonPressed );
    gt_funcs->mouse_CountButton     = HB_GT_FUNC( mouse_CountButton     );
    gt_funcs->mouse_SetBounds       = HB_GT_FUNC( mouse_SetBounds       );
    gt_funcs->mouse_GetBounds       = HB_GT_FUNC( mouse_GetBounds       );
}

//-------------------------------------------------------------------//

static HB_GT_INIT gtInit = { HB_GT_DRVNAME( HB_GT_NAME ),
                             HB_GT_FUNC( gtFnInit ), HB_GT_FUNC( mouseFnInit ) };

HB_GT_ANNOUNCE( HB_GT_NAME );

HB_CALL_ON_STARTUP_BEGIN( _hb_startup_gt_Init_ )
   hb_gtRegister( &gtInit );
HB_CALL_ON_STARTUP_END( _hb_startup_gt_Init_ )

#if defined( HB_PRAGMA_STARTUP )
   #pragma startup _hb_startup_gt_Init_
#elif defined(_MSC_VER)
   #if _MSC_VER >= 1010
      #pragma data_seg( ".CRT$XIY" )
      #pragma comment( linker, "/Merge:.CRT=.data" )
   #else
      #pragma data_seg( "XIY" )
   #endif
   static HB_$INITSYM hb_vm_auto__hb_startup_gt_Init_ = _hb_startup_gt_Init_;
   #pragma data_seg()
#endif

#endif  /* HB_MULTI_GT */

//-------------------------------------------------------------------//
//
//                 Modeless Dialogs Implementation
//
//-------------------------------------------------------------------//

HB_EXPORT BOOL CALLBACK hb_wvt_gtDlgProcMLess( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
   int      iIndex, iType;
   long int bReturn = FALSE ;
   PHB_ITEM pFunc = NULL;
   PHB_DYNS pDynSym;

   iType = (int) NULL;

   for ( iIndex = 0; iIndex < WVT_DLGML_MAX; iIndex++ )
   {
      if ( ( _s.hDlgModeless[ iIndex ] != NULL ) && ( _s.hDlgModeless[ iIndex ] == hDlg ) )
      {
         if ( _s.pFunc[ iIndex ] != NULL )
         {
            pFunc = _s.pFunc[ iIndex ];
            iType = _s.iType[ iIndex ];
         }
         break;
      }
   }

   if ( pFunc )
   {
      switch ( iType )
      {
         case 1:  // Function Name
         {
            pDynSym = ( PHB_DYNS ) pFunc;
            hb_vmPushSymbol( pDynSym->pSymbol );
            hb_vmPushNil();
            hb_vmPushLong( ( ULONG ) hDlg    );
            hb_vmPushLong( ( UINT  ) message );
            hb_vmPushLong( ( ULONG ) wParam  );
            hb_vmPushLong( ( ULONG ) lParam  );
            hb_vmDo( 4 );
            bReturn = hb_itemGetNL( ( PHB_ITEM ) &HB_VM_STACK.Return );
            break;
         }

         case 2:  // Block
         {
            /* eval the codeblock */
            if ( _s.pFunc[ iIndex ]->type == HB_IT_BLOCK )
            {
              HB_ITEM hihDlg, himessage, hiwParam, hilParam;
              PHB_ITEM pReturn;

              hihDlg.type = HB_IT_NIL;
              hb_itemPutNL( &hihDlg, (ULONG) hDlg );

              himessage.type = HB_IT_NIL;
              hb_itemPutNL( &himessage, (ULONG) message );

              hiwParam.type = HB_IT_NIL;
              hb_itemPutNL( &hiwParam, (ULONG) wParam );

              hilParam.type = HB_IT_NIL;
              hb_itemPutNL( &hilParam, (ULONG) lParam );

              pReturn = hb_vmEvalBlockV( (PHB_ITEM) _s.pFunc[ iIndex ], 4, &hihDlg, &himessage, &hiwParam, &hilParam );
              bReturn = hb_itemGetNL( pReturn );
            }
            else
            {
              //internal error: missing codeblock
            }


            break;
         }
      }
   }

   switch( message )
   {
      case WM_COMMAND:
      {
         switch( LOWORD( wParam ) )
         {
            case IDOK:
            {
               DestroyWindow( hDlg );
               bReturn = TRUE;
            }
            break;

            case IDCANCEL:
            {
               DestroyWindow( hDlg );
               bReturn = FALSE;
            }
            break;
         }
      }
      break;

      case WM_CLOSE:
      {
         DestroyWindow( hDlg );
         bReturn = FALSE;
      }
      break;

      case WM_NCDESTROY:
      {
         if ( _s.pFunc[ iIndex ] != NULL && _s.iType[ iIndex ] == 2 )
         {
            HB_ITEM_UNLOCK( ( PHB_ITEM ) _s.pFunc[ iIndex ] );
            hb_itemClear( ( PHB_ITEM ) _s.pFunc[ iIndex ] );
         }
         _s.hDlgModeless[ iIndex ] = NULL;
         _s.pFunc[ iIndex ] = NULL;
         _s.iType[ iIndex ] = (int) NULL;
         bReturn = FALSE;
      }
      break;
   }

   return bReturn;
}

//-------------------------------------------------------------------//

HB_EXPORT BOOL CALLBACK hb_wvt_gtDlgProcModal( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
   int      iIndex, iType;
   long int bReturn = FALSE ;
   PHB_ITEM pFunc   = NULL;
   PHB_DYNS pDynSym;
   int      iFirst  = ( int ) lParam;

   if ( iFirst > 0 && iFirst <= WVT_DLGMD_MAX )
   {
      _s.hDlgModal[ iFirst-1 ] = hDlg ;
      SendMessage( hDlg, WM_INITDIALOG, 0, 0 );
      return ( bReturn );
   }

   iType = ( int ) NULL;

   for ( iIndex = 0; iIndex < WVT_DLGMD_MAX; iIndex++ )
   {
      if ( ( _s.hDlgModal[ iIndex ] != NULL ) && ( _s.hDlgModal[ iIndex ] == hDlg ) )
      {
         if ( _s.pFuncModal[ iIndex ] != NULL )
         {
            pFunc = _s.pFuncModal[ iIndex ];
            iType = _s.iTypeModal[ iIndex ];
         }
         break;
      }
   }

   if ( pFunc )
   {
      switch ( iType )
      {
         case 1:  // Function Name
         {
            pDynSym = ( PHB_DYNS ) pFunc;
            hb_vmPushSymbol( pDynSym->pSymbol );
            hb_vmPushNil();
            hb_vmPushLong( ( ULONG ) hDlg    );
            hb_vmPushLong( ( UINT  ) message );
            hb_vmPushLong( ( ULONG ) wParam  );
            hb_vmPushLong( ( ULONG ) lParam  );
            hb_vmDo( 4 );
            bReturn = hb_itemGetNL( ( PHB_ITEM ) &HB_VM_STACK.Return );
            break;
         }

         case 2:  // Block
         {
            /* eval the codeblock */
            if (_s.pFuncModal[ iIndex ]->type == HB_IT_BLOCK )
            {
              HB_ITEM hihDlg, himessage, hiwParam, hilParam;
              PHB_ITEM pReturn;

              hihDlg.type = HB_IT_NIL;
              hb_itemPutNL( &hihDlg, (ULONG) hDlg );

              himessage.type = HB_IT_NIL;
              hb_itemPutNL( &himessage, (ULONG) message );

              hiwParam.type = HB_IT_NIL;
              hb_itemPutNL( &hiwParam, (ULONG) wParam );

              hilParam.type = HB_IT_NIL;
              hb_itemPutNL( &hilParam, (ULONG) lParam );

              pReturn = hb_vmEvalBlockV( (PHB_ITEM) _s.pFuncModal[ iIndex ], 4, &hihDlg, &himessage, &hiwParam, &hilParam );
              bReturn = hb_itemGetNL( pReturn );
            }
            else
            {
              //internal error: missing codeblock
            }

            break;
         }
      }
   }

   switch( message )
   {
      case WM_COMMAND:
      {
         switch( LOWORD( wParam ) )
         {
            case IDOK:
            {
               EndDialog( hDlg, IDOK );
               bReturn = TRUE;
            }
            break;

            case IDCANCEL:
            {
               EndDialog( hDlg, IDCANCEL );
               bReturn = FALSE;
            }
            break;
         }
      }
      break;

      case WM_CLOSE:
      {
         EndDialog( hDlg, IDCANCEL );
         bReturn = FALSE;
      }
      break;

      case WM_NCDESTROY:
      {
         if ( _s.pFuncModal[ iIndex ] != NULL && _s.iTypeModal[ iIndex ] == 2 )
         {
            HB_ITEM_UNLOCK( ( PHB_ITEM ) _s.pFuncModal[ iIndex ] );
            hb_itemClear( ( PHB_ITEM ) _s.pFuncModal[ iIndex ] );
         }
         _s.hDlgModal[ iIndex ]   = NULL;
         _s.pFuncModal[ iIndex ]  = NULL;
         _s.iTypeModal[ iIndex ]  = ( int ) NULL;
         bReturn = FALSE;
      }
      break;
   }

   return bReturn;
}

//-------------------------------------------------------------------//
