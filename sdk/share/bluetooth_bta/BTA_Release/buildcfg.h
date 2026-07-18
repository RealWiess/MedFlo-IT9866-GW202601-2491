/*
 * $ Copyright Broadcom Corporation $
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "string.h"
//#include <rtthread.h>

#ifndef TRUE
#define TRUE                            1
#endif

#ifndef FALSE
#define FALSE                           0
#endif

#define DEBUG_VERSION                   1 

#define BR_INCLUDED                     TRUE

#define BTM_EIR_CLIENT_INCLUDED         BR_INCLUDED
#define BTM_EIR_SERVER_INCLUDED         BR_INCLUDED
#define BTM_EIR_DEFAULT_FEC_REQUIRED    BR_INCLUDED
#define BTM_SSR_INCLUDED                BR_INCLUDED

#define BTM_INCLUDED                    TRUE
#define BTM_CMD_POOL_ID                 GKI_POOL_ID_2
#define BTM_CLB_INCLUDED                FALSE
#define BTM_CLB_RX_INCLUDED             FALSE
#define BTM_TBFC_INCLUDED               FALSE
#define BTM_SCO_INCLUDED                TRUE
#define BTM_INQ_DB_INCLUDED             FALSE
#define BTM_BUSY_LEVEL_CHANGE_INCLUDED  TRUE
#define BTM_ALLOW_CONN_IF_NONDISCOVER   TRUE
#define BTM_MAX_REM_BD_NAME_LEN         10
#define BTM_DUMO_ADDR_CENTRAL_ENABLED   FALSE
#define BTM_APP_DEV_INIT                bte_post_reset
#define BTM_INTERNAL_LINKKEY_STORAGE_INCLUDED FALSE
#define BTM_BLE_PRIVACY_SPT             TRUE
#define BTM_USE_CONTROLLER_PRIVATE_ADDRESS  FALSE
#define BTM_AUTOMATIC_HCI_RESET         TRUE
#define BTM_NO_SSP_ON_INQUIRY           TRUE
#define BTM_WBS_INCLUDED                TRUE //TRUE

#define BTM_SCO_HCI_INCLUDED            FALSE

#define BTU_INCLUDED                    TRUE
#define BTU_BTA_INCLUDED                TRUE
#define BTUTHIN_INCLUDED                FALSE
#define BTU_BTC_SNK_INCLUDED            FALSE
#define BTU_STACK_LITE_ENABLED          FALSE
#define BTU_DYNAMIC_CB_INCLUDED         FALSE
#define BTU_MUTEX_INCLUDED              TRUE

#define HCIC_INCLUDED                   TRUE
#define HCI_CMD_POOL_ID                 GKI_POOL_ID_1
#define HCI_ACL_POOL_ID                 GKI_POOL_ID_3
#define HCI_SCO_POOL_ID                 GKI_POOL_ID_0
#define HCI_ACL_POOL_BUF_SIZE           GKI_BUF3_SIZE
#define HCI_USE_VARIABLE_SIZE_CMD_BUF   TRUE

#define L2CAP_INCLUDED                  TRUE
#define L2CAP_CMD_POOL_ID               GKI_POOL_ID_2
#define L2CAP_FCR_INCLUDED              FALSE
#define L2CAP_UCD_INCLUDED              FALSE
#define L2CAP_WAKE_PARKED_LINK          FALSE
#define L2CAP_NON_FLUSHABLE_PB_INCLUDED FALSE
#define L2CAP_ROUND_ROBIN_CHANNEL_SERVICE   FALSE
#define L2CAP_MTU_SIZE                  ((UINT16)(HCI_ACL_POOL_BUF_SIZE - BT_HDR_SIZE - 8))
#define L2CAP_LE_COC_INCLUDED           FALSE

#define RFCOMM_INCLUDED                 TRUE
#define RFCOMM_USE_EXTERNAL_SCN         FALSE
#define RFCOMM_CMD_POOL_ID              GKI_POOL_ID_2
#define RFCOMM_DATA_POOL_ID             GKI_POOL_ID_3
#define MAX_RFC_PORTS                   31
#define MAX_BD_CONNECTIONS              4
#define PORT_RX_CRITICAL_WM             ((UINT32)(L2CAP_MTU_SIZE-L2CAP_MIN_OFFSET-RFCOMM_DATA_OVERHEAD)*PORT_RX_BUF_CRITICAL_WM)
#define PORT_RX_LOW_WM                  ((UINT32)(L2CAP_MTU_SIZE-L2CAP_MIN_OFFSET-RFCOMM_DATA_OVERHEAD)*PORT_RX_BUF_LOW_WM)
#define PORT_RX_HIGH_WM                 ((UINT32)(L2CAP_MTU_SIZE-L2CAP_MIN_OFFSET-RFCOMM_DATA_OVERHEAD)*PORT_RX_BUF_HIGH_WM)
#define PORT_RX_BUF_LOW_WM              8
#define PORT_RX_BUF_HIGH_WM             16
#define PORT_RX_BUF_CRITICAL_WM         20
#define PORT_TX_HIGH_WM                 ((UINT32)(L2CAP_MTU_SIZE-L2CAP_MIN_OFFSET-RFCOMM_DATA_OVERHEAD)*PORT_TX_BUF_HIGH_WM)
#define PORT_TX_CRITICAL_WM             ((UINT32)(L2CAP_MTU_SIZE-L2CAP_MIN_OFFSET-RFCOMM_DATA_OVERHEAD)*PORT_TX_BUF_CRITICAL_WM)
#define PORT_TX_BUF_HIGH_WM             3
#define PORT_TX_BUF_CRITICAL_WM         5
#define PORT_CREDIT_RX_LOW              2
#define PORT_CREDIT_RX_MAX              3

/* HID definitions */
#define HID_DEV_INCLUDED                FALSE //TRUE
#define HID_DEV_MAX_DESCRIPTOR_SIZE     200
#define HID_DEV_SET_CONN_MODE           FALSE

