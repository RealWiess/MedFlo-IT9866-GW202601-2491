/*****************************************************************************
 * Copyright (c) 2018-2019, Broadcom Inc.                                    *
 *                                                                           *
 * All Rights Reserved.                                                      *
 *                                                                           *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Inc.              *
 * the contents of this file may not be disclosed to third parties, copied   *
 * or duplicated in any form, in whole or in part, without the prior         *
 * written permission of Broadcom Inc.                                       *
 *****************************************************************************/
#include <string.h>
#include <stdlib.h>
#include "gki_int.h"
#include "bt_target.h"
#include "upio.h"

#define RTOS_WAIT_FOREVER                          (portMAX_DELAY)
#define GKI_ASSERT( error_string, assertion )   do { if ( !(assertion) ) printf( ( error_string ) ); } while(0)

/* arbitrary max number of 2-byte event messages to allow */
#define MAX_EVENT_MSGS 50
#define GKI_STATUS_UP   1
#define GKI_STATUS_DOWN 0

/* Define the structure that holds the GKI variables */
#if GKI_DYNAMIC_MEMORY == FALSE
tGKI_CB   gki_cb;
#endif

//const uint32_t  ms_to_tick_ratio = (uint32_t)( 1000 / (uint32_t)configTICK_RATE_HZ );

typedef void (*FR_TASK)(void*);

#define GKI_TIMER_THREAD_STACK_SIZE     2048
//uint8_t gki_timer_thread_stack[GKI_TIMER_THREAD_STACK_SIZE];
static  UINT8 gki_status = GKI_STATUS_DOWN;

extern void btapp_avk_ucodec_shutdown(void);
extern void btapp_avk_play_shutdown(void);
extern void bte_hcisu_shutdown();
extern void btu_task_shutdown(void);
extern void userial_read_thread_shutdown(void);
extern void btapp_task_shutdown(void);
extern void BTE_UnloadStack(void);
extern void BTM_Dev_Reset (void);

EventGroupHandle_t gki_tick_ev;



//uint8_t gki_openrtos_create_thread( TaskHandle_t* thread, uint8_t priority, const char* name, void* entry_function, void *parameter, uint32_t stack_size);
UINT8 gki_openrtos_create_thread( TaskHandle_t* thread, uint8_t priority, const char* name, TASKPTR entry_function, void *parameter, UINT32 stack_size)
{
	
	signed portBASE_TYPE result;	
	
	result = xTaskCreate( (TaskFunction_t)entry_function, name, (unsigned short)(stack_size / sizeof( portSTACK_TYPE )), (void*)NULL, (unsigned portBASE_TYPE) priority, thread );

    taskYIELD();
	
    return GKI_SUCCESS;
}


void gki_openrtos_suspend_thread(TaskHandle_t tsk)
{
	if(tsk == 0)
	{
		tsk = xTaskGetCurrentTaskHandle();
	}
	vTaskSuspend(tsk);
}

void gki_openrtos_resume_thread(TaskHandle_t tsk)
{
	vTaskResume(tsk);
}

UINT8 gki_openrtos_delete_thread(TaskHandle_t* thread)
{
    vTaskDelete( *thread );
    return GKI_SUCCESS;
}

UINT8 gki_openrtos_init_semaphore(SemaphoreHandle_t* semaphore)
{
    *semaphore = xSemaphoreCreateCounting( (unsigned portBASE_TYPE) 0x7fffffff, (unsigned portBASE_TYPE) 0 );
    return ( *semaphore!=NULL ) ? GKI_SUCCESS : GKI_FAILURE ;
}

UINT8 gki_openrtos_take_semaphore(SemaphoreHandle_t *semaphore , UINT32 timeout_ms)
{
    if ( pdTRUE == xSemaphoreTake( *semaphore, timeout_ms ) )
    {
        return GKI_SUCCESS;
    }
    else
    {
        return GKI_FAILURE;
    }
}

UINT8 gki_openrtos_set_semaphore(SemaphoreHandle_t *semaphore)
{
    return ( xSemaphoreGive( *semaphore ) == pdPASS )? GKI_SUCCESS : GKI_FAILURE;
}

