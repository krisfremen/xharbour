/*
 * $Id: wacore.c,v 1.12 2008/10/23 07:45:32 marchuet Exp $
 */

/*
 * Harbour Project source code:
 * Default RDD module
 *
 * Copyright 1999 Bruno Cantero <bruno@issnet.net>
 * Copyright 2007 Przemyslaw Czerpak <druzus / at / priv.onet.pl>
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

#ifdef __XHARBOUR__
/* JC1: optimizing stack access under MT */
#define HB_THREAD_OPTIMIZE_STACK
#endif

#include "hbapi.h"
#include "hbapirdd.h"
#include "hbapiitm.h"
#include "hbapierr.h"
#include "hbvm.h"
#include "hbstack.h"
#include "hbset.h"


#ifndef HB_THREAD_SUPPORT
   #define LOCK_AREA
   #define UNLOCK_AREA
   #define LOCK_AREA_INIT
   #define LOCK_AREA_DESTROY
#else
   #define LOCK_AREA          if (  pRddInfo->fMtLockInit ) HB_CRITICAL_LOCK( pRddInfo->mtxWorkArea );
   #define UNLOCK_AREA        if (  pRddInfo->fMtLockInit ) HB_CRITICAL_UNLOCK( pRddInfo->mtxWorkArea );
   #define LOCK_AREA_INIT     if ( !pRddInfo->fMtLockInit ) { HB_CRITICAL_INIT( pRddInfo->mtxWorkArea ); pRddInfo->fMtLockInit = TRUE; }
   #define LOCK_AREA_DESTROY  if (  pRddInfo->fMtLockInit ) { HB_CRITICAL_DESTROY( pRddInfo->mtxWorkArea ); pRddInfo->fMtLockInit = FALSE; }
#endif



#define HB_SET_WA( n )  do \
            { \
               pRddInfo->uiCurrArea = n; \
               pRddInfo->pCurrArea = ( ( pRddInfo->uiCurrArea < pRddInfo->uiWaNumMax ) ? \
                                 pRddInfo->waList[ pRddInfo->waNums[ pRddInfo->uiCurrArea ] ] : \
                                 NULL ); \
            } while( 0 )


static PHB_STACKRDD s_pRddInfo = NULL;
static int          s_iSetPrev;
static BOOL         s_bSetVoided = FALSE;

/*
 * Insert new WorkArea node at current WA position
 */
static void hb_waNodeInsert( PHB_STACKRDD pRddInfo, AREAP pArea )
{
   USHORT uiWaPos;

   if( pRddInfo->uiCurrArea >= pRddInfo->uiWaNumMax )
   {
      int iSize = ( ( ( int ) pRddInfo->uiCurrArea + 256 ) >> 8 ) << 8;

      if( iSize > HB_RDD_MAX_AREA_NUM )
         iSize = HB_RDD_MAX_AREA_NUM;

      if( pRddInfo->uiWaNumMax == 0 )
      {
         pRddInfo->waNums = (USHORT *) hb_xgrab( iSize * sizeof(USHORT) );
      }
      else
      {
         pRddInfo->waNums = (USHORT *) hb_xrealloc( pRddInfo->waNums, iSize * sizeof(USHORT) );
      }
      memset( &pRddInfo->waNums[ pRddInfo->uiWaNumMax ], 0, ( iSize - pRddInfo->uiWaNumMax ) * sizeof(USHORT) );
      pRddInfo->uiWaNumMax = iSize;
   }

   if( pRddInfo->uiWaSpace == 0 )
   {
      pRddInfo->uiWaSpace = 256;
      pRddInfo->waList = ( void ** ) hb_xgrab( pRddInfo->uiWaSpace * sizeof(void *) );
      memset( &pRddInfo->waList[ 0 ], 0, pRddInfo->uiWaSpace * sizeof(void *) );
      pRddInfo->waList[ 0 ] = NULL;
      uiWaPos = 1;
      pRddInfo->uiWaMax = 2;
   }
   else
   {
      uiWaPos = pRddInfo->uiWaMax++;
      if( pRddInfo->uiWaMax > pRddInfo->uiWaSpace )
      {
         pRddInfo->uiWaSpace = ( ( pRddInfo->uiWaMax + 256 ) >> 8 ) << 8;
         pRddInfo->waList = ( void ** ) hb_xrealloc( pRddInfo->waList, pRddInfo->uiWaSpace * sizeof(void *) );
         memset( &pRddInfo->waList[ pRddInfo->uiWaMax ], 0, ( pRddInfo->uiWaSpace - pRddInfo->uiWaMax ) * sizeof(void *) );
      }
      while( uiWaPos > 1 )
      {
         if( ( ( AREAP ) pRddInfo->waList[ uiWaPos - 1 ] )->uiArea < pRddInfo->uiCurrArea )
            break;
         pRddInfo->waList[ uiWaPos ] = pRddInfo->waList[ uiWaPos - 1 ];
         pRddInfo->waNums[ ( ( AREAP ) pRddInfo->waList[ uiWaPos ] )->uiArea ] = uiWaPos;
         uiWaPos--;
      }
   }
   pRddInfo->waNums[ pRddInfo->uiCurrArea ] = uiWaPos;
   pRddInfo->pCurrArea = pRddInfo->waList[ uiWaPos ] = pArea;
   pArea->uiArea = pRddInfo->uiCurrArea;
}

