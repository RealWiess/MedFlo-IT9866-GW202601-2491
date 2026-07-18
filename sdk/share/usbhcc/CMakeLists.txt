if (NOT DEFINED WIN32)
    add_library(usbhcc INTERFACE)
    add_library(usbhcc_archive STATIC IMPORTED)
    set_target_properties(usbhcc_archive PROPERTIES
        IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/../lib/${CFG_CPU_NAME}/libusbhcc.a")
    target_link_libraries(usbhcc INTERFACE usbhcc_archive )

    target_link_libraries(usbhcc
    INTERFACE
        itp_usbhcc
    )
endif()
