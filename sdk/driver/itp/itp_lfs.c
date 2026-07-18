#include <errno.h>
#include <sys/ioctl.h>
#include <sys/reent.h>
#include <sys/syslimits.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "nor/mmp_nor.h"
#ifdef CFG_NAND_ENABLE
#include "ite/ite_nand.h"
#endif
#include "itp_cfg.h"
#include "littlefs/lfs.h"
#include "littlefs/lfs_util.h"
#include "fat/api/api_fat.h"
#if defined(CFG_SD0_PARTITION0) || defined(CFG_SD1_PARTITION0)
#include "ite/ite_sd.h"
#endif
#if (CFG_LFS_CACHE_SIZE > 0)
#include "littlefs/lfs_cache.h"
#endif

static char *itp_lfs_realpath(const char *path, char* _resolved);
#define ITP_LFS_TAG         "littlefssfelttil"
#define OFFSETOF(container_type, member) ((size_t) &((container_type *)0)->member)
#define CONTAINEROF(member_ptr, container_type, member)		\
	 ((container_type *)						                \
	  ((char *)(member_ptr)						                \
	   - offsetof(container_type, member))

#define ITP_LFS_PREPAREBYFD() \
    lfs_file_t              *file   =itp_files[fd];                 \
    int                     drive   =itp_drives[fd];                \
    struct lfs_config       *config =itp_lfs_get_config('A'+drive); \
    struct itp_lfs_context  *context=itp_lfs_get_context('A'+drive);\
    lfs_t                   *lfs    =&context->lfs

#define ITP_LFS_PREPAREBYPATH(path) \
    struct lfs_config       *config =itp_lfs_get_config(*path);     \
    struct itp_lfs_context  *context=itp_lfs_get_context(*path);    \
    lfs_t                   *lfs    =&context->lfs;                 \
    int                     drive   =toupper(*path)-'A'

struct itp_lfs_partition {
    uint32_t    block_start;
    uint32_t    blocks;
    uint32_t    reserved[2];
};

struct itp_lfs_partition_table {
    char                        tag[16];
    struct itp_lfs_partition    partitions[ITP_MAX_PARTITION];
};

struct itp_lfs_context {
    lfs_t               lfs;
    struct lfs_config   config;
    uint32_t            block_start;
    uint32_t            partition;
    pthread_mutex_t     lock;
    ITPDriveStatus*     drive_status;
    char                cwd[PATH_MAX];
    LFS_CACHE*          lfs_cache;
};

static struct itp_lfs_context* drive_to_context['Z'-'A'+1]={0};
static LFS_CACHE* lfs_cache[ITH_DISK_MAX]={0};

#if defined(CFG_NAND_ENABLE)
extern ITE_NF_INFO itpGetNfInfo();
static ITE_NF_INFO itp_lfs_nf_info;
//static unsigned char* itp_lfs_nf_page_buffer=0;
static unsigned char* itp_lfs_nf_block_buffer=0;
#if (CFG_LFS_HAVE_HCCFTL)
extern F_DRIVER *ftl_initfunc(unsigned long driver_param);
F_DRIVER* f_driver=NULL;
F_PHY f_phy;
#endif
static int bad_block_table[4096]={0};
#endif
static int itp_lfs_flash_page_size[ITH_DISK_MAX];
static int itp_lfs_flash_spare_size[ITH_DISK_MAX];
static int itp_lfs_flash_block_size[ITH_DISK_MAX];
static int itp_lfs_flash_blocks[ITH_DISK_MAX];
static int itp_lfs_flash_block_cycles[ITH_DISK_MAX];
/*#ifdef CFG_NOR_USE_DPUAES
static uint8_t  cipher_key[16] = {
    0xff, 0x00, 0x11, 0x22,
    0x33, 0x44, 0x55, 0x66,
    0x77, 0x88, 0x99, 0xaa,
    0xbb, 0xcc, 0xdd, 0xee
};
#endif*/

extern int itpTaskGetVolume(void);

int itp_lfs_init()
{
#if defined(CFG_NAND_ENABLE)
    int i=0, n=0;
    int block=0;
    int page=0;
    int page2=0;

    itp_lfs_nf_info=itpGetNfInfo();

    itp_lfs_flash_page_size[ITP_DISK_NAND]=itp_lfs_nf_info.PageSize;
    itp_lfs_flash_spare_size[ITP_DISK_NAND]=itp_lfs_nf_info.SprSize;
    itp_lfs_flash_block_size[ITP_DISK_NAND]=itp_lfs_nf_info.PageInBlk*itp_lfs_nf_info.PageSize;
    itp_lfs_flash_blocks[ITP_DISK_NAND]=itp_lfs_nf_info.NumOfBlk;
    itp_lfs_flash_block_cycles[ITP_DISK_NAND]=10000;
    /*if (!itp_lfs_nf_page_buffer) {
        itp_lfs_nf_page_buffer=(unsigned char*)malloc(itp_lfs_flash_page_size+itp_lfs_flash_spare_size + 256);
    }*/
    if (!itp_lfs_nf_block_buffer) {
        itp_lfs_nf_block_buffer=(unsigned char*)calloc((itp_lfs_flash_page_size[ITP_DISK_NAND]+itp_lfs_flash_spare_size[ITP_DISK_NAND])*itp_lfs_nf_info.PageInBlk, 1);
    }
#if (CFG_LFS_HAVE_HCCFTL)
    itp_lfs_flash_block_cycles[ITP_DISK_NAND]=-1;
    if (f_driver==NULL) {
        f_driver=ftl_initfunc(CFG_NAND_RESERVED_SIZE);
        if (!f_driver) {
            printf("[X] ftl_initfunc(%08X) failed!\n", CFG_NAND_RESERVED_SIZE);
            /*if (itp_lfs_nf_page_buffer) {
                free(itp_lfs_nf_page_buffer);
                itp_lfs_nf_page_buffer=0;
            }*/
            if (itp_lfs_nf_block_buffer) {
                free(itp_lfs_nf_block_buffer);
                itp_lfs_nf_block_buffer=0;
            }
            return -1;
        }

        f_driver->getphy(f_driver, &f_phy);
        printf("[i] getphy: ns=%d, bps=%d, diff=%d\n", f_phy.number_of_sectors, f_phy.bytes_per_sector, itp_lfs_flash_block_size[ITP_DISK_NAND]*itp_lfs_flash_blocks-f_phy.number_of_sectors*f_phy.bytes_per_sector);
    }
#endif
    block=CFG_LFS_NAND_PTADDRESS/itp_lfs_flash_block_size[ITP_DISK_NAND];
    page=(CFG_LFS_NAND_PTADDRESS%itp_lfs_flash_block_size[ITP_DISK_NAND])/itp_lfs_flash_page_size[ITP_DISK_NAND];
    page2=page-sizeof(bad_block_table)/itp_lfs_flash_page_size[ITP_DISK_NAND];

    if (iteNfIsBadBlockForBoot(block)<0)
    {
        printf("[itp_lfs_init error] %d is bad block(%d)\n", block, itp_lfs_flash_block_size[ITP_DISK_NAND]);
        return -1;
    }

    for (i=0; i<itp_lfs_nf_info.PageInBlk; i++) {
        n=iteNfRead(
            block,
            i,
            itp_lfs_nf_block_buffer+i*(itp_lfs_flash_page_size[ITP_DISK_NAND]+itp_lfs_flash_spare_size[ITP_DISK_NAND]));
        if (n!=true) {
            printf("[X] %s():iteNfRead(%d, %d)\n", __FUNCTION__, block, i);
            return -1;
        }
    }

    i = (int)bad_block_table;
    while (page2<page)
    {
        memcpy((uint8_t *)i,
                itp_lfs_nf_block_buffer+page2*(itp_lfs_flash_page_size[ITP_DISK_NAND]+itp_lfs_flash_spare_size[ITP_DISK_NAND]),
                itp_lfs_flash_page_size[ITP_DISK_NAND]);
        i+=itp_lfs_flash_page_size[ITP_DISK_NAND];
        page2++;
    }

    /*for (i=0; i<sizeof(bad_block_table)/sizeof(bad_block_table[0]); i++)
        if (bad_block_table[i])
            printf("[itp_lfs_init] %d is initial bad block(%d)\n", i, itp_lfs_flash_block_size);*/
#elif (CFG_LFS_CACHE_SIZE > 0)
#if defined(CFG_SD0_PARTITION0)
    uint32_t sectorCnt = 0;
    uint32_t blockSize = 0;

    itp_lfs_flash_page_size[ITP_DISK_SD0]=512;
    itp_lfs_flash_spare_size[ITP_DISK_SD0]=0;
    iteSdGetCapacityEx(SD_0, &sectorCnt, &blockSize);
    itp_lfs_flash_block_size[ITP_DISK_SD0]=65536;
    itp_lfs_flash_blocks[ITP_DISK_SD0]=(int)sectorCnt * 512 / 65536;
    itp_lfs_flash_block_cycles[ITP_DISK_SD0]=-1;
    if (lfs_cache_constructor(CFG_LFS_CACHE_SIZE, itp_lfs_flash_page_size[ITP_DISK_SD0], itp_lfs_flash_block_size[ITP_DISK_SD0], itp_lfs_flash_spare_size[ITP_DISK_SD0], &lfs_cache[ITP_DISK_SD0]) == 0)
        printf("lfs_cache_constructor success! %p\n", lfs_cache[ITP_DISK_SD0]);
    else
        printf("lfs_cache_constructor error!\n");
#endif
#if defined(CFG_SD1_PARTITION0)
    uint32_t sectorCnt = 0;
    uint32_t blockSize = 0;

    itp_lfs_flash_page_size[ITP_DISK_SD1]=512;
    itp_lfs_flash_spare_size[ITP_DISK_SD1]=0;
    iteSdGetCapacityEx(SD_1, &sectorCnt, &blockSize);
    itp_lfs_flash_block_size[ITP_DISK_SD1]=65536;
    itp_lfs_flash_blocks[ITP_DISK_SD1]=(int)sectorCnt * 512 / 65536;
    itp_lfs_flash_block_cycles[ITP_DISK_SD1]=-1;
    if (lfs_cache_constructor(CFG_LFS_CACHE_SIZE, itp_lfs_flash_page_size[ITP_DISK_SD1], itp_lfs_flash_block_size[ITP_DISK_SD1], itp_lfs_flash_spare_size[ITP_DISK_SD1], &lfs_cache[ITP_DISK_SD1]) == 0)
        printf("lfs_cache_constructor success!\n");
    else
        printf("lfs_cache_constructor error!\n");
#endif
#if defined(CFG_NOR_ENABLE)
    itp_lfs_flash_page_size[ITP_DISK_NOR]=NorGetAttitude(SPI_0, SPI_CSN_0, NOR_ATTITUDE_PAGE_SIZE);
    itp_lfs_flash_spare_size[ITP_DISK_NOR]=0;
    itp_lfs_flash_block_size[ITP_DISK_NOR]=NorGetAttitude(SPI_0, SPI_CSN_0, NOR_ATTITUDE_ERASE_UNIT);
    itp_lfs_flash_blocks[ITP_DISK_NOR]=NorGetAttitude(SPI_0, SPI_CSN_0, NOR_ATTITUDE_BLOCK_SIZE)*NorGetAttitude(SPI_0, SPI_CSN_0, NOR_ATTITUDE_SECTOR_PER_BLOCK);
    itp_lfs_flash_block_cycles[ITP_DISK_NOR]=-1;
    if (lfs_cache_constructor(CFG_LFS_CACHE_SIZE, itp_lfs_flash_page_size[ITP_DISK_NOR], itp_lfs_flash_block_size[ITP_DISK_NOR], itp_lfs_flash_spare_size[ITP_DISK_NOR], &lfs_cache[ITP_DISK_NOR]) == 0)
        printf("lfs_cache_constructor success! %p\n", lfs_cache[ITP_DISK_NOR]);
    else
        printf("lfs_cache_constructor error!\n");
#endif
#endif

    memset(drive_to_context, 0, sizeof(drive_to_context));

    return 0;
}

int itp_lfs_exit()
{
#if (CFG_LFS_CACHE_SIZE > 0)
    int i;
    for(i = 0 ; i < ITH_DISK_MAX ; i ++)
    {
        if(lfs_cache[i])
        {
            if (lfs_cache_destructor(&lfs_cache[i]) == 0)
                printf("lfs_cache_destructor success!\n");
            else
                printf("lfs_cache_destructor error!\n");
        }
    }
#endif

    return 0;
}

#define HEXDUMP_COLS 16
void hexdump(char* msg, void *mem, unsigned int len)
{
        unsigned int i, j;
//return;
    printf(msg);
        for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++)
        {
                /* print offset */
                if(i % HEXDUMP_COLS == 0)
                {
                        printf("0x%06x: ", i);
                }

                /* print hex data */
                if(i < len)
                {
                        printf("%02x ", 0xFF & ((char*)mem)[i]);
                }
                else /* end of block, just aligning for ASCII dump */
                {
                        printf("   ");
                }

                /* print ASCII dump */
                if(i % HEXDUMP_COLS == (HEXDUMP_COLS - 1))
                {
                        for(j = i - (HEXDUMP_COLS - 1); j <= i; j++)
                        {
                                if(j >= len) /* end of block, not really printing */
                                {
                                        putchar(' ');
                                }
                                else if(isalnum(((char*)mem)[j])) /* printable char */
                                {
                                        putchar(0xFF & ((char*)mem)[j]);
                                }
                                else /* other char */
                                {
                                        putchar('.');
                                }
                        }
                        putchar('\n');
                }
        }
    printf("\n");
}


