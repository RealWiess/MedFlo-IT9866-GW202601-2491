#include "ite/ug.h"
#include "ug_cfg.h"
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static const char ugMarkEncryptDo[] = {'N', 'E', 'E', 'D', 'T', 'O', 'E', 'N', 'C', 'R', 'Y', 'P', 'T'};
static const char ugMarkEncryptDone[] = {'E', 'N', 'C', 'R', 'Y', 'P', 'T', 'D', 'O', 'N', 'E'};
static const char ugMarkUNKNOWN[] = {'U', 'N', 'K', 'N', 'O', 'W', 'N'};

static int ugMarkFD = -1;
static uint32_t ugMarkBlockSize = 0, ugMarkPos = 0;
static uint8_t *ugMarkBuf = NULL;

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
    ugMarkPos = CFG_FW_DPUAES_UPGRADE_MARK_POS / ugMarkBlockSize;

    UG_LOG_INFO("ugMarkFD=0x%X ugMarkBlockSize=%d ugMarkPos=0x%X\n", ugMarkFD, ugMarkBlockSize,
        ugMarkPos);

    // Seek to the mark position
    if (lseek(ugMarkFD, ugMarkPos, SEEK_SET) != ugMarkPos)
    {
        UG_LOG_ERR("seek to mark position %d(%d) error\n", CFG_FW_DPUAES_UPGRADE_MARK_POS, ugMarkPos);
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

    // Read one from the device into the mark buffer
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

/**
 * Determine the last upgrade state depending on the mark's value.
 *
 * @return The last upgrade state
 */
UG_FW_AES_UPGRADE_FLOW_TAG ugFWAESUpgradeCheck(void)
{
    int ret = 0;

    // Initialize the ugMark module.
    // Load the mark's value from the device file into the ugMarkBuf
    ret = ugMarkInit();
    if (ret)
        goto end;

    // Check the value of ugMarkBuf for different upgrade states
    if (memcmp(ugMarkBuf, ugMarkEncryptDo, sizeof(ugMarkEncryptDo)) == 0)
    {
        ret = UG_FW_AES_UPGRADE_FLOW_ENCRYPT_DO;
        UG_LOG_WARN("firmware need to be encrypted!\n");
        goto end;
    }
    else if (memcmp(ugMarkBuf, ugMarkEncryptDone, sizeof(ugMarkEncryptDone)) == 0)
    {
        ret = UG_FW_AES_UPGRADE_FLOW_ENCRYPT_DONE;
        UG_LOG_WARN("firmware has encrypted!\n");
        goto end;
    }
    else
    {
        ret = UG_FW_AES_UPGRADE_FLOW_UNKNOWN;
        UG_LOG_WARN("FW AES tag is unknown!\n");
        goto end;
    }

end:
    // Exit the ugMark module
    ugMarkExit();

    return ret;
}

/**
 * Update the ugMark based on the given upgrade flow tag.
 *
 * @param flow_tag The given upgrade flow tag.
 * @return 0 if successful, otherwise an error code.
 */
int ugFWAESUpgradeTag(UG_FW_AES_UPGRADE_FLOW_TAG flow_tag)
{
    int ret = 0;

    // Initialize the ugMark module.
    // Load the mark's value from the device file into the ugMarkBuf
    ret = ugMarkInit();
    if (ret)
        goto end;

    // Set the ugMarkBuf's value based on the upgrade flow tag
    if (flow_tag == UG_FW_AES_UPGRADE_FLOW_ENCRYPT_DO)
        memcpy(ugMarkBuf, ugMarkEncryptDo, sizeof(ugMarkEncryptDo));
    else if (flow_tag == UG_FW_AES_UPGRADE_FLOW_ENCRYPT_DONE)
        memcpy(ugMarkBuf, ugMarkEncryptDone, sizeof(ugMarkEncryptDone));
    else
        memcpy(ugMarkBuf, ugMarkUNKNOWN, sizeof(ugMarkUNKNOWN));

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
    // Exit the ugMark module
    ugMarkExit();

    return ret;
}
