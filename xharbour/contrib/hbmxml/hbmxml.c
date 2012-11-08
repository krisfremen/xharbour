/*
 * $Id$
 */

/*
 * Harbour Project source code:
 *    MINIXML functions wrapper
 *
 * Copyright 2010-2011 Petr Chornyj <myorg63@mail.ru>
 * www - http://harbour-project.org
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
 */

#include "hbapi.h"
#include "hbapiitm.h"
#include "hbapierr.h"
#include "hbapifs.h"
#include "hbapistr.h"
#include "hbstack.h"
#include "hbvm.h"
#include "hashapi.h"

#include "mxml.h"
#include "config.h"

#define BUFFER_SIZE     8192

#define MXML_ERR_ARGS   ( hb_errRT_BASE_SubstR( EG_ARG, 3012, NULL, HB_ERR_FUNCNAME, \
                                                HB_ERR_ARGS_BASEPARAMS ) )
/* ========================= cb's funcs support ============================= */

typedef struct
{
   PHB_ITEM type_cb;
   PHB_ITEM save_cb;
   PHB_ITEM sax_cb;
} HB_CBS_VAR;

typedef struct
{
   PHB_ITEM load_cb;
   PHB_ITEM save_cb;
} HB_CUSTOM_CBS_VAR;

typedef struct
{
   PHB_ITEM error_cb;
} HB_ERROR_CB_VAR;

static HB_CBS_VAR *        s_cbs_var         = NULL;
static HB_CUSTOM_CBS_VAR * s_custom_cbs_var  = NULL;
static HB_ERROR_CB_VAR *   s_error_cb_var    = NULL;

/* ======================= mxml_node_t * support ============================ */

static void mxmlRemoveRef( mxml_node_t * node )
{
   if( node->parent )
   {
      mxmlRemove( node );
      mxmlRelease( node );
   }
}

static int hbmxml_release( mxml_node_t * node )
{
   void * user_data;

   if( mxmlGetRefCount( node ) <= ( node->parent ? 2 : 1 ) )
   {
      /* remove userData  when last HVM item with given node is cleared
       * It's not nice solution because node can still exists and can
       * be extracted yet but there is not destructor for userData
       * in MXML library so we have no choice :-( [druzus]
       */
      if( ( user_data = mxmlGetUserData( node ) ) != NULL )
      {
         mxmlSetUserData( node, NULL );

         hb_itemRelease( ( PHB_ITEM ) user_data );
      }
   }

   return mxmlRelease( node );
}

/* ======================= mxml_node_t * support =========================== */
static HB_GARBAGE_FUNC( hbmxml_nodeDestructor )
{
   mxml_node_t ** pHbnode = ( mxml_node_t ** ) Cargo;

   if( pHbnode && *pHbnode )
   {
      hbmxml_release( *pHbnode );
      *pHbnode = NULL;
   }
}

static mxml_node_t * mxml_node_param( int iParam )
{
   mxml_node_t ** pHbnode = ( mxml_node_t ** ) hb_parptrGC( hbmxml_nodeDestructor,
                                                            iParam );

   return ( pHbnode && *pHbnode ) ? *pHbnode : NULL;
}

static mxml_node_t ** mxml_node_new( mxml_node_t * node, int iNew )
{
   mxml_node_t ** pHbnode = ( mxml_node_t ** ) hb_gcAlloc( sizeof( mxml_node_t * ),
                                                           hbmxml_nodeDestructor );

   *pHbnode = node;

   if( iNew == 0 )
      mxmlRetain( node );

   return pHbnode;
}

static void mxml_node_push( mxml_node_t * node, int iNew )
{
   hb_itemPutPtrGC( hb_stackAllocItem(), mxml_node_new( node, iNew ) );
}

static void mxml_node_ret( mxml_node_t * node, int iNew )
{
   if( node )
      hb_itemPutPtrGC( hb_stackReturnItem(), mxml_node_new( node, iNew ) );
}

/* ======================= mxml_index_t * support =========================== */

static HB_GARBAGE_FUNC( hbmxml_indexDestructor )
{
   mxml_index_t ** ppMxml_index = ( mxml_index_t ** ) Cargo;

   if( *ppMxml_index )
   {
      mxmlIndexDelete( *ppMxml_index );
      *ppMxml_index = NULL;
   }
}

static mxml_index_t * mxml_index_param( int iParam )
{
   mxml_index_t ** ppMxml_index = ( mxml_index_t ** ) hb_parptrGC( hbmxml_indexDestructor,
                                                                   iParam );

   return ( ppMxml_index && *ppMxml_index ) ? *ppMxml_index : NULL;
}

static mxml_index_t ** mxml_index_new( mxml_index_t * index )
{
   mxml_index_t ** ppMxml_index = ( mxml_index_t ** ) hb_gcAlloc( sizeof( mxml_index_t * ),
                                                                  hbmxml_indexDestructor );

   *ppMxml_index = index;

   return ppMxml_index;
}

static void mxml_index_ret( mxml_index_t * index )
{
   hb_itemPutPtrGC( hb_stackReturnItem(), mxml_index_new( index ) );
}

/* ======================= non-wrap HB_... funcs ============================ */

HB_FUNC( HB_MXMLVERSION )
{
   hb_retc( MXML_VERSION );
}

/* ====================== MXML_... wrapper funcs ============================ */

/*
 * - mxmlEntityAddCallback
 * - mxmlEntityRemoveCallback
 * - mxmlLoadFd
 * - mxmlSAXLoadFd
 * - mxmlSaveFd
 */

/* void mxmlAdd( mxml_node_t * parent,
                 int where, mxml_node_t * child, mxml_node_t * node ) */

HB_FUNC( MXMLADD )
{
   mxml_node_t *  parent   = mxml_node_param( 1 );
   int            where    = hb_parnidef( 2, MXML_ADD_BEFORE );
   mxml_node_t *  child    = mxml_node_param( 3 );
   mxml_node_t *  node     = mxml_node_param( 4 );

   if( parent && node )
   {
      where = ( where == MXML_ADD_BEFORE ) ? MXML_ADD_BEFORE : MXML_ADD_AFTER;

      mxmlAdd( parent, where, ( child != NULL ) ? child : MXML_ADD_TO_PARENT, node );
   }
   else
      MXML_ERR_ARGS;
}

/* void mxmlElementDeleteAttr( mxml_node_t * node, const char * name ) */

HB_FUNC( MXMLELEMENTDELETEATTR )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
   {
      void * hName;

      mxmlElementDeleteAttr( node, hb_parstr_utf8( 2, &hName, NULL ) );

      hb_strfree( hName );
   }
   else
      MXML_ERR_ARGS;
}

