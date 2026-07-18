/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
/*
** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/os/linux/include/gl_cfg80211.h#1
*/

/*! \file   gl_cfg80211.h
*    \brief  This file is for Portable Driver linux cfg80211 support.
*/


#ifndef _GL_CFG80211_H
#define _GL_CFG80211_H

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#if (!CFG_SUPPORT_NO_SUPPLICANT_OPS) || (!CFG_SUPPORT_NO_SUPPLICANT_OPS_P2P)
#include "driver.h"
#include "common/defs.h"
#include "openrtos/FreeRTOS.h"
#include "openrtos/semphr.h"
#endif

#if !CFG_SUPPORT_NO_SUPPLICANT_OPS
#include "driver_inband.h"
#endif

#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "mtk_wireless.h"

#if CFG_ENABLE_WIFI_DIRECT
#include "dhcpd.h"
#endif

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
/* code without supplicant */
#define ETH_ALEN	6		/* Octets in one ethernet addr	 */
#define ETH_HLEN	14		/* Total octets in header.	 */
#define ETH_ZLEN	60		/* Min. octets in frame sans FCS */
#define ETH_DATA_LEN	1500		/* Max. octets in payload	 */
#define ETH_FRAME_LEN	1514		/* Max. octets in frame sans FCS */
#define ETH_FCS_LEN	4		/* Octets in the FCS		 */
#define IEEE80211_MAX_SSID_LEN		32

extern const uint8_t bcast_addr[ETH_ALEN];


/* needed by mgmt/rlm_domain.c */
#define regulatory_hint(_wiphy, _alpha2) \
	KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__)
/*
 * needed by
 * mgmt/rlm_domain.c
 * nic/nic_cmd_event.c
 */
#define priv_to_wiphy(_priv) \
	((void *) KAL_NEED_IMPLEMENT(__FILE__, __func__, __LINE__, _priv))


/**
 * enum nl80211_band - Frequency band
 * @NL80211_BAND_2GHZ: 2.4 GHz ISM band
 * @NL80211_BAND_5GHZ: around 5 GHz band (4.9 - 5.7 GHz)
 * @NL80211_BAND_60GHZ: around 60 GHz band (58.32 - 64.80 GHz)
 **/
enum nl80211_band {
	NL80211_BAND_2GHZ,
	NL80211_BAND_5GHZ,
	NL80211_BAND_60GHZ,
	NUM_NL80211_BANDS,
};

/* fix redefined */
#if CFG_SUPPORT_NO_SUPPLICANT_OPS
enum nl80211_auth_type {
	NL80211_AUTHTYPE_OPEN_SYSTEM,
	NL80211_AUTHTYPE_SHARED_KEY,
	NL80211_AUTHTYPE_FT,
	NL80211_AUTHTYPE_NETWORK_EAP,
	NL80211_AUTHTYPE_SAE,
	/* keep last */
	__NL80211_AUTHTYPE_NUM,
	NL80211_AUTHTYPE_MAX = __NL80211_AUTHTYPE_NUM - 1,
	NL80211_AUTHTYPE_AUTOMATIC
};
#endif

enum ieee80211_band {
	IEEE80211_BAND_2GHZ = NL80211_BAND_2GHZ,
	IEEE80211_BAND_5GHZ = NL80211_BAND_5GHZ,
	IEEE80211_BAND_60GHZ = NL80211_BAND_60GHZ,
	/* keep last */
	IEEE80211_NUM_BANDS
};

struct ieee80211_channel {
	enum ieee80211_band band;
	uint16_t center_freq;
	uint16_t hw_value;
	uint32_t flags;
	int max_antenna_gain;
	int max_power;
	/* int max_reg_power; */
	/* bool beacon_found; */
	uint32_t orig_flags;
	/* int orig_mag, orig_mpwr; */
	 enum nl80211_dfs_state dfs_state;
	/* unsigned long dfs_state_entered; */
	/* unsigned int dfs_cac_ms; */
};


/* needed by mgmt/rlm_domain.c */
struct ieee80211_supported_band {
	struct ieee80211_channel *channels;
	uint32_t n_channels;
};

/* needed by
 * mgmt/rlm_domain.c
 * nic/nic_cmd_event.c
 */
