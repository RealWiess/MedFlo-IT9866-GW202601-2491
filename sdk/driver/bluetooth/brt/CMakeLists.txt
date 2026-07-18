if (DEFINED CFG_BARROT_MODULE_8821CS OR DEFINED CFG_BARROT_MODULE_BR8051)
	set (BT_MODULE BRT8051)
endif()

file(GLOB BRT_FW
	src/fwpatches/${BT_MODULE}/*.c
	)

include_directories(
    .
    ${PROJECT_SOURCE_DIR}/sdk/include/bluetooth/brt8051ble
    )

file(GLOB brt_SRC
	src/*.c
	)

get_filename_component(rm_file ${CMAKE_CURRENT_SOURCE_DIR}/src/mini_upper_stack.c ABSOLUTE)

list(REMOVE_ITEM brt_SRC "${rm_file}")

add_library(bluetooth STATIC
	${brt_SRC}
	${BRT_FW}
    )