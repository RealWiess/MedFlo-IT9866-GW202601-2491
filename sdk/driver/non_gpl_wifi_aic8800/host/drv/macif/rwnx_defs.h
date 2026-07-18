#ifndef _RWNX_DEFS_H_
#define _RWNX_DEFS_H_

#include "rwnx_cmds.h"
#include "rtos_al.h"
#include "lmac_mac.h"

enum survey_info_flags {
    SURVEY_INFO_NOISE_DBM       = BIT(0),
    SURVEY_INFO_IN_USE          = BIT(1),
    SURVEY_INFO_TIME            = BIT(2),
    SURVEY_INFO_TIME_BUSY       = BIT(3),
    SURVEY_INFO_TIME_EXT_BUSY   = BIT(4),
    SURVEY_INFO_TIME_RX         = BIT(5),
    SURVEY_INFO_TIME_TX         = BIT(6),
    SURVEY_INFO_TIME_SCAN       = BIT(7),
};


/* Structure containing channel survey information received from MAC */
struct rwnx_survey_info {
    // Filled
    u32 filled;
    // Amount of time in ms the radio spent on the channel
    u32 chan_time_ms;
    // Amount of time the primary channel was sensed busy
    u32 chan_time_busy_ms;
    // Noise in dbm
    s8 noise_dbm;
};

struct rwnx_roc_elem {
    //struct wireless_dev *wdev;
    struct ieee80211_channel *chan;
    unsigned int duration;
    /* Used to avoid call of CFG80211 callback upon expiration of RoC */
    bool mgmt_roc;
    /* Indicate if we have switch on the RoC channel */
    bool on_chan;
};

//#define ipc_host_env_tag ipc_fhost_host_env_tag;
/**
 * struct rwnx_hw - Main driver data
 *
 * @dev: device structure
 * @plat: Platform info
 * @task: Tasklet used to IRQ bottom half
 * @class: Class used to create device
 * @major: Major number for the driver
 * @term: Term device structure
 * @trace: Trace device structure
 * @cb_lock: Lock used to run message callback
 * @ipc_env: Pointer to IPC shared struture
 * @e2amsgs_pool: Pool of DMA buffer for messages push by FW
 * @dbgmsgs_pool: Pool of DMA buffer for dbg messages push by FW
 * @dbgdump_elem: DMA buffer for FW to upload debug dump (TODO)
 * @cmd_mgr: Command (aka fw message) structure
 * @mod_params: Module parameters used to initialize configuration buffer
 * @config: Configuration buffer passed to FW before start
 * @tv: Time at which we started the firmware
 * @debugfs: debugfs entries configuration
 */

struct rwnx_hw {
    int mode;
    int net_id;
    struct rwnx_cmd_mgr cmd_mgr;
    //struct rwnx_mod_params *mod_params;
    //uint32_t config[IPC_CFG_SIZE];
    struct mm_version_cfm version_cfm;          /* Lower layers versions - obtained via MM_VERSION_REQ */
    struct rwnx_survey_info survey[SCAN_CHANNEL_MAX];

    rtos_semaphore cli_cmd_sema;
    rtos_task_handle cli_cmd_task_hdl;
    u16 chipid;
    u8 fw_patch;    // fw: 0    patch: 1
    struct rwnx_roc_elem *roc_elem;	/* Information provided by cfg80211 in its remain on channel request */
    u32 roc_cookie_cnt;	/* Counter used to identify RoC request sent by cfg80211 */
};

struct rwnx_term_stream
{
    //struct list_head list;
    //struct rwnx_term *term;
    //char buf[2 * TERM_BUFFER_SIZE];
    char *read, *write;
    int avail, last_idx;
};

int rwnx_ipc_init(struct rwnx_hw *rwnx_hw, void *shared_ram);
void rwnx_ipc_start(struct rwnx_hw *rwnx_hw);
void rwnx_ipc_msg_push(struct rwnx_hw *rwnx_hw, void *msg_buf, uint16_t len);
int rwnx_host_send_msg(struct rwnx_hw *rwnx_hw, const void *msg_params,
                         int reqcfm, lmac_msg_id_t reqid, void *cfm);
int rwnx_ipc_txdesc_push(struct rwnx_hw *rwnx_hw, void *tx_desc,
                          void *hostid, int hw_queue);
int rwnx_ipc_rxbuf_elem_alloc(struct rwnx_hw *rwnx_hw);
void wifi_patch_prepare(void);

#endif
