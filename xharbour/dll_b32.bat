@echo off
REM
REM $Id: dll_b32.bat,v 1.6 2005/03/03 14:16:44 andijahja Exp $
REM
REM ��������������������������������������������Ŀ
REM � This is a batch file to create harbour.dll ��
REM � Please adjust envars accordingly           ��
REM �����������������������������������������������
REM  ����������������������������������������������

if not exist obj md obj
if not exist obj\dll md obj\dll
if not exist obj\dll\b32 md obj\dll\b32
if not exist lib\b32 md lib\b32

:BUILD

   make -fhrbdll.bc %1 %2 %3 > dll_b32.log
   if errorlevel 1 goto BUILD_ERR
   if "%1" == "clean" goto CLEAN
   if "%1" == "CLEAN" goto CLEAN

:BUILD_OK

if exist hdll.tmp del hdll.tmp
if exist bin\b32\harbour.lib copy bin\b32\harbour.lib lib > nul
if exist bin\b32\harbour.dll copy bin\b32\harbour.dll lib > nul
if exist bin\b32\harbour.dll copy bin\b32\harbour.dll tests > nul
if exist lib\b32\harbour.lib copy lib\b32\harbour.lib lib > nul

goto EXIT

:BUILD_ERR

notepad dll_b32.log
goto EXIT

:CLEAN
  if exist dll_b32.log del dll_b32.log

:EXIT
