/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _BTMTK_SDIO_H_
#define _BTMTK_SDIO_H_

#include "btmtk_define.h"
#include "btmtk_skbuff.h"
#include "openrtos/FreeRTOS.h"
#include "openrtos/task.h"
#include "openrtos/semphr.h"
#include "openrtos/event_groups.h"
#include "malloc.h"

#ifndef SDIO_VENDOR_ID_MEDIATEK
#define SDIO_VENDOR_ID_MEDIATEK 0x037A
#endif

#define BTMTK_SDIO_FUNC 2

#define SDIO_RW_RETRY_COUNT 20

#define SDIO_BLOCK_SIZE 512

/* CHLPCR */
#define C_FW_INT_EN_SET			0x00000001
#define C_FW_INT_EN_CLEAR		0x00000002

/* common register address */
#define CCIR		0x0000
#define CHLPCR		0x0004
#define CSDIOCSR	0x0008
#define CHCR		0x000C
#define CHISR		0x0010
#define CHIER		0x0014
#define CTDR		0x0018
#define CRDR		0x001C
#define CTFSR		0x0020
#define CRPLR		0x0024
#define CSICR		0x00C0
#define PD2HRM0R	0x00DC
#define SWPCDBGR	0x0154
#define PH2DSM0R	0x00C4

/* Driver & FW own related */
#define DRIVER_OWN 0
#define FW_OWN 1
#define SET_OWN_LOOP_COUNT 10

/* PD2HRM0R */
#define PD2HRM0R_DRIVER_OWN		0x00000001

/* MCU notify host dirver for L0.5 reset */
#define FIRMWARE_INT_BIT31		0x80000000

/* MCU notify host driver for coredump */
#define FIRMWARE_INT_BIT15		0x00008000
#define TX_FIFO_OVERFLOW		0x00000100
#define FW_INT_IND_INDICATOR	0x00000080
#define TX_COMPLETE_COUNT		0x00000070
#define TX_UNDER_THOLD			0x00000008
#define TX_EMPTY				0x00000004
#define RX_DONE					0x00000002
#define FW_OWN_BACK_INT			0x00000001

#define FIRMWARE_INT			0x0000FE00

#define URB_MAX_BUFFER_SIZE	(4*1024)

/* MCU address offset */
#define MCU_ADDRESS_OFFSET_CMD 12
#define MCU_ADDRESS_OFFSET_EVT 16

/* CMD&Event sent by driver */
#define READ_REGISTER_CMD_LEN		16
#define READ_REGISTER_EVT_HDR_LEN		11

#define WOBLE_COMP_EVENT_TIMO	5000

/* PH2DSM0R*/
#define PH2DSM0R_DRIVER_OWN		0x00000001

/* CHISR */
#define RX_PKT_LEN				0xFFFF0000

#define BTMTK_SDIO_THREAD_STOP	(1 << 0)
#define BTMTK_SDIO_THREAD_TX		(1 << 1)
#define BTMTK_SDIO_THREAD_RX		(1 << 2)
#define BTMTK_SDIO_THREAD_FW_OWN	(1 << 3)

#define CHECK_THREAD_RETRY_TIMES 50

struct btmtk_sdio_thread {
	TaskHandle_t task;
	SemaphoreHandle_t wait_semaphore;
	EventGroupHandle_t xEventGroup;
	void *priv;
	atomic_t thread_status;
};


struct btmtk_sdio_dev {
	struct sdio_func *func;
	struct btmtk_dev *bdev;

	bool patched;
	bool no_fw_own;
	atomic_t int_count;
	atomic_t tx_rdy;

	/* TODO, need to confirm the max size of urb data, also need to confirm
	 * whether intr_complete and bulk_complete and soc_complete can all share
	 * this urb_transfer_buf
	 */
	unsigned char	*transfer_buf;
	unsigned char	*sdio_packet;
	RTB_QUEUE_HEAD  *tx_queue;
	struct btmtk_sdio_thread sdio_thread;
#if 0 // ITE_MT7921_SDIO_BT_WIP
	
