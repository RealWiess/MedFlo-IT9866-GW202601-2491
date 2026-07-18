/****************************************************************************
**
**  Name:          ucodec_rk.c
**
**  Description:   Contains platform ucodec operation api
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bta_platform.h"
#include "bte_glue.h"
#include "audio_ite.h"
#include "audio_ctrl.h"

#if ((defined BTA_AVK_INCLUDED) && (BTA_AVK_INCLUDED == TRUE))

extern void BSP_AUDIO_OUT_TransferComplete_CallBack (void);
void        audio_DA_volume_set (uint32_t level);

UINT8       ucodec_opened[UCODEC_NUMBER];
UINT8       ucodec_started[UCODEC_NUMBER];
tUCODEC_CNF ucodec_cnf[UCODEC_NUMBER];
UINT8       ucodec_vol_percnet[UCODEC_NUMBER];

/******************************************************************************
**
** Function         ucodec_get_samplerate_value
**
** Description
**
**                  Input : sample_rate: sample rate index.
**
**                  Output Parameters : none
**
** Returns          sample rate value
**
******************************************************************************/
UINT32      ucodec_get_samplerate_value (tUCODEC_SBC_SMP_FREQ sample_rate)
{
    UINT32 ret_val = 0;
    if (sample_rate == UCODEC_SBC_SMP_FREQ_16)
    {
        ret_val = 16000;
    }
    else if (sample_rate == UCODEC_SBC_SMP_FREQ_32)
    {
        ret_val = 32000;
    }
    else if (sample_rate == UCODEC_SBC_SMP_FREQ_44)
    {
        ret_val = 44100;
    }
    else if (sample_rate == UCODEC_SBC_SMP_FREQ_48)
    {
        ret_val = 48000;
    }
    else
    {
        ret_val = 16000;
    }
    return ret_val;
}

/******************************************************************************
**
** Function         UCODEC_Init
**
** Description      Startup initialisation function. This function is called
**                  before any orther function of UCODEC it initialize UCODEC
**                  internal structure an the external codec.
**
**                  Input : CodecId: Id of the codec to perform the operation on.
**
**                  Output Parameters : None
**
** Returns          UCODEC_SUCCESS if The action was performed with sucess.
**                  Error code else.
**
******************************************************************************/
tUCODEC_STATUS UCODEC_Init (void * ptr)
{
    return UCODEC_SUCCESS;
}

UINT8 UCODEC_Convert_Vol (UINT8 vol)
{
    UINT8 con_vol = 0;

    if (vol > 128)
    {
        vol = 128;
    }

    con_vol = (UINT16)vol * 100 / 128;

    return con_vol;
}

/******************************************************************************
**
** Function         UCODEC_Configure
**
** Description      Initialise the CODEC for a particular stream.
**
**
**                  Input : CodecId: Id of the codec to perform the operation on.
**                          CbackPrt: Call back pointer for codec feedback.
**                          pConfig: Pointer on a codec configuration structure.
**
**                  Output Parameters : None
**
** Returns          UCODEC_SUCCESS if The action was performed with sucess.
**
******************************************************************************/
tUCODEC_STATUS UCODEC_Configure (tUCODEC_ID CodecId, tUCODEC_CBACK_PTR pCallBack, tUCODEC_CNF * pConfig)
{
    if (CodecId == UCODEC_ID_1)
    {
        /* Store the call back pointer */
        ucodec_cb.pCallBack = pCallBack;

        /* Sanity check */
        if (NULL == pConfig)
        {
            DRV_TRACE_ERROR0("ERROR in UCODEC_Config pConfig == NULL");
            return UCODEC_WRONG_PARAM;
        }

        if (pConfig->MediaType != UCODEC_MEDIA_TYPE_AUDIO)
        {
            DRV_TRACE_ERROR1("ERROR in UCODEC_Config only support audio type MediaType: 0x%2X", pConfig->MediaType);
            return UCODEC_UNSUPORTED_CNF;
        }

        switch (pConfig->Type.AudioType)
        {
            case UCODEC_AUDIO_SBC:
                DRV_TRACE_DEBUG6("SampleFreq:%d, Subband:%d, ChannelMode:%d, NumBlock:%d,MinBitPool:%d, MaxBitPool:%d",
                                 ucodec_get_samplerate_value(pConfig->Feature.SBCConfig.SampleFreq),
                                 pConfig->Feature.SBCConfig.Subband, pConfig->Feature.SBCConfig.ChannelMode,
                                 pConfig->Feature.SBCConfig.NumBlock, pConfig->Feature.SBCConfig.MinBitPool,
                                 pConfig->Feature.SBCConfig.MaxBitPool);
                ucodec_cnf[UCODEC_ID_1].MediaType = pConfig->MediaType;
                ucodec_cnf[UCODEC_ID_1].Type      = pConfig->Type;
                memcpy(&ucodec_cnf[UCODEC_ID_1].Feature, &pConfig->Feature.SBCConfig, sizeof(tUCODEC_CNF_SBC));

                // Add target board API

                return UCODEC_SUCCESS;

            case UCODEC_AUDIO_VOLUME:
                DRV_TRACE_DEBUG1("UCODEC_Configure: UCODEC_AUDIO_VOLUME Volume:%d.", pConfig->Feature.Volume);
                if (ucodec_opened[UCODEC_ID_1] == TRUE)
                {
                    // Add target board API
                    if (pConfig->Feature.Volume < 127)
                    {
                        float vol_percent_f             = (float)pConfig->Feature.Volume * ((float)AUDIO_DA_VOLUME_MAX / 127);
                        ucodec_vol_percnet[UCODEC_ID_1] = (UINT8)(vol_percent_f + 0.5); // float -> uint8
                    }
                    else
                    {
                        ucodec_vol_percnet[UCODEC_ID_1] = 100;
                    }
                    audio_DA_volume_set(ucodec_vol_percnet[UCODEC_ID_1]); // 0~100
                }
                return UCODEC_SUCCESS;

            case UCODEC_AUDIO_BALANCE:

                return UCODEC_SUCCESS;

            case UCODEC_AUDIO_M12_LAYER3:

                return UCODEC_SUCCESS;

            case UCODEC_AUDIO_M24_2LC:
            case UCODEC_AUDIO_M24_4LC:
            case UCODEC_AUDIO_M24_4LTP:
            case UCODEC_AUDIO_M24_4S:
            default:
                DRV_TRACE_ERROR0("ERROR in UCODEC_Config SBC not supported");
                return UCODEC_WRONG_PARAM;
        }
    }

    return UCODEC_UNSUPORTED_CNF;
}