UINT8 gki_openrtos_deinit_semaphore(SemaphoreHandle_t *semaphore)
{
    if (*semaphore != NULL)
    {
        vQueueDelete( *semaphore );
        *semaphore = NULL;
    }
    return GKI_SUCCESS;
}

UINT8 gki_openrtos_init_mutex( SemaphoreHandle_t* mutex )
{
    GKI_ASSERT( "gki_openrtos_init_mutex: null ptr",mutex != NULL);

    /* Mutex uses priority inheritance */
    *mutex = xSemaphoreCreateMutex( );
    GKI_ASSERT ( "gki_openrtos_init_mutex create error",*mutex != NULL );

    return GKI_SUCCESS;
}

UINT8 gki_openrtos_lock_mutex( SemaphoreHandle_t* mutex )
{
    GKI_ASSERT( "gki_openrtos_lock_mutex: null ptr",mutex != NULL);

    if ( xSemaphoreTake( *mutex, RTOS_WAIT_FOREVER ) != pdPASS )
    {
        return GKI_FAILURE;
    }
    
    return GKI_SUCCESS;
}


UINT8 gki_openrtos_unlock_mutex( SemaphoreHandle_t* mutex )
{
    GKI_ASSERT( "gki_openrtos_unlock_mutex: null ptr",mutex != NULL);

    if ( xSemaphoreGive( *mutex ) != pdPASS )
    {
        return GKI_FAILURE;
    }

    return GKI_SUCCESS;
}


UINT8 gki_openrtos_deinit_mutex( SemaphoreHandle_t* mutex )
{
    GKI_ASSERT( "gki_openrtos_deinit_mutex: null ptr" , (mutex != NULL) && (*mutex != NULL));

	vSemaphoreDelete( *mutex );
    *mutex = NULL;
	
    return GKI_SUCCESS;
}


UINT8 gki_openrtos_init_queue( QueueHandle_t* queue, const char* name, UINT32 message_size, UINT32 number_of_messages )
{
    *queue = xQueueCreate( (unsigned portBASE_TYPE) ( message_size * number_of_messages ), (unsigned portBASE_TYPE) message_size );
    GKI_ASSERT("gki_openrtos_init_queue fail",*queue != NULL);
    return GKI_SUCCESS;
}

UINT8 gki_openrtos_deinit_queue( QueueHandle_t* queue )
{
    vQueueDelete( *queue );
    *queue = NULL;
    return GKI_SUCCESS;
}

#if 0
UINT8 gki_openrtos_is_queue_empty( QueueHandle_t* queue )
{
    signed portBASE_TYPE result;

    taskENTER_CRITICAL();
    result = xQueueIsQueueEmptyFromISR( *queue );
    taskEXIT_CRITICAL();

    return (result == pdTRUE ) ? GKI_SUCCESS : GKI_FAILURE;
}

UINT8 gki_openrtos_is_queue_full( QueueHandle_t* queue )
{
    signed portBASE_TYPE result;

    taskENTER_CRITICAL();
    result = xQueueIsQueueFullFromISR( *queue );
    taskEXIT_CRITICAL();

    return (result == pdTRUE ) ? GKI_SUCCESS : GKI_FAILURE;
}
#endif

UINT8 gki_openrtos_push_to_queue(QueueHandle_t* queue, void* message)
{
    signed portBASE_TYPE retval = xQueueSendToBack( *queue, message, (TickType_t) ( RTOS_WAIT_FOREVER ) );
    if ( retval != pdPASS )
    {
        return GKI_FAILURE;
    }

    return GKI_SUCCESS;
}

UINT8 gki_openrtos_pop_from_queue( QueueHandle_t* queue, void* message, UINT32 timeout_ms )
{
    signed portBASE_TYPE retval = xQueueReceive( *queue, message, (TickType_t) ( timeout_ms )  );
    //GKI_ASSERT("gki_openrtos_pop_from_queue fail",retval == pdPASS);
    return (retval == pdPASS)? GKI_SUCCESS : GKI_FAILURE;
}