/*
 * Remove current WorkArea node
 */
static void hb_waNodeDelete( PHB_STACKRDD pRddInfo )
{
   USHORT uiWaPos;

   uiWaPos = pRddInfo->waNums[ pRddInfo->uiCurrArea ];
   pRddInfo->waNums[ pRddInfo->uiCurrArea ] = 0;
   pRddInfo->uiWaMax--;
   if( pRddInfo->uiWaMax <= 1 )
   {
      pRddInfo->uiWaSpace = pRddInfo->uiWaMax = pRddInfo->uiWaNumMax = 0;
      hb_xfree( pRddInfo->waList );
      hb_xfree( pRddInfo->waNums );
      pRddInfo->waList = NULL;
      pRddInfo->waNums = NULL;
   }
   else
   {
      while( uiWaPos < pRddInfo->uiWaMax )
      {
         pRddInfo->waList[ uiWaPos ] = pRddInfo->waList[ uiWaPos + 1 ];
         pRddInfo->waNums[ ( ( AREAP ) pRddInfo->waList[ uiWaPos ] )->uiArea ] = uiWaPos;
         uiWaPos++;
      }
      pRddInfo->waList[ pRddInfo->uiWaMax ] = NULL;
      if( pRddInfo->uiWaSpace - pRddInfo->uiWaMax >= 256 )
      {
         pRddInfo->uiWaSpace = ( ( pRddInfo->uiWaMax + 256 ) >> 8 ) << 8;
         pRddInfo->waList = ( void ** ) hb_xrealloc( pRddInfo->waList, pRddInfo->uiWaSpace * sizeof( void * ) );
      }
   }
   pRddInfo->pCurrArea = NULL;
}

/*
 * Return the next free WorkArea for later use.
 */
HB_EXPORT ERRCODE hb_rddSelectFirstAvailable( void )
{
   HB_THREAD_STUB
   PHB_STACKRDD pRddInfo = hb_stackRDD();
   USHORT uiArea;

   HB_TRACE(HB_TR_DEBUG, ("hb_rddSelectFirstAvailable()"));

   LOCK_AREA

   uiArea = 1;
   while( uiArea < pRddInfo->uiWaNumMax )
   {
      if( pRddInfo->waNums[ uiArea ] == 0 )
         break;
      uiArea++;
   }
   if( uiArea >= HB_RDD_MAX_AREA_NUM )
      return FAILURE;
   HB_SET_WA( uiArea );

   UNLOCK_AREA

   return SUCCESS;
}

/*
 * Creare and insert the new WorkArea node
 */
HB_EXPORT USHORT hb_rddInsertAreaNode( const char *szDriver )
{
   HB_THREAD_STUB
   PHB_STACKRDD pRddInfo = hb_stackRDD();
   LPRDDNODE pRddNode;
   USHORT uiRddID;
   AREAP pArea;

   HB_TRACE(HB_TR_DEBUG, ("hb_rddInsertAreaNode(%s)", szDriver));

   if( pRddInfo->uiCurrArea && pRddInfo->pCurrArea )
      return 0;

   pRddNode = hb_rddFindNode( szDriver, &uiRddID );
   if( !pRddNode )
      return 0;

   pArea = ( AREAP ) hb_rddNewAreaNode( pRddNode, uiRddID );
   if( !pArea )
      return 0;

   if ( pRddInfo->uiWaNumMax == 0  && hb_setGetWorkareasShared() )
   {
      LOCK_AREA_INIT
   }

   LOCK_AREA

   if( pRddInfo->uiCurrArea == 0 )
   {
      if( hb_rddSelectFirstAvailable() != SUCCESS )
         return 0;
   }

   hb_waNodeInsert( pRddInfo, pArea );

   UNLOCK_AREA

   return pRddInfo->uiCurrArea;
}

