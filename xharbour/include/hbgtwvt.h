#ifndef HB_WVT_H_
#define HB_WVT_H_

//-------------------------------------------------------------------//

/* NOTE: User programs should never call this layer directly! */

/* This definition has to be placed before #include "hbapigt.h" */

//-------------------------------------------------------------------//

#define HB_GT_NAME  WVT
//#define WVT_DEBUG

//-------------------------------------------------------------------//

#define HB_OS_WIN_32_USED

#include "hbset.h"
#include "hbapigt.h"
#include "hbapierr.h"
#include "hbapiitm.h"
#include "inkey.ch"
#include "error.ch"
#include "hbvm.h"
#include "hbstack.h"

#ifndef CINTERFACE
   #define CINTERFACE 1
#endif

#include <winuser.h>
#include <commctrl.h>
#if defined(__MINGW32__) || defined(__WATCOMC__) || defined(_MSC_VER)
   #include <ole2.h>
   #include <ocidl.h>

   #if defined(_MSC_VER)
      #include <olectl.h>
   #endif
#else
   #include <olectl.h>
#endif
#include <time.h>
#include <ctype.h>

#if defined( _MSC_VER )
  #include <conio.h>
#endif

//-------------------------------------------------------------------//

#define WVT_CHAR_QUEUE_SIZE  128
#define WVT_CHAR_BUFFER     1024
#define WVT_MAX_ROWS         256
#define WVT_MAX_COLS         256
#define WVT_DEFAULT_ROWS      25
#define WVT_DEFAULT_COLS      80

#define BLACK          RGB( 0x0 ,0x0 ,0x0  )
#define BLUE           RGB( 0x0 ,0x0 ,0x85 )
#define GREEN          RGB( 0x0 ,0x85,0x0  )
#define CYAN           RGB( 0x0 ,0x85,0x85 )
#define RED            RGB( 0x85,0x0 ,0x0  )
#define MAGENTA        RGB( 0x85,0x0 ,0x85 )
#define BROWN          RGB( 0x85,0x85,0x0  )
#define WHITE          RGB( 0xC6,0xC6,0xC6 )
#define LIGHT_GRAY     RGB( 0x60,0x60,0x60 )
#define BRIGHT_BLUE    RGB( 0x00,0x00,0xFF )
#define BRIGHT_GREEN   RGB( 0x60,0xFF,0x60 )
#define BRIGHT_CYAN    RGB( 0x60,0xFF,0xFF )
#define BRIGHT_RED     RGB( 0xF8,0x00,0x26 )
#define BRIGHT_MAGENTA RGB( 0xFF,0x60,0xFF )
#define YELLOW         RGB( 0xFF,0xFF,0x00 )
#define BRIGHT_WHITE   RGB( 0xFF,0xFF,0xFF )

#define WM_MY_UPDATE_CARET ( WM_USER + 0x0101 )

