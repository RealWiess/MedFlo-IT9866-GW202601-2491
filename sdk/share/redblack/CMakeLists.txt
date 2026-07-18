add_library(redblack STATIC)

target_include_directories(redblack
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/redblack
)

target_sources(redblack
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/redblack/redblack.h
    redblack.c
)