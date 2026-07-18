/*
 * Copyright (C) 2018-2020 AICSemi Ltd.
 *
 * All Rights Reserved
 */

#ifndef _PORTING_H_
#define _PORTING_H_

#include <aic_types.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "openrtos/FreeRTOSConfig.h"
/* compile not support zero array */
#define NOT_SUPPORT_ZERO_ARRAY
#define AIC_PRIORITY_MODE (4) // 1: default, ABOVE_NORMAL=150, NORMAL = 202, BELOW_NORMAL = 224.
                              // 2: no tx aggr
                              // 3: tx aggr, tx task, rx task and sdio task will increase priority
                              // over tcpip task when executing

/*============Task Priority===================*/
#if (AIC_PRIORITY_MODE == 1)
#define SDIO_DATRX_PRIORITY     (SCI_PRIORITY_ABOVE_NORMAL)
#define FHOST_CNTRL_PRIORITY    (SCI_PRIORITY_NORMAL)
#define FHOST_WPA_PRIORITY      (SCI_PRIORITY_ABOVE_NORMAL)
#define FHOST_TX_PRIORITY       (SCI_PRIORITY_ABOVE_NORMAL)
#define FHOST_RX_PRIORITY       (SCI_PRIORITY_ABOVE_NORMAL)
#define CLI_CMD_PRIORITY        (SCI_PRIORITY_ABOVE_NORMAL)
#define RWNX_TIMER_PRIORITY     (SCI_PRIORITY_ABOVE_NORMAL)
#define RWNX_APM_STALOSS_PRIORITY (SCI_PRIORITY_ABOVE_NORMAL)
#elif (AIC_PRIORITY_MODE == 2)
#define SDIO_DATRX_PRIORITY     (76)
#define FHOST_CNTRL_PRIORITY    (88)
#define FHOST_WPA_PRIORITY      (89)
#define FHOST_TX_PRIORITY       (76)
#define FHOST_RX_PRIORITY       (76)
#define CLI_CMD_PRIORITY        (88)
#define RWNX_TIMER_PRIORITY     (88)
#define RWNX_APM_STALOSS_PRIORITY (76)
#elif (AIC_PRIORITY_MODE == 3)
#define SDIO_DATRX_PRIORITY     (86)
#define FHOST_CNTRL_PRIORITY    (88)
#define FHOST_WPA_PRIORITY      (89)
#define FHOST_TX_PRIORITY       (86)
#define FHOST_RX_PRIORITY       (86)
#define CLI_CMD_PRIORITY        (88)
#define RWNX_TIMER_PRIORITY     (88)
#define RWNX_APM_STALOSS_PRIORITY (86)
#elif (AIC_PRIORITY_MODE == 4)
#define SDIO_DATRX_PRIORITY     (4)
#define FHOST_CNTRL_PRIORITY    (1)
#define FHOST_WPA_PRIORITY      (1)
#define FHOST_TX_PRIORITY       (4)
#define FHOST_RX_PRIORITY       (4)
#define CLI_CMD_PRIORITY        (4)
#define RWNX_TIMER_PRIORITY     (6)
#define RWNX_APM_STALOSS_PRIORITY (4)
#endif


#define TCPIP_PRIORITY          (83)
#define TASK_END_PRIO           (80)

extern uint32_t sdio_datrx_priority;
extern uint32_t fhost_cntrl_priority;
extern uint32_t fhost_wpa_priority;
extern uint32_t fhost_tx_priority;
extern uint32_t fhost_rx_priority;
extern uint32_t cli_cmd_priority;
extern uint32_t rwnx_timer_priority;
extern uint32_t rwnx_apm_staloss_priority;
extern uint32_t tcpip_priority;
extern uint32_t task_end_prio;
extern uint32_t aic_priority_mode;

/*============Stack Size (unint: 16bytes)===================*/
#define SDIO_DATRX_STACK_SIZE   configMINIMAL_STACK_SIZE //(2048)
#define FHOST_CNTRL_STACK_SIZE  (4096)
#define FHOST_WPA_STACK_SIZE    (4096)
#define FHOST_TX_STACK_SIZE     configMINIMAL_STACK_SIZE //(2048)
#define FHOST_RX_STACK_SIZE     configMINIMAL_STACK_SIZE //(2048)
#define CLI_CMD_STACK_SIZE      (2048)
#define RWNX_TIMER_STACK_SIZE   (2048)
#define RWNX_APM_STALOSS_STACK_SIZE (2048)

extern uint32_t sdio_datrx_stack_size;
extern uint32_t fhost_cntrl_stack_size;
extern uint32_t fhost_wpa_stack_size;
extern uint32_t fhost_tx_stack_size;
extern uint32_t fhost_rx_stack_size;
extern uint32_t cli_cmd_stack_size;
extern uint32_t rwnx_timer_stack_size;
extern uint32_t rwnx_apm_staloss_stack_size;

/*============Console setting===================*/
void hal_print(char *fmt, ...);

#define FFLUSH(x) fflush(x)

#define hal_putchar(ch) putchar(ch)
extern uint8_t hal_getchar(void);
#define ssv_inline __inline

/*============Compiler setting===================*/
#undef SSV_PACKED_STRUCT_BEGIN
#define SSV_PACKED_STRUCT_BEGIN
#define SSV_PACKED_STRUCT
#define SSV_PACKED_STRUCT_END
#define SSV_PACKED_STRUCT_STRUCT    __attribute__ ((packed))
#define SSV_PACKED_STRUCT_FIELD

#define UNION_PACKED
#define ALIGN_ARRAY(a) __attribute__((aligned(a)))

/*============Functions=============================*/
void platform_pwr_en_pin_init(void);
void platform_pwr_en_pin_set(bool en);
void platform_udelay(uint32_t us_delay);
bool platform_download_firmware(void);
#endif
