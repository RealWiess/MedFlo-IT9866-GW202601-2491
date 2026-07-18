add_library(nedmalloc STATIC)

target_compile_definitions(nedmalloc
PRIVATE
    USE_ALLOCATOR=1
)

target_compile_options(nedmalloc
PRIVATE
    -Wno-deprecated-declarations
)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    target_compile_definitions(nedmalloc
    PRIVATE
        DEBUG
    )
endif()

if ($ENV{CFG_PLATFORM} STREQUAL win32)
    target_compile_definitions(nedmalloc
    PRIVATE
        STRUCT_MALLINFO_DECLARED=1
    )
endif()

target_sources(nedmalloc
PRIVATE
    malloc.c.h
    nedmalloc.c
    nedmalloc.h
)
