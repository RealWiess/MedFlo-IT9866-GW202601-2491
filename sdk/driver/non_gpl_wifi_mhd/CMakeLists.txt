#if (DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6256 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6236 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6212 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6203 OR DEFINED CFG_NET_WIFI_SDIO_NGPL_SYN4345x OR DEFINED CFG_NET_WIFI_SDIO_NGPL_SYN4343x)
	add_library(non_gpl_wifi_mhd)
    add_library(non_gpl_wifi_mhd_archieve STATIC IMPORTED)
	add_definitions(-Wno-error)
    target_include_directories(non_gpl_wifi_mhd
    PRIVATE
          ${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_mhd/include
        ${PROJECT_SOURCE_DIR}/sdk/driver/sd/mmc/include
        ${PROJECT_SOURCE_DIR}/sdk/share/dhcps/
        ${PROJECT_SOURCE_DIR}/sdk/include/lwip/
    )

    target_sources(non_gpl_wifi_mhd
    PRIVATE
        api/bcm_api_test.c
        api/bcm_wifi_test.c
    )

    target_link_libraries(non_gpl_wifi_mhd
    PRIVATE
        non_gpl_wifi_mhd_archieve
    )

	if (DEFINED CFG_IPERF_ENABLE)
		target_compile_definitions(non_gpl_wifi_mhd
		PRIVATE
			CFG_IPERF_ENABLE
		)
	endif()
    if (DEFINED CFG_NET_LWIP)
        target_link_libraries(non_gpl_wifi_mhd_archieve
        INTERFACE
            lwip
        )
    elseif (DEFINED CFG_NET_LWIP_2)
        target_link_libraries(non_gpl_wifi_mhd_archieve
        INTERFACE
            lwip_2
        )
    endif()

	if (DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6256 AND DEFINED CFG_NET_LWIP_2 AND DEFINED CFG_NET_WIFI_SDIO_MHD_P2P)
	target_link_libraries(non_gpl_wifi_mhd_archieve INTERFACE
		"${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_mhd/wlan_lib/libmhdp2pctrl.a"
		"${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_mhd/wlan_lib/libwpa_supp.a"
	)
	endif()


    if (DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6256)
        if (DEFINED CFG_NET_LWIP)
            set_target_properties(non_gpl_wifi_mhd_archieve PROPERTIES
                IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_mhd/wlan_lib/libmhd4345x-LwIP_1_4_1.a"
            )
        endif()
        if (DEFINED CFG_NET_LWIP_2)
            set_target_properties(non_gpl_wifi_mhd_archieve PROPERTIES
                IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_mhd/wlan_lib/libmhd4345x-LwIP_2_2_0.a"
            )
        endif()
    endif()

    if (DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6236)
        if (DEFINED CFG_NET_LWIP)
            set_target_properties(non_gpl_wifi_mhd_archieve PROPERTIES
                IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_mhd/wlan_lib/libmhd4343x-LwIP_1_4_1.a"
            )
        endif()
        if (DEFINED CFG_NET_LWIP_2)
            set_target_properties(non_gpl_wifi_mhd_archieve PROPERTIES
                IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_mhd/wlan_lib/libmhd4343x-LwIP_2_2_0.a"
            )
        endif()
    endif()

    if (DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6212)
        if (DEFINED CFG_NET_LWIP)
            set_target_properties(non_gpl_wifi_mhd_archieve PROPERTIES
                IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_mhd/wlan_lib/libmhd43438-LwIP_1_4_1.a"
            )
        endif()
        if (DEFINED CFG_NET_LWIP_2)
            set_target_properties(non_gpl_wifi_mhd_archieve PROPERTIES
                IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_mhd/wlan_lib/libmhd43438-LwIP_2_2_0.a"
            )
        endif()
    endif()
    if (DEFINED CFG_NET_WIFI_SDIO_NGPL_AP6203)
        if (DEFINED CFG_NET_LWIP)
            set_target_properties(non_gpl_wifi_mhd_archieve PROPERTIES
                IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_mhd/wlan_lib/libmhd4301x-LwIP_1_4_1.a"
            )
        endif()
        if (DEFINED CFG_NET_LWIP_2)
            set_target_properties(non_gpl_wifi_mhd_archieve PROPERTIES
                IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_mhd/wlan_lib/libmhd4301x-LwIP_2_2_0.a"
            )
        endif()
    endif()

    if (DEFINED CFG_NET_WIFI_SDIO_NGPL_SYN4345x)
        if (DEFINED CFG_NET_LWIP)
            set_target_properties(non_gpl_wifi_mhd_archieve PROPERTIES
                IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_mhd/wlan_lib/libmhd_sdio_4345x_LWIP_1_4_1.a"
            )
        endif()	
        if (DEFINED CFG_NET_LWIP_2)
            set_target_properties(non_gpl_wifi_mhd_archieve PROPERTIES
                IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_mhd/wlan_lib/libmhd_sdio_4345x.a"
            )
        endif()
    endif()

    if (DEFINED CFG_NET_WIFI_SDIO_NGPL_SYN4343x)
        if (DEFINED CFG_NET_LWIP_2)
            set_target_properties(non_gpl_wifi_mhd_archieve PROPERTIES
                IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/sdk/driver/non_gpl_wifi_mhd/wlan_lib/libmhd_sdio_4343x.a"
            )
        endif()
    endif()

#endif()