/* AVDT/A2DP/AVRC definitions */
#define A2D_INCLUDED                    TRUE
#define A2D_SBC_INCLUDED                TRUE
#define A2D_M12_INCLUDED                FALSE
#define A2D_M24_INCLUDED                FALSE
#define AVDT_INCLUDED                   TRUE
#define AVDT_REPORTING                  FALSE
#define AVDT_MULTIPLEXING               FALSE
#define AVDT_NUM_LINKS                  2
#define AVDT_CMD_POOL_ID                GKI_POOL_ID_1
#define AVDT_DATA_POOL_ID               GKI_POOL_ID_3
#define AVDT_DATA_POOL_SIZE             GKI_BUF3_SIZE

#define AVRC_INCLUDED                   TRUE
#define AVCT_INCLUDED                   TRUE
#define AVCT_NUM_LINKS                  4
#define AVCT_NUM_CONN                   4
//#define AVRC_SEC_MASK                   (p_btm_cfg_settings->security_requirement_mask)
#define AVRC_CONTROL_MTU                (L2CAP_MTU_SIZE)
#define AVRC_BROWSE_MTU                 (L2CAP_MTU_SIZE)

/* PAN definitions */
#define PAN_INCLUDED                    FALSE
#define BNEP_INCLUDED                   FALSE

#define GATT_FIXED_DB                   TRUE
#define GATTS_APPU_USE_GATT_TRACE       TRUE
#define GATT_MAX_APPS                   6
#define GATT_MAX_SR_PROFILES            5
#define GATT_MAX_PHY_CHANNEL            (GATT_CL_MAX_LCB + GATT_MAX_SCCB)
#define GATT_MAX_ATTR_LEN               600
#define GATT_MAX_MTU_SIZE               517
#define GATT_CL_MAX_LCB                 6//14
#define GATT_MAX_SCCB                   4//7
#define GATTP_TRANSPORT_SUPPORTED       GATT_TRANSPORT_LE
#define GATTC_NOTIF_TIMEOUT             3
#define GATT_MAX_BG_CONN_DEV            3
#define GATT_DB_POOL_ID                 GKI_POOL_ID_3
#define GATT_PROFILE_DB_INCLUDED        TRUE
#define GAP_ATTR_DB_INCLUDED            TRUE

#define SIM_ACCESS_INCLUDED             FALSE
#define SAP_SERVER_INCLUDED             FALSE
#define SAP_CLIENT_INCLUDED             FALSE
#define BLE_INCLUDED                    TRUE
#define SMP_INCLUDED                    TRUE
#define GAP_INCLUDED                    TRUE
#define GAP_TRANSPORT_SUPPORTED         BT_TRANSPORT_LE
#define SMP_HOST_ENCRYPT_INCLUDED       FALSE
#define SMP_LE_SC_INCLUDED              TRUE
#define SMP_LE_SC_OOB_INCLUDED          FALSE
#define ATT_DEBUG                       FALSE
#define SMP_DEBUG                       FALSE
#define BLE_BRCM_INCLUDED               TRUE
#define BLE_DATA_LEN_EXT_INCLUDED       TRUE
#define BLE_BRCM_MULTI_ADV_INCLUDED     TRUE
#define BLE_BATCH_SCAN_INCLUDED         TRUE
#define BLE_SCAN_FILTER                 TRUE

