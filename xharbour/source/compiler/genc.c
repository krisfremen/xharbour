/*
 * $Id: genc.c,v 1.89 2005/03/31 03:16:10 druzus Exp $
 */

/*
 * Harbour Project source code:
 * Compiler C source generation
 *
 * Copyright 1999 Antonio Linares <alinares@fivetech.com>
 * www - http://www.harbour-project.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA (or visit
 * their web site at http://www.gnu.org/).
 *
 */

#include <assert.h>
#include <time.h>

#include "hbcomp.h"
#include "hbexemem.h"

static int hb_comp_iBaseLine;

static void hb_compGenCReadable( PFUNCTION pFunc, FILE * yyc );
static void hb_compGenCCompact( PFUNCTION pFunc, FILE * yyc );
static void hb_compGenCInLine( FILE* ) ;
static void hb_writeEndInit( FILE* yyc );
static char *hb_comParseLine( char *sz, BOOL );
static BOOL hb_compNewLine( char * sz, BOOL );
static char ** hb_comExternSplit ( char *string, int *iWord );
static void hb_compGenCAddExtern( int iOption, FILE *yyc, BOOL );
static void hb_compGenCAddProtos( int iOption, FILE *yyc );

// AJ: 2004-02-05
// Routines to check C-In-Line static function declared in a PRG file
// with #PRAGMA BEGINDUMP - ENDDUMP
static void hb_compGenCInLineSymbol( void );
static void hb_compGenCCheckInLineStatic( char * str );
static BOOL hb_compCStaticSymbolFound( char* szSymbol, BOOL bSearchStatic );
static BOOL hb_compSymbFound( char* szSymbol );
/* struct to hold symbol names of c-in-line static functions */
typedef struct _SSYMLIST
{
   char *    szName;
   struct _SSYMLIST * pNext;
} SSYMLIST, * PSSYMLIST;

static PSSYMLIST pStatSymFirst = NULL;
static PSSYMLIST pPubSymFirst = NULL;

/* AJ: for requests inside pragma begindump */
static PSSYMLIST pRequestList = NULL;
static void hb_compRequestList( char* szExternName );

/* helper structure to pass information */
typedef struct HB_stru_genc_info
{
   FILE * yyc;
   BOOL bVerbose;
   USHORT iNestedCodeblock;
} HB_GENC_INFO, * HB_GENC_INFO_PTR;

#define HB_GENC_FUNC( func ) HB_PCODE_FUNC( func, HB_GENC_INFO_PTR )
typedef HB_GENC_FUNC( HB_GENC_FUNC_ );
typedef HB_GENC_FUNC_ * HB_GENC_FUNC_PTR;

/*
 AJ: 2003-06-25
 Extended to generate pCode listing
 hb_comp_iGenVarList is first initialized in harbour.c as FALSE
 The value is TRUE when /gc3 is used
*/
extern BOOL hb_comp_iGenVarList;
extern char *hb_comp_FileAsSymbol;
extern char *hb_comp_PrgFileName;

extern char *hb_Command_Line;

/*
 hb_comp_pCodeList is the file handle on which pCode Listing will be written
*/
FILE *hb_comp_pCodeList = NULL;

extern BOOL hb_comp_Failure;

