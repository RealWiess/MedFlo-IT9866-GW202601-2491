/*****************************************************************************
**
**  Name:          btm_ble_api.h
**
**  Description:   This file contains the Bluetooth Manager (BTM) API function
**                 external definitions.
**
**
**  Copyright (c) 1999-2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/
#ifndef BTM_BLE_API_H
#define BTM_BLE_API_H

#include "btm_api.h"

#define CHNL_MAP_LEN    5
typedef UINT8 tBTM_BLE_CHNL_MAP[CHNL_MAP_LEN];

/* 0x00-0x04 only used for set advertising parameter command */
#define BTM_BLE_CONNECT_EVT     0x00        /* Connectable undirected advertising */
#define BTM_BLE_CONNECT_DIR_EVT 0x01        /* Connectable directed advertising */
#define BTM_BLE_DISCOVER_EVT    0x02        /* Scannable undirected advertising */
#define BTM_BLE_NON_CONNECT_EVT 0x03        /* Non connectable undirected advertising */
#define BTM_BLE_CONNECT_LO_DUTY_DIR_EVT 0x04        /* Connectable low duty cycle directed advertising  */
/* 0x00 - 0x05 can be received on adv event type */
#define BTM_BLE_SCAN_RSP_EVT    0x04
#define BTM_BLE_SCAN_REQ_EVT    0x05
#define BTM_BLE_UNKNOWN_EVT     0xff

#define BTM_BLE_UNKNOWN_EVT     0xff

typedef UINT8 tBTM_BLE_EVT;
typedef UINT8 tBTM_BLE_CONN_MODE;

typedef UINT32 tBTM_BLE_REF_VALUE;

#define BTM_BLE_SCAN_MODE_PASS      0
#define BTM_BLE_SCAN_MODE_ACTI      1
#define BTM_BLE_SCAN_MODE_NONE      0xff
typedef UINT8 tBTM_BLE_SCAN_MODE;
typedef UINT8 tBLE_SCAN_MODE;


#define BTM_BLE_BATCH_SCAN_MODE_DISABLE 0
#define BTM_BLE_BATCH_SCAN_MODE_PASS  1
#define BTM_BLE_BATCH_SCAN_MODE_ACTI  2
#define BTM_BLE_BATCH_SCAN_MODE_PASS_ACTI 3

typedef UINT8 tBTM_BLE_BATCH_SCAN_MODE;


/* advertising channel map */
#define BTM_BLE_ADV_CHNL_37    (0x01 << 0)
#define BTM_BLE_ADV_CHNL_38    (0x01 << 1)
#define BTM_BLE_ADV_CHNL_39    (0x01 << 2)
typedef UINT8 tBTM_BLE_ADV_CHNL_MAP;

/*default advertising channel map */
#ifndef BTM_BLE_DEFAULT_ADV_CHNL_MAP
#define BTM_BLE_DEFAULT_ADV_CHNL_MAP   (BTM_BLE_ADV_CHNL_37| BTM_BLE_ADV_CHNL_38| BTM_BLE_ADV_CHNL_39)
#endif

/* advertising filter policy */
#define AP_SCAN_CONN_ALL           0x00        /* default */
#define AP_SCAN_WL_CONN_ALL        0x01
#define AP_SCAN_ALL_CONN_WL        0x02
#define AP_SCAN_CONN_WL            0x03
#define AP_SCAN_CONN_POLICY_MAX    0x04
typedef UINT8   tBTM_BLE_AFP;

/* default advertising filter policy */
#ifndef BTM_BLE_DEFAULT_AFP
#define BTM_BLE_DEFAULT_AFP   AP_SCAN_CONN_ALL
#endif

/* scanning filter policy */
#define SP_ADV_ALL     0x00     /* 0: accept adv packet from all, directed adv pkt not directed
                                      to local device is ignored */
#define SP_ADV_WL      0x01     /* 1: accept adv packet from device in white list, directed adv
                                      packet not directed to local device is ignored */
#define SP_ADV_ALL_RPA_DIR_ADV     0x02     /* 2: accept adv packet from all, directed adv pkt
                                      not directed to me is ignored except direct adv with RPA */
#define SP_ADV_WL_RPA_DIR_ADV      0x03     /* 3: accept adv packet from device in white list,
                        directed adv pkt not directed to me is ignored except direct adv with RPA */
typedef UINT8   tBTM_BLE_SFP;

#ifndef BTM_BLE_DEFAULT_SFP
#define BTM_BLE_DEFAULT_SFP   SP_ADV_ALL
#endif

/* adv parameter boundary values */
#define BTM_BLE_ADV_INT_MIN            0x0020
#define BTM_BLE_ADV_INT_MAX            0x4000

/* Full scan boundary values */
#define BTM_BLE_ADV_SCAN_FULL_MIN      0x00
#define BTM_BLE_ADV_SCAN_FULL_MAX      0x64

/* Partial scan boundary values */
#define BTM_BLE_ADV_SCAN_TRUNC_MIN      BTM_BLE_ADV_SCAN_FULL_MIN
#define BTM_BLE_ADV_SCAN_TRUNC_MAX      BTM_BLE_ADV_SCAN_FULL_MAX

/* Threshold values */
#define BTM_BLE_ADV_SCAN_THR_MIN        BTM_BLE_ADV_SCAN_FULL_MIN
#define BTM_BLE_ADV_SCAN_THR_MAX        BTM_BLE_ADV_SCAN_FULL_MAX

/* connection parameter boundary values */
#define BTM_BLE_SCAN_INT_MIN            0x0004
#define BTM_BLE_SCAN_INT_MAX            0x4000
#define BTM_BLE_SCAN_WIN_MIN            0x0004
#define BTM_BLE_SCAN_WIN_MAX            0x4000
#define BTM_BLE_EXT_SCAN_INT_MAX        0x00FFFFFF
#define BTM_BLE_EXT_SCAN_WIN_MAX        0xFFFF
#define BTM_BLE_CONN_INT_MIN            0x0006
#define BTM_BLE_CONN_INT_MAX            0x0C80
#define BTM_BLE_CONN_LATENCY_MAX        500
#define BTM_BLE_CONN_SUP_TOUT_MIN       0x000A
#define BTM_BLE_CONN_SUP_TOUT_MAX       0x0C80
#define BTM_BLE_CONN_PARAM_UNDEF        0xffff      /* use this value when a specific value not to be overwritten */
#define BTM_BLE_CONN_SUP_TOUT_DEF       700

/* default connection parameters if not configured, use GAP recommend value for auto/selective connection */
/* default scan interval */
#ifndef BTM_BLE_SCAN_FAST_INT
#define BTM_BLE_SCAN_FAST_INT    96    /* 30 ~ 60 ms (use 60)  = 96 *0.625 */
#endif
/* default scan window for background connection, applicable for auto connection or selective conenction */
#ifndef BTM_BLE_SCAN_FAST_WIN
#define BTM_BLE_SCAN_FAST_WIN   48      /* 30 ms = 48 *0.625 */
#endif

/* default scan paramter used in reduced power cycle (background scanning) */
#ifndef BTM_BLE_SCAN_SLOW_INT_1
#define BTM_BLE_SCAN_SLOW_INT_1    2048    /* 1.28 s   = 2048 *0.625 */
#endif
#ifndef BTM_BLE_SCAN_SLOW_WIN_1
#define BTM_BLE_SCAN_SLOW_WIN_1   18      /* 11.25 ms = 18 *0.625 */
#endif

/* default scan paramter used in reduced power cycle (background scanning) */
#ifndef BTM_BLE_SCAN_SLOW_INT_2
#define BTM_BLE_SCAN_SLOW_INT_2    4096    /* 2.56 s   = 4096 *0.625 */
#endif
#ifndef BTM_BLE_SCAN_SLOW_WIN_2
#define BTM_BLE_SCAN_SLOW_WIN_2   36      /* 22.5 ms = 36 *0.625 */
#endif

/* default connection interval min */
#ifndef BTM_BLE_CONN_INT_MIN_DEF
#define BTM_BLE_CONN_INT_MIN_DEF     24      /* recommended min: 30ms  = 24 * 1.25 */
#endif
/* default connectino interval max */
#ifndef BTM_BLE_CONN_INT_MAX_DEF
#define BTM_BLE_CONN_INT_MAX_DEF     40      /* recommended max: 50 ms = 56 * 1.25 */
#endif
/* default slave latency */
#ifndef BTM_BLE_CONN_SLAVE_LATENCY_DEF
#define BTM_BLE_CONN_SLAVE_LATENCY_DEF  0      /* 0 */
#endif
/* default supervision timeout */
#ifndef BTM_BLE_CONN_TIMEOUT_DEF
#define BTM_BLE_CONN_TIMEOUT_DEF    2000
#endif

#define BTM_BLE_DIR_CONN_FALLBACK_UNDIR         1
#define BTM_BLE_DIR_CONN_FALLBACK_NO_ADV        2

#ifndef BTM_BLE_DIR_CONN_FALLBACK
#define BTM_BLE_DIR_CONN_FALLBACK   BTM_BLE_DIR_CONN_FALLBACK_UNDIR
#endif

