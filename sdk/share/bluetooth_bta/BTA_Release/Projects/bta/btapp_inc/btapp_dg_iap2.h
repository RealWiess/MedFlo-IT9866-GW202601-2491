/*****************************************************************************
**
**  Name:             btapp_dg_iap2.h
**
**  Description:     This file contains btapp dg_iap2
**                   definition
**
******************************************************************************/

#ifndef BTAPP_DG_IAP2_H
#define BTAPP_DG_IAP2_H

typedef void (*btapp_dg_parsedataCB_t) (uint8_t* pbuf, uint16_t len);

bool btapp_dg_spp_is_link(uint8_t app_id);
void btapp_dg_spp_set_parseCB(uint8_t app_id, btapp_dg_parsedataCB_t parseDataCB);

#endif
