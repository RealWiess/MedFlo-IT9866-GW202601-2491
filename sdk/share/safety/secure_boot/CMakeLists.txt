add_library(secure_boot)

target_include_directories(secure_boot
PRIVATE
    inc
    ../otp_check/inc
if (DEFINED CFG_SECURITY_SIGNATURE)
    ../sig_verify/inc
endif()
)

target_sources(secure_boot
PRIVATE
    src/secure_boot.c
)
