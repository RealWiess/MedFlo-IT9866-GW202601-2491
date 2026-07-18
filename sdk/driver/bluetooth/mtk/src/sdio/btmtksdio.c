/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "ite/ite_sdio.h"
#include "ite/itp.h"
#include "btmtk_sdio.h"
#include "btmtk_main.h"

#include "string.h"

static u8 event_need_compare[EVENT_COMPARE_SIZE] = {0};
static u8 event_need_compare_len;
static u8 event_compare_status;

static int sdio_interrupt_status;

static int btmtk_sdio_readl(u32 offset,  u32 *val, struct sdio_func *func);
static int btmtk_sdio_readl_without_claim(u32 offset,  u32 *val, struct sdio_func *func);
static int btmtk_sdio_writel(u32 offset, u32 val, struct sdio_func *func);
static int btmtk_sdio_writel_without_claim(u32 offset, u32 val, struct sdio_func *func);
static int btmtk_sdio_writesb(u32 offset, u8 *val, int len, struct sdio_func *func);
static int btmtk_sdio_readsb(u32 offset, u8 *val, int len, struct sdio_func *func);
static int btmtk_sdio_writeb(u32 offset, u8 val, struct sdio_func *func);
static int btmtk_sdio_readb(u32 offset, u8 *val, struct sdio_func *func);
void btmtk_sdio_set_no_fwn_own(struct btmtk_sdio_dev *cif_dev, int flag);

static struct btmtk_sdio_dev g_sdio_dev;

SemaphoreHandle_t SDIO_OWN_MUTEX;

int btmtk_init_sdio_own_mutex()
{
	int state = 0;
	BTMTK_INFO("%s", __func__);
	SDIO_OWN_MUTEX = xSemaphoreCreateMutex();
	if (SDIO_OWN_MUTEX == NULL){
		BTMTK_INFO("%s SDIO_OWN_MUTEX init fail", __func__);
		return -1;
	}
	return state;
}


static int btmtk_sdio_writel_without_claim(u32 offset, u32 val, struct sdio_func *func)
{
	int ret = 0;
	u32 retry_count = 0;
	
	//BTMTK_INFO("%s, offset = 0x%08x, val = 0x%08x", __func__, offset, val);

	if (func == NULL) {
		BTMTK_ERR("%s func is NULL", __func__);
		return -EIO;
	}

	do {
		sdio_writel(func, val, offset, &ret);
		if (ret) {
			BTMTK_ERR(" %s, ret:%d", __func__, ret);
			if (ret == -ETIMEDOUT) {
				return ret;
			}
			else {
				BTMTK_INFO("%s, vTaskDelay", __func__);
				vTaskDelay(SDIO_IO_DELAY / portTICK_RATE_MS);
			}
		}
		retry_count++;
		if (retry_count > SDIO_RW_RETRY_COUNT) {
			BTMTK_ERR(" %s, ret:%d", __func__, ret);
			break;
		}
	} while (ret);

	//BTMTK_INFO("%s end", __func__);

	return ret;
}


static int btmtk_sdio_writel(u32 offset, u32 val, struct sdio_func *func)
{
	int ret = 0;
	u32 retry_count = 0;
	
	//BTMTK_INFO("%s, offset = 0x%08x, val = 0x%08x", __func__, offset, val);

	if (func == NULL) {
		BTMTK_ERR("%s func is NULL", __func__);
		return -EIO;
	}

	do {
		//BTMTK_INFO("%s, sdio_claim_host", __func__);
		sdio_claim_host(func);
		sdio_writel(func, val, offset, &ret);
		//BTMTK_INFO("%s, sdio_release_host", __func__);
		sdio_release_host(func);
		if (ret) {
			BTMTK_ERR(" %s, ret:%d", __func__, ret);
			if (ret == -ETIMEDOUT) {
				return ret;
			}
			else {
				BTMTK_INFO("%s, vTaskDelay", __func__);
				vTaskDelay(SDIO_IO_DELAY / portTICK_RATE_MS);
			}
		}
		retry_count++;
		if (retry_count > SDIO_RW_RETRY_COUNT) {
			BTMTK_ERR(" %s, ret:%d", __func__, ret);
			break;
		}
	} while (ret);

	//BTMTK_INFO("%s end", __func__);

	return ret;
}

static int btmtk_sdio_readl_without_claim(u32 offset,  u32 *val, struct sdio_func *func)
{
	int ret = 0;
	u32 retry_count = 0;
	
	//BTMTK_INFO("%s, offset = 0x%08x", __func__, offset);

	if (func == NULL) {
		BTMTK_ERR("func is NULL");
		return -EIO;
	}

	do {
		//BTMTK_INFO("%s, sdio_claim_host", __func__);
		//sdio_claim_host(func);
		*val = sdio_readl(func, offset, &ret);
		//BTMTK_INFO("%s, sdio_release_host", __func__);
		//sdio_release_host(func);
		if (ret) {
			BTMTK_ERR(" %s, ret:%d", __func__, ret);
			if (ret == -ETIMEDOUT) {
				return ret;
			}
			else {
				BTMTK_INFO("%s, vTaskDelay", __func__);
				vTaskDelay(SDIO_IO_DELAY / portTICK_RATE_MS);
			}
		}
		retry_count++;
		if (retry_count > SDIO_RW_RETRY_COUNT) {
			BTMTK_ERR(" %s, ret:%d", __func__, ret);
			break;
		}
	} while (ret);
	
	//BTMTK_INFO("%s end", __func__);

	return ret;
}

static int btmtk_sdio_readl(u32 offset,  u32 *val, struct sdio_func *func)
{
	int ret = 0;
	u32 retry_count = 0;
	
	//BTMTK_INFO("%s, offset = 0x%08x", __func__, offset);

	if (func == NULL) {
		BTMTK_ERR("func is NULL");
		return -EIO;
	}

	do {
		//BTMTK_INFO("%s, sdio_claim_host", __func__);
		sdio_claim_host(func);
		*val = sdio_readl(func, offset, &ret);
		//BTMTK_INFO("%s, sdio_release_host", __func__);
		sdio_release_host(func);
		if (ret) {
			BTMTK_ERR(" %s, ret:%d", __func__, ret);
			if (ret == -ETIMEDOUT) {
				return ret;
			}
			else {
				BTMTK_INFO("%s, vTaskDelay", __func__);
				vTaskDelay(SDIO_IO_DELAY / portTICK_RATE_MS);
			}
		}
		retry_count++;
		if (retry_count > SDIO_RW_RETRY_COUNT) {
			BTMTK_ERR(" %s, ret:%d", __func__, ret);
			break;
		}
	} while (ret);
	
	//BTMTK_INFO("%s end", __func__);

	return ret;
}

static int btmtk_sdio_writesb(u32 offset, u8 *val, int len, struct sdio_func *func)
{
	int ret = 0;
	u32 retry_count = 0;
	
	//BTMTK_INFO("%s", __func__);

	if (func == NULL) {
		BTMTK_ERR("%s func is NULL", __func__);
		return -EIO;
	}

	do {
		sdio_claim_host(func);
		ret = sdio_writesb(func, offset, val, len);
		sdio_release_host(func);
		if (ret) {
			BTMTK_ERR(" %s, ret:%d", __func__, ret);
			if (ret == -ETIMEDOUT) {
				return ret;
			}
			else {
				vTaskDelay(SDIO_IO_DELAY / portTICK_RATE_MS);
			}
		}
		retry_count++;
		if (retry_count > SDIO_RW_RETRY_COUNT) {
			BTMTK_ERR("%s, ret:%d", __func__, ret);
			break;
		}
	} while (ret);


    //BTMTK_INFO("%s end", __func__);
	return ret;
}

static int btmtk_sdio_readsb_without_claim(u32 offset, u8 *val, int len, struct sdio_func *func)
{
	int ret = 0;
	u32 retry_count = 0;

	//BTMTK_INFO("%s", __func__);

	if (func == NULL) {
		BTMTK_ERR("%s func is NULL", __func__);
		return -EIO;
	}

	do {
		//sdio_claim_host(func);
		ret = sdio_readsb(func, val, offset, len);
		//sdio_release_host(func);
		if (ret) {
			BTMTK_ERR(" %s, ret:%d", __func__, ret);
			if (ret == -ETIMEDOUT)
				return ret;
			else
				vTaskDelay(SDIO_IO_DELAY / portTICK_RATE_MS);
		}
		retry_count++;
		if (retry_count > SDIO_RW_RETRY_COUNT) {
			BTMTK_ERR(" %s, ret:%d", __func__, ret);
			break;
		}
	} while (ret);

    //BTMTK_INFO("%s end", __func__);
	return ret;
}

static int btmtk_sdio_readsb(u32 offset, u8 *val, int len, struct sdio_func *func)
{
	int ret = 0;
	u32 retry_count = 0;

	//BTMTK_INFO("%s", __func__);

	if (func == NULL) {
		BTMTK_ERR("%s func is NULL", __func__);
		return -EIO;
	}

	do {
		sdio_claim_host(func);
		ret = sdio_readsb(func, val, offset, len);
		sdio_release_host(func);
		if (ret) {
			BTMTK_ERR(" %s, ret:%d", __func__, ret);
			if (ret == -ETIMEDOUT)
				return ret;
			else
				vTaskDelay(SDIO_IO_DELAY / portTICK_RATE_MS);
		}
		retry_count++;
		if (retry_count > SDIO_RW_RETRY_COUNT) {
			BTMTK_ERR(" %s, ret:%d", __func__, ret);
			break;
		}
	} while (ret);

    //BTMTK_INFO("%s end", __func__);
	return ret;
}

static int btmtk_sdio_writeb(u32 offset, u8 val, struct sdio_func *func)
{
	int ret = 0;
	u32 retry_count = 0;

	//BTMTK_INFO("%s, offset = 0x%08x, val = 0x%08x", __func__, offset, val);

	if (func == NULL) {
		BTMTK_ERR("%s func is NULL", __func__);
		return -EIO;
	}

	do {
		sdio_claim_host(func);
		sdio_writeb(func, val, offset, &ret);
		sdio_release_host(func);
		if (ret) {
			BTMTK_ERR(" %s, ret:%d", __func__, ret);
			if (ret == -ETIMEDOUT)
				return ret;
			else
				vTaskDelay(SDIO_IO_DELAY / portTICK_RATE_MS);
		}
		retry_count++;
		if (retry_count > SDIO_RW_RETRY_COUNT) {
			BTMTK_ERR(" %s, ret:%d", __func__, ret);
			break;
		}
	} while (ret);

    //BTMTK_INFO("%s end", __func__);
	return ret;
}

