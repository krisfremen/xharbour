/*
 * $Id: hbmutils.prg,v 1.11 2001/11/09 00:29:23 lculik Exp $
 */

/*
 * Harbour Project source code:
 * hbmutils utility functions for hbmake
 *
 * Copyright 2000 Luiz Rafael Culik <culik@sl.conex.net>
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

 */


#include "common.ch"

#ifndef __HARBOUR__

#include 'hbclip.ch'

#else

DECLARE extenprg( cExt AS STRING, nType AS NUMERIC ) AS STRING

declare exten( cExt as string, nType as numeric ) as string

DECLARE GetSourceFiles( lSubdir as logical ) as ARRAY

DECLARE GetDirs( cPat as USUAL ) as Array

DECLARE GetBccDir() as String

DECLARE GetVccDir() as String

DECLARE GetMakeDir() as String

DECLARE HB_ARGV( n as numeric ) as string

declare filedate( c as String ) as string

declare listasArray2( cString as String, cSep as String ) as Array

#endif



Function GetSourceFiles( lSubdir,lgcc ,cOs)



     Local adirs AS ARRAY

     Local aRet AS ARRAY := {}

     Local lLinux:=at('linux',lower(os()))>0

     Local cdir  := If( at('Linux',cOs)==0, '\' + Curdir() + '\', '/' + Curdir() + '/' )

     Local aStru     := { cDir }

     Local aData AS ARRAY

     Local nCounter as numeric := 0

     Local nArrayLen as numeric

     Local nDatalen as numeric

     Local y as numeric

     Local cItem as String

     Local cext

     Local cpath

     Local cdrive

     Local nPos

     Local xItem

     Default lSubdir To .t.

     default lGcc to .f.


     While ++ nCounter <= Len( aStru )

       If !Empty( adirs := GetDirs( astru[ nCounter ] ) )   // There are elements!

          Aeval( aDirs, { | xItem | Aadd( aStru, xItem +if(lGcc,"/","\")) } )

       Endif

     Enddo

     aDirs := {}



     Asort( aStru )

     nArrayLen := Len( aStru )



     For nCounter := 1 To nArrayLen



        If Len( aData := Directory( aStru[ nCounter ] + "*.*" ) ) != 0



           nDataLen := Len( aData )

           For y := 1 To nDataLen

              If At( '.PRG', Upper( adata[ y, 1 ] ) ) > 0 .or. At( '.C', Upper( adata[ y, 1 ] ) ) > 0

                 If lSubdir

                    Aadd( aRet, Strtran( astru[ nCounter ], cDir, '' ) + Pad( aData[ y, 1 ], 13 ) + ;
                          Str( aData[ y, 2 ], 8 ) + '  ' + ;
                          Dtoc( aData[ y, 3 ] ) + '  ' + ;
                          aData[ y, 4 ] )

                 Elseif !lsubdir .and. At( If( lGcc, "/", "\" ), Strtran( astru[ nCounter ], cDir, '' ) ) == 0

                    Aadd( aRet, Pad( aData[ y, 1 ], 13 ) + ;
                      Str( aData[ y, 2 ], 8 ) + '  ' + ;
                          Dtoc( aData[ y, 3 ] ) + '  ' + ;
                          aData[ y, 4 ] )

                 Endif

              Endif

           Next

        Endif

     Next

     For nCounter := 1 To Len( aret )



        xItem := Substr( aret[ nCounter ], Rat( If( lGcc, "/", '\' ), aret[ nCounter ] ) + 1 )



        nPos := Ascan( astru, { | x | x := Substr( x, Rat( if(lGcc,"/",'\'), x ) + 1 ), Left( x, At( ".", x ) ) == Left( xitem, At( ".", xitem ) ) } )

        If nPos > 0

           Adel( astru, nPos )

           Asize( astru, Len( astru ) - 1 )

        Endif



     Next

     For nCounter := 1 To Len( aStru )

        hb_FNAMESPLIT( Left( astru[ nCounter ], At( ' ', astru[ nCounter ] ) - 1 ), @cPath, @cItem, @cExt, @cDrive )

        If ( cExt == '.C' ) .or. ( cExt == ".c" )

           Aadd( aret, astru[ nCounter ] )

        Endif

     Next

Return aRet



Function extenprg( cExt, nType )



     Local aext AS ARRAY := { "C", "c" }

     Local nPos AS NUMERIC

     Local cTemp AS String := ""

     nPos := Ascan( aext, { | a | a == cExt } )

     If nPos > 0

        If nTYpe == 1

           cTemp := Strtran( cExt, aExt[ nPos ], 'prg' )

        Elseif ntype == 2

           cTemp := Strtran( cExt, aExt[ nPos ], 'prG' )

        Elseif ntype == 3

           cTemp := Strtran( cExt, aExt[ nPos ], 'pRg' )

        Elseif ntype == 4

           cTemp := Strtran( cExt, aExt[ nPos ], 'Prg' )

        Elseif ntype == 5

           cTemp := Strtran( cExt, aExt[ nPos ], 'PRg' )

        Elseif ntype == 6

           cTemp := Strtran( cExt, aExt[ nPos ], 'PrG' )

        Elseif ntype == 7

           cTemp := Strtran( cExt, aExt[ nPos ], 'PRG' )



        Endif

     Endif

Return ctemp



Static Function GetDirs( cPattern )

     Local aDir   := {}
     Local lLinux := At( 'linux', Os() ) > 0
     Aeval( Directory( cPattern + "*.", "D" ), ;
            { | xItem | If( xItem[ 5 ] = "D" .and. if(!llinux , xItem[ 1 ] != "." .and. xItem[ 1 ] != ".." ,), ;
            ( Aadd( aDir, cPattern + xItem[ 1 ] + If( llinux, "\", '/' ) ), ;
            Outstd( "." ) ), "" ) } )
Return ( aDir )



Static Function GetDirsl( cPattern )



     Local aDir   := {}
     qout(cPattern) 
     Aeval( Directory( cPattern+"*.*", "D" ), ;
            { | xItem | If( xItem[ 5 ] == "D" , ;
             Aadd( aDir, cPattern + xItem[ 1 ] ), "" ) } )



Return ( aDir )



Function GetBccDir()



     Local cPath := ''

     Local cEnv  := Gete( "PATH" )

     Local aEnv  := listasarray2( cEnv, ";" )

     Local nPos



     For nPos := 1 To Len( aEnv )

        If File( aenv[ nPos ] + '\bcc32.exe' ) .or. File( Upper( aenv[ nPos ] ) + '\BCC32.EXE' )

           cPath := aenv[ nPos ]

           cPath := Left( cPath, Rat( '\', cPath ) - 1 )

           Exit

        Endif

     Next



Return cPath

Function GetVccDir()



     Local cPath AS STRING := ''

     Local cEnv AS STRING := Gete( "PATH" )

     Local aEnv as array of string := listasarray2( cEnv, ";" )

     Local nPos as numeric



     For nPos := 1 To Len( aEnv )

        If File( aenv[ nPos ] + '\cl.exe' ) .or. File( Upper( aenv[ nPos ] ) + '\cl.EXE' )

           cPath := aenv[ nPos ]

           cPath := Left( cPath, Rat( '\', cPath ) - 1 )

           Exit

        Endif

     Next



Return cPath



Function exten( cExt, nType )



     Local aext as array := { 'C', 'c' }

     Local nPos as numeric

     Local cTemp as string := ""

     nPos := Ascan( aext, { | a | a == cExt } )

     If nPos > 0

        If nTYpe == 1

           cTemp := Strtran( cExt, aExt[ nPos ], 'o' )

        Elseif ntype == 2

           cTemp := Strtran( cExt, aExt[ nPos ], 'obj' )

        Endif

     Endif

Return ctemp

Function ListAsArray2( cList, cDelimiter )



     Local nPos as numeric

     Local aList as array := {}              // Define an empty array



     If cDelimiter = NIL

        cDelimiter := ","

     Endif

     //

     Do While ( nPos := At( cDelimiter, cList ) ) != 0

       Aadd( aList, Alltrim( Substr( cList, 1, nPos - 1 ) ) )                   // Add a new element

       cList := Substr( cList, nPos + 1 )

     Enddo

     Aadd( aList, Alltrim( cList ) )    // Add final element

     //

Return aList        // Return the array



Function GetMakeDir()



     Local cPath := ""

     Local cExe  := HB_ARGV( 0 )



     cPath := Left( cexe, Rat( "\", cexe ) - 1 )

     cPath := Left( cPath, Rat( "\", cPath ) - 1 )



Return cPath



Function GetSourceDirMacros(lgcc,cOs)



     Local adirs AS ARRAY

     Local lLinux:=At("linux",lower(os()))>0

     Local cdir := If( at('linux',lower(os()))>0, '/' + Curdir() + '/', '\' + Curdir() + '\' )

     Local aStru  := { cDir }



     Local nCounter  := 0

     Local amacros := {}



     While ++ nCounter <= Len( aStru )

       If !Empty( adirs := GetDirs( astru[ nCounter ] ) )   // There are elements!

          Aeval( aDirs, { | xItem | Aadd( aStru, xItem+if(lGcc,"/","\") ) } )

       Endif

     Enddo

/*     else

     While ++ nCounter <= Len( aStru )

       If !Empty( adirs := GetDirsl( astru[ nCounter ] ) )   // There are elements!

          Aeval( aDirs, { | xItem | Aadd( aStru, xItem+"/" ) } )

       Endif

     Enddo


     endif*/

     For nCounter := 1 To Len( aStru )

        Aadd( amacros, { "SRC" + Strzero( nCounter, 2, 0 ), Strtran( astru[ nCounter ], cDir, '' ),.f. } )

     Next

Return amacros



Function filedate( cFileName )



     Local aFiles := Directory( cFileName )



Return If( Len( aFiles ) == 1, aFiles[ 1, 3 ], Ctod( '' ) )



Function filetime( cFileName )



     Local aFiles := Directory( cFileName )



Return If( Len( aFiles ) == 1, aFiles[ 1, 4 ], '' )



Function TD2JUL( CTIME, DDATE )



Return DDATE - Ctod( '01/01/1900' ) + ( PRB_INT( TTOS( CTIME ) / 100000,, 5 ) )



Function TTOS( CTIME )



Return ( Val( Substr( CTIME, 7, 2 ) ) ) + ;
         ( Val( Substr( CTIME, 4, 2 ) ) * 60 ) + ;
         ( Val( Substr( CTIME, 1, 2 ) ) * 3600 )



Function PRB_INT( SOMENUMBER, length, NUM_DECIMALS )



     Local NEGATIVE   := ( SOMENUMBER < 0 )

     Local SOMESTRING

     Local dotat



     Default NUM_DECIMALS To 0

     Default length To 19



     If NEGATIVE

        SOMENUMBER := Abs( SOMENUMBER )

     Endif



     SOMENUMBER += .0000000000000005



     SOMESTRING := Alltrim( Str( SOMENUMBER ) )



     dotat := At( '.', somestring )



     Do Case

         Case NUM_DECIMALS == 0

             If dotat > 0

                somestring := Left( somestring, dotat - 1 )

             Endif

         Case NUM_DECIMALS > 0

             If dotat > 0

                somestring := Left( somestring, dotat + num_decimals )

             Endif

     Endcase



     If NEGATIVE

        SOMESTRING := '-' + SOMESTRING

     Endif



Return Val( SOMESTRING )

Function exte( cExt, nType )



     Local aext  := { 'prg', 'prG', 'pRg', 'Prg', 'PRg', 'PrG', 'PRG' }

     Local nPos

     Local cTemp := ""

     nPos := Ascan( aext, { | a | a == cExt } )

     If nPos > 0

        If nTYpe == 1

           cTemp := Strtran( cExt, aExt[ nPos ], 'c' )

        Elseif ntype == 2

           cTemp := Strtran( cExt, aExt[ nPos ], 'obj' )

        Elseif ntype == 3

           cTemp := Strtran( cExt, aExt[ nPos ], 'o' )



        Endif

     Endif

Return ctemp

Procedure ATTENTION( CSTRING, NLINENUM, CCOLOR )



     Local COLDCOLOR



     Default NLINENUM To 24

     Default CCOLOR To 'GR+/R'



     COLDCOLOR := Setcolor( CCOLOR )



     CSTRING := '  ' + Alltrim( CSTRING ) + '  '



     Devpos( NLINENUM, c( CSTRING ) )



     Devout( CSTRING )



     Setcolor( COLDCOLOR )



Return



Function c( CSTRING )



Return Max( ( Maxcol() / 2 ) - Int( Len( CSTRING ) / 2 ), 0 )



Function ReadLN( leof )



     Local cBuffer := ""

     cBuffer := FT_FREADLN()

     cBuffer := Strtran( cBuffer, Chr( 13 ), '' )

     cBuffer := Strtran( cBuffer, Chr( 10 ), '' )

     FT_FSKIP( 1 )

     leof := ft_FEOF()

Return cBuffer



*+ EOF: HBMUTILS.PRG

