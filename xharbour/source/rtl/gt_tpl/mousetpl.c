/*
 * $Id: mousetpl.c,v 1.1.1.1 2001/12/21 10:42:24 ronpinkas Exp $
 */

/*
 * Harbour Project source code:
 * {Harbour Mouse Subsystem Template}
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

/* This definition has to be placed before #include "hbapigt.h" */
#define HB_GT_FNPREF(x) tpl ## x

#include "hbapigt.h"

/* C callable low-level interface */

void HB_GT_FUNC(mouse_Init( void ))
{
   /* TODO: */
}

void HB_GT_FUNC(mouse_Exit( void ))
{
   /* TODO: */
}

BOOL HB_GT_FUNC(mouse_IsPresent( void ))
{
   /* TODO: */

   return FALSE;
}

void HB_GT_FUNC(mouse_Show( void ))
{
   /* TODO: */
}

void HB_GT_FUNC(mouse_Hide( void ))
{
   /* TODO: */
}

int HB_GT_FUNC(mouse_Col( void ))
{
   /* TODO: */

   return 0;
}

int HB_GT_FUNC(mouse_Row( void ))
{
   /* TODO: */

   return 0;
}

void HB_GT_FUNC(mouse_SetPos( int iRow, int iCol ))
{
   /* TODO: */

   HB_SYMBOL_UNUSED( iRow );
   HB_SYMBOL_UNUSED( iCol );
}

BOOL HB_GT_FUNC(mouse_IsButtonPressed( int iButton ))
{
   /* TODO: */

   HB_SYMBOL_UNUSED( iButton );

   return FALSE;
}

int HB_GT_FUNC(mouse_CountButton( void ))
{
   /* TODO: */

   return 0;
}

void HB_GT_FUNC(mouse_SetBounds( int iTop, int iLeft, int iBottom, int iRight ))
{
   /* TODO: */

   HB_SYMBOL_UNUSED( iTop );
   HB_SYMBOL_UNUSED( iLeft );
   HB_SYMBOL_UNUSED( iBottom );
   HB_SYMBOL_UNUSED( iRight );
}

void HB_GT_FUNC(mouse_GetBounds( int * piTop, int * piLeft, int * piBottom, int * piRight ))
{
   /* TODO: */

   HB_SYMBOL_UNUSED( piTop );
   HB_SYMBOL_UNUSED( piLeft );
   HB_SYMBOL_UNUSED( piBottom );
   HB_SYMBOL_UNUSED( piRight );
}
