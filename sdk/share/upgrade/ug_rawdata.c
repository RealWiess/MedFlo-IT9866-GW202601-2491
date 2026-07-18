#include <sys/ioctl.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include "ite/ug.h"
#include "ug_cfg.h"

#define MAX_HEADER_SIZE     0x10000
#define HEADER_LEN_OFFSET   12

#ifdef	CFG_NAND_RESERVED_SIZE
static int gNandReservedSize = CFG_NAND_RESERVED_SIZE;
#else
static int gNandReservedSize = 0;
#endif

#ifdef	CFG_ALLOW_CHANGE_RESERVED_SIZE
extern unsigned long g_pkg_reserved_size;
#endif

#pragma pack(1)
typedef struct
{
    uint32_t position;
    uint32_t rawdata_size;
} rawdata_t;

#ifdef CFG_UPGRADE_BACKUP_FAT_MBR
int ugCheckFatMBR(ITPDisk device_type, uint32_t pos, uint32_t len, bool backup, bool rollback)
{
    int ret = 0;
    int fd_r = -1, fd_w = -1;
    uint32_t src = 0, dst = 0, blocksize = 0, gapsize = 0, mbr_w = 0, mbr_crc = 0, mbr_inuse_crc = 0, mbr_bk_crc = 0;
    uint32_t filesize = 0, bufsize = 0, remainsize = 0, pos_r = 0, pos_w = 0, alignsize = 0, pos2_r = 0, pos2_w = 0, readsize = 0, size2 = 0;
    uint8_t *buf = 0, *mbr_buf = malloc(len);

    if (pos < 0x80000)
    {
        printf("pos less than 0x80000 error\n");
        ret = -1;
        goto end;
    }

    if (backup == true)
    {
        src = pos;
        dst = pos - (len + 8); //include 8 byte inuse and backup crc
        //mbr_buf = malloc(len);
    }else if (rollback == true)
    {
        src = pos - (len + 8); //include 8 byte inuse and backup crc
        dst = pos;
        //mbr_buf = malloc(len);
    }else
    {
        src = pos - (len + 8); //include 8 byte inuse and backup crc
        dst = pos;
        //mbr_buf = malloc(len);
    }
    dst = itpLetoh32(dst);
    src = itpLetoh32(src);
    len = itpLetoh32(len);

    if (len == 0)
    {
        printf("len error\n");
        ret = -1;
        goto end;
    }

    if (device_type == ITP_DISK_NAND)
    {
#ifdef CFG_NAND_RESERVED_SIZE
        bool ftl = ((src >= CFG_NAND_RESERVED_SIZE) && (dst >= CFG_NAND_RESERVED_SIZE)) ? true : false;
        if (ftl)
        {
            ret = ioctl(UG_DEVICE, ITP_IOCTL_MOUNT, (void*)ITP_DISK_NAND);
            fd_r = open(":nand", O_RDONLY, 1);
            fd_w = open(":nand", O_RDWR, 1);
        }
        else
#endif
        {
            fd_r = open(":nand", O_RDONLY, 0);
            fd_w = open(":nand", O_RDWR, 0);
        }
    }
    else if (device_type == ITP_DISK_NOR)
    {
        fd_r = open(":nor", O_RDWR, 0);
        fd_w = open(":nor", O_RDWR, 0);
    }
    else if (device_type == ITP_DISK_SD0)
    {
        fd_r = open(":sd0", O_RDWR, 0);
        fd_w = open(":sd0", O_RDWR, 0);
    }
    else if (device_type == ITP_DISK_SD1)
    {
        fd_r = open(":sd1", O_RDWR, 0);
        fd_w = open(":sd1", O_RDWR, 0);
    }


    if (fd_r == -1)
    {
        printf("[fd_r] Open device error: %d\n", ITP_DISK_NAND);
        ret = -1;
        goto end;
    }

    if (fd_w == -1)
    {
        printf("[fd_w] Open device error: %d\n", ITP_DISK_NAND);
        ret = -1;
        goto end;
    }

    if (ioctl(fd_r, ITP_IOCTL_GET_BLOCK_SIZE, &blocksize))
    {
        ret = __LINE__;
        printf("Get block size error: %d\n", errno);
        goto end;
    }

    if (ioctl(fd_r, ITP_IOCTL_GET_GAP_SIZE, &gapsize))
    {
        ret = __LINE__;
        printf("get gap size error: %d\n", errno);
        goto end;
    }

    printf("blocksize=%d gapsize=%d\n", blocksize, gapsize);

    filesize = len;
    alignsize = ITH_ALIGN_UP(len, blocksize + gapsize);
    bufsize = CFG_UG_BUF_SIZE < alignsize ? CFG_UG_BUF_SIZE : alignsize;
    buf = malloc(bufsize);
    if (!buf)
    {
        ret = __LINE__;
        printf("Out of memory: %ld\n", bufsize);
        goto end;
    }

    pos_r = src / (blocksize + gapsize);
    pos_w = dst / (blocksize + gapsize);

    alignsize = src % (blocksize + gapsize);
    if (alignsize > 0)
    {
        printf("[fd_r] Position 0x%X and block+gap size 0x%X are not aligned; align to 0x%X\n", src, (blocksize + gapsize), (pos_r * (blocksize + gapsize)));
    }

    pos2_r = lseek(fd_r, pos_r, SEEK_SET);
    if (pos2_r != pos_r)
    {
        ret = __LINE__;
        printf("[fd_r] Cannot lseek: %d != %d\n", pos2_r, pos_r);
        goto end;
    }

    alignsize = dst % (blocksize + gapsize);
    if (alignsize > 0)
    {
        printf("[fd_w] Position 0x%X and block+gap size 0x%X are not aligned; align to 0x%X\n", dst, (blocksize + gapsize), (pos_w * (blocksize + gapsize)));
    }

    pos2_w = lseek(fd_w, pos_w, SEEK_SET);
    if (pos2_w != pos_w)
    {
        ret = __LINE__;
        printf("[fd_w] Cannot lseek: %d != %d\n", pos2_w, pos_w);
        goto end;
    }

    if (backup == true)
    {
#ifdef CFG_NAND_RESERVED_SIZE
        if (device_type == ITP_DISK_NAND)
        {
            printf("[fd_w] Backup writing from 0x%x to 0x%x with size 0x%x, ftl: %d\n", src, dst, filesize, ftl);
        }
        else
#endif
        {
            printf("[fd_w] Backup writing from 0x%x to 0x%x with size 0x%x\n", src, dst, filesize);
        }

        remainsize = filesize;

        do
        {
            readsize = (remainsize < blocksize) ? remainsize : blocksize;
            alignsize = ITH_ALIGN_UP(readsize, blocksize) / blocksize;
            size2 = read(fd_r, buf, alignsize);
            if (size2 != alignsize)
            {
                ret = __LINE__;
                UG_LOG_ERR("Cannot read file: %ld != %ld\n", size2, alignsize);
                goto end;
            }
            memcpy(&mbr_buf[mbr_w], buf, blocksize);
            mbr_w += blocksize;
            //for(int i=0; i<512; i++)
            //{
            //    printf("%02x ", buf[i]);
            //}
            //printf("\n\n");
            size2 = write(fd_w, buf, alignsize);
            if (size2 != alignsize)
            {
                ret = __LINE__;
                UG_LOG_ERR("Cannot write file: %ld != %ld\n", size2, alignsize);
                goto end;
            }

            remainsize -= readsize;

            putchar('.');
            fflush(stdout);
        }
        while (remainsize > 0);
        mbr_crc = ugCalcBufferCrc(mbr_buf, len);
        printf("==================== FAT MBR CRC =0x%x ====================\n\n", mbr_crc);
        memcpy(buf, &mbr_crc, 4);       //backup MBR CRC
        memcpy(buf + 4, &mbr_crc, 4);   //inuse MBR CRC
        size2 = write(fd_w, buf, 1);
        if (size2 != alignsize)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot write file: %ld != %ld\n", size2, alignsize);
            goto end;
        }

        putchar('\n');

#ifdef CFG_NAND_RESERVED_SIZE
        if (device_type == ITP_DISK_NAND)
        {
            if (ftl)
                ioctl(ITP_DEVICE_NAND, ITP_IOCTL_FLUSH, (void*)ITP_NAND_FTL_MODE);
        }
#endif
    }else if (rollback == true)
    {
#ifdef CFG_NAND_RESERVED_SIZE
        if (device_type == ITP_DISK_NAND)
        {
            printf("[fd_w] rollback MBR from 0x%x to 0x%x with size 0x%x, ftl: %d\n", src, dst, filesize, ftl);
        }
        else
#endif
        {
            printf("[fd_w] rollback MBR from 0x%x to 0x%x with size 0x%x\n", src, dst, filesize);
        }

        remainsize = filesize;

        do
        {
            readsize = (remainsize < blocksize) ? remainsize : blocksize;
            alignsize = ITH_ALIGN_UP(readsize, blocksize) / blocksize;
            size2 = read(fd_r, buf, alignsize);
            if (size2 != alignsize)
            {
                ret = __LINE__;
                UG_LOG_ERR("Cannot read file: %ld != %ld\n", size2, alignsize);
                goto end;
            }
            //for(int i=0; i<512; i++)
            //{
            //    printf("%02x ", buf[i]);
            //}
            //printf("\n\n");
            size2 = write(fd_w, buf, alignsize);
            if (size2 != alignsize)
            {
                ret = __LINE__;
                UG_LOG_ERR("Cannot write file: %ld != %ld\n", size2, alignsize);
                goto end;
            }

            remainsize -= readsize;
        }
        while (remainsize > 0);
        putchar('\n');

#ifdef CFG_NAND_RESERVED_SIZE
        if (device_type == ITP_DISK_NAND)
        {
            if (ftl)
                ioctl(ITP_DEVICE_NAND, ITP_IOCTL_FLUSH, (void*)ITP_NAND_FTL_MODE);
        }
#endif
    }else
    {
#ifdef CFG_NAND_RESERVED_SIZE
        if (device_type == ITP_DISK_NAND)
        {
            printf("[fd_w] Check MBR from 0x%x to 0x%x with size 0x%x, ftl: %d\n", src, dst, filesize, ftl);
        }
        else
#endif
        {
            printf("[fd_w] Check MBR from 0x%x to 0x%x with size 0x%x\n", src, dst, filesize);
        }

        //read backup Fat MBR CRC
        remainsize = filesize;
        do
        {
            readsize = (remainsize < blocksize) ? remainsize : blocksize;
            alignsize = ITH_ALIGN_UP(readsize, blocksize) / blocksize;
            size2 = read(fd_r, buf, alignsize);
            if (size2 != alignsize)
            {
                ret = __LINE__;
                UG_LOG_ERR("Cannot read file: %ld != %ld\n", size2, alignsize);
                goto end;
            }
            memcpy(&mbr_buf[mbr_w], buf, blocksize);
            mbr_w += blocksize;
            //for(int i=0; i<512; i++)
            //{
            //    printf("%02x ", buf[i]);
            //}
            //printf("\n\n");
            remainsize -= readsize;
        }
        while (remainsize > 0);
        printf("\n");
        mbr_crc = ugCalcBufferCrc(mbr_buf, len);
        size2 = read(fd_r, buf, 1);
        mbr_bk_crc = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
        mbr_inuse_crc = (buf[7] << 24) | (buf[6] << 16) | (buf[5] << 8) | buf[4];
        printf("==================== BACKUP FAT MBR CRC =0x%x ====================\n\n", mbr_bk_crc);
        if (mbr_crc != mbr_bk_crc)
        {
            printf("======FAT Backup MBR CRC not correct(0x%x != 0x%x)======\n\n", mbr_crc, mbr_bk_crc);
            ret |= MBR_BACKUP_CRC_ERR;
        }

        //read Fat MBR to check CRC correct ot not
        mbr_w = 0;
        memset(mbr_buf, 0, len);
        remainsize = filesize;
        do
        {
            readsize = (remainsize < blocksize) ? remainsize : blocksize;
            alignsize = ITH_ALIGN_UP(readsize, blocksize) / blocksize;
            size2 = read(fd_w, buf, alignsize);
            if (size2 != alignsize)
            {
                ret = __LINE__;
                UG_LOG_ERR("Cannot read file: %ld != %ld\n", size2, alignsize);
                goto end;
            }
            memcpy(&mbr_buf[mbr_w], buf, blocksize);
            mbr_w += blocksize;
            //for(int i=0; i<512; i++)
            //{
            //    printf("%02x ", buf[i]);
            //}
            //printf("\n\n");

            remainsize -= readsize;
        }
        while (remainsize > 0);

        mbr_crc = ugCalcBufferCrc(mbr_buf, len);
        printf("==================== inuse FAT MBR CRC =0x%x ====================\n\n", mbr_crc);
        if (mbr_crc != mbr_inuse_crc)
        {
            printf("======FAT inuse MBR CRC not correct(0x%x != 0x%x)======\n\n", mbr_crc, mbr_inuse_crc);
            ret |= MBR_INUSE_CRC_ERR;
        }
    }

