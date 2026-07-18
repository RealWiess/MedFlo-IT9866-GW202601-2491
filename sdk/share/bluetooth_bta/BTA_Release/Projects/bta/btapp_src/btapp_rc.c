/****************************************************************************
**
**  Name:          btapp_rc.c
**
**  Description:   Contains application functions for AVRCP Metadata transfer
**                 and advanced control
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bt_target.h"
#include "gki.h"

#if ( defined BTA_AV_INCLUDED ) && ( BTA_AV_INCLUDED == TRUE )

#include "bta_platform.h"
#include "bte_glue.h"
#include "btapp_av.h"

/*
 *  btapp_meta_settings[] contains company and event lists (1024 bytes) as well as
 *      attribute settings [4 mandatory ones (4096 bytes) plus up to 8 menu extension (1024 bytes)].
 *  btapp_song_data[] contains song title, artist, album and genre (1024 bytes).
 *
 *  This values totally depend on the definition of BTAPP_AV_META_MAX_STR_LEN constant.
 *  The size of buffer can be configured based on this contstant.
 */
UINT8   btapp_meta_settings[BTAPP_AV_META_MAX_STR_LEN * 4 * 6];

/*
 *  btapp_song_data[] contains song title, artist, album, genre, track, total track,
 *  and playing time.
 *
 *  This values totally depend on the definition of BTAPP_AV_META_MAX_STR_LEN constant.
 *  The max length of track, total track and playing time will be 10 bytes each.
 */

UINT8   btapp_song_data[BTAPP_AV_META_SONG_DATA_LEN];
/*  btapp_file_data is similar to btapp_song_data[], but for retriving the file infor for browsing purposes */
UINT8   btapp_file_data[BTAPP_AV_META_SONG_DATA_LEN];

/* Set this to TRUE, when p_bta_av_cfg->rc_pass_rsp/BTA_AV_RC_PASS_RSP_CODE is BTA_AV_RSP_INTERIM.
 * the application code needs to process the request not marked as implemented in p_bta_av_rc_id.
 * If AVRC_ADV_CTRL_INCLUDED is not supported,
      p_bta_av_cfg->rc_pass_rsp/BTA_AV_RC_PASS_RSP_CODE shall be AVRC_RSP_NOT_IMPL
      and BTAPP_AV_RC_CONV_RC_ID shall be FALSE */
#ifndef BTAPP_AV_RC_CONV_RC_ID
#define BTAPP_AV_RC_CONV_RC_ID       FALSE
#endif

#if (defined(BTA_AV_RC_PASS_RSP_CODE) && (BTA_AV_RC_PASS_RSP_CODE == AVRC_RSP_INTERIM))
#undef BTAPP_AV_RC_CONV_RC_ID
#define BTAPP_AV_RC_CONV_RC_ID   TRUE
#endif

#if  (BTAPP_AV_RC_CONV_RC_ID == TRUE)
/* BTAPP_AV_RC_ID_LOW to BTAPP_AV_RC_ID_HIGH is the range of AVRCP/AVC ID that is implemented differently
 * by the players. If All players have the same capabilities or only one player is supported,
 * it is advised to set p_bta_av_cfg->rc_pass_rsp/BTA_AV_RC_PASS_RSP_CODE as AVRC_RSP_NOT_IMPL
 * and BTAPP_AV_RC_CONV_RC_ID as FALSE */
#define BTAPP_AV_RC_ID_LOW       AVRC_ID_PLAY
#define BTAPP_AV_RC_ID_HIGH      AVRC_ID_BACKWARD
/* BTAPP_AV_RC_NO_SUPPORT is set to a bit number that is not supported by all players.
 * 120 is bit 0 of  octet 15 */
#define BTAPP_AV_RC_NO_SUPPORT   120
/* the number in this table is the bit nomber in "Feature Bit Mask" */
const UINT8 btapp_rc_id_map [] =
{
    AVRC_PF_PLAY_BIT_NO,    /* play */
    AVRC_PF_STOP_BIT_NO,    /* stop */
    AVRC_PF_PAUSE_BIT_NO,   /* pause */
    BTAPP_AV_RC_NO_SUPPORT,  /* record */
    BTAPP_AV_RC_NO_SUPPORT,  /* rewind */
    BTAPP_AV_RC_NO_SUPPORT,  /* fast forward */
    BTAPP_AV_RC_NO_SUPPORT,  /* eject */
    AVRC_PF_FORWARD_BIT_NO, /* forward */
    AVRC_PF_BACKWARD_BIT_NO /* backward */
};
#endif /* BTAPP_AV_RC_CONV_RC_ID */

#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
tAVRC_ITEM_PLAYER btapp_av_player1_file =
{
    /* A unique identifier for this media player.*/
    BTAPP_AV_PLAYER_ID_FILES,
    /* Use AVRC_MJ_TYPE_AUDIO, AVRC_MJ_TYPE_VIDEO, AVRC_MJ_TYPE_BC_AUDIO, or AVRC_MJ_TYPE_BC_VIDEO.*/
    (AVRC_MJ_TYPE_AUDIO|AVRC_MJ_TYPE_VIDEO),
    /* Use AVRC_SUB_TYPE_NONE, AVRC_SUB_TYPE_AUDIO_BOOK, or AVRC_SUB_TYPE_PODCAST*/
    AVRC_SUB_TYPE_NONE,
    /* Use AVRC_PLAYSTATE_STOPPED, AVRC_PLAYSTATE_PLAYING, AVRC_PLAYSTATE_PAUSED, AVRC_PLAYSTATE_FWD_SEEK,
           AVRC_PLAYSTATE_REV_SEEK, or AVRC_PLAYSTATE_ERROR*/
    AVRC_PLAYSTATE_STOPPED,
    {   /* Supported feature bit mask*/
        0x1F, /* octet 0 bit mask: 0=SELECT, 1=UP, 2=DOWN, 3=LEFT,
                                   4=RIGHT, 5=RIGHT_UP, 6=RIGHT_DOWN, 7=LEFT_UP */
        0x02, /* octet 1 bit mask: 0=LEFT_DOWN, 1=ROOT_MENU, 2=SETUP_MENU, 3=CONT_MENU,
                                   4=FAV_MENU, 5=EXIT, 6=0, 7=1 */
        0x00, /* octet 2 bit mask: 0=2, 1=3, 2=4, 3=5,
                                   4=6, 5=7, 6=8, 7=9 */
        0x18, /* octet 3 bit mask: 0=DOT, 1=ENTER, 2=CLEAR, 3=CHAN_UP,
                                   4=CHAN_DOWN, 5=PREV_CHAN, 6=SOUND_SEL, 7=INPUT_SEL */
        0x00, /* octet 4 bit mask: 0=DISP_INFO, 1=HELP, 2=PAGE_UP, 3=PAGE_DOWN,
                                   4=POWER, 5=VOL_UP, 6=VOL_DOWN, 7=MUTE */
#if  (BTAPP_AV_RC_CONV_RC_ID == TRUE)
        0x87, /* octet 5 bit mask: 0=PLAY, 1=STOP, 2=PAUSE, 3=RECORD,
                                   4=REWIND, 5=FAST_FOR, 6=EJECT, 7=FORWARD */
        0x01, /* octet 6 bit mask: 0=BACKWARD, 1=ANGLE, 2=SUBPICT, 3=F1,
                                   4=F2, 5=F3, 6=F4, 7=F5 */
#else
        0x87, /* octet 5 bit mask: 0=PLAY, 1=STOP, 2=PAUSE, 3=RECORD,
                                   4=REWIND, 5=FAST_FOR, 6=EJECT, 7=FORWARD */
        0x01, /* octet 6 bit mask: 0=BACKWARD, 1=ANGLE, 2=SUBPICT, 3=F1,
                                   4=F2, 5=F3, 6=F4, 7=F5 */
#endif
        0x6C, /* octet 7 bit mask: 0=PassThru Vendor, 1=GroupNavi, 2=AdvCtrl, 3=Browsing,
                                   4=Searching, 5=AddToNowPlaying, 6=UID_unique, 7=BrowsableWhenAddressed */
        0x06, /* octet 8 bit mask: 0=SearchableWhenAddressed, 1=NowPlaying, 2=UID_Persistency */
        0x00, /* octet 9 bit mask: not used */
        0x00, /* octet 10 bit mask: not used */
        0x00, /* octet 11 bit mask: not used */
        0x00, /* octet 12 bit mask: not used */
        0x00, /* octet 13 bit mask: not used */
        0x00, /* octet 14 bit mask: not used */
        0x00  /* octet 15 bit mask: not used */
    },
    {   /* The player name, name length and character set id.*/
        AVRC_CHARSET_ID_UTF8,
        11,
        (UINT8 *)"File Player"
    }
};

tAVRC_ITEM_PLAYER btapp_av_player2_media_player =
{
    /* A unique identifier for this media player.*/
    BTAPP_AV_PLAYER_ID_MPLAYER,
    /* Use AVRC_MJ_TYPE_AUDIO, AVRC_MJ_TYPE_VIDEO, AVRC_MJ_TYPE_BC_AUDIO, or AVRC_MJ_TYPE_BC_VIDEO.*/
    AVRC_MJ_TYPE_AUDIO,
    /* Use AVRC_SUB_TYPE_NONE, AVRC_SUB_TYPE_AUDIO_BOOK, or AVRC_SUB_TYPE_PODCAST*/
    AVRC_SUB_TYPE_NONE,
    /* Use AVRC_PLAYSTATE_STOPPED, AVRC_PLAYSTATE_PLAYING, AVRC_PLAYSTATE_PAUSED, AVRC_PLAYSTATE_FWD_SEEK,
       AVRC_PLAYSTATE_REV_SEEK, or AVRC_PLAYSTATE_ERROR*/
    AVRC_PLAYSTATE_STOPPED,
    {   /* Supported feature bit mask*/
        0x1F, /* octet 0 bit mask: 0=SELECT, 1=UP, 2=DOWN, 3=LEFT,
                                   4=RIGHT, 5=RIGHT_UP, 6=RIGHT_DOWN, 7=LEFT_UP */
        0x02, /* octet 1 bit mask: 0=LEFT_DOWN, 1=ROOT_MENU, 2=SETUP_MENU, 3=CONT_MENU,
                                   4=FAV_MENU, 5=EXIT, 6=0, 7=1 */
        0x00, /* octet 2 bit mask: 0=2, 1=3, 2=4, 3=5,
                                   4=6, 5=7, 6=8, 7=9 */
        0x18, /* octet 3 bit mask: 0=DOT, 1=ENTER, 2=CLEAR, 3=CHAN_UP,
                                   4=CHAN_DOWN, 5=PREV_CHAN, 6=SOUND_SEL, 7=INPUT_SEL */
        0x00, /* octet 4 bit mask: 0=DISP_INFO, 1=HELP, 2=PAGE_UP, 3=PAGE_DOWN,
                                   4=POWER, 5=VOL_UP, 6=VOL_DOWN, 7=MUTE */
        0x87, /* octet 5 bit mask: 0=PLAY, 1=STOP, 2=PAUSE, 3=RECORD,
                                   4=REWIND, 5=FAST_FOR, 6=EJECT, 7=FORWARD */
        0x01, /* octet 6 bit mask: 0=BACKWARD, 1=ANGLE, 2=SUBPICT, 3=F1,
                                   4=F2, 5=F3, 6=F4, 7=F5 */
        0x00, /* octet 7 bit mask: 0=PassThru Vendor, 1=GroupNavi, 2=AdvCtrl, 3=Browsing,
                                   4=Searching, 5=AddToNowPlaying, 6=UID_unique, 7=BrowsableWhenAddressed */
        0x00, /* octet 8 bit mask: 0=SearchableWhenAddressed, 1=NowPlaying, 2=UID_Persistency */
        0x00, /* octet 9 bit mask: not used */
        0x00, /* octet 10 bit mask: not used */
        0x00, /* octet 11 bit mask: not used */
        0x00, /* octet 12 bit mask: not used */
        0x00, /* octet 13 bit mask: not used */
        0x00, /* octet 14 bit mask: not used */
        0x00  /* octet 15 bit mask: not used */
    },
    {   /* The player name, name length and character set id.*/
        AVRC_CHARSET_ID_UTF8,
        12,
        (UINT8 *)"Media Player"
    }
};

