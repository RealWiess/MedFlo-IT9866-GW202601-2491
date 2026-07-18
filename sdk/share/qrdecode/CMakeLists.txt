add_library(qrdecode STATIC)

target_sources(qrdecode
PRIVATE
    decode.c
    identify.c
    quirc.c
    quirc.h
    quirc_internal.h
    version_db.c
    dbgutil.c
    dbgutil.h
    identify.c
    #inspect.c
)