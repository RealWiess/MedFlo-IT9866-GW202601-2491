/****************************************************************************
**
**  Name:          btapp_pan.c
**
**  Description:   Contains application functions for pan
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bt_target.h"

#if ((defined BTA_PAN_INCLUDED ) && (BTA_PAN_INCLUDED == TRUE))

#include "gki.h"
#include "bta_api.h"
#include "btapp_int.h"
#include "bta_pan_api.h"
#include "bd.h"
#include "btapp_pan.h"
#include "btapp_dm.h"

#include "btapp_nv.h"

#define BTAPP_PAN_CONSOLE_INCLUDE FALSE

tBTAPP_PAN_CB btapp_pan_cb;

#if ((defined BTAPP_PAN_CONSOLE_INCLUDE) && (BTAPP_PAN_CONSOLE_INCLUDE == TRUE))

static void btapp_pan_menu(void)
{
    btapp_console_puts("0. Exit ");
    btapp_console_puts("1. CONNECT PAN");
    btapp_console_puts("2. DISCONNECT PAN");
    btapp_console_puts("    ");
}

static void btapp_pan_handler(tBTAPP_CONSOLE_MSG* p_console_msg)
{
    switch(p_console_msg->data[0])
    {
        case MENU_ITEM_0:
            if( btapp_console_menu_db[CONSOLE_DM_ID] != NULL)
            {
                btapp_console_cb.console_pre_state = btapp_console_cb.console_state;
                btapp_console_cb.console_state = CONSOLE_IDLE;
                btapp_console_menu_db[CONSOLE_DM_ID]();
            }
            break;

        case MENU_ITEM_1:
            btapp_console_cb.console_pre_state = btapp_console_cb.console_state;
            btapp_console_cb.console_state = CONSOLE_PAN_CONNECT;
            btapp_pan_open_conn(BTA_PAN_ROLE_NAP);
            break;

        case MENU_ITEM_2:
            btapp_console_cb.console_pre_state = btapp_console_cb.console_state;
            btapp_console_cb.console_state = CONSOLE_PAN_DISCONNECT;
            btapp_pan_close();
            break;

        default:
            APPL_TRACE_ERROR0("Command or menu not support!!!");
            break;
    }
}

void btapp_pan_console_init(void)
{
    btapp_console_register(btapp_pan_menu, btapp_pan_handler, CONSOLE_PAN_ID);
}
#endif

/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/

static void btapp_pan_cback(tBTA_PAN_EVT event, tBTA_PAN *p_data);

/*******************************************************************************
**
** Function         btapp_pan_init
**
** Description     Initialises PAN application
**
**
** Returns          void
*******************************************************************************/
void btapp_pan_init(void)
{
    tBTA_PAN_ROLE_INFO u_info;
    tBTA_PAN_ROLE_INFO gn_info;
    tBTA_PAN_ROLE_INFO nap_info;
    tBTA_PAN_ROLE  roles = 0;

    u_info.app_id = BTAPP_PAN_ID_PANU;
    u_info.p_srv_name = btapp_cfg.panu_service_name;
    u_info.sec_mask = btapp_cfg.pan_security;

    gn_info.app_id = BTAPP_PAN_ID_GN;
    gn_info.p_srv_name = btapp_cfg.pangn_service_name;
    gn_info.sec_mask = btapp_cfg.pan_security;

    nap_info.app_id = BTAPP_PAN_ID_NAP;
    nap_info.p_srv_name = btapp_cfg.pannap_service_name;
    nap_info.sec_mask = btapp_cfg.pan_security;

    if(btapp_cfg.panu_supported)
    {
        roles |= BTA_PAN_ROLE_PANU;
    }
    if (btapp_cfg.pangn_supported)
    {
        roles |= BTA_PAN_ROLE_GN;
    }
    if (btapp_cfg.pannap_supported)
    {
        roles |= BTA_PAN_ROLE_NAP;
    }

    BTA_PanEnable(btapp_pan_cback);

    BTA_PanSetRole(roles , &u_info, &gn_info, &nap_info);

#if ((defined BTAPP_PAN_CONSOLE_INCLUDE) && (BTAPP_PAN_CONSOLE_INCLUDE == TRUE))
    btapp_pan_console_init();
#endif
}

/*******************************************************************************
**
** Function         btapp_pan_close
**
** Description     Closes PAN connection
**
**
** Returns          void
*******************************************************************************/
/*TBD: void btapp_pan_close(UINT8 conn_index) for GU/NAP */
//void btapp_pan_close (UINT8 conn_index)
void btapp_pan_close (void)
{
    BTA_PanClose(btapp_pan_cb.app_cb[0].conn_handle);
}

/*******************************************************************************
**
** Function         btapp_pan_open_conn
**
** Description     Opens PAN connection
**
**
** Returns          void
*******************************************************************************/
void btapp_pan_open_conn(tBTA_PAN_ROLE peer_role)
{
    APPL_TRACE_DEBUG6("btapp_cb.peer_bdaddr:%02x-%02x-%02x-%02x-%02x-%02x",
                       btapp_cb.peer_bdaddr[0], btapp_cb.peer_bdaddr[1],
                       btapp_cb.peer_bdaddr[2], btapp_cb.peer_bdaddr[3],
                       btapp_cb.peer_bdaddr[4], btapp_cb.peer_bdaddr[5]);
    BTA_PanOpen(btapp_cb.peer_bdaddr, BTA_PAN_ROLE_PANU, peer_role);
}