// Read a region in a block. Negative error codes are propogated
// to the user.
int itp_lfs_flash_read(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size)
{
    int n;
    struct itp_lfs_context *context=(struct itp_lfs_context*)c->context;
#if defined(CFG_NAND_ENABLE)
    unsigned char *itp_lfs_nf_page_buffer_inner=NULL;
#endif

#if defined(CFG_NAND_ENABLE)
    int page=off/itp_lfs_flash_page_size[ITP_DISK_NAND];
//printf("[i] itp_lfs_flash_read(%d:%d, %d(%x), %d)\n", block, context->block_start+block, off, off, size);
    if ((off%itp_lfs_flash_page_size[ITP_DISK_NAND]) || (size%itp_lfs_flash_page_size[ITP_DISK_NAND]) || ((off+size)>itp_lfs_flash_block_size[ITP_DISK_NAND])) {
        printf("[X] itp_lfs_flash_read: off(%X) or size(%X) is not multiples of %X\n", off, size, itp_lfs_flash_page_size);
        return -1;
    }
#if (CFG_LFS_HAVE_HCCFTL)
    int sector=((context->block_start+block)*itp_lfs_flash_block_size+off)/f_phy.bytes_per_sector;
    n=f_driver->readmultiplesector(f_driver, buffer, sector, size/f_phy.bytes_per_sector);
    if (n) {
        printf("[X] %s(%d, %d, %d)=>readmultiplesector(%d, %d)\n", __FUNCTION__, block, off, size, sector, size/f_phy.bytes_per_sector);
        return -1;
    }
#else
    itp_lfs_nf_page_buffer_inner=(unsigned char *)malloc(itp_lfs_flash_page_size[ITP_DISK_NAND]+itp_lfs_flash_spare_size[ITP_DISK_NAND]+256);
    if (!itp_lfs_nf_page_buffer_inner)
    {
        printf("[X] %s(itp_lfs_nf_page_buffer_inner malloc error)\n", __FUNCTION__);
        return -1;
    }

    if (bad_block_table[context->block_start+block])
    {
        printf("[itp_lfs_flash_read error] %d is initial bad block(%d)\n", context->block_start+block, itp_lfs_flash_block_size[ITP_DISK_NAND]);
        free(itp_lfs_nf_page_buffer_inner);
        return -1;
    }

    while (size > 0) {
        n=iteNfRead(context->block_start+block, page, itp_lfs_nf_page_buffer_inner);
        if (n!=true) {
            printf("[X] %s(%d, %d, %d)=>iteNfRead(%d, %d)\n", __FUNCTION__, block, off, size, context->block_start+block, page);
            //free(itp_lfs_nf_page_buffer_inner);
            //return -1;
        }
        memcpy(buffer, itp_lfs_nf_page_buffer_inner, itp_lfs_flash_page_size[ITP_DISK_NAND]);
        page++;
        size-=itp_lfs_flash_page_size[ITP_DISK_NAND];
        buffer+=itp_lfs_flash_page_size[ITP_DISK_NAND];
    }
    free(itp_lfs_nf_page_buffer_inner);
#endif
#else
#if defined(CFG_SD0_PARTITION0)
    int sector=((context->block_start+block)*c->block_size+off)/itp_lfs_flash_page_size[ITP_DISK_SD0];
    if (iteSdReadMultipleSectorEx(SD_0, sector, size/itp_lfs_flash_page_size[ITP_DISK_SD0], buffer))
    {
        printf("[X] %s(%d, %d, %d)=>iteSdReadMultipleSectorEx(%d, %d)\n", __FUNCTION__, block, off, size, sector, size/c->block_size);
        return -1;
    }
#elif defined(CFG_SD1_PARTITION0)
    int sector=((context->block_start+block)*c->block_size+off)/itp_lfs_flash_page_size[ITP_DISK_SD1];
    if (iteSdReadMultipleSectorEx(SD_1, sector, size/itp_lfs_flash_page_size[ITP_DISK_SD1], buffer))
    {
        printf("[X] %s(%d, %d, %d)=>iteSdReadMultipleSectorEx(%d, %d)\n", __FUNCTION__, block, off, size, sector, size/c->block_size);
        return -1;
    }
#elif defined(CFG_NOR_ENABLE)
/*#ifdef CFG_NOR_USE_DPUAES
    pthread_mutex_lock(&DPU_AES_MUTEX);

    n = NorSetAESKey(SPI_0, SPI_CSN_0, cipher_key);

    if (n == 0)
        n=NorReadAES(SPI_0, SPI_CSN_0, (context->block_start+block)*c->block_size+off, (uint8_t*)buffer, size);

    pthread_mutex_unlock(&DPU_AES_MUTEX);

    if (n) {
        printf("[X] %s(%d, %d, %d)=>NorRead(%d, %d)\n", __FUNCTION__, block, off, size, (context->block_start+block)*c->block_size+off, size);
        return -1;
    }
#else*/
    n=NorRead(SPI_0, NOR_CSN, (context->block_start+block)*c->block_size+off, (uint8_t*)buffer, size);
    if (n) {
        printf("[X] %s(%d, %d, %d)=>NorRead(%d, %d)\n", __FUNCTION__, block, off, size, (context->block_start+block)*c->block_size+off, size);
        return -1;
    }
/*#endif*/
#endif
#endif
    return 0;
}

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
int itp_lfs_flash_prog(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size)
{
    int n;
    struct itp_lfs_context *context=(struct itp_lfs_context*)c->context;

#if defined(CFG_NAND_ENABLE)
    int page=off/itp_lfs_flash_page_size[ITP_DISK_NAND];
//printf("[i] itp_lfs_flash_prog(%d:%d, %d(%x), %d)\n", block, context->block_start+block, off, off, size);
    if ((off%itp_lfs_flash_page_size[ITP_DISK_NAND]) || (size%itp_lfs_flash_page_size[ITP_DISK_NAND]) || ((off+size)>itp_lfs_flash_block_size[ITP_DISK_NAND])) {
        printf("[X] itp_lfs_flash_prog: off(%X) or size(%X) is not multiples of %X\n", off, size, itp_lfs_flash_page_size[ITP_DISK_NAND]);
        return -1;
    }
#if (CFG_LFS_HAVE_HCCFTL)
    int sector=((context->block_start+block)*itp_lfs_flash_block_size[ITP_DISK_NAND]+off)/f_phy.bytes_per_sector;
    n=f_driver->writemultiplesector(f_driver, (void *)buffer, sector, size/f_phy.bytes_per_sector);
    if (n) {
        printf("[X] %s(%d, %d, %d)=>writemultiplesector(%d, %d)\n", __FUNCTION__, block, off, size, sector, size/f_phy.bytes_per_sector);
        return -1;
    }
#else
    static unsigned char oob[24]={0xff};

    if (bad_block_table[context->block_start+block])
    {
        printf("[itp_lfs_flash_prog] %d is initial bad block(%d)\n", context->block_start+block, itp_lfs_flash_block_size[ITP_DISK_NAND]);
        return LFS_ERR_CORRUPT;
    }

    while (size > 0) {
        n=iteNfWrite(context->block_start+block, page, (void *)buffer, oob);
        if (n!=true) {
            printf("[X] %s(%d, %d, %d)=>iteNfWrite(%d, %d)\n", __FUNCTION__, block, off, size, context->block_start+block, page);
            //return -1;
        }
        page++;
        size-=itp_lfs_flash_page_size[ITP_DISK_NAND];
        buffer+=itp_lfs_flash_page_size[ITP_DISK_NAND];
    }
#endif
#else
#if defined(CFG_SD0_PARTITION0)
    int sector=((context->block_start+block)*c->block_size+off)/itp_lfs_flash_page_size[ITP_DISK_SD0];
    if (iteSdWriteMultipleSectorEx(SD_0, sector, size/itp_lfs_flash_page_size[ITP_DISK_SD0], (uint8_t *)buffer))
    {
        printf("[X] %s(%d, %d, %d)=>iteSdWriteMultipleSectorEx(%d, %d)\n", __FUNCTION__, block, off, size, sector, size/c->block_size);
        return -1;
    }
#elif defined(CFG_SD1_PARTITION0)
    int sector=((context->block_start+block)*c->block_size+off)/itp_lfs_flash_page_size[ITP_DISK_SD1];
    if (iteSdWriteMultipleSectorEx(SD_1, sector, size/itp_lfs_flash_page_size[ITP_DISK_SD1], (uint8_t *)buffer))
    {
        printf("[X] %s(%d, %d, %d)=>iteSdWriteMultipleSectorEx(%d, %d)\n", __FUNCTION__, block, off, size, sector, size/c->block_size);
        return -1;
    }
#elif defined(CFG_NOR_ENABLE)
/*#ifdef CFG_NOR_USE_DPUAES
    pthread_mutex_lock(&DPU_AES_MUTEX);

    n = NorSetAESKey(SPI_0, SPI_CSN_0, cipher_key);

    if (n == 0)
        n=NorWriteWithoutEraseAES(SPI_0, SPI_CSN_0, (context->block_start+block)*c->block_size+off, (uint8_t*)buffer, size);

    pthread_mutex_unlock(&DPU_AES_MUTEX);

    if (n) {
        printf("[X] %s(%d, %d, %d)=>NorWriteWithoutErase(%d, %d)\n", __FUNCTION__, block, off, size, (context->block_start+block)*c->block_size+off, size);
        return -1;
    }
#else*/
    n=NorWriteWithoutErase(SPI_0, NOR_CSN, (context->block_start+block)*c->block_size+off, (uint8_t*)buffer, size);
    if (n) {
        printf("[X] %s(%d, %d, %d)=>NorWriteWithoutErase(%d, %d)\n", __FUNCTION__, block, off, size, (context->block_start+block)*c->block_size+off, size);
        return -1;
    }
/*#endif*/
#endif
#endif
    return 0;
}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
int itp_lfs_flash_erase(const struct lfs_config *c, lfs_block_t block)
{
    int n;
    struct itp_lfs_context *context=(struct itp_lfs_context*)c->context;

#if defined(CFG_NAND_ENABLE)
#if (CFG_LFS_HAVE_HCCFTL)
    n=0;
#else
    if (bad_block_table[context->block_start+block])
    {
        printf("[itp_lfs_flash_erase] %d is initial bad block(%d)\n", context->block_start+block, itp_lfs_flash_block_size);
        return LFS_ERR_CORRUPT;
    }

    n=iteNfErase(context->block_start+block);
    if (n!=true) {
        printf("[X] %s(%d):iteNfErase(%d)\n", __FUNCTION__, block, context->block_start+block);
        //return -1;
    }
//printf("[i] itp_lfs_flash_erase(%d:%d)=%d\n", block, context->block_start+block, n);
#endif
#else
#if defined(CFG_SD0_PARTITION0)
    static unsigned char erase_data[65536] = {0};
    int sector=((context->block_start+block)*c->block_size)/itp_lfs_flash_page_size[ITP_DISK_SD0];

    n=iteSdWriteMultipleSectorEx(SD_0, sector, c->block_size/itp_lfs_flash_page_size[ITP_DISK_SD0], (uint8_t *)erase_data);
    if (n)
    {
        printf("[X] %s(%d)=>iteSdWriteMultipleSectorEx(%d)\n", __FUNCTION__, block, sector);
        return -1;
    }
#elif defined(CFG_SD1_PARTITION0)
    static unsigned char erase_data[65536] = {0};
    int sector=((context->block_start+block)*c->block_size)/itp_lfs_flash_page_size[ITP_DISK_SD1];
    n=iteSdWriteMultipleSectorEx(SD_1, sector, c->block_size/itp_lfs_flash_page_size[ITP_DISK_SD1], (uint8_t *)erase_data);
    if (n)
    {
        printf("[X] %s(%d)=>iteSdWriteMultipleSectorEx(%d)\n", __FUNCTION__, block, sector);
        return -1;
    }
#elif defined(CFG_NOR_ENABLE)
    n=NorErase(SPI_0, NOR_CSN, (context->block_start+block)*c->block_size, c->block_size);
    if (n!=true) {
        printf("[X] %s(%d):NorErase(%d)\n", __FUNCTION__, block, (context->block_start+block)*c->block_size);
        return -1;
    }
#endif
#endif

    return 0;
}

