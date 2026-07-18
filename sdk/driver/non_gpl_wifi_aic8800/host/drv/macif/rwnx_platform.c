/**
 ******************************************************************************
 *
 * @file rwnx_platform.c
 *
 * Copyright (C) RivieraWaves 2012-2019
 *
 ******************************************************************************
 */

#include "rwnx_platform.h"
#include "reg_access.h"
//#include "hal_desc.h"
#include "rwnx_main.h"
#include "rwnx_msg_tx.h"
#include "rwnx_utils.h"
#include "aic_fw.h"
#include "rwnx_defs.h"
#include "log.h"
#include "wifi.h"

#ifdef AICWF_SDIO_SUPPORT
//#include "aicwf_sdio.h"
#endif

#ifdef AICWF_USB_SUPPORT
//#include "aicwf_usb.h"
#endif

struct rwnx_plat rwnx_plat_obj;
struct rwnx_plat *g_rwnx_plat = NULL;

extern u8 chip_id;
extern u8 chip_sub_id;
extern u8 chip_mcu_id;
extern u8 aic8800dc_rf_flag;


static int rwnx_load_firmware(struct rwnx_hw *rwnx_hw, u32 **fw_buf)
{
    int size = 0;

    if (rwnx_hw->chipid == PRODUCT_ID_AIC8801) {
        #if defined(CONFIG_AIC8800) || defined(CONFIG_AIC8801)
        if (rwnx_hw->mode == WIFI_MODE_RFTEST) {
#ifdef CONFIG_RFTEST
            *fw_buf = (u32 *)aic8800d_rf_fw_ptr_get();
            size = aic8800d_rf_fw_size_get();
#else
            printk("wrong config, check CONFIG_RFTEST in Makefile\n");
            *fw_buf = NULL;
            size = 0;
#endif
        } else {
            *fw_buf = (u32 *)aic8800d_fw_ptr_get();
            size = aic8800d_fw_size_get();
        }
        #else
        printk("wrong config, check CONFIG_AIC8800D in Makefile\n");
        *fw_buf = NULL;
        size = 0;
        #endif
    } else if (rwnx_hw->chipid == PRODUCT_ID_AIC8800DC || rwnx_hw->chipid == PRODUCT_ID_AIC8800DW) {
    } else if (rwnx_hw->chipid == PRODUCT_ID_AIC8800D80) {
        #if defined(CONFIG_AIC8800D80)
        if (chip_id == CHIP_REV_U01) {
        } else if (chip_id == CHIP_REV_U02 || chip_id == CHIP_REV_U03) {
            if (rwnx_hw->mode == WIFI_MODE_RFTEST) {
#ifdef CONFIG_RFTEST
                *fw_buf = (u32 *)aic8800d80_u02_rf_fw_ptr_get();
                size = aic8800d80_u02_rf_fw_size_get();
#else
                printk("wrong config, check CONFIG_RFTEST in Makefile\n");
                *fw_buf = NULL;
                size = 0;
#endif
            } else {
                *fw_buf = (u32 *)aic8800d80_u02_fw_ptr_get();
                size = aic8800d80_u02_fw_size_get();
            }
        }
        #else
        printk("wrong config, check CONFIG_AIC8800D80 in Makefile\n");
        *fw_buf = NULL;
        size = 0;
        #endif
    }
    return size;
}

static int rwnx_load_patch_tbl(struct rwnx_hw *rwnx_hw, u32 **fw_buf)
{
    int size = 0;

    if (rwnx_hw->chipid == PRODUCT_ID_AIC8800DC || rwnx_hw->chipid == PRODUCT_ID_AIC8800DW) {
    }
    return size;
}

static void rwnx_restore_firmware(u32 **fw_buf)
{
    *fw_buf = NULL;
}

/* buffer is allocated by kzalloc */
static int rwnx_request_firmware_common(struct rwnx_hw *rwnx_hw, u32** buffer)
{
    int size;
    if (rwnx_hw->fw_patch == 0)
        size = rwnx_load_firmware(rwnx_hw, buffer);
    else if (rwnx_hw->fw_patch == 1)
        size = rwnx_load_patch_tbl(rwnx_hw, buffer);
    return size;
}

static void rwnx_release_firmware_common(u32** buffer)
{
    rwnx_restore_firmware(buffer);
}

