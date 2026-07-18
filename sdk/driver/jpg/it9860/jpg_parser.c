#include <stdio.h>
#include <stdlib.h>
#include "jpg_parser.h"
#include "jpg_defs.h"
#include "jpg_reg.h"
#include "jpg_common.h"

// =============================================================================
//                  Constant Definition
// =============================================================================

// =============================================================================
//                  Macro Definition
// =============================================================================
#define GetHighByte(bLEndian, pRPtr)        (((uint32_t)*(pRPtr) & 0xF0u) >> 4)
#define GetLowByte(bLEndian, pRPtr)         ((uint32_t)*(pRPtr) & 0x0Fu)
#define GetByte(bLEndian, pRPtr)            (*(pRPtr))
#define GetWord(bLEndian, pRPtr)            ( ((bLEndian) == true) ? (((uint32_t)*((pRPtr) + 1) << 8) | (uint32_t)*(pRPtr)) : (((uint32_t)*(pRPtr) << 8) | (uint32_t)*((pRPtr) + 1)) )
#define GetDWord(bLEndian, pRPtr)           ( ((bLEndian) == true) ? (((uint32_t)*((pRPtr) + 3) << 24) | ((uint32_t)*((pRPtr) + 2) << 16) | ((uint32_t)*((pRPtr) + 1) << 8) | *(pRPtr)) : (((uint32_t)*(pRPtr) << 24) | ((uint32_t)*((pRPtr) + 1) << 16) | ((uint32_t)*((pRPtr) + 2) << 8) | (uint32_t)*((pRPtr) + 3)) )
#define GetValue(bLEndian, pRPtr, Count)    ( ((Count) == 2) ? GetWord(bLEndian, pRPtr) : GetDWord(bLEndian, pRPtr))

#define APP2_CONTENTS_LIST                  0x01u
#define APP2_STREAM_DATA                    0x02u

#define APP2_OFFSET_TYPE                    (0x0Au - 2u)
#define APP2_OFFSET_LIST_INDEX              (0x0Bu - 2u)
#define APP2_OFFSET_STREAM_DATA             (0x11u - 2u)

// =============================================================================
//                  Structure Definition
// =============================================================================

// =============================================================================
//                  Global Data Definition
// =============================================================================

// =============================================================================
//                  Private Function Definition
// =============================================================================
#if 0

static bool
_isLEndian (
    void)
{
    const uint32_t  v   = 0x12345678u;
    const uint8_t   * p = (const uint8_t *)&v;
    return (*p == (uint8_t)(0x12)) ? false : true;
}
#endif

static uint32_t
_GetCurFactorPos (
    JPG_STREAM_HANDLE   * pHJStream,
    uint32_t            realSize,
    uint32_t            offset)
{
    uint32_t        StreamPos       = 0;
    JPG_STREAM_DESC * pJStreamDesc  = &pHJStream->jStreamDesc;

    /**
     *                    |---------- realSize ---------|
     *              prevStreamPos    offset       curStreamPos
     *     ---------------|-------------i---------------|----------|
     **/
    if (pJStreamDesc->jTell_stream != NULL)
    {
        (void)pJStreamDesc->jTell_stream(pHJStream, &StreamPos, NULL);
    }

    return (StreamPos + offset - realSize);
}

static uint8_t *
_CompleteSection (
    JPG_STREAM_HANDLE   * pHJStream,
    uint8_t             * ptCurr,
    uint8_t             * ptEnd,
    JPG_PARSER_INFO     * pJPrsInfo)
{
    uint16_t        sectionLength   = 0;
#if 0	
    JPG_STREAM_DESC * pJStreamDesc  = &pHJStream->jStreamDesc;
    JPG_PRS_BS_CTRL * pJPrsBsCtrl   = &pJPrsInfo->jPrsBsCtrl;
#endif
    sectionLength = (uint16_t)GetWord(pJPrsInfo->bLEndian, ptCurr);

    if ((ptCurr + sectionLength) >= ptEnd)
    {
#if 1
        return NULL;
#else
        int keepLength  = 0;
        int readLength  = 0;

        keepLength  = (ptEnd - ptCurr);
        readLength  = sectionLength - keepLength;
        memcpy(pJPrsBsCtrl->bsPrsBuf, ptCurr, keepLength);

        if (pJStreamDesc->jFull_buf)
        {
            pJStreamDesc->jFull_buf(pHJStream,
                                    pJPrsBsCtrl->bsPrsBuf + keepLength,
                                    readLength, &pJPrsBsCtrl->realSize, 0);
        }
        // reset parameters
        ptCurr                  = pJPrsBsCtrl->bsPrsBuf;
        pJPrsInfo->remainSize   -= pJPrsBsCtrl->realSize;
#endif
    }

    return ptCurr;
}

static uint8_t *
_ParseSOF00 (
    JPG_STREAM_HANDLE   * pHJStream,
    uint8_t             * ptCurr,
    JPG_PARSER_INFO     * pJPrsInfo)
{
    /**
     * SOF        16        0xffc0        Start Of Frame
     * Lf        16        3Nf+8        Frame header length
     * P        8        8            Sample precision
     * Y        16        0-65535        Number of lines
     * X        16        1-65535        Samples per line
     * Nf        8        1-255        Number of image components (e.g. Y, U and V).
     *
     * ---------Repeats for the number of components (e.g. Nf)-----------------
     * Ci        8        0-255        Component identifier
     * Hi        4        1-4            Horizontal Sampling Factor
     * Vi        4        1-4            Vertical Sampling Factor
     * Tqi        8        0-3            Quantization Table Selector.
     */
     
    uint16_t        compCnt     = 0;
    JPG_FRM_COMP    * jFrmComp  = &pJPrsInfo->jFrmComp;
    uint16_t        i;

#if 0
    ptCurr += 2;
    // skip the sample precision field of scan segment
    ptCurr++;
#else
    ptCurr              += 3;
#endif

    jFrmComp->imgHeight = (uint16_t)GetWord(pJPrsInfo->bLEndian, ptCurr);
    ptCurr              += 2;
    jFrmComp->imgWidth  = (uint16_t)GetWord(pJPrsInfo->bLEndian, ptCurr);
    ptCurr              += 2;
    compCnt             = GetByte(pJPrsInfo->bLEndian, ptCurr);
    ptCurr++;
    if (compCnt > JPG_MAX_COMPONENT_NUM)
    {
        return NULL;
    }
    jFrmComp->bSingleChannel = (compCnt == 1u) ? true : false;

    for (i = 0; i < compCnt; i++)
    {
        jFrmComp->jFrmInfo[i].compId        = GetByte(pJPrsInfo->bLEndian, ptCurr);
        ptCurr++;
        jFrmComp->jFrmInfo[i].horizonSamp   = (uint16_t)GetHighByte(pJPrsInfo->bLEndian, ptCurr);
        jFrmComp->jFrmInfo[i].verticalSamp  = (uint16_t)GetLowByte(pJPrsInfo->bLEndian, ptCurr);
        ptCurr++;
        jFrmComp->jFrmInfo[i].qTableSel     = GetByte(pJPrsInfo->bLEndian, ptCurr);
        ptCurr++;
    }

    return ptCurr;
}

