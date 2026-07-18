/*****************************************************************************
**
**  Name:           bta_media_sco.c
**
**  Description:    This file contains an example codec implementation used
**                  by SCO over HCI.  The implementation uses the
**                  Windows waveIn API to get PCM samples.
**
**  Copyright (c) 2004, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bta_platform.h"
#include "bte_glue.h"
/*****************************************************************************
**  Constants
*****************************************************************************/
/* maximum number of bytes in each waveIn pcm data buffer
** samples per frame * num channels * bytes per sample
** Default 15ms will require 240 bytes PCM samples
*/
#define BTA_SCO_CODEC_BUF_MAX          BTM_SCO_DATA_SIZE_MAX

/* number of waveIn pcm data buffers */
#define BTA_SCO_CODEC_NUM_BUF          18


/*****************************************************************************
**  Data types for SCO codec
*****************************************************************************/

typedef struct
{
    unsigned char   data[BTA_SCO_CODEC_BUF_MAX];    /* data */
} tBTA_SCO_CODEC_BUF;

typedef struct
{
    UINT16                  cb_event;
    UINT16                  sco_handle;
    UINT8                   pool_id;                /* GKI buffer pool for output packets */
    UINT8                   cur_sco_route;

    tBTA_SCO_CODEC_BUF      buf[BTA_SCO_CODEC_NUM_BUF];    /* waveIn buffers */
    BUFFER_Q                out_q;                  /* output packet GKI queue */
    BT_HDR                  *p_buf;
    tBTA_SCO_CODEC_CBACK   *p_cback;               /* API callback */

} tBTA_SCO_CODEC_CB;

/*****************************************************************************
**  Local data
*****************************************************************************/

/* SCO codec control block */
tBTA_SCO_CODEC_CB bta_sco_codec_cb;

/*******************************************************************************
**
** Function         bta_sco_set_route
**
** Description      Initialize bsa sco route that will be used for the next sco connection.
**                          HCI_BRCM_SCO_ROUTE_HCI or HCI_BRCM_SCO_ROUTE_PCM
**
** Returns          0 if successful, different than 0 if assignment failed
**
*******************************************************************************/
tBTA_STATUS bta_sco_set_route(UINT8 sco_route)
{
    APPL_TRACE_DEBUG1("bta_sco_set_route route : %d", sco_route);
#if defined(BTM_SCO_HCI_INCLUDED) && (BTM_SCO_HCI_INCLUDED == TRUE )
    bta_sco_codec_cb.cur_sco_route = sco_route;
#else
    if (sco_route == HCI_BRCM_SCO_ROUTE_HCI)
    {
        APPL_TRACE_ERROR0("bta_sco_set_route() : HCI routing not supported");
        return BTA_ERROR_SRV_NOT_COMPILED;
    }
#endif
    return BTA_SUCCESS;
}
/* the rest of this file is only compiled if HCI is supported */
#if defined(BTM_SCO_HCI_INCLUDED) && (BTM_SCO_HCI_INCLUDED == TRUE )
/*******************************************************************************
**
** Function         bta_sco_get_route
**
** Description      return bsa sco route that will be used for the next sco connection.
**
**
** Returns          sco route HCI_BRCM_SCO_ROUTE_HCI or HCI_BRCM_SCO_ROUTE_PCM
**
*******************************************************************************/
UINT8 bta_sco_get_route(void)
{
    APPL_TRACE_DEBUG0("bta_sco_get_route()");
    //return bta_sco_codec_cb.cur_sco_route;
    return HCI_BRCM_SCO_ROUTE_HCI;
}

/*******************************************************************************
**
** Function         bta_sco_codec_open
**
** Description      Open the bsa codec service.  Initialize control block
**                  variables and SBC encoder.  Open the waveIn interface and
**                  start the Windows thread that handles data.
**
** Returns          void
**
*******************************************************************************/
void bta_sco_codec_open(tBTA_SCO_CODEC_CFG *p_cfg)
{
    APPL_TRACE_DEBUG0("bta_sco_codec_open()");

    /* initialize control block */
    bta_sco_codec_cb.pool_id = p_cfg->pool_id;
    bta_sco_codec_cb.p_cback = p_cfg->p_cback;
    bta_sco_codec_cb.cb_event = p_cfg->cb_event;
    bta_sco_codec_cb.sco_handle = p_cfg->sco_handle;
    GKI_init_q(&bta_sco_codec_cb.out_q);

    AUDIO_SCO_Open(btm_cb.sco_cb.codec_in_use);
}

