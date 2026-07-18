/*
 * Copyright (C) 2018-2020 AICSemi Ltd.
 *
 * All Rights Reserved
 */

//#include <config.h>
#include <stdio.h>
//#include <errno.h>
#include <string.h>
#include "porting.h"
#include "log.h"
#include "ite/ite_wifi.h"
#if defined(CFG_MMC_ENABLE)
#include "ite/ite_sdio.h"
#else
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio.h>
#endif

#include "lwip/errno.h"
#include "sdio_def.h"
#include "sdio_port.h"

#include "lmac_msg.h"
#include "rwnx_main.h"
#include "rtos_al.h"
#include "fhost_rx.h"
#include "wifi.h"

#define SDIOM_MAX_FUNCS 3

extern bool_l func_flag_tx;
extern bool_l func_flag_rx;

extern struct rwnx_hw* g_rwnx_hw;

#define CONFIG_TXMSG_TEST_EN    0

#define IRQF_TRIGGER_HIGH  1
#define IRQF_TRIGGER_LOW  2
#define FUNC_0  0
#define FUNC_1  1
#define FUNC_2  2

//#define SDIO_PORT_DEBUG
#define SDIO_DT_MODE_ADDR	0x0f
#define SDIO_FBR_SYSADDR_0	0x15c
//cache
#define SYS_CACHE_LINE_SIZE  32U
#define WCN_CACHE_ALIGNED(addr) (((uint32_t)(addr) + (SYS_CACHE_LINE_SIZE - 1)) & (~(SYS_CACHE_LINE_SIZE - 1)))
#define WCN_CACHE_ALIGN_VARIABLE    __attribute__((aligned(SYS_CACHE_LINE_SIZE)))
#define IS_WCN_CACHE_ALIGNED(addr) !((uint32_t)(addr) & (SYS_CACHE_LINE_SIZE - 1))
WCN_CACHE_ALIGN_VARIABLE static uint32_t sdiom_cache_align_addr[SYS_CACHE_LINE_SIZE/sizeof(uint32_t)];
WCN_CACHE_ALIGN_VARIABLE static uint32_t sdiom_cache_align_data[SYS_CACHE_LINE_SIZE/sizeof(uint32_t)];

static struct sdio_func *sdio_function[SDIOM_MAX_FUNCS];
static struct sdio_func sdio_function_inf[SDIOM_MAX_FUNCS];
#ifdef CONFIG_AIC_SDIO_INT_PINNUM
static unsigned int sdio_gpio_num = CONFIG_AIC_SDIO_INT_PINNUM;
#else
static unsigned int sdio_gpio_num = 0xFFFFFFFF;
#endif
static uint32_t sdio_block_size;
typedef void (*SDIO_ISR_FUNC)(void);

SDIO_ISR_FUNC g_sdio_isr_func = NULL;

static int msgcfm_poll_en = 1;
static rtos_semaphore sdio_rx_sema = NULL;
struct aic_sdio_dev sdio_dev = {NULL,};
static rtos_task_handle sdio_task_hdl = NULL;

#if (FHOST_RX_SW_VER == 3)
static struct sdio_buf_node_s sdio_rx_buf_node[SDIO_RX_BUF_COUNT];
WCN_CACHE_ALIGN_VARIABLE static uint8_t sdio_rx_buf_pool[SDIO_RX_BUF_COUNT][SDIO_RX_BUF_SIZE];
static struct sdio_buf_list_s sdio_rx_buf_list;
#endif

#define DRV_NAME "AIC8800"
#define VENDOR_ID_8800D  0x5449
#define DEVICE_ID_8800D  0x0145
#define VENDOR_ID_8800D80  0xC8A1
#define DEVICE_ID_8800D80  0x0082

struct sdio_func *wifi_sdio_func = NULL;

static const struct sdio_device_id sdio_ids[] = {
	//{ SDIO_DEVICE(SDIO_ANY_ID, SDIO_ANY_ID),.driver_data = NULL}
	#ifdef CONFIG_AIC8801
	{ SDIO_DEVICE(VENDOR_ID_8800D, DEVICE_ID_8800D),.driver_data = NULL}
    #elif CONFIG_AIC8800D80
	{ SDIO_DEVICE(VENDOR_ID_8800D80, DEVICE_ID_8800D80),.driver_data = NULL}
    #endif
};

extern void ITEAIC8800_sdio_wifi_init(void);

