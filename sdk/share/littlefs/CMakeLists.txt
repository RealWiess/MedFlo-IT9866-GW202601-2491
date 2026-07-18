add_library(littlefs STATIC)

target_compile_definitions(littlefs
PRIVATE
    LFS_YES_TRACE0=1
)

target_include_directories(littlefs
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/littlefs
)

target_sources(littlefs
PRIVATE
    lfs.c
    lfs_util.c
)

if (NOT CFG_LFS_CACHE_SIZE STREQUAL 0)
    target_sources(littlefs
    PRIVATE
        lfs_cache.c
        lfs_flash_interface.c
    )
endif()