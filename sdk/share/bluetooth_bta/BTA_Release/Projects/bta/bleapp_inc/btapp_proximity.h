/*****************************************************************************
**                                                                            
**  Name:             btapp_proximity.h                                              
**                                                                            
**  Description:     This file contains btapp interface               
**				     definition                                         
**                                                                            
**  Copyright (c) 2000-2010, Broadcom Corp., All Rights Reserved.               
**  Widcomm Bluetooth Core. Proprietary and confidential.                     
******************************************************************************/

#ifndef BTAPP_PROXIMITY_SR_H
#define BTAPP_PROXIMITY_SR_H


#include "bta_gatt_api.h"
#include "btapp_gattc_profile.h"


#define BTUI_SERVICE_PROX_MASK      (BTUI_SERVICE_LINK_LOSS_BIT)

#ifndef BTUI_PROX_PATH_LOSS_THRESH
#define BTUI_PROX_PATH_LOSS_THRESH      55
#endif

#if( defined BTA_GATT_INCLUDED ) && (BTA_GATT_INCLUDED == TRUE)
typedef struct
{
    BOOLEAN                 enabled;
    UINT8                   tx_pwr_inst_id;

}tBTUI_PROXIMITY_RPT_CB;
extern tBTUI_PROXIMITY_RPT_CB btui_proximity_rpt_cb;


typedef struct
{
    BOOLEAN                 enabled;
    UINT32                  sel_item_data;
}tBTUI_PROX_MNT_CB;

extern tBTUI_PROX_MNT_CB btui_prox_mnt_cb;

/* Proximity Reporter functions */
extern void btapp_proximity_reporter_init(void);
extern void btapp_proximity_reporter_disable(void);

/* Proximity Monitor functions */
extern void btui_menu_prox_mnt_main(void);

extern void btapp_prox_mnt_init(void);
extern void btapp_prox_mnt_disable(void);
extern void btapp_prox_mnt_connect(BD_ADDR remote_bda, BOOLEAN is_direct);
extern void btapp_prox_mnt_disconnect(BD_ADDR remote_bda);
extern void btapp_prox_mnt_discover_service(BD_ADDR remote_bda, UINT16 uuid);
extern void btapp_prox_mnt_set_ll_alert_level(BD_ADDR remote_bda, tBTUI_ALERT_LEVEL alert_level);
extern void btapp_prox_mnt_read_tx_power_level(BD_ADDR remote_bda);
extern void btapp_prox_mnt_cfg_tx_power_level_notification(BD_ADDR remote_bda);
extern void btapp_prox_mnt_write_immediate_alert_level(BD_ADDR remote_bda, tBTUI_IA_ALERT_LEVEL alert_level);

extern void btapp_calculate_path_loss(BD_ADDR remote_bda);
extern void btapp_prox_mnt_rssi_alert_cback(BD_ADDR bd_addr, UINT8 alert_evt, INT8 rssi);


#endif
#endif


