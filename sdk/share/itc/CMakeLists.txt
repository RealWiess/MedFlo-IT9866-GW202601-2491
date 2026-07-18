add_library(itc STATIC)

target_compile_definitions(itc
PRIVATE
    ITP_FAT_ACCEL
)

target_compile_definitions_ifdef(CFG_ITC_ERR itc PRIVATE CFG_ITC_ERR)
target_compile_definitions_ifdef(CFG_ITC_WARN itc PRIVATE CFG_ITC_WARN)
target_compile_definitions_ifdef(CFG_ITC_INFO itc PRIVATE CFG_ITC_INFO)
target_compile_definitions_ifdef(CFG_ITC_DBG itc PRIVATE CFG_ITC_DBG)

target_sources(itc
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/ite/itc.h
    itc_arraystream.c
    itc_blockstream.c
    itc_bufferstream.c
    itc_cfg.h
    itc_crc.c
    itc_filestream.c
    itc_liststream.c
    itc_stream.c
    itc_tree.c
    itc_url.c
    itc_clistream.c
)
