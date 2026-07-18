add_library(itp_boot)
add_library(itp)

target_link_libraries(itp_boot
PRIVATE
    ith_platform
)

if (DEFINED CFG_WATCHDOG_ENABLE)
    add_definitions(
        -DCFG_WATCHDOG_ENABLE
        -DCFG_WATCHDOG_TIMEOUT=${CFG_WATCHDOG_TIMEOUT}
        -DCFG_WATCHDOG_REFRESH_INTERVAL=${CFG_WATCHDOG_REFRESH_INTERVAL}
    )

    ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_WATCHDOG_INTR)
    ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_WATCHDOG_IDLETASK)
endif()

ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_ITP_ERR)
ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_ITP_WARN)
ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_ITP_INFO)
ITE_ADD_DEFINITIONS_IF_DEFINED(CFG_ITP_DBG)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/sdk/driver/usb/inc/
)

target_sources(itp_boot
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/ite/itp.h
    itp_backtrace.c
    itp_base.c
    itp_cfg.h
    itp_error.c
    itp_vmem.c
PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/itp_defer.c
)

if (DEFINED CFG_GCC_LTO)
    SET_TARGET_PROPERTIES(itp_boot PROPERTIES COMPILE_FLAGS "-fno-lto")
endif()

if (DEFINED CFG_DBG_RMALLOC)
    target_sources(itp_boot
    PRIVATE
        itp_rmalloc.c
    )
endif()

if (DEFINED CFG_DBG_PRINTBUF)
    target_sources(itp_boot
    PRIVATE
        itp_printbuf.c
    )
endif()

if (DEFINED CFG_LCD_ENABLE)
    target_sources(itp_boot
    PRIVATE
        itp_lcdconsole.c
    )
endif()

if (DEFINED CFG_LCD_ENABLE OR DEFINED CFG_TVOUT_ENABLE)
    target_sources(itp_boot
    PRIVATE
        itp_screen.c
    )
endif()

if (DEFINED CFG_BACKLIGHT_ENABLE)
    target_sources(itp_boot
    PRIVATE
        itp_backlight.c
    )
endif()

include_directories(
    ${PROJECT_SOURCE_DIR}/sdk/driver/itp
)

target_sources(itp
PRIVATE
    itp_uart_api.c
)

if ($ENV{CFG_PLATFORM} STREQUAL openrtos)
    if (DEFINED CFG_UART0_ENABLE OR DEFINED CFG_UART1_ENABLE OR DEFINED CFG_UART2_ENABLE OR DEFINED CFG_UART3_ENABLE OR DEFINED CFG_UART4_ENABLE OR DEFINED CFG_UART5_ENABLE)
        target_sources(itp
        PRIVATE
            itp_uart.c
        )
        target_link_libraries(itp PRIVATE uart)
    endif()

    if (DEFINED CFG_KEYPAD_ENABLE)
        target_sources(itp
        PRIVATE
            itp_keypad.c
            keypad/${CFG_KEYPAD_MODULE}
        )
        if (DEFINED CFG_SARADC_ENABLE)
            target_link_libraries(itp PRIVATE saradc)
        endif()
    endif()

    if (DEFINED CFG_IR0_ENABLE OR DEFINED CFG_IR1_ENABLE OR DEFINED CFG_IR2_ENABLE OR DEFINED CFG_IR3_ENABLE)
        target_sources(itp
        PRIVATE
            itp_ir.c
        )

        target_link_libraries(itp PRIVATE ir)
    endif()
endif()

if (DEFINED CFG_DBG_SWUART_CODEC OR DEFINED CFG_RS485_4_ENABLE OR DEFINED CFG_SWUART_CODEC_ENABLE)
    add_library(itp_boot STATIC
        itp_swuart_codec.c
        itp_swuart_dbg_codec.c
    )
endif()

if (DEFINED CFG_UART0_RS485 OR DEFINED CFG_UART1_RS485 OR DEFINED CFG_UART2_RS485 OR DEFINED CFG_UART3_RS485 OR DEFINED CFG_UART4_RS485 OR DEFINED CFG_UART5_RS485)
    target_sources(itp
    PRIVATE
        itp_rs485.c
    )