end:
    if (fd_r != -1)
        close(fd_r);

    if (fd_w != -1)
        close(fd_w);

    if (buf)
        free(buf);

    if (mbr_buf)
        free(mbr_buf);

    return ret;
}
#endif
#ifndef CFG_UPGRADE_BACKUP_RAW_DATA
int ugUpgradeRawData(ITCStream *f, ITPDisk disk, bool restore)
#else
int ugCopyRawData(uint32_t dst, uint32_t src, uint32_t len, ITPDisk srcDisk, ITPDisk dstDisk)
{
    int ret = 0;
    int fd_r = -1, fd_w = -1;
    uint32_t blocksize, blocksize_r = 0, gapsize_r = 0, sprsize_r = 0;
    uint32_t blocksize_w = 0, gapsize_w = 0, sprsize_w = 0;
    uint32_t filesize = 0, bufsize = 0, remainsize = 0, pos_r = 0, pos_w = 0, alignsize = 0, pos2_r = 0, pos2_w = 0, readsize = 0, size2 = 0;
    uint8_t *buf = 0;
    mode_t mode = 0;
    char srcDiskName[8] = {0}, dstDiskName[8] = {0};
    int flags_r = 0;
#ifdef CFG_NAND_RESERVED_SIZE
    bool ftl = ((src >= CFG_NAND_RESERVED_SIZE) && (dst >= CFG_NAND_RESERVED_SIZE)) ? true : false;
#endif

    dst = itpLetoh32(dst);
    src = itpLetoh32(src);
    len = itpLetoh32(len);

    if (len == 0)
    {
        printf("len error\n");
        ret = -1;
        goto end;
    }

#if defined(CFG_NAND_RESERVED_SIZE)
#if defined(CFG_FS_YAFFS2)
    // YAFFS2 copy needs to include the data in spare, src/dst outside the reserved area is YAFFS2, otherwise is FW.
    mode = ((src >= CFG_NAND_RESERVED_SIZE) || (dst >= CFG_NAND_RESERVED_SIZE)) ? ITP_NAND_FULL_DATA_MODE : ITP_NAND_RAW_MODE;
#else
    if (ftl)
    {
        int ftlRet = ioctl(UG_DEVICE, ITP_IOCTL_MOUNT, (void*)ITP_DISK_NAND);
        if(ftlRet)
        {
            // when nand have no partition, it will return error, but it's normal when first upgrade.
            UG_LOG_WARN("mount nand fat fail: %d\n", ftlRet);
        }
	    mode = 1;
    }
    else
    {
        mode = 0;
    }
#endif    
#endif

    switch(srcDisk)
    {
    case ITP_DISK_NOR:
        strcpy(srcDiskName, ":nor");
        mode = 0;
        flags_r = O_RDWR;
        break;
    case ITP_DISK_NAND:
        strcpy(srcDiskName, ":nand");
        flags_r = O_RDONLY;
        break;
    case ITP_DISK_SD0:
        strcpy(srcDiskName, ":sd0");
        mode = 0;
        flags_r = O_RDWR;
        break;
    case ITP_DISK_SD1:
        strcpy(srcDiskName, ":sd1");
        mode = 0;
        flags_r = O_RDWR;
        break;
    default:
        ret = -1;
        printf("Unknown source disk type: %d\n", srcDisk);
        goto end;
    }
    switch(dstDisk)
    {
    case ITP_DISK_NOR:
        strcpy(dstDiskName, ":nor");
        mode = 0;
        break;
    case ITP_DISK_NAND:
        strcpy(dstDiskName, ":nand");
        break;
    case ITP_DISK_SD0:
        strcpy(dstDiskName, ":sd0");
        mode = 0;
        break;
    case ITP_DISK_SD1:
        strcpy(dstDiskName, ":sd1");
        mode = 0;
        break;
    default:
        ret = -1;
        printf("Unknown destination disk type: %d\n", dstDisk);
        goto end;
    }
    fd_r = open(srcDiskName, flags_r, mode);
    fd_w = open(dstDiskName, O_RDWR, mode);

    if (fd_r == -1)
    {
        printf("[fd_r] Open device error: %d\n", ITP_DISK_NAND);
        ret = -1;
        goto end;
    }

    if (fd_w == -1)
    {
        printf("[fd_w] Open device error: %d\n", ITP_DISK_NAND);
        ret = -1;
        goto end;
    }

    if (ioctl(fd_r, ITP_IOCTL_GET_BLOCK_SIZE, &blocksize_r))
    {
        ret = __LINE__;
        printf("Get block size error: %d\n", errno);
        goto end;
    }

    if (ioctl(fd_r, ITP_IOCTL_GET_GAP_SIZE, &gapsize_r))
    {
        ret = __LINE__;
        printf("get gap size error: %d\n", errno);
        goto end;
    }

    if (ioctl(fd_w, ITP_IOCTL_GET_BLOCK_SIZE, &blocksize_w))
    {
        ret = __LINE__;
        printf("Get block size error: %d\n", errno);
        goto end;
    }
    if (ioctl(fd_w, ITP_IOCTL_GET_GAP_SIZE, &gapsize_w))
    {
        ret = __LINE__;
        printf("get gap size error: %d\n", errno);
        goto end;
    }
    printf("blocksize_r=%d gapsize_r=%d, blocksize_w=%d gapsize_w=%d\n", blocksize_r, gapsize_r, blocksize_w, gapsize_w);

    if (ioctl(fd_r, ITP_IOCTL_GET_SPARE_SIZE, &sprsize_r))
    {
        ret = __LINE__;
        printf("get gap size error: %d\n", errno);
        goto end;
    }

    if (ioctl(fd_w, ITP_IOCTL_GET_SPARE_SIZE, &sprsize_w))
    {
        ret = __LINE__;
        printf("get gap size error: %d\n", errno);
        goto end;
    }

    filesize = len;
    alignsize = (blocksize_r + gapsize_r) > (blocksize_w + gapsize_w) ? 
                ITH_ALIGN_UP(len, blocksize_r + gapsize_r) : ITH_ALIGN_UP(len, blocksize_w + gapsize_w);
    bufsize = CFG_UG_BUF_SIZE < alignsize ? CFG_UG_BUF_SIZE : alignsize;
    bufsize += sprsize_r > sprsize_w ? sprsize_r : sprsize_w;
    buf = malloc(bufsize);
    if (!buf)
    {
        ret = __LINE__;
        printf("Out of memory: %ld\n", bufsize);
        goto end;
    }

	pos_r = src / (blocksize_r + gapsize_r);
    pos_w = dst / (blocksize_w + gapsize_w);

    alignsize = src % (blocksize_r + gapsize_r);
    if (alignsize > 0)
    {
        printf("[fd_r] Position 0x%X and block+gap size 0x%X are not aligned; align to 0x%X\n", src, (blocksize_r + gapsize_r), (pos_r * (blocksize_r + gapsize_r)));
    }

    pos2_r = lseek(fd_r, pos_r, SEEK_SET);
    if (pos2_r != pos_r)
    {
        ret = __LINE__;
        printf("[fd_r] Cannot lseek: %d != %d\n", pos2_r, pos_r);
        goto end;
    }

    alignsize = dst % (blocksize_w + gapsize_w);
    if (alignsize > 0)
    {
        printf("[fd_w] Position 0x%X and block+gap size 0x%X are not aligned; align to 0x%X\n", dst, (blocksize_w + gapsize_w), (pos_w * (blocksize_w + gapsize_w)));
    }

    pos2_w = lseek(fd_w, pos_w, SEEK_SET);
    if (pos2_w != pos_w)
    {
        ret = __LINE__;
        printf("[fd_w] Cannot lseek: %d != %d\n", pos2_w, pos_w);
        goto end;
    }

#ifdef CFG_NAND_RESERVED_SIZE
    printf("[fd_w] Writing from 0x%x to 0x%x with size 0x%x, srcDisk = %d, dstDisk = %d, ftl: %d\n", src, dst, filesize, srcDisk, dstDisk, ftl);
#else
    printf("[fd_w] Writing from 0x%x to 0x%x with size 0x%x, srcDisk = %d, dstDisk = %d\n", src, dst, filesize, srcDisk, dstDisk);
#endif

    remainsize = filesize;
    blocksize = blocksize_r > blocksize_w ? blocksize_r : blocksize_w;
    do
    {
        readsize = (remainsize < blocksize) ? remainsize : blocksize;
        alignsize = ITH_ALIGN_UP(readsize, blocksize_r) / blocksize_r;
        size2 = read(fd_r, buf, alignsize);
        if (size2 != alignsize)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot read file: %ld != %ld\n", size2, alignsize);
            goto end;
        }
        alignsize = ITH_ALIGN_UP(readsize, blocksize_w) / blocksize_w;
        size2 = write(fd_w, buf, alignsize);
        if (size2 != alignsize)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot write file: %ld != %ld\n", size2, alignsize);
            goto end;
        }

        remainsize -= readsize;

        putchar('.');
        fflush(stdout);
    }
    while (remainsize > 0);
    putchar('\n');

