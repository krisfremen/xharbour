/*
 * $Id: dynsym.c,v 1.7 2003/03/25 02:36:12 ronpinkas Exp $
 */

/*
 * Harbour Project source code:
 * Dynamic symbol table management
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

#include "hbapi.h"

#define SYM_ALLOCATED ( ( HB_SYMBOLSCOPE ) -1 )

typedef struct
{
   PHB_DYNS pDynSym;             /* Pointer to dynamic symbol */
} DYNHB_ITEM, * PDYNHB_ITEM, * DYNHB_ITEM_PTR;

static PDYNHB_ITEM s_pDynItems = NULL;    /* Pointer to dynamic items */
static USHORT      s_uiDynSymbols = 0;    /* Number of symbols present */

/* Closest symbol for match. hb_dynsymFind() will search for the name. */
/* If it cannot find the name, it positions itself to the */
/* closest symbol.  */
static USHORT      s_uiClosestDynSym = 0; /* TOFIX: This solution is not thread safe. */

void HB_EXPORT hb_dynsymLog( void )
{
   USHORT uiPos;

   HB_TRACE(HB_TR_DEBUG, ("hb_dynsymLog()"));

   for( uiPos = 0; uiPos < s_uiDynSymbols; uiPos++ )   /* For all dynamic symbols */
   {
      printf( "%i %s\n", uiPos + 1, s_pDynItems[ uiPos ].pDynSym->pSymbol->szName );
   }
}

PHB_SYMB HB_EXPORT hb_symbolNew( char * szName )      /* Create a new symbol */
{
   PHB_SYMB pSymbol;

   HB_TRACE(HB_TR_DEBUG, ("hb_symbolNew(%s)", szName));

   pSymbol = ( PHB_SYMB ) hb_xgrab( sizeof( HB_SYMB ) );
   pSymbol->szName = ( char * ) hb_xgrab( strlen( szName ) + 1 );
   pSymbol->cScope = SYM_ALLOCATED; /* to know what symbols to release when exiting the app */
   strcpy( pSymbol->szName, szName );
   pSymbol->pFunPtr = NULL;
   pSymbol->pDynSym = NULL;

   return pSymbol;
}

PHB_DYNS HB_EXPORT hb_dynsymNew( PHB_SYMB pSymbol, PSYMBOLS pModuleSymbols )    /* creates a new dynamic symbol */
{
   PHB_DYNS pDynSym;

   HB_TRACE(HB_TR_DEBUG, ("hb_dynsymNew(%p)", pSymbol));

   pDynSym = hb_dynsymFind( pSymbol->szName ); /* Find position */

   if( pDynSym )            /* If name exists */
   {
      if( pSymbol->cScope & HB_FS_PUBLIC ) /* only for HB_FS_PUBLIC */
      {
         if( ( ! pDynSym->pFunPtr ) && pSymbol->pFunPtr ) /* The DynSym existed */
         {
            pDynSym->pFunPtr = pSymbol->pFunPtr;  /* but had no function ptr assigned */
            pDynSym->pSymbol = pSymbol;
            pDynSym->ulCalls = 0; /* profiler support */
            pDynSym->ulTime  = 0; /* profiler support */
            pDynSym->ulRecurse = 0;
         }
      }

      if( pSymbol->pDynSym == (PHB_DYNS) 1 )
      {
         pDynSym->pModuleSymbols = pModuleSymbols;
         //printf( "Symbol: '%s' Module: '%s'\n", pSymbol->szName, pModuleSymbols->szModuleName );
      }

      pSymbol->pDynSym = pDynSym;    /* place a pointer to DynSym */

      return pDynSym;                /* Return pointer to DynSym */
   }

   if( s_uiDynSymbols == 0 )   /* Do we have any symbols ? */
   {
      pDynSym = s_pDynItems[ 0 ].pDynSym;     /* Point to first symbol */
                            /* *<1>* Remember we already got this one */
   }
   else
   {                        /* We want more symbols ! */
      s_pDynItems = ( PDYNHB_ITEM ) hb_xrealloc( s_pDynItems, ( s_uiDynSymbols + 1 ) * sizeof( DYNHB_ITEM ) );

      if( s_uiClosestDynSym <= s_uiDynSymbols )   /* Closest < current !! */
      {                                     /* Here it goes :-) */
         USHORT uiPos;

         for( uiPos = 0; uiPos < ( s_uiDynSymbols - s_uiClosestDynSym ); uiPos++ )
         {
             /* Insert element in array */
            memcpy( &s_pDynItems[ s_uiDynSymbols - uiPos ], &s_pDynItems[ s_uiDynSymbols - uiPos - 1 ], sizeof( DYNHB_ITEM ) );
         }
      }

      pDynSym = ( PHB_DYNS ) hb_xgrab( sizeof( HB_DYNS ) );
      s_pDynItems[ s_uiClosestDynSym ].pDynSym = pDynSym;    /* Enter DynSym */
   }

   s_uiDynSymbols++;                   /* Got one more symbol */

   pDynSym->pSymbol        = pSymbol;
   pDynSym->hMemvar        = 0;
   pDynSym->hArea          = 0;
   pDynSym->ulCalls        = 0; /* profiler support */
   pDynSym->ulTime         = 0; /* profiler support */
   pDynSym->ulRecurse      = 0;
   pDynSym->pModuleSymbols = NULL;

   if( pSymbol->cScope & HB_FS_PUBLIC ) /* only for HB_FS_PUBLIC */
   {
      if( pDynSym->pFunPtr != pSymbol->pFunPtr ) /* it contains a function pointer */
      {
         pDynSym->pFunPtr = pSymbol->pFunPtr;    /* place the function at DynSym */
      }
   }

   if( pSymbol->pDynSym == (PHB_DYNS) 1 )
   {
      pDynSym->pModuleSymbols = pModuleSymbols;
      //printf( "Symbol: '%s' Module: '%s'\n", pSymbol->szName, pModuleSymbols->szModuleName );
   }

   pSymbol->pDynSym = pDynSym;                /* place a pointer to DynSym */

   return pDynSym;
}

