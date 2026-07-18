/**
  ******************************************************************************
  * File           : hci_protocal.h
  * Author         : Harvey_Guo
  * Date           : 2019/11/13
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef HCI_PROTOCOL_H
#define HCI_PROTOCOL_H
/* Includes ------------------------------------------------------------------*/
#include "stdbool.h"
#include "stdint.h"
/* Exported macro ------------------------------------------------------------*/
#define USE_HCI_H5
#if defined USE_HCI_H5 && defined USE_HCI_H4
#error "H5 & H4 support at same time, please confirm it!"
#endif
#if !defined USE_HCI_H5 && !defined USE_HCI_H4
#error "None of protocol selected, please confirm it!"
#endif

#define HCI_RX_ACL_PKT_BUF_OFFSET 0

/* Exported types ------------------------------------------------------------*/
typedef bool (*HCI_PROTOCOL_CALLBACK)(void *);
typedef struct T_HCI_PROTOCOL
{
    bool (*open)(void);
    bool (*close)(void);
    bool (*pack)(uint8_t *p_buf, uint16_t len);
    bool (*unpack)(uint8_t *p_buf, uint16_t len);
    bool (*get_package_tx)(uint8_t **p_buf, uint16_t *len);
    bool (*get_package_rx)(uint8_t **p_buf, uint16_t *len);
    bool (*tx_package_prune)(uint8_t *p_buf, uint16_t len);
    bool (*rx_package_prune)(uint8_t *p_buf, uint16_t len);
    bool (*tx_finish_callback)(void); //this is for supporting both h4 and h5
    bool (*rx_finish_callback)(void); //this is for supporting both h4 and h5
    bool (*connect_response)(void *);
} HCI_PROTOCOL;
/* Exported constants --------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
bool hci_protocol_init(HCI_PROTOCOL **protocol_handle);
bool hci_protocol_deinit(void);
#endif /* HCI_PROTOCOL_H */
/***************************************************************END OF FILE****/