tAVRC_ITEM_PLAYER btapp_av_player3_fm =
{
    /* A unique identifier for this media player.*/
    BTAPP_AV_PLAYER_ID_FM,
    /* Use AVRC_MJ_TYPE_AUDIO, AVRC_MJ_TYPE_VIDEO, AVRC_MJ_TYPE_BC_AUDIO, or AVRC_MJ_TYPE_BC_VIDEO.*/
    (AVRC_MJ_TYPE_BC_AUDIO),
    /* Use AVRC_SUB_TYPE_NONE, AVRC_SUB_TYPE_AUDIO_BOOK, or AVRC_SUB_TYPE_PODCAST*/
    AVRC_SUB_TYPE_NONE,
    /* Use AVRC_PLAYSTATE_STOPPED, AVRC_PLAYSTATE_PLAYING, AVRC_PLAYSTATE_PAUSED, AVRC_PLAYSTATE_FWD_SEEK,
           AVRC_PLAYSTATE_REV_SEEK, or AVRC_PLAYSTATE_ERROR*/
    AVRC_PLAYSTATE_STOPPED,
    {   /* Supported feature bit mask*/
        0x1F, /* octet 0 bit mask: 0=SELECT, 1=UP, 2=DOWN, 3=LEFT,
                                   4=RIGHT, 5=RIGHT_UP, 6=RIGHT_DOWN, 7=LEFT_UP */
        0x02, /* octet 1 bit mask: 0=LEFT_DOWN, 1=ROOT_MENU, 2=SETUP_MENU, 3=CONT_MENU,
                                   4=FAV_MENU, 5=EXIT, 6=0, 7=1 */
        0x00, /* octet 2 bit mask: 0=2, 1=3, 2=4, 3=5,
                                   4=6, 5=7, 6=8, 7=9 */
        0x18, /* octet 3 bit mask: 0=DOT, 1=ENTER, 2=CLEAR, 3=CHAN_UP,
                                   4=CHAN_DOWN, 5=PREV_CHAN, 6=SOUND_SEL, 7=INPUT_SEL */
        0x00, /* octet 4 bit mask: 0=DISP_INFO, 1=HELP, 2=PAGE_UP, 3=PAGE_DOWN,
                                   4=POWER, 5=VOL_UP, 6=VOL_DOWN, 7=MUTE */
        0x87, /* octet 5 bit mask: 0=PLAY, 1=STOP, 2=PAUSE, 3=RECORD,
                                   4=REWIND, 5=FAST_FOR, 6=EJECT, 7=FORWARD */
        0x01, /* octet 6 bit mask: 0=BACKWARD, 1=ANGLE, 2=SUBPICT, 3=F1,
                                   4=F2, 5=F3, 6=F4, 7=F5 */
        0x00, /* octet 7 bit mask: 0=PassThru Vendor, 1=GroupNavi, 2=AdvCtrl, 3=Browsing,
                                   4=Searching, 5=AddToNowPlaying, 6=UID_unique, 7=BrowsableWhenAddressed */
        0x00, /* octet 8 bit mask: 0=SearchableWhenAddressed, 1=NowPlaying, 2=UID_Persistency */
        0x00, /* octet 9 bit mask: not used */
        0x00, /* octet 10 bit mask: not used */
        0x00, /* octet 11 bit mask: not used */
        0x00, /* octet 12 bit mask: not used */
        0x00, /* octet 13 bit mask: not used */
        0x00, /* octet 14 bit mask: not used */
        0x00  /* octet 15 bit mask: not used */
    },
    {   /* The player name, name length and character set id.*/
        AVRC_CHARSET_ID_UTF8,
        8,
        (UINT8 *)"FM Radio"
    }
};

/* do not use const - the player state may be changed by platform code */
tAVRC_ITEM_PLAYER * btapp_av_players [] =
{
    /* player1 - when using files (wave, SBC, mp3) in btapp_app */
    &btapp_av_player1_file,
    /* player2 - when using windows media player */
    &btapp_av_player2_media_player,
    /* player3 - when using FM Radio */
    &btapp_av_player3_fm
};

const tAVRC_FEATURE_MASK btapp_av_empty_mask =     {   /* Supported feature bit mask*/
        0x00, /* octet 0 bit mask: 0=SELECT, 1=UP, 2=DOWN, 3=LEFT,
                                   4=RIGHT, 5=RIGHT_UP, 6=RIGHT_DOWN, 7=LEFT_UP */
        0x00, /* octet 1 bit mask: 0=LEFT_DOWN, 1=ROOT_MENU, 2=SETUP_MENU, 3=CONT_MENU,
                                   4=FAV_MENU, 5=EXIT, 6=0, 7=1 */
        0x00, /* octet 2 bit mask: 0=2, 1=3, 2=4, 3=5,
                                   4=6, 5=7, 6=8, 7=9 */
        0x18, /* octet 3 bit mask: 0=DOT, 1=ENTER, 2=CLEAR, 3=CHAN_UP,
                                   4=CHAN_DOWN, 5=PREV_CHAN, 6=SOUND_SEL, 7=INPUT_SEL */
        0x00, /* octet 4 bit mask: 0=DISP_INFO, 1=HELP, 2=PAGE_UP, 3=PAGE_DOWN,
                                   4=POWER, 5=VOL_UP, 6=VOL_DOWN, 7=MUTE */
        0x07, /* octet 5 bit mask: 0=PLAY, 1=STOP, 2=PAUSE, 3=RECORD,
                                   4=REWIND, 5=FAST_FOR, 6=EJECT, 7=FORWARD */
        0x00, /* octet 6 bit mask: 0=BACKWARD, 1=ANGLE, 2=SUBPICT, 3=F1,
                                   4=F2, 5=F3, 6=F4, 7=F5 */
        0x00, /* octet 7 bit mask: 0=PassThru Vendor, 1=GroupNavi, 2=AdvCtrl, 3=Browsing,
                                   4=Searching, 5=AddToNowPlaying, 6=UID_unique, 7=BrowsableWhenAddressed */
        0x00, /* octet 8 bit mask: 0=SearchableWhenAddressed, 1=NowPlaying, 2=UID_Persistency */
        0x00, /* octet 9 bit mask: not used */
        0x00, /* octet 10 bit mask: not used */
        0x00, /* octet 11 bit mask: not used */
        0x00, /* octet 12 bit mask: not used */
        0x00, /* octet 13 bit mask: not used */
        0x00, /* octet 14 bit mask: not used */
        0x00  /* octet 15 bit mask: not used */
    };

#endif

/*
 * Valid Attribute range for Extension menu should be in sync with IsValidAttribValue() function.
 */
#define BTAPP_RC_IS_SUPPORTED_ATTR(a)     (((((a > 0) && a <= AVRC_PLAYER_SETTING_SCAN)) || \
                                   ((a >= BTAPP_META_PLAYER_SETTING_MENU_EXT_1) && \
                                    (a <= BTAPP_META_PLAYER_SETTING_MENU_EXT_4))) ? TRUE : FALSE)

#define BTAPP_RC_IS_VALID_MEDIA_ATTR(a) ((a >= BTAPP_META_ATTR_ID_TITLE) && \
                                  (a <= BTAPP_META_ATTR_ID_PLAYING_TIME) ? TRUE : FALSE)


static BOOLEAN btapp_rc_is_valid_attrib_value(UINT8 attrib, UINT8 value);

extern void btapp_av_meta_cfg_init(UINT8 *p_buffer1, UINT8 *p_buffer2);
extern BT_HDR * btapp_platform_rc_get_items( UINT8 rc_handle, tAVRC_GET_ITEMS_CMD *p_cmd, tAVRC_GET_ITEMS_RSP *p_rsp );
extern void btapp_platform_rc_chg_path(tAVRC_CHG_PATH_CMD *p_cmd, tAVRC_CHG_PATH_RSP *p_rsp);
extern void btapp_av_audio_read_cur_meta_record(void);
extern UINT8 btapp_platform_rc_get_item_attrs(UINT16 charset_id, tAVRC_GET_ATTRS_CMD *p_cmd,
                                             tAVRC_ATTR_ENTRY *p_entries, char *p_name);
extern UINT8 btapp_platform_rc_get_attrs(UINT16 charset_id, UINT8 attr_count, UINT32 *p_attr_list,
                                        tAVRC_ATTR_ENTRY *p_entries, char *p_name);
static void btapp_rc_complete_notification(UINT8 event_id, tBTA_AV_CODE rsp_code);
extern void btapp_platform_rc_init(void);
#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
static UINT8 * btapp_rc_get_player_features(UINT16 player_id);
#endif
extern void btapp_platform_rc_cur_path( void );
extern UINT32 btapp_platform_rc_num_items_of_path(char *p_path);

tBTAPP_AV_CB btapp_av_cb;

/*******************************************************************************
**
** Function         btapp_rc_init
**
** Description      called by btapp_av_init() to initialize the metadata info
**
** Returns          none
*******************************************************************************/
void btapp_rc_init(void)
{
    /* TODO initialize metadata similar to cfg in a const data type */
//    btapp_av_meta_cfg_init(&btapp_meta_settings[0], &btapp_song_data[0]);
    /* this is the default addressed player */
    btapp_av_cb.meta_info.addr_player_id = BTAPP_AV_DEFAULT_ADDR_PLAYER;
    /* this is the default browsed player */
    btapp_av_cb.meta_info.br_player_id = BTAPP_AV_DEFAULT_BR_PLAYER;
    /* if database unaware player -> always 0. Otherwise, starts with a non-0 value */
    btapp_av_cb.meta_info.cur_uid_counter = 0;

//    btapp_platform_rc_init();
}

/*******************************************************************************
**
** Function         btapp_rc_pass_cmd
**
** Description      called by btapp_av_init() to initialize the metadata info
**
** Returns          none
*******************************************************************************/
#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
void btapp_rc_pass_cmd(tBTA_AV_REMOTE_CMD *p_cmd)
{
    tBTA_AV_CODE    rsp_code = AVRC_RSP_NOT_IMPL;
    tAVRC_MSG_PASS  pass;
#if  (BTAPP_AV_RC_CONV_RC_ID == TRUE)
    tBTA_AV_RC      rc_idx;
    UINT8           bit_no;
    UINT8           *p_mask;
#endif

    APPL_TRACE_DEBUG2("btapp_rc_pass_cmd ctype: %d rc_id:0x%x", p_cmd->hdr.ctype, p_cmd->rc_id);
    if (p_cmd->hdr.ctype == AVRC_RSP_INTERIM)
    {
#if  (BTAPP_AV_RC_CONV_RC_ID == TRUE)
        if (p_cmd->rc_id >= BTAPP_AV_RC_ID_LOW && p_cmd->rc_id <= BTAPP_AV_RC_ID_HIGH)
        {
            rc_idx = p_cmd->rc_id - BTAPP_AV_RC_ID_LOW;
            bit_no = btapp_rc_id_map[rc_idx];
            p_mask = btapp_rc_get_player_features(btapp_av_cb.meta_info.addr_player_id);
            APPL_TRACE_DEBUG4("bit_no:%d, octet: %d, mask:x%x/x%x", bit_no, (bit_no >> 3),
                (1 << (bit_no & 0x07)), p_mask[bit_no >> 3]);
            if (p_mask[bit_no >> 3] & (1 << (bit_no & 0x07)))
            {
                rsp_code = BTA_AV_RSP_ACCEPT;
            }

        }
#endif
        pass.op_id = p_cmd->rc_id;
        pass.state = p_cmd->key_state;
        pass.pass_len = p_cmd->len;
        pass.p_pass_data = p_cmd->p_data;
        memcpy(&pass.hdr, &p_cmd->hdr, sizeof (tAVRC_HDR));
        pass.hdr.ctype = rsp_code;
        AVRC_PassRsp(p_cmd->rc_handle, p_cmd->label, &pass);
    }
}
#endif

/*******************************************************************************
**
** Function         btapp_rc_check_reset
**
** Description      reset the  addr_player_id & br_player_id to the default one
**
** Returns          none
*******************************************************************************/
void btapp_rc_check_reset(void)
{
    /* this is the default addressed player */
    btapp_av_cb.meta_info.addr_player_id = BTAPP_AV_DEFAULT_ADDR_PLAYER;
    /* this is the default browsed player */
    btapp_av_cb.meta_info.br_player_id = BTAPP_AV_DEFAULT_BR_PLAYER;
}

