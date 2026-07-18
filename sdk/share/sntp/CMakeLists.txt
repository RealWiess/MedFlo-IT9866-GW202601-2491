add_library(sntp STATIC)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    target_compile_definitions(sntp
    PRIVATE
        LWIP_DEBUG
        SNTP_DEBUG=LWIP_DBG_ON
    )
endif()

target_sources(sntp
PRIVATE
    sntp.c
    sntp.h
)