static int btmtk_sdio_readb(u32 offset, u8 *val, struct sdio_func *func)
{
	int ret = 0;
	u32 retry_count = 0;

	//BTMTK_INFO("%s, offset = 0x%08x", __func__, offset);

	if (func == NULL) {
		BTMTK_ERR("%s func is NULL", __func__);
		return -EIO;
	}

	do {
		sdio_claim_host(func);
		*val = sdio_readb(func, offset, &ret);
		sdio_release_host(func);
		if (ret) {
			BTMTK_ERR(" %s, ret:%d", __func__, ret);
			if (ret == -ETIMEDOUT)
				return ret;
			else
				vTaskDelay(SDIO_IO_DELAY / portTICK_RATE_MS);
		}
		retry_count++;
		if (retry_count > SDIO_RW_RETRY_COUNT) {
			BTMTK_ERR(" %s, ret:%d", __func__, ret);
			break;
		}
	} while (ret);

    //BTMTK_INFO("%s end", __func__);
	return ret;
}

static int btmtk_sdio_enable_interrupt(int enable, struct sdio_func *func)
{
	int ret = 0;
	u32 cr_value = 0;

	BTMTK_INFO("%s, enable=%d", __func__, enable);
	sdio_interrupt_status = enable;
	if (enable) {
		cr_value |= C_FW_INT_EN_SET;
	}
	else {
		cr_value |= C_FW_INT_EN_CLEAR;
	}

	ret = btmtk_sdio_writel(CHLPCR, cr_value, func);
	BTMTK_INFO("%s Done", __func__);

	return ret;
}

/* Get BT whole packet length except hci type */
unsigned int get_pkt_len(unsigned char type, unsigned char *buf)
{
	unsigned int len = 0;

	switch (type) {
	/* Please reference hci header format
	 * AA = len
	 * xx = buf[0]
	 * cmd : 01 xx yy AA + payload
	 * acl : 02 xx yy AA AA + payload
	 * sco : 03 xx yy AA + payload
	 * evt : 04 xx AA + payload
	 * ISO : 05 xx yy AA AA + payload
	 */
	case HCI_COMMAND_PKT:
		len = buf[2] + 3;
		break;
	case HCI_ACLDATA_PKT:
		len = buf[2] + ((buf[3] << 8) & 0xff00) + 4;
		break;
	case HCI_SCODATA_PKT:
		len = buf[2] + 3;
		break;
	case HCI_EVENT_PKT:
		len = buf[1] + 2;
		break;
	case HCI_ISODATA_PKT:
		len = buf[2] + (((buf[3] & 0x3F) << 8) & 0xff00) + HCI_ISO_PKT_HEADER_SIZE;
		break;
	default:
		len = 0;
	}

	return len;
}

static int btmtk_cif_recv_evt_ext(struct btmtk_dev *bdev)
{
	int ret = 0;
	u32 u32ReadCRValue = 0;
	u32 u32ReadCRLEN = 0;
	u32 sdio_header_length = 0;
	int rx_length = 0;
	int payload = 0;
	u16 hci_pkt_len = 0;
	u8 hci_type = 0;
	struct btmtk_sdio_dev *cif_dev = (struct btmtk_sdio_dev *)bdev->cif_dev;

    //BTMTK_INFO("%s", __func__);

	memset(bdev->io_buf, 0, IO_BUF_SIZE);

	/* keep polling method */
	/* If interrupt method is working, we can remove it */
	ret = btmtk_sdio_readl_without_claim(CHISR, &u32ReadCRValue, cif_dev->func);
	BTMTK_DBG("%s: CHISR: 0x%08X", __func__, u32ReadCRValue);

	ret = btmtk_sdio_readl_without_claim(CRPLR, &u32ReadCRLEN, cif_dev->func);
	BTMTK_DBG("%s: CRPLR: 0x%08X", __func__, u32ReadCRLEN);

	rx_length = (u32ReadCRLEN & RX_PKT_LEN) >> 16;
	if (rx_length == 0xFFFF || rx_length == 0) {
		BTMTK_WARN("%s: rx_length = %d, error return -EIO", __func__, rx_length);
		return -EIO;
	}

	//BTMTK_DBG("%s: u32ReadCRValue = %08X", __func__, u32ReadCRValue);
	u32ReadCRValue &= 0xFFFB; //clear bit4 ,TX_COMPLETE_COUNT
	ret = btmtk_sdio_writel_without_claim(CHISR, u32ReadCRValue, cif_dev->func);
	// BTMTK_DBG("%s: write = %08X", __func__, u32ReadCRValue);

	//ret = btmtk_sdio_readl(PD2HRM0R, &u32ReadCRValue, cif_dev->func);

    memset(cif_dev->transfer_buf, 0, URB_MAX_BUFFER_SIZE);
	ret = btmtk_sdio_readsb_without_claim(CRDR, cif_dev->transfer_buf, rx_length, cif_dev->func);


	sdio_header_length = (cif_dev->transfer_buf[1] << 8);
	sdio_header_length |= cif_dev->transfer_buf[0];
	if (sdio_header_length != rx_length) {
		BTMTK_ERR("%s sdio header length %d, rx_length %d mismatch, trigger assert",
			__func__, sdio_header_length, rx_length);
		//BTMTK_INFO_RAW(cif_dev->transfer_buf, rx_length, "%s: RAW:", __func__);
		//btmtk_send_assert_cmd(bdev);
		return -EIO;
	}

	//BTMTK_INFO_RAW(cif_dev->transfer_buf, rx_length, "%s: RAW:", __func__);

	hci_type = cif_dev->transfer_buf[MTK_SDIO_PACKET_HEADER_SIZE];
	
	switch (hci_type) {
	/* Please reference hci header format
	 * A = len
	 * acl : 02 xx xx AA AA + payload
	 * sco : 03 xx xx AA + payload
	 * evt : 04 xx AA + payload
	 * ISO : 05 xx xx AA AA + payload
	 */
	case HCI_ACLDATA_PKT:
		hci_pkt_len = cif_dev->transfer_buf[MTK_SDIO_PACKET_HEADER_SIZE + 3] +
						(cif_dev->transfer_buf[MTK_SDIO_PACKET_HEADER_SIZE + 4] << 8) + 5;
		break;
	case HCI_SCODATA_PKT:
		hci_pkt_len = cif_dev->transfer_buf[MTK_SDIO_PACKET_HEADER_SIZE + 3] + 4; //only used for sdio inband
		break;
	case HCI_EVENT_PKT:
		hci_pkt_len = cif_dev->transfer_buf[MTK_SDIO_PACKET_HEADER_SIZE + 2] + 3;
		break;
	}

	ret = hci_pkt_len;
	bdev->recv_evt_len = hci_pkt_len;

	//BTMTK_INFO("%s: sdio header length: %d, rx_length: %d, hci_pkt_len: = %d",
	//		__func__, sdio_header_length, rx_length, hci_pkt_len);
	ret = btmtk_recv(bdev, cif_dev->transfer_buf + MTK_SDIO_PACKET_HEADER_SIZE, hci_pkt_len);
	if (cif_dev->transfer_buf[4] == HCI_EVENT_PKT) {
		payload = rx_length - cif_dev->transfer_buf[6] - 3;
		ret = rx_length - MTK_SDIO_PACKET_HEADER_SIZE - payload;
	}

	//BTMTK_INFO("%s: done", __func__);
	return ret;
}

static int btmtk_cif_recv_evt(struct btmtk_dev *bdev)
{
	int ret = 0;
	u32 u32ReadCRValue = 0;
	u32 u32ReadCRLEN = 0;
	u32 sdio_header_length = 0;
	int rx_length = 0;
	int payload = 0;
	u16 hci_pkt_len = 0;
	u8 hci_type = 0;
	struct btmtk_sdio_dev *cif_dev = (struct btmtk_sdio_dev *)bdev->cif_dev;

    //BTMTK_INFO("%s", __func__);

	memset(bdev->io_buf, 0, IO_BUF_SIZE);

	/* keep polling method */
	/* If interrupt method is working, we can remove it */
	ret = btmtk_sdio_readl(CHISR, &u32ReadCRValue, cif_dev->func);
	BTMTK_DBG("%s: CHISR: 0x%08X", __func__, u32ReadCRValue);
#if BTMTK_SDIO_DEBUG
	rx_debug_save(CHISR_r_2, u32ReadCRValue, NULL);
#endif

	ret = btmtk_sdio_readl(CRPLR, &u32ReadCRLEN, cif_dev->func);
	BTMTK_DBG("%s: CRPLR: 0x%08X", __func__, u32ReadCRLEN);
#if BTMTK_SDIO_DEBUG
	rx_debug_save(CRPLR_r, u32ReadCRLEN, NULL);
#endif

	rx_length = (u32ReadCRLEN & RX_PKT_LEN) >> 16;
	if (rx_length == 0xFFFF || rx_length == 0) {
		BTMTK_WARN("%s: rx_length = %d, error return -EIO", __func__, rx_length);
		return -EIO;
	}

	//BTMTK_DBG("%s: u32ReadCRValue = %08X", __func__, u32ReadCRValue);
	u32ReadCRValue &= 0xFFFB;
	ret = btmtk_sdio_writel(CHISR, u32ReadCRValue, cif_dev->func);
	// BTMTK_DBG("%s: write = %08X", __func__, u32ReadCRValue);
	ret = btmtk_sdio_readl(PD2HRM0R, &u32ReadCRValue, cif_dev->func);
#if BTMTK_SDIO_DEBUG
	rx_debug_save(PD2HRM0R_r, u32ReadCRValue, NULL);
#endif

    memset(cif_dev->transfer_buf, 0, URB_MAX_BUFFER_SIZE);
	ret = btmtk_sdio_readsb(CRDR, cif_dev->transfer_buf, rx_length, cif_dev->func);
#if BTMTK_SDIO_DEBUG
	rx_debug_save(RX_BUF, 0, cif_dev->transfer_buf);
#endif
	sdio_header_length = (cif_dev->transfer_buf[1] << 8);
	sdio_header_length |= cif_dev->transfer_buf[0];
	if (sdio_header_length != rx_length) {
		BTMTK_ERR("%s sdio header length %d, rx_length %d mismatch, trigger assert",
			__func__, sdio_header_length, rx_length);
		BTMTK_INFO_RAW(cif_dev->transfer_buf, rx_length, "%s: RAW:", __func__);
		//btmtk_send_assert_cmd(bdev);
		return -EIO;
	}

	//BTMTK_INFO_RAW(cif_dev->transfer_buf, rx_length, "%s: RAW:", __func__);

	hci_type = cif_dev->transfer_buf[MTK_SDIO_PACKET_HEADER_SIZE];
	
	switch (hci_type) {
	/* Please reference hci header format
	 * A = len
	 * acl : 02 xx xx AA AA + payload
	 * sco : 03 xx xx AA + payload
	 * evt : 04 xx AA + payload
	 * ISO : 05 xx xx AA AA + payload
	 */
	case HCI_ACLDATA_PKT:
		hci_pkt_len = cif_dev->transfer_buf[MTK_SDIO_PACKET_HEADER_SIZE + 3] +
						(cif_dev->transfer_buf[MTK_SDIO_PACKET_HEADER_SIZE + 4] << 8) + 5;
		break;
	case HCI_SCODATA_PKT:
		hci_pkt_len = cif_dev->transfer_buf[MTK_SDIO_PACKET_HEADER_SIZE + 3] + 4; //only used for sdio inband
		break;
	case HCI_EVENT_PKT:
		hci_pkt_len = cif_dev->transfer_buf[MTK_SDIO_PACKET_HEADER_SIZE + 2] + 3;
		break;
	}

	ret = hci_pkt_len;
	bdev->recv_evt_len = hci_pkt_len;

	//BTMTK_INFO("%s: sdio header length: %d, rx_length: %d, hci_pkt_len: = %d",
	//		__func__, sdio_header_length, rx_length, hci_pkt_len);
	ret = btmtk_recv(bdev, cif_dev->transfer_buf + MTK_SDIO_PACKET_HEADER_SIZE, hci_pkt_len);
	if (cif_dev->transfer_buf[4] == HCI_EVENT_PKT) {
		payload = rx_length - cif_dev->transfer_buf[6] - 3;
		ret = rx_length - MTK_SDIO_PACKET_HEADER_SIZE - payload;
	}

	//BTMTK_INFO("%s: done", __func__);
	return ret;
}

