if (DEFINED CFG_PATTERN_GEN AND NOT $ENV{CFG_PLATFORM} STREQUAL win32)
    add_library(patternGen)

    target_include_directories(patternGen
    PRIVATE
        ${CMAKE_BINARY_DIR}/../alt_cpu/lib/sm32
    )

    target_sources(patternGen
    PRIVATE
        patternGenDevice.c
    )

endif()
