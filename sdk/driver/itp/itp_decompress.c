/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL Decompress functions.
 *
 * @author Joseph
 * @version 1.0
 */
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <pthread.h>
#include "ite/ith.h"
#include "itp_cfg.h"

#if defined(__OPENRTOS__)
// #define DCPS_IRQ_ENABLE
#endif

#define ENABLE_DCPS_MUTEX

// #define ENABLE_DCPS_WB_CACHE
// #define ENABLE_MEASURE_TIME
// #define ENABLE_MEASURE_US_TIME
// #define ENABLE_DCPS_DBG_MSG

#define ITH_DCPS_BASE       (ITH_DPU_BASE + 0x100)
#define ITH_DCPS_REG_DAR    (ITH_DCPS_BASE + 0x0C)
#define ITH_DCPS_REG_DSR    (ITH_DCPS_BASE + 0x1C)
#define ITH_DCPS_REG_DDCR   (ITH_DCPS_BASE + 0x20)
/*******************************************************************
 "ENABLE_PATCH_SEAL_WAIT_FIFO_EMPTY_ISSUE" is workarounding the DCPS issue.
 DCPS module has a timming issue, but it's hard to ECO to fix it.
 So driver will refire 3 times if SEAL got this issue.
 and it's will be fixed in IT9860
 *******************************************************************/
#if (CFG_CHIP_FAMILY != 9860)
    #define ENABLE_PATCH_SEAL_WAIT_FIFO_EMPTY_ISSUE
#endif

#ifdef ENABLE_PATCH_SEAL_WAIT_FIFO_EMPTY_ISSUE
// #define ENABLE_AUTO_GEN_CASE_FOR_VERIFY_RETRY_FUNC
#endif

#ifdef ENABLE_AUTO_GEN_CASE_FOR_VERIFY_RETRY_FUNC
#include <stdlib.h>		//for rand()
#endif

#ifdef ENABLE_MEASURE_US_TIME
    #define DCPS_USE_TIMER (3)
#endif
/*******************************************************************
 "ENABLE_DYNAMIC_SWITCH_DCPS_CLOCK" is for power comsumption.
 *******************************************************************/
#define ENABLE_DYNAMIC_SWITCH_DCPS_CLOCK

/*******************************************************************
 "ENABLE_PATCH_DCPS_DMA_ISSUE" is for fixing the DCPS H/W issue.
 DPU can not work well with dma function (like: SPI...)
 *******************************************************************/
#if (CFG_CHIP_FAMILY == 9070)
    #define ENABLE_PATCH_DCPS_DMA_ISSUE
#endif

/*************************************************************************
 "ENABLE_PATCH_DST_BASE_ALIGNMENT_ISSUE" is for workaround the DCPS H/W issue.
 DCPS will fail if dstination length is not 4 byte alignenmt.
 But it will workaround if dstination buffer address was shift (4-n) bytes.
 ( n = dstination_length % 4 )
 *************************************************************************/
// remove it if IT9860 or IT970
// #define ENABLE_PATCH_DST_BASE_ALIGNMENT_ISSUE

#define DCPS_WB_ALIGN_SIZE (32)
/* ************************************************************ */
/* GLOBAL VARIABLES                                             */
/* ************************************************************ */
// DCPS MODE DEFINATION::
// g_DcpsInfo.DcpsMode = 0  -->SEAL DECOMPRESS MODE
// g_DcpsInfo.DcpsMode = 1  -->SPEEDY DECOMPRESS MODE
// g_DcpsInfo.DcpsMode = 2  -->SPEEDY COMPRESION (1/2/4)-BYTE MODE
/* ************************************************************ */
/* GLOBAL VARIABLES                                             */
/* ************************************************************ */

static ITH_DCPS_INFO    g_DcpsInfo;
static unsigned int     g_DcpsSetIndex  = 0;

static unsigned int     g_DcpsMode      = 0;

#if defined(DCPS_IRQ_ENABLE)
static sem_t        * DcpsIsrSemaphor   = NULL;
static unsigned int g_IsrCounter        = 0;
#endif

#ifdef  ENABLE_DCPS_WB_CACHE
static unsigned char * gDcpsSrcBase = NULL;
#endif

#ifdef ENABLE_MEASURE_TIME
static struct timeval startT, endT;
#endif

#ifdef ENABLE_MEASURE_US_TIME
static uint32_t gT0 = 0, gT1 = 0;
static uint32_t minDur              = 0xFFFFFFFF;
static uint32_t totalDur            = 0;
static uint32_t avgDur              = 0;
static uint32_t mCnt                = 0;
static uint32_t gTimerAlignValue    = 0;
#endif

#ifdef  ENABLE_DCPS_MUTEX
static pthread_mutex_t  dcpsMutex = PTHREAD_MUTEX_INITIALIZER;
#endif

static uint32_t g_DoWaitCmdq    = 0;

unsigned char   gDcpsEnClk      = 0;

#ifdef  CFG_DPU_ENABLE
extern unsigned char    gDpuEnClk;
#else
unsigned char           gDpuEnClk = 0;
#endif
/* ************************************************************ */
/* function implementation                                      */
/* ************************************************************ */
#if defined(DCPS_IRQ_ENABLE)

static void
dcps_isr (
    void * data)
{
    unsigned int    tmp     = 0;
    unsigned int    QueCnt  = 0;

    // ithPrintf("$in\n");
    g_IsrCounter++;

    ithDcpsGetStatus(&g_DcpsInfo);
    tmp = g_DcpsInfo.RegDcpsStatus;

    if (tmp & 0x04)
    {
        // decompress fail
        ithPrintf("decompress fail!!\n");
        // g_DecompressStatus |= DECOMPRESS_HW_FAIL;   //DECOMPRESS_FAIL  0x0004
    }

    ithDcpsClearIntr();

    itpSemPostFromISR(DcpsIsrSemaphor);

    // ithPrintf("$[%d,%d]\n",g_CmdQueDoneIndex,g_CmdQueIndex);
    // ithPrintf("$out\n");
}

static void
DcpsEnableIntr (
    void)
{
    // Initialize DCPS IRQ
    // (void)printf("Enable DCPS IRQ~~\n");

    // register DCPS Handler to IRQ
    ithIntrRegisterHandlerIrq(ITH_INTR_DECOMPRESS, dcps_isr, NULL);

    // set IRQ to edge trigger
    ithIntrSetTriggerModeIrq(ITH_INTR_DECOMPRESS, ITH_INTR_EDGE);

    // set IRQ to detect rising edge
    ithIntrSetTriggerLevelIrq(ITH_INTR_DECOMPRESS, ITH_INTR_HIGH_RISING);

    // Enable IRQ
    ithDcpsEnIntr();
    ithIntrEnableIrq(ITH_INTR_DECOMPRESS);

    if (!DcpsIsrSemaphor)
    {
        DcpsIsrSemaphor = malloc(sizeof(sem_t));
        sem_init(DcpsIsrSemaphor, 0, 0);
    }

    // (void)printf("DcpsIsrSemaphor=%x\n",DcpsIsrSemaphor);

    // (void)printf("Enable NAND IRQ~~leave\n");
}