/* const char * mxmlElementGetAttr( mxml_node_t * node, const char * name ) */

HB_FUNC( MXMLELEMENTGETATTR )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
   {
      void * hName;

      hb_retstr_utf8( mxmlElementGetAttr( node, hb_parstr_utf8( 2, &hName, NULL ) ) );

      hb_strfree( hName );
   }
   else
      MXML_ERR_ARGS;
}

/* void mxmlElementSetAttr( mxml_node_t * node,
                            const char * name, const char * value ) */

HB_FUNC( MXMLELEMENTSETATTR )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
   {
      void *   hName;
      void *   hValue;

      mxmlElementSetAttr( node, hb_parstr_utf8( 2, &hName, NULL ), hb_parstr_utf8( 3, &hValue, NULL ) );

      hb_strfree( hName );
      hb_strfree( hValue );
   }
   else
      MXML_ERR_ARGS;
}

/* const char * mxmlEntityGetName( int val ) */

HB_FUNC( MXMLENTITYGETNAME )
{
   hb_retstr_utf8( mxmlEntityGetName( hb_parni( 1 ) ) );
}

/* int mxmlEntityGetValue( const char * name ) */

HB_FUNC( MXMLENTITYGETVALUE )
{
   void * hName;

   hb_retni( mxmlEntityGetValue( hb_parstr_utf8( 1, &hName, NULL ) ) );

   hb_strfree( hName );
}

/*
   mxml_node_t * mxmlFindElement( mxml_node_t * node, mxml_node_t * top,
                                  const char * name, const char * attr,
                                  const char * value, int descend );
 */

HB_FUNC( MXMLFINDELEMENT )
{
   void *         hName;
   void *         hAttr;
   void *         hValue;
   mxml_node_t *  node  = mxml_node_param( 1 );
   mxml_node_t *  top   = mxml_node_param( 2 );

   if( node && top )
   {
      mxml_node_t * nodef = mxmlFindElement( node, top,
                                             hb_parstr_utf8( 3, &hName, NULL ),
                                             hb_parstr_utf8( 4, &hAttr, NULL ),
                                             hb_parstr_utf8( 5, &hValue, NULL ),
                                             hb_parni( 6 ) );

      hb_strfree( hName );
      hb_strfree( hAttr );
      hb_strfree( hValue );

      mxml_node_ret( nodef, 0 );
   }
   else
      MXML_ERR_ARGS;
}

/* mxml_node_t * mxmlFindPath( mxml_node_t * node, const char * path ) */

HB_FUNC( MXMLFINDPATH )
{
   void *         hPath;
   mxml_node_t *  node = mxml_node_param( 1 );

   if( node )
   {
      mxml_node_t * nodef = mxmlFindPath( node, hb_parstr_utf8( 2, &hPath, NULL ) );

      mxml_node_ret( nodef, 0 );

      hb_strfree( hPath );
   }
   else
      MXML_ERR_ARGS;
}

/* char * mxmlGetCDATA( mxml_node_t * node ) */

HB_FUNC( MXMLGETCDATA )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
      hb_retstr_utf8( mxmlGetCDATA( node ) );
   else
      MXML_ERR_ARGS;
}

/* char * mxmlGetElement( mxml_node_t * node ) */

HB_FUNC( MXMLGETELEMENT )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
      hb_retstr_utf8( mxmlGetElement( node ) );
   else
      MXML_ERR_ARGS;
}

/* mxml_node_t * mxmlGetFirstChild( mxml_node_t * node ) */

HB_FUNC( MXMLGETFIRSTCHILD )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
      mxml_node_ret( mxmlGetFirstChild( node ), 0 );
   else
      MXML_ERR_ARGS;
}

/* int mxmlGetInteger( mxml_node_t * node ) */

HB_FUNC( MXMLGETINTEGER )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
      hb_retni( mxmlGetInteger( node ) );
   else
      MXML_ERR_ARGS;
}

/* mxml_node_t * mxmlGetLastChild( mxml_node_t * node ) */

HB_FUNC( MXMLGETLASTCHILD )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
      mxml_node_ret( mxmlGetLastChild( node ), 0 );
   else
      MXML_ERR_ARGS;
}

/* mxml_node_t * mxmlGetNextSibling( mxml_node_t * node ) */

HB_FUNC( MXMLGETNEXTSIBLING )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
      mxml_node_ret( mxmlGetNextSibling( node ), 0 );
   else
      MXML_ERR_ARGS;
}

/* char * mxmlGetOpaque( mxml_node_t * node ) */

HB_FUNC( MXMLGETOPAQUE )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
      hb_retstr_utf8( mxmlGetOpaque( node ) );
   else
      MXML_ERR_ARGS;
}

/* mxml_node_t * mxmlGetParent( mxml_node_t * node ) */

HB_FUNC( MXMLGETPARENT )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
      mxml_node_ret( mxmlGetParent( node ), 0 );
   else
      MXML_ERR_ARGS;
}

/* mxml_node_t * mxmlGetPrevSibling( mxml_node_t *node ) */

HB_FUNC( MXMLGETPREVSIBLING )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
      mxml_node_ret( mxmlGetPrevSibling( node ), 0 );
   else
      MXML_ERR_ARGS;
}

/* double mxmlGetReal( mxml_node_t * node ) */

HB_FUNC( MXMLGETREAL )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
      hb_retnd( mxmlGetReal( node ) );
   else
      MXML_ERR_ARGS;
}

/* int mxmlGetRefCount( mxml_node_t * node ) */

HB_FUNC( MXMLGETREFCOUNT )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
      hb_retni( mxmlGetRefCount( node ) );
   else
      MXML_ERR_ARGS;
}

/* char * mxmlGetText( mxml_node_t * node, int * whitespace ) */

HB_FUNC( MXMLGETTEXT )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
   {
      int whitespace = hb_parni( 2 );

      hb_retstr_utf8( mxmlGetText( node, &whitespace ) );
      hb_storni( whitespace, 2 );
   }
   else
      MXML_ERR_ARGS;
}

/* mxml_type_t mxmlGetType( mxml_node_t * node) */

HB_FUNC( MXMLGETTYPE )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
      hb_retni( ( int ) mxmlGetType( node ) );
   else
      MXML_ERR_ARGS;
}

/* void * mxmlGetUserData( mxml_node_t * node ) */

