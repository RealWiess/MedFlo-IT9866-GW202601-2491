/*
 * Copyright (c) 2015 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * HAL DMA functions.
 *
 * @author Irene Lin
 * @version 1.0
 */
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <malloc.h>
#include <unistd.h>
#include "ith_cfg.h"
#include "ite/itp.h"

static void* dmaMutex;
ITHDma dmaCtxt[ITH_DMA_CH_NUM];
ITHDmaWidth dmaWidthMap[4] = { ITH_DMA_WIDTH_32, ITH_DMA_WIDTH_8, ITH_DMA_WIDTH_16, ITH_DMA_WIDTH_8 };


#if defined(DMA_IRQ_ENABLE)

static void DmaIntrHandler(void* data)
{
    ITHDma* ctxt = (ITHDma*)data;
    uint32_t tc, err_abt, intrStatus;
    uint32_t ch;
    //uint32_t need_flush=0;

    tc = ithReadRegA(ITH_DMA_BASE + ITH_DMA_INT_TC_REG);
    err_abt = ithReadRegA(ITH_DMA_BASE + ITH_DMA_INT_ERRABT_REG);
    ithWriteRegA(ITH_DMA_BASE + ITH_DMA_INT_TC_CLR_REG, tc);
    ithWriteRegA(ITH_DMA_BASE + ITH_DMA_INT_ERRABT_CLR_REG, err_abt);

    for (ch = 0; ch<ITH_DMA_CH_NUM; ch++)
    {
        intrStatus = 0;
        if ((tc >> ch) & 0x1)
            intrStatus |= ITH_DMA_INTS_TC;

        if (err_abt & ITH_DMA_EA_ERR_CH(ch))
            intrStatus |= ITH_DMA_INTS_ERR;

        if (err_abt & ITH_DMA_EA_WDT_CH(ch))
            intrStatus |= ITH_DMA_INTS_WDT;

        if (err_abt & ITH_DMA_EA_ABT_CH(ch))
            intrStatus |= ITH_DMA_INTS_ABT;

        if (ctxt[ch].isr && intrStatus)
            ctxt[ch].isr(ch, ctxt[ch].data, intrStatus);

        //if (dmaCtxt[ch].dstHw == ITH_DMA_NORMAL_MODE)
        //    need_flush = 1;
    }

    //if(need_flush)
    //    ithFlushAhbWrap();

    if(err_abt)
        (void)ithPrintf(" dma error 0x%08X \n\n", err_abt);

    return;
}

#endif


void ithDmaInit(void* mutex)
{
    uint32_t tmp = ithReadRegA(ITH_DMA_BASE + ITH_DMA_FEATURE_REG);

#if 1
    {
        const uint8_t cq_depth[4] = { 2, 4, 8, 16 };
        const uint8_t ldm_depth[4] = { 32, 64, 128, 0 };
        const uint8_t fifo_depth[8] = { 8, 16, 32, 64, 128 };
        const uint8_t bus_width[4] = { 32, 64, 128 };
        ITH_LOG_INFO("DMA Hardware Feature: \n");
        ITH_LOG_INFO(" command queue depth: %d \n", cq_depth[(tmp >> 28) & 0x3]);
        ITH_LOG_INFO(" LDM depth: %d \n", ldm_depth[(tmp >> 24) & 0x3]);
        ITH_LOG_INFO(" LDM: %s \n", ((tmp >> 20) & 0x1) ? "ON" : "OFF");
        ITH_LOG_INFO(" Number of the peripheral interfaces: %d \n", (((tmp >> 16) & 0xF) + 1));
        ITH_LOG_INFO(" Peripheral interface exists: %s \n", (((tmp >> 12) & 0x1) ? "Yes" : "No"));
        ITH_LOG_INFO(" data FIFO depth: %d \n", (fifo_depth[(tmp >> 8) & 0x7]) );
        ITH_LOG_INFO(" AXI data bus width of the AXI slave port: %d \n", (bus_width[(tmp >> 6) & 0x3]));
        ITH_LOG_INFO(" AXI data bus width of the AXI master port: %d \n", (bus_width[(tmp >> 4) & 0x3]));
        ITH_LOG_INFO(" Unalign transfer mode: %s \n", (((tmp >> 3) & 0x1) ? "ON" : "OFF"));
        ITH_LOG_INFO(" Channel Number: %d \n", ((tmp & 0x7) + 1));
    }
#endif

    ASSERT(mutex);
    dmaMutex = mutex;

    #if defined(DMA_IRQ_ENABLE)
    /** clear interrupt status */
    tmp = ithReadRegA(ITH_DMA_BASE + ITH_DMA_INT_TC_REG);
    ithWriteRegA(ITH_DMA_BASE + ITH_DMA_INT_TC_CLR_REG, tmp);
    tmp = ithReadRegA(ITH_DMA_BASE + ITH_DMA_INT_ERRABT_REG);
    ithWriteRegA(ITH_DMA_BASE + ITH_DMA_INT_ERRABT_CLR_REG, tmp);

    /** register interrupt handler to interrupt mgr */
    ithIntrRegisterHandlerIrq(ITH_INTR_DMA, DmaIntrHandler, dmaCtxt);
    ithIntrSetTriggerModeIrq(ITH_INTR_DMA, ITH_INTR_LEVEL);
    ithIntrSetTriggerLevelIrq(ITH_INTR_DMA, ITH_INTR_HIGH_RISING);
    ithIntrEnableIrq(ITH_INTR_DMA);
    #endif

    /** open channel synchronization logic */
    ithWriteRegA(ITH_DMA_BASE + ITH_DMA_SYNC_REG, 0xFFFF);

    /* APB slave error response enable */
    ithWriteRegA(ITH_DMA_BASE + ITH_DMA_PSE_REG, 0x1);

    /* Clear hardware handshaking source select register */
    ithWriteRegA(ITH_DMA_BASE + 0x60, 0x0);
    ithWriteRegA(ITH_DMA_BASE + 0x64, 0x0);
    ithWriteRegA(ITH_DMA_BASE + 0x68, 0x0);
    ithWriteRegA(ITH_DMA_BASE + 0x6C, 0x0);
}

