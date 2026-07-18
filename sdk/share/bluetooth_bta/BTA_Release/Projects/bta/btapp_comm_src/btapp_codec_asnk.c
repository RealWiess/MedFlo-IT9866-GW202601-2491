/*****************************************************************************
**
**  Name:          btapp_codec_asnk.c
**
**  Description:
**  This file contains the utility implementation used
**  for BTA advanced audio/video sink.
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bta_platform.h"
#include "bte_glue.h"

#if ((defined BTA_AVK_INCLUDED) && (BTA_AVK_INCLUDED == TRUE))
/*******************************************************************************
** Constants
*******************************************************************************/
#define BTAPP_AVK_DECODE_DATA               1

#define BTAPP_DEVICE_DB_NV_ID               2
/* volume change(up or down) step */
#define BTAPP_AVK_VOL_CHANGE_STEP           3
#define BTAPP_AVK_DEFAULT_VOLUME            65

#define BTAPP_AVK_AUDIO_PLAY_READY          0x0001
#define BTAPP_AVK_AUDIO_PLAY_I2S_DMA_DONE   0x0002
#define BTAPP_AVK_AUDIO_PLAY_STOP           0x0003
#define BTAPP_AVK_AUDIO_PLAY_SHUTDOWN       0x0100

#define BTAPP_AVK_AUDIO_DECODEC_DATA        0x0001
#define BTAPP_AVK_AUDIO_DECODEC_RESUME      0x0002
#define BTAPP_AVK_AUDIO_DECODEC_SHUTDOWN    0x0100

#define BTAPP_AVK_PLAY_LOW_TH               24
#define BTAPP_AVK_PLAY_HIGH_TH              28

#ifndef UCODEC_PCM_BUFFER_POOL
#define UCODEC_PCM_BUFFER_POOL              (GKI_POOL_ID_4)
#endif

/* Codec control block */
tUCODEC_INST_CB ucodec_cb;

/* Current codec */
tBTA_AVK_CODEC btapp_avk_codec_audio =      BTA_AVK_CODEC_SBC;

/* For compiling btapp_avk.c for both btapp_app and btapp_avsnk. */
tBTAPP_AVK_DB    btapp_avk_db;

typedef struct
{
    UINT8      PlayStarted;
}tBTAPP_AVK_PLAY_CB;
tBTAPP_AVK_PLAY_CB btapp_avk_play_cb;

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
void btapp_avk_codec_init(void)
{
    APPL_TRACE_DEBUG0("btapp_avk_codec_init");
    UCODEC_Init(NULL);
}

/******************************************************************************
**
** Function         btapp_avk_codec_configure
**
** Description      Initialise the CODEC for a particular stream.
**
**
**                  Input : CodecId: Id of the codec to perform the operation on.
**                          CbackPrt: Call back pointer for codec feedback.
**                          p_config: Pointer on a codec configuration structure.
**
**                  Output Parameters : None
**
** Returns          UCODEC_SUCCESS if The action was performed with sucess.
**
******************************************************************************/
tUCODEC_STATUS    btapp_avk_codec_configure  (tUCODEC_CNF *p_config)
{
    APPL_TRACE_DEBUG0("btapp_avk_codec_configure  ");
    UCODEC_Configure(UCODEC_ID_1, NULL, p_config);

    return UCODEC_SUCCESS;
}

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
void btapp_avk_codec_open(void)
{
    APPL_TRACE_DEBUG0("******** btapp_avk_codec_open ********");
    UCODEC_Open(UCODEC_ID_1);
}

/*******************************************************************************
**
** Function         btapp_avk_codec_start
**
** Description      Start the btapp codec service.  This initializes the waveOut
**                  pcm data buffers and starts reading pcm samples to the
**                  audio device.
**
** Returns          void
**
*******************************************************************************/
tUCODEC_STATUS btapp_avk_codec_start(void)
{
    APPL_TRACE_DEBUG0("******** btapp_avk_codec_start ********");
    return UCODEC_Start(UCODEC_ID_1);
}

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
void btapp_avk_codec_stop(void)
{
    APPL_TRACE_DEBUG0("******** btapp_avk_codec_stop ********");
    UCODEC_Stop(UCODEC_ID_1);
    GKI_send_event(AUDIO_PLAY_TASK, BTAPP_AVK_AUDIO_PLAY_STOP);
}

