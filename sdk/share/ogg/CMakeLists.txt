add_library(ogg STATIC)

target_sources(ogg
PRIVATE
    bitwise.c
    framing.c
)