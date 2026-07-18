/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL SD functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include <errno.h>
#include "itp_cfg.h"
#include "ite/ite_sd.h"

#define ITP_SD_OUTOF_POS        0x0101
#define ITP_SD_MAX_UNIT         4


typedef struct
{
    int index;
    uint32_t sectorCnt;
    uint32_t blockSize;
    uint16_t refCnt;
    uint16_t unit;
    uint32_t pos[ITP_SD_MAX_UNIT];
} ITPSd;

static int SdOpen(const char* name, int flags, int mode, void* info)
{
    ITPSd* ctxt = (ITPSd*)info;
    int i, unit_idx= -1;

    ithEnterCritical();
    for (i = 0; i < ITP_SD_MAX_UNIT; i++)
    {
        if ((ctxt->unit & (0x1U << i)) == 0x0UL)
        {
            unit_idx = i;
            ctxt->unit |= (0x1U << i);
            ctxt->refCnt++;
            ctxt->pos[unit_idx] = 0;
            break;
        }
    }
    ithExitCritical();

    if (unit_idx < 0)
    {
        ITP_LOG_ERR("sd%d SdOpen() refCnt > %d \n", ctxt->index, ITP_SD_MAX_UNIT);
    }

    return unit_idx;
}

static int SdClose(int file, void* info)
{
    ITPSd* ctxt = (ITPSd*)info;

    ithEnterCritical();
    ctxt->unit &= (uint16_t)~(0x1U << file);
    ctxt->refCnt--;
    ithExitCritical();

    return 0;
}

static int SdRead(int file, char *ptr, int len, void* info) // len: sector count
{
    ITPSd* ctxt = (ITPSd*)info;
    int res;
	int remains_sector, sec_cnt;
    uint8_t *buf = (uint8_t *)ptr;

    if ((ctxt->pos[file] + (uint32_t)len) > ctxt->sectorCnt)
    {
        len = (int)(ctxt->sectorCnt - ctxt->pos[file]);
        errno = (ctxt->index ? ITP_DEVICE_SD1 : ITP_DEVICE_SD0 << ITP_DEVICE_ERRNO_BIT) | ITP_SD_OUTOF_POS;
    }
    if (len == 0)
    {
        return len;
    }

    remains_sector = len;
    while (remains_sector != 0)
    {
        if (remains_sector > 512)
        {
            sec_cnt = 512;
        }
        else
        {
            sec_cnt = remains_sector;
        }

        res = iteSdReadMultipleSectorEx(ctxt->index, ctxt->pos[file], sec_cnt, (void*)buf);
        if (res != 0)
        {
            errno = (ctxt->index ? ITP_DEVICE_SD1 : ITP_DEVICE_SD0 << ITP_DEVICE_ERRNO_BIT) | res;
            break;
        }
        else
        {
            ctxt->pos[file] += (uint32_t)sec_cnt;
            buf += ((uint32_t)sec_cnt * ctxt->blockSize);
            remains_sector -= sec_cnt;
        }
    }

    return (len - remains_sector);
}

static int SdWrite(int file, char *ptr, int len, void* info)
{
    ITPSd* ctxt = (ITPSd*)info;
    int res;

    if ((ctxt->pos[file] + (uint32_t)len) > ctxt->sectorCnt)
    {
        len = (int)(ctxt->sectorCnt - ctxt->pos[file]);
        errno = (ctxt->index ? ITP_DEVICE_SD1 : ITP_DEVICE_SD0 << ITP_DEVICE_ERRNO_BIT) | ITP_SD_OUTOF_POS;
    }
    if (len == 0)
    {
        return len;
    }

    res = iteSdWriteMultipleSectorEx(ctxt->index, ctxt->pos[file], len, (void*)ptr);
    if(res != 0)
    {
        len = 0;
        errno = (ctxt->index ? ITP_DEVICE_SD1 : ITP_DEVICE_SD0 << ITP_DEVICE_ERRNO_BIT) | res;
    }
    else
    {
        ctxt->pos[file] += (uint32_t)len;
    }

    return len;
}

