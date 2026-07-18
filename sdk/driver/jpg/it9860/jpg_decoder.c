#include <stdbool.h>
#include "jpg_defs.h"
#include "jpg_codec.h"
#include "jpg_extern_link.h"
#include "jpg_hw.h"
#include "jpg_common.h"

//=============================================================================
//                  Constant Definition
//=============================================================================

//=============================================================================
//                  Macro Definition
//=============================================================================

//=============================================================================
//                  Structure Definition
//=============================================================================
typedef struct JPG_DECODER_TAG
{
    JPG_TRIGGER_MODE triggerMode;

    uint8_t          *pSysBsStart;
    int              vramBsSize;
    uint8_t          *vramBsAddr;
    uint32_t         vramRwSize;
} JPG_DECODER;
//=============================================================================
//                  Global Data Definition
//=============================================================================
extern uint16_t PITCH_ARR[6];

//=============================================================================
//                  Private Function Definition
//=============================================================================
static JPG_ERR
_JPG_HW_Update(
    JPG_CODEC_HANDLE *pHJCodec)
{
    JPG_ERR result = JPG_ERR_OK;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_DEC, "0x%x\n", pHJCodec);

    do
    {
        JPG_FRM_COMP      *pJFrmComp     = &pHJCodec->jFrmCompInfo;
        JPG_HW_CTRL       *pJHwCtrl      = &pHJCodec->jHwCtrl;
        JPG_LINE_BUF_INFO *pJLineBufInfo = &pHJCodec->jLineBufInfo;
        JPG_FRM_SIZE_INFO *pJFrmSizeInfo = &pHJCodec->jFrmSizeInfo;

        JPG_SetCodecCtrlReg(pJHwCtrl->codecCtrl);

        JPG_SetDriReg(pJFrmComp->restartInterval);

        JPG_SetTableSpecifyReg(pJFrmComp);

        JPG_SetFrmSizeInfoReg(pJFrmSizeInfo);

        JPG_SetLineBufInfoReg(pJLineBufInfo);

        JPG_SetLineBufSliceUnitReg(pJLineBufInfo->sliceNum, pJFrmComp->jFrmInfo[0].verticalSamp);

        // set pJHwBsCtrl
        // move out, for jpg prograssive

        JPG_SetSamplingFactorReg(pJFrmComp);

        // set Q table
        JPG_SetQtableReg(pJFrmComp);

        // set huffman table
        JPG_SetHuffmanCodeCtrlReg(JPG_HUUFFMAN_Y_DC,  pJHwCtrl->dcHuffTable[0]);
        JPG_SetHuffmanCodeCtrlReg(JPG_HUUFFMAN_UV_DC, pJHwCtrl->dcHuffTable[1]);
        JPG_SetHuffmanCodeCtrlReg(JPG_HUUFFMAN_Y_AC,  pJHwCtrl->acHuffTable[0]);
        JPG_SetHuffmanCodeCtrlReg(JPG_HUUFFMAN_UV_AC, pJHwCtrl->acHuffTable[1]);

        JPG_SetDcHuffmanValueReg(JPG_HUUFFMAN_Y_DC,
                                 (pJHwCtrl->dcHuffTable[0] + 16),
                                 pJHwCtrl->dcHuffW2talCodeLenCnt[0]);

        JPG_SetDcHuffmanValueReg(JPG_HUUFFMAN_UV_DC,
                                 (pJHwCtrl->dcHuffTable[1] + 16),
                                 pJHwCtrl->dcHuffW2talCodeLenCnt[1]);

        JPG_SetDecodeAcHuffmanValueReg(JPG_HUUFFMAN_Y_AC,
                                       (pJHwCtrl->acHuffTable[0] + 16),
                                       pJHwCtrl->acHuffW2talCodeLenCnt[0]);

        JPG_SetDecodeAcHuffmanValueReg(JPG_HUUFFMAN_UV_AC,
                                       (pJHwCtrl->acHuffTable[1] + 16),
                                       pJHwCtrl->acHuffW2talCodeLenCnt[1]);

        JPG_DropHv((JPG_REG)(pHJCodec->ctrlFlag >> 16));

		#if 0
        JPG_SetTilingTable(pJLineBufInfo, 0, 1);
		#endif
    } while (false);

    if (result != JPG_ERR_OK)
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
    }
    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_DEC);
    return result;
}

static JPG_ERR
_JPG_HW_Wait_Idle(
    uint32_t timeOutCnt)
{
    JPG_ERR  result = JPG_ERR_OK;
    JPG_REG  bufStatus = 0, hwStatus = 0;
    uint32_t timeOut = 0;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_DEC, "%d\n", timeOutCnt);

    bufStatus = JPG_GetProcStatusReg();
    while ( (bufStatus & (JPG_REG)JPG_STATUS_DECODE_COMPLETE) == 0u )
    {
        if ( (bufStatus & (JPG_REG)JPG_STATUS_DECODE_ERROR) != 0u )
        {
            result = JPG_ERR_DECODE_IRQ;
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg HW err !! ");
            JPG_LogReg(true);
            break;
        }

        hwStatus = JPG_GetEngineStatusReg();
        if ((hwStatus & 0xF800u) == 0x3000u)
        {
            (void)usleep(1000);
            hwStatus = JPG_GetEngineStatusReg();
            if ((hwStatus & 0xF800u) == 0x3000u)
            {
                result = JPG_ERR_BUSY_TIMEOUT;
                jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg HW err !! ");
                JPG_LogReg(true);
                break;
            }
        }

        (void)usleep(1000);

        timeOut++;
        if (timeOut > timeOutCnt)
        {
            result = JPG_ERR_BUSY_TIMEOUT;
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg HW err (bufStatus=0x%lx, hwStatus= 0x%lx)!! ", bufStatus, hwStatus);
            JPG_LogReg(true);
            break;
        }

        bufStatus = JPG_GetProcStatusReg();
    }

    if (result != JPG_ERR_OK)
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
    }
    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_DEC);
    return result;
}