#if 0
UINT8 gki_openrtos_init_event_flags(rt_event_t* event_flags )
{    
    *event_flags = rt_event_create("", RT_IPC_FLAG_FIFO);
    RT_ASSERT(*event_flags != RT_NULL); 
    return GKI_SUCCESS;
}

UINT8 gki_rtt_wait_for_event_flags( rt_event_t* event_flags, UINT32 flags_to_wait_for, UINT32* flags_set, UINT32 timeout_ms )
{
    rt_err_t err;
    rt_int32_t tick;

    tick = (timeout_ms == 0xFFFFFFFF) ? RT_WAITING_FOREVER : rt_tick_from_millisecond(timeout_ms);

    err = rt_event_recv(*event_flags,
                flags_to_wait_for,
                RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                tick,
                (rt_uint32_t *)flags_set);
    RT_ASSERT((err == RT_EOK) || (err == -RT_ETIMEOUT));

    return GKI_SUCCESS;
}


UINT8 gki_rtt_set_event_flags( rt_event_t* event_flags, UINT32 flags_to_set )
{
    RT_ASSERT (*event_flags != NULL);
    RT_ASSERT(RT_EOK == rt_event_send( *event_flags, flags_to_set ));

    return GKI_SUCCESS;
}

UINT8 gki_rtt_deinit_event_flags( rt_event_t* event_flags )
{
    ASSERT (*event_flags != NULL);

    ASSERT(RT_EOK == rt_event_delete( *event_flags ));  /* No return value */

    *event_flags = NULL;

    return GKI_SUCCESS;
}
#endif

#if 0
/*
 * semphore operations
*/
UINT8 gki_openrtos_init_semaphore( rt_sem_t* semaphore )
{
    *semaphore = rt_sem_create("", 1, RT_IPC_FLAG_FIFO);
    RT_ASSERT(RT_NULL != *semaphore);
    return GKI_SUCCESS;
}

UINT8 gki_openrtos_set_semaphore( rt_sem_t* semaphore )
{
    RT_ASSERT(RT_EOK == rt_sem_release(*semaphore));
    return GKI_SUCCESS;
}

UINT8 gki_openrtos_get_semaphore( rt_sem_t* semaphore, UINT32 timeout_ms )
{
    rt_int32_t tick;

    tick = (timeout_ms == 0xFFFFFFFF) ? RT_WAITING_FOREVER : rt_tick_from_millisecond(timeout_ms);

    ASSERT(RT_EOK == rt_sem_take (*semaphore, tick));
    return GKI_SUCCESS;
}

UINT8 gki_openrtos_deinit_semaphore( rt_sem_t* semaphore )
{
    ASSERT(RT_EOK == rt_sem_delete(*semaphore));
    return GKI_SUCCESS;
}
#endif

/*******************************************************************************
**
** Function         GKI_init
**
** Description      This function is called once at startup to initialize
**                  all the timer structures.
**
** Returns          void
**
*******************************************************************************/
void GKI_init(void)
{
    memset (&gki_cb, 0, sizeof (tGKI_CB));

#if 0
    gki_cb.os.GKI_mutex = xSemaphoreCreateMutex();
    gki_cb.com.OSDisableNesting = 0;

    gki_cb.os.GKI_sched_mutex = xSemaphoreCreateMutex();
#endif

    gki_buffer_init();
    gki_timers_init();

    gki_cb.com.OSTicks = 0;

}

/*******************************************************************************
**
** Function         GKI_get_os_tick_count
**
** Description      This function is called to retrieve the native OS system tick.
**
** Returns          Tick count of native OS.
**
*******************************************************************************/
UINT32 GKI_get_os_tick_count(void)
{
    return xTaskGetTickCount(); //gki_cb.com.OSTicks;
}

UINT8 get_priority_from_taskid(UINT8 task_id)
{
  switch(task_id)
  {
  case UCODEC_TASK :
    return 7;
  case BTU_TASK :
    return 6;
  case HCISU_TASK :
    return 5;
  case USERIAL_TASK:
    return 5;
  case BTAPPL_TASK:
    return 2;
  case TICKS_TASK:
    return 3;
  default:
    return 0;
  }
}

