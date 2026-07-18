/****************************************************************************
**
**  Name        gki_int.h
**
**  Function    This file contains GKI private definitions
**
**
**  Copyright (c) 1999-2004, WIDCOMM Inc., All Rights Reserved.
**  Proprietary and confidential.
**
*****************************************************************************/
#ifndef GKI_INT_H
#define GKI_INT_H

#include "gki_common.h"

#include "openrtos/FreeRTOS.h"
#include "openrtos/task.h"
#include "openrtos/semphr.h"
#include "openrtos/timers.h"
#include "openrtos/event_groups.h"

/**********************************************************************
** freeRTOS specific definitions
*/
typedef struct
{
    SemaphoreHandle_t    GKI_mutex;
    SemaphoreHandle_t    GKI_sched_mutex;
    TaskHandle_t         thread_id[GKI_MAX_TASKS];
    SemaphoreHandle_t    thread_evt_mutex[GKI_MAX_TASKS];
    QueueHandle_t        thread_evt_queue[GKI_MAX_TASKS];
    void*         queue_mem[GKI_MAX_TASKS];
}tGKI_OS, *pGKI_OS;

/* Contains common control block as well as OS specific variables */
typedef struct
{
    tGKI_OS     os;
    tGKI_COM_CB com;
} tGKI_CB;


/* Mutex */
UINT8 gki_openrtos_init_mutex( SemaphoreHandle_t* mutex );
UINT8 gki_openrtos_lock_mutex( SemaphoreHandle_t* mutex );
UINT8 gki_openrtos_unlock_mutex( SemaphoreHandle_t* mutex );
UINT8 gki_openrtos_deinit_mutex( SemaphoreHandle_t* mutex );

/* Semaphore */
UINT8 gki_openrtos_init_semaphore(SemaphoreHandle_t* semaphore);
UINT8 gki_openrtos_take_semaphore(SemaphoreHandle_t* semaphore , UINT32 timeout_ms);
UINT8 gki_openrtos_set_semaphore(SemaphoreHandle_t* semaphore);
UINT8 gki_openrtos_deinit_semaphore(SemaphoreHandle_t* semaphore);

#ifdef __cplusplus
extern "C" {
#endif

#if GKI_DYNAMIC_MEMORY == FALSE
GKI_API extern tGKI_CB  gki_cb;
#else
GKI_API extern tGKI_CB *gki_cb_ptr;
#define gki_cb (*gki_cb_ptr)
#endif

#ifdef __cplusplus
}
#endif

#endif