static void
DcpsDisableIntr (
    void)
{
    ithDcpsDisIntr();
    ithIntrDisableIrq(ITH_INTR_DECOMPRESS);
    if (DcpsIsrSemaphor)
    {
        sem_destroy(DcpsIsrSemaphor);
        free(DcpsIsrSemaphor);
        DcpsIsrSemaphor = NULL;
    }
}
#endif

#ifdef  ENABLE_MEASURE_US_TIME

static uint32_t
dcps_getUsByTimer (
    void)
{
    uint8_t     tmr = DCPS_USE_TIMER; // timer1 offset = 0x00, timer2 = 0x10, timer3 = 0x20, ....
    uint32_t    tmrCtlReg = ITH_TIMER_BASE + 0x80 + 4 * (tmr - 1);
    uint32_t    tmrValReg = ITH_TIMER_BASE + 0x10 * (tmr - 1);
    uint32_t    tmrSetReg = tmrValReg + 0xC;
    uint32_t    alignment = 0;
    uint32_t    gTa = 0, gTb = 0;

    /* Set first ticks value */
    if ((ithReadRegA(tmrCtlReg) & 0xFF) == 0)
    {
        ithWriteRegA(tmrSetReg, 0xFFFFFFFF);
        usleep(1000);
        ithWriteRegA(tmrCtlReg, 0x35);
        usleep(1000);

        /* to calculate the alignment of timer counter in 1000us */
        gTa = ithReadRegA(tmrValReg) & 0xFFFFFFFF;
        usleep(1000);
        gTb = ithReadRegA(tmrValReg) & 0xFFFFFFFF;

        if (gTb >= gTa)
        {
            alignment = gTb - gTa;
        }
        else
        {
            alignment = (0xFFFFFFFF - gTa) + gTb;
        }

        gTimerAlignValue = alignment / 1000;
        (void)printf("gTimerAlignValue = %d\n", gTimerAlignValue);
    }

    return (ithReadRegA(tmrValReg) & 0xFFFFFFFF);
}

static uint32_t
dcps_getDurByTimer (
    uint32_t    t1,
    uint32_t    t0)
{
    uint32_t dur = 0;
    if (t1 >= t0)
    {
        dur = t1 - t0;
    }
    else
    {
        dur = (0xFFFFFFFF - t0) + t1;
    }

    if (gTimerAlignValue)
    {
        dur = dur / gTimerAlignValue;
    }

    /* statistics */
    if ((minDur > dur) && (mCnt > 3))
    {
        minDur = dur;
    }

    mCnt++;
    avgDur      = (totalDur + dur) / mCnt;
    totalDur    = totalDur + dur;

    // (void)printf("duration = %d us, (%x - %x)\n", dur, t0, t1);
    // (void)printf("SUM dur: min=%d, avg=%d, total=%d, c=%d\n", minDur, avgDur, totalDur, mCnt);

    return dur;
}
#endif

static void
DcpsInit (
    void)
{
#ifdef  ENABLE_DCPS_DBG_MSG
    ITP_LOG_INFO("DCPS:init\n");
#endif

    g_DcpsInfo.srcLen               = 0;
    g_DcpsInfo.dstLen               = 0;
    g_DcpsInfo.srcbuf               = NULL;
    g_DcpsInfo.dstbuf               = NULL;
    g_DcpsInfo.BlkSize              = 0x80000;
    g_DcpsInfo.DcpsMode             = 0;
    g_DcpsInfo.IsFastBootingMode    = 0;

    ithDcpsInit(&g_DcpsInfo);

#if defined(DCPS_IRQ_ENABLE)
    if (!g_DcpsInfo.IsEnableComQ)
    {
        DcpsEnableIntr();
    }
#endif
}

static void
DcpsWaitIdle (
    ITH_DCPS_INFO * Info)
{
#if defined(DCPS_IRQ_ENABLE)
    uint8_t EventRst;

    if (Info->IsEnableComQ)
    {
        return;
    }

    if (Info->srcLen > 4)
    {
        EventRst = itpSemWaitTimeout(DcpsIsrSemaphor, (unsigned long)(Info->srcLen / 4));
    }
    else
    {
        EventRst = itpSemWaitTimeout(DcpsIsrSemaphor, (unsigned long)10);
    }
    if (EventRst)
    {
        (void)printf("[ITP_DCPS][ERR] itpSemWaitTimeout() error[%x]!!\n", EventRst);
    #if (CFG_CHIP_FAMILY == 9070)
        ithPrintRegA((ITH_DPU_BASE + 0x100), 0x40);
    #endif
    }
    ithDcpsWait(Info);
#else
    if (Info->IsEnableComQ)
    {
        if (0)
        {
            (void)printf("wait for cmdq cntB: %x\n", g_DcpsInfo.RegDcpsCmdqCnt);

            while (g_DcpsInfo.TotalCmdqCount != g_DcpsInfo.RegDcpsCmdqCnt)
            {
                ithDcpsGetCmdqCount(&g_DcpsInfo);
                (void)printf("	wait for cmdqCnt3(%x,%x)\n", g_DcpsInfo.TotalCmdqCount, g_DcpsInfo.RegDcpsCmdqCnt);
                usleep(1000);
            }
        }
    }
    else
    {
        ithDcpsWait(Info);
    }
#endif
}

#ifdef ENABLE_PATCH_SEAL_WAIT_FIFO_EMPTY_ISSUE

