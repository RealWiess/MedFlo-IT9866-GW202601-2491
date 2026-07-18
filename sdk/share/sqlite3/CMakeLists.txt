add_library(sqlite3 STATIC)

target_compile_definitions(sqlite3
PRIVATE
    SQLITE_MAX_MMAP_SIZE=0
    #SQLITE_NO_SYNC
    SQLITE_OMIT_RANDOMNESS
    #SQLITE_OMIT_SHARED_CACHE
    SQLITE_OMIT_WAL=1
    SQLITE_OS_UNIX=1
    #SQLITE_THREADSAFE=0
)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    target_compile_definitions(sqlite3
    PRIVATE
        SQLITE_DEBUG
        SQLITE_DEBUG_OS_TRACE=1
        SQLITE_ENABLE_IOTRACE
        SQLITE_FORCE_OS_TRACE
    )
endif()

target_sources(sqlite3
PRIVATE
    sqlite3-all.c
    sqlite3.h
)