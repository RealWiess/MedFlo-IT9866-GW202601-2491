/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL NAND functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include <errno.h>
#include <malloc.h>
#include <string.h>

#include "itp_cfg.h"
#include "ssp/mmp_spi.h"
#include "ite/ite_nand.h"
#include "nand/mmp_nand.h"

#define FAST_ITP_NAND_READ

#if defined(CFG_NAND_RESERVED_SIZE) && (CFG_NAND_RESERVED_SIZE != 0)
    #define ENABLE_CHECK_RESERVED_AREA

    #ifndef CFG_UPGRADE_IMAGE_POS
        #define CFG_UPGRADE_IMAGE_POS 0
    #endif

    #if defined(CFG_NET_ETHERNET_MAC_ADDR_POS)
        #if (CFG_NET_ETHERNET_MAC_ADDR_POS >= CFG_UPGRADE_IMAGE_POS)
            #error "ERROR: CFG_NET_ETHERNET_MAC_ADDR_POS can't larger than CFG_UPGRADE_IMAGE_POS"
        #endif
    #endif

    #if (CFG_UPGRADE_IMAGE_POS >= CFG_NAND_RESERVED_SIZE)
        #error "ERROR: CFG_UPGRADE_IMAGE_POS can't larger than CFG_NAND_RESERVED_SIZE"
    #endif
#endif

#define NAND_OTP_SIZE	(512)
/***************************************************************************

****************************************************************************/
#ifdef  CFG_SPI_NAND
    #ifdef CFG_SPI_NAND_USE_AXISPI
        #include "ssp/mmp_axispi.h"
    #else
        #include "ssp/mmp_spi.h"
    #endif

    #ifdef  CFG_SPI_NAND_USE_SPI0
        #define NAND_SPI_PORT   SPI_0
    #endif

    #ifdef  CFG_SPI_NAND_USE_SPI1
        #define NAND_SPI_PORT   SPI_1
    #endif

    #ifdef CFG_SPI_NAND_USE_AXISPI
        #define NAND_SPI_PORT   SPI_0
        #define NAND_SPI_CLOCK  SPI_CLK_40M
    #else

        #ifndef NAND_SPI_PORT
            #error "ERROR: itp_nand.c MUST define the SPI NAND bus(SPI0 or SPI1)"
        #endif

        #ifdef  CFG_SPI0_40MHZ_ENABLE
            #define NAND_SPI_CLOCK  SPI_CLK_40M
        #else
            #define NAND_SPI_CLOCK  SPI_CLK_20M
        #endif
    #endif

#endif

#if defined(CFG_FS_FAT)
extern int remap_image_posistion;
#endif
extern uint32_t
GetNewBlockIndex (uint32_t blk);

#ifdef	CFG_ALLOW_CHANGE_RESERVED_SIZE
extern unsigned long g_ftl_reserved_size;
extern unsigned long g_pkg_reserved_size;
#endif
/********************
 * global variable *
 ********************/
static ITE_NF_INFO  itpNfInfo;
static uint8_t      GoodBlkCodeMark[4] = {'F', 'I', 'N', 'E'};
#if defined(CFG_HAVE_LFS) || defined(CFG_HAVE_YAFFS2)

ITE_NF_INFO itpGetNfInfo () {return itpNfInfo;}
#endif
static int              gFsMaxSec           = 0;
static int              gFsSecSize          = 0;
static int              gFtl_has_init       = 0;
static int              gFtl_flush_counter  = 0;

static pthread_mutex_t  nf_mutex            = PTHREAD_MUTEX_INITIALIZER;

/********************
 * private function *
 ********************/
static void
nf_lock_mutex (
    void)
{
    pthread_mutex_lock(&nf_mutex);
}

static void
nf_unlock_mutex (
    void)
{
    pthread_mutex_unlock(&nf_mutex);
}

static int
FlushBootRom ()
{
    char        * BlkBuf = NULL;
    int         res = true;
    uint32_t    i, j;
    uint32_t    BakBlkNum;

    nf_lock_mutex();

    if (!itpNfInfo.BootRomSize)
    {
        errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | 1;  // nand initial fail
        ITP_LOG_ERR("nand block initial error: \n");
        goto end;
    }

    // allocate block buffer
    BlkBuf = malloc(itpNfInfo.PageInBlk * (itpNfInfo.PageSize + itpNfInfo.SprSize));
    if (!BlkBuf)
    {
        errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | 1;  // nand initial fail
        ITP_LOG_ERR("nand block initial error: \n");
        goto end;
    }

    // calculate block number of boot ROM
    BakBlkNum = 3;

    for (i = 0; i < BakBlkNum; i++)
    {
        char * tmpbuf = BlkBuf;

        // read a block data from bak area(include spare data)
        for (j = 0; j < itpNfInfo.PageInBlk; j++)
        {
            if (iteNfRead(i, j, tmpbuf) != true)
            {
                errno   = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | 1;
                ITP_LOG_ERR("itp nand read block error: \n");
                res     = -1;
                goto end;
            }
            tmpbuf += (itpNfInfo.PageSize + itpNfInfo.SprSize);
        }

        // write to boot area a block size
        tmpbuf = BlkBuf;
        for (j = 0; j < itpNfInfo.PageInBlk; j++)
        {
            if (iteNfWrite(i, j, tmpbuf, tmpbuf + itpNfInfo.SprSize) != true)
            {
                errno   = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | 1; // nand initial fail
                ITP_LOG_ERR("itp nand write block error: \n");
                res     = -1;
                goto end;
            }
            tmpbuf += (itpNfInfo.PageSize + itpNfInfo.SprSize);
        }
    }

end:
    if (BlkBuf != NULL)
    {
        free(BlkBuf);
    }
    nf_unlock_mutex();
    return res;
}

#ifdef  CFG_UPGRADE_MARK_POS

/*
To handle the power-off ECC issue by erasing mark block & checking if erase fail.
If it passes, the PKG upgrade will go on.
*/
int
repairMarkBlkEccErr (
    uint32_t    b,
    uint32_t    p,
    uint8_t     * buf)
{
    uint32_t    mblk = b;
    uint32_t    mpg = p;
    uint32_t    blkSize = itpNfInfo.PageInBlk * itpNfInfo.PageSize;
    uint32_t    mkblk = 0; // = CFG_UPGRADE_MARK_POS / blkSize;
    uint32_t    i = 0, j = 0;
    uint32_t    addr        = 0;
    uint8_t     * blkBuff   = NULL;
    uint8_t     * tmpbuf    = NULL;
    int         ret         = 0;

    // ITP_LOG_DBG("itp repair MarkBlk ECC issue: blk=%x, pg=%x\n", b, p );

    if (!blkSize)
    {
        ITP_LOG_ERR("itp nand blkSize = 0\n");
        ret = -1;
        goto mEnd;
    }

    mkblk = CFG_UPGRADE_MARK_POS / blkSize;
    ITP_LOG_DBG("	repair MarkBlk: mkblk=%x\n", mkblk);

    if (!mkblk || (mkblk != mblk))
    {
        ITP_LOG_ERR("itp nand incorrect mark block: %x, %x\n", mkblk, mblk);
        ret = -1;
        goto mEnd;
    }

    if (blkBuff == NULL)
    {
        blkBuff = (uint8_t *)malloc(blkSize);
        if (blkBuff == NULL)
        {
            ITP_LOG_ERR("itp nand blkBuff is out of memory\n");
            ret = -1;
            goto mEnd;
        }
    }

    if (tmpbuf == NULL)
    {
        tmpbuf = (uint8_t *)malloc(itpNfInfo.PageSize + 512);
        if (tmpbuf == NULL)
        {
            ITP_LOG_ERR("itp nand blkBuff is out of memory\n");
            ret = -1;
            goto mEnd;
        }
    }

    /* backup mark block */
    addr = 0;
    for (i = 0; i < itpNfInfo.PageInBlk; i++)
    {
        if (iteNfRead(mblk, i, tmpbuf) != true)
        {
            // if read page fail
            memset(&blkBuff[addr], 0XFF, itpNfInfo.PageSize);
        }
        else
        {
            // backup mark block data if read OK
            memcpy(&blkBuff[addr], tmpbuf, itpNfInfo.PageSize);
        }
        addr += itpNfInfo.PageSize;
    }

    /* Erase mark block */
    if (iteNfErase(mblk) != true)
    {
        char    db[4096]    = {0};                                      // max page size = 4096
        char    sb[512]     = {0};                                      // max spare size = 512

        // mark as bad block
        iteNfWrite(mblk, 0, db, sb);

        errno   = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__; // erase fail
        ITP_LOG_ERR("itp nand Erase mark block fail!\n");
        ret     = -1;
        goto mEnd;
    }

    /* restore mark block */
    addr = 0;
    for (i = 0; i < itpNfInfo.PageInBlk; i++)
    {
        memcpy(tmpbuf, &blkBuff[addr], itpNfInfo.PageSize);
        // ITP_LOG_DBG("WtMarkBlk: %x, %x, %p\n",mblk,i,tmpbuf );

        if (iteNfWrite(mblk, i, tmpbuf, tmpbuf + itpNfInfo.SprSize) != true)
        {
            errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // write fail
            ITP_LOG_ERR("write fail:[%d,%d]\n", mblk, j);

            // mark as bad block
            {
                char    db[4096]    = {0};                                  // max page size = 4096
                char    sb[512]     = {0};                                  // max spare size = 512
                int duumy = iteNfWrite(mblk, 0, db, sb);
            }
            ret = -1;
            break;
        }
        addr += itpNfInfo.PageSize;
    }

mEnd:

    if (blkBuff)
    {
        free(blkBuff);
    }

    if (tmpbuf)
    {
        free(tmpbuf);
    }

    ITP_LOG_DBG("	repair MarkBlk:End, r=%x\n", ret);
    return ret;
}
#endif