int ithDmaRequestCh(char* name, ITHDmaPriority priority, ITHDmaIntrHandler isr, void* arg)
{
    int i, found = 0;
    ASSERT(name);

    ithLockMutex(dmaMutex);

    for(i=0; i<ITH_DMA_CH_NUM; i++)
    {
        if(!dmaCtxt[i].name)
        {
            found = 1;
            break;
        }
    }

    if(found)
    {
        ithWriteRegA((ITH_DMA_BASE + ITH_DMA_CFG_CH(i)), ITH_DMA_INT_MASK);  // mask interrupt
        dmaCtxt[i].name = name;
        dmaCtxt[i].isr = isr;
        dmaCtxt[i].data = arg;
        dmaCtxt[i].priority = priority;
    }
    else
    {
        ITH_LOG_ERR(" request dma channel fail! name:%s \r\n", name );
        i = -1;
    }

    ithUnlockMutex(dmaMutex);

    return i;
}

void ithDmaFreeCh(int ch)
{
    ASSERT((ch>=0) && (ch<ITH_DMA_CH_NUM));

    if(!dmaCtxt[ch].name)
    {
        ITH_LOG_ERR(" trying to free channel %d which is already freed. \r\n", ch );
        return;
    }

    if (!ithCpuInInterrupt())
        ithLockMutex(dmaMutex);

    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_CFG_CH(ch)), ITH_DMA_INT_MASK);  // mask interrupt
    dmaCtxt[ch].name = NULL;
    dmaCtxt[ch].isr  = NULL;
    dmaCtxt[ch].data = NULL;

    if (!ithCpuInInterrupt())
        ithUnlockMutex(dmaMutex);
}

void ithDmaReset(int ch)
{
    ASSERT((ch>=0) && (ch<ITH_DMA_CH_NUM));
	ithWriteRegA((ITH_DMA_BASE + ITH_DMA_CTRL_CH(ch)), 0x0);
    (void)memset((void*)&dmaCtxt[ch].burst, 0x0, sizeof(ITHDma)-16);
}