void hb_compGenCCode( PHB_FNAME pFileName, char *szSourceExtension )      /* generates the C language output */
{
   char szFileName[ _POSIX_PATH_MAX ];
   char szpCodeFileName[ _POSIX_PATH_MAX ] ;
   char szSourceName[ _POSIX_PATH_MAX ], *pTmp;
   PFUNCTION pFunc = hb_comp_functions.pFirst;
   PCOMSYMBOL pSym = hb_comp_symbols.pFirst;
   PCOMDECLARED pDeclared;
   PCOMCLASS    pClass;
   FILE * yyc; /* file handle for C output */
   PINLINE pInline = hb_comp_inlines.pFirst;
   PVAR pGlobal, pDelete;
   short iLocalGlobals = 0, iGlobals = 0;

   BOOL bIsPublicFunction ;
   BOOL bIsInitFunction   ;
   BOOL bIsExitFunction   ;
   BOOL bIsStaticVariable ;
   BOOL bIsGlobalVariable ;

   BOOL bCritical = FALSE;

   int  iSymOffset, iStartupOffset;

   PSSYMLIST pTemp;
   BOOL bSymFIRST = FALSE;

   if( ! pFileName->szExtension )
   {
      pFileName->szExtension = ".c";
   }

   hb_fsFNameMerge( szFileName, pFileName );

   pFileName->szExtension = szSourceExtension;
   pFileName->szPath = NULL;
   hb_fsFNameMerge( szSourceName, pFileName );

   while( ( pTmp = strchr( szSourceName, '\\' ) ) != NULL )
   {
      *pTmp = '/';
   }

   hb_strupr( pFileName->szName );

   yyc = fopen( szFileName, "wb" );

   if( ! yyc )
   {
      hb_compGenError( hb_comp_szErrors, 'F', HB_COMP_ERR_UNTERMINATED_COMMENTS, NULL, NULL );
      hb_comp_Failure = TRUE;
      return;
   }

   /*
    Create *.p when /gc3 is used
   */
   if ( hb_comp_iGenVarList )
   {
      pFileName->szExtension = ".p";
      hb_fsFNameMerge( szpCodeFileName, pFileName );
      hb_comp_pCodeList = fopen( szpCodeFileName,"wb" );

      if( !hb_comp_pCodeList )
      {
         hb_comp_iGenVarList = FALSE;
      }
   }

   if( ! hb_comp_bQuiet )
   {
      printf( "Generating C source output to \'%s\'...\n", szFileName );
      fflush( stdout );
   }

   hb_strupr( hb_comp_FileAsSymbol );

   {
       int iTmp = strlen( hb_comp_FileAsSymbol );
       int iCh;

       for ( iCh = 0; iCh < iTmp; iCh ++ )
       {
          if ( ( hb_comp_FileAsSymbol[ iCh ] == '!' ) ||
               ( hb_comp_FileAsSymbol[ iCh ] == '~' ) ||
               ( hb_comp_FileAsSymbol[ iCh ] == '$' ) ||
               ( hb_comp_FileAsSymbol[ iCh ] == '%' ) ||
               ( hb_comp_FileAsSymbol[ iCh ] == '\'') ||
               ( hb_comp_FileAsSymbol[ iCh ] == ')' ) ||
               ( hb_comp_FileAsSymbol[ iCh ] == '(' ) ||
               ( hb_comp_FileAsSymbol[ iCh ] == ' ' ) ||
               ( hb_comp_FileAsSymbol[ iCh ] == '-' ) ||
               ( hb_comp_FileAsSymbol[ iCh ] == '@' ) ||
               ( hb_comp_FileAsSymbol[ iCh ] == '{' ) ||
               ( hb_comp_FileAsSymbol[ iCh ] == '}' ) )
          {
               hb_comp_FileAsSymbol[ iCh ] = '_';
          }
       }
   }

   {
      char *szComp = hb_verCompiler();
      char *szHrb  = hb_verHarbour();
      time_t t;
      struct tm * oTime;

      time( &t );
      oTime = localtime( &t );

      fprintf( yyc, "/*\n * %s\n", szHrb );
      /* AJ: Some compilers performs [f]printf("<%s>",string) incorrecltly */
      fprintf( yyc, " * Generated C source code from %s%s%s\n", "<", hb_comp_PrgFileName, ">" );
      if( hb_Command_Line && *hb_Command_Line )
      {
         fprintf( yyc, " * Command: %s\n", hb_Command_Line );
      }
      fprintf( yyc, " * Created: %04d.%02d.%02d %02d:%02d:%02d (%s)\n */\n\n", oTime->tm_year + 1900, oTime->tm_mon + 1, oTime->tm_mday, oTime->tm_hour, oTime->tm_min, oTime->tm_sec, szComp );

      hb_xfree( szComp );
      hb_xfree( szHrb );
   }

   if( hb_comp_iFunctionCnt )
   {
      fprintf( yyc, "#include \"hbvmpub.h\"\n" );

      if( hb_comp_iGenCOutput != HB_COMPGENC_COMPACT )
      {
         fprintf( yyc, "#include \"hbpcode.h\"\n" );
      }

      fprintf( yyc, "#include \"hbinit.h\"\n\n" );

      fprintf( yyc, "#define __PRG_SOURCE__ \"%s\"\n\n", hb_comp_PrgFileName );

      if( ! hb_comp_bStartProc )
      {
         pFunc = pFunc->pNext; /* No implicit starting procedure */
      }

      /* write functions prototypes for PRG defined functions */
      while( pFunc )
      {
         bIsInitFunction   = ( pFunc->cScope & HB_FS_INIT ) ;
         bIsExitFunction   = ( pFunc->cScope & HB_FS_EXIT ) ;
         bIsStaticVariable = ( pFunc == hb_comp_pInitFunc ) ;
         bIsGlobalVariable = ( pFunc == hb_comp_pGlobalsFunc ) ;
         bIsPublicFunction = ( pFunc->cScope == HB_FS_PUBLIC ) ;

         if( pFunc->cScope & HB_FS_CRITICAL )
         {
            bCritical = TRUE;
         }

         /* Is it a PUBLIC FUNCTION/PROCEDURE */
         if ( bIsPublicFunction )
         {
            fprintf( yyc, "HB_FUNC( %s );\n", pFunc->szName );
         }
         /* Is it a STATIC$ */
         else if ( bIsStaticVariable )
         {
            fprintf( yyc, "\nstatic HARBOUR hb_INITSTATICS( void );\n\n" ); /* NOTE: hb_ intentionally in lower case */
         }
         /* Is it a GLOBAL$ */
         else if ( bIsGlobalVariable )
         {
            fprintf( yyc, "static HARBOUR hb_INITGLOBALS( void );\n" ); /* NOTE: hb_ intentionally in lower case */
         }
         /* Is it an INIT FUNCTION/PROCEDURE */
         else if ( bIsInitFunction )
         {
            fprintf( yyc, "HB_FUNC_INIT( %.*s );\n", strlen( pFunc->szName ) - 1, pFunc->szName );
         }
         /* Is it an EXIT FUNCTION/PROCEDURE */
         else if ( bIsExitFunction )
         {
            fprintf( yyc, "HB_FUNC_EXIT( %.*s );\n", strlen( pFunc->szName ) - 1, pFunc->szName );
         }
         /* Then it must be a STATIC FUNCTION/PROCEDURE */
         else
         {
            fprintf( yyc, "HB_FUNC_STATIC( %s );\n", pFunc->szName );
         }

         pFunc = pFunc->pNext;
      }

      // Any NON EXTERN Globals to Register?
      pGlobal = hb_comp_pGlobals;
      while( pGlobal )
      {
         iGlobals++;

         if( pGlobal->szAlias == NULL )
         {
            iLocalGlobals++;
         }

         pGlobal = pGlobal->pNext;
      }

      if( iLocalGlobals )
      {
         fprintf( yyc, "static HARBOUR hb_REGISTERGLOBALS( void );\n\n" ); /* NOTE: hb_ intentionally in lower case */
      }

      /* write functions prototypes for inline blocks */
      while( pInline )
      {
         if( pInline->szName )
         {
            fprintf( yyc, "HB_FUNC_STATIC( %s );\n", pInline->szName );
         }
         pInline = pInline->pNext;
      }

      /* check c-in-line static functions */
      pInline = hb_comp_inlines.pFirst;
      if ( pInline && pInline->pCode )
      {
         hb_compGenCInLineSymbol();
      }

      /* write functions prototypes for called functions outside this PRG */
      pFunc = hb_comp_funcalls.pFirst;
      while( pFunc )
      {
         if( hb_compFunctionFind( pFunc->szName ) == NULL && hb_compInlineFind( pFunc->szName ) == NULL )
         {
            if( hb_compCStaticSymbolFound( pFunc->szName, TRUE ) )
            {
               fprintf( yyc, "HB_FUNC_STATIC( %s );\n", pFunc->szName );
            }
            else
            {
               if( hb_compCStaticSymbolFound( pFunc->szName, FALSE ) )
               {
                  fprintf( yyc, "HB_FUNC( %s );\n", pFunc->szName );
               }
               else
               {
                  fprintf( yyc, "HB_FUNC_EXTERN( %s );\n", pFunc->szName );
               }
            }
         }

         pFunc = pFunc->pNext;
      }

      hb_compGenCAddProtos( 1, yyc );
      hb_compGenCAddProtos( 2, yyc );
      hb_compGenCAddProtos( 3, yyc );

      if( bCritical )
      {
         fprintf( yyc, "\n#define HB_THREAD_SUPPORT\n" );
         fprintf( yyc, "#include \"thread.h\"\n\n" );

         pFunc = hb_comp_functions.pFirst;
         while( pFunc )
         {
            if( pFunc->cScope & HB_FS_CRITICAL )
            {
               fprintf( yyc, "static HB_CRITICAL_T s_Critical%s;\n", pFunc->szName );
            }

            pFunc = pFunc->pNext;
         }

         // Write init function for CRITICAL Functions Mutex initialization.
         fprintf( yyc, "\nHB_CALL_ON_STARTUP_BEGIN( hb_InitCritical%s )\n", pFileName->szName );

         pFunc = hb_comp_functions.pFirst;
         while( pFunc )
         {
            if( pFunc->cScope & HB_FS_CRITICAL )
            {
               fprintf( yyc, "   HB_CRITICAL_INIT( s_Critical%s );\n", pFunc->szName );
            }

            pFunc = pFunc->pNext;
         }

         fprintf( yyc, "HB_CALL_ON_STARTUP_END( hb_InitCritical%s )\n", pFileName->szName );
      }

      /* writes the symbol table */
      /* Generate the wrapper that will initialize local symbol table
       */

      fprintf( yyc, "\n#undef HB_PRG_PCODE_VER\n" );
      fprintf( yyc, "#define HB_PRG_PCODE_VER %i\n", (int) HB_PCODE_VER );

      fprintf( yyc, "\nHB_INIT_SYMBOLS_BEGIN( hb_vm_SymbolInit_%s%s )\n", hb_comp_szPrefix, hb_comp_FileAsSymbol );

      iSymOffset = 0;
      iStartupOffset = -1;

      while( pSym )
      {
         if( pSym->szName[ 0 ] == '(' )
         {
            /* Since the normal function cannot be INIT and EXIT at the same time
            * we are using these two bits to mark the special function used to
            * initialize static variables
            */
            fprintf( yyc, "{ \"(_INITSTATICS)\", HB_FS_INITEXIT, {hb_INITSTATICS}, NULL }" ); /* NOTE: hb_ intentionally in lower case */
         }
         else if( pSym->szName[ 0 ] == '[' )
         {
            /* Since the normal function cannot be INIT and EXIT at the same time
            * we are using these two bits to mark the special function used to
            * initialize global variables
            */
            fprintf( yyc, "{ \"(_INITGLOBALS)\", HB_FS_INITEXIT, {hb_INITGLOBALS}, NULL }" ); /* NOTE: hb_ intentionally in lower case */
         }
         else if( pSym->szName[ 0 ] == '{' )
         {
            /* Since the normal function cannot be INIT and EXIT at the same time
            * we are using these two bits to mark the special function used to
            * initialize global variables
            */
            fprintf( yyc, "{ \"hb_REGISTERGLOBALS\", HB_FS_INITEXIT, {hb_REGISTERGLOBALS}, NULL }" ); /* NOTE: hb_ intentionally in lower case */
         }
         else
         {
            fprintf( yyc, "{ \"%s\", ", pSym->szName );

            if( pSym->cScope & HB_FS_STATIC )
            {
               fprintf( yyc, "HB_FS_STATIC" );

               if( pSym->cScope & HB_FS_PUBLIC )
               {
                  fprintf( yyc, " | HB_FS_PUBLIC" );
               }
            }
            else if( pSym->cScope & HB_FS_INIT )
            {
               fprintf( yyc, "HB_FS_INIT" );

               if( pSym->cScope & HB_FS_PUBLIC )
               {
                  fprintf( yyc, " | HB_FS_PUBLIC" );
               }
            }
            else if( pSym->cScope & HB_FS_EXIT )
            {
               fprintf( yyc, "HB_FS_EXIT" );

               if( pSym->cScope & HB_FS_PUBLIC )
               {
                  fprintf( yyc, " | HB_FS_PUBLIC" );
               }
            }
            else if ( hb_compCStaticSymbolFound( pSym->szName, TRUE ) )
            {
               fprintf( yyc, "HB_FS_STATIC" );

               if( pSym->cScope & HB_FS_PUBLIC )
               {
                  fprintf( yyc, " | HB_FS_PUBLIC" );
               }
            }
            else
            {
               fprintf( yyc, "HB_FS_PUBLIC" );
               /*
                  We found first public function in program body and
                  make it starting procedure.
               */
               if ( !bSymFIRST && !hb_compFunCallFind( pSym->szName ) && !hb_comp_bNoStartUp )
               {
                  fprintf( yyc, " | HB_FS_FIRST" );
                  iStartupOffset = iSymOffset;
                  bSymFIRST = TRUE;
               }
            }

            if( pSym->cScope & VS_MEMVAR )
            {
               fprintf( yyc, " | HB_FS_MEMVAR" );
            }

            if( ( pSym->cScope != HB_FS_MESSAGE ) && ( pSym->cScope & HB_FS_MESSAGE ) ) /* only for non public symbols */
            {
               fprintf( yyc, " | HB_FS_MESSAGE" );
            }

            if ( ( pSym->cScope & HB_FS_FIRST ) && !( pSym->cScope & HB_FS_STATIC ) &&  ( ! hb_comp_bNoStartUp ) && ( ! bSymFIRST ) )
            {
               fprintf( yyc, " | HB_FS_FIRST" );
               iStartupOffset = iSymOffset;
               bSymFIRST = TRUE;
            }

            /* specify the function address if it is a defined function or an
               external called function */
            if( hb_compFunctionFind( pSym->szName ) ) /* is it a function defined in this module */
            {
               if( pSym->cScope & HB_FS_INIT )
               {
                  fprintf( yyc, ", {HB_INIT_FUNCNAME( %.*s )}, (PHB_DYNS) 1 }", strlen( pSym->szName ) - 1, pSym->szName );
               }
               else if( pSym->cScope & HB_FS_EXIT )
               {
                  fprintf( yyc, ", {HB_EXIT_FUNCNAME( %.*s )}, (PHB_DYNS) 1 }", strlen( pSym->szName ) - 1, pSym->szName );
               }
               else
               {
                  fprintf( yyc, ", {HB_FUNCNAME( %s )}, (PHB_DYNS) 1 }", pSym->szName );
               }
            }
            else if ( hb_compCStaticSymbolFound( pSym->szName, TRUE ) || hb_compInlineFind( pSym->szName ) )
            {
               fprintf( yyc, ", {HB_FUNCNAME( %s )}, (PHB_DYNS) 1 }", pSym->szName );
            }
            else if( hb_compFunCallFind( pSym->szName ) ) /* is it a function called from this module */
            {
               fprintf( yyc, ", {HB_FUNCNAME( %s )}, NULL }", pSym->szName );
            }
            else
            {
               fprintf( yyc, ", {NULL}, NULL }" );   /* memvar */
            }
         }

         if( pSym != hb_comp_symbols.pLast )
         {
            fprintf( yyc, ",\n" );
         }

         pSym = pSym->pNext;

         iSymOffset++;
      }

      hb_compGenCAddExtern( 1, yyc, bSymFIRST );
      hb_compGenCAddExtern( 2, yyc, bSymFIRST );
      hb_compGenCAddExtern( 3, yyc, bSymFIRST );

      /*
         End of initializaton codes
      */
      hb_writeEndInit( yyc );

      if( hb_comp_bExplicitStartProc && iStartupOffset >= 0 )
      {
         fprintf( yyc, "extern HB_EXPORT void hb_vmExplicitStartup( PHB_SYMB pSymbol );\n" );
         fprintf( yyc, "void hb_InitExplicitStartup( void )\n" );
         fprintf( yyc, "{\n" );
         fprintf( yyc, "   hb_vmExplicitStartup( symbols + %i );\n", iStartupOffset );
         fprintf( yyc, "}\n" );

         fprintf( yyc, "#if defined(HB_PRAGMA_STARTUP)\n" );
         fprintf( yyc, "   #pragma startup hb_InitExplicitStartup\n" );
         fprintf( yyc, "#elif defined(HB_MSC_STARTUP)\n"
                       "   #if _MSC_VER >= 1010\n"
                       "      #pragma data_seg( \".CRT$XIY\" )\n"
                       "      #pragma comment( linker, \"/Merge:.CRT=.data\" )\n"
                       "   #else\n"
                       "      #pragma data_seg( \"XIY\" )\n"
                       "   #endif\n"
                       "   static HB_$INITSYM hb_auto_InitExplicitStartup = hb_InitExplicitStartup;\n"
                       "   #pragma data_seg()\n"
                       "#endif\n\n" );
      }

      if( bCritical )
      {
         fprintf( yyc, "#if defined(HB_PRAGMA_STARTUP)\n" );
         fprintf( yyc, "   #pragma startup hb_InitCritical%s\n", hb_comp_FileAsSymbol );
         fprintf( yyc, "#elif defined(HB_MSC_STARTUP)\n"
                       "   #if _MSC_VER >= 1010\n"
                       "      #pragma data_seg( \".CRT$XIY\" )\n"
                       "      #pragma comment( linker, \"/Merge:.CRT=.data\" )\n"
                       "   #else\n"
                       "      #pragma data_seg( \"XIY\" )\n"
                       "   #endif\n"
                       "   static HB_$INITSYM hb_auto_InitCritical = hb_InitCritical%s;\n"
                       "   #pragma data_seg()\n"
                       "#endif\n\n", hb_comp_FileAsSymbol );
      }

      if( hb_comp_pGlobals )
      {
         fprintf( yyc, "\n#include \"hbapi.h\"\n\n" );

         pGlobal = hb_comp_pGlobals;
         while( pGlobal )
         {
            if( pGlobal->szAlias == NULL )
            {
               fprintf( yyc, "HB_ITEM %s = { 0, { { 0 } } };\n", pGlobal->szName );
            }
            else
            {
               fprintf( yyc, "extern HB_ITEM %s;\n", pGlobal->szName );
            }
            pGlobal = pGlobal->pNext;
         }

         fprintf( yyc, "\nstatic const PHB_ITEM pConstantGlobals[] = {\n" );

         pGlobal = hb_comp_pGlobals;

         while( pGlobal )
         {
            fprintf( yyc, "                                             &%s%c\n", pGlobal->szName, pGlobal->pNext ? ',' : ' ' );
            pGlobal = pGlobal->pNext;
         }

         fprintf( yyc, "                                           };\n"
                       "static PHB_ITEM *pGlobals = (PHB_ITEM *) pConstantGlobals;\n\n" );
      }

      /* Generate functions data
       */
      pFunc = hb_comp_functions.pFirst;

      if( ! hb_comp_bStartProc )
      {
         pFunc = pFunc->pNext; /* No implicit starting procedure */
      }

      while( pFunc )
      {
         // The pCode Table is Written Here
         if ( hb_comp_iGenVarList )
         {
            fprintf( hb_comp_pCodeList, "[%s]\n", pFunc->szName );
         }

         bIsInitFunction   = ( pFunc->cScope & HB_FS_INIT ) ;
         bIsExitFunction   = ( pFunc->cScope & HB_FS_EXIT ) ;
         bIsStaticVariable = ( pFunc == hb_comp_pInitFunc ) ;
         bIsGlobalVariable = ( pFunc == hb_comp_pGlobalsFunc ) ;
         bIsPublicFunction = ( pFunc->cScope == HB_FS_PUBLIC ) ;

         /* Is it a PUBLIC FUNCTION/PROCEDURE */
         if ( bIsPublicFunction )
         {
            fprintf( yyc, "HB_FUNC( %s )", pFunc->szName );
         }
         /* Is it STATICS$ */
         else if( bIsStaticVariable )
         {
            fprintf( yyc, "static HARBOUR hb_INITSTATICS( void )" ); /* NOTE: hb_ intentionally in lower case */
         }
         /* Is it GLOBALS$ */
         else if( bIsGlobalVariable )
         {
            fprintf( yyc, "static HARBOUR hb_INITGLOBALS( void )" ); /* NOTE: hb_ intentionally in lower case */
         }
         /* Is it an INIT FUNCTION/PROCEDURE */
         else if ( bIsInitFunction )
         {
            fprintf( yyc, "HB_FUNC_INIT( %.*s )", strlen( pFunc->szName ) - 1, pFunc->szName );
         }
         /* Is it an EXIT FUNCTION/PROCEDURE */
         else if ( bIsExitFunction )
         {
            fprintf( yyc, "HB_FUNC_EXIT( %.*s )", strlen( pFunc->szName ) - 1, pFunc->szName );
         }
         /* Then it must be a STATIC FUNCTION/PROCEDURE */
         else
         {
            fprintf( yyc, "HB_FUNC_STATIC( %s )", pFunc->szName );
         }

         fprintf( yyc, "\n{\n   static const BYTE pcode[] =\n   {\n" );

         if( hb_comp_iGenCOutput == HB_COMPGENC_COMPACT )
         {
            hb_compGenCCompact( pFunc, yyc );
         }
         else
         {
            hb_compGenCReadable( pFunc, yyc );
         }

         fprintf( yyc, "   };\n\n" );

         // Finished Writting The pCode Table
         // printf( "\n" );

         if( pFunc->cScope & HB_FS_CRITICAL )
         {
            fprintf( yyc, "   HB_CRITICAL_LOCK( s_Critical%s );\n", pFunc->szName );
         }

         fprintf( yyc, "   hb_vmExecute( pcode, symbols, %s );\n", hb_comp_pGlobals ? "&pGlobals" : "NULL" );

         if( pFunc->cScope & HB_FS_CRITICAL )
         {
            fprintf( yyc, "   HB_CRITICAL_UNLOCK( s_Critical%s );\n", pFunc->szName );
         }

         fprintf( yyc,  "}\n\n" );

         pFunc = pFunc->pNext;
      }

      if( iLocalGlobals )
      {
         fprintf( yyc, "static HARBOUR hb_REGISTERGLOBALS( void )\n"
                       "{\n"
                       "   hb_vmRegisterGlobals( &pGlobals, %i );\n", iGlobals );
         fprintf( yyc, "}\n\n" );
      }

      /*
      Generate pCode Listing
      */

      /* Generate codeblocks data
       */
      if( hb_comp_cInlineID )
      {
         fprintf( yyc, "#include \"hbapi.h\"\n" );
         fprintf( yyc, "#include \"hbstack.h\"\n" );
         fprintf( yyc, "#include \"hbapierr.h\"\n" );
         fprintf( yyc, "#include \"hbapiitm.h\"\n" );
         fprintf( yyc, "#include \"hbvm.h\"\n" );
         fprintf( yyc, "#include \"hboo.ch\"\n" );
      }

      pInline = hb_comp_inlines.pFirst;

      if ( pInline && pInline->pCode )
      {
         hb_compGenCInLine( yyc );
         if( hb_comp_Failure )
         {
            fclose( yyc );
            remove( szFileName );
            hb_compGenError( hb_comp_szErrors, 'F', HB_COMP_ERR_UNTERMINATED_COMMENTS, NULL, NULL );
            return;
         }
      }
   }
   else
   {
      /*
         We do not have an ordinary PRG code in file
      */
      if ( pInline && pInline->pCode )
      {
         hb_compGenCInLineSymbol();
      }

      /*
        We have functions in dump areas
      */
      if ( pStatSymFirst || pPubSymFirst || pRequestList )
      {
         fprintf( yyc, "#include \"hbvmpub.h\"\n" );

         if( hb_comp_iGenCOutput != HB_COMPGENC_COMPACT )
         {
            fprintf( yyc, "#include \"hbpcode.h\"\n" );
         }

         fprintf( yyc, "#include \"hbinit.h\"\n\n" );
         fprintf( yyc, "#define __PRG_SOURCE__ \"%s\"\n\n", hb_comp_PrgFileName );

         fprintf( yyc, "#undef HB_PRG_PCODE_VER\n" );
         fprintf( yyc, "#define HB_PRG_PCODE_VER %i\n\n", (int) HB_PCODE_VER );

         hb_compGenCAddProtos( 1, yyc );
         hb_compGenCAddProtos( 2, yyc );
         hb_compGenCAddProtos( 3, yyc );

         fprintf( yyc, "\nHB_INIT_SYMBOLS_BEGIN( hb_vm_SymbolInit_%s%s )\n", hb_comp_szPrefix, hb_comp_FileAsSymbol );

         /*
            REQUESTs in dump areas are processed here
         */
         pTemp = pRequestList;
         while( pTemp )
         {
            BOOL bWrite = FALSE;

            if (!hb_compCStaticSymbolFound( pTemp->szName, TRUE ))
            {
               bWrite = TRUE;
               fprintf( yyc, "{ \"%s\", HB_FS_PUBLIC, {HB_FUNCNAME( %s )}, NULL }",pTemp->szName,pTemp->szName);
            }

            pTemp = pTemp->pNext;

            if( bWrite )
            {
               if ( pTemp )
               {
                  fprintf( yyc, ",\n" );
               }
               else
               {
                  if ( pStatSymFirst || pPubSymFirst )
                  {
                     fprintf( yyc, ",\n" );
                  }
               }
            }
         }

         /*
            HB_FUNCs are processed here
         */
         pTemp = pPubSymFirst;
         while( pTemp )
         {
            // char szSym_Name[ HB_SYMBOL_NAME_LEN + 1 ];
            char *szSym_Name = (char*) hb_xgrab( HB_SYMBOL_NAME_LEN + 1 );

            hb_xmemset( szSym_Name, '\0', HB_SYMBOL_NAME_LEN + 1 );
            strcpy( szSym_Name, pTemp->szName );

            fprintf( yyc, "{ \"%s\", HB_FS_PUBLIC", pTemp->szName );

            pTemp = pTemp->pNext;

            if( pTemp )
            {
               fprintf( yyc, ", {HB_FUNCNAME( %s )}, (PHB_DYNS) 1 },\n",  szSym_Name );
            }
            else
            {
               /*
                  pPubSymFirst was in LIFO mode, we take the last function as
                  start up, if requested.
               */
               if ( !hb_comp_bNoStartUp )
               {
                  fprintf( yyc, " | HB_FS_FIRST, {HB_FUNCNAME( %s )}, (PHB_DYNS) 1 }", szSym_Name );
               }
               else
               {
                  fprintf( yyc, ", {HB_FUNCNAME( %s )}, (PHB_DYNS) 1 }", szSym_Name );
               }

               if ( pStatSymFirst )
               {
                  fprintf( yyc, ",\n" );
               }
            }

            hb_xfree( szSym_Name );
         }

         /*
            HB_FUNC_STATICs are processed here
         */
         pTemp = pStatSymFirst;
         while( pTemp )
         {
            /*
               We force HB_FS_PUBLIC | HB_FS_STATIC here otherwise static
               function cannot be called by program even it is inside the
               module. Perhaps must be corrected one time.
            */
            fprintf( yyc, "{ \"%s\", HB_FS_STATIC",pTemp->szName);

            fprintf( yyc, ", {HB_FUNCNAME( %s )}, (PHB_DYNS) 1 }",pTemp->szName);

            pTemp = pTemp->pNext;

            if( pTemp )
            {
               fprintf( yyc, ",\n" );
            }
         }

         /*
            End of initialization codes
         */
         hb_writeEndInit( yyc );
      }

      /*
         Function bodies are processed here
      */
      pInline = hb_comp_inlines.pFirst;

      if ( pInline && pInline->pCode )
      {
         hb_compGenCInLine( yyc );
         if( hb_comp_Failure )
         {
            /* Error, we erase the C file produced */
            fclose( yyc );
            remove( szFileName );
            hb_compGenError( hb_comp_szErrors, 'F', HB_COMP_ERR_UNTERMINATED_COMMENTS, NULL, NULL );
            return;
         }
      }
      else
      {
         fprintf( yyc, "/* Empty source file */\n\n" );
      }
   }

   fclose( yyc );

   pFunc = hb_comp_functions.pFirst;
   while( pFunc )
   {
      pFunc = hb_compFunctionKill( pFunc );
   }

   pFunc = hb_comp_funcalls.pFirst;
   while( pFunc )
   {
      hb_comp_funcalls.pFirst = pFunc->pNext;
      hb_xfree( ( void * ) pFunc );  /* NOTE: szName will be released by hb_compSymbolKill() */
      pFunc = hb_comp_funcalls.pFirst;
   }

   pInline = hb_comp_inlines.pFirst;
   while( pInline )
   {
      hb_comp_inlines.pFirst = pInline->pNext;
      if( pInline->pCode )
      {
         hb_xfree( ( void * ) pInline->pCode );
      }
      hb_xfree( ( void * ) pInline->szFileName );
      hb_xfree( ( void * ) pInline );  /* NOTE: szName will be released by hb_compSymbolKill() */
      pInline = hb_comp_inlines.pFirst;
   }

   if ( hb_comp_iWarnings >= 3 )
   {
      pDeclared = hb_comp_pReleaseDeclared->pNext;
      while( pDeclared )
      {
         hb_comp_pFirstDeclared = pDeclared->pNext;
         hb_xfree( ( void * ) pDeclared );
         pDeclared = hb_comp_pFirstDeclared;
      }

      pClass = hb_comp_pReleaseClass->pNext;
      while( pClass )
      {
         hb_comp_pFirstClass = pClass->pNext;

         pDeclared = pClass->pMethod;
         while ( pDeclared )
         {
            hb_comp_pFirstDeclared = pDeclared->pNext;
            hb_xfree( ( void * ) pDeclared );
            pDeclared = hb_comp_pFirstDeclared;
         }

         hb_xfree( ( void * ) pClass );
         pClass = hb_comp_pFirstClass;
      }
   }

   pSym = hb_comp_symbols.pFirst;
   while( pSym )
   {
      pSym = hb_compSymbolKill( pSym );
   }

   pGlobal = hb_comp_pGlobals;
   while( pGlobal )
   {
      pDelete = pGlobal;
      pGlobal = pGlobal->pNext;
      hb_xfree( pDelete );
   }

   #ifndef HB_BACK_END
      #define HB_BACK_END 0
   #endif

   if( HB_BACK_END == 0 )
   {
      if( ! hb_comp_bQuiet )
      {
         printf( "Done.\n" );
      }
   }

   /*
    Close .p file
   */
   if ( hb_comp_iGenVarList )
   {
      fclose( hb_comp_pCodeList );
   }

   hb_xfree( hb_comp_PrgFileName );

   pTemp = pStatSymFirst;
   while( pTemp )
   {
      // printf( "RELEASING STATIC: >>%s<<\n", pTemp->szName );
      hb_xfree( pTemp->szName );
      pTemp = pTemp->pNext;
      hb_xfree( ( void * ) pStatSymFirst );
      pStatSymFirst = pTemp;
   }

   pTemp = pPubSymFirst;
   while( pTemp )
   {
      // printf( "RELEASING PUBLIC: >>%s<<\n", pTemp->szName );
      hb_xfree( pTemp->szName );
      pTemp = pTemp->pNext;
      hb_xfree( ( void * ) pPubSymFirst );
      pPubSymFirst = pTemp;
   }

   pTemp = pRequestList;
   while( pTemp )
   {
      // printf( "RELEASING EXTERN: >>%s<<\n", pTemp->szName );
      hb_xfree( pTemp->szName );
      pTemp = pTemp->pNext;
      hb_xfree( ( void * ) pRequestList );
      pRequestList = pTemp;
   }

}

