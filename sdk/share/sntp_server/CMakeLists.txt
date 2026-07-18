add_library(sntp_server STATIC)

target_sources(sntp_server
PRIVATE
    datatype.h
    NtpTime.c
    NtpTime.h
    sntp.h
    sntp_server.c
    sntp_server.h
)