#define BTM_CMAC_TLEN_SIZE          8                   /* 64 bits */
#define BTM_BLE_AUTH_SIGN_LEN       12                   /* BLE data signature length 8 Bytes + 4 bytes counter*/
typedef UINT8 BLE_SIGNATURE[BTM_BLE_AUTH_SIGN_LEN];         /* Device address */

#ifndef BTM_BLE_HOST_SUPPORT
#define BTM_BLE_HOST_SUPPORT        0x01
#endif

#ifndef BTM_BLE_SIMULTANEOUS_HOST
#define BTM_BLE_SIMULTANEOUS_HOST    0x01
#endif

#ifndef BTM_BLE_PRIVATE_ADDR_INT
#define BTM_BLE_PRIVATE_ADDR_INT    900           /* 15 minutes minimum for random address refreshing */
#endif

/* Structure returned with Rand/Encrypt complete callback */
typedef struct
{
    UINT8   status;
    UINT8   param_len;
    UINT16  opcode;
    UINT8   param_buf[BT_OCTET16_LEN];
} tBTM_RAND_ENC;

/* General callback function for notifying an application that a synchronous
** BTM function is complete. The pointer contains the address of any returned data.
*/
typedef void (tBTM_RAND_ENC_CB) (tBTM_RAND_ENC *p1);

#define BTM_BLE_FILTER_TARGET_SCANNER       0x01
#define BTM_BLE_FILTER_TARGET_ADVR          0x00

#define BTM_BLE_POLICY_BLACK_ALL            0x00    /* relevant to both */
#define BTM_BLE_POLICY_ALLOW_SCAN           0x01    /* relevant to advertiser */
#define BTM_BLE_POLICY_ALLOW_CONN           0x02    /* relevant to advertiser */
#define BTM_BLE_POLICY_WHITE_ALL            0x03    /* relevant to both */

/* ADV data flag bit definition used for BTM_BLE_AD_TYPE_FLAG */
#define BTM_BLE_LIMIT_DISC_FLAG         (0x01 << 0)             /* bit 0 */
#define BTM_BLE_GEN_DISC_FLAG           (0x01 << 1)             /* bit 1 */
#define BTM_BLE_BREDR_NOT_SPT           (0x01 << 2)             /* bit 2*/
/* 4.1 spec adv flag for simultaneous BR/EDR+LE connection support */
#define BTM_BLE_DMT_CONTROLLER_SPT      (0x01 << 3)             /* bit 3 */
#define BTM_BLE_DMT_HOST_SPT            (0x01 << 4)             /* bit 4 */
#define BTM_BLE_NON_LIMIT_DISC_FLAG     (0x00 )         /* lowest bit unset */
#define BTM_BLE_ADV_FLAG_MASK           (BTM_BLE_LIMIT_DISC_FLAG | BTM_BLE_BREDR_NOT_SPT | BTM_BLE_GEN_DISC_FLAG)
#define BTM_BLE_LIMIT_DISC_MASK         (BTM_BLE_LIMIT_DISC_FLAG )

#define BTM_BLE_AD_BIT_DEV_NAME        (0x00000001 << 0)
#define BTM_BLE_AD_BIT_FLAGS           (0x00000001 << 1)
#define BTM_BLE_AD_BIT_MANU            (0x00000001 << 2)
#define BTM_BLE_AD_BIT_TX_PWR          (0x00000001 << 3)
#define BTM_BLE_AD_BIT_INT_RANGE       (0x00000001 << 5)
#define BTM_BLE_AD_BIT_SERVICE         (0x00000001 << 6)
#define BTM_BLE_AD_BIT_SERVICE_SOL     (0x00000001 << 7)
#define BTM_BLE_AD_BIT_SERVICE_DATA    (0x00000001 << 8)
#define BTM_BLE_AD_BIT_SIGN_DATA       (0x00000001 << 9)
#define BTM_BLE_AD_BIT_SERVICE_128SOL  (0x00000001 << 10)
#define BTM_BLE_AD_BIT_APPEARANCE      (0x00000001 << 11)
#define BTM_BLE_AD_BIT_PUBLIC_ADDR      (0x00000001 << 12)
#define BTM_BLE_AD_BIT_RANDOM_ADDR      (0x00000001 << 13)
#define BTM_BLE_AD_BIT_SERVICE_32       (0x00000001 << 4)
#define BTM_BLE_AD_BIT_SERVICE_32SOL    (0x00000001 << 14)
#define BTM_BLE_AD_BIT_PROPRIETARY      (0x00000001 << 15)
#define BTM_BLE_AD_BIT_SERVICE_128      (0x00000001 << 16)      /*128-bit Service UUIDs*/

typedef  UINT32  tBTM_BLE_AD_MASK;

#define BTM_BLE_AD_TYPE_FLAG            HCI_EIR_FLAGS_TYPE                  /* 0x01 */
#define BTM_BLE_AD_TYPE_16SRV_PART      HCI_EIR_MORE_16BITS_UUID_TYPE       /* 0x02 */
#define BTM_BLE_AD_TYPE_16SRV_CMPL      HCI_EIR_COMPLETE_16BITS_UUID_TYPE   /* 0x03 */
#define BTM_BLE_AD_TYPE_32SRV_PART      HCI_EIR_MORE_32BITS_UUID_TYPE       /* 0x04 */
#define BTM_BLE_AD_TYPE_32SRV_CMPL      HCI_EIR_COMPLETE_32BITS_UUID_TYPE   /* 0x05 */
#define BTM_BLE_AD_TYPE_128SRV_PART     HCI_EIR_MORE_128BITS_UUID_TYPE       /* 0x06 */
#define BTM_BLE_AD_TYPE_128SRV_CMPL     HCI_EIR_COMPLETE_128BITS_UUID_TYPE   /* 0x07 */
#define BTM_BLE_AD_TYPE_NAME_SHORT      HCI_EIR_SHORTENED_LOCAL_NAME_TYPE       /* 0x08 */
#define BTM_BLE_AD_TYPE_NAME_CMPL       HCI_EIR_COMPLETE_LOCAL_NAME_TYPE        /* 0x09 */
#define BTM_BLE_AD_TYPE_TX_PWR          HCI_EIR_TX_POWER_LEVEL_TYPE             /* 0x0A */
#define BTM_BLE_AD_TYPE_SIMPLE_PAIRING_HASH_C           0x0E
#define BTM_BLE_AD_TYPE_DEV_CLASS                       0x0D
#define BTM_BLE_AD_TYPE_SM_TK                           0x10
#define BTM_BLE_AD_TYPE_SM_OOB_FLAG                     0x11
#define BTM_BLE_AD_TYPE_INT_RANGE                       0x12
#define BTM_BLE_AD_TYPE_SOL_SRV_UUID                    0x14
#define BTM_BLE_AD_TYPE_128SOL_SRV_UUID                 0x15
#define BTM_BLE_AD_TYPE_SERVICE_DATA                    0x16
#define BTM_BLE_AD_TYPE_PUBLIC_TARGET                   0x17
#define BTM_BLE_AD_TYPE_RANDOM_TARGET                   0x18
#define BTM_BLE_AD_TYPE_APPEARANCE                      0x19
#define BTM_BLE_AD_TYPE_ADV_INT                         0x1a
#define BTM_BLE_AD_TYPE_LE_ADDR                         0x1b
#define BTM_BLE_AD_TYPE_LE_ROLE                         0x1c
#define BTM_BLE_AD_TYPE_SIMPLE_PAIRING_HASH_C_256       0x1d
#define BTM_BLE_AD_TYPE_SIMPLE_PAIRING_RANDOM_R_256     0x1e
#define BTM_BLE_AD_TYPE_32SOL_SRV_UUID                  0x1f
#define BTM_BLE_AD_TYPE_32SERVICE_DATA                  0x20
#define BTM_BLE_AD_TYPE_128SERVICE_DATA                 0x21
#define BTM_BLE_AD_TYPE_LE_SECURE_CONN_CONF_VAL         0x22
#define BTM_BLE_AD_TYPE_LE_SECURE_CONN_RANDOM_VAL       0x23
#define BTM_BLE_AD_TYPE_URI                             0x24

#define BTM_BLE_AD_TYPE_MANU            HCI_EIR_MANUFACTURER_SPECIFIC_TYPE      /* 0xff */
typedef UINT8   tBTM_BLE_AD_TYPE;

/* security settings used with L2CAP LE COC */
#define BTM_SEC_LE_LINK_ENCRYPTED           0x01
#define BTM_SEC_LE_LINK_PAIRED_WITHOUT_MITM 0x02
#define BTM_SEC_LE_LINK_PAIRED_WITH_MITM    0x04

/*  Min/max Preferred  number of payload octets that the local Controller
    should include in a single Link Layer Data Channel PDU. */
#define BTM_BLE_LL_DATA_PDU_MAX     0x00fb
#define BTM_BLE_LL_DATA_PDU_MIN     0x001b

/*  Preferred maximum number of microseconds that the local Controller
    should use to transmit a single Link Layer Data Channel PDU. */
#define BTM_BLE_LL_DATA_TX_TIME_MIN     0x0148
#define BTM_BLE_LL_DATA_TX_TIME_MAX     0x0848

