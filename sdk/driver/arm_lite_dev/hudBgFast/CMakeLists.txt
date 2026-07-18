if (DEFINED CFG_ARMLITE_HUD_BG_FAST AND NOT $ENV{CFG_PLATFORM} STREQUAL win32)
    add_library(hudBgFast)

    target_include_directories(hudBgFast
    PRIVATE
        ${PROJECT_SOURCE_DIR}/build/$ENV{CFG_BUILDPLATFORM}/arm_lite_dev/lib/fa626
    )

    target_sources(hudBgFast
    PRIVATE
        hudBgFast.c
    )
endif()
