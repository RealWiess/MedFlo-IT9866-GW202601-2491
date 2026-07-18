/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL NOR functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include <errno.h>
#include <time.h>
#include "itp_cfg.h"
#include "nor/mmp_nor.h"
#include "ssp/mmp_spi.h"

#define NOR_SPI_PORT  SPI_0
#define NOR_SPI_CSN   NOR_CSN
#define NOR_OPEN_MAX  32

typedef struct _ITPNor
{
    int      initCount;
    uint32_t arrayUsage;
    int      seekPos[NOR_OPEN_MAX];
} ITPNor;

extern void iteFatNorFlush(void);
static unsigned long norBlockSizeInBytes;
static uint8_t  cipher_key[16] = {
    0xff, 0x00, 0x11, 0x22,
    0x33, 0x44, 0x55, 0x66,
    0x77, 0x88, 0x99, 0xaa,
    0xbb, 0xcc, 0xdd, 0xee
};

static int _NorOpen(const char *name, int flags, int mode, void *info)
{
    int i = -1;
    int j = 0;
    ITPNor *norObj = (ITPNor *)info;

    if (norObj)
    {
        for (j = 0; j < NOR_OPEN_MAX; j++)
        {
            ithEnterCritical();
            if (!(norObj->arrayUsage & 0x1 << j))
            {
                i = j;
                norObj->arrayUsage |= 0x1 << j;
                norObj->initCount++;
                ithExitCritical();
                norObj->seekPos[i] = 0;
                return i;
            }
            ithExitCritical();
        }

        errno = ENOENT;
        return i;
    }
    else
    {
        errno = ENOENT;
        return i;
    }
}

static int _NorClose(int file, void *info)
{
    ITPNor *norObj = (ITPNor *)info;

    if (norObj)
    {
        ithEnterCritical();
        norObj->arrayUsage &= ~(0x1 << file);
        norObj->initCount--;
        ithExitCritical();
        return 0;
    }
    else
    {
        errno = ENOENT;
        return -1;
    }
}

static int _NorRead(int file, char *ptr, int len, void *info)
{
    ITPNor *norObj = (ITPNor *)info;

    if (norObj)
    {
        uint32_t norResult  = 0;
        int32_t  norSize    = (int32_t)NorGetCapacity(NOR_SPI_PORT, NOR_SPI_CSN);
        int      readLength = len * norBlockSizeInBytes;

        if (norObj->seekPos[file] >= norSize)
        {
            return 0;
        }

        if ((norObj->seekPos[file] + (len * norBlockSizeInBytes)) >= norSize)
        {
            readLength = norSize - norObj->seekPos[file];
        }

    #ifdef CFG_NOR_SHARE_GPIO
        mmpSpiSetControlMode(SPI_CONTROL_NOR);
    #endif

        pthread_mutex_lock(&DPU_AES_MUTEX);

        norResult = NorSetAESKey(NOR_SPI_PORT, NOR_SPI_CSN, cipher_key);

        if (norResult == 0)
            norResult = NorReadAES(NOR_SPI_PORT, NOR_SPI_CSN, norObj->seekPos[file], ptr, readLength);

        pthread_mutex_unlock(&DPU_AES_MUTEX);

    #ifdef CFG_NOR_SHARE_GPIO
        mmpSpiResetControl();
    #endif

        if (norResult == 0)
        {
            // Success
            norObj->seekPos[file] += readLength;
            return readLength / norBlockSizeInBytes;
        }
        else
        {
            errno = EIO;
            return -1;
        }
    }
    else
    {
        errno = EBADF;
        return -1;
    }
}

static int _NorWrite(int file, char *ptr, int len, void *info)
{
    ITPNor *norObj = (ITPNor *)info;

    if (norObj)
    {
        uint32_t norResult   = 0;
        int32_t  norSize     = (int32_t)NorGetCapacity(NOR_SPI_PORT, NOR_SPI_CSN);
        int	     writeLength = len * norBlockSizeInBytes;

        if (norObj->seekPos[file] >= norSize)
        {
            return 0;
        }

        if ((norObj->seekPos[file] + (len * norBlockSizeInBytes)) >= norSize)
        {
            writeLength = norSize - norObj->seekPos[file];
        }

    #ifdef CFG_NOR_SHARE_GPIO
        mmpSpiSetControlMode(SPI_CONTROL_NOR);
    #endif

        pthread_mutex_lock(&DPU_AES_MUTEX);

        norResult = NorSetAESKey(NOR_SPI_PORT, NOR_SPI_CSN, cipher_key);

        if (norResult == 0)
            norResult = NorWriteAES(NOR_SPI_PORT, NOR_SPI_CSN, norObj->seekPos[file], ptr, writeLength);

        pthread_mutex_unlock(&DPU_AES_MUTEX);

    #ifdef CFG_NOR_SHARE_GPIO
        mmpSpiResetControl();
    #endif

        if (norResult == 0)
        {
            // Success
            norObj->seekPos[file] += writeLength;
            return writeLength / norBlockSizeInBytes;
        }
        else
        {
            errno = EIO;
            return -1;
        }
    }
    else
    {
        errno = EBADF;
        return -1;
    }
}

