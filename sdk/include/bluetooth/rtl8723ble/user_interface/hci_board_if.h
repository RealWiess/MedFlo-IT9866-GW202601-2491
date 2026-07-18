/**
 * Copyright (C) 2017 Realtek Semiconductor Corporation
 * Author: Thomas_li <thomas_li@realsil.com.cn>
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#define BTRTL_FL_FW_FREE      (1 << 0)
#define BTRTL_FL_CONFIG_FREE  (1 << 1)

#define RTLBT_FL_EXT_BDADDR (1 << 0)
#define RTLBT_FL_SET_BAUDRATE  (1 << 2)
#define RTLBT_FL_FW_LOG  (1 << 3)
#define RTLBT_FL_FW_FLOW_CTRL  (1 << 4)
#define RTLBT_FL_FW_PATCH  (1 << 5)
#define RTLBT_FL_FW_CONFIG  (1 << 6)
#define RTLBT_FL_UPSTACK  (1 << 7)
typedef struct
{
    uint8_t             bdaddr[6];
    uint32_t            baudrate; //115200
    bool                fw_log;
    bool                flow_ctrl;
    uint32_t            change_flag;
    uint8_t             proto;

    uint8_t            *fw_buf;
    uint16_t            fw_len;
    uint8_t            *config_buf;
    uint16_t            config_len;

    uint32_t            hci_debug_level;

    void                *baudrate_timer_handle;
    bool                baud_changing;
    char                hci_stack_version[10];
    uint32_t            debug_level;

} T_HCI_PROCESS_INFO;

bool rtk_hci_set_hci_task(uint16_t *stack_size, uint16_t *priority);
bool rtk_hci_board_init(T_HCI_PROCESS_INFO *ps_info);
bool rtk_hci_board_complete(void);

bool rtkbt_reset(void);
bool rtkbt_power_off(void);



