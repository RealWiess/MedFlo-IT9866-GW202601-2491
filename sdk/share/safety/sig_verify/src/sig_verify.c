#include "sig_verify.h"

#define MAX_HEADER_SIZE 0x10000
#define HEADER_LEN_OFFSET 12

static uint8_t pubkey[SECURE_PUBKEY_SIZE] = "";
static pthread_mutex_t pubkey_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint8_t sig[SECURE_SIG_SIZE] = "";
static uint8_t secure_check_area[SECURE_CHECK_AREA] = "";
static uint8_t firmware_first_upgrading = 0;
#ifdef CFG_NOR_USE_DPUAES
static uint8_t cipher_key[16] = {
    0xff, 0x00, 0x11, 0x22,
    0x33, 0x44, 0x55, 0x66,
    0x77, 0x88, 0x99, 0xaa,
    0xbb, 0xcc, 0xdd, 0xee
};
#endif
static uint8_t global_rsa_hash[SECURE_PUBKEY_HASH_SIZE] = {0};

static bool sig_verify_pubkey_set(uint8_t *pub_key)
{
	pthread_mutex_lock(&pubkey_mutex);
	memcpy(pubkey, pub_key, SECURE_PUBKEY_SIZE);
	pthread_mutex_unlock(&pubkey_mutex);
	return true;
}

static void endian_convert(uint8_t *src, uint8_t *dest, int length)
{
    src += length - 1;
    
    for (int i = 0; i < length; ++i) {
        *dest = *src;
        dest++;
        src--;
    }
}

static uint8_t *find_sig_start_position(uint8_t *p_end_of_pub_key, int buf_left_size)
{
	uint8_t *p_find = p_end_of_pub_key;
	
	while(1)
	{
		if(*(uint32_t*)(p_find+12))
		{
			return p_find + 16;
		}
		p_find += 16;
		buf_left_size -= 16;
		if(buf_left_size <= 0)
		{
			return NULL;
		}
	}
	return NULL;
}

static uint8_t *find_sig_start_position2(uint8_t *start_addr, uint32_t sig_padding_pos)
{
    uint8_t *padding_flag_addr = NULL, *sig_addr = NULL;
    uint32_t totalsize = 0, size_tmp = 0, size_addoffset = 0;

    padding_flag_addr = start_addr + sig_padding_pos;
    if (*padding_flag_addr != SECURE_PUBKEY_PADDING_FLAG) return NULL;

    // binaddsig algorithm
    totalsize = sig_padding_pos;
    size_tmp = totalsize % 64;
    if (size_tmp <= 55) size_addoffset = 56 - size_tmp;
    else size_addoffset = 120 - size_tmp;
    size_addoffset += 8;

    sig_addr = start_addr + sig_padding_pos + size_addoffset;

    return sig_addr;
}

static int RSA_verify(uint8_t *p_pub_key_start, uint8_t *p_sig_start, uint8_t *rsa_hash)
{
	int ret = -1;
	uint8_t p_pub_key_N[256], p_pub_key_E[256];

	if (firmware_first_upgrading)
	{
		SIG_TRACE("secure first upgrading of firmware!\n");
		ret = 0;
		return ret;
	}

	mbedtls_rsa_context rsa;
	mbedtls_rsa_init(&rsa, MBEDTLS_RSA_PKCS_V15, 0);
    
	endian_convert(pubkey, p_pub_key_E, 256);
	mbedtls_mpi_read_binary( &rsa.E, p_pub_key_E, 256 );

	endian_convert(pubkey + 256, p_pub_key_N, 256);
	mbedtls_mpi_read_binary( &rsa.N, p_pub_key_N, 256 );
	
	rsa.len = (mbedtls_mpi_bitlen(&rsa.N) + 7) >> 3;

	ret = mbedtls_rsa_pkcs1_verify(&rsa, NULL, NULL, MBEDTLS_RSA_PUBLIC,
		MBEDTLS_MD_SHA256, 20, rsa_hash, sig);

	mbedtls_rsa_free(&rsa);

	return ret;
}

