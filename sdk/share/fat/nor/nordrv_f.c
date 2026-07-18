#include "nordrv_f.h"
#include "ite/ith.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cache2.h"
#include "nor/mmp_nor.h"
#include "ssp/mmp_spi.h"
#include <pthread.h>
#include "ite/itp.h"

#define ALIGN_RW_BYTES (0)  // Safe but slower
/*#ifdef CFG_NOR_USE_DPUAES
#include "ite/itp.h"
#endif*/

typedef struct
{
    unsigned long reserved;           // reserved size in byte
    unsigned long cacheSize;          // norCache size
    bool          deviceMode;         // device mode
    unsigned long partitionOffset;    // partition offset
} NORDrvParam;

typedef struct _NOR_CONTEXT
{
    unsigned long   reservrdFatSectors;	// reserved size in fat sector
	unsigned long   pageSize;			// page size in byte
	unsigned long	pagesPerSector;
	unsigned long	sectorsPerBlock;
	unsigned long	blockSize;
	unsigned long	fatSectorSize;
	unsigned long   blockSizeInBytes;
	unsigned long   fatSectorsPerBlock;
    pthread_mutex_t pageAccessMutex;
    pthread_mutex_t flushAccessMutex;
}NOR_CONTEXT;

static F_DRIVER		nor_drv;
static NOR_CACHE*	norCache;
NOR_CONTEXT	NorContext;
bool		CacheValidFlush = false; // Cache flush sync flag
/*#ifdef CFG_NOR_USE_DPUAES
static uint8_t  cipher_key[16] = {
    0xff, 0x00, 0x11, 0x22,
    0x33, 0x44, 0x55, 0x66,
    0x77, 0x88, 0x99, 0xaa,
    0xbb, 0xcc, 0xdd, 0xee
};
#endif*/

static bool readSectors(void* drvInfo, sec_t sector, sec_t numSectors, void* buffer);
static bool writeSectors(void* drvInfo, sec_t sector, sec_t numSectors, const void* buffer);
static bool writeBytes(unsigned long addr, unsigned long numBytes, const void* buffer);
static bool readBytes(unsigned long addr, unsigned long numBytes, const void* buffer);
static bool eraseBlock(unsigned long block);

void iteFatNorFlush(void)
{	
    if (norCache)
    {
        _NOR_cache_flush(norCache);
    }
}

int nor_getCapacity(unsigned int* sec_num, unsigned int* block_size)
{
	*sec_num = NorGetAttitude(SPI_0, SPI_CSN_0, NOR_ATTITUDE_TOTAL_SIZE)*NorGetAttitude(SPI_0, SPI_CSN_0, NOR_ATTITUDE_PAGE_SIZE)/512;
	*block_size = 512;
	//printf("sec_num = %d, block_size=%d \n", *sec_num, *block_size);
	return 0;
}

/****************************************************************************
 *
 * nor_readmultiplesector
 *
 * read multiple sectors from nordrive
 *
 * INPUTS
 *
 * driver - driver structure
 * data - data pointer where to store data
 * sector - where to read data from
 * cnt - number of sectors to read
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int nor_readmultiplesector(F_DRIVER *driver, void *data, unsigned long sector, int cnt)
{
    NORDrvParam *drvParam = NULL;

    if (!driver) return readSectors(NULL, sector, cnt, data) ? 0 : 1;

    drvParam = (NORDrvParam *)driver->user_ptr;

    if (!drvParam->deviceMode)
    {
        if (norCache)
        {
            return _NOR_cache_readSectors(norCache, sector, cnt, data) ? 0 : 1;
        }
        else
        {
            return readSectors(NULL, sector, cnt, data) ? 0 : 1;
        }
    }
    else
    {
        return readSectors(NULL, drvParam->partitionOffset + sector, cnt, data) ? 0 : 1;
    }
}

/****************************************************************************
 * Read one sector
 ***************************************************************************/

static int nor_readsector(F_DRIVER *driver,void *data, unsigned long sector)
{
    return nor_readmultiplesector(driver, data, sector, 1);
}