/*******************************************************************************
**
** Function         btapp_avk_codec_close
**
** Description      Close the btapp codec service.  Stop the codec, then close
**                  the waveOut interface, then stop the Windows thread.
**
** Returns          void
**
*******************************************************************************/
void btapp_avk_codec_close(void)
{
    APPL_TRACE_DEBUG0("******** btapp_avk_codec_close ********");
    UCODEC_Close(UCODEC_ID_1);
    GKI_send_event(AUDIO_PLAY_TASK, BTAPP_AVK_AUDIO_PLAY_STOP);
}

/*******************************************************************************
**
** Function         btapp_avk_set_the_vol
**
** Description      Set the codec volume.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_avk_set_the_vol(UINT8 volume)
{
    tUCODEC_CNF CodecCnf;

    btapp_ak_cb.cur_volume = volume;

    /* pass the current volume to the CODEC */
    CodecCnf.MediaType = UCODEC_MEDIA_TYPE_AUDIO;
    CodecCnf.Type.AudioType = UCODEC_AUDIO_VOLUME;
    CodecCnf.Feature.Volume = volume;

    /* set the volume of the codec  */
    UCODEC_Configure(UCODEC_ID_1, NULL, &CodecCnf);
}

/*******************************************************************************
**
** Function         btapp_avk_codec_write_sbc
**
** Description      write buffer
**
** Returns
**
*******************************************************************************/
tUCODEC_STATUS btapp_avk_codec_write_sbc(BT_HDR *p_pkt, UINT32 timestamp, UINT16 seq_num)
{
    p_pkt->len -= A2D_SBC_MPL_HDR_LEN;
    p_pkt->offset += A2D_SBC_MPL_HDR_LEN;

    /*
    APPL_TRACE_DEBUG4("btapp_avk_codec_write_sbc len:%d offset:%d timestamp:%d, seq_num:%d",
                      p_pkt->len, p_pkt->offset, timestamp, seq_num);
    */

    p_pkt->event = BTAPP_AVK_AUDIO_DECODEC_DATA;
    GKI_send_msg(UCODEC_TASK, TASK_MBOX_0, p_pkt);
    return UCODEC_SUCCESS;
}

tUCODEC_STATUS btapp_avk_codec_write_m12(BT_HDR *p_pkt)
{
    GKI_freebuf(p_pkt);
    return UCODEC_SUCCESS;
}

tUCODEC_STATUS btapp_avk_codec_write_m24(BT_HDR *p_pkt)
{
    GKI_freebuf(p_pkt);
    return UCODEC_SUCCESS;
}

