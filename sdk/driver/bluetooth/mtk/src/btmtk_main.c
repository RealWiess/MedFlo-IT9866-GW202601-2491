/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "btmtk_define.h"
#include "btmtk_main.h"
#include "btmtk_skbuff.h"
#include "btmtk_sdio.h"

#include <malloc.h>
#include <string.h>
#include <stdio.h>

/*btmtk main information*/
static struct btmtk_main_info main_info;

static btmtk_fwlog_recv_cb fwlog_recv_cb;
static bt_state_change_cb state_change_cb;
static bt_rx_data_ready_cb curr_data_ready_cb;
static bt_rx_data_ready_cb last_data_ready_cb;
xSemaphoreHandle data_ready_cb_mutex;

struct btmtk_main_info *btmtk_get_main_info(void)
{
	return &main_info;
}

uint8_t btmtk_log_lvl = BTMTK_LOG_LVL_DEF;

/* To support dynamic mount of interface can be probed */
static int btmtk_intf_num = BT_MCU_MINIMUM_INTERFACE_NUM;
static struct btmtk_dev **g_bdev;

/* State machine table that clarify through each HIF events,
 * To specify HIF event on
 * Entering / End / Error
 */
static const struct btmtk_cif_state g_cif_state[] = {
	/* HIF_EVENT_PROBE */
	{BTMTK_STATE_PROBE, BTMTK_STATE_WORKING, BTMTK_STATE_DISCONNECT},
	/* HIF_EVENT_DISCONNECT */
	{BTMTK_STATE_DISCONNECT, BTMTK_STATE_DISCONNECT, BTMTK_STATE_DISCONNECT},
	/* HIF_EVENT_SUSPEND */
	{BTMTK_STATE_SUSPEND, BTMTK_STATE_SUSPEND, BTMTK_STATE_FW_DUMP},
	/* HIF_EVENT_RESUME */
	{BTMTK_STATE_RESUME, BTMTK_STATE_WORKING, BTMTK_STATE_FW_DUMP},
	/* HIF_EVENT_STANDBY */
	{BTMTK_STATE_STANDBY, BTMTK_STATE_STANDBY, BTMTK_STATE_FW_DUMP},
	/* HIF_EVENT_SUBSYS_RESET */
	{BTMTK_STATE_SUBSYS_RESET, BTMTK_STATE_WORKING, BTMTK_STATE_FW_DUMP},
	/* HIF_EVENT_WHOLE_CHIP_RESET */
	{BTMTK_STATE_FW_DUMP, BTMTK_STATE_DISCONNECT, BTMTK_STATE_FW_DUMP},
	/* HIF_EVENT_FW_DUMP */
	{BTMTK_STATE_FW_DUMP, BTMTK_STATE_FW_DUMP, BTMTK_STATE_FW_DUMP},
};

void spinLockInit(SpinLock *lock) {
    lock->ownerTask = NULL;
    lock->lockCount = 0;
}

void spinLockAcquire(SpinLock *lock) {
    TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
	BTMTK_INFO("%s", __func__);
    if (lock->ownerTask == currentTask) {
        lock->lockCount++;
		BTMTK_INFO("%s count++", __func__);
    } else {
        while (xTaskNotifyWait(0, 0, NULL, portMAX_DELAY) != pdTRUE) {
            BTMTK_INFO("%s", __func__);
        }
        lock->ownerTask = currentTask;
        lock->lockCount = 1;
    }
}

void spinLockRelease(SpinLock *lock) {
    TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
    if (lock->ownerTask == currentTask) {
        lock->lockCount--;
		BTMTK_INFO("%s count--", __func__);
        if (lock->lockCount == 0) {
            lock->ownerTask = NULL;
			BTMTK_INFO("%s", __func__);
            xTaskNotifyGive(currentTask);
        }
    }
}

SemaphoreHandle_t CHIP_STATE_MUTEX;
SemaphoreHandle_t FOPS_MUTEX;

int btmtk_init_mutex()
{
	int state = 0;
	BTMTK_INFO("%s", __func__);
	CHIP_STATE_MUTEX = xSemaphoreCreateMutex();
	if (CHIP_STATE_MUTEX == NULL){
		BTMTK_INFO("%s CHIP_STATE_MUTEX init fail", __func__);
		return -1;
	}
	
	FOPS_MUTEX = xSemaphoreCreateMutex();
	if (FOPS_MUTEX == NULL){
		BTMTK_INFO("%s FOPS_MUTEX init fail", __func__);
		return -1;
	}

	return state;
}

uint32_t atomic_read(volatile uint32_t *ptr)
{
	uint32_t value;

	ATOMIC_ENTER_CRITICAL();
    value = *ptr;
    ATOMIC_EXIT_CRITICAL();
    
    return value;
}

uint32_t atomic_set(volatile uint32_t *ptr, uint32_t value)
{

	ATOMIC_ENTER_CRITICAL();
    *ptr = value;
    ATOMIC_EXIT_CRITICAL();
    //alway return success
    return 1;
}

int btmtk_get_chip_state(struct btmtk_dev *bdev)
{
	int state = 0;
	
 	//BTMTK_INFO("Enter %s", __func__);
	
	if (!bdev) {
		 BTMTK_ERR("%s, invalid parameters!", __func__);
		 return state;
	}
 
	//CHIP_STATE_MUTEX_LOCK();
	if (xSemaphoreTake(CHIP_STATE_MUTEX, portMAX_DELAY) == pdTRUE)
	{
		state = bdev->interface_state;
		xSemaphoreGive(CHIP_STATE_MUTEX);
	} else {
		BTMTK_ERR("%s, xSemaphoreTake fail!", __func__);
		return state;
	}
	//CHIP_STATE_MUTEX_UNLOCK();
 	//BTMTK_INFO("Exit %s (%d)", __func__,state);
	return state;
}

void btmtk_set_chip_state(struct btmtk_dev *bdev, u8 new_state)
{
	static const char * const state_msg[BTMTK_STATE_MSG_NUM] = {
		"UNKNOWN", "INIT", "DISCONNECT", "PROBE", "WORKING", "SUSPEND", "RESUME",
		"FW_DUMP", "STANDBY", "SUBSYS_RESET", "SEND_ASSERT",
	};
	//BTMTK_INFO("Enter %s", __func__);
	if (!bdev) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		return;
	}

	if (new_state >= BTMTK_STATE_MSG_NUM) {
		BTMTK_INFO("%s: new_state invalid(%d)", __func__, new_state);
		return;
	}

	BTMTK_INFO("%s: %s(%d) -> %s(%d)", __func__, state_msg[bdev->interface_state],
			bdev->interface_state, state_msg[new_state], new_state);

	// CHIP_STATE_MUTEX_LOCK();
	if (xSemaphoreTake(CHIP_STATE_MUTEX, portMAX_DELAY) == pdTRUE)
	{
		bdev->interface_state = new_state;
		xSemaphoreGive(CHIP_STATE_MUTEX);
	} else {
		BTMTK_ERR("%s, xSemaphoreTake fail!", __func__);
	}
	// CHIP_STATE_MUTEX_UNLOCK();
	//BTMTK_INFO("Exit %s", __func__);
}

u8 btmtk_fops_get_state(struct btmtk_dev *bdev)
{
	u8 state = 0;
	//BTMTK_INFO("Enter %s", __func__);
	if (!bdev) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		return state;
	}

	//FOPS_MUTEX_LOCK();
	if (xSemaphoreTake(FOPS_MUTEX, portMAX_DELAY) == pdTRUE)
	{
		state = bdev->fops_state;
		xSemaphoreGive(FOPS_MUTEX);
	} else {
		BTMTK_ERR("%s, xSemaphoreTake fail!", __func__);
	}
	//FOPS_MUTEX_UNLOCK();
	//BTMTK_INFO("Exit %s", __func__);
	return state;
}

static void btmtk_fops_set_state(struct btmtk_dev *bdev, u8 new_state)
{
	static const char * const fstate_msg[BTMTK_FOPS_STATE_MSG_NUM] = {
		"UNKNOWN", "INIT", "OPENING", "OPENED", "CLOSING", "CLOSED",
	};

	if (!bdev) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		return;
	}

	if (new_state >= BTMTK_FOPS_STATE_MSG_NUM) {
		BTMTK_INFO("%s: new_state invalid(%d)", __func__, new_state);
		return;
	}

	BTMTK_INFO("%s: FOPS_%s(%d) -> FOPS_%s(%d)", __func__, fstate_msg[bdev->fops_state],
			bdev->fops_state, fstate_msg[new_state], new_state);
	//FOPS_MUTEX_LOCK();
	if (xSemaphoreTake(FOPS_MUTEX, portMAX_DELAY) == pdTRUE)
	{
		bdev->fops_state = new_state;
		xSemaphoreGive(FOPS_MUTEX);
	} else {
		BTMTK_ERR("%s, xSemaphoreTake fail!", __func__);
	}
	//FOPS_MUTEX_UNLOCK();
}

/*sdio need implement this function*/
__attribute__((weak)) int btmtk_cif_register(void)
{
	BTMTK_WARN("weak function %s not implement", __func__);
	return -1;
}

/*sdio need implement this function*/
__attribute__((weak)) int btmtk_cif_deregister(void)
{
	BTMTK_WARN("weak function %s not implement", __func__);
	return -1;
}

#define TEST_PORT       ITP_DEVICE_UART1
#define TEST_ITH_PORT	ITH_UART1
#define TEST_DEVICE     itpDeviceUart1
#define TEST_BAUDRATE   CFG_UART1_BAUDRATE
#define TEST_PARITY		CFG_UART1_PARITY
#define TEST_GPIO_RX    CFG_GPIO_UART1_RX
#define TEST_GPIO_TX    CFG_GPIO_UART1_TX

void btmtk_util_dump_buffer(const char *label, const unsigned char *buf,
				unsigned int length, unsigned char force_print)
{

}

#if 0 
void timer_isr(void* data)
{
    uint32_t timer = (uint32_t)data;
    ithTimerClearIntr(timer);
	sem_post(&UartSemDma);
}

static void UartIntrHandler(unsigned int pin, void *arg)
{
	ITHTimer timer = (ITHTimer)arg;
	//ithPrintf("timer=%d\n", timer);
	ithGpioClearIntr(pin);
	ithTimerCtrlDisable(timer, ITH_TIMER_EN);	
	ithTimerSetTimeout(timer, 1000);//us
	ithTimerCtrlEnable(timer, ITH_TIMER_EN);	
}

void InitUartIntr(ITHUartPort port)
{
	ithEnterCritical();

    ithGpioClearIntr(TEST_GPIO_RX);
    ithGpioSetIn(TEST_GPIO_RX);
    ithGpioRegisterIntrHandler((unsigned int)TEST_GPIO_RX, UartIntrHandler, (void*)ITH_TIMER5);
	
    ithGpioCtrlDisable(TEST_GPIO_RX, ITH_GPIO_INTR_LEVELTRIGGER);   /* use edge trigger mode */
    ithGpioCtrlEnable(TEST_GPIO_RX, ITH_GPIO_INTR_BOTHEDGE); /* both edge */    
    ithIntrEnableIrq(ITH_INTR_GPIO);
    ithGpioEnableIntr(TEST_GPIO_RX);

    ithExitCritical();
}

