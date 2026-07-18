/*****************************************************************************
**                                                                           *
**  Name:          bte_load.c                                                *
**                                                                           *
**  Description:   This module contains the routines that allocate and       *
**                 free memory used for the components' control blocks.      *
**                 It is only used if dynamic memory is being used.          *
**                                                                           *
**                 NOTE: It is expected that these functions will need to be *
**                       taylored to meet the specific needs of the          *
**                       application.                                        *
**                                                                           *
**  Copyright (c) 2000-2014, Broadcom Corp., All Rights Reserved.            *
**  Broadcom Bluetooth Core. Proprietary and confidential.                   *
******************************************************************************/
#include "bt_target.h"
#if GKI_DYNAMIC_MEMORY == TRUE
#include "gki_int.h"
#else
#include "gki.h"
#endif

#ifndef BTA_INCLUDED
#define BTA_INCLUDED FALSE
#endif

/*****************************************************************************
** G L O B A L   P O I N T E R S   T O   C O N T R O L   B L O C K S         *
******************************************************************************/

/*****************************************************************
 * Allocate pointers for each mandatory core stack component     *
 *****************************************************************/
#if BTU_DYNAMIC_MEMORY == TRUE
#include "btu.h"
BTU_API tBTU_CB *btu_cb_ptr = NULL;
#endif

#if BTM_DYNAMIC_MEMORY == TRUE
#include "btm_int.h"
BTM_API tBTM_CB *btm_cb_ptr = NULL;
#endif

#if L2C_DYNAMIC_MEMORY == TRUE
#include "l2c_int.h"
L2C_API tL2C_CB *l2c_cb_ptr = NULL;
#endif

#if SDP_DYNAMIC_MEMORY == TRUE
#include "sdpint.h"
SDP_API tSDP_CB *sdp_cb_ptr = NULL;
#endif


/****************************************************************
 * Allocate pointers for each optional core stack component     *
 ****************************************************************/
#if SLIP_INCLUDED == TRUE && SLIP_DYNAMIC_MEMORY == TRUE
#include "slip.h"
BT_API tSLIP_CB  *slip_cb_ptr = NULL;
#endif

#if RFCOMM_INCLUDED == TRUE && RFC_DYNAMIC_MEMORY == TRUE
#include "rfc_int.h"
RFC_API tRFC_CB  *rfc_cb_ptr = NULL;
#endif

#if BNEP_INCLUDED == TRUE && BNEP_DYNAMIC_MEMORY == TRUE
#include "bnep_int.h"
BNEP_API tBNEP_CB *bnep_cb_ptr = NULL;
#endif

#if MCA_INCLUDED == TRUE && MCA_DYNAMIC_MEMORY == TRUE
#include "mca_int.h"
MCA_API tMCA_CB *mca_cb_ptr = NULL;
#endif

#if OBX_INCLUDED == TRUE && OBX_DYNAMIC_MEMORY == TRUE
#include "obx_int.h"
OBX_API tOBX_CB *obx_cb_ptr = NULL;
#endif

#if AVDT_INCLUDED == TRUE && AVDT_DYNAMIC_MEMORY == TRUE
#include "avdt_int.h"
AVDT_API tAVDT_CB *avdt_cb_ptr = NULL;
#endif

#if AVCT_INCLUDED == TRUE && AVCT_DYNAMIC_MEMORY == TRUE
#include "avct_int.h"
AVCT_API tAVCT_CB *avct_cb_ptr = NULL;
#endif

#if BLE_INCLUDED == TRUE
#if GATT_DYNAMIC_MEMORY == TRUE
#include "gatt_int.h"
GATT_API tGATT_CB *gatt_cb_ptr = NULL;
#endif

#if SMP_INCLUDED == TRUE && SMP_DYNAMIC_MEMORY == TRUE
#include "smp_int.h"
SMP_API tSMP_CB *smp_cb_ptr = NULL;
#endif

