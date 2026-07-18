/***************************************************************************
 *
 *            Copyright (c) 2018 by HCC Embedded
 *
 * This software is copyrighted by and is the sole property of
 * HCC.  All rights, title, ownership, or other interests
 * in the software remain the property of HCC.  This
 * software may only be used in accordance with the corresponding
 * license agreement.  Any unauthorized use, duplication, transmission,
 * distribution, or disclosure of this software is expressly forbidden.
 *
 * This Copyright notice may not be removed or modified without prior
 * written consent of HCC.
 *
 * HCC reserves the right to modify this software without notice.
 *
 * HCC Embedded
 * Budapest 1133
 * Vaci ut 76
 * Hungary
 *
 * Tel:  +36 (1) 450 1302
 * Fax:  +36 (1) 450 1303
 * http: www.hcc-embedded.com
 * email: info@hcc-embedded.com
 *
 ***************************************************************************/
#ifndef _API_USBH_CCID_H
#define _API_USBH_CCID_H

#include "../psp/include/psp_types.h"
#include "api_usb_host.h"

#include "../version/ver_usbh_ccid.h"
#if VER_USBH_CCID_MAJOR != 1 || VER_USBH_CCID_MINOR != 6
 #error Incompatible USBH_CCID version number!
#endif
#include "../version/ver_usb_host.h"
#if VER_USB_HOST_MAJOR != 3
 #error Incompatible USB_HOST version number!
#endif


#ifdef __cplusplus
extern "C" {
#endif

/***** notifications *****/
#define USBH_NTF_CCID_RX         ( USBH_NTF_CD_BASE + 0 )
#define USBH_NTF_CCID_INT        ( USBH_NTF_CD_BASE + 2 )
#define USBH_NTF_CCID_SLOT_ACT   ( USBH_NTF_CD_BASE + 3 )
#define USBH_NTF_CCID_SLOT_INACT ( USBH_NTF_CD_BASE + 4 )


typedef enum
{
  AUTOVOLTAGE = 0
  , VOLT5_0
  , VOLT3_0
  , VOLT1_8
}
t_ccid_powerselect;


typedef struct
{
  uint8_t  uid;
  uint8_t  slot;
  uint8_t  seq;
}
t_ccid_commonparams;


typedef struct
{
  uint8_t  bmFindexDindex;
  uint8_t  bmTCCKST01;
  uint8_t  bGuardTimeT01;
  uint8_t  bmWaitingIntegersT01;
  uint8_t  bClockStop;
  uint8_t  bIFSC;
  uint8_t  bNadValue;
}
t_ccid_abprotocol_datastructure;


typedef struct
{
  uint8_t   bMessageType;
  uint32_t  dwLength;
  uint8_t   bSlot;
  uint8_t   bSeq;
  uint8_t   bStatus;
  uint8_t   bError;
  uint8_t   bChainParameter;
}
t_ccid_rdrpc_datablock;


typedef struct
{
  uint8_t   bMessageType;
  uint32_t  dwLength;
  uint8_t   bSlot;
  uint8_t   bSeq;
  uint8_t   bStatus;
  uint8_t   bError;
  uint8_t   bClockStatus;
}
t_ccid_rdrpc_slotstatus;


typedef struct
{
  uint8_t                          bMessageType;
  uint32_t                         dwLength;
  uint8_t                          bSlot;
  uint8_t                          bSeq;
  uint8_t                          bStatus;
  uint8_t                          bError;
  uint8_t                          bProtocolNum;
  t_ccid_abprotocol_datastructure  abProtocolDataStructure;
}
t_ccid_rdrpc_parameters;


typedef struct
{
  uint8_t   bMessageType;
  uint32_t  dwLength;
  uint8_t   bSlot;
  uint8_t   bSeq;
  uint8_t   bStatus;
  uint8_t   bError;
  uint8_t   bRFU;
}
t_ccid_rdrpc_escape;


typedef struct
{
  uint8_t   bMessageType;
  uint32_t  dwLength;
  uint8_t   bSlot;
  uint8_t   bSeq;
  uint8_t   bStatus;
  uint8_t   bError;
  uint8_t   bRFU;
  uint32_t  dwClockFrequency;
  uint32_t  dwDataRate;
}
t_ccid_rdrpc_datarate_and_clockfrequency;


int usbh_ccid_register_ntf ( t_usbh_unit_id uid, t_usbh_ntf ntf, t_usbh_ntf_fn ntf_fn );
int usbh_ccid_present ( t_usbh_unit_id uid );

int usbh_ccid_init ( void );
int usbh_ccid_start ( void );
int usbh_ccid_stop ( void );
int usbh_ccid_delete ( void );

int usbh_ccid_pcrdr_iccpoweron ( t_ccid_commonparams cparams,
                                t_ccid_powerselect powerselect,
                                t_ccid_rdrpc_datablock * datablock,
                                uint8_t * abdata,
                                uint32_t abdata_len );
int usbh_ccid_pcrdr_iccpoweroff ( t_ccid_commonparams cparams, t_ccid_rdrpc_slotstatus * slotstatus );
int usbh_ccid_pcrdr_getslotstatus ( t_ccid_commonparams cparams, t_ccid_rdrpc_slotstatus * slotstatus );
int usbh_ccid_pcrdr_xfrblock ( t_ccid_commonparams cparams,
                              const uint8_t * abdata_send,
                              uint32_t len_abdata_send,
                              t_ccid_rdrpc_datablock * datablock,
                              uint8_t * abdata_recv,
                              uint32_t len_abdata_recv );
int usbh_ccid_pcrdr_getparameters ( t_ccid_commonparams cparams, t_ccid_rdrpc_parameters * parameters );
int usbh_ccid_pcrdr_resetparameters ( t_ccid_commonparams cparams, t_ccid_rdrpc_parameters * parameters );
int usbh_ccid_pcrdr_setparameters ( t_ccid_commonparams cparams, uint8_t protocolnum, t_ccid_rdrpc_parameters * parameters );
int usbh_ccid_pcrdr_iccclock ( t_ccid_commonparams cparams, uint8_t bClockCommand, t_ccid_rdrpc_slotstatus * slotstatus );
int usbh_ccid_pcrdr_t0apdu ( t_ccid_commonparams cparams,
                            uint8_t bmChanges,
                            uint8_t bClassGetResponse,
                            uint8_t bClassEnvelope,
                            t_ccid_rdrpc_slotstatus * slotstatus );
int usbh_ccid_pcrdr_abort ( t_ccid_commonparams cparams, t_ccid_rdrpc_slotstatus * slotstatus );


#ifdef __cplusplus
}
#endif


#endif /* ifndef _API_USBH_CCID_H */


