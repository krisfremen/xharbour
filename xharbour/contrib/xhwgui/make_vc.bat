@echo off
if "%1" == "clean" goto CLEAN
if "%1" == "CLEAN" goto CLEAN

if not exist obj md obj
if not exist obj\vc md obj\vc
if not exist lib md lib
if not exist lib\vc md lib\vc

SET _CF_=%CFLAGS%

:BUILD

   SET CFLAGS=-D__EXPORT__ -DHB_API_MACROS -DHB_STACK_MACROS -DHB_FM_STATISTICS_OFF
   nmake /Fmakefile.vc %1 %2 %3 > make_vc.log
   if errorlevel 1 goto BUILD_ERR

:BUILD_OK

   copy lib\vc\*.lib ..\..\lib >nul
   goto EXIT

:BUILD_ERR

   notepad make_vc.log
   goto EXIT

:CLEAN
  if exist lib\vc\*.lib del lib\vc\*.lib
  if exist obj\vc\*.obj del obj\vc\*.obj
  if exist obj\vc\*.c del obj\vc\*.c
  if exist make_vc.log del make_vc.log

:EXIT
   SET CFLAGS=%_CF_%
   SET _CF_=