/*******************************************************************************
**
** Function         GKI_create_task
**
** Description      This function is called to create a new OSS task.
**
** Parameters:      task_entry  - (input) pointer to the entry function of the task
**                  task_id     - (input) Task id is mapped to priority
**                  taskname    - (input) name given to the task
**                  stack       - (input) pointer to the start of stack
**                  stacksize   - (input) size of the stack allocated for the task
**
** Returns          GKI_SUCCESS if all OK, GKI_FAILURE if any problem
*******************************************************************************/
UINT8 GKI_create_task (TASKPTR task_entry, UINT8 task_id, INT8 *taskname, UINT16 *stack, UINT16 stacksize)
{
    UINT8 priority = GKI_BASE_PRIORITY;
   
    if (task_id >= GKI_MAX_TASKS)
    {
        GKI_TRACE_1("ERROR: GKI task_id must be less than %d",GKI_MAX_TASKS);
        return (GKI_FAILURE);
    }
	
    stacksize+=4096;
	
    GKI_TRACE_5("GKI_create_task func=0x%x  id=%d  name=%s  stack=0x%x  stackSize=%d", task_entry, task_id, taskname, stack, stacksize);

    priority = GKI_BASE_PRIORITY + task_id;
    //priority = configMAX_PRIORITIES - task_id;
    
    gki_cb.os.thread_evt_mutex[task_id] = xSemaphoreCreateMutex();  //call freeRTOS api

    if(gki_cb.os.thread_evt_mutex[task_id] == NULL)
    {
        GKI_TRACE_1("GKI_create_task create thread_evt_mutex failed %s!", taskname);
        return GKI_FAILURE;
    }

    gki_cb.os.thread_evt_queue[task_id] = xQueueCreate((unsigned portBASE_TYPE)THREAD_EVT_QUEUE_NUM_MSG, (unsigned portBASE_TYPE)THREAD_EVT_QUEUE_MSG_SIZE);

    if(gki_cb.os.thread_evt_queue[task_id] == NULL)
    {
        GKI_TRACE_1("GKI_create_task create thread_evt_queue failed %s!", taskname);
        return GKI_FAILURE;
    }

    GKI_sched_lock();

    BaseType_t status = xTaskCreate((FR_TASK)task_entry
                                    , (char const *)taskname
                                    , (stacksize / sizeof(portSTACK_TYPE))
                                    , 0
                                    , priority
                                    ,&gki_cb.os.thread_id[task_id]
                                   );
    //taskYIELD();
    
    gki_cb.com.OSRdyTbl[task_id]    = TASK_READY;
    gki_cb.com.OSTName[task_id]     = taskname;
    gki_cb.com.OSWaitTmr[task_id]   = 0;
    gki_cb.com.OSWaitEvt[task_id]   = 0;
    gki_cb.com.OSTPri[task_id]      = priority;

    GKI_sched_unlock();
	
	GKI_TRACE_1("GKI_create_task create thread id:%x",gki_cb.os.thread_id[task_id]);

    return (GKI_SUCCESS);

}

/*******************************************************************************
**
** Function         GKI_suspend_task()
**
** Description      This function suspends the task specified in the argument.
**
** Returns          GKI_SUCCESS if all OK, else GKI_FAILURE
**                  It also tries to do context switch.
**                  If task tries to suspend itself, then some other task must
**                  resume it, because it is no longer running.
**
*******************************************************************************/
UINT8 GKI_suspend_task(UINT8 task_id)
{
    vTaskSuspend(gki_cb.os.thread_id[task_id]);  /*  freeRTOS call */
    return GKI_SUCCESS;
}

/*******************************************************************************
**
** Function         GKI_resume_task()
**
** Description      This function resumes the task specified in the argument.
**
** Returns          GKI_SUCCESS if all OK, else GKI_FAILURE
**                  It also tries to do context switch.
**                  If task tries to suspend itself, then some other task must
**                  resume it, because it is no longer running.
**
*******************************************************************************/
UINT8 GKI_resume_task(UINT8 task_id)
{
    vTaskResume(gki_cb.os.thread_id[task_id]);  /*  freeRTOS call */
    return GKI_SUCCESS;
}

