add_library(hdmirx)

target_link_libraries(hdmirx
PRIVATE
    iic
)

if (DEFINED CFG_IT6801)
    target_sources(hdmirx
    PRIVATE
        it6802/edid.c
        it6802/IO.c
        it6802/it680x.c
        it6802/it680x_cec.c
        it6802/Utility.c
        it6802/it680x_interface.c
        mmp_hdmirx.c
    )
endif()

if (DEFINED CFG_IT66021)
    target_sources(hdmirx
    PRIVATE
        it66021/edid.c
        it66021/IO.c
        it66021/it660x.c
        it66021/it660x_cec.c
        it66021/Utility.c
        it66021/it6602_interface.c
        mmp_hdmirx.c
    )
endif()