#define AMP_INCLUDED                    FALSE
#define GPS_INCLUDED                    FALSE

#define HCISU_H4_INCLUDED               TRUE
#define HCILP_INCLUDED                  FALSE

#define HCILP_SLEEP_MODE                1       //HCILP_SLEEP_MODE_UART
#define HCILP_IDLE_THRESHOLD            0x18
#define HCILP_HC_IDLE_THRESHOLD         0x18    //Set BT controller LPM idle threshold, uint 12.5ms

#define HCILP_BT_WAKE_POLARITY          1
#define HCILP_HOST_WAKE_POLARITY        1
#define HCILP_BT_WAKE_IDLE_TIMEOUT      200

#define H4IBSS_INCLUDED                 FALSE
#define H4IBSS_DEBUG                    FALSE

#define SLIP_INCLUDED                   FALSE
#define SLIP_STATIS_INCLUDED            FALSE
#define SLIP_SW_FLOW_CTRL               FALSE
#define BT_TRACE_SLIP                   FALSE
#define SLIP_SLEEP_TO                   5000
#define SLIP_HOST_SLIDING_WINDOW_SIZE   7

#define BTM_FIRST_RESET_DELAY           0
#define BTM_INQ_DB_SIZE                 30
#define BTM_SEC_MAX_DEVICE_RECORDS      8
#if !defined(BTM_USE_CONTROLLER_PRIVATE_ADDRESS) || (BTM_USE_CONTROLLER_PRIVATE_ADDRESS == FALSE)
#define BTM_SEC_HOST_PRIVACY_ADDR_RESOLUTION_TABLE_SIZE    3
#endif
#define BTM_SEC_MAX_SERVICE_RECORDS     14
#define BTM_SEC_SERVICE_NAME_LEN        0
#define BTM_MAX_LOC_BD_NAME_LEN         248
#define BTM_MAX_PM_RECORDS              1
#define BTM_MAX_VSE_CALLBACKS           3
#define BTM_BLE_MAX_BG_CONN_DEV_NUM     10
#define BTM_OOB_INCLUDED                TRUE
#define BTM_BR_SC_INCLUDED              FALSE //TRUE
#define BTM_CROSS_TRANSP_KEY_DERIVATION FALSE //TRUE
#define BTM_PWR_MGR_INCLUDED            TRUE
#define BT_BRCM_VS_INCLUDED             TRUE
#define BTM_PCM2_INCLUDED               FALSE

#define BTTRC_INCLUDED                  FALSE
#define BTTRC_PARSER_INCLUDED           FALSE
#define MAX_TRACE_RAM_SIZE              10

#define SDP_DEBUG_RAW                   FALSE
#define SDP_INCLIDED                    TRUE
#define SDP_SERVER_ENABLED              TRUE
#define SDP_CLIENT_ENABLED              TRUE
#define SDP_POOL_ID                     GKI_POOL_ID_3
#define SDP_MAX_CONNECTIONS             4
#define SDP_MAX_RECORDS                 20
#define SDP_MAX_REC_ATTR                25
#define SDP_MAX_ATTR_LEN                512
#define SDP_MAX_UUID_FILTERS            3
#define SDP_MAX_ATTR_FILTERS            12
#define SDP_MAX_PROTOCOL_PARAMS         2
#define SDP_RAW_DATA_SERVER             FALSE

#define OBX_INCLUDED                    TRUE

#if (defined(BTU_DYNAMIC_CB_INCLUDED)  && (BTU_DYNAMIC_CB_INCLUDED == TRUE))
#define MAX_L2CAP_CLIENTS               8
#define MAX_L2CAP_LINKS                 6
#define MAX_L2CAP_CHANNELS              16

/* Connection Oriented Channel configuration */
#define MAX_L2CAP_BLE_CLIENTS           4
#define MAX_L2CAP_BLE_CHANNELS          4

#else /* BTU_DYNAMIC_CB_INCLUDED  */
#define MAX_L2CAP_CLIENTS               16
#define MAX_L2CAP_LINKS                 12
#define MAX_L2CAP_CHANNELS              32

/* Connection Oriented Channel configuration */
#define MAX_L2CAP_BLE_CLIENTS           4
#define MAX_L2CAP_BLE_CHANNELS          4

#endif /* BTU_DYNAMIC_CB_INCLUDED */

#define GAP_CONN_INCLUDED               FALSE