/*******************************************************************************
**
** Function         GKI_exit_task
**
** Description      This function is called to stop a GKI task.
**
** Returns          void
**
*******************************************************************************/
void GKI_exit_task (UINT8 task_id)
{
    gki_openrtos_deinit_mutex(&gki_cb.os.thread_evt_mutex[task_id]);
    gki_openrtos_deinit_queue(&gki_cb.os.thread_evt_queue[task_id]);
    gki_openrtos_delete_thread(&gki_cb.os.thread_id[task_id]);
    gki_cb.com.OSRdyTbl[task_id] = TASK_DEAD;
}

/*******************************************************************************
 **
 ** Function         GKI_run_task
 **
 ** Description      This function running task
 **
 ** Parameters:
 **
 ** Returns          void
 **
 ** NOTE
 *********************************************************************************/
void GKI_run_task(UINT32 useless)
{
    UINT32 ev;
    
    gki_tick_ev = xEventGroupCreate();
    if (NULL == gki_tick_ev)
    {
        //fatal error
        GKI_TRACE_0("GKI_run_task up failed!");
        return;
    }
    GKI_TRACE_0("GKI_run_task up");

    gki_status = GKI_STATUS_UP;

    while (1)
    {
        EventBits_t ev = xEventGroupWaitBits(gki_tick_ev, 0x00000001, pdTRUE, pdFALSE, pdMS_TO_TICKS(GKI_TIMER_INTERVAL));
        if((ev & 0x00000001))
        {//wait shutdown event
            break;
        }
        GKI_timer_update(100);
    }

    GKI_TRACE_0("GKI_run_task up shutdown");
    vEventGroupDelete(gki_tick_ev);
    GKI_exit_task(TICKS_TASK);
}

/*******************************************************************************
 **
 ** Function         GKI_ticks_task_shutdown
 **
 ** Description      shutdown GKI ticks task
 **
 ** Parameters:
 **
 ** Returns          void
 **
 ** NOTE
 *********************************************************************************/
void GKI_ticks_task_shutdown(void)
{
    if(gki_tick_ev != NULL)
    {
        xEventGroupSetBits(gki_tick_ev, 0x00000001);
    }
    else
    {
        GKI_TRACE_0("GKI_ticks_task_shutdown error");
    }
}

/*******************************************************************************
 **
 ** Function         GKI_run
 **
 ** Description      This function runs a task
 **
 ** Parameters:      p_task_id  - (input) pointer to task id
 **
 ** Returns          void
 **
 ** NOTE             This function is only needed for operating systems where
 **                  starting a task is a 2-step process. Most OS's do it in
 **                  one step, If your OS does it in one step, this function
 **                  should be empty.
 *********************************************************************************/
void GKI_run( void *p_task_id )
{
    GKI_create_task(GKI_run_task, TICKS_TASK, (INT8 *)"gki_timer_task", NULL, GKI_TIMER_THREAD_STACK_SIZE);
}

/*******************************************************************************
**
** Function         GKI_sched_lock
**
** Description      This function is called by tasks to disable scheduler switching.
**
** Returns          void
**
*******************************************************************************/
void GKI_sched_lock(void)
{
    taskENTER_CRITICAL();
}

/*******************************************************************************
**
** Function         GKI_sched_unlock
**
** Description      This function is called by tasks to enable scheduler switching.
**
** Returns          void
**
*******************************************************************************/
void GKI_sched_unlock(void)
{
    taskEXIT_CRITICAL();
}

