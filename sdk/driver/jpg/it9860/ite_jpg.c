#include "ite_jpg.h"
#include "jpg_defs.h"
#include "jpg_parser.h"
#include "jpg_common.h"
#include "jpg_codec.h"
//=============================================================================
//                  Constant Definition
//=============================================================================
#define JPG_MAX_WIDTH                  16376
#define JPG_MAX_HEIGHT                 16376

#define MJPG_MAX_WIDTH                 16376
#define MJPG_MAX_HEIGHT                16376

#if (JPG_CHECK_CALLER)
    #undef iteJpg_CreateHandle
    #undef iteJpg_DestroyHandle
    #undef iteJpg_SetStreamInfo
    #undef iteJpg_Parsing
    #undef iteJpg_Setup
    #undef iteJpg_Process
    #undef iteJpg_SetBaseOutInfo
    #undef iteJpg_Reset
    #undef iteJpg_WaitIdle

    #define iteJpg_CreateHandle         iteJpg_CreateHandle_dbg
    #define iteJpg_DestroyHandle        iteJpg_DestroyHandle_dbg
    #define iteJpg_SetStreamInfo        iteJpg_SetStreamInfo_dbg
    #define iteJpg_Parsing              iteJpg_Parsing_dbg
    #define iteJpg_Setup                iteJpg_Setup_dbg
    #define iteJpg_Process              iteJpg_Process_dbg
    #define iteJpg_SetBaseOutInfo       iteJpg_SetBaseOutInfo_dbg
    #define iteJpg_Reset                iteJpg_Reset_dbg
    #define iteJpg_WaitIdle             iteJpg_WaitIdle_dbg
#endif
//=============================================================================
//                  Macro Definition
//=============================================================================

//=============================================================================
//                  Structure Definition
//=============================================================================

typedef struct JPG_DEV_TAG
{
    JPG_INIT_PARAM      initParam;

    // it maybe need to pretect parsing
    pthread_mutex_t     jPsr_mutex;

    // I/O handler
    JPG_STREAM_HANDLE   hJInStream;
    JPG_STREAM_HANDLE   hJOutStream;

    // JPG_USER_INFO       userInfo;
    JPG_STATUS          status;
    JPG_RECT            jpgRect;

    // jpg common -> H/W handler
    JCOMM_HANDLE        *pHJComm;

    // jpg parser info
    JPG_PARSER_INFO     jPrsInfo;

    // output display info
    JPG_DISP_INFO       jDispInfo;

    // encode
    uint32_t            encSectHight;
    bool                bPartialEnc;
    uint8_t             *pExifInfo;
    uint32_t            exifInfoSize;

}JPG_DEV;
//=============================================================================
//                  Global Data Definition
//=============================================================================
uint32_t  jpgMsgOnFlag = ((uint32_t)JPG_MSG_TYPE_ERR);// | JPG_MSG_TYPE_TRACE_ITEJPG | JPG_MSG_TYPE_TRACE_JCOMM);  // | JPG_MSG_TYPE_TRACE_PARSER);

extern JPG_STREAM_DESC jpg_stream_file_desc;
extern JPG_STREAM_DESC jpg_stream_mem_desc;
extern bool b_JPG_REGS_USE_CMDQ;

static pthread_mutex_t  g_jpg_codec_mutex = PTHREAD_MUTEX_INITIALIZER; // only one jpg/isp H/W module
//=============================================================================
//                  Private Function Definition
//=============================================================================
static JPG_ERR
_Verify_DecType(
    JPG_STREAM_HANDLE   *pHJStream,
    JPG_PARSER_INFO     *pJPrsInfo,
    JPG_INIT_PARAM      *pInitParam)
{
    JPG_ERR             result = JPG_ERR_OK;
    JPG_ATTRIB          *pJAttrib = &pJPrsInfo->jAttrib;
    JPG_PRS_BS_CTRL     *pJPrsBsCtrl = &pJPrsInfo->jPrsBsCtrl;

    switch( pInitParam->decType )
    {
        case JPG_DEC_PRIMARY:
            if( ((uint32_t)pJAttrib->flag & (uint32_t)JATT_STATUS_UNSUPPORT_PRIMARY) != 0u )
            {
                result = JPG_ERR_HW_NOT_SUPPORT;
                goto end;
            }

            jPrs_Seek2SectionStart(pHJStream, pJPrsInfo, pJAttrib->primaryOffset, pJAttrib->primaryLength);
            pJAttrib->decThumbType = JPG_DEC_PRIMARY;
            break;

        case JPG_DEC_LARGE_THUMB:
        case JPG_DEC_SMALL_THUMB:
            // ------------------------------
            // select decode type dependent on parsing result
            switch( (uint32_t)pJAttrib->flag & ((uint32_t)JATT_STATUS_HAS_SMALL_THUMB |
                                     (uint32_t)JATT_STATUS_HAS_LARGE_THUMB |
                                     (uint32_t)JATT_STATUS_UNSUPPORT_SMALL_THUMB |
                                     (uint32_t)JATT_STATUS_UNSUPPORT_LARGE_THUMB))
            {
                case ((uint32_t)JATT_STATUS_HAS_SMALL_THUMB |
                      (uint32_t)JATT_STATUS_HAS_LARGE_THUMB): //Support both small thumbnail and large thumbnail.
                    if( pJAttrib->jBaseLiteInfo[JPG_DEC_SMALL_THUMB].width < pJAttrib->jBaseLiteInfo[JPG_DEC_LARGE_THUMB].width )
                    {
                        pJAttrib->decThumbType = JPG_DEC_LARGE_THUMB;
                    }
                    else
                    {
                        pJAttrib->decThumbType = JPG_DEC_SMALL_THUMB;
                    }
                    break;

                case ((uint32_t)JATT_STATUS_HAS_SMALL_THUMB |
                      (uint32_t)JATT_STATUS_HAS_LARGE_THUMB |
                      (uint32_t)JATT_STATUS_UNSUPPORT_LARGE_THUMB): // Has 2 thumbnail but large thumb is not supported.
                case (uint32_t)JATT_STATUS_HAS_SMALL_THUMB: // Only support small thumbnail.
                    pJAttrib->decThumbType = JPG_DEC_SMALL_THUMB;
                    break;

                case ((uint32_t)JATT_STATUS_HAS_LARGE_THUMB |
                      (uint32_t)JATT_STATUS_HAS_SMALL_THUMB |
                      (uint32_t)JATT_STATUS_UNSUPPORT_SMALL_THUMB): // Has 2 thumbnail but small thumb is not supported.
                case (uint32_t)JATT_STATUS_HAS_LARGE_THUMB: // Only support large thumbnail.
                    pJAttrib->decThumbType = JPG_DEC_LARGE_THUMB;
                    break;
                case ((uint32_t)JATT_STATUS_HAS_SMALL_THUMB |
                      (uint32_t)JATT_STATUS_UNSUPPORT_SMALL_THUMB): //Has thumbnail1 but not supported.
                case ((uint32_t)JATT_STATUS_HAS_SMALL_THUMB |
                      (uint32_t)JATT_STATUS_UNSUPPORT_SMALL_THUMB |
                      (uint32_t)JATT_STATUS_HAS_LARGE_THUMB |
                      (uint32_t)JATT_STATUS_UNSUPPORT_LARGE_THUMB): // Has 2 thumbnail but both can't supported.
                default:
                    if( ((uint32_t)pJAttrib->flag & (uint32_t)JATT_STATUS_UNSUPPORT_PRIMARY) != 0u )
                    {
                        result = JPG_ERR_HW_NOT_SUPPORT;
                        goto end;
                    }

                    jPrs_Seek2SectionStart(pHJStream, pJPrsInfo, pJAttrib->primaryOffset, pJAttrib->primaryLength);
                    pJAttrib->decThumbType = JPG_DEC_PRIMARY;
                    break;

            }

            if( pJAttrib->decThumbType == JPG_DEC_SMALL_THUMB )
            {
                // small thumbnail
                jPrs_Seek2SectionStart(
                    pHJStream, pJPrsInfo,
                    pJAttrib->jBaseLiteInfo[JPG_DEC_SMALL_THUMB].offset,
                    pJAttrib->jBaseLiteInfo[JPG_DEC_SMALL_THUMB].size);

                pJPrsInfo->bMultiSection = false;
            }
            else if( pJAttrib->decThumbType == JPG_DEC_LARGE_THUMB )
            {
                // large thumbnail
                // only std file I/O can work, mem I/O maybe crash
                uint32_t      thumbSize = 0;
                uint32_t      i;

                for(i = 0; i < pJAttrib->app2StreamCnt; i++)
                {
                    thumbSize += pJAttrib->app2StreamSize[i];
                }
                pJAttrib->jBaseLiteInfo[JPG_DEC_LARGE_THUMB].size = thumbSize;

                if( thumbSize > pJPrsBsCtrl->bsPrsBufMaxLeng )
                {
                    if( ((uint32_t)pJAttrib->flag & (uint32_t)JATT_STATUS_UNSUPPORT_PRIMARY) != 0u )
                    {
                        result = JPG_ERR_HW_NOT_SUPPORT;
                        goto end;
                    }

                    jPrs_Seek2SectionStart(pHJStream, pJPrsInfo, pJAttrib->primaryOffset, pJAttrib->primaryLength);
                    pJAttrib->decThumbType = JPG_DEC_PRIMARY;
                }
                else
                {
                    uint32_t            realSize = 0;
                    uint8_t             *tmpAddr = pJPrsBsCtrl->bsPrsBuf;
                    JPG_STREAM_DESC     *pJStreamDesc = &pHJStream->jStreamDesc;

                    for(i = 0; i < pJAttrib->app2StreamCnt; i++)
                    {
                        if( pJStreamDesc->jSeek_stream != NULL)
                        {
                            (void)pJStreamDesc->jSeek_stream(pHJStream, pJAttrib->app2StreamOffset[i], JPG_SEEK_SET, NULL);
                        }	

                        if( pJStreamDesc->jFull_buf != NULL)
                        {
                            (void)pJStreamDesc->jFull_buf(pHJStream, tmpAddr,
                                                    pJAttrib->app2StreamSize[i],
                                                    &realSize, NULL);
                        }
                        tmpAddr += pJAttrib->app2StreamSize[i];
                    }

                    pJPrsBsCtrl->curPos = 0;
                    pJPrsInfo->bMultiSection = false;
                }
            }
			else
			{
				;/*Do nothing*/
			}
            break;
		default:
			; /*Do nothing*/
			break;
    }

end:
    return result;
}

static void
_Set_Exif_Orientation(
    JPG_DEC_TYPE     decType,
    JPG_ROT_TYPE     rotType,
    bool             bExifOrientation,
    uint32_t         exifOrientationType,
    JPG_DISP_INFO    *pJDispInfo)
{
	uint32_t value;
    if( (bExifOrientation == true) && (decType == JPG_DEC_PRIMARY) )
    {
        switch( exifOrientationType )
        {
            case 0:             break;
            case 1: pJDispInfo->rotType = JPG_ROT_TYPE_0;    break;
            case 2:             break;
            case 3: pJDispInfo->rotType = JPG_ROT_TYPE_180;  break;
            case 4:             break;
            case 5:             break;
            case 6: pJDispInfo->rotType = JPG_ROT_TYPE_90;   break;
            case 7:             break;
            case 8: pJDispInfo->rotType = JPG_ROT_TYPE_270;  break;
			default:
				; /*Do nothing*/
				break;
        }

		value = ((uint32_t)rotType + (uint32_t)pJDispInfo->rotType) & 0x3u;

		if( value == 0u)
		{
				pJDispInfo->rotType = JPG_ROT_TYPE_0;
		}
		else if ( value == 1u)
		{
				pJDispInfo->rotType = JPG_ROT_TYPE_90;
		}
		else if ( value == 2u)
		{
				pJDispInfo->rotType = JPG_ROT_TYPE_180;
		}
		else if ( value == 3u)
		{
				pJDispInfo->rotType = JPG_ROT_TYPE_270;
		}
		else
		{
			;//Do nothing
		}
    }
    else
    {
        pJDispInfo->rotType = rotType;
    }
}

static void
_genParam_Fit (
    JPG_DISP_INFO     *pJDispInfo,
    uint32_t          dispWidth,
    uint32_t          dispHeight,
    uint32_t          realWidth,
    uint32_t          realHeight)
{
    pJDispInfo->srcW = realWidth;
    pJDispInfo->srcH = realHeight;
    pJDispInfo->srcX = 0;
	pJDispInfo->srcY = 0;

    pJDispInfo->dstW = dispWidth;
    pJDispInfo->dstH = dispHeight;
    pJDispInfo->dstX = 0;
	pJDispInfo->dstY = 0;
}