endif()

target_sources(itp
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/ite/itp.h
    itp_cfg.h
)

if (DEFINED CFG_WATCHDOG_ENABLE)
    target_sources(itp
    PRIVATE
        itp_watchdog.c
    )
endif()

if (DEFINED CFG_PMIC_WD_ENABLE)
    target_sources(itp
    PRIVATE
        itp_pmic.c
    )
endif()

if (DEFINED CFG_I2C0_ENABLE OR CFG_I2C1_ENABLE OR CFG_I2C2_ENABLE OR CFG_I2C3_ENABLE)
    target_sources(itp
    PRIVATE
        itp_i2c.c
    )

    target_link_libraries(itp PRIVATE iic)
endif()

if (DEFINED CFG_SPI_ENABLE)
    target_sources(itp
    PRIVATE
        itp_spi.c
    )
    target_link_libraries(itp PRIVATE spi)
endif()

if (DEFINED CFG_DCPS_ENABLE)
    target_sources(itp
    PRIVATE
        itp_decompress.c
    )
endif()

if (DEFINED CFG_DPU_ENABLE)
    if (DEFINED CFG_NOR_USE_DPUAES)
        target_sources(itp
        PRIVATE
            itp_dpu2.c
        )
    else()
        target_sources(itp
        PRIVATE
            itp_dpu.c
        )
    endif()
endif()

if (DEFINED CFG_TOUCH_ENABLE)
    target_sources(itp
    PRIVATE
        itp_tp.c
    )
endif()

if (DEFINED CFG_BATTERY_ENABLE)
    target_sources(itp
    PRIVATE
        battery/${CFG_BATTERY_MODULE}
    )
endif()

if (DEFINED CFG_BATTERY_ENABLE OR DEFINED CFG_EXTERNAL_POWER_ON OR DEFINED CFG_POWER_STANDBY OR DEFINED CFG_POWER_SLEEP OR DEFINED CFG_POWER_HIBERNATION OR DEFINED CFG_POWER_SUSPEND)
    target_sources(itp
    PRIVATE
        itp_power.c
    )
endif()

if (DEFINED CFG_GSENSOR_ENABLE)
    target_sources(itp
    PRIVATE
        itp_gsensor.c
        gsensor/${CFG_GSENSOR_MODULE}
    )
endif()

if (DEFINED CFG_MEM_BIST_ENABLE)
    target_sources(itp
    PRIVATE
        itp_membist.c
    )
endif()

if (DEFINED CFG_HEADSET_ENABLE)
    target_sources(itp
    PRIVATE
        itp_headset.c
        headset/${CFG_HEADSET_MODULE}
    )
endif()

if (DEFINED CFG_AMPLIFIER_ENABLE)
    target_sources(itp
    PRIVATE
        itp_amplifier.c
        amplifier/${CFG_AMPLIFIER_MODULE}
    )
endif()

if (DEFINED CFG_LED_ENABLE)
    target_sources(itp
    PRIVATE
        itp_led.c
    )
endif()

if (DEFINED CFG_SWITCH_ENABLE)
    target_sources(itp
    PRIVATE
        itp_switch.c
    )
endif()

if (DEFINED CFG_TUNER_ENABLE)
    target_sources(itp
    PRIVATE
        itp_tuner.c
    )
endif()

if (DEFINED CFG_WIEGAND0_BIT_CUSTOMIZE)
    target_sources(itp
    PRIVATE
        ${CFG_WIEGAND0_CUSTOMIZE_MODULE}
    )
elseif (DEFINED CFG_WIEGAND1_BIT_CUSTOMIZE)
    target_sources(itp
    PRIVATE
        ${CFG_WIEGAND1_CUSTOMIZE_MODULE}
    )
