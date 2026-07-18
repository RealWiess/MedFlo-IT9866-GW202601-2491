add_library(rtl8304mb)

target_include_directories(rtl8304mb
PUBLIC
    inc
)

target_sources(rtl8304mb
PRIVATE
    mdcmdio.c
    rtk_api.c
    rtl8309n_asicdrv.c
)
