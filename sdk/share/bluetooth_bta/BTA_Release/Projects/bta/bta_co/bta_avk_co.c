/*****************************************************************************
**
**  Name:           bta_avk_co.c
**
**  Description:    This is the audio sink call-out function
**                  implementation for Insight.
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
*****************************************************************************/

#include <string.h>
//#include "bta_platform.h"
#include "bte_glue.h"

#if (defined BTA_AVK_INCLUDED) && (BTA_AVK_INCLUDED == TRUE)

/****************************************************************************\
* Audio definitions
\****************************************************************************/
/* Optional audio codecs are unsupported by default */
#ifndef BTAPP_AVK_M12_SUPPORT
#define BTAPP_AVK_M12_SUPPORT      FALSE
#endif

#ifndef BTAPP_AVK_M24_SUPPORT
#define BTAPP_AVK_M24_SUPPORT      FALSE
#endif

#if (BTAPP_AVK_M12_SUPPORT == TRUE)
#include "a2d_m12.h"
#include "bta_avk_m12.h"
#endif

#if (BTAPP_AVK_M24_SUPPORT == TRUE)
#include "a2d_m24.h"
#include "bta_avk_m24.h"
#endif

#if (BTA_AVK_CO_CP_SCMS_T == TRUE)
#define BTA_AVK_CO_NUM_CP_SUPPORTED 1   /* We only support SCMS for now */
/* the length of the content protection header. */
#define BTA_AVK_CP_HDR_LEN          1
#else
#define BTA_AVK_CO_NUM_CP_SUPPORTED  0
#endif

enum {
    BTAPP_AVK_SBC_INDEX,
#if (BTAPP_AVK_M12_SUPPORT == TRUE)
    BTAPP_AVK_M12_INDEX,
#endif
#if (BTAPP_AVK_M24_SUPPORT == TRUE)
    BTAPP_AVK_M24_INDEX
#endif
};

#undef BT_USE_TRACES
#define BT_USE_TRACES   TRUE


/****************************************************************************\
* Video definitions
\****************************************************************************/

/* Optional video codecs are unsupported by default */
#ifndef BTAPP_AVK_MPEG4_SUPPORT
#define BTAPP_AVK_MPEG4_SUPPORT FALSE
#endif

#ifndef BTAPP_AVK_H263P3_SUPPORT
#define BTAPP_AVK_H263P3_SUPPORT FALSE
#endif

#ifndef BTAPP_AVK_H263P8_SUPPORT
#define BTAPP_AVK_H263P8_SUPPORT FALSE
#endif

#ifndef BTAPP_AVK_VEND1_SUPPORT       /* H.264 */
#define BTAPP_AVK_VEND1_SUPPORT FALSE
#endif

#ifndef BTAPP_AVK_VEND2_SUPPORT       /* IMG */
#define BTAPP_AVK_VEND2_SUPPORT FALSE
#endif


enum {
    BTAPP_AVK_H263P0_INDEX,
#if (BTAPP_AVK_MPEG4_SUPPORT == TRUE)
    BTAPP_AVK_MPEG4_INDEX,
#endif
#if (BTAPP_AVK_H263P3_SUPPORT == TRUE)
    BTAPP_AVK_H263P3_INDEX,
#endif
#if (BTAPP_AVK_H263P8_SUPPORT == TRUE)
    BTAPP_AVK_H263P8_INDEX,
#endif
#if (BTAPP_AVK_VEND1_SUPPORT == TRUE)
    BTAPP_AVK_VENDOR1_INDEX,
#endif
#if (BTAPP_AVK_VEND2_SUPPORT == TRUE)
    BTAPP_AVK_VENDOR2_INDEX
#endif
};


/* Buffer for AVDT to reassemble fragmented vide frames */
#ifndef BTA_AVK_MEDIA_BUFFER_SIZE
#define BTA_AVK_MEDIA_BUFFER_SIZE   0xFFFF
#endif

UINT8 bta_avk_media_buf[BTA_AVK_MEDIA_BUFFER_SIZE];

/*****************************************************************************
**  Constants
*****************************************************************************/

/* maximum number of packets for btapp codec to queue before dropping data
** this is in units of waveIn buffer periods, 20.3 ms
*/
#define BTA_AVK_CO_CODEC_TYPE_IDX       2
#define BTA_AVK_PKT_Q_MAX        10
#define BTA_AVK_CO_MAX_SRCS      5   /* max number of SNK devices */
#define BTA_AVK_CO_MAX_SEPS      BTA_AVK_MAX_SEPS /* number of codec suported locally */
#define BTA_AVK_SBC_MAX_BITPOOL  0x59

/* SBC codec capabilities */
const tA2D_SBC_CIE bta_avk_co_sbc_cap = {
    (A2D_SBC_IE_SAMP_FREQ_16 | A2D_SBC_IE_SAMP_FREQ_32) |
    (A2D_SBC_IE_SAMP_FREQ_44 | A2D_SBC_IE_SAMP_FREQ_48),    /* samp_freq */
    (A2D_SBC_IE_CH_MD_MONO | A2D_SBC_IE_CH_MD_STEREO |
     A2D_SBC_IE_CH_MD_JOINT | A2D_SBC_IE_CH_MD_DUAL),       /* ch_mode */
    (A2D_SBC_IE_BLOCKS_16 | A2D_SBC_IE_BLOCKS_12 |
     A2D_SBC_IE_BLOCKS_8 | A2D_SBC_IE_BLOCKS_4),            /* block_len */
    (A2D_SBC_IE_SUBBAND_4 | A2D_SBC_IE_SUBBAND_8),          /* num_subbands */
    (A2D_SBC_IE_ALLOC_MD_L | A2D_SBC_IE_ALLOC_MD_S),        /* alloc_mthd */
    BTA_AVK_SBC_MAX_BITPOOL,                                 /* max_bitpool */
    A2D_SBC_IE_MIN_BITPOOL                                  /* min_bitpool */
};

