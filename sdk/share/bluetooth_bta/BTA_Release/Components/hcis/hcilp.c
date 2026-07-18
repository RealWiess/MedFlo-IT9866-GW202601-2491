/*****************************************************************************
**
**  Name:          hcilp.c
**
**  Description:
**      This file implements the HCI sleep mode implementation on the host side
**
**
**  Copyright (c) 2002-2013, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/
#include <string.h>
#include "bt_target.h"
#include "bte.h"
#include "bt_types.h"
#include "gki.h"
#include "hcidefs.h"
#include "brcm_hcidefs.h"
#include "btm_api.h"
#include "upio.h"
#include "userial.h"
#include "hcilp.h"
#include "btu.h"
#if (defined(SLIP_INCLUDED) && SLIP_INCLUDED == TRUE)
#include "slip.h"
#endif
#include "hcisu.h"

#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
static UINT8 idle_thres_user_set = 0;
static UINT16 idle_timeout_user_set = 0;

/*******************************************************************************
 **
 ** Function           hcilp_start_bt_wake_idle_time
 **
 ** Description        start BT_WAKE idle timer
 **
 ** Output Parameter   None
 **
 ** Returns            void
 **
 *******************************************************************************/
static void hcilp_start_bt_wake_idle_time(void)
{
    //HCI_TRACE_DEBUG1("hcilp_start_bt_wake_idle_time %d", (UINT32)btu_cb.hcilp_cb.params.bt_wake_idle_timeout*QUICK_TIMER_TICKS_PER_SEC/1000);
    /* start host controller idle time for 300 or 25 msec unit */
    HCI_START_QUICK_TIMER (&btu_cb.hcilp_cb.lp_timer_list, BTU_TTYPE_LP_HC_IDLE_TO,
        (UINT32)btu_cb.hcilp_cb.params.bt_wake_idle_timeout*QUICK_TIMER_TICKS_PER_SEC/1000);
}

/*******************************************************************************
 **
 ** Function           hcilp_stop_hc_idle_time
 **
 ** Description        stop HC idle timer
 **
 ** Output Parameter   None
 **
 ** Returns            void
 **
 *******************************************************************************/
static void hcilp_stop_hc_idle_time(void)
{
    if (btu_cb.hcilp_cb.lp_timer_list.in_use)
    {
        //HCI_TRACE_DEBUG0("hcilp_stop_hc_idle_time");
        /* stop host controller idle time */
        HCI_STOP_QUICK_TIMER (&btu_cb.hcilp_cb.lp_timer_list);
    }
}

/*******************************************************************************
 **
 ** Function           hcilp_vs_cback
 **
 ** Description        set lpm params Callback function
 **
 ** Output Parameter   None
 **
 ** Returns            void
 **
 *******************************************************************************/
static void hcilp_vs_cback(tBTM_VSC_CMPL *p)
{
    /* if it is not completed */
    if (p->p_param_buf[0] != 0) {
        if (btu_cb.hcilp_cb.p_enable_cback)
        {
            btu_cb.hcilp_cb.p_enable_cback(HCILP_STATUS_FAILURE);
            btu_cb.hcilp_cb.p_enable_cback = NULL;
        }
    }

    btu_cb.hcilp_cb.lp_enabled = btu_cb.hcilp_cb.sent_vsc_lp_enabled;

    if (btu_cb.hcilp_cb.lp_enabled == FALSE)
    {
        hcilp_stop_hc_idle_time();
#if (defined(H4IBSS_INCLUDED) && H4IBSS_INCLUDED == TRUE)
        if( btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_H4IBSS )
        {
            h4ibss_init(0);
            HCI_STOP_QUICK_TIMER (&btu_cb.hcilp_cb.lp_timer_list);
        }
#endif
    }

    HCI_TRACE_DEBUG0("HCIS: hciu_lp - hcilp_vs_cback");

    if (btu_cb.hcilp_cb.p_enable_cback)
    {
        btu_cb.hcilp_cb.p_enable_cback(HCILP_STATUS_SUCCESS);
        btu_cb.hcilp_cb.p_enable_cback = NULL;
    }
}
#endif /*(defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)*/