/*
   Functions prototypes from dump areas
*/
static void hb_compGenCAddProtos( int iOption, FILE *yyc )
{
   PSSYMLIST pTemp = NULL;

   switch( iOption )
   {
      case 1 : /* REQUEST */
         pTemp = pRequestList;
         break;
      case 2 : /* HB_FUNC_STATIC */
         pTemp = pStatSymFirst;
         break;
      case 3 : /* HB_FUNC */
         pTemp = pPubSymFirst;
         break;
   }

   while( pTemp )
   {
      if(!hb_compSymbFound( pTemp->szName ))
      {
         if ( iOption == 1 )
         {
            if (!hb_compCStaticSymbolFound( pTemp->szName, TRUE ))
            {
               fprintf( yyc, "HB_FUNC_EXTERN( %s );\n",pTemp->szName);
            }
         }
         else if ( iOption == 2 )
         {
            fprintf( yyc, "HB_FUNC_STATIC( %s );\n", pTemp->szName );
         }
         else if ( iOption == 3 )
         {
            fprintf( yyc, "HB_FUNC( %s );\n", pTemp->szName );
         }
      }
      pTemp = pTemp->pNext;
   }
}

/*
  Write symbols to be initialized
*/
static void hb_compGenCAddExtern( int iOption, FILE *yyc, BOOL bSymFIRST )
{
   PSSYMLIST pTemp = NULL;
   BOOL bStart = FALSE;

   switch( iOption )
   {
      case 1 : /* REQUEST */
         pTemp = pRequestList;
         break;
      case 2 : /* HB_FUNC_STATIC */
         pTemp = pStatSymFirst;
         break;
      case 3 : /* HB_FUNC */
         pTemp = pPubSymFirst;
         break;
   }

   while( pTemp )
   {
      if(!hb_compSymbFound( pTemp->szName ))
      {
         if ( !bStart )
         {
            fprintf( yyc, ",\n" );
            bStart = TRUE;
         }

         if ( iOption == 1 )
         {
            fprintf( yyc, "{ \"%s\", HB_FS_PUBLIC",pTemp->szName );
         }
         else if ( iOption == 2 )
         {
            fprintf( yyc, "{ \"%s\",HB_FS_STATIC",pTemp->szName );
         }
         else
         {
            fprintf( yyc, "{ \"%s\", HB_FS_PUBLIC",pTemp->szName );
            if ( ( pTemp->pNext == NULL ) && ( !bSymFIRST ) &&  ( ! hb_comp_bNoStartUp ) )
            {
               fprintf( yyc, " | HB_FS_FIRST" );
            }
         }

         fprintf( yyc, ", {HB_FUNCNAME( %s )},",pTemp->szName );

         if ( iOption == 1 )
         {
            fprintf( yyc, " NULL }" );
         }
         else
         {
            fprintf( yyc, " (PHB_DYNS) 1 }" );
         }
      }

      pTemp = pTemp->pNext;

      if( pTemp && !hb_compSymbFound( pTemp->szName ))
      {
         if ( !bStart )
         {
            bStart = TRUE;
         }
         fprintf( yyc, ",\n" );
      }
   }
}

static void hb_writeEndInit( FILE* yyc )
{
   fprintf( yyc, "\nHB_INIT_SYMBOLS_END( hb_vm_SymbolInit_%s%s )\n\n"
                 "#if defined(HB_PRAGMA_STARTUP)\n"
                 "   #pragma startup hb_vm_SymbolInit_%s%s\n"
                 "#elif defined(HB_MSC_STARTUP)\n"
                 "   #if _MSC_VER >= 1010\n"
                 /* [pt] First version of MSC I have that supports this */
                 /* is msvc4.1 (which is msc 10.10) */
                 "      #pragma data_seg( \".CRT$XIY\" )\n"
                 "      #pragma comment( linker, \"/Merge:.CRT=.data\" )\n"
                 "   #else\n"
                 "      #pragma data_seg( \"XIY\" )\n"
                 "   #endif\n"
                 "   static HB_$INITSYM hb_vm_auto_SymbolInit_%s%s = hb_vm_SymbolInit_%s%s;\n"
                 "   #pragma data_seg()\n"
                 "#endif\n\n",
                 hb_comp_szPrefix, hb_comp_FileAsSymbol,
                 hb_comp_szPrefix, hb_comp_FileAsSymbol,
                 hb_comp_szPrefix, hb_comp_FileAsSymbol,
                 hb_comp_szPrefix, hb_comp_FileAsSymbol );
}

/*
  Searching for function names in glocal symbol list
*/
static BOOL hb_compSymbFound( char* szSymbol )
{
   BOOL bStatSymFound = FALSE;
   PCOMSYMBOL pSym_ = hb_comp_symbols.pFirst;

   while( pSym_ )
   {
      if( strcmp( pSym_->szName, szSymbol ) == 0 )
      {
         bStatSymFound = TRUE;
         break;
      }
      pSym_ = pSym_->pNext;
   }

   return bStatSymFound;
}

/*
  Searching for function names in in-line-c for writing prototypes
*/
static BOOL hb_compCStaticSymbolFound( char* szSymbol, BOOL bSearchStatic )
{
   BOOL bStatSymFound = FALSE;
   PSSYMLIST pStatSymTemp = bSearchStatic ? pStatSymFirst : pPubSymFirst;

   while( pStatSymTemp )
   {
      if( strcmp( pStatSymTemp->szName, szSymbol ) == 0 )
      {
         bStatSymFound = TRUE;
         break;
      }
      pStatSymTemp = pStatSymTemp->pNext;
   }

   return bStatSymFound;
}

/*
  Collecting function names from in-line-c. There are two categories, ie
  statics (HB_FUNC_STATIC) and publics (HB_FUNC)
*/
static void hb_compCStatSymList( char* statSymName, int iOption )
{
   PSSYMLIST pStatSymLast = NULL;
   int ulLen = strlen( statSymName );

   if ( iOption < 3 ) // It is not HB_FUNCNAME
   {
      pStatSymLast = (PSSYMLIST) hb_xgrab( sizeof( SSYMLIST ) );
   }

   while( ulLen && HB_ISSPACE( statSymName[ ulLen - 1 ] ) )
   {
      ulLen--;
   }

   if ( iOption < 3 ) // It is not HB_FUNCNAME
   {
      statSymName[ ulLen ] = '\0';
      pStatSymLast->szName = (char*) hb_xgrab( strlen( statSymName ) + 1 );
      strcpy( pStatSymLast->szName, statSymName );
   }

   if( iOption == 1 ) // HB_FUNC_STATIC
   {
      pStatSymLast->pNext = pStatSymFirst ? pStatSymFirst : NULL ;
      pStatSymFirst = pStatSymLast;
   }
   else if( iOption == 2 ) // HB_FUNC
   {
      pStatSymLast->pNext = pPubSymFirst ? pPubSymFirst : NULL;
      pPubSymFirst = pStatSymLast;
   }
   else if( iOption == 3 ) // HB_FUNCNAME
   {
      hb_compRequestList( statSymName );
   }
}

