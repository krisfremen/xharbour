/*
 * $Id: errorsys.prg,v 1.47 2005/05/25 13:18:01 druzus Exp $
 */

/*
 * Harbour Project source code:
 * The default error handler
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
/*
 * The following parts are Copyright of the individual authors.
 * www - http://www.harbour-project.org
 *
 * Copyright 2001 Ron Pinkas <ron@profit-master.com>
 *    TraceLog()
 *    CStr()
 * Copyright 2002 Luiz Rafael Culik <culikr@uol.com.br>
 *    StrValue()
 *    FWriteLine()
 *    LogError()
 */

#include "common.ch"
#include "error.ch"
#include "fileio.ch"

PROCEDURE ErrorSys

     Errorblock( { | oError | DefError( oError ) } )

Return

STATIC FUNCTION DefError( oError )

     LOCAL cMessage
     LOCAL cDOSError

     LOCAL aOptions
     LOCAL nChoice

     LOCAL n

     // By default, division by zero results in zero
     If oError:genCode == EG_ZERODIV
        Return 0
     Endif

     // Set NetErr() of there was a database open error
     If oError:genCode == EG_OPEN .and. ;
                oError:osCode == 32 .and. ;
                oError:canDefault
        Neterr( .T. )
        Return .F.
     Endif

     // Set NetErr() if there was a lock error on dbAppend()
     If oError:genCode == EG_APPENDLOCK .and. ;
                oError:canDefault
        Neterr( .T. )
        Return .F.
     Endif

     cMessage := ErrorMessage( oError )
     If !Empty( oError:osCode )
        cDOSError := "(DOS Error " + Ltrim( Str( oError:osCode ) ) + ")"
     Endif


     If ValType( oError:Args ) == "A"
       cMessage += " Arguments: (" + Arguments( oError ) + ")"
     Endif

     // Build buttons

     IF MaxCol() > 0
         aOptions := {}

         // AAdd( aOptions, "Break" )
         Aadd( aOptions, "Quit" )

         If oError:canRetry
            Aadd( aOptions, "Retry" )
         Endif

         If oError:canDefault
            Aadd( aOptions, "Default" )
         Endif

         // Show alert box
         //TraceLog( cMessage )

         nChoice := 0
         While nChoice == 0

            If Empty( oError:osCode )
               nChoice := Alert( cMessage, aOptions )
            Else
               nChoice := Alert( cMessage + ";" + cDOSError, aOptions )
            Endif

         Enddo

         If !Empty( nChoice )
            Do Case
                  Case aOptions[ nChoice ] == "Break"
                     Break( oError )
                  Case aOptions[ nChoice ] == "Retry"
                     Return .T.
                  Case aOptions[ nChoice ] == "Default"
                     Return .F.
            Endcase
         Endif
     Else
        If Empty( oError:osCode )
           nChoice := Alert( cMessage + ";" + oError:ProcName + "(" + LTrim( Str( oError:ProcLine() ) ) +  ") in module: " + oError:ModuleName )
        Else
           nChoice := Alert( cMessage + ";" + cDOSError + ";" + oError:ProcName + "(" + LTrim( Str( oError:ProcLine() ) ) +  ") in module: " + oError:ModuleName )
        Endif
     ENDIF

     // "Quit" selected

     If !Empty( oError:osCode )
        cMessage += " " + cDOSError
     Endif

     ? cMessage

     ?
     ? "Error at ...:", oError:ProcName + "(" + LTrim( Str( oError:ProcLine ) ) + ") in Module:", oError:ModuleName
     n := 3
     WHILE ( ! Empty(ProcName( n ) ) )
       ? "Called from :", ProcName( n ) + "(" + LTrim( Str( ProcLine( n ) ) ) + ") in Module:", ProcFile( n )
       n++
     END

     /// For some strange reason, the DOS prompt gets written on the first line
     /// *of* the message instead of on the first line *after* the message after
     /// the program quits, unless the screen has scrolled. - dgh
     LogError( oError )

     ErrorLevel(1)
     ?
     Quit

Return .F.

// [vszakats]

STATIC FUNCTION ErrorMessage( oError )

     LOCAL cMessage

     // start error message
     cMessage := Iif( oError:severity > ES_WARNING, "Error", "Warning" ) + " "

     // add subsystem name if available
     If Ischaracter( oError:subsystem )
        cMessage += oError:subsystem()
     Else
        cMessage += "???"
     Endif

     // add subsystem's error code if available
     If Isnumber( oError:subCode )
        cMessage += "/" + Ltrim( Str( oError:subCode ) )
     Else
        cMessage += "/???"
     Endif

     // add error description if available
     If Ischaracter( oError:description )
        cMessage += "  " + oError:description
     Endif

     // add either filename or operation
     Do Case
         Case !Empty( oError:filename )
             cMessage += ": " + oError:filename
         Case !Empty( oError:operation )
             cMessage += ": " + oError:operation
     Endcase

