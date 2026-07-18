if (DEFINED CFG_SW_UART AND NOT $ENV{CFG_PLATFORM} STREQUAL win32)
    add_library(swUart)

    target_include_directories(swUart
    PRIVATE
        ${CMAKE_BINARY_DIR}/../alt_cpu/lib/sm32
    )

    target_sources(swUart
    PRIVATE
        swUartDevice.c
    )
endif()