#if BRCM_DYNAMIC_MEMORY == TRUE
#include "brcm_ble.h"
BTM_API tBTM_BLE_BRCM_CB *btm_ble_brcm_ptr = NULL;
#endif
#endif
/***************************************************************
 * Allocate pointer for each optional profile stack component  *
 ***************************************************************/
#if GAP_INCLUDED == TRUE && GAP_DYNAMIC_MEMORY == TRUE
#include "gap_int.h"
GAP_API tGAP_CB *gap_cb_ptr = NULL;
#endif

#if PAN_INCLUDED == TRUE && PAN_DYNAMIC_MEMORY == TRUE
#include "pan_int.h"
PAN_API tPAN_CB *pan_cb_ptr = NULL;
#endif

#if SPP_INCLUDED == TRUE && SPP_DYNAMIC_MEMORY == TRUE
#include "spp_int.h"
SPP_API tSPP_CB *spp_cb_ptr = NULL;
#endif

#if DUN_INCLUDED == TRUE && DUN_DYNAMIC_MEMORY == TRUE
#include "dun_int.h"
DUN_API tDUN_CB *dun_cb_ptr = NULL;
#endif

#if HFP_INCLUDED == TRUE && HFP_DYNAMIC_MEMORY == TRUE
#include "hfp_int.h"
HFP_API tHFP_CB *hfp_hcb_ptr = NULL;
#endif

#if HSP2_INCLUDED == TRUE && HSP2_DYNAMIC_MEMORY == TRUE
#include "hsp2_int.h"
HSP2_API tHSP2_CB *hsp2_cb_ptr = NULL;
#endif

#if GOEP_INCLUDED == TRUE && GOEP_DYNAMIC_MEMORY == TRUE
#include "goep_int.h"
GOEP_API tGOEP_CB *goep_cb_ptr = NULL;
#endif

#if FTP_INCLUDED == TRUE && FTP_DYNAMIC_MEMORY == TRUE
#include "ftp_int.h"
FTP_API tFTP_CB *ftp_cb_ptr = NULL;
#endif

#if OPP_INCLUDED == TRUE && OPP_DYNAMIC_MEMORY == TRUE
#include "opp_int.h"
OPP_API tOPP_CB *opp_cb_ptr = NULL;
#endif

#if BPP_INCLUDED == TRUE && BPP_DYNAMIC_MEMORY == TRUE
#include "bpp_int.h"
BPP_API tBPP_CB *bpp_cb_ptr = NULL;
#endif

#if BPP_SND_INCLUDED == TRUE && BPP_DYNAMIC_MEMORY == TRUE
#include "bpp_int.h"
BPP_API tBPP_SND_CB *bpp_snd_cb_ptr = NULL;
#endif

#if BIP_INCLUDED == TRUE && BIP_DYNAMIC_MEMORY == TRUE
#include "bip_int.h"
BIP_API tBIP_CB *bip_cb_ptr = NULL;
#endif

#if A2D_INCLUDED == TRUE && A2D_DYNAMIC_MEMORY == TRUE
#include "a2d_int.h"
A2D_API tA2D_CB *a2d_cb_ptr = NULL;
#endif

#if AVRC_INCLUDED == TRUE && AVRC_DYNAMIC_MEMORY == TRUE
#include "avrc_int.h"
AVRC_API tAVRC_CB *avrc_cb_ptr = NULL;
#endif

#if VDP_INCLUDED == TRUE && VDP_DYNAMIC_MEMORY == TRUE
#include "vdp_int.h"
VDP_API tVDP_CB *vdp_cb_ptr = NULL;
#endif

#if HID_DEV_INCLUDED == TRUE && HID_DYNAMIC_MEMORY == TRUE
#include "hidd_int.h"
HID_API tHIDDEV_CB *hidd_cb_ptr = NULL;
#endif

