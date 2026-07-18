/******************************************************************************
**
**  Name:         btapp_avk_codec_int.h
**
**  Description:  This file contains the example decoder related definition used
**                for BTA advanced audio/video sink.
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_AVK_CODEC_INT_H
#define BTAPP_AVK_CODEC_INT_H

#include "data_types.h"
#include "bt_target.h"
#include "gki.h"
#include "ucodec.h"

/* include sbc decoder */
#include "sbc_decoder.h"
#include <stdbool.h>

/*****************************************************************************/
/*                            C O N S T A N T S                              */
/*****************************************************************************/
#define tUCODEC_STATE   UINT8
#define UCODEC_STATE_NOT_CNF        (0)
#define UCODEC_STATE_SBC            (2)
#define UCODEC_STATE_SBC_NOT_SYNK   (3)
#define UCODEC_STATE_SBC_SYNK       (4)
#define UCODEC_STATE_MP3            (5)

/*****************************************************************************/
/*                           D A T A   T Y P E S                             */
/*****************************************************************************/
/* Ucodec control block stucture definition */
typedef	struct tUCODEC_INST_CB_TAG
{
    BOOLEAN             IsProcessingCB;
    BOOLEAN             PCMFlowOn;
    BOOLEAN             Suspended;
    BOOLEAN             IsUnderRun;
    tUCODEC_CBACK_PTR   pCallBack;
    BUFFER_Q            PlayedQueue;
    BUFFER_Q            TxPCMQueue;             /* PCM sample to be played by the codec */
    BUFFER_Q            DecodeQueue;            /* SBC or MP3 decode Q to decode algorithm*/

    tUCODEC_STATE       CurrentRxStream;        /* from application to codec */
    tUCODEC_STATE       CurrentTxStream;        /* from codec to application */
    SBC_DEC_PARAMS      DecParams;
    SINT16              *pCurrentPcmBuf;
    UINT16              PcmOffset;              /* Offset in the PCM buffer being filled */
    UINT16              Offset;                 /* GKI buffer based offset for UCODEC_ReadBuf */

    BOOLEAN             RxReadySent;
    tUCODEC_STATUS      Status;

    /* sbc specific buffer */
    SINT32 SBCScartchMem[240+512+128+128+16+16];
    SINT32 SBCStaticMem[DEC_VX_BUFFER_SIZE + (SBC_MAX_PACKET_LENGTH >> 2)];

} tUCODEC_INST_CB;

/*****************************************************************************/
/*                            S T A T I C  D A T A                           */
/*****************************************************************************/
extern tUCODEC_INST_CB  ucodec_cb;

#endif
