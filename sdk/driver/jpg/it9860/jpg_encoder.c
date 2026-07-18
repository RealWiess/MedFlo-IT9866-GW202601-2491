#include "jpg_defs.h"
#include "jpg_codec.h"
#include "jpg_hw.h"
#include "jpg_common.h"
#include "jpg_extern_link.h"
#include "ite/ith.h"

#if (CONFIG_JPG_CODEC_DESC_ENCODER_DESC)
// =============================================================================
//                  Constant Definition
// =============================================================================

// =============================================================================
//                  Macro Definition
// =============================================================================

// =============================================================================
//                  Structure Definition
// =============================================================================
typedef struct JPG_ENCODER_TAG
{
    JPG_TRIGGER_MODE        triggerMode;

    uint8_t                 * pSysBsBuf_Cur; // record the current position in pSysBsBuf

    JPG_BS_RING_BUF_INFO    jBsRingBufInfo;

    uint8_t                 * pYuvEncBuf;
    uint32_t                yuvEncBufSize;
} JPG_ENCODER;
// =============================================================================
//                  Global Data Definition
// =============================================================================

// =============================================================================
//                  Private Function Definition
// =============================================================================

    #if 0
static void
jpg_isr (
    void * arg);

static void
jpg_isr (
    void * arg)
{
    ithPrintf("jpg_isr\n");
    JPG_ClearInterrupt();

    return;
}

static void
_JPG_Enable_Interrupt (
    ITHIntrHandler handler)
{
    /** register interrupt handler to interrupt mgr */
    ithIntrRegisterHandlerIrq(ITH_INTR_JPEG, handler, NULL);
    ithIntrSetTriggerModeIrq(ITH_INTR_JPEG, ITH_INTR_EDGE);
    ithIntrSetTriggerLevelIrq(ITH_INTR_JPEG, ITH_INTR_HIGH_RISING);
    ithIntrEnableIrq(ITH_INTR_JPEG);
    /** enable encoder interrupt */
}
    #endif

static JPG_ERR
_JPG_Enc_HW_Update (
    JPG_CODEC_HANDLE * pHJCodec)
{
    JPG_ERR result = JPG_ERR_OK;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ENC, "0x%x\n", pHJCodec);

    do
    {
        JPG_FRM_COMP        * pJFrmComp     = &pHJCodec->jFrmCompInfo;
        JPG_HW_CTRL         * pJHwCtrl      = &pHJCodec->jHwCtrl;
        JPG_LINE_BUF_INFO   * pJLineBufInfo = &pHJCodec->jLineBufInfo;
        JPG_HW_BS_CTRL      * pJHwBsCtrl    = &pHJCodec->jHwBsCtrl;
        JPG_FRM_SIZE_INFO   * pJFrmSizeInfo = &pHJCodec->jFrmSizeInfo;

        JPG_SetCodecCtrlReg(pJHwCtrl->codecCtrl);

        JPG_SetDriReg(pJFrmComp->restartInterval);

        JPG_SetTableSpecifyReg(pJFrmComp);

        JPG_SetFrmSizeInfoReg(pJFrmSizeInfo);

        JPG_SetLineBufInfoReg(pJLineBufInfo);

        JPG_SetLineBufSliceUnitReg(pJLineBufInfo->sliceNum, pJFrmComp->jFrmInfo[0].verticalSamp);

        JPG_SetBitStreamBufInfoReg(pJHwBsCtrl);

        JPG_SetSamplingFactorReg(pJFrmComp);

        // set Q table
        JPG_SetQtableReg(pJFrmComp);

    #if 1
        // set huffman table
        JPG_SetHuffmanCodeCtrlReg(JPG_HUUFFMAN_Y_DC, pJHwCtrl->dcHuffTable[0]);
        JPG_SetHuffmanCodeCtrlReg(JPG_HUUFFMAN_UV_DC, pJHwCtrl->dcHuffTable[1]);
        JPG_SetHuffmanCodeCtrlReg(JPG_HUUFFMAN_Y_AC, pJHwCtrl->acHuffTable[0]);
        JPG_SetHuffmanCodeCtrlReg(JPG_HUUFFMAN_UV_AC, pJHwCtrl->acHuffTable[1]);

        JPG_SetDcHuffmanValueReg(JPG_HUUFFMAN_Y_DC,
                                 (pJHwCtrl->dcHuffTable[0] + 16),
                                 pJHwCtrl->dcHuffW2talCodeLenCnt[0]);

        JPG_SetDcHuffmanValueReg(JPG_HUUFFMAN_UV_DC,
                                 (pJHwCtrl->dcHuffTable[1] + 16),
                                 pJHwCtrl->dcHuffW2talCodeLenCnt[1]);

        JPG_SetEncodeAcHuffmanValueReg(JPG_HUUFFMAN_Y_AC,
                                       (pJHwCtrl->acHuffTable[0] + 16),
                                       pJHwCtrl->acHuffW2talCodeLenCnt[0]);

        JPG_SetEncodeAcHuffmanValueReg(JPG_HUUFFMAN_UV_AC,
                                       (pJHwCtrl->acHuffTable[1] + 16),
                                       pJHwCtrl->acHuffW2talCodeLenCnt[1]);
    #endif

        JPG_DropHv((JPG_REG)(pHJCodec->ctrlFlag >> 16));

		#if 0
        JPG_SetTilingMode();
		#endif
        if (pJFrmComp->exColorSpace == JPG_COLOR_SPACE_NV12)
        {
            JPG_SetNV12Enable();
        }
        else if (pJFrmComp->exColorSpace == JPG_COLOR_SPACE_NV21)
        {
            JPG_SetNV21Enable();
        }

    #if 0      // just for test
        JPG_EnableInterrupt();
    #endif

		#if 0
        JPG_SetTilingTable(pJLineBufInfo, 1, 1);
		#endif
    } while (false);

    if (result != JPG_ERR_OK)
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
    }

    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ENC);
    return result;
}

