add_library(pyinput STATIC)

target_sources(pyinput
PRIVATE
    HZ_input.c
    JY_input.c
)
