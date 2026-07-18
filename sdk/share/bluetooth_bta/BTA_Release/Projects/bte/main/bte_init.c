/*****************************************************************************
**                                                                           *
**  Name:          bte_init.c                                                *
**                                                                           *
**  Description:   This module contains the routines that initialize the     *
**                 stack components.  It must be called before the BTU task  *
**                 is started.                                               *
**                                                                           *
**                 Note: If using dynamic memory, the control bloacks for    *
**                       each component must already be allocated            *
**                                                                           *
**  Copyright (c) 2018-2019, Broadcom Inc.                                   *
**  Broadcom Bluetooth Core. Proprietary and confidential.                   *
******************************************************************************/

#include "bt_target.h"
#include <string.h>

#ifndef BTA_INCLUDED
#define BTA_INCLUDED FALSE
#endif

/* Include initialization functions definitions */
#if (defined(RFCOMM_INCLUDED) && RFCOMM_INCLUDED == TRUE)
#include "port_api.h"
#endif

#if (defined(OBX_INCLUDED) && OBX_INCLUDED == TRUE)
#include "obx_api.h"
#endif

#if (defined(BNEP_INCLUDED) && BNEP_INCLUDED == TRUE)
#include "bnep_api.h"
#endif

#if (defined(GAP_INCLUDED) && GAP_INCLUDED == TRUE)
#include "gap_api.h"
#endif

#if (defined(SPP_INCLUDED) && SPP_INCLUDED == TRUE)
#include "spp_api.h"
#endif

#if (defined(DUN_INCLUDED) && DUN_INCLUDED == TRUE)
#include "dun_api.h"
#endif

#if (defined(GOEP_INCLUDED) &&  GOEP_INCLUDED == TRUE)
#include "goep_util.h"
#endif /* GOEP included */

#if (defined(FTP_INCLUDED) && FTP_INCLUDED == TRUE)
#include "ftp_api.h"
#endif /* FTP */

#if (defined(OPP_INCLUDED) && OPP_INCLUDED == TRUE)
#include "opp_api.h"
#endif /* OPP */

#if (defined(BIP_INCLUDED) && BIP_INCLUDED == TRUE)
#include "bip_api.h"
#endif

#if (defined(BTU_BTA_INCLUDED) && BTU_BTA_INCLUDED == TRUE)
#if (defined(BTA_BI_INCLUDED) && BTA_BI_INCLUDED == TRUE)
#include "bta_bi_api.h"
#endif
#endif

#if (defined(HFP_INCLUDED) && HFP_INCLUDED == TRUE)
#include "hfp_api.h"
#endif

#if ((defined(HSP2_INCLUDED) && HSP2_INCLUDED == TRUE)) || \
    ((defined(HFP_INCLUDED) && HFP_INCLUDED == TRUE))
#include "hsp2_api.h"
#endif

#if (defined(BPP_INCLUDED) && BPP_INCLUDED == TRUE)
#include "bpp_api.h"
#endif

#if (defined(PAN_INCLUDED) && PAN_INCLUDED == TRUE)
#include "pan_api.h"
#endif

#if ((defined(BTU_STACK_LITE_ENABLED) && BTU_STACK_LITE_ENABLED == TRUE) && (defined(AVDT_INCLUDED) && AVDT_INCLUDED == TRUE))
#include "avdt_api.h"
#endif

#if (defined(AVRC_INCLUDED) && AVRC_INCLUDED == TRUE)
#include "avrc_api.h"
#endif

#if (defined(A2D_INCLUDED) && A2D_INCLUDED == TRUE)
#include "a2d_api.h"
#endif

#if (defined(VDP_INCLUDED) && VDP_INCLUDED == TRUE)
#include "vdp_api.h"
#endif

#if (defined(HID_DEV_INCLUDED) && HID_DEV_INCLUDED == TRUE)
#include "hidd_api.h"
#endif

#if (defined(HID_HOST_INCLUDED) && HID_HOST_INCLUDED == TRUE)
#include "hidh_api.h"
#endif

#if (defined(SIM_ACCESS_INCLUDED) && SIM_ACCESS_INCLUDED == TRUE)
#include "sap_api.h"
#endif  /* SIM_ACCESS_INCLUDED */

#if (defined(MCA_INCLUDED) && MCA_INCLUDED == TRUE)
#include "mca_api.h"
#endif

#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
#include "gatt_api.h"
#if (defined(SMP_INCLUDED) && SMP_INCLUDED == TRUE)
#include "smp_api.h"
#endif
#endif

#if (defined BT_BRCM_VS_INCLUDED && BT_BRCM_VS_INCLUDED == TRUE)
/* control block for patch ram downloading */
#include "brcm_api.h"
#endif

