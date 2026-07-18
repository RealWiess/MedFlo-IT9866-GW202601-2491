add_library(upgrade STATIC)

target_compile_definitions(upgrade
PRIVATE
    ITP_FAT_ACCEL
)

target_compile_definitions_ifdef(CFG_UG_ERR upgrade PRIVATE CFG_UG_ERR)
target_compile_definitions_ifdef(CFG_UG_WARN upgrade PRIVATE CFG_UG_WARN)
target_compile_definitions_ifdef(CFG_UG_INFO upgrade PRIVATE CFG_UG_INFO)
target_compile_definitions_ifdef(CFG_UG_DBG upgrade PRIVATE CFG_UG_DBG)

if (DEFINED CFG_UG_BUF_SIZE)
    target_compile_definitions(upgrade
    PRIVATE
        CFG_UG_BUF_SIZE=${CFG_UG_BUF_SIZE}
    )
else()
    target_compile_definitions(upgrade
    PRIVATE
        CFG_UG_BUF_SIZE=0x20000
    )
endif()

target_sources(upgrade
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/ite/ug.h
    ug_cfg.h
    ug_filecrc.c
    ug_device.c
    ug_directory.c
    ug_end.c
    ug_file.c
    ug_mark.c
    ug_package.c
    ug_partition.c
    ug_rawdata.c
    ug_reset.c
    ug_unformatted.c
    ug_crc.c
)

if (DEFINED CFG_NOR_USE_DPUAES)
    if (NOT CFG_FW_DPUAES_UPGRADE_MARK_POS STREQUAL 0)
        target_sources(upgrade
        PRIVATE
            ug_fw_aes_mark.c
        )
    endif()
endif()

if ($ENV{CFG_PLATFORM} STREQUAL openrtos)
    target_link_libraries(upgrade ug_diff)
endif()