/* SBC codec preferences */
static tA2D_SBC_CIE bta_avk_co_sbc_pref = {
//    A2D_SBC_IE_SAMP_FREQ_44,                                /* samp_freq */
    A2D_SBC_IE_SAMP_FREQ_48,
    A2D_SBC_IE_CH_MD_STEREO,                                /* ch_mode */
    A2D_SBC_IE_BLOCKS_16,                                   /* block_len */
    A2D_SBC_IE_SUBBAND_8,                                   /* num_subbands */
    A2D_SBC_IE_ALLOC_MD_L,                                  /* alloc_mthd */
    0,                                                      /* max_bitpool */
    0                                                       /* min_bitpool */
};

#if (BTAPP_AVK_M12_SUPPORT == TRUE)
/* MPEG-1, 2 Audio (MP3) codec capabilities */
const tA2D_M12_CIE bta_avk_co_m12_cap = {
    (A2D_M12_IE_LAYER3),                                /* layers - MP3 only */
    TRUE,                                               /* Support of CRC protection or not */
    (A2D_M12_IE_CH_MD_MONO  | A2D_M12_IE_CH_MD_DUAL |   /* Channel mode */
     A2D_M12_IE_CH_MD_STEREO| A2D_M12_IE_CH_MD_JOINT),
    1,                                                  /* 1, if MPF-2 is supported. 0, otherwise */
    (A2D_M12_IE_SAMP_FREQ_16 | A2D_M12_IE_SAMP_FREQ_22 |/* Sampling frequency */
     A2D_M12_IE_SAMP_FREQ_24 | A2D_M12_IE_SAMP_FREQ_32 |
     A2D_M12_IE_SAMP_FREQ_44 | A2D_M12_IE_SAMP_FREQ_48),
    TRUE,                                               /* Variable Bit Rate */
    (A2D_M12_IE_BITRATE_7 | A2D_M12_IE_BITRATE_8  |     /* Bit rate index */
     A2D_M12_IE_BITRATE_9 | A2D_M12_IE_BITRATE_10 |     /* for MP3, bit rate index 1,2,3,4,6 are for single channel */
     A2D_M12_IE_BITRATE_11| A2D_M12_IE_BITRATE_12 |     /* bit rate index 13, 14 may be too much in data size */
     A2D_M12_IE_BITRATE_0 | A2D_M12_IE_BITRATE_5 )
};

/* MPEG-1, 2 Audio (MP3) codec preferences */
const tA2D_M12_CIE bta_avk_co_m12_pref = {
    (A2D_M12_IE_LAYER3),                                /* layers */
    TRUE,                                               /* Support of CRC protection or not */
    (A2D_M12_IE_CH_MD_JOINT),                           /* Channel mode */
    1,                                                  /* 1, if MPF-2 is supported. 0, otherwise */
    (A2D_M12_IE_SAMP_FREQ_44),                          /* Sampling frequency */
    TRUE,                                               /* Variable Bit Rate */
    A2D_M12_IE_BITRATE_9                                /* Bit rate index */
};
#endif

#if (BTAPP_AVK_M24_SUPPORT == TRUE)
/* AAC codec capabilities */
const tA2D_M24_CIE bta_avk_co_m24_cap = {
    (A2D_M24_IE_OBJ_2LC | A2D_M24_IE_OBJ_4LC |              /* Object type */
     A2D_M24_IE_OBJ_4LTP| A2D_M24_IE_OBJ_4S),
    (A2D_M24_IE_SAMP_FREQ_8  | A2D_M24_IE_SAMP_FREQ_11 |    /* Sampling frequency */
     A2D_M24_IE_SAMP_FREQ_12 | A2D_M24_IE_SAMP_FREQ_16 |
     A2D_M24_IE_SAMP_FREQ_22 | A2D_M24_IE_SAMP_FREQ_24 |
     A2D_M24_IE_SAMP_FREQ_32 | A2D_M24_IE_SAMP_FREQ_44 |
     A2D_M24_IE_SAMP_FREQ_48 | A2D_M24_IE_SAMP_FREQ_64 |
     A2D_M24_IE_SAMP_FREQ_88 | A2D_M24_IE_SAMP_FREQ_96),
    (A2D_M24_IE_CHNL_1 | A2D_M24_IE_CHNL_2),                /* Channel mode */
    TRUE,                                                   /* Variable Bit Rate */
    A2D_M24_IE_BITRATE_MSK                                  /* Bit rate index */
};
#endif


/* Video capabilities */
static tBTA_AVK_VIDEO_CFG bta_av_co_h263_cap =
{
    BTA_AVK_CODEC_H263_P0,      /* Codec type */
    VDP_H263_IE_LEVEL10         /* level mask */
};

/*****************************************************************************
**  Local data
*****************************************************************************/
/* delay report:
 * in unit as 0.1 ms, 60000 (i.e. 6 second) is the maximum
 * The value rported to SRC is the time from a media packet is received by baseband
 * and when the packet is presented to the user.
 * Maximum allowed presentation delay between audio and video is +80/-150 ms
 * Recommended presentation delay between audio and video is +35/-95 ms
 * Maximum allowed deviation of reported SNK Delay is +/- 30 ms
 * Recommended deviation of reported SNK Delay is +/- 15 ms
 *
 * bta_avk_co_*delay is called by bta_avk code when the initial delay needs to be sent
 * to the SRC device by calling BTA_AvkDelayReport().
 * When the platform detects a delay change, the platform code needs to report the
 * new delay value by calling BTA_AvkDelayReport() again.
 */
static UINT16 bta_avk_co_cur_audio_delay = 5000;
static UINT16 bta_avk_co_cur_video_delay = 5000;

/* timestamp */
static UINT32 bta_avk_co_timestamp;
/* peer configuration (get config) */
static UINT8  bta_avk_co_sep_info_idx;   /* sep_info_idx as reported/used in the bta_avk_co_getconfig */
static UINT8  bta_avk_co_curr_codec_info[AVDT_CODEC_SIZE]; /* current configuration for incoming connection */

/* Codec configured */
static BOOLEAN bta_avk_codec_configured = FALSE;

typedef struct
{
    UINT8           codec_info[AVDT_CODEC_SIZE]; /* peer SEP configuration */
    UINT8           seid;
    UINT8           sep_idx;
} tBTA_AVK_CO_SEP;

