#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "ite/ug.h"
#include "ug_cfg.h"

typedef enum
{
    TAG_END         = 0,
    TAG_DEVICE      = 1,
    TAG_UNFORMATTED = 2,
    TAG_RAWDATA     = 3,
    TAG_PARTITION   = 4,
    TAG_DIRECTORY   = 5,
    TAG_FILE        = 6
} tag_type;

#define FLAG_ENCRYPT            (0x1 << 0)
#define FLAG_PARTITION          (0x1 << 1)
#define FLAG_NO_PARTITION       (0x1 << 2)
#define FLAG_FORMAT_PARTITION   (0x1 << 3)
#define FLAG_DIFFERENCE         (0x1 << 4)

#pragma pack(1)
typedef struct
{
    uint8_t version[16];
    uint32_t blocks_offset;
    uint32_t flags;
} package_t;

extern bool fatInited;
extern bool driveInited;

static UGDevice ugDevice;

static int ugProgressPercentage = 0;

#ifdef CFG_ALLOW_CHANGE_RESERVED_SIZE
extern uint32_t g_pkg_reserved_size;
extern uint8_t gPkgUpgrading;
#endif

int ugUpgradePackage(ITCStream* file)
{
    package_t pkg;
    uint32_t readsize = 0;
    int ret = 0;
    bool firstTag = true;
    uint32_t tag = 0;
    uint32_t diff = 0;
    int partition = 0;
    char drive[4] = { '\0' };

#ifdef CFG_UPGRADE_BACKUP_RAW_DATA
    uint16_t val = 0;
    #if defined(CFG_FS_YAFFS2) || (defined(CFG_FS_LFS) && !defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0) && !defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))
    uint32_t FAT_OFFSET = 0;
    #else
    uint32_t FAT_OFFSET = 0x40000;
    #endif
#if defined(CFG_NAND_PARTITION0)
    uint32_t RESERVED_SIZE_AND_FAT_OFFSET = CFG_NAND_RESERVED_SIZE + FAT_OFFSET;
#elif defined(CFG_NOR_PARTITION0)
    uint32_t RESERVED_SIZE_AND_FAT_OFFSET = CFG_NOR_RESERVED_SIZE + FAT_OFFSET;
#elif defined(CFG_SD0_PARTITION0)
    uint32_t RESERVED_SIZE_AND_FAT_OFFSET = CFG_SD0_RESERVED_SIZE + FAT_OFFSET;
#elif defined(CFG_SD1_PARTITION0)
    uint32_t RESERVED_SIZE_AND_FAT_OFFSET = CFG_SD1_RESERVED_SIZE + FAT_OFFSET;
#else
    #error "unknown device."
#endif
#if defined(CFG_UPGRADE_BACKUP_RAW_DATA_NAND)
    ITPDisk srcDisk = ITP_DISK_NAND;
    ITPDisk dstDisk = ITP_DISK_NAND;
#elif defined(CFG_UPGRADE_BACKUP_RAW_DATA_NOR)
    ITPDisk srcDisk = ITP_DISK_NOR;
    ITPDisk dstDisk = ITP_DISK_NOR;
#elif defined(CFG_UPGRADE_BACKUP_RAW_DATA_SD0)
    ITPDisk srcDisk = ITP_DISK_SD0;
    ITPDisk dstDisk = ITP_DISK_SD0;
#elif defined(CFG_UPGRADE_BACKUP_RAW_DATA_SD1)
    ITPDisk srcDisk = ITP_DISK_SD1;
    ITPDisk dstDisk = ITP_DISK_SD1;
#else
    #error "unknown device."
#endif
#endif

    UG_LOG_INFO("Start to upgrade package\n");

    ugProgressPercentage = 0;

#ifdef CFG_ALLOW_CHANGE_RESERVED_SIZE
	gPkgUpgrading = 0x96;
#endif

#if defined(CFG_FS_YAFFS2)
    ioctl(ITP_DEVICE_YAFFS2, ITP_IOCTL_BG_GC_EXIT, NULL); //Close Yaffs Background GC when upgrade PKG
#endif

    // read package header
    readsize = itcStreamRead((ITCStream*)file, &pkg, sizeof(package_t));
    if (readsize != sizeof(package_t))
    {
        UG_LOG_ERR("Cannot read file: %d != %d\n", readsize, sizeof(package_t));
        goto end;
    }

    pkg.blocks_offset = itpLetoh32(pkg.blocks_offset);
    pkg.flags = itpLetoh32(pkg.flags);

    UG_LOG_DBG("version=%c%c%c%c%c%c%c%c blocks_offset=0x%X flags=0x%X\n",
        pkg.version[0], pkg.version[1], pkg.version[2], pkg.version[3], pkg.version[4], pkg.version[5], pkg.version[6], pkg.version[7],
        pkg.blocks_offset,
        pkg.flags
   );

#if defined(CFG_A_B_DUAL_BOOT_ENABLE)
    if(pkg.flags & FLAG_PARTITION)
    {
	    printf("PKG is force partiontion \n");
        ugUpgradeSetForcePartition(true);
    }else
    {
        printf("PKG is not partiontion \n");
        ugUpgradeSetForcePartition(false);
    }
#endif 

#ifdef CFG_UPGRADE_BACKUP_RAW_DATA
    val = pkg.flags >> 16;
    printf("version=%c%c%c%c%c%c%c%c blocks_offset=0x%X flags=0x%X\n",
            pkg.version[0], pkg.version[1], pkg.version[2], pkg.version[3], pkg.version[4], pkg.version[5], pkg.version[6], pkg.version[7],
            pkg.blocks_offset,
            pkg.flags);

    if (val & FLAG_CUSTOM_UPGRADE_FW)
    {
        printf("Copy FW to backup\n");
#if defined(CFG_UPGRADE_IMAGE_NAND)
        srcDisk = ITP_DISK_NAND;
#elif defined(CFG_UPGRADE_IMAGE_NOR)
        srcDisk = ITP_DISK_NOR;
#elif defined(CFG_UPGRADE_IMAGE_SD0)
        srcDisk = ITP_DISK_SD0;
#elif defined(CFG_UPGRADE_IMAGE_SD1)
        srcDisk = ITP_DISK_SD1;
#endif
        if (ret = ugCopyRawData(CFG_UPGRADE_BACKUP_RAW_DATA_FW_POS,
                CFG_UPGRADE_IMAGE_POS,
                CFG_UPGRADE_BACKUP_RAW_DATA_FW_SIZE, 
                srcDisk, 
                dstDisk))
        {
            printf("ugCopyRawData fail_0\n");
            goto end;
        }
    }

    if (val & FLAG_CUSTOM_UPGRADE_FAT_P0)
    {
        printf("Copy FAT partition 0 to backup\n");
        if (!fatInited)
        {
            itpRegisterDevice(ITP_DEVICE_FAT, &itpFSDeviceFat.dev);
            ioctl(ITP_DEVICE_FAT, ITP_IOCTL_INIT, NULL);
            ioctl(ITP_DEVICE_FAT, ITP_IOCTL_ENABLE, NULL);
        }
        if (!driveInited)
        {
    #if defined(CFG_FS_FAT) || defined(CFG_FS_NTFS) || defined(CFG_FS_LFS) || defined(CFG_FS_YAFFS2)
            itpRegisterDevice(ITP_DEVICE_DRIVE, &itpDeviceDrive);
            ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_INIT, NULL);
    #endif
        }

        srcDisk = ugGetPartitionDisk(0);
        if(srcDisk != -1)
        {
            if (ret = ugCopyRawData(CFG_UPGRADE_BACKUP_RAW_DATA_PARTITION0_POS,
                RESERVED_SIZE_AND_FAT_OFFSET,
                CFG_UPGRADE_BACKUP_RAW_DATA_PARTITION0_SIZE,
                srcDisk, 
                dstDisk))
            {
                printf("ugCopyRawData fail_1_0\n");
                goto end;
            }
        }
        else
        {
            printf("ugCopyRawData no disk, skip_1_0\n");
        }
    }

    if (val & FLAG_CUSTOM_UPGRADE_FAT_P1)
    {
        printf("Copy FAT partition 1 to backup\n");
        if (!fatInited)
        {
            itpRegisterDevice(ITP_DEVICE_FAT, &itpFSDeviceFat.dev);
            ioctl(ITP_DEVICE_FAT, ITP_IOCTL_INIT, NULL);
            ioctl(ITP_DEVICE_FAT, ITP_IOCTL_ENABLE, NULL);
        }
        if (!driveInited)
        {
    #if defined(CFG_FS_FAT) || defined(CFG_FS_NTFS) || defined(CFG_FS_LFS) || defined(CFG_FS_YAFFS2)
            itpRegisterDevice(ITP_DEVICE_DRIVE, &itpDeviceDrive);
            ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_INIT, NULL);
    #endif
        }

        srcDisk = ugGetPartitionDisk(1);
        if(srcDisk != -1)
        {
            if (ret = ugCopyRawData(CFG_UPGRADE_BACKUP_RAW_DATA_PARTITION1_POS,
                    RESERVED_SIZE_AND_FAT_OFFSET + CFG_UPGRADE_BACKUP_RAW_DATA_PARTITION0_SIZE,
                    CFG_UPGRADE_BACKUP_RAW_DATA_PARTITION1_SIZE,
                    srcDisk, 
                    dstDisk))
            {
                printf("ugCopyRawData fail_1_1\n");
                goto end;
            }
        }
        else
        {
            printf("ugCopyRawData no disk, skip_1_1\n");
        }
    }

    if (val & FLAG_CUSTOM_UPGRADE_FAT_P2)
    {
        printf("Copy FAT partition 2 to backup\n");
        if (!fatInited)
        {
            itpRegisterDevice(ITP_DEVICE_FAT, &itpFSDeviceFat.dev);
            ioctl(ITP_DEVICE_FAT, ITP_IOCTL_INIT, NULL);
            ioctl(ITP_DEVICE_FAT, ITP_IOCTL_ENABLE, NULL);
        }
        if (!driveInited)
        {
    #if defined(CFG_FS_FAT) || defined(CFG_FS_NTFS) || defined(CFG_FS_LFS) || defined(CFG_FS_YAFFS2)
            itpRegisterDevice(ITP_DEVICE_DRIVE, &itpDeviceDrive);
            ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_INIT, NULL);
    #endif
        }

        srcDisk = ugGetPartitionDisk(2);
        if(srcDisk != -1)
        {
            if (ret = ugCopyRawData(CFG_UPGRADE_BACKUP_RAW_DATA_PARTITION2_POS,
                RESERVED_SIZE_AND_FAT_OFFSET + CFG_UPGRADE_BACKUP_RAW_DATA_PARTITION0_SIZE + CFG_UPGRADE_BACKUP_RAW_DATA_PARTITION1_SIZE,
                CFG_UPGRADE_BACKUP_RAW_DATA_PARTITION2_SIZE,
                srcDisk, 
                dstDisk))
            {
                printf("ugCopyRawData fail_1_2\n");
                goto end;
            }
        }
        else
        {
            printf("ugCopyRawData no disk, skip_1_2\n");
        }
    }

    if (val & FLAG_CUSTOM_UPGRADE_FAT_P3)
    {
        printf("Copy FAT partition 3 to backup\n");
        if (!fatInited)
        {
            itpRegisterDevice(ITP_DEVICE_FAT, &itpFSDeviceFat.dev);
            ioctl(ITP_DEVICE_FAT, ITP_IOCTL_INIT, NULL);
            ioctl(ITP_DEVICE_FAT, ITP_IOCTL_ENABLE, NULL);
        }
        if (!driveInited)
        {
    #if defined(CFG_FS_FAT) || defined(CFG_FS_NTFS) || defined(CFG_FS_LFS) || defined(CFG_FS_YAFFS2)
            itpRegisterDevice(ITP_DEVICE_DRIVE, &itpDeviceDrive);
            ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_INIT, NULL);
    #endif
        }

        srcDisk = ugGetPartitionDisk(3);
        if(srcDisk != -1)
        {
            if (ret = ugCopyRawData(CFG_UPGRADE_BACKUP_RAW_DATA_PARTITION3_POS,
                    RESERVED_SIZE_AND_FAT_OFFSET + CFG_UPGRADE_BACKUP_RAW_DATA_PARTITION0_SIZE + CFG_UPGRADE_BACKUP_RAW_DATA_PARTITION1_SIZE + CFG_UPGRADE_BACKUP_RAW_DATA_PARTITION2_SIZE + FAT_OFFSET,
                    CFG_UPGRADE_BACKUP_RAW_DATA_PARTITION3_SIZE,
                    srcDisk, 
                    dstDisk))
            {
                printf("ugCopyRawData fail_1_3\n");
                goto end;
            }
        }
        else
        {
            printf("ugCopyRawData no disk, skip_1_3\n");
        }
    }