#define GKI_NUM_FIXED_BUF_POOLS         5
#define GKI_NUM_TOTAL_BUF_POOLS         5
#define GKI_BUF0_SIZE                   268
#define GKI_BUF0_MAX                    100 //75
#define GKI_BUF1_SIZE                   320
#define GKI_BUF1_MAX                    40 //20
#define GKI_BUF2_SIZE                   600
#define GKI_BUF2_MAX                    60 //40
#define GKI_BUF3_SIZE                   1024
#define GKI_BUF3_MAX                    120 //100
#define GKI_BUF4_SIZE                   (8080+26) //1204
#define GKI_BUF4_MAX                    40 //30 //10
#define GKI_MAX_BUF_SIZE                GKI_BUF4_SIZE
#define GKI_DEF_BUFPOOL_PERM_MASK       0xffe0

#define GKI_DYNAMIC_POOL_CFG            TRUE
#define GKI_DYNAMIC_MEMORY              FALSE
#define GKI_USE_DYNAMIC_BUFFERS         FALSE
#define GKI_TIMER_INTERVAL              10
#define QUICK_TIMER_TICKS_PER_SEC       (1000 / GKI_TIMER_INTERVAL)
#define GKI_TIMER_LIST_NOPREEMPT        FALSE
#define OS_TICKS_PER_SEC                1000

#define GKI_BASE_PRIORITY               4 //10
#define USERIAL_TASK                    1
#define HCISU_TASK                      2
#define AUDIO_PLAY_TASK                 0
#define UCODEC_TASK                     3
#define BTU_TASK                        4
#define BTAPPL_TASK                     5
#define TICKS_TASK                      6
#define BTAPP_CONSOLE_TASK              7
#define GKI_MAX_TASKS                   8

#define BTAPPL_INCLUDED                 TRUE
#define UNV_INCLUDED                    FALSE
#define UCODEC_INCLUDED                 TRUE
#define AUDIO_PLAY_INCLUDED             TRUE
#define BTAPP_CONSOLE_INCLUDED          FALSE

/* Miscellaneous application configuration */
#define _MAX_PATH                       16
#define TICKS_PER_SEC                   1000 //(RT_TICK_PER_SECOND / GKI_TIMER_INTERVAL)
#define GKI_SHUTDOWN_EVT                APPL_EVT_7
#define THREAD_EVT_QUEUE_MSG_SIZE       2
#define THREAD_EVT_QUEUE_NUM_MSG        100

#ifdef  DEBUG_VERSION
#ifndef BT_TRACE_PROTOCOL
#define BT_TRACE_PROTOCOL               FALSE
#endif
#ifndef BT_USE_TRACES
#define BT_USE_TRACES                   TRUE
#endif
#ifndef BT_TRACE_VERBOSE
#define BT_TRACE_VERBOSE                TRUE
#endif
#define BT_TRACE_DISP_HCICMD            FALSE
#define BT_TRACE_DISP_HCIEVT            FALSE

#define APPL_INITIAL_TRACE_LEVEL        BT_TRACE_LEVEL_DEBUG
#define HCI_INITIAL_TRACE_LEVEL         BT_TRACE_LEVEL_DEBUG
#define BTM_INITIAL_TRACE_LEVEL         BT_TRACE_LEVEL_DEBUG
#define L2CAP_INITIAL_TRACE_LEVEL       BT_TRACE_LEVEL_DEBUG
#define RFCOMM_INITIAL_TRACE_LEVEL      BT_TRACE_LEVEL_DEBUG
#define GAP_INITIAL_TRACE_LEVEL         BT_TRACE_LEVEL_DEBUG
#define HSP2_INITIAL_TRACE_LEVEL        BT_TRACE_LEVEL_NONE
#define SPP_INITIAL_TRACE_LEVEL         BT_TRACE_LEVEL_NONE
#define SMP_INITIAL_TRACE_LEVEL         BT_TRACE_LEVEL_DEBUG
#define SDP_INITIAL_TRACE_LEVEL         BT_TRACE_LEVEL_DEBUG
#define GATT_INITIAL_TRACE_LEVEL        BT_TRACE_LEVEL_DEBUG
#define HID_INITIAL_TRACE_LEVEL         BT_TRACE_LEVEL_DEBUG

#else
#ifndef BT_TRACE_PROTOCOL
#define BT_TRACE_PROTOCOL               FALSE
#endif
#ifndef BT_USE_TRACES
#define BT_USE_TRACES                   FALSE                       //When use the trace lib, should set this macro to 1 in the cmakelist
#endif
#define BT_TRACE_VERBOSE                FALSE
#define BT_TRACE_DISP_HCICMD            FALSE
#define BT_TRACE_DISP_HCIEVT            FALSE