struct wiphy {
	struct ieee80211_supported_band *bands[NUM_NL80211_BANDS];
};
/* needed by nic/nic_cmd_event.c */
struct wireless_dev {
	struct wiphy *wiphy;
};

struct cfg80211_connect_params {
	struct ieee80211_channel *channel;
	/* struct ieee80211_channel *channel_hint; */
	char bssid[ETH_ALEN];
	/* const uint8_t *bssid_hint; */
	/* const uint8_t ssid[IEEE80211_MAX_SSID_LEN]; */
	char ssid[IEEE80211_MAX_SSID_LEN];
	uint32_t ssid_len;
	enum nl80211_auth_type auth_type;
	/* const uint8_t *ie; */
	/* uint32_t ie_len; */
	bool privacy;
	/* enum nl80211_mfp mfp; */
	/* struct cfg80211_crypto_settings crypto; */
	/* const uint8_t *key; */
	/* uint8_t key_len, key_idx; */
	/* uint32_t flags; */
	/* int bg_scan_period; */
	/* struct ieee80211_ht_cap ht_capa; */
	/* struct ieee80211_ht_cap ht_capa_mask; */
	/* struct ieee80211_vht_cap vht_capa; */
	/* struct ieee80211_vht_cap vht_capa_mask; */
};
/* code without supplicant */

#ifdef CONFIG_NL80211_TESTMODE
#define NL80211_DRIVER_TESTMODE_VERSION 2
#endif

#define WPAS_MAX_SCAN_SSIDS 16

extern const uint8_t bcast_addr[ETH_ALEN];
/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

#ifdef CONFIG_NL80211_TESTMODE
#if CFG_SUPPORT_NFC_BEAM_PLUS

typedef struct _NL80211_DRIVER_SET_NFC_PARAMS {
	NL80211_DRIVER_TEST_MODE_PARAMS hdr;
	uint32_t NFC_Enable;

} NL80211_DRIVER_SET_NFC_PARAMS, *P_NL80211_DRIVER_SET_NFC_PARAMS;

#endif


typedef struct _NL80211_DRIVER_GET_STA_STATISTICS_PARAMS {
	NL80211_DRIVER_TEST_MODE_PARAMS hdr;
	uint32_t u4Version;
	uint32_t u4Flag;
	uint8_t aucMacAddr[MAC_ADDR_LEN];
} NL80211_DRIVER_GET_STA_STATISTICS_PARAMS, *P_NL80211_DRIVER_GET_STA_STATISTICS_PARAMS;

typedef enum _ENUM_TESTMODE_STA_STATISTICS_ATTR {
	NL80211_TESTMODE_STA_STATISTICS_INVALID = 0,
	NL80211_TESTMODE_STA_STATISTICS_VERSION,
	NL80211_TESTMODE_STA_STATISTICS_MAC,
	NL80211_TESTMODE_STA_STATISTICS_LINK_SCORE,
	NL80211_TESTMODE_STA_STATISTICS_FLAG,

	NL80211_TESTMODE_STA_STATISTICS_PER,
	NL80211_TESTMODE_STA_STATISTICS_RSSI,
	NL80211_TESTMODE_STA_STATISTICS_PHY_MODE,
	NL80211_TESTMODE_STA_STATISTICS_TX_RATE,

	NL80211_TESTMODE_STA_STATISTICS_TOTAL_CNT,
	NL80211_TESTMODE_STA_STATISTICS_THRESHOLD_CNT,
	NL80211_TESTMODE_STA_STATISTICS_AVG_PROCESS_TIME,

	NL80211_TESTMODE_STA_STATISTICS_FAIL_CNT,
	NL80211_TESTMODE_STA_STATISTICS_TIMEOUT_CNT,
	NL80211_TESTMODE_STA_STATISTICS_AVG_AIR_TIME,

	NL80211_TESTMODE_STA_STATISTICS_TC_EMPTY_CNT_ARRAY,
	NL80211_TESTMODE_STA_STATISTICS_TC_QUE_LEN_ARRAY,

	NL80211_TESTMODE_STA_STATISTICS_TC_AVG_QUE_LEN_ARRAY,
	NL80211_TESTMODE_STA_STATISTICS_TC_CUR_QUE_LEN_ARRAY,

	NL80211_TESTMODE_STA_STATISTICS_RESERVED_ARRAY,

	NL80211_TESTMODE_STA_STATISTICS_NUM
} ENUM_TESTMODE_STA_STATISTICS_ATTR;
#endif

