/*
 * bt_gatt.c
 *
 * GATT Server — Service 0xAAA0 的所有 Characteristic 定義與 handler。
 *
 * Service 0xAAA0 Characteristic 對照表：
 * ┌────────┬──────────┬────────────────────────────────────────┐
 * │ UUID   │ 屬性     │ 功能                                   │
 * ├────────┼──────────┼────────────────────────────────────────┤
 * │ 0xAAA1 │ R/Notify │ 讀取目前 WiFi IP；IP 變動時 Notify    │
 * │ 0xAAA2 │ R/W      │ 寫入要連線的 SSID；讀取 AP SSID       │
 * │ 0xAAA3 │ R/W      │ 寫入 WiFi 密碼；讀取 AP 密碼          │
 * │ 0xAAA4 │ W        │ 觸發 WifiMgr_Sta_Connect()            │
 * │ 0xAAA5 │ W        │ 切換 WiFi 模式（STA / SoftAP）        │
 * │ 0xAAA6 │ R/W/N    │ Write "SCAN" 觸發掃描；Notify 回結果  │
 * │ 0xAAA7 │ W        │ 保留                                   │
 * └────────┴──────────┴────────────────────────────────────────┘
 *
 * 依賴：
 *   - bt_gatt.h, bt_wifi.h
 *   - ctrlboard.h  (theConfig, NetworkWifiModeSwitch)
 *   - wifiMgr.h    (WifiMgr_Sta_Connect, WifiMgr_Sta_Disconnect)
 *   - host/ble_hs.h, host/ble_uuid.h
 *   - json.h, json_object.h
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "bsp/bsp.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "modlog/modlog.h"
#include "json.h"
#include "json_object.h"

#include "ctrlboard.h"
#include "bt_gatt.h"
#include "bt_wifi.h"

/* ================================================================ */
/*  公開變數                                                         */
/* ================================================================ */

uint16_t g_ble_ip_handle   = 0;   /* 0xAAA1 notify handle，bt_main.c 的 IP notify thread 使用 */

/* ================================================================ */
/*  內部靜態變數                                                     */
/* ================================================================ */

static uint8_t s_static_val = 0;  /* 0xAAA4 write 需要一個暫存目標 */

/* ================================================================ */
/*  工具函式                                                         */
/* ================================================================ */

/*
 * 從 os_mbuf 讀出資料到 dst。
 * 回傳 0 成功，BLE_ATT_ERR_* 失敗。
 */
static int chr_write(struct os_mbuf *om,
                     uint16_t min_len, uint16_t max_len,
                     void *dst, uint16_t *out_len)
{
    uint16_t om_len = OS_MBUF_PKTLEN(om);
    if (om_len < min_len || om_len > max_len)
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;

    int rc = ble_hs_mbuf_to_flat(om, dst, max_len, out_len);
    return (rc != 0) ? BLE_ATT_ERR_UNLIKELY : 0;
}

/* ================================================================ */
/*  GATT Service 定義                                               */
/* ================================================================ */

/* 前向宣告 */
static int chr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                         struct ble_gatt_access_ctxt *ctxt, void *arg);