// Sync the state of the underlying block device. Negative error codes
// are propogated to the user.
int itp_lfs_flash_sync(const struct lfs_config *c)
{
    struct itp_lfs_context *context=(struct itp_lfs_context*)c->context;

#if (CFG_LFS_HAVE_HCCFTL)
    ioctl(ITP_DEVICE_NAND, ITP_IOCTL_FLUSH, (void*)ITP_NAND_FTL_MODE);
#endif
//printf("[i] itp_lfs_flash_sync()\n");
    return 0;
}

static struct itp_lfs_context* itp_lfs_get_context(int driveLetter)
{
    int drive = toupper(driveLetter)-'A';

    return drive_to_context[drive];
}

static struct lfs_config* itp_lfs_get_config(int driveLetter)
{
    struct itp_lfs_context *context=itp_lfs_get_context(driveLetter);

    return context?&context->config:NULL;
}

static int itp_lfs_lock(const struct lfs_config *c)
{
    struct itp_lfs_context* context=(struct itp_lfs_context*)c->context;
    return pthread_mutex_lock(&context->lock);
}

static int itp_lfs_unlock(const struct lfs_config *c)
{
    struct itp_lfs_context* context=(struct itp_lfs_context*)c->context;
    return pthread_mutex_unlock(&context->lock);
}

///////////////////////////////////////////////////////////////////////////////
#if (CFG_LFS_CACHE_SIZE > 0)
int itp_lfs_cache_read(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size)
{
    struct itp_lfs_context *context=(struct itp_lfs_context*)c->context;
    uint32_t pageSize = context->lfs_cache->flashInfo->pageSize;
    uint32_t blockSize = context->lfs_cache->flashInfo->blockSize;
    uint32_t page=off/pageSize;
    uint32_t cnt = size/pageSize;

    if ((off%pageSize) || (size%pageSize) || ((off+size)>blockSize)) {
        printf("[X] itp_lfs_flash_read: off(%X) or size(%X) is not multiples of %X\n", off, size, pageSize);
        return -1;
    }
#if defined(CFG_NAND_ENABLE)
    if (bad_block_table[context->block_start+block])
    {
        printf("[itp_lfs_flash_read error] %d is initial bad block(%d)\n", context->block_start+block, itp_lfs_flash_block_size[ITP_DISK_NAND]);
        return -1;
    }
#endif
    if (lfs_cache_read_page(context->block_start+block, page, (uint32_t)buffer, cnt, context->drive_status->disk, context->lfs_cache) != 0)
    {
        printf("lfs_cache_read_page error!\n");
        return -1;
    }

    return 0;
}

