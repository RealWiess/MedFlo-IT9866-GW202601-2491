/*****************************************************************************
**
**  Name:           bta_pbc_int.h
**
**  Description:    This is the private file for the phone book access
**                  client (PBC).
**
**  Copyright (c) 2003-2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_PBC_INT_H
#define BTA_PBC_INT_H

#include "bt_target.h"
#include "bta_sys.h"
#include "obx_api.h"
#include "bta_pbc_api.h"
#include "bta_fs_co.h"
#include "bta_fs_ci.h"

/*****************************************************************************
**  Constants and data types
*****************************************************************************/

#define BTA_PBC_PB_ACCESS_TARGET_UUID       "\x79\x61\x35\xF0\xF0\xC5\x11\xD8\x09\x66\x08\x00\x20\x0C\x9A\x66"
#define BTA_PBC_UUID_LENGTH                 16
#define BTA_PBC_MAX_AUTH_KEY_SIZE           16  /* Must not be greater than OBX_MAX_AUTH_KEY_SIZE */

#define BTA_PBC_FOLDER_LISTING_TYPE         "x-obex/folder-listing"
#define BTA_PBC_PULL_PB_TYPE                "x-bt/phonebook"
#define BTA_PBC_PULL_VCARD_LISTING_TYPE     "x-bt/vcard-listing"
#define BTA_PBC_PULL_VCARD_ENTRY_TYPE       "x-bt/vcard"
#define BTA_PBC_PULL_PB_SPD_NAME            "telecom/spd.vcf"
#define BTA_PBC_PULL_PB_FAV_NAME            "telecom/fav.vcf"
#define BTA_PBC_PULL_LIST_SPD_NAME          "spd"
#define BTA_PBC_PULL_LIST_FAV_NAME          "fav"

/* Profile supported repositories */
#define BTA_PBC_REPOSIT_LOCAL      0x01    /* Local PhoneBook */
#define BTA_PBC_REPOSIT_SIM        0x02    /* SIM card PhoneBook */
#define BTA_PBC_REPOSIT_SPEED_DIAL 0x04    /* Speed Dial */
#define BTA_PBC_REPOSIT_FAVORITES  0x08    /* Favorites */

typedef UINT8 tBTA_PBC_SUP_REPOSIT_MASK;

#define BTA_PBC_VERSION_1_1         0x0101 /* PBAP 1.1 */
#define BTA_PBC_VERSION_1_2         0x0102 /* PBAP 1.2 */

#define BTA_PBC_REPOSITORIES_1_1    (BTA_PBC_REPOSIT_LOCAL | BTA_PBC_REPOSIT_SIM)
#define BTA_PBC_REPOSITORIES_1_2    (BTA_PBC_REPOSIT_LOCAL | BTA_PBC_REPOSIT_SIM | BTA_PBC_REPOSIT_SPEED_DIAL | BTA_PBC_REPOSIT_FAVORITES)

#if (defined(BTA_PBAP_1_2_SUPPORTED) && BTA_PBAP_1_2_SUPPORTED == TRUE)
#define BTA_PBC_DEFAULT_VERSION         BTA_PBC_VERSION_1_2
#else
#define BTA_PBC_DEFAULT_VERSION         BTA_PBC_VERSION_1_1
#endif

#define BTA_PBC_DEFAULT_REPOSITORIES        BTA_PBC_REPOSITORIES_1_1
#define BTA_PBC_DEFAULT_SUPPORTED_FEATURES  0x00000003  /* Default peer supported features */

/* PBC Active ftp obex operation (Valid in connected state) */
#define PBC_OP_NONE         0
#define PBC_OP_GET_FILE     1
#define PBC_OP_GET_LIST     2
#define PBC_OP_CHDIR        3

enum
{
    BTA_PBC_GET_CARD,   /* PBAP PullvCardEntry */
    BTA_PBC_GET_PB      /* PBAP PullPhoneBook */
};
typedef UINT8 tBTA_PBC_TYPE;

