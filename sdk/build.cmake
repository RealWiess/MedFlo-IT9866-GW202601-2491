add_library(${CMAKE_PROJECT_NAME}_interface INTERFACE)

ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(aap)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(protobuf)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(memtester)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(gcov)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(ocpp)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(onvif)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(qrdecode)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(qrencode)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(dtmf_dec)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(pillowtalk)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(httpserver)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(microhttpd)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(lwip_ftpd)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(tre)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(yajl)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(onvif_nvt)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(airplay_nmt)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(shairport)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(shairport_dacp)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(ushare)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(qtmultimedia)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(qtcharts)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(qtvirtualkeyboard)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(qtsvg)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(qtquickcontrols)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(qtdeclarative)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(qtbase)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(cups)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(mad)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(leaf)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(linphone)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(mpegencoder)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(uds_server)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(uds_interface)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(can_hal)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(can_bus)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(can_transceiver)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(irdecode)
if (DEFINED CFG_BUILD_LIVE555)
    if (DEFINED CFG_IPTV_TX)
        target_link_libraries(${CMAKE_PROJECT_NAME}_interface
        INTERFACE
            live555MediaServer
        )
    elseif(DEFINED CFG_BUILD_MIRACAST)
        target_link_libraries(${CMAKE_PROJECT_NAME}_interface
        INTERFACE
            live555WFD
            live555MediaServer
        )
    else()
        target_link_libraries(${CMAKE_PROJECT_NAME}_interface
        INTERFACE
            live555MediaClient
        )
    endif()
endif()
ITE_LINK_LIBRARY_IF_DEFINED(CFG_BUILD_MEDIASTREAMER2 mediastreamer)
ITE_LINK_LIBRARY_IF_DEFINED(CFG_BUILD_AUDIO_PREPROCESS eigens voipdsp)
ITE_LINK_LIBRARY_IF_DEFINED(CFG_BUILD_WFSTASR chains wfstasr voipdsp)

ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(sqlite3)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(ogg)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(vorbisdec)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(opus)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(spandsp726)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(bcg729)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(matroska)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(ebml)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(corec)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(agg)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(boost)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(sdl_ttf)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(sdl_image)
ITE_LINK_LIBRARY_IF_DEFINED(CFG_BUILD_SDL sdl_main sdl)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(iniparser)
ITE_LINK_LIBRARY_IF_DEFINED(CFG_BUILD_ITU itu_private itu)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(itu_renderer)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(awtk)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(lvgl)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(rlottie)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(pyinput)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(fontconfig)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(freetype)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(brotli)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(xml2)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(giflib)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(harfbuzz)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(png)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(jpeg)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(speex)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(wifi_mgr)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(ns)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(wfstasr)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(ucl)


ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(upnp)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(airplaylib)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(wac)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(dns_sd)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(air_play)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(ping)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(json)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(sntp)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(sntp_server)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(dhcps)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(siproxd)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(eXosip2)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(osip2)
ITE_LINK_LIBRARY_IF_DEFINED(CFG_BUILD_OPENSSL ssl crypto)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(stund)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(ortp)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(nghttp2)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(curl)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(eth_mgr)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(redblack)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(xlsxwriter)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(zlib)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(tslib)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(mbedtls)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(secure_boot)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(secure_debug)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(otp_check)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(sig_verify)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(pkg_verify)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(blueware)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(audio_mgr)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(flower)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(asr)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(mp3dec)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(ffmpeg)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(dewarp)
if (DEFINED CFG_BUILD_MIRACAST)
    target_link_libraries(${CMAKE_PROJECT_NAME}_interface
    INTERFACE
        ffmpegWFD
    )
endif()
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(aacdec)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(avc_decoder)
ITE_LINK_LIBRARY_IF_DEFINED(CFG_MJPEG_DEC_ENABLE mjpeg)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(itv)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(audio)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(m2d)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(it2d)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(itdcps)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(itdpu)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(itjpeg)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(itvp)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(jpg)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(jpegscale)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(isp)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(async_file)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(uiEnc)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(camera)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(i2s)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(cli)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(upgrade)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(itc)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(itp)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(saradc)

if (DEFINED CFG_BUILD_NAND)
    ITE_LINK_LIBRARY_IF_DEFINED(CFG_SPI_NAND spi_nand)
    ITE_LINK_LIBRARY_IF_DEFINED(CFG_PPI_NAND nand)
endif()

ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(xd)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(iic)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(iic_sw)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(pcf8575)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(wiegand)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(nor)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(axispi)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(spi)



if (DEFINED CFG_BL_DLM_ENABLE)
    ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(non_gpl_wifi_mhd)
else()
    if (NOT("${CMAKE_PROJECT_NAME}" STREQUAL "bootloader"))

        ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(non_gpl_wifi_mhd)
        ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(non_gpl_wifi_atbm)
		ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(non_gpl_wifi_aic8800)
          if (DEFINED CFG_NET_WIFI_SDIO_NGPL_ATBM6031)

        endif()
    endif()
endif()

ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(lwip)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(lwip_2)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(rtl8304mb)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(rtl8363nb)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(mac)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(wifi_mtk)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(non_gpl_wifi_8189f)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(non_gpl_wifi_8723d)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(non_gpl_wifi_8821c)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(non_gpl_wifi_8733b)

ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(speedy)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(ucl)

if (DEFINED CFG_BUILD_FAT)
    if (NOT DEFINED CFG_BUILD_FAT)
    target_link_libraries(${CMAKE_PROJECT_NAME}_interface
    INTERFACE
        fat
    )
    endif()

    if (DEFINED CFG_SD0_ENABLE OR DEFINED CFG_SD1_ENABLE)
        target_link_libraries(${CMAKE_PROJECT_NAME}_interface
        INTERFACE
            fat_sd
        )
    endif()

    ITE_LINK_LIBRARY_IF_DEFINED(CFG_MS_ENABLE fat_mspro)
    ITE_LINK_LIBRARY_IF_DEFINED(CFG_MSC_ENABLE fat_msc)

    if (DEFINED CFG_BUILD_NAND)
        if (CFG_NAND_PAGE_SIZE EQUAL 2048)
            target_link_libraries(${CMAKE_PROJECT_NAME}_interface
            INTERFACE
                fat_nand2k
            )
        endif()

        if (CFG_NAND_PAGE_SIZE EQUAL 4096)
            target_link_libraries(${CMAKE_PROJECT_NAME}_interface
            INTERFACE
                fat_nand4k
            )
        endif()
    endif()

    ITE_LINK_LIBRARY_IF_DEFINED(CFG_BUILD_XD fat_xd)
    ITE_LINK_LIBRARY_IF_DEFINED(CFG_BUILD_NOR fat_nor)
    ITE_LINK_LIBRARY_IF_DEFINED(CFG_RAMDISK_ENABLE fat_ramdisk)
endif()

ITE_LINK_LIBRARY_IF_DEFINED(CFG_BUILD_LITTLEFS littlefs)
ITE_LINK_LIBRARY_IF_DEFINED(CFG_BUILD_YAFFS2 yaffs2)

ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(uvc)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(esp32_sdio_at)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(ameba_sdio)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(sd)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(ecm)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(msc)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(uas)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(hid)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(usb)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(usbhcc)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(usblp)

#temp solution
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(hdmirx)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(hdmitx)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(video_encoder)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(capture)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(vp)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(sensor)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(encoder)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(async_file)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(iot)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(filex)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(levelx)

# Ts Module
ITE_LINK_LIBRARY_IF_DEFINED(CFG_IPTV_TX ts_airfile)

if (DEFINED CFG_PURE_TS_STREAM)
    target_link_libraries(${CMAKE_PROJECT_NAME}_interface
    INTERFACE
        tsi
    )
elseif (DEFINED CFG_ISDB_STANDARDS)
    target_link_libraries(${CMAKE_PROJECT_NAME}_interface
    INTERFACE
        demod_ctrl
        tsi
    )
elseif (DEFINED CFG_DVB_STANDARDS)

    target_link_libraries(${CMAKE_PROJECT_NAME}_interface
    INTERFACE
        ts_airfile
        demod_ctrl
        tsi
    )

    if (NOT DEFINED CFG_DEMOD_NONE)
        target_link_libraries(${CMAKE_PROJECT_NAME}_interface
        INTERFACE
            demod
        )
    endif()
endif()


ITE_LINK_LIBRARY_IF_DEFINED(CFG_BTA_ENABLE bluetooth_btapp)

# return channel option
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(ir)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(nedmalloc)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(linux)
ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(stl)

if (DEFINED CFG_RAMDISK_ENABLE)
    execute_process(COMMAND makedisk -p ${CMAKE_BINARY_DIR}/data/temp -o ${CMAKE_BINARY_DIR}/ramdisk.img -s ${CFG_RAMDISK_SIZE})
    execute_process(COMMAND dataconv -x ${CMAKE_BINARY_DIR}/ramdisk.img -o ${CMAKE_BINARY_DIR}/ramdisk.inc)
endif()

if (DEFINED CFG_BUILD_BLUETOOTH)
    if (DEFINED CFG_BUILD_NIMBLE)
        target_link_libraries(${CMAKE_PROJECT_NAME}_interface
        INTERFACE
            nimble
        )
        if (DEFINED CFG_RTL8723BLE OR DEFINED CFG_RTL8733BLE OR DEFINED CFG_RTL8821BLE
         OR DEFINED CFG_AP6212BLE OR DEFINED CFG_AP6236BLE OR DEFINED CFG_AP6256BLE OR DEFINED CFG_AP6203BLE)
            target_link_libraries(${CMAKE_PROJECT_NAME}_interface
            INTERFACE
                bluetooth
            )
        endif()
    elseif (DEFINED CFG_BUILD_BLUEDROID)
        target_link_libraries(${CMAKE_PROJECT_NAME}_interface
        INTERFACE
            bluedroid
        )
        if (DEFINED CFG_RTL8723BLE OR DEFINED CFG_RTL8733BLE OR DEFINED CFG_RTL8821BLE
         OR DEFINED CFG_AP6212BLE OR DEFINED CFG_AP6236BLE OR DEFINED CFG_AP6256BLE OR DEFINED CFG_MT7921BLE)
            target_link_libraries(${CMAKE_PROJECT_NAME}_interface
            INTERFACE
                bluetooth
            )
        endif()
	elseif (DEFINED CFG_BUILD_BARROT)
        target_link_libraries(${CMAKE_PROJECT_NAME}_interface
        INTERFACE
            barrot
        )
		
		if (DEFINED CFG_BARROT_MODULE_8821CS)
			add_definitions(-DCFG_BARROT_MODULE_8821CS)
		elseif (DEFINED CFG_BARROT_MODULE_BR8051)
			add_definitions(-DCFG_BARROT_MODULE_BR8051)
		endif()
		
		add_executable(${CMAKE_PROJECT_NAME}
			${PROJECT_SOURCE_DIR}/sdk/share/barrot/brt_config.c)
		
       	if (DEFINED CFG_BARROT_MODULE_8821CS OR DEFINED CFG_BARROT_MODULE_BR8051)
			target_link_libraries(${CMAKE_PROJECT_NAME}_interface
            INTERFACE
                bluetooth
            )
        endif()
    endif()
endif()