void ithDmaStart(int ch)
{
    /* 8-bits, 16-bits, 32-bits, 64-bits(unalign mode) */
    static const uint32_t dmaSizeDiv[4] = { 1, 2, 4, 1 };
    uint32_t value, ctrl = ITH_DMA_CTRL_ENABLE | ITH_DMA_CTRL_WSYNC;
    ITHDma *ctxt = &dmaCtxt[ch];

#if 1 // for write-back
    if (ctxt->srcHw == ITH_DMA_NORMAL_MODE)
        ithFlushDCacheRange((void*)ctxt->srcAddr, ctxt->txSize);
    if (ctxt->dstHw == ITH_DMA_NORMAL_MODE)
        ithFlushDCacheRange((void*)ctxt->dstAddr, ctxt->txSize);
#endif

    if ((ctxt->srcHw == ITH_DMA_NORMAL_MODE) || (ctxt->dstHw == ITH_DMA_NORMAL_MODE))
        ithFlushMemBuffer();

    // transfer size with src width unit
    value = ctxt->txSize / dmaSizeDiv[ctxt->srcWidth];
    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_SIZE_CH(ch)), value);

    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_LLP_CH(ch)), ctxt->lldAddr);

    // csr
    ctrl |= (ITH_DMA_CTRL_SrcTcnt(ctxt->burst) |
        ITH_DMA_CTRL_SRC_WIDTH(ctxt->srcWidth) |
        ITH_DMA_CTRL_DST_WIDTH(ctxt->dstWidth) |
        ITH_DMA_CTRL_SRC_BURST_CTRL(ctxt->srcCtrl) |
        ITH_DMA_CTRL_DST_BURST_CTRL(ctxt->dstCtrl));

    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_CTRL_CH(ch)), ctrl);
}

void ithDmaDumpReg(int ch)
{
    uint32_t value;
    (void)printf(" dma: %s \n", dmaCtxt[ch].name);
    value = ithReadRegA(ITH_DMA_BASE + ITH_DMA_CTRL_CH(ch));
    (void)printf(" 0x%08" PRIx32 " = 0x%08" PRIx32 "\n", (ITH_DMA_BASE + ITH_DMA_CTRL_CH(ch)), value);
    value = ithReadRegA(ITH_DMA_BASE + ITH_DMA_CFG_CH(ch));
    (void)printf(" 0x%08" PRIx32 " = 0x%08" PRIx32 "\n", (ITH_DMA_BASE + ITH_DMA_CFG_CH(ch)), value);
    value = ithReadRegA(ITH_DMA_BASE + ITH_DMA_SRC_CH(ch));
    (void)printf(" 0x%08" PRIx32 " = 0x%08" PRIx32 "\n", (ITH_DMA_BASE + ITH_DMA_SRC_CH(ch)), value);
    value = ithReadRegA(ITH_DMA_BASE + ITH_DMA_DST_CH(ch));
    (void)printf(" 0x%08" PRIx32 " = 0x%08" PRIx32 "\n", (ITH_DMA_BASE + ITH_DMA_DST_CH(ch)), value);
    value = ithReadRegA(ITH_DMA_BASE + ITH_DMA_SIZE_CH(ch));
    (void)printf(" 0x%08" PRIx32 " = 0x%08" PRIx32 "\n", (ITH_DMA_BASE + ITH_DMA_SIZE_CH(ch)), value);
    value = ithReadRegA(ITH_DMA_BASE + ITH_DMA_STRIDE_CH(ch));
    (void)printf(" 0x%08" PRIx32 " = 0x%08" PRIx32 "\n", (ITH_DMA_BASE + ITH_DMA_STRIDE_CH(ch)), value);
    value = ithReadRegA(ITH_DMA_BASE + ITH_DMA_LLP_CH(ch));
    (void)printf(" 0x%08" PRIx32 " = 0x%08" PRIx32 "\n", (ITH_DMA_BASE + ITH_DMA_LLP_CH(ch)), value);
}

#if !defined(WIN32)
//#define USE_NOTIFY
#endif

#define DMA_MEM_TIMEOUT     10000

static void m2m_isr(int ch, void* arg, uint32_t int_status)
{
    sem_t* sem = (sem_t*)arg;

    if (int_status & ITH_DMA_INTS_ERR)
        (void)ithPrintf(" dma ch%d error \n", ch);
    if (int_status & ITH_DMA_INTS_WDT)
        (void)ithPrintf(" dma ch%d watchdog timeout \n", ch);
    if (int_status & ITH_DMA_INTS_ABT)
        (void)ithPrintf(" dma ch%d abort \n", ch);

    itpSemPostFromISR(sem);
}

#if !defined(WIN32) && defined(USE_NOTIFY)
static void m2m_notify_isr(int ch, void* arg, uint32_t int_status)
{
    pthread_t dma_thread = (pthread_t)arg;

    if (int_status & ITH_DMA_INTS_ERR)
        (void)ithPrintf(" dma ch%d error \n", ch);
    if (int_status & ITH_DMA_INTS_WDT)
        (void)ithPrintf(" dma ch%d watchdog timeout \n", ch);
    if (int_status & ITH_DMA_INTS_ABT)
        (void)ithPrintf(" dma ch%d abort \n", ch);

    itpTaskNotifyGiveFromISR(dma_thread, 0);
}
#endif

