/*
 * $Id: hvm.c,v 1.143 2002/12/30 21:24:09 ronpinkas Exp $
 */

/*
 * Harbour Project source code:
 * The Virtual Machine
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
 * Copyright 1999-2001 Viktor Szakats <viktor.szakats@syenar.hu>
 *    hb_vmPushLongConst()
 *    hb_vmPushDoubleConst()
 *
 * Copyright 1999 Eddie Runia <eddie@runia.com>
 *    __VMVARSGET()
 *    __VMVARSLIST()
 *
 * See doc/license.txt for licensing terms.
 *
 */

#ifndef __MPW__
   #ifdef HB_OS_DARWIN
      #include <stdlib.h>     /* There is no malloc.h in Darwin */
   #else
      #include <malloc.h>
   #endif
#endif

#include <math.h>
#include <time.h>
#include <ctype.h>

#include "hbapi.h"
#include "hbfast.h"
#include "hbstack.h"
#include "hbapierr.h"
#include "hbapiitm.h"
#include "hbapilng.h"
#include "hbapirdd.h"
#include "hbapigt.h"
#include "hbvm.h"
#include "hbpcode.h"
#include "hbset.h"
#include "hbinkey.ch"
#include "inkey.ch"

#ifdef HB_MACRO_STATEMENTS
   #include "hbpp.h"
#endif

#ifdef HB_THREAD_SUPPORT
   //extern HB_FORBID_MUTEX hb_gcCollectionForbid;
   extern HB_CRITICAL_T hb_gcCollectionMutex;
#endif

/* DEBUG only*/
/*#include <windows.h>*/

typedef struct _SYMBOLS
{
   PHB_SYMB pModuleSymbols;  /* pointer to a one module own symbol table */
   USHORT   uiModuleSymbols; /* number of symbols on that table */
   struct _SYMBOLS * pNext;  /* pointer to the next SYMBOLS structure */
   HB_SYMBOLSCOPE hScope;    /* scope collected from all symbols in module used to speed initialization code */
} SYMBOLS, * PSYMBOLS;       /* structure to keep track of all modules symbol tables */

extern HB_FUNC( SYSINIT );

/* PCode functions */

/* Operators (mathematical / character / misc) */
static void    hb_vmNegate( void );          /* negates (-) the latest value on the stack */
static void    hb_vmPlus( void );            /* sums the latest two values on the stack, removes them and leaves the result */
static void    hb_vmMinus( void );           /* substracts the latest two values on the stack, removes them and leaves the result */
static void    hb_vmMult( void );            /* multiplies the latest two values on the stack, removes them and leaves the result */
static void    hb_vmDivide( void );          /* divides the latest two values on the stack, removes them and leaves the result */
static void    hb_vmModulus( void );         /* calculates the modulus of latest two values on the stack, removes them and leaves the result */
static void    hb_vmPower( void );           /* power the latest two values on the stack, removes them and leaves the result */
static void    hb_vmInc( void );             /* increment the latest numeric value on the stack */
static void    hb_vmDec( void );             /* decrements the latest numeric value on the stack */
static void    hb_vmFuncPtr( void );         /* pushes a function address pointer. Removes the symbol from the satck */

/* Operators (relational) */
static void    hb_vmEqual( BOOL bExact );    /* checks if the two latest values on the stack are equal, removes both and leaves result */
static void    hb_vmNotEqual( void );        /* checks if the two latest values on the stack are not equal, removes both and leaves result */
static void    hb_vmLess( void );            /* checks if the latest - 1 value is less than the latest, removes both and leaves result */
static void    hb_vmLessEqual( void );       /* checks if the latest - 1 value is less than or equal the latest, removes both and leaves result */
static void    hb_vmGreater( void );         /* checks if the latest - 1 value is greater than the latest, removes both and leaves result */
static void    hb_vmGreaterEqual( void );    /* checks if the latest - 1 value is greater than or equal the latest, removes both and leaves result */
static void    hb_vmInstringOrArray( void ); /* check whether string 1 is contained in string 2 */
static void    hb_vmForTest( void );         /* test for end condition of for */

/* Operators (logical) */
static void    hb_vmNot( void );             /* changes the latest logical value on the stack */
static void    hb_vmAnd( void );             /* performs the logical AND on the latest two values, removes them and leaves result on the stack */
static void    hb_vmOr( void );              /* performs the logical OR on the latest two values, removes them and leaves result on the stack */

/* Array */
static void    hb_vmArrayPush( void );       /* pushes an array element to the stack, removing the array and the index from the stack */
static void    hb_vmArrayPop( void );        /* pops a value from the stack */
static void    hb_vmArrayDim( USHORT uiDimensions ); /* generates an uiDimensions Array and initialize those dimensions from the stack values */
static void    hb_vmArrayGen( ULONG ulElements ); /* generates an ulElements Array and fills it from the stack values */
static void    hb_vmArrayNew( HB_ITEM_PTR, USHORT ); /* creates array */

/* Object */
static void    hb_vmOperatorCall( PHB_ITEM, PHB_ITEM, char * ); /* call an overloaded operator */
static void    hb_vmOperatorCallUnary( PHB_ITEM, char * ); /* call an overloaded unary operator */

/* Database */
static ERRCODE hb_vmSelectWorkarea( PHB_ITEM );  /* select the workarea using a given item or a substituted value */
static void    hb_vmSwapAlias( void );           /* swaps items on the eval stack and pops the workarea number */

/* Execution */
static HARBOUR hb_vmDoBlock( void );             /* executes a codeblock */
static void    hb_vmLocalName( USHORT uiLocal, char * szLocalName ); /* locals and parameters index and name information for the debugger */
static void    hb_vmStaticName( BYTE bIsGlobal, USHORT uiStatic, char * szStaticName ); /* statics vars information for the debugger */
static void    hb_vmModuleName( char * szModuleName ); /* PRG and function name information for the debugger */
static void    hb_vmFrame( BYTE bLocals, BYTE bParams ); /* increases the stack pointer for the amount of locals and params suplied */
static void    hb_vmSFrame( PHB_SYMB pSym );     /* sets the statics frame for a function */
static void    hb_vmStatics( PHB_SYMB pSym, USHORT uiStatics ); /* increases the global statics array to hold a PRG statics */
static void    hb_vmEndBlock( void );            /* copies the last codeblock pushed value into the return value */
static void    hb_vmRetValue( void );            /* pops the latest stack value into stack.Return */
static void    hb_vmDebuggerShowLine( USHORT uiLine ); /* makes the debugger shows a specific source code line */
static void    hb_vmDebuggerEndProc( void );     /* notifies the debugger for an endproc */

/* Push */
static void    hb_vmPushAlias( void );            /* pushes the current workarea number */
static void    hb_vmPushAliasedField( PHB_SYMB ); /* pushes an aliased field on the eval stack */
static void    hb_vmPushAliasedVar( PHB_SYMB );   /* pushes an aliased variable on the eval stack */
static void    hb_vmPushBlock( BYTE * pCode, PHB_SYMB pSymbols, PHB_ITEM** pGlobals ); /* creates a codeblock */
static void    hb_vmPushBlockShort( BYTE * pCode, PHB_SYMB pSymbols, PHB_ITEM** pGlobals ); /* creates a codeblock */
static void    hb_vmPushDoubleConst( double dNumber, int iWidth, int iDec ); /* Pushes a double constant (pcode) */
static void    hb_vmPushMacroBlock( BYTE * pCode, PHB_SYMB pSymbols ); /* creates a macro-compiled codeblock */
static void    hb_vmPushLocal( SHORT iLocal );    /* pushes the containts of a local onto the stack */
static void    hb_vmPushLocalByRef( SHORT iLocal );    /* pushes a local by refrence onto the stack */
static void    hb_vmPushLongConst( long lNumber );  /* Pushes a long constant (pcode) */
extern void    hb_vmPushNumType( double dNumber, int iDec, int iType1, int iType2 ); /* pushes a number on to the stack and decides if it is integer, long or double */
static void    hb_vmPushStatic( USHORT uiStatic );     /* pushes the containts of a static onto the stack */
static void    hb_vmPushStaticByRef( USHORT uiStatic ); /* pushes a static by refrence onto the stack */
static void    hb_vmPushVariable( PHB_SYMB pVarSymb ); /* pushes undeclared variable */
static void    hb_vmDuplicate( void );            /* duplicates the latest value on the stack */
static void    hb_vmDuplTwo( void );              /* duplicates the latest two value on the stack */

/* Pop */
static BOOL    hb_vmPopLogical( void );           /* pops the stack latest value and returns its logical value */
static double  hb_vmPopNumber( void );            /* pops the stack latest value and returns its numeric value */
static double  hb_vmPopDouble( int * );           /* pops the stack latest value and returns its double numeric format value */
static void    hb_vmPopAlias( void );             /* pops the workarea number form the eval stack */
static void    hb_vmPopAliasedField( PHB_SYMB );  /* pops an aliased field from the eval stack*/
static void    hb_vmPopAliasedVar( PHB_SYMB );    /* pops an aliased variable from the eval stack*/
static void    hb_vmPopLocal( SHORT iLocal );     /* pops the stack latest value onto a local */
static void    hb_vmPopStatic( USHORT uiStatic ); /* pops the stack latest value onto a static */

/* misc */
static void    hb_vmDoInitStatics( void );        /* executes all _INITSTATICS functions */
static void    hb_vmDoInitFunctions( void );      /* executes all defined PRGs INIT functions */
static void    hb_vmDoExitFunctions( void );      /* executes all defined PRGs EXIT functions */
static void    hb_vmReleaseLocalSymbols( void );  /* releases the memory of the local symbols linked list */

extern void * hb_mthRequested( void ); /* profiler from classes.c */
extern void hb_mthAddTime( void *, ULONG ); /* profiler from classes.c */

BOOL hb_bProfiler = FALSE; /* profiler status is off */
BOOL hb_bTracePrgCalls = FALSE; /* prg tracing is off */
ULONG hb_ulOpcodesCalls[ HB_P_LAST_PCODE ]; /* array to profile opcodes calls */
ULONG hb_ulOpcodesTime[ HB_P_LAST_PCODE ]; /* array to profile opcodes consumed time */

/* virtual machine state */

HB_SYMB  hb_symEval = { "__EVAL", HB_FS_PUBLIC, hb_vmDoBlock, NULL }; /* symbol to evaluate codeblocks */

static HB_ITEM  s_aStatics;         /* Harbour array to hold all application statics variables */
static USHORT   s_uiStatics;        /* Number of statics added after processing hb_vmStatics() */
static PHB_SYMB s_pSymStart = NULL; /* start symbol of the application. MAIN() is not required */
static PSYMBOLS s_pSymbols = NULL;  /* to hold a linked list of all different modules symbol tables */
static BYTE     s_byErrorLevel;     /* application exit errorlevel */

static BOOL     s_bDebugging;
static BOOL     s_bDebugRequest;    /* debugger invoked via the VM */
static BOOL     s_bDebugShowLines;  /* update source code line on the debugger display */
static BOOL     s_bDebuggerIsWorking; /* to know when __DBGENTRY is beeing invoked */

/* Stores the position on the stack of current SEQUENCE envelope or 0 if no
 * SEQUENCE is active
 */
static LONG     s_lRecoverBase;
#define  HB_RECOVER_STATE     -1
#define  HB_RECOVER_BASE      -2
#define  HB_RECOVER_ADDRESS   -3
#define  HB_RECOVER_VALUE     -4

int hb_vm_aiExtraParams[HB_MAX_MACRO_ARGS], hb_vm_iExtraParamsIndex = 0;
PHB_SYMB hb_vm_apExtraParamsSymbol[HB_MAX_MACRO_ARGS];

int hb_vm_aiExtraElements[HB_MAX_MACRO_ARGS], hb_vm_iExtraElementsIndex = 0, hb_vm_iExtraElements = 0;

int hb_vm_iExtraIndex;

/* Request for some action - stop processing of opcodes
 */
static USHORT   s_uiActionRequest;

static int s_iBaseLine;

char *hb_vm_sNull = "";

char *hb_vm_acAscii[256] = { "\x00", "\x01", "\x02", "\x03", "\x04", "\x05", "\x06", "\x07", "\x08", "\x09", "\x0A", "\x0B", "\x0C", "\x0D", "\x0E", "\x0F",
                             "\x10", "\x11", "\x12", "\x13", "\x14", "\x15", "\x16", "\x17", "\x18", "\x19", "\x1A", "\x1B", "\x1C", "\x1D", "\x1E", "\x1F",
                             "\x20", "\x21", "\x22", "\x23", "\x24", "\x25", "\x26", "\x27", "\x28", "\x29", "\x2A", "\x2B", "\x2C", "\x2D", "\x2E", "\x2F",
                             "\x30", "\x31", "\x32", "\x33", "\x34", "\x35", "\x36", "\x37", "\x38", "\x39", "\x3A", "\x3B", "\x3C", "\x3D", "\x3E", "\x3F",
                             "\x40", "\x41", "\x42", "\x43", "\x44", "\x45", "\x46", "\x47", "\x48", "\x49", "\x4A", "\x4B", "\x4C", "\x4D", "\x4E", "\x4F",
                             "\x50", "\x51", "\x52", "\x53", "\x54", "\x55", "\x56", "\x57", "\x58", "\x59", "\x5A", "\x5B", "\x5C", "\x5D", "\x5E", "\x5F",
                             "\x60", "\x61", "\x62", "\x63", "\x64", "\x65", "\x66", "\x67", "\x68", "\x69", "\x6A", "\x6B", "\x6C", "\x6D", "\x6E", "\x6F",
                             "\x70", "\x71", "\x72", "\x73", "\x74", "\x75", "\x76", "\x77", "\x78", "\x79", "\x7A", "\x7B", "\x7C", "\x7D", "\x7E", "\x7F",
                             "\x80", "\x81", "\x82", "\x83", "\x84", "\x85", "\x86", "\x87", "\x88", "\x89", "\x8A", "\x8B", "\x8C", "\x8D", "\x8E", "\x8F",
                             "\x90", "\x91", "\x92", "\x93", "\x94", "\x95", "\x96", "\x97", "\x98", "\x99", "\x9A", "\x9B", "\x9C", "\x9D", "\x9E", "\x9F",
                             "\xA0", "\xA1", "\xA2", "\xA3", "\xA4", "\xA5", "\xA6", "\xA7", "\xA8", "\xA9", "\xAA", "\xAB", "\xAC", "\xAD", "\xAE", "\xAF",
                             "\xB0", "\xB1", "\xB2", "\xB3", "\xB4", "\xB5", "\xB6", "\xB7", "\xB8", "\xB9", "\xBA", "\xBB", "\xBC", "\xBD", "\xBE", "\xBF",
                             "\xC0", "\xC1", "\xC2", "\xC3", "\xC4", "\xC5", "\xC6", "\xC7", "\xC8", "\xC9", "\xCA", "\xCB", "\xCC", "\xCD", "\xCE", "\xCF",
                             "\xD0", "\xD1", "\xD2", "\xD3", "\xD4", "\xD5", "\xD6", "\xD7", "\xD8", "\xD9", "\xDA", "\xDB", "\xDC", "\xDD", "\xDE", "\xDF",
                             "\xE0", "\xE1", "\xE2", "\xE3", "\xE4", "\xE5", "\xE6", "\xE7", "\xE8", "\xE9", "\xEA", "\xEB", "\xEC", "\xED", "\xEE", "\xEF",
                             "\xF0", "\xF1", "\xF2", "\xF3", "\xF4", "\xF5", "\xF6", "\xF7", "\xF8", "\xF9", "\xFA", "\xFB", "\xFC", "\xFD", "\xFE", "\xFF" };


HB_ITEM hb_vm_aWithObject[ HB_MAX_WITH_OBJECTS ];
USHORT  hb_vm_wWithObjectCounter; // Initilaized in hb_vmInit()
BOOL    hb_vm_bWithObject = FALSE;

HB_ITEM  hb_vm_aEnumCollection[ HB_MAX_ENUMERATIONS ];
PHB_ITEM hb_vm_apEnumVar[ HB_MAX_ENUMERATIONS ];
ULONG    hb_vm_awEnumIndex[ HB_MAX_ENUMERATIONS ];
USHORT   hb_vm_wEnumCollectionCounter = 0; // Initilaized in hb_vmInit()

static   HB_ITEM  s_aGlobals;         /* Harbour array to hold all application global variables */

static BOOL s_fmInit = TRUE;

/* 21/10/00 - maurilio.longo@libero.it
   This Exception Handler gets called in case of an abnormal termination of an harbour program and
   displays a full stack trace at the harbour language level */
#if defined(HB_OS_OS2)
ULONG _System OS2TermHandler(PEXCEPTIONREPORTRECORD       p1,
                             PEXCEPTIONREGISTRATIONRECORD p2,
                             PCONTEXTRECORD               p3,
                             PVOID                        pv);
#endif

// Initialize ErrorBlock() and __SetHelpK()
void hb_vmDoInitClip( void )
{
   PHB_DYNS pDynSym = hb_dynsymFind( "CLIPINIT" );

   if( pDynSym && pDynSym->pSymbol->pFunPtr )
   {
      hb_vmPushSymbol( pDynSym->pSymbol );
      hb_vmPushNil();
      hb_vmDo(0);
   }
}

// Initialize DBFCDX and DBFNTX if linked.
void hb_vmDoInitRdd( void )
{
   PHB_DYNS pDynSym = hb_dynsymFind( "DBFNTXINIT" );

   if( pDynSym && pDynSym->pSymbol->pFunPtr )
   {
      hb_vmPushSymbol( pDynSym->pSymbol );
      hb_vmPushNil();
      hb_vmDo(0);
   }

   pDynSym = hb_dynsymFind( "DBFCDXINIT" );

   if( pDynSym && pDynSym->pSymbol->pFunPtr )
   {
      hb_vmPushSymbol( pDynSym->pSymbol );
      hb_vmPushNil();
      hb_vmDo(0);
   }
}

/* application entry point */
void HB_EXPORT hb_vmInit( BOOL bStartMainProc )
{
   FILE *fpTrace;

   #if defined(HB_OS_OS2)
      EXCEPTIONREGISTRATIONRECORD RegRec = {0};       /* Exception Registration Record */
      APIRET rc = NO_ERROR;                           /* Return code                   */
   #endif

   HB_TRACE(HB_TR_DEBUG, ("hb_vmInit()"));

    #ifdef HB_THREAD_SUPPORT
       HB_TRACE( HB_TR_INFO, ("contextInit" ) );
       hb_threadInit();
    #endif

    hb_gcInit();

   /* initialize internal data structures */
   s_aStatics.type = HB_IT_NIL;
   s_byErrorLevel = 0;
   s_bDebugging = FALSE;
   s_bDebugShowLines = FALSE;
   s_bDebuggerIsWorking = FALSE;
   s_lRecoverBase = 0;
   s_uiActionRequest = 0;

   HB_VM_STACK.pItems = NULL; /* keep this here as it is used by fm.c */
   HB_VM_STACK.Return.type = HB_IT_NIL;

   for ( hb_vm_wWithObjectCounter = 0; hb_vm_wWithObjectCounter < HB_MAX_WITH_OBJECTS; hb_vm_wWithObjectCounter++  )
   {
      hb_vm_aWithObject[ hb_vm_wWithObjectCounter ].type = HB_IT_NIL;
   }
   hb_vm_wWithObjectCounter = 0;

   for ( hb_vm_wEnumCollectionCounter = 0; hb_vm_wEnumCollectionCounter < HB_MAX_ENUMERATIONS; hb_vm_wEnumCollectionCounter++ )
   {
      hb_vm_aEnumCollection[ hb_vm_wEnumCollectionCounter ].type = HB_IT_NIL;
      hb_vm_awEnumIndex[ hb_vm_wEnumCollectionCounter ] = 0;
   }
   hb_vm_wEnumCollectionCounter = 0;

   s_aGlobals.type = HB_IT_NIL;
   hb_arrayNew( &s_aGlobals, 0 );

   /* Moved to respective main!
   HB_TRACE( HB_TR_INFO, ("xinit" ) );
   hb_xinit();
   */

   HB_TRACE( HB_TR_INFO, ("errInit" ) );
   hb_errInit();
   HB_TRACE( HB_TR_INFO, ("stackInit" ) );
   hb_stackInit();
   HB_TRACE( HB_TR_INFO, ("dynsymNew" ) );
   hb_dynsymNew( &hb_symEval );  /* initialize dynamic symbol for evaluating codeblocks */
   HB_TRACE( HB_TR_INFO, ("setInitialize" ) );
   hb_setInitialize();        /* initialize Sets */
   HB_TRACE( HB_TR_INFO, ("conInit" ) );
   hb_conInit();    /* initialize Console */
   HB_TRACE( HB_TR_INFO, ("memvarsInit" ) );
   hb_memvarsInit();
   HB_TRACE( HB_TR_INFO, ("SymbolInit_RT" ) );
   hb_vmSymbolInit_RT();      /* initialize symbol table with runtime support functions */

   /* Set the language to the default */

   /* This trick is needed to stringify the macro value */
   #define HB_LANG_SELECT_DEFAULT( id ) HB_LANG_SELECT_DEFAULT_( id )
   #define HB_LANG_SELECT_DEFAULT_( id ) hb_langSelectID( #id )
   HB_LANG_SELECT_DEFAULT( HB_LANG_DEFAULT );

   /* Check for some internal switches */
   hb_cmdargProcessVM();

   /* Initialize opcodes profiler support arrays */
   {
      ULONG ul;

      for( ul = 0; ul < HB_P_LAST_PCODE; ul++ )
      {
         hb_ulOpcodesCalls[ ul ] = 0;
         hb_ulOpcodesTime[ ul ] = 0;
      }
   }

   /* Create trace.log for tracing. */
   fpTrace = fopen( "trace.log", "w" );
   if( fpTrace )
   {
      fclose( fpTrace );
   }
   else
   {
      //hb_errInternal( HB_EI_ERRUNRECOV, "Unable to create trace.log file", NULL, NULL );
   }

   /* Call functions that initializes static variables
    * Static variables have to be initialized before any INIT functions
    * because INIT function can use static variables.
    */
   hb_vmDoInitStatics();

   hb_vmDoInitClip(); // Initialize ErrorBlock() and __SetHelpK()
   hb_vmDoInitRdd();  // Initialize DBFCDX and DBFNTX if linked.
   hb_vmDoInitFunctions(); /* process defined INIT functions */

   /* This is undocumented CA-Clipper, if there's a function called _APPMAIN
      it will be executed first. [vszakats] */
   {
      PHB_DYNS pDynSym = hb_dynsymFind( "_APPMAIN" );

      if( pDynSym && pDynSym->pSymbol->pFunPtr )
         s_pSymStart = pDynSym->pSymbol;
#ifdef HARBOUR_START_PROCEDURE
      else
      {
         pDynSym = hb_dynsymFind( HARBOUR_START_PROCEDURE );

         if( pDynSym && pDynSym->pSymbol->pFunPtr )
            s_pSymStart = pDynSym->pSymbol;
         else
            hb_errInternal( HB_EI_VMBADSTARTUP, NULL, HARBOUR_START_PROCEDURE, NULL );
      }
#else
#ifndef HB_C52_STRICT
      else if( bStartMainProc && ! s_pSymStart )
         hb_errInternal( HB_EI_VMNOSTARTUP, NULL, NULL, NULL );
#endif
#endif
   }

#if defined(HB_OS_OS2) /* Add OS2TermHandler to this thread's chain of exception handlers */

   RegRec.ExceptionHandler = (ERR)OS2TermHandler;
   rc = DosSetExceptionHandler( &RegRec );
   if (rc != NO_ERROR) {
      hb_errInternal( HB_EI_ERRUNRECOV, "Unable to setup exception handler (DosSetExceptionHandler())", NULL, NULL );
   }
#endif

   if( bStartMainProc && s_pSymStart )
   {
      int i;
      int iArgCount;

      hb_vmPushSymbol( s_pSymStart ); /* pushes first HB_FS_PUBLIC defined symbol to the stack */
      hb_vmPushNil();                 /* places NIL at self */

      iArgCount = 0;
      for( i = 1; i < hb_cmdargARGC(); i++ )     /* places application parameters on the stack */
      {
         char ** argv = hb_cmdargARGV();

         /* Filter out any parameters beginning with //, like //INFO */
         if( ! hb_cmdargIsInternal( argv[ i ] ) )
         {
            hb_vmPushString( argv[ i ], strlen( argv[ i ] ) );
            iArgCount++;
         }
      }

      hb_vmDo( iArgCount ); /* invoke it with number of supplied parameters */
   }

#if defined(HB_OS_OS2)
   /* I don't do any check on return code since harbour is exiting in any case */
   rc = DosUnsetExceptionHandler( &RegRec );
#endif
}

void HB_EXPORT hb_vmQuit( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmQuit()"));

   //printf( "\nvmQuit()\n" );

   #ifdef HB_THREAD_SUPPORT
       hb_threadExit();
   #endif
   //printf( "After Thread\n" );

   #ifdef HB_MACRO_STATEMENTS
     hb_pp_Free();
   #endif

   s_uiActionRequest = 0;         /* EXIT procedures should be processed */
   hb_vmDoExitFunctions();       /* process defined EXIT functions */
   //printf( "After Functions\n" );

   /* release all known items stored in subsystems */
   hb_rddShutDown();
   //printf( "After RDD\n" );

   hb_idleShutDown();
   //printf( "After Idle\n" );

   hb_errExit();
   //printf( "After Err\n" );

   hb_clsReleaseAll();
   //printf( "After Class\n" );

   hb_vmReleaseLocalSymbols();  /* releases the local modules linked list */
   //printf( "After Symbols\n" );

   hb_dynsymRelease();          /* releases the dynamic symbol table */
   //printf( "After Dyn\n" );

   hb_conRelease();             /* releases Console */
   //printf( "After Con\n" );

   hb_setRelease();             /* releases Sets */
   //printf( "After Set\n" );

   /* release all remaining items */
   hb_stackRemove( 0 );
   //printf( "After Stack\n" );

   if( HB_IS_COMPLEX( &(HB_VM_STACK.Return) ) )
   {
      hb_itemClear( &(hb_stack.Return) );
   }
   //printf( "After Return\n" );

   if( s_aStatics.type == HB_IT_ARRAY )
   {
      hb_arrayRelease( &s_aStatics );
   }
   //printf( "\nAfter Statics\n" );

   if( s_aGlobals.type == HB_IT_ARRAY )
   {
      hb_arrayFill( &s_aGlobals, ( *HB_VM_STACK.pPos ), 1, s_aGlobals.item.asArray.value->ulLen );
      hb_arrayRelease( &s_aGlobals );
   }
   //printf( "\nAfter Globals\n" );

   hb_memvarsRelease();    /* clear all PUBLIC variables */
   //printf( "After Memvar\n" );

   /* release all known garbage */
   //hb_gcCollectAll();
   hb_gcReleaseAll();
   //printf( "After GC\n" );

   hb_memvarsFree();    /* free memory allocated for memvars table */
   //printf( "After memvarsFree\n" );

   hb_stackFree();
   //printf( "After stackFree\n" );

   /* hb_dynsymLog(); */

   hb_xexit();
   //printf( "After xexit\n" );

   hb_gcExit();

   exit( s_byErrorLevel );
}

