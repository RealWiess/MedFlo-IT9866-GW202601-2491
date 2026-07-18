/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL drive functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include <errno.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "openrtos/FreeRTOS.h"
#include "openrtos/queue.h"
#include "itp_cfg.h"
#include "ite/ite_sd.h"
#if defined(CFG_A_B_DUAL_BOOT_ENABLE) && defined(CFG_DYNAMIC_MOUNT_FS_ENABLE)
#include "ite/ug.h"
#endif

#define QUEUE_LEN               16
#define PROBE_POLLING_INTERVAL      (100 * 1000)  //100ms
#define PROBE_POLLING_TIMEOUT_COUNT (200)    

#ifdef CFG_UPGRADE_BACKUP_RAW_DATA
bool driveInited = false;
#endif

static ITPDriveStatus driveStatusTable[ITP_MAX_DRIVE];
static bool driveProbeEn;
static bool driveQuit;
static bool driveDiskAvail[ITP_CARD_MAX];
static QueueHandle_t driveQueue;

#ifdef CFG_TASK_DRIVE_PROBE
    static pthread_t driveProbeTask;
    static sem_t driveProbeSem;
#endif

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

static void DriveMountDisk(ITPDisk disk)
{
    int writable, removable;
    int ret = -1, i;

#if (!defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0)) && (!defined(CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1))
#ifdef CFG_FS_YAFFS2
	if(disk == ITP_DISK_NAND)
	{
		ret = ioctl(ITP_DEVICE_YAFFS2, ITP_IOCTL_MOUNT, (void*) disk);
	}
	else
	{
#ifdef CFG_FS_FAT
		ret = ioctl(ITP_DEVICE_FAT, ITP_IOCTL_MOUNT, (void*) disk);
#endif
	}
#else
#ifdef CFG_FS_FAT
    ret = ioctl(ITP_DEVICE_FAT, ITP_IOCTL_MOUNT, (void*) disk);
#endif
#ifdef CFG_FS_LFS
    if (ret)
    {
        ret = ioctl(ITP_DEVICE_LFS, ITP_IOCTL_MOUNT, (void*) disk);
    }
#endif
#endif
#else
    #ifdef CFG_FS_FAT
    ret = ioctl(ITP_DEVICE_FAT, ITP_IOCTL_MOUNT, (void*) disk);
    #endif
    #ifdef CFG_FS_YAFFS2
	ret = ioctl(ITP_DEVICE_YAFFS2, ITP_IOCTL_MOUNT, (void*) disk);
    #endif
    #ifdef CFG_FS_LFS
    ret = ioctl(ITP_DEVICE_LFS, ITP_IOCTL_MOUNT, (void*) disk);
    #endif
#endif

    switch (disk)
    {
#ifdef CFG_SD0_ENABLE
    case ITP_DISK_SD0:
    #ifdef CFG_SD0_STATIC
        writable    = true;
        removable   = false;
    #else
        writable    = !ithCardLocked(ITH_CARDPIN_SD0);
        removable   = true;
    #endif // CFG_SD0_STATIC
        break;
#endif // CFG_SD0_ENABLE

#ifdef CFG_SD1_ENABLE
    case ITP_DISK_SD1:
    #ifdef CFG_SD1_STATIC
        writable    = true;
        removable   = false;
    #else
        writable    = !ithCardLocked(ITH_CARDPIN_SD1);
        removable   = true;
    #endif // CFG_SD0_STATIC
        break;
#endif // CFG_SD0_ENABLE

#if defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)
        case ITP_DISK_MSC00:
        case ITP_DISK_MSC01:
        case ITP_DISK_MSC02:
        case ITP_DISK_MSC03:
        case ITP_DISK_MSC04:
        case ITP_DISK_MSC05:
        case ITP_DISK_MSC06:
        case ITP_DISK_MSC07:
        case ITP_DISK_MSC10:
        case ITP_DISK_MSC11:
        case ITP_DISK_MSC12:
        case ITP_DISK_MSC13:
        case ITP_DISK_MSC14:
        case ITP_DISK_MSC15:
        case ITP_DISK_MSC16:
        case ITP_DISK_MSC17:
            writable = true;
            removable = true;
            break;
#endif // defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)
#if defined(CFG_BUILD_LEVELX)
        case ITP_DISK_NOR:
            writable = true;
            removable = false;
            break;
#endif
    default:
        writable = true;
        removable = false;
    }

    for (i = 0; i < ITP_MAX_DRIVE; i++)
    {
        ITPDriveStatus* driveStatus = &driveStatusTable[i];

        if (driveStatus->avail && driveStatus->disk == disk)
        {
#if defined(CFG_BUILD_LEVELX)
            if(disk != ITP_DISK_NOR)       
#endif
            driveStatus->writable   = writable;
            driveStatus->removable  = removable;
            ITP_LOG_INFO(
                "mount %c: disk=%s dev=%d name=%s\n", 'A' + i, disk_table[driveStatus->disk], driveStatus->device >> ITP_DEVICE_BIT, driveStatus->name
            );
        }
    }
}

