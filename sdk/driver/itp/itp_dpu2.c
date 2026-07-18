/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
#include <errno.h>
#include <malloc.h>
#include <string.h>

#include "ite/ith.h"
#include "itp_cfg.h"

/*******************************************************************

 *******************************************************************/
#define DPU_IRQ_ENABLE
#define ITP_DPU_LITTLE_ENDIAN   0x00000004
#define ITP_DPU_BIG_ENDIAN      0x00300004

/*******************************************************************

 *******************************************************************/
static const ITPDpuModeEnum DpuModeTable[] =
{
    ITP_DPU_AES_MODE,
    ITP_DPU_DES_MODE,
    ITP_DPU_DES3_MODE,
    ITP_DPU_CSA_MODE,
    ITP_DPU_CRC_MODE,
    ITP_DPU_UNKNOW_MODE
};

static const char * ItpDpuMode[] =
{
    "aes"// ,
    // "des",
    // "des3",
    // "csa",
    // "crc"
};

static ITP_DPU_INFO g_DpuInfo = {0};

#if defined(DPU_IRQ_ENABLE)
static sem_t        * DpuIsrSemaphor    = NULL;
static unsigned int g_IsrCnt            = 0;
#endif

unsigned char gDpuEnClk = 0;

#ifdef  CFG_DCPS_ENABLE
extern unsigned char    gDcpsEnClk;
#else
unsigned char           gDcpsEnClk = 0;
#endif

#ifdef CFG_NOR_USE_DPUAES
pthread_mutex_t DPU_AES_MUTEX = PTHREAD_MUTEX_INITIALIZER;
#endif

#if defined(DPU_IRQ_ENABLE)

void
dpu_isr (
    void * data)
{
    unsigned int tmp = 0;

    g_IsrCnt++;

    ithDpuGetStatus(&tmp);

    if ((tmp & 0x1) == 0x1)
    {
        itpSemPostFromISR(DpuIsrSemaphor);
    }
    else
    {
        ithPrintf("$--ith dpu warning~~\n");
    }

    ithDpuClearIntr();
}

void
DpuEnableIntr (
    void)
{
    ithIntrDisableIrq(ITH_INTR_DPU);
    ithIntrClearIrq(ITH_INTR_DPU);
    ithIntrRegisterHandlerIrq(ITH_INTR_DPU, dpu_isr, NULL);
    ithIntrSetTriggerModeIrq(ITH_INTR_DPU, ITH_INTR_EDGE);
    ithIntrSetTriggerLevelIrq(ITH_INTR_DPU, ITH_INTR_HIGH_RISING);
    ithIntrEnableIrq(ITH_INTR_DPU);

    if (!DpuIsrSemaphor)
    {
        DpuIsrSemaphor = malloc(sizeof(sem_t));
        sem_init(DpuIsrSemaphor, 0, 0);
    }

    ithDpuEnableIntr();
}

void
DpuDisableIntr (
    void)
{
    ithDpuDisableIntr();
    ithIntrDisableIrq(ITH_INTR_DPU);
    if (DpuIsrSemaphor)
    {
        sem_destroy(DpuIsrSemaphor);
        free(DpuIsrSemaphor);
        DpuIsrSemaphor = NULL;
    }
}
#endif

static void
dpuReverse (
    unsigned char   * buff,
    unsigned long   size)
{
    int             i       = 0;
    unsigned char   tmpByte = 0;

    for (i = 0; i < (size / 2); i++)
    {
        tmpByte             = buff[size - i - 1];
        buff[size - i - 1]  = buff[i];
        buff[i]             = tmpByte;
    }
}

static void
_InvalidDpuBuffer (
    uint8_t     * buf,
    uint32_t    size)
{
    ithFlushDCacheRange((void *)buf, size);
    ithFlushMemBuffer();
}

static void
DpuInit (
    void)
{
    g_DpuInfo.srcLen    = 0;
    g_DpuInfo.dstLen    = 0;
    g_DpuInfo.srcBuf    = NULL;
    g_DpuInfo.dstBuf    = NULL;

#if defined(DPU_IRQ_ENABLE)
    DpuEnableIntr();
#endif
}