/*******************************************************************************
 **
 ** Function           HCILP_Init
 **
 ** Description        Init HCILP data.
 **
 ** Output Parameter   None
 **
 ** Returns            void
 **
 *******************************************************************************/
void HCILP_Init(tUSERIAL_PORT port)
{
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
    HCI_TRACE_DEBUG1("HCILP_Init() port = %d", port);

    btu_cb.hcilp_cb.lp_enabled          = FALSE;
    btu_cb.hcilp_cb.lp_no_tx_data       = FALSE;
    btu_cb.hcilp_cb.sent_vsc_lp_enabled = FALSE; /* whether we send vsc lp enable to controller */
    btu_cb.hcilp_cb.p_enable_cback      = NULL;
    btu_cb.hcilp_cb.p_wakeup_cback      = NULL;
    btu_cb.hcilp_cb.state               = HCILP_STATE_ACTIVE;

    btu_cb.hcilp_cb.params.sleep_mode                   = HCILP_SLEEP_MODE;
    if(idle_thres_user_set != 0)
    {
        btu_cb.hcilp_cb.params.host_stack_idle_threshold    = idle_thres_user_set;
        btu_cb.hcilp_cb.params.host_controller_idle_threshold = idle_thres_user_set;
    }
    else
    {
        btu_cb.hcilp_cb.params.host_stack_idle_threshold    = HCILP_IDLE_THRESHOLD;
        btu_cb.hcilp_cb.params.host_controller_idle_threshold = HCILP_HC_IDLE_THRESHOLD;
    }

    btu_cb.hcilp_cb.params.bt_wake_polarity             = HCILP_BT_WAKE_POLARITY;
    btu_cb.hcilp_cb.params.host_wake_polarity           = HCILP_HOST_WAKE_POLARITY;
    btu_cb.hcilp_cb.params.allow_host_sleep_during_sco  = HCILP_ALLOW_HOST_SLEEP_DURING_SCO;
    btu_cb.hcilp_cb.params.combine_sleep_mode_and_lpm   = HCILP_COMBINE_SLEEP_MODE_AND_LPM;
    btu_cb.hcilp_cb.params.enable_uart_txd_tri_state    = HCILP_ENABLE_UART_TXD_TRI_STATE;
    btu_cb.hcilp_cb.params.pulsed_host_wake             = HCILP_PULSED_HOST_WAKE;
    btu_cb.hcilp_cb.params.sleep_guard_time             = HCILP_SLEEP_GUARD_TIME;
    btu_cb.hcilp_cb.params.wakeup_guard_time            = HCILP_WAKEUP_GUARD_TIME;
    btu_cb.hcilp_cb.params.txd_config                   = HCILP_TXD_CONFIG;

    if(idle_timeout_user_set != 0)
    {
        btu_cb.hcilp_cb.params.bt_wake_idle_timeout     = idle_timeout_user_set;
    }
    else
    {
        btu_cb.hcilp_cb.params.bt_wake_idle_timeout     = HCILP_BT_WAKE_IDLE_TIMEOUT;
    }

    GKI_init_timer_list_entry(&btu_cb.hcilp_cb.lp_timer_list);

#if (defined(H4IBSS_INCLUDED) && H4IBSS_INCLUDED == TRUE)
    if( btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_H4IBSS )
    {
        h4ibss_init(port);
    }
#endif
#endif /*(defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)*/
}