/*
  Parsing in-line-c codes to extract function names
*/
static void hb_compGenCCheckInLineStatic( char *str )
{
   char *szStrip = hb_stripOutComments( str );
   char *szTmp, *szTmp2;
   int iLen = strlen( szStrip );
   int iOption;

   while( ( szStrip = strstr( szStrip, "HB_FUNC" ) ) != NULL )
   {
      szStrip += 7;
      iOption = 2;

      /* If it is a PHB_FUNC then skip it */
      if ( iLen > 8 && *(szStrip - 8 ) == 'P' )
      {
         continue;
      }
      /* If it is a HB_FUNCNAME, we want it for externs */
      else if ( szStrip[0] == 'N' &&
                szStrip[1] == 'A' &&
                szStrip[2] == 'M' &&
                szStrip[3] == 'E' )
      {
         iOption = 3;
         szStrip += 4;
      }
      /* If it is a HB_FUNC_PTR then skip it */
      else if ( szStrip[0] == '_' &&
                szStrip[1] == 'P' &&
                szStrip[2] == 'T' &&
                szStrip[3] == 'R' )
      {
         szStrip += 4;
         continue;
      }
      /* If it is a HB_FUNC_EXTERN then skip it */
      else if ( szStrip[0] == '_' &&
                szStrip[1] == 'E' &&
                szStrip[2] == 'X' &&
                szStrip[3] == 'T' &&
                szStrip[4] == 'E' &&
                szStrip[5] == 'R' &&
                szStrip[6] == 'N' )
      {
         szStrip += 7;
         continue;
      }
      /* If it is a HB_FUNC_EXEC then skip it */
      else if ( szStrip[0] == '_' &&
                szStrip[1] == 'E' &&
                szStrip[2] == 'X' &&
                szStrip[3] == 'E' &&
                szStrip[4] == 'C' )
      {
         szStrip += 5;
         continue;
      }
      /* If it is a HB_FUNC_STATIC we want it */
      else if ( szStrip[0] == '_' &&
                szStrip[1] == 'S' &&
                szStrip[2] == 'T' &&
                szStrip[3] == 'A' &&
                szStrip[4] == 'T' &&
                szStrip[5] == 'I' &&
                szStrip[6] == 'C' )
      {
         iOption = 1;
         szStrip += 7;
      }

      szTmp = strchr( szStrip, '(' );
      if( szTmp == NULL )
      {
         continue;
      }
      szTmp++;

      while( HB_ISSPACE( *szTmp ) )
      {
         szTmp++;
      }

      szTmp2 = strchr( szTmp, ')' );
      if( szTmp == NULL )
      {
         continue;
      }

      *szTmp2 = '\0';
      hb_compCStatSymList( szTmp, iOption );
      *szTmp2 = ')';

      szStrip = szTmp2 + 1;
   }

   if ( szStrip )
   {
      hb_xfree( szStrip );
   }
}

/*
  Grab the content of in-line-c codes to be parse for function names
*/
static void hb_compGenCInLineSymbol()
{
   PINLINE pInline = hb_comp_inlines.pFirst;
   char *sInline;

   while( pInline )
   {
      sInline = (char*) hb_xgrab( strlen( (char*) pInline->pCode ) + 1 );
      strcpy( sInline, (char*) pInline->pCode );
      hb_compGenCCheckInLineStatic( sInline );

      hb_xmemset( sInline, '\0', strlen( (char*) pInline->pCode ) + 1 );
      strcpy( sInline, (char*) pInline->pCode );

      hb_comParseLine( sInline, FALSE );

      pInline = pInline->pNext;

      hb_xfree( sInline );
   }
}

/*
  "Copy & Paste" the contents of in-line-c to C file output
*/
static void hb_compGenCInLine( FILE *yyc )
{
   PINLINE pInline = hb_comp_inlines.pFirst;
   char *pszFileName;
   char *pszInLine;

   while( pInline )
   {
      fprintf( yyc, "#line %i \"", ( pInline->iLine + 1 ) );

      pszFileName = pInline->szFileName;

      while( *pszFileName )
      {
         if( *pszFileName == '\\' )
         {
            fprintf( yyc, "\\" );
         }
         fprintf( yyc, "%c", *pszFileName++ );
      }

      fprintf( yyc, "\"\n" );

      if( pInline->szName )
      {
         fprintf( yyc, "HB_FUNC_STATIC( %s )\n", pInline->szName );
      }

      /* parse for REQUEST before writing */
      pszInLine = hb_comParseLine( (char*) pInline->pCode, TRUE );
      if( hb_comp_Failure )
      {
         break;
      }
      fprintf( yyc, "%s", pszInLine );
      hb_xfree( pszInLine );
      pInline = pInline->pNext;
   }
}

/*
  Parse in-line-code here
*/
static char* hb_comParseLine( char *sz, BOOL bRetSz )
{
  int i = strlen( sz ), j = 0 , t = 0;
  BOOL bExtern;
  char *szReturn = NULL;
  char *szInLine = ( char* ) hb_xgrab( i + 1 );

  hb_xmemset( szInLine, 0, i + 1 );

  if( bRetSz )
  {
     szReturn = ( char* ) hb_xgrab( i + 1024 );
     hb_xmemset( szReturn, 0, i + 1 );
  }

  while( t < i )
  {
    if ( sz [ t ] == '\n' || sz [ t ] == '\r' )
    {
       j = 0;

       if ( sz [ t ] == '\r' )
       {
          t ++;
       }
    }
    else
    {
       szInLine [ j ] = sz[ t ];
       j ++;
    }

    t ++;

    if ( j == 0 || t == i )
    {
       /* parse line here */
       bExtern = hb_compNewLine( szInLine, bRetSz ) ;

       if( hb_comp_Failure )
       {
          break;
       }

       if ( bRetSz )
       {
          if ( ! bExtern )
          {
             hb_xstrcat( szReturn, szInLine, "\n", NULL );
          }
          else
          {
             /* It is a REQUEST in C code, comment it */
             hb_xstrcat( szReturn, "// ", szInLine, "\n", NULL );
          }
       }

       hb_xmemset( szInLine, 0, i + 1 );
    }
  }

  hb_xfree( szInLine );

  return ( szReturn );
}

/*
  search for REQUEST or EXTERNAL
*/
static BOOL hb_compNewLine( char * sz, BOOL bRetSz )
{
   char **szExtern;
   int iExtern = 0, iToken;
   BOOL bExtern = FALSE;

   while( HB_ISSPACE( *sz ) )
   {
      sz++;
   }

   if( sz && *sz )
   {
      if ( strlen( sz ) > 9 )
      {
         if ( toupper( sz[0] ) == 'R' &&
              toupper( sz[1] ) == 'E' &&
              toupper( sz[2] ) == 'Q' &&
              toupper( sz[3] ) == 'U' &&
              toupper( sz[4] ) == 'E' &&
              toupper( sz[5] ) == 'S' &&
              toupper( sz[6] ) == 'T' &&
              sz[7] == ' ' )
            {
               sz += 8;
               bExtern = TRUE;
            }
         else if ( toupper( sz[0] ) == 'E' &&
                   toupper( sz[1] ) == 'X' &&
                   toupper( sz[2] ) == 'T' &&
                   toupper( sz[3] ) == 'E' &&
                   toupper( sz[4] ) == 'R' &&
                   toupper( sz[5] ) == 'N' &&
                   toupper( sz[6] ) == 'A' &&
                   toupper( sz[7] ) == 'L' &&
                   sz[8] == ' ' )
            {
               sz += 9;
               bExtern = TRUE;
            }

         if ( bExtern )
         {
            while( HB_ISSPACE( *sz ) )
            {
               sz++;
            }

            if ( !bRetSz )
            {
               szExtern = hb_comExternSplit( (char*) sz, &iExtern );

               for (iToken = 0; szExtern [iToken]; iToken++)
               {
                  int i = hb_strAt( "/*", 2, szExtern [iToken], strlen(szExtern [iToken]) );
                  int j = hb_strAt( "*/", 2, szExtern [iToken], strlen(szExtern [iToken]) );
                  int t = hb_strAt( "//", 2, szExtern [iToken], strlen(szExtern [iToken]) );

                  if( t )
                  {
                     int ulLen;

                     szExtern[iToken][t - 1] = 0;
                     ulLen = strlen( szExtern[iToken] );

                     while( ulLen && HB_ISSPACE( szExtern[iToken][ ulLen - 1 ] ) )
                     {
                        ulLen--;
                     }

                     szExtern[iToken][ ulLen ] = 0;
                  }

                  if( i && j )
                  {
                     int ulLen;
                     szExtern [iToken][i - 1] = 0;
                     ulLen = strlen( szExtern[iToken] );

                     while( ulLen && HB_ISSPACE( szExtern[iToken][ulLen - 1] ) )
                     {
                        ulLen--;
                     }
                     szExtern [iToken][ulLen] = 0;
                  }

                  if( i && !j )
                  {
                     hb_comp_Failure = TRUE;
                     break;
                  }

                  hb_compRequestList( szExtern [iToken] );
               }

               szExtern--;
               hb_xfree( szExtern [0] );
               hb_xfree( szExtern );
            }
         }
      }
   }

   return ( bExtern );
}

/*
  Add REQUESTs to array
*/
static void hb_compRequestList( char* szExternName )
{
   PSSYMLIST pCheck = pRequestList;
   BOOL bFound = FALSE;

   while( pCheck )
   {
      if( strcmp( pCheck->szName, szExternName ) )
      {
         pCheck = pCheck->pNext;
      }
      else
      {
         bFound = TRUE;
         break;
      }
   }

   if ( ! bFound )
   {
      PSSYMLIST pExternLast = (PSSYMLIST) hb_xgrab( sizeof( SSYMLIST ) );

      pExternLast->szName = (char*) hb_xgrab( strlen( szExternName ) + 1 );
      strcpy( pExternLast->szName, szExternName );

      pExternLast->pNext = pRequestList ? pRequestList: NULL;
      pRequestList = pExternLast;

   }
}

/*
  Getting REQUEST symbols
*/
static char ** hb_comExternSplit ( char *string, int *iWord )
{
   char *buffer = (char *) hb_xgrab( strlen( string ) + 1 );
   char *bufptr;
   char **extern_list;
   char last_char = '\0';
   int word_count = 0, word_nbr;

   bufptr = buffer;

   while ( *string )
   {
      if ( *string == ',' )
      {
         while ( *string == ',' )
         {
            string ++;
         }

         while( HB_ISSPACE( *string ) )
         {
            string ++;
         }

         if (bufptr > buffer)
         {
            word_count ++;
            last_char = *bufptr++ = '\0';
         }
      }
      else
      {
         last_char = *bufptr++ = toupper( *string++ );
      }
   }

   if (last_char > 0)
   {
      word_count++;
   }

   *bufptr = '\0';

   extern_list = (char **) hb_xgrab (sizeof (char *) * (word_count + 2));
   extern_list [0] = buffer;
   extern_list++;

   bufptr = buffer;

   for (word_nbr = 0; word_nbr < word_count; word_nbr++)
   {
      extern_list [word_nbr] = bufptr;
      bufptr += strlen (bufptr) + 1;
   }

   extern_list [word_count] = NULL;

   *iWord = word_count;

   return (extern_list);
}

static HB_GENC_FUNC( hb_p_and )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_AND,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_arraypush )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_ARRAYPUSH,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_arraypop )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_ARRAYPOP,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_dec )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_DEC,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_arraydim )
{
   fprintf( cargo->yyc, "\tHB_P_ARRAYDIM, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   if( cargo->bVerbose ) fprintf( cargo->yyc, "\t/* %i */", HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) ) );
   fprintf( cargo->yyc, "\n" );
   return 3;
}

static HB_GENC_FUNC( hb_p_divide )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_DIVIDE,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_do )
{
   fprintf( cargo->yyc, "\tHB_P_DO, %i, %i,\n",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   return 3;
}

static HB_GENC_FUNC( hb_p_doshort )
{
   fprintf( cargo->yyc, "\tHB_P_DOSHORT, %i,\n", pFunc->pCode[ lPCodePos + 1 ] );
   return 2;
}

static HB_GENC_FUNC( hb_p_duplicate )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_DUPLICATE,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_dupltwo )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_DUPLTWO,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_equal )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_EQUAL,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_exactlyequal )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_EXACTLYEQUAL,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_endblock )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   --cargo->iNestedCodeblock;
   fprintf( cargo->yyc, "\tHB_P_ENDBLOCK,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_endproc )
{
   if( (lPCodePos+1) == pFunc->lPCodePos )
   {
      fprintf( cargo->yyc, "\tHB_P_ENDPROC\n" );
   }
   else
   {
      fprintf( cargo->yyc, "\tHB_P_ENDPROC,\n" );
   }
   return 1;
}

static HB_GENC_FUNC( hb_p_false )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_FALSE,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_fortest )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_FORTEST,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_frame )
{
   fprintf( cargo->yyc, "\tHB_P_FRAME, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );

   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* locals, params */" );
   }

   fprintf( cargo->yyc, "\n" );
   return 3;
}

static HB_GENC_FUNC( hb_p_funcptr )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_FUNCPTR,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_function )
{
   fprintf( cargo->yyc, "\tHB_P_FUNCTION, %i, %i,\n",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   return 3;
}

static HB_GENC_FUNC( hb_p_functionshort )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_FUNCTIONSHORT, %i,\n", pFunc->pCode[ lPCodePos + 1 ] );
   return 1;
}

