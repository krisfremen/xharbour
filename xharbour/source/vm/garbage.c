/*
 * $Id: garbage.c,v 1.38 2002/12/31 07:15:44 jonnymind Exp $
 */

/*
 * Harbour Project source code:
 * The garbage collector for Harbour
 *
 * Copyright 1999 Ryszard Glab <rglab@imid.med.pl>
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
#include "hbfast.h"
#include "hbstack.h"
#include "hbapiitm.h"
#include "hbapierr.h"
#include "hbvm.h"
#include "error.ch"

#define HB_GC_COLLECTION_JUSTIFIED 64

/* status of memory block */
#define HB_GC_UNLOCKED     0
#define HB_GC_LOCKED       1  /* do not collect a memory block */
#define HB_GC_USED_FLAG    2  /* the bit for used/unused flag */
#define HB_GC_DELETE       4  /* item will be deleted during finalization */

/* pointer to memory block that will be checked in next step */
static HB_GARBAGE_PTR s_pCurrBlock = NULL;
/* memory blocks are stored in linked list with a loop */

/* pointer to locked memory blocks */
static HB_GARBAGE_PTR s_pLockedBlock = NULL;

//#define GC_RECYCLE

#ifdef GC_RECYCLE
   /* pointer to Cached Items memory blocks */
   static HB_GARBAGE_PTR s_pAvailableItems = NULL;

   /* pointer to Cached BaseArrays memory blocks */
   static HB_GARBAGE_PTR s_pAvailableBaseArrays = NULL;
#endif

/* marks if block releasing is requested during garbage collecting */
static BOOL s_bCollecting = FALSE;

/* Signify ReleaseAll Processing is taking place. */
static BOOL s_bReleaseAll = FALSE;

/* flag for used/unused blocks - the meaning of the HB_GC_USED_FLAG bit
 * is reversed on every collecting attempt
 */
static USHORT s_uUsedFlag = HB_GC_USED_FLAG;

static ULONG s_uAllocated = 0;

#ifdef GC_RECYCLE
   #define HB_GARBAGE_FREE( pAlloc )  ( pAlloc->pFunc == hb_gcGripRelease ? \
                                        ( hb_gcLink( &s_pAvailableItems, pAlloc ) ) \
                                        : \
                                        ( pAlloc->pFunc == hb_arrayReleaseGarbage ? \
                                          ( hb_gcLink( &s_pAvailableBaseArrays, pAlloc ) ) \
                                          : \
                                          ( hb_xfree( (void *) ( pAlloc ) ) ) \
                                        ) \
                                      )
#else
   #define HB_GARBAGE_NEW( ulSize )   ( HB_GARBAGE_PTR )hb_xgrab( ulSize )
   #define HB_GARBAGE_FREE( pAlloc )    hb_xfree( (void *)(pAlloc) )
#endif

#ifdef HB_THREAD_SUPPORT
   //HB_FORBID_MUTEX hb_gcCollectionForbid;
   static HB_CRITICAL_T s_CriticalMutex;
   HB_CRITICAL_T hb_gcCollectionMutex;
#endif

/* Forward declaration.*/
static HB_GARBAGE_FUNC( hb_gcGripRelease );

static void hb_gcLink( HB_GARBAGE_PTR *pList, HB_GARBAGE_PTR pAlloc )
{
   if( *pList )
   {
      /* add new block at the logical end of list */
      pAlloc->pNext = *pList;
      pAlloc->pPrev = (*pList)->pPrev;
      pAlloc->pPrev->pNext = pAlloc;
      (*pList)->pPrev = pAlloc;
   }
   else
   {
      *pList = pAlloc->pNext = pAlloc->pPrev = pAlloc;
   }
}

static void hb_gcUnlink( HB_GARBAGE_PTR *pList, HB_GARBAGE_PTR pAlloc )
{
   pAlloc->pPrev->pNext = pAlloc->pNext;
   pAlloc->pNext->pPrev = pAlloc->pPrev;

   if( *pList == pAlloc )
   {
      *pList = pAlloc->pNext;
   }

   if( ( pAlloc->pNext == pAlloc->pPrev ) && ( *pList == pAlloc ) )
   {
      *pList = NULL;    /* this was the last block */
   }
}