static void DriveUnmountDisk(ITPDisk disk)
{
	ioctl(ITP_DEVICE_FAT, ITP_IOCTL_UNMOUNT, (void*)disk);
}

#if defined(CFG_SD0_ENABLE) || defined(CFG_SD1_ENABLE) || defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)

static void DriveMountCard(ITPCardStatus* cardStatus)
{
    // SD0 and MS and xD are pin shared, need to try to init them to determine which one is inserted
#if defined(CFG_SD0_ENABLE) && !defined(CFG_SD0_STATIC) && !defined(CFG_SDIO0_STATIC)
    if ((cardStatus->card & ITP_CARD_SD0))
    {
#if 1//defined(CFG_MMC_ENABLE)
        SD_CARD_INFO card_info = { 0 };
        int retry = 3;

        while (iteSdcInitialize(SD_0, &card_info) && --retry);
		if (retry <= 0)
            return;

        #if defined(CFG_SDIO_ENABLE)
        if (card_info.type >= SD_TYPE_SDIO)
            ioctl(ITP_DEVICE_SDIO, ITP_IOCTL_ON, &card_info);
        #endif

        if (card_info.type == SD_TYPE_SDIO)
            return;
#endif
        if (!driveDiskAvail[ITP_DISK_SD0] && iteSdInitializeEx(0) == 0)
        {
            // inserted card is SD0
            DriveMountDisk(ITP_DISK_SD0);
            driveDiskAvail[ITP_DISK_SD0] = true;
            return;
        }
    }
#endif // defined(CFG_SD0_ENABLE) && !defined(CFG_SD0_STATIC)

#if defined(CFG_SD1_ENABLE) && !defined(CFG_SD1_STATIC) && !defined(CFG_SDIO1_STATIC)
    if ((cardStatus->card & ITP_CARD_SD1))
    {
#if 1//defined(CFG_MMC_ENABLE)
        SD_CARD_INFO card_info = { 0 };
        int retry = 3;

        while(iteSdcInitialize(SD_1, &card_info) && retry--);
		if (retry == 0)
            return;

        #if defined(CFG_SDIO_ENABLE)
        if (card_info.type >= SD_TYPE_SDIO)
            ioctl(ITP_DEVICE_SDIO, ITP_IOCTL_ON, &card_info);
        #endif

        if (card_info.type == SD_TYPE_SDIO)
            return;
#endif
        if (!driveDiskAvail[ITP_DISK_SD1] && iteSdInitializeEx(1) == 0)
        {
            // inserted card is SD1
            DriveMountDisk(ITP_DISK_SD1);
            driveDiskAvail[ITP_DISK_SD1] = true;
            return;
        }
    }
#endif // defined(CFG_SD1_ENABLE) && !defined(CFG_SD1_STATIC)

#if defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)
    uint32_t cardMsc = ITP_CARD_MSC00;
    uint32_t diskMsc = ITP_DISK_MSC00;
    do 	{
        if (cardStatus->card & cardMsc)
        {
            if (!driveDiskAvail[diskMsc])
            {
                DriveMountDisk(diskMsc);
                driveDiskAvail[diskMsc] = true;
                return;
            }
        }
        if(cardMsc == ITP_CARD_MSC17)
            break;
        cardMsc <<= 1;
        diskMsc++;
    } while(1);
#endif // defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)
}

