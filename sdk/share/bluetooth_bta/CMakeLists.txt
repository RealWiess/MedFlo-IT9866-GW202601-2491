add_library(bluetooth INTERFACE)
add_library(bluetooth_btapp)
add_library(bluetooth_private INTERFACE)

target_link_libraries(bluetooth INTERFACE bluetooth_btapp)
target_link_libraries(bluetooth INTERFACE bluetooth_private)

SET (NAME bluetooth_bta)

SET (BTA_PATH
    ${PROJECT_SOURCE_DIR}/sdk/share/bluetooth_bta/BTA_Release
)

if (DEFINED CFG_AP6256BLE)
    SET (BT_MODULE AP6256)
elseif (DEFINED CFG_AP6236BLE)
    SET (BT_MODULE AP6236)
elseif (DEFINED CFG_AP6203BLE)
    SET (BT_MODULE AP6203)
elseif (DEFINED CFG_AP6212BLE)
    SET (BT_MODULE AP6212)
else()
    SET (BT_MODULE AP6256)
endif()

SET (BTEINCLUDES
    ${BTA_PATH}/
    ${BTA_PATH}/Components/bta/ag
    ${BTA_PATH}/Components/bta/ar
    ${BTA_PATH}/Components/bta/avk
    ${BTA_PATH}/Components/bta/brcm
    ${BTA_PATH}/Components/bta/dg
    ${BTA_PATH}/Components/bta/dm
    ${BTA_PATH}/Components/bta/gatt
    ${BTA_PATH}/Components/bta/hd
    ${BTA_PATH}/Components/bta/hs
    ${BTA_PATH}/Components/bta/pb
    ${BTA_PATH}/Components/bta/include
    ${BTA_PATH}/Components/bta/rc
    ${BTA_PATH}/Components/bta/sys
    ${BTA_PATH}/Components/gki/common
    ${BTA_PATH}/Components/gki/openrtos
    ${BTA_PATH}/Components/hcis
    ${BTA_PATH}/Components/hcis/include
    ${BTA_PATH}/Components/stack/include
    ${BTA_PATH}/Components/stack/avrc
    ${BTA_PATH}/Components/stack/avct
    ${BTA_PATH}/Components/stack/avdt
    ${BTA_PATH}/Components/stack/a2dp
    ${BTA_PATH}/Components/stack/rfcomm
    ${BTA_PATH}/Components/stack/btm
    ${BTA_PATH}/Components/stack/brcm
    ${BTA_PATH}/Components/stack/l2cap
    ${BTA_PATH}/Components/stack/sdp
    ${BTA_PATH}/Components/stack/gap
    ${BTA_PATH}/Components/stack/gatt
    ${BTA_PATH}/Components/stack/smp
    ${BTA_PATH}/Components/stack/hid
    ${BTA_PATH}/Components/stack/hsp2
    ${BTA_PATH}/Components/stack/srvc
    ${BTA_PATH}/Components/rpc/include
    ${BTA_PATH}/Components/udrv/include
    ${BTA_PATH}/Components/stack/include
    ${BTA_PATH}/Components/embdrv/sbc/decoder/include
    ${BTA_PATH}/Components/embdrv/sbc/encoder/include
    ${BTA_PATH}/Projects/bte/main
    ${BTA_PATH}/Projects/bta/bleapp_inc
    ${BTA_PATH}/Projects/bta/btapp_comm_inc
    ${BTA_PATH}/Projects/bta/btapp_inc
    ${BTA_PATH}/Projects/bta/bleapp_inc
    ${BTA_PATH}/Projects/bta/bta_co
    ${BTA_PATH}/Projects/proto_disp
    ${BTA_PATH}/../port_ite/inc
    ${BTA_PATH}/../btsnoop
)