static void
_genParam_CutPart(
     JPG_DISP_INFO     *pJDispInfo,
     uint32_t          dispWidth,
     uint32_t          dispHeight,
     uint32_t          realWidth,
     uint32_t          realHeight,
     uint32_t          keepPercentage)
{
    uint32_t    scalWidth = 0, scalHeight = 0;
    uint32_t    picWidth  = 0;
    uint32_t    picHeight = 0;

    picWidth  = realWidth;
    picHeight = realHeight;

    switch( pJDispInfo->rotType )
    {
        case JPG_ROT_TYPE_0:
        case JPG_ROT_TYPE_180:
            // First, according to the aspect ratio of the JPEG picture to decide how to fit the decoded picture
            scalWidth  = dispWidth;
            scalHeight = scalWidth * picHeight / picWidth;

            if( scalHeight < dispHeight )
            {
                // Cut left and right of the source picture
                scalWidth = dispHeight * picWidth / picHeight;
                if( (scalWidth * keepPercentage) >= (100u * dispWidth) )
                {
                    // reserve 60% case
                    scalHeight = (dispWidth * 100u * picHeight) / (keepPercentage * picWidth);
                    pJDispInfo->srcW = picWidth * keepPercentage / 100u;
                    pJDispInfo->srcH = picHeight;
                    pJDispInfo->srcX = (picWidth - pJDispInfo->srcW) >> 1;
                    pJDispInfo->srcY = 0;

                    pJDispInfo->dstX = 0;
                    pJDispInfo->dstY = (dispHeight - scalHeight) >> 1;
                    pJDispInfo->dstW = dispWidth;
                    pJDispInfo->dstH = scalHeight;
                }
                else
                {
                    uint32_t    tmpH = picHeight;
                    uint32_t    tmpW = picHeight * dispWidth / dispHeight;

                    pJDispInfo->srcW = tmpW;
                    pJDispInfo->srcH = tmpH;
                    pJDispInfo->srcX = (picWidth - tmpW) >> 1;
                    pJDispInfo->srcY = 0;

                    pJDispInfo->dstX = 0;
                    pJDispInfo->dstY = 0;
                    pJDispInfo->dstW = dispWidth;
                    pJDispInfo->dstH = dispHeight;
                }
            }
            else
            {
                // Cut top and bottom of the source picture
                if ( (scalHeight * keepPercentage) >= (100u * dispHeight) )
                {
                    // reserve 60% case
                    scalWidth = (dispHeight * 100u * picWidth) / (keepPercentage * picHeight);

                    pJDispInfo->srcW = picWidth;
                    pJDispInfo->srcH = picHeight * keepPercentage / 100u;
                    pJDispInfo->srcX = 0;
                    pJDispInfo->srcY = (picHeight - pJDispInfo->srcH) >> 1;

                    pJDispInfo->dstX = (dispWidth - scalWidth) >> 1;
                    pJDispInfo->dstY = 0;
                    pJDispInfo->dstW = scalWidth;
                    pJDispInfo->dstH = dispHeight;
                }
                else
                {
                    uint32_t    tmpW = picWidth;
                    uint32_t    tmpH = picWidth * dispHeight / dispWidth;

                    pJDispInfo->srcW = tmpW;
                    pJDispInfo->srcH = tmpH;
                    pJDispInfo->srcX = 0;
                    pJDispInfo->srcY = (picHeight - tmpH) >> 1;

                    pJDispInfo->dstX = 0;
                    pJDispInfo->dstY = 0;
                    pJDispInfo->dstW = dispWidth;
                    pJDispInfo->dstH = dispHeight;
                }
            }
            break;

        case JPG_ROT_TYPE_90:
        case JPG_ROT_TYPE_270:
            // First, according to the aspect ratio of the JPEG picture
            //        to decide how to fit the decoded picture
            scalWidth = dispWidth;
            scalHeight = scalWidth * picHeight / picWidth;

            if( scalHeight < dispHeight )
            {
                // Cut top and bottom of the source picture
                scalWidth = dispHeight * picWidth / picHeight;
                if( (scalWidth * keepPercentage) >= (100u * dispWidth) )
                {
                    // reserve 60% case
                    scalHeight = (dispWidth * 100u * picHeight) / (keepPercentage * picWidth);

                    pJDispInfo->srcW = picHeight;
                    pJDispInfo->srcH = picWidth * keepPercentage / 100u;
                    pJDispInfo->srcX = 0;
                    pJDispInfo->srcY = (picWidth - pJDispInfo->srcH) >> 1;

                    pJDispInfo->dstX = 0;
                    pJDispInfo->dstY = (dispHeight - scalHeight) >> 1;
                    pJDispInfo->dstW = dispWidth;
                    pJDispInfo->dstH = scalHeight;
                }
                else
                {
                    uint32_t    tmpW = picHeight;
                    uint32_t    tmpH = picHeight * dispWidth / dispHeight;

                    pJDispInfo->srcW = tmpW;
                    pJDispInfo->srcH = tmpH;
                    pJDispInfo->srcX = 0;
                    pJDispInfo->srcY = (picWidth - tmpH) >> 1;

                    pJDispInfo->dstX = 0;
                    pJDispInfo->dstY = 0;
                    pJDispInfo->dstW = dispWidth;
                    pJDispInfo->dstH = dispHeight;
                }
            }
            else
            {
                // Cut left and right of the source picture
                if( (scalHeight * keepPercentage) >= (100u * dispHeight) )
                {
                    // reserve 60% case
                    scalWidth = (dispHeight * 100u * picWidth) / (keepPercentage * picHeight);

                    pJDispInfo->srcW = picHeight * keepPercentage / 100u;
                    pJDispInfo->srcH = picWidth;
                    pJDispInfo->srcX = (picHeight - pJDispInfo->srcW) >> 1;
                    pJDispInfo->srcY = 0;

                    pJDispInfo->dstX = (dispWidth - scalWidth) >> 1;
                    pJDispInfo->dstY = 0;
                    pJDispInfo->dstW = scalWidth;
                    pJDispInfo->dstH = dispHeight;
                }
                else
                {
                    uint32_t    tmpH = picWidth;
                    uint32_t    tmpW = picWidth * dispHeight / dispWidth;

                    pJDispInfo->srcW = tmpW;
                    pJDispInfo->srcH = tmpH;
                    pJDispInfo->srcX = (picHeight - tmpW) >> 1;
                    pJDispInfo->srcY = 0;

                    pJDispInfo->dstX = 0;
                    pJDispInfo->dstY = 0;
                    pJDispInfo->dstW = dispWidth;
                    pJDispInfo->dstH = dispHeight;
                }
            }
            break;
		default:
			; /*Do nothing*/
			break;
    }

    return;
}

static void
_genParam_CutByRect(
    JPG_DISP_INFO     *pJDispInfo,
    uint32_t          dispWidth,
    uint32_t          dispHeight,
    uint32_t          realWidth,
    uint32_t          realHeight)
{
    if(realWidth >= realHeight)
    {
        uint32_t    tmpH = realHeight;
        uint32_t    tmpW = realHeight * dispWidth / dispHeight;

        if(tmpW > realWidth)
        {
            tmpW = realWidth;
        }
        pJDispInfo->srcW = tmpW;
        pJDispInfo->srcH = tmpH;
        pJDispInfo->srcX = (realWidth - tmpW) >> 1;
        pJDispInfo->srcY = 0;

        pJDispInfo->dstX = 0;
        pJDispInfo->dstY = 0;
        pJDispInfo->dstW = dispWidth;
        pJDispInfo->dstH = dispHeight;
    }
    else
    {
        uint32_t    tmpW = realWidth;
        uint32_t    tmpH = realWidth * dispHeight / dispWidth;

        if(tmpH > realHeight)
        {
            tmpH = realHeight;
        }
        pJDispInfo->srcW = tmpW;
        pJDispInfo->srcH = tmpH;
        pJDispInfo->srcX = 0;
        pJDispInfo->srcY = (realHeight - tmpH) >> 1;

        pJDispInfo->dstX = 0;
        pJDispInfo->dstY = 0;
        pJDispInfo->dstW = dispWidth;
        pJDispInfo->dstH = dispHeight;
    }
}


static void
_calc_partial_enc_heigth(
    JPG_DEV     *pJpgDev)
{
    uint32_t             ratio = 0;
    JCOMM_HANDLE        *pHJComm = pJpgDev->pHJComm;

    // general compression ratio
    if( pJpgDev->initParam.encQuality > 90u )
    {
		ratio = 4;
    }
    else if( pJpgDev->initParam.encQuality > 85u )
    {
		ratio = 10;
    }
    else
    {
		ratio = 15;
    }

    pJpgDev->encSectHight = (pHJComm->sysBsBufSize * ratio) / (3u * pJpgDev->initParam.width);

    pJpgDev->encSectHight &= (~0xFu); // MCI 16x8

	/*
    // switch( pJpgDev->initParam.outColorSpace )
    // {
    //     // MCU 16x8
    //     case JPG_COLOR_SPACE_YUV422:  pJpgDev->encSectHight &= (~0x7); break;
    //
    //     // MCI 16x8
    //     case JPG_COLOR_SPACE_YUV420:  pJpgDev->encSectHight &= (~0xF);  break;
    // }
	*/

    if( pJpgDev->encSectHight > pJpgDev->initParam.height )
    {
        pJpgDev->bPartialEnc = false;
        pJpgDev->encSectHight = pJpgDev->initParam.height;
    }
    else
    {
        pJpgDev->bPartialEnc = true;
    }
    pHJComm->encSectHight = pJpgDev->encSectHight;

    return;
}

static JPG_ERR
_jpg_dec_setup(
    JPG_DEV     *pJpgDev)
{
    JPG_ERR             result = JPG_ERR_OK;
    JCOMM_HANDLE        *pHJComm = pJpgDev->pHJComm;
    JPG_PARSER_INFO     *pJPrsInfo = &pJpgDev->jPrsInfo;
    JPG_PRS_BS_CTRL     *pJPrsBsCtrl = &pJPrsInfo->jPrsBsCtrl;
    JPG_STREAM_HANDLE   *pHInJStream = &pJpgDev->hJInStream;
    JPG_STREAM_HANDLE   *pHOutJStream = &pJpgDev->hJOutStream;
    JPG_STREAM_DESC     *pJInStreamDesc = &pHInJStream->jStreamDesc;
    JPG_DISP_INFO       *pJDispInfo = &pJpgDev->jDispInfo;
    JPG_DEC_TYPE        decType = pJpgDev->initParam.decType;
    JCOMM_INIT_PARAM    jCommInitParam = {0};

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ITEJPG, "0x%x\n", pJpgDev);

    do{
        uint32_t     i;

        //-----------------------------------------------
        // jpg H/W init
        jCommInitParam.pHInJStream  = &pJpgDev->hJInStream;
        jCommInitParam.pHOutJStream = &pJpgDev->hJOutStream;
        jCommInitParam.pJDispInfo   = pJDispInfo;

        if( pJPrsInfo->jAttrib.jBaseLiteInfo[decType].bJprog == true )
        {
            jCommInitParam.codecType = JPG_CODEC_DEC_JPROG;
        }
        else
        {
            jCommInitParam.codecType = JPG_CODEC_DEC_JPG;
        }

        //---------------------------------  //Benson
        // assign info from parser
        pHJComm->pJFrmComp  = &pJPrsInfo->jFrmComp;

        result = jComm_Init(pHJComm, &jCommInitParam, NULL);
        if( result != JPG_ERR_OK )
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " error !!");
            result = JPG_ERR_ALLOCATE_FAIL;
            break;
        }

        //------------------------------------------------
        // jpg H/W setup
        if( (pJPrsInfo->bMultiSection == true) &&
            (pJPrsInfo->remainSize > 0u) )
        {
            // multi-section
            uint8_t     *tmpAddr = pJPrsBsCtrl->bsPrsBuf + pJPrsBsCtrl->curPos;
            uint32_t     tmpLength = pJPrsBsCtrl->realSize - pJPrsBsCtrl->curPos;

            // move and fill bs buffer
            (void)memcpy(pJPrsBsCtrl->bsPrsBuf, tmpAddr, tmpLength);
            tmpAddr = pJPrsBsCtrl->bsPrsBuf + tmpLength;
            if( pJInStreamDesc->jFull_buf != NULL )
            {
                (void)pJInStreamDesc->jFull_buf(pHInJStream, tmpAddr,
                                          pJPrsBsCtrl->curPos,
                                          &pHJComm->realSzie, NULL);
            }

            pHJComm->remainSize = pJPrsInfo->remainSize - pHJComm->realSzie;

            if( pHJComm->remainSize == 0u )
            {
                pHJComm->jInBufInfo[0].pBufAddr = pJPrsBsCtrl->bsPrsBuf;
				pHJComm->jInBufInfo[1].pBufAddr = pHJComm->jInBufInfo[0].pBufAddr;
                pHJComm->jInBufInfo[0].bufLength = pJPrsInfo->fileLength - pJPrsBsCtrl->curPos;
				pHJComm->jInBufInfo[1].bufLength = pHJComm->jInBufInfo[0].bufLength;
				
                pJPrsInfo->bMultiSection = false;
            }
            else
            {
                // for pipe line decode, bsBufMaxLeng_A should be alignment
                pHJComm->jInBufInfo[0].pBufAddr  = pJPrsBsCtrl->bsPrsBuf;
                pHJComm->jInBufInfo[1].pBufAddr  = pJPrsBsCtrl->bsPrsBuf + (pJPrsBsCtrl->bsPrsBufMaxLeng >> 1);

                pHJComm->jInBufInfo[0].bufLength = (pJPrsBsCtrl->bsPrsBufMaxLeng >> 1);
				pHJComm->jInBufInfo[1].bufLength = pHJComm->jInBufInfo[0].bufLength;
            }

            pJPrsBsCtrl->curPos = 0;
        }
        else
        {
            // signal section
            JPG_ATTRIB          *pJAttrib = &pJPrsInfo->jAttrib;

            pHJComm->jInBufInfo[0].pBufAddr = pJPrsBsCtrl->bsPrsBuf + pJPrsBsCtrl->curPos;
			pHJComm->jInBufInfo[1].pBufAddr = pHJComm->jInBufInfo[0].pBufAddr;
			
            switch( pJAttrib->decThumbType )
            {
                case JPG_DEC_SMALL_THUMB:
                case JPG_DEC_LARGE_THUMB:
                    pHJComm->jInBufInfo[0].bufLength = pJAttrib->jBaseLiteInfo[pJAttrib->decThumbType].size;
                    break;

                default:
                    pHJComm->jInBufInfo[0].bufLength = pJPrsBsCtrl->realSize - pJPrsBsCtrl->curPos;
                    break;
            }
            pHJComm->jInBufInfo[1].bufLength = pHJComm->jInBufInfo[0].bufLength;
            pHJComm->remainSize = 0;
        }

        pHJComm->actIOBuf_idx = 0;

        switch( pJpgDev->initParam.outColorSpace )
        {
            case JPG_COLOR_SPACE_ARGB4444:
            case JPG_COLOR_SPACE_ARGB8888:
            case JPG_COLOR_SPACE_RGB565:
                pHOutJStream->compCnt = 1;
                pHJComm->jOutBufInfo[0].pBufAddr  = pHOutJStream->jStreamInfo.jstream.mem[0].pAddr;
                pHJComm->jOutBufInfo[0].bufLength = pHOutJStream->jStreamInfo.jstream.mem[0].length;
                pHJComm->jOutBufInfo[0].pitch     = pHOutJStream->jStreamInfo.jstream.mem[0].pitch;
                break;

            case JPG_COLOR_SPACE_YUV422:
            case JPG_COLOR_SPACE_YUV420:
			case JPG_COLOR_SPACE_YUV444:
                // set output buffer info
                pHOutJStream->compCnt = pHOutJStream->jStreamInfo.validCompCnt;
                for(i = 0; i < pHOutJStream->compCnt; i++)
                {
                    pHJComm->jOutBufInfo[i].pBufAddr  = pHOutJStream->jStreamInfo.jstream.mem[i].pAddr;
                    pHJComm->jOutBufInfo[i].bufLength = pHOutJStream->jStreamInfo.jstream.mem[i].length;
                    pHJComm->jOutBufInfo[i].pitch     = pHOutJStream->jStreamInfo.jstream.mem[i].pitch;
                }
                break;
			default:
				; /*Do nothing*/
				break;
        }

        result = jComm_Setup(pHJComm, NULL);