#if HID_HOST_INCLUDED == TRUE && HID_DYNAMIC_MEMORY == TRUE
#include "hidh_int.h"
HID_API tHID_HOST_CTB *hidh_cb_ptr = NULL;
#endif

#if BTA_INCLUDED == TRUE && BTA_DYNAMIC_MEMORY == TRUE
#include "bta_api.h"
#include "bta_sys.h"

#if BTA_3DS_INCLUDED == TRUE
#include "bta_3ds_int.h"
tBTA_3DS_CB *bta_3ds_cb_ptr = NULL;
#endif

#if BTA_BAV_INCLUDED == TRUE
#include "bta_bav_int.h"
tBTA_BAV_CB *bta_bav_cb_ptr = NULL;
#endif

#if BTA_AC_INCLUDED == TRUE
#include "bta_acc_int.h"
#include "bta_acs_int.h"
tBTA_ACC_CB *bta_acc_cb_ptr = NULL;
tBTA_ACS_CB *bta_acs_cb_ptr = NULL;
#endif

#if BTA_AG_INCLUDED == TRUE
#include "bta_ag_int.h"
tBTA_AG_CB *bta_ag_cb_ptr = NULL;
#endif

#if BTA_HS_INCLUDED == TRUE
#include "bta_hs_int.h"
tBTA_HS_CB *bta_hs_cb_ptr = NULL;
#endif

#include "bta_dm_int.h"
tBTA_DM_CB *bta_dm_cb_ptr = NULL;
tBTA_DM_SEARCH_CB *bta_dm_search_cb_ptr = NULL;
tBTA_DM_DI_CB *bta_dm_di_cb_ptr = NULL;

#include "bta_gps_int.h"
tBTA_GPS_CB *bta_gps_cb_ptr = NULL;

#if BTA_DG_INCLUDED == TRUE
#include "bta_dg_api.h"
#include "bta_dg_int.h"
tBTA_DG_CB *bta_dg_cb_ptr = NULL;
#endif

#if BTA_FT_INCLUDED == TRUE
#include "bta_ftc_int.h"
#include "bta_fts_int.h"
tBTA_FTC_CB *bta_ftc_cb_ptr = NULL;
tBTA_FTS_CB *bta_fts_cb_ptr = NULL;
#endif

#if BTA_PBC_INCLUDED == TRUE
#include "bta_pbc_int.h"
tBTA_PBC_CB *bta_pbc_cb_ptr = NULL;
#endif

#if BTA_PBS_INCLUDED == TRUE
#include "bta_pbs_int.h"
tBTA_PBS_CB *bta_pbs_cb_ptr = NULL;
#endif

#if BTA_OP_INCLUDED == TRUE
#include "bta_opc_int.h"
#include "bta_ops_int.h"
tBTA_OPC_CB *bta_opc_cb_ptr = NULL;
tBTA_OPS_CB *bta_ops_cb_ptr = NULL;
#endif

#if BTA_SS_INCLUDED==TRUE
#include "bta_ss_int.h"
tBTA_SS_CB *bta_ss_cb_ptr = NULL;
#endif

#if BTA_BI_INCLUDED==TRUE
#include "bta_bic_int.h"
#include "bta_bis_int.h"
tBTA_BIC_CB *bta_bic_cb_ptr;
tBTA_BIS_CB *bta_bis_cb_ptr;
#endif

#if BTA_PR_INCLUDED==TRUE
#include "bta_pr_int.h"
tBTA_PR_CB *bta_pr_cb_ptr;
#endif

#if BTA_PAN_INCLUDED==TRUE
#include "bta_pan_int.h"
tBTA_PAN_CB *bta_pan_cb_ptr;
#endif

#if BTA_AR_INCLUDED==TRUE
#include "bta_ar_int.h"
tBTA_AR_CB *bta_ar_cb_ptr;
#endif