static void
DcpsFireRetry (
    ITH_DCPS_INFO * inf)
{
    unsigned int    retryCnt = 0, tmp;
    #ifdef ENABLE_AUTO_GEN_CASE_FOR_VERIFY_RETRY_FUNC
    int             i;
    uint32_t        tmpLen          = 0;
    unsigned char   bkByte[16];
    unsigned char   enModifySrcPtn  = 0;
    #endif

    while (retryCnt < 3)
    {
        // retry 3 times if wait time-out or Status-fail
    #ifdef ENABLE_AUTO_GEN_CASE_FOR_VERIFY_RETRY_FUNC
        if (!retryCnt)
        {
            unsigned int rd = rand();
            if ((rd & 0x0F) == 0)
            // if(cnt==3)
            {
                enModifySrcPtn = 1;
                (void)printf("Random Enable Crash case:rd=%x, enModifySrcPtn=%x\n", rd, enModifySrcPtn);
            }

            if (enModifySrcPtn)
            {
                // gen trigger case
                if (inf->srcLen > 16)
                {
                    tmpLen      = inf->srcLen;
                    inf->srcLen -= 16;
                    (void)printf("modify src len: nLen=%02x, oLen=%02x (rd=%x)\n", inf->srcLen, tmpLen, rd);
                }
                else
                {
                    (void)printf("NOT modified :len = %x\n", inf->srcLen);
                    enModifySrcPtn = 0;
                    // while(1);
                }
            }
        }
    #endif

        ithDcpsFire(inf);
        DcpsWaitIdle(inf);

    #ifdef ENABLE_AUTO_GEN_CASE_FOR_VERIFY_RETRY_FUNC
        tmp = ithReadRegA(ITH_DPU_BASE + 0x11C);

        if (enModifySrcPtn)
        {
            if (!(inf->RegDcpsStatus & 0x80000000) && !tmp)
            {
                (void)printf("it should be wrong, but PASS: RegStatus = %x, %x\n", inf->RegDcpsStatus, tmp);
                while (1)
                {
                }
            }
        }
    #endif

        if ((inf->RegDcpsStatus & 0x80000000) || (inf->RegDcpsStatus & 0x00000004))
        {
    		int k;

    		(void)printf("dump DCPS regitsers: %x\n",ITH_DPU_BASE + 0x100);

    		for(k=0; k<48; k+=4)
    		{
    			tmp = ithReadRegA(ITH_DPU_BASE + 0x100 + k);
    			(void)printf("%08X ", tmp);
    			if((k&0xC) == 0xC)
    			{
					(void)printf("\n");
    			}
    		}
    		(void)printf("\n");

            // reset DPU & DCPS engine
            tmp = ithReadRegA(ITH_DPU_BASE + 0x11C);
            (void)printf("[DCPS ERR] Check DCPS error bit fail!! (%x,%x,%d)\n", inf->RegDcpsStatus, tmp, retryCnt);
        	if(retryCnt==2)
        	{
				uint8_t *b = (uint8_t*)inf->srcbuf;
    			//(void)printf info data
    			(void)printf("dcpsInf->DcpsMode         = %x\n", inf->DcpsMode);
    			(void)printf("dcpsInf->TotalCmdqCount   = %x\n", inf->TotalCmdqCount);
    			(void)printf("dcpsInf->BlkSize          = %x\n", inf->BlkSize);
    			(void)printf("dcpsInf->srcbuf           = %x\n", inf->srcbuf);
    			(void)printf("dcpsInf->srcLen           = %x\n", inf->srcLen);
    			(void)printf("dcpsInf->dstbuf           = %x\n", inf->dstbuf);
    			(void)printf("dcpsInf->dstLen           = %x\n", inf->dstLen);
    			(void)printf("dcpsInf->IsEnableComQ     = %x\n", inf->IsEnableComQ);
    			(void)printf("dcpsInf->RegDcpsStatus    = %x\n", inf->RegDcpsStatus);
				(void)printf("\n");

				ithInvalidateDCacheRange((void*)inf->srcbuf, inf->srcLen);
				(void)printf("Dump source data: %x, %x\n",inf->srcbuf, inf->srcLen);
				for(k=0; k<inf->srcLen; k++)
				{
					(void)printf("%02X ", b[k]);
					if((k&0xF) == 0xF)
					{
						(void)printf("\n");
					}
				}
				(void)printf("\n");
				(void)printf("END of Dump source data: %x\n",inf->srcbuf, inf->srcLen);
                while (1)
                {
                }
        	}
            ithDcpsResetEngine();// also need "TotalCmdqCount = 0;"

    #if defined(DCPS_IRQ_ENABLE)
            DcpsEnableIntr();
    #endif

    #ifdef ENABLE_AUTO_GEN_CASE_FOR_VERIFY_RETRY_FUNC
            // clear src & dst buf
            if (enModifySrcPtn)
            {
                inf->srcLen     = tmpLen;
                (void)printf("resume srcLen1:%02x, %02x\n", inf->srcLen, tmpLen);
                enModifySrcPtn  = 0;
            }
    #endif

            inf->RegDcpsStatus = 0;
        }
        else
        {
    #ifdef ENABLE_AUTO_GEN_CASE_FOR_VERIFY_RETRY_FUNC
            if (enModifySrcPtn)
            {
                inf->srcLen = tmpLen;
                (void)printf("resume srcLen2:%02x, %02x\n", inf->srcLen, tmpLen);

                inf->RegDcpsStatus  = 0;
                enModifySrcPtn      = 0;
            }
            else
            {
                break;
            }
    #else
            break;
    #endif
        }
        retryCnt++;
    }
}
#endif

static uint32_t
DcpsGetSize (
    uint8_t     * Src,
    uint32_t    SrcLen,
    uint8_t     mode)
{
    uint32_t    DcpsSize    = 0;
    uint32_t    i           = 0;
    uint32_t    out_len     = 0;
    uint32_t    in_len      = 0;
	uint8_t     m           = mode;
	uint32_t    blkSz       = g_DcpsInfo.BlkSize;
	uint32_t    newBlkSz    = 0;

	if (m)
	{
		(void)printf("    (0) DcpsMode=%x, SrcBuf=%p, SrcLen=%x\n", g_DcpsInfo.DcpsMode, Src, SrcLen);
	}

    if (g_DcpsInfo.DcpsMode == 2)
    {
        return SrcLen + (SrcLen >> 3);
    }

    if (g_DcpsInfo.DcpsMode == 1)
    {
        // SPEEDY
        uint8_t * src_lz = (uint8_t *)&Src[6];

        while (i < SrcLen)
        {
            // out_len = ((src_lz[i+3]) | (src_lz[i+2]<<8) | (src_lz[i+1]<<16) | (src_lz[i]<<24));
            out_len = ((src_lz[i]) | (src_lz[i + 1] << 8) | (src_lz[i + 2] << 16) | (src_lz[i + 3] << 24));
            i       = i + 4;
            // in_len = ((src_lz[i+3]) | (src_lz[i+2]<<8) | (src_lz[i+1]<<16) | (src_lz[i]<<24));
            in_len  = ((src_lz[i + 0]) | (src_lz[i + 1] << 8) | (src_lz[i + 2] << 16) | (src_lz[i + 3] << 24));
            i       = i + 4;

            if (out_len == 0)
            {
                // ITH_DCPS_LOG_ERROR "End of DecompressData, InLen=%08x, OutLen=%08x!\n", in_len, out_len ITH_DCPS_LOG_END
                break;
            }

            DcpsSize    += out_len;

            i           += in_len;
            break;
        }
        // (void)printf("SPEEDY OUTPUT SIZE:%x\n",DcpsSize);
    }
    else
    {
        while (i < SrcLen)
        {
            out_len = ((Src[i + 3]) | (Src[i + 2] << 8) | (Src[i + 1] << 16) | (Src[i] << 24));
            i       = i + 4;
            in_len  = ((Src[i + 3]) | (Src[i + 2] << 8) | (Src[i + 1] << 16) | (Src[i] << 24));
            i       = i + 4;

			if (m)
            {
			    (void)printf("    (1) i=%x, olen=%x, ilen=%x (size=%x)\n", i, out_len, in_len, DcpsSize);
            }

            if (out_len == 0)
            {
                // ITH_DCPS_LOG_ERROR "End of DecompressData, InLen=%08x, OutLen=%08x!\n", in_len, out_len ITH_DCPS_LOG_END
                break;
            }

			if(i==8)
			{
				//first loop
				if(out_len > blkSz)
				{
					if((out_len & 0xFFFF) == 0)
					{
						if(out_len != 0x100000)
						{
							(void)printf("    warning: out_len > dcps_block_size(i=%x, o=%x, bs=%x)\n", i, out_len, blkSz);
						}
						newBlkSz = out_len;
					}
					else
					{
						if((in_len + i) != SrcLen)
						{
							// got somethimg wrong:
							(void)printf("[DCPS error]: incorrect buffer data(i=%x, o=%x, bs=%x)(sBuf=%p, slen=%x)\n", in_len, out_len, blkSz, Src, SrcLen);
							break;
						}
						else
						{
							/* this case is OK if((in_len + i) == SrcLen) */
						}
					}
				}
				else
				{
					/* this case is OK if(out_len <= blkSz) */
				}
			}
			else
			{
				//after first loop
				if(out_len > blkSz)
				{
					if(newBlkSz)
					{
						if(out_len > newBlkSz)
						{
							(void)printf("[DCPS error]: out_len > dcps_block_size1(i=%x, o=%x, bs=%x, nbs=%x)\n", i, out_len, blkSz, newBlkSz);
							break;
						}
						else
						{
							/* this condition makes sense if(out_len <= newBlkSz) */
						}
					}
					else
					{
						(void)printf("[DCPS error]: out_len > dcps_block_size2(i=%x, o=%x, bs=%x, nbs=%x)\n", i, out_len, blkSz, newBlkSz);
						break;
					}
				}
				else
				{
					/* this condition makes sense if(out_len <= blkSz) */
				}
			}

            if (in_len < out_len)
            {
                DcpsSize += out_len;
            }
            else
            {
                DcpsSize += in_len;
            }

            i += in_len;


			if (m)
            {
                (void)printf("    (2) i=%x, olen=%x, ilen=%x (size=%x)\n", i, out_len, in_len, DcpsSize);
            }
        }
    }

    return DcpsSize;
}

