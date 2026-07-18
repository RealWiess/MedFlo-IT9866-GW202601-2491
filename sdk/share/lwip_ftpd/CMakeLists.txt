add_library(lwip_ftpd)

target_sources(lwip_ftpd
PRIVATE
    ftpd.c
    ftpd.h
    vfs.c
    vfs.h
)