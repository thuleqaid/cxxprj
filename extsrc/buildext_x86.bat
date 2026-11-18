@echo off
call ..\config_mingw.bat
set JobCount=%NUMBER_OF_PROCESSORS%
set ERROR_COLOR=[31;47m
set NORMAL_COLOR=[0m
cd ..
mkdir extlibs
cd extlibs
set ROOT_PATH=%cd%
cd ..
REM build 32bit library
call %MING32DIR%\mingwvars.bat
set ROOT_PATH_ARCH=%ROOT_PATH%/x86
call extsrc\buildext.bat
cd extsrc
set ROOT_PATH=