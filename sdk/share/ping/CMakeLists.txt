add_library(ping STATIC)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    target_compile_definitions(ping
    PRIVATE
        LWIP_DEBUG
    )
endif()

target_sources(ping
PRIVATE
    ping.c
    ping.h
)