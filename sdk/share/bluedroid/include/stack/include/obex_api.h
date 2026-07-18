/******************************************************************************
 *
 *  Copyright (C) 2002-2012 Broadcom Corporation
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

/******************************************************************************
 *
 *  OBEX API header file
 *
 ******************************************************************************/
#ifndef OBEX_API_H
#define OBEX_API_H

/*****************************************************************************
**  Constants
*****************************************************************************/
#define OBEX_SUCCESS 					0
#define OBEX_ERROR  					1

#define OBEX_HEADER_INSERT_IS_FAILED 	OBEX_ERROR
#define OBEX_CONNECTION_IS_FAILED 		OBEX_ERROR
#define OBEX_DISCONNECTION_IS_FAILED 	OBEX_ERROR

#define OBEX_SEND_GET_IS_FAILED       	1
#define OBEX_GET_IS_FAIELD              2
#define OBEX_GET_HAS_MORE_DATA          3
#define OBEX_GET_IS_DONE                4

#define OBEX_SEND_PUT_IS_FAILED        	1
#define OBEX_PUT_IS_FAIELD              2
#define OBEX_PUT_IS_DONE                3


#define OBEX_CONTINUE					0x90
#define OBEX_OK							0xA0
#define OBEX_REQUEST_TIMEOUT			0xC8
#define OBEX_NOT_IMPLEMENTED			0XD1
#define OBEX_SERVICE_UNAVAILABLE		0XD2

/*****************************************************************************
**  Type definitions
*****************************************************************************/
typedef void* OBEX_INSTANCE;

typedef int (*OBEX_ReadData)(uint8_t* pBuffer, int bufferSize);
typedef int (*OBEX_WriteData)(uint8_t* pBuffer, int bufferSize);

typedef struct {
    uint8_t*        pName;
    uint32_t        nameSize;
    uint8_t*        pData;
    uint32_t        totalDataSize;
    uint32_t        currentDataIndex;
    uint32_t        remainBodySize;
} OBEX_PUT_OBJ;

typedef struct {
    uint8_t*        pData;
    uint32_t        totalDataSize;
    uint32_t        currentDataIndex;
    uint32_t        remainBodySize;
} OBEX_GET_OBJ;


//OBEX Header Identifier
#define OBEX_HEADER_COUNT                   0xC0
#define OBEX_HEADER_NAME 					0x01
#define OBEX_HEADER_TYPE 					0x42
#define OBEX_HEADER_LENGTH 					0xC3
#define OBEX_HEADER_TIME					0x44
#define OBEX_HEADER_DESCRIPTION 			0x05
#define OBEX_HEADER_TARGET 					0x46
#define OBEX_HEADER_HTTP 					0x47
#define OBEX_HEADER_BODY 					0x48
#define OBEX_HEADER_END_BODY 				0x49
#define OBEX_HEADER_WHO 					0x4A
#define OBEX_HEADER_CONNECTION_ID 			0xCB
#define OBEX_HEADER_APP_PARAM 				0x4C
#define OBEX_HEADER_AUTH_CHA 				0x4D
#define OBEX_HEADER_AUTH_RES 				0x4E
#define OBEX_HEADER_CREATOR_ID 				0xCF
#define OBEX_HEADER_WAN_UUID 				0x50
#define OBEX_HEADER_OBJ_CLASS 				0x51
#define OBEX_HEADER_SESSION_PARM 			0x52
#define OBEX_HEADER_SESSION_SEQ_NUM 		0x93
#define OBEX_HEADER_IMG_HANDLE              0x30
#define OBEX_HEADER_IMG_DESCRIPTION         0x71


//OBEX Op code
#define OBEX_CONNECT_REQUEST                0x80
#define OBEX_DISCONNECT_REQUEST             0x81

#define OBEX_GET_REQUEST                    0x83

#define OBEX_PUT_REQUEST                    0x02
#define OBEX_PUT_END_REQUEST                0x82
#define OBEX_ABORT                          0xFF

//OBEX Response code
#define OBEX_RESP_CONTINUE                  0x90
#define OBEX_RESP_OK                        0xA0
#define OBEX_RESP_BAD_REQUEST               0xC0
#define OBEX_RESP_NOT_IMPLEMENTED           0xD1
#define OBEX_RESP_SERVICE_NOT_AVAIL         0xD3

//OBEX version Number
#define OBEX_VERSION                        0x10


//PARSE PUT OBJ RESULT
#define PUT_NO_LEN_HEADER                   -1
#define PUT_GOT_NAME_LEN_INFO               -2
#define PUT_INVALID_SIZE                    -3
#define PUT_DATA_PARSING                    0x0
#define PUT_RECV_OBJ_IS_DONE                0x1

//PARSE GET OBJ RESULT
#define GET_NO_LEN_HEADER                   -1
#define GET_DATA_PARSING                    0x0
#define GET_RECV_OBJ_IS_DONE                0x1

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************
** Main Control Block
*******************************************************************************/
OBEX_INSTANCE OBEX_InitInstance(OBEX_ReadData pfReadFunc, OBEX_WriteData pfWriteFunc);
void OBEX_TerminateInstance(OBEX_INSTANCE obexInstance);

int OBEX_SendConnectCmd(OBEX_INSTANCE obexInstance);
int OBEX_RecvConnectCmdResult(OBEX_INSTANCE obexInstance);
int OBEX_SendConnectResp(OBEX_INSTANCE obexInstance);

int OBEX_SendDisconnectCmd(OBEX_INSTANCE obexInstance);
int OBEX_RecvDisconnectCmdResult();

int OBEX_SendGetCmd(OBEX_INSTANCE obexInstance);
int OBEX_RecvGetCmdResult(OBEX_INSTANCE obexInstance, uint8_t* pBuffer, int bufferSize, int* pRetSize, uint8_t* pResponseCode);

int OBEX_SendPutCmd(OBEX_INSTANCE obexInstance);
int OBEX_RecvPutCmdResult(OBEX_INSTANCE obexInstance, uint8_t* pBuffer, int bufferSize, int* pRetSize, uint8_t* pResponseCode);

int OBEX_SendAbortResp(OBEX_INSTANCE obexInstance);
int OBEX_SendAbort(OBEX_INSTANCE obexInstance);
int OBEX_SendContinue(OBEX_INSTANCE obexInstance);
int OBEX_SendRecvEnd(OBEX_INSTANCE obexInstance);
int OBEX_SendDisconnectResp(OBEX_INSTANCE obexInstance);

int OBEX_RecvRawData(OBEX_INSTANCE obexInstance, uint8_t** pBuffer, int bufferSize, int* pRetSize, uint8_t* pRespCode);
int OBEX_ParsePutObj(OBEX_INSTANCE obexInstance, OBEX_PUT_OBJ** pptPutObj, uint8_t* pInputData, uint32_t dataSize);
int OBEX_ParseGetObj(OBEX_INSTANCE obexInstance, OBEX_GET_OBJ** pptGetObj, uint8_t* pInputData, uint32_t dataSize);

void OBEX_ResetHeader(OBEX_INSTANCE obexInstance);
int OBEX_InsertHeader(OBEX_INSTANCE obexInstance, uint8_t headerId, uint8_t* pBuffer, int bufferSize);

int OBEX_NameToUTF16(uint8_t* pOutBuffer, int outSize, uint8_t* pInputBuffer, int inputSize);

#ifdef __cplusplus
}
#endif

#endif /* OBEX_API_H */
