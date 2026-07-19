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
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "os/mynewt.h"
#include "bsp/bsp.h"
#include "hal/hal_gpio.h"
#include "console/console.h"
#include "hal/hal_system.h"
#include "syscfg/syscfg.h"
#include "split/split.h"
#if MYNEWT_VAL(BLE_SVC_DIS_FIRMWARE_REVISION_READ_PERM) >= 0
#include "bootutil/image.h"
#include "imgmgr/imgmgr.h"
#include "services/dis/ble_svc_dis.h"
#endif

/* BLE */
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include <pthread.h>

/* Application-specified header. */
#include "bleprph.h"

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

#include "ctrlboard.h"

#ifdef CFG_MIRROR_TEST
extern bool gInDemoLayer;
#endif

static int bleprph_gap_event(struct ble_gap_event *event, void *arg);

/**
* Logs information about a connection to the console.
*/
static void
bleprph_print_conn_desc(struct ble_gap_conn_desc *desc)
{
	MODLOG_DFLT(INFO, "handle=%d our_ota_addr_type=%d our_ota_addr=",
		desc->conn_handle, desc->our_ota_addr.type);
	print_addr(desc->our_ota_addr.val);
	MODLOG_DFLT(INFO, " our_id_addr_type=%d our_id_addr=",
		desc->our_id_addr.type);
	print_addr(desc->our_id_addr.val);
	MODLOG_DFLT(INFO, " peer_ota_addr_type=%d peer_ota_addr=",
		desc->peer_ota_addr.type);
	print_addr(desc->peer_ota_addr.val);
	MODLOG_DFLT(INFO, " peer_id_addr_type=%d peer_id_addr=",
		desc->peer_id_addr.type);
	print_addr(desc->peer_id_addr.val);
	MODLOG_DFLT(INFO, " conn_itvl=%d conn_latency=%d supervision_timeout=%d "
		"encrypted=%d authenticated=%d bonded=%d\n",
		desc->conn_itvl, desc->conn_latency,
		desc->supervision_timeout,
		desc->sec_state.encrypted,
		desc->sec_state.authenticated,
		desc->sec_state.bonded);
}

/**
* Enables advertising with the following parameters:
*     o General discoverable mode.
*     o Undirected connectable mode.
*/
static void
bleprph_advertise(void)
{
	uint8_t own_addr_type;
	struct ble_gap_adv_params adv_params;
	struct ble_hs_adv_fields fields;
	int rc;
	uint8_t i = 0;
	uint8_t mac[6] = "";
	char name[32] = "ITEY_";
	char tmp[2] = "";

	/* Figure out address to use while advertising (no privacy for now) */
	rc = ble_hs_id_infer_auto(0, &own_addr_type);
	if (rc != 0) {
		MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
		return;
	}

	/**
	*  Set the advertisement data included in our advertisements:
	*     o Flags (indicates advertisement type and other general info).
	*     o Advertising tx power.
	*     o Device name.
	*     o 16-bit service UUIDs (alert notifications).
	*/

	memset(&fields, 0, sizeof fields);

	/* Advertise two flags:
	*     o Discoverability in forthcoming advertisement (general)
	*     o BLE-only (BR/EDR unsupported).
	*/
	fields.flags = BLE_HS_ADV_F_DISC_GEN |
		BLE_HS_ADV_F_BREDR_UNSUP;

	/* Indicate that the TX power level field should be included; have the
	* stack fill this value automatically.  This is done by assiging the
	* special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
	*/
	fields.tx_pwr_lvl_is_present = 1;
	fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

	//set display neme
	ble_hs_id_read_bd_addr(mac);
	for (i = 0; i < 6; i++)
	{
		sprintf(tmp, "%02X", mac[i]);
		strcat(name, tmp);
	}
	fields.name = (uint8_t *)name;
	fields.name_len = strlen(name);
	fields.name_is_complete = 1;

	fields.uuids16 = (ble_uuid16_t[]){
		BLE_UUID16_INIT(0xAAAA)
	};
	fields.num_uuids16 = 1;
	fields.uuids16_is_complete = 1;

	rc = ble_gap_adv_set_fields(&fields);
	if (rc != 0) {
		MODLOG_DFLT(ERROR, "error setting advertisement data; rc=%d\n", rc);
		return;
	}

	/* Begin advertising. */
	memset(&adv_params, 0, sizeof adv_params);
	adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
	adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
	rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER,
		&adv_params, bleprph_gap_event, NULL);
	if (rc != 0) {
		MODLOG_DFLT(ERROR, "error enabling advertisement; rc=%d\n", rc);
		return;
	}
}