int ithDmaMemcpylld(uint32_t dst, uint32_t src, uint32_t len);

int ithDmaMemcpy(uint32_t dst, uint32_t src, uint32_t len)
{
    int ch;
#if defined(USE_NOTIFY)
    pthread_t dma_thread = pthread_self();
#else
    sem_t   sem;
#endif
    uint32_t ctrl, cfg;
#if defined(DMA_IRQ_ENABLE)
    int res;
#else
    uint32_t timeout_ms = DMA_MEM_TIMEOUT;
#endif

    if (len > ITH_DMA_CYC_MASK)
    {
        ITH_LOG_INFO("%s() size = 0x%X > 0x%X \n", __func__, len, ITH_DMA_CYC_MASK );
        return ithDmaMemcpylld(dst, src, len);
    }
#if defined(USE_NOTIFY)
    ch = ithDmaRequestCh("m2m", ITH_DMA_CH_PRIO_HIGH, m2m_notify_isr, (void*)dma_thread);
#else
    ch = ithDmaRequestCh("m2m", ITH_DMA_CH_PRIO_HIGH, m2m_isr, &sem);
#endif
    if (ch < 0)
    {
        ITH_LOG_ERR("%s(0x%X, 0x%X,%d) no available dma channel \n", __func__, src, dst, len );
        return -1;
    }
    ithDmaReset(ch);
#if !defined(USE_NOTIFY)
    sem_init(&sem, 0, 0);
#endif

    ctrl = ITH_DMA_CTRL_SRC_WIDTH(ITH_DMA_WIDTH_64) |
        ITH_DMA_CTRL_DST_WIDTH(ITH_DMA_WIDTH_64) |
        ITH_DMA_CTRL_SRC_INC |
        ITH_DMA_CTRL_DST_INC |
        ITH_DMA_CTRL_ENABLE;

    cfg = ITH_DMA_CFG_UNALIGN_MODE |
        ITH_DMA_CFG_HIGH_PRIO |
        ITH_DMA_CFG_GW(GRANT_WINDOW);

#if 1 // for write-back
    ithFlushDCacheRange((void*)src, len);
    ithFlushDCacheRange((void*)dst, len);
#endif
    ithFlushMemBuffer();

    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_SRC_CH(ch)), src);
    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_DST_CH(ch)), dst);
    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_SIZE_CH(ch)), len);
    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_CFG_CH(ch)), cfg);
    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_CTRL_CH(ch)), ctrl);

#if defined(DMA_IRQ_ENABLE)
    #if defined(USE_NOTIFY)
    res = itpTaskNotifyTake(true, DMA_MEM_TIMEOUT, 0);
    #else
    res = itpSemWaitTimeout(&sem, DMA_MEM_TIMEOUT);
    #endif
    if (res)
#else
    while (ithDmaIsBusy(ch) && --timeout_ms)
        (void)usleep(1000);

    if (!timeout_ms)
#endif
    {
        ITH_LOG_ERR(" dma timeout!\n");
        ithDmaDumpReg(ch);
        //while (1);
    }
    ithInvalidateDCacheRange((void*)dst, len);

    ithDmaFreeCh(ch);
#if !defined(USE_NOTIFY)
    sem_destroy(&sem);
#endif
    return 0;
}

struct dma_mem_async {
	unsigned int ch : 4;
	unsigned int active : 1;
    ITHDmaAsyncHandler cb;
    void* arg;
};

static void m2m2_isr(int ch, void* arg, uint32_t int_status)
{
    struct dma_mem_async *data = (struct dma_mem_async *)arg;

    if (int_status & ITH_DMA_INTS_ERR)
        (void)ithPrintf(" dma ch%d error \n", ch);
    if (int_status & ITH_DMA_INTS_WDT)
        (void)ithPrintf(" dma ch%d watchdog timeout \n", ch);
    if (int_status & ITH_DMA_INTS_ABT)
        (void)ithPrintf(" dma ch%d abort \n", ch);

    if (data->cb)
        data->cb(data->arg, int_status);

    (void)memset((void*)data, 0x0, sizeof(struct dma_mem_async));
    ithDmaFreeCh(ch);
}

static struct dma_mem_async async_data[ITH_DMA_CH_NUM];