static int _NorLseek(int file, int ptr, int dir, void *info)
{
    ITPNor *norObj = (ITPNor *)info;

    if (norObj)
    {
        switch (dir)
        {
        case SEEK_SET:
            norObj->seekPos[file] = ptr * norBlockSizeInBytes;
            break;

        case SEEK_CUR:
            norObj->seekPos[file] += ptr * norBlockSizeInBytes;
            break;

        case SEEK_END:
            {
                uint32_t norSize = NorGetCapacity(NOR_SPI_PORT, NOR_SPI_CSN);

                norObj->seekPos[file] = norSize;
                norObj->seekPos[file] += ptr * (int)norBlockSizeInBytes;
            }
            break;

        default:
            return -1;
            break;
        }

        return norObj->seekPos[file] / norBlockSizeInBytes;
    }
    else
    {
        return -1;
    }
}

#if defined(CFG_FS_FAT) && CFG_NOR_CACHE_FLUSH_INTERVAL > 0

static struct timeval norLastTime;

static void NorFlushHandler(void)
{
    struct timeval currTime;

    gettimeofday(&currTime, NULL);

    if (itpTimevalDiff(&norLastTime, &currTime) >= CFG_NOR_CACHE_FLUSH_INTERVAL * 1000)
    {
        ITP_LOG_DBG("Flush NOR cache!\n" );
        iteFatNorFlush();
        gettimeofday(&norLastTime, NULL);
    }
}
#endif // defined(CFG_FS_FAT) && CFG_NOR_CACHE_FLUSH_INTERVAL > 0

static void NorInit(void)
{
    int i = 0;

#if defined(CFG_FS_FAT) && CFG_NOR_CACHE_FLUSH_INTERVAL > 0
    //itpRegisterIdleHandler(NorFlushHandler);
    gettimeofday(&norLastTime, NULL);
#endif // defined(CFG_FS_FAT) && CFG_NOR_CACHE_FLUSH_INTERVAL > 0

    //NorInitial(NOR_SPI_PORT, NOR_SPI_CSN);
    norBlockSizeInBytes = NorGetAttitude(NOR_SPI_PORT, NOR_SPI_CSN, NOR_ATTITUDE_PAGE_SIZE) * NorGetAttitude(NOR_SPI_PORT, NOR_SPI_CSN, NOR_ATTITUDE_PAGE_PER_SECTOR) * NorGetAttitude(NOR_SPI_PORT, NOR_SPI_CSN, NOR_ATTITUDE_SECTOR_PER_BLOCK);

    if (NorReadUID(NOR_SPI_PORT, NOR_SPI_CSN, cipher_key) == 0)
    {
        /*printf("FW key (NOR UID) is:\n");
        for (i = 0; i < sizeof(cipher_key); i++)
            printf("[%d]:*%x*\n", i, cipher_key[i]);*/
    }
    else
    {
        printf("read FW key (NOR UID) fail!\n");
    }
}

static int _NorIoctl(int file, unsigned long request, void *ptr, void *info)
{
    switch (request)
    {
    case ITP_IOCTL_FLUSH:
    #ifdef CFG_FS_FAT
        iteFatNorFlush();
    #endif
        break;

    case ITP_IOCTL_INIT:
        NorInit();
        break;

    case ITP_IOCTL_GET_BLOCK_SIZE:
        *(unsigned long *)ptr = norBlockSizeInBytes;
        break;

    case ITP_IOCTL_GET_GAP_SIZE:
        *(unsigned long *)ptr = 0;
        break;

    case ITP_IOCTL_GET_SPARE_SIZE:
        *(unsigned long *)ptr = 0;
        break;
        
    default:
        errno = (ITP_DEVICE_NORAES << ITP_DEVICE_ERRNO_BIT) | 1;
        return -1;
    }
    return 0;
}

static ITPNor ItpNor = {0, 0, {0}};
const ITPDevice itpDeviceNorAES =
{
    ":norAES",
    _NorOpen,
    _NorClose,
    _NorRead,
    _NorWrite,
    _NorLseek,
    _NorIoctl,
    &ItpNor
};
