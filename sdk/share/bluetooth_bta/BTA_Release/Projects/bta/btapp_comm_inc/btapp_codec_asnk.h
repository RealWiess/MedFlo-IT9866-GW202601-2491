/*****************************************************************************
**
**  Name:           btapp_codec_asnk.h
**
**  Description:    Interface file to the example codec implementation used
**                  by BTA advanced audio.  The implementation uses the
**                  Windows waveIn API to get PCM samples as input to the
**                  SBC encoder.
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
*****************************************************************************/

#ifndef BTAPP_CODEC_ASNK_H
#define BTAPP_CODEC_ASNK_H
#include "a2d_sbc.h"
#include "ucodec.h"

/*****************************************************************************
**  Constants
*****************************************************************************/

/* callback event */
#define BTAPP_CODEC_RX_READY         1

#ifndef BTAPP_CODEC_PLAYED_LOW_WM
#define BTAPP_CODEC_PLAYED_LOW_WM        (10)
#endif

#ifndef BTAPP_CODEC_PLAYED_HIGH_WM
#define BTAPP_CODEC_PLAYED_HIGH_WM       (40)
#endif

#define WOM_DONE_EVENT      APPL_EVT_1
#define WIM_DATA_EVENT      APPL_EVT_2

/*****************************************************************************
**  Data types
*****************************************************************************/

/* application callback */
typedef void (tBTAPP_CODEC_CBACK)(UINT8 event);

/* configuration structure passed to btapp_codec_open */
typedef struct
{
    tA2D_SBC_CIE        sbc_cie;        /* SBC encoder configuration */
    tBTAPP_CODEC_CBACK   *p_cback;       /* application callback */
    UINT16              bit_rate;       /* SBC encoder bit rate in kbps */
    UINT16              bit_rate_busy;  /* SBC encoder bit rate in kbps */
    UINT16              bit_rate_swampd;/* SBC encoder bit rate in kbps */
    UINT8               pool_id;        /* GKI pool id for output packet buffers */
    UINT16              offset;         /* GKI buffer offset */
    UINT8               pkt_q_max;      /* output packet queue max */
    UINT16              mtu;            /* output packet mtu in bytes */
} tBTAPP_CODEC_CFG;

/*******************************************************************************
**
** Function         btapp_avk_codec_init
**
** Description      Initialize btapp codec service.  This function just prints
**                  out debug messages displaying the available audio devices
**                  available on the PC.
**
** Returns          void
**
*******************************************************************************/
extern void btapp_avk_codec_init(void);

/*******************************************************************************
**
** Function         btapp_avk_codec_update_busy_level
**
** Description      update the busy level in the codec control block.
**
** Returns          void
**
*******************************************************************************/
extern void btapp_avk_codec_update_busy_level(UINT8 busy);

/*******************************************************************************
**
** Function         btapp_avk_codec_open
**
** Description      Open the btapp codec service.  Initialize control block
**                  variables and SBC encoder.  Open the waveIn interface and
**                  start the Windows thread that handles data.
**
** Returns          void
**
*******************************************************************************/
extern void btapp_avk_codec_open(void);

/*******************************************************************************
**
** Function         btapp_avk_codec_start
**
** Description      Start the btapp codec service.  This initializes the waveIn
**                  pcm data buffers and starts reading pcm samples from the
**                  audio device.
**
** Returns          void
**
*******************************************************************************/
extern tUCODEC_STATUS btapp_avk_codec_start(void);

/*******************************************************************************
**
** Function         btapp_avk_codec_stop
**
** Description      Stop the btapp codec service.  This resets the waveIn
**                  pcm data buffers and stops reading pcm samples from the
**                  audio device.
**
** Returns          void
**
*******************************************************************************/
extern void btapp_avk_codec_stop(void);

/*******************************************************************************
**
** Function         btapp_avk_codec_close
**
** Description      Close the btapp codec service.  Stop the codec, then close
**                  the waveIn interface, then stop the Windows thread.
**
** Returns          void
**
*******************************************************************************/
extern void btapp_avk_codec_close(void);

/*******************************************************************************
**
** Function         btapp_avk_codec_readbuf
**
** Description      Read the next packet buffer.  The buffer contains one or
**                  more SBC frames.  The number of frames is in the
**                  layer_specific element of the BT_HDR.
**
** Returns          Pointer to buffer or NULL if no buffer available.
**
*******************************************************************************/
extern void btapp_avk_codec_readbuf(BT_HDR **p_p_buf);

/*******************************************************************************
**
** Function         btapp_avk_codec_get_cur_codec_info
**
** Description      get the current codec_info
**
** Returns          the current codec_info.
**
*******************************************************************************/
extern UINT8 * btapp_avk_codec_get_cur_codec_info(void);

/*******************************************************************************
**
** Function         btapp_avk_codec_write_sbc
**
** Description      write stream data buffer using SBC codec
**
** Returns          the current sep_info_idx.
**
*******************************************************************************/
extern tUCODEC_STATUS btapp_avk_codec_write_sbc(BT_HDR *p_pkt, UINT32 timestamp, UINT16 seq_num);
//extern tUCODEC_STATUS btapp_avk_codec_write_m12(BT_HDR *p_pkt, UINT32 timestamp, UINT16 seq_num);
//extern tUCODEC_STATUS btapp_avk_codec_write_m24(BT_HDR *p_pkt, UINT32 timestamp, UINT16 seq_num);
extern void btapp_avk_codec_sbc_cnf(UINT8 *p_codec_info, BOOLEAN to_callin);
extern void btapp_avk_codec_m12_cnf(UINT8 *p_codec_info);
extern void btapp_avk_codec_m24_cnf(UINT8 *p_codec_info);
extern tUCODEC_STATUS btapp_avk_codec_configure  (tUCODEC_CNF *p_config);

extern void btapp_avk_codec_video_cnf(tBTA_AVK_CODEC codec_type, UINT8 *p_codec_info);
extern tVDP_STATUS btapp_avk_codec_video_vend_cnf(UINT8 *p_codec_info);
extern void btapp_avk_codec_video_open(void);
extern void btapp_avk_codec_video_write(UINT8 *p_media, UINT32 media_len, UINT32 timestamp, UINT16 seq_num);

#endif /* BTAPP_CODEC_ASNK_H */
