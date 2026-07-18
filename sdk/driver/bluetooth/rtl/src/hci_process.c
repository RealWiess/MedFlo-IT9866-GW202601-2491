/**
 *******************************************************************************
 * Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
 *******************************************************************************
 * @file  hci_process.c
 * @brief hci process table for extern
 * @details
 * @author Thomas_li
 * @version v1.0
 * @date 2019-08-23
 */
#include <stdio.h>
#include <string.h>

#include "hci_ps.h"
#include "hci_tp_dbg.h"

uint8_t hci_ps_read_bd_addr(void)
{
    bool ret = false;
    ret = hci_tp_send_hci_cmd(HCI_READ_LOACAL_BD_ADDR, NULL, 0);
    if (ret == true)
    {
        /*TODO:  send_hci_index++ */
        return HCI_TP_CHECK_OK;
    }
    return HCI_TP_CONFIG_FAIL;
}

uint8_t hci_pc_read_bd_addr_check(uint8_t len, uint8_t *p_buf)
{
    uint8_t bd_addr[6];
    uint8_t default_addr[6] = {0x99, 0x55, 0x23, 0x4c, 0xe0, 0x00};
    memcpy(bd_addr, p_buf, 6);
    tp_osif_printf("local bd addr %02x:%02x:%02x:%02x:%02x:%02x\n", bd_addr[5], bd_addr[4], bd_addr[3],
                   bd_addr[2],
                   bd_addr[1], bd_addr[0]);
    if (!memcmp(bd_addr, default_addr, 6))
    {
        hci_tp_err("WARN!!!! The bd addr is default, please check the BT MAC ADDRESS AND EFUSE TABLE");
    }
    return HCI_TP_CHECK_OK;
}

HCI_PROCESS_TABLE hci_process_table[] =
{
    //pre donload patch
    {HCI_READ_LOCAL_VERSION_INFO, hci_pc_read_local_ver, hci_pc_read_local_ver_check},
    {HCI_VSC_READ_ROM_VERSION, hci_pc_read_rom_ver, hci_pc_read_rom_ver_check},
    //download patch
    {HCI_VSC_UPDATE_BAUDRATE, hci_pc_set_controller_baudrate, hci_pc_set_baudrate_check},
    {HCI_VSC_DOWNLOAD_PATCH, hci_pc_download_patch, hci_pc_download_patch_check},
    {HCI_HCI_RESET, hci_ps_set_hci_reset, NULL},
    {HCI_READ_LOACAL_BD_ADDR, hci_ps_read_bd_addr, hci_pc_read_bd_addr_check},
//    add sample read bd addr
    {0, NULL, NULL},
};



