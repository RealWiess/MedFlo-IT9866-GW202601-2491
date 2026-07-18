/*
 * bt_wifi.c
 *
 * WiFi 掃描 → JSON 組裝 → BLE Notify 分段傳送
 * WiFi IP 取得（依 SDK 類型）
 *
 * 依賴：
 *   - wifiMgr.h   (WifiMgr_Get_Scan_AP_Info)
 *   - host/ble_hs.h (ble_gattc_notify_custom)
 *   - bt_wifi.h
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "host/ble_hs.h"
#include "wifiMgr.h"
#include "bt_wifi.h"
#include "bt_led.h"

/* ---- WiFi IP 取得（依 SDK 類型）---- */
#if defined(CFG_NET_WIFI_SDIO_VND_RTK)
#   include "lwip/ip.h"
#   include "wifi_conf.h"
    extern struct netif xnetif[NET_IF_NUM];
    extern uint8_t* LwIP_GetIP(struct netif *pnetif);
#endif

#if defined(CFG_NET_WIFI_SDIO_NGPL_AP6256) || defined(CFG_NET_WIFI_SDIO_NGPL_AP6236) \
 || defined(CFG_NET_WIFI_SDIO_NGPL_AP6212) || defined(CFG_NET_WIFI_SDIO_NGPL_AP6203)
#   include "mhd_api.h"
#   define FORMAT_IPADDR(x) \
        ((unsigned char *)&x)[3], ((unsigned char *)&x)[2], \
        ((unsigned char *)&x)[1], ((unsigned char *)&x)[0]
#endif

#ifdef CFG_MIRROR_TEST
extern bool gInDemoLayer;
#endif

/* ---- 公開變數（bt_gatt.c 寫入，本檔使用）---- */
uint16_t g_ble_scan_handle = 0;

/* ---- 內部常數 ---- */
/* BLE Notify payload 保守值（MTU 協商前用小包，避免 fragmentation） */
#define BT_WIFI_NOTIFY_PAYLOAD  60
/* 最多回報幾個 AP（避免 JSON 超過 jsonBuf） */
#define BT_WIFI_MAX_AP          64

/* ================================================================ */
/*  bt_wifi_get_ip                                                   */
/* ================================================================ */

void bt_wifi_get_ip(char *ip, int ip_len)
{
    strncpy(ip, "0.0.0.0", ip_len);

#if defined(CFG_NET_WIFI_SDIO_VND_RTK)
    unsigned char *ip_r = LwIP_GetIP(&xnetif[0]);
    if (ip_r) {
        ipaddr_ntoa_r((const ip_addr_t*)ip_r, ip, ip_len);
    }
#endif

/* ---- AP6xxx 晶片 ---- */
#if defined(CFG_NET_WIFI_SDIO_NGPL_AP6256) || defined(CFG_NET_WIFI_SDIO_NGPL_AP6236) \
 || defined(CFG_NET_WIFI_SDIO_NGPL_AP6212) || defined(CFG_NET_WIFI_SDIO_NGPL_AP6203)
    uint32_t nIp = htonl(mhd_sta_ipv4_ipaddr());
    if (nIp != 0) {
        snprintf(ip, ip_len, "%u.%u.%u.%u", FORMAT_IPADDR(nIp));
    }
#endif
}
/* ================================================================ */
/*  bt_wifi_scan_task  (pthread entry)                               */
/* ================================================================ */