int itp_lfs_cache_prog(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size)
{
    struct itp_lfs_context *context=(struct itp_lfs_context*)c->context;
    uint32_t pageSize = context->lfs_cache->flashInfo->pageSize;
    uint32_t blockSize = context->lfs_cache->flashInfo->blockSize;
    uint32_t page=off/pageSize;
    uint32_t cnt = size/pageSize;

    if ((off%pageSize) || (size%pageSize) || ((off+size)>blockSize)) {
        printf("[X] itp_lfs_flash_prog: off(%X) or size(%X) is not multiples of %X\n", off, size, pageSize);
        return -1;
    }
#if defined(CFG_NAND_ENABLE)
    if (bad_block_table[context->block_start+block])
    {
        printf("[itp_lfs_flash_prog] %d is initial bad block(%d)\n", context->block_start+block, itp_lfs_flash_block_size[ITP_DISK_NAND]);
        return LFS_ERR_CORRUPT;
    }
#endif
    if (lfs_cache_write_page(context->block_start+block, page, (uint32_t)buffer, cnt, context->drive_status->disk, context->lfs_cache) != 0)
    {
        printf("lfs_cache_write_page error!\n");
        return -1;
    }

    return 0;
}

int itp_lfs_cache_erase(const struct lfs_config *c, lfs_block_t block)
{
    struct itp_lfs_context *context=(struct itp_lfs_context*)c->context;
#if defined(CFG_NAND_ENABLE)
    if (bad_block_table[context->block_start+block])
    {
        printf("[itp_lfs_flash_erase] %d is initial bad block(%d)\n", context->block_start+block, itp_lfs_flash_block_size[ITP_DISK_NAND]);
        return LFS_ERR_CORRUPT;
    }
#endif
    if (lfs_cache_erase_block(context->block_start+block, context->drive_status->disk, context->lfs_cache) != 0)
    {
        printf("lfs_cache_erase_block error!\n");
        return -1;
    }

    return 0;
}
#endif
///////////////////////////////////////////////////////////////////////////////

static int itp_lfs_get_flash_config(struct lfs_config *config, uint32_t blocks, ITPDisk disk)
{
#if (CFG_LFS_CACHE_SIZE > 0)
    config->read  = itp_lfs_cache_read;
    config->prog  = itp_lfs_cache_prog;
    config->erase = itp_lfs_cache_erase;
#else
    config->read  = itp_lfs_flash_read;
    config->prog  = itp_lfs_flash_prog;
    config->erase = itp_lfs_flash_erase;
#endif
    config->sync  = itp_lfs_flash_sync;

    // todo: init values according to the NOR flash used
    config->read_size=itp_lfs_flash_page_size[disk];
    config->prog_size=itp_lfs_flash_page_size[disk];
    config->block_size=itp_lfs_flash_block_size[disk];
    config->block_count=blocks;
    config->cache_size=itp_lfs_flash_block_size[disk];
    config->lookahead_size=itp_lfs_flash_block_size[disk]/8;
    config->block_cycles=itp_lfs_flash_block_cycles[disk];
    config->lock=itp_lfs_lock;
    config->unlock=itp_lfs_unlock;
}

static struct itp_lfs_context* itp_lfs_new_flash_context(uint32_t block_start, uint32_t blocks, ITPDriveStatus*  drive_status, ITPDisk disk)
{
    struct lfs_config *config;
    struct itp_lfs_context* context;

#if (CFG_LFS_HAVE_HCCFTL)
//printf("[i] block_start=%d, blocks=%d\n", block_start, blocks);
    block_start-=(CFG_NAND_RESERVED_SIZE/itp_lfs_flash_block_size[disk]);
    block_start++;
    if ((block_start+blocks)*itp_lfs_flash_block_size[disk]>f_phy.number_of_sectors*f_phy.bytes_per_sector) {
        blocks=f_phy.number_of_sectors*f_phy.bytes_per_sector/itp_lfs_flash_block_size[disk]-block_start;
//printf("[i] blocks=%d*%d/%d-%d=%d\n", f_phy.number_of_sectors, f_phy.bytes_per_sector, itp_lfs_flash_block_size, block_start, blocks);
    }
    if (block_start<0 || blocks<=0) {
        printf("[X] itp_lfs_new_flash_context(%d, %d), ftl: ns=%d, bps:=%d\n", block_start+CFG_NAND_RESERVED_SIZE, blocks, f_phy.number_of_sectors, f_phy.bytes_per_sector);
        return NULL;
    }
#endif

    context=(struct itp_lfs_context*)calloc(1, sizeof(struct itp_lfs_context));
    if (!context) {
        return NULL;
    }
    pthread_mutex_init(&context->lock, NULL);

    context->block_start=block_start;
    context->drive_status=drive_status;
    context->cwd[0]='/';
    context->cwd[1]='\0';
    context->config.context=(void*)context;
    context->lfs_cache = lfs_cache[disk];
    itp_lfs_get_flash_config(&context->config, blocks, disk);

    return context;
}

static int itp_lfs_free_context(struct itp_lfs_context** context)
{
    if (context && *context) {
        pthread_mutex_destroy(&(*context)->lock);
        free(*context);
        *context=NULL;
    }
    return 0;
}

static int itp_lfs_read_partition_table(ITPDisk disk, struct itp_lfs_partition_table* pt)
{
    int i;
    int n;
    unsigned char tmp[256] = {0};
#if defined(CFG_SD0_PARTITION0) || defined(CFG_SD1_PARTITION0)
    unsigned char itp_lfs_sd_sector_buffer[512];
#endif
/*#ifdef CFG_NOR_USE_DPUAES
    unsigned char *tmp2 = 0;
#endif*/
#if defined(CFG_NAND_ENABLE)
    unsigned char *itp_lfs_nf_page_buffer_inner=NULL;
#endif

    if ((disk!=ITP_DISK_NOR) && (disk!=ITP_DISK_NAND) && (disk!=ITP_DISK_SD0) && (disk!=ITP_DISK_SD1)) {
        return -1;
    }

    memset(pt, 0, sizeof(*pt));
#if defined(CFG_NAND_ENABLE)
    itp_lfs_nf_page_buffer_inner=(unsigned char *)malloc(itp_lfs_flash_page_size[disk]+itp_lfs_flash_spare_size[disk]+256);
    if (!itp_lfs_nf_page_buffer_inner)
    {
        printf("[X] %s(itp_lfs_nf_page_buffer_inner malloc error)\n", __FUNCTION__);
        return -1;
    }
#if (CFG_LFS_HAVE_HCCFTL)
    n=f_driver->readmultiplesector(f_driver, itp_lfs_nf_page_buffer_inner, 0, 1);
    if (n) {
        ITP_LOG_ERR(
            "error reading partition table in FTL sector 0!\n"
        );
        free(itp_lfs_nf_page_buffer_inner);
        return -1;
    }
    memcpy(pt, itp_lfs_nf_page_buffer_inner, sizeof(*pt));
    free(itp_lfs_nf_page_buffer_inner);
#else
    n=iteNfRead(
        CFG_LFS_NAND_PTADDRESS/itp_lfs_flash_block_size[disk],
        (CFG_LFS_NAND_PTADDRESS%itp_lfs_flash_block_size[disk])/itp_lfs_flash_page_size[disk],
        itp_lfs_nf_page_buffer_inner);
    if (n!=true) {
        printf("[X] %s():iteNfRead(%d, %d)\n", __FUNCTION__,
        CFG_LFS_NAND_PTADDRESS/itp_lfs_flash_block_size[disk],
        (CFG_LFS_NAND_PTADDRESS%itp_lfs_flash_block_size[disk])/itp_lfs_flash_page_size[disk]);
        free(itp_lfs_nf_page_buffer_inner);
        return -1;
    }
    memcpy(pt, itp_lfs_nf_page_buffer_inner+(CFG_LFS_NAND_PTADDRESS%itp_lfs_flash_page_size[disk]), sizeof(*pt));
    free(itp_lfs_nf_page_buffer_inner);
#endif

#else
#if defined(CFG_SD0_PARTITION0)
    if(disk == ITP_DISK_SD0)
    {
        if (iteSdReadMultipleSectorEx(SD_0, CFG_LFS_SD0_PTADDRESS/itp_lfs_flash_page_size[disk], 1, itp_lfs_sd_sector_buffer))
        {
            printf("[X] %s():iteSdReadMultipleSectorEx(%d, %d)\n", __FUNCTION__,
            CFG_LFS_SD0_PTADDRESS/itp_lfs_flash_page_size[disk],
            1);
            return -1;
        }
        memcpy(pt, itp_lfs_sd_sector_buffer+(CFG_LFS_SD0_PTADDRESS%itp_lfs_flash_page_size[disk]), sizeof(*pt));
    }
    else
#endif
#if defined(CFG_SD1_PARTITION0)
    if(disk == ITP_DISK_SD1)
    {
        if (iteSdReadMultipleSectorEx(SD_1, CFG_LFS_SD1_PTADDRESS/itp_lfs_flash_page_size[disk], 1, itp_lfs_sd_sector_buffer))
        {
            printf("[X] %s():iteSdReadMultipleSectorEx(%d, %d)\n", __FUNCTION__,
            CFG_LFS_SD1_PTADDRESS/itp_lfs_flash_page_size[disk],
            1);
            return -1;
        }
        memcpy(pt, itp_lfs_sd_sector_buffer+(CFG_LFS_SD1_PTADDRESS%itp_lfs_flash_page_size[disk]), sizeof(*pt));
    }
    else
#endif
#if defined(CFG_NOR_PARTITION0)
    if(disk == ITP_DISK_NOR)
    {
/*#ifdef CFG_NOR_USE_DPUAES
    tmp2 = memalign(4, 256);

    pthread_mutex_lock(&DPU_AES_MUTEX);

    n = NorSetAESKey(SPI_0, SPI_CSN_0, cipher_key);

    if (n == 0)
        n=NorReadAES(SPI_0, SPI_CSN_0, CFG_LFS_NOR_PTADDRESS, (uint8_t *)tmp2, sizeof(tmp));

    pthread_mutex_unlock(&DPU_AES_MUTEX);

    memcpy((uint8_t *)pt, (uint8_t *)tmp2, sizeof(*pt));
    free(tmp2);
#else*/
        n=NorRead(SPI_0, NOR_CSN, CFG_LFS_NOR_PTADDRESS, (uint8_t *)tmp, sizeof(tmp));
        memcpy((uint8_t *)pt, (uint8_t *)tmp, sizeof(*pt));
    }
/*#endif*/
#endif
#endif
    hexdump("littlefs partition table (rd):\n", (unsigned char*)pt, sizeof(*pt));

    // check if valid
    if (memcmp(pt->tag, ITP_LFS_TAG, strlen(ITP_LFS_TAG))) {
        ITP_LOG_ERR("not a valid littlefs partition table\n" );
        return -1;
    }

    //printf("[i] itp_lfs_read_partition_table\n");
    for (i=0; i<sizeof(pt->partitions)/sizeof(pt->partitions[0]); i++) {
        if ((pt->partitions[i].block_start==-1)||(pt->partitions[i].blocks==-1)) {
            break;
        }
        pt->partitions[i].block_start/=itp_lfs_flash_block_size[disk];
        pt->partitions[i].blocks/=itp_lfs_flash_block_size[disk];
    }

    return 0;
}