/********************
 * public function *
 ********************/
static int
NandOpen (
    const char  * name,
    int         flags,
    int         mode,
    void        * info)
{
    int         blkSize = 0;
    int         i, tmpDev = 0;
    uint32_t    opCnt = 0;

    nf_lock_mutex();

    opCnt = itpNfInfo.openCnt;

    // ITP_LOG_DBG("NandOpen: blk=%x, page=%x\n",itpNfInfo.PageInBlk,itpNfInfo.PageSize );

    // check ITP NF device
    if (opCnt >= ITE_NF_OPEN_MAX)
    {
        ITP_LOG_ERR("ITP NAND open fail, over open!!\n");
        nf_unlock_mutex();
        return -1;
    }

    for (i = 0; i < ITE_NF_OPEN_MAX; i++)
    {
        if (!(itpNfInfo.usedDev & (0x1 << i)))
        {
            // ithEnterCritical();
            tmpDev              = i;
            itpNfInfo.usedDev   |= (0x1 << i);
            itpNfInfo.openCnt++;
            // ithExitCritical();
            break;
        }
    }

    itpNfInfo.blkGap[tmpDev] = 4;

#if defined(CFG_FS_FAT) || defined(CFG_LFS_HAVE_HCCFTL)
    if (mode == ITP_NAND_FTL_MODE)
    {
        if (!gFsMaxSec || !gFsSecSize)
        {
            mmpNandGetCapacity((unsigned long *)&gFsMaxSec, (unsigned long *)&gFsSecSize);
        }
        itpNfInfo.blkGap[tmpDev] = 0;
    }
    else
#endif
    {
        if (mode == ITP_NAND_BLK_MODE || mode == ITP_NAND_FULL_DATA_MODE)
        {
            itpNfInfo.blkGap[tmpDev] = 0;
        }
        else
        {
            itpNfInfo.blkGap[tmpDev] = 4;
        }
    }
    itpNfInfo.currMode[tmpDev]  = mode;

    itpNfInfo.CurBlk[tmpDev]    = 0;
    blkSize                     = itpNfInfo.PageInBlk * itpNfInfo.PageSize;
    if (!blkSize)
    {
        ITP_LOG_ERR("NAND Block size = 0!!\n");
        nf_unlock_mutex();
        return -1;
    }

#ifdef ENABLE_CHECK_RESERVED_AREA
    // to get reserved area BL/MAC/img position
    #if defined(CFG_FS_YAFFS2)
    itpNfInfo.blEndBlk  = itpNfInfo.NumOfBlk;
    #else
    #ifdef CFG_NET_ETHERNET_MAC_ADDR_NAND
    if (CFG_NET_ETHERNET_MAC_ADDR_POS % blkSize)
    {
        ITP_LOG_ERR("CFG_NET_ETHERNET_MAC_ADDR_POS is not at the block boundary, (%x,%x)\n", CFG_NET_ETHERNET_MAC_ADDR_POS, blkSize);
        nf_unlock_mutex();
        return -1;
    }

    itpNfInfo.blEndBlk  = CFG_NET_ETHERNET_MAC_ADDR_POS / blkSize;
    #else
    itpNfInfo.blEndBlk  = CFG_UPGRADE_IMAGE_POS / blkSize;
    #endif
    #endif
    if (CFG_UPGRADE_IMAGE_POS % blkSize)
    {
        ITP_LOG_ERR("CFG_UPGRADE_IMAGE_POS is not at the block boundary, (%x,%x)\n", CFG_UPGRADE_IMAGE_POS, blkSize);
        nf_unlock_mutex();
        return -1;
    }

    if (CFG_NAND_RESERVED_SIZE % blkSize)
    {
        ITP_LOG_ERR("CFG_NAND_RESERVED_SIZE is not at the block boundary, (%x,%x)\n", CFG_NAND_RESERVED_SIZE, blkSize);
        nf_unlock_mutex();
        return -1;
    }

    itpNfInfo.bootImgStartBlk   = CFG_UPGRADE_IMAGE_POS / blkSize;
	#ifdef CFG_ALLOW_CHANGE_RESERVED_SIZE
	itpNfInfo.bootImgEndBlk = g_ftl_reserved_size / blkSize;
	#else
    itpNfInfo.bootImgEndBlk     = CFG_NAND_RESERVED_SIZE / blkSize;
	#endif
    // ITP_LOG_DBG("reserved area, A1=0x%x, A2=0x%x, A3=0x%x(blks)\n", itpNfInfo.blEndBlk, itpNfInfo.bootImgStartBlk, itpNfInfo.bootImgEndBlk );

    #ifdef CFG_NET_ETHERNET_MAC_ADDR_NAND
    if (!itpNfInfo.blEndBlk)
    {
        ITP_LOG_INFO("MacPos=0x%x, ImgPos=0x%x, bSize=0x%x\n", CFG_NET_ETHERNET_MAC_ADDR_POS, CFG_UPGRADE_IMAGE_POS, blkSize);
    }
    #else
    if (!itpNfInfo.blEndBlk)
    {
        ITP_LOG_INFO("MacPos=0x%x, ImgPos=0x%x, bSize=0x%x\n", 0, CFG_UPGRADE_IMAGE_POS, blkSize);
    }
    #endif

    nf_unlock_mutex();
    return tmpDev;
#else
    // return fail if NAND has NO reserved size(Because it does NOT make sense)
    ITP_LOG_WARN("It's forbidden for NAND itp driver without reserved size\n");
    nf_unlock_mutex();
    return -1;
#endif
}

static int
NandClose (
    int     file,
    void    * info)
{
    // ITP_LOG_DBG("NandClose::%x, %x\n", itpNfInfo.PageInBlk, itpNfInfo.PageSize );
    nf_lock_mutex();

    /* clear these parameters of LBA mode  */
    if (itpNfInfo.openCnt)
    {
        itpNfInfo.CurBlk[file]      = 0;
        itpNfInfo.blkGap[file]      = 0;
        itpNfInfo.currMode[file]    = 0;
        itpNfInfo.usedDev           &= ~(0x1 << file);
        itpNfInfo.openCnt--;
    }

    // errno = ENOENT;
    // errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
    nf_unlock_mutex();
    return 0;
}