void InitTimer(ITHTimer timer, ITHIntr intr)
{
	ithTimerReset(timer);
	
	// Initialize Timer IRQ
	ithIntrDisableIrq(intr);
	ithIntrClearIrq(intr);

	// register Timer Handler to IRQ
	ithIntrRegisterHandlerIrq(intr, timer_isr, (void*)timer);

	// set Timer IRQ to edge trigger
	ithIntrSetTriggerModeIrq(intr, ITH_INTR_EDGE);

	// set Timer IRQ to detect rising edge
	ithIntrSetTriggerLevelIrq(intr, ITH_INTR_HIGH_RISING);

	// Enable Timer IRQ
	ithIntrEnableIrq(intr);
}
#endif

/*used for colloect picus log and firmware coredump*/
void* bt_uart_init(int port, int speed)
{

#if 0
	int len = 0, count = 0;
	char getstr[256] = "", sendstr[256] = "";

	UART_OBJ *pUartInfo	= (UART_OBJ*)malloc(sizeof(UART_OBJ));
	pUartInfo->port		= TEST_ITH_PORT;
	pUartInfo->parity	= TEST_PARITY;
	pUartInfo->txPin	= TEST_GPIO_TX;
	pUartInfo->rxPin	= TEST_GPIO_RX;
	pUartInfo->baud		= TEST_BAUDRATE;
	pUartInfo->timeout	= 0;
	pUartInfo->mode		= UART_DMA_MODE;
	pUartInfo->forDbg	= false;

	itpRegisterDevice(TEST_PORT, &TEST_DEVICE);
	ioctl(TEST_PORT, ITP_IOCTL_INIT, (void*)pUartInfo);
	//ioctl(TEST_PORT, ITP_IOCTL_RESET, (void*)pUartInfo);
	
	InitTimer(ITH_TIMER5, ITH_INTR_TIMER5);
	InitUartIntr(TEST_PORT);

	printf("Start uart DMA mode test!\n");

	sem_init(&UartSemDma, 0, 0);

	while(1)
	{
		sem_wait(&UartSemDma);
		len = read(TEST_PORT, getstr + count, 256);
		printf("len = %d, getstr = %s\n", len, getstr);
	    count += len;

	    if(count >= UartCommandLen)
	    {
			printf("uart read: %s,count=%d\n", getstr, count);
			memcpy(sendstr, getstr, count);
			sendstr[count] = '\n';
			write(TEST_PORT, sendstr, count + 1);

			memset(getstr, 0, count + 1);
			memset(sendstr, 0, count + 1);
			count = 0;
			ioctl(TEST_PORT, ITP_IOCTL_RESET, (void*)pUartInfo);
			printf("reset!!\n");
	    }
	}
#endif
}

uint32_t bt_uart_write(int fd, unsigned char *buf, int len)
{

    uint32_t ret = 0;
    int write_len = 0;
    uint32_t retry = 10;
    do {
        if (write_len)
            vTaskDelay(pdMS_TO_TICKS(10)); //10ms
        ret = write(TEST_PORT,(const uint8_t *)buf + write_len, len - write_len);
        write_len += ret;
    } while (write_len < len && retry-- > 0);

    if (write_len == len) {
        //BTMTK_INFO("bt_uart_write send success, write_len = %d retry = %d!!!\n", write_len, retry);
    } else {
        BTMTK_INFO("bt_uart_write send failure, write_len = %d retry = %d!!!\n", write_len, retry);
    }

	//write(TEST_PORT, buf, len);
    return len;
}

int btmtk_debug_rx(unsigned char *buf, unsigned int buf_len)
{
    sk_buff *skb;
#if 0
    uint8_t *upload_buf = NULL;
#endif
    unsigned int copy_len;
    struct btmtk_dev *bdev = NULL;

    if ((buf == NULL) || (buf_len == 0)) {
        BTMTK_INFO("%s invalid arg", __func__);
        return -1;
    }

    bdev = btmtk_get_dev();
    if (!bdev) {
        BTMTK_ERR("%s: invalid parameters!", __func__);
        return -1;
    }
	//return -1; //ok

	if (xSemaphoreTake(data_ready_cb_mutex, portMAX_DELAY) == pdFALSE) {
		BTMTK_ERR("%s, data_ready_cb_mutex get failed", __func__);
		return -1;
	}else {
		skb = RtbDequeueHead(bdev->rx_q);
		BTMTK_INFO("%s, data_ready_cb_mutex get success ,dequeue skb:%p", __func__, &skb);
		xSemaphoreGive(data_ready_cb_mutex);
	}

    if (skb == NULL) {
        BTMTK_ERR("%s: RtbDequeueHead returned NULL", __func__);
        return -1;
    }
    BTMTK_INFO("%s RtbDequeueHead len:%d buf_head:%p, skb_type:%d", __func__, skb->len, &buf, skb_get_pkt_type(skb));
    // 计算要复制的长度
	//copy_len = (buf_len < (skb->len + 1)) ? buf_len : (skb->len + 1);
	copy_len = (buf_len < (skb->len)) ? buf_len : skb->len;
    BTMTK_INFO("%s will copy_len:%d, buf left:%d", __func__, copy_len, buf_len);
	// 将 TYPE 复制到 buf
	//memcpy(buf, skb_get_pkt_type(skb), 1);
    buf[0] = skb_get_pkt_type(skb);
	// 将 内容 复制到 buf
    //memcpy((uint8_t*)buf + 1, skb->data, copy_len-1);
	memcpy((uint8_t*)buf + 1, skb->data, copy_len);
	BTMTK_INFO_RAW(buf, copy_len+1, "%s:copy to buf ", __func__);
    return copy_len+1;
}

int bt_uart_deinit(int fd)
{
#if 0
#ifdef CHIP_MT7933
	hal_uart_status_t status_t = HAL_UART_STATUS_OK;

	if (g_inited_uart_port < 0)
		return status_t;

	if (fd == HAL_UART_0)
		return status_t;

	status_t = hal_uart_deinit(fd); //(HAL_UART_2);

#ifdef OPEN_UART_DMA_MODE
	if (g_vff_mem_addr) {
		// vPortFreeNC((unsigned int *)g_vff_mem_addr);
		SYS_FREE_NC((unsigned int *)g_vff_mem_addr);
		g_vff_mem_addr = 0;
	}
#endif

	g_inited_uart_port = -1;
	return status_t;
#else
	return 0;
#endif
#endif
	return 0;
}

void btmtk_enable_bperf(bool enable)
{
#if 0
	BTIF_LOG_I("%s: %s", __func__, enable == pdTRUE ? "True" : "False");
	bperf_enable = enable;
#endif
}

int btmtk_debug_tx(unsigned char *cmd, unsigned int length)
{

	int ret = 0;
	int state;
	sk_buff *skb = NULL;
	struct btmtk_dev *bdev;

	bdev = btmtk_get_dev();
	if (!bdev) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		ret = -EINVAL;
		goto exit;
	}

	state = btmtk_get_chip_state(bdev);
	if (state == BTMTK_STATE_FW_DUMP || state == BTMTK_STATE_SUSPEND || state == BTMTK_STATE_SEND_ASSERT) {
		BTMTK_WARN("%s: FW dumping already or in suspend state don't send assert, state = %d!!!",
			__func__, state);
		return ret;
	}

	BTMTK_INFO("%s: send debug cmd", __func__);

	skb = skb_alloc_and_init(HCI_COMMAND_PKT, (uint8_t *)cmd, length);
	if (!skb) {
		BTMTK_ERR("%s allocate skb failed!!", __func__);
		goto exit;
	}

	ret = main_info.hif_hook.send_cmd(bdev, skb, WMT_DELAY_TIMES, RETRY_TIMES,
			(int)BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
	if (ret < 0) {
		BTMTK_ERR("%s failed!!", __func__);
	} 

exit:
	return ret;

}

int btmtk_send_assert_cmd()
{
	int ret = 0;
	int state;
	u8 cmd[ASSERT_CMD_LEN] = { 0x01, 0x6F, 0xFC, 0x05, 0x01, 0x02, 0x01, 0x00, 0x08 };
	sk_buff *skb = NULL;
	struct btmtk_dev *bdev;

	bdev = btmtk_get_dev();
	if (!bdev) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		ret = -EINVAL;
		goto exit;
	}

	state = btmtk_get_chip_state(bdev);
	if (state == BTMTK_STATE_FW_DUMP || state == BTMTK_STATE_SUSPEND || state == BTMTK_STATE_SEND_ASSERT) {
		BTMTK_WARN("%s: FW dumping already or in suspend state don't send assert, state = %d!!!",
			__func__, state);
		return ret;
	}

	BTMTK_INFO("%s: send assert cmd", __func__);

	//skb = alloc_skb(ASSERT_CMD_LEN + BT_SKB_RESERVE, GFP_ATOMIC);
	skb = skb_alloc_and_init(HCI_COMMAND_PKT, (uint8_t *)cmd, ASSERT_CMD_LEN);
	if (!skb) {
		BTMTK_ERR("%s allocate skb failed!!", __func__);
		goto exit;
	}
	//btmtk_assert_wake_lock();
	//bt_cb(skb)->pkt_type = HCI_COMMAND_PKT;
	//memcpy(skb->data, cmd, ASSERT_CMD_LEN);
	//skb->len = ASSERT_CMD_LEN;

	ret = main_info.hif_hook.send_cmd(bdev, skb, WMT_DELAY_TIMES, RETRY_TIMES,
			(int)BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
	if (ret < 0) {
		BTMTK_ERR("%s failed!!", __func__);
		//kfree_skb(skb);
		//btmtk_reset_trigger(bdev);
	} else {
		//btmtk_reset_timer_update(bdev);
		BTMTK_INFO("%s: OK", __func__);
		btmtk_set_chip_state(bdev, BTMTK_STATE_SEND_ASSERT);
	}

exit:
	return ret;
}

void bt_close()
{
	struct btmtk_dev *bdev = NULL;
	BTMTK_INFO("%s", __func__);
	
	//1. get device
	bdev = btmtk_get_dev();
	if (!bdev) {
		BTMTK_ERR("%s: invalid parameters!", __func__);
		return;
	} 

	RtbQueueFree(bdev->rx_q);
	RtbQueueFree(bdev->fwlog_queue);
}

int btmtk_send_init_cmds(struct btmtk_dev *bdev)
{
	int ret = -1;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();

	if (!bdev) {
		BTMTK_ERR("%s: bdev is NULL !", __func__);
		goto exit;
	}

	BTMTK_INFO("%s", __func__);

	ret = btmtk_send_wmt_power_on_cmd(bdev);
	if (ret < 0) {
		if (bdev->power_state != BTMTK_DONGLE_STATE_POWER_ON) {
			BTMTK_ERR("%s, btmtk_reset_power_on failed!", __func__);
			if (main_info.reset_stack_flag == HW_ERR_NONE)
				main_info.reset_stack_flag = HW_ERR_CODE_POWER_ON;
		}
		goto exit;
	}

exit:
	return ret;
}

