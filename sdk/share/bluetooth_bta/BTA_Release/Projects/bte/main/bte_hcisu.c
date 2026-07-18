/*****************************************************************************
**
**  Name:          bte_hcisu.c
**
**  Description:
**      HCI Services for the upper layer stack.
**
**      This file contains the main function for the HCISU task - used to put
**      handle events for the HCI Services upper-layer transport.
**
**      This file also contains event notification and message sending
**      functions for the HCISU libraries to use.
**
**  Copyright (c) 2018-2019, Broadcom Inc.
**  Copyright (c) 2002-2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/
#include <string.h>
#include "bt_target.h"
#include "gki.h"
#include "bte.h"
#include "btm_int.h"
#include "btu.h"
#include "hcidefs.h"
#include "hcisu.h"
#if (defined(SLIP_INCLUDED) && SLIP_INCLUDED == TRUE)
#include "slip.h"
#endif
#if (BTU_DUAL_STACK_MM_INCLUDED == TRUE) || (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
#include "uipc.h"
#endif

/* HCI and L2CAP Protocol Trace display functions */
#if BT_TRACE_PROTOCOL == TRUE
#include "trace_api.h"
#endif

#if (!defined(NFC_ONLY_MODE))
#define NFC_ONLY_MODE       FALSE
#endif

/*******************************************************************************
** Global Variables
*******************************************************************************/
/* Pointer to current lower-layer HCI Service functions */
tHCISU_IF *p_hcisu_if = NULL;
void *p_hcisu_cfg = NULL;       /* Pointer to configuration parameter of current HCIS */

#if defined(QUICK_TIMER_TICKS_PER_SEC) && (QUICK_TIMER_TICKS_PER_SEC > 0)
static TIMER_LIST_Q  hcisu_quick_timer_queue;        /* Timer queue for transport level (100/10 msec)*/
static void bte_hcisu_process_quick_timer_evt(void);
#endif

/*******************************************************************************
**
** Function         bte_hcisu_init
**
** Description      Initialize HCI services
**
** Returns          nothing
**
*******************************************************************************/
BT_API void bte_hcisu_init(void)
{
    hcisu_quick_timer_queue.last_ticks  = 0;
    hcisu_quick_timer_queue.p_first     = NULL;
    hcisu_quick_timer_queue.p_last      = NULL;

    /* Handle case where no HCI transport has been selected */
    if (!p_hcisu_if)
        return;

    /* Initialize control block and other memories associated with HCISL */
    if (p_hcisu_if->init)
        p_hcisu_if->init(BTU_TASK, HCISU_TASK, HCISU_EVT_MASK);

    /* Open connection for HCISL */
    if (p_hcisu_if->open)
        p_hcisu_if->open(p_hcisu_cfg);
}

/*******************************************************************************
**
** Function         bte_hcisu_reset_slip
**
** Description
**
** Returns          void
**
*******************************************************************************/
BT_API void bte_hcisu_reset_slip (void)
{
#if defined (SLIP_INCLUDED) && SLIP_INCLUDED == TRUE
    BT_HDR *p_msg;

    if ((p_msg = (BT_HDR *)GKI_getbuf(BT_HDR_SIZE)) != NULL)
    {
        p_msg->event = BT_EVT_TO_HCISU_H5_RESET_EVT;
        GKI_send_msg (HCISU_TASK, HCISU_MBOX, p_msg);
    }
#endif
}