static int
NandReadFullData (
    int     file,
    char    * ptr,
    int     len,
    void    * info)
{
    int         DoneLen     = 0;
    uint32_t    blk         = 0;
    uint32_t    pg          = 0;
    uint32_t    doLen       = 0;
    uint32_t    readPageSize= 0;
    char        * databuf   = NULL;
    char        * tmpbuf;
    char        * srcbuf    = ptr;
    int         blkSize;
    uint32_t    areaBoundary;
    bool        skipBlk     = false;

    nf_lock_mutex();

    blkSize = itpNfInfo.PageInBlk * itpNfInfo.PageSize;

    // check baundary of bootloader/Mac address/boot image
    // check current block index "tpNfInfo.CurBlk" for suring BL/Mac/image area
    // if "tpNfInfo.CurBlk" or "tpNfInfo.CurBlk+len" is over the boundary, then return fail

    if (blkSize)
    {
#ifdef ENABLE_CHECK_RESERVED_AREA
        if (itpNfInfo.CurBlk[file] < itpNfInfo.blEndBlk)
        {
            areaBoundary = itpNfInfo.blEndBlk;
        }
        else if (itpNfInfo.CurBlk[file] < itpNfInfo.bootImgStartBlk)
        {
            areaBoundary = itpNfInfo.bootImgStartBlk;
        }
        else
        {
            areaBoundary = itpNfInfo.bootImgEndBlk;
        }
#else
        areaBoundary = itpNfInfo.CurBlk[file] + len;
#endif
        if ((itpNfInfo.CurBlk[file] + (uint32_t)len) > (uint32_t)areaBoundary)
        {
            ITP_LOG_ERR("itpNfRead has NO enough space for reading, curB=%x, len=%x, boundary=%x\n", itpNfInfo.CurBlk[file], len, areaBoundary);
            (void)printf("itpNfRead has NO enough space for reading, curB=%x, len=%x, boundary=%x\n", itpNfInfo.CurBlk[file], len, areaBoundary);

#ifdef ENABLE_CHECK_RESERVED_AREA
            if (areaBoundary == itpNfInfo.blEndBlk)
            {
                (void)printf("OUT of bootloader area!!\n");
            }
            else if (areaBoundary == itpNfInfo.bootImgStartBlk)
            {
                (void)printf("OUT of MAC-address area!!\n");
            }
            else if (areaBoundary == itpNfInfo.bootImgEndBlk)
            {
                (void)printf("OUT of boot-image area!!\n");
            }
            else
            {
                (void)printf("UNKNOW condition::%x,%x\n", areaBoundary, itpNfInfo.bootImgEndBlk);
            }
#endif
            goto end;
        }
    }
    else
    {
        ITP_LOG_ERR("itpNfRead Error, block size = 0!!\n");
        goto end;
    }

    if (!ptr || !len)
    {
        errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // ptr or len error
        ITP_LOG_ERR("ptr or len error: \n");
        goto end;
    }

    if (!itpNfInfo.NumOfBlk)
    {
        errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // nand initial fail
        ITP_LOG_ERR("nand block initial error: \n");
        goto end;
    }
    readPageSize = itpNfInfo.PageSize + itpNfInfo.AllSprSize;

    databuf = (char *)malloc(itpNfInfo.PageInBlk * readPageSize);
    if (databuf == NULL)
    {
        errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // out of memory:
        ITP_LOG_ERR("out of memory:\n");
        goto end;
    }

    for (blk = itpNfInfo.CurBlk[file]; blk < areaBoundary; blk++)
    {
#ifdef ENABLE_CHECK_RESERVED_AREA
        if (blk >= (uint32_t)areaBoundary)
        {
            ITP_LOG_ERR("itpNfRead impossible condition: out of boundary:blk=%d, Boundary=%d\n", blk, areaBoundary);
            (void)printf("itpNfRead impossible condition: out of boundary:blk=%d, Boundary=%d\n", blk, areaBoundary);
            // (void)printf("ERROR:: NO enough space for nand writing data, %x, %x, %x, %x\n",blk,itpNfInfo.blEndBlk,itpNfInfo.bootImgStartBlk,itpNfInfo.bootImgEndBlk);

            itpNfInfo.CurBlk[file] = blk;   // set current block index
            goto end;
        }
#endif
        // check if "blk" a bad block that read from file system
        if (iteNfIsBadBlockForBoot(blk) != true)
        {
            // true means "GOOD BLOCK"; !true means "BAD BLOCK"
            if (blk >= itpNfInfo.bootImgEndBlk)
            {
                // Read from file system, no need skip bad block     
                ITP_LOG_WARN("Read bad block 0x%X, skip\n", blk);
                skipBlk = true;                
            }
            else continue;
        }
        else
            skipBlk = false;
        
        tmpbuf = databuf;
        if(!skipBlk)
        {
            for ( pg = 0; pg < itpNfInfo.PageInBlk; pg++)
            {            
                int readRet = itpNfInfo.currMode[file] == ITP_NAND_FULL_DATA_MODE ? 
                            iteNfRead(blk, pg, tmpbuf) : iteNfReadPart(blk, pg, tmpbuf, LL_RP_DATA);

                if (readRet != true)
                {
                    ITP_LOG_ERR("itpNfRead Error: blk=%x, pg=%x\n", blk, pg);

    #ifdef  CFG_UPGRADE_MARK_POS
                    {
                        uint32_t markBlk = CFG_UPGRADE_MARK_POS / blkSize;
                        if (blk == markBlk)
                        {
                            ITP_LOG_ERR("itpNfRead MarkBlk Error: blk=%x, pg=%x\n", blk, pg);
                            iteNfSetWtProtect(0, areaBoundary, 0);
                            if (repairMarkBlkEccErr(blk, pg, tmpbuf))                           // =0 success, others: fail
                            {
                                iteNfSetWtProtect(0, areaBoundary, 1);
                                errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // read fail
                                if (databuf != NULL)
                                {
                                    free(databuf);
                                }
                                nf_unlock_mutex();
                                return (-1);
                            }
                            iteNfSetWtProtect(0, areaBoundary, 1);
                        }
                    }
    #else
                    errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // read fail
                    if (databuf != NULL)
                    {
                        free(databuf);
                    }
                    nf_unlock_mutex();
                    return (-1);
    #endif
                }
                tmpbuf += readPageSize;
            }
        }
        else
        {
            memset(tmpbuf, 0xff, itpNfInfo.PageInBlk * readPageSize);
        }

        if (blk)
        {
#ifndef FAST_ITP_NAND_READ
            memcpy((void *)srcbuf, (void *)(databuf + itpNfInfo.blkGap[file]), itpNfInfo.PageInBlk * readPageSize - itpNfInfo.blkGap[file]);
#else
            ithDmaMemcpy((uint32_t)srcbuf, (uint32_t)(databuf + itpNfInfo.blkGap[file]), itpNfInfo.PageInBlk * readPageSize - itpNfInfo.blkGap[file]);
#endif
            srcbuf += (itpNfInfo.PageInBlk * readPageSize) - itpNfInfo.blkGap[file];
        }
        else
        {
#ifndef FAST_ITP_NAND_READ
            memcpy((void *)srcbuf, (void *)databuf, itpNfInfo.PageInBlk * readPageSize);
#else
            ithDmaMemcpy((uint32_t)srcbuf, (uint32_t)databuf, itpNfInfo.PageInBlk * readPageSize);
#endif
            srcbuf += (itpNfInfo.PageInBlk * readPageSize);
        }

        DoneLen++;
        if (DoneLen > len)
        {
            ITP_LOG_ERR("itpNfRead impossible condition: DoneLen:%d, len:%d\n", DoneLen, len);
            itpNfInfo.CurBlk[file] = blk + 1;
            goto end;
        }

        if (DoneLen == len)
        {
            break;
        }
    }

    // (void)printf("itpNfRd:cBlk=%x, b=%x\n",itpNfInfo.CurBlk[file],blk);
    itpNfInfo.CurBlk[file] = blk + 1;

end:
    if (databuf)
    {
        free(databuf);
    }
    nf_unlock_mutex();

    return DoneLen;
}