static JPG_ERR
_JPG_Enc_HW_Wait_Idle (
    uint32_t    timeOutCnt,
    JPG_REG     * bufStatus)
{
    #define JPG_ENC_STOP_STATUS ((JPG_REG)JPG_STATUS_BITSTREAM_BUF_FULL | (JPG_REG)JPG_STATUS_ENCODE_COMPLETE)

    JPG_ERR     result  = JPG_ERR_OK;
    uint32_t    cnt     = 0;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ENC, "%d, 0x%x\n", timeOutCnt, bufStatus);

    *bufStatus = JPG_GetProcStatusReg();
    //(void)printf("*bufStatus=0x%lx\n", *bufStatus);
    while ( ((*bufStatus) & (JPG_REG)JPG_ENC_STOP_STATUS) == 0u )
    {
        (void)usleep(10 * 1000);
        cnt++;
        if (cnt > timeOutCnt)
        {
            result = JPG_ERR_ENC_TIMEOUT;
            JPG_LogReg(true);
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg err 0x%x !", result);
            break;
        }

        *bufStatus = JPG_GetProcStatusReg();
    }

    if (result != JPG_ERR_OK)
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
    }

    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ENC);
    return result;
}

static JPG_ERR
_JPG_Enc_Copy_To_SysBsBuf (
    JPG_CODEC_HANDLE    * pHJCodec,
    uint32_t            copySize)
{
    JPG_ERR                 result = JPG_ERR_OK;
    uint32_t                resiBsBufSize = 0;
    uint32_t                ringBsBufSize = 0;
    JPG_MEM_MOVE_INFO       jMemInfo = {0};
    JPG_ENCODER             * pJEncoder = (JPG_ENCODER *)pHJCodec->privData;
    JPG_BS_RING_BUF_INFO    * pJBsRingBufInfo = &((JPG_ENCODER *)pHJCodec->privData)->jBsRingBufInfo;
    uint8_t                 * pCur = NULL, * pVramBsRingBuf = NULL, * pNewBsBufAddr = NULL;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ENC, "0x%x, %d\n", pHJCodec, copySize);

    pCur            = pJEncoder->pSysBsBuf_Cur;

    pVramBsRingBuf  = pJBsRingBufInfo->pBsRingBuf;
    // win32
    _jpg_reflash_vram(pHJCodec->pHInJStream,
                      JPG_STREAM_CMD_GET_VRAM_ENC_RING_BUF,
                      0, 0, &pVramBsRingBuf);

    pNewBsBufAddr   = (uint8_t *)((uint32_t)pVramBsRingBuf + pJBsRingBufInfo->rwSize);

    resiBsBufSize   = pJBsRingBufInfo->bsRingBufLeng - pJBsRingBufInfo->rwSize;

    copySize        &= ~0x1u;

    ringBsBufSize   = (copySize > resiBsBufSize) ? (copySize - resiBsBufSize) : ringBsBufSize;
    if (ringBsBufSize > 0u)
    {
        /**
         *                              last       wrSize
         *  |<-------------------------->|<--------->|<----copy this segment----->|
         */
        jMemInfo.dstAddr    = 0;
        jMemInfo.srcAddr    = (uint32_t)pNewBsBufAddr;
        jMemInfo.sizeByByte = resiBsBufSize;
        (void)Jpg_Ext_Link_Ctrl(JEL_CTRL_CPU_INVALD_CACHE, NULL, (void *)&jMemInfo);

        // move data
        jMemInfo.dstAddr    = (uint32_t)pCur;
        jMemInfo.srcAddr    = (uint32_t)pNewBsBufAddr;
        jMemInfo.sizeByByte = resiBsBufSize;
        (void)Jpg_Ext_Link_Ctrl(JEL_CTRL_HOST_R_MEM, NULL, (void *)&jMemInfo);

        /**
         *  last == ringBitstreamBufSize
         *                             last        wrSize
         *  |<--copy this segment------>|<---------->|<------------------------->|
         */
        pVramBsRingBuf = pJBsRingBufInfo->pBsRingBuf;
        // win32
        _jpg_reflash_vram(pHJCodec->pHInJStream,
                          JPG_STREAM_CMD_GET_VRAM_ENC_RING_BUF,
                          0, 0, &pVramBsRingBuf);

        pNewBsBufAddr       = pVramBsRingBuf;
        pCur                = (uint8_t *)((uint32_t)pCur + resiBsBufSize);

        jMemInfo.dstAddr    = 0;
        jMemInfo.srcAddr    = (uint32_t)pNewBsBufAddr;
        jMemInfo.sizeByByte = ringBsBufSize;
        (void)Jpg_Ext_Link_Ctrl(JEL_CTRL_CPU_INVALD_CACHE, NULL, (void *)&jMemInfo);

        // move data
        jMemInfo.dstAddr        = (uint32_t)pCur;
        jMemInfo.srcAddr        = (uint32_t)pNewBsBufAddr;
        jMemInfo.sizeByByte     = ringBsBufSize;
        (void)Jpg_Ext_Link_Ctrl(JEL_CTRL_HOST_R_MEM, NULL, (void *)&jMemInfo);

        pJBsRingBufInfo->rwSize = ringBsBufSize;
        pCur                    += ringBsBufSize;
    }
    else
    {
        jMemInfo.dstAddr    = 0;
        jMemInfo.srcAddr    = (uint32_t)pNewBsBufAddr;
        jMemInfo.sizeByByte = copySize;
        (void)Jpg_Ext_Link_Ctrl(JEL_CTRL_CPU_INVALD_CACHE, NULL, (void *)&jMemInfo);

        // move data
        jMemInfo.dstAddr    = (uint32_t)pCur;
        jMemInfo.srcAddr    = (uint32_t)pNewBsBufAddr;
        jMemInfo.sizeByByte = copySize;
        (void)Jpg_Ext_Link_Ctrl(JEL_CTRL_HOST_R_MEM, NULL, (void *)&jMemInfo);

        // update SW bitstream buffer read point
        pJBsRingBufInfo->rwSize = ((pJBsRingBufInfo->rwSize + copySize) % (pJBsRingBufInfo->bsRingBufLeng));
        pCur                    += copySize;
    }

    pJEncoder->pSysBsBuf_Cur = pCur;

    // set bitstream buffer read size
    JPG_SetBitstreamBufRwSizeReg(copySize);

    // set bitstream buffer read end
    JPG_SetBitstreamBufCtrlReg(JPG_MSK_BITSTREAM_BUF_RW_END);

    if (result != JPG_ERR_OK)
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
    }

    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ENC);
    return result;
}

