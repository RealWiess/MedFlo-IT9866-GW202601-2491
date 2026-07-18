#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ite/ug.h"
#include "nor/mmp_nor.h"
#include "ug_cfg.h"
#include <assert.h>

#ifndef _WIN32
#include "ssp/mmp_axispi.h"
#endif

#define MAX_PARTITION_COUNT 4

#define MOUNT_DISKS_MAX         (5+1)   // NOR, NAND, SD0, SD1 and RAMDISK, + 1 for end marker

#if defined(CFG_A_B_DUAL_BOOT_ENABLE)
#ifndef CFG_SUB_PUBLIC_PARTITION
#define CFG_SUB_PUBLIC_PARTITION 0xFF
#endif
#ifndef CFG_SUB_PRIVATE_PARTITION
#define CFG_SUB_PRIVATE_PARTITION 0xFF
#endif
#endif

#pragma pack(1)
typedef struct
{
    uint64_t partition_size;
} partition_t;

typedef enum
{
    DEVICE_NAND = 0,
    DEVICE_NOR  = 1,
    DEVICE_SD0  = 2,
    DEVICE_SD1  = 3,

    DEVICE_COUNT
} DeviceType;

static const char* device_table[] =
{
    "NAND",
    "NOR",
    "SD0",
    "SD1"
};

static const char* disk_table[] =
{
    "SD0",
    "SD1",
    "CF",
    "MS",
    "XD",
    "NAND",
    "NOR",
    "MSC00",
    "MSC01",
    "MSC02",
    "MSC03",
    "MSC04",
    "MSC05",
    "MSC06",
    "MSC07",
    "MSC10",
    "MSC11",
    "MSC12",
    "MSC13",
    "MSC14",
    "MSC15",
    "MSC16",
    "MSC17",
    "RAM",
};

#if defined(CFG_A_B_DUAL_BOOT_ENABLE)
static bool ugPkgisForcePartition = false;

void ugUpgradeSetForcePartition(bool isForcePartition)
{
    ugPkgisForcePartition = isForcePartition;
}

bool ugUpgradeIsForcePartition(void)
{
    return ugPkgisForcePartition;
}
#endif


/**
 * @brief Get the sequence of disk to be mounted.
 *
 * This function get the sequence of disk to be mounted in the given order of NOR, NAND, SD0, SD1 and RAMDISK, 
 * and the order of disk is defined by itp_drive_openrtos.c.
 *
 * @param seq The sequence of disk, the length of seq is MOUNT_DISKS_MAX.
 */
static void GetDiskMountSeq(ITPDisk *seq)
{
    int count = 0;
#ifdef CFG_NOR_ENABLE
    seq[count] = ITP_DISK_NOR;
    count++;
#endif
#ifdef CFG_NAND_ENABLE
    seq[count] = ITP_DISK_NAND;
    count++;
#endif
#ifdef CFG_SD0_STATIC
    seq[count] = ITP_DISK_SD0;
    count++;
#endif
#ifdef CFG_SD1_STATIC
    seq[count] = ITP_DISK_SD1;
    count++;
#endif
#ifdef CFG_RAMDISK_ENABLE
    seq[count] = ITP_DISK_RAM;
    count++;
#endif
    assert(count < MOUNT_DISKS_MAX);
    for(; count < MOUNT_DISKS_MAX; count++)
    {
        seq[count] = -1;
    }
} 
/**
 * @brief Get the disk of the given partition.
 *
 * This function get the disk type of the given partition. The partition is counted from 0, 
 * and the order of disk is defined by itp_drive_openrtos.c.
 *
 * @param partition The partition number, starting from 0.
 *
 * @return The disk type of the given partition, or -1 if the partition is not found.
 */
ITPDisk ugGetPartitionDisk(int partition)
{
    ITPDisk mountSeq[MOUNT_DISKS_MAX] = {0};
    ITPPartition par = {0};
    int partitionCnt = 0;
    
    GetDiskMountSeq(mountSeq);
    for(int i = 0; i < MOUNT_DISKS_MAX; i++)
    {
        if (mountSeq[i] == -1) break;
        par.disk = mountSeq[i];
        ioctl(UG_DEVICE, ITP_IOCTL_GET_PARTITION, (void *)&par);
        partitionCnt += par.count;
        if (partitionCnt > partition) return par.disk;
    }
    return -1;
} 

