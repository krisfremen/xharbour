/*
 * $Id: gtdos.c,v 1.22 2004/09/08 00:17:13 druzus Exp $
 */

/*
 * Harbour Project source code:
 * Video subsystem for DOS compilers
 *
 * Copyright 1999 {list of individual authors and e-mail addresses}
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
 * Copyright 1999-2001 Viktor Szakats <viktor.szakats@syenar.hu>
 *    hb_gt_CtrlBrkHandler()
 *    hb_gt_CtrlBrkRestore()
 *
 * Copyright 1999 David G. Holm <dholm@jsd-llc.com>
 *    hb_gt_ReadKey()
 *
 * Copyright 2000 Alejandro de Garate <alex_degarate@hotmail.com>
 *    vmode12x40()
 *    vmode25x40()
 *    vmode28x40()
 *    vmode50x40()
 *    vmode12x80()
 *    vmode25x80()
 *    vmode28x80()
 *    vmode43x80()
 *    vmode50x80()
 *    hb_gt_SetMode()
 *    hb_gt_GetDisplay()
 *
 * See doc/license.txt for licensing terms.
 *
 */

/*
 *  This module is based on VIDMGR by Andrew Clarke and modified for
 *  the Harbour project
 */

/* NOTE: User programs should never call this layer directly! */

/* This definition has to be placed before #include "hbapigt.h" */
#define HB_GT_NAME	DOS

#include "hbapi.h"
#include "hbapigt.h"
#include "hbapifs.h"
#include "hbset.h" /* For Ctrl+Break handling */
#include "hbvm.h" /* For Ctrl+Break handling */
#include "inkey.ch"

#include <string.h>
#include <time.h>
#include <conio.h>

#if defined(__DJGPP__)
   #include <pc.h>
   #include <sys\exceptn.h>
   #include <sys\farptr.h>
#elif defined(_MSC_VER)
   #include <signal.h>
#endif

/* For screen support */
#if defined(__POWERC) || (defined(__TURBOC__) && !defined(__BORLANDC__)) || (defined(__ZTC__) && !defined(__SC__))
   #define FAR far
#elif defined(HB_OS_DOS) && !defined(__DJGPP__) && !defined(__RSX32__) && !defined(__WATCOMC__)
   #define FAR _far
#else
   #define FAR
#endif

#if !defined(__DJGPP__)
   #ifndef MK_FP
      #define MK_FP( seg, off ) \
         ((void FAR *)(((ULONG)(seg) << 16)|(unsigned)(off)))
   #endif
#endif

static void hb_gt_xGetXY( USHORT cRow, USHORT cCol, BYTE * attr, BYTE * ch );
static void hb_gt_xPutch( USHORT cRow, USHORT cCol, BYTE attr, BYTE ch );

static char HB_GT_FUNC(gt_GetScreenMode( void ));
static void HB_GT_FUNC(gt_SetCursorSize( char start, char end ));
static void HB_GT_FUNC(gt_GetCursorSize( char * start, char * end ));

static char *s_clipboard = NULL;
static ULONG s_clipsize = 0;

#if defined(__WATCOMC__)
   #if defined(__386__)
      #define FAR
   #endif
   #include <signal.h>
#endif
#if !defined(__DJGPP__)
   static char FAR * scrnPtr;
   static char FAR * scrnStealth = NULL;
   static char FAR * hb_gt_ScreenAddress( void );
   static int    scrnVirtual = FALSE;
   static USHORT scrnWidth = 0;
   static USHORT scrnHeight = 0;
   static SHORT  scrnPosRow = -1;
   static SHORT  scrnPosCol = -1;
#else
   static char * scrnPtr = NULL;
   static int    scrnVirtual = FALSE;
   static USHORT scrnWidth = 0;
   static USHORT scrnHeight = 0;
   static SHORT  scrnPosRow = -1;
   static SHORT  scrnPosCol = -1;
#endif

static BOOL s_bBreak; /* Used to signal Ctrl+Break to hb_inkeyPoll() */
static USHORT s_uiDispCount;

static int s_iStdIn, s_iStdOut, s_iStdErr;

#if defined(__RSX32__)

static int kbhit( void )
{
   union REGS regs;

   regs.h.ah = 0x0B;
   HB_DOS_INT86( 0x21, &regs, &regs );

   return regs.HB_XREGS.ax;
}

#endif

#if !defined(__DJGPP__) && !defined(__RSX32__)
#if defined(__WATCOMC__) || defined(_MSC_VER)
static void HB_GT_FUNC(gt_CtrlBreak_Handler( int iSignal ))
{
   /* Ctrl-Break was pressed */
   /* NOTE: the layout of this function is forced by the compiler
    */
   HB_SYMBOL_UNUSED( iSignal );
   s_bBreak = TRUE;
}
#else
static int s_iOldCtrlBreak = 0;

static int HB_GT_FUNC(gt_CtrlBrkHandler( void ))
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_CtrlBrkHandler()"));
   s_bBreak = TRUE;
   return 1;
}
#endif

static void HB_GT_FUNC(gt_CtrlBrkRestore( void ))
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_CtrlBrkRestore()"));
   #if defined(__WATCOMC__)
      signal( SIGBREAK, SIG_DFL );
   #elif defined(_MSC_VER)
      signal( SIGINT, SIG_DFL );
   #elif !defined(__RSX32__)
      setcbrk( s_iOldCtrlBreak );
   #endif
}
#endif

void HB_GT_FUNC(gt_Init( int iFilenoStdin, int iFilenoStdout, int iFilenoStderr ))
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_Init()"));

   /*
   HB_SYMBOL_UNUSED( iFilenoStdin );
   HB_SYMBOL_UNUSED( iFilenoStdout );
   HB_SYMBOL_UNUSED( iFilenoStderr );
   */

   /* stdin && stdout && stderr */
   s_iStdIn  = iFilenoStdin;
   s_iStdOut = iFilenoStdout;
   s_iStdErr = iFilenoStderr;

   s_bBreak = FALSE;
   s_uiDispCount = 0;

   /* Set the Ctrl+Break handler [vszakats] */

#if defined(__DJGPP__)

   gppconio_init();
   __djgpp_hwint_flags |= 2;     /* Count Ctrl+Break instead of killing program */
   __djgpp_set_ctrl_c( 0 );      /* Disable Ctrl+C */
   __djgpp_set_sigquit_key( 0 ); /* Disable Ctrl+\ */

#elif defined(__WATCOMC__)

   signal( SIGBREAK, HB_GT_FUNC(gt_CtrlBreak_Handler ));
   atexit( HB_GT_FUNC(gt_CtrlBrkRestore) );

#elif defined(_MSC_VER)

   signal( SIGINT, HB_GT_FUNC(gt_CtrlBreak_Handler));
   atexit( HB_GT_FUNC(gt_CtrlBrkRestore));

#elif defined(__RSX32__)

   /* TODO */

#else

   ctrlbrk( HB_GT_FUNC(gt_CtrlBrkHandler));
   s_iOldCtrlBreak = getcbrk();
   setcbrk( 1 );
   atexit( HB_GT_FUNC(gt_CtrlBrkRestore));

#endif

   /* */

   scrnVirtual = FALSE;
#if !defined(__DJGPP__)
   scrnStealth = ( char * ) -1;
   scrnPtr = hb_gt_ScreenAddress();
#else
   scrnPtr = NULL;
#endif

   HB_GT_FUNC(mouse_Init());
}

void HB_GT_FUNC(gt_Exit( void ))
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_Exit()"));

   if ( s_clipboard != NULL )
   {
      hb_xfree( s_clipboard );
   }

   HB_GT_FUNC(mouse_Exit());
#if !defined(__DJGPP__)
  if( scrnStealth != ( char * ) -1 )
     hb_xfree( scrnStealth );
#endif
}

int HB_GT_FUNC(gt_ExtendedKeySupport())
{
   return 0;
}