static JPG_ERR
_JPG_Enc_Get_CodedData (
    JPG_CODEC_HANDLE    * pHJCodec,
    bool                * bEncIdle)
{
    JPG_ERR result = JPG_ERR_OK;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ENC, "0x%x, 0x%x\n", pHJCodec, bEncIdle);

    do
    {
        JPG_ENCODER * pJEncoder         = (JPG_ENCODER *)pHJCodec->privData;
        JPG_REG     bufStatus           = 0;
        uint32_t    bsBufValidSize      = 0;
        int         sysRamRemainSize    = 0;

        *bEncIdle = false;

        // ----------------------------------------
        // check system bs buffer full or not
        sysRamRemainSize = pHJCodec->pSysBsBuf + pHJCodec->sysBsBufSize - pJEncoder->pSysBsBuf_Cur;
        if (sysRamRemainSize < 2)  // reserve last 2 bytes
        {
            // sys bs buffer full
            result = JPG_ERR_ENC_OVER_BS_BUF;
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg err 0x%x (Need to extend AP bit stream buffer) !", result);
            break;
        }

        // ----------------------------------------
        // check H/W status
        result = _JPG_Enc_HW_Wait_Idle(JPG_TIMEOUT_COUNT, &bufStatus);
        if (result != JPG_ERR_OK)
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg err 0x%x !", result);
            break;
        }

        bsBufValidSize  = JPG_GetBitStreamValidSizeReg();
        //(void)printf("bsBufValidSize=0x%lx\n", bsBufValidSize);
        bsBufValidSize  *= 4u;
		
        if ((int)bsBufValidSize > sysRamRemainSize)
        {
            // sys bs buffer full
            result = JPG_ERR_ENC_OVER_BS_BUF;
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg err 0x%x (Need to extend AP bit stream buffer) !", result);
            break;
        }

        // ------------------------------------
        // handle bs buffer full case (it should not be happened),
        if ( (bufStatus & (JPG_REG)JPG_STATUS_BITSTREAM_BUF_FULL) != 0u )
        {
            //(void)printf("run here!,bufStatus=%ld\n", bufStatus);
            // Move bitstream in unit of 32 bytes alignment
            // if JPEG_STATUS_ENCODE_COMPLETE, remove all the remained bitstream at once.
            bsBufValidSize = ((bsBufValidSize >> 5) << 5);
        }

        if (bsBufValidSize > 0u)
        {
            // need to move out data which is in bs buffer.
            result = _JPG_Enc_Copy_To_SysBsBuf(pHJCodec, bsBufValidSize);
            if (result != JPG_ERR_OK)
            {
                jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg err 0x%x !", result);
                break;
            }
        }

        //(void)printf("bufStatus=0x%lx\n", bufStatus);
        (*bEncIdle) = ( (bufStatus & (JPG_REG)JPG_STATUS_LINE_BUF_EMPTY) != 0u ) ? true : (*bEncIdle);

        // --------------------------------------
        // handle encoding process complete
        if ( (bufStatus & (JPG_REG)JPG_STATUS_ENCODE_COMPLETE) != 0u )
        {
            JPG_SetBitstreamBufCtrlReg(JPG_MSK_LAST_BITSTREAM_DATA);
            *bEncIdle = true;
        }
    } while (false);

    if (result != JPG_ERR_OK)
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
    }

    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ENC);
    return result;
}

