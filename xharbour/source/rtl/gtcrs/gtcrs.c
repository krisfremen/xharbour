/*
 * $Id: gtcrs.c,v 1.15 2003/06/19 22:39:02 druzus Exp $
 */

/*
 * Harbour Project source code:
 * Video subsystem based on ncurses screen library.
 *
 * Copyright 2003 Przemyslaw Czerpak <druzus@polbox.com>
 * www - http://www.harbour-project.org
 * Special thanks to Marek Paliwoda <paliwoda@inetia.pl>
 * author of gtsln from which I borrowed a lot of code and ideas.
 * and to Gonzalo Diethelm <gonzalo.diethelm@iname.com>
 * author of previous version of gtcrs.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
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
 * not apply to the code that you add in this way.   To avoid misleading
 * anyone as to the status of such modified files, you must delete
 * this exception notice from them.
 *
 * If you write modifications of your own for Harbour, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice.
 *
 */

/* NOTE: User programs should never call this layer directly! */

/* *********************************************************************** */

#include "gtcrs.h"
#include "inkey.ch"
#include "setcurs.ch"

#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>
#ifdef HAVE_GPM_H
# include <gpm.h>
#endif
#if defined( HB_OS_LINUX ) || defined( HB_OS_BSD )
# include <pty.h>  /* for openpty and forkpty */
# include <utmp.h> /* for login_tty */
#endif

/* this variable should be global and checked in main VM loop */
static volatile BOOL s_BreakFlag = FALSE;
static volatile BOOL s_InetrruptFlag = FALSE;

static volatile BOOL s_WinSizeChangeFlag = FALSE;

typedef struct evtFD {
    int fd;
    int mode;
    int status;
    void *data;
    int (*eventFunc) (int, int, void*);
} evtFD;

typedef struct mouseEvent {
    int row, col;
    int lrow, lcol;
    int buttons;
    int lbuttons;
    int lbup_row, lbup_col;
    int lbdn_row, lbdn_col;
    int rbup_row, rbup_col;
    int rbdn_row, rbdn_col;
    int mbup_row, mbup_col;
    int mbdn_row, mbdn_col;
    /* to analize DBLCLK on xterm */
    int click_delay;
    struct timeval BL_time;
    struct timeval BR_time;
    struct timeval BM_time;
} mouseEvent;

typedef struct keyTab {
    int	ch;
    int key;
    struct keyTab *nextCh;
    struct keyTab *otherCh;
} keyTab;

typedef struct InOutBase {
    int terminal_type;

    struct keyTab *pKeyTab;
    int key_flag;
    int esc_delay;
    int key_counter;
    int nation_mode;
    unsigned char *in_transtbl;
    unsigned char *out_transtbl;
    unsigned char *nation_transtbl;
    int *charmap;

    int cursor, lcursor;
    int row, col;
    int maxrow, maxcol;
    int is_color;
    unsigned int disp_count;

    unsigned char *acsc, *bell, *flash, *civis, *cnorm, *cvvis;

    int is_mouse;
    int mButtons;
    int nTermMouseChars;
    char cTermMouseBuf[3];
    mouseEvent mLastEvt;
#ifdef HAVE_GPM_H
    Gpm_Connect Conn;
#endif

    int base_infd;
    int base_outfd;
    int stdoutfd;
    int stderrfd;
    pid_t termpid;
    int lTIOsaved;
    struct termios saved_TIO, curr_TIO;

    unsigned char stdin_buf[STDIN_BUFLEN];
    int stdin_ptr_l;
    int stdin_ptr_r;
    int stdin_inbuf;

    evtFD **event_fds;
    int efds_size;
    int efds_no;

    /* curses data */
    SCREEN *basescr;
    WINDOW *stdscr;
    FILE *basein;
    FILE *baseout;
    chtype std_chmap[256];
    chtype box_chmap[256];
    chtype attr_map[256];
    chtype attr_mask;
} InOutBase;

static InOutBase *s_ioBase = NULL;

static InOutBase **s_ioBaseTab = NULL;
static int s_iSize_ioBaseTab = 0;
static int s_iActive_ioBase = -1;

static void set_tmevt(char *cMBuf, mouseEvent *);
static int getMouseKey(mouseEvent *);
static void destroy_ioBase(InOutBase *ioBase);

typedef struct ClipKeyCode {
    int key;
    int alt_key;
    int ctrl_key;
    int shift_key;
} ClipKeyCode;

static const ClipKeyCode stdKeyTab[NO_STDKEYS] = {
    {K_SPACE,              0,             0,         0}, /*  32 */
    {'!',                  0,             0,         0}, /*  33 */
    {'"',                  0,             0,         0}, /*  34 */
    {'#',                  0,             0,         0}, /*  35 */
    {'$',                  0,             0,         0}, /*  36 */
    {'%',                  0,             0,         0}, /*  37 */
    {'&',                  0,             0,         0}, /*  38 */
    {'\'',               296,             7,         0}, /*  39 */
    {'(',                  0,             0,         0}, /*  40 */
    {')',                  0,             0,         0}, /*  41 */
    {'*',                  0,             0,         0}, /*  42 */
    {'+',                  0,             0,         0}, /*  43 */
    {',',                307,             0,         0}, /*  44 */
    {'-',                386,            31,         0}, /*  45 */
    {'.',                308,             0,         0}, /*  46 */
    {'/',                309,           127,         0}, /*  47 */
    {'0',            K_ALT_0,             0,         0}, /*  48 */
    {'1',            K_ALT_1,             0,         0}, /*  49 */
    {'2',            K_ALT_2,           259,         0}, /*  50 */
    {'3',            K_ALT_3,            27,         0}, /*  51 */
    {'4',            K_ALT_4,            28,         0}, /*  52 */
    {'5',            K_ALT_5,            29,         0}, /*  53 */
    {'6',            K_ALT_6,            30,         0}, /*  54 */
    {'7',            K_ALT_7,            31,         0}, /*  55 */
    {'8',            K_ALT_8,           127,         0}, /*  56 */
    {'9',            K_ALT_9,             0,         0}, /*  57 */
    {':',                  0,             0,         0}, /*  58 */
    {';',                295,             0,         0}, /*  59 */
    {'<',                  0,             0,         0}, /*  60 */
    {'=',       K_ALT_EQUALS,             0,         0}, /*  61 */
    {'>',                  0,             0,         0}, /*  62 */
    {'?',                  0,             0,         0}, /*  63 */
    {'@',                  0,             0,         0}, /*  64 */
    {'A',            K_ALT_A,      K_CTRL_A,         0}, /*  65 */
    {'B',            K_ALT_B,      K_CTRL_B,         0}, /*  66 */
    {'C',            K_ALT_C,      K_CTRL_C,         0}, /*  67 */
    {'D',            K_ALT_D,      K_CTRL_D,         0}, /*  68 */
    {'E',            K_ALT_E,      K_CTRL_E,         0}, /*  69 */
    {'F',            K_ALT_F,      K_CTRL_F,         0}, /*  70 */
    {'G',            K_ALT_G,      K_CTRL_G,         0}, /*  71 */
    {'H',            K_ALT_H,      K_CTRL_H,         0}, /*  72 */
    {'I',            K_ALT_I,      K_CTRL_I,         0}, /*  73 */
    {'J',            K_ALT_J,      K_CTRL_J,         0}, /*  74 */
    {'K',            K_ALT_K,      K_CTRL_K,         0}, /*  75 */
    {'L',            K_ALT_L,      K_CTRL_L,         0}, /*  76 */
    {'M',            K_ALT_M,      K_CTRL_M,         0}, /*  77 */
    {'N',            K_ALT_N,      K_CTRL_N,         0}, /*  78 */
    {'O',            K_ALT_O,      K_CTRL_O,         0}, /*  79 */
    {'P',            K_ALT_P,      K_CTRL_P,         0}, /*  80 */
    {'Q',            K_ALT_Q,      K_CTRL_Q,         0}, /*  81 */
    {'R',            K_ALT_R,      K_CTRL_R,         0}, /*  82 */
    {'S',            K_ALT_S,      K_CTRL_S,         0}, /*  83 */
    {'T',            K_ALT_T,      K_CTRL_T,         0}, /*  84 */
    {'U',            K_ALT_U,      K_CTRL_U,         0}, /*  85 */
    {'V',            K_ALT_V,      K_CTRL_V,         0}, /*  86 */
    {'W',            K_ALT_W,      K_CTRL_W,         0}, /*  87 */
    {'X',            K_ALT_X,      K_CTRL_X,         0}, /*  88 */
    {'Y',            K_ALT_Y,      K_CTRL_Y,         0}, /*  89 */
    {'Z',            K_ALT_Z,      K_CTRL_Z,         0}, /*  90 */
    {'[',                282,            27,         0}, /*  91 */
    {'\\',               299,            28,         0}, /*  92 */
    {']',                283,            29,         0}, /*  93 */
    {'^',            K_ALT_6,            30,         0}, /*  94 */
    {'_',                386,            31,         0}, /*  95 */
    {'`',                297,           297,         0}, /*  96 */
    {'a',            K_ALT_A,      K_CTRL_A,         0}, /*  97 */
    {'b',            K_ALT_B,      K_CTRL_B,         0}, /*  98 */
    {'c',            K_ALT_C,      K_CTRL_C,         0}, /*  99 */
    {'d',            K_ALT_D,      K_CTRL_D,         0}, /* 100 */
    {'e',            K_ALT_E,      K_CTRL_E,         0}, /* 101 */
    {'f',            K_ALT_F,      K_CTRL_F,         0}, /* 102 */
    {'g',            K_ALT_G,      K_CTRL_G,         0}, /* 103 */
    {'h',            K_ALT_H,      K_CTRL_H,         0}, /* 104 */
    {'i',            K_ALT_I,      K_CTRL_I,         0}, /* 105 */
    {'j',            K_ALT_J,      K_CTRL_J,         0}, /* 106 */
    {'k',            K_ALT_K,      K_CTRL_K,         0}, /* 107 */
    {'l',            K_ALT_L,      K_CTRL_L,         0}, /* 108 */
    {'m',            K_ALT_M,      K_CTRL_M,         0}, /* 109 */
    {'n',            K_ALT_N,      K_CTRL_N,         0}, /* 110 */
    {'o',            K_ALT_O,      K_CTRL_O,         0}, /* 111 */
    {'p',            K_ALT_P,      K_CTRL_P,         0}, /* 112 */
    {'q',            K_ALT_Q,      K_CTRL_Q,         0}, /* 113 */
    {'r',            K_ALT_R,      K_CTRL_R,         0}, /* 114 */
    {'s',            K_ALT_S,      K_CTRL_S,         0}, /* 115 */
    {'t',            K_ALT_T,      K_CTRL_T,         0}, /* 116 */
    {'u',            K_ALT_U,      K_CTRL_U,         0}, /* 117 */
    {'v',            K_ALT_V,      K_CTRL_V,         0}, /* 118 */
    {'w',            K_ALT_W,      K_CTRL_W,         0}, /* 119 */
    {'x',            K_ALT_X,      K_CTRL_X,         0}, /* 120 */
    {'y',            K_ALT_Y,      K_CTRL_Y,         0}, /* 121 */
    {'z',            K_ALT_Z,      K_CTRL_Z,         0}, /* 122 */
    {'{',                282,            27,         0}, /* 123 */
    {'|',                299,            28,         0}, /* 124 */
    {'}',                283,            29,         0}, /* 125 */
    {'~',                297,           297,         0}, /* 126 */
    {K_CTRL_BS,     K_ALT_BS,           127,         0}  /* 127 */
};           

static const ClipKeyCode extdKeyTab[NO_EXTDKEYS] = {
    {K_F1,          K_ALT_F1,     K_CTRL_F1,   K_SH_F1}, /*  00 */
    {K_F2,          K_ALT_F2,     K_CTRL_F2,   K_SH_F2}, /*  01 */
    {K_F3,          K_ALT_F3,     K_CTRL_F3,   K_SH_F3}, /*  02 */
    {K_F4,          K_ALT_F4,     K_CTRL_F4,   K_SH_F4}, /*  03 */
    {K_F5,          K_ALT_F5,     K_CTRL_F5,   K_SH_F5}, /*  04 */
    {K_F6,          K_ALT_F6,     K_CTRL_F6,   K_SH_F6}, /*  05 */
    {K_F7,          K_ALT_F7,     K_CTRL_F7,   K_SH_F7}, /*  06 */
    {K_F8,          K_ALT_F8,     K_CTRL_F8,   K_SH_F8}, /*  07 */
    {K_F9,          K_ALT_F9,     K_CTRL_F9,   K_SH_F9}, /*  08 */
    {K_F10,        K_ALT_F10,    K_CTRL_F10,  K_SH_F10}, /*  09 */
    {K_F11,        K_ALT_F11,    K_CTRL_F11,  K_SH_F11}, /*  10 */
    {K_F12,        K_ALT_F12,    K_CTRL_F12,  K_SH_F12}, /*  11 */
    {K_UP,          K_ALT_UP,     K_CTRL_UP,         0}, /*  12 */
    {K_DOWN,      K_ALT_DOWN,   K_CTRL_DOWN,         0}, /*  13 */
    {K_LEFT,      K_ALT_LEFT,   K_CTRL_LEFT,         0}, /*  14 */
    {K_RIGHT,    K_ALT_RIGHT,  K_CTRL_RIGHT,         0}, /*  15 */
    {K_INS,        K_ALT_INS,    K_CTRL_INS,         0}, /*  16 */
    {K_DEL,        K_ALT_DEL,    K_CTRL_DEL,         0}, /*  17 */
    {K_HOME,      K_ALT_HOME,   K_CTRL_HOME,         0}, /*  18 */
    {K_END,        K_ALT_END,    K_CTRL_END,         0}, /*  19 */
    {K_PGUP,      K_ALT_PGUP,   K_CTRL_PGUP,         0}, /*  20 */
    {K_PGDN,      K_ALT_PGDN,   K_CTRL_PGDN,         0}, /*  21 */
    {K_BS,          K_ALT_BS,           127,         0}, /*  22 */
    {K_TAB,        K_ALT_TAB,    K_CTRL_TAB,  K_SH_TAB}, /*  23 */
    {K_ESC,        K_ALT_ESC,         K_ESC,         0}, /*  24 */
    {K_ENTER,    K_ALT_ENTER,  K_CTRL_ENTER,         0}, /*  25 */
    {K_ENTER,   KP_ALT_ENTER,  K_CTRL_ENTER,         0}, /*  26 */
    {KP_CENTER,            0,     KP_CTRL_5,         0}, /*  27 */
    {K_PRTSCR,             0, K_CTRL_PRTSCR,         0}, /*  28 */
    {K_PAUSE,              0,             0,         0}  /*  29 */
};