int ithDmaMemcpyAsync(uint32_t dst, uint32_t src, uint32_t len, ITHDmaAsyncHandler cb, void* arg)
{
    int ch, i;
    uint32_t ctrl, cfg;
	struct dma_mem_async* pdata = NULL;

    if (len > ITH_DMA_CYC_MASK)
    {
        ITH_LOG_ERR("%s() size = 0x%X > 0x%X \n", __func__, len, ITH_DMA_CYC_MASK );
        return -1;
    }

	ithEnterCritical();
	for (i = 0; i < ITH_DMA_CH_NUM; i++) {
		if (async_data[i].active == 0) {
			pdata = &async_data[i];
			pdata->active = 1;
			break;
		}
	}
	ithExitCritical();
	if (pdata == NULL) {
		ITH_LOG_ERR("%s() can't find aysnc data struct \n", __func__ );
		return -2;
	}
	pdata->arg = arg;
	pdata->cb = cb;

	ch = ithDmaRequestCh("m2m2", ITH_DMA_CH_PRIO_HIGH, m2m2_isr, pdata);
    if (ch < 0)
    {
		(void)memset((void*)pdata, 0x0, sizeof(struct dma_mem_async));
        ITH_LOG_ERR("%s(0x%X, 0x%X,%d) no available dma channel \n", __func__, src, dst, len );
        return -1;
    }
    pdata->ch = ch;
    ithDmaReset(ch);

    ctrl = ITH_DMA_CTRL_SRC_WIDTH(ITH_DMA_WIDTH_64) |
        ITH_DMA_CTRL_DST_WIDTH(ITH_DMA_WIDTH_64) |
        ITH_DMA_CTRL_SRC_INC |
        ITH_DMA_CTRL_DST_INC |
        ITH_DMA_CTRL_ENABLE;

    cfg = ITH_DMA_CFG_UNALIGN_MODE |
        ITH_DMA_CFG_HIGH_PRIO |
        ITH_DMA_CFG_GW(GRANT_WINDOW);

#if 1 // for write-back
    ithFlushDCacheRange((void*)src, len);
    ithFlushDCacheRange((void*)dst, len);
#endif
    ithFlushMemBuffer();
    ithInvalidateDCacheRange((void*)dst, len);

    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_SRC_CH(ch)), src);
    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_DST_CH(ch)), dst);
    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_SIZE_CH(ch)), len);
    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_CFG_CH(ch)), cfg);
    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_CTRL_CH(ch)), ctrl);

    return 0;
}

int ithDmaMemcpylld(uint32_t dst, uint32_t src, uint32_t len)
{
    int ch;
    sem_t   sem;
    uint32_t ctrl, cfg, cycle, offset;
    ITHDmaDesc *first = NULL, *desc = NULL, *next = NULL;
    ITHDmaDesc *prev = NULL;
    int res = 0;
#if defined(DMA_IRQ_ENABLE)
#else
    uint32_t timeout_ms = DMA_MEM_TIMEOUT;
#endif

    ch = ithDmaRequestCh("m2mlld", ITH_DMA_CH_PRIO_HIGH, m2m_isr, &sem);
    if (ch < 0)
    {
        ITH_LOG_ERR("%s(0x%X, 0x%X,%d) no available dma channel \n", __func__, src, dst, len );
            return -1;
    }
    ithDmaReset(ch);
    sem_init(&sem, 0, 0);

    ctrl = ITH_DMA_CTRL_SRC_WIDTH(ITH_DMA_WIDTH_64) |
        ITH_DMA_CTRL_DST_WIDTH(ITH_DMA_WIDTH_64) |
        ITH_DMA_CTRL_SRC_INC |
        ITH_DMA_CTRL_DST_INC |
        ITH_DMA_CTRL_ENABLE;

    cfg = ITH_DMA_CFG_UNALIGN_MODE |
        ITH_DMA_CFG_HIGH_PRIO |
        ITH_DMA_CFG_GW(GRANT_WINDOW);

    for (offset = 0; offset < len; offset += cycle)
    {
        cycle = ITH_MIN((len-offset), MAX_CYCLE_PER_BLOCK);
        desc = (ITHDmaDesc *)malloc(sizeof(ITHDmaDesc));
        if (!desc) {
			res = -1;
            goto end;
        }
        (void)memset((void*)desc, 0x0, sizeof(ITHDmaDesc));
        ITH_LOG_DBG(" + %X\n", desc );

        desc->cfg = cfg;
        desc->len = len;

        if (first == NULL) /** first command */
        {
			desc->lld.src = src + offset;
			desc->lld.dst = dst + offset;
			desc->lld.cycle = cycle;
			desc->lld.ctrl = ctrl;

            first = desc;
        }
        else /** second and later command for LLD */
        {
			desc->lld.src = cpu_to_le32(src + offset);
			desc->lld.dst = cpu_to_le32(dst + offset);
			desc->lld.cycle = cpu_to_le32(cycle);
			desc->lld.ctrl = cpu_to_le32(ctrl);

            if (prev == first) {
                prev->lld.ctrl |= ITH_DMA_CTRL_MASK_TC;
                prev->lld.next = (uint32_t)desc;
            }
            else {
                prev->lld.ctrl = cpu_to_le32(le32_to_cpu(prev->lld.ctrl) | ITH_DMA_CTRL_MASK_TC);
                prev->lld.next = cpu_to_le32(desc);
            }
            #if 1 // for write-back
			ithFlushDCacheRange((void*)prev, sizeof(ITHDmaLLD));
            #endif
        }

        prev = desc;
    }
#if 1 // for write-back
    ithFlushDCacheRange((void*)prev, sizeof(ITHDmaLLD));
    ithFlushDCacheRange((void*)src, len);
    ithFlushDCacheRange((void*)dst, len);
#endif
    ithFlushMemBuffer();

    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_SRC_CH(ch)), first->lld.src);
    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_DST_CH(ch)), first->lld.dst);
    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_LLP_CH(ch)), first->lld.next);
    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_SIZE_CH(ch)), first->lld.cycle);
    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_CFG_CH(ch)), first->cfg);
    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_CTRL_CH(ch)), first->lld.ctrl);

