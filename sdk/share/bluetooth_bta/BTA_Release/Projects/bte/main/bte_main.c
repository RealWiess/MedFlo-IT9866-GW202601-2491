/*****************************************************************************
**                                                                           *
**  Name:          bte_main.c                                                *
**                                                                           *
**  Description:   This is the main module for the BTE system.  It contains  *
**                 task implementations, system initialization functions,    *
**                 and the main function for the system.                     *
**                                                                           *
**  Copyright (c) 2018-2019, Broadcom Inc.                                   *
**  Broadcom Bluetooth Core. Proprietary and confidential.                   *
******************************************************************************/

#include "bt_target.h"
#include "gki.h"
#include "bte.h"
#include "upio.h"
#include "utimer.h"
#include "userial.h"

/* global tracing variable */
#include "testercfg.h"
CTesterCfg tester_cfg;
int ScrProtocolTraceFlag = 0xFFFFFFFF;
UINT8 appl_trace_level = APPL_INITIAL_TRACE_LEVEL;
boot_cplt_cb_t boot_cplt_cb;

/* UNV support task */
#if (defined(UNV_INCLUDED) && (UNV_INCLUDED == TRUE))
extern UDRV_API void unv_task(UINT32 param);
#endif

BTU_API extern UINT32 btu_task (UINT32 param);
BTU_API extern void BTE_Init (void);
BT_API extern void BTE_LoadStack(void);

/*****************************************************************************/
/*                            HCIS Configuration                             */
/*****************************************************************************/
#include "hcisu.h"

/* H4 Configuration for BTE */
#if defined(HCISU_H4_INCLUDED) && HCISU_H4_INCLUDED == TRUE
#include "userial.h"
#include "hcis_h4.h"
extern const tHCISU_IF hcisu_h4;
static const tHCIS_H4_CFG bte_hcisu_h4_cfg =
{
    USERIAL_PORT_1,
    USERIAL_BAUD_115200,
    USERIAL_DATABITS_8 | USERIAL_PARITY_NONE | USERIAL_STOPBITS_1,
#if defined (SLIP_INCLUDED) && SLIP_INCLUDED == TRUE
    1,
#else
    0,
#endif

};
#endif /* HCISU_H4_INCLUDED */

/*****************************************************************************
**                        System Task Configuration                          *
******************************************************************************/
/* HCI transport task */
#ifndef BTE_HCI_STACK_SIZE
#ifdef BT_TRACE_PROTOCOL
#define BTE_HCI_STACK_SIZE       0x2000
#else
#define BTE_HCI_STACK_SIZE       0x0800         /* In bytes */
#endif
#endif

#define BTE_HCI_TASK_STR        ((INT8 *) "hci_su_task")

/* bluetooth protocol stack (BTU) task */
#ifndef BTE_BTU_STACK_SIZE
#define BTE_BTU_STACK_SIZE       0x1000         /* In bytes */
#endif
#define BTE_BTU_TASK_STR        ((INT8 *) "btu_task")

#if (defined(UNV_INCLUDED) && (UNV_INCLUDED == TRUE))
/* universal NV stack (UNV) task */
#ifndef BTE_UNV_STACK_SIZE
#define BTE_UNV_STACK_SIZE       0x0400         /* In bytes */
#endif
#define BTE_UNV_TASK_STR        ((INT8 *) "unv_task")
#endif

#if ((defined(BTAPPL_INCLUDED)) && (BTAPPL_INCLUDED == TRUE))
#ifndef BTAPPL_STACK_SIZE
#define BTAPPL_STACK_SIZE      0x0400         /* In bytes */
#endif

#ifndef BTAPPL_TASK_STR
#define BTAPPL_TASK_STR       ((INT8 *) "btapp_task")   /* Default Name */
#endif
extern void btapp_task(UINT32 params);

#endif

#if ((defined(UCODEC_INCLUDED) && (UCODEC_INCLUDED == TRUE)))
#define UCODEC_STACK_SIZE      0x0800
#define UCODEC_TASK_STR        ((INT8 *) "ucodec_task")
extern void btapp_avk_ucodec_task(UINT32 params);
#endif

#if ((defined(AUDIO_PLAY_INCLUDED) && (AUDIO_PLAY_INCLUDED == TRUE)))
#define AUDIO_PLAY_STACK_SIZE      0x0800
#define AUDIO_PLAY_TASK_STR        ((INT8 *) "audio_play_task")
extern void btapp_avk_audio_play_task(UINT32 params);
#endif

#if ((defined(BTAPP_CONSOLE_INCLUDED) && (BTAPP_CONSOLE_INCLUDED == TRUE)))
#define BTAPP_CONSOLE_STACK_SIZE    0x0200
#define BTAPP_CONSOLE_TASK_STR     ((INT8*)"btapp_console_task")
extern void btapp_console_task(UINT32 p);
#endif

