/*****************************************************************************
**
**  Name:          bte_logmsg.c
**
**  Description: Contains the LogMsg wrapper routines for BTE.  It routes calls
**               the appropriate application's LogMsg equivalent.
**
**  Copyright (c) 2018-2019, Broadcom Inc.
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "bta_platform.h"
#include "bte_glue.h"

#include "buildcfg.h"

#define BTE_LOG_STATIC_BUF_SIZE           512

extern taskinfo_t TaskInfoID[GKI_MAX_TASKS];
#define BTE_LOG_PRINT_TASK_PRIORITY             TaskInfoID[BTAPP_CONSOLE_TASK].taskPriority
#define BTE_LOG_PRINT_TASK_STACK_SIZE           TaskInfoID[BTAPP_CONSOLE_TASK].taskStackSize
#define BTE_LOG_PRINT_QUEUE_SIZE          (4)
#define BTE_LOG_PRINT_QUEUE_CNT           (500)
#define RTOS_WAIT_FOREVER                 (portMAX_DELAY)

typedef struct
{
uint16_t count;
uint8_t* buf;
} bte_log_print_t;

static unsigned char is_log_enable = 1;
static unsigned char log_enable_mask = 0;
//static uint8_t buffer[BTE_LOG_STATIC_BUF_SIZE];
SemaphoreHandle_t bte_trace_log_mutex = NULL;
QueueHandle_t bte_log_print_queue = NULL;
TaskHandle_t  h_print_task = NULL;
static uint8_t    bte_log_inited = 0;

pthread_t h_print_pthread;
static void bte_log_print_task(void* pvParamters);


void BTE_EnableLog(UINT8 mask)
{
    log_enable_mask |= mask;

    if(log_enable_mask == 0x01)
        is_log_enable = 1;
}

void BTE_DisableLog(UINT8 mask)
{
    log_enable_mask &= ~mask;
    if(log_enable_mask != 0x01)
        is_log_enable = 0;
}

void BTE_DeleteLogModule(void)
{
}


char* LogGetTraceLayerMsg(UINT32 trace_set_mask)
{
    UINT32 trace_layer_idx = (trace_set_mask & 0x00FF0000);

    switch(trace_layer_idx)
    {
        case TRACE_LAYER_NONE:
            return "[APPL  ]";
            break;
        case TRACE_LAYER_USB:
            return "[USB   ]";
            break;
        case TRACE_LAYER_SERIAL:
            return "[SERIAL]";
            break;
        case TRACE_LAYER_LC:
            return "[LC    ]";
            break;
        case TRACE_LAYER_LM:
            return "[LM    ]";
            break;
        case TRACE_LAYER_HCI:
            return "[HCI   ]";
            break;
        case TRACE_LAYER_L2CAP:
            return "[L2CAP ]";
            break;
        case TRACE_LAYER_RFCOMM:
            return "[RFCOMM]";
            break;
        case TRACE_LAYER_SDP:
            return "[SDP   ]";
            break;
        case TRACE_LAYER_BTM:
            return "[BTM   ]";
            break;
        case TRACE_LAYER_GAP:
            return "[GAP   ]";
            break;
        case TRACE_LAYER_HSP2:
            return "[HSP2  ]";
            break;
        case TRACE_LAYER_SPP:
            return "[SPP   ]";
            break;
        case TRACE_LAYER_BTU:
            return "[BTU   ]";
            break;
        case TRACE_LAYER_GKI:
            return "[GKI   ]";
            break;
        case TRACE_LAYER_HFP:
            return "[HFP   ]";
            break;
        case TRACE_LAYER_AVP:
            return "[AVP   ]";
            break;
        case TRACE_LAYER_A2D:
            return "[A2D   ]";
            break;
        case TRACE_LAYER_ATT:
            return "[ATT   ]";
            break;
        case TRACE_LAYER_SMP:
            return "[SMP   ]";
            break;
        case TRACE_LAYER_CONSOLE:
            return "[CONSOLE]";
            break;
        case TRACE_LAYER_PAN:
            return "[PAN   ]";
            break;
        case TRACE_LAYER_BNEP:
            return "[BNEP  ]";
            break;
        default:
            break;
    }
    return "[Undef ]";
}

char* LogGetTraceType(UINT32 trace_set_mask)
{
    UINT32 trace_type = (trace_set_mask & 0x000000FF);
    switch (trace_type)
    {
        case TRACE_TYPE_ERROR:
            return "\033[0;31m";
            break;
        case TRACE_TYPE_WARNING:
            return "\033[0;32m";
            break;
        case TRACE_TYPE_API:
            return "\033[0;37m";
            break;
        case TRACE_TYPE_EVENT:
            return "\033[0;37m";
            break;
        case TRACE_TYPE_DEBUG:
            return "\033[0;37m";
            break;
    }
    return "\033[0;37m";
}

char* LogGetTimeStamp(void)
{
    UINT32 curr_tick = GKI_get_os_tick_count();
    static char buf[9];

    buf[0] = curr_tick/1000000 + '0';
    buf[1] = curr_tick%1000000/100000 + '0';
    buf[2] = curr_tick%1000000%100000/10000 + '0';
    buf[3] = curr_tick%1000000%100000%10000/1000 + '0';
    buf[4] = '.';
    buf[5] = curr_tick%1000000%100000%10000%1000/100 + '0';
    buf[6] = curr_tick%1000000%100000%10000%1000%100/10 + '0';
    buf[7] = curr_tick%10 + '0';
    buf[8] = '\0';

    return buf;
}

#if BT_USE_TRACES == TRUE
void LogMsg(UINT32 trace_set_mask, const char *fmt_str, ...)
{
    uint8_t buffer[BTE_LOG_STATIC_BUF_SIZE];
    va_list ap;
    UINT16 idx = 0;
	bte_log_print_t* hprint = NULL;
	signed portBASE_TYPE result;	

#if 1
    if(!is_log_enable || !btapp_cfg.stack_trace_enable)
        return;
#endif
    
    if (bte_trace_log_mutex == NULL)
    {
        //printf("init log mutex\r\n");
        bte_trace_log_mutex = xSemaphoreCreateMutex();
        if (bte_trace_log_mutex == NULL)
        {   //TODO: fatal error
            printf("LogMsg mutex create fail\r\n");
            return;
        }
        xSemaphoreGive(bte_trace_log_mutex);
    }

	if(bte_log_print_queue == NULL)
    {
		bte_log_print_queue = xQueueCreate( BTE_LOG_PRINT_QUEUE_CNT, BTE_LOG_PRINT_QUEUE_SIZE );
		if(bte_log_print_queue == NULL)
		printf("LogMsg Queue create fail\r\n");
	    return;
    }

    if(bte_log_inited == 0)
    {
         result = xTaskCreate( &bte_log_print_task, \
					  "bte_log_task", \
					  BTE_LOG_PRINT_TASK_STACK_SIZE, \
					  NULL,
                      BTE_LOG_PRINT_TASK_PRIORITY, \
					  &h_print_task);
		printf("init BTE log task %d\r\n",result);
        bte_log_inited = 1;
    }
    
    //xSemaphoreTake(bte_trace_log_mutex, RTOS_WAIT_FOREVER );
    if(xSemaphoreTake(bte_trace_log_mutex, RTOS_WAIT_FOREVER ) != pdTRUE)
    {
        return;
    }
#if 1
    va_start(ap, fmt_str);
    //idx = snprintf((char*)&buffer[idx],  BTE_LOG_STATIC_BUF_SIZE, "%s", LogGetTraceType(trace_set_mask));
    idx += snprintf((char*)&buffer[idx], BTE_LOG_STATIC_BUF_SIZE, "%s ", LogGetTimeStamp());
    idx += snprintf((char*)&buffer[idx], BTE_LOG_STATIC_BUF_SIZE, "%s ", LogGetTraceLayerMsg(trace_set_mask));
    idx += vsnprintf((char*)&buffer[idx], BTE_LOG_STATIC_BUF_SIZE, fmt_str, ap);
    idx += snprintf((char*)&buffer[idx], BTE_LOG_STATIC_BUF_SIZE, "%s", "\r\n");
	buffer[idx] = 0;
    va_end(ap);
	//check buf may overflow
	if( idx >= (BTE_LOG_STATIC_BUF_SIZE - 1))
	{
      xSemaphoreGive(bte_trace_log_mutex);
	  return;
    }
    hprint = malloc(sizeof(bte_log_print_t));

	if (hprint == NULL)
	{
	   xSemaphoreGive(bte_trace_log_mutex);
	   return;
	}

	hprint->buf = malloc(idx + 1);
    if(hprint->buf == NULL)
    {
        free(hprint);
	    xSemaphoreGive(bte_trace_log_mutex);
        return;
    }
	memcpy(hprint->buf, buffer, (idx + 1));
    hprint->count = (idx + 1);
	
	if(bte_log_print_queue != NULL && hprint->count != 0)
    {
		result = xQueueSendToBack( bte_log_print_queue, (void*)&hprint, 0);
        if(result != pdPASS)
        {
            free(hprint->buf);
            free(hprint);
        }
    }
    else
    {
        free(hprint->buf);
        free(hprint);
    }

    xSemaphoreGive(bte_trace_log_mutex);

	return;
	
#else

    va_start(ap, fmt_str);
    //idx = snprintf((char*)&buffer[idx],  BTE_LOG_STATIC_BUF_SIZE, "%s", LogGetTraceType(trace_set_mask));
    idx += snprintf((char*)&buffer[idx], BTE_LOG_STATIC_BUF_SIZE, "%s ", LogGetTimeStamp());
    idx += snprintf((char*)&buffer[idx], BTE_LOG_STATIC_BUF_SIZE, "%s ", LogGetTraceLayerMsg(trace_set_mask));
    idx += vsnprintf((char*)&buffer[idx], BTE_LOG_STATIC_BUF_SIZE, fmt_str, ap);
    idx += snprintf((char*)&buffer[idx], BTE_LOG_STATIC_BUF_SIZE, "%s", "\r\n");
    buffer[idx] = 0;
    va_end(ap);
    printf("%s",buffer);
#endif
    xSemaphoreGive(bte_trace_log_mutex);
}

#else

void LogMsg(UINT32 trace_set_mask, const char *fmt_str, ...)
{

}

#endif

extern SemaphoreHandle_t uart_tx_sem;
static void bte_log_print_task(void* pvParamters)
{
    bte_log_print_t* hprint = NULL;
    uint32_t queue_content;

    UNUSED(pvParamters);

    while(1)
    {
        //Use the portMAX_DELAY to wait queue, so must get the queue item
        xQueueReceive( bte_log_print_queue, (void*)&queue_content, RTOS_WAIT_FOREVER  );
        hprint = (bte_log_print_t*)queue_content;
        printf("%s", hprint->buf);
        free(hprint->buf);
        free(hprint);
    }
}

/********************************************************************************
 **
 **    Function Name:   LogMsg_0
 **
 **    Purpose:  Encodes a trace message that has no parameter arguments
 **
 **    Input Parameters:  trace_set_mask: tester trace type.
 **                       fmt_str: displayable string.
 **    Returns:
 **                      Nothing.
 **
 *********************************************************************************/
