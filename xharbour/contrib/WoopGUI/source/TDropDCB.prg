/*
*-----------------------------------------------------------------------------
* WoopGUI for Harbour - Win32 OOP GUI library source code
* Copyright 2002 Francesco Saverio Giudice <info@fsgiudice.com>
*
*-----------------------------------------------------------------------------
* Parts of this project come from:
* "Harbour MiniGUI"
*                   Copyright 2002 Roberto Lopez <roblez@ciudad.com.ar>
*                   http://www.geocities.com/harbour_minigui/
* "Harbour GUI framework for Win32"
*                   Copyright 2001 Alexander S.Kresin <alex@belacy.belgorod.su>
*                   Copyright 2001 Antonio Linares <alinares@fivetech.com>
*                   http://www.harbour-project.org
*-----------------------------------------------------------------------------
*
*/

#include "woopgui.ch"
#include "common.ch"
#include "hbclass.ch"
#include "winuser.ch"

// Windows definitions
CLASS TDropDownComboBox FROM TComboBox

    // METODI
    METHOD New() CONSTRUCTOR

ENDCLASS

METHOD New( cName, nRow, nCol, nWidth, nHeight, oParent, aRows, bAction, cToolTip, cStatusBar, lPixel, nID, nValue ) CLASS TDropDownComboBox

    DEFAULT lPixel TO TRUE

    ASSIGN ::cName     WITH cName    DEFAULT "DropDCB_1"
    ASSIGN ::nStyle    WITH WS_VISIBLE + WS_CHILD + WS_TABSTOP + WS_VSCROLL + CBS_DROPDOWN
    ASSIGN ::nRow      WITH nRow     DEFAULT 0
    ASSIGN ::nCol      WITH nCol     DEFAULT 0
    ASSIGN ::nWidth    WITH nWidth   DEFAULT IIF( lPixel, 100, WG_Pixel2DialogX( 100 ) )
    ASSIGN ::nHeight   WITH nHeight  DEFAULT IIF( lPixel, 100, WG_Pixel2DialogY( 100 ) )

    // Creo l'istanza tramite la classe window
    ::Super:New( ::cName, ::nStyle, ;
                          ::nRow, ::nCol, ::nWidth, ::nHeight, ;
                          oParent, aRows, bAction, cToolTip, cStatusBar, lPixel, nID, nValue )

RETURN Self

