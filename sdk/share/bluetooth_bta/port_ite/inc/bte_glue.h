#ifndef BTE_GLUE_H
#define BTE_GLUE_H

#include "bt_target.h"
#include "bte.h"
#include "btu.h"
#include "gki.h"
#include "gki_int.h"
#include "bd.h"
#include "btu_utils.h"

#include "userial.h"
#include "upio.h"
#include "pf_trans.h"
#include "unv.h"
#include "ucodec.h"
//#include "audio_rk.h"

#include "brcm_api.h"
#include "brcm_hcidefs.h"
#include "btm_api.h"
#include "btm_int.h"
#include "bta_api.h"
#include "bta_rt_api.h"
#include "bta_brcm_api.h"
#include "bta_gatt_api.h"
#include "bta_dm_ci.h"
#include "bta_dm_co.h"
#include "bta_dg_api.h"
#include "bta_dg_co.h"
#include "bta_dg_ci.h"
#include "bta_av_api.h"

#include "a2d_api.h"
#include "a2d_sbc.h"
#include "bta_avk_api.h"
#include "bta_avk_co.h"
#include "bta_avk_ci.h"
#include "bta_avk_sbc.h"

#include "bta_gattc_co.h"
#include "bta_gattc_ci.h"

#include "bta_gatts_co.h"

#include "gap_api.h"

#include "bta_hs_api.h"
#include "bta_hs_co.h"

#include "bta_pan_api.h"
#include "bta_pan_ci.h"
#include "bta_pan_co.h"
#include "pan_api.h"

#include "bta_sys.h"
#include "bta_sys_co.h"
#include "bta_sys_ci.h"

#include "bta_media_sco.h"

#include "btapp_int.h"
#include "btapp.h"
#include "btapp_dm.h"
#include "btapp_cli.h"
#include "btapp_mfg.h"
#include "btapp_cfg.h"
#include "btapp_gatts.h"
#include "btapp_int.h"
#include "btapp_nv.h"
#include "btapp_gattc.h"
#include "btapp_gatt_ibeacon.h"
#include "btapp_gattc_profile.h"
#include "btapp_utility.h"
#include "btapp_ble_main.h"
#include "btapp_dg.h"
#include "btapp_pan.h"
#include "btapp_avk.h"
#include "btapp_hs.h"
#include "btapp_avk_codec_int.h"
#include "btapp_codec_asnk.h"

#define UNUSED(x)       (void)(x)
#define ARRAY_COUNT(x)  sizeof(x)/sizeof(x[0])

extern uint8_t   btapp_inited;
#define VERIFY_BT_STACK_ENABLED()    \
do \
{ \
    if(btapp_inited == 0) \
    {\
        BRCM_PLATFORM_TRACE( "ERROR brcm_bt not enable yet\r\n"); \
        return -1; \
    } \
} while(0)

#define BT_LOCAL_NAME               "BTA_iTE"


typedef struct
{
	char          bd_name[64];    /**< Device name */
    BD_ADDR       bd_addr;                         /**< Device address */
    
} bt_device_info_t;


bt_device_info_t bb_device_list[8];

int bb_test_count;
bool g_scan_done;

#endif