/* adv tx power level */
#define BTM_BLE_ADV_TX_POWER_MIN        0           /* minimum tx power */
#define BTM_BLE_ADV_TX_POWER_LOW        1           /* low tx power     */
#define BTM_BLE_ADV_TX_POWER_MID        2           /* middle tx power  */
#define BTM_BLE_ADV_TX_POWER_UPPER      3           /* upper tx power   */
#define BTM_BLE_ADV_TX_POWER_MAX        4           /* maximum tx power */
typedef UINT8 tBTM_BLE_ADV_TX_POWER;

#define BTM_BLE_ISVALID_PARAM(x, min, max)  (((x) >= (min) && (x) <= (max)) || ((x) == BTM_BLE_CONN_PARAM_UNDEF))


/* adv tx power in dBm */
typedef struct
{
    UINT8 adv_inst_max;         /* max adv instance supported in controller */
    UINT8 rpa_offloading;
    UINT16 tot_scan_results_strg;
    UINT8 max_irk_list_sz;
    UINT8 filter_support;
    UINT8 max_filter;
    UINT8 energy_support;
    BOOLEAN values_read;
    UINT16 version_supported;
    UINT16 total_trackable_advertisers;
    UINT8 extended_scan_support;
    UINT8 debug_logging_supported;
}tBTM_BLE_VSC_CB;

/* slave preferred connection interval range */
typedef struct
{
    UINT16  low;
    UINT16  hi;

}tBTM_BLE_INT_RANGE;

/* Service tag supported in the device */
typedef struct
{
    UINT8       num_service;
    BOOLEAN     list_cmpl;
    UINT16      *p_uuid;
}tBTM_BLE_SERVICE;

/* 32 bits Service supported in the device */
typedef struct
{
    UINT8       num_service;
    BOOLEAN     list_cmpl;
    UINT32      *p_uuid;
}tBTM_BLE_32SERVICE;

/* 128 bits Service supported in the device */
typedef struct
{
    BOOLEAN     list_cmpl;
    UINT8       uuid128[MAX_UUID_SIZE];
}tBTM_BLE_128SERVICE;

typedef struct
{
    UINT8       len;
    UINT8      *p_val;
}tBTM_BLE_MANU;


typedef struct
{
    tBT_UUID    service_uuid;
    UINT8       len;
    UINT8      *p_val;
}tBTM_BLE_SERVICE_DATA;

typedef struct
{
    UINT8       adv_type;
    UINT8       len;
    UINT8       *p_val;     /* number of len byte */
}tBTM_BLE_PROP_ELEM;

typedef struct
{
    UINT8                   num_elem;
    tBTM_BLE_PROP_ELEM      *p_elem;
}tBTM_BLE_PROPRIETARY;

typedef struct
{
    tBTM_BLE_INT_RANGE      int_range;         /* slave prefered conn interval range */
    tBTM_BLE_MANU           *p_manu;           /* manufactuer data */
    tBTM_BLE_SERVICE        *p_services;       /* services */
    tBTM_BLE_128SERVICE     *p_services_128b;  /* 128 bits service */
    tBTM_BLE_32SERVICE      *p_service_32b;     /* 32 bits Service UUID */
    tBTM_BLE_SERVICE        *p_sol_services;    /* 16 bits services Solicitation UUIDs */
    tBTM_BLE_32SERVICE      *p_sol_service_32b;    /* List of 32 bit Service Solicitation UUIDs */
    tBTM_BLE_128SERVICE     *p_sol_service_128b;    /* List of 128 bit Service Solicitation UUIDs */
    tBTM_BLE_PROPRIETARY    *p_proprietary;
    tBTM_BLE_SERVICE_DATA   *p_service_data;    /* service data */
    UINT16                  appearance;
    UINT8                   flag;
    UINT8                   tx_power;
}tBTM_BLE_ADV_DATA;

/* These are the fields returned in each device adv packet.  It
** is returned in the results callback if registered.
*/
typedef struct
{
    UINT8               conn_mode;
    tBTM_BLE_AD_MASK    ad_mask;        /* mask of the valid adv data field */
    UINT8               flag;
    UINT8               tx_power_level;
    UINT8               remote_name_len;
    UINT8               *p_remote_name;
    tBTM_BLE_SERVICE    service;
} tBTM_BLE_INQ_DATA;

enum
{
    BTM_BLE_CONN_NONE,
    BTM_BLE_CONN_AUTO,
    BTM_BLE_CONN_SELECTIVE
};
typedef UINT8   tBTM_BLE_CONN_TYPE;

typedef BOOLEAN (tBTM_BLE_SEL_CBACK)(BD_ADDR random_bda,     UINT8 *p_remote_name);

/* callback function for SMP signing algorithm, signed data in little endian order with tlen bits long */
typedef void (tBTM_BLE_SIGN_CBACK)(void *p_ref_data, UINT8 *p_signing_data);
typedef void (tBTM_BLE_VERIFY_CBACK)(void *p_ref_data, BOOLEAN match);
/* random address set complete callback */
typedef void (tBTM_BLE_RANDOM_SET_CBACK) (BD_ADDR random_bda);

typedef void (tBTM_START_ADV_CMPL_CBACK) (UINT8 status);
typedef void (tBTM_START_STOP_ADV_CMPL_CBACK) (UINT8 status);

#if BLE_BATCH_SCAN_INCLUDED == TRUE

//typedef UINT8 tGATT_IF;

typedef void (tBTM_BLE_SCAN_THRESHOLD_CBACK)(tBTM_BLE_REF_VALUE ref_value);
typedef void (tBTM_BLE_SCAN_REP_CBACK)(tBTM_BLE_REF_VALUE ref_value, UINT8 report_format,
                                       UINT8 num_records, UINT16 total_len,
                                       UINT8* p_rep_data, UINT8 status);
typedef void (tBTM_BLE_SCAN_SETUP_CBACK)(UINT8 evt, tBTM_BLE_REF_VALUE ref_value, UINT8 status);

#ifndef BTM_BLE_BATCH_SCAN_MAX
#define BTM_BLE_BATCH_SCAN_MAX   5
#endif

#ifndef BTM_BLE_BATCH_REP_MAIN_Q_SIZE
#define BTM_BLE_BATCH_REP_MAIN_Q_SIZE  2
#endif

typedef enum
{
    BTM_BLE_SCAN_INVALID_STATE=0,
    BTM_BLE_SCAN_ENABLE_CALLED=1,
    BTM_BLE_SCAN_ENABLED_STATE=2,
    BTM_BLE_SCAN_DISABLE_CALLED=3,
    BTM_BLE_SCAN_DISABLED_STATE=4
}tBTM_BLE_BATCH_SCAN_STATE;

enum
{
    BTM_BLE_DISCARD_OLD_ITEMS,
    BTM_BLE_DISCARD_LOWER_RSSI_ITEMS
};
typedef UINT8 tBTM_BLE_DISCARD_RULE;

typedef struct
{
    UINT8   sub_code[BTM_BLE_BATCH_SCAN_MAX];
    tBTM_BLE_BATCH_SCAN_STATE cur_state[BTM_BLE_BATCH_SCAN_MAX];
    tBTM_BLE_REF_VALUE        ref_value[BTM_BLE_BATCH_SCAN_MAX];
    UINT8   pending_idx;
    UINT8   next_idx;
}tBTM_BLE_BATCH_SCAN_OPQ;

typedef struct
{
    UINT8   rep_mode[BTM_BLE_BATCH_REP_MAIN_Q_SIZE];
    tBTM_BLE_REF_VALUE  ref_value[BTM_BLE_BATCH_REP_MAIN_Q_SIZE];
    UINT8   num_records[BTM_BLE_BATCH_REP_MAIN_Q_SIZE];
    UINT16  data_len[BTM_BLE_BATCH_REP_MAIN_Q_SIZE];
    UINT8   *p_data[BTM_BLE_BATCH_REP_MAIN_Q_SIZE];
    UINT8   pending_idx;
    UINT8   next_idx;
}tBTM_BLE_BATCH_SCAN_REP_Q;

typedef struct
{
    tBTM_BLE_BATCH_SCAN_STATE      cur_state;
    tBTM_BLE_BATCH_SCAN_MODE scan_mode;
    UINT32                  scan_interval;
    UINT32                  scan_window;
    tBLE_ADDR_TYPE          addr_type;
    tBTM_BLE_DISCARD_RULE   discard_rule;
    tBTM_BLE_BATCH_SCAN_OPQ  op_q;
    tBTM_BLE_BATCH_SCAN_REP_Q main_rep_q;
    tBTM_BLE_SCAN_SETUP_CBACK     *p_setup_cback;
    tBTM_BLE_SCAN_THRESHOLD_CBACK *p_thres_cback;
    tBTM_BLE_SCAN_REP_CBACK       *p_scan_rep_cback;
    tBTM_BLE_REF_VALUE             ref_value;
}tBTM_BLE_BATCH_SCAN_CB;
#endif

/* filter selection bit index  */
#define BTM_BLE_PF_ADDR_FILTER          0
#define BTM_BLE_PF_SRVC_DATA            1
#define BTM_BLE_PF_SRVC_UUID            2
#define BTM_BLE_PF_SRVC_SOL_UUID        3
#define BTM_BLE_PF_LOCAL_NAME           4
#define BTM_BLE_PF_MANU_DATA            5
#define BTM_BLE_PF_SRVC_DATA_PATTERN    6
#define BTM_BLE_PF_TYPE_ALL             7  /* when passed in payload filter type all, only clear action is applicable */
#define BTM_BLE_PF_TYPE_MAX             8

