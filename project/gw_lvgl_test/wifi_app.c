/*
 * wifi_app.c — WiFi STA for gw_lvgl_test
 * Follows SDK 2491 test_wifi_sdio standard init flow
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
static char s_ip[16] = "0.0.0.0";
static char s_ssid[64] = "";

/* ── WiFi callback ── */
static int wifi_callback(int condition)
{
    switch (condition) {
    case WIFIMGR_STATE_CALLBACK_CONNECTION_FINISH:
        printf("[WiFi] ASSOCIATED\n");
        s_connected = true;
        break;
    case WIFIMGR_STATE_CALLBACK_CLIENT_MODE_CONNECTING_FAIL:
        printf("[WiFi] CONNECT FAIL\n");
        s_connected = false;
        break;
    case WIFIMGR_STATE_CALLBACK_CLIENT_MODE_TEMP_DISCONNECT:
        printf("[WiFi] TEMP DISCONNECT\n");
        s_connected = false;
        break;
    default:
        break;
    }
    return 0;
}

/* ── Background WiFi task (SDK 2491 standard flow) ── */
static void wifi_task(void *arg)
{
    (void)arg;

    printf("[WiFi] waiting 500ms for chip init...\n");
    usleep(500 * 1000);

    /* ── SDK 2491 STANDARD FLOW (ref: test_wifi_sdio/RTL/power_main.c WifiFirstPowerOn) ── */
    /* Step 1: Init SDIO + WiFi NGPL + ITP_IOCTL_INIT (sets wifiInited = true) */
    printf("[WiFi] SDIO init + WiFi NGPL...\n");
    itpRegisterDevice(ITP_DEVICE_SDIO, &itpDeviceSdio);
    ioctl(ITP_DEVICE_SDIO, ITP_IOCTL_INIT, NULL);
    itpRegisterDevice(ITP_DEVICE_WIFI_NGPL, &itpDeviceWifiNgpl);
    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_INIT, NULL);
    sleep(2);

    /* Build WifiMgr settings */
    WIFI_MGR_SETTING cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.wifiCallback = wifi_callback;
    cfg.setting.dhcp = 1;

    /* Step 2: Switch WiFi ON */
    printf("[WiFi] WifiMgr_Sta_Switch(ON)...\n");
    WifiMgr_Sta_Switch(WIFIMGR_SWITCH_ON);
    sleep(1);

    /* Step 2: Init WiFi manager */
    int ret = WifiMgr_Init(WIFIMGR_MODE_CLIENT, 0, cfg);
    printf("[WiFi] WifiMgr_Init = %d\n", ret);

    if (ret != WIFIMGR_ECODE_OK) {
        printf("[WiFi] Init failed, retrying after 3s...\n");
        sleep(3);
        WifiMgr_Sta_Switch(WIFIMGR_SWITCH_ON);
        sleep(1);
        ret = WifiMgr_Init(WIFIMGR_MODE_CLIENT, 0, cfg);
        printf("[WiFi] WifiMgr_Init retry = %d\n", ret);
    }

    /* Step 3: Connect to AP */
    for (int i = 0; i < (int)SSID_COUNT && !s_connected; i++) {
        strncpy(s_ssid, ssid_list[i], sizeof(s_ssid) - 1);
        printf("[WiFi] connecting to %s...\n", ssid_list[i]);

        ret = WifiMgr_Sta_Connect(ssid_list[i], pass_list[i], "4");
        printf("[WiFi] WifiMgr_Sta_Connect = %d\n", ret);

        /* Wait for DHCP — callback sets s_connected on CONNECTION_FINISH */
        int timeout = 30;
        while (timeout-- > 0 && !s_connected) {
            sleep(1);
            uint8_t *ip = WifiMgr_Get_WIFI_IP();
            if (ip && !(ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0)) {
                snprintf(s_ip, sizeof(s_ip), "%d.%d.%d.%d",
                         ip[0], ip[1], ip[2], ip[3]);
                printf("[WiFi] DHCP OK! %s IP=%s\n", s_ssid, s_ip);
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
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 8192);
    pthread_create(&tid, &attr, (void *(*)(void *))wifi_task, NULL);
    pthread_attr_destroy(&attr);
}

bool wifi_app_is_connected(void)  { return s_connected; }
const char *wifi_app_get_ssid(void) { return s_connected ? s_ssid : "scanning..."; }
const char *wifi_app_get_ip(void)   { return s_ip; }