#if BTA_AV_INCLUDED==TRUE
#include "bta_av_int.h"
tBTA_AV_CB *bta_av_cb_ptr;
#endif

#if BTA_AVK_INCLUDED==TRUE
#include "bta_avk_int.h"
tBTA_AVK_CB *bta_avk_cb_ptr;
#endif

#if BTA_SC_INCLUDED==TRUE
#include "bta_sc_int.h"
tBTA_SC_CB *bta_sc_cb_ptr = NULL;
#endif

#if BTA_HD_INCLUDED==TRUE
#include "bta_hd_int.h"
tBTA_HD_CB *bta_hd_cb_ptr = NULL;
#endif

#if BTA_HH_INCLUDED==TRUE
#include "bta_hh_int.h"
tBTA_HH_CB *bta_hh_cb_ptr = NULL;
#endif

#if BTA_FM_INCLUDED==TRUE
#include "bta_fm_int.h"
tBTA_FM_CB *bta_fm_cb_ptr = NULL;
#endif

#if BTA_FMTX_INCLUDED==TRUE
#include "bta_fmtx_int.h"
tBTA_FMTX_CB *bta_fmtx_cb_ptr = NULL;
#endif

#if BTA_JV_INCLUDED==TRUE
#include "bta_jv_int.h"
tBTA_JV_CB *bta_jv_cb_ptr = NULL;
#endif

#if BTA_SSR_INCLUDED==TRUE
#include "bta_ssr_int.h"
tBTA_SSR_CB *bta_ssr_cb_ptr = NULL;
#endif

#if BTA_MCE_INCLUDED == TRUE
#include "bta_mce_int.h"
tBTA_MCE_CB *bta_mce_cb_ptr = NULL;
#endif

#if BTA_MSE_INCLUDED == TRUE
#include "bta_mse_int.h"
tBTA_MSE_CB *bta_mse_cb_ptr = NULL;
#endif

#if BTA_HL_INCLUDED == TRUE
#include "bta_hl_int.h"
tBTA_HL_CB *bta_hl_cb_ptr = NULL;
#endif

#if BTA_GATT_INCLUDED == TRUE
#include "bta_gattc_int.h"
tBTA_GATTC_CB *bta_gattc_cb_ptr = NULL;

#include "bta_gatts_int.h"
tBTA_GATTS_CB *bta_gatts_cb_ptr = NULL;
#endif

#if L2CAP_LE_COC_INCLUDED == TRUE
#include "bta_lecoc_int.h"
tBTA_LECOC_CB *bta_lecoc_cb_ptr = NULL;
#endif

#if BTU_DUAL_STACK_BTC_INCLUDED == TRUE
#include "bta_rt_int.h"
tBTA_RT_CB *bta_rt_cb_ptr = NULL;
#endif

#if BTA_MIP_INCLUDED == TRUE
#include "bta_mip_int.h"
tBTA_MIP_CB *bta_mip_cb_ptr = NULL;
#endif

#include "bta_sys_int.h"
tBTA_SYS_CB *bta_sys_cb_ptr = NULL;

#if (defined BT_BRCM_VS_INCLUDED && BT_BRCM_VS_INCLUDED == TRUE)
/* control block for patch ram downloading */
#include "bta_brcm_int.h"
tBTA_PRM_CB *bta_prm_cb_ptr = NULL;
#endif

#endif /* BTA_INCLUDED */

/* control block for bt-trace */
#if BTTRC_INCLUDED == TRUE && BTTRC_DYNAMIC_MEMORY == TRUE
#include "bttrc_int.h"
BT_API tBTTRC_TRACE_CB *bt_trace_cb_ptr = NULL;
#endif

/*****************************************************************************
**                    C O M P O N E N T   T A B L E                          *
******************************************************************************/
typedef struct
{
    void  **p_cb;   /* Pointer to the address of a control block of a component */
    UINT32  size;
} tBT_LOAD_TBL;