/*******************************************************************************
**
** Function         bta_sco_codec_close
**
** Description      Close the bsa codec service.
**
** Returns          void
**
*******************************************************************************/
void bta_sco_codec_close()
{
    APPL_TRACE_DEBUG0("bta_sco_codec_close()");

    /* Flush the SCO queue */
    while (GKI_IS_QUEUE_EMPTY(&bta_sco_codec_cb.out_q) == FALSE)
    {
        GKI_freebuf(GKI_dequeue(&bta_sco_codec_cb.out_q));
    }

    AUDIO_SCO_Close();
}
/*******************************************************************************
**
** Function         bta_sco_codec_readbuf
**
** Description      Read the next packet buffer.  The buffer contains one or
**                  more SBC frames.  The number of frames is in the
**                  layer_specific element of the BT_HDR.
**
** Returns          Pointer to buffer or NULL if no buffer available.
**
*******************************************************************************/
void bta_sco_codec_readbuf(BT_HDR **p_p_buf)
{
    *p_p_buf = (BT_HDR *) GKI_dequeue(&bta_sco_codec_cb.out_q);

    /* when Queue is not empty, send callback event */
    if (!GKI_IS_QUEUE_EMPTY(&bta_sco_codec_cb.out_q))
    {
        (*bta_sco_codec_cb.p_cback)(bta_sco_codec_cb.cb_event, bta_sco_codec_cb.sco_handle);
    }
}

/*******************************************************************************
**
** Function         bta_sco_data_cback
**
** Description      Callback function for file reading sco data. This
**                  function read audio data from local audio file, and does the
**                  sampling rate down conversion if needed.
**                  Everytime when called, this function reads
**                  BTA_SCO_BYTES_PER_SEG_MAX bytes PCM samples in 8k/16bits format.
**
** Returns          void.
**
*******************************************************************************/
void bta_sco_data_cback (UINT8 len)

{
    UINT16  cpy_len;
    UINT8   *p_target_sco_buf;
    int read_len;

    /* pack into GKI buffer queue */
    while (len > 0)
    {
        APPL_TRACE_ERROR1("bta_sco_data_cback len %d", len);
        if (bta_sco_codec_cb.p_buf == NULL)
        {
            if ((bta_sco_codec_cb.p_buf = (BT_HDR *) GKI_getpoolbuf(bta_sco_codec_cb.pool_id)) == NULL)
            {
                APPL_TRACE_ERROR0("Can not allocate SCO buffer.");
                return;
            }
            else
            {
                memset(bta_sco_codec_cb.p_buf+1, 0, BTA_SCO_CODEC_BUF_MAX+3); /* fill up with silence */
                bta_sco_codec_cb.p_buf->offset = 3;
                bta_sco_codec_cb.p_buf->len = 0;
            }
        }

        p_target_sco_buf = (UINT8 *)(bta_sco_codec_cb.p_buf + 1) + bta_sco_codec_cb.p_buf->len + bta_sco_codec_cb.p_buf->offset;
        cpy_len = ((UINT16)(BTA_SCO_CODEC_BUF_MAX - bta_sco_codec_cb.p_buf->len) > len) ? len :\
                    BTA_SCO_CODEC_BUF_MAX - bta_sco_codec_cb.p_buf->len;

        read_len = AUDIO_SCO_Out_Data(p_target_sco_buf, cpy_len);
        if(read_len < 0)
        {
            APPL_TRACE_ERROR1("warning bta_sco_data_cback len error: %d", read_len);
            read_len = 0;
        }
        if(cpy_len > (UINT16)read_len)
        {
            APPL_TRACE_ERROR0("warning bta_sco_data_cback underrun");
        }

        bta_sco_codec_cb.p_buf->len += cpy_len;

        if (bta_sco_codec_cb.p_buf->len == BTA_SCO_CODEC_BUF_MAX)
        {
            GKI_enqueue(&bta_sco_codec_cb.out_q, bta_sco_codec_cb.p_buf);
            bta_sco_codec_cb.p_buf = NULL;
        }
        len -= cpy_len;
    }

    /* when Queue is not empty, send callback event */
    if (!GKI_IS_QUEUE_EMPTY(&bta_sco_codec_cb.out_q))
    {
        (*bta_sco_codec_cb.p_cback)(bta_sco_codec_cb.cb_event, bta_sco_codec_cb.sco_handle);
    }
}
#endif