/* max number of filter spot for different filter type */
#ifndef BTM_BLE_MAX_UUID_FILTER
#define BTM_BLE_MAX_UUID_FILTER     8
#endif
#ifndef BTM_BLE_MAX_ADDR_FILTER
#define BTM_BLE_MAX_ADDR_FILTER     8
#endif
#ifndef BTM_BLE_PF_STR_COND_MAX
#define BTM_BLE_PF_STR_COND_MAX     4   /* apply to manu data , or local name */
#endif
#ifndef BTM_BLE_PF_STR_LEN_MAX
#define BTM_BLE_PF_STR_LEN_MAX      29  /* match for first 29 bytes */
#endif

typedef UINT8   tBTM_BLE_PF_COND_TYPE;

#define BTM_BLE_PF_LOGIC_OR              0
#define BTM_BLE_PF_LOGIC_AND             1
typedef UINT8 tBTM_BLE_PF_LOGIC_TYPE;

#define BTM_BLE_PF_ENABLE       1
#define BTM_BLE_PF_CONFIG       2
typedef UINT8 tBTM_BLE_PF_ACTION;

typedef UINT8 tBTM_BLE_PF_FILT_INDEX;

typedef UINT8 tBTM_BLE_PF_AVBL_SPACE;

#define BTM_BLE_PF_BRDCAST_ADDR_FILT  1
#define BTM_BLE_PF_SERV_DATA_CHG_FILT 2
#define BTM_BLE_PF_SERV_UUID          4
#define BTM_BLE_PF_SERV_SOLC_UUID     8
#define BTM_BLE_PF_LOC_NAME_CHECK    16
#define BTM_BLE_PF_MANUF_NAME_CHECK  32
#define BTM_BLE_PF_SERV_DATA_CHECK   64
typedef UINT16 tBTM_BLE_PF_FEAT_SEL;

#define BTM_BLE_PF_LIST_LOGIC_OR   1
#define BTM_BLE_PF_LIST_LOGIC_AND  2
typedef UINT16 tBTM_BLE_PF_LIST_LOGIC_TYPE;

#define BTM_BLE_PF_FILT_LOGIC_OR   0
#define BTM_BLE_PF_FILT_LOGIC_AND  1
typedef UINT16 tBTM_BLE_PF_FILT_LOGIC_TYPE;

typedef UINT8  tBTM_BLE_PF_RSSI_THRESHOLD;
typedef UINT8  tBTM_BLE_PF_DELIVERY_MODE;
typedef UINT16 tBTM_BLE_PF_TIMEOUT;
typedef UINT8  tBTM_BLE_PF_TIMEOUT_CNT;
typedef UINT16 tBTM_BLE_PF_ADV_TRACK_ENTRIES;

typedef struct
{
    tBTM_BLE_PF_FEAT_SEL feat_seln;
    tBTM_BLE_PF_LIST_LOGIC_TYPE logic_type;
    tBTM_BLE_PF_FILT_LOGIC_TYPE filt_logic_type;
    tBTM_BLE_PF_RSSI_THRESHOLD  rssi_high_thres;
    tBTM_BLE_PF_RSSI_THRESHOLD  rssi_low_thres;
    tBTM_BLE_PF_DELIVERY_MODE dely_mode;
    tBTM_BLE_PF_TIMEOUT found_timeout;
    tBTM_BLE_PF_TIMEOUT lost_timeout;
    tBTM_BLE_PF_TIMEOUT_CNT found_timeout_cnt;
    tBTM_BLE_PF_ADV_TRACK_ENTRIES num_of_tracking_entries;
}tBTM_BLE_PF_FILT_PARAMS;

enum
{
    BTM_BLE_SCAN_COND_ADD,
    BTM_BLE_SCAN_COND_DELETE,
    BTM_BLE_SCAN_COND_CLEAR = 2
};
typedef UINT8 tBTM_BLE_SCAN_COND_OP;

enum
{
    BTM_BLE_FILT_ENABLE_DISABLE = 1,
    BTM_BLE_FILT_CFG            = 2,
    BTM_BLE_FILT_ADV_PARAM      = 3
};

typedef UINT8 tBTM_BLE_FILT_CB_EVT;

/* BLE adv payload filtering config complete callback */
typedef void (tBTM_BLE_PF_CFG_CBACK)(tBTM_BLE_PF_ACTION action, tBTM_BLE_SCAN_COND_OP cfg_op,
                                     tBTM_BLE_PF_AVBL_SPACE avbl_space, tBTM_STATUS status,
                                     tBTM_BLE_REF_VALUE ref_value);

typedef void (tBTM_BLE_PF_CMPL_CBACK) (tBTM_BLE_PF_CFG_CBACK);

/* BLE adv payload filtering status setup complete callback */
typedef void (tBTM_BLE_PF_STATUS_CBACK) (UINT8 action, tBTM_STATUS status,
                                        tBTM_BLE_REF_VALUE ref_value);

/* BLE adv payload filtering param setup complete callback */
typedef void (tBTM_BLE_PF_PARAM_CBACK) (tBTM_BLE_PF_ACTION action_type,
                                        tBTM_BLE_PF_AVBL_SPACE avbl_space,
                                        tBTM_BLE_REF_VALUE ref_value, tBTM_STATUS status);

typedef union
{
      UINT16              uuid16_mask;
      UINT32              uuid32_mask;
      UINT8               uuid128_mask[LEN_UUID_128];
}tBTM_BLE_PF_COND_MASK;

typedef struct
{
    tBLE_BD_ADDR            *p_target_addr;     /* target address, if NULL, generic UUID filter */
    tBT_UUID                uuid;           /* UUID condition */
    tBTM_BLE_PF_LOGIC_TYPE  cond_logic;    /* AND/OR */
    tBTM_BLE_PF_COND_MASK   *p_uuid_mask;           /* UUID mask */
}tBTM_BLE_PF_UUID_COND;

typedef struct
{
    UINT8                   data_len;       /* <= 20 bytes */
    UINT8                   *p_data;
}tBTM_BLE_PF_LOCAL_NAME_COND;

typedef struct
{
    UINT16                  company_id;     /* company ID */
    UINT8                   data_len;       /* <= 20 bytes */
    UINT8                   *p_pattern;
    UINT16                  company_id_mask; /* UUID value mask */
    UINT8                   *p_pattern_mask; /* Manufacturer data matching mask,
                                                same length as data pattern,
                                                set to all 0xff, match exact data */
}tBTM_BLE_PF_MANU_COND;

typedef struct
{
    UINT16                  uuid;     /* service ID */
    UINT8                   data_len;       /* <= 20 bytes */
    UINT8                   *p_pattern;
    UINT8                   *p_pattern_mask; /* Service data matching mask, same length as data pattern,
                                                set to all 0xff, match exact data */
}tBTM_BLE_PF_SRVC_PATTERN_COND;


typedef union
{
    tBLE_BD_ADDR                            target_addr;
    tBTM_BLE_PF_LOCAL_NAME_COND             local_name; /* lcoal name filtering */
    tBTM_BLE_PF_MANU_COND                   manu_data;  /* manufactuer data filtering */
    tBTM_BLE_PF_UUID_COND                   srvc_uuid;  /* service UUID filtering */
    tBTM_BLE_PF_UUID_COND                   solicitate_uuid;   /* solicitated service UUID filtering */
    tBTM_BLE_PF_SRVC_PATTERN_COND           srvc_data;      /* service data pattern */
}tBTM_BLE_PF_COND_PARAM;

typedef struct
{
    UINT8   action_ocf[BTM_BLE_PF_TYPE_MAX];
    tBTM_BLE_REF_VALUE  ref_value[BTM_BLE_PF_TYPE_MAX];
    tBTM_BLE_PF_PARAM_CBACK  *p_filt_param_cback[BTM_BLE_PF_TYPE_MAX];
    tBTM_BLE_PF_CFG_CBACK *p_scan_cfg_cback[BTM_BLE_PF_TYPE_MAX];
    UINT8   cb_evt[BTM_BLE_PF_TYPE_MAX];
    UINT8   pending_idx;
    UINT8   next_idx;
}tBTM_BLE_ADV_FILTER_ADV_OPQ;




#define BTM_BLE_MAX_FILTER_COUNTER  (BTM_BLE_MAX_ADDR_FILTER + 1) /* per device filter + one generic filter indexed by 0 */

#ifndef BTM_CS_IRK_LIST_MAX
#define BTM_CS_IRK_LIST_MAX 0x20
#endif

/* RSSI monitoring mode */
#define BTM_BLE_RSSI_MODE_LINK          0x01
#define BTM_BLE_RSSI_MODE_ADV           0x02
typedef UINT8 tBTM_BLE_RSSI_MODE;

#define BTM_BLE_META_RSSI_MONITOR_LEN    8



#define BTM_BLE_VSE_RSSI_ADV_LEN            9
#define BTM_BLE_VSE_RSSI_LINK_LEN           4

