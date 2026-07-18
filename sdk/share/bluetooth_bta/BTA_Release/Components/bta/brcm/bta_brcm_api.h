/*****************************************************************************
**
**  Name:           bta_brcm_api.h
**
**  Description:    This is the public interface file for the patch ram
**                  subsystem of BTA, Broadcom's
**                  Bluetooth application layer for mobile phones.
**
**  Copyright (c) 2003-2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_BRCM_API_H
#define BTA_BRCM_API_H

#include "bta_api.h"
#include "bta_sys.h"
#include "brcm_api.h"

#if (BLE_INCLUDED == TRUE && BLE_BRCM_INCLUDED == TRUE)

#define BTA_DM_API_BLE_SCATTERNET    0x01
#define BTA_DM_API_BLE_DYN_SCAN      0x02
#endif

/* Search callback */
typedef void (tBTA_DM_ADV_PF_CBACK)(tBTA_DM_BLE_PF_EVT event, tBTA_DM_BLE_PF_COND_TYPE cfg_cond, tBTA_STATUS status);

/* callback function */
typedef void (tBTA_PRM_CBACK) (tBTA_STATUS status);

#define BTA_BLE_MULTI_ADV_MAX       BTM_BLE_MULTI_ADV_MAX
#define BTA_BLE_MULTI_ADV_ILLEGAL   0

/* multi adv callback event */
#define BTA_BLE_MULTI_ADV_ENB_EVT           1
#define BTA_BLE_MULTI_ADV_DISABLE_EVT       2
#define BTA_BLE_MULTI_ADV_PARAM_EVT         3
#define BTA_BLE_MULTI_ADV_DATA_EVT          4

typedef UINT8 tBTA_BLE_MULTI_ADV_EVT;

/* multi adv callback */
typedef void (tBTA_BLE_MULTI_ADV_CBACK)(tBTA_BLE_MULTI_ADV_EVT event, UINT8 inst_id, void *p_ref, tBTA_STATUS status);

/*****************************************************************************
**  External Function Declarations
*****************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
**
** Function         BTA_PatchRam
**
** Description      Patch Ram function
**
** Parameters       p_cback - callback to indicate status of download operation.
**                  p_name  - path and file name of the patch file.
**                          - if p_name is NULL, try to use patch data built into code.
**                  fs_app_id - app_id used by bta file system callout functions
**                            - to distinguish between applications which uses FS.
**                  address - address of patch ram
**
**
** Returns          void
**
**
*******************************************************************************/
BTA_API extern void BTA_PatchRam(tBTA_PRM_CBACK *p_cback, const char *p_name,
                                 UINT8 fs_app_id, UINT32 address);

/*******************************************************************************
**
** Function         BTA_BrcmInit
**
** Description      This function initializes Broadcom specific VS handler in BTA
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_BrcmInit (void);

/*******************************************************************************
**
** Function         BTA_ReadBrcmFeatures
**
** Description      This function read BRCM supported features
**
** Returns          void
**
*******************************************************************************/
BTA_API extern UINT8 *BTA_ReadBrcmFeatures (void);

#if (BTM_TBFC_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         BTA_DmSetRemoteTBFCMode
**
** Description      This function set the remote device TBFC scan mode
**
** Parameters:      bd_addr       - Address of the peer device
**                  scan_mode    - TBFC scan enabled/disabled
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_DmSetRemoteTBFCMode(BD_ADDR bd_addr, tBTA_DM_TBFC_SCAN_MODE tbfc_scan);

#endif /* BTM_TBFC_INCLUDED */