static uint8_t *
_ParseSOS (
    JPG_STREAM_HANDLE   * pHJStream,
    uint8_t             * ptCurr,
    JPG_PARSER_INFO     * pJPrsInfo)
{
    /**
     * SOS        16        0xffda            Start Of Scan
     * Ls        16        2Ns + 6            Scan header length
     * Ns        8        1-4                Number of image components
     * Csj        8        0-255            Scan Component Selector
     * Tdj        4        0-1                DC Coding Table Selector
     * Taj        4        0-1                AC Coding Table Selector
     * Ss        8        0                Start of spectral selection
     * Se        8        63                End of spectral selection
     * Ah        4        0                Successive Approximation Bit High
     * Ai        4        0                Successive Approximation Bit Low
     */
    uint16_t        compCnt = 0;
    uint16_t        compIdx = 0;
    JPG_FRM_COMP    * jFrmComp = &pJPrsInfo->jFrmComp;
    JPG_FRM_INFO    * jFrmInfo = pJPrsInfo->jFrmComp.jFrmInfo;
    uint16_t        i, j;

    ptCurr  += 2;

    compCnt = GetByte(pJPrsInfo->bLEndian, ptCurr);
    ptCurr++;
    if (compCnt > JPG_MAX_COMPONENT_NUM)
    {
        return NULL;
    }
    jFrmComp->validComp = 0;
    for (i = 0; i < compCnt; i++)
    {
        compIdx = GetByte(pJPrsInfo->bLEndian, ptCurr);
        ptCurr++;

        for (j = 0; j < compCnt; j++)
        {
            if (jFrmInfo[j].compId == compIdx)
            {
                jFrmInfo[j].dcHuffTableSel  = (uint16_t)GetHighByte(pJPrsInfo->bLEndian, ptCurr);
                jFrmInfo[j].acHuffTableSel  = (uint16_t)GetLowByte(pJPrsInfo->bLEndian, ptCurr);
                ptCurr++;

                switch (j)
                {
                    case 0: jFrmComp->validComp |= (uint16_t)JPG_MSK_DEC_COMPONENT_A_VALID;
                        break;

                    case 1: jFrmComp->validComp |= (uint16_t)JPG_MSK_DEC_COMPONENT_B_VALID;
                        break;

                    case 2: jFrmComp->validComp |= (uint16_t)JPG_MSK_DEC_COMPONENT_C_VALID;
                        break;

                    case 3: jFrmComp->validComp |= (uint16_t)JPG_MSK_DEC_COMPONENT_D_VALID;
                        break;
                }
            }
        }
    }

    // skip : Ss, Se, Ah, Al
    ptCurr                      += 3;

    jFrmComp->bNonInterleaved   = (jFrmComp->bSingleChannel && (compCnt == 1u)) ? true : false;
    jFrmComp->compNum           = compCnt;

    return ptCurr;
}

static uint8_t *
_ParseDHT (
    JPG_STREAM_HANDLE   * pHJStream,
    uint8_t             * ptCurr,
    JPG_PARSER_INFO     * pJPrsInfo)
{
    /**
     * u16 0xffc4
     * u16 be length of segment
     * 4-bits class (0 is DC, 1 is AC, more on this later)
     * 4-bits table id
     * array of 16 u8 number of elements for each of 16 depths
     * array of u8 elements, in order of depth
     */
    uint16_t        length          = 0;
    uint8_t         tabClass        = 0;                            // 0:DC, 1:AC
    uint8_t         desId           = 0;
    uint8_t         index           = 0;
    uint8_t         * ptSegEnd      = NULL;
    JPG_STREAM_DESC * pJStreamDesc  = &pHJStream->jStreamDesc;
    JPG_H_TABLE     * huff_DC       = pJPrsInfo->jFrmComp.huff_DC;  // 0->Y, 1->UV
    JPG_H_TABLE     * huff_AC       = pJPrsInfo->jFrmComp.huff_AC;  // 0->Y, 1->UV
    int             i;

    length      = (uint16_t)GetWord(pJPrsInfo->bLEndian, ptCurr);
    ptSegEnd    = ptCurr + length;

    ptCurr      += 2;

    while (ptCurr < ptSegEnd)
    {
        tabClass    = (uint8_t)GetHighByte(pJPrsInfo->bLEndian, ptCurr);
        desId       = (uint8_t)GetLowByte(pJPrsInfo->bLEndian, ptCurr);

        ptCurr++;

        index = ((desId == 0u) ? desId : 1u);
        switch (tabClass)
        {
            case 0:   // DC Huffman table
                huff_DC[index].totalCodeLenCnt = 0;
                for (i = 0; i < 16; i++)
                {
                    huff_DC[index].totalCodeLenCnt += (uint16_t)*(ptCurr + i);
                }
                if ( (huff_DC[index].totalCodeLenCnt + 16u) > length)
                {
                    return NULL;
                }

                if (pJStreamDesc->jHeap_mem != NULL)
                {
                    uint32_t realSize = 0;
                    huff_DC[index].pHuffmanTable = pJStreamDesc->jHeap_mem(pHJStream, JPG_HEAP_DEF,
                                                                           (huff_DC[index].totalCodeLenCnt + 16u), &realSize);
                    if ( (huff_DC[index].pHuffmanTable != NULL) &&
                        (realSize == ((uint32_t)huff_DC[index].totalCodeLenCnt + 16u)))
                    {
                        (void)memcpy(huff_DC[index].pHuffmanTable, ptCurr, ((uint32_t)huff_DC[index].totalCodeLenCnt + 16u));
                    }
                    else if (huff_DC[index].pHuffmanTable != NULL)
                    {
                        if (pJStreamDesc->jFree_mem != NULL)
                        {
                            pJStreamDesc->jFree_mem(pHJStream, JPG_HEAP_DEF, huff_DC[index].pHuffmanTable);
                        }
                        huff_DC[index].pHuffmanTable = NULL;
                    }
					else
					{
						; /*Do nothing*/
                    }
                }

                ptCurr  += 16;
                ptCurr  += huff_DC[index].totalCodeLenCnt;
                break;

            case 1:   // AC Huffman table
                huff_AC[index].totalCodeLenCnt = 0;
                for (i = 0; i < 16; i++)
                {
                    huff_AC[index].totalCodeLenCnt += *(ptCurr + i);
                }
                if ( (huff_AC[index].totalCodeLenCnt + 16u) > length)
                {
                    return NULL;
                }

                if (pJStreamDesc->jHeap_mem != NULL)
                {
                    uint32_t realSize = 0;
                    huff_AC[index].pHuffmanTable = pJStreamDesc->jHeap_mem(pHJStream, JPG_HEAP_DEF,
                                                                           (huff_AC[index].totalCodeLenCnt + 16u), &realSize);
                    if ( (huff_AC[index].pHuffmanTable != NULL) &&
                        (realSize == ((uint32_t)huff_AC[index].totalCodeLenCnt + 16u)))
                    {
                        (void)memcpy(huff_AC[index].pHuffmanTable, ptCurr, ((uint32_t)huff_AC[index].totalCodeLenCnt + 16u));
                    }
                    else if (huff_AC[index].pHuffmanTable != NULL)
                    {
                        if (pJStreamDesc->jFree_mem != NULL)
                        {
                            pJStreamDesc->jFree_mem(pHJStream, JPG_HEAP_DEF, huff_AC[index].pHuffmanTable);
                        }
                        huff_AC[index].pHuffmanTable = NULL;
                    }
					else
					{
						; /*Do nothing*/
                    }
                }

                ptCurr  += 16;
                ptCurr  += huff_AC[index].totalCodeLenCnt;
                break;
			default:
				; /*Do nothing*/
				break;
        }
    }

    return ptCurr;
}