#define BTM_BLE_MAX_FILTER_COUNTER  (BTM_BLE_MAX_ADDR_FILTER + 1) /* per device filter + one generic filter indexed by 0 */

#define BTM_BLE_BRCM_WL_ATTR_NONE       0
#define BTM_BLE_BRCM_WL_ATTR_SCAN       1
#define BTM_BLE_BRCM_WL_ATTR_INIT       2
#define BTM_BLE_BRCM_WL_ATTR_ADV        4
#define BTM_BLE_BRCM_WL_ATTR_ALL        (BTM_BLE_BRCM_WL_ATTR_SCAN|BTM_BLE_BRCM_WL_ATTR_INIT|BTM_BLE_BRCM_WL_ATTR_ADV)

typedef struct
{
    BOOLEAN    in_use;
    BD_ADDR    bd_addr;
    UINT8      pf_counter[BTM_BLE_PF_TYPE_MAX]; /* number of filter indexed by tBTM_BLE_PF_COND_TYPE */
}tBTM_BLE_PF_COUNT;

typedef struct
{
    BOOLEAN             enable;
    UINT8               op_type;
    tBTM_BLE_PF_COUNT   *p_addr_filter_count; /* per BDA filter array */
    tBLE_BD_ADDR        cur_filter_target;
    tBTM_BLE_PF_STATUS_CBACK *p_filt_stat_cback;
    tBTM_BLE_ADV_FILTER_ADV_OPQ  op_q;
}tBTM_BLE_ADV_FILTER_CB;

/* Sub codes */
#define BTM_BLE_META_PF_ENABLE          0x00
#define BTM_BLE_META_PF_FEAT_SEL        0x01
#define BTM_BLE_META_PF_ADDR            0x02
#define BTM_BLE_META_PF_UUID            0x03
#define BTM_BLE_META_PF_SOL_UUID        0x04
#define BTM_BLE_META_PF_LOCAL_NAME      0x05
#define BTM_BLE_META_PF_MANU_DATA       0x06
#define BTM_BLE_META_PF_SRVC_DATA       0x07
#define BTM_BLE_META_PF_ALL             0x08

typedef UINT8 BTM_BLE_ADV_STATE;
typedef UINT8 BTM_BLE_ADV_INFO_PRESENT;
typedef UINT8 BTM_BLE_RSSI_VALUE;
typedef UINT16 BTM_BLE_ADV_INFO_TIMESTAMP;

#define ADV_INFO_PRESENT        0x00
#define NO_ADV_INFO_PRESENT     0x01

typedef struct
{
    UINT8  client_if;
    UINT8  filt_index;
    UINT8  advertiser_state;
    UINT8  advertiser_info_present;
    UINT8  addr_type;
    UINT8  tx_power;
    INT8  rssi_value;
    UINT16 time_stamp;
    BD_ADDR bd_addr;
    UINT8  adv_pkt_len;
    UINT8  *p_adv_pkt_data;
    UINT8  scan_rsp_len;
    UINT8  *p_scan_rsp_data;
} btgatt_track_adv_info_t;

typedef btgatt_track_adv_info_t tBTM_BLE_TRACK_ADV_DATA;

typedef void (tBTM_BLE_TRACK_ADV_CBACK)(tBTM_BLE_TRACK_ADV_DATA *p_track_adv_data);

typedef UINT8 tBTM_BLE_TRACK_ADV_EVT;

typedef struct
{
    tBTM_BLE_REF_VALUE             ref_value;
    tBTM_BLE_TRACK_ADV_CBACK *p_track_cback;
}tBTM_BLE_ADV_TRACK_CB;

enum
{
    BTM_BLE_TRACK_ADV_ADD,
    BTM_BLE_TRACK_ADV_REMOVE
};

typedef UINT8 tBTM_BLE_TRACK_ADV_ACTION;

#define BTM_BLE_MULTI_ADV_INVALID   0

#define BTM_BLE_BATCH_SCAN_ENABLE_EVT     1
#define BTM_BLE_BATCH_SCAN_CFG_STRG_EVT   2
#define BTM_BLE_BATCH_SCAN_READ_REPTS_EVT 3
#define BTM_BLE_BATCH_SCAN_THR_EVT        4
#define BTM_BLE_BATCH_SCAN_PARAM_EVT      5
#define BTM_BLE_BATCH_SCAN_DISABLE_EVT    6

typedef UINT8 tBTM_BLE_BATCH_SCAN_EVT;

typedef UINT32 tBTM_BLE_TX_TIME_MS;
typedef UINT32 tBTM_BLE_RX_TIME_MS;
typedef UINT32 tBTM_BLE_IDLE_TIME_MS;
typedef UINT32 tBTM_BLE_ENERGY_USED;

typedef void (tBTM_BLE_ENERGY_INFO_CBACK)(tBTM_BLE_TX_TIME_MS tx_time, tBTM_BLE_RX_TIME_MS rx_time,
                                          tBTM_BLE_IDLE_TIME_MS idle_time,
                                          tBTM_BLE_ENERGY_USED  energy_used,
                                          tBTM_STATUS status);
typedef void (tBTM_BLE_PHY_INFO_CBACK)( UINT16 usConnId,
                                         UINT8 ucTxPhy,
                                         UINT8 ucRxPhy,
                                         tBTM_STATUS xStatus );

typedef struct
{
    tBTM_BLE_ENERGY_INFO_CBACK *p_ener_cback;
}tBTM_BLE_ENERGY_INFO_CB;

typedef void (tBTM_BLE_CTRL_FEATURES_CBACK)(tBTM_STATUS status);

typedef void (tBTM_BLE_SCAN_REQ_CBACK)(BD_ADDR remote_bda, tBLE_ADDR_TYPE addr_type, UINT8 adv_evt);
typedef void (*tBLE_SCAN_PARAM_SETUP_CBACK)(tGATT_IF client_if, tBTM_STATUS status);

tBTM_BLE_SCAN_SETUP_CBACK bta_ble_scan_setup_cb;

/*****************************************************************************
**  EXTERNAL FUNCTION DECLARATIONS
*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
/*******************************************************************************
**
** Function         BTM_SecAddBleDevice
**
** Description      Add/modify device.  This function will be normally called
**                  during host startup to restore all required information
**                  for a LE device stored in the NVRAM.
**
** Parameters:      bd_addr          - BD address of the peer
**                  bd_name          - Name of the peer device.  NULL if unknown.
**                  dev_type         - Remote device's device type.
**                  addr_type        - LE device address type.
**
** Returns          TRUE if added OK, else FALSE
**
*******************************************************************************/
BTM_API extern BOOLEAN BTM_SecAddBleDevice (BD_ADDR bd_addr, BD_NAME bd_name,
                                           tBT_DEVICE_TYPE dev_type, tBLE_ADDR_TYPE addr_type, UINT32 auth_mode);

/*******************************************************************************
**
** Function         BTM_SecAddBleKey
**
** Description      Add/modify LE device information.  This function will be
**                  normally called during host startup to restore all required
**                  information stored in the NVRAM.
**
** Parameters:      bd_addr          - BD address of the peer
**                  p_le_key         - LE key values.
**                  key_type         - LE SMP key type.
*
** Returns          TRUE if added OK, else FALSE
**
*******************************************************************************/
BTM_API extern BOOLEAN BTM_SecAddBleKey (BD_ADDR bd_addr, tBTM_LE_KEY_VALUE *p_le_key,
                                         tBTM_LE_KEY_TYPE key_type);

/*******************************************************************************
**
** Function         BTM_BleSetAdvParams
**
** Description      This function is called to set advertising parameters.
**
** Parameters:       None.
**
** Returns          void
**
*******************************************************************************/
BTM_API extern tBTM_STATUS BTM_BleSetAdvParams(UINT16 adv_int_min, UINT16 adv_int_max,
                                tBLE_BD_ADDR *p_dir_bda, tBTM_BLE_ADV_CHNL_MAP chnl_map);

/*******************************************************************************
**
** Function         BTM_BleSetAdvParamsStartAdv
**
** Description      This function is called to set all of the advertising parameters.
**
** Parameters:       None.
**
** Returns          void
**
*******************************************************************************/
BTM_API extern tBTM_STATUS BTM_BleSetAdvParamsStartAdv(UINT16 adv_int_min, UINT16 adv_int_max, UINT8 adv_type,
                                        tBLE_ADDR_TYPE own_bda_type, tBLE_BD_ADDR *p_dir_bda,
                                        tBTM_BLE_ADV_CHNL_MAP chnl_map, tBTM_BLE_AFP afp, tBTM_START_ADV_CMPL_CBACK *adv_cb);

/*******************************************************************************
**
** Function         BTM_BleWriteAdvData
**
** Description      This function is called to write advertising data.
**
** Parameters:       None.
**
** Returns          void
**
*******************************************************************************/
BTM_API extern tBTM_STATUS BTM_BleWriteAdvData(tBTM_BLE_AD_MASK  data_mask,
                                               tBTM_BLE_ADV_DATA *p_data);

/*******************************************************************************
**
** Function         BTM_BleWriteAdvDataRaw
**
** Description      This function is called to write raw advertising data.
**
** Parameters:      p_raw_adv : point to raw advertising data
**                  raw_adv_len : raw advertising data
**
** Returns          BTM_SUCCESS means success.
**
*******************************************************************************/
BTM_API extern tBTM_STATUS BTM_BleWriteAdvDataRaw(UINT8 *p_raw_adv, UINT32 raw_adv_len);