static uint8_t *
_JPG_Enc_Clear_Dummy (
    uint8_t * ptEnd)
{
    int validLeng = 0;

    while (validLeng < 10)  // 10 => just follow old version
    {
        ptEnd--;
        if (*ptEnd != 0xFFu)
        {
            break;
        }
        validLeng++;
    }

    // add end of image marker
    *(ptEnd + 1)    = 0xFF;
    *(ptEnd + 2)    = 0xD9;

    ptEnd           += 3;

    return ptEnd;
}

static JPG_ERR
_JPG_Enc_Set_Line_buf (
    JPG_CODEC_HANDLE * pHJCodec)
{
    JPG_ERR result = JPG_ERR_OK;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ENC, "0x%x\n", pHJCodec);

    do
    {
        JPG_LINE_BUF_INFO   * pJLineBufInfo = &pHJCodec->jLineBufInfo;
        JPG_SHARE_DATA      * pJShare2Isp   = &pHJCodec->jShare2Isp;
    #ifdef _MSC_VER
        JPG_STREAM_DESC     * pJStreamDesc  = &pHJCodec->pHInJStream->jStreamDesc;
    #endif
        if ( (pJShare2Isp->addrY == 0u) || (pJShare2Isp->addrU == 0u) || (pJShare2Isp->addrV == 0u) )
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, "line buf Null pointer !!");
            result = JPG_ERR_NULL_POINTER;
            break;
        }

    #ifdef _MSC_VER
        // ------------------------------
        // allocate vram line buff
        if ( pJStreamDesc->jHeap_mem != NULL )
        {
            JPG_ENCODER     * pJEncoder = (JPG_ENCODER *)pHJCodec->privData;
            JPG_FRM_COMP    * pJFrmComp = &pHJCodec->jFrmCompInfo;
            uint32_t        yuvSrcBufSize = 0;
            uint8_t         * pVramYuvBufAddr = NULL, * pTmpBufAddr = NULL;

            yuvSrcBufSize           = ((pJShare2Isp->pitchY * (pJShare2Isp->sliceCount << 3)) << 1);
            pJEncoder->pYuvEncBuf   = pJStreamDesc->jHeap_mem(pHJCodec->pHInJStream,
                                                            (uint32_t)JPG_HEAP_ENC_YUV_BUF,
                                                            yuvSrcBufSize, &pJEncoder->yuvEncBufSize);
            // Y
            pTmpBufAddr = pJEncoder->pYuvEncBuf;
            (void)memcpy(pTmpBufAddr, pJShare2Isp->addrY, pJShare2Isp->pitchY * (pJShare2Isp->sliceCount << 3));
            // U
            pTmpBufAddr += (pJShare2Isp->pitchY * (pJShare2Isp->sliceCount << 3));
            (void)memcpy(pTmpBufAddr, pJShare2Isp->addrU,
                   pJShare2Isp->pitchUv * (pJShare2Isp->sliceCount << 3) / pJFrmComp->jFrmInfo[0].verticalSamp);
            // V
            pTmpBufAddr += (pJShare2Isp->pitchUv * (pJShare2Isp->sliceCount << 3) / pJFrmComp->jFrmInfo[0].verticalSamp);
            (void)memcpy(pTmpBufAddr, pJShare2Isp->addrV,
                   pJShare2Isp->pitchUv * (pJShare2Isp->sliceCount << 3) / pJFrmComp->jFrmInfo[0].verticalSamp);

            _jpg_reflash_vram(pHJCodec->pHInJStream, JPG_STREAM_CMD_GET_VRAM_ENC_YUV_BUF,
                              pJEncoder->pYuvEncBuf, yuvSrcBufSize, &pVramYuvBufAddr);

            pJLineBufInfo->comp1Addr    = (uint8_t *)pVramYuvBufAddr;
            pJLineBufInfo->comp2Addr    = (uint8_t *)(pJLineBufInfo->comp1Addr + (pJShare2Isp->pitchY * (pJShare2Isp->sliceCount << 3)));
            pJLineBufInfo->comp3Addr    = (uint8_t *)(pJLineBufInfo->comp2Addr + (pJShare2Isp->pitchUv * (pJShare2Isp->sliceCount << 3) / pJFrmComp->jFrmInfo[0].verticalSamp));
            (void)printf("line buf: y=0x%x,u=0x%x, v=0x%x\n", (uint32_t)pJLineBufInfo->comp1Addr, (uint32_t)pJLineBufInfo->comp2Addr, (uint32_t)pJLineBufInfo->comp3Addr);
        }

    #else
        pJLineBufInfo->comp1Addr    = (uint8_t *)pJShare2Isp->addrY;
        pJLineBufInfo->comp2Addr    = (uint8_t *)pJShare2Isp->addrU;
        pJLineBufInfo->comp3Addr    = (uint8_t *)pJShare2Isp->addrV;
    #endif

        pJLineBufInfo->comp1Pitch   = pJShare2Isp->pitchY;
        pJLineBufInfo->comp23Pitch  = pJShare2Isp->pitchUv;
        pJLineBufInfo->sliceNum     = pJShare2Isp->sliceCount;
    } while (false);

    if (result != JPG_ERR_OK)
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
    }

    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ENC);
    return result;
}

