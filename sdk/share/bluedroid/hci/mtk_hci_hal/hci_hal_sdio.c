/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#define LOG_TAG "bt_hci_sdio"

#include <assert.h>
#include <errno.h>
#include <string.h>

#include "hci/include/hci/hci_hal.h"
#include "osi/osi.h"
#include "osi/thread.h"
#include "hci/include/hci/vendor.h"

#include "hci/include/hci/buffer_allocator.h"
#include "hci/include/hci/hci_layer.h"

#include "openrtos/FreeRTOS.h"
#include "openrtos/semphr.h"
#include "openrtos/task.h"
#include "ite/ith.h"
#include <pthread.h>
#include "openrtos/FreeRTOS_CLI.h"
#include "btmtk_main.h"

extern int picus_cmd_handler(int argc, char *argv[]);
extern uint8_t btpriv_cli(uint8_t len, char *param[]);
//instead of bt_read
//extern int btmtk_rx(unsigned char *buf, unsigned int buf_len);

#define HCI_COMMAND_PKT         0x01
#define HCI_ACLDATA_PKT         0x02
#define HCI_SCODATA_PKT         0x03
#define HCI_EVENT_PKT           0x04

#define HCI_HAL_SERIAL_BUFFER_SIZE 1026
#define HCI_BLE_EVENT 0x3e

char *cmd[3];
// Increased HCI thread priority to keep up with the audio sub-system
// when streaming time sensitive data (A2DP).
#define HCI_THREAD_PRIORITY -19

//callback to hci layer
static const hci_hal_callbacks_t *sdio_hal_callbacks;

//rx task
//static pthread_t hci_sdio_rx_task_handler;
TaskHandle_t hci_sdio_rx_task_handler = NULL;
xSemaphoreHandle hal_action_mutex;
static int task_run;

///definition to get MTK_BUFFER's control buffer context pointer
#define BT_CONTEXT(_Rtb) ((struct BT_RTB_CONTEXT *)((_Rtb)->Context))



#define PACKET_TYPE_TO_INDEX(type) ((type) - 1)
static const uint16_t outbound_event_types[] = {
    MSG_HC_TO_STACK_HCI_ERR,
    MSG_HC_TO_STACK_HCI_ACL,
    MSG_HC_TO_STACK_HCI_SCO,
    MSG_HC_TO_STACK_HCI_EVT
};

static const allocator_t *buffer_allocator;

static void sdio_data_ready_cb(BT_HDR *packet);

static void driver_data_ready_cb(void)
{
    //printf("%s", __func__);
    if (hci_sdio_rx_task_handler)
        xTaskNotifyGive(hci_sdio_rx_task_handler);
}


void hci_hal_sdio_rx_handler(void *arg)
{
    BtTaskEvt_t e;
    sk_buff *data_skb;
    printf("Start %s\n", __func__);

    while (task_run) {
            //printf("hci hal rx server sleep\n");
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            //printf("hci hal rx server wakeup\n");
            //printf("rx require \n");
            if (xSemaphoreTake(hal_action_mutex, portMAX_DELAY) == pdFALSE) {
                printf("%s, hal_action_mutex get failed\n", __func__);
                break;
            }

            //repeat read
            data_skb = bt_read();
            
            while (data_skb != NULL) {
                //printf("data_skb %p", &data_skb);

                volatile int length = skb_get_data_length(data_skb);
                //free packet in hci upper layer
                BT_HDR *packet = buffer_allocator->alloc(length + sizeof(BT_HDR)); 

                if (packet == NULL) {
                    skb_free(&data_skb);
                    printf("cant allocate packet\n");
                    break;
                }
                memcpy(packet->data, data_skb->data, length);

                packet->layer_specific = 0;
                packet->offset = 0;
                packet->len = length - packet->offset;
                packet->event = outbound_event_types[PACKET_TYPE_TO_INDEX(skb_get_pkt_type(data_skb))];

                skb_free(&data_skb);
                sdio_data_ready_cb(packet);
                data_skb = bt_read();
            }
            //printf("rx release\n");
            xSemaphoreGive(hal_action_mutex);
    }

    printf("exit %s", __func__);
}

#ifdef CFG_MT7921S_ENABLE_PICUS