HB_FUNC( MXMLGETUSERDATA )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
   {
      PHB_ITEM pItem = ( PHB_ITEM ) mxmlGetUserData( node );

      if( pItem )
         hb_itemCopy( hb_stackReturnItem(), pItem );
   }
   else
      MXML_ERR_ARGS;
}

/* mxml_node_t * mxmlIndexEnum( mxml_index_t * ind ) */

HB_FUNC( MXMLINDEXENUM )
{
   mxml_index_t * index = mxml_index_param( 1 );

   if( index )
      mxml_node_ret( mxmlIndexEnum( index ), 0 );
   else
      MXML_ERR_ARGS;
}

/* mxml_node_t * mxmlIndexFind( mxml_index_t * ind,
                                const char * element, const char * value ) */

HB_FUNC( MXMLINDEXFIND )
{
   mxml_index_t * index = mxml_index_param( 1 );

   if( index )
   {
      void *         hElement;
      void *         hValue;
      mxml_node_t *  nodef = mxmlIndexFind( index,
                                            hb_parstr_utf8( 2, &hElement, NULL ),
                                            hb_parstr_utf8( 3, &hValue, NULL ) );

      hb_strfree( hElement );
      hb_strfree( hValue );

      mxml_node_ret( nodef, 0 );
   }
   else
      MXML_ERR_ARGS;
}

/* void mxmlIndexDelete( mxml_index_t * ind ) */

HB_FUNC( MXMLINDEXDELETE )
{

   mxml_index_t ** ppIndex = ( mxml_index_t ** ) hb_parptrGC( hbmxml_indexDestructor, 1 );

   if( ppIndex && *ppIndex )
   {
      mxmlIndexDelete( *ppIndex );
      *ppIndex = NULL;
   }
   else
      MXML_ERR_ARGS;
}

/* int mxmlIndexGetCount( mxml_index_t * ind ) */

HB_FUNC( MXMLINDEXGETCOUNT )
{
   mxml_index_t * index = mxml_index_param( 1 );

   if( index )
      hb_retni( mxmlIndexGetCount( index ) );
   else
      MXML_ERR_ARGS;
}

/* mxml_index_t * mxmlIndexNew( mxml_node_t * node,
                                const char * element, const char * attr ) */

HB_FUNC( MXMLINDEXNEW )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
   {
      void *         hElement;
      void *         hAttribute;
      mxml_index_t * index = mxmlIndexNew( node,
                                           hb_parstr_utf8( 2, &hElement, NULL ),
                                           hb_parstr_utf8( 3, &hAttribute, NULL ) );

      hb_strfree( hElement );
      hb_strfree( hAttribute );

      if( index )
         mxml_index_ret( index );
   }
   else
      MXML_ERR_ARGS;
}

/* mxml_node_t * mxmlIndexReset( mxml_index_t * ind ) */

HB_FUNC( MXMLINDEXRESET )
{
   mxml_index_t * index = mxml_index_param( 1 );

   if( index )
      mxml_node_ret( mxmlIndexReset( index ), 0 );
   else
      MXML_ERR_ARGS;
}

/* ============== mxml_type_t type_cb( mxml_node_t * node ) ================= */

static mxml_type_t type_cb( mxml_node_t * node )
{
   if( s_cbs_var != NULL )
   {
      if( s_cbs_var->type_cb && hb_vmRequestReenter() )
      {
         int iResult;

         hb_vmPushEvalSym();
         hb_vmPush( s_cbs_var->type_cb );
         mxml_node_push( node, 0 );
         hb_vmSend( 1 );

         iResult = hb_parnidef( -1, MXML_TEXT );

         hb_vmRequestRestore();
         return ( mxml_type_t ) iResult;
      }
   }
   return MXML_TEXT;
}

HB_FUNC( MXMLLOADFILE )
{
   void *         hFree;
   mxml_node_t *  node_top;
   mxml_node_t *  node;
   mxml_load_cb_t cb = MXML_NO_CALLBACK;
   FILE *         file;

   if( HB_ISNIL( 1 ) || ( HB_ISNUM( 1 ) && hb_parni( 1 ) == MXML_NO_PARENT ) )
      node_top = MXML_NO_PARENT;
   else
   {
      node_top = mxml_node_param( 1 );

      if( ! node_top )
      {
         MXML_ERR_ARGS;
         return;
      }
   }

   if( HB_ISBLOCK( 3 ) )
   {
      s_cbs_var->type_cb   = hb_param( 3, HB_IT_BLOCK );
      cb                   = type_cb;
   }
   else if( HB_ISNUM( 3 ) )
   {
      switch( hb_parni( 3 ) )
      {
         case 0:  cb = MXML_NO_CALLBACK;       break;
         case 1:  cb = MXML_INTEGER_CALLBACK;  break;
         case 2:  cb = MXML_OPAQUE_CALLBACK;   break;
         case 3:  cb = MXML_REAL_CALLBACK;     break;
         case 4:  cb = MXML_TEXT_CALLBACK;     break;
         case 5:  cb = MXML_IGNORE_CALLBACK;   break;
         default: cb = MXML_NO_CALLBACK;
      }
   }

   file = hb_fopen( hb_parstr_utf8( 2, &hFree, NULL ), "rb" );

   if( file )
   {
      node = mxmlLoadFile( node_top, file, cb );

      mxml_node_ret( node, ( node_top == MXML_NO_PARENT ) ? 1 : 0 );
      fclose( file );
   }

   s_cbs_var->type_cb = NULL;

   hb_strfree( hFree );
}

/*
   mxml_node_t * mxmlLoadString( mxml_node_t * top, const char * s,
                                 mxml_type_t (*cb)(mxml_node_t *) )
 */

HB_FUNC( MXMLLOADSTRING )
{
   void *         hFree;
   mxml_node_t *  node_top;
   mxml_node_t *  node;
   mxml_load_cb_t cb = MXML_NO_CALLBACK;

   if( HB_ISNIL( 1 ) || ( HB_ISNUM( 1 ) && hb_parni( 1 ) == MXML_NO_PARENT ) )
      node_top = MXML_NO_PARENT;
   else
   {
      node_top = mxml_node_param( 1 );

      if( ! node_top )
      {
         MXML_ERR_ARGS;
         return;
      }
   }

   if( HB_ISBLOCK( 3 ) )
   {
      s_cbs_var->type_cb   = hb_param( 3, HB_IT_BLOCK );
      cb                   = type_cb;
   }
   else if( HB_ISNUM( 3 ) )
   {
      switch( hb_parni( 3 ) )
      {
         case 0:  cb = MXML_NO_CALLBACK;       break;
         case 1:  cb = MXML_INTEGER_CALLBACK;  break;
         case 2:  cb = MXML_OPAQUE_CALLBACK;   break;
         case 3:  cb = MXML_REAL_CALLBACK;     break;
         case 4:  cb = MXML_TEXT_CALLBACK;     break;
         case 5:  cb = MXML_IGNORE_CALLBACK;   break;
         default: cb = MXML_NO_CALLBACK;
      }
   }

   node                 = mxmlLoadString( node_top, hb_parstr_utf8( 2, &hFree, NULL ), cb );

   s_cbs_var->type_cb   = NULL;
   mxml_node_ret( node, ( node_top == MXML_NO_PARENT ) ? 1 : 0 );

   hb_strfree( hFree );
}