static int msg_digest_meanwhile_find_pub_key_token(uint32_t *addr, uint32_t buffer_size, uint8_t *rsa_hash)
{
	int ret = -1, i, pub_key_left = 0, secure_buf_left = 0, fw_check_i = 0;
	uint8_t *p_find, *p_sig_start, *p_pub_key_start, *p_flash_read_back_buf;
	uint32_t start_addr = *addr;

	p_flash_read_back_buf = (uint8_t*)memalign(32, buffer_size);
	if(p_flash_read_back_buf == NULL)
	{
		SIG_TRACE("memalign failed!\n");
		return -1;
	}

	mbedtls_md_context_t ctx_md;
	const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
	mbedtls_md_init( &ctx_md );
	if( ( ret = mbedtls_md_setup( &ctx_md, md_info, 0 ) ) != 0 )
        goto fail_cleanup_md;
    if( ( ret = mbedtls_md_starts( &ctx_md ) ) != 0 )
        goto fail_cleanup_md;

#ifdef CFG_NOR_USE_DPUAES
	NorReadUID(SPI_0, SPI_CSN_0, cipher_key);
#endif

    for(i = 0;; i++)
	{
		*addr = start_addr + i * buffer_size;

		if (i == 0)
		{
			NorRead(SPI_0, NOR_CSN, *addr, p_flash_read_back_buf, buffer_size);
			for (fw_check_i = 0; fw_check_i < buffer_size; fw_check_i++)
				if (p_flash_read_back_buf[fw_check_i] != 0xFF)
					break;

			if (fw_check_i == buffer_size)
			{
				SIG_TRACE("secure first upgrading of firmware!\n");
				start_addr = 0;
				*addr = 0;
				firmware_first_upgrading = 1;
			}
		}

#ifdef CFG_NOR_USE_DPUAES
		if (!firmware_first_upgrading)
		{
			pthread_mutex_lock(&DPU_AES_MUTEX);
			NorSetAESKey(SPI_0, SPI_CSN_0, cipher_key);
			NorReadAES(SPI_0, SPI_CSN_0, *addr, p_flash_read_back_buf, buffer_size);
			pthread_mutex_unlock(&DPU_AES_MUTEX);
		}
		else
#endif
		NorRead(SPI_0, NOR_CSN, *addr, p_flash_read_back_buf, buffer_size);

		p_find = p_flash_read_back_buf;

 		while ( (p_pub_key_start = strstr(p_find, SECURE_PUBKEY_TOKEN)) == NULL)
   		{
			p_find = p_find + strlen(p_find) + 1;
			if(p_find > p_flash_read_back_buf + buffer_size)
			{
				p_find = NULL;
				break;
			}
   		}

 		if (p_pub_key_start) 
		{
            p_pub_key_start += SECURE_PUBKEY_TOKEN_LEN;

			pub_key_left = p_pub_key_start + SECURE_PUBKEY_SIZE - p_flash_read_back_buf - buffer_size;

			if (pub_key_left > 0)
			{
				if ((ret = mbedtls_md_update(&ctx_md, p_flash_read_back_buf, buffer_size)) != 0)
					goto fail_cleanup_md;
				memcpy(secure_check_area, p_pub_key_start, SECURE_PUBKEY_SIZE - pub_key_left);
				*addr += buffer_size;
#ifdef CFG_NOR_USE_DPUAES
				if (!firmware_first_upgrading)
				{
					pthread_mutex_lock(&DPU_AES_MUTEX);
					NorSetAESKey(SPI_0, SPI_CSN_0, cipher_key);
					NorReadAES(SPI_0, SPI_CSN_0, *addr, p_flash_read_back_buf, buffer_size);
					pthread_mutex_unlock(&DPU_AES_MUTEX);
				}
				else
#endif
				NorRead(SPI_0, NOR_CSN, *addr, p_flash_read_back_buf, buffer_size);
				if ((ret = mbedtls_md_update(&ctx_md, p_flash_read_back_buf, pub_key_left)) != 0)
					goto fail_cleanup_md;
				memcpy(secure_check_area + SECURE_PUBKEY_SIZE - pub_key_left, p_flash_read_back_buf, pub_key_left + SECURE_SIG_MAX_PADDING_SIZE + SECURE_SIG_SIZE);
			}
			else
			{
				if ((ret = mbedtls_md_update(&ctx_md, p_flash_read_back_buf, p_pub_key_start + SECURE_PUBKEY_SIZE - p_flash_read_back_buf)) != 0)
					goto fail_cleanup_md;
				secure_buf_left = p_pub_key_start + SECURE_CHECK_AREA - p_flash_read_back_buf - buffer_size;
				if(secure_buf_left > 0)
				{
					memcpy(secure_check_area, p_pub_key_start, SECURE_CHECK_AREA - secure_buf_left);
					*addr += buffer_size;
#ifdef CFG_NOR_USE_DPUAES
					if (!firmware_first_upgrading)
					{
						pthread_mutex_lock(&DPU_AES_MUTEX);
						NorSetAESKey(SPI_0, SPI_CSN_0, cipher_key);
						NorReadAES(SPI_0, SPI_CSN_0, *addr, p_flash_read_back_buf, buffer_size);
						pthread_mutex_unlock(&DPU_AES_MUTEX);
					}
					else
#endif
					NorRead(SPI_0, NOR_CSN, *addr, p_flash_read_back_buf, buffer_size);
					memcpy(secure_check_area + SECURE_CHECK_AREA - secure_buf_left, p_flash_read_back_buf, secure_buf_left);
				}
				else
				{
					memcpy(secure_check_area, p_pub_key_start, SECURE_CHECK_AREA);
				}
			}

			// memcpy(pubkey, secure_check_area, SECURE_PUBKEY_SIZE);
			/*if(!sig_verify_pubkey_set(secure_check_area))
			{
				printf("sig_verify_pubkey_set failed!\n");
				goto fail_cleanup_md;
			}*/


			p_sig_start = find_sig_start_position(secure_check_area + SECURE_PUBKEY_SIZE, SECURE_CHECK_AREA - SECURE_PUBKEY_SIZE);
			if(p_sig_start == NULL)
			{
				goto fail_cleanup_md;
			}
			memcpy(sig, p_sig_start, SECURE_SIG_SIZE);
            
	        ret = mbedtls_md_finish( &ctx_md, rsa_hash );
			break;
		}

		if( ( ret = mbedtls_md_update( &ctx_md, p_flash_read_back_buf, buffer_size ) ) != 0 )
            goto fail_cleanup_md;
	}
	mbedtls_md_free( &ctx_md );
	free(p_flash_read_back_buf);
	return 0;

fail_cleanup_md:
    mbedtls_md_free( &ctx_md );
	free(p_flash_read_back_buf);
	return -1;
}