#if defined(DMA_IRQ_ENABLE)
    res = itpSemWaitTimeout(&sem, DMA_MEM_TIMEOUT);
    if (res)
#else
    while (ithDmaIsBusy(ch) && --timeout_ms)
        (void)usleep(1000);

    if (!timeout_ms)
#endif
    {
        ITH_LOG_ERR(" dma timeout!\n");
        ithDmaDumpReg(ch);
        //while (1);
    }

    ithInvalidateDCacheRange((void*)dst, len);

end:
    desc = first;
	if (!desc)
        goto out;

    do {
		if (desc != first)
			next = (ITHDmaDesc *)le32_to_cpu(desc->lld.next);
		else
            next = (ITHDmaDesc *)desc->lld.next;
        ITH_LOG_DBG(" - %X\n", desc );
        free(desc);
        desc = next;
    } while (desc);

out:
    ithDmaFreeCh(ch);
    sem_destroy(&sem);

    return res;
}

int ithDmaMemset(uint32_t dst, uint32_t pattern, uint32_t len)
{
    int ch;
    sem_t   sem;
    uint32_t ctrl, cfg;
#if defined(DMA_IRQ_ENABLE)
    int res;
#else
    uint32_t timeout_ms = DMA_MEM_TIMEOUT;
#endif

    if (len > ITH_DMA_CYC_MASK)
    {
        ITH_LOG_INFO("%s() size = 0x%X > 0x%X \n", __func__, len, ITH_DMA_CYC_MASK );
        return -1;
    }
    if ((len | dst) & 0x3)
    {
        ITH_LOG_ERR("%s: dst 0x%X, len %d => need 4-bytes alignment \n", __func__, dst, len );
        return -1;
    }

    ch = ithDmaRequestCh("m2m", ITH_DMA_CH_PRIO_HIGH, m2m_isr, &sem);
    if (ch < 0)
    {
        ITH_LOG_ERR("%s(0x%X, 0x%X,%d) no available dma channel \n", __func__, dst, pattern, len );
            return -1;
    }
    ithDmaReset(ch);
    sem_init(&sem, 0, 0);

    ctrl = ITH_DMA_CTRL_SRC_WIDTH(ITH_DMA_WIDTH_32) |
        ITH_DMA_CTRL_DST_WIDTH(ITH_DMA_WIDTH_32) |
        ITH_DMA_CTRL_SRC_FIXED |
        ITH_DMA_CTRL_DST_INC |
        ITH_DMA_CTRL_ENABLE;

    cfg = ITH_DMA_CFG_WO_MODE |
        ITH_DMA_CFG_HIGH_PRIO |
        ITH_DMA_CFG_GW(GRANT_WINDOW);

#if 1 // for write-back
    ithFlushDCacheRange((void*)dst, len);
#endif
    ithFlushMemBuffer();

    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_WO_VALUE_REG), pattern);
    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_DST_CH(ch)), dst);
    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_SIZE_CH(ch)), (len/sizeof(uint32_t)));
    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_CFG_CH(ch)), cfg);
    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_CTRL_CH(ch)), ctrl);