void LogMsg_0( UINT32 trace_set_mask, const char *fmt_str )
{
    LogMsg( trace_set_mask, fmt_str );
}

/********************************************************************************
 **
 **    Function Name:   LogMsg_1
 **
 **    Purpose:  Encodes a trace message that has one parameter argument
 **
 **    Input Parameters:  trace_set_mask: tester trace type.
 **                       fmt_str: displayable string.
 **    Returns:
 **                      Nothing.
 **
 *********************************************************************************/
void LogMsg_1( UINT32 trace_set_mask, const char *fmt_str, UINTPTR p1 )
{
    LogMsg( trace_set_mask, fmt_str, p1 );
}

/********************************************************************************
 **
 **    Function Name:   LogMsg_2
 **
 **    Purpose:  Encodes a trace message that has two parameter arguments
 **
 **    Input Parameters:  trace_set_mask: tester trace type.
 **                       fmt_str: displayable string.
 **    Returns:
 **                      Nothing.
 **
 *********************************************************************************/
void LogMsg_2( UINT32 trace_set_mask, const char *fmt_str, UINTPTR p1, UINTPTR p2 )
{
    LogMsg( trace_set_mask, fmt_str, p1, p2 );
}

/********************************************************************************
 **
 **    Function Name:   LogMsg_3
 **
 **    Purpose:  Encodes a trace message that has three parameter arguments
 **
 **    Input Parameters:  trace_set_mask: tester trace type.
 **                       fmt_str: displayable string.
 **    Returns:
 **                      Nothing.
 **
 *********************************************************************************/
