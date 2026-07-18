/****************************************************************************
**
**  Name:          btapp.h
**
**  Description:   btapp header file
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_H
#define BTAPP_H

#include "stdlib.h"
#include "stdint.h"

int8_t btapp_init(int argc, const char **argv);
int8_t btapp_deinit(int argc, const char **argv);

int8_t btapp_ble_start_adv(int argc, const char **argv);
int8_t btapp_ble_stop_adv(int argc, const char **argv);
int8_t btapp_ble_start_scan(int argc, const char **argv);
int8_t btapp_ble_stop_scan(int argc, const char **argv);
int8_t btapp_discoverity_enable(int argc, const char **argv);
int8_t btapp_discoverity_disable(int argc, const char **argv);

int8_t btapp_gatt_server_start(int argc, const char **argv);
int8_t btapp_gatt_server_stop(int argc, const char **argv);
int8_t btapp_gatt_test_send(int argc, const char **argv);

int8_t btapp_ble_ibeacon_start(int argc, const char **argv);
int8_t btapp_ble_ibeacon_stop(int argc, const char **argv);
int8_t btapp_ble_ibeacon_adv(int argc, const char **argv);

int8_t btapp_gatt_client_start(int argc, const char **argv);
int8_t btapp_gatt_client_stop(int argc, const char **argv);
int8_t btapp_gatt_client_connect(int argc, const char **argv);
int8_t btapp_gatt_client_disconnect(int argc, const char **argv);

int8_t btapp_spp_register_idx(int argc, const char **argv);
int8_t btapp_spp_register_nums(int argc, const char **argv);
int8_t btapp_spp_deregister_all(int argc, const char **argv);
int8_t btapp_spp_test_send(int argc, const char **argv);
int8_t btapp_spp_test_send_tht(int argc, const char **argv);

int8_t btapp_debug_print_gki_free(int argc, const char **argv);
int8_t btapp_debug_print_gki_alloced(int argc, const char **argv);
int8_t btapp_debug_lpm_mode_set(int argc, const char **argv);
int8_t btapp_stack_stress_test(int argc, const char **argv);
int8_t btapp_trace_ctrl(int argc, const char **argv);

#endif