if ($ENV{CFG_PLATFORM} STREQUAL win32)
    target_link_libraries(${CMAKE_PROJECT_NAME}_interface
    INTERFACE
        boot
        itp_boot
        ith_platform
        ith
        ptw32
        ftdi
        #${PROJECT_SOURCE_DIR}/$ENV{CFG_PLATFORM}/lib/FTCSPI.lib
        ${PROJECT_SOURCE_DIR}/$ENV{CFG_PLATFORM}/lib/Packet.lib
        imm32.lib
        Vfw32.lib
        version.lib
        winmm.lib
        ws2_32.lib
    )


    target_link_libraries(${CMAKE_PROJECT_NAME}
        ${CMAKE_PROJECT_NAME}_interface)

    #configure_file(${PROJECT_SOURCE_DIR}/tool/bin/FTCSPI.dll ${CMAKE_CURRENT_BINARY_DIR}/FTCSPI.dll COPYONLY)

    if (NOT $ENV{AUTOBUILD})
        if (DEFINED CFG_PRIVATE_DRIVE)
            execute_process(COMMAND subst ${CFG_PRIVATE_DRIVE}: /D ERROR_QUIET)
            execute_process(COMMAND subst ${CFG_PRIVATE_DRIVE}: ${CMAKE_BINARY_DIR}/data/private)
        endif()

        if (DEFINED CFG_PUBLIC_DRIVE)
            execute_process(COMMAND subst ${CFG_PUBLIC_DRIVE}: /D ERROR_QUIET)
            execute_process(COMMAND subst ${CFG_PUBLIC_DRIVE}: ${CMAKE_BINARY_DIR}/data/public)
        endif()

        if (DEFINED CFG_TEMP_DRIVE)
            execute_process(COMMAND subst ${CFG_TEMP_DRIVE}: /D ERROR_QUIET)
            execute_process(COMMAND subst ${CFG_TEMP_DRIVE}: ${CMAKE_BINARY_DIR}/data/temp)
        endif()
    endif()

    if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")

        file(GLOB files ${PROJECT_SOURCE_DIR}/sdk/target/debug/win32/win32/*.in)

        foreach (src ${files})
            string(REPLACE "${PROJECT_SOURCE_DIR}/sdk/target/debug/win32/win32/" "${CMAKE_CURRENT_BINARY_DIR}/" tmp ${src})
            string(REPLACE ".in" "" dest ${tmp})
            configure_file(${src} ${dest})
        endforeach()
    endif()

elseif ($ENV{CFG_PLATFORM} STREQUAL openrtos)
    target_link_libraries(${CMAKE_PROJECT_NAME}_interface
    INTERFACE
        boot
        # itp_boot
        ith_platform
        ith
        openrtos
        malloc
    )

    ITE_LINK_LIBRARY_IF_DEFINED(CFG_CPU_RISCV cpu)

    # add risc libraray here to solve linking order issue
    ITE_LINK_LIBRARY_IF_DEFINED(CFG_AUDIO_ENABLE risc)
    ITE_LINK_LIBRARY_IF_DEFINED(CFG_BUILD_STL risc)
    # ARM LITE DEV
    if (DEFINED CFG_ARMLITE_ENABLE)
        target_link_libraries(${CMAKE_PROJECT_NAME}_interface
        INTERFACE
            risc
        )

        ITE_LINK_LIBRARY_IF_DEFINED(CFG_ARMLITE_OPUS_CODEC opuscodec)
        ITE_LINK_LIBRARY_IF_DEFINED(CFG_ARMLITE_HUD_BG_FAST hudBgFast)
		ITE_LINK_LIBRARY_IF_DEFINED(CFG_ARMLITE_CANBUS can_bus_lite)
        ITE_LINK_LIBRARY_IF_DEFINED(CFG_ARMLITE_LCD_CRC_CAL lcdCrcCal)
    endif()

    if (DEFINED CFG_RISCV2_ENABLE)
        target_link_libraries(${CMAKE_PROJECT_NAME}_interface
        INTERFACE
            risc
        )

        ITE_LINK_LIBRARY_IF_DEFINED(CFG_RISCV2_TEST_DEVICE riscv2TestDevice)
    endif()

    # add swUart libraray here to solve linking order issue
    ITE_LINK_LIBRARY_IF_DEFINED(CFG_SW_UART swUart)
    ITE_LINK_LIBRARY_IF_DEFINED(CFG_SW_UART_LIN swUartLIN)

    # add uart libraray here to solve linking order issue
    ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(uart)

    # move alt cpu libraray here to solve linking order issue
    # ALT CPU
    if (DEFINED CFG_ALT_CPU_ENABLE)
        target_link_libraries(${CMAKE_PROJECT_NAME}_interface
        INTERFACE
            risc
        )

        ITE_LINK_LIBRARY_IF_DEFINED(CFG_RSL_MASTER rslMaster)
        ITE_LINK_LIBRARY_IF_DEFINED(CFG_RSL_SLAVE rslSlave)
        ITE_LINK_LIBRARY_IF_DEFINED(CFG_SW_PWM swPwm)
        ITE_LINK_LIBRARY_IF_DEFINED(CFG_PATTERN_GEN patternGen)
        ITE_LINK_LIBRARY_IF_DEFINED(CFG_SW_UART swUart)
        ITE_LINK_LIBRARY_IF_DEFINED(CFG_SW_SERIAL_PORT swSerialPort)
        ITE_LINK_LIBRARY_IF_DEFINED(CFG_OLED_CTRL oledCtrl)
        ITE_LINK_LIBRARY_IF_DEFINED(CFG_FREQ_DETECT freqDetect)
        ITE_LINK_LIBRARY_IF_DEFINED(CFG_WATCHDOG_MONITOR watchDogMonitor)
        ITE_LINK_LIBRARY_IF_DEFINED(CFG_SW_UART_LIN swUartLIN)

        if (DEFINED CFG_ALT_CPU_CUSTOM_DEVICE)
            target_link_libraries(${CMAKE_PROJECT_NAME}_interface
            INTERFACE
                customDevice
            )
            ITE_LINK_LIBRARY_IF_DEFINED_CFG_BUILD_LIB(m2d)
        endif()
    endif()

    target_link_libraries(${CMAKE_PROJECT_NAME}
        ${CMAKE_PROJECT_NAME}_interface
    )

    # remove previous built target file
    file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME})

    # mkrom arguments
    set(args -max_file 10)

    if (DEFINED CFG_ROM_COMPRESS)
        if (DEFINED CFG_CHIP_PKG_IT9910)
            set(args ${args} -z0 -b 512K)
        else()
            set(args ${args} -z -b 512K)
        endif()
    endif()

    if (DEFINED CFG_SD_DUAL_BOOT)
        if (DEFINED CFG_SD0_ENABLE AND NOT DEFINED CFG_SD0_STATIC)
            set(args ${args} -d ${CFG_GPIO_SD0_CARD_DETECT})
        elseif (DEFINED CFG_SD1_ENABLE AND NOT DEFINED CFG_SD1_STATIC)
            set(args ${args} -d ${CFG_GPIO_SD1_CARD_DETECT})
        endif()
    endif()

    if (DEFINED CFG_SPI_NAND_BOOT)
        set(args ${args} -snf ${CFG_NAND_PAGE_SIZE} ${CFG_NAND_BLOCK_SIZE} ${CFG_SPI_NAND_ADDR_TYPE})
    endif()

    # make rom
    add_custom_command(
        TARGET ${CMAKE_PROJECT_NAME}
        POST_BUILD
        COMMAND ${OBJCOPY}
        ARGS --remove-section=.dlm_* -O binary ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin
    )

    # If security boot is enabled and the current project is bootloader
    if ((DEFINED CFG_SECURITY_SIGNATURE) AND ("${CMAKE_PROJECT_NAME}" STREQUAL "bootloader"))
        set(args ${args} -security ${CMAKE_BINARY_DIR}/rsa/rsa_pub.txt)
		if (NOT (EXISTS ${CMAKE_BINARY_DIR}/rsa/rsa_priv.txt))
			add_custom_command(
				TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
				COMMAND rsa_genkey
			)
			add_custom_command(
				TARGET ${CMAKE_PROJECT_NAME}
				POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy
				ARGS ${CMAKE_CURRENT_BINARY_DIR}/rsa_pub.txt ${CMAKE_BINARY_DIR}/rsa/rsa_pub.txt
				DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/rsa_pub.txt
				COMMAND ${CMAKE_COMMAND} -E copy
				ARGS ${CMAKE_CURRENT_BINARY_DIR}/rsa_priv.txt ${CMAKE_BINARY_DIR}/rsa/rsa_priv.txt
				DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/rsa_priv.txt
			)
		endif()
	elseif (DEFINED CFG_SECURITY_SIGNATURE)
		set(args ${args} -security ${CMAKE_BINARY_DIR}/rsa/rsa_pub.txt)
    endif()
    if (DEFINED CFG_SPEEDY_DECOMPRESS_ENABLE)
        if (NOT("${CMAKE_PROJECT_NAME}" STREQUAL "bootloader"))
            add_custom_command(
                TARGET ${CMAKE_PROJECT_NAME}
                POST_BUILD
                COMMAND blzpack
                ARGS c ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.spd
            )
        endif()
    endif()

    if (DEFINED CFG_DLM_ENABLE)
        if (NOT("${CMAKE_PROJECT_NAME}" STREQUAL "bootloader"))

            # make dlm
            add_custom_command(
                TARGET ${CMAKE_PROJECT_NAME}
                POST_BUILD
                COMMAND ${OBJCOPY}
                ARGS --only-section=.dlm_* -O binary ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME} ${CMAKE_CURRENT_BINARY_DIR}/dlm.bin
            )

        else()

            # make bootloader dlm
            if (DEFINED CFG_BL_DLM_ENABLE)
                add_custom_command(
                    TARGET ${CMAKE_PROJECT_NAME}
                    POST_BUILD
                    COMMAND ${OBJCOPY}
                    ARGS --only-section=.dlm_* -O binary ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME} ${CMAKE_CURRENT_BINARY_DIR}/bl_dlm.bin
                )
            endif()
        endif()
    endif()

    add_custom_command(
        TARGET ${CMAKE_PROJECT_NAME}
        POST_BUILD
        COMMAND ${READELF}
        ARGS -a ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME} > ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.symbol
    )

    if ((DEFINED CFG_CPU_FA626) AND ("${CMAKE_PROJECT_NAME}" STREQUAL "bootloader"))

        # remove previous generated script file
        file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/init.scr)

        get_directory_property(defs COMPILE_DEFINITIONS)

        foreach (def ${defs})
            set(defargs ${defargs} -D${def})
        endforeach()

        if (CMAKE_BUILD_TYPE STREQUAL Debug)
            set(defargs ${defargs} -g)
        endif()

        if (DEFINED CFG_LCD_INIT_ON_BOOTING)
            add_custom_command(
                TARGET ${CMAKE_PROJECT_NAME} PRE_BUILD
                COMMAND ${CMAKE_C_COMPILER}
                    ARGS ${CMAKE_C_COMPILER_ARG1} ${defargs} -mcpu=fa626te -O3 -fPIC
                        -I${PROJECT_SOURCE_DIR}/$ENV{CFG_PLATFORM}/include
                        -I${PROJECT_SOURCE_DIR}/sdk/include
                        -o ${CMAKE_CURRENT_BINARY_DIR}/lcd_clear
                        ${PROJECT_SOURCE_DIR}/$ENV{CFG_PLATFORM}/boot/fa626/lcd_clear.c
                        -Ttext 0x80000000 -nostartfiles
                COMMAND ${OBJCOPY}
                    ARGS -O binary ${CMAKE_CURRENT_BINARY_DIR}/lcd_clear ${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/lcd_clear.bin
                COMMAND ${READELF}
                    ARGS -a ${CMAKE_CURRENT_BINARY_DIR}/lcd_clear > ${CMAKE_CURRENT_BINARY_DIR}/lcd_clear.symbol
                #COMMAND ${OBJDUMP}
                #    ARGS -d ${CMAKE_CURRENT_BINARY_DIR}/lcd_clear > ${CMAKE_CURRENT_BINARY_DIR}/lcd_clear.dbg
                DEPENDS ${PROJECT_SOURCE_DIR}/$ENV{CFG_PLATFORM}/boot/fa626/lcd_clear.c
            )

            if (DEFINED CFG_BACKLIGHT_ENABLE)
                add_custom_command(
                    TARGET ${CMAKE_PROJECT_NAME} PRE_BUILD
                    COMMAND ${CMAKE_C_COMPILER}
                        ARGS ${CMAKE_C_COMPILER_ARG1} ${defargs} -mcpu=fa626te -O3 -fPIC
                            -I${PROJECT_SOURCE_DIR}/$ENV{CFG_PLATFORM}/include
                            -I${PROJECT_SOURCE_DIR}/sdk/include
                            -o ${CMAKE_CURRENT_BINARY_DIR}/backlight
                            ${PROJECT_SOURCE_DIR}/$ENV{CFG_PLATFORM}/boot/fa626/backlight.c
                            -Ttext 0x80000000 -nostartfiles
                    COMMAND ${OBJCOPY}
                        ARGS -O binary ${CMAKE_CURRENT_BINARY_DIR}/backlight ${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/backlight.bin
                    COMMAND ${READELF}
                        ARGS -a ${CMAKE_CURRENT_BINARY_DIR}/backlight > ${CMAKE_CURRENT_BINARY_DIR}/backlight.symbol
                    #COMMAND ${OBJDUMP}
                    #    ARGS -d ${CMAKE_CURRENT_BINARY_DIR}/backlight > ${CMAKE_CURRENT_BINARY_DIR}/backlight.dbg
                    DEPENDS ${PROJECT_SOURCE_DIR}/$ENV{CFG_PLATFORM}/boot/fa626/backlight.c
                )
            endif()
        endif()

        add_custom_command(
            TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND}
            ARGS
                -DCFG_BACKLIGHT_ENABLE=${CFG_BACKLIGHT_ENABLE}
                -DCFG_LCD_INIT_ON_BOOTING=${CFG_LCD_INIT_ON_BOOTING}
                -DCFG_LCD_MULTIPLE=${CFG_LCD_MULTIPLE}
                -DCFG_LCD_BPP=${CFG_LCD_BPP}
                -DCFG_LCD_BOOT_BITMAP=${CFG_LCD_BOOT_BITMAP}
                -DCFG_LCD_BOOT_BGCOLOR=${CFG_LCD_BOOT_BGCOLOR}
                -DCFG_LCD_HEIGHT=${CFG_LCD_HEIGHT}
                -DCFG_LCD_INIT_SCRIPT=${CFG_LCD_INIT_SCRIPT}
                -DCFG_LCD_PITCH=${CFG_LCD_PITCH}
                -DCFG_LCD_WIDTH=${CFG_LCD_WIDTH}
                -DCFG_RAM_INIT_SCRIPT=${CFG_RAM_INIT_SCRIPT}
                -DCFG_TILING_WIDTH_128=${CFG_TILING_WIDTH_128}
                -DCFG_SET_ROTATE=${CFG_SET_ROTATE}
                -DCPP=${CPP}
                -DCMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
                -DCMAKE_PROJECT_NAME=${CMAKE_PROJECT_NAME}
                -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
                -P ${PROJECT_SOURCE_DIR}/$ENV{CFG_PLATFORM}/boot/fa626/init.cmake
            COMMAND mkrom
            ARGS ${args} ${CMAKE_CURRENT_BINARY_DIR}/init.scr ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin ${CMAKE_CURRENT_BINARY_DIR}/kproc.sys
        )

        if (DEFINED CFG_BL_DLM_ENABLE)
            add_custom_command(
                TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
                COMMAND mkrom
                ARGS -z -b 512K ${PROJECT_SOURCE_DIR}/sdk/target/ram/${CFG_RAM_INIT_SCRIPT} ${CMAKE_CURRENT_BINARY_DIR}/bl_dlm.bin ${CMAKE_CURRENT_BINARY_DIR}/bl_dlm.dpk
            )
        endif()

    else()

        if (DEFINED CFG_SPEEDY_DECOMPRESS_ENABLE)
            if (DEFINED CFG_SECURITY_SIGNATURE)
                add_custom_command(
                    TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
                    COMMAND mkrom
                    ARGS -security ${CMAKE_BINARY_DIR}/rsa/rsa_pub.txt ${PROJECT_SOURCE_DIR}/sdk/target/ram/${CFG_RAM_INIT_SCRIPT} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.spd ${CMAKE_CURRENT_BINARY_DIR}/kproc.sys
                )
            else()
                add_custom_command(
                    TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
                    COMMAND mkrom
                    ARGS ${PROJECT_SOURCE_DIR}/sdk/target/ram/${CFG_RAM_INIT_SCRIPT} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.spd ${CMAKE_CURRENT_BINARY_DIR}/kproc.sys
                )
            endif()
        else()
            add_custom_command(
                TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
                COMMAND mkrom
                ARGS ${args} ${PROJECT_SOURCE_DIR}/sdk/target/ram/${CFG_RAM_INIT_SCRIPT} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin ${CMAKE_CURRENT_BINARY_DIR}/kproc.sys
            )
        endif()

        if (DEFINED CFG_DLM_ENABLE)
            add_custom_command(
                TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
                COMMAND mkrom
                ARGS -z -b 512K ${PROJECT_SOURCE_DIR}/sdk/target/ram/${CFG_RAM_INIT_SCRIPT} ${CMAKE_CURRENT_BINARY_DIR}/dlm.bin ${CMAKE_CURRENT_BINARY_DIR}/dlm.dpk
            )
        endif()

        if (DEFINED CFG_UPGRADE_BOOTING_FROM_PRIVATE)
            add_custom_command(
                TARGET ${CMAKE_PROJECT_NAME}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                ARGS ${CMAKE_CURRENT_BINARY_DIR}/kproc.sys ${CMAKE_BINARY_DIR}/data/private/kproc.sys
                DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/kproc.sys
            )
        endif()

    endif()

	# If security boot is enabled and the current project is bootloader
    if ((DEFINED CFG_SECURITY_SIGNATURE))# AND ("${CMAKE_PROJECT_NAME}" STREQUAL "bootloader"))
        set(args ${args} -security ${CMAKE_BINARY_DIR}/rsa/rsa_pub.txt)

        # Copy ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin to ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}_o.bin
        add_custom_command(
            TARGET ${CMAKE_PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ARGS ${CMAKE_CURRENT_BINARY_DIR}/kproc.sys ${CMAKE_CURRENT_BINARY_DIR}/kproc_o.sig
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/kproc.sys
        )

        #message("mkrom do B flow:parameters: ${args}")
        add_custom_command(
            TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
            COMMAND rsa_sign
            ARGS ${CMAKE_CURRENT_BINARY_DIR}/kproc.sys ${CMAKE_BINARY_DIR}/rsa/rsa_priv.txt
        )

        add_custom_command(
            TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
            COMMAND binaddsig
            ARGS ${CMAKE_CURRENT_BINARY_DIR}/kproc.sys.sig ${CMAKE_CURRENT_BINARY_DIR}/kproc_o.sig ${CMAKE_CURRENT_BINARY_DIR}/kproc.sys
        )
        #file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/kproc.sys.sig)
    endif()

    # debug
    if (DEFINED CFG_DBG_OUTPUT_DEBUG_FILES)
        add_custom_command(
            TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
            COMMAND ${OBJDUMP}
            ARGS -d ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME} > ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.dbg
        )
    endif()

    # make pkg
    unset(args)
    unset(packargs)
    unset(pkgdsk)

    # backup package
    if (DEFINED CFG_UPGRADE_BACKUP_PACKAGE)
        if (NOT (${CMAKE_PROJECT_NAME} STREQUAL "bootloader"))
            if (DEFINED CFG_UPGRADE_IMAGE_NAND)
                set(args ${args} --unformatted-device 0 --unformatted-index 0 --unformatted-pos ${CFG_UPGRADE_BACKUP_PACKAGE_POS})
                #set(packargs ${packargs} --unformatted-device 0 --unformatted-index 0 --unformatted-pos ${CFG_UPGRADE_BACKUP_PACKAGE_POS})
                #set(packargs ${packargs} --unformatted-device 0 --unformatted-index 0 --unformatted-pos ${CFG_UPGRADE_BACKUP_PACKAGE_POS})

            elseif (DEFINED CFG_UPGRADE_IMAGE_NOR)
                set(args ${args} --unformatted-device 1 --unformatted-index 0 --unformatted-pos ${CFG_UPGRADE_BACKUP_PACKAGE_POS})
                set(packargs ${packargs} --unformatted-device 1 --unformatted-index 0 --unformatted-pos ${CFG_UPGRADE_BACKUP_PACKAGE_POS})

            elseif (DEFINED CFG_UPGRADE_IMAGE_SD0)
                set(args ${args} --unformatted-device 2 --unformatted-index 0 --unformatted-pos ${CFG_UPGRADE_BACKUP_PACKAGE_POS})
                set(packargs ${packargs} --unformatted-device 1 --unformatted-index 0 --unformatted-pos ${CFG_UPGRADE_BACKUP_PACKAGE_POS})

            elseif (DEFINED CFG_UPGRADE_IMAGE_SD1)
                set(args ${args} --unformatted-device 3 --unformatted-index 0 --unformatted-pos ${CFG_UPGRADE_BACKUP_PACKAGE_POS})
                set(packargs ${packargs} --unformatted-device 1 --unformatted-index 0 --unformatted-pos ${CFG_UPGRADE_BACKUP_PACKAGE_POS})

            endif()
        endif()
    endif()

    unset(bootloader_args)

    # bootloader
    if (DEFINED CFG_UPGRADE_BOOTLOADER AND DEFINED CFG_BOOTLOADER_ENABLE)
        if ((DEFINED CFG_UPGRADE_BOOTLOADER_EXTERNAL_PROJECT) AND (DEFINED CFG_UPGRADE_BOOTLOADER_EXTERNAL_PROJECT_PATH))
            set(bootloader_name ${CFG_UPGRADE_BOOTLOADER_EXTERNAL_PROJECT_PATH})
        else()
            set(bootloader_name bootloader)
        endif()
        set(bootloader_path ${CMAKE_BINARY_DIR}/project/${bootloader_name}/kproc.sys)

        set(pack_bootloader_path "./${bootloader_name}/kproc.sys")

        if (DEFINED CFG_UPGRADE_BOOTLOADER_NAND)
            set(bootloader_args --unformatted-device 0 --unformatted-index 1 --unformatted-data ${bootloader_path})
            set(packargs ${packargs} --unformatted-device 0 --unformatted-index 1 --unformatted-data ${pack_bootloader_path})
            set(pkgdsk ${pkgdsk} --unformatted-device 0 --unformatted-index 1 --unformatted-data ${bootloader_path})

            if (DEFINED CFG_BL_DLM_ENABLE AND NOT DEFINED CFG_UPGRADE_BACKUP_PACKAGE)
                set(bootloader_args ${bootloader_args} --unformatted-device 0 --unformatted-index 0 --unformatted-data ${CMAKE_BINARY_DIR}/project/${bootloader_name}/bl_dlm.dpk)
                set(bootloader_args ${bootloader_args} --unformatted-pos ${CFG_UPGRADE_BL_DLM_PACKAGE_POS})

                set(packargs ${packargs} --unformatted-device 0 --unformatted-index 0 --unformatted-data ${CMAKE_BINARY_DIR}/project/${bootloader_name}/bl_dlm.dpk)
                set(packargs ${packargs} --unformatted-pos ${CFG_UPGRADE_BL_DLM_PACKAGE_POS})

                set(pkgdsk ${pkgdsk} --unformatted-device 0 --unformatted-index 0 --unformatted-data ${CMAKE_BINARY_DIR}/project/${bootloader_name}/bl_dlm.dpk)
                set(pkgdsk ${pkgdsk} --unformatted-pos ${CFG_UPGRADE_BL_DLM_PACKAGE_POS})
            endif()

        elseif (DEFINED CFG_UPGRADE_BOOTLOADER_NOR)
            set(bootloader_args --unformatted-device 1 --unformatted-index 1 --unformatted-data ${bootloader_path})
            set(packargs ${packargs} --unformatted-device 1 --unformatted-index 1 --unformatted-data ${pack_bootloader_path})
            #set(pkgdsk ${pkgdsk} --unformatted-device 1 --unformatted-index 1 --unformatted-data ${pack_bootloader_path})

            if (DEFINED CFG_BL_DLM_ENABLE AND NOT DEFINED CFG_UPGRADE_BACKUP_PACKAGE)
                set(bootloader_args ${bootloader_args} --unformatted-device 1 --unformatted-index 0 --unformatted-data ${CMAKE_BINARY_DIR}/project/${bootloader_name}/bl_dlm.dpk)
                set(bootloader_args ${bootloader_args} --unformatted-pos ${CFG_UPGRADE_BL_DLM_PACKAGE_POS})

                set(packargs ${packargs} --unformatted-device 1 --unformatted-index 0 --unformatted-data ${CMAKE_BINARY_DIR}/project/${bootloader_name}/bl_dlm.dpk)
                set(packargs ${packargs} --unformatted-pos ${CFG_UPGRADE_BL_DLM_PACKAGE_POS})
            endif()
        elseif (DEFINED CFG_UPGRADE_BOOTLOADER_SD0)
            set(bootloader_args --unformatted-device 2 --unformatted-index 1 --unformatted-data ${bootloader_path})
            set(packargs ${packargs} --unformatted-device 2 --unformatted-index 1 --unformatted-data ${pack_bootloader_path})
            #set(pkgdsk ${pkgdsk} --unformatted-device 2 --unformatted-index 1 --unformatted-data ${pack_bootloader_path})
        elseif (DEFINED CFG_UPGRADE_BOOTLOADER_SD1)
            set(bootloader_args --unformatted-device 3 --unformatted-index 1 --unformatted-data ${bootloader_path})
            set(packargs ${packargs} --unformatted-device 3 --unformatted-index 1 --unformatted-data ${pack_bootloader_path})
            #set(pkgdsk ${pkgdsk} --unformatted-device 3 --unformatted-index 1 --unformatted-data ${pack_bootloader_path})
        endif()
    endif()

    # image
    if (DEFINED CFG_UPGRADE_IMAGE)
        if (NOT (${CMAKE_PROJECT_NAME} STREQUAL "bootloader"))
            if (DEFINED CFG_UPGRADE_IMAGE_NAND)
                set(args ${args} --unformatted-device 0 --unformatted-index 2 --unformatted-data ${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/kproc.sys)
                set(args ${args} --unformatted-pos ${CFG_UPGRADE_IMAGE_POS})

                set(packargs ${packargs} --unformatted-device 0 --unformatted-index 2 --unformatted-data "./${CMAKE_PROJECT_NAME}/kproc.sys")
                set(packargs ${packargs} --unformatted-pos ${CFG_UPGRADE_IMAGE_POS})

                set(pkgdsk ${pkgdsk} --unformatted-device 0 --unformatted-index 2 --unformatted-data ${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/kproc.sys)
                set(pkgdsk ${pkgdsk} --unformatted-pos ${CFG_UPGRADE_IMAGE_POS})

                if (DEFINED CFG_DLM_ENABLE)
                    set(args ${args} --unformatted-device 0 --unformatted-index 3 --unformatted-data ${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/dlm.dpk)
                    set(args ${args} --unformatted-pos ${CFG_UPGRADE_DLM_PACKAGE_POS})

                    set(packargs ${packargs} --unformatted-device 0 --unformatted-index 3 --unformatted-data ${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/dlm.dpk)
                    set(packargs ${packargs} --unformatted-pos ${CFG_UPGRADE_DLM_PACKAGE_POS})

                    set(pkgdsk ${pkgdsk} --unformatted-device 0 --unformatted-index 3 --unformatted-data ${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/dlm.dpk)
                    set(pkgdsk ${pkgdsk} --unformatted-pos ${CFG_UPGRADE_DLM_PACKAGE_POS})
                endif()

            elseif (DEFINED CFG_UPGRADE_IMAGE_NOR)
                set(args ${args} --unformatted-device 1 --unformatted-index 2 --unformatted-data ${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/kproc.sys)
                set(args ${args} --unformatted-pos ${CFG_UPGRADE_IMAGE_POS})

                set(packargs ${packargs} --unformatted-device 1 --unformatted-index 2 --unformatted-data "./${CMAKE_PROJECT_NAME}/kproc.sys")
                set(packargs ${packargs} --unformatted-pos ${CFG_UPGRADE_IMAGE_POS})

                #set(pkgdsk ${pkgdsk} --unformatted-device 1 --unformatted-index 2 --unformatted-data "./${CMAKE_PROJECT_NAME}/kproc.sys")
                #set(pkgdsk ${pkgdsk} --unformatted-pos ${CFG_UPGRADE_IMAGE_POS})

                if (DEFINED CFG_DLM_ENABLE)
                    set(args ${args} --unformatted-device 1 --unformatted-index 3 --unformatted-data ${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/dlm.dpk)
                    set(args ${args} --unformatted-pos ${CFG_UPGRADE_DLM_PACKAGE_POS})

                    set(packargs ${packargs} --unformatted-device 1 --unformatted-index 3 --unformatted-data ${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/dlm.dpk)
                    set(packargs ${packargs} --unformatted-pos ${CFG_UPGRADE_DLM_PACKAGE_POS})
                endif()

            elseif (DEFINED CFG_UPGRADE_IMAGE_SD0)
                set(args ${args} --unformatted-device 2 --unformatted-index 2 --unformatted-data ${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/kproc.sys)
                set(args ${args} --unformatted-pos ${CFG_UPGRADE_IMAGE_POS})

                set(packargs ${packargs} --unformatted-device 2 --unformatted-index 2 --unformatted-data "./${CMAKE_PROJECT_NAME}/kproc.sys")
                set(packargs ${packargs} --unformatted-pos ${CFG_UPGRADE_IMAGE_POS})

                #set(pkgdsk ${pkgdsk} --unformatted-device 2 --unformatted-index 2 --unformatted-data "./${CMAKE_PROJECT_NAME}/kproc.sys")
                #set(pkgdsk ${pkgdsk} --unformatted-pos ${CFG_UPGRADE_IMAGE_POS})

            elseif (DEFINED CFG_UPGRADE_IMAGE_SD1)
                set(args ${args} --unformatted-device 3 --unformatted-index 2 --unformatted-data ${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/kproc.sys)
                set(args ${args} --unformatted-pos ${CFG_UPGRADE_IMAGE_POS})

                set(packargs ${packargs} --unformatted-device 3 --unformatted-index 2 --unformatted-data "./${CMAKE_PROJECT_NAME}/kproc.sys")
                set(packargs ${packargs} --unformatted-pos ${CFG_UPGRADE_IMAGE_POS})

                #set(pkgdsk ${pkgdsk} --unformatted-device 3 --unformatted-index 2 --unformatted-data "./${CMAKE_PROJECT_NAME}/kproc.sys")
                #set(pkgdsk ${pkgdsk} --unformatted-pos ${CFG_UPGRADE_IMAGE_POS})

            endif()
        endif()
    endif()

    if (DEFINED CFG_A_B_DUAL_BOOT_ENABLE AND DEFINED CFG_UPGRADE_PARTITION AND DEFINED CFG_UPGRADE_IMAGE)
        if (NOT (${CMAKE_PROJECT_NAME} STREQUAL "bootloader"))
            if (DEFINED CFG_UPGRADE_IMAGE_NAND)
            	set(args ${args} --unformatted-device 0 --unformatted-index 5 --unformatted-data ${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/kproc.sys)
                set(args ${args} --unformatted-pos ${CFG_UPGRADE_SUB_IMAGE_POS})

                set(packargs ${packargs} --unformatted-device 0 --unformatted-index 5 --unformatted-data "./${CMAKE_PROJECT_NAME}/kproc.sys")
                set(packargs ${packargs} --unformatted-pos ${CFG_UPGRADE_SUB_IMAGE_POS})

                set(pkgdsk ${pkgdsk} --unformatted-device 0 --unformatted-index 5 --unformatted-data ${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/kproc.sys)
                set(pkgdsk ${pkgdsk} --unformatted-pos ${CFG_UPGRADE_SUB_IMAGE_POS})

            elseif (DEFINED CFG_UPGRADE_IMAGE_NOR)
				set(args ${args} --unformatted-device 1 --unformatted-index 5 --unformatted-data ${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/kproc.sys)
                set(args ${args} --unformatted-pos ${CFG_UPGRADE_SUB_IMAGE_POS})

                set(packargs ${packargs} --unformatted-device 1 --unformatted-index 5 --unformatted-data "./${CMAKE_PROJECT_NAME}/kproc.sys")
                set(packargs ${packargs} --unformatted-pos ${CFG_UPGRADE_SUB_IMAGE_POS})

                #set(pkgdsk ${pkgdsk} --unformatted-device 1 --unformatted-index 5 --unformatted-data "./${CMAKE_PROJECT_NAME}/kproc.sys")
                #set(pkgdsk ${pkgdsk} --unformatted-pos ${CFG_UPGRADE_SUB_IMAGE_POS})

            elseif (DEFINED CFG_UPGRADE_IMAGE_SD0)
                set(args ${args} --unformatted-device 2 --unformatted-index 5 --unformatted-data ${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/kproc.sys)
                set(args ${args} --unformatted-pos ${CFG_UPGRADE_SUB_IMAGE_POS})

                set(packargs ${packargs} --unformatted-device 2 --unformatted-index 5 --unformatted-data "./${CMAKE_PROJECT_NAME}/kproc.sys")
                set(packargs ${packargs} --unformatted-pos ${CFG_UPGRADE_SUB_IMAGE_POS})

                #set(pkgdsk ${pkgdsk} --unformatted-device 2 --unformatted-index 5 --unformatted-data "./${CMAKE_PROJECT_NAME}/kproc.sys")
                #set(pkgdsk ${pkgdsk} --unformatted-pos ${CFG_UPGRADE_SUB_IMAGE_POS})

            elseif (DEFINED CFG_UPGRADE_IMAGE_SD1)
                set(args ${args} --unformatted-device 3 --unformatted-index 5 --unformatted-data ${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/kproc.sys)
                set(args ${args} --unformatted-pos ${CFG_UPGRADE_SUB_IMAGE_POS})

                set(packargs ${packargs} --unformatted-device 3 --unformatted-index 5 --unformatted-data "./${CMAKE_PROJECT_NAME}/kproc.sys")
                set(packargs ${packargs} --unformatted-pos ${CFG_UPGRADE_SUB_IMAGE_POS})

                #set(pkgdsk ${pkgdsk} --unformatted-device 3 --unformatted-index 5 --unformatted-data "./${CMAKE_PROJECT_NAME}/kproc.sys")
                #set(pkgdsk ${pkgdsk} --unformatted-pos ${CFG_UPGRADE_SUB_IMAGE_POS})

            endif()
        endif()
    endif()

    # unformatted
    if (DEFINED CFG_NAND_ENABLE AND DEFINED CFG_NAND_RESERVED_SIZE)
        set(args ${args} --unformatted-device 0 --unformatted-size ${CFG_NAND_RESERVED_SIZE})
        set(packargs ${packargs} --unformatted-device 0 --unformatted-size ${CFG_NAND_RESERVED_SIZE})
        set(pkgdsk ${pkgdsk} --unformatted-device 0 --unformatted-size ${CFG_NAND_RESERVED_SIZE})
    endif()
    if (DEFINED CFG_NOR_ENABLE AND DEFINED CFG_NOR_RESERVED_SIZE)
        set(args ${args} --unformatted-device 1 --unformatted-size ${CFG_NOR_RESERVED_SIZE})
        set(packargs ${packargs} --unformatted-device 1 --unformatted-size ${CFG_NOR_RESERVED_SIZE})
        #set(pkgdsk ${pkgdsk} --unformatted-device 1 --unformatted-size ${CFG_NOR_RESERVED_SIZE})
    endif()
    if (DEFINED CFG_SD0_RESERVED_SIZE)
        set(args ${args} --unformatted-device 2 --unformatted-size ${CFG_SD0_RESERVED_SIZE})
        set(packargs ${packargs} --unformatted-device 2 --unformatted-size ${CFG_SD0_RESERVED_SIZE})
        #set(pkgdsk ${pkgdsk} --unformatted-device 2 --unformatted-size ${CFG_SD0_RESERVED_SIZE})
    endif()
    if (DEFINED CFG_SD1_RESERVED_SIZE)
        set(args ${args} --unformatted-device 3 --unformatted-size ${CFG_SD1_RESERVED_SIZE})
        set(packargs ${packargs} --unformatted-device 3 --unformatted-size ${CFG_SD1_RESERVED_SIZE})
        #set(pkgdsk ${pkgdsk} --unformatted-device 3 --unformatted-size ${CFG_SD1_RESERVED_SIZE})
    endif()

    # data
    if (DEFINED CFG_UPGRADE_DATA)
		if (DEFINED CFG_A_B_DUAL_BOOT_ENABLE AND NOT DEFINED CFG_SUB_DATA_NONE)

        	# private data
			if(NOT DEFINED CFG_PRIVATE_PARTITION)
            	set(CFG_PRIVATE_PARTITION 0)
        	endif()
        	if (DEFINED CFG_UPGRADE_PRIVATE_NAND)
        		set(args ${args} --partition-device 0 --partition-index ${CFG_PRIVATE_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/private)
        		set(packargs ${packargs} --partition-device 0 --partition-index ${CFG_PRIVATE_PARTITION} --partition-dir "%CURR_PATH%/data/private")
        	elseif (DEFINED CFG_UPGRADE_PRIVATE_NOR)
        		set(args ${args} --partition-device 1 --partition-index ${CFG_PRIVATE_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/private)
        		set(packargs ${packargs} --partition-device 1 --partition-index ${CFG_PRIVATE_PARTITION} --partition-dir "%CURR_PATH%/data/private")
        	elseif (DEFINED CFG_UPGRADE_PRIVATE_SD0)
        		set(args ${args} --partition-device 2 --partition-index ${CFG_PRIVATE_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/private)
        		set(packargs ${packargs} --partition-device 2 --partition-index ${CFG_PRIVATE_PARTITION} --partition-dir "%CURR_PATH%/data/private")
        	elseif (DEFINED CFG_UPGRADE_PRIVATE_SD1)
        		set(args ${args} --partition-device 3 --partition-index ${CFG_PRIVATE_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/private)
        		set(packargs ${packargs} --partition-device 3 --partition-index ${CFG_PRIVATE_PARTITION} --partition-dir "%CURR_PATH%/data/private")
        	endif()

        	set(lfsfatargs ${args})

        	# public data
			if(NOT DEFINED CFG_PUBLIC_PARTITION)
            	set(CFG_PUBLIC_PARTITION 1)
        	endif()
        	if (DEFINED CFG_UPGRADE_PUBLIC_NAND)
        		set(args ${args} --partition-device 0 --partition-index ${CFG_PUBLIC_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/public)
        		set(packargs ${packargs} --partition-device 0 --partition-index ${CFG_PUBLIC_PARTITION} --partition-dir "%CURR_PATH%/data/public")
        	elseif (DEFINED CFG_UPGRADE_PUBLIC_NOR)
        		set(args ${args} --partition-device 1 --partition-index ${CFG_PUBLIC_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/public)
        		set(packargs ${packargs} --partition-device 1 --partition-index ${CFG_PUBLIC_PARTITION} --partition-dir "%CURR_PATH%/data/public")
        	elseif (DEFINED CFG_UPGRADE_PUBLIC_SD0)
        		set(args ${args} --partition-device 2 --partition-index ${CFG_PUBLIC_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/public)
        		set(packargs ${packargs} --partition-device 2 --partition-index ${CFG_PUBLIC_PARTITION} --partition-dir "%CURR_PATH%/data/public")
        	elseif (DEFINED CFG_UPGRADE_PUBLIC_SD1)
        		set(args ${args} --partition-device 3 --partition-index ${CFG_PUBLIC_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/public)
        		set(packargs ${packargs} --partition-device 3 --partition-index ${CFG_PUBLIC_PARTITION} --partition-dir "%CURR_PATH%/data/public")
        	endif()

			# sub private data
			if(NOT DEFINED CFG_SUB_PRIVATE_PARTITION)
            	set(CFG_SUB_PRIVATE_PARTITION 3)
        	endif()
    	    if (DEFINED CFG_UPGRADE_PARTITION AND DEFINED CFG_UPGRADE_PRIVATE_NAND AND CFG_PRIVATE_A_B_DUAL)
          		set(args ${args} --partition-device 0 --partition-index ${CFG_SUB_PRIVATE_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/private)
              	set(packargs ${packargs} --partition-device 0 --partition-index ${CFG_SUB_PRIVATE_PARTITION} --partition-dir "%CURR_PATH%/data/private")
	        elseif (DEFINED CFG_UPGRADE_PARTITION AND DEFINED CFG_UPGRADE_PRIVATE_NOR AND CFG_PRIVATE_A_B_DUAL)
	            set(args ${args} --partition-device 1 --partition-index ${CFG_SUB_PRIVATE_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/private)
	            set(packargs ${packargs} --partition-device 1 --partition-index ${CFG_SUB_PRIVATE_PARTITION} --partition-dir "%CURR_PATH%/data/private")
	        elseif (DEFINED CFG_UPGRADE_PARTITION AND DEFINED CFG_UPGRADE_PRIVATE_SD0 AND CFG_PRIVATE_A_B_DUAL)
	            set(args ${args} --partition-device 2 --partition-index ${CFG_SUB_PRIVATE_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/private)
	            set(packargs ${packargs} --partition-device 2 --partition-index ${CFG_SUB_PRIVATE_PARTITION} --partition-dir "%CURR_PATH%/data/private")
	        elseif (DEFINED CFG_UPGRADE_PARTITION AND DEFINED CFG_UPGRADE_PRIVATE_SD1 AND CFG_PRIVATE_A_B_DUAL)
	            set(args ${args} --partition-device 3 --partition-index ${CFG_SUB_PRIVATE_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/private)
	            set(packargs ${packargs} --partition-device 3 --partition-index ${CFG_SUB_PRIVATE_PARTITION} --partition-dir "%CURR_PATH%/data/private")
	        endif()

	        # sub public data
			if(NOT DEFINED CFG_SUB_PUBLIC_PARTITION)
            	set(CFG_SUB_PUBLIC_PARTITION 4)
        	endif()
	        if (DEFINED CFG_UPGRADE_PARTITION AND DEFINED CFG_UPGRADE_PUBLIC_NAND AND CFG_PUBLIC_A_B_DUAL)
	            set(args ${args} --partition-device 0 --partition-index ${CFG_SUB_PUBLIC_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/public)
	            set(packargs ${packargs} --partition-device 0 --partition-index ${CFG_SUB_PUBLIC_PARTITION} --partition-dir "%CURR_PATH%/data/public")
	        elseif (DEFINED CFG_UPGRADE_PARTITION AND DEFINED CFG_UPGRADE_PUBLIC_NOR AND CFG_PUBLIC_A_B_DUAL)
	            set(args ${args} --partition-device 1 --partition-index ${CFG_SUB_PUBLIC_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/public)
	            set(packargs ${packargs} --partition-device 1 --partition-index ${CFG_SUB_PUBLIC_PARTITION} --partition-dir "%CURR_PATH%/data/public")
	        elseif (DEFINED CFG_UPGRADE_PARTITION AND DEFINED CFG_UPGRADE_PUBLIC_SD0 AND CFG_PUBLIC_A_B_DUAL)
	            set(args ${args} --partition-device 2 --partition-index ${CFG_SUB_PUBLIC_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/public)
	            set(packargs ${packargs} --partition-device 2 --partition-index ${CFG_SUB_PUBLIC_PARTITION} --partition-dir "%CURR_PATH%/data/public")
	        elseif (DEFINED CFG_UPGRADE_PARTITION AND DEFINED CFG_UPGRADE_PUBLIC_SD1 AND CFG_PUBLIC_A_B_DUAL)
	            set(args ${args} --partition-device 3 --partition-index ${CFG_SUB_PUBLIC_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/public)
	            set(packargs ${packargs} --partition-device 3 --partition-index ${CFG_SUB_PUBLIC_PARTITION} --partition-dir "%CURR_PATH%/data/public")
	        endif()

	        # temporary data
			if(NOT DEFINED CFG_TEMP_PARTITION)
            	set(CFG_TEMP_PARTITION 2)
        	endif()
	        if (DEFINED CFG_UPGRADE_TEMP_NAND)
	              set(args ${args} --partition-device 0 --partition-index ${CFG_TEMP_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/temp)
	              set(packargs ${packargs} --partition-device 0 --partition-index ${CFG_TEMP_PARTITION} --partition-dir "%CURR_PATH%/data/temp")
	        elseif (DEFINED CFG_UPGRADE_TEMP_NOR)
	              set(args ${args} --partition-device 1 --partition-index ${CFG_TEMP_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/temp)
	              set(packargs ${packargs} --partition-device 1 --partition-index ${CFG_TEMP_PARTITION} --partition-dir "%CURR_PATH%/data/temp")
	        elseif (DEFINED CFG_UPGRADE_TEMP_SD0)
	              set(args ${args} --partition-device 2 --partition-index ${CFG_TEMP_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/temp)
	              set(packargs ${packargs} --partition-device 2 --partition-index ${CFG_TEMP_PARTITION} --partition-dir "%CURR_PATH%/data/temp")
	        elseif (DEFINED CFG_UPGRADE_TEMP_SD1)
	              set(args ${args} --partition-device 3 --partition-index ${CFG_TEMP_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/temp)
	              set(packargs ${packargs} --partition-device 3 --partition-index ${CFG_TEMP_PARTITION} --partition-dir "%CURR_PATH%/data/temp")
	        endif()
	          
	    else()
	        # private data
        	if(NOT DEFINED CFG_UPGRADE_PRIVATE_PARTITION)
            	set(CFG_UPGRADE_PRIVATE_PARTITION 0)
	        endif()
	        if (DEFINED CFG_UPGRADE_PRIVATE_NAND)
	            set(args ${args} --partition-device 0 --partition-index ${CFG_UPGRADE_PRIVATE_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/private)
	            set(packargs ${packargs} --partition-device 0 --partition-index ${CFG_UPGRADE_PRIVATE_PARTITION} --partition-dir "%CURR_PATH%/data/private")
	        elseif (DEFINED CFG_UPGRADE_PRIVATE_NOR)
	            set(args ${args} --partition-device 1 --partition-index ${CFG_UPGRADE_PRIVATE_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/private)
	            set(packargs ${packargs} --partition-device 1 --partition-index ${CFG_UPGRADE_PRIVATE_PARTITION} --partition-dir "%CURR_PATH%/data/private")
	        elseif (DEFINED CFG_UPGRADE_PRIVATE_SD0)
	            set(args ${args} --partition-device 2 --partition-index ${CFG_UPGRADE_PRIVATE_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/private)
	            set(packargs ${packargs} --partition-device 2 --partition-index ${CFG_UPGRADE_PRIVATE_PARTITION} --partition-dir "%CURR_PATH%/data/private")
	        elseif (DEFINED CFG_UPGRADE_PRIVATE_SD1)
	            set(args ${args} --partition-device 3 --partition-index ${CFG_UPGRADE_PRIVATE_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/private)
	            set(packargs ${packargs} --partition-device 3 --partition-index ${CFG_UPGRADE_PRIVATE_PARTITION} --partition-dir "%CURR_PATH%/data/private")
	        endif()

        	set(lfsfatargs ${args})

	        # public data
	        if(NOT DEFINED CFG_UPGRADE_PUBLIC_PARTITION)
	            set(CFG_UPGRADE_PUBLIC_PARTITION 1)
	        endif()
	        if (DEFINED CFG_UPGRADE_PUBLIC_NAND)
	            set(args ${args} --partition-device 0 --partition-index ${CFG_UPGRADE_PUBLIC_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/public)
	            set(packargs ${packargs} --partition-device 0 --partition-index ${CFG_UPGRADE_PUBLIC_PARTITION} --partition-dir "%CURR_PATH%/data/public")
	            if (DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1)
	                set(lfsfatargs ${lfsfatargs} --partition-device 0 --partition-index 1 --partition-dir ${CMAKE_BINARY_DIR}/data/public)
	            endif()

	        elseif (DEFINED CFG_UPGRADE_PUBLIC_NOR)
	            set(args ${args} --partition-device 1 --partition-index ${CFG_UPGRADE_PUBLIC_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/public)
	            set(packargs ${packargs} --partition-device 1 --partition-index ${CFG_UPGRADE_PUBLIC_PARTITION} --partition-dir "%CURR_PATH%/data/public")
	            if (DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1)
	                set(lfsfatargs ${lfsfatargs} --partition-device 1 --partition-index 1 --partition-dir ${CMAKE_BINARY_DIR}/data/public)
	            endif()

	        elseif (DEFINED CFG_UPGRADE_PUBLIC_SD0)
	            set(args ${args} --partition-device 2 --partition-index ${CFG_UPGRADE_PUBLIC_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/public)
	            set(packargs ${packargs} --partition-device 2 --partition-index ${CFG_UPGRADE_PUBLIC_PARTITION} --partition-dir "%CURR_PATH%/data/public")
	            if (DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1)
	                set(lfsfatargs ${lfsfatargs} --partition-device 2 --partition-index 1 --partition-dir ${CMAKE_BINARY_DIR}/data/public)
	            endif()

	        elseif (DEFINED CFG_UPGRADE_PUBLIC_SD1)
	            set(args ${args} --partition-device 3 --partition-index ${CFG_UPGRADE_PUBLIC_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/public)
	            set(packargs ${packargs} --partition-device 3 --partition-index ${CFG_UPGRADE_PUBLIC_PARTITION} --partition-dir "%CURR_PATH%/data/public")
	            if (DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1)
	                set(lfsfatargs ${lfsfatargs} --partition-device 3 --partition-index 1 --partition-dir ${CMAKE_BINARY_DIR}/data/public)
	            endif()
	        endif()

	        # temporary data
            if(NOT DEFINED CFG_UPGRADE_TEMP_PARTITION)
                set(CFG_UPGRADE_TEMP_PARTITION 2)
            endif()
	        if (DEFINED CFG_UPGRADE_TEMP_NAND)
	            set(args ${args} --partition-device 0 --partition-index ${CFG_UPGRADE_TEMP_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/temp)
	            set(packargs ${packargs} --partition-device 0 --partition-index ${CFG_UPGRADE_TEMP_PARTITION} --partition-dir "%CURR_PATH%/data/temp")
	        elseif (DEFINED CFG_UPGRADE_TEMP_NOR)
	            set(args ${args} --partition-device 1 --partition-index ${CFG_UPGRADE_TEMP_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/temp)
	            set(packargs ${packargs} --partition-device 1 --partition-index ${CFG_UPGRADE_TEMP_PARTITION} --partition-dir "%CURR_PATH%/data/temp")
	        elseif (DEFINED CFG_UPGRADE_TEMP_SD0)
	            set(args ${args} --partition-device 2 --partition-index ${CFG_UPGRADE_TEMP_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/temp)
	            set(packargs ${packargs} --partition-device 2 --partition-index ${CFG_UPGRADE_TEMP_PARTITION} --partition-dir "%CURR_PATH%/data/temp")
	        elseif (DEFINED CFG_UPGRADE_TEMP_SD1)
	            set(args ${args} --partition-device 3 --partition-index ${CFG_UPGRADE_TEMP_PARTITION} --partition-dir ${CMAKE_BINARY_DIR}/data/temp)
	            set(packargs ${packargs} --partition-device 3 --partition-index ${CFG_UPGRADE_TEMP_PARTITION} --partition-dir "%CURR_PATH%/data/temp")
	        endif()
	    endif()

        # partition
        if (DEFINED CFG_NAND_ENABLE AND DEFINED CFG_NAND_PARTITION0)
            if (DEFINED CFG_UPGRADE_PARTITION OR DEFINED CFG_UPGRADE_FORMAT_NAND_PARTITION0)
                set(args ${args} --partition-device 0 --partition-index 0 --partition-size ${CFG_NAND_PARTITION0_SIZE})
                set(packargs ${packargs} --partition-device 0 --partition-index 0 --partition-size ${CFG_NAND_PARTITION0_SIZE})
                set(lfsfatargs ${lfsfatargs} --partition-device 0 --partition-index 0 --partition-size ${CFG_NAND_PARTITION0_SIZE})
            else()
                set(args ${args} --partition-device 0 --partition-index 0 --partition-size 0)
                set(packargs ${packargs} --partition-device 0 --partition-index 0 --partition-size 0)
                set(lfsfatargs ${lfsfatargs} --partition-device 0 --partition-index 0 --partition-size 0)
            endif()
        endif()

        if (DEFINED CFG_NAND_ENABLE AND DEFINED CFG_NAND_PARTITION0 AND DEFINED CFG_NAND_PARTITION1)
            if (DEFINED CFG_UPGRADE_PARTITION OR DEFINED CFG_UPGRADE_FORMAT_NAND_PARTITION1)
                set(args ${args} --partition-device 0 --partition-index 1 --partition-size ${CFG_NAND_PARTITION1_SIZE})
                set(packargs ${packargs} --partition-device 0 --partition-index 1 --partition-size ${CFG_NAND_PARTITION1_SIZE})

                if (DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1)
                    set(lfsfatargs ${lfsfatargs} --partition-device 0 --partition-index 1 --partition-size ${CFG_NAND_PARTITION1_SIZE})
                endif()
            else()
                set(args ${args} --partition-device 0 --partition-index 1 --partition-size 0)
                set(packargs ${packargs} --partition-device 0 --partition-index 1 --partition-size 0)

                if (DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1)
                    set(lfsfatargs ${lfsfatargs} --partition-device 0 --partition-index 1 --partition-size 0)
                endif()
            endif()
        endif()

        if (DEFINED CFG_NAND_ENABLE AND DEFINED CFG_NAND_PARTITION0 AND DEFINED CFG_NAND_PARTITION1 AND DEFINED CFG_NAND_PARTITION2)
            if (DEFINED CFG_UPGRADE_PARTITION OR DEFINED CFG_UPGRADE_FORMAT_NAND_PARTITION2)
                set(args ${args} --partition-device 0 --partition-index 2 --partition-size ${CFG_NAND_PARTITION2_SIZE})
                set(packargs ${packargs} --partition-device 0 --partition-index 2 --partition-size ${CFG_NAND_PARTITION2_SIZE})
            else()
                set(args ${args} --partition-device 0 --partition-index 2 --partition-size 0)
                set(packargs ${packargs} --partition-device 0 --partition-index 2 --partition-size 0)
            endif()
        endif()

        if (DEFINED CFG_NAND_ENABLE AND DEFINED CFG_NAND_PARTITION0 AND DEFINED CFG_NAND_PARTITION1 AND DEFINED CFG_NAND_PARTITION2 AND DEFINED CFG_NAND_PARTITION3)
            set(args ${args} --partition-device 0 --partition-index 3 --partition-size ${CFG_NAND_PARTITION3_SIZE})
            set(packargs ${packargs} --partition-device 0 --partition-index 3 --partition-size ${CFG_NAND_PARTITION3_SIZE})
        endif()

		if (DEFINED CFG_NAND_ENABLE AND DEFINED CFG_NAND_PARTITION0 AND DEFINED CFG_NAND_PARTITION1 AND DEFINED CFG_NAND_PARTITION2 AND DEFINED CFG_NAND_PARTITION3 AND DEFINED CFG_NAND_PARTITION4)
            set(args ${args} --partition-device 0 --partition-index 4 --partition-size ${CFG_NAND_PARTITION4_SIZE})
            set(packargs ${packargs} --partition-device 0 --partition-index 4 --partition-size ${CFG_NAND_PARTITION4_SIZE})
        endif()

        if (DEFINED CFG_NAND_ENABLE AND DEFINED CFG_NAND_PARTITION0 AND DEFINED CFG_NAND_PARTITION1 AND DEFINED CFG_NAND_PARTITION2 AND DEFINED CFG_NAND_PARTITION3 AND DEFINED CFG_NAND_PARTITION4 AND DEFINED CFG_NAND_PARTITION5)
            set(args ${args} --partition-device 0 --partition-index 5 --partition-size ${CFG_NAND_PARTITION5_SIZE})
            set(packargs ${packargs} --partition-device 0 --partition-index 5 --partition-size ${CFG_NAND_PARTITION5_SIZE})
        endif()
 
        if (DEFINED CFG_NOR_ENABLE AND DEFINED CFG_NOR_PARTITION0)
            if (DEFINED CFG_UPGRADE_PARTITION OR DEFINED CFG_UPGRADE_FORMAT_NOR_PARTITION0)
                set(args ${args} --partition-device 1 --partition-index 0 --partition-size ${CFG_NOR_PARTITION0_SIZE})
                set(packargs ${packargs} --partition-device 1 --partition-index 0 --partition-size ${CFG_NOR_PARTITION0_SIZE})
                set(lfsfatargs ${lfsfatargs} --partition-device 1 --partition-index 0 --partition-size ${CFG_NOR_PARTITION0_SIZE})
            else()
                set(args ${args} --partition-device 1 --partition-index 0 --partition-size 0)
                set(packargs ${packargs} --partition-device 1 --partition-index 0 --partition-size 0)
                set(lfsfatargs ${lfsfatargs} --partition-device 1 --partition-index 0 --partition-size 0)
            endif()
        endif()

        if (DEFINED CFG_NOR_ENABLE AND DEFINED CFG_NOR_PARTITION0 AND DEFINED CFG_NOR_PARTITION1)
            if (DEFINED CFG_UPGRADE_PARTITION OR DEFINED CFG_UPGRADE_FORMAT_NOR_PARTITION1)
                set(args ${args} --partition-device 1 --partition-index 1 --partition-size ${CFG_NOR_PARTITION1_SIZE})
                set(packargs ${packargs} --partition-device 1 --partition-index 1 --partition-size ${CFG_NOR_PARTITION1_SIZE})

                if (DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1)
                    set(lfsfatargs ${lfsfatargs} --partition-device 1 --partition-index 1 --partition-size ${CFG_NOR_PARTITION1_SIZE})
                endif()
            else()
                set(args ${args} --partition-device 1 --partition-index 1 --partition-size 0)
                set(packargs ${packargs} --partition-device 1 --partition-index 1 --partition-size 0)

                if (DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1)
                    set(lfsfatargs ${lfsfatargs} --partition-device 1 --partition-index 1 --partition-size 0)
                endif()
            endif()
        endif()

        if (DEFINED CFG_NOR_ENABLE AND DEFINED CFG_NOR_PARTITION0 AND DEFINED CFG_NOR_PARTITION1 AND DEFINED CFG_NOR_PARTITION2)
            if (DEFINED CFG_UPGRADE_PARTITION OR DEFINED CFG_UPGRADE_FORMAT_NOR_PARTITION2)
                set(args ${args} --partition-device 1 --partition-index 2 --partition-size ${CFG_NOR_PARTITION2_SIZE})
                set(packargs ${packargs} --partition-device 1 --partition-index 2 --partition-size ${CFG_NOR_PARTITION2_SIZE})
            else()
                set(args ${args} --partition-device 1 --partition-index 2 --partition-size 0)
                set(packargs ${packargs} --partition-device 1 --partition-index 2 --partition-size 0)
            endif()
        endif()

        if (DEFINED CFG_NOR_ENABLE AND DEFINED CFG_NOR_PARTITION0 AND DEFINED CFG_NOR_PARTITION1 AND DEFINED CFG_NOR_PARTITION2 AND DEFINED CFG_NOR_PARTITION3)
            set(args ${args} --partition-device 1 --partition-index 3 --partition-size ${CFG_NOR_PARTITION3_SIZE})
            set(packargs ${packargs} --partition-device 1 --partition-index 3 --partition-size ${CFG_NOR_PARTITION3_SIZE})
        endif()

		if (DEFINED CFG_NOR_ENABLE AND DEFINED CFG_NOR_PARTITION0 AND DEFINED CFG_NOR_PARTITION1 AND DEFINED CFG_NOR_PARTITION2 AND DEFINED CFG_NOR_PARTITION3 AND DEFINED CFG_NOR_PARTITION4)
            set(args ${args} --partition-device 1 --partition-index 4 --partition-size ${CFG_NOR_PARTITION4_SIZE})
            set(packargs ${packargs} --partition-device 1 --partition-index 4 --partition-size ${CFG_NOR_PARTITION4_SIZE})
        endif()

        if (DEFINED CFG_NOR_ENABLE AND DEFINED CFG_NOR_PARTITION0 AND DEFINED CFG_NOR_PARTITION1 AND DEFINED CFG_NOR_PARTITION2 AND DEFINED CFG_NOR_PARTITION3 AND DEFINED CFG_NOR_PARTITION4 AND DEFINED CFG_NOR_PARTITION5)
            set(args ${args} --partition-device 1 --partition-index 5 --partition-size ${CFG_NOR_PARTITION5_SIZE})
            set(packargs ${packargs} --partition-device 1 --partition-index 5 --partition-size ${CFG_NOR_PARTITION5_SIZE})
        endif()

        if (DEFINED CFG_SD0_PARTITION0)
            if (DEFINED CFG_UPGRADE_PARTITION OR DEFINED CFG_UPGRADE_FORMAT_SD0_PARTITION0)
                set(args ${args} --partition-device 2 --partition-index 0 --partition-size ${CFG_SD0_PARTITION0_SIZE})
                set(packargs ${packargs} --partition-device 2 --partition-index 0 --partition-size ${CFG_SD0_PARTITION0_SIZE})
                set(lfsfatargs ${lfsfatargs} --partition-device 2 --partition-index 0 --partition-size ${CFG_SD0_PARTITION0_SIZE})
            else()
                set(args ${args} --partition-device 2 --partition-index 0 --partition-size 0)
                set(packargs ${packargs} --partition-device 2 --partition-index 0 --partition-size 0)
                set(lfsfatargs ${lfsfatargs} --partition-device 2 --partition-index 0 --partition-size 0)
            endif()
        endif()

        if (DEFINED CFG_SD0_PARTITION0 AND DEFINED CFG_SD0_PARTITION1)
            if (DEFINED CFG_UPGRADE_PARTITION OR DEFINED CFG_UPGRADE_FORMAT_SD0_PARTITION1)
                set(args ${args} --partition-device 2 --partition-index 1 --partition-size ${CFG_SD0_PARTITION1_SIZE})
                set(packargs ${packargs} --partition-device 2 --partition-index 1 --partition-size ${CFG_SD0_PARTITION1_SIZE})
                if (DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1)
                    set(lfsfatargs ${lfsfatargs} --partition-device 2 --partition-index 1 --partition-size ${CFG_SD0_PARTITION1_SIZE})
            endif()
		        else()
                    set(args ${args} --partition-device 2 --partition-index 1 --partition-size 0)
                    set(packargs ${packargs} --partition-device 2 --partition-index 1 --partition-size 0)
                    if (DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1)
                        set(lfsfatargs ${lfsfatargs} --partition-device 2 --partition-index 1 --partition-size 0)
		            endif()
		        endif()
        endif()

        if (DEFINED CFG_SD0_PARTITION0 AND DEFINED CFG_SD0_PARTITION1 AND DEFINED CFG_SD0_PARTITION2)
            if (DEFINED CFG_UPGRADE_PARTITION OR DEFINED CFG_UPGRADE_FORMAT_SD0_PARTITION2)
                set(args ${args} --partition-device 2 --partition-index 2 --partition-size ${CFG_SD0_PARTITION2_SIZE})
                set(packargs ${packargs} --partition-device 2 --partition-index 2 --partition-size ${CFG_SD0_PARTITION2_SIZE})
            else()
                set(args ${args} --partition-device 2 --partition-index 2 --partition-size 0)
                set(packargs ${packargs} --partition-device 2 --partition-index 2 --partition-size 0)
            endif()
        endif()

        if (DEFINED CFG_SD0_PARTITION0 AND DEFINED CFG_SD0_PARTITION1 AND DEFINED CFG_SD0_PARTITION2 AND DEFINED CFG_SD0_PARTITION3)
            set(args ${args} --partition-device 2 --partition-index 3 --partition-size ${CFG_SD0_PARTITION3_SIZE})
            set(packargs ${packargs} --partition-device 2 --partition-index 3 --partition-size ${CFG_SD0_PARTITION3_SIZE})
        endif()

        if (DEFINED CFG_SD1_PARTITION0)
            if (DEFINED CFG_UPGRADE_PARTITION OR DEFINED CFG_UPGRADE_FORMAT_SD1_PARTITION0)
                set(args ${args} --partition-device 3 --partition-index 0 --partition-size ${CFG_SD1_PARTITION0_SIZE})
                set(packargs ${packargs} --partition-device 3 --partition-index 0 --partition-size ${CFG_SD1_PARTITION0_SIZE})
                set(lfsfatargs ${lfsfatargs} --partition-device 3 --partition-index 0 --partition-size ${CFG_SD1_PARTITION0_SIZE})
            else()
                set(args ${args} --partition-device 3 --partition-index 0 --partition-size 0)
                set(packargs ${packargs} --partition-device 3 --partition-index 0 --partition-size 0)
                set(lfsfatargs ${lfsfatargs} --partition-device 3 --partition-index 0 --partition-size 0)
            endif()
        endif()

        if (DEFINED CFG_SD1_PARTITION0 AND DEFINED CFG_SD1_PARTITION1)
            if (DEFINED CFG_UPGRADE_PARTITION OR DEFINED CFG_UPGRADE_FORMAT_SD1_PARTITION1)
                set(args ${args} --partition-device 3 --partition-index 1 --partition-size ${CFG_SD1_PARTITION1_SIZE})
                set(packargs ${packargs} --partition-device 3 --partition-index 1 --partition-size ${CFG_SD1_PARTITION1_SIZE})
                if (DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1)
                    set(lfsfatargs ${lfsfatargs} --partition-device 3 --partition-index 1 --partition-size ${CFG_SD1_PARTITION1_SIZE})
            endif()
            else()
                set(args ${args} --partition-device 3 --partition-index 1 --partition-size 0)
                set(packargs ${packargs} --partition-device 3 --partition-index 1 --partition-size 0)
                if (DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1)
                    set(lfsfatargs ${lfsfatargs} --partition-device 3 --partition-index 1 --partition-size 0)
                endif()
            endif()
        endif()

        if (DEFINED CFG_SD1_PARTITION0 AND DEFINED CFG_SD1_PARTITION1 AND DEFINED CFG_SD1_PARTITION2)
            if (DEFINED CFG_UPGRADE_PARTITION OR DEFINED CFG_UPGRADE_FORMAT_SD1_PARTITION2)
                set(args ${args} --partition-device 3 --partition-index 2 --partition-size ${CFG_SD1_PARTITION2_SIZE})
                set(packargs ${packargs} --partition-device 3 --partition-index 2 --partition-size ${CFG_SD1_PARTITION2_SIZE})
            else()
                set(args ${args} --partition-device 3 --partition-index 2 --partition-size 0)
                set(packargs ${packargs} --partition-device 3 --partition-index 2 --partition-size 0)
            endif()
        endif()

        if (DEFINED CFG_SD1_PARTITION0 AND DEFINED CFG_SD1_PARTITION1 AND DEFINED CFG_SD1_PARTITION2 AND DEFINED CFG_SD1_PARTITION3)
            set(args ${args} --partition-device 3 --partition-index 3 --partition-size ${CFG_SD1_PARTITION3_SIZE})
            set(packargs ${packargs} --partition-device 3 --partition-index 3 --partition-size ${CFG_SD1_PARTITION3_SIZE})
        endif()

		if (DEFINED CFG_SD1_PARTITION0 AND DEFINED CFG_SD1_PARTITION1 AND DEFINED CFG_SD1_PARTITION2 AND DEFINED CFG_SD1_PARTITION3 AND DEFINED CFG_SD1_PARTITION4)
            set(args ${args} --partition-device 3 --partition-index 4 --partition-size ${CFG_SD1_PARTITION4_SIZE})
            set(packargs ${packargs} --partition-device 3 --partition-index 4 --partition-size ${CFG_SD1_PARTITION4_SIZE})
        endif()

        if (DEFINED CFG_SD1_PARTITION0 AND DEFINED CFG_SD1_PARTITION1 AND DEFINED CFG_SD1_PARTITION2 AND DEFINED CFG_SD1_PARTITION3 AND DEFINED CFG_SD1_PARTITION4 AND DEFINED CFG_SD1_PARTITION5)
            set(args ${args} --partition-device 3 --partition-index 5 --partition-size ${CFG_SD1_PARTITION5_SIZE})
            set(packargs ${packargs} --partition-device 3 --partition-index 5 --partition-size ${CFG_SD1_PARTITION5_SIZE})
        endif()

        if (DEFINED CFG_UPGRADE_PARTITION)
            set(args ${args} --partition)
            set(packargs ${packargs} --partition)
        endif()

        if (DEFINED CFG_UPGRADE_FORMAT_PARTITION)
            set(args ${args} --format-partition)
            set(packargs ${packargs} --format-partition)
        endif()
    endif()

    if (DEFINED CFG_NAND_ENABLE AND DEFINED CFG_UPGRADE_NAND_DATA_BY_DISK_IMAGE AND NOT (${CMAKE_PROJECT_NAME} STREQUAL bootloader))
        set(pkgdsk ${pkgdsk} --unformatted-device 0 --unformatted-index 3 --unformatted-data ${CFG_UPGRADE_NAND_DISK_IMAGE_FILENAME})
        set(pkgdsk ${pkgdsk} --unformatted-pos ${CFG_NAND_RESERVED_SIZE})
    endif()

    if (DEFINED CFG_UPGRADE_PACKAGE_VERSION)
        set(args ${args} --version ${CFG_UPGRADE_PACKAGE_VERSION})
        set(packargs ${packargs} --version ${CFG_UPGRADE_PACKAGE_VERSION})
        set(pkgdsk ${pkgdsk} --version ${CFG_UPGRADE_PACKAGE_VERSION})
    endif()

    if (DEFINED CFG_UPGRADE_CUSTOM_FLAGS)
        set(args ${args} --flags ${CFG_UPGRADE_CUSTOM_FLAGS})
        set(packargs ${packargs} --flags ${CFG_UPGRADE_CUSTOM_FLAGS})
        set(pkgdsk ${pkgdsk} --flags ${CFG_UPGRADE_CUSTOM_FLAGS})
    endif()

    if (DEFINED CFG_UPGRADE_BOOTLOADER OR DEFINED CFG_UPGRADE_IMAGE OR DEFINED CFG_UPGRADE_DATA)

        #message("pkgtool ${args} ${bootloader_args}")
        add_custom_command(
            TARGET ${CMAKE_PROJECT_NAME}
            POST_BUILD
            COMMAND pkgtool
            ARGS -o ${CFG_UPGRADE_FILENAME} ${args} ${bootloader_args} --key ${CFG_UPGRADE_ENC_KEY}
            COMMAND pkgtool
            ARGS -l ${CFG_UPGRADE_FILENAME}
        )

        # Difference Upgrade
        if ($ENV{CFG_PLATFORM} STREQUAL openrtos
            AND NOT (${CMAKE_PROJECT_NAME} STREQUAL "bootloader")
            AND NOT (${CMAKE_PROJECT_NAME} STREQUAL "sdk_standard_ngpl")
            AND NOT (${CMAKE_PROJECT_NAME} STREQUAL "sdk_standard_ngpl_intl"))
            # run diff.cmake
            add_custom_command(
                TARGET ${CMAKE_PROJECT_NAME}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND}
                ARGS
                    -DCMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}
                    -DCMAKE_PROJECT_NAME=${CMAKE_PROJECT_NAME}
                    -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
                    -DCMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
                    -P ${PROJECT_SOURCE_DIR}/sdk/target/diff/diff.cmake
                DEPENDS ${PROJECT_SOURCE_DIR}/build/openrtos/${CMAKE_PROJECT_NAME}/project/${CMAKE_PROJECT_NAME}/kproc.sys
            )
        endif()

        if (DEFINED CFG_FS_LFS AND (DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0 OR DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))
            #message("pkgtool ${lfsfatargs} ${bootloader_args}")
            add_custom_command(
                TARGET ${CMAKE_PROJECT_NAME}
                POST_BUILD
                COMMAND pkgtool
                ARGS -o ${CFG_UPGRADE_FILENAME}.fat0 ${lfsfatargs} ${bootloader_args} --key ${CFG_UPGRADE_ENC_KEY}
                COMMAND pkgtool
                ARGS -l ${CFG_UPGRADE_FILENAME}.fat0
            )
        endif()

        # pack NOR rom file
        if (DEFINED CFG_NOR_ENABLE AND DEFINED CFG_UPGRADE_NOR_IMAGE AND NOT (${CMAKE_PROJECT_NAME} STREQUAL bootloader))
            if (DEFINED CFG_FS_LFS AND (DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0 OR DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))
                add_custom_command(
                    TARGET ${CMAKE_PROJECT_NAME}
                    POST_BUILD
                    COMMAND pkgtool
                    ARGS -s ${CFG_UPGRADE_NOR_IMAGE_SIZE} -o ${CFG_UPGRADE_NOR_IMAGE_FILENAME} -n ${CFG_UPGRADE_FILENAME}.fat0
                )
            elseif(DEFINED CFG_BUILD_FILEX)
                if(DEFINED CFG_BUILD_LEVELX)
                    set(READONLY_ARGS "")
                    foreach(i RANGE 0 3)
                        set(partition_var "CFG_NOR_PARTITION${i}_READONLY")
                        if(DEFINED ${partition_var} AND NOT "${${partition_var}}" STREQUAL "0")
                            list(APPEND READONLY_ARGS --readonly-index ${i})
                        endif()
                    endforeach()

                    add_custom_command(
                        TARGET ${CMAKE_PROJECT_NAME}
                        POST_BUILD
                        COMMAND pkgtool
                        ARGS --filesystem-mode 2 ${READONLY_ARGS} -s ${CFG_UPGRADE_NOR_IMAGE_SIZE} -o ${CFG_UPGRADE_NOR_IMAGE_FILENAME} -n ${CFG_UPGRADE_FILENAME}
                    )
                else()
                add_custom_command(
                    TARGET ${CMAKE_PROJECT_NAME}
                    POST_BUILD
                    COMMAND pkgtool
                    ARGS --filesystem-mode 1 -s ${CFG_UPGRADE_NOR_IMAGE_SIZE} -o ${CFG_UPGRADE_NOR_IMAGE_FILENAME} -n ${CFG_UPGRADE_FILENAME}
                )
                endif()
            else()
                add_custom_command(
                    TARGET ${CMAKE_PROJECT_NAME}
                    POST_BUILD
                    COMMAND pkgtool
                    ARGS -s ${CFG_UPGRADE_NOR_IMAGE_SIZE} -o ${CFG_UPGRADE_NOR_IMAGE_FILENAME} -n ${CFG_UPGRADE_FILENAME}
                )
            endif()

            if (DEFINED CFG_FS_LFS)
                if (DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0)
                    if(DEFINED CFG_NOR_PARTITION1)
                        if(${CFG_NOR_PARTITION1_SIZE} STREQUAL "0")
                            math(EXPR CFG_NOR_PARTITION1_SIZE "${CFG_UPGRADE_NOR_IMAGE_SIZE}-${CFG_NOR_RESERVED_SIZE}-${CFG_NOR_PARTITION0_SIZE}-${CFG_LFS_FAT_GAP}")
                            set(CFG_NOR_PARTITION2_SIZE "0")
                            set(CFG_NOR_PARTITION3_SIZE "0")
                        else()
                            if(DEFINED CFG_NOR_PARTITION2)
                                if(${CFG_NOR_PARTITION2_SIZE} STREQUAL "0")
                                    math(EXPR CFG_NOR_PARTITION2_SIZE "${CFG_UPGRADE_NOR_IMAGE_SIZE}-${CFG_NOR_RESERVED_SIZE}-${CFG_NOR_PARTITION0_SIZE}-${CFG_NOR_PARTITION1_SIZE}-${CFG_LFS_FAT_GAP}")
                                    set(CFG_NOR_PARTITION3_SIZE "0")
                                else()
                                    if(DEFINED CFG_NOR_PARTITION3)
                                        if(${CFG_NOR_PARTITION3_SIZE} STREQUAL "0")
                                            math(EXPR CFG_NOR_PARTITION3_SIZE "${CFG_UPGRADE_NOR_IMAGE_SIZE}-${CFG_NOR_RESERVED_SIZE}-${CFG_NOR_PARTITION0_SIZE}-${CFG_NOR_PARTITION1_SIZE}-${CFG_NOR_PARTITION2_SIZE}-${CFG_LFS_FAT_GAP}")
                                        endif()
                                    else()
                                        set(CFG_NOR_PARTITION3_SIZE "0")
                                    endif()
                                endif()
                            else()
                                set(CFG_NOR_PARTITION2_SIZE "0")
                                set(CFG_NOR_PARTITION3_SIZE "0")
                            endif()
                        endif()

                        math(EXPR lfs_pos "${CFG_NOR_RESERVED_SIZE}+${CFG_NOR_PARTITION0_SIZE}+${CFG_LFS_FAT_GAP}")
                        message("--datapath=${CMAKE_BINARY_DIR}/data --target=${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/${CFG_UPGRADE_NOR_IMAGE_FILENAME} --reserversize=${lfs_pos} --p0=${CFG_NOR_PARTITION1_SIZE},${CMAKE_BINARY_DIR}/data/public --p1=${CFG_NOR_PARTITION2_SIZE},${CMAKE_BINARY_DIR}/data/temp --p2=${CFG_NOR_PARTITION3_SIZE},")

                        add_custom_command(
                            TARGET ${CMAKE_PROJECT_NAME}
                            POST_BUILD
                            COMMAND lfstool
                            ARGS --lfscmd=${PROJECT_SOURCE_DIR}/tool/bin --datapath=${CMAKE_BINARY_DIR}/data --bsize=65536 --rsize=256 --psize=256 --target=${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/${CFG_UPGRADE_NOR_IMAGE_FILENAME} --reserversize=${lfs_pos} --p0=${CFG_NOR_PARTITION1_SIZE},${CMAKE_BINARY_DIR}/data/public --p1=${CFG_NOR_PARTITION2_SIZE},${CMAKE_BINARY_DIR}/data/temp --p2=${CFG_NOR_PARTITION3_SIZE},
                        )
                    endif()
                elseif (DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1)
                    if(DEFINED CFG_NOR_PARTITION1)
                        if(${CFG_NOR_PARTITION1_SIZE} STREQUAL "0")
                            math(EXPR CFG_NOR_PARTITION1_SIZE "${CFG_UPGRADE_NOR_IMAGE_SIZE}-${CFG_NOR_RESERVED_SIZE}-${CFG_NOR_PARTITION0_SIZE}-${CFG_LFS_FAT_GAP}")
                            set(CFG_NOR_PARTITION2_SIZE "0")
                            set(CFG_NOR_PARTITION3_SIZE "0")
                        else()
                            if(DEFINED CFG_NOR_PARTITION2)
                                if(${CFG_NOR_PARTITION2_SIZE} STREQUAL "0")
                                    math(EXPR CFG_NOR_PARTITION2_SIZE "${CFG_UPGRADE_NOR_IMAGE_SIZE}-${CFG_NOR_RESERVED_SIZE}-${CFG_NOR_PARTITION0_SIZE}-${CFG_NOR_PARTITION1_SIZE}-${CFG_LFS_FAT_GAP}")
                                    set(CFG_NOR_PARTITION3_SIZE "0")
                                else()
                                    if(DEFINED CFG_NOR_PARTITION3)
                                        if(${CFG_NOR_PARTITION3_SIZE} STREQUAL "0")
                                            math(EXPR CFG_NOR_PARTITION3_SIZE "${CFG_UPGRADE_NOR_IMAGE_SIZE}-${CFG_NOR_RESERVED_SIZE}-${CFG_NOR_PARTITION0_SIZE}-${CFG_NOR_PARTITION1_SIZE}-${CFG_NOR_PARTITION2_SIZE}-${CFG_LFS_FAT_GAP}")
                                        endif()
                                    else()
                                        set(CFG_NOR_PARTITION3_SIZE "0")
                                    endif()
                                endif()
                            else()
                                set(CFG_NOR_PARTITION2_SIZE "0")
                                set(CFG_NOR_PARTITION3_SIZE "0")
                            endif()
                        endif()

                        math(EXPR lfs_pos "${CFG_NOR_RESERVED_SIZE}+${CFG_NOR_PARTITION0_SIZE}+${CFG_NOR_PARTITION1_SIZE}+${CFG_LFS_FAT_GAP}")
                        message("--datapath=${CMAKE_BINARY_DIR}/data --target=${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/${CFG_UPGRADE_NOR_IMAGE_FILENAME} --reserversize=${lfs_pos} --p0=${CFG_NOR_PARTITION2_SIZE},${CMAKE_BINARY_DIR}/data/temp --p1=${CFG_NOR_PARTITION3_SIZE},")

                        add_custom_command(
                            TARGET ${CMAKE_PROJECT_NAME}
                            POST_BUILD
                            COMMAND lfstool
                            ARGS --lfscmd=${PROJECT_SOURCE_DIR}/tool/bin --datapath=${CMAKE_BINARY_DIR}/data --bsize=65536 --rsize=256 --psize=256 --target=${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/${CFG_UPGRADE_NOR_IMAGE_FILENAME} --reserversize=${lfs_pos} --p0=${CFG_NOR_PARTITION2_SIZE},${CMAKE_BINARY_DIR}/data/temp --p1=${CFG_NOR_PARTITION3_SIZE},
                        )
                    endif()
                else()
                    if(DEFINED CFG_NOR_PARTITION0)
                        if(${CFG_NOR_PARTITION0_SIZE} STREQUAL "0")
                            math(EXPR CFG_NOR_PARTITION0_SIZE "${CFG_UPGRADE_NOR_IMAGE_SIZE}-${CFG_NOR_RESERVED_SIZE}")
                            set(CFG_NOR_PARTITION1_SIZE "0")
                            set(CFG_NOR_PARTITION2_SIZE "0")
                            set(CFG_NOR_PARTITION3_SIZE "0")
                        else()
                            if(DEFINED CFG_NOR_PARTITION1)
                                if(${CFG_NOR_PARTITION1_SIZE} STREQUAL "0")
                                    math(EXPR CFG_NOR_PARTITION1_SIZE "${CFG_UPGRADE_NOR_IMAGE_SIZE}-${CFG_NOR_RESERVED_SIZE}-${CFG_NOR_PARTITION0_SIZE}")
                                    set(CFG_NOR_PARTITION2_SIZE "0")
                                    set(CFG_NOR_PARTITION3_SIZE "0")
                                else()
                                    if(DEFINED CFG_NOR_PARTITION2)
                                        if(${CFG_NOR_PARTITION2_SIZE} STREQUAL "0")
                                            math(EXPR CFG_NOR_PARTITION2_SIZE "${CFG_UPGRADE_NOR_IMAGE_SIZE}-${CFG_NOR_RESERVED_SIZE}-${CFG_NOR_PARTITION0_SIZE}-${CFG_NOR_PARTITION1_SIZE}")
                                            set(CFG_NOR_PARTITION3_SIZE "0")
                                        else()
                                            if(DEFINED CFG_NOR_PARTITION3)
                                                if(${CFG_NOR_PARTITION3_SIZE} STREQUAL "0")
                                                    math(EXPR CFG_NOR_PARTITION3_SIZE "${CFG_UPGRADE_NOR_IMAGE_SIZE}-${CFG_NOR_RESERVED_SIZE}-${CFG_NOR_PARTITION0_SIZE}-${CFG_NOR_PARTITION1_SIZE}-${CFG_NOR_PARTITION2_SIZE}")
                                                endif()
                                            else()
                                                set(CFG_NOR_PARTITION3_SIZE "0")
                                            endif()
                                        endif()
                                    else()
                                        set(CFG_NOR_PARTITION2_SIZE "0")
                                        set(CFG_NOR_PARTITION3_SIZE "0")
                                    endif()
                                endif()
                            else()
                                set(CFG_NOR_PARTITION1_SIZE "0")
                                set(CFG_NOR_PARTITION2_SIZE "0")
                                set(CFG_NOR_PARTITION3_SIZE "0")
                            endif()
                        endif()
                    else()
                        set(CFG_NOR_PARTITION0_SIZE "0")
                        set(CFG_NOR_PARTITION1_SIZE "0")
                        set(CFG_NOR_PARTITION2_SIZE "0")
                        set(CFG_NOR_PARTITION3_SIZE "0")
                    endif()

                    message("--datapath=${CMAKE_BINARY_DIR}/data --target=${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/${CFG_UPGRADE_NOR_IMAGE_FILENAME} --reserversize=${CFG_NOR_RESERVED_SIZE} --p0=${CFG_NOR_PARTITION0_SIZE},${CMAKE_BINARY_DIR}/data/private --p1=${CFG_NOR_PARTITION1_SIZE},${CMAKE_BINARY_DIR}/data/public --p2=${CFG_NOR_PARTITION2_SIZE},${CMAKE_BINARY_DIR}/data/temp --p3=${CFG_NOR_PARTITION3_SIZE},")

                    add_custom_command(
                        TARGET ${CMAKE_PROJECT_NAME}
                        POST_BUILD
                        COMMAND lfstool
                        ARGS --lfscmd=${PROJECT_SOURCE_DIR}/tool/bin --datapath=${CMAKE_BINARY_DIR}/data --bsize=65536 --rsize=256 --psize=256 --target=${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/${CFG_UPGRADE_NOR_IMAGE_FILENAME} --reserversize=${CFG_NOR_RESERVED_SIZE} --p0=${CFG_NOR_PARTITION0_SIZE},${CMAKE_BINARY_DIR}/data/private --p1=${CFG_NOR_PARTITION1_SIZE},${CMAKE_BINARY_DIR}/data/public --p2=${CFG_NOR_PARTITION2_SIZE},${CMAKE_BINARY_DIR}/data/temp --p3=${CFG_NOR_PARTITION3_SIZE},
                    )
                endif()
            endif()

            if (DEFINED CFG_UPGRADE_BACKUP_PACKAGE)
                #message("pkgtool ${args}")
                add_custom_command(
                    TARGET ${CMAKE_PROJECT_NAME}
                    POST_BUILD
                    COMMAND pkgtool
                    ARGS -o ${CFG_UPGRADE_FILENAME}_NO_BOOTLOADER ${args} --key ${CFG_UPGRADE_ENC_KEY}
                    COMMAND pkgtool
                    ARGS -l ${CFG_UPGRADE_FILENAME}_NO_BOOTLOADER
                    COMMAND ${CMAKE_COMMAND} -E rename
                    ARGS ${CFG_UPGRADE_NOR_IMAGE_FILENAME} ${CFG_UPGRADE_NOR_IMAGE_FILENAME}_ORG
                    COMMAND fsw
                    ARGS ${CFG_UPGRADE_FILENAME}_NO_BOOTLOADER ${CFG_UPGRADE_NOR_IMAGE_FILENAME}_ORG ${CFG_UPGRADE_NOR_IMAGE_FILENAME} ${CFG_UPGRADE_BACKUP_PACKAGE_POS}
                )
            endif()

        endif()
        
        # pack NAND rom file
        if (DEFINED CFG_NAND_ENABLE AND DEFINED CFG_YAFFS2_GENERATE_ROM_FOR_PROGRAMMER AND NOT (${CMAKE_PROJECT_NAME} STREQUAL bootloader))
            add_custom_command(
                TARGET ${CMAKE_PROJECT_NAME}
                POST_BUILD
                COMMAND pkgtool
                ARGS -o ${CFG_YAFFS2_UPGRADE_NAND_PROGRAMMER_FILENAME} --filesystem-mode 3 --page-size ${CFG_NAND_PAGE_SIZE} --pages-per-block ${CFG_NAND_BLOCK_SIZE} -r ${CFG_UPGRADE_FILENAME} -s ${CFG_YAFFS2_ESTIMATED_DISK_SIZE}
            )
        endif()
        
        if (DEFINED CFG_NAND_ENABLE AND DEFINED CFG_UPGRADE_NAND_DATA_BY_DISK_IMAGE AND NOT (${CMAKE_PROJECT_NAME} STREQUAL bootloader))
            if (DEFINED CFG_UPGRADE_BACKUP_PACKAGE AND NOT DEFINED CFG_UPGRADE_BACKUP_PACKAGE_EXT_FILE)
                message("Gen backup PKG-file without bootloader")
                #message("pkgtool ${args}")
                add_custom_command(
                    TARGET ${CMAKE_PROJECT_NAME}
                    POST_BUILD
                    COMMAND pkgtool
                    ARGS -o ${CFG_UPGRADE_FILENAME}_NO_BOOTLOADER ${args} --key ${CFG_UPGRADE_ENC_KEY}
                    COMMAND pkgtool
                    ARGS -l ${CFG_UPGRADE_FILENAME}_NO_BOOTLOADER
                    COMMAND ${CMAKE_COMMAND} -E rename
                    ARGS ${CFG_UPGRADE_FILENAME}_NO_BOOTLOADER ${CFG_UPGRADE_FILENAME}_ORG
                )
            endif()

            # parse partitions as disk image
            if (DEFINED CFG_FS_LFS)
                message("BUILD nand disk image")
                if(${CFG_NAND_PARTITION3_SIZE} STREQUAL "0")
                    math(EXPR CFG_NAND_PARTITION3_SIZE "${CFG_UPGRADE_NAND_DISK_IMAGE_SIZE}-${CFG_NAND_RESERVED_SIZE}-${CFG_NAND_PARTITION0_SIZE}-${CFG_NAND_PARTITION1_SIZE}-${CFG_NAND_PARTITION2_SIZE}")
                endif()

                unset(nand_lfs_blk_size)
                unset(nand_total_blk_cnt)
                unset(nand_max_bad_cnt)

                set(nand_total_blk_cnt 1024)

                math(EXPR nand_max_bad_cnt "${nand_total_blk_cnt}/50")
                math(EXPR nand_lfs_blk_size "${CFG_NAND_PAGE_SIZE}*${CFG_NAND_BLOCK_SIZE}")

                #calculate the CFG_NAND_PARTITION3_SIZE for deduction of max bad blocks and 40-free blocks
                math(EXPR CFG_NAND_PARTITION3_SIZE "${CFG_NAND_PARTITION3_SIZE}-${nand_lfs_blk_size}*40-${nand_lfs_blk_size}*${nand_max_bad_cnt}")

                #message("LFS info: block-size: ${nand_lfs_blk_size}, partition3-size: ${CFG_NAND_PARTITION3_SIZE}")
                #message("LFS info: MaxBadCnt: ${nand_max_bad_cnt}, TotalblkCnt: ${nand_total_blk_cnt}")

                #delete and then build
                add_custom_command(
                    TARGET ${CMAKE_PROJECT_NAME}
                    POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E remove
                       ARGS ${CFG_UPGRADE_NAND_DISK_IMAGE_FILENAME}
                    COMMAND lfstool
                    ARGS --idoffset=0 --bsize=${nand_lfs_blk_size} --rsize=${CFG_NAND_PAGE_SIZE} --psize=${CFG_NAND_PAGE_SIZE} --target=${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/${CFG_UPGRADE_NAND_DISK_IMAGE_FILENAME} --datapath=${CMAKE_BINARY_DIR}/data --reserversize=${CFG_NAND_RESERVED_SIZE} --shrink=3  --p0=${CFG_NAND_PARTITION0_SIZE},${CMAKE_BINARY_DIR}/data/private --p1=${CFG_NAND_PARTITION1_SIZE},${CMAKE_BINARY_DIR}/data/public --p2=${CFG_NAND_PARTITION2_SIZE},${CMAKE_BINARY_DIR}/data/temp --p3=${CFG_NAND_PARTITION3_SIZE},
                )
            else()
                add_custom_command(
                    TARGET ${CMAKE_PROJECT_NAME}
                    POST_BUILD
                    COMMAND pkgtool
                    ARGS -t -o ${CFG_UPGRADE_NAND_DISK_IMAGE_FILENAME} -u ${CFG_UPGRADE_FILENAME} -e ${CFG_UPGRADE_NAND_DISK_SECTOR_SIZE} -s ${CFG_UPGRADE_NAND_DISK_IMAGE_SIZE}
                )
            endif()

            if (DEFINED CFG_UPGRADE_BACKUP_PACKAGE)
                if (DEFINED CFG_UPGRADE_BACKUP_PACKAGE_EXT_FILE)
                    set(pkgdsk ${pkgdsk} --unformatted-device 0 --unformatted-index 0 --unformatted-data ${CFG_UPGRADE_BACKUP_PKG_EXT_FILENAME})
                    set(pkgdsk ${pkgdsk} --unformatted-pos ${CFG_UPGRADE_BACKUP_PACKAGE_POS})
                else()
                    set(pkgdsk ${pkgdsk} --unformatted-device 0 --unformatted-index 0 --unformatted-data ${CFG_UPGRADE_FILENAME}_ORG)
                    set(pkgdsk ${pkgdsk} --unformatted-pos ${CFG_UPGRADE_BACKUP_PACKAGE_POS})
                endif()
            endif()

            # build PKG file with disk image
            add_custom_command(
                TARGET ${CMAKE_PROJECT_NAME}
                POST_BUILD
                COMMAND pkgtool
                ARGS -o ${CFG_UPGRADE_FILENAME} ${pkgdsk} --key ${CFG_UPGRADE_ENC_KEY}
                COMMAND pkgtool
                ARGS -l ${CFG_UPGRADE_FILENAME}
            )

            # convert PKG file to ROM format for NAND progermmer
            if (DEFINED CFG_UPGRADE_FILE_FOR_NAND_PROGRAMMER)
                add_custom_command(
                    TARGET ${CMAKE_PROJECT_NAME}
                    POST_BUILD
                    COMMAND dataconv
                    ARGS -p ${CFG_UPGRADE_FILENAME} -o ${CFG_UPGRADE_NAND_PROGRAMMER_FILENAME}
                )
            endif()
        endif()

        # pack SD0 rom file
        if (DEFINED CFG_SD0_ENABLE AND DEFINED CFG_UPGRADE_SD0_IMAGE AND NOT (${CMAKE_PROJECT_NAME} STREQUAL bootloader))
            if (DEFINED CFG_FS_LFS AND (DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0 OR DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))
                add_custom_command(
                    TARGET ${CMAKE_PROJECT_NAME}
                    POST_BUILD
                    COMMAND pkgtool
                    ARGS -s ${CFG_UPGRADE_SD0_IMAGE_SIZE} -o ${CFG_UPGRADE_SD0_IMAGE_FILENAME} -w ${CFG_UPGRADE_FILENAME}.fat0
                )
            elseif(DEFINED CFG_BUILD_FILEX)
                if(DEFINED CFG_BUILD_LEVELX)
                    set(READONLY_ARGS "")
                    foreach(i RANGE 0 3)
                        set(partition_var "CFG_NOR_PARTITION${i}_READONLY")
                        if(DEFINED ${partition_var} AND NOT "${${partition_var}}" STREQUAL "0")
                            list(APPEND READONLY_ARGS --readonly-index ${i})
                        endif()
                    endforeach()

                    add_custom_command(
                        TARGET ${CMAKE_PROJECT_NAME}
                        POST_BUILD
                        COMMAND pkgtool
                        ARGS --filesystem-mode 2 ${READONLY_ARGS} -s ${CFG_UPGRADE_SD0_IMAGE_SIZE} -o ${CFG_UPGRADE_SD0_IMAGE_FILENAME} -w ${CFG_UPGRADE_FILENAME}
                    )
                else()
                add_custom_command(
                    TARGET ${CMAKE_PROJECT_NAME}
                    POST_BUILD
                    COMMAND pkgtool
                    ARGS --filesystem-mode 1 -s ${CFG_UPGRADE_SD0_IMAGE_SIZE} -o ${CFG_UPGRADE_SD0_IMAGE_FILENAME} -w ${CFG_UPGRADE_FILENAME}
                )
                endif()
            else()
                add_custom_command(
                    TARGET ${CMAKE_PROJECT_NAME}
                    POST_BUILD
                    COMMAND pkgtool
                    ARGS -s ${CFG_UPGRADE_SD0_IMAGE_SIZE} -o ${CFG_UPGRADE_SD0_IMAGE_FILENAME} -w ${CFG_UPGRADE_FILENAME}
                )
            endif()

            if (DEFINED CFG_FS_LFS)
                if (DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0)
                    if(DEFINED CFG_SD0_PARTITION0)
                        if(${CFG_SD0_PARTITION1_SIZE} STREQUAL "0")
                            math(EXPR CFG_SD0_PARTITION1_SIZE "${CFG_UPGRADE_SD0_IMAGE_SIZE}-${CFG_SD0_RESERVED_SIZE}-${CFG_SD0_PARTITION0_SIZE}-${CFG_LFS_FAT_GAP}")
                            set(CFG_SD0_PARTITION2_SIZE "0")
                            set(CFG_SD0_PARTITION3_SIZE "0")
                        else()
                            if(DEFINED CFG_SD0_PARTITION2)
                                if(${CFG_SD0_PARTITION2_SIZE} STREQUAL "0")
                                    math(EXPR CFG_SD0_PARTITION2_SIZE "${CFG_UPGRADE_SD0_IMAGE_SIZE}-${CFG_SD0_RESERVED_SIZE}-${CFG_SD0_PARTITION0_SIZE}-${CFG_SD0_PARTITION1_SIZE}-${CFG_LFS_FAT_GAP}")
                                    set(CFG_SD0_PARTITION3_SIZE "0")
                                else()
                                    if(DEFINED CFG_SD0_PARTITION3)
                                        if(${CFG_SD0_PARTITION3_SIZE} STREQUAL "0")
                                            math(EXPR CFG_SD0_PARTITION3_SIZE "${CFG_UPGRADE_SD0_IMAGE_SIZE}-${CFG_SD0_RESERVED_SIZE}-${CFG_SD0_PARTITION0_SIZE}-${CFG_SD0_PARTITION1_SIZE}-${CFG_SD0_PARTITION2_SIZE}-${CFG_LFS_FAT_GAP}")
                                        endif()
                                    else()
                                        set(CFG_SD0_PARTITION3_SIZE "0")
                                    endif()
                                endif()
                            else()
                                set(CFG_SD0_PARTITION2_SIZE "0")
                                set(CFG_SD0_PARTITION3_SIZE "0")
                            endif()
                        endif()

                        math(EXPR lfs_pos "${CFG_SD0_RESERVED_SIZE}+${CFG_SD0_PARTITION0_SIZE}+${CFG_LFS_FAT_GAP}")
                        message("--datapath=${CMAKE_BINARY_DIR}/data --target=${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/${CFG_UPGRADE_SD0_IMAGE_FILENAME} --reserversize=${lfs_pos} --p0=${CFG_SD0_PARTITION1_SIZE},${CMAKE_BINARY_DIR}/data/public --p1=${CFG_SD0_PARTITION2_SIZE},${CMAKE_BINARY_DIR}/data/temp --p2=${CFG_SD0_PARTITION3_SIZE},")

                        add_custom_command(
                            TARGET ${CMAKE_PROJECT_NAME}
                            POST_BUILD
                            COMMAND lfstool
                            ARGS --lfscmd=${PROJECT_SOURCE_DIR}/tool/bin --datapath=${CMAKE_BINARY_DIR}/data --bsize=65536 --rsize=512 --psize=512 --target=${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/${CFG_UPGRADE_SD0_IMAGE_FILENAME} --reserversize=${lfs_pos} --p0=${CFG_SD0_PARTITION1_SIZE},${CMAKE_BINARY_DIR}/data/public --p1=${CFG_SD0_PARTITION2_SIZE},${CMAKE_BINARY_DIR}/data/temp --p2=${CFG_SD0_PARTITION3_SIZE},
                        )
                    endif()
                elseif (DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1)
                    if(DEFINED CFG_SD0_PARTITION0)
                        if(${CFG_SD0_PARTITION1_SIZE} STREQUAL "0")
                            math(EXPR CFG_SD0_PARTITION1_SIZE "${CFG_UPGRADE_SD0_IMAGE_SIZE}-${CFG_SD0_RESERVED_SIZE}-${CFG_SD0_PARTITION0_SIZE}-${CFG_LFS_FAT_GAP}")
                            set(CFG_SD0_PARTITION2_SIZE "0")
                            set(CFG_SD0_PARTITION3_SIZE "0")
                        else()
                            if(DEFINED CFG_SD0_PARTITION2)
                                if(${CFG_SD0_PARTITION2_SIZE} STREQUAL "0")
                                    math(EXPR CFG_SD0_PARTITION2_SIZE "${CFG_UPGRADE_SD0_IMAGE_SIZE}-${CFG_SD0_RESERVED_SIZE}-${CFG_SD0_PARTITION0_SIZE}-${CFG_SD0_PARTITION1_SIZE}-${CFG_LFS_FAT_GAP}")
                                    set(CFG_SD0_PARTITION3_SIZE "0")
                                else()
                                    if(DEFINED CFG_SD0_PARTITION3)
                                        if(${CFG_SD0_PARTITION3_SIZE} STREQUAL "0")
                                            math(EXPR CFG_SD0_PARTITION3_SIZE "${CFG_UPGRADE_SD0_IMAGE_SIZE}-${CFG_SD0_RESERVED_SIZE}-${CFG_SD0_PARTITION0_SIZE}-${CFG_SD0_PARTITION1_SIZE}-${CFG_SD0_PARTITION2_SIZE}-${CFG_LFS_FAT_GAP}")
                                        endif()
                                    else()
                                        set(CFG_SD0_PARTITION3_SIZE "0")
                                    endif()
                                endif()
                            else()
                                set(CFG_SD0_PARTITION2_SIZE "0")
                                set(CFG_SD0_PARTITION3_SIZE "0")
                            endif()
                        endif()

                        math(EXPR lfs_pos "${CFG_SD0_RESERVED_SIZE}+${CFG_SD0_PARTITION0_SIZE}+${CFG_SD0_PARTITION1_SIZE}+${CFG_LFS_FAT_GAP}")
                        message("--datapath=${CMAKE_BINARY_DIR}/data --target=${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/${CFG_UPGRADE_SD0_IMAGE_FILENAME} --reserversize=${lfs_pos} --p0=${CFG_SD0_PARTITION2_SIZE},${CMAKE_BINARY_DIR}/data/temp --p1=${CFG_SD0_PARTITION3_SIZE},")

                        add_custom_command(
                            TARGET ${CMAKE_PROJECT_NAME}
                            POST_BUILD
                            COMMAND lfstool
                            ARGS --lfscmd=${PROJECT_SOURCE_DIR}/tool/bin --datapath=${CMAKE_BINARY_DIR}/data --bsize=65536 --rsize=512 --psize=512 --target=${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/${CFG_UPGRADE_SD0_IMAGE_FILENAME} --reserversize=${lfs_pos} --p0=${CFG_SD0_PARTITION2_SIZE},${CMAKE_BINARY_DIR}/data/temp --p1=${CFG_SD0_PARTITION3_SIZE},
                        )
                    endif()
                else()
                    if(DEFINED CFG_SD0_PARTITION0)
                        if(${CFG_SD0_PARTITION0_SIZE} STREQUAL "0")
                            math(EXPR CFG_SD0_PARTITION0_SIZE "${CFG_UPGRADE_SD0_IMAGE_SIZE}-${CFG_SD0_RESERVED_SIZE}")
                            set(CFG_SD0_PARTITION1_SIZE "0")
                            set(CFG_SD0_PARTITION2_SIZE "0")
                            set(CFG_SD0_PARTITION3_SIZE "0")
                        else()
                            if(DEFINED CFG_SD0_PARTITION1)
                                if(${CFG_SD0_PARTITION1_SIZE} STREQUAL "0")
                                    math(EXPR CFG_SD0_PARTITION1_SIZE "${CFG_UPGRADE_SD0_IMAGE_SIZE}-${CFG_SD0_RESERVED_SIZE}-${CFG_SD0_PARTITION0_SIZE}")
                                    set(CFG_SD0_PARTITION2_SIZE "0")
                                    set(CFG_SD0_PARTITION3_SIZE "0")
                                else()
                                    if(DEFINED CFG_SD0_PARTITION2)
                                        if(${CFG_SD0_PARTITION2_SIZE} STREQUAL "0")
                                            math(EXPR CFG_SD0_PARTITION2_SIZE "${CFG_UPGRADE_SD0_IMAGE_SIZE}-${CFG_SD0_RESERVED_SIZE}-${CFG_SD0_PARTITION0_SIZE}-${CFG_SD0_PARTITION1_SIZE}")
                                            set(CFG_SD0_PARTITION3_SIZE "0")
                                        else()
                                            if(DEFINED CFG_SD0_PARTITION3)
                                                if(${CFG_SD0_PARTITION3_SIZE} STREQUAL "0")
                                                    math(EXPR CFG_SD0_PARTITION3_SIZE "${CFG_UPGRADE_SD0_IMAGE_SIZE}-${CFG_SD0_RESERVED_SIZE}-${CFG_SD0_PARTITION0_SIZE}-${CFG_SD0_PARTITION1_SIZE}-${CFG_SD0_PARTITION2_SIZE}")
                                                endif()
                                            else()
                                                set(CFG_SD0_PARTITION3_SIZE "0")
                                            endif()
                                        endif()
                                    else()
                                        set(CFG_SD0_PARTITION2_SIZE "0")
                                        set(CFG_SD0_PARTITION3_SIZE "0")
                                    endif()
                                endif()
                            else()
                                set(CFG_SD0_PARTITION1_SIZE "0")
                                set(CFG_SD0_PARTITION2_SIZE "0")
                                set(CFG_SD0_PARTITION3_SIZE "0")
                            endif()
                        endif()
                    else()
                        set(CFG_SD0_PARTITION0_SIZE "0")
                        set(CFG_SD0_PARTITION1_SIZE "0")
                        set(CFG_SD0_PARTITION2_SIZE "0")
                        set(CFG_SD0_PARTITION3_SIZE "0")
                    endif()

                    message("--datapath=${CMAKE_BINARY_DIR}/data --target=${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/${CFG_UPGRADE_SD0_IMAGE_FILENAME} --reserversize=${CFG_SD0_RESERVED_SIZE} --p0=${CFG_SD0_PARTITION0_SIZE},${CMAKE_BINARY_DIR}/data/private --p1=${CFG_SD0_PARTITION1_SIZE},${CMAKE_BINARY_DIR}/data/public --p2=${CFG_SD0_PARTITION2_SIZE},${CMAKE_BINARY_DIR}/data/temp --p3=${CFG_SD0_PARTITION3_SIZE},")

                    add_custom_command(
                        TARGET ${CMAKE_PROJECT_NAME}
                        POST_BUILD
                        COMMAND lfstool
                        ARGS --lfscmd=${PROJECT_SOURCE_DIR}/tool/bin --datapath=${CMAKE_BINARY_DIR}/data --bsize=65536 --rsize=512 --psize=512 --target=${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/${CFG_UPGRADE_SD0_IMAGE_FILENAME} --reserversize=${CFG_SD0_RESERVED_SIZE} --p0=${CFG_SD0_PARTITION0_SIZE},${CMAKE_BINARY_DIR}/data/private --p1=${CFG_SD0_PARTITION1_SIZE},${CMAKE_BINARY_DIR}/data/public --p2=${CFG_SD0_PARTITION2_SIZE},${CMAKE_BINARY_DIR}/data/temp --p3=${CFG_SD0_PARTITION3_SIZE},
                    )
                endif()
            endif()

            if (DEFINED CFG_UPGRADE_BACKUP_PACKAGE)
                #message("pkgtool ${args}")
                add_custom_command(
                    TARGET ${CMAKE_PROJECT_NAME}
                    POST_BUILD
                    COMMAND pkgtool
                    ARGS -o ${CFG_UPGRADE_FILENAME}_NO_BOOTLOADER ${args} --key ${CFG_UPGRADE_ENC_KEY}
                    COMMAND pkgtool
                    ARGS -l ${CFG_UPGRADE_FILENAME}_NO_BOOTLOADER
                    COMMAND ${CMAKE_COMMAND} -E rename
                    ARGS ${CFG_UPGRADE_SD0_IMAGE_FILENAME} ${CFG_UPGRADE_SD0_IMAGE_FILENAME}_ORG
                    COMMAND fsw
                    ARGS ${CFG_UPGRADE_FILENAME}_NO_BOOTLOADER ${CFG_UPGRADE_SD0_IMAGE_FILENAME}_ORG ${CFG_UPGRADE_SD0_IMAGE_FILENAME} ${CFG_UPGRADE_BACKUP_PACKAGE_POS}
                )
            endif()

        endif()

        # pack SD1 rom file
        if (DEFINED CFG_SD1_ENABLE AND DEFINED CFG_UPGRADE_SD1_IMAGE AND NOT (${CMAKE_PROJECT_NAME} STREQUAL bootloader))
            if (DEFINED CFG_FS_LFS AND (DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0 OR DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))
                add_custom_command(
                    TARGET ${CMAKE_PROJECT_NAME}
                    POST_BUILD
                    COMMAND pkgtool
                    ARGS -s ${CFG_UPGRADE_SD1_IMAGE_SIZE} -o ${CFG_UPGRADE_SD1_IMAGE_FILENAME} -x ${CFG_UPGRADE_FILENAME}.fat0
                )
            elseif(DEFINED CFG_BUILD_FILEX)
                if(DEFINED CFG_BUILD_LEVELX)
                    set(READONLY_ARGS "")
                    foreach(i RANGE 0 3)
                        set(partition_var "CFG_NOR_PARTITION${i}_READONLY")
                        if(DEFINED ${partition_var} AND NOT "${${partition_var}}" STREQUAL "0")
                            list(APPEND READONLY_ARGS --readonly-index ${i})
                        endif()
                    endforeach()

                    add_custom_command(
                        TARGET ${CMAKE_PROJECT_NAME}
                        POST_BUILD
                        COMMAND pkgtool
                        ARGS --filesystem-mode 2 ${READONLY_ARGS} -s ${CFG_UPGRADE_SD1_IMAGE_SIZE} -o ${CFG_UPGRADE_SD1_IMAGE_FILENAME} -x ${CFG_UPGRADE_FILENAME}
                    )
                else()
                add_custom_command(
                    TARGET ${CMAKE_PROJECT_NAME}
                    POST_BUILD
                    COMMAND pkgtool
                    ARGS --filesystem-mode 1 -s ${CFG_UPGRADE_SD1_IMAGE_SIZE} -o ${CFG_UPGRADE_SD1_IMAGE_FILENAME} -x ${CFG_UPGRADE_FILENAME}
                )
                endif()
            else()
                add_custom_command(
                    TARGET ${CMAKE_PROJECT_NAME}
                    POST_BUILD
                    COMMAND pkgtool
                    ARGS -s ${CFG_UPGRADE_SD1_IMAGE_SIZE} -o ${CFG_UPGRADE_SD1_IMAGE_FILENAME} -x ${CFG_UPGRADE_FILENAME}
                )
            endif()

            if (DEFINED CFG_FS_LFS)
                if (DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0)
                    if(DEFINED CFG_SD1_PARTITION0)
                        if(${CFG_SD1_PARTITION1_SIZE} STREQUAL "0")
                            math(EXPR CFG_SD1_PARTITION1_SIZE "${CFG_UPGRADE_SD1_IMAGE_SIZE}-${CFG_SD1_RESERVED_SIZE}-${CFG_SD1_PARTITION0_SIZE}-${CFG_LFS_FAT_GAP}")
                            set(CFG_SD1_PARTITION2_SIZE "0")
                            set(CFG_SD1_PARTITION3_SIZE "0")
                        else()
                            if(DEFINED CFG_SD1_PARTITION2)
                                if(${CFG_SD1_PARTITION2_SIZE} STREQUAL "0")
                                    math(EXPR CFG_SD1_PARTITION2_SIZE "${CFG_UPGRADE_SD1_IMAGE_SIZE}-${CFG_SD1_RESERVED_SIZE}-${CFG_SD1_PARTITION0_SIZE}-${CFG_SD1_PARTITION1_SIZE}-${CFG_LFS_FAT_GAP}")
                                    set(CFG_SD1_PARTITION3_SIZE "0")
                                else()
                                    if(DEFINED CFG_SD1_PARTITION3)
                                        if(${CFG_SD1_PARTITION3_SIZE} STREQUAL "0")
                                            math(EXPR CFG_SD1_PARTITION3_SIZE "${CFG_UPGRADE_SD1_IMAGE_SIZE}-${CFG_SD1_RESERVED_SIZE}-${CFG_SD1_PARTITION0_SIZE}-${CFG_SD1_PARTITION1_SIZE}-${CFG_SD1_PARTITION2_SIZE}-${CFG_LFS_FAT_GAP}")
                                        endif()
                                    else()
                                        set(CFG_SD1_PARTITION3_SIZE "0")
                                    endif()
                                endif()
                            else()
                                set(CFG_SD1_PARTITION2_SIZE "0")
                                set(CFG_SD1_PARTITION3_SIZE "0")
                            endif()
                        endif()

                        math(EXPR lfs_pos "${CFG_SD1_RESERVED_SIZE}+${CFG_SD1_PARTITION0_SIZE}+${CFG_LFS_FAT_GAP}")
                        message("--datapath=${CMAKE_BINARY_DIR}/data --target=${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/${CFG_UPGRADE_SD1_IMAGE_FILENAME} --reserversize=${lfs_pos} --p0=${CFG_SD1_PARTITION1_SIZE},${CMAKE_BINARY_DIR}/data/public --p1=${CFG_SD1_PARTITION2_SIZE},${CMAKE_BINARY_DIR}/data/temp --p2=${CFG_SD1_PARTITION3_SIZE},")

                        add_custom_command(
                            TARGET ${CMAKE_PROJECT_NAME}
                            POST_BUILD
                            COMMAND lfstool
                            ARGS --lfscmd=${PROJECT_SOURCE_DIR}/tool/bin --datapath=${CMAKE_BINARY_DIR}/data --bsize=65536 --rsize=512 --psize=512 --target=${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/${CFG_UPGRADE_SD1_IMAGE_FILENAME} --reserversize=${lfs_pos} --p0=${CFG_SD1_PARTITION1_SIZE},${CMAKE_BINARY_DIR}/data/public --p1=${CFG_SD1_PARTITION2_SIZE},${CMAKE_BINARY_DIR}/data/temp --p2=${CFG_SD1_PARTITION3_SIZE},
                        )
                    endif()
                elseif (DEFINED CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1)
                    if(DEFINED CFG_SD1_PARTITION0)
                        if(${CFG_SD1_PARTITION1_SIZE} STREQUAL "0")
                            math(EXPR CFG_SD1_PARTITION1_SIZE "${CFG_UPGRADE_SD1_IMAGE_SIZE}-${CFG_SD1_RESERVED_SIZE}-${CFG_SD1_PARTITION0_SIZE}-${CFG_LFS_FAT_GAP}")
                            set(CFG_SD1_PARTITION2_SIZE "0")
                            set(CFG_SD1_PARTITION3_SIZE "0")
                        else()
                            if(DEFINED CFG_SD1_PARTITION2)
                                if(${CFG_SD1_PARTITION2_SIZE} STREQUAL "0")
                                    math(EXPR CFG_SD1_PARTITION2_SIZE "${CFG_UPGRADE_SD1_IMAGE_SIZE}-${CFG_SD1_RESERVED_SIZE}-${CFG_SD1_PARTITION0_SIZE}-${CFG_SD1_PARTITION1_SIZE}-${CFG_LFS_FAT_GAP}")
                                    set(CFG_SD1_PARTITION3_SIZE "0")
                                else()
                                    if(DEFINED CFG_SD1_PARTITION3)
                                        if(${CFG_SD1_PARTITION3_SIZE} STREQUAL "0")
                                            math(EXPR CFG_SD1_PARTITION3_SIZE "${CFG_UPGRADE_SD1_IMAGE_SIZE}-${CFG_SD1_RESERVED_SIZE}-${CFG_SD1_PARTITION0_SIZE}-${CFG_SD1_PARTITION1_SIZE}-${CFG_SD1_PARTITION2_SIZE}-${CFG_LFS_FAT_GAP}")
                                        endif()
                                    else()
                                        set(CFG_SD1_PARTITION3_SIZE "0")
                                    endif()
                                endif()
                            else()
                                set(CFG_SD1_PARTITION2_SIZE "0")
                                set(CFG_SD1_PARTITION3_SIZE "0")
                            endif()
                        endif()

                        math(EXPR lfs_pos "${CFG_SD1_RESERVED_SIZE}+${CFG_SD1_PARTITION0_SIZE}+${CFG_SD1_PARTITION1_SIZE}+${CFG_LFS_FAT_GAP}")
                        message("--datapath=${CMAKE_BINARY_DIR}/data --target=${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/${CFG_UPGRADE_SD1_IMAGE_FILENAME} --reserversize=${lfs_pos} --p0=${CFG_SD1_PARTITION2_SIZE},${CMAKE_BINARY_DIR}/data/temp --p1=${CFG_SD1_PARTITION3_SIZE},")

                        add_custom_command(
                            TARGET ${CMAKE_PROJECT_NAME}
                            POST_BUILD
                            COMMAND lfstool
                            ARGS --lfscmd=${PROJECT_SOURCE_DIR}/tool/bin --datapath=${CMAKE_BINARY_DIR}/data --bsize=65536 --rsize=512 --psize=512 --target=${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/${CFG_UPGRADE_SD1_IMAGE_FILENAME} --reserversize=${lfs_pos} --p0=${CFG_SD1_PARTITION2_SIZE},${CMAKE_BINARY_DIR}/data/temp --p1=${CFG_SD1_PARTITION3_SIZE},
                        )
                    endif()
                else()
                    if(DEFINED CFG_SD1_PARTITION0)
                        if(${CFG_SD1_PARTITION0_SIZE} STREQUAL "0")
                            math(EXPR CFG_SD1_PARTITION0_SIZE "${CFG_UPGRADE_SD1_IMAGE_SIZE}-${CFG_SD1_RESERVED_SIZE}")
                            set(CFG_SD1_PARTITION1_SIZE "0")
                            set(CFG_SD1_PARTITION2_SIZE "0")
                            set(CFG_SD1_PARTITION3_SIZE "0")
                        else()
                            if(DEFINED CFG_SD1_PARTITION1)
                                if(${CFG_SD1_PARTITION1_SIZE} STREQUAL "0")
                                    math(EXPR CFG_SD1_PARTITION1_SIZE "${CFG_UPGRADE_SD1_IMAGE_SIZE}-${CFG_SD1_RESERVED_SIZE}-${CFG_SD1_PARTITION0_SIZE}")
                                    set(CFG_SD1_PARTITION2_SIZE "0")
                                    set(CFG_SD1_PARTITION3_SIZE "0")
                                else()
                                    if(DEFINED CFG_SD1_PARTITION2)
                                        if(${CFG_SD1_PARTITION2_SIZE} STREQUAL "0")
                                            math(EXPR CFG_SD1_PARTITION2_SIZE "${CFG_UPGRADE_SD1_IMAGE_SIZE}-${CFG_SD1_RESERVED_SIZE}-${CFG_SD1_PARTITION0_SIZE}-${CFG_SD1_PARTITION1_SIZE}")
                                            set(CFG_SD1_PARTITION3_SIZE "0")
                                        else()
                                            if(DEFINED CFG_SD1_PARTITION3)
                                                if(${CFG_SD1_PARTITION3_SIZE} STREQUAL "0")
                                                    math(EXPR CFG_SD1_PARTITION3_SIZE "${CFG_UPGRADE_SD1_IMAGE_SIZE}-${CFG_SD1_RESERVED_SIZE}-${CFG_SD1_PARTITION0_SIZE}-${CFG_SD1_PARTITION1_SIZE}-${CFG_SD1_PARTITION2_SIZE}")
                                                endif()
                                            else()
                                                set(CFG_SD1_PARTITION3_SIZE "0")
                                            endif()
                                        endif()
                                    else()
                                        set(CFG_SD1_PARTITION2_SIZE "0")
                                        set(CFG_SD1_PARTITION3_SIZE "0")
                                    endif()
                                endif()
                            else()
                                set(CFG_SD1_PARTITION1_SIZE "0")
                                set(CFG_SD1_PARTITION2_SIZE "0")
                                set(CFG_SD1_PARTITION3_SIZE "0")
                            endif()
                        endif()
                    else()
                        set(CFG_SD1_PARTITION0_SIZE "0")
                        set(CFG_SD1_PARTITION1_SIZE "0")
                        set(CFG_SD1_PARTITION2_SIZE "0")
                        set(CFG_SD1_PARTITION3_SIZE "0")
                    endif()

                    message("--datapath=${CMAKE_BINARY_DIR}/data --target=${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/${CFG_UPGRADE_SD1_IMAGE_FILENAME} --reserversize=${CFG_SD1_RESERVED_SIZE} --p0=${CFG_SD1_PARTITION0_SIZE},${CMAKE_BINARY_DIR}/data/private --p1=${CFG_SD1_PARTITION1_SIZE},${CMAKE_BINARY_DIR}/data/public --p2=${CFG_SD1_PARTITION2_SIZE},${CMAKE_BINARY_DIR}/data/temp --p3=${CFG_SD1_PARTITION3_SIZE},")

                    add_custom_command(
                        TARGET ${CMAKE_PROJECT_NAME}
                        POST_BUILD
                        COMMAND lfstool
                        ARGS --lfscmd=${PROJECT_SOURCE_DIR}/tool/bin --datapath=${CMAKE_BINARY_DIR}/data --bsize=65536 --rsize=512 --psize=512 --target=${CMAKE_BINARY_DIR}/project/${CMAKE_PROJECT_NAME}/${CFG_UPGRADE_SD1_IMAGE_FILENAME} --reserversize=${CFG_SD1_RESERVED_SIZE} --p0=${CFG_SD1_PARTITION0_SIZE},${CMAKE_BINARY_DIR}/data/private --p1=${CFG_SD1_PARTITION1_SIZE},${CMAKE_BINARY_DIR}/data/public --p2=${CFG_SD1_PARTITION2_SIZE},${CMAKE_BINARY_DIR}/data/temp --p3=${CFG_SD1_PARTITION3_SIZE},
                    )
                endif()
            endif()

            if (DEFINED CFG_UPGRADE_BACKUP_PACKAGE)
                #message("pkgtool ${args}")
                add_custom_command(
                    TARGET ${CMAKE_PROJECT_NAME}
                    POST_BUILD
                    COMMAND pkgtool
                    ARGS -o ${CFG_UPGRADE_FILENAME}_NO_BOOTLOADER ${args} --key ${CFG_UPGRADE_ENC_KEY}
                    COMMAND pkgtool
                    ARGS -l ${CFG_UPGRADE_FILENAME}_NO_BOOTLOADER
                    COMMAND ${CMAKE_COMMAND} -E rename
                    ARGS ${CFG_UPGRADE_SD1_IMAGE_FILENAME} ${CFG_UPGRADE_SD1_IMAGE_FILENAME}_ORG
                    COMMAND fsw
                    ARGS ${CFG_UPGRADE_FILENAME}_NO_BOOTLOADER ${CFG_UPGRADE_SD1_IMAGE_FILENAME}_ORG ${CFG_UPGRADE_SD1_IMAGE_FILENAME} ${CFG_UPGRADE_BACKUP_PACKAGE_POS}
                )
            endif()

        endif()

        # If security boot is enabled
        if (DEFINED CFG_SECURITY_SIGNATURE AND DEFINED CFG_BOOTLOADER_ENABLE)
            # Copy ${CMAKE_CURRENT_BINARY_DIR}/${CFG_UPGRADE_FILENAME}.PKG to ${CFG_UPGRADE_FILENAME}/${CMAKE_PROJECT_NAME}_o.PKG
            add_custom_command(
                TARGET ${CMAKE_PROJECT_NAME}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                ARGS ${CMAKE_CURRENT_BINARY_DIR}/${CFG_UPGRADE_FILENAME} ${CMAKE_CURRENT_BINARY_DIR}/${CFG_UPGRADE_FILENAME}_o
                DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${CFG_UPGRADE_FILENAME}
            )

            add_custom_command(
                TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
                COMMAND rsa_sign
                ARGS ${CMAKE_CURRENT_BINARY_DIR}/${CFG_UPGRADE_FILENAME} ${CMAKE_BINARY_DIR}/rsa/rsa_priv.txt
            )

            add_custom_command(
                TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
                COMMAND binaddsig
                ARGS ${CMAKE_CURRENT_BINARY_DIR}/${CFG_UPGRADE_FILENAME}.sig ${CMAKE_CURRENT_BINARY_DIR}/${CFG_UPGRADE_FILENAME}_o ${CMAKE_CURRENT_BINARY_DIR}/${CFG_UPGRADE_FILENAME}
            )
            #file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/${CFG_UPGRADE_FILENAME}.bin.sig)
        endif()

    endif()

    file(GLOB files ${PROJECT_SOURCE_DIR}/sdk/target/debug/*.in)

    foreach (src ${files})
        string(REPLACE "${PROJECT_SOURCE_DIR}/sdk/target/debug/" "${CMAKE_CURRENT_BINARY_DIR}/" tmp ${src})
        string(REPLACE ".in" "" dest ${tmp})
        configure_file(${src} ${dest})
    endforeach()

    if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")

        file(GLOB files ${PROJECT_SOURCE_DIR}/sdk/target/debug/win32/${CFG_CPU_NAME}/*.in)

        foreach (src ${files})
            string(REPLACE "${PROJECT_SOURCE_DIR}/sdk/target/debug/win32/${CFG_CPU_NAME}/" "${CMAKE_CURRENT_BINARY_DIR}/" tmp ${src})
            string(REPLACE ".in" "" dest ${tmp})
            configure_file(${src} ${dest})
        endforeach()

        file(GLOB files ${PROJECT_SOURCE_DIR}/sdk/target/debug/win32/*.in)

        foreach (src ${files})
            string(REPLACE "${PROJECT_SOURCE_DIR}/sdk/target/debug/win32/" "${CMAKE_CURRENT_BINARY_DIR}/" tmp ${src})
            string(REPLACE ".in" "" dest ${tmp})
            configure_file(${src} ${dest})
        endforeach()

    elseif (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")

        file(GLOB files ${PROJECT_SOURCE_DIR}/sdk/target/debug/linux/${CFG_CPU_NAME}/*.in)

        foreach (src ${files})
            string(REPLACE "${PROJECT_SOURCE_DIR}/sdk/target/debug/linux/${CFG_CPU_NAME}/" "${CMAKE_CURRENT_BINARY_DIR}/" tmp ${src})
            string(REPLACE ".in" "" dest ${tmp})
            configure_file(${src} ${dest})
        endforeach()

        file(GLOB files ${PROJECT_SOURCE_DIR}/sdk/target/debug/linux/*.in)

        foreach (src ${files})
            string(REPLACE "${PROJECT_SOURCE_DIR}/sdk/target/debug/linux/" "${CMAKE_CURRENT_BINARY_DIR}/" tmp ${src})
            string(REPLACE ".in" "" dest ${tmp})
            configure_file(${src} ${dest})
        endforeach()

    endif()

    configure_file(${PROJECT_SOURCE_DIR}/sdk/target/debug/cvd.csf.in ${CMAKE_BINARY_DIR}/cvd.csf)

endif()

if (DEFINED CFG_GENERATE_DOC AND (NOT DEFINED ENV{DO_CI_TEST}))
    # Generate the SDK documents in html format.
    #
    # It supports to generating the SDK documents via doxygen or sphinx.
    # However, sphinx-version documentation is not ready.
    find_package(Doxygen REQUIRED)
    # The system needs breath and sphinx_rtd_theme extensions to generate
    # the documentation via sphinx.
    find_package(Sphinx REQUIRED COMPONENTS breathe sphinx_rtd_theme)

    configure_file(${PROJECT_SOURCE_DIR}/doc/readme.html ${CMAKE_BINARY_DIR}/readme.html)

    set(DOC_SOURCE_DIR
        ${CMAKE_SOURCE_DIR}/doc
        ${CMAKE_SOURCE_DIR}/sdk/include/ite
        ${CMAKE_SOURCE_DIR}/sdk/include/isp
        ${CMAKE_SOURCE_DIR}/sdk/include/jpg
        ${CMAKE_SOURCE_DIR}/sdk/include/nor
        ${CMAKE_SOURCE_DIR}/sdk/include/ssp
        ${CMAKE_SOURCE_DIR}/sdk/include/iic
        ${CMAKE_SOURCE_DIR}/project/test_posixapi
        ${CMAKE_SOURCE_DIR}/project/test_posixapi/Pthread
        ${CMAKE_SOURCE_DIR}/sdk/include/linphone
        ${CMAKE_SOURCE_DIR}/sdk/itu/include/ite
        ${CMAKE_SOURCE_DIR}/sdk/itu/doc)

    include(${PROJECT_SOURCE_DIR}/doc/def.cmake)

    set(GENERATE_DOC_VIA_SPHINX FALSE)
    if (Sphinx_breathe_FOUND AND Sphinx_sphinx_rtd_theme_FOUND)
        set(GENERATE_DOC_VIA_SPHINX TRUE)
    endif()
    if (GENERATE_DOC_VIA_SPHINX)
        set(DOXYGEN_GENERATE_XML YES)
        set(DOXYGEN_GENERATE_HTML FALSE)
    else()
        set(DOXYGEN_GENERATE_HTML YES)
    endif()

    doxygen_add_docs("doxygen_docs" ${DOC_SOURCE_DIR})

    if (GENERATE_DOC_VIA_SPHINX)
        sphinx_add_docs("docs"
            BREATHE_PROJECTS doxygen_docs
            BUILDER "html"
            SOURCE_DIRECTORY "${CMAKE_SOURCE_DIR}/doc/sphinx/it${CFG_CHIP_FAMILY}"
            OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/html")
    else()
        execute_process(COMMAND doxygen ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.doxygen_docs)
    endif()
endif()

if ((DEFINED CFG_GENERATE_PACK_ENV) AND NOT (${CMAKE_PROJECT_NAME} STREQUAL bootloader))
    set(CFG_PACK_NAME "${CMAKE_PROJECT_NAME}_${CFG_VERSION_MAJOR}${CFG_VERSION_MINOR}${CFG_VERSION_PATCH}${CFG_VERSION_CUSTOM}${CFG_VERSION_TWEAK}")

    unset(args)

    foreach (def ${packargs})
        set(args "${args} ${def}")
    endforeach()

    add_custom_command(
        TARGET ${CMAKE_PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS
            -DCMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}
            -DCFG_PACK_NAME=${CFG_PACK_NAME}
            -DCMAKE_PROJECT_NAME=${CMAKE_PROJECT_NAME}
            -Dbootloader_path=${bootloader_path}
            -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
            -DCMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -DCFG_UPGRADE_FILENAME=${CFG_UPGRADE_FILENAME}
            -Dargs=${args}
            -DCFG_UPGRADE_ENC_KEY=${CFG_UPGRADE_ENC_KEY}
            -DCFG_UPGRADE_NOR_IMAGE_SIZE=${CFG_UPGRADE_NOR_IMAGE_SIZE}
            -DCFG_UPGRADE_NOR_IMAGE_FILENAME=${CFG_UPGRADE_NOR_IMAGE_FILENAME}
            -P ${PROJECT_SOURCE_DIR}/sdk/target/sdk/pack.cmake
    )

endif()

if ($ENV{CFG_PLATFORM} STREQUAL openrtos AND DEFINED CFG_DBG_TRACE_ANALYZER)
    if (DEFINED CFG_DBG_VCD)
        set(DUMPVCD "1")
    else()
        set(DUMPVCD "0")
    endif()

    add_custom_command(
        TARGET ${CMAKE_PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS
            -DCMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}
            -DCMAKE_PROJECT_NAME=${CMAKE_PROJECT_NAME}
            -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
            -DCMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -DDUMPVCD=${DUMPVCD}
            -P ${PROJECT_SOURCE_DIR}/sdk/target/trace/trace.cmake
    )
endif()