static int calculate_pub_key_hash(const unsigned char *input, size_t i_len, unsigned char output[32], int is_224)
{
	return mbedtls_sha256_ret(input, i_len, output, is_224);
}

static int flash_integrity_check(uint32_t start_addr, SIGNATURE_OPERATION sig_op)
{	
	uint32_t addr = start_addr, buffer_size = SECURE_FLASH_READ_SIZE;
    uint8_t pub_key_hash[SECURE_PUBKEY_HASH_SIZE] = { 0 };

	if (sig_op == SIGNATURE_OPERATION_RSAPKH_GET)
	{
		if (msg_digest_meanwhile_find_pub_key_token(&addr, buffer_size, global_rsa_hash))
		{
			SIG_TRACE("message digest error, goto reboot!\n");
			return 1;
		}

		if (calculate_pub_key_hash(pubkey, SECURE_PUBKEY_SIZE, pub_key_hash, 0))
		{
			SIG_TRACE("sha256 error, goto reboot!\n");
			return 1;
		}
	}
	else if (sig_op == SIGNATURE_OPERATION_VERIFY)
	{
		if(RSA_verify(pubkey, sig, global_rsa_hash))
		{
			SIG_TRACE("rsa check failed!\n");
			return 1;
		}
		else
		{
			SIG_TRACE("rsa check passed!\n");
		}
	}
	else
	{
		SIG_TRACE("sig_op is unknown!\n");
		return 1;
	}

//end:
	return 0;
}