int ugUpgradePartition(ITCStream *f, ITPDisk disk, int partition, char* drive, bool format)
{
    ITPDriveStatus* driveStatusTable = NULL;
    int ret, i, j;
    partition_t par;
    uint32_t readsize;
#if defined(CFG_FS_LFS)
    int first_index = -1;
#endif
#if defined(CFG_A_B_DUAL_BOOT_ENABLE) && !defined(CFG_SUB_DATA_NONE)
    uint8_t boot_args = 0x0U;
    int partition_offset = 0;
#if defined(CFG_MAIN_DATA_NAND)
    ITPDisk main_disk = ITP_DISK_NAND;
#elif defined(CFG_MAIN_DATA_NOR)
    ITPDisk main_disk = ITP_DISK_NOR;
#elif defined(CFG_MAIN_DATA_SD0)
    ITPDisk main_disk = ITP_DISK_SD0;
#elif defined(CFG_MAIN_DATA_SD1)
    ITPDisk main_disk = ITP_DISK_SD1;
#endif
    if (ugBootargsRead(&boot_args))
    {
        UG_LOG_ERR("Upgrade read boot args failed.\n");
    }
    if ((boot_args == 0xAAU) && (disk == main_disk) && ugUpgradeIsForcePartition() == false)
    {
        if (partition == CFG_PRIVATE_PARTITION)
        {
            partition_offset = CFG_SUB_PRIVATE_PARTITION;
        }else if (partition == CFG_PUBLIC_PARTITION)
        {
            partition_offset = CFG_SUB_PUBLIC_PARTITION;
        }
        else
        {
            partition_offset = partition;
        }
    }else
    {
        partition_offset = partition;
    }
#endif

    // read partition header
    readsize = itcStreamRead((ITCStream*)f, &par, sizeof(partition_t));
    if (readsize != sizeof(partition_t))
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot read file: %ld != %ld\n", readsize, sizeof(partition_t));
        return ret;
    }

    par.partition_size  = itpLetoh64(par.partition_size);

    UG_LOG_DBG("partition_size=%llu\n", par.partition_size);

#ifdef _WIN32
    goto end;
#endif

    // try to find the drive
    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    j = 0;
    for (i = 0; i < ITP_MAX_DRIVE; i++)
    {
        ITPDriveStatus* driveStatus = &driveStatusTable[i];

        if (driveStatus->avail)
        {
            UG_LOG_DBG(
                "drive %c: disk=%s name=%s\n", 'A' + i, disk_table[driveStatus->disk], driveStatus->name
            );
        }

        if (driveStatus->disk == disk && driveStatus->avail)
        {
#if defined(CFG_FS_LFS)
            if (driveStatus->device == ITP_DEVICE_LFS)
                if (first_index == -1)
                    first_index = i;
#endif
#if defined(CFG_A_B_DUAL_BOOT_ENABLE) && !defined(CFG_SUB_DATA_NONE)
            if (((boot_args == 0xAAU) || (boot_args == 0xBBU)) && ((partition == CFG_SUB_PRIVATE_PARTITION) || (partition == CFG_SUB_PUBLIC_PARTITION)) && ugUpgradeIsForcePartition() == false)
            {
                ret = 0;
                goto end;
            }
#if defined(CFG_DYNAMIC_MOUNT_FS_ENABLE)
            if (driveStatus->par_idx == partition_offset)
#else
            if (j++ == partition_offset)
#endif
#else
            if (j++ == partition)
#endif
            {
                if (format && par.partition_size > 0)
                {
#if (!defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0)) && (!defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))
    #if defined(CFG_FS_LFS)
                    ret = ioctl(UG_DEVICE, ITP_IOCTL_FORMAT_PARTITION, (void*) ((disk << 8) | (i - first_index)));
    #else
                    ret = ioctl(UG_DEVICE, ITP_IOCTL_FORMAT, (void*) i);
    #endif
#else
                if (driveStatus->device == ITP_DEVICE_FAT)
                {
                    ret = ioctl(UG_DEVICE_2, ITP_IOCTL_FORMAT, (void*) i);
                }
                else if (driveStatus->device == ITP_DEVICE_LFS)
                {
                    ret = ioctl(UG_DEVICE, ITP_IOCTL_FORMAT_PARTITION, (void*) ((disk << 8) | (i - first_index)));
                }
#endif
                UG_LOG_DBG(
                    "format drive %c: disk=%s name=%s ret=%d\n", 'A' + i, disk_table[driveStatus->disk], driveStatus->name, ret
                );
                }
                strcpy(drive, driveStatus->name);
                ret = 0;
                goto end;
            }
        }
    }

    UG_LOG_ERR("Cannot find the partition %d in %s disk\n", partition, disk_table[disk]);
    ret = __LINE__;

end:
#ifdef _WIN32
    ret = 0;
    strcpy(drive, "A:/");
#endif
    return ret;
}

