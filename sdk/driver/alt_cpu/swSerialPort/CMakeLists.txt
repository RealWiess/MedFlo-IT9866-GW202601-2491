if (DEFINED CFG_SW_SERIAL_PORT AND NOT $ENV{CFG_PLATFORM} STREQUAL win32)
    add_library(swSerialPort)

    target_include_directories(swSerialPort
    PRIVATE
        ${CMAKE_BINARY_DIR}/../alt_cpu/lib/sm32
    )

    target_sources(swSerialPort
    PRIVATE
        swSerialPortDevice.c
    )
endif()