static JPG_ERR
_JPG_Wait_Bs_Buf_Empty(
    uint32_t timeOutCnt)
{
    JPG_ERR  result    = JPG_ERR_OK;
    JPG_REG  bufStatus = 0;
    uint32_t timeOut   = 0;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_DEC, "%d\n", timeOutCnt);

    bufStatus = JPG_GetProcStatusReg();
    while ( (bufStatus & ((JPG_REG)JPG_STATUS_BITSTREAM_BUF_EMPTY | (JPG_REG)JPG_STATUS_DECODE_COMPLETE)) == 0u )
    {
        if ( (bufStatus & (JPG_REG)JPG_STATUS_DECODE_ERROR) != 0u )
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg err ! bufStatus = 0x%lx\n", bufStatus);
            JPG_LogReg(true);
            result = JPG_ERR_DECODE_IRQ;
            break;
        }

        (void)usleep(1000);

        timeOut++;
        if (timeOut > timeOutCnt)
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg timeout ! bufStatus = 0x%lx\n", bufStatus);
            JPG_LogReg(true);
            result = JPG_ERR_BS_CONSUME_TIMEOUT;
            break;
        }

        bufStatus = JPG_GetProcStatusReg();
    }

    if (result != JPG_ERR_OK)
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
    }
    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_DEC);
    return result;
}