static uint8_t *
_ParseDQT (
    JPG_STREAM_HANDLE   * pHJStream,
    uint8_t             * ptCurr,
    JPG_PARSER_INFO     * pJPrsInfo)
{
    /**
     * DQT        16        0xffdb            quantization table(s)
     * Lq        16                        quantization table length
     * Pq       4                       value "1" indicates 16-bit Qk values, value "0" indicates 8-bit Qk values
     * Tq       4
     * Qk       array of 16/u8 number of elements
     */

    uint16_t    i;
    uint16_t    length      = 0;
    uint16_t    qTabCnt     = 0;
    uint16_t    qTabIdx     = 0;
    JPG_Q_TABLE * qTable    = &pJPrsInfo->jFrmComp.qTable;

    length  = (uint16_t)GetWord(pJPrsInfo->bLEndian, ptCurr);
    ptCurr  += 2;

    qTabCnt = (length - 2u) / (JPG_Q_TABLE_ELEMENT_NUM + 1u);

    if (qTabCnt > 4u)
    {
        return NULL;
    }

    for (i = 0; i < qTabCnt; i++)
    {
        qTabIdx = (uint16_t)GetLowByte(pJPrsInfo->bLEndian, ptCurr);

        // skip the precision & destination field identifier of Q table
        ptCurr++;
        (void)memcpy(qTable->table[qTabIdx], ptCurr, JPG_Q_TABLE_ELEMENT_NUM);
        ptCurr += JPG_Q_TABLE_ELEMENT_NUM;
    }

    qTable->tableCnt += qTabCnt;
    return ptCurr;
}

static uint8_t *
_ParseDRI (
    JPG_STREAM_HANDLE   * pHJStream,
    uint8_t             * ptCurr,
    JPG_PARSER_INFO     * pJPrsInfo)
{
    uint16_t        restartInterval = 0;
    JPG_FRM_COMP    * jFrmComp      = &pJPrsInfo->jFrmComp;

    ptCurr                      += 2;
    restartInterval             = (uint16_t)GetWord(pJPrsInfo->bLEndian, ptCurr);
    ptCurr                      += 2;

    jFrmComp->restartInterval   = restartInterval;

    return ptCurr;
}

static void
_SetDefHTable (
    JPG_STREAM_HANDLE   * pHJStream,
    JPG_PARSER_INFO     * pJPrsInfo)
{
    JPG_H_TABLE * huff_DC   = pJPrsInfo->jFrmComp.huff_DC;      // 0->Y, 1->UV
    JPG_H_TABLE * huff_AC   = pJPrsInfo->jFrmComp.huff_AC;      // 0->Y, 1->UV
    uint16_t    dstIdx      = 0;

    for (dstIdx = 0; dstIdx < 2u; dstIdx++)
    {
        huff_DC[dstIdx].totalCodeLenCnt = 12;
        huff_DC[dstIdx].pHuffmanTable   = (uint8_t *)(&Def_DCHuffTable[dstIdx][0]);

        huff_AC[dstIdx].totalCodeLenCnt = 162;
        huff_AC[dstIdx].pHuffmanTable   = (uint8_t *)(&Def_ACHuffTable[dstIdx][0]);
    }
}

