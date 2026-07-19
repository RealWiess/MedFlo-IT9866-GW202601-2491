/*
 * bt_main.c
 *
 * BLE 初始化、廣播、GAP event 處理、IP 定期 Notify。
 *
 * main.c 呼叫方式（直接呼叫，不需自己建 thread）：
 *
 *   #ifdef CFG_BUILD_NIMBLE
 *       nimbleInit();       // ← 內部自己建 thread，立即返回
 *   #endif
 *   ScreenInit();           // ← 正常繼續執行
 *
 * 依賴：
 *   - bt_main.h, bt_gatt.h, bt_wifi.h
 *   - nimble/ble.h, host/ble_hs.h, host/util/util.h
 *   - ctrlboard.h (theConfig)
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "host/ble_gap.h"
#include "services/gap/ble_svc_gap.h"
#include "nimble/nimble_port.h"
#include "modlog/modlog.h"

#include "ctrlboard.h"
#include "bt_main.h"
#include "bt_gatt.h"
#include "bt_wifi.h"
#include "bt_led.h"
#include "bt_log.h"

#undef printf
#define printf app_printf

/* ================================================================ */
/*  前向宣告                                                         */
/* ================================================================ */

static int bt_gap_event(struct ble_gap_event *event, void *arg);
static void bt_scan_start(void);
static void bt_handle_disc(struct ble_gap_event *event);

/* ================================================================ */
/*  內部靜態變數                                                     */
/* ================================================================ */

static uint16_t s_conn_handle  = 0;
static bool     s_notify_state = false;  /* Central 有沒有開啟 Notify 訂閱 */
static bool     s_notify_run   = false;  /* IP notify thread 是否正在執行  */

/* WiFi driver 初始化完成信號，避免 BLE HCI 與 WiFi 同時搶 UART0 */
static sem_t s_wifi_ready_sem;

bool g_ble_started = false;
bool g_ble_sem_passed = false;
int  g_ble_disc_total = 0;   // 收到的 BLE 廣播總數
int  g_ble_disc_passed = 0;  // 通過 MEDFLO 過濾的數量
int  g_ble_init_step = 0;    // BLE 初始化步驟追蹤 (0=未開始, 5=完成)

/* 唯一 MEDFLO 裝置追蹤 (避免重複計算同一台裝置) */
#define MEDFLO_MAX_DEVICES 100
static uint64_t s_medflo_macs[MEDFLO_MAX_DEVICES];
static int      s_medflo_unique_count = 0;

int nmgw_medflo_device_count(void)
{
    return s_medflo_unique_count;
}

static void medflo_track_mac(const uint8_t mac[6])
{
    /* 將 6 bytes MAC 壓成 64-bit key */
    uint64_t key = 0;
    for (int i = 0; i < 6; i++)
        key = (key << 8) | mac[i];

    /* 查重 */
    for (int i = 0; i < s_medflo_unique_count; i++) {
        if (s_medflo_macs[i] == key)
            return;  /* 已存在，不重複計算 */
    }

    /* 新增 */
    if (s_medflo_unique_count < MEDFLO_MAX_DEVICES) {
        s_medflo_macs[s_medflo_unique_count++] = key;
    } else {
        /* 滿了，用最舊的 slot (FIFO 取代) */
        static int idx = 0;
        s_medflo_macs[idx] = key;
        idx = (idx + 1) % MEDFLO_MAX_DEVICES;
    }
}

/* ================================================================ */
/*  Manufacturer Data 結構 (Gateway 狀態上報)                        */
/* ================================================================ */

struct nmgw_manufacturer_data {
    uint16_t company_id;         /* 0xFFFF (Bluetooth SIG 測試用 ID) */
    uint8_t  status_flags;       /* NMGW_FLAG_xxx bitmask             */
    uint8_t  disconnect_reason;  /* 上次斷線的 callback type          */
    uint8_t  disconnect_total;   /* 累計斷線總次數 (TEMP+30S+FAIL)    */
};

uint8_t s_nmgw_status_flags = 0;
uint8_t s_nmgw_disconnect_reason = 0;
uint8_t s_nmgw_disc_count_temp = 0;     /* TEMP_DISCONNECT 次數      */
uint8_t s_nmgw_disc_count_30s = 0;      /* DISCONNECT_30S 次數       */
uint8_t s_nmgw_disc_count_fail = 0;     /* CONNECTING_FAIL 次數      */
static volatile bool s_adv_dirty = false; /* disconnect counter 變更，需刷新廣告 */
static char  s_nmgw_mac_suffix[9] = "";  /* cached MAC 後 4 bytes hex */

