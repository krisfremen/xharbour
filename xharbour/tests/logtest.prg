************************************************************
* logtest.prg
* $Id: logtest.prg,v 1.5 2003/07/22 11:35:11 jonnymind Exp $
*
* Demonstrates the standard log system
* See inline help strings to know how to use this
*
* To see an example of dynamic log channel creation, see the
* ActivateEmail() procedure.
*
* (C) Giancarlo Niccolai
*

#include "hblog.ch"
#include "inkey.ch"
GLOBAL oEmail
GLOBAL oDebug

Procedure MAIN()
   LOCAL nChoice
   LOCAL cMessage
   LOCAL GetList := {}

   // This is an emailer log object, initialized if needed by
   // the procedure ActivateEmail()

   // We'll use this to allow users to change level.
   LOCAL aLevelNames := { ;
      "Critical",;
      "Error", ;
      "Warning", ;
      "Information", ;
      "Debug", ;
      "Debug (lower)", ;
      "Debug (still lower)";
   }
   // mapping level names to real HB_ log levels
   LOCAL aLevels := { ;
      HB_LOG_CRITICAL,;
   HB_LOG_ERROR, ;
   HB_LOG_WARNING, ;
   HB_LOG_INFO, ;
   HB_LOG_DEBUG, ;
   HB_LOG_DEBUG+1, ;
   HB_LOG_DEBUG+2 ;
   }

   SET COLOR TO w+/b
   CLEAR SCREEN
   @1,22 SAY "X H A R B O U R - Log test "
   @3,3 SAY "Select a log priority and then write a string in the field."
   @4,3 SAY "The string will be reported to the log with this predecence:"
   @5,5 SAY "To the file logtest.log: only INFO or above"
   @6,5 SAY "To the system log: only error or critical."
   @7,5 SAY "To a monitor internet port: connect to this host at port 7761."
   @8,5 SAY "To 'console': all messages. Press F1 to activate EMAIL log, F2 for debug"
   @9,3 SAY "Press ESC to select another priority; Press it again to exit."
   @10,3 SAY "** To demonstrate self log file rolling feature, log file limit is set to 2K"

   // Log can be initialized in any moment
   // Other than the HB_Logger class, there is a "standard log" that is
   // a static HB_Logger instantation, that is accessed with some functions.
   // The following instructions are preprocessor wrappers.

   // preparing a "virual console"
   @21,0 SAY "---- Virtual console  ----------------------------------------"
   @22,0 SAY Space( 80 )
   @22,0

   INIT LOG ON ;
      File( HB_LOG_INFO, "logtest.log", 2, 5 ) ;
      CONSOLE() ;
      SYSLOG( HB_LOG_ERROR, 0xff33ff33ff33) ;
      MONITOR() ;
      NAME "Log test program"

   //The above creates the default HB_Logger and automatically adds
   // HB_LogFile channel to logtest.log, INFO and above, 2k, max 5 backups
   // HB_LogSyslog, ERROR and above, application code 0x3ffaaffaa
   // HB_LogConsole, any level.
   // Also, the application name for logging is set to "Log Test Program


   // a demo of how the log works from within a program
   // LOG can accept also a set of strings or variables separated by commas
   // PRIO[RITY] is optional, If omitted, will log as INFO
   @22,0 SAY Space( 80 )
   @22,0
   LOG "LogTest", "Hello", "debug" PRIO HB_LOG_DEBUG

   @13,30 SAY "Insert message to log below:"
   cMessage := Space(45)

   nChoice := 1
   SET KEY K_F1 TO ActivateEmail()
   SET KEY K_F2 TO ActivateDebug()
   DO WHILE nChoice > 0
      MakeBox( 12,2, 20, 25 )
      @12,5 SAY  "Select Level: "
      nChoice := Achoice(13, 4, 19, 24 , aLevelNames)

      IF nChoice > 0 .and. LastKey() != K_F1
         DO While LastKey() != K_ESC
            @14,30 GET cMessage
            READ
            IF LastKey() != K_ESC .and. LastKey() != K_F1
               // Preparing virtual console
               @22,0 SAY Space(80)
               @22,0

               LOG cMessage PRIO aLevels[ nChoice ]
               cMessage := Space( 45 )
            ENDIF
         ENDDO
      ENDIF
   ENDDO

   // Log closing creates a log INFO message
   @22,0 SAY Space(80)
   @22,0
   // closing the log
   CLOSE LOG

   @24,0