static JPG_ERR
_AppSectionOverview (
    JPG_STREAM_HANDLE   * pHJStream,
    JPG_PARSER_INFO     * pJPrsInfo)
{
    JPG_ERR         result          = JPG_ERR_OK;
    JPG_STREAM_DESC * pJStreamDesc  = &pHJStream->jStreamDesc;
    JPG_PRS_BS_CTRL * pJPrsBsCtrl   = &pJPrsInfo->jPrsBsCtrl;
    JPG_ATTRIB      * pJAttrib      = &pJPrsInfo->jAttrib;
    uint8_t         * ptCur;
    uint8_t         * ptEnd;
    bool            bExistApp01     = false;
    bool            bSupportedApp   = false;

    ptCur = (uint8_t *)(pJPrsBsCtrl->bsPrsBuf + pJPrsBsCtrl->curPos);

    // check jpg file
    if (pJPrsBsCtrl->b1stSection == true)
    {
        pJPrsBsCtrl->b1stSection = false;

        if ((*ptCur == JPG_MARKER_START) &&
            (*(ptCur + 1) == JPG_START_OF_IMAGE_MARKER))
        {
            ptCur += 2;
        }
        else
        {
            pJAttrib->flag  |= ( (uint32_t)JATT_STATUS_UNSUPPORT_PRIMARY |
                (uint32_t)JATT_STATUS_UNSUPPORT_SMALL_THUMB |
                (uint32_t)JATT_STATUS_UNSUPPORT_LARGE_THUMB );
            result          = JPG_ERR_NOT_JPEG;
            goto end;
        }
    }

    ptEnd = pJPrsBsCtrl->bsPrsBuf + pJPrsBsCtrl->realSize;

    // parsing process
    while (!((*ptCur == JPG_MARKER_START) && ((*(ptCur + 1) & 0xE0u) == 0xC0u)))
    {
        if (*ptCur == JPG_MARKER_START)
        {
            bSupportedApp = false;
            switch (*(ptCur + 1))
            {
                case JPG_APP00_MARKER: // E0
                    if (bExistApp01 == true)
                    {
                        break;
                    }

                    ptCur                       += 2;
                    bSupportedApp               = true;
                    pJAttrib->exifAppOffset[0]  = _GetCurFactorPos(pHJStream, pJPrsBsCtrl->realSize, ((uint32_t)ptCur - (uint32_t)pJPrsBsCtrl->bsPrsBuf));
                    pJAttrib->exifAppLength[0]  = (uint32_t)GetWord(pJPrsInfo->bLEndian, ptCur);
                    pJAttrib->exifAppCnt        = 1;
                    break;

                case JPG_APP01_MARKER: // E1
                    if (memcmp((ptCur + 4), "Exif", 4) == 0)
                    {
                        // Exif
                        ptCur                       += 2;
                        bSupportedApp               = true;
                        pJAttrib->exifAppOffset[0]  = _GetCurFactorPos(pHJStream, pJPrsBsCtrl->realSize, ((uint32_t)ptCur - (uint32_t)pJPrsBsCtrl->bsPrsBuf));
                        pJAttrib->exifAppLength[0]  = (uint32_t)GetWord(pJPrsInfo->bLEndian, ptCur);
                        pJAttrib->exifAppCnt        = 1;
                        bExistApp01                 = true;
                        pJAttrib->bApp1Find         = true;
                    }
                    break;

                case JPG_APP02_MARKER: // E2
                    ptCur           += 2;
                    bSupportedApp   = true;

                    // skip ICC_PROFILE
                    if (memcmp((ptCur + 2), "ICC_PROFILE", 11) != 0)
                    {
                        pJAttrib->exifAppOffset[pJAttrib->exifAppCnt]   = _GetCurFactorPos(pHJStream, pJPrsBsCtrl->realSize, ((uint32_t)ptCur - (uint32_t)pJPrsBsCtrl->bsPrsBuf));
                        pJAttrib->exifAppLength[pJAttrib->exifAppCnt]   = (uint32_t)GetWord(pJPrsInfo->bLEndian, ptCur);
                        pJAttrib->exifAppCnt++;
                    }
                    break;

                case JPG_APP13_MARKER: // ED
                    if (bExistApp01 == true)
                    {
                        break;
                    }

                    if (memcmp((ptCur + 4), "PHOTOSHOP", 9) == 0)
                    {
                        // PHOTOSHOP
                        break;
                    }

                    ptCur                       += 2;
                    bSupportedApp               = true;
                    pJAttrib->exifAppOffset[0]  = _GetCurFactorPos(pHJStream, pJPrsBsCtrl->realSize, ((uint32_t)ptCur - (uint32_t)pJPrsBsCtrl->bsPrsBuf));
                    pJAttrib->exifAppLength[0]  = (uint32_t)GetWord(pJPrsInfo->bLEndian, ptCur);
                    pJAttrib->exifAppCnt        = 1;
                    break;

                case JPG_APP14_MARKER: // EE
                    if ((memcmp((ptCur + 4), "Adobe", 5) == 0) && (*(ptCur + 15) == 0x02u))
                    {
                        // AppE for Adobe YCCK
                        pJAttrib->bCMYK = true;
                        ptCur           += 2;
                        bSupportedApp   = true;
                    }
                    break;
				default:
					; /*Do nothing*/
					break;
            }

            // check remain bs_buf size
            if (((bSupportedApp == true) && ((*(ptCur - 1) & 0xF0u) == 0xE0u)) ||
                ((bSupportedApp == false) && ((*(ptCur + 1) & 0xF0u) == 0xE0u)) ||
                ((bSupportedApp == false) && ((*(ptCur + 1) & 0xF0u) == 0xF0u)))
            {
                uint32_t sectionLength = 0;

                if (bSupportedApp == false)
                {
                    ptCur += 2;
                }

                sectionLength = (uint32_t)GetWord(pJPrsInfo->bLEndian, ptCur);

                if (((uint32_t)ptCur + sectionLength) < (uint32_t)ptEnd)
                {
                    ptCur += sectionLength;
                }
                else
                {
                    uint32_t skipLength = 0;

                    skipLength = sectionLength - ((uint32_t)ptEnd - (uint32_t)ptCur);

                    if (pJStreamDesc->jSeek_stream != NULL)
                    {
                        (void)pJStreamDesc->jSeek_stream(pHJStream, skipLength, JPG_SEEK_CUR, NULL);
                    }

                    pJPrsInfo->remainSize -= skipLength;

                    if (pJStreamDesc->jFull_buf != NULL)
                    {
                        (void)pJStreamDesc->jFull_buf(pHJStream, pJPrsBsCtrl->bsPrsBuf,
                                                pJPrsBsCtrl->bsPrsBufMaxLeng,
                                                &pJPrsBsCtrl->realSize, NULL);
                    }
                    // reset parameters
                    ptCur                   = pJPrsBsCtrl->bsPrsBuf;
                    pJPrsInfo->remainSize   -= pJPrsBsCtrl->realSize;
                }

                continue;
            }
        }

        // check remain bs_buf size
        if ((ptCur + 4) < ptEnd)   // keep last 4 bytes for avoiding breaking Marker
        {
            ptCur++;
        }
        else
        {
            (void)memmove(pJPrsBsCtrl->bsPrsBuf, ptCur, 4);
            if (pJStreamDesc->jFull_buf != NULL)
            {
                (void)pJStreamDesc->jFull_buf(pHJStream, pJPrsBsCtrl->bsPrsBuf + 4,
                                        pJPrsBsCtrl->bsPrsBufMaxLeng - 4u,
                                        &pJPrsBsCtrl->realSize, NULL);
            }
            // reset parameters
            ptCur                   = pJPrsBsCtrl->bsPrsBuf;
            pJPrsInfo->remainSize   -= pJPrsBsCtrl->realSize;
        }
    }

    pJAttrib->primaryOffset = _GetCurFactorPos(pHJStream, pJPrsBsCtrl->realSize, ((uint32_t)ptCur - (uint32_t)pJPrsBsCtrl->bsPrsBuf));
    pJAttrib->primaryLength = pJPrsInfo->fileLength - pJAttrib->primaryOffset;
    pJPrsBsCtrl->curPos     = (uint32_t)ptCur - (uint32_t)pJPrsBsCtrl->bsPrsBuf;

end:
    if (result != JPG_ERR_OK)
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() Err 0x%x ", __FUNCTION__, result);
    }

    return result;
}

