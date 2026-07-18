/*******************************************************************************
**  Name:       pf_trans.h
**
**  Description:
**
**  This file contains platform transport layer Drivers API.
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
*******************************************************************************/

#ifndef PF_TRANS_H
#define PF_TRANS_H

#include <stdint.h>

#define PF_TRANS_STATUS_READY  1
#define PF_TRANS_STATUS_STOP   0

extern int   pf_trans_init(uint32_t baud);
extern void  pf_trans_deinit(void);
extern uint8_t pf_trans_get_status(void);
extern void pf_trans_reconfig_baud(uint32_t baud);
extern int32_t pf_trans_send(unsigned char * data, unsigned int length);
extern int32_t pf_trans_receive(unsigned char * data, unsigned int length, unsigned int timeout);

#endif