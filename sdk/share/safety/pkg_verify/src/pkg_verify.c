#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include "ite/itp.h"
#include <string.h>
#include "mbedtls/md.h"
#include "mbedtls/rsa.h"
#include "mbedtls/md_internal.h"
#include "pkg_verify.h"
#include "sig_verify.h"

#define DEBUGMSG    0u

static uint32_t find_padding_offset_size(ITCStream *f, uint32_t len)
{
#define PADDING_FLAG 0x80u
#define MAXOFFSET 72u
    uint8_t buf_tmp[128]={0};
    uint32_t offset = 0, readsize = 0;

    if (itcStreamSeek((ITCStream*)f, -(int32_t)(len+MAXOFFSET), SEEK_END) != 0)
    {
        (void)printf("Cannot seek file\n");
        return offset;
    }
    readsize = (uint32_t)itcStreamRead((ITCStream*)f, buf_tmp, MAXOFFSET);
    if (readsize != MAXOFFSET)
    {
        (void)printf("find_padding_offset_size Cannot read file: %ld != %ld\n", readsize, MAXOFFSET);
        return offset;
    }
    offset = 8;
    for(int x = 63; x >= 0; x--)
    {
        offset++;
        if(buf_tmp[x] == PADDING_FLAG)
        {
            break;
        }
    }
#if DEBUGMSG
    (void)printf("sizeOffset=%d\n", offset);
#endif
    if (itcStreamSeek((ITCStream*)f, 0, SEEK_SET) != 0)
    {
        (void)printf("Cannot seek file\n");
    }
    return offset;
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

static bool get_pub_key_info(mbedtls_rsa_context *rsa)
{
    uint8_t t_pubkey[SECURE_PUBKEY_SIZE] = "";
    uint8_t p_pub_key_N[256], p_pub_key_E[256];

    if (sig_verify_pubkey_get(t_pubkey) == false)
    {
        return false;
    }
    endian_convert(t_pubkey, p_pub_key_E, 256);
	(void)mbedtls_mpi_read_binary( &rsa->E, p_pub_key_E, 256 );

	endian_convert(t_pubkey + 256, p_pub_key_N, 256);
	(void)mbedtls_mpi_read_binary( &rsa->N, p_pub_key_N, 256 );

    rsa->len = (mbedtls_mpi_bitlen(&rsa->N) + 7u) >> 3;
#if DEBUGMSG
    (void)printf("rsa.len=%d\n",rsa->len);
#endif
    return true;
}

static bool read_pkg_sig_content(ITCStream *f, mbedtls_rsa_context *rsa, uint8_t **sig_buf)
{
    uint32_t readsize = 0;
    if (itcStreamSeek((ITCStream*)f, -(int32_t)(rsa->len), SEEK_END) != 0)
    {
        (void)printf("Cannot seek file\n");
        return false;
    }

    *sig_buf = malloc(rsa->len);
    if( *sig_buf == NULL ) {
        (void)printf("Error: unable to allocate required memory\n");
        return false;
    }

    readsize = (uint32_t)itcStreamRead((ITCStream*)f, *sig_buf, rsa->len);
    if (readsize != rsa->len)
    {
        (void)printf("read_pkg_sig_content Cannot read file: %ld != %ld\n", readsize, rsa->len);
        return false;
    }
#if DEBUGMSG
    for(int x=0; x<readsize ; x++)
    {
        (void)printf("sig_buf[%d]=%02x\n", x, (*sig_buf)[x]);
    }
#endif
    return true;
}

static bool cal_pkg_hash(ITCStream *f, mbedtls_rsa_context *rsa, uint8_t *rsa_hash)
{
    bool ret = true;
    uint32_t filesize, bufsize, readsize, readsize2, sizeoffset;
    mbedtls_md_context_t ctx;
    uint8_t data_buf[1024];
    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

    if (itcStreamSeek((ITCStream*)f, 0, SEEK_SET) != 0)
    {
        (void)printf("Cannot seek file\n");
        return false;
    }

    mbedtls_md_init( &ctx );

    if( mbedtls_md_setup( &ctx, md_info, 0 ) != 0 )
    {
        ret = false;
        goto cleanup;
    }

    if( md_info->starts_func( ctx.md_ctx ) != 0 )
    {
        ret = false;
        goto cleanup;
    }

    sizeoffset = find_padding_offset_size(f, rsa->len);
    if (sizeoffset == 0u)
    {
        ret = false;
        (void)printf("Error padding_offset not find\n");
        goto cleanup;
    }

    filesize = (uint32_t)f->size - rsa->len - sizeoffset;
    bufsize = sizeof(data_buf);
#if DEBUGMSG
    (void)printf("file: %ld(%x) (%ld)\n", filesize, filesize, bufsize);
#endif
    do
    {
        readsize = (filesize < bufsize) ? filesize : bufsize;
        readsize2 = (uint32_t)itcStreamRead((ITCStream*)f, data_buf, readsize);
        if (readsize2 != readsize)
        {
            ret = false;
            (void)printf("cal_pkg_hash Cannot read file: %ld != %ld\n", readsize2, readsize);
            goto cleanup;
        }

        if( md_info->update_func( ctx.md_ctx, data_buf, readsize ) != 0 )
        {
            ret = false;
            goto cleanup;
        }

        filesize -= readsize;
#if DEBUGMSG
        (void)printf("filesize: %ld\n", filesize);
#endif
    }
    while (filesize > 0u);

    if(md_info->finish_func( ctx.md_ctx, rsa_hash ) != 0)
    {
        ret = false;
#if DEBUGMSG
        (void)printf("fail!\n");
#endif
    }
cleanup:
	mbedtls_md_free(&ctx);

    if (itcStreamSeek((ITCStream*)f, 0, SEEK_SET) != 0)
    {
        ret = false;
        (void)printf("Cannot seek file\n");
    }

    return ret;
}

bool pkgRSAVerify(ITCStream *f)
{
    bool ret;
    uint8_t *sig_buf = NULL;
    mbedtls_rsa_context rsa;
    uint8_t rsa_hash[SECURE_PUBKEY_HASH_SIZE] = { 0 };

    (void)printf("pkg rsa verify start, plz waiting...\n");
    // read key
    mbedtls_rsa_init(&rsa, MBEDTLS_RSA_PKCS_V15, 0);
    ret = get_pub_key_info(&rsa);
    if (ret == false)
    {
        (void)printf("Cannot get pub key\n");
        goto end;
    }

    //read sig
    ret = read_pkg_sig_content(f, &rsa, &sig_buf);
    if (ret == false)
    {
        (void)printf("read pkg sig fail\n");
        goto end;
    }

    /*
	* Compute the SHA-256 hash of the input file and
	* verify the signature
	*/
    ret = cal_pkg_hash(f, &rsa, rsa_hash);
	if(ret == false)
    {
        (void)printf("cal pkg hash failed\n");
		goto end;
	}
#if DEBUGMSG
    for(int x=0;x<32;x++)
    {
        (void)printf("rsa_hash[%d]=%x\n", x, rsa_hash[x]);
    }
    for(int x=0;x<rsa.len;x++)
    {
        (void)printf("sig_buf[%d]=%x\n", x, sig_buf[x]);
    }
#endif
	if (mbedtls_rsa_pkcs1_verify(&rsa, NULL, NULL, MBEDTLS_RSA_PUBLIC, MBEDTLS_MD_SHA256, 20, rsa_hash, sig_buf) != 0)
	{
		ret = false;
        (void)printf("pkg rsa check failed!\n");
		goto end;
	}
    (void)printf("pkg rsa check passed!\n");
end:
    mbedtls_rsa_free(&rsa);
    if(sig_buf != NULL)
    {
        free(sig_buf);
    }
#if DEBUGMSG
    (void)printf("pkg RSA Verify end!\n");
#endif
    return ret;
}


bool pkgRSAVerify_APP(ITCStream *f)
{
#ifndef CFG_UPGRADE_IMAGE_POS
#define CFG_UPGRADE_IMAGE_POS 0u
#endif
    if(sig_verify_pubkey_verify(CFG_UPGRADE_IMAGE_POS) == false)
    {
        return false;
    }
    return pkgRSAVerify(f);
}
