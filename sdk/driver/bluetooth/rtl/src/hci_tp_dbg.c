/**
 * Copyright (C) 2017 Realtek Semiconductor Corporation
 * Author: Alex Lu <alex_lu@realsil.com.cn>
 */
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include "bt_board.h"
#include "hci_tp_dbg.h"

// =========================debug===========
static uint32_t debug_level;
static char     print_buffer[1024];

/****************************************************************************/

/* Check if in task context (true), or isr context (false)                  */

/****************************************************************************/
int
tp_osif_printf (
    const char * fmt,
    ...)
{
    int     ret;

    va_list args;
    va_start(args, fmt);
    ret = vsnprintf(print_buffer, 1024, fmt, args);
#ifdef HCI_STACK_PRINT_EANBLE
    (void)printf("%s", print_buffer);
#endif
    va_end(args);

    return ret;
}

int
hci_tp_warn (
    const char * fmt,
    ...)
{
    if (CHECK_SW(HCI_TP_DEBUG_WARN))
    {
        int     ret;

        va_list args;
        va_start(args, fmt);
        ret = vsnprintf(print_buffer, 1024, fmt, args);
#ifdef HCI_STACK_PRINT_EANBLE
        (void)printf("%s", print_buffer);
#endif
        va_end(args);

        return ret;
    }
    else
    {
        return 0;
    }
}

int
hci_tp_dbg (
    const char * fmt,
    ...)
{
    if (CHECK_SW(HCI_TP_DEBUG_DEBUG))
    {
        int     ret;

        va_list args;
        va_start(args, fmt);
        ret = vsnprintf(print_buffer, 1024, fmt, args);
#ifdef HCI_STACK_PRINT_EANBLE
        (void)printf("%s", print_buffer);
#endif
        va_end(args);

        return ret;
    }
    else
    {
        return 0;
    }
}

int
hci_tp_info (
    const char * fmt,
    ...)
{
    if (CHECK_SW(HCI_TP_DEBUG_INFO))
    {
        int     ret;

        va_list args;
        va_start(args, fmt);
        ret = vsnprintf(print_buffer, 1024, fmt, args);
#ifdef HCI_STACK_PRINT_EANBLE
        (void)printf("%s", print_buffer);
#endif
        va_end(args);

        return ret;
    }
    else
    {
        return 0;
    }
}

int
hci_tp_hci_stack_dbg (
    const char * fmt,
    ...)
{
    if (CHECK_SW(HCI_TP_DEBUG_HCI_STACK_DEBUG))
    {
        int     ret;

        va_list args;
        va_start(args, fmt);
        ret = vsnprintf(print_buffer, 1024, fmt, args);
#ifdef HCI_STACK_PRINT_EANBLE
        (void)printf("%s", print_buffer);
#endif
        va_end(args);

        return ret;
    }
    else
    {
        return 0;
    }
}
// =========================debug===========
static uint32_t debug_level;

void
hci_tp_debug_set_level (
    uint32_t level)
{
    debug_level = level;
    if (debug_level != 0)
    {
        tp_osif_printf("debug_level:%x\r\n", debug_level);
    }
}

uint32_t
hci_tp_debug_get_level (
    void)
{
    return debug_level;
}
