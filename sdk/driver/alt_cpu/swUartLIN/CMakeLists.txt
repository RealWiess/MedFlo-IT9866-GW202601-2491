if (DEFINED CFG_SW_UART_LIN AND NOT $ENV{CFG_PLATFORM} STREQUAL win32)
    add_library(swUartLIN)
    
    target_include_directories(swUartLIN
    PRIVATE
        ${CMAKE_BINARY_DIR}/../alt_cpu/lib/sm32
    )

    target_sources(swUartLIN
    PRIVATE
        swUartLINDevice.c
    )
endif()