#if !(_MSC_VER)
        pHJComm->actIOBuf_idx = ((pHJComm->actIOBuf_idx+1u) & 0x1u);
#endif

    }while (false);

    if( result != JPG_ERR_OK )
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR,"%s() err 0x%x !", __FUNCTION__, result);
    }
    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ITEJPG);
    return result;
}

static JPG_ERR
_jpg_dec_fire(
    JPG_DEV     *pJpgDev)
{
    JPG_ERR             result = JPG_ERR_OK;
    JCOMM_HANDLE        *pHJComm = pJpgDev->pHJComm;
    JPG_STREAM_HANDLE   *pHInJStream = &pJpgDev->hJInStream;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ITEJPG, "0x%x\n", pJpgDev);

    do{
        bool                b1stFire = true;
        JPG_STREAM_DESC     *pJInStreamDesc = &pHInJStream->jStreamDesc;
        JPG_PARSER_INFO     *pJPrsInfo = &pJpgDev->jPrsInfo;

        result = jComm_Fire(pHJComm, ((pHJComm->remainSize == 0u) ? true : false), NULL);
        if( result != JPG_ERR_OK )
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg err 0x%x !! ", result);
            break;
        }

        pHJComm->actIOBuf_idx = ((pHJComm->actIOBuf_idx+1u) & 0x1u);
        //--------------------------------------------
        // jpg H/W fire
        while( (pJPrsInfo->bMultiSection == true) && (pHJComm->remainSize > 0u) )
        {
            uint8_t     *pCurBsBuf = NULL;
            uint32_t    curBsBufSize = 0;

            (void)usleep(1000);
            if( b1stFire == true )
            {
				b1stFire = false;
            }
            else
            {
				pHJComm->remainSize -= pHJComm->realSzie;
            }	

            result = jComm_Fire(pHJComm, ((pHJComm->remainSize == 0u) ? true : false), NULL);
            if( result != JPG_ERR_OK )
            {
                jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg err 0x%x !! ", result);
                break;
            }

            if( pHJComm->remainSize > 0u )
            {
                // fill buff
        #if (_MSC_VER) // win32
                pHJComm->actIOBuf_idx = ((pHJComm->actIOBuf_idx+1) & 0x1);
                pCurBsBuf    = pHJComm->jInBufInfo[pHJComm->actIOBuf_idx].pBufAddr;
                curBsBufSize = pHJComm->jInBufInfo[pHJComm->actIOBuf_idx].bufLength;
                if( pJInStreamDesc->jFull_buf != NULL)
                {
                    (void)pJInStreamDesc->jFull_buf(pHInJStream, pCurBsBuf,
                                              curBsBufSize,
                                              &pHJComm->realSzie, 0);
                }
        #else
                pCurBsBuf    = pHJComm->jInBufInfo[pHJComm->actIOBuf_idx].pBufAddr;
                curBsBufSize = pHJComm->jInBufInfo[pHJComm->actIOBuf_idx].bufLength;
                if( pJInStreamDesc->jFull_buf != NULL)
                {
                    (void)pJInStreamDesc->jFull_buf(pHInJStream, pCurBsBuf,
                                              curBsBufSize,
                                              &pHJComm->realSzie, NULL);
                }
                pHJComm->actIOBuf_idx = ((pHJComm->actIOBuf_idx+1u) & 0x1u);
        #endif
            }
        }
    }while (false);

    if( result != JPG_ERR_OK )
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR,"%s() err 0x%x !", __FUNCTION__, result);
    }

    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ITEJPG);
    return result;
}

static JPG_ERR
_jpg_dec_cmd_setup(
    JPG_DEV     *pJpgDev)
{
    JPG_ERR             result = JPG_ERR_OK;
    JCOMM_HANDLE        *pHJComm = pJpgDev->pHJComm;
    JPG_PARSER_INFO     *pJPrsInfo = &pJpgDev->jPrsInfo;
    JPG_PRS_BS_CTRL     *pJPrsBsCtrl = &pJPrsInfo->jPrsBsCtrl;
    JPG_STREAM_HANDLE   *pHOutJStream = &pJpgDev->hJOutStream;
    JPG_DISP_INFO       *pJDispInfo = &pJpgDev->jDispInfo;
    JCOMM_INIT_PARAM    jCommInitParam = {0};

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ITEJPG, "0x%x\n", pJpgDev);

    do{
        uint32_t     i;

        //-----------------------------------------------
        // jpg H/W init
        jCommInitParam.pHInJStream  = &pJpgDev->hJInStream;
        jCommInitParam.pHOutJStream = &pJpgDev->hJOutStream;
		jCommInitParam.pJDispInfo	= &pJpgDev->jDispInfo;
        jCommInitParam.codecType    = JPG_CODEC_DEC_JPG_CMD;
        jCommInitParam.alphaPlane = pJpgDev->initParam.alphaPlane;

        result = jComm_Init(pHJComm, &jCommInitParam, NULL);
        if( result != JPG_ERR_OK )
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " error !!");
            result = JPG_ERR_ALLOCATE_FAIL;
            break;
        }

        //---------------------------------
        // assign info from parser
        pHJComm->pJDispInfo = pJDispInfo;
        pHJComm->pJFrmComp  = &pJPrsInfo->jFrmComp;

        {
            // signal section
            //JPG_ATTRIB          *pJAttrib = &pJPrsInfo->jAttrib;

            pHJComm->jInBufInfo[0].pBufAddr = pJPrsBsCtrl->bsPrsBuf + pJPrsBsCtrl->curPos; 
            pHJComm->jInBufInfo[1].pBufAddr = pHJComm->jInBufInfo[0].pBufAddr;

            pHJComm->jInBufInfo[0].bufLength = pJPrsBsCtrl->realSize - pJPrsBsCtrl->curPos;

            pHJComm->jInBufInfo[1].bufLength = pHJComm->jInBufInfo[0].bufLength;
            pHJComm->remainSize = 0;
        }

        pHJComm->actIOBuf_idx = 0;

        pHOutJStream->compCnt = pHOutJStream->jStreamInfo.validCompCnt;
        for(i = 0; i < pHOutJStream->compCnt; i++)
        {
            pHJComm->jOutBufInfo[i].pBufAddr  = pHOutJStream->jStreamInfo.jstream.mem[i].pAddr;
            pHJComm->jOutBufInfo[i].bufLength = pHOutJStream->jStreamInfo.jstream.mem[i].length;
            pHJComm->jOutBufInfo[i].pitch     = pHOutJStream->jStreamInfo.jstream.mem[i].pitch;
        }

        result = jComm_Setup(pHJComm, NULL);

    }while (false);

    if( result != JPG_ERR_OK )
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR,"%s() err 0x%x !", __FUNCTION__, result);
    }
    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ITEJPG);
    return result;

}

static JPG_ERR
_jpg_dec_cmd_fire(
    JPG_DEV         *pJpgDev,
    JPG_BUF_INFO    *pEntropyBufInfo)
{
    JPG_ERR             result = JPG_ERR_OK;
    JCOMM_HANDLE        *pHJComm = pJpgDev->pHJComm;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ITEJPG, "0x%x\n", pJpgDev);

    do{
        JPG_STREAM_INFO     *pJOutStreamInfo = &pJpgDev->hJOutStream.jStreamInfo;

        // set input stream info
        pHJComm->jInBufInfo[0].pBufAddr  = pEntropyBufInfo->pBufAddr;
        pHJComm->jInBufInfo[0].bufLength = pEntropyBufInfo->bufLength;
        pHJComm->jInBufInfo[0].pitch     = pEntropyBufInfo->pitch;

        // mjpg only support memory out
        if( (pJOutStreamInfo->streamType != JPG_STREAM_MEM) ||
            (pJOutStreamInfo->streamIOType != JPG_STREAM_IO_WRITE) )
        {
            result = JPG_ERR_NO_IMPLEMENT;
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg err 0x%x !! ", result);
            break;
        }

        result = jComm_Fire(pHJComm, true, NULL);
        if( result != JPG_ERR_OK )
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg err 0x%x !! ", result);
            break;
        }

    }while (false);

    if( result != JPG_ERR_OK )
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR,"%s() err 0x%x !", __FUNCTION__, result);
    }
    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ITEJPG);
    return result;

}

#if (CONFIG_JPG_CODEC_DESC_DEC_MJPG_DESC)
static JPG_ERR
_jpg_dec_mjpg_setup(
    JPG_DEV     *pJpgDev)
{
    JPG_ERR             result = JPG_ERR_OK;
    JCOMM_HANDLE        *pHJComm = pJpgDev->pHJComm;
    JPG_PARSER_INFO     *pJPrsInfo = &pJpgDev->jPrsInfo;
	JPG_PRS_BS_CTRL     *pJPrsBsCtrl = &pJPrsInfo->jPrsBsCtrl;
    JPG_STREAM_HANDLE   *pHOutJStream = &pJpgDev->hJOutStream;
    JPG_DISP_INFO       *pJDispInfo = &pJpgDev->jDispInfo;
    JCOMM_INIT_PARAM    jCommInitParam = {0};


    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ITEJPG, "0x%x\n", pJpgDev);

    do{
        bool    bBreak = false;
        uint32_t     i;

        //-----------------------------------------------
        // jpg H/W init
        jCommInitParam.pHInJStream  = &pJpgDev->hJInStream;
        jCommInitParam.pHOutJStream = &pJpgDev->hJOutStream;
		jCommInitParam.pJDispInfo	= &pJpgDev->jDispInfo;
        jCommInitParam.codecType    = JPG_CODEC_DEC_MJPG;

        result = jComm_Init(pHJComm, &jCommInitParam, NULL);
        if( result != JPG_ERR_OK )
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " error !!");
            result = JPG_ERR_ALLOCATE_FAIL;
            break;
        }

		//---------------------------------
        // assign info from parser
        pHJComm->pJDispInfo = pJDispInfo;
        pHJComm->pJFrmComp  = &pJPrsInfo->jFrmComp;

        {
            // signal section
            //JPG_ATTRIB          *pJAttrib = &pJPrsInfo->jAttrib;

            pHJComm->jInBufInfo[0].pBufAddr = pJPrsBsCtrl->bsPrsBuf + pJPrsBsCtrl->curPos;
            pHJComm->jInBufInfo[1].pBufAddr = pHJComm->jInBufInfo[0].pBufAddr;

            pHJComm->jInBufInfo[0].bufLength = pJPrsBsCtrl->realSize - pJPrsBsCtrl->curPos;
            pHJComm->jInBufInfo[1].bufLength = pHJComm->jInBufInfo[0].bufLength;

            pHJComm->remainSize = 0;
        }

        pHJComm->actIOBuf_idx = 0;

        switch( pJpgDev->initParam.outColorSpace )
        {
             case JPG_COLOR_SPACE_RGB565:     //For isp handshake  ,Benson
			 	pHOutJStream->compCnt = 1;
                pHJComm->jOutBufInfo[0].pBufAddr  = pHOutJStream->jStreamInfo.jstream.mem[0].pAddr;
                pHJComm->jOutBufInfo[0].bufLength = pHOutJStream->jStreamInfo.jstream.mem[0].length;
                pHJComm->jOutBufInfo[0].pitch     = pHOutJStream->jStreamInfo.jstream.mem[0].pitch;
			 	break;
            case JPG_COLOR_SPACE_YUV422:
            case JPG_COLOR_SPACE_YUV420:
                // set output buffer info
                pHOutJStream->compCnt = pHOutJStream->jStreamInfo.validCompCnt;
                for(i = 0; i < pHOutJStream->compCnt; i++)
                {
                    pHJComm->jOutBufInfo[i].pBufAddr  = pHOutJStream->jStreamInfo.jstream.mem[i].pAddr;
                    pHJComm->jOutBufInfo[i].bufLength = pHOutJStream->jStreamInfo.jstream.mem[i].length;
                    pHJComm->jOutBufInfo[i].pitch     = pHOutJStream->jStreamInfo.jstream.mem[i].pitch;
                }
                break;

            default:
                jpg_msg_ex(JPG_MSG_TYPE_ERR, " error !!");
                result = JPG_ERR_HW_NOT_SUPPORT;
                bBreak = true;
                break;
        }

        if( bBreak == true )
        {
			break;
        }
        result = jComm_Setup(pHJComm, NULL);

    }while (false);

    if( result != JPG_ERR_OK )
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR,"%s() err 0x%x !", __FUNCTION__, result);
    }
    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ITEJPG);
    return result;
}