/*******************************************************************************
**
** Function         btapp_avk_codec_sbc_cnf
**
** Description      This function configure the codec in sbc format.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_avk_codec_sbc_cnf(UINT8 *p_codec_info, BOOLEAN to_callin)
{
    tUCODEC_CNF CodecCnf;

    APPL_TRACE_DEBUG6("btapp_avk_codec_sbc_cnf: 0x%02X;0x%02X;0x%02X;0x%02X;0x%02X;0x%02X",
        *(p_codec_info+1), *(p_codec_info+2), *(p_codec_info+3), *(p_codec_info+4), *(p_codec_info+5), *(p_codec_info+6));

    /* Codec configured */
    btapp_avk_codec_audio = BTA_AVK_CODEC_SBC;

    if (to_callin)
        bta_avk_ci_setconfig(BTA_AVK_CHNL_AUDIO, A2D_SUCCESS, AVDT_ASC_CODEC);

    CodecCnf.MediaType = UCODEC_MEDIA_TYPE_AUDIO;
    CodecCnf.Type.AudioType = UCODEC_AUDIO_SBC;

    p_codec_info += 3;  /* skip the length, the media type and the codec type byte */

    /* Sample frequency configuration */
    switch((*p_codec_info) & A2D_SBC_IE_SAMP_FREQ_MSK)
    {
        case A2D_SBC_IE_SAMP_FREQ_16 :
            CodecCnf.Feature.SBCConfig.SampleFreq = UCODEC_SBC_SMP_FREQ_16;
            break;
        case A2D_SBC_IE_SAMP_FREQ_32 :
            CodecCnf.Feature.SBCConfig.SampleFreq = UCODEC_SBC_SMP_FREQ_32;
            break;
        case A2D_SBC_IE_SAMP_FREQ_44:
            CodecCnf.Feature.SBCConfig.SampleFreq = UCODEC_SBC_SMP_FREQ_44;
            break;
        case A2D_SBC_IE_SAMP_FREQ_48:
            CodecCnf.Feature.SBCConfig.SampleFreq = UCODEC_SBC_SMP_FREQ_48;
            break;
        default :
            APPL_TRACE_ERROR0("ERROR: wrong sample freq for config");
            break;
    }

    /*  Chanel mode configuration */
    switch((*p_codec_info) & A2D_SBC_IE_CH_MD_MSK)
    {
        case A2D_SBC_IE_CH_MD_MONO   :
            CodecCnf.Feature.SBCConfig.ChannelMode = UCODEC_CHN_MONO;
            break;
        case A2D_SBC_IE_CH_MD_DUAL   :
            CodecCnf.Feature.SBCConfig.ChannelMode = UCODEC_CHN_DUAL;
            break;
        case A2D_SBC_IE_CH_MD_STEREO :
            CodecCnf.Feature.SBCConfig.ChannelMode = UCODEC_CHN_STEREO;
            break;
        case A2D_SBC_IE_CH_MD_JOINT  :
            CodecCnf.Feature.SBCConfig.ChannelMode = UCODEC_CHN_JOINT_STEREO;
            break;
        default :
            APPL_TRACE_ERROR0("ERROR: wrong ChannelMode for config");
            break;
    }

    p_codec_info++;
    /* block length */
    switch((*p_codec_info) & A2D_SBC_IE_BLOCKS_MSK)
    {

        case A2D_SBC_IE_BLOCKS_4 :
            CodecCnf.Feature.SBCConfig.NumBlock = 4;
            break;
        case A2D_SBC_IE_BLOCKS_8 :
            CodecCnf.Feature.SBCConfig.NumBlock = 8;
            break;
        case A2D_SBC_IE_BLOCKS_12:
            CodecCnf.Feature.SBCConfig.NumBlock = 12;
            break;
        case A2D_SBC_IE_BLOCKS_16:
            CodecCnf.Feature.SBCConfig.NumBlock = 16;
            break;
        default :
            APPL_TRACE_ERROR0("ERROR: wrong block len for config");
            break;
    }

    /* subband */
    switch((*p_codec_info) & A2D_SBC_IE_SUBBAND_MSK)
    {
        case A2D_SBC_IE_SUBBAND_4   :
            CodecCnf.Feature.SBCConfig.Subband = UCODEC_SBC_SUBBAND_4;
            break;
        case A2D_SBC_IE_SUBBAND_8   :
            CodecCnf.Feature.SBCConfig.Subband = UCODEC_SBC_SUBBAND_8;
            break;
        default :
            APPL_TRACE_ERROR0("ERROR: wrong subband");
            break;
    }

    /* Allocation method */
    switch((*p_codec_info) & A2D_SBC_IE_ALLOC_MD_MSK)
    {
        case A2D_SBC_IE_ALLOC_MD_S   :
            CodecCnf.Feature.SBCConfig.AllocMthd = UCODEC_SBC_ALLOC_MD_S;
            break;
        case A2D_SBC_IE_ALLOC_MD_L   :
            CodecCnf.Feature.SBCConfig.AllocMthd = UCODEC_SBC_ALLOC_MD_L;
            break;
        default :
            APPL_TRACE_ERROR0("ERROR: wrong subband");
            break;
    }

    CodecCnf.Feature.SBCConfig.MaxBitPool = *++p_codec_info;
    CodecCnf.Feature.SBCConfig.MinBitPool = *++p_codec_info;

    UCODEC_Configure(UCODEC_ID_1, NULL, &CodecCnf);

    /* set the default volume */
    btapp_avk_set_the_vol(BTAPP_AVK_DEFAULT_VOLUME);
}

