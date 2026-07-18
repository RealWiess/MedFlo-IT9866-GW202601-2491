/**
 *******************************************************************************
 * Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
 *******************************************************************************
 * @file rtl8723d_fwconfig.c
 * @brief RTL8723ds firmware and configuration
 * @details
 * @author Thomas_li
 * @version v1.0
 * @date 2019-08-23
 */
#include "rtlbt_common.h"
T_PATCH_CONFIG rtkbt_config_info =
{
    .rtlbt_hci_ver = 0x08,
    .rtlbt_hci_rev = 0x000c,
    .rtlbt_lmp_subver = 0x8821,
    //#define _ROM_VER_ 0  // 0: A cut, 1: B cut...
    .rtlbt_eco_version = 0x01,
    //if don't know use 0, just check the lmp_subver
    .rtlbt_fw_check_flag = 0,
    .rtlbt_config_uart_offset = 0x000c,
    .rtlbt_config_bd_addr_offset = 0x0044,
    .rtlbt_config_fw_log_offset = 0x00ed,
    .rtlbt_config_fw_log_on = 0x0101,
    .rtlbt_config_fw_log_off = 0x00,
    .rtlbt_fw_hci_proto = RTLBT_UART_H5,
    .bt_wifi_coex_flag = 1,
};

uint8_t rtlbt_fw_hci_proto = RTLBT_UART_H5;
/** @brief RTL8761ATV configuration */
unsigned char rtlbt_config[] =
{
     0x55, 0xab, 0x23, 0x87,
    //0x27, 0x00,
    0x1F, 0x00,

    /* UART*/
    0x0c, 0x00, 0x10,
    //3M
    //0x01, 0x80, 0x92, 0x04, 0x50, 0xc5, 0xea, 0x19,
    //0xe1, 0x1b, 0xfd, 0xaf, 0x5b, 0x01, 0xa4, 0x0b,
    //2M
    0x02, 0x50, 0x00, 0x00, 0x50, 0xc5, 0xea, 0x19,
    0xe1, 0x1b, 0xfd, 0xaf, 0x5b, 0x01, 0xa4, 0x0b,

    /*PCM */
    0xd9, 0x00, 0x01, 0x0f,
    0xe4, 0x00, 0x01, 0x08,
    //0x8d, 0x00, 0x01, 0xfa,
    /* FW log default on */
    0xed, 0x00, 0x01, 0x01,
    /*BDADDR*/
    //0x8f, 0x00, 0x01, 0xbf,
};

/** @brief The length of RTL8761ATV configuration */
unsigned int rtlbt_config_len = sizeof(rtlbt_config);
