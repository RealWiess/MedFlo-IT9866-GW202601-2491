/****************************************************************************
**                                                                           
**  Name:          btapp_proximity_reporter.c                                           
**                 
**  Description:   Contains btapp functions for proximity reporter.
**                 
**                                                                           
**  Copyright (c) 2003-2010, Broadcom Corp, All Rights Reserved.              
**  Broadcom Bluetooth Core. Proprietary and confidential.                    
******************************************************************************/

#include "bt_target.h"

//#if( defined BTA_PROXIMITY_INCLUDED ) && (BTA_PROXIMITY_INCLUDED == TRUE)
#if( BLE_INCLUDED == TRUE) && (BTA_GATT_INCLUDED == TRUE)


#include "bta_api.h"
#include "btapp.h"
#include "btapp_int.h"
#include "btapp_immediate_alert.h"
#include "btapp_tx_power.h"
#include "btapp_proximity.h"
#include "btapp_linkloss.h"

/* BTUI proximity control block */
tBTUI_PROXIMITY_RPT_CB btui_proximity_rpt_cb;

static void btui_proximity_tx_pwr_cback(UINT8 inst_id, UINT8 status);

/*******************************************************************************
**
** Function         btapp_proximity_reporter_init
**
** Description      initialize proximity reporter
**                  
**
** Returns          void
**
*******************************************************************************/
void btapp_proximity_reporter_init(void)
{
    /* need tp update the namespace as proximity only */
    tBTA_GATT_CHAR_PRES pres_fmt= {0};

    memset(&btui_proximity_rpt_cb, 0, sizeof(tBTUI_PROXIMITY_RPT_CB));

    /* instatiate link loss service */
    btapp_linkloss_init();
    /* immediate alert service */
    btapp_immediate_alert_init();
    /* tx power */ 
    //btapp_tx_power_init();
    //btapp_tx_power_instatiate(&pres_fmt, FALSE, TRUE, btui_proximity_tx_pwr_cback);

    btui_proximity_rpt_cb.enabled = TRUE;
    
}
/*******************************************************************************
**
** Function         btapp_proximity_reporter_disable
**
** Description      disable the proximity reporter.
**                  
**
** Returns          void
**
*******************************************************************************/
void btapp_proximity_reporter_disable(void)
{
    /* disable link loss service */
    btapp_linkloss_disable();

    btapp_immediate_alert_disable();

    btapp_tx_power_disable(btui_proximity_rpt_cb.tx_pwr_inst_id);

    btui_proximity_rpt_cb.enabled = FALSE;

}
/*******************************************************************************
**
** Function         btui_proximity_tx_pwr_cback
**
** Description      tx power service creation callback for proximity profile.
**                  
** Returns          void
**
*******************************************************************************/
static void btui_proximity_tx_pwr_cback(UINT8 inst_id, tBTA_GATT_STATUS status)
{
    if (status == BTA_GATT_OK)
        btui_proximity_rpt_cb.tx_pwr_inst_id = inst_id;
}

//#endif
#endif