static JPG_ERR
_Parse_App01 (
    JPG_STREAM_HANDLE   * pHJStream,
    JPG_PARSER_INFO     * pJPrsInfo)
{
    JPG_ERR         result = JPG_ERR_OK;
    JPG_PRS_BS_CTRL * pJPrsBsCtrl = &pJPrsInfo->jPrsBsCtrl;
    JPG_ATTRIB      * pJAttrib = &pJPrsInfo->jAttrib;
    JPG_EXIF_INFO   * pJExifInfo = &pJAttrib->exifInfo;
    bool            bFindSmallJpeg = false;
    uint8_t         * ptCur = NULL, * ptBasePos = NULL;
    uint8_t         * ptEnd = NULL;

    (void)memset((void *)pJExifInfo, 0, sizeof(JPG_EXIF_INFO));

    if (pJAttrib->bApp1Find)
    {
        uint32_t    byteOrder, IFDOffset;
        bool        bLEndian, bFindSOI;
        uint32_t    IFDEntryCnt = 0;
        uint32_t    i, Tag;
        uint32_t    exifIFDoffset = 0;

        jPrs_Seek2SectionStart(pHJStream, pJPrsInfo, pJAttrib->exifAppOffset[0], pJAttrib->exifAppLength[0]);

        ptCur = pJPrsBsCtrl->bsPrsBuf + pJPrsBsCtrl->curPos;
		ptBasePos   = ptCur;
        ptEnd       = ptCur + pJAttrib->exifAppLength[0];

        ptCur       += 8;
        byteOrder   = ( ((uint32_t)*(ptCur) << 8) | (uint32_t)*(ptCur + 1) );
        bLEndian    = (byteOrder == 0x4949u) ? true : false;

        ptCur       += 4;
        IFDOffset   = (uint32_t)GetValue(bLEndian, ptCur, 4);

        // 0-th IFD
        ptCur       = ptBasePos + 8u + IFDOffset;
        IFDEntryCnt = (uint32_t)GetValue(bLEndian, ptCur, 2);
        ptCur       += 2;

        for (i = 0; i < IFDEntryCnt; i++)
        {
            if ((uint32_t)ptCur > (uint32_t)ptEnd)
            {
                goto end;
            }

            Tag = (uint32_t)GetValue(bLEndian, ptCur, 2);

            switch (Tag)
            {
                case 0x0112u:    // Orientation Tag
                    pJExifInfo->primaryOrientation = (uint32_t)GetValue(bLEndian, ptCur + 8, 2);
					#if 0
                    jpg_msg(1, "Primary Orientation => Tag 0x%x Orientation %d\n", Tag, pJExifInfo->primaryOrientation);
					#endif
                    break;

                case 0x0100u:    // image Width Tag
                    pJExifInfo->imageWidth = (uint32_t)GetValue(bLEndian, ptCur + 8, 4);
					#if 0
                    jpg_msg(1, " Tag 0x%x imageWidth %d\n", Tag, pJExifInfo->imageWidth);
					#endif
                    break;

                case 0x0101u:    // image Height Tag
                    pJExifInfo->imageHeight = (uint32_t)GetValue(bLEndian, ptCur + 8, 4);
					#if 0
                    jpg_msg(1, " Tag 0x%x imageHeight %d\n", Tag, pJExifInfo->imageHeight);
					#endif
                    break;

                case 0x8769u:    // EXIF IFD Pointer
                    exifIFDoffset = (uint32_t)GetValue(bLEndian, ptCur + 8, 4);
                    break;

                case 0x010Fu:    // Camera Make Tag
                {
                    uint32_t    dateOffset  = (uint32_t)GetValue(bLEndian, ptCur + 8, 4);
                    uint32_t    count       = (uint32_t)GetValue(bLEndian, ptCur + 4, 4);
                    uint8_t     * value     = ptBasePos + 8 + dateOffset;

                    if (count > 4u)
                    {
                        count                       = (count < 32u) ? count : 31u;
                        (void)memcpy((void *)pJExifInfo->cameraMake, (void *)value, count);
                        pJExifInfo->cameraMake[31]  = '\0';
                    }
                    else
                    {
                        if (bLEndian == true)
                        {
                            dateOffset = ((dateOffset & 0xffu) << 24) |
                                         ((dateOffset & 0xff00u) << 8) |
                                         ((dateOffset & 0xff0000u) >> 8) |
                                         ((dateOffset & 0xff000000u) >> 24);
                        }
                        (void)memcpy((void *)pJExifInfo->cameraMake, (void *)&dateOffset, count);
                    }
                }
                break;

                case 0x0110u: // Camera Model Tag
                {
                    uint32_t    dateOffset  = (uint32_t)GetValue(bLEndian, ptCur + 8, 4);
                    uint32_t    count       = (uint32_t)GetValue(bLEndian, ptCur + 4, 4);
                    uint8_t     * value     = ptBasePos + 8 + dateOffset;

                    if (count > 4u)
                    {
                        count                       = (count < 32u) ? count : 31u;
                        (void)memcpy((void *)pJExifInfo->cameraModel, (void *)value, count);
                        pJExifInfo->cameraModel[31] = '\0';
                    }
                    else
                    {
                        if (bLEndian == true)
                        {
                            dateOffset = ((dateOffset & 0xffu) << 24) |
                                         ((dateOffset & 0xff00u) << 8) |
                                         ((dateOffset & 0xff0000u) >> 8) |
                                         ((dateOffset & 0xff000000u) >> 24);
                        }
                        (void)memcpy((void *)pJExifInfo->cameraModel, (void *)&dateOffset, count);
                    }
                }
                break;

                default:
					; /*Do nothing*/
                    break;
            }

            ptCur += 12;
        }

        IFDOffset = (uint32_t)GetValue(bLEndian, ptCur, 4);

        // exif IFD
        if (exifIFDoffset != 0u)
        {
            ptCur       = ptBasePos + 8u + exifIFDoffset;
            IFDEntryCnt = (uint32_t)GetValue(bLEndian, ptCur, 2);

            ptCur       += 2;
            for (i = 0; i < IFDEntryCnt; i++)
            {
                if ((uint32_t)ptCur > (uint32_t)ptEnd)
                {
                    goto end;
                }

                Tag = (uint32_t)GetValue(bLEndian, ptCur, 2);

                switch (Tag)
                {
                    case 0x9003u: // DateTimeOriginal
                    {
                        uint32_t    dateOffset  = (uint32_t)GetValue(bLEndian, ptCur + 8, 4);
                        uint32_t    count       = (uint32_t)GetValue(bLEndian, ptCur + 4, 4);
                        uint8_t     * dateValue = ptBasePos + 8u + dateOffset;

                        count                       = (count < 20u) ? count : 19u;
                        (void)memcpy((void *)pJExifInfo->dateTime, (void *)dateValue, count);
                        pJExifInfo->dateTime[19]    = '\0';
                    }
                    break;

                    case 0xA002u:    // image Width Tag
                        pJExifInfo->imageWidth = (uint32_t)GetValue(bLEndian, ptCur + 8, 4);
						#if 0
                        jpg_msg(1, " Tag 0x%x imageWidth %d\n", Tag, pJExifInfo->imageWidth);
						#endif
                        break;

                    case 0xA003u:    // image Height Tag
                        pJExifInfo->imageHeight = (uint32_t)GetValue(bLEndian, ptCur + 8, 4);
						#if 0
                        jpg_msg(1, " Tag 0x%x imageHeight %d\n", Tag, pJExifInfo->imageHeight);
						#endif
                        break;

                    default:
						; /*Do nothing*/
                        break;
                }
                ptCur += 12;
            }
        }

        // 1-th IFD ( optional )
        ptCur       = ptBasePos + 8u + IFDOffset;
        IFDEntryCnt = (uint32_t)GetValue(bLEndian, ptCur, 2);
        ptCur       += 2;

        for (i = 0 ; i < IFDEntryCnt; i++)
        {
            if ((uint32_t)ptCur > (uint32_t)ptEnd)
            {
                goto end;
            }

            Tag = (uint32_t)GetValue(bLEndian, ptCur, 2);

            switch (Tag)
            {
                case 0x0112u: // Orientation Tag
                    pJExifInfo->thumbOrientation = (uint32_t)GetValue(bLEndian, ptCur + 8, 2);
					#if 0
                    jpg_msg(1, "thumbnail Orientation => Tag 0x%x Orientation %d\n", Tag, pJExifInfo->thumbOrientation);
					#endif
                    break;

                default:
					; /*Do nothing*/
                    break;
            }
            ptCur += 12;
        }

        // Find thumbnail's offset and length
        bFindSOI = false;
        while ((uint32_t)ptCur < (uint32_t)ptEnd)
        {
            if (*(ptCur) != JPG_MARKER_START)
            {
                ptCur++;
                continue;
            }

            switch (*(ptCur + 1))
            {
                case JPG_START_OF_IMAGE_MARKER:
                    if (bFindSOI == false)
                    {
                        pJAttrib->jBaseLiteInfo[JPG_DEC_SMALL_THUMB].offset = ((uint32_t)ptCur - (uint32_t)ptBasePos) + pJAttrib->exifAppOffset[0];
                        bFindSOI                                            = true;
                        ptCur                                               += 2;
                    }
                    break;

                case JPG_END_OF_IMAGE_MARKER:
                    if (bFindSOI == true)
                    {
                        pJAttrib->jBaseLiteInfo[JPG_DEC_SMALL_THUMB].size   = ((uint32_t)ptCur - (uint32_t)ptBasePos) + pJAttrib->exifAppOffset[0] - pJAttrib->jBaseLiteInfo[JPG_DEC_SMALL_THUMB].offset + 2u;
                        bFindSmallJpeg                                      = true;
                        goto end;
                    }
                    break;

                case JPG_PROGRESSIVE:
                    pJAttrib->jBaseLiteInfo[JPG_DEC_SMALL_THUMB].bJprog = true;
					pJAttrib->jBaseLiteInfo[JPG_DEC_SMALL_THUMB].width = (((uint32_t)*(ptCur + 7) << 8) | (uint32_t)*(ptCur + 8));
                    pJAttrib->jBaseLiteInfo[JPG_DEC_SMALL_THUMB].height = (((uint32_t)*(ptCur + 5) << 8) | (uint32_t)*(ptCur + 6));
                    ptCur                                               += ((((uint32_t)ptCur[2] << 8) | (uint32_t)ptCur[3]) + 2u);
                    break;

                case JPG_BASELINE_MARKER:
                    pJAttrib->jBaseLiteInfo[JPG_DEC_SMALL_THUMB].width = (((uint32_t)*(ptCur + 7) << 8) | (uint32_t)*(ptCur + 8));
                    pJAttrib->jBaseLiteInfo[JPG_DEC_SMALL_THUMB].height = (((uint32_t)*(ptCur + 5) << 8) | (uint32_t)*(ptCur + 6));
                    ptCur                                               += ((((uint32_t)ptCur[2] << 8) | (uint32_t)ptCur[3]) + 2u);
                    break;

                default:
                    if (((*(ptCur + 1) & 0xC0u) == 0xC0u) && (*(ptCur + 1) != 0xFFu))
                    {
                        ptCur += ((((uint32_t)ptCur[2] << 8) | (uint32_t)ptCur[3]) + 2u);
                    }
                    else
                    {
                        ptCur++;
                    }
                    break;
            }
        }
    }

end:
    if (bFindSmallJpeg == true)
    {
        pJAttrib->flag |= (uint32_t)JATT_STATUS_HAS_SMALL_THUMB;
    }
    else
    {
        pJAttrib->flag &= ~(uint32_t)JATT_STATUS_HAS_SMALL_THUMB;
    }

    return result;
}

