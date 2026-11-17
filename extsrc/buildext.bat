@echo off
if "%ROOT_PATH%"=="" goto :setpath
goto :done
:setpath
set JobCount=%NUMBER_OF_PROCESSORS%
set ERROR_COLOR=[31;47m
set NORMAL_COLOR=[0m
cd ..
mkdir extlibs
cd extlibs
set ROOT_PATH_ARCH=%cd%
cd ..
:done

set srcpath=extsrc/nng-2.1.0
if exist %srcpath% (
  mkdir build-nng
  cd build-nng
  cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" ../%srcpath%
  mingw32-make install -j %JobCount%
  if %errorlevel%==0 (
    cd ..
    sed -i "/^\[preprocessor\.indexing\]/a optional = true" %srcpath%/docs/book.toml
    mdbook build %srcpath%/docs
    xcopy /S /I /Y "%srcpath%/docs/book" "%ROOT_PATH_ARCH%/doc/nng"
    rmdir /S /Q build-nng
  ) else (
    cd ..
  )
) else (
  echo %ERROR_COLOR%Not found: %srcpath%%NORMAL_COLOR%
)

set srcpath=extsrc/zlib-1.3.2
if exist %srcpath% (
  mkdir build-zlib
  cd build-zlib
  cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" ../%srcpath%
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
set srcpath=extsrc/abseil-cpp-20250512.1
if exist %srcpath% (
  mkdir build-abseil
  cd build-abseil
  cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" ../%srcpath%
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
set srcpath=extsrc/protobuf-36.2
if exist %srcpath% (
  mkdir build-protobuf
  cd build-protobuf
  rem cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -Dprotobuf_WITH_ZLIB=OFF -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" -DCMAKE_PREFIX_PATH="%ROOT_PATH_ARCH%" ../%srcpath%
  cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -Dprotobuf_WITH_ZLIB=ON -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" -DCMAKE_PREFIX_PATH="%ROOT_PATH_ARCH%" ../%srcpath%
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

set srcpath=extsrc/libpng-1.6.59
if exist %srcpath% (
  mkdir build-libpng
  cd build-libpng
  cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" -DCMAKE_PREFIX_PATH="%ROOT_PATH_ARCH%" ../%srcpath%
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
set srcpath=extsrc/freetype-2.15.0
if exist %srcpath% (
  mkdir build-freetype
  cd build-freetype
  rem cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -D FT_REQUIRE_ZLIB=TRUE -D FT_DISABLE_BZIP2=TRUE -D FT_REQUIRE_PNG=TRUE -D FT_DISABLE_HARFBUZZ=TRUE -D FT_DISABLE_BROTLI=TRUE -D FT_DISABLE_HVF=TRUE -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" -DCMAKE_PREFIX_PATH="%ROOT_PATH_ARCH%" ../%srcpath%
  cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -D FT_DISABLE_ZLIB=TRUE -D FT_DISABLE_BZIP2=TRUE -D FT_DISABLE_PNG=TRUE -D FT_DISABLE_HARFBUZZ=TRUE -D FT_DISABLE_BROTLI=TRUE -D FT_DISABLE_HVF=TRUE -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" -DCMAKE_PREFIX_PATH="%ROOT_PATH_ARCH%" ../%srcpath%
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

set srcpath=extsrc/googletest-1.17.0
if exist %srcpath% (
  mkdir build-gtest
  cd build-gtest
  cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" ../%srcpath%
  mingw32-make install -j %JobCount%
  if %errorlevel%==0 (
    cd ..
    xcopy /S /I /Y "%srcpath%/docs" "%ROOT_PATH_ARCH%/doc/gtest&gmock"
    rmdir /S /Q build-gtest
  ) else (
    cd ..
  )
) else (
  echo %ERROR_COLOR%Not found: %srcpath%%NORMAL_COLOR%
)
set srcpath=extsrc/doctest-2.5.3
if exist %srcpath% (
  mkdir build-doctest
  cd build-doctest
  cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" ../%srcpath%
  mingw32-make install -j %JobCount%
  if %errorlevel%==0 (
    cd ..
    xcopy /S /I /Y "%srcpath%/doc/markdown" "%ROOT_PATH_ARCH%/doc/doctest"
    rmdir /S /Q build-doctest
  ) else (
    cd ..
  )
) else (
  echo %ERROR_COLOR%Not found: %srcpath%%NORMAL_COLOR%
)

set srcpath=extsrc/uchardet-0.0.8
if exist %srcpath% (
  mkdir build-uchardet
  sed -i "s/^cmake_minimum_required.*/cmake_minimum_required(VERSION 4.0)/" %srcpath%/CMakeLists.txt
  cd build-uchardet
  cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" ../%srcpath%
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
set srcpath=extsrc/mbedtls-4.2.0
if exist %srcpath% (
  mkdir build-mbedtls
  cd build-mbedtls
  cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" ../%srcpath%
  mingw32-make install -j %JobCount%
  cmake --build . --target mbedtls-apidoc
  if %errorlevel%==0 (
    cd ..
    xcopy /S /I /Y "%srcpath%/apidoc" "%ROOT_PATH_ARCH%/doc/mbedtls"
    rmdir /S /Q build-mbedtls
  ) else (
    cd ..
  )
) else (
  echo %ERROR_COLOR%Not found: %srcpath%%NORMAL_COLOR%
)
set srcpath=extsrc/vector_blf-2.4.2
if exist %srcpath% (
  mkdir build-vector_blf
  cd build-vector_blf
  cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS=-fPIC -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%ROOT_PATH_ARCH%" -DCMAKE_PREFIX_PATH="%ROOT_PATH_ARCH%" ../%srcpath%
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