/*******************************************************************************
**
** Function           HCILP_Enable
**
** Description        Enables Sleep mode.
**
** Output Parameter   None
**
** Returns            TRUE  if sleep mode command was issued to BT device
**                    FALSE if the feature is not supported
**
*******************************************************************************/
BOOLEAN HCILP_Enable(tHCILP_Params *p_param, tHCILP_ENABLE_CBACK *p_cb)
{
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
    tBTM_STATUS result;
    UINT8 data[HCI_BRCM_WRITE_SLEEP_MODE_LENGTH];
    UINT8 *p = data;

    if( p_param )
    {
        memcpy( &btu_cb.hcilp_cb.params, p_param, sizeof(tHCILP_Params));
    }

    if( btu_cb.hcilp_cb.params.sleep_mode )
        btu_cb.hcilp_cb.sent_vsc_lp_enabled = TRUE;
    else
        btu_cb.hcilp_cb.sent_vsc_lp_enabled = FALSE;

    HCI_TRACE_DEBUG1("HCILP_Enable sleep_mode = %d",
                        btu_cb.hcilp_cb.params.sleep_mode);

    /* set callback */
    btu_cb.hcilp_cb.p_enable_cback = p_cb;

    UINT8_TO_STREAM(p, btu_cb.hcilp_cb.params.sleep_mode);

    if ((btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_DISABLE)
      ||(btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_UART)
      ||(btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_H5))
    {
        /* Controller expects all of bytes regardless of sleep mode */
        UINT8_TO_STREAM(p, btu_cb.hcilp_cb.params.host_stack_idle_threshold);
        UINT8_TO_STREAM(p, btu_cb.hcilp_cb.params.host_controller_idle_threshold);
        UINT8_TO_STREAM(p, btu_cb.hcilp_cb.params.bt_wake_polarity);
        UINT8_TO_STREAM(p, btu_cb.hcilp_cb.params.host_wake_polarity);
        UINT8_TO_STREAM(p, btu_cb.hcilp_cb.params.allow_host_sleep_during_sco);
        UINT8_TO_STREAM(p, btu_cb.hcilp_cb.params.combine_sleep_mode_and_lpm);
        UINT8_TO_STREAM(p, btu_cb.hcilp_cb.params.enable_uart_txd_tri_state);
        UINT8_TO_STREAM(p, 0); /* Active_Connection_Handling_On_Suspend only for mode 3 or 5 */
        UINT8_TO_STREAM(p, 0); /* Resume_Timeout only for mode 3 or 5   */
        UINT8_TO_STREAM(p, 0); /* Enable_BREAK_To_Host only for mode 12 */
        UINT8_TO_STREAM(p, btu_cb.hcilp_cb.params.pulsed_host_wake);

        result = BTM_VendorSpecificCommand( HCI_BRCM_WRITE_SLEEP_MODE,
                                            HCI_BRCM_WRITE_SLEEP_MODE_LENGTH,
                                            (UINT8 *)data,
                                            hcilp_vs_cback);
    }
#if (defined(H4IBSS_INCLUDED) && H4IBSS_INCLUDED == TRUE)
    else if (btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_H4IBSS)
    {
        UINT8_TO_STREAM(p, btu_cb.hcilp_cb.params.host_stack_idle_threshold);
        UINT8_TO_STREAM(p, btu_cb.hcilp_cb.params.host_controller_idle_threshold);
        UINT8_TO_STREAM(p, btu_cb.hcilp_cb.params.sleep_guard_time);
        UINT8_TO_STREAM(p, btu_cb.hcilp_cb.params.wakeup_guard_time);
        UINT8_TO_STREAM(p, btu_cb.hcilp_cb.params.allow_host_sleep_during_sco);
        UINT8_TO_STREAM(p, btu_cb.hcilp_cb.params.txd_config);

        result = BTM_VendorSpecificCommand( HCI_BRCM_ENABLE_H4IBSS,
                                            HCI_BRCM_ENABLE_H4IBSS_LENGTH,
                                            (UINT8 *)data,
                                            hcilp_vs_cback);

        h4ibss_init(btu_cb.hcilp_cb.params.port);
    }
#endif
    else
    {
        HCI_TRACE_ERROR1("HCILP_Enable Unsupported sleep_mode = %d",
                            btu_cb.hcilp_cb.params.sleep_mode);

        /* reset callback */
        btu_cb.hcilp_cb.p_enable_cback = NULL;

        return FALSE;
    }

    return (result <= BTM_CMD_STARTED) ? TRUE : FALSE;
#else
    return FALSE;
#endif
}

