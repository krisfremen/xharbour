//
// $Id: readhrb.prg,v 1.1.1.1 2001/12/21 10:46:24 ronpinkas Exp $
//

/*
 * ReadHRB
 *
 * This program will read the .HRB file and shows its contents
 *
 * ReadHRB <program file>  {No .HRB extension please}
 *
 * Written by Eddie Runia <eddie@runia.com>
 * www - http://www.harbour-project.org
 *
 * Placed in the public domain
 */

#include "set.ch"

function Main( cFrom )

   local hFile
   local cBlock := " "
   local n, m
   local nVal
   local nSymbols
   local nFuncs
   local cMode := "SYMBOL"
   local nScope
   local nLenCount
   local nIdx
   local aTypes := { "NOLINK", "FUNC", "EXTERN" }
   local aScopes  := { { "HB_FS_PUBLIC", 0x01 } ,;
                       { "HB_FS_STATIC", 0x02 } ,;
                       { "HB_FS_FIRST",  0x04 } ,;
                       { "HB_FS_INIT" ,  0x08 } ,;
                       { "HB_FS_EXIT" ,  0x10 } ,;
                       { "HB_FS_CRITICAL", 0x20 },;
                       { "HB_FS_MESSAGE" , 0x40 } ,;
                       { "HB_FS_MEMVAR"  , 0x80  } }

   set( _SET_EXACT, .T. )
   set( _SET_ALTERNATE, "readhrb.out" )
   set( _SET_ALTERNATE, .T. )

   if cFrom == NIL
      cFrom := "hello.hrb"
   else
      cFrom := cFrom + ".hrb"
   endif

   hFile := fOpen( cFrom )
   if hFile == -1
      QOut( "No such file:", cFrom )
   else
      // Throw away header
      fReadStr( hFile, 6 )

      cBlock := fReadStr( hFile, 4 )
      nSymbols := asc(substr(cBlock,1,1))           +;
                  asc(substr(cBlock,2,1)) *256      +;
                  asc(substr(cBlock,3,1)) *65536    +;
                  asc(substr(cBlock,4,1)) *16777216

      for n := 1 to nSymbols
         cFrom := ""
         cBlock := fReadStr( hFile, 1 )
         do while Asc( cBlock ) <> 0
            cFrom += cBlock
            cBlock := fReadStr( hFile, 1 )
         enddo

         QQOut(PadR(cFrom, 20))

         cBLock := fReadStr( hFile, 1 )
         QQOut(" Scope ", Hex2Val(asc(cBlock)))
         nScope := Asc(cBlock)
         cBlock := " "
         for m := 1 to Len(aScopes)
            if ( nScope & aScopes[m][2] ) == aScopes[m][2]
               cBlock += aScopes[m][1] + "|"
            endif
         next
         cBlock[Len(cBlock)] := ""
         QQOut(cBlock)
         cBlock := fReadStr( hFile, 1 )
         nIdx   := asc( cBlock ) + 1
         QQOut(" Type ", aTypes[ nIdx ] )
         QOut()
      next n

      cBlock := fReadStr( hFile, 4 )
      nFuncs := asc(substr(cBlock,1,1))           +;
                asc(substr(cBlock,2,1)) *256      +;
                asc(substr(cBlock,3,1)) *65536    +;
                asc(substr(cBlock,4,1)) *16777216

      for n := 1 to nFuncs
         QOut()
         cBlock := fReadStr( hFile, 1 )
         do while asc( cBlock ) != 0
            QQOut( cBlock )
            cBlock := fReadStr( hFile, 1 )
         enddo
         QOut( "Len = " )
         cBlock := Space(4)
         fRead( hFile, @cBlock, 4 )

         nLenCount := asc(substr(cBlock,1,1))           +;
                      asc(substr(cBlock,2,1)) *256      +;
                      asc(substr(cBlock,3,1)) *65536    +;
                      asc(substr(cBlock,4,1)) *16777216

         QQOut( str(nLenCount) )
         QOut()


         for m := 1 to nLenCount
            cBlock := fReadStr( hFile, 1 )
            nVal   := asc( cBlock )
            QQOut( Hex2Val( nVal ) )
            if nVal > 32 .and. nVal < 128
               QQOut( "("+cBlock+")" )
            endif
            if m < nLenCount
               QQOut(",")
            endif
         next m
      next n

      fClose( cFrom )
   end if
   set( _SET_ALTERNATE, .F. )

return nil


function Hex2Val( nVal )

return HexDigit( int(nVal / 16) ) + HexDigit( int(nVal % 16) )

function HexDigit( nDigit )

return if(nDigit>=10, chr( 55 + nDigit ), chr( 48 + nDigit ) )