static bool
DoDcps (
    ITH_DCPS_INFO * Info)
{
    ITH_DCPS_INFO   TmpInfo;
    uint32_t        i = 0, j = 0;
    // int cnt = 0;
    uint32_t        in_len;
    uint32_t        out_len;
    uint32_t        Reg32;
    uint8_t         * SrcBuff       = Info->srcbuf;
    uint8_t         * DstBuff       = Info->dstbuf;
#ifdef ENABLE_PATCH_DST_BASE_ALIGNMENT_ISSUE
    uint8_t         * alignDstBuff  = NULL;
#endif

#if defined(DCPS_IRQ_ENABLE)
    uint8_t EventRst;
#endif

#if !defined(__OPENRTOS__)
    uint8_t * SrcVbuf   = (uint8_t *)ithVmemAlloc(Info->srcLen + 16);
    uint8_t * DstVbuf   = (uint8_t *)ithVmemAlloc(Info->dstLen + 16);
#endif

#ifdef  ENABLE_DCPS_DBG_MSG
    (void)printf("src=%x, sL=%x\n", Info->srcbuf, Info->srcLen);
    (void)printf("dst=%x, dL=%x\n", Info->dstbuf, Info->dstLen);
#endif

    /*
    if(Info->srcLen > Info->dstLen)
    {
        (void)printf("ERROR: Src is larger than Dest[%d,%d]\n",Info->srcLen ,Info->dstLen);
        return false;
    }
    */
    if (Info->srcLen > 0x4000000)
    {
        (void)printf("Warning: src data is over 64MB[%d],(Not make sense)\n", Info->srcLen);
    }

    if (Info->dstLen > 0x4000000)
    {
        (void)printf("Warning: dst data is over 64MB[%d](Not make sense)\n", Info->dstLen);
    }

#ifdef  CFG_CPU_WB
    ithFlushDCacheRange((void *)Info->srcbuf, Info->srcLen);
    ithFlushMemBuffer();
    /*In FastBooting Mode, when the dst buffer is 0, ithInvalidateDCacheRange() must not be used..*/
    if (!g_DcpsInfo.IsFastBootingMode)
    {
        ithInvalidateDCacheRange((uint32_t *)Info->dstbuf, Info->dstLen);
    }
#endif

    if (g_DcpsInfo.DcpsMode)
    {
        TmpInfo.srcLen = Info->srcLen;
        TmpInfo.dstLen = Info->dstLen;

#if defined(WIN32)
        ithWriteVram((uint32_t *)SrcVbuf, SrcBuff, TmpInfo.srcLen);
        TmpInfo.srcbuf  = &SrcVbuf[i];
        TmpInfo.dstbuf  = &DstVbuf[j];
#else
        TmpInfo.srcbuf  = Info->srcbuf;
        TmpInfo.dstbuf  = Info->dstbuf;
#endif

/*
        if (out_len == 0)
        {
            //ITH_DCPS_LOG_WARING "End of DecompressData, InLen=%08x, OutLen=%08x!\n", in_len, out_len ITH_DCPS_LOG_END
            return false;
        }
*/
#ifdef  ENABLE_PATCH_DCPS_DMA_ISSUE
        ithLockMutex(ithStorMutex);
#endif

#ifdef  ENABLE_MEASURE_TIME
        gettimeofday(&startT, NULL);
        dcps_StartTicks();
#endif

#ifdef  ENABLE_MEASURE_US_TIME
        gT0 = dcps_getUsByTimer();
#endif

        TmpInfo.IsEnableComQ        = Info->IsEnableComQ;
        TmpInfo.DcpsMode            = g_DcpsInfo.DcpsMode;
        TmpInfo.LzCpsBytePerPxl     = g_DcpsInfo.LzCpsBytePerPxl;
        TmpInfo.IsFastBootingMode   = g_DcpsInfo.IsFastBootingMode;
        TmpInfo.TotalCmdqCount      = Info->TotalCmdqCount;

        ithDcpsFire(&TmpInfo);
        if (!g_DcpsInfo.IsFastBootingMode)
        {
            DcpsWaitIdle(Info);
        }

        Info->TotalCmdqCount = TmpInfo.TotalCmdqCount;

#ifdef  ENABLE_MEASURE_US_TIME
        {
            uint32_t gTdur = 0;
            gT1     = dcps_getUsByTimer();
            gTdur   = dcps_getDurByTimer(gT1, gT0);
            (void)printf("speedy dur=%d us, (%x - %x)\n", gTdur, gT0, gT1);
        }
#endif

#ifdef  ENABLE_MEASURE_TIME
        gettimeofday(&endT, NULL);
        (void)printf("dcps(speedy) dur=%d ms, tick=%d ms\n", itpTimevalDiff(&startT, &endT));
#endif

#ifdef  ENABLE_PATCH_DCPS_DMA_ISSUE
        ithUnlockMutex(ithStorMutex);
#endif

#if defined(WIN32)
        ithReadVram((uint32_t)Info->dstbuf, (void *)DstVbuf, Info->dstLen);
        if (SrcVbuf)
        {
            ithVmemFree(SrcVbuf);
        }
        if (DstVbuf)
        {
            ithVmemFree(DstVbuf);
        }
#else
        // if(!Info->IsEnableComQ)  ithInvalidateDCacheRange((void*)TmpInfo.dstbuf, Info->dstLen);
#endif

        return true;
    }

    for (;;)
    {
        int r = 0;
        TmpInfo.DcpsMode = 0;

        if ((i >= Info->srcLen) || (j >= Info->dstLen))
        {
            if (i > Info->srcLen)
            {
                (void)printf("warning: DCPS over Length[%x,%x][%x,%x]\n", i, Info->srcLen, j, Info->dstLen);
                (void)printf("src=%x, ", (uint32_t)Info->srcbuf);
                (void)printf("srcBuf=[%02x,%02x,%02x,%02x][%02x,%02x,%02x,%02x]\n", SrcBuff[0], SrcBuff[1], SrcBuff[2], SrcBuff[3], SrcBuff[4], SrcBuff[5], SrcBuff[6], SrcBuff[7]);
            }
            break;
        }

        // get src data length & get output data length
        out_len = ((SrcBuff[i + 3]) | (SrcBuff[i + 2] << 8) | (SrcBuff[i + 1] << 16) | (SrcBuff[i] << 24));
        i       = i + 4;
        in_len  = ((SrcBuff[i + 3]) | (SrcBuff[i + 2] << 8) | (SrcBuff[i + 1] << 16) | (SrcBuff[i] << 24));
        i       = i + 4;

        // (void)printf("dcps[%x].src=%x,%x dst=%x,%x\n",cnt,i,in_len,j,out_len);
        // cnt++;

        if (out_len == 0)
        {
            // ITH_DCPS_LOG_WARING "End of DecompressData, InLen=%08x, OutLen=%08x!\n", in_len, out_len ITH_DCPS_LOG_END
            break;
        }

        if (in_len < out_len)
        {
            TmpInfo.srcLen  = in_len;
            TmpInfo.dstLen  = out_len;

#if defined(WIN32)
            ithWriteVram((uint32_t *)SrcVbuf, SrcBuff, in_len + 8);
            TmpInfo.srcbuf  = &SrcVbuf[i];
            TmpInfo.dstbuf  = &DstVbuf[j];
#else
            TmpInfo.srcbuf  = &SrcBuff[i];
            TmpInfo.dstbuf  = &DstBuff[j];
#endif

            TmpInfo.IsEnableComQ = Info->IsEnableComQ;
            // (void)printf("set Enable Command Queue(%x,%x)\n",TmpInfo.IsEnableComQ, Info->IsEnableComQ);

#ifdef ENABLE_PATCH_DST_BASE_ALIGNMENT_ISSUE
            if (TmpInfo.dstLen % 4)
            {
                alignDstBuff    = (uint8_t *)malloc(Info->dstLen + 64);
                TmpInfo.dstbuf  = (uint8_t *)&alignDstBuff[4 - (Info->dstLen % 4)];
            }
#endif

#ifdef  ENABLE_PATCH_DCPS_DMA_ISSUE
            ithLockMutex(ithStorMutex);
#endif

#ifdef  ENABLE_MEASURE_TIME
            gettimeofday(&startT, NULL);
            dcps_StartTicks();
#endif

#ifdef  ENABLE_MEASURE_US_TIME
            gT0 = dcps_getUsByTimer();
#endif

#ifdef ENABLE_PATCH_DST_BASE_ALIGNMENT_ISSUE
            if (TmpInfo.dstLen % 4)
            {
                ithInvalidateDCacheRange((void *)alignDstBuff, out_len + 4 - (Info->dstLen % 4));
            }
            else
            {
                ithInvalidateDCacheRange((void *)TmpInfo.dstbuf, out_len);
            }
#else
            if (!Info->IsEnableComQ)
            {
                if (ithReadRegA(ITH_DCPS_REG_DDCR) == 0)
                {
                    uint32_t r32 = ithReadRegA(ITH_DCPS_REG_DSR);   // 0x1c
                    if ((r32 & 0x3) == 0)
                    {
                        (void)printf("	do DCPS invalid Dcache: r32=%x\n", r32);
                        ithInvalidateDCacheRange((void *)TmpInfo.dstbuf, out_len);
                    }
                }
            }
#endif

            TmpInfo.TotalCmdqCount = Info->TotalCmdqCount;

#ifdef ENABLE_PATCH_SEAL_WAIT_FIFO_EMPTY_ISSUE
            DcpsFireRetry(&TmpInfo);
            Info->RegDcpsStatus = TmpInfo.RegDcpsStatus;
#else
            ithDcpsFire(&TmpInfo);
            DcpsWaitIdle(Info);
#endif

            Info->TotalCmdqCount = TmpInfo.TotalCmdqCount;

#ifdef  ENABLE_MEASURE_US_TIME
            {
                uint32_t gTdur = 0;
                gT1     = dcps_getUsByTimer();
                gTdur   = dcps_getDurByTimer(gT1, gT0);
                (void)printf("ucl dur=%d us, (%x - %x)\n", gTdur, gT0, gT1);
            }
#endif

#ifdef  ENABLE_MEASURE_TIME
            gettimeofday(&endT, NULL);
            (void)printf("dcps dur=%d ms, tick=%d ms\n", itpTimevalDiff(&startT, &endT),  tick2);
#endif

#ifdef  ENABLE_PATCH_DCPS_DMA_ISSUE
            ithUnlockMutex(ithStorMutex);
#endif

#ifdef ENABLE_PATCH_DST_BASE_ALIGNMENT_ISSUE
            if (TmpInfo.dstLen % 4)
            {
                memcpy(&DstBuff[j], TmpInfo.dstbuf, Info->dstLen);

    #ifdef  CFG_CPU_WB
                ithFlushDCacheRange((void *)(&DstBuff[j]), Info->dstLen);
                ithFlushMemBuffer();
    #endif

                if (alignDstBuff != NULL)
                {
                    free(alignDstBuff);
                    alignDstBuff = NULL;
                }
            }
#endif

            if (Info->RegDcpsStatus & 0x04)
            {
                (void)printf("[DCPS ERR] Check DCPS error bit fail!!\n");
                return false;
            }
            // ithDcpsGetDoneLen(&out_len);

            i   += in_len;
            j   += out_len;
        }
        else
        {
#if defined(WIN32)
            ithWriteVram((uint32_t)&DstVbuf[j], (void *)&SrcBuff[i], in_len);
#else
            memcpy((void *)&DstBuff[j], (void *)&SrcBuff[i], in_len);
    #ifdef  CFG_CPU_WB
            ithFlushDCacheRange((void *)&DstBuff[j], in_len);
            ithFlushMemBuffer();
    #endif
#endif
            i   += in_len;
            j   += in_len;
        }
    }

#if defined(WIN32)
    ithReadVram((uint32_t)Info->dstbuf, (void *)DstVbuf, Info->dstLen);
    if (SrcVbuf)
    {
        ithVmemFree(SrcVbuf);
    }
    if (DstVbuf)
    {
        ithVmemFree(DstVbuf);
    }
#else
    /*In FastBooting Mode, when the dst buffer is 0, ithInvalidateDCacheRange() must not be used..*/
    if (!g_DcpsInfo.IsFastBootingMode)
    {
        ithInvalidateDCacheRange((void *)Info->dstbuf, Info->dstLen);
    }
#endif

    g_DcpsSetIndex++;

    return true;
}