static int getClipKey(int nKey)
{
    int nRet = 0, nFlag = 0, n;

    if (IS_CLIPKEY(nKey))
	nRet = GET_CLIPKEY(nKey);
    else {
	nFlag = GET_KEYMASK(nKey);
	nKey = CLR_KEYMASK(nKey);
	if (nFlag & KEY_EXTDMASK) {
	    if (nKey >= 0 && nKey < NO_EXTDKEYS) {
		if ((nFlag & KEY_ALTMASK) && (nFlag & KEY_CTRLMASK) &&
		     extdKeyTab[nKey].shift_key != 0)
		    nRet = extdKeyTab[nKey].shift_key;
		else if ((nFlag & KEY_ALTMASK) && extdKeyTab[nKey].alt_key != 0)
		    nRet = extdKeyTab[nKey].alt_key;
		else if ((nFlag & KEY_CTRLMASK) && extdKeyTab[nKey].ctrl_key != 0)
		    nRet = extdKeyTab[nKey].ctrl_key;
		else
		    nRet = extdKeyTab[nKey].key;
	    }
	} else {
	    if (nKey > 0 && nKey < 32) {
		nFlag |= KEY_CTRLMASK;
		nKey += 64;
	    }
	    n = nKey - 32;
	    if (n >= 0 && n < NO_STDKEYS) {
		if ((nFlag & KEY_ALTMASK) && (nFlag & KEY_CTRLMASK) &&
		     stdKeyTab[n].shift_key != 0)
		    nRet = stdKeyTab[n].shift_key;
		else if ((nFlag & KEY_ALTMASK) && stdKeyTab[n].alt_key != 0)
		    nRet = stdKeyTab[n].alt_key;
		else if ((nFlag & KEY_CTRLMASK) && stdKeyTab[n].ctrl_key != 0)
		    nRet = stdKeyTab[n].ctrl_key;
		else
		    nRet = stdKeyTab[n].key;
	    } else
		nRet = nKey;
	    
	}
    }

    return nRet;
}

#if 1
static void sig_handler(int signo)
{
    int e = errno, stat;
    pid_t pid;

    switch( signo ) {
      case SIGCHLD:
        while ( ( pid = waitpid( -1, &stat, WNOHANG ) ) > 0 );
        break;
      case SIGWINCH:
        s_WinSizeChangeFlag = TRUE;
        break;
      case SIGINT:
        s_InetrruptFlag = TRUE;
        break;
      case SIGQUIT:
        s_BreakFlag = TRUE;
        break;
      case SIGTSTP:
        break;
      default:
        break;
    }

    errno = e;
    return;
}

static void set_signals()
{
    struct sigaction act;
    int i, sigs[] = { SIGINT, SIGQUIT, SIGTSTP, SIGWINCH, SIGCHLD, 0 };

    /* Ignore SIGPIPEs so they don't kill us. */
    signal( SIGPIPE, SIG_IGN );

    for ( i = 0; sigs[i]; ++i ) {
	sigaction( sigs[i], 0, &act );
	act.sa_handler = sig_handler;
	act.sa_flags = SA_RESTART | (sigs[i] == SIGCHLD ? SA_NOCLDSTOP : 0);
	sigaction( sigs[i], &act, 0 );
    }
}

#else
static void sig_handler(int signo)
{
    int e = errno, stat;
    char *pszSig;
    pid_t pid;

    switch( signo ) {
      case SIGCHLD:
        pszSig = "SIGCHLD";
        while ( ( pid = waitpid( -1, &stat, WNOHANG ) ) > 0 );
        break;
      case SIGWINCH:
        pszSig = "SIGWINCH";
        break;
      case SIGPIPE:
        pszSig = "SIGPIPE";
        break;
      case SIGTERM:
        pszSig = "SIGTERM";
        break;
      case SIGINT:
        pszSig = "SIGINT";
        break;
      case SIGQUIT:
        pszSig = "SIGQUIT";
        break;
      case SIGCONT:
        pszSig = "SIGCONT";
        break;
      case SIGTSTP:
        pszSig = "SIGTSTP";
        break;
      case SIGTTOU:
        pszSig = "SIGTTOU";
        break;
      default:
        pszSig = "other signal";
        break;
    }

    printf("\nreceived signal: %d -> %s\n", signo, pszSig);
    fflush(stdout);

    errno = e;
    return;
}

static void set_signals()
{
    struct sigaction act;
    int i;

    for (i=1; i<32; ++i) {
	sigaction( i, 0, &act );
	act.sa_handler = sig_handler;
	act.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	sigaction( i, &act, 0);
    }
}
#endif

static int add_efds(InOutBase *ioBase, int fd, int mode, int (*eventFunc) (int, int, void *), void *data)
{
    struct evtFD *pefd = NULL;
    int i, fl;

    if (eventFunc == NULL && mode != O_RDONLY)
	return -1;

    if (fcntl(fd, F_GETFL, &fl) == -1)
	return -1;
    fl &= O_RDONLY | O_WRONLY | O_RDWR;

    if ((fl == O_RDONLY && mode == O_WRONLY) ||
        (fl == O_WRONLY && mode == O_RDONLY))
	return -1;

    for (i = 0; i < ioBase->efds_no && !pefd; i++)
	if (ioBase->event_fds[i]->fd == fd)
	    pefd = ioBase->event_fds[i];

    if (pefd) {
	pefd->mode = mode;
	pefd->data = data;
	pefd->eventFunc = eventFunc;
	pefd->status = EVTFDSTAT_RUN;
    } else {
	if (ioBase->efds_size <= ioBase->efds_no) {
	    if (ioBase->event_fds == NULL)
		ioBase->event_fds = hb_xgrab((ioBase->efds_size += 10) * sizeof(evtFD *));
	    else
		ioBase->event_fds = hb_xrealloc(ioBase->event_fds, (ioBase->efds_size += 10) * sizeof(evtFD *));
	}

	pefd = hb_xgrab(sizeof(evtFD));
	pefd->fd = fd;
	pefd->mode = mode;
	pefd->data = data;
	pefd->eventFunc = eventFunc;
	pefd->status = EVTFDSTAT_RUN;
	ioBase->event_fds[ioBase->efds_no++] = pefd;
    }

    return fd;
}

static void del_efds(InOutBase *ioBase, int fd)
{
    int i, n = -1;

    for (i=0; i<ioBase->efds_no && n == -1; i++)
	if (ioBase->event_fds[i]->fd == fd)
	    n = i;

    if (n != -1) {
	hb_xfree(ioBase->event_fds[n]);
	ioBase->efds_no--;
	for (i = n; i < ioBase->efds_no; i++)
	    ioBase->event_fds[i] = ioBase->event_fds[i+1];
    }
}

static void del_all_efds(InOutBase *ioBase)
{
    int i;

    if (ioBase->event_fds != NULL) {
	for (i=0; i<ioBase->efds_no; i++)
	    hb_xfree(ioBase->event_fds[i]);

	hb_xfree(ioBase->event_fds);

	ioBase->event_fds = NULL;
	ioBase->efds_no = ioBase->efds_size = 0;
    }
}

static int get_inch(InOutBase *ioBase, int  milisec )
{
    int nRet = 0, npfd = -1, nchk = ioBase->efds_no, lRead = 0;
    int mode, i, n, counter;
    struct timeval tv, *ptv;
    struct evtFD *pefd = NULL;
    fd_set rfds, wfds;

    if( milisec == 0 )
      ptv = NULL;
    else {
      if( milisec < 0 )
        milisec = 0;
      tv.tv_sec = (milisec / 1000);
      tv.tv_usec = (milisec % 1000) * 1000;
      ptv = &tv;
    }

    while( nRet == 0 && lRead == 0 ) {
	n = -1;
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	for (i = 0; i < ioBase->efds_no; i++) {
	    if (ioBase->event_fds[i]->status == EVTFDSTAT_RUN) {
		if (ioBase->event_fds[i]->mode == O_RDWR || ioBase->event_fds[i]->mode == O_RDONLY) {
		    FD_SET(ioBase->event_fds[i]->fd, &rfds);
		    if (n < ioBase->event_fds[i]->fd)
			n = ioBase->event_fds[i]->fd;
		}
		if (ioBase->event_fds[i]->mode == O_RDWR || ioBase->event_fds[i]->mode == O_WRONLY) {
		    FD_SET(ioBase->event_fds[i]->fd, &wfds);
		    if (n < ioBase->event_fds[i]->fd)
			n = ioBase->event_fds[i]->fd;
		}
	    }
	}

	counter = ioBase->key_counter;
	if ( select(n+1, &rfds, &wfds, NULL, ptv) > 0 ) {
	    for (i = 0; i < ioBase->efds_no; i++) {
		n = (FD_ISSET(ioBase->event_fds[i]->fd, &rfds) ? 1 : 0) |
		    (FD_ISSET(ioBase->event_fds[i]->fd, &wfds) ? 2 : 0);
		if (n != 0) {
		    if (ioBase->event_fds[i]->eventFunc == NULL) {
			lRead = 1;
			if (STDIN_BUFLEN > ioBase->stdin_inbuf) {
            		    unsigned char buf[STDIN_BUFLEN];

            		    n = read(ioBase->event_fds[i]->fd, buf, STDIN_BUFLEN-ioBase->stdin_inbuf);
			    if (n == 0)
				ioBase->event_fds[i]->status = EVTFDSTAT_STOP;
			    else
            			for (i=0; i<n; i++) {
            			    ioBase->stdin_buf[ioBase->stdin_ptr_r++] = buf[i];
            			    if( ioBase->stdin_ptr_r == STDIN_BUFLEN )
                			ioBase->stdin_ptr_r = 0;
            			    ioBase->stdin_inbuf++;
            			}
			}
		    } else if (nRet == 0 && counter == ioBase->key_counter) {
			if (n==3)
			    mode = O_RDWR;
			else if (n==2)
			    mode = O_WRONLY;
			else
			    mode = O_RDONLY;
			ioBase->event_fds[i]->status = EVTFDSTAT_STOP;
			n = (ioBase->event_fds[i]->eventFunc)(ioBase->event_fds[i]->fd, mode, ioBase->event_fds[i]->data);
			if (IS_EVTFDSTAT(n)) {
			    ioBase->event_fds[i]->status = n;
			    if (nchk > i)
				nchk = i;
			}
			else {
			    ioBase->event_fds[i]->status = EVTFDSTAT_RUN;
			    if (IS_CLIPKEY(n)) {
				nRet = n;
				npfd = ioBase->event_fds[i]->fd;
				if (nchk > i)
				    nchk = i;
			    }
			}
		    }
		}
    	    }
        } else
	    lRead = 1;
    }

    for (i = n = nchk; i < ioBase->efds_no; i++) {
	if (ioBase->event_fds[i]->status == EVTFDSTAT_DEL)
	    hb_xfree(ioBase->event_fds[i]);
	else if( ioBase->event_fds[i]->fd == npfd)
	    pefd = ioBase->event_fds[i];
	else {
	    if (i > n)
		ioBase->event_fds[n] = ioBase->event_fds[i];
	    n++;
	}
    }
    if (pefd)
	ioBase->event_fds[n++] = pefd;
    ioBase->efds_no = n;

    return nRet;
}

static int test_bufch(InOutBase *ioBase, int n, int delay)
{
    int nKey = 0;

    if( ioBase->stdin_inbuf == n )
        nKey = get_inch(ioBase, delay);

    return IS_CLIPKEY(nKey) ? nKey :
	    (ioBase->stdin_inbuf > n) ? 
	      ioBase->stdin_buf[(ioBase->stdin_ptr_l + n) % STDIN_BUFLEN] : -1;
}

static void free_bufch(InOutBase *ioBase, int n)
{
    if (n > ioBase->stdin_inbuf)
	n = ioBase->stdin_inbuf;
    ioBase->stdin_ptr_l = (ioBase->stdin_ptr_l + n) % STDIN_BUFLEN;
    ioBase->stdin_inbuf -= n;
}

static int wait_key(InOutBase *ioBase, int milisec )
{
    int nKey, esc, n, i, ch, counter;
    struct keyTab *ptr;

    counter = ++(ioBase->key_counter);
restart:
    nKey = esc = n = i = 0;
again:
    if ((nKey = getMouseKey(&ioBase->mLastEvt)) != 0)
	return nKey;

    ch = test_bufch(ioBase, i, ioBase->nTermMouseChars ? ioBase->esc_delay : milisec);
    if (counter != ioBase->key_counter)
	goto restart;

    if( ch >=0 && ch <=255 ) {
	++i;
	if (ioBase->nTermMouseChars) {
	    ioBase->cTermMouseBuf[3-ioBase->nTermMouseChars] = ch;
	    free_bufch(ioBase, i);
	    i = 0;
	    if (--(ioBase->nTermMouseChars) == 0)
		set_tmevt(ioBase->cTermMouseBuf, &ioBase->mLastEvt);
	    goto again;
	}

        nKey = ch;
	ptr = ioBase->pKeyTab;
	if (i == 1 && nKey == K_ESC && esc == 0)
	    esc = 1;
        while( ch >= 0 && ch <= 255 && ptr != NULL ) {
            if( ptr->ch == ch ) {
        	if( ptr->key != K_UNDEF ) {
            	    nKey = ptr->key;
		    switch (nKey) {
			case K_METAALT:
			    ioBase->key_flag |= KEY_ALTMASK;
			    break;
			case K_METACTRL:
			    ioBase->key_flag |= KEY_CTRLMASK;
			    break;
			case K_NATIONAL:
			    ioBase->nation_mode = ! ioBase->nation_mode;
			    break;
			case K_MOUSETERM:
			    ioBase->nTermMouseChars = 3;
			    break;
			default:
        		    n = i;
		    }
		    if ( n != i ) {
			free_bufch(ioBase, i);
			i = n = nKey = 0;
			if (esc == 2)
			    break;
			esc = 0;
			goto again;
		    }
        	}
        	ptr = ptr->nextCh;
		if (ptr)
        	    if ((ch = test_bufch(ioBase, i, ioBase->esc_delay)) != -1)
			++i;
		    if (counter != ioBase->key_counter)
			goto restart;
	    } else
        	ptr = ptr->otherCh;
        }
    }
    if (ch == -1 && ioBase->nTermMouseChars)
	ioBase->nTermMouseChars = 0;

    if (ch != -1 && IS_CLIPKEY(ch))
	nKey = GET_CLIPKEY(ch);
    else {
	if (esc == 1 && n == 0 && (ch != -1 || i >= 2)) {
	    nKey = 0;
	    esc = 2;
	    i = n = 1;
	    goto again;
	}
	if (esc == 2) {
	    if (nKey != 0)
		ioBase->key_flag |= KEY_ALTMASK;
	    else
		nKey = K_ESC;
	    if (n == 1 && i>1)
		n = 2;
	} else if (n == 0 && i>0)
	    n = 1;

	if (n > 0)
	    free_bufch(ioBase, n);

	if( ioBase->key_flag != 0 && nKey !=0 ) {
	    nKey |= ioBase->key_flag;
	    ioBase->key_flag = 0;
	}

	if (ioBase->nation_transtbl && ioBase->nation_mode && 
		nKey >= 32 && nKey < 128 && ioBase->nation_transtbl[nKey])
	    nKey = ioBase->nation_transtbl[nKey];
	if (ioBase->in_transtbl && nKey >=0 && nKey <=255 && ioBase->in_transtbl[nKey])
	    nKey = ioBase->in_transtbl[nKey];

	if (nKey)
	    nKey = getClipKey(nKey);
    }

    return nKey;
}