elseif (DEFINED CFG_WIEGAND0_ENABLE OR DEFINED CFG_WIEGAND1_ENABLE)
    target_sources(itp
    PRIVATE
        itp_wiegand.c
        itp_wiegandout.c
    )

    target_link_libraries(itp PRIVATE wiegand)
endif()

target_sources(itp
PRIVATE
    itp_ctrlboard_codec.c
)

if (DEFINED CFG_STNLCD_ENABLE)
    target_sources(itp
    PRIVATE
        itp_stnlcd.c
        stnlcd/${CFG_STNLCD_MODULE}
    )
endif()

if (DEFINED CFG_RTC_ENABLE)
    if (DEFINED CFG_RTC_REDUCE_IO_ACCESS_ENABLE)
        target_sources(itp
        PRIVATE
            rtc/itp_rtc_assist.c
        )
    endif()

    target_sources(itp
    PRIVATE
        itp_rtc.c
        rtc/${CFG_RTC_MODULE}
        rtc/itp_rtc_sw_switch.c
    )
endif()

if (DEFINED CFG_RGBTOMIPI_ENABLE)
    target_sources(itp
    PRIVATE
        itp_rgbtomipi.c
        rgbtomipi/${CFG_RGBTOMIPI_MODULE}
    )
endif()

if (DEFINED CFG_NAND_ENABLE)
    target_sources(itp
    PRIVATE
        itp_nand.c
    )

    if (DEFINED CFG_PPI_NAND)
        target_link_libraries(itp PRIVATE nand)
    endif()
    if (DEFINED CFG_SPI_NAND)
        target_link_libraries(itp PRIVATE spi_nand)
    endif()
    if (DEFINED CFG_FS_FAT)
        if (CFG_NAND_PAGE_SIZE EQUAL 2048)
            target_link_libraries(itp PRIVATE fat_nand2k)
        endif()
        if (CFG_NAND_PAGE_SIZE EQUAL 4096)
            target_link_libraries(itp PRIVATE fat_nand4k)
        endif()
    endif()
endif()

if (DEFINED CFG_NOR_ENABLE)
    if (DEFINED CFG_NOR_USE_DPUAES)
        target_sources(itp
        PRIVATE
            itp_nor.c
            itp_nor_aes.c
        )
    else()
        target_sources(itp
        PRIVATE
            itp_nor.c
        )
    endif()

    target_link_libraries(itp PRIVATE nor)
    if (DEFINED CFG_FS_FAT)
        target_link_libraries(itp PRIVATE fat_nor)
    endif()
endif()

if (DEFINED CFG_XD_ENABLE)
    target_sources(itp
    PRIVATE
        itp_xd.c
    )
endif()

if (DEFINED CFG_USBHCC)
    include_directories(
        ${PROJECT_SOURCE_DIR}/openrtos/include/openrtos/
    )

    target_sources(itp
    PRIVATE
        usbhcc/itp_usb.c
        usbhcc/itp_usbh.c
    )

    target_link_libraries(itp PRIVATE usbhcc)

    add_library(itp_usbhcc)
    target_sources(itp_usbhcc
    PRIVATE
        usbhcc/itp_usbd_mst_ext.c
        usbhcc/itp_usbd_ext.c
        )
endif()

if (DEFINED CFG_USBH_CD_HUB)
    target_sources(itp
    PRIVATE
        usbhcc/itp_usbh_hub.c
    )
endif()

if (DEFINED CFG_USBH_CD_CCID)
    target_sources(itp
    PRIVATE
        usbhcc/itp_usbh_ccid.c
    )
endif()

if (DEFINED CFG_USBH_CD_MST)
    target_sources(itp
    PRIVATE
        usbhcc/itp_usbh_mst.c
    )
endif()

if (DEFINED CFG_USBH_CD_HID)
    target_sources(itp
    PRIVATE
        usbhcc/itp_usbh_hid.c
    )
endif()

if (DEFINED CFG_USBH_CD_MOUSE_ITU)
    target_sources(itp
    PRIVATE
        usbhcc/itp_usbh_mouse.c
    )
endif()

