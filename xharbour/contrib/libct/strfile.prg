* �� Program ���������������������������������������������������������������Ŀ
* �  Application: CA-TOOLS 3.0                                               �
* �  Description: StrFile() work around                                      �
* �    File Name: STRFILE.PRG                                                �
* �       Author: Flemming Ho                                                �
* � Date created: 09-28-93              Date updated: �09-28-93              �
* � Time created: 02:39:32pm            Time updated: �02:39:32pm            �
* �    Exec File: StrFile.obj           Docs By: Roy Johanson                �
* �    Copyright: (c) 1993 by Computer Associates                            �
* ����������������������������������������������������������������������������

#include "fileio.ch"
function StrFile( cString, cFile, lOverwrite, nOffSet, lCutOff )

   local nHandle
   local nBytes := 0

   lOverWrite := IF( lOverWrite == NIL, .F., lOverWrite )
   lCutOff := IF( lCutOff == NIL, .F., lCutOff )

   if lOverWrite .and. file( cFile )
      /* TODO: when SETSHARE()/GETSHARE() will be implemented respect
               flags returned by GETSHARE() function */
      nHandle := Fopen( cFile, FO_WRITE )
   else
      /* TODO: when SETFCREATE() will be implemented use
               flags returned by this function */
      nHandle := Fcreate( cFile )
      lCutOff := .F.
   endif
   if nHandle > 0
      if nOffSet == NIL
         Fseek( nHandle, 0, FS_END )
         lCutOff := .F.
      else
         Fseek( nHandle, nOffSet, FS_SET )
      endif
      nBytes := Fwrite( nHandle, cString, len( cString ) )
      if lCutOff
         /* FWRITE with 0 bytes trunc the file */
         Fwrite( nHandle, "", 0 )
      endif
      Fclose( nHandle )
   endif
   return( nBytes )
