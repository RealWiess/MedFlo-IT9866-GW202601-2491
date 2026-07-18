if (DEFINED CFG_RSL_MASTER AND NOT $ENV{CFG_PLATFORM} STREQUAL win32)
    add_library(rslMaster)

    target_include_directories(rslMaster
    PRIVATE
        ${CMAKE_BINARY_DIR}/../alt_cpu/lib/sm32
    )

    target_sources(rslMaster
    PRIVATE
        rslMasterDevice.c
    )
endif()