Return cMessage

STATIC FUNCTION LogError( oerr )

     LOCAL cScreen
     LOCAL aLogFile    := SET( _SET_ERRORLOG )
     LOCAL cLogFile    := aLogFile[1]
     LOCAL lAppendLog  := aLogFile[2]
     LOCAL nStart      := 1
     LOCAL nCellSize
     LOCAL nRange
     LOCAL nCols
     LOCAL nRows
     LOCAL nFhandle
     LOCAL nCount
     LOCAL nMemHandle
     LOCAL nForLoop
     LOCAL nMemLength
     LOCAL nMemWidth
     LOCAL cOutString
     LOCAL cSubString
     LOCAL cVarName
     LOCAL cVarType
     LOCAL nLenTemp
     LOCAL cTemp
     LOCAL nBytesLocal
     LOCAL cVarRec
     LOCAL nHandle
     LOCAL nBytes
     LOCAL nMemCount

     nCols := MaxCol()
     IF nCols > 0
        nRows := MaxRow()
        cScreen := Savescreen()
     ENDIF
     //Alert( 'An error occured, Information will be ;written to error.log' )

     If !lAppendLog
        nHandle := Fcreate( cLogFile, FC_NORMAL )
     Else
        If !File( cLogFile )
           nHandle := Fcreate( cLogFile, FC_NORMAL )
        Else
           nHandle := Fopen( cLogFile, FO_READWRITE )
           FSeek( nHandle, 0, FS_END )
        Endif
     Endif

     If nHandle < 3 .and. lower( cLogFile ) != 'error.log'
        // Force creating error.log in case supplied log file cannot
        // be created for any reason
        cLogFile := 'error.log'
        nHandle := Fcreate( cLogFile, FC_NORMAL )
     Endif

     If nHandle < 3
     Else
        FWriteLine( nHandle, Padc( ' Error log file ', 79, '*' ) )
        FWriteLine( nHandle, '' )
        // FWriteLine( nHandle, '' )
        // FWriteLine( nHandle, '' )
        FWriteLine( nHandle, 'Date ............: ' + Dtoc( Date() ) )
        FWriteLine( nHandle, 'Time ............: ' + Time() )
        FWriteLine( nHandle, 'Available Memory : ' + strvalue( Memory( 0 ) ) )
        FWriteLine( nHandle, 'Multi Threading  : ' + If( Hb_MultiThread(),"Yes","No" ) )
        FWriteLine( nHandle, 'VM Optimization  : ' + strvalue( Hb_VmMode() ) )
        FWriteLine( nHandle, 'Application      : ' + hb_cmdargargv() )
        FWriteLine( nHandle, 'Operating System : ' + os() )
        FWriteLine( nHandle, 'Compiler         : ' + hb_compiler() )
        FWriteLine( nHandle, 'xHarbour Version : ' + version() )
        FWriteLine( nHandle, 'Build Date       : ' + hb_builddate() )

        IF Type( "Select()" ) == "UI"
           FWriteLine( nHandle, 'Current Area ....:' + strvalue( &("Select()") ) )
        ENDIF

        FWriteLine( nHandle, '' )
        FWriteLine( nHandle, Padc( ' Environmental Information ', 79, '-' ) )
        FWriteLine( nHandle, '' )
        FWriteLine( nHandle, "Exact is ........: " + strvalue( Set( 1 ), .T. ) )
        FWriteLine( nHandle, "Fixed is ........: " + strvalue( Set( 2 ), .T. ) )
        FWriteLine( nHandle, "Decimals is at ..: " + strvalue( Set( 3 ) ) )
        FWriteLine( nHandle, "Date Format is ..: " + strvalue( Set( 4 ) ) )
        FWriteLine( nHandle, "Epoch is ........: " + strvalue( Set( 5 ) ) )
        FWriteLine( nHandle, "Path is .........: " + strvalue( Set( 6 ) ) )
        FWriteLine( nHandle, "Default is ......: " + strvalue( Set( 7 ) ) )
        FWriteLine( nHandle, "Exclusive is ....: " + strvalue( Set( 8 ), .T. ) )
        FWriteLine( nHandle, "SoftSeek is .....: " + strvalue( Set( 9 ), .T. ) )
        FWriteLine( nHandle, "Unique is .......: " + strvalue( Set( 10 ), .T. ) )
        FWriteLine( nHandle, "Deleted is ......: " + strvalue( Set( 11 ), .T. ) )
        FWriteLine( nHandle, "Cancel is .......: " + strvalue( Set( 12 ), .T. ) )
        FWriteLine( nHandle, "Debug is ........: " + strvalue( Set( 13 ) ) )
        FWriteLine( nHandle, "Color is ........: " + strvalue( Set( 15 ) ) )
        FWriteLine( nHandle, "Cursor is .......: " + strvalue( Set( 16 ) ) )
        FWriteLine( nHandle, "Console is ......: " + strvalue( Set( 17 ), .T. ) )
        FWriteLine( nHandle, "Alternate is ....: " + strvalue( Set( 18 ), .T. ) )
        FWriteLine( nHandle, "AltFile is ......: " + strvalue( Set( 19 ) ) )
        FWriteLine( nHandle, "Device is .......: " + strvalue( Set( 20 ) ) )
        FWriteLine( nHandle, "Printer is ......: " + strvalue( Set( 23 ) ) )
        FWriteLine( nHandle, "PrintFile is ....: " + strvalue( Set( 24 ) ) )
        FWriteLine( nHandle, "Margin is .......: " + strvalue( Set( 25 ) ) )
        FWriteLine( nHandle, "Bell is .........: " + strvalue( Set( 26 ), .T. ) )
        FWriteLine( nHandle, "Confirm is ......: " + strvalue( Set( 27 ), .T. ) )
        FWriteLine( nHandle, "Escape is .......: " + strvalue( Set( 28 ), .T. ) )
        FWriteLine( nHandle, "Insert is .......: " + strvalue( Set( 29 ), .T. ) )
        FWriteLine( nHandle, "Intensity is ....: " + strvalue( Set( 31 ), .T. ) )
        FWriteLine( nHandle, "Scoreboard is ...: " + strvalue( Set( 32 ), .T. ) )
        FWriteLine( nHandle, "Delimeters is ...: " + strvalue( Set( 33 ), .T. ) )
        FWriteLine( nHandle, "Delimchars is ...: " + strvalue( Set( 34 ) ) )
        FWriteLine( nHandle, "Wrap is .........: " + strvalue( Set( 35 ), .T. ) )
        FWriteLine( nHandle, "Message is ......: " + strvalue( Set( 36 ) ) )
        FWriteLine( nHandle, "MCenter is ......: " + strvalue( Set( 37 ), .T. ) )
        //FWriteLine( nHandle, "" )

        FWriteLine( nHandle, "" )
        IF nCols > 0
            FWriteLine( nHandle, Padc( 'Detailed Work Area Items', nCols, "=" ) )
        ELSE
            FWriteLine( nHandle, 'Detailed Work Area Items ' )
        ENDIF
        FWriteLine( nHandle, "" )

        IF Type( "Select()" ) == "UI"
           For nCount := 1 To 600
              If !Empty( ( nCount )->( &("Alias()") ) )
                 ( nCount )->( FWriteLine( nHandle, "Work Area No ....: " + strvalue( &("Select()") ) ) )
                 ( nCount )->( FWriteLine( nHandle, "Alias ...........: " + &("Alias()") ) )
                 ( nCount )->( FWriteLine( nHandle, "Current Recno ...: " + strvalue( &("RecNo()") ) ) )
                 ( nCount )->( FWriteLine( nHandle, "Current Filter ..: " + &("DbFilter()") ) )
                 ( nCount )->( FWriteLine( nHandle, "Relation Exp. ...: " + &("DbRelation()") ) )
                 ( nCount )->( FWriteLine( nHandle, "Index Order .....: " + strvalue( &("IndexOrd(0)") ) ) )
                 ( nCount )->( FWriteLine( nHandle, "Active Key ......: " + strvalue( &("IndexKey(0)") ) ) )
                 ( nCount )->( FWriteLine( nHandle, "" ) )
              Endif
           Next
        ENDIF

        FWriteLine( nHandle, "" )
        IF nCols > 0
            FWriteLine( nHandle, Padc( " Internal Error Handling Information  ", nCols, "+" ) )
        ELSE
            FWriteLine( nHandle, " Internal Error Handling Information  " )
        ENDIF
        FWriteLine( nHandle, "" )
        FWriteLine( nHandle, "Subsystem Call ..: " + oErr:subsystem() )
        FWriteLine( nHandle, "System Code .....: " + strvalue( oErr:suBcode() ) )
        FWriteLine( nHandle, "Default Status ..: " + strvalue( oerr:candefault() ) )
        FWriteLine( nHandle, "Description .....: " + oErr:description() )
        FWriteLine( nHandle, "Operation .......: " + oErr:operation() )
        FWriteLine( nHandle, "Arguments .......: " + Arguments( oErr ) )
        FWriteLine( nHandle, "Involved File ...: " + oErr:filename() )
        FWriteLine( nHandle, "Dos Error Code ..: " + strvalue( oErr:oscode() ) )

        #ifdef HB_THREAD_SUPPORT
        FWriteLine( nHandle, "Running threads .: " + strvalue( oErr:RunningThreads() ) )
        FWriteLine( nHandle, "VM thread ID ....: " + strvalue( oErr:VmThreadId() ) )
        FWriteLine( nHandle, "OS thread ID ....: " + strvalue( oErr:OsThreadId() ) )
        #endif

        FWriteLine( nHandle, "" )
        FWriteLine( nHandle, " Trace Through:" )
        FWriteLine( nHandle, "----------------" )

        FWriteLine( nHandle, Padr( oErr:ProcName, 21 ) + " : " + Transform( oErr:ProcLine, "999,999" ) + " in Module: " + oErr:ModuleName )
        nCount := 3
        While !Empty( Procname( ++ nCount ) )
          FWriteLine( nHandle, Padr( Procname( nCount ), 21 ) + ' : ' + Transform( Procline( nCount ), "999,999" ) + " in Module: " + ProcFile( nCount ) )
        Enddo

        FWriteLine( nHandle, "" )
        FWriteLine( nHandle, "" )

        IF valtype( cScreen ) == "C"
            FWriteLine( nHandle, Padc( " Video Screen Dump ", nCols, "#" ) )
            FWriteLine( nHandle, "" )
            //FWriteLine( nHandle, "" )
            FWriteLine( nHandle, "+" + Replicate( '-', nCols + 1 ) + "+" )
            //FWriteLine( nHandle, "" )
            nCellSize := len( Savescreen( 0, 0, 0, 0 ) )
            nRange := ( nCols + 1 ) * nCellSize
            For nCount := 1 To nRows + 1
               cOutString := ''
               cSubString := Substr( cScreen, nStart, nRange )
               For nForLoop := 1 To nRange step nCellSize
                  cOutString += Substr( cSubString, nForLoop, 1 )
               Next
               FWriteLine( nHandle, "|" + cOutString + "|" )
               nStart += nRange
            Next
            FWriteLine( nHandle, "+" + Replicate( '-', nCols + 1 ) + "+" )
            FWriteLine( nHandle, "" )
            FWriteLine( nHandle, "" )
        ELSE
            FWriteLine( nHandle, " Video Screen Dump not available" )
        ENDIF


        /*
        FWriteLine( nHandle, padc(" Available Memory Variables ",nCols,'+') )
        FWriteLine( nHandle, "" )
        Save All Like * To errormem
        nMemHandle := Fopen( 'errormem.mem', FO_READWRITE )
        nMemLength := Fseek( nMemHandle, 0, 2 )
        Fseek( nMemHandle, 0 )
        nCount := 1
        While Fseek( nMemHandle, 0, 1 ) + 1 < nMemLength
          nMemWidth := Space( 18 )
          Fread( nMemHandle, @nMemWidth, 18 )
          cVarName  := Left( nMemWidth, At( Chr( 0 ), nMemWidth ) - 1 )
          cVarType  := Substr( nMemWidth, 12, 1 )
          cVarRec   := Bin2w( Right( nMemWidth, 2 ) )
          nMemCount := If( cVarType IN Chr( 195 ) + Chr( 204 ), 14 + cVarRec, 22 )
          Fseek( nMemHandle, nMemCount, 1 )
          cTemp  := Left( cVarName + Space( 10 ), 10 )
          cTemp  += " TYPE " + Type( cVarName )
          cTemp  += " " + If( Type( cVarName ) == "C", '"' + &cVarName + '"', strvalue( &cVarName ) )
          nBytes := 0
          Switch ValType( cVarName )
              Case "C"
                  nBytes += ( nLenTemp := Len( &cVarName ) )
                  exit
              Case "N"
                  nBytes += ( nLenTemp := 9 )
                  exit
              Case 'L'
                  nBytes += ( nLenTemp := 2 )
                  exit
              Case "D"
                  nBytes += ( nLenTemp := 9 )
                  exit
          End
          Fwrite( nFhandle, "            " + Transform( nLenTemp, '999999' ) + 'bytes -> ' )
          FWriteLine( nHandle, "      " + cTemp )
        Enddo
        Fclose( nMemHandle )
        Ferase( 'errormem.mem' )
        */
        Fclose( nhandle )
     Endif