static tBT_LOAD_TBL bt_load_tbl[] =
{
/***********************************************************
 * Load memory for each mandatory core stack component     *
 ***********************************************************/
#if BTU_DYNAMIC_MEMORY == TRUE
    {(void **)&btu_cb_ptr, sizeof(tBTU_CB)},
#endif

#if BTM_DYNAMIC_MEMORY == TRUE
    {(void **)&btm_cb_ptr, sizeof(tBTM_CB)},
#endif

#if L2C_DYNAMIC_MEMORY == TRUE
    {(void **)&l2c_cb_ptr, sizeof(tL2C_CB)},
#endif

#if SDP_DYNAMIC_MEMORY == TRUE
    {(void **)&sdp_cb_ptr, sizeof(tSDP_CB)},
#endif

/**********************************************************
 * Load memory for each optional core stack component     *
 **********************************************************/
#if SLIP_INCLUDED == TRUE && SLIP_DYNAMIC_MEMORY == TRUE
    {(void **)&slip_cb_ptr, sizeof(tSLIP_CB)},
#endif

#if RFCOMM_INCLUDED == TRUE && RFC_DYNAMIC_MEMORY == TRUE
    {(void **)&rfc_cb_ptr, sizeof(tRFC_CB)},
#endif

#if BNEP_INCLUDED == TRUE && BNEP_DYNAMIC_MEMORY == TRUE
    {(void **)&bnep_cb_ptr, sizeof(tBNEP_CB)},
#endif

#if OBX_INCLUDED == TRUE && OBX_DYNAMIC_MEMORY == TRUE
    {(void **)&obx_cb_ptr, sizeof(tOBX_CB)},
#endif

#if AVDT_INCLUDED == TRUE && AVDT_DYNAMIC_MEMORY == TRUE
    {(void **)&avdt_cb_ptr, sizeof(tAVDT_CB)},
#endif

#if AVCT_INCLUDED == TRUE && AVCT_DYNAMIC_MEMORY == TRUE
    {(void **)&avct_cb_ptr, sizeof(tAVCT_CB)},
#endif

#if MCA_INCLUDED == TRUE && MCA_DYNAMIC_MEMORY == TRUE
    {(void **)&mca_cb_ptr, sizeof(tMCA_CB)},
#endif

 /******************************************************
 * Load memory for each optional profile component     *
 *******************************************************/
#if BLE_INCLUDED == TRUE
#if BRCM_DYNAMIC_MEMORY == TRUE
    {(void **)&btm_ble_brcm_ptr, sizeof(tBTM_BLE_BRCM_CB)},
#endif

#if GATT_DYNAMIC_MEMORY == TRUE
    {(void **)&gatt_cb_ptr, sizeof(tGATT_CB)},
#endif

#if SMP_INCLUDED == TRUE && SMP_DYNAMIC_MEMORY == TRUE
    {(void **)&smp_cb_ptr, sizeof(tSMP_CB)},
#endif
#endif

#if GAP_INCLUDED == TRUE && GAP_DYNAMIC_MEMORY == TRUE
    {(void **)&gap_cb_ptr, sizeof(tGAP_CB)},
#endif

#if PAN_INCLUDED == TRUE && PAN_DYNAMIC_MEMORY == TRUE
    {(void **)&pan_cb_ptr, sizeof(tPAN_CB)},
#endif

#if SPP_INCLUDED == TRUE && SPP_DYNAMIC_MEMORY == TRUE
    {(void **)&spp_cb_ptr, sizeof(tSPP_CB)},
#endif

#if DUN_INCLUDED == TRUE && DUN_DYNAMIC_MEMORY == TRUE
    {(void **)&dun_cb_ptr, sizeof(tDUN_CB)},
#endif

#if HFP_INCLUDED == TRUE && HFP_DYNAMIC_MEMORY == TRUE
    {(void **)&hfp_hcb_ptr, sizeof(tHFP_CB)},
#endif

#if HSP2_INCLUDED == TRUE && HSP2_DYNAMIC_MEMORY == TRUE
    {(void **)&hsp2_cb_ptr, sizeof(tHSP2_CB)},
#endif

#if GOEP_INCLUDED == TRUE && GOEP_DYNAMIC_MEMORY == TRUE
    {(void **)&goep_cb_ptr, sizeof(tGOEP_CB)},
#endif

#if FTP_INCLUDED == TRUE && FTP_DYNAMIC_MEMORY == TRUE
    {(void **)&ftp_cb_ptr, sizeof(tFTP_CB)},
#endif

#if OPP_INCLUDED == TRUE && OPP_DYNAMIC_MEMORY == TRUE
    {(void **)&opp_cb_ptr, sizeof(tOPP_CB)},
#endif

#if BPP_INCLUDED == TRUE && BPP_DYNAMIC_MEMORY == TRUE
    {(void **)&bpp_cb_ptr, sizeof(tBPP_CB)},
#endif

#if BPP_SND_INCLUDED == TRUE && BPP_DYNAMIC_MEMORY == TRUE
    {(void **)&bpp_snd_cb_ptr, sizeof(tBPP_SND_CB)},
#endif

#if BIP_INCLUDED == TRUE && BIP_DYNAMIC_MEMORY == TRUE
    {(void **)&bip_cb_ptr, sizeof(tBIP_CB)},
#endif

#if A2D_INCLUDED == TRUE && A2D_DYNAMIC_MEMORY == TRUE
    {(void **)&a2d_cb_ptr, sizeof(tA2D_CB)},
#endif

#if AVRC_INCLUDED == TRUE && AVRC_DYNAMIC_MEMORY == TRUE
    {(void **)&avrc_cb_ptr, sizeof(tAVRC_CB)},
#endif

#if HID_DEV_INCLUDED == TRUE && HID_DYNAMIC_MEMORY == TRUE
    {(void **)&hidd_cb_ptr, sizeof(tHIDDEV_CB)},
#endif

#if HID_HOST_INCLUDED == TRUE && HID_DYNAMIC_MEMORY == TRUE
    {(void **)&hidh_cb_ptr, sizeof(tHID_HOST_CTB)},
#endif

#if BTA_INCLUDED == TRUE && BTA_DYNAMIC_MEMORY == TRUE
    {(void **)&bta_sys_cb_ptr, sizeof(tBTA_SYS_CB)},
    {(void **)&bta_dm_cb_ptr, sizeof(tBTA_DM_CB)},
    {(void **)&bta_dm_search_cb_ptr, sizeof(tBTA_DM_SEARCH_CB)},
    {(void **)&bta_dm_di_cb_ptr, sizeof(tBTA_DM_DI_CB)},
#if (defined BT_BRCM_VS_INCLUDED && BT_BRCM_VS_INCLUDED == TRUE)
    {(void **)&bta_prm_cb_ptr, sizeof(tBTA_PRM_CB)},
#endif
    {(void **)&bta_gps_cb_ptr, sizeof(tBTA_GPS_CB)},

#if BTA_3DS_INCLUDED == TRUE
    {(void **)&bta_3ds_cb_ptr, sizeof(tBTA_3DS_CB)},
#endif

#if BTA_BAV_INCLUDED == TRUE
    {(void **)&bta_bav_cb_ptr, sizeof(tBTA_BAV_CB)},
#endif

#if BTA_AG_INCLUDED == TRUE
    {(void **)&bta_ag_cb_ptr, sizeof(tBTA_AG_CB)},
#endif

#if BTA_AC_INCLUDED == TRUE
    {(void **)&bta_acc_cb_ptr, sizeof(tBTA_ACC_CB)},
    {(void **)&bta_acs_cb_ptr, sizeof(tBTA_ACS_CB)},
#endif

#if BTA_HS_INCLUDED == TRUE
    {(void **)&bta_hs_cb_ptr, sizeof(tBTA_HS_CB)},
#endif

#if BTA_DG_INCLUDED == TRUE
    {(void **)&bta_dg_cb_ptr, sizeof(tBTA_DG_CB)},
#endif

#if BTA_FT_INCLUDED==TRUE
    {(void **)&bta_ftc_cb_ptr, sizeof(tBTA_FTC_CB)},
    {(void **)&bta_fts_cb_ptr, sizeof(tBTA_FTS_CB)},
#endif

#if BTA_PBC_INCLUDED==TRUE
    {(void **)&bta_pbc_cb_ptr, sizeof(tBTA_PBC_CB)},
#endif

#if BTA_PBS_INCLUDED==TRUE
    {(void **)&bta_pbs_cb_ptr, sizeof(tBTA_PBS_CB)},
#endif

#if BTA_OP_INCLUDED==TRUE
    {(void **)&bta_opc_cb_ptr, sizeof(tBTA_OPC_CB)},
    {(void **)&bta_ops_cb_ptr, sizeof(tBTA_OPS_CB)},
#endif

#if BTA_SS_INCLUDED==TRUE
    {(void **)&bta_ss_cb_ptr, sizeof(tBTA_SS_CB)},
#endif

#if BTA_BI_INCLUDED==TRUE
    {(void **)&bta_bic_cb_ptr, sizeof(tBTA_BIC_CB)},
    {(void **)&bta_bis_cb_ptr, sizeof(tBTA_BIS_CB)},
#endif

#if BTA_PR_INCLUDED==TRUE
    {(void **)&bta_pr_cb_ptr, sizeof(tBTA_PR_CB)},
#endif

#if BTA_PAN_INCLUDED==TRUE
    {(void **)&bta_pan_cb_ptr, sizeof(tBTA_PAN_CB)},
#endif

#if BTA_AR_INCLUDED==TRUE
    {(void **)&bta_ar_cb_ptr, sizeof(tBTA_AR_CB)},
#endif

#if BTA_AV_INCLUDED==TRUE
    {(void **)&bta_av_cb_ptr, sizeof(tBTA_AV_CB)},
#endif

#if BTA_AVK_INCLUDED==TRUE
    {(void **)&bta_avk_cb_ptr, sizeof(tBTA_AVK_CB)},
#endif

#if BTA_SC_INCLUDED==TRUE
    {(void **)&bta_sc_cb_ptr, sizeof(tBTA_SC_CB)},
#endif

#if BTA_HD_INCLUDED==TRUE
    {(void **)&bta_hd_cb_ptr, sizeof(tBTA_HD_CB)},
#endif

#if BTA_HH_INCLUDED==TRUE
    {(void **)&bta_hh_cb_ptr, sizeof(tBTA_HH_CB)},
#endif

#if BTA_FM_INCLUDED==TRUE
    {(void **)&bta_fm_cb_ptr, sizeof(tBTA_FM_CB)},
#endif

#if BTA_FMTX_INCLUDED==TRUE
    {(void **)&bta_fmtx_cb_ptr, sizeof(tBTA_FMTX_CB)},
#endif

#if BTA_SSR_INCLUDED==TRUE
    {(void **)&bta_ssr_cb_ptr, sizeof(tBTA_SSR_CB)},
#endif

#if BTA_MCE_INCLUDED==TRUE
    {(void **)&bta_mce_cb_ptr, sizeof(tBTA_MCE_CB)},
#endif

#if BTA_MSE_INCLUDED==TRUE
    {(void **)&bta_mse_cb_ptr, sizeof(tBTA_MSE_CB)},
#endif

#if BTA_HL_INCLUDED==TRUE
    {(void **)&bta_hl_cb_ptr, sizeof(tBTA_HL_CB)},
#endif

#if BTA_GATT_INCLUDED==TRUE
    {(void **)&bta_gattc_cb_ptr, sizeof(tBTA_GATTC_CB)},

    {(void **)&bta_gatts_cb_ptr, sizeof(tBTA_GATTS_CB)},
#endif

#if L2CAP_LE_COC_INCLUDED==TRUE
    {(void **)&bta_lecoc_cb_ptr, sizeof(tBTA_LECOC_CB)},
#endif

#if BTU_DUAL_STACK_BTC_INCLUDED == TRUE
    {(void **)&bta_rt_cb_ptr, sizeof(tBTA_RT_CB)},
#endif


#if BTA_MIP_INCLUDED == TRUE
    {(void **)&bta_mip_cb_ptr, sizeof(tBTA_MIP_CB)},
#endif

#endif

/**********************************************************
 * Load memory for each trace (bttrc) component           *
 **********************************************************/
#if BTTRC_INCLUDED == TRUE && BTTRC_DYNAMIC_MEMORY == TRUE
    {(void **)&bt_trace_cb_ptr, sizeof(tBTTRC_TRACE_CB)},
#endif

    {(void **)NULL, 0}   /* End of Table Marker */
};