/*******************************************************************************
**
** Function         btapp_rc_validate_pdu
**
** Description      make sure the requested pdu id is valid with the current enabled features.
**
** Returns          AVRC_STS_NO_ERROR, if no error
**
*******************************************************************************/
static tAVRC_STS btapp_rc_validate_pdu(tBTA_AV_FEAT av_features, UINT8 pdu_id)
{
    tAVRC_STS   sts = AVRC_STS_BAD_CMD;

    APPL_TRACE_DEBUG1("btapp_rc_validate_pdu av_features: x%x", av_features);
    if (av_features & BTA_AV_FEAT_METADATA)
    {
        switch (pdu_id)
        {
        case AVRC_PDU_GET_CAPABILITIES:            /* 0x10 */
        case AVRC_PDU_LIST_PLAYER_APP_ATTR:        /* 0x11 */
        case AVRC_PDU_LIST_PLAYER_APP_VALUES:      /* 0x12 */
        case AVRC_PDU_GET_CUR_PLAYER_APP_VALUE:    /* 0x13 */
        case AVRC_PDU_SET_PLAYER_APP_VALUE:        /* 0x14 */
        case AVRC_PDU_GET_PLAYER_APP_ATTR_TEXT:    /* 0x15 */
        case AVRC_PDU_GET_PLAYER_APP_VALUE_TEXT:   /* 0x16 */
        case AVRC_PDU_INFORM_DISPLAY_CHARSET:      /* 0x17 */
        case AVRC_PDU_INFORM_BATTERY_STAT_OF_CT:   /* 0x18 */
        case AVRC_PDU_GET_ELEMENT_ATTR:            /* 0x20 */
        case AVRC_PDU_GET_PLAY_STATUS:             /* 0x30 */
        case AVRC_PDU_REGISTER_NOTIFICATION:       /* 0x31 */
            sts = AVRC_STS_NO_ERROR;
            break;
        }
    }
#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
    if (av_features & BTA_AV_FEAT_ADV_CTRL)
    {
        switch (pdu_id)
        {
        case AVRC_PDU_SET_ABSOLUTE_VOLUME:         /* 0x50 */
        case AVRC_PDU_SET_ADDRESSED_PLAYER:        /* 0x60 */
        case AVRC_PDU_PLAY_ITEM:                   /* 0x74 */
        case AVRC_PDU_ADD_TO_NOW_PLAYING:          /* 0x90 */
            sts = AVRC_STS_NO_ERROR;
            break;
        }
    }
    else if (pdu_id == AVRC_PDU_SET_ADDRESSED_PLAYER)
    {
        /* allow the addressed player to be switched to any player */
        sts = AVRC_STS_NO_ERROR;
    }

    if (av_features & BTA_AV_FEAT_BROWSE)
    {
        switch (pdu_id)
        {
        case AVRC_PDU_SET_BROWSED_PLAYER:            /* 0x70 */
        case AVRC_PDU_GET_FOLDER_ITEMS:              /* 0x71 */
        case AVRC_PDU_CHANGE_PATH:                   /* 0x72 */
        case AVRC_PDU_GET_ITEM_ATTRIBUTES:           /* 0x73 */
#if (AVRC_1_6_INCLUDED == TRUE)
        case AVRC_PDU_GET_TOTAL_NUM_OF_ITEMS:        /* 0x75 */
#endif
        case AVRC_PDU_SEARCH:                        /* 0x80 */
            sts = AVRC_STS_NO_ERROR;
            break;
        }
    }
#endif
    return sts;
}

#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         btapp_rc_chk_player_id
**
** Description      make sure the requested player id is valid.
**
** Returns          AVRC_STS_NO_ERROR, if no error
**
*******************************************************************************/
static tAVRC_STS btapp_rc_chk_player_id(UINT16 player_id)
{
    tAVRC_STS   status = AVRC_STS_NO_ERROR;
    UINT8       xx;

    /* make sure the player_id is valid */
    for (xx=0; xx<BTAPP_AV_NUM_MPLAYERS; xx++)
    {
        if (btapp_av_players[xx] && player_id == btapp_av_players[xx]->player_id)
        {
            break;
        }
    }
    if (xx == BTAPP_AV_NUM_MPLAYERS)
    {
        status = AVRC_STS_BAD_PLAYER_ID;
    }

    return status;
}

/*******************************************************************************
**
** Function         btapp_rc_get_player_features
**
** Description      retrieve the player feature bit mask of the given playet id
**
** Returns          the player feature bit mask
**
*******************************************************************************/
static UINT8 * btapp_rc_get_player_features(UINT16 player_id)
{
    UINT8 *p_mask = (UINT8 *)btapp_av_empty_mask;
    tAVRC_STS   status = AVRC_STS_NO_ERROR;
    UINT8       xx;

    /* make sure the player_id is valid */
    for (xx=0; xx<BTAPP_AV_NUM_MPLAYERS; xx++)
    {
        if (btapp_av_players[xx] && player_id == btapp_av_players[xx]->player_id)
        {
            p_mask = btapp_av_players[xx]->features;
            break;
        }
    }

    return p_mask;
}
#endif

/*******************************************************************************
**
** Function         btapp_rc_proc_list_attrib
**
** Description      composes the AVRC_PDU_LIST_PLAYER_APP_ATTR response
**
** Returns          the GKI message that contains the response
**
*******************************************************************************/
static BT_HDR * btapp_rc_proc_list_attrib(tAVRC_LIST_APP_ATTR_RSP *p_rsp, tBTA_AV_CODE *p_code)
{
    BT_HDR  *p_rsp_pkt = NULL;
    UINT8   i, count = btapp_av_cb.meta_info.max_attrib_num;
    tBTAPP_AV_META_ATTRIB *p_pas_attrib;

    APPL_TRACE_DEBUG1("btapp_rc_proc_list_attrib : %d * * *", count);
    p_rsp->num_attr = 1;
    p_pas_attrib = &btapp_av_cb.meta_info.pas_info.equalizer;
    for (i = 0; i < count; i++)
    {
        p_rsp->attrs[0] = p_pas_attrib->attrib_id;
        AVRC_BldResponse (0, (tAVRC_RESPONSE *)p_rsp, &p_rsp_pkt);
        p_pas_attrib++;
    }
    *p_code = BTA_AV_RSP_IMPL_STBL;

    return p_rsp_pkt;
}

/*******************************************************************************
**
** Function         btapp_rc_proc_list_app_values
**
** Description      composes the AVRC_PDU_LIST_PLAYER_APP_VALUES response
**
** Returns          the GKI message that contains the response
**
*******************************************************************************/
static BT_HDR * btapp_rc_proc_list_app_values(tAVRC_LIST_APP_VALUES_CMD *p_cmd,
                                              tAVRC_LIST_APP_VALUES_RSP *p_rsp, tBTA_AV_CODE *p_code)
{
    BT_HDR  *p_rsp_pkt = NULL;
    UINT8   i, j, count = btapp_av_cb.meta_info.max_attrib_num;
    tBTAPP_AV_META_ATTRIB *p_pas_attrib;

    APPL_TRACE_DEBUG0("btapp_rc_proc_list_app_values");
    p_pas_attrib = &btapp_av_cb.meta_info.pas_info.equalizer;
    for (i = 0; i < count; i++)
    {
        if (p_pas_attrib->attrib_id == p_cmd->attr_id)
        {
            *p_code = BTA_AV_RSP_IMPL_STBL;
            p_rsp->num_val = 1;
            for (j=0; j<AVRC_MAX_APP_SETTINGS; j++)
            {
                if (p_pas_attrib->p_setting_str[j] && p_pas_attrib->p_setting_str[j][0])
                {
                    APPL_TRACE_DEBUG2("%d: %s", j, p_pas_attrib->p_setting_str[j]);
                    p_rsp->vals[0] = j + 1;
                    AVRC_BldResponse (0, (tAVRC_RESPONSE *)p_rsp, &p_rsp_pkt);
                }
                else
                    break;
            }
            break;
        }
        else
            p_pas_attrib++;
    }

    if (*p_code != BTA_AV_RSP_IMPL_STBL)
    {
        if (p_rsp_pkt)
        {
            GKI_freebuf (p_rsp_pkt);
            p_rsp_pkt = NULL;
        }
        /* must be BTA_AV_RSP_REJ */
        if (AVRC_IsValidPlayerAttr(p_cmd->attr_id))
        {
            /* a valid attribute, but not in the list -> not implemented */
            *p_code = BTA_AV_RSP_NOT_IMPL;
        }
        else
        {
            p_rsp->status = AVRC_STS_BAD_PARAM;
        }
    }

    return p_rsp_pkt;
}

/*******************************************************************************
**
** Function         btapp_rc_proc_get_cur_app_val
**
** Description      composes the AVRC_PDU_GET_CUR_PLAYER_APP_VALUE response
**
** Returns          the GKI message that contains the response
*******************************************************************************/
static BT_HDR * btapp_rc_proc_get_cur_app_val(tAVRC_GET_CUR_APP_VALUE_CMD *p_cmd,
                                              tAVRC_GET_CUR_APP_VALUE_RSP *p_rsp, tBTA_AV_CODE *p_code)
{
    BT_HDR  *p_rsp_pkt = NULL;
    UINT8   i, j, count = btapp_av_cb.meta_info.max_attrib_num;
    tBTAPP_AV_META_ATTRIB    *p_pas_attrib;
    tAVRC_APP_SETTING       app_setting;
    UINT8   xx=0;

    APPL_TRACE_DEBUG0("btapp_rc_proc_get_cur_app_val");
    p_pas_attrib = &btapp_av_cb.meta_info.pas_info.equalizer;
    p_rsp->num_val = 1;
    p_rsp->p_vals = &app_setting;
    for (i = 0; i < count; i++)
    {
        for (j = 0; j < p_cmd->num_attr; j++)
        {
            if (p_pas_attrib->attrib_id == p_cmd->attrs[j])
            {
                xx++;
                app_setting.attr_id = p_cmd->attrs[j];
                app_setting.attr_val = p_pas_attrib->curr_value;
                AVRC_BldResponse (0, (tAVRC_RESPONSE *)p_rsp, &p_rsp_pkt);
                break;
            }
        }

        APPL_TRACE_DEBUG3("i=%d, j=%d, attr:%d", i, j, p_cmd->num_attr);
        if (xx == p_cmd->num_attr)
        {
            /* found all attributes */
            *p_code = BTA_AV_RSP_IMPL_STBL;
            break;
        }

        if (j == p_cmd->num_attr)
        {
            *p_code = BTA_AV_RSP_NOT_IMPL;
        }
        p_pas_attrib++;
    }

    return p_rsp_pkt;
}

/*******************************************************************************
**
** Function         btapp_rc_proc_set_app_val
**
** Description      composes the AVRC_PDU_SET_PLAYER_APP_VALUE response
**
** Returns          the GKI message that contains the response
*******************************************************************************/
static BT_HDR * btapp_rc_proc_set_app_val(tAVRC_SET_APP_VALUE_CMD *p_cmd,
                                          tAVRC_RSP *p_rsp, tBTA_AV_CODE *p_code, UINT8 *p_evt_id)
{
    BT_HDR  *p_rsp_pkt = NULL;
    UINT8   i, j, count = btapp_av_cb.meta_info.max_attrib_num;
    tBTAPP_AV_META_ATTRIB    *p_pas_attrib;
    UINT8   xx=0;

    APPL_TRACE_DEBUG0("btapp_rc_proc_set_app_val");
    p_pas_attrib = &btapp_av_cb.meta_info.pas_info.equalizer;
    for (j = 0; j < p_cmd->num_val; j++)
    {
        if (!BTAPP_RC_IS_SUPPORTED_ATTR(p_cmd->p_vals[j].attr_id))
        {
            *p_code = BTA_AV_RSP_NOT_IMPL;
            p_rsp->status = AVRC_STS_BAD_PARAM;
            break;
        }
        else if (!btapp_rc_is_valid_attrib_value(p_cmd->p_vals[j].attr_id, p_cmd->p_vals[j].attr_val))
        {
            p_rsp->status = AVRC_STS_BAD_PARAM;
            break;
        }
    }

    if (p_rsp->status == AVRC_STS_NO_ERROR)
    {
        for (j = 0; j < p_cmd->num_val; j++)
        {
            p_pas_attrib = &btapp_av_cb.meta_info.pas_info.equalizer;
            for (i = 0; i < count; i++)
            {
                if (p_pas_attrib->attrib_id == p_cmd->p_vals[j].attr_id)
                {
                    xx++;
                    p_pas_attrib->curr_value = p_cmd->p_vals[j].attr_val;
                    break;
                }
                p_pas_attrib++;
            }

            APPL_TRACE_DEBUG3("i=%d, j=%d, attr:%d", i, j, p_cmd->num_val);
            if (xx == p_cmd->num_val)
            {
                /* found all attributes */
                *p_code = BTA_AV_RSP_ACCEPT;
                AVRC_BldResponse (0, (tAVRC_RESPONSE *)p_rsp, &p_rsp_pkt);
                *p_evt_id = AVRC_EVT_APP_SETTING_CHANGE;
                break;
            }

            if (j == p_cmd->num_val)
            {
                *p_code = BTA_AV_RSP_NOT_IMPL;
                p_rsp->status = AVRC_STS_BAD_PARAM;
                break;
            }
        }
    }

    return p_rsp_pkt;
}

/*******************************************************************************
**
** Function         btapp_rc_proc_get_app_attr_txt
**
** Description      composes the AVRC_PDU_GET_PLAYER_APP_ATTR_TEXT response
**
** Returns          the GKI message that contains the response
*******************************************************************************/
static BT_HDR * btapp_rc_proc_get_app_attr_txt(UINT16 charset_id, tAVRC_GET_APP_ATTR_TXT_CMD *p_cmd,
                                               tAVRC_GET_APP_ATTR_TXT_RSP *p_rsp, tBTA_AV_CODE *p_code)
{
    BT_HDR  *p_rsp_pkt = NULL;
    UINT8   i, j, count = btapp_av_cb.meta_info.max_attrib_num;
    tBTAPP_AV_META_ATTRIB    *p_pas_attrib;
    tAVRC_APP_SETTING_TEXT  app_setting;
    UINT8   xx=0;

    APPL_TRACE_DEBUG0("btapp_rc_proc_get_app_attr_txt");
    p_pas_attrib = &btapp_av_cb.meta_info.pas_info.equalizer;
    p_rsp->num_attr = 1;
    p_rsp->p_attrs = &app_setting;
    app_setting.charset_id = charset_id;
    for (i = 0; i < count; i++)
    {
        for (j = 0; j < p_cmd->num_attr; j++)
        {
            if (p_pas_attrib->attrib_id == p_cmd->attrs[j] && p_pas_attrib->p_attrib_str)
            {
                xx++;
                app_setting.attr_id = p_cmd->attrs[j];
                app_setting.p_str = p_pas_attrib->p_attrib_str;
                app_setting.str_len = strlen(p_pas_attrib->p_attrib_str);
                AVRC_BldResponse (0, (tAVRC_RESPONSE *)p_rsp, &p_rsp_pkt);
                break;
            }
        }

        APPL_TRACE_DEBUG4("i=%d, j=%d, attr:%d, xx:%d", i, j, p_cmd->num_attr, xx);
        if (xx == p_cmd->num_attr)
        {
            /* found all attributes */
            break;
        }

        p_pas_attrib++;
    }

    if (xx)
    {
        /* found at least one attributes */
        *p_code = BTA_AV_RSP_IMPL_STBL;
    }
    else
    {
        *p_code = BTA_AV_RSP_NOT_IMPL;
    }

    return p_rsp_pkt;
}