static int
DecompressRead (
    int     file,
    char    * ptr,
    int     len,
    void    * info)
{
    int     result  = 0;
    uint8_t * b     = (uint8_t *)g_DcpsInfo.srcbuf;
#ifdef  ENABLE_DCPS_DBG_MSG
    uint8_t * b     = (uint8_t *)g_DcpsInfo.srcbuf;
    ITP_LOG_INFO("DCPS:rd %x,%x,%x,[%02x,%02x,%02x,%02x][%02x,%02x,%02x,%02x]\n", ptr, len, b, b[0], b[01], b[2], b[3], b[4], b[5], b[6], b[7]);
#endif

    if (((ptr == NULL) && !g_DcpsInfo.IsFastBootingMode) || (g_DcpsInfo.srcbuf == NULL))
    {
        errno   = (ITP_DEVICE_DECOMPRESS << ITP_DEVICE_ERRNO_BIT) | __LINE__;
        ITP_LOG_ERR("buffer is NULL [%x,%x]\n", (uint32_t)ptr, (uint32_t)g_DcpsInfo.srcbuf);
        result  = 0;
        goto end;
    }

    if (!len || !g_DcpsInfo.srcLen || !g_DcpsInfo.dstLen)
    {
        errno   = (ITP_DEVICE_DECOMPRESS << ITP_DEVICE_ERRNO_BIT) | __LINE__;
        ITP_LOG_ERR("incorrect argument %x,%x,%x\n", len, g_DcpsInfo.srcLen, g_DcpsInfo.dstLen);
        result  = 0;
        goto end;
    }

    if (len != g_DcpsInfo.dstLen)
    {
        errno = (ITP_DEVICE_DECOMPRESS << ITP_DEVICE_ERRNO_BIT) | __LINE__;
        ITP_LOG_ERR("len dose not match the calculated value %x,%x,%x\n", len, g_DcpsInfo.dstLen, (uint32_t)g_DcpsInfo.srcbuf);
        {
            uint8_t * b = (uint8_t *)g_DcpsInfo.srcbuf;
            ITP_LOG_ERR("srcBuf=%x, [%02x,%02x,%02x,%02x][%02x,%02x,%02x,%02x]\n", (uint32_t)g_DcpsInfo.srcbuf, b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]);
        }
        result = 0;
        goto end;
    }

    g_DcpsInfo.dstbuf = ptr;
    // g_DcpsInfo.IsEnableComQ = 1; //don't use command queue

    if (g_DcpsInfo.DcpsMode == 1)
    {
        uint32_t sz = 0;
        // check pixel mode
        if ((b[0] != 0x49) || (b[1] != 0x54) || (b[2] != 0x45) || ((b[3] & 0xF0) != 0x60))
        {
            errno = (ITP_DEVICE_DECOMPRESS << ITP_DEVICE_ERRNO_BIT) | __LINE__;
            ITP_LOG_ERR("incorrect SPEEDY format: srcBase = %x\n", (uint32_t)b);
            ITP_LOG_INFO("[%02x,%02x,%02x,%02x,%02x,%02x][%02x,%02x,%02x,%02x][%02x,%02x,%02x,%02x]\n", b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11], b[12], b[13]);
            result = 0;
            goto end;
        }
        if ((b[3] & 0x0F) != 0x04)
        {
            errno   = (ITP_DEVICE_DECOMPRESS << ITP_DEVICE_ERRNO_BIT) | __LINE__;
            ITP_LOG_ERR("incorrect SPEEDY Pixel mode:[%02x,%02x,%02x,%02x]\n", b[0], b[1], b[2], b[3]);
            result  = 0;
            goto end;
        }
        sz = ((b[10]) | (b[11] << 8) | (b[12] << 16) | (b[13] << 24));
        if (g_DcpsInfo.srcLen < sz)
        {
            errno   = (ITP_DEVICE_DECOMPRESS << ITP_DEVICE_ERRNO_BIT) | __LINE__;
            ITP_LOG_ERR("incorrect SPEEDY source size: %x, %x [%02x,%02x,%02x,%02x]\n", g_DcpsInfo.srcLen, sz, b[0], b[1], b[2], b[3]);
            result  = 0;
            goto end;
        }
        if (g_DcpsInfo.srcLen != (sz + 14))
        {
            ITP_LOG_INFO("SPEEDY source size: %x, %x [%02x,%02x,%02x,%02x]\n", g_DcpsInfo.srcLen, sz, b[0], b[1], b[2], b[3]);
        }
    }

    if ((g_DcpsInfo.DcpsMode == 1) && (g_DcpsInfo.dstLen == g_DcpsInfo.srcLen))
    {
        memcpy(g_DcpsInfo.dstbuf, g_DcpsInfo.srcbuf + 14, g_DcpsInfo.dstLen);
        ithFlushDCacheRange(g_DcpsInfo.dstbuf, g_DcpsInfo.dstLen);
        result = g_DcpsInfo.dstLen;
        goto end;
    }

    if (DoDcps(&g_DcpsInfo) == true)
    {
        result = g_DcpsInfo.dstLen;
    }
    else
    {
        errno   = (ITP_DEVICE_DECOMPRESS << ITP_DEVICE_ERRNO_BIT) | __LINE__;
        ITP_LOG_ERR("Decompress fail:\n");
        result  = 0;
        goto end;
    }

