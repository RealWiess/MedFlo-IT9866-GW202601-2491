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
#if (CFG_LFS_CACHE_SIZE > 0)
#include <sys/ioctl.h>
#endif

#define NOR_SPI_PORT  SPI_0
#define NOR_SPI_CSN   NOR_CSN
#define NOR_OPEN_MAX  32

typedef struct ITPNor_
{
    int      initCount;
    uint32_t arrayUsage;
    int      seekPos[NOR_OPEN_MAX];
} ITPNor;

#ifdef CFG_FS_FAT
extern void iteFatNorFlush(void);
#endif
static unsigned long norBlockSizeInBytes;

static int NorOpen_(const char *name, int flags, int mode, void *info)
{
    int i = -1;
    int j = 0;
    ITPNor *norObj = (ITPNor *)info;

    if (norObj != NULL)
    {
        for (j = 0; j < NOR_OPEN_MAX; j++)
        {
            ithEnterCritical();
            if ((norObj->arrayUsage & ((uint32_t)0x1U << (uint32_t)j)) == 0x0U)
            {
                i = j;
                norObj->arrayUsage |= (uint32_t)0x1U << (uint32_t)j;
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

static int NorClose_(int file, void *info)
{
    ITPNor *norObj = (ITPNor *)info;

    if (norObj != NULL)
    {
        ithEnterCritical();
        norObj->arrayUsage &= ~((uint32_t)0x1U << (uint32_t)file);
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

static int NorRead_(int file, char *ptr, int len, void *info)
{
    ITPNor *norObj = (ITPNor *)info;

    if (norObj != NULL)
    {
        uint32_t norResult  = 0;
        int32_t  norSize    = (int32_t)NorGetCapacity(NOR_SPI_PORT, NOR_SPI_CSN);
        int      readLength = len * (int)norBlockSizeInBytes;

        if (norObj->seekPos[file] >= norSize)
        {
            return 0;
        }

        if ((norObj->seekPos[file] + (len * (int)norBlockSizeInBytes)) >= norSize)
        {
            readLength = norSize - norObj->seekPos[file];
        }

    #ifdef CFG_NOR_SHARE_GPIO
        mmpSpiSetControlMode(SPI_CONTROL_NOR);
    #endif

        norResult = NorRead(NOR_SPI_PORT, NOR_SPI_CSN, (uint32_t)(norObj->seekPos[file]), (uint8_t *)ptr, (uint32_t)readLength);

    #ifdef CFG_NOR_SHARE_GPIO
        mmpSpiResetControl();
    #endif

        if (norResult == 0U)
        {
            // Success
            norObj->seekPos[file] += readLength;
            return readLength / (int)norBlockSizeInBytes;
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

static int NorWrite_(int file, char *ptr, int len, void *info)
{
    ITPNor *norObj = (ITPNor *)info;

    if (norObj != NULL)
    {
        uint32_t norResult   = 0;
        int32_t  norSize     = (int32_t)NorGetCapacity(NOR_SPI_PORT, NOR_SPI_CSN);
        int	     writeLength = len * (int)norBlockSizeInBytes;

        if (norObj->seekPos[file] >= norSize)
        {
            return 0;
        }

        if ((norObj->seekPos[file] + (len * (int)norBlockSizeInBytes)) >= norSize)
        {
            writeLength = norSize - norObj->seekPos[file];
        }

    #ifdef CFG_NOR_SHARE_GPIO
        mmpSpiSetControlMode(SPI_CONTROL_NOR);
    #endif

        norResult = NorWrite(NOR_SPI_PORT, NOR_SPI_CSN, (uint32_t)norObj->seekPos[file], (uint8_t *)ptr, (uint32_t)writeLength);

    #ifdef CFG_NOR_SHARE_GPIO
        mmpSpiResetControl();
    #endif

        if (norResult == 0U)
        {
            // Success
            norObj->seekPos[file] += writeLength;
            return writeLength / (int)norBlockSizeInBytes;
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

static int NorLseek_(int file, int ptr, int dir, void *info)
{
    ITPNor *norObj = (ITPNor *)info;
    uint32_t tmp = 0U;

    if (norObj != NULL)
    {
        switch (dir)
        {
        case SEEK_SET:
            norObj->seekPos[file] = ptr * (int)norBlockSizeInBytes;
            break;

        case SEEK_CUR:
            norObj->seekPos[file] += ptr * (int)norBlockSizeInBytes;
            break;

        case SEEK_END:
            {
                uint32_t norSize = NorGetCapacity(NOR_SPI_PORT, NOR_SPI_CSN);

                norObj->seekPos[file] = (int)norSize;
                norObj->seekPos[file] += ptr * (int)norBlockSizeInBytes;
            }
            break;

        default:
            tmp = 1U;
            break;
        }

        if (tmp == 1U) {return -1;}
        else {return norObj->seekPos[file] / (int)norBlockSizeInBytes;}
    }
    else
    {
        return -1;
    }
}

#if defined(CFG_FS_FAT) && (CFG_NOR_CACHE_FLUSH_INTERVAL > 0)

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
    uint32_t tmp = 0U;

#if defined(CFG_FS_FAT) && (CFG_NOR_CACHE_FLUSH_INTERVAL > 0)
    itpRegisterIdleHandler(NorFlushHandler);
    gettimeofday(&norLastTime, NULL);
#endif // defined(CFG_FS_FAT) && CFG_NOR_CACHE_FLUSH_INTERVAL > 0

    (void)NorInitial(NOR_SPI_PORT, NOR_SPI_CSN);
    tmp = NorGetAttitude(NOR_SPI_PORT, NOR_SPI_CSN, NOR_ATTITUDE_PAGE_SIZE);
    tmp *= NorGetAttitude(NOR_SPI_PORT, NOR_SPI_CSN, NOR_ATTITUDE_PAGE_PER_SECTOR);
    tmp *= NorGetAttitude(NOR_SPI_PORT, NOR_SPI_CSN, NOR_ATTITUDE_SECTOR_PER_BLOCK);
    norBlockSizeInBytes = tmp;
}

static int NorIoctl_(int file, unsigned long request, void *ptr, void *info)
{
    unsigned long tmp = 0UL;

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
#if 0
        errno = (ITP_DEVICE_NOR << ITP_DEVICE_ERRNO_BIT) | 1;
#endif
        tmp = ((17UL << ITP_DEVICE_BIT) << ITP_DEVICE_ERRNO_BIT) | 1UL;
        errno = (int)tmp;
        break;
    }

    if (tmp == 0UL) {return 0;}
    else {return -1;}
}

static ITPNor ItpNor = {0, 0, {0}};
const ITPDevice itpDeviceNor =
{
    ":nor",
    &NorOpen_,
    &NorClose_,
    &NorRead_,
    &NorWrite_,
    &NorLseek_,
    &NorIoctl_,
    &ItpNor
};
