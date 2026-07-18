/****************************************************************************
**
**  Name:          btapp_utility.h
**
**  Description:   Contains btapp utility API header file
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_UTILITY_H
#define BTAPP_UTILITY_H

#include "bd.h"

#define IS_HEX_ALPHA(x) ((x>='A' && x<='F'))
#define TO_UPPER_HEX(x)   ((x>='a' && x<='f') ? (x-32) : x)
#define TO_HEX(x)  ((IS_HEX_ALPHA(TO_UPPER_HEX(x))) ? (TO_UPPER_HEX(x) - 'A' + 10) : ((x) - '0'))

void btapp_start_timer (TIMER_LIST_ENT *p_tle, UINT16 type, UINT32 timeout);
void btapp_stop_timer(TIMER_LIST_ENT *p_tle);
void btapp_set_dev_addr(BD_ADDR bd_addr);
UINT8 *btapp_utl_keys_to_str (BT_OCTET16 keys);
UINT8* btapp_utl_bda_to_str (BD_ADDR bd_addr);
BD_ADDR* btapp_utl_str_to_bda(const char* str_bda);

#endif