#define APPL_INITIAL_TRACE_LEVEL        BT_TRACE_LEVEL_NONE
#define HCI_INITIAL_TRACE_LEVEL         BT_TRACE_LEVEL_NONE
#define BTM_INITIAL_TRACE_LEVEL         BT_TRACE_LEVEL_NONE
#define L2CAP_INITIAL_TRACE_LEVEL       BT_TRACE_LEVEL_NONE
#define RFCOMM_INITIAL_TRACE_LEVEL      BT_TRACE_LEVEL_NONE
#define GAP_INITIAL_TRACE_LEVEL         BT_TRACE_LEVEL_NONE
#define HSP2_INITIAL_TRACE_LEVEL        BT_TRACE_LEVEL_NONE
#define SPP_INITIAL_TRACE_LEVEL         BT_TRACE_LEVEL_NONE
#define SMP_INITIAL_TRACE_LEVEL         BT_TRACE_LEVEL_NONE
#define SDP_INITIAL_TRACE_LEVEL         BT_TRACE_LEVEL_NONE
#define GATT_INITIAL_TRACE_LEVEL        BT_TRACE_LEVEL_NONE
#endif

typedef void (*TASKPTR)(void* parameter);
#define TASKPTR TASKPTR

/*  BTA Configuration Macro Section  */
#define BTA_DM_PAGE_TIMEOUT             8192

#define BTA_AG_INCLUDED                 TRUE
#define BTA_AG_DEBUG                    FALSE

#define BTA_DG_INCLUDED                 TRUE

#define BTA_HS_INCLUDED                 TRUE
#define BTA_HS_DEBUG                    FALSE

#define BTA_AV_INCLUDED                 TRUE
#define BTA_AR_INCLUDED                 FALSE
#define BTA_AV_DEBUG                    FALSE

#define BTA_AVK_INCLUDED                TRUE
#define BTA_AVK_DEBUG                   FALSE

#define BTA_PAN_INCLUDED                FALSE
#define BTA_PBC_INCLUDED                TRUE
//#define BTA_PBAP_1_2_SUPPORTED        TRUE
#define BTA_HD_INCLUDED                 TRUE

#if(BLE_INCLUDED == TRUE)
#define BTA_GATT_INCLUDED               TRUE
#define BTA_PROXIMITY_INCLUDED          FALSE
#define BTA_LINKLOSS_INCLUDED           FALSE
#define BTA_IMMEDIATE_ALERT_INCLUDED    FALSE
#define BTA_TX_POWER_INCLUDED           FALSE
#define BTA_FINEME_INCLUDED             FALSE
#else
#define BTA_GATT_INCLUDED               FALSE
#define BTA_PROXIMITY_INCLUDED          FALSE
#define BTA_LINKLOSS_INCLUDED           FALSE
#define BTA_IMMEDIATE_ALERT_INCLUDED    FALSE
#define BTA_TX_POWER_INCLUDED           FALSE
#define BTA_FINEME_INCLUDED             FALSE
#endif

#define BTA_FMTX_INCLUDED               FALSE
#define BTA_SSR_INCLUDED                FALSE

#define BTA_SET_DEVICE_NAME             TRUE
/*  BTA Configuration Macro Section end */

/* BT APP Configuration Macro Section */
#define BTAPP_BRCM_CS_INCLUDED          TRUE

/* BT APP Configuration Macro Section end */

/* BT UI Configuration Macro Section */
#define BTAPP_AG_INSTANCES               1

/* BT UI Configuration Macro Section end */

#define PORT_SCHEDULE_LOCK
#define PORT_SCHEDULE_UNLOCK

#define BLE_TEST_MODE_ENABLE            1

#define BTAPP_TARGET_PLATFORM_NXP       1

#define SBC_FOR_EMBEDDED_LINUX          1


#define HCI_LOG_FILE           TRUE
#define BRCM_BTSNOOP_DIR       "/bt"
#define BRCM_BTSNOOP_NAME      "btnsoop.log"

#define PCM_CLOCK_RATE  1  //0:128Bbps, 1:256Kbps, 2:512KBps, 3:1024KBps, 4:2048KBps
#define PCM_FRAME_TYPE  0  //0:Short, 1:Long
#define PCM_SYNC_MODE   0  //0:Slave, 1:Master
#define PCM_CLOCK_MODE  0  //0:Slave, 1:Master


#define OTPP_INCLUDED                    TRUE
#define AVRC_ADV_CTRL_INCLUDED FLASE
#define AVCT_BROWSE_INCLUDED   FALSE

#ifdef __cplusplus
} /*extern "C" */
#endif