typedef struct
{
    BD_ADDR         addr; /* address of audio SRC */
    tBTA_AVK_CO_SEP  sep[BTA_AVK_CO_MAX_SEPS];
    UINT8           num_seps;   /* total number of seps at peer */
    UINT8           num_src;    /* total number of src at peer */
    UINT8           num_good;   /* total number of src that we are interested */
    UINT8           idx;    /* the index of sep[] to store info next */
    UINT8           use;    /* the index of sep[] used for streaming */
} tBTA_AVK_CO_SRC;

tBTA_AVK_CO_SRC bta_avk_co_src;
UINT8 bta_avk_co_src_idx = 0;
UINT8 bta_avk_co_num_src = 0;
UINT8 bta_avk_co_sep_info_idx_temp = 0;

/*******************************************************************************
**
** Function         bta_avk_co_get_sep_info_idx
**
** Description      find the sep_info_idx for the stream with the given codec type.
**
**
** Returns          sep_info_idx
**
*******************************************************************************/
UINT8 bta_avk_co_get_sep_info_idx(UINT8 codec_type)
{
    UINT8 sep_info_idx = 0;
    int     xx;
    tBTA_AVK_CO_SRC *p_src = &bta_avk_co_src; /* need to find an unused one */
    for(xx=0; xx<p_src->num_good; xx++)
    {
        if(p_src->sep[xx].codec_info[BTA_AVK_CO_CODEC_TYPE_IDX] == codec_type)
        {
            APPL_TRACE_DEBUG2("[%d] sep_info_idx: %d", xx, p_src->sep[xx].sep_idx);
            sep_info_idx = p_src->sep[xx].sep_idx;
            break;
        }
    }
    return sep_info_idx;
}

/*******************************************************************************
**
** Function         bta_avk_co_audio_init
**
** Description      This callout function is executed by AK when it is
**                  started by calling BTA_AvkEnable().  This function can be
**                  used by the phone to initialize audio paths or for other
**                  initialization purposes.
**
**
** Returns          Stream codec and content protection capabilities info.
**
*******************************************************************************/
BOOLEAN bta_avk_co_audio_init(UINT8 *p_codec_type, UINT8 *p_codec_info,
                    UINT8 *p_num_protect, UINT8 *p_protect_info, UINT8 index)
{
    BOOLEAN ret = FALSE;

    APPL_TRACE_DEBUG1("bta_avk_co_audio_init: %d", index);

#if (BTA_AVK_CO_CP_SCMS_T == TRUE)
    if (*p_num_protect == BTA_AVK_CO_NUM_CP_SUPPORTED)
    {
        /* AVK only support one CP stream for now */
        *p_num_protect = BTA_AVK_CO_NUM_CP_SUPPORTED;
        *p_protect_info++ = BTA_AVK_CP_LOSC;
        UINT16_TO_STREAM(p_protect_info, BTA_AVK_CP_SCMS_T_ID);
        APPL_TRACE_DEBUG0("bta_avk_co_audio_init CP enabled ");
    }
    else
    {
        /* If CP is not supported, set num_protect to 0 */
        *p_num_protect = 0;
        *p_protect_info = 0;
    }
#else
        *p_num_protect = BTA_AVK_CO_NUM_CP_SUPPORTED;
        *p_protect_info = 0;
#endif

    switch(index)
    {
    case BTAPP_AVK_SBC_INDEX: /* SBC */
        bta_avk_co_src.num_good = 0;

        /* initialize SBC codec preferences */
#if( defined BTA_AVK_INCLUDED ) && (BTA_AVK_INCLUDED == TRUE)
        bta_avk_co_sbc_pref.ch_mode = btapp_cfg.avk_channel_mode;
#endif
        *p_codec_type = BTA_AVK_CODEC_SBC;
        A2D_BldSbcInfo(AVDT_MEDIA_AUDIO, (tA2D_SBC_CIE *) &bta_avk_co_sbc_cap, p_codec_info);
        btapp_avk_codec_init();
        ret = TRUE;
        break;

#if (BTAPP_AVK_M12_SUPPORT == TRUE)
    case BTAPP_AVK_M12_INDEX: /* MP3 */
        /* set up for MP
        3 codec */
        *p_codec_type = BTA_AVK_CODEC_M12;
        A2D_BldM12Info(AVDT_MEDIA_AUDIO, (tA2D_M12_CIE *) &bta_avk_co_m12_cap, p_codec_info);
        APPL_TRACE_DEBUG6("bta_avk_co_init m12 [%x;%x;%x;%x;%x]", \
            p_codec_info[1], p_codec_info[2], p_codec_info[3],  p_codec_info[4],  p_codec_info[5], p_codec_info[6]);
        ret = TRUE;
        break;
#endif

#if (BTAPP_AVK_M24_SUPPORT == TRUE)
    case BTAPP_AVK_M24_INDEX: /* AAC */
        /* set up for MP3 codec */
        *p_codec_type = BTA_AVK_CODEC_M24;
        A2D_BldM24Info(AVDT_MEDIA_AUDIO, (tA2D_M24_CIE *) &bta_avk_co_m24_cap, p_codec_info);
        APPL_TRACE_DEBUG6("bta_avk_co_init m24 [%x;%x;%x;%x;%x]", \
            p_codec_info[1], p_codec_info[2], p_codec_info[3],  p_codec_info[4],  p_codec_info[5], p_codec_info[6]);
        ret = TRUE;
        break;
#endif


    default:
        break;
    }

    return ret;
}

/*******************************************************************************
**
** Function         bta_avk_co_audio_disc_res
**
** Description      This callout function is executed by AK to report the
**                  number of stream end points (SEP) were found during the
**                  AVDT stream discovery process.
**
**
** Returns          void.
**
*******************************************************************************/
void bta_avk_co_audio_disc_res(UINT8 num_seps, UINT8 num_src, BD_ADDR addr)
{
    /* outgoing */
    APPL_TRACE_DEBUG2("bta_avk_co_disc_res out : good: %d num_seps = %d",\
    bta_avk_co_src.num_good, num_seps);
    bta_avk_co_src.num_seps = num_seps;
    bta_avk_co_src.num_src  = num_src;
    bta_avk_co_src.num_good = 0;
    bdcpy(bta_avk_co_src.addr, addr);
    bta_avk_co_src.idx = 0;
    bta_avk_co_src.use = 0;
}