end:
   	if(result == 0)
   	{
		int k = 0;

    	//print info data
    	(void)printf("### DCPS ERROR, DCPS-info ###\n");
    	(void)printf("dcpsInf->DcpsMode         = %x\n", g_DcpsInfo.DcpsMode);
    	(void)printf("dcpsInf->TotalCmdqCount   = %x\n", g_DcpsInfo.TotalCmdqCount);
    	(void)printf("dcpsInf->BlkSize          = %x\n", g_DcpsInfo.BlkSize);
    	(void)printf("dcpsInf->srcbuf           = %x\n", (uint32_t)g_DcpsInfo.srcbuf);
    	(void)printf("dcpsInf->srcLen           = %x\n", g_DcpsInfo.srcLen);
    	(void)printf("dcpsInf->dstbuf           = %x\n", (uint32_t)g_DcpsInfo.dstbuf);
    	(void)printf("dcpsInf->dstLen           = %x\n", g_DcpsInfo.dstLen);
    	(void)printf("dcpsInf->IsEnableComQ     = %x\n", g_DcpsInfo.IsEnableComQ);
    	(void)printf("dcpsInf->RegDcpsStatus    = %x\n", g_DcpsInfo.RegDcpsStatus);
		(void)printf("\n");

		ithInvalidateDCacheRange((void*)ptr, g_DcpsInfo.srcLen);
		(void)printf("Dump source data: %x, len: %d\n",(uint32_t)g_DcpsInfo.srcbuf, g_DcpsInfo.srcLen);
        if ( g_DcpsInfo.srcbuf)
        {
		    for(k=0; k<256; k++)
		    {
			    (void)printf("%02X ", g_DcpsInfo.srcbuf[k]);
			    if((k&0xF) == 0xF)	(void)printf("\n");
            }
		}
        if ((uint32_t)ptr != (uint32_t)g_DcpsInfo.srcbuf)
        {
			(void)printf("\n ptr != g_DcpsInfo.srcbuf: ptr=%X, srcBuf=%X\n", (uint32_t)ptr, (uint32_t)g_DcpsInfo.srcbuf);
			if (ptr)
		    {
			    for(k=0; k<256; k++)
			    {
			       (void)printf("%02X ", ptr[k]);
			       if((k&0xF) == 0xF)	(void)printf("\n");
			    }
            }
		}
		(void)printf("\n");
		(void)printf("END of Dump source data: %x, %d\n",(uint32_t)g_DcpsInfo.srcbuf, g_DcpsInfo.srcLen);
    }

    g_DcpsInfo.srcLen   = 0;
    g_DcpsInfo.dstLen   = 0;
    g_DcpsInfo.srcbuf   = NULL;
    g_DcpsInfo.dstbuf   = NULL;

