@echo off
rem
rem $Id$
rem

rem ---------------------------------------------------------------
rem This is a generic template file, if it doesn't fit your own needs
rem please DON'T MODIFY IT.
rem
rem Instead, make a local copy and modify that one, or make a call to
rem this batch file from your customized one. [vszakats]
rem ---------------------------------------------------------------

SET HB_MT=
SET HB_GT_LIB=gtwin

call %HB_BIN_INSTALL%\bld.bat hbdict

if errorlevel 1 goto exit
copy hbdict.exe %HB_BIN_INSTALL%

REM
REM If you add a translation file, add also a copy command here
REM
copy i18n\it_IT.hit %HB_BIN_INSTALL%\hbdict_it_IT.hit

:exit

