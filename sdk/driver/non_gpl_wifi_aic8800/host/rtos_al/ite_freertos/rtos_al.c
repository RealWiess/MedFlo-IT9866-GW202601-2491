/*
 * Copyright (C) 2018-2020 AICSemi Ltd.
 *
 * All Rights Reserved
 */

#ifndef _KERNEL
#define _KERNEL
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
//#include "rtos_def.h"
#include "rtos_al.h"
//#include "aic_common.h"
//#include "plf.h"
#include <string.h>
#include "porting.h"
#include "log.h"
#include "co_list.h"
#include "co_math.h"
#include "dbg_assert.h"
#include "openrtos/task.h"
#include "openrtos/semphr.h"
#include "openrtos/FreeRTOS.h"

#define RTOS_AL_INFO_DUMP 0//1

#define AIC_TASK_NAME_MAX_LEN 64

#define QUEUE_COUNT_USE_SEMAPHORE   0
#define QUEUE_EXTRA_ELEMENT_COUNT   4
#define QUEUE_TRACE                 0

/*
 * FUNCTIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Convert ms to ticks
 *
 * @param[in] timeout_ms Timeout value in ms (use -1 for no timeout).
 * @return number of ticks for the specified timeout value.
 *
 ****************************************************************************************
 */
#if 1
__INLINE rtos_tick_type rtos_timeout_2_tickcount(int timeout_ms)
{
    if (timeout_ms < 0)
    {
        return portMAX_DELAY;
    }
    else
    {
        return pdMS_TO_TICKS(timeout_ms);
    }
}

__INLINE uint32_t rtos_ms_to_tick(uint32_t ms)
{
	return (uint32_t)pdMS_TO_TICKS(ms);
}

__INLINE uint32_t rtos_tick_to_ms(uint32_t tick)
{
	return (uint32_t)tick * portTICK_PERIOD_MS;
}

uint32_t rtos_now(bool isr)
{
	if (isr)
		return xTaskGetTickCountFromISR();
	else
		return xTaskGetTickCount();
}

void rtos_msleep(uint32 time_in_ms)
{
    uint32_t tick = rtos_ms_to_tick(time_in_ms);

    vTaskDelay(tick);
#endif
}

void *rtos_malloc(uint32_t size)
{
    void * ptr = malloc(size);
    
    return ptr;
}

void *rtos_calloc(uint32_t nb_elt, uint32_t size)
{
    void * ptr = NULL;

    ptr = calloc(nb_elt, size);

    return ptr;
}

void rtos_free(void *ptr)
{
    if (ptr) {
        free(ptr);
    }
}

void rtos_memcpy(void *pdest, const void *psrc, uint32_t size)
{
    memcpy(pdest, psrc, size);
}

void rtos_memset(void *pdest, uint8_t byte, uint32_t size)
{
    memset(pdest, byte, size);
}
/*
void rtos_heap_info(int *total_size, int *free_size, int *min_free_size)
{
	*total_size = configTOTAL_HEAP_SIZE;
#if (PLF_STDLIB)
    struct mallinfo info = mallinfo();
    *free_size = info.fordblks + (configTOTAL_HEAP_SIZE - info.arena);
    *min_free_size = configTOTAL_HEAP_SIZE - info.arena;
#else
    *free_size = xPortGetFreeHeapSize();
    *min_free_size = xPortGetMinimumEverFreeHeapSize();
#endif
}
*/
uint32_t rtos_entercritical()
{
    portDISABLE_INTERRUPTS();

    return 0;
}

void rtos_exitcritical()
{
    portENABLE_INTERRUPTS();
}

rtos_task_handle rtos_get_current_task(void)
{
    return (rtos_task_handle)xTaskGetCurrentTaskHandle();
}