static JPG_ERR
jpg_enc_init (
    JPG_CODEC_HANDLE    * pHJCodec,
    void                * extraData)
{
    JPG_ERR         result          = JPG_ERR_OK;
    JPG_STREAM_DESC * pJStreamDesc  = &pHJCodec->pHInJStream->jStreamDesc;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ENC, "0x%x, 0x%x\n", pHJCodec, extraData);

    do
    {
        JPG_PowerUp();

        // ------------------------------
        // allocate memory
        if ( pJStreamDesc->jHeap_mem != NULL )
        {
            uint32_t                bsRingBufSize       = 0;
            JPG_BS_RING_BUF_INFO    * pJBsRingBufInfo   = NULL;

            // allocate private data
            pHJCodec->privData = pJStreamDesc->jHeap_mem(
                pHJCodec->pHInJStream, JPG_HEAP_DEF,
                sizeof(JPG_ENCODER), NULL);
            if ( pHJCodec->privData == NULL )
            {
                jpg_msg_ex(JPG_MSG_TYPE_ERR, " %s() Allocate Line buf fail !! ", __FUNCTION__);
                result = JPG_ERR_ALLOCATE_FAIL;
                break;
            }
            (void)memset(pHJCodec->privData, 0x0, sizeof(JPG_ENCODER));
            pJBsRingBufInfo = &((JPG_ENCODER *)pHJCodec->privData)->jBsRingBufInfo;

            // allocate jpg H/W bs ring buffer
            if ( pJStreamDesc->jControl != NULL )
            {
                (void)pJStreamDesc->jControl(pHJCodec->pHInJStream,
                                       (uint32_t)JPG_STREAM_CMD_GET_BS_RING_BUF_SIZE,
                                       &bsRingBufSize, NULL);
            }

            pJBsRingBufInfo->pBsRingBuf = pJStreamDesc->jHeap_mem(
                pHJCodec->pHInJStream, JPG_HEAP_ENC_BS_RING_BUF,
                bsRingBufSize, &pJBsRingBufInfo->bsRingBufLeng);
            if ( pJBsRingBufInfo->pBsRingBuf == NULL )
            {
                jpg_msg_ex(JPG_MSG_TYPE_ERR, " %s() Allocate bs ring buf fail !! ", __FUNCTION__);
                result = JPG_ERR_ALLOCATE_FAIL;
                break;
            }
        }
    } while (false);

    if (result != JPG_ERR_OK)
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
    }

    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ENC);
    return result;
}