// #include "nl80211_copy.h"
/* copy from nl80211_copy.h */
enum nl80211_commands {
	/* don't change the order or add anything between, this is ABI! */
	NL80211_CMD_UNSPEC,

	NL80211_CMD_GET_WIPHY,    /* can dump */
	NL80211_CMD_SET_WIPHY,
	NL80211_CMD_NEW_WIPHY,
	NL80211_CMD_DEL_WIPHY,

	NL80211_CMD_GET_INTERFACE,  /* can dump */
	NL80211_CMD_SET_INTERFACE,
	NL80211_CMD_NEW_INTERFACE,
	NL80211_CMD_DEL_INTERFACE,

	NL80211_CMD_GET_KEY,
	NL80211_CMD_SET_KEY, /* 10 */
	NL80211_CMD_NEW_KEY,
	NL80211_CMD_DEL_KEY,

	NL80211_CMD_GET_BEACON,
	NL80211_CMD_SET_BEACON,
	NL80211_CMD_NEW_BEACON,
	NL80211_CMD_DEL_BEACON,

	NL80211_CMD_GET_STATION,
	NL80211_CMD_SET_STATION,
	NL80211_CMD_NEW_STATION,
	NL80211_CMD_DEL_STATION, /* 20 */

	NL80211_CMD_GET_MPATH,
	NL80211_CMD_SET_MPATH,
	NL80211_CMD_NEW_MPATH,
	NL80211_CMD_DEL_MPATH,

	NL80211_CMD_SET_BSS,

	NL80211_CMD_SET_REG,
	NL80211_CMD_REQ_SET_REG,

	NL80211_CMD_GET_MESH_CONFIG,
	NL80211_CMD_SET_MESH_CONFIG,

	NL80211_CMD_SET_MGMT_EXTRA_IE, /* 30 */

	NL80211_CMD_GET_REG,

	NL80211_CMD_GET_SCAN,
	NL80211_CMD_TRIGGER_SCAN,
	NL80211_CMD_NEW_SCAN_RESULTS,
	NL80211_CMD_SCAN_ABORTED,

	NL80211_CMD_REG_CHANGE,

	NL80211_CMD_AUTHENTICATE,
	NL80211_CMD_ASSOCIATE,
	NL80211_CMD_DEAUTHENTICATE,
	NL80211_CMD_DISASSOCIATE,

	NL80211_CMD_MICHAEL_MIC_FAILURE,

	NL80211_CMD_REG_BEACON_HINT,

	NL80211_CMD_JOIN_IBSS,
	NL80211_CMD_LEAVE_IBSS,

	NL80211_CMD_TESTMODE,

	NL80211_CMD_CONNECT,
	NL80211_CMD_ROAM,
	NL80211_CMD_DISCONNECT,

	NL80211_CMD_SET_WIPHY_NETNS,

	NL80211_CMD_GET_SURVEY,
	NL80211_CMD_NEW_SURVEY_RESULTS,

	NL80211_CMD_SET_PMKSA,
	NL80211_CMD_DEL_PMKSA,
	NL80211_CMD_FLUSH_PMKSA,

	NL80211_CMD_REMAIN_ON_CHANNEL,
	NL80211_CMD_CANCEL_REMAIN_ON_CHANNEL,

	NL80211_CMD_SET_TX_BITRATE_MASK,

	NL80211_CMD_REGISTER_FRAME,
	NL80211_CMD_REGISTER_ACTION = NL80211_CMD_REGISTER_FRAME,
	NL80211_CMD_FRAME,
	NL80211_CMD_ACTION = NL80211_CMD_FRAME,
	NL80211_CMD_FRAME_TX_STATUS,
	NL80211_CMD_ACTION_TX_STATUS = NL80211_CMD_FRAME_TX_STATUS,

	NL80211_CMD_SET_POWER_SAVE,
	NL80211_CMD_GET_POWER_SAVE,

	NL80211_CMD_SET_CQM,
	NL80211_CMD_NOTIFY_CQM,

	NL80211_CMD_SET_CHANNEL,
	NL80211_CMD_SET_WDS_PEER,

	NL80211_CMD_FRAME_WAIT_CANCEL,