/*******************************************************************************
**
** Function         btapp_avk_codec_m12_cnf
**
** Description      This function configure the codec in m12 format.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_avk_codec_m12_cnf(UINT8 *pcodec_info)
{
    btapp_avk_codec_audio = BTA_AVK_CODEC_M12;

    /* Codec configured */
    bta_avk_ci_setconfig(BTA_AVK_CHNL_AUDIO, A2D_SUCCESS, AVDT_ASC_CODEC);
}

/*******************************************************************************
**
** Function         btapp_avk_codec_m24_cnf
**
** Description      This function configure the codec in m24 format.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_avk_codec_m24_cnf(UINT8 *p_codec_info)
{
    btapp_avk_codec_audio = BTA_AVK_CODEC_M24;

    /* Codec configured */
    bta_avk_ci_setconfig(BTA_AVK_CHNL_AUDIO, A2D_SUCCESS, AVDT_ASC_CODEC);
}

/*******************************************************************************
**
** Function         btapp_platform_avk_remote_cmd
**
** Description      handle remote control command
**
** Returns
**
*******************************************************************************/
void btapp_platform_avk_remote_cmd(tBTA_AVK_RC rc_id, tBTA_AVK_STATE key_state)
{
    APPL_TRACE_DEBUG2("btapp_platform_avk_remote_cmd: rc_id:%d  key_state:%d",
                      rc_id, key_state);
    if(key_state == BTA_AVK_STATE_RELEASE)
    {
        /* no need to handle Key_Release??? */
        APPL_TRACE_DEBUG0("Don't handle Key Release event");
        return;
    }
    switch(rc_id)
    {
        case BTA_AVK_RC_VOL_UP:
            if(btapp_ak_cb.cur_volume > (0xFF - BTAPP_AVK_VOL_CHANGE_STEP))
                btapp_avk_set_the_vol(0xFF);
            else
                btapp_avk_set_the_vol((UINT8)(btapp_ak_cb.cur_volume + BTAPP_AVK_VOL_CHANGE_STEP));
            break;

        case BTA_AVK_RC_VOL_DOWN:
            if(btapp_ak_cb.cur_volume > BTAPP_AVK_VOL_CHANGE_STEP)
                btapp_avk_set_the_vol((UINT8)(btapp_ak_cb.cur_volume - BTAPP_AVK_VOL_CHANGE_STEP));
            break;
    }
}

/******************************************************************************
**
** Function         ucodec_alloc_buffer
**
** Description      SBC codec pcm buffer allocation call back.
**
**
** Returns          Amount of data pointed by pPacket.
**
******************************************************************************/
UINT16 ucodec_alloc_buffer(SINT16 **ppSbcBuffer, UINT16 Length)
{
    /* Provide a pcm buffer if needed */
    if(ucodec_cb.pCurrentPcmBuf == NULL)
    {
        ucodec_cb.pCurrentPcmBuf = (SINT16 *) GKI_getpoolbuf(UCODEC_PCM_BUFFER_POOL);
        ucodec_cb.PcmOffset = 0;
        /*
        DRV_TRACE_DEBUG4("ucodec_alloc_buffer pCurrentPcmBuf 0x%x, Length%d, PCMOffset %d, ReaminingBytes %d", \
            ucodec_cb.pCurrentPcmBuf, Length, ucodec_cb.PcmOffset, ucodec_cb.DecParams.u16ReaminingBytes);
            */
    }

    if(ucodec_cb.pCurrentPcmBuf == NULL)
    {
        DRV_TRACE_ERROR1("Error in ucodec_alloc_buffer->no GKI buffer for PCM Txcount %d", ucodec_cb.TxPCMQueue.count);
        return 0;
    }

    if((GKI_get_buf_size(ucodec_cb.pCurrentPcmBuf) - ucodec_cb.PcmOffset) < Length)
    {
        DRV_TRACE_ERROR3("ucodec_alloc_buffer -> buffer too small: BufSize %d, Offset %d, len %d", GKI_get_buf_size(ucodec_cb.pCurrentPcmBuf), ucodec_cb.PcmOffset, Length);
        return 0;
    }

    *ppSbcBuffer = ucodec_cb.pCurrentPcmBuf + (ucodec_cb.PcmOffset/sizeof(SINT16));

    return GKI_get_buf_size(ucodec_cb.pCurrentPcmBuf) - ucodec_cb.PcmOffset;
}