int rtos_task_create(rtos_task_fct func,
                     const char * const name,
                     int task_id,
                     const uint16_t stack_depth,
                     void * const params,
                     rtos_prio prio,
                     rtos_task_handle * task_handle)
{
    BaseType_t res;
    rtos_task_handle handle;

    #if OSAL_TRACE
    creating_task_id = task_id;
    #endif
    res = xTaskCreate(func, name, stack_depth, params, prio, &handle);

    if (res != pdPASS)
        return 1;

    #if ( configUSE_TRACE_FACILITY == 1 ) || ( configUSE_RW_TASK_ID == 1 )
    vTaskSetTaskNumber(handle, task_id);
    #endif

    if (task_handle)
        *task_handle = handle;
	//AIC_LOG_PRINTF("rtos_task_create %s %p\n", name, (void*)task_handle);
    return 0;
}

void rtos_task_delete(rtos_task_handle task_handle)
{
    if (!task_handle)
        task_handle = xTaskGetCurrentTaskHandle();
    if (eTaskGetState(task_handle) != eDeleted)
        vTaskDelete(task_handle);
}

void rtos_task_suspend(int duration)
{
    if (duration <= 0)
        return;
    vTaskDelay(pdMS_TO_TICKS(duration));
}
/*
int rtos_task_list(int buffer_size)
{
    int total_size, free_size, min_free_size;
    rtos_heap_info(&total_size, &free_size, &min_free_size);

    if(free_size < buffer_size) {
        AIC_LOG_PRINTF("Heap size is not enough!\n");
        return -1;
    }

    #if (configUSE_STATS_FORMATTING_FUNCTIONS > 0)
    char *pbuf= (char *)rtos_calloc(1, buffer_size);
    vTaskList(pbuf);
    AIC_LOG_PRINTF("TaskList:\n");
    AIC_LOG_PRINTF("%s\n", pbuf);
    rtos_free(pbuf);
    #endif
    return 0;
} */

#if (configUSE_TRACE_FACILITY == 1)
void rtos_task_info(void)
{
    UBaseType_t array_size = uxTaskGetNumberOfTasks();
    TaskStatus_t *task_status_array = rtos_malloc(array_size * sizeof(TaskStatus_t));
    if (task_status_array) {
        const char task_st_ch[] = {'X', 'R', 'B', 'S', 'D'};
        UBaseType_t idx;
        array_size = uxTaskGetSystemState(task_status_array, array_size, NULL);
        AIC_LOG_PRINTF("TaskName\tNum\tPrio\tStat\tStkHWM\tHdl\tStkBA\n");
        for (idx = 0; idx < array_size; idx++) {
            AIC_LOG_PRINTF("%s%s%d\t%d\t%c\t%d\t%x\t%x\n",
                task_status_array[idx].pcTaskName,
                (strlen(task_status_array[idx].pcTaskName) < (configMAX_TASK_NAME_LEN - 8)) ? "\t\t" : "\t", 
task_status_array[idx].xTaskNumber,
                task_status_array[idx].uxCurrentPriority, task_st_ch[task_status_array[idx].eCurrentState],
                task_status_array[idx].usStackHighWaterMark, (uint32_t)task_status_array[idx].xHandle, (uint32_t)
task_status_array[idx].pxStackBase);
        }
        rtos_free(task_status_array);
    }
}
#endif

int rtos_task_init_notification(rtos_task_handle task)
{
    return 0;
}

uint32_t rtos_task_wait_notification(int timeout)
{
    return ulTaskNotifyTake(pdTRUE, rtos_timeout_2_tickcount(timeout));
}

void rtos_task_notify(rtos_task_handle task_handle, uint32_t value, bool isr)
{
    if (isr)
    {
        BaseType_t task_woken = pdFALSE;

        xTaskNotifyFromISR(task_handle, value, eSetValueWithOverwrite, &task_woken);
        portYIELD_FROM_ISR(task_woken);
    }
    else
    {
        xTaskNotify(task_handle, value, eSetValueWithOverwrite);
    }

}

void rtos_task_notify_setbits(rtos_task_handle task_handle, uint32_t value, bool isr)
{
    if (isr)
    {
        BaseType_t task_woken = pdFALSE;

        xTaskNotifyFromISR(task_handle, value, eSetBits, &task_woken);
        portYIELD_FROM_ISR(task_woken);
    }
    else
    {
        xTaskNotify(task_handle, value, eSetBits);
    }

}