static int itp_lfs_write_partition_table(ITPDisk disk, struct itp_lfs_partition_table* pt)
{
    struct itp_lfs_partition_table ptable;
    int i,n;
#if defined(CFG_SD0_PARTITION0) || defined(CFG_SD1_PARTITION0)
    unsigned char itp_lfs_sd_sector_buffer[512];
#endif
/*#ifdef CFG_NOR_USE_DPUAES
    unsigned char *tmp2 = 0;
#endif*/
#if defined(CFG_NAND_ENABLE)
    unsigned char *itp_lfs_nf_page_buffer_inner=NULL;
#endif

    if ((disk!=ITP_DISK_NOR) && (disk!=ITP_DISK_NAND) && (disk!=ITP_DISK_SD0) && (disk!=ITP_DISK_SD1)) {
        return -1;
    }

    memset(&ptable, 0, sizeof(ptable));
    memcpy(ptable.tag, ITP_LFS_TAG, strlen(ITP_LFS_TAG));
    for (i=0; i<sizeof(ptable.partitions)/sizeof(ptable.partitions[0]); i++) {
        ptable.partitions[i].block_start=pt->partitions[i].block_start*itp_lfs_flash_block_size[disk];
        ptable.partitions[i].blocks     =pt->partitions[i].blocks*itp_lfs_flash_block_size[disk];
    }
    hexdump("littlefs partition table (wr):\n", (unsigned char*)&ptable, sizeof(ptable));
#if defined(CFG_NAND_ENABLE)
#if (CFG_LFS_HAVE_HCCFTL)
    itp_lfs_nf_page_buffer_inner=(unsigned char *)malloc(itp_lfs_flash_page_size[disk]+itp_lfs_flash_spare_size[disk]+256);
    if (!itp_lfs_nf_page_buffer_inner)
    {
        printf("[X] %s(itp_lfs_nf_page_buffer_inner malloc error)\n", __FUNCTION__);
        return -1;
    }
    memset(itp_lfs_nf_page_buffer_inner, 0, f_phy.bytes_per_sector);
    memcpy(itp_lfs_nf_page_buffer_inner, &ptable, sizeof(ptable));
    n=f_driver->writemultiplesector(f_driver, itp_lfs_nf_page_buffer_inner, 0, 1);
    if (n) {
        ITP_LOG_ERR(
            "error writing partition table in FTL sector 0!\n"
        );
        free(itp_lfs_nf_page_buffer_inner);
        return -1;
    }
    free(itp_lfs_nf_page_buffer_inner);
#else
    int block=CFG_LFS_NAND_PTADDRESS/itp_lfs_flash_block_size[disk];
    int page=(CFG_LFS_NAND_PTADDRESS%itp_lfs_flash_block_size[disk])/itp_lfs_flash_page_size[disk];
    int page2=page-sizeof(bad_block_table)/itp_lfs_flash_page_size[disk];
    int ret=0;
    uint32_t j=0, k=0;

    if (iteNfIsBadBlockForBoot(block)<0)
    {
        printf("[itp_lfs_write_partition_table error] %d is initial bad block(%d)\n", block, itp_lfs_flash_block_size[disk]);
        return -1;
    }
    else
    {
        bad_block_table[block]=0;
    }

    i = 0;
    while (pt->partitions[i].block_start>0)
    {
        j = 0;
        while (j<pt->partitions[i].blocks)
        {
            k = pt->partitions[i].block_start+j;
            ret = iteNfIsBadBlockForBoot(k);
            if (ret<0)
            {
                printf("[itp_lfs_write_partition_table] %d is initial bad block(%d)\n", k, itp_lfs_flash_block_size[disk]);
                bad_block_table[k]=1;
            }
            else
            {
                bad_block_table[k]=0;
            }
            j++;
        }
        i++;
    }

    for (i=0; i<itp_lfs_nf_info.PageInBlk; i++) {
        n=iteNfRead(
            block,
            i,
            itp_lfs_nf_block_buffer+i*(itp_lfs_flash_page_size[disk]+itp_lfs_flash_spare_size[disk]));
        if (n!=true) {
            printf("[X] %s():iteNfRead(%d, %d)\n", __FUNCTION__, block, i);
            return -1;
        }
    }
    i = (int)bad_block_table;
    while (page2<page)
    {
        memcpy(itp_lfs_nf_block_buffer+page2*(itp_lfs_flash_page_size[disk]+itp_lfs_flash_spare_size[disk]),
                (uint8_t *)i,
                itp_lfs_flash_page_size[disk]);
        i+=itp_lfs_flash_page_size[disk];
        page2++;
    }
    memcpy(itp_lfs_nf_block_buffer+page*(itp_lfs_flash_page_size[disk]+itp_lfs_flash_spare_size[disk])+(CFG_LFS_NAND_PTADDRESS%itp_lfs_flash_page_size[disk]),
            (uint8_t*)&ptable,
            sizeof(ptable));
    n=iteNfErase(block);
    if (n!=true) {
        printf("[X] %s():iteNfErase(%d)\n", __FUNCTION__, block);
        return -1;
    }
    for (i=0; i<itp_lfs_nf_info.PageInBlk; i++) {
        printf("[i] write %d:%02d\n", block, i);
        n=iteNfWrite(
                block,
                i,
                itp_lfs_nf_block_buffer+i*(itp_lfs_flash_page_size[disk]+itp_lfs_flash_spare_size[disk]),
                itp_lfs_nf_block_buffer+i*(itp_lfs_flash_page_size[disk]+itp_lfs_flash_spare_size[disk])+itp_lfs_flash_page_size[disk]);
        if (n!=true) {
            printf("[X] %s():iteNfWrite(%d, %d)\n", __FUNCTION__, block, i);
            return -1;
        }
    }
#endif
#else
#if defined(CFG_SD0_PARTITION0)
    if(disk == ITP_DISK_SD0)
    {
        if (iteSdReadMultipleSectorEx(SD_0, CFG_LFS_SD0_PTADDRESS/itp_lfs_flash_page_size[disk], 1, itp_lfs_sd_sector_buffer))
        {
            printf("[X] %s():iteSdReadMultipleSectorEx(%d, %d)\n", __FUNCTION__,
            CFG_LFS_SD0_PTADDRESS/itp_lfs_flash_page_size[disk],
            1);
            return -1;
        }
        memcpy(itp_lfs_sd_sector_buffer + CFG_LFS_SD0_PTADDRESS % itp_lfs_flash_page_size[disk], &ptable, sizeof(ptable));
        if (iteSdWriteMultipleSectorEx(SD_0, CFG_LFS_SD0_PTADDRESS/itp_lfs_flash_page_size[disk], 1, itp_lfs_sd_sector_buffer))
        {
            printf("[X] %s():iteSdWriteMultipleSectorEx(%d, %d)\n", __FUNCTION__,
            CFG_LFS_SD0_PTADDRESS/itp_lfs_flash_page_size[disk],
            1);
            return -1;
        }
    }
    else
#endif
#if defined(CFG_SD1_PARTITION0)
    if(disk == ITP_DISK_SD1)
    {
        if (iteSdReadMultipleSectorEx(SD_1, CFG_LFS_SD1_PTADDRESS/itp_lfs_flash_page_size[disk], 1, itp_lfs_sd_sector_buffer))
        {
            printf("[X] %s():iteSdReadMultipleSectorEx(%d, %d)\n", __FUNCTION__,
            CFG_LFS_SD1_PTADDRESS/itp_lfs_flash_page_size[disk],
            1);
            return -1;
        }
        memcpy(itp_lfs_sd_sector_buffer + CFG_LFS_SD1_PTADDRESS % itp_lfs_flash_page_size[disk], &ptable, sizeof(ptable));
        if (iteSdWriteMultipleSectorEx(SD_1, CFG_LFS_SD1_PTADDRESS/itp_lfs_flash_page_size[disk], 1, itp_lfs_sd_sector_buffer))
        {
            printf("[X] %s():iteSdWriteMultipleSectorEx(%d, %d)\n", __FUNCTION__,
            CFG_LFS_SD1_PTADDRESS/itp_lfs_flash_page_size[disk],
            1);
            return -1;
        }
    }
    else
#endif
#if defined(CFG_NOR_PARTITION0)
    if(disk == ITP_DISK_NOR)
    {
    /*#ifdef CFG_NOR_USE_DPUAES
        tmp2 = memalign(4, 256);
        memcpy((uint8_t *)tmp2, (uint8_t *)&ptable, sizeof(ptable));

        pthread_mutex_lock(&DPU_AES_MUTEX);

        n = NorSetAESKey(SPI_0, SPI_CSN_0, cipher_key);

        if (n == 0)
            n=NorWriteAES(SPI_0, SPI_CSN_0, CFG_LFS_NOR_PTADDRESS, (uint8_t *)tmp2, sizeof(ptable));

        pthread_mutex_unlock(&DPU_AES_MUTEX);

        free(tmp2);
    #else*/
        n=NorWrite(SPI_0, NOR_CSN, CFG_LFS_NOR_PTADDRESS, (uint8_t*)&ptable, sizeof(ptable));
    }
/*#endif*/
#endif
#endif
    return 0;
}