static int btmtk_tx_pkt(struct btmtk_sdio_dev *cif_dev, sk_buff *skb)
{
	u8 MultiBluckCount = 0;
	u8 redundant = 0;
	int len = 0;
	int ret = 0;

	//BTMTK_INFO("%s", __func__);
	
	memset(cif_dev->sdio_packet, 0, URB_MAX_BUFFER_SIZE);

	cif_dev->sdio_packet[0] = (4 + skb->len) & 0xFF;
	cif_dev->sdio_packet[1] = ((4 + skb->len) & 0xFF00) >> 8;

	memcpy(cif_dev->sdio_packet + MTK_SDIO_PACKET_HEADER_SIZE, skb->data,
		skb->len);
	len = skb->len + MTK_SDIO_PACKET_HEADER_SIZE;

	//BTMTK_INFO_RAW(skb->data, skb->len, "%s skb:", __func__);

	MultiBluckCount = len / SDIO_BLOCK_SIZE;
	redundant = len % SDIO_BLOCK_SIZE;
	if (redundant) {
		len = (MultiBluckCount+1)*SDIO_BLOCK_SIZE;
	}
	atomic_set(&cif_dev->tx_rdy, 0);
	//BTMTK_DBG("%s, tx_rdy:%d", __func__, atomic_read(&cif_dev->tx_rdy));
	//(cif_dev->sdio_packet, len, "%s sdio_packet:", __func__);
	ret = btmtk_sdio_writesb(CTDR, cif_dev->sdio_packet, len, cif_dev->func);
	if (ret < 0) {
		BTMTK_ERR("ret = %d", ret);
	}
	skb_free(&skb);
	return ret;
}

static int btmtk_sdio_interrupt_process(struct btmtk_dev *bdev)
{
	struct btmtk_sdio_dev *cif_dev = (struct btmtk_sdio_dev *)bdev->cif_dev;
	int ret = 0;
	int state = btmtk_get_chip_state(bdev);
	u32 u32ReadCRValue = 0;
	 
	//BTMTK_INFO("%s", __func__);

	ret = btmtk_sdio_readl(CHISR, &u32ReadCRValue, cif_dev->func);
#if BTMTK_SDIO_DEBUG
	rx_debug_save(CHISR_r_1, u32ReadCRValue, NULL);
#endif
	//BTMTK_INFO("%s, CHISR: 0x%08x", __func__, u32ReadCRValue);

	if (u32ReadCRValue & FIRMWARE_INT_BIT15) {
		btmtk_sdio_set_no_fwn_own(cif_dev, 1);
		btmtk_sdio_writel(PH2DSM0R, PH2DSM0R_DRIVER_OWN, cif_dev->func);
	}

	if (u32ReadCRValue & FIRMWARE_INT_BIT31) {
		/* clean tx queue */
		RtbEmptyQueue(cif_dev->tx_queue);
		BTMTK_INFO("clean tx_queue:%ld ", RtbGetQueueLen(cif_dev->tx_queue));

		/* It's read-only bit (WDT interrupt)
		 * Host can't modify it.
		 */
		ret = btmtk_sdio_readl(CHISR, &u32ReadCRValue, cif_dev->func);
		BTMTK_INFO("%s CHISR 0x%08x", __func__, u32ReadCRValue);
		//DUMP_TIME_STAMP("notify_chip_reset");
		//btmtk_reset_trigger(bdev);
		return ret;
	}

	if (TX_EMPTY & u32ReadCRValue) {
		ret = btmtk_sdio_writel(CHISR, (TX_EMPTY | TX_COMPLETE_COUNT), cif_dev->func);
		atomic_set(&cif_dev->tx_rdy, 1);
		//BTMTK_INFO("%s, tx_rdy:%d", __func__, atomic_read(&cif_dev->tx_rdy));
		if (ret < 0)
			BTMTK_ERR(" %s, ret:%d", __func__, ret);
#if BTMTK_SDIO_DEBUG
		tx_empty_cnt++;
#endif
	}

	if (RX_DONE & u32ReadCRValue) {
		ret = btmtk_cif_recv_evt(bdev);
		//BTMTK_INFO("%s recv_evt, ret = %d", __func__, ret);
	}

	if (state == BTMTK_STATE_WORKING) {
		ret = btmtk_sdio_enable_interrupt(1, cif_dev->func);
	}
	
	//BTMTK_INFO("%s done, ret = %d", __func__, ret);
	return ret;
}

static int btmtk_sdio_enable_host_int(struct btmtk_sdio_dev *cif_dev)
{
	int ret = 0;
	u32 read_data = 0;

	if (!cif_dev || !cif_dev->func)
		return -EINVAL;

	/* workaround for some platform no host clock sometimes */

	ret = btmtk_sdio_readl(CSDIOCSR, &read_data, cif_dev->func);
	BTMTK_INFO("%s read CSDIOCSR is 0x%X, ret = %d", __func__, read_data, ret);
	read_data |= 0x4;
	ret = btmtk_sdio_writel(CSDIOCSR, read_data, cif_dev->func);
	BTMTK_INFO("%s write CSDIOCSR is 0x%X, ret = %d", __func__, read_data, ret);

	return ret;
}

static void btmtk_sdio_stop_main_thread(struct btmtk_sdio_dev *cif_dev)
{
	u8 i = 0;

	vTaskDelete(cif_dev->sdio_thread.task);
	xEventGroupSetBits(cif_dev->sdio_thread.xEventGroup, BTMTK_SDIO_THREAD_STOP);

	while (atomic_read(&cif_dev->sdio_thread.thread_status) && i < RETRY_TIMES) {
		BTMTK_INFO("wait btmtk_sdio_main_thread stop");
		vTaskDelay(pdMS_TO_TICKS(100));
		i++;
		if (i == RETRY_TIMES - 1) {
			BTMTK_INFO("wait btmtk_sdio_main_thread stop failed");
			break;
		}
	}
	BTMTK_INFO("btmtk_sdio_stop_main_thread end!");
	
}

static void btmtk_sdio_interrupt(struct sdio_func *func)
{
	struct btmtk_dev *bdev;
	struct btmtk_sdio_dev *cif_dev;
	u32 cr_value = 0;
	u32 u32ReadCRValue = 0;
	int ret = 0;
	//int state = btmtk_get_chip_state(bdev);

	//BTMTK_INFO("%s", __func__);

	bdev = sdio_get_drvdata(func);
	if (!bdev)
		return;

	/*Normally, unlock it in main thread to make sure the RX data is received in milliseconds.
	Just set timeout in case unexpected error happens*/
	//btmtk_sdio_wake_lock(g_sdio_dev.irq_ws, 1000);
	cif_dev = (struct btmtk_sdio_dev *)bdev->cif_dev;
	atomic_set(&cif_dev->int_count, 1);

	//btmtk_sdio_enable_interrupt(0, cif_dev->func);

	/*disable interrupt*/
	//BTMTK_INFO("disable %s", __func__);
	cr_value |= C_FW_INT_EN_CLEAR;
	ret = btmtk_sdio_writel_without_claim(CHLPCR, cr_value, func);
	
	//BTMTK_INFO("read CHISR %s", __func__);
	ret = btmtk_sdio_readl_without_claim(CHISR, &u32ReadCRValue, cif_dev->func);

	//BTMTK_INFO("%s, CHISR: 0x%08x", __func__, u32ReadCRValue);

	if (u32ReadCRValue & FIRMWARE_INT_BIT15) {
		btmtk_sdio_set_no_fwn_own(cif_dev, 1);
		ret = btmtk_sdio_writel_without_claim(PH2DSM0R, PH2DSM0R_DRIVER_OWN, cif_dev->func);
	}

	if (u32ReadCRValue & FIRMWARE_INT_BIT31) {
		/* clean tx queue */
		RtbEmptyQueue(cif_dev->tx_queue);
		BTMTK_INFO("clean tx_queue:%ld ", RtbGetQueueLen(cif_dev->tx_queue));

		/* It's read-only bit (WDT interrupt)
		 * Host can't modify it.
		 */
		ret = btmtk_sdio_readl_without_claim(CHISR, &u32ReadCRValue, cif_dev->func);
		BTMTK_INFO("%s CHISR 0x%08x", __func__, u32ReadCRValue);
		//DUMP_TIME_STAMP("notify_chip_reset");
		//btmtk_reset_trigger(bdev);
		return;
	}

	if (TX_EMPTY & u32ReadCRValue) {
		ret = btmtk_sdio_writel_without_claim(CHISR, (TX_EMPTY | TX_COMPLETE_COUNT), cif_dev->func);
		atomic_set(&cif_dev->tx_rdy, 1);
		//BTMTK_INFO("%s, tx_rdy:%d", __func__, atomic_read(&cif_dev->tx_rdy));
		if (ret < 0)
			BTMTK_ERR(" %s, ret:%d", __func__, ret);
	}

	if (RX_DONE & u32ReadCRValue) {
		ret = btmtk_cif_recv_evt_ext(bdev);
		//BTMTK_INFO("%s recv_evt, ret = %d", __func__, ret);
	}


	/*enable interrupt*/
	//BTMTK_INFO("enable interrupt %s", __func__);
	cr_value = 0;
	cr_value |= C_FW_INT_EN_SET;
	ret = btmtk_sdio_writel_without_claim(CHLPCR, cr_value, func);

	//BTMTK_INFO("%s end", __func__);

}

