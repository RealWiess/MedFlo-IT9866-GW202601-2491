if (DEFINED CFG_CPU_FA626 AND $ENV{CFG_PLATFORM} STREQUAL openrtos)
    set(CMAKE_EXE_LINKER_FLAGS  "${EXTRA_FLAGS} -mcpu=fa626te ${MAP_FLAGS} -nostartfiles -T\"${LIBRARY_OUTPUT_PATH}/default.ld\" -L\"${LIBRARY_OUTPUT_PATH}\" \"${LIBRARY_OUTPUT_PATH}/tlb_wt.o\" \"${LIBRARY_OUTPUT_PATH}/startup.o\"")
endif()

ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_WATCHDOG_ENABLE)
ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_BL_ERR)
ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_BL_WARN)
ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_BL_INFO)
ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_BL_DBG)
ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_BL_LCD_CONSOLE)
ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_SPEEDY_DECOMPRESS_ENABLE)

if (DEFINED CFG_BOOT_TESTBIN_ENABLE)
    add_definitions(
        -DCFG_BOOT_TESTBIN_ENABLE
        -DCFG_TESTBIN_FILENAME="${CFG_TESTBIN_FILENAME}"
    )
endif()

if (DEFINED CFG_BL_DLM_ENABLE)
    add_definitions(
        -DCFG_BL_DLM_ENABLE
        -DCFG_UPGRADE_BL_DLM_PACKAGE_POS=${CFG_UPGRADE_BL_DLM_PACKAGE_POS}
    )
endif()

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/sdk/share/ffmpeg
    ${PROJECT_SOURCE_DIR}/sdk/driver/itv
    ${PROJECT_SOURCE_DIR}/sdk/share/speedy
)

if (CFG_CHIP_FAMILY STREQUAL 9850)
    include_directories(
        ${PROJECT_SOURCE_DIR}/sdk/share/itu
        ${PROJECT_SOURCE_DIR}/sdk/share/itu/it9850
    )
else()
    include_directories(
        ${PROJECT_SOURCE_DIR}/sdk/share/itu
        ${PROJECT_SOURCE_DIR}/sdk/share/itu/it9070
    )
endif()

add_executable(${CMAKE_PROJECT_NAME}
    main_$ENV{CFG_PLATFORM}.c
)

if (DEFINED CFG_SKIP_UPGRADE_CHECK_BOOT_TIME)
include_directories(
	${PROJECT_SOURCE_DIR}/sdk/include
)

add_executable(${CMAKE_PROJECT_NAME}
    bootloader.h
    boot.c
    config.h
    main_skip_upgrade_check.c
    upgrade.c
)
else()
add_executable(${CMAKE_PROJECT_NAME}
    bootloader.h
    boot.c
    config.h
    main_upgrade_check.c
    main_upgrade_api.h
    main_upgrade_api.c
    uart_upgrade.c
    upgrade.c
)
endif()

#if (DEFINED CFG_BUILD_ASR)
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
#endif()

if (DEFINED CFG_ENABLE_UART_CLI)
    include_directories(
        ${PROJECT_SOURCE_DIR}/sdk/share/cli/
    )
endif()

if (DEFINED CFG_OPENRTOS_CLI)
    include_directories(
        ${PROJECT_SOURCE_DIR}/sdk/share/cli/
    )
endif()

if ((DEFINED CFG_ENABLE_UART_CLI) AND (DEFINED CFG_NET_TFTP))
    add_executable(${CMAKE_PROJECT_NAME}
        recovery.c
    )
    include_directories(
        ${PROJECT_SOURCE_DIR}/sdk/share/wifi_mgr/
        ${PROJECT_SOURCE_DIR}/sdk/driver/wifi_rtk/non_gpl_wifi_8189f/include/
        ${PROJECT_SOURCE_DIR}/sdk/driver/wifi_rtk/non_gpl_wifi_8189f/api/wifi/
    )
endif()

if (DEFINED CFG_UPGRADE_BACKUP_PACKAGE)
    add_executable(${CMAKE_PROJECT_NAME}
        restore.c
    )
endif()

if (DEFINED CFG_GPIO_STORAGE_UPGRADE)
    add_definitions(
        -DCFG_GPIO_STORAGE_UPGRADE=${CFG_GPIO_STORAGE_UPGRADE}
    )
endif()

if (DEFINED CFG_GPIO_WIFI_UPGRADE)
    add_definitions(
        -DCFG_GPIO_WIFI_UPGRADE=${CFG_GPIO_WIFI_UPGRADE}
        -DCFG_APP_STA_SSID="${CFG_APP_STA_SSID}"
        -DCFG_APP_STA_PW="${CFG_APP_STA_PW}"
        -DCFG_UPGRADE_URL="${CFG_UPGRADE_URL}"
    )
endif()

if (DEFINED CFG_NET_WIFI)
    include_directories(
        ${CMAKE_CURRENT_BINARY_DIR}
            ${PROJECT_SOURCE_DIR}/sdk/share/wifi_mgr/
        #=== BCM Series ===#
        if (DEFINED CFG_NET_WIFI_SDIO_VND_MHD)
            ${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_mhd/include
            ${PROJECT_SOURCE_DIR}/sdk/include/lwip
        endif()
    )
endif()

# logo
if (DEFINED CFG_BL_SHOW_LOGO)
    add_executable(${CMAKE_PROJECT_NAME}
        logo.c
    )

    execute_process(COMMAND convert -define bmp:format=bmp3 -define bmp3:alpha=true ${PROJECT_SOURCE_DIR}/data/bitmap/${CFG_LCD_BOOT_BITMAP} ${CMAKE_CURRENT_BINARY_DIR}/logo.bmp)

    if (CFG_LCD_BPP STREQUAL "4")
        set(args --format=ARGB8888)
    else()
        set(args --format=RGB565)
    endif()

    if (DEFINED CFG_LCD_BOOT_BITMAP_DITHER)
        set(args ${args} --dither)
    endif()

    message("dataconv ${args}")
    execute_process(COMMAND dataconv ${args} -m ${CMAKE_CURRENT_BINARY_DIR}/logo.bmp -o ${CMAKE_CURRENT_BINARY_DIR}/logo.inc)
endif()

# recovery
if (DEFINED CFG_UPGRADE_RECOVERY)
    add_executable(${CMAKE_PROJECT_NAME}
        recovery.c
    )
endif()

# USB device mode
if (DEFINED CFG_UPGRADE_USB_DEVICE)
    add_executable(${CMAKE_PROJECT_NAME}
        usbdevice.c
    )
endif()

# check CRC
if (DEFINED CFG_BL_CHECK_CRC)
    add_executable(${CMAKE_PROJECT_NAME}
        check_crc.c
    )
endif()

# build
include(${PROJECT_SOURCE_DIR}/sdk/build.cmake)