/* ================================================================ */
/*  廣播欄位設定（獨立函式，供 status update 重複呼叫）               */
/* ================================================================ */

static int bt_adv_set_fields(void)
{
    struct ble_hs_adv_fields fields;
    char    name[32] = "NMGW2601-";
    char    tmp[3]   = "";
    int     rc;

    /* 使用 WiFi MAC 後 4 bytes (晶片唯一)，首次讀取後 cache 避免跳動 */
    if (s_nmgw_mac_suffix[0] == '\0') {
        char mac_str[20];
        bt_wifi_get_mac(mac_str, sizeof(mac_str));
        unsigned int m[6] = {0};
        sscanf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
               &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]);
        snprintf(s_nmgw_mac_suffix, sizeof(s_nmgw_mac_suffix),
                 "%02X%02X%02X%02X", m[2], m[3], m[4], m[5]);
    }
    strcat(name, s_nmgw_mac_suffix);

    /* 檢查 WiFi 狀態：IP 不為 0.0.0.0 代表已連線 */
    {
        char ip[16];
        bt_wifi_get_ip(ip, sizeof(ip));
        if (strcmp(ip, "0.0.0.0") != 0)
            s_nmgw_status_flags |= NMGW_FLAG_WIFI_OK;
        else
            s_nmgw_status_flags &= ~NMGW_FLAG_WIFI_OK;
    }

    struct nmgw_manufacturer_data mfg;
    mfg.company_id        = 0xFFFF;
    mfg.status_flags      = s_nmgw_status_flags;
    mfg.disconnect_reason = s_nmgw_disconnect_reason;
    mfg.disconnect_total  = s_nmgw_disc_count_temp + s_nmgw_disc_count_30s
                            + s_nmgw_disc_count_fail;

    memset(&fields, 0, sizeof(fields));
    fields.flags             = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name              = (uint8_t*)name;
    fields.name_len          = strlen(name);
    fields.name_is_complete  = 1;
    fields.mfg_data          = (uint8_t*)&mfg;
    fields.mfg_data_len      = sizeof(mfg);

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "[bt_main] set adv fields failed: %d\n", rc);
    }
    return rc;
}

void nmgw_status_update(uint8_t flag, bool active)
{
    if (active)
        s_nmgw_status_flags |= flag;
    else
        s_nmgw_status_flags &= ~flag;

    /* 如果 BLE 已啟動，立即刷新廣告中的 Manufacturer Data */
    if (g_ble_started) {
        bt_adv_set_fields();
    }
}

/**
 * 僅設定 flag，不觸發 BLE 廣告刷新。
 * 供非 BLE thread（如 WiFi thread）安全呼叫，避免跨 thread 存取 NimBLE API。
 * Flag 會在下一次 bt_adv_set_fields() 或 bt_advertise() 時自然反映。
 */
void nmgw_status_set_flag(uint8_t flag)
{
    s_nmgw_status_flags |= flag;
}

/**
 * 記錄 WiFi 斷線事件（thread-safe）。
 * 會寫入 disconnect_reason 並遞增 disconnect_count。
 * 不會呼叫 NimBLE API，可在任何 thread 中安全使用。
 */
void nmgw_disconnect_record(uint8_t reason)
{
    s_nmgw_disconnect_reason = reason;
    switch (reason) {
        case NMGW_DISC_REASON_TEMP:      s_nmgw_disc_count_temp++; break;
        case NMGW_DISC_REASON_30S:       s_nmgw_disc_count_30s++;  break;
        case NMGW_DISC_REASON_CONN_FAIL: s_nmgw_disc_count_fail++; break;
        default: break;
    }
    s_adv_dirty = true;  // 通知 BLE thread 下次刷新廣告
}

uint8_t nmgw_status_get_flags(void)
{
    return s_nmgw_status_flags;
}

uint8_t nmgw_get_disconnect_reason(void)
{
    return s_nmgw_disconnect_reason;
}

uint8_t nmgw_get_disconnect_count(void)
{
    return s_nmgw_disc_count_temp + s_nmgw_disc_count_30s
           + s_nmgw_disc_count_fail;
}

uint8_t nmgw_get_disc_count_temp(void) { return s_nmgw_disc_count_temp; }
uint8_t nmgw_get_disc_count_30s(void)  { return s_nmgw_disc_count_30s;  }
uint8_t nmgw_get_disc_count_fail(void) { return s_nmgw_disc_count_fail; }