static void write_ttyseq(InOutBase *ioBase, char *seq)
{
    if ( ioBase->baseout != NULL ) {
	fwrite(seq, strlen(seq), 1, ioBase->baseout);
	fflush(ioBase->baseout);
    } else
	write(ioBase->base_outfd, seq, strlen(seq));
}

static int addKeyMap(InOutBase *ioBase, int nKey, const unsigned char *cdesc)
{
    int ret = K_UNDEF, i=0, c;
    struct keyTab **ptr;

    if (cdesc == NULL)
	return ret;

    c = (unsigned char) cdesc[i++];
    ptr = &ioBase->pKeyTab;

    while( c ) {
      if( *ptr == NULL ) {
        *ptr = (struct keyTab *) hb_xgrab( sizeof(struct keyTab) );
        (*ptr)->ch = c;
        (*ptr)->key = K_UNDEF;
        (*ptr)->nextCh = NULL;
        (*ptr)->otherCh = NULL;
      }
      if( (*ptr)->ch == c ) {
        c = (unsigned char) cdesc[i++];
        if( c )
          ptr = &((*ptr)->nextCh);
        else {
          ret = (*ptr)->key;
          (*ptr)->key = nKey;
        }
      } else
        ptr = &((*ptr)->otherCh);
    }
    return ret;
}

static int removeKeyMap(InOutBase *ioBase, unsigned char *cdesc)
{
    int ret = K_UNDEF, i=0, c;
    struct keyTab **ptr;

    c = cdesc[i++];
    ptr = &ioBase->pKeyTab;

    while( c && *ptr != NULL ) {
      if( (*ptr)->ch == c ) {
        c = cdesc[i++];
        if( !c ) {
          ret = (*ptr)->key;
          (*ptr)->key = K_UNDEF;
	  if ((*ptr)->nextCh == NULL && (*ptr)->otherCh == NULL) {
	    hb_xfree(*ptr);
	    *ptr = NULL;
	  }
        } else
          ptr = &((*ptr)->nextCh);
      } else
        ptr = &((*ptr)->otherCh);
    }
    return ret;
}

static void removeAllKeyMap(struct keyTab **ptr)
{
    if ((*ptr)->nextCh != NULL)
	removeAllKeyMap(&((*ptr)->nextCh));
    if ((*ptr)->otherCh != NULL)
	removeAllKeyMap(&((*ptr)->otherCh));

    hb_xfree(*ptr);
    *ptr = NULL;
}

static int getMouseKey(mouseEvent *mEvt)
{
    int nKey = 0;

    if (mEvt->lrow != mEvt->row || mEvt->lcol != mEvt->col) {
	nKey = K_MOUSEMOVE;
	mEvt->lrow = mEvt->row;
	mEvt->lcol = mEvt->col;
    } else if (mEvt->lbuttons != mEvt->buttons) {
	int butt = mEvt->lbuttons ^ mEvt->buttons;

	if (butt & M_BUTTON_LEFT) {
	    if (mEvt->buttons & M_BUTTON_LEFT) {
		mEvt->lbdn_row = mEvt->row;
		mEvt->lbdn_col = mEvt->col;
	    } else {
		mEvt->lbup_row = mEvt->row;
		mEvt->lbup_col = mEvt->col;
	    }
	    nKey = (mEvt->buttons & M_BUTTON_LEFT) ? 
			((mEvt->buttons & M_BUTTON_LDBLCK) ? K_LDBLCLK : 
			    K_LBUTTONDOWN) : K_LBUTTONUP;
	    mEvt->lbuttons ^= M_BUTTON_LEFT;
	    mEvt->buttons &= ~M_BUTTON_LDBLCK;
	} else if (butt & M_BUTTON_RIGHT) {
	    if (mEvt->buttons & M_BUTTON_RIGHT) {
		mEvt->rbdn_row = mEvt->row;
		mEvt->rbdn_col = mEvt->col;
	    } else {
		mEvt->rbup_row = mEvt->row;
		mEvt->rbup_col = mEvt->col;
	    }
	    nKey = (mEvt->buttons & M_BUTTON_RIGHT) ? 
			((mEvt->buttons & M_BUTTON_RDBLCK) ? K_RDBLCLK : 
			    K_RBUTTONDOWN) : K_RBUTTONUP;
	    mEvt->lbuttons ^= M_BUTTON_RIGHT;
	    mEvt->buttons &= ~M_BUTTON_RDBLCK;
	} else if (butt & M_BUTTON_MIDDLE) {
	    if (mEvt->buttons & M_BUTTON_MIDDLE) {
		mEvt->mbdn_row = mEvt->row;
		mEvt->mbdn_col = mEvt->col;
	    } else {
		mEvt->mbup_row = mEvt->row;
		mEvt->mbup_col = mEvt->col;
	    }
	    mEvt->lbuttons ^= M_BUTTON_MIDDLE;
	    mEvt->buttons &= ~M_BUTTON_MIDDLE;
	} else
	    mEvt->lbuttons = mEvt->buttons;
    }

    return nKey;
}

static void chk_mevtdblck(mouseEvent *mEvt)
{
    if (mEvt->lbuttons != mEvt->buttons) {
	struct timeval tv;

	TIMEVAL_GET(tv);

	if (mEvt->buttons & M_BUTTON_LEFT &&
	    ~(mEvt->lbuttons & M_BUTTON_LEFT)) {
	    if (TIMEVAL_LESS(tv, mEvt->BL_time))
		mEvt->buttons |= M_BUTTON_LDBLCK;
	    TIMEVAL_ADD(mEvt->BL_time, tv, mEvt->click_delay);
	}
	if (mEvt->buttons & M_BUTTON_MIDDLE &&
	    ~(mEvt->lbuttons & M_BUTTON_MIDDLE)) {
	    if (TIMEVAL_LESS(tv, mEvt->BM_time))
		mEvt->buttons |= M_BUTTON_MDBLCK;
	    TIMEVAL_ADD(mEvt->BM_time, tv, mEvt->click_delay);
	}
	if (mEvt->buttons & M_BUTTON_RIGHT &&
	    ~(mEvt->lbuttons & M_BUTTON_RIGHT)) {
	    if (TIMEVAL_LESS(tv, mEvt->BR_time))
		mEvt->buttons |= M_BUTTON_RDBLCK;
	    TIMEVAL_ADD(mEvt->BR_time, tv, mEvt->click_delay);
	}
    }
}

static void set_tmevt(char *cMBuf, mouseEvent *mEvt)
{
    switch (cMBuf[0] & 0x3) {
	case 0x0:	mEvt->buttons |= M_BUTTON_LEFT; break;
	case 0x1:	mEvt->buttons |= M_BUTTON_MIDDLE; break;
	case 0x2:	mEvt->buttons |= M_BUTTON_RIGHT; break;
	case 0x3:	mEvt->buttons = 0; break;
    }
    mEvt->col = cMBuf[1] - 33;
    mEvt->row = cMBuf[2] - 33;
    chk_mevtdblck(mEvt);
//    printf("\n\rmouse event: %02x, %02x, %02x\n\r", cMBuf[0], cMBuf[1], cMBuf[2]);
    return;
}

#ifdef HAVE_GPM_H
static int set_gpmevt(int fd, int mode, void *data )
{
    int nKey = 0;
    mouseEvent *mEvt;
    Gpm_Event gEvt;

    HB_SYMBOL_UNUSED( fd );
    HB_SYMBOL_UNUSED( mode );

    mEvt = (mouseEvent *) data;

    if ( Gpm_GetEvent( &gEvt ) > 0 ) {
        mEvt->row = gEvt.y;
        mEvt->col = gEvt.x;
	if( gEvt.type & GPM_DOWN ) {
	    if (gEvt.buttons & GPM_B_LEFT)
		mEvt->buttons |= M_BUTTON_LEFT;
	    if (gEvt.buttons & GPM_B_MIDDLE)
		mEvt->buttons |= M_BUTTON_MIDDLE;
	    if (gEvt.buttons & GPM_B_RIGHT)
		mEvt->buttons |= M_BUTTON_RIGHT;
	} else if( gEvt.type & GPM_UP ) {
	    if (gEvt.buttons & GPM_B_LEFT)
		mEvt->buttons &= ~M_BUTTON_LEFT;
	    if (gEvt.buttons & GPM_B_MIDDLE)
		mEvt->buttons &= ~M_BUTTON_MIDDLE;
	    if (gEvt.buttons & GPM_B_RIGHT)
		mEvt->buttons &= ~M_BUTTON_RIGHT;
	}
    }
    chk_mevtdblck(mEvt);
    nKey = getMouseKey(mEvt);

    return( nKey ? SET_CLIPKEY(nKey) : 0 );
}

static void flush_gpmevt(mouseEvent *mEvt)
{
    if( gpm_fd >= 0 )
    {
        struct timeval tv = { 0, 0 };
        fd_set rfds;
	
        FD_ZERO( &rfds ); 
        FD_SET( gpm_fd, &rfds );

        while ( select( gpm_fd+1, &rfds, NULL, NULL, &tv ) > 0 )
	    set_gpmevt(gpm_fd, O_RDONLY, (void *) mEvt);

	while ( getMouseKey(mEvt) );
    }
    return;
}
#endif

static void mouse_init(InOutBase *ioBase)
{
    if( ioBase->terminal_type == TERM_XTERM )
    {
	/* save old hilit tracking & enable mouse tracking */
	write_ttyseq(ioBase, "\033[?1001s\033[?1002h");
	ioBase->is_mouse = 1;
	memset((void *) &ioBase->mLastEvt, 0, sizeof(ioBase->mLastEvt));
	ioBase->mLastEvt.click_delay = DBLCLK_DELAY;
	/* curses mouse buttons check */
	ioBase->mButtons = tigetnum("btns");
	if (ioBase->mButtons < 1)
	    ioBase->mButtons = 2;
    }
#ifdef HAVE_GPM_H
    else if( ioBase->terminal_type == TERM_LINUX )
    {
        ioBase->Conn.eventMask = GPM_MOVE | GPM_DRAG | GPM_UP | GPM_DOWN | GPM_DOUBLE;
        /* give me move events but handle them anyway */
        ioBase->Conn.defaultMask= GPM_MOVE | GPM_HARD; 
        /* only pure mouse events, no Ctrl,Alt,Shft events */
        ioBase->Conn.minMod = ioBase->Conn.maxMod = 0;
        gpm_zerobased = 1;  gpm_visiblepointer = 1;
        if( Gpm_Open( &ioBase->Conn, 0 ) >= 0 && gpm_fd >= 0 )
        {
            ioBase->is_mouse = 1;
	    memset((void *) &ioBase->mLastEvt, 0, sizeof(ioBase->mLastEvt));
	    ioBase->mLastEvt.click_delay = DBLCLK_DELAY;
	    flush_gpmevt(&ioBase->mLastEvt);
	    add_efds(ioBase, gpm_fd, O_RDONLY, set_gpmevt, (void *) &ioBase->mLastEvt);
	    ioBase->mButtons = Gpm_GetSnapshot( NULL );
            if( gpm_visiblepointer )
        	Gpm_DrawPointer( ioBase->mLastEvt.col, ioBase->mLastEvt.row, gpm_consolefd );
	}
    }
#endif
}

static void mouse_exit(InOutBase *ioBase)
{
    if( ioBase->terminal_type == TERM_XTERM )
    {
        /* disable mouse tracking & restore old hilit tracking */
	write_ttyseq(ioBase, "\033[?1002l\033[?1001r");	
    }
#ifdef HAVE_GPM_H
    else if( ioBase->terminal_type == TERM_LINUX )
    {
        if( ioBase->is_mouse && gpm_fd >= 0 )
	{
	    del_efds(ioBase, gpm_fd);
            Gpm_Close();
	}
    }
#endif
}

static void disp_cursor(InOutBase *ioBase)
{
    if (ioBase->cursor != ioBase->lcursor) {
        int lcurs = -1;
        char escseq[ 64 ], *cv = NULL;

        switch( ioBase->cursor ) {
            case SC_NONE:
                lcurs = 1;
                cv = ioBase->civis;
                break;
            case SC_NORMAL:
                lcurs = 2;
                cv = ioBase->cnorm;
                break;
            case SC_INSERT:
                lcurs = 4;
                cv = ioBase->cvvis;
                break;
            case SC_SPECIAL1:
                lcurs = 8;
                cv = ioBase->cvvis;
                break;
            case SC_SPECIAL2:
                /* TODO: find a proper sequqnce to set a cursor
                   to SC_SPECIAL2 under Linux console?
                   There is no such mode in current stable kernels (2.4.20)
                */
                lcurs = 4;
                cv = ioBase->cvvis;
                break;
        }

        if ( lcurs != -1 ) {
            if( ioBase->terminal_type == TERM_LINUX ) {
                snprintf( escseq, sizeof(escseq) - 1, "\033[?25%c\033[?%hdc", 
                                ioBase->cursor == SC_NONE ? 'l' : 'h', lcurs);
                escseq[ sizeof(escseq) - 1 ] = '\0';
                write_ttyseq(ioBase, escseq);
            } else if ( cv != NULL )
                /* curses cursor shape set */
                /* curs_set( ncurs ); */
                write_ttyseq(ioBase, cv);
	    
        }

        ioBase->lcursor = ioBase->cursor;
    }
}

static void set_cursor(InOutBase *ioBase, int style)
{
    switch( style ) {
        case SC_NONE:
        case SC_NORMAL:
        case SC_INSERT:
        case SC_SPECIAL1:
        case SC_SPECIAL2:
            ioBase->cursor = style;
	    disp_cursor( ioBase );
	    break;
    }
}

static void gt_refresh(InOutBase *ioBase)
{
    if( ioBase->disp_count == 0 )
    {
/*
	if (ioBase->cursor == SC_NONE)
	    leaveok( ioBase->stdscr, FALSE );
	else
	    leaveok( ioBase->stdscr, TRUE );
*/
/*	if (ioBase->cursor != SC_NONE) */
	wmove( ioBase->stdscr, ioBase->row, ioBase->col );
	wrefresh( ioBase->stdscr );
	disp_cursor( ioBase );
#ifdef HAVE_GPM_H
        if( ioBase->is_mouse && ioBase->terminal_type == TERM_LINUX )
            if( gpm_visiblepointer )
                Gpm_DrawPointer( ioBase->mLastEvt.col, ioBase->mLastEvt.row, gpm_consolefd );
#endif
    }
}

static void gt_ttyset(InOutBase *ioBase)
{
    if (isatty(ioBase->base_infd))
	tcsetattr( ioBase->base_infd, TCSANOW, &ioBase->curr_TIO );
}

static void gt_ttyrestore(InOutBase *ioBase)
{
    if (ioBase->lTIOsaved)
	tcsetattr( ioBase->base_infd, TCSANOW, &ioBase->saved_TIO );
}