#if defined(DMA_IRQ_ENABLE)
    res = itpSemWaitTimeout(&sem, DMA_MEM_TIMEOUT);
    if (res)
#else
    while (ithDmaIsBusy(ch) && --timeout_ms)
        (void)usleep(1000);

    if (!timeout_ms)
#endif
    {
        ITH_LOG_ERR(" dma timeout!\n");
            ithDmaDumpReg(ch);
        //while (1);
    }
    ithInvalidateDCacheRange((void*)dst, len);

    ithDmaFreeCh(ch);
    sem_destroy(&sem);

    return 0;

}

int ithDmaTx2D(uint32_t srcAddr, uint32_t dstAddr,
    uint16_t xCnt, uint16_t yCnt,
    int16_t srcStride, int16_t dstStride)
{
    int ch;
    sem_t   sem;
    uint32_t ctrl, cfg;
#if defined(DMA_IRQ_ENABLE)
    int res;
#else
    uint32_t timeout_ms = DMA_MEM_TIMEOUT;
#endif

    if ((srcStride | dstStride) & 0x7)
    {
        ITH_LOG_ERR("%s: srcStride %d, dstStride %d => need 64-bits alignment \n", __func__, srcStride, dstStride );
        return -1;
    }

    ch = ithDmaRequestCh("m2m", ITH_DMA_CH_PRIO_HIGH, m2m_isr, &sem);
    if (ch < 0)
    {
        ITH_LOG_ERR("%s() no available dma channel \n", __func__ );
        return -1;
    }
    ithDmaReset(ch);
    sem_init(&sem, 0, 0);

    ctrl = ITH_DMA_CTRL_SRC_WIDTH(ITH_DMA_WIDTH_64) |
        ITH_DMA_CTRL_DST_WIDTH(ITH_DMA_WIDTH_64) |
        ITH_DMA_CTRL_SRC_INC |
        ITH_DMA_CTRL_DST_INC |
        ITH_DMA_CTRL_EXP |
        ITH_DMA_CTRL_2D;

    cfg = ITH_DMA_CFG_UNALIGN_MODE |
        ITH_DMA_CFG_HIGH_PRIO |
        ITH_DMA_CFG_GW(GRANT_WINDOW);

#if 1 // for write-back
    ithFlushDCacheRange((void*)srcAddr, (srcStride*yCnt));
    ithFlushDCacheRange((void*)dstAddr, (dstStride*yCnt));
#endif
    ithFlushMemBuffer();

	ithWriteRegA((ITH_DMA_BASE + ITH_DMA_CTRL_CH(ch)), ctrl);  /* need enable 2d function first */
	ithWriteRegA((ITH_DMA_BASE + ITH_DMA_SRC_CH(ch)), srcAddr);
    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_DST_CH(ch)), dstAddr);
    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_SIZE_CH(ch)), (uint32_t)((yCnt << 16) | xCnt));
    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_STRIDE_CH(ch)), (uint32_t)((dstStride << 16) | srcStride));
    ithWriteRegA((ITH_DMA_BASE + ITH_DMA_CFG_CH(ch)), cfg);
	//ithDmaDumpReg(ch);
	ctrl |= ITH_DMA_CTRL_ENABLE;
	ithWriteRegA((ITH_DMA_BASE + ITH_DMA_CTRL_CH(ch)), ctrl);

#if defined(DMA_IRQ_ENABLE)
    res = itpSemWaitTimeout(&sem, DMA_MEM_TIMEOUT);
    if (res)
#else
    while (ithDmaIsBusy(ch) && --timeout_ms)
        (void)usleep(1000);

    if (!timeout_ms)
#endif
    {
        ITH_LOG_ERR(" dma timeout!\n");
        ithDmaDumpReg(ch);
        //while (1);
    }
    ithInvalidateDCacheRange((void*)dstAddr, (dstStride*yCnt));

    ithDmaFreeCh(ch);
    sem_destroy(&sem);

    return 0;
}


/***************************************************
 * 1.check if any dma channel busy?
 * 2.if dma is busy, then check if belong AXISPI
 * 3.wait axispi ready if it is busying.
 ***************************************************/
