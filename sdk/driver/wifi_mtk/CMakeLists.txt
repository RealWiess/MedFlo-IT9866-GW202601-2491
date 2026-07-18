if (DEFINED CFG_NET_WIFI_SDIO_NGPL_MT7921)
	#get_filename_component(LIB_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
	add_library(wifi_mtk STATIC)

    set(GEN4M ${CMAKE_CURRENT_LIST_DIR}/gen4m)
    set(MTK_LWIP_PORT ${CMAKE_CURRENT_LIST_DIR}/mtk_lwip_port)
    set(MINISUPP ${CMAKE_CURRENT_LIST_DIR}/minisupp)
    set(MINISUPP_INC ${MINISUPP}/inc)
    set(MINISUPP_2_10 ${MINISUPP}/src_protected/wpa_supplicant_2.10)
    set(DHCPD_INC ${CMAKE_CURRENT_LIST_DIR}/dhcpd/inc)

if (DEFINED CFG_NET_LWIP)
	if (DEFINED CFG_GCC_LTO)
		target_link_libraries(wifi_mtk PUBLIC ${CMAKE_CURRENT_LIST_DIR}/wlan_lib/libAwm6721S_Lwip_1.4.1_LTO.a)
	else()
		target_link_libraries(wifi_mtk PUBLIC ${CMAKE_CURRENT_LIST_DIR}/wlan_lib/libAwm6721S_Lwip_1.4.1.a)
	endif()
endif()
if (DEFINED CFG_NET_LWIP_2)
	if (DEFINED CFG_GCC_LTO)
		target_link_libraries(wifi_mtk PUBLIC ${CMAKE_CURRENT_LIST_DIR}/wlan_lib/libAwm6721S_Lwip_2.2.0_LTO.a)
	else()
		target_link_libraries(wifi_mtk PUBLIC ${CMAKE_CURRENT_LIST_DIR}/wlan_lib/libAwm6721S_Lwip_2.2.0.a)
	endif()	