void LogMsg_3( UINT32 trace_set_mask, const char *fmt_str, UINTPTR p1, UINTPTR p2, UINTPTR p3 )
{
    LogMsg( trace_set_mask, fmt_str, p1, p2, p3 );
}

/********************************************************************************
 **
 **    Function Name:   LogMsg_4
 **
 **    Purpose:  Encodes a trace message that has four parameter arguments
 **
 **    Input Parameters:  trace_set_mask: tester trace type.
 **                       fmt_str: displayable string.
 **    Returns:
 **                      Nothing.
 **
 *********************************************************************************/
void LogMsg_4( UINT32 trace_set_mask, const char *fmt_str, UINTPTR p1, UINTPTR p2, UINTPTR p3, UINTPTR p4 )
{
    LogMsg( trace_set_mask, fmt_str, p1, p2, p3, p4 );
}

/********************************************************************************
 **
 **    Function Name:   LogMsg_5
 **
 **    Purpose:  Encodes a trace message that has five parameter arguments
 **
 **    Input Parameters:  trace_set_mask: tester trace type.
 **                       fmt_str: displayable string.
 **    Returns:
 **                      Nothing.
 **
 *********************************************************************************/
void LogMsg_5( UINT32 trace_set_mask, const char *fmt_str, UINTPTR p1, UINTPTR p2, UINTPTR p3, UINTPTR p4, UINTPTR p5 )
{
    LogMsg( trace_set_mask, fmt_str, p1, p2, p3, p4, p5 );
}

/********************************************************************************
 **
 **    Function Name:   LogMsg_6
 **
 **    Purpose:  Encodes a trace message that has six parameter arguments
 **
 **    Input Parameters:  trace_set_mask: tester trace type.
 **                       fmt_str: displayable string.
 **    Returns:
 **                      Nothing.
 **
 *********************************************************************************/
void LogMsg_6( UINT32 trace_set_mask, const char *fmt_str, UINTPTR p1, UINTPTR p2, UINTPTR p3, UINTPTR p4, UINTPTR p5, UINTPTR p6 )
{
    LogMsg( trace_set_mask, fmt_str, p1, p2, p3, p4, p5, p6 );
}


