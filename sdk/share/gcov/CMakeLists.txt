add_library(gcov)

target_include_directories(gcov
PRIVATE
    src
PUBLIC
    inc
)

target_sources(gcov
PRIVATE
    src/gcov_gcc.c
    src/gcov_printf.c
    src/gcov_public.c
)