/****************************************************************************
 *
 * nor_readbytes
 *
 * read bytes from nordrive, if less than one sector and cache miss, direct read
 *
 * INPUTS
 *
 * driver - driver structure
 * data - data pointer where to store data
 * addr - where to read data from
 * cnt - number of bytes to read
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int nor_readbytes(F_DRIVER *driver, void *data, unsigned long addr, int cnt)
{
    NORDrvParam *drvParam = NULL;

    if (!driver) return readBytes(addr, cnt, data) ? 0 : 1;

    drvParam = (NORDrvParam *)driver->user_ptr;

    if (!drvParam->deviceMode)
    {
        if (norCache)
        {
            sec_t sec = addr / NorContext.fatSectorSize;
            unsigned long offset = addr % NorContext.fatSectorSize;
#if ALIGN_RW_BYTES
            unsigned long remaining = cnt;
            unsigned char *dest = (unsigned char *)data;

            while (remaining > 0) {
                unsigned long bytes_to_read = NorContext.fatSectorSize - offset;
                if (bytes_to_read > remaining) {
                    bytes_to_read = remaining;
                }
                
                if (!_NOR_cache_readExistedPartialSector(norCache, dest, sec, offset, bytes_to_read)) {
                    int ret = readBytes(addr, bytes_to_read, dest) ? 0 : 1;
                    return ret;
                }
    
                remaining -= bytes_to_read;
                dest += bytes_to_read;
                addr += bytes_to_read;
                sec++;
                offset = 0; // 之后的sector都是从0开始
            }
            return 0;
#else
            if(cnt < NorContext.fatSectorSize || offset != 0)
            {
                if(!_NOR_cache_readExistedPartialSector(norCache, data, sec, offset, cnt))
                    return readBytes(addr, cnt, data) ? 0 : 1;                
                return 0;
            }
            else
            {
                return _NOR_cache_readSectors(norCache, sec, cnt/NorContext.fatSectorSize, data) ? 0 : 1;       
            }        
#endif                
        }
        else
        {
            return readBytes(addr, cnt, data) ? 0 : 1;
        }
    }
    else
    {
        return readBytes(drvParam->partitionOffset + addr, cnt, data) ? 0 : 1;
    }
}

/****************************************************************************
 *
 * nor_writemultiplesector
 *
 * write multiple sectors into nordrive
 *
 * INPUTS
 *
 * driver - driver structure
 * data - data pointer
 * sector - where to write data
 * cnt - number of sectors to write
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int nor_writemultiplesector(F_DRIVER *driver, void *data, unsigned long sector, int cnt)
{
    NORDrvParam *drvParam = NULL;

    if (!driver) return writeSectors(NULL, sector, cnt, data) ? 0 : 1;

    drvParam = (NORDrvParam *)driver->user_ptr;

    if (!drvParam->deviceMode)
    {
        if (norCache)
        {
            return _NOR_cache_writeSectors(norCache, sector, cnt, data) ? 0 : 1;
        }
        else
        {
            return writeSectors(NULL, sector, cnt, data) ? 0 : 1;
        }
    }
    else
    {
        return writeSectors(NULL, drvParam->partitionOffset + sector, cnt, data) ? 0 : 1;
    }
}

/****************************************************************************
 *
 * nor_writebytes
 *
 * write bytes into nordrive, without erase, if cache miss, direct write
 *
 * INPUTS
 *
 * driver - driver structure
 * data - data pointer
 * addr - where to write data
 * cnt - number of bytes to write
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int nor_writebytes(F_DRIVER *driver, void *data, unsigned long addr, int cnt)
{
    NORDrvParam *drvParam = NULL;

    if (!driver) return writeBytes(addr, cnt, data) ? 0 : 1;

    drvParam = (NORDrvParam *)driver->user_ptr;

    if (!drvParam->deviceMode)
    {
        if (norCache)
        {
            sec_t secStart = addr / NorContext.fatSectorSize, secEnd = (addr + cnt) / NorContext.fatSectorSize, sec;
            int remain;
            unsigned char *ptr = data;
            unsigned long writeAddr = addr;
            if(secEnd == secStart) secEnd ++;
            remain = cnt;
            for(sec = secStart ; sec < secEnd ; sec ++)
            {
                unsigned int offset;
                size_t size;                
                offset = writeAddr % NorContext.fatSectorSize;
                size = offset + remain > NorContext.fatSectorSize ? ((offset + remain) % NorContext.fatSectorSize) : remain;
                writeAddr += size;
                if(!_NOR_cache_writeExistedPartialSector(norCache, ptr, sec, offset, size, true))
                {
                    // cache miss, ignore
                }
                ptr += size;
                remain -= size;
            }
            return writeBytes(addr, cnt, data) ? 0 : 1;
        }
        else
        {
            return writeBytes(addr, cnt, data) ? 0 : 1;
        }
    }
    else
    {
        return writeBytes(drvParam->partitionOffset * NorContext.fatSectorSize + addr, cnt, data) ? 0 : 1;
    }
}

/****************************************************************************
 * Write one sector
 ***************************************************************************/

