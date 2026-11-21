@echo off
if "%ROOT_PATH%"=="" goto :error
set mkdocs2mdbook=tools\mkdocs2mdbook\mkdocs2mdbook.exe

set srcpath=extsrc\header\cxxopts-3.3.1
if exist %srcpath% (
  if not exist "%ROOT_PATH%\include\cxxopts" mkdir "%ROOT_PATH%\include\cxxopts"
  if not exist "%ROOT_PATH%\doc" mkdir "%ROOT_PATH%\doc"
  xcopy /S /I /Y "%srcpath%\include\*.hpp" "%ROOT_PATH%\include\cxxopts\"
  copy /Y "%srcpath%\README.md" "%ROOT_PATH%\doc\cxxopts.md"
) else (
  echo %ERROR_COLOR%Not found: %srcpath%%NORMAL_COLOR%
)

set srcpath=extsrc\header\fff-1.1
if exist %srcpath% (
  if not exist "%ROOT_PATH%\include\fff" mkdir "%ROOT_PATH%\include\fff"
  if not exist "%ROOT_PATH%\doc" mkdir "%ROOT_PATH%\doc"
  copy /Y "%srcpath%\fff.h" "%ROOT_PATH%\include\fff\fff.h"
  copy /Y "%srcpath%\README.md" "%ROOT_PATH%\doc\fff.md"
) else (
  echo %ERROR_COLOR%Not found: %srcpath%%NORMAL_COLOR%
)

set srcpath=extsrc\header\tinyfsm-0.3.3
if exist %srcpath% (
  if not exist "%ROOT_PATH%\include\tinyfsm" mkdir "%ROOT_PATH%\include\tinyfsm"
  if not exist "%ROOT_PATH%\doc\tinyfsm" mkdir "%ROOT_PATH%\doc\tinyfsm"
  copy /Y "%srcpath%\include\tinyfsm.hpp" "%ROOT_PATH%\include\tinyfsm\tinyfsm.hpp"
  xcopy /S /I /Y "%srcpath%\doc\*" "%ROOT_PATH%\doc\tinyfsm\"
) else (
  echo %ERROR_COLOR%Not found: %srcpath%%NORMAL_COLOR%
)

set srcpath=extsrc\header\spdlog-1.17.0
if exist %srcpath% (
  if not exist "%ROOT_PATH%\include\spdlog" mkdir "%ROOT_PATH%\include\spdlog"
  if not exist "%ROOT_PATH%\doc" mkdir "%ROOT_PATH%\doc"
  xcopy /S /I /Y "%srcpath%\include\spdlog\*" "%ROOT_PATH%\include\spdlog\"
  copy /Y "%srcpath%\README.md" "%ROOT_PATH%\doc\spdlog.md"
) else (
  echo %ERROR_COLOR%Not found: %srcpath%%NORMAL_COLOR%
)

set srcpath=extsrc\header\doctest-2.5.3
if exist %srcpath% (
  if not exist "%ROOT_PATH%\doc\doctest" mkdir "%ROOT_PATH%\doc\doctest"
  xcopy /S /I /Y "%srcpath%\doc\markdown\*" "%ROOT_PATH%\doc\doctest\"
  xcopy /S /I /Y "%srcpath%\doctest\extensions" "%ROOT_PATH%\include\doctest\extensions"
  copy /Y "%srcpath%\doctest\*.h" "%ROOT_PATH%\include\doctest\"
) else (
  echo %ERROR_COLOR%Not found: %srcpath%%NORMAL_COLOR%
)

set srcpath=extsrc\header\fkYAML-0.5.0
if exist %srcpath% (
  xcopy /S /I /Y "%srcpath%\single_include\fkYAML" "%ROOT_PATH%\include\fkYAML"
  if not exist %mkdocs2mdbook% (
    call tools\mkdocs2mdbook\build_mkdocs2mdbook.bat
  )
  if exist %mkdocs2mdbook% (
    %mkdocs2mdbook% -i %srcpath%\docs -o %srcpath%\mdbook -a
    pushd %cd%
    cd %srcpath%\mdbook
    mdbook build
    popd
    xcopy /S /I /Y "%srcpath%\mdbook\book" "%ROOT_PATH%\doc\fkYAML"
  )
) else (
  echo %ERROR_COLOR%Not found: %srcpath%%NORMAL_COLOR%
)