static bool nofityStart = false, notifyState = false;
uint16_t hrs_hrm_handle;
static uint16_t conn_handle;
static void
ble_tx_notify_stop(void)
{
	if (nofityStart)
	{
		printf("callout stop!!!!!!\n");//os_callout_stop(&blehr_tx_timer);
		nofityStart = false;
	}
}

static void
ble_tx_notify_reset(void)
{
	if (!nofityStart)
	{
		int rc;
		printf("callout reset!!!!!!\n");//rc = os_callout_reset(&blehr_tx_timer, 100000);
		nofityStart = true;
		assert(rc == 0);
	}
}

static void
ble_tx_notify(struct os_event *ev)
{
	static uint8_t hrm[16];
	int rc;
	struct os_mbuf *om;

	if (!notifyState) {
		ble_tx_notify_stop();
		return;
	}

	char ip[16] = "0.0.0.0";
	/*get WiFi IP addr start*/


    if (theConfig.wifi_mode == WIFIMGR_MODE_SOFTAP)
    {
#ifdef CFG_MIRROR_TEST
        ITPWifiInfo     netInfo;
        char* msg_wifi_ip;
        if (gInDemoLayer)
        {
            ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_GET_INFO, &netInfo);
            msg_wifi_ip = ipaddr_ntoa((const ip_addr_t*)&netInfo.address);
        }
        else
            msg_wifi_ip = "0.0.0.0";

        printf("ip: %s \n", msg_wifi_ip);
        memcpy(ip, msg_wifi_ip, 16);
#endif
    }
	else
	{
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
	}

	memcpy(hrm, ip, 16);

	om = ble_hs_mbuf_from_flat(hrm, sizeof(hrm));

	rc = ble_gattc_notify_custom(conn_handle, hrs_hrm_handle, om);

	assert(rc == 0);
	ble_tx_notify_reset();
}

static void *taskA(void *arg)
{
	while (1)
	{
		usleep(1000 * 1000);
		if (nofityStart)
		{
			ble_tx_notify(NULL);
		}
	}
}

