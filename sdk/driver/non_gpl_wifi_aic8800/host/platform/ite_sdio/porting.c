/*
 * Copyright (C) 2018-2020 AICSemi Ltd.
 *
 * All Rights Reserved
 */

#define _PORTING_C_
#include <porting.h>

#define APP_INCLUDE_WIFI_FW

/*============Task Priority===================*/
uint32_t sdio_datrx_priority = SDIO_DATRX_PRIORITY;
uint32_t fhost_cntrl_priority = FHOST_CNTRL_PRIORITY;
uint32_t fhost_wpa_priority = FHOST_WPA_PRIORITY;
uint32_t fhost_tx_priority = FHOST_TX_PRIORITY;
uint32_t fhost_rx_priority = FHOST_RX_PRIORITY;
uint32_t cli_cmd_priority = CLI_CMD_PRIORITY;
uint32_t rwnx_timer_priority = RWNX_TIMER_PRIORITY;
uint32_t rwnx_apm_staloss_priority = RWNX_APM_STALOSS_PRIORITY;
uint32_t tcpip_priority = TCPIP_PRIORITY;
uint32_t task_end_prio = TASK_END_PRIO;
uint32_t aic_priority_mode = AIC_PRIORITY_MODE;

/*============Stack Size (unint: 16bytes)===================*/
uint32_t sdio_datrx_stack_size = SDIO_DATRX_STACK_SIZE;
uint32_t fhost_cntrl_stack_size = FHOST_CNTRL_STACK_SIZE;
uint32_t fhost_wpa_stack_size = FHOST_WPA_STACK_SIZE;
uint32_t fhost_tx_stack_size = FHOST_TX_STACK_SIZE;
uint32_t fhost_rx_stack_size = FHOST_RX_STACK_SIZE;
uint32_t cli_cmd_stack_size = CLI_CMD_STACK_SIZE;
uint32_t rwnx_timer_stack_size = RWNX_TIMER_STACK_SIZE;
uint32_t rwnx_apm_staloss_stack_size = RWNX_APM_STALOSS_STACK_SIZE;

/*=============console==================*/
uint8_t hal_getchar(void)
{
    uint8_t data=0;
    data = getchar();
    return data;
}

#define PRINT_BUF_SIZE                  512
static char print_buf[PRINT_BUF_SIZE];
static char print_buf_2[PRINT_BUF_SIZE+24];

void hal_print(char *fmt, ...)
{
    va_list v_list;
    int32_t ret;
    va_start(v_list, fmt);
    ret = vsnprintf(print_buf, PRINT_BUF_SIZE, fmt, v_list);
    if(ret < 0)
    {
        return;
    }
    va_end(v_list);

    print_buf[PRINT_BUF_SIZE-1] = 0;
    print_buf_2[0] = 0;
    strcat(print_buf_2, "#AIC# ");
    //add by wyl 20210909 for download assert
    if ((strlen(print_buf)>2) && (((print_buf[0] == '\r') && (print_buf[1] == '\n'))||((print_buf[1] == '\r') && (print_buf[0] == '\n'))))
    //end by wyl 20210909 for download assert
    {
        strcat(print_buf_2, print_buf+2);
    }
    else
    {
        strcat(print_buf_2, print_buf);
    }
    printf("%s", print_buf_2);
}


//=====================Platform LDO EN ping setting=======================
#ifdef CONFIG_AIC_PWR_EN_PINNUM
#define AIC_PWR_EN_PINNUM   CONFIG_AIC_PWR_EN_PINNUM
#endif

void platform_pwr_en_pin_init(void)
{
#ifdef CONFIG_AIC_PWR_EN_PINNUM
    hal_print("AIC_PWR_EN_PINNUM is %d\n", AIC_PWR_EN_PINNUM);
    GPIO_Enable(AIC_PWR_EN_PINNUM);
    GPIO_SetDirection(AIC_PWR_EN_PINNUM, 1);
#else
    hal_print("AIC_PWR_EN_PINNUM undefined\n");
#endif
}

void platform_pwr_en_pin_set(bool en)
{
#ifdef CONFIG_AIC_PWR_EN_PINNUM
    GPIO_SetValue(AIC_PWR_EN_PINNUM, en);
#endif
}

