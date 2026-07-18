if(DEFINED CFG_RTL8821BLE)
    set(BLUEWARE_LIB_PATH
        ${PROJECT_SOURCE_DIR}/sdk/share/blueware/lib/rtl8821cs/libblueware.a
    )
elseif(DEFINED CFG_RTL8733BLE)
    set(BLUEWARE_LIB_PATH
        ${PROJECT_SOURCE_DIR}/sdk/share/blueware/lib/rtl8733fs/libblueware.a
    )
endif()

file(COPY ${BLUEWARE_LIB_PATH}
     DESTINATION ${CMAKE_BINARY_DIR}/lib/${CFG_CPU_NAME}/
)