static JPG_ERR
_Parse_App02 (
    JPG_STREAM_HANDLE   * pHJStream,
    JPG_PARSER_INFO     * pJPrsInfo)
{
    JPG_ERR         result = JPG_ERR_OK;
    JPG_PRS_BS_CTRL * pJPrsBsCtrl = &pJPrsInfo->jPrsBsCtrl;
    JPG_ATTRIB      * pJAttrib = &pJPrsInfo->jAttrib;
    bool            bFindLargeJpeg = false;
    bool            bFindCtntLst = false;
    bool            bFindNullT = false;
    bool            bFindSOI = false;
    uint8_t         * ptCur = NULL, * ptEnd = NULL;
    uint32_t        entityCnt = 0, entitySize = 0;
    uint8_t         Default_Value[4];
    uint32_t        stage;
    uint32_t        jpegIdx;
    uint16_t        tmpValue = 0;
    uint32_t        i, j, k;

    if (pJAttrib->exifAppCnt < 2u)
    {
        goto end;
    }

    i = 0;
    // ----------------------------------------------
    // Find Contents list
    while (!bFindCtntLst && (i < pJAttrib->exifAppCnt))
    {
        i++;
        // seek position
        jPrs_Seek2SectionStart(pHJStream, pJPrsInfo, pJAttrib->exifAppOffset[i], pJAttrib->exifAppLength[i]);

        ptCur   = pJPrsBsCtrl->bsPrsBuf + pJPrsBsCtrl->curPos;
        ptEnd   = ptCur + pJAttrib->exifAppLength[i];

        if ((*(ptCur + APP2_OFFSET_TYPE) == APP2_CONTENTS_LIST) &&
            (memcmp((ptCur + 2), "FPXR", 4) == 0)) // FPXR
        {
            bool bStorageCase;

            ptCur       += APP2_OFFSET_LIST_INDEX;

            entityCnt   = ( ((uint32_t)*(ptCur) << 8) | (uint32_t)*(ptCur + 1) );
			#if 0
            jpg_msg(1, "EntityCnt %d\n", EntityCnt);
			#endif
            ptCur       += 2;

            // Get the Default value
            k = 0;
            for (j = 0; j < entityCnt; j++)
            {
                entitySize  = (uint32_t)GetDWord(pJPrsInfo->bLEndian, ptCur);
                ptCur       += 4;

                if (entitySize == 0xFFFFFFFFu)
                {
                    bStorageCase = true;
                    ptCur++;
                }
                else
                {
                    bStorageCase        = false;
                    Default_Value[k]  = *ptCur;
					k++;
					ptCur++;
					#if 0
                    jpg_msg(1, "Default Value %d\n", Default_Value[k-1]);
					#endif
                }

                // search NULL termination
                stage       = 0;
                bFindNullT  = false;

                while ( (ptCur < ptEnd) && !bFindNullT )
                {
                    if (*ptCur++ == 0x00u)
                    {
                        stage++;
                        if (stage == 3u)
                        {
                            bFindNullT = true;
                        }
                    }
                    else
                    {
                        stage = 0;
                    }
                }

                if (bStorageCase)
                {
                    ptCur += 16;
                }
            }

            bFindCtntLst = true;
        }
    }

    if (!bFindCtntLst)
    {
        goto end;
    }

    jpegIdx = 0;
    // ----------------------------------------------
    // Parse second jpeg
    while (i < pJAttrib->exifAppCnt)
    {
        i++;
        // seek position
        jPrs_Seek2SectionStart(pHJStream, pJPrsInfo, pJAttrib->exifAppOffset[i], pJAttrib->exifAppLength[i]);

        ptCur   = pJPrsBsCtrl->bsPrsBuf + pJPrsBsCtrl->curPos;
        ptEnd   = ptCur + pJAttrib->exifAppLength[i];

        if ((*(ptCur + APP2_OFFSET_TYPE)) == APP2_STREAM_DATA)
        {
            if (bFindSOI == false)
            {
                jpegIdx = ( ((uint32_t)*(ptCur + APP2_OFFSET_LIST_INDEX) << 8) | (uint32_t)*(ptCur + APP2_OFFSET_LIST_INDEX + 1) );
            }
            else
            {
                pJAttrib->app2StreamCnt++;
                if ( ( ((uint32_t)*(ptCur + APP2_OFFSET_LIST_INDEX) << 8) | (uint32_t)*(ptCur + APP2_OFFSET_LIST_INDEX + 1) ) != jpegIdx)
                {
                    jpg_msg(0, "0x%02x, 0x%02x\n", *(ptCur + APP2_OFFSET_LIST_INDEX), *(ptCur + APP2_OFFSET_LIST_INDEX + 1));
                    pJAttrib->app2StreamCnt--;
                    continue;
                }

                ptCur += APP2_OFFSET_STREAM_DATA;

                // skip default value
                while (*ptCur == Default_Value[jpegIdx - 1u])
                {
                    ptCur++;
                }

                pJAttrib->app2StreamOffset[pJAttrib->app2StreamCnt] = _GetCurFactorPos(pHJStream, pJPrsBsCtrl->realSize, ((uint32_t)ptCur - (uint32_t)pJPrsBsCtrl->bsPrsBuf));
                pJAttrib->app2StreamSize[pJAttrib->app2StreamCnt]   = (uint32_t)ptEnd - (uint32_t)ptCur;
            }

            while (ptCur < ptEnd)
            {
                tmpValue = (uint16_t)GetWord(pJPrsInfo->bLEndian, ptCur);
                switch (tmpValue)
                {
                    case 0xFFD8:
                        bFindSOI                                            = true;
                        pJAttrib->app2StreamOffset[pJAttrib->app2StreamCnt] = _GetCurFactorPos(pHJStream, pJPrsBsCtrl->realSize, ((uint32_t)ptCur - (uint32_t)pJPrsBsCtrl->bsPrsBuf));
                        pJAttrib->app2StreamSize[pJAttrib->app2StreamCnt]   = (uint32_t)ptEnd - (uint32_t)ptCur;
                        ptCur                                               += 2;
                        break;

                    case 0xFFC2:
                        pJAttrib->jBaseLiteInfo[JPG_DEC_LARGE_THUMB].bJprog = true;
						pJAttrib->jBaseLiteInfo[JPG_DEC_LARGE_THUMB].width = (((uint32_t)*(ptCur + 7) << 8) | (uint32_t)*(ptCur + 8));
                        pJAttrib->jBaseLiteInfo[JPG_DEC_LARGE_THUMB].height = (((uint32_t)*(ptCur + 5) << 8) | (uint32_t)*(ptCur + 6));
                        ptCur                                               += ((((uint32_t)ptCur[2] << 8) | (uint32_t)ptCur[3]) + 2u);
                        break;

                    case 0xFFC0:
                        pJAttrib->jBaseLiteInfo[JPG_DEC_LARGE_THUMB].width = (((uint32_t)*(ptCur + 7) << 8) | (uint32_t)*(ptCur + 8));
                        pJAttrib->jBaseLiteInfo[JPG_DEC_LARGE_THUMB].height = (((uint32_t)*(ptCur + 5) << 8) | (uint32_t)*(ptCur + 6));
                        ptCur                                               += ((((uint32_t)ptCur[2] << 8) | (uint32_t)ptCur[3]) + 2u);
                        break;

                    case 0xFFD9:
                        if (bFindSOI == true)
                        {
                            bFindLargeJpeg = true;
                            pJAttrib->app2StreamCnt++;
                            goto end;
                        }
                        break;

                    default:
                        if ((*ptCur == 0xFFu) && ((*(ptCur + 1) & 0xC0u) == 0xC0u) && (*(ptCur + 1) != 0xFFu))
                        {
                            ptCur += ((((uint32_t)ptCur[2] << 8) | (uint32_t)ptCur[3]) + 2u);
                        }
                        else
                        {
                            ptCur++;
                        }
                        break;
                }
            }
        }
    }

end:
    if (bFindLargeJpeg == true)
    {
        pJAttrib->flag |= (uint32_t)JATT_STATUS_HAS_LARGE_THUMB;
    }
    else
    {
        pJAttrib->flag &= ~(uint32_t)JATT_STATUS_HAS_LARGE_THUMB;
    }

    return result;
}

