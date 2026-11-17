cmake_minimum_required(VERSION 4.0)

set(ENABLE_TEST OFF)
set(MinGW_DYNAMIC_LIB ON)
set(DYNAMIC_CHECK OFF)
# Use folder name as the project name
string(REGEX REPLACE "^.+/([^/]+)$" "\\1" PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR})

project(${PROJECT_NAME} C CXX)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src srcfiles)
# list(REMOVE_ITEM srcfiles "${CMAKE_CURRENT_SOURCE_DIR}/src/xxx.cpp")

add_library(${PROJECT_NAME} STATIC ${srcfiles})
target_compile_options(${PROJECT_NAME}
    PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<BOOL:${GNU_BIG_OBJ_FLAG_ENABLE}>>:-Wa,-mbig-obj>
    )

if(NOT EXISTS "${ROOT_DIR}/extsrc/check_file.cmake")
    set(DYNAMIC_CHECK OFF)
endif()
if(DYNAMIC_CHECK)
    # Usage:
        #include "dynamic_check.h"
        #if HAVE_DYNAMIC_FILE==1
        #include "dynamic_xxx.h"
        #endif
    set(CHECK_TARGET_FILE "${CMAKE_CURRENT_SOURCE_DIR}/inc/dynamic_xxx.h")
    set(GENERATED_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/inc/dynamic_check.h")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
        -DTARGET_FILE=${CHECK_TARGET_FILE}
        -DOUTPUT_HEADER=${GENERATED_HEADER}
        -P "${ROOT_DIR}/extsrc/check_file.cmake"
    )
    add_custom_target(check_dynamic_file
        COMMAND ${CMAKE_COMMAND}
            -DTARGET_FILE=${CHECK_TARGET_FILE}
            -DOUTPUT_HEADER=${GENERATED_HEADER}
            -P "${ROOT_DIR}/extsrc/check_file.cmake"
        BYPRODUCTS "${GENERATED_HEADER}"
    )
    add_dependencies(${PROJECT_NAME} check_dynamic_file)
    unset(GENERATED_HEADER)
    unset(CHECK_TARGET_FILE)
endif()

if(DOCTEST AND ENABLE_TEST)
    add_executable(test_${PROJECT_NAME} ${srcfiles})
    target_compile_features(test_${PROJECT_NAME} PRIVATE cxx_std_17)
    target_compile_definitions(test_${PROJECT_NAME} PUBLIC -DTEST_MODE)
    set_target_properties(test_${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${ROOT_DIR}/build")
    if(MinGW_STATIC_LIB AND (NOT MinGW_DYNAMIC_LIB))
        target_link_libraries(test_${PROJECT_NAME} -static gcc stdc++ winpthread -dynamic)
    endif()
    if(DYNAMIC_CHECK)
        add_dependencies(${PROJECT_NAME} check_dynamic_file)
    endif()
endif()
unset(DYNAMIC_CHECK)
unset(MinGW_DYNAMIC_LIB)
unset(ENABLE_TEST)