/* allocates a memory block */
void * hb_gcAlloc( ULONG ulSize, HB_GARBAGE_FUNC_PTR pCleanupFunc )
{
   HB_GARBAGE_PTR pAlloc;

   #ifdef HB_THREAD_SUPPORT
       HB_CRITICAL_LOCK( s_CriticalMutex );
   #endif

   #ifdef GC_RECYCLE
      if( s_pAvailableBaseArrays && ulSize == sizeof( HB_BASEARRAY ) )
      {
         pAlloc = s_pAvailableBaseArrays;
         hb_gcUnlink( &s_pAvailableBaseArrays, s_pAvailableBaseArrays );
      }
      else
      {
         pAlloc = ( HB_GARBAGE_PTR ) hb_xgrab( ulSize + sizeof( HB_GARBAGE ) );
      }
   #else
      pAlloc = HB_GARBAGE_NEW( ulSize + sizeof( HB_GARBAGE ) );
   #endif

   if( pAlloc )
   {
      s_uAllocated++;

      hb_gcLink( &s_pCurrBlock, pAlloc );

      pAlloc->pFunc  = pCleanupFunc;
      pAlloc->locked = 0;
      pAlloc->used   = s_uUsedFlag;

      HB_TRACE( HB_TR_DEBUG, ( "hb_gcAlloc %p in %p", pAlloc + 1, pAlloc ) );

      #ifdef HB_THREAD_SUPPORT
          HB_CRITICAL_UNLOCK( s_CriticalMutex );
      #endif

      return (void *)( pAlloc + 1 );   /* hide the internal data */
   }
   else
   {
      #ifdef HB_THREAD_SUPPORT
         HB_CRITICAL_UNLOCK( s_CriticalMutex );
      #endif

      return NULL;
   }
}

/* release a memory block allocated with hb_gcAlloc() */
void hb_gcFree( void *pBlock )
{
   HB_TRACE( HB_TR_DEBUG, ( "hb_gcFree(%p)", pBlock ) );

   if( s_bReleaseAll )
   {
      HB_TRACE( HB_TR_DEBUG, ( "Aborted - hb_gcFree(%p)", pBlock ) );

      return;
   }

   #ifdef HB_THREAD_SUPPORT
      HB_CRITICAL_LOCK( s_CriticalMutex );
   #endif

   if( pBlock )
   {
      HB_GARBAGE_PTR pAlloc = ( HB_GARBAGE_PTR ) pBlock;
      --pAlloc;

      if( pAlloc->locked )
      {
         HB_TRACE( HB_TR_DEBUG, ( "hb_gcFree(%p) *LOCKED* %p", pBlock, pAlloc ) );
         hb_gcUnlink( &s_pLockedBlock, pAlloc );
         HB_GARBAGE_FREE( pAlloc );
      }
      else
      {
         // Might already be marked for deletion.
         if( ! ( pAlloc->used & HB_GC_DELETE ) )
         {
            s_uAllocated--;

            hb_gcUnlink( &s_pCurrBlock, pAlloc );

            HB_GARBAGE_FREE( pAlloc );
         }
      }
   }
   else
   {
      hb_errInternal( HB_EI_XFREENULL, NULL, NULL, NULL );
   }

   #ifdef HB_THREAD_SUPPORT
      HB_CRITICAL_UNLOCK( s_CriticalMutex );
   #endif
}

static HB_GARBAGE_FUNC( hb_gcGripRelease )
{
   /* Item was already released in hb_gcGripDrop() - then we have nothing
    * to do here
    */
   HB_SYMBOL_UNUSED( Cargo );
}

HB_ITEM_PTR hb_gcGripGet( HB_ITEM_PTR pOrigin )
{
   HB_GARBAGE_PTR pAlloc;

   #if defined( HB_THREAD_SUPPORT )
      HB_CRITICAL_LOCK( s_CriticalMutex );
   #endif

   #ifdef GC_RECYCLE
      if( s_pAvailableItems )
      {
         pAlloc = s_pAvailableItems;
         hb_gcUnlink( &s_pAvailableItems, s_pAvailableItems );
      }
      else
      {
         pAlloc = ( HB_GARBAGE_PTR ) hb_xgrab( sizeof( HB_ITEM ) + sizeof( HB_GARBAGE ) );
      }
   #else
      pAlloc = HB_GARBAGE_NEW( sizeof( HB_ITEM ) + sizeof( HB_GARBAGE ) );
   #endif

   if( pAlloc )
   {
      HB_ITEM_PTR pItem = ( HB_ITEM_PTR )( pAlloc + 1 );

      hb_gcLink( &s_pLockedBlock, pAlloc );

      pAlloc->pFunc  = hb_gcGripRelease;
      pAlloc->locked = 1;
      pAlloc->used   = s_uUsedFlag;

      if( pOrigin )
      {
         pItem->type = HB_IT_NIL;
         hb_itemCopy( pItem, pOrigin );
      }
      else
      {
         memset( pItem, 0, sizeof( HB_ITEM ) );
         pItem->type = HB_IT_NIL;
      }

      #if defined( HB_THREAD_SUPPORT )
         HB_CRITICAL_UNLOCK( s_CriticalMutex );
      #endif

      return pItem;
   }
   else
   {
      #if defined( HB_THREAD_SUPPORT )
         HB_CRITICAL_UNLOCK( s_CriticalMutex );
      #endif

      return NULL;
   }
}

