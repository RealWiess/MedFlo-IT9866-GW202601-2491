add_library(nor STATIC)

target_link_libraries(nor PRIVATE axispi)

if (DEFINED CFG_ARM_NOR_WRITER)
    target_compile_definitions(nor
    PRIVATE
        ARM_NOR_WRITER
    )
endif()

target_include_directories(nor
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/nor
)

target_sources(nor
PRIVATE
    nor.c
    nor_msg.h
)