static int nor_writesector(F_DRIVER *driver,void *data, unsigned long sector)
{
	return nor_writemultiplesector(driver, data, sector, 1);
}

/****************************************************************************
 *
 * nor_eraseblock
 *
 * erase block, if cached, invalid it
 *
 * INPUTS
 *
 * driver - driver structure
 * block - block number to erase
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

int nor_eraseblock(F_DRIVER *driver, unsigned long block)
{
    NORDrvParam *drvParam = NULL;
    if (!driver) return eraseBlock(block) ? 0 : 1;

    drvParam = (NORDrvParam *)driver->user_ptr;

    if (!drvParam->deviceMode)
    {
        if (norCache)
        {
            sec_t sec;
            for(sec = block * NorContext.fatSectorsPerBlock ; sec < (block + 1) * NorContext.fatSectorsPerBlock ; sec ++)
            {
                _NOR_cache_invalidateSector(norCache, sec);
            }
            return eraseBlock(block) ? 0 : 1;
        }
        else
        {
            return eraseBlock(block) ? 0 : 1;
        }
    }
    else
    {
        return eraseBlock((drvParam->partitionOffset * NorContext.fatSectorSize)/NorContext.blockSizeInBytes + block) ? 0 : 1;
    }
}
/****************************************************************************
 *
 * nor_getphy
 *
 * determinate nordrive physicals
 *
 * INPUTS
 *
 * driver - driver structure
 * phy - this structure has to be filled with physical information
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/

static int nor_getphy(F_DRIVER *driver,F_PHY *phy)
{
	if ( phy )
	{
		phy->bytes_per_sector  = NorContext.fatSectorSize;
		//phy->number_of_sectors = (NorContext.blockSize > 0 ? NorContext.blockSize - 1 : NorContext.blockSize) * NorContext.fatSectorsPerBlock - NorContext.reservrdFatSectors;
		phy->number_of_sectors = NorContext.blockSize * NorContext.fatSectorsPerBlock - NorContext.reservrdFatSectors;
        phy->sectors_per_cluster = NorContext.blockSizeInBytes / NorContext.fatSectorSize;
        phy->reserved_sectors = NorContext.reservrdFatSectors;
	}
	
	return NOR_NO_ERROR;
}

/****************************************************************************
 *
 * nor_release
 *
 * Releases a drive
 *
 * INPUTS
 *
 * driver_panor - driver panoreter
 *
 ***************************************************************************/

static void nor_release (F_DRIVER *driver)
{
	if (norCache)
    {
        _NOR_cache_destructor(norCache);
        norCache = NULL;
    }

    pthread_mutex_lock(&NorContext.pageAccessMutex);
    pthread_mutex_unlock(&NorContext.pageAccessMutex);

    if (NorContext.pageAccessMutex)
    {
        pthread_mutex_destroy(&NorContext.pageAccessMutex);
    }

	/* Disable norCache flush */
    pthread_mutex_lock(&NorContext.flushAccessMutex);
    CacheValidFlush = false;
    pthread_mutex_unlock(&NorContext.flushAccessMutex);

    if (NorContext.flushAccessMutex)
    {
        pthread_mutex_destroy(&NorContext.flushAccessMutex);
    }
}

/****************************************************************************
 *
 * nor_ioctl
 *
 * NOR drive ioctl 
 *
 * INPUTS
 *
 * driver_panor - driver panoreter
 *
 ***************************************************************************/

static int nor_ioctl (F_DRIVER *driver, unsigned long msg, void *iparam, void *oparam)
{
	switch (msg)
	{
    case F_IOCTL_MSG_FLUSH:
        iteFatNorFlush();
        break;
    default :
        printf("nor_ioctl unknown msg %X\n", msg);
        break;
    }
    return 0;
}

/****************************************************************************
 *
 * nor_initfunc
 *
 * this init function has to be passed for highlevel to initiate the
 * driver functions
 *
 * INPUTS
 *
 * driver_panor - driver panoreter
 *
 * RETURNS
 *
 * driver structure pointer
 *
 ***************************************************************************/

