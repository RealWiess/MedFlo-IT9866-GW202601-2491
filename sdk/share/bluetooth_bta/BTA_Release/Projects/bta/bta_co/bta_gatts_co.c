/*****************************************************************************
**
**  Name:           bta_gattc_co.c
**
**  Description:    This file contains the GATT client call-out
**                  function implementation for Insight.
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
*****************************************************************************/

#include "bta_platform.h"
#include "bte_glue.h"

#if( defined BLE_INCLUDED ) && (BLE_INCLUDED == TRUE)
#if( defined BTA_GATT_INCLUDED ) && (BTA_GATT_INCLUDED == TRUE)

/*****************************************************************************
**  Function Declarations
*****************************************************************************/
/*
extern void btapp_gatts_update_handle_range(BOOLEAN is_add, tBTA_GATTS_HNDL_RANGE *p_hndl_range);
extern BOOLEAN btapp_gatts_get_handle_range(UINT8 index, tBTA_GATTS_HNDL_RANGE *p_handle);
extern BOOLEAN btapp_gatts_srv_chg(tBTA_GATTS_SRV_CHG_CMD cmd,
                                   tBTA_GATTS_SRV_CHG_REQ *p_req,
                                   tBTA_GATTS_SRV_CHG_RSP *p_rsp);
*/
/*******************************************************************************
**
** Function         bta_gatts_co_update_handle_range
**
** Description      This callout function is executed by GATTS when a GATT server
**                  handle range ios to be added or removed.
**
** Parameter        is_add: true is to add a handle range; otherwise is to delete.
**                  p_hndl_range: handle range.
**
** Returns          void.
**
*******************************************************************************/
void bta_gatts_co_update_handle_range(BOOLEAN is_add, tBTA_GATTS_HNDL_RANGE *p_hndl_range)
{
    /*
    btapp_gatts_update_handle_range(is_add, p_hndl_range);
    */
}

/*******************************************************************************
**
** Function         bta_gatts_co_srv_chg
**
** Description      This call-out is to read/write/remove service change related
**                  informaiton. The request consists of the cmd and p_req and the
**                  response is returned in p_rsp
**
** Parameter        cmd - request command
**                  p_req - request paramters
**                  p_rsp - response data for the request
**
** Returns          TRUE - if the request is processed successfully and
**                         the response is returned in p_rsp.
**                  FASLE - if the request can not be processed
**
*******************************************************************************/
BOOLEAN bta_gatts_co_srv_chg(tBTA_GATTS_SRV_CHG_CMD cmd,
                             tBTA_GATTS_SRV_CHG_REQ *p_req,
                             tBTA_GATTS_SRV_CHG_RSP *p_rsp)
{
    APPL_TRACE_EVENT1("bta_gatts_co_srv_chg cmd=%d", cmd);
    return FALSE;//btapp_gatts_srv_chg(cmd, p_req, p_rsp);
}

/*******************************************************************************
**
** Function         bta_gatts_co_load_handle_range
**
** Description      This callout function is executed by GATTS when a GATT server
**                  handle range is requested to be loaded from NV.
**
** Parameter
**
** Returns          void.
**
*******************************************************************************/
BOOLEAN bta_gatts_co_load_handle_range(UINT8 index,
                                       tBTA_GATTS_HNDL_RANGE *p_handle_range)
{
    return FALSE;//btapp_gatts_get_handle_range(index, p_handle_range);
}
#endif
#endif