void HB_EXPORT hb_vmExecute( const BYTE * pCode, PHB_SYMB pSymbols, PHB_ITEM **pGlobals )
{
   LONG w = 0;
   BOOL bCanRecover = FALSE;
   ULONG ulPrivateBase;
   LONG lOffset;
   ULONG ulLastOpcode = 0; /* opcodes profiler support */
   ULONG ulPastClock = 0;  /* opcodes profiler support */

   USHORT wEnumCollectionCounter = hb_vm_wEnumCollectionCounter;
   USHORT wWithObjectCounter = hb_vm_wWithObjectCounter;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmExecute(%p, %p, %p)", pCode, pSymbols, pGlobals));

   /* NOTE: if pSymbols == NULL then hb_vmExecute is called from macro
    * evaluation. In this case all PRIVATE variables created during
    * macro evaluation belong to a function/procedure where macro
    * compiler was called.
    */

   /* NOTE: Initialization with 0 is needed to avoid GCC -O2 warning */
   ulPrivateBase = pSymbols ? hb_memvarGetPrivatesBase() : 0;

   while( pCode[ w ] != HB_P_ENDPROC )
   {

      if( hb_bProfiler )
      {
         ULONG ulActualClock = ( ULONG ) clock();

         hb_ulOpcodesTime[ ulLastOpcode ] += ( ulActualClock - ulPastClock );
         ulPastClock = ulActualClock;
         ulLastOpcode = pCode[ w ];
         hb_ulOpcodesCalls[ ulLastOpcode ]++;
      }

      if( hb_set.HB_SET_CANCEL )
      {
         static unsigned short s_iCancel = 0;

         if( ++s_iCancel == 65535 )
         {
            int ch = hb_gt_ReadKey( hb_set.HB_SET_EVENTMASK );

            switch( ch )
            {
               case HB_K_ALT_C:           /* Check for extended Alt+C */
               case K_ALT_C:              /* Check for normal Alt+C */
                  hb_vmRequestCancel();/* Request cancellation */
            }
         }
      }

      switch( pCode[ w ] )
      {
         /* Operators ( mathematical / character / misc ) */

         case HB_P_NEGATE:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_NEGATE") );
            hb_vmNegate();
            w++;
            break;

         case HB_P_PLUS:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PLUS") );
            hb_vmPlus();
            w++;
            break;

         case HB_P_MINUS:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_MINUS") );
            hb_vmMinus();
            w++;
            break;

         case HB_P_MULT:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_MULT") );
            hb_vmMult();
            w++;
            break;

         case HB_P_DIVIDE:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_DIVIDE") );
            hb_vmDivide();
            w++;
            break;

         case HB_P_MODULUS:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_MODULUS") );
            hb_vmModulus();
            w++;
            break;

         case HB_P_POWER:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_POWER") );
            hb_vmPower();
            w++;
            break;

         case HB_P_INC:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_INC") );
            hb_vmInc();
            w++;
            break;

         case HB_P_DEC:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_DEC") );
            hb_vmDec();
            w++;
            break;

         case HB_P_FUNCPTR:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_FUNCPTR") );
            hb_vmFuncPtr();
            w++;
            break;

         /* Operators (relational) */

         case HB_P_EQUAL:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_EQUAL") );
            hb_vmEqual( FALSE );
            w++;
            break;

         case HB_P_EXACTLYEQUAL:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_EXACTLTYEQUAL") );
            hb_vmEqual( TRUE );
            w++;
            break;

         case HB_P_NOTEQUAL:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_NOTEQUAL") );
            hb_vmNotEqual();
            w++;
            break;

         case HB_P_LESS:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_LESS") );
            hb_vmLess();
            w++;
            break;

         case HB_P_LESSEQUAL:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_LESSEQUAL") );
            hb_vmLessEqual();
            w++;
            break;

         case HB_P_GREATER:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_GREATER") );
            hb_vmGreater();
            w++;
            break;

         case HB_P_GREATEREQUAL:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_GREATEREQUAL") );
            hb_vmGreaterEqual();
            w++;
            break;

            // Also used for Arrays!!!
         case HB_P_INSTRING:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_INSTRING") );
            hb_vmInstringOrArray();
            w++;
            break;

         case HB_P_FORTEST:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_FORTEST") );
            hb_vmForTest();
            w++;
            break;

         /* Operators (logical) */

         case HB_P_NOT:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_NOT") );
            hb_vmNot();
            w++;
            break;

         case HB_P_AND:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_AND") );
            hb_vmAnd();
            w++;
            break;

         case HB_P_OR:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_OR") );
            hb_vmOr();
            w++;
            break;

         /* Array */

         case HB_P_ARRAYPUSH:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_ARRAYPUSH") );
            hb_vmArrayPush();
            w++;
            break;

         case HB_P_ARRAYPOP:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_ARRAYPOP") );
            hb_vmArrayPop();
            w++;
            break;

         case HB_P_ARRAYDIM:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_ARRAYDIM") );
            hb_vmArrayDim( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) );
            w += 3;
            break;

         case HB_P_ARRAYGEN:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_ARRAYGEN %i + %i", pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ), hb_vm_iExtraElements ) );
            hb_vmArrayGen( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) + hb_vm_iExtraElements );
            hb_vm_iExtraElements = 0;
            w += 3;
            break;

         /* Object */

         case HB_P_MESSAGE:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_MESSAGE") );
            hb_vmPushSymbol( pSymbols + ( USHORT ) ( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) ) );
            w += 3;
            break;

         /* Database */

         case HB_P_SWAPALIAS:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_SWAPALIAS") );
            hb_vmSwapAlias();
            w++;
            break;

         /* Execution */

         case HB_P_DO:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_DO") );
            hb_vmDo( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) );
            w += 3;
            break;

         case HB_P_DOSHORT:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_DOSHORT") );
            hb_vmDo( pCode[ w + 1 ] );
            w += 2;
            break;

         case HB_P_FUNCTION:
         {
            USHORT uiParams = pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 );

            HB_TRACE( HB_TR_DEBUG, ("HB_P_FUNCTION") );

            //hb_vmFunction( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) );

            if( HB_IS_COMPLEX( &(HB_VM_STACK.Return) ) )
            {
               hb_itemClear( &(HB_VM_STACK.Return) );
            }
            else
            {
               ( &(HB_VM_STACK.Return) )->type = HB_IT_NIL;
            }

            hb_vmDo( uiParams );

            hb_itemForwardValue( ( * HB_VM_STACK.pPos ), &(HB_VM_STACK.Return) );
            hb_stackPush();

            w += 3;
            break;
         }

         case HB_P_FUNCTIONSHORT:
         {
            USHORT uiParams = pCode[ w + 1 ];

            HB_TRACE( HB_TR_DEBUG, ("HB_P_FUNCTIONSHORT") );

            //hb_vmFunction( pCode[ w + 1 ] );

            if( HB_IS_COMPLEX( &(HB_VM_STACK.Return) ) )
            {
               hb_itemClear( &(HB_VM_STACK.Return) );
            }
            else
            {
               ( &(HB_VM_STACK.Return) )->type = HB_IT_NIL;
            }

            hb_vmDo( uiParams );

            hb_itemForwardValue( ( * HB_VM_STACK.pPos ), &(HB_VM_STACK.Return) );
            hb_stackPush();

            w += 2;
            break;
         }

         case HB_P_WITHOBJECT:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_WITHOBJECT") );
            hb_itemForwardValue( &( hb_vm_aWithObject[ hb_vm_wWithObjectCounter++ ] ), hb_stackItemFromTop( -1 ) );
            hb_stackPop();
            w++;
            break;

         case HB_P_ENDWITHOBJECT:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_ENDWITHOBJECT") );
            hb_itemClear( &( hb_vm_aWithObject[ --hb_vm_wWithObjectCounter ] ) );
            w++;
            break;

         case HB_P_FOREACH:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_FOREACH") );
            hb_itemForwardValue( &( hb_vm_aEnumCollection[ hb_vm_wEnumCollectionCounter ] ), hb_stackItemFromTop( -1 ) );
            hb_stackPop();

            if( hb_vm_aEnumCollection[ hb_vm_wEnumCollectionCounter ].type != HB_IT_ARRAY && hb_vm_aEnumCollection[ hb_vm_wEnumCollectionCounter ].type != HB_IT_STRING )
            {
               hb_errRT_BASE( EG_ARG, 1068, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 2, &( hb_vm_aEnumCollection[ hb_vm_wEnumCollectionCounter ] ), hb_itemPutNI( * HB_VM_STACK.pPos, 1 ) );
            }

            hb_vm_apEnumVar[ hb_vm_wEnumCollectionCounter ] = hb_itemUnRef( hb_stackItemFromTop( -1 ) );
            hb_stackPop();
            hb_vm_wEnumCollectionCounter++;
            w++;
            break;

         case HB_P_ENUMERATE:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_ENUMERATE") );
            hb_vmPushLogical( hb_arrayGetByRef( &( hb_vm_aEnumCollection[ hb_vm_wEnumCollectionCounter - 1 ] ),
                                          ++hb_vm_awEnumIndex[ hb_vm_wEnumCollectionCounter - 1 ],
                                          hb_vm_apEnumVar[ hb_vm_wEnumCollectionCounter - 1 ] ) );

            w++;
            break;

         case HB_P_ENDENUMERATE:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_ENDENUMERATE") );
            --hb_vm_wEnumCollectionCounter;

            hb_itemClear( &( hb_vm_aEnumCollection[ hb_vm_wEnumCollectionCounter ] ) );
            hb_vm_awEnumIndex[ hb_vm_wEnumCollectionCounter ] = 0;

            // Incase EXIT was used.
            hb_itemClear( hb_vm_apEnumVar[ hb_vm_wEnumCollectionCounter ] );

            w++;
            break;

         case HB_P_ENUMINDEX:
            ( *HB_VM_STACK.pPos )->type = HB_IT_LONG;
            ( *HB_VM_STACK.pPos )->item.asLong.value = hb_vm_awEnumIndex[ hb_vm_wEnumCollectionCounter - 1 ];
            ( * HB_VM_STACK.pPos )->item.asLong.length = 10;
            hb_stackPush();
            w++;
            break;

         case HB_P_SENDWITH:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_SENDWITH") );
            // Intentionally NOT breaking - fall through!
            hb_vm_bWithObject = TRUE;

         case HB_P_SEND:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_SEND") );

            hb_vmSend( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) );

            w += 3;

            if( pCode[w] == HB_P_POP )
            {
               HB_TRACE( HB_TR_DEBUG, ("skipped HB_P_POP") );
               w++;
            }
            else
            {
               hb_itemForwardValue( ( * HB_VM_STACK.pPos ), &(HB_VM_STACK.Return) );
               hb_stackPush();
            }

            break;

         case HB_P_SENDWITHSHORT:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_SENDWITHSHORT") );
            hb_vm_bWithObject = TRUE;
            // Intentionally NOT breaking - fall through!

         case HB_P_SENDSHORT:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_SENDSHORT") );

            hb_vmSend( pCode[ w + 1 ] );

            w += 2;

            if( pCode[w] == HB_P_POP )
            {
               HB_TRACE( HB_TR_DEBUG, ("skipped HB_P_POP") );
               w++;
            }
            else
            {
               hb_itemForwardValue( ( * HB_VM_STACK.pPos ), &(HB_VM_STACK.Return) );
               hb_stackPush();
            }

            break;

         case HB_P_BASELINE:
            s_iBaseLine = pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 );
            hb_stackBaseItem()->item.asSymbol.lineno = s_iBaseLine;

            //printf( "BASE Proc: %s Line: %i\n", hb_stackBaseItem()->item.asSymbol.value->szName, hb_stackBaseItem()->item.asSymbol.lineno );

            if( s_bDebugging && s_bDebugShowLines )
            {
               hb_vmDebuggerShowLine( s_iBaseLine );
            }

            w += 3;

            HB_TRACE(HB_TR_INFO, ("Opcode: HB_P_LINE: %s (%i)", hb_stackBaseItem()->item.asSymbol.value->szName, hb_stackBaseItem()->item.asSymbol.lineno));

            break;

         case HB_P_LINE:
            hb_stackBaseItem()->item.asSymbol.lineno = pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 );

            //printf( "Proc: %s Line: %i\n", hb_stackBaseItem()->item.asSymbol.value->szName, hb_stackBaseItem()->item.asSymbol.lineno );

            if( s_bDebugging && s_bDebugShowLines )
            {
               hb_vmDebuggerShowLine( hb_stackBaseItem()->item.asSymbol.lineno );
            }

            w += 3;

            HB_TRACE(HB_TR_INFO, ("Opcode: HB_P_LINE: %s (%i)", hb_stackBaseItem()->item.asSymbol.value->szName, hb_stackBaseItem()->item.asSymbol.lineno));

            break;

         case HB_P_LINEOFFSET:
            hb_stackBaseItem()->item.asSymbol.lineno = s_iBaseLine + (unsigned char) pCode[ w + 1 ];

            if( s_bDebugging && s_bDebugShowLines )
            {
               hb_vmDebuggerShowLine( hb_stackBaseItem()->item.asSymbol.lineno );
            }
            w += 2;

            HB_TRACE(HB_TR_INFO, ("Opcode: HB_P_LINEOFFSET: %s (%i)", hb_stackBaseItem()->item.asSymbol.value->szName, hb_stackBaseItem()->item.asSymbol.lineno));

            break;

         case HB_P_PARAMETER:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PARAMETER") );
            hb_memvarNewParameter( pSymbols + ( USHORT ) ( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) ), hb_stackItemFromBase( pCode[ w + 3 ] ) );
            w += 4;
            break;

         case HB_P_FRAME:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_FRAME") );
            hb_vmFrame( pCode[ w + 1 ], pCode[ w + 2 ] );
            w += 3;
            break;

         case HB_P_SFRAME:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_SFRAME") );
            hb_vmSFrame( pSymbols + ( USHORT ) ( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) ) );
            w += 3;
            break;

         case HB_P_STATICS:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_STATICS") );
            hb_vmStatics( pSymbols + ( USHORT ) ( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) ), pCode[ w + 3 ] + ( pCode[ w + 4 ] * 256 ) );
            w += 5;
            break;

         case HB_P_RETVALUE:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_RETVALUE") );
            hb_vmRetValue();
            w++;
            break;

         case HB_P_LOCALNAME:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_LOCALNAME") );
            hb_vmLocalName( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ),
                            ( char * ) pCode + w + 3 );
            w += 3;
            while( pCode[ w++ ] );
            break;

         case HB_P_STATICNAME:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_STATICNAME") );
            hb_vmStaticName( pCode[ w + 1 ], pCode[ w + 2 ] + ( pCode[ w + 3 ] * 256 ),
                            ( char * ) pCode + w + 4 );
            w += 4;
            while( pCode[ w++ ] );
            break;

         case HB_P_MODULENAME:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_MODULENAME") );
            hb_vmModuleName( ( char * ) pCode + w + 1 );
            while( pCode[ w++ ] );
            break;

         case HB_P_ENDBLOCK:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_ENDBLOCK") );
            hb_vmEndBlock();

            /* end of a codeblock - stop evaluation */
            if( pSymbols )
            {
               hb_memvarSetPrivatesBase( ulPrivateBase );
            }

            /*
             * *** NOTE!!! Return!!!
             */

            return;

         /* BEGIN SEQUENCE/RECOVER/END SEQUENCE */

         case HB_P_SEQBEGIN:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_SEQBEGIN") );
            /*
             * Create the SEQUENCE envelope
             * [ break return value      ]  -4
             * [ address of recover code ]  -3
             * [ previous recover base   ]  -2
             * [ current recovery state  ]  -1
             * [                         ] <- new recover base
             */
            /*
             * 1) clear the storage for value returned by BREAK statement
             */
            ( * HB_VM_STACK.pPos )->type = HB_IT_NIL;
            hb_stackPush();
            /*
             * 2) store the address of RECOVER or END opcode
             */
            ( * HB_VM_STACK.pPos )->type = HB_IT_LONG;

            lOffset = pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) + ( pCode[ w + 3 ] * 65536 );
            if( lOffset > 8388607L )
               lOffset = ( lOffset - 16777216 );

            ( * HB_VM_STACK.pPos )->item.asLong.value = w + lOffset;
            hb_stackPush();
            /*
             * 3) store current RECOVER base
             */
            ( * HB_VM_STACK.pPos )->type = HB_IT_LONG;
            ( * HB_VM_STACK.pPos )->item.asLong.value = s_lRecoverBase;
            hb_stackPush();
            /*
             * 4) store current bCanRecover flag - in a case of nested sequences
             * in the same procedure/function
             */
            ( * HB_VM_STACK.pPos )->type = HB_IT_LOGICAL;
            ( * HB_VM_STACK.pPos )->item.asLogical.value = bCanRecover;
            hb_stackPush();
            /*
             * set new recover base
             */
            s_lRecoverBase = hb_stackTopOffset();
            /*
             * we are now inside a valid SEQUENCE envelope
             */
            bCanRecover = TRUE;
            w += 4;
            break;

         case HB_P_SEQEND:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_SEQEND") );
            /*
             * Remove the SEQUENCE envelope
             * This is executed either at the end of sequence or as the
             * response to the break statement if there is no RECOVER clause
             */
            /*
             * 4) Restore previous recovery state
             */
            hb_stackDec();
            bCanRecover = ( * HB_VM_STACK.pPos )->item.asLogical.value;
            ( * HB_VM_STACK.pPos )->type = HB_IT_NIL;
            /*
             * 3) Restore previous RECOVER base
             */
            hb_stackDec();
            s_lRecoverBase = ( * HB_VM_STACK.pPos )->item.asLong.value;
            ( * HB_VM_STACK.pPos )->type = HB_IT_NIL;
            /*
             * 2) Remove RECOVER address
             */
            hb_stackDec();
            ( * HB_VM_STACK.pPos )->type = HB_IT_NIL;
            /* 1) Discard the value returned by BREAK statement - there
             * was no RECOVER clause or there was no BREAK statement
             */
            hb_stackPop();
            /*
             * skip outside of SEQUENCE structure
             */
            lOffset = pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) + ( pCode[ w + 3 ] * 65536 );
            if( lOffset > 8388607L )
               lOffset = ( lOffset - 16777216 );

            w += lOffset;
            break;

         case HB_P_SEQRECOVER:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_SEQRECOVER") );
            /*
             * Execute the RECOVER code
             */
            /*
             * 4) Restore previous recovery state
             */
            hb_stackDec();
            bCanRecover = ( * HB_VM_STACK.pPos )->item.asLogical.value;
            ( * HB_VM_STACK.pPos )->type = HB_IT_NIL;
            /*
             * 3) Restore previous RECOVER base
             */
            hb_stackDec();
            s_lRecoverBase = ( * HB_VM_STACK.pPos )->item.asLong.value;
            ( * HB_VM_STACK.pPos )->type = HB_IT_NIL;
            /*
             * 2) Remove RECOVER address
             */
            hb_stackDec();
            ( * HB_VM_STACK.pPos )->type = HB_IT_NIL;
            /*
             * 1) Leave the value returned from BREAK  - it will be popped
             * in next executed opcode
             */
            w++;
            break;

         /* Jumps */

         case HB_P_JUMPNEAR:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_JUMPNEAR") );
            lOffset = pCode[ w + 1 ];
            if( lOffset > 127 )
               lOffset -= 256 ;
            w += lOffset;
            break;

         case HB_P_JUMP:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_JUMP") );
            lOffset = pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 );
            if( lOffset > SHRT_MAX )
               lOffset -= 65536;
            w += lOffset;
            break;

         case HB_P_JUMPFAR:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_JUMPFAR") );
            lOffset = pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) + ( pCode[ w + 3 ] * 65536 );
            if( lOffset > 8388607L )
               lOffset -= 16777216L;
            w += lOffset;
            break;

         case HB_P_JUMPFALSENEAR:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_JUMPFALSENEAR") );
            if( ! hb_vmPopLogical() )
            {
               lOffset = pCode[ w + 1 ];
               if( lOffset > 127 )
                  lOffset -= 256;

               w += lOffset;
            }
            else
               w += 2;
            break;

         case HB_P_JUMPFALSE:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_JUMPFALSE") );
            if( ! hb_vmPopLogical() )
            {
               lOffset = pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 );
               if( lOffset > SHRT_MAX )
                  lOffset -= 65536;

               w += lOffset;
            }
            else
               w += 3;
            break;

         case HB_P_JUMPFALSEFAR:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_JUMPFALSEFAR") );
            if( ! hb_vmPopLogical() )
            {
               lOffset = pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) + ( pCode[ w + 3 ] * 65536 );
               if( lOffset > 8388607L )
                  lOffset -= 16777216L;

               w += lOffset;
            }
            else
               w += 4;
            break;

         case HB_P_JUMPTRUENEAR:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_JUMPTRUENEAR") );
            if( hb_vmPopLogical() )
            {
               lOffset = pCode[ w + 1 ];
               if( lOffset > 127 )
                  lOffset -= 256;

               w += lOffset;
            }
            else
               w += 2;
            break;

         case HB_P_JUMPTRUE:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_JUMPTRUE") );
            if( hb_vmPopLogical() )
            {
               lOffset = pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 );
               if( lOffset > SHRT_MAX )
                  lOffset -= 65536;

               w += lOffset;
            }
            else
               w += 3;
            break;

         case HB_P_JUMPTRUEFAR:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_JUMPTRUEFAR") );
            if( hb_vmPopLogical() )
            {
               lOffset = pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) + ( pCode[ w + 3 ] * 65536 );
               if( lOffset > 8388607L )
                  lOffset -= 16777216L;

               w += lOffset;
            }
            else
               w += 4;
            break;

         /* Push */

         case HB_P_TRUE:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_TRUE") );
            ( * HB_VM_STACK.pPos )->type = HB_IT_LOGICAL;
            ( * HB_VM_STACK.pPos )->item.asLogical.value = TRUE;
            hb_stackPush();
            w++;
            break;

         case HB_P_FALSE:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_FALSE") );
            ( * HB_VM_STACK.pPos )->type = HB_IT_LOGICAL;
            ( * HB_VM_STACK.pPos )->item.asLogical.value = FALSE;
            hb_stackPush();
            w++;
            break;

         case HB_P_ONE:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_ONE") );
            ( * HB_VM_STACK.pPos )->type = HB_IT_INTEGER;
            ( * HB_VM_STACK.pPos )->item.asInteger.value = 1;
            ( * HB_VM_STACK.pPos )->item.asInteger.length = 10;
            hb_stackPush();
            w++;
            break;

         case HB_P_ZERO:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_ZERO") );
            ( * HB_VM_STACK.pPos )->type = HB_IT_INTEGER;
            ( * HB_VM_STACK.pPos )->item.asInteger.value = 0;
            ( * HB_VM_STACK.pPos )->item.asInteger.length = 10;
            hb_stackPush();
            w++;
            break;

         case HB_P_PUSHNIL:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHNIL") );
            ( * HB_VM_STACK.pPos )->type = HB_IT_NIL;
            hb_stackPush();
            w++;
            break;

         case HB_P_PUSHBYTE:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHBYTE") );
            ( * HB_VM_STACK.pPos )->type = HB_IT_INTEGER;
            ( * HB_VM_STACK.pPos )->item.asInteger.value = ( char ) pCode[ w + 1 ];
            ( * HB_VM_STACK.pPos )->item.asInteger.length = 10;
            hb_stackPush();
            w += 2;
            break;

         case HB_P_PUSHINT:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHINT") );
            ( * HB_VM_STACK.pPos )->type = HB_IT_INTEGER;
            ( * HB_VM_STACK.pPos )->item.asInteger.value = ( short ) ( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) );
            ( * HB_VM_STACK.pPos )->item.asInteger.length = 10;
            hb_stackPush();
            w += 3;
            break;

         case HB_P_PUSHLONG:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHLONG") );
            hb_vmPushLongConst( * ( long * ) ( &pCode[ w + 1 ] ) );
            w += 5;
            break;

         case HB_P_PUSHDOUBLE:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHDOUBLE") );
            hb_vmPushDoubleConst( * ( double * ) ( &pCode[ w + 1 ] ),
                                  ( int ) * ( BYTE * ) &pCode[ w + 1 + sizeof( double ) ],
                                  ( int ) * ( BYTE * ) &pCode[ w + 1 + sizeof( double ) + sizeof( BYTE ) ] );
            w += 1 + sizeof( double ) + sizeof( BYTE ) + sizeof( BYTE );
            break;

         case HB_P_PUSHSTR:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHSTR") );
         {
            USHORT uiSize = pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 );

            hb_itemPushStaticString( ( char * ) ( pCode ) + w + 3, ( ULONG )( uiSize - 1 ) );

            w += ( 3 + uiSize );
            break;
         }

         case HB_P_PUSHSTRSHORT:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHSTRSHORT") );
         {
            BYTE uiSize = pCode[ w + 1 ];

            hb_itemPushStaticString( ( char * ) ( pCode ) + w + 2, ( ULONG ) ( uiSize - 1 ) );

            w += ( 2 + uiSize );
            break;
         }

         case HB_P_PUSHBLOCK:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHBLOCK") );
            /* +0    -> _pushblock
             * +1 +2 -> size of codeblock
             * +3 +4 -> number of expected parameters
             * +5 +6 -> number of referenced local variables
             * +7    -> start of table with referenced local variables
             */
            hb_vmPushBlock( ( BYTE * ) ( pCode + w ), pSymbols, pGlobals );
            w += ( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) );
            break;

         case HB_P_PUSHBLOCKSHORT:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHBLOCKSHORT") );
            /* +0    -> _pushblock
             * +1    -> size of codeblock
             */
            hb_vmPushBlockShort( ( BYTE * ) ( pCode + w ), pSymbols, pGlobals );
            w += pCode[ w + 1 ];
            break;

         case HB_P_PUSHSELF:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHSELF") );
            hb_vmPush( hb_stackSelfItem() );
            w++;
            break;

         case HB_P_PUSHSYM:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHSYM") );
            hb_vmPushSymbol( pSymbols + ( USHORT ) ( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) ) );
            w += 3;
            break;

         case HB_P_PUSHSYMNEAR:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHSYMNEAR") );
            hb_vmPushSymbol( pSymbols + ( USHORT ) pCode[ w + 1 ] );
            w += 2;
            break;

         case HB_P_PUSHALIAS:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHALIAS") );
            hb_vmPushAlias();
            w++;
            break;

         case HB_P_PUSHALIASEDFIELD:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_ALIASFIELD") );
            hb_vmPushAliasedField( pSymbols + ( USHORT ) ( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) ) );
            w += 3;
            break;

         case HB_P_PUSHALIASEDFIELDNEAR:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_ALIASFIELDNEAR") );
            hb_vmPushAliasedField( pSymbols + pCode[ w + 1 ] );
            w += 2;
            break;

         case HB_P_PUSHALIASEDVAR:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_ALIASEDVAR") );
            hb_vmPushAliasedVar( pSymbols + ( USHORT ) ( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) ) );
            w += 3;
            break;

         case HB_P_PUSHFIELD:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHFIELD") );
            /* It pushes the current value of the given field onto the eval stack
             */
            hb_rddGetFieldValue( ( * HB_VM_STACK.pPos ), pSymbols + ( USHORT ) ( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) ) );
            hb_stackPush();
            w += 3;
            break;

         case HB_P_PUSHGLOBAL:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHGLOBAL") );
            hb_vmPush( (*pGlobals)[ pCode[ w + 1 ] ] );
            w += 2;
            break;

         case HB_P_PUSHGLOBALREF:
             HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHGLOBALREF") );
             {
                short iGlobal    = pCode[ w + 1 ];
                HB_ITEM_PTR pTop = ( * HB_VM_STACK.pPos );

                pTop->type = HB_IT_BYREF;

                pTop->item.asRefer.value = iGlobal + 1; // To offset the -1 below.
                pTop->item.asRefer.offset = -1; // Because 0 will be translated as a STATIC in hb_itemUnref();
                pTop->item.asRefer.BasePtr.itemsbasePtr = pGlobals;

                if( (*pGlobals)[ iGlobal ]->type == HB_IT_STRING && ( (*pGlobals)[ iGlobal ]->item.asString.bStatic || *( (*pGlobals)[ iGlobal ]->item.asString.puiHolders ) > 1 ) )
                {
                   char *sString = (char*) hb_xgrab( (*pGlobals)[ iGlobal ]->item.asString.length + 1 );

                   memcpy( sString, (*pGlobals)[ iGlobal ]->item.asString.value, (*pGlobals)[ iGlobal ]->item.asString.length + 1 );

                   if( (*pGlobals)[ iGlobal ]->item.asString.bStatic == FALSE )
                   {
                      hb_itemReleaseString( (*pGlobals)[ iGlobal ] );
                   }

                   (*pGlobals)[ iGlobal ]->item.asString.value = sString;
                   (*pGlobals)[ iGlobal ]->item.asString.bStatic = FALSE;
                   (*pGlobals)[ iGlobal ]->item.asString.puiHolders = (USHORT *) hb_xgrab( sizeof( USHORT ) );
                   *( (*pGlobals)[ iGlobal ]->item.asString.puiHolders ) = 1;
                }

                hb_stackPush();
             }

             w += 2;
             break;

         case HB_P_PUSHLOCAL:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHLOCAL") );
            hb_vmPushLocal( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) );
            w += 3;
            break;

         case HB_P_PUSHLOCALNEAR:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHLOCALNEAR") );
            hb_vmPushLocal( ( signed char ) pCode[ w + 1 ] );
            w += 2;  /* only first two bytes are used */
            break;

         case HB_P_LOCALNEARADDINT:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_LOCALNEARADDINT") );
         {
            PHB_ITEM pLocal = hb_stackItemFromBase( pCode[ w + 1 ] );
            short iAdd = ( short ) ( pCode[ w + 2 ] + ( pCode[ w + 3 ] * 256 ) );
            double dNewVal;

            w += 4;

            if( HB_IS_BYREF( pLocal ) )
            {
               pLocal = hb_itemUnRef( pLocal );
            }

            if( pLocal->type & HB_IT_INTEGER )
            {
               dNewVal = pLocal->item.asInteger.value + iAdd;
            }
            else if( pLocal->type & HB_IT_LONG )
            {
               dNewVal = pLocal->item.asLong.value + iAdd;
            }
            else if( pLocal->type & HB_IT_DOUBLE )
            {
               dNewVal = pLocal->item.asDouble.value + iAdd;
            }
            else if( HB_IS_DATE( pLocal ) )
            {
               pLocal->item.asDate.value += iAdd;
               break;
            }
            else if( HB_IS_STRING( pLocal ) && pLocal->item.asString.length == 1 )
            {
               pLocal->item.asString.value = hb_vm_acAscii[ (unsigned char) ( pLocal->item.asString.value[0] + iAdd ) ];
               break;
            }
            else
            {
               PHB_ITEM pAdd = hb_itemPutNI( NULL, ( int ) iAdd );
               PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1081, NULL, "+", 2, pLocal, pAdd );

               hb_itemRelease( pAdd );

               if( pResult )
               {
                  hb_itemForwardValue( pLocal, pResult );
               }

               break;
            }

            if( pLocal->type & HB_IT_DOUBLE )
            {
               pLocal->item.asDouble.value = dNewVal;
            }
            else if( SHRT_MIN <= dNewVal && dNewVal <= SHRT_MAX )
            {
               pLocal->type = HB_IT_INTEGER;
               pLocal->item.asInteger.value = ( int ) dNewVal;
            }
            else if( LONG_MIN <= dNewVal && dNewVal <= LONG_MAX )
            {
               pLocal->type = HB_IT_LONG;
               pLocal->item.asLong.value = ( long ) dNewVal;
            }
            else
            {
               pLocal->type = HB_IT_DOUBLE;
               pLocal->item.asDouble.value = dNewVal;
               pLocal->item.asDouble.length = ( dNewVal >= 10000000000.0 || dNewVal <= -1000000000.0 ) ? 20 : 10;
               pLocal->item.asDouble.decimal = hb_set.HB_SET_DECIMALS;
            }

            break;
         }

         case HB_P_LOCALNEARSETINT:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_LOCALNEARSETINT") );
         {
            PHB_ITEM pLocal = hb_stackItemFromBase( pCode[ w + 1 ] );
            short iNewVal = ( short ) ( pCode[ w + 2 ] + ( pCode[ w + 3 ] * 256 ) );

            if( HB_IS_BYREF( pLocal ) )
            {
               pLocal = hb_itemUnRef( pLocal );
            }

            if( HB_IS_COMPLEX( pLocal ) )
            {
               hb_itemClear( pLocal );
            }

            pLocal->type = HB_IT_INTEGER;
            pLocal->item.asInteger.length = 10;
            pLocal->item.asInteger.value = iNewVal;

            w += 4;
            break;
         }

         case HB_P_LOCALNEARSETSTR:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_LOCALNEARSETSTR") );
         {
            PHB_ITEM pLocal = hb_stackItemFromBase( pCode[ w + 1 ] );
            USHORT uiSize = pCode[ w + 2 ] + ( pCode[ w + 3 ] * 256 );

            if( HB_IS_BYREF( pLocal ) )
            {
               pLocal = hb_itemUnRef( pLocal );
            }

            if( HB_IS_COMPLEX( pLocal ) )
            {
               hb_itemClear( pLocal );
            }

            pLocal->type = HB_IT_STRING;
            pLocal->item.asString.bStatic = TRUE;
            pLocal->item.asString.value = ( char * ) ( pCode ) + w + 4 ;
            pLocal->item.asString.length = ( ULONG ) ( uiSize - 1 );

            w += ( 4 + uiSize );
            break;
         }

         case HB_P_ADDINT:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_ADDINT") );
         {
            PHB_ITEM pTop = hb_stackItemFromTop( -1 );
            short iAdd = ( short ) ( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) );
            double dNewVal = 0.0;

            w += 3;

            if( pTop->type & HB_IT_INTEGER )
            {
               dNewVal = pTop->item.asInteger.value + iAdd;
            }
            else if( pTop->type & HB_IT_LONG )
            {
               dNewVal = pTop->item.asLong.value + iAdd;
            }
            else if( pTop->type & HB_IT_DOUBLE )
            {
               dNewVal = pTop->item.asDouble.value + iAdd;
            }
            else if( HB_IS_DATE( pTop ) )
            {
               pTop->item.asDate.value += iAdd;
               break;
            }
            else if( HB_IS_STRING( pTop ) && pTop->item.asString.length == 1 )
            {
               pTop->type = HB_IT_LONG;
               pTop->item.asLong.value  = pTop->item.asString.value[0] + iAdd;
               pTop->item.asLong.length = 10;
               //printf( "Added: %i, Result: %i", iAdd, pTop->item.asLong.value );
               break;
            }
            else
            {
               PHB_ITEM pAdd    = hb_itemPutNI( NULL, ( int ) iAdd );
               PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1081, NULL, "+", 2, pTop, pAdd );

               hb_itemRelease( pAdd );

               if( pResult )
               {
                  hb_itemForwardValue( pTop, pResult );
                  break;
               }
            }

            if( pTop->type & HB_IT_DOUBLE )
            {
               pTop->item.asDouble.value = dNewVal;
            }
            else if( SHRT_MIN <= dNewVal && dNewVal <= SHRT_MAX )
            {
               pTop->type = HB_IT_INTEGER;
               pTop->item.asInteger.value = ( int ) dNewVal;
            }
            else if( LONG_MIN <= dNewVal && dNewVal <= LONG_MAX )
            {
               pTop->type = HB_IT_LONG;
               pTop->item.asLong.value = ( long ) dNewVal;
            }
            else
            {
               pTop->type = HB_IT_DOUBLE;
               pTop->item.asDouble.value = dNewVal;
               pTop->item.asDouble.length = ( dNewVal >= 10000000000.0 || dNewVal <= -1000000000.0 ) ? 20 : 10;
               pTop->item.asDouble.decimal = hb_set.HB_SET_DECIMALS;
            }

            break;
         }

         case HB_P_LEFT:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_LEFT") );
         {
            unsigned short iNewLen = ( unsigned short ) ( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) );
            PHB_ITEM pString = hb_stackItemFromTop( -1 ), pTmp;
            char *sString;

            if( HB_IS_STRING( pString ) )
            {
               if( ( ULONG ) iNewLen < pString->item.asString.length )
               {
                  if( pString->item.asString.bStatic )
                  {
                     sString = (char*) hb_xgrab( iNewLen + 1 );
                     memcpy( sString, pString->item.asString.value, iNewLen );
                     sString[ iNewLen ] = '\0';
                     pString->item.asString.bStatic = FALSE;
                     pString->item.asString.puiHolders = (USHORT*) hb_xgrab( sizeof( USHORT ) );
                     *( pString->item.asString.puiHolders ) = 1;
                     pString->item.asString.value = sString;
                     pString->item.asString.length = iNewLen;
                  }
                  else if( *( pString->item.asString.puiHolders ) == 1 )
                  {
                     pString->item.asString.value[ iNewLen ] = '\0';
                     pString->item.asString.length = iNewLen;
                  }
                  else
                  {
                     sString = (char*) hb_xgrab( iNewLen + 1 );
                     memcpy( sString, pString->item.asString.value, iNewLen );
                     sString[ iNewLen ] = '\0';
                     hb_itemReleaseString( pString );
                     pString->item.asString.puiHolders = (USHORT*) hb_xgrab( sizeof( USHORT ) );
                     *( pString->item.asString.puiHolders ) = 1;
                     pString->item.asString.value = sString;
                     pString->item.asString.length = iNewLen;
                  }
               }
            }
            else
            {
               hb_errRT_BASE_SubstR( EG_ARG, 1124, NULL, "LEFT", 2, pString, ( pTmp = hb_itemPutNI( NULL, iNewLen ) ) );
               hb_itemRelease( pTmp );
            }

            w += 3;
            break;
         }

         case HB_P_RIGHT:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_LEFT") );
         {
            unsigned short iNewLen = ( unsigned short ) ( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) );
            PHB_ITEM pString = hb_stackItemFromTop( -1 );
            char *sString;

            if( HB_IS_STRING( pString ) )
            {
               if( ( ULONG ) iNewLen < pString->item.asString.length )
               {
                  if( pString->item.asString.bStatic )
                  {
                     sString = (char*) hb_xgrab( iNewLen + 1 );
                     memcpy( sString, pString->item.asString.value + pString->item.asString.length - iNewLen, iNewLen );
                     sString[ iNewLen ] = '\0';
                     pString->item.asString.bStatic = FALSE;
                     pString->item.asString.puiHolders = (USHORT*) hb_xgrab( sizeof( USHORT ) );
                     *( pString->item.asString.puiHolders ) = 1;
                     pString->item.asString.value = sString;
                     pString->item.asString.length = iNewLen;
                  }
                  else if( *( pString->item.asString.puiHolders ) == 1 )
                  {
                     memmove( pString->item.asString.value, pString->item.asString.value + pString->item.asString.length - iNewLen, iNewLen );
                     pString->item.asString.value[ iNewLen ] = '\0';
                     pString->item.asString.length = iNewLen;
                  }
                  else
                  {
                     sString = (char*) hb_xgrab( iNewLen + 1 );
                     memcpy( sString, pString->item.asString.value + pString->item.asString.length - iNewLen, iNewLen );
                     sString[ iNewLen ] = '\0';
                     hb_itemReleaseString( pString );
                     pString->item.asString.puiHolders = (USHORT*) hb_xgrab( sizeof( USHORT ) );
                     *( pString->item.asString.puiHolders ) = 1;
                     pString->item.asString.value = sString;
                     pString->item.asString.length = iNewLen;
                  }
               }
            }
            else
            {
               /* Clipper doesn't error */
               hb_itemPutC( pString, NULL );
            }

            w += 3;
            break;
         }

         case HB_P_PUSHLOCALREF:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHLOCALREF") );
            hb_vmPushLocalByRef( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) );
            w += 3;
            break;

         case HB_P_PUSHSTATIC:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHSTATIC") );
            hb_vmPushStatic( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) );
            w += 3;
            break;

         case HB_P_PUSHSTATICREF:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHSTATICREF") );
            hb_vmPushStaticByRef( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) );
            w += 3;
            break;

         case HB_P_PUSHMEMVAR:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHMEMVAR") );
            hb_memvarGetValue( ( * HB_VM_STACK.pPos ), pSymbols + ( USHORT ) ( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) ) );
            hb_stackPush();
            w += 3;
            break;

         case HB_P_PUSHMEMVARREF:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHMEMVARREF") );
            hb_memvarGetRefer( ( * HB_VM_STACK.pPos ), pSymbols + ( USHORT ) ( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) ) );
            hb_stackPush();
            w += 3;
            break;

         case HB_P_PUSHVARIABLE:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_PUSHVARIABLE") );
            /* Push a value of variable of unknown type onto the eval stack
             */
            hb_vmPushVariable( pSymbols + pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) );
            w += 3;
            break;

         case HB_P_DUPLICATE:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_DUPLICATE") );
            hb_vmDuplicate();
            w++;
            break;

         case HB_P_DUPLTWO:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_DUPLTWO") );
            hb_vmDuplTwo();
            w++;
            break;

         /* Pop */

         case HB_P_POP:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_POP") );
            hb_stackPop();
            w++;
            break;

         case HB_P_POPALIAS:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_POPALIAS") );
            hb_vmPopAlias();
            w++;
            break;

         case HB_P_POPALIASEDFIELD:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_POPALIASEDFIELD") );
            hb_vmPopAliasedField( pSymbols + ( USHORT ) ( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) ) );
            w += 3;
            break;

         case HB_P_POPALIASEDFIELDNEAR:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_POPALIASEDFIELDNEAR") );
            hb_vmPopAliasedField( pSymbols + pCode[ w + 1 ] );
            w += 2;
            break;

         case HB_P_POPALIASEDVAR:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_POPALIASEDVAR") );
            hb_vmPopAliasedVar( pSymbols + ( USHORT ) ( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) ) );
            w += 3;
            break;

         case HB_P_POPFIELD:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_POPFIELD") );
            /* Pops a value from the eval stack and uses it to set
             * a new value of the given field
             */
         {
            PHB_ITEM pTop;

            hb_rddPutFieldValue( ( hb_stackItemFromTop(-1) ), pSymbols + ( USHORT ) ( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) ) );
            hb_stackDec();

            pTop = ( * HB_VM_STACK.pPos );

            if( HB_IS_COMPLEX( pTop ) )
            {
               hb_itemClear( pTop );
            }
            else
            {
               pTop->type = HB_IT_NIL;
            }

            HB_TRACE(HB_TR_INFO, ("(hb_vmPopField)"));
            w += 3;
            break;
         }

         case HB_P_POPGLOBAL:
         {
            BYTE iGlobal = pCode[ w + 1 ];

            HB_TRACE( HB_TR_DEBUG, ("HB_P_POPGLOBAL") );

            hb_itemForwardValue( (*pGlobals)[ iGlobal ], *( HB_VM_STACK.pPos - 1 ) );

            hb_stackDec();

            w += 2;
            break;
         }

         case HB_P_POPLOCAL:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_POPLOCAL") );
            hb_vmPopLocal( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) );
            w += 3;
            break;

         case HB_P_POPLOCALNEAR:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_POPLOCALNEAR") );
            hb_vmPopLocal( ( signed char ) pCode[ w + 1 ] );
            w += 2;  /* only first two bytes are used */
            break;

         case HB_P_POPSTATIC:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_POPSTATIC") );
            hb_vmPopStatic( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) );
            w += 3;
            break;

         case HB_P_POPMEMVAR:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_POPMEMVAR") );
         {
            PHB_ITEM pTop;

            hb_stackDec();

            pTop = ( * HB_VM_STACK.pPos );

            hb_memvarSetValue( pSymbols + ( USHORT ) ( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) ), pTop );

            if( HB_IS_COMPLEX( pTop ) )
            {
               hb_itemClear( pTop );
            }
            else
            {
               pTop->type = HB_IT_NIL;
            }

            HB_TRACE(HB_TR_INFO, ("(hb_vmPopMemvar)"));
            w += 3;
            break;
         }

         case HB_P_POPVARIABLE:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_POPVARIABLE") );
         {
            USHORT uiParams;
            PHB_DYNS pDyn;

            /* Pops a value from the eval stack and uses it to set
             * a new value of a variable of unknown type.
             */
            uiParams = pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 );
            /* First try if passed symbol is a name of field
             * in a current workarea - if it is not a field (FAILURE)
             * then try the memvar variable (it will create PRIVATE
             * variable if this variable doesn't exist)
             */

            /* memvars.c 417 */
            pDyn = ( PHB_DYNS ) (pSymbols + uiParams)->pDynSym;

            if( pDyn && pDyn->hMemvar )
            {
               /* If exist a memory symbol with this name use it */
               hb_memvarSetValue( pSymbols + uiParams, ( hb_stackItemFromTop(-1) ) );
            }
            else
            {
               /* Try with a field and after create a memvar */
               if( hb_rddFieldPut( ( hb_stackItemFromTop(-1) ), pSymbols + uiParams ) == FAILURE )
               {
                  hb_memvarSetValue( pSymbols + uiParams, ( hb_stackItemFromTop(-1) ) );
               }
            }

            hb_stackPop();

            w += 3;
            break;
         }

         /* macro creation */

         case HB_P_MACROPOP:
            /* compile and run - pop a value from the stack */
            hb_macroSetValue( hb_stackItemFromTop( -1 ), pCode[ ++w ] );
            w++;
            break;

         case HB_P_MACROPOPALIASED:
            /* compile and run - pop an aliased variable from the stack */
            hb_macroPopAliasedValue( hb_stackItemFromTop( - 2  ), hb_stackItemFromTop( -1 ), pCode[ ++w ] );
            w++;
            break;

         case HB_P_MACROPUSH:
            /* compile and run - leave the result on the stack */
            /* the topmost element on the stack contains a macro
             * string for compilation
             */
            hb_macroGetValue( hb_stackItemFromTop( -1 ), 0, pCode[ ++w ] );
            w++;
            break;

         case HB_P_MACROPUSHARG:
            /* compile and run - leave the result on the stack */
            /* the topmost element on the stack contains a macro
             * string for compilation
             */
            hb_macroGetValue( hb_stackItemFromTop( -1 ), HB_P_MACROPUSHARG, pCode[ ++w ] );
            w++;

            if( hb_vm_iExtraParamsIndex && hb_vm_apExtraParamsSymbol[hb_vm_iExtraParamsIndex - 1] == NULL )
            {
               if( pCode[w] == HB_P_PUSHSYMNEAR )
               {
                  hb_vm_apExtraParamsSymbol[hb_vm_iExtraParamsIndex - 1] = pSymbols + ( USHORT ) ( pCode[w + 1] );
                  w += 2;
               }
               else if( pCode[w] == HB_P_MPUSHSYM )
               {
                  HB_DYNS_PTR *pDynSym = ( HB_DYNS_PTR * ) ( pCode + w + 1 );

                  hb_vm_apExtraParamsSymbol[hb_vm_iExtraParamsIndex - 1] = ( *pDynSym )->pSymbol;
                  w += sizeof( HB_DYNS_PTR ) + 1;
               }
               else
               {
                  hb_vm_apExtraParamsSymbol[hb_vm_iExtraParamsIndex - 1] = pSymbols + ( USHORT ) ( pCode[w + 1] + ( pCode[w + 2] * 256 ) );
                  w += 3;
               }
            }
            else
            {
               if( pCode[w] == HB_P_PUSHSYMNEAR )
               {
                  w += 2;
               }
               else if( pCode[w] == HB_P_MPUSHSYM )
               {
                  w += sizeof( HB_DYNS_PTR ) + 1;
               }
               else
               {
                  w += 3;
               }
            }
            break;

         case HB_P_MACROLIST:
            hb_vm_aiExtraElements[hb_vm_iExtraElementsIndex++] = 0;
            w++;
            break;

         case HB_P_MACROPUSHLIST:
            /* compile and run - leave the result on the stack */
            /* the topmost element on the stack contains a macro
             * string for compilation
             */
            hb_macroGetValue( hb_stackItemFromTop( -1 ), HB_P_MACROPUSHLIST, pCode[ ++w ] );
            w++;
            break;

         case HB_P_MACROLISTEND:
            hb_vm_iExtraElements = hb_vm_aiExtraElements[--hb_vm_iExtraElementsIndex];
            w++;
            break;

         case HB_P_MACROPUSHINDEX:
            /* compile and run - leave the result on the stack */
            /* the topmost element on the stack contains a macro
             * string for compilation
             */
            hb_macroGetValue( hb_stackItemFromTop( -1 ), HB_P_MACROPUSHINDEX, pCode[ ++w ] );
            if( hb_vm_iExtraIndex )
            {
               HB_ITEM *aExtraItems = (HB_ITEM *) hb_xgrab( sizeof( HB_ITEM ) * hb_vm_iExtraIndex );
               int i;

               /* Storing and removing the extra indexes. */
               for ( i = hb_vm_iExtraIndex - 1; i >= 0; i-- )
               {
                  hb_itemCopy( aExtraItems + i, hb_stackItemFromTop(-1) );
                  hb_stackPop();
               }

               /* First index is still on stack.*/
               hb_vmArrayPush();

               /* Now process each of the additional index including the last one (we will skip the HB_P_ARRAYPUSH which is know to follow . */
               for ( i = 0; i < hb_vm_iExtraIndex; i++ )
               {
                  hb_vmPush( aExtraItems + i );
                  hb_vmArrayPush();
               }

               hb_xfree( aExtraItems );

               w++; /* To force skip the HB_P_ARRAYPUSH (was already processed above). */
            }

            w++;
            break;

         case HB_P_MACROPUSHPARE:
            /* compile and run - leave the result on the stack */
            /* the topmost element on the stack contains a macro
             * string for compilation
             */
            hb_macroGetValue( hb_stackItemFromTop( -1 ), HB_P_MACROPUSHPARE, pCode[ ++w ] );
            w++;
            break;

         case HB_P_MACROPUSHALIASED:
            /* compile and run - leave an aliased variable on the stack */
            hb_macroPushAliasedValue( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ), pCode[ ++w ] );
            w++;
            break;

         case HB_P_MACROSYMBOL:
            /* compile into a symbol name (used in function calls) */
            hb_macroPushSymbol( hb_stackItemFromTop( -1 ) );
            w++;
            break;

         case HB_P_MACROTEXT:
            /* macro text substitution
             * "text &macro.other text"
             */
            hb_macroTextValue( hb_stackItemFromTop( -1 ) );
            w++;
            break;

         /* macro compiled opcodes - we are using symbol address here */

         case HB_P_MMESSAGE:
         {
            HB_DYNS_PTR * pDynSym = ( HB_DYNS_PTR * ) ( pCode + w + 1 );

            hb_vmPushSymbol( ( *pDynSym )->pSymbol );
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPOPALIASEDFIELD:
         {
            HB_DYNS_PTR * pDynSym = ( HB_DYNS_PTR * ) ( pCode + w + 1 );

            hb_vmPopAliasedField( ( *pDynSym )->pSymbol );
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPOPALIASEDVAR:
         {
            HB_DYNS_PTR * pDynSym = ( HB_DYNS_PTR * ) ( pCode + w + 1 );
            hb_vmPopAliasedVar( ( *pDynSym )->pSymbol );
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPOPFIELD:
         {
            HB_DYNS_PTR * pDynSym = ( HB_DYNS_PTR * ) ( pCode + w + 1 );
            PHB_ITEM pTop;

            /* Pops a value from the eval stack and uses it to set
            * a new value of the given field
            */
            hb_rddPutFieldValue( ( hb_stackItemFromTop(-1) ), ( *pDynSym )->pSymbol );

            hb_stackDec();

            pTop = ( * HB_VM_STACK.pPos );

            if( HB_IS_COMPLEX( pTop ) )
            {
               hb_itemClear( pTop );
            }
            else
            {
               pTop->type = HB_IT_NIL;
            }

            HB_TRACE(HB_TR_INFO, ("(hb_vmMPopField)"));
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPOPMEMVAR:
         {
            HB_DYNS_PTR * pDynSym = ( HB_DYNS_PTR * ) ( pCode + w + 1 );
            PHB_ITEM pTop;

            pTop = *( HB_VM_STACK.pPos - 1 );

            hb_memvarSetValue( ( *pDynSym )->pSymbol, pTop );

            hb_stackPop();

            HB_TRACE(HB_TR_INFO, ("(hb_vmMPopMemvar)"));
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPUSHALIASEDFIELD:
         {
            HB_DYNS_PTR * pDynSym = ( HB_DYNS_PTR * ) ( pCode + w + 1 );
            hb_vmPushAliasedField( ( *pDynSym )->pSymbol );
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPUSHALIASEDVAR:
         {
            HB_DYNS_PTR * pDynSym = ( HB_DYNS_PTR * ) ( pCode + w + 1 );
            hb_vmPushAliasedVar( ( *pDynSym )->pSymbol );
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPUSHBLOCK:
         {
            /*NOTE: the pcode is stored in dynamically allocated memory
            * We need to handle it with more care than compile-time
            * codeblocks
            */
            /* +0    -> _pushblock
             * +1 +2 -> size of codeblock
             * +3 +4 -> number of expected parameters
             * +5    -> pcode bytes
             */
            hb_vmPushMacroBlock( ( BYTE * ) ( pCode + w ), pSymbols );
            w += ( pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 ) );
            break;
         }

         case HB_P_MPUSHFIELD:
         {
            HB_DYNS_PTR * pDynSym = ( HB_DYNS_PTR * ) ( pCode + w + 1 );
            /* It pushes the current value of the given field onto the eval stack
            */
            hb_rddGetFieldValue( ( * HB_VM_STACK.pPos ), ( *pDynSym )->pSymbol );
            hb_stackPush();
            HB_TRACE(HB_TR_INFO, ("(hb_vmMPushField)"));
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPUSHMEMVAR:
         {
            HB_DYNS_PTR * pDynSym = ( HB_DYNS_PTR * ) ( pCode + w + 1 );
            hb_memvarGetValue( ( * HB_VM_STACK.pPos ), ( *pDynSym )->pSymbol );
            hb_stackPush();
            HB_TRACE(HB_TR_INFO, ("(hb_vmMPushMemvar)"));
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPUSHMEMVARREF:
         {
            HB_DYNS_PTR * pDynSym = ( HB_DYNS_PTR * ) ( pCode + w + 1 );
            hb_memvarGetRefer( ( * HB_VM_STACK.pPos ), ( *pDynSym )->pSymbol );
            hb_stackPush();
            HB_TRACE(HB_TR_INFO, ("(hb_vmMPushMemvarRef)"));
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPUSHSYM:
         {
            HB_DYNS_PTR * pDynSym = ( HB_DYNS_PTR * ) ( pCode + w + 1 );
            hb_vmPushSymbol( ( *pDynSym )->pSymbol );
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPUSHVARIABLE:
         {
            HB_DYNS_PTR * pDynSym = ( HB_DYNS_PTR * ) ( pCode + w + 1 );
            hb_vmPushVariable( ( *pDynSym )->pSymbol );
            w += sizeof( HB_DYNS_PTR ) + 1;
            break;
         }

         case HB_P_MPUSHSTR:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_MPUSHSTR") );
         {
            USHORT uiSize = pCode[ w + 1 ] + ( pCode[ w + 2 ] * 256 );

            hb_vmPushString( ( char * ) ( pCode ) + w + 3, ( ULONG )( uiSize - 1 ) );

            w += ( 3 + uiSize );
            break;
         }

         /* misc */

         case HB_P_NOOP:
            HB_TRACE( HB_TR_DEBUG, ("HB_P_NOOP") );
            /* Intentionally do nothing */
            w++;
            break;

         default:
            /* TODO: Include to failing pcode in the error message */
            hb_errInternal( HB_EI_VMBADOPCODE, NULL, NULL, NULL );
            break;
      }

      if( s_uiActionRequest )
      {
         if( s_uiActionRequest & HB_BREAK_REQUESTED )
         {
            if( bCanRecover )
            {
               /*
                * There is the BEGIN/END sequence deifined in current
                * procedure/function - use it to continue opcodes execution
                */
               /*
                * remove all items placed on the stack after BEGIN code
                */
               hb_stackRemove( s_lRecoverBase );
               /*
                * reload the address of recovery code
                */
               w = ( hb_stackItem( s_lRecoverBase + HB_RECOVER_ADDRESS ) )->item.asLong.value;
               /*
                * leave the SEQUENCE envelope on the stack - it will
                * be popped either in RECOVER or END opcode
                */
               s_uiActionRequest = 0;
            }
            else
               break;
         }
         else if( s_uiActionRequest & HB_QUIT_REQUESTED )
            break;
         else if( s_uiActionRequest & HB_ENDPROC_REQUESTED )
         {
            /* request to stop current procedure was issued
             * (from macro evaluation)
             */
            s_uiActionRequest = 0;
            break;
         }
      }
   }

   HB_TRACE(HB_TR_DEBUG, ("DONE hb_vmExecute(%p, %p)", pCode, pSymbols));

   if( pSymbols )
   {
      hb_memvarSetPrivatesBase( ulPrivateBase );
   }

   // Reset FOR EACH.
   while( hb_vm_wEnumCollectionCounter > wEnumCollectionCounter )
   {
      hb_vm_wEnumCollectionCounter--;
      hb_itemClear( &( hb_vm_aEnumCollection[ hb_vm_wEnumCollectionCounter ] ) );
      hb_vm_awEnumIndex[ hb_vm_wEnumCollectionCounter ] = 0;
   }

   // Reset WITH OBJECT.
   while( hb_vm_wWithObjectCounter > wWithObjectCounter )
   {
      --hb_vm_wWithObjectCounter;
      hb_itemClear( &( hb_vm_aWithObject[ hb_vm_wWithObjectCounter ] ) );
   }

   HB_TRACE(HB_TR_DEBUG, ("RESET PrivateBase hb_vmExecute(%p, %p)", pCode, pSymbols));
}

/* ------------------------------- */
/* Operators ( mathematical        */
/*             character / misc )  */
/* ------------------------------- */

/* NOTE: Clipper is resetting the number width on a negate. */

static void hb_vmNegate( void )
{
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmNegate()"));

   pItem = hb_stackItemFromTop( -1 );

   if( HB_IS_INTEGER( pItem ) )
   {
      pItem->item.asInteger.value = -pItem->item.asInteger.value;
      pItem->item.asInteger.length = 10;
   }
   else if( HB_IS_LONG( pItem ) )
   {
      pItem->item.asLong.value = -pItem->item.asLong.value;
      pItem->item.asLong.length = ( pItem->item.asLong.value <= -1000000000 ) ? 20 : 10;
   }
   else if( HB_IS_DOUBLE( pItem ) )
   {
      pItem->item.asDouble.value = -pItem->item.asDouble.value;
      /* NOTE: Yes, -999999999.0 is right instead of -1000000000.0 [vszakats] */
      pItem->item.asDouble.length = ( pItem->item.asDouble.value >= 10000000000.0 || pItem->item.asDouble.value <= -999999999.0 ) ? 20 : 10;
   }
   else if( HB_IS_STRING( pItem ) && pItem->item.asString.length == 1 )
   {
      pItem->item.asInteger.value = - pItem->item.asString.value[0];
      pItem->type = HB_IT_INTEGER;
      pItem->item.asInteger.length = 10;
   }
   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1080, NULL, "-", 1, pItem );

      if( pResult )
      {
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmPlus( void )
{
   PHB_ITEM pItem1;
   PHB_ITEM pItem2;

   HB_TRACE( HB_TR_DEBUG, ( "hb_vmPlus()" ) );

   pItem1 = hb_stackItemFromTop( -2 );
   pItem2 = hb_stackItemFromTop( -1 );

   // Must be first because STRING (length 1) qualifies as NUMERIC!
   if( HB_IS_STRING( pItem1 ) && HB_IS_STRING( pItem2 ) )
   {
      ULONG ulLen1     = pItem1->item.asString.length;
      ULONG ulLen2     = pItem2->item.asString.length;

      //printf( "Adding '%s' + '%s'\n", pItem1->item.asString.value, pItem2->item.asString.value );

      if( ( double ) ( ( double ) ulLen1 + ( double ) ulLen2 ) < ( double ) ULONG_MAX )
      {
         ULONG ulNewLen   = ulLen1 + ulLen2; char *pNewString;

         if( pItem1->item.asString.bStatic || ( *( pItem1->item.asString.puiHolders ) > 1 ) )
         {
             pNewString = ( char * ) hb_xgrab( ulNewLen + 1 );

             hb_xmemcpy( (void * ) pNewString, (void *) pItem1->item.asString.value, ulLen1 );

             hb_itemReleaseString( pItem1 );

             pItem1->item.asString.puiHolders = (USHORT*) hb_xgrab( sizeof( USHORT ) );
             *( pItem1->item.asString.puiHolders ) = 1;
             pItem1->item.asString.bStatic = FALSE;
         }
         else
         {
             pNewString = pItem1->item.asString.value;
             pNewString = (char *) hb_xrealloc( (void *) pNewString, ulNewLen + 1 );
         }

         hb_xmemcpy( pNewString + ulLen1, pItem2->item.asString.value, ulLen2 );

         pItem1->item.asString.value   = pNewString;
         pItem1->item.asString.length  = ulNewLen;
         pItem1->item.asString.value[ ulNewLen ] = '\0';

         hb_stackPop();
      }
      else
      {
         HB_TRACE( HB_TR_DEBUG, ( "Error! String overflow in hb_vmPlus()" ) );
         hb_errRT_BASE( EG_STROVERFLOW, 1209, NULL, "+", 2, pItem1, pItem2 );
      }
   }
   /* Intentionally using HB_IS_NUMERIC() instead of HB_IS_NUMBER() on the right
      Clipper consider DATE + NUMBER => DATE and DATE + DATE => DATE
   */
   else if( ( HB_IS_STRING( pItem1 ) || HB_IS_STRING( pItem2 ) ) && ( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) ) )
   {
      double dNumber2 = hb_vmPopNumber();

      pItem1->item.asString.value = hb_vm_acAscii[ (unsigned char) ( hb_itemGetND( pItem1 ) + dNumber2 ) ];

      if( pItem1->type != HB_IT_STRING )
      {
         pItem1->type = HB_IT_STRING;
         pItem1->item.asString.bStatic = TRUE;
         pItem1->item.asString.length = 1;
      }
   }
   else if( ( HB_IS_DATE( pItem1 ) || HB_IS_DATE( pItem2 ) ) && ( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) ) )
   {
      hb_vmPushDate( (long) hb_vmPopNumber() + (long) hb_vmPopNumber() );
   }
   else if( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
   {
      int iDec2, iDec1, iType2 = pItem2->type, iType1 = pItem2->type;
      double dNumber2 = hb_vmPopDouble( &iDec2 );
      double dNumber1 = hb_vmPopDouble( &iDec1 );

      hb_vmPushNumType( dNumber1 + dNumber2, ( ( iDec1 > iDec2 ) ? iDec1 : iDec2 ), iType1, iType2 );
   }
   else if( HB_IS_OBJECT( pItem1 ) && hb_objHasMsg( pItem1, "__OpPlus" ) )
   {
      hb_vmOperatorCall( pItem1, pItem2, "__OPPLUS" );
   }
   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1081, NULL, "+", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmMinus( void )
{
   PHB_ITEM pItem1;
   PHB_ITEM pItem2;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmMinus()"));

   pItem1 = hb_stackItemFromTop( -2 );
   pItem2 = hb_stackItemFromTop( -1 );

   // Must be first because STRING (length 1) qualifies as NUMERIC!
   if( HB_IS_STRING( pItem1 ) && HB_IS_STRING( pItem2 ) )
   {
      if( ( double ) ( ( double ) pItem1->item.asString.length + ( double ) pItem2->item.asString.length ) < ( double ) ULONG_MAX )
      {
         ULONG ulLen      = pItem1->item.asString.length;
         ULONG ulNewLen   = pItem1->item.asString.length + pItem2->item.asString.length;
         char *pNewString = ( char * ) hb_xgrab( ulNewLen + 1 );

         hb_xmemcpy( pNewString, pItem1->item.asString.value, pItem1->item.asString.length );

         hb_itemReleaseString( pItem1 );
         HB_TRACE( HB_TR_DEBUG, ( "Released hb_vmMinus() Created \"%s\"", pNewString ) );

         pItem1->item.asString.puiHolders = (USHORT*) hb_xgrab( sizeof( USHORT ) );
         *( pItem1->item.asString.puiHolders ) = 1;
         pItem1->item.asString.bStatic = FALSE;
         pItem1->item.asString.value   = pNewString;

         pItem1->item.asString.length = ulNewLen;

         while( ulLen && pItem1->item.asString.value[ ulLen - 1 ] == ' ' )
         {
            ulLen--;
         }

         hb_xmemcpy( pItem1->item.asString.value + ulLen, pItem2->item.asString.value, pItem2->item.asString.length );
         ulLen += pItem2->item.asString.length;
         hb_xmemset( pItem1->item.asString.value + ulLen, ' ', pItem1->item.asString.length - ulLen );
         pItem1->item.asString.value[ pItem1->item.asString.length ] = '\0';

         hb_stackPop();
      }
      else
      {
         hb_errRT_BASE( EG_STROVERFLOW, 1210, NULL, "-", 2, pItem1, pItem2 );
      }
   }
   else if( ( HB_IS_STRING( pItem1 ) || HB_IS_STRING( pItem2 ) ) && ( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) ) )
   {
      double dNumber2 = hb_vmPopNumber();

      pItem1->item.asString.value = hb_vm_acAscii[ (unsigned char) ( hb_itemGetND( pItem1 ) - dNumber2 ) ];

      if( pItem1->type != HB_IT_STRING )
      {
         pItem1->type = HB_IT_STRING;
         pItem1->item.asString.bStatic = TRUE;
         pItem1->item.asString.length = 1;
      }
   }
   else if( HB_IS_DATE( pItem1 ) && HB_IS_NUMBER( pItem2 ) )
   {
      double dNumber2 = hb_vmPopNumber();
      double dNumber1 = hb_vmPopNumber();

      hb_vmPushDate( (long) dNumber1 - (long) dNumber2 );
   }
   else if( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
   {
      int iDec2, iDec1, iType2 = pItem2->type, iType1 = pItem1->type;
      double dNumber2 = hb_vmPopDouble( &iDec2 );
      double dNumber1 = hb_vmPopDouble( &iDec1 );

      hb_vmPushNumType( dNumber1 - dNumber2, ( ( iDec1 > iDec2 ) ? iDec1 : iDec2 ), iType1, iType2 );
   }
   else if( HB_IS_OBJECT( pItem1 ) && hb_objHasMsg( pItem1, "__OpMinus" ) )
   {
      hb_vmOperatorCall( pItem1, pItem2, "__OPMINUS" );
   }
   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1082, NULL, "-", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmMult( void )
{
   PHB_ITEM pItem1;
   PHB_ITEM pItem2;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmMult()"));

   pItem1 = hb_stackItemFromTop( -2 );
   pItem2 = hb_stackItemFromTop( -1 );

   if( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
   {
      int iDec2, iDec1, iType2 = pItem2->type, iType1 = pItem1->type;
      double d2 = hb_vmPopDouble( &iDec2 );
      double d1 = hb_vmPopDouble( &iDec1 );

      hb_vmPushNumType( d1 * d2, iDec1 + iDec2, iType1, iType2 );
   }
   else if( HB_IS_OBJECT( pItem1 ) && hb_objHasMsg( pItem1, "__OpMult" ) )
   {
      hb_vmOperatorCall( pItem1, pItem2, "__OPMULT" );
   }
   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1083, NULL, "*", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmDivide( void )
{
   PHB_ITEM pItem1;
   PHB_ITEM pItem2;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmDivide()"));

   pItem1 = hb_stackItemFromTop( -2 );
   pItem2 = hb_stackItemFromTop( -1 );

   if( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
   {
      double d2 = hb_vmPopNumber();
      double d1 = hb_vmPopNumber();

      if( d2 == 0.0 )
      {
         PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ZERODIV, 1340, NULL, "/", 2, pItem1, pItem2 );

         if( pResult )
         {
            hb_itemPushForward( pResult );
            hb_itemRelease( pResult );
         }
      }
      else
      {
         /* If all both operand was integer and the result is an integer, too,
            push the number without decimals. Clipper compatible. Actually,
            this is not Clipper compatible. The only time Clipper returns 0
            decimal places is for compiler optimized integer division with an
            integer result. Therefore this code is not needed and has been
            removed - David G. Holm <dholm@jsd-llc.com>
         if( bIntegerOperands && fmod( d1, d2 ) == 0.0 )
            hb_vmPushNumber( d1 / d2, 0 );
         else
         */
         hb_vmPushDouble( d1 / d2, hb_set.HB_SET_DECIMALS );
      }
   }
   else if( HB_IS_OBJECT( pItem1 ) && hb_objHasMsg( pItem1, "__OpDivide" ) )
   {
      hb_vmOperatorCall( pItem1, pItem2, "__OPDIVIDE" );
   }
   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1084, NULL, "/", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmModulus( void )
{
   PHB_ITEM pItem1;
   PHB_ITEM pItem2;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmModulus()"));

   pItem1 = hb_stackItemFromTop( -2 );
   pItem2 = hb_stackItemFromTop( -1 );

   if( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
   {
      int iDec2, iDec1, iType2 = pItem2->type, iType1 = pItem1->type;
      double d2 = hb_vmPopDouble( &iDec2 );
      double d1 = hb_vmPopDouble( &iDec1 );

      if( d2 == 0.0 )
      {
         PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ZERODIV, 1341, NULL, "%", 2, pItem1, pItem2 );

         if( pResult )
         {
            hb_itemPushForward( pResult );
            hb_itemRelease( pResult );
         }
      }
      else
         /* NOTE: Clipper always returns the result of modulus
                  with the SET number of decimal places. */
         hb_vmPushNumType( fmod( d1, d2 ), hb_set.HB_SET_DECIMALS, iType1, iType2 );
   }
   else if( HB_IS_OBJECT( pItem1 ) && hb_objHasMsg( pItem1, "__OpMod" ) )
   {
      hb_vmOperatorCall( pItem1, pItem2, "__OPMOD" );
   }
   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1085, NULL, "%", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmPower( void )
{
   PHB_ITEM pItem1;
   PHB_ITEM pItem2;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPower()"));

   pItem1 = hb_stackItemFromTop( -2 );
   pItem2 = hb_stackItemFromTop( -1 );

   if( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
   {
      double d2 = hb_vmPopNumber();
      double d1 = hb_vmPopNumber();

      /* NOTE: Clipper always returns the result of power
               with the SET number of decimal places. */
      hb_vmPushDouble( pow( d1, d2 ), hb_set.HB_SET_DECIMALS );
   }
   else if( HB_IS_OBJECT( pItem1 ) && hb_objHasMsg( pItem1, "__OpPower" ) )
   {
      hb_vmOperatorCall( pItem1, pItem2, "__OPPOWER" );
   }
   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1088, NULL, "^", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmInc( void )
{
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmInc()"));

   pItem = hb_stackItemFromTop( -1 );

   if( HB_IS_STRING( pItem ) && pItem->item.asString.length == 1 )
   {
      pItem->item.asString.value = hb_vm_acAscii[ (unsigned char) ( pItem->item.asString.value[0] + 1 ) ];
   }
   else if( HB_IS_DATE( pItem ) )
   {
      pItem->item.asDate.value++;
   }
   else if( HB_IS_NUMERIC( pItem ) )
   {
      int iDec, iType = pItem->type;
      double dNumber = hb_vmPopDouble( &iDec );
      hb_vmPushNumType( ++dNumber, iDec, iType, iType );
   }
   else if( HB_IS_OBJECT( pItem ) && hb_objHasMsg( pItem, "__OpInc" ) )
   {
      hb_vmOperatorCallUnary( pItem, "__OPINC" );
   }
   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1086, NULL, "++", 1, pItem );

      if( pResult )
      {
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmDec( void )
{
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmDec()"));

   pItem = hb_stackItemFromTop( -1 );

   if( HB_IS_STRING( pItem ) && pItem->item.asString.length == 1 )
   {
      pItem->item.asString.value = hb_vm_acAscii[ (unsigned char) ( pItem->item.asString.value[0] - 1 ) ];
   }
   else if( HB_IS_DATE( pItem ) )
   {
      pItem->item.asDate.value--;
   }
   else if( HB_IS_NUMERIC( pItem ) )
   {
      int iDec, iType = pItem->type;
      double dNumber = hb_vmPopDouble( &iDec );
      hb_vmPushNumType( --dNumber, iDec, iType, iType );
   }
   else if( HB_IS_OBJECT( pItem ) && hb_objHasMsg( pItem, "__OpDec" ) )
   {
      hb_vmOperatorCallUnary( pItem, "__OPDEC" );
   }
   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1087, NULL, "--", 1, pItem );

      if( pResult )
      {
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmFuncPtr( void )  /* pushes a function address pointer. Removes the symbol from the satck */
{
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmFuncPtr()"));

   pItem = hb_stackItemFromTop( -1 );

   if( HB_IS_SYMBOL( pItem ) )
   {
      pItem->item.asLong.value = (long) pItem->item.asSymbol.value->pFunPtr;
      pItem->type = HB_IT_LONG;
   }
   else
   {
      hb_errInternal( HB_EI_VMNOTSYMBOL, NULL, "hb_vmFuncPtr()", NULL );
   }
}

/* ------------------------------- */
/* Operators (relational)          */
/* ------------------------------- */

static void hb_vmEqual( BOOL bExact )
{
   PHB_ITEM pItem2;
   PHB_ITEM pItem1;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmEqual(%d)", (int) bExact));

   pItem2 = hb_stackItemFromTop( -1 );
   pItem1 = hb_stackItemFromTop( -2 );

   if( HB_IS_NIL( pItem1 ) && HB_IS_NIL( pItem2 ) )
   {
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( TRUE );
   }

   else if( HB_IS_NIL( pItem1 ) || HB_IS_NIL( pItem2 ) )
   {
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( FALSE );
   }

   else if( HB_IS_STRING( pItem1 ) && HB_IS_STRING( pItem2 ) )
   {
      int i = hb_itemStrCmp( pItem1, pItem2, bExact );
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( i == 0 );
   }

   else if( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
      hb_vmPushLogical( hb_vmPopNumber() == hb_vmPopNumber() );
   else if( HB_IS_LOGICAL( pItem1 ) && HB_IS_LOGICAL( pItem2 ) )
      hb_vmPushLogical( hb_vmPopLogical() == hb_vmPopLogical() );
   else if( HB_IS_OBJECT( pItem1 ) && hb_objHasMsg( pItem1, "__OpEqual" ) )
      hb_vmOperatorCall( pItem1, pItem2, "__OPEQUAL" );
   else if( bExact && HB_IS_ARRAY( pItem1 ) && HB_IS_ARRAY( pItem2 ) )
   {
      BOOL bResult = pItem1->item.asArray.value->pItems && pItem2->item.asArray.value->pItems &&
                     pItem1->item.asArray.value->pItems == pItem2->item.asArray.value->pItems;
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( bResult );
   }
   else if( pItem1->type != pItem2->type ||
            ( HB_IS_BLOCK( pItem1 ) && HB_IS_BLOCK( pItem2 ) ) ||
            ( ! bExact && HB_IS_ARRAY( pItem1 ) && HB_IS_ARRAY( pItem2 ) ) )
   {
      PHB_ITEM pResult;

      if( bExact )
         pResult = hb_errRT_BASE_Subst( EG_ARG, 1070, NULL, "==", 2, pItem1, pItem2 );
      else
         pResult = hb_errRT_BASE_Subst( EG_ARG, 1071, NULL, "=", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }
   }
   else
   {
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( FALSE );
   }
}

static void hb_vmNotEqual( void )
{
   PHB_ITEM pItem2;
   PHB_ITEM pItem1;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmNotEqual()"));

   pItem2 = hb_stackItemFromTop( -1 );
   pItem1 = hb_stackItemFromTop( -2 );

   if( HB_IS_NIL( pItem1 ) && HB_IS_NIL( pItem2 ) )
   {
      hb_stackDec();
      hb_stackDec();
      hb_vmPushLogical( FALSE );
   }

   else if( HB_IS_NIL( pItem1 ) || HB_IS_NIL( pItem2 ) )
   {
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( TRUE );
   }

   else if( HB_IS_STRING( pItem1 ) && HB_IS_STRING( pItem2 ) )
   {
      int i = hb_itemStrCmp( pItem1, pItem2, FALSE );
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( i != 0 );
   }

   else if( HB_IS_NUMERIC( pItem1 ) && HB_IS_NUMERIC( pItem2 ) )
      hb_vmPushLogical( hb_vmPopNumber() != hb_vmPopNumber() );
   else if( HB_IS_LOGICAL( pItem1 ) && HB_IS_LOGICAL( pItem2 ) )
      hb_vmPushLogical( hb_vmPopLogical() != hb_vmPopLogical() );
   else if( HB_IS_OBJECT( pItem1 ) && hb_objHasMsg( pItem1, "__OpNotEqual" ) )
      hb_vmOperatorCall( pItem1, pItem2, "__OPNOTEQUAL" );
   else if( pItem1->type != pItem2->type ||
            ( HB_IS_BLOCK( pItem1 ) && HB_IS_BLOCK( pItem2 ) ) ||
            ( HB_IS_ARRAY( pItem1 ) && HB_IS_ARRAY( pItem2 ) ) )
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1072, NULL, "<>", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }
   }
   else
   {
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( TRUE );
   }
}

static void hb_vmLess( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmLess()"));

   if( HB_IS_STRING( hb_stackItemFromTop( -2 ) ) && HB_IS_STRING( hb_stackItemFromTop( -1 ) ) )
   {
      int i = hb_itemStrCmp( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ), FALSE );
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( i < 0 );
   }
   else if( HB_IS_NUMERIC( hb_stackItemFromTop( - 1  ) ) && HB_IS_NUMERIC( hb_stackItemFromTop( -2 ) ) )
   {
      double dNumber2 = hb_vmPopNumber();
      double dNumber1 = hb_vmPopNumber();
      hb_vmPushLogical( dNumber1 < dNumber2 );
   }
   else if( HB_IS_LOGICAL( hb_stackItemFromTop( -1 ) ) && HB_IS_LOGICAL( hb_stackItemFromTop( -2 ) ) )
   {
      BOOL bLogical2 = hb_vmPopLogical();
      BOOL bLogical1 = hb_vmPopLogical();
      hb_vmPushLogical( bLogical1 < bLogical2 );
   }
   else if( HB_IS_OBJECT( hb_stackItemFromTop( -2 ) ) && hb_objHasMsg( hb_stackItemFromTop( -2 ), "__OpLess" ) )
      hb_vmOperatorCall( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ), "__OPLESS" );
   else
   {
      PHB_ITEM pItem2 = hb_stackItemFromTop( -1 );
      PHB_ITEM pItem1 = hb_stackItemFromTop( -2 );
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1073, NULL, "<", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmLessEqual( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmLessEqual()"));

   if( HB_IS_STRING( hb_stackItemFromTop( -2 ) ) && HB_IS_STRING( hb_stackItemFromTop( -1 ) ) )
   {
      int i = hb_itemStrCmp( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ), FALSE );
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( i <= 0 );
   }
   else if( HB_IS_NUMERIC( hb_stackItemFromTop( -1 ) ) && HB_IS_NUMERIC( hb_stackItemFromTop( -2 ) ) )
   {
      double dNumber2 = hb_vmPopNumber();
      double dNumber1 = hb_vmPopNumber();
      hb_vmPushLogical( dNumber1 <= dNumber2 );
   }
   else if( HB_IS_LOGICAL( hb_stackItemFromTop( -1 ) ) && HB_IS_LOGICAL( hb_stackItemFromTop( -2 ) ) )
   {
      BOOL bLogical2 = hb_vmPopLogical();
      BOOL bLogical1 = hb_vmPopLogical();
      hb_vmPushLogical( bLogical1 <= bLogical2 );
   }
   else if( HB_IS_OBJECT( hb_stackItemFromTop( -2 ) ) && hb_objHasMsg( hb_stackItemFromTop( -2 ), "__OpLessEqual" ) )
      hb_vmOperatorCall( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ), "__OPLESSEQUAL" );
   else
   {
      PHB_ITEM pItem2 = hb_stackItemFromTop( -1 );
      PHB_ITEM pItem1 = hb_stackItemFromTop( -2 );
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1074, NULL, "<=", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmGreater( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmGreater()"));

   if( HB_IS_STRING( hb_stackItemFromTop( -2 ) ) && HB_IS_STRING( hb_stackItemFromTop( -1 ) ) )
   {
      int i = hb_itemStrCmp( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ), FALSE );
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( i > 0 );
   }
   else if( HB_IS_NUMERIC( hb_stackItemFromTop( -1 ) ) && HB_IS_NUMERIC( hb_stackItemFromTop( -2 ) ) )
   {
      double dNumber2 = hb_vmPopNumber();
      double dNumber1 = hb_vmPopNumber();
      hb_vmPushLogical( dNumber1 > dNumber2 );
   }
   else if( HB_IS_LOGICAL( hb_stackItemFromTop( -1 ) ) && HB_IS_LOGICAL( hb_stackItemFromTop( -2 ) ) )
   {
      BOOL bLogical2 = hb_vmPopLogical();
      BOOL bLogical1 = hb_vmPopLogical();
      hb_vmPushLogical( bLogical1 > bLogical2 );
   }
   else if( HB_IS_OBJECT( hb_stackItemFromTop( -2 ) ) && hb_objHasMsg( hb_stackItemFromTop( -2 ), "__OpGreater" ) )
      hb_vmOperatorCall( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ), "__OPGREATER" );
   else
   {
      PHB_ITEM pItem2 = hb_stackItemFromTop( -1 );
      PHB_ITEM pItem1 = hb_stackItemFromTop( -2 );
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1075, NULL, ">", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmGreaterEqual( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmGreaterEqual()"));

   if( HB_IS_STRING( hb_stackItemFromTop( -2 ) ) && HB_IS_STRING( hb_stackItemFromTop( -1 ) ) )
   {
      int i = hb_itemStrCmp( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ), FALSE );
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( i >= 0 );
   }
   else if( HB_IS_NUMERIC( hb_stackItemFromTop( -1 ) ) && HB_IS_NUMERIC( hb_stackItemFromTop( -2 ) ) )
   {
      double dNumber2 = hb_vmPopNumber();
      double dNumber1 = hb_vmPopNumber();
      hb_vmPushLogical( dNumber1 >= dNumber2 );
   }
   else if( HB_IS_LOGICAL( hb_stackItemFromTop( -1 ) ) && HB_IS_LOGICAL( hb_stackItemFromTop( -2 ) ) )
   {
      BOOL bLogical2 = hb_vmPopLogical();
      BOOL bLogical1 = hb_vmPopLogical();
      hb_vmPushLogical( bLogical1 >= bLogical2 );
   }
   else if( HB_IS_OBJECT( hb_stackItemFromTop( -2 ) ) && hb_objHasMsg( hb_stackItemFromTop( -2 ), "__OpGreaterEqual" ) )
      hb_vmOperatorCall( hb_stackItemFromTop( -2 ), hb_stackItemFromTop( -1 ), "__OPGREATEREQUAL" );
   else
   {
      PHB_ITEM pItem2 = hb_stackItemFromTop( -1 );
      PHB_ITEM pItem1 = hb_stackItemFromTop( -2 );
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1076, NULL, ">=", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmInstringOrArray( void )
{
   PHB_ITEM pItem1;
   PHB_ITEM pItem2;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmInstring()"));

   pItem1 = hb_stackItemFromTop( -2 );
   pItem2 = hb_stackItemFromTop( -1 );

   if( HB_IS_STRING( pItem1 ) && HB_IS_STRING( pItem2 ) )
   {
      BOOL bResult = ( hb_strAt( pItem1->item.asString.value, pItem1->item.asString.length,
                                 pItem2->item.asString.value, pItem2->item.asString.length ) != 0 );
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( bResult );
   }
   else if( HB_IS_OBJECT( pItem1 ) && hb_objHasMsg( pItem1, "__OpInstring" ) )
   {
      hb_vmOperatorCall( pItem1, pItem2, "__OPINSTRING" );
   }
   else if( HB_IS_ARRAY( pItem2 ) )
   {
      BOOL bResult = hb_arrayScan( pItem2, pItem1, NULL, NULL );

      hb_stackPop();
      hb_stackPop();

      hb_vmPushLogical( bResult );
   }
   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1109, NULL, "$", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }
   }
}

/* At this moment the eval stack should store:
 * -3 -> <current counter value>
 * -2 -> <end value>
 * -1 -> <step value>
 */
static void hb_vmForTest( void )        /* Test to check the end point of the FOR */
{
   double dStep;
   double dEnd;
   double dCurrent;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmForTest()"));

   while( ! HB_IS_NUMERIC( hb_stackItemFromTop( -1 ) ) )
   {
      PHB_ITEM pItem1 = hb_stackItemFromTop( -1 );
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1073, NULL, "<", 1, pItem1 );

      if( pResult )
      {
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }
      else
         /* NOTE: Return from the inside. */
         return;
   }

   dStep = hb_vmPopNumber();

   while( ! HB_IS_NUMERIC( hb_stackItemFromTop( -1 ) ) )
   {
      PHB_ITEM pItem1 = hb_stackItemFromTop( -1 );
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1073, NULL, "<", 1, pItem1 );

      if( pResult )
      {
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }
      else
         /* NOTE: Return from the inside. */
         return;
   }

   dEnd = hb_vmPopNumber();

   while( ! HB_IS_NUMERIC( hb_stackItemFromTop( -1 ) ) )
   {
      PHB_ITEM pItem1 = hb_stackItemFromTop( -1 );
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1073, NULL, "<", 1, pItem1 );

      if( pResult )
      {
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }
      else
         /* NOTE: Return from the inside. */
         return;
   }

   dCurrent = hb_vmPopNumber();

   if( dStep >= 0 )          /* Positive loop. Use LESS */
   {
      hb_vmPushLogical( dCurrent <= dEnd );
   }
   else if( dStep < 0 )      /* Negative loop. Use GREATER */
   {
      hb_vmPushLogical( dCurrent >= dEnd );
   }
}

/* ------------------------------- */
/* Operators (logical)             */
/* ------------------------------- */

static void hb_vmNot( void )
{
   PHB_ITEM pItem;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmNot()"));

   pItem = hb_stackItemFromTop( -1 );

   if( HB_IS_LOGICAL( pItem ) )
   {
      pItem->item.asLogical.value = ! pItem->item.asLogical.value;
   }
   else if( HB_IS_OBJECT( pItem ) && hb_objHasMsg( pItem, "__OpNot" ) )
   {
      hb_vmOperatorCallUnary( pItem, "__OPNOT" );
   }
   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1077, NULL, ".NOT.", 1, pItem );

      if( pResult )
      {
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmAnd( void )
{
   PHB_ITEM pItem2;
   PHB_ITEM pItem1;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmAnd()"));

   pItem2 = hb_stackItemFromTop( -1 );
   pItem1 = hb_stackItemFromTop( -2 );

   if( HB_IS_LOGICAL( pItem1 ) && HB_IS_LOGICAL( pItem2 ) )
   {
      BOOL bResult = pItem1->item.asLogical.value && pItem2->item.asLogical.value;
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( bResult );
   }
   else if( HB_IS_OBJECT( pItem1 ) && hb_objHasMsg( pItem1, "__OpAnd" ) )
      hb_vmOperatorCall( pItem1, pItem2, "__OPAND" );
   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1078, NULL, ".AND.", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }
   }
}

static void hb_vmOr( void )
{
   PHB_ITEM pItem2;
   PHB_ITEM pItem1;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmOr()"));

   pItem2 = hb_stackItemFromTop( -1 );
   pItem1 = hb_stackItemFromTop( -2 );

   if( HB_IS_LOGICAL( pItem1 ) && HB_IS_LOGICAL( pItem2 ) )
   {
      BOOL bResult = pItem1->item.asLogical.value || pItem2->item.asLogical.value;
      hb_stackPop();
      hb_stackPop();
      hb_vmPushLogical( bResult );
   }
   else if( HB_IS_OBJECT( pItem1 ) && hb_objHasMsg( pItem1, "__OpOr" ) )
      hb_vmOperatorCall( pItem1, pItem2, "__OPOR" );
   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1079, NULL, ".OR.", 2, pItem1, pItem2 );

      if( pResult )
      {
         hb_stackPop();
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }
   }
}

/* ------------------------------- */
/* Array                           */
/* ------------------------------- */

static void hb_vmArrayPush( void )
{
   PHB_ITEM pIndex;
   PHB_ITEM pArray;
   long     lIndex;

 #ifndef HB_C52_STRICT
   BOOL bStrIndex = FALSE;
 #endif

   HB_TRACE(HB_TR_DEBUG, ("hb_vmArrayAt()"));

   pIndex = hb_stackItemFromTop( -1 );
   pArray = hb_stackItemFromTop( -2 );

   if( HB_IS_INTEGER( pIndex ) )
   {
      lIndex = ( long ) pIndex->item.asInteger.value;
   }
   else if( HB_IS_LONG( pIndex ) )
   {
      lIndex = ( long ) pIndex->item.asLong.value;
   }
   else if( HB_IS_DOUBLE( pIndex ) )
   {
      lIndex = ( long ) pIndex->item.asDouble.value;
   }
 #ifndef HB_C52_STRICT
   else if( HB_IS_STRING( pIndex ) && pIndex->item.asString.length == 1 )
   {
      lIndex = ( long ) pIndex->item.asString.value[0];
      bStrIndex = TRUE;
   }
 #endif
   else
   {
      PHB_ITEM pResult = hb_errRT_BASE_Subst( EG_ARG, 1068, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 2, pArray, pIndex );

      if( pResult )
      {
         hb_stackPop();
         hb_stackPop();
         hb_itemPushForward( pResult );
         hb_itemRelease( pResult );
      }

      return;
   }

   if( HB_IS_ARRAY( pArray ) )
   {
      #ifndef HB_C52_STRICT
         if( lIndex < 0 )
         {
            if( pArray->item.asArray.value->ulLen )
            {
               lIndex += ( pArray->item.asArray.value->ulLen + 1 );
            }
            // Empty array and -1 Index -> return NIL.
            else if( lIndex == -1 )
            {
               hb_stackPop();
               hb_itemClear( hb_stackItemFromTop( -1 ) );
               return;
            }
         }
      #endif

      if( lIndex > 0 && (ULONG) lIndex <= pArray->item.asArray.value->ulLen )
      {
         if( pArray->item.asArray.value->uiHolders > 1 )
         {
            /* this is a temporary copy of an array - we can overwrite
             * it with no problem
            */
            hb_arrayGet( pArray, (ULONG) lIndex, pArray );
            hb_stackPop();
         }
         else
         {
            /* this is a constant array { 1, 2, 3 } - we cannot use
             * the optimization here
            */
            HB_ITEM item;

            ( &item )->type = HB_IT_NIL;

            hb_arrayGet( pArray, lIndex, &item );

            hb_stackPop();

            hb_itemForwardValue( hb_stackItemFromTop( -1 ), &item );
         }
      }
      else
      {
         hb_errRT_BASE( EG_BOUND, 1132, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 2, pArray, pIndex );
      }
   }
 #ifndef HB_C52_STRICT
   else if( HB_IS_STRING( pArray ) )
   {
      if( lIndex > 0 )
      {
         lIndex--;
      }
      else if( lIndex < 0 )
      {
         if( pArray->item.asString.length )
         {
            lIndex += pArray->item.asString.length;
         }
         // Empty string and -1 Index -> return NIL.
         else if( lIndex == -1 )
         {
            hb_stackPop();
            hb_itemClear( hb_stackItemFromTop( -1 ) );
            return;
         }

         if( lIndex < 0 )
         {
            lIndex = 0;
         }
      }

      hb_stackPop();

      if( (ULONG) lIndex < pArray->item.asString.length )
      {
         if( pArray->item.asString.bStatic )
         {
            pArray->item.asString.value = hb_vm_acAscii[ (unsigned char) ( pArray->item.asString.value[lIndex] ) ];
         }
         else
         {
            unsigned char cChar = pArray->item.asString.value[lIndex];

            hb_itemReleaseString( pArray );

            pArray->item.asString.value   = hb_vm_acAscii[ cChar ];
            pArray->item.asString.bStatic = TRUE;
         }

         pArray->item.asString.length  = 1;
      }
      else if( bStrIndex )
      {
         hb_errRT_BASE( EG_ARG, 1068, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 2, pArray, pIndex );
      }
      else
      {
         if( ! pArray->item.asString.bStatic )
         {
            hb_itemReleaseString( pArray );
         }

         pArray->item.asString.value  = hb_vm_sNull;
         pArray->item.asString.length = 0;
         pArray->item.asString.bStatic = TRUE;
      }
   }
 #endif
   else
   {
      hb_errRT_BASE( EG_ARG, 1068, NULL, hb_langDGetErrorDesc( EG_ARRACCESS ), 2, pArray, pIndex );
   }
}

static void hb_vmArrayPop( void )
{
   PHB_ITEM pValue;
   PHB_ITEM pIndex;
   PHB_ITEM pArray;
   long     lIndex;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmArrayPop()"));

   pValue = hb_stackItemFromTop( -3 );
   pArray = hb_stackItemFromTop( -2 );
   pIndex = hb_stackItemFromTop( -1 );

   if( HB_IS_BYREF( pArray ) )
   {
      pArray = hb_itemUnRef( pArray );
   }

   if( HB_IS_INTEGER( pIndex ) )
   {
      lIndex = ( long ) pIndex->item.asInteger.value;
   }
   else if( HB_IS_LONG( pIndex ) )
   {
      lIndex = ( long ) pIndex->item.asLong.value;
   }
   else if( HB_IS_DOUBLE( pIndex ) )
   {
      lIndex = ( long ) pIndex->item.asDouble.value;
   }
   else if( HB_IS_STRING( pIndex ) && pIndex->item.asString.length == 1 )
   {
      lIndex = ( long ) pIndex->item.asString.value[0];
   }
   else
   {
      hb_errRT_BASE( EG_ARG, 1069, NULL, hb_langDGetErrorDesc( EG_ARRASSIGN ), 1, pIndex );
      return;
   }

   if( HB_IS_ARRAY( pArray ) )
   {
      #ifndef HB_C52_STRICT
         if( lIndex < 0 )
         {
            lIndex += ( pArray->item.asArray.value->ulLen + 1 );
         }
      #endif

      if( lIndex > 0 && (ULONG) lIndex <= pArray->item.asArray.value->ulLen )
      {
         /* Remove MEMOFLAG if exists (assignment from field). */
         pValue->type &= ~HB_IT_MEMOFLAG;

         hb_arraySet( pArray, (ULONG) lIndex, pValue );
         hb_stackPop();
         hb_stackPop();
         hb_stackPop();    /* remove the value from the stack just like other POP operations */
      }
      else
      {
         hb_errRT_BASE( EG_BOUND, 1133, NULL, hb_langDGetErrorDesc( EG_ARRASSIGN ), 1, pIndex );
      }
   }
 #ifndef HB_C52_STRICT
   // Only allowing assignment of strings (char) and numerics into String as Array.
   else if( HB_IS_STRING( pArray ) && ( HB_IS_STRING( pValue ) || HB_IS_NUMERIC( pValue ) ) )
   {
      if( lIndex > 0 )
      {
         lIndex--;
      }
      else if( lIndex < 0 )
      {
         lIndex += pArray->item.asString.length;
      }

      if( lIndex >= 0 && (ULONG) lIndex < pArray->item.asString.length )
      {
         BYTE bNewChar;

         //pArray = pArray->item.asString.pOrigin;

         if( pValue->type == HB_IT_STRING )
         {
            bNewChar = pValue->item.asString.value[0];
         }
         else if( pValue->type == HB_IT_INTEGER )
         {
            bNewChar = (BYTE) pValue->item.asInteger.value;
         }
         else if( pValue->type == HB_IT_LONG )
         {
            bNewChar = (BYTE) pValue->item.asLong.value;
         }
         else
         {
            bNewChar = (BYTE) pValue->item.asDouble.value;
         }

         if( pArray->item.asString.bStatic || *( pArray->item.asString.puiHolders ) > 1 )
         {
            char *sNew = (char *) hb_xgrab( pArray->item.asString.length + 1 );

            memcpy( sNew, pArray->item.asString.value, pArray->item.asString.length );
            sNew[ pArray->item.asString.length ] = '\0';

            if( ! pArray->item.asString.bStatic )
            {
               hb_itemReleaseString( pArray );
            }

            pArray->item.asString.value           =  sNew;
            pArray->item.asString.puiHolders      = (USHORT*) hb_xgrab( sizeof( USHORT ) );
            *( pArray->item.asString.puiHolders ) = 1;
            pArray->item.asString.bStatic         = FALSE;
         }

         pArray->item.asString.value[ lIndex ] = bNewChar;

         hb_stackPop();
         hb_stackPop();
         hb_stackPop();    /* remove the value from the stack just like other POP operations */
      }
      else
      {
         hb_errRT_BASE( EG_BOUND, 1133, NULL, hb_langDGetErrorDesc( EG_ARRASSIGN ), 3, pArray, pIndex, pValue );
      }
   }
 #endif
   else
   {
      hb_errRT_BASE( EG_ARG, 1069, NULL, hb_langDGetErrorDesc( EG_ARRASSIGN ), 3, pArray, pIndex, pValue );
   }
}

static void hb_vmArrayDim( USHORT uiDimensions ) /* generates an uiDimensions Array and initialize those dimensions from the stack values */
{
   HB_ITEM itArray;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmArrayDim(%hu)", uiDimensions));

   itArray.type = HB_IT_NIL;

   #ifdef HB_THREAD_SUPPORT
      if( hb_ht_context )
      {
         //hb_threadForbid( &hb_gcCollectionForbid );
         HB_CRITICAL_LOCK( hb_gcCollectionMutex );
      }
   #endif

   hb_vmArrayNew( &itArray, uiDimensions );

   while( uiDimensions-- )
   {
      hb_stackPop();
   }

   hb_itemForwardValue( ( * HB_VM_STACK.pPos ), &itArray );
   hb_stackPush();

   #ifdef HB_THREAD_SUPPORT
      if( hb_ht_context )
      {
         //hb_threadAllow( &hb_gcCollectionForbid );
         HB_CRITICAL_UNLOCK( hb_gcCollectionMutex );
      }
   #endif
}

static void hb_vmArrayGen( ULONG ulElements ) /* generates an ulElements Array and fills it from the stack values */
{
   HB_ITEM itArray;
   ULONG ulPos;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmArrayGen(%lu)", ulElements));

   #ifdef HB_THREAD_SUPPORT
      if( hb_ht_context )
      {
         //hb_threadForbid( &hb_gcCollectionForbid );
         HB_CRITICAL_LOCK( hb_gcCollectionMutex );
      }
   #endif

   itArray.type = HB_IT_NIL;
   hb_arrayNew( &itArray, ulElements );

   for( ulPos = 0; ulPos < ulElements; ulPos++ )
   {
      hb_itemForwardValue( itArray.item.asArray.value->pItems + ulPos, hb_stackItemFromTop( ulPos - ulElements ) );
   }

   /* Poping 1 less than element,so we can override the 1st element with the new array */
   for( ulPos = 1; ulPos < ulElements; ulPos++ )
   {
      hb_stackPop();
   }

   /* Override 1st element if there was one, or push... */
   if( ulElements )
   {
      hb_itemForwardValue( ( hb_stackItemFromTop( - 1 ) ), &itArray );
   }
   else
   {
      hb_itemForwardValue( ( * HB_VM_STACK.pPos ), &itArray );
      hb_stackPush();
   }

   #ifdef HB_THREAD_SUPPORT
      if( hb_ht_context )
      {
         //hb_threadAllow( &hb_gcCollectionForbid );
         HB_CRITICAL_UNLOCK( hb_gcCollectionMutex );
      }
   #endif
}

/* This function creates an array item using 'uiDimension' as an index
 * to retrieve the number of elements from the stack
 */
static void hb_vmArrayNew( HB_ITEM_PTR pArray, USHORT uiDimension )
{
   ULONG ulElements;
   HB_ITEM_PTR pDim;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmArrayNew(%p, %hu)", pArray, uiDimension));

   // HB_CRITICAL_LOCK() - Protected an ENTRY POINT hb_vmArrayDim()!

   pDim = hb_stackItemFromTop( - uiDimension );

   /* use the proper type of number of elements */
   switch( pDim->type & ~HB_IT_BYREF )
   {
      case HB_IT_INTEGER:
         ulElements = ( ULONG ) pDim->item.asInteger.value;
         break;

      case HB_IT_LONG:
         ulElements = pDim->item.asLong.value;
         break;

      case HB_IT_DOUBLE:
         ulElements = ( ULONG ) pDim->item.asDouble.value;
         break;

      default:
         /* NOTE: Clipper creates empty array if non-numeric value is
          * specified as dimension and stops further processing.
          * There is no runtime error generated.
          */
         ulElements = 0;
         break;
   }

   /* create an array */
   hb_arrayNew( pArray, ulElements );

   if( --uiDimension )
   {
      /* call self recursively to create next dimensions
       */
      while( ulElements )
      {
         hb_vmArrayNew( hb_arrayGetItemPtr( pArray, ulElements-- ), uiDimension );
      }
   }
}

/* ------------------------------- */
/* Object                          */
/* ------------------------------- */

#if 0
void hb_vmMessage( PHB_SYMB pSymMsg ) /* sends a message to an object */
{
   PHB_ITEM pItemMsg = hb_stackItemFromTop( -1 );

   HB_TRACE(HB_TR_DEBUG, ("hb_vmMessage(%p, %s)", pSymMsg, pSymMsg->szName));

   hb_itemForwardValue( ( * HB_VM_STACK.pPos ), pItemMsg ); /* moves the object forward */
   pItemMsg->type = HB_IT_SYMBOL;
   pItemMsg->item.asSymbol.value = pSymMsg;
   pItemMsg->item.asSymbol.stackbase = hb_stackTopOffset() - 1;
   hb_stackPush();
}
#endif

static void hb_vmOperatorCall( PHB_ITEM pObjItem, PHB_ITEM pMsgItem, char * szSymbol )
{
   /* NOTE: There is no need to test if specified symbol exists. It is checked
    * by the caller (if HB_IS_OBJECT() && HAS_METHOD() )
    */
   HB_ITEM ItemMsg;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmOperatorCall(%p, %p, %s)", pObjItem, pMsgItem, szSymbol));

   ItemMsg.type = HB_IT_SYMBOL;
   ItemMsg.item.asSymbol.value = hb_dynsymFind( szSymbol )->pSymbol;
   ItemMsg.item.asSymbol.stackbase = hb_stackTopOffset();

   hb_vmPush( &ItemMsg );
   hb_vmPush( pObjItem );                             /* Push object              */
   hb_vmPush( pMsgItem );                             /* Push argument            */

   hb_vmSend( 1 );

   /* pop passed arguments - only one here */
   hb_stackPop();               /* pMsgItem */

   /* Push return value on the stack
    * NOTE: for performance reason we don't pop the second argument.
    * We can replace the second argument with the return value.
    */
   hb_itemForwardValue( pObjItem, &(HB_VM_STACK.Return) );
}

static void hb_vmOperatorCallUnary( PHB_ITEM pObjItem, char * szSymbol )
{
   /* NOTE: There is no need to test if specified symbol exists. It is checked
    * by the caller (if HB_IS_OBJECT() && HAS_METHOD() )
    */
   HB_ITEM ItemMsg;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmOperatorCallUnary(%p, %s)", pObjItem, szSymbol));

   ItemMsg.type = HB_IT_SYMBOL;
   ItemMsg.item.asSymbol.value = hb_dynsymFind( szSymbol )->pSymbol;
   ItemMsg.item.asSymbol.stackbase = hb_stackTopOffset();

   hb_vmPush( &ItemMsg );
   hb_vmPush( pObjItem );                             /* Push object */

   hb_vmSend( 0 );

   /* Pop passed argument.
    * NOTE: for performance reason we don't pop it and we don't push the
    * return value. We can replace the last element with the new value.
    */
   hb_itemForwardValue( pObjItem, &(HB_VM_STACK.Return) );
}

/* ------------------------------- */
/* Database                        */
/* ------------------------------- */

static ERRCODE hb_vmSelectWorkarea( PHB_ITEM pAlias )
{
   ERRCODE bSuccess = SUCCESS;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmSelectWorkArea(%p)", pAlias));

   /* NOTE: Clipper doesn't generate an error if an workarea specified
    * as numeric value cannot be selected
    */
   switch( pAlias->type & ~HB_IT_BYREF )
   {
      case HB_IT_INTEGER:
         /* Alias was used as integer value, for example: 4->field
          * or it was saved on the stack using hb_vmPushAlias()
          * or was evaluated from an expression, (nWorkArea)->field
          */
         hb_rddSelectWorkAreaNumber( pAlias->item.asInteger.value );
         pAlias->type = HB_IT_NIL;
         break;

      case HB_IT_LONG:
         /* Alias was evaluated from an expression, (nWorkArea)->field
          */
         hb_rddSelectWorkAreaNumber( pAlias->item.asLong.value );
         pAlias->type = HB_IT_NIL;
         break;

      case HB_IT_DOUBLE:
         /* Alias was evaluated from an expression, (nWorkArea)->field
          */
         hb_rddSelectWorkAreaNumber( ( int ) pAlias->item.asDouble.value );
         pAlias->type = HB_IT_NIL;
         break;

      case HB_IT_SYMBOL:
         /* Alias was specified using alias identifier, for example: al->field
          */
         bSuccess = hb_rddSelectWorkAreaSymbol( pAlias->item.asSymbol.value );
         pAlias->type = HB_IT_NIL;
         break;

      case HB_IT_STRING:
         /* Alias was evaluated from an expression, for example: (cVar)->field
          */
         {
         /* expand '&' operator if exists */
            char *cAlias;
            BOOL bNewString;

            cAlias = hb_macroExpandString( pAlias->item.asString.value, pAlias->item.asString.length, &bNewString );
            bSuccess = hb_rddSelectWorkAreaAlias( cAlias );

            if( bNewString )
            {
               hb_xfree( cAlias );
            }

            hb_itemClear( pAlias );
         }
         break;

      default:
         {
            PHB_ITEM pSubstVal = hb_errRT_BASE_Subst( EG_ARG, 1065, NULL, "&", 1, pAlias );

            if( pSubstVal )
            {
               bSuccess = hb_vmSelectWorkarea( pSubstVal );
            }
            else
            {
               bSuccess = FAILURE;
            }

            if( HB_IS_COMPLEX( pAlias ) )
            {
               hb_itemClear( pAlias );
            }
            else
            {
               pAlias->type = HB_IT_NIL;
            }
         }

         break;
   }

   return bSuccess;
}

/* Swaps two last items on the eval stack - the last item after swaping
 * is popped as current workarea number
 */
static void hb_vmSwapAlias( void )
{
   HB_ITEM_PTR pItem;
   HB_ITEM_PTR pWorkArea;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmSwapAlias()"));

   pItem = hb_stackItemFromTop( -1 );
   pWorkArea = hb_stackItemFromTop( -2 );
   hb_vmSelectWorkarea( pWorkArea );

   memcpy( pWorkArea, pItem, sizeof( HB_ITEM ) );
   pItem->type = HB_IT_NIL;
   hb_stackDec();
}

/* ------------------------------- */
/* Execution                       */
/* ------------------------------- */

void hb_vmDo( USHORT uiParams )
{
   PHB_ITEM pItem;
   PHB_SYMB pSym;
   HB_STACK_STATE sStackState;
   PHB_ITEM pSelf;
   PHB_FUNC pFunc;
   BOOL     bDebugPrevState;
   ULONG    ulClock = 0;
   BOOL     bProfiler = hb_bProfiler; /* because profiler state may change */
   int      iPresetBase = s_iBaseLine;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmDo(%hu)", uiParams));

   //printf( "\VmDo nItems: %i Params: %i Extra %i\n", HB_VM_STACK.pPos - HB_VM_STACK.pBase, uiParams, hb_vm_aiExtraParams[hb_vm_iExtraParamsIndex - 1] );

   if( hb_vm_iExtraParamsIndex && HB_IS_SYMBOL( pItem = hb_stackItemFromTop( -( uiParams + hb_vm_aiExtraParams[hb_vm_iExtraParamsIndex - 1] + 2 ) ) ) && pItem->item.asSymbol.value == hb_vm_apExtraParamsSymbol[hb_vm_iExtraParamsIndex - 1] )
   {
      uiParams += hb_vm_aiExtraParams[--hb_vm_iExtraParamsIndex];
   }

   if( hb_stackItemFromTop( - ( uiParams + 1 ) )->type )
   {
      hb_vmSend( uiParams );
      return;
   }

   if( bProfiler )
   {
      ulClock = ( ULONG ) clock();
   }

   //hb_inkeyPoll();           /* Poll the console keyboard */

   HB_TRACE( HB_TR_INFO, ( "StackNewFrame %hu", uiParams ) );

   pItem = hb_stackNewFrame( &sStackState, uiParams );
   pSym = pItem->item.asSymbol.value;
   pSelf = hb_stackSelfItem();   /* NIL, OBJECT or BLOCK */
   bDebugPrevState = s_bDebugging;
   s_bDebugging = FALSE;
   HB_TRACE( HB_TR_INFO, ( "Symbol: '%s'", pSym->szName ) );

   if( HB_IS_NIL( pSelf ) ) /* are we sending a message ? */
   {
      pFunc = pSym->pFunPtr;

      if( pFunc )
      {
         if( bProfiler && pSym->pDynSym )
         {
            pSym->pDynSym->ulRecurse++;
         }

         if ( hb_bTracePrgCalls )
         {
            HB_TRACE(HB_TR_ALWAYS, ("Calling: %s", pSym->szName));
         }

         HB_TRACE( HB_TR_DEBUG, ("Calling: %s", pSym->szName));

         pFunc();

         HB_TRACE( HB_TR_DEBUG, ("Done: %s", pSym->szName));

         if( bProfiler && pSym->pDynSym )
         {
            pSym->pDynSym->ulCalls++;                   /* profiler support */

            /* Time spent has to be added only inside topmost call of a recursive function */
            if( pSym->pDynSym->ulRecurse == 1 )
            {
               pSym->pDynSym->ulTime += clock() - ulClock; /* profiler support */
            }
            pSym->pDynSym->ulRecurse--;
         }
      }
      else
      {
         /* Attempt to call an undefined function
          *  - generate unrecoverable runtime error
          */
         PHB_ITEM pArgsArray = hb_arrayFromStack( uiParams );

         hb_errRT_BASE_SubstR( EG_NOFUNC, 1001, NULL, pSym->szName, 1, pArgsArray );
         hb_itemRelease( pArgsArray );
      }
   }
   else
   {
      HB_TRACE( HB_TR_ERROR, ( "hb_vmDo() internal logic error, Symbol: '%s' Fun: %p, Self Type: %i", pSym->szName, pSym->pFunPtr, pSelf->type ) );
      fprintf( stderr, "Error! hb_vmDo() internal logic failure, Symbol: '%s' Fun: %p, Self Type: %i", pSym->szName, pSym->pFunPtr, pSelf->type );
      exit(1);
   }

   HB_TRACE(HB_TR_DEBUG, ("DONE hb_vmDo(%hu)", uiParams));

   hb_stackOldFrame( &sStackState );

   HB_TRACE(HB_TR_DEBUG, ("Restored OldFrame hb_vmDo(%hu)", uiParams));

   if( s_bDebugging )
   {
      hb_vmDebuggerEndProc();
   }

   s_bDebugging = bDebugPrevState;

   s_iBaseLine = iPresetBase;
}

void hb_vmSend( USHORT uiParams )
{
   PHB_ITEM       pItem;
   PHB_SYMB       pSym;
   HB_STACK_STATE sStackState;
   PHB_ITEM       pSelf;
   PHB_FUNC       pFunc = NULL;
   BOOL           bDebugPrevState;
   ULONG          ulClock = 0;
   void           *pMethod = NULL;
   BOOL           bProfiler = hb_bProfiler; /* because profiler state may change */
   PHB_BASEARRAY  pSelfBase = NULL;
   BOOL           lPopSuper = FALSE;
   int            iPresetBase = s_iBaseLine;
   BOOL           bConstructor = FALSE;

   HB_TRACE_STEALTH( HB_TR_DEBUG, ( "hb_vmSend(%hu)", uiParams ) );

   if( HB_IS_COMPLEX( &(HB_VM_STACK.Return) ) )
   {
      hb_itemClear( &(HB_VM_STACK.Return) );
   }
   else
   {
      ( &(HB_VM_STACK.Return) )->type = HB_IT_NIL;
   }

   //printf( "\n VmSend nItems: %i Params: %i Extra %i\n", HB_VM_STACK.pPos - HB_VM_STACK.pBase, uiParams, hb_vm_aiExtraParams[hb_vm_iExtraParamsIndex - 1] );

   if( hb_vm_iExtraParamsIndex && HB_IS_SYMBOL( pItem = hb_stackItemFromTop( -( uiParams + hb_vm_aiExtraParams[hb_vm_iExtraParamsIndex - 1] + 2 ) ) ) && pItem->item.asSymbol.value == hb_vm_apExtraParamsSymbol[hb_vm_iExtraParamsIndex - 1] )
   {
      uiParams += hb_vm_aiExtraParams[--hb_vm_iExtraParamsIndex];
   }

   if( bProfiler )
   {
      ulClock = ( ULONG ) clock();
   }

   pItem = hb_stackNewFrame( &sStackState, uiParams );   /* procedure name */
   pSym = pItem->item.asSymbol.value;

   if( hb_vm_bWithObject )
   {
      hb_vm_bWithObject = FALSE;
      hb_itemCopy( * ( HB_VM_STACK.pBase + 1 ), &( hb_vm_aWithObject[ hb_vm_wWithObjectCounter - 1 ] ) ) ;
   }

   pSelf = ( * ( HB_VM_STACK.pBase + 1 ) );   /* NIL, OBJECT or BLOCK */

   bDebugPrevState = s_bDebugging;
   s_bDebugging = FALSE;

   //printf( "Symbol: '%s'\n", pSym->szName );

   if( HB_IS_BYREF( pSelf ) )
   {
      pSelf = hb_itemUnRef( pSelf );
   }

   if( HB_IS_BLOCK( pSelf ) )
   {
      if( pSym == &( hb_symEval ) )
      {
         pFunc = pSym->pFunPtr;                 /* __EVAL method = function */
      }
      else if( strncmp( pSym->szName, "EVAL", 4 ) == 0 )
      {
         pSym = &hb_symEval;
         pFunc = pSym->pFunPtr;                 /* __EVAL method = function */
      }
   }
   else if( HB_IS_OBJECT( pSelf ) )               /* Object passed            */
   {
      lPopSuper = FALSE;
      pFunc     = hb_objGetMthd( pSelf, pSym, TRUE, &bConstructor );
      pSelfBase = pSelf->item.asArray.value;

      if( pSelfBase->uiPrevCls ) /* Is is a Super cast ? */
      {
         PHB_ITEM pRealSelf;
         USHORT nPos;
         USHORT uiClass;

         //printf( "\n VmSend Method: %s \n", pSym->szName );
         uiClass = pSelfBase->uiClass;

         pRealSelf = hb_itemNew( NULL ) ;
         hb_itemCopy( pRealSelf, pSelf->item.asArray.value->pItems ) ;  // hb_arrayGetItemPtr(pSelf,1) ;
         /* and take back the good pSelfBase */
         pSelfBase = pRealSelf->item.asArray.value;
         /* Now I should exchnage it with the current stacked value */
         hb_itemSwap( pSelf, pRealSelf );
         hb_itemRelease( pRealSelf ) ; /* and release the fake one */

         /* Push current SuperClass handle */
         lPopSuper = TRUE ;

         if( ! pSelf->item.asArray.value->puiClsTree )
         {
            pSelf->item.asArray.value->puiClsTree   = ( USHORT * ) hb_xgrab( sizeof( USHORT ) );
            pSelf->item.asArray.value->puiClsTree[0]=0;
         }

         nPos=pSelfBase->puiClsTree[0]+1;
         pSelfBase->puiClsTree = ( USHORT * ) hb_xrealloc( pSelfBase->puiClsTree, sizeof( USHORT ) * (nPos+1) ) ;
         pSelfBase->puiClsTree[0] = nPos ;
         pSelfBase->puiClsTree[ nPos ] = uiClass;
      }
   }

   if( pFunc )
   {
      if( bProfiler )
      {
         pMethod = hb_mthRequested();
      }

      if ( hb_bTracePrgCalls )
      {
         HB_TRACE(HB_TR_ALWAYS, ("Calling: %s", pSym->szName));
      }

      HB_TRACE( HB_TR_DEBUG, ("Calling: %s", pSym->szName));

      pFunc();

      HB_TRACE( HB_TR_DEBUG, ("Done: %s", pSym->szName));

      if ( ( pSym != &hb_symEval ) && lPopSuper && pSelfBase->puiClsTree )
      {
         USHORT nPos = pSelfBase->puiClsTree[0] - 1;

         /* POP SuperClass handle */
         if( nPos )
         {
            pSelfBase->puiClsTree = ( USHORT * ) hb_xrealloc( pSelfBase->puiClsTree, sizeof( USHORT ) * (nPos + 1) );
            pSelfBase->puiClsTree[0]=nPos;
         }
         else
         {
            hb_xfree(pSelfBase->puiClsTree);
            pSelfBase->puiClsTree = NULL ;
         }
      }

      if( bProfiler )
      {
         hb_mthAddTime( pMethod, clock() - ulClock );
      }

      // Constructor must ALWAYS return Self.
      if( bConstructor )
      {
         hb_itemForwardValue( &(HB_VM_STACK.Return), pSelf );
      }
   }
   else
   {
      char *sClass = hb_objGetClsName( pSelf );

      if( strncmp( pSym->szName, "CLASSNAME", strlen( pSym->szName ) < 4 ? 4 : strlen( pSym->szName ) ) == 0 )
      {
         hb_itemPutC( &(HB_VM_STACK.Return), sClass );
      }
      else if( strncmp( pSym->szName, "CLASSH", 6 ) == 0 )
      {
         hb_itemPutNI( &(HB_VM_STACK.Return), 0 );
      }
      else
      {
         char sDesc[128];

         if( pSym->szName[ 0 ] == '_' )
         {
            PHB_ITEM pArgsArray = hb_arrayFromStack( uiParams );

            sprintf( (char *) sDesc, "Class: '%s' has no property", sClass );
            hb_errRT_BASE_SubstR( EG_NOVARMETHOD, 1005, (char *) sDesc, pSym->szName + 1, 1, pArgsArray );
            hb_itemRelease( pArgsArray );
         }
         else
         {
            PHB_ITEM pArgsArray = hb_arrayFromStack( uiParams );

            sprintf( (char *) sDesc, "Class: '%s' has no exported method", sClass );
            hb_errRT_BASE_SubstR( EG_NOMETHOD, 1004, (char *) sDesc, pSym->szName, 1, pArgsArray );
            hb_itemRelease( pArgsArray );
         }
      }
   }

   HB_TRACE(HB_TR_DEBUG, ("Done hb_vmSend()"));

   hb_stackOldFrame( &sStackState );

   HB_TRACE(HB_TR_DEBUG, ("Restored Stack hb_vmSend()"));

   if( s_bDebugging )
   {
      hb_vmDebuggerEndProc();
   }

   s_bDebugging = bDebugPrevState;

   s_iBaseLine = iPresetBase;
}

static HARBOUR hb_vmDoBlock( void )
{
   PHB_ITEM pBlock;
   USHORT uiLine;
   int iParam;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmDoBlock()"));

   pBlock = hb_stackSelfItem();

   if( HB_IS_BYREF( pBlock ) )
   {
      pBlock = hb_itemUnRef( pBlock );
   }

   if( ! HB_IS_BLOCK( pBlock ) )
   {
      hb_errInternal( HB_EI_VMNOTCBLOCK, NULL, "hb_vmDoBlock()", NULL );
   }

   /* Check for valid count of parameters */
   iParam = pBlock->item.asBlock.paramcnt - hb_pcount();

   /* add missing parameters */
   while( iParam-- > 0 )
   {
      hb_vmPushNil();
   }

   /* set the current line number to a line where the codeblock was defined
    */
   uiLine = hb_stackBaseItem()->item.asSymbol.lineno;
   hb_stackBaseItem()->item.asSymbol.lineno = pBlock->item.asBlock.lineno;

   hb_codeblockEvaluate( pBlock );

   HB_TRACE(HB_TR_DEBUG, ("Done hb_vmDoBlock()"));

   /* restore stack pointers */
   hb_stackBaseItem()->item.asSymbol.lineno = uiLine;

   HB_TRACE(HB_TR_DEBUG, ("Restored Satck hb_vmDoBlock()"));
}

/* Evaluates a passed codeblock item with no arguments passed to a codeblock
*/
HB_ITEM_PTR hb_vmEvalBlock( HB_ITEM_PTR pBlock )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmEvalBlock(%p)", pBlock));

   hb_vmPushSymbol( &hb_symEval );
   hb_vmPush( pBlock );
   hb_vmSend( 0 );
   return &(HB_VM_STACK.Return);
}

/* Evaluates a codeblock item using passed additional arguments
 * pBlock = an item of codeblock type to evaluate
 * ulArgCount = number of arguments passed to a codeblock
 * ... = the list of arguments of type PHB_ITEM
 *
 *for example:
 * retVal = hb_vmEvalBlockV( pBlock, 2, pParam1, pParam2 );
*/
HB_ITEM_PTR HB_EXPORT hb_vmEvalBlockV( HB_ITEM_PTR pBlock, ULONG ulArgCount, ... )
{
   va_list va;
   ULONG i;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmEvalBlockV(%p, %hu, ...)", pBlock, ulArgCount));

   hb_vmPushSymbol( &hb_symEval );
   hb_vmPush( pBlock );

   va_start( va, ulArgCount );
   for( i = 1; i <= ulArgCount; i++ )
   {
      hb_vmPush( va_arg( va, PHB_ITEM ) );
   }
   va_end( va );

   /* take care here, possible loss of data long to short ... */
   /* added an explicit casting here for VC++ JFL */
   hb_vmSend( (USHORT) ulArgCount );

   return &(HB_VM_STACK.Return);
}

void hb_vmFunction( USHORT uiParams )
{
   HB_TRACE( HB_TR_DEBUG, ("hb_vmFunction(%hu)", uiParams ) );

   if( hb_stackItemFromTop( - ( uiParams + 1 ) )->type )
   {
      hb_vmSend( uiParams );
   }
   else
   {
      if( HB_IS_COMPLEX( &(HB_VM_STACK.Return) ) )
      {
         hb_itemClear( &(HB_VM_STACK.Return) );
      }
      else
      {
         ( &(HB_VM_STACK.Return) )->type = HB_IT_NIL;
      }

      hb_vmDo( uiParams );
   }

   /*
    ***
    * TODO: This should be changed to hb_itemForwardValue()
    * This is here to protect against ill behaved FWH code, which uses HB_VM_STACK.Return
    * after calling vmFunction().
    ***
    */
   hb_itemCopy( ( * HB_VM_STACK.pPos ), &(HB_VM_STACK.Return) );
   hb_stackPush();
}

static void hb_vmLocalName( USHORT uiLocal, char * szLocalName ) /* locals and parameters index and name information for the debugger */
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmLocalName(%hu, %s)", uiLocal, szLocalName));

   s_bDebugging = TRUE;
   s_bDebugShowLines = FALSE;
   hb_vmPushSymbol( hb_dynsymFind( "__DBGENTRY" )->pSymbol );
   hb_vmPushNil();
   hb_vmPushLongConst( uiLocal );
   hb_vmPushString( szLocalName, strlen( szLocalName ) );
   hb_vmPushLongConst( 2 ); /* 1 for statics, 2 for locals */
   s_bDebuggerIsWorking = TRUE;
   hb_vmDo( 3 );
   s_bDebuggerIsWorking = FALSE;
   s_bDebugShowLines = TRUE;
}

static void hb_vmStaticName( BYTE bIsGlobal, USHORT uiStatic, char * szStaticName ) /* statics vars information for the debugger */
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmStaticName(%hu, %s)", uiStatic, szStaticName));

   s_bDebugging = TRUE;
   s_bDebugShowLines = FALSE;
   hb_vmPushSymbol( hb_dynsymFind( "__DBGENTRY" )->pSymbol );
   hb_vmPushNil();

   if( bIsGlobal )
   {
      hb_vmPushLongConst( hb_arrayLen( &s_aStatics ) - s_uiStatics );
   }
   else
   {
      hb_vmPushLongConst( HB_VM_STACK.iStatics + uiStatic );
   }
   hb_vmPushString( szStaticName, strlen( szStaticName ) );
   hb_vmPushLongConst( 1 ); /* 1 for statics, 2 for locals */
   s_bDebuggerIsWorking = TRUE;
   hb_vmDo( 3 );
   s_bDebuggerIsWorking = FALSE;
   s_bDebugShowLines = TRUE;
}

static void hb_vmModuleName( char * szModuleName ) /* PRG and function name information for the debugger */
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmModuleName(%s)", szModuleName));

   s_bDebugging = TRUE;
   s_bDebugShowLines = FALSE;
   hb_vmPushSymbol( hb_dynsymFind( "__DBGENTRY" )->pSymbol );
   hb_vmPushNil();
   hb_vmPushString( szModuleName, strlen( szModuleName ) );
   s_bDebuggerIsWorking = TRUE;
   hb_vmDo( 1 );
   s_bDebuggerIsWorking = FALSE;
   s_bDebugShowLines = TRUE;
}

static void hb_vmFrame( BYTE bLocals, BYTE bParams )
{
   int iTotal, iExtra;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmFrame(%d, %d)", (int) bLocals, (int) bParams));

   if( bParams == 255 )
   {
      hb_stackBaseItem()->item.asSymbol.paramcnt += 256;

      while( bLocals-- > 0 )
      {
         hb_vmPushNil();
      }

      return;
   }

   iExtra = hb_pcount() - bParams;

   while( iExtra > 0 )
   {
      PHB_ITEM pExtra = hb_stackItemFromTop( -iExtra );

      if( HB_IS_COMPLEX( pExtra ) )
      {
         hb_itemClear( pExtra );
      }
      else
      {
         pExtra->type = HB_IT_NIL;
      }

      iExtra--;
   }

   iTotal = bLocals + bParams;
   if( iTotal )
   {
      int i = iTotal - hb_pcount();

      while( i-- > 0 )
      {
         hb_vmPushNil();
      }
   }
}

static void hb_vmSFrame( PHB_SYMB pSym )      /* sets the statics frame for a function */
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmSFrame(%p)", pSym));

   /* _INITSTATICS is now the statics frame. Statics() changed it! */
   HB_VM_STACK.iStatics = ( int ) pSym->pFunPtr; /* pSym is { "$_INITSTATICS", HB_FS_INIT | HB_FS_EXIT, _INITSTATICS } for each PRG */
}

static void hb_vmStatics( PHB_SYMB pSym, USHORT uiStatics ) /* initializes the global aStatics array or redimensionates it */
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmStatics(%p, %hu)", pSym, uiStatics));

   if( HB_IS_NIL( &s_aStatics ) )
   {
      pSym->pFunPtr = NULL;         /* statics frame for this PRG */
      hb_arrayNew( &s_aStatics, uiStatics );
   }
   else
   {
      pSym->pFunPtr = ( PHB_FUNC ) hb_arrayLen( &s_aStatics );
      hb_arraySize( &s_aStatics, hb_arrayLen( &s_aStatics ) + uiStatics );
   }

   s_uiStatics = uiStatics; /* We need s_uiStatics for processing hb_vmStaticName() */
}

static void hb_vmEndBlock( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmEndBlock()"));

   hb_stackDec();                               /* make the last item visible */
   hb_itemForwardValue( &(HB_VM_STACK.Return), ( * HB_VM_STACK.pPos ) ); /* Forward it */
}

static void hb_vmRetValue( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmRetValue()"));

   hb_stackDec();                               /* make the last item visible */

   hb_itemForwardValue( &(HB_VM_STACK.Return), ( * HB_VM_STACK.pPos ) ); /* Forward it */
}

static void hb_vmDebuggerEndProc( void )
{
   HB_ITEM item;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmDebuggerEndProc()"));

   ( &item )->type = HB_IT_NIL;

   hb_itemForwardValue( &item, &(HB_VM_STACK.Return) ); /* saves the previous returned value */

   s_bDebugShowLines = FALSE;
   hb_vmPushSymbol( hb_dynsymFind( "__DBGENTRY" )->pSymbol );
   hb_vmPushNil();
   s_bDebuggerIsWorking = TRUE;
   hb_vmDo( 0 );
   s_bDebuggerIsWorking = FALSE;
   s_bDebugShowLines = TRUE;

   hb_itemForwardValue( &(HB_VM_STACK.Return), &item ); /* restores the previous returned value */
}

static void hb_vmDebuggerShowLine( USHORT uiLine ) /* makes the debugger shows a specific source code line */
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmDebuggerShowLine(%hu)", uiLine));

   s_bDebugShowLines = FALSE;
   hb_vmPushSymbol( hb_dynsymFind( "__DBGENTRY" )->pSymbol );
   hb_vmPushNil();
   hb_vmPushInteger( uiLine );
   s_bDebuggerIsWorking = TRUE;
   hb_vmDo( 1 );
   s_bDebuggerIsWorking = FALSE;
   s_bDebugShowLines = TRUE;
}

/* ------------------------------- */
/* Push                            */
/* ------------------------------- */

void hb_vmPush( PHB_ITEM pItem )
{
   HB_TRACE_STEALTH( HB_TR_DEBUG, ( "hb_vmPush(%p) type: %i", pItem, pItem->type ) );

   hb_itemCopy( ( * HB_VM_STACK.pPos ), pItem );
   hb_stackPush();
}

void hb_vmPushNil( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushNil()"));

   ( * HB_VM_STACK.pPos )->type = HB_IT_NIL;
   hb_stackPush();
}

void hb_vmPushLogical( BOOL bValue )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushLogical(%d)", (int) bValue));

   ( * HB_VM_STACK.pPos )->type = HB_IT_LOGICAL;
   ( * HB_VM_STACK.pPos )->item.asLogical.value = bValue;
   hb_stackPush();
}

void hb_vmPushNumber( double dNumber, int iDec )
{
   hb_vmPushNumType( dNumber, iDec, 0, 0 );
}

void hb_vmPushNumType( double dNumber, int iDec, int iType1, int iType2 )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushNumber(%lf, %d)", dNumber, iDec));

   //printf( "%i\n", (int) dNumber );

   if( iDec || iType1 & HB_IT_DOUBLE || iType2 & HB_IT_DOUBLE )
   {
      hb_vmPushDouble( dNumber, iDec );
   }
   else if( SHRT_MIN <= dNumber && dNumber <= SHRT_MAX )
   {
      hb_vmPushInteger( ( int ) dNumber );
   }
   else if( LONG_MIN <= dNumber && dNumber <= LONG_MAX )
   {
      hb_vmPushLong( ( long ) dNumber );
   }
   else
   {
      hb_vmPushDouble( dNumber, hb_set.HB_SET_DECIMALS );
   }
}