/*******************************************************************************
**
** Function         bta_avk_co_audio_getconfig
**
** Description      This callout function is executed by AK to retrieve the
**                  desired codec and content protection configuration for the
**                  stream.
**
**
** Returns          Stream codec and content protection configuration info.
**
*******************************************************************************/
UINT8 bta_avk_co_audio_getconfig(tBTA_AVK_CODEC codec_type, UINT8 *p_codec_info,
                         UINT8 *p_sep_info_idx, UINT8 seid,
                         UINT8 *p_num_protect, UINT8 *p_protect_info)
{
    UINT8 status = A2D_FAIL;
    tBTA_AVK_CO_SRC *p_src = &bta_avk_co_src;
    tBTA_AVK_CO_SEP *p_sep = &p_src->sep[p_src->idx];

    APPL_TRACE_DEBUG1("bta_avk_co_audio_getconfig, codec_type:%d", codec_type);
    /* if more than one codec type is supported, the application needs to keep
     * candidate *p_sep_info_idx and p_codec_info[] in its control block.
     * at the last report (num_src times) of configuration
     * or when the preferred configuration
     * is found, copy the decided *p_sep_info_idx and p_codec_info[] back to AK
     * and return 0.
     * otherwise return 1 (non-zero) to tell AK to keep getting the configuration
     * from headset.
     */

    /* remember the peer codec info - capabilities */
    memcpy(p_sep->codec_info, p_codec_info, AVDT_CODEC_SIZE);

    /* get codec configuration */
    switch(codec_type)
    {
    case BTA_AVK_CODEC_SBC:
        /* verify SBC configuration */
        status = bta_avk_sbc_cfg_for_cap(p_codec_info, (tA2D_SBC_CIE *) &bta_avk_co_sbc_cap,
                 (tA2D_SBC_CIE *) &bta_avk_co_sbc_pref);
        /* the preferred is SBC in this case */
        p_src->use = p_src->idx;
        bta_avk_co_sep_info_idx_temp = p_sep->sep_idx + 1;
        memcpy(bta_avk_co_curr_codec_info, p_codec_info, AVDT_CODEC_SIZE);
        break;

#if (BTAPP_AVK_M12_SUPPORT == TRUE)
    case BTA_AVK_CODEC_M12:
        /* verify MP3 configuration */
        status = A2D_SUCCESS;
        break;
#endif

#if (BTAPP_AVK_M24_SUPPORT == TRUE)
    case BTA_AVK_CODEC_M24:
        /* verify AAC configuration */
        status = A2D_SUCCESS;
        break;
#endif

    default:
        break;
    }

    p_sep->sep_idx  = *p_sep_info_idx;
    if(status == A2D_SUCCESS)
    {
        APPL_TRACE_DEBUG3("[%d] codec_type: %d sep_info_idx: %d", p_src->idx, codec_type, *p_sep_info_idx);
        p_src->num_good++;
        p_src->idx++;
        /* we like this one. store info */
        p_sep->seid     = seid;
    }

    if(bta_avk_co_sep_info_idx_temp && p_sep->sep_idx == (p_src->num_seps - 1))
    {
        /* this is the last one */
        status = A2D_SUCCESS;
        memcpy(p_codec_info, bta_avk_co_curr_codec_info, AVDT_CODEC_SIZE);
        *p_sep_info_idx = bta_avk_co_sep_info_idx_temp - 1;
        bta_avk_co_sep_info_idx_temp = 0;
    }

    APPL_TRACE_DEBUG1("status: %d", status);

    return status;
}

/*******************************************************************************
**
** Function         bta_avk_co_audio_setconfig
**
** Description      This callout function is executed by AK to set the
**                  codec and content protection configuration of the stream.
**
**
** Returns          void
**
*******************************************************************************/
void bta_avk_co_audio_setconfig(tBTA_AVK_CODEC codec_type,
                                UINT8 *p_codec_info,
                                UINT8 seid,
                                BD_ADDR addr,
                                UINT8 num_protect,
                                UINT8 *p_protect_info)
{
    UINT8 status = A2D_FAIL;
    UINT8 cp_status = A2D_FAIL;
    UINT8 category = AVDT_ASC_CODEC;
    UINT8 num = 0, count = 0;
#if (BTA_AVK_CO_CP_SCMS_T == TRUE)
    UINT16 cp_id;
#endif

    APPL_TRACE_DEBUG1("bta_avk_co_setconfig, codec_type:%d", codec_type);

    /* set decoder configuration */
    switch(codec_type)
    {
    case BTA_AVK_CODEC_SBC:
        if ((status = bta_avk_sbc_cfg_in_cap(p_codec_info,
            (tA2D_SBC_CIE *) &bta_avk_co_sbc_cap)) == A2D_SUCCESS)
        {
            /* config decoder */
            btapp_avk_codec_sbc_cnf(p_codec_info, TRUE);
        }
        break;

#if (BTAPP_AVK_M12_SUPPORT == TRUE)
    case BTA_AVK_CODEC_M12:
        /* verify MP3 configuration */
        if ((status = bta_avk_m12_cfg_in_cap(p_codec_info,
            (tA2D_M12_CIE *) &bta_avk_co_m12_cap)) == A2D_SUCCESS)
        {
            /* config codec */
            btapp_avk_codec_m12_cnf(p_codec_info);
        }
        break;
#endif

#if (BTAPP_AVK_M24_SUPPORT == TRUE)
    case BTA_AVK_CODEC_M24:
        /* verify AAC configuration */
        if ((status = bta_avk_m24_cfg_in_cap(p_codec_info,
            (tA2D_M24_CIE *) &bta_avk_co_m24_cap)) == A2D_SUCCESS)
        {
            btapp_avk_codec_m24_cnf(p_codec_info);
        }

        break;
#endif

    default:
        break;
    }

#if (BTA_AVK_CO_CP_SCMS_T == TRUE)
    if (status == A2D_SUCCESS)
    {
        if (num_protect == BTA_AVK_CO_NUM_CP_SUPPORTED)
        {
            if (*p_protect_info++ >= BTA_AVK_CP_LOSC)
            {
                STREAM_TO_UINT16(cp_id, p_protect_info);
                /* check whether CP type is SCMS */
                if (BTA_AVK_CP_SCMS_T_ID == cp_id)
                {
                    btapp_ak_cb.avk_cp_engaged = TRUE;
                    cp_status = A2D_SUCCESS;
                    bta_avk_ci_cp_scms(BTA_AVK_CHNL_AUDIO, TRUE, BTA_AVK_CP_SCMS_COPY_PROHIBITED);
                }
            }
        }

        if (cp_status != A2D_SUCCESS)
        {
            /* local or peer device does not support CP in this case. */
            btapp_ak_cb.avk_cp_engaged = FALSE;
            bta_avk_ci_cp_scms(BTA_AVK_CHNL_AUDIO, FALSE, BTA_AVK_CP_SCMS_COPY_PROHIBITED);
        }
    }
    else
        btapp_ak_cb.avk_cp_engaged = FALSE;
#endif

    memcpy(bta_avk_co_curr_codec_info, p_codec_info, AVDT_CODEC_SIZE);
    bta_avk_co_sep_info_idx = 0;

    num = bta_avk_co_src.num_good;
    bta_avk_co_src.idx = 0;

    /* If config is invalid, then send config response now */
    /* (If config is valid, then wait until codec has been configured before sending response) */
    if (status != A2D_SUCCESS)
        bta_avk_ci_setconfig(BTA_AVK_CHNL_AUDIO, status, category);
    else
        bta_avk_codec_configured = TRUE;

}