static int btmtk_write_pinmux_setting_7961(struct btmtk_dev *bdev, uint8_t *cmd,
		int cmd_len, const uint8_t *event, const int event_len, u32 value)
{
	int ret = 0;

	if (!bdev) {
		BTMTK_ERR("%s: bdev is NULL !", __func__);
		return -1;
	}
	BTMTK_INFO("%s begin, value=0x%08x", __func__, value);

	cmd[cmd_len -4] = (value & 0x000000FF);
	cmd[cmd_len -3] = ((value & 0x0000FF00) >> 8);
	cmd[cmd_len -2] = ((value & 0x00FF0000) >> 16);
	cmd[cmd_len -1] = ((value & 0xFF000000) >> 24);

	ret = btmtk_main_send_cmd(bdev,cmd, cmd_len, event, event_len,
			0, 0, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
	if (ret < 0) {
		BTMTK_ERR("%s: failed(%d)", __func__, ret);
		return ret;
	}

	BTMTK_INFO("%s exit,", __func__);
	return ret;
}

static int btmtk_read_pinmux_setting_7961(struct btmtk_dev *bdev, const uint8_t *cmd,
		const int cmd_len, const uint8_t *event, const int event_len, u32 *value)
{
	int ret = 0;

	if (!bdev) {
		BTMTK_ERR("%s: bdev is NULL !", __func__);
		return -1;
	}
	BTMTK_INFO("%s: enter ", __func__);

	ret = btmtk_main_send_cmd(bdev,cmd, cmd_len, event, event_len,
			0, 0, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
	if (ret < 0) {
		BTMTK_ERR("%s: failed(%d)", __func__, ret);
		return ret;
	}

	*value = (bdev->io_buf[READ_PINMUX_EVT_REAL_LEN - 1] << 24) +
			(bdev->io_buf[READ_PINMUX_EVT_REAL_LEN - 2] << 16) +
			(bdev->io_buf[READ_PINMUX_EVT_REAL_LEN - 3] << 8) +
			bdev->io_buf[READ_PINMUX_EVT_REAL_LEN - 4];

	BTMTK_INFO("%s: value=0x%08x", __func__, *value);
	return ret;
}

static int btmtk_set_audio_pin_mux(struct btmtk_dev *bdev) 
{
	char pin_addr[PINMUX_REG_NUM] = {0x50, 0x54};
	int ret = 0;
	u32 pinmux = 0;
	unsigned int i = 0;
	u8 read_pinmux_cmd[READ_PINMUX_CMD_LEN] = { 0x01, 0xD1, 0xFC, 0x04, 0x50, 0x50, 0x00, 0x70};
	u8 read_pinmux_event[READ_PINMUX_EVT_CMP_LEN] = { 0x04, 0x0E, 0x08, 0x01, 0xD1, 0xFC};

	u8 write_pinmux_cmd[WRITE_PINMUX_CMD_LEN] = { 0x01, 0xD0, 0xFC, 0x08, 0x50, 0x50, 0x00, 0x70,
											0x00, 0x10, 0x11, 0x01};
	u8 write_pinmux_event[WRITE_PINMUX_EVT_LEN] = { 0x04, 0x0E, 0x04, 0x01, 0xD0, 0xFC, 0x00 };


	BTMTK_INFO("%s: enter ", __func__);
	if (!bdev) {
		BTMTK_ERR("%s: bdev is NULL !", __func__);
		return -1;
	}

	for (i = 0; i < PINMUX_REG_NUM; i++){
		pinmux = 0;
		read_pinmux_cmd[AUDIO_PINMUX_SETTING_OFFSET] = pin_addr[i];
		write_pinmux_cmd[AUDIO_PINMUX_SETTING_OFFSET] = pin_addr[i];
		ret = btmtk_read_pinmux_setting_7961(bdev, read_pinmux_cmd, READ_PINMUX_CMD_LEN,
									read_pinmux_event, READ_PINMUX_EVT_CMP_LEN, &pinmux);
		if (ret) {
			BTMTK_ERR("%s: btmtk_read_pinmux_setting_7961 error(%d) !", __func__, ret);
			goto exit;
		}
		if (i == 0) {
			pinmux &= 0x00FFFFFF;
			pinmux |= 0x11000000;
		} else {
			pinmux &= 0x00FFF0F0;
			pinmux |= 0x00000101;
		}
		ret = btmtk_write_pinmux_setting_7961(bdev, write_pinmux_cmd, WRITE_PINMUX_CMD_LEN,
									write_pinmux_event, WRITE_PINMUX_EVT_LEN, pinmux);
		if (ret) {
			BTMTK_ERR("%s: btmtk_read_pinmux_setting_7961 error(%d) !", __func__, ret);
			goto exit;
		}

		pinmux = 0;
		ret = btmtk_read_pinmux_setting_7961(bdev, read_pinmux_cmd, READ_PINMUX_CMD_LEN,
									read_pinmux_event, READ_PINMUX_EVT_CMP_LEN, &pinmux);
		if (ret) {
			BTMTK_ERR("%s: btmtk_read_pinmux_setting_7961 error(%d) !", __func__, ret);
			goto exit;
		}

		BTMTK_INFO("%s: confirm pinmux register 0x%02x pinmux 0x%08x", __func__,
				write_pinmux_event[4], pinmux);
	}
exit:
	return ret;
}

static int btmtk_set_audio_slave(struct btmtk_dev *bdev)
{
	u8 cmd[AUDIO_SETTING_CMD_LEN] = { 0x01, 0x72, 0xFC, 0x04, 0x49, 0x00, 0x80, 0x00};
	u8 event[AUDIO_SETTING_EVT_LEN] = { 0x04, 0x0E, 0x04, 0x01, 0x72, 0xFC, 0x00 };
	int ret = 0;

	BTMTK_INFO("%s: enter ", __func__);
	if (!bdev) {
		BTMTK_ERR("%s: bdev is NULL !", __func__);
		return -1;
	}

	ret = btmtk_main_send_cmd(bdev,
			cmd, AUDIO_SETTING_CMD_LEN,
			event, AUDIO_SETTING_EVT_LEN,
			0, 0, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
	if (ret < 0) {
		BTMTK_ERR("%s: failed(%d)", __func__, ret);
		return ret;
	}
}

int btmtk_set_audio_setting(struct btmtk_dev *bdev)
{
	int ret = 0;

	ret = btmtk_set_audio_slave(bdev);
	if (ret < 0) {
		BTMTK_ERR("%s: btmtk_set_audio_setting fail", __func__);
		return ret;
	}

	ret = btmtk_set_audio_pin_mux(bdev);
	if (ret < 0) {
		BTMTK_ERR("%s: btmtk_set_audio_pin_mux fail", __func__);
		return ret;
	}
	return ret;
};


int bt_open()
{
	int ret = 0;
	int state = 0;
	unsigned char fstate = 0;
	struct btmtk_dev *bdev = NULL;

	BTMTK_INFO("%s: MTK BT Driver Version : %s", __func__, VERSION);

	//1. get device
	bdev = btmtk_get_dev();
	if (!bdev) {
		BTMTK_ERR("%s: invalid parameters!", __func__);
		return -EFAULT;
	}
	//2. check and init status
	state = btmtk_get_chip_state(bdev);
	if (state == BTMTK_STATE_INIT || state == BTMTK_STATE_DISCONNECT) {
		ret = -EAGAIN;
		goto failed;
	}

	if (state != BTMTK_STATE_WORKING && state != BTMTK_STATE_STANDBY) {
		BTMTK_WARN("%s: not in working state and standby state(%d).", __func__, state);
		ret = -ENODEV;
		goto failed;
	}

	fstate = btmtk_fops_get_state(bdev);
	if (fstate == BTMTK_FOPS_STATE_OPENED) {
		BTMTK_WARN("%s: fops opened!", __func__);
		ret = -EIO;
		goto failed;
	}

	if ((fstate == BTMTK_FOPS_STATE_CLOSING) ||
		(fstate == BTMTK_FOPS_STATE_OPENING)) {
		BTMTK_WARN("%s: fops open/close is on-going !", __func__);
		ret = -EAGAIN;
		goto failed;
	}

	btmtk_fops_set_state(bdev, BTMTK_FOPS_STATE_OPENING);

	//3.tell sido open
	if (main_info.hif_hook.open)
	{
		ret = main_info.hif_hook.open(bdev);
		if (ret < 0) {
			BTMTK_ERR("%s, cif_open failed", __func__);
			goto failed;
		}
	} else {
		BTMTK_ERR("TODO: Fix main_info.hif_hook.open = NULL");
	}

	//4.send init cmds: open
	ret = btmtk_send_init_cmds(bdev);
	if (ret < 0) {
		BTMTK_ERR("%s, btmtk_send_init_cmds failed", __func__);
		goto failed;
	}

	//5.tell sdio open done
	if (main_info.hif_hook.open_done)
		main_info.hif_hook.open_done(bdev);

	//6.update fops state
	btmtk_fops_set_state(bdev, BTMTK_FOPS_STATE_OPENED);
	if (state_change_cb)
		state_change_cb(pdTRUE);
	main_info.reset_stack_flag = HW_ERR_NONE;

	BTMTK_INFO("%s:Done", __func__);
	return 0;

failed:
	btmtk_fops_set_state(bdev, BTMTK_FOPS_STATE_CLOSED);
	if (state_change_cb)
		state_change_cb(pdFALSE);
	return ret;
}

uint16_t bt_send_frame(sk_buff *skb)
{
	//BTMTK_INFO("%s", __func__);
	uint16_t ret = 0;
	int state = 0;
	unsigned char fstate = 0;
	struct btmtk_dev *bdev = NULL;
	u8 reset_cmd[HCI_RESET_CMD_LEN] = { 0x01, 0x03, 0x0C, 0x00 };

	//1. get device
	bdev = btmtk_get_dev();
	if (bdev == NULL) {
		BTMTK_ERR("%s, bdev is invalid!", __func__);
		ret = -ENODEV;
		goto exit;
	}

	//TODO
	if (main_info.hif_hook.cif_mutex_lock)
		main_info.hif_hook.cif_mutex_lock(bdev);

	fstate = btmtk_fops_get_state(bdev);
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTMTK_WARN("%s: fops is not open yet(%d)!", __func__, fstate);
		ret = -ENODEV;
		goto exit;
	}

	state = btmtk_get_chip_state(bdev);
	if (state != BTMTK_STATE_WORKING) {
		BTMTK_WARN_LIMITTED("%s: chip state is %d.", __func__, state);
		if (state == BTMTK_STATE_DISCONNECT)
			ret = -ENODEV;
		else
			ret = -EAGAIN;
		goto exit;
	}

	if (bdev->power_state == BTMTK_DONGLE_STATE_POWER_OFF) {
		BTMTK_WARN("%s: dongle state already power off, do not write", __func__);
		ret = -EFAULT;
		goto exit;
	}

	if (main_info.reset_stack_flag) {
		BTMTK_WARN("%s: reset_stack_flag (%d)!", __func__, main_info.reset_stack_flag);
		ret = -EFAULT;
		goto exit;
	}

	BTMTK_INFO("%s: type:%d", __func__, skb_get_pkt_type(skb));
	//BTMTK_INFO_RAW(skb->data, skb->len, "%s: ", __func__);

	if (skb_get_pkt_type(skb) == HCI_COMMAND_PKT) {
#if CFG_SUPPORT_BT_AUDIO
		/*Enable hfp over i2s or pcm */
		if(bdev->get_hci_reset == 0){
			ret = btmtk_set_audio_setting(bdev);
			bdev->get_hci_reset = 0;
			if (ret < 0) {
				BTMTK_ERR("%s: btmtk_set_audio_setting fail", __func__);
				return ret;
			}
		}
#endif
			if (skb->len == HCI_RESET_CMD_LEN &&
					!memcmp(skb->data, reset_cmd, HCI_RESET_CMD_LEN))
				BTMTK_INFO("%s: got command: 0x03 0C 00 (HCI_RESET)", __func__);
	} 

	ret = main_info.hif_hook.send_cmd(bdev, skb, 0, 0,
				(int)BTMTK_TX_PKT_FROM_HOST, CMD_NO_NEED_FILTER);

	if (ret < 0) {
		BTMTK_ERR("%s failed!!", __func__);
		ret = 0;
	} else {
		//success send,return the length
		ret = skb->len;
	}

exit:
	//TODO
	//BTMTK_INFO("%s: Done", __func__);
	if (main_info.hif_hook.cif_mutex_unlock)
		main_info.hif_hook.cif_mutex_unlock(bdev);

	return ret;
}

void btmtk_register_fwlog_recv_cb(btmtk_fwlog_recv_cb cb)
{
	BTMTK_INFO("%s cb: %p", __func__, cb);
	fwlog_recv_cb = cb;
}

int bt_driver_register_event_cb(bt_rx_data_ready_cb cb, int restore_cb)
{
	unsigned char fstate = 0;
	struct btmtk_dev *bdev = NULL;

	bdev = btmtk_get_dev();
	if (bdev == NULL) {
		BTMTK_ERR("%s, bdev is invalid!", __func__);
		return -1;
	}

	fstate = btmtk_fops_get_state(bdev);
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		BTMTK_WARN("%s: fops is not open yet(%d)!", __func__, fstate);
		return -1;
	}

	if (xSemaphoreTake(data_ready_cb_mutex, portMAX_DELAY) == pdFALSE) {
		BTMTK_ERR("%s, data_ready_cb_mutex get failed", __func__);
		return -1;
	}

	if (restore_cb) {
		BTMTK_INFO("restore event cb from %p -> %p", curr_data_ready_cb, last_data_ready_cb);
		curr_data_ready_cb = last_data_ready_cb;
	} else {
		BTMTK_INFO("update event cb from %p -> %p", curr_data_ready_cb, cb);
		last_data_ready_cb = curr_data_ready_cb;
		curr_data_ready_cb = cb;
	}

	RtbEmptyQueue(bdev->rx_q);
	
	xSemaphoreGive(data_ready_cb_mutex);
	BTMTK_INFO("%s, end", __func__);
	return 0;
}

void bt_driver_register_state_change_cb(bt_state_change_cb cb, int *curr_state)
{
	struct btmtk_dev *bdev = NULL;
	unsigned char fstate = 0;
	
	state_change_cb = cb;
	
	bdev = btmtk_get_dev();
	if (!bdev) {
		BTMTK_ERR("%s: invalid parameters!", __func__);
		*curr_state = 0;
		return;
	}

	fstate = btmtk_fops_get_state(bdev);
	if (fstate == BTMTK_FOPS_STATE_OPENED) {
		BTMTK_INFO("%s, cb: %p, current_state: %d", __func__, cb, 1);
		*curr_state = 1;
	} else {
		BTMTK_INFO("%s, cb: %p, current_state: %d", __func__, cb, 0);
		*curr_state = 0;
	}
}

int btmtk_dispatch_pkt(struct btmtk_dev *bdev, sk_buff *skb)
{
	static u8 fwlog_picus_blocking_warn;
	static u8 fwlog_fwdump_blocking_warn;
	int state = 0;
	u8 hci_reset_event[HCI_RESET_EVT_LEN] = { 0x04, 0x0E, 0x04, 0x01, 0x03, 0x0c, 0x00 };
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	struct sk_buff *skb_opcode = NULL;
	uint8_t *upload_buf = NULL;

#if 0
	if (!g_fwlog)
		return 0;
#endif

	if ((skb_get_pkt_type(skb) == HCI_ACLDATA_PKT) &&
			skb->data[0] == 0x6f &&
			skb->data[1] == 0xfc) {
		static int dump_data_counter;
		static int dump_data_length;
		//BTMTK_INFO_RAW(skb->data, skb->len, "%s: 0x02 ", __func__);//used for collect fw dump
		state = btmtk_get_chip_state(bdev);
		if (state != BTMTK_STATE_FW_DUMP) {
			BTMTK_INFO("%s: FW dump begin", __func__);
			DUMP_TIME_STAMP("FW_dump_start");
			//btmtk_hci_snoop_print_to_log();
			/* Print too much log, it may cause kernel panic. */
			dump_data_counter = 0;
			dump_data_length = 0;
			btmtk_set_chip_state(bdev, BTMTK_STATE_FW_DUMP);

			//if (!btmtk_assert_wake_lock_check())
			//	btmtk_assert_wake_lock();
			//if (!btmtk_reset_timer_check(bdev))
			//	btmtk_reset_timer_update(bdev);
		}

		dump_data_counter++;
		dump_data_length += skb->len;

		/* coredump */
		/* print dump data to console */
		if (dump_data_counter % 1000 == 0) {
			BTMTK_INFO("%s: FW dump on-going, total_packet = %d, total_length = %d",
					__func__, dump_data_counter, dump_data_length);
		}

		/* print dump data to console */
		if (dump_data_counter < 50)
			BTMTK_INFO("%s: FW dump data (%d): %s",
					__func__, dump_data_counter, &skb->data[4]);

		/* In the new generation, we will check the keyword of coredump (; coredump end)
		 * Such as : 79xx
		 */
		if (skb->data[skb->len - 4] == 'e' &&
			skb->data[skb->len - 3] == 'n' &&
			skb->data[skb->len - 2] == 'd') {
			/* This is the latest coredump packet. */
			BTMTK_INFO("%s: FW dump end, dump_data_counter = %d", __func__, dump_data_counter);
			/* TODO: Chip reset*/
			bmain_info->reset_stack_flag = HW_ERR_CODE_CORE_DUMP;
			DUMP_TIME_STAMP("FW_dump_end");
			//if (bmain_info->hif_hook.waker_notify)
			//	bmain_info->hif_hook.waker_notify(bdev);
		}
		BTMTK_INFO("driver dump len:%d", skb->len);
		upload_buf = malloc(skb->len + 1);
		if (upload_buf == NULL) {
			BTMTK_INFO("%s: malloc fwlog buf fail", __func__);
			return 1;
		}

		upload_buf[0] = HCI_ACLDATA_PKT;
		memcpy((uint8_t*)upload_buf+1, skb->data, skb->len);

		if (fwlog_recv_cb) {
			BTMTK_INFO("driver callbck dump len:%d", skb->len + 1);
			fwlog_recv_cb(upload_buf, skb->len + 1);
		}
			
		//picus_coredump_out

		//if (RtbGetQueueLen(&bdev->fwlog_queue) < FWLOG_ASSERT_QUEUE_COUNT) {
			/* sent picus data to queue, picus tool will log it */
		//	RtbQueueTail(bdev->fwlog_queue,skb)
			//wake_up_interruptible(&g_fwlog->fw_log_inq);
			//fwlog_fwdump_blocking_warn = 0;
		//}

		if (!bdev->bt_cfg.support_picus_to_host)
			return 1;
	} else if ((skb_get_pkt_type(skb) == HCI_ACLDATA_PKT) &&
				(skb->data[0] == 0xff || skb->data[0] == 0xfe) &&
				skb->data[1] == 0x05 &&
				!bdev->bt_cfg.support_picus_to_host) {
			/* picus or syslog */

		//BTMTK_INFO_RAW(skb->data, skb->len, "%s: 0x02 ", __func__);
		upload_buf = malloc(skb->len + 1);
		if (upload_buf == NULL) {
			BTMTK_INFO("%s: malloc fwlog buf fail", __func__);
			return 1;
		}

		upload_buf[0] = HCI_ACLDATA_PKT;
		memcpy((uint8_t*)upload_buf+1, skb->data, skb->len);

		if (fwlog_recv_cb)
			fwlog_recv_cb(upload_buf, skb->len + 1);
			
		//if (RtbGetQueueLen(&bdev->fwlog_queue) < < FWLOG_QUEUE_COUNT) {
		//	RtbQueueTail(bdev->fwlog_queue,skb)
		//}
		return 1;
	} else if (memcmp(skb->data, &hci_reset_event[1], HCI_RESET_EVT_LEN - 1) == 0) {
		BTMTK_INFO("%s: Get RESET_EVENT", __func__);
		bdev->get_hci_reset = 1;
	}

	/* filter event from usr cmd */
	if ((skb_get_pkt_type(skb) == HCI_EVENT_PKT) &&
			skb->data[0] == 0x0E) {

		//BTMTK_INFO("%s: Not implement filter event from usr cmd", __func__);

#if 0
		if (skb_queue_len(&g_fwlog->usr_opcode_queue)) {
			BTMTK_DBG("%s: opcode queue len is %d", __func__,
					skb_queue_len(&g_fwlog->usr_opcode_queue));
			skb_opcode = skb_dequeue(&g_fwlog->usr_opcode_queue);
		}

		if (skb_opcode == NULL)
			return 0;

		if (skb_opcode->data[0] == skb->data[3] &&
					skb_opcode->data[1] == skb->data[4]) {
			BTMTK_INFO_RAW(skb->data, skb->len, "%s: recv event from user hci command - ", __func__);
			// should return to upper layer tool
			if (btmtk_skb_enq_fwlog(bdev, skb->data, skb->len, bt_cb(skb)->pkt_type,
						&g_fwlog->fwlog_queue) == 0) {
				wake_up_interruptible(&g_fwlog->fw_log_inq);
			}
			kfree_skb(skb_opcode);
			return 1;
		}

		BTMTK_DBG("%s: check opcode fail!", __func__);
		skb_queue_head(&g_fwlog->usr_opcode_queue, skb_opcode);
#endif
	}
	return 0;
}

sk_buff* bt_read()
{
	struct btmtk_dev *bdev = NULL;
	sk_buff *skb;
	unsigned char fstate = 0;
	int state = 0;
	
	//1. get device
	bdev = btmtk_get_dev();
	if (!bdev) {
		BTMTK_ERR("%s: invalid parameters!", __func__);
		return NULL;
	}

	fstate = btmtk_fops_get_state(bdev);
	if (fstate != BTMTK_FOPS_STATE_OPENED) {
		//BTMTK_WARN("%s: fops is not open yet(%d)!", __func__, fstate);
		return NULL;
	}

	state = btmtk_get_chip_state(bdev);
	if (state != BTMTK_STATE_WORKING) {
		//BTMTK_WARN_LIMITTED("%s: chip state is %d.", __func__, state);
		return NULL;
	}

	if (curr_data_ready_cb) {
		if (xSemaphoreTake(data_ready_cb_mutex, portMAX_DELAY) == pdFALSE) {
			BTMTK_ERR("%s, data_ready_cb_mutex get failed\n", __func__);
			return NULL;
		}else {
			skb = RtbDequeueHead(bdev->rx_q);
			BTMTK_INFO("%s, data_ready_cb_mutex get success ,dequeue skb:%p\n", __func__, &skb);
			xSemaphoreGive(data_ready_cb_mutex);
		}
	} else {
		skb = RtbDequeueHead(bdev->rx_q);
	}
	
	if (skb == NULL) {
		//BTMTK_WARN("%s, skb == NULL", __func__);
		return NULL;
	}
	BTMTK_INFO("bt_read skb:%p", &skb);
	BTMTK_INFO_RAW(skb->data, skb->len, "%s upload: ", __func__);
	//send data to hal layer
	return skb;
}

void btmtk_reg_hif_hook(struct hif_hook_ptr *hook)
{
	BTMTK_INFO("%s", __func__);
	memcpy(&main_info.hif_hook, hook, sizeof(struct hif_hook_ptr));
}

void btmtk_save_filter_vendor_cmd(sk_buff *skb, struct btmtk_dev *bdev, bool flag)
{
	int i = 0;
	u16 recv_opcode = 0;
	u16 filter_list_opcode = 0;
	BTMTK_INFO("%s", __func__);
}

/* Check power status, if power is off, try to set power on */
int btmtk_reset_power_on(struct btmtk_dev *bdev)
{
	if (bdev->power_state == BTMTK_DONGLE_STATE_POWER_OFF) {
		bdev->power_state = BTMTK_DONGLE_STATE_ERROR;
		if (btmtk_send_wmt_power_on_cmd(bdev) < 0) {
			return -1;
		}
		bdev->power_state = BTMTK_DONGLE_STATE_POWER_ON;
	}

	if (bdev->power_state != BTMTK_DONGLE_STATE_POWER_ON) {
		BTMTK_WARN("%s: end of Incorrect state:%d", __func__, bdev->power_state);
		return -1;
	}
	BTMTK_INFO("%s: end success", __func__);

	return 0;
}

int btmtk_send_wmt_power_on_cmd(struct btmtk_dev *bdev)
{
	u8 cmd[WMT_POWER_ON_CMD_LEN] = { 0x01, 0x6F, 0xFC, 0x06, 0x01, 0x06, 0x02, 0x00, 0x00, 0x01 };
	u8 event[WMT_POWER_ON_EVT_HDR_LEN] = { 0x04, 0xE4, 0x05, 0x02, 0x06, 0x01, 0x00 };	/* event[7] is key */
	int ret = -1, retry = RETRY_TIMES;

	if (!bdev) {
		BTMTK_ERR("%s: bdev is NULL !", __func__);
		return ret;
	}

retry_again:

	ret = btmtk_main_send_cmd(bdev,
			cmd, WMT_POWER_ON_CMD_LEN,
			event, WMT_POWER_ON_EVT_HDR_LEN,
			WMT_DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
	if (ret < 0) {
		BTMTK_ERR("%s: failed(%d)", __func__, ret);
		bdev->power_state = BTMTK_DONGLE_STATE_ERROR;
		ret = -1;
	} else if (ret == 0 && bdev->recv_evt_len > 0) {
		switch (bdev->io_buf[WMT_POWER_ON_EVT_RESULT_OFFSET]) {
		case 0:			 /* successful */
			BTMTK_INFO("%s: OK", __func__);
			bdev->power_state = BTMTK_DONGLE_STATE_POWER_ON;
			break;
		case 2:			 /* TODO:retry */
			if (retry > 0) {
				/* comment from fw, we need to retry a sec until power on successfully. */
				retry--;
				BTMTK_INFO("%s: need to try again", __func__);
				//msleep(50);
				vTaskDelay(50 / portTICK_RATE_MS);
				goto retry_again;
			}
			break;
		default:
			BTMTK_WARN("%s: Unknown result: %02X", __func__, bdev->io_buf[WMT_POWER_ON_EVT_RESULT_OFFSET]);
			bdev->power_state = BTMTK_DONGLE_STATE_ERROR;
			ret = -1;
			break;
		}
	}

	return ret;
}

int btmtk_send_wmt_power_off_cmd(struct btmtk_dev *bdev)
{
	u8 cmd[WMT_POWER_OFF_CMD_LEN] = { 0x01, 0x6F, 0xFC, 0x06, 0x01, 0x06, 0x02, 0x00, 0x00, 0x00 };
	/* To-Do, for event check */
	u8 event[WMT_POWER_OFF_EVT_HDR_LEN] = { 0x04, 0xE4, 0x05, 0x02, 0x06, 0x01, 0x00 };
	int ret = 0;

	if (!bdev) {
		BTMTK_ERR("%s: bdev is NULL !", __func__);
		return ret;
	}

	if (bdev->power_state == BTMTK_DONGLE_STATE_POWER_OFF) {
		BTMTK_WARN("%s: power_state already power off", __func__);
		return 0;
	}

	ret = btmtk_main_send_cmd(bdev,
			cmd, WMT_POWER_OFF_CMD_LEN,
			event, WMT_POWER_OFF_EVT_HDR_LEN,
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
	if (ret < 0) {
		BTMTK_ERR("%s: failed(%d)", __func__, ret);
		bdev->power_state = BTMTK_DONGLE_STATE_ERROR;
		return ret;
	}

	bdev->power_state = BTMTK_DONGLE_STATE_POWER_OFF;
	BTMTK_INFO("%s done", __func__);
	return ret;
}

int btmtk_vendor_cmd_filter(struct btmtk_dev *bdev, sk_buff *skb)
{
	int i = 0;
	u16 recv_opcode = 0;
	u16 filter_list_opcode = 0;
	return 0;
}

struct btmtk_dev *btmtk_get_dev(void)
{
	int i = 0;
	struct btmtk_dev *tmp_bdev = NULL;

	//BTMTK_INFO("%s", __func__);

	for (i = 0; i < btmtk_intf_num; i++) {
		/* Find empty slot for newly probe interface.
		 * Judged from load_rom_patch is done and
		 * Identified chip_id from cap_init.
		 */
		if (g_bdev[i]->hdev == NULL) {
			if (i == 0)
				g_bdev[i]->dongle_index = i;
			else
				g_bdev[i]->dongle_index = g_bdev[i - 1]->dongle_index + 1;

			/* reset pin initial value need to be -1, used to judge after
			 * disconnected before probe, can't do chip reset
			 */
			g_bdev[i]->bt_cfg.dongle_reset_gpio_pin = -1;
			//BTMTK_INFO("%s init support_picus_to_host=false", __func__);  to more called
			g_bdev[i]->bt_cfg.support_picus_to_host = false;
			tmp_bdev = g_bdev[i];

			/* Hook pre-defined table on state machine */
			g_bdev[i]->cif_state = (struct btmtk_cif_state *)g_cif_state;
			break;
		}
	}
	return tmp_bdev;
}

/* To allow g_bdev being sized from btmtk_intf_num setting */
struct btmtk_dev *g_sbdev;

static void btmtk_main_info_initialize(void)
{
	u32 snoop_idx = 0;
	BTMTK_INFO("%s", __func__);
	memset(&main_info, 0, sizeof(main_info));
	for (snoop_idx = 0; snoop_idx < HCI_SNOOP_TYPE_MAX; snoop_idx++)
		main_info.snoop[snoop_idx].index = HCI_SNOOP_ENTRY_NUM - 1;

	main_info.wmt_over_hci_header[0] = HCI_COMMAND_PKT;
	main_info.wmt_over_hci_header[1] = 0x6F;
	main_info.wmt_over_hci_header[2] = 0xFC;

	main_info.read_iso_packet_size_cmd[0] = HCI_COMMAND_PKT;
	main_info.read_iso_packet_size_cmd[1] = 0x98;
	main_info.read_iso_packet_size_cmd[2] = 0xFD;
	main_info.read_iso_packet_size_cmd[3] = 0x02;

}

struct btmtk_dev *btmtk_allocate_dev_memory(struct btmtk_dev *dev)
{
	struct btmtk_dev *bdev;
	size_t len = sizeof(*bdev);

	BTMTK_INFO("%s", __func__);

/*
	if (dev != NULL)
		bdev = devm_kzalloc(dev, len, GFP_KERNEL);
	else
		bdev = kzalloc(len, GFP_KERNEL);

	if (!bdev)
		return NULL;
*/

    bdev = malloc(len);

	memset(bdev, 0, len);

	btmtk_set_chip_state(bdev, BTMTK_STATE_INIT);

	return bdev;
}

static int main_init(void)
{
	int i = 0;

	BTMTK_INFO("%s", __func__);

	/* Check if user changes default minimum supported intf count */
	if (btmtk_intf_num < BT_MCU_MINIMUM_INTERFACE_NUM) {
		btmtk_intf_num = BT_MCU_MINIMUM_INTERFACE_NUM;
		BTMTK_WARN("%s minimum interface is %d", __func__, btmtk_intf_num);
	}

	BTMTK_INFO("%s supported intf count <%d>", __func__, btmtk_intf_num);

	g_bdev = malloc((sizeof(*g_bdev) * btmtk_intf_num));
	memset(g_bdev, 0, (sizeof(*g_bdev) * btmtk_intf_num));

	for (i = 0; i < btmtk_intf_num; i++) {
		g_bdev[i] = btmtk_allocate_dev_memory(NULL);
		if (g_bdev[i]) {
			/* BTMTK_STATE_UNKNOWN instead? */
			/* btmtk_set_chip_state(g_bdev[i], BTMTK_STATE_INIT); */

			/* BTMTK_FOPS_STATE_UNKNOWN instead? */
			btmtk_fops_set_state(g_bdev[i], BTMTK_FOPS_STATE_INIT);
		} else {
			return -ENOMEM;
		}
	}

	// Set global variable for btif interface
	g_sbdev = g_bdev[0];

	btmtk_main_info_initialize();

	return 0;
}

static int main_exit(void)
{
	int i = 0;

	BTMTK_INFO("%s releasing intf count <%d>", __func__, btmtk_intf_num);

	if (g_bdev == NULL) {
		BTMTK_WARN("%s g_data is NULL", __func__);
		return 0;
	}

	if (data_ready_cb_mutex)
		vSemaphoreDelete(data_ready_cb_mutex);
	data_ready_cb_mutex = NULL;

	for (i = 0; i < btmtk_intf_num; i++) {
		if (g_bdev[i] != NULL)
			free(g_bdev[i]);
	}
	free(g_bdev);
	return 0;
}

int btmtk_main_init(void)
{
	u8 i = 0;
	int ret = 0;
	
	BTMTK_INFO("%s", __func__);

	ret = btmtk_init_mutex();
	if (ret == -1)
	{
		BTMTK_ERR("%s: init mutex fail", __func__);
		return -EIO;
	}
	
	ret = main_init();
	if (ret < 0)
		return ret;

	for (i = 0; i < btmtk_intf_num; i++)
		btmtk_set_chip_state(g_bdev[i], BTMTK_STATE_DISCONNECT);

/*  pending: support fw log
	ret = btmtk_fops_initfwlog();
	if (ret < 0) {
		BTMTK_ERR("*** STPBTFWLOG registration failed(%d)! ***", ret);
		main_exit();
		return ret;
	}
*/
	ret = btmtk_cif_register();
	if (ret < 0) {
		BTMTK_ERR("*** USB registration failed(%d)! ***", ret);
		main_exit();
		return ret;
	}

	if (main_info.hif_hook.init)
		ret = main_info.hif_hook.init();

	BTMTK_INFO("%s: Done", __func__);
	return ret;
}

/*turnkey: main_driver_exit*/
void btmtk_main_deinit(void)
{	
	BTMTK_INFO("%s", __func__);
}

static int btmtk_main_allocate_memory(struct btmtk_dev *bdev)
{
	int err = -1;

	BTMTK_INFO("%s Begin", __func__);

	if (bdev->rom_patch_bin_file_name == NULL) {
		bdev->rom_patch_bin_file_name = malloc(MAX_BIN_FILE_NAME_LEN);
		if (!bdev->rom_patch_bin_file_name) {
			BTMTK_ERR("%s: alloc memory fail (bdev->rom_patch_bin_file_name)", __func__);
			goto end;
		}
	}

	if (bdev->io_buf == NULL) {
		bdev->io_buf = malloc(IO_BUF_SIZE);
		if (!bdev->io_buf) {
			BTMTK_ERR("%s: alloc memory fail (bdev->io_buf)", __func__);
			goto err2;
		}
	}

	if (bdev->bt_cfg_file_name == NULL) {
		bdev->bt_cfg_file_name = malloc(MAX_BIN_FILE_NAME_LEN);
		if (!bdev->bt_cfg_file_name) {
			BTMTK_ERR("%s: alloc memory fail (bdev->bt_cfg_file_name)", __func__);
			goto err1;
		}
	}

	if (bdev->country_file_name == NULL) {
		bdev->country_file_name = malloc(MAX_BIN_FILE_NAME_LEN);
		if (!bdev->country_file_name) {
			BTMTK_ERR("%s: alloc memory fail (bdev->country_file_name)", __func__);
			goto err0;
		}
	}
	BTMTK_INFO("%s Done", __func__);
	return 0;

err0:
	free(bdev->bt_cfg_file_name);
	bdev->bt_cfg_file_name = NULL;
err1:
	free(bdev->io_buf);
	bdev->io_buf = NULL;
err2:
	free(bdev->rom_patch_bin_file_name);
	bdev->rom_patch_bin_file_name = NULL;
end:
	return err;
}

static void btmtk_main_free_memory(struct btmtk_dev *bdev)
{
	free(bdev->rom_patch_bin_file_name);
	bdev->rom_patch_bin_file_name = NULL;

	free(bdev->bt_cfg_file_name);
	bdev->bt_cfg_file_name = NULL;

	free(bdev->country_file_name);
	bdev->country_file_name = NULL;

	free(bdev->io_buf);
	bdev->io_buf = NULL;

	BTMTK_INFO("%s: Success", __func__);
}

static void btmtk_init_memory(struct btmtk_dev *bdev)
{
	bdev->suspend_count = 0;
	bdev->tx_in_flight = 0;
	bdev->get_hci_reset = 0;
	bdev->iso_threshold = 0;
	bdev->sco_num = 0;
	bdev->isoc_altsetting = 0;
	bdev->tx_state = 0;
	bdev->need_compare_num = 0;

#if CFG_SUPPORT_LEAUDIO_CLK
	bdev->le_aud.irq = -1;
#endif
}

int btmtk_cap_init(struct btmtk_dev *bdev)
{
	int ret = 0;

	if (!bdev) {
		BTMTK_ERR("%s, bdev is NULL!", __func__);
		ret = -1;
		goto exit;
	}
	
	BTMTK_INFO("%s", __func__);

	/* Todo read wifi fw version
	 * int wifi_fw_ver;

	 * btmtk_cif_write_register(bdev, 0x7C4001C4, 0x00008800);
	 * btmtk_cif_read_register(bdev, 0x7c4f0004, &wifi_fw_ver);
	 * BTMTK_ERR("wifi fw_ver = %04X", wifi_fw_ver);
	 */

	ret = main_info.hif_hook.reg_read(bdev, CHIP_ID, &bdev->chip_id);
	if (ret < 0) {
		BTMTK_ERR("read chip id failed");
		ret = -EIO;
		goto exit;
	} else {
		if (is_mt6639(bdev->chip_id) || is_mt7902(bdev->chip_id)
				|| is_mt7922(bdev->chip_id) || is_mt7961(bdev->chip_id)) {
			ret = main_info.hif_hook.reg_read(bdev, FLAVOR, &bdev->flavor);
			if (ret < 0) {
				BTMTK_ERR("read flavor id failed");
				ret = -EIO;
				goto exit;
			}
			ret = main_info.hif_hook.reg_read(bdev, FW_VERSION, &bdev->fw_version);
			if (ret < 0) {
				BTMTK_ERR("read fw version failed");
				ret = -EIO;
				goto exit;
			}
		} else {
			BTMTK_ERR("Unknown Mediatek device(%04X)\n", bdev->chip_id);
			ret = -EIO;
			goto exit;
		}
	}

	BTMTK_INFO("%s: Chip ID = 0x%x", __func__, bdev->chip_id);
	BTMTK_INFO("%s: flavor = 0x%x", __func__, bdev->flavor);
	BTMTK_INFO("%s: FW Ver = 0x%x", __func__, bdev->fw_version);

exit:
	return ret;
}

int btmtk_main_cif_initialize(struct btmtk_dev *bdev, int hci_bus)
{
	int err = 0;
	
	BTMTK_INFO("%s", __func__);

	// btmtk_reset_timer_add(bdev);

	err = btmtk_main_allocate_memory(bdev);
	if (err < 0) {
		BTMTK_ERR("btmtk_main_allocate_memory failed!");
		goto end;
	}

	//btmtk_initialize_cfg_items(bdev);
	btmtk_init_memory(bdev);

    /*
	err = btmtk_allocate_hci_device(bdev, hci_bus);
	if (err < 0) {
		BTMTK_ERR("btmtk_allocate_hci_device failed!");
		goto free_mem;
	}
	*/
	
	bdev->rx_q = RtbQueueInit();
	bdev->fwlog_queue = RtbQueueInit();

	data_ready_cb_mutex = xSemaphoreCreateMutex();
	if (data_ready_cb_mutex == NULL) {
		BTMTK_ERR("%s create data_ready_cb_mutex fail", __func__);
		goto end;
	}

	err = btmtk_cap_init(bdev);
	if (err < 0) {
		if (err == -EIO) {
			BTMTK_ERR("btmtk_cap_init failed, do chip reset!");
			goto end;
		} else {
			BTMTK_ERR("btmtk_cap_init failed!");
			goto free_hci_dev;
		}
	}

	return 0;

free_hci_dev:
	// btmtk_free_hci_device(bdev, hci_bus);
//free_mem:
	//btmtk_main_free_memory(bdev);
end:
	if (data_ready_cb_mutex)
		vSemaphoreDelete(data_ready_cb_mutex);
	data_ready_cb_mutex = NULL;
	return err;
}

int btmtk_main_send_cmd(struct btmtk_dev *bdev, const uint8_t *cmd,
		const int cmd_len, const uint8_t *event, const int event_len, int delay,
		int retry, int pkt_type, bool flag)
{
	sk_buff *skb = NULL;
	int ret = 0;
	int state = 0;

	// if (bdev == NULL || bdev->hdev == NULL ||
	if (bdev == NULL ||
		cmd == NULL || cmd_len <= 0) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		ret = -EINVAL;
		goto exit;
	}

	if (!is_mt66xx(bdev->chip_id) &&
		memcmp(cmd, main_info.wmt_over_hci_header, WMT_OVER_HCI_HEADER_SIZE) &&
		pkt_type != BTMTK_TX_ACL_FROM_DRV &&
		bdev->power_state != BTMTK_DONGLE_STATE_POWER_ON) {
		BTMTK_WARN("%s: chip power isn't on, ignore this command, state is %d", __func__, bdev->power_state);
		goto exit;
	}

	state = btmtk_get_chip_state(bdev);
	if (state == BTMTK_STATE_FW_DUMP) {
		BTMTK_WARN("%s: FW dumping ongoing, don't send any cmd to FW!!!", __func__);
		ret = -1;
		goto exit;
	}
	
	//BTMTK_INFO("%s", __func__);
	
	skb = skb_alloc_and_init(HCI_COMMAND_PKT, (uint8_t *)cmd, cmd_len);

/*
	skb = alloc_skb(cmd_len + BT_SKB_RESERVE);
	if (skb == NULL) {
		BTMTK_ERR("%s allocate skb failed!!", __func__);
		ret = -ENOMEM;
		goto exit;
	}

	skb_reserve(skb, 7);
	sbk->pkt_type = HCI_COMMAND_PKT;
	memcpy(skb->data, cmd, cmd_len);
	skb->len = cmd_len;
*/

#if ENABLESTP
	skb = mtk_add_stp(bdev, skb);
#endif

	/* wmt cmd and download fw patch using wmt cmd with USB interface, need use
	 * usb_control_msg to recv wmt event;
	 * other HIF don't use this method to recv wmt event
	 */

	ret = main_info.hif_hook.send_and_recv(bdev,
	//ret = btmtk_sdio_send_and_recv(bdev,
			skb,
			event, event_len,
			delay, retry, pkt_type, flag);

	if (ret < 0) {
		BTMTK_ERR("%s send_and_recv failed!!", __func__);
		/* ERRNUM is used to handle when skb has been sent successful,
		 * but wait related event failed, in this case, we don't need to free skb here,
		 * otherwise, it will be double free.
		 */
		if (ret != -ERRNUM) {
			// btmtk_free_skb(skb);
		}
	}

exit:
	//BTMTK_INFO("%s end!!", __func__);
	return ret;
}

int btmtk_recv_acl(struct btmtk_dev *bdev, sk_buff *skb)
{
	//struct btmtk_dev *bdev = NULL;

	if (skb == NULL) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		return -EINVAL;
	}

	if (bdev == NULL) {
		BTMTK_ERR("%s, bdev or workqueue is invalid!", __func__);
		return -EINVAL;
	}

	//BTMTK_INFO_RAW(skb->data, skb->len, "%s: 0x02 ", __func__);

	if (btmtk_dispatch_pkt(bdev,skb)) {
		/* Drop by driver, don't send to stack */
		skb_free(&skb);
		return 0;
	}

	if (main_info.hif_hook.event_filter(bdev, skb)) {
		/* Drop by driver, don't send to stack */
		skb_free(&skb);
		return 0;
	}
	BTMTK_INFO_RAW(skb->data, skb->len, "%s: enqueue", __func__);
	BTMTK_INFO("btmtk_recv_acl skb:%p", &skb);

	if (curr_data_ready_cb) {
		if (xSemaphoreTake(data_ready_cb_mutex, portMAX_DELAY) == pdFALSE) {
			BTMTK_ERR("%s, data_ready_cb_mutex get failed\n", __func__);
			return 0;
		} else {
			RtbQueueTail(bdev->rx_q, skb);
			BTMTK_INFO("%s, data_ready_cb_mutex get success, enqueu skb:%p\n", __func__, &skb);
			xSemaphoreGive(data_ready_cb_mutex);
		}

		BTMTK_INFO("RX ready, notify consumer,rx_len:%d\n", RtbGetQueueLen(bdev->rx_q));
		BTMTK_INFO_RAW(skb->data, skb->len, "%s: RX (t=%d) ready, notify consumer\n", __func__, skb_get_pkt_type(skb));
		curr_data_ready_cb();
		return 0;
	} else {
		RtbQueueTail(bdev->rx_q, skb);
	}

	// queue_work(bdev->workqueue, &bdev->rx_work);
	//btmtk_rx_work(bdev);

	/* remove it, if workqueue can't be scheduled, you can reuse it */
#if 0
	skip_pkt = btmtk_dispatch_fwlog(bdev, skb);
	if (skip_pkt == 0)
		err = hci_recv_frame(hdev, skb);
#endif
	return 0;
}

int btmtk_recv_event(struct btmtk_dev *bdev, sk_buff *skb)
{
	//struct btmtk_dev *bdev = NULL;

	//if (hdev == NULL || skb == NULL) {
	if (skb == NULL) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		return -EINVAL;
	}

	//bdev = hci_get_drvdata(hdev);
	// if (bdev == NULL || bdev->workqueue == NULL) {
	if (bdev == NULL) {
		BTMTK_ERR("%s, bdev or workqueue is invalid!", __func__);
		skb_free(&skb);
		return -EINVAL;
	}
	
	//BTMTK_INFO("%s", __func__);

	/* Fix up the vendor event id with 0xff for vendor specific instead
	 * of 0xe4 so that event send via monitoring socket can be parsed
	 * properly.
	 */
	/* if (hdr->evt == 0xe4) {
	 * BTMTK_DBG("%s hdr->evt is %02x", __func__, hdr->evt);
	 * hdr->evt = HCI_EV_VENDOR;
	 * }
	 */

	//BTMTK_INFO_RAW(skb->data, skb->len, "%s: 0x04 ", __func__);

	if (btmtk_dispatch_pkt(bdev,skb)) {
		/* Drop by driver, don't send to stack */
		skb_free(&skb);
		return 0;
	}

	if (main_info.hif_hook.event_filter(bdev, skb)) {
		/* Drop by driver, don't send to stack */
		skb_free(&skb);
		return 0;
	}

	BTMTK_INFO_RAW(skb->data, skb->len, "%s: enqueue ", __func__);
	BTMTK_INFO("btmtk_recv_event skb:%p", &skb);
	if (curr_data_ready_cb) {
		if (xSemaphoreTake(data_ready_cb_mutex, portMAX_DELAY) == pdFALSE) {
			BTMTK_ERR("%s, data_ready_cb_mutex get failed\n", __func__);
			return 0;
		} else {
			RtbQueueTail(bdev->rx_q, skb);
			BTMTK_INFO("%s, data_ready_cb_mutex get success, enqueu skb:%p\n", __func__, &skb);
			xSemaphoreGive(data_ready_cb_mutex);
		}

		BTMTK_INFO("RX ready, notify consumer,rx_len:%d\n", RtbGetQueueLen(bdev->rx_q));
		BTMTK_INFO_RAW(skb->data, skb->len, "%s: RX (t=%d) ready, notify consumer\n", __func__, skb_get_pkt_type(skb));
		curr_data_ready_cb();
		return 0;
	} else {
		RtbQueueTail(bdev->rx_q, skb);
	}
	//queue_work(bdev->workqueue, &bdev->rx_work);
	//btmtk_rx_work(bdev);

	/* remove it, if workqueue can't be scheduled, you can reuse it */
#if 0
	skip_pkt = btmtk_dispatch_event(hdev, skb);
	if (skip_pkt == 0)
		err = hci_recv_frame(hdev, skb);

	if (err < 0) {
		BTMTK_ERR("%s hci_recv_failed, err = %d", __func__, err);
		goto err_out;
	}
#endif
	return 0;
}