void hb_vmPushInteger( int iNumber )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushInteger(%d)", iNumber));

   ( * HB_VM_STACK.pPos )->type = HB_IT_INTEGER;
   ( * HB_VM_STACK.pPos )->item.asInteger.value = iNumber;
   ( * HB_VM_STACK.pPos )->item.asInteger.length = 10;
   hb_stackPush();
}

void hb_vmPushLong( long lNumber )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushLong(%ld)", lNumber));

   ( * HB_VM_STACK.pPos )->type = HB_IT_LONG;
   ( * HB_VM_STACK.pPos )->item.asLong.value = lNumber;
   ( * HB_VM_STACK.pPos )->item.asLong.length = 10;
   hb_stackPush();
}

void hb_vmPushLongConst( long lNumber )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushLongConst(%ld)", lNumber));

   ( * HB_VM_STACK.pPos )->type = HB_IT_LONG;
   ( * HB_VM_STACK.pPos )->item.asLong.value = lNumber;

   if( lNumber >= 1000000000 )
      ( * HB_VM_STACK.pPos )->item.asLong.length = 11;
   else if( lNumber <= -1000000000 )
      ( * HB_VM_STACK.pPos )->item.asLong.length = 20;
   else
      ( * HB_VM_STACK.pPos )->item.asLong.length = 10;

   hb_stackPush();
}

