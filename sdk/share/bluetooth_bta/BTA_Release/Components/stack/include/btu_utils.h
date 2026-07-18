/*****************************************************************************
**
**  Name:          btu_utils.h
**
**  Description:   This file contains utility functions that can be used by
**                 the core stack, as well as the layers above it.
**
**  Broadcom Proprietary and Confidential. (C) 2017 Broadcom. All rights reserved.
******************************************************************************/
#ifndef BTU_UTILS_H
#define BTU_UTILS_H

#include "data_types.h"

/*****************************************************************************
**  Constants
*****************************************************************************/

/*****************************************************************************
**  Type Definitions
*****************************************************************************/


#ifdef __cplusplus
extern "C"
{
#endif

/*****************************************************************************
**  External Function Declarations
*****************************************************************************/

/*******************************************************************************
**
** Function         utl_str2int
**
** Description      This utility function converts a character string to an
**                  integer.  Acceptable values in string are 0-9.  If invalid
**                  string or string value too large, -1 is returned.
**
**
** Returns          Integer value or -1 on error.
**
*******************************************************************************/
BTU_API extern INT16 utl_str2int(const char *p_s);

/*******************************************************************************
**
** Function         utl_strucmp
**
** Description      This utility function compares two strings in uppercase.
**                  String p_s must be uppercase.  String p_t is converted to
**                  uppercase if lowercase.  If p_s ends first, the substring
**                  match is counted as a match.
**
**
** Returns          0 if strings match, nonzero otherwise.
**
*******************************************************************************/
BTU_API extern int utl_strucmp(const char *p_s, const char *p_t);

/*******************************************************************************
**
** Function         utl_itoa
**
** Description      This utility function converts a UINT16 to a string.  The
**                  string is NULL-terminated.  The length of the string is
**                  returned.
**
**
** Returns          Length of string.
**
*******************************************************************************/
BTU_API extern UINT8 utl_itoa(UINT16 i, char *p_s);

/*******************************************************************************
**
** Function         utl_freebuf
**
** Description      This function calls GKI_freebuf to free the buffer passed
**                  in, if buffer pointer is not NULL, and also initializes
**                  buffer pointer to NULL.
**
**
** Returns          Nothing.
**
*******************************************************************************/
BTU_API extern void utl_freebuf(void **p);

/********************************************************************************
**
**  Name:           utl_bd_addr_to_string
**
**                  Generates a string representation of a bd address
**
**  Parameters:     bd_addr: bd address.
**
**  Returns:        A string representation of a bd address.
**
*********************************************************************************/
BTU_API extern UINT8 *utl_bd_addr_to_string (BD_ADDR bd_addr);

#ifdef __cplusplus
}
#endif

#endif /* UTL_H */
