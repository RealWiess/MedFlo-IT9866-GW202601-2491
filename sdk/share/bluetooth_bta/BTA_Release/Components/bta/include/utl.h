/*****************************************************************************
**
**  Name:           utl.h
**
**  Description:    Basic utility functions.
**
**  Broadcom Proprietary and Confidential. (C) 2017 Broadcom. All rights reserved.
**  Copyright (c) 2003-2004, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef UTL_H
#define UTL_H

#include "data_types.h"

/*****************************************************************************
**  Constants
*****************************************************************************/
/*** class of device settings ***/
#define BTA_UTL_SET_COD_MAJOR_MINOR     0x01
#define BTA_UTL_SET_COD_SERVICE_CLASS   0x02 /* only set the bits in the input */
#define BTA_UTL_CLR_COD_SERVICE_CLASS   0x04
#define BTA_UTL_SET_COD_ALL             0x08 /* take service class as the input (may clear some set bits!!) */
#define BTA_UTL_INIT_COD                0x0a

/*****************************************************************************
**  Type Definitions
*****************************************************************************/

/** for utl_set_device_class() **/
typedef struct
{
    UINT8       minor;
    UINT8       major;
    UINT16      service;
} tBTA_UTL_COD;


#ifdef __cplusplus
extern "C"
{
#endif

/*****************************************************************************
**  External Function Declarations
*****************************************************************************/

/*******************************************************************************
**
** Function         utl_set_device_class
**
** Description      This function updates the local Device Class.
**
** Parameters:
**                  p_cod   - Pointer to the device class to set to
**
**                  cmd     - the fields of the device class to update.
**                            BTA_UTL_SET_COD_MAJOR_MINOR, - overwrite major, minor class
**                            BTA_UTL_SET_COD_SERVICE_CLASS - set the bits in the input
**                            BTA_UTL_CLR_COD_SERVICE_CLASS - clear the bits in the input
**                            BTA_UTL_SET_COD_ALL - overwrite major, minor, set the bits in service class
**                            BTA_UTL_INIT_COD - overwrite major, minor, and service class
**
** Returns          TRUE if successful, Otherwise FALSE
**
*******************************************************************************/
extern BOOLEAN utl_set_device_class(tBTA_UTL_COD *p_cod, UINT8 cmd);

/*******************************************************************************
**
** Function         utl_isintstr
**
** Description      This utility function checks if the given string is an
**                  integer string or not
**
**
** Returns          TRUE if successful, Otherwise FALSE
**
*******************************************************************************/
extern BOOLEAN utl_isintstr(const char *p_s);

/*******************************************************************************
**
** Function         utl_isdialstr
**
** Description      This utility function checks if the given string contains
**                  only dial digits or not
**
**
** Returns          TRUE if successful, Otherwise FALSE
**
*******************************************************************************/
extern BOOLEAN utl_isdialstr(const char *p_s);

#ifdef __cplusplus
}
#endif

#endif /* UTL_H */