static const struct ble_gatt_svc_def s_gatt_svcs[] = {
    /* Generic Attribute Profile Service */
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(0x1801),
    },
    /* Generic Access Profile Service */
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(0x1800),
    },
    /* 主服務：WiFi 控制 */
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(0xAAA0),
        .characteristics = (struct ble_gatt_chr_def[]) {
            {   /* 0xAAA1 — IP Notify */
                .uuid       = BLE_UUID16_DECLARE(0xAAA1),
                .access_cb  = chr_access_cb,
                .val_handle = &g_ble_ip_handle,
                .flags      = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            },
            {   /* 0xAAA2 — SSID */
                .uuid       = BLE_UUID16_DECLARE(0xAAA2),
                .access_cb  = chr_access_cb,
                .flags      = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ,
            },
            {   /* 0xAAA3 — Password */
                .uuid       = BLE_UUID16_DECLARE(0xAAA3),
                .access_cb  = chr_access_cb,
                .flags      = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ,
            },
            {   /* 0xAAA4 — 觸發連線 */
                .uuid       = BLE_UUID16_DECLARE(0xAAA4),
                .access_cb  = chr_access_cb,
                .flags      = BLE_GATT_CHR_F_WRITE,
            },
            {   /* 0xAAA5 — 模式切換 */
                .uuid       = BLE_UUID16_DECLARE(0xAAA5),
                .access_cb  = chr_access_cb,
                .flags      = BLE_GATT_CHR_F_WRITE,
            },
            {   /* 0xAAA6 — WiFi Scan */
                .uuid       = BLE_UUID16_DECLARE(0xAAA6),
                .access_cb  = chr_access_cb,
                .val_handle = &g_ble_scan_handle,
                .flags      = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ
                            | BLE_GATT_CHR_F_NOTIFY,
            },
            {   /* 0xAAA7 — 保留 */
                .uuid       = BLE_UUID16_DECLARE(0xAAA7),
                .access_cb  = chr_access_cb,
                .flags      = BLE_GATT_CHR_F_WRITE,
            },
            { 0 } /* 結束標記 */
        },
    },
    { 0 } /* 結束標記 */
};

/* ================================================================ */
/*  Characteristic Access Callback                                   */
/* ================================================================ */

