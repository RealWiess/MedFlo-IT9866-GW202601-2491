if ((${CMAKE_PROJECT_NAME} STREQUAL sdk_standard_ngpl)
 OR (${CMAKE_PROJECT_NAME} STREQUAL sdk_standard_ngpl_intl))
    set(CFG_RTL8821BLE y)
    set(CFG_GPIO_BT_REG_ON 27)
endif()

if (DEFINED CFG_RTL8723BLE)
	set (BT_MODULE RTL8723)
elseif(DEFINED CFG_RTL8733BLE)
	set (BT_MODULE RTL8733)
elseif(DEFINED CFG_RTL8821BLE)
	set (BT_MODULE RTL8821)
endif()

file(GLOB RTK_FW
	src/fwpatches/${BT_MODULE}/*.c
	)

include_directories(
    .
    ${PROJECT_SOURCE_DIR}/sdk/include/bluetooth/rtl8723ble
    )

file(GLOB rtk_SRC
	src/*.c
	)

get_filename_component(rm_file ${CMAKE_CURRENT_SOURCE_DIR}/src/mini_upper_stack.c ABSOLUTE)

list(REMOVE_ITEM rtk_SRC "${rm_file}")

add_library(bluetooth STATIC
	${rtk_SRC}
	${RTK_FW}
    )