/*****************************************************************************
**                                                                           *
**  Name:          btem_post_reset.c                                         *
**                                                                           *
**  Description:   Post controller reset routines                            *
**                                                                           *
**  Copyright (c) 2018-2019, Broadcom Inc.                                   *
******************************************************************************/
#include "bta_platform.h"
#include "bte_glue.h"
#include "hcilp.h"


/******************************************************
 *                    Constants
 ******************************************************/
#define BTE_MAIN_HCI_RECONFIG_BAUD_4_PATCH_DL          TRUE
#define BTE_MAIN_HCI_RECONFIG_BAUD_RATE_4_PATCH_DL     USERIAL_BAUD_1M
#define BTE_MAIN_HCI_RECONFIG_BAUD                     TRUE //TRUE
#define BTE_MAIN_HCI_RECONFIG_BAUD_RATE                USERIAL_BAUD_1M
#define BTE_MAIN_HCI_RECONFIG_CLOCK_RATE               4800000

static UINT8 bte_hci_lpm = 0;                                              //enable low power mode default.
static UINT8 bte_hci_uart_br = BTE_MAIN_HCI_RECONFIG_BAUD_RATE;            //dafault hci uart baud rate is BTE_MAIN_HCI_RECONFIG_BAUD_RATE
static BD_ADDR bt_controller_default_addr = {0x43, 0x01, 0x30, 0xCC, 0xEE, 0xFF};

extern boot_cplt_cb_t boot_cplt_cb;

void bte_continue_up(UINT8 lpm);

void bte_hci_lpm_set(UINT8 mode)
{
    bte_hci_lpm = mode;
}

UINT8 bte_hci_lpm_get(void)
{
    return bte_hci_lpm;
}

void bte_hci_lpm_parameters_set(UINT8 idle_thre, UINT16 idle_timeout)
{
    HCILP_SetParameters(idle_thre, idle_timeout);
}

void bte_hci_lpm_status_get(void)
{
    UINT8 lpm_enabled = 0;
    UINT8 idle_thres = 0;
    UINT16 idle_timeout = 0;

    lpm_enabled = bte_hci_lpm_get();
    HCILP_GetParameters(&idle_thres, &idle_timeout);

    APPL_TRACE_DEBUG3("BT Stack lpm %s, idle_thres:%d * 12.5 ms, idle_timeout:%d ms", (lpm_enabled == 0) ? "Disabled" : "Enabled", idle_thres, idle_timeout);
}

void bte_hci_uart_baudrate_set(UINT8 baud)
{
    bte_hci_uart_br = baud;
}

UINT8 bte_hci_uart_baudrate_get(void)
{
    return bte_hci_uart_br;
}

#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
static void bte_hci_lp_enable_cplt_cb(UINT8 status)
{
    tUPIO_STATE state;

    if(status == BTA_SUCCESS)
    {
        APPL_TRACE_ERROR0("BT HCILP Enable Success!");

        /*Because the stack treat HCILP state as active state default, if the HCILP mode enable,
        meanwhile if gki_timer task blocked by other task, the sleep timeout not execute,
        the HCILP state will not change to sleep state flag. But in order to the first
        time delay 20ms after DEV_WAKE pull up WAR, we use the HCILP state to judge whether
        DEV_WAKE pull up or not, rather than every HCI command need send to pull up DEV_WAKE every time
        Avoid BT controller sleep during the stack initilization stage, we need pull up the DEV_WAKE after
        HCILP enable complete success callback be invoked */
        state = (btu_cb.hcilp_cb.params.bt_wake_polarity ? UPIO_ON : UPIO_OFF);
        UPIO_Set(UPIO_GENERAL, HCILP_BT_WAKE_GPIO, state);

        bte_continue_up(0);
    }
    else
    {
        APPL_TRACE_ERROR1("BT HCILP Enable Failed... Status:%d", status);
    }
}
#endif

void bte_continue_up(UINT8 lpm)
{
    if(bte_hci_lpm == 1 && lpm == 1)
    {
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
        HCILP_Enable(NULL, bte_hci_lp_enable_cplt_cb);
#endif
    }
    else
    {
        //BTM_SetLocalDeviceAddr(bt_controller_default_addr, NULL); //use User MAC
        BTM_ContinueReset();

        if(boot_cplt_cb != NULL)
        {
            boot_cplt_cb();
        }
    }
}

static void bte_post_download_baud_update_cback(UINT8 status)
{
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
    bte_continue_up(1);
#else
    bte_continue_up(0);
#endif
}

void bte_prm_cback(tBTA_STATUS status)
{
    if (status == BTA_SUCCESS)
    {
        BTE_EnableLog(0x01);

        GKI_delay(300);                //This delay purpose is for the controller to prepare ready after launch ram.

#if (defined(BTE_MAIN_HCI_RECONFIG_BAUD_4_PATCH_DL) && (BTE_MAIN_HCI_RECONFIG_BAUD_4_PATCH_DL == TRUE))
        USERIAL_ChangeBaud(USERIAL_PORT_1, 115200);
#endif

        btsnd_hcic_reset(0);
        GKI_delay(100);

#if (defined(BTE_MAIN_HCI_RECONFIG_BAUD) && (BTE_MAIN_HCI_RECONFIG_BAUD == TRUE))
        if(bte_hci_uart_br == BTE_MAIN_HCI_RECONFIG_BAUD_RATE || \
           bte_hci_uart_br == USERIAL_BAUD_1_5M || \
           bte_hci_uart_br == USERIAL_BAUD_921600)
        {//we only update these three values baud rate, otherwise use the default 115200 baud.
           HCIUTIL_UpdateBaudRate(bte_hci_uart_br,
                                    BTE_MAIN_HCI_RECONFIG_CLOCK_RATE,
                                    bte_post_download_baud_update_cback);
        }
        else
        {
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
            bte_continue_up(1);
#else
            bte_continue_up(0);
#endif
        }
#else
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
        bte_continue_up(1);
#else
        bte_continue_up(0);
#endif
#endif
    }
    else
    {
        APPL_TRACE_ERROR1("bte_prm_cback failed status=0x%x", status);
    }
}

void bte_baud_update_cback(UINT8 status)
{
    APPL_TRACE_DEBUG1("bte_baud_update_cback status=%d", status);

#if (defined(BTE_MAIN_HCI_RECONFIG_BAUD_4_PATCH_DL) && (BTE_MAIN_HCI_RECONFIG_BAUD_4_PATCH_DL == TRUE))
    /* Delay for 50ms to allow controller to set new baud rate */
    GKI_delay(50);
#endif

    BTE_DisableLog(0x01);

    BTA_PatchRam(bte_prm_cback, NULL, 0, 0);
}

void bte_post_reset(void)
{
    APPL_TRACE_DEBUG0("bte_post_reset");

#if (defined(BTE_MAIN_HCI_RECONFIG_BAUD_4_PATCH_DL) && (BTE_MAIN_HCI_RECONFIG_BAUD_4_PATCH_DL == TRUE))
    HCIUTIL_UpdateBaudRate(BTE_MAIN_HCI_RECONFIG_BAUD_RATE_4_PATCH_DL,
            BTE_MAIN_HCI_RECONFIG_CLOCK_RATE,
            bte_baud_update_cback);
#else
    bte_baud_update_cback(HCIUTIL_STATUS_SUCCESS);
#endif
}