/*****************************************************************************
**                          F U N C T I O N S                                *
******************************************************************************/
/*******************************************************************************
**
** Function         BTE_LoadStack
**
** Description      This function is called before starting up the Stack (BTU Task)
**                  to allocate memory dynamically for the components' control blocks.
**
**                  Note:  This function does not need to be called if the control
**                         blocks for all components are staticly defined.
**
** Returns          void
**
*******************************************************************************/
BT_API void BTE_LoadStack(void)
{
    tBT_LOAD_TBL  *p_entry = &bt_load_tbl[0];

    for (p_entry = &bt_load_tbl[0]; p_entry->p_cb != NULL; p_entry++)
    {
        *(p_entry->p_cb) = GKI_os_malloc(p_entry->size);
    }
}


/*****************************************************************************
**
** Function         BTE_UnloadStack
**
** Description      Initialize control block memory for each component.
**
**
** Returns          void
**
******************************************************************************/
BT_API void BTE_UnloadStack(void)
{
    tBT_LOAD_TBL  *p_entry = &bt_load_tbl[0];

    for (p_entry = &bt_load_tbl[0]; p_entry->p_cb != NULL; p_entry++)
    {
        if (*(p_entry->p_cb) != NULL)
        {
            GKI_os_free(*(p_entry->p_cb));
            *(p_entry->p_cb) = NULL;
        }
    }
}

