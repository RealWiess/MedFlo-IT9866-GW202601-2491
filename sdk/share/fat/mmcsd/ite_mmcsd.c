#include <stdio.h>
#include <string.h>
#include "mmcsd.h"
#include "ite/ite_sd.h"
#include "mmcsd_cache2.h"
#include <pthread.h>


#define BLOCK_SIZE      512
#define SD_MAX_WRITE_BLOCKS (1024)

/** same with itp_fat.c */
struct sd_param
{
    int sd;                 // sd0 or sd1
    int removable;          // removable or not
    unsigned long reserved; // reserved size
    unsigned long cacheSize;          // cache size
    unsigned long cachePageSize;      // cache page size
};

typedef struct _MMCSD_CONTEXT
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
}MMCSD_CONTEXT;

MMCSD_CONTEXT	mmcsdContext;
static int g_initok[2] = {0,0};
static F_DRIVER g_drivers[2];
static MMCSD_CACHE*	sdCache[2];
bool		mmcsdCacheValidFlush = false; // Cache flush sync flag

static int mmcsd_readmultiplesector(F_DRIVER *driver,void *data,unsigned long sector,int cnt);
static int mmcsd_writemultiplesector(F_DRIVER *driver,void *data,unsigned long sector,int cnt); 
static bool readSectors(void* drvInfo, sec_t sector, sec_t numSectors, void* buffer);
static bool writeSectors(void* drvInfo, sec_t sector, sec_t numSectors, const void* buffer);

void iteFatSdFlush(void)
{	
    int i;
    for(i = 0 ; i < 2 ; i ++)
    {
        if (sdCache[i])
        {
            _MMCSD_cache_flush(sdCache[i]);
        }
    }
}

/****************************************************************************
 *
 * mmcsd_readsector
 *
 * read one sector from the card
 *
 * INPUTS
 *
 * driver - driver structure
 * data - pointer where to store data
 * sector - which sector is needed
 *
 * RETURNS
 *
 * 0 - if successful
 * other if any error
 *
 ***************************************************************************/

static int mmcsd_readsector(F_DRIVER *driver,void *data,unsigned long sector) 
{
    struct sd_param* sdParam = (struct sd_param*)driver->user_data;

    if (!g_initok[sdParam->sd]) 
        return F_ERR_INVALIDDRIVE; /*card is not initialized*/

    return mmcsd_readmultiplesector(driver, data, sector, 1);
}
/****************************************************************************
 *
 * mmcsd_writesector
 *
 * write one sector into the card
 *
 * INPUTS
 *
 * driver - driver structure
 * data - pointer where original data is
 * sector - which sector needs to be written
 *
 * RETURNS
 *
 * 0 - if successful
 * other if any error
 *
 ***************************************************************************/

static int mmcsd_writesector(F_DRIVER *driver,void *data,unsigned long sector) 
{
    struct sd_param* sdParam = (struct sd_param*)driver->user_data;

    if (!g_initok[sdParam->sd]) 
        return F_ERR_INVALIDDRIVE; /*card is not initialized*/

    return mmcsd_writemultiplesector(driver, data, sector, 1);
}

/****************************************************************************
 *
 * mmcsd_readsector
 *
 * read one sector from the card
 *
 * INPUTS
 *
 * driver - driver structure
 * data - pointer where to store data
 * sector - which sector is needed
 * cnt - number of sectors to read 
 *
 * RETURNS
 *
 * 0 - if successful
 * other if any error
 *
 ***************************************************************************/

static int mmcsd_readmultiplesector(F_DRIVER *driver,void *data,unsigned long sector,int cnt) 
{
    struct sd_param* sdParam = (struct sd_param*)driver->user_data;

    if (!g_initok[sdParam->sd]) 
        return F_ERR_INVALIDDRIVE; /*card is not initialized*/

    if (!driver) return readSectors(sdParam, sector, cnt, data) ? 0 : 1;

    //if (!drvParam->deviceMode)
    {
        if (sdCache[sdParam->sd])
        {
            return _MMCSD_cache_readSectors(sdCache[sdParam->sd], sector, cnt, data) ? 0 : 1;
        }
        else
        {
            return readSectors(sdParam, sector, cnt, data) ? 0 : 1;
        }
    }
    /*else
    {
        return readSectors(NULL, sector, cnt, data) ? 0 : 1;
    }*/
}

