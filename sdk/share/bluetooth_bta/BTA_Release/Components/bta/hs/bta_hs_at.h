/*****************************************************************************
**
**  Name:           bta_hs_at.h
**
**  Description:    Interface file for BTA HS AT command interpreter.
**
**  Copyright (c) 2004, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_HS_AT_H
#define BTA_HS_AT_H

/*****************************************************************************
**  Constants
*****************************************************************************/

/* AT command argument capabilities */
#define BTA_HS_AT_NONE          0x01        /* no argument */
#define BTA_HS_AT_SET           0x02        /* set value */
#define BTA_HS_AT_READ          0x04        /* read value */
#define BTA_HS_AT_TEST          0x08        /* test value range */
#define BTA_HS_AT_FREE          0x10        /* freeform argument */

/* AT argument format */
#define BTA_HS_AT_FMT_NONE          0           /* no arguments */
#define BTA_HS_AT_FMT_STR           1           /* string */
#define BTA_HS_AT_FMT_INT           2           /* integer */


/* no event to application */
#define BTA_HS_NO_EVT 0





/*****************************************************************************
**  Data types
*****************************************************************************/

typedef struct
{
    const char  *p_res;         /* AT command string */
    UINT8       arg_type;
    tBTA_HS_EVT app_evt;

} tBTA_HS_AT_RES;

/* callback function executed when command is parsed */
typedef void (tBTA_HS_AT_RES_CBACK)(void *p_user, UINT16 res, char * p_arg);

/* callback function executed to send "ERROR" result code */
typedef void (tBTA_HS_AT_ERR_CBACK)(void *p_user, BOOLEAN unknown, char *p_arg);

/* AT command parsing control block */
typedef struct
{
    tBTA_HS_AT_RES          *p_at_res_tbl;      /* AT command table */
    tBTA_HS_AT_RES_CBACK    *p_res_cback;   /* command callback */
    tBTA_HS_AT_ERR_CBACK    *p_err_cback;   /* error callback */
    void                    *p_user;        /* user-defined data */
    char                    *p_res_buf;     /* temp parsing buffer */
    UINT16                  res_pos;        /* position in temp buffer */
    UINT16                  res_max_len;    /* length of temp buffer to allocate */
    UINT8                   state;          /* parsing state */
} tBTA_HS_AT_CB;


/*****************************************************************************
**  Function prototypes
*****************************************************************************/

/*****************************************************************************
**
** Function         bta_hs_at_init
**
** Description      Initialize the AT command parser control block.
**
**
** Returns          void
**
*****************************************************************************/
extern void bta_hs_at_init(tBTA_HS_AT_CB *p_cb);

/*****************************************************************************
**
** Function         bta_hs_at_reinit
**
** Description      Re-initialize the AT command parser control block.  This
**                  function resets the AT command parser state and frees
**                  any GKI buffer.
**
**
** Returns          void
**
*****************************************************************************/
extern void bta_hs_at_reinit(tBTA_HS_AT_CB *p_cb);

/*****************************************************************************
**
** Function         bta_hs_at_parse
**
** Description      Parse AT commands.  This function will take the input
**                  character string and parse it for AT commands according to
**                  the AT command table passed in the control block.
**
**
** Returns          BOOLEAN TRUE if done with string. Flase if waiting for new data
**
*****************************************************************************/
extern BOOLEAN bta_hs_at_parse(tBTA_HS_AT_CB *p_cb, char *p_buf, UINT16 len);

#endif /* BTA_HS_AT_H */



