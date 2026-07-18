add_library(zlib STATIC)

target_compile_definitions(zlib
PRIVATE
    IOAPI_NO_64
)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    target_compile_definitions(zlib
    PRIVATE
        DEBUG
    )
endif()

target_sources(zlib
PRIVATE
    adler32.c
    compress.c
    crc32.c
    deflate.c
    gzclose.c
    gzlib.c
    gzread.c
    gzwrite.c
    infback.c
    inffast.c
    inflate.c
    inftrees.c
    trees.c
    uncompr.c
    zutil.c
    contrib/minizip/ioapi.c
    contrib/minizip/mztools.c
    contrib/minizip/unzip.c
    contrib/minizip/zip.c
)