static int chr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                         struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    int rc;
    uint16_t uuid = ble_uuid_u16(ctxt->chr->uuid);

    /* ---- 0xAAA1：讀取目前 WiFi IP ---- */
    if (uuid == 0xAAA1) {
        if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) {
            assert(0); return BLE_ATT_ERR_UNLIKELY;
        }
        char ip[16];
        bt_wifi_get_ip(ip, sizeof(ip));
        printf("[bt_gatt] Read IP: %s\n", ip);
        rc = os_mbuf_append(ctxt->om, ip, sizeof(ip));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    /* ---- 0xAAA2：SSID 讀/寫 ---- */
    if (uuid == 0xAAA2) {
        switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            memset(theConfig.ssid, 0, sizeof(theConfig.ssid));
            rc = chr_write(ctxt->om, 0, sizeof(theConfig.ssid),
                           theConfig.ssid, NULL);
            printf("[bt_gatt] Write SSID: %s\n", theConfig.ssid);
            return rc;
        case BLE_GATT_ACCESS_OP_READ_CHR:
            printf("[bt_gatt] Read AP SSID: %s\n", theConfig.ap_ssid);
            rc = os_mbuf_append(ctxt->om, theConfig.ap_ssid,
                                strlen(theConfig.ap_ssid) + 1);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        default:
            assert(0); return BLE_ATT_ERR_UNLIKELY;
        }
    }

    /* ---- 0xAAA3：Password 讀/寫 ---- */
    if (uuid == 0xAAA3) {
        switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            memset(theConfig.password, 0, sizeof(theConfig.password));
            rc = chr_write(ctxt->om, 0, sizeof(theConfig.password),
                           theConfig.password, NULL);
            printf("[bt_gatt] Write Password: (hidden)\n");
            return rc;
        case BLE_GATT_ACCESS_OP_READ_CHR:
            rc = os_mbuf_append(ctxt->om, theConfig.ap_password,
                                strlen(theConfig.ap_password) + 1);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        default:
            assert(0); return BLE_ATT_ERR_UNLIKELY;
        }
    }

    /* ---- 0xAAA4：觸發 WiFi 連線 ---- */
    if (uuid == 0xAAA4) {
        if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
            assert(0); return BLE_ATT_ERR_UNLIKELY;
        }
        printf("[bt_gatt] Connect WiFi: SSID=%s\n", theConfig.ssid);
        theConfig.wifi_on_off = 1;
        WifiMgr_Sta_Disconnect();
        usleep(1000 * 1000);
        WifiMgr_Sta_Connect(theConfig.ssid, theConfig.password,
                            theConfig.secumode);
        rc = chr_write(ctxt->om, sizeof(s_static_val), sizeof(s_static_val),
                       &s_static_val, NULL);
        return rc;
    }

    /* ---- 0xAAA5：WiFi 模式切換（STA / SoftAP）---- */
    if (uuid == 0xAAA5) {
        if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
            assert(0); return BLE_ATT_ERR_UNLIKELY;
        }
        char new_mode = 0;
        rc = chr_write(ctxt->om, 0, sizeof(new_mode), &new_mode, NULL);
        printf("[bt_gatt] WiFi mode: %d -> %d\n",
               theConfig.wifi_mode, (int)new_mode);
        if (theConfig.wifi_mode != (int)new_mode) {
            theConfig.wifi_mode = (int)new_mode;
            NetworkWifiModeSwitch();
        }
        return rc;
    }

    /* ---- 0xAAA6：WiFi Scan ---- */
    if (uuid == 0xAAA6) {
        switch (ctxt->op) {

        case BLE_GATT_ACCESS_OP_WRITE_CHR: {
            /* 安全讀取命令字串 */
            char     cmd[128] = {0};
            uint16_t cmd_len  = 0;

            if (ctxt->om == NULL) {
                printf("[bt_gatt] 0xAAA6 NULL om\n");
                return BLE_ATT_ERR_UNLIKELY;
            }

            int rr = ble_hs_mbuf_to_flat(ctxt->om, cmd, sizeof(cmd) - 1, &cmd_len);
            if (rr != 0) {
                printf("[bt_gatt] 0xAAA6 mbuf read failed: %d\n", rr);
                return BLE_ATT_ERR_UNLIKELY;
            }
            cmd[cmd_len] = '\0';
            printf("[bt_gatt] 0xAAA6 cmd: [%s] len=%d\n", cmd, cmd_len);

            /* === 穩定前鎖定 WiFi 設定 === */
            if (strncmp(cmd, "WIFI_", 5) == 0) {
                extern bool s_stable_baseline;
                if (!s_stable_baseline) {
                    const char *err = "{\"err\":\"system not ready\"}";
                    struct os_mbuf *om = ble_hs_mbuf_from_flat(err, strlen(err));
                    if (om) ble_gattc_notify_custom(conn_handle, g_ble_scan_handle, om);
                    return 0;
                }
            }

            if (strncmp(cmd, "WIFI_SET:", 9) == 0) {
                // WIFI_SET:slot,ssid,password
                char *p = cmd + 9;
                int slot = atoi(p);
                char *ssid = strchr(p, ','); if (!ssid) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
                ssid++; char *pass = strchr(ssid, ','); if (!pass) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
                *pass = '\0'; pass++;
                printf("[bt_gatt] WIFI_SET slot=%d ssid=%s\n", slot, ssid);
                if (slot == 1) {
                    strncpy(theConfig.ssid, ssid, sizeof(theConfig.ssid)-1);
                    strncpy(theConfig.password, pass, sizeof(theConfig.password)-1);
                } else if (slot == 2) {
                    strncpy(theConfig.ssid2, ssid, sizeof(theConfig.ssid2)-1);
                    strncpy(theConfig.password2, pass, sizeof(theConfig.password2)-1);
                } else return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
                ConfigSave();
                const char *ok = "{\"wifi_set\":\"ok\"}";
                struct os_mbuf *om = ble_hs_mbuf_from_flat(ok, strlen(ok));
                if (om) ble_gattc_notify_custom(conn_handle, g_ble_scan_handle, om);
                return 0;
            }

            if (strncmp(cmd, "WIFI_DEL:", 9) == 0) {
                int slot = atoi(cmd + 9);
                printf("[bt_gatt] WIFI_DEL slot=%d\n", slot);
                if (slot == 1) {
                    theConfig.ssid[0] = '\0'; theConfig.password[0] = '\0';
                } else if (slot == 2) {
                    theConfig.ssid2[0] = '\0'; theConfig.password2[0] = '\0';
                } else return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
                ConfigSave();
                const char *ok = "{\"wifi_del\":\"ok\"}";
                struct os_mbuf *om = ble_hs_mbuf_from_flat(ok, strlen(ok));
                if (om) ble_gattc_notify_custom(conn_handle, g_ble_scan_handle, om);
                return 0;
            }

            if (strncmp(cmd, "WIFI_LIST", 9) == 0) {
                char buf[256];
                snprintf(buf, sizeof(buf),
                    "{\"ssid1\":\"%s\",\"ssid2\":\"%s\"}", theConfig.ssid, theConfig.ssid2);
                struct os_mbuf *om = ble_hs_mbuf_from_flat(buf, strlen(buf));
                if (om) ble_gattc_notify_custom(conn_handle, g_ble_scan_handle, om);
                return 0;
            }

            if (strncmp(cmd, "SCAN", 4) == 0) {
                uint16_t *pHandle = malloc(sizeof(uint16_t));
                if (pHandle == NULL) {
                    printf("[bt_gatt] malloc failed\n");
                    return BLE_ATT_ERR_INSUFFICIENT_RES;
                }
                *pHandle = conn_handle;

                pthread_t      tid;
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                pthread_attr_setstacksize(&attr, 32 * 1024); /* scan 用較大 stack */
                int tret = pthread_create(&tid, &attr, bt_wifi_scan_task, pHandle);
                if (tret != 0) {
                    printf("[bt_gatt] scan thread create failed: %d\n", tret);
                    free(pHandle);
                    return BLE_ATT_ERR_UNLIKELY;
                }
                printf("[bt_gatt] Scan thread started\n");
            }
            return 0;
        }

        case BLE_GATT_ACCESS_OP_READ_CHR: {
            /* 回傳 hotspot 狀態 JSON（維持原有功能） */
            const char *str = "{\"fun\":\"hotspot\",\"type\":\"4\"}";
            struct json_object *obj = json_tokener_parse(str);
            const char *json_str   = json_object_to_json_string(obj);
            uint32_t    len        = strlen(json_str);

            uint8_t buf[256] = {0};
            buf[0] = 'E'; buf[1] = 'Y';
            buf[2] = (len >> 24) & 0xFF;
            buf[3] = (len >> 16) & 0xFF;
            buf[4] = (len >>  8) & 0xFF;
            buf[5] =  len        & 0xFF;
            memcpy(&buf[6], json_str, len);

            rc = os_mbuf_append(ctxt->om, buf, len + 6);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }

        default:
            assert(0); return BLE_ATT_ERR_UNLIKELY;
        }
    }

    /* ---- 0xAAA7：保留 ---- */
    if (uuid == 0xAAA7) {
        if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
            assert(0); return BLE_ATT_ERR_UNLIKELY;
        }
        rc = chr_write(ctxt->om, 0, sizeof(theConfig.password),
                       theConfig.password, NULL);
        return rc;
    }

    /* 不應該到這裡 */
    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