static HB_GENC_FUNC( hb_p_arraygen )
{
   fprintf( cargo->yyc, "\tHB_P_ARRAYGEN, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );

   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %i */", HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) ) );
   }

   fprintf( cargo->yyc, "\n" );
   return 3;
}

static HB_GENC_FUNC( hb_p_greater )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_GREATER,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_greaterequal )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_GREATEREQUAL,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_inc )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_INC,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_instring )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_INSTRING,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_jumpnear )
{
   fprintf( cargo->yyc, "\tHB_P_JUMPNEAR, %i,",
            pFunc->pCode[ lPCodePos + 1 ] );
   if( cargo->bVerbose )
   {
      LONG lOffset = ( signed char ) ( pFunc->pCode[ lPCodePos + 1 ] );

      fprintf( cargo->yyc, "\t/* %li (abs: %05li) */", lOffset, ( LONG ) ( lPCodePos + lOffset ) );
   }
   fprintf( cargo->yyc, "\n" );
   return 2;
}

static HB_GENC_FUNC( hb_p_jump )
{
   fprintf( cargo->yyc, "\tHB_P_JUMP, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   if( cargo->bVerbose )
   {
      LONG lOffset = HB_PCODE_MKSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) );
      fprintf( cargo->yyc, "\t/* %li (abs: %05li) */", lOffset, ( LONG ) ( lPCodePos + lOffset ) );
   }
   fprintf( cargo->yyc, "\n" );
   return 3;
}

static HB_GENC_FUNC( hb_p_jumpfar )
{
   fprintf( cargo->yyc, "\tHB_P_JUMPFAR, %i, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ],
            pFunc->pCode[ lPCodePos + 3 ] );
   if( cargo->bVerbose )
   {
      LONG lOffset = HB_PCODE_MKINT24( &( pFunc->pCode[ lPCodePos + 1 ] ) );
      fprintf( cargo->yyc, "\t/* %li (abs: %08li) */", lOffset, ( LONG ) ( lPCodePos + lOffset ) );
   }
   fprintf( cargo->yyc, "\n" );
   return 4;
}

static HB_GENC_FUNC( hb_p_jumpfalsenear )
{
   fprintf( cargo->yyc, "\tHB_P_JUMPFALSENEAR, %i,",
            pFunc->pCode[ lPCodePos + 1 ] );
   if( cargo->bVerbose )
   {
      LONG lOffset = ( signed char ) ( pFunc->pCode[ lPCodePos + 1 ] );
      fprintf( cargo->yyc, "\t/* %li (abs: %05li) */", lOffset, ( LONG ) ( lPCodePos + lOffset ) );
   }
   fprintf( cargo->yyc, "\n" );
   return 2;
}

static HB_GENC_FUNC( hb_p_jumpfalse )
{
   fprintf( cargo->yyc, "\tHB_P_JUMPFALSE, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   if( cargo->bVerbose )
   {
      LONG lOffset = HB_PCODE_MKSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) );
      fprintf( cargo->yyc, "\t/* %li (abs: %05li) */", lOffset, ( LONG ) ( lPCodePos + lOffset ) );
   }
   fprintf( cargo->yyc, "\n" );
   return 3;
}

static HB_GENC_FUNC( hb_p_jumpfalsefar )
{
   fprintf( cargo->yyc, "\tHB_P_JUMPFALSEFAR, %i, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ],
            pFunc->pCode[ lPCodePos + 3 ] );
   if( cargo->bVerbose )
   {
      LONG lOffset = HB_PCODE_MKINT24( &( pFunc->pCode[ lPCodePos + 1 ] ) );
      fprintf( cargo->yyc, "\t/* %li (abs: %08li) */", lOffset, ( LONG ) ( lPCodePos + lOffset ) );
   }
   fprintf( cargo->yyc, "\n" );
   return 4;
}

static HB_GENC_FUNC( hb_p_jumptruenear )
{
   fprintf( cargo->yyc, "\tHB_P_JUMPTRUENEAR, %i,",
            pFunc->pCode[ lPCodePos + 1 ] );
   if( cargo->bVerbose )
   {
      LONG lOffset = ( signed char ) ( pFunc->pCode[ lPCodePos + 1 ] );
      fprintf( cargo->yyc, "\t/* %li (abs: %05li) */", lOffset, ( LONG ) ( lPCodePos + lOffset ) );
   }
   fprintf( cargo->yyc, "\n" );
   return 2;
}

static HB_GENC_FUNC( hb_p_jumptrue )
{
   fprintf( cargo->yyc, "\tHB_P_JUMPTRUE, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   if( cargo->bVerbose )
   {
      LONG lOffset = HB_PCODE_MKSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) );
      fprintf( cargo->yyc, "\t/* %li (abs: %05li) */", lOffset, ( LONG ) ( lPCodePos + lOffset ) );
   }
   fprintf( cargo->yyc, "\n" );
   return 3;
}

static HB_GENC_FUNC( hb_p_jumptruefar )
{
   fprintf( cargo->yyc, "\tHB_P_JUMPTRUEFAR, %i, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ],
            pFunc->pCode[ lPCodePos + 3 ] );
   if( cargo->bVerbose )
   {
      LONG lOffset = HB_PCODE_MKINT24( &( pFunc->pCode[ lPCodePos + 1 ] ) );
      fprintf( cargo->yyc, "\t/* %li (abs: %08li) */", lOffset, ( LONG ) ( lPCodePos + lOffset ) );
   }
   fprintf( cargo->yyc, "\n" );
   return 4;
}

static HB_GENC_FUNC( hb_p_less )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_LESS,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_lessequal )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_LESSEQUAL,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_line )
{
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "/* %05li */ ", lPCodePos );
   }
   else
   {
      fprintf( cargo->yyc, "\t" );
   }

   fprintf( cargo->yyc, "HB_P_LINE, %i, %i,", pFunc->pCode[ lPCodePos + 1 ], pFunc->pCode[ lPCodePos + 2 ] );

   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %i */", HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) ) );
   }

   fprintf( cargo->yyc, "\n" );

   return 3;
}

static HB_GENC_FUNC( hb_p_localname )
{
   ULONG ulStart = lPCodePos;

   fprintf( cargo->yyc, "\tHB_P_LOCALNAME, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );

   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %s */", ( char * ) pFunc->pCode + lPCodePos + 3 );
   }

   fprintf( cargo->yyc, "\n" );
   lPCodePos += 3;
   while( pFunc->pCode[ lPCodePos ] )
   {
      char chr = pFunc->pCode[ lPCodePos++ ];
      if( chr == '\'' || chr == '\\')
      {
         fprintf( cargo->yyc, " \'\\%c\',", chr );
      }
      else
      {
         fprintf( cargo->yyc, " \'%c\',", chr );
      }
   }
   fprintf( cargo->yyc, " 0,\n" );

   return (USHORT) ( lPCodePos - ulStart + 1 );
}

static HB_GENC_FUNC( hb_p_macropop )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_MACROPOP, %i,\n", pFunc->pCode[ lPCodePos + 1 ] );
   return 2;
}

static HB_GENC_FUNC( hb_p_macropopaliased )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_MACROPOPALIASED, %i,\n", pFunc->pCode[ lPCodePos + 1 ] );
   return 2;
}

static HB_GENC_FUNC( hb_p_macropush )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_MACROPUSH, %i,\n", pFunc->pCode[ lPCodePos + 1 ] );
   return 2;
}

static HB_GENC_FUNC( hb_p_macropusharg )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_MACROPUSHARG, %i,\n", pFunc->pCode[ lPCodePos + 1 ] );
   return 2;
}

static HB_GENC_FUNC( hb_p_macropushlist )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_MACROPUSHLIST, %i,\n", pFunc->pCode[ lPCodePos + 1 ] );
   return 2;
}

static HB_GENC_FUNC( hb_p_macropushindex )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_MACROPUSHINDEX, %i,\n", pFunc->pCode[ lPCodePos + 1 ] );
   return 2;
}

static HB_GENC_FUNC( hb_p_macropushpare )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_MACROPUSHPARE, %i,\n", pFunc->pCode[ lPCodePos + 1 ] );
   return 2;
}

static HB_GENC_FUNC( hb_p_macropushaliased )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_MACROPUSHALIASED, %i,\n", pFunc->pCode[ lPCodePos + 1 ] );
   return 2;
}

static HB_GENC_FUNC( hb_p_macrosymbol )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_MACROSYMBOL,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_macrotext )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_MACROTEXT,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_message )
{
   fprintf( cargo->yyc, "\tHB_P_MESSAGE, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %s */", hb_compSymbolGetPos( HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) ) )->szName );
   }
   fprintf( cargo->yyc, "\n" );
   return 3;
}

static HB_GENC_FUNC( hb_p_minus )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_MINUS,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_modulename )
{
   ULONG ulStart = lPCodePos;

   fprintf( cargo->yyc, "\tHB_P_MODULENAME," );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %s */", ( char * ) pFunc->pCode + lPCodePos + 1 );
   }
   fprintf( cargo->yyc, "\n" );
   lPCodePos++;
   while( pFunc->pCode[ lPCodePos ] )
   {
      char chr = pFunc->pCode[ lPCodePos++ ];
      if( chr == '\'' || chr == '\\')
      {
         fprintf( cargo->yyc, " \'\\%c\',", chr );
      }
      else
      {
         fprintf( cargo->yyc, " \'%c\',", chr );
      }
   }
   fprintf( cargo->yyc, " 0,\n" );

   return (USHORT) ( lPCodePos - ulStart + 1 );
}

static HB_GENC_FUNC( hb_p_modulus )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_MODULUS,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_mult )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_MULT,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_negate )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_NEGATE,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_not )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_NOT,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_notequal )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_NOTEQUAL,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_or )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_OR,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_parameter )
{
   fprintf( cargo->yyc, "\tHB_P_PARAMETER, %i, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ],
            pFunc->pCode[ lPCodePos + 3 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %s */", hb_compSymbolGetPos( HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) ) )->szName );
   }
   fprintf( cargo->yyc, "\n" );
   return 4;
}

static HB_GENC_FUNC( hb_p_plus )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_PLUS,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_pop )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_POP,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_popalias )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_POPALIAS,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_popaliasedfield )
{
   fprintf( cargo->yyc, "\tHB_P_POPALIASEDFIELD, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %s */", hb_compSymbolGetPos( HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) ) )->szName );
   }
   fprintf( cargo->yyc, "\n" );
   return 3;
}

static HB_GENC_FUNC( hb_p_popaliasedfieldnear )
{
   fprintf( cargo->yyc, "\tHB_P_POPALIASEDFIELDNEAR, %i,",
            pFunc->pCode[ lPCodePos + 1 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %s */", hb_compSymbolGetPos( pFunc->pCode[ lPCodePos + 1 ] )->szName );
   }
   fprintf( cargo->yyc, "\n" );
   return 2;
}

static HB_GENC_FUNC( hb_p_popaliasedvar )
{
   fprintf( cargo->yyc, "\tHB_P_POPALIASEDVAR, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %s */", hb_compSymbolGetPos( HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) ) )->szName );
   }
   fprintf( cargo->yyc, "\n" );
   return 3;
}

static HB_GENC_FUNC( hb_p_popfield )
{
   fprintf( cargo->yyc, "\tHB_P_POPFIELD, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %s */", hb_compSymbolGetPos( HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) ) )->szName );
   }
   fprintf( cargo->yyc, "\n" );
   return 3;
}

static HB_GENC_FUNC( hb_p_poplocal )
{
   fprintf( cargo->yyc, "\tHB_P_POPLOCAL, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   if( cargo->bVerbose )
   {
      int iVar = HB_PCODE_MKSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) );

      /* Variable with negative order are local variables
       * referenced in a codeblock -handle it with care
       */
      if( cargo->iNestedCodeblock )
      {
         /* we are accesing variables within a codeblock */
         /* the names of codeblock variable are lost     */
         if( iVar < 0 )
         {
            fprintf( cargo->yyc, "\t/* localvar%i */", -iVar );
         }
         else
         {
            fprintf( cargo->yyc, "\t/* codeblockvar%i */", iVar );
         }
      }
      else
      {
         fprintf( cargo->yyc, "\t/* %s */", hb_compLocalVariableFind( pFunc, iVar )->szName );
      }
   }

   fprintf( cargo->yyc, "\n" );

   return 3;
}

static HB_GENC_FUNC( hb_p_poplocalnear )
{
   fprintf( cargo->yyc, "\tHB_P_POPLOCALNEAR, %i,",
            pFunc->pCode[ lPCodePos + 1 ] );

   if( cargo->bVerbose )
   {
      int iVar = (int) (signed char) pFunc->pCode[ lPCodePos + 1 ];

      /* Variable with negative order are local variables
         * referenced in a codeblock -handle it with care
         */

      if( cargo->iNestedCodeblock )
      {
         /* we are accesing variables within a codeblock */
         /* the names of codeblock variable are lost     */
         if( iVar < 0 )
         {
            fprintf( cargo->yyc, "\t/* localvar%i */", -iVar );
         }
         else
         {
            fprintf( cargo->yyc, "\t/* codeblockvar%i */", iVar );
         }
      }
      else
      {
         fprintf( cargo->yyc, "\t/* %s */", hb_compLocalVariableFind( pFunc, iVar )->szName );
      }
   }

   fprintf( cargo->yyc, "\n" );

   return 2;
}

static HB_GENC_FUNC( hb_p_popmemvar )
{
   fprintf( cargo->yyc, "\tHB_P_POPMEMVAR, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );

   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %s */", hb_compSymbolGetPos( HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) ) )->szName );
   }

   fprintf( cargo->yyc, "\n" );

   return 3;
}

static HB_GENC_FUNC( hb_p_popstatic )
{
   fprintf( cargo->yyc, "\tHB_P_POPSTATIC, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );

   if( cargo->bVerbose )
   {
      PVAR pVar;
      PFUNCTION pTmp = hb_comp_functions.pFirst;
      USHORT wVar = HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) );

      while( pTmp->pNext && pTmp->pNext->iStaticsBase < wVar )
      {
         pTmp = pTmp->pNext;
      }

      pVar = hb_compVariableFind( pTmp->pStatics, wVar - pTmp->iStaticsBase );

      fprintf( cargo->yyc, "\t/* %s */", pVar->szName );
   }
   fprintf( cargo->yyc, "\n" );
   return 3;
}

