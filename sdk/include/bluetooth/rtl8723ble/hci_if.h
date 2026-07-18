/**
 * Copyright (c) 2015, Realsil Semiconductor Corporation. All rights reserved.
 */

#ifndef _HCI_IF_H_
#define _HCI_IF_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    HCI_IF_EVT_OPENED,     /* hci I/F open, status, true for success, false for failed */
    HCI_IF_EVT_CLOSED,     /* hci I/F close, status, true for success, false for failed */
    HCI_IF_EVT_DATA_IND,   /* hci I/F rx data indicated, *p_buf and len is available */
    HCI_IF_EVT_DATA_XMIT,  /* hci I/F tx data transmitted, true for success, false for failed  */
    HCI_IF_EVT_ERROR,      /* hci I/F error occurred */
} T_HCI_IF_EVT;

/**
 * @brief :call back function to pass messages and data
 * @param[out] T_HCI_IF_EVT: event type see T_HCI_IF_EVT
 * @param[out] status: related to T_HCI_IF_EVT
 * @param[out] p_buf: pointer if evt is HCI_IF_EVT_DATA_IND, for received data
 * @param[out] len: length of data if evt is HCI_IF_EVT_DATA_IND, for received data
 * @return: true for upper stack successfully received 
 *          false for upper stack failed to received 
 */
typedef bool (*P_HCI_IF_CALLBACK)(T_HCI_IF_EVT evt, bool status, uint8_t *p_buf, uint32_t len);

/**
 * @brief :start BT controller
 * @param[in] P_HCI_IF_CALLBACK:  callback data to get hci message and bt data
 * @return: true for start initialize
 *          false failed to start initialize
 */
bool hci_if_open(P_HCI_IF_CALLBACK p_callback);

/**
 * @brief :close BT controller
 * @param :none
 * @return: true for start close
 *          false failed to start close
 */
bool hci_if_close(void);

/**
 * @brief :upperstack write data to HCI stack, data format must match 
 *		   BT core spec v5.0 vol2 part E section 5 HCI data format
 * @param[in] *p_buf: data buffer pointer to transmit
 * @param[in] len: all data length, including header
 * @return: true for start transmit
 *          false failed to start transmit
 */
bool hci_if_write(uint8_t *p_buf, uint32_t len);

/**
 * @brief :upperstack inform HCI to free memory after use, this is used 
 *			for reducing memory copy
 * @param[in] *p_buf: address provided by P_HCI_IF_CALLBACK evt HCI_IF_EVT_DATA_IND
 * @return: true for success
 *          false for failed
 */
bool hci_if_confirm(uint8_t *p_buf);

/**
 * @example :
bool hci_if_callback(T_HCI_IF_EVT evt, bool status, uint8_t *p_buf, uint32_t len)
{
	switch(evt)
	{
		case HCI_IF_EVT_OPENED:     //
			//parse status, true for success, false for failed 
			break;
		case HCI_IF_EVT_CLOSED:     
			//parse status, true for success, false for failed 
			break;
		case HCI_IF_EVT_DATA_IND:   
			//  handle_data(*p_buf, len );
			hci_if_confirm(p_buf);// note: you can keep this buffer until finishing using
			break;
		case HCI_IF_EVT_DATA_XMIT:  
			// hci I/F tx data transmitted, true for success, false for failed  
			break;
		case HCI_IF_EVT_ERROR:      // hci I/F error occurred 
			break;
	}
	
}
void example_task()
{
	hci_if_open(hci_if_callback);
}
 */

#ifdef __cplusplus
}
#endif

#endif /* _HCI_IF_H_ */