ITH_DPU_MODE
GetIthDpuMode (
    ITP_DPU_INFO * Info)
{
    ITH_DPU_MODE res = UNKOWN_MODE;

    switch (ITP_DPU_AES_MODE)
    {
        case ITP_DPU_AES_MODE:
            switch (Info->cipher)
            {
                case ITP_DPU_CIPHER_ECB:
                    res = AES_ECB_MODE;
                    break;

                case ITP_DPU_CIPHER_CBC:
                    res = AES_CBC_MODE;
                    break;

                case ITP_DPU_CIPHER_CFB:
                    res = AES_CFB_MODE;
                    break;

                case ITP_DPU_CIPHER_OFB:
                    res = AES_OFB_MODE;
                    break;

                case ITP_DPU_CIPHER_CTR:
                    res = AES_CTR_MODE;
                    break;

                default:
                    (void)printf("[DPU ERROR] incorrect cipher mode of AES\n");
                    break;
            }
            break;

        default:
            (void)printf("[DPU ERROR] incorrect DPU mode\n");
            break;
    }

    return res;
}

static void
DpuWaitIdle (
    ITP_DPU_INFO * Info)
{
#if defined(DPU_IRQ_ENABLE)
    uint8_t EventRst = 0;

    if (Info->dpuLen > 1000)
    {
        EventRst = itpSemWaitTimeout(DpuIsrSemaphor, (unsigned long)(Info->dpuLen / 500));
    }
    else
    {
        EventRst = itpSemWaitTimeout(DpuIsrSemaphor, (unsigned long)2);
    }
    if (EventRst)
    {
        (void)printf("[ITP_DPU][ERR] itpSemWaitTimeout() error[%x]!!\n", EventRst);
    }

    EventRst = ithDpuWait();
#else
    bool EventRst = 0;

    EventRst = ithDpuWait();
#endif
}

static bool
HwDpuFire (
    ITP_DPU_INFO * Info)
{
    ithDpuSetSrcAddr((uint32_t)(Info->srcBuf));
    ithDpuSetDstAddr((uint32_t)(Info->dstBuf));
    ithDpuSetSize(Info->dpuLen);

    if (Info->descrypt)
    {
        ithDpuSetDescrypt();
    }
    else
    {
        ithDpuSetEncrypt();
    }

    if (!Info->descrypt)
    {
        _InvalidDpuBuffer(Info->srcBuf, Info->dpuLen);
    }

    ithInvalidateDCacheRange((uint32_t *)(Info->dstBuf), Info->dpuLen);
    ithDpuFire();
    DpuWaitIdle(Info);

    /*if (Info->descrypt)
        ithInvalidateDCacheRange((uint32_t *)(Info->dstBuf), Info->dpuLen);*/

    return true;
}

static int
DpuOpen (
    const char  * name,
    int         flags,
    int         mode,
    void        * info)
{
    int i = 0;

    if (name == NULL)
    {
        errno = (ITP_DEVICE_DPU << ITP_DEVICE_ERRNO_BIT) | __LINE__;
        ITP_LOG_ERR("[ITP_DPU_ERR][DpuOpen]name == NULL\n");
        return -1;
    }

    for (i = 0; i < ITH_COUNT_OF(ItpDpuMode); i++)
    {
        if (strcmp(name, ItpDpuMode[i]) == 0)
        {
            g_DpuInfo.dpuMode   = i;
            g_DpuInfo.descrypt  = 0;
            g_DpuInfo.cipher    = ITP_DPU_CIPHER_ECB;

            return i;
        }
    }

    errno = (ITP_DEVICE_DPU << ITP_DEVICE_ERRNO_BIT) | __LINE__;
    ITP_LOG_ERR("[ITP_DPU_ERR][DpuOpen]dpu not exist: %s\n", name);

    return -1;
}

static int
DpuClose (
    int     file,
    void    * info)
{
    g_DpuInfo.srcLen    = 0;
    g_DpuInfo.dstLen    = 0;

    return 0;
}

