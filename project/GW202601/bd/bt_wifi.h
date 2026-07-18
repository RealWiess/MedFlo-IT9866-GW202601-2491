/*
 * bt_wifi.h
 *
 * BT 模組內部：WiFi scan、Notify 傳輸介面。
 * 僅供 bt_gatt.c 使用，不對外暴露。
 */

#ifndef BT_WIFI_H
#define BT_WIFI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 0xAAA6 的 GATT val_handle，由 bt_gatt.c 註冊後寫入，
 * bt_wifi.c 的 notify thread 使用它送出資料。
 */
extern uint16_t g_ble_scan_handle;

/**
 * WiFi Scan pthread entry point。
 *
 * 由 bt_gatt.c 在收到 "SCAN" 命令時，以 detached thread 呼叫。
 *
 * @param arg  heap-allocated uint16_t*，內含 BLE conn_handle，
 *             函式內部負責 free()。
 * @return     NULL
 *
 * 流程：
 *   1. WifiMgr_Get_Scan_AP_Info(pList) 取得 AP 列表
 *   2. 組成 JSON 字串
 *   3. 分段 ble_gattc_notify_custom() 送給 Central
 *
 * 封包格式：[ seq(1B) | total(1B) | JSON片段 ]
 * JSON 格式：[{"s":"SSID","r":-65,"m":4}, ...]
 *   s = ssid, r = rssi(dBm), m = security mode
 */
void* bt_wifi_scan_task(void *arg);

/**
 * 取得目前 WiFi IP 字串。
 * 依編譯時的 SDK 類型（RTK / MHD / AP6xxx）自動選擇正確 API。
 *
 * @param ip      輸出緩衝，建議 16 bytes。
 * @param ip_len  緩衝大小。
 */
void bt_wifi_get_ip(char *ip, int ip_len);
void bt_wifi_get_mac(char *mac_str, int mac_len);

#ifdef __cplusplus
}
#endif

#endif /* BT_WIFI_H */