#ifdef CFG_NAND_RESERVED_SIZE
    if (ftl)
        ioctl(ITP_DEVICE_NAND, ITP_IOCTL_FLUSH, (void*)ITP_NAND_FTL_MODE);
#endif

end:
    if (fd_r != -1)
        close(fd_r);

    if (fd_w != -1)
        close(fd_w);

    if (buf)
        free(buf);

    return ret;
}

int ugUpgradeRawData(ITCStream *f, ITPDisk disk, bool restore, bool upgrade)
#endif
{
    int ret = 0;
    int fd = -1;
    int pos2;
    rawdata_t rdata = {0};
    char* buf = NULL;
    char* buf2 = NULL;
    uint32_t filesize, bufsize, readsize, size2, remainsize, alignsize, blocksize = 0, pos, gapsize = 0;

    // read raw data header
    readsize = itcStreamRead((ITCStream*)f, &rdata, sizeof(rawdata_t));
    if (readsize != sizeof(rawdata_t))
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot read file: %ld != %ld\n", readsize, sizeof(rawdata_t));
        goto end;
    }

#if defined(CFG_A_B_DUAL_BOOT_ENABLE)
    uint8_t boot_args = 0x0U;
    int partition_offset = 0;
    if (ugBootargsRead(&boot_args))
    {
        UG_LOG_ERR("Upgrade read boot args failed.\n");
    }
    if (rdata.position == CFG_UPGRADE_IMAGE_POS)
    {
        if (boot_args == 0xAAU && ugUpgradeIsForcePartition() == false)
        {
            rdata.position = CFG_UPGRADE_SUB_IMAGE_POS;
        }
    }