int HB_GT_FUNC(gt_ReadKey( HB_inkey_enum eventmask ))
{
   int ch = 0;

   HB_TRACE(HB_TR_DEBUG, ("hb_gt_ReadKey(%d)", (int) eventmask));

#if defined(__DJGPP__)
   /* Check to see if Ctrl+Break has been detected */
   if( __djgpp_cbrk_count )
   {
      __djgpp_cbrk_count = 0; /* Indicate that Ctrl+Break has been handled */
      ch = HB_BREAK_FLAG; /* Note that Ctrl+Break was pressed */
   }
#else
   /* First check for Ctrl+Break, which is handled by gt/gtdos.c,
      with the exception of the DJGPP compiler */
   if( s_bBreak )
   {
      s_bBreak = FALSE; /* Indicate that Ctrl+Break has been handled */
      ch = HB_BREAK_FLAG; /* Note that Ctrl+Break was pressed */
   }
#endif
   else if( kbhit() )
   {
      /* A key code is available in the BIOS keyboard buffer, so read it */
#if defined(__DJGPP__)
      if( eventmask & INKEY_RAW ) ch = getxkey();
      else ch = getkey();
      if( ch == 256 )
         /* Ignore Ctrl+Break, because it is being handled as soon as it
            happens (see above) rather than waiting for it to show up in
            the keyboard input queue */
         ch = -1;
#else
      /* A key code is available in the BIOS keyboard buffer */
      ch = getch();                  /* Get the key code */
      if( ch == 0 && kbhit() )
      {
         /* It was a function key lead-in code, so read the actual
            function key and then offset it by 256 */
         ch = getch() + 256;
      }
      else if( ch == 224 && kbhit() )
      {
         /* It was an extended function key lead-in code, so read
            the actual function key and then offset it by 256,
            unless extended keyboard events are allowed, in which
            case offset it by 512 */
         if( eventmask & INKEY_RAW ) ch = getch() + 512;
         else ch = getch() + 256;
      }
#endif
   }

   /* Perform key translations */
   switch( ch )
   {
      case -1:  /* No key available */
         return 0;
      case 328:  /* Up arrow */
         ch = K_UP;
         break;
      case 336:  /* Down arrow */
         ch = K_DOWN;
         break;
      case 331:  /* Left arrow */
         ch = K_LEFT;
         break;
      case 333:  /* Right arrow */
         ch = K_RIGHT;
         break;
      case 327:  /* Home */
         ch = K_HOME;
         break;
      case 335:  /* End */
         ch = K_END;
         break;
      case 329:  /* Page Up */
         ch = K_PGUP;
         break;
      case 337:  /* Page Down */
         ch = K_PGDN;
         break;
      case 371:  /*  Ctrl + Left arrow */
         ch = K_CTRL_LEFT;
         break;
      case 372:  /* Ctrl + Right arrow */
         ch = K_CTRL_RIGHT;
         break;
      case 375:  /* Ctrl + Home */
         ch = K_CTRL_HOME;
         break;
      case 373:  /* Ctrl + End */
         ch = K_CTRL_END;
         break;
      case 388:  /* Ctrl + Page Up */
         ch = K_CTRL_PGUP;
         break;
      case 374:  /* Ctrl + Page Down */
         ch = K_CTRL_PGDN;
         break;
      case 338:  /* Insert */
         ch = K_INS;
         break;
      case 339:  /* Delete */
         ch = K_DEL;
         break;
      case 315:  /* F1 */
         ch = K_F1;
         break;
      case 316:  /* F2 */
      case 317:  /* F3 */
      case 318:  /* F4 */
      case 319:  /* F5 */
      case 320:  /* F6 */
      case 321:  /* F7 */
      case 322:  /* F8 */
      case 323:  /* F9 */
      case 324:  /* F10 */
         ch = 315 - ch;
         break;
      case 340:  /* Shift + F1 */
      case 341:  /* Shift + F2 */
      case 342:  /* Shift + F3 */
      case 343:  /* Shift + F4 */
      case 344:  /* Shift + F5 */
      case 345:  /* Shift + F6 */
      case 346:  /* Shift + F7 */
      case 347:  /* Shift + F8 */
      case 348:  /* Shift + F9 */
      case 349:  /* Shift + F10 */
      case 350:  /* Ctrl + F1 */
      case 351:  /* Ctrl + F2 */
      case 352:  /* Ctrl + F3 */
      case 353:  /* Ctrl + F4 */
      case 354:  /* Ctrl + F5 */
      case 355:  /* Ctrl + F6 */
      case 356:  /* Ctrl + F7 */
      case 357:  /* Ctrl + F8 */
      case 358:  /* Ctrl + F9 */
      case 359:  /* Ctrl + F10 */
      case 360:  /* Alt + F1 */
      case 361:  /* Alt + F2 */
      case 362:  /* Alt + F3 */
      case 363:  /* Alt + F4 */
      case 364:  /* Alt + F5 */
      case 365:  /* Alt + F6 */
      case 366:  /* Alt + F7 */
      case 367:  /* Alt + F8 */
      case 368:  /* Alt + F9 */
      case 369:  /* Alt + F10 */
         ch = 330 - ch;
         break;
      case 389:  /* F11 */
      case 390:  /* F12 */
      case 391:  /* Shift + F11 */
      case 392:  /* Shift + F12 */
      case 393:  /* Ctrl + F11 */
      case 394:  /* Ctrl + F12 */
      case 395:  /* Alt + F11 */
      case 396:  /* Alt + F12 */
         ch = 349 - ch;
   }

   return ch;
}

BOOL HB_GT_FUNC(gt_AdjustPos( BYTE * pStr, ULONG ulLen ))
{
   union REGS regs;

   HB_TRACE(HB_TR_DEBUG, ("hb_gt_AdjustPos(%s, %lu)", pStr, ulLen ));

   HB_SYMBOL_UNUSED( pStr );
   HB_SYMBOL_UNUSED( ulLen );

   regs.h.ah = 0x03;
   regs.h.bh = 0;
   HB_DOS_INT86( 0x10, &regs, &regs );

   hb_gtSetPos( regs.h.dh, regs.h.dl );

   return TRUE;
}

BOOL HB_GT_FUNC(gt_IsColor( void ))
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_IsColor()"));

   return HB_GT_FUNC(gt_GetScreenMode()) != 7;
}

#if !defined(__DJGPP__)
static char FAR * hb_gt_ScreenAddress()
{
   char FAR * ptr;

   HB_TRACE(HB_TR_DEBUG, ("hb_gt_ScreenAddress()"));

   #if defined(__WATCOMC__) && defined(__386__)
      if( hb_gt_IsColor() )
         ptr = ( char * ) ( 0xB800 << 4 );
      else
         ptr = ( char * )( 0xB000 << 4 );
   #else
      if( hb_gt_IsColor() )
         ptr = ( char FAR * ) MK_FP( 0xB800, 0x0000 );
      else
         ptr = ( char FAR * ) MK_FP( 0xB000, 0x0000 );
   #endif

   return ptr;
}
#endif

#if !defined(__DJGPP__)
static char FAR * hb_gt_ScreenPtr( USHORT cRow, USHORT cCol )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_ScreenPtr(%hu, %hu)", cRow, cCol));

   return scrnPtr + ( cRow * hb_gt_GetScreenWidth() * 2 ) + ( cCol * 2 );
}
#else
static char * hb_gt_ScreenPtr( USHORT cRow, USHORT cCol )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_ScreenPtr(%hu, %hu)", cRow, cCol));

   return scrnPtr + ( cRow * hb_gt_GetScreenWidth() * 2 ) + ( cCol * 2 );
}
#endif

static char HB_GT_FUNC(gt_GetScreenMode( void ))
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_GetScreenMode()"));

#if defined(__WATCOMC__) && defined(__386__)
   return *( ( char * ) 0x0449 );
#elif defined(__DJGPP__)
   return _farpeekb( 0x0040, 0x0049 );
#else
   return *( ( char FAR * ) MK_FP( 0x0040, 0x0049 ) );
#endif
}

