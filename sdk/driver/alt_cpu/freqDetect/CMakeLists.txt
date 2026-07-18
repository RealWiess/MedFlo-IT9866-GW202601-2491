if (DEFINED CFG_FREQ_DETECT AND NOT $ENV{CFG_PLATFORM} STREQUAL win32)
    add_library(freqDetect)

    target_include_directories(freqDetect
    PRIVATE
        ${CMAKE_BINARY_DIR}/../alt_cpu/lib/sm32
    )

    target_sources(freqDetect
    PRIVATE
        freqDetectDevice.c
    )
endif()