endif()

    add_compile_definitions (
        MT7921S_ITE_PLATFORM
        MT7921_PORTING_TMP_MASK
        MT7921_PORTING_PM_RM
        MTK_MINISUPP_ENABLE
        CFG_SUPPORT_NO_SUPPLICANT_OPS=0
        CFG_THIN_DRV
        CFG_STATIC_MEM_ALLOC=1
        )
    #From mt7933 config.mk
    add_compile_definitions (
        CFG_SUPPORT_SINGLE_SKU_LOCAL_DB=1
        )

    #First draft all the same with linux drvier built 20240315
    add_compile_definitions (
        ARP_MONITER_ENABLE=1
        BUILD_QA_DBG=0
        CC_HAVE_ASM_GOTO
        CC_USING_FENTRY
        CFG_SUPPORT_CFG_FILE=1
        CFG_ADM_CTRL=0
        CFG_ADVANCED_80211_MLO=0
        CFG_BUILD_X86_PLATFORM
        CFG_BUILT_IN_DRIVER=0
        CFG_CHIP_RESET_KO_SUPPORT=0
        CFG_COALESCING_INTERRUPT=0
        CFG_CONTROL_ASPM_BY_FW=0
        CFG_DISABLE_SCAN_UNDER_TWT=0
        CFG_DOWNLOAD_DYN_MEMORY_MAP=0
        CFG_DRIVER_INITIAL_RUNNING_MODE=3
        CFG_ENABLE_GKI_SUPPORT=0
        #CFG_ENABLE_GTK_FRAME_FILTER=1
        CFG_ENABLE_PS_INTV_CTRL=0
        CFG_FLAVOR_FIRMWARE=0
        CFG_GET_CNM_INFO_BC=0
        CFG_HIF_TX_PREALLOC_DATA_BUFFER=1
        CFG_HS20_DEBUG=1
        CFG_IPI_2CHAIN_SUPPORT=0
        CFG_KEEPFULLPOWER_BEFORE_TWT_NEGO=0
        CFG_KERNEL_AN13_515=0
        CFG_KERNEL_AN14_515=0
        CFG_MET_PACKET_TRACE_SUPPORT=0
        CFG_MET_TAG_SUPPORT=0
        CFG_MTK_ANDROID_EMI=0
        CFG_MTK_ANDROID_WMT=0
        CFG_MTK_CONNSYS_DEDICATED_LOG_PATH=0
        CFG_NOT_CLR_FREE_MSDU_IN_ACTIVE_NETWORK=0
        CFG_P2P_SCAN_REPORT_ALL_BSS=0
        CFG_POWER_ON_DOWNLOAD_EMI_ROM_PATCH=0
        CFG_REDIRECT_OID_SUPPORT=1
        CFG_ROM_PATCH_NO_SEM_CTRL=0
        CFG_RX_DIRECT=1
        CFG_RX_REPORT_FORMAT=1
        CFG_SAP_SUPPORT_WPA3_H2E=1
        CFG_SDIO_INTR_ENHANCE_FORMAT=2
        CFG_SDIO_RX_AGG_WORKQUE=0
        CFG_SUPPORT_11B_DYNAMIC=1
        CFG_SUPPORT_6G_PASSIVE_SCAN=1
        CFG_SUPPORT_802_11AX=1
        CFG_SUPPORT_802_11BE=0
        CFG_SUPPORT_AEE=0
        CFG_SUPPORT_AGPS_ASSIST=0
        #CFG_SUPPORT_CFG80211_QUEUE=1
        CFG_SUPPORT_CHNL_CONFLICT_REVISE=0
        CFG_SUPPORT_CMD_OVER_WFDMA=1
        CFG_SUPPORT_COEX_NON_COTX=0
        CFG_SUPPORT_CONNAC2X=1
        CFG_SUPPORT_CONNAC3X=0
        CFG_SUPPORT_DEBUG_FS=0
        CFG_SUPPORT_DEBUG_SOP=0
        CFG_SUPPORT_DISABLE_DATA_DDONE_INTR=1
        CFG_SUPPORT_DMASHDL_SYSDVT=0
        CFG_SUPPORT_DUAL_STA=1
        CFG_SUPPORT_DUAL_WTBL_GTK_REKEY_OFFLOAD=1
        CFG_SUPPORT_HE_ER=0
        CFG_SUPPORT_HIDDEN_SW_AP=0
        CFG_SUPPORT_HOST_RX_WM_EVENT_FROM_PSE=1
        CFG_SUPPORT_IPC_WITHOUT_MMIO_READ=1
        CFG_SUPPORT_MAILBOX_ACK=1
        CFG_SUPPORT_MDNS_OFFLOAD=1
        CFG_SUPPORT_NAN=0
        CFG_SUPPORT_OLD_VENDOR_HAL=0
        #CFG_SUPPORT_OWE=1
        #CFG_SUPPORT_PASSPOINT=1
        CFG_SUPPORT_PCIE_ASPM=0
        CFG_SUPPORT_PCIE_ASPM_IMPROVE=0
        CFG_SUPPORT_PCIE_WFDMA_WMM=0
        CFG_SUPPORT_PDMA_SCATTER=0
        #CFG_SUPPORT_PERMON=1
        CFG_SUPPORT_RETURN_TASK=0
        CFG_SUPPORT_RX_NAPI=0
        CFG_SUPPORT_RX_WORK=0
        CFG_SUPPORT_SCHED_SCAN=1
        CFG_SUPPORT_SINGLE_SKU_DFS_PROTECT=1
        CFG_SUPPORT_SNIFFER_RADIOTAP=0
        CFG_SUPPORT_SOFTAP_OWE=1
        #CFG_SUPPORT_SUPPLICANT_SME=1
        CFG_SUPPORT_SW_BIP_GMAC=1
        CFG_SUPPORT_SW_FAST_PATH=0
        CFG_SUPPORT_TPENHANCE_MODE=0
        CFG_SUPPORT_TSF_USING_BOOTTIME=1
        CFG_SUPPORT_TWT=1
        #CFG_SUPPORT_TWT_HOTSPOT=1
        #CFG_SUPPORT_TWT_HOTSPOT_AC=0
        CFG_SUPPORT_TX_MGMT_USE_DATAQ=0
        CFG_SUPPORT_TX_SG=0
        CFG_SUPPORT_TX_TSO_SW=0
        CFG_SUPPORT_UPDATE_HE_BSS_COLOR_FROM_BEACON=0
        CFG_SUPPORT_WAC=0
        CFG_SUPPORT_WFDMA_REALLOC=1
        CFG_SUPPORT_WIFI_6G=0
        CFG_SUPPORT_WIFI_DBDC6G=0
        CFG_SUPPORT_WIFI_DL_BT_PATCH=1
        CFG_SUPPORT_WIFI_DL_TEST_MODE=0
        CFG_SUPPORT_WIFI_DL_ZB_PATCH=0
        CFG_SUPPORT_WIFI_SYSDVT=0
        CFG_SUPPORT_WOW_EINT=1
        CFG_SUPPORT_WPA3=1
        CFG_TC10_FEATURE=0
        CFG_TC1_FEATURE=0
        CFG_TX_DIRECT=1
        CFG_TX_RSRC_WMM_ENHANCE=1
        CFG_USB_REQ_RX_DATA_CNT=2
        CFG_USB_REQ_TX_DATA_CNT=2
        CFG_USB_REQ_TX_DATA_FFA_CNT=10
        CFG_WIFI_FWDL_UMAC_RESERVE_SIZE_PARA=128
        CFG_WIFI_GET_MCS_INFO=0
        CFG_WIFI_IP_SET=1
        CFG_WIFI_PHY_SUPPORT_GET_BCNTH=0
        CFG_WIFI_RAM_COEX_SPDT_SHR_ANT_CTRL=1
        CFG_WIFI_SUPPORT_NOISE_HISTOGRAM=0
        CFG_WIFI_TX_ETH_CHK_EMPTY_PAYLOAD=1
        CFG_SUPPORT_TX_BF=0
        CONFIG_AS_AVX2=1
        CONFIG_AS_AVX512=1
        CONFIG_AS_AVX=1
        CONFIG_AS_CFI=1
        CONFIG_AS_CFI_SECTIONS=1
        CONFIG_AS_CFI_SIGNAL_FRAME=1
        CONFIG_AS_CRC32=1
        CONFIG_AS_FXSAVEQ=1
        CONFIG_AS_SHA1_NI=1
        CONFIG_AS_SHA256_NI=1
        CONFIG_AS_SSSE3=1
        CONFIG_TEST_ENGINE_OFFLOAD=0
        CONFIG_WIFI_ANTENNA_REPORT=0
        CONFIG_WIFI_SUPPORT_GET_NOISE=0
        CONFIG_WLAN_SERVICE=0
        CONFIG_X86_X32_ABI
        DBG=0
        DRIVER_BUILD_DATE='"20240314201736"'
        KBUILD_BASENAME='"fw_dl"'
        KBUILD_MODNAME='"wlan_mt7961_sdio"'
        #LINUX
        FREERTOS
        OMODULE
        MT7961
        QM_CMD_RESERVED_THRESHOLD=1
        RETPOLINE
        WLAN_INCLUDE_PROC
        _HIF_SDIO=1
        __KERNEL__
    )

    target_include_directories(wifi_mtk

        PRIVATE
        ${GEN4M}/include
        ${GEN4M}/include/chips
        ${GEN4M}/include/mgmt
        ${GEN4M}/include/nic
        ${GEN4M}/os
        ${GEN4M}/os/freertos/hif/sdio/include
        ${GEN4M}/os/freertos/hif/common/include
        ${GEN4M}/os/freertos/include
        ${MINISUPP_INC}/
        ${MINISUPP_2_10}/src/
        ${MINISUPP_2_10}/src/utils/
        ${MINISUPP_2_10}/src/drivers/
        ${DHCPD_INC}/
        ${MTK_LWIP_PORT}/
        PUBLIC
    )

    if (DEFINED CFG_GCC_LTO)
        message ("LTO")
        add_definitions(-fno-lto)
    endif()

	target_sources(wifi_mtk
        PRIVATE
            ${GEN4M}/os/freertos/gl_init.c
            ${GEN4M}/os/freertos/gl_kal.c
            ${GEN4M}/os/freertos/gl_upperlayer.c
#            ${GEN4M}/os/freertos/gl_p2p.c
#            ${GEN4M}/os/freertos/gl_wifi_cli.c
            ${GEN4M}/os/freertos/wifi_api.c
            ${GEN4M}/os/freertos/netif/wifi_netif.c
            ${GEN4M}/mgmt/reg_rule.c
        )

endif()