/*******************************************************************************
**
** Function         bta_avk_co_audio_opencfg
**
** Description      This callout function is executed by AK to set the
**                  codec and content protection configuration of the stream
**                  when the stream connection on LPST/SE is open
**
** Returns          void
**
*******************************************************************************/
void bta_avk_co_audio_opencfg(tBTA_AVK_CODEC codec_type, UINT8 *p_codec_info, BD_ADDR addr,
                                    UINT8 num_protect, UINT8 *p_protect_info)
{
    APPL_TRACE_DEBUG0("bta_avk_co_audio_opencfg");
#if (BTA_AVK_CO_CP_SCMS_T == TRUE)
    btapp_ak_cb.avk_cp_engaged = FALSE;
#endif

    memcpy(bta_avk_co_curr_codec_info, p_codec_info, AVDT_CODEC_SIZE);
    bta_avk_co_sep_info_idx = 0;

    bta_avk_co_src.idx = 0;
    bta_avk_codec_configured = TRUE;
    bta_avk_co_timestamp = 0;
    /* set decoder configuration */
    switch(codec_type)
    {
    case BTA_AVK_CODEC_SBC:
        /* config decoder */
        btapp_avk_codec_sbc_cnf(p_codec_info, FALSE);
        btapp_avk_codec_open();
        break;

#if (BTAPP_AVK_M12_SUPPORT == TRUE)
    case BTA_AVK_CODEC_M12:
        /* config codec */
        btapp_avk_codec_m12_cnf(p_codec_info);
        btapp_avk_codec_open();
        break;
#endif

#if (BTAPP_AVK_M24_SUPPORT == TRUE)
    case BTA_AVK_CODEC_M24:
        btapp_avk_codec_m24_cnf(p_codec_info);
        btapp_avk_codec_open();
        break;
#endif

    default:
        break;
    }


}

/*******************************************************************************
**
** Function         bta_avk_co_audio_open
**
** Description      This function is called by AK when the stream connection
**                  is opened.
**
**
** Returns          void
**
*******************************************************************************/
void bta_avk_co_audio_open(tBTA_AVK_CODEC codec_type, UINT8 *p_codec_info, UINT16 mtu)
{
    APPL_TRACE_DEBUG1("bta_avk_co_audio_open mtu:%d", mtu);

    if (!bta_avk_codec_configured)
    {
        /* If SNK initiated connection,                                 */
        /* decoder codec is not configured at the time of setconfig.    */
        btapp_avk_codec_sbc_cnf(bta_avk_co_curr_codec_info, FALSE);
        bta_avk_codec_configured = TRUE;
    }

#if (BTU_BTC_SNK_INCLUDED == TRUE)
    /* If using BTC lite stack, no further processing needed. */
    if ((btapp_ak_cb.btc_route != BTA_DM_ROUTE_NONE) &&
        (btapp_ak_cb.audio_path_supported == TRUE))
        return;
#endif

    bta_avk_co_timestamp = 0;

    switch(codec_type)
    {
    case BTA_AVK_CODEC_SBC:
        btapp_avk_codec_open();
        break;

#if (BTAPP_AVK_M12_SUPPORT == TRUE)
    case BTA_AVK_CODEC_M12:
        /* need to prepare the audio buffers according to the MTU */
        btapp_avk_codec_open();
        break;
#endif

#if (BTAPP_AVK_M24_SUPPORT == TRUE)
    case BTA_AVK_CODEC_M24:
        btapp_avk_codec_open();
        break;
#endif

    default:
        break;
    }

}

/*******************************************************************************
**
** Function         bta_avk_co_audio_close
**
** Description      This function is called by AK when the stream connection
**                  is closed.
**
**
** Returns          void
**
*******************************************************************************/
void bta_avk_co_audio_close(void)
{
    APPL_TRACE_DEBUG0("bta_avk_co_audio_close");
    bta_avk_codec_configured = FALSE;

#if (BTU_BTC_SNK_INCLUDED == TRUE)
    /* If using BTC lite stack, no further processing needed. */
    if ((btapp_ak_cb.btc_route != BTA_DM_ROUTE_NONE) &&
        (btapp_ak_cb.audio_path_supported == TRUE))
        return;
#endif

    btapp_avk_codec_close();
}

/*******************************************************************************
**
** Function         bta_avk_co_audio_delay
**
** Description      This function is called by AV when the audio stream connection
**                  needs to send the initial delay report to the connected SRC.
**
**
** Returns          void
**
*******************************************************************************/
void bta_avk_co_audio_delay(UINT8 err)
{
    APPL_TRACE_DEBUG0("bta_avk_co_audio_delay");
    BTA_AvkDelayReport(BTA_AVK_CHNL_AUDIO, bta_avk_co_cur_audio_delay);
}