static int btmtk_sdio_register_dev(struct btmtk_sdio_dev *bdev)
{
	struct sdio_func *func;
	u8	u8ReadCRValue = 0;
	int ret = 0;

	if (!bdev || !bdev->func) {
		BTMTK_ERR("Error: card or function is NULL!");
		ret = -EINVAL;
		goto failed;
	}

	func = bdev->func;

	sdio_claim_host(func);
	ret = sdio_enable_func(func);
	sdio_release_host(func);
	if (ret) {
		BTMTK_ERR("sdio_enable_func() failed: ret=%d", ret);
		ret = -EIO;
		goto failed;
	}

	//sdio_f0_readb(SDIO_CCCR_IENx, &u8ReadCRValue, func);
	//BTMTK_INFO("SDIO_CCCR_IENx %x, func num %d", u8ReadCRValue, func->num);

	sdio_claim_host(func);
	ret = sdio_claim_irq(func, btmtk_sdio_interrupt);
	sdio_release_host(func);
	if (ret) {
		BTMTK_ERR("sdio_claim_irq failed: ret=%d", ret);
		ret = -EIO;
		goto disable_func;
	}

	BTMTK_INFO("sdio_claim_irq success: ret=%d", ret);

	//sdio_f0_readb(SDIO_CCCR_IENx, &u8ReadCRValue, func);
	//BTMTK_INFO("SDIO_CCCR_IENx %x", u8ReadCRValue);

	sdio_claim_host(func);
	ret = sdio_set_block_size(func, SDIO_BLOCK_SIZE);
	sdio_release_host(func);
	if (ret) {
		BTMTK_ERR("cannot set SDIO block size");
		ret = -EIO;
		goto release_irq;
	}

	return 0;

release_irq:
	sdio_release_irq(func);

disable_func:
	sdio_disable_func(func);

failed:
	BTMTK_INFO("%s fail", __func__);
	return ret;
}

static int btmtk_sdio_set_write_clear(struct btmtk_sdio_dev *cif_dev)
{
	u32 u32ReadCRValue = 0;
	int ret = 0;

	ret = btmtk_sdio_readl(CHCR, &u32ReadCRValue, cif_dev->func);
	if (ret) {
		BTMTK_ERR("%s read CHCR error", __func__);
		ret = EINVAL;
		return ret;
	}

	u32ReadCRValue |= 0x00000002;
	btmtk_sdio_writel(CHCR, u32ReadCRValue, cif_dev->func);
	BTMTK_INFO("%s write CHCR 0x%08X", __func__, u32ReadCRValue);
	ret = btmtk_sdio_readl(CHCR, &u32ReadCRValue, cif_dev->func);
	BTMTK_INFO("%s read CHCR 0x%08X", __func__, u32ReadCRValue);
	if (u32ReadCRValue&0x00000002)
		BTMTK_INFO("%s write clear", __func__);
	else
		BTMTK_INFO("%s read clear", __func__);

	return ret;
}

void btmtk_sdio_set_no_fwn_own(struct btmtk_sdio_dev *cif_dev, int flag)
{
	if (cif_dev->no_fw_own != flag) {
		BTMTK_INFO("%s set no_fw_own %d", __func__, flag);
	}
	cif_dev->no_fw_own = flag;
}

static int btmtk_sdio_set_fw_own(struct btmtk_sdio_dev *cif_dev, int retry)
{
	/*Set fw own*/
	int ret = 0;
	u32 u32LoopCount = 0;
	u32 u32PollNum = 0;
	u32 u32CHLPCRValue = 0;
	u32 u32PD2HRM0RValue = 0;
	u32 ownValue = 0;
	u32 i = 0;
	u8 chlpcr_driver_own = 0;
	u8 pd2hrm0r_driver_own = 0;

	BTMTK_DBG("%s", __func__);

	//atomic_set(&cif_dev->fw_own_timer_flag, FW_OWN_TIMER_INIT);

	if (cif_dev->no_fw_own) {
		return 0;
	}

	// SDIO_OWN_MUTEX_LOCK();

	/* For CHLPCR, bit 8 could help us to check driver own or fw own
	 * 0: COM driver doesn't have ownership
	 * 1: COM driver has ownership
	 */
	ret = btmtk_sdio_readl(CHLPCR, &u32CHLPCRValue, cif_dev->func);
	if (ret) {
		BTMTK_ERR("%s read CHLPCR fail ret: %d", __func__, ret);
		goto done;
	}
	chlpcr_driver_own = ((u32CHLPCRValue & 0x100) == 0x100) ? 1 : 0;

	if (cif_dev->patched == 1) {
		ret = btmtk_sdio_readl(PD2HRM0R, &u32PD2HRM0RValue, cif_dev->func);
		if (ret) {
			BTMTK_ERR("%s read PD2HRM0R fail ret: %d", __func__, ret);
			goto done;
		}
		pd2hrm0r_driver_own = ((u32PD2HRM0RValue & PD2HRM0R_DRIVER_OWN) == PD2HRM0R_DRIVER_OWN) ? 1 : 0;
	} else {
		pd2hrm0r_driver_own = 0;
	}

	BTMTK_DBG("CHLPCR: 0x%0x, PD2HRM0R: 0x%0x", u32CHLPCRValue, u32PD2HRM0RValue);

	if (!chlpcr_driver_own && !pd2hrm0r_driver_own) {
		ret = 0;
		goto unlock;
	}

	ownValue = 0x00000100;
retry_own:
	if (cif_dev->patched == 1) {
		/* write CSICR to notify FW to set PD2HRM0R to 0 */
		ret = btmtk_sdio_writel(CSICR, 1, cif_dev->func);
		if (ret) {
			BTMTK_ERR("%s write CSICR 1 fail ret: %d", __func__, ret);
			ret = -EINVAL;
			goto done;
		}

		ret = btmtk_sdio_writel(CSICR, 0xC0, cif_dev->func);
		if (ret) {
			BTMTK_ERR("%s write CSICR 0xC0 fail ret: %d", __func__, ret);
			ret = -EINVAL;
			goto done;
		}

		u32LoopCount = SET_OWN_LOOP_COUNT;
		pd2hrm0r_driver_own = 0;
		do {
			vTaskDelay(SDIO_IO_DELAY / portTICK_RATE_MS);
			u32LoopCount--;
			u32PollNum++;
			ret = btmtk_sdio_readl(PD2HRM0R, &u32PD2HRM0RValue, cif_dev->func);
			if (ret) {
				BTMTK_ERR("%s read CHLPCR fail ret: %d", __func__, ret);
				goto done;
			}
			BTMTK_DBG("%s set driver own PD2HRM0R = 0x%0x", __func__, u32PD2HRM0RValue);
			pd2hrm0r_driver_own = ((u32PD2HRM0RValue & PD2HRM0R_DRIVER_OWN) == PD2HRM0R_DRIVER_OWN) ? 1 : 0;
		} while ((u32LoopCount > 0) && pd2hrm0r_driver_own);

		if (pd2hrm0r_driver_own) {
			if (retry > 0) {
				BTMTK_WARN("%s retry set_check fw own(%d), PD2HRM0R:0x%x", __func__, u32PollNum, u32PD2HRM0RValue);
				/*
				for (i = 0; i < 3; i++) {
					DUMP_FW_PC(cif_dev);
				}
				*/
				if (u32PollNum == 30) {
					goto unlock;
				}
				retry--;
				// usleep_range(5*1000, 10*1000);
				vTaskDelay(SDIO_IO_DELAY / portTICK_RATE_MS);
				goto retry_own;
			} else {
				ret = -EINVAL;
			}
		}
	}

	/* Write CR for Driver or FW own */
	if (ret == 0) {
		ret = btmtk_sdio_writel(CHLPCR, ownValue, cif_dev->func);
		if (ret) {
			BTMTK_ERR("%s write CHLPCR fail ret: %d", __func__, ret);
			ret = -EINVAL;
			goto done;
		}
	}
done:
#if BTMTK_SDIO_DEBUG
	fw_own_cnt++;
#endif
	if (ret) {
		BTMTK_ERR("%s set FW own fail, ret: %d", __func__, ret);
		if (ret != -ETIMEDOUT) {
			// btmtk_sdio_dump_debug_sop(cif_dev->bdev);
		}
	} else {
		BTMTK_DBG("%s set FW own success", __func__);
	}

unlock:
	// SDIO_OWN_MUTEX_UNLOCK();
	return ret;
}

