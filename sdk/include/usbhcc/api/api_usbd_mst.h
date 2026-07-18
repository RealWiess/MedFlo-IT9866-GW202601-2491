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
#ifndef _API_USBD_MST_H_
#define _API_USBD_MST_H_

#include "../version/ver_usbd_mst.h"
#if VER_USBD_MST_MAJOR != 6 || VER_USBD_MST_MINOR != 16
 #error "Incorrect USBD_MST version."
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MSTD_SUCCESS 0
#define MSTD_ERROR   1

/* type of callback to be called when a packet with unknown size arrives */
typedef void ( * t_vmst_com_unknown_rx_cb )( uint8_t * p_buf
                                            , uint32_t length );

int mstd_reg_unknown_rx_cb ( t_vmst_com_unknown_rx_cb vmst_com_unknown_rx_cb );

int mstd_init ( void );
int mstd_start ( void );
int mstd_stop ( void );
void mstd_delete ( void );
int mstd_is_idle ( uint8_t if_idx );

#ifdef __cplusplus
}
#endif

#endif /* ifndef _API_USBD_MST_H_ */