RETURN


PROCEDURE MakeBox( nRow, nCol, nRowTo, nColTo )
   @nRow, nCol, nRowTo, nColTo ;
      BOX( Chr( 201 ) + Chr( 205 ) + Chr( 187 ) + Chr( 186 ) +;
      Chr( 188 ) + Chr( 205 ) + Chr( 200 ) + Chr( 186 ) + Space( 1 ) )
RETURN


PROCEDURE ActivateEmail()
   LOCAL cFrom
   LOCAL cTo
   LOCAL cSubject
   LOCAL cServer
   LOCAL GetList := {}
   LOCAL bToadd := .F.

   /* Creation of a new email, or modification of the old one */
   IF oEmail == NIL
      oEmail := HB_LogEmail():New( HB_LOG_CRITICAL, "Log test program",  ;
         "mail.myserver.net", "admin@host")

      oEmail:cPrefix := "This message has been automatically generated by " + InetCRLF() +;
         "Log test program from xharbour.org test suite." + InetCRLF() +;
         "It is generated in response to a critical log request," + InetCRLF() +;
         "usually to inform administrators of a server going down.";

      oEmail:cPostfix := "-----------------------------" + InetCRLF() +;
         "Log system and logtest are by Giancarlo Niccolai. " + InetCRLF() + ;
         "Hope you enjoy them!"

      // if the user don't press ESC, we'll add the channel to the logger
      bToAdd := .T.
   ENDIF

   /* Getting data for read */
   cServer  := padr( oEmail:cServer  , 40 )
   cFrom    := padr( oEmail:cAddress , 40 )
   cTo      := padr( oEmail:cSendTo  , 40 )
   cSubject := padr( oEmail:cSubject , 40 )

   /* Just a nice visualization */
   SAVE SCREEN
   SET COLOR TO w+/N
   @13,11 CLEAR TO 21,71
   SET COLOR TO g+/b
   MakeBox( 12, 10, 20, 70 )
   @12, 14 SAY " E-mail channel "

   SET CONFIRM ON
   @13, 12 SAY "SMTP server: " GET cServer
   @14, 12 SAY "Mail from  : " GET cFrom
   @15, 12 SAY "Mail to    : " GET cTo
   @16, 12 SAY "Subject    : " GET cSubject
   @18, 12 SAY "** Set priority to 'CRITICAL' to send an e-mail"
   READ

   IF Lastkey() != K_ESC
      oEmail:cServer  := Alltrim( cServer  )
      oEmail:cAddress := Alltrim( cFrom    )
      oEmail:cSendTo  := Alltrim( cTo      )
      oEmail:cSubject := Alltrim( cSubject )
      // If it's the first time...
      IF bToAdd
         // let's add this channel to the standard logger
         HB_StandardLogAdd( oEmail )
      ENDIF
   ENDIF

   SET COLOR TO w+/b
   RESTORE SCREEN

RETURN

PROCEDURE ActivateDebug()
   IF oDebug == NIL
      oDebug := HB_LogDebug():New( HB_LOG_DEBUG+2, HB_LOG_DEBUG )
      oDebug:open()
      HB_StandardLogAdd( oDebug )

      @22,0 SAY Space(80)
      @22,0
      LOG "Opened debug logger" PRIORITY HB_LOG_DEBUG
      @22,0 SAY Space(80)
      @22,0
      LOG "This window will display only messages with priority DEBUG or lower" ;
         PRIORITY HB_LOG_DEBUG
   ENDIF
RETURN