Return .f.

STATIC FUNCTION strvalue( c, l )

     LOCAL cr := ''
     Default l To .f.
     Switch ValType( c )
         Case "C"
             cr := c
             exit
         Case "N"
             cr := Alltrim( Str( c ) )
             exit
         Case "M"
             cr := c
             exit
         Case "D"
             cr := Dtoc( c )
             exit
         Case "L"
             cr := If( l, If( c, "On", "Off" ), If( c, "True", "False" ) )
             exit
     End
Return cr

STATIC FUNCTION FWriteLine( nh, c )

   Fwrite( nh, c + HB_OsNewLine() )
   //HB_OutDebug( c + HB_OsNewLine() )
Return nil

STATIC FUNCTION Arguments( oErr )

   LOCAL xArg, cArguments := ""

   IF ValType( oErr:Args ) == "A"
      FOR EACH xArg IN oErr:Args
         cArguments += " [" + Str( HB_EnumIndex(), 2 ) + "] = Type: " + ValType( xArg )

         IF xArg != NIL
            cArguments +=  " Val: " + CStr( xArg )
         ENDIF
      NEXT
   ENDIF

RETURN cArguments

#ifdef __PLATFORM__Windows
#pragma BEGINDUMP

#include "hbapi.h"
#include "hbapiitm.h"
#include "hbvm.h"
#include "hbvmpub.h"
#include "hbfast.h"
#include "hbstack.h"
#include "thread.h"

