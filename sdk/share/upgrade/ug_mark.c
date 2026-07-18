#include "ite/ug.h"
#include "ug_cfg.h"
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#ifndef CFG_UPGRADE_MARK_POS
    #define CFG_UPGRADE_MARK_POS 0
#endif

#ifndef CFG_BOOT_ARGS_POS
    #define CFG_BOOT_ARGS_POS 0
#endif

#if CFG_UPGRADE_MARK_POS

static const char ugMarkBackuping[] = {'B', 'A', 'C', 'K', 'U', 'P', 'I', 'N', 'G'};
static const char ugMarkUpgrading[] = {'U', 'P', 'G', 'R', 'A', 'D', 'I', 'N', 'G'};

    #ifdef CFG_UPGRADE_BACKUP_RAW_DATA
static const char ugMarkUpgradingFW[]      = {'F', 'W', 'U', 'P', 'G', 'R', 'A', 'D', 'I', 'N', 'G'};
static const char ugMarkUpgradingFATP0[]   = {'F', 'A', 'T', 'P', '0', 'U', 'P', 'G', 'R', 'A', 'D', 'I', 'N', 'G'};
static const char ugMarkUpgradingFATP1[]   = {'F', 'A', 'T', 'P', '1', 'U', 'P', 'G', 'R', 'A', 'D', 'I', 'N', 'G'};
static const char ugMarkUpgradingFATP2[]   = {'F', 'A', 'T', 'P', '2', 'U', 'P', 'G', 'R', 'A', 'D', 'I', 'N', 'G'};
static const char ugMarkUpgradingFATP3[]   = {'F', 'A', 'T', 'P', '3', 'U', 'P', 'G', 'R', 'A', 'D', 'I', 'N', 'G'};
static const char ugMarkUpgradingFINISH[]  = {'F', 'I', 'N', 'I', 'S', 'H'};
static const char ugMarkUpgradingUNKNOWN[] = {'U', 'N', 'K', 'N', 'O', 'W', 'N'};
    #endif

static int ugMarkFD = -1;
static uint32_t ugMarkBlockSize, ugMarkPos;
static uint8_t* ugMarkBuf = NULL;
static bool ugBackupPackageFinished;

/**
 * Exit the ugMark module.
 *
 * Frees the memory occupied by ugMarkBuf and closes the file descriptor ugMarkFD.
 */
static void ugMarkExit(void)
{
    if (ugMarkBuf != NULL)
    {
        free(ugMarkBuf);
        ugMarkBuf = NULL;
    }

    if (ugMarkFD != -1)
    {
        close(ugMarkFD);
        ugMarkFD = -1;
    }
}

/**
 * Initialize the ugMark module.
 *
 * This function opens the appropriate device file based on the configuration.
 * It also retrieves the mark value from the appropriate device file.
 *
 * @return 0 on success, error code otherwise.
 */