PHB_DYNS HB_EXPORT hb_dynsymGet( char * szName )  /* finds and creates a symbol if not found */
{
   PHB_DYNS pDynSym;
   char szUprName[ HB_SYMBOL_NAME_LEN + 1 ];

   HB_TRACE(HB_TR_DEBUG, ("hb_dynsymGet(%s)", szName));

   /* make a copy as we may get a const string, then turn it to uppercase */
   /* NOTE: This block is optimized for speed [vszakats] */
   {
      int iLen = strlen( szName );
      char * pDest = szUprName;

      if( iLen > HB_SYMBOL_NAME_LEN )
         iLen = HB_SYMBOL_NAME_LEN;

      pDest[ iLen ] = '\0';
      while( iLen-- )
      {
         char cChar = *szName++;

         if( cChar >= 'a' && cChar <= 'z' )
         {
            *pDest++ = cChar - ( 'a' - 'A' );
         }
         else if( cChar == ' ' || cChar == '\t' )
         {
            *pDest = '\0';
            break;
         }
         else
         {
            *pDest++ = cChar;
         }
      }
   }

   pDynSym = hb_dynsymFind( szUprName );

   if( ! pDynSym )       /* Does it exists ? */
   {
      if( (* HB_VM_STACK.pBase)->item.asSymbol.value->pDynSym )
      {
         pDynSym = hb_dynsymNew( hb_symbolNew( szUprName ), (* HB_VM_STACK.pBase)->item.asSymbol.value->pDynSym->pModuleSymbols );   /* Make new symbol */
      }
      else
      {
         pDynSym = hb_dynsymNew( hb_symbolNew( szUprName ), NULL );   /* Make new symbol */
      }
   }

   return pDynSym;
}

PHB_DYNS HB_EXPORT hb_dynsymGetCase( char * szName )  /* finds and creates a symbol if not found CASE SENSITIVE! */
{
   PHB_DYNS pDynSym;

   HB_TRACE(HB_TR_DEBUG, ("hb_dynsymGetCase(%s)", szName));

   //TraceLog( NULL, "Searching: %s\n", szName );

   pDynSym = hb_dynsymFind( szName );

   if( ! pDynSym )       /* Does it exists ? */
   {
      //TraceLog( NULL, "Creating: %s\n", szName );
      if( (* HB_VM_STACK.pBase)->item.asSymbol.value->pDynSym )
      {
         pDynSym = hb_dynsymNew( hb_symbolNew( szName ), (* HB_VM_STACK.pBase)->item.asSymbol.value->pDynSym->pModuleSymbols );   /* Make new symbol */
      }
      else
      {
         pDynSym = hb_dynsymNew( hb_symbolNew( szName ), NULL );   /* Make new symbol */
      }
   }

   //TraceLog( NULL, "Returning: %p\n", pDynSym );

   return pDynSym;
}

