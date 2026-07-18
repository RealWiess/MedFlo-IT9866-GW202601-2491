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
#ifndef OAL_EVENT_H
#define OAL_EVENT_H

#include "oal_common.h"
#include "os/oalp_defs.h"
#include "os/oalp_event.h"
#include "os/oalp_task.h"

#include "../version/ver_oal.h"
#if VER_OAL_MAJOR != 2 || VER_OAL_MINOR != 8
 #error Incompatible OAL version number!
#endif

#ifdef __cplusplus
extern "C" {
#endif

oal_ret_t oal_event_create ( oal_event_t * p_event );
oal_ret_t oal_event_delete ( oal_event_t * p_event );
oal_ret_t oal_event_clear ( oal_event_t * p_event, oal_event_flags_t cflags, oal_task_id_t task_id );
oal_ret_t oal_event_get ( oal_event_t * p_event, oal_event_flags_t wflags, oal_event_flags_t * sflags, oal_event_timeout_t timeout );
oal_ret_t oal_event_set ( oal_event_t * p_event, oal_event_flags_t flags, oal_task_id_t task_id );
oal_ret_t oal_event_set_int ( oal_event_t * p_event, oal_event_flags_t flags,  oal_task_id_t task_id );

#ifdef __cplusplus
}
#endif

#endif /* ifndef OAL_EVENT_H */

