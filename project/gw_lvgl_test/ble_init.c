/*
 * ble_init.c — NimBLE init + MedFlo device scan
 * Pattern: SDK test_nimble/test_scan.c
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include "nimble/nimble_port.h"
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "hci_if.h"
#include "ite/itp.h"

#include "ble_init.h"

/* ── Scan state ── */
#define MAX_MEDFLO_DEV 32
static medflo_dev_t s_devs[MAX_MEDFLO_DEV];
static int s_dev_cnt = 0;
static int s_adv_total = 0;  /* debug: total advertisements received */

/* ── Forward decl ── */
static void ble_scan_start(void);

/* ── Helpers ── */
static const char *addr_str(const uint8_t *addr)
{
    static char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
        addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
    return buf;
}

/* ── Case-insensitive prefix match ── */
static bool match_prefix(const uint8_t *data, int len, const char *prefix, int plen)
{
    if (len < plen) return false;
    for (int i = 0; i <= len - plen; i++) {
        int j;
        for (j = 0; j < plen; j++) {
            char c = data[i+j];
            char p = prefix[j];
            if (c >= 'a' && c <= 'z') c -= 32;  /* toupper */
            if (c != p) break;
        }
        if (j == plen) return true;
    }
    return false;
}

static bool is_medflo_name(const uint8_t *data, int len)
{
    if (!data || len < 6) return false;
    return match_prefix(data, len, "MEDFLO", 6)
        || match_prefix(data, len, "NEXMED", 6);  /* NEXMEDAI starts with NEXMED */
}

/* ── MAC extraction from "MEDFLO-XXXXXXXXXXXX" ── */
static void extract_mac(const char *name, uint8_t *mac_out)
{
    /* Find last '-' or 12-char hex at end */
    const char *p = strrchr(name, '-');
    if (!p) p = name;
    else p++; /* skip '-' */
    int len = strlen(p);
    if (len >= 12) {
        for (int i = 0; i < 6; i++) {
            char hex[3] = {p[i*2], p[i*2+1], 0};
            mac_out[5-i] = (uint8_t)strtoul(hex, NULL, 16);
        }
    }
}

/* ── Parse manufacturer data for status + counter ── */
static void parse_mfg_data(const uint8_t *data, int len, uint8_t *status, uint8_t *counter)
{
    *status = 0; *counter = 0;
    if (len >= 2) {
        *status  = data[0];
        *counter = data[1];
    }
}

/* ── Add or update device in scan list ── */
static void add_device(const uint8_t *addr, int8_t rssi,
                       const char *name, uint8_t status, uint8_t counter)
{
    const char *mac_str = addr_str(addr);
    /* Check if already in list */
    for (int i = 0; i < s_dev_cnt; i++) {
        if (memcmp(s_devs[i].mac, mac_str, 17) == 0) {
            s_devs[i].rssi = rssi;
            s_devs[i].status = status;
            s_devs[i].counter = counter;
            return;
        }
    }
    /* Add new */
    if (s_dev_cnt < MAX_MEDFLO_DEV) {
        strncpy(s_devs[s_dev_cnt].mac, mac_str, 18);
        s_devs[s_dev_cnt].rssi = rssi;
        s_devs[s_dev_cnt].status = status;
        s_devs[s_dev_cnt].counter = counter;
        if (name) strncpy(s_devs[s_dev_cnt].name, name, 32);
        s_dev_cnt++;
        printf("[BLE] MedFlo device #%d: %s rssi=%d status=%d cnt=%d name=%s\n",
               s_dev_cnt, mac_str, rssi, status, counter, name ? name : "?");
    }
}

