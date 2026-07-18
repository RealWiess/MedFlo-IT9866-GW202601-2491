/*****************************************************************************
 **
 **  Name:           bta_media_sco.h
 **
 **  Description:    Interface file to the example codec implementation used
 **                  by BTA advanced audio.  The implementation uses the
 **                  Windows waveIn API to get PCM samples.
 **
 **  Copyright (c) 2010-2012, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/
#ifndef BTA_MEDIA_SCO_H
#define BTA_MEDIA_SCO_H

#include "bta_api.h"
#include "bta_sys.h"

/*****************************************************************************
**  Data types
*****************************************************************************/

/* application callback */
typedef void (tBTA_SCO_CODEC_CBACK)(UINT16 event, UINT16 sco_handle);

/* configuration structure passed to bta_sco_codec_open */
typedef struct
{
    tBTA_SCO_CODEC_CBACK   *p_cback;       /* application callback */
    UINT16              offset;         /* GKI buffer offset */
    UINT16              cb_event;
    UINT16              sco_handle;
    UINT8               pkt_size;            /* output packet mtu in bytes */
    UINT8               pool_id;        /* GKI pool id for output packet buffers */
} tBTA_SCO_CODEC_CFG;

/*****************************************************************************
 **  External Function Declarations
 *****************************************************************************/

void bta_sco_data_cback(UINT8 len);

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
tBTA_STATUS bta_sco_set_route(UINT8 sco_route);

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
UINT8 bta_sco_get_route(void);

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
void bta_sco_codec_open(tBTA_SCO_CODEC_CFG *p_cfg);

/*******************************************************************************
 **
 ** Function         bta_sco_codec_close
 **
 ** Description      Close the bsa codec service.
 **
 ** Returns          void
 **
 *******************************************************************************/
void bta_sco_codec_close(void);

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
void bta_sco_codec_readbuf(BT_HDR **p_p_buf);


#endif /* BTA_SCO_CODEC_H */