/*******************************************************************************
**
** Function         bte_hcisu_send
**
** Description
**      This function is called by the upper stack to send an HCI message. The
**      function displays a protocol trace message (if enabled), and then calls
**      the 'send' function associated with the currently selected HCI transport.
**
** Returns          void
**
*******************************************************************************/
BT_API void bte_hcisu_send (BT_HDR *p_msg, UINT16 event)
{
#if (((BT_TRACE_PROTOCOL == TRUE) && (HCITHIN_INCLUDED == FALSE)) || (defined(NFC_SHARED_TRANSPORT_ENABLED) && (NFC_SHARED_TRANSPORT_ENABLED==TRUE)))
    UINT16 bt_event = event & BT_EVT_MASK;
#endif
    UINT16 sub_event = event & BT_SUB_EVT_MASK;  /* local controller ID */

    p_msg->event = event;

#ifdef HCI_LOG_FILE
    extern UINT8 is_btsnoop_hci_enabled();
    if(is_btsnoop_hci_enabled())
    {
        extern void btsnoop_capture(const BT_HDR *buffer, BOOLEAN is_received);
        btsnoop_capture(p_msg, FALSE);
    }
#endif

#if ((BT_TRACE_PROTOCOL == TRUE) && (HCITHIN_INCLUDED == FALSE))
    /* Display protocol trace message */
    /* (No protocol traces for Thin HCI because HCI messages are packed differently) */
    if (bt_event == BT_EVT_TO_LM_HCI_CMD)
    {
#if (BT_TRACE_DISP_HCICMD == TRUE)
        DispHciCmd(p_msg);
#endif
    }
    else if (bt_event == BT_EVT_TO_LM_HCI_ACL)
    {
        DispL2CCmd(p_msg,FALSE);
        DispHciAclData (p_msg, FALSE);
    }
    else if (bt_event == BT_EVT_TO_LM_HCI_SCO)
    {
        DispHciScoData (p_msg, FALSE);
    }
#if (defined(NFC_SHARED_TRANSPORT_ENABLED) && (NFC_SHARED_TRANSPORT_ENABLED==TRUE))
    else if (bt_event == BT_EVT_TO_NFC_NCI)
    {
        DispNci ((UINT8 *)(p_msg+1) + p_msg->offset, p_msg->len, FALSE);
    }
#endif

#endif

#if (BTU_DUAL_STACK_MM_INCLUDED == TRUE) || (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
    /* If co-processor is has control of HCI then send the buffer to the co-processor using IPC */
    if (BTU_IsHciOverIpc())
    {
        UIPC_SENDBUF(p_msg);
        return;
    }
#endif

    if (sub_event == LOCAL_BR_EDR_CONTROLLER_ID
#if (BLE_INCLUDED == TRUE)
        || sub_event == LOCAL_BLE_CONTROLLER_ID
#endif
        )
    {
        GKI_send_msg(HCISU_TASK, HCISU_MBOX, p_msg);
    }
#if (defined(NFC_SHARED_TRANSPORT_ENABLED) && (NFC_SHARED_TRANSPORT_ENABLED==TRUE))
    else if( bt_event == BT_EVT_TO_NFC_NCI )
        GKI_send_msg(HCISU_TASK, HCISU_MBOX, p_msg);
#endif
    else
    {
        /* AMP output macro not defined. Just free the buffer. */
        HCI_TRACE_ERROR0( "HCIS: AMP not included. Discarding AMP message.");
        GKI_freebuf(p_msg);
    }
}

#if (defined(BTU_STACK_LITE_ENABLED) && BTU_STACK_LITE_ENABLED == FALSE)
/*******************************************************************************
**
** Function         bte_hcisu_lp_app_sleeping
**
** Description
**
** Returns          void
**
*******************************************************************************/
BT_API void bte_hcisu_lp_app_sleeping (void)
{
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
    tHCISU_LP_IF_MSG *p_msg;

    if ((p_msg = (tHCISU_LP_IF_MSG *) GKI_getbuf(sizeof(tHCISU_LP_IF_MSG))) != NULL)
    {
        p_msg->hdr.event = BT_EVT_TO_HCISU_LP_APP_SLEEPING_EVT;
        GKI_send_msg(HCISU_TASK, HCISU_MBOX, p_msg);
    }
#endif
}
/*******************************************************************************
**
** Function         bte_hcisu_lp_allow_bt_device_sleep
**
** Description
**
** Returns          void
**
*******************************************************************************/
BT_API void bte_hcisu_lp_allow_bt_device_sleep (void)
{
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
    tHCISU_LP_IF_MSG *p_msg;

    if ((p_msg = (tHCISU_LP_IF_MSG *) GKI_getbuf(sizeof(tHCISU_LP_IF_MSG))) != NULL)
    {
        p_msg->hdr.event = BT_EVT_TO_HCISU_LP_ALLOW_BT_SLEEP_EVT;
        GKI_send_msg(HCISU_TASK, HCISU_MBOX, p_msg);
    }
#endif
}
/*******************************************************************************
**
** Function         bte_hcisu_lp_wakeup_host
**
** Description
**
** Returns          void
**
*******************************************************************************/
BT_API void bte_hcisu_lp_wakeup_host (void)
{
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
    tHCISU_LP_IF_MSG *p_msg;

    if ((p_msg = (tHCISU_LP_IF_MSG *) GKI_getbuf(sizeof(tHCISU_LP_IF_MSG))) != NULL)
    {
        p_msg->hdr.event = BT_EVT_TO_HCISU_LP_WAKEUP_HOST_EVT;
        GKI_send_msg(HCISU_TASK, HCISU_MBOX, p_msg);
    }
#endif
}

