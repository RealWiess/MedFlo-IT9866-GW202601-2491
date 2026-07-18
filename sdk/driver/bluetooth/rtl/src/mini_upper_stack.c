/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      mini_upper_stack.c
* @brief     the main function of the mini upper stack
* @author    Harvey_Guo
* @date      2019-10-14
* @version   v0.1
* *********************************************************************************************************
*/
#include <stdio.h>
#include "user_interface/osif.h"
#include "hci_if.h"
// #define DBG

#define HCI_READ_ADDR           0x1009
#define HCI_READ_LOCAL_VERSION  0x1001
#define HCI_SCAN_ENABLE         0x0c1a
#define HCI_WRITE_LOCAL_NAME    0x0c13

uint32_t    step        = 0;
uint32_t    recCount    = 0;
void        * up_stack_message_handler;
void        * up_stack_task_handler;
uint8_t     hci_data[20];
uint8_t     messagedata = 0;

typedef void up_func(void);
void
hci_send_start_cmd (void);

extern int  (*nim_rx_cb)(void *,
                              uint8_t);
extern int  (*nim_tx_cb)(void *);

void
up_stack_task (
    void * p_param)
{
    uint8_t receivedata;
    // (void)printf("up_stack_task runing\r\n");
    for (;;)
    {
        if (osif_msg_recv(up_stack_message_handler, &receivedata, 0xFFFFFFFFUL))
        {
            // (void)printf("up_stack_task message received\r\n");
            ((up_func *)p_param)();
        }
    }
}

void
up_handler (
    void)
{
    hci_send_start_cmd();
}

void
hci_send_msg (
    uint16_t    opcode,
    uint8_t     * buf,
    uint8_t     len)
{
    hci_data[0] = 0x01;
    hci_data[1] = opcode & 0xff;
    hci_data[2] = (opcode >> 8) & 0xff;
    hci_data[3] = len;
    if (len)
    {
        for (int i = 0; i < len; i++)
        {
            hci_data[4 + i] = buf[i];
        }
    }
    if (hci_if_write(hci_data, 4 + len) == false)
    {
        (void)printf("%s, %d:fail\r\n", __func__, __LINE__);
    }
}

void
hci_send_start_cmd (
    void)
{
    uint8_t hci_buf[20];
    // (void)printf("\r\n\n===step %x ====\n\n", step);
    switch (step)
    {
        case 0:
            hci_send_msg(HCI_READ_ADDR, NULL, 0);
            break;

        case 1:
            hci_send_msg(HCI_READ_LOCAL_VERSION, NULL, 0);
            break;

        case 2:
        {
            uint8_t LocalName[8] = {
                'a', 'h', 'a', 'r'
                ,    'v', 'e', 'y', 0
            };
            hci_send_msg(HCI_WRITE_LOCAL_NAME, LocalName, 8);
        }
        break;

        case 3:
            hci_buf[0] = 0x03;
            hci_send_msg(HCI_SCAN_ENABLE, hci_buf, 1);
            break;

        case 4:
            (void)printf("mini upperstack start ok\r\n");
            break;

        default:
            // (void)printf("step unknow %x\r\n", step);
            break;
    }
    step++;
}

bool
hci_callback (
    uint8_t     evt,
    bool        status,
    uint8_t     * p_buf,
    uint32_t    len)
{
    bool result = true;

    switch (evt)
    {
        case HCI_IF_EVT_OPENED: /* hci I/F open completed */
#ifdef DBG
            (void)printf("hci_if_callback HCI_IF_EVT_OPENED %x:\r\n", status);
#endif
            if (status == true)
            {
                hci_send_start_cmd();
            }
            break;

        case HCI_IF_EVT_CLOSED:
#ifdef DBG
            (void)printf("hci_if_callback HCI_IF_EVT_CLOSED %x:\r\n", status);
#endif
            break;

        case HCI_IF_EVT_DATA_IND:
#ifdef DBG
            (void)printf("hci_if_callback HCI_IF_EVT_DATA_IND %x:\r\n", status);
            (void)printf("Receive Data:\r\n");
#endif
            for (int i = 0; i < len; i++)
            {
#ifdef DBG
                (void)printf("0x%x ", p_buf[i]);
#endif
                if (step > 4)
                {
                    nim_rx_cb((void *)0x1, p_buf[i]);
                }
            }
#ifdef DBG
            (void)printf("\r\n");
#endif
            if (osif_msg_send(up_stack_message_handler, &messagedata, 0XFFFFFFFFUL) == false)
            {
                (void)printf("osif_msg_send fail\r\n");
            }

        case HCI_IF_EVT_DATA_XMIT:
#ifdef DBG
            (void)printf("hci_if_callback HCI_IF_EVT_DATA_XMIT %x:\r\n", status);
#endif
            break;

        case HCI_IF_EVT_ERROR:
#ifdef DBG
            (void)printf("hci_if_callback HCI_IF_EVT_ERROR %x:\r\n", status);
#endif
            break;

        default:
            (void)printf("hci_if_callback other evt:%x\r\n", evt);
            break;
    }
    return result;
}

bool
bte_init (
    void)
{
    bool ret = true;
    (void)printf("Mini_Upper_Stack\nbte_init: start mem, data left heap %d\r\n", osif_mem_peek(0));
    ret = osif_msg_queue_create(&up_stack_message_handler, 0x200, sizeof(uint8_t));  // temporary solution for preventing crash by expanding queue size from 0x20 to 0x200
    if (!ret)
    {
        (void)printf("%s, %d:ret:%x\r\n", __func__, __LINE__, ret);
    }
    ret = osif_task_create(&up_stack_task_handler, "upstk", up_stack_task, up_handler, 0x200, 3);
    if (!ret)
    {
        (void)printf("%s, %d:ret:%x\r\n", __func__, __LINE__, ret);
    }
    ret = hci_if_open((P_HCI_IF_CALLBACK)hci_callback);
    if (!ret)
    {
        (void)printf("%s, %d:ret:%x\r\n", __func__, __LINE__, ret);
    }
    return true;
}