/*******************************************************************************
**
** Function         btapp_rc_proc_get_app_val_txt
**
** Description      composes the AVRC_PDU_GET_PLAYER_APP_VALUE_TEXT response
**
** Returns          the GKI message that contains the response
*******************************************************************************/
static BT_HDR * btapp_rc_proc_get_app_val_txt(UINT16 charset_id, tAVRC_GET_APP_VAL_TXT_CMD *p_cmd,
                                              tAVRC_GET_APP_ATTR_TXT_RSP *p_rsp, tBTA_AV_CODE *p_code)
{
    BT_HDR  *p_rsp_pkt = NULL;
    UINT8   i, j, count = btapp_av_cb.meta_info.max_attrib_num;
    tBTAPP_AV_META_ATTRIB    *p_pas_attrib;
    tAVRC_APP_SETTING_TEXT  app_setting;
    UINT8   xx=0;
    UINT8   sel;
    tAVRC_STS   status = AVRC_STS_NO_ERROR;

    APPL_TRACE_DEBUG0("btapp_rc_proc_get_app_val_txt");
    p_pas_attrib = &btapp_av_cb.meta_info.pas_info.equalizer;
    p_rsp->num_attr = 1;
    p_rsp->p_attrs = &app_setting;
    app_setting.charset_id = charset_id;
    for (i = 0; i < count; i++, p_pas_attrib++)
    {
        j = 0;
        if (p_pas_attrib->attrib_id == p_cmd->attr_id)
        {
            for (; j < p_cmd->num_val; j++)
            {
                if (btapp_rc_is_valid_attrib_value ( p_cmd->attr_id, p_cmd->vals[j]) )
                {
                    sel = p_cmd->vals[j] - 1;
                    app_setting.attr_id = p_cmd->vals[j];
                    app_setting.p_str = p_pas_attrib->p_setting_str[sel];
                    app_setting.str_len = strlen(p_pas_attrib->p_setting_str[sel]);
                    AVRC_BldResponse (0, (tAVRC_RESPONSE *)p_rsp, &p_rsp_pkt);
                    xx++;
                }
                else
                    status = AVRC_STS_BAD_PARAM;
            }
        }

        APPL_TRACE_DEBUG4("i=%d, j=%d, attr:%d, xx:%d", i, j, p_cmd->num_val, xx);
        if (xx == p_cmd->num_val)
        {
            /* found all attributes */
            break;
        }
    }

    if (xx)
    {
        /* found at least one attributes */
        *p_code = BTA_AV_RSP_IMPL_STBL;
    }
    else
    {
        if (status == AVRC_STS_NO_ERROR)
            *p_code = BTA_AV_RSP_NOT_IMPL;
        else
            p_rsp->status = status;
    }

    return p_rsp_pkt;
}

/*******************************************************************************
**
** Function         btapp_rc_proc_inform_charset
**
** Description      composes the AVRC_PDU_INFORM_DISPLAY_CHARSET response
**
** Returns          the GKI message that contains the response
*******************************************************************************/
static tAVRC_STS btapp_rc_proc_inform_charset(UINT8 rc_handle, tAVRC_INFORM_CHARSET_CMD *p_cmd,
                                              tBTA_AV_CODE *p_code)
{
    UINT16  charset_id = btapp_av_cb.meta_info.charset_id[rc_handle];
    UINT8   xx;
    tAVRC_STS   status = AVRC_STS_NO_ERROR;

    APPL_TRACE_DEBUG0("btapp_rc_proc_inform_charset");
    for (xx=0; xx<p_cmd->num_id; xx++)
    {
        if (p_cmd->charsets[xx] == AVRC_CHARSET_ID_UTF8 || p_cmd->charsets[xx] == AVRC_CHARSET_ID_ASCII)
        {
            *p_code = BTA_AV_RSP_ACCEPT;
            charset_id = p_cmd->charsets[xx];
            break;
        }
    }

    btapp_av_cb.meta_info.charset_id[rc_handle] = charset_id;

    if (*p_code != BTA_AV_RSP_ACCEPT)
        status = AVRC_STS_BAD_PARAM;
    return status;
}

/*******************************************************************************
**
** Function         btapp_rc_proc_inform_battery_status
**
** Description      composes the AVRC_PDU_INFORM_BATTERY_STAT_OF_CT response
**
** Returns          the GKI message that contains the response
*******************************************************************************/
static BT_HDR * btapp_rc_proc_inform_battery_status(tBTA_AV_CODE *p_code)
{
    BT_HDR  *p_rsp_pkt = NULL;

    APPL_TRACE_DEBUG0("btapp_rc_proc_inform_battery_status");
    *p_code = BTA_AV_RSP_ACCEPT;
    return p_rsp_pkt;
}

/*******************************************************************************
**
** Function         btapp_rc_proc_get_elem_attrs
**
** Description      composes the AVRC_PDU_GET_ELEMENT_ATTR response
**
** Returns          the GKI message that contains the response
*******************************************************************************/
static BT_HDR * btapp_rc_proc_get_elem_attrs(UINT16 charset_id, tAVRC_GET_ELEM_ATTRS_CMD *p_cmd,
                                             tAVRC_GET_ELEM_ATTRS_RSP *p_rsp, tBTA_AV_CODE *p_code)
{
    BT_HDR  *p_rsp_pkt = NULL;
    tAVRC_ATTR_ENTRY    attr_entry;
    UINT8   xx, attr_count = p_cmd->num_attr;
    UINT32  attr_list[AVRC_MAX_NUM_MEDIA_ATTR_ID], *p_attr;
    UINT8   *p_str;

    APPL_TRACE_DEBUG1("btapp_rc_proc_get_elem_attrs: %d", p_cmd->num_attr);
    /* read the meta data info of the current file into btapp_av_cb.meta_info.media_info */
//    btapp_av_audio_read_cur_meta_record();
    p_rsp->num_attr = 1;
    p_rsp->p_attrs  = &attr_entry;
    attr_entry.name.charset_id = charset_id;
    p_attr = p_cmd->attrs;

    if (attr_count == 0)
    {
        /* request all supported attributes */
        p_attr = attr_list;
        attr_count = AVRC_MAX_NUM_MEDIA_ATTR_ID;
        for (xx=0; xx<AVRC_MAX_NUM_MEDIA_ATTR_ID; xx++)
            attr_list[xx] = xx + 1;
    }

    for (xx=0; xx<attr_count; xx++)
    {
        attr_entry.attr_id = p_attr[xx];
        switch(attr_entry.attr_id)
        {
        case AVRC_MEDIA_ATTR_ID_TITLE:
            p_str =  btapp_av_cb.meta_info.media_info.p_song_title;
            break;
        case AVRC_MEDIA_ATTR_ID_ARTIST:
            p_str = btapp_av_cb.meta_info.media_info.p_artist_name;
            break;
        case AVRC_MEDIA_ATTR_ID_ALBUM:
            p_str = btapp_av_cb.meta_info.media_info.p_album_name;
            break;
        case AVRC_MEDIA_ATTR_ID_TRACK_NUM:
            p_str = btapp_av_cb.meta_info.media_info.p_track_num;
            break;
        case AVRC_MEDIA_ATTR_ID_NUM_TRACKS:
            p_str = btapp_av_cb.meta_info.media_info.p_total_track;
            break;
        case AVRC_MEDIA_ATTR_ID_GENRE:
            p_str = btapp_av_cb.meta_info.media_info.p_genre_name;
            break;
        case AVRC_MEDIA_ATTR_ID_PLAYING_TIME:
            p_str = btapp_av_cb.meta_info.media_info.p_playing_time;
            break;
        default:
            p_str = NULL;
        }

        if (p_str)
        {
            APPL_TRACE_DEBUG1("btapp adding: %d",  xx);
            attr_entry.name.str_len = strlen ((char *)p_str);
            attr_entry.name.p_str = p_str;
            AVRC_BldResponse (0, (tAVRC_RESPONSE *)p_rsp, &p_rsp_pkt);
        }
    }

    if (p_rsp_pkt == NULL)
    {
        *p_code = AVRC_RSP_REJ;
        p_rsp->status = AVRC_STS_NOT_FOUND;
    }
    else
        *p_code = AVRC_RSP_IMPL_STBL;

    return p_rsp_pkt;
}

/*******************************************************************************
**
** Function         btapp_rc_proc_get_play_status
**
** Description      composes the AVRC_PDU_GET_PLAY_STATUS response
**
** Returns          the GKI message that contains the response
*******************************************************************************/
static BT_HDR * btapp_rc_proc_get_play_status(tAVRC_GET_PLAY_STATUS_RSP *p_rsp, tBTA_AV_CODE *p_code)
{
    BT_HDR  *p_rsp_pkt = NULL;
    APPL_TRACE_DEBUG0("btapp_rc_proc_get_play_status");
    p_rsp->song_len = btapp_av_cb.meta_info.media_info.playing_time;
    p_rsp->song_pos = btapp_av_cb.meta_info.play_status.song_pos;
    p_rsp->play_status = btapp_av_cb.meta_info.play_status.play_status;
    *p_code = AVRC_RSP_IMPL_STBL;
    return p_rsp_pkt;
}

/*******************************************************************************
**
** Function         btapp_rc_proc_reg_notif
**
** Description      composes the AVRC_PDU_REGISTER_NOTIFICATION response
**
** Returns          the GKI message that contains the response
*******************************************************************************/
static BT_HDR * btapp_rc_proc_reg_notif(tBTA_AV_META_MSG *p_msg, tAVRC_REG_NOTIF_CMD *p_cmd,
                                        tAVRC_REG_NOTIF_RSP *p_rsp, tBTA_AV_CODE *p_code)
{
    BT_HDR  *p_rsp_pkt = NULL;
    UINT8   rc_handle = p_msg->rc_handle;
    UINT16  evt_mask = 1, index;
    UINT8   *p;
    UINT8   xx;
    tBTAPP_AV_META_ATTRIB *p_pas_attrib;
    tBTAPP_AV_AUDIO_ENT *p_ent;

    APPL_TRACE_DEBUG1("btapp_rc_proc_reg_notif 0x%04x",
                      btapp_av_cb.meta_info.registered_events[rc_handle].evt_mask);
    *p_code = BTA_AV_RSP_INTERIM;
    index = p_cmd->event_id - 1;
    evt_mask <<= index;
    /* Register event to the BTAPP AV control block  */
    btapp_av_cb.meta_info.registered_events[rc_handle].evt_mask |= evt_mask;
    btapp_av_cb.meta_info.registered_events[rc_handle].label[index] = p_msg->label;
    APPL_TRACE_DEBUG2("rc_handle:%d, evt_mask: 0x%04x", rc_handle, btapp_av_cb.meta_info.registered_events[rc_handle].evt_mask);

    p_rsp->event_id = p_cmd->event_id;
    switch(p_cmd->event_id)
    {
    case AVRC_EVT_PLAY_STATUS_CHANGE:   /* 0x01 */
        p_rsp->param.play_status = btapp_av_cb.meta_info.play_status.play_status;
        break;

    case AVRC_EVT_TRACK_CHANGE:         /* 0x02 */
        p = p_rsp->param.track;
        APPL_TRACE_DEBUG2("play_count %d cur_play %d",btapp_av_cb.play_count, btapp_av_cb.cur_play);
        /* Check whether folder and file is selected or not */
        if (btapp_av_cb.play_count == 0 )
        {
            /* If not, add track ID to be 0xFFFFFFFF-FFFFFFFF */
            UINT32_TO_BE_STREAM(p, AVRCP_NO_NOW_PLAYING_FOLDER_ID); /* the id from the no now playing folder */
            UINT32_TO_BE_STREAM(p, AVRCP_NO_NOW_PLAYING_FILE_ID);   /* the id from the no now playing file */
        }
        else
        {
            /* If selected, return 0x0 for interim response if AVRCP 1.4 or later */
            UINT32_TO_BE_STREAM(p, 0);
            UINT32_TO_BE_STREAM(p, 0);
        }
        break;

    case AVRC_EVT_TRACK_REACHED_END:    /* 0x03 */
    case AVRC_EVT_TRACK_REACHED_START:  /* 0x04 */
        break;

    case AVRC_EVT_PLAY_POS_CHANGED:     /* 0x05 */
        p_rsp->param.play_pos = btapp_av_cb.meta_info.play_status.song_pos;
        break;

    case AVRC_EVT_BATTERY_STATUS_CHANGE:/* 0x06 */
        p_rsp->param.battery_status = btapp_av_cb.meta_info.notif_info.bat_stat;
        break;

    case AVRC_EVT_SYSTEM_STATUS_CHANGE: /* 0x07 */
        p_rsp->param.system_status = btapp_av_cb.meta_info.notif_info.sys_stat;
        break;

    case AVRC_EVT_APP_SETTING_CHANGE:   /* 0x08 */
        p_rsp->param.player_setting.num_attr = AVRC_MAX_APP_SETTINGS;
        p_pas_attrib = &btapp_av_cb.meta_info.pas_info.equalizer;
        for (xx=0; xx<p_rsp->param.player_setting.num_attr; xx++)
        {
            p_rsp->param.player_setting.attr_id[xx] = p_pas_attrib->attrib_id;
            p_rsp->param.player_setting.attr_value[xx] = p_pas_attrib->curr_value;
            p_pas_attrib++;
        }
        break;

    case AVRC_EVT_NOW_PLAYING_CHANGE:   /* 0x09 no param */
    case AVRC_EVT_AVAL_PLAYERS_CHANGE:  /* 0x0a no param */
        break;

    case AVRC_EVT_ADDR_PLAYER_CHANGE:   /* 0x0b */
        p_rsp->param.addr_player.player_id = btapp_av_cb.meta_info.addr_player_id;
        /* UID counter is always 0 for Database Unaware Players */
        p_rsp->param.addr_player.uid_counter = btapp_av_cb.meta_info.cur_uid_counter;
        break;

    case AVRC_EVT_UIDS_CHANGE:          /* 0x0c */
        p_rsp->param.uid_counter = 0;
        break;

    case AVRC_EVT_VOLUME_CHANGE:        /* 0x0d */
        p_ent = btapp_av_get_audio_ent_by_rc_hndl(rc_handle);
        p_rsp->param.volume = 0;
        if (p_ent)
        {
            p_rsp->param.volume = p_ent->cur_volume;
        }
        break;

    default:
        *p_code = BTA_AV_RSP_NOT_IMPL;
    }

    if (*p_code == BTA_AV_RSP_INTERIM)
        AVRC_BldResponse (rc_handle, (tAVRC_RESPONSE *)p_rsp, &p_rsp_pkt);

    return p_rsp_pkt;
}