/* Response Timer Operations */
#define PBC_TIMER_OP_STOP   0
#define PBC_TIMER_OP_ABORT  1

/* state machine events */
enum
{
    /* these events are handled by the state machine */
    BTA_PBC_API_DISABLE_EVT = BTA_SYS_EVT_START(BTA_ID_PBC),

    BTA_PBC_API_OPEN_EVT,           /* Open a connection request */
    BTA_PBC_API_CLOSE_EVT,          /* Close an open connection request */
    BTA_PBC_API_GETFILE_EVT,        /* Get File request */
    BTA_PBC_API_LISTDIR_EVT,        /* List Directory request */
    BTA_PBC_API_CHDIR_EVT,          /* Change Directory request */
    BTA_PBC_API_AUTHRSP_EVT,        /* Response to password request */
    BTA_PBC_API_ABORT_EVT,          /* Response to an abort request */
    BTA_PBC_SDP_OK_EVT,             /* Service search was successful */
    BTA_PBC_SDP_FAIL_EVT,           /* Service search failed */
    BTA_PBC_CI_WRITE_EVT,           /* Call-in response to Write request */
    BTA_PBC_CI_OPEN_EVT,            /* Call-in response to File Open request */
    BTA_PBC_OBX_CONN_RSP_EVT,       /* OBX Channel Connect Request */
    BTA_PBC_OBX_ABORT_RSP_EVT,      /* OBX_operation aborted */
    BTA_PBC_OBX_TOUT_EVT,           /* OBX Operation Timeout */
    BTA_PBC_OBX_PASSWORD_EVT,       /* OBX password requested */
    BTA_PBC_OBX_CLOSE_EVT,          /* OBX Channel Disconnected (Link Lost) */
    BTA_PBC_OBX_GET_RSP_EVT,        /* Read file data or folder listing */
    BTA_PBC_OBX_SETPATH_RSP_EVT,    /* Make or Change Directory */
    BTA_PBC_OBX_CMPL_EVT,           /* operation has completed */
    BTA_PBC_CLOSE_CMPL_EVT,         /* Finish the closing of the channel */
    BTA_PBC_DISABLE_CMPL_EVT,       /* Finished disabling system */
    BTA_PBC_RSP_TOUT_EVT,           /* Timeout waiting for response from server */

    /* these events are handled outside the state machine */
    BTA_PBC_API_ENABLE_EVT
};

typedef UINT16 tBTA_PBC_INT_EVT;

typedef UINT8 tBTA_PBC_STATE;

/* Application Parameters Header
Tag IDs used in the Application Parameters header:
*/
                                            /*  Tag ID          Length      Possible Values */
#define BTA_PBC_APH_ORDER           0x01    /* Order            1 bytes     0x0 to 0x2 */
#define BTA_PBC_APH_SEARCH_VALUE    0x02    /* SearchValue      variable    text */
#define BTA_PBC_APH_SEARCH_ATTR     0x03    /* SearchAttribute  1 byte      0x0 to 0x2 */
#define BTA_PBC_APH_MAX_LIST_COUNT  0x04    /* MaxListCount     2 bytes     0x0000 to 0xFFFF */
#define BTA_PBC_APH_LIST_STOFF      0x05    /* ListStartOffset  2 bytes     0x0000 to 0xFFFF */
#define BTA_PBC_APH_PROP_SELECTOR   0x06    /* Property selector        8 bytes     0x00000000 to 0xFFFFFFFF */
#define BTA_PBC_APH_FORMAT          0x07    /* Format           1 byte      0x00(2.1), 0x01(3.0) */
#define BTA_PBC_APH_PB_SIZE         0x08    /* PhoneBookSize    2 byte      0x0000 to 0xFFFF */
#define BTA_PBC_APH_NEW_MISSED_CALL 0x09    /* NewMissedCall    1 bytes     0x00 to 0xFF */
#define BTA_PBC_APH_PRI_VER_COUNTER 0x0A    /* PrimaryVersionCounter    16 bytes    0x00 to 0xFF */
#define BTA_PBC_APH_SEC_VER_COUNTER 0x0B    /* SecondaryVersionCounter  16 bytes    0x00 to 0xFF */
#define BTA_PBC_APH_VCARD_SELE      0x0C    /* VcardSelector            8 bytes     0x00 to 0xFF */
#define BTA_PBC_APH_DB_ID           0x0D    /* DatabaseIdentifier       16 bytes    0x00 to 0xFF */
#define BTA_PBC_APH_VCARD_SELE_OP   0x0E    /* VcardSelectorOperator    1 bytes     0x00 to 0x01 */
#define BTA_PBC_APH_RESET_NMC       0x0F    /* ResetNewMissedCall       1 bytes     0x01 */
#define BTA_PBC_APH_SUP_FEA         0x10    /* PbapSupportedFeatures    4 bytes     */
#define BTA_PBC_APH_MAX_TAG         BTA_PBC_APH_SUP_FEA

