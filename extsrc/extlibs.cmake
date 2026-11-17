if (GMOCK OR GTEST)
  add_library(gtest UNKNOWN IMPORTED)
  add_library(gtest_main UNKNOWN IMPORTED)
  if (MSVC)
    set_target_properties(gtest PROPERTIES
      IMPORTED_LOCATION ${EXT_LIBS_ARCH_ROOT}/lib/gtest.lib
      INTERFACE_INCLUDE_DIRECTORIES "${EXT_LIBS_ARCH_ROOT}/include"
    )
    set_target_properties(gtest_main PROPERTIES
      IMPORTED_LOCATION ${EXT_LIBS_ARCH_ROOT}/lib/gtest_main.lib
      INTERFACE_INCLUDE_DIRECTORIES "${EXT_LIBS_ARCH_ROOT}/include"
    )
  else ()
    set_target_properties(gtest PROPERTIES
      IMPORTED_LOCATION ${EXT_LIBS_ARCH_ROOT}/lib/libgtest.a
      INTERFACE_INCLUDE_DIRECTORIES "${EXT_LIBS_ARCH_ROOT}/include"
    )
    set_target_properties(gtest_main PROPERTIES
      IMPORTED_LOCATION ${EXT_LIBS_ARCH_ROOT}/lib/libgtest_main.a
      INTERFACE_INCLUDE_DIRECTORIES "${EXT_LIBS_ARCH_ROOT}/include"
    )
  endif ()
  add_definitions(-DENABLE_GTEST)
endif ()
if (GMOCK)
  add_library(gmock UNKNOWN IMPORTED)
  add_library(gmock_main UNKNOWN IMPORTED)
  if (MSVC)
    set_target_properties(gmock PROPERTIES
      IMPORTED_LOCATION ${EXT_LIBS_ARCH_ROOT}/lib/gmock.lib
      INTERFACE_INCLUDE_DIRECTORIES "${EXT_LIBS_ARCH_ROOT}/include"
    )
    set_target_properties(gmock_main PROPERTIES
      IMPORTED_LOCATION ${EXT_LIBS_ARCH_ROOT}/lib/gmock_main.lib
      INTERFACE_INCLUDE_DIRECTORIES "${EXT_LIBS_ARCH_ROOT}/include"
    )
  else ()
    set_target_properties(gmock PROPERTIES
      IMPORTED_LOCATION ${EXT_LIBS_ARCH_ROOT}/lib/libgmock.a
      INTERFACE_INCLUDE_DIRECTORIES "${EXT_LIBS_ARCH_ROOT}/include"
    )
    set_target_properties(gmock_main PROPERTIES
      IMPORTED_LOCATION ${EXT_LIBS_ARCH_ROOT}/lib/libgmock_main.a
      INTERFACE_INCLUDE_DIRECTORIES "${EXT_LIBS_ARCH_ROOT}/include"
    )
  endif ()
  add_definitions(-DENABLE_GTEST)
  add_definitions(-DENABLE_GMOCK)
endif ()

if (NNG)
  find_package(nng CONFIG REQUIRED)
  add_definitions(-DNNG_ELIDE_DEPRECATED)
  set(EXTLIBS ${EXTLIBS} nng::nng)
endif()

if (FREETYPE)
  find_package(Freetype REQUIRED)
  set(EXTLIBS ${EXTLIBS} ${FREETYPE_LIBRARIES})
  include_directories(${EXT_LIBS_ARCH_ROOT}/include/freetype2)
endif()

if (UCHARDET)
  find_package (uchardet)
  set(EXTLIBS ${EXTLIBS} uchardet::libuchardet)
  file(COPY_FILE "${EXT_LIBS_ARCH_ROOT}/bin/libuchardet.dll" "${CMAKE_BINARY_DIR}/libuchardet.dll" RESULT STATUS)
endif()

if (ZLIB)
  find_package(ZLIB REQUIRED)
  set(EXTLIBS ${EXTLIBS} ZLIB::ZLIB)
  file(COPY_FILE "${EXT_LIBS_ARCH_ROOT}/bin/libz.dll" "${CMAKE_BINARY_DIR}/libz.dll" RESULT STATUS)
endif ()

if (PNG)
  find_package(PNG REQUIRED)
  set(EXTLIBS ${EXTLIBS} PNG::PNG)
  file(COPY_FILE "${EXT_LIBS_ARCH_ROOT}/bin/libpng16.dll" "${CMAKE_BINARY_DIR}/libpng16.dll" RESULT STATUS)
endif ()

if (Vector_BLF)
  set(EXTLIBS ${EXTLIBS} Vector_BLF)
  file(COPY_FILE "${EXT_LIBS_ARCH_ROOT}/lib/libVector_BLF.dll" "${CMAKE_BINARY_DIR}/libVector_BLF.dll" RESULT STATUS)
endif ()