static void DriveUnmountCard(ITPCardStatus* cardStatus)
{
#if defined(CFG_SD0_ENABLE) && !defined(CFG_SD0_STATIC)
    if ((cardStatus->card & ITP_CARD_SD0))
    {
        if (driveDiskAvail[ITP_DISK_SD0])
        {
            // inserted card is SD0
            DriveUnmountDisk(ITP_DISK_SD0);
            driveDiskAvail[ITP_DISK_SD0] = false;
            return;
        }
    }
#endif // defined(CFG_SD0_ENABLE) && !defined(CFG_SD0_STATIC)

#if defined(CFG_SD1_ENABLE) && !defined(CFG_SD1_STATIC)
    if ((cardStatus->card & ITP_CARD_SD1))
    {
        if (driveDiskAvail[ITP_DISK_SD1])
        {
            // inserted card is SD1
            DriveUnmountDisk(ITP_DISK_SD1);
            driveDiskAvail[ITP_DISK_SD1] = false;
            return;
        }
    }
#endif // defined(CFG_SD1_ENABLE) && !defined(CFG_SD1_STATIC)

#if defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)
    uint32_t cardMsc = ITP_CARD_MSC00;
    uint32_t diskMsc = ITP_DISK_MSC00;
    do {
        if ((cardStatus->card & cardMsc))
        {
            if (driveDiskAvail[diskMsc])
            {
                DriveUnmountDisk(diskMsc);
                driveDiskAvail[diskMsc] = false;
                return;
            }
        }
        if(cardMsc == ITP_CARD_MSC17)
            break;
        cardMsc <<= 1;
        diskMsc++;
    } while(1);
#endif // defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)
}

#endif // defined(CFG_SD0_ENABLE) || defined(CFG_SD1_ENABLE) || defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)

static void DriveProbe(ITPCardStatus* cardStatus)
{
    int i;
    int timeout = PROBE_POLLING_TIMEOUT_COUNT;
#if defined(CFG_SD0_ENABLE) || defined(CFG_SD1_ENABLE)
    int sd_index = -1;
    uint32_t delay_ms = 0;
#endif
    if (driveQuit)
        return;
    while(!driveProbeEn)    // For race condition with backup flow.
    {
        usleep(PROBE_POLLING_INTERVAL);
        if(timeout -- <= 0)
        {
            ITP_LOG_WARN("enable timeout\n");
            return;
        }
    }

#if defined(CFG_SD0_ENABLE) || defined(CFG_SD1_ENABLE) || defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)

#if defined(CFG_SD0_ENABLE) || defined(CFG_SD1_ENABLE)
    if (cardStatus->card == ITP_CARD_SD0) {
        sd_index = SD_0;
        #if defined(CFG_SD0_DELAY_TIME)
        delay_ms = CFG_SD0_DELAY_TIME;
        #endif
    }
    if (cardStatus->card == ITP_CARD_SD1) {
        sd_index = SD_1;
        #if defined(CFG_SD1_DELAY_TIME)
        delay_ms = CFG_SD1_DELAY_TIME;
        #endif
    }

    if ((sd_index >= SD_0) && cardStatus->inserted) {
        SD_CARD_INFO card_info = { 0 };
        int retry = 3;

        usleep(delay_ms * 1000);
        while (iteSdcInitialize(sd_index, &card_info) && --retry);
        if (retry <= 0)
            return;

        #if defined(CFG_SDIO_ENABLE)
        if (card_info.type >= SD_TYPE_SDIO)
            ioctl(ITP_DEVICE_SDIO, ITP_IOCTL_ON, &card_info);
        #endif

        if (card_info.type == SD_TYPE_SDIO)
            return;
    }
    #if defined(CFG_SDIO_ENABLE)
    if ((sd_index >= SD_0) && !cardStatus->inserted) {
        if (iteSdcGetType(sd_index) == SD_TYPE_SDIO)
            goto sdio_exit;
    }
    #endif
#endif

    // detect inserted/removed of cards
    if (cardStatus->inserted)
        DriveMountCard(cardStatus);
    else
        DriveUnmountCard(cardStatus);

#if defined(CFG_SD0_ENABLE) || defined(CFG_SD1_ENABLE)
    if ((sd_index >= SD_0) && !cardStatus->inserted) {
        #if defined(CFG_SDIO_ENABLE)
sdio_exit:
        if (iteSdcGetType(sd_index) >= SD_TYPE_SDIO)
            ioctl(ITP_DEVICE_SDIO, ITP_IOCTL_OFF, (void*)sd_index);
        #endif

        iteSdcTerminate(sd_index);
    }
#endif

#endif // defined(CFG_SD0_ENABLE) || defined(CFG_SD1_ENABLE) || defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)
}

