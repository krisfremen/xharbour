@echo off

REM
REM $Id: dll_vc.bat,v 1.2 2003/05/27 20:41:29 paultucker Exp $
REM
REM 旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴커
REM � This is a batch file to create harbour.dll 넴
REM 읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸�
REM  賽賽賽賽賽賽賽賽賽賽賽賽賽賽賽賽賽賽賽賽賽賽賽

if not exist obj md obj
if not exist obj\dll md obj\dll
if not exist obj\dll\vc md obj\dll\vc

   nmake /f hrbdll.vc %1 %2 %3 >dll_vc.log
   if errorlevel 1 goto BUILD_ERR

   copy lib\vc\harbour.lib lib > nul
   copy lib\vc\harbour.exp lib > nul
   copy lib\vc\harbour.dll lib > nul

   goto EXIT

:BUILD_ERR
   notepad hrbdllvc.log

:EXIT