/*******************************************************************************
**
** Function         bta_avk_co_audio_audio_start
**
** Description      This function is called by AK when the streaming data
**                  transfer is started.
**
**
** Returns          void
**
*******************************************************************************/
void bta_avk_co_audio_start(tBTA_AVK_CODEC codec_type)
{
    APPL_TRACE_DEBUG0("bta_avk_co_audio_start");

#if (BTU_BTC_SNK_INCLUDED == TRUE)
    /* If using BTC lite stack, no further processing needed. */
    if ((btapp_ak_cb.btc_route != BTA_DM_ROUTE_NONE) &&
        (btapp_ak_cb.audio_path_supported == TRUE))
        return;
#endif

    switch(codec_type)
    {
    case BTA_AVK_CODEC_SBC:
        btapp_avk_codec_start();
        break;

#if (BTAPP_AVK_M12_SUPPORT == TRUE)
    case BTA_AVK_CODEC_M12:
        btapp_avk_codec_start();
        break;
#endif

#if (BTAPP_AVK_M24_SUPPORT == TRUE)
    case BTA_AVK_CODEC_M24:
        btapp_avk_codec_start();
        break;
#endif

    default:
        break;
    }

}

#if (BTU_BTC_SNK_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         bta_avk_co_audio_btc_start
**
** Description      This function is called by AV when the audio streaming data
**                  transfer is started.
**
**
** Returns          void
**
*******************************************************************************/
void bta_avk_co_audio_btc_start(void)
{
    APPL_TRACE_DEBUG0("bta_avk_co_audio_btc_start");

    APPL_TRACE_DEBUG2("is switched %d switch_to %d",
                       btapp_cb.is_switched, btapp_cb.switch_to);
    /* If during the switching process, set the pending flag. Otherwise call CI
       to send start response immediately */
    if (btapp_cb.is_switched == FALSE && btapp_cb.switch_to == BTA_DM_SW_BB_TO_BTC)
    {
        APPL_TRACE_DEBUG0("bta_avk_co_audio_btc_start Switch to lite stack is going on");
        btapp_ak_cb.btc_start_pending = TRUE;
    }
    else
    {
        bta_avk_ci_audio_btc_start(BTA_AVK_CHNL_AUDIO);
    }
}
#endif


/*******************************************************************************
**
** Function         bta_avk_co_audio_stop
**
** Description      This function is called by AK when the streaming data
**                  transfer is stopped.
**
**
** Returns          void
**
*******************************************************************************/
void bta_avk_co_audio_stop(void)
{
#if (BTU_BTC_SNK_INCLUDED == TRUE)
    /* If using BTC lite stack, no further processing needed. */
    if ((btapp_ak_cb.btc_route != BTA_DM_ROUTE_NONE) &&
        (btapp_ak_cb.audio_path_supported == TRUE))
        return;
#endif

    APPL_TRACE_DEBUG0("bta_avk_co_audio_stop");

    btapp_avk_codec_stop();
}

/*******************************************************************************
**
** Function         bta_avk_co_sync_media_start
**
** Description      This function is called by AVK when the controller starts to
**                  deliver audio sync media packets.
**
**
** Returns          void
**
*******************************************************************************/
void bta_avk_co_sync_media_start(void)
{
    APPL_TRACE_DEBUG0("bta_avk_co_sync_media_start");
    //for embedded platform: free all received media packets and decoded PCM
}

/*******************************************************************************
**
** Function         bta_avk_co_lpst_sync_ready
**
** Description      This function is called by AVK when the lpst sync toggle
**                  is ready at both of PE and SE.
**                  need to stop music player.
**                  music player will resume by the lpst toggle.
**
**
** Returns          void
**
*******************************************************************************/
void bta_avk_co_lpst_sync_ready(void)
{
    APPL_TRACE_DEBUG0("bta_avk_co_lpst_sync_ready");
    //btapp_avk_codec_stop();
}

/*******************************************************************************
**
** Function         bta_avk_co_audio_data
**
** Description      This function is called by AK when the stream data is received.
**
**
** Returns          void
**
*******************************************************************************/
void bta_avk_co_audio_data(tBTA_AVK_CODEC codec_type,
                            BT_HDR *p_pkt,
                            UINT32 timestamp,
                            UINT16 seq_num,
                            UINT8 m_pt)
{
#if (BTU_BTC_SNK_INCLUDED == TRUE)
    /* If using BTC lite stack, no further processing needed. */
    if ((btapp_ak_cb.btc_route != BTA_DM_ROUTE_NONE) &&
        (btapp_ak_cb.audio_path_supported == TRUE))
    {
        GKI_freebuf(p_pkt);
        return;
    }
#endif

    //APPL_TRACE_DEBUG2("bta_avk_co_audio_data, timestamp = %d, seq_num = %d", timestamp, seq_num);

#if (BTA_AVK_CO_CP_SCMS_T == TRUE)
    if (btapp_ak_cb.avk_cp_engaged == TRUE)
    {
        /* CP header is not used here */
        APPL_TRACE_DEBUG1("CP header 0x%02x", *((UINT8 *)(p_pkt+1)+p_pkt->offset));
        p_pkt->len -= BTA_AVK_CP_HDR_LEN;
        p_pkt->offset += BTA_AVK_CP_HDR_LEN;
    }
#endif

    switch(codec_type)
    {
    case BTA_AVK_CODEC_SBC:
        btapp_avk_codec_write_sbc(p_pkt, timestamp, seq_num);
        break;

#if (BTAPP_AVK_M12_SUPPORT == TRUE)
    case BTA_AVK_CODEC_M12:
        btapp_avk_codec_write_m12(p_pkt, timestamp, seq_num);
        break;
#endif

#if (BTAPP_AVK_M24_SUPPORT == TRUE)
    case BTA_AVK_CODEC_M24:
        btapp_avk_codec_write_m24(p_pkt, timestamp, seq_num);
        break;
#endif

    default:
        GKI_freebuf(p_pkt);
        break;
    }
}





/*******************************************************************************
**                            VIDEO CALL-OUT FUNCTIONS
*******************************************************************************/