int btmtk_recv_sco(struct btmtk_dev *bdev, sk_buff *skb)
{

	if (skb == NULL) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		return -EINVAL;
	}

	if (bdev == NULL) {
		BTMTK_ERR("%s, bdev or workqueue is invalid!", __func__);
		skb_free(&skb);
		return -EINVAL;
	}
	
	//BTMTK_INFO("%s", __func__);

	BTMTK_INFO_RAW(skb->data, skb->len, "%s: enqueue ", __func__);
	BTMTK_INFO("H4_RECV_SCO skb:%p", &skb);
	if (curr_data_ready_cb) {
		if (xSemaphoreTake(data_ready_cb_mutex, portMAX_DELAY) == pdFALSE) {
			BTMTK_ERR("%s, data_ready_cb_mutex get failed\n", __func__);
			return 0;
		} else {
			RtbQueueTail(bdev->rx_q, skb);
			BTMTK_INFO("%s, data_ready_cb_mutex get success, enqueu skb:%p\n", __func__, &skb);
			xSemaphoreGive(data_ready_cb_mutex);
		}

		BTMTK_INFO("RX ready, notify consumer,rx_len:%d\n", RtbGetQueueLen(bdev->rx_q));
		BTMTK_INFO_RAW(skb->data, skb->len, "%s: RX (t=%d) ready, notify consumer\n", __func__, skb_get_pkt_type(skb));
		curr_data_ready_cb();
		return 0;
	} else {
		RtbQueueTail(bdev->rx_q, skb);
	}
	return 0;
}


