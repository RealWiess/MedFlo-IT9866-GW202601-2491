#ifndef __BD_IAP2_API_H__
#define __BD_IAP2_API_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*tBTAPP_JV_parsedataCB_t) (uint8_t* pbuf, uint16_t len);

/**
 *
 * Init iap2.
 */
void bd_iap2_init(void);

/**
 *
 * iap2 send data.
 */
int bta_co_rfc_senddata(uint8_t app_id, uint8_t *buf, uint16_t size);

/**
 *
 * iap2 check connect.
 */
bool bta_iap2_is_link(uint8_t app_id);

/**
 *
 * iap2 set data receive callback.
 */
void bta_iap2_set_parseCB(uint8_t app_id, tBTAPP_JV_parsedataCB_t parseDataCB);


#ifdef __cplusplus
}
#endif


#endif /* __BD_IAP2_API_H__ */