/*******************************************************************************
**
** Function           HCILP_Disable
**
** Description        Disable sleep mode by setting sleep mode 0 to controller
**
** Output Parameter   None
**
** Returns            TRUE  if sleep mode command was issued to BT device
**                    FALSE if the feature is not supported
**
*******************************************************************************/
BOOLEAN HCILP_Disable(tHCILP_ENABLE_CBACK *p_cb)
{
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
    tBTM_STATUS result;
    UINT8 data[HCI_BRCM_WRITE_SLEEP_MODE_LENGTH];
    UINT8 *p = data;

    HCI_TRACE_DEBUG0("HCILP_Disable");

    btu_cb.hcilp_cb.sent_vsc_lp_enabled = FALSE;

    UINT8_TO_STREAM(p, HCILP_SLEEP_MODE_DISABLE); /* sleep mode 0, disable */

    /* set callback */
    btu_cb.hcilp_cb.p_enable_cback = p_cb;

    if ((btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_UART)
      ||(btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_H5))
    {
        result = BTM_VendorSpecificCommand( HCI_BRCM_WRITE_SLEEP_MODE,
                                            HCI_BRCM_WRITE_SLEEP_MODE_LENGTH,
                                            (UINT8 *)data,
                                            hcilp_vs_cback);
    }
#if (defined(H4IBSS_INCLUDED) && H4IBSS_INCLUDED == TRUE)
    else if (btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_H4IBSS)
    {
        result = BTM_VendorSpecificCommand( HCI_BRCM_ENABLE_H4IBSS,
                                            HCI_BRCM_ENABLE_H4IBSS_LENGTH,
                                            (UINT8 *)data,
                                            hcilp_vs_cback);
    }
#endif
    else
    {
        /* reset callback */
        btu_cb.hcilp_cb.p_enable_cback = NULL;

        return FALSE;
    }

    return (result <= BTM_CMD_STARTED) ? TRUE : FALSE;
#else
    return FALSE;
#endif
}

/*******************************************************************************
**
** Function           HCILP_IsEnabled
**
** Description        return whether LPM is enabled or not
**
** Output Parameter   None
**
** Returns            TRUE  if sleep mode is enabled
**                    FALSE if sleep mode is not enabled
**
*******************************************************************************/
BOOLEAN HCILP_IsEnabled(void)
{
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
    return btu_cb.hcilp_cb.lp_enabled;
#else
    return FALSE;
#endif
}

/*******************************************************************************
**
** Function           HCILP_IsSleepState
**
** Description        Checks if its ok for application to go to sleep mode
**
** Output Parameter   None
**
** Returns            TRUE  if bluetooth is OK for app to go to sleep mode
**                    FALSE if bluetooth is awake and cannot go to sleep mode
**
*******************************************************************************/
BOOLEAN HCILP_IsSleepState(void)
{
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)

    tUPIO_STATE gpio_state;

    if (btu_cb.hcilp_cb.lp_enabled)
    {
        if(( btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_UART )
         ||( btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_UART_WITH_MSG )
         ||( btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_UART_CS_N ))
        {
            /* Read HOST_WAKE gpio */
            gpio_state = UPIO_Read(UPIO_GENERAL, HCILP_HOST_WAKE_GPIO);

            if(btu_cb.hcilp_cb.params.bt_wake_polarity)
                return (gpio_state == UPIO_OFF) ? TRUE : FALSE;
            else
                return (gpio_state == UPIO_ON) ? TRUE : FALSE;
        }
#if (defined(H4IBSS_INCLUDED) && H4IBSS_INCLUDED == TRUE)
        else if( btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_H4IBSS )
        {
            if(h4ibss_is_uart_active())
            {
                return TRUE;
            }
            else
                return FALSE;
        }
#endif
    }
