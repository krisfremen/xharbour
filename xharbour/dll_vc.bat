@echo off

REM
REM $Id: dll_vc.bat,v 1.1 2002/01/07 04:06:26 andijahja Exp $
REM
REM ��������������������������������������������Ŀ
REM � This is a batch file to create harbour.dll ��
REM �����������������������������������������������
REM  ����������������������������������������������

if not exist obj md obj
if not exist obj\dll md obj\dll
if not exist obj\dll\vc md obj\dll\vc

   nmake /NOLOGO /f hrbdll.vc %1 %2 %3 >hrbdllvc.log
   if errorlevel 1 goto BUILD_ERR

   copy lib\vc\harbour.lib lib > nul
   copy lib\vc\harbour.exp lib > nul
   copy lib\vc\harbour.dll lib > nul

   goto EXIT

:BUILD_ERR
   notepad hrbdllvc.log
   goto EXIT

:EXIT
