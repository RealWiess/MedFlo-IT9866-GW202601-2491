#include <stdio.h>

typedef enum OTP_OPERATION_TAG
{
    OTP_OPERATION_AESK,
    OTP_OPERATION_RSAPKH,
    OTP_OPERATION_AESK_RSAPKH
} OTP_OPERATION;

void otp_check(uint8_t *aes_key, uint8_t *rsa_public_key_hash, OTP_OPERATION oip_op);
