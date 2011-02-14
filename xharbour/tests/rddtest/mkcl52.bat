@echo off
rem DBFCDX
clipper rddmktst /m/n/w/es2/d_TEST_CMPDIDX_
if errorlevel 1 goto error
rtlink fi rddmktst lib dbfcdx
if errorlevel 1 goto error
del *.obj
rddmktst cdxcl52.prg dbfcdx
clipper cdxcl52 /m/n/w/es2
if errorlevel 1 goto error
rtlink fi cdxcl52 lib dbfcdx
if errorlevel 1 goto error

rem DBFNTX
clipper rddmktst /m/n/w/es2
if errorlevel 1 goto error
rtlink fi rddmktst lib dbfcdx
if errorlevel 1 goto error
del *.obj
rddmktst ntxcl52.prg dbfntx
clipper ntxcl52 /m/n/w/es2
if errorlevel 1 goto error
rtlink fi ntxcl52 lib dbfcdx
if errorlevel 1 goto error

rem DBFCDX for ADS RDD
clipper rddmktst /m/n/w/es2/d_TEST_ADS_ /d_TEST_CMPDIDX_
if errorlevel 1 goto error
rtlink fi rddmktst lib dbfcdx
if errorlevel 1 goto error
del *.obj
rddmktst adscl52.prg dbfcdx
clipper adscl52 /m/n/w/es2
if errorlevel 1 goto error
rtlink fi adscl52 lib dbfcdx
if errorlevel 1 goto error


:error
if exist *.obj del *.obj