/***** BTA Modules ******/
#if BTA_INCLUDED == TRUE && BTA_DYNAMIC_MEMORY == TRUE
#include "bta_api.h"
#include "bta_sys.h"


/* GPS enable/disable functions */
#include "bta_gps_api.h"
#include "bta_gps_int.h"

#if BTA_3DS_INCLUDED == TRUE
#include "bta_3ds_int.h"
#endif

#if BTA_BAV_INCLUDED == TRUE
#include "bta_bav_int.h"
#endif

#if BTA_AC_INCLUDED == TRUE
#include "bta_acs_int.h"
#include "bta_acc_int.h"
#endif

#if BTA_AG_INCLUDED == TRUE
#include "bta_ag_int.h"
#endif

#if BTA_HS_INCLUDED == TRUE
#include "bta_hs_int.h"
#endif

#include "bta_dm_int.h"

#if BTA_DG_INCLUDED == TRUE
#include "bta_dg_api.h"
#include "bta_dg_int.h"
#endif

#if BTA_FT_INCLUDED == TRUE
#include "bta_ftc_int.h"
#include "bta_fts_int.h"
#endif

#if 0
#if BTA_PBC_INCLUDED == TRUE
#include "bta_pbc_int.h"
#endif

#if BTA_PBS_INCLUDED == TRUE
#include "bta_pbs_int.h"
#endif
#endif

#if BTA_OP_INCLUDED == TRUE
#include "bta_opc_int.h"
#include "bta_ops_int.h"
#endif

#if BTA_SS_INCLUDED==TRUE
#include "bta_ss_int.h"
#endif

#if BTA_BI_INCLUDED==TRUE
#include "bta_bic_int.h"
#include "bta_bis_int.h"
#endif

#if BTA_PR_INCLUDED==TRUE
#include "bta_pr_int.h"
#endif

#if BTA_AR_INCLUDED==TRUE
#include "bta_ar_int.h"
#endif
#if BTA_AV_INCLUDED==TRUE
#include "bta_av_int.h"
#endif
#if BTA_AVK_INCLUDED==TRUE
#include "bta_avk_int.h"
#endif

#if BTA_SC_INCLUDED==TRUE
#include "bta_sc_int.h"
#endif

#if BTA_HD_INCLUDED==TRUE
#include "bta_hd_int.h"
#endif

#if BTA_HH_INCLUDED==TRUE
#include "bta_hh_int.h"
#endif

#if BTA_FM_INCLUDED==TRUE
#include "bta_fm_int.h"
#endif

#if BTA_FMTX_INCLUDED==TRUE
#include "bta_fmtx_int.h"
#endif

#if 0
#if BTA_JV_INCLUDED==TRUE
#include "bta_jv_int.h"
#endif
#endif

#if BTA_SSR_INCLUDED==TRUE
#include "bta_ssr_int.h"
#endif

#if BTA_MCE_INCLUDED == TRUE
#include "bta_mce_int.h"
#endif

#if BTA_MSE_INCLUDED == TRUE
#include "bta_mse_int.h"
#endif

#if BTA_HL_INCLUDED == TRUE
#include "bta_hl_int.h"
#endif

#if BTA_GATT_INCLUDED == TRUE
#include "bta_gattc_int.h"
#include "bta_gatts_int.h"
#endif

#if BTA_PAN_INCLUDED==TRUE
#include "bta_pan_int.h"
#endif

#if L2CAP_LE_COC_INCLUDED == TRUE
#include "bta_lecoc_int.h"
#endif

#include "bta_sys_int.h"

#if (defined BT_BRCM_VS_INCLUDED && BT_BRCM_VS_INCLUDED == TRUE)
/* control block for patch ram downloading */
#include "bta_brcm_int.h"
#endif
#endif /* BTA_INCLUDED */

/*****************************************************************************
**                          F U N C T I O N S                                *
******************************************************************************/