static void gt_outstr(InOutBase *ioBase, int fd, const unsigned char *str, int len)
{
    unsigned char *buf = (unsigned char *) hb_xgrab(len), c;
    int i;
    
    for ( i = 0; i < len; ++i )
    {
	c = str[i];
	if ( ioBase->out_transtbl[c] )
	    buf[i] = ioBase->out_transtbl[c];
	else
	    buf[i] = c ? c : ' ';
    }
    write(fd, buf, len);
    hb_xfree(buf);
}

static void gt_outstd(InOutBase *ioBase, unsigned const char *str, int len)
{
    gt_outstr(ioBase, ioBase->stdoutfd, str, len);
}

static void gt_outerr(InOutBase *ioBase, const char *str, int len)
{
    gt_outstr(ioBase, ioBase->stderrfd, str, len);
}

static char* tiGetS(char *capname)
{
    char *ptr;
    
    ptr = tigetstr( capname );
    if ( ptr )
    {
	if (ptr == (char *)-1)
	    ptr = NULL;
	else if ( !ptr[0] )
	    ptr = NULL;
    }
    return ptr;
}

void get_acsc(InOutBase *ioBase, unsigned char c, chtype *pch)
{
    unsigned char *ptr;

    if (ioBase->acsc != NULL)
	for (ptr = ioBase->acsc; *ptr && *(ptr+1); ptr+=2)
	    if (*ptr == c)
	    {
		*pch = *(ptr+1) | A_ALTCHARSET;
		return;
	    }

    switch (c)
    {
	case '.':	*pch = 'v' | A_NORMAL; break;
	case ',':	*pch = '<' | A_NORMAL; break;
	case '+':	*pch = '>' | A_NORMAL; break;
	case '-':	*pch = '^' | A_NORMAL; break;
	case 'a':	*pch = '#' | A_NORMAL; break;
	case '0':
	case 'h':	get_acsc(ioBase, 'a', pch); break;
	default:	*pch = c | A_ALTCHARSET;
    }
}

static void init_keys(InOutBase *ioBase)
{
    /* virual CTRL/ALT sequences */
    addKeyMap( ioBase, K_METACTRL, CTRL_SEQ );
    addKeyMap( ioBase, K_METAALT,  ALT_SEQ );
    /* national mode key sequences */
#ifdef NATION_SEQ
    addKeyMap( ioBase, K_NATIONAL, NATION_SEQ );
#endif

    /* some harcoded sequences */
    /* addKeyMap( ioBase, K_ESC, "\033\033" ); */
    addKeyMap( ioBase, EXKEY_ENTER, "\r" );
    addKeyMap( ioBase, K_MOUSETERM, "\033[M" );

    if( ioBase->terminal_type == TERM_XTERM ) {

      addKeyMap( ioBase, EXKEY_UP    , "\033[A" );
      addKeyMap( ioBase, EXKEY_DOWN  , "\033[B" );
      addKeyMap( ioBase, EXKEY_RIGHT , "\033[C" );
      addKeyMap( ioBase, EXKEY_LEFT  , "\033[D" );
      addKeyMap( ioBase, EXKEY_CENTER, "\033[E" );
      addKeyMap( ioBase, EXKEY_END   , "\033[F" );
      addKeyMap( ioBase, EXKEY_HOME  , "\033[H" );
      addKeyMap( ioBase, EXKEY_HOME  , "\033[1~" );
      addKeyMap( ioBase, EXKEY_END   , "\033[4~" );
      addKeyMap( ioBase, EXKEY_DEL   , "\177" );

      addKeyMap( ioBase, EXKEY_F1    , "\033[11~" );        // kf1
      addKeyMap( ioBase, EXKEY_F2    , "\033[12~" );        // kf2
      addKeyMap( ioBase, EXKEY_F3    , "\033[13~" );        // kf3
      addKeyMap( ioBase, EXKEY_F4    , "\033[14~" );        // kf4
      addKeyMap( ioBase, EXKEY_F5    , "\033[15~" );        // kf5

      addKeyMap( ioBase, EXKEY_UP    |KEY_CTRLMASK, "\033[5A" );
      addKeyMap( ioBase, EXKEY_DOWN  |KEY_CTRLMASK, "\033[5B" );
      addKeyMap( ioBase, EXKEY_RIGHT |KEY_CTRLMASK, "\033[5C" );
      addKeyMap( ioBase, EXKEY_LEFT  |KEY_CTRLMASK, "\033[5D" );
      addKeyMap( ioBase, EXKEY_CENTER|KEY_CTRLMASK, "\033[5E" );
      addKeyMap( ioBase, EXKEY_END   |KEY_CTRLMASK, "\033[5F" );
      addKeyMap( ioBase, EXKEY_HOME  |KEY_CTRLMASK, "\033[5H" );
      addKeyMap( ioBase, EXKEY_INS   |KEY_CTRLMASK, "\033[2;5~" );
      addKeyMap( ioBase, EXKEY_PGUP  |KEY_CTRLMASK, "\033[5;5~" );
      addKeyMap( ioBase, EXKEY_PGDN  |KEY_CTRLMASK, "\033[6;5~" );

      addKeyMap( ioBase, EXKEY_F1    |KEY_CTRLMASK|KEY_ALTMASK, "\033O2P" );
      addKeyMap( ioBase, EXKEY_F2    |KEY_CTRLMASK|KEY_ALTMASK, "\033O2Q" );
      addKeyMap( ioBase, EXKEY_F3    |KEY_CTRLMASK|KEY_ALTMASK, "\033O2R" );
      addKeyMap( ioBase, EXKEY_F4    |KEY_CTRLMASK|KEY_ALTMASK, "\033O2S" );
      addKeyMap( ioBase, EXKEY_F5    |KEY_CTRLMASK|KEY_ALTMASK, "\033[15;2~" );
      addKeyMap( ioBase, EXKEY_F6    |KEY_CTRLMASK|KEY_ALTMASK, "\033[17;2~" );
      addKeyMap( ioBase, EXKEY_F7    |KEY_CTRLMASK|KEY_ALTMASK, "\033[18;2~" );
      addKeyMap( ioBase, EXKEY_F8    |KEY_CTRLMASK|KEY_ALTMASK, "\033[19;2~" );
      addKeyMap( ioBase, EXKEY_F9    |KEY_CTRLMASK|KEY_ALTMASK, "\033[20;2~" );
      addKeyMap( ioBase, EXKEY_F10   |KEY_CTRLMASK|KEY_ALTMASK, "\033[21;2~" );
      addKeyMap( ioBase, EXKEY_F11   |KEY_CTRLMASK|KEY_ALTMASK, "\033[23;2~" );
      addKeyMap( ioBase, EXKEY_F12   |KEY_CTRLMASK|KEY_ALTMASK, "\033[24;2~" );

      addKeyMap( ioBase, EXKEY_TAB   |KEY_CTRLMASK|KEY_ALTMASK, "\033[Z" );

    } else if( ioBase->terminal_type == TERM_LINUX ) {

      addKeyMap( ioBase, EXKEY_F1 , "\033[[A"  );        // kf1
      addKeyMap( ioBase, EXKEY_F2 , "\033[[B"  );        // kf2
      addKeyMap( ioBase, EXKEY_F3 , "\033[[C"  );        // kf3
      addKeyMap( ioBase, EXKEY_F4 , "\033[[D"  );        // kf4
      addKeyMap( ioBase, EXKEY_F5 , "\033[[E"  );        // kf5
      addKeyMap( ioBase, EXKEY_F6 , "\033[17~" );        // kf6
      addKeyMap( ioBase, EXKEY_F7 , "\033[18~" );        // kf7
      addKeyMap( ioBase, EXKEY_F8 , "\033[19~" );        // kf8
      addKeyMap( ioBase, EXKEY_F9 , "\033[20~" );        // kf9
      addKeyMap( ioBase, EXKEY_F10, "\033[21~" );        // kf10
      addKeyMap( ioBase, EXKEY_F11, "\033[23~" );        // kf11
      addKeyMap( ioBase, EXKEY_F12, "\033[24~" );        // kf12

      addKeyMap( ioBase, EXKEY_F1 |KEY_CTRLMASK|KEY_ALTMASK, "\033[25~" );	// kf13
      addKeyMap( ioBase, EXKEY_F2 |KEY_CTRLMASK|KEY_ALTMASK, "\033[26~" );	// kf14
      addKeyMap( ioBase, EXKEY_F3 |KEY_CTRLMASK|KEY_ALTMASK, "\033[28~" );	// kf15
      addKeyMap( ioBase, EXKEY_F4 |KEY_CTRLMASK|KEY_ALTMASK, "\033[29~" );	// kf16
      addKeyMap( ioBase, EXKEY_F5 |KEY_CTRLMASK|KEY_ALTMASK, "\033[31~" );	// kf17
      addKeyMap( ioBase, EXKEY_F6 |KEY_CTRLMASK|KEY_ALTMASK, "\033[32~" );	// kf18
      addKeyMap( ioBase, EXKEY_F7 |KEY_CTRLMASK|KEY_ALTMASK, "\033[33~" );	// kf19
      addKeyMap( ioBase, EXKEY_F8 |KEY_CTRLMASK|KEY_ALTMASK, "\033[34~" );	// kf20
      addKeyMap( ioBase, EXKEY_F9 |KEY_CTRLMASK|KEY_ALTMASK, "\033[35~" );	// kf21
      addKeyMap( ioBase, EXKEY_F10|KEY_CTRLMASK|KEY_ALTMASK, "\033[36~" );	// kf22
      addKeyMap( ioBase, EXKEY_F11|KEY_CTRLMASK|KEY_ALTMASK, "\033[37~" );	// kf23
      addKeyMap( ioBase, EXKEY_F12|KEY_CTRLMASK|KEY_ALTMASK, "\033[38~" );	// kf24

      addKeyMap( ioBase, EXKEY_F1 |KEY_CTRLMASK, "\033[39~" );        // kf25
      addKeyMap( ioBase, EXKEY_F2 |KEY_CTRLMASK, "\033[40~" );        // kf26
      addKeyMap( ioBase, EXKEY_F3 |KEY_CTRLMASK, "\033[41~" );        // kf27
      addKeyMap( ioBase, EXKEY_F4 |KEY_CTRLMASK, "\033[42~" );        // kf28
      addKeyMap( ioBase, EXKEY_F5 |KEY_CTRLMASK, "\033[43~" );        // kf29
      addKeyMap( ioBase, EXKEY_F6 |KEY_CTRLMASK, "\033[44~" );        // kf30
      addKeyMap( ioBase, EXKEY_F7 |KEY_CTRLMASK, "\033[45~" );        // kf31
      addKeyMap( ioBase, EXKEY_F8 |KEY_CTRLMASK, "\033[46~" );        // kf32
      addKeyMap( ioBase, EXKEY_F9 |KEY_CTRLMASK, "\033[47~" );        // kf33
      addKeyMap( ioBase, EXKEY_F10|KEY_CTRLMASK, "\033[48~" );        // kf34
      addKeyMap( ioBase, EXKEY_F11|KEY_CTRLMASK, "\033[49~" );        // kf35
      addKeyMap( ioBase, EXKEY_F12|KEY_CTRLMASK, "\033[50~" );        // kf36

      addKeyMap( ioBase, EXKEY_F1 |KEY_ALTMASK , "\033[51~" );        // kf37
      addKeyMap( ioBase, EXKEY_F2 |KEY_ALTMASK , "\033[52~" );        // kf38
      addKeyMap( ioBase, EXKEY_F3 |KEY_ALTMASK , "\033[53~" );        // kf39
      addKeyMap( ioBase, EXKEY_F4 |KEY_ALTMASK , "\033[54~" );        // kf40
      addKeyMap( ioBase, EXKEY_F5 |KEY_ALTMASK , "\033[55~" );        // kf41
      addKeyMap( ioBase, EXKEY_F6 |KEY_ALTMASK , "\033[56~" );        // kf42
      addKeyMap( ioBase, EXKEY_F7 |KEY_ALTMASK , "\033[57~" );        // kf43
      addKeyMap( ioBase, EXKEY_F8 |KEY_ALTMASK , "\033[58~" );        // kf44
      addKeyMap( ioBase, EXKEY_F9 |KEY_ALTMASK , "\033[59~" );        // kf45
      addKeyMap( ioBase, EXKEY_F10|KEY_ALTMASK , "\033[70~" );        // kf46
      addKeyMap( ioBase, EXKEY_F11|KEY_ALTMASK , "\033[71~" );        // kf47
      addKeyMap( ioBase, EXKEY_F12|KEY_ALTMASK , "\033[72~" );        // kf48
    }


    /* (curses) termcap/terminfo sequences */

    /* terminal mouse event */
    addKeyMap( ioBase, K_MOUSETERM, "kmous" );

    /* FlagShip extension */
    addKeyMap( ioBase, EXKEY_HOME  | KEY_CTRLMASK, tiGetS( "ked"   ) );
    addKeyMap( ioBase, EXKEY_END   | KEY_CTRLMASK, tiGetS( "kel"   ) );
    addKeyMap( ioBase, EXKEY_PGUP  | KEY_CTRLMASK, tiGetS( "kri"   ) );
    addKeyMap( ioBase, EXKEY_PGDN  | KEY_CTRLMASK, tiGetS( "kind"  ) );
    addKeyMap( ioBase, EXKEY_RIGHT | KEY_CTRLMASK, tiGetS( "kctab" ) );
    addKeyMap( ioBase, EXKEY_LEFT  | KEY_CTRLMASK, tiGetS( "khts"  ) );

    /* some xterms extension */
    addKeyMap( ioBase, EXKEY_HOME,   tiGetS( "kfnd"  ) );
    addKeyMap( ioBase, EXKEY_END,    tiGetS( "kslt"  ) );

    /* keypad */
    addKeyMap( ioBase, EXKEY_CENTER, tiGetS( "kb2"   ) );
    addKeyMap( ioBase, EXKEY_HOME,   tiGetS( "ka1"   ) );
    addKeyMap( ioBase, EXKEY_END,    tiGetS( "kc1"   ) );
    addKeyMap( ioBase, EXKEY_PGUP,   tiGetS( "ka3"   ) );
    addKeyMap( ioBase, EXKEY_PGDN,   tiGetS( "kc3"   ) );

    /* other keys */
    addKeyMap( ioBase, EXKEY_ENTER,  tiGetS( "kent"  ) );
    addKeyMap( ioBase, EXKEY_END,    tiGetS( "kend"  ) );
    addKeyMap( ioBase, EXKEY_PGUP,   tiGetS( "kpp"   ) );
    addKeyMap( ioBase, EXKEY_PGDN,   tiGetS( "knp"   ) );
    addKeyMap( ioBase, EXKEY_UP,     tiGetS( "kcuu1" ) );
    addKeyMap( ioBase, EXKEY_DOWN,   tiGetS( "kcud1" ) );
    addKeyMap( ioBase, EXKEY_RIGHT,  tiGetS( "kcuf1" ) );
    addKeyMap( ioBase, EXKEY_LEFT,   tiGetS( "kcub1" ) );
    addKeyMap( ioBase, EXKEY_HOME,   tiGetS( "khome" ) );
    addKeyMap( ioBase, EXKEY_INS,    tiGetS( "kich1" ) );
    addKeyMap( ioBase, EXKEY_DEL,    tiGetS( "kdch1" ) );
    addKeyMap( ioBase, EXKEY_TAB,    tiGetS( "ht"    ) );
    addKeyMap( ioBase, EXKEY_BS,     tiGetS( "kbs"   ) );
    addKeyMap( ioBase, EXKEY_TAB | KEY_ALTMASK, tiGetS( "kcbt" ) );

    /* function keys */
    addKeyMap( ioBase, EXKEY_F1,     tiGetS( "kf1"   ) );
    addKeyMap( ioBase, EXKEY_F2,     tiGetS( "kf2"   ) );
    addKeyMap( ioBase, EXKEY_F3,     tiGetS( "kf3"   ) );
    addKeyMap( ioBase, EXKEY_F4,     tiGetS( "kf4"   ) );
    addKeyMap( ioBase, EXKEY_F5,     tiGetS( "kf5"   ) );
    addKeyMap( ioBase, EXKEY_F6,     tiGetS( "kf6"   ) );
    addKeyMap( ioBase, EXKEY_F7,     tiGetS( "kf7"   ) );
    addKeyMap( ioBase, EXKEY_F8,     tiGetS( "kf8"   ) );
    addKeyMap( ioBase, EXKEY_F9,     tiGetS( "kf9"   ) );
    addKeyMap( ioBase, EXKEY_F10,    tiGetS( "kf10"  ) );
    addKeyMap( ioBase, EXKEY_F11,    tiGetS( "kf11"  ) );
    addKeyMap( ioBase, EXKEY_F12,    tiGetS( "kf12"  ) );

    /* shifted function keys */
    addKeyMap( ioBase, EXKEY_F1 |KEY_CTRLMASK|KEY_ALTMASK, tiGetS( "kf13" ) );
    addKeyMap( ioBase, EXKEY_F2 |KEY_CTRLMASK|KEY_ALTMASK, tiGetS( "kf14" ) );
    addKeyMap( ioBase, EXKEY_F3 |KEY_CTRLMASK|KEY_ALTMASK, tiGetS( "kf15" ) );
    addKeyMap( ioBase, EXKEY_F4 |KEY_CTRLMASK|KEY_ALTMASK, tiGetS( "kf16" ) );
    addKeyMap( ioBase, EXKEY_F5 |KEY_CTRLMASK|KEY_ALTMASK, tiGetS( "kf17" ) );
    addKeyMap( ioBase, EXKEY_F6 |KEY_CTRLMASK|KEY_ALTMASK, tiGetS( "kf18" ) );
    addKeyMap( ioBase, EXKEY_F7 |KEY_CTRLMASK|KEY_ALTMASK, tiGetS( "kf19" ) );
    addKeyMap( ioBase, EXKEY_F8 |KEY_CTRLMASK|KEY_ALTMASK, tiGetS( "kf20" ) );
    addKeyMap( ioBase, EXKEY_F9 |KEY_CTRLMASK|KEY_ALTMASK, tiGetS( "kf21" ) );
    addKeyMap( ioBase, EXKEY_F10|KEY_CTRLMASK|KEY_ALTMASK, tiGetS( "kf22" ) );
    addKeyMap( ioBase, EXKEY_F11|KEY_CTRLMASK|KEY_ALTMASK, tiGetS( "kf23" ) );
    addKeyMap( ioBase, EXKEY_F12|KEY_CTRLMASK|KEY_ALTMASK, tiGetS( "kf24" ) );
}

