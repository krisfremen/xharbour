#include <shlobj.h>
#include <windows.h>
#include <commctrl.h>
#include "hbapi.h"

//-------------------------------------------------------------------------//

HB_FUNC( TVINSERTITEM ) // ( hWnd, cItemText, hItem, nImage )
{
   TV_INSERTSTRUCT is;

//   _bset( ( LPBYTE ) &is, 0, sizeof( TV_INSERTSTRUCT ) );

   is.hParent      = ( HTREEITEM ) hb_parnl( 3 );
   is.hInsertAfter = TVI_LAST;

   #if (_WIN32_IE >= 0x0400)
      is.DUMMYUNIONNAME.item.pszText = hb_parc( 2 );
      is.DUMMYUNIONNAME.item.mask    = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
      is.DUMMYUNIONNAME.item.iImage  = hb_parnl( 4 );
      is.DUMMYUNIONNAME.item.iSelectedImage = hb_parnl( 4 );
   #else
      is.item.pszText = hb_parc( 2 );
      is.item.mask    = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
      is.item.iImage  = hb_parnl( 4 );
      is.item.iSelectedImage = hb_parnl( 4 );
   #endif

   hb_retnl( SendMessage( ( HWND ) hb_parnl( 1 ), TVM_INSERTITEM, 0,
           ( LPARAM )( LPTV_INSERTSTRUCT )( &is ) ) );
}

//-------------------------------------------------------------------------//

HB_FUNC( TVSETIMAGELIST ) // ( hWnd, hImageList, nType )
{
   hb_retnl( ( LONG ) TreeView_SetImageList( ( HWND ) hb_parnl( 1 ),
            ( HIMAGELIST ) hb_parnl( 2 ), hb_parnl( 3 ) ) );
}

//-------------------------------------------------------------------------//

HB_FUNC( TVGETSELTEXT ) // ( hWnd ) --> cText
{
   HWND hWnd = ( HWND ) hb_parnl( 1 );
   HTREEITEM hItem = TreeView_GetSelection( hWnd );
   TV_ITEM tvi;
   BYTE buffer[ 100 ];
   if( hItem )
   {
      tvi.mask       = TVIF_TEXT;
      tvi.hItem      = hItem;
      tvi.pszText    = buffer;
      tvi.cchTextMax = 100;
      TreeView_GetItem( hWnd, &tvi );
      hb_retc( tvi.pszText );
   }
   else
      hb_retc( "" );
}

//-------------------------------------------------------------------------//

HB_FUNC( TVGETSELECTED ) // ( hWnd ) --> hItem
{
   hb_retnl( ( LONG ) TreeView_GetSelection( ( HWND ) hb_parnl( 1 ) ) );
}

//-------------------------------------------------------------------------//