int ugUpgradeDoPartition(UGDevice* device)
{
    int i, ret = 0;
    uint64_t inc;
    ITPPartition par = {0};
    ITPDriveStatus* driveStatusTable = NULL;
    int targetDev, targetDev2;
#if !defined(_WIN32) && defined(CFG_NOR_ENABLE) && !defined(CFG_FS_LFS)
    uint8_t fat_exbr[ITP_FAT_EXBR_SIZE] = {0};
    uint8_t fat_exbr_path[ITP_FAT_EXBR_PATH_SIZE] = {0};
    uint8_t fat_exbr_drive = 0;
    bool    fat_exbr_drive_find = false;
    FILE *f = NULL;
#endif
#if (defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0)) || (defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))
    ITPPartition par_2 = {0};
#endif

    if (device->device_type == DEVICE_NAND)
    {
        par.disk = ITP_DISK_NAND;
        #if defined (UG_DEVICE_2)
            targetDev = targetDev2 = UG_DEVICE_2;
        #else
            targetDev = targetDev2 = UG_DEVICE;
        #endif
        targetDev = targetDev2 = UG_DEVICE;
    #if (defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0)) || (defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))
        par_2.disk = ITP_DISK_NAND;
    #endif
    }
    else if (device->device_type == DEVICE_NOR)
    {
        par.disk = ITP_DISK_NOR;
        targetDev = UG_DEVICE;
    #if defined (UG_DEVICE_2)
        targetDev2 = UG_DEVICE_2;
    #else
        targetDev2 = UG_DEVICE;
    #endif
    #if (defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0)) || (defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))
        par_2.disk = ITP_DISK_NOR;
    #endif
    }
    else if (device->device_type == DEVICE_SD0)
    {
        par.disk = ITP_DISK_SD0;

        #if (defined (UG_DEVICE_2) && defined(CFG_UPGRADE_FAT_ON_SD))
            targetDev = targetDev2 = UG_DEVICE_2;
        #elif (defined (UG_DEVICE_2))
            targetDev = UG_DEVICE;
            targetDev2 = UG_DEVICE_2;
        #else
            targetDev = targetDev2 = UG_DEVICE;
        #endif
    #if (defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0)) || (defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))
        par_2.disk = ITP_DISK_SD0;
    #endif
    }
    else if (device->device_type == DEVICE_SD1)
    {
        par.disk = ITP_DISK_SD1;
        #if (defined (UG_DEVICE_2) && defined(CFG_UPGRADE_FAT_ON_SD))
            targetDev = targetDev2 = UG_DEVICE_2;
        #elif (defined (UG_DEVICE_2))
            targetDev = UG_DEVICE;
            targetDev2 = UG_DEVICE_2;
        #else
            targetDev = targetDev2 = UG_DEVICE;
        #endif
    #if (defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0)) || (defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))
        par_2.disk = ITP_DISK_SD1;
    #endif
    }
    else
    {
        ret = __LINE__;
        UG_LOG_ERR("Unknown device type: %d\n", device->device_type);
        goto end;
    }

    if (device->partition_count == 0)
        goto end;

    if (device->partition)
    {
        UG_LOG_INFO("Force to partition\n");
        goto repartition;
    }

/*#if ((defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0)) || (defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))) && defined(CFG_UPGRADE_PARTITION)
    UG_LOG_INFO("Force to partition for UPGRADE_LFS_FAT_ON_PARTITION0\n");
    goto repartition;
#endif*/

#ifdef _WIN32
    goto end;
#endif

    if (!device->nopartition)
    {
    #if (!defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0)) && (!defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))
        ret = ioctl(targetDev, ITP_IOCTL_GET_PARTITION, &par);
        if (ret)
        {
            UG_LOG_INFO("Origial partition not exist\n");
            goto repartition;
        }

        if (par.count != device->partition_count)
        {
            UG_LOG_INFO("Origial partition count %d not equal %d\n", par.count, device->partition_count);
            goto repartition;
        }

        for (i = 0; i < (int)device->partition_count; i++)
        {
            UG_LOG_INFO("Origial partition %d size is %lld bytes\n", i, par.size[i]);
            if (device->size[i] && par.size[i] != device->size[i])
                goto repartition;
        }
    #else
        #if !defined(UG_DEVICE_PAR_INFO)
            ret = ioctl(targetDev2, ITP_IOCTL_GET_PARTITION, &par_2);
            if (ret)
            {
                UG_LOG_INFO("Origial partition_0 not exist for UPGRADE_LFS_FAT_ON_PARTITION0\n");
                goto repartition;
            }

            #if !defined(CFG_NOR_PARTITION0_SIZE)
                if (par_2.count != 0)
                {
                    UG_LOG_INFO("Origial partition_0 count %d not equal %d for UPGRADE_LFS_FAT_ON_PARTITION0\n", par_2.count, 0);
                    goto repartition;
                }

                UG_LOG_INFO("Origial partition_0 %d size is %lld bytes\n", 0, par_2.size[0]);
                if (par_2.size[0] != 0)
                    goto repartition;
            #else
                if (par_2.count != 1)
                {
                    UG_LOG_INFO("Origial partition_0 count %d not equal %d for UPGRADE_LFS_FAT_ON_PARTITION0\n", par_2.count, 1);
                    goto repartition;
                }

                UG_LOG_INFO("Origial partition_0 %d size is %lld bytes\n", 0, par_2.size[0]);

                if (CFG_NOR_PARTITION0_SIZE == 0)
                {
                    if (CFG_UPGRADE_NOR_IMAGE_SIZE - CFG_NOR_RESERVED_SIZE && par_2.size[0] != CFG_UPGRADE_NOR_IMAGE_SIZE - CFG_NOR_RESERVED_SIZE)
                        goto repartition;
                }
                else
                {
                    if (CFG_NOR_PARTITION0_SIZE && par_2.size[0] != CFG_NOR_PARTITION0_SIZE)
                        goto repartition;
                }
            #endif

            ret = ioctl(targetDev, ITP_IOCTL_GET_PARTITION, &par);
            if (ret)
            {
                UG_LOG_INFO("Origial partition_1 not exist for UPGRADE_LFS_FAT_ON_PARTITION0\n");
                goto repartition;
            }

            #if !defined(CFG_NOR_PARTITION1_SIZE)
                if (par.count != 0)
                {
                    UG_LOG_INFO("Origial partition_1 count %d not equal %d for UPGRADE_LFS_FAT_ON_PARTITION0\n", par.count, 0);
                    goto repartition;
                }

                UG_LOG_INFO("Origial partition_1 %d size is %lld bytes\n", 0, par.size[0]);
                if (par.size[0] != 0)
                    goto repartition;
            #else
                if (CFG_NOR_PARTITION1_SIZE == 0)
                {
                    if (CFG_UPGRADE_NOR_IMAGE_SIZE - CFG_NOR_RESERVED_SIZE - CFG_NOR_PARTITION0_SIZE - CFG_LFS_FAT_GAP &&
                            par.size[0] != CFG_UPGRADE_NOR_IMAGE_SIZE - CFG_NOR_RESERVED_SIZE - CFG_NOR_PARTITION0_SIZE - CFG_LFS_FAT_GAP)
                        goto repartition;
                }
                else
                {
                    if (CFG_NOR_PARTITION1_SIZE && par.size[0] != CFG_NOR_PARTITION1_SIZE)
                        goto repartition;

                #if !defined(CFG_NOR_PARTITION2_SIZE)
                    if (par.count != 1)
                    {
                        UG_LOG_INFO("Origial partition_1 count %d not equal %d for UPGRADE_LFS_FAT_ON_PARTITION0\n", par.count, 1);
                        goto repartition;
                    }

                    UG_LOG_INFO("Origial partition_1 %d size is %lld bytes\n", 1, par.size[1]);
                    if (par.size[1] != 0)
                        goto repartition;
                #else
                    if (CFG_NOR_PARTITION2_SIZE == 0)
                    {
                        if (CFG_UPGRADE_NOR_IMAGE_SIZE - CFG_NOR_RESERVED_SIZE - CFG_NOR_PARTITION0_SIZE - CFG_NOR_PARTITION1_SIZE - CFG_LFS_FAT_GAP &&
                                par.size[1] != CFG_UPGRADE_NOR_IMAGE_SIZE - CFG_NOR_RESERVED_SIZE - CFG_NOR_PARTITION0_SIZE - CFG_NOR_PARTITION1_SIZE - CFG_LFS_FAT_GAP)
                            goto repartition;
                    }
                    else
                    {
                        if (CFG_NOR_PARTITION2_SIZE && par.size[1] != CFG_NOR_PARTITION2_SIZE)
                            goto repartition;

                    #if !defined(CFG_NOR_PARTITION3_SIZE)
                        if (par.count != 2)
                        {
                            UG_LOG_INFO("Origial partition_1 count %d not equal %d for UPGRADE_LFS_FAT_ON_PARTITION0\n", par.count, 2);
                            goto repartition;
                        }

                        UG_LOG_INFO("Origial partition_1 %d size is %lld bytes\n", 2, par.size[2]);
                        if (par.size[2] != 0)
                            goto repartition;
                    #else
                        if (par.count != 3)
                        {
                            UG_LOG_INFO("Origial partition_1 count %d not equal %d for UPGRADE_LFS_FAT_ON_PARTITION0\n", par.count, 3);
                            goto repartition;
                        }

                        UG_LOG_INFO("Origial partition_1 %d size is %lld bytes\n", 2, par.size[2]);

                        if (CFG_NOR_PARTITION3_SIZE == 0)
                        {
                            if (CFG_UPGRADE_NOR_IMAGE_SIZE - CFG_NOR_RESERVED_SIZE - CFG_NOR_PARTITION0_SIZE - CFG_NOR_PARTITION1_SIZE - CFG_NOR_PARTITION2_SIZE - CFG_LFS_FAT_GAP &&
                                    par.size[2] != CFG_UPGRADE_NOR_IMAGE_SIZE - CFG_NOR_RESERVED_SIZE - CFG_NOR_PARTITION0_SIZE - CFG_NOR_PARTITION1_SIZE - CFG_NOR_PARTITION2_SIZE - CFG_LFS_FAT_GAP)
                                goto repartition;
                        }
                        else
                        {
                            if (CFG_NOR_PARTITION3_SIZE && par.size[2] != CFG_NOR_PARTITION3_SIZE)
                                goto repartition;
                        }
                    #endif
                    }
                #endif
                }
            #endif
        #else
            if(targetDev != targetDev2)
            {
                ret = ioctl(targetDev2, ITP_IOCTL_GET_PARTITION, &par_2);
                if (ret)
                {
                    UG_LOG_INFO("Origial partition_0 not exist for UPGRADE_LFS_FAT_ON_PARTITION0\n");
                    goto repartition;
                }
            }
            #if defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0)
                if(targetDev != targetDev2)
                {
                    if (par_2.count != 1)
                    {
                        UG_LOG_INFO("Origial partition_0 count %d not equal %d for UPGRADE_LFS_FAT_ON_PARTITION0\n", par_2.count, 1);
                        goto repartition;
                    }

                    UG_LOG_INFO("Origial partition_0 %d size is %lld bytes\n", 0, par_2.size[0]);

                    if (device->size[0] && par_2.size[0] != device->size[0])
                        goto repartition;
                }
                ret = ioctl(targetDev, ITP_IOCTL_GET_PARTITION, &par);
                if (ret)
                {
                    UG_LOG_INFO("Origial partition_1 not exist for UPGRADE_LFS_FAT_ON_PARTITION0\n");
                    goto repartition;
                }
                if (par.count != (int)device->partition_count - 1)
                {
                    UG_LOG_INFO("Origial partition_0 count %d not equal %d for UPGRADE_LFS_FAT_ON_PARTITION0\n", par.count, (int)device->partition_count - 1);
                    goto repartition;
                }

                if ((int)device->partition_count > 1)
                {
                    for (i = 1; i < (int)device->partition_count; i++)
                    {
                        UG_LOG_INFO("Origial partition_1 %d size is %lld bytes\n", i - 1, par.size[i - 1]);
                        if (device->size[i] && par.size[i - 1] != device->size[i])
                            goto repartition;
                    }
                }
            #else
                if (par_2.count != 2)
                {
                    UG_LOG_INFO("Origial partition_0 count %d not equal %d for UPGRADE_LFS_FAT_ON_PARTITION0_AND_1\n", par_2.count, 2);
                    goto repartition;
                }

                UG_LOG_INFO("Origial partition_0 %d size is %lld bytes\n", 0, par_2.size[0]);

                if (device->size[0] && par_2.size[0] != device->size[0])
                    goto repartition;

                UG_LOG_INFO("Origial partition_1 %d size is %lld bytes\n", 1, par_2.size[1]);

                if (device->size[1] && par_2.size[1] != device->size[1])
                    goto repartition;

                ret = ioctl(targetDev, ITP_IOCTL_GET_PARTITION, &par);
                if (ret)
                {
                    UG_LOG_INFO("Origial partition_2 not exist for UPGRADE_LFS_FAT_ON_PARTITION0_AND_1\n");
                    goto repartition;
                }

                if (par.count != (int)device->partition_count - 2)
                {
                    UG_LOG_INFO("Origial partition_0 count %d not equal %d for UPGRADE_LFS_FAT_ON_PARTITION0_AND_1\n", par.count, (int)device->partition_count - 2);
                    goto repartition;
                }

                if ((int)device->partition_count > 2)
                {
                    for (i = 2; i < (int)device->partition_count; i++)
                    {
                        UG_LOG_INFO("Origial partition_2 %d size is %lld bytes\n", i - 2, par.size[i - 2]);
                        if (device->size[i] && par.size[i - 2] != device->size[i])
                            goto repartition;
                    }
                }
            #endif
        #endif
    #endif
    }

    UG_LOG_INFO("Mount %s disk(s)...\n", device_table[device->device_type]);

#if (!defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0)) && (!defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))
	#ifdef CFG_FS_YAFFS2
	ret = ioctl(targetDev, ITP_IOCTL_MOUNT, (void*)par.disk);
    if (ret)
    {
        UG_LOG_ERR("Mount %s disk(s) fail: 0x%X\n", device_table[device->device_type], ret);
        goto end;
    }
	#else
        if(targetDev == ITP_DEVICE_FAT)
        {
            if ((int)device->partition_count > 3)
            {
                ret = ioctl(targetDev, ITP_IOCTL_MOUNT, (void*)par.disk);
                if (ret)
                {
                    UG_LOG_ERR("Mount %s disk(s) fail: 0x%X\n", device_table[device->device_type], ret);
                    goto end;
                }
            }
            else
            {
                ret = ioctl(targetDev, ITP_IOCTL_MOUNT_NOEXBR, (void*)par.disk);
                if (ret)
                {
                    UG_LOG_ERR("Mount_NOEXBR %s disk(s) fail: 0x%X\n", device_table[device->device_type], ret);
                    goto end;
                }
            }
        }
        else
        {
            ret = ioctl(targetDev, ITP_IOCTL_MOUNT, (void*)par.disk);
            if (ret)
            {
                UG_LOG_ERR("Mount %s disk(s) fail: 0x%X\n", device_table[device->device_type], ret);
                goto end;
            }
        }
	#endif
#else
    ret = ioctl(targetDev2, ITP_IOCTL_MOUNT, (void*)par_2.disk);
    if (ret)
    {
        UG_LOG_ERR("Mount %s disk(s) fail_0: 0x%X\n", device_table[device->device_type], ret);
        goto end;
    }

    ret = ioctl(targetDev, ITP_IOCTL_MOUNT, (void*)par.disk);
    if (ret)
    {
        UG_LOG_ERR("Mount %s disk(s) fail_1: 0x%X\n", device_table[device->device_type], ret);
        goto end;
    }
#endif
    goto end;

repartition:
    UG_LOG_INFO("Partitioning...\n");

#if (!defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0)) && (!defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))
    par.count = device->partition_count;
    inc = device->unformatted_size;
    for (i = 0; i < (int)device->partition_count; i++)
    {
        par.start[i] = inc;
        par.size[i] = device->size[i];
        inc += device->size[i];
    }
#else
    #if !defined(UG_DEVICE_PAR_INFO)
        inc = CFG_NOR_RESERVED_SIZE;
        par_2.start[0] = inc;

        #if !defined(CFG_NOR_PARTITION0_SIZE)
            par_2.size[0] = 0;
            par_2.count = 0;
        #else
            if (CFG_NOR_PARTITION0_SIZE == 0)
            {
                par_2.size[0] = CFG_UPGRADE_NOR_IMAGE_SIZE - CFG_NOR_RESERVED_SIZE;
            }
            else
            {
                par_2.size[0] = CFG_NOR_PARTITION0_SIZE;
            }
            par_2.count = 1;
        #endif

        inc = CFG_NOR_RESERVED_SIZE + CFG_NOR_PARTITION0_SIZE + CFG_LFS_FAT_GAP;
        par.start[0] = inc;

        #if !defined(CFG_NOR_PARTITION1_SIZE)
            par.size[0] = 0;
            par.count = 0;
        #else
            if (CFG_NOR_PARTITION1_SIZE == 0)
            {
                par.size[0] = CFG_UPGRADE_NOR_IMAGE_SIZE - CFG_NOR_RESERVED_SIZE - CFG_NOR_PARTITION0_SIZE - CFG_LFS_FAT_GAP;
                par.count = 1;
            }
            else
            {
                par.size[0] = CFG_NOR_PARTITION1_SIZE;
                inc += CFG_NOR_PARTITION1_SIZE;
                par.start[1] = inc;

            #if !defined(CFG_NOR_PARTITION2_SIZE)
                par.size[1] = 0;
                par.count = 1;
            #else
                if (CFG_NOR_PARTITION2_SIZE == 0)
                {
                    par.size[1] = CFG_UPGRADE_NOR_IMAGE_SIZE - CFG_NOR_RESERVED_SIZE - CFG_NOR_PARTITION0_SIZE - CFG_NOR_PARTITION1_SIZE - CFG_LFS_FAT_GAP;
                    par.count = 2;
                }
                else
                {
                    par.size[1] = CFG_NOR_PARTITION2_SIZE;
                    inc += CFG_NOR_PARTITION2_SIZE;
                    par.start[2] = inc;

                #if !defined(CFG_NOR_PARTITION3_SIZE)
                    par.size[2] = 0;
                    par.count = 2;
                #else
                    if (CFG_NOR_PARTITION3_SIZE == 0)
                    {
                        par.size[2] = CFG_UPGRADE_NOR_IMAGE_SIZE - CFG_NOR_RESERVED_SIZE - CFG_NOR_PARTITION0_SIZE - CFG_NOR_PARTITION1_SIZE - CFG_NOR_PARTITION2_SIZE - CFG_LFS_FAT_GAP;
                        par.count = 3;
                    }
                    else
                    {
                        par.size[2] = CFG_NOR_PARTITION3_SIZE;
                        par.count = 3;
                    }
                #endif
                }
            #endif
            }
        #endif
    #else
        if(targetDev == targetDev2)
        {
            par.count = device->partition_count;
            inc = device->unformatted_size;
            for (i = 0; i < (int)device->partition_count; i++)
            {
                par.start[i] = inc;
                par.size[i] = device->size[i];
                inc += device->size[i];
            }
        }
        else
        {
            par_2.count = 1;
            inc = device->unformatted_size;
            par_2.start[0] = inc;
            par_2.size[0] = device->size[0];

            #if defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0)
                inc += device->size[0] + CFG_LFS_FAT_GAP;

                par.count = 0;
                par.start[0] = 0;
                par.size[0] = 0;
                if ((int)device->partition_count > 1)
                {
                    par.count = (int)device->partition_count - 1;
                    for (i = 1; i < (int)device->partition_count; i++)
                    {
                        par.start[i - 1] = inc;
                        par.size[i - 1] = device->size[i];
                        inc += device->size[i];
                    }
                }
            #else
                par_2.count = 2;
                inc += device->size[0];
                par_2.start[1] = inc;
                par_2.size[1] = device->size[1];
                inc += device->size[1] + CFG_LFS_FAT_GAP;

                par.count = 0;
                par.start[0] = 0;
                par.size[0] = 0;
                if ((int)device->partition_count > 2)
                {
                    par.count = (int)device->partition_count - 2;
                    for (i = 2; i < (int)device->partition_count; i++)
                    {
                        par.start[i - 2] = inc;
                        par.size[i - 2] = device->size[i];
                        inc += device->size[i];
                    }
                }
            #endif
        }
    #endif
#endif

    UG_LOG_INFO("Unmount %s disk(s)...\n", device_table[device->device_type]);

#if (!defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0)) && (!defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))
    ret = ioctl(targetDev, ITP_IOCTL_UNMOUNT, (void*)par.disk);
    if (ret)
    {
        UG_LOG_ERR("Unmount %s disk(s) fail: 0x%X\n", device_table[device->device_type], ret);
    }
#else
    if(targetDev != targetDev2)
    {
        ret = ioctl(targetDev2, ITP_IOCTL_UNMOUNT, (void*)par_2.disk);
        if (ret)
        {
            UG_LOG_ERR("Unmount %s disk(s) fail_0: 0x%X\n", device_table[device->device_type], ret);
        }
    }

    ret = ioctl(targetDev, ITP_IOCTL_UNMOUNT, (void*)par.disk);
    if (ret)
    {
        UG_LOG_ERR("Unmount %s disk(s) fail_1: 0x%X\n", device_table[device->device_type], ret);
    }

#endif

    UG_LOG_INFO("Create partition(s)...\n");

#if (!defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0)) && (!defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))
    ret = ioctl(targetDev, ITP_IOCTL_CREATE_PARTITION, &par);
    if (ret)
    {
        UG_LOG_ERR("Create partition(s) fail: 0x%X\n", ret);
        goto end;
    }
#else
    if(targetDev != targetDev2)
    {
        ret = ioctl(targetDev2, ITP_IOCTL_CREATE_PARTITION, &par_2);
        if (ret)
        {
            UG_LOG_ERR("Create partition(s) fail_0: 0x%X\n", ret);
            goto end;
        }
    }
    printf("ITP_IOCTL_CREATE_PARTITION %d\r\n", par.count);

    ret = ioctl(targetDev, ITP_IOCTL_CREATE_PARTITION, &par);
    if (ret)
    {
        UG_LOG_ERR("Create partition(s) fail_1: 0x%X\n", ret);
        goto end;
    }

#endif

#if (!defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0)) && (!defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))
    #if defined(CFG_FS_LFS)
        if(targetDev == ITP_DEVICE_LFS)
        {
            for (i = 0; i < (int)device->partition_count; i++) {
                UG_LOG_INFO("Format partition %d @ %s disk(s)...\n", i, device_table[device->device_type]);
                ret = ioctl(targetDev, ITP_IOCTL_FORMAT_PARTITION, (void*) ((par.disk << 8) | i));
                if (ret)
                {
                    UG_LOG_INFO("Format partition %d @ %s disk(s) fail: 0x%X\n", i, device_table[device->device_type], ret);
                    goto end;
                }
            }

            UG_LOG_INFO("Mount %s disk(s)...\n", device_table[device->device_type]);
            ret = ioctl(targetDev, ITP_IOCTL_MOUNT, (void*)par.disk);
            if (ret)
            {
                UG_LOG_ERR("Mount %s disk(s) fail: 0x%X\n", device_table[device->device_type], ret);
            }
            goto end;
        }
    #endif
#endif

    UG_LOG_INFO("Mount %s disk(s)...\n", device_table[device->device_type]);

#if (!defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0)) && (!defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))
	#if defined(CFG_FS_YAFFS2)
	ret = ioctl(targetDev, ITP_IOCTL_FORMAT_MOUNT, (void*)par.disk);
    if (ret)
    {
        UG_LOG_ERR("Mount %s disk(s) fail: 0x%X\n", device_table[device->device_type], ret);
        goto end;
    }
	#else
    if (par.count > 3)
    {
        ret = ioctl(targetDev, ITP_IOCTL_FORCE_MOUNT, (void*)par.disk);
        if (ret)
        {
            UG_LOG_ERR("Mount %s disk(s) fail: 0x%X\n", device_table[device->device_type], ret);
            goto end;
        }
    }
    else
    {
        ret = ioctl(targetDev, ITP_IOCTL_FORCE_MOUNT_NOEXBR, (void*)par.disk);
        if (ret)
        {
            UG_LOG_ERR("Mount_NOEXBR %s disk(s) fail: 0x%X\n", device_table[device->device_type], ret);
            goto end;
        }
    }
	#endif
#else
    if(targetDev2 == ITP_DEVICE_FAT)
    {
        ret = ioctl(targetDev2, ITP_IOCTL_FORCE_MOUNT, (void*)par_2.disk);
        if (ret)
        {
            UG_LOG_ERR("Mount %s disk(s) fail_0: 0x%X\n", device_table[device->device_type], ret);
            goto end;
        }
    }
#endif

#if !defined(CFG_FS_YAFFS2)
    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    for (i = 0; i < ITP_MAX_DRIVE; i++)
    {
        ITPDriveStatus* driveStatus = &driveStatusTable[i];

        UG_LOG_DBG("drive[%d] disk=%d avail=%d\n", i, driveStatus->disk, driveStatus->avail);

        if (driveStatus->disk == par.disk && driveStatus->avail)
        {
            UG_LOG_INFO("Format drive %c: ...\n", 'A' + i);

        #if !defined(_WIN32) && defined(CFG_NOR_ENABLE) && !defined(CFG_FS_LFS)
            if (par.disk == ITP_DISK_NOR && !fat_exbr_drive_find)
            {
                fat_exbr_drive = 'A' + i;
                fat_exbr_drive_find = true;
            }
        #endif

        #if (!defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0)) && (!defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))
            ret = ioctl(targetDev, ITP_IOCTL_FORMAT, (void*) i);
            if (ret)
            {
                UG_LOG_ERR("Format fail: 0x%X\n", ret);
                goto end;
            }
        #else
            if(targetDev2 == ITP_DEVICE_FAT)
            {
                printf("format %d\r\n", i);
                ret = ioctl(targetDev2, ITP_IOCTL_FORMAT, (void*) i);
                if (ret)
                {
                    UG_LOG_ERR("Format fail_0: 0x%X\n", ret);
                    goto end;
                }
            }
        #endif
        }
    }
#endif

#if !defined(_WIN32) && defined(CFG_NOR_ENABLE) && !defined(CFG_FS_LFS)
    if (par.disk == ITP_DISK_NOR && par.count > 3 && fat_exbr_drive_find)
    {
    #if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
        ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
    #endif

        if (NorRead(SPI_0, NOR_CSN, (uint32_t)(par.exbr_start), fat_exbr + 8, ITP_FAT_EXBR_SIZE - 8))
        {
            UG_LOG_ERR("NorRead fail.\n");
        }
        else
        {
            memcpy(fat_exbr, &par.exbr_start, 8);

            /*for (i = 0; i < ITP_FAT_EXBR_SIZE; i++)
                UG_LOG_INFO("%x ", fat_exbr[i]);
            UG_LOG_INFO("\n");*/

            sprintf((char *)fat_exbr_path, "%c:/%s", fat_exbr_drive, ITP_FAT_EXBR_FILE);
            UG_LOG_INFO("write file %s...\n", fat_exbr_path);

            f = fopen(fat_exbr_path, "wb");
            if (!f)
            {
                UG_LOG_ERR("fopen fail\n");
            }
            else
            {
                if (fwrite(fat_exbr, 1, ITP_FAT_EXBR_SIZE, f) != ITP_FAT_EXBR_SIZE)
                    UG_LOG_ERR("fwrite fail\n");
                fclose(f);
            }
        }
    }
#endif

#if (defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0)) || (defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))
    if(targetDev == ITP_DEVICE_LFS)
    {
        for (i = 0; i < par.count; i++) {
            UG_LOG_INFO("Format partition_1 %d @ %s disk(s)...\n", i, device_table[device->device_type]);
            ret = ioctl(targetDev, ITP_IOCTL_FORMAT_PARTITION, (void*) ((par.disk << 8) | i));
            if (ret)
            {
                UG_LOG_INFO("Format partition_1 %d @ %s disk(s) fail: 0x%X\n", i, device_table[device->device_type], ret);
                goto end;
            }
        }

        UG_LOG_INFO("Mount_0 %s disk(s)...\n", device_table[device->device_type]);
        ret = ioctl(targetDev, ITP_IOCTL_MOUNT, (void*)par.disk);
        if (ret)
        {
            UG_LOG_ERR("Mount %s disk(s) fail_0: 0x%X\n", device_table[device->device_type], ret);
        }
    }
#endif

    UG_LOG_INFO("Partition finished.\n");

#ifdef CFG_UPGRADE_BACKUP_FAT_MBR
    uint32_t mbr_pos = 0;
#if defined(CFG_NAND_PARTITION0)
    if (par.disk == ITP_DISK_NAND)
    {
        mbr_pos = CFG_NAND_RESERVED_SIZE;
    }
#endif
#if defined(CFG_NOR_PARTITION0)
    if (par.disk == ITP_DISK_NOR)
    {
#if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
        ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
#endif
        mbr_pos = CFG_NOR_RESERVED_SIZE;
    }
#endif
#if defined(CFG_SD0_PARTITION0)
    if (par.disk == ITP_DISK_SD0)
    {
        mbr_pos = CFG_SD0_RESERVED_SIZE;
    }
#endif
#if defined(CFG_SD1_PARTITION0)
    if (par.disk == ITP_DISK_SD1)
    {
        mbr_pos = CFG_SD1_RESERVED_SIZE;
    }
#endif
    UG_LOG_INFO("Start copy %s disk MBR to backup area.\n", device_table[device->device_type]);
    ret = ugCheckFatMBR(par.disk,
                mbr_pos,
                0x40000,
                true,
                false);
    if (ret)
    {
        UG_LOG_ERR("Backup FAT MBR %s disk(s) fail_0: 0x%X\n", device_table[device->device_type], ret);
    }
#endif
#if (defined(CFG_BUILD_LEVELX)) && (defined(CFG_BUILD_LEVELX))
    for (i = 0; i < ITP_MAX_DRIVE; i++)
    {
        ITPDriveStatus* driveStatus = &driveStatusTable[i];

        if (driveStatus->disk == par.disk && driveStatus->avail)
        {
            UG_LOG_INFO("Disable drive %c: write protect\n", 'A' + i);

            ITP_FAT_SET_PARAM param = {0};
            param.volume = i;
            param.param1 = 0;   // disable write protect
            ret = ioctl(targetDev, ITP_IOCTL_SET_WRITE_PROTECT, (void*)&param);
            if (ret)
            {
                UG_LOG_ERR("Set write protect fail: 0x%X\n", ret);
                goto end;
            }
        }
    }
#endif
end:
#ifdef _WIN32
    ret = 0;
#endif
    return ret;
}