uint32_t rtos_task_get_priority(rtos_task_handle task_handle)
{
    return uxTaskPriorityGet(task_handle);

}

void rtos_task_set_priority(rtos_task_handle task_handle, uint32_t priority)
{
    vTaskPrioritySet(task_handle, priority);

}

int rtos_queue_create(int elt_size, int nb_elt, rtos_queue *queue, const char * const name)
{
    *queue = xQueueCreate(nb_elt, elt_size);

    if ( *queue == NULL )
        return -1;

    return 0;
}

void rtos_queue_delete(rtos_queue queue)
{
    vQueueDelete(queue);
}

bool rtos_queue_is_empty(rtos_queue queue)
{
    BaseType_t res;

    //GLOBAL_INT_DISABLE();
    res = xQueueIsQueueEmptyFromISR(queue);
    //GLOBAL_INT_RESTORE();

    return (res == pdTRUE);

}

bool rtos_queue_is_full(rtos_queue queue)
{
    BaseType_t res;

    //GLOBAL_INT_DISABLE();
    res = xQueueIsQueueFullFromISR(queue);
    //GLOBAL_INT_RESTORE();

    return (res == pdTRUE);
}

int rtos_queue_cnt(rtos_queue queue)
{
    UBaseType_t res;

	//GLOBAL_INT_DISABLE();
    res = uxQueueMessagesWaiting(queue);//uxQueueMessagesWaitingFromISR(queue);
	//GLOBAL_INT_RESTORE();
    return ((int)res);
}

int rtos_queue_write(rtos_queue queue, void *msg, int timeout, bool isr)
{
    BaseType_t res;

    if (isr)
    {
        BaseType_t task_woken = pdFALSE;

        res = xQueueSendToBackFromISR(queue, msg, &task_woken);
        portYIELD_FROM_ISR(task_woken);
    }
    else
    {
        res = xQueueSendToBack(queue, msg, rtos_timeout_2_tickcount(timeout));
    }

    return (res == errQUEUE_FULL);

}

int rtos_queue_read(rtos_queue queue, void *msg, int timeout, bool isr)
{
     BaseType_t res = pdPASS;

    if (isr)
    {
        BaseType_t task_woken = pdFALSE;

        res = xQueueReceiveFromISR(queue, msg, &task_woken);
        portYIELD_FROM_ISR(task_woken);
    }
    else
    {
        res = xQueueReceive(queue, msg, rtos_timeout_2_tickcount(timeout));
    }

    return (res == errQUEUE_EMPTY);
}

int rtos_queue_peek(rtos_queue queue, void *msg, int timeout, bool isr)
{
    BaseType_t res = pdPASS;

    if (isr)
    {
        res = xQueuePeekFromISR(queue, msg);
    }
    else
    {
        res = xQueuePeek(queue, msg, rtos_timeout_2_tickcount(timeout));
    }

    return (res == errQUEUE_EMPTY);
}

int rtos_queue_reset(rtos_queue queue)
{
    BaseType_t res = pdPASS;

    res = xQueueReset(queue);

    return (res == pdPASS);
}

int rtos_semaphore_create(rtos_semaphore *semaphore, const char * const name, int max_count, int init_count)
{
    int res = -1;

    if (max_count == 1)
    {
        *semaphore = xSemaphoreCreateBinary();

        if (*semaphore != 0)
        {
            if (init_count)
            {
                xSemaphoreGive(*semaphore);
            }
            res = 0;
        }
    }
    else
    {
        *semaphore = xSemaphoreCreateCounting(max_count, init_count);

        if (*semaphore != 0)
        {
            res = 0;
        }
    }

    return res;
}

void rtos_semaphore_delete(rtos_semaphore semaphore)
{
    vSemaphoreDelete(semaphore);
}

int rtos_semaphore_get_count(rtos_semaphore semaphore)
{
    return uxSemaphoreGetCount(semaphore);
}

