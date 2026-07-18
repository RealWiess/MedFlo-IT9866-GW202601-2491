/*****************************************************************************
**
**  Name:           bta_ag_ci.c
**
**  Description:    This is the implementation file for audio gateway call-in
**                  functions.
**
**  Copyright (c) 2003-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <string.h>
#include "bta_api.h"
#include "bta_ag_api.h"
#include "bta_ag_int.h"
#include "bta_ag_ci.h"
#include "gki.h"

/******************************************************************************
**
** Function         bta_ag_ci_rx_write
**
** Description      This function is called to send data to the AG when the AG
**                  is configured for AT command pass-through.  The function
**                  copies data to an event buffer and sends it.
**
** Returns          void
**
******************************************************************************/
void bta_ag_ci_rx_write(UINT16 handle, char *p_data, UINT16 len)
{
    tBTA_AG_CI_RX_WRITE *p_buf;

    if ((p_buf = (tBTA_AG_CI_RX_WRITE *) GKI_getbuf(sizeof(tBTA_AG_CI_RX_WRITE))) != NULL)
    {
        p_buf->hdr.event = BTA_AG_CI_RX_WRITE_EVT;
        p_buf->hdr.layer_specific = handle;
        len = (len < BTA_AG_MTU) ? len : BTA_AG_MTU;
        BCM_STRNCPY_S(p_buf->p_data, sizeof(p_buf->p_data), p_data, len);
        p_buf->p_data[len] = 0;
        bta_sys_sendmsg(p_buf);
    }
}

/******************************************************************************
**
** Function         bta_ag_ci_slc_ready
**
** Description      This function is called to notify AG that SLC is up at
**                  the application. This funcion is only used when the app
**                  is running in pass-through mode.
**
** Returns          void
**
******************************************************************************/
void bta_ag_ci_slc_ready(UINT16 handle)
{
    tBTA_AG_DATA *p_buf;

    if ((p_buf = (tBTA_AG_DATA *)GKI_getbuf(sizeof(tBTA_AG_DATA))) != NULL)
    {
        p_buf->hdr.event = BTA_AG_CI_SLC_READY_EVT;
        p_buf->hdr.layer_specific = handle;
        bta_sys_sendmsg(p_buf);
    }
}

