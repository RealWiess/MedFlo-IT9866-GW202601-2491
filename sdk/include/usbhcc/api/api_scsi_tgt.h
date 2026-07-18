/***************************************************************************
 *
 *            Copyright (c) 2006-2019 by HCC Embedded
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
#ifndef _API_SCSI_TGT_H_
#define _API_SCSI_TGT_H_

#include <stdint.h>

#include "api_mdriver.h"
#include "../config/config_usbd_mst.h"

#include "../version/ver_usbd_mst.h"
#if VER_USBD_MST_MAJOR != 6 || VER_USBD_MST_MINOR != 16
 #error "Incorrect USBD_MST version."
#endif

#include "../version/ver_mdriver.h"
#if VER_MDRIVER_MAJOR != 1
 #error Incompatible MDRIVER version number!
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SCSI_TGT_SUCCESS 0
#define SCSI_TGT_ERROR   1

#if ( ( MST_USE_MST_IF != 0u ) && ( MST_USE_VMST_IF != 0u ) )
 #define SCSI_MST_IDX    0u
 #define SCSI_VMST_IDX   1u
#else
 #if ( MST_USE_MST_IF != 0u )
  #define SCSI_MST_IDX   0u
 #endif

 #if ( MST_USE_VMST_IF != 0u )
  #define SCSI_VMST_IDX  0u
 #endif
#endif

/* this structure contains the media driver init function and parameter */
/* It is used to build the LUN list in config/config_scsi_tgt.c */
typedef struct
{
  F_DRIVERINIT  driver_init;
  uint32_t      param;
  uint8_t       iso;
} scsim_table_entry_t;

int scsis_disable_lun ( uint8_t if_idx, uint8_t lun );
int scsis_enable_lun ( uint8_t if_idx, uint8_t lun, uint8_t readonly );
int scsis_lun_disabled ( uint8_t if_idx, uint8_t lun );
int scsis_media_remove_allowed ( uint8_t if_idx, uint8_t lun, uint8_t * allowed );

#ifdef __cplusplus
}
#endif

#endif /* ifndef _API_SCSI_TGT_H_ */