static JPG_ERR
_JPG_Set_Less_Line_Buf(
    JPG_CODEC_HANDLE *pHJCodec)
{
    JPG_ERR result = JPG_ERR_OK;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_DEC, "0x%x\n", pHJCodec);

    do
    {
        JPG_LINE_BUF_INFO *pJLineBufInfo = &pHJCodec->jLineBufInfo;
        JPG_FRM_SIZE_INFO *pJFrmSizeInfo = &pHJCodec->jFrmSizeInfo;
        JPG_FRM_COMP      *pJFrmComp     = &pHJCodec->jFrmCompInfo;
        uint32_t          real_w         = pJFrmSizeInfo->realWidth;
        uint32_t          real_h         = pJFrmSizeInfo->realHeight;
        uint32_t          tmpSliceNum    = 0;
        uint32_t          heightFactor   = 1;
        bool              bSkip          = false;

        if ( (pHJCodec->ctrlFlag & (uint32_t)JPG_FLAGS_DEC_DC_ONLY) != 0u )
        {
            real_w >>= 3;
            real_h >>= 3;
        }
        else if ( (pHJCodec->ctrlFlag & (uint32_t)JPG_FLAGS_DEC_Y_HOR_DOWNSAMPLE) != 0u )
        {
            real_w >>= 1;
        }
		else
		{
			; /*Do nothing*/
		}

        {
            pJLineBufInfo->comp1Pitch = (uint16_t)((real_w + 0x7u) & ~0x7u);
            if (pJLineBufInfo->comp1Pitch < 2048u) pJLineBufInfo->comp1Pitch = 2048u;

            // long time ago, Isp and Jpg H/W handshaking bug.
            // Now, it should be fixed but we lazily modify.
            tmpSliceNum = pJLineBufInfo->size / ((uint32_t)pJLineBufInfo->comp1Pitch << 2);

            switch (pJFrmComp->decColorSpace)
            {
            case JPG_COLOR_SPACE_YUV444:
                pJLineBufInfo->comp23Pitch = pJLineBufInfo->comp1Pitch;
                heightFactor               = 1u;
                pJLineBufInfo->sliceNum    = (uint16_t)((tmpSliceNum / 6u) & ~0x1u);
                break;

            case JPG_COLOR_SPACE_YUV411:    // 411 will transfer to 422 for ISP
            case JPG_COLOR_SPACE_YUV422:
                pJLineBufInfo->comp23Pitch = pJLineBufInfo->comp1Pitch >> 1;
                heightFactor               = 1;
                pJLineBufInfo->sliceNum    = (uint16_t)((tmpSliceNum >> 2) & ~0x1u);
                break;

            case JPG_COLOR_SPACE_YUV420:
                pJLineBufInfo->comp23Pitch = pJLineBufInfo->comp1Pitch >> 1;
                heightFactor               = 2;
                pJLineBufInfo->sliceNum    = (uint16_t)((tmpSliceNum / 3u) & ~0x1u);
                break;

            case JPG_COLOR_SPACE_YUV422R:
                pJLineBufInfo->comp23Pitch = pJLineBufInfo->comp1Pitch;
                heightFactor               = 2;
                pJLineBufInfo->sliceNum    = (uint16_t)((tmpSliceNum >> 2) & ~0x1u);
                break;

            default:
                jpg_msg_ex(JPG_MSG_TYPE_ERR, "Wrong color format (0x%x)!! ", pJFrmComp->decColorSpace);
                result = JPG_ERR_INVALID_PARAMETER;
                bSkip  = true;
                break;
            }

            if (bSkip == true) 
            {
				break;
            }

            pJLineBufInfo->sliceNum       = (pJLineBufInfo->sliceNum > 100u) ? 100u : pJLineBufInfo->sliceNum;

            pJLineBufInfo->ySliceByteSize = ((uint32_t)pJLineBufInfo->comp1Pitch << 3);
            pJLineBufInfo->uSliceByteSize = ((uint32_t)pJLineBufInfo->comp23Pitch << 3) / heightFactor;
            pJLineBufInfo->vSliceByteSize = pJLineBufInfo->uSliceByteSize;

            // comp1Addr should be the same with addrAlloc
            pJLineBufInfo->comp1Addr      = (uint8_t *)(((uint32_t)pJLineBufInfo->addrAlloc + 3u) & ~3u);

            // win32
            _jpg_reflash_vram(pHJCodec->pHInJStream,
                              JPG_STREAM_CMD_GET_VRAM_LINE_BUF,
                              0, 0, &pJLineBufInfo->comp1Addr);

            pJLineBufInfo->comp2Addr = pJLineBufInfo->comp1Addr + (pJLineBufInfo->sliceNum * pJLineBufInfo->ySliceByteSize);
            pJLineBufInfo->comp3Addr = pJLineBufInfo->comp2Addr + (pJLineBufInfo->sliceNum * pJLineBufInfo->uSliceByteSize);
        }

        if (pJFrmComp->bSingleChannel == true)
        {
            // Single channel will be processed with YUV444 by H/W
            uint8_t           wrColor = 0x80;
            JPG_MEM_MOVE_INFO memInfo = {0};
            uint32_t          i, j, size;

            memInfo.sizeByByte = 1u;

            size               = (pJLineBufInfo->sliceNum * pJLineBufInfo->uSliceByteSize) >> 3;

            // ??? we should do so stupid flow ???
            for (j = 0; j < 8u; j++)
            {
                for (i = 0; i < size; i++)
                {
                    memInfo.dstAddr = (uint32_t)pJLineBufInfo->comp2Addr + i + (j * size);
                    memInfo.srcAddr = (uint32_t)(&wrColor);
                    (void)Jpg_Ext_Link_Ctrl(JEL_CTRL_HOST_W_MEM, NULL, (void *)&memInfo);
                    (void)usleep(1000);
                }
            }

            for (j = 0; j < 8u; j++)
            {
                for (i = 0; i < size; i++)
                {
                    memInfo.dstAddr = (uint32_t)pJLineBufInfo->comp3Addr + i + (j * size);
                    memInfo.srcAddr = (uint32_t)(&wrColor);
                    (void)Jpg_Ext_Link_Ctrl(JEL_CTRL_HOST_W_MEM, NULL, (void *)&memInfo);
                    (void)usleep(1000);
                }
            }
        }
    } while (false);

    if (result != JPG_ERR_OK)
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
    }
    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_DEC);
    return result;
}
#if 0
static JPG_ERR
_JPG_Set_Full_Line_Buf(
    JPG_CODEC_HANDLE *pHJCodec)
{
    JPG_ERR result = JPG_ERR_OK;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_DEC, "0x%x\n", pHJCodec);

    do
    {
        JPG_LINE_BUF_INFO *pJLineBufInfo = &pHJCodec->jLineBufInfo;
        JPG_FRM_SIZE_INFO *pJFrmSizeInfo = &pHJCodec->jFrmSizeInfo;
        JPG_FRM_COMP      *pJFrmComp     = &pHJCodec->jFrmCompInfo;
        uint32_t          real_w         = pJFrmSizeInfo->realWidth;
        uint32_t          real_h         = pJFrmSizeInfo->realHeight;

        pJLineBufInfo->comp1Pitch = (real_w + 0x7) & ~0x7;
        switch (pJFrmComp->decColorSpace)
        {
        case JPG_COLOR_SPACE_YUV444:
            pJLineBufInfo->comp23Pitch = pJLineBufInfo->comp1Pitch;
            pJLineBufInfo->sliceNum    = ((real_h + 0x7) >> 3);
            break;

        case JPG_COLOR_SPACE_YUV411:     // 411 will transfer to 422 for ISP
        case JPG_COLOR_SPACE_YUV422:
            pJLineBufInfo->comp23Pitch = pJLineBufInfo->comp1Pitch >> 1;
            pJLineBufInfo->sliceNum    = ((real_h + 0x7) >> 3);
            break;

        case JPG_COLOR_SPACE_YUV420:
            pJLineBufInfo->comp23Pitch = pJLineBufInfo->comp1Pitch >> 1;
            pJLineBufInfo->sliceNum    = ((real_h + 0x7) >> 3);
            break;

        case JPG_COLOR_SPACE_YUV422R:
            pJLineBufInfo->comp23Pitch = pJLineBufInfo->comp1Pitch;
            pJLineBufInfo->sliceNum    = ((real_h + 0x7) >> 3);
            break;

        default:
            jpg_msg_ex(JPG_MSG_TYPE_ERR, "Wrong color format (0x%x)!! ", pJFrmComp->decColorSpace);
            result = JPG_ERR_INVALID_PARAMETER;
            break;
        }

        pJLineBufInfo->comp1Addr = (uint8_t *)(((uint32_t)pJLineBufInfo->addrAlloc + 3) & ~3);
        pJLineBufInfo->comp2Addr = pJLineBufInfo->comp1Addr + (pJLineBufInfo->sliceNum * 8 * pJLineBufInfo->comp1Pitch);
        pJLineBufInfo->comp3Addr = pJLineBufInfo->comp2Addr + (pJLineBufInfo->sliceNum * 8 * pJLineBufInfo->comp23Pitch);

        if ((pJLineBufInfo->comp3Addr + (pJLineBufInfo->sliceNum * 8 * pJLineBufInfo->comp23Pitch)) >
            (pJLineBufInfo->addrAlloc + pJLineBufInfo->size))
        {
            result = JPG_ERR_INVALID_PARAMETER;
            jpg_msg_ex(JPG_MSG_TYPE_ERR, "  line buf out range !! ");
            break;
        }

#if 0
        printf("lineBuf: y=0x%x, u=0x%x, v=0x%x, pitch_y=%d, pitch_uv=%d, height=%d\n",
               (uint32_t)pJLineBufInfo->comp1Addr, (uint32_t)pJLineBufInfo->comp2Addr, (uint32_t)pJLineBufInfo->comp3Addr,
               pJLineBufInfo->comp1Pitch, pJLineBufInfo->comp23Pitch,
               (pJLineBufInfo->sliceNum << 3));
#endif
    } while (false);

    if (result != JPG_ERR_OK)
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);

    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_DEC);
    return result;
}
#endif
static JPG_ERR
_JPG_Set_Line_Buf_Info(
    JPG_CODEC_HANDLE *pHJCodec)
{
    JPG_ERR result = JPG_ERR_OK;

    // _jpg_trace_enter(JPG_MSG_TYPE_TRACE_DEC, "0x%x, %d, %d\n", pHJCodec, *actBsSize, *bEnd);
    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_DEC, "0x%x, %d, %d\n", pHJCodec);

