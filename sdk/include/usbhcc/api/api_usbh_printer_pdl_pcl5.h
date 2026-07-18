/***************************************************************************
 *
 *            Copyright (c) 2010-2018 by HCC Embedded
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
#ifndef _API_USBH_PRINTER_PDL_PCL5_H
#define _API_USBH_PRINTER_PDL_PCL5_H

#include "../psp/include/psp_types.h"

#include "../version/ver_usbh_printer_pdl_pcl5.h"
#if ( VER_USBH_PRINTER_PDL_PCL5_MAJOR != 1 ) || ( VER_USBH_PRINTER_PDL_PCL5_MINOR != 2 )
 #error Incompatible USBH_PRINTER_PDL_PCL5 version number!
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PCL5_SUCCESS      0
#define PCL5_ERR_BUF_FULL 1
#define PCL5_ERR_PARAM    2

typedef struct
{
  char    * start;
  uint32_t  index;
  uint32_t  length;
} pcl5_buffer_t;

typedef enum
{
  chrs_normal  = 0
  , chrs_ita     = 1
  , chrs_cond    = 4
  , chrs_condita = 5
  , chrs_compr   = 8
  , chrs_expand  = 24
  , chrs_outline = 32
  , chrs_inline  = 64
  , chrs_sha     = 128
  , chrs_outsha  = 160
} chr_style_t;

typedef enum
{
  lpi_1 = 1
  , lpi_2 = 2
  , lpi_3 = 3
  , lpi_4 = 4
  , lpi_6 = 6
  , lpi_8 = 7
  , lpi_12 = 12
  , lpi_16 = 16
  , lpi_24 = 24
  , lpi_48 = 48
} line_spacing_t;

int pcl5_init ( void );
int pcl5_start_job ( pcl5_buffer_t * buf );
int pcl5_set_col ( pcl5_buffer_t * buf, int col );
int pcl5_set_row ( pcl5_buffer_t * buf, int row );
int pcl5_set_l_margin ( pcl5_buffer_t * buf, int col );
int pcl5_set_r_margin ( pcl5_buffer_t * buf, int col );
int pcl5_set_t_margin ( pcl5_buffer_t * buf, int row );

int pcl5_fixed_spacing ( pcl5_buffer_t * buf );
int pcl5_prportional_spacing ( pcl5_buffer_t * buf );

int pcl5_set_fixed_pitch ( pcl5_buffer_t * buf, int pitch );
int pcl5_set_proportional_height ( pcl5_buffer_t * buf, int height );

int pcl5_set_line_spacing ( pcl5_buffer_t * buf, line_spacing_t lpi );

int pcl5_set_style ( pcl5_buffer_t * buf, chr_style_t style );

int pcl5_underline_off ( pcl5_buffer_t * buf );
int pcl5_underline_on ( pcl5_buffer_t * buf );

int pcl5_set_weight ( pcl5_buffer_t * buf, int weight );

int pcl5_select_symbol_set ( pcl5_buffer_t * buf, char * id );

int pcl5_end_job ( pcl5_buffer_t * buf );
int pcl5_eject_page ( pcl5_buffer_t * buf );
int pcl5_print_text ( pcl5_buffer_t * buf, char * text );
int pcl5_print_transparent ( pcl5_buffer_t * buf, char * text, int length );

int pcl5_img_start ( pcl5_buffer_t * buf, uint16_t width, uint16_t height, uint16_t resolution );
int pcl5_img_rgbdata ( pcl5_buffer_t * buf, char * src, uint16_t len );
int pcl5_img_finish ( pcl5_buffer_t * buf );


#ifdef __cplusplus
}
#endif

#endif /* ifndef _API_USBH_PRINTER_PDL_PCL5_H */