set (BRCM_BTAPP_DEMO_SRC
    ${BTA_PATH}/Projects/bta/btapp_src/btapp.c
    ${BTA_PATH}/Projects/bta/btapp_src/btapp_avk.c
    ${BTA_PATH}/Projects/bta/btapp_src/btapp_dg.c
    ${BTA_PATH}/Projects/bta/btapp_src/btapp_dm.c
    ${BTA_PATH}/Projects/bta/btapp_src/btapp_hs.c
    ${BTA_PATH}/Projects/bta/btapp_src/btapp_rc.c
    ${BTA_PATH}/Projects/bta/btapp_src/btapp_pbc.c
    ${BTA_PATH}/Projects/bta/btapp_src/btapp_av.c
    ${BTA_PATH}/Projects/bta/btapp_src/btapp_ag.c
    ${BTA_PATH}/Projects/bta/btapp_src/btapp_dg.c
    ${BTA_PATH}/Projects/bta/bleapp_src/btapp_gatts.c
    ${BTA_PATH}/Projects/bta/bleapp_src/btapp_gattc.c
    ${BTA_PATH}/Projects/bta/bleapp_src/btapp_gattc_profile.c
    ${BTA_PATH}/Projects/bta/bleapp_src/btapp_ble_main.c
    ${BTA_PATH}/Projects/bta/bta_co/bta_fs_co.c
)

set (BRCM_BTAPP_COMM_SRC
    ${BTA_PATH}/Projects/bta/btapp_comm_src/btapp_cfg.c
    ${BTA_PATH}/Projects/bta/btapp_comm_src/btapp_codec_asnk.c
    ${BTA_PATH}/Projects/bta/btapp_comm_src/btapp_nv.c
    ${BTA_PATH}/Projects/bta/btapp_comm_src/btapp_utility.c
    #${BTA_PATH}/Projects/bta/btapp_comm_src/tnt_ble_nus.c
)

SET (BTA_COMMON
    ${BTA_PATH}/../port_ite/src/upio_ite.c
    ${BTA_PATH}/../port_ite/src/userial_ite.c
    ${BTA_PATH}/../port_ite/src/audio_ite.c
    ${BTA_PATH}/../port_ite/src/ucodec_ite.c
    ${BTA_PATH}/../port_ite/src/audio_ctrl.c
    ${BTA_PATH}/Projects/bte/main/bte_disp_stubs.c
    ${BTA_PATH}/Projects/bte/main/bte_hcisu.c
    ${BTA_PATH}/Projects/bte/main/bte_load.c
    ${BTA_PATH}/Projects/bte/main/bte_logmsg.c
    ${BTA_PATH}/Projects/bte/main/bte_main.c
    ${BTA_PATH}/Projects/bte/main/bte_post_reset.c
    ${BTA_PATH}/Projects/bte/main/bte_init.c
	${BTA_PATH}/Projects/bte/fwpatches/${BT_MODULE}/bt_firmware_image.c
)

set (BRCM_BT_HCD_SRC 
    ${BTA_PATH}/Projects/bte/fwpatches/${BT_MODULE}/bt_firmware_image.c
)

if (DEFINED CFG_BTA_ENABLE)

    add_definitions(
        -DBUILDCFG
        -DDEBUG_VERSION=1
        -DCFG_UART_INTR_BUF_SIZE=65536
    )

    include_directories(
        .
        ${BTEINCLUDES}
    )
        
    set (PREBUILD_LIB
        ${BTA_PATH}/libs/libbluetooth_bta.a
    )
	
	add_library(bluetooth_btapp_private_archive STATIC IMPORTED)
	set_target_properties(bluetooth_btapp_private_archive PROPERTIES
		IMPORTED_LOCATION "${PREBUILD_LIB}")
	target_link_libraries(bluetooth_private INTERFACE bluetooth_btapp_private_archive bluetooth)
	target_link_libraries(bluetooth_btapp_private_archive INTERFACE bluetooth_btapp)
	
	target_sources(bluetooth_btapp
	PRIVATE
		${BRCM_BTAPP_COMM_SRC}
		${BRCM_BTAPP_DEMO_SRC}
		${BRCM_BT_HCD_SRC}
		${BTA_COMMON}
	)
	target_link_libraries(bluetooth_btapp INTERFACE bluetooth_btapp_private_archive)
 
endif()