void* bt_wifi_scan_task(void *arg)
{
    /* arg 是 heap-allocated uint16_t*，用完要 free */
    uint16_t conn_hdl = *(uint16_t*)arg;
    free(arg);

    /* ----------------------------------------------------------
     * 1. 執行 WiFi Scan
     *    參考 network_func.c scan_ap_loop() 的正確用法：
     *      - pList 是陣列，不是單一 struct
     *      - 回傳值是掃到的 AP 數量（< 0 表示失敗）
     * ---------------------------------------------------------- */
    WIFI_MGR_SCANAP_LIST pList[BT_WIFI_MAX_AP];
    memset(pList, 0, sizeof(pList));

    printf("[bt_wifi] Starting WiFi scan...\n");
    bt_led_set_wifi(WIFI_LED_STATE_SCANNING);  /* 橘燈快閃：掃描中 */
    int count = WifiMgr_Get_Scan_AP_Info(pList);
    if (count <= 0) {
        printf("[bt_wifi] Scan failed or no AP found (%d)\n", count);
        const char *err = "{\"err\":1}";
        struct os_mbuf *om = ble_hs_mbuf_from_flat(err, strlen(err));
        if (om)
            ble_gattc_notify_custom(conn_hdl, g_ble_scan_handle, om);
        pthread_exit(NULL);
    }
    printf("[bt_wifi] Found %d APs\n", count);

    /* ----------------------------------------------------------
     * 2. 組 JSON 字串
     *    格式：[{"s":"SSID","r":-65,"m":4}, ...]
     *      s = ssid
     *      r = rssi (dBm)
     *      m = security mode
     * ---------------------------------------------------------- */
    char jsonBuf[2048];
    int  pos = 0;

    pos += snprintf(jsonBuf + pos, sizeof(jsonBuf) - pos, "[");
    for (int i = 0; i < count && pos < (int)sizeof(jsonBuf) - 80; i++) {
        if (i > 0)
            pos += snprintf(jsonBuf + pos, sizeof(jsonBuf) - pos, ",");
            pos += snprintf(jsonBuf + pos, sizeof(jsonBuf) - pos,
                            "{\"s\":\"%s\",\"r\":%d,\"m\":%ld}", // 註: securityMode 通常是 unsigned long，格式用 %ld
                            pList[i].ssidName,
                            pList[i].rfQualityRSSI,
                            pList[i].securityMode);
    }
    pos += snprintf(jsonBuf + pos, sizeof(jsonBuf) - pos, "]");
    printf("[bt_wifi] JSON ready (%d bytes)\n", pos);

    /* ----------------------------------------------------------
     * 3. 分段 Notify
     *    封包格式：[ seq(1B) | total(1B) | JSON片段... ]
     *    電腦端依 seq 順序重組，收齊 total 包後 parse JSON。
     * ---------------------------------------------------------- */
    int total_pkts = (pos + BT_WIFI_NOTIFY_PAYLOAD - 1) / BT_WIFI_NOTIFY_PAYLOAD;
    int sent       = 0;
    int seq        = 0;

    while (sent < pos) {
        int chunk = pos - sent;
        if (chunk > BT_WIFI_NOTIFY_PAYLOAD)
            chunk = BT_WIFI_NOTIFY_PAYLOAD;

        uint8_t pkt[BT_WIFI_NOTIFY_PAYLOAD + 2];
        pkt[0] = (uint8_t)seq;
        pkt[1] = (uint8_t)total_pkts;
        memcpy(&pkt[2], jsonBuf + sent, chunk);

        struct os_mbuf *om = ble_hs_mbuf_from_flat(pkt, chunk + 2);
        if (om)
            ble_gattc_notify_custom(conn_hdl, g_ble_scan_handle, om);

        sent += chunk;
        seq++;
        usleep(50 * 1000); /* 50ms 間隔，避免 BLE congestion */
    }
    printf("[bt_wifi] Scan notify done (%d packets)\n", seq);
    bt_led_set_wifi(WIFI_LED_STATE_OFF);  /* 掃描完畢，等待使用者選擇 */

    pthread_exit(NULL);
    return NULL;
}

/* ================================================================ */
/*  bt_wifi_get_mac                                                  */
/* ================================================================ */
void bt_wifi_get_mac(char *mac_str, int mac_len)
{
    static char s_cached_mac[20] = "00:00:00:00:00:00";
    static bool s_mac_ready = false;

    if (!s_mac_ready) {
#if defined(CFG_NET_WIFI_SDIO_VND_RTK)
        if (xnetif[0].hwaddr_len == 6) {
            bool all_zero = true;
            for (int i = 0; i < 6; i++) {
                if (xnetif[0].hwaddr[i] != 0) { all_zero = false; break; }
            }
            if (!all_zero) {
                snprintf(s_cached_mac, sizeof(s_cached_mac), "%02X:%02X:%02X:%02X:%02X:%02X",
                         xnetif[0].hwaddr[0], xnetif[0].hwaddr[1], xnetif[0].hwaddr[2],
                         xnetif[0].hwaddr[3], xnetif[0].hwaddr[4], xnetif[0].hwaddr[5]);
                s_mac_ready = true;
            }
        }
#endif
    }

    strncpy(mac_str, s_cached_mac, mac_len);
}