/* Power management state for PBC */
#define BTA_PBC_PM_BUSY     0
#define BTA_PBC_PM_IDLE     1

typedef UINT8 tBTA_PBC_PM_STATE;



/* data type for BTA_PBC_API_ENABLE_EVT */
typedef struct
{
    BT_HDR              hdr;
    tBTA_PBC_CBACK     *p_cback;
    UINT8               app_id;
#if (defined(BTA_PBAP_1_2_SUPPORTED) && BTA_PBAP_1_2_SUPPORTED == TRUE)
    tBTA_PBC_SUP_FEA_MASK   local_features;
#endif
} tBTA_PBC_API_ENABLE;

/* data type for BTA_PBC_API_OPEN_EVT */
typedef struct
{
    BT_HDR              hdr;
    BD_ADDR             bd_addr;
    UINT8               sec_mask;
} tBTA_PBC_API_OPEN;

typedef struct
{
    tBTA_PBC_FILTER_MASK    filter;
    UINT16                  max_list_count;
    UINT16                  list_start_offset;
    tBTA_PBC_FORMAT         format;
#if (defined(BTA_PBAP_1_2_SUPPORTED) && BTA_PBAP_1_2_SUPPORTED == TRUE)
    BOOLEAN                 is_reset_miss_calls;
    tBTA_PBC_FILTER_MASK    selector;
    UINT8                   selector_op;
#endif
} tBTA_PBC_GET_PARAM;

/* data type for BTA_PBC_API_GETFILE_EVT */
typedef struct
{
    BT_HDR              hdr;
    char               *p_rem_name; /* UTF-8 name from listing */
    char               *p_name;
    tBTA_PBC_GET_PARAM *p_param;
    UINT8               obj_type;

} tBTA_PBC_API_GET;

/* data type for BTA_PBC_API_CHDIR_EVT */
typedef struct
{
    BT_HDR              hdr;
    char               *p_dir;    /* UTF-8 name from listing */
    tBTA_PBC_FLAG       flag;
} tBTA_PBC_API_CHDIR;

typedef struct
{
    char                *p_value;
    UINT16              max_list_count;
    UINT16              list_start_offset;
    tBTA_PBC_ORDER      order;
    tBTA_PBC_ATTR       attribute;
#if (defined(BTA_PBAP_1_2_SUPPORTED) && BTA_PBAP_1_2_SUPPORTED == TRUE)
    BOOLEAN             is_reset_miss_calls;
    tBTA_PBC_FILTER_MASK    selector;
    UINT8                   selector_op;
#endif
} tBTA_PBC_LST_PARAM;

/* data type for BTA_PBC_API_LISTDIR_EVT */
typedef struct
{
    BT_HDR              hdr;
    char               *p_dir;    /* UTF-8 name from listing */
    tBTA_PBC_LST_PARAM *p_param;
} tBTA_PBC_API_LIST;