#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         btapp_rc_proc_addr_player
**
** Description      composes the AVRC_PDU_SET_ADDRESSED_PLAYER response
**
** Returns          the GKI message that contains the response
*******************************************************************************/
static BT_HDR * btapp_rc_proc_addr_player(tAVRC_SET_ADDR_PLAYER_CMD *p_cmd, tAVRC_RSP *p_rsp,
                                          tBTA_AV_CODE *p_code, UINT8 *p_evt_id, UINT16 *p_evt_mask)
{
    BT_HDR  *p_rsp_pkt = NULL;

    APPL_TRACE_DEBUG1("btapp_rc_proc_addr_player: %d", p_cmd->player_id);

    /* make sure the player_id is valid  */
    p_rsp->status = btapp_rc_chk_player_id (p_cmd->player_id);

    if (p_rsp->status == AVRC_STS_NO_ERROR)
    {
        /* the player id is valid.
         * allow set addressed player to all supported players just in case the CT wants to use pass thru commands */
        /* actually change the addressed player */
        btapp_av_cb.meta_info.addr_player_id = p_cmd->player_id;

        *p_evt_id   =   AVRC_EVT_ADDR_PLAYER_CHANGE;
        /* and report the events specified by the spec */
        *p_evt_mask =   BTAPP_AV_RC_EVT_MSK_PLAY_STATUS_CHANGE |
                        BTAPP_AV_RC_EVT_MSK_TRACK_CHANGE |
                        BTAPP_AV_RC_EVT_MSK_TRACK_REACHED_END |
                        BTAPP_AV_RC_EVT_MSK_TRACK_REACHED_START |
                        BTAPP_AV_RC_EVT_MSK_PLAY_POS_CHANGED |
                        BTAPP_AV_RC_EVT_MSK_APP_SETTING_CHANGE |
                        BTAPP_AV_RC_EVT_MSK_NOW_PLAYING_CHANGE;
        *p_code = BTA_AV_RSP_ACCEPT;
    }
    return p_rsp_pkt;
}

#if (AVCT_BROWSE_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         btapp_rc_proc_br_player
**
** Description      composes the AVRC_PDU_SET_BROWSED_PLAYER response
**
** Returns          the GKI message that contains the response
*******************************************************************************/
static BT_HDR * btapp_rc_proc_br_player(UINT16 charset_id, tAVRC_SET_BR_PLAYER_CMD *p_cmd,
                                tAVRC_SET_BR_PLAYER_RSP *p_rsp, tBTA_AV_CODE *p_code, UINT16 *p_evt_mask)
{
    BT_HDR  *p_rsp_pkt = NULL;
    UINT8   *p_mask;

    APPL_TRACE_DEBUG1("btapp_rc_proc_br_player: %d", p_cmd->player_id);
    /* make sure the player_id is valid */
    p_rsp->status = btapp_rc_chk_player_id (p_cmd->player_id);

    if (p_rsp->status == AVRC_STS_NO_ERROR)
    {
        /* the player id is valid now double check the player features */
        p_mask = btapp_rc_get_player_features(p_cmd->player_id);
        if (AVRC_PF_BROWSE_SUPPORTED(p_mask))
        {
            if (AVRC_PF_BR_WH_ADDR_SUPPORTED(p_mask) &&
                (btapp_av_cb.meta_info.addr_player_id != p_cmd->player_id))
            {
                p_rsp->status = AVRC_STS_PLAYER_N_ADDR;
            }
            else
            {
                /* actually change the browsed player and make sure the player is browsable */
                btapp_av_cb.meta_info.br_player_id = p_cmd->player_id;
                p_rsp->uid_counter  = 0;
                p_rsp->charset_id   = charset_id;
                p_rsp->folder_depth = btapp_av_cb.br_info.cur_depth;
                p_rsp->p_folders    = btapp_av_cb.br_info.folder_depth;
//                btapp_platform_rc_cur_path();
//                p_rsp->num_items    = btapp_platform_rc_num_items_of_path(btapp_av_cb.path);
                APPL_TRACE_DEBUG1("folder_depth: %d", p_rsp->folder_depth);
                *p_evt_mask =   BTAPP_AV_RC_EVT_MSK_UIDS_CHANGE;
                *p_code = BTA_AV_RSP_ACCEPT;
            }
        }
        else
            p_rsp->status = AVRC_STS_PLAYER_N_BR;
    }
    return p_rsp_pkt;
}

/*******************************************************************************
**
** Function         btapp_rc_proc_get_items
**
** Description      composes the AVRC_PDU_GET_FOLDER_ITEMS response
**
** Returns          the GKI message that contains the response
*******************************************************************************/
static BT_HDR * btapp_rc_proc_get_items(UINT8 rc_handle, tAVRC_GET_ITEMS_CMD *p_cmd,
                                        tAVRC_GET_ITEMS_RSP *p_rsp, tBTA_AV_CODE *p_code)
{
    BT_HDR  *p_rsp_pkt = NULL;
    tAVRC_ITEM  item;
    UINT16      xx;
    tAVRC_STS   res;

    APPL_TRACE_DEBUG1("btapp_rc_proc_get_items scope:%d", p_cmd->scope);
    switch (p_cmd->scope)
    {
    case AVRC_SCOPE_PLAYER_LIST:
        p_rsp->status = p_cmd->status;
        p_rsp->uid_counter = 0;
        p_rsp->item_count = 1;
        p_rsp->p_item_list = &item;
        item.item_type = AVRC_ITEM_PLAYER;
        for (xx=p_cmd->start_item; xx<BTAPP_AV_NUM_MPLAYERS && xx <=p_cmd->end_item ; xx++)
        {
            if (btapp_av_players[xx])
            {
                memcpy(&item.u.player, btapp_av_players[xx], sizeof(tAVRC_ITEM_PLAYER));
                res = AVRC_BldResponse (0, (tAVRC_RESPONSE *)p_rsp, &p_rsp_pkt);
            }
        }
        if (p_rsp_pkt == NULL)
        {
            p_rsp->item_count = 0;
            p_rsp->status = AVRC_STS_BAD_RANGE;
        }
        break;

    case AVRC_SCOPE_FILE_SYSTEM:
        /* put the current path in btapp_av_cb.path */
//        btapp_platform_rc_cur_path();
//        p_rsp_pkt = btapp_platform_rc_get_items( rc_handle, p_cmd, p_rsp );
        break;

    case AVRC_SCOPE_NOW_PLAYING:
        /* put the now playing path in btapp_av_cb.path */
        //sprintf(btapp_av_cb.path, "%s\\%s\\%s\\", btapp_cfg.root_path, BTAPP_AV_PATH, BTAPP_AV_PATH_NOW_PLAY);
//        p_rsp_pkt = btapp_platform_rc_get_items( rc_handle, p_cmd, p_rsp );
        break;

    default:
        /* TODO AVRC_SCOPE_SEARCH */
        *p_code = BTA_AV_RSP_NOT_IMPL;
    }

    return p_rsp_pkt;
}

/*******************************************************************************
**
** Function         btapp_rc_proc_chg_path
**
** Description      composes the AVRC_PDU_CHANGE_PATH response
**
** Returns          the GKI message that contains the response
*******************************************************************************/
static BT_HDR * btapp_rc_proc_chg_path(tAVRC_CHG_PATH_CMD *p_cmd, tAVRC_CHG_PATH_RSP *p_rsp,
                                       tBTA_AV_CODE *p_code)
{
    BT_HDR  *p_rsp_pkt = NULL;
    APPL_TRACE_DEBUG0("btapp_rc_proc_chg_path");
//    btapp_platform_rc_chg_path( p_cmd, p_rsp );
    *p_code = BTA_AV_RSP_ACCEPT;
    return p_rsp_pkt;
}

/*******************************************************************************
**
** Function         btapp_rc_proc_get_item_attr
**
** Description      composes the AVRC_PDU_GET_ITEM_ATTRIBUTES response
**
** Returns          the GKI message that contains the response
*******************************************************************************/
static BT_HDR * btapp_rc_proc_get_item_attr(UINT8 rc_handle, tAVRC_GET_ATTRS_CMD *p_cmd,
                                            tAVRC_GET_ATTRS_RSP *p_rsp, tBTA_AV_CODE *p_code)
{
    BT_HDR  *p_rsp_pkt = NULL;
    UINT8   found;
    tAVRC_ATTR_ENTRY attr_entries[BTAPP_AV_MAX_NUM_MEDIA_ATTR];
    UINT16          charset_id = btapp_av_cb.meta_info.charset_id[rc_handle];

    APPL_TRACE_DEBUG2("btapp_rc_proc_get_item_attr scope:%d uid[0]:0x%02x", p_cmd->scope, p_cmd->uid[0]);
    if (p_cmd->uid_counter != btapp_av_cb.meta_info.cur_uid_counter)
    {
        /* this error code does not sound right for database unaware player,
         * but is required by the conformace tests */
        p_rsp->status = AVRC_STS_UID_CHANGED;
    }
    else
    {
        switch (p_cmd->scope)
        {
        case AVRC_SCOPE_NOW_PLAYING:
            /* Position 0 is the changing part for FOLDER ID */
            if (p_cmd->uid[0] != AVRCP_NOW_PLAYING_FOLDER_ID_FIRST_BYTE)
            {
                p_rsp->status = AVRC_STS_NOT_EXIST;
                break;
            }
            /* else continue to process this case in the same manner as AVRC_SCOPE_FILE_SYSTEM */
        case AVRC_SCOPE_FILE_SYSTEM:
//            found = btapp_platform_rc_get_item_name_by_uid(p_cmd->uid, btapp_av_cb.temp_path);
            if (found == BTAPP_AV_FS_UID_FOUND)
            {
//                p_rsp->attr_count = btapp_platform_rc_get_attrs(charset_id, p_cmd->attr_count,
//                    p_cmd->p_attr_list, attr_entries, btapp_av_cb.temp_path);
                p_rsp->p_attr_list = attr_entries;
                AVRC_BldResponse (rc_handle, (tAVRC_RESPONSE *)p_rsp, &p_rsp_pkt);
            }
            else if (found == BTAPP_AV_FS_UID_IS_FOLDER)
                p_rsp->status = AVRC_STS_UID_IS_DIR;
            else
                p_rsp->status = AVRC_STS_NOT_EXIST;
            break;

        default:
            /* TODO AVRC_SCOPE_SEARCH */
            *p_code = BTA_AV_RSP_NOT_IMPL;
        }
    }
    return p_rsp_pkt;
}

