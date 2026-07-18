add_library(speedy STATIC)

target_sources(speedy
PRIVATE
    speedy_comp.c
    speedy_comp.h
    speedy_decomp.c
    speedy_main.c
)