	NL80211_CMD_JOIN_MESH,
	NL80211_CMD_LEAVE_MESH,

	NL80211_CMD_UNPROT_DEAUTHENTICATE,
	NL80211_CMD_UNPROT_DISASSOCIATE,

	NL80211_CMD_NEW_PEER_CANDIDATE,

	NL80211_CMD_GET_WOWLAN,
	NL80211_CMD_SET_WOWLAN,

	NL80211_CMD_START_SCHED_SCAN,
	NL80211_CMD_STOP_SCHED_SCAN,
	NL80211_CMD_SCHED_SCAN_RESULTS,
	NL80211_CMD_SCHED_SCAN_STOPPED,

	NL80211_CMD_SET_REKEY_OFFLOAD,

	NL80211_CMD_PMKSA_CANDIDATE,

	NL80211_CMD_TDLS_OPER,
	NL80211_CMD_TDLS_MGMT,

	NL80211_CMD_UNEXPECTED_FRAME,

	NL80211_CMD_PROBE_CLIENT,

	NL80211_CMD_REGISTER_BEACONS,

	NL80211_CMD_UNEXPECTED_4ADDR_FRAME,

	NL80211_CMD_SET_NOACK_MAP,

	NL80211_CMD_EXTERNAL_AUTH,

	NL80211_CMD_ACS,
	/* add new commands above here */
	NL80211_CMD_DISCONNECT_LOCALLY,

	NL80211_CMD_FT_INDICATION,

	/* used to define NL80211_CMD_MAX below */
	__NL80211_CMD_AFTER_LAST,
	NL80211_CMD_MAX = __NL80211_CMD_AFTER_LAST - 1
};

enum nl80211_iftype {
	NL80211_IFTYPE_UNSPECIFIED,
	NL80211_IFTYPE_ADHOC,
	NL80211_IFTYPE_STATION,
	NL80211_IFTYPE_AP,
	NL80211_IFTYPE_AP_VLAN,
	NL80211_IFTYPE_WDS,
	NL80211_IFTYPE_MONITOR,
	NL80211_IFTYPE_MESH_POINT,
	NL80211_IFTYPE_P2P_CLIENT,
	NL80211_IFTYPE_P2P_GO,

	/* keep last */
	NUM_NL80211_IFTYPES,
	NL80211_IFTYPE_MAX = NUM_NL80211_IFTYPES - 1
};

enum nl80211_attrs {
	/* don't change the order or add anything between, this is ABI! */
	NL80211_ATTR_UNSPEC,

	NL80211_ATTR_WIPHY,
	NL80211_ATTR_WIPHY_NAME,

	NL80211_ATTR_IFINDEX,
	NL80211_ATTR_IFNAME,
	NL80211_ATTR_IFTYPE,

	NL80211_ATTR_MAC,

	NL80211_ATTR_KEY_DATA,
	NL80211_ATTR_KEY_IDX,
	NL80211_ATTR_KEY_CIPHER,
	NL80211_ATTR_KEY_SEQ,
	NL80211_ATTR_KEY_DEFAULT,

	NL80211_ATTR_BEACON_INTERVAL,
	NL80211_ATTR_DTIM_PERIOD,
	NL80211_ATTR_BEACON_HEAD,
	NL80211_ATTR_BEACON_TAIL,

	NL80211_ATTR_STA_AID,
	NL80211_ATTR_STA_FLAGS,
	NL80211_ATTR_STA_LISTEN_INTERVAL,
	NL80211_ATTR_STA_SUPPORTED_RATES,
	NL80211_ATTR_STA_VLAN,
	NL80211_ATTR_STA_INFO,

	NL80211_ATTR_WIPHY_BANDS,

	NL80211_ATTR_MNTR_FLAGS,

	NL80211_ATTR_MESH_ID,
	NL80211_ATTR_STA_PLINK_ACTION,
	NL80211_ATTR_MPATH_NEXT_HOP,
	NL80211_ATTR_MPATH_INFO,

	NL80211_ATTR_BSS_CTS_PROT,
	NL80211_ATTR_BSS_SHORT_PREAMBLE,
	NL80211_ATTR_BSS_SHORT_SLOT_TIME,

	NL80211_ATTR_HT_CAPABILITY,

	NL80211_ATTR_SUPPORTED_IFTYPES,