static JPG_ERR
jpg_enc_deInit (
    JPG_CODEC_HANDLE    * pHJCodec,
    void                * extraData)
{
    JPG_ERR         result          = JPG_ERR_OK;
    JPG_STREAM_DESC * pJStreamDesc  = &pHJCodec->pHInJStream->jStreamDesc;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ENC, "0x%x, 0x%x\n", pHJCodec, extraData);

    JPG_EncPowerDown();

    if ( pJStreamDesc->jFree_mem != NULL )
    {
        if ( pHJCodec->privData != NULL )
        {
            JPG_BS_RING_BUF_INFO    * pJBsRingBufInfo   = NULL;
            uint8_t                 * pYuvEncBuf        = NULL;

            pJBsRingBufInfo = &((JPG_ENCODER *)pHJCodec->privData)->jBsRingBufInfo;

            // free H/W bs ring buffer
            if (pJBsRingBufInfo->pBsRingBuf != NULL)
            {
                pJStreamDesc->jFree_mem(pHJCodec->pHInJStream, JPG_HEAP_ENC_BS_RING_BUF, pJBsRingBufInfo->pBsRingBuf);
                pJBsRingBufInfo->pBsRingBuf = NULL;
            }

            // free yuv encoded src buf, win32 case
            pYuvEncBuf = ((JPG_ENCODER *)pHJCodec->privData)->pYuvEncBuf;
            if (pYuvEncBuf != NULL)
            {
                pJStreamDesc->jFree_mem(pHJCodec->pHInJStream, JPG_HEAP_ENC_YUV_BUF, pYuvEncBuf);
                ((JPG_ENCODER *)pHJCodec->privData)->pYuvEncBuf = NULL;
            }

            // free private data
            pJStreamDesc->jFree_mem(pHJCodec->pHInJStream, JPG_HEAP_DEF, pHJCodec->privData);
            pHJCodec->privData = NULL;
        }
    }

    if (result != JPG_ERR_OK)
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
    }

    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ENC);
    return result;
}