/****************************************************************************
 *
 * mmcsd_writesector
 *
 * write one sector into the card
 *
 * INPUTS
 *
 * driver - driver structure
 * data - pointer where original data is
 * sector - which sector needs to be written
 * cnt - number of sectors to write
 *
 * RETURNS
 *
 * 0 - if successful
 * other if any error
 *
 ***************************************************************************/

static int mmcsd_writemultiplesector(F_DRIVER *driver,void *data,unsigned long sector,int cnt) 
{
    struct sd_param* sdParam = (struct sd_param*)driver->user_data;
    if (!g_initok[sdParam->sd]) 
        return F_ERR_INVALIDDRIVE; /*card is not initialized*/
    //if (!drvParam->deviceMode)
    {
        if (sdCache[sdParam->sd])
        {
            int ret = _MMCSD_cache_writeSectors(sdCache[sdParam->sd], sector, cnt, data) ? 0 : 1;
            ret = writeSectors(sdParam, sector, cnt, data) ? 0 : 1;
            return ret;
        }
        else
        {
            return writeSectors(sdParam, sector, cnt, data) ? 0 : 1;
        }
    }
    /*else
    {
        return writeSectors(NULL, drvParam->partitionOffset + sector, cnt, data) ? 0 : 1;
    }*/
}

/****************************************************************************
 *
 * mmcsd_getstatus
 *
 * get status of card, missing or/and removed,changed,writeprotect
 *
 * INPUTS
 *
 * driver - driver structure
 *
 * RETURNS
 *
 * F_ST_xxx code for high level
 *
 ***************************************************************************/

static long mmcsd_getstatus(F_DRIVER *driver) 
{
    struct sd_param* sdParam = (struct sd_param*)driver->user_data;
    int state=0;

    if (iteSdGetCardStateEx(sdParam->sd, SD_INSERTED))
    {
        if (g_initok[sdParam->sd]==0 && iteSdGetCardStateEx(sdParam->sd, SD_INIT_OK)) 
        {
            state |= F_ST_CHANGED;
            if (iteSdIsLockEx(sdParam->sd)) 
                state |= F_ST_WRPROTECT;
            g_initok[sdParam->sd] = 1;
        }
    }
    else
    {
        g_initok[sdParam->sd] = 0;
        state |= F_ST_MISSING;
    }

    return state; 
}

/****************************************************************************
 *
 * mmcsd_getphy
 *
 * determinate flash card physicals
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

static int mmcsd_getphy(F_DRIVER *driver, F_PHY *phy) 
{
    struct sd_param* sdParam = (struct sd_param*)driver->user_data;
    int res = 0;
    uint32_t sectors=0, blockSize=0;
    
    if (g_initok[sdParam->sd])
    {
        res = iteSdGetCapacityEx(sdParam->sd, &sectors, &blockSize);
        if(res)
            return res;
        phy->number_of_cylinders = 0;
        phy->sector_per_track = 63;
        phy->number_of_heads = 255;
        phy->number_of_sectors = sectors - sdParam->reserved/BLOCK_SIZE;
		phy->bytes_per_sector = blockSize;
        phy->sectors_per_cluster = 128; // TBD for FileX reserved size, need larger than SD phy block size.
        if(sdParam->removable)
            phy->media_descriptor = 0xf0; /* removable */
        else
            phy->media_descriptor = 0xf8; /* fixdrive */
        phy->reserved_sectors = sdParam->reserved/BLOCK_SIZE;
    }
    
    return 0;
}