void hb_vmPushDouble( double dNumber, int iDec )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushDouble(%lf, %d)", dNumber, iDec));

   ( * HB_VM_STACK.pPos )->type = HB_IT_DOUBLE;
   ( * HB_VM_STACK.pPos )->item.asDouble.value = dNumber;
   ( * HB_VM_STACK.pPos )->item.asDouble.length = ( dNumber >= 10000000000.0 || dNumber <= -1000000000.0 ) ? 20 : 10;
   if( iDec == HB_DEFAULT_DECIMALS )
      ( * HB_VM_STACK.pPos )->item.asDouble.decimal = hb_set.HB_SET_DECIMALS;
   else
      ( * HB_VM_STACK.pPos )->item.asDouble.decimal = iDec;

   hb_stackPush();
}

static int hb_vmCalcDoubleWidth( double dNumber, int iDec )
{
   char buffer[ 260 ];

   sprintf( buffer, "%.*f", 0, dNumber );

   if( iDec == 0 )
      return strlen( buffer ) + 1;
   else
      return strlen( buffer );
}

static void hb_vmPushDoubleConst( double dNumber, int iWidth, int iDec )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushDoubleConst(%lf, %d, %d)", dNumber, iWidth, iDec));

   ( * HB_VM_STACK.pPos )->type = HB_IT_DOUBLE;
   ( * HB_VM_STACK.pPos )->item.asDouble.value = dNumber;

   if( iDec == HB_DEFAULT_DECIMALS )
      ( * HB_VM_STACK.pPos )->item.asDouble.decimal = hb_set.HB_SET_DECIMALS;
   else
      ( * HB_VM_STACK.pPos )->item.asDouble.decimal = iDec;

   /* NOTE: Part of these width calculations could be moved to the compiler for
            maximum speed. */

   if( dNumber >= 1000000000.0 )
   {
      /* NOTE: If the width info is not provided by the pcode stream,
               it will be determined at runtime with a relatively slow
               method. [vszakats] */

      if( iWidth == HB_DEFAULT_WIDTH )
         ( * HB_VM_STACK.pPos )->item.asDouble.length = hb_vmCalcDoubleWidth( dNumber, iDec );
      else
         ( * HB_VM_STACK.pPos )->item.asDouble.length = iWidth;
   }
   else if( dNumber <= -1000000000.0 )
      ( * HB_VM_STACK.pPos )->item.asDouble.length = 20;
   else
      ( * HB_VM_STACK.pPos )->item.asDouble.length = 10;

   hb_stackPush();
}