#if  0 //(ENABLE_JDEBUG_MODE)
       // for debug
    result = _JPG_Set_Full_Line_Buf(pHJCodec);
#else
    result = _JPG_Set_Less_Line_Buf(pHJCodec);
#endif

    if (result != JPG_ERR_OK)
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
    }
    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_DEC);
    return result;
}

static JPG_ERR
_JPG_Set_CodedData(
    JPG_CODEC_HANDLE *pHJCodec,
    uint32_t         *actBsSize,
    bool             *bEnd)
{
    JPG_ERR result = JPG_ERR_OK;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_DEC, "0x%x, %d, %d\n", pHJCodec, *actBsSize, *bEnd);

    do
    {
        JPG_HW_BS_CTRL *pJHwBsCtrl = &pHJCodec->jHwBsCtrl;
        JPG_FRM_COMP   *pJFrmComp  = &pHJCodec->jFrmCompInfo;
        JPG_REG        bufStatus   = 0;

        *bEnd     = false;

        bufStatus = JPG_GetProcStatusReg();
        if ( (bufStatus & (JPG_REG)JPG_STATUS_DECODE_COMPLETE) != 0u )
        {
            *bEnd = true;
        }
        result = _JPG_Wait_Bs_Buf_Empty(JPG_DEC_TIMEOUT_COUNT);
        if (result != JPG_ERR_OK)
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg err !!");
            break;
        }

        if ( (pHJCodec->bLastSection == true) && (pJFrmComp->bProgressive == false) )
        {
            /*Why needs to rewrite the end of inputBuffer of endMarker(0xFFD9) again, I think it doesn`t necessary, Benson,2016/0315 */
			#if 0
            *(pJHwBsCtrl->preBsBuf + (*actBsSize) - 2) = 0xFF;
            *(pJHwBsCtrl->preBsBuf + (*actBsSize) - 1) = 0xD9;
			#endif
        }

        // set bs buffer addr and length
        pJHwBsCtrl->preBsBuf = pHJCodec->pSysBsBuf;
        pJHwBsCtrl->addr     = pHJCodec->pSysBsBuf;
        pJHwBsCtrl->size     = (*actBsSize);
        JPG_SetBitStreamBufInfoReg(pJHwBsCtrl);

        // set bs buffer read size
        JPG_SetBitstreamBufRwSizeReg((*actBsSize));

        // set bs buffer write end
        JPG_SetBitstreamBufCtrlReg((JPG_REG)JPG_MSK_BITSTREAM_BUF_RW_END);
        *actBsSize = 0;
        if (pHJCodec->bLastSection == true)
        {
            JPG_SetBitstreamBufCtrlReg((JPG_REG)JPG_MSK_LAST_BITSTREAM_DATA);
            *bEnd = true;
        }

        bufStatus = JPG_GetProcStatusReg();
        if ( (bufStatus & (JPG_REG)JPG_STATUS_DECODE_ERROR) != 0u )
        {
            result = JPG_ERR_DECODE_IRQ;
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg HW err !! ");
            break;
        }
    } while (false);

    if (result != JPG_ERR_OK)
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
    }
    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_DEC);
    return result;
}

