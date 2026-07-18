if (NOT DEFINED BUILD_TESTING)
    add_library(ith)
    add_library(ith_platform)

    target_link_libraries(ith_platform PRIVATE boot)

    ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_ITH_ERR)
    ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_ITH_WARN)
    ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_ITH_INFO)
    ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_ITH_DBG)

    if (DEFINED CFG_ITH_FPGA)
        add_definitions(
            -DCFG_ITH_FPGA
            -DCFG_ITH_FPGA_CLK_CPU=${CFG_ITH_FPGA_CLK_CPU}
            -DCFG_ITH_FPGA_CLK_MEM=${CFG_ITH_FPGA_CLK_MEM}
            -DCFG_ITH_FPGA_CLK_BUS=${CFG_ITH_FPGA_CLK_BUS}
        )
    endif()

    ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_BUILD_MEDIASTREAMER2)

    include_directories(
        ${CMAKE_CURRENT_BINARY_DIR}
        ${PROJECT_SOURCE_DIR}/$ENV{CFG_PLATFORM}/include
    )

    target_sources(ith
    PRIVATE
        ${PROJECT_SOURCE_DIR}/sdk/include/ith/ith_generic.h
        ith_cfg.h
        ith_chipid.c
        ith_clock.c
        ith_isp.c
        ith_dma_axi.c
        ith_host.c
        ith_interrupt.c
        ith_interrupt_handler.c
        ith_printf.c
        ith_timer.c
        ith_utility.c
        ith_list.c
    )

    include_directories(
        ${CMAKE_BINARY_DIR}/lib/sm32
    )

    if (DEFINED CFG_CMDQ_ENABLE)
        target_sources(ith
        PRIVATE
            ith_cmdq.c
        )

        target_compile_definitions(ith PRIVATE CFG_CMDQ_ENABLE)
    endif()

    if (DEFINED CFG_KEYPAD_ENABLE)
        add_library(ith STATIC
            ith_keypad.c
        )
    endif()

    if (DEFINED CFG_LCD_ENABLE)
        target_sources(ith
        PRIVATE
            ith_lcd.c
            ith_lcd.h
			ith_lcd_interface.c
			ith_lcd_interface.h
        )
    endif()

    if (DEFINED CFG_RTC_ENABLE)
        add_library(ith STATIC
            ith_rtc.c
        )
    endif()

    target_sources(ith
    PRIVATE
        ith_gpio.c
        ith_gpio_handler.c
    )

    if (DEFINED CFG_UART0_ENABLE OR DEFINED CFG_UART1_ENABLE OR DEFINED CFG_UART2_ENABLE OR DEFINED CFG_UART3_ENABLE OR DEFINED CFG_UART4_ENABLE OR DEFINED CFG_UART5_ENABLE OR DEFINED CFG_RS485_0_ENABLE OR DEFINED CFG_RS485_1_ENABLE OR DEFINED CFG_RS485_2_ENABLE OR DEFINED CFG_RS485_3_ENABLE OR DEFINED CFG_RS485_4_ENABLE OR DEFINED CFG_RS485_5_ENABLE)
        target_sources(ith
        PRIVATE
            ith_uart.c
        )
    endif()

    target_sources(ith
    PRIVATE
        ith_pwm.c
    )

    if (DEFINED CFG_TVOUT_ENABLE)
        target_sources(ith
        PRIVATE
            ith_tve.c
        )
    endif()

    if (DEFINED CFG_VIDEO_ENABLE)
        target_sources(ith
        PRIVATE
            #${PROJECT_SOURCE_DIR}/sdk/include/ith/ith_vregs.h
            ${PROJECT_SOURCE_DIR}/sdk/include/ith/ith_video.h
            ith_video.c
        )
    endif()

    if (DEFINED CFG_JPEG_HW_ENABLE)
        target_sources(ith
        PRIVATE
            ith_jpeg.c
        )
    endif()

    if (DEFINED CFG_UI_ENC_ENABLE)
        target_sources(ith
        PRIVATE
            ith_uiEnc.c
        )
    endif()

    if (DEFINED CFG_EFUSE_ENABLE)
        target_sources(ith
        PRIVATE
            ith_efuse.c
        )
    endif()

    if (DEFINED CFG_SD0_ENABLE OR DEFINED CFG_SD1_ENABLE OR DEFINED CFG_CF_ENABLE OR DEFINED CFG_MS_ENABLE OR DEFINED CFG_XD_ENABLE OR DEFINED CFG_NOR_ENABLE OR DEFINED CFG_NAND_ENABLE)
        target_sources(ith
        PRIVATE
            ith_card.c
        )
    endif()

    if (DEFINED CFG_USB0_ENABLE OR DEFINED CFG_USB1_ENABLE OR DEFINED CFG_USBHCC)
        target_sources(ith
        PRIVATE
            ith_usb.c
        )
    endif()

    if (DEFINED CFG_WATCHDOG_ENABLE)
        target_sources(ith
        PRIVATE
            ith_watchdog.c
        )
    endif()

    if (DEFINED CFG_DCPS_ENABLE)
        target_sources(ith
        PRIVATE
            ith_dcps.c
        )
    endif()

    if (DEFINED CFG_DPU_ENABLE)
        if (DEFINED CFG_NOR_USE_DPUAES)
            target_sources(ith
            PRIVATE
                ith_dpu2.c
            )
        else()
            target_sources(ith
            PRIVATE
                ith_dpu.c
            )
        endif()
    endif()

    if (DEFINED CFG_NAND_ENABLE)
        target_sources(ith
        PRIVATE
            ith_nand.c
        )
    endif()

    if (DEFINED CFG_XD_ENABLE)
        target_sources(ith
        PRIVATE
            ith_xd.c
        )
    endif()

    include_directories(
        ${PROJECT_SOURCE_DIR}/sdk/driver/ith
    )

    if (DEFINED CFG_IR0_ENABLE OR DEFINED CFG_IR1_ENABLE OR DEFINED CFG_IR2_ENABLE OR DEFINED CFG_IR3_ENABLE)
        target_sources(ith
        PRIVATE
            ith_ir.c
        )
    endif()

    if (DEFINED CFG_SARADC_ENABLE)
        target_sources(ith
        PRIVATE
            ith_saradc.c
        )
    endif()

    if (DEFINED CFG_WATCHDOG_ENABLE)
        add_definitions(
            -DCFG_WATCHDOG_ENABLE
        )
    endif()

    if (DEFINED CFG_MEM_BIST_ENABLE)
        target_sources(ith
        PRIVATE
            ith_membist.c
        )
    endif()

    if (DEFINED CFG_CPU_WB OR CFG_WT_SIZE STRGREATER 0 OR $ENV{CFG_PLATFORM} STREQUAL win32)
        add_library(ith STATIC
            ith_vmem.c
        )
    endif()

    if (DEFINED CFG_M2D_RESERVE_MEM_ENABLE)
        add_library(ith STATIC
            ith_vmem_m2d.c
        )
    endif()

    if ($ENV{CFG_PLATFORM} STREQUAL win32)

        execute_process(COMMAND mkscript ${PROJECT_SOURCE_DIR}/sdk/target/debug/${CFG_DBG_INIT_SCRIPT} ${CMAKE_CURRENT_BINARY_DIR}/ram.inc)

        target_sources(ith_platform
        PRIVATE
            ith_win32.c
        )

        if (DEFINED CFG_WIN32_SIMULATOR)
            target_sources(ith_platform
            PRIVATE
                ith_win32_sim.c
            )
        else()
            target_sources(ith_platform
            PRIVATE
                ith_win32_spi.c
            )
        endif()

    else()

        target_sources(ith_platform
        PRIVATE
            ith_openrtos.c
        )

    endif()
else()

    add_subdirectory(test)
endif()