static JPG_ERR
_jpg_dec_mjpg_fire(
    JPG_DEV         *pJpgDev,
    JPG_BUF_INFO    *pEntropyBufInfo)
{
    JPG_ERR             result = JPG_ERR_OK;
    JCOMM_HANDLE        *pHJComm = pJpgDev->pHJComm;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ITEJPG, "0x%x\n", pJpgDev);

    do{
        JPG_STREAM_INFO     *pJOutStreamInfo = &pJpgDev->hJOutStream.jStreamInfo;

        // set input stream info
        pHJComm->jInBufInfo[0].pBufAddr  = pEntropyBufInfo->pBufAddr;
        pHJComm->jInBufInfo[0].bufLength = pEntropyBufInfo->bufLength;
        pHJComm->jInBufInfo[0].pitch     = pEntropyBufInfo->pitch;

        // mjpg only support memory out
        if( (pJOutStreamInfo->streamType != JPG_STREAM_MEM) ||
            (pJOutStreamInfo->streamIOType != JPG_STREAM_IO_WRITE) )
        {
            result = JPG_ERR_NO_IMPLEMENT;
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg err 0x%x !! ", result);
            break;
        }

        result = jComm_Fire(pHJComm, true, NULL);
        if( result != JPG_ERR_OK )
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg err 0x%x !! ", result);
            break;
        }

    }while (false);

    if( result != JPG_ERR_OK )
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR,"%s() err 0x%x !", __FUNCTION__, result);
    }
    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ITEJPG);
    return result;
}
#endif

#if (CONFIG_JPG_CODEC_DESC_ENCODER_DESC)
static JPG_ERR
_jpg_enc_setup(
    JPG_DEV     *pJpgDev)
{
    JPG_ERR             result = JPG_ERR_OK;
    JCOMM_INIT_PARAM    jCommInitParam = {0};

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ITEJPG, "0x%x\n", pJpgDev);

    do{
        JCOMM_HANDLE        *pHJComm = pJpgDev->pHJComm;
        JPG_STREAM_HANDLE   *pHInJStream = &pJpgDev->hJInStream;
        uint32_t                 i;

        //-----------------------------------------------
        // jpg H/W init
        jCommInitParam.pHInJStream  = &pJpgDev->hJInStream;
        jCommInitParam.pHOutJStream = &pJpgDev->hJOutStream;
        jCommInitParam.codecType    = JPG_CODEC_ENC_JPG;
        jCommInitParam.encQuality   = pJpgDev->initParam.encQuality;
        jCommInitParam.encWidth     = pJpgDev->initParam.width;
        jCommInitParam.encHeight    = pJpgDev->initParam.height;
        jCommInitParam.header_width = pJpgDev->initParam.header_width;
        jCommInitParam.header_height = pJpgDev->initParam.header_height;
        jCommInitParam.encColorSpace = pJpgDev->initParam.outColorSpace;
		jCommInitParam.exColorSpace = pJpgDev->initParam.exColorSpace;

        result = jComm_Init(pHJComm, &jCommInitParam, NULL);
        if( result != JPG_ERR_OK )
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " error !!");
            result = JPG_ERR_ALLOCATE_FAIL;
            break;
        }

        pHJComm->pJFrmComp  = &pJpgDev->jPrsInfo.jFrmComp;
        pHJComm->pJDispInfo = &pJpgDev->jDispInfo;

        //--------------------------------
        // calculate section height for encoding
        _calc_partial_enc_heigth(pJpgDev);
        //jpg_msg(1, "\t.... bPartialEnc = %d\r\n", pJpgDev->bPartialEnc); //Benson

        //---------------------------------
        // set input stream info
        pHInJStream->compCnt = pHInJStream->jStreamInfo.validCompCnt;
        for(i = 0; i < pHInJStream->compCnt; i++)
        {
            pHJComm->jInBufInfo[i].pBufAddr  = pHInJStream->jStreamInfo.jstream.mem[i].pAddr;
            pHJComm->jInBufInfo[i].bufLength = pHInJStream->jStreamInfo.jstream.mem[i].length;
            pHJComm->jInBufInfo[i].pitch     = pHInJStream->jStreamInfo.jstream.mem[i].pitch;
        }

        result = jComm_Setup(pHJComm, NULL);

    }while (false);

    if( result != JPG_ERR_OK )
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR,"%s() err 0x%x !", __FUNCTION__, result);
    }

    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ITEJPG);
    return result;
}

static JPG_ERR
_jpg_enc_fire(
    JPG_DEV         *pJpgDev,
    JPG_BUF_INFO    *pSysBsBufInfo,
    uint32_t        *pJpgSize)
{
    JPG_ERR             result = JPG_ERR_OK;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ITEJPG, "0x%x\n", pJpgDev);

    do{
        JCOMM_HANDLE        *pHJComm = pJpgDev->pHJComm;
        JPG_STREAM_HANDLE   *pHOutJStream = &pJpgDev->hJOutStream;
        JPG_STREAM_DESC     *pJOutStreamDesc = &pHOutJStream->jStreamDesc;
        uint32_t            remain_H = pJpgDev->initParam.height;
        uint32_t            totalEncLeng = 0;
        uint8_t             *AsyncSaveAllAddr = NULL;

        if( pJOutStreamDesc->jOut_buf == NULL )
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg err, No out stream API!! ");
            result = JPG_ERR_INVALID_PARAMETER;
            break;
        }

        result = jComm_Fire(pHJComm, !pJpgDev->bPartialEnc, NULL);
        if( result != JPG_ERR_OK )
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg err 0x%x !! ", result);
            break;
        }

        //---------------------------------
        // save enc leng
        totalEncLeng = pJpgDev->pHJComm->sysValidBsBufSize + pJpgDev->pHJComm->jHdrDataSize + pJpgDev->exifInfoSize;

        //---------------------------------
        // output jpg heander
        if(pHOutJStream->jStreamInfo.streamType == JPG_STREAM_MEM) //Benson add for async mode side effect.
        {  
            //Original coding.
            if( pJpgDev->pExifInfo != NULL )   //it seems all to change
            {
                (void)pJOutStreamDesc->jOut_buf(pHOutJStream, pJpgDev->pHJComm->pJHdrData, 2, NULL);
                (void)pJOutStreamDesc->jOut_buf(pHOutJStream, pJpgDev->pExifInfo, pJpgDev->exifInfoSize, NULL);
                (void)pJOutStreamDesc->jOut_buf(pHOutJStream, pJpgDev->pHJComm->pJHdrData, (pJpgDev->pHJComm->jHdrDataSize - 2u), NULL);
            }
            else
            {
                (void)pJOutStreamDesc->jOut_buf(pHOutJStream, pJpgDev->pHJComm->pJHdrData, pJpgDev->pHJComm->jHdrDataSize, NULL);
            }
            // output entropy code data
            (void)pJOutStreamDesc->jOut_buf(pHOutJStream, pJpgDev->pHJComm->pSysBsBufAddr, pJpgDev->pHJComm->sysValidBsBufSize, NULL);
         }
         else
         {  
            //change to async mode , save all header and data to fwrite once time.  
            if( pJpgDev->pExifInfo != NULL ) 
            {
                AsyncSaveAllAddr =malloc(totalEncLeng);    
                if(AsyncSaveAllAddr == NULL)        
                {
                	(void)printf("malloc AsyncSaveAllAddr fail\n");
					result = JPG_ERR_ALLOCATE_FAIL;
					break;
				}
				else
				{
	                (void)memcpy(AsyncSaveAllAddr ,  pJpgDev->pHJComm->pJHdrData ,2);
	                (void)memcpy(AsyncSaveAllAddr+2 ,  pJpgDev->pExifInfo ,pJpgDev->exifInfoSize);
	                (void)memcpy(AsyncSaveAllAddr+2+pJpgDev->exifInfoSize ,  pJpgDev->pHJComm->pJHdrData,(pJpgDev->pHJComm->jHdrDataSize - 2u));

	                (void)memcpy(AsyncSaveAllAddr+2+pJpgDev->exifInfoSize+(pJpgDev->pHJComm->jHdrDataSize - 2u) ,  pJpgDev->pHJComm->pSysBsBufAddr,pJpgDev->pHJComm->sysValidBsBufSize);
	                (void)pJOutStreamDesc->jOut_buf(pHOutJStream, AsyncSaveAllAddr, totalEncLeng, NULL);
	                free(AsyncSaveAllAddr);
				}
            }else
            {
                AsyncSaveAllAddr =malloc(totalEncLeng);
                if(AsyncSaveAllAddr ==NULL)        
                {
                	(void)printf("malloc AsyncSaveAllAddr fail\n");
					result = JPG_ERR_ALLOCATE_FAIL;
					break;
				}
				else
				{
	                (void)memcpy(AsyncSaveAllAddr ,  pJpgDev->pHJComm->pJHdrData ,pJpgDev->pHJComm->jHdrDataSize);
	                (void)memcpy(AsyncSaveAllAddr+pJpgDev->pHJComm->jHdrDataSize ,  pJpgDev->pHJComm->pSysBsBufAddr,pJpgDev->pHJComm->sysValidBsBufSize);
	                (void)pJOutStreamDesc->jOut_buf(pHOutJStream, AsyncSaveAllAddr, totalEncLeng, NULL);
	                free(AsyncSaveAllAddr);
				}
            }
        }


        pJpgDev->pHJComm->sysValidBsBufSize = 0; // for next section encode
        remain_H -= pJpgDev->encSectHight;

        // ------------------------------------
        // multi-encode not verify
        while( (pJpgDev->bPartialEnc == true) && (remain_H > 0u) )
        {
            JPG_STREAM_INFO     *pJInStreamInfo = &pJpgDev->hJInStream.jStreamInfo;

#if 0
            // reserve code, read new yuv source (ex. scanner) not verify
            JPG_STREAM_DESC     *pJInStreamDesc = &pJpgDev->hJInStream.jStreamDesc;
            if( pJInStreamDesc->jFull_buf )
                pJInStreamDesc->jFull_buf(&pJpgDev->hJInStream, pYuvBufAddr,
                                          slice*N, 0, 0);
#else
            // update input addrY/U/V
            pHJComm->jInBufInfo[0].pBufAddr += (pJpgDev->encSectHight * pJInStreamInfo->jstream.mem[0].pitch);
            switch( pJpgDev->initParam.outColorSpace )
            {
                case JPG_COLOR_SPACE_YUV422:
                    pHJComm->jInBufInfo[1].pBufAddr += (pJpgDev->encSectHight * pJInStreamInfo->jstream.mem[1].pitch);
                    pHJComm->jInBufInfo[2].pBufAddr += (pJpgDev->encSectHight * pJInStreamInfo->jstream.mem[2].pitch);
                    break;

                case JPG_COLOR_SPACE_YUV420:
                    pHJComm->jInBufInfo[1].pBufAddr += ((pJpgDev->encSectHight * pJInStreamInfo->jstream.mem[1].pitch) >> 1);
                    pHJComm->jInBufInfo[2].pBufAddr += ((pJpgDev->encSectHight * pJInStreamInfo->jstream.mem[2].pitch) >> 1);
                    break;
				default:
					; /*Do nothing*/
					break;
            }
#endif
            // encode fire
            pJpgDev->encSectHight = (remain_H < pJpgDev->encSectHight) ? remain_H : pJpgDev->encSectHight;

            remain_H -= pJpgDev->encSectHight;

            result = jComm_Fire(pHJComm, (remain_H > 0u) ? false : true, NULL);
            if( result != JPG_ERR_OK )
            {
                jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg err 0x%x !! ", result);
                break;
            }

            // out stream
            (void)pJOutStreamDesc->jOut_buf(pHOutJStream, pJpgDev->pHJComm->pSysBsBufAddr, pJpgDev->pHJComm->sysValidBsBufSize, NULL);

            totalEncLeng += pJpgDev->pHJComm->sysValidBsBufSize;
            pJpgDev->pHJComm->sysValidBsBufSize = 0; // for next section encode
            (void)usleep(1000);
        }

        //jpg_msg(1, " enc size = %f kb\r\n", (float)totalEncLeng/1024.0f);
        *pJpgSize = totalEncLeng;

    }while (false);

    if( result != JPG_ERR_OK )
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR,"%s() err 0x%x !", __FUNCTION__, result);
    }

    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ITEJPG);
    return result;
}
#endif

#if (CONFIG_JPG_CODEC_DESC_ENC_MJPG_DESC)
static JPG_ERR
_jpg_enc_mjpg_setup(
    JPG_DEV     *pJpgDev)
{
    JPG_ERR             result = JPG_ERR_OK;
    JCOMM_INIT_PARAM    jCommInitParam = {0};

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ITEJPG, "0x%x\n", pJpgDev);

    do{
        JCOMM_HANDLE        *pHJComm = pJpgDev->pHJComm;
        JPG_STREAM_HANDLE   *pHInJStream = &pJpgDev->hJInStream;
        uint32_t i;

        //-----------------------------------------------
        // jpg H/W init
        jCommInitParam.pHInJStream  = &pJpgDev->hJInStream;
        jCommInitParam.pHOutJStream = &pJpgDev->hJOutStream;
        jCommInitParam.codecType    = JPG_CODEC_ENC_MJPG;
        jCommInitParam.encQuality   = pJpgDev->initParam.encQuality;
        jCommInitParam.encWidth     = pJpgDev->initParam.width;
        jCommInitParam.encHeight    = pJpgDev->initParam.height;
        jCommInitParam.encColorSpace = pJpgDev->initParam.outColorSpace;
		jCommInitParam.exColorSpace = pJpgDev->initParam.exColorSpace;

        result = jComm_Init(pHJComm, &jCommInitParam, NULL);
        if( result != JPG_ERR_OK )
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " error !!");
            result = JPG_ERR_ALLOCATE_FAIL;
            break;
        }

        pHJComm->pJFrmComp  = &pJpgDev->jPrsInfo.jFrmComp;
        pHJComm->pJDispInfo = &pJpgDev->jDispInfo;

        //---------------------------------
        // set input stream info
        pHInJStream->compCnt = pHInJStream->jStreamInfo.validCompCnt;
        for(i = 0; i < pHInJStream->compCnt; i++)
        {
            pHJComm->jInBufInfo[i].pBufAddr  = pHInJStream->jStreamInfo.jstream.mem[i].pAddr;
            pHJComm->jInBufInfo[i].bufLength = pHInJStream->jStreamInfo.jstream.mem[i].length;
            pHJComm->jInBufInfo[i].pitch     = pHInJStream->jStreamInfo.jstream.mem[i].pitch;
        }

        result = jComm_Setup(pHJComm, NULL);

    }while (false);

    if( result != JPG_ERR_OK )
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR,"%s() err 0x%x !", __FUNCTION__, result);
    }

    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ITEJPG);
    return result;
}

