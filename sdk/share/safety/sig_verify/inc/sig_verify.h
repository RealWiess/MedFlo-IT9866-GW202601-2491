#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include "ite/itp.h"
#include <string.h>
#include "nor/mmp_nor.h"
#include "mbedtls/sha256.h"
#include "mbedtls/md.h"
#include "mbedtls/rsa.h"
// #include "config.h"

#define SIG_DEBUG

#ifdef SIG_DEBUG
#define SIG_TRACE(...)    \
do \
{ \
 printf(__VA_ARGS__) ; \
} while(0)
#else
#define SIG_TRACE(...)    \
do \
{ \
} while(0)
#endif

#define SECURE_SET 1u
#define SECURE_PUBKEY_TOKEN "PKPK55AA55AAKPKP"
#define SECURE_PUBKEY_TOKEN_LEN (strlen(SECURE_PUBKEY_TOKEN))
#define SECURE_PUBKEY_SIZE 512u
#define SECURE_PUBKEY_PADDING_FLAG 0x80u
#define SECURE_PUBKEY_HASH_SIZE 32u
#define SECURE_SIG_SIZE 256u
#define SECURE_SIG_MAX_PADDING_SIZE 64u
#define SECURE_SIG_MAX_PADDING_SIZE2 72u
#define SECURE_CHECK_AREA (SECURE_PUBKEY_SIZE + SECURE_SIG_MAX_PADDING_SIZE + SECURE_SIG_SIZE)
#define SECURE_FLASH_READ_SIZE 0x10000u
#if defined(CFG_NAND_RESERVED_SIZE)
	#define SECURE_PUBKEY_FLASH_POS (CFG_NAND_RESERVED_SIZE - 0x40000) //NAND align size: 0x40000
#elif defined(CFG_NOR_RESERVED_SIZE)
	#define SECURE_PUBKEY_FLASH_POS (CFG_NOR_RESERVED_SIZE - 0x40000) //NOR align size: 0x40000
#endif

typedef enum SIGNATURE_OPERATION_TAG
{
    SIGNATURE_OPERATION_RSAPKH_GET,
    SIGNATURE_OPERATION_VERIFY
} SIGNATURE_OPERATION;

void sig_verify(uint32_t start_addr, SIGNATURE_OPERATION sig_op);
bool sig_verify_pubkey_get(uint8_t *pub_key);
bool sig_verify_pubkey_verify(uint32_t start_addr);
void sig_address_pubkey_hash(uint8_t *pub_key_hash);
bool sig_verify_boot(uint8_t *start_addr, uint32_t sig_padding_pos);
