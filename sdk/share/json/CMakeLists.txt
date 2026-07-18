add_library(json STATIC)

target_compile_definitions(json
PRIVATE
    PING_USE_SOCKETS=0
)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    target_compile_definitions(json
    PRIVATE
        LWIP_DEBUG
    )
endif()

target_sources(json
PRIVATE
    arraylist.c
    debug.c
    json_c_version.c
    json_object.c
    json_object_iterator.c
    json_pointer.c
    json_tokener.c
    json_util.c
    json_visit.c
    linkhash.c
    printbuf.c
    random_seed.c
    strerror_override.c
    test2.c
)