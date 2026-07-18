/*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*  http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bsp/bsp.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "bleprph.h"
#include "ctrlboard.h"
#include "json.h"
#include "json_object.h"
#include "json_types.h"

#if defined(CFG_NET_WIFI_SDIO_VND_RTK)
#include "lwip/ip.h"
#include "wifi_conf.h"
extern int gShowTextMode;
extern struct netif xnetif[NET_IF_NUM];
extern uint8_t* LwIP_GetMAC(struct netif *pnetif);
extern uint8_t* LwIP_GetIP(struct netif *pnetif);
#endif

#if defined(CFG_NET_WIFI_SDIO_NGPL_AP6256) || defined(CFG_NET_WIFI_SDIO_NGPL_AP6236) || defined(CFG_NET_WIFI_SDIO_NGPL_AP6212) || defined(CFG_NET_WIFI_SDIO_NGPL_AP6203)
#include "mhd_api.h"

#define FORMAT_IPADDR(x) ((unsigned char *)&x)[3], ((unsigned char *)&x)[2], ((unsigned char *)&x)[1], ((unsigned char *)&x)[0]

#endif

#ifdef CFG_MIRROR_TEST
extern bool gInDemoLayer;
#endif
extern uint16_t hrs_hrm_handle;

// 0xAAA6 是自訂的 Characteristic，專門用來 Notify WiFi Scan 結果
uint16_t hrs_hrm_handle_aaa6;
uint16_t conn_handle = 0;

static void
set_mbuf_string(char *dst, int len, const struct os_mbuf *om)
{
	int colon, i;
	memset(dst, 0, len);
	colon = 0;
	while (om != NULL) {
		if (colon) {
			MODLOG_DFLT(DEBUG, ":");
		}
		else {
			colon = 1;
		}
		for (i = 0; i < om->om_len; i++)
		{
			memcat(dst, om->om_data, i);
		}
		memcat(dst, 0, i);
		printf("get: %s\n", dst);
	}
}

/*
 * WiFi Scan → 組 JSON → 分段 Notify 給電腦端
 * 封包格式: [seq(1B)][total(1B)][JSON片段...]
 * 電腦端收齊後依 seq 重組成完整 JSON
 *
 * JSON格式: [{"s":"SSID名稱","r":-65,"m":4}, ...]
 *   s=ssid, r=rssi, m=security mode
 */
static void* wifi_scan_notify_task(void *arg)
{
    uint16_t conn_hdl = *(uint16_t*)arg;
    free(arg);

    // 1. 宣告一個可以容納 64 個 AP 的陣列 (WIFI_SCAN_LIST_NUMBER 定義在 wifiMgr_def.h 裡是 64)
    WIFI_MGR_SCANAP_LIST apList[WIFI_SCAN_LIST_NUMBER];
    memset(apList, 0, sizeof(apList));

    // ✅ 把原本致命的 sprintf 拿掉了，完全不印 Log 最安全
    
    // ITE SDK 是透過函數回傳值來告知你找到了幾個 AP
    int ap_count = WifiMgr_Get_Scan_AP_Info(apList);
    if (ap_count < 0) {
        // ✅ 這裡致命的 sprintf 也拿掉了
        
        // 回傳錯誤給手機/電腦端
        const char *errJson = "{\"err\":1}";
        struct os_mbuf *om = ble_hs_mbuf_from_flat(errJson, strlen(errJson));
        ble_gattc_notify_custom(conn_hdl, hrs_hrm_handle_aaa6, om);
        pthread_exit(NULL);
    }

    // 2. 組 JSON
    char jsonBuf[2048];
    int pos = 0;
    pos += snprintf(jsonBuf + pos, sizeof(jsonBuf) - pos, "[");
    
    // 使用剛剛拿到的 ap_count 跑迴圈
    for (int i = 0; i < ap_count && pos < (int)sizeof(jsonBuf) - 80; i++) {
        if (i > 0)
            pos += snprintf(jsonBuf + pos, sizeof(jsonBuf) - pos, ",");
            
        // 這裡的變數全部改成對齊 wifiMgr_def.h 裡面的欄位名稱！
        pos += snprintf(jsonBuf + pos, sizeof(jsonBuf) - pos,
                        "{\"s\":\"%s\",\"r\":%d,\"m\":%ld}",
                        apList[i].ssidName,       //
                        apList[i].rfQualityRSSI,  //
                        (unsigned long)apList[i].securityMode); //
    }
    pos += snprintf(jsonBuf + pos, sizeof(jsonBuf) - pos, "]");
    
    // ✅ 幫你把這行原本漏掉、可能造成死鎖的 printf 註解掉了！
    // printf("[SCAN] JSON (%d bytes): %s\n", pos, jsonBuf); //

    // 3. 分段 Notify（每包 = 2 bytes header + payload）
    const int PAYLOAD = 100;  // 保守值，MTU協商前用小包
    int total_pkts = (pos + PAYLOAD - 1) / PAYLOAD; //
    int sent = 0, seq = 0; //

    while (sent < pos) { //
        int chunk = pos - sent; //
        if (chunk > PAYLOAD) chunk = PAYLOAD; //

        uint8_t pktBuf[102]; //
        pktBuf[0] = (uint8_t)seq; //
        pktBuf[1] = (uint8_t)total_pkts; //
        memcpy(&pktBuf[2], jsonBuf + sent, chunk); //

        struct os_mbuf *om = ble_hs_mbuf_from_flat(pktBuf, chunk + 2); //
        ble_gattc_notify_custom(conn_hdl, hrs_hrm_handle_aaa6, om); //

        sent += chunk; //
        seq++; //
        usleep(50 * 1000);  // 50ms 間隔，避免 congestion
    }
    
    // ✅ 這裡致命的 sprintf 也拿掉了
    
    pthread_exit(NULL); //
}