/**
* The nimble host executes this callback when a GAP event occurs.  The
* application associates a GAP event callback with each connection that forms.
* bleprph uses the same callback for all connections.
*
* @param event                 The type of event being signalled.
* @param ctxt                  Various information pertaining to the event.
* @param arg                   Application-specified argument; unuesd by
*                                  bleprph.
*
* @return                      0 if the application successfully handled the
*                                  event; nonzero on failure.  The semantics
*                                  of the return code is specific to the
*                                  particular GAP event being signalled.
*/
static int
bleprph_gap_event(struct ble_gap_event *event, void *arg)
{
	struct ble_gap_conn_desc desc;
	int rc;
	printf("get event: %d\n", event->type);
	switch (event->type) {
	case BLE_GAP_EVENT_CONNECT:
		/* A new connection was established or a connection attempt failed. */
		MODLOG_DFLT(INFO, "connection %s; status=%d ",
			event->connect.status == 0 ? "established" : "failed",
			event->connect.status);
		if (event->connect.status == 0) {
			rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
			assert(rc == 0);
			bleprph_print_conn_desc(&desc);

#if MYNEWT_VAL(BLEPRPH_LE_PHY_SUPPORT)
			phy_conn_changed(event->connect.conn_handle);
#endif
		}
		MODLOG_DFLT(INFO, "\n");

		if (event->connect.status != 0) {
			/* Connection failed; resume advertising. */
			bleprph_advertise();
			conn_handle = 0;
		}
		else
		{
			conn_handle = event->connect.conn_handle;
		}
		return 0;

	case BLE_GAP_EVENT_DISCONNECT:
		MODLOG_DFLT(INFO, "disconnect; reason=%d ", event->disconnect.reason);
		bleprph_print_conn_desc(&event->disconnect.conn);
		MODLOG_DFLT(INFO, "\n");

#if MYNEWT_VAL(BLEPRPH_LE_PHY_SUPPORT)
		phy_conn_changed(CONN_HANDLE_INVALID);
#endif

		/* Connection terminated; resume advertising. */
		bleprph_advertise();
		return 0;

	case BLE_GAP_EVENT_CONN_UPDATE:
		/* The central has updated the connection parameters. */
		MODLOG_DFLT(INFO, "connection updated; status=%d ",
			event->conn_update.status);
		rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
		assert(rc == 0);
		bleprph_print_conn_desc(&desc);
		MODLOG_DFLT(INFO, "\n");
		return 0;

	case BLE_GAP_EVENT_ADV_COMPLETE:
		MODLOG_DFLT(INFO, "advertise complete; reason=%d",
			event->adv_complete.reason);
		bleprph_advertise();
		return 0;

	case BLE_GAP_EVENT_ENC_CHANGE:
		/* Encryption has been enabled or disabled for this connection. */
		MODLOG_DFLT(INFO, "encryption change event; status=%d ",
			event->enc_change.status);
		rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
		assert(rc == 0);
		bleprph_print_conn_desc(&desc);
		MODLOG_DFLT(INFO, "\n");
		return 0;

	case BLE_GAP_EVENT_SUBSCRIBE:
		MODLOG_DFLT(INFO, "subscribe event; conn_handle=%d attr_handle=%d "
			"reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
			event->subscribe.conn_handle,
			event->subscribe.attr_handle,
			event->subscribe.reason,
			event->subscribe.prev_notify,
			event->subscribe.cur_notify,
			event->subscribe.prev_indicate,
			event->subscribe.cur_indicate);
		if (event->subscribe.attr_handle == hrs_hrm_handle) {
			notifyState = event->subscribe.cur_notify;
			ble_tx_notify_reset();
		}
		else if (event->subscribe.attr_handle != hrs_hrm_handle) {
			notifyState = event->subscribe.cur_notify;
			ble_tx_notify_stop();
		}
		break;
		return 0;

	case BLE_GAP_EVENT_MTU:
		MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
			event->mtu.conn_handle,
			event->mtu.channel_id,
			event->mtu.value);
		return 0;

	case BLE_GAP_EVENT_REPEAT_PAIRING:
		/* We already have a bond with the peer, but it is attempting to
		* establish a new secure link.  This app sacrifices security for
		* convenience: just throw away the old bond and accept the new link.
		*/

		/* Delete the old bond. */
		rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
		assert(rc == 0);
		ble_store_util_delete_peer(&desc.peer_id_addr);

		/* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
		* continue with the pairing operation.
		*/
		return BLE_GAP_REPEAT_PAIRING_RETRY;

#if MYNEWT_VAL(BLEPRPH_LE_PHY_SUPPORT)
	case BLE_GAP_EVENT_PHY_UPDATE_COMPLETE:
		/* XXX: assume symmetric phy for now */
		phy_update(event->phy_updated.tx_phy);
		return 0;
#endif
	}

	return 0;
}

static void
bleprph_on_reset(int reason)
{
	MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

static void
bleprph_on_sync(void)
{
	int rc;

	/* Make sure we have proper identity address set (public preferred) */
	rc = ble_hs_util_ensure_addr(0);
	assert(rc == 0);

	/* Begin advertising. */
	bleprph_advertise();
}

/**
* main
*
* The main task for the project. This function initializes the packages,
* then starts serving events from default event queue.
*
* @return int NOTE: this function should never return!
*/
void*
TestBLE(void* arg)
{
#if MYNEWT_VAL(BLE_SVC_DIS_FIRMWARE_REVISION_READ_PERM) >= 0
	struct image_version ver;
	static char ver_str[IMGMGR_NMGR_MAX_VER];
#endif
	int rc;

	/* Initialize OS */
	nimble_port_init(); //sysinit();

	/* Initialize the NimBLE host configuration. */
	ble_hs_cfg.reset_cb = bleprph_on_reset;
	ble_hs_cfg.sync_cb = bleprph_on_sync;
	ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
	ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

	rc = gatt_svr_init();
	assert(rc == 0);

	int res;
	pthread_t task;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	res = pthread_create(&task, &attr, taskA, NULL);

#if MYNEWT_VAL(BLE_SVC_DIS_FIRMWARE_REVISION_READ_PERM) >= 0
	/* Set firmware version in DIS */
	imgr_my_version(&ver);
	imgr_ver_str(&ver, ver_str);
	ble_svc_dis_firmware_revision_set(ver_str);
#endif

#if MYNEWT_VAL(BLEPRPH_LE_PHY_SUPPORT)
	phy_init();
#endif

	//conf_load();

	/* If this app is acting as the loader in a split image setup, jump into
	* the second stage application instead of starting the OS.
	*/
#if MYNEWT_VAL(SPLIT_LOADER)
	{
		void *entry;
		rc = split_app_go(&entry, true);
		if (rc == 0) {
			hal_system_start(entry);
		}
	}
#endif

	/*
	* As the last thing, process events from default event queue.
	*/
	nimble_port_run();

	return NULL;
}