static int
NandRead (
    int     file,
    char    * ptr,
    int     len,
    void    * info)
{
    int         DoneLen     = 0;
    uint32_t    blk         = 0;
    uint32_t    pg          = 0;
    uint32_t    doLen       = 0;
    char        * databuf   = NULL;
    char        * tmpbuf;
    char        * srcbuf    = ptr;
    int         blkSize;
    uint32_t    areaBoundary;

    if (itpNfInfo.currMode[file] == ITP_NAND_FULL_DATA_MODE)
    {
        return NandReadFullData(file, ptr, len, info);
    }

    nf_lock_mutex();

    // (void)printf("itpNfRd::f=%x, buf=%x, l=%x, b=%x, m=%x!!\n", file, ptr, len, itpNfInfo.CurBlk[file], itpNfInfo.currMode[file]);

    blkSize = itpNfInfo.PageInBlk * itpNfInfo.PageSize;

#if defined(CFG_FS_FAT) || defined(CFG_LFS_HAVE_HCCFTL)
    if (itpNfInfo.currMode[file] == ITP_NAND_FTL_MODE)
    {
        unsigned long sector, sCnt;

        sector  = itpNfInfo.CurBlk[file] * (blkSize / gFsSecSize);
        sCnt    = len * (blkSize / gFsSecSize);
        // (void)printf("itpmmpRd:%x,%x,%x\n",sector,sCnt,ptr);

        if (mmpNandReadSector(sector, sCnt, ptr) == 0)
        {
            DoneLen                 = len;
            itpNfInfo.CurBlk[file]  += DoneLen;
        }
        goto end;
    }
#endif
    // check baundary of bootloader/Mac address/boot image
    // check current block index "tpNfInfo.CurBlk" for suring BL/Mac/image area
    // if "tpNfInfo.CurBlk" or "tpNfInfo.CurBlk+len" is over the boundary, then return fail

    if (blkSize)
    {
#ifdef ENABLE_CHECK_RESERVED_AREA
        if (itpNfInfo.CurBlk[file] < itpNfInfo.blEndBlk)
        {
            areaBoundary = itpNfInfo.blEndBlk;
        }
        else if (itpNfInfo.CurBlk[file] < itpNfInfo.bootImgStartBlk)
        {
            areaBoundary = itpNfInfo.bootImgStartBlk;
        }
        else
        {
            areaBoundary = itpNfInfo.bootImgEndBlk;
        }
#else
        areaBoundary = itpNfInfo.CurBlk[file] + len;
#endif
        if ((itpNfInfo.CurBlk[file] + (uint32_t)len) > (uint32_t)areaBoundary)
        {
            ITP_LOG_ERR("itpNfRead has NO enough space for reading, curB=%x, len=%x, boundary=%x\n", itpNfInfo.CurBlk[file], len, areaBoundary);
            (void)printf("itpNfRead has NO enough space for reading, curB=%x, len=%x, boundary=%x\n", itpNfInfo.CurBlk[file], len, areaBoundary);

#ifdef ENABLE_CHECK_RESERVED_AREA
            if (areaBoundary == itpNfInfo.blEndBlk)
            {
                (void)printf("OUT of bootloader area!!\n");
            }
            else if (areaBoundary == itpNfInfo.bootImgStartBlk)
            {
                (void)printf("OUT of MAC-address area!!\n");
            }
            else if (areaBoundary == itpNfInfo.bootImgEndBlk)
            {
                (void)printf("OUT of boot-image area!!\n");
            }
            else
            {
                (void)printf("UNKNOW condition::%x,%x\n", areaBoundary, itpNfInfo.bootImgEndBlk);
            }
#endif
            goto end;
        }
    }
    else
    {
        ITP_LOG_ERR("itpNfRead Error, block size = 0!!\n");
        goto end;
    }

    if (!ptr || !len)
    {
        errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // ptr or len error
        ITP_LOG_ERR("ptr or len error: \n");
        goto end;
    }

    if (!itpNfInfo.NumOfBlk)
    {
        errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // nand initial fail
        ITP_LOG_ERR("nand block initial error: \n");
        goto end;
    }

    databuf = (char *)malloc(itpNfInfo.PageInBlk * itpNfInfo.PageSize);
    if (databuf == NULL)
    {
        errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // out of memory:
        ITP_LOG_ERR("out of memory:\n");
        goto end;
    }

    for (blk = itpNfInfo.CurBlk[file]; blk < areaBoundary; blk++)
    {
#ifdef ENABLE_CHECK_RESERVED_AREA
        if (blk >= (uint32_t)areaBoundary)
        {
            ITP_LOG_ERR("itpNfRead impossible condition: out of boundary:blk=%d, Boundary=%d\n", blk, areaBoundary);
            (void)printf("itpNfRead impossible condition: out of boundary:blk=%d, Boundary=%d\n", blk, areaBoundary);
            // (void)printf("ERROR:: NO enough space for nand writing data, %x, %x, %x, %x\n",blk,itpNfInfo.blEndBlk,itpNfInfo.bootImgStartBlk,itpNfInfo.bootImgEndBlk);

            itpNfInfo.CurBlk[file] = blk;   // set current block index
            goto end;
        }
#endif
        // check if "blk" a bad block
        if (iteNfIsBadBlockForBoot(blk) != true)
        {
            // true means "GOOD BLOCK"; !true means "BAD BLOCK"
            continue;
        }

        tmpbuf = databuf;
        for ( pg = 0; pg < itpNfInfo.PageInBlk; pg++)
        {
            if (iteNfReadPart(blk, pg, tmpbuf, LL_RP_DATA) != true)
            {
                ITP_LOG_ERR("itpNfRead Error: blk=%x, pg=%x\n", blk, pg);

#ifdef  CFG_UPGRADE_MARK_POS
                {
                    uint32_t markBlk = CFG_UPGRADE_MARK_POS / blkSize;
                    if (blk == markBlk)
                    {
                        ITP_LOG_ERR("itpNfRead MarkBlk Error: blk=%x, pg=%x\n", blk, pg);
                        iteNfSetWtProtect(0, areaBoundary, 0);
                        if (repairMarkBlkEccErr(blk, pg, tmpbuf))                           // =0 success, others: fail
                        {
                            iteNfSetWtProtect(0, areaBoundary, 1);
                            errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // read fail
                            if (databuf != NULL)
                            {
                                free(databuf);
                            }
                            nf_unlock_mutex();
                            return (-1);
                        }
                        iteNfSetWtProtect(0, areaBoundary, 1);
                    }
                }
#else
                errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // read fail
                if (databuf != NULL)
                {
                    free(databuf);
                }
                nf_unlock_mutex();
                return (-1);
#endif
            }
            tmpbuf += itpNfInfo.PageSize;
        }

        if (blk)
        {
#ifndef FAST_ITP_NAND_READ
            memcpy((void *)srcbuf, (void *)(databuf + itpNfInfo.blkGap[file]), itpNfInfo.PageInBlk * itpNfInfo.PageSize - itpNfInfo.blkGap[file]);
#else
            ithDmaMemcpy((uint32_t)srcbuf, (uint32_t)(databuf + itpNfInfo.blkGap[file]), itpNfInfo.PageInBlk * itpNfInfo.PageSize - itpNfInfo.blkGap[file]);
#endif
            srcbuf += (itpNfInfo.PageInBlk * itpNfInfo.PageSize) - itpNfInfo.blkGap[file];
        }
        else
        {
#ifndef FAST_ITP_NAND_READ
            memcpy((void *)srcbuf, (void *)databuf, itpNfInfo.PageInBlk * itpNfInfo.PageSize);
#else
            ithDmaMemcpy((uint32_t)srcbuf, (uint32_t)databuf, itpNfInfo.PageInBlk * itpNfInfo.PageSize);
#endif
            srcbuf += (itpNfInfo.PageInBlk * itpNfInfo.PageSize);
        }

        DoneLen++;
        if (DoneLen > len)
        {
            ITP_LOG_ERR("itpNfRead impossible condition: DoneLen:%d, len:%d\n", DoneLen, len);
            itpNfInfo.CurBlk[file] = blk + 1;
            goto end;
        }

        if (DoneLen == len)
        {
            break;
        }
    }

    // (void)printf("itpNfRd:cBlk=%x, b=%x\n",itpNfInfo.CurBlk[file],blk);
    itpNfInfo.CurBlk[file] = blk + 1;

end:
    if (databuf)
    {
        free(databuf);
    }
    nf_unlock_mutex();

    return DoneLen;
}