if (DEFINED CFG_USBH_CD_CDCECM_HCC)
    target_sources(itp
    PRIVATE
        usbhcc/itp_usbh_cdcecm.c
    )
endif()

if (DEFINED CFG_USBH_CD_CDCACM)
    target_sources(itp
    PRIVATE
        usbhcc/itp_usbh_cdcacm.c
    )
endif()

if (DEFINED CFG_USBH_CD_BULK_RAW)
    target_sources(itp
    PRIVATE
        usbhcc/itp_usbh_raw.c
    )
endif()

if (DEFINED CFG_USBH_CD_RNDIS)
    target_sources(itp
    PRIVATE
        usbhcc/itp_usbh_rndis.c
    )
endif()

if (DEFINED CFG_USBHCC_DEVICE)
    target_sources(itp
    PRIVATE
        usbhcc/itp_usbd.c
    )
endif()

if (DEFINED CFG_USBD_CD_CDCACM)
    target_sources(itp
    PRIVATE
        usbhcc/itp_usbd_cdcacm.c
    )
endif()

if (DEFINED CFG_USBD_CD_CDCNCM)
    target_sources(itp
    PRIVATE
        usbhcc/itp_usbd_cdcncm.c
    )
endif()

if (DEFINED CFG_USBD_CD_CDCECM)
    target_sources(itp
    PRIVATE
        usbhcc/itp_usbd_cdcecm.c
    )
endif()

if (DEFINED CFG_USBD_CD_MST)
    target_sources(itp
    PRIVATE
        usbhcc/itp_usbd_mst.c
    )
endif()

if (DEFINED CFG_USBD_CD_BULK_RAW)
    target_sources(itp
    PRIVATE
        usbhcc/itp_usbd_raw.c
    )
endif()

if (DEFINED CFG_USBD_CD_HID)
    target_sources(itp
    PRIVATE
        usbhcc/itp_usbd_hid.c
    )
endif()

if (DEFINED CFG_USBD_CD_CDCACM_RAW)
    target_sources(itp
    PRIVATE
        usbhcc/itp_usbd_cdcacm.c
        usbhcc/itp_usbd_raw.c
    )
endif()

if (DEFINED CFG_USB0_ENABLE OR DEFINED CFG_USB1_ENABLE)
    target_sources(itp
    PRIVATE
        itp_usb.c
    )

    if (DEFINED CFG_BUILD_USB)
        target_link_libraries(itp PRIVATE usb)
    endif()
endif()

if (DEFINED CFG_USB_OPTION)
    target_sources(itp
    PRIVATE
        itp_usb_option.c
    )
endif()

if (DEFINED CFG_USB_DEVICE)
    target_sources(itp
    PRIVATE
        itp_usb_device.c
    )
endif()

if (DEFINED CFG_USB_HID)
    target_sources(itp
    PRIVATE
        itp_usb_hid.c
    )
else()
    if (DEFINED CFG_USB_MOUSE)
        target_sources(itp
        PRIVATE
            itp_usb_mouse.c
        )
    endif()

    if (DEFINED CFG_USB_KBD)
        target_sources(itp
        PRIVATE
            itp_usb_kbd.c
        )
    endif()
endif()

if (DEFINED CFG_USBD_MASS_STORAGE)
    target_sources(itp
    PRIVATE
        itp_usbd_fsg.c
    )
endif()

if (DEFINED CFG_USBD_ACM)
    target_sources(itp
    PRIVATE
        itp_usbd_acm.c
    )
endif()

if (DEFINED CFG_USBD_HID)
    target_sources(itp
    PRIVATE
        itp_usbd_hid.c
    )
endif()

if (DEFINED CFG_SD0_ENABLE OR DEFINED CFG_SD1_ENABLE)
    target_sources(itp
    PRIVATE
        itp_sd.c
    )

    target_link_libraries(itp PRIVATE sd)
    if (DEFINED CFG_FS_FAT)
        target_link_libraries(itp PRIVATE fat_sd)
    endif()
endif()