static JPG_ERR
_jpg_enc_mjpg_fire(
    JPG_DEV         *pJpgDev,
    JPG_BUF_INFO    *pSysBsBufInfo,
    uint32_t        *pJpgSize)
{
    JPG_ERR             result = JPG_ERR_OK;

    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ITEJPG, "0x%x\n", pJpgDev);

    do{
        uint32_t            totalEncLeng = 0;
        JCOMM_HANDLE        *pHJComm = pJpgDev->pHJComm;
        JPG_STREAM_HANDLE   *pHOutJStream = &pJpgDev->hJOutStream;
        JPG_STREAM_DESC     *pJOutStreamDesc = &pHOutJStream->jStreamDesc;

        if( pJOutStreamDesc->jOut_buf == NULL )
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg err, No out stream API!! ");
            result = JPG_ERR_INVALID_PARAMETER;
            break;
        }

        // output jpg header info
        (void)pJOutStreamDesc->jOut_buf(pHOutJStream, pJpgDev->pHJComm->pJHdrData, pJpgDev->pHJComm->jHdrDataSize, NULL);

        // for performance, make jpg H/W directly write to AP Bit Stream buffer.
        pJpgDev->pHJComm->pSysBsBufAddr = (uint8_t*)pHOutJStream->curBsPos;
        pJpgDev->pHJComm->sysBsBufSize  = pHOutJStream->streamSize - pJpgDev->pHJComm->jHdrDataSize;

        result = jComm_Fire(pHJComm, true, NULL);
        if( result != JPG_ERR_OK )
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg err 0x%x !! ", result);
            break;
        }

        totalEncLeng = pJpgDev->pHJComm->sysValidBsBufSize + pJpgDev->pHJComm->jHdrDataSize;

        jpg_msg(1, " enc size = %f kb\r\n", (float)totalEncLeng/1024.0f);
        *pJpgSize = totalEncLeng;

    }while (false);

    if( result != JPG_ERR_OK )
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR,"%s() err 0x%x !", __FUNCTION__, result);
    }

    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ITEJPG);
    return result;
}
#endif

//=============================================================================
//                  Public Function Definition
//=============================================================================
JPG_ERR
iteJpg_CreateHandle(
    HJPG            **pHJpeg,
    JPG_INIT_PARAM  *pInitParam,
    void            *extraData
    JPG_EXTRA_INFO)
{
    JPG_ERR     result = JPG_ERR_OK;
    JPG_DEV     *pJpgDev = NULL;

    _jpg_trace_caller(" %s: caller=%s()[#%d]\n", __FUNCTION__, _caller, _line);
    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ITEJPG, "0x%x, 0x%x, 0x%x\n", pHJpeg, pInitParam, extraData);

    do{
        if( *pHJpeg != NULL )
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " error, Exist handle !!");
            result = JPG_ERR_INVALID_PARAMETER;
            break;
        }

        // ------------------------
        // craete dev info
        pJpgDev = malloc(sizeof(JPG_DEV));
        if( pJpgDev == NULL )
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " error, allocate fail !!");
            result = JPG_ERR_ALLOCATE_FAIL;
            break;
        }
		
        (void)memset(pJpgDev, 0x0, sizeof(JPG_DEV));
		
        if( pInitParam != NULL)
        {
            (void)memcpy(&pJpgDev->initParam, pInitParam, sizeof(JPG_INIT_PARAM));

			if (pJpgDev->initParam.codecType != JPG_CODEC_DEC_MJPG)
			{
		        _jpg_mutex_lock(JPG_MSG_TYPE_TRACE_ITEJPG, g_jpg_codec_mutex);
			}
		    //set cmdq flag
		    if(pInitParam->bUsingCmdq)
		    {
			    b_JPG_REGS_USE_CMDQ = true;
		    }
	        //--------------------------------------
	        // set handle descriptor by codec type
	        switch( pInitParam->codecType )
	        {
	        #if (CONFIG_JPG_CODEC_DESC_ENC_MJPG_DESC)
	            case JPG_CODEC_ENC_MJPG:
	        #endif
	        #if (CONFIG_JPG_CODEC_DESC_ENCODER_DESC)
	            case JPG_CODEC_ENC_JPG:
	        #endif
	        #if (CONFIG_JPG_CODEC_DESC_ENC_MJPG_DESC) || (CONFIG_JPG_CODEC_DESC_ENCODER_DESC)
	                if( (pInitParam->encQuality > 99u) || (pInitParam->encQuality < 1u) )
	                {
	                    result = JPG_ERR_INVALID_PARAMETER;
	                    jpg_msg_ex(JPG_MSG_TYPE_ERR, " err,  1 < quality(%ld) < 99 !", pInitParam->encQuality);
	                }

	                switch( pInitParam->outColorSpace )
	                {
	                    case JPG_COLOR_SPACE_YUV422:
	                        pJpgDev->initParam.width &= ~0xFu;
	                        pJpgDev->initParam.height &= ~0xFu;
	                        break;
	                    case JPG_COLOR_SPACE_YUV420:
	                        // yuv420 => enc width or height must be 32 alignment in it9070
	                        // I guess that is H/W bug.
	                        #if 1
	                        if( (pJpgDev->initParam.width & 0x1Fu) < (pJpgDev->initParam.height & 0x1Fu) )
	                        {
	                            //pJpgDev->initParam.width &= ~0x1F;
	                            pJpgDev->initParam.width = ((pJpgDev->initParam.width + 31u) / 32u) * 32u;
	                            pJpgDev->initParam.height &= ~0xFu;
	                        }
	                        else
	                        {
	                            //pJpgDev->initParam.height &= ~0x1F;
	                            pJpgDev->initParam.height = ((pJpgDev->initParam.height + 31u) / 32u) * 32u;
	                            pJpgDev->initParam.width &= ~0xFu;
	                        }
							#endif
	                        break;
	                    default:
	                        result = JPG_ERR_INVALID_PARAMETER;
	                        jpg_msg_ex(JPG_MSG_TYPE_ERR, " err, enc only support yuv422 or yuv420 !");
	                        break;
	                }

	                if( result == JPG_ERR_OK )
	                {
	                    // Benson
	                    //jpg_msg(1, " alignment (W, H) from (%d, %d) to (%d, %d) !",
	                   //     pInitParam->width, pInitParam->height,
	                   //     pJpgDev->initParam.width, pJpgDev->initParam.height);
	                }
					(void)jComm_CreateHandle(&pJpgDev->pHJComm, NULL);
	                break;
	        #endif
	        #if (CONFIG_JPG_CODEC_DESC_DEC_MJPG_DESC)
	            case JPG_CODEC_DEC_MJPG:
	        #endif
	            case JPG_CODEC_DEC_JPG_CMD:
	            case JPG_CODEC_DEC_JPG:
	                (void)jComm_CreateHandle(&pJpgDev->pHJComm, NULL);
	                break;

	            default:
	                result = JPG_ERR_INVALID_PARAMETER;
	                jpg_msg_ex(JPG_MSG_TYPE_ERR, " err, wrong parameters !");
	                break;
	        }
        }
    }while (false);

    if( result != JPG_ERR_OK )
    {
        if( pJpgDev != NULL)
        {
            (void)iteJpg_DestroyHandle((HJPG**)&pJpgDev, NULL);
        }
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !", __FUNCTION__, result);
    }

    (*pHJpeg) = (HJPG)pJpgDev;
    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ITEJPG);
    return result;
}

JPG_ERR
iteJpg_DestroyHandle(
    HJPG            **pHJpeg,
    void            *extraData
    JPG_EXTRA_INFO)
{
    JPG_ERR     result = JPG_ERR_OK;
    JPG_DEV     *pJpgDev = (JPG_DEV*)(*pHJpeg);

    _jpg_trace_caller(" %s: caller=%s()[#%d]\n", __FUNCTION__, _caller, _line);
    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ITEJPG, "0x%x, 0x%x\n", pHJpeg, extraData);
    _jpg_verify_handle(JPG_MSG_TYPE_TRACE_ITEJPG, pHJpeg, 0, result);
    _jpg_verify_handle(JPG_MSG_TYPE_TRACE_ITEJPG, (*pHJpeg), 0, result);

    if( pJpgDev != NULL)
    {
        JPG_STREAM_HANDLE   *pHInJStream = &pJpgDev->hJInStream;
        JPG_STREAM_DESC     *pJInStreamDesc = &pJpgDev->hJInStream.jStreamDesc;
        JPG_STREAM_HANDLE   *pHOutJStream = &pJpgDev->hJOutStream;
        JPG_STREAM_DESC     *pJOutStreamDesc = &pJpgDev->hJOutStream.jStreamDesc;
        JPG_PRS_BS_CTRL     *pJPrsBsCtrl = &pJpgDev->jPrsInfo.jPrsBsCtrl;
        JPG_HEAP_TYPE       heapType = JPG_HEAP_DEF;
        JPG_PARSER_INFO     *pJPrsInfo = &pJpgDev->jPrsInfo;
        int                 i;

        switch( pJpgDev->initParam.codecType )
        {
        #if (CONFIG_JPG_CODEC_DESC_DEC_MJPG_DESC)
            case JPG_CODEC_DEC_MJPG:
        #endif
            case JPG_CODEC_DEC_JPG_CMD:
            case JPG_CODEC_DEC_JPG:
                // jpg parser terminate
                heapType = (pJPrsInfo->jFrmComp.bFindHDT == true) ? JPG_HEAP_DEF : JPG_HEAP_STATIC;

                for(i = 0; i < 2; i++)
                {
                    if( pJPrsInfo->jFrmComp.huff_DC[i].pHuffmanTable != NULL )
                    {
                        if( pJInStreamDesc->jFree_mem != NULL )
                        {
                            pJInStreamDesc->jFree_mem(pHInJStream, heapType, pJPrsInfo->jFrmComp.huff_DC[i].pHuffmanTable);
                        }
                        pJPrsInfo->jFrmComp.huff_DC[i].pHuffmanTable = NULL;
                    }

                    if( pJPrsInfo->jFrmComp.huff_AC[i].pHuffmanTable != NULL )
                    {
                        if( pJInStreamDesc->jFree_mem != NULL )
                        {
                            pJInStreamDesc->jFree_mem(pHInJStream, heapType, pJPrsInfo->jFrmComp.huff_AC[i].pHuffmanTable);
                        }

                        pJPrsInfo->jFrmComp.huff_AC[i].pHuffmanTable = NULL;
                    }
                }
                break;

        #if (CONFIG_JPG_CODEC_DESC_ENCODER_DESC)
            case JPG_CODEC_ENC_JPG:
                break;
        #endif

        #if (CONFIG_JPG_CODEC_DESC_ENC_MJPG_DESC)
            case JPG_CODEC_ENC_MJPG:
                break;
        #endif
			default:
			    ; /*Do nothing*/
				break;
        }

        result = jComm_deInit(pJpgDev->pHJComm, NULL);

        (void)jComm_DestroyHandle(&pJpgDev->pHJComm, NULL);

        if( (pJPrsBsCtrl->bsPrsBuf != NULL) &&
            (pJInStreamDesc->jFree_mem != NULL) )
        {
            pJInStreamDesc->jFree_mem(pHInJStream, JPG_HEAP_BS_BUF, pJPrsBsCtrl->bsPrsBuf);
            pJPrsBsCtrl->bsPrsBuf = NULL;
        }

        if( pJInStreamDesc->jClose_stream != NULL)
        {
            (void)pJInStreamDesc->jClose_stream(pHInJStream, NULL);
        }
        if( pJOutStreamDesc->jClose_stream != NULL)
        {
            (void)pJOutStreamDesc->jClose_stream(pHOutJStream, NULL);
        }
        
        if (pJpgDev->initParam.codecType != JPG_CODEC_DEC_MJPG)
		{
			_jpg_mutex_unlock(JPG_MSG_TYPE_TRACE_ITEJPG, g_jpg_codec_mutex);
		}
		
        free(pJpgDev);
        *pHJpeg = NULL;
		b_JPG_REGS_USE_CMDQ = false;
    }
    
    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ITEJPG);
    return result;
}

