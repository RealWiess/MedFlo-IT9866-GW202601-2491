add_library(sig_verify)

target_include_directories(sig_verify
PRIVATE
    src
PUBLIC
    inc
)

target_sources(sig_verify
PRIVATE
    src/sig_verify.c
)

target_link_libraries(sig_verify mbedtls)