	NL80211_ATTR_REG_ALPHA2,
	NL80211_ATTR_REG_RULES,

	NL80211_ATTR_MESH_CONFIG,

	NL80211_ATTR_BSS_BASIC_RATES,

	NL80211_ATTR_WIPHY_TXQ_PARAMS,
	NL80211_ATTR_WIPHY_FREQ,
	NL80211_ATTR_WIPHY_CHANNEL_TYPE,

	NL80211_ATTR_KEY_DEFAULT_MGMT,

	NL80211_ATTR_MGMT_SUBTYPE,
	NL80211_ATTR_IE,

	NL80211_ATTR_MAX_NUM_SCAN_SSIDS,

	NL80211_ATTR_SCAN_FREQUENCIES,
	NL80211_ATTR_SCAN_SSIDS,
	NL80211_ATTR_GENERATION, /* replaces old SCAN_GENERATION */
	NL80211_ATTR_BSS,

	NL80211_ATTR_REG_INITIATOR,
	NL80211_ATTR_REG_TYPE,

	NL80211_ATTR_SUPPORTED_COMMANDS,

	NL80211_ATTR_FRAME,
	NL80211_ATTR_SSID,
	NL80211_ATTR_AUTH_TYPE,
	NL80211_ATTR_REASON_CODE,

	NL80211_ATTR_KEY_TYPE,

	NL80211_ATTR_MAX_SCAN_IE_LEN,
	NL80211_ATTR_CIPHER_SUITES,

	NL80211_ATTR_FREQ_BEFORE,
	NL80211_ATTR_FREQ_AFTER,

	NL80211_ATTR_FREQ_FIXED,


	NL80211_ATTR_WIPHY_RETRY_SHORT,
	NL80211_ATTR_WIPHY_RETRY_LONG,
	NL80211_ATTR_WIPHY_FRAG_THRESHOLD,
	NL80211_ATTR_WIPHY_RTS_THRESHOLD,

	NL80211_ATTR_TIMED_OUT,

	NL80211_ATTR_USE_MFP,

	NL80211_ATTR_STA_FLAGS2,

	NL80211_ATTR_CONTROL_PORT,

	NL80211_ATTR_TESTDATA,

	NL80211_ATTR_PRIVACY,

	NL80211_ATTR_DISCONNECTED_BY_AP,
	NL80211_ATTR_STATUS_CODE,

	NL80211_ATTR_CIPHER_SUITES_PAIRWISE,
	NL80211_ATTR_CIPHER_SUITE_GROUP,
	NL80211_ATTR_WPA_VERSIONS,
	NL80211_ATTR_AKM_SUITES,

	NL80211_ATTR_REQ_IE,
	NL80211_ATTR_RESP_IE,

	NL80211_ATTR_PREV_BSSID,

	NL80211_ATTR_KEY,
	NL80211_ATTR_KEYS,

	NL80211_ATTR_PID,

	NL80211_ATTR_4ADDR,

	NL80211_ATTR_SURVEY_INFO,

	NL80211_ATTR_PMKID,
	NL80211_ATTR_MAX_NUM_PMKIDS,

	NL80211_ATTR_DURATION,

	NL80211_ATTR_COOKIE,

	NL80211_ATTR_WIPHY_COVERAGE_CLASS,

	NL80211_ATTR_TX_RATES,

	NL80211_ATTR_FRAME_MATCH,

	NL80211_ATTR_ACK,

	NL80211_ATTR_PS_STATE,

	NL80211_ATTR_CQM,

	NL80211_ATTR_LOCAL_STATE_CHANGE,

	NL80211_ATTR_AP_ISOLATE,

	NL80211_ATTR_WIPHY_TX_POWER_SETTING,
	NL80211_ATTR_WIPHY_TX_POWER_LEVEL,

	NL80211_ATTR_TX_FRAME_TYPES,
	NL80211_ATTR_RX_FRAME_TYPES,
	NL80211_ATTR_FRAME_TYPE,

	NL80211_ATTR_CONTROL_PORT_ETHERTYPE,
	NL80211_ATTR_CONTROL_PORT_NO_ENCRYPT,

	NL80211_ATTR_SUPPORT_IBSS_RSN,

	NL80211_ATTR_WIPHY_ANTENNA_TX,
	NL80211_ATTR_WIPHY_ANTENNA_RX,