JPG_ERR
iteJpg_SetStreamInfo(
    HJPG            *pHJpeg,
    JPG_STREAM_INFO *pInStreamInfo,
    JPG_STREAM_INFO *pOutStreamInfo,
    void            *extraData
    JPG_EXTRA_INFO)
{
    JPG_ERR     result = JPG_ERR_OK;
    JPG_DEV     *pJpgDev = (JPG_DEV*)pHJpeg;

    _jpg_trace_caller(" %s: caller=%s()[#%d]\n", __FUNCTION__, _caller, _line);
    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ITEJPG, "0x%x, 0x%x, 0x%x, 0x%x\n", pHJpeg, pInStreamInfo, pOutStreamInfo, extraData);
    _jpg_verify_handle(JPG_MSG_TYPE_TRACE_ITEJPG, pHJpeg, 0, result);

    if( pJpgDev->status != JPG_STATUS_FAIL )
    {
    #define SET_UNKNOW_INFO     0xFF
    #define SET_INPUT_INFO      0x11
    #define SET_OUTPUT_INFO     0xCC

        int                 curState = SET_INPUT_INFO;
        JPG_STREAM_INFO     *pJStreamInfo = NULL;
        JPG_STREAM_HANDLE   *pHJtStream = NULL;

        while( curState != SET_UNKNOW_INFO )
        {
            JPG_STREAM_DESC     *pJStreamDesc = NULL;
            JPG_STREAM_INFO     *pStreamInfo = NULL;
            bool                bSkip = true;

            switch( curState )
            {
                case SET_INPUT_INFO:
                    if( pInStreamInfo != NULL)
                    {
                        pHJtStream   = &pJpgDev->hJInStream;
                        pJStreamInfo = &pJpgDev->hJInStream.jStreamInfo;
                        pJStreamInfo->streamIOType = JPG_STREAM_IO_READ;
                        bSkip = false;
                    }

                    pStreamInfo = pInStreamInfo;
                    curState = SET_OUTPUT_INFO;
                    break;

                case SET_OUTPUT_INFO:
                    if( pOutStreamInfo != NULL)
                    {
                        pHJtStream   = &pJpgDev->hJOutStream;
                        pJStreamInfo = &pJpgDev->hJOutStream.jStreamInfo;
                        pJStreamInfo->streamIOType = JPG_STREAM_IO_WRITE;
                        bSkip = false;
                    }

                    pStreamInfo = pOutStreamInfo;
                    curState = SET_UNKNOW_INFO;
                    break;

                default :
					; /*Do nothing*/
                    break;
            }

            if( bSkip == false )
            {
                //--------------------------------
                // close old stream
                {
	                if( pStreamInfo->streamType != JPG_STREAM_UNKNOW )
	                {
	                    pJStreamDesc = &pHJtStream->jStreamDesc;
	                    if( pJStreamDesc->jClose_stream != NULL)
	                    {
	                        while( pHJtStream->bOpened == true )
	                        {
		                        result = pJStreamDesc->jClose_stream(pHJtStream, NULL);
								pHJtStream->bOpened = false;
		  					 	(void)usleep(1000);
	                        }
	                    }
	                }

                    (void)memcpy(&pHJtStream->jStreamInfo, pStreamInfo, sizeof(JPG_STREAM_INFO));

	                //--------------------------------------
	                // reset default handle descriptor by input stream type
	                switch( pStreamInfo->streamType )
	                {
	                    case JPG_STREAM_FILE:   pHJtStream->jStreamDesc = jpg_stream_file_desc;   break;
	                    case JPG_STREAM_MEM:    pHJtStream->jStreamDesc = jpg_stream_mem_desc;    break;

	                    case JPG_STREAM_CUSTOMER:       break;
	                    default:
	                        result = JPG_ERR_INVALID_PARAMETER;
	                        jpg_msg_ex(JPG_MSG_TYPE_ERR, " err, wrong parameters !");
	                        break;
	                }
                }
                // reset default handler if customer want to do.
                if( pHJtStream->jStreamInfo.jpg_reset_stream_info != NULL)
                {
                    (void)pHJtStream->jStreamInfo.jpg_reset_stream_info(pHJtStream, NULL);
                }
                //-------------------------------
                // open new stream
                pJStreamDesc = &pHJtStream->jStreamDesc;
                if( pJStreamDesc->jOpen_stream != NULL)
                {
                    result = pJStreamDesc->jOpen_stream(pHJtStream, NULL);
                    if( result == JPG_ERR_OK )
                    {
						pHJtStream->bOpened = true;
                    }
                }
            }
        }
    }

    if( result != JPG_ERR_OK )
    {
        pJpgDev->status = JPG_STATUS_FAIL;
        jpg_msg_ex(JPG_MSG_TYPE_ERR,"%s() err 0x%x !", __FUNCTION__, result);
    }

    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ITEJPG);
    return result;
}

JPG_ERR
iteJpg_Parsing(
    HJPG            *pHJpeg,
    JPG_BUF_INFO    *pEntropyBufInfo,
    void            *extraData)
    //JPG_EXTRA_INFO)
{
    JPG_ERR     result = JPG_ERR_OK;
    JPG_DEV     *pJpgDev = (JPG_DEV*)pHJpeg;

    _jpg_trace_caller(" %s: caller=%s()[#%d]\n", __FUNCTION__, _caller, _line);
    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ITEJPG, "0x%x, 0x%x, 0x%x\n", pHJpeg, pEntropyBufInfo, extraData);
    _jpg_verify_handle(JPG_MSG_TYPE_TRACE_ITEJPG, pHJpeg, 0, result);

    if( pJpgDev->status != JPG_STATUS_FAIL )
    {
        JPG_PARSER_INFO     *pJPrsInfo = &pJpgDev->jPrsInfo;
        JPG_PRS_BS_CTRL     *pJPrsBsCtrl = &pJPrsInfo->jPrsBsCtrl;
        JPG_STREAM_HANDLE   *pHInJStream = &pJpgDev->hJInStream;
        JPG_STREAM_DESC     *pJInStreamDesc = &pHInJStream->jStreamDesc;
        JPG_FRM_COMP        *pJFrmComp = &pJPrsInfo->jFrmComp;
        JPG_DISP_INFO       *pJDispInfo = &pJpgDev->jDispInfo;
        JPG_EXIF_INFO       *pExifInfo = &pJPrsInfo->jAttrib.exifInfo;
        JPG_RECT                *pJrect = (JPG_RECT*)extraData; //Benson
        uint32_t            max_width = 0, max_height = 0;

        if( (pJpgDev->initParam.codecType != JPG_CODEC_DEC_JPG) &&
            (pJpgDev->initParam.codecType != JPG_CODEC_DEC_JPG_CMD) &&
            (pJpgDev->initParam.codecType != JPG_CODEC_DEC_MJPG) )
        {
            goto end;
        }

        //--------------------------------------
        // check parser bs buf
        if( pJPrsBsCtrl->bsPrsBuf == NULL ) 
        {
            uint32_t      realSzie = 0;

            // check endian
            pJPrsInfo->bLEndian = false;

            // heap parser bs buf
            if( pJInStreamDesc->jControl != NULL)
            {
                (void)pJInStreamDesc->jControl(pHInJStream, (uint32_t)JPG_STREAM_CMD_GET_BS_BUF_SIZE,
                                       &pJPrsBsCtrl->bsPrsBufMaxLeng, NULL);
            }

            if( pJInStreamDesc->jHeap_mem != NULL)
            {
                pJPrsBsCtrl->bsPrsBuf = pJInStreamDesc->jHeap_mem(pHInJStream, JPG_HEAP_BS_BUF,
                                                                  pJPrsBsCtrl->bsPrsBufMaxLeng, &realSzie);
            }
            if( pJPrsBsCtrl->bsPrsBuf == NULL )
            {
                result = JPG_ERR_ALLOCATE_FAIL;
                jpg_msg_ex(JPG_MSG_TYPE_ERR, " mallocate fail !!");
                goto end;
            }

            pJPrsBsCtrl->bsPrsBufMaxLeng = realSzie;  // Real heap size can be used to debug, but I just replay to bsBufMaxLeng_A
        }

        //-------------------------------------------------
        // get stream total length
        pJPrsInfo->fileLength = pHInJStream->streamSize;  //>256KB ,using multi section
        pJPrsInfo->remainSize = pJPrsInfo->fileLength;

#ifdef CFG_CPU_WB
        ithFlushDCacheRange((void*)(pJPrsBsCtrl->bsPrsBuf), pJPrsInfo->fileLength);
        ithFlushMemBuffer();
#endif

        //-------------------------------------------------
        // fill buf
        if( (pJPrsInfo->fileLength > pJPrsBsCtrl->bsPrsBufMaxLeng) &&
            (pJpgDev->initParam.codecType != JPG_CODEC_DEC_MJPG) )
        {
            // need multi-section
            if( pJInStreamDesc->jFull_buf != NULL)
            {
                (void)pJInStreamDesc->jFull_buf(pHInJStream, pJPrsBsCtrl->bsPrsBuf,
                                        pJPrsBsCtrl->bsPrsBufMaxLeng,
                                        &pJPrsBsCtrl->realSize, NULL);
            }
            pJPrsInfo->bMultiSection = true;
            if( pJPrsBsCtrl->realSize != pJPrsBsCtrl->bsPrsBufMaxLeng )
            {
                jpg_msg_ex(JPG_MSG_TYPE_ERR, "Fill buffer something wrong !! (%ld, %ld) ", pJPrsInfo->remainSize, pJPrsBsCtrl->bsPrsBufMaxLeng);
            }
        }
        else
        {
            // one section
            if( pJInStreamDesc->jFull_buf != NULL)
            {
                (void)pJInStreamDesc->jFull_buf(pHInJStream, (void*)pJPrsBsCtrl->bsPrsBuf,
                                        pJPrsInfo->fileLength,
                                        &pJPrsBsCtrl->realSize, NULL);
            }
            pJPrsInfo->bMultiSection = false;
        }

        pJPrsInfo->remainSize -= pJPrsBsCtrl->realSize;

        pJPrsBsCtrl->b1stSection = true;
        pJPrsBsCtrl->curPos = 0;

        switch( pJpgDev->initParam.codecType )
        {
            case JPG_CODEC_DEC_JPG_CMD:
                max_width  = JPG_MAX_WIDTH;
                max_height = JPG_MAX_HEIGHT;

                pJpgDev->initParam.decType = JPG_DEC_PRIMARY;
                break;

            case JPG_CODEC_DEC_JPG:
                max_width  = JPG_MAX_WIDTH;
                max_height = JPG_MAX_HEIGHT;

                if( (pJpgDev->initParam.decType != JPG_DEC_PRIMARY) ||
                    (pJpgDev->initParam.bExifParsing == true) )
                {
                    //----------------------------------------
                    // parsing APP section
                    result = jPrs_AppParser(pHInJStream, pJPrsInfo);
                    if( result != JPG_ERR_OK )
                    {
                        jpg_msg_ex(JPG_MSG_TYPE_ERR, " err ! ");
                        goto end;
                    }

                    //----------------------------------------
                    // set decode bs buffer info and select decode type (primary or thumbnail)
                    // If CMYK jpg, always decode primary.
                    if( pJPrsInfo->jAttrib.bCMYK == true )
                    {
                        pJpgDev->initParam.decType = JPG_DEC_PRIMARY;
                    }	

                    result = _Verify_DecType(pHInJStream, pJPrsInfo, &pJpgDev->initParam);
                    if( result != JPG_ERR_OK )
                    {
                        jpg_msg_ex(JPG_MSG_TYPE_ERR, " err ! ");
                        goto end;
                    }
                }
                break;

            case JPG_CODEC_DEC_MJPG:
                max_width  = MJPG_MAX_WIDTH;
                max_height = MJPG_MAX_HEIGHT;

                pJpgDev->initParam.decType = JPG_DEC_PRIMARY;
                break;

            default:
                result = JPG_ERR_INVALID_PARAMETER;
                jpg_msg_ex(JPG_MSG_TYPE_ERR, " err, wrong parameters !");
                break;
        }

		if( result != JPG_ERR_OK )
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " err ! ");
                goto end;
        }

        //----------------------------------------------
        // parsing Base section
        do{
            result = jPrs_BaseParser(pHInJStream, pJPrsInfo);

            // clear table info
            if( result == JPG_ERR_HDER_ONLY_TABLES )
            {
                int              i;
                JPG_HEAP_TYPE    heapType = JPG_HEAP_DEF;

                heapType = (pJPrsInfo->jFrmComp.bFindHDT == true) ? JPG_HEAP_DEF : JPG_HEAP_STATIC;
                for(i = 0; i < 2; i++)
                {
                    if( pJPrsInfo->jFrmComp.huff_DC[i].pHuffmanTable != NULL )
                    {
                        if( pJInStreamDesc->jFree_mem != NULL )
                        {
                            pJInStreamDesc->jFree_mem(pHInJStream, heapType, pJPrsInfo->jFrmComp.huff_DC[i].pHuffmanTable);
                        }
                        pJPrsInfo->jFrmComp.huff_DC[i].pHuffmanTable = NULL;
                    }

                    if( pJPrsInfo->jFrmComp.huff_AC[i].pHuffmanTable != NULL )
                    {
                        if( pJInStreamDesc->jFree_mem != NULL )
                        {
                            pJInStreamDesc->jFree_mem(pHInJStream, heapType, pJPrsInfo->jFrmComp.huff_AC[i].pHuffmanTable);
                        }
                        pJPrsInfo->jFrmComp.huff_AC[i].pHuffmanTable = NULL;
                    }
                }
            }
            else if( result == JPG_ERR_JPROG_STREAM)
            {
              goto end;
            }
			else
			{
				; /* do nothing */
			}

        }while( result == JPG_ERR_HDER_ONLY_TABLES );

        if( result != JPG_ERR_OK )
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " err ! ");
            goto end;
        }

        if( (pJFrmComp->imgWidth == 0u) || (pJFrmComp->imgHeight == 0u) )
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " err, jpg stream get wrong W(%ld)/H(%ld) !! ", pJFrmComp->imgWidth, pJFrmComp->imgHeight);
            result = JPG_ERR_INVALID_PARAMETER;
            goto end;
        }

        // error handle, 1:1 scaling
        if( (pJpgDev->initParam.width == 0u) || (pJpgDev->initParam.height == 0u) )
        {
            if( pJpgDev->initParam.codecType != JPG_CODEC_DEC_JPG_CMD )
            {
                jpg_msg_ex(JPG_MSG_TYPE_ERR, " Waring, jpg wrong output resolution W(%ld)/H(%ld), reset to W(%ld)/H(%ld) !!", pJpgDev->initParam.width, pJpgDev->initParam.height, pJFrmComp->imgWidth, pJFrmComp->imgHeight);
            }
            pJpgDev->initParam.width  = pJFrmComp->imgWidth;
            pJpgDev->initParam.height = pJFrmComp->imgHeight;
        }

        if( (pJFrmComp->imgWidth > max_width) || (pJFrmComp->imgHeight > max_height) )
        {
            result = JPG_ERR_HW_NOT_SUPPORT;
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " Resolution not support !!");
            goto end;
        }

        if( (pJpgDev->initParam.codecType == JPG_CODEC_DEC_MJPG) ||
            (pJpgDev->initParam.codecType == JPG_CODEC_DEC_JPG_CMD) )
        {
            if( pEntropyBufInfo != NULL)
            {
                pEntropyBufInfo->pBufAddr  = pJPrsBsCtrl->bsPrsBuf + pJPrsBsCtrl->curPos;
                pEntropyBufInfo->bufLength = pJPrsBsCtrl->realSize - pJPrsBsCtrl->curPos;
            }

			pJDispInfo->srcW = pJFrmComp->imgWidth;
            pJDispInfo->dstW = pJDispInfo->srcW;
			pJDispInfo->srcH = pJFrmComp->imgHeight;
            pJDispInfo->dstH = pJDispInfo->srcH;
            pJDispInfo->srcX = 0;
			pJDispInfo->srcY = 0;
            pJDispInfo->dstX = 0;
			pJDispInfo->dstY = 0;

			/*
			//if(pJpgDev->initParam.codecType == JPG_CODEC_DEC_MJPG )
            //goto end; // when MJPG, jpg parsing finish  
			*/
        }

        //----------------------------------
        // Set Orientation, performance issue
        _Set_Exif_Orientation(
            pJpgDev->initParam.decType,
            pJpgDev->initParam.rotType,
            pJpgDev->initParam.bExifOrientation,
            pExifInfo->primaryOrientation,
            pJDispInfo);

        //----------------------------------
        // set display info (fit/cut/center) for H/W module
        switch( pJpgDev->initParam.outColorSpace )
        {
            case JPG_COLOR_SPACE_RGB565:
            case JPG_COLOR_SPACE_ARGB4444:
            case JPG_COLOR_SPACE_ARGB8888:
            case JPG_COLOR_SPACE_YUV422:        // for transcode
            case JPG_COLOR_SPACE_YUV420:        // for transcode
		    case JPG_COLOR_SPACE_YUV444:
                pJDispInfo->outColorSpace = pJpgDev->initParam.outColorSpace;
                break;

            default:
                result = JPG_ERR_INVALID_PARAMETER;
                jpg_msg_ex(JPG_MSG_TYPE_ERR, " err, wrong parameters !");
                break;
        }

        pJDispInfo->colorCtrl = pJpgDev->initParam.colorCtl;
        pJDispInfo->bCMYK     = pJPrsInfo->jAttrib.bCMYK;
        // calculate src/dest range

        switch( pJpgDev->initParam.dispMode )
        {
            case JPG_DISP_FIT:
                {
                    _genParam_Fit(pJDispInfo,
                                  pJpgDev->initParam.width, pJpgDev->initParam.height,
                                  pJFrmComp->imgWidth, pJFrmComp->imgHeight);
                    pJrect->x = pJDispInfo->dstX;
                    pJrect->y = pJDispInfo->dstY;
                    pJrect->h = pJDispInfo->dstH;
                    pJrect->w = pJDispInfo->dstW;
                }
                break;

            case JPG_DISP_CUT:
            case JPG_DISP_CENTER:
            case JPG_DISP_CUT_PART:
                {
                    uint32_t keepPercentage = (pJpgDev->initParam.dispMode == JPG_DISP_CUT_PART) ?
                                              pJpgDev->initParam.keepPercentage :
                                                  ((pJpgDev->initParam.dispMode == JPG_DISP_CENTER) ? 100u : 0u);

                    _genParam_CutPart(pJDispInfo,
                                      pJpgDev->initParam.width, pJpgDev->initParam.height,
                                      pJFrmComp->imgWidth, pJFrmComp->imgHeight,
                                      keepPercentage);
                    pJrect->x = pJDispInfo->dstX;
                    pJrect->y = pJDispInfo->dstY;
                    pJrect->h = pJDispInfo->dstH;
                    pJrect->w = pJDispInfo->dstW;
                }
                break;
            case JPG_DISP_CUT_BY_RECT:
                {
                    _genParam_CutByRect(pJDispInfo,
                                  pJpgDev->initParam.width, pJpgDev->initParam.height,
                                  pJFrmComp->imgWidth, pJFrmComp->imgHeight);
                    pJrect->x = pJDispInfo->dstX;
                    pJrect->y = pJDispInfo->dstY;
                    pJrect->h = pJDispInfo->dstH;
                    pJrect->w = pJDispInfo->dstW;
                }
                break;
            default:
                result = JPG_ERR_INVALID_PARAMETER;
                jpg_msg_ex(JPG_MSG_TYPE_ERR, " err, wrong parameters !");
                break;
        }
    }

