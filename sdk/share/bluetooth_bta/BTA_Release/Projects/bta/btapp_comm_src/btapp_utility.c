/****************************************************************************
**
**  Name:          btapp_utility.c
**
**  Description:   Contains btapp utility apis source file
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bta_platform.h"
#include "bte_glue.h"

static BD_ADDR setting_addr;
static UINT8 btapp_utl_keys_str[] = "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ";
const UINT8 btapp_utl_empty_keys[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static UINT8 utl_bdaddr_str[] = "00:00:00:00:00:00";  /* shared memory used for displaying string */
static BD_ADDR utl_bdaddr_buf;

/*******************************************************************************
**
** Function         btapp_app_start_timer
**
** Description      Start a timer for the specified amount of time.
**                  NOTE: The timeout resolution is in SECONDS! (Even
**                          though the timer structure field is ticks)
**
** Returns          void
**
*******************************************************************************/
void btapp_start_timer (TIMER_LIST_ENT *p_tle, UINT16 type, UINT32 timeout)
{
    BT_HDR *p_msg;

    /* if timer list is currently empty, start periodic GKI timer */
    if (btapp_cb.timer_queue.p_first == NULL)
    {
        if (GKI_get_taskid() != BTAPPL_TASK)
        {
            /* post event to start timer in BTU task */
            if ((p_msg = (BT_HDR *)GKI_getbuf(BT_HDR_SIZE)) != NULL)
            {
                p_msg->event = BTAPP_START_TIMER;
                GKI_send_msg (BTAPPL_TASK, TASK_MBOX_0, p_msg);
            }
        }
        else
        {
            /* Start free running 1 second timer for list management */
            GKI_start_timer (TIMER_0, GKI_SECS_TO_TICKS (1), TRUE);
        }
    }

    GKI_remove_from_timer_list (&btapp_cb.timer_queue, p_tle);

    p_tle->event = type;
    p_tle->ticks = timeout;         /* Save the number of seconds for the timer */

    GKI_add_to_timer_list (&btapp_cb.timer_queue, p_tle);

}

/*******************************************************************************
**
** Function         btapp_stop_timer
**
** Description      Stop the target timer.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_stop_timer(TIMER_LIST_ENT *p_tle)
{
    BT_HDR *p_msg;
    GKI_remove_from_timer_list (&btapp_cb.timer_queue, p_tle);

    if (GKI_get_taskid() != BTAPPL_TASK)
    {
        /* post event to stop timer in BTAPPL task */
        if ((p_msg = (BT_HDR *)GKI_getbuf(BT_HDR_SIZE)) != NULL)
        {
            p_msg->event = BTAPP_STOP_TIMER;
            GKI_send_msg (BTAPPL_TASK, TASK_MBOX_0, p_msg);
        }
    }
    else
    {
        /* if timer list is empty stop periodic GKI timer */
        if (btapp_cb.timer_queue.p_first == NULL)
        {
            GKI_stop_timer(TIMER_0);
        }
    }
}

/*******************************************************************************
**
** Function         btapp_set_addr_cplt_cb
**
** Description      btapp layer set BT device address complete callback.
**
** Parameters       status
**
** Returns          void
**
** Note
*******************************************************************************/
static void btapp_set_addr_cplt_cb(tBTA_STATUS status)
{
    if(status == BTA_SUCCESS)
    {
        //It may need update the NVRAM sections about device address
        bdcpy(btapp_cfg.set_local_addr, setting_addr);
        APPL_TRACE_API1("Set device address %s success!", utl_bd_addr_to_string(setting_addr));
    }
    else
    {
        APPL_TRACE_API1("Set device address failed, status:%d!", status);
    }
}

/*******************************************************************************
**
** Function         btapp_set_dev_addr
**
** Description      btapp layer set BT device address.
**
** Parameters       bd_addr: need set device address, the address endian need be little endian
**
** Returns          void
**
** Note             this api only provide for the shell menu, it should set the device address out of any connections.
*******************************************************************************/
void btapp_set_dev_addr(BD_ADDR bd_addr)
{
    if(bdcmp(bd_addr, bd_addr_any) == 0 || bdcmp(bd_addr, bd_addr_null) == 0)
    {
        APPL_TRACE_ERROR0("Don't set the any or null address!!!");
    }
    else
    {
        //Set device address
        bdcpy(setting_addr, bd_addr);
        BTA_DmSetLocalDeviceAddr(bd_addr, btapp_set_addr_cplt_cb);
    }
}

/*******************************************************************************
**
** Function         btapp_utl_keys_to_str
**
** Description      convert keys to string format.
**
** Parameters       keys: link keys
**
** Returns          void
**
** Note             this api convert mental link key data to easy string format which the OTA analyer can easily input and recognize.
*******************************************************************************/
UINT8 *btapp_utl_keys_to_str (BT_OCTET16 keys)
{
    memset (btapp_utl_keys_str, 0, sizeof(btapp_utl_keys_str));

    sprintf ((char *)btapp_utl_keys_str, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ", keys[0],
        keys[1], keys[2], keys[3], keys[4], keys[5], keys[6], keys[7], keys[8],
        keys[9], keys[10], keys[11], keys[12], keys[13], keys[14], keys[15]);

    return btapp_utl_keys_str;
}

/********************************************************************************
**
**  Name:           utl_bd_addr_to_string
**
**                  Generates a string representation of a bd address
**
**  Parameters:     bd_addr: bd address.
**
**  Returns:        A string representation of a bd address.
**
**  Note: All the strings returned share the same address so they should be used
**      used immediately.
**
*********************************************************************************/
UINT8* btapp_utl_bda_to_str (BD_ADDR bd_addr)
{
    memset (utl_bdaddr_str, 0, sizeof(utl_bdaddr_str));

    sprintf ((char *)utl_bdaddr_str, "%02x:%02x:%02x:%02x:%02x:%02x", bd_addr[0],
        bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);

    return utl_bdaddr_str;
}

/********************************************************************************
**
**  Name:           btapp_utl_str_to_bda
**
**                  Generates bd address from string representation
**
**  Parameters:     bd_addr: string representation of a bd address
**
**  Returns:        bd address.
**
**
*********************************************************************************/
BD_ADDR* btapp_utl_str_to_bda(const char* str_bda)
{
    uint8_t* p_parm = (uint8_t*)str_bda;
    uint16_t pos = 0, idx = 0;

    while(pos < strlen(p_parm) && idx < sizeof(utl_bdaddr_buf))
    {
        unsigned int val = 0;
        if (sscanf(p_parm + pos, "%02x", &val) != 1)
        {
            return NULL;
        }
        utl_bdaddr_buf[idx++] = val;
        pos += 3;
    }

    return &utl_bdaddr_buf;
}

