if (DEFINED CFG_ARMLITE_CANBUS AND NOT $ENV{CFG_PLATFORM} STREQUAL win32)
    add_library(can_bus_lite)

    target_include_directories(can_bus_lite
    PRIVATE
        ${PROJECT_SOURCE_DIR}/build/$ENV{CFG_BUILDPLATFORM}/arm_lite_dev/lib/fa626
    )

    target_sources(can_bus_lite
    PRIVATE
        can_bus_lite.c
    )
endif()
