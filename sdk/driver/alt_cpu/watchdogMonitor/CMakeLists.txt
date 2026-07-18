if (DEFINED CFG_WATCHDOG_MONITOR AND NOT $ENV{CFG_PLATFORM} STREQUAL win32)
    add_library(watchdogMonitor)

    target_include_directories(watchdogMonitor
    PRIVATE
        ${CMAKE_BINARY_DIR}/../alt_cpu/lib/sm32
    )

    target_sources(watchdogMonitor
    PRIVATE
        watchdogMonitorDevice.c
    )
endif()
