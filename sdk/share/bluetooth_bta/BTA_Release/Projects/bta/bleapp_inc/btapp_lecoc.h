/*****************************************************************************
**
**  Name:             btapp_lecoc.h
**
**  Description:     This file contains btapp lecoc interface
**				     definition
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_LECOC_H
#define BTAPP_LECOC_H


#include "bta_lecoc_api.h"


typedef struct
{
    UINT32      write_length;
} tBTAPP_LECOC_CB;

extern tBTAPP_LECOC_CB btapp_lecoc_cb;

#endif /* BTAPP_LECOC_H */