static const struct h4_recv_pkt mtk_recv_pkts[] = {
	{ H4_RECV_ACL,      .recv = btmtk_recv_acl },
	{ H4_RECV_SCO,      .recv = btmtk_recv_sco },
	{ H4_RECV_EVENT,    .recv = btmtk_recv_event },
	//{ H4_RECV_ISO,      .recv = btmtk_recv_iso },
};
#define MTK_RECV_PKTS_ARRAY_SIZE 4

/* HCI receive mechnism */
static inline sk_buff *h4_recv_buf(struct btmtk_dev *bdev,
					  sk_buff *skb,
					  const unsigned char *buffer,
					  int count,
					  const struct h4_recv_pkt *pkts,
					  int pkts_count)
{
	//struct btmtk_dev *bdev = NULL;
	/* used for print debug log*/
	const unsigned char *buffer_dbg = buffer;
	int count_dbg = count;
	unsigned char *skb_tmp = NULL;

	//if (hdev == NULL || buffer == NULL) {
	if (buffer == NULL) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		// return ERR_PTR(-EINVAL);
		return NULL;
	}

			/* Complete frame */
			(&pkts[buffer[0]])->recv(bdev, skb);
			/*
			if (is_mt66xx(bdev->chip_id))
				btmtk_set_sleep(hdev, FALSE);
			*/
			skb = NULL;

	return skb;
}