set srcpath=extsrc\header\json-3.12.0
if exist %srcpath% (
  if not exist "%ROOT_PATH%\include\nlohmann" mkdir "%ROOT_PATH%\include\nlohmann"
  xcopy /S /I /Y "%srcpath%\single_include\nlohmann\*.hpp" "%ROOT_PATH%\include\nlohmann\"
  if not exist %mkdocs2mdbook% (
    call tools\mkdocs2mdbook\build_mkdocs2mdbook.bat
  )
  if exist %mkdocs2mdbook% (
    %mkdocs2mdbook% -i %srcpath%\docs\mkdocs -o %srcpath%\docs\mdbook -a
    pushd %cd%
    cd %srcpath%\docs\mdbook
    mdbook build
    popd
    xcopy /S /I /Y "%srcpath%\docs\mdbook\book" "%ROOT_PATH%\doc\nlohmann"
  )
) else (
  echo %ERROR_COLOR%Not found: %srcpath%%NORMAL_COLOR%
)

set srcpath=extsrc\header\inja-3.5.0
if exist %srcpath% (
  pushd %CD%
  cd "%srcpath%\doc"
  doxygen
  popd
  xcopy /S /I /Y "%srcpath%\doc\html" "%ROOT_PATH%\doc\inja"
  if not exist "%ROOT_PATH%\include\inja" mkdir "%ROOT_PATH%\include\inja"
  copy /Y "%srcpath%\single_include\inja\inja.hpp" "%ROOT_PATH%\include\inja\inja.hpp"
) else (
  echo %ERROR_COLOR%Not found: %srcpath%%NORMAL_COLOR%
)

set srcpath=extsrc\nng-2.1.0
if exist %srcpath% (
  mkdir build-nng
  cd build-nng
  cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" ..\%srcpath%
  mingw32-make install -j %JobCount%
  if %errorlevel%==0 (
    cd ..
    sed -i "/^\[preprocessor\.indexing\]/a optional = true" %srcpath%\docs\book.toml
    mdbook build %srcpath%\docs
    if not exist "%ROOT_PATH_ARCH%\doc\nng" mkdir "%ROOT_PATH_ARCH%\doc\nng"
    xcopy /S /I /Y "%srcpath%\docs\book\*" "%ROOT_PATH_ARCH%\doc\nng\"
    rmdir /S /Q build-nng
  ) else (
    cd ..
  )
) else (
  echo %ERROR_COLOR%Not found: %srcpath%%NORMAL_COLOR%
)

set srcpath=extsrc\zlib-1.3.2
if exist %srcpath% (
  mkdir build-zlib
  cd build-zlib
  cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" ..\%srcpath%
  mingw32-make install -j %JobCount%
  if %errorlevel%==0 (
    cd ..
    rmdir /S /Q build-zlib
  ) else (
    cd ..
  )
) else (
  echo %ERROR_COLOR%Not found: %srcpath%%NORMAL_COLOR%
)
set srcpath=extsrc\abseil-cpp-20250512.1
if exist %srcpath% (
  mkdir build-abseil
  cd build-abseil
  cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" ..\%srcpath%
  mingw32-make install -j %JobCount%
  if %errorlevel%==0 (
    cd ..
    rmdir /S /Q build-abseil
  ) else (
    cd ..
  )
) else (
  echo %ERROR_COLOR%Not found: %srcpath%%NORMAL_COLOR%
)
set srcpath=extsrc\protobuf-36.2
if exist %srcpath% (
  mkdir build-protobuf
  cd build-protobuf
  rem cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -Dprotobuf_WITH_ZLIB=OFF -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" -DCMAKE_PREFIX_PATH="%ROOT_PATH_ARCH%" ..\%srcpath%
  cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -Dprotobuf_WITH_ZLIB=ON -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" -DCMAKE_PREFIX_PATH="%ROOT_PATH_ARCH%" ..\%srcpath%
  mingw32-make install -j %JobCount%
  if %errorlevel%==0 (
    cd ..
    rmdir /S /Q build-protobuf
  ) else (
    cd ..
  )
) else (
  echo %ERROR_COLOR%Not found: %srcpath%%NORMAL_COLOR%
)

