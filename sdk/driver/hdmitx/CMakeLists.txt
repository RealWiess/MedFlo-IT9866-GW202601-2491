add_library(hdmitx STATIC)

if ($ENV{CFG_PLATFORM} STREQUAL win32)
    target_compile_definitions(hdmitx
    PRIVATE
        HDMITX_EXPORTS
    )
endif()

target_sources(hdmitx
PRIVATE
    csc.c
    mmp_hdmitx.c
    IT66121/hdmitx_drv.c
    IT66121/hdmitx_sys.c
)

#target_link_libraries(hdmitx
#PRIVATE
#    itp
#    mmp
#    iic
#)