int btmtk_recv(struct btmtk_dev *bdev, const u8 *data, size_t count)
{
	//struct btmtk_dev *bdev = NULL;
	const unsigned char *p_left = data;
	int sz_left = count;
	int err;

	//if (hdev == NULL || data == NULL) {
	if (data == NULL) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		return -EINVAL;
	}
	
    sk_buff *skb = NULL;

    if (data[0] == HCI_EVENT_PKT) {
		skb = skb_alloc_and_init(HCI_EVENT_PKT, (uint8_t *)data + 1, count - 1);
		btmtk_recv_event(bdev, skb);
	}
	else if (data[0] == HCI_ACLDATA_PKT) {
		skb = skb_alloc_and_init(HCI_ACLDATA_PKT, (uint8_t *)data + 1, count - 1);
		btmtk_recv_acl(bdev, skb);
	}
	else if (data[0] == HCI_SCODATA_PKT)
	{
		skb = skb_alloc_and_init(HCI_SCODATA_PKT, (uint8_t *)data + 1, count - 1);
		btmtk_recv_sco(bdev, skb);
		//BTMTK_INFO_RAW(skb->data, skb->len, "%s: not implement ", __func__);
	}
	return 0;
}


static int btmtk_send_wmt_download_cmd(struct btmtk_dev *bdev, u8 *cmd,
		int cmd_len, u8 *event, int event_len, struct _Section_Map *sectionMap,
		u8 fw_state, u8 dma_flag, int patch_flag)
{
	int payload_len = 0;
	int ret = -1;
	int i = 0;
	u32 revert_SecSpec = 0;

	if (bdev == NULL || cmd == NULL || event == NULL || sectionMap == NULL) {
		BTMTK_ERR("%s: invalid parameter!", __func__);
		return ret;
	}

	/* need refine this cmd to mtk_wmt_hdr struct*/
	/* prepare HCI header */
	cmd[0] = 0x01;
	cmd[1] = 0x6F;
	cmd[2] = 0xFC;

	/* prepare WMT header */
	cmd[4] = 0x01;
	cmd[5] = 0x01; /* opcode */

	if (fw_state == 0) {
		/* prepare WMT DL cmd */
		payload_len = SEC_MAP_NEED_SEND_SIZE + 2;

		cmd[3] = (payload_len + 4) & 0xFF; /* length*/
		cmd[6] = payload_len & 0xFF;
		cmd[7] = (payload_len >> 8) & 0xFF;
		cmd[8] = 0x00; /* which is the FW download state 0 */
		cmd[9] = dma_flag; /* 1:using DMA to download, 0:using legacy wmt cmd*/
		cmd_len = SEC_MAP_NEED_SEND_SIZE + PATCH_HEADER_SIZE;

		if (patch_flag == WIFI_DOWNLOAD) {
			for (i = 0; i < SECTION_SPEC_NUM; i++) {
				revert_SecSpec = be2cpu32(sectionMap->u4SecSpec[i]);
				memcpy(&cmd[PATCH_HEADER_SIZE] + i * sizeof(u32), (u8 *)&revert_SecSpec, sizeof(u32));
			}
		} else
			memcpy(&cmd[PATCH_HEADER_SIZE], (u8 *)(sectionMap->u4SecSpec), SEC_MAP_NEED_SEND_SIZE);

		BTMTK_INFO_RAW(cmd, cmd_len, "%s: CMD: ", __func__);

		ret = btmtk_main_send_cmd(bdev, cmd, cmd_len,
				event, event_len, DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV,
				CMD_NO_NEED_FILTER);
		if (ret < 0) {
			BTMTK_ERR("%s: send wmd dl cmd failed, terminate!", __func__);
			return PATCH_ERR;
		}

		if (bdev->recv_evt_len >= event_len)
			return bdev->io_buf[PATCH_STATUS];

		return PATCH_ERR;
	}

	BTMTK_ERR("%s: fw state is error!", __func__);
	return ret;
}

