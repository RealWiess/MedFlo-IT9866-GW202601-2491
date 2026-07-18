add_library(wiegand STATIC)

target_include_directories(wiegand
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/wiegand
)

target_sources(wiegand
PRIVATE
    wiegand.c
    wiegandout.c
)