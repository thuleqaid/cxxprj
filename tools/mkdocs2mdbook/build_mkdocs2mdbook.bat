@echo off
call ..\..\config_mingw.bat
call %MING32DIR%\mingwvars.bat
set SOURCES=converter/*.cpp
g++ -std=c++17 -DTEST_MODE -O2 -I../../extlibs/include -o "%~dp0test_converter.exe" -static-libgcc -static-libstdc++ -static %SOURCES%
if %errorlevel% equ 0 (
    echo Successfully built test_converter.exe
) else (
    echo Build failed with error %errorlevel%
)
g++ -std=c++17 -O2 -I../../extlibs/include -o "%~dp0mkdocs2mdbook.exe" -static-libgcc -static-libstdc++ -static %SOURCES% main.cpp
if %errorlevel% equ 0 (
    echo Successfully built mkdocs2mdbook.exe
) else (
    echo Build failed with error %errorlevel%
)