/*****************************************************************************
**                          F U N C T I O N S                                *
******************************************************************************/
/*******************************************************************************
**
** Function         BTE_InitHW
**
** Description      Initialize hardware.
**
** Returns          void
**
*******************************************************************************/
void BTE_InitHW(void)
{
    /* initialize gpio devices */
    UPIO_Init(NULL);

    GKI_delay(5);

    /* initialize serial driver */
    USERIAL_Init(NULL);

#if 0 //For 43013
    UPIO_Set_CTS_Low();
#endif

    /* reset bt controller*/
    UPIO_Set(UPIO_GENERAL, BT_REG_ON_GPIO, UPIO_OFF);
    GKI_delay(50);
    UPIO_Set(UPIO_GENERAL, BT_REG_ON_GPIO, UPIO_ON);
    GKI_delay(50);

#if 0
    UPIO_Restore_CTS_Setting();
#endif
}

/*******************************************************************************
**
** Function         BTE_CreateTasks
**
** Description      Create BTE tasks.
**
** Returns          void
**
*******************************************************************************/
static void BTE_CreateTasks (void)
{
    tester_cfg.nLiteProtoFilter = TRACE_FILTER_HCI_PARAM | \
                                  SCR_PROTO_TRACE_L2CAP;
//                                  TRACE_FILTER_HCI_DATA_HDR

#if (defined(UNV_INCLUDED) && (UNV_INCLUDED == TRUE))
    /* Create UNV Task */
    GKI_create_task((TASKPTR)unv_task, UNV_TASK, BTE_UNV_TASK_STR,
                    NULL,
                    BTE_UNV_STACK_SIZE);
#endif

#if (defined (BTU_INCLUDED) && (BTU_INCLUDED == TRUE))
    GKI_create_task((TASKPTR)btu_task, BTU_TASK, BTE_BTU_TASK_STR,
                NULL,
                BTE_BTU_STACK_SIZE);
#endif

#if ((defined(HCISU_H4_INCLUDED) && (HCISU_H4_INCLUDED == TRUE)))
    /* Initialize pointer to function that sends hci commands and data to the transport */
    p_hcisu_if  = (tHCISU_IF *)&hcisu_h4;
    p_hcisu_cfg = (void *)&bte_hcisu_h4_cfg;

    /* Start the HCI Services task */
    GKI_create_task((TASKPTR)bte_hcisu_task, HCISU_TASK, BTE_HCI_TASK_STR,
        NULL,
        BTE_HCI_STACK_SIZE);

#endif  /* HCISU_H4_INCLUDED */

#if ((defined(BTAPPL_INCLUDED)) && (BTAPPL_INCLUDED == TRUE))
    /* Create the BTE application task */
    GKI_create_task((TASKPTR)btapp_task, BTAPPL_TASK, BTAPPL_TASK_STR,
                   NULL,
                   BTAPPL_STACK_SIZE);
#endif

#if ((defined(UCODEC_INCLUDED)) && (UCODEC_INCLUDED == TRUE))
    /* Create the ucodec task */
    GKI_create_task((TASKPTR)btapp_avk_ucodec_task, UCODEC_TASK, UCODEC_TASK_STR,
                   NULL,
                   UCODEC_STACK_SIZE);
#endif

#if ((defined(AUDIO_PLAY_INCLUDED) && (AUDIO_PLAY_INCLUDED == TRUE)))
    /* Create the audio play task */
    GKI_create_task((TASKPTR)btapp_avk_audio_play_task, AUDIO_PLAY_TASK, AUDIO_PLAY_TASK_STR,
               NULL,
               AUDIO_PLAY_STACK_SIZE);
#endif

#if ((defined(BTAPP_CONSOLE_INCLUDED) && (BTAPP_CONSOLE_INCLUDED == TRUE)))
    GKI_create_task((TASKPTR)btapp_console_task, BTAPP_CONSOLE_TASK, BTAPP_CONSOLE_TASK_STR,
               NULL,
               BTAPP_CONSOLE_STACK_SIZE);
#endif
}

/*******************************************************************************
**
** Function         BootEntry
**
** Description      BTE main function.
**
** Returns          nothing
**
*******************************************************************************/
BT_API int BootEntry( boot_cplt_cb_t cb )
{
    /* Get the lib version string*/
    bte_version_build();

#if 0 //def HCI_LOG_FILE
    extern BOOLEAN btsnoop_open(const BOOLEAN save_existing);
    btsnoop_open(TRUE);
#endif

    /* initialize OS */
    GKI_init();

#ifdef BTE_PLATFORM_INITHW
    /* Implement platform HW initialization in this macro - can be BTE_InitHW function */
    BTE_PLATFORM_INITHW;
#endif

    BTE_InitHW();

//    /* enable interrupts */
//    GKI_enable();

    /* (optionally) allocate memory for stack control blocks */
    BTE_LoadStack();

    /* Initialize BTE control block */
    BTE_Init();

    /* start tasks */
    GKI_run(0);
    GKI_delay(10);

    /* create tasks */
    BTE_CreateTasks();

    boot_cplt_cb = cb;

    return 0;
}