/*******************************************************************************
**
** Function         GKI_wait
**
** Description      This function is called by tasks to wait for a specific
**                  event or set of events. The task may specify the duration
**                  that it wants to wait for, or 0 if infinite.
**
** Parameters:      flag -    (input) the event or set of events to wait for
**                  timeout - (input) the duration that the task wants to wait
**                                    for the specific events (in system ticks)
**
**
** Returns          the event mask of received events or zero if timeout
**                  Calling routine must check the returned event bits versus
**                  what it was waiting for, and re-call this routine if
**                  desired event bit(s) not returned.
**
*******************************************************************************/
UINT16 GKI_wait (UINT16 flag, UINT32 timeout)
{
    UINT16  evt = 0;
    UINT8   rtask;
    UINT8   msg_buf[THREAD_EVT_QUEUE_MSG_SIZE];
    BaseType_t  rtn = 5;

    rtask = GKI_get_taskid();

    //It should not over-protect, if check the NULL queue, post one logs then return.
    if(gki_cb.os.thread_evt_queue[rtask] == NULL)
    {
        GKI_TRACE_1("GKI_wait wait in task:%d, target queue is empty", rtask);
        return (GKI_INVALID_TASK);
    }

    //Because different task only wait their own events, so the gki_cb.com.OSWaitForEvt should not be protect by the critical region.
    gki_cb.com.OSWaitTmr[rtask] = (UINT16) timeout;
    gki_cb.com.OSWaitForEvt[rtask] = flag;
    evt = gki_cb.com.OSWaitEvt[rtask]; /*  start by picking up any events sent from interrupts or leftover */

    /* Check if anything in any of the mailboxes. Possible race condition. */
    if (gki_cb.com.OSTaskQFirst[rtask][0])
        gki_cb.com.OSWaitEvt[rtask] |= TASK_MBOX_0_EVT_MASK;
    if (gki_cb.com.OSTaskQFirst[rtask][1])
        gki_cb.com.OSWaitEvt[rtask] |= TASK_MBOX_1_EVT_MASK;
    if (gki_cb.com.OSTaskQFirst[rtask][2])
        gki_cb.com.OSWaitEvt[rtask] |= TASK_MBOX_2_EVT_MASK;
    if (gki_cb.com.OSTaskQFirst[rtask][3])
        gki_cb.com.OSWaitEvt[rtask] |= TASK_MBOX_3_EVT_MASK;

    //GKI_TRACE_3("- %x  %x %x +", rtask, evt, flag);
    if ((evt & flag) != flag) /* If requested events not already received then check for new ones */
    {
        if (timeout)
        {
            if ((rtn = xQueueReceive(gki_cb.os.thread_evt_queue[rtask], msg_buf, timeout)) > 0)
                evt |= (msg_buf[0]<<8 | msg_buf[1]);  /*  gather all the events that have been sent so far */
        }
        else
        {
            xQueueReceive(gki_cb.os.thread_evt_queue[rtask], msg_buf, 0xFFFFFFFF);  /* gki 0 means forever, freeRTOS means immediate */
            evt |= (msg_buf[0]<<8 | msg_buf[1]);  /*  gather all the events that have been sent so far */
        }
    }

    /* Clear only those bits which user wants...save the rest as leftovers */
    gki_cb.com.OSWaitEvt[rtask] = ~flag & evt;

    /* Return only those bits which user wants... */
    evt &= flag;

    //GKI_TRACE_3("-- %x  %x %x ++ ", evt, rtn, rtask);

    return evt;
}

/*******************************************************************************
**
** Function         GKI_delay
**
** Description      This function is called by tasks to sleep unconditionally
**                  for a specified amount of time.
**
** Returns          void
**
*******************************************************************************/
void GKI_delay (UINT32 timeout)
{
    timeout = GKI_MS_TO_TICKS(timeout);     /* convert from milliseconds to ticks */

    if(timeout == 0)
    {
        timeout = 1;
    }
    vTaskDelay(timeout);
}