/* ================================================================ */
/*  廣播（Advertising）                                              */
/* ================================================================ */

/*
 * 廣播名稱格式：NMGW2601- + MAC 後 4 bytes = "NMGW2601-XXXXXXXX"
 * 用 MAC 後 4 bytes 確保名稱在 31 bytes BLE payload 限制內。
 * Manufacturer Data: Company ID + Gateway status flags。
 */
/*
 * 設定 Scan Response：攜帶 FW 版本號，供 BLE Scanner 遠端辨識。
 * 放在 Scan Response 不佔用主廣告 31 bytes 限制。
 * 格式：Manufacturer Data (0xFF) + Company ID 0xFFFF + FW_BUILD_TIME
 */
static void bt_adv_set_scan_rsp(void)
{
    struct ble_hs_adv_fields rsp;
    /* FW_BUILD_TIME (15 chars) + Company ID (2) = 17 bytes payload */
    uint8_t mfg[32];
    const char *fw = FW_BUILD_TIME;
    int fw_len = strlen(fw);
    if (fw_len > 28) fw_len = 28;  // safety cap: 31 - 1(len) - 1(type) - 2(cid) = 27 usable

    mfg[0] = 0xFF;                  // Company ID LSB
    mfg[1] = 0xFF;                  // Company ID MSB
    memcpy(&mfg[2], fw, fw_len);    // FW version string

    memset(&rsp, 0, sizeof(rsp));
    rsp.mfg_data     = mfg;
    rsp.mfg_data_len = 2 + fw_len;

    int rc = ble_gap_adv_rsp_set_fields(&rsp);
    if (rc != 0) {
        printf("[bt_main] Scan rsp set failed: %d\n", rc);
    } else {
        printf("[bt_main] Scan rsp: FW=%s\n", fw);
    }
}

static void bt_advertise(void)
{
    struct ble_gap_adv_params adv_params;
    int     rc;

    /* 設定廣播欄位（名稱、Manufacturer Data） */
    if (bt_adv_set_fields() != 0)
        return;

    /* 設定 Scan Response（FW 版本號） */
    bt_adv_set_scan_rsp();

    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    adv_params.itvl_min = BLE_GAP_ADV_ITVL_MS(100);   // 100ms 廣播一次，狀態監控
    adv_params.itvl_max = BLE_GAP_ADV_ITVL_MS(150);   // 加入隨機延遲避免持續碰撞

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                           &adv_params, bt_gap_event, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "[bt_main] adv start failed: %d\n", rc);
        return;
    }
    printf("[bt_main] Advertising active (flags=0x%02X)\n", s_nmgw_status_flags);
}

/* ================================================================ */
/*  IP Notify Thread                                                 */
/* ================================================================ */

/*
 * 每 1 秒送一次目前 WiFi IP 給已訂閱的 Central（0xAAA1 Notify）。
 */
static void* bt_ip_notify_task(void *arg)
{
    while (1) {
        usleep(1000 * 1000);

        /* 若 disconnect counter 有變更，刷新 BLE 廣告中的 DC 值 */
        if (s_adv_dirty) {
            s_adv_dirty = false;
            if (g_ble_started) {
                bt_adv_set_fields();
            }
        }

        if (!s_notify_run || !s_notify_state)
            continue;

        char ip[16];
        bt_wifi_get_ip(ip, sizeof(ip));

        struct os_mbuf *om = ble_hs_mbuf_from_flat(ip, sizeof(ip));
        if (om)
            ble_gattc_notify_custom(s_conn_handle, g_ble_ip_handle, om);
    }
    return NULL;
}

/* ================================================================ */
/*  GAP Event Handler                                                */
/* ================================================================ */