int rtos_semaphore_wait(rtos_semaphore semaphore, int timeout)
{
    BaseType_t res = pdPASS;
	
   	res = xSemaphoreTake(semaphore, rtos_timeout_2_tickcount(timeout));

    return (res == errQUEUE_EMPTY);
}

int rtos_semaphore_signal(rtos_semaphore semaphore, bool isr)
{
    BaseType_t res;

    if (isr)
    {
        BaseType_t task_woken = pdFALSE;

        res = xSemaphoreGiveFromISR(semaphore, &task_woken);
        portYIELD_FROM_ISR(task_woken);
    }
    else
    {
        res = xSemaphoreGive(semaphore);
    }

    return (res == errQUEUE_FULL);
}

TimerHandle_t rtos_timer_create( const char * const pcTimerName,
                            const TickType_t xTimerPeriodInTicks,
                            const UBaseType_t uxAutoReload,
                            void * const pvTimerID,
                            TimerCallbackFunction_t pxCallbackFunction )
{
    return xTimerCreate(pcTimerName , xTimerPeriodInTicks , uxAutoReload , pvTimerID , pxCallbackFunction);
}

int rtos_timer_stcart(TimerHandle_t xTimer,TickType_t xTicksToWait, bool isr)
{
    BaseType_t res = pdPASS;
    if (!isr) {
        res = xTimerStart(xTimer, xTicksToWait);
    } else {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        res = xTimerStartFromISR(xTimer, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken) {
            taskYIELD();
        }
    }
    return (res == pdFAIL);
}

int rtos_timer_stop(TimerHandle_t xTimer,TickType_t xTicksToWait)
{
    BaseType_t res = pdPASS;
    res = xTimerStop(xTimer,xTicksToWait);

    return (res == pdFAIL);
}

int rtos_timer_stop_isr(rtos_timer xTimer)
{
    BaseType_t res = pdPASS;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    res = xTimerStopFromISR(xTimer, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        taskYIELD();
    }
    return (res == pdFAIL);
}

int rtos_timer_delete(TimerHandle_t xTimer,TickType_t xTicksToWait)
{
    BaseType_t res = pdPASS;
    res = xTimerDelete(xTimer,xTicksToWait);

    return (res == pdFAIL);
}

int rtos_timer_change_period(TimerHandle_t xTimer, TickType_t xNewPeriod,TickType_t xTicksToWait)
{
    BaseType_t res = pdPASS;

    res = xTimerChangePeriod(xTimer, xNewPeriod, xTicksToWait);

    return (res == pdFAIL);
}

int rtos_timer_change_period_isr(TimerHandle_t xTimer, TickType_t xNewPeriod)
{
    BaseType_t res = pdPASS;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    res = xTimerChangePeriodFromISR(xTimer, xNewPeriod, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        taskYIELD();
    }
    return (res == pdFAIL);
}

int rtos_timer_restart(TimerHandle_t xTimer,TickType_t xTicksToWait, bool isr)
{
    BaseType_t res = pdPASS;
    if (!isr) {
        res = xTimerReset(xTimer, xTicksToWait);
    } else {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        res = xTimerResetFromISR(xTimer, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken) {
            taskYIELD();
        }
    }
    return (res == pdFAIL);
}

void *rtos_timer_get_pvTimerID( TimerHandle_t xTimer )
{
    return pvTimerGetTimerID(xTimer);
}

int rtos_mutex_recursive_create(rtos_mutex *mutex)
{
    int res = -1;

    *mutex = xSemaphoreCreateRecursiveMutex();
    if (*mutex != 0)
    {
        res = 0;
    }

    return res;
}

int rtos_mutex_recursive_lock(rtos_mutex mutex)
{
    BaseType_t res = pdPASS;

    res =  xSemaphoreTakeRecursive(mutex, portMAX_DELAY);

    return (res == pdFAIL);
}

int rtos_mutex_recursive_unlock(rtos_mutex mutex)
{
    BaseType_t res = pdPASS;

    res =  xSemaphoreGiveRecursive(mutex);

    return (res == pdFAIL);
}

