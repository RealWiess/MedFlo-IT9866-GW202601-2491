/*
 * wifi_app.c — WiFi STA for gw_lvgl_test
 */
#include "wifi_app.h"
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "ite/itp.h"
#include "wifiMgr.h"

/* ── WiFi credentials ── */
#define WIFI_SECU  4  /* secumode=4 → WPA2-AES */
static const char *ssid_list[] = { "Getnew6336", "wiess-2.4G" };
static const char *pass_list[] = { "YWMD6151", "52804417" };
#define SSID_COUNT (sizeof(ssid_list) / sizeof(ssid_list[0]))

static bool s_connected = false;
static char s_ip[16] = "--";
static char s_ssid[64] = "";

/* ── WiFi callback ── */
static int wifi_callback(int condition)
{
    switch (condition) {
    case WIFIMGR_STATE_CALLBACK_CONNECTION_FINISH:
        printf("[WiFi] ASSOCIATED\n"); break;
    case WIFIMGR_STATE_CALLBACK_CLIENT_MODE_CONNECTING_FAIL:
        printf("[WiFi] CONNECT FAIL\n"); s_connected = false; break;
    case WIFIMGR_STATE_CALLBACK_CLIENT_MODE_TEMP_DISCONNECT:
        printf("[WiFi] TEMP DISCONNECT\n"); s_connected = false; break;
    default: break;
    }
    return 0;
}

/* ── Background WiFi task ── */
static void wifi_task(void *arg)
{
    (void)arg;

    printf("[WiFi] waiting 5s for chip init...\n");
    usleep(5000 * 1000);

    /* Build settings */
    WIFI_MGR_SETTING cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.wifiCallback = wifi_callback;
    cfg.setting.dhcp = 1;
    cfg.secumode     = WIFI_SECU;
    strncpy(cfg.ssid, ssid_list[0], sizeof(cfg.ssid)-1);
    strncpy(cfg.password, pass_list[0], sizeof(cfg.password)-1);

    /* POWER_ON_OFF_USER_DEFINED=y: must init SDIO + WiFi NGPL manually */
    printf("[WiFi] init SDIO + WiFi NGPL...\n");
    itpRegisterDevice(ITP_DEVICE_SDIO, &itpDeviceSdio);
    ioctl(ITP_DEVICE_SDIO, ITP_IOCTL_INIT, NULL);
    itpRegisterDevice(ITP_DEVICE_WIFI_NGPL, &itpDeviceWifiNgpl);
    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_INIT, NULL);
    sleep(2);

    /* Enable + add netif */
    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_ENABLE, NULL);
    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_ADD_NETIF, NULL);
    printf("[WiFi] enabled + netif added\n");

    /* Init WiFi manager */
    int ret = WifiMgr_Init(WIFIMGR_MODE_CLIENT, 0, cfg);
    printf("[WiFi] WifiMgr_Init = %d\n", ret);
    sleep(5);

    /* Try each SSID (auto-connect, no explicit scan) */
    for (int i = 0; i < (int)SSID_COUNT && !s_connected; i++) {
        strncpy(s_ssid, ssid_list[i], sizeof(s_ssid)-1);
        printf("[WiFi] connecting to %s...\n", ssid_list[i]);

        /* Just wait for DHCP — wifi manager handles connection */
        int timeout = 25;
        while (timeout-- > 0 && !s_connected) {
            sleep(1);
            uint8_t *ip = WifiMgr_Get_WIFI_IP();
            if (ip && !(ip[0]==0&&ip[1]==0&&ip[2]==0&&ip[3]==0)) {
                snprintf(s_ip, sizeof(s_ip), "%d.%d.%d.%d",
                         ip[0], ip[1], ip[2], ip[3]);
                s_connected = true;
                printf("[WiFi] connected! %s IP=%s\n", s_ssid, s_ip);
                break;
            }
        }
    }
    if (!s_connected)
        printf("[WiFi] all attempts failed\n");

    while (1) { sleep(10); }
}

/* ── Public API ── */
void wifi_app_start(void)
{
    pthread_t tid; pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 8192);
    pthread_create(&tid, &attr, (void*(*)(void*))wifi_task, NULL);
    pthread_attr_destroy(&attr);
}
bool wifi_app_is_connected(void)  { return s_connected; }
const char* wifi_app_get_ssid(void) { return s_connected ? s_ssid : "scanning..."; }
const char* wifi_app_get_ip(void)   { return s_ip; }