static int bt_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    int rc;

    // 僅記錄非 DISCOVERY 事件（DISC=7 每秒數十次，會淹沒 ring buffer）
    if (event->type != BLE_GAP_EVENT_DISC) {
        printf("[bt_main] GAP event: %d\n", event->type);
    }

    switch (event->type) {

    case BLE_GAP_EVENT_CONNECT:
        MODLOG_DFLT(INFO, "[bt_main] Connection %s; status=%d\n",
                    event->connect.status == 0 ? "established" : "failed",
                    event->connect.status);
        if (event->connect.status == 0) {
            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            assert(rc == 0);
            s_conn_handle = event->connect.conn_handle;
            s_notify_run  = true;
            bt_led_set_bt(BT_LED_STATE_CONNECTED);
        } else {
            bt_advertise();
            s_conn_handle = 0;
            s_notify_run  = false;
            bt_led_set_bt(BT_LED_STATE_ADV);
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        MODLOG_DFLT(INFO, "[bt_main] Disconnect; reason=%d\n",
                    event->disconnect.reason);
        s_notify_run   = false;
        s_notify_state = false;
        s_conn_handle  = 0;
        bt_led_set_bt(BT_LED_STATE_ADV);
        bt_advertise();
        return 0;

    case BLE_GAP_EVENT_CONN_UPDATE:
        MODLOG_DFLT(INFO, "[bt_main] Conn update; status=%d\n",
                    event->conn_update.status);
        return 0;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        MODLOG_DFLT(INFO, "[bt_main] Adv complete; reason=%d\n",
                    event->adv_complete.reason);
        bt_advertise();
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
        MODLOG_DFLT(INFO, "[bt_main] Subscribe; handle=%d cur_notify=%d\n",
                    event->subscribe.attr_handle,
                    event->subscribe.cur_notify);
        if (event->subscribe.attr_handle == g_ble_ip_handle)
            s_notify_state = event->subscribe.cur_notify;
        return 0;

    case BLE_GAP_EVENT_MTU:
        MODLOG_DFLT(INFO, "[bt_main] MTU update; mtu=%d\n",
                    event->mtu.value);
        return 0;

    case BLE_GAP_EVENT_ENC_CHANGE:
        MODLOG_DFLT(INFO, "[bt_main] Enc change; status=%d\n",
                    event->enc_change.status);
        return 0;

    case BLE_GAP_EVENT_REPEAT_PAIRING:
        rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        assert(rc == 0);
        ble_store_util_delete_peer(&desc.peer_id_addr);
        return BLE_GAP_REPEAT_PAIRING_RETRY;

    case BLE_GAP_EVENT_DISC:
        bt_handle_disc(event);
        return 0;

    default:
        return 0;
    }
}

/* ================================================================ */
/*  NimBLE Callbacks                                                 */
/* ================================================================ */

static void on_reset(int reason)
{
    MODLOG_DFLT(ERROR, "[bt_main] NimBLE reset; reason=%d\n", reason);
    printf("[bt_main] ERROR: NimBLE controller reset! reason=%d\n", reason);
}

static void on_sync(void)
{
    printf("[bt_main] NimBLE host-controller sync OK\n");
    g_ble_init_step = 5;

    /* 將 WiFi MAC 設為 NimBLE public address，確保 BLE 廣播地址不變 */
    {
        char mac_str[20];
        bt_wifi_get_mac(mac_str, sizeof(mac_str));
        uint8_t pub_addr[6];
        sscanf(mac_str, "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX",
               &pub_addr[5], &pub_addr[4], &pub_addr[3],
               &pub_addr[2], &pub_addr[1], &pub_addr[0]);
        ble_hs_id_set_rnd(pub_addr);
        printf("[bt_main] BLE public addr set to WiFi MAC: %s\n", mac_str);
    }

    int rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);
    bt_led_set_bt(BT_LED_STATE_ADV);
    bt_advertise();
    bt_scan_start();
    g_ble_started = true;

    /* 設定 BLE 狀態旗標 */
    nmgw_status_update(NMGW_FLAG_BLE_OK, true);

    printf("[bt_main] BLE scan started, advertising active, flags=0x%02X\n",
           s_nmgw_status_flags);
}

/* ================================================================ */
/*  BLE 主執行緒（內部，阻塞）                                       */
/* ================================================================ */