void sig_verify(uint32_t start_addr, SIGNATURE_OPERATION sig_op)
{
	int ret = -1;

	ret = flash_integrity_check(start_addr, sig_op);
	
	if(ret)
	{
		SIG_TRACE("flash integrity check failed, goto reboot!\n");
		sleep(1);
		exit(0);
	}
	else
	{
		SIG_TRACE("flash integrity check passed!\n");
	}

	return;
}

bool sig_verify_pubkey_get(uint8_t *pub_key)
{
	pthread_mutex_lock(&pubkey_mutex);
	memcpy(pub_key, pubkey, SECURE_PUBKEY_SIZE);
	pthread_mutex_unlock(&pubkey_mutex);
	return true;
}

bool sig_verify_pubkey_verify(uint32_t start_addr)
{
	uint32_t addr = start_addr, buffer_size = SECURE_FLASH_READ_SIZE;
    uint8_t pub_key_hash[SECURE_PUBKEY_HASH_SIZE] = { 0 };
    uint8_t rsa_hash[SECURE_PUBKEY_HASH_SIZE] = { 0 };

	if (msg_digest_meanwhile_find_pub_key_token(&addr, buffer_size, rsa_hash))
	{
		SIG_TRACE("pkey verify:message digest error!\n");
		return false;
	}
    
	if (calculate_pub_key_hash(pubkey, SECURE_PUBKEY_SIZE, pub_key_hash, 0))
	{
		SIG_TRACE("pkey verify:sha256 error!\n");
		return false;
	}

    return true;
}

