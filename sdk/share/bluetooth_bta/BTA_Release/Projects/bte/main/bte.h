/****************************************************************************
**
**  Name:          bte.h
**
**  Description:   this file contains bte defintions
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTE_H
#define BTE_H

#include "bt_target.h"

/* Target Modes (based on jumper settings on hardware [see user manual]) */
enum
{
                            /* BTE                  BBY                     */
                            /* J3   J4              SW3-3   SW3-2   SW3-1   */
                            /* -------------------------------------------- */
    BTE_MODE_SERIAL_APP,    /* OUT  OUT             OFF     OFF     OFF     Sample serial port application      */
    BTE_MODE_APPL,    	    /* IN   OUT             OFF     OFF     ON      Target used with Tester through RPC */
    BTE_MODE_RESERVED,      /* OUT  IN              OFF     ON      OFF     Reserved                            */
    BTE_MODE_SAMPLE_APPS,   /* IN   IN              OFF     ON      ON      Sample applications (HSP)           */
    BTE_MODE_DONGLE,        /* not yet supported    ON      OFF     OFF     Dongle mode                         */
    BTE_MODE_INVALID
};

typedef void (*boot_cplt_cb_t)(void);

extern volatile UINT8    bte_target_mode;    /* indicates the mode that the board is running in */

/* Startup options */
extern UINT32 bte_startup_options;                      /* Switch and jumper settings at startup */
void bte_get_startup_options(UINT32 *p_options);        /* Platform specific function for getting startup options */

#define BTE_OPTIONS_TARGET_MODE_MASK    0x00000007      /* bits 2-0 indicate target mode (QuickConnect: jp3 & jp4, BBY: SW3-1 & SW3-2)*/

typedef void (*bte_ver_printf)(char* fmt, ...);

/* Pointer to function for sending HCI commands and data to the HCI tranport */
extern int (*p_bte_hci_send)(UINT16 port, BT_HDR *p_msg);


/* Protocol trace mask */
extern UINT32 bte_proto_trace_mask;

/* Version string */
extern UINT8* p_bte_ver_str;
extern UINT8 bte_version_string[];

extern const char brcm_patch_version_bt[];
extern const int brcm_patch_ram_length_bt;
extern const char brcm_patch_version[];
extern const int brcm_patch_ram_length;

extern void bte_version_build(void);

extern BT_API int BootEntry( boot_cplt_cb_t cb );
extern void brcm_bt_enable(void);

extern void brcm_bt_disable(void);

extern void BTE_EnableLog(UINT8 mask);
extern void BTE_DisableLog(UINT8 mask);
//extern void BTE_DeleteLogModule(void);

extern void bte_hci_lpm_set(UINT8 mode);
extern UINT8 bte_hci_lpm_get(void);
extern void bte_hci_uart_baudrate_set(UINT8 baud);
extern UINT8 bte_hci_uart_baudrate_get(void);

extern void bte_version_print(bte_ver_printf pfunc);

#endif  /* BTE_H */