/*******************************************************************************
**
** Function         GKI_send_event
**
** Description      This function is called by tasks to send events to other
**                  tasks. Tasks can also send events to themselves.
**
** Returns          GKI_SUCCESS if all OK, else GKI_FAILURE
**
*******************************************************************************/
UINT8 GKI_send_event (UINT8 task_id, UINT16 event)
{
    UINT8 msg_buf[THREAD_EVT_QUEUE_MSG_SIZE];

    if (task_id >= GKI_MAX_TASKS)
    {
        GKI_TRACE_1("GKI_send_event task out of range, task_id:%d", task_id);
        return (GKI_FAILURE);
    }

    //GKI_TRACE_2("+ %d %d -", task_id, event);

    /*This region should be the critical region, since different task will use this same API to send different events,
      if the higher priority task preempt the lower task at this place, will cause race condition.*/
    GKI_disable();
    msg_buf[0] = (UINT8) (event >> 8); /* MSB */
    msg_buf[1] = (UINT8) (event & 0xFF); /* LSB */
    GKI_enable();

    //It should not over-protect, if check the NULL queue, post one logs then return.
    if(gki_cb.os.thread_evt_queue[task_id] == NULL)
    {
        GKI_TRACE_2("GKI_send_event send to task:%d from task:%d, target queue is empty", task_id, GKI_get_taskid());
        return (GKI_INVALID_TASK);
    }
    
    if (xQueueSend(gki_cb.os.thread_evt_queue[task_id], msg_buf, 0x00) == 0)  /* freeRTOS call */
    {
        GKI_TRACE_1("GKI_send_event send event fail, task_id:%d", task_id);
        return (GKI_FAILURE);
    }

    return (GKI_SUCCESS);
}

/*******************************************************************************
**
** Function         GKI_get_taskid
**
** Description      This function gets the currently running GKI task ID.
**
**
** Returns          GKI task ID, or 0 if current task is not a GKI task
**
*******************************************************************************/
UINT8 GKI_get_taskid(void)
{
     TaskHandle_t taskID;
     UINT8 task_id = 0;
     UINT8 i;

    taskID = xTaskGetCurrentTaskHandle( );

    for (i=0; i<GKI_MAX_TASKS; i++)
    {
        if (gki_cb.os.thread_id[i] == taskID)
        {
            task_id = i;
            break;
        }
    }

    return task_id;
}

/*******************************************************************************
**
** Function         GKI_map_taskname
**
** Description      This function gets the task name of the taskid passed as arg.
**                  If GKI_MAX_TASKS is passed as arg the currently running task
**                  name is returned
**
** Returns          pointer to task name
**
*******************************************************************************/
INT8 *GKI_map_taskname(UINT8 task_id)
{
    if (task_id < GKI_MAX_TASKS)
        return (gki_cb.com.OSTName[task_id]);

    else if (task_id == GKI_MAX_TASKS )
        return (gki_cb.com.OSTName[GKI_get_taskid()]);

    else
        return (INT8*)"BAD";
}

/*******************************************************************************
**
** Function         GKI_enable
**
** Description      This function enables interrupts.
**
** Returns          void
**
*******************************************************************************/
void GKI_enable(void)
{
#if 0
    if (gki_cb.com.OSIntNesting)
        return;

    if (gki_cb.com.OSDisableNesting > 0)
    {
        gki_cb.com.OSDisableNesting--;
    }
    if (gki_cb.com.OSDisableNesting == 0)
    {
        //xSemaphoreGive(gki_cb.os.GKI_mutex);     /* freeRTOS call */
    }
#else
    taskENTER_CRITICAL();
#endif
}

/*******************************************************************************
**
** Function         GKI_disable
**
** Description      This function disables interrupts.
**
** Returns          void
**
*******************************************************************************/
void GKI_disable (void)
{
#if 0
    if (gki_cb.com.OSIntNesting)
        return;

    if (!gki_cb.com.OSDisableNesting)
    {
        xSemaphoreTake(gki_cb.os.GKI_mutex, 0xFFFFFFFF);  /* freeRTOS call */
    }
    gki_cb.com.OSDisableNesting++;
#else
    taskEXIT_CRITICAL();
#endif
}

/*******************************************************************************
**
** Function         GKI_exception
**
** Description      This function throws an exception
**
** Returns          void
**
*******************************************************************************/

void GKI_exception (UINT16 code, char *msg)
{
    GKI_TRACE_ERROR_0( "********************************************************************");
    GKI_TRACE_ERROR_2( "* GKI_exception(): 0x%02x %s", code, msg);
    GKI_TRACE_ERROR_0( "********************************************************************");

#if (GKI_DEBUG == TRUE)
    GKI_disable();

    if (gki_cb.com.ExceptionCnt < GKI_MAX_EXCEPTION)
    {
        EXCEPTION_T *pExp;

        pExp =  &gki_cb.com.Exception[gki_cb.com.ExceptionCnt++];
        pExp->type = code;
        pExp->taskid = GKI_get_taskid();
        strncpy((char *)pExp->msg, msg, GKI_MAX_EXCEPTION_MSGLEN - 1);
    }

    GKI_enable();
#endif

    /* TODO:  Put the specific handler or logger here */


    return;
}