static int ugMarkInit(void)
{
    int ret = 0;

    assert(ugMarkFD == -1);

    #ifndef CFG_UPGRADE_BACKUP_RAW_DATA

        #if defined(CFG_UPGRADE_IMAGE_NAND)
    ugMarkFD = open(":nand", O_RDONLY, 0);
    UG_LOG_DBG("nand fd: 0x%X\n", ugMarkFD);
        #elif defined(CFG_UPGRADE_IMAGE_NOR)
    ugMarkFD = open(":nor", O_RDONLY, 0);
    UG_LOG_DBG("nor fd: 0x%X\n", ugMarkFD);
        #elif defined(CFG_UPGRADE_IMAGE_SD0)
    ugMarkFD = open(":sd0", O_RDONLY, 0);
    UG_LOG_DBG("sd0 fd: 0x%X\n", ugMarkFD);
        #elif defined(CFG_UPGRADE_IMAGE_SD1)
    ugMarkFD = open(":sd1", O_RDONLY, 0);
    UG_LOG_DBG("sd1 fd: 0x%X\n", ugMarkFD);
        #else
            #error "unknown device."
        #endif

    #else

        #if defined(CFG_NAND_PARTITION0)
    ugMarkFD = open(":nand", O_RDONLY, 0);
    UG_LOG_DBG("nand fd: 0x%X\n", ugMarkFD);
        #elif defined(CFG_NOR_PARTITION0)
    ugMarkFD = open(":nor", O_RDONLY, 0);
    UG_LOG_DBG("nor fd: 0x%X\n", ugMarkFD);
        #elif defined(CFG_SD0_PARTITION0)
    ugMarkFD = open(":sd0", O_RDONLY, 0);
    UG_LOG_DBG("sd0 fd: 0x%X\n", ugMarkFD);
        #elif defined(CFG_SD1_PARTITION0)
    ugMarkFD = open(":sd1", O_RDONLY, 0);
    UG_LOG_DBG("sd1 fd: 0x%X\n", ugMarkFD);
        #else
            #error "unknown device."
        #endif

    #endif

    // Check if the device open was successful
    if (ugMarkFD == -1)
    {
        UG_LOG_ERR("open device error\n");
        ret = __LINE__;
        goto end;
    }

    if (ioctl(ugMarkFD, ITP_IOCTL_GET_BLOCK_SIZE, &ugMarkBlockSize))
    {
        UG_LOG_ERR("get block size error\n");
        ret = __LINE__;
        goto end;
    }
    // Calculate the mark position
    ugMarkPos = CFG_UPGRADE_MARK_POS / ugMarkBlockSize;

    UG_LOG_INFO("ugMarkFD=0x%X ugMarkBlockSize=%d ugMarkPos=0x%X\n", ugMarkFD, ugMarkBlockSize,
        ugMarkPos);

    // Seek to the mark position
    if (lseek(ugMarkFD, ugMarkPos, SEEK_SET) != ugMarkPos)
    {
        UG_LOG_ERR("seek to mark position %d(%d) error\n", CFG_UPGRADE_MARK_POS, ugMarkPos);
        ret = __LINE__;
        goto end;
    }

    // Ensure the block size is at least 8 bytes
    assert(ugMarkBlockSize >= 8);
    // Allocate memory for the mark buffer
    ugMarkBuf = (uint8_t*)malloc(ugMarkBlockSize);
    if (!ugMarkBuf) // Check if memory allocation was successful
    {
        UG_LOG_ERR("out of memory %d\n", ugMarkBlockSize);
        ret = __LINE__;
        goto end;
    }

    // Read 1 byte from the device into the mark buffer
    if (read(ugMarkFD, ugMarkBuf, 1) != 1)
    {
        UG_LOG_ERR("read mark error: %d != 1\n", ret);
        ret = __LINE__;
        goto end;
    }

end:
    if (ret)
        ugMarkExit();   // Clean up and exit the function if there was an error

    return ret;
}
#endif // CFG_UPGRADE_MARK_POS

/**
 * Determine the last upgrade state depending on the mark's value.
 *
 * @return The last upgrade state
 */
#ifndef CFG_UPGRADE_BACKUP_RAW_DATA
int ugUpgradeCheck(void)
#else
UG_UPGRADE_FLOW_TAG ugUpgradeCheck(void)
#endif
{
    int ret = 0;
#if CFG_UPGRADE_MARK_POS

    #ifdef CFG_UPGRADE_BACKUP_RAW_DATA
    // Initialize the ugMark module.
    // Load the mark'v value from the device file into the ugMarkBuf
    ret = ugMarkInit();
    if (ret)
    {
        ret = -1;
        goto end;
    }

    // Check the value of ugMarkBuf for different upgrade states
    if (memcmp(ugMarkBuf, ugMarkUpgradingFINISH, sizeof(ugMarkUpgradingFINISH)) == 0)
    {
        ret = UG_UPGRADE_FLOW_FINISH;
        UG_LOG_WARN("Upgrading is finished.\n");
        goto end;
    }
    else if (memcmp(ugMarkBuf, ugMarkUpgradingFW, sizeof(ugMarkUpgradingFW)) == 0)
    {
        ret = UG_UPGRADE_FLOW_FW;
        UG_LOG_WARN("Upgrading FW is not finished.\n");
        goto end;
    }
    else if (memcmp(ugMarkBuf, ugMarkUpgradingFATP0, sizeof(ugMarkUpgradingFATP0)) == 0)
    {
        ret = UG_UPGRADE_FLOW_FAT_P0;
        UG_LOG_WARN("Upgrading FAT P0 is not finished.\n");
        goto end;
    }
    else if (memcmp(ugMarkBuf, ugMarkUpgradingFATP1, sizeof(ugMarkUpgradingFATP1)) == 0)
    {
        ret = UG_UPGRADE_FLOW_FAT_P1;
        UG_LOG_WARN("Upgrading FAT P1 is not finished.\n");
        goto end;
    }
    else if (memcmp(ugMarkBuf, ugMarkUpgradingFATP2, sizeof(ugMarkUpgradingFATP2)) == 0)
    {
        ret = UG_UPGRADE_FLOW_FAT_P2;
        UG_LOG_WARN("Upgrading FAT P2 is not finished.\n");
        goto end;
    }
    else if (memcmp(ugMarkBuf, ugMarkUpgradingFATP3, sizeof(ugMarkUpgradingFATP3)) == 0)
    {
        ret = UG_UPGRADE_FLOW_FAT_P3;
        UG_LOG_WARN("Upgrading FAT P3 is not finished.\n");
        goto end;
    }
    else
    {
        ret = UG_UPGRADE_FLOW_UNKNOWN;
        UG_LOG_WARN("Upgrading is unknown.\n");
        goto end;
    }

end:
    ugMarkExit();   // Exit the ugMark module
    #else
    // Initialize the ugMark module.
    // Load the mark'v value from the device file into the ugMarkBuf
    ret = ugMarkInit();
    if (ret)
        goto end;

    // Check if last upgrading is not finished
    if (memcmp(ugMarkBuf, ugMarkUpgrading, sizeof(ugMarkUpgrading)) == 0)
    {
        UG_LOG_ERR("last upgrading is not finished.\n");
        ret = __LINE__;
        goto end;
    }
    // Check if last backuping package is not finished
    else if (memcmp(ugMarkBuf, ugMarkBackuping, sizeof(ugMarkBackuping)) == 0)
    {
        UG_LOG_WARN("last backuping package is not finished.\n");
        goto end;
    }

end:
    ugMarkExit();   // Exit the ugMark module
    #endif

#endif // CFG_UPGRADE_MARK_POS
    return ret;
}