/*******************************************************************************
**
** Function         bte_hcisu_lp_h4ibss_evt
**
** Description
**
** Returns          void
**
*******************************************************************************/
BT_API void bte_hcisu_lp_h4ibss_evt(UINT8 *p, UINT8 evt_len)
{
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)&&(defined(H4IBSS_INCLUDED) && H4IBSS_INCLUDED == TRUE)
    tHCISU_LP_IF_MSG *p_msg;

    if ((p_msg = (tHCISU_LP_IF_MSG *) GKI_getbuf(sizeof(tHCISU_LP_IF_MSG))) != NULL)
    {
        p_msg->hdr.event = BT_EVT_TO_HCISU_LP_RCV_H4IBSS_EVT;
        p_msg->h4ibss_evt = *p;
        GKI_send_msg(HCISU_TASK, HCISU_MBOX, p_msg);
    }
#endif
}
#endif /*(defined(BTU_STACK_LITE_ENABLED) && BTU_STACK_LITE_ENABLED == FALSE)*/

/*****************************************************************************
** HCISU Task
*****************************************************************************/

/*******************************************************************************
**
** Function         bte_hcisu_task
**
** Description      The task that handles HCI transport events.
**
** Returns          void
**
*******************************************************************************/
BT_API void bte_hcisu_task(UINT32 param)
{
    UINT16 event;
    UINT16 ctrl_id;
    INT32  sleep_interval = 0;
    BT_HDR                  *p_msg;
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
    tHCISU_LP_IF_MSG        *p_lp_msg;
#endif
    UINT8  need_exit = FALSE;

    GKI_TRACE_0("bte_hcisu_task up");
    GKI_delay(10);

    /* Initialize HCI services */
    bte_hcisu_init();

#if defined(HCILP_INCLUDED) && (HCILP_INCLUDED == TRUE)
    HCILP_Init(0);
#endif

    /* notify BTU task that HCISU is ready */
    GKI_send_event(BTU_TASK, TASK_MBOX_0_EVT_MASK);

    while (TRUE)
    {
        event = GKI_wait(0xFFFF, sleep_interval);
        if(sleep_interval != 0)
        {
            GKI_TRACE_1("bte_hcisu_task sleep_interval:%d", sleep_interval);
        }

        /* Call HCISU event handler if an event occured from USERIAL, */
        /* or sleep_interval has elapsed */
        if (((event & HCISU_EVT_MASK) || (!event)) && p_hcisu_if)
            sleep_interval = p_hcisu_if->handle_event(event);

        /* Processing received messages */
        if (event & HCISU_MBOX_EVT_MASK)
        {
            while ((p_msg = (BT_HDR *) GKI_read_mbox (HCISU_MBOX)) != NULL)
            {
                switch (p_msg->event & BT_EVT_MASK)
                {
#if (defined(NFC_SHARED_TRANSPORT_ENABLED) && (NFC_SHARED_TRANSPORT_ENABLED==TRUE))
                case BT_EVT_TO_NFC_NCI:
                    if (p_hcisu_if)
                        p_hcisu_if->send(p_msg);
                    else
                        GKI_freebuf(p_msg);
                    break;
#endif
                case BT_EVT_TO_LM_HCI_ACL:
                case BT_EVT_TO_LM_HCI_SCO:
                case BT_EVT_TO_LM_HCI_CMD:
                case BT_EVT_TO_LM_DIAG:
                    /* Controller id is in the sub-event mask */
                    ctrl_id = p_msg->event & BT_SUB_EVT_MASK; /* local controller ID */
                    p_msg->event &= (BT_EVT_MASK
#if (BLE_INCLUDED == TRUE)
                        | LOCAL_BLE_CONTROLLER_ID
#endif
                        );

                    /* Redirect AMPs commands/data to AMP output function */
                    /* (eventually, this may be moved to a different task) */
                    if (ctrl_id != LOCAL_BR_EDR_CONTROLLER_ID
#if (BLE_INCLUDED == TRUE)
                        && ctrl_id != LOCAL_BLE_CONTROLLER_ID
#endif
                        )
                    {
                        /* AMP output macro not defined. Just free the buffer. */
                        HCI_TRACE_ERROR0( "HCIS: AMP not included. Discarding AMP message.");
                        GKI_freebuf(p_msg);
                        break;
                    }

                    if (p_hcisu_if)
                        p_hcisu_if->send(p_msg);
                    else
                        GKI_freebuf(p_msg);
                    break;


#if (defined(BTU_STACK_LITE_ENABLED) && BTU_STACK_LITE_ENABLED == FALSE)
                case BT_EVT_HCISU:
                {
                    switch (p_msg->event)
                    {
#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
                    case BT_EVT_TO_HCISU_LP_APP_SLEEPING_EVT:
                        HCILP_AppSleeping();
                        GKI_freebuf(p_msg);
                        break;

                    case BT_EVT_TO_HCISU_LP_ALLOW_BT_SLEEP_EVT:
                        HCILP_AllowBTDeviceSleep();
                        GKI_freebuf(p_msg);
                        break;

                    case BT_EVT_TO_HCISU_LP_WAKEUP_HOST_EVT:
                        HCILP_WakeupHost();
                        GKI_freebuf(p_msg);
                        break;

#if (defined(H4IBSS_INCLUDED) && H4IBSS_INCLUDED == TRUE)
                    case BT_EVT_TO_HCISU_LP_RCV_H4IBSS_EVT:
                        p_lp_msg = (tHCISU_LP_IF_MSG*)p_msg;
                        h4ibss_sleep_mode_evt((&p_lp_msg->h4ibss_evt), 1);
                        GKI_freebuf(p_msg);
                        break;
#endif
#endif
#if defined (SLIP_INCLUDED) && SLIP_INCLUDED == TRUE
                    case BT_EVT_TO_HCISU_H5_RESET_EVT :
                        if (p_hcisu_if->reset_slip)
                            p_hcisu_if->reset_slip(p_hcisu_cfg);
                        GKI_freebuf (p_msg);
                        break;
#endif

#if defined(QUICK_TIMER_TICKS_PER_SEC) && (QUICK_TIMER_TICKS_PER_SEC > 0)
                    case BT_EVT_HCISU_START_QUICK_TIMER :
                        GKI_start_timer (TIMER_2, QUICK_TIMER_TICKS, TRUE);
                        GKI_freebuf (p_msg);
                        break;
#endif
                    case BT_EVT_TO_HCISU_SHUTDOWN:
                        need_exit = TRUE;
                        GKI_freebuf(p_msg);
                        break;
                    default:
                        GKI_freebuf(p_msg);
                        break;
                    }
                    break;
                }
#endif /*(defined(BTU_STACK_LITE_ENABLED) && BTU_STACK_LITE_ENABLED == FALSE)*/
                default:
                    GKI_freebuf(p_msg);
                    break;
                }
            }

            if(need_exit == TRUE)
            {
                break;
            }
        }

        /* processing timer event */
#if defined(QUICK_TIMER_TICKS_PER_SEC) && (QUICK_TIMER_TICKS_PER_SEC > 0)
        if (event & HCISU_TIMER_EVT_MASK)
            bte_hcisu_process_quick_timer_evt();
#endif
    }

    GKI_TRACE_0("bte_hcisu_task exit");
    GKI_exit_task(HCISU_TASK);
}