#ifdef  ENABLE_DCPS_WB_CACHE
    if (gDcpsSrcBase != NULL)
    {
        free(gDcpsSrcBase);
        gDcpsSrcBase = NULL;
    }
#endif

    return result;
}

static int
DecompressWrite (
    int     file,
    char    * ptr,
    int     len,
    void    * info)
{
#ifdef  ENABLE_DCPS_DBG_MSG
    uint8_t * b = (uint8_t *)ptr;
    ITP_LOG_INFO("DCPS:wt %x,%x,%x,[%02x,%02x,%02x,%02x][%02x,%02x,%02x,%02x]\n", ptr, len, b, b[0], b[01], b[2], b[3], b[4], b[5], b[6], b[7]);
#endif

    if ((ptr == NULL) || !len)
    {
        errno = (ITP_DEVICE_DECOMPRESS << ITP_DEVICE_ERRNO_BIT) | __LINE__;
        ITP_LOG_ERR("incorrect argument %x,%x\n", (uint32_t)ptr, len);
        goto wt_end;
    }

	if( ((uint32_t)ptr >= CFG_RAM_SIZE) || (len >= CFG_RAM_SIZE))
	{
		errno = (ITP_DEVICE_DECOMPRESS << ITP_DEVICE_ERRNO_BIT) | __LINE__;
        ITP_LOG_ERR("incorrect argument ptr=%p, len=%x, ram_size=%x\n", ptr, len, CFG_RAM_SIZE);
		goto wt_end;
	}

    g_DcpsInfo.dstLen = DcpsGetSize(ptr, len, 0);

    if (!g_DcpsInfo.dstLen)
    {
        errno = (ITP_DEVICE_DECOMPRESS << ITP_DEVICE_ERRNO_BIT) | __LINE__;
        ITP_LOG_ERR("decompress length is 0 !!\n");
        goto wt_end;
    }

	if(g_DcpsInfo.dstLen >= CFG_RAM_SIZE)
	{
		errno = (ITP_DEVICE_DECOMPRESS << ITP_DEVICE_ERRNO_BIT) | __LINE__;
        ITP_LOG_ERR("decompress length > RAM size(%x)!!\n",(uint32_t)CFG_RAM_SIZE);
		goto wt_end;
	}

#ifdef  ENABLE_DCPS_WB_CACHE
    if (g_DcpsInfo.IsEnableComQ)
    {
        g_DcpsInfo.srcbuf = ptr;
    }
    else
    {
        if (g_DcpsInfo.IsFastBootingMode)
        {
            g_DcpsInfo.srcbuf = ptr;
        }
        else
        {
            if ((uint32_t)ptr & ((uint32_t)(DCPS_WB_ALIGN_SIZE - 1)))
            {
                ITP_LOG_WARN("DCPS's buffer is not alignment(WB cache): %x, %x\n", ptr, DCPS_WB_ALIGN_SIZE);

                gDcpsSrcBase        = malloc(len + 64 + DCPS_WB_ALIGN_SIZE - 1);
                g_DcpsInfo.srcbuf   = (uint8_t *)((uint32_t)(gDcpsSrcBase + DCPS_WB_ALIGN_SIZE - 1) & (uint32_t) ~(DCPS_WB_ALIGN_SIZE - 1));

                if (g_DcpsInfo.DcpsMode == 1)
                {
                    memcpy(g_DcpsInfo.srcbuf, ptr, len + 8 + 6);
                }
                else
                {
                    memcpy(g_DcpsInfo.srcbuf, ptr, len + 8);
                }
            }
            else
            {
                g_DcpsInfo.srcbuf = ptr;
            }
        }
    }

    g_DcpsInfo.srcLen   = len;
#else
    g_DcpsInfo.srcbuf   = ptr;
    g_DcpsInfo.srcLen   = len;
#endif

#ifdef  ENABLE_DCPS_DBG_MSG
    ITP_LOG_INFO("dcpsWt:%x,%d\n", g_DcpsInfo.srcbuf, g_DcpsInfo.srcLen);
#endif
    if (g_DcpsInfo.DcpsMode == 1)
    {
        ithFlushDCacheRange((void *)g_DcpsInfo.srcbuf, g_DcpsInfo.srcLen + 8 + 6);
    }
    else
    {
        ithFlushDCacheRange((void *)g_DcpsInfo.srcbuf, g_DcpsInfo.srcLen + 8);
    }

    ithFlushMemBuffer();

    return len;

wt_end:
    return 0;
}