static void DriveMount(ITPDisk* disks)
{
    int i;
    ITPCardStatus* cardStatusTable = {0};

    if (disks)
    {
        int j = 0;
        while (disks[j] != -1)
        {
        #if defined(CFG_NOR_ENABLE) && defined(CFG_NOR_PARTITION0)
            if (disks[j] == ITP_DISK_NOR)
                DriveMountDisk(ITP_DISK_NOR);
        #endif

        #if defined(CFG_NAND_ENABLE) && defined(CFG_NAND_PARTITION0)
            if (disks[j] == ITP_DISK_NAND)
                DriveMountDisk(ITP_DISK_NAND);
        #endif

        #if defined(CFG_SD0_STATIC) && defined(CFG_SD0_PARTITION0)
            if (disks[j] == ITP_DISK_SD0)
                DriveMountDisk(ITP_DISK_SD0);
        #endif

        #if defined(CFG_SD1_STATIC) && defined(CFG_SD1_PARTITION0)
            if (disks[j] == ITP_DISK_SD1)
                DriveMountDisk(ITP_DISK_SD1);
        #endif

        #ifdef CFG_RAMDISK_ENABLE
            if (disks[j] == ITP_DISK_RAM)
                DriveMountDisk(ITP_DISK_RAM);
        #endif

            j++;
        }

    #if defined(CFG_SD0_ENABLE) || defined(CFG_SD1_ENABLE) || defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)

        ioctl(ITP_DEVICE_CARD, ITP_IOCTL_GET_TABLE, (void*)&cardStatusTable);

        for (i = 0; i < ITP_CARD_MAX; i++)
        {
            ITPCardStatus* cardStatus = &cardStatusTable[i];

            if (cardStatus->inserted)
            {
                int j = 0;
                while (disks[j] != -1)
                {
                    if (cardStatus->card & (0x1 << disks[j]))
                        DriveMountCard(cardStatus);

                    j++;
                }
            }
        }
    #endif // defined(CFG_SD0_ENABLE) || defined(CFG_SD1_ENABLE) || defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)
    }
    else
    {
    #if defined(CFG_NOR_ENABLE) && defined(CFG_NOR_PARTITION0)
        DriveMountDisk(ITP_DISK_NOR);
    #endif

    #if defined(CFG_NAND_ENABLE) && defined(CFG_NAND_PARTITION0)
        DriveMountDisk(ITP_DISK_NAND);
    #endif

    #if defined(CFG_SD0_STATIC) && defined(CFG_SD0_PARTITION0)
        DriveMountDisk(ITP_DISK_SD0);
    #endif

    #if defined(CFG_SD1_STATIC) && defined(CFG_SD1_PARTITION0)
        DriveMountDisk(ITP_DISK_SD1);
    #endif

    #ifdef CFG_RAMDISK_ENABLE
        DriveMountDisk(ITP_DISK_RAM);
    #endif

    #if defined(CFG_SD0_ENABLE) || defined(CFG_SD1_ENABLE) || defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)

        ioctl(ITP_DEVICE_CARD, ITP_IOCTL_GET_TABLE, (void*)&cardStatusTable);

        for (i = 0; i < ITP_CARD_MAX; i++)
        {
            ITPCardStatus* cardStatus = &cardStatusTable[i];

            if (cardStatus->inserted)
                DriveMountCard(cardStatus);
        }
    #endif // defined(CFG_SD0_ENABLE) || defined(CFG_SD1_ENABLE) || defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)
    }

#ifdef CFG_TASK_DRIVE_PROBE
    sem_post(&driveProbeSem);
#endif
}