/*******************************************************************************
**
** Function         bta_avk_co_video_init
**
** Description      This callout function is executed by AV when it is
**                  started by calling BTA_AvkEnable().  This function can be
**                  used by the phone to initialize video paths or for other
**                  initialization purposes.
**
**
** Returns          Stream codec and content protection capabilities info.
**
*******************************************************************************/
BOOLEAN bta_avk_co_video_init(UINT8 *p_codec_type, UINT8 *p_codec_info,
                                   UINT8 *p_num_protect, UINT8 *p_protect_info, UINT8 index)
{
#if (BTAPP_AVK_VEND1_SUPPORT == TRUE || BTAPP_AVK_VEND2_SUPPORT == TRUE)
    UINT16  codec_id = BTA_AVK_CODEC_ID_IMG;
#endif
    BOOLEAN ret = FALSE;

    APPL_TRACE_DEBUG0("bta_av_co_video_init");

#if (BTA_AVK_CO_CP_SCMS_T == TRUE)
    if (*p_num_protect == BTA_AVK_CO_NUM_CP_SUPPORTED)
    {
        /* AVK only support one CP stream for now */
        *p_num_protect = BTA_AVK_CO_NUM_CP_SUPPORTED;
        *p_protect_info++ = BTA_AVK_CP_LOSC;
        UINT16_TO_STREAM(p_protect_info, BTA_AVK_CP_SCMS_T_ID);
        APPL_TRACE_DEBUG0("bta_avk_co_audio_init CP enabled ");
    }
    else
    {
        /* If CP is not supported, set num_protect to 0 */
        *p_num_protect = 0;
        *p_protect_info = 0;
    }
#else
        *p_num_protect = BTA_AVK_CO_NUM_CP_SUPPORTED;
        *p_protect_info = 0;
#endif

    switch(index)
    {
    case BTAPP_AVK_H263P0_INDEX:
        /* set up for H.263 Profile 0 codec */
        *p_codec_type = BTA_AVK_CODEC_H263_P0;
        VDP_SetCodecInfo(p_codec_info, AVDT_MEDIA_VIDEO, BTA_AVK_CODEC_H263_P0,
                        (VDP_H263_IE_LEVEL10 | VDP_H263_IE_LEVEL20 | VDP_H263_IE_LEVEL30));
        ret = TRUE;
        break;

#if (BTAPP_AVK_MPEG4_SUPPORT == TRUE)
    case BTAPP_AVK_MPEG4_INDEX:
        *p_codec_type = BTA_AVK_CODEC_MPEG4;
        VDP_SetCodecInfo(p_codec_info, AVDT_MEDIA_VIDEO, BTA_AVK_CODEC_MPEG4,
                        (VDP_MPEG_IE_LEVEL0 | VDP_MPEG_IE_LEVEL1 | VDP_MPEG_IE_LEVEL2 | VDP_MPEG_IE_LEVEL3));
        ret = TRUE;
        break;
#endif

#if (BTAPP_AVK_H263P3_SUPPORT == TRUE)
    case BTAPP_AVK_H263P3_INDEX:
        *p_codec_type = BTA_AVK_CODEC_H263_P3;
        VDP_SetCodecInfo(p_codec_info, AVDT_MEDIA_VIDEO, BTA_AVK_CODEC_H263_P3,
                        (VDP_H263_IE_LEVEL10 | VDP_H263_IE_LEVEL20 | VDP_H263_IE_LEVEL30));
        ret = TRUE;
        break;
#endif

#if (BTAPP_AVK_H263P8_SUPPORT == TRUE)
    case BTAPP_AVK_H263P8_INDEX:
        *p_codec_type = BTA_AVK_CODEC_H263_P8;
        VDP_SetCodecInfo(p_codec_info, AVDT_MEDIA_VIDEO, BTA_AVK_CODEC_H263_P8,
                        (VDP_H263_IE_LEVEL10 | VDP_H263_IE_LEVEL20 | VDP_H263_IE_LEVEL30));
        ret = TRUE;
        break;
#endif

#if (BTAPP_AVK_VEND1_SUPPORT == TRUE || BTAPP_AVK_VEND2_SUPPORT == TRUE)
#if (BTAPP_AVK_VEND1_SUPPORT == TRUE)
    case BTAPP_AVK_VENDOR1_INDEX:
        codec_id = BTA_AVK_CODEC_ID_H264;
#endif
#if (BTAPP_AVK_VEND2_SUPPORT == TRUE)
    case BTAPP_AVK_VENDOR2_INDEX:
#endif

        *p_codec_type = BTA_AVK_CODEC_VEND;
        VDP_SetVsInfo(p_codec_info, AVDT_MEDIA_VIDEO, codec_id, NULL, 0);
        ret = TRUE;
        break;
#endif

    default:
        break;
    }

    return ret;
}

/*******************************************************************************
**
** Function         bta_avk_co_video_disc_res
**
** Description      This callout function is executed by AV to report the
**                  number of stream end points (SEP) were found during the
**                  AVDT stream discovery process.
**
**
** Returns          void.
**
*******************************************************************************/
void bta_avk_co_video_disc_res(UINT8 num_seps, UINT8 num_snk, BD_ADDR addr)
{
}

/*******************************************************************************
**
** Function         bta_avk_co_video_getconfig
**
** Description      This callout function is executed by AV to retrieve the
**                  desired codec and content protection configuration for the
**                  video stream.
**
**
** Returns          Stream codec and content protection configuration info.
**
*******************************************************************************/
UINT8 bta_avk_co_video_getconfig(tBTA_AVK_CODEC codec_type,
                                         UINT8 *p_codec_info, UINT8 *p_sep_info_idx, UINT8 seid,
                                         UINT8 *p_num_protect, UINT8 *p_protect_info)
{
    UINT8 status = VDP_FAIL;

    return (status);
}