/* data type for BTA_PBC_API_AUTHRSP_EVT */
typedef struct
{
    BT_HDR  hdr;
    UINT8   key [BTA_PBC_MAX_AUTH_KEY_SIZE];      /* The authentication key.*/
    UINT8   key_len;
    UINT8   userid [OBX_MAX_REALM_LEN];      /* The authentication user id.*/
    UINT8   userid_len;
} tBTA_PBC_API_AUTHRSP;

/* data type for BTA_PBC_SDP_OK_EVT */
typedef struct
{
    BT_HDR  hdr;
    UINT16  version;
    UINT16  psm;
    UINT8   scn;
    BOOLEAN                     is_peer_features_present;/* Whether peer feature present */
    tBTA_PBC_SUP_FEA_MASK       peer_features;          /* Peer supported features */
    tBTA_PBC_SUP_REPOSIT_MASK   peer_repositories;      /* Peer supported repositories */

} tBTA_PBC_SDP_OK_EVT;

/* data type for all obex events
    hdr.event contains the PBC event
*/
typedef struct
{
    BT_HDR              hdr;
    tOBX_HANDLE         handle;
    tOBX_EVT_PARAM      param;
    BT_HDR             *p_pkt;
    tOBX_EVENT          obx_event;
    UINT8               rsp_code;
} tBTA_PBC_OBX_EVT;

/* union of all event data types */
typedef union
{
    BT_HDR                  hdr;
    tBTA_PBC_API_ENABLE     api_enable;
    tBTA_PBC_API_OPEN       api_open;
    tBTA_PBC_API_GET        api_get;
    tBTA_PBC_API_CHDIR      api_chdir;
    tBTA_PBC_API_AUTHRSP    auth_rsp;
    tBTA_PBC_API_LIST       api_list;
    tBTA_PBC_SDP_OK_EVT     sdp_ok;
    tBTA_PBC_OBX_EVT        obx_evt;
    tBTA_FS_CI_OPEN_EVT     open_evt;
    tBTA_FS_CI_WRITE_EVT    write_evt;
} tBTA_PBC_DATA;


/* OBX Response Packet Structure - Holds current command/response packet info */
typedef struct
{
    BT_HDR  *p_pkt;             /* (Get/Put) Holds the current OBX header for Put or Get */
    UINT8   *p_start;           /* (Get/Put) Start of the Body of the packet */
    UINT16   offset;            /* (Get/Put) Contains the current offset into the Body (p_start) */
    UINT16   bytes_left;        /* (Get/Put) Holds bytes available left in Obx packet */
    BOOLEAN  final_pkt;         /* (Get)     Holds the final bit of the Put packet */
} tBTA_PBC_OBX_PKT;

/* PBC control block */
typedef struct
{
    tBTA_PBC_CBACK    *p_cback;       /* pointer to application callback function */
    char              *p_name;        /* Holds the local file name */
    tSDP_DISCOVERY_DB *p_db;          /* pointer to discovery database */
    UINT32             sdp_handle;    /* SDP record handle for PCE */
    tBTA_PBC_OBX_PKT   obx;           /* Holds the current OBX packet information */
    TIMER_LIST_ENT     rsp_timer;     /* response timer */
    tBTA_SERVICE_MASK  services;      /* PBAP */
    int                fd;            /* File Descriptor of opened file */
    UINT32             file_size;     /* (Put/Get) length of file */
    UINT16             peer_mtu;
    UINT16             sdp_service;
    BD_ADDR            bd_addr;
    tOBX_HANDLE        obx_handle;
    UINT8              sec_mask;
    tBTA_PBC_STATE     state;         /* state machine state */
    UINT8              obx_oper;      /* current active OBX operation PUT FILE or GET FILE */
    UINT8              timer_oper;    /* current active response timer action (abort or close) */
    UINT8              app_id;
    tBTA_PBC_TYPE      obj_type;      /* type of get op */
    BOOLEAN            first_get_pkt; /* TRUE if retrieving the first packet of GET file */
    BOOLEAN            cout_active;   /* TRUE if call-out is currently active */
    BOOLEAN            disabling;     /* TRUE if client is in process of disabling */
    BOOLEAN            aborting;      /* TRUE if client is in process of aborting */
    BOOLEAN            is_enabled;    /* TRUE if client is enabled */
    BOOLEAN            req_pending;   /* TRUE when waiting for an obex response */
    BOOLEAN            sdp_pending;   /* TRUE when waiting for SDP to complete */
    tBTA_PBC_PM_STATE  pm_state;      /* power management state */
#if (defined(BTA_PBAP_1_2_SUPPORTED) && BTA_PBAP_1_2_SUPPORTED == TRUE)
    tBTA_PBC_SUP_FEA_MASK   local_features; /* Local supported features */
#endif
    tBTA_PBC_SUP_FEA_MASK       peer_features;      /* Peer supported features */
    tBTA_PBC_SUP_REPOSIT_MASK   peer_repositories;  /* Peer supported repositories */
} tBTA_PBC_CB;