static bool readSectors(void* drvInfo, sec_t sector, sec_t numSectors, void* buffer)
{
    uint32_t norResult = 0;

#if defined(CFG_NOR_SHARE_GPIO) || defined(CFG_NOR_SHARE_SPI_NAND)
    //mmpSpiSetControlMode(SPI_CONTROL_NOR);
#endif

/*#ifdef CFG_NOR_USE_DPUAES
    pthread_mutex_lock(&DPU_AES_MUTEX);

    norResult = NorSetAESKey(SPI_0, SPI_CSN_0, cipher_key);

    if (norResult == 0)
        norResult = NorReadAES(SPI_0, SPI_CSN_0, (NorContext.reservrdFatSectors + sector) * NorContext.fatSectorSize, buffer, numSectors * NorContext.fatSectorSize);

    pthread_mutex_unlock(&DPU_AES_MUTEX);
#else*/
	norResult = NorRead(SPI_0, NOR_CSN, (NorContext.reservrdFatSectors + sector) * NorContext.fatSectorSize, buffer, numSectors * NorContext.fatSectorSize);
/*#endif*/

#if defined(CFG_NOR_SHARE_GPIO) || defined(CFG_NOR_SHARE_SPI_NAND)
    //mmpSpiResetControl();
#endif

	if ( norResult == 0 )
	{
		// Success
		return true;
	}
	else
	{
		return false;
	}
}

static bool writeSectors(void* drvInfo, sec_t sector, sec_t numSectors, const void* buffer)
{
    uint32_t norResult = 0;

#if defined(CFG_NOR_SHARE_GPIO) || defined(CFG_NOR_SHARE_SPI_NAND)
    //mmpSpiSetControlMode(SPI_CONTROL_NOR);
#endif

/*#ifdef CFG_NOR_USE_DPUAES
    pthread_mutex_lock(&DPU_AES_MUTEX);

    norResult = NorSetAESKey(SPI_0, SPI_CSN_0, cipher_key);

    if (norResult == 0)
        norResult = NorWriteAES(SPI_0, SPI_CSN_0, (NorContext.reservrdFatSectors + sector) * NorContext.fatSectorSize, (void*)buffer, numSectors * NorContext.fatSectorSize);

    pthread_mutex_unlock(&DPU_AES_MUTEX);
#else*/
	norResult = NorWrite(SPI_0, NOR_CSN, (NorContext.reservrdFatSectors + sector) * NorContext.fatSectorSize, (void*)buffer, numSectors * NorContext.fatSectorSize);
/*#endif*/

#if defined(CFG_NOR_SHARE_GPIO) || defined(CFG_NOR_SHARE_SPI_NAND)
    //mmpSpiResetControl();
#endif

	if ( norResult == 0 )
	{
		// Success
		return true;
	}
	else
	{
		return false;
	}
}

static bool writeBytes(unsigned long addr, unsigned long numBytes, const void* buffer)
{
    uint32_t norResult = 0;

#if defined(CFG_NOR_SHARE_GPIO) || defined(CFG_NOR_SHARE_SPI_NAND)
    //mmpSpiSetControlMode(SPI_CONTROL_NOR);
#endif
#if ALIGN_RW_BYTES
    if ((addr & 0xFF || numBytes & 0xFF))
    {
        // misalign, need read to temp buf
        uint8_t *tempBuf = (uint8_t *)itpVmemAlignedAlloc(32, 1024);
        unsigned long alignedAddr = addr & ~0x3FF;
        unsigned long offset = addr & 0x3FF;
        unsigned long remaining = numBytes;
        const uint8_t *src = (const uint8_t *)buffer;
        while (remaining > 0) 
        {
            unsigned long bytesToWrite = 1024 - offset;
            if (bytesToWrite > remaining) {
                bytesToWrite = remaining;
            }

            if (offset || bytesToWrite < 1024) {
                NorRead(SPI_0, NOR_CSN, NorContext.reservrdFatSectors * NorContext.fatSectorSize + alignedAddr, tempBuf, 1024);
                memcpy(tempBuf + offset, src, bytesToWrite);
            } else {
                memcpy(tempBuf, src, 1024);
            }

            norResult = NorWriteWithoutErase(SPI_0, NOR_CSN, NorContext.reservrdFatSectors * NorContext.fatSectorSize + alignedAddr, tempBuf, 1024);
            if (norResult) {
                break;
            }

            remaining -= bytesToWrite;
            src += bytesToWrite;
            alignedAddr += 1024;
            offset = 0;
        }
        itpVmemFree((uint32_t)tempBuf);
    }
    else
	    norResult = NorWriteWithoutErase(SPI_0, NOR_CSN, NorContext.reservrdFatSectors * NorContext.fatSectorSize + addr, (void*)buffer, numBytes);
#else
    norResult = NorWriteWithoutEraseMisalign(SPI_0, NOR_CSN, NorContext.reservrdFatSectors * NorContext.fatSectorSize + addr, (void*)buffer, numBytes);
#endif
#if defined(CFG_NOR_SHARE_GPIO) || defined(CFG_NOR_SHARE_SPI_NAND)
    //mmpSpiResetControl();
#endif

	if ( norResult == 0 )
	{
		// Success
		return true;
	}
	else
	{
		return false;
	}
}