int aicwf_patch_table_load(struct rwnx_hw *rwnx_hw)
{
    int err = 0;
    unsigned int i = 0, size;
    u32 *dst;
    u8 *describle;
    u32 fmacfw_patch_tbl_8800dc_u02_describe_size = 124;
    u32 fmacfw_patch_tbl_8800dc_u02_describe_base;//read from patch_tbl

    rwnx_hw->fw_patch = 1;

    /* Copy the file on the Embedded side */
    //printk("### Upload %s \n", filename);

    size = rwnx_request_firmware_common(rwnx_hw, &dst);
    if (!dst) {
        printk("No such file or directory\n");
        return -1;
    }
    if (size <= 0) {
            printk("wrong size of firmware file\n");
            dst = NULL;
            err = -1;
    }

    printk("tbl size = %d \n",size);

    fmacfw_patch_tbl_8800dc_u02_describe_base = dst[0];
    printk("FMACFW_PATCH_TBL_8800DC_U02_DESCRIBE_BASE = %x \n",fmacfw_patch_tbl_8800dc_u02_describe_base);

    if (!err && (i < size)) {
        err=rwnx_send_dbg_mem_block_write_req(rwnx_hw, fmacfw_patch_tbl_8800dc_u02_describe_base, fmacfw_patch_tbl_8800dc_u02_describe_size + 4, dst);
        if(err){
            printk("write describe information fail \n");
        }

        describle = (u8 *)rtos_malloc(fmacfw_patch_tbl_8800dc_u02_describe_size);
        memcpy(describle,&dst[1],fmacfw_patch_tbl_8800dc_u02_describe_size);
        printk("%s",describle);
        rtos_free(describle);
        describle=NULL;
    }

    if (!err && (i < size)) {
        for (i =(128/4); i < (size/4); i +=2) {
            printk("patch_tbl:  %x  %x\n", dst[i], dst[i+1]);
            err = rwnx_send_dbg_mem_write_req(rwnx_hw, dst[i], dst[i+1]);
        }
        if (err) {
            printk("bin upload fail: %x, err:%d\r\n", dst[i], err);
        }
    }

    if (dst) {
        rwnx_release_firmware_common(&dst);
    }

    return err;
}

//#ifndef CONFIG_ROM_PATCH_EN
/**
 * rwnx_plat_bin_fw_upload_2() - Load the requested binary FW into embedded side.
 *
 * @rwnx_hw: Main driver data
 * @fw_addr: Address where the fw must be loaded
 * @filename: Name of the fw.
 *
 * Load a fw, stored as a binary file, into the specified address
 */
int rwnx_plat_bin_fw_upload_2(struct rwnx_hw *rwnx_hw, u32 fw_addr)
{
    int err = 0;
    unsigned int i = 0, size;
    u32 *dst;
    const int BLOCK_SIZE = 512;//1024; 512 is requested by 8800dc/dw

    /* Copy the file on the Embedded side */
    printk("\n### Upload firmware, @ = %x\n", fw_addr);

    rwnx_hw->fw_patch = 0;

    size = rwnx_request_firmware_common(rwnx_hw, &dst);
    if (size <= 0) {
            printk("wrong size of firmware file\n");
            dst = NULL;
            err = -1;
    }

    printk("\n### dst=%p, size=%d\n", dst, size);
    if (size > BLOCK_SIZE) {
        for (; i < (size - BLOCK_SIZE); i += BLOCK_SIZE) {
            printk("wr blk 0: %p -> %x\r\n", dst + i / 4, fw_addr + i);
            err = rwnx_send_dbg_mem_block_write_req(rwnx_hw, fw_addr + i, BLOCK_SIZE, dst + i / 4);
            if (err) {
                printk("bin upload fail: %x, err:%d\r\n", fw_addr + i, err);
                break;
            }
        }
    }
    if (!err && (i < size)) {
        printk("wr blk 1: %p -> %x\r\n", dst + i / 4, fw_addr + i);
        err = rwnx_send_dbg_mem_block_write_req(rwnx_hw, fw_addr + i, size - i, dst + i / 4);
        if (err) {
            printk("bin upload fail: %x, err:%d\r\n", fw_addr + i, err);
        }
    }

    if (dst) {
        rwnx_release_firmware_common(&dst);
    }

    return err;
}
//#endif /* !CONFIG_ROM_PATCH_EN */

//#ifndef CONFIG_ROM_PATCH_EN
/**
 * rwnx_plat_fmac_load() - Load FW code
 *
 * @rwnx_hw: Main driver data
 */
