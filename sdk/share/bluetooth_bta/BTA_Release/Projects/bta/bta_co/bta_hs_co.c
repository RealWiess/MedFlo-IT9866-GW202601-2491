/*****************************************************************************
**
**  Name:           bta_hs_co.c
**
**  Description:    This file contains the headset  callout function
**                  implementation for Insight.
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
*****************************************************************************/

#include "bta_platform.h"
#include "bte_glue.h"

/*******************************************************************************
**
** Function         bta_hs_co_init
**
** Description      This callout function is executed by HS when it is
**                  started by calling BTA_HsEnable().  This function can be
**                  used by the phone to initialize audio paths or for other
**                  initialization purposes.
**
**
** Returns          Void.
**
*******************************************************************************/
void bta_hs_co_init(void)
{

    APPL_TRACE_DEBUG0("bta_hs_co_init");

}


/*******************************************************************************
**
** Function         bta_hs_co_audio_state
**
** Description      This function is called by the HS when the audio connection
**                  goes up or down.
**
**
** Returns          void
**
*******************************************************************************/
void bta_hs_co_audio_state(UINT16 handle, UINT8 app_id, tBTA_HS_AUDIO_STATE status)
{

    APPL_TRACE_DEBUG2("bta_hs_co_audio_state handle %d status:%d", handle, status);

}




