/***************************************************************************
 *
 *            Copyright (c) 2010-2023 by Tuxera
 *
 * This software is copyrighted by and is the sole property of
 * Tuxera.  All rights, title, ownership, or other interests
 * in the software remain the property of Tuxera.  This
 * software may only be used in accordance with the corresponding
 * license agreement.  Any unauthorized use, duplication, transmission,
 * distribution, or disclosure of this software is expressly forbidden.
 *
 * This Copyright notice may not be removed or modified without prior
 * written consent of Tuxera.
 *
 * Tuxera reserves the right to modify this software without notice.
 *
 * Tuxera Inc
 * Westendintie 1
 * 02160 Espoo
 * Finland
 *
 * Tel:  +358 20 764 1720
 * http: www.tuxera.com
 * email: info@tuxera.com
 *
 ***************************************************************************/
#ifndef OAL_MUTEX_H
#define OAL_MUTEX_H

#include "oal_common.h"
#include "os/oalp_defs.h"
#include "os/oalp_mutex.h"

#include "../version/ver_oal.h"
#if VER_OAL_MAJOR != 2 || VER_OAL_MINOR != 8
 #error Incompatible OAL version number!
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if 1 // Irene Lin
#define oal_mutex_create	uh_oal_mutex_create
#define oal_mutex_delete	uh_oal_mutex_delete
#define oal_mutex_get		uh_oal_mutex_get
#define oal_mutex_put		uh_oal_mutex_put
#endif

oal_ret_t oal_mutex_create ( oal_mutex_t * p_mutex );
oal_ret_t oal_mutex_delete ( oal_mutex_t * p_mutex );
oal_ret_t oal_mutex_get ( oal_mutex_t * p_mutex );
oal_ret_t oal_mutex_put ( oal_mutex_t * p_mutex );

#ifdef __cplusplus
}
#endif

#endif /* ifndef OAL_MUTEX_H */