void hb_vmPushDate( long lDate )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushDate(%ld)", lDate));

   ( * HB_VM_STACK.pPos )->type = HB_IT_DATE;
   ( * HB_VM_STACK.pPos )->item.asDate.value = lDate;
   hb_stackPush();
}

void hb_vmPushPointer( void * pPointer )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushPointer(%ld)", pPointer));

   ( * HB_VM_STACK.pPos )->type = HB_IT_POINTER;
   ( * HB_VM_STACK.pPos )->item.asPointer.value = pPointer;
   hb_stackPush();
}

void hb_vmPushString( char * szText, ULONG length )
{
   PHB_ITEM pTop = ( * HB_VM_STACK.pPos );
   char * szTemp;

   HB_TRACE( HB_TR_DEBUG, ( "hb_vmPushString( \"%s\", %lu ) %p", szText, length, pTop ) );

   szTemp = ( char * ) hb_xgrab( length + 1 );
   hb_xmemcpy( szTemp, szText, length );
   szTemp[ length ] = '\0';

   pTop->item.asString.puiHolders = (USHORT*) hb_xgrab( sizeof( USHORT ) );
   *( pTop->item.asString.puiHolders ) = 1;
   pTop->type = HB_IT_STRING;
   pTop->item.asString.bStatic = FALSE;
   pTop->item.asString.length = length;
   pTop->item.asString.value = szTemp;

   hb_stackPush();

   HB_TRACE( HB_TR_DEBUG, ( "Pushed String %p '%s'", hb_stackItemFromTop(-1), hb_stackItemFromTop(-1)->item.asString.value ) );
}