/*******************************************************************************
**
** Function         btapp_rc_proc_get_num_of_items
**
** Description      composes the AVRC_PDU_GET_TOTAL_NUM_OF_ITEMS response
**
** Returns          the GKI message that contains the response
*******************************************************************************/
#if (AVRC_1_6_INCLUDED == TRUE)
static BT_HDR * btapp_rc_proc_get_num_of_items (tAVRC_GET_NUM_OF_ITEMS_CMD *p_cmd,
                                                tAVRC_GET_NUM_OF_ITEMS_RSP *p_rsp)
{
    BT_HDR  *p_rsp_pkt = NULL;

    APPL_TRACE_DEBUG1("btapp_rc_proc_get_num_of_items scope %d", p_cmd->scope);
    switch (p_cmd->scope)
    {
    case AVRC_SCOPE_PLAYER_LIST:
        /* There is only one medie player */
        p_rsp->uid_counter = 0;
        p_rsp->num_items = 1;
        break;

    case AVRC_SCOPE_FILE_SYSTEM:
        p_rsp->uid_counter = 0;
        sprintf(btapp_av_cb.path, "%s\\%s\\", btapp_cfg.root_path, BTAPP_AV_PATH);
        APPL_TRACE_DEBUG1("path: %s", btapp_av_cb.path);
//        p_rsp->num_items    = btapp_platform_rc_num_items_of_path(btapp_av_cb.path);
        break;

    case AVRC_SCOPE_SEARCH:
        p_rsp->uid_counter = 0;
        /* put the play list path in btapp_av_cb.path */
        sprintf(btapp_av_cb.path, "%s\\%s\\%s\\", btapp_cfg.root_path, BTAPP_AV_PATH, BTAPP_AV_PATH_SEARCH);
        APPL_TRACE_DEBUG1("path: %s", btapp_av_cb.path);
//        p_rsp->num_items    = btapp_platform_rc_num_items_of_path(btapp_av_cb.path);
        break;

    case AVRC_SCOPE_NOW_PLAYING:
        p_rsp->uid_counter = 0;
        /* put the play list path in btapp_av_cb.path */
        sprintf(btapp_av_cb.path, "%s\\%s\\%s\\", btapp_cfg.root_path, BTAPP_AV_PATH, BTAPP_AV_PATH_NOW_PLAY);
        APPL_TRACE_DEBUG1("path: %s", btapp_av_cb.path);
//        p_rsp->num_items    = btapp_platform_rc_num_items_of_path(btapp_av_cb.path);
        break;

    }

    return p_rsp_pkt;
}
#endif /* #if (AVRC_1_6_INCLUDED == TRUE) */

/*******************************************************************************
**
** Function         btapp_rc_proc_search
**
** Description      composes the AVRC_PDU_SEARCH response
**
** Returns          the GKI message that contains the response
*******************************************************************************/
static BT_HDR * btapp_rc_proc_search(tAVRC_SEARCH_RSP *p_rsp)
{
    BT_HDR  *p_rsp_pkt = NULL;
    p_rsp->status = AVRC_STS_SEARCH_NOT_SUP;
    AVRC_BldResponse (0, (tAVRC_RESPONSE *)p_rsp, &p_rsp_pkt);
    return p_rsp_pkt;
}
#endif /* AVCT_BROWSE_INCLUDED == TRUE */

/*******************************************************************************
**
** Function         btapp_rc_play_file
**
** Description      read the cfg from the selected file, check if reconfig is needed
**                  and play the current file
**
** Returns          none
*******************************************************************************/
void btapp_rc_play_file(UINT8 rc_handle)
{
    tBTAPP_AV_AUDIO_ENT *p_ent;
    BOOLEAN is_open;

    p_ent = btapp_av_get_audio_ent_by_rc_hndl(rc_handle);
    if (p_ent)
    {
        is_open = btapp_av_chk_a2dp_open (p_ent);
//        btapp_audio_play_item();
        if (is_open)
            bta_av_co_chk_state (p_ent->av_handle);
        if (bta_av_co_chk_reconfig_4file_chg())
        {
//            btapp_av_audio_set_prev_start();
            bta_av_co_reconfig_4file();
//            btapp_audio_reconfig_pend();
        }
        else
        {
//            btapp_audio_reconfig_done();
            btapp_av_rc_play(p_ent, FALSE);
        }
    }
}

/*******************************************************************************
**
** Function         btapp_rc_proc_play_item
**
** Description      composes and send the AVRC_PDU_PLAY_ITEM response
**
** Returns          none
*******************************************************************************/
static void btapp_rc_proc_play_item(tBTA_AV_META_MSG *p_msg, tAVRC_PLAY_ITEM_CMD *p_cmd,
                                    tAVRC_RSP *p_rsp, tBTA_AV_CODE rsp_code)
{
    BT_HDR  *p_rsp_pkt = NULL;
    char    media_name[BTAPP_DEV_NAME_LENGTH+1], md_name[BTAPP_DEV_NAME_LENGTH+1];
    UINT8   found;

    APPL_TRACE_DEBUG4("btapp_rc_proc_play_item scope:%d, uid[0]:0x%02d, uid[7]:0x%02d, play_count:%d",
        p_cmd->scope,p_cmd->uid[0], p_cmd->uid[7], btapp_av_cb.play_count);
    switch (p_cmd->scope)
    {
    case AVRC_SCOPE_NOW_PLAYING:
        /* Position 0 is the changing part for FOLDER ID */
        if (p_cmd->uid[0] != AVRCP_NOW_PLAYING_FOLDER_ID_FIRST_BYTE)
        {
            p_rsp->status = AVRC_STS_NOT_EXIST;
            break;
        }
        /* else continue to process this case in the same manner as AVRC_SCOPE_FILE_SYSTEM */
    case AVRC_SCOPE_FILE_SYSTEM:
//        found = btapp_platform_rc_get_item_name_by_uid(p_cmd->uid, btapp_av_cb.temp_path);
//        if ((found == BTAPP_AV_FS_UID_FOUND) &&
//            btapp_platform_rc_read_file_names_by_uidname(btapp_av_cb.temp_path, media_name, md_name))
//        {
//            rsp_code = BTA_AV_RSP_ACCEPT;
//        }
//        else if (found == BTAPP_AV_FS_UID_IS_FOLDER)
//            p_rsp->status = AVRC_STS_UID_IS_DIR;
//        else
            p_rsp->status = AVRC_STS_NOT_EXIST;
        break;

    default:
        /* TODO AVRC_SCOPE_SEARCH*/
        rsp_code = BTA_AV_RSP_NOT_IMPL;
    }

    AVRC_BldResponse (p_msg->rc_handle, (tAVRC_RESPONSE *)p_rsp, &p_rsp_pkt);
    if (p_rsp_pkt)
        BTA_AvMetaRsp(p_msg->rc_handle, p_msg->label, rsp_code, p_rsp_pkt);

    if (rsp_code == BTA_AV_RSP_ACCEPT)
    {
        btapp_av_stop_stream();
//        btapp_av_audio_stop();
        /* Position 0 is the changing part for FOLDER ID */
        if (p_cmd->uid[0] != AVRCP_NOW_PLAYING_FOLDER_ID_FIRST_BYTE)
        {
//            btapp_av_remove_now_playing_items();
//            btapp_av_add_now_play_item(BTAPP_AV_ITEM_UNKNOWN, media_name, md_name);
        }
        else
        {
            if (p_cmd->uid[AVRC_UID_SIZE - 1] - 1 < btapp_av_cb.play_count)
                btapp_av_cb.cur_play = p_cmd->uid[AVRC_UID_SIZE - 1] - 1;
        }
        btapp_rc_play_file(p_msg->rc_handle);
    }
}

/*******************************************************************************
**
** Function         btapp_rc_proc_add_to_now_play
**
** Description      composes and send the AVRC_PDU_ADD_TO_NOW_PLAYING response
**
** Returns          none
*******************************************************************************/
static void btapp_rc_proc_add_to_now_play(tBTA_AV_META_MSG *p_msg, tAVRC_ADD_TO_PLAY_CMD *p_cmd,
                                          tAVRC_RSP *p_rsp, tBTA_AV_CODE rsp_code)
{
    BT_HDR  *p_rsp_pkt = NULL;
    UINT8   found;
    char    media_name[BTAPP_DEV_NAME_LENGTH+1], md_name[BTAPP_DEV_NAME_LENGTH+1];

    APPL_TRACE_DEBUG1("btapp_rc_proc_add_to_now_play scope:%d", p_cmd->scope);
    switch (p_cmd->scope)
    {
    case AVRC_SCOPE_FILE_SYSTEM:
//        found = btapp_platform_rc_get_item_name_by_uid(p_cmd->uid, btapp_av_cb.temp_path);
//        if ( (found == BTAPP_AV_FS_UID_FOUND) &&
//            btapp_platform_rc_read_file_names_by_uidname(btapp_av_cb.temp_path, media_name, md_name))
//        {
//            rsp_code = BTA_AV_RSP_ACCEPT;
//       }
//        else if (found == BTAPP_AV_FS_UID_IS_FOLDER)
//            p_rsp->status = AVRC_STS_UID_IS_DIR;
//        else
            p_rsp->status = AVRC_STS_NOT_EXIST;
        break;

    case AVRC_SCOPE_SEARCH:
    case AVRC_SCOPE_NOW_PLAYING:
        /* TODO */
        rsp_code = BTA_AV_RSP_NOT_IMPL;
        p_rsp->status = AVRC_STS_BAD_SCOPE;
        break;

    default:
        /* AVRC_SCOPE_PLAYER_LIST */
        p_rsp->status = AVRC_STS_BAD_SCOPE;
        break;
    }

    if (rsp_code == BTA_AV_RSP_ACCEPT)
    {
//        p_rsp->status = btapp_av_add_now_play_item(BTAPP_AV_ITEM_UNKNOWN, media_name, md_name);
    }
    if (p_rsp->status != AVRC_STS_NO_ERROR && rsp_code == BTA_AV_RSP_ACCEPT)
        rsp_code = BTA_AV_RSP_REJ;
    APPL_TRACE_DEBUG2("status:%d, rsp_code:%d", p_rsp->status, rsp_code);

    AVRC_BldResponse (p_msg->rc_handle, (tAVRC_RESPONSE *)p_rsp, &p_rsp_pkt);
    if (p_rsp_pkt)
        BTA_AvMetaRsp(p_msg->rc_handle, p_msg->label, rsp_code, p_rsp_pkt);

    return;
}
#endif /* AVRC_ADV_CTRL_INCLUDED == TRUE */