/******************************************************************************
**
** Function         ucodec_dump_buffer
**
** Description      SBC codec output data call back.
**
**                  Input : u16NumberOfBytes number of byte pointed by pPacket.
**                          pPacket: buffer to be sent out.
**
**                  Output Parameters : None
**
** Returns          SBC_SUCCESS if data were output properly
**                  SBC_FAILURE if data were not ouput properly.
**
******************************************************************************/
UINT16 ucodec_dump_buffer(SINT16 *PacketPtr, UINT16 u16NumberOfPcm)
{
    BT_HDR* p_msg;
    BT_HDR* p_msg_sent;
    UINT16  evt = 0;
    UINT8 st = SBC_SUCCESS;

    if(ucodec_cb.CurrentRxStream == UCODEC_STATE_SBC_SYNK || ucodec_cb.CurrentRxStream == UCODEC_STATE_SBC_NOT_SYNK)
    {
        if(ucodec_cb.PlayedQueue.count > BTAPP_AVK_PLAY_HIGH_TH)
        {
            ucodec_cb.PCMFlowOn = 1;
            //If the PlayQueue count larger than BTAPP_AVK_PLAY_HIGH_TH
            evt = GKI_wait(0xFFFF, 0);
            if(evt & TASK_MBOX_0_EVT_MASK)
            {
                while ((p_msg = (BT_HDR *) GKI_read_mbox (TASK_MBOX_0)) != NULL)
                {
                    if(p_msg->event == BTAPP_AVK_AUDIO_DECODEC_SHUTDOWN)
                    {
                        st = SBC_FAILURE;
                        ucodec_cb.PCMFlowOn = 0;
                    }

                    GKI_freebuf(p_msg);
                }
            }
        }

        if(st == SBC_SUCCESS)
        {
            //p_msg = GKI_getbuf(u16NumberOfPcm*sizeof(SINT16) + BT_HDR_SIZE);
            p_msg = GKI_getpoolbuf(UCODEC_PCM_BUFFER_POOL);
            if(p_msg == NULL)
            {//This is fatal error, need to trap
                DRV_TRACE_ERROR0("ucodec_dump_buffer can't alloc play buffer");
            }
            else
            {
                memcpy((UINT8*)(p_msg+1), (UINT8*)PacketPtr, u16NumberOfPcm*sizeof(SINT16));

                p_msg->len = u16NumberOfPcm*sizeof(SINT16);
                p_msg->offset = 0;
                p_msg->layer_specific = 0;

                GKI_enqueue(&ucodec_cb.PlayedQueue, p_msg);             //insert the decoded data into to the playQueue

                //Notify the audio play task to consume the decoded data.
                if ((p_msg_sent = (BT_HDR *)GKI_getbuf(BT_HDR_SIZE)) != NULL)
                {
                    p_msg_sent->event = BTAPP_AVK_AUDIO_PLAY_READY;
                    GKI_send_msg (AUDIO_PLAY_TASK, TASK_MBOX_0, p_msg_sent);
                }
                else
                {//This is fatal error, need to trap
                    GKI_freebuf(GKI_dequeue(&ucodec_cb.PlayedQueue));
                    DRV_TRACE_ERROR0("ucodec_dump_buffer can't alloc msg buffer");
                }
            }
        }
    }
    else
    {//Just ingore the decoded SBC data.

    }

    return st;
}

