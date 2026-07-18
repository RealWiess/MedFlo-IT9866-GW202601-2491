/*
 * bt_gatt.h
 *
 * BT 模組內部：GATT Server 初始化介面。
 * 僅供 bt_main.c 使用。
 */

#ifndef BT_GATT_H
#define BT_GATT_H

#include "host/ble_hs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 0xAAA1 notify handle（IP 通知）。
 * bt_main.c 的 notify thread（taskA）使用此 handle 送出 IP。
 */
extern uint16_t g_ble_ip_handle;

/**
 * 註冊 GATT register callback，由 bt_main.c 傳給 ble_hs_cfg。
 */
void bt_gatt_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);

/**
 * 初始化並註冊所有 GATT service / characteristic。
 * 由 bt_main.c 在 nimble_port_init() 之後呼叫。
 *
 * @return 0 成功，非 0 失敗。
 */
int bt_gatt_init(void);

#ifdef __cplusplus
}
#endif

#endif /* BT_GATT_H */