/* ================================================================ */
/*  公開函式實作                                                     */
/* ================================================================ */

void bt_gatt_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];
    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        MODLOG_DFLT(DEBUG, "[bt_gatt] Registered service  %s handle=%d\n",
                    ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                    ctxt->svc.handle);
        break;
    case BLE_GATT_REGISTER_OP_CHR:
        MODLOG_DFLT(DEBUG, "[bt_gatt] Registered char     %s def=%d val=%d\n",
                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                    ctxt->chr.def_handle, ctxt->chr.val_handle);
        break;
    case BLE_GATT_REGISTER_OP_DSC:
        MODLOG_DFLT(DEBUG, "[bt_gatt] Registered desc     %s handle=%d\n",
                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                    ctxt->dsc.handle);
        break;
    default:
        assert(0);
        break;
    }
}

int bt_gatt_init(void)
{
    int rc;

    rc = ble_gatts_count_cfg(s_gatt_svcs);
    if (rc != 0) {
        printf("[bt_gatt] ble_gatts_count_cfg failed: %d\n", rc);
        return rc;
    }

    rc = ble_gatts_add_svcs(s_gatt_svcs);
    if (rc != 0) {
        printf("[bt_gatt] ble_gatts_add_svcs failed: %d\n", rc);
        return rc;
    }

    printf("[bt_gatt] GATT server init OK\n");
    return 0;
}