/* mxml_node_t * mxmlNewCDATA( mxml_node_t * parent, const char * string ) */

HB_FUNC( MXMLNEWCDATA )
{
   void *         hString;
   mxml_node_t *  node;

   if( HB_ISNIL( 1 ) || ( HB_ISNUM( 1 ) && hb_parni( 1 ) == MXML_NO_PARENT ) )
   {
      node = mxmlNewCDATA( MXML_NO_PARENT, hb_parstr_utf8( 2, &hString, NULL ) );

      mxml_node_ret( node, 1 );

      hb_strfree( hString );
   }
   else
   {
      mxml_node_t * node_parent = mxml_node_param( 1 );

      if( node_parent )
      {
         node = mxmlNewCDATA( node_parent, hb_parstr_utf8( 2, &hString, NULL ) );

         mxml_node_ret( node, 0 );

         hb_strfree( hString );
      }
      else
         MXML_ERR_ARGS;
   }
}

/* mxml_node_t * mxmlNewElement( mxml_node_t * parent, const char * name ) */

HB_FUNC( MXMLNEWELEMENT )
{
   void *         hName;
   mxml_node_t *  node;

   if( HB_ISNIL( 1 ) || ( HB_ISNUM( 1 ) && hb_parni( 1 ) == MXML_NO_PARENT ) )
   {
      node = mxmlNewElement( MXML_NO_PARENT, hb_parstr_utf8( 2, &hName, NULL ) );

      mxml_node_ret( node, 1 );

      hb_strfree( hName );
   }
   else
   {
      mxml_node_t * node_parent = mxml_node_param( 1 );

      if( node_parent )
      {
         node = mxmlNewElement( node_parent, hb_parstr_utf8( 2, &hName, NULL ) );

         mxml_node_ret( node, 0 );

         hb_strfree( hName );
      }
      else
         MXML_ERR_ARGS;
   }
}

/* mxml_node_t * mxmlNewInteger( mxml_node_t * parent, int integer ) */

HB_FUNC( MXMLNEWINTEGER )
{
   if( HB_ISNIL( 1 ) || ( HB_ISNUM( 1 ) && hb_parni( 1 ) == MXML_NO_PARENT ) )
      mxml_node_ret( mxmlNewInteger( MXML_NO_PARENT, hb_parni( 2 ) ), 1 );
   else
   {
      mxml_node_t * node_parent = mxml_node_param( 1 );

      if( node_parent )
         mxml_node_ret( mxmlNewInteger( node_parent, hb_parni( 2 ) ), 0 );
      else
         MXML_ERR_ARGS;
   }
}

/* mxml_node_t * mxmlNewOpaque( mxml_node_t *parent, const char * opaque ) */

HB_FUNC( MXMLNEWOPAQUE )
{
   void *         hOpaque;
   mxml_node_t *  node;

   if( HB_ISNIL( 1 ) || ( HB_ISNUM( 1 ) && hb_parni( 1 ) == MXML_NO_PARENT ) )
   {
      node = mxmlNewOpaque( MXML_NO_PARENT, hb_parstr_utf8( 2, &hOpaque, NULL ) );

      mxml_node_ret( node, 1 );

      hb_strfree( hOpaque );
   }
   else
   {
      mxml_node_t * node_parent = mxml_node_param( 1 );

      if( node_parent )
      {
         node = mxmlNewOpaque( node_parent, hb_parstr_utf8( 2, &hOpaque, NULL ) );

         mxml_node_ret( node, 0 );

         hb_strfree( hOpaque );
      }
      else
         MXML_ERR_ARGS;
   }
}

/* mxml_node_t * mxmlNewReal( mxml_node_t * parent, double real ) */

HB_FUNC( MXMLNEWREAL )
{
   if( HB_ISNIL( 1 ) || ( HB_ISNUM( 1 ) && hb_parni( 1 ) == MXML_NO_PARENT ) )
      mxml_node_ret( mxmlNewReal( MXML_NO_PARENT, hb_parnd( 2 ) ), 1 );
   else
   {
      mxml_node_t * node_parent = mxml_node_param( 1 );

      if( node_parent )
         mxml_node_ret( mxmlNewReal( node_parent, hb_parnd( 2 ) ), 0 );
      else
         MXML_ERR_ARGS;
   }
}

/* mxml_node_t * mxmlNewText( mxml_node_t * parent,
                              int whitespace, const char * string ) */

HB_FUNC( MXMLNEWTEXT )
{
   void *         hString;
   mxml_node_t *  node;

   if( HB_ISNIL( 1 ) || ( HB_ISNUM( 1 ) && hb_parni( 1 ) == MXML_NO_PARENT ) )
   {
      node = mxmlNewText( MXML_NO_PARENT, hb_parnidef( 2, 0 ),
                          hb_parstr_utf8( 3, &hString, NULL ) );

      mxml_node_ret( node, 1 );

      hb_strfree( hString );
   }
   else
   {
      mxml_node_t * node_parent = mxml_node_param( 1 );

      if( node_parent )
      {
         node = mxmlNewText( node_parent, hb_parnidef( 2, 0 ),
                             hb_parstr_utf8( 3, &hString, NULL ) );

         mxml_node_ret( node, 0 );

         hb_strfree( hString );
      }
      else
         MXML_ERR_ARGS;
   }
}

/* mxml_node_t * mxmlNewXML( const char * version ) */

HB_FUNC( MXMLNEWXML )
{
   mxml_node_t * node;

   if( HB_ISCHAR( 1 ) )
   {
      void * hVersion;

      node = mxmlNewXML( hb_parstr_utf8( 1, &hVersion, NULL ) );

      hb_strfree( hVersion );
   }
   else
      node = mxmlNewXML( NULL );

   mxml_node_ret( node, 1 );
}