void sig_address_pubkey_hash(uint8_t *pub_key_hash)
{
    int fd = -1;
    uint32_t blocksize = 0, gapsize = 0, pos = 0, alignsize = 0, blockcount = 0, ret = 0,
            headersize = 0, contentsize = 0, imagesize = 0, bufsize = 0, pos_remain = 0;
    uint8_t *rombuf = NULL, *imagebuf = NULL;
    bool compressed = false;

    fd = open(":nor", O_RDONLY, 0);
    if (fd == -1)
    {
        printf("[sig_address_pubkey_hash] fd error!\n");
        while (1) sleep(1);
    }

    if (ioctl(fd, ITP_IOCTL_GET_BLOCK_SIZE, &blocksize))
    {
        printf("[sig_address_pubkey_hash] get block size error\n");
        while (1) sleep(1);
    }

    if (ioctl(fd, ITP_IOCTL_GET_GAP_SIZE, &gapsize))
    {
        printf("[sig_address_pubkey_hash] get gap size error\n");
        while (1) sleep(1);
    }

    if (lseek(fd, pos, SEEK_SET) != pos)
    {
        printf("[sig_address_pubkey_hash] seek to rom position %d error\n", pos);
        while (1) sleep(1);
    }

    if (blocksize > MAX_HEADER_SIZE)
        alignsize = blocksize;
    else
        alignsize = MAX_HEADER_SIZE;

    rombuf = malloc(alignsize);
    if (!rombuf)
    {
        printf("[sig_address_pubkey_hash] out of memory\n");
        while (1) sleep(1);
    }

    blockcount = alignsize / blocksize;
    ret = read(fd, rombuf, blockcount);
    if (ret != blockcount)
    {
        printf("[sig_address_pubkey_hash] read rom header error: %d != %d\n", ret, blockcount);
        while (1) sleep(1);
    }

    headersize = *(uint32_t*)(rombuf + HEADER_LEN_OFFSET);
    headersize = itpBetoh32(headersize);
    if ((headersize % 4) || headersize > 100 * 1024)
    {
        printf("[sig_address_pubkey_hash] invalid header size: %d\n", headersize);
        while (1) sleep(1);
    }

    imagebuf = rombuf + headersize;
    if (strncmp(imagebuf, "SMAZ", 4) == 0)
    {
        // compressed rom
        contentsize = *(uint32_t*)(rombuf + HEADER_LEN_OFFSET + 4);
        contentsize = itpBetoh32(contentsize);
        compressed = true;
    }
    else
    {
        // non-compressed rom
        imagesize = *(uint32_t*)(rombuf + headersize - 4);
        imagesize = itpBetoh32(imagesize);
        contentsize = imagesize;
        compressed = false;
    }

    bufsize = headersize + contentsize;
    if (bufsize % 16) bufsize = bufsize + (16 - bufsize % 16);
    pos = bufsize / blocksize;
    pos_remain = bufsize % blocksize;
    blockcount = (pos_remain + SECURE_PUBKEY_TOKEN_LEN + SECURE_CHECK_AREA) / blocksize;
    if ((pos_remain + SECURE_PUBKEY_TOKEN_LEN + SECURE_CHECK_AREA) % blocksize) blockcount++;
    bufsize = blockcount * (blocksize + gapsize);

    rombuf = realloc(rombuf, bufsize);
    if (!rombuf)
    {
        printf("[sig_address_pubkey_hash] out of memory\n");
        while (1) sleep(1);
    }

    if (lseek(fd, pos, SEEK_SET) != pos)
    {
        printf("[sig_address_pubkey_hash] seek to rom position %d error\n", pos);
        while (1) sleep(1);
    }

    ret = read(fd, rombuf, blockcount);
    if (ret != blockcount)
    {
        printf("[sig_address_pubkey_hash] read rom image error: %d != %d\n", ret, blockcount);
        while (1) sleep(1);
    }

    if (memcmp(rombuf + pos_remain, SECURE_PUBKEY_TOKEN, SECURE_PUBKEY_TOKEN_LEN) != 0)
    {
        printf("[sig_address_pubkey_hash] SECURE_PUBKEY_TOKEN check error\n");
        while (1) sleep(1);
    }

    if (!sig_verify_pubkey_set(rombuf + pos_remain + SECURE_PUBKEY_TOKEN_LEN))
    {
        printf("[sig_address_pubkey_hash] failed!\n");
        while (1) sleep(1);
    }

    if (calculate_pub_key_hash(pubkey, SECURE_PUBKEY_SIZE, pub_key_hash, 0))
    {
        printf("[sig_address_pubkey_hash] sha256 error!\n");
        while (1) sleep(1);
    }

    if (fd != -1)
        close(fd);

    if (rombuf)
        free(rombuf);
}

bool sig_verify_boot(uint8_t *start_addr, uint32_t sig_padding_pos)
{
    bool result = false;
    int ret = 0;
    uint8_t *p_sig_start = NULL;
    mbedtls_md_context_t ctx_md = {0};
    const mbedtls_md_info_t *md_info = NULL;

    printf("sig_verify_boot start_addr:*%x*, sig_padding_pos:*%x*\n", start_addr, sig_padding_pos);
    p_sig_start = find_sig_start_position2(start_addr, sig_padding_pos);
    if (p_sig_start == NULL)
    {
        printf("[sig_verify_boot] p_sig_start error\n");
        return result;
    }
    printf("sig_verify_boot p_sig_start:*%x*\n", p_sig_start);
    memcpy(sig, p_sig_start, SECURE_SIG_SIZE);

    md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_init(&ctx_md);
    if (( ret = mbedtls_md_setup(&ctx_md, md_info, 0)) != 0) goto end;
    if (( ret = mbedtls_md_starts(&ctx_md)) != 0) goto end;
    if (( ret = mbedtls_md_update(&ctx_md, start_addr, sig_padding_pos)) != 0) goto end;
	if (( ret = mbedtls_md_finish(&ctx_md, global_rsa_hash)) != 0) goto end;
    mbedtls_md_free(&ctx_md);

    if (!RSA_verify(pubkey, sig, global_rsa_hash)) result = true;
    else printf("[sig_verify_boot] RSA_verify error\n");
    return result;

end:
    mbedtls_md_free(&ctx_md);
    return result;
}