static int SdLseek(int file, int ptr, int dir, void* info)  // ptr: sector unit
{
    ITPSd* ctxt = (ITPSd*)info;
    switch(dir)
    {
    default:
    case SEEK_SET:
        ctxt->pos[file] = (uint32_t)ptr;
        break;
    case SEEK_CUR:
        ctxt->pos[file] += (uint32_t)ptr;
        break;
    case SEEK_END:
        break;
    }
    return (int)ctxt->pos[file];
}

static int SdIoctl(int file, unsigned long request, void* ptr, void* info)
{
    int res;
    ITPSd* ctxt = (ITPSd*)info;
    switch (request)
    {
    case ITP_IOCTL_INIT:
        {
            int retry = 3;
            #if 1//defined(CFG_MMC_ENABLE)
            {
                int rc;
                SD_CARD_INFO card_info = { 0 };
                rc = iteSdcInitialize(ctxt->index, &card_info);
                if (rc != 0)
                {
                    return -2;
                }
                if (card_info.type == SD_TYPE_SDIO)
                {
                    return -3;
                }
            }
            #endif

            do {
                res = iteSdInitializeEx(ctxt->index);
                retry--;
            } while((res != 0) && (retry > 0));
            if (res != 0)
            {
                errno = (ctxt->index ? ITP_DEVICE_SD1 : ITP_DEVICE_SD0 << ITP_DEVICE_ERRNO_BIT) | res;
                return -1;
            }
            iteSdGetCapacityEx(ctxt->index, &ctxt->sectorCnt, &ctxt->blockSize);
        }
        break;

    case ITP_IOCTL_EXIT:
        if (ctxt->refCnt != 0)
        {
            printf("[ITP][SD] Error exit! ==> refCnt(%d) != 0", ctxt->refCnt);
        }

        iteSdTerminateEx(ctxt->index);
        iteSdcTerminate(ctxt->index);
        break;

    case ITP_IOCTL_GET_BLOCK_SIZE:
        *(unsigned long*)ptr = ctxt->blockSize;
        break;

    case ITP_IOCTL_GET_GAP_SIZE:
        *(unsigned long*)ptr = 0;
        break;

    case ITP_IOCTL_GET_SPARE_SIZE:
        *(unsigned long *)ptr = 0;
        break;
        
    case ITP_IOCTL_ERASE:
    {
        ITPSdErase *param = (ITPSdErase *)ptr;
        res = iteSdEraseEx(ctxt->index, param->pos, param->count);
        if (res != 0)
        {
            errno = (ctxt->index ? ITP_DEVICE_SD1 : ITP_DEVICE_SD0 << ITP_DEVICE_ERRNO_BIT) | res;
            return -1;
        }
        break;
    }

    default:
        errno = (ctxt->index ? ITP_DEVICE_SD1 : ITP_DEVICE_SD0 << ITP_DEVICE_ERRNO_BIT) | 1;
        return -1;
    }
    return 0;
}
#ifdef CFG_SD0_ENABLE
static ITPSd itpSd0 = { SD_0, 0, 0, 0, 0, { 0 } };
    const ITPDevice itpDeviceSd0 =
    {
        ":sd0",
        SdOpen,
        SdClose,
        SdRead,
        SdWrite,
        SdLseek,
        SdIoctl,
        (void*)&itpSd0
    };
#endif // CFG_SD0_ENABLE

#ifdef CFG_SD1_ENABLE
    static ITPSd itpSd1 = { SD_1, 0, 0, 0, 0, { 0 } };
    const ITPDevice itpDeviceSd1 =
    {
        ":sd1",
        SdOpen,
        SdClose,
        SdRead,
        SdWrite,
        SdLseek,
        SdIoctl,
        (void*)&itpSd1
    };
#endif // CFG_SD1_ENABLE