#endif

    rdata.position      = itpLetoh32(rdata.position);
    rdata.rawdata_size  = itpLetoh32(rdata.rawdata_size);

#ifdef CFG_UPGRADE_BACKUP_RAW_DATA
    if (upgrade && rdata.position == CFG_UPGRADE_IMAGE_POS)
    {
    #if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
        UG_LOG_INFO("Flushing NOR cache...\n");
        ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
    #endif
    #if (CFG_LFS_CACHE_SIZE > 0)
        UG_LOG_INFO("Flushing LFS cache...\n");
        ioctl(ITP_DEVICE_LFS, ITP_IOCTL_FLUSH, NULL);
    #endif
        ret = ugUpgradeTag(UG_UPGRADE_FLOW_FW);
        if (ret)
        {
            UG_LOG_ERR("Start to upgrade FW failed: %d\n", ret);
            goto end;
        }
    }
#endif

#ifdef CFG_UPGRADE_BACKUP_PACKAGE
    uint32_t streampos = 0;

    if (rdata.rawdata_size == 0)
    {
        if (restore)
        {
            UG_LOG_INFO("[%d%%] On restore mode, skip position 0x%X.\n", ugGetProrgessPercentage(), rdata.position);
            goto end;
        }
        else
        {
            streampos = itcStreamTell((ITCStream*)f);
            itcStreamSeek((ITCStream*)f, 0, SEEK_SET);
            rdata.rawdata_size = f->size;

            UG_LOG_DBG("streampos=0x%X\n", streampos);
        }
    }
#endif // CFG_UPGRADE_BACKUP_PACKAGE

    UG_LOG_INFO("[%d%%] Write data to position 0x%X, size %ld bytes\n", ugGetProrgessPercentage(), rdata.position, rdata.rawdata_size);

    // upgrade
    if (disk == ITP_DISK_NAND)
    {
		#ifdef CFG_ALLOW_CHANGE_RESERVED_SIZE
		gNandReservedSize = g_pkg_reserved_size;
		UG_LOG_DBG( "	### vary-PKG upgrade: %x, %x\n", rdata.position, gNandReservedSize );
		#endif

    	if(rdata.position == gNandReservedSize)
    	{
    		//add force to MOUNT NAND
    		ret = ioctl(UG_DEVICE, ITP_IOCTL_MOUNT, (void*)disk);

    		if(!rdata.position)	printf("WARNING: raw data position is 0!!\n");
    		fd = open(":nand", O_RDWR, 1);
    		printf("open NAND drive with FAT+FTL OK!\n");
    	}
    	else
    	{
    		fd = open(":nand", O_RDWR, 0);
    		printf("open NAND device OK!\n");
    	}

        UG_LOG_DBG("nand fd=0x%X\n", fd);

        #ifdef CFG_NET_ETHERNET_MAC_ADDR_NAND
        if (fd != -1)
        {
            if(ioctl(fd, ITP_IOCTL_NAND_CHECK_MAC_AREA, NULL))
            {
                UG_LOG_ERR("nand MAC address has NO space\n");
                printf( "nand MAC address has NO space\n" );
                fd = -1;
            }
        }
        #endif
    }
    else if (disk == ITP_DISK_NOR)
    {
#if defined(CFG_NOR_USE_DPUAES) && (CFG_FW_DPUAES_UPGRADE_MARK_POS == 0)
        if (rdata.position == CFG_UPGRADE_IMAGE_POS)
            fd = open(":norAES", O_RDWR, 0);
        else
#endif
        fd = open(":nor", O_RDWR, 0);
        UG_LOG_DBG("nor fd=0x%X\n", fd);
    }
    else if (disk == ITP_DISK_SD0)
    {
        fd = open(":sd0", O_RDWR, 0);
        UG_LOG_DBG("sd0 fd=0x%X\n", fd);
    }
    else if (disk == ITP_DISK_SD1)
    {
        fd = open(":sd1", O_RDWR, 0);
        UG_LOG_DBG("sd1 fd=0x%X\n", fd);
    }
    else
    {
        UG_LOG_DBG("Unknown disk: %d\n", disk);
        ret = -1;
        goto end;
    }

    if (fd == -1)
    {
        UG_LOG_ERR("Open device error: %d\n", disk);
        ret = -1;
        goto end;
    }

    if (ioctl(fd, ITP_IOCTL_GET_BLOCK_SIZE, &blocksize))
    {
        ret = __LINE__;
        UG_LOG_ERR("Get block size error: %d\n", errno);
        goto end;
    }

    if (ioctl(fd, ITP_IOCTL_GET_GAP_SIZE, &gapsize))
    {
        ret = __LINE__;
        UG_LOG_ERR("get gap size error: %d\n", errno);
        goto end;
    }

    UG_LOG_DBG("blocksize=%d gapsize=%d\n", blocksize, gapsize);

    filesize = rdata.rawdata_size;
    alignsize = ITH_ALIGN_UP(filesize, blocksize + gapsize);
    bufsize = CFG_UG_BUF_SIZE < alignsize ? CFG_UG_BUF_SIZE : alignsize;
#if defined(CFG_NOR_USE_DPUAES) && (CFG_FW_DPUAES_UPGRADE_MARK_POS == 0)
    if (rdata.position == CFG_UPGRADE_IMAGE_POS)
        buf = memalign(4, bufsize);
    else
#endif
    buf = malloc(bufsize);
    if (!buf)
    {
        ret = __LINE__;
        UG_LOG_ERR("Out of memory: %ld\n", bufsize);
        goto end;
    }
    if (blocksize + gapsize == 0)
    {
        // Should not happen under normal circumstances.
        ret = __LINE__;
        UG_LOG_ERR("Unexpected zero block size: blocksize=%u, gapsize=%u\r\n",
                    blocksize, gapsize);
        goto end;
    }
    else
    {
        pos = rdata.position / (blocksize + gapsize);
        alignsize = rdata.position % (blocksize + gapsize);
    }
    if (alignsize > 0)
    {
        UG_LOG_WARN("Position 0x%X and block+gap size 0x%X are not aligned; align to 0x%X\n", rdata.position, (blocksize + gapsize), (pos * (blocksize + gapsize)));
    }

    pos2 = lseek(fd, pos, SEEK_SET);
    if (pos2 != pos)
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot lseek: %d != %d\n", pos2, pos);
        goto end;
    }

    UG_LOG_INFO("[%d%%] Writing", ugGetProrgessPercentage());

    remainsize = filesize;

#ifdef CFG_UPGRADE_BACKUP_PACKAGE
    if (streampos > 0)
    {
        alignsize = cpu_to_le32(rdata.rawdata_size);
        memcpy(buf, &alignsize, 4);

        if ( !pos2 && (disk == ITP_DISK_NAND) )
        {
        	readsize = (remainsize < blocksize+gapsize) ? remainsize : blocksize+gapsize;
        	pos2++;
        }
        else
        {
        	if(disk == ITP_DISK_NAND)	readsize = (remainsize < blocksize) ? remainsize : blocksize;
            else	readsize = (remainsize < bufsize) ? remainsize : bufsize;
        }
        readsize -= 4;

        size2 = itcStreamRead((ITCStream*)f, &buf[4], readsize);
        if (size2 != readsize)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot read file: %ld != %ld\n", size2, readsize);
            goto end;
        }

        alignsize = ITH_ALIGN_UP(readsize + 4, blocksize) / blocksize;
        size2 = write(fd, buf, alignsize);
        if (size2 != alignsize)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot write file: %ld != %ld\n", size2, alignsize);
            goto end;
        }

        remainsize -= readsize;

        putchar('.');
        fflush(stdout);
    }