static int
NandWriteFullData (
    int     file,
    char    * ptr,
    int     len,
    void    * info)
{
    int         DoneLen = 0;
    char        * DataBuf = NULL;
    char        * tmpDataBuf;
    char        * tmpSrcBuf = ptr;
    char        * SprBuf = NULL;
    uint32_t    blk, pg;
    int         blkSize;
    uint32_t    areaBoundary = 0;
    uint32_t    writePageSize= 0;
    bool        skipBlk      = false;   // skip this block if it broken, or it will overlap next partition. However, data loss is inevitable.
    bool        isFS         = false;   // target block is file system

    nf_lock_mutex();

    // (void)printf("itpNfWt::f=%x, buf=%x, b=%x, l=%x, m=%x!!\n", file, ptr, itpNfInfo.CurBlk[file], len, itpNfInfo.currMode[file]);

    if (!ptr || !len)
    {
        errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // ptr or len error
        ITP_LOG_ERR("ptr or len error: \n");
        goto end;
    }

    if (!itpNfInfo.NumOfBlk)
    {
        errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // nand initial fail
        ITP_LOG_ERR("nand block initial error: \n");
        goto end;
    }

    blkSize = itpNfInfo.PageInBlk * itpNfInfo.PageSize;

#ifdef ENABLE_CHECK_RESERVED_AREA
    // check baundary of bootloader/Mac address/boot image
    // check current block index "tpNfInfo.CurBlk" for suring area of BL/Mac/image
    // if "tpNfInfo.CurBlk" is over the boundary, then return fail
    if (itpNfInfo.CurBlk[file] < itpNfInfo.blEndBlk)
    {
        areaBoundary = itpNfInfo.blEndBlk;
    }
    else if (itpNfInfo.CurBlk[file] < itpNfInfo.bootImgStartBlk)
    {
        areaBoundary = itpNfInfo.bootImgStartBlk;
    }
    else
    {
        areaBoundary = itpNfInfo.bootImgEndBlk;
    }
#else
    if (blkSize)
    {
        areaBoundary = CFG_NAND_RESERVED_SIZE / blkSize;        // (unit:block)
    }
    else
    {
        (void)printf("NfErr::block size = 0!!\n");
        ITP_LOG_ERR("block size = 0:\n");
        goto end;
    }
#endif

    if ((itpNfInfo.CurBlk[file] + (uint32_t)len) > areaBoundary)
    {
        ITP_LOG_ERR("write over the boundary, curB=%x, len=%x, boundary=%x\n", itpNfInfo.CurBlk[file], len, areaBoundary);
        (void)printf("write over the boundary, curB=%x, len=%x, boundary=%x\n", itpNfInfo.CurBlk[file], len, areaBoundary);
        goto end;
    }
    isFS = itpNfInfo.CurBlk[file] >= itpNfInfo.bootImgEndBlk;
    writePageSize = itpNfInfo.PageSize + itpNfInfo.AllSprSize;

    DataBuf = (char *)malloc(itpNfInfo.PageInBlk * writePageSize);
    if (DataBuf == NULL)
    {
        errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // out of memory:
        ITP_LOG_ERR("out of memory:\n");
        goto end;
    }

    if (!itpNfInfo.SprSize)
    {
        errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // nand initial fail
        ITP_LOG_ERR("nand spare initial error: \n");
        goto end;
    }

    SprBuf = (char *)malloc(itpNfInfo.SprSize);
    if (SprBuf == NULL)
    {
        errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // out of memory:
        ITP_LOG_ERR("out of memory:\n");
        goto end;
    }

    memset((uint8_t *)SprBuf, 0xFF, itpNfInfo.SprSize);

    if (itpNfInfo.currMode[file] != ITP_NAND_FTL_MODE)
    {
        iteNfSetWtProtect(0, areaBoundary, 0);
    }

    for (blk = itpNfInfo.CurBlk[file]; blk < areaBoundary; blk++)
    {
        int allNfPgWtHasPass;

        if (blk >= areaBoundary)
        {
            ITP_LOG_ERR("itpNfWrite out of boundary: blk:%d, Boundary:%d\n", blk, areaBoundary);
            (void)printf("ERROR:: NO enough space for nand writing data, %x, %x, %x, %x\n", blk, itpNfInfo.blEndBlk, itpNfInfo.bootImgStartBlk, itpNfInfo.bootImgEndBlk);
#ifdef ENABLE_CHECK_RESERVED_AREA
            if (areaBoundary == itpNfInfo.blEndBlk)
            {
                (void)printf("OUT of bootloader area!!\n");
            }
            else if (areaBoundary == itpNfInfo.bootImgStartBlk)
            {
                (void)printf("OUT of MAC-address area!!\n");
            }
            else if (areaBoundary == itpNfInfo.bootImgEndBlk)
            {
                (void)printf("OUT of boot-image area!!\n");
            }
            else
            {
                (void)printf("UNKNOW condition::%x,%x\n", areaBoundary, itpNfInfo.bootImgEndBlk);
            }
#endif
            itpNfInfo.CurBlk[file] = blk; // set current block index
            goto end;
        }

        // check if "blk" a bad block
        if (iteNfIsBadBlockForBoot(blk) != true)
        {
            // true means "GOOD BLOCK"; !true means "BAD BLOCK"
            if(isFS) skipBlk = true;            
            else continue;
        }
        else skipBlk = false;

        // erase current block
        if (!skipBlk && iteNfErase(blk) != true)
        {
            errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // erase fail
            ITP_LOG_ERR("erase block fail: %d\n", blk);

            (void)printf("NfErsErr:: need to mark bad block(blk=0x%x)\n", blk);
            // mark as bad block
            {
                char    db[4096]    = {0};  // max page size = 4096
                char    sb[512]     = {0};  // max spare size = 512
                int dummy = iteNfWrite(blk, 0, db, sb);
            }
            if(!isFS)
                continue;
            else skipBlk = true;
        }
        
        allNfPgWtHasPass    = 1;
        if(!skipBlk)
        {
            if (blk)
            {
                memcpy((void *)DataBuf, (void *)GoodBlkCodeMark, itpNfInfo.blkGap[file]);
                memcpy((void *)(DataBuf + itpNfInfo.blkGap[file]), (void *)tmpSrcBuf, itpNfInfo.PageInBlk * itpNfInfo.PageSize - itpNfInfo.blkGap[file]);
            }
            else
            {
                memcpy((void *)DataBuf, (void *)tmpSrcBuf, itpNfInfo.PageInBlk * itpNfInfo.PageSize);
            }
            tmpDataBuf          = DataBuf;

            for ( pg = 0; pg < itpNfInfo.PageInBlk; pg++)
            {
                bool allFF = true;
                // SetSprBuff(blk, pg, SprBuf);
                if(itpNfInfo.currMode[file] == ITP_NAND_FULL_DATA_MODE)
                {
                    memcpy(SprBuf, tmpDataBuf + itpNfInfo.PageSize, itpNfInfo.SprSize);
                    int i;
                    
                    for(i = 0; i < itpNfInfo.SprSize && allFF; i++)
                    {                    
                        if((unsigned char)SprBuf[i] != 0xff)
                        {
                            allFF = false;
                        }   
                    }           
                }
                else allFF = false;
                if (!allFF && iteNfWrite(blk, pg, tmpDataBuf, SprBuf) != true)
                {
                    errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // erase fail
                    ITP_LOG_ERR("write fail:[%d, %d]\n", blk, pg);

                    // mark as bad block
                    {
                        char    db[4096]    = {0};                                  // max page size = 4096
                        char    sb[512]     = {0};                                  // max spare size = 512
                        int dummy = iteNfWrite(blk, 0, db, sb);
                    }
                    tmpDataBuf          = DataBuf;
                    ITP_LOG_ERR("buffer address return to 0x%p:(ori=0x%p)\n", tmpDataBuf, (tmpDataBuf - (itpNfInfo.PageSize * pg)));
                    allNfPgWtHasPass    = 0;
                    break;
                }
                tmpDataBuf += writePageSize;
            }
        }
        if (allNfPgWtHasPass || isFS)
        {
            DoneLen++;
        }
        else
        {
            continue;
        }

        if (DoneLen > len)
        {
            ITP_LOG_ERR("itpNfWrite impossible condition: DoneLen:%d, len:%d\n", DoneLen, len);
            itpNfInfo.CurBlk[file] = blk + 1;
            goto end;
        }

        if (DoneLen == len)
        {
            break;
        }

        if (blk)
        {
            tmpSrcBuf += itpNfInfo.PageInBlk * writePageSize - itpNfInfo.blkGap[file];
        }
        else
        {
            tmpSrcBuf += itpNfInfo.PageInBlk * writePageSize;
        }
    }

    (void)printf("itpNfWt:cBlk=%x, b=%x, new-cb=%x\n", itpNfInfo.CurBlk[file], blk, blk + 1);
    itpNfInfo.CurBlk[file] = blk + 1;

end:
    if (itpNfInfo.currMode[file] != ITP_NAND_FTL_MODE)
    {
        iteNfSetWtProtect(0, areaBoundary, 1);
    }

    if (SprBuf)
    {
        free(SprBuf);
    }
    if (DataBuf)
    {
        free(DataBuf);
    }

    nf_unlock_mutex();

    return DoneLen;
}
static int
NandWrite (
    int     file,
    char    * ptr,
    int     len,
    void    * info)
{
    int         DoneLen = 0;
    char        * DataBuf = NULL;
    char        * tmpDataBuf;
    char        * tmpSrcBuf = ptr;
    char        * SprBuf = NULL;
    uint32_t    blk, pg;
    int         blkSize;
    uint32_t    areaBoundary = 0;
    uint32_t    writePageSize= 0;
    if(itpNfInfo.currMode[file] == ITP_NAND_FULL_DATA_MODE)
    {
        return NandWriteFullData(file, ptr, len, info);
    }

    nf_lock_mutex();

    // (void)printf("itpNfWt::f=%x, buf=%x, b=%x, l=%x, m=%x!!\n", file, ptr, itpNfInfo.CurBlk[file], len, itpNfInfo.currMode[file]);

    if (!ptr || !len)
    {
        errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // ptr or len error
        ITP_LOG_ERR("ptr or len error: \n");
        goto end;
    }

    if (!itpNfInfo.NumOfBlk)
    {
        errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // nand initial fail
        ITP_LOG_ERR("nand block initial error: \n");
        goto end;
    }

    blkSize = itpNfInfo.PageInBlk * itpNfInfo.PageSize;

    if (blkSize)
    {
        areaBoundary = CFG_NAND_RESERVED_SIZE / blkSize;        // (unit:block)
    }
    else
    {
        (void)printf("NfErr::block size = 0!!\n");
        ITP_LOG_ERR("block size = 0:\n");
        goto end;
    }

#if defined(CFG_FS_FAT) || defined(CFG_LFS_HAVE_HCCFTL)
    if (itpNfInfo.currMode[file] == ITP_NAND_FTL_MODE)
    {
        unsigned long sector, sCnt;

        sector  = itpNfInfo.CurBlk[file] * (blkSize / gFsSecSize);
        sCnt    = len * (blkSize / gFsSecSize);
        // (void)printf("itpmmpWt:%x,%x,%x\n",sector,sCnt,ptr);
        if (mmpNandWriteSector(sector, sCnt, ptr) == 0)
        {
            DoneLen                 = len;
            itpNfInfo.CurBlk[file]  += len;
        }
        goto end;
    }
#endif

#ifdef ENABLE_CHECK_RESERVED_AREA
    // check baundary of bootloader/Mac address/boot image
    // check current block index "tpNfInfo.CurBlk" for suring area of BL/Mac/image
    // if "tpNfInfo.CurBlk" is over the boundary, then return fail
    if (itpNfInfo.CurBlk[file] < itpNfInfo.blEndBlk)
    {
        areaBoundary = itpNfInfo.blEndBlk;
    }
    else if (itpNfInfo.CurBlk[file] < itpNfInfo.bootImgStartBlk)
    {
        areaBoundary = itpNfInfo.bootImgStartBlk;
    }
    else
    {
        areaBoundary = itpNfInfo.bootImgEndBlk;
    }
#endif

    if ((itpNfInfo.CurBlk[file] + (uint32_t)len) > areaBoundary)
    {
        ITP_LOG_ERR("write over the boundary, curB=%x, len=%x, boundary=%x\n", itpNfInfo.CurBlk[file], len, areaBoundary);
        (void)printf("write over the boundary, curB=%x, len=%x, boundary=%x\n", itpNfInfo.CurBlk[file], len, areaBoundary);
        goto end;
    }

    DataBuf = (char *)malloc(itpNfInfo.PageInBlk * itpNfInfo.PageSize);
    if (DataBuf == NULL)
    {
        errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // out of memory:
        ITP_LOG_ERR("out of memory:\n");
        goto end;
    }

    if (!itpNfInfo.SprSize)
    {
        errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // nand initial fail
        ITP_LOG_ERR("nand spare initial error: \n");
        goto end;
    }

    SprBuf = (char *)malloc(itpNfInfo.SprSize);
    if (SprBuf == NULL)
    {
        errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // out of memory:
        ITP_LOG_ERR("out of memory:\n");
        goto end;
    }

    memset((uint8_t *)SprBuf, 0xFF, itpNfInfo.SprSize);

    if (itpNfInfo.currMode[file] != ITP_NAND_FTL_MODE)
    {
        iteNfSetWtProtect(0, areaBoundary, 0);
    }

    for (blk = itpNfInfo.CurBlk[file]; blk < areaBoundary; blk++)
    {
        int allNfPgWtHasPass;

        if (blk >= areaBoundary)
        {
            ITP_LOG_ERR("itpNfWrite out of boundary: blk:%d, Boundary:%d\n", blk, areaBoundary);
            (void)printf("ERROR:: NO enough space for nand writing data, %x, %x, %x, %x\n", blk, itpNfInfo.blEndBlk, itpNfInfo.bootImgStartBlk, itpNfInfo.bootImgEndBlk);
#ifdef ENABLE_CHECK_RESERVED_AREA
            if (areaBoundary == itpNfInfo.blEndBlk)
            {
                (void)printf("OUT of bootloader area!!\n");
            }
            else if (areaBoundary == itpNfInfo.bootImgStartBlk)
            {
                (void)printf("OUT of MAC-address area!!\n");
            }
            else if (areaBoundary == itpNfInfo.bootImgEndBlk)
            {
                (void)printf("OUT of boot-image area!!\n");
            }
            else
            {
                (void)printf("UNKNOW condition::%x,%x\n", areaBoundary, itpNfInfo.bootImgEndBlk);
            }
#endif
            itpNfInfo.CurBlk[file] = blk; // set current block index
            goto end;
        }

        // check if "blk" a bad block
        if (iteNfIsBadBlockForBoot(blk) != true)
        {
            // true means "GOOD BLOCK"; !true means "BAD BLOCK"
            continue;
        }

        // erase current block
        if (iteNfErase(blk) != true)
        {
            errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // erase fail
            ITP_LOG_ERR("erase block fail: %d\n", blk);

#if 1
            (void)printf("NfErsErr:: need to mark bad block(blk=0x%x)\n", blk);
            // mark as bad block
            {
                char    db[4096]    = {0};  // max page size = 4096
                char    sb[512]     = {0};  // max spare size = 512
                int dummy = iteNfWrite(blk, 0, db, sb);
            }
            continue;
#else
            // DoneLen = 0;
            itpNfInfo.CurBlk[file] = blk;
            goto end;
#endif
        }

        if (blk)
        {
            memcpy((void *)DataBuf, (void *)GoodBlkCodeMark, itpNfInfo.blkGap[file]);
            memcpy((void *)(DataBuf + itpNfInfo.blkGap[file]), (void *)tmpSrcBuf, itpNfInfo.PageInBlk * itpNfInfo.PageSize - itpNfInfo.blkGap[file]);
        }
        else
        {
            memcpy((void *)DataBuf, (void *)tmpSrcBuf, itpNfInfo.PageInBlk * itpNfInfo.PageSize);
        }
        tmpDataBuf          = DataBuf;

        allNfPgWtHasPass    = 1;
        for ( pg = 0; pg < itpNfInfo.PageInBlk; pg++)
        {
            // SetSprBuff(blk, pg, SprBuf);
            if (iteNfWrite(blk, pg, tmpDataBuf, SprBuf) != true)
            {
                errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;   // erase fail
                ITP_LOG_ERR("write fail:[%d, %d]\n", blk, pg);

                // mark as bad block
                {
                    char    db[4096]    = {0};                                  // max page size = 4096
                    char    sb[512]     = {0};                                  // max spare size = 512
                    int dummy = iteNfWrite(blk, 0, db, sb);
                }
                tmpDataBuf          = DataBuf;
                ITP_LOG_ERR("buffer address return to 0x%p:(ori=0x%p)\n", tmpDataBuf, (tmpDataBuf - (itpNfInfo.PageSize * pg)));
                allNfPgWtHasPass    = 0;
                break;
                // DoneLen = 0;
                // itpNfInfo.CurBlk[file] = blk;
                // goto end;
            }
            tmpDataBuf += itpNfInfo.PageSize;
        }
        if (allNfPgWtHasPass)
        {
            DoneLen++;
        }
        else
        {
            continue;
        }

        if (DoneLen > len)
        {
            ITP_LOG_ERR("itpNfWrite impossible condition: DoneLen:%d, len:%d\n", DoneLen, len);
            itpNfInfo.CurBlk[file] = blk + 1;
            goto end;
        }

        if (DoneLen == len)
        {
            break;
        }

        if (blk)
        {
            tmpSrcBuf += itpNfInfo.PageInBlk * itpNfInfo.PageSize - itpNfInfo.blkGap[file];
        }
        else
        {
            tmpSrcBuf += itpNfInfo.PageInBlk * itpNfInfo.PageSize;
        }
    }
    // itpNfInfo.CurBlk[file] += DoneLen;
    (void)printf("itpNfWt:cBlk=%x, b=%x, new-cb=%x\n", itpNfInfo.CurBlk[file], blk, blk + 1);
    itpNfInfo.CurBlk[file] = blk + 1;

end:
    if (itpNfInfo.currMode[file] != ITP_NAND_FTL_MODE)
    {
        iteNfSetWtProtect(0, areaBoundary, 1);
    }

    if (SprBuf)
    {
        free(SprBuf);
    }
    if (DataBuf)
    {
        free(DataBuf);
    }

    nf_unlock_mutex();

    return DoneLen;
}