// =============================================================================
//                  Public Function Definition
// =============================================================================
void
jPrs_Seek2SectionStart (
    JPG_STREAM_HANDLE   * pHJStream,
    JPG_PARSER_INFO     * pJPrsInfo,
    uint32_t            destPos,
    uint32_t            sectLength)
{
    /**
     *                    |---------- realSize ---------|
     *              prevStreamPos    destPos       curStreamPos
     *     ---------------|-------------i---------------|----------|
     *
     *      ps. MUST be "sectLength < bsPrsBufMaxLeng"
     **/
    uint32_t        curStreamPos    = 0;
    uint32_t        prevStreamPos   = 0;
    JPG_PRS_BS_CTRL * pJPrsBsCtrl   = &pJPrsInfo->jPrsBsCtrl;
    JPG_STREAM_DESC * pJStreamDesc  = &pHJStream->jStreamDesc;

    if (pJStreamDesc->jTell_stream != NULL)
    {
        (void)pJStreamDesc->jTell_stream(pHJStream, &curStreamPos, NULL);
    }

    prevStreamPos = curStreamPos - pJPrsBsCtrl->realSize;

    if ((prevStreamPos < destPos) && ((destPos + sectLength) < curStreamPos))
    {
        // one bs buffer case
        pJPrsBsCtrl->curPos = destPos - prevStreamPos;
    }
    else
    {
        // multi-bs buffer case
        if (pJStreamDesc->jSeek_stream != NULL)
        {
            (void)pJStreamDesc->jSeek_stream(pHJStream, destPos, JPG_SEEK_SET, NULL);
        }

        if (pJStreamDesc->jFull_buf != NULL)
        {
            (void)pJStreamDesc->jFull_buf(pHJStream, pJPrsBsCtrl->bsPrsBuf,
                                    pJPrsBsCtrl->bsPrsBufMaxLeng,
                                    &pJPrsBsCtrl->realSize, NULL);
        }
        pJPrsBsCtrl->curPos     = 0;

        pJPrsInfo->remainSize   = pJPrsInfo->fileLength - (destPos + pJPrsBsCtrl->realSize);
    }
}