if (DEFINED CFG_SDIO_ENABLE)
    target_sources(itp
    PRIVATE
        itp_sdio.c
    )
endif()

if (DEFINED CFG_FS_FAT)
    if(DEFINED CFG_BUILD_FILEX)
        include_directories(
            ${PROJECT_SOURCE_DIR}/sdk/share/filex/common/inc
        )
        if(DEFINED CFG_BUILD_LEVELX)
            include_directories(${PROJECT_SOURCE_DIR}/sdk/share/levelx/common/inc)
        endif()
        target_sources(itp
        PRIVATE
            itp_filex.c
        )        
    	target_link_libraries(itp PRIVATE filex fat)
        if(DEFINED CFG_BUILD_LEVELX)
            target_link_libraries(itp PRIVATE levelx)
        endif()
    else()
        target_sources(itp
        PRIVATE
            itp_fat.c
        )
        target_link_libraries(itp PRIVATE fat)
    endif()
endif()

if (DEFINED CFG_FS_YAFFS2)
    include_directories(
        ${PROJECT_SOURCE_DIR}/sdk/share/yaffs2
    )
    target_sources(itp
    PRIVATE
        itp_yaffs2.c
    )

    target_link_libraries(itp PRIVATE yaffs2)
endif()

if (DEFINED CFG_FS_LFS)
    target_sources(itp
    PRIVATE
        itp_lfs.c
    )

    target_link_libraries(itp PRIVATE littlefs)
endif()

if (DEFINED CFG_FS_FAT OR DEFINED CFG_FS_LFS OR DEFINED CFG_FS_YAFFS2 OR $ENV{CFG_PLATFORM} STREQUAL win32)
    target_sources(itp
    PRIVATE
        itp_logdisk.c
    )
endif()

if (DEFINED CFG_AUDIO_ENABLE)
    target_sources(itp
    PRIVATE
        itp_codec.c
        dac/${CFG_AUDIO_DAC_MODULE}
    )
else()
    target_sources(itp
    PRIVATE
        itp_codec.c
        dac/itp_dac_null.c
    )
endif()

if (DEFINED CFG_VIDEO_ENABLE)
    if ($ENV{CFG_PLATFORM} STREQUAL openrtos)
        target_sources(itp
        PRIVATE
            it9860/itp_stc.c
        )
    elseif ($ENV{CFG_PLATFORM} STREQUAL win32)
        target_sources(itp
        PRIVATE
            itp_stc_win32.c
        )
    endif()
endif()

if (DEFINED CFG_NET_ENABLE)
    target_sources(itp
    PRIVATE
        itp_pipe.c
        itp_socket.c
    )

    if (DEFINED CFG_BUILD_LWIP)
        target_link_libraries(itp PRIVATE lwip)
    endif()

    if (DEFINED CFG_BUILD_LWIP_2)
        target_link_libraries(itp PRIVATE lwip_2)
    endif()
endif()

if (DEFINED CFG_NET_ETHERNET OR DEFINED CFG_NET_AMEBA_SDIO OR DEFINED CFG_USB_ECM OR DEFINED CFG_USBH_CD_CDCECM)
    target_sources(itp
    PRIVATE
        itp_ethernet.c
    )

    if (DEFINED CFG_BUILD_MAC)
        target_link_libraries(itp PRIVATE mac)
    endif()

    if (DEFINED CFG_BUILD_LWIP)
        target_link_libraries(itp PRIVATE lwip)
    endif()

    if (DEFINED CFG_BUILD_LWIP_2)
        target_link_libraries(itp PRIVATE lwip_2)
    endif()
endif()

if (DEFINED CFG_USB_ECM_EX OR DEFINED CFG_USBH_CD_CDCECM_EX)
    target_sources(itp
    PRIVATE
        itp_ecm.c
    )

    if (DEFINED CFG_BUILD_ECM)
        target_link_libraries(itp PRIVATE ecm)
    endif()

    if (DEFINED CFG_BUILD_LINUX)
        target_link_libraries(itp PRIVATE linux)
    endif()