static void gt_tone(InOutBase *ioBase, double dFrequency, double dDuration)
{
    char escseq[ 64 ];

    if( ioBase->terminal_type == TERM_LINUX && ioBase->bell != NULL ) {
        snprintf( escseq, sizeof(escseq) - 1, "\033[10;%hd]\033[11;%hd]%s", 
                          (int) dFrequency, 
                          (int)(dDuration * 1000.0 / 18.2), 
                          ioBase->bell );
	escseq[ sizeof(escseq) - 1 ] = '\0';
	write_ttyseq(ioBase, escseq);
    } else
	/* curses beep() */
	if ( ioBase->bell != NULL )
	    write_ttyseq(ioBase, ioBase->bell);
	else if ( ioBase->flash != NULL )
	    write_ttyseq(ioBase, ioBase->flash);
}

static void set_sig_keys(InOutBase *ioBase, int key_int, int key_brk, int key_stp)
{
    if (isatty(ioBase->base_infd)) {

	/* set SIGINT character, default ^C */
	if (key_int >= 0 && key_int <= 255)
	    ioBase->curr_TIO.c_cc[VINTR] = key_int;

	/* set SIGQUIT character, default ^D */
	if (key_brk >= 0 && key_brk <= 255)
	    ioBase->curr_TIO.c_cc[VQUIT] = key_brk;

	/* set SIGTSTP character, default ^Z */
	if (key_stp >= 0 && key_stp <= 255)
	    ioBase->curr_TIO.c_cc[VSUSP] = key_stp;

	/* enable siganls from terminal device */
	if ( ioBase->curr_TIO.c_cc[VINTR] != 0 ||
             ioBase->curr_TIO.c_cc[VQUIT] != 0 ||
             ioBase->curr_TIO.c_cc[VSUSP] != 0 )
            ioBase->curr_TIO.c_lflag |= ISIG;

	/* ioctl( ioBase->base_infd, TIOCSCTTY, 0 ); */
	gt_ttyset( ioBase );
    }
}

static int gt_resize(InOutBase *ioBase)
{

    int ret = -1;
    int rows = 0, cols = 0;

    if (isatty(ioBase->base_outfd)) {
        struct winsize win;

	if (ioctl(ioBase->base_outfd, TIOCGWINSZ, (char *) &win) != -1) {
            rows = win.ws_row;
            cols = win.ws_col;
	}
    }

    if (rows <= 0 || cols <= 0) {
        char *env;

        if ((env = getenv("COLUMNS")))
            cols = atoi(env);
        if ((env = getenv("LINES")))
            rows = atoi(env);
    }

    if (rows > 0 && cols > 0) {
/*
#if defined(NCURSES_VERSION)
	wresize( ioBase->stdscr, rows, cols );
#endif
*/
	endwin();
        gt_refresh( ioBase );
	ret = 0;
/*
#if defined(NCURSES_VERSION)
        if ( resize_term( rows, cols ) == OK ) {
	    ret = 0;
            gt_refresh( ioBase );
	}
#endif
*/
        getmaxyx( ioBase->stdscr, ioBase->maxrow, ioBase->maxcol );
    }
    return ret;
}

static int gt_setsize(InOutBase *ioBase, int rows, int cols)
{
    int ret = -1;
    char escseq[ 64 ];

    if (ioBase->terminal_type == TERM_XTERM) {
        snprintf( escseq, sizeof(escseq) - 1, "\033[8;%hd;%hdt", rows, cols);
        escseq[ sizeof(escseq) - 1 ] = '\0';
        write_ttyseq(ioBase, escseq);
	/* dirty hack - wait for SIGWINCH */
	sleep(3);

        if (s_WinSizeChangeFlag)
        {
            s_WinSizeChangeFlag = FALSE;
            ret = gt_resize(ioBase);
        } else if (isatty(ioBase->base_outfd)) {
            struct winsize win;

            if (ioctl(ioBase->base_outfd, TIOCGWINSZ, (char *) &win) != -1) {
                win.ws_row = rows;
                win.ws_col = cols;
                ioctl(ioBase->base_outfd, TIOCSWINSZ, (char *) &win);
            }
            ret = gt_resize(ioBase);
        }
    }

    return ret;
}

void setKeyTrans(InOutBase *ioBase, unsigned char *ksrc, unsigned char *kdst )
{
    unsigned char n, c;

    if (ksrc && kdst) {
	if (ioBase->in_transtbl == NULL)
	    ioBase->in_transtbl = hb_xgrab(256);

	memset(ioBase->in_transtbl, 0, 256);

	for (n=0; n<256 && (c = ksrc[n]); n++)
	    ioBase->in_transtbl[c] = kdst[n];

    } else if (ioBase->in_transtbl != NULL) {
	hb_xfree(ioBase->in_transtbl);
	ioBase->in_transtbl = NULL;
    }
}

void setDispTrans(InOutBase *ioBase, unsigned char *src, unsigned char *dst, int box )
{
    unsigned char c, d;
    int i, aSet = 0;
    chtype ch;

    if (src && dst)
	aSet = 1;

    for (i = 0; i < 256; i++ )
    {
	ch = ioBase->charmap[i] & 0xffff;
	switch ((ioBase->charmap[i] >> 16) & 0xff)
	{
	    case 1:
		ioBase->std_chmap[i] = ioBase->box_chmap[i] = A_NORMAL;
		break;
	    case 2:
		ioBase->std_chmap[i] = ioBase->box_chmap[i] = A_ALTCHARSET;
		break;
	    case 3:
		ioBase->std_chmap[i] = ioBase->box_chmap[i] = A_PROTECT;
		break;
	    case 4:
		ioBase->std_chmap[i] = ioBase->box_chmap[i] = A_ALTCHARSET | A_PROTECT;
		break;
	    case 5:
		get_acsc(ioBase, ch & 0xff, &ch);
		ioBase->std_chmap[i] = ioBase->box_chmap[i] = ch & ~A_CHARTEXT;
		ch &= A_CHARTEXT;
		break;
	    case 0:
	    default:
		ioBase->std_chmap[i] = aSet ? A_ALTCHARSET : A_NORMAL;
		ioBase->box_chmap[i] = A_ALTCHARSET;
		break;
	}
	ioBase->std_chmap[i] |= ch;
	ioBase->box_chmap[i] |= ch;

	if ( (unsigned int) i != (ch & A_CHARTEXT) && 
	     (ioBase->std_chmap[i] & A_ALTCHARSET) == 0 )
	{
	    if ( ioBase->out_transtbl == NULL )
	    {
		ioBase->out_transtbl = hb_xgrab(256);
		memset(ioBase->out_transtbl, 0, 256);
	    }
	    ioBase->out_transtbl[i] = ch & A_CHARTEXT;
	}
    }
    if (aSet)
    {
	for (i=0; i<256 && (c = src[i]); i++)
	{
	    d = dst[i];
	    ioBase->std_chmap[c] = d | A_NORMAL;
	    if (box)
		ioBase->box_chmap[c] = d | A_NORMAL;
	    if ( c != d )
	    {
		if (ioBase->out_transtbl == NULL)
		{
		    ioBase->out_transtbl = hb_xgrab(256);
		    memset(ioBase->out_transtbl, 0, 256);
		}
		ioBase->out_transtbl[c] = d;
	    }
	}

    }
}

static InOutBase* create_ioBase(char *term, int infd, int outfd, int errfd, pid_t termpid)
{
    InOutBase *ioBase;
    int bg, fg;
    unsigned int i;
    char buf[256], *ptr, *crsterm = NULL;

    ioBase = hb_xgrab(sizeof(*ioBase));
    memset(ioBase, 0, sizeof(*ioBase));

    if (!term || ! *term)
	term = getenv("HB_TERM");
    if (!term || ! *term)
	term = getenv("TERM");

    if (term && *term) {
	if (strncmp( term, "linux", 5 ) == 0)
	    ioBase->terminal_type = TERM_LINUX;
	else if (strstr( term, "xterm" ) != NULL || 
	         strncmp( term, "rxvt", 4 ) == 0 || 
		 strncmp( term, "putty", 4 ) == 0)
	    ioBase->terminal_type = TERM_XTERM;

	if ( (ptr = strchr( term, '/' )) != NULL )
	{
	    if ( (i = ptr - term) >= sizeof(buf) )
		i = sizeof(buf) - 1;
	    strncpy(buf, term, i);
	    buf[i] = '\0';
	    if ( i )
		crsterm = buf;
	}
	else
	    crsterm = term;
    }

    ioBase->esc_delay = ESC_DELAY;
    ioBase->base_infd = infd;
    ioBase->base_outfd = outfd;
    ioBase->stdoutfd = outfd;
    ioBase->stderrfd = errfd;
    ioBase->termpid = termpid;
    ioBase->cursor = ioBase->lcursor = SC_UNDEF;

    if (!isatty(ioBase->base_outfd) && isatty(ioBase->base_infd))
	ioBase->base_outfd = ioBase->base_infd;

    if (isatty(ioBase->base_infd)) {
	tcgetattr( ioBase->base_infd, &ioBase->curr_TIO ); /* save current terminal settings */
	memcpy(&ioBase->saved_TIO, &ioBase->curr_TIO, sizeof(struct termios));
	ioBase->lTIOsaved = 1;

	ioBase->curr_TIO.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	ioBase->curr_TIO.c_lflag |= NOFLSH;
	ioBase->curr_TIO.c_cflag &= ~(CSIZE | PARENB);
	ioBase->curr_TIO.c_cflag |= CS8 | CREAD;
	ioBase->curr_TIO.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	ioBase->curr_TIO.c_oflag &= ~OPOST;
	/* ioBase->curr_TIO.c_oflag |= ONLCR | OPOST; */

	memset(ioBase->curr_TIO.c_cc, 0, NCCS);
    }

    /* curses SCREEN initialisation */
    if (ioBase->base_infd == fileno(stdin))
	ioBase->basein = stdin;
    else
	ioBase->basein = ioBase->basein = fdopen(dup(ioBase->base_infd), "r");

    if (ioBase->base_outfd == fileno(stdout))
	ioBase->baseout = stdout;
    else
	ioBase->baseout = ioBase->baseout = fdopen(dup(ioBase->base_outfd), "w");


    /* curses screen initialization */
    /* initscr(); */
    ioBase->basescr = newterm(crsterm, ioBase->baseout, ioBase->basein);

    if ( ioBase->basescr == NULL ) {
	destroy_ioBase(ioBase);
	return NULL;
    }

    ioBase->stdscr = stdscr;

    ioBase->flash = tiGetS("flash");
    ioBase->bell  = tiGetS("bel");
    ioBase->civis = tiGetS("civis");
    ioBase->cnorm = tiGetS("cnorm");
    ioBase->cvvis = tiGetS("cvvis");
    if ( ioBase->cvvis == NULL )
	ioBase->cvvis = ioBase->cnorm;
    ioBase->acsc  = tiGetS("acsc");

    ioBase->charmap = hb_xgrab(256 * sizeof(int));
    HB_GT_FUNC(gt_chrmapinit(ioBase->charmap, term));
    setDispTrans(ioBase, NULL, NULL, 0 );

    ioBase->attr_mask = -1;
    if( has_colors() ) {
        /*  DOS->CURSES color maping
          DOS              -> curses
          --------------------------------
           0 black         -> COLOR_BLACK
           1 blue          -> COLOR_BLUE
           2 green         -> COLOR_GREEN
           3 cyan          -> COLOR_CYAN
           4 red           -> COLOR_RED
           5 magenta       -> COLOR_MAGENTA
           6 yellow        -> COLOR_YELLOW
           7 light gray    -> COLOR_WHITE
    
           8 gray          -> BOLD/BLINK BLACK
           9 light blue    -> BOLD/BLINK BLUE
          10 light green   -> BOLD/BLINK GREEN
          11 light cyan    -> BOLD/BLINK CYAN
          12 light red     -> BOLD/BLINK RED
          13 light magenta -> BOLD/BLINK MAGENTA
          14 light yellow  -> BOLD/BLINK YELLOW
          15 white         -> BOLD/BLINK WHITE
        */
        static const char color_map[] = { COLOR_BLACK,
                                          COLOR_BLUE,
                                          COLOR_GREEN,
                                          COLOR_CYAN,
                                          COLOR_RED,
                                          COLOR_MAGENTA,
                                          COLOR_YELLOW,
                                          COLOR_WHITE };

        start_color();
        for( bg = 0; bg < COLORS; bg++ )
            for( fg = 0; fg < COLORS; fg++ )
                init_pair( bg * COLORS + fg, color_map[ fg ], color_map[ bg ] );

        for( i = 0; i < 256; i++  ) {
            bg = ( i >> 4 ) & 0x07;         /* extract background color bits 4-6 */
            fg = ( i & 0x07 );              /* extract forground color bits 0-2 */
            ioBase->attr_map[ i ] = COLOR_PAIR( (bg * COLORS + fg) );
            if( i & 0x08 )                  /* highlight forground bit 3 */
                ioBase->attr_map[ i ] |= A_BOLD;
            if( i & 0x80 )                  /* blink/highlight background bit 7 */
                ioBase->attr_map[ i ] |= A_BLINK;
        }
	ioBase->is_color = 1;
    } else {
        for( i = 0; i < 256; i++  ) {
            bg = ( i >> 4 ) & 0x07;         /* extract background color bits 4-6 */
            fg = ( i & 0x07 );              /* extract forground color bits 0-2 */
            ioBase->attr_map[ i ] = 0;
            if( fg < bg )
                ioBase->attr_map[ i ] |= A_REVERSE;
            if( fg == 1 )                   /* underline? */
                ioBase->attr_map[ i ] |= A_UNDERLINE;
            if( i & 0x08 )                  /* highlight forground bit 3 */
                ioBase->attr_map[ i ] |= A_BOLD;
            if( i & 0x80 )                  /* blink/highlight background bit 7 */
                ioBase->attr_map[ i ] |= A_BLINK;
	ioBase->is_color = 0;
      }
    }

    getmaxyx( ioBase->stdscr, ioBase->maxrow, ioBase->maxcol );
    scrollok( ioBase->stdscr, FALSE );
    idcok( ioBase->stdscr, FALSE );
    wbkgdset( ioBase->stdscr, ' ' ); //| ioBase->attr_map[ 7 ] );
    wclear( ioBase->stdscr );
    wrefresh( ioBase->stdscr );
    /* curs_set( 0 ); */

    /* curses keyboard initialization */
    /* we have our own keyboard routine so it's unnecessary now */
/*
    raw();
    nodelay( stdscr, TRUE);
    keypad( stdscr, FALSE);
    timeout( 0 );
    noecho();
*/

    gt_ttyset( ioBase );
    add_efds(ioBase, ioBase->base_infd, O_RDONLY, NULL, NULL);

    init_keys(ioBase);
    mouse_init(ioBase);

    return ioBase;
}