static HB_GENC_FUNC( hb_p_popvariable )
{
   fprintf( cargo->yyc, "\tHB_P_POPVARIABLE, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %s */", hb_compSymbolGetPos( HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) ) )->szName  );
   }
   fprintf( cargo->yyc, "\n" );
   return 3;
}

static HB_GENC_FUNC( hb_p_power )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_POWER,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_pushalias )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_PUSHALIAS,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_pushaliasedfield )
{
   fprintf( cargo->yyc, "\tHB_P_PUSHALIASEDFIELD, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %s */", hb_compSymbolGetPos( HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) ) )->szName );
   }
   fprintf( cargo->yyc, "\n" );
   return 3;
}

static HB_GENC_FUNC( hb_p_pushaliasedfieldnear )
{
   fprintf( cargo->yyc, "\tHB_P_PUSHALIASEDFIELDNEAR, %i,",
            pFunc->pCode[ lPCodePos + 1 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %s */", hb_compSymbolGetPos( pFunc->pCode[ lPCodePos + 1 ] )->szName );
   }
   fprintf( cargo->yyc, "\n" );
   return 2;
}

static HB_GENC_FUNC( hb_p_pushaliasedvar )
{
   fprintf( cargo->yyc, "\tHB_P_PUSHALIASEDVAR, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %s */", hb_compSymbolGetPos( HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) ) )->szName );
   }
   fprintf( cargo->yyc, "\n" );
   return 3;
}

static HB_GENC_FUNC( hb_p_pushblock )
{
   USHORT wVar, w;
   ULONG ulStart = lPCodePos;

   ++cargo->iNestedCodeblock;

   fprintf( cargo->yyc, "\tHB_P_PUSHBLOCK, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %i */",
               HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) ) );
   }
   fprintf( cargo->yyc, "\n" );

   w = HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 3 ] ) );
   fprintf( cargo->yyc, "\t%i, %i,",
            pFunc->pCode[ lPCodePos + 3 ],
            pFunc->pCode[ lPCodePos + 4 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* number of local parameters (%i) */", w );
   }
   fprintf( cargo->yyc, "\n" );

   wVar = HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 5 ] ) );

   fprintf( cargo->yyc, "\t%i, %i,",
            pFunc->pCode[ lPCodePos + 5 ],
            pFunc->pCode[ lPCodePos + 6 ] );

   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* number of local variables (%i) */", wVar );
   }

   fprintf( cargo->yyc, "\n" );

   lPCodePos += 7;  /* codeblock size + number of parameters + number of local variables */

   /* create the table of referenced local variables */
   while( wVar-- )
   {
      w = HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos ] ) );
      fprintf( cargo->yyc, "\t%i, %i,", pFunc->pCode[ lPCodePos ], pFunc->pCode[ lPCodePos + 1 ] );

      /* NOTE:
         * When a codeblock is used to initialize a static variable
         * the names of local variables cannot be determined
         * because at the time of C code generation we don't know
         * in which function was defined this local variable
         */
      if( ( pFunc->cScope & HB_FS_INITEXIT ) != HB_FS_INITEXIT )
      {
         if( cargo->bVerbose )
         {
            fprintf( cargo->yyc, "\t/* %s */", hb_compLocalVariableFind( pFunc, w )->szName );
         }
      }

      fprintf( cargo->yyc, "\n" );
      lPCodePos += 2;
   }

   return (USHORT) (lPCodePos - ulStart);
}

static HB_GENC_FUNC( hb_p_pushblockshort )
{
   ++cargo->iNestedCodeblock;

   fprintf( cargo->yyc, "\tHB_P_PUSHBLOCKSHORT, %i,",
            pFunc->pCode[ lPCodePos + 1 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %i */",
               pFunc->pCode[ lPCodePos + 1 ] );
   }
   fprintf( cargo->yyc, "\n" );

   return 2;
}

static HB_GENC_FUNC( hb_p_pushdouble )
{
   int i;

   fprintf( cargo->yyc, "\tHB_P_PUSHDOUBLE," );
   ++lPCodePos;
   for( i = 0; i < ( int ) ( sizeof( double ) + sizeof( BYTE ) + sizeof( BYTE ) ); ++i )
   {
      fprintf( cargo->yyc, " %i,", ( ( BYTE * ) pFunc->pCode )[ lPCodePos + i ] );
   }
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %.*f, %d, %d */",
      *( ( BYTE * ) &( pFunc->pCode[ lPCodePos + sizeof( double ) ] ) ),
      HB_PCODE_MKDOUBLE( &( pFunc->pCode[ lPCodePos ] ) ),
      *( ( BYTE * ) &( pFunc->pCode[ lPCodePos + sizeof( double ) ] ) ),
      *( ( BYTE * ) &( pFunc->pCode[ lPCodePos + sizeof( double ) + sizeof( BYTE ) ] ) ) );
   }
   fprintf( cargo->yyc, "\n" );

   return sizeof( double ) + sizeof( BYTE ) + sizeof( BYTE ) + 1;
}

static HB_GENC_FUNC( hb_p_pushfield )
{
   fprintf( cargo->yyc, "\tHB_P_PUSHFIELD, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %s */", hb_compSymbolGetPos( HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) ) )->szName );
   }
   fprintf( cargo->yyc, "\n" );
   return 3;
}

static HB_GENC_FUNC( hb_p_pushbyte )
{
   fprintf( cargo->yyc, "\tHB_P_PUSHBYTE, %i,",
            pFunc->pCode[ lPCodePos + 1 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %i */",
               ( signed char ) pFunc->pCode[ lPCodePos + 1 ] );
   }
   fprintf( cargo->yyc, "\n" );
   return 2;
}

static HB_GENC_FUNC( hb_p_pushint )
{
   fprintf( cargo->yyc, "\tHB_P_PUSHINT, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %i */", HB_PCODE_MKSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) ) );
   }
   fprintf( cargo->yyc, "\n" );
   return 3;
}

static HB_GENC_FUNC( hb_p_pushlocal )
{
   fprintf( cargo->yyc, "\tHB_P_PUSHLOCAL, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );

   if( cargo->bVerbose )
   {
      int iVar = HB_PCODE_MKSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) );

      /* Variable with negative order are local variables
       * referenced in a codeblock -handle it with care
       */
      if( cargo->iNestedCodeblock )
      {
         /* we are accesing variables within a codeblock */
         /* the names of codeblock variable are lost     */
         if( iVar < 0 )
         {
            fprintf( cargo->yyc, "\t/* localvar%i */", -iVar );
         }
         else
         {
            fprintf( cargo->yyc, "\t/* codeblockvar%i */", iVar );
         }
      }
      else
      {
         fprintf( cargo->yyc, "\t/* %s */", hb_compLocalVariableFind( pFunc, iVar )->szName );
      }
   }

   fprintf( cargo->yyc, "\n" );

   return 3;
}

static HB_GENC_FUNC( hb_p_pushlocalnear )
{
   fprintf( cargo->yyc, "\tHB_P_PUSHLOCALNEAR, %i,",
            pFunc->pCode[ lPCodePos + 1 ] );

   if( cargo->bVerbose )
   {
      int iVar = (int )(signed char) pFunc->pCode[ lPCodePos + 1 ];

      /* Variable with negative order are local variables
       * referenced in a codeblock -handle it with care
       */

      if( cargo->iNestedCodeblock )
      {
         /* we are accesing variables within a codeblock */
         /* the names of codeblock variable are lost     */
         if( iVar < 0 )
         {
            fprintf( cargo->yyc, "\t/* localvar%i */", -iVar );
         }
         else
         {
            fprintf( cargo->yyc, "\t/* codeblockvar%i */", iVar );
         }
      }
      else
      {
         fprintf( cargo->yyc, "\t/* %s */", hb_compLocalVariableFind( pFunc, iVar )->szName );
      }
   }

   fprintf( cargo->yyc, "\n" );

   return 2;
}

static HB_GENC_FUNC( hb_p_pushlocalref )
{
   fprintf( cargo->yyc, "\tHB_P_PUSHLOCALREF, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );

   if( cargo->bVerbose )
   {
      SHORT iVar = HB_PCODE_MKSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) );

      /* Variable with negative order are local variables
       * referenced in a codeblock -handle it with care
       */
      if( cargo->iNestedCodeblock )
      {
         /* we are accesing variables within a codeblock */
         /* the names of codeblock variable are lost     */
         if( iVar < 0 )
         {
            fprintf( cargo->yyc, "\t/* localvar%i */", -iVar );
         }
         else
         {
            fprintf( cargo->yyc, "\t/* codeblockvar%i */", iVar );
         }
      }
      else
      {
         fprintf( cargo->yyc, "\t/* %s */", hb_compLocalVariableFind( pFunc, iVar )->szName );
      }
   }

   fprintf( cargo->yyc, "\n" );

   return 3;
}

static HB_GENC_FUNC( hb_p_pushlong )
{
   fprintf( cargo->yyc, "\tHB_P_PUSHLONG, %i, %i, %i, %i, ",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ],
            pFunc->pCode[ lPCodePos + 3 ],
            pFunc->pCode[ lPCodePos + 4 ] );

   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %li */", HB_PCODE_MKLONG( &( pFunc->pCode[ lPCodePos + 1 ] ) ) );
   }
   fprintf( cargo->yyc, "\n" );

   return 5;
}

static HB_GENC_FUNC( hb_p_pushlonglong )
{
   fprintf( cargo->yyc, "\tHB_P_PUSHLONGLONG, %i, %i, %i, %i, %i, %i, %i, %i, ",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ],
            pFunc->pCode[ lPCodePos + 3 ],
            pFunc->pCode[ lPCodePos + 4 ],
            pFunc->pCode[ lPCodePos + 5 ],
            pFunc->pCode[ lPCodePos + 6 ],
            pFunc->pCode[ lPCodePos + 7 ],
            pFunc->pCode[ lPCodePos + 8 ] );
   if( cargo->bVerbose )
   {
#ifdef HB_LONG_LONG_OFF
      fprintf( cargo->yyc, "\t/* %f */", HB_PCODE_MKLONGLONG( &( pFunc->pCode[ lPCodePos + 1 ] ) ) );
#else
      fprintf( cargo->yyc, "\t/* %" PFLL "i */", HB_PCODE_MKLONGLONG( &( pFunc->pCode[ lPCodePos + 1 ] ) ) );
#endif
   }
   fprintf( cargo->yyc, "\n" );

   return 9;
}

static HB_GENC_FUNC( hb_p_pushmemvar )
{
   fprintf( cargo->yyc, "\tHB_P_PUSHMEMVAR, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %s */", hb_compSymbolGetPos( HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) ) )->szName );
   }
   fprintf( cargo->yyc, "\n" );
   return 3;
}

static HB_GENC_FUNC( hb_p_pushmemvarref )
{
   fprintf( cargo->yyc, "\tHB_P_PUSHMEMVARREF, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %s */", hb_compSymbolGetPos( HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) ) )->szName );
   }
   fprintf( cargo->yyc, "\n" );
   return 3;
}

static HB_GENC_FUNC( hb_p_pushnil )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_PUSHNIL,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_pushself )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_PUSHSELF,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_pushstatic )
{
   fprintf( cargo->yyc, "\tHB_P_PUSHSTATIC, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   if( cargo->bVerbose )
   {
      PVAR pVar;
      PFUNCTION pTmp = hb_comp_functions.pFirst;
      USHORT wVar = HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) );

      while( pTmp->pNext && pTmp->pNext->iStaticsBase < wVar )
      {
         pTmp = pTmp->pNext;
      }

      pVar = hb_compVariableFind( pTmp->pStatics, wVar - pTmp->iStaticsBase );
      fprintf( cargo->yyc, "\t/* %s */", pVar->szName );
   }

   fprintf( cargo->yyc, "\n" );

   return 3;
}

static HB_GENC_FUNC( hb_p_pushstaticref )
{
   fprintf( cargo->yyc, "\tHB_P_PUSHSTATICREF, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   if( cargo->bVerbose )
   {
      PVAR pVar;
      PFUNCTION pTmp = hb_comp_functions.pFirst;
      USHORT wVar = HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) );

      while( pTmp->pNext && pTmp->pNext->iStaticsBase < wVar )
      {
         pTmp = pTmp->pNext;
      }

      pVar = hb_compVariableFind( pTmp->pStatics, wVar - pTmp->iStaticsBase );
      fprintf( cargo->yyc, "\t/* %s */", pVar->szName );
   }

   fprintf( cargo->yyc, "\n" );

   return 3;
}

static HB_GENC_FUNC( hb_p_pushstr )
{
   ULONG ulStart = lPCodePos;
   USHORT wLen = HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) );

   fprintf( cargo->yyc, "\tHB_P_PUSHSTR, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );

   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %i */", wLen );
   }

   lPCodePos += 3;
   if( wLen > 0 )
   {
      fprintf( cargo->yyc, "\n\t" );
      while( wLen-- )
      {
         BYTE uchr = ( BYTE ) pFunc->pCode[ lPCodePos++ ];
         /*
          * NOTE: After optimization some CHR(n) can be converted
          *    into a string containing nonprintable characters.
          *
          * TODO: add switch to use hexadecimal format "%#04x"
          */
         if( ( uchr < ( BYTE ) ' ' ) || ( uchr >= 127 ) )
         {
            fprintf( cargo->yyc, "%i, ", uchr );
         }
         else if( strchr( "\'\\\"", uchr ) )
         {
            fprintf( cargo->yyc, "%i, ", uchr );
         }
         else
         {
            fprintf( cargo->yyc, "\'%c\', ", uchr );
         }
      }
   }
   fprintf( cargo->yyc, "\n" );

   return (USHORT) (lPCodePos - ulStart);
}