static JPG_ERR
jpg_enc_setup (
    JPG_CODEC_HANDLE    * pHJCodec,
    void                * extraData)
{
    JPG_ERR         result      = JPG_ERR_OK;
    JPG_ENCODER     * pJEncoder = (JPG_ENCODER *)pHJCodec->privData;
    JPG_HW_CTRL     * pJHwCtrl  = &pHJCodec->jHwCtrl;
    JPG_FRM_COMP    * pJFrmComp = &pHJCodec->jFrmCompInfo;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ENC, "0x%x, 0x%x\n", pHJCodec, extraData);

    // set JPG_COMMAND_TRIGGER mode
    pJEncoder->triggerMode  = JPG_COMMAND_TRIGGER;

    pJHwCtrl->codecCtrl     = ((uint16_t)JPG_OP_ENCODE | (uint16_t)pJEncoder->triggerMode);
    pJHwCtrl->codecCtrl     |= ((uint16_t)jpgCompCtrl[pJFrmComp->compNum] & (uint16_t)JPG_MSK_LINE_BUF_COMPONENT_VALID);

    do
    {
        JPG_HW_BS_CTRL  * pJHwBsCtrl        = &pHJCodec->jHwBsCtrl;
        uint8_t         * pVramBsBufAddr    = NULL;

        // set jpg interrupt
		#if 0
        _JPG_Enable_Interrupt(jpg_isr);
		#endif

        // set line buffer information (YUV)
        result = _JPG_Enc_Set_Line_buf(pHJCodec);
        if (result != JPG_ERR_OK)
        {
            break;
        }

        // set H/W bit-stream ring buffer
        pVramBsBufAddr = pJEncoder->jBsRingBufInfo.pBsRingBuf;
        // win32
        _jpg_reflash_vram(pHJCodec->pHInJStream, JPG_STREAM_CMD_GET_VRAM_ENC_RING_BUF, 0, 0, &pVramBsBufAddr);

    #ifdef CFG_CPU_WB
        ithFlushDCacheRange(&pVramBsBufAddr[0], pJEncoder->jBsRingBufInfo.bsRingBufLeng);
        ithFlushMemBuffer();
    #endif
        pJHwBsCtrl->addr    = pVramBsBufAddr;
        pJHwBsCtrl->size    = pJEncoder->jBsRingBufInfo.bsRingBufLeng;

        // set HW register
        (void)_JPG_Enc_HW_Update(pHJCodec);

        // ---------------------------------
        // start fire jpg
        JPG_LogReg(false);
        JPG_StartReg();
    } while (false);

    if (result != JPG_ERR_OK)
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
    }

    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ENC);
    return result;
}