static int itp_lfs_mount(ITPDisk disk)
{
    struct itp_lfs_partition_table pt;
    struct itp_lfs_context *context;
    int lfs_count = 0;
    int i, j;
    ITPDriveStatus* drive_status = NULL;
    ITPDriveStatus* driveStatusTable = NULL;

    switch (disk)
    {
    case ITP_DISK_NOR:
    case ITP_DISK_NAND:
    case ITP_DISK_SD0:
    case ITP_DISK_SD1:
        break;
    default:
        ITP_LOG_INFO("unsupport disk: %d\n", disk );
        return -1;
    }

    if (itp_lfs_read_partition_table(disk, &pt)<0) {
        return -1;
    }

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    // mount lfs partitions
    for (i=j=0; i<sizeof(pt.partitions)/sizeof(pt.partitions[0]); i++) {
        if ((pt.partitions[i].block_start==0) || (pt.partitions[i].blocks==0)) {
            // no more partitions
            break;
        }

#if 0 && defined(CFG_NAND_ENABLE) && (!CFG_LFS_HAVE_HCCFTL)
    {
    char msg[256];

    sprintf(msg, "block 0:%d (mount %02d:%02d)\n", pt.partitions[i].block_start, disk, i);
    iteNfRead(pt.partitions[i].block_start, 0, itp_lfs_nf_page_buffer);
    hexdump(msg, itp_lfs_nf_page_buffer, 256);

    sprintf(msg, "block 1:%d (mount %02d:%02d)\n", pt.partitions[i].block_start+1, disk, i);
    iteNfRead(pt.partitions[i].block_start+1, 0, itp_lfs_nf_page_buffer);
    hexdump(msg, itp_lfs_nf_page_buffer, 256);
    }
#endif

        // find an empty drive
        for (; j < ITP_MAX_DRIVE; j++)
        {
            drive_status = driveStatusTable+j;

            if (!drive_status->avail)
                break;
//printf("[i] %02d: %c:%02d:%02d skipped\n\n", j, drive_status->name[0], drive_status->disk, drive_status->device);
//sleep(1);
        }

        drive_status->name[0] = 'A' + j;
//printf("[i] %02d: %c selected\n\n", j, drive_status->name[0]);
//sleep(1);
        drive_status->disk       = disk;
#if (CFG_LFS_CACHE_SIZE > 0)
        lfs_cache_set_flashType((uint32_t)disk, lfs_cache[disk]);
#endif
        // todo: get actual NOR flash params
        context=itp_lfs_new_flash_context(pt.partitions[i].block_start, pt.partitions[i].blocks, drive_status, disk);

        if (lfs_mount(&context->lfs, &context->config)>=0) {
printf("[i] mount lfs %02d %c: off: %08X len: %08X\n\n", i, drive_status->name[0], pt.partitions[i].block_start*itp_lfs_flash_block_size[disk], pt.partitions[i].blocks*itp_lfs_flash_block_size[disk]);
//sleep(1);
            drive_to_context[j]      = context;

            //drive_status->disk       = disk;
            drive_status->device     = ITP_DEVICE_LFS;
            drive_status->avail      = true;
            drive_status->name[1]    = ':';
            drive_status->name[2]    = '/';
            write(ITP_DEVICE_DRIVE, drive_status, sizeof (ITPDriveStatus));
            context->partition=lfs_count++;
        }
        else {
printf("[X] mount lfs %02d %c: off: %08X len: %08X\n\n", i, drive_status->name[0], pt.partitions[i].block_start*itp_lfs_flash_block_size[disk], pt.partitions[i].blocks*itp_lfs_flash_block_size[disk]);
//sleep(1);
            itp_lfs_free_context(&context);
        }
    }
//printf("[i] sleep 10s..\n");
//sleep(10);
    return lfs_count?0:-1;
}

static int itp_lfs_umount(ITPDisk disk)
{
    struct itp_lfs_context* context;
    int i;

    for (i=0; i<sizeof(drive_to_context)/sizeof(drive_to_context[0]); i++) {
        context=drive_to_context[i];
        if (context && (context->drive_status->disk==disk)) {
            lfs_unmount(&context->lfs);
            drive_to_context[i]=NULL;
            context->drive_status->avail=false;
            write(ITP_DEVICE_DRIVE, context->drive_status, sizeof (ITPDriveStatus));
            itp_lfs_free_context(&context);
        }
    }
    return 0;
}

static int itp_lfs_create_partitions(ITPPartition* par)
{
    struct itp_lfs_context* context;
    struct itp_lfs_partition_table pt;
    int i;

    switch (par->disk)
    {
    case ITP_DISK_NOR:
    case ITP_DISK_NAND:
    case ITP_DISK_SD0:
    case ITP_DISK_SD1:
        break;
    default:
        ITP_LOG_INFO("unsupport disk: %d\n", par->disk );
        return -1;
    }

    memset(&pt, 0, sizeof(pt));

    for (i = 0; i < par->count; i++)
    {
        pt.partitions[i].block_start=(uint32_t)(par->start[i]/itp_lfs_flash_block_size[par->disk]);
        pt.partitions[i].blocks=(uint32_t)(par->size[i]/itp_lfs_flash_block_size[par->disk]);
        if (pt.partitions[i].block_start && !pt.partitions[i].blocks) {
            pt.partitions[i].blocks=itp_lfs_flash_blocks[par->disk]-pt.partitions[i].block_start;
            break;
        }
        //context=itp_lfs_new_flash_context(pt.partitions[i].block_start, pt.partitions[i].blocks, NULL);
        //lfs_format(&context->lfs, &context->config);
        //itp_lfs_free_context(&context);
    }

    // TODO: write partition table
    itp_lfs_write_partition_table(par->disk, &pt);

    return 0;
}

static int itp_lfs_get_partitions(ITPPartition* par)
{
    struct itp_lfs_context* context;
    struct itp_lfs_partition_table pt;
    int i;

    switch (par->disk)
    {
    case ITP_DISK_NOR:
    case ITP_DISK_NAND:
    case ITP_DISK_SD0:
    case ITP_DISK_SD1:
        break;
    default:
        ITP_LOG_INFO("unsupport disk: %d\n", par->disk );
        return -1;
    }

    //TODO: read partition table
    if (itp_lfs_read_partition_table(par->disk, &pt)<0) {
        return -1;
    }

    par->count=0;
    memset(par->size, 0, sizeof(par->size));
    memset(par->start, 0, sizeof(par->start));

    for (i=0; pt.partitions[i].block_start || pt.partitions[i].blocks; i++) {
        par->start[i]=pt.partitions[i].block_start*itp_lfs_flash_block_size[par->disk];
        par->size[i]=pt.partitions[i].blocks*itp_lfs_flash_block_size[par->disk];

    }
    par->count=i;

    return 0;
}