/*****************************************************************************
**
** Function         BTE_InitStack
**
** Description      Initialize control block memory for each component.
**
**                  Note: The core stack components must be called
**                      before creating the BTU Task.  The rest of the
**                      components can be initialized at a later time if desired
**                      as long as the component's init function is called
**                      before accessing any of its functions.
**
** Returns          void
**
******************************************************************************/
BT_API void BTE_InitStack(void)
{
#if (defined BT_BRCM_VS_INCLUDED && BT_BRCM_VS_INCLUDED == TRUE)
    BTE_BrcmInit();
#endif
/* Initialize the optional stack components */

/****************************
** RFCOMM and its profiles **
*****************************/
#if (defined(RFCOMM_INCLUDED) && RFCOMM_INCLUDED == TRUE)
    RFCOMM_Init();

#if (defined(SPP_INCLUDED) && SPP_INCLUDED == TRUE)
    SPP_Init();
#endif  /* SPP */

#if (defined(DUN_INCLUDED) && DUN_INCLUDED == TRUE)
    DUN_Init();
#endif  /* DUN */

#if (defined(HSP2_INCLUDED) && HSP2_INCLUDED == TRUE)
    HSP2_Init();
#endif  /* HSP2 */

#if (defined(HFP_INCLUDED) && HFP_INCLUDED == TRUE)
    HFP_Init();
#endif  /* HFP */

/**************************
** OBEX and its profiles **
***************************/
#if (defined(OBX_INCLUDED) && OBX_INCLUDED == TRUE)
    OBX_Init();
#if (defined(BIP_INCLUDED) && BIP_INCLUDED == TRUE)
    BIP_Init();
#if (defined(BTU_BTA_INCLUDED) && BTU_BTA_INCLUDED == TRUE)
#if (defined(BTA_BI_INCLUDED) && BTA_BI_INCLUDED == TRUE)
    BTA_BicInit();
#endif  /* BTA BI */
#endif
#endif  /* BIP */

#if (defined(GOEP_INCLUDED) && GOEP_INCLUDED == TRUE)
    GOEP_Init();
#endif /* GOEP */


#if (defined(FTP_INCLUDED) && FTP_INCLUDED == TRUE)
    FTP_Init();
#endif
#if (defined(OPP_INCLUDED) && OPP_INCLUDED == TRUE)
    OPP_Init();
#endif

#if (defined(BPP_INCLUDED) && BPP_INCLUDED == TRUE)
    BPP_Init();
#endif  /* BPP */
#endif  /* OBX */


#endif  /* RFCOMM Included */

/**************************
** BNEP and its profiles **
***************************/
#if (defined(BNEP_INCLUDED) && BNEP_INCLUDED == TRUE)
    BNEP_Init();

#if (defined(PAN_INCLUDED) && PAN_INCLUDED == TRUE)
    PAN_Init();
#endif  /* PAN */
#endif  /* BNEP Included */


/**************************
** AVDT and its profiles **
***************************/
#if ((defined(BTU_STACK_LITE_ENABLED) && BTU_STACK_LITE_ENABLED == TRUE) && (defined(AVDT_INCLUDED) && AVDT_INCLUDED == TRUE))
    AVDT_Init();
#endif  /* AVDT Included */

#if (defined(A2D_INCLUDED) && A2D_INCLUDED == TRUE)
    A2D_Init();
#endif  /* AADP */

#if (defined(VDP_INCLUDED) && VDP_INCLUDED == TRUE)
    VDP_Init();
#endif

#if (defined(AVRC_INCLUDED) && AVRC_INCLUDED == TRUE)
    AVRC_Init();
#endif


/***********
** Others **
************/
#if (defined(GAP_INCLUDED) && GAP_INCLUDED == TRUE)
    GAP_Init();
#endif  /* GAP Included */

#if (defined(SIM_ACCESS_INCLUDED) && SIM_ACCESS_INCLUDED == TRUE)
    SAP_Init();
#endif /* SIM_ACCESS_INCLUDED */

#if (defined(HID_DEV_INCLUDED) && HID_DEV_INCLUDED == TRUE)
    HID_DevInit();
#endif
#if (defined(HID_HOST_INCLUDED) && HID_HOST_INCLUDED == TRUE)
    HID_HostInit();
#endif

#if (defined(MCA_INCLUDED) && MCA_INCLUDED == TRUE)
    MCA_Init();
#endif  /* MCA_INCLUDED */

/****************
** BTA Modules **
*****************/
#if (BTA_INCLUDED == TRUE && BTA_DYNAMIC_MEMORY == TRUE)
    memset((void*)bta_sys_cb_ptr, 0, sizeof(tBTA_SYS_CB));
    memset((void*)bta_dm_cb_ptr, 0, sizeof(tBTA_DM_CB));
    memset((void*)bta_dm_search_cb_ptr, 0, sizeof(tBTA_DM_SEARCH_CB));
    memset((void*)bta_dm_di_cb_ptr, 0, sizeof(tBTA_DM_DI_CB));
#if (defined BT_BRCM_VS_INCLUDED && BT_BRCM_VS_INCLUDED == TRUE)
    memset((void*)bta_prm_cb_ptr, 0, sizeof(tBTA_PRM_CB));
#endif
    memset((void*)bta_gps_cb_ptr, 0, sizeof(tBTA_GPS_CB));

#if BTA_3DS_INCLUDED == TRUE
    memset((void*)bta_3ds_cb_ptr, 0, sizeof(tBTA_3DS_CB));
#endif
#if BTA_BAV_INCLUDED == TRUE
    memset((void*)bta_bav_cb_ptr, 0, sizeof(tBTA_BAV_CB));
#endif
#if BTA_AC_INCLUDED == TRUE
    memset((void*)bta_acc_cb_ptr, 0, sizeof(tBTA_ACC_CB));
    memset((void*)bta_acs_cb_ptr, 0, sizeof(tBTA_ACS_CB));
#endif
#if BTA_AG_INCLUDED == TRUE
    memset((void*)bta_ag_cb_ptr, 0, sizeof(tBTA_AG_CB));
#endif
#if BTA_HS_INCLUDED == TRUE
    memset((void*)bta_hs_cb_ptr, 0, sizeof(tBTA_HS_CB));
#endif
#if BTA_DG_INCLUDED == TRUE
    memset((void*)bta_dg_cb_ptr, 0, sizeof(tBTA_DG_CB));
#endif
#if BTA_FT_INCLUDED==TRUE
    memset((void*)bta_ftc_cb_ptr, 0, sizeof(tBTA_FTC_CB));
    memset((void*)bta_fts_cb_ptr, 0, sizeof(tBTA_FTS_CB));
#endif
#if 0
#if BTA_PBC_INCLUDED==TRUE
    memset((void*)bta_pbc_cb_ptr, 0, sizeof(tBTA_PBC_CB));
#endif
#if BTA_PBS_INCLUDED==TRUE
    memset((void*)bta_pbs_cb_ptr, 0, sizeof(tBTA_PBS_CB));
#endif
#endif
#if BTA_OP_INCLUDED==TRUE
    memset((void*)bta_opc_cb_ptr, 0, sizeof(tBTA_OPC_CB));
    memset((void*)bta_ops_cb_ptr, 0, sizeof(tBTA_OPS_CB));
#endif
#if BTA_SS_INCLUDED==TRUE
    memset((void*)bta_ss_cb_ptr, 0, sizeof(tBTA_SS_CB));
#endif
#if BTA_BI_INCLUDED==TRUE
    memset((void *)bta_bic_cb_ptr, 0, sizeof(tBTA_BIC_CB));
    memset((void *)bta_bis_cb_ptr, 0, sizeof(tBTA_BIS_CB));
#endif
#if BTA_AR_INCLUDED==TRUE
    memset((void *)bta_ar_cb_ptr, 0, sizeof(tBTA_AR_CB));
#endif
#if BTA_AV_INCLUDED==TRUE
    memset((void *)bta_av_cb_ptr, 0, sizeof(tBTA_AV_CB));
#endif
#if BTA_AVK_INCLUDED==TRUE
    memset((void *)bta_avk_cb_ptr, 0, sizeof(tBTA_AVK_CB));
#endif
#if BTA_PR_INCLUDED==TRUE
    memset((void *)bta_pr_cb_ptr, 0, sizeof(tBTA_PR_CB));
#endif
#if BTA_SC_INCLUDED==TRUE
    memset((void *)bta_sc_cb_ptr, 0, sizeof(tBTA_SC_CB));
#endif
#if BTA_HD_INCLUDED==TRUE
    memset((void *)bta_hd_cb_ptr, 0, sizeof(tBTA_HD_CB));
#endif
#if BTA_HH_INCLUDED==TRUE
    memset((void *)bta_hh_cb_ptr, 0, sizeof(tBTA_HH_CB));
#endif
#if BTA_FM_INCLUDED==TRUE
    memset((void *)bta_fm_cb_ptr, 0, sizeof(tBTA_FM_CB));
#endif
#if BTA_FMTX_INCLUDED==TRUE
    memset((void *)bta_fmtx_cb_ptr, 0, sizeof(tBTA_FMTX_CB));
#endif
#if 0
#if BTA_JV_INCLUDED==TRUE
    memset((void *)bta_jv_cb_ptr, 0, sizeof(tBTA_JV_CB));
#endif
#endif
#if BTA_SSR_INCLUDED==TRUE
    memset((void *)bta_ssr_cb_ptr, 0, sizeof(tBTA_SSR_CB));
#endif
#if BTA_HL_INCLUDED==TRUE
    memset((void *)bta_hl_cb_ptr, 0, sizeof(tBTA_HL_CB));
#endif
#if BTA_GATT_INCLUDED==TRUE
    memset((void *)bta_gattc_cb_ptr, 0, sizeof(tBTA_GATTC_CB));
    memset((void *)bta_gatts_cb_ptr, 0, sizeof(tBTA_GATTS_CB));
#endif
#if BTA_PAN_INCLUDED==TRUE
    memset((void *)bta_pan_cb_ptr, 0, sizeof(tBTA_PAN_CB));
#endif
#if L2CAP_LE_COC_INCLUDED==TRUE
    memset((void *)bta_lecoc_cb_ptr, 0, sizeof(tBTA_LECOC_CB));
#endif

#endif /* BTA_INCLUDED == TRUE */
}
