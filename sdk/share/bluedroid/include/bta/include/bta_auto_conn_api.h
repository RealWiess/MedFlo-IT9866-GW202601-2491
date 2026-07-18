/******************************************************************************
 *
 *  Copyright (c) 2014 The Android Open Source Project
 *  Copyright (C) 2003-2012 Broadcom Corporation
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

#include "include/bta/include/bta_sys.h"
#include "include/bta/include/bta_api.h"

/*****************************************************************************
**  Constants
*****************************************************************************/
// The ALWAYS_RETRY rule means that iTE device will try to construct connection
// with stored paired remote device once the system is power on. Even though
// the disconnection is forced by remote device. 
#define ALWAYS_RETRY                   0
// The STOP_IF_REMOTE_SEND_DISCONN rule means that iTE device will try to
// construct connection // with stored paired remote device once the system is
// power on. But once the disconnection is triggered by remote device, the iTE
// will stop auto construct connection with remote device. Even though, the
// system is reboot. 
#define STOP_IF_REMOTE_SEND_DISCONN    1

#define DEFAULT_RETRY_TIME_PERIOD      10


/*****************************************************************************
**  Data types
*****************************************************************************/
typedef struct {
    int rule;
    int retryCount; //0 means retry forever.
    int retryPeriodSec; //retry period in seconds. (0 is default)
} tBTA_AUTO_CONN_RULE;

/*****************************************************************************
**  Global data
*****************************************************************************/

/*****************************************************************************
**  Function prototypes
*****************************************************************************/

/*******************************************************************************
** Function         bta_auto_connect_enable
**
** Description      Start up auto connection feature for classic BT
**
**                  Input Parameters:
**                      ptAutoConnRule: auto connection rule
** Returns          void
*******************************************************************************/
void bta_auto_connect_enable(tBTA_AUTO_CONN_RULE* ptAutoConnRule);

/*******************************************************************************
** Function         bta_auto_connect_disable
**
** Description      Stop auto connection feature for classic BT
**
** Returns          void
*******************************************************************************/
void bta_auto_connect_disable(void);