static void destroy_ioBase(InOutBase *ioBase)
{
    mouse_exit(ioBase);
    del_all_efds(ioBase);

    if( ioBase->terminal_type == TERM_LINUX ) {
	/* restore a standard bell frequency and duration */
	write_ttyseq(ioBase, "\033[10]\033[11]");
    }

    /* curses SCREEN delete */
    if (ioBase->stdscr != NULL) {
	ioBase->disp_count = 0;
	/* on exit restore a cursor share and leave it visible
	   Marek's NOTE: This is incompatible with Clipper */
	if (ioBase->cursor != SC_UNDEF)
	    set_cursor(ioBase, SC_NORMAL);
	gt_refresh(ioBase);
    }
    if (ioBase->basescr != NULL)
	delscreen(ioBase->basescr);
    if (ioBase->basein != NULL && ioBase->basein != stdin)
	fclose(ioBase->basein);
    if (ioBase->baseout != NULL && ioBase->baseout != stdout)
	fclose(ioBase->baseout);

    /* free allocated memory */
    if (ioBase->charmap != NULL)
	hb_xfree(ioBase->charmap);

    if (ioBase->in_transtbl != NULL)
	hb_xfree(ioBase->in_transtbl);

    if (ioBase->out_transtbl != NULL)
	hb_xfree(ioBase->out_transtbl);

    if (ioBase->nation_transtbl != NULL)
	hb_xfree(ioBase->nation_transtbl);

    if (ioBase->pKeyTab != NULL)
	removeAllKeyMap(&ioBase->pKeyTab);

    /* restore terminal settings */
    gt_ttyrestore(ioBase);

    /* kill terminal proces if any */
    if (ioBase->termpid > 0) {
	kill(ioBase->termpid, SIGTERM);
	waitpid(ioBase->termpid, NULL, 0);
    }

    hb_xfree(ioBase);
}

static InOutBase* create_newXterm()
{
#if defined( HB_OS_LINUX ) || defined( HB_OS_BSD )
#if 0
    int masterfd, slavefd, fd;
    pid_t termpid;
    char ptyname[64], buf[64], *ptr;

    if (!getenv("DISPLAY"))
	return NULL;

    if (openpty(&masterfd, &slavefd, ptyname, NULL, NULL) == -1)
	return NULL;

    tcsetpgrp(masterfd, getpgrp());

    if ((termpid = fork()) == -1) {
	close(masterfd);
	close(slavefd);
	return NULL;
    }

    if (termpid == 0) {
	if ((ptr = strrchr(ptyname, '/')))
	    ++ptr;
	else
	    ptr = ptyname;
	snprintf(buf, sizeof(buf), "-S%s/%d", ptr, masterfd);
	buf[sizeof(buf)-1] = '\0';
/*
        close(0);
        close(1);
        close(2);
*/
/*
	dup2(masterfd, 0);
	dup2(masterfd, 1);
	//fd = open("/tmp/hb-xterm.log", O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
	fd = open("/dev/null", O_WRONLY);
	dup2(fd, 2);
*/
	for (fd = 3; fd < MAXFD; ++fd)
	    if (fd != masterfd)
		close(fd);

	setuid(getuid());
	setgid(getgid());
	execlp( "xterm", "xterm", buf, "+sb",
	        "-fg", "white",
		"-bg", "black",
		"-fn", "fixed",
		"-T", "HB-XTERM Window", NULL );
	exit(0);
    }
    close(masterfd);
    return create_ioBase("xterm", slavefd, slavefd, slavefd, termpid);
#endif
#endif
    return NULL;
}

static int set_active_ioBase( int iNO_ioBase )
{
    int iPrev = s_iActive_ioBase;

    if ( iNO_ioBase >= 0 && iNO_ioBase < s_iSize_ioBaseTab )
    {
	s_iActive_ioBase = iNO_ioBase;
	s_ioBase = s_ioBaseTab[s_iActive_ioBase];
	set_term(s_ioBase->basescr);
    }

    return iPrev;
}

static int add_new_ioBase(InOutBase *ioBase)
{
    int i, n, add = 0;

    for( i = 0; i < s_iSize_ioBaseTab && !add; ++i )
	if ( !s_ioBaseTab[i] )
	{
	    s_ioBaseTab[i] = ioBase;
	    add = 1;
	}

    if ( !add )
    {
	if (s_ioBaseTab == NULL)
	    s_ioBaseTab = hb_xgrab( (s_iSize_ioBaseTab += 10) * sizeof(InOutBase*) );
	else
	    s_ioBaseTab = hb_xrealloc( s_ioBaseTab, 
			   (s_iSize_ioBaseTab += 10) * sizeof(InOutBase*) );
	s_ioBaseTab[i] = ioBase;
	for (n = i + 1; n < s_iSize_ioBaseTab; n++)
	    s_ioBaseTab[n] = NULL;
    }

    if ( !s_ioBase )
	set_active_ioBase(i);

    return i;
}

static int del_ioBase( int iNO_ioBase )
{
    int i;

    if ( iNO_ioBase >= 0 && iNO_ioBase < s_iSize_ioBaseTab )
    {
	destroy_ioBase( s_ioBaseTab[iNO_ioBase] );
	s_ioBaseTab[iNO_ioBase] = NULL;
	if ( s_iActive_ioBase == iNO_ioBase )
	{
	    s_iActive_ioBase = -1;
	    s_ioBase = NULL;
	    for( i = 0; i < s_iSize_ioBaseTab && !s_ioBase; ++i )
		if ( s_ioBaseTab[i] )
		    set_active_ioBase(i);
	}
    }

    return s_iActive_ioBase;
}

static void del_all_ioBase()
{
    int i;

    if ( s_ioBaseTab )
    {
	for( i = 0; i < s_iSize_ioBaseTab; ++i )
	    if ( s_ioBaseTab[i] )
		destroy_ioBase( s_ioBaseTab[i] );
	hb_xfree( s_ioBaseTab );
	s_ioBaseTab = NULL;
    }
    s_iActive_ioBase = -1;
    s_ioBase = NULL;
}



/* *********************************************************************** */

void HB_GT_FUNC(gt_Init( int iFilenoStdin, int iFilenoStdout, int iFilenoStderr ))
{
    InOutBase *ioBase;

    HB_TRACE(HB_TR_DEBUG, ("hb_gt_Init()"));

    if ( !s_ioBase )
    {
#ifdef HB_GT_CRS_TTYHACK
	int ittyfd;
	if ( (ittyfd = open("/dev/tty",O_RDWR)) != -1)
	    iFilenoStdin = iFilenoStdout = ittyfd;
#endif
	set_signals();
	ioBase = create_ioBase( NULL, iFilenoStdin, iFilenoStdout, iFilenoStderr, -1 );
	add_new_ioBase( ioBase );
    }
    if ( !s_ioBase )
    {
        char *errmsg = "\r\nInternal error : screen driver initialization failure\r\n";
        /* TODO: a standard Harbour error should be generated here ! */
        write( iFilenoStderr, errmsg , strlen( errmsg ) );
        exit( 20 );
    }
}

/* *********************************************************************** */

