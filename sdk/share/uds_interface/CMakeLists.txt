add_library(uds_interface STATIC)

target_include_directories(uds_interface
PRIVATE
    ${PROJECT_SOURCE_DIR}/sdk/include/
    ${PROJECT_SOURCE_DIR}/sdk/share/uds_server/
)


target_sources(uds_interface
PRIVATE
    test_suite_can.c
    server_implement.c
    user_config.c
    test_can_upgrade.c
)