#ifdef _MSC_VER
static JPG_ERR
_JPG_Set_CodedDataToVram(
    JPG_CODEC_HANDLE *pHJCodec,
    uint32_t         *actBsSize,
    bool             *bEnd)
{
    JPG_ERR result = JPG_ERR_OK;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_DEC, "0x%x, %d, %d\n", pHJCodec, *actBsSize, *bEnd);

    do
    {
        JPG_HW_BS_CTRL *pJHwBsCtrl = &pHJCodec->jHwBsCtrl;
        JPG_DECODER    *pJDecoder  = (JPG_DECODER *)pHJCodec->privData;
        JPG_REG        bufStatus   = 0;
        uint32_t       bsCopySize  = 0;

        *bEnd     = false;

        bufStatus = JPG_GetProcStatusReg();
        if ( (bufStatus & (JPG_REG)JPG_STATUS_DECODE_COMPLETE) != 0u )
        {
            *bEnd = true;
        }
        result = _JPG_Wait_Bs_Buf_Empty(JPG_DEC_TIMEOUT_COUNT);
        if (result != JPG_ERR_OK)
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg err !!");
            break;
        }

        bsCopySize = (pJDecoder->vramBsSize - JPG_GetBitStreamValidSizeReg());
        bsCopySize = (bsCopySize < (*actBsSize)) ? bsCopySize : (*actBsSize); //  check 1-st or n-th hanlde at the current section

        // move to vram
        do
        {
            uint32_t          resiBSBufSize = 0;
            uint32_t          ringBSBufSize = 0;
            uint8_t           *pVramBsCur   = pJDecoder->vramBsAddr + pJDecoder->vramRwSize;
            uint8_t           *pSysBsCurr   = pJDecoder->pSysBsStart; //pJHwBsCtrl->addr;
            JPG_MEM_MOVE_INFO jMemInfo      = {0};

            resiBSBufSize = pJDecoder->vramBsSize - pJDecoder->vramRwSize;

            if (bsCopySize > resiBSBufSize)
            {
                ringBSBufSize = bsCopySize - resiBSBufSize;
            }

            if (ringBSBufSize > 0)
            {
                // move data
                jMemInfo.dstAddr    = (uint32_t)pVramBsCur;
                jMemInfo.srcAddr    = (uint32_t)pSysBsCurr;
                jMemInfo.sizeByByte = resiBSBufSize;
                (void)Jpg_Ext_Link_Ctrl(JEL_CTRL_HOST_W_MEM, NULL, (void *)&jMemInfo);

                // ---------------------
                pVramBsCur          = pJDecoder->vramBsAddr;
                jMemInfo.dstAddr    = (uint32_t)pVramBsCur;
                jMemInfo.srcAddr    = (uint32_t)(pSysBsCurr + resiBSBufSize);
                jMemInfo.sizeByByte = ringBSBufSize;
                (void)Jpg_Ext_Link_Ctrl(JEL_CTRL_HOST_W_MEM, NULL, (void *)&jMemInfo);

                // ---------------------
                pJDecoder->vramRwSize = ringBSBufSize;
                pSysBsCurr           += ringBSBufSize;
            }
            else
            {
                // move data
                jMemInfo.dstAddr    = (uint32_t)pVramBsCur;
                jMemInfo.srcAddr    = (uint32_t)pSysBsCurr;
                jMemInfo.sizeByByte = resiBSBufSize;
                (void)Jpg_Ext_Link_Ctrl(JEL_CTRL_HOST_W_MEM, NULL, (void *)&jMemInfo);

                // ---------------------
                // update SW bitstream buffer write point
                pJDecoder->vramRwSize = ((pJDecoder->vramRwSize + bsCopySize) % (pJDecoder->vramBsSize));
                pSysBsCurr           += bsCopySize;
            }

            // update jpg stream point
            pJHwBsCtrl->addr = pSysBsCurr;

            // set bitstream buffer read size
            JPG_SetBitstreamBufRwSizeReg(bsCopySize);

            // set bitstream buffer write end
            JPG_SetBitstreamBufCtrlReg((JPG_REG)JPG_MSK_BITSTREAM_BUF_RW_END);
        } while (false);

        //--------------------------------------
        // check status
        *actBsSize -= bsCopySize;
        if ( ((*actBsSize) == 0) && (pHJCodec->bLastSection == true) )
        {
            JPG_SetBitstreamBufCtrlReg((JPG_REG)JPG_MSK_LAST_BITSTREAM_DATA);
            *bEnd = true;
            break;
        }

        bufStatus = JPG_GetProcStatusReg();
        if ( (bufStatus & (JPG_REG)JPG_STATUS_DECODE_ERROR) != 0u )
        {
            result = JPG_ERR_DECODE_IRQ;
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg HW err !! ");
            break;
        }
    } while (false);

    if (result != JPG_ERR_OK)
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
    }
    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_DEC);
    return result;
}
#endif

static JPG_ERR
jpg_dec_init(
    JPG_CODEC_HANDLE *pHJCodec,
    void             *extraData)
{
    JPG_ERR result = JPG_ERR_OK;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_DEC, "0x%x, 0x%x\n", pHJCodec, extraData);

    do
    {
        JPG_STREAM_DESC *pJStreamDesc = &pHJCodec->pHInJStream->jStreamDesc;

        // a workaround for entering JPEG handshaking. (VP clk divider from 3 to 8)
        if (ithGetRevisionId() == 0)
        {
            ithWriteRegMaskA(0xd8000030, 0x808, 0x80f);
        }
        // isp init
        result = Jpg_Ext_Link_Ctrl(JEL_CTRL_ISP_INIT, (uint32_t *)JEL_SET_ISP_SHOW_IMAGE, NULL);
        if (result)
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, "Isp init err 0x%x !!", result);
            break;
        }

        JPG_PowerUp();

        if (pJStreamDesc->jHeap_mem != NULL)
        {
            // allocate private data
            pHJCodec->privData = pJStreamDesc->jHeap_mem(
                pHJCodec->pHInJStream, JPG_HEAP_DEF,
                sizeof(JPG_DECODER), NULL);
            if (pHJCodec->privData == NULL)
            {
                jpg_msg_ex(JPG_MSG_TYPE_ERR, " %s() Allocate Line buf fail !! ", __FUNCTION__);
                result = JPG_ERR_ALLOCATE_FAIL;
                break;
            }

            (void)memset(pHJCodec->privData, 0x0, sizeof(JPG_DECODER));
        }
    } while (false);

    if (result != JPG_ERR_OK)
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
    }
    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_DEC);
    return result;
}

