/**
 * Copyright (C) 2017 Realtek Semiconductor Corporation
 * Author: Alex Lu <alex_lu@realsil.com.cn>
 */
#include <stdio.h>
#include <stdint.h>
#ifndef _HCI_TP_DBG_H
#define _HCI_TP_DBG_H
#include <stdint.h> /* READ COMMENT ABOVE. */

#ifdef __cplusplus
extern "C" {
#endif

//#define CSM_DEBUG

//=========================debug===========
#define HCI_TP_DEBUG
#if 0
#ifdef HCI_TP_DEBUG
#define hci_tp_dbg(fmt, ...) \
    tp_osif_printf("%s:%d(dbg) "fmt"\n", __func__, __LINE__, ##__VA_ARGS__)
#define hci_tp_info(fmt, ...) \
    tp_osif_printf("%s:%d(info) "fmt"\n", __func__, __LINE__, ##__VA_ARGS__)
#define hci_tp_warn(fmt, ...) \
    tp_osif_printf("%s:%d(warn) "fmt"\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#define hci_tp_dbg(fmt, ...) do { ; } while (0)
#define hci_tp_info(fmt, ...) do { ; } while (0)
#define hci_tp_warn(fmt, ...) do { ; } while (0)
#endif
#endif

typedef enum
{
    HCI_TP_DEBUG_ERROR,
    HCI_TP_DEBUG_WARN,
    HCI_TP_DEBUG_INFO,
    HCI_TP_DEBUG_DEBUG,
    HCI_TP_DEBUG_HCI_UART_TX,
    HCI_TP_DEBUG_HCI_UART_RX,
    HCI_TP_DEBUG_HCI_UART_RX_IDX,
    HCI_TP_DEBUG_DOWNLOAD_PATCH,
    HCI_TP_DEBUG_HCI_STACK_DEBUG,
} T_HCI_TP_DEBUG_LEVEL;

#define H_BIT(x)    1<<x
#define HCI_TP_DEBUG_ALL   H_BIT(HCI_TP_DEBUG_DEBUG)|H_BIT(HCI_TP_DEBUG_INFO)|H_BIT(HCI_TP_DEBUG_WARN)| H_BIT(HCI_TP_DEBUG_ERROR) //0xFFFFFFFF

//must
#define hci_tp_err(fmt, ...) \
    tp_osif_printf("%s:%d(err) "fmt"\n", __func__, __LINE__, ##__VA_ARGS__)


int tp_osif_printf(const char *fmt, ...);

void hci_tp_debug_set_level(uint32_t level);
uint32_t hci_tp_debug_get_level(void);

#define CHECK_SW(x)                  (hci_tp_debug_get_level() & (1<<x))


#ifdef CSM_DEBUG
#define hci_tp_dbg(fmt, ...); \
    do { \
        if (CHECK_SW(HCI_TP_DEBUG_DEBUG)) \
            tp_osif_printf("%s:%d(warn) "fmt"\n",__FILE__, __LINE__, ##__VA_ARGS__); \
    }while(0)
#define hci_tp_info(fmt, ...); \
    do { \
        if (CHECK_SW(HCI_TP_DEBUG_INFO)) \
            tp_osif_printf("%s:%d(warn) "fmt"\n",__FILE__, __LINE__, ##__VA_ARGS__); \
    }while(0)
#define hci_tp_warn(fmt, ...); \
    do { \
        if (CHECK_SW(HCI_TP_DEBUG_WARN)) \
            tp_osif_printf("%s:%d(warn) "fmt"\n",__FILE__, __LINE__, ##__VA_ARGS__); \
    }while(0)
#define hci_tp_hci_stack_dbg(fmt, ...); \
    do { \
        if (CHECK_SW(HCI_TP_DEBUG_HCI_STACK_DEBUG)) \
            tp_osif_printf("%s:%d(warn) "fmt"\n",__FILE__, __LINE__, ##__VA_ARGS__); \
    }while(0)
#else
int hci_tp_dbg(const char *fmt, ...);
int hci_tp_info(const char *fmt, ...);
int hci_tp_warn(const char *fmt, ...);
int hci_tp_hci_stack_dbg(const char *fmt, ...);

#endif
#ifdef __cplusplus
}
#endif

#endif /* _HCI_TP_DBG_H */