	NL80211_ATTR_MCAST_RATE,

	NL80211_ATTR_OFFCHANNEL_TX_OK,

	NL80211_ATTR_BSS_HT_OPMODE,

	NL80211_ATTR_KEY_DEFAULT_TYPES,

	NL80211_ATTR_MAX_REMAIN_ON_CHANNEL_DURATION,

	NL80211_ATTR_MESH_SETUP,

	NL80211_ATTR_WIPHY_ANTENNA_AVAIL_TX,
	NL80211_ATTR_WIPHY_ANTENNA_AVAIL_RX,

	NL80211_ATTR_SUPPORT_MESH_AUTH,
	NL80211_ATTR_STA_PLINK_STATE,

	NL80211_ATTR_WOWLAN_TRIGGERS,
	NL80211_ATTR_WOWLAN_TRIGGERS_SUPPORTED,

	NL80211_ATTR_SCHED_SCAN_INTERVAL,

	NL80211_ATTR_INTERFACE_COMBINATIONS,
	NL80211_ATTR_SOFTWARE_IFTYPES,

	NL80211_ATTR_REKEY_DATA,

	NL80211_ATTR_MAX_NUM_SCHED_SCAN_SSIDS,
	NL80211_ATTR_MAX_SCHED_SCAN_IE_LEN,

	NL80211_ATTR_SCAN_SUPP_RATES,

	NL80211_ATTR_HIDDEN_SSID,

	NL80211_ATTR_IE_PROBE_RESP,
	NL80211_ATTR_IE_ASSOC_RESP,

	NL80211_ATTR_STA_WME,
	NL80211_ATTR_SUPPORT_AP_UAPSD,

	NL80211_ATTR_ROAM_SUPPORT,

	NL80211_ATTR_SCHED_SCAN_MATCH,
	NL80211_ATTR_MAX_MATCH_SETS,

	NL80211_ATTR_PMKSA_CANDIDATE,

	NL80211_ATTR_TX_NO_CCK_RATE,

	NL80211_ATTR_TDLS_ACTION,
	NL80211_ATTR_TDLS_DIALOG_TOKEN,
	NL80211_ATTR_TDLS_OPERATION,
	NL80211_ATTR_TDLS_SUPPORT,
	NL80211_ATTR_TDLS_EXTERNAL_SETUP,

	NL80211_ATTR_DEVICE_AP_SME,

	NL80211_ATTR_DONT_WAIT_FOR_ACK,

	NL80211_ATTR_FEATURE_FLAGS,

	NL80211_ATTR_PROBE_RESP_OFFLOAD,

	NL80211_ATTR_PROBE_RESP,

	NL80211_ATTR_DFS_REGION,

	NL80211_ATTR_DISABLE_HT,
	NL80211_ATTR_HT_CAPABILITY_MASK,

	NL80211_ATTR_NOACK_MAP,

	/* add attributes here, update the policy in nl80211.c */

	__NL80211_ATTR_AFTER_LAST,
	NL80211_ATTR_MAX = __NL80211_ATTR_AFTER_LAST - 1
};
/* end of copy from nl80211_copy.h */


/* copy from netlink */
struct nlattr { // TODO: should be rename
	unsigned short  nla_len;
	unsigned short  nla_type;
};
/* end of copy from netlink */


#define DRIVER_MSG_PORT_STA_ONLY 6662
#define DRIVER_MSG_PORT_AP_ONLY  6663
#define DRIVER_MSG_PORT_APCLI    6664
#define DRIVER_MSG_PORT_AP       6665
#define DRIVER_MSG_PORT_P2P      6666

/* copy from driver_nl80211 */
struct gen4m_global {
	void *ctx;
	struct dl_list interfaces;
	int if_add_ifindex;
	void *drv_priv;
	//leo struct netlink_data *netlink;
	//leo struct nl_cb *nl_cb;
	//leo struct nl_handle *nl;
	int nl80211_id;
	int ioctl_sock; /* socket for ioctl() use */

	//leo struct nl_handle *nl_event;
};

struct wpa_driver_gen4m_data {
#if 0 //ndef DRV_INBAND_SHRINK_SIZE
	struct nl80211_global   *global;
	struct dl_list          list;
	struct dl_list          wiphy_list;
	char                    phyname[32];
#endif
	void *ctx;
	int ifindex;
	int if_removed;
	int if_disabled;
	int ignore_if_down_event;
	//leo struct rfkill_data *rfkill;
	struct wpa_driver_capa capa;
	int has_capability;

