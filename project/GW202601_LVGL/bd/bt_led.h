/*
 * bt_led.h
 *
 * LED 狀態指示
 *   綠燈 GPIO32 — BT 狀態
 *   橘燈 GPIO31 — WiFi 狀態
 *
 * 燈號對照：
 * ┌─────────────────────────┬──────────────┬──────────────┐
 * │ 系統狀態                │ 綠燈(BT)     │ 橘燈(WiFi)   │
 * ├─────────────────────────┼──────────────┼──────────────┤
 * │ 開機初始化              │ 滅           │ 滅           │
 * │ BT 廣播中（等待連線）   │ 慢閃 1s      │ 滅           │
 * │ BT 已連線               │ 常亮         │ 滅           │
 * │ WiFi 掃描中             │ 常亮         │ 快閃 0.2s    │
 * │ WiFi 連線中             │ 常亮         │ 慢閃 0.5s    │
 * │ WiFi 已連線（取得IP）   │ 常亮         │ 常亮         │
 * │ BT 斷線                 │ 慢閃 1s      │ 依WiFi狀態   │
 * │ WiFi 斷線               │ 依BT狀態     │ 快閃 0.5s    │
 * └─────────────────────────┴──────────────┴──────────────┘
 */

#ifndef BT_LED_H
#define BT_LED_H

#ifdef __cplusplus
extern "C" {
#endif

/* BT 狀態 */
typedef enum {
    BT_LED_STATE_OFF        = 0,  /* 未初始化     */
    BT_LED_STATE_ADV        = 1,  /* 廣播中       */
    BT_LED_STATE_CONNECTED  = 2,  /* 已連線       */
} BtLedState;

/* WiFi 狀態 */
typedef enum {
    WIFI_LED_STATE_OFF       = 0, /* 未連線       */
    WIFI_LED_STATE_SCANNING  = 1, /* 掃描中       */
    WIFI_LED_STATE_CONNECTING= 2, /* 連線中       */
    WIFI_LED_STATE_CONNECTED = 3, /* 已連線取得IP */
    WIFI_LED_STATE_LOST      = 4, /* 連線中斷     */
} WifiLedState;

/**
 * 初始化 LED GPIO，啟動 LED 控制 thread。
 * 在 nimbleInit() 之前呼叫。
 */
void bt_led_init(void);

/**
 * 更新 BT LED 狀態。
 * 由 bt_main.c 的 GAP event handler 呼叫。
 */
void bt_led_set_bt(BtLedState state);

/**
 * 更新 WiFi LED 狀態。
 * 由 network_wifi.c 的 wifiCallbackFunction 呼叫。
 */
void bt_led_set_wifi(WifiLedState state);

#ifdef __cplusplus
}
#endif

#endif /* BT_LED_H */