/*
 * Closes and releases the current WorkArea preparing it
 * to be used with a new database.
 */
HB_EXPORT void hb_rddReleaseCurrentArea( void )
{
   HB_THREAD_STUB
   PHB_STACKRDD pRddInfo = hb_stackRDD();
   AREAP pArea;

   HB_TRACE(HB_TR_DEBUG, ("hb_rddReleaseCurrentArea()"));

   pArea = ( AREAP ) pRddInfo->pCurrArea;
   if( !pArea )
      return;

   LOCK_AREA

   if( SELF_CLOSE( pArea ) == FAILURE )
   {
      UNLOCK_AREA

      return;
   }

   SELF_RELEASE( pArea );

   hb_waNodeDelete( pRddInfo );

   UNLOCK_AREA
}

HB_EXPORT PHB_STACKRDD hb_rddWaInit( void )
{
   PHB_STACKRDD pRddInfo;

   if( !hb_setGetWorkareasShared() || s_pRddInfo == NULL )
   {
      pRddInfo = (PHB_STACKRDD) hb_xgrab( sizeof( HB_STACKRDD ) );

      pRddInfo->szDefaultRDD  = NULL;
      pRddInfo->waList        = NULL;
      pRddInfo->waNums        = NULL;
      pRddInfo->pCurrArea     = NULL;
      pRddInfo->fNetError     = FALSE;
      pRddInfo->uiWaMax       = 0;
      pRddInfo->uiWaSpace     = 0;
      pRddInfo->uiWaNumMax    = 0;
      pRddInfo->uiCurrArea    = 1;
#ifdef HB_THREAD_SUPPORT
      pRddInfo->fMtLockInit   = FALSE;
      pRddInfo->ulCounter     = 1;
#endif
      
      if( s_pRddInfo == NULL )
      {
         s_pRddInfo = pRddInfo;
      }
   }
   else
   {
      pRddInfo = s_pRddInfo;
#ifdef HB_THREAD_SUPPORT
      HB_ATOMIC_INC( pRddInfo->ulCounter );
#endif
   }

   return pRddInfo;
}

HB_EXPORT BOOL hb_rddChangeSetWorkareasShared( BOOL bPrev, BOOL bSet )
{
   BOOL bOk = TRUE;
#ifdef HB_THREAD_SUPPORT
   if( bPrev == bSet )
   {
      return TRUE;
   }

   hb_threadWaitForIdle();
   if( !bSet )
   {
      // Hay que crear las estructuras HB_STACKRDD para cada thread y guardarlas en los respectivos stack
      // y hay que destruir el LOCK_AREA y poner fMtLockInit en FALSE

      HB_STACK *p = hb_ht_stack->next; // Comenzamos desde el segundo thread
      PHB_STACKRDD pRddInfo;

      while( p )
      {
         pRddInfo = (PHB_STACKRDD) hb_xgrab( sizeof( HB_STACKRDD ) );

         pRddInfo->szDefaultRDD  = NULL;
         pRddInfo->waList        = NULL;
         pRddInfo->waNums        = NULL;
         pRddInfo->pCurrArea     = NULL;
         pRddInfo->fNetError     = FALSE;
         pRddInfo->uiWaMax       = 0;
         pRddInfo->uiWaSpace     = 0;
         pRddInfo->uiWaNumMax    = 0;
         pRddInfo->uiCurrArea    = 1;
         pRddInfo->fMtLockInit   = FALSE;
         pRddInfo->ulCounter     = 1;

         p->rdd = pRddInfo;

         p = p->next;
      }

      pRddInfo = s_pRddInfo;
      pRddInfo->ulCounter = 1;
      if( pRddInfo->fMtLockInit )
      {
         LOCK_AREA_DESTROY
      }
   }
   else
   {
      // Hay que verificar que no haya areas abiertas en ninguno de los threads excepto el principal.
      // Hay que destruir las estructuras HB_STACKRDD de cada thread, excepto el del principal y
      // setear en cada stack para que use el puntero a HB_STACKRDD del principal.
      // En el thread principal, si hay areas abiertas, hay que crear el LOCK_AREA y poner el fMtLockInit en TRUE.

      HB_STACK *p = hb_ht_stack->next; // Comenzamos desde el segundo thread
      PHB_STACKRDD pRddInfo;

      while( p )
      {
         if( p->rdd->uiWaMax > 0 )
         {
            bOk = FALSE;
            break;
         }
         p = p->next;
      }

      if( bOk )
      {
         p = hb_ht_stack; // Comenzamos desde el primer thread
         pRddInfo = p->rdd;

         if( pRddInfo->uiWaMax > 0 )
         {
            LOCK_AREA_INIT
         }
         p = p->next;

         while( p )
         {
            hb_xfree( p->rdd );
            p->rdd = pRddInfo;
            p = p->next;
         }
      }
   }
   hb_threadIdleEnd();
#endif
   return bOk;
}