void hb_gcGripDrop( HB_ITEM_PTR pItem )
{

   HB_TRACE( HB_TR_DEBUG, ( "hb_gcGripDrop(%p)", pItem ) );

   if( s_bReleaseAll )
   {
      HB_TRACE( HB_TR_DEBUG, ( "Aborted - hb_gcGripDrop(%p)", pItem ) );

      return;
   }

   #if defined( HB_THREAD_SUPPORT )
      HB_CRITICAL_LOCK( s_CriticalMutex );
   #endif

   if( pItem )
   {
      HB_GARBAGE_PTR pAlloc = ( HB_GARBAGE_PTR ) pItem;
      --pAlloc;

      HB_TRACE( HB_TR_INFO, ( "Drop %p %p", pItem, pAlloc ) );

      if( pAlloc->pFunc == hb_gcGripRelease )
      {
         if( HB_IS_COMPLEX( pItem ) )
         {
            hb_itemClear( pItem );    /* clear value stored in this item */
         }
      }

      hb_gcUnlink( &s_pLockedBlock, pAlloc );

      HB_GARBAGE_FREE( pAlloc );
   }

   #if defined( HB_THREAD_SUPPORT )
      HB_CRITICAL_UNLOCK( s_CriticalMutex );
   #endif
}

/* Lock a memory pointer so it will not be released if stored
   outside of harbour variables
*/
void * hb_gcLock( void * pBlock )
{
   #ifdef HB_THREAD_SUPPORT
      HB_CRITICAL_LOCK( s_CriticalMutex );
   #endif

   if( pBlock )
   {
      HB_GARBAGE_PTR pAlloc = ( HB_GARBAGE_PTR ) pBlock;
      --pAlloc;

      if( ! pAlloc->locked )
      {
         //hb_gcUnlink( pContextList, pAlloc );
         hb_gcUnlink( &s_pCurrBlock, pAlloc );

         hb_gcLink( &s_pLockedBlock, pAlloc );

         pAlloc->used = s_uUsedFlag;
      }
      ++pAlloc->locked;
   }

   #ifdef HB_THREAD_SUPPORT
      HB_CRITICAL_UNLOCK( s_CriticalMutex );
   #endif

   return pBlock;
}

/* Unlock a memory pointer so it can be released if there is no
   references inside of harbour variables
*/
void *hb_gcUnlock( void *pBlock )
{
   #ifdef HB_THREAD_SUPPORT
      HB_CRITICAL_LOCK( s_CriticalMutex );
   #endif

   if( pBlock )
   {
      HB_GARBAGE_PTR pAlloc = ( HB_GARBAGE_PTR ) pBlock;
      --pAlloc;

      if( pAlloc->locked )
      {
         if( --pAlloc->locked == 0 )
         {
            hb_gcUnlink( &s_pLockedBlock, pAlloc );

            hb_gcLink( &s_pCurrBlock, pAlloc );

            pAlloc->used = s_uUsedFlag;
         }
      }
   }

   #ifdef HB_THREAD_SUPPORT
      HB_CRITICAL_UNLOCK( s_CriticalMutex );
   #endif

   return pBlock;
}