PHB_DYNS HB_EXPORT hb_dynsymFindName( char * szName )  /* finds a symbol */
{
   char szUprName[ HB_SYMBOL_NAME_LEN + 1 ];

   HB_TRACE(HB_TR_DEBUG, ("hb_dynsymFindName(%s)", szName));

   /* make a copy as we may get a const string, then turn it to uppercase */
   /* NOTE: This block is optimized for speed [vszakats] */
   {
      int iLen = strlen( szName );
      char * pDest = szUprName;

      if( iLen > HB_SYMBOL_NAME_LEN )
      {
         iLen = HB_SYMBOL_NAME_LEN;
      }

      pDest[ iLen ] = '\0';
      while( iLen-- )
      {
         char cChar = *szName++;

         if( cChar >= 'a' && cChar <= 'z' )
         {
            *pDest++ = cChar - ( 'a' - 'A' );
         }
         else if( cChar == ' ' || cChar == '\t' )
         {
            *pDest = '\0';
            break;
         }
         else
         {
            *pDest++ = cChar;
         }
      }
   }

   return hb_dynsymFind( szUprName );
}

PHB_DYNS HB_EXPORT hb_dynsymFind( char * szName )
{
   HB_TRACE(HB_TR_DEBUG, ("hb_dynsymFind(%s)", szName));

   if( s_pDynItems == NULL )
   {
      s_pDynItems = ( PDYNHB_ITEM ) hb_xgrab( sizeof( DYNHB_ITEM ) );     /* Grab array */
      s_pDynItems->pDynSym = ( PHB_DYNS ) hb_xgrab( sizeof( HB_DYNS ) );
                /* Always grab a first symbol. Never an empty bucket. *<1>* */
      s_pDynItems->pDynSym->hMemvar = 0;
      s_pDynItems->pDynSym->pSymbol = NULL;
      s_pDynItems->pDynSym->pFunPtr = NULL;

      return NULL;
   }
   else
   {
      /* Classic Tree Insert Sort Mechanism
       *
       * Insert Sort means the new item is entered alphabetically into
       * the array. In this case s_pDynItems !
       *
       * 1) We start in the middle of the array.
       * 2a) If the symbols are equal -> we have found the symbol !!
       *     Champagne ! We're done.
       *  b) If the symbol we are looking for ('ge') is greater than the
       *     middle ('po'), we start looking left.
       *     Only the first part of the array is going to be searched.
       *     Go to (1)
       *  c) If the symbol we are looking for ('ge') is smaller than the
       *     middle ('ko'), we start looking right
       *     Only the last part of the array is going to be searched.
       *     Go to (1)
       */

      USHORT uiFirst = 0;
      USHORT uiLast = s_uiDynSymbols;
      USHORT uiMiddle = uiLast / 2;

      s_uiClosestDynSym = uiMiddle;                  /* Start in the middle      */

      while( uiFirst < uiLast )
      {
         int iCmp = strcmp( s_pDynItems[ uiMiddle ].pDynSym->pSymbol->szName, szName );

         if( iCmp == 0 )
         {
            s_uiClosestDynSym = uiMiddle;
            return s_pDynItems[ uiMiddle ].pDynSym;
         }
         else if( iCmp < 0 )
         {
            uiLast = uiMiddle;
            s_uiClosestDynSym = uiMiddle;
         }
         else /* if( iCmp > 0 ) */
         {
            uiFirst = uiMiddle + 1;
            s_uiClosestDynSym = uiFirst;
         }

         uiMiddle = uiFirst + ( ( uiLast - uiFirst ) / 2 );
      }
   }

   #ifdef HB_SYMLIMIT_10_WORKAROUND

   /*
   *  (c) 2002, Marcelo Lombardo <lombardo@uol.com.br>
   *  This is an emulation workaround for the symbol table limited to 10 chars,
   *  since the build flag -DHB_SYMBOL_NAME_LEN=10 is not an option anymore.
   */

   if (s_uiClosestDynSym < s_uiDynSymbols)
   {
      USHORT iLen1 = strlen( szName );
      USHORT iLen2 = strlen( s_pDynItems[ s_uiClosestDynSym ].pDynSym->pSymbol->szName );
      BOOL   bOk  = 1;
      USHORT uiPos;

      /*
      *  Let's check the closer symbol found. This code compares each char in the smallest symbol
      *  name to the largest symbol name, if both are larger than 10.
      */

      if (iLen1 >= 10 && iLen2 >= 10 && (!(iLen1 == iLen2 && iLen1 == 10)))
      {
         if (iLen1 > iLen2)
         {
            for( uiPos = 0; uiPos < iLen2; uiPos++ )
            {
               if (szName[uiPos] != s_pDynItems[ s_uiClosestDynSym ].pDynSym->pSymbol->szName[uiPos])
               {
                  bOk = 0;
                  break;
               }
            }
         }
         else if (iLen2 > iLen1)
         {
            for( uiPos = 0; uiPos < iLen1; uiPos++ )
            {
               if (szName[uiPos] != s_pDynItems[ s_uiClosestDynSym ].pDynSym->pSymbol->szName[uiPos])
               {
                  bOk = 0;
                  break;
               }
            }
         }
         else if (iLen1 == iLen2)
         {
            bOk = 0;
         }

         if (bOk)
         {
            return s_pDynItems[ s_uiClosestDynSym ].pDynSym;
         }
      }

      /*
      *  We did not find the symbol, but "nCount" looks closer to the tree search
      *  than "nCountDial", when searching for "nCountDialog". So our best chance
      *  is to cut off szName up to 10 chars and redo the search.
      */

      if (iLen1 > 10 && iLen2 < 10)
      {
         USHORT uiFirst = 0;
         USHORT uiLast = s_uiDynSymbols;
         USHORT uiMiddle = uiLast / 2;
         char szNameLimited[ 10 + 1 ];
         char * pDest = szNameLimited;

         iLen1 = 10;

         pDest[ iLen1 ] = '\0';

         while( iLen1-- )
            *pDest++ = *szName++;

         s_uiClosestDynSym = uiMiddle;                  /* Start in the middle      */

         while( uiFirst < uiLast )
         {
            int iCmp = strcmp( s_pDynItems[ uiMiddle ].pDynSym->pSymbol->szName, szNameLimited );

            if( iCmp == 0 )
            {
               s_uiClosestDynSym = uiMiddle;
               return s_pDynItems[ uiMiddle ].pDynSym;
            }
            else if( iCmp < 0 )
            {
               uiLast = uiMiddle;
               s_uiClosestDynSym = uiMiddle;
            }
            else /* if( iCmp > 0 ) */
            {
               uiFirst = uiMiddle + 1;
               s_uiClosestDynSym = uiFirst;
            }
            uiMiddle = uiFirst + ( ( uiLast - uiFirst ) / 2 );
         }
      }

      /*
      *  In other hand, if szName has 10 chars and the Symbol table contains a similar
      *  entry, s_uiClosestDynSym may be wrong.
      *  For instance, if we search for "cAliasRela", but in the symbol table we have
      *  "cAliasRelac" and "cAliasNiv", the tree schema returns the wrong entry as the
      *  closest ("cAliasNiv"). The best solution in this case is to complete szName
      *  with some trailing chars ( "_" ), and redo the process.
      */

      if (iLen1 == 10 && iLen2 < 10)
      {
         USHORT uiFirst = 0;
         USHORT uiLast = s_uiDynSymbols;
         USHORT uiMiddle = uiLast / 2;
         USHORT iuCount;
         char szNameExtended[ 10 + 8 ];
         char * pDest = szNameExtended;

         pDest[ 17 ] = '\0';

         for (iuCount=0;iuCount<17;iuCount++)
         {
            if (iuCount<10)
               pDest[iuCount] = szName[iuCount];
            else
               pDest[iuCount] = '_';
         }

         s_uiClosestDynSym = uiMiddle;                  /* Start in the middle      */

         while( uiFirst < uiLast )
         {
            int iCmp = strcmp( s_pDynItems[ uiMiddle ].pDynSym->pSymbol->szName, szNameExtended );

            if( iCmp == 0 )
            {
               s_uiClosestDynSym = uiMiddle;
               return s_pDynItems[ uiMiddle ].pDynSym;
            }
            else if( iCmp < 0 )
            {
               uiLast = uiMiddle;
               s_uiClosestDynSym = uiMiddle;
            }
            else /* if( iCmp > 0 ) */
            {
               uiFirst = uiMiddle + 1;
               s_uiClosestDynSym = uiFirst;
            }
            uiMiddle = uiFirst + ( ( uiLast - uiFirst ) / 2 );
         }

         iLen1 = strlen( szName );
         iLen2 = strlen( s_pDynItems[ s_uiClosestDynSym ].pDynSym->pSymbol->szName );
         bOk  = 1;

         if (iLen2 > 10)
         {
            for( uiPos = 0; uiPos < iLen1; uiPos++ )
            {
               if (szName[uiPos] != s_pDynItems[ s_uiClosestDynSym ].pDynSym->pSymbol->szName[uiPos])
               {
                  bOk = 0;
                  break;
               }
            }

            if (bOk)
               return s_pDynItems[ s_uiClosestDynSym ].pDynSym;
         }
      }
   }

   #endif

   return NULL;
}