#endif

    for (;;)
    {
        // reag tag
        readsize = itcStreamRead((ITCStream*)file, &tag, sizeof(uint32_t));
        if (readsize != sizeof(uint32_t))
        {
            UG_LOG_ERR("Cannot read file: %ld != %ld\n", readsize, sizeof(uint32_t));
            goto end;
        }

        tag = itpLetoh32(tag);

        if (firstTag && !ugDevice.restore && (tag >= TAG_RAWDATA))
        {
            bool backup = false;

            if (tag == TAG_RAWDATA)
            {
                #pragma pack(1)
                typedef struct
                {
                    uint32_t position;
                    uint32_t rawdata_size;
                } rawdata_t;
                rawdata_t rdata;
                uint32_t readsize;

                //read out diff upgrade flag
                readsize = itcStreamRead((ITCStream*)file, &diff, sizeof(uint32_t));
                if (readsize != sizeof(uint32_t))
                {
                    ret = __LINE__;
                    UG_LOG_ERR("Cannot read file: %ld != %ld\n", readsize, sizeof(uint32_t));
                    goto end;
                }
                
                // read raw data header
                readsize = itcStreamRead((ITCStream*)file, &rdata, sizeof(rawdata_t));
                if (readsize != sizeof(rawdata_t))
                {
                    ret = __LINE__;
                    UG_LOG_ERR("Cannot read file: %ld != %ld\n", readsize, sizeof(rawdata_t));
                    goto end;
                }
                itcStreamSeek((ITCStream*)file, -((long)(sizeof(rawdata_t)+sizeof(uint32_t))), SEEK_CUR);

                rdata.rawdata_size  = itpLetoh32(rdata.rawdata_size);

                if (rdata.rawdata_size == 0)
                {
                    backup = true;
                }
            }

#ifndef CFG_UPGRADE_BACKUP_RAW_DATA
            ret = ugUpgradeStart(backup);
#endif
            if (ret)
            {
                UG_LOG_ERR("Start to upgrade package failed: %d\n", ret);
                goto end;
            }
            firstTag = false;
        }

        switch (tag)
        {
        case TAG_END:
            UG_LOG_DBG("end:\n");
            ret = ugUpgradeEnd(file);
            if (ret)
                goto end;

#ifndef CFG_UPGRADE_BACKUP_RAW_DATA
            ret = ugUpgradeFinish();
#else
    #if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
            UG_LOG_INFO("Flushing NOR cache...\n");
            ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
    #endif
    #if (CFG_LFS_CACHE_SIZE > 0)
            UG_LOG_INFO("Flushing LFS cache...\n");
            ioctl(ITP_DEVICE_LFS, ITP_IOCTL_FLUSH, NULL);
    #endif
            ret = ugUpgradeTag(UG_UPGRADE_FLOW_FINISH);
#endif
            if (ret)
            {
                UG_LOG_ERR("Finish upgrading package failed: %d\n", ret);
                goto end;
            }
            goto end;

        case TAG_DEVICE:
            UG_LOG_DBG("device:\n");
            ret = ugUpgradeDevice(file, &ugDevice);
            if (ret)
                goto end;

            ugDevice.partition = pkg.flags & FLAG_PARTITION ? true : false;
            ugDevice.nopartition = pkg.flags & FLAG_NO_PARTITION ? true : false;

            partition = 0;
            break;

        case TAG_UNFORMATTED:
            UG_LOG_DBG("unformatted:\n");
            ret = ugUpgradeUnformatted(file, ugDevice.disk);
            if (ret)
                goto end;
            break;

        case TAG_RAWDATA:
            UG_LOG_DBG("rawdata:\n");

            // reag diff flag
            readsize = itcStreamRead((ITCStream*)file, &diff, sizeof(uint32_t));
            if (readsize != sizeof(uint32_t))
            {
                UG_LOG_ERR("Cannot read file: %ld != %ld\n", readsize, sizeof(uint32_t));
                goto end;
            }

            diff = itpLetoh32(diff);
            UG_LOG_INFO("rawdata diff %d:\n", diff);

#ifndef CFG_UPGRADE_BACKUP_RAW_DATA
            if((pkg.flags & FLAG_DIFFERENCE) && diff)
                ret = ugUpgradeRawData_diff(file, ugDevice.disk, ugDevice.restore);
            else
                ret = ugUpgradeRawData(file, ugDevice.disk, ugDevice.restore);
#else
            if((pkg.flags & FLAG_DIFFERENCE) && diff)
                ret = ugUpgradeRawData_diff(file, ugDevice.disk, ugDevice.restore, val & FLAG_CUSTOM_UPGRADE_FW ? true : false);
            else
                ret = ugUpgradeRawData(file, ugDevice.disk, ugDevice.restore, val & FLAG_CUSTOM_UPGRADE_FW ? true : false);
#endif
            if (ret)
                goto end;
            break;

        case TAG_PARTITION:
            UG_LOG_DBG("partition:\n");
            if (partition == 0)
            {
                ret = ugUpgradeDoPartition(&ugDevice);
                if (ret)
                    goto end;
            }

#ifdef CFG_UPGRADE_BACKUP_RAW_DATA
    #if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
            UG_LOG_INFO("Flushing NOR cache...\n");
            ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
    #endif
    #if (CFG_LFS_CACHE_SIZE > 0)
            UG_LOG_INFO("Flushing LFS cache...\n");
            ioctl(ITP_DEVICE_LFS, ITP_IOCTL_FLUSH, NULL);
    #endif
            if (partition == 0 && val & FLAG_CUSTOM_UPGRADE_FAT_P0)
                ret = ugUpgradeTag(UG_UPGRADE_FLOW_FAT_P0);
            else if (partition == 1 && val & FLAG_CUSTOM_UPGRADE_FAT_P1)
                ret = ugUpgradeTag(UG_UPGRADE_FLOW_FAT_P1);
            else if (partition == 2 && val & FLAG_CUSTOM_UPGRADE_FAT_P2)
                ret = ugUpgradeTag(UG_UPGRADE_FLOW_FAT_P2);
            else if (partition == 3 && val & FLAG_CUSTOM_UPGRADE_FAT_P3)
                ret = ugUpgradeTag(UG_UPGRADE_FLOW_FAT_P3);

            if (ret)
            {
                UG_LOG_ERR("Start to upgrade FAT failed: %d\n", ret);
                goto end;
            }
#endif

            // reag diff flag
            readsize = itcStreamRead((ITCStream*)file, &diff, sizeof(uint32_t));
            if (readsize != sizeof(uint32_t))
            {
                UG_LOG_ERR("Cannot read file: %ld != %ld\n", readsize, sizeof(uint32_t));
                goto end;
            }

            diff = itpLetoh32(diff);
            UG_LOG_INFO("partition diff %d:\n", diff);

            if(pkg.flags & FLAG_DIFFERENCE)
                ret = ugUpgradePartition(file, ugDevice.disk, partition, drive, (diff & 0x1) ? false : true);
            else
                ret = ugUpgradePartition(file, ugDevice.disk, partition, drive, (pkg.flags & FLAG_FORMAT_PARTITION) ? true : false);
            if (ret)
                goto end;

            partition++;
            break;

        case TAG_DIRECTORY:
            UG_LOG_DBG("directory:\n");
            ret = ugUpgradeDirectory(file, drive);
            if (ret)
                goto end;
            break;

        case TAG_FILE:
            UG_LOG_DBG("file:\n");
			if(pkg.flags & FLAG_DIFFERENCE)
            	ret = ugUpgradeFile_diff(file, drive);
			else
				ret = ugUpgradeFile(file, drive);
            if (ret)
                goto end;
            break;

        default:
            UG_LOG_ERR("Unknown tag %d\n", tag);
            ret = __LINE__;
            goto end;
        }
        ugProgressPercentage = (int)((uint64_t)(file->size - itcStreamAvailable(file)) * 100 / file->size);
#ifdef CFG_A_B_DUAL_BOOT_ENABLE
        usleep(1000);
#endif
    }

end:

#ifdef CFG_ALLOW_CHANGE_RESERVED_SIZE
	gPkgUpgrading = 0;
	g_pkg_reserved_size = 0;
#endif

    itcStreamClose((ITCStream*)file);
    return ret;
}

int ugGetProrgessPercentage(void)
{
    return ugProgressPercentage;
}

void ugRestoreStart(void)
{
    ugDevice.restore = true;
}