static void DriveUnmount(ITPDisk* disks)
{
    int i;
    ITPCardStatus* cardStatusTable;

    if (disks)
    {
        for (i = 0; i < ITP_MAX_DRIVE; i++)
        {
            ITPDriveStatus* driveStatus = &driveStatusTable[i];

            if (driveStatus->avail)
            {
                int j = 0;
                while (disks[j] != -1)
                {
                    if (disks[j] == driveStatus->disk)
                        DriveUnmountDisk(driveStatus->disk);

                    j++;
                }
            }
        }

     #if defined(CFG_SD0_ENABLE) || defined(CFG_SD1_ENABLE) || defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)

        ioctl(ITP_DEVICE_CARD, ITP_IOCTL_GET_TABLE, (void*)&cardStatusTable);

        for (i = 0; i < ITP_CARD_MAX; i++)
        {
            ITPCardStatus* cardStatus = &cardStatusTable[i];

            if (cardStatus->inserted)
            {
                int j = 0;
                while (disks[j] != -1)
                {
                    if (cardStatus->card & (0x1 << disks[j]))
                        DriveUnmountCard(cardStatus);

                    j++;
                }
            }
        }
    #endif // defined(CFG_SD0_ENABLE) || defined(CFG_SD1_ENABLE) || defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)
    }
    else
    {
        for (i = 0; i < ITP_MAX_DRIVE; i++)
        {
            ITPDriveStatus* driveStatus = &driveStatusTable[i];

            if (driveStatus->avail)
                DriveUnmountDisk(driveStatus->disk);
        }

    #if defined(CFG_SD0_ENABLE) || defined(CFG_SD1_ENABLE) || defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)

        ioctl(ITP_DEVICE_CARD, ITP_IOCTL_GET_TABLE, (void*)&cardStatusTable);

        for (i = 0; i < ITP_CARD_MAX; i++)
        {
            ITPCardStatus* cardStatus = &cardStatusTable[i];

            if (cardStatus->inserted)
                DriveUnmountCard(cardStatus);
        }
    #endif // defined(CFG_SD0_ENABLE) || defined(CFG_SD1_ENABLE) || defined(CFG_MSC_ENABLE) || defined(CFG_USBH_CD_MST)
    }
}

#ifdef CFG_TASK_DRIVE_PROBE

static void* DriveProbeTask(void* arg)
{
    sem_wait(&driveProbeSem);

    // mount/unmount cards when it's inserted/removed
    while (!driveQuit)
    {
        ITPCardStatus cardStatus;

        if (read(ITP_DEVICE_CARD, &cardStatus, sizeof (ITPCardStatus)) == sizeof (ITPCardStatus))
            DriveProbe(&cardStatus);
    }

    return NULL;
}

#endif // CFG_TASK_DRIVE_PROBE