static int itp_lfs_format_partition(ITPDisk disk, int partition)
{
   struct itp_lfs_context* context;
   struct itp_lfs_partition_table pt;
   int ret;

printf("[i] %s(%d,%d)\n", __FUNCTION__, disk, partition);
    if ((disk!=ITP_DISK_NOR) && (disk!=ITP_DISK_NAND) && (disk!=ITP_DISK_SD0) && (disk!=ITP_DISK_SD1)) {
        return -1;
    }

    if (partition>=sizeof(pt.partitions)/sizeof(pt.partitions[0])) {
        return -1;
    }

    //TODO: read partition table
    if (itp_lfs_read_partition_table(disk, &pt)<0) {
        return -1;
    }

    if (!pt.partitions[partition].block_start || !pt.partitions[partition].blocks) {
        return -1;
    }
#if (CFG_LFS_CACHE_SIZE > 0)
    lfs_cache_set_flashType((uint32_t)disk, lfs_cache[disk]);
#endif
    context=itp_lfs_new_flash_context(pt.partitions[partition].block_start, pt.partitions[partition].blocks, NULL, disk);
#if 0&&defined(CFG_NAND_ENABLE)&&(!CFG_LFS_HAVE_HCCFTL)
{
char msg[256];
sprintf(msg, "block 0:%d (++format %02d:%02d)\n", pt.partitions[partition].block_start, disk, partition);
iteNfRead(pt.partitions[partition].block_start, 0, itp_lfs_nf_page_buffer);
hexdump(msg, itp_lfs_nf_page_buffer, 256);

sprintf(msg, "block 1:%d (++format %02d:%02d)\n", pt.partitions[partition].block_start+1, disk, partition);
iteNfRead(pt.partitions[partition].block_start+1, 0, itp_lfs_nf_page_buffer);
hexdump(msg, itp_lfs_nf_page_buffer, 256);
}
#endif
    ret=lfs_format(&context->lfs, &context->config);
#if 0&&defined(CFG_NAND_ENABLE)&&(!CFG_LFS_HAVE_HCCFTL)
{
char msg[256];
sprintf(msg, "block 0:%d (--format %02d:%02d)\n", pt.partitions[partition].block_start, disk, partition);
iteNfRead(pt.partitions[partition].block_start, 0, itp_lfs_nf_page_buffer);
hexdump(msg, itp_lfs_nf_page_buffer, 256);

sprintf(msg, "block 1:%d (--format %02d:%02d)\n", pt.partitions[partition].block_start+1, disk, partition);
iteNfRead(pt.partitions[partition].block_start+1, 0, itp_lfs_nf_page_buffer);
hexdump(msg, itp_lfs_nf_page_buffer, 256);
}
#endif
    itp_lfs_free_context(&context);

    return ret;
}

static lfs_file_t* itp_files[OPEN_MAX]={0};
static lfs_t* itp_lfses[OPEN_MAX];
static int   itp_drives[OPEN_MAX];
static int itp_lfs_open(const char* name, int flags, int mode, void* info)
{
    ITP_LFS_PREPAREBYPATH(name);
    lfs_file_t *file = NULL;
    int lfs_flags = 0;
    int i, err;
//printf("[i] %s: %s, 0x%04X\n", __FUNCTION__, name, flags);
    ithEnterCritical();
    for (i=0; i<OPEN_MAX; i++) {
        if (!itp_files[i]) {
            itp_files[i]=file=calloc(1, sizeof(lfs_file_t));
            itp_lfses[i]=lfs;
            itp_drives[i]=drive;
            break;
        }
    }
    ithExitCritical();

    if (!file) {
        return -1;
    }



    if (flags & O_RDONLY) lfs_flags |= LFS_O_RDONLY;
    if (flags & O_WRONLY) lfs_flags |= LFS_O_WRONLY;
    if (flags & O_RDWR)   lfs_flags |= LFS_O_RDWR;
    if (flags & O_CREAT)  lfs_flags |= LFS_O_CREAT;
    if (flags & O_EXCL)   lfs_flags |= LFS_O_EXCL;
    if (flags & O_TRUNC)  lfs_flags |= LFS_O_TRUNC;
    if (flags & O_APPEND) lfs_flags |= LFS_O_APPEND;
//printf("[i] lfs_flags=%04X\n", flags);
    err = lfs_file_open(lfs, file, name+2, lfs_flags);
    if (err) {
        itp_files[i]=NULL;
        free(file);
        return err;
    }


    return i;
}

static int itp_lfs_close(int fd, void* info)
{
    ITP_LFS_PREPAREBYFD();
    int err = lfs_file_close(lfs, file);
    free(file);
    itp_files[fd]=NULL;
    return err;
}

static int itp_lfs_read(int fd, char *ptr, int len, void* info)
{
    ITP_LFS_PREPAREBYFD();
    return lfs_file_read(lfs, file, ptr, len);
}

static int itp_lfs_write(int fd, char *ptr, int len, void* info)
{
    ITP_LFS_PREPAREBYFD();
    return lfs_file_write(lfs, file, ptr, len);
}

static int itp_lfs_seek(int fd, int ptr, int dir, void* info)
{
    ITP_LFS_PREPAREBYFD();
    return lfs_file_seek(lfs, file,  ptr, dir);
}

static int itp_lfs_remove(const char *path)
{
    ITP_LFS_PREPAREBYPATH(path);

    return lfs_remove(lfs, path+2);
}

static int itp_lfs_rename(const char *oldname, const char *newname)
{
    ITP_LFS_PREPAREBYPATH(oldname);

    return lfs_rename(lfs, oldname+2, newname+2);
}

static int itp_lfs_chdir(const char *path)
{
    struct itp_lfs_context* context;
    char *realpath;

    realpath=(char*)malloc(PATH_MAX+2);
    if (!realpath) {
        errno=ENOMEM;
        return -1;
    }

    if (!itp_lfs_realpath(path, realpath)) {
        free(realpath);
        return -1;
    }

    context=itp_lfs_get_context(realpath[0]);
    strcpy(context->cwd, realpath+2);
    free(realpath);

    return 0;
}

static int itp_lfs_chmod(const char *path, mode_t mode)
{
    return 0;
}

static int itp_lfs_mkdir(const char *path, mode_t mode)
{
    ITP_LFS_PREPAREBYPATH(path);

    return lfs_mkdir(lfs, path+2);
}

static int itp_lfs_rmdir(const char *path)
{
    ITP_LFS_PREPAREBYPATH(path);

    return lfs_remove(lfs, path+2);
}

static void itp_lfs_tostat(struct stat *s, struct lfs_info *info) {
    memset(s, 0, sizeof(struct stat));

    s->st_size = info->size;
    s->st_mode = S_IRWXU | S_IRWXG | S_IRWXO;

    switch (info->type) {
        case LFS_TYPE_DIR: s->st_mode |= S_IFDIR; break;
        case LFS_TYPE_REG: s->st_mode |= S_IFREG; break;
    }
}

static int itp_lfs_stat(const char *path, struct stat *sbuf)
{
    int n;
    ITP_LFS_PREPAREBYPATH(path);
    struct lfs_info lfs_info;

    n=lfs_stat(lfs, path+2, &lfs_info);
    if (n>=0) {
        itp_lfs_tostat(sbuf, &lfs_info);
    }
    return (n>=0)?0:-1;
}

static int itp_lfs_statvfs(const char *path, struct statvfs *sbuf)
{
    ITP_LFS_PREPAREBYPATH(path);

    lfs_ssize_t in_use = lfs_fs_size(lfs);
    if (in_use < 0) {
        return -1;
    }

    memset(sbuf, 0, sizeof(*sbuf));
    sbuf->f_bsize = config->block_size;
    sbuf->f_frsize = config->block_size;
    sbuf->f_blocks = config->block_count;
    sbuf->f_bfree = config->block_count - in_use;
    sbuf->f_bavail = config->block_count - in_use;
    sbuf->f_namemax = config->name_max;
    sbuf->f_files    = -1;
    sbuf->f_ffree    = -1;
    sbuf->f_favail   = -1;
    sbuf->f_flag     = -1;
    sbuf->f_fsid     = ITP_DEVICE_LFS;
    #if (0)
    printf("[i] %s: in_use   =%d\n", __FUNCTION__, in_use); usleep(1000*2);
    printf("[i] %s: f_bsize  =%d\n", __FUNCTION__, sbuf->f_bsize); usleep(1000*2);
    printf("[i] %s: f_frsize =%d\n", __FUNCTION__, sbuf->f_frsize); usleep(1000*2);
    printf("[i] %s: f_blocks =%d\n", __FUNCTION__, sbuf->f_blocks); usleep(1000*2);
    printf("[i] %s: f_bfree  =%d\n", __FUNCTION__, sbuf->f_bfree); usleep(1000*2);
    printf("[i] %s: f_bavail =%d\n", __FUNCTION__, sbuf->f_bavail); usleep(1000*2);
    printf("[i] %s: f_namemax=%d\n", __FUNCTION__, sbuf->f_namemax); usleep(1000*2);
    #endif
    return 0;
}

static int itp_lfs_fstat(int fd, struct stat *st)
{
    ITP_LFS_PREPAREBYFD();

    itp_lfs_tostat(st, &(struct lfs_info){
        .size = lfs_file_size(lfs, file),
        .type = LFS_TYPE_REG,
    });

    return 0;
}

static char* itp_lfs_getcwd(char *buf, size_t size)
{
    struct itp_lfs_context* context=itp_lfs_get_context('A'+itpTaskGetVolume());

    if (!context) {
        return NULL;
    }

    buf[0]=context->drive_status->name[0];
    buf[1]=':';
    strlcpy(buf+2, context->cwd, size-2);
    return buf;
}

typedef struct itp_lfs_dir {
    lfs_dir_t dir;
    lfs_t *lfs;
    struct dirent dirent;
} itp_lfs_dir_t;

static DIR* itp_lfs_opendir(const char *path)
{
    ITP_LFS_PREPAREBYPATH(path);
    itp_lfs_dir_t *dir;
    int err;

    DIR* dirp = malloc(sizeof (DIR));
    if (!dirp)
    {
        errno = ENOMEM;
        return NULL;
    }

    dir = malloc(sizeof(*dir));
    if (!dir) {
        free(dirp);
        errno = ENOMEM;
        return NULL;
    }
    memset(dir, 0, sizeof(*dir));
    dir->lfs=lfs;

    err = lfs_dir_open(lfs, &dir->dir, path+2);
    if (err) {
        free(dir);
        free(dirp);
        errno=err;
        return NULL;
    }

    dirp->dd_fd     = ITP_DEVICE_LFS;
    dirp->dd_size   = sizeof (*dir);
    dirp->dd_buf    = (char*) dir;
    dirp->dd_loc    = -1;

    return dirp;
}