/* Mark a passed item as used so it will be not released by the GC
*/
void hb_gcItemRef( HB_ITEM_PTR pItem )
{

   if( HB_IS_BYREF( pItem ) )
   {
      pItem = hb_itemUnRef( pItem );
   }

   if( HB_IS_POINTER( pItem ) )
   {
      HB_GARBAGE_PTR pAlloc = ( HB_GARBAGE_PTR ) pItem->item.asPointer.value;
      --pAlloc;

      /* check if this memory was allocated by a hb_gcAlloc() */
      if ( pItem->item.asPointer.collect )
      {
         /* Check this memory only if it was not checked yet */
         if( pAlloc->used == s_uUsedFlag )
         {
            /* mark this memory as used so it will be no re-checked from
             * other references
             */
            pAlloc->used ^= HB_GC_USED_FLAG;
         }
      }
   }
   else if( HB_IS_ARRAY( pItem ) )
   {
      HB_GARBAGE_PTR pAlloc = ( HB_GARBAGE_PTR ) pItem->item.asArray.value;

      //printf( "Array %p\n", pItem->item.asArray.value );

      --pAlloc;

      /* Check this array only if it was not checked yet */
      if( pAlloc->used == s_uUsedFlag )
      {
         ULONG ulSize = pItem->item.asArray.value->ulLen;
         /* mark this block as used so it will be no re-checked from
          * other references
          */
         pAlloc->used ^= HB_GC_USED_FLAG;

         /* mark also all array elements */
         if( pItem->item.asArray.value->pItems )
         {
            pItem = pItem->item.asArray.value->pItems;
            //printf( "Items %p\n", pItem );

            while( ulSize )
            {
               //printf( "Item %p\n", pItem );
               hb_gcItemRef( pItem++ );
               --ulSize;
            }
         }
      }
   }
   else if( HB_IS_BLOCK( pItem ) )
   {
      HB_GARBAGE_PTR pAlloc = ( HB_GARBAGE_PTR ) pItem->item.asBlock.value;
      --pAlloc;

      /* Check this block only if it was not checked yet */
      if( pAlloc->used == s_uUsedFlag )
      {
         HB_CODEBLOCK_PTR pCBlock = pItem->item.asBlock.value;
         USHORT ui = 1;

         pAlloc->used ^= HB_GC_USED_FLAG;  /* mark this codeblock as used */

         /* mark as used all detached variables in a codeblock */
         while( ui <= pCBlock->uiLocals )
         {
            hb_gcItemRef( &pCBlock->pLocals[ ui ] );
            ++ui;
         }
      }
   }
   /* all other data types don't need the GC */

}

void hb_gcCollect( void )
{
   /* TODO: decrease the amount of time spend collecting */
   hb_gcCollectAll();
}