	int operstate;
	int scan_complete_events;

	//leo struct nl_cb *nl_cb;
#ifndef DRV_INBAND_SHRINK_SIZE
	u8 auth_bssid[ETH_ALEN];
#endif
	u8 bssid[ETH_ALEN];
	int associated;
	u8 ssid[WIFI_MAX_LENGTH_OF_SSID];
	size_t ssid_len;
	enum nl80211_iftype nlmode; // nl80211_copy.h
	enum nl80211_iftype ap_scan_as_station; // nl80211_copy.h
	unsigned int assoc_freq;

#ifndef DRV_INBAND_SHRINK_SIZE
	int monitor_sock;
	int monitor_ifidx;
	int monitor_refcount;
#endif // #ifndef DRV_INBAND_SHRINK_SIZE

	unsigned int disabled_11b_rates: 1;
	unsigned int pending_remain_on_chan: 1;
	unsigned int in_interface_list: 1;
	unsigned int device_ap_sme: 1;
	unsigned int poll_command_supported: 1;
	unsigned int data_tx_status: 1;
	unsigned int scan_for_auth: 1;
	unsigned int retry_auth: 1;
	unsigned int use_monitor: 1;

#ifndef DRV_INBAND_SHRINK_SIZE
	u64 remain_on_chan_cookie;
	u64 send_action_cookie;

	unsigned int last_mgmt_freq;

	//leo struct wpa_driver_scan_filter *filter_ssids;
	size_t num_filter_ssids;

	//leo struct i802_bss first_bss;

	int eapol_tx_sock;
#endif // #ifndef DRV_INBAND_SHRINK_SIZE

	struct l2_packet_data *sock_recv;

#ifdef HOSTAPD
	struct hostapd_data *hapd;

	int eapol_sock; /* socket for EAPOL frames */

	int default_if_indices[16];
	int *if_indices;
	int num_if_indices;

	int last_freq;
	int last_freq_ht;
#endif /* HOSTAPD */

#ifndef DRV_INBAND_SHRINK_SIZE
	/* From failed authentication command */
	int auth_freq;
	u8 auth_bssid_[ETH_ALEN];
	u8 auth_ssid[32];
	size_t auth_ssid_len;
	int auth_alg;
	u8 *auth_ie;
	size_t auth_ie_len;
	u8 auth_wep_key[4][16];
	size_t auth_wep_key_len[4];
	int auth_wep_tx_keyidx;
	int auth_local_state_change;
	int auth_p2p;
#endif // #ifndef DRV_INBAND_SHRINK_SIZE
	int driver_msg_sock;
	int driver_msg_port;

	u8 own_addr[ETH_ALEN];
	int connect_wmm_ap;
	u8 ignore_sta_disconnect_event;
};

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
/* cfg80211 hooks */
/* FreeRTOS redine interface with upperlayer state machine */
int mtk_freertos_wpa_get_ssid(uint8_t *ssid, uint32_t *ssid_len);

int mtk_freertos_wpa_get_bssid(uint8_t *bssid);

int mtk_freertos_connect(uint16_t center_freq, uint8_t *bssid,
				uint8_t *ssid, uint32_t ssid_len);

int mtk_freertos_scan(uint16_t *ch_list, int num_ch);

/*For mini supplicant*/
#if (!CFG_SUPPORT_NO_SUPPLICANT_OPS) || (!CFG_SUPPORT_NO_SUPPLICANT_OPS_P2P)
extern SemaphoreHandle_t g_wait_drv_ready;
#endif

#if (!CFG_SUPPORT_NO_SUPPLICANT_OPS)
int mtk_freertos_wpa_get_channel_list_full(
				uint8_t ucSpecificBand,
				uint8_t ucMaxChannelNum,
				uint8_t ucNoDfs,
				uint8_t *pucNumOfChannel,
				uint8_t *paucChannelList);

int mtk_freertos_wpa_set_favor_ssid(void *priv,
	uint8_t *ssid, uint8_t len);

int mtk_freertos_wpa_scan(void *priv,
				struct wpa_driver_scan_params *req);