/* type for action functions */
typedef void (*tBTA_PBC_ACTION)(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);

/* Configuration structure */
typedef struct
{
    UINT8       realm_charset;          /* Server only */
    BOOLEAN     userid_req;             /* TRUE if user id is required during obex authentication (Server only) */
    char        *pce_name;              /* service name for PBAP PCE SDP record */
    INT32       stopabort_tout;         /* Timeout in milliseconds to wait for abort or close OBEX response (client only) */
} tBTA_PBC_CFG;

/*****************************************************************************
**  Global data
*****************************************************************************/

/* PBC control block */
#if BTA_DYNAMIC_MEMORY == FALSE
extern tBTA_PBC_CB  bta_pbc_cb;
#else
extern tBTA_PBC_CB *bta_pbc_cb_ptr;
#define bta_pbc_cb (*bta_pbc_cb_ptr)
#endif

/* PBC configuration constants */
extern tBTA_PBC_CFG * p_bta_pbc_cfg;

/*****************************************************************************
**  Function prototypes
*****************************************************************************/

extern BOOLEAN  bta_pbc_hdl_event(BT_HDR *p_msg);
extern void     bta_pbc_sm_execute(tBTA_PBC_CB *p_cb, UINT16 event,
                                   tBTA_PBC_DATA *p_data);
extern void     bta_pbc_obx_cback (tOBX_HANDLE handle, tOBX_EVENT event,
                                   UINT8 rsp_code, tOBX_EVT_PARAM param,
                                   BT_HDR *p_pkt);

extern void bta_pbc_init_open(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_init_close(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_init_getfile(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_chdir(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_send_authrsp(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_abort(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_ci_write(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_ci_open(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_obx_conn_rsp(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_obx_abort_rsp(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_obx_password(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_obx_timeout(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_obx_get_rsp(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_obx_setpath_rsp(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_initialize(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_trans_cmpl(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_stop_client(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_start_client(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_free_db(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_ignore_obx(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_find_service(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_close(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_close_complete(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_set_disable(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_rsp_timeout(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_listdir(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void bta_pbc_disable_complete(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);

/* miscellaneous functions */
extern UINT8 bta_pbc_send_get_req(tBTA_PBC_CB *p_cb);
extern void  bta_pbc_proc_get_rsp(tBTA_PBC_CB *p_cb, tBTA_PBC_DATA *p_data);
extern void  bta_pbc_proc_list_data(tBTA_PBC_CB *p_cb, tBTA_PBC_OBX_EVT *p_evt);
extern void  bta_pbc_get_listing(tBTA_PBC_CB *p_cb, char *p_name, tBTA_PBC_LST_PARAM *p_param);
extern void  bta_pbc_listing_err(BT_HDR **p_pkt, tBTA_PBC_STATUS status);

extern tBTA_PBC_STATUS bta_pbc_convert_obx_to_pbc_status(tOBX_STATUS obx_status);

#endif /* BTA_PBC_INT_H */