/* Check all memory blocks if they can be released
*/
void hb_gcCollectAll( void )
{
   if( s_uAllocated < HB_GC_COLLECTION_JUSTIFIED )
   {
      return;
   }

   #ifdef HB_THREAD_SUPPORT

      HB_CRITICAL_LOCK( hb_gcCollectionMutex );

      /*
      HB_CRITICAL_LOCK( hb_gcCollectionForbid.Control );

      while( hb_gcCollectionForbid.lCount )
      {
         if( hb_gcCollectionForbid.lCount < 0 )
         {
            HB_CRITICAL_UNLOCK( hb_gcCollectionForbid.Control );
            printf( "Unexpected condition!\n" );
            exit(1);
         }

         HB_CRITICAL_UNLOCK( hb_gcCollectionForbid.Control );

         #if defined(HB_OS_WIN_32)
             Sleep( 0 );
         #elif defined(HB_OS_DARWIN)
             usleep( 1 );
         #else
             static struct timespec nanosecs = { 0, 1000 };
             nanosleep( &nanosecs, NULL );
         #endif

         HB_CRITICAL_LOCK( hb_gcCollectionForbid.Control );
      }

      HB_CRITICAL_UNLOCK( hb_gcCollectionForbid.Control );
      */
   #endif


   //printf(  "Collecting...\n" );

   HB_TRACE( HB_TR_INFO, ( "hb_gcCollectAll(), %p, %i", s_pCurrBlock, s_bCollecting ) );

   if( s_pCurrBlock && ! s_bCollecting )
   {
      HB_GARBAGE_PTR pAlloc, pDelete;

      s_bCollecting = TRUE;
      s_uAllocated = 0;

      /* Step 1 - mark */
      /* All blocks are already marked because we are flipping
       * the used/unused flag
       */

      HB_TRACE( HB_TR_INFO, ( "Sweep Scan" ) );

      //printf( "Sweep Scan\n" );

      /* Step 2 - sweep */
      /* check all known places for blocks they are referring */
      #ifdef HB_THREAD_SUPPORT
         hb_threadIsLocalRef();
      #else
         hb_vmIsLocalRef();
      #endif

      //printf( "After LocalRef\n" );

      hb_vmIsStaticRef();
      //printf( "After StaticRef\n" );

      hb_vmIsGlobalRef();
      //printf( "After Globals\n" );

      hb_memvarsIsMemvarRef();
      //printf( "After MemvarRef\n" );

      hb_clsIsClassRef();
      //printf( "After ClassRef\n" );

      HB_TRACE( HB_TR_INFO, ( "Locked Scan" ) );

      /* check list of locked blocks for blocks referenced from
       * locked block
      */
      if( s_pLockedBlock )
      {
         pAlloc = s_pLockedBlock;

         do
         {
            /* it is not very elegant method but it works well */
            if( pAlloc->pFunc == hb_gcGripRelease )
            {
               hb_gcItemRef( ( HB_ITEM_PTR ) ( pAlloc + 1 ) );
            }
            else if( pAlloc->pFunc == hb_arrayReleaseGarbage )
            {
               HB_ITEM FakedItem;

               (&FakedItem)->type = HB_IT_ARRAY;
               (&FakedItem)->item.asArray.value = ( PHB_BASEARRAY )( pAlloc + 1 );

               hb_gcItemRef( &FakedItem );
            }
            else if( pAlloc->pFunc == hb_codeblockDeleteGarbage )
            {
               HB_ITEM FakedItem;

               (&FakedItem)->type = HB_IT_BLOCK;
               (&FakedItem)->item.asBlock.value = ( PHB_CODEBLOCK )( pAlloc + 1 );

               hb_gcItemRef( &FakedItem );
            }

            pAlloc = pAlloc->pNext;

         }
         while ( s_pLockedBlock != pAlloc );
      }

      HB_TRACE( HB_TR_INFO, ( "Cleanup Scan" ) );

      /* Step 3 - Call Cleanup Functions */
      pAlloc = s_pCurrBlock;
      do
      {
         if( s_pCurrBlock->used == s_uUsedFlag )
         {
           /* Mark for deletion. */
           s_pCurrBlock->used |= HB_GC_DELETE;

           //printf( "Marked, %p Item: %p\n", s_pCurrBlock, s_pCurrBlock + 1 );

           /* call the cleanup function. */
           if( s_pCurrBlock->pFunc )
           {
              HB_TRACE( HB_TR_INFO, ( "Cleanup, %p", s_pCurrBlock ) );
              ( s_pCurrBlock->pFunc )( ( void *)( s_pCurrBlock + 1 ) );
              HB_TRACE( HB_TR_INFO, ( "DONE Cleanup, %p", s_pCurrBlock ) );
           }
         }

         s_pCurrBlock = s_pCurrBlock->pNext;

      }
      while ( s_pCurrBlock && ( s_pCurrBlock != pAlloc ) );

      HB_TRACE( HB_TR_INFO, ( "Release Scan" ) );

      /* Step 4 - Release all blocks that are still marked as unused */
      pAlloc = s_pCurrBlock;
      do
      {
         NewTopBlock:

         if( s_pCurrBlock->used & HB_GC_DELETE )
         {
            HB_TRACE( HB_TR_INFO, ( "Delete, %p", s_pCurrBlock ) );

            pDelete = s_pCurrBlock;
            hb_gcUnlink( &s_pCurrBlock, s_pCurrBlock );

            /*
               Releasing the top block in the list, so we must mark the new top into pAlloc
               but we still need to process this new top. Without this goto, the while
               condition will immediatly fail. Using extra flags, and new conditions
               will adversly effect performance.
            */
            if( pDelete == pAlloc )
            {
               HB_TRACE( HB_TR_INFO, ( "New Top, %p", pDelete ) );

               pAlloc = s_pCurrBlock;
               HB_GARBAGE_FREE( pDelete );

               if( s_pCurrBlock )
               {
                  goto NewTopBlock;
               }
            }
            else
            {
               HB_TRACE( HB_TR_INFO, ( "Free, %p", pDelete ) );
               HB_GARBAGE_FREE( pDelete );
               HB_TRACE( HB_TR_INFO, ( "DONE Free, %p", pDelete ) );
            }
         }
         else
         {
            s_pCurrBlock = s_pCurrBlock->pNext;
         }

      }
      while ( s_pCurrBlock && ( pAlloc != s_pCurrBlock ) );

      s_bCollecting = FALSE;

      s_pCurrBlock = pAlloc;

      /* Step 4 - flip flag */
      /* Reverse used/unused flag so we don't have to mark all blocks
       * during next collecting
       */
      s_uUsedFlag ^= HB_GC_USED_FLAG;
   }

   #ifdef HB_THREAD_SUPPORT
      HB_CRITICAL_UNLOCK( hb_gcCollectionMutex );
   #endif

   //printf( "Done Collecting...\n" );
}

