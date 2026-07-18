if (NOT $ENV{CFG_PLATFORM} STREQUAL win32)
    add_library(risc STATIC)

    target_include_directories(risc
    PRIVATE
        ${PROJECT_SOURCE_DIR}/sdk/include/
        ${CMAKE_BINARY_DIR}/lib/sm32
    )

    target_sources(risc
    PRIVATE
        it${CFG_CHIP_FAMILY}/ite_risc.c
    )

endif()