/*******************************************************************************
**
** Function         BTM_BleSetAdvParams
**
** Description      This function is called to set advertising parameters.
**
** Parameters       adv_int_min: minimum advertising interval
**                  adv_int_max: maximum advertising interval
**                  p_dir_bda: connectable direct initiator's LE device address
**                  chnl_map: advertising channel map.
**
** Returns          void
**
*******************************************************************************/
BTM_API extern void BTM_BleReadAdvParams (UINT16 *adv_int_min, UINT16 *adv_int_max,
                            tBLE_BD_ADDR *p_dir_bda, tBTM_BLE_ADV_CHNL_MAP *p_chnl_map);


/*******************************************************************************
**
** Function         BTM_BleSetScanParams
**
** Description      This function is called to set Scan parameters.
**
** Parameters       adv_int_min: minimum advertising interval
**                  adv_int_max: maximum advertising interval
**                  scan_type: scan mode.
**
** Returns          void
**
*******************************************************************************/
BTM_API extern void BTM_BleSetScanParams(UINT16 scan_interval, UINT16 scan_window,
                                         tBTM_BLE_SCAN_MODE scan_type);

/*******************************************************************************
**
** Function         BTM_BleSetScanParamsWithCb
**
** Description      This function is called to set scan parameters.
**
** Parameters       client_if - Client IF
**                  scan_interval - Scan interval
**                  scan_window - Scan window
**                  scan_mode -    Scan mode
**                  scan_setup_status_cback - Scan param setup status callback
**
** Returns          void
**
*******************************************************************************/
BTM_API extern void BTM_BleSetScanParamsWithCb(tGATT_IF client_if, UINT32 scan_interval, UINT32 scan_window,
                          tBLE_SCAN_MODE scan_mode,
                          tBLE_SCAN_PARAM_SETUP_CBACK scan_setup_status_cback);
/*******************************************************************************
**
** Function         BTM_BleSetStorageConfig
**
** Description      This function is called to setup storage configuration and setup callbacks.
**
** Parameters       UINT8 batch_scan_full_max -Batch scan full maximum
                    UINT8 batch_scan_trunc_max - Batch scan truncated value maximum
                    UINT8 batch_scan_notify_threshold - Threshold value
                    tBTM_BLE_SCAN_SETUP_CBACK *p_setup_cback - Setup callback
                    tBTM_BLE_SCAN_THRESHOLD_CBACK *p_thres_cback -Threshold callback
                    void *p_ref - Reference value
**
** Returns          tBTM_STATUS
**
*******************************************************************************/
//extern
tBTM_STATUS BTM_BleSetStorageConfig(UINT8 batch_scan_full_max,
                                    UINT8 batch_scan_trunc_max,
                                    UINT8 batch_scan_notify_threshold,
                                    tBTM_BLE_SCAN_SETUP_CBACK *p_setup_cback,
                                    tBTM_BLE_SCAN_THRESHOLD_CBACK *p_thres_cback,
                                    tBTM_BLE_SCAN_REP_CBACK *p_cback,
                                    tBTM_BLE_REF_VALUE ref_value);

/*******************************************************************************
**
** Function         BTM_BleEnableBatchScan
**
** Description      This function is called to enable batch scan
**
** Parameters       tBTM_BLE_BATCH_SCAN_MODE scan_mode - Batch scan mode
                    UINT32 scan_interval -Scan interval
                    UINT32 scan_window - Scan window value
                    tBLE_ADDR_TYPE addr_type - Address type
                    tBTM_BLE_DISCARD_RULE discard_rule - Data discard rules
**
** Returns          tBTM_STATUS
**
*******************************************************************************/
//extern
tBTM_STATUS BTM_BleEnableBatchScan(tBTM_BLE_BATCH_SCAN_MODE scan_mode,
                                   UINT32 scan_interval, UINT32 scan_window,
                                   tBTM_BLE_DISCARD_RULE discard_rule,
                                   tBLE_ADDR_TYPE addr_type,
                                   tBTM_BLE_REF_VALUE ref_value);

/*******************************************************************************
**
** Function         BTM_BleDisableBatchScan
**
** Description      This function is called to disable batch scanning
**
** Parameters       void
**
** Returns          void
**
*******************************************************************************/
//extern
tBTM_STATUS BTM_BleDisableBatchScan(tBTM_BLE_REF_VALUE ref_value);

/*******************************************************************************
**
** Function         BTM_BleReadScanReports
**
** Description      This function is called to read batch scan reports
**
** Parameters       tBLE_SCAN_MODE scan_mode - Scan mode report to be read out
                    tBTM_BLE_SCAN_REP_CBACK* p_cback - Reports callback
**
** Returns          tBTM_STATUS
**
*******************************************************************************/
//extern
tBTM_STATUS BTM_BleReadScanReports(tBLE_SCAN_MODE scan_mode,
                                   tBTM_BLE_REF_VALUE ref_value);

/*******************************************************************************
**
** Function         BTM_BleTrackAdvertiser
**
** Description      This function is called to read batch scan reports
**
** Parameters       p_track_cback - Tracking callback
**                  ref_value - Reference value
**
** Returns          tBTM_STATUS
**
*******************************************************************************/
//extern
tBTM_STATUS BTM_BleTrackAdvertiser(tBTM_BLE_TRACK_ADV_CBACK *p_track_cback,
                                   tBTM_BLE_REF_VALUE ref_value);

/*******************************************************************************
**
** Function         BTM_BleWriteScanRsp
**
** Description      This function is called to write LE scan response.
**
** Parameters:      p_scan_rsp: scan response.
**
** Returns          status
**
*******************************************************************************/
BTM_API extern tBTM_STATUS BTM_BleWriteScanRsp(tBTM_BLE_AD_MASK data_mask,
                                               tBTM_BLE_ADV_DATA *p_data);

/*******************************************************************************
**
** Function         BTM_BleWriteScanRspRaw
**
** Description      This function is called to write raw scan response data
**
** Parameters:      None.
**
** Returns          void
**
*******************************************************************************/
BTM_API extern tBTM_STATUS BTM_BleWriteScanRspRaw(UINT8 *p_raw_scan_rsp, UINT32 raw_scan_rsp_len);

/*******************************************************************************
**
** Function         BTM_BleObserve
**
** Description      This procedure keep the device listening for advertising
**                  events from a broadcast device.
**
** Parameters       start: start or stop observe.
**
** Returns          void
**
*******************************************************************************/
BTM_API extern tBTM_STATUS BTM_BleObserve(BOOLEAN start, UINT8 duration,
                           tBTM_INQ_RESULTS_CB *p_results_cb, tBTM_CMPL_CB *p_cmpl_cb);


/*******************************************************************************
**
** Function         BTM_GetDeviceIDRoot
**
** Description      This function is called to read the local device identity
**                  root.
**
** Returns          void
**                  the local device ER is copied into er
**
*******************************************************************************/
BTM_API extern void BTM_GetDeviceIDRoot (BT_OCTET16 ir);

/*******************************************************************************
**
** Function         BTM_GetDeviceEncRoot
**
** Description      This function is called to read the local device encryption
**                  root.
**
** Returns          void
**                  the local device ER is copied into er
**
*******************************************************************************/
BTM_API extern void BTM_GetDeviceEncRoot (BT_OCTET16 er);

/*******************************************************************************
**
** Function         BTM_GetDeviceDHK
**
** Description      This function is called to read the local device DHK.
**
** Returns          void
**                  the local device DHK is copied into dhk
**
*******************************************************************************/
BTM_API extern void BTM_GetDeviceDHK (BT_OCTET16 dhk);

/*******************************************************************************
**
** Function         BTM_SecurityGrant
**
** Description      This function is called to grant security process.
**
** Parameters       bd_addr - peer device bd address.
**                  res     - result of the operation BTM_SUCCESS if success.
**                            Otherwise, BTM_REPEATED_ATTEMPTS is too many attempts.
**
** Returns          None
**
*******************************************************************************/
BTM_API extern void BTM_SecurityGrant(BD_ADDR bd_addr, UINT8 res);

/*******************************************************************************
**
** Function         BTM_BlePasskeyReply
**
** Description      This function is called after Security Manager submitted
**                  passkey request to the application.
**
** Parameters:      bd_addr      - Address of the device for which passkey was requested
**                  res          - result of the operation SMP_SUCCESS if success
**                  passkey - numeric value in the range of
**                  BTM_MIN_PASSKEY_VAL(0) - BTM_MAX_PASSKEY_VAL(999999(0xF423F)).
**
*******************************************************************************/
BTM_API extern void BTM_BlePasskeyReply (BD_ADDR bd_addr, UINT8 res, UINT32 passkey);

/*******************************************************************************
**
** Function         BTM_BleConfirmReply
**
** Description      This function is called after Security Manager submitted
**                  numeric comparison request to the application.
**
** Parameters:      bd_addr      - Address of the device with which numeric
**                                 comparison was requested
**                  res          - comparison result BTM_SUCCESS if success
**
*******************************************************************************/
BTM_API extern void BTM_BleConfirmReply (BD_ADDR bd_addr, UINT8 kp_val);