end:
    if( (result != JPG_ERR_OK) && (result != JPG_ERR_JPROG_STREAM) )
    {
        pJpgDev->status = JPG_STATUS_FAIL;
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() err 0x%x !", __FUNCTION__, result);
    }
    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ITEJPG);
    return result;
}

JPG_ERR
iteJpg_Setup(
    HJPG            *pHJpeg,
    void            *extraData
    JPG_EXTRA_INFO)
{
    JPG_ERR     result = JPG_ERR_OK;
    JPG_DEV     *pJpgDev = (JPG_DEV*)pHJpeg;

    _jpg_trace_caller(" %s: caller=%s()[#%d]\n", __FUNCTION__, _caller, _line);
    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ITEJPG, "0x%x, 0x%x\n", pHJpeg, extraData);
    _jpg_verify_handle(JPG_MSG_TYPE_TRACE_ITEJPG, pHJpeg, 0, result);

	if (pJpgDev->initParam.codecType == JPG_CODEC_DEC_MJPG)
	{
        _jpg_mutex_lock(JPG_MSG_TYPE_TRACE_ITEJPG, g_jpg_codec_mutex);
	}
    if( pJpgDev->status != JPG_STATUS_FAIL )
    {
        switch( pJpgDev->initParam.codecType )
        {
            case JPG_CODEC_DEC_JPG_CMD: result = _jpg_dec_cmd_setup(pJpgDev);   break;
            case JPG_CODEC_DEC_JPG:     result = _jpg_dec_setup(pJpgDev);       break;

        #if (CONFIG_JPG_CODEC_DESC_DEC_MJPG_DESC)
            case JPG_CODEC_DEC_MJPG:    result = _jpg_dec_mjpg_setup(pJpgDev);  break;
        #endif

        #if (CONFIG_JPG_CODEC_DESC_ENCODER_DESC)
            case JPG_CODEC_ENC_JPG:     result = _jpg_enc_setup(pJpgDev);       break;
        #endif

        #if (CONFIG_JPG_CODEC_DESC_ENC_MJPG_DESC)
            case JPG_CODEC_ENC_MJPG:    result = _jpg_enc_mjpg_setup(pJpgDev);  break;
        #endif

            default:
                result = JPG_ERR_INVALID_PARAMETER;
                jpg_msg_ex(JPG_MSG_TYPE_ERR, " err, wrong parameters !");
                break;
        }
    }

    if( result != JPG_ERR_OK )
    {
        pJpgDev->status = JPG_STATUS_FAIL;
        jpg_msg_ex(JPG_MSG_TYPE_ERR,"%s() err 0x%x !", __FUNCTION__, result);
    }

    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ITEJPG);
    return result;
}


JPG_ERR
iteJpg_Process(
    HJPG            *pHJpeg,
    JPG_BUF_INFO    *pStreamBufInfo,
    uint32_t        *pJpgSize,
    void            *extraData
    JPG_EXTRA_INFO)
{
    JPG_ERR     result = JPG_ERR_OK;
    JPG_DEV     *pJpgDev = (JPG_DEV*)pHJpeg;

    _jpg_trace_caller(" %s: caller=%s()[#%d]\n", __FUNCTION__, _caller, _line);
    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ITEJPG, "0x%x, 0x%x, 0x%x\n", pHJpeg, pJpgSize, extraData);
    _jpg_verify_handle(JPG_MSG_TYPE_TRACE_ITEJPG, pHJpeg, 0, result);

    if( pJpgDev->status != JPG_STATUS_FAIL )
    {
        switch( pJpgDev->initParam.codecType )
        {
            case JPG_CODEC_DEC_JPG:
                result = _jpg_dec_fire(pJpgDev);
                break;

            case JPG_CODEC_DEC_JPG_CMD:
            case JPG_CODEC_DEC_MJPG:
                {
                    JPG_PRS_BS_CTRL     *pJPrsBsCtrl = &pJpgDev->jPrsInfo.jPrsBsCtrl;
                    JPG_BUF_INFO        bsBufInfo = {0};

                    if( pStreamBufInfo != NULL)
                    {
                        (void)memcpy(&bsBufInfo, pStreamBufInfo, sizeof(JPG_BUF_INFO));
                    }
                    else
                    {
                        bsBufInfo.pBufAddr  = pJPrsBsCtrl->bsPrsBuf + pJPrsBsCtrl->curPos;
                        bsBufInfo.bufLength = pJPrsBsCtrl->realSize - pJPrsBsCtrl->curPos;
                    }

                    if( pJpgDev->initParam.codecType == JPG_CODEC_DEC_JPG_CMD )
                    {
                        result = _jpg_dec_cmd_fire(pJpgDev, &bsBufInfo);
                    }
                #if (CONFIG_JPG_CODEC_DESC_DEC_MJPG_DESC)
                    else if( pJpgDev->initParam.codecType == JPG_CODEC_DEC_MJPG )
                    {
                        result = _jpg_dec_mjpg_fire(pJpgDev, &bsBufInfo);
                    }
					else
					{
						; /* do nothing */
					}
                #endif
                }
                break;


        #if (CONFIG_JPG_CODEC_DESC_ENCODER_DESC)
            case JPG_CODEC_ENC_JPG:
                if( pJpgSize != NULL )
                {
                    result = _jpg_enc_fire(pJpgDev, pStreamBufInfo, pJpgSize);
                }
                break;
        #endif

        #if (CONFIG_JPG_CODEC_DESC_ENC_MJPG_DESC)
            case JPG_CODEC_ENC_MJPG:
                if( pJpgSize != NULL )
                {
                    result = _jpg_enc_mjpg_fire(pJpgDev, pStreamBufInfo, pJpgSize);
                }
                break;
        #endif

            default:
                result = JPG_ERR_INVALID_PARAMETER;
                jpg_msg_ex(JPG_MSG_TYPE_ERR, " err, wrong parameters !");
                break;
        }
    }

    if( result != JPG_ERR_OK )
    {
        pJpgDev->status = JPG_STATUS_FAIL;
        jpg_msg_ex(JPG_MSG_TYPE_ERR,"%s() err 0x%x !", __FUNCTION__, result);
    }

    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ITEJPG);
    return result;
}

JPG_ERR
iteJpg_SetBaseOutInfo(
    HJPG            *pHJpeg,
    uint32_t        *pWidth,
    uint32_t        *pHeight,
    void            *extraData
    JPG_EXTRA_INFO)
{
    JPG_ERR     result = JPG_ERR_OK;
    JPG_DEV     *pJpgDev = (JPG_DEV*)pHJpeg;

    _jpg_trace_caller(" %s: caller=%s()[#%d]\n", __FUNCTION__, _caller, _line);
    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ITEJPG, "0x%x, 0x%x\n", pHJpeg, extraData);
    _jpg_verify_handle(JPG_MSG_TYPE_TRACE_ITEJPG, pHJpeg, 0, result);

    if( pJpgDev->status != JPG_STATUS_FAIL )
    {
        JPG_INIT_PARAM      *pInitParam = &pJpgDev->initParam;
        JPG_FRM_COMP        *pJFrmComp = &pJpgDev->jPrsInfo.jFrmComp;
        JPG_DISP_INFO       *pJDispInfo = &pJpgDev->jDispInfo;

        if( pWidth != NULL )
        {
			pInitParam->width = *pWidth;
        }
        if( pHeight != NULL )
        {
			pInitParam->height = *pHeight;
        }
        // calculate src/dest range
        switch( pJpgDev->initParam.dispMode )
        {
            case JPG_DISP_FIT:
                _genParam_Fit(pJDispInfo,
                              pInitParam->width, pInitParam->height,
                              pJFrmComp->imgWidth, pJFrmComp->imgHeight);
                break;

            case JPG_DISP_CUT:
            case JPG_DISP_CENTER:
            case JPG_DISP_CUT_PART:
                {
                    uint32_t keepPercentage = (pInitParam->dispMode == JPG_DISP_CUT_PART) ?
                                              pInitParam->keepPercentage :
                                                  ((pInitParam->dispMode == JPG_DISP_CENTER) ? 100u : 0u);

                    _genParam_CutPart(pJDispInfo,
                                      pInitParam->width, pInitParam->height,
                                      pJFrmComp->imgWidth, pJFrmComp->imgHeight,
                                      keepPercentage);
                }
                break;
            case JPG_DISP_CUT_BY_RECT:
                _genParam_CutByRect(pJDispInfo,
                                    pJpgDev->initParam.width, pJpgDev->initParam.height,
                                    pJFrmComp->imgWidth, pJFrmComp->imgHeight);
                break;
            default:
                result = JPG_ERR_INVALID_PARAMETER;
                jpg_msg_ex(JPG_MSG_TYPE_ERR, " err, wrong parameters !");
                break;
        }
    }

    if( result != JPG_ERR_OK )
    {
        pJpgDev->status = JPG_STATUS_FAIL;
        jpg_msg_ex(JPG_MSG_TYPE_ERR,"%s() err 0x%x !", __FUNCTION__, result);
    }

    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ITEJPG);
    return result;
}