#endif // CFG_UPGRADE_BACKUP_PACKAGE

    do
    {
#if defined(CFG_CHIP_PKG_IT9910)
        usleep(1000);
#endif
        if ( !pos2 && (disk == ITP_DISK_NAND) )
        {
       		readsize = (remainsize < blocksize+gapsize) ? remainsize : blocksize+gapsize;
        	pos2++;
        }
        else
        {
        	if(disk == ITP_DISK_NAND)	readsize = (remainsize < blocksize) ? remainsize : blocksize;
            else	readsize = (remainsize < bufsize) ? remainsize : bufsize;
        }
        size2 = itcStreamRead((ITCStream*)f, buf, readsize);
        if (size2 != readsize)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot read file: %ld != %ld\n", size2, readsize);
            goto end;
        }
#if defined(CFG_CHIP_PKG_IT9910)
        usleep(1000);
#endif
        alignsize = ITH_ALIGN_UP(readsize, blocksize) / blocksize;
        if((alignsize * blocksize) > readsize && readsize < bufsize)
            memset(buf + readsize, 0xff, (alignsize * blocksize) - readsize);
        size2 = write(fd, buf, alignsize);
        if (size2 != alignsize)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot write file: %ld != %ld\n", size2, alignsize);
            goto end;
        }

        remainsize -= readsize;

        putchar('.');
        fflush(stdout);
#ifdef CFG_A_B_DUAL_BOOT_ENABLE
        usleep(1000);
#endif
    }
    while (remainsize > 0);
    putchar('\n');

    // verify
    pos2 = lseek(fd, pos, SEEK_SET);
    if (pos2 != pos)
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot lseek: %d != %d\n", pos2, pos);
        goto end;
    }

#if defined(CFG_NOR_USE_DPUAES) && (CFG_FW_DPUAES_UPGRADE_MARK_POS == 0)
    if (rdata.position == CFG_UPGRADE_IMAGE_POS)
        buf2 = memalign(4, bufsize);
    else
#endif
    buf2 = malloc(bufsize);
    if (!buf2)
    {
        ret = __LINE__;
        UG_LOG_ERR("Out of memory: %ld\n", bufsize);
        goto end;
    }

    UG_LOG_INFO("[%d%%] Verifying", ugGetProrgessPercentage());

    remainsize = filesize;

#ifdef CFG_UPGRADE_BACKUP_PACKAGE
    if (streampos > 0)
    {
        if (itcStreamSeek((ITCStream*)f, 0, SEEK_SET))
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot seek file: %d\n", errno);
            goto end;
        }

        if ( !pos2 && (disk == ITP_DISK_NAND) )
        {
        	readsize = (remainsize < blocksize+gapsize) ? remainsize : blocksize+gapsize;
        	pos2++;
        }
        else
        {
        	if(disk == ITP_DISK_NAND)	readsize = (remainsize < blocksize) ? remainsize : blocksize;
            else	readsize = (remainsize < bufsize) ? remainsize : bufsize;
        }
        readsize -= 4;

        alignsize = cpu_to_le32(rdata.rawdata_size);
        memcpy(buf, &alignsize, 4);

        size2 = itcStreamRead((ITCStream*)f, &buf[4], readsize);
        if (size2 != readsize)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot read file: %ld != %ld\n", size2, readsize);
            goto end;
        }

        alignsize = ITH_ALIGN_UP(readsize, blocksize) / blocksize;

        size2 = read(fd, buf2, alignsize);
        if (size2 != alignsize)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot read: %ld != %ld\n", size2, alignsize);
            goto end;
        }

        if (memcmp(buf, buf2, readsize + 4))
        {
        #ifndef _WIN32
            ret = __LINE__;
            UG_LOG_ERR("\nVerify failed.\n");
            #if defined(CFG_NAND_ENABLE)
            UG_LOG_ERR("\nMaybe the issue that the upgrade use buffer is too small to upgrade.\n");
            #endif
            goto end;
        #endif // _WIN32
        }

        remainsize -= readsize;

        putchar('.');
        fflush(stdout);
    }
    else
#endif // CFG_UPGRADE_BACKUP_PACKAGE
    {
        if (itcStreamSeek((ITCStream*)f, -(long)filesize, SEEK_CUR))
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot seek file: %d\n", errno);
            goto end;
        }
    }

    do
    {
        if ( !pos2 && (disk == ITP_DISK_NAND) )
        {
        	readsize = (remainsize < blocksize+gapsize) ? remainsize : blocksize+gapsize;
        	pos2++;
        }
        else
        {
        	if(disk == ITP_DISK_NAND)	readsize = (remainsize < blocksize) ? remainsize : blocksize;
            else	readsize = (remainsize < bufsize) ? remainsize : bufsize;
        }
        size2 = itcStreamRead((ITCStream*)f, buf, readsize);
        if (size2 != readsize)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot read file: %ld != %ld\n", size2, readsize);
            goto end;
        }

        alignsize = ITH_ALIGN_UP(readsize, blocksize) / blocksize;

        size2 = read(fd, buf2, alignsize);
        if (size2 != alignsize)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot read: %ld != %ld\n", size2, alignsize);
            goto end;
        }

        if (memcmp(buf, buf2, readsize))
        {
        #ifndef _WIN32
            ret = __LINE__;
            UG_LOG_ERR("\nVerify failed.\n");
            #if defined(CFG_NAND_ENABLE)
            UG_LOG_ERR("\nMaybe the issue that the upgrade use buffer is too small to upgrade.\n");
            #endif
            goto end;
        #endif // _WIN32
        }

        remainsize -= readsize;

        putchar('.');
        fflush(stdout);
#ifdef CFG_A_B_DUAL_BOOT_ENABLE
        usleep(1000);
#endif
    }
    while (remainsize > 0);
    putchar('\n');

#if defined(CFG_NOR_USE_DPUAES) && (CFG_FW_DPUAES_UPGRADE_MARK_POS > 0)
    if (rdata.position == CFG_UPGRADE_IMAGE_POS)
    {
    #if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
        UG_LOG_INFO("Flushing NOR cache...\n");
        ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
    #endif
    #if (CFG_LFS_CACHE_SIZE > 0)
        UG_LOG_INFO("Flushing LFS cache...\n");
        ioctl(ITP_DEVICE_LFS, ITP_IOCTL_FLUSH, NULL);
    #endif
        ret = ugFWAESUpgradeTag(UG_FW_AES_UPGRADE_FLOW_ENCRYPT_DO);
        if (ret)
        {
            UG_LOG_ERR("Start to upgrade FW failed: %d\n", ret);
            goto end;
        }
    }
#endif

#ifdef CFG_UPGRADE_BACKUP_PACKAGE
    if (streampos != 0)
    {
        itcStreamSeek((ITCStream*)f, streampos, SEEK_SET);

#ifndef CFG_UPGRADE_BACKUP_RAW_DATA
        ret = ugUpgradeBackupFinish();
        if (ret)
            goto end;
#endif
    }
#endif // CFG_UPGRADE_BACKUP_PACKAGE

end:
    if (fd != -1)
        close(fd);

    if (buf2)
        free(buf2);

    if (buf)
        free(buf);

    return ret;
}

#ifndef CFG_UPGRADE_BACKUP_RAW_DATA
int ugUpgradeRawData_diff(ITCStream *f, ITPDisk disk, bool restore)
#else
int ugUpgradeRawData_diff(ITCStream *f, ITPDisk disk, bool restore, bool upgrade)
#endif
{
    int ret = 0, result = 0;
    int fd = -1;
    int pos, pos2;
    rawdata_t rdata = {0};
    char* buf = NULL;
    char* buf2 = NULL;
    uint32_t filesize, bufsize, readsize, size2, remainsize, alignsize, blocksize = 0, gapsize = 0, blockcount, ori_blockcount;
    uint32_t headersize, contentsize, old_romsize = 0, new_romsize = 0, diff_size = 0, new_rombufpos = 0, crc32val = 0;
    uint8_t *buf_check = NULL, *old_rombuf = NULL, *new_rombuf = NULL, *diff_buf = NULL, *ucl_buf = NULL;
    uint8_t *realloc_tmp = NULL;

    // read raw data header
    readsize = itcStreamRead((ITCStream*)f, &rdata, sizeof(rawdata_t));
    if (readsize != sizeof(rawdata_t))
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot read file header: %u\n", readsize);
        goto end;
    }

