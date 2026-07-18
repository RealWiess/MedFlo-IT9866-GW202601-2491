if (DEFINED CFG_SW_PWM AND NOT $ENV{CFG_PLATFORM} STREQUAL win32)
    add_library(swPwm)

    target_include_directories(swPwm
    PRIVATE
        ${CMAKE_BINARY_DIR}/../alt_cpu/lib/sm32
    )

    target_sources(swPwm
    PRIVATE
        swPwmDevice.c
    )
endif()