#ifndef CFG_UPGRADE_BACKUP_RAW_DATA
/**
 * Starts the upgrade process.
 *
 * @param backup - Whether to perform a backup before upgrading.
 * @return The result of the upgrade process.
 */
int ugUpgradeStart(bool backup)
{
    int ret = 0;

    // Perform actions only if CFG_UPGRADE_MARK_POS is not zero.
    #if CFG_UPGRADE_MARK_POS
    ugBackupPackageFinished = false;

    // Initialize the ugMark module.
    // Load the mark's value from the device file into the ugMarkBuf
    ret                     = ugMarkInit();
    if (ret)
        goto end;

    // Set the ugMarkBuf based on backup flag
    if (backup)
        memcpy(ugMarkBuf, ugMarkBackuping, sizeof(ugMarkBackuping));
    else
        memcpy(ugMarkBuf, ugMarkUpgrading, sizeof(ugMarkUpgrading));

    // Seek to the mark position
    if (lseek(ugMarkFD, ugMarkPos, SEEK_SET) != ugMarkPos)
    {
        UG_LOG_ERR("seek to mark position %d(%d) error\n", CFG_UPGRADE_MARK_POS, ugMarkPos);
        ret = __LINE__;
        goto end;
    }

    // Write the mark's new value to the device file
    if (write(ugMarkFD, ugMarkBuf, 1) != 1)
    {
        UG_LOG_ERR("write mark fail\n");
        goto end;
    }

end:
    ugMarkExit();   // Exit the ugMark module

    #endif // CFG_UPGRADE_MARK_POS
    return ret;
}

/**
 * Finish the upgrade backup.
 *
 * @return int 0 on success, non-zero on failure
 */
int ugUpgradeBackupFinish(void)
{
    int ret = 0;
    #if CFG_UPGRADE_MARK_POS

    // If the package backup process is already finished, return
    if (ugBackupPackageFinished)
        goto end;

    // Initialize the ugMark module.
    // Load the mark's value from the device file into the ugMarkBuf
    ret = ugMarkInit();
    if (ret)
        goto end;

    // Set the ugMarkBuf as ugMarkUpgrading
    memcpy(ugMarkBuf, ugMarkUpgrading, sizeof(ugMarkUpgrading));

    // Seek to the mark position
    if (lseek(ugMarkFD, ugMarkPos, SEEK_SET) != ugMarkPos)
    {
        UG_LOG_ERR("seek to mark position %d(%d) error\n", CFG_UPGRADE_MARK_POS, ugMarkPos);
        ret = __LINE__;
        goto end;
    }

    // Write the mark's new value to the device file
    if (write(ugMarkFD, ugMarkBuf, 1) != 1)
    {
        UG_LOG_ERR("write mark fail\n");
        goto end;
    }

    // Set the backup package finished flag
    ugBackupPackageFinished = true;

end:
    ugMarkExit();   // Exit the ugMark module

    #endif // CFG_UPGRADE_MARK_POS

    return ret;
}