int rwnx_plat_fmac_load(struct rwnx_hw *rwnx_hw)
{
    int ret;

    RWNX_DBG(RWNX_FN_ENTRY_STR);
    ret = rwnx_plat_bin_fw_upload_2(rwnx_hw, RAM_FMAC_FW_ADDR);
    return ret;
}
//#endif /* !CONFIG_ROM_PATCH_EN */

static int rwnx_plat_patch_load(struct rwnx_hw *rwnx_hw)
{
    int ret = 0;

    RWNX_DBG(RWNX_FN_ENTRY_STR);
    if(rwnx_hw->chipid == PRODUCT_ID_AIC8800DC ||
        rwnx_hw->chipid == PRODUCT_ID_AIC8800DW){
    }
    return ret;
}

/**
 * rwnx_platform_reset() - Reset the platform
 *
 * @rwnx_plat: platform data
 */
static int rwnx_platform_reset(struct rwnx_plat *rwnx_plat)
{
    return 0;
}

/**
 * rwnx_platform_on() - Start the platform
 *
 * @rwnx_hw: Main driver data
 * @config: Config to restore (NULL if nothing to restore)
 *
 * It starts the platform :
 * - load fw and ucodes
 * - initialize IPC
 * - boot the fw
 * - enable link communication/IRQ
 *
 * Called by 802.11 part
 */
int rwnx_platform_on(struct rwnx_hw *rwnx_hw)
{
    //#ifndef CONFIG_ROM_PATCH_EN
    #ifdef CONFIG_DOWNLOAD_FW
    int ret;
    #endif
    //#endif
    //struct rwnx_plat *rwnx_plat = rwnx_hw->plat;
    struct rwnx_plat *rwnx_plat = &rwnx_plat_obj;

    RWNX_DBG(RWNX_FN_ENTRY_STR);

    if (rwnx_plat->enabled)
        return 0;

    #ifdef CONFIG_DOWNLOAD_FW
    if (rwnx_hw->chipid != PRODUCT_ID_AIC8800DC && rwnx_hw->chipid != PRODUCT_ID_AIC8800DW) {
        ret = rwnx_plat_fmac_load(rwnx_hw);
        if (ret)
            return ret;
    } else {
        rwnx_plat_patch_load(rwnx_hw);
    }
    #endif

    #ifdef CONFIG_LOAD_USERCONFIG
    //rwnx_plat_userconfig_load(rwnx_hw);
    #endif

    rwnx_plat->enabled = true;

    return 0;
}

/**
 * rwnx_platform_off() - Stop the platform
 *
 * @rwnx_hw: Main driver data
 * @config: Updated with pointer to config, to be able to restore it with
 * rwnx_platform_on(). It's up to the caller to free the config. Set to NULL
 * if configuration is not needed.
 *
 * Called by 802.11 part
 */
void rwnx_platform_off(struct rwnx_hw *rwnx_hw)
{
    //rwnx_platform_reset(rwnx_hw->plat);
    //rwnx_hw->plat->enabled = false;
    struct rwnx_plat *rwnx_plat = &rwnx_plat_obj;
    rwnx_platform_reset(rwnx_plat);
    rwnx_plat->enabled = false;
}

/**
 * rwnx_platform_init() - Initialize the platform
 *
 * @rwnx_plat: platform data (already updated by platform driver)
 * @platform_data: Pointer to store the main driver data pointer (aka rwnx_hw)
 *                That will be set as driver data for the platform driver
 * Return: 0 on success, < 0 otherwise
 *
 * Called by the platform driver after it has been probed
 */
int rwnx_platform_init(struct rwnx_plat *rwnx_plat, void **platform_data)
{
    RWNX_DBG(RWNX_FN_ENTRY_STR);

    rwnx_plat->enabled = false;
    g_rwnx_plat = rwnx_plat;

#if defined CONFIG_RWNX_FULLMAC
    //return rwnx_fdrv_init(rwnx_plat, platform_data);
    return 0;
#endif
}

/**
 * rwnx_platform_deinit() - Deinitialize the platform
 *
 * @rwnx_hw: main driver data
 *
 * Called by the platform driver after it is removed
 */
void rwnx_platform_deinit(struct rwnx_hw *rwnx_hw)
{
    RWNX_DBG(RWNX_FN_ENTRY_STR);

#if defined CONFIG_RWNX_FULLMAC
    //rwnx_fdrv_deinit(rwnx_hw);
#endif
}