#if defined(CFG_A_B_DUAL_BOOT_ENABLE)
    uint8_t boot_args = 0x0U;
    int partition_offset = 0;
    if (ugBootargsRead(&boot_args))
    {
        UG_LOG_ERR("Upgrade read boot args failed.\n");
    }
    if (rdata.position == CFG_UPGRADE_IMAGE_POS)
    {
        if (boot_args == 0xAAU && ugUpgradeIsForcePartition() == false)
        {
            rdata.position = CFG_UPGRADE_SUB_IMAGE_POS;
        }
    }
#endif

    rdata.position      = itpLetoh32(rdata.position);
    rdata.rawdata_size  = itpLetoh32(rdata.rawdata_size);

#ifdef CFG_UPGRADE_BACKUP_RAW_DATA
    if (upgrade && rdata.position == CFG_UPGRADE_IMAGE_POS)
    {
    #if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
        UG_LOG_INFO("Flushing NOR cache...\n");
        ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
    #endif
    #if (CFG_LFS_CACHE_SIZE > 0)
        UG_LOG_INFO("Flushing LFS cache...\n");
        ioctl(ITP_DEVICE_LFS, ITP_IOCTL_FLUSH, NULL);
    #endif
        ret = ugUpgradeTag(UG_UPGRADE_FLOW_FW);
        if (ret)
        {
            UG_LOG_ERR("Start to upgrade FW failed: %d\n", ret);
            goto end;
        }
    }
#endif

    //read back original rom raw data to do diff patch
    if (disk == ITP_DISK_NAND)
    {
    	if(rdata.position == gNandReservedSize)
    	{
    		//add force to MOUNT NAND
    		ret = ioctl(UG_DEVICE, ITP_IOCTL_MOUNT, (void*)disk);

    		if(!rdata.position)	printf("WARNING: raw data position is 0!!\n");
    		fd = open(":nand", O_RDWR, 1);
    		printf("open NAND drive with FAT+FTL OK!\n");
    	}
    	else
    	{
    		fd = open(":nand", O_RDWR, 0);
    		printf("open NAND device OK!\n");
    	}

        UG_LOG_DBG("nand fd=0x%X\n", fd);

        #ifdef CFG_NET_ETHERNET_MAC_ADDR_NAND
        if (fd != -1)
        {
            if(ioctl(fd, ITP_IOCTL_NAND_CHECK_MAC_AREA, NULL))
            {
                UG_LOG_ERR("nand MAC address has NO space\n");
                printf( "nand MAC address has NO space\n" );
                fd = -1;
            }
        }
        #endif
    }
    else if (disk == ITP_DISK_NOR)
    {
#if defined(CFG_NOR_USE_DPUAES) && (CFG_FW_DPUAES_UPGRADE_MARK_POS == 0)
        if (rdata.position == CFG_UPGRADE_IMAGE_POS)
            fd = open(":norAES", O_RDWR, 0);
        else
#endif
        fd = open(":nor", O_RDWR, 0);
        UG_LOG_DBG("nor fd=0x%X\n", fd);
    }
    else if (disk == ITP_DISK_SD0)
    {
        fd = open(":sd0", O_RDWR, 0);
        UG_LOG_DBG("sd0 fd=0x%X\n", fd);
    }
    else if (disk == ITP_DISK_SD1)
    {
        fd = open(":sd1", O_RDWR, 0);
        UG_LOG_DBG("sd1 fd=0x%X\n", fd);
    }
    else
    {
        UG_LOG_DBG("Unknown disk: %d\n", disk);
        ret = -1;
        goto end;
    }

    if (fd == -1)
    {
        UG_LOG_ERR("Open device error: %d\n", disk);
        ret = -1;
        goto end;
    }

    if (ioctl(fd, ITP_IOCTL_GET_BLOCK_SIZE, &blocksize))
    {
        ret = __LINE__;
        UG_LOG_ERR("Get block size error: %d\n", errno);
        goto end;
    }

    if (ioctl(fd, ITP_IOCTL_GET_GAP_SIZE, &gapsize))
    {
        ret = __LINE__;
        UG_LOG_ERR("get gap size error: %d\n", errno);
        goto end;
    }

    UG_LOG_DBG("blocksize=%u gapsize=%u\n", blocksize, gapsize);

    // read original rom header size
    if (blocksize + gapsize == 0)
    {
        // Should not happen under normal circumstances.
        ret = __LINE__;
        UG_LOG_ERR("Unexpected zero block size: blocksize=%u, gapsize=%u\r\n",
                    blocksize, gapsize);
        goto end;
    }
    else
    {
        pos = rdata.position / (blocksize + gapsize);
        blockcount = rdata.position % (blocksize + gapsize);
    }
    if (blockcount > 0)
    {
        UG_LOG_WARN("rom position 0x%X and block size 0x%X are not aligned; align to 0x%X\n", rdata.position, blocksize, (pos * blocksize));
    }

    UG_LOG_DBG("blocksize=%u pos=0x%X align=%u\n", blocksize, pos, blockcount);

    if (lseek(fd, pos, SEEK_SET) != pos)
    {
        UG_LOG_ERR("seek to rom position %d error\n", pos);
        goto end;
    }

	if(pos == 0U)
	{
		//just for NAND, block 0 real size is (blocksize + gapsize), other block is blocksize.
		if(blocksize + gapsize > MAX_HEADER_SIZE)
			alignsize = blocksize + gapsize;
		else
			alignsize = MAX_HEADER_SIZE;
    }else
	{
        if(blocksize > MAX_HEADER_SIZE)
            alignsize = blocksize;
        else
            alignsize = MAX_HEADER_SIZE;
	}
#if defined(CFG_NOR_USE_DPUAES) && (CFG_FW_DPUAES_UPGRADE_MARK_POS == 0)
    old_rombuf = memalign(4, alignsize);
#else
    old_rombuf = malloc(alignsize);
#endif

    if (!old_rombuf)
    {
        UG_LOG_ERR("out of memory %u\n", alignsize);
        goto end;
    }

	if(pos == 0U)
	{   
		//just for NAND, block 0 real size is (blocksize + gapsize), other block is blocksize.
		blockcount = alignsize / (blocksize + gapsize);
	}else
	{
    	blockcount = alignsize / blocksize;
	}
    
    result = read(fd, old_rombuf, blockcount);
    if (result != blockcount)
    {
        ret = __LINE__;
        UG_LOG_ERR("read rom header error: %d != %u\n", result, blockcount);
        goto end;
    }

    headersize = *(uint32_t*)(old_rombuf + HEADER_LEN_OFFSET);
    headersize = itpBetoh32(headersize);
    UG_LOG_DBG("old rom header size: %u\n", headersize);

    if ((headersize %4) || headersize > 100*1024)
    {
        UG_LOG_ERR("invalid header size: %u\n", headersize);
        goto end;
    }

    buf_check = old_rombuf + headersize;

#ifndef CFG_SPEEDY_DECOMPRESS_ENABLE
	if (strncmp(buf_check, "SMAZ", 4) == 0)
	{
		// compressed rom
		contentsize = *(uint32_t*)(old_rombuf + HEADER_LEN_OFFSET + 4);
		contentsize = itpBetoh32(contentsize);
		UG_LOG_DBG("seal compress rom, content size: %d\n", contentsize);
	}
