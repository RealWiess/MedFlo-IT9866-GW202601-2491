/*****************************************************************************
**
**  Name:           bta_sys_co.c
**
**  Description:    This file contains the HW management callout functions
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
*****************************************************************************/

#include "bta_platform.h"
#include "bte_glue.h"

/*******************************************************************************
**
** Function         bta_sys_hw_co_enable
**
** Description      This function is called by the stack to power up the HW
**
** Returns          void
**
*******************************************************************************/
void bta_sys_hw_co_enable( tBTA_SYS_HW_MODULE module )
{
    /* platform specific implementation to power-up the HW */


    /* if no client/server asynchronous system like linux-based OS, directly call the ci here */
    bta_sys_hw_ci_enabled( module );
}

/*******************************************************************************
**
** Function         bta_sys_hw_co_disable
**
** Description     This function is called by the stack to power down the HW
**
** Returns          void
**
*******************************************************************************/
void bta_sys_hw_co_disable( tBTA_SYS_HW_MODULE module )
{
    /* platform specific implementation to power-down the HW */


    /* if no client/server asynchronous system like linux-based OS, directly call the ci here */
    bta_sys_hw_ci_disabled( module );

}