/* void mxmlRemove( mxml_node_t * node ) */

/* mxmlRemove() does not update reference counters, we use our own
 * wrapper which does it [druzus]
 */


HB_FUNC( MXMLREMOVE )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
      mxmlRemoveRef( node );
   else
      MXML_ERR_ARGS;
}

/* void mxmlDelete( mxml_node_t * node ) */
HB_FUNC( MXMLDELETE )
{
   mxml_node_t ** pHbnode = ( mxml_node_t ** ) hb_parptrGC( hbmxml_nodeDestructor, 1 );

   if( pHbnode && *pHbnode )
   {
      mxmlRemoveRef( *pHbnode );
      hbmxml_release( *pHbnode );
      *pHbnode = NULL;
   }
   else
      MXML_ERR_ARGS;
}

#if 0
/* int mxmlRelease( mxml_node_t * node ) */

HB_FUNC( MXMLRELEASE )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
      hb_retni( mxmlRelease( node ) );
   else
      MXML_ERR_ARGS;
}

/* int mxmlRetain( mxml_node_t * node ) */

HB_FUNC( MXMLRETAIN )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
      hb_retni( mxmlRetain( node ) );
   else
      MXML_ERR_ARGS;
}
#endif

/* ===== void (*mxml_sax_cb_t)(mxml_node_t *, mxml_sax_event_t, void *)====== */

static void sax_cb( mxml_node_t * node, mxml_sax_event_t event, void * data )
{
   if( node != NULL && s_cbs_var != NULL )
   {
      if( s_cbs_var->sax_cb && hb_vmRequestReenter() )
      {
         HB_USHORT uPCount = 2;

         hb_vmPushEvalSym();
         hb_vmPush( s_cbs_var->sax_cb );
         mxml_node_push( node, 0 );
         hb_vmPushInteger( ( int ) ( event + 1 ) );

         if( data != NULL )
         {
            hb_vmPush( ( PHB_ITEM ) data );
            uPCount++;
         }
         hb_vmSend( uPCount );

         hb_vmRequestRestore();
      }
   }
}

/*
   mxml_node_t * mxmlSAXLoadFile( mxml_node_t * top,
                                  FILE * fp,
                                  mxml_load_cb_t cb,
                                  mxml_sax_cb_t sax_cb,
                                  void * sax_data )
 */

HB_FUNC( MXMLSAXLOADFILE )
{
   void *         hFree;
   mxml_node_t *  node_top;
   mxml_node_t *  node;
   mxml_load_cb_t cb       = MXML_NO_CALLBACK;
   mxml_sax_cb_t  cb_sax   = MXML_NO_CALLBACK;
   PHB_ITEM       pData    = ( hb_pcount() > 4 ) ? hb_param( 5, HB_IT_ANY ) : NULL;
   FILE *         file;

   if( HB_ISNIL( 1 ) || ( HB_ISNUM( 1 ) && hb_parni( 1 ) == MXML_NO_PARENT ) )
   {
      node_top = MXML_NO_PARENT;
   }
   else
   {
      node_top = mxml_node_param( 1 );

      if( ! node_top )
      {
         MXML_ERR_ARGS;
         return;
      }
   }

   if( HB_ISBLOCK( 3 ) )
   {
      s_cbs_var->type_cb   = hb_param( 3, HB_IT_BLOCK );
      cb                   = type_cb;
   }
   else if( HB_ISNUM( 3 ) )
   {
      switch( hb_parni( 3 ) )
      {
         case 0:  cb = MXML_NO_CALLBACK;       break;
         case 1:  cb = MXML_INTEGER_CALLBACK;  break;
         case 2:  cb = MXML_OPAQUE_CALLBACK;   break;
         case 3:  cb = MXML_REAL_CALLBACK;     break;
         case 4:  cb = MXML_TEXT_CALLBACK;     break;
         case 5:  cb = MXML_IGNORE_CALLBACK;   break;
         default: cb = MXML_NO_CALLBACK;
      }
   }

   if( HB_ISBLOCK( 4 ) )
   {
      s_cbs_var->sax_cb = hb_param( 4, HB_IT_BLOCK );
      cb_sax            = sax_cb;
   }

   file = hb_fopen( hb_parstr_utf8( 2, &hFree, NULL ), "rb" );

   if( file )
   {
      node = mxmlSAXLoadFile( node_top, file, cb, cb_sax, pData );

      mxml_node_ret( node, ( node_top == MXML_NO_PARENT ) ? 1 : 0 );

      fclose( file );
   }

   s_cbs_var->type_cb   = NULL;
   s_cbs_var->sax_cb    = NULL;

   hb_strfree( hFree );
}

/*
   mxml_node_t * mxmlSAXLoadString( mxml_node_t * top,
                                    const char * s,
                                    mxml_load_cb_t cb,
                                    mxml_sax_cb_t sax_cb,
                                    void * sax_data )
 */

HB_FUNC( MXMLSAXLOADSTRING )
{
   void *         hFree;
   mxml_node_t *  node_top;
   mxml_node_t *  node;
   mxml_load_cb_t cb       = MXML_NO_CALLBACK;
   mxml_sax_cb_t  cb_sax   = MXML_NO_CALLBACK;
   PHB_ITEM       pData    = ( hb_pcount() > 4 ) ? hb_param( 5, HB_IT_ANY ) : NULL;
   const char *   s;

   if( HB_ISNIL( 1 ) || ( HB_ISNUM( 1 ) && hb_parni( 1 ) == MXML_NO_PARENT ) )
      node_top = MXML_NO_PARENT;
   else
   {
      node_top = mxml_node_param( 1 );

      if( ! node_top )
      {
         MXML_ERR_ARGS;
         return;
      }
   }

   if( HB_ISBLOCK( 3 ) )
   {
      s_cbs_var->type_cb   = hb_param( 3, HB_IT_BLOCK );
      cb                   = type_cb;
   }
   else if( HB_ISNUM( 3 ) )
   {
      switch( hb_parni( 3 ) )
      {
         case 0:  cb = MXML_NO_CALLBACK;       break;
         case 1:  cb = MXML_INTEGER_CALLBACK;  break;
         case 2:  cb = MXML_OPAQUE_CALLBACK;   break;
         case 3:  cb = MXML_REAL_CALLBACK;     break;
         case 4:  cb = MXML_TEXT_CALLBACK;     break;
         case 5:  cb = MXML_IGNORE_CALLBACK;   break;
         default: cb = MXML_NO_CALLBACK;
      }
   }

   if( HB_ISBLOCK( 4 ) )
   {
      s_cbs_var->sax_cb = hb_param( 4, HB_IT_BLOCK );
      cb_sax            = sax_cb;
   }

   s = hb_parstr_utf8( 2, &hFree, NULL );

   if( s )
   {
      node = mxmlSAXLoadString( node_top, s, cb, cb_sax, pData );

      mxml_node_ret( node, ( node_top == MXML_NO_PARENT ) ? 1 : 0 );
   }

   s_cbs_var->type_cb   = NULL;
   s_cbs_var->sax_cb    = NULL;

   hb_strfree( hFree );
}

