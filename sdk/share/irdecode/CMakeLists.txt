add_library(irdecode STATIC)

target_include_directories(irdecode
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/
)

target_sources(irdecode
PRIVATE
    ir_nec.c
)