/******************************************************************************
**
** Function         UCODEC_FlushTx
**
** Description      Fluch Tx buffer Q.
**
**                  Input : CodecId: Id of the codec to perform the operation on.
**
**                  Output Parameters : None
**
** Returns          UCODEC_SUCCESS if The action was performed with sucess.
**                  Error code else.
**
******************************************************************************/
tUCODEC_STATUS UCODEC_FlushTx (tUCODEC_ID codec_id)
{
    BT_HDR * p_msg;

    if (codec_id == UCODEC_ID_1)
    {
        if (ucodec_cb.RxReadySent == 0)
        {
            ucodec_cb.RxReadySent = 1;
        }

        return UCODEC_SUCCESS;
    }

    return UCODEC_UNSUPORTED_CNF;
}

/******************************************************************************
**
** Function         UCODEC_FlushRx
**
** Description      Fluch Rx buffer Q.
**
**                  Input : CodecId: Id of the codec to perform the operation on.
**
**                  Output Parameters : None
**
** Returns          UCODEC_SUCCESS if The action was performed with sucess.
**                  Error code else.
**
******************************************************************************/
tUCODEC_STATUS UCODEC_FlushRx (tUCODEC_ID codec_id)
{
    return 0;
}

/******************************************************************************
**
** Function         UCODEC_FillBuf
**
** Description      Fill a buffer to the codec.
**
**                  Input :
**
**                  Output Parameters : None
**
** Returns
**
******************************************************************************/
tUCODEC_STATUS UCODEC_FillBuf (tUCODEC_ID codec_id, void * pbuf, UINT32 len)
{
    tUCODEC_STATUS st         = UCODEC_UNSUPORTED_CNF;
    uint32_t       regPrimask = 0;

    if (codec_id == UCODEC_ID_1 && ucodec_opened[UCODEC_ID_1] == TRUE)
    {
        AUDIO_Playback_Play(pbuf, len);
    }

    return st;
}

/******************************************************************************
**
** Function         UCODEC_WriteBuf
**
** Description      Send a buffer to the codec.
**
**                  Input : CodecId: Id of the codec to perform the operation on.
**                          pBuf: Pointer onto the GKI buffer to be send to the CODEC.
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
tUCODEC_STATUS UCODEC_WriteBuf (tUCODEC_ID codec_id, BT_HDR * pBuf)
{
    if (codec_id == UCODEC_ID_1 && ucodec_opened[UCODEC_ID_1] == TRUE)
    {
        DRV_TRACE_DEBUG0("UCODEC_WriteBuf");

        return UCODEC_SUCCESS;
    }

    return UCODEC_UNSUPORTED_CNF;
}

/******************************************************************************
**
** Function         UCODEC_ReadBuf
**
** Description      Get a buffer from the codec.
**
**                  Input : CodecId: Id of the codec to perform the operation on.
**
**                  Output Parameters : None
**
** Returns          Pointer on the GKI buffer. NULL if the Rx Q is empty
**
******************************************************************************/
tUCODEC_STATUS UCODEC_ReadBuf (tUCODEC_ID codec_id, BT_HDR ** pp_buf, tUCODEC_BUF_INFO * info)
{
    return 0;
}