static int btmtk_sdio_set_driver_own(struct btmtk_sdio_dev *cif_dev, int retry)
{
	/*Set driver own*/
	int ret = 0;
	u32 u32LoopCount = 0;
	u32 u32PollNum = 0;
	u32 u32CHLPCRValue = 0;
	u32 u32PD2HRM0RValue = 0;
	u32 ownValue = 0;
	u32 i = 0;
	u8 chlpcr_driver_own = 0;
	u8 pd2hrm0r_driver_own = 0;

	BTMTK_DBG("%s", __func__);

/*
	if (cif_dev->no_fw_own == 0)
		btmtk_sdio_update_fw_own_timer(cif_dev);
*/

	// SDIO_OWN_MUTEX_LOCK();
	/* For CHLPCR, bit 8 could help us to check driver own or fw own
	 * 0: COM driver doesn't have ownership
	 * 1: COM driver has ownership
	 */
	ret = btmtk_sdio_readl(CHLPCR, &u32CHLPCRValue, cif_dev->func);
	if (ret) {
		BTMTK_ERR("%s read CHLPCR fail ret: %d", __func__, ret);
		goto done;
	}
	chlpcr_driver_own = ((u32CHLPCRValue & 0x100) == 0x100) ? 1 : 0;

	if (cif_dev->patched == 1) {
		ret = btmtk_sdio_readl(PD2HRM0R, &u32PD2HRM0RValue, cif_dev->func);
		if (ret) {
			BTMTK_ERR("%s read PD2HRM0R fail ret: %d", __func__, ret);
			goto done;
		}
		pd2hrm0r_driver_own = ((u32PD2HRM0RValue & PD2HRM0R_DRIVER_OWN) == PD2HRM0R_DRIVER_OWN) ? 1 : 0;
	} else {
		pd2hrm0r_driver_own = 1;
	}

	BTMTK_DBG("CHLPCR: 0x%0x, PD2HRM0R: 0x%0x", u32CHLPCRValue, u32PD2HRM0RValue);

	if (chlpcr_driver_own && pd2hrm0r_driver_own) {
		ret = 0;
		goto unlock;
	}

	ownValue = 0x00000200;
retry_own:
	/* Write CR for Driver or FW own */
	ret = btmtk_sdio_writel(CHLPCR, ownValue, cif_dev->func);
	if (ret) {
		BTMTK_ERR("%s write CHLPCR fail ret: %d", __func__, ret);
		ret = -EINVAL;
		goto done;
	}

	u32LoopCount = SET_OWN_LOOP_COUNT;
	do {
		vTaskDelay(SDIO_IO_DELAY / portTICK_RATE_MS);
		u32LoopCount--;
		u32PollNum++;
		ret = btmtk_sdio_readl(CHLPCR, &u32CHLPCRValue, cif_dev->func);
		if (ret) {
			BTMTK_ERR("%s read CHLPCR fail ret: %d", __func__, ret);
			goto done;
		}
		BTMTK_DBG("%s set driver own CHLPCR = 0x%0x", __func__, u32CHLPCRValue);
		chlpcr_driver_own = ((u32CHLPCRValue & 0x100) == 0x100) ? 1 : 0;
		if (cif_dev->patched == 1) {
			ret = btmtk_sdio_readl(PD2HRM0R, &u32PD2HRM0RValue, cif_dev->func);
			if (ret) {
				BTMTK_ERR("%s read CHLPCR fail ret: %d", __func__, ret);
				goto done;
			}
			BTMTK_DBG("%s set driver own PD2HRM0R = 0x%0x", __func__, u32PD2HRM0RValue);
			pd2hrm0r_driver_own = ((u32PD2HRM0RValue & PD2HRM0R_DRIVER_OWN) == PD2HRM0R_DRIVER_OWN) ? 1 : 0;
		} else {
			pd2hrm0r_driver_own = 1;
		}
	} while ((u32LoopCount > 0) && (!chlpcr_driver_own || !pd2hrm0r_driver_own));

	if (!chlpcr_driver_own || !pd2hrm0r_driver_own) {
		if (retry > 0) {
			BTMTK_WARN("%s retry set_check driver own(%d), CHLPCR:0x%x, PD2HRM0R:0x%x", __func__, u32PollNum, u32CHLPCRValue, u32PD2HRM0RValue);
			/*
			for (i = 0; i < 3; i++) {
				DUMP_FW_PC(cif_dev);
			}
			*/
			retry--;
			vTaskDelay(SDIO_IO_DELAY / portTICK_RATE_MS);
			goto retry_own;
		} else {
			ret = -EINVAL;
		}
	}
done:
#if BTMTK_SDIO_DEBUG
	driver_own_cnt++;
#endif
	if (ret) {
		BTMTK_ERR("%s set driver own fail, ret: %d", __func__, ret);
		if (ret != -ETIMEDOUT) {
			/*
			for (i = 0; i < 8; i++) {
				DUMP_FW_PC(cif_dev);
				vTaskDelay(SDIO_IO_DELAY / portTICK_RATE_MS);
			}
			*/
			// btmtk_sdio_dump_debug_sop(cif_dev->bdev);
		}
	} else {
		BTMTK_DBG("%s set driver own success", __func__);
	}
unlock:
	// SDIO_OWN_MUTEX_UNLOCK();

	return ret;
}

static int btmtk_sdio_keep_driver_own(struct btmtk_sdio_dev *cif_dev, int enable)
{
	int ret = 0;

	if (enable == 1) {
		btmtk_sdio_set_no_fwn_own(cif_dev, 1);
		ret = btmtk_sdio_set_driver_own(cif_dev, RETRY_TIMES);
	} else {
		btmtk_sdio_set_no_fwn_own(cif_dev, 0);
		ret = btmtk_sdio_set_fw_own(cif_dev, RETRY_TIMES);
	}
	return ret;
}

static int btmtk_cif_allocate_memory(struct btmtk_sdio_dev *cif_dev)
{
	int ret = 0;

	if (cif_dev->transfer_buf == NULL) {
		cif_dev->transfer_buf = malloc(URB_MAX_BUFFER_SIZE);
		if (!cif_dev->transfer_buf) {
			BTMTK_ERR("%s: alloc memory fail (bdev->transfer_buf)", __func__);
			goto end;
		}
		memset(cif_dev->transfer_buf, 0, URB_MAX_BUFFER_SIZE);
	}

	if (cif_dev->sdio_packet == NULL) {
		cif_dev->sdio_packet = malloc(URB_MAX_BUFFER_SIZE);
		if (!cif_dev->sdio_packet) {
			BTMTK_ERR("%s: alloc memory fail (bdev->transfer_buf)", __func__);
			goto err;
		}
		memset(cif_dev->sdio_packet, 0, URB_MAX_BUFFER_SIZE);
	}

	BTMTK_INFO("%s: Done", __func__);
	return 0;
err:
	free(cif_dev->transfer_buf);
	cif_dev->transfer_buf = NULL;
end:
	return ret;
}

static void btmtk_cif_free_memory(struct btmtk_sdio_dev *cif_dev)
{
	free(cif_dev->transfer_buf);
	cif_dev->transfer_buf = NULL;

	free(cif_dev->sdio_packet);
	cif_dev->sdio_packet = NULL;

	BTMTK_INFO("%s: Success", __func__);
}

/*
 * This function handles the event generated by firmware, rx data
 * received from firmware, and tx data sent from kernel.
 */
static void btmtk_sdio_main_thread(void *data)
{
	struct btmtk_dev *bdev = data;
	struct btmtk_sdio_dev *cif_dev = NULL;
	sk_buff *skb;
	int ret = 0;
	EventBits_t uxBitsToWaitFor = BTMTK_SDIO_THREAD_STOP | BTMTK_SDIO_THREAD_TX | BTMTK_SDIO_THREAD_RX |
	                            BTMTK_SDIO_THREAD_FW_OWN;
    EventBits_t uxBits;

	cif_dev = (struct btmtk_sdio_dev *)bdev->cif_dev;

	atomic_set(&cif_dev->sdio_thread.thread_status, 1);
	BTMTK_INFO("thread_status = %d, btmtk_sdio_main_thread start running...",
		atomic_read(&cif_dev->sdio_thread.thread_status));

	for (;;) {
		BTMTK_DBG("%s start waiting", __func__);
		uxBits = xEventGroupWaitBits(cif_dev->sdio_thread.xEventGroup, 
				uxBitsToWaitFor, pdTRUE, pdFALSE, portMAX_DELAY);
		
		if ((uxBits & uxBitsToWaitFor) != 0) {
            if (uxBits & BTMTK_SDIO_THREAD_STOP) {
                BTMTK_WARN("sdio_thread: break from main thread");
				break;
            }
		}

		//BTMTK_INFO("main_thread doing...");
		if (uxBits & BTMTK_SDIO_THREAD_FW_OWN) {
			BTMTK_INFO("BTMTK_SDIO_THREAD_FW_OWN doing...");
			ret = btmtk_sdio_set_fw_own(cif_dev, RETRY_TIMES);
			if (ret) {
				BTMTK_ERR("set fw own return fail");
				//btmtk_reset_trigger(bdev);
				break;
			}
		}

		/* Do interrupt */
		if (uxBits & BTMTK_SDIO_THREAD_RX) {
			BTMTK_INFO("main_thread go int");
			atomic_set(&cif_dev->int_count, 0);
			//btmtk_sdio_wake_unlock(g_sdio_dev.irq_ws);
			if (btmtk_sdio_interrupt_process(bdev)) {
				BTMTK_ERR("btmtk_sdio_interrupt_process fail");
				//btmtk_reset_trigger(bdev);
				break;
			}
		} else {
			BTMTK_DBG("go tx");
		}

		if (uxBits & BTMTK_SDIO_THREAD_TX) {
			//BTMTK_DBG("%s, check tx_rdy:%d", __func__, atomic_read(&cif_dev->tx_rdy));
			if (!atomic_read(&cif_dev->tx_rdy))
			{
				BTMTK_INFO("%s, not ready tx_rdy:%d", __func__, atomic_read(&cif_dev->tx_rdy));
				continue;
			}
            skb = RtbDequeueHead(cif_dev->tx_queue);
            while(skb) {
                ret = btmtk_tx_pkt(cif_dev, skb);
                if (ret) {

                BTMTK_ERR("tx pkt return fail %d", ret);
                //btmtk_reset_trigger(bdev);
                goto exit;
                break;
                }
                skb = RtbDequeueHead(cif_dev->tx_queue);
                if(skb)
                {
                    BTMTK_INFO(" ====== main_thread has multi pkt to send =====\n\n");
                }
            }
         }
    }
exit:
	atomic_set(&cif_dev->sdio_thread.thread_status, 0);
	return;
}

