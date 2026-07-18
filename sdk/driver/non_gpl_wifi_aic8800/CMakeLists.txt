if (DEFINED CFG_NET_WIFI_SDIO_NGPL_AIC8800)
  	get_filename_component(LIB_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
  	
	#SET(LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_LIST_DIR}/wlan_lib")
	
  	add_library(${LIB_NAME} STATIC)

  	include(${CMAKE_CURRENT_LIST_DIR}/host/CMakeLists.txt OPTIONAL)

	target_include_directories(${LIB_NAME}
    PUBLIC
	    ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/host/
		${CMAKE_CURRENT_LIST_DIR}/host/common/api/
		${CMAKE_CURRENT_LIST_DIR}/host/drv/fhost/
		${CMAKE_CURRENT_LIST_DIR}/host/drv/aic_bsp/
        ${CMAKE_CURRENT_LIST_DIR}/host/drv/fw/
		${CMAKE_CURRENT_LIST_DIR}/host/drv/macif/
		${CMAKE_CURRENT_LIST_DIR}/host/drv/sdio/
		${CMAKE_CURRENT_LIST_DIR}/host/drv/wlan/
		${CMAKE_CURRENT_LIST_DIR}/host/inc/priv/
		${CMAKE_CURRENT_LIST_DIR}/host/net_al/
		${CMAKE_CURRENT_LIST_DIR}/host/net_al/ite_freertos/
		${CMAKE_CURRENT_LIST_DIR}/host/platform/ite_sdio/
		${CMAKE_CURRENT_LIST_DIR}/host/rtos_al/ite_freertos/
		${PROJECT_SOURCE_DIR}/sdk/driver/sd/mmc/include/
		${PROJECT_SOURCE_DIR}/sdk/share/
		${PROJECT_SOURCE_DIR}/sdk/share/dhcps/
		${PROJECT_SOURCE_DIR}/sdk/include/
		${PROJECT_SOURCE_DIR}/sdk/include/mbedtls
		${PROJECT_SOURCE_DIR}/openrtos/include/
	)
	add_definitions(-Wno-error)
	add_definitions(-DCONFIG_SDIO_SUPPORT)
	add_definitions(-DCONFIG_AIC8800D80)
	#add_definitions(-DCONFIG_AIC8801)
	#add_definitions(-DCONFIG_BT_SUPPORT)
	add_definitions(-DPLATFORM_ITE_FREERTOS)
	add_definitions(-DCONFIG_RWNX_FULLMAC)
	add_definitions(-DUSE_5G)
	add_definitions(-DCONFIG_PMIC_SETTING)
	add_definitions(-DCONFIG_RWNX_DBG)
	add_definitions(-DCONFIG_DOWNLOAD_FW)
	add_definitions(-DCONFIG_LOAD_USERCONFIG)
	add_definitions(-DCONFIG_FHOST_RX_ASYNC)
	add_definitions(-DCONFIG_REORD_FORWARD_LIST)
	add_definitions(-DCONFIG_OFFCHANNEL)
	add_definitions(-DCONFIG_P2P)
	#add_definitions(-DCONFIG_MBEDTLS)
	#add_definitions(-DCONFIG_PING_DUMP)

    target_sources(${LIB_NAME}
	PUBLIC
		${CMAKE_CURRENT_LIST_DIR}/host/common/src/co_dlist.c
		${CMAKE_CURRENT_LIST_DIR}/host/common/src/co_list.c
		${CMAKE_CURRENT_LIST_DIR}/host/common/src/co_math.c
		${CMAKE_CURRENT_LIST_DIR}/host/common/src/co_pool.c
		${CMAKE_CURRENT_LIST_DIR}/host/common/src/co_ring.c
		${CMAKE_CURRENT_LIST_DIR}/host/common/src/co_version.c
		${CMAKE_CURRENT_LIST_DIR}/host/drv/fhost/fhost_cntrl.c
		${CMAKE_CURRENT_LIST_DIR}/host/drv/fhost/fhost_config.c
		${CMAKE_CURRENT_LIST_DIR}/host/drv/fhost/fhost_rx.c
		${CMAKE_CURRENT_LIST_DIR}/host/drv/fhost/fhost_tx.c
		${CMAKE_CURRENT_LIST_DIR}/host/drv/fhost/fhost_wpa_config.c
		${CMAKE_CURRENT_LIST_DIR}/host/drv/aic_bsp/aic_bsp_driver.c
		${CMAKE_CURRENT_LIST_DIR}/host/drv/aic_bsp/aic_bsp_main.c
		${CMAKE_CURRENT_LIST_DIR}/host/drv/fw/aic_fw.c
		${CMAKE_CURRENT_LIST_DIR}/host/drv/macif/cli_cmd.c
		${CMAKE_CURRENT_LIST_DIR}/host/drv/macif/rwnx_cmds.c
		${CMAKE_CURRENT_LIST_DIR}/host/drv/macif/rwnx_main.c
		${CMAKE_CURRENT_LIST_DIR}/host/drv/macif/rwnx_msg.c
		${CMAKE_CURRENT_LIST_DIR}/host/drv/macif/rwnx_msg_rx.c
		${CMAKE_CURRENT_LIST_DIR}/host/drv/macif/rwnx_msg_tx.c
		${CMAKE_CURRENT_LIST_DIR}/host/drv/macif/rwnx_platform.c
		${CMAKE_CURRENT_LIST_DIR}/host/drv/macif/rwnx_utils.c
		${CMAKE_CURRENT_LIST_DIR}/host/drv/wlan/wifi_api_dummy.c
		${CMAKE_CURRENT_LIST_DIR}/host/net_al/ite_freertos/net_al.c
		${CMAKE_CURRENT_LIST_DIR}/host/platform/ite_sdio/porting.c
		${CMAKE_CURRENT_LIST_DIR}/host/platform/ite_sdio/sdio_port.c
		${CMAKE_CURRENT_LIST_DIR}/host/rtos_al/ite_freertos/rtos_al.c
		${CMAKE_CURRENT_LIST_DIR}/host/rtos_al/ite_freertos/aic_skbuff.c
		${CMAKE_CURRENT_LIST_DIR}/wifi.c
		${CMAKE_CURRENT_LIST_DIR}/userconfig.c
	)
	#if (DEFINED CONFIG_MBEDTLS)
	#	target_sources(${LIB_NAME}
    #	PRIVATE 
	#		${CMAKE_CURRENT_LIST_DIR}/host/wpa_supplicant/wpa_supplicant/sme.c
	#		${CMAKE_CURRENT_LIST_DIR}/host/wpa_supplicant/src/common/sae.c
	#		${CMAKE_CURRENT_LIST_DIR}/host/wpa_supplicant/src/common/dragonfly.c
	#		${CMAKE_CURRENT_LIST_DIR}/host/wpa_supplicant/src/crypto/crypto_mbedtls.c
	#	)
	#endif()

	target_link_libraries(${LIB_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/host/libwlan_priv.a)
endif()