USHORT HB_GT_FUNC(gt_GetScreenWidth( void ))
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_GetScreenWidth()"));

#if defined(__WATCOMC__) && defined(__386__)
   if( scrnWidth == 0 )
     scrnWidth = ( USHORT ) *( ( char * ) 0x044A );
/*   return ( USHORT ) *( ( char * ) 0x044A ); */
#elif defined(__DJGPP__)
   if( scrnWidth == 0 )
     scrnWidth = ( USHORT ) _farpeekb( 0x0040, 0x004A );
/*   return ( USHORT ) _farpeekb( 0x0040, 0x004A ); */
#else
   if( scrnWidth == 0 )
     scrnWidth = ( USHORT ) *( ( char FAR * ) MK_FP( 0x0040, 0x004A ) );
/*   return ( USHORT ) *( ( char FAR * ) MK_FP( 0x0040, 0x004A ) ); */
#endif
   return scrnWidth;
}

USHORT HB_GT_FUNC(gt_GetScreenHeight( void ))
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_GetScreenHeigth()"));

#if defined(__WATCOMC__) && defined(__386__)
   if( scrnHeight == 0 )
     scrnHeight = ( USHORT ) ( char ) ( *( ( char * ) 0x0484 ) + 1 );
/*   return ( USHORT ) ( char ) ( *( ( char * ) 0x0484 ) + 1 ); */
#elif defined(__DJGPP__)
   if( scrnHeight == 0 )
     scrnHeight = ( USHORT ) _farpeekb( 0x0040, 0x0084 ) + 1;
/*   return ( USHORT ) _farpeekb( 0x0040, 0x0084 ) + 1; */
#else
   if( scrnHeight == 0 )
     scrnHeight = ( USHORT ) ( ( char ) ( *( ( char FAR * ) MK_FP( 0x0040, 0x0084 ) ) + 1 ) );
/*   return ( USHORT ) ( ( char ) ( *( ( char FAR * ) MK_FP( 0x0040, 0x0084 ) ) + 1 ) ); */
#endif
   return scrnHeight;
}

void HB_GT_FUNC(gt_SetPos( SHORT iRow, SHORT iCol, SHORT iMethod ))
{
   union REGS regs;

   HB_TRACE(HB_TR_DEBUG, ("hb_gt_SetPos(%hd, %hd, %hd)", iRow, iCol, iMethod));

   HB_SYMBOL_UNUSED( iMethod );

   if( scrnVirtual )
   {
      scrnPosRow = iRow;
      scrnPosCol = iCol;
   }
   else
   {
      regs.h.ah = 0x02;
      regs.h.bh = 0;
      regs.h.dh = ( BYTE ) iRow;
      regs.h.dl = ( BYTE ) iCol;
      HB_DOS_INT86( 0x10, &regs, &regs );
   }
}

static void HB_GT_FUNC(gt_SetCursorSize( char start, char end ))
{
   union REGS regs;

   HB_TRACE(HB_TR_DEBUG, ("hb_gt_SetCursorSize(%d, %d)", (int) start, (int) end));

   regs.h.ah = 0x01;
   regs.h.ch = start;
   regs.h.cl = end;
   HB_DOS_INT86( 0x10, &regs, &regs );
}

static void HB_GT_FUNC(gt_GetCursorSize( char * start, char *end ))
{
   union REGS regs;

   HB_TRACE(HB_TR_DEBUG, ("hb_gt_GetCursorSize(%p, %p)", start, end));

   regs.h.ah = 0x03;
   regs.h.bh = 0;
   HB_DOS_INT86( 0x10, &regs, &regs );
   *start = regs.h.ch;
   *end = regs.h.cl;
}

USHORT HB_GT_FUNC(gt_GetCursorStyle( void ))
{
   char start, end;
   int rc;

   HB_TRACE(HB_TR_DEBUG, ("hb_gt_GetCursorStyle()"));

   HB_GT_FUNC(gt_GetCursorSize( &start, &end ));

   if( ( start == 32 ) && ( end == 32 ) )
      rc = SC_NONE;

   else if( ( start == 6 ) && ( end == 7 ) )
      rc = SC_NORMAL;

   else if( ( start == 4 ) && ( end == 7 ) )
      rc = SC_INSERT;

   else if( ( start == 0 ) && ( end == 7 ) )
      rc = SC_SPECIAL1;

   else if( ( start == 0 ) && ( end == 3 ) )
      rc = SC_SPECIAL2;

   else
      rc = SC_NONE;

   return rc;
}

void HB_GT_FUNC(gt_SetCursorStyle( USHORT style ))
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_SetCursorStyle(%hu)", style));

   switch( style )
   {
   case SC_NONE:
      HB_GT_FUNC(gt_SetCursorSize( 32, 32 ));
      break;

   case SC_NORMAL:
      HB_GT_FUNC(gt_SetCursorSize( 6, 7 ));
      break;

   case SC_INSERT:
      HB_GT_FUNC(gt_SetCursorSize( 4, 7 ));
      break;

   case SC_SPECIAL1:
      HB_GT_FUNC(gt_SetCursorSize( 0, 7 ));
      break;

   case SC_SPECIAL2:
      HB_GT_FUNC(gt_SetCursorSize( 0, 3 ));
      break;

   default:
      break;
   }
}

static void hb_gt_xGetXY( USHORT cRow, USHORT cCol, BYTE * attr, BYTE * ch )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_xGetXY(%hu, %hu, %p, %p", cRow, cCol, ch, attr));

#if defined(__DJGPP__TEXT)
   {
     if( scrnVirtual )
     {
        char *p;
        p = hb_gt_ScreenPtr( cRow, cCol );
        *ch = *p;
        *attr = *( p + 1 );
     }
     else
     {
        short ch_attr;
        gettext( cCol + 1, cRow + 1, cCol + 1, cRow + 1, &ch_attr );
        *ch = ch_attr >> 8;
        *attr = ch_attr & 0xFF;
     }
   }
#elif defined(__DJGPP__)
   {
     if( scrnVirtual )
     {
        char *p;
        p = hb_gt_ScreenPtr( cRow, cCol );
        *ch = *p;
        *attr = *( p + 1 );
     }
     else
     {
        int nCh, nAttr;
	
        ScreenGetChar( &nCh, &nAttr, cCol, cRow );
	*ch = nCh;
	*attr = nAttr;
     }
   }
#else
   {
     char FAR *p;
     p = hb_gt_ScreenPtr( cRow, cCol );
     *ch = *p;
     *attr = *( p + 1 );
   }
#endif
}

static void hb_gt_xPutch( USHORT cRow, USHORT cCol, BYTE attr, BYTE ch )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_xPutch(%hu, %hu, %d, %d", cRow, cCol, (int) attr, (int) ch));

#if defined(__DJGPP__TEXT)
   {
     if( scrnVirtual )
     {
        char *p;
        p          = hb_gt_ScreenPtr( cRow, cCol );
        *p         = ch;
        *( p + 1 ) = attr;
     }
     else
     {
        LONG ch_attr;
        ch_attr = ( ch << 8 ) | attr;
        puttext( cCol + 1, cRow + 1, cCol + 1, cRow + 1, &ch_attr );
     }
   }
#elif defined(__DJGPP__)
   {
     if( scrnVirtual )
     {
        char *p;
        p          = hb_gt_ScreenPtr( cRow, cCol );
        *p         = ch;
        *( p + 1 ) = attr;
     }
     else
        ScreenPutChar( ch, attr, cCol, cRow );
   }
#else
   {
     USHORT FAR * p = (USHORT FAR *) hb_gt_ScreenPtr( cRow, cCol );
     *p = (attr << 8) + ch;
   }
#endif
}