/*******************************************************************************
**
** Function         GKI_os_malloc
**
** Description      This function allocates memory
**
** Returns          the address of the memory allocated, or NULL if failed
**
*******************************************************************************/
void *GKI_os_malloc (UINT32 size)
{
    return ((void*)pvPortMalloc(size));
}

/*******************************************************************************
**
** Function         GKI_os_free
**
** Description      This function frees memory
**
** Returns          void
**
*******************************************************************************/
void GKI_os_free (void *p_mem)
{
    vPortFree(p_mem);
}

/*******************************************************************************
**
** Function         GKI_dump_status
**
** Description      This function to dump GKI task status
**
** Returns          void
**
*******************************************************************************/
void GKI_dump_status(void)
{
    UINT8 i = 0;

    for(i = 0; i < GKI_MAX_TASKS; i ++)
    {
        if(gki_cb.com.OSRdyTbl[i] != TASK_DEAD)
        {
           GKI_TRACE_5("GKI task %d, th_id:0x%8x, name:%s, pri:%d, state:%d", i, gki_cb.os.thread_id[i], gki_cb.com.OSTName[i], gki_cb.com.OSTPri[i], gki_cb.com.OSRdyTbl[i]);
        }
    }

    GKI_TRACE_5("%d %d %d %d %d ", GKI_BUF0_MAX, GKI_BUF1_MAX, GKI_BUF2_MAX, GKI_BUF3_MAX, GKI_TIMER_INTERVAL);
    GKI_TRACE_5("GKI releated task_id %d %d %d %d %d", USERIAL_TASK, HCISU_TASK, BTU_TASK, BTAPPL_TASK, TICKS_TASK);
    GKI_TRACE_1("HCILP_HC_IDLE_THRESHOLD:%d", HCILP_HC_IDLE_THRESHOLD);
    GKI_TRACE_1("gki_cb size:%d ", sizeof(gki_cb));
}

/*******************************************************************************
**
** Function         GKI_shut_down
**
** Description      Shut down the whole BT stack
**
** Returns          void
**
*******************************************************************************/
void GKI_shut_down(void)
{
    if(gki_status == GKI_STATUS_UP)
    {
        /*Need close all upper layer opened services*/
#if UCODEC_INCLUDED
        /* Close ucodec_task */
        btapp_avk_ucodec_shutdown();
#endif

#if AUDIO_PLAY_INCLUDED
        /* Close audio_play_task */
        btapp_avk_play_shutdown();
#endif

#if BTAPPL_INCLUDED
        /* Close btapp_task */
        btapp_task_shutdown();
#endif
        /* Close HCISU task */
        bte_hcisu_shutdown();

        /* Close btu_task */
        btu_task_shutdown();

        /* Close gki ticks task*/
        GKI_ticks_task_shutdown();

        /* Close userial task */
        userial_read_thread_shutdown();

        /* Given the opportunity let the related task exit */
        GKI_delay(1000);

        /* gki buffers need deinit*/
        gki_buffer_deinit();

        BTE_UnloadStack();

        BTM_Dev_Reset();

        /* Shutdown BT controller*/
        UPIO_Set(UPIO_GENERAL, BT_REG_ON_GPIO, UPIO_OFF);
        UPIO_Set(UPIO_GENERAL, HCILP_BT_WAKE_GPIO, UPIO_OFF);
        UPIO_DeInit();

        //BTE_DeleteLogModule();
#if 0
        vSemaphoreDelete(gki_cb.os.GKI_mutex);
        vSemaphoreDelete(gki_cb.os.GKI_sched_mutex);
#endif

#ifdef HCI_LOG_FILE
        extern void btsnoop_close(void);
        btsnoop_close();
#endif
        gki_status = GKI_STATUS_DOWN;
    }
    else
    {
        GKI_TRACE_0("GKI has down!");
    }
}