/* ======== const char *(*mxml_save_cb_t)(mxml_node_t *, int) =========== */
static const char * save_cb( mxml_node_t * node, int where )
{
   if( node != NULL && s_cbs_var != NULL )
   {
      if( s_cbs_var->save_cb )
      {
         const char *   pszResult;
         void *         hText;

         hb_vmPushEvalSym();
         hb_vmPush( s_cbs_var->save_cb );
         mxml_node_push( node, 0 );
         hb_vmPushInteger( where );
         hb_vmSend( 2 );

         pszResult = hb_itemGetStrUTF8( hb_param( -1, HB_IT_ANY ), &hText, NULL );

         if( hText )
            hb_strfree( hText );

         return pszResult;
      }
   }
   return NULL;
}

/* char * mxmlSaveAllocString( mxml_node_t *node, mxml_save_cb_t cb ) */

HB_FUNC( MXMLSAVEALLOCSTRING )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
   {
      mxml_save_cb_t cb = MXML_NO_CALLBACK;
      char           buffer[ BUFFER_SIZE ];
      int            bytes;

      if( HB_ISBLOCK( 2 ) )
      {
         s_cbs_var->save_cb   = hb_param( 2, HB_IT_BLOCK );
         cb                   = save_cb;
      }

      bytes                = mxmlSaveString( node, buffer, BUFFER_SIZE, cb );
      s_cbs_var->save_cb   = NULL;

      if( bytes <= 0 )
         hb_retc_null();
      else if( bytes < ( int ) sizeof( buffer ) )
         hb_retclen( buffer, bytes );
      else
      {
         char * s = ( char * ) hb_xalloc( bytes + 1 );

         if( s == NULL )
            hb_retc_null();
         else
         {
            mxmlSaveString( node, s, bytes + 1, cb );
            hb_retclen_buffer( s, bytes );
         }
      }
   }
   else
      MXML_ERR_ARGS;
}

HB_FUNC( MXMLSAVEFILE )
{
   mxml_node_t *  node = mxml_node_param( 1 );
   FILE *         file;
   void *         hFree;

   if( node && HB_ISCHAR( 2 ) )
   {
      mxml_save_cb_t cb = MXML_NO_CALLBACK;

      if( HB_ISBLOCK( 3 ) )
      {
         s_cbs_var->save_cb   = hb_param( 3, HB_IT_BLOCK );
         cb                   = save_cb;
      }

      file = hb_fopen( hb_parstr_utf8( 2, &hFree, NULL ), "wb" );

      if( file )
      {
         hb_retni( mxmlSaveFile( node, file, cb ) );
         fclose( file );
      }

      s_cbs_var->save_cb = NULL;

      hb_strfree( hFree );
   }
   else
      MXML_ERR_ARGS;
}

/* int mxmlSaveString( mxml_node_t * node,
                       char * buffer,
                       int bufsize,
                       mxml_save_cb_t cb )
 */

HB_FUNC( MXMLSAVESTRING )
{
   mxml_node_t *  node     = mxml_node_param( 1 );
   PHB_ITEM       pBuffer  = hb_param( 2, HB_IT_STRING );

   if( node )
   {
      mxml_save_cb_t cb = MXML_NO_CALLBACK;

      if( pBuffer && HB_ISBYREF( 2 ) && hb_parcsiz( 2 ) > 0 )
      {
         char *   buffer;
         HB_SIZE  buffer_size;
         int      bytes;

         if( hb_itemGetWriteCL( pBuffer, &buffer, &buffer_size ) )
         {
            if( HB_ISBLOCK( 3 ) )
            {
               s_cbs_var->save_cb   = hb_param( 3, HB_IT_BLOCK );
               cb                   = save_cb;
            }

            bytes = mxmlSaveString( node, buffer, ( int ) ( buffer_size + 1 ), cb );

            if( bytes > 0 && ( HB_SIZE ) bytes <= buffer_size )
               hb_storclen( buffer, bytes, 2 );

            hb_retni( bytes );

            s_cbs_var->save_cb = NULL;

            return;
         }
      }
   }

   MXML_ERR_ARGS;
}

/* int mxmlSetCDATA( mxml_node_t * node, const char * data ) */

HB_FUNC( MXMLSETCDATA )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
   {
      void * hData;

      hb_retni( mxmlSetCDATA( node, hb_parstr_utf8( 2, &hData, NULL ) ) );

      hb_strfree( hData );
   }
   else
      MXML_ERR_ARGS;
}

/* int mxmlSetElement( mxml_node_t * node, const char * name ) */

HB_FUNC( MXMLSETELEMENT )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
   {
      void * hFree;

      hb_retni( mxmlSetElement( node, hb_parstr_utf8( 2, &hFree, NULL ) ) );

      hb_strfree( hFree );
   }
   else
      MXML_ERR_ARGS;
}

/* ============== void ( *mxml_error_cb_t )( const char * ) ================= */

static void error_cb( const char * pszErrorMsg )
{
   if( s_error_cb_var != NULL )
   {
      if( s_error_cb_var->error_cb && hb_vmRequestReenter() )
      {
         hb_vmPushEvalSym();
         hb_vmPush( s_error_cb_var->error_cb );
         hb_itemPutC( hb_stackAllocItem(), pszErrorMsg );

         hb_vmSend( 1 );
         hb_vmRequestRestore();
      }
   }
}

HB_FUNC( MXMLSETERRORCALLBACK )
{
   PHB_ITEM pError = hb_param( 1, HB_IT_BLOCK );

   if( pError )
   {
      if( s_error_cb_var->error_cb )
         hb_itemRelease( s_error_cb_var->error_cb );

      s_error_cb_var->error_cb = hb_itemNew( pError );

      mxmlSetErrorCallback( error_cb );
   }
   else
   {
      if( s_error_cb_var && s_error_cb_var->error_cb )
      {
         hb_itemRelease( s_error_cb_var->error_cb );
         s_error_cb_var->error_cb = NULL;
      }

      mxmlSetErrorCallback( NULL );
   }
}