/*******************************************************************************
**
** Function         bta_avk_co_video_setconfig
**
** Description      This callout function is executed by AV to set the
**                  codec and content protection configuration of the video stream.
**
**
** Returns          void
**
*******************************************************************************/
void bta_avk_co_video_setconfig(tBTA_AVK_CODEC codec_type, UINT8 *p_codec_info,
                                        UINT8 seid, BD_ADDR addr,
                                        UINT8 num_protect, UINT8 *p_protect_info)
{
    UINT8 status = VDP_FAIL;
    UINT8 category = AVDT_ASC_CODEC;
    UINT8 num = 0, count = 0;
    UINT8 levels_mask;
    UINT8 cie_codec_type;

    APPL_TRACE_DEBUG0("bta_avk_co_video_setconfig");


    /* set decoder configuration */
    switch(codec_type)
    {
    case BTA_AVK_CODEC_H263_P0:
    case BTA_AVK_CODEC_H263_P3:
    case BTA_AVK_CODEC_H263_P8:
    case BTA_AVK_CODEC_MPEG4:
        if ((status = VDP_GetCodecInfo(p_codec_info, NULL, &cie_codec_type, &levels_mask)) == VDP_SUCCESS)
        {
            /* Make sure requested codec_type matches the stream */
            if (codec_type != cie_codec_type)
            {
                status = VDP_WRONG_CODEC;
            }
            /* Make sure requested level is supported (for H263 codecs only) */
            else if ((codec_type != BTA_AVK_CODEC_MPEG4) && ((levels_mask & bta_av_co_h263_cap.levels)==0))
            {
                status = VDP_INVALID_LEVEL;
            }
            /* do not allow content protection for now */
            else if (num_protect != 0)
            {
                status = VDP_BAD_CP_TYPE;
                category = AVDT_ASC_PROTECT;
            }
            else
            {
                /* Configuration is valid */
                /* Call application specific function for configuring the codec */
                btapp_avk_codec_video_cnf(codec_type, p_codec_info);
            }
        }
        break;

    case BTA_AVK_CODEC_VEND:
        /* Call application specific function for configuring the vendor specific video codec */
        status = btapp_avk_codec_video_vend_cnf(p_codec_info);
        break;


    default:
        status = VDP_BAD_CODEC_TYPE;
        break;
    }

    /* If config is invalid, then send config response now */
    /* (If config is valid, then wait until codec has been configured before sending response) */
    if (status != VDP_SUCCESS)
        bta_avk_ci_setconfig(BTA_AVK_CHNL_VIDEO, status, category);
}


/*******************************************************************************
**
** Function         bta_avk_co_video_open
**
** Description      This function is called by AV when the video stream connection
**                  is opened.
**
**
** Returns          void
**
*******************************************************************************/
void bta_avk_co_video_open(tBTA_AVK_CODEC codec_type, UINT8 *p_codec_info,
                                         UINT8 avdt_handle, UINT16 mtu)
{
    /* Set media buffer for AVDT to reassemble fragmented video frames */
    AVDT_SetMediaBuf(avdt_handle, bta_avk_media_buf, BTA_AVK_MEDIA_BUFFER_SIZE);

    /* Call platform specific open handler */
    //xieqi btapp_avk_codec_video_open();
}

/*******************************************************************************
**
** Function         bta_avk_co_video_close
**
** Description      This function is called by AV when the video stream connection
**                  is closed.
**
**
** Returns          void
**
*******************************************************************************/
void bta_avk_co_video_close(void)
{
}

/*******************************************************************************
**
** Function         bta_avk_co_video_start
**
** Description      This function is called by AV when the video streaming data
**                  transfer is started.
**
**
** Returns          void
**
*******************************************************************************/
void bta_avk_co_video_start(tBTA_AVK_CODEC codec_type)
{
}

#if (BTU_BTC_SNK_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         bta_avk_co_video_btc_start
**
** Description      This function is called by AV when the video streaming data
**                  transfer is started.
**
**
** Returns          void
**
*******************************************************************************/
void bta_avk_co_video_btc_start(void)
{
    APPL_TRACE_DEBUG0("bta_avk_co_video_btc_start");
}

#endif
/*******************************************************************************
**
** Function         bta_avk_co_video_stop
**
** Description      This function is called by AV when the video streaming data
**                  transfer is stopped.
**
**
** Returns          void
**
*******************************************************************************/
void bta_avk_co_video_stop(void)
{
}


/*******************************************************************************
**
** Function         bta_avk_co_video_src_data_path
**
** Description      This function is called to get the next data buffer from
**                  the video codec.
**
** Returns          NULL if data is not ready.
**                  Otherwise, a video data buffer (UINT8*).
**
*******************************************************************************/
void bta_avk_co_video_data(tBTA_AVK_CODEC codec_type, UINT8 *p_media, UINT32 media_len,
                                                    UINT32 timestamp, UINT16 seq_num, UINT8 m_pt, UINT8 avdt_handle)
{
    APPL_TRACE_DEBUG0("bta_avk_co_video_data");
    switch(codec_type)
    {
    case BTA_AVK_CODEC_H263_P0:
    case BTA_AVK_CODEC_H263_P3:
    case BTA_AVK_CODEC_H263_P8:
    case BTA_AVK_CODEC_MPEG4:
        //xieqi btapp_avk_codec_video_write(p_media, media_len, timestamp, seq_num);
        break;

    case BTA_AVK_CODEC_VEND:
        break;

    default:
        break;
    }
}

/*******************************************************************************
**
** Function         bta_avk_co_video_report_conn
**
** Description      This function is called by AV when the reporting channel is
**                  opened (open=TRUE) or closed (open=FALSE).
**
** Returns          void
**
*******************************************************************************/
void bta_avk_co_video_report_conn (BOOLEAN open, UINT8 avdt_handle)
{
}

/*******************************************************************************
**
** Function         bta_avk_co_video_report_rr
**
** Description      This function is called by AV when a Receiver Report is
**                  received
**
** Returns          void
**
*******************************************************************************/
void bta_avk_co_video_report_rr (UINT32 packet_lost)
{
}

/*******************************************************************************
**
** Function         bta_avk_co_video_delay
**
** Description      This function is called by AV when the video stream connection
**                  needs to send the initial delay report to the connected SRC.
**
**
** Returns          void
**
*******************************************************************************/
void bta_avk_co_video_delay(UINT8 err)
{
    APPL_TRACE_DEBUG0("bta_avk_co_video_delay");
    BTA_AvkDelayReport(BTA_AVK_CHNL_VIDEO, bta_avk_co_cur_video_delay);
}

#endif