/*******************************************************************************
**
** Function         bte_hcisu_shutdown
**
** Description      shutdown bte_hcisu task
**
**
** Returns          void
**
*******************************************************************************/
void bte_hcisu_shutdown(void)
{
    BT_HDR *p_msg;

    if ((p_msg = (BT_HDR *)GKI_getbuf(BT_HDR_SIZE)) != NULL)
    {
        p_msg->event = BT_EVT_TO_HCISU_SHUTDOWN;
        GKI_send_msg (HCISU_TASK, HCISU_MBOX, p_msg);
    }
    else
    {
        GKI_TRACE_0("bte_hcisu_shutdown failed!");
    }
}

#if defined(QUICK_TIMER_TICKS_PER_SEC) && (QUICK_TIMER_TICKS_PER_SEC > 0)
/*******************************************************************************
**
** Function         hcisu_start_quick_timer
**
** Description      Start a timer for the specified amount of time.
**                  NOTE: The timeout resolution depends on including modules.
**                  QUICK_TIMER_TICKS_PER_SEC should be used to convert from
**                  time to ticks.
**
**
** Returns          void
**
*******************************************************************************/
void bte_hcisu_start_quick_timer (TIMER_LIST_ENT *p_tle, UINT16 type, UINT32 timeout)
{
    BT_HDR *p_msg;

    /* if timer list is currently empty, start periodic GKI timer */
    if (hcisu_quick_timer_queue.p_first == NULL)
    {
        /* if timer starts on other than HCISU task */
        if(GKI_get_taskid() != HCISU_TASK)
        {
            /* post event to start timer in HCISU task */
            if ((p_msg = (BT_HDR *)GKI_getbuf(BT_HDR_SIZE)) != NULL)
            {
                p_msg->event = BT_EVT_HCISU_START_QUICK_TIMER;
                GKI_send_msg (HCISU_TASK, HCISU_MBOX, p_msg);
                GKI_TRACE_0("Other task send timer2 out");
            }
        }
        else
        {
            GKI_start_timer(TIMER_2, QUICK_TIMER_TICKS, TRUE);
        }
    }

    GKI_remove_from_timer_list (&hcisu_quick_timer_queue, p_tle);

    p_tle->event = type;
    p_tle->ticks = timeout; /* Save the number of ticks for the timer */

    GKI_add_to_timer_list (&hcisu_quick_timer_queue, p_tle);
}


