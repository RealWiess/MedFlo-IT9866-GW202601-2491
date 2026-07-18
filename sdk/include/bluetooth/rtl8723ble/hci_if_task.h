/**
  ******************************************************************************
  * File           : hci_if_task.h
  * Author         : Harvey_Guo
  * Date           : 2019/11/12
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef HCI_IF_TASK_H
#define HCI_IF_TASK_H
/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "hci_if.h"
#include "hci_protocol.h"
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
//HCI_IF_TASK_MESSAGE
#define HCI_IF_MSG_OPEN                 0x01
#define HCI_IF_MSG_READY                0x02
#define HCI_IF_MSG_FAIL                 0x03
#define HCI_IF_MSG_CLOSE                0x04

#define HCI_IF_MSG_TX_PACK_REQ          0x05
#define HCI_IF_MSG_TX_UART_TRANSMIT     0x06
#define HCI_IF_MSG_TX_INFORM_UPPERSTACK 0x07

#define HCI_IF_MSG_RX_IND_UNPACK        0x08
#define HCI_IF_MSG_RX_INFORM_UPPERSTACK 0x09
#define HCI_IF_MSG_RX_CFM               0x0a


//HCI_IF_TASK_STATE
#define    HCI_IF_STATE_UNINITIALIZED               0
#define    HCI_IF_STATE_SYNC                        1
#define    HCI_IF_STATE_CONFIG                      2
#define    HCI_IF_STATE_PATCH                       3
#define    HCI_IF_STATE_READY                       4
#define    HCI_IF_STATE_CLOSING                     5

#define	VERSION_BUILD_STR	"\0"
#define	BUILDING_TIME		"\0"
#define	LIB_NAME			"\0"
#define	VERSION_GCID		1
#define	VERSION_GCIDH		1

/* Exported variables ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void hci_if_send_message_to_ifctrl(uint8_t message);
bool hci_if_send_message_to_upperstack(T_HCI_IF_EVT evt, bool status, uint8_t *p_buf, uint32_t len);
void hci_error_indicate(int level);
bool hci_if_check_opened(void);
void hci_if_deinit(void);
void rtkbt_disable(void);
#endif /* HCI_IF_TASK_H */
/********************************************************* *****END OF FILE****/