int rtos_mutex_create(rtos_mutex *mutex, const char * const name)
{
    int res = -1;

    *mutex = xSemaphoreCreateMutex();
    if (*mutex != 0)
    {
        res = 0;
    }

    return res;
}

void rtos_mutex_delete(rtos_mutex mutex)
{
    ASSERT_ERR(xSemaphoreGetMutexHolder(mutex) == NULL);
    vSemaphoreDelete(mutex);
}

int rtos_mutex_lock(rtos_mutex mutex, int timeout)
{
    BaseType_t res = pdPASS;

    res = xSemaphoreTake(mutex, rtos_timeout_2_tickcount(timeout));

    return (res == pdFAIL);
}

int rtos_mutex_unlock(rtos_mutex mutex)
{
    BaseType_t res = pdPASS;

    res = xSemaphoreGive(mutex);

    return (res == pdFAIL);
}

int rtos_event_group_create(rtos_event_group *event_group)
{
    int res = -1;

    *event_group = xEventGroupCreate();
    if (*event_group != 0)
    {
        res = 0;
    }

    return res;
}

void rtos_event_group_delete(rtos_event_group event_group)
{
    vEventGroupDelete(event_group);
}

uint32_t rtos_event_group_get_bits(rtos_event_group event_group, bool isr)
{
    if (isr) {
        return xEventGroupGetBitsFromISR(event_group);
    } else {
        return xEventGroupGetBits(event_group);
    }
}

uint32_t rtos_event_group_wait_bits(rtos_event_group event_group, const uint32_t val,
    const bool clear_on_exit, const bool wait_all_bits, int timeout)
{
    return xEventGroupWaitBits(event_group, val, clear_on_exit, wait_all_bits, rtos_timeout_2_tickcount(timeout));
}

uint32_t rtos_event_group_clear_bits(rtos_event_group event_group, const uint32_t val, bool isr)
{
    if (isr) {
        return xEventGroupClearBitsFromISR(event_group, val);
    } else {
        return xEventGroupClearBits(event_group, val);
    }
}

uint32_t rtos_event_group_set_bits(rtos_event_group event_group, const uint32_t val, bool isr)
{
    uint32_t res;

    if (isr) {
        BaseType_t task_woken = pdFALSE;

        res = xEventGroupSetBitsFromISR(event_group, val, &task_woken);
        portYIELD_FROM_ISR(task_woken);
    } else {
        res = xEventGroupSetBits(event_group, val);
    }

    return res;
}

uint32_t rtos_protect(void)
{
    taskENTER_CRITICAL();
    return 1;
}

void rtos_unprotect(uint32_t protect)
{
    (void) protect;
    taskEXIT_CRITICAL();
}

void rtos_start_scheduler(void)
{
    vTaskStartScheduler();
}


int rtos_init(void)
{
    #if OSAL_TRACE
    memset(task_table, 0, sizeof(task_table));
    #endif

    return 0;
}

rtos_task_handle rtos_get_task_handle(void)
{
    return xTaskGetCurrentTaskHandle();
}

rtos_sched_state rtos_get_scheduler_state(void)
{
    return xTaskGetSchedulerState();
}