endif()

if (DEFINED CFG_USBD_ECM OR DEFINED CFG_USBD_CD_CDCECM)
    target_sources(itp
    PRIVATE
        itp_usbd_ecm.c
    )

    if (DEFINED CFG_BUILD_LWIP)
        target_link_libraries(itp PRIVATE lwip)
    endif()

    if (DEFINED CFG_BUILD_LWIP_2)
        target_link_libraries(itp PRIVATE lwip_2)
    endif()
endif()

if (DEFINED CFG_USBD_NCM OR DEFINED CFG_USBD_CD_CDCNCM)
    target_sources(itp
    PRIVATE
        itp_usbd_ncm.c
    )

    if (DEFINED CFG_BUILD_LWIP)
        target_link_libraries(itp PRIVATE lwip)
    endif()

    if (DEFINED CFG_BUILD_LWIP_2)
        target_link_libraries(itp PRIVATE lwip_2)
    endif()
endif()

if (DEFINED CFG_NET_RTL8201FL)
    target_sources(itp
    PRIVATE
        phy/phy_rtl8201fl.c
    )
endif()

if (DEFINED CFG_NET_IP101GR)
    target_sources(itp
    PRIVATE
        phy/phy_ip101gr.c
    )
endif()

if (DEFINED CFG_NET_DM9162)
    target_sources(itp
    PRIVATE
        phy/phy_dm9162.c
    )
endif()

if (DEFINED CFG_NET_RTL8201_COMPATIBLE)
    target_sources(itp
    PRIVATE
        phy/phy_rtl8201_compatible.c
    )
endif()

if (DEFINED CFG_NET_IP175LLF)
    target_sources(itp
    PRIVATE
        phy/ip175llf.c
    )
endif()

if (DEFINED CFG_NET_LAN8720)
    target_sources(itp
    PRIVATE
        phy/phy_lan8720.c
    )
endif()

if (DEFINED CFG_NET_TJA1101)
    target_sources(itp
    PRIVATE
        phy/phy_TJA1101.c
    )
endif()

if (DEFINED CFG_NET_RTL8211F)
    target_sources(itp
    PRIVATE
        phy/phy_rtl8211f.c
    )
endif()

if (DEFINED CFG_NET_RTL8304MB)
    target_sources(itp
    PRIVATE
        phy/rtl8304mb.c
    )

    target_link_libraries(itp PRIVATE rtl8304mb)
endif()

if (DEFINED CFG_NET_RTL8363NB)
    target_sources(itp
    PRIVATE
        phy/rtl8363nb.c
    )
    target_link_libraries_ifdef(CFG_BUILD_RTL8363NB itp PRIVATE rtl8363nb)
endif()

if (DEFINED CFG_NET_WIFI)
    if (DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6256 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6236 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6212 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6203 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_SYN4345x OR DEFINED CFG_NET_WIFI_SDIO_NGPL_SYN4343x)
        target_sources(itp
        PRIVATE
            itp_wifi_mhd.c
        )
    elseif (DEFINED CFG_NET_WIFI_SDIO_NGPL_ATBM6031)
        target_sources(itp
        PRIVATE
            itp_wifi_atbm.c
        )
    elseif(DEFINED CFG_NET_WIFI_SDIO_NGPL_AIC8800)
		add_definitions(-Wno-error)  	
        target_sources(itp
        PRIVATE
            itp_wifi_aic.c
        )
    elseif(DEFINED CFG_NET_WIFI_SDIO_NGPL_MT7921)
    	include_directories(
			${PROJECT_SOURCE_DIR}/sdk/driver/wifi_mtk/gen4m/os/freertos/include
			${PROJECT_SOURCE_DIR}/sdk/driver/wifi_mtk
    	)
    	target_sources(itp
    	PRIVATE
 				itp_wifi_mtk.c
    )
if (DEFINED CFG_NET_LWIP)
	target_link_libraries(itp PUBLIC ${PROJECT_SOURCE_DIR}/sdk/driver/wifi_mtk/wlan_lib/libAwm6721S_Lwip_1.4.1_LTO.a)