void HB_GT_FUNC(gt_Puts( USHORT cRow, USHORT cCol, BYTE attr, BYTE *str, ULONG len ))
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_Puts(%hu, %hu, %d, %p, %lu", cRow, cCol, (int) attr, str, len));

#if defined(__DJGPP__TEXT)
   {
     ULONG i;

     if( scrnVirtual )
     {
        for( i=0; i<len; i++ )
        {
          char *p;
          p          = hb_gt_ScreenPtr( cRow, cCol++ );
          *p         = str[ i ];
          *( p + 1 ) = attr;
        }
     else
     {
        int bottom, left, right, top;
        int width;
        BYTE * ch_attr;
        BYTE * ptr;

        i = len;
        left = cCol;
        top = cRow;
        width = hb_gt_GetScreenWidth();
        ptr = ch_attr = hb_xgrab( i << 1 );
        while( i-- )
          {
             *ptr++ = *str++;
             *ptr++ = attr;
          }
        i = len - 1; /* We want end position, not next cursor position */
        right = left;
        bottom = top;
        if( right + i > width - 1 )
          {
            /*
             * Calculate end row position and the remainder size for the
             * end column adjust.
             */
            bottom += ( i / width );
            i = i % width;
          }
        right += i;
        if( right > width - 1 )
          {
            /* Column movement overflows into next row */
            bottom++;
            right -= width;
          }
        puttext( left + 1, top + 1, right + 1, bottom + 1, ch_attr );
        hb_xfree( ch_attr );
     }
   }
#elif defined(__DJGPP__)
   {
      ULONG i;

      for( i=0; i<len; i++ )
      {
        if( scrnVirtual )
        {
           char *p;
           p          = hb_gt_ScreenPtr( cRow, cCol++ );
           *p         = str[ i ];
           *( p + 1 ) = attr;
        }
        else
           ScreenPutChar( str[ i ], attr, cCol++, cRow );
      }
   }
#else
   {
      USHORT FAR *p;
      register USHORT byAttr = attr << 8;

      p = (USHORT FAR *) hb_gt_ScreenPtr( cRow, cCol );
      while( len-- )
      {
         *p++ = byAttr + (*str++);
      }
   }
#endif
}

int HB_GT_FUNC(gt_RectSize( USHORT rows, USHORT cols ))
{
   return rows * cols * 2;
}

void HB_GT_FUNC(gt_GetText( USHORT usTop, USHORT usLeft, USHORT usBottom, USHORT usRight, BYTE * dest ))
{
   
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_GetText(%hu, %hu, %hu, %hu, %p)", usTop, usLeft, usBottom, usRight, dest));


#if defined(__DJGPP__TEXT) || defined(__DJGPP__)
   {
     if( scrnVirtual )
     {
        USHORT x, y;

        for( y = usTop; y <= usBottom; y++ )
          {
            for( x = usLeft; x <= usRight; x++ )
              {
                hb_gt_xGetXY( y, x, dest + 1, dest );
                dest += 2;
              }
          }
     }
     else
        gettext( usLeft + 1, usTop + 1, usRight + 1, usBottom + 1, dest );
   }
#else
   {
     USHORT x, y;

     for( y = usTop; y <= usBottom; y++ )
       {
         for( x = usLeft; x <= usRight; x++ )
           {
             hb_gt_xGetXY( y, x, dest + 1, dest );
             dest += 2;
           }
       }
   }
#endif
}

void HB_GT_FUNC(gt_PutText( USHORT usTop, USHORT usLeft, USHORT usBottom, USHORT usRight, BYTE * srce ))
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_PutText(%hu, %hu, %hu, %hu, %p", usTop, usLeft, usBottom, usRight, srce));

#if defined(__DJGPP__TEXT) || defined(__DJGPP__)
   {
     if( scrnVirtual )
     {
        USHORT x, y;

        for( y = usTop; y <= usBottom; y++ )
          {
            for( x = usLeft; x <= usRight; x++ )
              {
                hb_gt_xPutch( y, x, *( srce + 1 ), *srce );
                srce += 2;
              }
          }
     }
     else
        puttext( usLeft + 1, usTop + 1, usRight + 1, usBottom + 1, srce );
   }
#else
   {
     USHORT x, y;

     for( y = usTop; y <= usBottom; y++ )
       {
         for( x = usLeft; x <= usRight; x++ )
           {
             hb_gt_xPutch( y, x, *( srce + 1 ), *srce );
             srce += 2;
           }
       }
   }
#endif
}

void HB_GT_FUNC(gt_SetAttribute( USHORT usTop, USHORT usLeft, USHORT usBottom, USHORT usRight, BYTE attr ))
{
   USHORT x, y;

   HB_TRACE(HB_TR_DEBUG, ("hb_gt_SetAttribute(%hu, %hu, %hu, %hu, %d", usTop, usLeft, usBottom, usRight, (int) attr));

   for( y = usTop; y <= usBottom; y++ )
   {
      BYTE scratchattr;
      BYTE ch;

      for( x = usLeft; x <= usRight; x++ )
      {
         hb_gt_xGetXY( y, x, &scratchattr, &ch );
         hb_gt_xPutch( y, x, attr, ch );
      }
   }
}

SHORT HB_GT_FUNC(gt_Col( void ))
{
   union REGS regs;

   HB_TRACE(HB_TR_DEBUG, ("hb_gt_Col()"));

   if( scrnVirtual )
   {
      return scrnPosCol;
   }
   else
   {
      regs.h.ah = 0x03;
      regs.h.bh = 0;
      HB_DOS_INT86( 0x10, &regs, &regs );

      return regs.h.dl;
   }
}

SHORT HB_GT_FUNC(gt_Row( void ))
{
   union REGS regs;

   HB_TRACE(HB_TR_DEBUG, ("hb_gt_Row()"));

   if( scrnVirtual )
   {
      return scrnPosRow;
   }
   else
   {
      regs.h.ah = 0x03;
      regs.h.bh = 0;
      HB_DOS_INT86( 0x10, &regs, &regs );

      return regs.h.dh;
   }
}

void HB_GT_FUNC(gt_Scroll( USHORT usTop, USHORT usLeft, USHORT usBottom, USHORT usRight, BYTE attr, SHORT sVert, SHORT sHoriz ))
{
   int iRows = sVert, iCols = sHoriz;

   /* NOTE: 'SHORT' is used intentionally to correctly compile
   *  with C++ compilers
   */
   SHORT usRow, usCol;
   UINT uiSize;   /* gtRectSize returns int */
   int iLength = ( usRight - usLeft ) + 1;
   int iCount, iColOld, iColNew, iColSize;

   HB_TRACE(HB_TR_DEBUG, ("hb_gt_Scroll(%hu, %hu, %hu, %hu, %d, %hd, %hd)", usTop, usLeft, usBottom, usRight, (int) attr, sVert, sHoriz));

   hb_gtGetPos( &usRow, &usCol );

   if( hb_gtRectSize( usTop, usLeft, usBottom, usRight, &uiSize ) == 0 )
   {
      /* NOTE: 'unsigned' is used intentionally to correctly compile
       * with C++ compilers
       */
      unsigned char * fpBlank = ( unsigned char * ) hb_xgrab( iLength );
      unsigned char * fpBuff = ( unsigned char * ) hb_xgrab( iLength * 2 );

      memset( fpBlank, ' ', iLength );

      iColOld = iColNew = usLeft;
      if( iCols >= 0 )
      {
         iColOld += iCols;
         iColSize = ( int ) ( usRight - usLeft );
         iColSize -= iCols;
      }
      else
      {
         iColNew -= iCols;
         iColSize = ( int ) ( usRight - usLeft );
         iColSize += iCols;
      }

      for( iCount = ( iRows >= 0 ? usTop : usBottom );
           ( iRows >= 0 ? iCount <= usBottom : iCount >= usTop );
           ( iRows >= 0 ? iCount++ : iCount-- ) )
      {
         int iRowPos = iCount + iRows;

         /* Blank the scroll region in the current row */
         hb_gt_Puts( iCount, usLeft, attr, fpBlank, iLength );

         if( ( iRows || iCols ) && iRowPos <= usBottom && iRowPos >= usTop )
         {
            /* Read the text to be scrolled into the current row */
            hb_gt_GetText( iRowPos, iColOld, iRowPos, iColOld + iColSize, fpBuff );

            /* Write the scrolled text to the current row */
            hb_gt_PutText( iCount, iColNew, iCount, iColNew + iColSize, fpBuff );
         }
      }

      hb_xfree( fpBlank );
      hb_xfree( fpBuff );
   }

   hb_gtSetPos( usRow, usCol );
}

void HB_GT_FUNC(gt_DispBegin( void ))
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_DispBegin()"));

/* ptucker */
#if !defined(__DJGPP__)
   if( ++s_uiDispCount == 1 )
   {
      char FAR * ptr;
      ULONG nSize;

      nSize = hb_gt_GetScreenWidth() * hb_gt_GetScreenHeight() * 2;

      ptr = scrnPtr;
      if( ( scrnPtr = scrnStealth ) == ( char * ) -1 )
         scrnPtr = ( char FAR * ) hb_xgrab( nSize );
      scrnStealth = ptr;
      memcpy( ( void * ) scrnPtr, ( void * ) ptr, nSize );
      hb_gtGetPos( &scrnPosRow, &scrnPosCol );

   }
#else
   if( ++s_uiDispCount == 1 )
   {
      ULONG nSize;

      nSize = hb_gt_GetScreenWidth() * hb_gt_GetScreenHeight() * 2;

      hb_gtGetPos( &scrnPosRow, &scrnPosCol );

      scrnPtr = ( char * ) hb_xgrab( nSize );
      ScreenRetrieve( ( void * ) scrnPtr );

      scrnVirtual = TRUE;
   }
#endif
}