USHORT HB_EXPORT hb_dynsymEval( PHB_DYNS_FUNC pFunction, void * Cargo )
{
   BOOL bCont = TRUE;
   USHORT uiPos;

   HB_TRACE(HB_TR_DEBUG, ("hb_dynsymEval(%p, %p)", pFunction, Cargo));

   for( uiPos = 0; uiPos < s_uiDynSymbols && bCont; uiPos++ )
   {
      bCont = ( pFunction )( s_pDynItems[ uiPos ].pDynSym, Cargo );
   }

   return uiPos;
}

void HB_EXPORT hb_dynsymRelease( void )
{
   USHORT uiPos;

   HB_TRACE(HB_TR_DEBUG, ("hb_dynsymRelease()"));

   for( uiPos = 0; uiPos < s_uiDynSymbols; uiPos++ )
   {
      /* it is a allocated symbol ? */
      if( ( s_pDynItems + uiPos )->pDynSym->pSymbol->cScope == SYM_ALLOCATED )
      {
         hb_xfree( ( s_pDynItems + uiPos )->pDynSym->pSymbol->szName );
         hb_xfree( ( s_pDynItems + uiPos )->pDynSym->pSymbol );
      }

      hb_xfree( ( s_pDynItems + uiPos )->pDynSym );
   }

   hb_xfree( s_pDynItems );
}