/*******************************************************************************
**
** Function         btapp_pan_set_device_authorized
**
** Description      Bond with the device
**
**
** Returns          void
*******************************************************************************/
void btapp_pan_set_device_authorized (tBTAPP_REM_DEVICE * p_device_rec)
{
    /* update BTA with this information.If a device is set as trusted, BTA will
    not ask application for authorization, */

    p_device_rec->is_trusted = TRUE;
    p_device_rec->trusted_mask |= BTA_PANU_SERVICE_MASK |BTA_NAP_SERVICE_MASK | BTA_GN_SERVICE_MASK ;
    btapp_store_device(p_device_rec);
    btapp_dm_sec_add_device(p_device_rec);

}
/*******************************************************************************
**
** Function         btapp_pan_cback
**
** Description      Call back from BTA pan
**
**
** Returns          void
*******************************************************************************/
static void btapp_pan_cback(tBTA_PAN_EVT event, tBTA_PAN *p_data)
{
    tBTAPP_PAN_CB *p_tempcb = &btapp_pan_cb;
    UINT8 i;

    APPL_TRACE_DEBUG1("pan callback %d",event);
    switch (event)
    {
    case BTA_PAN_ENABLE_EVT:
        APPL_TRACE_EVENT0("btapp_pan_cback BTA_PAN_ENABLE_EVT");
        break;
    case BTA_PAN_SET_ROLE_EVT:
        APPL_TRACE_EVENT0("btapp_pan_cback BTA_PAN_SET_ROLE_EVT");
        break;
    case BTA_PAN_OPENING_EVT:
        APPL_TRACE_EVENT0("btapp_pan_cback BTA_PAN_OPENING_EVT");
        break;
    case BTA_PAN_OPEN_EVT:
        APPL_TRACE_DEBUG1("BTA_PAN_OPEN_EVT: status:%d", p_data->open.status);
        if (p_data->open.status == BTA_PAN_SUCCESS)
        {
            APPL_TRACE_DEBUG6("  bd_addr:%02x-%02x-%02x-%02x-%02x-%02x",
                    p_data->open.bd_addr[0], p_data->open.bd_addr[1],
                    p_data->open.bd_addr[2], p_data->open.bd_addr[3],
                    p_data->open.bd_addr[4], p_data->open.bd_addr[5]);
            APPL_TRACE_DEBUG1(" handle %x", p_data->open.handle);
            APPL_TRACE_DEBUG1(" Local role %d", p_data->open.local_role);
            BTAPP_PAN_SETSTATUS(BTAPP_PAN_ST_CONNECT);
        }

        if(p_data->open.local_role == BTA_PAN_ROLE_PANU)
        {
            p_tempcb->app_cb[0].conn_handle = p_data->open.handle;
            p_tempcb->app_cb[0].service_id = BTA_PANU_SERVICE_ID;
        }
        /*TBD: i =0 for PANU Only. Need to implement GU/NAP Role. */
#if 0
        else if(p_data->open.local_role == PAN_ROLE_GN_SERVER)
        {
            p_tempcb->app_cb[i].conn_handle = p_data->open.handle;
            p_tempcb->app_cb[i].service_id = BTA_GN_SERVICE_ID;
        }
        else if(p_data->open.local_role == PAN_ROLE_NAP_SERVER)
        {
            p_tempcb->app_cb[i].conn_handle = p_data->open.handle;
            p_tempcb->app_cb[i].service_id = BTA_NAP_SERVICE_ID;
        }
#endif
        break;
    case BTA_PAN_CLOSE_EVT:

        for (i=0; i<BTAPP_PAN_NUM_SERVICES; i++)
        {
            if(p_tempcb->app_cb[i].conn_handle == p_data->close.handle)
            {
                APPL_TRACE_DEBUG3("BTA_PAN_CLOSE_EVT cb[%d]: hdl %d, srvid %d", i,
                p_tempcb->app_cb[i].conn_handle, p_tempcb->app_cb[i].service_id);

                break;
            }
        }

        /* If valid handle found set the role */
        if (i < BTAPP_PAN_NUM_SERVICES)
        {
            if (p_tempcb->app_cb[i].service_id == PAN_ROLE_CLIENT)
            {
                APPL_TRACE_DEBUG0("p_tempcb->app_cb[i].service_id=PAN_ROLE_CLIENT");
            }
            else if(p_tempcb->app_cb[i].service_id == PAN_ROLE_GN_SERVER)
            {
                APPL_TRACE_DEBUG0("p_tempcb->app_cb[i].service_id=PAN_ROLE_GN_SERVER");
            }
            else if (p_tempcb->app_cb[i].service_id == PAN_ROLE_NAP_SERVER)
            {
                APPL_TRACE_DEBUG0("p_tempcb->app_cb[i].service_id=PAN_ROLE_NAP_SERVER");
            }

            /* Done with control block */
            p_tempcb->app_cb[i].conn_handle = 0;
            p_tempcb->app_cb[i].service_id = 0;
        }
        else    /* Bad service id */
        {
            APPL_TRACE_ERROR0("BTA_PAN_CLOSE_EVT: NO Active Service");
        }
        BTAPP_PAN_SETSTATUS(BTAPP_PAN_ST_DISCONNECT);
        break;
    }
}

#endif
