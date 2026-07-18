/****************************************************************************
**
**  Name:          btapp_ble_main.c
**
**  Description:   Contains btapp functions for proximity reporter.
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#if( BLE_INCLUDED == TRUE) && (BTA_GATT_INCLUDED == TRUE)
#include "bta_platform.h"
#include "bte_glue.h"
#include "btapp_utility.h"


void btapp_ble_main_init(void)
{

}

static void btapp_ble_obs_res_cback (tBTA_DM_SEARCH_EVT event, tBTA_DM_SEARCH *p_data)
{
    tBTA_DM_INQ_RES* inq_res = &p_data->inq_res;

    switch (event)
    {
        case BTA_DM_INQ_RES_EVT:
        {
            BRCM_PLATFORM_TRACE("scaned bda: %s, rssi:%i"LINE_ENDING,
                            btapp_utl_bda_to_str(inq_res->bd_addr), inq_res->rssi);
        }
        break;

        case BTA_DM_INQ_CMPL_EVT:
        {
            BRCM_PLATFORM_TRACE("%s  BLE observe complete. Num Resp %d"LINE_ENDING,
                              __FUNCTION__,p_data->inq_cmpl.num_resps);
            break;
        }

        default:
            BRCM_PLATFORM_TRACE("%s : Unhandle event 0x%x"LINE_ENDING, __FUNCTION__, event);

        return;
    }
}

void btapp_ble_observer_start(void)
{
    btapp_gattc_cb.enabled = TRUE;

    BTA_DmBleObserve(TRUE, 0, btapp_ble_obs_res_cback);
}

void btapp_ble_observer_stop(void)
{
    btapp_gattc_cb.enabled = FALSE;

    BTA_DmBleObserve(FALSE, 0, NULL);
}

#endif