static int
DpuRead (
    int     file,
    char    * ptr,
    int     len,
    void    * info)
{
    int result = 0;

    if ((ptr == NULL) || (g_DpuInfo.srcBuf == NULL))
    {
        errno   = (ITP_DEVICE_DPU << ITP_DEVICE_ERRNO_BIT) | __LINE__;
        ITP_LOG_ERR("[ITP_DPU_ERR][DpuRead]buffer is NULL [%x,%x]\n", ptr, g_DpuInfo.srcBuf);
        result  = -1;
        goto end;
    }

    if (!len || !g_DpuInfo.dpuLen || ((uint32_t)len != g_DpuInfo.dpuLen))
    {
        errno   = (ITP_DEVICE_DPU << ITP_DEVICE_ERRNO_BIT) | __LINE__;
        ITP_LOG_ERR("[ITP_DPU_ERR][DpuRead]incorrect argument %x,%x,%x\n", len, g_DpuInfo.srcLen, g_DpuInfo.dstLen);
        result  = -1;
        goto end;
    }

    g_DpuInfo.dstBuf = ptr;

    if (HwDpuFire(&g_DpuInfo) == true)
    {
        result = g_DpuInfo.dpuLen;
    }
    else
    {
        errno   = (ITP_DEVICE_DPU << ITP_DEVICE_ERRNO_BIT) | __LINE__;
        ITP_LOG_ERR("[ITP_DPU_ERR][DpuRead]Decompress fail:\n");
        result  = -1;
        goto end;
    }

end:
    g_DpuInfo.dpuLen    = 0;
    g_DpuInfo.srcBuf    = NULL;
    g_DpuInfo.dstBuf    = NULL;

    return result;
}

static int
DpuWrite (
    int     file,
    char    * ptr,
    int     len,
    void    * info)
{
    int result = 0;

    if ((ptr == NULL) || !len)
    {
        errno   = (ITP_DEVICE_DPU << ITP_DEVICE_ERRNO_BIT) | __LINE__;
        ITP_LOG_ERR("[ITP_DPU_ERR]incorrect argument %x,%x\n", ptr, len);
        result  = -1;
        goto end;
    }

    g_DpuInfo.dpuLen    = len;
    g_DpuInfo.srcBuf    = ptr;

end:
    return result;
}

static int
DpuIoctl (
    int             file,
    unsigned long   request,
    void            * ptr,
    void            * info)
{
    switch (request)
    {
        case ITP_IOCTL_INIT:
            gDpuEnClk = 1;
            ithDpuEnableClock();
            DpuInit();
            break;

        case ITP_IOCTL_EXIT:
            gDpuEnClk = 0;
            if (!gDcpsEnClk)
            {
                ithDpuDisableClock();
            }
            break;

        case ITP_IOCTL_SET_DESCRYPT:
            g_DpuInfo.descrypt = *(unsigned char *)ptr;
            break;

        case ITP_IOCTL_SET_CIPHER:
            g_DpuInfo.cipher = *(unsigned char *)ptr;
            ithDpuClearCtrl();
            ithDpuInitCtrl(GetIthDpuMode(&g_DpuInfo));
            break;

        case ITP_IOCTL_SET_KEY_LENGTH:
            g_DpuInfo.keyLen = *(uint32_t *)ptr;
            break;

        case ITP_IOCTL_SET_VECTOR_LENGTH:
            g_DpuInfo.vctLen = *(uint32_t *)ptr;
            break;

        case ITP_IOCTL_SET_KEY_BUFFER:
            g_DpuInfo.keyBuf = (unsigned char *)ptr;
            ITP_DPU_INFO    * Info  = &g_DpuInfo;
            uint8_t         * buf   = (uint8_t *)malloc(16);

            if (Info->keyLen)
            {
                memcpy(buf, Info->keyBuf, Info->keyLen / 8);
                dpuReverse(buf, Info->keyLen / 8);
                ithDpuSetKey((uint32_t *)buf, Info->keyLen / 32);
            }

            if (Info->vctLen)
            {
                memcpy(buf, Info->vctBuf, Info->vctLen / 8);
                dpuReverse(buf, Info->vctLen / 8);
                ithDpuSetVector((uint32_t *)buf, Info->vctLen / 32);
            }

            free(buf);
            ithDpuSetDpuEndian(ITP_DPU_BIG_ENDIAN);
            break;

        case ITP_IOCTL_SET_VECTOR_BUFFER:
            g_DpuInfo.vctBuf = (unsigned char *)ptr;
            break;

        default:
            errno = (ITP_DEVICE_DPU << ITP_DEVICE_ERRNO_BIT) | 1;
            ITP_LOG_ERR("[ITP_DPU_ERR]DpuIoctl incorrect request (%x)\n", request);
            return -1;
    }

    return 0;
}

const ITPDevice itpDeviceDpu =
{
    ":dpu",
    DpuOpen,
    DpuClose,
    DpuRead,
    DpuWrite,
    itpLseekDefault,
    DpuIoctl,
    NULL
};