static void mmcsd_release(F_DRIVER *driver)
{
    struct sd_param* sdParam = (struct sd_param*)driver->user_data;

    if (sdCache[sdParam->sd])
    {
        _MMCSD_cache_destructor(sdCache[sdParam->sd]);
        sdCache[sdParam->sd] = NULL;
    }
    if (!iteSdGetCardStateEx(sdParam->sd, SD_INSERTED))
        iteSdTerminateEx(sdParam->sd);
    /* Disable norCache flush */
    pthread_mutex_lock(&mmcsdContext.flushAccessMutex);
    mmcsdCacheValidFlush = false;
    pthread_mutex_unlock(&mmcsdContext.flushAccessMutex);

    (void)memset (driver,0,sizeof(F_DRIVER));
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

static int mmcsd_ioctl (F_DRIVER *driver, unsigned long msg, void *iparam, void *oparam)
{
	switch (msg)
	{
    case F_IOCTL_MSG_FLUSH:
        iteFatSdFlush();
        break;
    default :
        printf("mmcsd_ioctl unknown msg %X\n", msg);
        break;
    }
    return 0;
}

static bool readSectors(void* drvInfo, sec_t sector, sec_t numSectors, void* buffer)
{
    int ret = 0;
    struct sd_param* sdParam = (struct sd_param*)drvInfo;
    ret = iteSdReadMultipleSectorEx(sdParam->sd, (sector+sdParam->reserved/BLOCK_SIZE), numSectors, buffer);
	if ( ret == 0 )
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
    uint32_t ret = 0;
    struct sd_param* sdParam = (struct sd_param*)drvInfo;
    int i;
    sec_t writeSectorCnt = 0;
    char* writePtr = (char *)buffer;
    while(writeSectorCnt < numSectors && ret == 0)
    {
        sec_t writeSectors = (numSectors - writeSectorCnt > SD_MAX_WRITE_BLOCKS ) ? SD_MAX_WRITE_BLOCKS : (numSectors - writeSectorCnt);
        ret = iteSdWriteMultipleSectorEx(sdParam->sd, (sector+sdParam->reserved/BLOCK_SIZE) + writeSectorCnt, writeSectors, (void*)writePtr);
        writeSectorCnt += writeSectors;
        writePtr += writeSectors * BLOCK_SIZE;
    }

	if ( ret == 0 )
	{
		// Success
		return true;
	}
	else
	{
		return false;
	}
}

static const DISC_INTERFACE discInterface =
{
    readSectors,
    writeSectors
};

/****************************************************************************
 *
 * mmcsd_initfunc
 *
 * this init function has to be passed for highlevel to initiate the
 * driver functions
 *
 * INPUTS
 *
 * driver_param - special code for driver initialization
 *
 * RETURNS
 *
 * driver pointer or zero if error
 *
 ***************************************************************************/

F_DRIVER * mmcsd_initfunc(unsigned long driver_param)
{
    struct sd_param* sdParam = (struct sd_param*)driver_param;
    F_DRIVER *driver;
    pthread_mutexattr_t attr;

    g_initok[sdParam->sd] = 0;
    if (pthread_mutex_init(&mmcsdContext.pageAccessMutex, NULL) != 0)
    {
        mmcsdContext.pageAccessMutex = (pthread_mutex_t)NULL;
        return NULL;
    }

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&mmcsdContext.flushAccessMutex, &attr) != 0)
    {
        mmcsdContext.flushAccessMutex = (pthread_mutex_t)NULL;
        return NULL;
    }

    /* init driver functions */
    driver=&g_drivers[sdParam->sd];

    (void)memset (driver,0,sizeof(F_DRIVER));
    {
        // init norCache system
        if (sdParam->cacheSize > 0)
        {
            uint32_t sectors=0, blockSize=0;
            iteSdGetCapacityEx(sdParam->sd, &sectors, &blockSize);
            
            blockSize = sdParam->cachePageSize;
            unsigned int cacheBlockCount = sdParam->cacheSize / blockSize;
            unsigned int endOfFatSector = sectors - sdParam->reserved/BLOCK_SIZE - 1;
            
            // Create the norCache
            
            sdCache[sdParam->sd] = _MMCSD_cache_constructor(cacheBlockCount, blockSize/BLOCK_SIZE, &discInterface, endOfFatSector, BLOCK_SIZE, (void*)sdParam);
        }
    }

    driver->readsector=mmcsd_readsector;
    driver->writesector=mmcsd_writesector;
    driver->readmultiplesector=mmcsd_readmultiplesector;
    driver->writemultiplesector=mmcsd_writemultiplesector;
    driver->getstatus=mmcsd_getstatus;
    driver->getphy=mmcsd_getphy;
    driver->release=mmcsd_release;
    driver->ioctl=mmcsd_ioctl;
    driver->user_data=driver_param;
    mmcsd_getstatus(driver);
    /* Enable sdCache flush */
	pthread_mutex_lock(&mmcsdContext.flushAccessMutex);
	mmcsdCacheValidFlush = true;
	pthread_mutex_unlock(&mmcsdContext.flushAccessMutex);

    return driver;
}