PHB_DYNS HB_EXPORT hb_dynsymFindFromFunction( PHB_FUNC pFunc )
{
   USHORT uiPos;

   HB_TRACE(HB_TR_DEBUG, ("hb_vmFindSymbolFromFunction(%p)", pFunc ));

   for( uiPos = 0; uiPos < s_uiDynSymbols; uiPos++ )
   {
      if( s_pDynItems[ uiPos ].pDynSym->pFunPtr == pFunc )
      {
         return s_pDynItems[ uiPos ].pDynSym;
      }
   }

   return NULL;
}

// NOT TESTED YET!!!
PHB_DYNS HB_EXPORT hb_dynsymPos( USHORT uiPos )
{
   if( uiPos < s_uiDynSymbols )
   {
      return s_pDynItems[ uiPos ].pDynSym;
   }

   return NULL;
}

#ifdef HB_EXTENSION

HB_FUNC( __DYNSCOUNT ) /* How much symbols do we have: dsCount = __dynsymCount() */
{
   hb_retnl( ( long ) s_uiDynSymbols );
}

HB_FUNC( __DYNSGETNAME ) /* Get name of symbol: cSymbol = __dynsymGetName( dsIndex ) */
{
   long lIndex = hb_parnl( 1 ); /* NOTE: This will return zero if the parameter is not numeric */

   if( lIndex >= 1 && lIndex <= s_uiDynSymbols )
      hb_retc( s_pDynItems[ lIndex - 1 ].pDynSym->pSymbol->szName );
   else
      hb_retc( "" );
}

HB_FUNC( __DYNSGETINDEX ) /* Gimme index number of symbol: dsIndex = __dynsymGetIndex( cSymbol ) */
{
   PHB_DYNS pDynSym = hb_dynsymFindName( hb_parc( 1 ) );

   if( pDynSym )
   {
      hb_retnl( ( long ) ( s_uiClosestDynSym + 1 ) );
   }
   else
   {
      hb_retnl( 0L );
   }
}

HB_FUNC( __DYNSISFUN ) /* returns .t. if a symbol has a function/procedure pointer,
                          given its symbol index */
{
   long lIndex = hb_parnl( 1 ); /* NOTE: This will return zero if the parameter is not numeric */

   if( lIndex >= 1 && lIndex <= s_uiDynSymbols )
   {
      hb_retl( s_pDynItems[ lIndex - 1 ].pDynSym->pFunPtr != NULL );
   }
   else
   {
      hb_retl( FALSE );
   }
}

HB_FUNC( __DYNSGETPRF ) /* profiler: It returns an array with a function or procedure
                                     called and consumed times { nTimes, nTime }
                                     , given the dynamic symbol index */
{
   long lIndex = hb_parnl( 1 ); /* NOTE: This will return zero if the parameter is not numeric */

   hb_reta( 2 );
   hb_stornl( 0, -1, 1 );
   hb_stornl( 0, -1, 2 );

   if( lIndex >= 1 && lIndex <= s_uiDynSymbols )
   {
      if( s_pDynItems[ lIndex - 1 ].pDynSym->pFunPtr ) /* it is a function or procedure */
      {
         hb_stornl( s_pDynItems[ lIndex - 1 ].pDynSym->ulCalls, -1, 1 );
         hb_stornl( s_pDynItems[ lIndex - 1 ].pDynSym->ulTime,  -1, 2 );
      }
   }
}

#endif
