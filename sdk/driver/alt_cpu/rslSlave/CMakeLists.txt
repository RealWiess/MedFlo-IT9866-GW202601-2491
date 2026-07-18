if (DEFINED CFG_RSL_SLAVE AND NOT $ENV{CFG_PLATFORM} STREQUAL win32)
    add_library(rslSlave)

    target_include_directories(rslSlave
    PRIVATE
        ${CMAKE_BINARY_DIR}/../alt_cpu/lib/sm32
    )

    target_sources(rslSlave
    PRIVATE
        rslSlaveDevice.c
    )
endif()