static HB_GENC_FUNC( hb_p_pushstrshort )
{
   ULONG ulStart = lPCodePos;
   USHORT wLen = pFunc->pCode[ lPCodePos + 1 ];

   fprintf( cargo->yyc, "\tHB_P_PUSHSTRSHORT, %i,", pFunc->pCode[ lPCodePos + 1 ] );

   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %i */", wLen );
   }

   lPCodePos += 2;
   if( wLen > 0 )
   {
      fprintf( cargo->yyc, "\n\t" );
      while( wLen-- )
      {
         BYTE uchr = ( BYTE ) pFunc->pCode[ lPCodePos++ ];
         /*
            * NOTE: After optimization some CHR(n) can be converted
            *    into a string containing nonprintable characters.
            *
            * TODO: add switch to use hexadecimal format "%#04x"
            */
         if( ( uchr < ( BYTE ) ' ' ) || ( uchr >= 127 ) )
         {
            fprintf( cargo->yyc, "%i, ", uchr );
         }
         else if( strchr( "\'\\\"", uchr ) )
         {
            fprintf( cargo->yyc, "%i, ", uchr );
         }
         else
         {
            fprintf( cargo->yyc, "\'%c\', ", uchr );
         }
      }
   }
   fprintf( cargo->yyc, "\n" );

   return (USHORT) ( lPCodePos - ulStart );
}

static HB_GENC_FUNC( hb_p_pushsym )
{
   fprintf( cargo->yyc, "\tHB_P_PUSHSYM, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %s */", hb_compSymbolGetPos( HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) ) )->szName );
   }
   fprintf( cargo->yyc, "\n" );
   return 3;
}

static HB_GENC_FUNC( hb_p_pushsymnear )
{
   fprintf( cargo->yyc, "\tHB_P_PUSHSYMNEAR, %i,",
            pFunc->pCode[ lPCodePos + 1 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %s */", hb_compSymbolGetPos( pFunc->pCode[ lPCodePos + 1 ] )->szName );
   }
   fprintf( cargo->yyc, "\n" );
   return 2;
}

static HB_GENC_FUNC( hb_p_pushvariable )
{
   fprintf( cargo->yyc, "\tHB_P_PUSHVARIABLE, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %s */", hb_compSymbolGetPos( HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) ) )->szName );
   }
   fprintf( cargo->yyc, "\n" );
   return 3;
}

static HB_GENC_FUNC( hb_p_retvalue )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_RETVALUE,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_send )
{
   fprintf( cargo->yyc, "\tHB_P_SEND, %i, %i,\n",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   return 3;
}

static HB_GENC_FUNC( hb_p_sendshort )
{
   fprintf( cargo->yyc, "\tHB_P_SENDSHORT, %i,\n", pFunc->pCode[ lPCodePos + 1 ] );
   return 2;
}

static HB_GENC_FUNC( hb_p_seqbegin )
{
   fprintf( cargo->yyc, "\tHB_P_SEQBEGIN, %i, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ],
            pFunc->pCode[ lPCodePos + 3 ] );
   if( cargo->bVerbose )
   {
      LONG lOffset = HB_PCODE_MKINT24( &( pFunc->pCode[ lPCodePos + 1 ] ) );
      fprintf( cargo->yyc, "\t/* %li (abs: %08li) */", lOffset, lPCodePos + lOffset );
   }
   fprintf( cargo->yyc, "\n" );
   return 4;
}

static HB_GENC_FUNC( hb_p_seqend )
{
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "/* %05li */ ", lPCodePos );
   }
   else
   {
      fprintf( cargo->yyc, "\t" );
   }
   fprintf( cargo->yyc, "HB_P_SEQEND, %i, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ],
            pFunc->pCode[ lPCodePos + 3 ] );
   if( cargo->bVerbose )
   {
      LONG lOffset = HB_PCODE_MKINT24( &( pFunc->pCode[ lPCodePos + 1 ] ) );
      fprintf( cargo->yyc, "\t/* %li (abs: %08li) */", lOffset, lPCodePos + lOffset );
   }
   fprintf( cargo->yyc, "\n" );
   return 4;
}

static HB_GENC_FUNC( hb_p_seqrecover )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_SEQRECOVER,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_sframe )
{
   fprintf( cargo->yyc, "\tHB_P_SFRAME, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* symbol (_INITSTATICS) */" );
   }
   fprintf( cargo->yyc, "\n" );
   return 3;
}

static HB_GENC_FUNC( hb_p_statics )
{
   LONG lByteCount = 5;

   fprintf( cargo->yyc, "\tHB_P_STATICS, %i, %i, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ],
            pFunc->pCode[ lPCodePos + 3 ],
            pFunc->pCode[ lPCodePos + 4 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* symbol (_INITSTATICS), %i statics */", HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 3 ] ) ) );
   }
   fprintf( cargo->yyc, "\n" );
   return (USHORT) lByteCount;
}

static HB_GENC_FUNC( hb_p_staticname )
{
   ULONG ulStart = lPCodePos;

   fprintf( cargo->yyc, "\tHB_P_STATICNAME, %i, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ],
            pFunc->pCode[ lPCodePos + 3 ] );
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %s */", ( char * ) pFunc->pCode + lPCodePos + 4 );
   }
   fprintf( cargo->yyc, "\n" );
   lPCodePos += 4;
   while( pFunc->pCode[ lPCodePos ] )
   {
      char chr = pFunc->pCode[ lPCodePos++ ];
      if( chr == '\'' || chr == '\\')
      {
         fprintf( cargo->yyc, " \'\\%c\',", chr );
      }
      else
      {
         fprintf( cargo->yyc, " \'%c\',", chr );
      }
   }
   fprintf( cargo->yyc, " 0,\n" );

   return (USHORT) ( lPCodePos - ulStart + 1 );
}

static HB_GENC_FUNC( hb_p_swapalias )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_SWAPALIAS,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_true )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_TRUE,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_one )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_ONE,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_zero )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_ZERO,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_noop )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_NOOP,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_dummy )
{
   HB_SYMBOL_UNUSED( cargo );
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );
   return 1;
}

static HB_GENC_FUNC( hb_p_macrolist )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_MACROLIST,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_macrolistend )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_MACROLISTEND,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_localnearaddint )
{
   fprintf( cargo->yyc, "\tHB_P_LOCALNEARADDINT, %i, %i, %i,",
                        pFunc->pCode[ lPCodePos + 1 ],
                        pFunc->pCode[ lPCodePos + 2 ],
                        pFunc->pCode[ lPCodePos + 3 ] );

   if( cargo->bVerbose )
   {
      int iVar = (int) (signed char) pFunc->pCode[ lPCodePos + 1 ];

      if( cargo->iNestedCodeblock )
      {
         /* we are accesing variables within a codeblock */
         /* the names of codeblock variable are lost     */
         if( iVar < 0 )
         {
            fprintf( cargo->yyc, "\t/* localvar%i */", -iVar );
         }
         else
         {
            fprintf( cargo->yyc, "\t/* codeblockvar%i */", iVar );
         }
      }
      else
      {
         fprintf( cargo->yyc, "\t/* %s %i*/", hb_compLocalVariableFind( pFunc, iVar )->szName, iVar );
      }
   }

   fprintf( cargo->yyc, "\n" );

   return 4;
}

static HB_GENC_FUNC( hb_p_localnearsetint )
{
   fprintf( cargo->yyc, "\tHB_P_LOCALNEARSETINT, %i, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ],
            pFunc->pCode[ lPCodePos + 3 ] );

   if( cargo->bVerbose )
   {
      int iVar = (int) (signed char) pFunc->pCode[ lPCodePos + 1 ];

      if( cargo->iNestedCodeblock )
      {
         /* we are accesing variables within a codeblock */
         /* the names of codeblock variable are lost     */
         if( iVar < 0 )
         {
            fprintf( cargo->yyc, "\t/* localvar%i */", -iVar );
         }
         else
         {
            fprintf( cargo->yyc, "\t/* codeblockvar%i */", iVar );
         }
      }
      else
      {
         fprintf( cargo->yyc, "\t/* %s %i*/", hb_compLocalVariableFind( pFunc, iVar )->szName, HB_PCODE_MKSHORT( &( pFunc->pCode[ lPCodePos + 2 ] ) ) );
      }
   }

   fprintf( cargo->yyc, "\n" );

   return 4;
}

static HB_GENC_FUNC( hb_p_addint )
{
   fprintf( cargo->yyc, "\tHB_P_ADDINT, %i, %i,", pFunc->pCode[ lPCodePos + 1 ], pFunc->pCode[ lPCodePos + 2 ] );

   if( cargo->bVerbose )
   {
      int iVar = (int) (signed char) pFunc->pCode[ lPCodePos + 2 ];

      if( cargo->iNestedCodeblock )
      {
         /* we are accesing variables within a codeblock */
         /* the names of codeblock variable are lost     */
         if( iVar < 0 )
         {
            fprintf( cargo->yyc, "\t/* localvar%i */", -iVar );
         }
         else
         {
            fprintf( cargo->yyc, "\t/* codeblockvar%i */", iVar );
         }
      }
      else
      {
         fprintf( cargo->yyc, "\t/* %i*/", iVar );
      }
   }

   fprintf( cargo->yyc, "\n" );

   return 3;
}

static HB_GENC_FUNC( hb_p_localnearsetstr )
{

   ULONG ulStart = lPCodePos;
   USHORT uLen   = HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 2 ] ) );

   fprintf( cargo->yyc, "\tHB_P_LOCALNEARSETSTR, %i, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ],
            pFunc->pCode[ lPCodePos + 3 ] );

   if( cargo->bVerbose )
   {
      int iVar = (int) (signed char) pFunc->pCode[ lPCodePos + 1 ];

      if( cargo->iNestedCodeblock )
      {
         /* we are accesing variables within a codeblock */
         /* the names of codeblock variable are lost     */
         if( iVar < 0 )
         {
            fprintf( cargo->yyc, "\t/* localvar%i */", -iVar );
         }
         else
         {
            fprintf( cargo->yyc, "\t/* codeblockvar%i */", iVar );
         }
      }
      else
      {
         fprintf( cargo->yyc, "\t/* %s %i*/", hb_compLocalVariableFind( pFunc, iVar )->szName, uLen );
      }
   }

   lPCodePos += 4;

   if( uLen > 0 )
   {
      fprintf( cargo->yyc, "\n\t" );

      while( uLen-- )
      {
         BYTE uchr = ( BYTE ) pFunc->pCode[ lPCodePos++ ];
         /*
          * NOTE: After optimization some CHR(n) can be converted
          *    into a string containing nonprintable characters.
          *
          * TODO: add switch to use hexadecimal format "%#04x"
          */
         if( ( uchr < ( BYTE ) ' ' ) || ( uchr >= 127 ) )
         {
            fprintf( cargo->yyc, "%i, ", uchr );
         }
         else if( strchr( "\'\\\"", uchr ) )
         {
            fprintf( cargo->yyc, "%i, ", uchr );
         }
         else
         {
            fprintf( cargo->yyc, "\'%c\', ", uchr );
         }
      }
   }

   fprintf( cargo->yyc, "\n" );

   return ( USHORT ) ( lPCodePos - ulStart );
}

static HB_GENC_FUNC( hb_p_left )
{
   fprintf( cargo->yyc, "\tHB_P_LEFT, %i, %i,", pFunc->pCode[ lPCodePos + 1 ], pFunc->pCode[ lPCodePos + 2 ] );

   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %i */", HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) ) );
   }

   fprintf( cargo->yyc, "\n" );

   return 3;
}

static HB_GENC_FUNC( hb_p_right )
{
   fprintf( cargo->yyc, "\tHB_P_RIGHT, %i, %i,", pFunc->pCode[ lPCodePos + 1 ], pFunc->pCode[ lPCodePos + 2 ] );

   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %i */", HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) ) );
   }

   fprintf( cargo->yyc, "\n" );

   return 3;
}

static HB_GENC_FUNC( hb_p_substr )
{
   fprintf( cargo->yyc, "\tHB_P_SUBSTR, %i, %i, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ], pFunc->pCode[ lPCodePos + 2 ],
            pFunc->pCode[ lPCodePos + 3 ], pFunc->pCode[ lPCodePos + 4 ] );

   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %i %i*/",
               HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) ),
               HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 3 ] ) ) );
   }

   fprintf( cargo->yyc, "\n" );

   return 5;
}

static HB_GENC_FUNC( hb_p_lineoffset )
{
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "/* %05li */ ", lPCodePos );
   }
   else
   {
      fprintf( cargo->yyc, "\t" );
   }

   fprintf( cargo->yyc, "HB_P_LINEOFFSET, %i,", pFunc->pCode[ lPCodePos + 1 ] );

   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %i */", hb_comp_iBaseLine + pFunc->pCode[ lPCodePos + 1 ] );
   }
   fprintf( cargo->yyc, "\n" );

   return 2;
}

static HB_GENC_FUNC( hb_p_baseline )
{
   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "/* %05li */ ", lPCodePos );
   }
   else
   {
      fprintf( cargo->yyc, "\t" );
   }

   fprintf( cargo->yyc, "HB_P_BASELINE, %i, %i,", pFunc->pCode[ lPCodePos + 1 ], pFunc->pCode[ lPCodePos + 2 ] );

   if( cargo->bVerbose )
   {
      hb_comp_iBaseLine = HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) );
      fprintf( cargo->yyc, "\t/* %i */", hb_comp_iBaseLine );
   }

   fprintf( cargo->yyc, "\n" );

   return 3;
}

static HB_GENC_FUNC( hb_p_withobject )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_WITHOBJECT,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_sendwith )
{
   fprintf( cargo->yyc, "\tHB_P_SENDWITH, %i, %i,\n",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ] );
   return 3;
}

static HB_GENC_FUNC( hb_p_sendwithshort )
{
   fprintf( cargo->yyc, "\tHB_P_SENDWITHSHORT, %i,\n", pFunc->pCode[ lPCodePos + 1 ] );
   return 2;
}

static HB_GENC_FUNC( hb_p_endwithobject )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_ENDWITHOBJECT,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_foreach )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_FOREACH,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_enumerate )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_ENUMERATE,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_endenumerate )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_ENDENUMERATE,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_enumindex )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_ENUMINDEX,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_pushglobal )
{
   fprintf( cargo->yyc, "\tHB_P_PUSHGLOBAL, %i,",
            pFunc->pCode[ lPCodePos + 1 ] );

   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %s */", hb_compVariableFind( hb_comp_pGlobals, (USHORT) pFunc->pCode[ lPCodePos + 1 ] + 1 )->szName );
   }

   fprintf( cargo->yyc, "\n" );

   return 2;
}