static JPG_ERR
jpg_dec_deInit(
    JPG_CODEC_HANDLE *pHJCodec,
    void             *extraData)
{
    JPG_ERR           result         = JPG_ERR_OK;
    JPG_STREAM_DESC   *pJStreamDesc  = &pHJCodec->pHInJStream->jStreamDesc;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_DEC, "0x%x, 0x%x\n", pHJCodec, extraData);

    // a workaround for exiting JPEG handshaking. (VP clk divider from 8 to 3)
    if (ithGetRevisionId() == 0)
    {
        ithWriteRegMaskA(0xd8000030, 0x803, 0x80f);
    }
    result = JPG_DecPowerDown();

#if 0     // isp end  , Benson mark it.
    result = Jpg_Ext_Link_Ctrl(JEL_CTRL_ISP_TERMINATE, 0, 0);
    if (result != JPG_ERR_OK)
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "Isp init err 0x%x !!", result);
    }
#endif

    if (pJStreamDesc->jFree_mem != NULL)
    {
        // free private data
        if (pHJCodec->privData != NULL)
        {
            pJStreamDesc->jFree_mem(pHJCodec->pHInJStream, JPG_HEAP_DEF, pHJCodec->privData);
            pHJCodec->privData = NULL;
        }
    }

    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_DEC);
    return result;
}

static JPG_ERR
jpg_dec_setup(
    JPG_CODEC_HANDLE *pHJCodec,
    void             *extraData)
{
    JPG_ERR result = JPG_ERR_OK;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_DEC, "0x%x, 0x%x\n", pHJCodec, extraData);

    do
    {
        JCOMM_HANDLE      *pHJComm       = (JCOMM_HANDLE *)pHJCodec->pHJComm;
        JPG_DECODER       *pJDecoder     = (JPG_DECODER *)pHJCodec->privData;
        JPG_FRM_COMP      *pJFrmComp     = &pHJCodec->jFrmCompInfo;
        JPG_HW_CTRL       *pJHwCtrl      = &pHJCodec->jHwCtrl;
        JPG_SHARE_DATA    *pJShare2Isp   = &pHJCodec->jShare2Isp;
        JPG_FRM_SIZE_INFO *pJFrmSizeInfo = &pHJCodec->jFrmSizeInfo;
        JPG_LINE_BUF_INFO *pJLineBufInfo = &pHJCodec->jLineBufInfo;
        JPG_HW_BS_CTRL    *pJHwBsCtrl    = &pHJCodec->jHwBsCtrl;

        //---------------------------------
        // set ISP engine
        // set display info and output buffer info
        result = Jpg_Ext_Link_Ctrl(JEL_CTRL_ISP_SET_DISP_INFO, NULL, (void *)pHJCodec->pHJComm);
        if (result != JPG_ERR_OK)
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " err, 0x%x !!", result);
            break;
        }

        // set line buffer info
        result = _JPG_Set_Line_Buf_Info(pHJCodec);
        if (result != JPG_ERR_OK)
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " err, 0x%x !!", result);
            break;
        }

        // set JPG_ISP_TRIGGER mode
        pJDecoder->triggerMode = JPG_ISP_TRIGGER;

        pJHwBsCtrl->addr       = pHJCodec->pSysBsBuf;
        pJHwBsCtrl->size       = pHJCodec->sysBsBufSize;

        //---------------------------------
        // HW control setting
        if (pJFrmComp->bProgressive == true)
        {
            pJHwCtrl->codecCtrl = ((uint16_t)JPG_OP_DECODE_PROGRESSIVE | (uint16_t)pJDecoder->triggerMode);
        }
        else if ( (pHJCodec->ctrlFlag & (uint32_t)JPG_FLAGS_DEC_DC_ONLY) != 0u)
        {
            pJHwCtrl->codecCtrl = ((uint16_t)JPG_OP_DECODE_DC | (uint16_t)pJDecoder->triggerMode | (uint16_t)JPG_DC_MODE_HW_FLAG);
        }
        else
        {
            pJHwCtrl->codecCtrl = ((uint16_t)JPG_OP_DECODE | (uint16_t)pJDecoder->triggerMode);
        }

        if (pHJComm->pJDispInfo->bCMYK == true)
        {
            pJFrmComp->validComp &= ~(uint16_t)JPG_MSK_DEC_COMPONENT_D_VALID;
            pJFrmComp->compNum    = 3;
            pJHwCtrl->codecCtrl  |= (pJFrmComp->validComp & (uint16_t)JPG_MSK_DEC_COMPONENT_VALID);
        }
        else
        {
            pJHwCtrl->codecCtrl |= (pJFrmComp->validComp & (uint16_t)JPG_MSK_DEC_COMPONENT_VALID);
        }
        pJHwCtrl->codecCtrl               |= ((uint16_t)jpgCompCtrl[pJFrmComp->compNum] & (uint16_t)JPG_MSK_LINE_BUF_COMPONENT_VALID);