/*******************************************************************************
**
** Function         btapp_rc_handler
**
** Description      Metadata message handler
**
**
** Returns          void
**
*******************************************************************************/
void btapp_rc_handler(tBTA_AV_EVT event, tBTA_AV *p_data)
{
    tAVRC_MSG       *p_msg = p_data->meta_msg.p_msg;
    UINT8           rc_handle = p_data->meta_msg.rc_handle;
    tAVRC_COMMAND   command;
    tAVRC_RESPONSE  response;
    tAVRC_STS       sts = AVRC_STS_BAD_CMD;
    BT_HDR          *p_rsp_pkt = NULL;
    tBTA_AV_CODE    rsp_code = BTA_AV_RSP_REJ;
    UINT16          charset_id = btapp_av_cb.meta_info.charset_id[rc_handle];
    UINT8           event_id = 0;
    UINT16          rej_evt_mask = 0;
    UINT16          xx, mask;
    tBTAPP_AV_AUDIO_ENT *p_ent;
    INT8            delta_volume;
#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
    UINT8           *p_mask;
#endif
    tBTA_AV_FEAT    av_features;

    if (event == BTA_AV_REMOTE_CMD_EVT)
    {
        /* must be the group navigation commands - the response is already sent by BTA-AV
         * there may be some platform dependent code to handle this command
         * If the platform code does not want to support group navigation/receive this event,
         * set p_bta_av_cfg->avrc_group to FALSE */
        return;
    }

    APPL_TRACE_DEBUG2("btapp_rc_handler ctype: %d, opcode: x%x", p_msg->hdr.ctype, p_msg->hdr.opcode);
    if (p_msg->hdr.ctype >= BTA_AV_RSP_NOT_IMPL)
    {
        /* handle response - probably for absolute volume */
        /* the UI may need to update the slide bar to refect the volume on peer device */
        p_ent = btapp_av_get_audio_ent_by_rc_hndl(rc_handle);
        if (p_ent == NULL)
            return;

        if (AVRC_ParsResponse (p_msg, &response, btapp_dm_avrc_buf, BTAPP_DM_AVRC_BUF_SIZE) == AVRC_STS_NO_ERROR)
        {
            /* this is a response - must be absolute volume related messages
             * update the current volume to reflect the setting at peer device */
            if (response.pdu == AVRC_PDU_REGISTER_NOTIFICATION)
            {
                if (p_msg->hdr.ctype == BTA_AV_RSP_CHANGED)
                    p_ent->cur_volume = response.reg_notif.param.volume;
            }
            else if (response.pdu == AVRC_PDU_SET_ABSOLUTE_VOLUME)
            {
                p_ent->cur_volume = response.volume.volume;
            }
        }

        /* got a response, so no command is pending any more */
        if ((response.pdu == AVRC_PDU_REGISTER_NOTIFICATION && p_msg->hdr.ctype == BTA_AV_RSP_INTERIM) ||
            (response.pdu == AVRC_PDU_SET_ABSOLUTE_VOLUME))
        {
            p_ent->pend_label = 0;
        }
        APPL_TRACE_DEBUG1("current volume: %d", p_ent->cur_volume);

        if (p_ent->pend_label == 0)
        {
            /* send one of the pending commands now */
            if (p_ent->queue_notif)
            {
                p_ent->queue_notif = 0;
                btapp_rc_reg_volume_notif(p_ent);
                return;
            }
            if (p_ent->queue_volume)
            {
                delta_volume = p_ent->queue_volume;
                p_ent->queue_volume = 0;
                btapp_rc_send_abs_volume_cmd (p_ent, delta_volume);
            }
        }
        return;
    }

    /* check if the currently addressed player support AdvControl or not */
    av_features = btapp_cfg.av_features & BTA_AV_FEAT_METADATA;
    APPL_TRACE_DEBUG1("av_features(1): 0x%04x", av_features);
#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
    p_mask = btapp_rc_get_player_features(btapp_av_cb.meta_info.addr_player_id);
    if (AVRC_PF_ADV_CTRL_SUPPORTED(p_mask))
    {
        av_features |= (btapp_cfg.av_features & BTA_AV_FEAT_ADV_CTRL);
    }
    APPL_TRACE_DEBUG1("av_features(2): 0x%04x", av_features);

    /* check if the currently browsed player support browsing or not */
    p_mask = btapp_rc_get_player_features(btapp_av_cb.meta_info.br_player_id);
    if ((btapp_cfg.av_features & BTA_AV_FEAT_ADV_CTRL) && AVRC_PF_BROWSE_SUPPORTED(p_mask))
        av_features |= (btapp_cfg.av_features & BTA_AV_FEAT_BROWSE);
    APPL_TRACE_DEBUG1("av_features(3): 0x%04x", av_features);
#endif

    sts = AVRC_ParsCommand (p_msg, &command, btapp_dm_avrc_buf, BTAPP_DM_AVRC_BUF_SIZE);
    response.pdu = command.pdu;
    if (sts == AVRC_STS_NO_ERROR)
        sts = btapp_rc_validate_pdu(av_features, response.pdu);

    if (sts == AVRC_STS_NO_ERROR)
    {
        response.rsp.status = AVRC_STS_NO_ERROR;
        switch (command.pdu)
        {
        /* case AVRC_PDU_GET_CAPABILITIES:              0x10 - handled internally by BTA-AV */
        case AVRC_PDU_LIST_PLAYER_APP_ATTR:          /* 0x11 */
            p_rsp_pkt = btapp_rc_proc_list_attrib(&response.list_app_attr, &rsp_code);
            break;

        case AVRC_PDU_LIST_PLAYER_APP_VALUES:        /* 0x12 */
            p_rsp_pkt = btapp_rc_proc_list_app_values(&command.list_app_values, &response.list_app_values, &rsp_code);
            break;

        case AVRC_PDU_GET_CUR_PLAYER_APP_VALUE:      /* 0x13 */
            p_rsp_pkt = btapp_rc_proc_get_cur_app_val(&command.get_cur_app_val, &response.get_cur_app_val, &rsp_code);
            break;

        case AVRC_PDU_SET_PLAYER_APP_VALUE:          /* 0x14 */
            p_rsp_pkt = btapp_rc_proc_set_app_val(&command.set_app_val, &response.set_app_val, &rsp_code, &event_id);
            break;

        case AVRC_PDU_GET_PLAYER_APP_ATTR_TEXT:      /* 0x15 */
            p_rsp_pkt = btapp_rc_proc_get_app_attr_txt(charset_id, &command.get_app_attr_txt,
                            &response.get_app_attr_txt, &rsp_code);
            break;
        case AVRC_PDU_GET_PLAYER_APP_VALUE_TEXT:     /* 0x16 */
            p_rsp_pkt = btapp_rc_proc_get_app_val_txt(charset_id, &command.get_app_val_txt,
                            &response.get_app_val_txt, &rsp_code);
            break;
        case AVRC_PDU_INFORM_DISPLAY_CHARSET:        /* 0x17 */
            response.rsp.status = btapp_rc_proc_inform_charset(rc_handle, &command.inform_charset, &rsp_code);
            break;
        case AVRC_PDU_INFORM_BATTERY_STAT_OF_CT:     /* 0x18 */
            p_rsp_pkt = btapp_rc_proc_inform_battery_status(&rsp_code);
            break;
        case AVRC_PDU_GET_ELEMENT_ATTR:              /* 0x20 */
            p_rsp_pkt = btapp_rc_proc_get_elem_attrs(charset_id, &command.get_elem_attrs,
                            &response.get_elem_attrs, &rsp_code);
            break;
        case AVRC_PDU_GET_PLAY_STATUS:               /* 0x30 */
            p_rsp_pkt = btapp_rc_proc_get_play_status(&response.get_play_status, &rsp_code);
            break;
        case AVRC_PDU_REGISTER_NOTIFICATION:         /* 0x31 */
            p_rsp_pkt = btapp_rc_proc_reg_notif(&p_data->meta_msg, &command.reg_notif, &response.reg_notif, &rsp_code);
            break;
        /* case AVRC_PDU_REQUEST_CONTINUATION_RSP:      0x40 - handled internally by avrc */
        /* case AVRC_PDU_ABORT_CONTINUATION_RSP:        0x41 - handled internally by avrc*/
#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
        case AVRC_PDU_SET_ABSOLUTE_VOLUME:           /* 0x50 */
            /* we only implement the CT role for this command. we should reject this command as a TG role */
            rsp_code = BTA_AV_RSP_NOT_IMPL;
            break;
        case AVRC_PDU_SET_ADDRESSED_PLAYER:          /* 0x60 */
            p_rsp_pkt = btapp_rc_proc_addr_player(&command.addr_player, &response.addr_player,
                            &rsp_code, &event_id, &rej_evt_mask);
            break;
#if (AVCT_BROWSE_INCLUDED == TRUE)
        case AVRC_PDU_SET_BROWSED_PLAYER:            /* 0x70 */
            p_rsp_pkt = btapp_rc_proc_br_player(charset_id, &command.br_player,
                            &response.br_player, &rsp_code, &rej_evt_mask);
            break;
        case AVRC_PDU_GET_FOLDER_ITEMS:              /* 0x71 */
            p_rsp_pkt = btapp_rc_proc_get_items(rc_handle, &command.get_items, &response.get_items, &rsp_code);
            break;
        case AVRC_PDU_CHANGE_PATH:                   /* 0x72 */
            p_rsp_pkt = btapp_rc_proc_chg_path(&command.chg_path, &response.chg_path, &rsp_code);
            break;
        case AVRC_PDU_GET_ITEM_ATTRIBUTES:           /* 0x73 */
            p_rsp_pkt = btapp_rc_proc_get_item_attr (rc_handle, &command.get_attrs, &response.get_attrs, &rsp_code);
            break;
#if (AVRC_1_6_INCLUDED == TRUE)
        case AVRC_PDU_GET_TOTAL_NUM_OF_ITEMS:        /* 0x75 */
            p_rsp_pkt = btapp_rc_proc_get_num_of_items(&command.get_num_of_items, &response.get_num_of_items);
            break;
#endif
        case AVRC_PDU_SEARCH:                        /* 0x80 */
            p_rsp_pkt = btapp_rc_proc_search (&response.search);
            break;
#endif /* AVCT_BROWSE_INCLUDED == TRUE */
        case AVRC_PDU_PLAY_ITEM:                     /* 0x74 */
            btapp_rc_proc_play_item (&p_data->meta_msg, &command.play_item, &response.play_item, rsp_code);
            return;
        case AVRC_PDU_ADD_TO_NOW_PLAYING:            /* 0x90 */
            btapp_rc_proc_add_to_now_play (&p_data->meta_msg, &command.add_to_play, &response.add_to_play, rsp_code);
            return;
#endif /* AVRC_ADV_CTRL_INCLUDED == TRUE */
        /* case AVRC_PDU_GENERAL_REJECT:                0xA0 - TG should not received this */
        default:
            APPL_TRACE_DEBUG1("unknown pdu: 0x%x", command.pdu);
            break;
        }
    }
    else
    {
        APPL_TRACE_DEBUG2("* * * PARSING ERROR : %d opcode:x%x * * *", sts, p_msg->hdr.opcode);
        /* compose reject message */
        response.rsp.status = sts;
        response.rsp.opcode = p_msg->hdr.opcode;
        if (response.rsp.opcode == AVRC_OP_BROWSE && sts == AVRC_STS_BAD_CMD)
            response.pdu = AVRC_PDU_GENERAL_REJECT;
        AVRC_BldResponse (0, &response, &p_rsp_pkt);
    }

    if (rsp_code == BTA_AV_RSP_NOT_IMPL)
    {
        if (p_rsp_pkt)
        {
            GKI_freebuf (p_rsp_pkt);
            p_rsp_pkt = NULL;
        }
#if (AVCT_BROWSE_INCLUDED == TRUE)
        if (p_msg->hdr.opcode == AVRC_OP_BROWSE)
        {
            /* use general reject */
            response.rsp.status = AVRC_STS_INTERNAL_ERR;
        }
        else
#endif
        {
            /* must be vendor command */
            BTA_AvVendorRsp(p_data->meta_msg.rc_handle, p_data->meta_msg.label, BTA_AV_RSP_NOT_IMPL,
                p_msg->vendor.p_vendor_data, p_msg->vendor.vendor_len, p_msg->vendor.company_id);
            return;
        }
    }

    if (p_rsp_pkt == NULL)
    {
        response.rsp.opcode = p_msg->hdr.opcode;
        AVRC_BldResponse (p_data->meta_msg.rc_handle, &response, &p_rsp_pkt);
    }

    if (p_rsp_pkt)
    {
        BTA_AvMetaRsp(p_data->meta_msg.rc_handle, p_data->meta_msg.label, rsp_code, p_rsp_pkt);
        if (event_id)
            btapp_rc_send_notification (event_id);

        if (rej_evt_mask)
        {
            for (xx=0; xx<AVRC_NUM_NOTIF_EVENTS && rej_evt_mask; xx++)
            {
                mask = (1 << xx);
                APPL_TRACE_DEBUG3("%d/x%04x rej_evt_mask:x%04x", (xx + 1), mask, rej_evt_mask);
                if (rej_evt_mask & mask)
                {
                    rej_evt_mask &= ~mask;
                    btapp_rc_reject_notification ((UINT8)(xx+1));
                }
            }
        }
    }
}

/*******************************************************************************
**
** Function         btapp_rc_conn
**
** Description      Send event notification that was registered before.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_rc_conn(tBTAPP_AV_AUDIO_ENT *p_ent)
{
    /* clean up the registration */
    if (!p_ent)
        return;
    btapp_av_cb.meta_info.registered_events[p_ent->rc_handle].evt_mask = 0;
    btapp_av_cb.meta_info.charset_id[p_ent->rc_handle] = BTAPP_META_DEFAULT_CHARACTER_SET;
    /* the default volume */
    p_ent->cur_volume   = 50;
    p_ent->queue_notif  = 0;
    p_ent->queue_volume = 0;
    p_ent->label = 1;
    p_ent->pend_label = 0;
    p_ent->label_volume = 0;
}

/*******************************************************************************
**
** Function         btapp_rc_next_label
**
** Description      set the label for next next time.
**
** Returns          void
**
*******************************************************************************/
void btapp_rc_next_label(tBTAPP_AV_AUDIO_ENT *p_ent)
{
    p_ent->label++;
    p_ent->label &= 0xF;
    if (p_ent->label == p_ent->label_volume)
        p_ent->label++;
}