#endif /*(defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)*/
    return FALSE;
}


/*******************************************************************************
**
** Function           HCILP_AppSleeping
**
** Description        Called by application when it wants to indicate to bluetooth
**                    that it is going into low power mode
**
** Output Parameter   None
**
**
*******************************************************************************/
void HCILP_AppSleeping(void)
{
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)

    tUPIO_STATE state;
    if (btu_cb.hcilp_cb.lp_enabled)
    {
        if(( btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_UART )
         ||( btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_UART_WITH_MSG )
         ||( btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_UART_CS_N ))
        {
            /* assert BT_WAKE if its deasserted */
            state = (btu_cb.hcilp_cb.params.bt_wake_polarity ? UPIO_OFF : UPIO_ON);
            UPIO_Set(UPIO_GENERAL, HCILP_BT_WAKE_GPIO, state);
        }
#if (defined(H4IBSS_INCLUDED) && H4IBSS_INCLUDED == TRUE)
        else if( btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_H4IBSS )
        {
            h4ibss_sm_execute (H4IBSS_API_ALLOW_BT_SLEEP_EVT);
        }
#endif
    }
#endif /*(defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)*/
}


/*******************************************************************************
**
** Function           HCILP_WakeupBTDevice
**
** Description        Called to wake up Bluetooth chip. This is called by HCIS
**                    when there is data to be sent over UART
**
** Output Parameter   None
**
** Returns            TRUE  If UART is active
**                    FALSE If UART is NOT active
**
**
*******************************************************************************/
BOOLEAN HCILP_WakeupBTDevice(tHCILP_WAKEUP_BT_CBACK *p_cback)
{
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
    tUPIO_STATE state;

    if (btu_cb.hcilp_cb.lp_enabled && btu_cb.hcilp_cb.state != HCILP_STATE_ACTIVE)
    {
        if(( btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_UART )
         ||( btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_UART_WITH_MSG )
         ||( btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_UART_CS_N ))
        {
            /* deassert BT_WAKE in case we are waking up and sending data */
            state = (btu_cb.hcilp_cb.params.bt_wake_polarity ? UPIO_ON : UPIO_OFF);
            UPIO_Set(UPIO_GENERAL, HCILP_BT_WAKE_GPIO, state);
            //when host assert DEV_WAKE, delay 20ms for chip to prepare wake from sleep mode
            //GKI_delay(20);
        }
#if (defined(H4IBSS_INCLUDED) && H4IBSS_INCLUDED == TRUE)
        else if( btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_H4IBSS )
        {
            if(!h4ibss_is_uart_active())
            {
                h4ibss_sm_execute (H4IBSS_API_WAKE_UP_BT_EVT);
                btu_cb.hcilp_cb.p_wakeup_cback = p_cback;
                return FALSE;
            }
        }
#endif
    }
    hcilp_stop_hc_idle_time();
    btu_cb.hcilp_cb.state = HCILP_STATE_ACTIVE;
    HCILP_IsTxDoneCplted(FALSE);
#endif
    return TRUE;
}

/*******************************************************************************
**
** Function           HCILP_WakeupHost
**
** Description
**
** Output Parameter   None
**
** Returns            None
**
*******************************************************************************/
void HCILP_WakeupHost(void)
{
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
#if (defined(H4IBSS_INCLUDED) && H4IBSS_INCLUDED == TRUE)
    if( btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_H4IBSS )
        h4ibss_sm_execute (H4IBSS_CTS_LINE_LOW_EVT);
#endif
#endif
}