#if 0
        // Y: huffman table
        pJHwCtrl->dcHuffTable[0]           = pJFrmComp->huff_DC[0].pHuffmanTable;
        pJHwCtrl->acHuffTable[0]           = pJFrmComp->huff_AC[0].pHuffmanTable;
        pJHwCtrl->dcHuffW2talCodeLenCnt[0] = pJFrmComp->huff_DC[0].totalCodeLenCnt;
        pJHwCtrl->acHuffW2talCodeLenCnt[0] = pJFrmComp->huff_AC[0].totalCodeLenCnt;

        // UV: huffman table
        pJHwCtrl->dcHuffTable[1]           = pJFrmComp->huff_DC[1].pHuffmanTable;
        pJHwCtrl->acHuffTable[1]           = pJFrmComp->huff_AC[1].pHuffmanTable;
        pJHwCtrl->dcHuffW2talCodeLenCnt[1] = pJFrmComp->huff_DC[1].totalCodeLenCnt;
        pJHwCtrl->acHuffW2talCodeLenCnt[1] = pJFrmComp->huff_AC[1].totalCodeLenCnt;

        // Q table
        pJHwCtrl->qTableY                  = pJFrmComp->qTable.table[pJFrmComp->jFrmInfo[0].qTableSel];
        if (pJFrmComp->qTable.tableCnt == 1)
        {
            // only one Q table case
            pJHwCtrl->qTableUv         = pJFrmComp->qTable.table[pJFrmComp->jFrmInfo[0].qTableSel];
            pJFrmComp->qTable.tableCnt = 2;
        }
        else
            pJHwCtrl->qTableUv = pJFrmComp->qTable.table[pJFrmComp->jFrmInfo[1].qTableSel];
#endif
        //---------------------------------
        // set HW register (update H/W register)
        (void)_JPG_HW_Update(pHJCodec);

        //---------------------------------
        // store buffer information with isp_share_data
        if ( (pHJCodec->ctrlFlag & (uint32_t)JPG_FLAGS_DEC_DC_ONLY) != 0u)
        {
            pJShare2Isp->width  = (pJFrmSizeInfo->dispWidth >> 3);
            pJShare2Isp->height = (pJFrmSizeInfo->dispHeight >> 3);
        }
        else if ( (pHJCodec->ctrlFlag & (uint32_t)JPG_FLAGS_DEC_Y_HOR_DOWNSAMPLE) != 0u)
        {
            pJShare2Isp->width  = (pJFrmSizeInfo->dispWidth >> 1);
            pJShare2Isp->height = pJFrmSizeInfo->dispHeight;
        }
        else
        {
            pJShare2Isp->width  = pJFrmSizeInfo->dispWidth;
            pJShare2Isp->height = pJFrmSizeInfo->dispHeight;
        }

        pJShare2Isp->addrY      = (uint32_t)pJLineBufInfo->comp1Addr;
        pJShare2Isp->addrU      = (uint32_t)pJLineBufInfo->comp2Addr;
        pJShare2Isp->addrV      = (uint32_t)pJLineBufInfo->comp3Addr;
        pJShare2Isp->pitchY     = pJLineBufInfo->comp1Pitch;
        pJShare2Isp->pitchUv    = pJLineBufInfo->comp23Pitch;
        pJShare2Isp->sliceCount = pJLineBufInfo->sliceNum;
        pJShare2Isp->bCMYK      = pHJComm->pJDispInfo->bCMYK;
        pJShare2Isp->colorSpace = pJFrmComp->decColorSpace;

        //---------------------------------
        // bs info
        if (pJFrmComp->bProgressive == true)
        {
            // not ready
            #if 0
            _JPG_Set_Bs_Buf_Info(jBsBufInfo, jBsBufInfo->addr, 128*1024);
            JPG_SetBitstreamReadBytePosReg(0);
			#endif
        }
        else
        {
            uint8_t  *tmpAddr     = 0;
            uint32_t tmpSize      = 0;
            JPG_REG  invalidBytes = 0;

            // win32
            _jpg_reflash_vram(pHJCodec->pHInJStream,
                              JPG_STREAM_CMD_GET_VRAM_BS_BUF_A,
                              0, 0, &pJHwBsCtrl->addr);

            invalidBytes                                 = (JPG_REG)(((uint32_t)pJHwBsCtrl->addr) & 0x3u);
            tmpAddr                                      = (uint8_t *)(((uint32_t)pJHwBsCtrl->addr) & ~0x3u);
            tmpSize                                      = (((((uint32_t)pJHwBsCtrl->addr) & 0x3u) + pJHwBsCtrl->size) + 0x3u) & ~0x3u;

            pJHwBsCtrl->preBsBuf                         = pJDecoder->vramBsAddr
                                                         = pJHwBsCtrl->addr
                                                         = tmpAddr;

            pJDecoder->vramBsSize                        = pJHwBsCtrl->size = tmpSize;

            JPG_SetBitStreamBufInfoReg(pJHwBsCtrl);
            JPG_SetBitstreamReadBytePosReg(invalidBytes);
        }

        //---------------------------------
        // start fire jpg
        JPG_LogReg(false);  //Benson
        JPG_StartReg();
    } while (false);

    if (result != JPG_ERR_OK)
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
    }
    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_DEC);
    return result;
}