#if (BLE_INCLUDED == TRUE && BLE_BRCM_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         BTA_DmBleMonitorLinkRSSI
**
** Description      This function start RSSI monitoring on a LE link
**
** Parameters       conn_addr: BLE connection address to be monitored.
**                  alert_mask: alert mask, if BTA_BLE_RSSI_ALERT_NONE (0) indicate
**                              turn off the monitoring
**                  low_threshold: lowest rssi threshold
**                  range: threshold between low and high
**                  hi_threshold: highest threshold
**                  p_cback: ADV RSSI alert indication callback.
**
** Returns          status
**
*******************************************************************************/
BTA_API extern void BTA_DmBleMonitorLinkRSSI   (BD_ADDR conn_addr,
                                                tBTA_DM_BLE_RSSI_ALERT_MASK  alert_mask,
                                                INT8 low_threshold,
                                                INT8 range,
                                                INT8 hi_threshold,
                                                tBTA_DM_BLE_RSSI_CBACK *p_rssi_cback);

/*******************************************************************************
**
** Function         BTA_DmBleMonitorAdvRSSI
**
** Description      Enable or disable RSSI monitoring on non-connectable adv device
**
** Parameters       alert_mask: alert mask, if BTA_BLE_RSSI_ALERT_NONE (0) indicate
**                              turn off the monitoring
**                  low_threshold: lowest rssi threshold
**                  range: threshold between low and high
**                  hi_threshold: highest threshold
**                  p_cback: ADV RSSI alert indication callback.
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_DmBleMonitorAdvRSSI   ( tBTA_DM_BLE_RSSI_ALERT_MASK  alert_mask,
                                                INT8 low_threshold,
                                                INT8 range,
                                                INT8 hi_threshold,
                                                tBTA_DM_BLE_RSSI_CBACK *p_cback);

/*******************************************************************************
**
** Function         BTA_DmEnableScanFilter
**
** Description      This function is called to enable the adv data payload filter
**
** Parameters       action - enable or disable the APCF feature
**                  p_cmpl_cback - Command completed callback
**                  ref_value - Reference value
**
** Returns          void
**
*******************************************************************************/
extern void BTA_DmEnableScanFilter(UINT8 action,
                                        tBTA_DM_BLE_PF_STATUS_CBACK *p_cmpl_cback,
                                        tBTA_DM_BLE_REF_VALUE ref_value);
/*******************************************************************************
**
** Function         BTA_DmBleScanFilterSetup
**
** Description      This function is called to setup the filter params
**
** Parameters       p_target: enable the filter condition on a target device; if NULL
**                  filt_index - Filter index
**                  p_filt_params -Filter parameters
**                  ref_value - Reference value
**                  action - Add, delete or clear
**                  p_cmpl_back - Command completed callback
**
** Returns          void
**
*******************************************************************************/
extern void BTA_DmBleScanFilterSetup(UINT8 action,
                                                   tBTA_DM_BLE_PF_FILT_INDEX filt_index,
                                                   tBTA_DM_BLE_PF_FILT_PARAMS *p_filt_params,
                                                   tBLE_BD_ADDR *p_target,
                                                   tBTA_DM_BLE_PF_PARAM_CBACK *p_cmpl_cback,
                                                   tBTA_DM_BLE_REF_VALUE ref_value);

/*******************************************************************************
**
** Function         BTA_DmBleCfgFilterCondition
**
** Description      This function is called to configure the adv data payload filter
**                  condition.
**
** Parameters       action: to read/write/clear
**                  cond_type: filter condition type.
**                  p_cond: filter condition paramter
**
** Returns          void
**
*******************************************************************************/
extern void BTA_DmBleCfgFilterCondition(tBTA_DM_BLE_SCAN_COND_OP action,
                                                 tBTA_DM_BLE_PF_COND_TYPE cond_type,
                                                 tBTA_DM_BLE_PF_FILT_INDEX filt_index,
                                                 tBTA_DM_BLE_PF_COND_PARAM *p_cond,
                                                 tBTA_DM_BLE_PF_CFG_CBACK *p_cmpl_cback,
                                                 tBTA_DM_BLE_REF_VALUE ref_value);

