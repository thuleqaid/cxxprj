set(HAS_FILE 0)
if(EXISTS "${TARGET_FILE}")
    set(HAS_FILE 1)
endif()

set(HEADER_CONTENT "// Automatic genereted, do not edit!\n#pragma once\n#define HAVE_DYNAMIC_FILE ${HAS_FILE}\n")

set(NEED_WRITE TRUE)
if(EXISTS "${OUTPUT_HEADER}")
    file(READ "${OUTPUT_HEADER}" OLD_CONTENT)
    if(OLD_CONTENT STREQUAL HEADER_CONTENT)
        set(NEED_WRITE FALSE)
    endif()
endif()

if(NEED_WRITE)
    file(WRITE "${OUTPUT_HEADER}" "${HEADER_CONTENT}")
endif()

unset(NEED_WRITE)
unset(HEADER_CONTENT)
unset(OLD_CONTENT)
unset(HAS_FILE)