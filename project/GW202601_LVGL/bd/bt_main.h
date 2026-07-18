/*
 * bt_main.h
 *
 * BT 模組對外介面，main.c 只需 include 這一個 header。
 *
 * main.c 呼叫方式：
 *   #include "bt/bt_main.h"
 *   ...
 *   #ifdef CFG_BUILD_NIMBLE
 *       nimbleInit();    // 內部自己建 thread，立即返回，不會阻塞
 *   #endif
 *   ScreenInit();        // 正常繼續執行
 */

#ifndef BT_MAIN_H
#define BT_MAIN_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================ */
/*  Gateway Status Flags (Manufacturer Data)                         */
/* ================================================================ */

#define NMGW_FLAG_WIFI_OK    (1 << 0)  /* WiFi 已連線且有 IP          */
#define NMGW_FLAG_BLE_OK     (1 << 1)  /* BLE 掃描/廣播運行中         */
#define NMGW_FLAG_MQTT_OK    (1 << 2)  /* MQTT (DCareW) 已連線        */
#define NMGW_FLAG_OTA_OK     (1 << 3)  /* MQTT OTA (DCareR) 已連線    */
#define NMGW_FLAG_WIFI_READY (1 << 4)  /* WiFi driver 初始化成功      */

/**
 * 更新 Gateway 狀態旗標並刷新 BLE 廣告中的 Manufacturer Data。
 *
 * @param flag   要更新的旗標 (NMGW_FLAG_xxx)
 * @param active true=設定, false=清除
 */
void nmgw_status_update(uint8_t flag, bool active);

/**
 * 僅設定 flag，不觸發 BLE 廣告刷新（thread-safe，可供 WiFi thread 等呼叫）。
 */
void nmgw_status_set_flag(uint8_t flag);

/* Disconnect callback types (對應 WIFIMGR_STATE_CALLBACK_E) */
#define NMGW_DISC_REASON_TEMP      1   /* TEMP_DISCONNECT          */
#define NMGW_DISC_REASON_30S       2   /* DISCONNECT_30S           */
#define NMGW_DISC_REASON_CONN_FAIL 3   /* CONNECTING_FAIL          */

/**
 * 記錄 WiFi 斷線事件（更新 reason 並遞增 counter）。
 * thread-safe，僅寫入全域變數，不呼叫 NimBLE API。
 */
void nmgw_disconnect_record(uint8_t reason);

uint8_t nmgw_get_disconnect_reason(void);
uint8_t nmgw_get_disconnect_count(void);
uint8_t nmgw_get_disc_count_temp(void);
uint8_t nmgw_get_disc_count_30s(void);
uint8_t nmgw_get_disc_count_fail(void);

/**
 * 取得目前 Gateway 狀態旗標值。
 *
 * @return NMGW_FLAG_xxx bitmask
 */
uint8_t nmgw_status_get_flags(void);

/**
 * 取得目前偵測到的唯一 MedFlo 裝置數量。
 *
 * @return 唯一裝置數 (上限 100)
 */
int nmgw_medflo_device_count(void);

/* ================================================================ */
/*  Public API                                                       */
/* ================================================================ */

/**
 * 初始化 NimBLE stack 並啟動 BLE 廣播。
 * 內部建立獨立 thread 執行，呼叫後立即返回。
 * 由 main.c 在 NetworkInit() 之後呼叫。
 */
void nimbleInit(void);

/**
 * 通知 BLE thread WiFi driver 已完成初始化，可以開始使用 UART0。
 * 由 network_wifi.c 的 NetworkWifiProcess() 在 WifiMgr_Init() 完成後呼叫。
 */
void nimbleNotifyWifiReady(void);

#ifdef __cplusplus
}
#endif

#endif /* BT_MAIN_H */