/*******************************************************************************
**
** Function         BTA_DmBleScatternetEnable
**
** Description      This function to enable/disable LE scatternet capability.
**                  Bluetooth 4.0 spec does not allow a single device support both
**                  master and slave connections. To support scatternet special
**                    requirement needs FW support.
**
** Parameter        enable: turn on/off this feature.
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_DmBleScatternetEnable (BOOLEAN enable);

/*******************************************************************************
**
** Function         BTA_DmBleDynamicScanEnable
**
** Description      This function to enable/disable concurrent scan and initiator
**                  capability. Need HW/FW support, available in 4334B0, 4324B0, 4335A0.
**                  4334B0 only support active scan with no white list and initiator
**                  with or without white list.
**                  4324B0/4335A0: supports all scan and initiator combination
**                  with or without white list.
**                  When white list are enabled for both operations, one white
**                  list will be used. So BTE/BTA host does not use WL for scan(inquiry)
**                  activity for current implementation.
**                  When both the Scan command and the Initiating are issued, the scan
**                  parameter provided for scan activity will overwrite the scan params
**                  provided through BTA_DmSetBleConnScanParams().
**
** Parameter        enable: turn on/off this feature.
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void BTA_DmBleDynamicScanEnable (BOOLEAN enable);


/*******************************************************************************
**
** Function         BTA_BleEnableAdvInstance
**
** Description      This function enables a Multi-ADV instance with the specified
**                  adv parameters
**
** Parameters       p_params: pointer to the adv parameter structure.
**                  p_cback: callback function associated to this adv instance.
**                  p_ref: reference data pointer to this adv instance.
**
** Returns          BTA_SUCCESS if command started sucessfully; otherwise failure.
**
*******************************************************************************/
BTA_API extern tBTA_STATUS BTA_BleEnableAdvInstance (tBTA_BLE_ADV_PARAMS *p_params,
                                            tBTA_BLE_MULTI_ADV_CBACK *p_cback,
                                            void *p_ref);

/*******************************************************************************
**
** Function         BTA_BleUpdateAdvInstParam
**
** Description      This function update a Multi-ADV instance with the specififed
**                  adv parameters.
**
** Parameters       inst_id: Adv instance to update the parameter.
**                  p_params: pointer to the adv parameter structure.
**
** Returns          BTA_SUCCESS if command started sucessfully; otherwise failure.
**
*******************************************************************************/
BTA_API extern tBTA_STATUS BTA_BleUpdateAdvInstParam (UINT8 inst_id, tBTA_BLE_ADV_PARAMS *p_params);


/*******************************************************************************
**
** Function         BTA_BleCfgAdvInstData
**
** Description      This function configure a Multi-ADV instance with the specififed
**                  adv data or scan response data.
**
** Parameter        inst_id: adv instance to write to.
**                  is_scan_rsp: is this scan response data or adv data.
**                  data_mask: adv data type in bit mask.
**                  p_data: pointer to the ADV data structure tBTA_BLE_ADV_DATA. This
**                          memory space can not be freed until BTA_BLE_MULTI_ADV_DATA_EVT
**                          is sent to application.
**
** Returns          BTA_SUCCESS if command started sucessfully; otherwise failure.
**
*******************************************************************************/
BTA_API extern tBTA_STATUS BTA_BleCfgAdvInstData (UINT8 inst_id, BOOLEAN is_scan_rsp,
                            tBTA_BLE_AD_MASK data_mask,
                            tBTA_BLE_ADV_DATA *p_data);

/*******************************************************************************
**
** Function         BTA_BleDisableAdvInstance
**
** Description      This function disable a Multi-ADV instance.
**
** Parameter        inst_id: instance ID to disable.
**
** Returns          BTA_SUCCESS if command started sucessfully; otherwise failure.
**
*******************************************************************************/
BTA_API extern tBTA_STATUS BTA_BleDisableAdvInstance (UINT8 inst_id);

BTA_API extern tBTA_STATUS BTA_DmRemoveAllACL(void);
#endif

#ifdef __cplusplus
}
#endif

#endif


