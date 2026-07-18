add_library(ir)

target_include_directories(ir
PRIVATE
    .
    ${PROJECT_SOURCE_DIR}/sdk/include/ir
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_sources(ir
PRIVATE
    ir.c
)
