/***************************************************************************
 *
 *            Copyright (c) 2010-2021 by HCC Embedded
 *
 * This software is copyrighted by and is the sole property of
 * HCC.  All rights, title, ownership, or other interests
 * in the software remain the property of HCC.  This
 * software may only be used in accordance with the corresponding
 * license agreement.  Any unauthorized use, duplication, transmission,
 * distribution, or disclosure of this software is expressly forbidden.
 *
 * This Copyright notice may not be removed or modified without prior
 * written consent of HCC.
 *
 * HCC reserves the right to modify this software without notice.
 *
 * HCC Embedded
 * Budapest 1133
 * Vaci ut 76
 * Hungary
 *
 * Tel:  +36 (1) 450 1302
 * Fax:  +36 (1) 450 1303
 * http: www.hcc-embedded.com
 * email: info@hcc-embedded.com
 *
 ***************************************************************************/
#ifndef _API_SCSI_H
#define _API_SCSI_H

#include "../psp/include/psp_types.h"
#include "../config/config_scsi.h"

#include "../version/ver_usbh_mst.h"
#if VER_USBH_MST_MAJOR != 3 || VER_USBH_MST_MINOR != 17
 #error Incompatible USBH_MST version number!
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* SCSI return codes */
#define SCSI_SUCCESS           0
#define SCSI_ERR_UNIT          1
#define SCSI_ERR_TRANSFER      2
#define SCSI_ERR_NOT_SUPPORTED 3
#define SCSI_ERR_RESOURCE      4


/* SCSI drive states */
#define SCSI_ST_DISCONNECTED   0
#define SCSI_ST_CONNECTED      1
#define SCSI_ST_CHANGED        2
#define SCSI_ST_WRPROTECT      4

/* type definition of state change callback function */
typedef void  t_state_change_fn ( uint8_t unit, uint8_t state );

/* SCSI communication layer unit ID (API->SCSI->COM) */
typedef uint8_t  t_scsi_com_unit_id;

/* logical unit information. */
typedef struct
{
  char      vendor[8];
  char      prod_id[16];
  char      rev[4];

  uint8_t   b_removable;
  uint32_t  no_of_sectors;
  uint32_t  sector_size;
  uint32_t  last_wr_lba;    /* Address of last written block */
  uint32_t  last_wr_cnt;    /* Number of last written blocks */
#if SCSI_SUPPORT_REMOVABLE_DEVICE
  uint32_t  last_poll_tick; /* Timestamp in OS ticks */
#endif
} t_lun_info;


int scsi_init ( void );
int scsi_start ( void );
int scsi_stop ( void );
int scsi_delete ( void );

int scsi_get_com_info ( uint8_t unit, char * * name, t_scsi_com_unit_id * com_uid );
int scsi_get_lun_info ( uint8_t unit, t_lun_info * lun_info );
void scsi_register_cb ( t_state_change_fn * sc_cb );

int scsi_raw_read ( uint8_t unit, uint8_t * p_cmd, uint8_t cmd_len, uint8_t * p_buf, uint32_t buf_len );
int scsi_raw_write ( uint8_t unit, uint8_t * p_cmd, uint8_t cmd_len, uint8_t * p_buf, uint32_t buf_len );


/* Please note that the functions below can only be called if non HCC
   file system is used */
int scsi_read ( uint8_t unit, uint32_t lba, uint32_t cnt, void * buffer );
int scsi_write ( uint8_t unit, uint32_t lba, uint32_t cnt, void * buffer );
int scsi_get_unit_state ( uint8_t unit );
int scsi_release ( uint8_t unit );
int scsi_flush ( uint8_t unit );

#ifdef __cplusplus
}
#endif

#endif /* ifndef _API_SCSI_H */


