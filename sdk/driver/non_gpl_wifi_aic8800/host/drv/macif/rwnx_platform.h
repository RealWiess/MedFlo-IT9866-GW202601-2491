/**
 ******************************************************************************
 *
 * @file rwnx_platorm.h
 *
 * Copyright (C) RivieraWaves 2012-2019
 *
 ******************************************************************************
 */

#ifndef _RWNX_PLATFORM_H_
#define _RWNX_PLATFORM_H_

#include "lmac_msg.h"

struct rwnx_hw;

/**
 * struct rwnx_plat - Operation pointers for RWNX PCI platform
 *
 * @pci_dev: pointer to pci dev
 * @enabled: Set if embedded platform has been enabled (i.e. fw loaded and
 *          ipc started)
 * @enable: Configure communication with the fw (i.e. configure the transfers
 *         enable and register interrupt)
 * @disable: Stop communication with the fw
 * @deinit: Free all ressources allocated for the embedded platform
 * @get_address: Return the virtual address to access the requested address on
 *              the platform.
 * @ack_irq: Acknowledge the irq at link level.
 * @get_config_reg: Return the list (size + pointer) of registers to restore in
 * order to reload the platform while keeping the current configuration.
 *
 * @priv Private data for the link driver
 */
struct rwnx_plat {
    #if 0
    struct pci_dev *pci_dev;

#ifdef AICWF_SDIO_SUPPORT
	struct aic_sdio_dev *sdiodev;
#endif

#ifdef AICWF_USB_SUPPORT
    struct aic_usb_dev *usbdev;
#endif
    #endif
    bool enabled;

    #if 0
    int (*enable)(struct rwnx_hw *rwnx_hw);
    int (*disable)(struct rwnx_hw *rwnx_hw);
    void (*deinit)(struct rwnx_plat *rwnx_plat);
    u8* (*get_address)(struct rwnx_plat *rwnx_plat, int addr_name,
                       unsigned int offset);
    void (*ack_irq)(struct rwnx_plat *rwnx_plat);
    int (*get_config_reg)(struct rwnx_plat *rwnx_plat, const u32 **list);

    u8 priv[0] __aligned(sizeof(void *));
    #endif
};

extern struct rwnx_plat *g_rwnx_plat;

int rwnx_platform_init(struct rwnx_plat *rwnx_plat, void **platform_data);
void rwnx_platform_deinit(struct rwnx_hw *rwnx_hw);

int rwnx_platform_on(struct rwnx_hw *rwnx_hw);
void rwnx_platform_off(struct rwnx_hw *rwnx_hw);

int aicwf_patch_table_load(struct rwnx_hw *rwnx_hw);

#ifdef CONFIG_LOAD_USERCONFIG
void get_nvram_txpwr_idx(txpwr_idx_conf_t *txpwr_idx);
void get_nvram_txpwr_ofst(txpwr_ofst_conf_t *txpwr_ofst);
void get_nvram_xtal_cap(xtal_cap_conf_t *xtal_cap);


void get_userconfig_txpwr_lvl(txpwr_lvl_conf_t *txpwr_lvl);
void get_userconfig_txpwr_lvl_v2(txpwr_lvl_conf_v2_t *txpwr_lvl_v2);
void get_userconfig_txpwr_lvl_v3(txpwr_lvl_conf_v3_t *txpwr_lvl_v3);
void get_userconfig_txpwr_ofst(txpwr_ofst_conf_t *txpwr_ofst);
void get_userconfig_txpwr_ofst2x(txpwr_ofst2x_conf_t *txpwr_ofst2x);
void get_userconfig_xtal_cap(xtal_cap_conf_t *xtal_cap);
#endif

#endif /* _RWNX_PLATFORM_H_ */