static int btmtk_sdio_probe(struct sdio_func *func,
					const struct sdio_device_id *id)
{
	int err = -1;
	struct btmtk_dev *bdev = NULL;
	struct btmtk_sdio_dev *cif_dev = NULL;
	struct btmtk_main_info *bmain_info = btmtk_get_main_info();
	u8 i = 0;
	u8 cap_init_fail = 0;

	bdev = sdio_get_drvdata(func);
	if (!bdev) {
		BTMTK_ERR("[ERR] bdev is NULL");
		return -ENOMEM;
	}
	bmain_info->chip_reset_flag = 0;

	cif_dev = (struct btmtk_sdio_dev *)bdev->cif_dev;

	cif_dev->func = func;
	cif_dev->bdev = bdev;

	BTMTK_INFO("%s 1. func device %p", __func__, func);
	cif_dev->patched = 0;

	/* it's for L0/L0.5 reset */
	//INIT_WORK(&bdev->reset_waker, btmtk_reset_waker);

	spinLockInit(&bdev->txlock);
	spinLockInit(&bdev->rxlock);

	if (btmtk_sdio_register_dev(cif_dev) < 0) {
		BTMTK_ERR("Failed to register BT device!");
		return -ENODEV;
	}
	BTMTK_INFO("%s 2. btmtk_sdio_register_dev done", __func__);

	/* Disable the interrupts on the card */
	btmtk_sdio_enable_host_int(cif_dev);
	BTMTK_INFO("%s 3. call btmtk_sdio_enable_host_int done", __func__);

	sdio_set_drvdata(func, bdev);
	//BUG:tmp patch: disable interrupt before probe done
	btmtk_sdio_enable_interrupt(0, cif_dev->func);

	btmtk_sdio_keep_driver_own(cif_dev, 1);

	BTMTK_INFO("%s 4. call btmtk_sdio_keep_driver_own done", __func__);
	cif_dev->tx_queue = RtbQueueInit();

	//init main thread parameter
	cif_dev->sdio_thread.wait_semaphore = xSemaphoreCreateBinary();
    if (cif_dev->sdio_thread.wait_semaphore == NULL) {
 		BTMTK_ERR("create sdio_thread.wait_semaphore fail");
    }

	cif_dev->sdio_thread.xEventGroup = xEventGroupCreate();
	if (cif_dev->sdio_thread.xEventGroup == NULL) {
 		BTMTK_ERR("create sdio_thread.xEventGroup fail");
    }

	//compare event
	bdev->compare_event = xEventGroupCreate();
	if (bdev->compare_event == NULL) {
 		BTMTK_ERR("create compare_event fail");
    }

	//create main thread
	atomic_set(&cif_dev->int_count, 0);
	atomic_set(&cif_dev->tx_rdy,1);
	//BTMTK_INFO("%s, tx_rdy:%d", __func__, atomic_read(&cif_dev->tx_rdy));

	xTaskCreate(btmtk_sdio_main_thread, "btmtk_sdio_main_thread", 200, (void *)bdev, 
									tskIDLE_PRIORITY + 1, &cif_dev->sdio_thread.task);
	
	BTMTK_INFO("%s 5. btmtk_sdio_main_thread done 200 byte", __func__);

	/* Set interrupt output */
	err = btmtk_sdio_writel(CHIER, FIRMWARE_INT_BIT31 | FIRMWARE_INT_BIT15 |
			FIRMWARE_INT | TX_FIFO_OVERFLOW |
			FW_INT_IND_INDICATOR | TX_COMPLETE_COUNT |
			TX_UNDER_THOLD | TX_EMPTY | RX_DONE, cif_dev->func);
	if (err) {
		BTMTK_ERR("Set interrupt output fail(%d)", err);
		err = -EIO;
		// goto free_thread;
	}

	/* write clear method */
	btmtk_sdio_set_write_clear(cif_dev);

	BTMTK_INFO("%s 6. Set interrupt output done", __func__);

	err = btmtk_cif_allocate_memory(cif_dev);
	if (err < 0) {
		BTMTK_ERR("[ERR] btmtk_cif_allocate_memory failed!");
		// goto free_thread;
	}
	BTMTK_INFO("%s 7. btmtk_cif_allocate_memory done", __func__);

	/* temp solution for fix when do read chip id, main thread don't start at the time,
	 * then read chip id will be failed. The final solution maybe need move all
	 * cmd to the new tx thread.
	 */
	while (!atomic_read(&cif_dev->sdio_thread.thread_status) && i < CHECK_THREAD_RETRY_TIMES) {
		BTMTK_INFO("Error, main thread start too slow,wait btmtk_sdio_main_thread start(%d)",cif_dev->sdio_thread.thread_status);
		//usleep_range(5*1000, 10*1000);
		 TickType_t xDelayInTicks = pdMS_TO_TICKS((rand() % (10000 - 5000 + 1)) + 5000); // 生成5000到10000之间的随机延迟时间
		 vTaskDelay(xDelayInTicks);
		i++;
		if (i == RETRY_TIMES - 1) {
			BTMTK_WARN("wait btmtk_sdio_main_thread start failed, do chip reset!!!");
		}
	}

	err = btmtk_main_cif_initialize(bdev, HCI_SDIO);
	if (err < 0) {
		if (err == -EIO) {
			BTMTK_ERR("[ERR] btmtk_main_cif_initialize failed, do chip reset!!!");
			cap_init_fail = 1;
			//goto exit;
		} else {
			BTMTK_ERR("[ERR] btmtk_main_cif_initialize failed!");
			//goto free_mem;
		}
	}

	BTMTK_INFO("%s 8. btmtk_load_rom_patch", __func__);
	err = btmtk_load_rom_patch(bdev);
	if (err < 0) {
		BTMTK_ERR("%s, btmtk load rom patch failed", __func__);
	}

	//BUG:tmp patch: enable interrupt after download patch success
	btmtk_sdio_enable_interrupt(1, cif_dev->func);
    return 0;

}

static void btmtk_cif_disconnect(struct sdio_func *func)
{
	BTMTK_INFO("%s", __func__);
}


int btmtk_sdio_load_fw_patch_using_dma(struct btmtk_dev *bdev, u8 *image,
		u8 *fwbuf, int section_dl_size, int section_offset)
{
	int cur_len = 0;
	int ret = -1;
	s32 sent_len = 0;
	s32 sdio_len = 0;
	s32 next_len = 0;
	u32 block_count = 0;
	u32 redundant = 0;
	u32 delay_count = 0;
	struct btmtk_sdio_dev *cif_dev = NULL;
	u8 cmd[LD_PATCH_CMD_LEN] = {0x02, 0x6F, 0xFC, 0x05, 0x00, 0x01, 0x01, 0x01, 0x00, PATCH_PHASE3};
	u8 event[LD_PATCH_EVT_LEN] = {0x04, 0xE4, 0x05, 0x02, 0x01, 0x01, 0x00, 0x00}; /* event[7] is status*/

	cif_dev = (struct btmtk_sdio_dev *)bdev->cif_dev;

	if (bdev == NULL || image == NULL || fwbuf == NULL) {
		BTMTK_ERR("%s: invalid parameters!", __func__);
		return -1;
	}

	BTMTK_INFO("%s: loading rom patch... start", __func__);
	while (section_dl_size != cur_len) {

		sent_len = (section_dl_size - cur_len) >= (UPLOAD_PATCH_UNIT - MTK_SDIO_PACKET_HEADER_SIZE) ?
			(UPLOAD_PATCH_UNIT - MTK_SDIO_PACKET_HEADER_SIZE) : (section_dl_size - cur_len);

		BTMTK_INFO("%s: sent_len = %d, cur_len = %d, delay_count = %d, next_len = %d",
				__func__, sent_len, cur_len, delay_count, next_len);

		sdio_len = sent_len + MTK_SDIO_PACKET_HEADER_SIZE;
		memset(image, 0, UPLOAD_PATCH_UNIT);
		image[0] = (sdio_len & 0x00FF);
		image[1] = (sdio_len & 0xFF00) >> 8;
		image[2] = 0;
		image[3] = 0;

		memcpy(image + MTK_SDIO_PACKET_HEADER_SIZE,
			fwbuf + section_offset + cur_len,
			sent_len);

		block_count = sdio_len / SDIO_BLOCK_SIZE;
		redundant = sdio_len % SDIO_BLOCK_SIZE;
		if (redundant) {
			sdio_len = (block_count + 1) * SDIO_BLOCK_SIZE;
		}
		atomic_set(&cif_dev->tx_rdy, 0);
		//BTMTK_INFO("%s, tx_rdy:%d", __func__, atomic_read(&cif_dev->tx_rdy));
		ret = btmtk_sdio_writesb(CTDR, image, sdio_len, cif_dev->func);
		cur_len += sent_len;
		delay_count = 0;

		if (ret < 0) {
			BTMTK_ERR("%s: send patch failed, terminate", __func__);
			goto failed;
		}
	}

	BTMTK_INFO("%s: send dl cmd", __func__);
	ret = btmtk_main_send_cmd(bdev,
			cmd, LD_PATCH_CMD_LEN,
			event, LD_PATCH_EVT_LEN,
			PATCH_DOWNLOAD_PHASE3_DELAY_TIME,
			PATCH_DOWNLOAD_PHASE3_RETRY,
			BTMTK_TX_ACL_FROM_DRV, CMD_NO_NEED_FILTER);
	if (ret < 0) {
		BTMTK_ERR("%s: send wmd dl cmd failed, terminate!", __func__);
		return ret;
	}

	BTMTK_INFO("%s: loading rom patch... Done", __func__);
	return ret;

failed:
	BTMTK_ERR("%s: loading rom patch... Failed!!!", __func__);
	//btmtk_sdio_print_debug_sr(cif_dev);
	//btmtk_sdio_dump_debug_sop(bdev);
	return ret;
}