static uint8_t gatt_svr_sec_test_static_val;

static int
gatt_svr_chr_access_sec_test(uint16_t conn_handle, uint16_t attr_handle,
struct ble_gatt_access_ctxt *ctxt,
	void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
	{
		.type = BLE_GATT_SVC_TYPE_PRIMARY,
		.uuid = BLE_UUID16_DECLARE(0x1801),
	},
	{
		.type = BLE_GATT_SVC_TYPE_PRIMARY,
		.uuid = BLE_UUID16_DECLARE(0x1800),
	},
	{
		/*** Service: Security test. */
		.type = BLE_GATT_SVC_TYPE_PRIMARY,
		.uuid = BLE_UUID16_DECLARE(0xAAA0),
		.characteristics = (struct ble_gatt_chr_def[]) {
			{
				/*** Characteristic: Random number generator. */
				.uuid = BLE_UUID16_DECLARE(0xAAA1),
				.access_cb = gatt_svr_chr_access_sec_test,
				.val_handle = &hrs_hrm_handle,
				.flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
			}, {
				/*** Characteristic: Static value. */
				.uuid = BLE_UUID16_DECLARE(0xAAA2),
				.access_cb = gatt_svr_chr_access_sec_test,
				.flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
			}, {
				/*** Characteristic: Static value. */
				.uuid = BLE_UUID16_DECLARE(0xAAA3),
				.access_cb = gatt_svr_chr_access_sec_test,
				.flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
			}, {
				/*** Characteristic: Static value. */
				.uuid = BLE_UUID16_DECLARE(0xAAA4),
				.access_cb = gatt_svr_chr_access_sec_test,
				.flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
			}, {/*** Characteristic: Static value. */
				.uuid = BLE_UUID16_DECLARE(0xAAA5),
				.access_cb = gatt_svr_chr_access_sec_test,
				.flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
			}, {/*** Characteristic: Static value. */
				.uuid = BLE_UUID16_DECLARE(0xAAA6),
				.access_cb = gatt_svr_chr_access_sec_test,
				.val_handle = &hrs_hrm_handle_aaa6,  // ✅ 加在這裡才對
				.flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
			}, {/*** Characteristic: Static value. */
				.uuid = BLE_UUID16_DECLARE(0xAAA7),
				.access_cb = gatt_svr_chr_access_sec_test,
				.flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
			}, {
				0, /* No more characteristics in this service. */
			}
		},
	},
	{
		0, /* No more services. */
	},
};

static int
gatt_svr_chr_write(struct os_mbuf *om, uint16_t min_len, uint16_t max_len,
void *dst, uint16_t *len)
{
	uint16_t om_len;
	int rc;

	om_len = OS_MBUF_PKTLEN(om);
	if (om_len < min_len || om_len > max_len) {
		return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
	}

	rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
	if (rc != 0) {
		return BLE_ATT_ERR_UNLIKELY;
	}

	return 0;
}

void
print_mbuf(const struct os_mbuf *om)
{
	int colon;

	colon = 0;
	while (om != NULL) {
		if (colon) {
			MODLOG_DFLT(DEBUG, ":");
		}
		else {
			colon = 1;
		}
		print_bytes(om->om_data, om->om_len);
		om = SLIST_NEXT(om, om_next);
	}
}