/* Destroy the workarea mutex */
HB_EXPORT void hb_rddWaShutDown( PHB_STACKRDD pRddInfo )
{
#ifdef HB_THREAD_SUPPORT
   if( !hb_setGetWorkareasShared() )
   {
      hb_rddCloseAll();
   }

   if( HB_ATOMIC_DEC( pRddInfo->ulCounter ) == 0 )
   {
      if( pRddInfo->fMtLockInit )
      {
         LOCK_AREA_DESTROY
      }
      hb_xfree( pRddInfo );
   }
#else
   hb_xfree( pRddInfo );
#endif
}

/*
 * Closes all WorkAreas.
 */
HB_EXPORT void hb_rddCloseAll( void )
{
   HB_THREAD_STUB
   PHB_STACKRDD pRddInfo = hb_stackRDD();

   HB_TRACE(HB_TR_DEBUG, ("hb_rddCloseAll()"));

   if( pRddInfo->uiWaMax > 0 )
   {
      BOOL isParents, isFinish = FALSE;
      AREAP pArea;
      USHORT uiIndex;

      LOCK_AREA

      do
      {
         isParents = FALSE;
         for( uiIndex = 1; uiIndex < pRddInfo->uiWaMax; uiIndex++ )
         {
            pArea = ( AREAP ) pRddInfo->waList[ uiIndex ];
            HB_SET_WA( pArea->uiArea );
            if( isFinish )
            {
               SELF_RELEASE( pArea );
               pRddInfo->waNums[ pRddInfo->uiCurrArea ] = 0;
               pRddInfo->pCurrArea = NULL;
            }
            else if( pArea->uiParents )
            {
               isParents = TRUE;
            }
            else
            {
               SELF_CLOSE( pArea );
            }
         }
         if( !isParents && !isFinish )
         {
            isParents = isFinish = TRUE;
         }
      }
      while( isParents );

      pRddInfo->uiWaSpace = pRddInfo->uiWaMax = pRddInfo->uiWaNumMax = 0;
      hb_xfree( pRddInfo->waList );
      hb_xfree( pRddInfo->waNums );
      pRddInfo->waList = NULL;
      pRddInfo->waNums = NULL;
      HB_SET_WA( 1 );

      UNLOCK_AREA
   }
}

HB_EXPORT void hb_rddFlushAll( void )
{
   HB_THREAD_STUB
   PHB_STACKRDD pRddInfo = hb_stackRDD();
   USHORT uiArea = hb_rddGetCurrentWorkAreaNumber(), uiIndex;

   LOCK_AREA

   for( uiIndex = 1; uiIndex < pRddInfo->uiWaMax; ++uiIndex )
   {
      hb_rddSelectWorkAreaNumber( ( ( AREAP ) pRddInfo->waList[ uiIndex ] )->uiArea );
      SELF_FLUSH( ( AREAP ) pRddInfo->pCurrArea );
   }

   UNLOCK_AREA

   hb_rddSelectWorkAreaNumber( uiArea );
}

HB_EXPORT void hb_rddUnLockAll( void )
{
   HB_THREAD_STUB
   PHB_STACKRDD pRddInfo = hb_stackRDD();
   USHORT uiArea = hb_rddGetCurrentWorkAreaNumber(), uiIndex;

   LOCK_AREA

   for( uiIndex = 1; uiIndex < pRddInfo->uiWaMax; ++uiIndex )
   {
      hb_rddSelectWorkAreaNumber( ( ( AREAP ) pRddInfo->waList[ uiIndex ] )->uiArea );
      SELF_UNLOCK( ( AREAP ) pRddInfo->pCurrArea, NULL );
   }

   UNLOCK_AREA

   hb_rddSelectWorkAreaNumber( uiArea );
}

/*
 * call a pCallBack function with all open workareas ###
 */
