#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "hci_tp_dbg.h"
#include "osif.h"

void *tp_osif_malloc(uint32_t len);
bool tp_osif_free(void *p);
void tp_osif_delay(uint32_t ms);

bool tp_osif_timer_create(void **pp_handle, const char *p_timer_name, uint32_t timer_id,
                          uint32_t interval_ms, bool reload, void (*p_timer_callback)(void *));

bool tp_osif_timer_start(void **pp_handle);

bool tp_osif_timer_restart(void **pp_handle, uint32_t interval_ms);

bool tp_osif_timer_stop(void **pp_handle);

bool tp_osif_timer_delete(void **pp_handle);

/****************************************************************************/
/* Lock critical section                                                    */
/****************************************************************************/
uint32_t tp_osif_lock(void);

/****************************************************************************/
/* Unlock critical section                                                  */
/****************************************************************************/
void tp_osif_unlock(uint32_t flags);

/****************************************************************************/
/* Lock critical section                                                    */
/****************************************************************************/
void tp_osif_suspend(void);

/****************************************************************************/
/* Unlock critical section                                                  */
/****************************************************************************/
void tp_osif_resume(void);

bool tp_osif_msg_queue_create(void **pp_handle, uint32_t msg_num, uint32_t msg_size);

bool tp_osif_msg_queue_delete(void *p_handle);

bool tp_osif_msg_send(void *p_handle, void *p_msg, uint32_t wait_ms);

bool tp_osif_msg_recv(void *p_handle, void *p_msg, uint32_t wait_ms);

bool tp_osif_task_create(void **pp_handle, const char *p_name, void (*p_routine)(void *),
                         void *p_param, uint16_t stack_size, uint16_t priority);

bool tp_osif_task_delete(void *p_handle);