struct wpa_scan_results *
mtk_freertos_wpa_scan_results(void *priv, int *num);

int mtk_freertos_wpa_associate(void *priv,
				struct wpa_driver_associate_params *params);

int mtk_freertos_wpa_set_power(uint8_t listen_int_present, uint8_t listen_int,
				uint8_t ps_mode_present, uint8_t ps_mode);

int
mtk_freertos_wpa_disconnect(void *priv, const uint8_t *addr, int reason_code);

int
mtk_freertos_mgmt_tx(void *priv, int freq, const uint8_t *buf,
		     uint32_t len, uint64_t *cookie, int no_cck, int offchan);

int
mtk_freertos_wpa_set_key(const char *ifname, void *priv,
					enum wpa_alg alg, const uint8_t *addr,
					int key_idx, int set_tx,
					const uint8_t *seq, uint32_t seq_len,
					const uint8_t *key, uint32_t key_len);

int
mtk_freertos_wpa_set_rekey_data(void *priv,
					const uint8_t *kek, size_t kek_len,
					const uint8_t *kck, size_t kck_len,
					const uint8_t *replay_ctr);

int mtk_freertos_update_ft_ies(uint16_t mdid, const uint8_t *ies,
			       size_t ies_len);

int mtk_freertos_abort_scan(void);

int mtk_freertos_wpa_external_auth(struct external_auth *params);

int mtk_freertos_wpa_send_mlme(void *priv, const u8 *buf, size_t len);

int mtk_freertos_wpa_add_pmkid(void *priv, const u8 *bssid, const u8 *pmkid);

int mtk_freertos_wpa_remove_pmkid(void *priv, const u8 *bssid, const u8 *pmkid);

int mtk_freertos_wpa_flush_pmkid(void *priv);

#if !CFG_SUPPORT_NO_SUPPLICANT_OPS_P2P /*For mini supplicant*/

int mtk_p2p_freertos_wpa_get_channel_list(
					uint8_t ucSpecificBand,
					uint8_t ucMaxChannelNum,
					uint8_t *pucNumOfChannel,
					uint8_t *paucChannelList);

int mtk_p2p_freertos_wpa_start_ap(void *priv,
			struct wpa_driver_ap_params *settings);

int mtk_p2p_freertos_wpa_do_acs(void *priv,
					 struct drv_acs_params *params);

int mtk_p2p_freertos_wpa_stop_ap(VOID);

int mtk_p2p_freertos_wpa_del_station(void *priv,
							const uint8_t *mac);

int mtk_p2p_freertos_wpa_set_ap(void *priv,
					 struct wpa_driver_ap_params *params);

int mtk_p2p_freertos_wpa_change_beacon(void *priv,
					 struct wpa_driver_ap_params *info);

int mtk_p2p_freertos_wpa_mgmt_tx(void *priv,
				int freq, unsigned int wait_time,
				const void *data, uint32_t len, uint64_t *cookie,
				int no_cck, int no_ack, int offchan);

void mtk_p2p_freertos_wpa_add_iface(void);

void mtk_p2p_freertos_wpa_change_iface(void);

void mtk_p2p_freertos_wpa_del_iface(void);

#endif
#endif
/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

const uint8_t *mtk_cfg80211_find_ie_match_mask(uint8_t eid,
				const uint8_t *ies, int len,
				const uint8_t *match,
				int match_len, int match_offset,
				const uint8_t *match_mask);

err_t mtk_freertos_wlan_send(struct netif *netif, struct pbuf *p);
void *mtk_freertos_wpa_global_init(void *ctx);

void mtk_freertos_wpa_global_deinit(void *priv);

#if CFG_SUPPORT_SUPPLICANT_MBO
u_int8_t wextSrchDesiredSupOpClassIE(IN uint8_t *pucIEStart,
			IN int32_t i4TotalIeLen, OUT uint8_t **ppucDesiredIE);

u_int8_t wextSrchDesiredMboIE(IN uint8_t *pucIEStart,
			IN int32_t i4TotalIeLen, OUT uint8_t **ppucDesiredIE);
#endif /* CFG_SUPPORT_SUPPLICANT_MBO */

#if CFG_ENABLE_WIFI_DIRECT
void wifi_dhcpd_init(dhcpd_settings_t *dhcpd_settings);
#endif

#endif /* _GL_CFG80211_H */