void rtos_trace_task(int id, void *task)
{
    #if OSAL_TRACE
    enum rtos_task_id task_id = rtos_trace_task_id(task);

    if (id == RTOS_TRACE_SWITCH_IN)
    {
        AIC_LOG_PRINTF("Enter Task %d,%d\n", task_id, rtos_now(false));
    }
    else if (id == RTOS_TRACE_SWITCH_OUT)
    {
        AIC_LOG_PRINTF("Exit Task %d,%d\n", task_id, rtos_now(false));
    }
    else if (id == RTOS_TRACE_DELETE)
    {
        AIC_LOG_PRINTF("Delete task %d\n", task_id);
    }
    else if (id == RTOS_TRACE_SUSPEND)
    {
        AIC_LOG_PRINTF("Suspend task %d\n", task_id);
    }
    else if (id == RTOS_TRACE_RESUME)
    {
        AIC_LOG_PRINTF("Resume task %d\n", task_id);
    }
    else if (id == RTOS_TRACE_RESUME_FROM_ISR)
    {
        AIC_LOG_PRINTF("Resume from ISR task %d\n", task_id);
    }
    else if (id == RTOS_TRACE_CREATE)
    {
        if ((creating_task_id == UNDEF_TASK) &&
            (task_table[IDLE_TASK] == NULL)
            #if( configSUPPORT_STATIC_ALLOCATION == 0 )
            && (task == xTaskGetIdleTaskHandle())
            #endif
            )
        {
            task_table[IDLE_TASK] = task;
            creating_task_id = IDLE_TASK;
        }
        else if ((creating_task_id == UNDEF_TASK) &&
            (task_table[TMR_DAEMON_TASK] == NULL)
        #if( configSUPPORT_STATIC_ALLOCATION == 0 )
            && (task == xTimerGetTimerDaemonTaskHandle())
        #endif
            )
        {
            task_table[TMR_DAEMON_TASK] = task;
            creating_task_id = TMR_DAEMON_TASK;
        }
        else if (creating_task_id < MAX_TASK)
        {
            task_table[creating_task_id] = task;
        }

        AIC_LOG_PRINTF("Create task %d\n", creating_task_id);
        creating_task_id = UNDEF_TASK;
    }
    #endif
}

void rtos_trace_mem(int id, void *ptr, int size, int free_size)
{
    #if OSAL_TRACE
    enum rtos_task_id task_id = rtos_trace_task_id(NULL);

    if (id == RTOS_TRACE_ALLOC)
    {
        if (ptr == NULL)
        {
            AIC_LOG_PRINTF("[%d] Failed to allocate %d bytes. (free_size = %d)\n",
                  task_id, size, free_size);
        }
        #if RTOS_MALLOC_TRACE_LEVEL > 0
        else
        {
            AIC_LOG_PRINTF("[%d] Allocate %d bytes at %p. (free_size = %d)\n",
                       task_id, size, (ptr), free_size);
        }
        #endif
    }
    else if (id == RTOS_TRACE_FREE)
    {

        AIC_LOG_PRINTF("[%d] Free %d bytes at %p. (free_size = %d)\n",
                   task_id, size, (ptr), free_size);
    }
    #endif
}

void rtos_priority_set(rtos_task_handle handle, rtos_prio priority)
{
    vTaskPrioritySet(handle, priority);
}

#if 0 //(configCHECK_FOR_STACK_OVERFLOW > 0)
void vApplicationStackOverflowHook(TaskHandle_t* xTask, char *pcTaskName)
{
    AIC_LOG_PRINTF("Stack overflow detected for task "
        #if OSAL_TRACE
        "%d"
        #endif
        "(%x)\n",
            #if OSAL_TRACE
            rtos_trace_task_id(xTask),
            #endif
            (uint32_t)xTask);
    dump_mem((uint32_t)xTask, 20, 4, true);
    ASSERT_ERR(0);
}
#endif /* (configCHECK_FOR_STACK_OVERFLOW > 0) */

#if (PLF_AIC8800)
PRIVATE_HOST_DECLARE(G0RTOS, TickType_t, xTickCount);
#endif

void rtos_data_save(void)
{
    #if (PLF_AIC8800)
    // backup data
    backup_xTickCount = xTaskGetTickCount();
    sleep_data_save();
    #if PLF_AON_SUPPORT
    co_timer_save();
    #endif
    #if PLF_WIFI_STACK
    #ifdef CONFIG_RWNX_LWIP
    fhost_data_save();
    lwip_data_save();
    wpas_data_save();
    ipc_host_flag_save();
    #endif /* CONFIG_RWNX_LWIP */
    #endif /* PLF_WIFI_STACK */
    user_data_save();
    lp_ticker_data_save();
    #endif
}

int aic_time_get(enum time_origin_t origin, uint32_t *sec, uint32_t *usec)
{
    struct timeval tvp;
	int ret = gettimeofday(&tvp, NULL);
	
	if (ret != 0)
		return ret;

    *sec = tvp.tv_sec;
    *usec = tvp.tv_usec;

    return 0;
}

