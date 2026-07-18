add_library(otp_check)

target_include_directories(otp_check
PRIVATE
    inc
)

target_sources(otp_check
PRIVATE
    src/otp_check.c
)
