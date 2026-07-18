/**
 *******************************************************************************
 * Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
 *******************************************************************************
 * @file  osif_tp.c
 * @brief hci stack osif
 * @details
 * @author Thomas_li
 * @version v1.0
 * @date 2019-08-23
 */

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "hci_tp_dbg.h"
#include "osif_tp.h"
#include "osif.h"


void *tp_osif_malloc(uint32_t len)
{
    void *p = NULL;
    p = osif_mem_alloc(RAM_TYPE_DATA_ON, len);
    //fix the bug
    memset(p, 0, len);
    //hci_tp_dbg("\r\n===malloc===p = %p, len:%x\r\n", p, len);
    return p;
}

bool tp_osif_free(void *p)
{
    ///hci_tp_dbg("\r\n===free====p = %p\r\n", p);
    osif_mem_free(p);
    return true;
}
#if 0
void *tp_osif_h5_malloc(uint32_t len)
{
    void *p = NULL;
    p = osif_mem_alloc(RAM_TYPE_DATA_ON, len);
    //fix the bug
    memset(p, 0, len);
    hci_tp_dbg("\r\n===malloc===p = %p, len:%x\r\n", p, len);
    return p;
}

bool tp_osif_h5_free(void *p)
{
    osif_mem_free(p);
    return true;
}
#endif

void tp_osif_delay(uint32_t ms)
{
    osif_delay(ms);
}

bool tp_osif_timer_create(void **pp_handle, const char *p_timer_name, uint32_t timer_id,
                          uint32_t interval_ms, bool reload, void (*p_timer_callback)(void *))
{
    return osif_timer_create(pp_handle, p_timer_name, timer_id, interval_ms, reload, p_timer_callback);
}

bool tp_osif_timer_start(void **pp_handle)
{
    return osif_timer_start(pp_handle);
}

bool tp_osif_timer_restart(void **pp_handle, uint32_t interval_ms)
{
    return osif_timer_restart(pp_handle, interval_ms);
}
bool tp_osif_timer_stop(void **pp_handle)
{
    return osif_timer_stop(pp_handle);
}

bool tp_osif_timer_delete(void **pp_handle)
{
    return osif_timer_delete(pp_handle);
}

/****************************************************************************/
/* Lock critical section                                                    */
/****************************************************************************/
uint32_t tp_osif_lock(void)
{
    osif_sched_suspend();
    return 0;
}

/****************************************************************************/
/* Unlock critical section                                                  */
/****************************************************************************/
void tp_osif_unlock(uint32_t flags)
{
    osif_sched_resume();
}

/****************************************************************************/
/* Lock critical section                                                    */
/****************************************************************************/
void tp_osif_suspend(void)
{
    osif_sched_suspend();
}

/****************************************************************************/
/* Unlock critical section                                                  */
/****************************************************************************/
void tp_osif_resume(void)
{
    osif_sched_resume();
    //osif_unlock(flags);
}

bool tp_osif_msg_queue_create(void **pp_handle, uint32_t msg_num, uint32_t msg_size)
{
    return osif_msg_queue_create(pp_handle, msg_num, msg_size);
}
bool tp_osif_msg_queue_delete(void *p_handle)
{
    return osif_msg_queue_delete(p_handle);
}
bool tp_osif_msg_send(void *p_handle, void *p_msg, uint32_t wait_ms)
{
    return osif_msg_send(p_handle, p_msg, wait_ms);
}

bool tp_osif_msg_recv(void *p_handle, void *p_msg, uint32_t wait_ms)
{
    return osif_msg_recv(p_handle, p_msg, wait_ms);
}

bool tp_osif_task_create(void **pp_handle, const char *p_name, void (*p_routine)(void *),
                         void *p_param, uint16_t stack_size, uint16_t priority)
{
    return osif_task_create(pp_handle, p_name, p_routine, p_param, stack_size, priority);
}
bool tp_osif_task_delete(void *p_handle)
{
    return osif_task_delete(p_handle);
}