/******************************************************************************
**
** Function         ucodec_fill_buffer
**
** Description      codec input call back.
**
**                  Input : pBuf: Pointer onto the GKI buffer to be send to the CODEC.
**
**                  Output Parameters : pPacket: pointer in the input data to be decoded
**
** Returns          Amount of data pointed by pPacket.
**
******************************************************************************/
UINT16 ucodec_fill_buffer(UINT8 *pPacket, UINT16 u16NumberOfBytes)
{
    BT_HDR  *pBuf;
    UINT16  length;
    UINT8   *pp;
    BT_HDR* p_msg;

    if ((u16NumberOfBytes == 0) || ((pBuf = (BT_HDR *) GKI_getfirst(&(ucodec_cb.DecodeQueue))) == NULL))
    {
//      DRV_TRACE_ERROR1("ucodec_fill_buffer no more buffer to decode count %d", ucodec_cb.DecodeQueue.count);

        return 0;
    }

    /* Make sure we do not give more data than expected */
    if (pBuf->len < u16NumberOfBytes)
        length = pBuf->len;
    else
        length = u16NumberOfBytes;

    /* Read Data from the input stream */
    pp = ((UINT8 *)(pBuf+1))+pBuf->offset;
    memcpy (pPacket, pp, length);

    /* update buffer pointer */
    pBuf->len -= length;
    pBuf->offset += length;

//    DRV_TRACE_ERROR2("ucodec_fill_buffer pkt:0x%08x len:%d ", pPacket, length);

    if (pBuf->len == 0)
    {
        p_msg = GKI_dequeue(&(ucodec_cb.DecodeQueue));
//        DRV_TRACE_ERROR0("Dequeue ucodec_cb.DecodeQueue ");
        if(p_msg != NULL)
            GKI_freebuf(p_msg);
    }

    return(length);
}

/******************************************************************************
**
** Function         ucodec_write_sbc_not_synked
**
** Description      Decode and send a buffer to the codec.
**
**                  Input : pBuf: Pointer onto the GKI buffer to be send to the CODEC.
**
**                  Output Parameters : None
**
** Returns          UCODEC_SUCCESS if The action was performed with sucess.
**                  UCODEC_FLOW_CTRL_ON if The codec buffer Q had reach a UCODEC_HIGH_WM
**                                      watermark. The buffer is queued
**                  UCODEC_OVERFLOW if The codec buffer Q had reach a critical
**                                     watermark. The buffer is dropped.
**
******************************************************************************/
tUCODEC_STATUS  ucodec_write_sbc_not_synked (void)
{
    if (SBC_Decoder_Init(&(ucodec_cb.DecParams)) == SBC_FAILURE)
    {
        DRV_TRACE_ERROR0("ucodec_write_sbc_not_synked failed");
        return UCODEC_GENERIC_ERROR;
    }

    ucodec_cb.CurrentRxStream = UCODEC_STATE_SBC_SYNK;

    return UCODEC_SUCCESS;
}

/******************************************************************************
**
** Function         ucodec_write_sbc
**
** Description      Decode and send an SBC buffer to the codec.
**
**                  Input : pBuf: Pointer onto the GKI buffer to be send to the CODEC.
**
**                  Output Parameters : None
**
** Returns          UCODEC_SUCCESS if The action was performed with sucess.
**                  UCODEC_FLOW_CTRL_ON if The codec buffer Q had reach a UCODEC_HIGH_WM
**                                      watermark. The buffer is queued
**                  UCODEC_OVERFLOW if The codec buffer Q had reach a critical
**                                     watermark. The buffer is dropped.
**
******************************************************************************/
tUCODEC_STATUS ucodec_write_sbc (void)
{
    do
    {
        if (SBC_Decoder(&(ucodec_cb.DecParams)) == SBC_FAILURE)
        {
            DRV_TRACE_ERROR0("ucodec_write_sbc failed");
            return UCODEC_GENERIC_ERROR;
        }

    } while (ucodec_cb.DecodeQueue.count);

    return UCODEC_SUCCESS;
}