typedef struct global_data
{

  POINT     PTEXTSIZE;                 // size of the fixed width font
  BOOL      FixedFont;                 // TRUE if current font is a fixed font
  int       FixedSize[ WVT_MAX_COLS ]; // buffer for ExtTextOut() to emulate fixed pitch when Proportional font selected
  USHORT    ROWS;                      // number of displayable rows in window
  USHORT    COLS;                      // number of displayable columns in window
  COLORREF  foreground;                // forground colour
  COLORREF  background;                // background colour

  USHORT    BUFFERSIZE;                // size of the screen text buffer
  BYTE      byAttributes[ WVT_MAX_ROWS * WVT_MAX_COLS ]; // buffer with the attributes
  BYTE      byBuffer[ WVT_MAX_ROWS * WVT_MAX_COLS ];     // buffer with the text to be displayed on the screen
  BYTE      *pAttributes;              // pointer to buffer
  BYTE      *pBuffer;                  //   "     "    "
  POINT     caretPos;                  // the current caret position
  BOOL      CaretExist;                // TRUE if a caret has been created
  int       CaretSize;
  POINT     mousePos;                  // the last mousedown position
  BOOL      MouseMove;                 // Flag to say whether to return mouse movement events
  HWND      hWnd;                      // the window handle
  int       Keys[ WVT_CHAR_QUEUE_SIZE ]; // Array to hold the characters & events
  int       keyPointerIn;              // Offset into key array for character to be placed
  int       keyPointerOut;             // Offset into key array of next character to read
  BOOL      displayCaret;              // flag to indicate if caret is on
  RECT      RectInvalid;               // Invalid rectangle if DISPBEGIN() active
  HFONT     hFont;
  int       fontHeight;                // requested font height
  int       fontWidth ;                // requested font width
  int       fontWeight;                // Bold level
  int       fontQuality;
  char      fontFace[ LF_FACESIZE ];   // requested font face name LF_FACESIZE #defined in wingdi.h
  int       closeEvent;                // command to return ( in ReadKey ) on close
  int       shutdownEvent;             // command to return ( in ReadKey ) on shutdown
  int       LastMenuEvent;             // Last menu item selected
  int       MenuKeyEvent;              // User definable event number for windows menu command
  BOOL      CentreWindow;              // True if window is to be Reset into centre of window
  int       CodePage;                  // Code page to use for display characters
  BOOL      Win9X;                     // Flag to say if running on Win9X not NT/2000/XP
  BOOL      AltF4Close;                // Can use Alt+F4 to close application
  BOOL      InvalidateWindow;          // Flag for controlling whether to use ScrollWindowEx()
  BOOL      EnableShortCuts;           // Determines whether ALT key enables menu or system menu
  HPEN      penWhite;
  HPEN      penBlack;
  HPEN      penWhiteDim;
  HPEN      penDarkGray;
  HDC       hdc;
  PHB_DYNS  pSymWVT_PAINT;             // Stores pointer to WVT_PAINT function
} GLOBAL_DATA;

BOOL   HB_EXPORT hb_wvt_gtSetMenuKeyEvent( int iMenuKeyEvent );
BOOL   HB_EXPORT hb_wvt_gtSetCentreWindow( BOOL bCentre, BOOL bPaint );
void   HB_EXPORT hb_wvt_gtResetWindow( void );
BOOL   HB_EXPORT hb_wvt_gtSetCodePage( int iCodePage );
int    HB_EXPORT hb_wvt_gtGetLastMenuEvent( void );
void   HB_EXPORT hb_wvt_gtSetWindowTitle( char * title );
DWORD  HB_EXPORT hb_wvt_gtSetWindowIcon( int icon );
DWORD  HB_EXPORT hb_wvt_gtSetWindowIconFromFile( char *icon );
int    HB_EXPORT hb_wvt_gtGetWindowTitle( char *title, int length );
BOOL   HB_EXPORT hb_wvt_gtSetFont( char *fontFace, int height, int width, int Bold, int Quality );
void   HB_EXPORT hb_wvt_gtSetCloseEvent( int iEvent );
void   HB_EXPORT hb_wvt_gtSetShutdownEvent( int iEvent );
HWND   HB_EXPORT hb_wvt_gtGetWindowHandle( void );
void   HB_EXPORT hb_wvt_gtPostMessage( int message );
BOOL   HB_EXPORT hb_wvt_gtSetWindowPos( int left, int top );
BOOL   HB_EXPORT hb_wvt_gtSetAltF4Close( BOOL bCanClose );
void   HB_EXPORT hb_wvt_gtDoProcessMessages( void );
BOOL   HB_EXPORT hb_wvt_gtSetMouseMove( BOOL bHandleEvent );
BOOL   HB_EXPORT hb_wvt_gtEnableShortCuts( BOOL bEnable );
BOOL   HB_EXPORT hb_wvt_gtDrawImage( int x1, int y1, int wd, int ht, char * image );
HB_EXPORT GLOBAL_DATA * hb_wvt_gtGetGlobalData( void );

#endif