#else
	if (strncmp(buf_check, "ITE", 3) == 0)
	{
		// compressed rom
		contentsize = ((uint32_t)buf_check[13] << 24) | ((uint32_t)buf_check[12] << 16) | ((uint32_t)buf_check[11] << 8) | (uint32_t)buf_check[10];
		contentsize += 14; //add speedy header length
		UG_LOG_DBG("speedy compress rom, content size: %d\n", contentsize);
	}
#endif
    else
    {
        // non-compressed rom
        contentsize = *(uint32_t*)(old_rombuf + headersize - 4);
        contentsize = itpBetoh32(contentsize);
        UG_LOG_DBG("non-compress rom, content size: %d\n", contentsize);
    }

    // read original rom image
    ori_blockcount = blockcount;
    old_romsize = headersize + contentsize;
    blockcount = old_romsize / blocksize;
    if(old_romsize % blocksize)    blockcount++;
    old_romsize = blockcount * (blocksize + gapsize);
    UG_LOG_DBG("old rom(addr=0x%x) size: %u\n", rdata.position, old_romsize);
    ////
#if defined(CFG_NOR_USE_DPUAES) && (CFG_FW_DPUAES_UPGRADE_MARK_POS == 0)
    realloc_tmp = old_rombuf;
    old_rombuf = memalign(4, old_romsize);
    if (old_rombuf)
        memcpy(old_rombuf, realloc_tmp, alignsize);
    free(realloc_tmp);
#else
    realloc_tmp = realloc(old_rombuf, old_romsize);
#endif
    if (!realloc_tmp)
    {
        UG_LOG_ERR("out of memory %u\n", old_romsize);
        free(old_rombuf);
        old_rombuf = NULL;
        goto end;
    }
    old_rombuf = realloc_tmp;

    blockcount -= ori_blockcount;

#if defined(CFG_UPGRADE_BOOTLOADER_NAND) || defined(CFG_UPGRADE_IMAGE_NAND)
    if(pos == 0U)
    {
        //just for NAND, block 0 real size is (blocksize + gapsize), other block is blocksize.
        result = read(fd, old_rombuf + blocksize + gapsize, blockcount);
    }else
    {
        result = read(fd, old_rombuf + blocksize, blockcount);
    }
#else
    result = read(fd, old_rombuf + alignsize, blockcount);
#endif
    if (result != blockcount)
    {
        ret = __LINE__;
        UG_LOG_ERR("read rom image error: %d != %u\n", result, blockcount);
        goto end;
    }
#if 0
    FILE * xd = fopen("A:/original.bin", "wb");
    if (!xd)
    {
    	printf("file open %s fail\n", "A:/original.bin");
    }else
	{
		fwrite(old_rombuf, 1, headersize + contentsize, xd);
    	fclose(xd);
	}
#endif
    UG_LOG_DBG("read old rom raw size: %u\n", headersize + contentsize);

    ucl_buf = malloc(rdata.rawdata_size);
    filesize = itcStreamRead((ITCStream*)f, ucl_buf, rdata.rawdata_size);
    if (filesize != rdata.rawdata_size)
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot read file: %u != %u\n", filesize, rdata.rawdata_size);
        goto end;
    }
#ifndef _WIN32
    diff_buf = hw_ucl_decompress(ucl_buf, rdata.rawdata_size, &diff_size);
    if (!diff_buf)
	{
	    ret = __LINE__;
	    UG_LOG_ERR("ucl decompress error!!!\n");
	    goto end;
	}
    new_romsize = ugpatch_getsize(diff_buf);
    new_rombuf = ugpatch_getbuf(old_rombuf, headersize + contentsize, diff_buf);
    if (!new_rombuf)
	{
	    ret = __LINE__;
	    UG_LOG_ERR("diff patch error!!!\n");
	    goto end;
	}
#endif
    crc32val = (diff_buf[diff_size - 1] << 24) | (diff_buf[diff_size - 2] << 16) | (diff_buf[diff_size - 3] << 8) | diff_buf[diff_size - 4];
    printf("check CRC---0x%x(0x%x)\n", crc32val, ugCalcBufferCrc(new_rombuf, new_romsize));
    if(ugCalcBufferCrc(new_rombuf, new_romsize) != crc32val)
    {
	    ret = __LINE__;
	    UG_LOG_ERR("crc check error!!!\n");
	    goto end;
	}
#if 0
	FILE * xd = fopen("A:/diff_patch.bin", "wb");
    if (!xd)
    {
    	printf("file open %s fail\n", "A:/diff_patch.bin");
    }else
    {
        fwrite(new_rombuf, 1, new_romsize, xd);
        fclose(xd);
    }
#endif
    UG_LOG_DBG("read new rom raw size: %u\n", new_romsize);
    //while(1);

    UG_LOG_INFO("[%d%%] Write data to position 0x%X, size %u bytes\n", ugGetProrgessPercentage(), rdata.position, new_romsize);

    // upgrade
    filesize = new_romsize;
    alignsize = ITH_ALIGN_UP(filesize, blocksize + gapsize);
    bufsize = CFG_UG_BUF_SIZE < alignsize ? CFG_UG_BUF_SIZE : alignsize;
#if defined(CFG_NOR_USE_DPUAES) && (CFG_FW_DPUAES_UPGRADE_MARK_POS == 0)
    if (rdata.position == CFG_UPGRADE_IMAGE_POS)
        buf = memalign(4, bufsize);
    else
#endif
    buf = malloc(bufsize);
    if (!buf)
    {
        ret = __LINE__;
        UG_LOG_ERR("Out of memory: %u\n", bufsize);
        goto end;
    }

   	pos = rdata.position / (blocksize + gapsize);
    alignsize = rdata.position % (blocksize + gapsize);

    if (alignsize > 0)
    {
        UG_LOG_WARN("Position 0x%X and block+gap size 0x%X are not aligned; align to 0x%X\n", rdata.position, (blocksize + gapsize), (pos * (blocksize + gapsize)));
    }

    pos2 = lseek(fd, pos, SEEK_SET);
    if (pos2 != pos)
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot lseek: %d != %d\n", pos2, pos);
        goto end;
    }

    UG_LOG_INFO("[%d%%] Writing", ugGetProrgessPercentage());

    remainsize = filesize;
    do
    {
#if defined(CFG_CHIP_PKG_IT9910)
        usleep(1000);
#endif
        if ( !pos2 && (disk == ITP_DISK_NAND) )
        {
       		readsize = (remainsize < blocksize+gapsize) ? remainsize : blocksize+gapsize;
        	pos2++;
        }
        else
        {
        	if(disk == ITP_DISK_NAND)	readsize = (remainsize < blocksize) ? remainsize : blocksize;
            else	readsize = (remainsize < bufsize) ? remainsize : bufsize;
        }
        memcpy(buf, new_rombuf + new_rombufpos, readsize);
        new_rombufpos += readsize;
#if defined(CFG_CHIP_PKG_IT9910)
        usleep(1000);
#endif
        alignsize = ITH_ALIGN_UP(readsize, blocksize) / blocksize;
        if((alignsize * blocksize) > readsize && readsize < bufsize)
            memset(buf + readsize, 0xff, (alignsize * blocksize) - readsize);
        size2 = write(fd, buf, alignsize);
        if (size2 != alignsize)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot write file: %u != %u\n", size2, alignsize);
            goto end;
        }

        remainsize -= readsize;

        putchar('.');
        fflush(stdout);
    }
    while (remainsize > 0);
    putchar('\n');

    // verify
    pos2 = lseek(fd, pos, SEEK_SET);
    if (pos2 != pos)
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot lseek: %d != %d\n", pos2, pos);
        goto end;
    }

#if defined(CFG_NOR_USE_DPUAES) && (CFG_FW_DPUAES_UPGRADE_MARK_POS == 0)
    if (rdata.position == CFG_UPGRADE_IMAGE_POS)
        buf2 = memalign(4, bufsize);
    else