	struct btmtk_woble bt_woble;

	struct timer_list fw_own_timer;
	atomic_t fw_own_timer_flag;

	struct wakeup_source *main_ws;	//for main thread
	struct wakeup_source *irq_ws;	//for irq handler
	struct wakeup_source *device_ws;	//for probe and disconnect procedure
#endif
};

#define FW_ASSERT_CMD_LEN 4
#define FW_ASSERT_CMD1_LEN 9
#define NOTIFY_ALT_EVT_LEN 7

#define BLE_EVT_TYPE 0x3E


#define MTK_SDIO_PACKET_HEADER_SIZE 4
#define WOBLE_DEBUG_EVT_TYPE 0xE8


int btmtk_sdio_read_register(struct btmtk_dev *bdev, u32 reg, u32 *val);
int btmtk_sdio_event_filter(struct btmtk_dev *bdev, sk_buff *skb);
int btmtk_sdio_send_and_recv(struct btmtk_dev *bdev, sk_buff *skb,
		const uint8_t *event, const int event_len,
		int delay, int retry, int pkt_type, bool flag);

#define SDIO_IO_DELAY 2

int btmtk_sdio_load_fw_patch_using_dma(struct btmtk_dev *bdev, u8 *image,
		u8 *fwbuf, int section_dl_size, int section_offset);
int mmpMtkBtDriverRegister(void);
#if 0 // ITE_MT7921_SDIO_BT_WIP

























#include "btmtk_main.h"

#include "btmtk_woble.h"
#include "btmtk_chip_reset.h"

#ifndef BTMTK_SDIO_DEBUG
#define BTMTK_SDIO_DEBUG 0
#endif

#define HCI_HEADER_LEN	4

#define MTK_STP_TLR_SIZE	2
#define STP_HEADER_LEN	4
#define STP_HEADER_CRC_LEN	2
#define HCI_MAX_COMMAND_SIZE	255






#define PD2HRM0R_FW_OWN		0x00000000





/* wifi CR */
#define CONDBGCR		0x0034
#define CONDBGCR_SEL		0x0040
#define SDIO_CTRL_EN		(1 << 31)
#define WM_MONITER_SEL		(~(0x40000000))
#define PC_MONITER_SEL		(~(0x20000000))
#define PC_IDX_SWH(val, idx)	((val & (~(0x3F << 16))) | ((0x3F & idx) << 16))

typedef int (*pdwnc_func) (u8 fgReset);
typedef int (*reset_func_ptr2) (unsigned int gpio, int init_value);
typedef int (*set_gpio_low)(u8 gpio);
typedef int (*set_gpio_high)(u8 gpio);

/**
 * Send cmd dispatch evt
 */
#define HCI_EV_VENDOR			0xff










//#define READ_ADDRESS_EVT_HDR_LEN 7





// #define LD_PATCH_EVT_LEN 8



/*Extend to 60ms to prohibit set fw own while iso streaming. Or iso TX/RX may pass anchor interval as set FW own takes long time.
And also FW may assert due to queue full since driver cannot read data in time*/
#define FW_OWN_TIMEOUT		60
#define FW_OWN_TIMER_INIT	0
#define FW_OWN_TIMER_RUNNING	1



struct btmtk_sdio_hdr {
	/* For SDIO Header */
	__le16	len;
	__le16	reserved;
	/* For hci type */
	u8	bt_type;
} __packed;

int btmtk_sdio_read_bt_mcu_pc(u32 *val);
int btmtk_sdio_read_conn_infra_pc(u32 *val);
int btmtk_sdio_set_driver_own_for_subsys_reset(int enable);
int btmtk_sdio_whole_reset(struct btmtk_dev *bdev);
int btmtk_sdio_read_wifi_mcu_pc(u8 PcLogSel, u32 *val);

#endif

#endif