/*******************************************************************************
**
** Function         BTM_BleKeyprNotif
**
** Description      This function passes keypress notificatin value to SMP by
**                  calling SMP function.
**
** Parameters:      bd_addr      - Address of the device to which keypress
**                                 notificatin has to be sent.
**                  kp_val       - keypress notification value to send.
**
*******************************************************************************/
BTM_API extern BOOLEAN BTM_BleKeyprNotif (BD_ADDR bd_addr, UINT8 kp_val);

/*******************************************************************************
**
** Function         BTM_LeOobDataReply
**
** Description      This function is called to provide the OOB data for
**                  SMP in response to BTM_LE_OOB_REQ_EVT
**
** Parameters:      bd_addr     - Address of the peer device
**                  res         - result of the operation SMP_SUCCESS if success
**                  p_data      - simple pairing Randomizer  C.
**
*******************************************************************************/
BTM_API extern void BTM_BleOobDataReply(BD_ADDR bd_addr, UINT8 res, UINT8 len, UINT8 *p_data);


/*******************************************************************************
**
** Function         BTM_BleDataSignature
**
** Description      This function is called to sign the data using AES128 CMAC
**                  algorith.
**
** Parameter        bd_addr: target device the data to be signed for.
**                  p_text: singing data
**                  len: length of the signing data
**                  signature: output parameter where data signature is going to
**                             be stored.
**
** Returns          TRUE if signing sucessul, otherwise FALSE.
**
*******************************************************************************/
BTM_API extern BOOLEAN BTM_BleDataSignature (BD_ADDR bd_addr, UINT8 *p_text, UINT16 len,
                                             BLE_SIGNATURE signature);

/*******************************************************************************
**
** Function         BTM_BleVerifySignature
**
** Description      This function is called to verify the data signature
**
** Parameter        bd_addr: target device the data to be signed for.
**                  p_orig:  original data before signature.
**                  len: length of the signing data
**                  counter: counter used when doing data signing
**                  p_comp: signature to be compared against.

** Returns          TRUE if signature verified correctly; otherwise FALSE.
**
*******************************************************************************/
BTM_API extern BOOLEAN BTM_BleVerifySignature (BD_ADDR bd_addr, UINT8 *p_orig,
                                            UINT16 len, UINT32 counter,
                                            UINT8 *p_comp);

/*******************************************************************************
**
** Function         BTM_ReadConnectionAddr
**
** Description      This function is called to set the local device random address
**                  .
**
** Returns          void
**
*******************************************************************************/
BTM_API extern void BTM_ReadConnectionAddr (BD_ADDR remote_bda, BD_ADDR local_conn_addr,
                                            tBLE_ADDR_TYPE *p_addr_type);



/*******************************************************************************
**
** Function         BTM_ReadRemoteConnectionAddr
**
** Description      This function is read the remote device address currently used
**                  .
**
** Returns          void
**
*******************************************************************************/
BTM_API extern BOOLEAN BTM_ReadRemoteConnectionAddr(BD_ADDR pseudo_addr,
                                                    BD_ADDR conn_addr,
                                                    tBLE_ADDR_TYPE *p_addr_type);

/*******************************************************************************
**
** Function         BTM_BleLoadLocalKeys
**
** Description      Local local identity key, encryption root or sign counter.
**
** Parameters:      key_type: type of key, can be BTM_BLE_KEY_TYPE_ID, BTM_BLE_KEY_TYPE_ER
**                            or BTM_BLE_KEY_TYPE_COUNTER.
**                  p_key: pointer to the key.
*
** Returns          non2.
**
*******************************************************************************/
BTM_API extern void BTM_BleLoadLocalKeys(UINT8 key_type, tBTM_BLE_LOCAL_KEYS *p_key);

/*******************************************************************************
**
** Function         BTM_BleLocalPrivacyEnabled
**
** Description        Checks if local device supports private address
**
** Returns          Return TRUE if local privacy is enabled else FALSE
**
*******************************************************************************/
BTM_API extern BOOLEAN BTM_BleLocalPrivacyEnabled(void);

/*******************************************************************************
**
** Function         BTM_BleSetBgConnType
**
** Description      This function is called to set BLE background connection
**                  procedure type. It can be auto connection, or selective connection.
**
** Parameters       conn_type: it can be auto connection, or selective connection.
**                  p_select_cback: callback function when selective connection procedure
**                              is being used.
**
** Returns          void
**
*******************************************************************************/
BTM_API extern BOOLEAN BTM_BleSetBgConnType(tBTM_BLE_CONN_TYPE   conn_type,
                                    tBTM_BLE_SEL_CBACK   *p_select_cback);

/*******************************************************************************
**
** Function         BTM_BleUpdateBgConnDev
**
** Description      This function is called to add or remove a device into/from
**                  background connection procedure. The background connection
*                   procedure is decided by the background connection type, it can be
*                   auto connection, or selective connection.
**
** Parameters       add_remove: TRUE to add; FALSE to remove.
**                  remote_bda: device address to add/remove.
**
** Returns          void
**
*******************************************************************************/
BTM_API extern BOOLEAN BTM_BleUpdateBgConnDev(BOOLEAN add_remove, BD_ADDR   remote_bda);


/********************************************************
**
** Function         BTM_BleSetPrefConnParams
**
** Description      Set a peripheral's preferred connection parameters. When
**                  any of the value does not want to be updated while others
**                  do, use BTM_BLE_CONN_PARAM_UNDEF for the ones want to
**                  leave untouched.
**
** Parameters:      bd_addr          - BD address of the peripheral
**                  min_conn_int     - minimum preferred connection interval
**                  max_conn_int     - maximum preferred connection interval
**                  slave_latency    - preferred slave latency
**                  supervision_tout - preferred supervision timeout
**
** Returns          void
**
*******************************************************************************/
BTM_API extern  void BTM_BleSetPrefConnParams (BD_ADDR bd_addr,
                                               UINT16 min_conn_int,  UINT16 max_conn_int,
                                               UINT16 slave_latency, UINT16 supervision_tout);

/******************************************************************************
**
** Function         BTM_BleSetConnScanParams
**
** Description      Set scan parameters used in BLE connection request
**
** Parameters:      scan_interval    - scan interval
**                  scan_window      - scan window
**
** Returns          void
**
*******************************************************************************/
BTM_API extern  void BTM_BleSetConnScanParams (UINT16 scan_interval, UINT16 scan_window);

/*******************************************************************************
**
** Function         BTM_CheckAdvData
**
** Description      This function is called to get ADV data for a specific type.
**
** Parameters       p_adv - pointer of ADV data
**                  type   - finding ADV data type
**                  p_length - return the length of ADV data not including type
**
** Returns          pointer of ADV data
**
*******************************************************************************/
BTM_API extern  UINT8 *BTM_CheckAdvData( UINT8 *p_adv, UINT8 type, UINT8 *p_length);

/*******************************************************************************
**
** Function         BTM_ReadDevInfo
**
** Description      This function is called to read the device/address type
**                  of BD address.
**
** Parameter        remote_bda: remote device address
**                  p_dev_type: output parameter to read the device type.
**                  p_addr_type: output parameter to read the address type.
**
*******************************************************************************/
BTM_API extern void BTM_ReadDevInfo (BD_ADDR remote_bda, tBT_DEVICE_TYPE *p_dev_type,
                                     tBLE_ADDR_TYPE *p_addr_type);

/*******************************************************************************
**
** Function         BTM_BleBroadcast
**
** Description      This function is to start or stop broadcasting.
**
** Parameters       start: start or stop broadcasting.
**
** Returns          status.
**
*******************************************************************************/
BTM_API extern tBTM_STATUS BTM_BleBroadcast(BOOLEAN start, tBTM_START_STOP_ADV_CMPL_CBACK  *p_stop_adv_cback);

/*******************************************************************************
**
** Function         BTM_BleConfigPrivacy
**
** Description      This function is called to enable or disable the privacy in
**                  the local device.
**
** Parameters       enable: TRUE to enable it; FALSE to disable it.
**
** Returns          BTM_SUCCESS privacy mode set success; otherwise failed.
**
*******************************************************************************/
BTM_API extern tBTM_STATUS BTM_BleConfigPrivacy(BOOLEAN enable, tBTM_SET_LOCAL_PRIVACY_CBACK *set_local_privacy_cback);

/*******************************************************************************
**
** Function         BTM_BleEnableMixedPrivacyMode
**
** Description      This function is called to enabled Mixed mode if privacy 1.2
**                  is applicable in controller.
**
** Parameters       mixed_on:  mixed mode to be used or not.
**
** Returns          void
**
*******************************************************************************/
BTM_API extern void BTM_BleEnableMixedPrivacyMode(BOOLEAN mixed_on);

/*******************************************************************************
**
** Function         BTM_BleSetConnMode
**
** Description      This function is called to set BLE connectable mode for a
**                  peripheral device.
**
** Parameters       conn_mode:  directed connectable mode, or non-directed.It can
**                              be BTM_BLE_CONNECT_EVT, BTM_BLE_CONNECT_DIR_EVT or
**                              BTM_BLE_CONNECT_LO_DUTY_DIR_EVT
**
** Returns          BTM_ILLEGAL_VALUE if controller does not support BLE.
**                  BTM_SUCCESS is status set successfully; otherwise failure.
**
*******************************************************************************/
BTM_API extern tBTM_STATUS BTM_BleSetConnMode(UINT8 conn_mode);