int btmtk_sdio_send_cmd(struct btmtk_dev *bdev, sk_buff *skb,
		int delay, int retry, int pkt_type, bool flag)
{
	int ret = 0;
	u32 crAddr = 0, crValue = 0;
	ulong flags;
	struct btmtk_sdio_dev *cif_dev = NULL;

	/* for fw assert */
	u8 fw_assert_cmd[FW_ASSERT_CMD_LEN] = { 0x01, 0x5B, 0xFD, 0x00 };
	u8 fw_assert_cmd1[FW_ASSERT_CMD1_LEN] = { 0x01, 0x6F, 0xFC, 0x05, 0x01, 0x02, 0x01, 0x00, 0x08 };
	/* for read/write CR */
	u8 notify_alt_evt[NOTIFY_ALT_EVT_LEN] = {0x04, 0x0E, 0x04, 0x01, 0x03, 0x0c, 0x00};
	sk_buff *evt_skb;
	
	//BTMTK_INFO("%s", __func__);

	if (bdev == NULL) {
		BTMTK_ERR("bdev is NULL");
		ret = -1;
		goto exit;
	}
	cif_dev = (struct btmtk_sdio_dev *)bdev->cif_dev;
	if (cif_dev == NULL) {
		BTMTK_ERR("cif_dev is NULL, bdev=%p", bdev);
		ret = -1;
		goto exit;
	}

	/* For read write CR */
	if (skb->len > 9) {
		if (skb->data[0] == 0x01 && skb->data[1] == 0x6f && skb->data[2] == 0xfc &&
				skb->data[3] == 0x0D && skb->data[4] == 0x01 &&
				skb->data[5] == 0xff && skb->data[6] == 0x09 &&
				skb->data[7] == 0x00 && skb->data[8] == 0x02) {
			crAddr = ((skb->data[9] & 0xff) << 24) + ((skb->data[10] & 0xff) << 16)
				+ ((skb->data[11] & 0xff) << 8) + (skb->data[12] & 0xff);
			crValue = ((skb->data[13] & 0xff) << 24) + ((skb->data[14] & 0xff) << 16)
				+ ((skb->data[15] & 0xff) << 8) + (skb->data[16] & 0xff);

			BTMTK_INFO("%s crAddr = 0x%08x crValue = 0x%08x",
				__func__, crAddr, crValue);

			btmtk_sdio_writel(crAddr, crValue, cif_dev->func);
			evt_skb = skb_copy(skb);
			if (evt_skb) {
				skb_set_pkt_type(evt_skb, notify_alt_evt[0]) ;
				notify_alt_evt[3] = (crValue & 0xFF000000) >> 24;
				notify_alt_evt[4] = (crValue & 0x00FF0000) >> 16;
				notify_alt_evt[5] = (crValue & 0x0000FF00) >> 8;
				notify_alt_evt[6] = (crValue & 0x000000FF);
				memcpy(evt_skb->data, &notify_alt_evt[1], NOTIFY_ALT_EVT_LEN - 1);
				evt_skb->len = NOTIFY_ALT_EVT_LEN - 1;
				/* make sure memcpy done before send to hci layer */
				// smp_wmb();
				// hci_recv_frame(bdev->hdev, evt_skb);
				skb_free(&evt_skb);
				skb_free(&skb);
				skb = NULL;
			} else {
				BTMTK_ERR("%s skb_copy failed", __func__);
			}
			goto exit;
		} else if (skb->data[0] == 0x01 && skb->data[1] == 0x6f && skb->data[2] == 0xfc &&
				skb->data[3] == 0x09 && skb->data[4] == 0x01 &&
				skb->data[5] == 0xff && skb->data[6] == 0x05 &&
				skb->data[7] == 0x00 && skb->data[8] == 0x01) {

			crAddr = ((skb->data[9] & 0xff) << 24) +
				((skb->data[10] & 0xff) << 16) +
				((skb->data[11] & 0xff) << 8) +
				(skb->data[12] & 0xff);

			btmtk_sdio_readl(crAddr, &crValue, cif_dev->func);
			BTMTK_INFO("%s read crAddr = 0x%08x crValue = 0x%08x",
					__func__, crAddr, crValue);
			evt_skb = skb_copy(skb);
			if (evt_skb) {
				skb_set_pkt_type(evt_skb, notify_alt_evt[0]) ;
				/* memcpy(&notify_alt_evt[2], &crValue, sizeof(crValue)); */
				notify_alt_evt[3] = (crValue & 0xFF000000) >> 24;
				notify_alt_evt[4] = (crValue & 0x00FF0000) >> 16;
				notify_alt_evt[5] = (crValue & 0x0000FF00) >> 8;
				notify_alt_evt[6] = (crValue & 0x000000FF);
				memcpy(evt_skb->data, &notify_alt_evt[1], NOTIFY_ALT_EVT_LEN - 1);
				evt_skb->len = NOTIFY_ALT_EVT_LEN - 1;
				/* make sure memcpy done before send to hci layer */
				//smp_wmb();
				//hci_recv_frame(bdev->hdev, evt_skb);
				skb_free(&evt_skb);
				skb_free(&skb);
				skb = NULL;
			} else {
				BTMTK_ERR("%s skb_copy failed", __func__);
			}
			goto exit;
		}
	}

	/* error handle, can't do free skb at this point, it will be released at hci_send_frame when failed */

	if (!atomic_read(&cif_dev->sdio_thread.thread_status)) {
		BTMTK_WARN("%s main thread already stopped, don't send cmd anymore!!", __func__);
		ret = -1;
		goto exit;
	}


	if (skb->data[0] == HCI_ACLDATA_PKT && skb->data[1] == 0x00 && skb->data[2] == 0x44) {
		/* it's for ble iso, remove speicific header
		 * 02 00 44 len len + payload to 05 + payload
		 */
		skb_pull(skb, 4);
		skb->data[0] = HCI_ISODATA_PKT;
	}

	if ((skb->len == FW_ASSERT_CMD_LEN &&
		!memcmp(skb->data, fw_assert_cmd, FW_ASSERT_CMD_LEN))
		|| (skb->len == FW_ASSERT_CMD1_LEN &&
		!memcmp(skb->data, fw_assert_cmd1, FW_ASSERT_CMD1_LEN))) {
		BTMTK_INFO_RAW(skb->data, skb->len, "%s: Trigger FW assert, dump CR", __func__);
		//btmtk_sdio_keep_driver_own(cif_dev, 1);
		//btmtk_sdio_print_debug_sr(cif_dev);
		//btmtk_sdio_dump_debug_sop(bdev);
	}
	if (skb){
		//BTMTK_INFO("%s skb add to tx_queue", __func__);
	}
	
	//spin_lock_irqsave(&bdev->txlock, flags);
	//spinLockAcquire(&bdev->txlock);
	RtbQueueTail(cif_dev->tx_queue, skb);
	//spinLockRelease(&bdev->txlock);
	//spin_unlock_irqrestore(&bdev->txlock, flags);
	xEventGroupSetBits(cif_dev->sdio_thread.xEventGroup, BTMTK_SDIO_THREAD_TX);
	//wake_up_interruptible(&cif_dev->sdio_thread.wait_q);

exit:
	return ret;
}


