/***************************************************************************
 *
 *            Copyright (c) 2015 by HCC Embedded
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
#ifndef _API_USBH_PRINTER_PDL_ESCPOS_H
#define _API_USBH_PRINTER_PDL_ESCPOS_H

#include "../psp/include/psp_types.h"

#include "../version/ver_usbh_printer_pdl_escpos.h"
#if ( VER_USBH_PRINTER_PDL_ESCPOS_MAJOR != 1 ) || ( VER_USBH_PRINTER_PDL_ESCPOS_MINOR != 1 )
 #error Incompatible USBH_PRINTER_PDL_ESCPOS version number!
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define ESCPOS_SUCCESS      0
#define ESCPOS_ERR_BUF_FULL 1

typedef struct
{
  char    * start;
  uint32_t  index;
  uint32_t  length;
} escpos_buffer_t;

typedef enum
{
  /* Function A */
  UPC_A = 0
  , UPC_E = 1
  , JAN13 = 2
  , JAN8 = 3
  , CODE39 = 4
  , ITF = 5
  , CODABAR = 6,

  /* Function B */
  UPC_A_FUNC_B = 65
  , UPC_E_FUNC_B = 66
  , JAN13_FUNC_B = 67
  , JAN8_FUNC_B = 68
  , CODE39_FUNC_B = 69
  , ITF_FUNC_B = 70
  , CODABAR_FUNC_B = 71
  , CODE93 = 72
  , CODE128 = 73
  , GS1_128 = 74
  , GS1_DATABAR_OMNIDIRECTIONAL = 75
  , GS1_DATABAR_TRUNCATED = 76
  , GS1_DATABAR_LIMITED = 77
  , GS1_DATABAR_EXPANDED = 78
} t_barcode;

int escpos_init ( void );
int escpos_start_job ( escpos_buffer_t * buf );
int escpos_end_job ( escpos_buffer_t * buf, uint8_t feed_lines );

int escpos_set_line_spacing ( escpos_buffer_t * buf, uint8_t line_spacing );
int escpos_default_spacing ( escpos_buffer_t * buf );

int escpos_underline_off ( escpos_buffer_t * buf );
int escpos_underline_on ( escpos_buffer_t * buf );

int escpos_emphasize_off ( escpos_buffer_t * buf );
int escpos_emphasize_on ( escpos_buffer_t * buf );

int escpos_set_char_size ( escpos_buffer_t * buf, uint8_t width, uint8_t height );
int escpos_set_font ( escpos_buffer_t * buf, uint8_t font );

int escpos_print_text ( escpos_buffer_t * buf, char * text );
int escpos_cut_paper ( escpos_buffer_t * buf );
int escpos_set_barcode_width ( escpos_buffer_t * buf, uint8_t width );
int escpos_set_barcode_height ( escpos_buffer_t * buf, uint8_t height );
int escpos_print_barcode ( escpos_buffer_t * buf, t_barcode type, uint8_t * barcode, uint8_t length );

#ifdef __cplusplus
}
#endif

#endif /* ifndef _API_USBH_PRINTER_PDL_ESCPOS_H */

