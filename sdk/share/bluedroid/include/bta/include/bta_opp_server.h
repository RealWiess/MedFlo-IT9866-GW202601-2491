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
#define OPP_SERVER_RECV_PUSH_OBJ_INFO   1
#define OPP_SERVER_RECV_PUSH_OBJ_DONE   2
#define OPP_SERVER_PUSH_OBJ_ABORT       3
#define OPP_SERVER_PUSH_OBJ_DISCONNECT  4

/*****************************************************************************
**  Data types
*****************************************************************************/

typedef struct {
    uint8_t* pName;
    uint32_t nameSize;
    uint8_t* pData;
    uint32_t dataSize;
}
tBTA_OPP_SERVER_PUSH_OBJ;

typedef void (*pfPushObjCbFunc)(int event);

/*****************************************************************************
**  Global data
*****************************************************************************/

/*****************************************************************************
**  Function prototypes
*****************************************************************************/

/* main functions */
void bta_opp_server_init(void);
void bta_opp_server_register_cb(pfPushObjCbFunc pfEventNotifyFunc);
void bta_opp_server_terminate();
void bta_opp_server_recv_file(void);
void bta_opp_server_reject_file(void);
void bta_opp_server_get_download_status(uint32_t* pCurPos, uint32_t* pDataSize);
void bta_opp_server_get_push_obj(tBTA_OPP_SERVER_PUSH_OBJ** pptPushObj);
void bta_opp_server_destroy_push_obj(tBTA_OPP_SERVER_PUSH_OBJ* ptPushObj);
void bta_opp_server_get_obj_info(uint8_t* pNameBuffer, uint32_t nameBufferSize, uint32_t* pRetNameSize, uint32_t* pTotalDataSize);