static void* bt_ble_task(void *arg)
{
    int rc;

    /* 0. 等待 WiFi 連線完成（避免 WiFi scan 與 BLE 搶 RTL8821 RF）
     *    main.c 已經先跑 NetworkInit()，WiFi 可能已連上或正在連。
     *    最多等 30 秒，之後強制啟動 BLE（WiFi 失敗不該癱瘓 BLE）。 */
    g_ble_init_step = 1;
    printf("[bt_main] BLE task started, waiting for WiFi connection...\n");
    {
        extern bool NetworkWifiIsReady(void);
        int waited = 0;
        while (!NetworkWifiIsReady() && waited < 30) {
            sleep(1);
            waited++;
        }
        if (NetworkWifiIsReady())
            printf("[bt_main] WiFi connected after %ds, starting BLE\n", waited);
        else
            printf("[bt_main] WiFi not ready after %ds, starting BLE anyway\n", waited);
    }
    g_ble_sem_passed = true;
    g_ble_init_step = 2;
    printf("[bt_main] WiFi wait done, initializing NimBLE port...\n");

    /* 0.5. 初始化 Log Buffer */
    bt_log_init();
    printf("[bt_main] Log buffer ready\n");

    /* 1. 初始化 NimBLE port */
    printf("[bt_main] Calling nimble_port_init()...\n");
    nimble_port_init();
    g_ble_init_step = 3;
    printf("[bt_main] nimble_port_init() completed\n");

    /* 2. 設定 host callbacks */
    ble_hs_cfg.reset_cb          = on_reset;
    ble_hs_cfg.sync_cb           = on_sync;
    ble_hs_cfg.gatts_register_cb = bt_gatt_register_cb;
    ble_hs_cfg.store_status_cb   = ble_store_util_status_rr;

    /* 3. 初始化 GATT server */
    rc = bt_gatt_init();
    assert(rc == 0);
    printf("[bt_main] GATT server ready\n");

    /* 4. 啟動 IP Notify thread */
    pthread_t      ip_tid;
    pthread_attr_t ip_attr;
    pthread_attr_init(&ip_attr);
    pthread_attr_setdetachstate(&ip_attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&ip_attr, 8 * 1024);
    pthread_create(&ip_tid, &ip_attr, bt_ip_notify_task, NULL);
    printf("[bt_main] IP notify thread started\n");

    /* 5. 啟動 NimBLE event loop（阻塞，不會返回） */
    g_ble_init_step = 4;
    printf("[bt_main] Entering nimble_port_run() - waiting for controller sync...\n");
    nimble_port_run();
    printf("[bt_main] nimble_port_run() exited unexpectedly!\n");

    return NULL;
}

/* ================================================================ */
/*  nimbleInit  — 對外唯一 entry point                              */
/*                                                                   */
/*  main.c 直接呼叫，內部建 thread 後立即返回：                      */
/*    #ifdef CFG_BUILD_NIMBLE                                        */
/*        nimbleInit();    ? 不會阻塞                               */
/*    #endif                                                         */
/*    ScreenInit();        ? 正常繼續                               */
/* ================================================================ */

void nimbleInit(void)
{
    pthread_t      bt_tid;
    pthread_attr_t bt_attr;

    /* 初始化 semaphore，初始值 0（bt_ble_task 會 wait，直到 WiFi 通知） */
    sem_init(&s_wifi_ready_sem, 0, 0);

    pthread_attr_init(&bt_attr);
    pthread_attr_setdetachstate(&bt_attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&bt_attr, 32 * 1024);
    pthread_create(&bt_tid, &bt_attr, bt_ble_task, NULL);

    printf("[bt_main] BLE thread started\n");
    /* 立即返回，main.c 繼續執行 ScreenInit() 等後續初始化 */
}

void nimbleNotifyWifiReady(void)
{
    printf("[bt_main] WiFi init done, releasing BLE thread\n");
    sem_post(&s_wifi_ready_sem);
}

/* ================================================================ */
/*  BLE 掃描啟動與廣播過濾解析                                       */
/* ================================================================ */

static void bt_scan_start(void)
{
    struct ble_gap_disc_params disc_params;
    uint8_t own_addr_type;
    int rc;

    memset(&disc_params, 0, sizeof(disc_params));
    disc_params.filter_duplicates = 0;  // 接收重複廣播以維持即時狀態與 RSSI 更新
    disc_params.passive = 0;            // 主動掃描（Active Scanning），才能收到 Scan Response 中的名字！
    disc_params.itvl = 320;             // 掃描間隔 200ms (320 × 0.625ms)
    disc_params.window = 64;            // 掃描視窗 40ms (64 × 0.625ms, 20% duty cycle)

    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        printf("[bt_main] Infer own address type failed: %d\n", rc);
        return;
    }

    rc = ble_gap_disc(own_addr_type, BLE_HS_FOREVER, &disc_params, bt_gap_event, NULL);
    if (rc != 0) {
        printf("[bt_main] Start BLE scanning failed: %d\n", rc);
    } else {
        printf("[bt_main] BLE scanning started successfully.\n");
    }
}

