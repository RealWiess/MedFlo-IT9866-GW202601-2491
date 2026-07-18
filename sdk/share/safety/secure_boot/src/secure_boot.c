#include "secure_boot.h"
#include "otp_check.h"
#ifdef CFG_SECURITY_SIGNATURE
#include "sig_verify.h"
#endif

void secure_boot(void)
{
#if defined(CFG_NOR_USE_DPUAES)
    uint8_t otp_app_aes_key[] = {CFG_NOR_DPUAES_APP_KEY};
#endif
#ifdef CFG_SECURITY_SIGNATURE
    uint8_t otp_pubkey_hash[SECURE_PUBKEY_HASH_SIZE] = {0};
#endif

#ifdef CFG_SECURITY_SIGNATURE
    printf("\ndefined CFG_SECURITY_SIGNATURE! Get Public Key Hash ...\n");
    sig_address_pubkey_hash(otp_pubkey_hash);
    //sig_verify(CFG_UPGRADE_IMAGE_POS, SIGNATURE_OPERATION_RSAPKH_GET);
#if defined(CFG_NOR_USE_DPUAES)
    otp_check(otp_app_aes_key, otp_pubkey_hash, OTP_OPERATION_AESK_RSAPKH);
#else
    otp_check(NULL, otp_pubkey_hash, OTP_OPERATION_RSAPKH);
#endif
    //printf("\ndefined CFG_SECURITY_SIGNATURE! Verify SIGNATURE ...\n");
    //sig_verify(CFG_UPGRADE_IMAGE_POS, SIGNATURE_OPERATION_VERIFY);
#else
    otp_check(otp_app_aes_key, NULL, OTP_OPERATION_AESK);
#endif
}