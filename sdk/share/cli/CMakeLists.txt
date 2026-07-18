add_library(cli)

target_compile_definitions_ifdef(CFG_CLI_ERR cli PRIVATE CFG_CLI_ERR)
target_compile_definitions_ifdef(CFG_CLI_WARN cli PRIVATE CFG_CLI_WARN)
target_compile_definitions_ifdef(CFG_CLI_INFO cli PRIVATE CFG_CLI_INFO)
target_compile_definitions_ifdef(CFG_CLI_DBG cli PRIVATE CFG_CLI_DBG)
target_compile_definitions_ifdef(CFG_CLI_BUF_SIZE cli
PRIVATE
    CFG_CLI_BUF_SIZE=${CFG_CLI_BUF_SIZE}
)
target_compile_definitions_ifndef(CFG_CLI_BUF_SIZE cli
PRIVATE
    CFG_CLI_BUF_SIZE=0x20000
)

target_sources(cli
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/ite/cli.h
    cli.c
    cli_cfg.h
    cli_system.c
)

if (DEFINED CFG_FS_FAT)
    target_sources(cli
    PRIVATE
        cli_disk.c
        cli_file.c
    )
endif()

if (DEFINED CFG_OPENRTOS_USE_TRACE_FACILITY AND DEFINED CFG_OPENRTOS_GENERATE_RUN_TIME_STATS)
    target_sources(cli
    PRIVATE
        cli_stats.c
    )
endif()

if (DEFINED CFG_CLI_USB_ACM)
    target_compile_definitions(cli
    PRIVATE
        CFG_CLI_USB_ACM
    )

    if (DEFINED CFG_SD0_STATIC OR DEFINED CFG_SD1_STATIC OR DEFINED CFG_NAND_ENABLE OR DEFINED CFG_NOR_ENABLE)
        target_sources(cli
        PRIVATE
            cli_rawdata.c
        )
    endif()

    if (DEFINED CFG_UPGRADE_BOOTLOADER OR DEFINED CFG_UPGRADE_IMAGE OR DEFINED CFG_UPGRADE_DATA)
        target_compile_definitions(cli
        PRIVATE
            CFG_CLI_UPGRADE
        )

        target_sources(cli
        PRIVATE
            cli_upgrade.c
        )
    endif()

endif()

if (DEFINED CFG_ENABLE_UART_CLI)
    if (DEFINED CFG_UPGRADE_BOOTLOADER OR DEFINED CFG_UPGRADE_IMAGE OR DEFINED CFG_UPGRADE_DATA)
        target_compile_definitions(cli
        PRIVATE
            CFG_CLI_UPGRADE
        )
        target_sources(cli
        PRIVATE
            cli_upgrade_uart.c
        )
    endif()
endif()