int btmtk_sdio_read_register(struct btmtk_dev *bdev, u32 reg, u32 *val)
{
	int ret = 0;
	u8 cmd[READ_REGISTER_CMD_LEN] = {0x01, 0x6F, 0xFC, 0x0C,
				0x01, 0x08, 0x08, 0x00,
				0x02, 0x01, 0x00, 0x01,
				0x00, 0x00, 0x00, 0x00};

	u8 event[READ_REGISTER_EVT_HDR_LEN] = {0x04, 0xE4, 0x10, 0x02,
			0x08, 0x0C, 0x00, 0x00,
			0x00, 0x00, 0x01};

	/* To-do using structure for sdio header
	 * struct btmtk_sdio_hdr *sdio_hdr;
	 * sdio_hdr = (void *) cmd;
	 * sdio_hdr->len = cpu_to_le16(skb->len);
	 * sdio_hdr->reserved = cpu_to_le16(0);
	 * sdio_hdr->bt_type = hci_skb_pkt_type(skb);
	 */

	BTMTK_INFO("%s: read cr %x", __func__, reg);

	memcpy(&cmd[MCU_ADDRESS_OFFSET_CMD], &reg, sizeof(reg));

	ret = btmtk_main_send_cmd(bdev, cmd, READ_REGISTER_CMD_LEN, event, READ_REGISTER_EVT_HDR_LEN, DELAY_TIMES,
			RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	memcpy(val, bdev->io_buf + MCU_ADDRESS_OFFSET_EVT - HCI_TYPE_SIZE, sizeof(u32));
	*val = le32_to_cpu(*val);
	/* make sure memcpy done before we use the register */
	// smp_wmb();

	BTMTK_INFO("%s: reg=%x, value=0x%08x", __func__, reg, *val);

	return ret;
}

static int btmtk_sdio_write_register(struct btmtk_dev *bdev, u32 reg, u32 val)
{
	BTMTK_INFO("%s: reg=%x, value=0x%08x, not support", __func__, reg, val);
	return 0;
}

int btmtk_sdio_send_and_recv(struct btmtk_dev *bdev,
		sk_buff *skb,
		const uint8_t *event, const int event_len,
		int delay, int retry, int pkt_type, bool flag)
{
	unsigned long start_time = 0;
	int ret = -1;
	struct btmtk_sdio_dev *cif_dev = NULL;
	int state = BTMTK_STATE_INIT;

	//BTMTK_INFO("%s", __func__);

	cif_dev = (struct btmtk_sdio_dev *)bdev->cif_dev;

	if (event) {
		if (event_len > EVENT_COMPARE_SIZE) {
			BTMTK_ERR("%s, event_len (%d) > EVENT_COMPARE_SIZE(%d), error",
				__func__, event_len, EVENT_COMPARE_SIZE);
			ret = -1;
			goto exit;
		}
		event_compare_status = BTMTK_EVENT_COMPARE_STATE_NEED_COMPARE;
		memcpy(event_need_compare, event + 1, event_len - 1);
		event_need_compare_len = event_len - 1;

		//start_time = jiffies;
		/* check hci event /wmt event for SDIO/UART interface, check hci
		 * event for USB interface
		 */
		/*
		BTMTK_INFO("event_need_compare_len %d, event_compare_status %d",
			event_need_compare_len, event_compare_status);
		*/
	} else {
		BTMTK_INFO("%s no need compare, set success", __func__);
		event_compare_status = BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS;
	}

	//BTMTK_INFO_RAW(skb->data, skb->len, "%s, send, len = %d", __func__, skb->len);

	ret = btmtk_sdio_send_cmd(bdev, skb, delay, retry, pkt_type, flag);
	if (ret < 0) {
		BTMTK_ERR("%s btmtk_sdio_send_cmd failed!!", __func__);
		goto exit;
	}
	
	/* check if event_compare_success */
	if (event_compare_status == BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS) {
		/* make sure memcpy done before we compare it */
		// smp_rmb();
		ret = 0;
	} else {
		//BTMTK_INFO("%s: begin wait event interruptible", __func__);
		/*
		wait_event_interruptible_timeout(bdev->compare_event_wq, (event_compare_status == BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS),
			msecs_to_jiffies(WOBLE_COMP_EVENT_TIMO));
		*/
#if 1
		state = btmtk_get_chip_state(bdev);
		if (state == BTMTK_STATE_WORKING) {
			BTMTK_INFO("%s: begin wait event interruptible", __func__);
			u32 u32ReadCRValue = 0;
			ret = btmtk_sdio_readl(CHISR, &u32ReadCRValue, cif_dev->func);
			BTMTK_INFO("%s u32ReadCRValue = 0x%08x", __func__, u32ReadCRValue);

			ret = btmtk_sdio_readl(CHLPCR, &u32ReadCRValue, cif_dev->func);
			BTMTK_INFO("%s u32ReadCRValue = 0x%08x", __func__, u32ReadCRValue);
			
			//vTaskDelay(6000 / portTICK_RATE_MS);
			
			EventBits_t uxBitsToWaitFor = 0x01;
			EventBits_t uxBits;
			while (1)
			{
				BTMTK_INFO("%s start wait for compare_event", __func__);
				uxBits = xEventGroupWaitBits(bdev->compare_event, 
							uxBitsToWaitFor, pdTRUE, pdFALSE, pdMS_TO_TICKS(WOBLE_COMP_EVENT_TIMO)); //5s超时pdMS_TO_TICKS(WOBLE_COMP_EVENT_TIMO)
				BTMTK_INFO("%s match compare_event condition , uxBits=%d", __func__, uxBits);
				if ((uxBits & uxBitsToWaitFor) == uxBitsToWaitFor) {
					BTMTK_INFO("%s wait success ", __func__);
					break;
				} 
				if (uxBits == 0){
					BTMTK_INFO("%s compare_event timeout(%d)ms", __func__, WOBLE_COMP_EVENT_TIMO);
					break;
				}
				
				vTaskDelay(1000);
			}
			
			ret = btmtk_sdio_readl(CHISR, &u32ReadCRValue, cif_dev->func);
			BTMTK_INFO("%s u32ReadCRValue = 0x%08x", __func__, u32ReadCRValue);

			ret = btmtk_sdio_readl(CHLPCR, &u32ReadCRValue, cif_dev->func);
			BTMTK_INFO("%s u32ReadCRValue = 0x%08x", __func__, u32ReadCRValue);
		} else {
			//BTMTK_INFO("%s: polling event", __func__);
			u32 u32ReadCRValue = 0;
			u32ReadCRValue = 0;
			while (!((TX_EMPTY & u32ReadCRValue) && (RX_DONE & u32ReadCRValue))){
				ret = btmtk_sdio_readl(CHISR, &u32ReadCRValue, cif_dev->func);
				// BTMTK_INFO("%s u32ReadCRValue = 0x%08x", __func__, u32ReadCRValue);
				if ((TX_EMPTY & u32ReadCRValue) && (RX_DONE & u32ReadCRValue)) {
					btmtk_sdio_interrupt_process(bdev);
				}
				vTaskDelay(10 / portTICK_RATE_MS);
			}
		}
#endif
		if (event_compare_status == BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS) {
			/* make sure memcpy done before we compare it */
			// smp_rmb();
			ret = 0;
		}
	}

	/* error handle*/
	/*
	if (!atomic_read(&cif_dev->sdio_thread.thread_status)) {
		BTMTK_WARN("%s main thread already stopped, don't wait evt anymore!!", __func__);
		ret = -ERRNUM;
		goto exit;
	}
	*/

	if (event_compare_status == BTMTK_EVENT_COMPARE_STATE_NEED_COMPARE){ //&&
		// atomic_read(&cif_dev->sdio_thread.thread_status)) {
		BTMTK_ERR("%s wait expect event timeout!!", __func__);
		ret = -ERRNUM;
		goto fw_assert;
	}

	event_compare_status = BTMTK_EVENT_COMPARE_STATE_NOTHING_NEED_COMPARE;
	goto exit;
fw_assert:
	// btmtk_send_assert_cmd(bdev);
exit:
	return ret;
}

#define DROP_BY_DRIVER 1
#define SEND_TO_STACK 0

int btmtk_sdio_event_filter(struct btmtk_dev *bdev, sk_buff *skb)
{
	const u8 read_address_event[READ_ADDRESS_EVT_HDR_LEN] = { 0x4, 0x0E, 0x0A, 0x01, 0x09, 0x10, 0x00 };
	u8 *p = NULL, *pend = NULL;
	u8 attr_len, attr_type;
	int state = btmtk_get_chip_state(bdev);
		
	//BTMTK_INFO("%s", __func__);

	if (event_compare_status == BTMTK_EVENT_COMPARE_STATE_NEED_COMPARE &&
		skb->len >= event_need_compare_len) {
		if (memcmp(skb->data, &read_address_event[1], READ_ADDRESS_EVT_HDR_LEN - 1) == 0
			&& (skb->len == (READ_ADDRESS_EVT_HDR_LEN - 1 + BD_ADDRESS_SIZE))) {
			memcpy(bdev->bdaddr, &skb->data[READ_ADDRESS_EVT_PAYLOAD_OFFSET - 1], BD_ADDRESS_SIZE);
			//BTMTK_INFO("%s: GET BDADDR = "MACSTR, __func__, MAC2STR(bdev->bdaddr));
			BTMTK_INFO("%s: GET BDADDR = "MACSTR, __func__, MAC2STR(bdev->bdaddr));

			event_compare_status = BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS;
			if (state == BTMTK_STATE_WORKING) {
				xEventGroupSetBits(bdev->compare_event, 0x01);
			}
		} else if (memcmp(skb->data, event_need_compare,
					event_need_compare_len) == 0) {

			/* If driver need to check result from skb, it can get from io_buf */
			/* Such as chip_id, fw_version, etc. */
			// bdev->io_buf[0] = bt_cb(skb)->pkt_type;
			// BTMTK_INFO("%s: bdev->io_buf[0] = skb_get_pkt_type(skb);", __func__);
			bdev->io_buf[0] = skb_get_pkt_type(skb);
			// BTMTK_INFO("%s: memcpy(&bdev->io_buf[1], skb->data, skb->len);", __func__);
			memcpy(&bdev->io_buf[1], skb->data, skb->len);
			/* make sure memcpy done before we compare it */
			//smp_wmb();
			
			event_compare_status = BTMTK_EVENT_COMPARE_STATE_COMPARE_SUCCESS;
			if (state == BTMTK_STATE_WORKING) {
				xEventGroupSetBits(bdev->compare_event, 0x01);
			}

			//BTMTK_INFO("%s, compare success", __func__);
			return DROP_BY_DRIVER;
		} else {
			if (skb->data[0] != BLE_EVT_TYPE) {
				/* Don't care BLE event */
				BTMTK_INFO("%s compare fail", __func__);
				BTMTK_INFO("%s event_need_compare_len = %d, skb->len = %d", __func__, event_need_compare_len, skb->len);
				BTMTK_INFO_RAW(event_need_compare, event_need_compare_len,
					"%s: event_need_compare:", __func__);
				BTMTK_INFO_RAW(skb->data, skb->len, "%s: skb->data:", __func__);
			}

			if (btmtk_vendor_cmd_filter(bdev, skb)) {
				return DROP_BY_DRIVER;
			}

			return SEND_TO_STACK;
		}
	} else {
		if (btmtk_vendor_cmd_filter(bdev, skb)) {
			return DROP_BY_DRIVER;
		} else {
			return SEND_TO_STACK;
		}
	}
}

static int btmtk_sdio_open(struct btmtk_dev *bdev)
{
	int ret = -1;
	BTMTK_INFO("%s", __func__);
	struct btmtk_sdio_dev *cif_dev = (struct btmtk_sdio_dev *)bdev->cif_dev;

	ret = btmtk_init_sdio_own_mutex();
	if (ret < 0){
		BTMTK_INFO("%s:init_sdio_own_mutex fail!", __func__);
		return -1;
	}

	//clear tx_queue 
	RtbEmptyQueue(cif_dev->tx_queue);

	BTMTK_INFO("%s:Done!", __func__);
	return 0;
}

static void btmtk_sdio_open_done(struct btmtk_dev *bdev)
{
	BTMTK_INFO("%s enter!", __func__);
}

int btmtk_cif_deregister(void)
{
	BTMTK_INFO("%s", __func__);
	return 0;
}

int btmtk_cif_register(void)
{
	int retval = 0;
	struct hif_hook_ptr hook;

	BTMTK_INFO("%s", __func__);

	memset(&hook, 0, sizeof(hook));
	hook.open = btmtk_sdio_open;
	hook.open_done = btmtk_sdio_open_done;
	hook.send_cmd = btmtk_sdio_send_cmd;
	hook.reg_read = btmtk_sdio_read_register;
	hook.reg_write = btmtk_sdio_write_register;
    hook.send_and_recv = btmtk_sdio_send_and_recv;
	hook.event_filter = btmtk_sdio_event_filter;
	btmtk_reg_hif_hook(&hook);

	return retval;
}

static int btmtk_cif_probe(struct sdio_func *func,
					const struct sdio_device_id *id)
{
	int ret = -1;
	int cif_event = 0;
	struct btmtk_cif_state *cif_state;
	struct btmtk_dev *bdev;
	
	u32 u32ReadCRValue = 0;
	
	/* Mediatek Driver Version */
	BTMTK_INFO("%s: 1. MTK BT Driver Version: %s", __func__, VERSION);
	BTMTK_INFO("vendor=0x%x, device=0x%x, class=%d, fn=%d",
	           id->vendor, id->device, id->class, func->num);
	// DUMP_TIME_STAMP("probe_start");
	
	/* sdio interface numbers  */
	if (func->num != BTMTK_SDIO_FUNC) {
		BTMTK_ERR("%s: func num is not match, func_num = %d", __func__, func->num);
		return -ENODEV;
	}

	/* Retrieve priv data and set to interface structure */
	bdev = btmtk_get_dev();
	if (!bdev) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -ENODEV;
	}
	BTMTK_INFO("%s: 2. after btmtk_get_dev", __func__);

    // ITE_MT7921_SDIO_BT_WIP
	//bdev->intf_dev = &func->dev;
	bdev->cif_dev = &g_sdio_dev;
	sdio_set_drvdata(func, bdev);

	/* Retrieve current HIF event state */
	cif_event = HIF_EVENT_PROBE;
	if (BTMTK_CIF_IS_NULL(bdev, cif_event)) {
		/* Error */
		BTMTK_ERR("%s priv setting is NULL", __func__);
		return -ENODEV;
	}

    // ITE_MT7921_SDIO_BT_WIP
	// btmtk_sdio_wake_lock(g_sdio_dev.device_ws, 0);
	
	cif_state = &bdev->cif_state[cif_event];

	/* Set Entering state */
	btmtk_set_chip_state((void *)bdev, cif_state->ops_enter);

	/* Do HIF events */
	ret = btmtk_sdio_probe(func, id);
	BTMTK_INFO("%s: 3. after btmtk_sdio_probe", __func__);

	/* Set End/Error state */
	if (ret == 0)
		btmtk_set_chip_state((void *)bdev, cif_state->ops_end);
	else
		btmtk_set_chip_state((void *)bdev, cif_state->ops_error);

    // ITE_MT7921_SDIO_BT_WIP
	// btmtk_sdio_wake_unlock(g_sdio_dev.device_ws);

	BTMTK_INFO("%s 4. end", __func__);
	DUMP_TIME_STAMP("probe_end");

    return ret;
}

const struct sdio_device_id btmtk_sdio_tabls[] = {
	/* Mediatek MT7961 Bluetooth device */
	{ SDIO_DEVICE(SDIO_VENDOR_ID_MEDIATEK, 0x7961)},
	{ }	/* Terminating entry */
};

struct sdio_driver btmtk_sdio_driver = {
	.name = "btmtksdio",
	.id_table = btmtk_sdio_tabls,
	.probe = btmtk_cif_probe,
	.remove = btmtk_cif_disconnect,
};

int mmpMtkBtDriverRegister(void)
{
	int ret;
	
    printf("====>Do mmpMtkBtDriverRegister....\n");
	ret = btmtk_main_init(); // turnkey： main_driver_init  TODO:btmtk_main_deinit
	if (ret < 0)
	{
		printf("btmtk_main_init fail !!! \n");
	}
	ret = sdio_register_driver(&btmtk_sdio_driver);

	return ret;
}