void hb_gcReleaseAll( void )
{
   HB_GARBAGE_PTR pAlloc, pDelete;

   HB_TRACE( HB_TR_INFO, ( "hb_gcReleaseAll()" ) );

   s_bReleaseAll = TRUE;
   s_bCollecting = TRUE;

   if( s_pLockedBlock )
   {
      pAlloc = s_pLockedBlock;
      do
      {
         /* call the cleanup function */
         if( s_pLockedBlock->pFunc )
         {
            HB_TRACE( HB_TR_INFO, ( "Cleanup for Locked, %p", s_pLockedBlock ) );
            ( s_pLockedBlock->pFunc )( ( void *)( s_pLockedBlock + 1 ) );
         }

         s_pLockedBlock = s_pLockedBlock->pNext;

      } while ( s_pLockedBlock && ( s_pLockedBlock != pAlloc ) );

      do
      {
         HB_TRACE( HB_TR_INFO, ( "Release Locked %p", s_pLockedBlock ) );
         pDelete = s_pLockedBlock;
         hb_gcUnlink( &s_pLockedBlock, s_pLockedBlock );
         //HB_GARBAGE_FREE( pDelete );
         hb_xfree( (void *) ( pDelete ) );
      }
      while ( s_pLockedBlock );
   }

   if( s_pCurrBlock )
   {
      pAlloc = s_pCurrBlock;
      do
      {
         /* call the cleanup function */
         if( s_pCurrBlock->pFunc )
         {
            HB_TRACE( HB_TR_INFO, ( "Cleanup, %p", s_pCurrBlock ) );
            ( s_pCurrBlock->pFunc )( ( void *)( s_pCurrBlock + 1 ) );
            HB_TRACE( HB_TR_INFO, ( "DONE Cleanup, %p", s_pCurrBlock ) );
         }

         s_pCurrBlock = s_pCurrBlock->pNext;

      } while( s_pCurrBlock && ( s_pCurrBlock != pAlloc ) );

      do
      {
         HB_TRACE( HB_TR_INFO, ( "Release %p", s_pCurrBlock ) );
         pDelete = s_pCurrBlock;
         hb_gcUnlink( &s_pCurrBlock, s_pCurrBlock );
         //HB_GARBAGE_FREE( pDelete );
         hb_xfree( (void *) ( pDelete ) );
      }
      while( s_pCurrBlock );
   }

   #ifdef GC_RECYCLE
      while( s_pAvailableItems )
      {
         HB_TRACE( HB_TR_INFO, ( "Release %p", s_pAvailableItems ) );
         pDelete = s_pAvailableItems;
         hb_gcUnlink( &s_pAvailableItems, s_pAvailableItems );
         //HB_GARBAGE_FREE( pDelete );
         hb_xfree( (void *) ( pDelete ) );
      }

      while( s_pAvailableBaseArrays )
      {
         HB_TRACE( HB_TR_INFO, ( "Release %p", s_pAvailableBaseArrays ) );
         pDelete = s_pAvailableBaseArrays;
         hb_gcUnlink( &s_pAvailableBaseArrays, s_pAvailableBaseArrays );
         //HB_GARBAGE_FREE( pDelete );
         hb_xfree( (void *) ( pDelete ) );
      }
   #endif

   s_bCollecting = FALSE;
   s_bReleaseAll = FALSE;

   HB_TRACE( HB_TR_INFO, ( "DONE Release All" ) );

}

void hb_gcInit( void )
{
   #ifdef HB_THREAD_SUPPORT
      //hb_threadForbidenInit( &hb_gcCollectionForbid );
      HB_CRITICAL_INIT( s_CriticalMutex );
      HB_CRITICAL_INIT( hb_gcCollectionMutex );
   #endif
}

void hb_gcExit( void )
{
   #ifdef HB_THREAD_SUPPORT
      //hb_threadForbidenDestroy( &hb_gcCollectionForbid );
      HB_CRITICAL_DESTROY( s_CriticalMutex );
      HB_CRITICAL_DESTROY( hb_gcCollectionMutex );
   #endif
}

/* service a single garbage collector step
 * Check a single memory block if it can be released
*/
HB_FUNC( HB_GCSTEP )
{
   hb_gcCollect();
}

/* Check all memory blocks if they can be released
*/
HB_FUNC( HB_GCALL )
{
   if( hb_parl( 1 ) )
   {
      s_uAllocated = HB_GC_COLLECTION_JUSTIFIED;
   }

   hb_gcCollectAll();
}