#include <windows.h>

static PHB_FUNC s_xHbFunc;

LONG WINAPI PRGUnhandledExceptionFilter( EXCEPTION_POINTERS *ExceptionInfo )
{
   PHB_DYNS pExecSym;

   hb_dynsymLock();
   pExecSym = hb_dynsymFindFromFunction( s_xHbFunc );
   hb_dynsymUnlock();

   if( pExecSym )
   {
      HB_ITEM Exception;
      PHB_DYNS pDyn = hb_dynsymFind( "HB_CSTRUCTURE" );

      Exception.type = HB_IT_NIL;

      //TraceLog( NULL, "%s(%p)\n", pExecSym->pSymbol->szName, ExceptionInfo );

      if( pDyn )
      {
         hb_vmPushSymbol( pDyn->pSymbol );
         hb_vmPushNil();
         hb_itemPushStaticString( "EXCEPTION_POINTERS", 18 );
         hb_vmPushLong( 8 );
         hb_vmDo( 2 );

         if( HB_VM_STACK.Return.type == HB_IT_OBJECT )
         {
            HB_ITEM Buffer, Adopt;

            hb_itemForwardValue( &Exception, &HB_VM_STACK.Return );

            Buffer.type = HB_IT_STRING;
            Buffer.item.asString.value = (char *) ExceptionInfo;
            Buffer.item.asString.length = sizeof( EXCEPTION_POINTERS );
            Buffer.item.asString.bStatic = TRUE;

            Adopt.type = HB_IT_LOGICAL;
            Adopt.item.asLogical.value = FALSE;

            hb_objSendMsg( &Exception, "Buffer", 2, &Buffer, &Adopt );
         }
      }

      hb_vmPushSymbol( pExecSym->pSymbol );
      hb_vmPushNil();
      hb_itemPushForward( &Exception );
      hb_vmDo( 1 );

      //TraceLog( NULL, "Done\n" );
   }

   return hb_itemGetNL( &HB_VM_STACK.Return );
}

HB_FUNC( SETERRORMODE )
{
   hb_retni( SetErrorMode( hb_parni( 1 ) ) ) ;
}

HB_FUNC( SETUNHANDLEDEXCEPTIONFILTER )
{
   LPTOP_LEVEL_EXCEPTION_FILTER pDefaultHandler;

   s_xHbFunc = (PHB_FUNC) hb_parptr( 1 );

   pDefaultHandler = SetUnhandledExceptionFilter( PRGUnhandledExceptionFilter );
   //TraceLog( NULL, "Default: %p\n", pDefaultHandler );

   hb_retnl( (LONG) pDefaultHandler );
}

#pragma ENDDUMP

#endif
*+ EOF: ERRORSYS.PRG
