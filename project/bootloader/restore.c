#include <sys/ioctl.h>
#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bootloader.h"
#include "config.h"

void RestorePackage(void)
{
    uint32_t ret, pkgsize, alignsize, blockcount, blocksize, pos, gapsize;
    uint8_t *pkgbuf;
    ITCStream* stream;
    ITCArrayStream arrayStream;
    ITCBlockStream blockStream;
                
#if defined(CFG_UPGRADE_IMAGE_NAND)
    const char* devname = ":nand";
    int fd = open(devname, O_RDONLY, 0);
    LOG_DBG "nand fd: 0x%X\n", fd LOG_END
#elif defined(CFG_UPGRADE_IMAGE_NOR)
    const char* devname = ":nor";
    int fd = open(devname, O_RDONLY, 0);
    LOG_DBG "nor fd: 0x%X\n", fd LOG_END
#elif defined(CFG_UPGRADE_IMAGE_SD0)
    const char* devname = ":sd0";
    int fd = open(devname, O_RDONLY, 0);
    LOG_DBG "sd0 fd: 0x%X\n", fd LOG_END
#elif defined(CFG_UPGRADE_IMAGE_SD1)
    const char* devname = ":sd1";
    int fd = open(devname, O_RDONLY, 0);
    LOG_DBG "sd1 fd: 0x%X\n", fd LOG_END
#endif

    if (fd == -1)
    {
        LOG_ERR "open device error: %d\n", fd LOG_END
        return;
    }

    if (ioctl(fd, ITP_IOCTL_GET_BLOCK_SIZE, &blocksize))
    {
        LOG_ERR "get block size error\n" LOG_END
        return;
    }
    
    if (ioctl(fd, ITP_IOCTL_GET_GAP_SIZE, &gapsize))
    {
        LOG_ERR "get gap size error:\n" LOG_END
        return;
    }    
    
    // read package size
    pos = CFG_UPGRADE_BACKUP_PACKAGE_POS / (blocksize + gapsize);
    blockcount = CFG_UPGRADE_BACKUP_PACKAGE_POS % (blocksize + gapsize);
    if (blockcount > 0)
    {
        LOG_WARN "package position 0x%X and block size 0x%X are not aligned; align to 0x%X\n", CFG_UPGRADE_BACKUP_PACKAGE_POS, blocksize, (pos * blocksize) LOG_END
    }

    LOG_DBG "blocksize=%d pos=0x%X blockcount=%d\n", blocksize, pos, blockcount LOG_END

    if (lseek(fd, pos, SEEK_SET) != pos)
    {
        LOG_ERR "seek to package position %d error\n", pos LOG_END
        return;
    }

    pkgbuf = malloc(blocksize);
    if (!pkgbuf)
    {
        LOG_ERR "out of memory %d\n", blocksize LOG_END
        return;
    }

    blockcount = 1;
    ret = read(fd, pkgbuf, blockcount);
    if (ret != blockcount)
    {
        LOG_ERR "read package header error: %d != %d\n", ret, blockcount LOG_END
        return;
    }

    pkgsize = *(uint32_t*)pkgbuf;
    pkgsize = itpLetoh32(pkgsize);
    LOG_DBG "package size: %d\n", pkgsize LOG_END

    // read package
    alignsize = ITH_ALIGN_UP(pkgsize + 4, blocksize);
    pkgbuf = realloc(pkgbuf, alignsize);
    if (pkgbuf)
    {
        blockcount = alignsize / blocksize - blockcount;
    
        ret = read(fd, pkgbuf + blocksize, blockcount);
        if (ret != blockcount)
        {
            LOG_ERR "read package error: %d != %d\n", ret, blockcount LOG_END
            return;
        }
    
        itcArrayStreamOpen(&arrayStream, pkgbuf + 4, pkgsize);
        stream = &arrayStream.stream;
    }
    else
    {
        close(fd);
        
        if (itcBlockStreamOpen(&blockStream, devname, CFG_UPGRADE_BACKUP_PACKAGE_POS + 4, pkgsize, false))
        {
             LOG_ERR "Open block stream failed: dev=%s pos=%p size=%p\n", devname, CFG_UPGRADE_BACKUP_PACKAGE_POS + 4, pkgsize LOG_END
             return;
        }

        stream = &blockStream.stream;
    }

    if (ugCheckCrc(stream, NULL))
    {
        LOG_ERR "Check CRC failed.\n" LOG_END
    }
    else
    {
        ugRestoreStart();
        ret = ugUpgradePackage(stream);
    }

#if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
    LOG_INFO "Flushing NOR cache...\n" LOG_END
    ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
#endif

    if (ret)
        LOG_INFO "Upgrade failed.\n" LOG_END
    else
        LOG_INFO "Upgrade finished.\n" LOG_END

    exit(ret);    
}