static int
NandLseek (
    int     file,
    int     ptr,
    int     dir,
    void    * info)
{
    int DefBlk;
    int returnBlk = 0;

    nf_lock_mutex();

	#ifdef CFG_ALLOW_CHANGE_RESERVED_SIZE
	if(g_pkg_reserved_size)
	{
		DefBlk = g_pkg_reserved_size / (itpNfInfo.PageInBlk*itpNfInfo.PageSize);
	}
	else
	{
		DefBlk = g_ftl_reserved_size / (itpNfInfo.PageInBlk*itpNfInfo.PageSize);
	}
	#else
    DefBlk = CFG_NAND_RESERVED_SIZE / (itpNfInfo.PageInBlk * itpNfInfo.PageSize);
	#endif

    switch (dir)
    {
        default:
        case SEEK_SET:
            if (itpNfInfo.currMode[file] == ITP_NAND_FTL_MODE)
            {
                // int DefBlk = CFG_NAND_RESERVED_SIZE / (itpNfInfo.PageInBlk*itpNfInfo.PageSize);
                if (DefBlk > ptr)
                {
                    (void)printf("ERROR: current seek position is abnormal, ori pos=%x, rev pos=%x\n", ptr, DefBlk);
                }
                else
                {
                    itpNfInfo.CurBlk[file] = ptr - DefBlk;
                }
            }
            else
            {
                itpNfInfo.CurBlk[file] = ptr; // raw data mode(without FTL)
            }
#ifdef ENABLE_CHECK_RESERVED_AREA
            {
                if (itpNfInfo.CurBlk[file] < itpNfInfo.blEndBlk)
                {
                    ITP_LOG_INFO("itpNfSeek to bootloader area!!(%x,%x)\n", itpNfInfo.CurBlk[file], itpNfInfo.blEndBlk);
                }
                else if (itpNfInfo.CurBlk[file] < itpNfInfo.bootImgStartBlk)
                {
                    ITP_LOG_INFO("itpNfSeek to MAC address area!!(%x,%x)\n", itpNfInfo.CurBlk[file], itpNfInfo.bootImgStartBlk);
                }
                else if (itpNfInfo.CurBlk[file] < itpNfInfo.bootImgEndBlk)
                {
                    ITP_LOG_INFO("itpNfSeek to IMAGE area!!(%x,%x)\n", itpNfInfo.CurBlk[file], itpNfInfo.bootImgEndBlk);
                }
                else
                {
                    ITP_LOG_INFO("itpNfSeek to UNKNOW area::%x, %x, %x, %x\n", itpNfInfo.CurBlk[file], itpNfInfo.blEndBlk, itpNfInfo.bootImgStartBlk, itpNfInfo.bootImgEndBlk);
                }
            }
#endif
            break;

        case SEEK_CUR:
            itpNfInfo.CurBlk[file] += ptr;
            break;

        case SEEK_END:
            break;
    }

    if (itpNfInfo.currMode[file] == ITP_NAND_FTL_MODE)
    {
        returnBlk = itpNfInfo.CurBlk[file] + DefBlk;
    }
    else
    {
        returnBlk = itpNfInfo.CurBlk[file];
    }

    nf_unlock_mutex();

    return returnBlk;
}

