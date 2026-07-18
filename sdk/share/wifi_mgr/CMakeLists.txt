add_library(wifi_mgr STATIC)

if (DEFINED CFG_NET_WIFI_SDIO_VND_MTK OR
    DEFINED CFG_NET_WIFI_SDIO_VND_RTK OR
    DEFINED CFG_NET_WIFI_SDIO_VND_MHD OR
    DEFINED CFG_NET_WIFI_SDIO_NGPL_ATBM6031 OR
    DEFINED CFG_NET_WIFI_SDIO_VND_AIC)

    target_include_directories(wifi_mgr
    PRIVATE
        ${PROJECT_SOURCE_DIR}/sdk/share/wifi_mgr/
        ${PROJECT_SOURCE_DIR}/sdk/share/wifi_mgr/wifiMgr_inc
        ${PROJECT_SOURCE_DIR}/sdk/share/dhcps/
        ${PROJECT_SOURCE_DIR}/sdk/include/lwip/
    )

    if (DEFINED CFG_NET_WIFI_SDIO_NGPL_MT7921)
        target_include_directories(wifi_mgr
        PRIVATE
            ${PROJECT_SOURCE_DIR}/sdk/driver/wifi_mtk/gen4m/os/freertos/include/
            ${PROJECT_SOURCE_DIR}/sdk/driver/wifi_mtk/gen4m/include/
			${PROJECT_SOURCE_DIR}/sdk/driver/wifi_mtk/gen4m/include/nic/
			${PROJECT_SOURCE_DIR}/sdk/driver/wifi_mtk/minisupp/inc/
			${PROJECT_SOURCE_DIR}/sdk/driver/wifi_mtk/minisupp/src_protected/wpa_supplicant_2.10/src/

        )
    endif()

    if (DEFINED CFG_NET_WIFI_SDIO_NGPL_8189FTV)
        target_include_directories(wifi_mgr
        PRIVATE
            ${PROJECT_SOURCE_DIR}/sdk/driver/wifi_rtk/non_gpl_wifi_8189f/include/
            ${PROJECT_SOURCE_DIR}/sdk/driver/wifi_rtk/non_gpl_wifi_8189f/api/wifi/
        )
    endif()

    if (DEFINED CFG_NET_WIFI_SDIO_NGPL_8723DS)
        target_include_directories(wifi_mgr
        PRIVATE
            ${PROJECT_SOURCE_DIR}/sdk/driver/wifi_rtk/non_gpl_wifi_8723d/include/
            ${PROJECT_SOURCE_DIR}/sdk/driver/wifi_rtk/non_gpl_wifi_8723d/api/wifi/
        )
    endif()

    if (DEFINED CFG_NET_WIFI_SDIO_NGPL_8821CS)
        target_include_directories(wifi_mgr
        PRIVATE
            ${PROJECT_SOURCE_DIR}/sdk/driver/wifi_rtk/non_gpl_wifi_8821c/include/
            ${PROJECT_SOURCE_DIR}/sdk/driver/wifi_rtk/non_gpl_wifi_8821c/api/wifi/
        )
    endif()

    if (DEFINED CFG_NET_WIFI_SDIO_NGPL_8733BS)
        target_include_directories(wifi_mgr
        PRIVATE
            ${PROJECT_SOURCE_DIR}/sdk/driver/wifi_rtk/non_gpl_wifi_8733b/include/
            ${PROJECT_SOURCE_DIR}/sdk/driver/wifi_rtk/non_gpl_wifi_8733b/api/wifi/
        )
    endif()

    if (DEFINED CFG_NET_WIFI_SDIO_VND_MHD)
        target_include_directories(wifi_mgr
        PRIVATE
            ${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_mhd/include/
        )
    endif()

    if (DEFINED CFG_NET_WIFI_SDIO_NGPL_ATBM6031)
        target_include_directories(wifi_mgr
        PRIVATE
            ${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_atbm/include/
            ${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_atbm/
        )
    endif()
	
    if (DEFINED CFG_NET_WIFI_SDIO_VND_AIC)
		add_definitions(-Wno-error)  	
        target_include_directories(wifi_mgr
        PRIVATE
			${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_aic8800/
            ${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_aic8800/include/
			#${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_aic8800/host/drv/wlan/
			#${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_aic8800/host/drv/macif/
			#${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_aic8800/host/common/api/
        )
    endif()
    if (DEFINED CFG_NET_WIFI_SDIO_VND_RTK)
        target_sources(wifi_mgr
        PRIVATE
            wifiMgr_ngpl_rtk.c
        )
    elseif(DEFINED CFG_NET_WIFI_SDIO_VND_MHD)
        target_sources(wifi_mgr
        PRIVATE
            wifiMgr_ngpl_mhd.c
        )
    elseif(DEFINED CFG_NET_WIFI_SDIO_NGPL_ATBM6031)
        target_sources(wifi_mgr
        PRIVATE
            wifiMgr_ngpl_atbm.c
        )
	elseif(DEFINED CFG_NET_WIFI_SDIO_VND_AIC)
        target_sources(wifi_mgr
        PRIVATE
            wifiMgr_ngpl_aic.c
        )
    elseif(DEFINED CFG_NET_WIFI_SDIO_NGPL_MT7921)
        message("wifi_mgr/CMakeLists.txt CFG_NET_WIFI_SDIO_NGPL_MT7921")
        target_sources(wifi_mgr
        PRIVATE
            wifiMgr_ngpl_mtk.c
        )
    else()
        target_sources(wifi_mgr
        PRIVATE
            wifiMgr.c
        )
    endif()

else()
    target_include_directories(wifi_mgr
    PRIVATE
        ${PROJECT_SOURCE_DIR}/sdk/share/wifi_mgr/
        ${PROJECT_SOURCE_DIR}/sdk/share/wifi_mgr/wifiMgr_inc
        ${PROJECT_SOURCE_DIR}/sdk/share/dhcps/
        ${PROJECT_SOURCE_DIR}/sdk/include/lwip/
    )

    target_sources(wifi_mgr
    PRIVATE
        wifiMgr_win32.c
    )
endif()