add_library(pkg_verify)

target_include_directories(pkg_verify
PRIVATE
    src
PUBLIC
    inc
    ../sig_verify/inc
)

target_sources(pkg_verify
PRIVATE
    src/pkg_verify.c
)

target_link_libraries(pkg_verify sig_verify mbedtls)
