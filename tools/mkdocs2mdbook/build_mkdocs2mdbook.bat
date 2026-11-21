@echo off
call %~dp0..\..\config_mingw.bat
set COMPILER="%MING32DIR%\bin\g++.exe"
set SOURCES=%~dp0converter\*.cpp
if /i "%~1"=="all" (
    %COMPILER% -std=c++17 -DTEST_MODE -O2 -I%~dp0..\..\extlibs\include -o "%~dp0test_converter.exe" -static-libgcc -static-libstdc++ -static %SOURCES%
    if %errorlevel% equ 0 (
        echo Successfully built test_converter.exe
    ) else (
        echo Build failed with error %errorlevel%
    )
)
%COMPILER% -std=c++17 -O2 -I%~dp0..\..\extlibs\include -o "%~dp0mkdocs2mdbook.exe" -static-libgcc -static-libstdc++ -static %SOURCES% %~dp0main.cpp
if %errorlevel% equ 0 (
    echo Successfully built mkdocs2mdbook.exe
) else (
    echo Build failed with error %errorlevel%
)