/*******************************************************************************
**
** Function         hcisu_stop_quick_timer
**
** Description      Stop a timer.
**
** Returns          void
**
*******************************************************************************/
void bte_hcisu_stop_quick_timer (TIMER_LIST_ENT *p_tle)
{
    GKI_remove_from_timer_list (&hcisu_quick_timer_queue, p_tle);

    /* if timer list is empty stop periodic GKI timer */
    if (hcisu_quick_timer_queue.p_first == NULL)
    {
        GKI_stop_timer(TIMER_2);
        //GKI_TRACE_0("Stop Timer2");
    }
}

/*******************************************************************************
**
** Function         hcisu_process_quick_timer_evt
**
** Description      Process quick timer event
**
** Returns          void
**
*******************************************************************************/
void bte_hcisu_process_quick_timer_evt(void)
{
#if (NFC_ONLY_MODE == FALSE)
    process_quick_timer_evt(&hcisu_quick_timer_queue);

    /* if timer list is empty stop periodic GKI timer */
    if (hcisu_quick_timer_queue.p_first == NULL)
    {
        GKI_stop_timer(TIMER_2);
        //GKI_TRACE_0("bte_hcisu Stop Timer2");
    }
#endif
}
#endif /* defined(QUICK_TIMER_TICKS_PER_SEC) && (QUICK_TIMER_TICKS_PER_SEC > 0) */