static void bt_handle_disc(struct ble_gap_event *event)
{
    const uint8_t *ad = event->disc.data;
    int ad_len = event->disc.length_data;
    uint8_t target_mac[6];
    uint8_t status_byte = 0xFF;
    uint8_t rolling_counter = 0;
    bool has_medflo_name = false;

    g_ble_disc_total++;
    memcpy(target_mac, event->disc.addr.val, 6);

    // Pass 1: 檢查是否為 MedFlo 設備 + 提取 GPIO 狀態
    int idx = 0;
    while (idx < ad_len) {
        uint8_t len = ad[idx];
        if (len == 0 || idx + len > ad_len) {
            break;
        }
        uint8_t type = ad[idx + 1];
        const uint8_t *val = &ad[idx + 2];
        uint8_t val_len = len - 1;

        // 在 Device Name (0x08/0x09) 或 Manufacturer Data (0xFF) 中搜尋 MEDFLO/NEXMEDAI
        if (type == 0x08 || type == 0x09 || type == 0xFF) {
            if (val_len >= 6) {
                for (int k = 0; k <= (int)val_len - 6; k++) {
                    if (val[k] == 'M' && val[k+1] == 'E' && val[k+2] == 'D' &&
                        val[k+3] == 'F' && val[k+4] == 'L' && val[k+5] == 'O') {
                        has_medflo_name = true;
                        // 從 Device Name 提取真實 MAC（格式: "MEDFLO-XXXXXXXXXXXX"）
                        if (type == 0x08 || type == 0x09) {
                            if (k + 19 < (int)val_len && val[k+6] == '-') {
                                // 跳過 "MEDFLO-"，後面 12 個 hex 字元轉成 6 bytes MAC
                                for (int m = 0; m < 6; m++) {
                                    char hex[3] = {0};
                                    hex[0] = (char)val[k+7+m*2];
                                    hex[1] = (char)val[k+8+m*2];
                                    target_mac[5-m] = (uint8_t)strtol(hex, NULL, 16);
                                }
                            }
                        }
                        goto found_medflo;
                    }
                }
            }
            // NEXMEDAI (8 bytes)
            if (val_len >= 8) {
                for (int k = 0; k <= (int)val_len - 8; k++) {
                    if (val[k] == 'N' && val[k+1] == 'E' && val[k+2] == 'X' &&
                        val[k+3] == 'M' && val[k+4] == 'E' && val[k+5] == 'D' &&
                        val[k+6] == 'A' && val[k+7] == 'I') {
                        has_medflo_name = true;
                        goto found_medflo;
                    }
                }
            }
        }
        found_medflo:
        idx += len + 1;
    }

    // Pass 2: 從 AD 0xFF 提取 GPIO 狀態 (適用於 MedFlo 格式: 0xFF 帶 1-2 bytes status)
    if (has_medflo_name) {
        idx = 0;
        while (idx < ad_len) {
            uint8_t len = ad[idx];
            if (len == 0 || idx + len > ad_len) break;
            uint8_t type = ad[idx + 1];
            const uint8_t *val = &ad[idx + 2];
            uint8_t val_len = len - 1;

            if (type == 0xFF && val_len >= 1) {
                // MedFlo 格式: AD 0xFF 的第一個 byte 是 GPIO 狀態
                if (val_len == 1 || val_len == 2) {
                    status_byte = val[0];
                    if (val_len >= 2)
                        rolling_counter = val[1];
                }
                // 標準格式: Company ID + product data (保持相容)
                else if (val_len >= 3) {
                    uint16_t company_id = (val[1] << 8) | val[0];
                    if (company_id == 0xFAFA || company_id == 0xFFFF || company_id == 0x0304 || company_id == 0x004C) {
                        uint8_t product_id = val[2];
                        if (product_id == 0x00 || product_id == 0x01 || product_id == 0x02 || product_id == 0x12 || product_id == 0x10) {
                            if (company_id == 0xFFFF || company_id == 0x0304) {
                                status_byte = product_id;
                            } else if (product_id == 0x10) {
                                status_byte = val[3];
                                rolling_counter = val[4];
                            } else if (val_len >= 11) {
                                status_byte = val[9];
                                rolling_counter = val[10];
                            } else if (val_len >= 6) {
                                status_byte = val[5];
                                rolling_counter = val[4];
                            } else if (val_len >= 5) {
                                status_byte = val[3];
                                rolling_counter = val[4];
                            }
                        }
                    }
                }
            }
            idx += len + 1;
        }
    }

    // 僅轉發 MedFlo 裝置
    if (has_medflo_name) {
        g_ble_disc_passed++;
        medflo_track_mac(target_mac);
        bt_log_add(target_mac, event->disc.rssi, status_byte, rolling_counter, ad, ad_len);
    }
}