/**
 * Finish the upgrade process.
 *
 * @return int 0 on success, non-zero on failure
 */
int ugUpgradeFinish(void)
{
    int ret = 0;
    #if CFG_UPGRADE_MARK_POS

    // Initialize the ugMark module.
    // Load the mark's value from the device file into the ugMarkBuf
    ret = ugMarkInit();
    if (ret)
        goto end;

    // Initialize the upgrade mark buffer with -1
    memset(ugMarkBuf, -1, sizeof(ugMarkUpgrading));

    // Seek to the mark position
    if (lseek(ugMarkFD, ugMarkPos, SEEK_SET) != ugMarkPos)
    {
        UG_LOG_ERR("seek to mark position %d(%d) error\n", CFG_UPGRADE_MARK_POS, ugMarkPos);
        ret = __LINE__;
        goto end;
    }

    // Flush the cache if NOR flash is enabled
        #if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
    ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
        #endif
        #if (CFG_LFS_CACHE_SIZE > 0)
        ioctl(ITP_DEVICE_LFS, ITP_IOCTL_FLUSH, NULL);
        #endif

    // Write the mark's new value to the device file
    if (write(ugMarkFD, ugMarkBuf, 1) != 1)
    {
        UG_LOG_ERR("clear mark fail\n");
        goto end;
    }

end:
    ugMarkExit();   // Exit the ugMark module

    #endif // CFG_UPGRADE_MARK_POS

    return ret;
}
#else
/**
 * Update the ugMark based on the given upgrade flow tag.
 *
 * @param flow_tag The given upgrade flow tag.
 * @return 0 if successful, otherwise an error code.
 */
int ugUpgradeTag(UG_UPGRADE_FLOW_TAG flow_tag)
{
    int ret = 0;

    #if CFG_UPGRADE_MARK_POS
    // Initialize the ugMark module.
    // Load the mark's value from the device file into the ugMarkBuf
    ret     = ugMarkInit();
    if (ret)
        goto end;

    // Set the ugMarkBuf's value based on the upgrade flow tag
    if (flow_tag == UG_UPGRADE_FLOW_FINISH)
        memcpy(ugMarkBuf, ugMarkUpgradingFINISH, sizeof(ugMarkUpgradingFINISH));
    else if (flow_tag == UG_UPGRADE_FLOW_FW)
        memcpy(ugMarkBuf, ugMarkUpgradingFW, sizeof(ugMarkUpgradingFW));
    else if (flow_tag == UG_UPGRADE_FLOW_FAT_P0)
        memcpy(ugMarkBuf, ugMarkUpgradingFATP0, sizeof(ugMarkUpgradingFATP0));
    else if (flow_tag == UG_UPGRADE_FLOW_FAT_P1)
        memcpy(ugMarkBuf, ugMarkUpgradingFATP1, sizeof(ugMarkUpgradingFATP1));
    else if (flow_tag == UG_UPGRADE_FLOW_FAT_P2)
        memcpy(ugMarkBuf, ugMarkUpgradingFATP2, sizeof(ugMarkUpgradingFATP2));
    else if (flow_tag == UG_UPGRADE_FLOW_FAT_P3)
        memcpy(ugMarkBuf, ugMarkUpgradingFATP3, sizeof(ugMarkUpgradingFATP3));
    else
        memcpy(ugMarkBuf, ugMarkUpgradingUNKNOWN, sizeof(ugMarkUpgradingUNKNOWN));

    // Seek to the mark position
    if (lseek(ugMarkFD, ugMarkPos, SEEK_SET) != ugMarkPos)
    {
        UG_LOG_ERR("seek to mark position %d(%d) error\n", CFG_UPGRADE_MARK_POS, ugMarkPos);
        ret = __LINE__;
        goto end;
    }

    // Write the mark's new value to the device file
    if (write(ugMarkFD, ugMarkBuf, 1) != 1)
    {
        UG_LOG_ERR("write mark fail\n");
        ret = __LINE__;
        goto end;
    }

end:
    ugMarkExit();   // Exit the ugMark module

    #endif // CFG_UPGRADE_MARK_POS
    return ret;
}
#endif