static int aic_drv_init(struct sdio_func *func, const struct sdio_device_id *id)
{
    printk("====>aic_drv_init: hook sdio_func!! vendor=0x%04x device=0x%04x class=0x%02x\n",
    	func->vendor, func->device, func->class);
    wifi_sdio_func = NULL;
    wifi_sdio_func = func;
#ifdef CONFIG_AIC8801
	if ((func->vendor == VENDOR_ID_8800D) && (func->device == DEVICE_ID_8800D)) {
#elif CONFIG_AIC8800D80
    if ((func->vendor == VENDOR_ID_8800D80) && (func->device == DEVICE_ID_8800D80)) {
#endif
		ITEAIC8800_freertos_init();
	}
    return 0;
}

static void aic_dev_remove(struct sdio_func *func)
{
    printk("====>aic_dev_remove()\n");
    return;
}

struct sdio_drv_priv {
	struct sdio_driver wifi_drv;
	int drv_registered;
};

static struct sdio_drv_priv sdio_drvpriv = {
	.wifi_drv.probe = aic_drv_init,
	.wifi_drv.remove = aic_dev_remove,
	.wifi_drv.name = (char*)DRV_NAME,
	.wifi_drv.id_table = sdio_ids,
};

int mmpAicWifiDriverRegister(void)
{
	int ret;

	printk("====>Do mmpAicWifiDriverRegister....\n");
	ret = sdio_register_driver(&sdio_drvpriv.wifi_drv);

	return ret;
}

bool sdio_readb_cmd52(uint32_t addr, uint8_t *data)
{
    int err;
    uint8_t  val;

    //AIC_LOG_PRINTF("sdio_readb_cmd52, addr: 0x%x", addr);
    sdio_claim_host(sdio_function[FUNC_1]);
    val = sdio_readb(sdio_function[FUNC_1], addr, &err);
    if(err) {
        AIC_LOG_PRINTF("sdio_readb_cmd52 fail %d!\n", err);
	sdio_release_host(sdio_function[FUNC_1]);
        return err;
    }
    *data = val;
    sdio_release_host(sdio_function[FUNC_1]);
    //AIC_LOG_PRINTF("sdio_readb_cmd52, val=0x%x\n", val);
    return TRUE;
}

bool sdio_readb_cmd52_func2(uint32_t addr, uint8_t *data)
{
    int err;
    uint8_t  val;

    //AIC_LOG_PRINTF("sdio_readb_cmd52, addr: 0x%x", addr);
    sdio_claim_host(sdio_function[FUNC_2]);
    val = sdio_readb(sdio_function[FUNC_2], addr, &err);
    if (err != TRUE){
        AIC_LOG_PRINTF("sdio_readb_cmd52 fail0!\n");
        sdio_release_host(sdio_function[FUNC_2]);
        return FALSE;
    }

    //AIC_LOG_PRINTF("sdio_readb_cmd52, val=0x%x", val);
    *data = val;
    sdio_release_host(sdio_function[FUNC_2]);
    return TRUE;
}

bool sdio_writeb_cmd52(uint32_t addr, uint8_t data)
{
    int err;
    //AIC_LOG_PRINTF("sdio_writeb_cmd52, addr: 0x%x\n", addr);
    sdio_claim_host(sdio_function[FUNC_1]);
    sdio_writeb(sdio_function[FUNC_1], data, addr, &err);
    if(err) {
        AIC_LOG_PRINTF("sdio_writeb_cmd52 fail %d!\n", err);
	sdio_release_host(sdio_function[FUNC_1]);
        return err;
    }

    sdio_release_host(sdio_function[FUNC_1]);

    //AIC_LOG_PRINTF("sdio_writeb_cmd52, data=0x%x\n", data);

    return TRUE;
}

bool sdio_writeb_cmd52_func2(uint32_t addr, uint8_t data)
{
    int err;

    sdio_claim_host(sdio_function[FUNC_2]);
    sdio_writeb(sdio_function[FUNC_2], data, addr, &err);
    if(err != TRUE){
	    AIC_LOG_PRINTF("sdio_writeb_cmd52 fail0!\n");
	    sdio_release_host(sdio_function[FUNC_2]);
	    return err;
    }
    sdio_release_host(sdio_function[FUNC_2]);

    //AIC_LOG_PRINTF("sdio_writeb_cmd52, addr 0x%x, data=0x%x", addr, data);

    return TRUE;
}

bool sdio_read_cmd53(uint32_t dataPort, uint8_t *dat, size_t size)
{
    int ret = 0;
    uint32_t rx_blocks = 0, blksize = 0;
    //char* temp_addr, *temp_addr2;
    //static int init_malloc = 0;
    //sdiom_cache_align_addr[0] = dataPort;
    if (size > sdio_block_size)
    {
        rx_blocks = (size + sdio_block_size - 1) / sdio_block_size;
        blksize = sdio_block_size;
    }
    else
    {
        blksize = size;
        rx_blocks = 1;
    }

    #if 0
    if (init_malloc == 0)
    {
        temp_addr = (char*)OS_MemAlloc(8192+0x3f);
        init_malloc = 1;
    }

    if (temp_addr)
    {
        temp_addr2 = temp_addr + (0x3f - (int)temp_addr&0x3f)+1;
        //AIC_LOG_PRINTF("sdio_read_cmd53 11, add1=0x%x, addr2=0x%x, dataPort=0x%x, siez=%d", temp_addr, temp_addr2, dataPort, size);
        sdio_claim_host(sdio_function[FUNC_1]);
        ret = sdio_memcpy_fromio(sdio_function[FUNC_1], temp_addr2, sdiom_cache_align_addr[0], rx_blocks*blksize);
        memset(dat, 0, size);
        memcpy(dat, temp_addr2, size);
    }
    #else
    sdio_claim_host(sdio_function[FUNC_1]);
    ret = sdio_memcpy_fromio(sdio_function[FUNC_1], dat, dataPort, rx_blocks*blksize);
    #endif
    sdio_release_host(sdio_function[FUNC_1]);
    if(ret != TRUE){
        AIC_LOG_PRINTF("sdio_read_cmd53 size = %d, fail!\n", size);
        return FALSE;
    }
    //AIC_LOG_PRINTF("sdio_read_cmd53 12, dat0=0x%x, dat2=0x%x, dat3=0x%x, dat4=0x%x", dat[0], dat[1], dat[2], dat[3]);
    return TRUE;
}

bool sdio_write_cmd53(uint32_t dataPort, uint8_t *dat, size_t size)
{
    int ret = 0;
    //char* temp_addr, *temp_addr2;
    uint32_t tx_blocks = 0, blksize = 0;

    sdiom_cache_align_addr[0] = dataPort;

    if (size > sdio_block_size)
    {
        tx_blocks = (size + sdio_block_size - 1) / sdio_block_size;
        blksize = sdio_block_size;
    }
    else
    {
        blksize = size;
        tx_blocks = 1;
    }

    size = blksize * tx_blocks;

#if 0
    temp_addr = (char*)OS_MemAlloc(size+0x3f);

    temp_addr2 = temp_addr + (0x3f - (int)temp_addr&0x3f)+1;
    memcpy(temp_addr2, dat, size);

    //AIC_LOG_PRINTF("sdio_write_cmd53 11,add1=0x%x, addr2=0x%x, dataPort=0x%x, siez=%d", temp_addr, temp_addr2, dataPort, size);
#endif

    //sdio_claim_host(sdio_function[FUNC_1]);
    ret = sdio_memcpy_toio(sdio_function[FUNC_1], sdiom_cache_align_addr[0], dat, size);
    //ret = sdio_memcpy_toio(sdio_function[FUNC_1], sdiom_cache_align_addr[0], temp_addr2, blksize*tx_blocks);
    //sdio_release_host(sdio_function[FUNC_1]);
    //free(temp_addr);
    if(ret != TRUE){
        AIC_LOG_PRINTF("sdio_write_cmd53 size = %d, fail!!\n", size);
        return FALSE;
    }

    return TRUE;
}

void sdio_release_func2(void)
{
    int ret = 0;
    struct aic_sdio_dev *sdiodev = &sdio_dev;
    AIC_LOG_PRINTF("%s\n", __func__);
    //sdio_claim_host(sdio_function[FUNC_2]);
    ret = sdio_writeb_cmd52_func2(sdiodev->sdio_reg.intr_config_reg, 0x0);
    if (ret < 0) {
        AIC_LOG_PRINTF("reg:%d write failed!\n", sdiodev->sdio_reg.intr_config_reg);
    }
    //sdio_release_irq(sdio_function[FUNC_2]);
    //sdio_release_host(sdio_function[FUNC_2]);
}

int tx_aggr_counter = MAX_AGGR_TXPKT_CNT + DATA_FLOW_CTRL_THRESH;
int aicwf_sdio_flow_ctrl_msg(void)
{
    int ret = -1;
    u8 fc_reg = 0;
    u32 count = 0;

    while (true) {
        struct aic_sdio_dev *sdiodev = &sdio_dev;
        rtos_mutex_lock(sdiodev->bus_txrx, -1);
        ret = sdio_readb_cmd52(sdiodev->sdio_reg.flow_ctrl_reg, &fc_reg);
        rtos_mutex_unlock(sdiodev->bus_txrx);
        if (ret == FALSE) {
            AIC_LOG_PRINTF("%s, reg read failed\n", __func__);
            return -1;
        }

        if (g_rwnx_hw->chipid == PRODUCT_ID_AIC8801 || g_rwnx_hw->chipid == PRODUCT_ID_AIC8800DC ||
            g_rwnx_hw->chipid == PRODUCT_ID_AIC8800DW) {
            fc_reg = fc_reg & SDIOWIFI_FLOWCTRL_MASK_REG;
        }

        if (fc_reg != 0) {
            ret = fc_reg;
            if (ret > tx_aggr_counter) {
                ret = tx_aggr_counter;
            }
            return ret;
        } else {
            if (count >= FLOW_CTRL_RETRY_COUNT) {
                ret = -fc_reg;
                AIC_LOG_PRINTF("msg fc:%d\n", ret);
                break;
            }
            count++;
            if (count < 30)
                udelay(200);
            else if(count < 40)
                rtos_msleep(2);
            else
                rtos_msleep(10);
        }
    }

    return ret;
}

int aicwf_sdio_flow_ctrl(void)
{
    int ret = -1;
    u8 fc_reg = 0;
    u32 count = 0;

    while (true) {
        struct aic_sdio_dev *sdiodev = &sdio_dev;
        rtos_mutex_lock(sdiodev->bus_txrx, -1);
        ret = sdio_readb_cmd52(sdiodev->sdio_reg.flow_ctrl_reg, &fc_reg);
        rtos_mutex_unlock(sdiodev->bus_txrx);
        if (ret == FALSE) {
            AIC_LOG_PRINTF("%s, reg read failed\n", __func__);
            return -1;
        }

        if (g_rwnx_hw->chipid == PRODUCT_ID_AIC8801 || g_rwnx_hw->chipid == PRODUCT_ID_AIC8800DC ||
            g_rwnx_hw->chipid == PRODUCT_ID_AIC8800DW) {
            fc_reg = fc_reg & SDIOWIFI_FLOWCTRL_MASK_REG;
        }

        if (fc_reg > 2) {
            ret = fc_reg;
            if (ret > tx_aggr_counter) {
                ret = tx_aggr_counter;
            }
            return ret;
        } else {
            if (count >= FLOW_CTRL_RETRY_COUNT) {
                ret = -fc_reg;
                AIC_LOG_PRINTF("data fc:%d\n", ret);
                break;
            }
            count++;
            if (count < 30)
                udelay(200);
            else if(count < 40)
                rtos_msleep(2);
            else
                rtos_msleep(10);
        }
    }

    return ret;
}

int aicwf_sdio_send_msg(u8 *buf, uint count)
{
    int ret = 0;
    struct aic_sdio_dev *sdiodev = &sdio_dev;
    rtos_mutex_lock(sdiodev->bus_txrx, -1);
    if (!func_flag_tx){
        sdio_claim_host(sdio_function[FUNC_1]);
        ret = sdio_writesb(sdio_function[FUNC_1], sdiodev->sdio_reg.wr_fifo_addr, buf, count);
        sdio_release_host(sdio_function[FUNC_1]);
    } else {
        sdio_claim_host(sdio_function[FUNC_2]);
        ret = sdio_writesb(sdio_function[FUNC_2], sdiodev->sdio_reg.wr_fifo_addr, buf, count);
        sdio_release_host(sdio_function[FUNC_2]);
    }
    rtos_mutex_unlock(sdiodev->bus_txrx);
    return ret;
}

int aicwf_sdio_send_pkt(u8 *buf, uint count)
{
    int ret = 0;
    struct aic_sdio_dev *sdiodev = &sdio_dev;
    rtos_mutex_lock(sdiodev->bus_txrx, -1);
    sdio_claim_host(sdio_function[FUNC_1]);
    ret = sdio_writesb(sdio_function[FUNC_1], sdiodev->sdio_reg.wr_fifo_addr, buf, count);
    sdio_release_host(sdio_function[FUNC_1]);
    rtos_mutex_unlock(sdiodev->bus_txrx);
    return ret;
}

int aicwf_sdio_recv_pkt(u8 *buf, u32 size, u8 msg)
{
    int ret;
    if ((!buf) || (!size)) {
        return -EINVAL;;
    }

    struct aic_sdio_dev *sdiodev = &sdio_dev;
    rtos_mutex_lock(sdiodev->bus_txrx, -1);
    if (g_rwnx_hw->chipid == PRODUCT_ID_AIC8801 || g_rwnx_hw->chipid == PRODUCT_ID_AIC8800D80) {
        sdio_claim_host(sdio_function[FUNC_1]);
        ret = sdio_readsb(sdio_function[FUNC_1], buf, sdiodev->sdio_reg.rd_fifo_addr, size);
        sdio_release_host(sdio_function[FUNC_1]);
    } else if (g_rwnx_hw->chipid == PRODUCT_ID_AIC8800DC || g_rwnx_hw->chipid == PRODUCT_ID_AIC8800DW) {
        if (!func_flag_rx) {
            sdio_claim_host(sdio_function[FUNC_1]);
            ret = sdio_readsb(sdio_function[FUNC_1], buf, sdiodev->sdio_reg.rd_fifo_addr, size);
            sdio_release_host(sdio_function[FUNC_1]);
        } else {
            if(!msg) {
                sdio_claim_host(sdio_function[FUNC_1]);
                ret = sdio_readsb(sdio_function[FUNC_1], buf, sdiodev->sdio_reg.rd_fifo_addr, size);
                sdio_release_host(sdio_function[FUNC_1]);
            } else {
                sdio_claim_host(sdio_function[FUNC_2]);
                ret = sdio_readsb(sdio_function[FUNC_2], buf, sdiodev->sdio_reg.rd_fifo_addr, size);
                sdio_release_host(sdio_function[FUNC_2]);
            }
        }
    }
    rtos_mutex_unlock(sdiodev->bus_txrx);

    return ret;
}

int aicwf_sdio_tx_msg(u8 *buf, uint count, u8 *out)
{
    int err = 0;
    u16 len;
    u8 *payload = buf;
    u16 payload_len = (u16)count;
    u8 adjust_str[4] = {0, 0, 0, 0};
    int adjust_len = 0;
    int buffer_cnt = 0;
    u8 retry = 0;
    struct aic_sdio_dev *sdiodev = &sdio_dev;

    len = payload_len;
    if ((len % TX_ALIGNMENT) != 0) {
        adjust_len = (len + TX_ALIGNMENT) & ~(TX_ALIGNMENT - 1); //roundup(len, TX_ALIGNMENT);
        memcpy(payload+payload_len, adjust_str, (adjust_len - len));
        payload_len += (adjust_len - len);
    }
    len = payload_len;

    //link tail is necessary
    if (len % SDIOWIFI_FUNC_BLOCKSIZE != 0) {
        memset(payload+payload_len, 0, TAIL_LEN);
        payload_len += TAIL_LEN;
        len = (payload_len/SDIOWIFI_FUNC_BLOCKSIZE + 1) * SDIOWIFI_FUNC_BLOCKSIZE;
    } else
        len = payload_len;

    if (g_rwnx_hw->chipid == PRODUCT_ID_AIC8801 || g_rwnx_hw->chipid == PRODUCT_ID_AIC8800D80) {
        buffer_cnt = aicwf_sdio_flow_ctrl_msg();
        while ((buffer_cnt <= 0 || (buffer_cnt > 0 && len > (buffer_cnt * BUFFER_SIZE))) && retry < 10) {
            retry++;
            buffer_cnt = aicwf_sdio_flow_ctrl_msg();
        }
    } else if (g_rwnx_hw->chipid == PRODUCT_ID_AIC8800DC || g_rwnx_hw->chipid == PRODUCT_ID_AIC8800DW) {
        if (!func_flag_tx) {
            buffer_cnt = aicwf_sdio_flow_ctrl_msg();
            while ((buffer_cnt <= 0 || (buffer_cnt > 0 && len > (buffer_cnt * BUFFER_SIZE))) && retry < 10) {
                retry++;
                buffer_cnt = aicwf_sdio_flow_ctrl_msg();
            }
        }
    }

    //down(&sdiodev->tx_priv->cmd_txsema);

    if (g_rwnx_hw->chipid == PRODUCT_ID_AIC8801) {
        if (buffer_cnt > 0 && len <= (buffer_cnt * BUFFER_SIZE)) {
            //AIC_LOG_PRINTF("aicwf_sdio_send_pkt, len=%d\n", len);
            err = aicwf_sdio_send_pkt(payload, len);
            if (err) {
                AIC_LOG_PRINTF("aicwf_sdio_send_pkt fail %d\n", err);
                goto txmsg_exit;
            }
            if (msgcfm_poll_en && out) {
                u8 intstatus = 0;
                u32 data_len;
                int ret, idx;
                udelay(100);
                for (idx = 0; idx < 8; idx++) {
                    rtos_mutex_lock(sdiodev->bus_txrx, -1);
                    ret = sdio_readb_cmd52(sdiodev->sdio_reg.block_cnt_reg, &intstatus);
                    rtos_mutex_unlock(sdiodev->bus_txrx);
                    while ((ret == FALSE) || (intstatus & SDIO_OTHER_INTERRUPT)) {
                        AIC_LOG_PRINTF("ret=%d, intstatus=%x\n",ret, intstatus);
                        rtos_mutex_lock(sdiodev->bus_txrx, -1);
                        ret = sdio_readb_cmd52(sdiodev->sdio_reg.block_cnt_reg, &intstatus);
                        rtos_mutex_unlock(sdiodev->bus_txrx);
                    }
                    AIC_LOG_PRINTF("[%d] intstatus=%d\n", idx, intstatus);
                    if (intstatus > 0) {
                        if (intstatus < 64) {
                            data_len = intstatus * SDIOWIFI_FUNC_BLOCKSIZE;
                        } else {
                            u8 byte_len = 0;
                            rtos_mutex_lock(sdiodev->bus_txrx, -1);
                            ret = sdio_readb_cmd52(sdiodev->sdio_reg.bytemode_len_reg, &byte_len);
                            rtos_mutex_unlock(sdiodev->bus_txrx);
                            if (ret) {
                                AIC_LOG_PRINTF("byte mode len read err %d\r\n", ret);
                                err = ret;
                                goto txmsg_exit;
                            }
                            AIC_LOG_PRINTF("byte mode len=%d\r\n", byte_len);
                            data_len = byte_len * 4; //byte_len must<= 128
                        }
                        if (data_len) {
                            ret = aicwf_sdio_recv_pkt(out, data_len, 0);
                            if (ret) {
                                AIC_LOG_PRINTF("aicwf_sdio_tx_msg, recv pkt err %d\r\n", ret);
                                err = ret;
                                goto txmsg_exit;
                            }
                            AIC_LOG_PRINTF("recv pkt done len=%d\r\n", data_len);
                        }
                        break;
                    }
                }
            }
        } else {
            AIC_LOG_PRINTF("tx msg fc retry fail\n");
            //up(&sdiodev->tx_priv->cmd_txsema);
            return -1;
        }
    } else if (g_rwnx_hw->chipid == PRODUCT_ID_AIC8800DC || g_rwnx_hw->chipid == PRODUCT_ID_AIC8800DW) {
        if (((!func_flag_tx) && (buffer_cnt > 0 && len <= (buffer_cnt * BUFFER_SIZE))) || func_flag_tx) {
            //AIC_LOG_PRINTF("aicwf_sdio_send_pkt, len=%d\n", len);
            err = aicwf_sdio_send_msg(payload, len);
            if (err) {
                AIC_LOG_PRINTF("aicwf_sdio_send_pkt fail%d\n", err);
                goto txmsg_exit;
            }
            if (msgcfm_poll_en && out) {
                u8 intstatus = 0;
                u32 data_len;
                int ret, idx;
                udelay(100);
                for (idx = 0; idx < 8; idx++) {
                    rtos_mutex_lock(sdiodev->bus_txrx, -1);
                    ret = sdio_readb_cmd52_func2(sdiodev->sdio_reg.block_cnt_reg, &intstatus);
                    rtos_mutex_unlock(sdiodev->bus_txrx);
                    while ((ret == FALSE) || (intstatus & SDIO_OTHER_INTERRUPT)) {
                        AIC_LOG_PRINTF("ret=%d, intstatus=%x\n",ret, intstatus);
                        rtos_mutex_lock(sdiodev->bus_txrx, -1);
                        ret = sdio_readb_cmd52_func2(sdiodev->sdio_reg.block_cnt_reg, &intstatus);
                        rtos_mutex_unlock(sdiodev->bus_txrx);
                    }
                    AIC_LOG_PRINTF("[%d] intstatus=%d\n", idx, intstatus);
                    if (intstatus > 0) {
                        if (intstatus < 64) {
                            data_len = intstatus * SDIOWIFI_FUNC_BLOCKSIZE;
                        } else {
                            u8 byte_len = 0;
                            rtos_mutex_lock(sdiodev->bus_txrx, -1);
                            ret = sdio_readb_cmd52_func2(sdiodev->sdio_reg.bytemode_len_reg, &byte_len);
                            rtos_mutex_unlock(sdiodev->bus_txrx);
                            if (ret) {
                                AIC_LOG_PRINTF("byte mode len read err %d\r\n", ret);
                                err = ret;
                                goto txmsg_exit;
                            }
                            AIC_LOG_PRINTF("byte mode len=%d\r\n", byte_len);
                            data_len = byte_len * 4; //byte_len must<= 128
                        }
                        if (data_len) {
                            ret = aicwf_sdio_recv_pkt(out, data_len, 1);
                            if (ret) {
                                AIC_LOG_PRINTF("recv pkt err %d\r\n", ret);
                                err = ret;
                                goto txmsg_exit;
                            }
                            AIC_LOG_PRINTF("recv pkt done len=%d\r\n", data_len);
                        }
                        break;
                    }
                }
            }
        } else {
            AIC_LOG_PRINTF("tx msg fc retry fail\n");
            //up(&sdiodev->tx_priv->cmd_txsema);
            return -1;
        }
    }else if (g_rwnx_hw->chipid == PRODUCT_ID_AIC8800D80) {
        if (buffer_cnt > 0 && len <= (buffer_cnt * BUFFER_SIZE)) {
            //AIC_LOG_PRINTF("aicwf_sdio_send_pkt, len=%d\n", len);
            err = aicwf_sdio_send_pkt(payload, len);
            if (err) {
                AIC_LOG_PRINTF("aicwf_sdio_send_pkt fail%d\n", err);
                goto txmsg_exit;
            }
            if (msgcfm_poll_en && out) {
                u8 intstatus = 0;
                u32 data_len;
                int ret, idx;
                udelay(100);
                for (idx = 0; idx < 8; idx++) {
                    rtos_mutex_lock(sdiodev->bus_txrx, -1);
                    ret = sdio_readb_cmd52(sdiodev->sdio_reg.misc_int_status_reg, &intstatus);
                    rtos_mutex_unlock(sdiodev->bus_txrx);
                    while (ret == FALSE) {
                        AIC_LOG_PRINTF("ret=%d, intstatus=%x\n",ret, intstatus);
                        rtos_mutex_lock(sdiodev->bus_txrx, -1);
                        ret = sdio_readb_cmd52(sdiodev->sdio_reg.misc_int_status_reg, &intstatus);
                        rtos_mutex_unlock(sdiodev->bus_txrx);
                    }

                    if (intstatus & SDIO_OTHER_INTERRUPT) {
                        u8 int_pending;
                        rtos_mutex_lock(sdiodev->bus_txrx, -1);
                        ret = sdio_readb_cmd52(sdiodev->sdio_reg.sleep_reg, &int_pending);
                        rtos_mutex_unlock(sdiodev->bus_txrx);
                        if (ret == FALSE) {
                            AIC_LOG_PRINTF("reg:%d read failed!\n", sdiodev->sdio_reg.sleep_reg);
                        }
                        int_pending &= ~0x01; // dev to host soft irq
                        ret = sdio_writeb_cmd52(sdiodev->sdio_reg.sleep_reg, int_pending);
                        if (ret == FALSE) {
                            AIC_LOG_PRINTF("reg:%d write failed!\n", sdiodev->sdio_reg.sleep_reg);
                        }
                    }
                    AIC_LOG_PRINTF("[%d] intstatus=%d\n", idx, intstatus);
                    if (intstatus > 0) {
                        uint8_t intmaskf2 = intstatus | (0x1UL << 3);
                        if (intstatus == 120U) {    // byte mode
                            u8 byte_len = 0;
                            rtos_mutex_lock(sdiodev->bus_txrx, -1);
                            ret = sdio_readb_cmd52(sdiodev->sdio_reg.bytemode_len_reg, &byte_len);
                            rtos_mutex_unlock(sdiodev->bus_txrx);
                            if (ret) {
                                AIC_LOG_PRINTF("byte mode len read err %d\r\n", ret);
                            }
                            AIC_LOG_PRINTF("byte mode len=%d\r\n", byte_len);
                            data_len = byte_len * 4; //byte_len must<= 128
                        } else {
                            data_len = (intstatus & 0x7FU) * SDIOWIFI_FUNC_BLOCKSIZE;
                        }
                        if (data_len) {
                            ret = aicwf_sdio_recv_pkt(out, data_len, 0);
                            if (ret) {
                                AIC_LOG_PRINTF("recv pkt err %d\r\n", ret);
                                err = ret;
                                goto txmsg_exit;
                            }
                            AIC_LOG_PRINTF("recv pkt done len=%d\r\n", data_len);
                        }
                        break;
                    }
                }
            }
        } else {
            AIC_LOG_PRINTF("tx msg fc retry fail\n");
            return -1;
        }
    }

txmsg_exit:
    return err;
}

int aicwf_sdio_aggr(struct aic_sdio_dev *sdiodev, u8 *pkt_data, u32 pkt_len);
void aicwf_sdio_aggr_send(struct aic_sdio_dev *sdiodev);
void aicwf_sdio_aggrbuf_reset(struct aic_sdio_dev *sdiodev);

int aicwf_sdio_send_check()
{
    struct aic_sdio_dev *sdiodev = &sdio_dev;
    u32 aggr_len = 0;

    aggr_len = (sdiodev->tail - sdiodev->head);
    if(((sdiodev->aggr_count == 0) && (aggr_len != 0))
        || ((sdiodev->aggr_count != 0) && (aggr_len == 0))) {
        if (aggr_len > 0)
            aicwf_sdio_aggrbuf_reset(sdiodev);
        AIC_LOG_PRINTF("aggr_count=%d, aggr_len=%d, check fail\n", sdiodev->aggr_count, aggr_len);
        return -1;
    }

    if (sdiodev->aggr_count == (sdiodev->fw_avail_bufcnt - DATA_FLOW_CTRL_THRESH)) {
        if (sdiodev->aggr_count > 0) {
            sdiodev->fw_avail_bufcnt -= sdiodev->aggr_count;
            aic_dbg("cnt equals\n");
            aicwf_sdio_aggr_send(sdiodev); //send and check the next pkt;
            return 1;
        }
    }

    return 0;
}

int aicwf_sdio_send(u8 *pkt_data, u32 pkt_len, bool last)
{
    struct sk_buff *pkt;
    struct aic_sdio_dev *sdiodev = &sdio_dev;
    int retry_times = 0;
    int max_retry_times = 5;

#if 0
    if (sdiodev->fw_avail_bufcnt <= 2) { // init val
        sdiodev->fw_avail_bufcnt = aicwf_sdio_flow_ctrl();
        while(sdiodev->fw_avail_bufcnt <=2 && retry_times < max_retry_times) {
            retry_times++;
            sdiodev->fw_avail_bufcnt = aicwf_sdio_flow_ctrl();
        }
        if(sdiodev->fw_avail_bufcnt <= 2) {
            AIC_LOG_PRINTF("fc retry %d fail\n", sdiodev->fw_avail_bufcnt);
            goto done;
        }
    }
    aic_dbg("ava_cnt=%d,last=%d\n",sdiodev->fw_avail_bufcnt,last);
#endif
retry:
    #if 1
    if(sdiodev==NULL || sdiodev->tail==NULL)
        AIC_LOG_PRINTF("null error\n");
    if (aicwf_sdio_aggr(sdiodev, pkt_data, pkt_len)) {
        if (sdiodev->aggr_end) {
            sdiodev->fw_avail_bufcnt -= sdiodev->aggr_count;
            aicwf_sdio_aggr_send(sdiodev);
            goto retry;
        } else {
            aicwf_sdio_aggrbuf_reset(sdiodev);
            AIC_LOG_PRINTF("add aggr pkts failed!\n");
            goto done;
        }
    }

    //when aggr finish or there is cmd to send, just send this aggr pkt to fw
    //if ((int)atomic_read(&sdiodev->tx_priv->tx_pktcnt) == 0 || sdiodev->tx_priv->cmd_txstate) { //no more pkt send it!
    if (last || sdiodev->aggr_end) {
        sdiodev->fw_avail_bufcnt -= sdiodev->aggr_count;
        aicwf_sdio_aggr_send(sdiodev);
    } else {
        goto done;
    }
    #else
    aicwf_sdio_aggr(sdiodev, pkt_data, pkt_len);
    aicwf_sdio_aggr_send(sdiodev);
    #endif

done:
    return 0;
}

uint8_t crc8_ponl_107(uint8_t *p_buffer, uint16_t cal_size)
{
    uint8_t i;
    uint8_t crc = 0;
    if (cal_size==0) {
        return crc;
    }
    while (cal_size--) {
        for (i = 0x80; i > 0; i /= 2) {
            if (crc & 0x80)  {
                crc *= 2;
                crc ^= 0x07; //polynomial X8 + X2 + X + 1,(0x107)
            } else {
                crc *= 2;
            }
            if ((*p_buffer) & i) {
                crc ^= 0x07;
            }
        }
        p_buffer++;
    }
    return crc;
}

int aicwf_sdio_aggr(struct aic_sdio_dev *sdiodev, u8 *pkt_data, u32 pkt_len)
{
    struct rwnx_txhdr *txhdr = (struct rwnx_txhdr *)pkt_data;
    u8 *start_ptr = sdiodev->tail;
    u8 sdio_header[4];
    u8 adjust_str[4] = {0, 0, 0, 0};
    u32 curr_len = 0;
    int allign_len = 0;

    u32 aggr_len = sdiodev->len + ((pkt_len + sizeof(sdio_header) + (TX_ALIGNMENT - 1)) & (~(TX_ALIGNMENT - 1)));
    if ((aggr_len % TXPKT_BLOCKSIZE) != 0) {
        aggr_len += TAIL_LEN;
    }

    if (aggr_len > MAX_AGGR_TXPKT_LEN) {
        AIC_LOG_PRINTF("sdio aggr overflow:%d/%d/%d/%d\n", sdiodev->len, pkt_len, aggr_len, (u32)(sdiodev->aggr_buf_align - sdiodev->aggr_buf));
        sdiodev->aggr_end = 1;
        return -1;
    }

    #if 0
    sdio_header[0] =((pkt_len - sizeof(struct rwnx_txhdr) + sizeof(struct txdesc_api)) & 0xff);
    sdio_header[1] =(((pkt_len - sizeof(struct rwnx_txhdr) + sizeof(struct txdesc_api)) >> 8)&0x0f);
    sdio_header[2] = 0x01; //data
    sdio_header[3] = 0; //reserved

    memcpy(sdiodev->tail, (u8 *)&sdio_header, sizeof(sdio_header));
    sdiodev->tail += sizeof(sdio_header);
    //payload
    memcpy(sdiodev->tail, (u8 *)(long)&txhdr->sw_hdr->desc, sizeof(struct txdesc_api));
    sdiodev->tail += sizeof(struct txdesc_api); //hostdesc
    memcpy(sdiodev->tail, (u8 *)((u8 *)txhdr + txhdr->sw_hdr->headroom), pkt_len-txhdr->sw_hdr->headroom);
    sdiodev->tail += (pkt_len - txhdr->sw_hdr->headroom);
    #else
    sdio_header[0] =((pkt_len) & 0xff);
    sdio_header[1] =(((pkt_len) >> 8)&0x0f);
    sdio_header[2] = 0x01; //data
	if (g_rwnx_hw->chipid == PRODUCT_ID_AIC8801 || g_rwnx_hw->chipid == PRODUCT_ID_AIC8800DC ||
        g_rwnx_hw->chipid == PRODUCT_ID_AIC8800DW)
        sdio_header[3] = 0; //reserved
    else if (g_rwnx_hw->chipid == PRODUCT_ID_AIC8800D80)
	    sdio_header[3] = crc8_ponl_107(&sdio_header[0], 3); // crc8

    memcpy(sdiodev->tail, (u8 *)&sdio_header, sizeof(sdio_header));
    sdiodev->tail += sizeof(sdio_header);
    // hostdesc + payload
    memcpy(sdiodev->tail, (u8 *)pkt_data, pkt_len);
    sdiodev->tail += pkt_len;
    #endif

    //word alignment
    curr_len = sdiodev->tail - sdiodev->head;
    if (curr_len & (TX_ALIGNMENT - 1)) {
        allign_len = TX_ALIGNMENT - (curr_len & (TX_ALIGNMENT - 1));
        memcpy(sdiodev->tail, adjust_str, allign_len);
        sdiodev->tail += allign_len;
    }

    if (g_rwnx_hw->chipid == PRODUCT_ID_AIC8801 || g_rwnx_hw->chipid == PRODUCT_ID_AIC8800DC ||
        g_rwnx_hw->chipid == PRODUCT_ID_AIC8800DW) {
        start_ptr[0] = ((sdiodev->tail - start_ptr - 4) & 0xff);
        start_ptr[1] = (((sdiodev->tail - start_ptr - 4)>>8) & 0x0f);
    }

    #if 0
    if(!txhdr->sw_hdr->need_cfm) {
        kmem_cache_free(txhdr->sw_hdr->rwnx_vif->rwnx_hw->sw_txhdr_cache, txhdr->sw_hdr);
        skb_pull(pkt, txhdr->sw_hdr->headroom);
        consume_skb(pkt);
    }
    #endif

    sdiodev->len = sdiodev->tail - sdiodev->head;
    if (((sdiodev->len + (TXPKT_BLOCKSIZE - 1)) & ~(TXPKT_BLOCKSIZE - 1)) > MAX_AGGR_TXPKT_LEN) {
	printf("over MAX_AGGR_TXPKT_LEN\n");
        sdiodev->aggr_end = 1;
    }
    sdiodev->aggr_count++;
    if (sdiodev->aggr_count == sdiodev->fw_avail_bufcnt - DATA_FLOW_CTRL_THRESH) {
        sdiodev->aggr_end = 1;
    }

    return 0;
}

void aicwf_sdio_aggr_send(struct aic_sdio_dev *sdiodev)
{
    u8 *tx_buf = sdiodev->aggr_buf_align;
    int ret = 0;
    int curr_len = 0;

    if (sdiodev->aggr_count > 1) {
        //aic_dbg("sdio ac=%d\n",sdiodev->aggr_count);
    }

    //link tail is necessary
    curr_len = sdiodev->tail - sdiodev->head;
    if ((curr_len % TXPKT_BLOCKSIZE) != 0) {
        memset(sdiodev->tail, 0, TAIL_LEN);
        sdiodev->tail += TAIL_LEN;
    }

    sdiodev->len = sdiodev->tail - sdiodev->head;
    curr_len = (sdiodev->len + SDIOWIFI_FUNC_BLOCKSIZE - 1) / SDIOWIFI_FUNC_BLOCKSIZE * SDIOWIFI_FUNC_BLOCKSIZE;
    ret = aicwf_sdio_send_pkt(tx_buf, curr_len);
    if (ret < 0) {
        AIC_LOG_PRINTF("fail to send aggr pkt!\n");
    }

    aicwf_sdio_aggrbuf_reset(sdiodev);
}

void aicwf_sdio_aggrbuf_reset(struct aic_sdio_dev *sdiodev)
{
    sdiodev->tail = sdiodev->head;
    sdiodev->len = 0;
    sdiodev->aggr_count = 0;
    sdiodev->aggr_end = 0;
}

void aicwf_sdio_tx_init(void)
{
    int ret;
    struct aic_sdio_dev *sdiodev = &sdio_dev;
    sdiodev->aggr_buf = rtos_malloc(MAX_AGGR_TXPKT_LEN + AGGR_TXPKT_ALIGN_SIZE);
    if(!sdiodev->aggr_buf) {
        AIC_LOG_PRINTF("Alloc sdio tx aggr_buf failed!\n");
        return;
    }
    sdiodev->aggr_buf_align = (u8 *)(((uint32_t)sdiodev->aggr_buf + AGGR_TXPKT_ALIGN_SIZE - 1) & ~(AGGR_TXPKT_ALIGN_SIZE - 1));
    sdiodev->fw_avail_bufcnt = 0;
    //sdiodev->tx_pktcnt = 0;
    sdiodev->head = sdiodev->aggr_buf_align;
    aicwf_sdio_aggrbuf_reset(sdiodev);
    ret = rtos_mutex_create(&sdiodev->bus_txrx, "sdiodev->bus_txrx");
    if (ret) {
        AIC_LOG_PRINTF("Alloc sdio txrx mutex failed, ret=%d\n", ret);
    }
    AIC_LOG_PRINTF("sdio aggr_buf:%#x, aggr_buf_align:%#x\n", sdiodev->aggr_buf, sdiodev->aggr_buf_align);
}

void aicwf_sdio_tx_deinit(void)
{
    struct aic_sdio_dev *sdiodev = &sdio_dev;
    if (sdiodev->aggr_buf) {
        rtos_free(sdiodev->aggr_buf);
        sdiodev->aggr_buf = NULL;
        sdiodev->aggr_buf_align = NULL;
    }
    aicwf_sdio_aggrbuf_reset(sdiodev);
    rtos_mutex_delete(sdiodev->bus_txrx);
}

static __inline int gpio_request_irq(uint32_t gpio_id, uint32_t trig_type, int (* isr)(uint32_t, uint32_t))
{
	ithEnterCritical();
	ithGpioClearIntr(gpio_id);
    ithGpioRegisterIntrHandler(gpio_id, (ITHGpioIntrHandler)isr, ithGpioGet(gpio_id));
	ithGpioCtrlEnable(gpio_id, ITH_GPIO_INTR_LEVELTRIGGER); 		/* use level trigger mode */
	ithGpioCtrlEnable(gpio_id, trig_type);
	ithGpioEnableIntr(gpio_id);
	ithExitCritical();
    return TRUE;
}

static __inline void gpio_enable_irq(uint32_t gpio_id)
{
    ithGpioEnableIntr(gpio_id);
}

static __inline void gpio_disable_irq(uint32_t gpio_id)
{
    ithGpioDisableIntr(gpio_id);
}

bool sdio_host_enable_isr(bool enable)
{
#ifdef CONFIG_AIC_SDIO_INT_PINNUM
     if(enable)
        gpio_enable_irq(sdio_gpio_num);
     else
        gpio_disable_irq(sdio_gpio_num);
#endif
    return TRUE;
}

bool aic_sdio_set_block_size(unsigned int blksize)
{
    unsigned char blk[2];
    uint8_t in, out;
    int err_ret = 0;

    if ((blksize == 0) || (blksize > 512))
    {
        blksize = SDIOWIFI_FUNC_BLOCKSIZE;
    }
    #if 0
    blk[0] = blksize & 0x0ff;
    blk[1] = (blksize >> 8)&0x0ff;

    //sdio_claim_host(sdio_function[FUNC_0]);
    //sdio_writeb(sdio_function[FUNC_0], blk[0], SDIOWIFI_FBR_FUNC1_BLK_SIZE_REG, &err_ret);
    sdio_f0_writeb(func, blk[0], SDIOWIFI_FBR_FUNC1_BLK_SIZE_REG, &err_ret);
    if(err_ret != TRUE){
        AIC_LOG_PRINTF("aic_sdio_set_block_size fail 0!\n");
        //sdio_release_host(sdio_function[FUNC_0]);
        return FALSE;
    }

    //sdio_writeb(sdio_function[FUNC_0], blk[1], SDIOWIFI_FBR_FUNC1_BLK_SIZE_REG + 1, &err_ret);
    sdio_f0_writeb(func, blk[1], SDIOWIFI_FBR_FUNC1_BLK_SIZE_REG + 1, &err_ret);
    if(err_ret != TRUE){
        AIC_LOG_PRINTF("aic_sdio_set_block_size fail 1!\n");
        //sdio_release_host(sdio_function[FUNC_0]);
        return FALSE;
    }

    blk[0] = 0;
    blk[1] = 0;
    //blk[0] = sdio_readb(sdio_function[FUNC_0], SDIOWIFI_FBR_FUNC1_BLK_SIZE_REG, &err_ret);
    blk[0] = sdio_f0_readb(func, SDIOWIFI_FBR_FUNC1_BLK_SIZE_REG, &err_ret);
    if(err_ret != TRUE){
        AIC_LOG_PRINTF("sdio_readb 0x%d fail 0!\n", SDIOWIFI_FBR_FUNC1_BLK_SIZE_REG);
        //sdio_release_host(sdio_function[FUNC_0]);
        return FALSE;
    }

    //blk[1] = sdio_readb(sdio_function[FUNC_0], SDIOWIFI_FBR_FUNC1_BLK_SIZE_REG + 1, &err_ret);
    blk[1] = sdio_f0_readb(func, SDIOWIFI_FBR_FUNC1_BLK_SIZE_REG + 1, &err_ret);
    if(err_ret != TRUE){
        AIC_LOG_PRINTF("sdio_readb 0x%x fail 1!\n", SDIOWIFI_FBR_FUNC1_BLK_SIZE_REG + 1);
        //sdio_release_host(sdio_function[FUNC_0]);
        return FALSE;
    }

	sdio_release_host(sdio_function[FUNC_0]);

    if (((unsigned int)(blk[1] << 8) | blk[0]) != blksize) {
        AIC_LOG_PRINTF("aic_sdio_set_block_size fail, target=%d, readback=%d!\n", blksize, ((unsigned int)(blk[1] << 8) | blk[0]));
        return FALSE;
    }
    #endif
    sdio_block_size = blksize;
    return TRUE;
}

bool aic_sdio_set_block_size_func2(unsigned int blksize)
{
    unsigned char blk[2];
    uint8_t in, out;
    int err_ret = 0;

    if ((blksize == 0) || (blksize > 512))
    {
        blksize = SDIOWIFI_FUNC_BLOCKSIZE;
    }
    #if 0
    blk[0] = blksize & 0x0ff;
    blk[1] = (blksize >> 8)&0x0ff;

    sdio_claim_host(sdio_function[FUNC_0]);
    sdio_writeb(sdio_function[FUNC_0], blk[0], SDIOWIFI_FBR_FUNC2_BLK_SIZE_REG, &err_ret);
    if(err_ret != TRUE){
        AIC_LOG_PRINTF("aic_sdio_set_block_size fail0!\n");
        sdio_release_host(sdio_function[FUNC_0]);
        return FALSE;
    }

    sdio_writeb(sdio_function[FUNC_0], blk[1], SDIOWIFI_FBR_FUNC2_BLK_SIZE_REG + 1, &err_ret);
    if(err_ret != TRUE){
        AIC_LOG_PRINTF("aic_sdio_set_block_size fail0!\n");
        sdio_release_host(sdio_function[FUNC_0]);
        return FALSE;
    }

    blk[0] = 0;
    blk[1] = 0;
    blk[0] = sdio_readb(sdio_function[FUNC_0], SDIOWIFI_FBR_FUNC2_BLK_SIZE_REG, &err_ret);
    if(err_ret != TRUE){
        AIC_LOG_PRINTF("sdio_readb 0x%d fail0!\n", SDIOWIFI_FBR_FUNC2_BLK_SIZE_REG);
        sdio_release_host(sdio_function[FUNC_0]);
        return FALSE;
    }

    blk[1] = sdio_readb(sdio_function[FUNC_0], SDIOWIFI_FBR_FUNC2_BLK_SIZE_REG + 1, &err_ret);
    if(err_ret != TRUE){
        AIC_LOG_PRINTF("sdio_readb 0x%x fail0!\n", SDIOWIFI_FBR_FUNC2_BLK_SIZE_REG + 1);
        sdio_release_host(sdio_function[FUNC_0]);
        return FALSE;
    }

    sdio_release_host(sdio_function[FUNC_0]);

    if (((unsigned int)(blk[1] << 8) | blk[0]) != blksize) {
        AIC_LOG_PRINTF("aic_sdio_set_block_size fail, target=%d, readback=%d!\n", blksize, ((unsigned int)(blk[1] << 8) | blk[0]));
        return FALSE;
    }
    #endif
    sdio_block_size = blksize;
    return TRUE;
}

uint32_t sdio_get_block_size(void)
{
    return sdio_block_size;
}

bool sdio_set_clk(uint32_t clk)
{
    return TRUE;
}

bool sdio_set_bwidth(uint8_t bwidth)
{
    //if(SDIO_BUS_1_BIT == bwidth)
    //    return sdio_set_bus_width(USE_ONE_BUS);

    //if(SDIO_BUS_4_BIT == bwidth)
    //    return sdio_set_bus_width(USE_FOUR_BUS);
    return TRUE;
}

void sdio_host_isr (uint32_t gpio_id, uint32_t gpio_state)
{
    //AIC_LOG_PRINTF("sdio_host_isr gpio_id=%d\n", gpio_id);
    //if (SDHCI_INTR_STS_CARD_INTR & arg)
    {
	#if 0
	sdio_host_enable_isr(FALSE);
        if (g_sdio_isr_func)
        {
            g_sdio_isr_func();
        }
        #endif
    }
    rtos_semaphore_signal(sdio_rx_sema, true);
}

static void aicwf_sdio_irq_hdl(struct sdio_func *func)
{
	//AIC_LOG_PRINTF("aicwf_sdio_irq_hdl\n");
    rtos_semaphore_signal(sdio_rx_sema, false);
    //sdio_rx_task(func);
}


#if (FHOST_RX_SW_VER == 3)
void sdio_buf_init(void)
{
    int idx, ret;
    ret = rtos_mutex_create(&sdio_rx_buf_list.mutex, "sdio_rx_buf_list.mutex");
    if (ret) {
        aic_dbg("sdio rx buf mutex create fail: %d\n", ret);
        return;
    }
    co_list_init(&sdio_rx_buf_list.list);
    if (rtos_semaphore_create(&sdio_rx_buf_list.sdio_rx_node_sema, "sdio_rx_buf_list.sdio_rx_node_sema", SDIO_RX_BUF_COUNT, 0)) {
        ASSERT_ERR(0);
    }
    for (idx = 0; idx < SDIO_RX_BUF_COUNT; idx++) {
        struct sdio_buf_node_s *node = &sdio_rx_buf_node[idx];
        node->buf_raw = &sdio_rx_buf_pool[idx][0];
        node->buf = NULL;
        node->buf_len = 0;
        node->pad_len = 0;
        co_list_push_back(&sdio_rx_buf_list.list, &node->hdr);
        rtos_semaphore_signal(sdio_rx_buf_list.sdio_rx_node_sema, 0);
    }
    AIC_LOG_PRINTF("sdio_rx_node_sema initial count:%d\n", rtos_semaphore_get_count(sdio_rx_buf_list.sdio_rx_node_sema));
}

struct sdio_buf_node_s *sdio_buf_alloc(uint16_t buf_len)
{
    struct sdio_buf_node_s *node = NULL;

    int ret = rtos_semaphore_wait(sdio_rx_buf_list.sdio_rx_node_sema, 100);

    if (ret == 0) {
        rtos_mutex_lock(sdio_rx_buf_list.mutex, -1);
        node = (struct sdio_buf_node_s *)co_list_pop_front(&sdio_rx_buf_list.list);
        if (buf_len > SDIO_RX_BUF_SIZE) {
            uint8_t *buf_raw = rtos_malloc(buf_len + SYS_CACHE_LINE_SIZE);
            if (buf_raw == NULL) {
                AIC_LOG_PRINTF("sdio buf alloc fail(len=%d)!!!\n", buf_len);
                node->buf = NULL;
                node->buf_len = 0;
                node->pad_len = 0;
            } else {
                node->buf = WCN_CACHE_ALIGNED(buf_raw);
                node->buf_len = buf_len;
                node->pad_len = node->buf - buf_raw;
            }
        } else {
            node->buf = node->buf_raw;
            node->buf_len = buf_len;
            node->pad_len = 0;
        }
        rtos_mutex_unlock(sdio_rx_buf_list.mutex);
    }
    return node;
}

void sdio_buf_free(struct sdio_buf_node_s *node)
{
    rtos_mutex_lock(sdio_rx_buf_list.mutex, -1);
    if (node->buf_len == 0) {
        AIC_LOG_PRINTF("null sdio buf free, buf=%p\n", node->buf);
    } else if (node->buf_len > SDIO_RX_BUF_SIZE) {
        uint8_t *buf_raw = node->buf - node->pad_len;
        rtos_free(buf_raw);
    }
    node->buf = NULL;
    node->buf_len = 0;
    node->pad_len = 0;
    co_list_push_back(&sdio_rx_buf_list.list, &node->hdr);
    rtos_mutex_unlock(sdio_rx_buf_list.mutex);
    rtos_semaphore_signal(sdio_rx_buf_list.sdio_rx_node_sema, 0);
}
#endif

void sdio_rx_task(void *argv)
{
    struct rwnx_hw *rwnx_hw = (struct rwnx_hw *)argv;
    AIC_LOG_PRINTF("sdio_rx_task\n");
    while (1) {
	    int polling = 1;
        int32_t ret = rtos_semaphore_wait(sdio_rx_sema, 10); //polling to avoid irq missing
        //AIC_LOG_PRINTF("aft sdio sema\n");

        if ((aic_priority_mode == 3) && (sdio_datrx_priority >= tcpip_priority)) {
            rtos_task_set_priority(sdio_task_hdl, tcpip_priority - 1);
        }
        //aic_dbg("#*Si\n");

        if (ret == 1) {
            polling = 1;
        } else if (ret < 0) {
            AIC_LOG_PRINTF("wait sdio_rx_sema fail: ret=%d\n", ret);
        }

        if (msgcfm_poll_en == 0) { // process sdio rx in task
            u8 intstatus = 0;
            u32 data_len;
            int ret, idx, retry_cnt = 0;
            //__align(64) static u8 buffer_rx[2048] = {0,}; // TBD: dynamic allocation
            static uint32_t max_data_len = 0;
            struct aic_sdio_dev *sdiodev = &sdio_dev;
            rtos_mutex_lock(sdiodev->bus_txrx, -1);
            if (rwnx_hw->chipid == PRODUCT_ID_AIC8801) {
            ret = sdio_readb_cmd52(sdiodev->sdio_reg.block_cnt_reg, &intstatus);
                while ((ret == FALSE) || (intstatus & SDIO_OTHER_INTERRUPT)) {
                    AIC_LOG_PRINTF("ret=%d, intstatus=%x\n",ret, intstatus);
                    ret = sdio_readb_cmd52(sdiodev->sdio_reg.block_cnt_reg, &intstatus);
                    retry_cnt++;
                    if (retry_cnt >= 8) {
                        break;
                    }
                }
            } else if (rwnx_hw->chipid == PRODUCT_ID_AIC8800DC || rwnx_hw->chipid == PRODUCT_ID_AIC8800DW) {
                if (func_flag_rx)
                    ret = sdio_readb_cmd52_func2(sdiodev->sdio_reg.block_cnt_reg, &intstatus);
                else
                    ret = sdio_readb_cmd52(sdiodev->sdio_reg.block_cnt_reg, &intstatus);
                while ((ret == FALSE) || (intstatus & SDIO_OTHER_INTERRUPT)) {
                    AIC_LOG_PRINTF("ret=%d, intstatus=%x\n", ret, intstatus);
                    if (func_flag_rx)
                        ret = sdio_readb_cmd52_func2(sdiodev->sdio_reg.block_cnt_reg, &intstatus);
                    else
                        ret = sdio_readb_cmd52(sdiodev->sdio_reg.block_cnt_reg, &intstatus);
                    retry_cnt++;
                    if (retry_cnt >= 8) {
                        break;
                    }
                }
            } else if (rwnx_hw->chipid == PRODUCT_ID_AIC8800D80) {
                ret = sdio_readb_cmd52(sdiodev->sdio_reg.misc_int_status_reg, &intstatus);
                while (ret == FALSE) {
                    AIC_LOG_PRINTF("ret=%d, intstatus=%x\n",ret, intstatus);
                    ret = sdio_readb_cmd52(sdiodev->sdio_reg.misc_int_status_reg, &intstatus);
                    retry_cnt++;
                    if (retry_cnt >= 8) {
                        break;
                    }
                }

                if (intstatus & SDIO_OTHER_INTERRUPT) {
                    u8 int_pending;
                    ret = sdio_readb_cmd52(sdiodev->sdio_reg.sleep_reg, &int_pending);
                    if (ret == FALSE) {
                        AIC_LOG_PRINTF("reg:%d read failed!\n", sdiodev->sdio_reg.sleep_reg);
                    }
                    int_pending &= ~0x01; // dev to host soft irq
                    ret = sdio_writeb_cmd52(sdiodev->sdio_reg.sleep_reg, int_pending);
                    if (ret == FALSE) {
                        AIC_LOG_PRINTF("reg:%d write failed!\n", sdiodev->sdio_reg.sleep_reg);
                    }
                }
            }
            rtos_mutex_unlock(sdiodev->bus_txrx);

            //sdio_host_enable_isr(TRUE); // re-enable after cmd52
            //AIC_LOG_PRINTF("[task] intstatus=%d, retry_cnt=%d\n", intstatus, retry_cnt);
            if ((intstatus > 0) && (retry_cnt < 8)) {
                if (rwnx_hw->chipid == PRODUCT_ID_AIC8801 || rwnx_hw->chipid == PRODUCT_ID_AIC8800DC || rwnx_hw->chipid == PRODUCT_ID_AIC8800DW) {
                    if (intstatus < 64) {
                        data_len = intstatus * SDIOWIFI_FUNC_BLOCKSIZE;
                    } else {
                        u8 byte_len = 0;

                        rtos_mutex_lock(sdiodev->bus_txrx, -1);
                        if (rwnx_hw->chipid == PRODUCT_ID_AIC8801) {
                            ret = sdio_readb_cmd52(sdiodev->sdio_reg.bytemode_len_reg, &byte_len);
                        } else if (rwnx_hw->chipid == PRODUCT_ID_AIC8800DC || rwnx_hw->chipid == PRODUCT_ID_AIC8800DW) {
                            if (func_flag_rx)
                                ret = sdio_readb_cmd52_func2(sdiodev->sdio_reg.bytemode_len_reg, &byte_len);
                            else
                                ret = sdio_readb_cmd52(sdiodev->sdio_reg.bytemode_len_reg, &byte_len);
                        }
                        rtos_mutex_unlock(sdiodev->bus_txrx);
                        if (ret) {
                            AIC_LOG_PRINTF("byte mode len read err %d\r\n", ret);
                        }
                        AIC_LOG_PRINTF("byte mode len=%d\r\n", byte_len);
                        data_len = byte_len * 4; //byte_len must<= 128
                    }
                }else if (rwnx_hw->chipid == PRODUCT_ID_AIC8800D80) {
                    uint8_t intmaskf2 = intstatus | (0x1UL << 3);
                    if (intstatus == 120U) {    // byte mode
                        u8 byte_len = 0;

                        rtos_mutex_lock(sdiodev->bus_txrx, -1);
                        ret = sdio_readb_cmd52(sdiodev->sdio_reg.bytemode_len_reg, &byte_len);
                        rtos_mutex_unlock(sdiodev->bus_txrx);
                        if (ret) {
                            AIC_LOG_PRINTF("byte mode len read err %d\r\n", ret);
                        }
                        AIC_LOG_PRINTF("byte mode len=%d\r\n", byte_len);
                        data_len = byte_len * 4; //byte_len must<= 128
                    } else {
                        data_len = (intstatus & 0x7FU) * SDIOWIFI_FUNC_BLOCKSIZE;
                    }
                }
                if (data_len) {
                    if (max_data_len < data_len) {
                        max_data_len = data_len;
                        aic_dbg("max_data_len=%d\n", max_data_len);
                    }
                    #if (FHOST_RX_SW_VER == 3)
                    do {
                        uint8_t *buf_rx;
                        struct sdio_buf_node_s *node = sdio_buf_alloc(data_len);
                        if ((node == NULL) || (node->buf == NULL)) {
                            AIC_LOG_PRINTF("node/buf alloc fail(len=%d),node=%p,buf=%p!!!\n", data_len, node, node->buf);
                            if (node) {
                                sdio_buf_free(node);
                                node == NULL;
                            }
                            break;
                        }
                        buf_rx = node->buf;
                        ret = aicwf_sdio_recv_pkt(buf_rx, data_len, 1);
                        if (ret) {
                            AIC_LOG_PRINTF("sdio_rx_task, recv pkt err %d\n", ret);
                            sdio_buf_free(node);
                            break;
                        }
                        //aic_dbg("enq,%p,%d, node:%p\n",buf_rx, data_len, node);
                        fhost_rxframe_enqueue(node);
                        rtos_semaphore_signal(fhost_rx_env.rxq_trigg, false);
                    } while (0);
                    #endif
                }
            } else if (!polling) {
                AIC_LOG_PRINTF("Interrupt but no data, intstatus=%d, retry_cnt=%d\n", intstatus, retry_cnt);
            }
        } else {
            //sdio_host_enable_isr(TRUE); // re-enable after rx done
            AIC_LOG_PRINTF("msgcfm_poll_en is 1\n");
        }
        //aic_dbg("#*So\n");
        if ((aic_priority_mode == 3) && (sdio_datrx_priority >= tcpip_priority)) {
            rtos_task_set_priority(sdio_task_hdl, sdio_datrx_priority);
        }
    }
}

static int sdio_interrupt_init(void)
{
	int ret;
	sdio_claim_host(sdio_function[1]);

	ret = sdio_claim_irq(sdio_function[1], &aicwf_sdio_irq_hdl);
	if (ret)
	{
		AIC_LOG_PRINTF("%s: sdio_claim_irq FAIL(%d)!\n", __func__, ret);
	}
	sdio_release_host(sdio_function[1]);
	return ret;
}

#if (CONFIG_TXMSG_TEST_EN)
void aic_txmsg_test(void)
{
    __align(64) static u8 buffer[64] = {0,};
    __align(64) static u8 buffer_rx[512] = {0,};
    int err;
    int len = sizeof(struct lmac_msg) + sizeof(struct dbg_mem_read_req);
    struct dbg_mem_read_cfm rd_mem_addr_cfm;
    struct dbg_mem_read_cfm *cfm = &rd_mem_addr_cfm;
    struct lmac_msg *msg;
    struct dbg_mem_read_req *req;
    int index = 0;
    buffer[0] = (len+4) & 0x00ff;
    buffer[1] = ((len+4) >> 8) &0x0f;
    buffer[2] = 0x11;
    if (rwnx_hw->chipid == PRODUCT_ID_AIC8801 || rwnx_hw->chipid == PRODUCT_ID_AIC8800DC ||
        rwnx_hw->chipid == PRODUCT_ID_AIC8800DW)
        buffer[3] = 0x0;
    else if (rwnx_hw->chipid == PRODUCT_ID_AIC8800D80)
        buffer[3] = crc8_ponl_107(&buffer[0], 3); // crc8
    index += 4;
    //there is a dummy word
    index += 4;
    msg = (struct lmac_msg *)&buffer[index];
    msg->id = DBG_MEM_READ_REQ;
    msg->dest_id = TASK_DBG;
    msg->src_id = 100;
    msg->param_len = sizeof(struct dbg_mem_read_req);
    req = (struct dbg_mem_read_req *)&msg->param[0];
    req->memaddr = 0x40500000;
    err = aicwf_sdio_tx_msg(buffer, len + 8, buffer_rx);
    if (!err) {
        AIC_LOG_PRINTF("tx msg done\n");
    }
}
#endif

int aic_gpio_ind_init(struct rwnx_hw *rwnx_hw)
{
    int err = 0;
    #if 0
    __align(64) static u8 buffer[64] = {0,};
    __align(64) static u8 buffer_rx[512] = {0,};
    int len = sizeof(struct lmac_msg) + sizeof(struct dbg_mem_write_req);
    struct dbg_mem_write_cfm wr_mem_cfm;
    struct dbg_mem_write_cfm *cfm = &wr_mem_cfm;
    struct lmac_msg *msg;
    struct dbg_mem_write_req *req;
    int index = 0;
    buffer[0] = (len+4) & 0x00ff;
    buffer[1] = ((len+4) >> 8) &0x0f;
    buffer[2] = 0x11;
    if (rwnx_hw->chipid == PRODUCT_ID_AIC8801 || rwnx_hw->chipid == PRODUCT_ID_AIC8800DC ||
        rwnx_hw->chipid == PRODUCT_ID_AIC8800DW)
        buffer[3] = 0x0;
    else if (rwnx_hw->chipid == PRODUCT_ID_AIC8800D80)
        buffer[3] = crc8_ponl_107(&buffer[0], 3); // crc8
    index += 4;
    //there is a dummy word
    index += 4;
    msg = (struct lmac_msg *)&buffer[index];
    msg->id = DBG_MEM_WRITE_REQ;
    msg->dest_id = TASK_DBG;
    msg->src_id = 100;
    msg->param_len = sizeof(struct dbg_mem_write_req);
    req = (struct dbg_mem_write_req *)&msg->param[0];
    #if 0
    do {
        req->memaddr = 0x40500028;
        req->memdata = 0x00000000;
        err = aicwf_sdio_tx_msg(buffer, len + 8, buffer_rx);
        if (err) {
            AIC_LOG_PRINTF("tx msg [0] fail\n");
            break;
        }
        req->memaddr = 0x4050301C;
        req->memdata = 0x00000007;
        err = aicwf_sdio_tx_msg(buffer, len + 8, buffer_rx);
        if (err) {
            AIC_LOG_PRINTF("tx msg [1] fail\n");
            break;
        }
        req->memaddr = 0x40100054;
        req->memdata = 0x00000001;
        err = aicwf_sdio_tx_msg(buffer, len + 8, buffer_rx);
        if (err) {
            AIC_LOG_PRINTF("tx msg [2] fail\n");
            break;
        }
        req->memaddr = 0x4024107C;
        req->memdata = 0x00000001;
        err = aicwf_sdio_tx_msg(buffer, len + 8, buffer_rx);
        if (err) {
            AIC_LOG_PRINTF("tx msg [3] fail\n");
            break;
        }
        req->memaddr = 0x402400F0;
        req->memdata = 0x00340022;
        err = aicwf_sdio_tx_msg(buffer, len + 8, buffer_rx);
        if (err) {
            AIC_LOG_PRINTF("tx msg [4] fail\n");
            break;
        }
    } while (0);
    #else
    int idx, cnt = 0;
    uint32_t (*p_tbl)[2] = NULL;
    // for 8800d
    const uint32_t gpio_cfg_tbl_8800d[][2] = {
        {0x40500028, 0x00000000},
        {0x4050301C, 0x00000007},
        {0x40100054, 0x00000001},
        {0x4024107C, 0x00000001},
        {0x402400F0, 0x00340022},
    };
#ifndef CONFIG_GPIOINT_WPAEUPPIN
    // for 8800dc/dw    map data1 isr to gpioa7
    const uint32_t gpio_cfg_tbl_8800dcdw[][2] = {
        {0x40500040, 0x00000000},
        {0x4050401C, 0x00000007},
        {0x40100030, 0x00000001},
        {0x40241020, 0x00000001},
        {0x402400F0, 0x00340022},
    };
#else
    // for 8800dc/dw    map data1 isr to gpiob1
    const uint32_t gpio_cfg_tbl_8800dcdw[][2] = {
        {0x40504044, 0x00000002},
        {0x40500060, 0x03020700},
        {0x40500040, 0x00000000},
        {0x40100030, 0x00000001},
        {0x40241020, 0x00000001},
        {0x402400F0, 0x00340022},
    };
#endif

#ifndef CONFIG_GPIOINT_WPAEUPPIN
    // for 8800d40/d80     map data1 isr to gpioa7
    const uint32_t gpio_cfg_tbl_8800d40d80[][2] = {
        {0x4050401C, 0x00000007},
        {0x40500040, 0x00000000},
        {0x40100030, 0x00000001},
        {0x40241020, 0x00000001},
        {0x40240030, 0x00000004},
    };
#else
    // for 8800d40/d80     map data1 isr to gpiob1
    const uint32_t gpio_cfg_tbl_8800d40d80[][2] = {
        {0x40504084, 0x00000006},
        {0x40500040, 0x00000000},
        {0x40100030, 0x00000001},
        {0x40241020, 0x00000001},
        {0x40240030, 0x00000004},
        {0x40240020, 0x03020700},
    };
#endif
    if (rwnx_hw->chipid == PRODUCT_ID_AIC8801) {
        p_tbl = gpio_cfg_tbl_8800d;
        cnt = sizeof(gpio_cfg_tbl_8800d) / sizeof(uint32_t) / 2;
    } else if ((rwnx_hw->chipid == PRODUCT_ID_AIC8800DC) || (rwnx_hw->chipid == PRODUCT_ID_AIC8800DW)) {
        p_tbl = gpio_cfg_tbl_8800dcdw;
        cnt = sizeof(gpio_cfg_tbl_8800dcdw) / sizeof(uint32_t) / 2;
    } else if (rwnx_hw->chipid == PRODUCT_ID_AIC8800D80) {
        p_tbl = gpio_cfg_tbl_8800d40d80;
        cnt = sizeof(gpio_cfg_tbl_8800d40d80) / sizeof(uint32_t) / 2;
    } else {
        AIC_LOG_PRINTF("unsupported chipid\n");
        return -1;
    }
    for (idx = 0; idx < cnt; idx++) {
        req->memaddr = p_tbl[idx][0];
        req->memdata = p_tbl[idx][1];
        err = aicwf_sdio_tx_msg(buffer, len + 8, buffer_rx);
        if (err) {
            AIC_LOG_PRINTF("tx msg [%d] fail\n", idx);
            break;
        }
    }
    #endif
    #endif
    msgcfm_poll_en = 0;
    return err;
}

void aicwf_sdio_reg_init(struct rwnx_hw *rwnx_hw, struct aic_sdio_dev *sdiodev)
{
    AIC_LOG_PRINTF("%s\n", __func__);

    if (rwnx_hw->chipid == PRODUCT_ID_AIC8801 || rwnx_hw->chipid == PRODUCT_ID_AIC8800DC || 
       rwnx_hw->chipid == PRODUCT_ID_AIC8800DW){
        sdiodev->sdio_reg.bytemode_len_reg =       SDIOWIFI_BYTEMODE_LEN_REG;
        sdiodev->sdio_reg.intr_config_reg =        SDIOWIFI_INTR_CONFIG_REG;
        sdiodev->sdio_reg.sleep_reg =              SDIOWIFI_SLEEP_REG;
        sdiodev->sdio_reg.wakeup_reg =             SDIOWIFI_WAKEUP_REG;
        sdiodev->sdio_reg.flow_ctrl_reg =          SDIOWIFI_FLOW_CTRL_REG;
        sdiodev->sdio_reg.register_block =         SDIOWIFI_REGISTER_BLOCK;
        sdiodev->sdio_reg.bytemode_enable_reg =    SDIOWIFI_BYTEMODE_ENABLE_REG;
        sdiodev->sdio_reg.block_cnt_reg =          SDIOWIFI_BLOCK_CNT_REG;
        sdiodev->sdio_reg.rd_fifo_addr =           SDIOWIFI_RD_FIFO_ADDR;
        sdiodev->sdio_reg.wr_fifo_addr =           SDIOWIFI_WR_FIFO_ADDR;
	} else if (rwnx_hw->chipid == PRODUCT_ID_AIC8800D80){
        sdiodev->sdio_reg.bytemode_len_reg =       SDIOWIFI_BYTEMODE_LEN_REG_V3;
        sdiodev->sdio_reg.intr_config_reg =        SDIOWIFI_INTR_ENABLE_REG_V3;
        sdiodev->sdio_reg.sleep_reg =              SDIOWIFI_INTR_PENDING_REG_V3;
        sdiodev->sdio_reg.wakeup_reg =             SDIOWIFI_INTR_TO_DEVICE_REG_V3;
        sdiodev->sdio_reg.flow_ctrl_reg =          SDIOWIFI_FLOW_CTRL_Q1_REG_V3;
        sdiodev->sdio_reg.bytemode_enable_reg =    SDIOWIFI_BYTEMODE_ENABLE_REG_V3;
        sdiodev->sdio_reg.misc_int_status_reg =    SDIOWIFI_MISC_INT_STATUS_REG_V3;
        sdiodev->sdio_reg.rd_fifo_addr =           SDIOWIFI_RD_FIFO_ADDR_V3;
        sdiodev->sdio_reg.wr_fifo_addr =           SDIOWIFI_WR_FIFO_ADDR_V3;
    }
}

int aicwf_sdio_func_init(struct rwnx_hw *rwnx_hw, struct aic_sdio_dev *sdiodev)
{
    uint32_t ret = 0;
    uint8_t block_bit0 = 0x1;
    uint8_t byte_mode_disable = 0x1;//1: no byte mode

    /* Enable Function 1 */
    sdio_claim_host(sdio_function[1]);
    AIC_LOG_PRINTF("sdio_host_init:func1:aic_sdio_set_block_size %d\n", SDIOWIFI_FUNC_BLOCKSIZE);
    aic_sdio_set_block_size(SDIOWIFI_FUNC_BLOCKSIZE);
    AIC_LOG_PRINTF("sdio_host_init:func1:sdio_set_block_size %d\n", SDIOWIFI_FUNC_BLOCKSIZE);
    sdio_set_block_size(sdio_function[1], SDIOWIFI_FUNC_BLOCKSIZE);
    if (sdio_block_size != SDIOWIFI_FUNC_BLOCKSIZE) {
        AIC_LOG_PRINTF("sdio_host_init:func1: blksize set failed\n");
    }
    AIC_LOG_PRINTF("sdio_host_init:sdio_func_enable func1\n");
    ret = sdio_enable_func(sdio_function[1]);
    if (ret < 0) {
        AIC_LOG_PRINTF("sdio_host_init:enable func1 err! ret is %d", ret);
        return ret;
    }
    AIC_LOG_PRINTF("sdio_host_init:enable func1 ok!\n");
    sdio_release_host(sdio_function[1]);

    ret = sdio_writeb_cmd52(sdiodev->sdio_reg.register_block, block_bit0);
    if (ret < 0) {
        AIC_LOG_PRINTF("reg:%d write failed!\n", sdiodev->sdio_reg.register_block);
        return ret;
    }

    //1: no byte mode
    ret = sdio_writeb_cmd52(sdiodev->sdio_reg.bytemode_enable_reg, byte_mode_disable);
    if (ret < 0) {
        AIC_LOG_PRINTF("reg:%d write failed!\n", sdiodev->sdio_reg.bytemode_enable_reg);
        return ret;
    }

    //enable sdio interrupt
    ret = sdio_writeb_cmd52(sdiodev->sdio_reg.intr_config_reg, 0x7);
    if (ret < 0) {
        AIC_LOG_PRINTF("reg:%d write failed!\n", sdiodev->sdio_reg.intr_config_reg);
        return ret;
    }

    if (rwnx_hw->chipid == PRODUCT_ID_AIC8800DC || rwnx_hw->chipid == PRODUCT_ID_AIC8800DW) {
        /* Enable Function 2 */
        sdio_claim_host(sdio_function[2]);
        AIC_LOG_PRINTF("sdio_host_init:func2:aic_sdio_set_block_size %d\n", SDIOWIFI_FUNC_BLOCKSIZE);
        aic_sdio_set_block_size_func2(SDIOWIFI_FUNC_BLOCKSIZE);
        AIC_LOG_PRINTF("sdio_host_init:func2:sdio_set_block_size %d\n", SDIOWIFI_FUNC_BLOCKSIZE);
        sdio_set_block_size(sdio_function[2], SDIOWIFI_FUNC_BLOCKSIZE);
        if (sdio_block_size != SDIOWIFI_FUNC_BLOCKSIZE) {
            AIC_LOG_PRINTF("sdio_host_init:func2: blksize set failed\n");
        }
        AIC_LOG_PRINTF("sdio_host_init:sdio_func_enable func2\n");
        ret = sdio_enable_func(sdio_function[2]);
        if (ret < 0) {
            AIC_LOG_PRINTF("sdio_host_init:enable func2 err! ret is %d", ret);
            return ret;
        }
        AIC_LOG_PRINTF("sdio_host_init:enable func2 ok!\n");
        sdio_release_host(sdio_function[2]);

        ret = sdio_writeb_cmd52_func2(sdiodev->sdio_reg.register_block, block_bit0);
        if (ret < 0) {
            AIC_LOG_PRINTF("reg:%d write failed!\n", sdiodev->sdio_reg.register_block);
            return ret;
        }

        //1: no byte mode
        ret = sdio_writeb_cmd52_func2(sdiodev->sdio_reg.bytemode_enable_reg, byte_mode_disable);
        if (ret < 0) {
            AIC_LOG_PRINTF("reg:%d write failed!\n", sdiodev->sdio_reg.bytemode_enable_reg);
            return ret;
        }

        //enable sdio interrupt
        ret = sdio_writeb_cmd52_func2(sdiodev->sdio_reg.intr_config_reg, 0x7);
        if (ret < 0) {
            AIC_LOG_PRINTF("reg:%d write failed!\n", sdiodev->sdio_reg.intr_config_reg);
            return ret;
        }
    }
}

int aicwf_sdiov3_func_init(struct rwnx_hw *rwnx_hw, struct aic_sdio_dev *sdiodev)
{
    uint32_t ret = 0;
    uint8_t byte_mode_disable = 0x1;//1: no byte mode
    u8 val = 0x01;

    /* Enable Function 1 */
    sdio_claim_host(sdio_function[FUNC_1]);
    AIC_LOG_PRINTF("sdio_host_init:func1:aic_sdio_set_block_size %d\n", SDIOWIFI_FUNC_BLOCKSIZE);
    aic_sdio_set_block_size(SDIOWIFI_FUNC_BLOCKSIZE);
    AIC_LOG_PRINTF("sdio_host_init:func1:sdio_set_block_size %d\n", SDIOWIFI_FUNC_BLOCKSIZE);
    sdio_set_block_size(sdio_function[FUNC_1], SDIOWIFI_FUNC_BLOCKSIZE);
    if (sdio_block_size != SDIOWIFI_FUNC_BLOCKSIZE) {
        AIC_LOG_PRINTF("sdio_host_init:func1: blksize set failed\n");
    }
    AIC_LOG_PRINTF("sdio_host_init:sdio_func_enable func1\n");
    ret = sdio_enable_func(sdio_function[FUNC_1]);
    if (ret < 0) {
        AIC_LOG_PRINTF("sdio_host_init:enable func1 err! ret is %d", ret);
        return ret;
    }
    AIC_LOG_PRINTF("sdio_host_init:enable func1 ok!\n");
    sdio_release_host(sdio_function[FUNC_1]);

    sdio_claim_host(sdio_function[FUNC_1]);
    sdio_f0_writeb(sdio_function[FUNC_1], 0x7F, 0xF2, &ret);
	if (ret < 0){
		AIC_LOG_PRINTF("set func0 0xF2 fail %d\n", ret);
		sdio_release_host(sdio_function[FUNC_1]);
		return FALSE;
	}

#if 1
    val |= SDIOCLK_FREE_RUNNING_BIT;
    sdio_f0_writeb(sdio_function[FUNC_1], val, 0xF0, &ret);
	if(ret < 0){
		AIC_LOG_PRINTF("set iopad ctrl fail %d\n", ret);
		sdio_release_host(sdio_function[FUNC_1]);
		return FALSE;
	}

    sdio_f0_writeb(sdio_function[FUNC_1], 0xA0, 0xF8, &ret);
    AIC_LOG_PRINTF("!!!!!!!F8 0x%x\n", 0x80);
	if(ret < 0){
		AIC_LOG_PRINTF("set iopad delay2 fail %d\n", ret);
		sdio_release_host(sdio_function[FUNC_1]);
		return FALSE;
	}

    sdio_f0_writeb(sdio_function[FUNC_1], 0x00, 0xF1, &ret);
    AIC_LOG_PRINTF("!!!!!!!F1 0x%x\n", 0x00);
	if(ret < 0){
		AIC_LOG_PRINTF("set iopad delay1 fail %d\n", ret);
		sdio_release_host(sdio_function[FUNC_1]);
		return FALSE;
	}

    udelay(100);
#endif
    sdio_release_host(sdio_function[FUNC_1]);

    //1: no byte mode
    ret = sdio_writeb_cmd52(sdiodev->sdio_reg.bytemode_enable_reg, byte_mode_disable);
    if (ret < 0) {
        AIC_LOG_PRINTF("reg:%d write failed!\n", sdiodev->sdio_reg.bytemode_enable_reg);
        return ret;
    }

    sdio_claim_host(sdio_function[FUNC_1]);
    sdio_f0_writeb(sdio_function[FUNC_1], 0x07, 0x04, &ret);
	if (ret < 0){
		AIC_LOG_PRINTF("set func0 int en fail %d\n", ret);
		sdio_release_host(sdio_function[FUNC_1]);
		return FALSE;
	}
    sdio_release_host(sdio_function[FUNC_1]);

    //enable sdio interrupt
    ret = sdio_writeb_cmd52(sdiodev->sdio_reg.intr_config_reg, 0x7);
    if (ret < 0) {
        AIC_LOG_PRINTF("reg:%d write failed!\n", sdiodev->sdio_reg.intr_config_reg);
        return ret;
    }
}

bool sdio_host_init(struct rwnx_hw *rwnx_hw, void (*sdio_isr)(void))
{
    uint32_t ret = 0;
    uint8_t in;
    int err_ret = 0;
    int i =0;
    uint8_t block_bit0 = 0x1;
    uint8_t byte_mode_disable = 0x1;//1: no byte mode
    aicwf_sdio_tx_init();
    sdio_function[FUNC_1] = wifi_sdio_func;
    struct aic_sdio_dev *sdiodev = &sdio_dev;

    aicwf_sdio_reg_init(rwnx_hw, sdiodev);

    if (rwnx_hw->chipid != PRODUCT_ID_AIC8800D80) {
        ret = aicwf_sdio_func_init(rwnx_hw, sdiodev);
    } else {
        ret = aicwf_sdiov3_func_init(rwnx_hw, sdiodev);
    }
    if (ret < 0) {
        AIC_LOG_PRINTF("sdio func init fail\n");
        return FALSE;
    }
    AIC_LOG_PRINTF("sdio func init success\n");

    if (sdio_interrupt_init()) {
        AIC_LOG_PRINTF("sdio_host_init: sdio_interrupt_init failed\n");
        return FALSE;
    }

    /* disable sdio control interupt at first */
    //sdio_host_enable_isr(FALSE);

    /* install isr here */
    g_sdio_isr_func = sdio_isr;

    /* prepare for sdio isr */
    if (aic_gpio_ind_init(rwnx_hw)) {
        AIC_LOG_PRINTF("gpio ind init fail\n");
        return FALSE;
    }

    #if (FHOST_RX_SW_VER == 3)
    sdio_buf_init();
    #endif

    rtos_semaphore_create(&sdio_rx_sema, "sdio_rx_sema", 8, 0);
    if (sdio_rx_sema == NULL) {
        AIC_LOG_PRINTF("sdio rx sema create fail\n");
        return FALSE;
    }

    ret = rtos_task_create(sdio_rx_task, "sdio_rx_task", SDIO_DATRX_TASK,
                           sdio_datrx_stack_size, (void*)rwnx_hw, sdio_datrx_priority,
                           &sdio_task_hdl);
    if (ret || (sdio_task_hdl == NULL)) {
        AIC_LOG_PRINTF("sdio task create fail\n");
        return FALSE;
    }

    /* enable sdio control interupt */
    //sdio_host_enable_isr(TRUE);

    AIC_LOG_PRINTF("sdio_host_init:host_int ok\n");

    #if (CONFIG_TXMSG_TEST_EN)
    aic_txmsg_test();
	#endif

    return TRUE;
}

bool sdio_host_deinit(struct rwnx_hw *rwnx_hw)
{
    int ret = 0;

    AIC_LOG_PRINTF("sdio_host_deinit\n");

    rtos_semaphore_delete(sdio_rx_sema);
    sdio_rx_sema = NULL;

    aicwf_sdio_tx_deinit();

    sdio_host_enable_isr(0);

    if (rwnx_hw->chipid == PRODUCT_ID_AIC8800DC || rwnx_hw->chipid == PRODUCT_ID_AIC8800DW) {
        func_flag_tx = true;
        func_flag_rx = true;
    }

    return TRUE;
}