/*******************************************************************************
**
** Function         BTE_LoadGki
**
** Description      This function is called before calling GKI_init() to allocate
**                  memory for the GKI control block (if GKI_DYNAMIC_MEMORY) is
**                  being used.
**
**                  Note:  This function does not need to be called if the GKI is
**                         using static memory.  Also, the buffere pools that gki
**                         uses are initialized in GKI_init() and are static or
**                         dynamic based on GKI_USE_DYNAMIC_BUFFERS defined in
**                         target.h.
**
** Returns          void
**
*******************************************************************************/
#if GKI_DYNAMIC_MEMORY == TRUE
GKI_API tGKI_CB *gki_cb_ptr = NULL;

BT_API void BTE_LoadGki(void)
{
#if GKI_DYNAMIC_MEMORY == TRUE
    gki_cb_ptr = (tGKI_CB *)GKI_os_malloc(sizeof(tGKI_CB));
#endif
}

/*****************************************************************************
**
** Function         BTE_UnloadGki
**
** Description      Initialize GKI block memory.
**
**
** Returns          void
**
******************************************************************************/
BT_API void BTE_UnloadGki(void)
{
    if (gki_cb_ptr != NULL)
    {
        GKI_os_free(gki_cb_ptr);
        gki_cb_ptr = NULL;
    }
}

#endif