/*******************************************************************************
**
** Function         btapp_rc_send_abs_volume_cmd
**
** Description      send abslute volume command
**
**
** Returns          void
**
*******************************************************************************/
void btapp_rc_send_abs_volume_cmd (tBTAPP_AV_AUDIO_ENT *p_ent, INT8 delta_volume)
{
    tAVRC_SET_VOLUME_CMD cmd;
    UINT8   old_volume;
    BT_HDR  *p_pkt = NULL;

    if (p_ent == NULL)
    {
        APPL_TRACE_DEBUG0("btapp_rc_send_abs_volume_cmd NULL ent");
        return;
    }

    if ((p_ent->rc_open) && (p_ent->peer_features & BTA_AV_FEAT_ADV_CTRL)
        && (p_ent->peer_tg.features & AVRC_SUPF_CT_CAT2))
    {
        if (p_ent->pend_label)
        {
            p_ent->queue_volume += delta_volume;
            APPL_TRACE_DEBUG2("another command is pending queue_volume:%d delta_volume:%d",
                p_ent->queue_volume, delta_volume);
        }
        else
        {
            cmd.pdu = AVRC_PDU_SET_ABSOLUTE_VOLUME;
            cmd.status = AVRC_STS_NO_ERROR;
            if (delta_volume > 0)
            {
                old_volume = delta_volume & AVRC_MAX_VOLUME;
                if ((AVRC_MAX_VOLUME - p_ent->cur_volume) < delta_volume)
                    p_ent->cur_volume = AVRC_MAX_VOLUME;
                else
                    p_ent->cur_volume += delta_volume;
            }
            else
            {
                old_volume = (0-delta_volume) & AVRC_MAX_VOLUME;
                if (p_ent->cur_volume < old_volume)
                    p_ent->cur_volume = 0;
                else
                    p_ent->cur_volume -= old_volume;
            }
            APPL_TRACE_DEBUG2("delta_volume:%d, old_volume: %d", delta_volume, old_volume);
            cmd.volume = p_ent->cur_volume;
            AVRC_BldCommand((tAVRC_COMMAND *)&cmd, &p_pkt);
            if (p_pkt)
            {
                p_ent->pend_label = p_ent->label;
                BTA_AvMetaCmd(p_ent->rc_handle, p_ent->label, BTA_AV_CMD_CTRL, p_pkt);
                btapp_rc_next_label(p_ent);
                APPL_TRACE_DEBUG1("Request Absolute Volume: %d", p_ent->cur_volume);
            }
            else
            {
                APPL_TRACE_DEBUG0("Can not build Absolute Volume command");
            }
        }
    }
    else
    {
        APPL_TRACE_DEBUG0("Peer does not support Advanced Control or TG role");
    }
}

/*******************************************************************************
**
** Function         btapp_rc_reg_volume_notif
**
** Description      send register notification command on volume change event
**
**
** Returns          void
**
*******************************************************************************/
void btapp_rc_reg_volume_notif (tBTAPP_AV_AUDIO_ENT *p_ent)
{
    tAVRC_REG_NOTIF_CMD notif_cmd;
    BT_HDR  *p_pkt = NULL;

    if (p_ent == NULL)
    {
        APPL_TRACE_DEBUG0("btapp_rc_reg_volume_notif NULL ent");
        return;
    }

    if ((p_ent->rc_open) && (p_ent->peer_features & BTA_AV_FEAT_ADV_CTRL)
         && (p_ent->peer_tg.features & AVRC_SUPF_CT_CAT2))
    {
        if (p_ent->pend_label)
        {
            APPL_TRACE_DEBUG0("another command is pending");
            p_ent->queue_notif = 1;
        }
        else
        {
            notif_cmd.pdu = AVRC_PDU_REGISTER_NOTIFICATION;
            notif_cmd.event_id = AVRC_EVT_VOLUME_CHANGE;
            notif_cmd.param = 0;
            AVRC_BldCommand((tAVRC_COMMAND *)&notif_cmd, &p_pkt);
            if (p_pkt)
            {
                p_ent->pend_label = p_ent->label;
                p_ent->label_volume = p_ent->label;
                BTA_AvMetaCmd(p_ent->rc_handle, p_ent->label, BTA_AV_CMD_NOTIF, p_pkt);
                btapp_rc_next_label(p_ent);
                APPL_TRACE_DEBUG0("Request EVENT_VOLUME_CHANGE (0x0d)");
            }
            else
            {
                APPL_TRACE_DEBUG0("Can not build EVENT_VOLUME_CHANGE (0x0d) command");
            }
        }
    }
    else
    {
        APPL_TRACE_DEBUG0("Peer does not support Advanced Control or TG role");
    }
}

/*******************************************************************************
**
** Function         btapp_rc_send_notification
**
** Description      Send event changed notification that was registered before.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_rc_send_notification(UINT8 event_id)
{
    APPL_TRACE_DEBUG1("btapp_rc_send_notification: %d", event_id);
    btapp_rc_complete_notification(event_id, BTA_AV_RSP_CHANGED);
}

/*******************************************************************************
**
** Function         btapp_rc_complete_notification
**
** Description      Send event notification that was registered before with the
**                  given response code
**
** Returns          void
**
*******************************************************************************/
static void btapp_rc_complete_notification(UINT8 event_id, tBTA_AV_CODE rsp_code)
{
    tAVRC_REG_NOTIF_RSP response;
    tBTAPP_AV_EVT_MASK evt_mask = 1;
    UINT8   i, count=0;
    UINT8 rc_handle, rc_label;
    BT_HDR  *p_rsp_pkt = NULL;
    UINT8   *p, xx;
    tBTAPP_AV_META_ATTRIB *p_pas_attrib;
    tAVRC_STS sts = AVRC_STS_NO_ERROR;
    tBTAPP_AV_AUDIO_ENT *p_ent;

    if (rsp_code == BTA_AV_RSP_REJ)
        sts = AVRC_STS_ADDR_PLAYER_CHG;

    /* Check if it is one of the supported events  */
    evt_mask <<= (event_id - 1);

    for (i=0; i < BTA_AV_NUM_STRS+1; i++)
    {
        if (btapp_av_cb.meta_info.registered_events[i].evt_mask & evt_mask)
        {
            APPL_TRACE_DEBUG4("Event(0x%x) was registered(0x%x) on device %d, evt_mask:0x%x",
                               event_id, btapp_av_cb.meta_info.registered_events, i,
                               btapp_av_cb.meta_info.registered_events[i].evt_mask );

            rc_handle = i;
            rc_label = btapp_av_cb.meta_info.registered_events[i].label[event_id-1];
            response.pdu = AVRC_PDU_REGISTER_NOTIFICATION;
            response.event_id = event_id;
            response.status = sts;

            switch(event_id)
            {
            case AVRC_EVT_PLAY_STATUS_CHANGE:      /* 0x01 */
                response.param.play_status = btapp_av_cb.meta_info.play_status.play_status;
                break;
            case AVRC_EVT_TRACK_CHANGE:            /* 0x02 */
                p = response.param.track;
                APPL_TRACE_DEBUG2("play_count %d cur_play %d",btapp_av_cb.play_count, btapp_av_cb.cur_play);
                /* Check whether folder and file is selected or not */
                if (btapp_av_cb.play_count == 0 )
                {
                    /* If not, add track ID to be 0xFFFFFFFF-FFFFFFFF */
                    UINT32_TO_BE_STREAM(p, AVRCP_NO_NOW_PLAYING_FOLDER_ID); /* the id from the no now playing folder */
                    UINT32_TO_BE_STREAM(p, AVRCP_NO_NOW_PLAYING_FILE_ID);   /* the id from the no now playing file */
                }
                else
                {
                    /* If selected, add track ID to be the current playing file ID  */
                    UINT32_TO_BE_STREAM(p, AVRCP_PLAY_LISTS_FOLDER_ID); /* the id from the now playing folder */
                    UINT32_TO_BE_STREAM(p, (UINT32)btapp_av_cb.cur_play);         /* the id from the current playing file */
                }
                break;
            case AVRC_EVT_TRACK_REACHED_END:       /* 0x03 */
            case AVRC_EVT_TRACK_REACHED_START:     /* 0x04 */
                /* not supported */
                break;

            case AVRC_EVT_PLAY_POS_CHANGED:        /* 0x05 */
                response.param.play_pos = btapp_av_cb.meta_info.play_status.song_pos;
                break;
            case AVRC_EVT_BATTERY_STATUS_CHANGE:   /* 0x06 */
                response.param.battery_status = btapp_av_cb.meta_info.notif_info.bat_stat;
                break;
            case AVRC_EVT_SYSTEM_STATUS_CHANGE:    /* 0x07 */
                response.param.system_status = btapp_av_cb.meta_info.notif_info.sys_stat;
                break;
            case AVRC_EVT_APP_SETTING_CHANGE:      /* 0x08 */
                response.param.player_setting.num_attr = AVRC_MAX_APP_SETTINGS;
                p_pas_attrib = &btapp_av_cb.meta_info.pas_info.equalizer;
                for (xx=0; xx<response.param.player_setting.num_attr; xx++)
                {
                    response.param.player_setting.attr_id[xx] = p_pas_attrib->attrib_id;
                    response.param.player_setting.attr_value[xx] = p_pas_attrib->curr_value;
                    p_pas_attrib++;
                }
                break;

            case AVRC_EVT_NOW_PLAYING_CHANGE:   /* 0x09 no param */
            case AVRC_EVT_AVAL_PLAYERS_CHANGE:  /* 0x0a no param */
                break;

            case AVRC_EVT_ADDR_PLAYER_CHANGE:   /* 0x0b */
                response.param.addr_player.player_id = btapp_av_cb.meta_info.addr_player_id;
                response.param.addr_player.uid_counter = btapp_av_cb.meta_info.cur_uid_counter;
                break;

            case AVRC_EVT_UIDS_CHANGE:          /* 0x0c */
                response.param.uid_counter = 0;
                break;

            case AVRC_EVT_VOLUME_CHANGE:        /* 0x0d */
                p_ent = btapp_av_get_audio_ent_by_rc_hndl(rc_handle);
                response.param.volume = 0;
                if (p_ent)
                {
                    response.param.volume = p_ent->cur_volume;
                }
                break;
            }

            /* De-register the event */
            btapp_av_cb.meta_info.registered_events[rc_handle].evt_mask &= ~evt_mask;
            btapp_av_cb.meta_info.registered_events[rc_handle].label[event_id-1] = 0xFF;
            APPL_TRACE_DEBUG4("EVENT NOTIF - Registered evt val after 0x%x clearing:0x%x on device %d w label %d",
                               btapp_av_cb.meta_info.registered_events[rc_handle].evt_mask,
                               evt_mask, rc_handle, rc_label );
            AVRC_BldResponse (0, (tAVRC_RESPONSE *)&response, &p_rsp_pkt);
            if (p_rsp_pkt)
                BTA_AvMetaRsp(rc_handle, rc_label, rsp_code, p_rsp_pkt);


        }
        else
        {
            APPL_TRACE_DEBUG3("Not registered event rcvd:0x%x (Registered evt val=0x%x) on device %d",
                               event_id, btapp_av_cb.meta_info.registered_events, i );
        }
    }
}

/*******************************************************************************
**
** Function         btapp_rc_reject_notification
**
** Description      Send event rejected notification that was registered before.
**
**
** Returns          void
**
*******************************************************************************/
void btapp_rc_reject_notification(UINT8 event_id)
{
    APPL_TRACE_DEBUG1("btapp_rc_reject_notification: %d", event_id);
    btapp_rc_complete_notification(event_id, BTA_AV_RSP_REJ);
}

/*******************************************************************************
**
** Function         btapp_rc_change_play_status
**
** Description      Change the play status. If the event is registered, send the
**                  changed notification now
**
** Returns          void
**
*******************************************************************************/
void btapp_rc_change_play_status(UINT8 new_play_status)
{
    UINT8   old_play_status = btapp_av_cb.meta_info.play_status.play_status;

    btapp_av_cb.meta_info.play_status.play_status = new_play_status;
    APPL_TRACE_DEBUG2("play_status = %d/%d", old_play_status, btapp_av_cb.meta_info.play_status.play_status );
    if (old_play_status != btapp_av_cb.meta_info.play_status.play_status)
        btapp_rc_send_notification(AVRC_EVT_PLAY_STATUS_CHANGE);
}

/*******************************************************************************
**
** Function         btapp_rc_is_valid_attrib_value
**
** Description      Check if the given attrib value is valid for its attribute
**
**
** Returns          BOOLEAN
**
*******************************************************************************/
static BOOLEAN btapp_rc_is_valid_attrib_value(UINT8 attrib, UINT8 value)
{
    BOOLEAN result=FALSE;

    switch(attrib)
    {
    case AVRC_PLAYER_SETTING_EQUALIZER:
         if ((value > 0)  &&
            (value <= AVRC_PLAYER_VAL_ON))
            result=TRUE;
         break;
    case AVRC_PLAYER_SETTING_REPEAT:
         if ((value > 0)  &&
            (value <= AVRC_PLAYER_VAL_GROUP_REPEAT))
            result=TRUE;
         break;
    case AVRC_PLAYER_SETTING_SHUFFLE:
    case AVRC_PLAYER_SETTING_SCAN:
         if ((value > 0)  &&
            (value <= AVRC_PLAYER_VAL_GROUP_SHUFFLE))
            result=TRUE;
         break;

         /* The legal ranges of TG driven static media player menu extension attribute IDs
          * are from 0x80 to 0xFF. However it is limited only between 0x80 and 0x83
          * in this implementation. Their value range is also restricted from 1 to 2.
          * See METADATA.DAT file for more details.
          *
          * These areas need to be reviewed carefully in the final product integration.
          */
    case BTAPP_META_PLAYER_SETTING_MENU_EXT_1:
    case BTAPP_META_PLAYER_SETTING_MENU_EXT_2:
    case BTAPP_META_PLAYER_SETTING_MENU_EXT_3:
    case BTAPP_META_PLAYER_SETTING_MENU_EXT_4:
         if ((value > 0) && (value <= 2))
            result=TRUE;
         break;
    }

    if (!result)
        APPL_TRACE_ERROR2("btapp_rc_is_valid_attrib_value() found not matching attrib(x%x)-value(x%x) pair!",
        attrib, value);

    return result;
}
#endif

