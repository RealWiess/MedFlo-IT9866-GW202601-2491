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
    .rtlbt_hci_rev = 0x000d,
    .rtlbt_lmp_subver = 0x8723,
    //#define _ROM_VER_ 0  // 0: A cut, 1: B cut...
    .rtlbt_eco_version = 0x02,
    //if don't know use 0, just check the lmp_subver
    .rtlbt_fw_check_flag = 0,
    .rtlbt_config_uart_offset = 0x000c,
    .rtlbt_config_bd_addr_offset = 0x0044,
    .rtlbt_config_fw_log_offset = 0x00ed,
    .rtlbt_config_fw_log_on = 0x09,
    .rtlbt_config_fw_log_off = 0x00,
    .rtlbt_fw_hci_proto = RTLBT_UART_H5,
    .bt_wifi_coex_flag = 1,
};

uint8_t rtlbt_fw_hci_proto = RTLBT_UART_H5;
/** @brief RTL8761ATV configuration */
unsigned char rtlbt_config[] =
{
    0x55, 0xab, 0x23, 0x87, /* Signature */
    0x31, 0x00, /* Length */
    /* UART Setting */
    0x0C, 0x00, 0x10,
    0x02, 0x80, 0x92, 0x04, 0x50, 0xC5, 0xEA, 0x19,
    0xE1, 0x1B, 0xFD, 0xAF, 0x5F, 0x01, 0xA4, 0x0B,
    /* PCM Setting */
    0xF4, 0x00, 0x01, 0x01,
    0xF6, 0x00, 0x02, 0x81, 0x00,
    0xFA, 0x00, 0x02, 0x12, 0x80,
    0xD9, 0x00, 0x01, 0x0F,
    0xE4, 0x00, 0x01, 0x08,
    0x8D, 0x00, 0x01, 0xFA,
    /* FW log default on */
    0xed, 0x00, 0x01, 0x09,
#if 0
    /* BDADDR */
    0x44, 0x00, 0x06,
    0x91, 0x82, 0x93, 0x04, 0x85, 0x07,
#endif
};

/** @brief The length of RTL8761ATV configuration */
unsigned int rtlbt_config_len = sizeof(rtlbt_config);