static int btmtk_load_fw_patch_using_wmt_cmd(struct btmtk_dev *bdev,
		u8 *image, u8 *fwbuf, u8 *event, int event_len, u32 patch_len, int offset)
{
	int ret = 0;
	u32 cur_len = 0;
	s32 sent_len;
	int first_block = 1;
	u8 phase;
	int delay = PATCH_DOWNLOAD_PHASE1_2_DELAY_TIME;
	int retry = PATCH_DOWNLOAD_PHASE1_2_RETRY;

	if (bdev == NULL || image == NULL || fwbuf == NULL) {
		BTMTK_WARN("%s, invalid parameters!", __func__);
		ret = -1;
		goto exit;
	}

	/* loading rom patch */
	while (1) {
		s32 sent_len_max = UPLOAD_PATCH_UNIT - PATCH_HEADER_SIZE;

		sent_len = (patch_len - cur_len) >= sent_len_max ? sent_len_max : (patch_len - cur_len);

		if (sent_len > 0) {
			if (first_block == 1) {
				if (sent_len < sent_len_max)
					phase = PATCH_PHASE3;
				else
					phase = PATCH_PHASE1;
				first_block = 0;
			} else if (sent_len == sent_len_max) {
				if (patch_len - cur_len == sent_len_max)
					phase = PATCH_PHASE3;
				else
					phase = PATCH_PHASE2;
			} else {
				phase = PATCH_PHASE3;
			}


			/* prepare HCI header */
			image[0] = 0x02;
			image[1] = 0x6F;
			image[2] = 0xFC;
			image[3] = (sent_len + 5) & 0xFF;
			image[4] = ((sent_len + 5) >> 8) & 0xFF;

			/* prepare WMT header */
			image[5] = 0x01;
			image[6] = 0x01;
			image[7] = (sent_len + 1) & 0xFF;
			image[8] = ((sent_len + 1) >> 8) & 0xFF;

			image[9] = phase;
			memcpy(&image[10], fwbuf + offset + cur_len, sent_len);
			if (phase == PATCH_PHASE3) {
				if (is_mt7922(bdev->chip_id) || is_mt7902(bdev->chip_id)) {
					/* if secure boot enable, it need take 76ms at less
					 * for RSA check.
					 */
					delay = PATCH_DOWNLOAD_PHASE3_SECURE_BOOT_DELAY_TIME;
					retry = PATCH_DOWNLOAD_PHASE3_RETRY;
				} else {
					delay = PATCH_DOWNLOAD_PHASE3_DELAY_TIME;
					retry = PATCH_DOWNLOAD_PHASE3_RETRY;
				}
			}

			cur_len += sent_len;
			BTMTK_DBG("%s: sent_len = %d, cur_len = %d, phase = %d", __func__,
					sent_len, cur_len, phase);

			ret = btmtk_main_send_cmd(bdev, image, sent_len + PATCH_HEADER_SIZE,
					event, event_len, delay, retry, BTMTK_TX_ACL_FROM_DRV,
					CMD_NO_NEED_FILTER);
			if (ret < 0) {
				BTMTK_INFO("%s: send patch failed, terminate", __func__);
				goto exit;
			}
		} else
			break;
	}

exit:
	return ret;
}