void HB_GT_FUNC(gt_Exit( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_Exit()"));

    del_all_ioBase();
}

/* *********************************************************************** */

USHORT HB_GT_FUNC(gt_GetScreenHeight( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_GetScreenHeight()"));

    if (s_WinSizeChangeFlag)
    {
	s_WinSizeChangeFlag = FALSE;
	gt_resize(s_ioBase);
    }

    return s_ioBase->maxrow;
}
/* *********************************************************************** */

USHORT HB_GT_FUNC(gt_GetScreenWidth( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_GetScreenWidth()"));

    if (s_WinSizeChangeFlag)
    {
	s_WinSizeChangeFlag = FALSE;
	gt_resize(s_ioBase);
    }

    return s_ioBase->maxcol;
}


/* *********************************************************************** */

SHORT HB_GT_FUNC(gt_Col( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_Col()"));

    return s_ioBase->col;
}

/* *********************************************************************** */

SHORT HB_GT_FUNC(gt_Row( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_Row()"));

    return s_ioBase->row;
}

/* *********************************************************************** */

void HB_GT_FUNC(gt_SetPos( SHORT iRow, SHORT iCol, SHORT iMethod ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_SetPos(%hd, %hd, %hd)", iRow, iCol, iMethod));

    HB_SYMBOL_UNUSED( iMethod );

    s_ioBase->row = iRow;
    s_ioBase->col = iCol;

    gt_refresh( s_ioBase );
}

/* *********************************************************************** */

BOOL HB_GT_FUNC(gt_AdjustPos( BYTE * pStr, ULONG ulLen ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_AdjustPos(%s, %lu)", pStr, ulLen ));

    HB_SYMBOL_UNUSED( pStr );
    HB_SYMBOL_UNUSED( ulLen );

    return FALSE;
}

/* *********************************************************************** */

USHORT HB_GT_FUNC(gt_GetCursorStyle( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_GetCursorStyle()"));

    if ( s_ioBase->cursor == SC_UNDEF )
        return SC_NORMAL;

    return s_ioBase->cursor;
}

/* *********************************************************************** */

void HB_GT_FUNC(gt_SetCursorStyle( USHORT uiStyle ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_SetCursorStyle(%hu)", uiStyle));

    set_cursor(s_ioBase, uiStyle);
}

/* *********************************************************************** */

BOOL HB_GT_FUNC(gt_IsColor( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_IsColor()"));

    return s_ioBase->is_color;
}

/* *********************************************************************** */

void HB_GT_FUNC(gt_DispBegin( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_DispBegin()"));

    s_ioBase->disp_count++;
}

/* *********************************************************************** */

void HB_GT_FUNC(gt_DispEnd( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_DispEnd()"));

    if( s_ioBase->disp_count )
        s_ioBase->disp_count--;

    gt_refresh( s_ioBase );
}

/* *********************************************************************** */

USHORT HB_GT_FUNC(gt_DispCount( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_DispCount()"));

    return s_ioBase->disp_count;
}

/* *********************************************************************** */

BOOL HB_GT_FUNC(gt_SetMode( USHORT uiRows, USHORT uiCols ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_SetMode(%hu, %hu)", uiRows, uiCols));

    if (gt_setsize( s_ioBase, uiRows, uiCols ) == 0)
	return TRUE;

    return FALSE;
}

/* *********************************************************************** */

BOOL HB_GT_FUNC(gt_GetBlink( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_GetBlink()"));

    /* TODO: current implementation disables blinking/intensity */
    return ( s_ioBase->attr_mask & A_BLINK ) ? TRUE : FALSE;
}

/* *********************************************************************** */

void HB_GT_FUNC(gt_SetBlink( BOOL bBlink ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_SetBlink(%d)", (int) bBlink));

    /* TODO: current implementation disables blinking/intensity */
    if ( bBlink )
        s_ioBase->attr_mask |= A_BLINK;
    else
        s_ioBase->attr_mask &= ~A_BLINK;
}

/* *********************************************************************** */

void HB_GT_FUNC(gt_Tone( double dFrequency, double dDuration ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_Tone(%lf, %lf)", dFrequency, dDuration));

    gt_tone(s_ioBase, dFrequency, dDuration);

    if( s_ioBase->terminal_type == TERM_LINUX )
    {
        /* NOTE : the code below is adapted from gtdos.c/hb_gt_Tone() */

        dDuration *= 1800;
        while( dDuration > 0.0 )
        {
            USHORT temp = ( USHORT ) HB_MIN( HB_MAX( 0, dDuration ), 1000 );
#ifndef HB_OS_DARWIN	    
            static struct timespec nanosecs;
#endif

            dDuration -= temp;
            if( temp <= 0 )
                /* Ensure that the loop gets terminated when
                    only a fraction of the delay time remains. */
                dDuration = -1.0;
            else
            {
                hb_idleState();
#ifndef HB_OS_DARWIN		
                nanosecs.tv_sec  = 0;
                nanosecs.tv_nsec = temp * 10;
                nanosleep( &nanosecs, NULL );
#else
		usleep(temp / 100);
#endif
            }
        }
	hb_idleReset();
    }
}

/* *********************************************************************** */

char * HB_GT_FUNC(gt_Version( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_Version()"));

    return "Harbour Terminal: nCurses";
}

/* *********************************************************************** */

void HB_GT_FUNC(gt_OutStd( BYTE * pbyStr, ULONG ulLen ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_OutStd(%s, %hu)", pbyStr, ulLen));

    if (s_ioBase->baseout != NULL && s_ioBase->base_outfd == s_ioBase->stdoutfd)
    {
        HB_GT_FUNC(gt_DispBegin());
        hb_gtWriteCon( pbyStr, ulLen );
        HB_GT_FUNC(gt_DispEnd());
    }
    else
        gt_outstd( s_ioBase, pbyStr, ulLen );
}

/* *********************************************************************** */

void HB_GT_FUNC(gt_OutErr( BYTE * pbyStr, ULONG ulLen ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_OutErr(%s, %hu)", pbyStr, ulLen));

    gt_outerr( s_ioBase, pbyStr, ulLen );
}

/* *********************************************************************** */

BOOL HB_GT_FUNC(gt_Suspend( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_Suspend()"));

    gt_refresh( s_ioBase );
    endwin();
    gt_ttyrestore( s_ioBase );
    return TRUE;
}

/* *********************************************************************** */

BOOL HB_GT_FUNC(gt_Resume( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_Resume()"));

    wrefresh( s_ioBase->stdscr );
    gt_ttyset( s_ioBase );
    /* redrawwin( curscr ); */
    gt_refresh( s_ioBase );
    return TRUE;
}

/* *********************************************************************** */

BOOL HB_GT_FUNC(gt_PreExt( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_PreExt()"));

    gt_refresh( s_ioBase );
    return TRUE;
}

/* *********************************************************************** */

BOOL HB_GT_FUNC(gt_PostExt( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_PostExt()"));

    return TRUE;
}

/* *********************************************************************** */

int HB_GT_FUNC(gt_RectSize( USHORT rows, USHORT cols ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_RectSize(%hu, %hu)", rows, cols));

    return rows * cols * sizeof( chtype );
}

/* *********************************************************************** */

void HB_GT_FUNC(gt_GetText( USHORT uiTop, USHORT uiLeft, USHORT uiBottom, USHORT uiRight, BYTE * pbyDst ))
{
    int col;
    chtype * pBuffer = ( chtype * ) pbyDst;

    HB_TRACE(HB_TR_DEBUG, ("hb_gt_GetText(%hu, %hu, %hu, %hu, %p)", uiTop, uiLeft, uiBottom, uiRight, pbyDst));

    while( uiTop <= uiBottom )
    {
        for( col = uiLeft; col <= uiRight; col++, pBuffer++)
            *pBuffer = mvwinch( s_ioBase->stdscr, uiTop, col );
        ++uiTop;
    }
}

/* *********************************************************************** */

void HB_GT_FUNC(gt_PutText( USHORT uiTop, USHORT uiLeft, USHORT uiBottom, USHORT uiRight, BYTE * pbySrc ))
{
    int cols;
    chtype * pBuffer = ( chtype * ) pbySrc;

    HB_TRACE(HB_TR_DEBUG, ("hb_gt_PutText(%hu, %hu, %hu, %hu, %p)", uiTop, uiLeft, uiBottom, uiRight, pbySrc));

    cols = uiRight - uiLeft + 1;
    while( uiTop <= uiBottom )
    {
        mvwaddchnstr( s_ioBase->stdscr, uiTop, uiLeft, pBuffer, cols );
        pBuffer += cols;
        ++uiTop;
    }
    gt_refresh( s_ioBase );
}

/* *********************************************************************** */

void HB_GT_FUNC(gt_SetAttribute( USHORT uiTop, USHORT uiLeft, USHORT uiBottom, USHORT uiRight, BYTE byAttr ))
{
    int newAttr = s_ioBase->attr_map[ byAttr ] & s_ioBase->attr_mask;
    int dx;
    chtype c;

    HB_TRACE(HB_TR_DEBUG, ("hb_gt_SetAttribute(%hu, %hu, %hu, %hu, %d)", uiTop, uiLeft, uiBottom, uiRight, (int) byAttr));

    while( uiTop <= uiBottom )
    {
        for( dx = uiLeft; dx <= uiRight; dx++ )
        {
            c = mvwinch( s_ioBase->stdscr, uiTop, dx );
            /* extract character only (remember about alternate chars) */
            c &= (A_CHARTEXT | A_NORMAL | A_ALTCHARSET | A_PROTECT);
            /* set new attribute */
            c |= newAttr;
            mvwaddch( s_ioBase->stdscr, uiTop, dx , c );
        }
        uiTop++;
    }
    gt_refresh( s_ioBase );
}

/* *********************************************************************** */

void HB_GT_FUNC(gt_Puts( USHORT uiRow, USHORT uiCol, BYTE byAttr, BYTE * pbyStr, ULONG ulLen ))
{
    ULONG i;
    int attr;

    HB_TRACE(HB_TR_DEBUG, ("hb_gt_Puts(%hu, %hu, %d, %p, %lu)", uiRow, uiCol, (int) byAttr, pbyStr, ulLen));

    attr = s_ioBase->attr_map[ byAttr ] & s_ioBase->attr_mask;
    wmove( s_ioBase->stdscr, uiRow, uiCol );

    for( i = 0; i < ulLen; ++i )
        waddch( s_ioBase->stdscr, s_ioBase->std_chmap[ pbyStr[ i ] ] | attr );

    getyx( s_ioBase->stdscr, s_ioBase->row, s_ioBase->col );
    gt_refresh( s_ioBase );
}

/* *********************************************************************** */

void HB_GT_FUNC(gt_Replicate( USHORT uiRow, USHORT uiCol, BYTE byAttr, BYTE byChar, ULONG ulLen ))
{
    chtype ch;

    HB_TRACE(HB_TR_DEBUG, ("hb_gt_Replicate(%hu, %hu, %i, %i, %lu)", uiRow, uiCol, byAttr, byChar, ulLen));

    ch = s_ioBase->box_chmap[ byChar ] | 
         ( s_ioBase->attr_map[ byAttr ] & s_ioBase->attr_mask );
    wmove( s_ioBase->stdscr, uiRow, uiCol );

    while( ulLen-- )
        waddch( s_ioBase->stdscr, ch );

    /* getyx( s_ioBase->stdscr, s_ioBase->row, s_ioBase->col ); */
    gt_refresh( s_ioBase );
}

/* *********************************************************************** */

void HB_GT_FUNC(gt_Scroll( USHORT usTop, USHORT usLeft, USHORT usBottom, USHORT usRight, BYTE byAttr, SHORT iRows, SHORT iCols ))
{
    UINT uiSize;

    int iLength = ( usRight - usLeft ) + 1;
    int iCount, iColOld, iColNew, iColSize;

    HB_TRACE(HB_TR_DEBUG, ("hb_gt_Scroll(%hu, %hu, %hu, %hu, %d, %hd, %hd)", usTop, usLeft, usBottom, usRight, (int) byAttr, iRows, iCols));

    if( hb_gtRectSize( usTop, usLeft, usBottom, usRight, &uiSize ) == 0 )
    {
        unsigned char * fpBlank = ( unsigned char * ) hb_xgrab( iLength );
        unsigned char * fpBuff = ( unsigned char * ) hb_xgrab( iLength * sizeof( chtype ) );

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

	HB_GT_FUNC(gt_DispBegin());

        for( iCount = ( iRows >= 0 ? usTop : usBottom );
              ( iRows >= 0 ? iCount <= usBottom : iCount >= usTop );
              ( iRows >= 0 ? iCount++ : iCount-- ) )
        {
            int iRowPos = iCount + iRows;

            /* Read the text to be scrolled into the current row */
            if( ( iRows || iCols ) && iRowPos <= usBottom && iRowPos >= usTop )
                HB_GT_FUNC(gt_GetText( iRowPos, iColOld, iRowPos, iColOld + iColSize, fpBuff ));

            /* Blank the scroll region in the current row */
            HB_GT_FUNC(gt_Puts( iCount, usLeft, byAttr, fpBlank, iLength ));

            /* Write the scrolled text to the current row */
            if( ( iRows || iCols ) && iRowPos <= usBottom && iRowPos >= usTop )
                HB_GT_FUNC(gt_PutText( iCount, iColNew, iCount, iColNew + iColSize, fpBuff ));
        }

        hb_xfree( fpBlank );
        hb_xfree( fpBuff );

	HB_GT_FUNC(gt_DispEnd());
    }
}

/* *********************************************************************** */

USHORT HB_GT_FUNC(gt_Box( SHORT Top, SHORT Left, SHORT Bottom, SHORT Right,
                        BYTE * szBox, BYTE byAttr ))
{
    int ret = 1;
    chtype chAttr;
    SHORT Row;
    SHORT Col;
    SHORT Height;
    SHORT Width;
    USHORT maxrow, maxcol;

    HB_TRACE(HB_TR_DEBUG, ("hb_gt_box(%i, %i, %i, %i, %s, %hu)", Top, Left, Bottom, Right, szBox, byAttr));

    maxrow = HB_GT_FUNC(gt_GetScreenHeight());
    maxcol = HB_GT_FUNC(gt_GetScreenWidth());

    if( ( Left   >= 0 && Left   < maxcol )  || 
        ( Right  >= 0 && Right  < maxcol )  || 
        ( Top    >= 0 && Top    < maxrow )  || 
        ( Bottom >= 0 && Bottom < maxrow ) )
    {
	HB_GT_FUNC(gt_DispBegin());

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

	/* get color attribute */
	chAttr = s_ioBase->attr_map[ byAttr ] & s_ioBase->attr_mask;

        /* Draw the box or line as specified */
        Height = Bottom - Top + 1;
        Width  = Right - Left + 1;

        if( Height > 1 && Width > 1 && Top >= 0 && Top < maxrow && Left >= 0 && Left < maxcol )
            mvwaddch( s_ioBase->stdscr, Top, Left,
                      s_ioBase->box_chmap[ szBox[ 0 ] ] | chAttr );

        Col = ( Height > 1 ? Left + 1 : Left );
        if(Col < 0 )
        {
            Width += Col;
            Col = 0;
        }
        if( Right >= maxcol )
        {
            Width -= Right - maxcol;
        }

        if( Col <= Right && Col < maxcol && Top >= 0 && Top < maxrow )
            /* Top line */
            HB_GT_FUNC(gt_Replicate( Top, Col, byAttr, szBox[ 1 ], Width + ( (Right - Left) > 1 ? -2 : 0 ) ));

        if( Height > 1 && (Right - Left) > 1 && Right < maxcol && Top >= 0 && Top < maxrow )
            /* Upper right corner */
            mvwaddch( s_ioBase->stdscr, Top, Right,
                      s_ioBase->box_chmap[ szBox[ 2 ] ] | chAttr );

        if( szBox[ 8 ] && Height > 2 && Width > 2 )
        {
            for( Row = Top + 1; Row < Bottom; Row++ )
            {
                if( Row >= 0 && Row < maxrow )
                {
                    Col = Left;
                    if( Col < 0 )
                        Col = 0; /* The width was corrected earlier. */
                    else
			/* Left side */
			mvwaddch( s_ioBase->stdscr, Row, Col++,
                                  s_ioBase->box_chmap[ szBox[ 7 ] ] | chAttr );
                    /* Fill */
                    HB_GT_FUNC(gt_Replicate( Row, Col, byAttr, szBox[ 8 ], Width - 2 ));
                    if( Right < maxcol )
                        /* Right side */
			mvwaddch( s_ioBase->stdscr, Row, Right,
                                  s_ioBase->box_chmap[ szBox[ 3 ] ] | chAttr );
                }
            }
        }
        else
        {
            for( Row = ( Width > 1 ? Top + 1 : Top ); Row < ( (Right - Left ) > 1 ? Bottom : Bottom + 1 ); Row++ )
            {
                if( Row >= 0 && Row < maxrow )
                {
                    if( Left >= 0 && Left < maxcol )
			/* Left side */
			mvwaddch( s_ioBase->stdscr, Row, Left,
                                  s_ioBase->box_chmap[ szBox[ 7 ] ] | chAttr );
                    if( ( Width > 1 || Left < 0 ) && Right < maxcol )
			/* Right side */
			mvwaddch( s_ioBase->stdscr, Row, Right,
                                  s_ioBase->box_chmap[ szBox[ 3 ] ] | chAttr );
                }
            }
        }

        if( Height > 1 && Width > 1 )
        {
            if( Left >= 0 && Bottom < maxrow )
		/* Bottom left corner */
		mvwaddch( s_ioBase->stdscr, Bottom, Left,
                          s_ioBase->box_chmap[ szBox[ 6 ] ] | chAttr );

            Col = Left + 1;
            if( Col < 0 )
                Col = 0; /* The width was corrected earlier. */

            if( Col <= Right && Bottom < maxrow )
		/* Bottom line */
                HB_GT_FUNC(gt_Replicate( Bottom, Col, byAttr, szBox[ 5 ], Width - 2 ));

            if( Right < maxcol && Bottom < maxrow )
		/* Bottom right corner */
		mvwaddch( s_ioBase->stdscr, Bottom, Right,
                          s_ioBase->box_chmap[ szBox[ 4 ] ] | chAttr );
        }

        /* getyx( s_ioBase->stdscr, s_ioBase->row, s_ioBase->col ); */
	HB_GT_FUNC(gt_DispEnd());
        ret = 0;
    }

    return ret;
}

/* *********************************************************************** */

USHORT HB_GT_FUNC(gt_BoxD( SHORT Top, SHORT Left, SHORT Bottom, SHORT Right, BYTE * pbyFrame, BYTE byAttr ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_boxD(%i, %i, %i, %i, %s, %hu)", Top, Left, Bottom, Right, pbyFrame, byAttr));

    return HB_GT_FUNC(gt_Box( Top, Left, Bottom, Right, pbyFrame, byAttr ));
}

/* *********************************************************************** */

USHORT HB_GT_FUNC(gt_BoxS( SHORT Top, SHORT Left, SHORT Bottom, SHORT Right, BYTE * pbyFrame, BYTE byAttr ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_boxD(%i, %i, %i, %i, %s, %hu)", Top, Left, Bottom, Right, pbyFrame, byAttr));

    return HB_GT_FUNC(gt_Box( Top, Left, Bottom, Right, pbyFrame, byAttr ));
}

/* *********************************************************************** */

USHORT HB_GT_FUNC(gt_HorizLine( SHORT Row, SHORT Left, SHORT Right, BYTE byChar, BYTE byAttr ))
{
    int ret = 1;
    USHORT uCols;
    chtype ch;
    USHORT maxrow, maxcol;

    HB_TRACE(HB_TR_DEBUG, ("hb_gt_HorizLine(%i, %i, %i, %hu, %hu)", Row, Left, Right, byChar, byAttr));

    maxrow = HB_GT_FUNC(gt_GetScreenHeight());
    maxcol = HB_GT_FUNC(gt_GetScreenWidth());

    if( Row >= 0 && Row < maxrow )
    {
        if( Left < 0 )
            Left = 0;
        else if( Left >= maxcol )
            Left = maxcol - 1;

        if( Right < 0 )
            Right = 0;
        else if( Right >= maxcol )
            Right = maxcol - 1;

        if( Left <= Right )
	{
	    wmove( s_ioBase->stdscr, Row, Left );
	    uCols = Right - Left + 1;
	}
	else
	{
	    wmove( s_ioBase->stdscr, Row, Right );
	    uCols = Left - Right + 1;
	}

	ch = s_ioBase->box_chmap[ byChar ] | 
	     ( s_ioBase->attr_map[ byAttr ] & s_ioBase->attr_mask );
	while( uCols-- )
            waddch( s_ioBase->stdscr, ch );

        //getyx( s_ioBase->stdscr, s_ioBase->row, s_ioBase->col );
        gt_refresh( s_ioBase );
        ret = 0;
    }

    return ret;
}

/* *********************************************************************** */

USHORT HB_GT_FUNC(gt_VertLine( SHORT Col, SHORT Top, SHORT Bottom, BYTE byChar, BYTE byAttr ))
{
    int ret = 1;
    USHORT uRow;
    chtype ch;
    USHORT maxrow, maxcol;

    HB_TRACE(HB_TR_DEBUG, ("hb_gt_VertLine(%i, %i, %i, %hu, %hu)", Col, Top, Bottom, byChar, byAttr));

    maxrow = HB_GT_FUNC(gt_GetScreenHeight());
    maxcol = HB_GT_FUNC(gt_GetScreenWidth());

    if( Col >= 0 && Col < maxcol )
    {
        if( Top < 0 )
            Top = 0;
        else if( Top >= maxrow )
            Top = maxrow - 1;

        if( Bottom < 0 )
            Bottom = 0;
        else if( Bottom >= maxrow )
            Bottom = maxrow - 1;

        if( Top <= Bottom )
            uRow = Top;
        else
        {
            uRow = Bottom;
            Bottom = Top;
        }

	ch = s_ioBase->box_chmap[ byChar ] |
	     ( s_ioBase->attr_map[ byAttr ] & s_ioBase->attr_mask );

        while( uRow <= Bottom )
            mvwaddch( s_ioBase->stdscr, uRow++, Col, ch );

        //getyx( s_ioBase->stdscr, s_ioBase->row, s_ioBase->col );
        gt_refresh( s_ioBase );
        ret = 0;
    }

    return ret;
}

/* *********************************************************************** */

void HB_GT_FUNC(mouse_Init( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_mouse_Init()"));
}

/* *********************************************************************** */

void HB_GT_FUNC(mouse_Exit( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_mouse_Exit()"));
}

/* *********************************************************************** */

BOOL HB_GT_FUNC(mouse_IsPresent( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_mouse_IsPresent()"));

    return s_ioBase->is_mouse;
}

/* *********************************************************************** */

void HB_GT_FUNC(mouse_Show( void ))
{
   HB_TRACE(HB_TR_DEBUG, ("hb_mouse_Show()"));

#ifdef HAVE_GPM_H
    if( s_ioBase->terminal_type == TERM_LINUX && s_ioBase->is_mouse )
    {
	gpm_visiblepointer = 1;
        Gpm_DrawPointer( s_ioBase->mLastEvt.col, s_ioBase->mLastEvt.row, gpm_consolefd );
    }
#endif
}

/* *********************************************************************** */

void HB_GT_FUNC(mouse_Hide( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_mouse_Hide()"));

#ifdef HAVE_GPM_H
    if( s_ioBase->terminal_type == TERM_LINUX && s_ioBase->is_mouse )
    {
        gpm_visiblepointer = 0;
    }
#endif
}

/* *********************************************************************** */

int HB_GT_FUNC(mouse_Col( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_mouse_Col()"));

    return s_ioBase->mLastEvt.col;
}

/* *********************************************************************** */

int HB_GT_FUNC(mouse_Row( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_mouse_Row()"));

    return s_ioBase->mLastEvt.row;
}

/* *********************************************************************** */

void HB_GT_FUNC(mouse_SetPos( int iRow, int iCol ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_mouse_SetPos(%i, %i)", iRow, iCol));

    /* it does really nothing */
    s_ioBase->mLastEvt.col = iCol;
    s_ioBase->mLastEvt.row = iRow;
#ifdef HAVE_GPM_H
    if( s_ioBase->terminal_type == TERM_LINUX && s_ioBase->is_mouse )
        if( gpm_visiblepointer )
            Gpm_DrawPointer( iCol, iRow, gpm_consolefd );
#endif
}

/* *********************************************************************** */

BOOL HB_GT_FUNC(mouse_IsButtonPressed( int iButton ))
{
    BOOL ret = FALSE;

    HB_TRACE(HB_TR_DEBUG, ("hb_mouse_IsButtonPressed(%i)", iButton));

    if ( s_ioBase->is_mouse )
    {
        int mask;
        mask = (iButton == 1) ? M_BUTTON_LEFT : M_BUTTON_RIGHT;
        ret = (s_ioBase->mLastEvt.buttons & mask) != 0;
    }

    return ret;
}

/* *********************************************************************** */

int HB_GT_FUNC(mouse_CountButton( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_mouse_CountButton()"));

    return( s_ioBase->mButtons );
}

/* *********************************************************************** */

void HB_GT_FUNC(mouse_SetBounds( int iTop, int iLeft, int iBottom, int iRight ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_mouse_SetBounds(%i, %i, %i, %i)", iTop, iLeft, iBottom, iRight));

    HB_SYMBOL_UNUSED( iTop );
    HB_SYMBOL_UNUSED( iLeft );
    HB_SYMBOL_UNUSED( iBottom );
    HB_SYMBOL_UNUSED( iRight );
}

/* *********************************************************************** */

void HB_GT_FUNC(mouse_GetBounds( int * piTop, int * piLeft, int * piBottom, int * piRight ))
{
   HB_TRACE(HB_TR_DEBUG, ("hb_mouse_GetBounds(%p, %p, %p, %p)", piTop, piLeft, piBottom, piRight));

   *piTop = *piLeft = 0;
   *piBottom = HB_GT_FUNC(gt_GetScreenHeight()) - 1;
   *piRight = HB_GT_FUNC(gt_GetScreenWidth()) - 1;
}

/* *********************************************************************** */

int HB_GT_FUNC(gt_ExtendedKeySupport( void ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_ExtendedKeySupport()"));

    return( 0 );
}

/* *********************************************************************** */

int HB_GT_FUNC(gt_ReadKey( HB_inkey_enum eventmask ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_ReadKey(%d)", (int) eventmask));
    
    return wait_key( s_ioBase, -1 );
}

/* *********************************************************************** */

int HB_GT_FUNC(gt_WaitKey( double dTimeOut ))
{
    HB_TRACE(HB_TR_DEBUG, ("hb_gt_ReadKey(%f)", dTimeOut));

    return wait_key( s_ioBase, (int) (dTimeOut * 1000.0) );
}

/* *********************************************************************** */

void HB_GT_FUNC(gt_SetKeyTrans( unsigned char *szSrc, unsigned char *szDst ))
{
    setKeyTrans( s_ioBase, szSrc, szDst );
}

/* *********************************************************************** */

int HB_GT_FUNC(gt_AddKeyMap( int iKey, char *szSequence ))
{
    return addKeyMap( s_ioBase, SET_CLIPKEY(iKey), szSequence );
}

/* *********************************************************************** */

int HB_GT_FUNC(gt_RemoveKeyMap( char *szSequence ))
{
    return removeKeyMap( s_ioBase, szSequence );
}

/* *********************************************************************** */

int HB_GT_FUNC(gt_ESCdelay( int iDelay ))
{
    int iRet = s_ioBase->esc_delay;

    s_ioBase->esc_delay = iDelay;

    return iRet;
}

/* *********************************************************************** */

void HB_GT_FUNC(gt_SetInterruptKey( int iInterupt ))
{
    set_sig_keys( s_ioBase, iInterupt, -1, -1 );
}

/* *********************************************************************** */

void HB_GT_FUNC(gt_SetDebugKey( int iDebug ))
{
    set_sig_keys( s_ioBase, -1, iDebug, -1 );
}

/* *********************************************************************** */

int HB_GT_FUNC(gt_Shft_Pressed())
{
    return ( s_ioBase->key_flag & KEY_CTRLMASK ) != 0 && 
           ( s_ioBase->key_flag & KEY_ALTMASK ) != 0;
}

/* *********************************************************************** */

int HB_GT_FUNC(gt_Ctrl_Pressed())
{
    return ( s_ioBase->key_flag & KEY_CTRLMASK ) != 0;
}

/* *********************************************************************** */

int HB_GT_FUNC(gt_Alt_Pressed())
{
    return ( s_ioBase->key_flag & KEY_ALTMASK ) != 0;
}

/* *********************************************************************** */

int HB_GT_FUNC(gt_Kbd_State())
{
    return s_ioBase->key_flag;
}

/* *********************************************************************** */

void HB_GT_FUNC(gt_SetDispCP( char * pszTermCDP, char * pszHostCDP, BOOL bBox ))
{
#ifndef HB_CDP_SUPPORT_OFF
    if ( !pszHostCDP || !*pszHostCDP )
    {
        pszHostCDP = s_cdpage->id;
        if ( !pszHostCDP )
            pszHostCDP = pszTermCDP;
    }
    if ( !pszTermCDP || !*pszTermCDP )
        pszTermCDP = pszHostCDP;

    if ( pszTermCDP && pszHostCDP && *pszTermCDP && *pszHostCDP )
    {
        PHB_CODEPAGE cdpTerm = hb_cdpFind( pszTermCDP ),
                     cdpHost = hb_cdpFind( pszHostCDP );
        if ( cdpTerm && cdpHost && 
             cdpTerm->nChars && cdpTerm->nChars == cdpHost->nChars )
        {
            char * pszHostLetters = hb_xgrab( cdpHost->nChars * 2 + 1 );
            char * pszTermLetters = hb_xgrab( cdpTerm->nChars * 2 + 1 );

            strncpy( pszHostLetters, cdpHost->CharsUpper, cdpHost->nChars + 1 );
            strncat( pszHostLetters, cdpHost->CharsLower, cdpHost->nChars + 1 );
            strncpy( pszTermLetters, cdpTerm->CharsUpper, cdpTerm->nChars + 1 );
            strncat( pszTermLetters, cdpTerm->CharsLower, cdpTerm->nChars + 1 );

            setDispTrans( s_ioBase, (unsigned char *) pszHostLetters,
                                    (unsigned char *) pszTermLetters,
				    bBox ? 1 : 0 );

            hb_xfree( pszHostLetters );
            hb_xfree( pszTermLetters );
	}
    }
#else
    HB_SYMBOL_UNUSED( pszTermCDP );
    HB_SYMBOL_UNUSED( pszHostCDP );
#endif
}

/* *********************************************************************** */

void HB_GT_FUNC(gt_SetKeyCP( char * pszTermCDP, char * pszHostCDP ))
{
#ifndef HB_CDP_SUPPORT_OFF
    if ( !pszHostCDP || !*pszHostCDP )
    {
        pszHostCDP = s_cdpage->id;
        if ( !pszHostCDP )
            pszHostCDP = pszTermCDP;
    }

    if ( pszTermCDP && pszHostCDP && *pszTermCDP && *pszHostCDP )
    {
        PHB_CODEPAGE cdpTerm = hb_cdpFind( pszTermCDP ),
                     cdpHost = hb_cdpFind( pszHostCDP );
        if ( cdpTerm && cdpHost && cdpTerm != cdpHost &&
             cdpTerm->nChars && cdpTerm->nChars == cdpHost->nChars )
        {
            char * pszHostLetters = hb_xgrab( cdpHost->nChars * 2 + 1 );
            char * pszTermLetters = hb_xgrab( cdpTerm->nChars * 2 + 1 );

            strncpy( pszHostLetters, cdpHost->CharsUpper, cdpHost->nChars + 1 );
            strncat( pszHostLetters, cdpHost->CharsLower, cdpHost->nChars + 1 );
            strncpy( pszTermLetters, cdpTerm->CharsUpper, cdpTerm->nChars + 1 );
            strncat( pszTermLetters, cdpTerm->CharsLower, cdpTerm->nChars + 1 );

            setKeyTrans( s_ioBase, (unsigned char *) pszTermLetters,
                                   (unsigned char *) pszHostLetters );

            hb_xfree( pszHostLetters );
            hb_xfree( pszTermLetters );
	}
    }
#else
    HB_SYMBOL_UNUSED( pszTermCDP );
    HB_SYMBOL_UNUSED( pszHostCDP );
#endif
}

/* *********************************************************************** */

int HB_GT_FUNC(gt_NewXTerm( void ))
{
    InOutBase *ioBase;
    int iHandle = -1;

    ioBase = create_newXterm();
    if ( ioBase )
    {
        set_sig_keys( ioBase, 'C'-64, 'D'-64, 'Z'-64 );
        iHandle = add_new_ioBase( ioBase );
    }
    return iHandle;
}

/* *********************************************************************** */

int HB_GT_FUNC(gt_SetTerm( int iHandle ))
{
    return set_active_ioBase( iHandle );
}

/* *********************************************************************** */

int HB_GT_FUNC(gt_CloseTerm( int iHandle ))
{
    return del_ioBase( iHandle );
}

/* *********************************************************************** */

BOOL HB_GT_FUNC(gt_AddEventHandle( int iFile, int iMode,
                           int (*eventFunc) (int, int, void *), void *data ))
{
    return add_efds( s_ioBase, iFile, iMode, eventFunc, data ) == iFile;
}

/* *********************************************************************** */

void HB_GT_FUNC(gt_DelEventHandle( int iFileDes ))
{
    del_efds( s_ioBase, iFileDes );
}

/* *********************************************************************** */

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
    /* extended GT functions */
    gt_funcs->SetDispCP             = HB_GT_FUNC( gt_SetDispCP );
    gt_funcs->SetKeyCP              = HB_GT_FUNC( gt_SetKeyCP );
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

HB_CALL_ON_STARTUP_BEGIN( HB_GT_FUNC(_gt_Init_) )
   hb_gtRegister( &gtInit );
HB_CALL_ON_STARTUP_END( HB_GT_FUNC(_gt_Init_) )
#if ! defined(__GNUC__) && ! defined(_MSC_VER)
   #pragma startup HB_GT_FUNC(_gt_Init_)
#endif

#endif  /* HB_MULTI_GT */

/* *********************************************************************** */