HB_EXPORT ERRCODE hb_rddIterateWorkAreas( WACALLBACK pCallBack, void * cargo )
{
   HB_THREAD_STUB
   PHB_STACKRDD pRddInfo = hb_stackRDD();
   ERRCODE errCode = SUCCESS;
   USHORT uiIndex;

   HB_TRACE(HB_TR_DEBUG, ("hb_rddIterateWorkAreas(%p,%p)", pCallBack, cargo));

   LOCK_AREA

   for( uiIndex = 1; uiIndex < pRddInfo->uiWaMax; uiIndex++ )
   {
      errCode = pCallBack( ( AREAP ) pRddInfo->waList[ uiIndex ], cargo );
      if( errCode != SUCCESS )
         break;
   }

   UNLOCK_AREA

   return errCode;
}

HB_EXPORT BOOL hb_rddGetNetErr( void )
{
   HB_THREAD_STUB
   return hb_stackRDD()->fNetError;
}

HB_EXPORT void hb_rddSetNetErr( BOOL fNetErr )
{
   HB_THREAD_STUB
   hb_stackRDD()->fNetError = fNetErr;
}

/*
 * Get (/set) default RDD driver
 */
HB_EXPORT const char * hb_rddDefaultDrv( const char * szDriver )
{
   HB_THREAD_STUB
   PHB_STACKRDD pRddInfo = hb_stackRDD();

   if( szDriver && *szDriver )
   {
      char szNewDriver[ HB_RDD_MAX_DRIVERNAME_LEN + 1 ];
      USHORT uiRddID;

      hb_strncpyUpper( szNewDriver, szDriver, sizeof( szNewDriver ) - 1 );
      if( !hb_rddFindNode( szNewDriver, &uiRddID ) )
         return NULL;

      pRddInfo->szDefaultRDD = hb_rddGetNode( uiRddID )->szName;
   }
   else if( !pRddInfo->szDefaultRDD && hb_rddGetNode( 0 ) )
   {
      const char *szDrvTable[] = { "DBFNTX", "DBFCDX", "DBFFPT", "DBF", NULL };
      int i;

      pRddInfo->szDefaultRDD = "";
      for( i = 0; szDrvTable[ i ]; ++i )
      {
         if( hb_rddFindNode( szDrvTable[ i ], NULL ) )
         {
            pRddInfo->szDefaultRDD = szDrvTable[ i ];
            break;
         }
      }
   }

   return pRddInfo->szDefaultRDD;
}

/*
 * Function for getting given workarea pointer
 */
HB_EXPORT void * hb_rddGetWorkAreaPointer( int iArea )
{
   HB_THREAD_STUB
   PHB_STACKRDD pRddInfo = hb_stackRDD();

   HB_TRACE(HB_TR_DEBUG, ("hb_rddGetWorkAreaPointer(%d)", iArea));

   if( iArea == 0 )
      return pRddInfo->pCurrArea;
   else if( iArea >= 1 && ( UINT ) iArea < ( UINT ) pRddInfo->uiWaNumMax )
      return pRddInfo->waList[ pRddInfo->waNums[ iArea ] ];
   else
      return NULL;
}

/*
 * Function for getting current workarea pointer
 */
HB_EXPORT void * hb_rddGetCurrentWorkAreaPointer( void )
{
   HB_THREAD_STUB
   HB_TRACE(HB_TR_DEBUG, ("hb_rddGetCurrentWorkAreaPointer()"));

   return hb_stackRDD()->pCurrArea;
}

/*
 * Return the current WorkArea number.
 */
HB_EXPORT int hb_rddGetCurrentWorkAreaNumber( void )
{
   HB_THREAD_STUB
   HB_TRACE(HB_TR_DEBUG, ("hb_rddGetCurrentWorkAreaNumber()"));

   return hb_stackRDD()->uiCurrArea;
}

/*
 * Select a WorkArea by the number.
 */
HB_EXPORT ERRCODE hb_rddSelectWorkAreaNumber( int iArea )
{
   HB_THREAD_STUB
   PHB_STACKRDD pRddInfo = hb_stackRDD();

   HB_TRACE(HB_TR_DEBUG, ("hb_rddSelectWorkAreaNumber(%d)", iArea));

   LOCK_AREA

   if( iArea < 1 || iArea > HB_RDD_MAX_AREA_NUM )
      HB_SET_WA( 0 );
   else
      HB_SET_WA( iArea );

   UNLOCK_AREA

   return ( pRddInfo->pCurrArea == NULL ) ? FAILURE : SUCCESS;
}