static HB_GENC_FUNC( hb_p_popglobal )
{
   fprintf( cargo->yyc, "\tHB_P_POPGLOBAL, %i,",
            pFunc->pCode[ lPCodePos + 1 ] );

   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %s */", hb_compVariableFind( hb_comp_pGlobals, (USHORT) pFunc->pCode[ lPCodePos + 1 ] + 1 )->szName );
   }

   fprintf( cargo->yyc, "\n" );

   return 2;
}

 static HB_GENC_FUNC( hb_p_pushglobalref )
 {
    fprintf( cargo->yyc, "\tHB_P_PUSHGLOBALREF, %i,",
             pFunc->pCode[ lPCodePos + 1 ] );

    if( cargo->bVerbose )
    {
       fprintf( cargo->yyc, "\t/* %s */", hb_compVariableFind( hb_comp_pGlobals, (USHORT) pFunc->pCode[ lPCodePos + 1 ] + 1 )->szName );
    }

    fprintf( cargo->yyc, "\n" );

    return 2;
 }

static HB_GENC_FUNC( hb_p_switchcase )
{
   fprintf( cargo->yyc, "\tHB_P_SWITCHCASE, %i, %i, %i, %i, ",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ],
            pFunc->pCode[ lPCodePos + 3 ],
            pFunc->pCode[ lPCodePos + 4 ] );

   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %li */", HB_PCODE_MKLONG( &( pFunc->pCode[ lPCodePos + 1 ] ) ) );
   }

   fprintf( cargo->yyc, "\n" );

   return 5;
}

static HB_GENC_FUNC( hb_p_like )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_LIKE,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_match )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_MATCH,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_pushmacroref )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_PUSHMACROREF,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_ivarref )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_IVARREF,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_classsetmodule )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_CLASSSETMODULE,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_bitand )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_BITAND,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_bitor )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_BITOR,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_bitxor )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_BITXOR,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_bitshiftr )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_BITSHIFTR,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_bitshiftl )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_BITSHIFTL,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_largeframe )
{
   fprintf( cargo->yyc, "\tHB_P_LARGEFRAME, %i, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ],
            pFunc->pCode[ lPCodePos + 3 ] );

   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* (lo)locals, (hi)locals, params */" );
   }

   fprintf( cargo->yyc, "\n" );
   return 4;
}

static HB_GENC_FUNC( hb_p_pushwith )
{
   HB_SYMBOL_UNUSED( pFunc );
   HB_SYMBOL_UNUSED( lPCodePos );

   fprintf( cargo->yyc, "\tHB_P_PUSHWITH,\n" );
   return 1;
}

static HB_GENC_FUNC( hb_p_pushstrhidden )
{
   ULONG ulStart = lPCodePos;
   USHORT wLen = HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 1 ] ) );
   BYTE bType = pFunc->pCode[ lPCodePos + 3 ];
   USHORT wLenBuffer = HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 4 ] ) );

   fprintf( cargo->yyc, "\tHB_P_PUSHSTRHIDDEN, %i, %i, %i, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],     // LO: String length
            pFunc->pCode[ lPCodePos + 2 ],     // HI: String length
            pFunc->pCode[ lPCodePos + 3 ],     // Hide type
            pFunc->pCode[ lPCodePos + 4 ],     // LO: Buffer length
            pFunc->pCode[ lPCodePos + 5 ] );   // HI: Buffer length

   if( cargo->bVerbose )
   {
      fprintf( cargo->yyc, "\t/* %i, %i, %i */", wLen, bType, wLenBuffer );
   }

   lPCodePos += 6;
   if( wLen > 0 )
   {
      fprintf( cargo->yyc, "\n\t" );
      while( wLenBuffer-- )
      {
         BYTE uchr = ( BYTE ) pFunc->pCode[ lPCodePos++ ];
         /*
          * NOTE: After optimization some CHR(n) can be converted
          *    into a string containing nonprintable characters.
          *
          * TODO: add switch to use hexadecimal format "%#04x"
          */
         if( ( uchr < ( BYTE ) ' ' ) || ( uchr >= 127 ) )
         {
            fprintf( cargo->yyc, "%i, ", uchr );
         }
         else if( strchr( "\'\\\"", uchr ) )
         {
            fprintf( cargo->yyc, "%i, ", uchr );
         }
         else
         {
            fprintf( cargo->yyc, "\'%c\', ", uchr );
         }
      }
   }
   fprintf( cargo->yyc, "\n" );

   return (USHORT) (lPCodePos - ulStart);
}

static HB_GENC_FUNC( hb_p_localnearsetstrhidden )
{
   ULONG ulStart = lPCodePos;
   USHORT uLen   = HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 2 ] ) );
   BYTE bType = pFunc->pCode[ lPCodePos + 4 ];
   USHORT wLenBuffer = HB_PCODE_MKUSHORT( &( pFunc->pCode[ lPCodePos + 5 ] ) );

   fprintf( cargo->yyc, "\tHB_P_LOCALNEARSETSTRHIDDEN, %i, %i, %i, %i, %i, %i,",
            pFunc->pCode[ lPCodePos + 1 ],
            pFunc->pCode[ lPCodePos + 2 ],     // LO: String length
            pFunc->pCode[ lPCodePos + 3 ],     // HI: String length
            pFunc->pCode[ lPCodePos + 4 ],     // Hide type
            pFunc->pCode[ lPCodePos + 5 ],     // LO: Buffer length
            pFunc->pCode[ lPCodePos + 6 ] );   // HI: Buffer length

   if( cargo->bVerbose )
   {
      int iVar = (int) (signed char) pFunc->pCode[ lPCodePos + 1 ];

      if( cargo->iNestedCodeblock )
      {
         /* we are accesing variables within a codeblock */
         /* the names of codeblock variable are lost     */
         if( iVar < 0 )
         {
            fprintf( cargo->yyc, "\t/* localvar%i, %i, %i */", -iVar, bType, wLenBuffer );
         }
         else
         {
            fprintf( cargo->yyc, "\t/* codeblockvar%i, %i, %i */", iVar, bType, wLenBuffer );
         }
      }
      else
      {
         fprintf( cargo->yyc, "\t/* %s %i, %i, %i*/", hb_compLocalVariableFind( pFunc, iVar )->szName, uLen, bType, wLenBuffer );
      }
   }

   lPCodePos += 7;

   if( uLen > 0 )
   {
      fprintf( cargo->yyc, "\n\t" );

      while( wLenBuffer-- )
      {
         BYTE uchr = ( BYTE ) pFunc->pCode[ lPCodePos++ ];
         /*
          * NOTE: After optimization some CHR(n) can be converted
          *    into a string containing nonprintable characters.
          *
          * TODO: add switch to use hexadecimal format "%#04x"
          */
         if( ( uchr < ( BYTE ) ' ' ) || ( uchr >= 127 ) )
         {
            fprintf( cargo->yyc, "%i, ", uchr );
         }
         else if( strchr( "\'\\\"", uchr ) )
         {
            fprintf( cargo->yyc, "%i, ", uchr );
         }
         else
         {
            fprintf( cargo->yyc, "\'%c\', ", uchr );
         }
      }
   }

   fprintf( cargo->yyc, "\n" );

   return ( USHORT ) ( lPCodePos - ulStart );
}


/* NOTE: The order of functions has to match the order of opcodes
 *       mnemonics
 */
static HB_GENC_FUNC_PTR s_verbose_table[] = {
   hb_p_and,
   hb_p_arraypush,
   hb_p_arraypop,
   hb_p_arraydim,
   hb_p_arraygen,
   hb_p_equal,
   hb_p_endblock,
   hb_p_endproc,
   hb_p_exactlyequal,
   hb_p_false,
   hb_p_fortest,
   hb_p_function,
   hb_p_functionshort,
   hb_p_frame,
   hb_p_funcptr,
   hb_p_greater,
   hb_p_greaterequal,
   hb_p_dec,
   hb_p_divide,
   hb_p_do,
   hb_p_doshort,
   hb_p_duplicate,
   hb_p_dupltwo,
   hb_p_inc,
   hb_p_instring,
   hb_p_jumpnear,
   hb_p_jump,
   hb_p_jumpfar,
   hb_p_jumpfalsenear,
   hb_p_jumpfalse,
   hb_p_jumpfalsefar,
   hb_p_jumptruenear,
   hb_p_jumptrue,
   hb_p_jumptruefar,
   hb_p_lessequal,
   hb_p_less,
   hb_p_line,
   hb_p_localname,
   hb_p_macropop,
   hb_p_macropopaliased,
   hb_p_macropush,
   hb_p_macropusharg,
   hb_p_macropushlist,
   hb_p_macropushindex,
   hb_p_macropushpare,
   hb_p_macropushaliased,
   hb_p_macrosymbol,
   hb_p_macrotext,
   hb_p_message,
   hb_p_minus,
   hb_p_modulus,
   hb_p_modulename,
   /* start: pcodes generated by macro compiler */
   hb_p_dummy,
   hb_p_dummy,
   hb_p_dummy,
   hb_p_dummy,
   hb_p_dummy,
   hb_p_dummy,
   hb_p_dummy,
   hb_p_dummy,
   hb_p_dummy,
   hb_p_dummy,
   hb_p_dummy,
   hb_p_dummy,
   hb_p_dummy,
   /* end: */
   hb_p_mult,
   hb_p_negate,
   hb_p_noop,
   hb_p_not,
   hb_p_notequal,
   hb_p_or,
   hb_p_parameter,
   hb_p_plus,
   hb_p_pop,
   hb_p_popalias,
   hb_p_popaliasedfield,
   hb_p_popaliasedfieldnear,
   hb_p_popaliasedvar,
   hb_p_popfield,
   hb_p_poplocal,
   hb_p_poplocalnear,
   hb_p_popmemvar,
   hb_p_popstatic,
   hb_p_popvariable,
   hb_p_power,
   hb_p_pushalias,
   hb_p_pushaliasedfield,
   hb_p_pushaliasedfieldnear,
   hb_p_pushaliasedvar,
   hb_p_pushblock,
   hb_p_pushblockshort,
   hb_p_pushfield,
   hb_p_pushbyte,
   hb_p_pushint,
   hb_p_pushlocal,
   hb_p_pushlocalnear,
   hb_p_pushlocalref,
   hb_p_pushlong,
   hb_p_pushmemvar,
   hb_p_pushmemvarref,
   hb_p_pushnil,
   hb_p_pushdouble,
   hb_p_pushself,
   hb_p_pushstatic,
   hb_p_pushstaticref,
   hb_p_pushstr,
   hb_p_pushstrshort,
   hb_p_pushsym,
   hb_p_pushsymnear,
   hb_p_pushvariable,
   hb_p_retvalue,
   hb_p_send,
   hb_p_sendshort,
   hb_p_seqbegin,
   hb_p_seqend,
   hb_p_seqrecover,
   hb_p_sframe,
   hb_p_statics,
   hb_p_staticname,
   hb_p_swapalias,
   hb_p_true,
   hb_p_zero,
   hb_p_one,
   hb_p_macrolist,
   hb_p_macrolistend,
   hb_p_localnearaddint,
   hb_p_localnearsetint,
   hb_p_localnearsetstr,
   hb_p_addint,
   hb_p_left,
   hb_p_right,
   hb_p_substr,
   hb_p_dummy,
   hb_p_baseline,
   hb_p_lineoffset,
   hb_p_withobject,
   hb_p_sendwith,
   hb_p_sendwithshort,
   hb_p_endwithobject,
   hb_p_foreach,
   hb_p_enumerate,
   hb_p_endenumerate,
   hb_p_pushglobal,
   hb_p_popglobal,
   hb_p_pushglobalref,
   hb_p_enumindex,
   hb_p_switchcase,
   hb_p_like,
   hb_p_match,
   hb_p_pushmacroref,
   hb_p_ivarref,
   hb_p_classsetmodule,
   hb_p_bitand,
   hb_p_bitor,
   hb_p_bitxor,
   hb_p_bitshiftr,
   hb_p_bitshiftl,
   hb_p_largeframe,
   hb_p_pushwith,
   hb_p_pushlonglong,
   hb_p_pushstrhidden,
   hb_p_localnearsetstrhidden
};

static void hb_compGenCReadable( PFUNCTION pFunc, FILE * yyc )
{
   HB_GENC_INFO genc_info;

   /* Make sure that table is correct */
   assert( HB_P_LAST_PCODE == sizeof( s_verbose_table ) / sizeof( HB_GENC_FUNC_PTR ) );

   genc_info.iNestedCodeblock = 0;
   genc_info.bVerbose = ( hb_comp_iGenCOutput == HB_COMPGENC_VERBOSE );
   genc_info.yyc = yyc;

   if( ! hb_comp_bQuiet && hb_comp_iGenVarList )
   {
      printf( "Generating pcode list for '%s'...\n", pFunc->szName );
   }

   hb_compPCodeEval( pFunc, ( HB_PCODE_FUNC_PTR * ) s_verbose_table, ( void * ) &genc_info, hb_comp_iGenVarList );

   if( genc_info.bVerbose )
   {
      fprintf( yyc, "/* %05li */\n", pFunc->lPCodePos );
   }
}

static void hb_compGenCCompact( PFUNCTION pFunc, FILE * yyc )
{
   ULONG lPCodePos = 0;
   int nChar;

   fprintf( yyc, "\t" );

   nChar = 0;

   while( lPCodePos < pFunc->lPCodePos )
   {
      ++nChar;

      if( nChar > 1 )
      {
         fprintf( yyc, ", " );
      }

      if( nChar == 15 )
      {
         fprintf( yyc, "\n\t" );
         nChar = 1;
      }

      /* Displaying as decimal is more compact than hex */
      fprintf( yyc, "%d", ( int ) pFunc->pCode[ lPCodePos++ ] );

   }

   if( nChar != 0)
   {
      fprintf( yyc, "\n" );
   }
}