/*******************************************************************************
**
** Function         btapp_avk_ucodec_task
**
** Description      codec task to handler the codec pakect
**
** Returns
**
*******************************************************************************/
void btapp_avk_ucodec_task(UINT32 params)
{
    UINT16  event;
    BT_HDR  *p_msg;
    UINT8   need_exit = FALSE;
    tUCODEC_STATUS  st = UCODEC_SUCCESS;

    ucodec_cb.CurrentRxStream = UCODEC_STATE_SBC_NOT_SYNK;
    ucodec_cb.DecParams.s32ScartchMem = ucodec_cb.SBCScartchMem;
    ucodec_cb.DecParams.s32StaticMem  = ucodec_cb.SBCStaticMem;
    ucodec_cb.DecParams.AllocBufferFP = ucodec_alloc_buffer;
    ucodec_cb.DecParams.FillBufferFP  = ucodec_fill_buffer;
    ucodec_cb.DecParams.EmptyBufferFP = ucodec_dump_buffer;
    GKI_init_q(&ucodec_cb.DecodeQueue);
    GKI_init_q(&ucodec_cb.PlayedQueue);

    while(1)
    {
        event = GKI_wait (0xFFFF, 0);

        if (event & TASK_MBOX_0_EVT_MASK)
        {
            /* Process all messages in the queue */
            while ((p_msg = (BT_HDR *) GKI_read_mbox (TASK_MBOX_0)) != NULL)
            {
                switch(p_msg->event)
                {
                    case BTAPP_AVK_AUDIO_DECODEC_DATA:
#if BTAPP_AVK_DECODE_DATA
                        if(ucodec_cb.CurrentRxStream == UCODEC_STATE_SBC_NOT_SYNK)
                        {
                            GKI_enqueue(&ucodec_cb.DecodeQueue, p_msg);
                            st = ucodec_write_sbc_not_synked();
                            if(st != UCODEC_SUCCESS)
                            {
                                GKI_freebuf(GKI_dequeue(&ucodec_cb.DecodeQueue));
                            }
                        }
                        else if(ucodec_cb.CurrentRxStream == UCODEC_STATE_SBC_SYNK)
                        {
                            GKI_enqueue(&ucodec_cb.DecodeQueue, p_msg);
                            st = ucodec_write_sbc();
                            if(st != UCODEC_SUCCESS)
                            {
                                /* if ucodec_write_sbc not return success, from the low layer implemetion,
                                 * maybe, the low layer receive the shutdown notify when flow on
                                 */
                                GKI_freebuf(GKI_dequeue(&ucodec_cb.DecodeQueue));
                                need_exit = TRUE;
                            }
                        }
                        else
                        {
                            GKI_freebuf(p_msg);
                        }
#else
                        //Since the board doesn't contain audio codec, so just free the receive audio data.
                        GKI_freebuf(p_msg);
#endif
                        break;

                    case BTAPP_AVK_AUDIO_DECODEC_RESUME:
                        GKI_freebuf(p_msg);
                        break;

                    case BTAPP_AVK_AUDIO_DECODEC_SHUTDOWN:
                        need_exit = TRUE;
                        GKI_freebuf(p_msg);
                        break;

                    default:
                        APPL_TRACE_DEBUG1("btapp_avk_ucodec_task unexpected event:%d", p_msg->event);
                        GKI_freebuf(p_msg);
                        break;
                }
            }
        }
        else
        {
            APPL_TRACE_DEBUG1("btapp_avk_ucodec_task unexpected mailbox event received:%d", event);
        }

        if(need_exit == TRUE)
        {
            break;
        }
    }

    //Exit task
    APPL_TRACE_DEBUG0("btapp_avk_ucodec_task shutdown");
    GKI_exit_task(UCODEC_TASK);
}