extern const char mtk_bt_ram_ver[];
extern const char mtk_bt_ram_bin[];
extern const int mtk_bt_ram_len;

#define PATCH_DELAY 100

static int btmtk_send_fw_rom_patch_79xx(struct btmtk_dev *bdev,
		u8 *fwbuf, int patch_flag)
{
	u8 *pos;
	int loop_count = 0;
	int ret = 0;
	u32 section_num = 0;
	u32 section_offset = 0;
	u32 dl_size = 0;
	int patch_status = 0;
	int retry = 20;
	u8 dma_flag = PATCH_DOWNLOAD_USING_WMT;
	struct _Section_Map *sectionMap;
	struct _Global_Descr *globalDescr;
	u8 event[LD_PATCH_EVT_LEN] = {0x04, 0xE4, 0x05, 0x02, 0x01, 0x01, 0x00, 0x00}; /* event[7] is status*/
	
    BTMTK_INFO("%s", __func__);

	if (fwbuf == NULL) {
		BTMTK_WARN("%s, fwbuf is NULL", __func__);
		BTMTK_WARN("%s, fwbuf = (u8 *)&mtk_bt_ram_bin;", __func__);
		fwbuf = (u8 *)&mtk_bt_ram_bin;
		// ret = -1;
		// goto exit;
	}

	globalDescr = (struct _Global_Descr *)(fwbuf + FW_ROM_PATCH_HEADER_SIZE);

	BTMTK_INFO("%s: loading rom patch...\n", __func__);

	if (patch_flag == WIFI_DOWNLOAD)
		section_num = be2cpu32(globalDescr->u4SectionNum);
	else
		section_num = globalDescr->u4SectionNum;
	BTMTK_INFO("%s: section_num = 0x%08x\n", __func__, section_num);

	if (section_num > SECTION_NUM_MAX) {
		BTMTK_ERR("%s: section_num 0x%08x is an error value", __func__, section_num);
		ret = -1;
		goto exit;
	}

	pos = malloc(UPLOAD_PATCH_UNIT);
	if (!pos) {
		BTMTK_ERR("%s: alloc memory failed", __func__);
		ret = -1;
		goto exit;
	}

	do {
		sectionMap = (struct _Section_Map *)(fwbuf + FW_ROM_PATCH_HEADER_SIZE +
				FW_ROM_PATCH_GD_SIZE + FW_ROM_PATCH_SEC_MAP_SIZE * loop_count);

		if (patch_flag == WIFI_DOWNLOAD) {
			/* wifi is big-endian */
			section_offset = be2cpu32(sectionMap->u4SecOffset);
			dl_size = be2cpu32(sectionMap->bin_info_spec.u4DLSize);
			if (main_info.hif_hook.dl_dma)
				dma_flag = be2cpu32(sectionMap->bin_info_spec.u4DLModeCrcType) & 0xFF;
		} else {
			/* BT & ZB are little-endian */
			section_offset = sectionMap->u4SecOffset;
			dl_size = sectionMap->bin_info_spec.u4DLSize;
			/*
			 * loop_count = 0: BGF patch
			 *              1: BT ILM
			 * only BT ILM support DL DMA for Buzzard
			 */
			if (main_info.hif_hook.dl_dma) {
				dma_flag = le2cpu32(sectionMap->bin_info_spec.u4DLModeCrcType) & 0xFF;
			}
		}
		BTMTK_INFO("%s: loop_count = %d, section_offset = 0x%08x, download patch_len = 0x%08x, dl mode = %d\n",
				__func__, loop_count, section_offset, dl_size, dma_flag);
		if (dl_size > 0) {
			retry = 20;
			do {
				patch_status = btmtk_send_wmt_download_cmd(bdev, pos, 0,
						event, LD_PATCH_EVT_LEN - 1, sectionMap, 0, dma_flag, patch_flag);
				BTMTK_INFO("%s: patch_status %d", __func__, patch_status);

				if (patch_status > PATCH_READY || patch_status == PATCH_ERR) {
					BTMTK_ERR("%s: patch_status error", __func__);
					ret = -1;
					goto err;
				} else if (patch_status == PATCH_READY) {
					BTMTK_INFO("%s: no need to load rom patch section%d", __func__, loop_count);
					goto next_section;
				} else if (patch_status == PATCH_IS_DOWNLOAD_BY_OTHER) {
					// msleep(100);
					vTaskDelay(PATCH_DELAY / portTICK_RATE_MS);
					retry--;
				} else if (patch_status == PATCH_NEED_DOWNLOAD) {
					break;  /* Download ROM patch directly */
				}
			} while (retry > 0);

			if (patch_status == PATCH_IS_DOWNLOAD_BY_OTHER) {
				BTMTK_WARN("%s: Hold by another fun more than 2 seconds", __func__);
				ret = -1;
				goto err;
			}

            dma_flag = PATCH_DOWNLOAD_USING_WMT;
			// if (dma_flag == PATCH_DOWNLOAD_USING_DMA && main_info.hif_hook.dl_dma) {
			if (dma_flag == PATCH_DOWNLOAD_USING_DMA) {
				/* using DMA to download fw patch*/
				// ret = main_info.hif_hook.dl_dma(bdev,
				ret = btmtk_sdio_load_fw_patch_using_dma(bdev,
					pos, fwbuf,
					dl_size, section_offset);
				if (ret < 0) {
					BTMTK_ERR("%s: btmtk_load_fw_patch_using_dma failed!", __func__);
					goto err;
				}
			} else {
				/* using legacy wmt cmd to download fw patch */
				ret = btmtk_load_fw_patch_using_wmt_cmd(bdev, pos, fwbuf, event,
						LD_PATCH_EVT_LEN - 1, dl_size, section_offset);
				if (ret < 0) {
					BTMTK_ERR("%s: btmtk_load_fw_patch_using_wmt_cmd failed!", __func__);
					goto err;
				}
			}
		}

next_section:
		continue;
	} while (++loop_count < section_num);

err:
	free(pos);
	pos = NULL;

exit:
	return ret;
}

int btmtk_load_rom_patch_79xx(struct btmtk_dev *bdev, int patch_flag)
{
	int ret = 0;
	u8 *rom_patch = NULL;
	unsigned int rom_patch_len = 0;

	BTMTK_INFO("%s, patch_flag = %d!", __func__, patch_flag);

	if (!bdev) {
		BTMTK_ERR("%s, invalid parameters!", __func__);
		return -EINVAL;
	}

	ret = btmtk_send_fw_rom_patch_79xx(bdev, rom_patch, patch_flag);
	if (ret < 0) {
		BTMTK_ERR("%s, btmtk_send_fw_rom_patch_79xx failed!", __func__);
		goto err;
	}

	bdev->power_state = BTMTK_DONGLE_STATE_POWER_OFF;
	BTMTK_INFO("btmtk_load_rom_patch_79xx end");

err:
	return ret;
}

int btmtk_load_rom_patch(struct btmtk_dev *bdev)
{
	int err = -1;
	if (!bdev) {
		BTMTK_ERR("%s: invalid parameters!", __func__);
		return err;
	}
	
	BTMTK_INFO("%s", __func__);

	if (is_mt7961(bdev->chip_id)) {
		err = btmtk_load_rom_patch_79xx(bdev, BT_DOWNLOAD);
		if (err < 0) {
			BTMTK_ERR("%s: btmtk_load_rom_patch_79xx bt patch failed!", __func__);
			return err;
		}
	} else {
		BTMTK_WARN("%s: unknown chip id (%d)", __func__, bdev->chip_id);
}

	return err;
}