JPG_ERR
iteJpg_Reset(
    HJPG            *pHJpeg,
    void            *extraData
    JPG_EXTRA_INFO)
{
    JPG_ERR     result = JPG_ERR_OK;
    JPG_DEV     *pJpgDev = (JPG_DEV*)pHJpeg;

    _jpg_trace_caller(" %s: caller=%s()[#%d]\n", __FUNCTION__, _caller, _line);
    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ITEJPG, "0x%x, 0x%x\n", pHJpeg, extraData);
    _jpg_verify_handle(JPG_MSG_TYPE_TRACE_ITEJPG, pHJpeg, 0, result);

    {
        JPG_HEAP_TYPE       heapType = JPG_HEAP_DEF;
        JPG_PARSER_INFO     *pJPrsInfo = &pJpgDev->jPrsInfo;
        JPG_STREAM_HANDLE   *pHInJStream = &pJpgDev->hJInStream;
        JPG_STREAM_DESC     *pJInStreamDesc = &pJpgDev->hJInStream.jStreamDesc;
        JCOMM_INIT_PARAM    jCommInitParam = {0};
        int                 i;

        switch( pJpgDev->initParam.codecType )
        {
        #if (CONFIG_JPG_CODEC_DESC_DEC_MJPG_DESC)
            case JPG_CODEC_DEC_MJPG:
        #endif
            case JPG_CODEC_DEC_JPG:
                // jpg parser terminate
                heapType = (pJPrsInfo->jFrmComp.bFindHDT == true) ? JPG_HEAP_DEF : JPG_HEAP_STATIC;
                for(i = 0; i < 2; i++)
                {
                    if( pJPrsInfo->jFrmComp.huff_DC[i].pHuffmanTable != NULL )
                    {
                        if( pJInStreamDesc->jFree_mem != NULL )
                    {
                            pJInStreamDesc->jFree_mem(pHInJStream, heapType, pJPrsInfo->jFrmComp.huff_DC[i].pHuffmanTable);
                        }
                        pJPrsInfo->jFrmComp.huff_DC[i].pHuffmanTable = NULL;
                    }

                    if( pJPrsInfo->jFrmComp.huff_AC[i].pHuffmanTable != NULL )
                    {
                        if( pJInStreamDesc->jFree_mem != NULL )
                    {
                            pJInStreamDesc->jFree_mem(pHInJStream, heapType, pJPrsInfo->jFrmComp.huff_AC[i].pHuffmanTable);
                        }
                        pJPrsInfo->jFrmComp.huff_AC[i].pHuffmanTable = NULL;
                    }
                }

                switch( pJpgDev->initParam.codecType )
                {
                    case JPG_CODEC_DEC_JPG:
                        if( pJpgDev->initParam.decType == JPG_DEC_PRIMARY )
                        {
                            result = JPG_ERR_HW_NOT_SUPPORT;
                            break;
                        }
                        else
                        {
                            pJpgDev->initParam.decType = JPG_DEC_PRIMARY;
                        }

                        if( (pJPrsInfo->jPrsBsCtrl.bsPrsBuf != NULL) &&
                            (pJInStreamDesc->jFree_mem != NULL) )
                        {
                            pJInStreamDesc->jFree_mem(pHInJStream, JPG_HEAP_BS_BUF, pJPrsInfo->jPrsBsCtrl.bsPrsBuf);
                            pJPrsInfo->jPrsBsCtrl.bsPrsBuf = NULL;
                        }

                        (void)jComm_deInit(pJpgDev->pHJComm, NULL);

                        jCommInitParam.pHInJStream  = &pJpgDev->hJInStream;
                        jCommInitParam.pHOutJStream = &pJpgDev->hJOutStream;

                        if( pJPrsInfo->jAttrib.jBaseLiteInfo[pJpgDev->initParam.decType].bJprog == true )
                        {
                            jCommInitParam.codecType = JPG_CODEC_DEC_JPROG;
                        }
                        else
                        {
                            jCommInitParam.codecType = JPG_CODEC_DEC_JPG;
                        }
                        (void)jComm_Init(pJpgDev->pHJComm, &jCommInitParam, NULL);

                        if( pJInStreamDesc->jSeek_stream != NULL )
                        {
                            (void)pJInStreamDesc->jSeek_stream(pHInJStream, 0, JPG_SEEK_SET, NULL);
                        }
                        pJpgDev->status = JPG_STATUS_IDLE;
                        break;

                    case JPG_CODEC_DEC_MJPG:
                        pJPrsInfo->bSkipHDT = false;
                        pJPrsInfo->jFrmComp.qTable.tableCnt = 0;

						if( (pJPrsInfo->jPrsBsCtrl.bsPrsBuf != NULL) && (pJInStreamDesc->jFree_mem != NULL) )
                        {
                            pJInStreamDesc->jFree_mem(pHInJStream, JPG_HEAP_BS_BUF, pJPrsInfo->jPrsBsCtrl.bsPrsBuf);
                            pJPrsInfo->jPrsBsCtrl.bsPrsBuf = NULL;
                        }

						#if 0  //Open MotionJpeg should not  turn on/off the power repeatly ,so I mark it, Benson 2016/0412
                        jComm_deInit(pJpgDev->pHJComm, 0);

                        jCommInitParam.pHInJStream  = &pJpgDev->hJInStream;
                        jCommInitParam.pHOutJStream = &pJpgDev->hJOutStream;
                        jCommInitParam.codecType    = JPG_CODEC_DEC_MJPG;
                        jComm_Init(pJpgDev->pHJComm, &jCommInitParam, 0);
						#endif

                        pJpgDev->status = JPG_STATUS_IDLE;
                        break;
					default:
						; /*Do nothing*/
						break;
                }
                break;

        #if (CONFIG_JPG_CODEC_DESC_ENCODER_DESC)
            case JPG_CODEC_ENC_JPG:
                break;
        #endif

        #if (CONFIG_JPG_CODEC_DESC_ENC_MJPG_DESC)
            case JPG_CODEC_ENC_MJPG:
                break;
        #endif

            default:
                result = JPG_ERR_INVALID_PARAMETER;
                jpg_msg_ex(JPG_MSG_TYPE_ERR, " err, wrong parameters !");
                break;
        }

        // ?? need this ??
    }

    if( result != JPG_ERR_OK )
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR,"%s() err 0x%x !", __FUNCTION__, result);
    }

	if (pJpgDev->initParam.codecType == JPG_CODEC_DEC_MJPG)
	{
		_jpg_mutex_unlock(JPG_MSG_TYPE_TRACE_ITEJPG, g_jpg_codec_mutex);
	}
    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ITEJPG);
    return result;
}

JPG_ERR
iteJpg_WaitIdle(
    HJPG            *pHJpeg,
    void            *extraData
    JPG_EXTRA_INFO)
{
    JPG_ERR     result = JPG_ERR_OK;
    JPG_DEV     *pJpgDev = (JPG_DEV*)pHJpeg;

    _jpg_trace_caller(" %s: caller=%s()[#%d]\n", __FUNCTION__, _caller, _line);
    _jpg_trace_enter(JPG_MSG_TYPE_TRACE_ITEJPG, "0x%x, 0x%x\n", pHJpeg, extraData);
    _jpg_verify_handle(JPG_MSG_TYPE_TRACE_ITEJPG, pHJpeg, 0, result);

    if( pJpgDev->status != JPG_STATUS_FAIL )
    {
        JCOMM_HANDLE        *pHJComm = pJpgDev->pHJComm;

        result = jComm_Control(pHJComm, (uint32_t)JCODEC_CMD_WAIT_IDLE, NULL, NULL);
        if( result != JPG_ERR_OK )
        {
            jpg_msg_ex(JPG_MSG_TYPE_ERR, " error, wait idle time out !! ");
        }

    }

    if( result != JPG_ERR_OK )
    {
        pJpgDev->status = JPG_STATUS_FAIL;
        jpg_msg_ex(JPG_MSG_TYPE_ERR,"%s() err 0x%x !", __FUNCTION__, result);
    }

    _jpg_trace_leave(JPG_MSG_TYPE_TRACE_ITEJPG);
    return result;
}

JPG_ERR
iteJpg_GetStatus(
    HJPG            *pHJpeg,
    JPG_USER_INFO   *pJpgUserInfo,
    void            *extraData)
{
    JPG_ERR     result = JPG_ERR_OK;
    JPG_DEV     *pJpgDev = (JPG_DEV*)pHJpeg;

    _jpg_verify_handle(JPG_MSG_TYPE_TRACE_ITEJPG, pHJpeg, 0, result);

    if( pJpgUserInfo != NULL )
    {
        JPG_DISP_INFO       *pJDispInfo = &pJpgDev->jDispInfo;
        JPG_FRM_COMP        *pJFrmComp = &pJpgDev->jPrsInfo.jFrmComp;
        //JPG_INIT_PARAM      *pJInitParam = &pJpgDev->initParam;

        pJpgUserInfo->status = pJpgDev->status;
        if( pJpgDev->status != JPG_STATUS_FAIL )
        {
            uint16_t             heightUnit;
            uint16_t             widthUnit;

            pJpgUserInfo->jpgRect.x = pJDispInfo->dstX;
            pJpgUserInfo->jpgRect.y = pJDispInfo->dstY;
            pJpgUserInfo->jpgRect.w = pJDispInfo->dstW;
            pJpgUserInfo->jpgRect.h = pJDispInfo->dstH;


            widthUnit  = (pJFrmComp->jFrmInfo[0].horizonSamp << 3);
            heightUnit = (pJFrmComp->jFrmInfo[0].verticalSamp << 3);

            pJpgUserInfo->real_width = (pJFrmComp->imgWidth + ((uint32_t)widthUnit - 1u)) & ~((uint32_t)widthUnit - 1u);
            pJpgUserInfo->real_height = (pJFrmComp->imgHeight + ((uint32_t)heightUnit - 1u)) & ~((uint32_t)heightUnit - 1u);

            pJpgUserInfo->slice_num   = ((pJpgUserInfo->real_height + 0x7u) >> 3);

            pJpgUserInfo->imgWidth  = pJFrmComp->imgWidth;
            pJpgUserInfo->imgHeight = pJFrmComp->imgHeight;

            pJpgUserInfo->comp1Pitch = (uint32_t)(pJpgUserInfo->real_width + 0x7u) & ~0x7u; 


            if( (pJFrmComp->jFrmInfo[0].horizonSamp == 1u) &&
                (pJFrmComp->jFrmInfo[0].verticalSamp == 1u) )
            { // JPG_COLOR_SPACE_YUV444
                pJpgUserInfo->comp23Pitch = (pJpgUserInfo->comp1Pitch >> 1);
                pJpgUserInfo->colorFormate = JPG_COLOR_SPACE_YUV422;
            }
            else if( (pJFrmComp->jFrmInfo[0].horizonSamp == 1u) &&
                     (pJFrmComp->jFrmInfo[0].verticalSamp == 2u) )
            { 
            	if( (pJFrmComp->jFrmInfo[1].horizonSamp == 1u) &&
                     (pJFrmComp->jFrmInfo[1].verticalSamp == 2u) )
            	{
            		pJpgUserInfo->comp23Pitch = pJpgUserInfo->comp1Pitch;
					pJpgUserInfo->colorFormate = JPG_COLOR_SPACE_YUV444;
                }
				else
				{
            		// JPG_COLOR_SPACE_YUV422R
                	pJpgUserInfo->comp23Pitch = pJpgUserInfo->comp1Pitch;
                	pJpgUserInfo->colorFormate = JPG_COLOR_SPACE_YUV422R;
				}
            }
            else
            {
                pJpgUserInfo->comp23Pitch = (pJpgUserInfo->comp1Pitch >> 1);
                if( (pJFrmComp->jFrmInfo[0].horizonSamp == 1u) &&
                    (pJFrmComp->jFrmInfo[0].verticalSamp == 2u) )
                {
                    pJpgUserInfo->colorFormate = JPG_COLOR_SPACE_YUV422R;
                }
                else if( (pJFrmComp->jFrmInfo[0].horizonSamp == 2u) &&
                         (pJFrmComp->jFrmInfo[0].verticalSamp == 2u) )
                {
					pJpgUserInfo->colorFormate = JPG_COLOR_SPACE_YUV422;
                }
                else if( (pJFrmComp->jFrmInfo[0].horizonSamp == 4u) &&
                         (pJFrmComp->jFrmInfo[0].verticalSamp == 1u) )
                {
                    pJpgUserInfo->colorFormate = JPG_COLOR_SPACE_YUV422;
                }
                else if( (pJFrmComp->jFrmInfo[0].horizonSamp == 2u) &&
                         (pJFrmComp->jFrmInfo[0].verticalSamp == 1u) )
                {
                    pJpgUserInfo->colorFormate = JPG_COLOR_SPACE_YUV422;
                }
				else
				{
					; /* do nothing */
				}
            }

			switch(pJDispInfo->outColorSpace)
			{
				case JPG_COLOR_SPACE_RGB565:
					pJpgUserInfo->colorFormate = JPG_COLOR_SPACE_RGB565;
					pJpgUserInfo->comp23Pitch = 0;
					break;
					
				case JPG_COLOR_SPACE_ARGB8888:
					pJpgUserInfo->colorFormate = JPG_COLOR_SPACE_ARGB8888;
					pJpgUserInfo->comp23Pitch = pJpgUserInfo->comp1Pitch; // for AlphaPlane pitch.
					break;

				case JPG_COLOR_SPACE_ARGB4444:
					pJpgUserInfo->colorFormate = JPG_COLOR_SPACE_ARGB4444;
					pJpgUserInfo->comp23Pitch = pJpgUserInfo->comp1Pitch; // for AlphaPlane pitch.
					break;
					
				default:
					; /*Do nothing*/
            		break;
			}
			/*
			//printf("ite_jpg.c pJpgUserInfo->colorFormate = %d\n",pJpgUserInfo->colorFormate);
			*/
        }
        else
        {
            if( (&pJpgUserInfo->jpgRect) != NULL )
            {
                (void)memset(&pJpgUserInfo->jpgRect, 0x0, sizeof(JPG_RECT));
            }
        }
    }

    if( result != JPG_ERR_OK )
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR,"%s() err 0x%x !", __FUNCTION__, result);
    }

    return result;
}