#endif
    buf2 = malloc(bufsize);
    if (!buf2)
    {
        ret = __LINE__;
        UG_LOG_ERR("Out of memory: %u\n", bufsize);
        goto end;
    }

    UG_LOG_INFO("[%d%%] Verifying", ugGetProrgessPercentage());

    remainsize = filesize;
    new_rombufpos = 0;
    do
    {
        if ( !pos2 && (disk == ITP_DISK_NAND) )
        {
        	readsize = (remainsize < blocksize+gapsize) ? remainsize : blocksize+gapsize;
        	pos2++;
        }
        else
        {
        	if(disk == ITP_DISK_NAND)	readsize = (remainsize < blocksize) ? remainsize : blocksize;
            else	readsize = (remainsize < bufsize) ? remainsize : bufsize;
        }
        memcpy(buf, new_rombuf + new_rombufpos, readsize);
        new_rombufpos += readsize;

        alignsize = ITH_ALIGN_UP(readsize, blocksize) / blocksize;

        size2 = read(fd, buf2, alignsize);
        if (size2 != alignsize)
        {
            ret = __LINE__;
            UG_LOG_ERR("Cannot read: %u != %u\n", size2, alignsize);
            goto end;
        }

        if (memcmp(buf, buf2, readsize))
        {
        #ifndef _WIN32
            ret = __LINE__;
            UG_LOG_ERR("\nVerify failed.\n");
            #if defined(CFG_NAND_ENABLE)
            UG_LOG_ERR("\nMaybe the issue that the upgrade use buffer is too small to upgrade.\n");
            #endif
            goto end;
        #endif // _WIN32
        }

        remainsize -= readsize;

        putchar('.');
        fflush(stdout);
    }
    while (remainsize > 0);
    putchar('\n');

end:
    if (fd != -1)
        close(fd);

    if (buf2)
        free(buf2);

    if (buf)
        free(buf);

    if (diff_buf)
        free(diff_buf);

    if (ucl_buf)
        free(ucl_buf);

    if (old_rombuf)
        free(old_rombuf);

    if (new_rombuf)
        free(new_rombuf);

    return ret;
}

#if defined(CFG_NOR_USE_DPUAES) && (CFG_FW_DPUAES_UPGRADE_MARK_POS > 0)
int ugFWAESUpgradeData(void)
{
    int ret = 0, fd = -1;
    uint32_t blocksize = 0, gapsize = 0, pos = 0, blockcount = 0, alignsize = 0, headersize = 0, imagesize = 0,
            contentsize = 0, ori_blockcount = 0, bufsize = 0, size2 = 0;
    uint8_t *rombuf = NULL, *imagebuf = NULL, *realloc_tmp = NULL;
    bool compressed = false;
#ifdef CFG_SECURITY_SIGNATURE
    uint32_t sigpaddingpos = 0, size_tmp = 0, size_addoffset = 0;
#endif

    fd = open(":nor", O_RDONLY, 0);
    if (fd == -1)
    {
        printf("open device error!\n");
        ret = 1;
        goto end;
    }

    if (ioctl(fd, ITP_IOCTL_GET_BLOCK_SIZE, &blocksize))
    {
        printf("get block size error!\n");
        ret = 1;
        goto end;
    }

    if (ioctl(fd, ITP_IOCTL_GET_GAP_SIZE, &gapsize))
    {
        printf("get gap size error!\n");
        ret = 1;
        goto end;
    }

    pos = CFG_UPGRADE_IMAGE_POS / (blocksize + gapsize);
    if (lseek(fd, pos, SEEK_SET) != pos)
    {
        printf("seek to rom position error!\n");
        ret = 1;
        goto end;
    }

    if (blocksize > MAX_HEADER_SIZE)
        alignsize = blocksize;
    else
        alignsize = MAX_HEADER_SIZE;

    rombuf = memalign(4, alignsize);
    if (!rombuf)
    {
        printf("out of memory!\n");
        ret = 1;
        goto end;
    }

    blockcount = alignsize / blocksize;
    size2 = read(fd, rombuf, blockcount);
    if (size2 != blockcount)
    {
        printf("read rom header error!\n");
        ret = 1;
        goto end;
    }

    headersize = *(uint32_t *)(rombuf + HEADER_LEN_OFFSET);
    headersize = itpBetoh32(headersize);
    printf("rom header size: %d\n", headersize);
    if ((headersize % 4) || headersize > 100 * 1024)
    {
        printf("invalid header size: %d\n", headersize);
        ret = 1;
        goto end;
    }

    imagebuf = rombuf + headersize;
#ifndef CFG_SPEEDY_DECOMPRESS_ENABLE
    if (strncmp(imagebuf, "SMAZ", 4) == 0)
    {
        // compressed rom
        imagesize = *(uint32_t *)(rombuf + headersize - 4);
        imagesize = itpBetoh32(imagesize);
        printf("rom image size: %d\n", imagesize);
        contentsize = *(uint32_t *)(rombuf + HEADER_LEN_OFFSET + 4);
        contentsize = itpBetoh32(contentsize);
        printf("compress rom, content size: %d\n", contentsize);
        compressed = true;
    }
#else
    if (strncmp(imagebuf, "ITE", 3) == 0)
    {
        // compressed rom
        imagesize = ((uint32_t)imagebuf[9] << 24) | ((uint32_t)imagebuf[8] << 16) | ((uint32_t)imagebuf[7] << 8) | (uint32_t)imagebuf[6];
        contentsize = *(uint32_t *)(rombuf + headersize - 4);
        contentsize = itpBetoh32(contentsize);
        printf("compress rom, content size: %d\n", contentsize);
        compressed = true;
    }
#endif
    else
    {
        // non-compressed rom
        imagesize = *(uint32_t *)(rombuf + headersize - 4);
        imagesize = itpBetoh32(imagesize);
        printf("rom image size: %d\n", imagesize);
        contentsize = imagesize;
        printf("non-compress rom, content size: %d\n", contentsize);
        compressed = false;
    }

    ori_blockcount = blockcount;
    bufsize = headersize + contentsize;

#ifdef CFG_SECURITY_SIGNATURE
    if (bufsize % 16) bufsize = bufsize + (16 - bufsize % 16);
    sigpaddingpos = bufsize + 16 + 512;
    size_tmp = sigpaddingpos % 64;
    if (size_tmp <= 55) size_addoffset = 56 - size_tmp;
    else size_addoffset = 120 - size_tmp;
    size_addoffset += 8;
    bufsize = sigpaddingpos + size_addoffset + 256;
#endif

    blockcount = bufsize / blocksize;
    if (bufsize % blocksize) blockcount++;
    bufsize = blockcount * (blocksize + gapsize);

    realloc_tmp = rombuf;
    rombuf = memalign(4, bufsize);
    if (rombuf)
        memcpy(rombuf, realloc_tmp, alignsize);
    free(realloc_tmp);
    if (!rombuf)
    {
        printf("out of memory %d\n", bufsize);
        ret = 1;
        goto end;
    }

    blockcount -= ori_blockcount;
    size2 = read(fd, rombuf + alignsize, blockcount);
    if (size2 != blockcount)
    {
        printf("read rom image error: %d != %d\n", ret, blockcount);
        ret = 1;
        goto end;
    }

    close(fd);
    fd = open(":norAES", O_RDWR, 0);
    if (fd == -1)
    {
        printf("Open device error!\n");
        ret = 1;
        goto end;
    }

    if (lseek(fd, pos, SEEK_SET) != pos)
    {
        printf("seek to rom position error!\n");
        ret = 1;
        goto end;
    }

    printf("start to encrypt for firmware ...\n");
    blockcount += ori_blockcount;
    size2 = write(fd, rombuf, blockcount);
    if (size2 != blockcount)
    {
        printf("Cannot write file: %d != %d\n", size2, alignsize);
        ret = 1;
        goto end;
    }

end:
    if (rombuf)
        free(rombuf);

    if (fd != -1)
        close(fd);

    return ret;
}
#endif