/******************************************************************************
**
** Function         UCODEC_Open
**
** Description      This function is called to resume the codec from low power
**                  mode after UCODEC_Close had been called. It will put the
**                  codec in the state it was before UCODEC_Close being called.
**
**                  Input : CodecId: Id of the codec to perform the operation on.
**
**                  Output Parameters : None
**
** Returns          UCODEC_SUCCESS : The action was performed with sucess.
**                  Error code else.
**
******************************************************************************/
tUCODEC_STATUS UCODEC_Open (tUCODEC_ID codec_id)
{
    if (codec_id == UCODEC_ID_1)
    {
        if (ucodec_opened[UCODEC_ID_1] == FALSE)
        {
            DRV_TRACE_DEBUG0("UCODEC_Open");

            // Add target board API

            ucodec_opened[UCODEC_ID_1] = TRUE;

            return UCODEC_SUCCESS;
        }

        DRV_TRACE_DEBUG0("UCODEC_Open error state");
        return UCODEC_WRONG_PARAM;
    }
    return UCODEC_UNSUPORTED_CNF;
}

/******************************************************************************
**
** Function         UCODEC_Start
**
** Description
**
**                  Input : CodecId: Id of the codec to perform the operation on.
**
**
**
** Returns
**
******************************************************************************/
tUCODEC_STATUS UCODEC_Start (tUCODEC_ID codec_id)
{

    if (ucodec_started[codec_id] == FALSE)
    {
        ucodec_started[codec_id]  = TRUE;

        ucodec_cb.CurrentRxStream = UCODEC_STATE_SBC_NOT_SYNK;

    // Add target board API
    // Stream start
    #if 1
        {
            uint32_t      sample_rate = ucodec_get_samplerate_value(ucodec_cnf[UCODEC_ID_1].Feature.SBCConfig.SampleFreq);
            uint8_t       ch          = &ucodec_cnf[UCODEC_ID_1].Feature.SBCConfig.ChannelMode == UCODEC_CHN_MONO ? 1 : 2;
            STRC_I2S_SPEC ite_audio_param;
            AUDIO_Playback_init_ite_Params(ch, sample_rate, &ite_audio_param);
            AUDIO_Playback_Configure(&ite_audio_param);
            AUDIO_Playback_Open();
        }
    #endif
        return UCODEC_SUCCESS;
    }
    return UCODEC_WRONG_PARAM;
}

/******************************************************************************
**
** Function         UCODEC_Stop
**
** Description
**
**                  Input : CodecId: Id of the codec to perform the operation on.
**
**
**
** Returns
**
******************************************************************************/
void UCODEC_Stop (tUCODEC_ID codec_id)
{
    if (ucodec_started[codec_id] == TRUE)
    {
        // Add target board API
        // Stream stop
        AUDIO_Playback_Close();
        ucodec_started[codec_id] = FALSE;
    }
}

/******************************************************************************
**
** Function         UCODEC_Close
**
** Description      This function is called to put the codec in low power mode
**
**
**                  Input : CodecId: Id of the codec to perform the operation on.
**
**                  Output Parameters : None
**
** Returns          UCODEC_SUCCESS : The action was performed with sucess.
**                  Error code else.
**
******************************************************************************/
tUCODEC_STATUS UCODEC_Close (tUCODEC_ID codec_id)
{
    if (codec_id == UCODEC_ID_1)
    {
        if (ucodec_opened[UCODEC_ID_1] == TRUE)
        {
            DRV_TRACE_DEBUG0("UCODEC_Close");

            // Add target board API

            ucodec_opened[UCODEC_ID_1] = FALSE;
            return UCODEC_SUCCESS;
        }
        else
        {
            DRV_TRACE_DEBUG0("UCODEC_Close error state");
            return UCODEC_NOT_CONFIGURED;
        }
    }
    return UCODEC_UNSUPORTED_CNF;
}

/******************************************************************************
**
** Function         UCODEC_IsIdle
**
** Description      This function is called to judge the codec is idle or not
**
**
**                  Input : CodecId: Id of the codec to perform the operation on.
**
**                  Output Parameters : idle or not
**
** Returns
**
******************************************************************************/
BOOLEAN UCODEC_IsIdle (tUCODEC_ID codec_id)
{
    if (codec_id == UCODEC_ID_1)
    {
        return (ucodec_cb.RxReadySent == 0);
    }

    return 0;
}

BOOLEAN UCODEC_IsOpened (tUCODEC_ID codec_id)
{
    if (codec_id == UCODEC_ID_1)
    {
        return (ucodec_opened[UCODEC_ID_1]);
    }

    return 0;
}

#endif