static int
gatt_svr_chr_access_sec_test(uint16_t conn_handle, uint16_t attr_handle,
struct ble_gatt_access_ctxt *ctxt,
	void *arg)
{
	const ble_uuid_t *uuid;
	int mode;
	int rc;
	char p_wrote_val[8] = { 0 };
	struct json_object *my_obj;
	uint8_t temp_buff[256] = { 0 };
	uint32_t len;
	char *str = "{\"fun\":\"hotspot\", \"type\":\"4\"}";   //WIFI link status

	uuid = ble_uuid_u16(ctxt->chr->uuid);
	if (uuid == 0xAAA1){
		char ip[16] = "0.0.0.0";
		/*get WiFi IP addr start*/
#if defined(CFG_NET_WIFI_SDIO_VND_RTK)
#ifdef CFG_MIRROR_TEST
        if (gInDemoLayer)
        {
            unsigned char *ip_r = LwIP_GetIP(&xnetif[0]);
            unsigned char *mac = LwIP_GetMAC(&xnetif[0]);
            ipaddr_ntoa_r((const ip_addr_t*)ip_r, ip, sizeof(ip));
        }
#endif
#endif
#if defined(CFG_NET_WIFI_SDIO_NGPL_AP6256) || defined(CFG_NET_WIFI_SDIO_NGPL_AP6236) || defined(CFG_NET_WIFI_SDIO_NGPL_AP6212) || defined(CFG_NET_WIFI_SDIO_NGPL_AP6203)
#ifdef CFG_MIRROR_TEST
        if (gInDemoLayer)
        {
            uint32_t nIp;

            nIp = htonl(mhd_sta_ipv4_ipaddr());

            sprintf(ip, "%u.%u.%u.%u", FORMAT_IPADDR(nIp));
        }
#endif
#endif
		/*get WiFi IP addr end*/
		switch (ctxt->op) {
		case BLE_GATT_ACCESS_OP_READ_CHR:
			printf("ip: %s\n", ip);
			//memcpy(&gatt_svr_sec_test_static_val, ip, 16);
			rc = os_mbuf_append(ctxt->om, &ip, 16);
			return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
		default:
			assert(0);
			return BLE_ATT_ERR_UNLIKELY;
		}
	}
	if (uuid == 0xAAA2){
		switch (ctxt->op) {
		case BLE_GATT_ACCESS_OP_WRITE_CHR:
			memset(theConfig.ssid, 0, sizeof(theConfig.ssid));
			rc = gatt_svr_chr_write(ctxt->om,
				0,
				sizeof(theConfig.ssid),
				&theConfig.ssid, NULL);
			int i = 0;
			while (theConfig.ssid[i++] > 0);
			theConfig.ssid[i] = '\0';
			printf("theConfig.ssid: %s\n", theConfig.ssid);
			return rc;
		case BLE_GATT_ACCESS_OP_READ_CHR:
			printf("AP SSID: %s\n", theConfig.ap_ssid);
			rc = os_mbuf_append(ctxt->om, &theConfig.ap_ssid, strlen(theConfig.ap_ssid) + 1);
			return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
		default:
			assert(0);
			return BLE_ATT_ERR_UNLIKELY;
		}
	}
	if (uuid == 0xAAA3){
		switch (ctxt->op) {
		case BLE_GATT_ACCESS_OP_WRITE_CHR:
			memset(theConfig.password, 0, sizeof(theConfig.password));
			rc = gatt_svr_chr_write(ctxt->om,
				0,
				sizeof(theConfig.password),
				&theConfig.password, NULL);
			int i = 0;
			while (theConfig.password[i++] > 0);
			theConfig.password[i] = '\0';
			printf("theConfig.password: %s\n", theConfig.password);
			return rc;
		case BLE_GATT_ACCESS_OP_READ_CHR:
			printf("AP PASSWORD: %s\n", theConfig.ap_password);
			rc = os_mbuf_append(ctxt->om, &theConfig.ap_password, strlen(theConfig.ap_password) + 1);
			return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
		default:
			assert(0);
			return BLE_ATT_ERR_UNLIKELY;
		}
	}
	if (uuid == 0xAAA4){
		switch (ctxt->op) {
		case BLE_GATT_ACCESS_OP_WRITE_CHR:
			theConfig.wifi_on_off = 1;
			WifiMgr_Sta_Disconnect();
			usleep(1000 * 1000);
			printf("!!!!!!!!!!!!!!!!!Start linking AP!!!!!!!!!!!!!!\n");
			/*start link WiFi start*/
			WifiMgr_Sta_Connect(theConfig.ssid, theConfig.password, theConfig.secumode);
			/*start link WiFi end*/
			rc = gatt_svr_chr_write(ctxt->om,
				sizeof gatt_svr_sec_test_static_val,
				sizeof gatt_svr_sec_test_static_val,
				&gatt_svr_sec_test_static_val, NULL);
			return rc;
		default:
			assert(0);
			return BLE_ATT_ERR_UNLIKELY;
		}
	}
	if (uuid == 0xAAA5){
		switch (ctxt->op) {
		case BLE_GATT_ACCESS_OP_WRITE_CHR:
			memset(p_wrote_val, 0, 8);
			rc = gatt_svr_chr_write(ctxt->om,
				0,
				sizeof(p_wrote_val),
				&p_wrote_val, NULL);
			printf("Peer wrote content:   |%d|\n", p_wrote_val[0]);
			printf(">>>>>>>>>>>>>>>>wifi_mode(%d)<<<<<<<<<<<<<<<\n", theConfig.wifi_mode);
			if (theConfig.wifi_mode != p_wrote_val[0])
			{
				theConfig.wifi_mode = p_wrote_val[0];
				NetworkWifiModeSwitch();
			}
			return rc;
		default:
			assert(0);
			return BLE_ATT_ERR_UNLIKELY;
		}
	}
	if (uuid == 0xAAA6) {
		switch (ctxt->op) {
		case BLE_GATT_ACCESS_OP_WRITE_CHR:
		{
			// 收到電腦端寫入的命令
			char cmd[16] = {0};
			rc = gatt_svr_chr_write(ctxt->om, 0, sizeof(cmd) - 1, cmd, NULL);
			if (rc != 0) return rc;

			printf("[BLE] 0xAAA6 received cmd: %s\n", cmd);

			if (strncmp(cmd, "SCAN", 4) == 0)
			{
				printf("[BLE] Start WiFi Scan...\n");
				pthread_t scan_tid;
				pthread_attr_t scan_attr;
				pthread_attr_init(&scan_attr);
				pthread_attr_setdetachstate(&scan_attr, PTHREAD_CREATE_DETACHED);

				extern uint16_t conn_handle; 
				uint16_t *pHandle = malloc(sizeof(uint16_t));
				*pHandle = conn_handle;
				pthread_create(&scan_tid, &scan_attr, wifi_scan_notify_task, pHandle);
			}
			return 0;
		}

		case BLE_GATT_ACCESS_OP_READ_CHR:
			strcpy(temp_buff, "EY");
			my_obj = json_tokener_parse(str);
			len = strlen(json_object_to_json_string(my_obj));
			temp_buff[2] = (len & 0xFF000000) >> 24;
			temp_buff[3] = (len & 0xFF0000) >> 16;
			temp_buff[4] = (len & 0xFF00) >> 8;
			temp_buff[5] = len & 0xFF;
			memcpy(&temp_buff[6], json_object_to_json_string(my_obj), len);
			printf("+++++++++++++++++++tokener parse: (%s, %s)%d\n", (uint16_t *)temp_buff, json_object_to_json_string(my_obj), len);
			rc = os_mbuf_append(ctxt->om, &temp_buff, len + 6);
			return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

		default:
			assert(0);
			return BLE_ATT_ERR_UNLIKELY;
		}
	}
	if (uuid == 0xAAA7){
		switch (ctxt->op) {
		case BLE_GATT_ACCESS_OP_WRITE_CHR:
			memset(theConfig.password, 0, sizeof(theConfig.password));
			rc = gatt_svr_chr_write(ctxt->om,
				0,
				sizeof(theConfig.password),
				&theConfig.password, NULL);
			int i = 0;
			while (theConfig.password[i++] > 0);
			theConfig.password[i] = '\0';
			printf("theConfig.password: %s\n", theConfig.password);
			return rc;
		default:
			assert(0);
			return BLE_ATT_ERR_UNLIKELY;
		}
	}

	/* Unknown characteristic; the nimble stack should not have called this
	* function.
	*/
	assert(0);
	return BLE_ATT_ERR_UNLIKELY;
}

void
gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
	char buf[BLE_UUID_STR_LEN];

	switch (ctxt->op) {
	case BLE_GATT_REGISTER_OP_SVC:
		MODLOG_DFLT(DEBUG, "registered service %s with handle=%d\n",
			ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
			ctxt->svc.handle);
		break;

	case BLE_GATT_REGISTER_OP_CHR:
		MODLOG_DFLT(DEBUG, "registering characteristic %s with "
			"def_handle=%d val_handle=%d\n",
			ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
			ctxt->chr.def_handle,
			ctxt->chr.val_handle);
		break;

	case BLE_GATT_REGISTER_OP_DSC:
		MODLOG_DFLT(DEBUG, "registering descriptor %s with handle=%d\n",
			ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
			ctxt->dsc.handle);
		break;

	default:
		assert(0);
		break;
	}
}

int
gatt_svr_init(void)
{
	int rc;

	rc = ble_gatts_count_cfg(gatt_svr_svcs);
	if (rc != 0) {
		return rc;
	}

	rc = ble_gatts_add_svcs(gatt_svr_svcs);
	if (rc != 0) {
		return rc;
	}

	return 0;
}