void HB_GT_FUNC(gt_DispEnd( void ))
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_DispEnd()"));

/* ptucker */
#if !defined(__DJGPP__)
   if( --s_uiDispCount == 0 )
   {
      char FAR * ptr;
      ULONG nSize;

      nSize = hb_gt_GetScreenWidth() * hb_gt_GetScreenHeight() * 2;

      ptr = scrnPtr;
      scrnPtr = scrnStealth;
      scrnStealth = ptr;
      memcpy( ( void * ) scrnPtr, ( void * )ptr, nSize );
      hb_gt_SetPos( scrnPosRow, scrnPosCol, 0 );
   }
#else
   if( --s_uiDispCount == 0 )
   {
      ULONG nSize;

      nSize = hb_gt_GetScreenWidth() * hb_gt_GetScreenHeight() * 2;

      ScreenUpdate( ( void * ) scrnPtr );
/*      dosmemput( ( void * ) scrnPtr, nSize, hb_gt_ScreenAddress() * 16 );*/

      scrnVirtual = FALSE;
      hb_xfree( scrnPtr );

      hb_gt_SetPos( scrnPosRow, scrnPosCol, 0 );
   }
#endif
}

BOOL HB_GT_FUNC(gt_GetBlink())
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_GetBlink()"));

#if defined(__WATCOMC__) && defined(__386__)
   return *( ( char * ) 0x0465 ) & 0x10;
#elif defined(__DJGPP__)
   return _farpeekb( 0x0040, 0x0065 ) & 0x10;
#else
   return *( ( char FAR * ) MK_FP( 0x0040, 0x0065 ) ) &0x10;
#endif
}

void HB_GT_FUNC(gt_SetBlink( BOOL bBlink ))
{
   union REGS regs;

   HB_TRACE(HB_TR_DEBUG, ("hb_gt_SetBlink(%d)", (int) bBlink));

   regs.h.ah = 0x10;
   regs.h.al = 0x03;
   regs.h.bh = 0;
   regs.h.bl = bBlink;
   HB_DOS_INT86( 0x10, &regs, &regs );
}

void HB_GT_FUNC(gt_Tone( double dFrequency, double dDuration ))
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_Tone(%lf, %lf)", dFrequency, dDuration));

   dFrequency = HB_MIN( HB_MAX( 0.0, dFrequency ), 32767.0 );

   /* sync with internal clock with very small time period */
   hb_idleSleep( 0.01 );
#if defined(__BORLANDC__) || defined(__WATCOMC__)
   sound( ( unsigned ) dFrequency );
#elif defined(__DJGPP__)
   sound( ( int ) dFrequency );
#endif

   /* The conversion from Clipper (DOS) timer tick units to
      milliseconds is * 1000.0 / 18.2. */
   hb_idleSleep( dDuration / 18.2 );

#if defined(__BORLANDC__) || defined(__WATCOMC__)
   nosound();
#elif defined(__DJGPP__)
   sound( 0 );
#endif
}

char * HB_GT_FUNC(gt_Version( int iType ))
{
   HB_TRACE( HB_TR_DEBUG, ( "hb_gt_Version()" ) );

   if ( iType == 0 )
      return HB_GT_DRVNAME( HB_GT_NAME );

   return "Harbour Terminal: DOS console";
}

USHORT HB_GT_FUNC(gt_DispCount())
{
   return s_uiDispCount;
}

