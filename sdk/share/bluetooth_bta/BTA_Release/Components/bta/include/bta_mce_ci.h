/*****************************************************************************
**
**  Name:           bta_mce_ci.h
**
**  Description:    This is the interface file for the Message Client Equipment
**                  (MCE) subsystem call-in functions.
**
**  Copyright (c) 2009 - 2013, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTA_MCE_CI_H
#define BTA_MCE_CI_H

#include "bta_api.h"
#include "bta_ma_def.h"
#include "bta_mse_api.h"
#include "bta_mce_api.h"
#include "bta_fs_ci.h"


/*****************************************************************************
**  Constants and Data Types
*****************************************************************************/
/**************************
**  Common Definitions
***************************/
/* Read Ready Event */
typedef struct
{
    BT_HDR              hdr;
    UINT16              num_read;
    tBTA_MA_STATUS      status;
} tBTA_MCE_CI_READ_EVT;

/* Open Complete Event */
typedef struct
{
    BT_HDR              hdr;
    tBTA_FS_CO_STATUS   status;
    UINT32              file_size;
    int                 fd;
    const char          *p_file;
    tBTA_MA_INST_ID     inst_id;

} tBTA_MCE_CI_OPEN_EVT;

/* Write Ready Event */
typedef struct
{
    BT_HDR              hdr;
    tBTA_MA_MSG_HANDLE  msg_handle;
    tBTA_MA_STATUS      status;
} tBTA_MCE_CI_WRITE_EVT;

/*****************************************************************************
**  Function Declarations
*****************************************************************************/
/**************************
**  Common Functions
***************************/

/*******************************************************************************
**
** Function         bta_mce_ci_write_msg
**
** Description      This function is a response for the bta_mce_co_write_msg
**
**
** Parameters       STATUS - indicate the write operation is successful or not
**                  evt    - event that waspassed into the call-out function.
**                  session_id - Session ID of the current OBEX connection
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_mce_ci_write_msg( tBTA_MA_STATUS status,
                                          UINT16 evt, tBTA_MA_SESS_HANDLE session_id);

/*******************************************************************************
**
** Function         bta_mce_ci_open
**
** Description      This function sends an event to BTA indicating the phone has
**                  finished opening a file for reading or writing.
**
** Parameters       fd - file descriptor passed to the stack in the
**                       bta_fs_ci_open call-in function.
**                  status - BTA_FS_CO_OK if file was opened in mode specified
**                                          in the call-out function.
**                           BTA_FS_CO_EACCES if the file exists, but contains
**                                          the wrong access permissions.
**                           BTA_FS_CO_FAIL if any other error has occurred.
**                  file_size - The total size of the file
**                  evt - Used Internally by BTA -> MUST be same value passed
**                       in call-out function.
**                  inst_id - Instance ID
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_mce_ci_open(int fd, UINT16 status, UINT32 file_size, UINT16 evt,
                     tBTA_MA_INST_ID inst_id);

/*******************************************************************************
**
** Function         bta_mce_ci_read_upload_msg
**
** Description      This function is a response for the bta_mce_co_read_upload_msg
**
**
** Parameters       STATUS - indicate the read operation is successful or not
**                  filled_msg_size - size of the filled message
**                  is_last_packet - FASLE - need to do more read TRUE- done
**                  evt    - event that waspassed into the call-out function.
**                  session_id - Session ID of the OBEX connection
**
**
** Returns          void
**
*******************************************************************************/
BTA_API extern void bta_mce_ci_read_upload_msg( tBTA_MA_STATUS status,
                                                UINT32 num_bytes_read,
                                                UINT16 evt, tBTA_MA_SESS_HANDLE session_id );


#endif /* BTA_MCE_CI_H */