static void DriveInit(void)
{
    int i;

    driveQueue = xQueueCreate(QUEUE_LEN, (unsigned portBASE_TYPE) sizeof(ITPDriveStatus));

    for (i = 0; i < ITP_MAX_DRIVE; i++)
    {
        ITPDriveStatus* driveStatus = &driveStatusTable[i];

        driveStatus->disk      = -1;
        driveStatus->device    = -1;
        driveStatus->avail     = false;
    }

    driveQuit = false;

#ifdef CFG_TASK_DRIVE_PROBE
    sem_init(&driveProbeSem, 0, 0);
#endif
#if defined(CFG_A_B_DUAL_BOOT_ENABLE) && defined(CFG_DYNAMIC_MOUNT_FS_ENABLE)
    uint8_t mount_flag = 0x0U;
    if(ugBootargsRead(&mount_flag))
    {
        (void)printf("###Read boot args failed\n");
    }else
    {
        (void)printf("###Read boot args(0x%x)\n", mount_flag);
    }
    ioctl(ITP_DEVICE_FAT, ITP_IOCTL_MOUNT_DUAL_AB, (void*)&mount_flag);
#endif

}

static void DriveExit(void)
{
    driveQuit = true;

#ifdef CFG_TASK_DRIVE_PROBE
    ioctl(ITP_DEVICE_CARD, ITP_IOCTL_EXIT, NULL);
    pthread_join(driveProbeTask, NULL);
    sem_destroy(&driveProbeSem);
#endif
    vQueueDelete(driveQueue);
}

static int DriveOpen(const char* name, int flags, int mode, void* info)
{
    if (flags & O_NONBLOCK)
        return 1;

    return 0;
}

static int DriveRead(int file, char *ptr, int len, void* info)
{
    if (driveQuit || (len < sizeof (ITPDriveStatus)))
        return 0;

    if (xQueueReceive(driveQueue, ptr, file ? 0 : portMAX_DELAY))
        return sizeof (ITPDriveStatus);

    return 0;
}

static int DriveWrite(int file, char *ptr, int len, void* info)
{
    if (!driveQuit && (len == sizeof (ITPDriveStatus)))
    {
        if (xQueueSend(driveQueue, ptr, 0) == pdTRUE)
            return sizeof (ITPDriveStatus);
    }
    return 0;
}

static int DriveIoctl(int file, unsigned long request, void* ptr, void* info)
{
    switch (request)
    {
    case ITP_IOCTL_INIT:
        DriveInit();
#ifdef CFG_UPGRADE_BACKUP_RAW_DATA
        driveInited = true;
#endif
        break;

    case ITP_IOCTL_EXIT:
        DriveExit();
#ifdef CFG_UPGRADE_BACKUP_RAW_DATA
        driveInited = false;
#endif
        break;

    case ITP_IOCTL_ENABLE:
        driveProbeEn = true;
        break;

    case ITP_IOCTL_DISABLE:
        driveProbeEn = false;
        break;

    case ITP_IOCTL_MOUNT:
        DriveMount((ITPDisk*)ptr);
        break;

    case ITP_IOCTL_UNMOUNT:
        DriveUnmount((ITPDisk*)ptr);
        break;

    case ITP_IOCTL_PROBE:
        DriveProbe((ITPCardStatus*) ptr);
        break;

    case ITP_IOCTL_GET_TABLE:
        *(ITPDriveStatus**)ptr = driveStatusTable;
        break;

#ifdef CFG_TASK_DRIVE_PROBE
    case ITP_IOCTL_INIT_TASK:
        {
            // create drive probe task
            pthread_attr_t attr;        
            pthread_attr_init(&attr);
            attr.stacksize = 128 * 1024;            
            pthread_create(&driveProbeTask, &attr, DriveProbeTask, NULL);
        }
        break;
#endif // CFG_TASK_DRIVE_PROBE

    default:
        errno = (ITP_DEVICE_DRIVE << ITP_DEVICE_ERRNO_BIT) | 1;
        return -1;
    }
    return 0;
}

const ITPDevice itpDeviceDrive =
{
    ":drive",
    DriveOpen,
    itpCloseDefault,
    DriveRead,
    DriveWrite,
    itpLseekDefault,
    DriveIoctl,
    NULL
};
