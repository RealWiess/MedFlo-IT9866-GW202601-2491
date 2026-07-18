add_library(tre STATIC)

target_include_directories(tre
PRIVATE
    .
)

target_compile_definitions(tre
PRIVATE
    HAVE_CONFIG_H
)

target_sources(tre
PRIVATE
    xmalloc.c
    tre-mem.c
    tre-stack.c
    tre-ast.c
    tre-parse.c
    tre-compile.c
    tre-match-parallel.c
    tre-match-backtrack.c
    tre-match-approx.c
    regcomp.c
    regexec.c
    regerror.c
)