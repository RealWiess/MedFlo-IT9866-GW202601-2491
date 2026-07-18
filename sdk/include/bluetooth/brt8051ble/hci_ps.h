/**
 * Copyright (C) 2017 Realtek Semiconductor Corporation
 * Author: Thomas_li <thomas_li@realsil.com.cn>
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define  HCI_STACK_VERSION     "V1.0.1"

#define HCI_CMD_PKT             0x01
#define HCI_ACL_PKT             0x02
#define HCI_SCO_PKT             0x03
#define HCI_EVT_PKT             0x04
#define HCI_CMD_HDR_LEN         4   /* packet type (1), command (2), length (1) */
#define HCI_COMMAND_COMPLETE            0x0e
#define HCI_COMMAND_STATUS              0x0f


#define HCI_READ_LOCAL_VERSION_INFO     0x1001
#define HCI_READ_LOACAL_BD_ADDR         0x1009
#define HCI_VSC_READ_ROM_VERSION        0xFC6D
#define HCI_VSC_UPDATE_BAUDRATE         0xFC17
#define HCI_VSC_DOWNLOAD_PATCH          0xFC20
#define HCI_VSC_CHECK_32K                     0xFC02
#define HCI_HCI_RESET                             0x0C03
#define HCI_VENDOR_RF_RADIO_REG_WRITE     0xfd4a
#define HCI_VSC_VENDOR_IQK        0xFD91
#define HCI_VSC_READ_THERMAL       0xFC40

#define HCI_TP_CHECK_OK           0x00
#define HCI_TP_CONFIG_END         0x01
#define HCI_TP_CONFIG_FAIL        0x02
#define HCI_TP_CHECK_AGAIN        0x03
#define HCI_TP_NOT_SEND           0x04
#define HCI_TP_CHECK_ERROR        0xFF
#define HCI_TP_CHECK_WAIT_TIMEOUT 0x05

typedef struct _HCI_PROCESS_TABLE_
{
    uint16_t    opcode;
    uint8_t (*start_pro)(void);
    uint8_t (*check_func)(uint8_t len, uint8_t *p_buf);
} HCI_PROCESS_TABLE, *PHCI_PROCESS_TABLE;

typedef bool (*P_HCI_TP_TX_CB)(void);

typedef bool (*P_HCI_TP_RX_IND)(void);


uint8_t hci_pc_read_local_ver(void);
uint8_t hci_pc_read_local_ver_check(uint8_t len, uint8_t *p_buf);
uint8_t hci_pc_read_rom_ver(void);
uint8_t hci_pc_read_rom_ver_check(uint8_t len, uint8_t *p_buf);

uint8_t hci_pc_set_controller_baudrate(void);
uint8_t hci_pc_set_baudrate_check(uint8_t len, uint8_t *p_buf);

uint8_t hci_pc_download_patch(void);
uint8_t hci_pc_download_patch_check(uint8_t len, uint8_t *p_buf);

uint8_t hci_ps_set_hci_reset(void);

bool hci_proto_send(uint8_t *p_buf, uint16_t len, P_HCI_TP_TX_CB cb);


//utils

void hci_ps_utils_baudc_to_speed(uint32_t baudc, uint32_t *speed);
void rtlbt_ps_util_speed_to_baudc(uint32_t *baudc, uint32_t speed);

void hci_ps_util_hexdump(char dir, const unsigned char *buf, size_t len);
bool hci_tp_send_hci_cmd(uint16_t opcode, uint8_t *data, uint8_t len);
bool hci_tp_send_hci_cmd_with_byte(uint16_t opcode, uint8_t index, uint8_t *data, uint8_t len);

void hci_ps_hci_num_plus(void);

bool rtkbt_set_config_param(uint8_t *config_addr, uint16_t cfg_len, uint16_t parm_offset,
                            uint8_t *param, uint8_t param_len);

bool rtkbt_get_config_param(uint8_t *config_addr, uint16_t cfg_len, uint16_t parm_offset,
                            uint8_t *param, uint8_t param_len);
bool rtkbt_delete_last_param(uint8_t *config_addr, uint16_t *cfg_len);

//adapter
extern HCI_PROCESS_TABLE hci_process_table[];


bool hci_pos_process_init(void);
bool hci_ps_start_next_process(void);
bool hci_ps_check_process(uint8_t *p_buf, uint16_t len);

bool rtk_hci_pc_init(P_HCI_TP_RX_IND rx_ind);
bool hci_ps_complete(bool flag);

bool rtk_hci_ps_deinit(void);