/*******************************************************************************
**
** Function         BTM_BleUpdateAdvWhitelist
**
** Description      Add or remove device from advertising white list
**
** Returns          void
**
*******************************************************************************/
BTM_API extern BOOLEAN BTM_BleUpdateAdvWhitelist(BOOLEAN add_remove, BD_ADDR emote_bda);

/*******************************************************************************
**
** Function         BTM_BleUpdateAdvFilterPolicy
**
** Description      This function update the filter policy of advertiser.
**
** Parameter        adv_policy: advertising filter policy
**
** Return           void
*******************************************************************************/
BTM_API extern BOOLEAN BTM_BleUpdateAdvFilterPolicy(tBTM_BLE_AFP adv_policy);

/*******************************************************************************
**
** Function         BTM_UseLeLink
**
** Description      This function is to select the underneath physical link to use.
**
** Returns          TRUE to use LE, FALSE use BR/EDR.
**
*******************************************************************************/
BTM_API extern BOOLEAN BTM_UseLeLink (BD_ADDR bd_addr);

/*******************************************************************************
**
** Function         BTM_BleStackEnable
**
** Description      Enable/Disable BLE functionality on stack regarless controller
**                  capability.
**
** Parameters:      enable: TRUE to enable, FALSE to disable.
**
** Returns          TRUE if added OK, else FALSE
**
*******************************************************************************/
BTM_API extern tBTM_STATUS BTM_BleStackEnable (BOOLEAN enable);

/*******************************************************************************
**
** Function         BTM_GetLeSecurityState
**
** Description      This function is called to get security mode 1 flags and
**                  encryption key size for LE peer.
**
** Returns          BOOLEAN TRUE if LE device is found, FALSE otherwise.
**
*******************************************************************************/
BTM_API extern BOOLEAN BTM_GetLeSecurityState (BD_ADDR bd_addr,
                                               UINT8 *p_le_dev_sec_flags,
                                               UINT8 *p_le_key_size);

/*******************************************************************************
**
** Function         BTM_BleSecurityProcIsRunning
**
** Description      This function indicates if LE security procedure is
**                  currently running with the peer.
**
** Returns          BOOLEAN TRUE if security procedure is running, FALSE otherwise.
**
*******************************************************************************/
BTM_API extern BOOLEAN BTM_BleSecurityProcIsRunning (BD_ADDR bd_addr);

/*******************************************************************************
**
** Function         BTM_BleGetSupportedKeySize
**
** Description      This function gets the maximum encryption key size in bytes
**                  the local device can suport.
**                  record.
**
** Returns          the key size or 0 if the size can't be retrieved.
**
*******************************************************************************/
BTM_API extern UINT8 BTM_BleGetSupportedKeySize (BD_ADDR bd_addr);

/*******************************************************************************
**
** Function         BTM_SetDataChannelPDULength
**
** Description      This function is called to set maximum LL transmission packet size
**
** Returns          BTM_SUCCESS if success; otherwise failed.
**
*******************************************************************************/
BTM_API extern tBTM_STATUS BTM_SetDataChannelPDULength(BD_ADDR bd_addr, UINT16 tx_pdu_length);

/*******************************************************************************
**
** Function         BTM_BleUpdateLtkSize
**
** Description      This function updates LTK size
**
** Returns          void
**
*******************************************************************************/
BTM_API extern void BTM_BleUpdateLtkSize(BD_ADDR bd_addr, UINT8 enc_key_size);

/*******************************************************************************
**
** Function         BTM_BleReadSecKeySize
**
** Description      This function depending on transport provides LK or LTK size.
**
** Returns          LK or LTK size
**
*******************************************************************************/
BTM_API extern UINT8 BTM_BleReadSecKeySize(BD_ADDR bd_addr, tBT_TRANSPORT transport);

/*******************************************************************************
**
** Function         BTM_BleGetDeviceLTK
**
** Description      This function returns peer LTK value in ltk.
**
** Returns          BTM_SUCCESS if LTK is successfully retrieved,
**                  BTM_UNKNOWN_ADDR otherwise.
**
*******************************************************************************/
BTM_API extern tBTM_STATUS BTM_BleGetDeviceLTK(BD_ADDR bd_addr, UINT8 *ltk);

/*******************************************************************************
**
** Function         BTM_BleReadLeLocalFeatures
**
** Description      This function is called to read the LE local features
**
** Returns          pointer to the LE local features string
**
*******************************************************************************/
BTM_API extern UINT8 *BTM_BleReadLeLocalFeatures (void);

BT_API extern UINT8 * btm_ble_get_remote_feature(BD_ADDR remote_bda);

/*******************************************************************************
**
** Function         BTM_BleGetVendorCapabilities
**
** Description      This function reads local LE features
**
** Parameters       p_cmn_vsc_cb : Locala LE capability structure
**
** Returns          void
**
*******************************************************************************/
extern void BTM_BleGetVendorCapabilities(tBTM_BLE_VSC_CB *p_cmn_vsc_cb);
/*******************************************************************************
**
** Function         BTM_BleAdvFilterParamSetup
**
** Description      This function is called to setup the adv data payload filter
**                  condition.
**
** Parameters       p_target: enabble the filter condition on a target device; if NULL
**                            enable the generic scan condition.
**                  enable: enable or disable the filter condition
**
** Returns          void
**
*******************************************************************************/
//extern
tBTM_STATUS BTM_BleAdvFilterParamSetup(int action,
                                       tBTM_BLE_PF_FILT_INDEX filt_index,
                                       tBTM_BLE_PF_FILT_PARAMS *p_filt_params,
                                       tBLE_BD_ADDR *p_target, tBTM_BLE_PF_PARAM_CBACK *p_cmpl_cback,
                                       tBTM_BLE_REF_VALUE ref_value);

/*******************************************************************************
**
** Function         BTM_BleReceiverTest
**
** Description      This function is called to start the LE Receiver test
**
** Parameter       rx_freq - Frequency Range
**               p_cmd_cmpl_cback - Command Complete callback
**
*******************************************************************************/
extern void BTM_BleReceiverTest(UINT8 rx_freq, tBTM_CMPL_CB *p_cmd_cmpl_cback);


/*******************************************************************************
**
** Function         BTM_BleTransmitterTest
**
** Description      This function is called to start the LE Transmitter test
**
** Parameter       tx_freq - Frequency Range
**                       test_data_len - Length in bytes of payload data in each packet
**                       packet_payload - Pattern to use in the payload
**                       p_cmd_cmpl_cback - Command Complete callback
**
*******************************************************************************/
extern void BTM_BleTransmitterTest(UINT8 tx_freq, UINT8 test_data_len,
                                 UINT8 packet_payload, tBTM_CMPL_CB *p_cmd_cmpl_cback);

/*******************************************************************************
**
** Function         BTM_BleTestEnd
**
** Description      This function is called to stop the in-progress TX or RX test
**
** Parameter       p_cmd_cmpl_cback - Command complete callback
**
*******************************************************************************/
extern void BTM_BleTestEnd(tBTM_CMPL_CB *p_cmd_cmpl_cback);


extern void BTM_BleReadPhy(UINT16 handle, tBTM_BLE_PHY_INFO_CBACK *p_cmd_cmpl_cback);


extern void BTM_BleSetPhy(UINT16 handle, UINT8 tx_phy, UINT8 rx_phy, UINT16 phy_options, tBTM_BLE_PHY_INFO_CBACK *p_set_phy_cback);

tBTM_STATUS BTM_BleEnableDisableFilterFeature(UINT8 enable,
        tBTM_BLE_PF_STATUS_CBACK *p_stat_cback,
        tBTM_BLE_REF_VALUE ref_value);
/*******************************************************************************
**
** Function         BTM_BleGetEnergyInfo
**
** Description      This function obtains the energy info
**
** Parameters       p_ener_cback - Callback pointer
**
** Returns          status
**
*******************************************************************************/
extern tBTM_STATUS BTM_BleGetEnergyInfo(tBTM_BLE_ENERGY_INFO_CBACK *p_ener_cback);

/*******************************************************************************
**
** Function         BTM_BleCfgFilterCondition
**
** Description      This function is called to configure the adv data payload filter
**                  condition.
**
** Parameters       action: to read/write/clear
**                  cond_type: filter condition type.
**                  p_cond: filter condition paramter
**
** Returns          tBTM_STATUS
**
*******************************************************************************/
BTM_API extern tBTM_STATUS BTM_BleCfgFilterCondition(tBTM_BLE_SCAN_COND_OP action,
                                      tBTM_BLE_PF_COND_TYPE cond_type,
                                      tBTM_BLE_PF_FILT_INDEX filt_index,
                                      tBTM_BLE_PF_COND_PARAM *p_cond,
                                      tBTM_BLE_PF_CFG_CBACK *p_cmpl_cback,
                                      tBTM_BLE_REF_VALUE ref_value);

#ifdef __cplusplus
}
#endif

#endif

