add_library(iic_sw STATIC)

target_include_directories(iic_sw
PRIVATE
    .
    ${PROJECT_SOURCE_DIR}/sdk/include/iic
)

target_sources(iic_sw
PRIVATE
    iic_sw.c
    iic.c
)