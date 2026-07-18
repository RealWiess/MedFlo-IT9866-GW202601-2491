/****************************************************************************
*
*            Copyright (c) 2008-2016 by HCC Embedded
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
* Vaci ut 76.
* Hungary
*
* Tel:  +36 (1) 450 1302
* Fax:  +36 (1) 450 1303
* http: www.hcc-embedded.com
* email: info@hcc-embedded.com
*
***************************************************************************/

#ifndef _CONFIG_USBD_CONFIG_H_
#define _CONFIG_USBD_CONFIG_H_

/* HCC Embedded generated source */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Macro definitions */
#ifdef CFG_USBD_CD_AUDIO
 #define SUM_PACKET_SIZE_OUT ( 192 +0u )
 #define SUM_PACKET_SIZE_IN  (192 +0u )
#endif

/* Type definitions */
typedef struct 
{
  const uint8_t * device_descriptor;
  const uint8_t * string_descriptor;
  int number_of_languages;
  int number_of_strings;
  const uint8_t * const * const * const strings;
  int number_of_configurations;
  const uint8_t * const * const configurations_fsls;
  const uint8_t * const * const configurations_hs;
  const uint8_t * const dev_qualify_fsls;
  const uint8_t * const dev_qualify_hs;
} usbd_config_t;
 
#ifdef CFG_USBD_CD_AUDIO
typedef struct {
  uint8_t  ep_addr;
  uint8_t  ifc_id;
  uint16_t pkt_size;
} usbd_audio_map_t;
#endif

/* Global definitions: */
#ifdef CFG_USBD_CD_MST_PRINTER
extern const usbd_config_t device_cfg_mst_printer;

#elif defined(CFG_USBD_CD_CDCACM_RAW) // Irene Lin
extern const usbd_config_t device_cfg_cdc_acm_raw;

#elif defined(CFG_USBD_CD_MST_HID)
extern const usbd_config_t device_cfg_mst_hid;

#elif defined(CFG_USBD_CD_MST_HID_AUDIO)
extern const usbd_config_t device_cfg_mst_hid_audio;

extern const usbd_audio_map_t audio_map_out[1];
extern const usbd_audio_map_t audio_map_in[1];

#elif defined(CFG_USBD_CD_HID)
extern const usbd_config_t device_cfg_hid_gen;

#elif defined(CFG_USBD_CD_BULK_RAW)
extern const usbd_config_t device_cfg_bulk;

#elif defined(CFG_USBD_CD_PRINTER)
extern const usbd_config_t device_cfg_printer;

#elif defined(CFG_USBD_CD_CDCACM)
extern const usbd_config_t device_cfg_cdc_acm;

#elif defined(CFG_USBD_CD_CDCECM)
extern const usbd_config_t device_cfg_cdc_ecm;

#elif defined(CFG_USBD_CD_CDCNCM)
extern const usbd_config_t device_cfg_cdc_ncm;

#elif defined(CFG_USBD_CD_MST)
extern const usbd_config_t device_cfg_mst;

#elif defined(CFG_USBD_CD_AUDIO)
extern const usbd_config_t device_cfg_spk_mic;

extern const usbd_audio_map_t audio_map_out[1];
extern const usbd_audio_map_t audio_map_in[1];

#endif /* defined(CFG_USBD_CD_BULK_RAW) */

/* public array definitions: */
extern const unsigned char hid_report_descriptor[];

extern const uint8_t * hid_descriptor[1];

/* HID-specific definitions: */
#define HID_INTERFACE_COUNT    1
#define HID_DESCRIPTOR_LENGTH  9
#define HID_PHYSICAL_ADDR      NULL  
#define HID_PHYSICAL_LENGTH    0  

extern const uint8_t * report_descriptor[1];
extern const uint16_t  report_descriptor_length[1];
      

#ifdef __cplusplus
}
#endif

#endif /* _CONFIG_USBD_CONFIG_H_ */

/****************************** END OF FILE **********************************/