// parsing base jpg info, ex. Q-table, H-thable, ...etc.
JPG_ERR
jPrs_BaseParser (
    JPG_STREAM_HANDLE   * pHJStream,
    JPG_PARSER_INFO     * pJPrsInfo)
{
    JPG_ERR         result = JPG_ERR_OK;
    bool            bFindHTable = false;
#if 0	
    JPG_STREAM_DESC * pJStreamDesc = &pHJStream->jStreamDesc;
#endif
    JPG_PRS_BS_CTRL * pJPrsBsCtrl = &pJPrsInfo->jPrsBsCtrl;
    JPG_ATTRIB      * pJAttrib = &pJPrsInfo->jAttrib;
    uint8_t         * ptCur, * ptEnd;

    // parsing process
    ptCur = pJPrsBsCtrl->bsPrsBuf + pJPrsBsCtrl->curPos;

    if (pJPrsBsCtrl->b1stSection == true)
    {
        pJPrsBsCtrl->b1stSection = false;

        // for the swf jpeg
        if ((*ptCur == JPG_MARKER_START) &&
            (*(ptCur + 1) == JPG_END_OF_IMAGE_MARKER))
        {
            ptCur += 2;
        }

        if ((*ptCur == JPG_MARKER_START) &&
            (*(ptCur + 1) == JPG_START_OF_IMAGE_MARKER))
        {
            ptCur += 2;
        }
        else
        {
            pJAttrib->flag  |= ( (uint32_t)JATT_STATUS_UNSUPPORT_PRIMARY |
                (uint32_t)JATT_STATUS_UNSUPPORT_SMALL_THUMB |
                (uint32_t)JATT_STATUS_UNSUPPORT_LARGE_THUMB );
            result          = JPG_ERR_NOT_JPEG;
            goto end;
        }
    }

    ptEnd                           = pJPrsBsCtrl->bsPrsBuf + pJPrsBsCtrl->realSize;
    pJPrsInfo->jFrmComp.bFindHDT    = false;

    // parsing process
    while (*ptCur == JPG_MARKER_START)
    {
        switch (*(ptCur + 1))
        {
            default:
                if (((*(ptCur + 1) & 0xF0u) == 0xE0u) ||
                    ((*(ptCur + 1) & 0xF0u) == 0xF0u))
                {
                    uint32_t sectionLength = 0;

                    ptCur           += 2;
                    sectionLength   = (uint32_t)GetWord(pJPrsInfo->bLEndian, ptCur);

                    if ((ptCur + sectionLength) < ptEnd)
                    {
                        ptCur += sectionLength;
                    }
                    else
                    {
#if 1
                        result = JPG_ERR_JPROG_STREAM;
                        goto end;
#else
                        int skipLength = 0;

                        skipLength = sectionLength - ((uint32_t)ptEnd - (uint32_t)ptCur);
                        if (pJStreamDesc->jSeek_stream)
                        {
                            pJStreamDesc->jSeek_stream(pHJStream, skipLength, JPG_SEEK_CUR, 0);
                        }

                        pJPrsInfo->remainSize -= skipLength;

                        if (pJStreamDesc->jFull_buf)
                        {
                            pJStreamDesc->jFull_buf(pHJStream, pJPrsBsCtrl->bsPrsBuf,
                                                    pJPrsBsCtrl->bsPrsBufMaxLeng,
                                                    &pJPrsBsCtrl->realSize, 0);
                        }
                        // reset parameters
                        ptCur                   = pJPrsBsCtrl->bsPrsBuf;
                        pJPrsInfo->remainSize   -= pJPrsBsCtrl->realSize;
#endif
                    }
                }
                else
                {
                    ptCur++;
                }
                break;

            case JPG_START_OF_IMAGE_MARKER:
                ptCur += 2;
                break;

            case JPG_END_OF_IMAGE_MARKER:
                ptCur   += 2;
                result  = JPG_ERR_HDER_ONLY_TABLES;
                goto end;
                break;

            case JPG_NO_MEANING_MARKER:
                ptCur++;
                break;

            case JPG_DIFF_PROGRESSIVE:
            case JPG_DIFF_LOSSLESS:
            case JPG_DIFF_SEQUENTIAL:
            case JPG_EXTENDED_SEQUENTIAL:
            case JPG_LOSSLESS_SEQUENTIAL:
                result = JPG_ERR_HW_NOT_SUPPORT;
                jpg_msg(JPG_MSG_TYPE_ERR, "H/W not support !!\n");
                goto end;
                break;

            case JPG_PROGRESSIVE:
                result = JPG_ERR_JPROG_STREAM;
                jpg_msg(JPG_MSG_TYPE_ERR, "Jprog data, not support !!\n");
                goto end;
                break;

            case JPG_BASELINE_MARKER:
                ptCur   += 2;
                ptCur   = _CompleteSection(pHJStream, ptCur, ptEnd, pJPrsInfo);
                if (ptCur == NULL)
                {
                    result = JPG_ERR_JPROG_STREAM;
                    goto end;
                }

                ptCur = _ParseSOF00(pHJStream, ptCur, pJPrsInfo);
                if (ptCur == NULL)
                {
                    result = JPG_ERR_JPROG_STREAM;
                    goto end;
                }
                break;

            case JPG_HUFFMAN_TABLE_MARKER:
                ptCur   += 2;
                ptCur   = _CompleteSection(pHJStream, ptCur, ptEnd, pJPrsInfo);
                if (ptCur == NULL)
                {
                    result = JPG_ERR_JPROG_STREAM;
                    goto end;
                }

                if (pJPrsInfo->bSkipHDT == false)
                {
                    ptCur = _ParseDHT(pHJStream, ptCur, pJPrsInfo);
                    if (ptCur == NULL)
                    {
                        result = JPG_ERR_JPROG_STREAM;
                        goto end;
                    }
                }
                else
                {
                    ptCur += GetWord(pJPrsInfo->bLEndian, ptCur);
                }

                pJPrsInfo->jFrmComp.bFindHDT    = true;
                bFindHTable                     = true;
                break;

            case JPG_Q_TABLE_MARKER:
                ptCur   += 2;
                ptCur   = _CompleteSection(pHJStream, ptCur, ptEnd, pJPrsInfo);
                if (ptCur == NULL)
                {
                    result = JPG_ERR_JPROG_STREAM;
                    goto end;
                }

                ptCur = _ParseDQT(pHJStream, ptCur, pJPrsInfo);
                if (ptCur == NULL)
                {
                    result = JPG_ERR_JPROG_STREAM;
                    goto end;
                }
                break;

            case JPG_DRI_MARKER:
                ptCur   += 2;
                ptCur   = _CompleteSection(pHJStream, ptCur, ptEnd, pJPrsInfo);
                if (ptCur == NULL)
                {
                    result = JPG_ERR_JPROG_STREAM;
                    goto end;
                }

                ptCur = _ParseDRI(pHJStream, ptCur, pJPrsInfo);
                break;

            case JPG_START_OF_SCAN_MARKER:
                ptCur   += 2;
                ptCur   = _CompleteSection(pHJStream, ptCur, ptEnd, pJPrsInfo);
                if (ptCur == NULL)
                {
                    result = JPG_ERR_JPROG_STREAM;
                    goto end;
                }

                ptCur = _ParseSOS(pHJStream, ptCur, pJPrsInfo);
                if (ptCur == NULL)
                {
                    result = JPG_ERR_JPROG_STREAM;
                    goto end;
                }
                break;

            case 0x00: // the first entropy code may be "0xFF 0x00"
                goto end;
                break;
        }

        // check remain bs_buf size
    }

    if (bFindHTable == false)
    {
        // loading default H-Table
        _SetDefHTable(pHJStream, pJPrsInfo);
        jpg_msg(JPG_MSG_TYPE_ERR, "JPEG loss H-table section, use default table !!\n");
    }

end:
    pJPrsBsCtrl->curPos = (uint32_t)ptCur - (uint32_t)pJPrsBsCtrl->bsPrsBuf;

    if ((result != JPG_ERR_OK) && (result != JPG_ERR_HDER_ONLY_TABLES) && (result != JPG_ERR_JPROG_STREAM))
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, "%s() Err 0x%x ", __FUNCTION__, result);
    }

    return result;
}

// parsing app section info, ex. EXIF, 0xE0 ~ 0xEF, 0xF0 ~ 0xFF
JPG_ERR
jPrs_AppParser (
    JPG_STREAM_HANDLE   * pHJStream,
    JPG_PARSER_INFO     * pJPrsInfo)
{
    JPG_ERR result = JPG_ERR_OK;

    // -----------------------------------
    // get App sections position
    result = _AppSectionOverview(pHJStream, pJPrsInfo);
    if (result != JPG_ERR_OK)
    {
        jpg_msg_ex(JPG_MSG_TYPE_ERR, " Jpg err 0x%x ! ", result);
        goto end;
    }

    // ---------------------------------
    // detail parse app and get thumbnail position
    // app1
    (void)_Parse_App01(pHJStream, pJPrsInfo);

    // app2
    (void)_Parse_App02(pHJStream, pJPrsInfo);

end:
    return result;
}