void vTaskPicus() {

    printf("enter vTaskBoots\n");
    printf("Command: %s %s %s\n", cmd[0], cmd[1], cmd[2]);
    picus_cmd_handler(3, cmd);
    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("Task completed\n");

    vPortFree(cmd[1]);
    vPortFree(cmd[2]);

    vTaskDelete(NULL);
}

static portBASE_TYPE PicusCLI(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    printf("%s enter\n", __func__);
    const char *param1;
    const char *param2;

    BaseType_t param1Length, param2Length;
    param1 = FreeRTOS_CLIGetParameter(pcCommandString, 1, &param1Length);
    param2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &param2Length);
    printf("%s %s param1Length=%d param2Length=%d\n", __func__, pcCommandString, param1Length, param2Length);

    cmd[0] = "picus";
    cmd[1] = (char *)pvPortMalloc((param1Length + 1) * sizeof(char));
    cmd[2] = (char *)pvPortMalloc((param2Length + 1) * sizeof(char));

    if (cmd[1] == NULL || cmd[2] == NULL) {
        printf("Memory allocation failed\n");
        return -1;
    }

    strncpy(cmd[1], param1, param1Length);
    cmd[1][param1Length] = '\0';

    strncpy(cmd[2], param2, param2Length);
    cmd[2][param2Length] = '\0';

    xTaskCreate(
        vTaskPicus,
        "picusTask",
        1024,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );

    // There is no more data to return after this single string, so return pdFALSE
    printf("exit picus cli\n");

    return pdFALSE;
}

static const xCommandLineInput xPicusCommand =
{
    ( const char * const ) "picus",
    ( const char * const ) "picus: mtk bluetooth firmware debug tool\n",
    PicusCLI,
    2 // parameter num
};
#endif

#ifdef CFG_MT7921S_ENABLE_BOOTS

void vTaskBoots() {
    printf("enter vTaskBoots\n");
    printf("Command: %s %s %s\n", cmd[0], cmd[1], cmd[2]);
    btpriv_cli(3, cmd);

    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("Task completed\n");
    vPortFree(cmd[1]);
    vPortFree(cmd[2]);
    vTaskDelete(NULL);
}

static portBASE_TYPE BootsCLI(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    printf("%s enter\n", __func__);
    const char *param1;
    const char *param2;

    BaseType_t param1Length, param2Length;
    param1 = FreeRTOS_CLIGetParameter(pcCommandString, 1, &param1Length);
    param2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &param2Length);
    printf("%s %s param1Length=%d param2Length=%d\n", __func__, pcCommandString, param1Length, param2Length);

    cmd[0] = "boots";
    cmd[1] = (char *)pvPortMalloc((param1Length + 1) * sizeof(char));
    cmd[2] = (char *)pvPortMalloc((param2Length + 1) * sizeof(char));

    if (cmd[1] == NULL || cmd[2] == NULL) {
        printf("Memory allocation failed\n");
        return -1;
    }

    strncpy(cmd[1], param1, param1Length);
    cmd[1][param1Length] = '\0';

    strncpy(cmd[2], param2, param2Length);
    cmd[2][param2Length] = '\0';

    xTaskCreate(
        vTaskBoots,
        "bootsTask",
        1024,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );

    // There is no more data to return after this single string, so return pdFALSE
    printf("exit boots cli\n");
    return pdFALSE;
}

static const xCommandLineInput xBootsCommand =
{
    ( const char * const ) "boots",
    ( const char * const ) "boots: mtk bluetooth factory test tool\n",
    BootsCLI,
    2 // parameter num
};
#endif