static struct dirent* itp_lfs_readdir(DIR *dirp)
{
    itp_lfs_dir_t *dir=(itp_lfs_dir_t*)dirp->dd_buf;
    struct dirent* d=&dir->dirent;
    struct lfs_info info;

    if (lfs_dir_read(dir->lfs, &dir->dir, &info)<=0) {
        return NULL;
    }

    memset(d, 0, sizeof(*d));
    d->d_type=(info.type==LFS_TYPE_DIR)?DT_DIR:DT_REG;
    d->d_namlen=strlen(info.name);
    strcpy(d->d_name, info.name);
//printf("[i] readdir: %s\n", d->d_name);
    return d;
}

static void itp_lfs_rewidndir(DIR *dirp)
{
    itp_lfs_dir_t *dir=(itp_lfs_dir_t*)dirp->dd_buf;
    struct dirent* d=&  dir->dirent;

    lfs_dir_rewind(dir->lfs, &dir->dir);
}

static int itp_lfs_closedir(DIR *dirp)
{
    itp_lfs_dir_t *dir=(itp_lfs_dir_t*)dirp->dd_buf;
    struct dirent* d=&dir->dirent;
    int err = lfs_dir_close(dir->lfs, &dir->dir);
    free(dir);
    free(dirp);
    return err;
}

static long itp_lfs_tell(int fd)
{
    ITP_LFS_PREPAREBYFD();

    return lfs_file_tell(lfs, file);
}

static int	itp_lfs_flush(int fd)
{
    ITP_LFS_PREPAREBYFD();

    return lfs_file_sync(lfs, file);
}

static int	itp_lfs_eof(int fd)
{
    ITP_LFS_PREPAREBYFD();

    return (file->pos>=file->ctz.size)?EOF:0;
}

static int itp_lfs_ioctl(int fd, unsigned long   request, void* ptr, void* info)
{
    int ret;
    int i;

    switch (request)
    {
    case ITP_IOCTL_MOUNT:
        ret = itp_lfs_mount((ITPDisk)ptr);
        if (ret)
        {
            errno = (ITP_DEVICE_LFS << ITP_DEVICE_ERRNO_BIT) | ret;
            return -1;
        }
        break;

    case ITP_IOCTL_UNMOUNT:
        ret = itp_lfs_umount((ITPDisk)ptr);
        if (ret)
        {
            errno = (ITP_DEVICE_LFS << ITP_DEVICE_ERRNO_BIT) | ret;
            return -1;
        }
        break;

    case ITP_IOCTL_ENABLE:
        break;

    case ITP_IOCTL_DISABLE:
        break;

    case ITP_IOCTL_CREATE_PARTITION:
        ret = itp_lfs_create_partitions((ITPPartition*)ptr);
        if (ret)
        {
            errno = (ITP_DEVICE_LFS << ITP_DEVICE_ERRNO_BIT) | ret;
            return -1;
        }
        break;

    case ITP_IOCTL_GET_PARTITION:
        ret = itp_lfs_get_partitions((ITPPartition*)ptr);
        if (ret)
        {
            errno = (ITP_DEVICE_LFS << ITP_DEVICE_ERRNO_BIT) | ret;
            return -1;
        }
        break;

#if (CFG_LFS_CACHE_SIZE > 0)
    case ITP_IOCTL_FLUSH:
        for(i = 0 ; i < ITH_DISK_MAX ; i ++)
        {
            if(lfs_cache[i])
                ret = lfs_cache_flush(lfs_cache[i]);
            if (ret)
            {
                errno = (ITP_DEVICE_LFS << ITP_DEVICE_ERRNO_BIT) | ret;
                return -1;
            }
        }
        break;
#endif

    case ITP_IOCTL_FORMAT_PARTITION:
        ret = itp_lfs_format_partition((ITPDisk)((((uint32_t)ptr)>>8)&0x00ff), (int)(((uint32_t)ptr)&0x00ff));
        if (ret)
        {
            errno = (ITP_DEVICE_LFS << ITP_DEVICE_ERRNO_BIT) | ret;
            return -1;
        }
        break;

    case ITP_IOCTL_INIT:
        printf("itp_lfs_init!\n");
        ret=itp_lfs_init();
        if (ret) {
            return -1;
        }
        break;

    case ITP_IOCTL_EXIT:
        printf("itp_lfs_exit!\n");
        ret=itp_lfs_exit();
        if (ret) {
            return -1;
        }
        break;

    default:
        errno = (ITP_DEVICE_LFS << ITP_DEVICE_ERRNO_BIT) | __LINE__;
        return -1;
    }
    return 0;
}

const ITPFSDevice itpLittlefsDevice =
{
    {
        ":lfs",
        itp_lfs_open,
        itp_lfs_close,
        itp_lfs_read,
        itp_lfs_write,
        itp_lfs_seek,
        itp_lfs_ioctl,
        NULL
    },
    itp_lfs_remove,
    itp_lfs_rename,
    itp_lfs_chdir,
    itp_lfs_chmod,
    itp_lfs_mkdir,
    itp_lfs_stat,
    itp_lfs_statvfs,
    itp_lfs_fstat,
    itp_lfs_getcwd,
    itp_lfs_rmdir,
    itp_lfs_closedir,
    itp_lfs_opendir,
    itp_lfs_readdir,
    itp_lfs_rewidndir,
    itp_lfs_tell,
    itp_lfs_flush,
    itp_lfs_eof
};

/*
 * char *realpath(const char *path, char resolved[PATH_MAX+2]);
 *
 * Find the real name of path, by removing all ".", ".." and symlink
 * components.  Returns (resolved) on success, or (NULL) on failure,
 * in which case the path which caused trouble is left in (resolved).
 */
static char *
itp_lfs_realpath(const char *path, char *_resolved)
{
	struct stat sb;
	char *p, *q, *s, *left, *next_token;
	size_t left_len, resolved_len;
	char *resolved=_resolved+2, *ret=NULL;
    int volume=itpTaskGetVolume();
    struct itp_lfs_context* context;

    left=(char*)malloc(PATH_MAX);
    next_token=(char*)malloc(PATH_MAX);
    if (!left || !next_token) {
        errno=ENOMEM;
        goto lExit;
    }

	if (path[0] == '/') {
        context=itp_lfs_get_context('A'+volume);
        if (!context) {
            errno=ENOENT;
            goto lExit;
        }
		resolved[0] = '/';
		resolved[1] = '\0';
		resolved_len = strlen(resolved);
		left_len = strlcpy(left, path + 1, PATH_MAX);
	} else if (path[1]==':') {
        context=itp_lfs_get_context(path[0]);
        if (!context) {
            errno=ENOENT;
            goto lExit;
        }
		if (path[2]=='/') {
			resolved[0] = '/';
			resolved[1] = '\0';
			resolved_len = strlen(resolved);
			left_len = strlcpy(left, path + 3, PATH_MAX);
		}
		else {
            strcpy(resolved, context->cwd);
			resolved_len = strlen(resolved);
			left_len = strlcpy(left, path + 2, PATH_MAX);
		}
	}
	else {
        context=itp_lfs_get_context('A'+volume);
        if (!context) {
            errno=ENOENT;
            goto lExit;
        }
        strcpy(resolved, context->cwd);
		resolved_len = strlen(resolved);
		left_len = strlcpy(left, path, PATH_MAX);
	}

	if (left_len >= PATH_MAX || resolved_len >= PATH_MAX) {
		errno = ENAMETOOLONG;
        goto lExit;
	}

	/*
	 * Iterate over path components in `left'.
	 */
	while (left_len != 0) {
		/*
		 * Extract the next path component and adjust `left'
		 * and its length.
		 */
		p = strchr(left, '/');
		s = p ? p : left + left_len;
		if (s - left >= PATH_MAX) {
			errno = ENAMETOOLONG;
            goto lExit;
		}
		memcpy(next_token, left, s - left);
		next_token[s - left] = '\0';
		left_len -= s - left;
		if (p != NULL)
			memmove(left, s + 1, left_len + 1);
		if (resolved[resolved_len - 1] != '/') {
			if (resolved_len + 1 >= PATH_MAX) {
				errno = ENAMETOOLONG;
				goto lExit;
			}
			resolved[resolved_len++] = '/';
			resolved[resolved_len] = '\0';
		}
		if (next_token[0] == '\0')
			continue;
		else if (strcmp(next_token, ".") == 0)
			continue;
		else if (strcmp(next_token, "..") == 0) {
			/*
			 * Strip the last path component except when we have
			 * single "/"
			 */
			if (resolved_len > 1) {
				resolved[resolved_len - 1] = '\0';
				q = strrchr(resolved, '/') + 1;
				*q = '\0';
				resolved_len = q - resolved;
			}
			continue;
		}

		/*
		 * Append the next path component and lstat() it. If
		 * lstat() fails we still can return successfully if
		 * there are no more path components left.
		 */
		resolved_len = strlcat(resolved, next_token, PATH_MAX);
		if (resolved_len >= PATH_MAX) {
			errno = ENAMETOOLONG;
			goto lExit;
		}
	}

	/*
	 * Remove trailing slash except when the resolved pathname
	 * is a single "/".
	 */
	if (resolved_len > 1 && resolved[resolved_len - 1] == '/')
		resolved[resolved_len - 1] = '\0';
    _resolved[0]=context->drive_status->name[0];
    _resolved[1]=':';
    ret=_resolved;
lExit:
    if (left) free(left);
    if (next_token) free(next_token);
	return ret;
}