void ithDmaWaitAxiSpiIdle(void)
{
	uint32_t dma_base = ITH_DMA_BASE;
	uint32_t reg32 = 0;
	uint32_t i;
	uint8_t tmp8 = 0;
	uint8_t gotAxiApiCh = 0;
	uint8_t gotAxiApiCh2 = 0;
	uint8_t gotAxiApiBusy = 0;
	uint8_t gotDoubleBusy = 0;

	//(void)ithPrintf("chk dma idle...\n");

	for(i=0; i<8; i++)
	{
		reg32 = ithReadRegA(dma_base + 0x100 + (i * 0x20));
		//(void)ithPrintf("	reg[100+%x] = %x\n", i*0x20, reg32);
		if(reg32 & (1<<16))//check dma enable
		{
			uint32_t reg = 0xFF;
			//check if AXISPI??
			reg = ithReadRegA(dma_base + 0x104 + (i * 0x20));
			//(void)ithPrintf("	reg[104+%x] = %x\n", i*0x20, reg);
			if(reg & (1<<13))
			{
				uint8_t A1 = 0;
				uint8_t A2 = 0;
				uint32_t r1;
				uint8_t tmp;
				tmp8 = (reg >> 9) & 0xF;
				A1 = tmp8 / 4;
				A2 = tmp8 % 4;
				//(void)ithPrintf("	tmp8a=%x, a1=%x, a2=%x\n", tmp8, A1, A2);
				r1 = ithReadRegA(dma_base + 0x60 + (A1 * 0x4));
				tmp = (r1 >> (8*A2)) & 0xFF;
				//(void)ithPrintf("	rega[%x]=%x, tmp=%x\n", 0x60 + (A1 * 0x4), r1, tmp);
				if(tmp == 0xA)
				{
					//(void)ithPrintf("### GOT AXISPI DMA busyA! ###\n\n");
					if(gotAxiApiBusy)
					{
						//(void)ithPrintf("	### GOT busyB! ch1=%x, ch2=%x ###\n\n", gotAxiApiCh, i);
						gotDoubleBusy = 1;
						gotAxiApiCh2 = i;
					}
					gotAxiApiBusy = 1;
					gotAxiApiCh = i;
					//break;
				}
			}
			if(reg & (1<<7))
			{
				uint8_t A1 = 0;
				uint8_t A2 = 0;
				uint32_t r1;
				uint8_t tmp;

				tmp8 = (reg >> 3) & 0xF;
				A1 = tmp8 / 4;
				A2 = tmp8 % 4;
				//(void)ithPrintf("	tmp8b=%x, a1=%x, a2=%x\n", tmp8, A1, A2);

				r1 = ithReadRegA(dma_base + 0x60 + (A1 * 0x4));
				tmp = (r1 >> (8*A2)) & 0xFF;
				//(void)ithPrintf("	regb[%x]=%x, tmp=%x\n", 0x60 + (A1 * 0x4), r1, tmp);

				if(tmp == 0xA)
				{
					//(void)ithPrintf("### GOT AXISPI DMA busyB! ###\n\n");
					if(gotAxiApiBusy)
					{
						//(void)ithPrintf("	### GOT busyA! ch1=%x, ch2=%x ###\n\n",gotAxiApiCh,i);
						gotDoubleBusy = 1;
						gotAxiApiCh2 = i;
					}
					gotAxiApiBusy = 1;
					gotAxiApiCh = i;
					//break;
				}
			}
		}
	}
	if(gotAxiApiBusy)
	{
		uint32_t c = 0;
		reg32 = ithReadRegA(dma_base + 0x100 + (gotAxiApiCh * 0x20));
		while(reg32 & (1<<16))
		{
			reg32 = ithReadRegA(dma_base + 0x100 + (gotAxiApiCh * 0x20));
			c++;
			if(c>100*1000*1000)	break;
		}
		//(void)ithPrintf("	# GOT AXISPI ready! c: %d, %x=%x #\n\n", c, dma_base + 0x100 + (gotAxiApiCh * 0x20), reg32);
	}

	if(gotDoubleBusy)
	{
		uint32_t c = 0;
		reg32 = ithReadRegA(dma_base + 0x100 + (gotAxiApiCh2 * 0x20));
		while(reg32 & (1<<16))
		{
			reg32 = ithReadRegA(dma_base + 0x100 + (gotAxiApiCh2 * 0x20));
			c++;
			if(c>100*1000*1000)	break;
		}
		//(void)ithPrintf("	### got double busy Case! ###\n");
		//(void)ithPrintf("	# GOT AXISPI ready! c: %d, %x=%x #\n\n", c, dma_base + 0x100 + (gotAxiApiCh2 * 0x20), reg32);
	}
}