static bool readBytes(unsigned long addr, unsigned long numBytes, const void* buffer)
{
    uint32_t norResult = 0;

	norResult = NorRead(SPI_0, NOR_CSN, NorContext.reservrdFatSectors * NorContext.fatSectorSize + addr, (void*)buffer, numBytes);

	if ( norResult == 0 )
	{
		// Success
		return true;
	}
	else
	{
		return false;
	}
}

static bool eraseBlock(unsigned long block)
{
    return NorErase(SPI_0, NOR_CSN, NorContext.reservrdFatSectors * NorContext.fatSectorSize + block * NorContext.blockSizeInBytes, 
                    NorContext.blockSizeInBytes);
}
static const DISC_INTERFACE discInterface =
{
    readSectors,
    writeSectors
};

F_DRIVER *nor_initfunc(unsigned long driver_panor)
{
    NORDrvParam* drvParam = (NORDrvParam*)driver_panor;
    pthread_mutexattr_t attr;

    unsigned long reservrdBlocks;

    if (pthread_mutex_init(&NorContext.pageAccessMutex, NULL) != 0)
    {
        NorContext.pageAccessMutex = (pthread_mutex_t)NULL;
        return NULL;
    }

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&NorContext.flushAccessMutex, &attr) != 0)
    {
        NorContext.flushAccessMutex = (pthread_mutex_t)NULL;
        return NULL;
    }

	NorContext.pageSize             = NorGetAttitude(SPI_0, SPI_CSN_0, NOR_ATTITUDE_PAGE_SIZE);
	NorContext.pagesPerSector       = NorGetAttitude(SPI_0, SPI_CSN_0, NOR_ATTITUDE_PAGE_PER_SECTOR);
	NorContext.sectorsPerBlock      = NorGetAttitude(SPI_0, SPI_CSN_0, NOR_ATTITUDE_SECTOR_PER_BLOCK);
	NorContext.blockSize            = NorGetAttitude(SPI_0, SPI_CSN_0, NOR_ATTITUDE_BLOCK_SIZE);
	NorContext.fatSectorSize        = NorContext.pageSize > 512 ? NorContext.pageSize : 512;
	NorContext.blockSizeInBytes     = NorContext.pageSize * NorContext.pagesPerSector * NorContext.sectorsPerBlock;
	NorContext.fatSectorsPerBlock   = NorContext.blockSizeInBytes / NorContext.fatSectorSize;
	    
    reservrdBlocks = drvParam->reserved / NorContext.blockSizeInBytes;

    if (drvParam->reserved % NorContext.blockSizeInBytes > 0)
        printf("nor reserved size is not align on block.\n");

    NorContext.reservrdFatSectors = reservrdBlocks * NorContext.fatSectorsPerBlock;

    if (!drvParam->deviceMode)
    {
        // init norCache system
        if (drvParam->cacheSize > 0)
        {
            unsigned int cacheBlockCount = drvParam->cacheSize / NorContext.blockSizeInBytes;
            unsigned int endOfFatSector = NorContext.blockSize * NorContext.fatSectorsPerBlock - NorContext.reservrdFatSectors - 1;

            // Create the norCache
            norCache = _NOR_cache_constructor(cacheBlockCount, NorContext.fatSectorsPerBlock, &discInterface, endOfFatSector, NorContext.fatSectorSize);
        }
    }

	(void)memset (&nor_drv,0,sizeof(F_DRIVER));

	nor_drv.readsector=nor_readsector;
	nor_drv.writesector=nor_writesector;
	nor_drv.readmultiplesector=nor_readmultiplesector;
	nor_drv.writemultiplesector=nor_writemultiplesector;
	nor_drv.getphy=nor_getphy;
	nor_drv.release=nor_release;
#if defined (CFG_BUILD_FILEX)   // FAT will call flush frequently, only enable on FileX 
    nor_drv.ioctl=nor_ioctl;
    nor_drv.writebytes=nor_writebytes;
    nor_drv.readbytes=nor_readbytes;
    nor_drv.eraseblock=nor_eraseblock;
#endif    
	nor_drv.user_ptr=drvParam;

	/* Enable norCache flush */
	pthread_mutex_lock(&NorContext.flushAccessMutex);
	CacheValidFlush = true;
	pthread_mutex_unlock(&NorContext.flushAccessMutex);
	
	return &nor_drv;
}

/******************************************************************************
 *
 *  End of nordrv.c
 *
 *****************************************************************************/