#ifdef CFG_A_B_DUAL_BOOT_ENABLE
int ugBootargsRead(uint8_t *boot_args)
{
    uint8_t  *buf   = NULL;
    uint32_t pos, blocksize = 0;
    int      ret = 0, i;
    uint32_t offset = 0;

    UG_LOG_INFO("Read boot args from storage\n" );

#if defined(CFG_BOOT_ARGS_NAND)
    int fd = open(":nand", O_RDWR, 0);
    UG_LOG_DBG("nand fd: 0x%X\n", fd);
#elif defined(CFG_BOOT_ARGS_NOR)
    int fd = open(":nor", O_RDWR, 0);
    UG_LOG_DBG("nor fd: 0x%X\n", fd);
#elif defined(CFG_BOOT_ARGS_SD0)
    int fd = open(":sd0", O_RDWR, 0);
    UG_LOG_DBG("sd0 fd: 0x%X\n", fd);
#elif defined(CFG_BOOT_ARGS_SD1)
    int fd = open(":sd1", O_RDWR, 0);
    UG_LOG_DBG("sd1 fd: 0x%X\n", fd);
#else
    int fd = -1;
#endif
    if (fd == -1)
    {
        UG_LOG_ERR("open device error: %d\n", fd);
        ret = __LINE__;
        goto end;
    }
    if (ioctl(fd, ITP_IOCTL_GET_BLOCK_SIZE, &blocksize))
    {
        UG_LOG_ERR("get block size error\n");
        ret = __LINE__;
        goto end;
    }
    UG_LOG_DBG("blocksize=%d\n", blocksize);

    pos = CFG_BOOT_ARGS_POS / blocksize;

    if (lseek(fd, pos, SEEK_SET) != pos)
    {
        UG_LOG_ERR("seek to boot args addr position %d(%d) error\n", CFG_BOOT_ARGS_POS, pos);
        ret = __LINE__;
        goto end;
    }

    assert(blocksize >= 8);
    buf = (uint8_t *)malloc(blocksize);
    if (!buf)
    {
        ret = __LINE__;
        goto end;
    }
    // Read 1 byte from the device
    ret = read(fd, buf, 1);
    if (ret != 1)
    {
        UG_LOG_ERR("read boot args address error: %d != 1\n", ret );
        ret = __LINE__;
        goto end;
    }else
    {
        ret = 0;
    }
    memcpy(boot_args, buf, 1);

end:
    if (fd != -1)
        close(fd);

    if (buf)
        free(buf);

    return ret;
}

int ugBootargsWrite(uint8_t boot_args)
{
    uint8_t  *buf   = NULL;
    uint32_t pos, blocksize = 0;
    int      ret = 0, i;
    uint32_t offset = 0;

    UG_LOG_INFO("Write boot args from storage\n" );

#if defined(CFG_BOOT_ARGS_NAND)
    int fd = open(":nand", O_RDWR, 0);
    UG_LOG_DBG("nand fd: 0x%X\n", fd);
#elif defined(CFG_BOOT_ARGS_NOR)
    int fd = open(":nor", O_RDWR, 0);
    UG_LOG_DBG("nor fd: 0x%X\n", fd);
#elif defined(CFG_BOOT_ARGS_SD0)
    int fd = open(":sd0", O_RDWR, 0);
    UG_LOG_DBG("sd0 fd: 0x%X\n", fd);
#elif defined(CFG_BOOT_ARGS_SD1)
    int fd = open(":sd1", O_RDWR, 0);
    UG_LOG_DBG("sd1 fd: 0x%X\n", fd);
#else
    int fd = -1;
#endif
    if (fd == -1)
    {
        UG_LOG_ERR("open device error: %d\n", fd);
        ret = __LINE__;
        goto end;
    }
    if (ioctl(fd, ITP_IOCTL_GET_BLOCK_SIZE, &blocksize))
    {
        UG_LOG_ERR("get block size error\n");
        ret = __LINE__;
        goto end;
    }
    UG_LOG_DBG("blocksize=%d\n", blocksize);

    pos = CFG_BOOT_ARGS_POS / blocksize;

    if (lseek(fd, pos, SEEK_SET) != pos)
    {
        UG_LOG_ERR("seek to boot args addr position %d(%d) error\n", CFG_BOOT_ARGS_POS, pos);
        ret = __LINE__;
        goto end;
    }

    assert(blocksize >= 8);
    buf = (uint8_t *)malloc(blocksize);
    if (!buf)
    {
        ret = __LINE__;
        goto end;
    }
    // Write 1 byte to the device
    buf[0] = boot_args;
    if (write(fd, buf, 1) != 1)
    {
        UG_LOG_ERR("write boot args addr fail\n" );
        goto end;
    }
    
end:
    if (fd != -1)
        close(fd);

    if (buf)
        free(buf);

    return ret;
}
#endif