static bool sdio_hal_open(const hci_hal_callbacks_t *upper_callbacks)
{
    HCI_TRACE_DEBUG("%s", __func__);
    BaseType_t xReturn = 0;
    //1.save the hci layer callback
    configASSERT(upper_callbacks != NULL);
    sdio_hal_callbacks = upper_callbacks;
    //2.no longer used, uart to ,rx buffer queue?
    buffer_allocator = buffer_allocator_get_interface();
    //3.start hal rx thread
    task_run = 1;

    //open driver
    //TODI:retry open
    if (bt_open() == 0) {
#ifdef CFG_MT7921S_ENABLE_BOOTS  
        FreeRTOS_CLIRegisterCommand( &xBootsCommand );
        task_run = 0;
#endif
    if (task_run) {

            hal_action_mutex = xSemaphoreCreateMutex();
            if (hal_action_mutex == NULL) {
                printf("%s create hal_action_mutex fail\n", __func__);
                return false;
            }

        xReturn = xTaskCreate(
            hci_hal_sdio_rx_handler,
            "halRxTask",
            1024,
            NULL,
            tskIDLE_PRIORITY + 2,
            &hci_sdio_rx_task_handler
        );
        if (xReturn == pdPASS){
            bt_driver_register_event_cb(driver_data_ready_cb,0);
            printf("hci hal register driver data ready callback");
        }
    }

#ifdef CFG_MT7921S_ENABLE_PICUS
        FreeRTOS_CLIRegisterCommand( &xPicusCommand );
        char *cmd[3];
        cmd[0] = "picus";
        cmd[1] = "-x";
        cmd[2] = "1";
        picus_cmd_handler(3,cmd);
#endif
    }
    return true;
}

static void sdio_hal_close()
{
    HCI_TRACE_DEBUG("%s", __func__);
    if (hal_action_mutex)
        vSemaphoreDelete(hal_action_mutex);
    hal_action_mutex = NULL;
    bt_close();
    buffer_allocator = NULL;
    //free rx task
    task_run = 0;
    //pthread_join(hci_sdio_rx_task_handler, NULL);
    vTaskDelete(hci_sdio_rx_task_handler);
    //hci_sdio_rx_task_handler =  (pthread_t)NULL;
}

//this function never be called in hci_layer.c
static size_t sdio_read_data(serial_data_type_t type, uint8_t *buffer, size_t max_size)
{
    HCI_TRACE_ERROR("MTK Not implement %s", __func__);
    return 0;
}

//this function never be called in hci_layer.c
static void sdio_packet_finished(serial_data_type_t type)
{
    HCI_TRACE_ERROR("MTK Not implement %s", __func__);
}

static void sdio_data_ready_cb(BT_HDR *packet)
{
    //upload the event to hci layer
    sdio_hal_callbacks->packet_ready(packet);
}

sk_buff *skb_alloc_and_init_append_type(uint8_t PktType, uint8_t *Data, uint32_t  DataLen);

static uint16_t sdio_transmit_data(serial_data_type_t type, uint8_t *data, uint16_t length)
{
    configASSERT(data != NULL);
    configASSERT(length > 0);
    sk_buff  *skb = NULL;

    volatile uint16_t transmitted_length = 0;
    volatile uint16_t opcode;
    volatile uint8_t i;
    if (xSemaphoreTake(hal_action_mutex, portMAX_DELAY) == pdFALSE) {
        printf("%s, hal_action_mutex get failed\n", __func__);
        return 0;
    }

    //BTMTK_INFO_RAW(data, length, "%s type:%d  length:%d", __func__,type, length);

    if (type < DATA_TYPE_COMMAND || type > DATA_TYPE_SCO) {
        HCI_TRACE_ERROR("%s invalid data type: %d", __func__, type);
        return 0;
    }

    if (type == DATA_TYPE_COMMAND || type == DATA_TYPE_ACL || type == DATA_TYPE_SCO || type == DATA_TYPE_EVENT) {
        skb = skb_alloc_and_init_append_type(type, data, length);
    } else {
        HCI_TRACE_ERROR("no define type ptk!");
    }

    if (!skb) {
        HCI_TRACE_ERROR("skb_alloc_and_init fail!");
        return 0;
    }
    BTMTK_INFO_RAW(skb->data, skb->len, "%s packed: ", __func__);
    volatile uint16_t ret = bt_send_frame(skb);
    if ( ret > 0)
    {
        transmitted_length = length;
    }
    else
    {
        printf("XXXXXXXXX ret: %d\n", ret);
    }
    xSemaphoreGive(hal_action_mutex);
    //usleep(5000);
    //taskYIELD();
    return transmitted_length;
}

static const hci_hal_t sdio_interface = {
    sdio_hal_open,
    sdio_hal_close,
    sdio_read_data, //no longer used
    sdio_packet_finished,//no longer used
    sdio_transmit_data,
};

const hci_hal_t *hci_hal_sdio_get_interface()
{
    return &sdio_interface;
}
