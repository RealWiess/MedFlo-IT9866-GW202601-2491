/**
 *******************************************************************************
 * Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
 *******************************************************************************
 * @file btrtl_fwconfig.c
 * @brief RTL8761ATV firmware and configuration
 * @details
 * @author Thomas Li
 * @version v1.0
 * @date 2019-08-22
 */
#include <stdint.h>

#define RTL_FW_MATCH_CHIP_TYPE  (1 << 0)
#define RTL_FW_MATCH_HCI_VER    (1 << 1)
#define RTL_FW_MATCH_HCI_REV    (1 << 2)
#define RTL_FW_MATCH_ECO_VER    (1 << 3)

#define RTLBT_UART_H4    4
#define RTLBT_UART_H5    5

typedef struct t_patch_config_struct
{
    uint8_t  rtlbt_hci_ver;
    uint16_t rtlbt_hci_rev;
    uint16_t rtlbt_lmp_subver;
    uint8_t  rtlbt_eco_version;
    uint8_t  rtlbt_fw_check_flag;

    uint16_t rtlbt_config_uart_offset ;
    uint16_t rtlbt_config_fw_log_offset ;
    uint16_t rtlbt_config_bd_addr_offset ;

    uint16_t rtlbt_config_fw_log_on;
    uint8_t rtlbt_config_fw_log_off;
    uint8_t rtlbt_fw_hci_proto;

    uint8_t bt_wifi_coex_flag;
} T_PATCH_CONFIG;

extern T_PATCH_CONFIG rtkbt_config_info;

extern unsigned char rtlbt_config[];
extern unsigned int rtlbt_config_len;

extern const unsigned char rtlbt_fw[];
extern unsigned int rtlbt_fw_len;
#if 0
#inlcude "rlt8761_fw.h"

#define RTK_FW_PATCH(a)   rtl##a##_fw
#define RTK_FW_PATCH_LEN(a)   rtl##a##_fw_len

#define RTK_FW_PATCH_CONFIG(a)   rtl##a##_config
#define RTK_FW_PATCH_CONFIG_LEN(a)   rtl##a##_config_len

#define rtlbt_fw    RTK_FW_PATCH(8761a)
#define rtlbt_fw_len  RTK_FW_PATCH_LEN(8761a)

#define rtlbt_config    RTK_FW_PATCH_CONFIG(8761a)
#define rtlbt_config_len  RTK_FW_PATCH_CONFIG_LEN(8761a)

#endif