/* int mxmlSetInteger( mxml_node_t * node, int integer ) */

HB_FUNC( MXMLSETINTEGER )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
      hb_retni( mxmlSetInteger( node, hb_parnidef( 2, 0 ) ) );
   else
      MXML_ERR_ARGS;
}

/* int mxmlSetOpaque( mxml_node_t * node, const char * opaque ) */

HB_FUNC( MXMLSETOPAQUE )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
   {
      void * hOpaque;

      hb_retni( mxmlSetOpaque( node, hb_parstr_utf8( 2, &hOpaque, NULL ) ) );

      hb_strfree( hOpaque );
   }
   else
      MXML_ERR_ARGS;
}

/* int mxmlSetReal( mxml_node_t * node, double real ) */

HB_FUNC( MXMLSETREAL )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
      hb_retni( mxmlSetReal( node, hb_parnd( 2 ) ) );
   else
      MXML_ERR_ARGS;
}

/* int mxmlSetText( mxml_node_t * node, int whitespace, const char * string ) */

HB_FUNC( MXMLSETTEXT )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
   {
      void * hString;

      hb_retni( mxmlSetText( node, hb_parnidef( 2, 1 ), hb_parstr_utf8( 3, &hString, NULL ) ) );

      hb_strfree( hString );
   }
   else
      MXML_ERR_ARGS;
}

/* int mxmlSetUserData(mxml_node_t * node, void * data ) */

HB_FUNC( MXMLSETUSERDATA )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
   {
      PHB_ITEM pItem = hb_itemClone( hb_param( 2, HB_IT_ANY ) );

      if( pItem )
         hb_retni( mxmlSetUserData( node, pItem ) );
   }
   else
      MXML_ERR_ARGS;
}

/* void mxmlSetWrapMargin( int column ) */

HB_FUNC( MXMLSETWRAPMARGIN )
{
   mxmlSetWrapMargin( hb_parni( 1 ) );
}

/* mxml_node_t * mxmlWalkNext( mxml_node_t * node,
                               mxml_node_t * top, int descend ) */

HB_FUNC( MXMLWALKNEXT )
{
   mxml_node_t *  node  = mxml_node_param( 1 );
   mxml_node_t *  top   = mxml_node_param( 2 );

   if( node && top )
   {
      mxml_node_t * node_next = mxmlWalkNext( node, top, hb_parnidef( 3, MXML_NO_DESCEND ) );

      mxml_node_ret( node_next, 0 );
   }
   else
      MXML_ERR_ARGS;
}

/* mxml_node_t * mxmlWalkPrev( mxml_node_t * node,
                               mxml_node_t * top, int descend ) */

HB_FUNC( MXMLWALKPREV )
{
   mxml_node_t *  node  = mxml_node_param( 1 );
   mxml_node_t *  top   = mxml_node_param( 2 );

   if( node && top )
      mxml_node_ret( mxmlWalkPrev( node, top, hb_parnidef( 3, MXML_NO_DESCEND ) ), 0 );
   else
      MXML_ERR_ARGS;
}

/* const void * mxmlGetCustom( mxml_node_t * node ) */

HB_FUNC( MXMLGETCUSTOM )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node )
   {
      PHB_ITEM pItem = ( PHB_ITEM ) mxmlGetCustom( node );

      if( pItem )
         hb_itemReturn( pItem );
   }
   else
      MXML_ERR_ARGS;
}

/* ============= void ( *mxml_custom_destroy_cb_t )( void * ) =============== */

static void custom_destroy_cb( void * Cargo )
{
   PHB_ITEM pItem = ( PHB_ITEM ) Cargo;

   if( pItem != NULL )
      hb_itemRelease( pItem );
}

/*
   mxml_node_t * mxmlNewCustom( mxml_node_t * parent,
                                void * data, mxml_custom_destroy_cb_t destroy )
 */

HB_FUNC( MXMLNEWCUSTOM )
{
   if( hb_pcount() > 1 )
   {
      PHB_ITEM       pItem    = hb_itemClone( hb_param( 2, HB_IT_ANY ) );
      mxml_node_t *  parent   = MXML_NO_PARENT;
      mxml_node_t *  node     = NULL;

      if( HB_ISNIL( 1 ) || ( HB_ISNUM( 1 ) && hb_parni( 1 ) == MXML_NO_PARENT ) )
         node = mxmlNewCustom( MXML_NO_PARENT, pItem, custom_destroy_cb );
      else
      {
         parent = mxml_node_param( 1 );

         if( parent != NULL )
            node = mxmlNewCustom( parent, pItem, custom_destroy_cb );
      }

      if( node )
      {
         mxml_node_ret( node, ( parent == MXML_NO_PARENT ) ? 1 : 0 );
         return;
      }

      hb_itemRelease( pItem );
   }
   MXML_ERR_ARGS;
}

/*
   int mxmlSetCustom ( mxml_node_t * node,
                       void * data, mxml_custom_destroy_cb_t destroy )
 */

HB_FUNC( MXMLSETCUSTOM )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node && hb_pcount() > 1 )
   {
      int      iResult;

      PHB_ITEM pItem = hb_itemClone( hb_param( 2, HB_IT_ANY ) );

      if( pItem )
      {
         iResult = mxmlSetCustom( node, pItem, custom_destroy_cb );

         if( iResult < 0 )
            hb_itemRelease( pItem );

         hb_retni( iResult );
      }
   }
   else
      MXML_ERR_ARGS;
}

/* ===== int ( *mxml_custom_load_cb_t )( mxml_node_t *, const char * ) ====== */

static int custom_load_cb( mxml_node_t * node, const char * data )
{
   if( node != NULL && s_custom_cbs_var != NULL && data != NULL )
   {
      if( s_custom_cbs_var->load_cb && hb_vmRequestReenter() )
      {
         int iResult;

         hb_vmPushEvalSym();
         hb_vmPush( s_custom_cbs_var->load_cb );
         mxml_node_push( node, 0 );
         hb_itemPutC( hb_stackAllocItem(), data );

         hb_vmSend( 2 );

         iResult = hb_parnidef( -1, 1 );

         hb_vmRequestRestore();
         return iResult;
      }
   }
   return 1;
}