void HB_GT_FUNC(gt_Replicate( USHORT uiRow, USHORT uiCol, BYTE byAttr, BYTE byChar, ULONG nLength ))
{
   HB_TRACE(HB_TR_DEBUG, ("hb_gt_Replicate(%hu, %hu, %i, %i, %lu)", uiRow, uiCol, byAttr, byChar, nLength));

#if defined(__DJGPP__TEXT)
   {
     if( scrnVirtual )
     {
        while( nLength-- )
        {
           char *p;
           p          = hb_gt_ScreenPtr( uiRow, uiCol++ );
           *p         = byChar;
           *( p + 1 ) = byAttr;
        }
     else
     {
        int i;
        int bottom, left, right, top;
        int width;
        BYTE * ch_attr;
        BYTE * ptr;

        i = ( int ) nLength;
        left = cCol;
        top = cRow;
        width = hb_gt_GetScreenWidth();
        ptr = ch_attr = hb_xgrab( i * 2 );
        while( i-- )
          {
            *ptr++ = byChar;
            *ptr++ = byAttr;
          }
        i = nLength - 1; /* We want end position, not next cursor position */
        right = left;
        bottom = top;
        if( right + i > width - 1 )
          {
            /*
             * Calculate end row position and the remainder size for the
             * end column adjust.
             */
            bottom += ( i / width );
            i = i % width;
          }
        right += i;
        if( right > width - 1 )
          {
            /* Column movement overflows into next row */
            bottom++;
            right -= width;
          }
        puttext( left + 1, top + 1, right + 1, bottom + 1, ch_attr );
        hb_xfree( ch_attr );
     }
   }
#elif defined(__DJGPP__)
   {
      while( nLength-- )
      {
         if( scrnVirtual )
         {
            char *p;
            p          = hb_gt_ScreenPtr( uiRow, uiCol++ );
            *p         = byChar;
            *( p + 1 ) = byAttr;
         }
         else
            ScreenPutChar( byChar, byAttr, uiCol++, uiRow );
      }
   }
#else
   {
      USHORT FAR *p;
      USHORT byte = (byAttr << 8) + byChar;

      p = (USHORT FAR *) hb_gt_ScreenPtr( uiRow, uiCol );
      while( nLength-- )
      {
         *p++ = byte;
      }
   }
#endif
}

USHORT HB_GT_FUNC(gt_Box( SHORT Top, SHORT Left, SHORT Bottom, SHORT Right,
                  BYTE * szBox, BYTE byAttr ))
{
   USHORT ret = 1;
   SHORT Row;
   SHORT Col;
   SHORT Height;
   SHORT Width;

   if( ( Left   >= 0 && Left   < hb_gt_GetScreenWidth()  )  ||
       ( Right  >= 0 && Right  < hb_gt_GetScreenWidth()  )  ||
       ( Top    >= 0 && Top    < hb_gt_GetScreenHeight() )  ||
       ( Bottom >= 0 && Bottom < hb_gt_GetScreenHeight() ) )
   {

      /* Ensure that box is drawn from top left to bottom right. */
      if( Top > Bottom )
      {
         SHORT tmp = Top;
         Top = Bottom;
         Bottom = tmp;
      }
      if( Left > Right )
      {
         SHORT tmp = Left;
         Left = Right;
         Right = tmp;
      }

      /* Draw the box or line as specified */
      Height = Bottom - Top + 1;
      Width  = Right - Left + 1;

      hb_gt_DispBegin();

      if( Height > 1 && Width > 1 && Top >= 0 && Top < hb_gt_GetScreenHeight() && Left >= 0 && Left < hb_gt_GetScreenWidth() )
         hb_gt_xPutch( Top, Left, byAttr, szBox[ 0 ] ); /* Upper left corner */

      Col = ( Height > 1 ? Left + 1 : Left );
      if(Col < 0 )
      {
         Width += Col;
         Col = 0;
      }
      if( Right >= hb_gt_GetScreenWidth() )
      {
         Width -= Right - hb_gt_GetScreenWidth();
      }

      if( Col <= Right && Col < hb_gt_GetScreenWidth() && Top >= 0 && Top < hb_gt_GetScreenHeight() )
         hb_gt_Replicate( Top, Col, byAttr, szBox[ 1 ], Width + ( (Right - Left) > 1 ? -2 : 0 ) ); /* Top line */

      if( Height > 1 && (Right - Left) > 1 && Right < hb_gt_GetScreenWidth() && Top >= 0 && Top < hb_gt_GetScreenHeight() )
         hb_gt_xPutch( Top, Right, byAttr, szBox[ 2 ] ); /* Upper right corner */

      if( szBox[ 8 ] && Height > 2 && Width > 2 )
      {
         for( Row = Top + 1; Row < Bottom; Row++ )
         {
            if( Row >= 0 && Row < hb_gt_GetScreenHeight() )
            {
               Col = Left;
               if( Col < 0 )
                  Col = 0; /* The width was corrected earlier. */
               else
                  hb_gt_xPutch( Row, Col++, byAttr, szBox[ 7 ] ); /* Left side */
               hb_gt_Replicate( Row, Col, byAttr, szBox[ 8 ], Width - 2 ); /* Fill */
               if( Right < hb_gt_GetScreenWidth() )
                  hb_gt_xPutch( Row, Right, byAttr, szBox[ 3 ] ); /* Right side */
            }
         }
      }
      else
      {
         for( Row = ( Width > 1 ? Top + 1 : Top ); Row < ( (Right - Left ) > 1 ? Bottom : Bottom + 1 ); Row++ )
         {
            if( Row >= 0 && Row < hb_gt_GetScreenHeight() )
            {
               if( Left >= 0 && Left < hb_gt_GetScreenWidth() )
                  hb_gt_xPutch( Row, Left, byAttr, szBox[ 7 ] ); /* Left side */
               if( ( Width > 1 || Left < 0 ) && Right < hb_gt_GetScreenWidth() )
                  hb_gt_xPutch( Row, Right, byAttr, szBox[ 3 ] ); /* Right side */
            }
         }
      }

      if( Height > 1 && Width > 1 )
      {
         if( Left >= 0 && Bottom < hb_gt_GetScreenHeight() )
            hb_gt_xPutch( Bottom, Left, byAttr, szBox[ 6 ] ); /* Bottom left corner */

         Col = Left + 1;
         if( Col < 0 )
            Col = 0; /* The width was corrected earlier. */

         if( Col <= Right && Bottom < hb_gt_GetScreenHeight() )
            hb_gt_Replicate( Bottom, Col, byAttr, szBox[ 5 ], Width - 2 ); /* Bottom line */

         if( Right < hb_gt_GetScreenWidth() && Bottom < hb_gt_GetScreenHeight() )
            hb_gt_xPutch( Bottom, Right, byAttr, szBox[ 4 ] ); /* Bottom right corner */
      }
      hb_gt_DispEnd();
      ret = 0;
   }

   return ret;
}

USHORT HB_GT_FUNC(gt_BoxD( SHORT Top, SHORT Left, SHORT Bottom, SHORT Right, BYTE * pbyFrame, BYTE byAttr ))
{
   return hb_gt_Box( Top, Left, Bottom, Right, pbyFrame, byAttr );
}

USHORT HB_GT_FUNC(gt_BoxS( SHORT Top, SHORT Left, SHORT Bottom, SHORT Right, BYTE * pbyFrame, BYTE byAttr ))
{
   return hb_gt_Box( Top, Left, Bottom, Right, pbyFrame, byAttr );
}

USHORT HB_GT_FUNC(gt_HorizLine( SHORT Row, SHORT Left, SHORT Right, BYTE byChar, BYTE byAttr ))
{
   USHORT ret = 1;
   if( Row >= 0 && Row < hb_gt_GetScreenHeight() )
   {
      if( Left < 0 )
         Left = 0;
      else if( Left >= hb_gt_GetScreenWidth() )
         Left = hb_gt_GetScreenWidth() - 1;

      if( Right < 0 )
         Right = 0;
      else if( Right >= hb_gt_GetScreenWidth() )
         Right = hb_gt_GetScreenWidth() - 1;

      if( Left < Right )
         hb_gt_Replicate( Row, Left, byAttr, byChar, Right - Left + 1 );
      else
         hb_gt_Replicate( Row, Right, byAttr, byChar, Left - Right + 1 );
      ret = 0;
   }
   return ret;
}

USHORT HB_GT_FUNC(gt_VertLine( SHORT Col, SHORT Top, SHORT Bottom, BYTE byChar, BYTE byAttr ))
{
   USHORT ret = 1;
   SHORT Row;

   if( Col >= 0 && Col < hb_gt_GetScreenWidth() )
   {
      if( Top < 0 )
         Top = 0;
      else if( Top >= hb_gt_GetScreenHeight() )
         Top = hb_gt_GetScreenHeight() - 1;

      if( Bottom < 0 )
         Bottom = 0;
      else if( Bottom >= hb_gt_GetScreenHeight() )
         Bottom = hb_gt_GetScreenHeight() - 1;

      if( Top <= Bottom )
         Row = Top;
      else
      {
         Row = Bottom;
         Bottom = Top;
      }
      while( Row <= Bottom )
         hb_gt_xPutch( Row++, Col, byAttr, byChar );
      ret = 0;
   }
   return ret;
}

/* some definitions */
#define INT_VIDEO    0x10

#if defined(__DJGPP__)
   #define POKE_BYTE( s, o, b ) /* Do nothing */
   #define outport outportw     /* Use correct function name */
#elif defined(__RSX32__)
   #define inportb( p ) 0       /* Return 0 */
   #define outport( p, w )      /* Do nothing */
   #define outportb( p, b )     /* Do nothing */
   #define POKE_BYTE( s, o, b )  (*((BYTE FAR *)MK_FP((s),(o)) )=(BYTE)(b))
#elif defined(__WATCOMC__)
   #define outportb outp        /* Use correct function name */
   #define outport outpw        /* Use correct function name */
   #define inport inpw          /* Use correct function name */
   #define inportb inp          /* Use correct function name */
   #define POKE_BYTE( s, o, b )  (*((BYTE FAR *)MK_FP((s),(o)) )=(BYTE)(b))
#else
   #define POKE_BYTE( s, o, b )  (*((BYTE FAR *)MK_FP((s),(o)) )=(BYTE)(b))
#endif

static void vmode12x40( void )
{
   union REGS regs;
   regs.HB_XREGS.ax = 0x0001;               /* video mode 40 cols */
   HB_DOS_INT86( INT_VIDEO, &regs, &regs);
   outportb( 0x03D4, 0x09 );         /* update cursor size / pointers */
   regs.h.al = ( inportb( 0x03D5 ) | 0x80 );
   outportb( 0x03D5, regs.h.al );
   POKE_BYTE( 0x40, 0x84, 11);       /* 11 rows number update */
}

static void vmode25x40( void )
{
   union REGS regs;
   regs.HB_XREGS.ax = 0x0001;
   HB_DOS_INT86( INT_VIDEO, &regs, &regs);
}

static void vmode28x40( void )
{
   union REGS regs;
   regs.HB_XREGS.ax = 0x0001;               /* video mode 40 cols */
   HB_DOS_INT86( INT_VIDEO, &regs, &regs);
   regs.HB_XREGS.bx = 0;                    /* load block 0 (BL = 0) */
   regs.HB_XREGS.ax = 0x1111;               /* load 8x8 monochrome char set into RAM */
   HB_DOS_INT86( INT_VIDEO, &regs, &regs);
}

static void vmode50x40( void )
{
   union REGS regs;
   regs.HB_XREGS.ax = 0x0001;
   HB_DOS_INT86( INT_VIDEO, &regs, &regs);
   regs.HB_XREGS.bx = 0;                    /* load block 0 (BL = 0) */
   regs.HB_XREGS.ax = 0x1112;               /* load 8x8 double dot char set into RAM */
   HB_DOS_INT86( INT_VIDEO, &regs, &regs);
   outport( 0x03D4, 0x060A );
}

static void vmode12x80( void )
{
   union REGS regs;
   regs.HB_XREGS.ax = 0x0003;                  /* mode in AL, if bit 7 is on, No CLS */
   HB_DOS_INT86( INT_VIDEO, &regs, &regs);
   outportb( 0x03D4, 0x09 );            /* update cursor size / pointers */
   regs.h.al = ( inportb( 0x03D5 ) | 0x80 );
   outportb( 0x03D5, regs.h.al );
   POKE_BYTE( 0x40, 0x84, 11);          /* 11 rows number update */
}

static void vmode25x80( void )
{
   union REGS regs;
   regs.HB_XREGS.ax = 0x1202;              /* select 350 scan line mode */
   regs.h.bl = 0x30;
   HB_DOS_INT86( INT_VIDEO, &regs, &regs);
   regs.HB_XREGS.ax = 0x0083;              /* mode in AL, if higher bit is on, No CLS */
   HB_DOS_INT86( INT_VIDEO, &regs, &regs);
   regs.HB_XREGS.bx = 0;                   /* load block 0 (BL = 0) */
   regs.HB_XREGS.ax = 0x1114;              /* load 8x14 VGA char set into RAM */
   HB_DOS_INT86( INT_VIDEO, &regs, &regs);
}

static void vmode28x80( void )
{
   union REGS regs;
   regs.HB_XREGS.ax = 0x0003;              /* mode in AL, if higher bit is on, No CLS */
   HB_DOS_INT86( INT_VIDEO, &regs, &regs);
   regs.HB_XREGS.bx = 0;                   /* load block 0 (BL = 0) */
   regs.HB_XREGS.ax = 0x1111;              /* load 8x8 monochrome char set into RAM */
   HB_DOS_INT86( INT_VIDEO, &regs, &regs);
}

static void vmode43x80( void )
{
   union REGS regs;
   regs.HB_XREGS.ax = 0x1201;              /*  select 350 scan line mode */
   regs.h.bl = 0x30;
   HB_DOS_INT86( INT_VIDEO, &regs, &regs);
   regs.HB_XREGS.ax = 0x0003;              /* mode in AL, if higher bit is on, No CLS */
   HB_DOS_INT86( INT_VIDEO, &regs, &regs);
   regs.h.bh = 0x1;                 /* bytes per character */
   regs.h.bl = 0x0;                 /* load block 0 */
   regs.HB_XREGS.ax = 0x1112;              /* load 8x8 double dot char set into RAM */
   HB_DOS_INT86( INT_VIDEO, &regs, &regs);
   outport( 0x03D4, 0x060A );       /* update cursor size / pointers */
   POKE_BYTE( 0x40, 0x84, 42);      /* 42 rows number update */
}

static void vmode50x80( void )
{
   union REGS regs;
   regs.HB_XREGS.ax = 0x1202;               /*  select 400 scan line mode */
   regs.h.bl = 0x30;
   HB_DOS_INT86( INT_VIDEO, &regs, &regs);
   regs.HB_XREGS.ax = 0x0003;               /* mode in AL, if bit 7 is on, No CLS */
   HB_DOS_INT86( INT_VIDEO, &regs, &regs);
   regs.HB_XREGS.bx = 0;                    /* load block 0 (BL = 0) */
   regs.HB_XREGS.ax = 0x1112;               /* load 8x8 double dot char set into RAM */
   HB_DOS_INT86( INT_VIDEO, &regs, &regs);
}

/***************************************************************************
 * Return the display combination: monitor + video card
 *
 * INT 10 - VIDEO - GET DISPLAY COMBINATION CODE (PS,VGA/MCGA)
 *         AX = 1A00h
 * Return: AL = 1Ah if function was supported
 *         BL = active display code (see below)
 *         BH = alternate display code
 *
 * Values for display combination code:
 *  00h    no display
 *  01h    monochrome adapter w/ monochrome display
 *  02h    CGA w/ color display
 *  03h    reserved
 *  04h    EGA w/ color display
 *  05h    EGA w/ monochrome display
 *  06h    PGA w/ color display
 *  07h    VGA w/ monochrome analog display
 *  08h    VGA w/ color analog display
 *  09h    reserved
 *  0Ah    MCGA w/ digital color display
 *  0Bh    MCGA w/ monochrome analog display
 *  0Ch    MCGA w/ color analog display
 *  FFh    unknown display type
 ****************************************************************************/

static USHORT hb_gt_GetDisplay( void )
{
   union REGS regs;

   regs.HB_XREGS.ax = 0x1A00;
   HB_DOS_INT86( INT_VIDEO, &regs, &regs);

   return ( regs.h.al == 0x1A ) ? regs.h.bl : 0xFF;
}

BOOL HB_GT_FUNC(gt_SetMode( USHORT uiRows, USHORT uiCols ))
{
   /* hb_gt_IsColor() test for color card, we need to know if it is a VGA board...*/
   BOOL bIsVGA     = ( hb_gt_GetDisplay() == 8 );
   BOOL bIsVesa    = FALSE;
   USHORT bSuccess = FALSE;

   HB_TRACE( HB_TR_DEBUG, ("hb_gt_SetMode(%hu, %hu)", uiRows, uiCols) );

   /* Available modes in B&N and color screens */
   if( uiCols == 40 )
   {
      if( uiRows == 12 )
          vmode12x40();

      if( uiRows == 25 )
          vmode25x40();

      if( uiRows == 28 )
          vmode28x40();

      if( uiRows == 50 )
          vmode50x40();
   }

   if( bIsVGA )
   {
      if( uiCols == 80)
      {
         if( uiRows == 12 )
             vmode12x80();

         if( uiRows == 25 )
             vmode25x80();

         if( uiRows == 28 )
             vmode28x80();

         if( uiRows == 43 )
             vmode43x80();

         if( uiRows == 50 )
             vmode50x80();
      }

      if( uiCols > 80 && bIsVesa )
      {
         /* In development process
          * return( hb_gt_Modevesa( nMode) );
          */
      }
   }

   /* Check for succesful */

   if( ( hb_gtMaxRow() == uiRows - 1 ) &&
       ( hb_gtMaxCol() == uiCols - 1 ) )
   {
      bSuccess = TRUE;
   }
   else
   {
      vmode25x80();
      bSuccess = FALSE;
   }

   return bSuccess;
}

BOOL HB_GT_FUNC(gt_PreExt())
{
   return TRUE;
}

BOOL HB_GT_FUNC(gt_PostExt())
{
   return TRUE;
}

BOOL HB_GT_FUNC(gt_Suspend())
{
   return TRUE;
}

BOOL HB_GT_FUNC(gt_Resume())
{
   return TRUE;
}

void HB_GT_FUNC(gt_OutStd( BYTE * pbyStr, ULONG ulLen ))
{
    hb_fsWriteLarge( s_iStdOut, ( BYTE * ) pbyStr, ulLen );
}

void HB_GT_FUNC(gt_OutErr( BYTE * pbyStr, ULONG ulLen ))
{
    hb_fsWriteLarge( s_iStdOut, ( BYTE * ) pbyStr, ulLen );
}

/* ************************** Clipboard support ********************************** */

void HB_GT_FUNC( gt_GetClipboard( char *szData, ULONG *pulMaxSize ) )
{
   if ( *pulMaxSize == 0 || s_clipsize < *pulMaxSize )
   {
      *pulMaxSize = s_clipsize;
   }

   if ( *pulMaxSize != 0 )
   {
      memcpy( szData, s_clipboard, *pulMaxSize );
   }

}

void HB_GT_FUNC( gt_SetClipboard( char *szData, ULONG ulSize ) )
{
   if ( s_clipboard != NULL )
   {
      hb_xfree( s_clipboard );
   }

   s_clipboard = (char *) hb_xgrab( ulSize +1 );
   memcpy( s_clipboard, szData, ulSize );
   s_clipboard[ ulSize ] = '\0';
   s_clipsize = ulSize;
}

ULONG HB_GT_FUNC( gt_GetClipboardSize( void ) )
{
   return s_clipsize;
}


/**************************************************************/

int HB_GT_FUNC(gt_info(int iMsgType, BOOL bUpdate, int iParam, void *vpParam ))
{
   HB_SYMBOL_UNUSED( bUpdate );
   HB_SYMBOL_UNUSED( iParam );
   HB_SYMBOL_UNUSED( vpParam );

   switch ( iMsgType )
   {
      case GTI_ISGRAPHIC:
         return (int) FALSE;

      case GTI_INPUTFD:
         return s_iStdIn;

      case GTI_OUTPUTFD:
         return s_iStdOut;

      case GTI_VIEWMAXWIDTH:
         return _GetScreenWidth();

      case GTI_VIEWMAXHEIGHT:
         return _GetScreenHeight();

   }
   // DEFAULT: there's something wrong if we are here.
   return -1;
}

/* ********** Graphics API ********** */

int HB_GT_FUNC( gt_gfxPrimitive( int iType, int iTop, int iLeft, int iBottom, int iRight, int iColor ) )
{
  HB_SYMBOL_UNUSED( iType );
  HB_SYMBOL_UNUSED( iTop );
  HB_SYMBOL_UNUSED( iLeft );
  HB_SYMBOL_UNUSED( iBottom );
  HB_SYMBOL_UNUSED( iRight );
  HB_SYMBOL_UNUSED( iColor );

  return 0;
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

#ifdef HB_MULTI_GT

static void HB_GT_FUNC(gtFnInit( PHB_GT_FUNCS gt_funcs ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gtFnInit(%p)", gt_funcs));

    gt_funcs->Init                  = HB_GT_FUNC( gt_Init );
    gt_funcs->Exit                  = HB_GT_FUNC( gt_Exit );
    gt_funcs->GetScreenWidth        = HB_GT_FUNC( gt_GetScreenWidth );
    gt_funcs->GetScreenHeight       = HB_GT_FUNC( gt_GetScreenHeight );
    gt_funcs->Col                   = HB_GT_FUNC( gt_Col );
    gt_funcs->Row                   = HB_GT_FUNC( gt_Row );
    gt_funcs->SetPos                = HB_GT_FUNC( gt_SetPos );
    gt_funcs->AdjustPos             = HB_GT_FUNC( gt_AdjustPos );
    gt_funcs->IsColor               = HB_GT_FUNC( gt_IsColor );
    gt_funcs->GetCursorStyle        = HB_GT_FUNC( gt_GetCursorStyle );
    gt_funcs->SetCursorStyle        = HB_GT_FUNC( gt_SetCursorStyle );
    gt_funcs->DispBegin             = HB_GT_FUNC( gt_DispBegin );
    gt_funcs->DispEnd               = HB_GT_FUNC( gt_DispEnd );
    gt_funcs->DispCount             = HB_GT_FUNC( gt_DispCount );
    gt_funcs->Puts                  = HB_GT_FUNC( gt_Puts );
    gt_funcs->Replicate             = HB_GT_FUNC( gt_Replicate );
    gt_funcs->RectSize              = HB_GT_FUNC( gt_RectSize );
    gt_funcs->GetText               = HB_GT_FUNC( gt_GetText );
    gt_funcs->PutText               = HB_GT_FUNC( gt_PutText );
    gt_funcs->SetAttribute          = HB_GT_FUNC( gt_SetAttribute );
    gt_funcs->Scroll                = HB_GT_FUNC( gt_Scroll );
    gt_funcs->SetMode               = HB_GT_FUNC( gt_SetMode );
    gt_funcs->GetBlink              = HB_GT_FUNC( gt_GetBlink );
    gt_funcs->SetBlink              = HB_GT_FUNC( gt_SetBlink );
    gt_funcs->Version               = HB_GT_FUNC( gt_Version );
    gt_funcs->Box                   = HB_GT_FUNC( gt_Box );
    gt_funcs->BoxD                  = HB_GT_FUNC( gt_BoxD );
    gt_funcs->BoxS                  = HB_GT_FUNC( gt_BoxS );
    gt_funcs->HorizLine             = HB_GT_FUNC( gt_HorizLine );
    gt_funcs->VertLine              = HB_GT_FUNC( gt_VertLine );
    gt_funcs->Suspend               = HB_GT_FUNC( gt_Suspend );
    gt_funcs->Resume                = HB_GT_FUNC( gt_Resume );
    gt_funcs->PreExt                = HB_GT_FUNC( gt_PreExt );
    gt_funcs->PostExt               = HB_GT_FUNC( gt_PostExt );
    gt_funcs->OutStd                = HB_GT_FUNC( gt_OutStd );
    gt_funcs->OutErr                = HB_GT_FUNC( gt_OutErr );
    gt_funcs->Tone                  = HB_GT_FUNC( gt_Tone );
    gt_funcs->ExtendedKeySupport    = HB_GT_FUNC( gt_ExtendedKeySupport );
    gt_funcs->ReadKey               = HB_GT_FUNC( gt_ReadKey );
    gt_funcs->info                  = HB_GT_FUNC( gt_info );
    gt_funcs->SetClipboard          = HB_GT_FUNC( gt_SetClipboard );
    gt_funcs->GetClipboard          = HB_GT_FUNC( gt_GetClipboard );
    gt_funcs->GetClipboardSize      = HB_GT_FUNC( gt_GetClipboardSize );
    
    /* Graphics API */
    gt_funcs->gfxPrimitive          = HB_GT_FUNC( gt_gfxPrimitive );
}

/* ********************************************************************** */

static void HB_GT_FUNC(mouseFnInit( PHB_GT_FUNCS gt_funcs ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_mouseFnInit(%p)", gt_funcs));

    gt_funcs->mouse_Init            = HB_GT_FUNC( mouse_Init );
    gt_funcs->mouse_Exit            = HB_GT_FUNC( mouse_Exit );
    gt_funcs->mouse_IsPresent       = HB_GT_FUNC( mouse_IsPresent );
    gt_funcs->mouse_Show            = HB_GT_FUNC( mouse_Show );
    gt_funcs->mouse_Hide            = HB_GT_FUNC( mouse_Hide );
    gt_funcs->mouse_Col             = HB_GT_FUNC( mouse_Col );
    gt_funcs->mouse_Row             = HB_GT_FUNC( mouse_Row );
    gt_funcs->mouse_SetPos          = HB_GT_FUNC( mouse_SetPos );
    gt_funcs->mouse_IsButtonPressed = HB_GT_FUNC( mouse_IsButtonPressed );
    gt_funcs->mouse_CountButton     = HB_GT_FUNC( mouse_CountButton );
    gt_funcs->mouse_SetBounds       = HB_GT_FUNC( mouse_SetBounds );
    gt_funcs->mouse_GetBounds       = HB_GT_FUNC( mouse_GetBounds );
}


/* ********************************************************************** */

static HB_GT_INIT gtInit = { HB_GT_DRVNAME( HB_GT_NAME ),
                             HB_GT_FUNC(gtFnInit), HB_GT_FUNC(mouseFnInit) };

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

/* *********************************************************************** */
