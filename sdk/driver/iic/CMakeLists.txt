add_library(iic STATIC)

target_include_directories(iic
PRIVATE
    .
    ${PROJECT_SOURCE_DIR}/sdk/include/iic
)

target_sources(iic
PRIVATE
    iic.c
)