void hb_vmPushSymbol( PHB_SYMB pSym )
{
   HB_TRACE_STEALTH( HB_TR_DEBUG, ("hb_vmPushSymbol(%p) \"%s\"", pSym, pSym->szName ) );

   ( * HB_VM_STACK.pPos )->type = HB_IT_SYMBOL;
   ( * HB_VM_STACK.pPos )->item.asSymbol.value = pSym;
   ( * HB_VM_STACK.pPos )->item.asSymbol.stackbase = hb_stackTopOffset();

   hb_stackPush();
}

/* +0    -> HB_P_PUSHBLOCK
 * +1 +2 -> size of codeblock
 * +3 +4 -> number of expected parameters
 * +5 +6 -> number of referenced local variables
 * +7    -> start of table with referenced local variables
 *
 * NOTE: pCode points to static memory
 */
static void hb_vmPushBlock( BYTE * pCode, PHB_SYMB pSymbols, PHB_ITEM** pGlobals )
{
   USHORT uiLocals;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushBlock(%p, %p)", pCode, pSymbols));

   ( * HB_VM_STACK.pPos )->type = HB_IT_BLOCK;

   #ifdef HB_THREAD_SUPPORT
      if( hb_ht_context )
      {
         //hb_threadForbid( &hb_gcCollectionForbid );
         HB_CRITICAL_LOCK( hb_gcCollectionMutex );
      }
   #endif

   uiLocals = pCode[ 5 ] + ( pCode[ 6 ] * 256 );
   ( * HB_VM_STACK.pPos )->item.asBlock.value =
         hb_codeblockNew( pCode + 7 + uiLocals * 2, /* pcode buffer         */
         uiLocals,                                  /* number of referenced local variables */
         ( USHORT * ) ( pCode + 7 ),                /* table with referenced local variables */
         pSymbols, pGlobals );

   /* store the statics base of function where the codeblock was defined
    */
   ( * HB_VM_STACK.pPos )->item.asBlock.statics = HB_VM_STACK.iStatics;
   /* store the number of expected parameters
    */
   ( * HB_VM_STACK.pPos )->item.asBlock.paramcnt = pCode[ 3 ] + ( pCode[ 4 ] * 256 );
   /* store the line number where the codeblock was defined
    */
   ( * HB_VM_STACK.pPos )->item.asBlock.lineno = hb_stackBaseItem()->item.asSymbol.lineno;
   hb_stackPush();

   #ifdef HB_THREAD_SUPPORT
      if( hb_ht_context )
      {
         //hb_threadAllow( &hb_gcCollectionForbid );
         HB_CRITICAL_UNLOCK( hb_gcCollectionMutex );
      }
   #endif
}