static JPG_ERR
jpg_enc_fire (
    JPG_CODEC_HANDLE    * pHJCodec,
    void                * extraData)
{
    JPG_ERR result = JPG_ERR_OK;
    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ENC, "0x%x, 0x%x\n", pHJCodec, extraData);

    do
    {
        JPG_ENCODER         * pJEncoder = (JPG_ENCODER *)pHJCodec->privData;
        JPG_MULTI_SECT_INFO * pJMultiSectInfo = &pHJCodec->jMultiSectInfo;
        JPG_LINE_BUF_INFO   * pJLineBufInfo = &pHJCodec->jLineBufInfo;
        JPG_FRM_COMP        * pJFrmComp = &pHJCodec->jFrmCompInfo;
        uint32_t                 remainSliceNum = 0, procSliceNum = 0;
        bool                bEncIdle = false, bSkip = false;

        pJEncoder->jBsRingBufInfo.rwSize = 0;

        if (pJMultiSectInfo->bFirst == true)
        {
            pJMultiSectInfo->bFirst = false;
        }
        else
        {
            // update line buf info
            JPG_SetLineBufInfoReg(pJLineBufInfo);
        }

        pJEncoder->pSysBsBuf_Cur = pHJCodec->pSysBsBuf;

        // --------------------------------
        // Set slice number
        remainSliceNum = ((pJMultiSectInfo->section_hight + 7u) >> 3);
        JPG_SetLineBufSliceUnitReg(remainSliceNum, pJFrmComp->jFrmInfo[0].verticalSamp);    // Ex. 1280 x 1024 encoded frame and 422 has (1024/(1x8)) slice unit.

        // --------------------------------
        // command trigger (while loop not work now)
        while (remainSliceNum > 0u)
        {
            procSliceNum = remainSliceNum;

            // set line buffer r/w data size
            //(void)printf("(procSliceNum/pJFrmComp->jFrmInfo[0].verticalSamp)=0x%lx,pJFrmComp->jFrmInfo[0].verticalSamp=0x%lx,procSliceNum=0x%lx\n", (procSliceNum / pJFrmComp->jFrmInfo[0].verticalSamp), pJFrmComp->jFrmInfo[0].verticalSamp, procSliceNum);
            JPG_SetLineBufSliceWriteNumReg((JPG_REG)(procSliceNum / (uint32_t)pJFrmComp->jFrmInfo[0].verticalSamp));
            // set line buffer write end
            JPG_SetLineBufCtrlReg((JPG_REG)JPG_MSK_LINE_BUF_WRITE_END);

            remainSliceNum -= procSliceNum;

            // set last line buffer data flag
            if (pJMultiSectInfo->bFinished == true)
            {
                JPG_SetLineBufCtrlReg((JPG_REG)JPG_MSK_LAST_ENCODE_DATA);
            }
            //(void)usleep(10 * 1000);

            // check encode status (if want to work while loop)
            // 1. check H/W busy or idle => JPG_GetProcStatusReg()
            // 2. bs buffer full => need to move bit stream from bs buffer
            // 2.1. set bitstream buffer read size => JPG_SetBitstreamBufRwSizeReg();
            // 2.2. set bitstream buffer read end => JPG_SetBitstreamBufCtrlReg(JPEG_MSK_BITSTREAM_BUF_RW_END);
        }

        //(void)usleep(10 * 1000);

        // ----------------------------------------
        // Read valid data in bitstream ring buffer
        while (bEncIdle == false)
        {
            result = _JPG_Enc_Get_CodedData(pHJCodec, &bEncIdle);
            if (result != JPG_ERR_OK)
            {
                jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
                bSkip = true;
                break;
            }
            (void)usleep(1000);
        }

        if (bSkip == true)
        {
            break;
        }

        if (pJMultiSectInfo->bFinished == true)
        {
            // remove dummy bytes (0xFF) and add EOI marker
            pJEncoder->pSysBsBuf_Cur = _JPG_Enc_Clear_Dummy(pJEncoder->pSysBsBuf_Cur);
        }

        pHJCodec->sysValidBsBufSize = ((uint32_t)pJEncoder->pSysBsBuf_Cur - (uint32_t)pHJCodec->pSysBsBuf);
    } while (false);

    if (result != JPG_ERR_OK)
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
    }

    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ENC);
    return result;
}

static JPG_ERR
jpg_enc_ctrl (
    JPG_CODEC_HANDLE    * pHJCodec,
    uint32_t            cmd,
    uint32_t            * value,
    void                * extraData)
{
    JPG_ERR result = JPG_ERR_OK;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ENC, "0x%x, 0x%x, 0x%x, 0x%x\n", pHJCodec, cmd, value, extraData);

    switch (cmd)
    {
        default:
            result = JPG_ERR_NO_IMPLEMENT;
            break;
    }

    if ((result != JPG_ERR_OK) &&
        (result != JPG_ERR_NO_IMPLEMENT))
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
    }

    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ENC);
    return result;
}
// =============================================================================
//                  Public Function Definition
// =============================================================================
JPG_CODEC_DESC JPG_CODEC_DESC_encoder_desc =
{
    "signal jpg enc",       
    NULL,                   
    JPG_CODEC_ENC_JPG,      
    jpg_enc_init,           
    jpg_enc_deInit,         
    jpg_enc_setup,          
    jpg_enc_fire,           
    jpg_enc_ctrl,           
};
#else

JPG_CODEC_DESC JPG_CODEC_DESC_encoder_desc = {0};
#endif
