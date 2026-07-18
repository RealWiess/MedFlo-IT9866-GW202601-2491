if (DEFINED CFG_OLED_CTRL AND NOT $ENV{CFG_PLATFORM} STREQUAL win32)
    add_library(oledCtrl)

    target_include_directories(oledCtrl
    PRIVATE
        ${CMAKE_BINARY_DIR}/../alt_cpu/lib/sm32
    )

    target_sources(oledCtrl
    PRIVATE
        oledCtrl.c
    )
endif()