/* +0    -> HB_P_PUSHBLOCKSHORT
 * +1    -> size of codeblock
 * +2    -> start of table with referenced local variables
 *
 * NOTE: pCode points to static memory
 */
static void hb_vmPushBlockShort( BYTE * pCode, PHB_SYMB pSymbols, PHB_ITEM** pGlobals )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushBlockShort(%p, %p)", pCode, pSymbols));

   ( * HB_VM_STACK.pPos )->type = HB_IT_BLOCK;

   #ifdef HB_THREAD_SUPPORT
      if( hb_ht_context )
      {
         //hb_threadForbid( &hb_gcCollectionForbid );
         HB_CRITICAL_LOCK( hb_gcCollectionMutex );
      }
   #endif

   ( * HB_VM_STACK.pPos )->item.asBlock.value =
         hb_codeblockNew( pCode + 2,                /* pcode buffer         */
         0,                                         /* number of referenced local variables */
         NULL,                                      /* table with referenced local variables */
         pSymbols, pGlobals );

   /* store the statics base of function where the codeblock was defined
    */
   ( * HB_VM_STACK.pPos )->item.asBlock.statics = HB_VM_STACK.iStatics;
   /* store the number of expected parameters
    */
   ( * HB_VM_STACK.pPos )->item.asBlock.paramcnt = 0;
   /* store the line number where the codeblock was defined
    */
   ( * HB_VM_STACK.pPos )->item.asBlock.lineno = hb_stackBaseItem()->item.asSymbol.lineno;
   hb_stackPush();

   #ifdef HB_THREAD_SUPPORT
      if( hb_ht_context )
      {
         //hb_threadAllow( &hb_gcCollectionForbid );
         HB_CRITICAL_UNLOCK( hb_gcCollectionMutex );
      }
   #endif
}

/* +0    -> HB_P_MPUSHBLOCK
 * +1 +2 -> size of codeblock
 * +3 +4 -> number of expected parameters
 * +5    -> start of pcode
 *
 * NOTE: pCode points to dynamically allocated memory
 */
static void hb_vmPushMacroBlock( BYTE * pCode, PHB_SYMB pSymbols )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushMacroBlock(%p, %p)", pCode, pSymbols));

   HB_SYMBOL_UNUSED( pSymbols ); /* TODO: remove pSymbols */

   ( * HB_VM_STACK.pPos )->type = HB_IT_BLOCK;

   ( * HB_VM_STACK.pPos )->item.asBlock.value = hb_codeblockMacroNew( pCode + 5, pCode[ 1 ] + pCode[ 2 ] * 256 - 5 );

   /* store the statics base of function where the codeblock was defined
    */
   ( * HB_VM_STACK.pPos )->item.asBlock.statics = HB_VM_STACK.iStatics;
   /* store the number of expected parameters
    */
   ( * HB_VM_STACK.pPos )->item.asBlock.paramcnt = pCode[ 3 ] + ( pCode[ 4 ] * 256 );
   /* store the line number where the codeblock was defined
    */
   ( * HB_VM_STACK.pPos )->item.asBlock.lineno = hb_stackBaseItem()->item.asSymbol.lineno;
   hb_stackPush();
}

/* pushes current workarea number on the eval stack
 */
static void hb_vmPushAlias( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushAlias()"));

   ( * HB_VM_STACK.pPos )->type = HB_IT_INTEGER;
   ( * HB_VM_STACK.pPos )->item.asInteger.value = hb_rddGetCurrentWorkAreaNumber();
   ( * HB_VM_STACK.pPos )->item.asInteger.length = 10;
   hb_stackPush();
}

/* It pops the last item from the stack to use it to select a workarea
 * and next pushes the value of a given field
 * (for performance reason it replaces alias value with field value)
 */
static void hb_vmPushAliasedField( PHB_SYMB pSym )
{
   PHB_ITEM pAlias;
   int iCurrArea;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushAliasedField(%p)", pSym));

   pAlias = hb_stackItemFromTop( -1 );
   iCurrArea = hb_rddGetCurrentWorkAreaNumber();

   /* NOTE: hb_vmSelecWorkarea clears passed item
    */
   if( hb_vmSelectWorkarea( pAlias ) == SUCCESS )
      hb_rddGetFieldValue( pAlias, pSym );

   hb_rddSelectWorkAreaNumber( iCurrArea );
}

/* It pops the last item from the stack to use it to select a workarea
 * and next pushes the value of either a field or a memvar based on alias value
 * (for performance reason it replaces alias value with field value)
 * This is used in the following context:
 * ( any_alias )->variable
 */
static void hb_vmPushAliasedVar( PHB_SYMB pSym )
{
   PHB_ITEM pAlias = hb_stackItemFromTop( -1 );

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushAliasedVar(%p)", pSym));

   if( HB_IS_STRING( pAlias ) )
   {
      char *szAlias = hb_strUpperCopy( pAlias->item.asString.value, pAlias->item.asString.length );

      if( szAlias[ 0 ] == 'M' && szAlias[ 1 ] == '\0' )
      {  /* M->variable */
         hb_memvarGetValue( pAlias, pSym );
      }
      else
      {
         int iCmp = strncmp( szAlias, "MEMVAR", 4 );

         if( iCmp == 0 )
                 {
            iCmp = strncmp( szAlias, "MEMVAR", pAlias->item.asString.length );
                 }

         if( iCmp == 0 )
         {  /* MEMVAR-> or MEMVA-> or MEMV-> */
            hb_memvarGetValue( pAlias, pSym );
         }
         else
         {  /* field variable */
            iCmp = strncmp( szAlias, "FIELD", 4 );

                        if( iCmp == 0 )
                        {
               iCmp = strncmp( szAlias, "FIELD", pAlias->item.asString.length );
                        }

            if( iCmp == 0 )
            {  /* FIELD-> */
               hb_rddGetFieldValue( pAlias, pSym );
            }
            else
            {  /* database alias */
               hb_vmPushAliasedField( pSym );
            }
         }
      }

          hb_xfree( szAlias );
   }
   else
   {
      hb_vmPushAliasedField( pSym );
   }
}

static void hb_vmPushLocal( SHORT iLocal )
{
   PHB_ITEM pLocal;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushLocal(%hd)", iLocal));

   if( iLocal >= 0 )
   {
      /* local variable or local parameter */
      pLocal = hb_stackItemFromBase( iLocal );
   }
   else
   {
      /* local variable referenced in a codeblock
       * hb_stackSelfItem() points to a codeblock that is currently evaluated
       */
      pLocal = hb_codeblockGetVar( hb_stackSelfItem(), ( LONG ) iLocal );
   }

   if( HB_IS_BYREF( pLocal ) )
   {
      hb_itemCopy( ( * HB_VM_STACK.pPos ), hb_itemUnRef( pLocal ) );
   }
   else
   {
      hb_itemCopy( ( * HB_VM_STACK.pPos ), pLocal );
   }

   hb_stackPush();
}

static void hb_vmPushLocalByRef( SHORT iLocal )
{
   PHB_ITEM pTop = ( * HB_VM_STACK.pPos );
   PHB_ITEM pLocal;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushLocalByRef(%hd)", iLocal));

   pTop->type = HB_IT_BYREF;
   /* we store its stack offset instead of a pointer to support a dynamic stack */
   pTop->item.asRefer.offset = hb_stackBaseOffset();

   if( iLocal >= 0 )
   {
      // SomeFunc( ... ) - Variable paramaters.
      if( hb_stackBaseItem()->item.asSymbol.paramcnt > 255 )
      {
         iLocal += hb_stackBaseItem()->item.asSymbol.paramcnt - 256;
      }

      pLocal = *( HB_VM_STACK.pBase + iLocal + 1 );

      if( pLocal->type == HB_IT_STRING && ( pLocal->item.asString.bStatic || *( pLocal->item.asString.puiHolders ) > 1 ) )
      {
         char *sString = (char*) hb_xgrab( pLocal->item.asString.length + 1 );

         memcpy( sString, pLocal->item.asString.value, pLocal->item.asString.length + 1 );

         if( pLocal->item.asString.bStatic == FALSE )
         {
            hb_itemReleaseString( pLocal );
         }

         pLocal->item.asString.value = sString;
         pLocal->item.asString.bStatic = FALSE;
         pLocal->item.asString.puiHolders = (USHORT*) hb_xgrab( sizeof( USHORT ) );
         *( pLocal->item.asString.puiHolders ) = 1;
      }

      pTop->item.asRefer.BasePtr.itemsbasePtr = &HB_VM_STACK.pItems;
   }
   else
   {
      /* store direct codeblock address because an item where a codeblock
       * is stored can be no longer placed on the eval stack at the time
       * of a codeblock evaluation or variable access
      */
      pTop->item.asRefer.BasePtr.block = (hb_stackSelfItem())->item.asBlock.value;
   }

   pTop->item.asRefer.value = iLocal;

   hb_stackPush();
}

static void hb_vmPushStatic( USHORT uiStatic )
{
   PHB_ITEM pStatic;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushStatic(%hu)", uiStatic));

   pStatic = s_aStatics.item.asArray.value->pItems + HB_VM_STACK.iStatics + uiStatic - 1;

   if( HB_IS_BYREF( pStatic ) )
   {
      hb_itemCopy( ( * HB_VM_STACK.pPos ), hb_itemUnRef( pStatic ) );
   }
   else
   {
      hb_itemCopy( ( * HB_VM_STACK.pPos ), pStatic );
   }

   hb_stackPush();
}

static void hb_vmPushStaticByRef( USHORT uiStatic )
{
   HB_ITEM_PTR pTop = ( * HB_VM_STACK.pPos );
   PHB_ITEM pReference = s_aStatics.item.asArray.value->pItems + HB_VM_STACK.iStatics + uiStatic - 1;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPushStaticByRef(%hu)", uiStatic));

   if( pReference->type == HB_IT_STRING && ( pReference->item.asString.bStatic || *( pReference->item.asString.puiHolders ) > 1 ) )
   {
      char *sString = (char*) hb_xgrab( pReference->item.asString.length + 1 );

      memcpy( sString, pReference->item.asString.value, pReference->item.asString.length + 1 );

      if( pReference->item.asString.bStatic == FALSE )
      {
         hb_itemReleaseString( pReference );
      }

      pReference->item.asString.value = sString;
      pReference->item.asString.bStatic = FALSE;
      pReference->item.asString.puiHolders = (USHORT*) hb_xgrab( sizeof( USHORT ) );
      *( pReference->item.asString.puiHolders ) = 1;
   }

   pTop->type = HB_IT_BYREF;
   /* we store the offset instead of a pointer to support a dynamic stack */
   pTop->item.asRefer.value = HB_VM_STACK.iStatics + uiStatic - 1;
   pTop->item.asRefer.offset = 0;    /* 0 for static variables */
   pTop->item.asRefer.BasePtr.itemsbase = &s_aStatics.item.asArray.value->pItems;

   hb_stackPush();
}

static void hb_vmPushVariable( PHB_SYMB pVarSymb )
{
   USHORT uiAction;

   do
   {
      /* First try if passed symbol is a name of field
         * in a current workarea - if it is not a field (FAILURE)
         * then try the memvar variable
         */

      uiAction = hb_rddFieldGet( ( * HB_VM_STACK.pPos ), pVarSymb );

      if( uiAction == SUCCESS )
         hb_stackPush();
      else
      {
         uiAction = hb_memvarGet( ( * HB_VM_STACK.pPos ), pVarSymb );
         if( uiAction == SUCCESS )
            hb_stackPush();
         else
         {
            HB_ITEM_PTR pError;

            pError = hb_errRT_New( ES_ERROR, NULL, EG_NOVAR, 1003,
                                    NULL, pVarSymb->szName,
                                    0, EF_CANRETRY );

            uiAction = hb_errLaunch( pError );
            hb_errRelease( pError );
         }
      }
   }
   while( uiAction == E_RETRY );
   HB_TRACE(HB_TR_INFO, ("(hb_vmPushVariable)"));
}


static void hb_vmDuplicate( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmDuplicate()"));

   hb_itemCopy( ( * HB_VM_STACK.pPos ), hb_stackItemFromTop( -1 ) );
   hb_stackPush();
}

static void hb_vmDuplTwo( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmDuplTwo()"));

   hb_itemCopy( ( * HB_VM_STACK.pPos ), hb_stackItemFromTop( -2 ) );
   hb_stackPush();
   hb_itemCopy( ( * HB_VM_STACK.pPos ), hb_stackItemFromTop( -2 ) );
   hb_stackPush();
}

/* ------------------------------- */
/* Pop                             */
/* ------------------------------- */

static BOOL hb_vmPopLogical( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmPopLogical()"));

   if( HB_IS_LOGICAL( hb_stackItemFromTop( -1 ) ) )
   {
      hb_stackDec();

      ( * HB_VM_STACK.pPos )->type = HB_IT_NIL;
      return ( * HB_VM_STACK.pPos )->item.asLogical.value;
   }
   else
   {
      hb_errRT_BASE( EG_ARG, 1066, NULL, hb_langDGetErrorDesc( EG_CONDITION ), 1, hb_stackItemFromTop( -1 ) );
      return FALSE;
   }
}

/* NOTE: Type checking should be done by the caller. */

static double hb_vmPopNumber( void )
{
   PHB_ITEM pItem;
   double dNumber;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPopNumber()"));

   pItem = hb_stackItemFromTop( -1 );
   hb_stackDec();

   switch( pItem->type & ~HB_IT_BYREF )
   {
      case HB_IT_INTEGER:
         dNumber = ( double ) pItem->item.asInteger.value;
         break;

      case HB_IT_LONG:
         dNumber = ( double ) pItem->item.asLong.value;
         break;

      case HB_IT_DOUBLE:
         dNumber = pItem->item.asDouble.value;
         break;

      case HB_IT_DATE:
         dNumber = (double) pItem->item.asDate.value;
         break;

      case HB_IT_STRING:
         dNumber = (double) ( pItem->item.asString.value[0] );
         break;

      default:
         dNumber = 0;  /* To avoid GCC -O2 warning */
         hb_errInternal( HB_EI_VMPOPINVITEM, NULL, "hb_vmPopNumber()", NULL );
         break;
   }

   ( * HB_VM_STACK.pPos )->type = HB_IT_NIL;

   return dNumber;
}

/* NOTE: Type checking should be done by the caller. */

static double hb_vmPopDouble( int * piDec )
{
   PHB_ITEM pItem;
   double dNumber;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPopDouble(%p)", piDec));

   pItem = hb_stackItemFromTop( -1 );

   hb_stackDec();

   switch( pItem->type & ~HB_IT_BYREF )
   {
      case HB_IT_INTEGER:
         dNumber = ( double ) pItem->item.asInteger.value;
         *piDec = 0;
         break;

      case HB_IT_LONG:
         dNumber = ( double ) pItem->item.asLong.value;
         *piDec = 0;
         break;

      case HB_IT_DOUBLE:
         dNumber = pItem->item.asDouble.value;
         *piDec = pItem->item.asDouble.decimal;
         break;

      case HB_IT_DATE:
         dNumber = (double) pItem->item.asDate.value;
         *piDec = 0;
         break;

      case HB_IT_STRING:
         dNumber = (double) pItem->item.asString.value[0];
         *piDec = 0;
         break;

      default:
         dNumber = 0;  /* To avoid GCC -O2 warning */
         hb_errInternal( HB_EI_VMPOPINVITEM, NULL, "hb_vmPopDouble()", NULL );
         break;
   }

   ( * HB_VM_STACK.pPos )->type = HB_IT_NIL;

   return dNumber;
}

/* Pops the item from the eval stack and uses it to select the current
 * workarea
 */
static void hb_vmPopAlias( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmPopAlias()"));

   hb_vmSelectWorkarea( hb_stackItemFromTop( -1 ) ); /* it clears the passed item */
   hb_stackDec();
}

/* Pops the alias to use it to select a workarea and next pops a value
 * into a given field
 */
static void hb_vmPopAliasedField( PHB_SYMB pSym )
{
   int iCurrArea;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPopAliasedField(%p)", pSym));

   iCurrArea = hb_rddGetCurrentWorkAreaNumber();

   if( hb_vmSelectWorkarea( hb_stackItemFromTop( -1 ) ) == SUCCESS )
   {
      hb_rddPutFieldValue( hb_stackItemFromTop( -2 ), pSym );
   }

   hb_rddSelectWorkAreaNumber( iCurrArea );
   hb_stackDec();    /* alias - it was cleared in hb_vmSelectWorkarea */
   hb_stackPop();    /* field value */
}

/* Pops the alias to use it to select a workarea and next pops a value
 * into either a field or a memvar based on the alias value
 * This is used in the following context:
 * ( any_alias )->variable
 */