set srcpath=extsrc\libpng-1.6.59
if exist %srcpath% (
  mkdir build-libpng
  cd build-libpng
  cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" -DCMAKE_PREFIX_PATH="%ROOT_PATH_ARCH%" ..\%srcpath%
  mingw32-make install -j %JobCount%
  if %errorlevel%==0 (
    cd ..
    rmdir /S /Q build-libpng
  ) else (
    cd ..
  )
) else (
  echo %ERROR_COLOR%Not found: %srcpath%%NORMAL_COLOR%
)
set srcpath=extsrc\freetype-2.15.0
if exist %srcpath% (
  mkdir build-freetype
  cd build-freetype
  rem cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -D FT_REQUIRE_ZLIB=TRUE -D FT_DISABLE_BZIP2=TRUE -D FT_REQUIRE_PNG=TRUE -D FT_DISABLE_HARFBUZZ=TRUE -D FT_DISABLE_BROTLI=TRUE -D FT_DISABLE_HVF=TRUE -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" -DCMAKE_PREFIX_PATH="%ROOT_PATH_ARCH%" ..\%srcpath%
  cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -D FT_DISABLE_ZLIB=TRUE -D FT_DISABLE_BZIP2=TRUE -D FT_DISABLE_PNG=TRUE -D FT_DISABLE_HARFBUZZ=TRUE -D FT_DISABLE_BROTLI=TRUE -D FT_DISABLE_HVF=TRUE -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" -DCMAKE_PREFIX_PATH="%ROOT_PATH_ARCH%" ..\%srcpath%
  mingw32-make install -j %JobCount%
  if %errorlevel%==0 (
    cd ..
    xcopy /S /I /Y "%srcpath%/docs/reference" "%ROOT_PATH_ARCH%/doc/freetype"
    rmdir /S /Q build-freetype
  ) else (
    cd ..
  )
) else (
  echo %ERROR_COLOR%Not found: %srcpath%%NORMAL_COLOR%
)

set srcpath=extsrc\googletest-1.17.0
if exist %srcpath% (
  mkdir build-gtest
  cd build-gtest
  cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" ..\%srcpath%
  mingw32-make install -j %JobCount%
  if %errorlevel%==0 (
    cd ..
  if not exist "%ROOT_PATH_ARCH%\doc\gtest&gmock" mkdir "%ROOT_PATH_ARCH%\doc\gtest&gmock"
  xcopy /S /I /Y "%srcpath%\docs\*" "%ROOT_PATH_ARCH%\doc\gtest&gmock\"
    rmdir /S /Q build-gtest
  ) else (
    cd ..
  )
) else (
  echo %ERROR_COLOR%Not found: %srcpath%%NORMAL_COLOR%
)

set srcpath=extsrc\uchardet-0.0.8
if exist %srcpath% (
  mkdir build-uchardet
  sed -i "s/^cmake_minimum_required.*/cmake_minimum_required(VERSION 4.0)/" %srcpath%\CMakeLists.txt
  cd build-uchardet
  cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" ..\%srcpath%
  mingw32-make install -j %JobCount%
  if %errorlevel%==0 (
    cd ..
    rmdir /S /Q build-uchardet
  ) else (
    cd ..
  )
) else (
  echo %ERROR_COLOR%Not found: %srcpath%%NORMAL_COLOR%
)
set srcpath=extsrc\mbedtls-4.2.0
if exist %srcpath% (
  mkdir build-mbedtls
  cd build-mbedtls
  cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" ..\%srcpath%
  mingw32-make install -j %JobCount%
  cmake --build . --target mbedtls-apidoc
  if %errorlevel%==0 (
    cd ..
    if not exist "%ROOT_PATH_ARCH%\doc\mbedtls" mkdir "%ROOT_PATH_ARCH%\doc\mbedtls"
    xcopy /S /I /Y "%srcpath%\apidoc\*" "%ROOT_PATH_ARCH%\doc\mbedtls\"
    rmdir /S /Q build-mbedtls
  ) else (
    cd ..
  )
) else (
  echo %ERROR_COLOR%Not found: %srcpath%%NORMAL_COLOR%
)
set srcpath=extsrc\vector_blf-2.4.2
if exist %srcpath% (
  mkdir build-vector_blf
  cd build-vector_blf
  cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" -DCMAKE_PREFIX_PATH="%ROOT_PATH_ARCH%" ..\%srcpath%
  mingw32-make install -j %JobCount%
  if %errorlevel%==0 (
    cd ..
    rmdir /S /Q build-vector_blf
  ) else (
    cd ..
  )
) else (
  echo %ERROR_COLOR%Not found: %srcpath%%NORMAL_COLOR%
)

goto :done
:error
set ERROR_COLOR=[31;47m
set NORMAL_COLOR=[0m
echo %ERROR_COLOR%Should be called by buildext_xnn.bat%NORMAL_COLOR%
:done
