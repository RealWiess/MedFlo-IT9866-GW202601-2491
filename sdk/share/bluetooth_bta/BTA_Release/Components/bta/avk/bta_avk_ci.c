/*****************************************************************************
**
**  Name:           bta_avk_ci.c
**
**  Description:    This is the implementation file for advanced audio/video
**                  call-in functions.
**
**  Copyright (c) 2004-2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bta_api.h"
#include "bta_sys.h"
#include "bta_avk_int.h"
#include "bta_avk_ci.h"

#include <string.h>

/*******************************************************************************
**
** Function         bta_avk_ci_setconfig
**
** Description      This function must be called in response to function
**                  bta_avk_co_audio_setconfig() or bta_avk_co_video_setconfig.
**                  Parameter err_code is set to an AVDTP status value;
**                  AVDT_SUCCESS if the codec configuration is ok,
**                  otherwise error.
**
** Returns          void
**
*******************************************************************************/
void bta_avk_ci_setconfig(tBTA_AVK_CHNL chnl, UINT8 err_code, UINT8 category)
{
    tBTA_AVK_CI_SETCONFIG  *p_buf;

    if ((p_buf = (tBTA_AVK_CI_SETCONFIG *) GKI_getbuf(sizeof(tBTA_AVK_CI_SETCONFIG))) != NULL)
    {
        p_buf->hdr.layer_specific   = chnl;
        p_buf->hdr.event = (err_code == AVDT_SUCCESS) ?
                           BTA_AVK_CI_SETCONFIG_OK_EVT : BTA_AVK_CI_SETCONFIG_FAIL_EVT;
        p_buf->err_code = err_code;
        p_buf->category = category;

        bta_sys_sendmsg(p_buf);
    }
}

/*******************************************************************************
**
** Function         bta_avk_ci_cp_scms
**
** Description      This function is called to set the SCMS Content Protection
**                  for an AVK connection
**
** Returns          void
**
*******************************************************************************/
void bta_avk_ci_cp_scms(tBTA_AVK_CHNL chnl, BOOLEAN enable, UINT8 scms_hdr)
{
    tBTA_AVK_CI_CP_SCMS  *p_buf;

    if ((p_buf = (tBTA_AVK_CI_CP_SCMS *) GKI_getbuf(sizeof(tBTA_AVK_CI_CP_SCMS))) != NULL)
    {
        p_buf->hdr.layer_specific   = chnl;
        p_buf->hdr.event = BTA_AVK_CI_CP_SCMS_EVT;
        p_buf->hdr.len = 0;
        p_buf->hdr.offset = 0;
        p_buf->enable = enable;
        p_buf->scms_hdr = scms_hdr;
        bta_sys_sendmsg(p_buf);
    }
}

#if (BTU_BTC_SNK_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         bta_avk_ci_audio_btc_start
**
** Description      This function is called in response to function
**                  bta_avk_co_audio_btc_start
**
** Returns          void
**
*******************************************************************************/
void bta_avk_ci_audio_btc_start(tBTA_AVK_CHNL chnl)
{
    tBTA_AVK_CI_AUDIO_BTC_START  *p_buf;

    if ((p_buf = (tBTA_AVK_CI_AUDIO_BTC_START *) GKI_getbuf(sizeof(tBTA_AVK_CI_AUDIO_BTC_START))) != NULL)
    {
        p_buf->hdr.layer_specific   = chnl;
        p_buf->hdr.event = BTA_AVK_CI_AUDIO_BTC_START_EVT;
        p_buf->hdr.len = 0;
        p_buf->hdr.offset = 0;
        bta_sys_sendmsg(p_buf);
    }
}

#endif