static int
DecompressIoctl (
    int             file,
    unsigned long   request,
    void            * ptr,
    void            * info)
{
    switch (request)
    {
        case ITP_IOCTL_INIT:

			#ifdef  ENABLE_DCPS_MUTEX
			pthread_mutex_lock(&dcpsMutex);
			#endif

#ifdef  ENABLE_DYNAMIC_SWITCH_DCPS_CLOCK
            gDcpsEnClk = 1;
            ithDcpsEnableClock();
#endif
            if (ptr != NULL)
            {
                unsigned long enCmdQ = *(unsigned long *)ptr;

                if (enCmdQ)
                {
                    g_DcpsInfo.IsEnableComQ = 1;
                    if (enCmdQ == 2)
                    {
                        g_DoWaitCmdq = 1;
                    }
                    else
                    {
                        g_DoWaitCmdq = 0;
                    }
                }
                else
                {
                    if (g_DcpsInfo.IsEnableComQ)
                    {
                        // wait for cmdq finished
                        while (g_DcpsInfo.TotalCmdqCount != g_DcpsInfo.RegDcpsCmdqCnt)
                        {
                            ithDcpsGetCmdqCount(&g_DcpsInfo);
                            (void)printf("	wait for cmdqCnt1(%x,%x)\n", g_DcpsInfo.TotalCmdqCount, g_DcpsInfo.RegDcpsCmdqCnt);
                            usleep(2*1000);
                        }
                    }
                    g_DcpsInfo.IsEnableComQ = 0;
                }
            }
            else
            {
                if (g_DcpsInfo.IsEnableComQ)
                {
                    // wait for cmdq finished
                    while (g_DcpsInfo.TotalCmdqCount != g_DcpsInfo.RegDcpsCmdqCnt)
                    {
                        ithDcpsGetCmdqCount(&g_DcpsInfo);
                        (void)printf("	wait for cmdqCnt2(%x,%x)\n", g_DcpsInfo.TotalCmdqCount, g_DcpsInfo.RegDcpsCmdqCnt);
                        usleep(2*1000);
                    }
                }
                g_DcpsInfo.IsEnableComQ = 0;
            }

            DcpsInit();
            break;

        case ITP_IOCTL_EXIT:
#if defined(DCPS_IRQ_ENABLE)
            DcpsDisableIntr();
#endif

            if (g_DcpsInfo.IsEnableComQ)
            {
                // wait for cmdq finished
                if (g_DoWaitCmdq)
                {
					uint32_t to_cnt = 0;
                    while (g_DcpsInfo.TotalCmdqCount != g_DcpsInfo.RegDcpsCmdqCnt)
                    {
                        ithDcpsGetCmdqCount(&g_DcpsInfo);
                        // (void)printf("   wait for cmdqCnt0(%x,%x)\n",g_DcpsInfo.TotalCmdqCount,g_DcpsInfo.RegDcpsCmdqCnt);
                        usleep(1 * 1000);
						if(to_cnt++ > 2000)
						{
							//wait for 2 seconds
							(void)printf("[DCPS error] Wait DCPS timeout(%x, %x)\n",g_DcpsInfo.TotalCmdqCount, g_DcpsInfo.RegDcpsCmdqCnt);
							break;
						}
                    }
                }
                g_DoWaitCmdq = 0;
            }
            else
            {
                // (void)printf("   my1 wait cmdqCnt0(%x,%x)\n",g_DcpsInfo.TotalCmdqCount,g_DcpsInfo.RegDcpsCmdqCnt);
                if (g_DcpsInfo.TotalCmdqCount != g_DcpsInfo.RegDcpsCmdqCnt)
                {
					uint32_t to_cnt = 0;
                    while (g_DcpsInfo.TotalCmdqCount != g_DcpsInfo.RegDcpsCmdqCnt)
                    {
                        ithDcpsGetCmdqCount(&g_DcpsInfo);
                        // (void)printf("   wait for cmdqCnt0(%x,%x)\n",g_DcpsInfo.TotalCmdqCount,g_DcpsInfo.RegDcpsCmdqCnt);
                        usleep(1000);
						if(to_cnt++ > 2000)
						{
							//wait for 2 seconds
							(void)printf("[DCPS error] Wait DCPS timeout(%x, %x)\n",g_DcpsInfo.TotalCmdqCount, g_DcpsInfo.RegDcpsCmdqCnt);
							break;
						}
                    }
                }
            }

            ithDcpsExit();

#ifdef  ENABLE_DYNAMIC_SWITCH_DCPS_CLOCK
            // if fast boot, don't disable dcps's clock
            gDcpsEnClk = 0;
            if (!g_DcpsInfo.IsFastBootingMode)
            {
                if (!gDpuEnClk)
                {
                    ithDcpsDisableClock();
                }
            }
#endif

#ifdef  ENABLE_DCPS_MUTEX
            pthread_mutex_unlock(&dcpsMutex);
#endif

            break;

        case ITP_IOCTL_GET_SIZE:
            *(unsigned long *)ptr = g_DcpsInfo.dstLen;
            break;

        case ITP_IOCTL_SET_VOLUME:
            g_DcpsInfo.BlkSize = *(unsigned long *)ptr;
            break;

        case ITP_IOCTL_SET_MODE:
        {
            uint8_t blzMode = *(uint8_t *)ptr;
            // if(blzMode > 2)
            if ((blzMode != 0) && (blzMode != 1) && (blzMode != 2))
            {
                errno = (ITP_DEVICE_DECOMPRESS << ITP_DEVICE_ERRNO_BIT) | __LINE__;
                ITP_LOG_ERR("incorrect setting of DCPS mode [%x]\n", blzMode);
                return -1;
            }
            else
            {
                g_DcpsInfo.DcpsMode = blzMode;
                // (void)printf("g_DcpsInfo.DcpsMode=%d\n",g_DcpsInfo.DcpsMode);
            }
        }
        break;

        case ITP_IOCTL_SET_BYTE_PER_PIXEL:
        {
            uint8_t lzByteMode = (uint8_t)*(unsigned long *)ptr;
            if ((lzByteMode != 1) && (lzByteMode != 2) && (lzByteMode != 4))
            {
                // (void)printf("incorrect setting(only 1/2/4 bytes per pixel) [%d]\n", lzByteMode);
                errno = (ITP_DEVICE_DECOMPRESS << ITP_DEVICE_ERRNO_BIT) | __LINE__;
                ITP_LOG_ERR("incorrect setting(only 1/2/4 bytes per pixel) [%x]\n", lzByteMode);
                return -1;
            }
            else
            {
                g_DcpsInfo.LzCpsBytePerPxl = lzByteMode;
                (void)printf("g_DcpsInfo.LzCpsByte=%d\n", g_DcpsInfo.LzCpsBytePerPxl);
            }
        }
        break;

        case ITP_IOCTL_SET_FAST_BOOTING:
        {
            g_DcpsInfo.IsFastBootingMode = 1;
        }
        break;

        default:
            errno = (ITP_DEVICE_DECOMPRESS << ITP_DEVICE_ERRNO_BIT) | 1;
            return -1;
    }
    return 0;
}

const ITPDevice itpDeviceDecompress =
{
    ":decompress",
    itpOpenDefault,
    itpCloseDefault,
    DecompressRead,
    DecompressWrite,
    itpLseekDefault,
    DecompressIoctl,
    NULL
};