static int
NandIoctl (
    int             file,
    unsigned long   request,
    void            * ptr,
    void            * info)
{
    int res = -1;

    nf_lock_mutex();

    switch (request)
    {
        case ITP_IOCTL_INIT:
#ifdef CFG_UPGRADE_IMAGE_POS
        {
            int pos = (int)CFG_UPGRADE_IMAGE_POS;
    #if defined(CFG_FS_FAT)
            remap_image_posistion = pos;
            (void)printf("itpNand.1: IOCTL init img_pos = %x, %x\n", remap_image_posistion, CFG_UPGRADE_IMAGE_POS);
    #endif
        }
#endif

			#ifdef CFG_ALLOW_CHANGE_RESERVED_SIZE
			itpNfInfo.BootRomSize = (uint32_t)g_ftl_reserved_size;
			#else
            itpNfInfo.BootRomSize   = (uint32_t)CFG_NAND_RESERVED_SIZE;
			#endif

#ifdef  CFG_NOR_SHARE_SPI_NAND
            itpNfInfo.enSpiCsCtrl   = 1;
#else
            itpNfInfo.enSpiCsCtrl   = 0;
#endif

#ifdef  CFG_SPI_NAND_ADDR_HAS_DUMMY_BYTE
            ITP_LOG_INFO("set AHDB=1\n");
            itpNfInfo.addrHasDummyByte = 1;
#else
            ITP_LOG_INFO("set AHDB=0\n");
            itpNfInfo.addrHasDummyByte = 0;
#endif

            // (void)printf("ITP_IOCTL_INIT[01](%x,%x)(%x,%x)\n",&itpNfInfo,itpNfInfo.NumOfBlk,itpNfInfo.BootRomSize,itpNfInfo.enSpiCsCtrl);
            if (iteNfInitialize(&itpNfInfo) == true)
            {
#ifdef  CFG_UPGRADE_FILE_FOR_NAND_PROGRAMMER
                itpNfInfo.rmpFlg = 0;
                if (1)// !itpNfInfo.Init)
                {
                    unsigned char * pBuf;
                    pBuf = (unsigned char *)malloc(itpNfInfo.PageSize + 512);
                    if (pBuf != NULL)
                    {
                        // read 0th page of 0th block for checking remap info
                        iteNfRead(0, 0, pBuf);
                        // ithPrintVram8(pBuf,128);
                        if ((pBuf[20] == 0x55) && (pBuf[21] == 0xAA))
                        {
                            itpNfInfo.rmpFlg = 1;
                        }
                        (void)printf("itpNand: rmpFlag = %x, (%x, %x)\n", itpNfInfo.rmpFlg, pBuf[20], pBuf[21]);
                        free(pBuf);
                    }
                }
#endif

                {
                    int i = 0;

                    /* initial itpNfInfo */
                    itpNfInfo.openCnt   = 0;
                    itpNfInfo.usedDev   = 0;
                    for (i = 0; i < ITE_NF_OPEN_MAX; i++)
                    {
                        itpNfInfo.CurBlk[i] = 0;
                    }
                    for (i = 0; i < ITE_NF_OPEN_MAX; i++)
                    {
                        itpNfInfo.blkGap[i] = 0;
                    }
                    for (i = 0; i < ITE_NF_OPEN_MAX; i++)
                    {
                        itpNfInfo.currMode[i] = 0;
                    }
                }

                // (void)printf("ITP_IOCTL_INIT[02](%x,%x)(%x), init=%x\n",&itpNfInfo, itpNfInfo.NumOfBlk, itpNfInfo.BootRomSize,itpNfInfo.Init);
                res = 0;
            }
#ifdef  CFG_NOR_SHARE_SPI_NAND
            mmpSpiSetControlMode(SPI_CONTROL_NOR);
            mmpSpiResetControl();
#endif
            break;

        case ITP_IOCTL_GET_BLOCK_SIZE:
            if (itpNfInfo.NumOfBlk)
            {
                *(unsigned long *)ptr   = (itpNfInfo.PageInBlk) * (itpNfInfo.PageSize) - itpNfInfo.blkGap[file];
                res                     = 0;
            }
            break;

        case ITP_IOCTL_FLUSH:
        {
            uint32_t mode = (uint32_t)ptr;

            if (!itpNfInfo.Init)
            {
                (void)printf("NAND Flush fail: NAND not initial, yet\n");
                res = 0;
                break;
            }

#if defined(CFG_FS_FAT) || defined(CFG_LFS_HAVE_HCCFTL)
            if (mode == ITP_NAND_FTL_MODE)
            {
    #if !defined(CFG_HAVE_LFS)
                uint8_t * pBuf = malloc(4096 + 512);

                if (!gFtl_has_init)
                {
                    unsigned long   a;
                    unsigned long   b;

                    mmpNandGetCapacity(&a, &b);
                    (void)printf("LBA=%x, sector_size=%d\n", a, b);

                    if ((a > 0) && (b == 512))
                    {
                        gFtl_has_init   = 1;
                        gFsMaxSec       = a;
                        gFsSecSize      = b;
                    }
                }

                if (pBuf != NULL)
                {
                    if (gFtl_has_init)
                    {
                        int a   = gFsMaxSec - 1;
                        int b   = gFtl_flush_counter;

                        // read different sector for ECC issue
                        if (b > a)
                        {
                            b = 0;
                        }

                        if (mmpNandReadSector(a - b, 1, pBuf) == 0)
                        {
                            res = 0;
                        }
                        else
                        {
                            ITP_LOG_ERR("flush NAND FAIL, read nand error!!\n");
                        }

                        gFtl_flush_counter++;
                        if (gFtl_flush_counter >= a)
                        {
                            gFtl_flush_counter = 0;
                        }
                    }
                    free(pBuf);
                }
                else
                {
                    ITP_LOG_ERR("flush NAND FAIL, out of memory!!\n");
                }
    #endif
            }
            else
#endif
            {
                if (FlushBootRom() == true)
                {
                    res = 0;
                }
            }
        }
        break;

        case ITP_IOCTL_SCAN:
        {
            uint32_t * blk = (uint32_t *)ptr;
            if (iteNfIsBadBlockForBoot(*blk) == true)
            {
                // *(unsigned long*)ptr = itpNfInfo.PageInBlk * itpNfInfo.PageSize;
                res = 0;
            }
        }
        break;

        case ITP_IOCTL_GET_GAP_SIZE:
        {
            *(unsigned long *)ptr   = itpNfInfo.blkGap[file];
            res                     = 0;
        }
        break;

        case ITP_IOCTL_NAND_CHECK_MAC_AREA:
#ifdef CFG_NET_ETHERNET_MAC_ADDR_NAND
            // check MAC address area(need 1 good block)
        {
            int isNoGoodBlock = 1;
            int i;

            for (i = itpNfInfo.blEndBlk; i < itpNfInfo.bootImgStartBlk; i++)
            {
                // check bad block
                if (iteNfIsBadBlockForBoot(i) == true)
                {
                    // got one good block
                    isNoGoodBlock = 0;
                    break;
                }
            }
            if (isNoGoodBlock)
            {
                ITP_LOG_ERR("This NAND has too many bad blocks in MAC-address area\n");
                ithDelay(10000);
            }
            else
            {
                res = 0;
            }
        }
#endif
            break;

        case ITP_IOCTL_NAND_CHECK_REMAP:
#if defined(CFG_FS_FAT)
            if (itpNfInfo.rmpFlg)
            {
                res = 0;
            }

    #ifdef CFG_UPGRADE_IMAGE_POS
            remap_image_posistion = (int)CFG_UPGRADE_IMAGE_POS;
    #endif

            (void)printf("itpNand: IOCTL remap = %x, img_pos = %x\n", itpNfInfo.rmpFlg, remap_image_posistion);
#endif
            break;

        case ITP_IOCTL_SET_WRITE_PROTECT:
            /**************************************************************************
            note: wp[0]: 0 is WP disable, 1 is WP enable
                  wp[1]: the start sector of WP(sector size is 512Bytes)
                  wp[2]: the end sector of WP(sector size is 512Bytes)
                  ex: if start address of WP is 0, end address of WP is 0xD00000
                      then, set wp[0]=1, wp[1]=0, wp[2]=(0xD00000/0x200)=0x6800
            ***************************************************************************/
            /*
            {
                unsigned long *wp = (unsigned long *)ptr;
                if(mmpNandWriteProtect(wp))
                {
                    (void)printf("itpNand: IOCTL_SET_WP, FAIL: wp0=%d, wp1=%d, wp2=%d\n",wp[0],wp[1],wp[2]);
                }
                else
                {
                    (void)printf("itpNand: IOCTL_SET_WP, wp0=%d, wp1=%d, wp2=%d\n",wp[0],wp[1],wp[2]);
                    res = 0;
                }
            }
            */
            break;

        case ITP_IOCTL_OTP_READ:
			{
				uint8_t * buf = (uint8_t *)ptr;
				
				//NAND_OTP_SIZE is 512
				ITP_LOG_INFO( "itpNftpRead: buf:%p\n",buf );
				if(iteNfReadOtp(buf, (uint32_t)NAND_OTP_SIZE) == true)
				{
					res = 0;
				}
				else
				{
					ITP_LOG_ERR( "OTP READ FAIL!\n" );
				}
			}
            break;

        case ITP_IOCTL_OTP_WRITE:
			{
				uint8_t * buf = (uint8_t *)ptr;
				
				//NAND_OTP_SIZE is 512
				ITP_LOG_INFO("itpNftpWrite: buf:%p\n",buf );
				if(iteNfWriteOtp(buf, (uint32_t)NAND_OTP_SIZE) == true)
				{
					res = 0;
				}
				else
				{
					ITP_LOG_ERR("OTP WRITE FAIL!\n");
				}
			}
            break;
            
		case ITP_IOCTL_GET_SPARE_SIZE:
            {
                *(unsigned long *)ptr = itpNfInfo.PageInBlk * itpNfInfo.AllSprSize;
                res = 0;
            }
            break;
            
        default:
            errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | 1;
            break;
    }

    nf_unlock_mutex();

    return res;
}

const ITPDevice itpDeviceNand =
{
    ":nand",
    NandOpen,
    NandClose,
    NandRead,
    NandWrite,
    NandLseek,
    NandIoctl,
    NULL
};
