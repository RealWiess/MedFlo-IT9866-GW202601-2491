if (DEFINED CFG_NET_WIFI_SDIO_NGPL_8821CS)
    get_filename_component(LIB_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)

    add_library(${LIB_NAME} STATIC)

    target_include_directories(${LIB_NAME}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/os/customer_rtos/
        ${CMAKE_CURRENT_LIST_DIR}/os/os_dep/include/
        ${CMAKE_CURRENT_LIST_DIR}/platform/include
        ${CMAKE_CURRENT_LIST_DIR}/platform/rtwlan_bsp/
        ${CMAKE_CURRENT_LIST_DIR}/platform/sdio/core/
        ${CMAKE_CURRENT_LIST_DIR}/platform/sdio/include/
        ${PROJECT_SOURCE_DIR}/sdk/driver/sd/mmc/include
        ${PROJECT_SOURCE_DIR}/sdk/driver/usb/inc
        ${PROJECT_SOURCE_DIR}/sdk/share/dhcps/
        ${PROJECT_SOURCE_DIR}/sdk/include/lwip/
		${PROJECT_SOURCE_DIR}/openrtos/include/openrtos/
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/api/wifi/
		${CMAKE_CURRENT_LIST_DIR}/api/wifi/rtw_wpa_supplicant/src/
        ${CMAKE_CURRENT_LIST_DIR}/api/wifi/rtw_wpa_supplicant/wpa_supplicant
        ${PROJECT_SOURCE_DIR}/sdk/include/mbedtls/
    )

    #target_link_libraries(${LIB_NAME}
    #PRIVATE
    #    ${CMAKE_CURRENT_LIST_DIR}/wlan_lib/wlan_lib.a
    #
    #)
    add_library(${LIB_NAME}_archive STATIC IMPORTED)
    set_target_properties(${LIB_NAME}_archive PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/wlan_lib/wlan_lib.a")
    target_link_libraries(${LIB_NAME}_archive
    INTERFACE
        ${LIB_NAME}
        sd
    )

    target_link_libraries(${LIB_NAME}
    PRIVATE
        ${LIB_NAME}_archive
        mbedtls
    )

    target_sources(${LIB_NAME}
    PRIVATE
        api/wifi/wifi_conf.c
        api/wifi/wifi_ind.c
        api/wifi/wifi_promisc.c
        api/wifi/wifi_util.c
		api/wifi/rtw_wpa_supplicant/wpa_supplicant/wifi_p2p_config.c
		api/wifi/rtw_wpa_supplicant/wpa_supplicant/wifi_wps_config.c
        api/wifi_interactive_mode.c
		api/rtw_opt_crypto_ssl.c

        os/customer_rtos/customer_rtos_service.c

        os/os_dep/device_lock.c
        os/os_dep/net_stack_intf.c
        os/os_dep/osdep_service.c

        platform/rtwlan_bsp/rtwlan_bsp.c
        platform/sdio/wifi_io.c
    )
endif()