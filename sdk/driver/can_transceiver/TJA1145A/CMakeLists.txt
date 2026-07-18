add_library(can_transceiver STATIC)

target_include_directories(can_transceiver
PRIVATE
	.
	..
    ${PROJECT_SOURCE_DIR}/sdk/include/
	${PROJECT_SOURCE_DIR}/sdk/include/can_transceiver/TJA1145A/
)

target_sources(can_transceiver
PRIVATE
	TJA1145AFD_Application_Specific_Functions.c
	NXP_TJA1145AFD_Functions.c
	uC_Specific_Function.c
)