static void hb_vmPopAliasedVar( PHB_SYMB pSym )
{
   HB_ITEM_PTR pAlias = hb_stackItemFromTop( -1 );

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPopAliasedVar(%p)", pSym));

   /* "M", "MEMV" - "MEMVAR" and "FIEL" - "FIELD" are reserved aliases
    */
   if( HB_IS_STRING( pAlias ) )
   {
      char * szAlias = pAlias->item.asString.value;

      if( szAlias[ 0 ] == 'M' && szAlias[ 1 ] == '\0' )
      {  /* M->variable */
         hb_memvarSetValue( pSym, hb_stackItemFromTop( -2 ) );
         hb_stackPop();    /* alias */
         hb_stackPop();    /* value */
      }
      else
      {
         int iCmp = strncmp( szAlias, "MEMVAR", 4 );

         if( iCmp == 0 )
         {
            iCmp = strncmp( szAlias, "MEMVAR", pAlias->item.asString.length );
         }

         if( iCmp == 0 )
         {  /* MEMVAR-> or MEMVA-> or MEMV-> */
            hb_memvarSetValue( pSym, hb_stackItemFromTop( -2 ) );
            hb_stackPop();    /* alias */
            hb_stackPop();    /* value */
         }
         else
         {  /* field variable */
            iCmp = strncmp( szAlias, "FIELD", 4 );

            if( iCmp == 0 )
            {
               iCmp = strncmp( szAlias, "FIELD", pAlias->item.asString.length );
            }

            if( iCmp == 0 )
            {  /* FIELD-> */
               hb_rddPutFieldValue( hb_stackItemFromTop( -2 ), pSym );
               hb_stackPop();    /* alias */
               hb_stackPop();    /* value */
            }
            else
            {  /* database alias */
               hb_vmPopAliasedField( pSym );
            }
         }
      }
   }
   else
   {
      hb_vmPopAliasedField( pSym );
   }
}

static void hb_vmPopLocal( SHORT iLocal )
{
   PHB_ITEM pLocal, pVal;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPopLocal(%hd)", iLocal));

   pVal = *( HB_VM_STACK.pPos - 1 );

   /* Remove MEMOFLAG if exists (assignment from field). */
   pVal->type &= ~HB_IT_MEMOFLAG;

   if( iLocal >= 0 )
   {
      /* local variable or local parameter */
      pLocal = hb_stackItemFromBase( iLocal );

      /* Assigned to a parameter by refrence. */
      if( HB_IS_BYREF( pLocal ) )
      {
         pLocal = hb_itemUnRef( pLocal );
      }
   }
   else
   {
      /* local variable referenced in a codeblock
       * hb_stackSelfItem() points to a codeblock that is currently evaluated
       */
      pLocal = hb_codeblockGetVar( hb_stackSelfItem(), iLocal ) ;
   }

   hb_itemForwardValue( pLocal, pVal );

   hb_stackDec();
}

static void hb_vmPopStatic( USHORT uiStatic )
{
   PHB_ITEM pStatic, pVal;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmPopStatic(%hu)", uiStatic));

   pVal = *( HB_VM_STACK.pPos - 1 );

   /* Remove MEMOFLAG if exists (assignment from field). */
   pVal->type &= ~HB_IT_MEMOFLAG;

   pStatic = s_aStatics.item.asArray.value->pItems + HB_VM_STACK.iStatics + uiStatic - 1;

   hb_itemForwardValue( pStatic, pVal );

   hb_stackDec();
}

/* ----------------------------------------------- */

void HB_EXPORT hb_vmProcessSymbols( PHB_SYMB pModuleSymbols, ... ) /* module symbols initialization */
{
   PSYMBOLS pNewSymbols;
   USHORT ui;
   va_list ap;
   USHORT uiModuleSymbols;
   int iPCodeVer = 0, iLen;
   char *sModule;
   BOOL bFree = FALSE;

   if( s_fmInit )
   {
      s_fmInit = FALSE;
      hb_xinit();
   }

   HB_TRACE(HB_TR_DEBUG, ("hb_vmProcessSymbols(%p, %dl )", pModuleSymbols));

   va_start( ap, pModuleSymbols );
      uiModuleSymbols = (USHORT) va_arg( ap, int );
      sModule = va_arg( ap, char * );

      if( sModule )
      {
         iLen = strlen( sModule );
         if( ( iLen > 2 && sModule[ iLen - 2 ] == '.' ) || ( iLen > 3 && sModule[ iLen - 3 ] == '.' ) || ( iLen > 4 && sModule[ iLen - 4 ] == '.' ) )
         {
            iPCodeVer = va_arg( ap, int );
         }
         else
         {
            sModule = (char *) hb_xgrab( 128 );
            bFree = TRUE;

            for( ui = 0; ui < uiModuleSymbols; ui++ )
            {
               if( pModuleSymbols[ui].pFunPtr )
               {
                  sprintf( sModule, "Program with 1st fun: %s", pModuleSymbols[ui].szName );
                  break;
               }
            }

            if( ui == uiModuleSymbols )
            {
               sprintf( sModule, "Program with 1st sym: %s", pModuleSymbols[0].szName );
            }

            iPCodeVer = 0;
         }
      }
      else
      {
         sModule = (char *) hb_xgrab( 128 );
         bFree = TRUE;

         for( ui = 0; ui < uiModuleSymbols; ui++ )
         {
            if( pModuleSymbols[ui].pFunPtr )
            {
               sprintf( sModule, "Program with 1st fun: %s", pModuleSymbols[ui].szName );
               break;
            }
         }

         if( ui == uiModuleSymbols )
         {
            sprintf( sModule, "Program with 1st sym: %s", pModuleSymbols[0].szName );
         }
      }
   va_end( ap );

   if( iPCodeVer != HB_PCODE_VER )
   {
      char sTemp[512];

      sprintf( (char *) sTemp, "'%s' was compiled by older version, PCODE version %i is no longer supported - Please recompile.\n", sModule, iPCodeVer );

      hb_errInternal( HB_EI_ERRUNRECOV, sTemp, NULL, NULL );
   }

   if( bFree )
   {
      hb_xfree( sModule );
   }

   pNewSymbols = ( PSYMBOLS ) hb_xgrab( sizeof( SYMBOLS ) );
   pNewSymbols->pModuleSymbols = pModuleSymbols;
   pNewSymbols->uiModuleSymbols = uiModuleSymbols;
   pNewSymbols->pNext = NULL;
   pNewSymbols->hScope = 0;

   if( s_pSymbols == NULL )
      s_pSymbols = pNewSymbols;
   else
   {
      PSYMBOLS pLastSymbols;

      pLastSymbols = s_pSymbols;
      while( pLastSymbols->pNext ) /* locates the latest processed group of symbols */
      {
         pLastSymbols = pLastSymbols->pNext;
      }
      pLastSymbols->pNext = pNewSymbols;
   }

   for( ui = 0; ui < uiModuleSymbols; ui++ ) /* register each public symbol on the dynamic symbol table */
   {
      HB_SYMBOLSCOPE hSymScope;

      hSymScope = ( pModuleSymbols + ui )->cScope;
      pNewSymbols->hScope |= hSymScope;
      if( ( ! s_pSymStart ) && ( hSymScope & HB_FS_FIRST && ! (  hSymScope & HB_FS_INITEXIT ) ) )
      {
         s_pSymStart = pModuleSymbols + ui;  /* first public defined symbol to start execution */
      }

      if( hSymScope & ( HB_FS_PUBLIC | HB_FS_MESSAGE | HB_FS_MEMVAR | HB_FS_FIRST ) )
      {
         hb_dynsymNew( pModuleSymbols + ui );
      }
   }
}

static void hb_vmReleaseLocalSymbols( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmReleaseLocalSymbols()"));

   while( s_pSymbols )
   {
      PSYMBOLS pDestroy;

      pDestroy = s_pSymbols;
      s_pSymbols = s_pSymbols->pNext;
      hb_xfree( pDestroy );
   }

   HB_TRACE(HB_TR_DEBUG, ("Done hb_vmReleaseLocalSymbols()"));
}

/* This calls all _INITSTATICS functions defined in the application.
 * We are using a special symbol's scope ( HB_FS_INIT | HB_FS_EXIT ) to mark
 * this function. These two bits cannot be marked at the same
 * time for normal user defined functions.
 */
static void hb_vmDoInitStatics( void )
{
   PSYMBOLS pLastSymbols = s_pSymbols;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmDoInitStatics()"));

   do
   {
      if( ( pLastSymbols->hScope & ( HB_FS_INIT | HB_FS_EXIT ) ) == ( HB_FS_INIT | HB_FS_EXIT ) )
      {
         USHORT ui;

         for( ui = 0; ui < pLastSymbols->uiModuleSymbols; ui++ )
         {
            HB_SYMBOLSCOPE scope = ( pLastSymbols->pModuleSymbols + ui )->cScope & ( HB_FS_EXIT | HB_FS_INIT );

            if( scope == ( HB_FS_INIT | HB_FS_EXIT ) )
            {
               hb_vmPushSymbol( pLastSymbols->pModuleSymbols + ui );
               hb_vmPushNil();
               hb_vmDo( 0 );
            }
         }
      }
      pLastSymbols = pLastSymbols->pNext;

   } while( pLastSymbols );
}

static void hb_vmDoExitFunctions( void )
{
   PSYMBOLS pLastSymbols = s_pSymbols;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmDoExitFunctions()"));

   do
   {
      /* only if module contains some EXIT functions */
      if( pLastSymbols->hScope & HB_FS_EXIT )
      {
         USHORT ui;

         for( ui = 0; ui < pLastSymbols->uiModuleSymbols; ui++ )
         {
            HB_SYMBOLSCOPE scope = ( pLastSymbols->pModuleSymbols + ui )->cScope & ( HB_FS_EXIT | HB_FS_INIT );

            if( scope == HB_FS_EXIT )
            {
               hb_vmPushSymbol( pLastSymbols->pModuleSymbols + ui );
               hb_vmPushNil();
               hb_vmDo( 0 );
               if( s_uiActionRequest )
                  /* QUIT or BREAK was issued - stop processing
                  */
                  return;
            }
         }
      }
      pLastSymbols = pLastSymbols->pNext;

   } while( pLastSymbols );
}

static void hb_vmDoInitFunctions( void )
{
   PSYMBOLS pLastSymbols = s_pSymbols;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmDoInitFunctions()"));

   do
   {
      /* only if module contains some INIT functions */
      if( pLastSymbols->hScope & HB_FS_INIT )
      {
         USHORT ui;

         for( ui = 0; ui < pLastSymbols->uiModuleSymbols; ui++ )
         {
            HB_SYMBOLSCOPE scope = ( pLastSymbols->pModuleSymbols + ui )->cScope & ( HB_FS_EXIT | HB_FS_INIT );

            if( scope == HB_FS_INIT )
            {
               int argc = hb_cmdargARGC();
               char ** argv = hb_cmdargARGV();

               int i;
               int iArgCount;

               hb_vmPushSymbol( pLastSymbols->pModuleSymbols + ui );
               hb_vmPushNil();

               iArgCount = 0;
               for( i = 1; i < argc; i++ ) /* places application parameters on the stack */
               {
                  /* Filter out any parameters beginning with //, like //INFO */
                  if( ! hb_cmdargIsInternal( argv[ i ] ) )
                  {
                     hb_vmPushString( argv[ i ], strlen( argv[ i ] ) );
                     iArgCount++;
                  }
               }

               hb_vmDo( iArgCount );
            }
         }
      }
      pLastSymbols = pLastSymbols->pNext;

   } while( pLastSymbols );
}

/* NOTE: We should make sure that these get linked.
         Don't make this function static, because it's not called from
         this file. [vszakats] */

void hb_vmForceLink( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmForceLink()"));

   HB_FUNCNAME( SYSINIT )();
}

/* ----------------------------- */

HB_FUNC( ERRORLEVEL )
{
   hb_retni( s_byErrorLevel );

   /* NOTE: This should be ISNUM( 1 ), but it's sort of a Clipper bug that it
            accepts other types also and considers them zero. [vszakats] */

   if( hb_pcount() >= 1 )
   {
      /* Only replace the error level if a parameter was passed */
      s_byErrorLevel = hb_parni( 1 );
   }
}

void hb_vmRequestQuit( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmRequestQuit()"));

   s_uiActionRequest = HB_QUIT_REQUESTED;
}

void hb_vmRequestEndProc( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmRequestEndProc()"));

   s_uiActionRequest = HB_ENDPROC_REQUESTED;
}

void hb_vmRequestBreak( PHB_ITEM pItem )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmRequestBreak(%p)", pItem));

   //printf( "Recover %i\n", s_lRecoverBase );

   if( s_lRecoverBase )
   {
      if( pItem )
         hb_itemCopy( hb_stackItem( s_lRecoverBase + HB_RECOVER_VALUE ), pItem );

      s_uiActionRequest = HB_BREAK_REQUESTED;
   }
   else
      s_uiActionRequest = HB_QUIT_REQUESTED;
}

USHORT hb_vmRequestQuery( void )
{
   return s_uiActionRequest;
}

void hb_vmRequestCancel( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmRequestCancel()"));

   if( hb_set.HB_SET_CANCEL )
   {
      char buffer[ HB_SYMBOL_NAME_LEN + HB_SYMBOL_NAME_LEN + 2 ];
      int i = 1, i2;
      unsigned long ulLine;
      PHB_ITEM * pBase;

      hb_conOutErr( hb_conNewLine(), 0 );
      sprintf( buffer, "Cancelled at: %s (%i)", hb_stackBaseItem()->item.asSymbol.value->szName, hb_stackBaseItem()->item.asSymbol.lineno );
      hb_conOutErr( buffer, 0 );
      hb_conOutErr( hb_conNewLine(), 0 );

      while ( buffer[0] )
      {
         i2 = i;
         hb_procinfo( i++, buffer, NULL );

         if( buffer[0] == 0 )
            break;

         pBase = HB_VM_STACK.pBase;
         while( ( i2-- > 0 ) && pBase != HB_VM_STACK.pItems )
            pBase = HB_VM_STACK.pItems + ( *pBase )->item.asSymbol.stackbase;

         if( i2 == -1 )
            ulLine = ( *pBase )->item.asSymbol.lineno;
         else
            ulLine = 0;

         i2 = strlen( (char *) buffer );
         sprintf( buffer + i2, " (%lu)", ulLine );

         hb_conOutErr( buffer, 0 );
         hb_conOutErr( hb_conNewLine(), 0 );
      }

      s_uiActionRequest = HB_QUIT_REQUESTED;
   }
}

void hb_vmRequestDebug( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmRequestCancel()"));
   s_bDebugRequest = TRUE;
}

HB_FUNC( INVOKEDEBUG )
{
   BOOL bRequest = s_bDebugRequest;
   s_bDebugRequest = FALSE;
   hb_retl( bRequest );
}

/* $Doc$
 * $FuncName$     <aStat> __vmVarSList()
 * $Description$  Return a clone of the statics array.
 * $End$ */
HB_FUNC( __VMVARSLIST )
{
   PHB_ITEM pStatics = hb_arrayClone( &s_aStatics, NULL );

   hb_itemForwardValue( &(HB_VM_STACK.Return), pStatics );
   hb_itemRelease( pStatics );
}

/* $Doc$
 * $FuncName$     <nStatics> __vmVarSLen()
 * $Description$  Return the statics array length.
 * $End$ */
HB_FUNC( __VMVARSLEN )
{
   hb_retnl( s_aStatics.item.asArray.value->ulLen );
}

/* $Doc$
 * $FuncName$     <xStat> __vmVarSGet(<nStatic>)
 * $Description$  Return a specified statics
 * $End$ */
HB_FUNC( __VMVARSGET )
{
   hb_itemCopy( &(HB_VM_STACK.Return), s_aStatics.item.asArray.value->pItems + hb_parni( 1 ) - 1 );
}

/* $Doc$
 * $FuncName$     __vmVarSSet(<nStatic>,<uValue>)
 * $Description$  Sets the value of a specified statics
 * $End$ */
HB_FUNC( __VMVARSSET )
{
   hb_itemCopy( s_aStatics.item.asArray.value->pItems + hb_parni( 1 ) - 1, * ( HB_VM_STACK.pBase + 3 ) );
}

/* ------------------------------------------------------------------------ */
/* The garbage collector interface */
/* ------------------------------------------------------------------------ */

/* Mark all locals as used so they will not be released by the
 * garbage collector
 */
void hb_vmIsLocalRef( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmIsLocalRef()"));

   if( hb_stack.Return.type & (HB_IT_BYREF | HB_IT_POINTER | HB_IT_ARRAY | HB_IT_BLOCK) )
   {
      hb_gcItemRef( &(hb_stack.Return) );
   }
   //printf( "After ReturnRef\n" );

   if( hb_stack.pPos > hb_stack.pItems )
   {
      /* the eval stack is not cleared yet */
      HB_ITEM_PTR * pItem = hb_stack.pPos - 1;

      while( pItem != hb_stack.pItems )
      {
         if( ( *pItem )->type & (HB_IT_BYREF | HB_IT_POINTER | HB_IT_ARRAY | HB_IT_BLOCK) )
         {
            hb_gcItemRef( *pItem );
         }

         --pItem;
      }
   }
}

/* Mark all statics as used so they will not be released by the
 * garbage collector
 */
void hb_vmIsStaticRef( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmIsStaticRef()"));

   /* statics are stored as an item of array type */
   hb_gcItemRef( &s_aStatics );
}

void hb_vmRegisterGlobals( PHB_ITEM **pGlobals, short iGlobals )
{
   HB_ITEM_PTR pTop = ( * HB_VM_STACK.pPos );
   short iGlobal;

   pTop->type = HB_IT_BYREF;

   for ( iGlobal = 0; iGlobal < iGlobals; iGlobal++ )
   {
      pTop->item.asRefer.value = iGlobal + 1; // To offset the -1 below.
      pTop->item.asRefer.offset = -1; // Because 0 will be translated as a STATIC in hb_itemUnref();
      pTop->item.asRefer.BasePtr.itemsbasePtr = pGlobals;

      hb_arrayAdd( &s_aGlobals, pTop );
      //printf( "*** Added %i ***\n", iGlobal );
   }

   pTop->type = HB_IT_NIL;
}

void hb_vmIsGlobalRef( void )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_vmIsGlobalRef()"));

   /* Globals are stored as an item of array type */
   hb_gcItemRef( &s_aGlobals );
}

/* $Doc$
 * $FuncName$     <aStat> __vmVarGList()
 * $Description$  Return a clone of the globals array.
 * $End$ */
HB_FUNC( __VMVARGLIST )
{
   PHB_ITEM pGlobals = hb_arrayClone( &s_aGlobals, NULL );

   hb_itemForwardValue( &(HB_VM_STACK.Return), pGlobals );
   hb_itemRelease( pGlobals );
}

/* $Doc$
 * $FuncName$     __SETPROFILER( <lOnOff> ) --> <lOldValue>
 * $Description$  Turns on | off the profiler activity
 * $End$ */
HB_FUNC( __SETPROFILER )
{
   BOOL bOldValue = hb_bProfiler;

   hb_bProfiler = hb_parl( 1 );

   hb_retl( bOldValue );
}

/* $Doc$
 * $FuncName$     __TRACEPRGCALLS( <lOnOff> ) --> <lOldValue>
 * $Description$  Turns on | off tracing of PRG-level function and method calls
 * $End$ */
HB_FUNC( __TRACEPRGCALLS )
{
   BOOL bOldValue = hb_bTracePrgCalls;

   hb_bTracePrgCalls = hb_parl( 1 );

   hb_retl( bOldValue );
}

/* hvm support for pcode DLLs */

void HB_EXPORT hb_vmProcessDllSymbols( PHB_SYMB pModuleSymbols, USHORT uiModuleSymbols )
{
   PSYMBOLS pNewSymbols;
   USHORT ui;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmProcessDllSymbols(%p, %hu)", pModuleSymbols, uiModuleSymbols));

   pNewSymbols = ( PSYMBOLS ) hb_xgrab( sizeof( SYMBOLS ) );
   pNewSymbols->pModuleSymbols = pModuleSymbols;
   pNewSymbols->uiModuleSymbols = uiModuleSymbols;
   pNewSymbols->pNext = NULL;
   pNewSymbols->hScope = 0;

   if( s_pSymbols == NULL )
      s_pSymbols = pNewSymbols;
   else
   {
      PSYMBOLS pLastSymbols;

      pLastSymbols = s_pSymbols;
      while( pLastSymbols->pNext ) /* locates the latest processed group of symbols */
         pLastSymbols = pLastSymbols->pNext;

      pLastSymbols->pNext = pNewSymbols;
   }

   for( ui = 0; ui < uiModuleSymbols; ui++ ) /* register each public symbol on the dynamic symbol table */
   {
      HB_SYMBOLSCOPE hSymScope;

      hSymScope = ( pModuleSymbols + ui )->cScope;
      pNewSymbols->hScope |= hSymScope;

      if( ( hSymScope == HB_FS_PUBLIC ) || ( hSymScope & ( HB_FS_MESSAGE | HB_FS_MEMVAR | HB_FS_FIRST ) ) )
      {
         PHB_DYNS pDynSym = hb_dynsymFind( ( pModuleSymbols + ui )->szName );

         if( pDynSym && pDynSym->pFunPtr && ( pModuleSymbols + ui )->pFunPtr )
            ( pModuleSymbols + ui )->pFunPtr = pDynSym->pFunPtr;
         else
             hb_dynsymNew( ( pModuleSymbols + ui ) );
      }
   }
}

HB_FUNC( HB_FUNCPTR )
{
   PHB_ITEM pParam = hb_stackItemFromBase( 1 );
   PHB_DYNS pDynSym;

   if( HB_IS_STRING( pParam ) )
   {
      char *sSym = hb_strUpperCopy( pParam->item.asString.value, pParam->item.asString.length );

      pDynSym = hb_dynsymFind( sSym );
      hb_xfree( sSym );

      if( pDynSym )
      {
         ( &(HB_VM_STACK.Return) )->type = HB_IT_LONG;
         ( &(HB_VM_STACK.Return) )->item.asLong.value = (ULONG) pDynSym->pSymbol->pFunPtr;
         ( &(HB_VM_STACK.Return) )->item.asLong.length = 10;
      }
      else
      {
         hb_vmPushLong( 0 );
      }
   }
   else
   {
      hb_errRT_BASE_SubstR( EG_ARG, 1099, NULL, "HB_FuncPtr", 1, hb_paramError( 1 ) );
   }
}

HB_FUNC( HB_EXEC )
{
   PHB_ITEM pPointer = *( HB_VM_STACK.pBase + 1 + 1 );

   if ( pPointer->type == HB_IT_LONG )
   {
      PHB_FUNC pFunc = (PHB_FUNC) hb_itemGetNL( pPointer );
      PHB_ITEM pSelf;
      PHB_DYNS pExecSym;
      int iParams;

      if( hb_pcount() >= 2 )
      {
         if( HB_IS_OBJECT( *( HB_VM_STACK.pBase + 1 + 2 ) ) )
         {
            pSelf = *( HB_VM_STACK.pBase + 1 + 2 );
            pExecSym = hb_clsSymbolFromFunction( pSelf, pFunc );
         }
         else
         {
            pSelf = NULL;
         }

         iParams = hb_pcount() - 2;
      }
      else
      {
         pSelf = NULL;
         iParams = 0;
         hb_vmPushNil();
      }

      if( pSelf == NULL )
      {
         pExecSym = hb_dynsymFindFromFunction( pFunc );
      }

      if( pExecSym == NULL )
      {
         hb_errRT_BASE_SubstR( EG_ARG, 1099, NULL, "HB_Exec", 2, hb_paramError( 1 ), hb_paramError( 2 ) );
         return;
      }

      //printf( "Sym: %p Name: %s\n", pExecSym, pExecSym->pSymbol->szName );

      // Changing the Pointer item to a Symbol Item, so that we don't have to re-push paramters.
      pPointer->type = HB_IT_SYMBOL;
      pPointer->item.asSymbol.value = pExecSym->pSymbol;
      pPointer->item.asSymbol.stackbase = hb_stackTopOffset() - 2 - iParams;

      if( pSelf )
      {
         hb_vmSend( iParams );
      }
      else
      {
         hb_vmDo( iParams );
      }
   }
   else
   {
      hb_errRT_BASE_SubstR( EG_ARG, 1099, NULL, "HB_Exec", 2, hb_paramError( 1 ), hb_paramError( 2 ) );
   }
}

HB_FUNC( HB_QWITH )
{
   hb_itemCopy( &(HB_VM_STACK.Return), &( hb_vm_aWithObject[ hb_vm_wWithObjectCounter - 1 ] ) );
}

HB_FUNC( HB_QSELF )
{
   PHB_ITEM * pBase = HB_VM_STACK.pBase;

   // Outer function level.
   pBase = HB_VM_STACK.pItems + ( *pBase )->item.asSymbol.stackbase;

   // Outer while in Block.
   while( ( strcmp( ( *pBase )->item.asSymbol.value->szName, "EVAL" ) == 0 ||
            strcmp( ( *pBase )->item.asSymbol.value->szName, "__EVAL" ) == 0 ) &&
           pBase != HB_VM_STACK.pItems )
   {
      pBase = HB_VM_STACK.pItems + ( *pBase )->item.asSymbol.stackbase;
   }

   hb_itemCopy( &(HB_VM_STACK.Return), *( pBase + 1 ) );
}

HB_FUNC( __OPCOUNT ) /* it returns the total amount of opcodes */
{
   hb_retnl( HB_P_LAST_PCODE - 1 );
}

HB_FUNC( __OPGETPRF ) /* profiler: It returns an array with an opcode called and
                         consumed times { nTimes, nTime },
                         given the opcode index */
{
   ULONG ulOpcode = hb_parnl( 1 );

   hb_reta( 2 );
   hb_stornl( 0, -1, 1 );
   hb_stornl( 0, -1, 2 );

   if( ulOpcode < HB_P_LAST_PCODE )
   {
      hb_stornl( hb_ulOpcodesCalls[ ulOpcode ], -1, 1 );
      hb_stornl( hb_ulOpcodesTime[ ulOpcode ],  -1, 2 );
   }
}