static JPG_ERR
jpg_dec_fire(
    JPG_CODEC_HANDLE *pHJCodec,
    void             *extraData)
{
    JPG_ERR result = JPG_ERR_OK;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_DEC, "0x%x, 0x%x\n", pHJCodec, extraData);

    do
    {
        uint32_t            actBsSize = 0;
        bool                bEnd = false, bSkip = false;
        JPG_HW_BS_CTRL      *pJHwBsCtrl      = &pHJCodec->jHwBsCtrl;
        JPG_DECODER         *pJDecoder       = (JPG_DECODER *)pHJCodec->privData;
        JPG_MULTI_SECT_INFO *pJMultiSectInfo = &pHJCodec->jMultiSectInfo;
        JCOMM_HANDLE        *pHJComm         = (JCOMM_HANDLE *)extraData;

        uint8_t             *WriteBuf        = NULL;
        uint8_t             *mappedSysRam    = NULL;

        //---------------------------------
        // fire isp image process
        if (pJMultiSectInfo->bFirst == true)
        {
            pJMultiSectInfo->bFirst = false;
            result                  = Jpg_Ext_Link_Ctrl(JEL_CTRL_ISP_IMG_PROC, NULL, (void *)&pHJCodec->jShare2Isp);
            if (result != JPG_ERR_OK)
            {
                jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
                break;
            }
        }

        //---------------------------------
        // set system bs buffer info
        pJDecoder->pSysBsStart = pHJCodec->pSysBsBuf;
        actBsSize              = pHJCodec->sysBsBufSize;

#if 1
        WriteBuf               = (uint8_t *)itpVmemAlloc((pJHwBsCtrl->size ));
        if (WriteBuf != NULL)
        {
            mappedSysRam = (uint8_t *)ithMapVram((uint32_t)WriteBuf, (pJHwBsCtrl->size ), ITH_VRAM_WRITE);

            if (pHJComm->actIOBuf_idx != 0u)
            {
                (void)memcpy(mappedSysRam, pHJComm->jInBufInfo[0].pBufAddr, (pJHwBsCtrl->size ));
                ithUnmapVram((void *)mappedSysRam, (pJHwBsCtrl->size ));
                ithFlushDCacheRange((void *)mappedSysRam, (pJHwBsCtrl->size ));
                ithFlushMemBuffer();
            }
            else
            {
                (void)memcpy(mappedSysRam, pHJComm->jInBufInfo[1].pBufAddr, (pJHwBsCtrl->size ));
                ithUnmapVram(mappedSysRam, (pJHwBsCtrl->size ));
                ithFlushDCacheRange(mappedSysRam, (pJHwBsCtrl->size ));
                ithFlushMemBuffer();
            }

            itpVmemFree((uint32_t)WriteBuf);
        }
#endif
        while ( (actBsSize > 0) && (bEnd == false) )
        {
            (void)usleep(1000);

#ifdef _MSC_VER
            result = _JPG_Set_CodedDataToVram(pHJCodec, &actBsSize, &bEnd);
#else
            result = _JPG_Set_CodedData(pHJCodec, &actBsSize, &bEnd);
#endif
            if (result != JPG_ERR_OK)
            {
                jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg Err !!");
                bSkip = true;
                break;
            }
        }

        if (bSkip == true) 
        {
			break;
        }

        if (bEnd == true) 
        {
			pHJCodec->bLastSection = true;
        }

        if (pHJCodec->bLastSection == true)
        {
            result = _JPG_HW_Wait_Idle(JPG_DEC_TIMEOUT_COUNT);
            if (result != JPG_ERR_OK)
            {
                jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg Err !!");
                //break;
            }
        }

        //---------------------------------
        // // if this were last section, it has to wait ISP engine ready.
        if (pHJCodec->jMultiSectInfo.bFinished == true)
        {
            (void)usleep(10 * 1000);
            //Benson mark it
#if 1
            result = Jpg_Ext_Link_Ctrl(JEL_CTRL_ISP_WAIT_IDLE, NULL, NULL);
            if (result != JPG_ERR_OK)
            {
                result = Jpg_Ext_Link_Ctrl(JEL_CTRL_ISP_POWERDOWN, NULL, NULL);
				#if 0
                jpg_msg_ex(JPG_MSG_TYPE_ERR, " Isp err !! ", result);
                JPG_LogReg(true);
                break;
				#endif
            }
#endif
        }
    } while (false);

    if (result != JPG_ERR_OK)
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
    }
    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_DEC);
    return result;
}

static JPG_ERR
jpg_dec_ctrl(
    JPG_CODEC_HANDLE *pHJCodec,
    uint32_t         cmd,
    uint32_t         *value,
    void             *extraData)
{
    JPG_ERR result = JPG_ERR_OK;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_DEC, "0x%x, 0x%x, 0x%x, 0x%x\n", pHJCodec, cmd, value, extraData);

    switch (cmd)
    {
    default:
        result = JPG_ERR_NO_IMPLEMENT;
        break;
    }

    if ( (result != JPG_ERR_OK) &&
        (result != JPG_ERR_NO_IMPLEMENT) )
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !! ", __FUNCTION__, result);
    }
    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_DEC);
    return result;
}

//=============================================================================
//                  Public Function Definition
//=============================================================================
JPG_CODEC_DESC JPG_CODEC_DESC_decoder_desc =
{
    "signal jpg dec",   
    NULL,               
    JPG_CODEC_DEC_JPG,  
    jpg_dec_init,       
    jpg_dec_deInit,     
    jpg_dec_setup,      
    jpg_dec_fire,       
    jpg_dec_ctrl,       
};
