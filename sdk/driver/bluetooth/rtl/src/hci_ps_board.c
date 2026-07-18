/**
 * Copyright (C) 2017 Realtek Semiconductor Corporation
 * Author: Thomas_li <thomas_li@realsil.com.cn>
 */


#include "bt_board.h"
#include "hci_board_if.h"
#include <stdarg.h>
#include "hci_tp_dbg.h"


///=======================rtkbt=board_set===================================
bool rtk_hci_set_hci_task(uint16_t *stack_size, uint16_t *priority)
{
    //use the default
    hci_tp_warn("\r\nHCI TASK:PRIORYT: %d STACKSIZE:%d\n", *priority, *stack_size);
    return false;
}
bool rtk_hci_board_init(T_HCI_PROCESS_INFO *ps_info)
{
    hci_tp_debug_set_level(HCI_TP_DEBUG_ALL);
    //hci_tp_debug_set_level(0xffffffff);
#ifdef HCI_CHANGE_BAUDRATE_NOT_READY
    ps_info->baudrate = 115200;
    ps_info->change_flag |= RTLBT_FL_SET_BAUDRATE;
    //return false use the default;
    hci_tp_info("\r\nBT init start\n");
    return true;
#else
    hci_tp_debug_set_level(HCI_TP_DEBUG_ALL);
    //hci_tp_debug_set_level(0xffffffff);
    hci_tp_info("\r\nBT init start, HCI_STACK_VERSION:%s\n", ps_info->hci_stack_version);
    return true;
#endif
}

bool rtk_hci_board_complete(void)
{
    hci_tp_info("\r\nBT download patch success start upperstack\n");
    return true;
}
//=========================trace======B=======================================