/*******************************************************************************
**
** Function         btapp_avk_audio_play_task
**
** Description      audio play task to handler the decodec pakect to the audio component.
**
** Returns
**
*******************************************************************************/
void btapp_avk_audio_play_task(UINT32 params)
{
    UINT8  need_exit = FALSE;
    UINT16 event;
    BT_HDR* p_msg;
    BT_HDR* p_msg_sent;

    btapp_avk_play_cb.PlayStarted = FALSE;

    while(1)
    {
        event = GKI_wait(0xFFFF, 0);

        if(event & TASK_MBOX_0_EVT_MASK)
        {
            while ((p_msg = (BT_HDR *) GKI_read_mbox (TASK_MBOX_0)) != NULL)
            {
                switch(p_msg->event)
                {
                    case BTAPP_AVK_AUDIO_PLAY_READY:
                        btapp_avk_play_cb.PlayStarted = TRUE;
                        break;

                    case BTAPP_AVK_AUDIO_PLAY_I2S_DMA_DONE:
                        if(ucodec_cb.PlayedQueue.count != 0)
                        {
                            GKI_freebuf(GKI_dequeue(&ucodec_cb.PlayedQueue));
                        }
                        break;

                    case BTAPP_AVK_AUDIO_PLAY_STOP:
                        btapp_avk_play_cb.PlayStarted = FALSE;
                        ucodec_cb.CurrentRxStream = UCODEC_STATE_NOT_CNF;
                        break;

                    case BTAPP_AVK_AUDIO_PLAY_SHUTDOWN:
                        need_exit = TRUE;
                        break;

                    default:
                        APPL_TRACE_DEBUG1("btapp_avk_audio_play_task unexpected event:%d", p_msg->event);
                        break;
                }

                //Free p_msg immediately, since the p_msg not enqueue
                GKI_freebuf(p_msg);

                //During the read message, we can get the opportunity to handle
                if(btapp_avk_play_cb.PlayStarted == TRUE)
                {
                    if(UCODEC_IsOpened(UCODEC_ID_1) == TRUE)
                    {
                        while(ucodec_cb.PlayedQueue.count != 0)
                        {
                            //add for rk2108
                            p_msg = GKI_dequeue(&ucodec_cb.PlayedQueue); 
                            UCODEC_FillBuf(UCODEC_ID_1, (UINT8*)(p_msg + 1), p_msg->len);

                            GKI_freebuf(p_msg);
                        }
                    }
                    else
                    {
                        if(ucodec_cb.PlayedQueue.count != 0)
                        {
                            GKI_freebuf(GKI_dequeue(&ucodec_cb.PlayedQueue));
                        }
                    }
                }

                if(btapp_avk_play_cb.PlayStarted == FALSE)
                {
                    while(ucodec_cb.DecodeQueue.count)
                    {
                        GKI_freebuf(GKI_dequeue(&ucodec_cb.DecodeQueue));
                    }
                    while(ucodec_cb.PlayedQueue.count)
                    {
                        GKI_freebuf(GKI_dequeue(&ucodec_cb.PlayedQueue));
                    }
                }
            }
        }
        else
        {
            APPL_TRACE_DEBUG1("btapp_avk_audio_play_task unexpected mailbox event received:%d", event);
        }

        if(need_exit == TRUE)
        {
            break;
        }
    }

    //Exit task
    APPL_TRACE_DEBUG0("btapp_avk_audio_play_task shutdown");
    //If low layer ucodec is opened, need to close
    if(UCODEC_IsOpened(UCODEC_ID_1))
    {
        UCODEC_Stop(UCODEC_ID_1);
        UCODEC_Close(UCODEC_ID_1);
    }

    GKI_exit_task(AUDIO_PLAY_TASK);
}

void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
    BT_HDR* p_msg;
    if ((p_msg = (BT_HDR *)GKI_getbuf(BT_HDR_SIZE)) != NULL)
    {
        p_msg->event = BTAPP_AVK_AUDIO_PLAY_I2S_DMA_DONE;
        GKI_send_msg (AUDIO_PLAY_TASK, TASK_MBOX_0, p_msg);
    }
    else
    {//This is fatal error, need to trap, it's in the ISR context

    }
}

void btapp_avk_ucodec_shutdown(void)
{
    BT_HDR* p_msg;
    if ((p_msg = (BT_HDR *)GKI_getbuf(BT_HDR_SIZE)) != NULL)
    {
        p_msg->event = BTAPP_AVK_AUDIO_DECODEC_SHUTDOWN;
        GKI_send_msg (UCODEC_TASK, TASK_MBOX_0, p_msg);
    }
    else
    {
        GKI_TRACE_0("btapp_avk_ucodec_shutdown failed!");
    }
}

void btapp_avk_play_shutdown(void)
{
    BT_HDR* p_msg;
    if ((p_msg = (BT_HDR *)GKI_getbuf(BT_HDR_SIZE)) != NULL)
    {
        p_msg->event = BTAPP_AVK_AUDIO_PLAY_SHUTDOWN;
        GKI_send_msg (AUDIO_PLAY_TASK, TASK_MBOX_0, p_msg);
    }
    else
    {
        GKI_TRACE_0("btapp_avk_play_shutdown failed!");
    }
}

#endif