#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
/*******************************************************************************
**
** Function           hcilp_do_allow_dev_sleep
**
** Description        allow BT device to sleep after HC idle time
**
** Output Parameter   None
**
**
*******************************************************************************/
void hcilp_do_allow_dev_sleep(void)
{
    tUPIO_STATE state;

    if (btu_cb.hcilp_cb.lp_enabled && btu_cb.hcilp_cb.lp_no_tx_data)
    {
        state = (btu_cb.hcilp_cb.params.bt_wake_polarity ? UPIO_OFF : UPIO_ON);
        UPIO_Set(UPIO_GENERAL, HCILP_BT_WAKE_GPIO, state);
        btu_cb.hcilp_cb.state = HCILP_STATE_LOW_POWER;
    }
}
#endif /*(defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)*/

/*******************************************************************************
**
** Function           HCILP_AllowBTDeviceSleep
**
** Description        Called by HCIS to indicate to bluetooth chip that it can go to
**                    sleep mode.This is called by when all data has been
**                    read from UART
**
** Output Parameter   None
**
**
*******************************************************************************/
void HCILP_AllowBTDeviceSleep(void)
{
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
    if (btu_cb.hcilp_cb.lp_enabled)
    {
        if(( btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_UART )
         ||( btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_UART_WITH_MSG )
         ||( btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_UART_CS_N ))
        {
            if(btu_cb.hcilp_cb.state != HCILP_STATE_ACTIVE)
            {
                return;
            }
            else if(btu_cb.hcilp_cb.lp_no_tx_data)
            {
                btu_cb.hcilp_cb.state = HCILP_STATE_W4_TIMEOUT;
                /* wait up BT_WAKE idle time */
                hcilp_start_bt_wake_idle_time();
            }
            else
            {
                btu_cb.hcilp_cb.state = HCILP_STATE_W4_TX_DONE;
            }
        }
#if (defined(H4IBSS_INCLUDED) && H4IBSS_INCLUDED == TRUE)
        else if( btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_H4IBSS )
        {
            h4ibss_sm_execute (H4IBSS_API_ALLOW_BT_SLEEP_EVT);
        }
#endif
#if (defined(SLIP_INCLUDED) && SLIP_INCLUDED == TRUE)
        else if( btu_cb.hcilp_cb.params.sleep_mode == HCILP_SLEEP_MODE_H5 )
        {
            SLIP_AllowBTDeviceSleep();
        }
#endif
        return;
    }
#endif /*(defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)*/
}

/*******************************************************************************
**
** Function          HCILP_IsTxDoneCplted
**
** Description       This function is to inform the hcilp module
**                   if data is waiting in the Tx Q or not.
**
**                   IsTxDone: TRUE if All data in the Tx Q are gone
**                             FALSE if all data in the Tx Q not gone.
**                   Typicaly this function must be called
**                   before USERIAL Write and in the Tx Done routine
**
** Returns           None
**
*******************************************************************************/
void HCILP_IsTxDoneCplted(BOOLEAN IsTxDone)
{
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
    btu_cb.hcilp_cb.lp_no_tx_data = IsTxDone;

    if ((btu_cb.hcilp_cb.state == HCILP_STATE_W4_TX_DONE)
      &&(IsTxDone))
    {
        btu_cb.hcilp_cb.state = HCILP_STATE_W4_TIMEOUT;
        /* wait up BT_WAKE idle time */
        hcilp_start_bt_wake_idle_time();
    }
#endif
}

void HCILP_SetParameters(UINT8 bt_controller_idle_threshold, UINT16 bt_wake_idle_timeout)
{
    if(bt_controller_idle_threshold >= 0x02 && bt_wake_idle_timeout >= 1)
    {
        idle_thres_user_set= bt_controller_idle_threshold;
        idle_timeout_user_set = bt_wake_idle_timeout;
    }
    else
    {

    }
}

void HCILP_GetParameters(UINT8* bt_controller_idle_threshold, UINT16* bt_wake_idle_timeout)
{
    *bt_controller_idle_threshold = btu_cb.hcilp_cb.params.host_controller_idle_threshold;
    *bt_wake_idle_timeout         = btu_cb.hcilp_cb.params.bt_wake_idle_timeout;
}