/* ── Advertisement parser (matching old SDK 2470 bt_handle_disc) ── */
static void parse_adv(const struct ble_hs_adv_fields *fields, int8_t rssi,
                      const uint8_t *addr)
{
    uint8_t mac[6] = {0};
    const char *dev_name = NULL;
    char name_buf[33] = {0};
    bool is_medflo = false;
    uint8_t status = 0, counter = 0;

    /* ── Search NAME field (AD Type 0x08/0x09) for MEDFLO/NEXMED ── */
    if (fields->name && fields->name_len > 0) {
        int nlen = fields->name_len < 32 ? fields->name_len : 32;
        memcpy(name_buf, fields->name, nlen);
        name_buf[nlen] = 0;
        dev_name = name_buf;
        if (is_medflo_name(fields->name, fields->name_len)) {
            is_medflo = true;
            extract_mac(name_buf, mac);
        }
    }

    /* ── Search Manufacturer Data (AD Type 0xFF) for MEDFLO/NEXMED ──
     *    Old SDK also checks type 0xFF — some devices put name here */
    if (!is_medflo && fields->mfg_data && fields->mfg_data_len >= 6) {
        if (is_medflo_name(fields->mfg_data, fields->mfg_data_len)) {
            is_medflo = true;
            memcpy(mac, addr, 6);  /* use BLE packet MAC */
        }
        parse_mfg_data(fields->mfg_data, fields->mfg_data_len, &status, &counter);
    } else if (is_medflo && fields->mfg_data && fields->mfg_data_len > 0) {
        parse_mfg_data(fields->mfg_data, fields->mfg_data_len, &status, &counter);
    }

    if (is_medflo) {
        add_device(mac, rssi, dev_name, status, counter);
    }
}

/* ── GAP event callback ── */
static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    switch (event->type) {
    case BLE_GAP_EVENT_DISC: {
        s_adv_total++;
        struct ble_hs_adv_fields fields;
        int rc = ble_hs_adv_parse_fields(&fields, event->disc.data,
                                         event->disc.length_data);
        if (rc == 0) {
            parse_adv(&fields, event->disc.rssi, event->disc.addr.val);
        }
        if (s_adv_total % 50 == 0) {
            printf("[BLE] adv total=%d medflo=%d\n", s_adv_total, s_dev_cnt);
        }
        return 0;
    }
    case BLE_GAP_EVENT_DISC_COMPLETE:
        printf("[BLE] scan complete: adv=%d medflo=%d\n", s_adv_total, s_dev_cnt);
        s_adv_total = 0;
        ble_scan_start();  /* restart scan */
        return 0;
    default:
        return 0;
    }
}

/* ── Start scanning ── */
static void ble_scan_start(void)
{
    struct ble_gap_disc_params params = {
        .itvl = 160,        /* 100ms */
        .window = 160,      /* 100ms — full duty cycle test */
        .filter_policy = 0,
        .limited = 0,
        .passive = 0,       /* active scan */
        .filter_duplicates = 0,
    };
    ble_gap_disc(BLE_OWN_ADDR_RANDOM, 60000, &params, ble_gap_event, NULL);
}

/* ── NimBLE address ── */
static void ble_set_addr(void)
{
    ble_addr_t addr;
    int rc = ble_hs_id_gen_rnd(1, &addr);
    assert(rc == 0);
    rc = ble_hs_id_set_rnd(addr.val);
    assert(rc == 0);
}

/* ── Sync callback: host↔controller ready ── */
static void on_sync(void)
{
    printf("[BLE] sync done — starting scan\n");
    ble_set_addr();
    ble_scan_start();
}

static void on_reset(int reason)
{
    printf("[BLE] reset (reason=%d)\n", reason);
}

/* ── BLE task ── */
static void *ble_task(void *arg)
{
    (void)arg;
    printf("[BLE] starting NimBLE scan task...\n");

    nimble_port_init();

    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.reset_cb = on_reset;

    nimble_port_run();  /* blocks forever */
    return NULL;
}

/* ── Public API ── */
void ble_init(void)
{
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 8192);
    pthread_create(&tid, &attr, ble_task, NULL);
    pthread_attr_destroy(&attr);
    printf("[BLE] scan task started\n");
}

int ble_get_device_count(void)
{
    return s_dev_cnt;
}

const medflo_dev_t *ble_get_devices(void)
{
    return s_devs;
}