/* ========== char * ( *mxml_custom_save_cb_t )( mxml_node_t  * ) =========== */
static char * custom_save_cb( mxml_node_t * node )
{
   if( node != NULL && s_custom_cbs_var != NULL )
   {
      if( s_custom_cbs_var->save_cb && hb_vmRequestReenter() )
      {
         char *         pszResult;
         const char *   pszText;
         void *         hText;

         hb_vmPushEvalSym();
         hb_vmPush( s_custom_cbs_var->save_cb );
         mxml_node_push( node, 0 );

         hb_vmSend( 1 );

         pszText     = hb_parstr_utf8( -1, &hText, NULL );
         pszResult   = pszText ? strdup( pszText ) : NULL;
         hb_strfree( hText );

         hb_vmRequestRestore();
         return pszResult;
      }
   }
   return NULL;
}

/* void mxmlSetCustomHandlers( mxml_custom_load_cb_t load,
                               mxml_custom_save_cb_t save ) */

HB_FUNC( MXMLSETCUSTOMHANDLERS )
{
   PHB_ITEM pLoad = hb_param( 1, HB_IT_BLOCK ),
            pSave = hb_param( 2, HB_IT_BLOCK );

   if( pLoad && pSave )
   {
      if( s_custom_cbs_var->load_cb )
         hb_itemRelease( s_custom_cbs_var->load_cb );

      s_custom_cbs_var->load_cb = hb_itemNew( pLoad );

      if( s_custom_cbs_var->save_cb )
         hb_itemRelease( s_custom_cbs_var->save_cb );

      s_custom_cbs_var->save_cb = hb_itemNew( pSave );

      mxmlSetCustomHandlers( custom_load_cb, custom_save_cb );
   }
   else
   {
      if( s_custom_cbs_var )
      {
         if( s_custom_cbs_var->load_cb )
         {
            hb_itemRelease( s_custom_cbs_var->load_cb );
            s_custom_cbs_var->load_cb = NULL;
         }
         if( s_custom_cbs_var->save_cb )
         {
            hb_itemRelease( s_custom_cbs_var->save_cb );
            s_custom_cbs_var->save_cb = NULL;
         }
      }

      mxmlSetCustomHandlers( NULL, NULL );
   }
}

/* ============ temporary solution before final 2.7 release =============== */

HB_FUNC( HB_MXMLGETATTRSCOUNT )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node && mxmlGetType( node ) == MXML_ELEMENT )
      hb_retni( node->value.element.num_attrs );
   else
      MXML_ERR_ARGS;
}

HB_FUNC( HB_MXMLGETATTRSARRAY )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node && mxmlGetType( node ) == MXML_ELEMENT )
   {
      PHB_ITEM       pAttrs   = hb_itemArrayNew( 0 ),
                     pAttr    = hb_itemNew( NULL );
      int            i;
      mxml_attr_t *  attr;

      for( i = node->value.element.num_attrs, attr = node->value.element.attrs;
           i > 0;
           i--, attr++ )
      {
         hb_arrayNew( pAttr, 2 );

         hb_arraySetStrUTF8( pAttr, 1, attr->name );
         hb_arraySetStrUTF8( pAttr, 2, attr->value );

         hb_arrayAddForward( pAttrs, pAttr );
      }
      hb_itemRelease( pAttr );

      hb_itemReturnRelease( pAttrs );
   }
   else
      MXML_ERR_ARGS;
}

HB_FUNC( HB_MXMLGETATTRS )
{
   mxml_node_t * node = mxml_node_param( 1 );

   if( node && mxmlGetType( node ) == MXML_ELEMENT )
   {
      PHB_ITEM       pAttrs   = hb_hashNew( hb_itemNew( NULL ) ),
                     pKey     = NULL,
                     pValue   = NULL;
      int            i;
      mxml_attr_t *  attr;

      for( i = node->value.element.num_attrs, attr = node->value.element.attrs;
           i > 0;
           i--, attr++ )
      {
         pKey     = hb_itemPutStrUTF8( pKey, attr->name );
         pValue   = hb_itemPutStrUTF8( pValue, attr->value );

         hb_hashAdd( pAttrs, ULONG_MAX, pKey, pValue );
      }
      hb_itemRelease( pKey );
      hb_itemRelease( pValue );

      hb_itemReturnRelease( pAttrs );
   }
   else
      MXML_ERR_ARGS;
}

static void hb_hbxmlInit( void * cargo )
{
   HB_SYMBOL_UNUSED( cargo );

   s_cbs_var                  = ( HB_CBS_VAR * ) hb_xgrab( sizeof( HB_CBS_VAR ) );
   s_custom_cbs_var           = ( HB_CUSTOM_CBS_VAR * ) hb_xgrab( sizeof( HB_CUSTOM_CBS_VAR ) );
   s_error_cb_var             = ( HB_ERROR_CB_VAR * ) hb_xgrab( sizeof( HB_ERROR_CB_VAR ) );

   s_cbs_var->type_cb         = NULL;
   s_cbs_var->save_cb         = NULL;
   s_cbs_var->sax_cb          = NULL;

   s_custom_cbs_var->load_cb  = NULL;
   s_custom_cbs_var->save_cb  = NULL;
   s_error_cb_var->error_cb   = NULL;
}

static void hb_hbxmlExit( void * cargo )
{
   HB_SYMBOL_UNUSED( cargo );

   if( s_cbs_var )
      hb_xfree( s_cbs_var );

   if( s_custom_cbs_var )
   {
      if( s_custom_cbs_var->load_cb )
         hb_itemRelease( s_custom_cbs_var->load_cb );

      if( s_custom_cbs_var->save_cb )
         hb_itemRelease( s_custom_cbs_var->save_cb );

      hb_xfree( s_custom_cbs_var );
   }

   if( s_error_cb_var )
   {
      if( s_error_cb_var->error_cb )
         hb_itemRelease( s_error_cb_var->error_cb );

      hb_xfree( s_error_cb_var );
   }
}

//------------------------------------------------------------------------------
#define __PRG_SOURCE__        __FILE__

#ifdef HB_PCODE_VER
   #undef HB_PRG_PCODE_VER
   #define HB_PRG_PCODE_VER   HB_PCODE_VER
#endif

HB_CALL_ON_STARTUP_BEGIN( _hb_hbmxml_init_ )
hb_vmAtInit( hb_hbxmlInit, NULL );
hb_vmAtExit( hb_hbxmlExit, NULL );
HB_CALL_ON_STARTUP_END( _hb_hbmxml_init_ )

#if defined( HB_PRAGMA_STARTUP )
   #pragma startup _hb_hbmxml_init_
#elif defined( HB_DATASEG_STARTUP )
   #define HB_DATASEG_BODY HB_DATASEG_FUNC( _hb_hbmxml_init_ )
   #include "hbiniseg.h"
#endif