endif()
if (DEFINED CFG_NET_LWIP_2)
	target_link_libraries(itp PUBLIC ${PROJECT_SOURCE_DIR}/sdk/driver/wifi_mtk/wlan_lib/libAwm6721S_Lwip_2.2.0_LTO.a)
endif()
	else()
        target_sources(itp
        PRIVATE
            itp_wifi.c
        )
    endif()

    if (DEFINED CFG_BUILD_LWIP)
        target_link_libraries(itp PRIVATE lwip)
    endif()

    if (DEFINED CFG_BUILD_LWIP_2)
        target_link_libraries(itp PRIVATE lwip_2)
    endif()
endif()

if (DEFINED CFG_MT7921BLE)
    include_directories(
    ${PROJECT_SOURCE_DIR}/sdk/driver/bluetooth
    ${PROJECT_SOURCE_DIR}/sdk/driver/bluetooth/mtk/src/include/
    ${PROJECT_SOURCE_DIR}/sdk/driver/bluetooth/mtk/src/include/sdio/
    ${PROJECT_SOURCE_DIR}/sdk/include/bluetooth/mt7921ble/
    ${PROJECT_SOURCE_DIR}/sdk/driver/bluetooth/mtk/src/fwpatches/MT7921/
    ${PROJECT_SOURCE_DIR}/sdk/driver/bluetooth/mtk/src/*.c
    )
    target_link_libraries(itp PRIVATE bluetooth)
endif()

if (DEFINED CFG_DBG_NETCONSOLE)
    target_sources(itp
    PRIVATE
        itp_netconsole.c
    )
endif()

if (DEFINED CFG_DBG_STATS)
    target_sources(itp
    PRIVATE
        itp_stats.c
    )
endif()

if (DEFINED CFG_DEMOD_ENABLE)
    target_sources(itp
    PRIVATE
        itp_demod.c
        itp_demod_thread.c
    )
elseif(DEFINED CFG_DEMOD_ISDB_ENABLE)
    target_sources(itp
    PRIVATE
        itp_demod_isdb.c
        itp_demod_thread.c
    )
endif()

if (DEFINED CFG_ARMLITE_ENABLE)
    target_link_libraries(itp_boot
    PRIVATE
        risc
    )
    if (DEFINED CFG_ARMLITE_HUD_BG_FAST AND NOT $ENV{CFG_PLATFORM} STREQUAL win32)
        target_link_libraries(itp_boot
        PRIVATE
            hudBgFast
        )
    endif()
    if (DEFINED CFG_ARMLITE_CANBUS AND NOT $ENV{CFG_PLATFORM} STREQUAL win32)
        target_link_libraries(itp_boot
        PRIVATE
            can_bus_lite
        )
    endif()	
    if (DEFINED CFG_ARMLITE_LCD_CRC_CAL AND NOT $ENV{CFG_PLATFORM} STREQUAL win32)
        target_link_libraries(itp_boot
        PRIVATE
            lcdCrcCal
        )
    endif()	
    target_sources(itp_boot
    PRIVATE
        itp_armlite_dev.c
    )
endif()

if (DEFINED CFG_RISCV2_ENABLE)
    target_sources(itp_boot
    PRIVATE
        itp_riscv2.c
    )
endif()

if (DEFINED CFG_ALT_CPU_ENABLE)
    target_link_libraries(itp_boot
    PRIVATE
        risc
    )
    if (DEFINED CFG_SW_UART AND NOT $ENV{CFG_PLATFORM} STREQUAL win32)
        target_link_libraries(itp_boot
        PRIVATE
            swUart
        )
    endif()
    if (DEFINED CFG_SW_UART_LIN AND NOT $ENV{CFG_PLATFORM} STREQUAL win32)
        target_link_libraries(itp_boot
        PRIVATE
            swUartLIN
        )
    endif()

    target_sources(itp_boot
    PRIVATE
        itp_alt_cpu.c
    )
endif()

if ($ENV{CFG_PLATFORM} STREQUAL openrtos)
    target_sources(itp_boot
    PRIVATE
        itp_posix_openrtos.c
    )

    target_compile_options(itp_boot
    PUBLIC
        "-Wl,-u basename"
    )

    target_sources(itp
    PRIVATE
        itp_init_openrtos.c
    )

    if (DEFINED CFG_BUILD_RISC)
        target_link_libraries(itp
        PRIVATE
            risc
        )
    endif()

    if (DEFINED CFG_SD0_ENABLE OR DEFINED CFG_SD1_ENABLE OR DEFINED CFG_RAMDISK_ENABLE OR DEFINED CFG_MSC_ENABLE OR DEFINED CFG_USBH_CD_MST)
        target_sources(itp
        PRIVATE
            itp_card.c
        )
    endif()

    if (DEFINED CFG_FS_FAT OR DEFINED CFG_FS_LFS OR DEFINED CFG_FS_YAFFS2)
        target_sources(itp
        PRIVATE
            itp_drive_openrtos.c
        )
    endif()

    if (DEFINED CFG_DBG_TRACE_ANALYZER AND DEFINED CFG_DBG_VCD)
        target_sources(itp_boot
        PRIVATE
            itp_stats_taskvcd.c
        )
    endif()

elseif ($ENV{CFG_PLATFORM} STREQUAL win32)
    target_sources(itp_boot
    PRIVATE
        itp_file.c
        itp_posix_win32.c
    )

    target_sources(itp
    PRIVATE
        itp_drive_win32.c
        itp_init_win32.c
    )
endif()

if (DEFINED CFG_LCD_ENABLE)

    set(CFG_LCDA_ADDR "0x0")

    if (DEFINED CFG_LCD_MULTIPLE)

        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/lcd.inc "")

        foreach (script ${CFG_LCD_INIT_SCRIPTS})
            execute_process(COMMAND mkscript ${PROJECT_SOURCE_DIR}/sdk/target/lcd/${script}.txt ${CMAKE_CURRENT_BINARY_DIR}/${script}.inc)

            file(READ ${CMAKE_CURRENT_BINARY_DIR}/${script}.inc array)

            string(REPLACE "." "" var ${script})
            file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/lcd.inc "static const LCD_SCRIPT_TYPE ${var}[] = {\n${array}};\n")

        endforeach()

        file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/lcd.inc "static const LCD_SCRIPT_TYPE* lcdScript[] =\n{\n")

        foreach (script ${CFG_LCD_INIT_SCRIPTS})
            string(REPLACE "." "" var ${script})
            file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/lcd.inc "${var},\n")

        endforeach()

        file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/lcd.inc "};\n")

        file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/lcd.inc "static const int lcdScriptSize[] =\n{\n")

        foreach (script ${CFG_LCD_INIT_SCRIPTS})
            string(REPLACE "." "" var ${script})
            file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/lcd.inc "ITH_COUNT_OF(${var}),\n")

        endforeach()

        file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/lcd.inc "};\n")

    else()
        configure_file(${PROJECT_SOURCE_DIR}/sdk/target/lcd/${CFG_LCD_INIT_SCRIPT} ${CMAKE_CURRENT_BINARY_DIR}/lcd.txt)

        execute_process(COMMAND mkscript ${CMAKE_CURRENT_BINARY_DIR}/lcd.txt ${CMAKE_CURRENT_BINARY_DIR}/lcd.inc)
    endif()

endif()

if (DEFINED CFG_TVOUT_ENABLE)
    configure_file(${PROJECT_SOURCE_DIR}/sdk/target/tvout/tv_ntsc.inc ${CMAKE_CURRENT_BINARY_DIR}/tv_ntsc.inc)
    configure_file(${PROJECT_SOURCE_DIR}/sdk/target/tvout/tv_pal.inc ${CMAKE_CURRENT_BINARY_DIR}/tv_pal.inc)
endif()
