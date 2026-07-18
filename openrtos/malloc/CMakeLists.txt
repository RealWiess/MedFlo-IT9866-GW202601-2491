add_library(malloc)

target_compile_definitions(malloc
PRIVATE
    HAVE_MMAP=0
    INTERNAL_NEWLIB
    MORECORE_CANNOT_TRIM
    USE_LOCKS=1
)

target_compile_options_ifdef(CFG_GCC_LTO malloc
PRIVATE
    "-fno-lto"
)

target_sources(malloc
PRIVATE
    calloc.c
    malign.c
    malloc.c
    mallocr.c
    msize.c
    mstats.c
    mtrim.c
    realloc.c
    valloc.c
)
