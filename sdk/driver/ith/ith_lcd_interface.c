/*
 * Copyright (c) 2024 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * HAL LCD interface functions.
 *
 * @author Irene Wang
 * @version 1.0
 */
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>
#include <sys/ioctl.h>
#include <ith/ith_cmdq.h>
#include "ith_cfg.h"
#include "ith_lcd_interface.h"
#include "ite/itp.h"
#include "ith/ith_lcd.h"
#if defined(IT6122) || defined(IT6151) || defined(GM8775)
#include "iic/mmp_iic.h"
#endif

#if defined(IT6122) || defined(IT6151) || defined(GM8775)
#define IIC_PORT_NUM        IIC_PORT_1  //IT6122 or IT6151 or GM8775 using IIC port
#endif


#if defined(IT6122)
static int interrupt_count = 0;
#endif

#if defined(IT6151)
static int errorSum = 0;

#if defined PANEL_RESOLUTION_1280x800_4LANE_24B
static struct PanelInfoStr sPInfo = { 0, 1280, DP_2_LANE, B_LBR, MIPI_4_LANE, H_Neg, V_Pos, 0, RGB_24b, H_ReSync, F_ReSync, 2, 0, 0, 4 };
#elif defined PANEL_RESOLUTION_1920x1080p60_4LANE_24B
static struct PanelInfoStr sPInfo = { 16, 1920, DP_2_LANE, B_HBR, MIPI_4_LANE, H_Pos, V_Pos, 0, RGB_24b, H_ReSync, F_ReSync, 2, 1, 0, 2 };
#elif defined PANEL_RESOLUTION_1920x1080p60_4LANE_18B
static struct PanelInfoStr sPInfo = { 16, 1920, DP_2_LANE, B_LBR, MIPI_4_LANE, H_Pos, V_Pos, 0, RGB_18b_L, H_ReSync, F_ReSync, 2, 1, 0, 4 };
#elif defined PANEL_RESOLUTION_1920x1200_4LANE_24B
static struct PanelInfoStr sPInfo = { 0, 1920, DP_2_LANE, B_HBR, MIPI_4_LANE, H_Pos, V_Neg, 0, RGB_24b, H_ReSync, F_ReSync, 2, 1, 0, 2 };
#elif defined PANEL_RESOLUTION_2048x1536_4LANE_24B_UFO
static struct PanelInfoStr sPInfo = { 0, 2048, DP_4_LANE, B_LBR, MIPI_4_LANE, H_Neg, V_Pos, En_UFO, RGB_24b, 0, 0, 2, 1, 0, 4 };
#elif defined PANEL_RESOLUTION_2048x1536_4LANE_24B
static struct PanelInfoStr sPInfo = { 0, 2048, DP_4_LANE, B_LBR, MIPI_4_LANE, H_Neg, V_Pos, 0, RGB_24b, H_ReSync, F_ReSync, 2, 1, 0, 2 };
#elif defined PANEL_RESOLUTION_2048x1536_4LANE_18B
static struct PanelInfoStr sPInfo = { 0, 2048, DP_4_LANE, B_LBR, MIPI_4_LANE, H_Neg, V_Pos, 0, RGB_18b_L, H_ReSync, F_ReSync, 2, 1, 0, 4 };
#elif defined PANEL_RESULUTION_1536x2048_4LANE_24B
static struct PanelInfoStr sPInfo = { 0, 1536, DP_4_LANE, B_LBR, MIPI_4_LANE, H_Pos, V_Pos, 0, RGB_24b, H_ReSync, F_ReSync, 2, 1, 0, 2 };
#elif defined PANEL_RESULUTION_1536x2048_4LANE_24B_UFO
static struct PanelInfoStr sPInfo = { 0, 1536, DP_4_LANE, B_LBR, MIPI_4_LANE, H_Pos, V_Pos, En_UFO, RGB_24b, 0, 0, 2, 1, 0, 4 };
#elif defined PANEL_RESOLUTION_1366x768_4LANE_24B
static struct PanelInfoStr sPInfo = { 0, 1366, DP_2_LANE, B_LBR, MIPI_4_LANE, H_Neg, V_Neg, 0, RGB_24b, H_ReSync, F_ReSync, 2, 0, 0, 4 };
#elif defined PANEL_RESOLUTION_1366x768_2LANE_24B
static struct PanelInfoStr sPInfo = { 0, 1366, DP_2_LANE, B_LBR, MIPI_2_LANE, H_Neg, V_Neg, 0, RGB_24b, H_ReSync, F_ReSync, 5, 0, 0, 4 };
#elif defined PANEL_RESOLUTION_1368x768_4LANE_24B
static struct PanelInfoStr sPInfo = { 0, 1368, DP_2_LANE, B_LBR, MIPI_4_LANE, H_Neg, V_Neg, 0, RGB_24b, H_ReSync, F_ReSync, 2, 0, 0, 4 };
#elif defined PANEL_RESOLUTION_1368x768_4LANE_18B_1DP
static struct PanelInfoStr sPInfo = { 0, 1368, DP_1_LANE, B_HBR, MIPI_4_LANE, H_Neg, V_Neg, 0, RGB_18b_L, H_ReSync, F_ReSync, 2, 0, 0, 4 };
#elif defined PANEL_RESOLUTION_1280x720_4LANE_24B
static struct PanelInfoStr sPInfo = { 0, 1280, DP_2_LANE, B_LBR, MIPI_4_LANE, H_Neg, V_Neg, 0, RGB_24b, H_ReSync, F_ReSync, 2, 0, 0, 4 };
#endif
#endif

#if defined(IT6122) || defined(IT6151) || defined(GM8775)
static void i2c_write_byte(uint8_t dev_addr, uint8_t addr, uint8_t data)
{
    uint8_t     CmdBuf[2];

    CmdBuf[0] = addr;
    CmdBuf[1] = data;
    mmpIicSendDataEx(IIC_PORT_NUM, dev_addr, CmdBuf, 2);
}

static void i2c_read_byte(uint8_t dev_addr, uint8_t addr, uint8_t *dataBuffer)
{
    *dataBuffer = addr;

    mmpIicReceiveData(IIC_PORT_NUM, dev_addr, &addr, 1, dataBuffer, 1);
}
#endif

#if defined(IT6122)
static void IT6122_DumpReg(void)
{
    uint8_t i = 0U;
    uint8_t value;

    (void)printf("HMPRX_R Reg\r\n");
    for (i = 0U; i < 0xFFU; i++)
    {

        if ((i % 0x10U) == 0x00U)
        {
            (void)printf("\r\n%02x|\t", i);
        }
        i2c_read_byte(0x6CU, i, &value);
        (void)printf("%02x ", value);
    }
    i2c_read_byte(0x6CU, 0xFFU, &value);
    (void)printf("%02x\r\n", value);
}

#if 0
static uint8_t g_u8SysSts = 0U;
#endif
#define CHECK_CNT      20U

static uint8_t IT6122_EnSSCPLL(void)
{
    if (0U == EnLVDMode)
    {
        return EnSSCPLL;
    }
    return 0U;
}

// *****************************************
//  Copyright (C) 2009-2022
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
// *****************************************
//   @file   <IT6122.c>
//   @author Mike.Hsieh@ite.com.tw
//   @date   2022/06/13
//   @fileversion: IT6122_1.07
// *****************************************
static bool IT6122_Sys_ini(void)
{
    uint8_t VenID[2], DevID[2], u8RevID;
    uint8_t value     = 0U;
    uint16_t nCnt = 0U;
    uint8_t u8WaitCnt = 100U / CHECK_CNT;
    //uint8_t u8Ary[CHECK_CNT] = { 0 };

    i2c_read_byte(MPRxDevAddr, 0x00U, &VenID[0]);
    i2c_read_byte(MPRxDevAddr, 0x01U, &VenID[1]);
    i2c_read_byte(MPRxDevAddr, 0x02U, &DevID[0]);
    i2c_read_byte(MPRxDevAddr, 0x03U, &DevID[1]);
    i2c_read_byte(MPRxDevAddr, 0x04U, &u8RevID);

    (void)printf("###############################################\r\r\n ");
    (void)printf("#            MP2LV Initialization             #\r\r\n ");
    (void)printf("###############################################\r\r\n ");

    if ((VenID[0] != 0x54U) || (VenID[1] != 0x49U) || (DevID[0] != 0x22U) ||
        (DevID[1] != 0x61U) ||
        ((u8RevID != 0xA0U) && (u8RevID != 0xB0U) && (u8RevID != 0xC0U) &&
         (u8RevID != 0xC1U) && (u8RevID != 0xC2U)))
    {
        (void)printf("\r\n Error: Can not find IT6121 MP2LV Device !!!\r\n ");

        (void)printf("Current DevID=%02X%02X\r\n ", DevID[1], DevID[0]);
        (void)printf("Current VenID=%02X%02X\r\n ", VenID[1], VenID[0]);
        (void)printf("Current u8RevID=%02X\r\n \r\n ", u8RevID);
        return false;
    }
    i2c_write_byte(MPRxDevAddr, 0x05U, 0x17U);
    i2c_write_byte(MPRxDevAddr, 0xC0U, (EnHighPCLK << 4U) + 0x01U);//0x11
    i2c_write_byte(MPRxDevAddr, 0x05U, 0x1FU);
    i2c_write_byte(MPRxDevAddr, 0x05U, 0x10U);
    i2c_write_byte(MPRxDevAddr, 0xCBU, 0x50U);//manual reset

    i2c_write_byte(MPRxDevAddr, 0xC0U, (EnHighPCLK << 4U));
    i2c_write_byte(MPRxDevAddr, 0xC0U, (EnHighPCLK << 4U) + 0x01U);
    if ((u8RevID != 0xA0U) && (u8RevID != 0xB0U))
    {
        i2c_write_byte(MPRxDevAddr, 0xC5U, 0xC0U + (EnHighPCLK << 2U)); // 0xC4
        if (0U != IT6122_EnSSCPLL())
        {
            i2c_write_byte(MPRxDevAddr, 0xC5U, 0xC0U + (EnHighPCLK << 2U) + (IT6122_EnSSCPLL() << 1U)); // 0xC4
        }
        i2c_write_byte(MPRxDevAddr, 0xC6U, 0x20U);
        i2c_write_byte(MPRxDevAddr, 0xC7U, 0x20U);
        #if 0
        STA_CHANGE(g_u8SysSts, FW_CTL_TXOUT_MASK, FW_CTL_TXOUT_ENABLE);
        #endif
    }
    #if 0
    if (g_u8DipSwitchSts)
    {
        i2c_write_byte(MPRxDevAddr, 0xCBU, 0x50U + (g_u8DipSwitchSts & 0x08U) + ((g_u8DipSwitchSts & 0x04U) >> 2U));
    }
    else
    {
        i2c_write_byte(MPRxDevAddr, 0xCBU, 0x50U + (EnLVDMode << 3U) + (En6bitout << 2U) + MAPVESA); // 0x30
    }
    #endif
    i2c_write_byte(MPRxDevAddr, 0xCBU, 0x50U + (EnLVDMode << 3U) + (En6bitout << 2U) + MAPVESA);//0x30
    i2c_write_byte(MPRxDevAddr, 0xC9U, (EnLVLinkSwap << 4U) + (EnLVL1PNSwap << 3U) + (EnLVL0PNSwap << 2U) + (EnLVL1LnSwap << 1U) + EnLVL0LnSwap);
    #if 0
    // swing level adjust
    i2c_write_byte(MPRxDevAddr, 0xC3U, 0x86U + (LVSwingLevel << 4U));
    #endif

    //LVTx setting
    if ((u8RevID != 0xA0U) && (u8RevID != 0xB0U))
    {
        if (EnLVTxSSC && (0U != IT6122_EnSSCPLL()))
        {
            i2c_write_byte(MPRxDevAddr, 0xF2U, LVSDM & 0xFFU);
            i2c_write_byte(MPRxDevAddr, 0xF3U, (LVSDM & 0xFF00U) >> 8U);
            i2c_write_byte(MPRxDevAddr, 0xF4U, LVSDMINC & 0xFFU);
            i2c_write_byte(MPRxDevAddr, 0xF5U,
                           (EnSSCBufAutoRst << 6U) + (EnLVTxSSC << 4U) +
                               ((LVSDMINC & 0x700U) >> 8U));
        }
        else
        {
            i2c_write_byte(MPRxDevAddr, 0xF5U, 0x01U + (EnSSCBufAutoRst << 6U));
        }
        i2c_write_byte(MPRxDevAddr, 0xD8U,
                       (EnSSCBufConcat << 6U) + 0x10U); // 0x10
    }

    if ((u8RevID != 0xA0U) && (u8RevID != 0xB0U))
    {
        i2c_write_byte(MPRxDevAddr, 0xDAU, EnLVVidRecInt << 7U); // 0x80
    }
    else
    {
        i2c_write_byte(MPRxDevAddr, 0xDFU, EnLVVidRecInt << 7U);
    }

    i2c_write_byte(MPRxDevAddr, 0x09U, 0xFFU);
    i2c_write_byte(MPRxDevAddr, 0x0AU, 0xFFU);
    i2c_write_byte(MPRxDevAddr, 0x0BU, 0xFFU);
    if ((u8RevID != 0xA0U) && (u8RevID != 0xB0U))
    {
        i2c_write_byte(MPRxDevAddr, 0xF8U, 0x03U); // vidrec, sscfifo
    }

    i2c_write_byte(MPRxDevAddr, 0x0DU, 0x00U);
    i2c_write_byte(MPRxDevAddr, 0x0CU, (EnLaneSwap << 3U) + (EnPNSwap << 2U) + MPLaneNum);
    #if 0
    if (g_u8DipSwitchSts)
    {
        iTE_u8 u8Lane = g_u8DipSwitchSts & 0x03;
        if (u8Lane != 0x02U)
        {   // 0: 1lane , 1:2 lane, 3: 4 lane
            i2c_write_byte(MPRxDevAddr, 0x0CU, (EnLaneSwap << 3U) + (EnPNSwap << 2U) + u8Lane);
        }
    }
    else
    {
        i2c_write_byte(MPRxDevAddr, 0x0CU, (EnLaneSwap << 3U) + (EnPNSwap << 2U) + MPLaneNum);
    }
    #endif
    i2c_write_byte(MPRxDevAddr, 0x11U, (EnExtStdby << 3U) + (InvPCLK << 1U) + InvMCLK);//0x80
    #if 0
    i2c_write_byte(MPRxDevAddr, 0x12U, 0x00U);//
    #endif
    i2c_write_byte(MPRxDevAddr, 0x12U, 0x01U);//
    i2c_write_byte(MPRxDevAddr, 0x18U, (EnSyncErr << 7U) + (SkipStg << 4U) + HSSetNum);//0x80
    i2c_write_byte(MPRxDevAddr, 0x19U, 0x02U + EnDeSkew);
    i2c_write_byte(MPRxDevAddr, 0x20U, 0x03U);
    i2c_write_byte(MPRxDevAddr, 0x44U, (MREC_Update << 5U) + (PREC_Update << 4U));
    #if (MIPI_EVENT_MODE == 1)
    i2c_write_byte(MPRxDevAddr, 0x4BU, 0x01U);
    i2c_write_byte(MPRxDevAddr, 0x4EU, 0x00U);
    #else
    i2c_write_byte(MPRxDevAddr, 0x4BU, (EnFReSync << 4U) + 0x01U);
    i2c_write_byte(MPRxDevAddr, 0x4EU, (EnVReSync << 3U) + (EnHReSync << 2U));
    #endif
    #if 0
    i2c_write_byte(MPRxDevAddr, 0x4BU, (EnFReSync << 4U) + 0x01U);
    #endif
    i2c_write_byte(MPRxDevAddr, 0x4CU, 0x04U);//0x02
    #if 0
    i2c_write_byte(MPRxDevAddr, 0x4EU, (EnVReSync << 3U) + (EnHReSync << 2U));
    #endif
    i2c_write_byte(MPRxDevAddr, 0x4FU, 0x01U);
    i2c_write_byte(MPRxDevAddr, 0x27U, MPVidType);
    i2c_write_byte(MPRxDevAddr, 0x70U, 0x01U);
    i2c_write_byte(MPRxDevAddr, 0x72U, 0x07U);
    i2c_write_byte(MPRxDevAddr, 0x73U, 0x04U);//0x03
    i2c_write_byte(MPRxDevAddr, 0x80U, 0x03U);
    i2c_write_byte(MPRxDevAddr, 0x84U, 0xFFU);
    i2c_write_byte(MPRxDevAddr, 0x85U, EnMPRxPRBS);

    if (EnMPRxPRBS)
    {
        i2c_write_byte(MPRxDevAddr, 0x71U, 0xC0U);
    }

    #if (EnMBPM)
    i2c_write_byte(MPRxDevAddr, 0xA1U, 0x00U);
    i2c_write_byte(MPRxDevAddr, 0xA2U, 0x00U);

    i2c_write_byte(MPRxDevAddr, 0xA3U, BPHSW); // HSW//0x60
    i2c_write_byte(MPRxDevAddr, 0xA5U, BPVSW); // VSW
    #endif
    i2c_write_byte(MPRxDevAddr, 0xA0U, EnMBPM);

    i2c_write_byte(MPRxDevAddr, 0x92U, 0x1EU);

    i2c_write_byte(MPRxDevAddr, 0x80U, MPPCLKSel);

    i2c_write_byte(MPRxDevAddr, 0x70U, 0x01U);
    i2c_write_byte(MPRxDevAddr, 0x05U, 0x14U);

    #if(EN_INT_MODE == 0)
    i2c_write_byte(MPRxDevAddr, 0xC0U, (EnHighPCLK << 4U) + 0x03U);
    i2c_write_byte(MPRxDevAddr, 0xC5U, 0xC0U + (EnHighPCLK << 2U) + (IT6122_EnSSCPLL() << 1U) + IT6122_EnSSCPLL());	//0xC4
    #endif
    i2c_write_byte(MPRxDevAddr, 0x05U, 0x00U);
    i2c_write_byte(MPRxDevAddr, 0xCBU, 0x10U + (EnLVDMode << 3U) + (En6bitout << 2U) + MAPVESA);//release manual reset
    i2c_write_byte(MPRxDevAddr, 0x91U, 0xFBU);//RCLK using default setting
    i2c_write_byte(MPRxDevAddr, 0x31U, 0x3FU);
    i2c_write_byte(MPRxDevAddr, 0x33U, 0x3FU);
    i2c_write_byte(MPRxDevAddr, 0x35U, 0x3FU);
    i2c_write_byte(MPRxDevAddr, 0x37U, 0x3FU);
    i2c_write_byte(MPRxDevAddr, 0x39U, 0x3FU);
    #if 0
    i2c_write_byte(MPRxDevAddr, 0x3AU, 0x00U);
    i2c_write_byte(MPRxDevAddr, 0x3CU, 0x7FU);
    i2c_write_byte(MPRxDevAddr, 0x3EU, 0x7FU);
    #endif
    i2c_write_byte(MPRxDevAddr, 0x3BU, 0x1FU);
    i2c_write_byte(MPRxDevAddr, 0x3DU, 0x1FU);
    i2c_write_byte(MPRxDevAddr, 0x3FU, 0x1FU);
    i2c_write_byte(MPRxDevAddr, 0x41U, 0x1FU);
    i2c_write_byte(MPRxDevAddr, 0x43U, 0x1FU);

    #if (MIPI_EVENT_MODE == 1)
    if ((u8RevID == 0xA0U) || (u8RevID == 0xB0U))
    {
        i2c_write_byte(MPRxDevAddr, 0x33U, 0x80U);
        i2c_write_byte(MPRxDevAddr, 0x32U, (uint8_t)((0U == EnMBPM) ? 0x08U : BPHSW));
        i2c_write_byte(MPRxDevAddr, 0x3DU, 0x80U);
        i2c_write_byte(MPRxDevAddr, 0x3CU, (uint8_t)((0U == EnMBPM) ? 0x02U : BPVSW));
    }
    else
    {
        #if (!EnMBPM)
        // only normal mode need user define
        i2c_write_byte(MPRxDevAddr, 0x33U, 0x80U);
        i2c_write_byte(MPRxDevAddr, 0x32U, 0x08U);
        i2c_write_byte(MPRxDevAddr, 0x3DU, 0x80U);
        i2c_write_byte(MPRxDevAddr, 0x3CU, 0x02U);
        #endif
    }
    #endif

    #if 0
    i2c_write_byte(MPRxDevAddr, 0x4EU, 0x00U);//0x03
    #endif
    i2c_write_byte(MPRxDevAddr, 0xB0U, 0x00U);
    #if 0
    i2c_write_byte(MPRxDevAddr, 0xE2U, 0x00U);
    i2c_write_byte(MPRxDevAddr, 0xE3U, 0x00U);
    i2c_write_byte(MPRxDevAddr, 0xE4U, 0x00U);
    i2c_write_byte(MPRxDevAddr, 0xE5U, 0xFFU);
    i2c_write_byte(MPRxDevAddr, 0xE6U, 0xFFU);
    i2c_write_byte(MPRxDevAddr, 0xE7U, 0xFFU);
    #endif
    i2c_write_byte(MPRxDevAddr, 0xD0U, 0x00U);//0x22
    #if 0
    i2c_write_byte(MPRxDevAddr, 0x05U, 0x00U);
    #endif
    #if(EN_INT_MODE == 0)
    i2c_write_byte(MPRxDevAddr, 0x06U, 0x11U); // 0x01

    do
    {
        i2c_read_byte(MPRxDevAddr, 0x0DU, &value);
        (void)usleep(1000 * (int)CHECK_CNT);
        nCnt++;
    } while (((value & 0x38U) != 0x30U) && (nCnt < u8WaitCnt));
    i2c_read_byte(MPRxDevAddr, 0x0DU, &value);
    (void)printf("%d, %d, %x\r\n", nCnt, CHECK_CNT, value);

        #if 0
    (void)usleep(100 * 1000);//100
        #endif
    i2c_write_byte(MPRxDevAddr, 0x05U, 0x10U);
    i2c_write_byte(MPRxDevAddr, 0xCBU, 0x50U + (EnLVDMode << 3U) + (En6bitout << 2U) + MAPVESA);//set manual reset
    i2c_write_byte(MPRxDevAddr, 0x05U, 0x00U);
    i2c_write_byte(MPRxDevAddr, 0xCBU, 0x10U + (EnLVDMode << 3U) + (En6bitout << 2U) + MAPVESA);//reset manual reset
    if ((u8RevID != 0xA0U) && (u8RevID != 0xB0U))
    {
        i2c_write_byte(MPRxDevAddr, 0xC6U, 0x00U);
        i2c_write_byte(MPRxDevAddr, 0xC7U, 0x00U);
        #if 0
        STA_CHANGE(g_u8SysSts, FW_CTL_TXOUT_MASK, FW_CTL_TXOUT_DISABLE);
        #endif
    }
    #endif

    return true;
}

static void* IT6122_check_Task(void* arg)
{
#if 0
    static uint16_t gu16LVHTotal = 0U;
    static int changeSum = 0;
#endif

    static struct timeval lastTime, currTime;
#if 0
    static uint32_t tick = 0U;
    static uint32_t lasttick = 0U;
#endif

    gettimeofday(&lastTime, NULL);

    for (;;)
    {
        uint8_t u8RegF7;

        i2c_read_byte(MPRxDevAddr, 0xF7U, &u8RegF7);
        i2c_write_byte(MPRxDevAddr, 0xF7U, u8RegF7);

        if (0U != (u8RegF7 & 0x02U))
        {
#if 0
            uint8_t value, value1;
            uint16_t u16LVHTotal;
#endif

            (void)printf("LVDSTx VidRec parameters change Interrupt !!!\n");
    #if 0
            (void)printf("MIPI error status 0x38:0x%x !!\n", ithReadRegA(0xd0c00038U));
            (void)printf("LCD error status 0x160:0x%x !!\n", ithReadRegA(0xd0000160U));
            changeSum = 0;

            for (int i = 0; i < 10; i++)
            {
                i2c_read_byte(MPRxDevAddr, 0xe6U, &value1);
                i2c_read_byte(MPRxDevAddr, 0xe7U, &value);
                u16LVHTotal = value1 + ((value & 0x3fU) << 8U);
                (void)printf("LVHTotal = %d, gu16LVHTotal = %d\n", u16LVHTotal,
                             gu16LVHTotal); // 2120 2128 2112

                if (abs(u16LVHTotal - gu16LVHTotal) > 5)
                {
                    changeSum++;
                }

                gu16LVHTotal = u16LVHTotal;
            }

            (void)printf("interrupt_count = %d, changeSum = %d\n",
                         interrupt_count, changeSum); // 2120 2128 2112
    #endif
            (void)printf("interrupt_count = %d\n", interrupt_count);

            if (interrupt_count > 5)
            {
                uint32_t lcdBaseA = ithLcdGetBaseAddrA();
                uint32_t lcdBaseB = ithLcdGetBaseAddrB();
    #if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
                uint32_t lcdBaseC = ithLcdGetBaseAddrC();
    #endif
                (void)printf("IT6122 Parameter Change Reset LCD "
                             "Start+++++++++++++++++++++++++++++++\n");

                // wait cmdq and 2D idle
                ithCmdQLock(ITH_CMDQ0_OFFSET);
                while (true)
                {
                    if (0U != (ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_SR1_REG + ITH_CMDQ0_OFFSET) &
                               (0x1UL << ITH_CMDQ_ALLIDLE_BIT)))
                    {
                        break;
                    }

                    (void)usleep(100);
                }

                while (0U != (ithReadRegA(0xB07000C4U) & (1U << 0U)))
                {
                    (void)usleep(100);
                }

                ithLcdReset();

                ioctl(ITP_DEVICE_SCREEN, ITP_IOCTL_RESET, NULL);

                (void)usleep(1000);
                ithLcdEnableHwFlip();
                (void)usleep(1000);

                ithLcdSetBaseAddrA(lcdBaseA);
                ithLcdSetBaseAddrB(lcdBaseB);
    #if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
                ithLcdSetBaseAddrC(lcdBaseC);
    #endif
                ioctl(ITP_DEVICE_SCREEN, ITP_IOCTL_POST_RESET, NULL);

                ithCmdQUnlock(ITH_CMDQ0_OFFSET);

                interrupt_count = 0;
            }
            else
            {
                interrupt_count++;
            }
        }
        else
        {
            gettimeofday(&currTime, NULL);
            if (itpTimevalDiff(&lastTime, &currTime) >= 3000)
            {
                if (interrupt_count > 0)
                {
                    interrupt_count--;
                    (void)printf("recover interrupt count = %d\n",
                                 interrupt_count);
                }
                lastTime = currTime;
            }
        }

        (void)usleep(1000);
    }

    return NULL;
}
#endif

#if defined(IT6151)
static void IT6151_DPTX_init(void)
{
#if 0
    (void)printf(" IT6151_DPTX_init !!!\n");
#endif

    i2c_write_byte(DP_I2C_ADDR, 0x05U, 0x29U);
    i2c_write_byte(DP_I2C_ADDR, 0x05U, 0x00U);
    i2c_write_byte(DP_I2C_ADDR, 0x09U, INT_MASK);// Enable HPD_IRQ,HPD_CHG,VIDSTABLE
    i2c_write_byte(DP_I2C_ADDR, 0x0AU, 0x00U);
    i2c_write_byte(DP_I2C_ADDR, 0x0BU, 0x00U);
    i2c_write_byte(DP_I2C_ADDR, 0xC5U, 0xC1U);
    i2c_write_byte(DP_I2C_ADDR, 0xB5U, 0x00U);
    i2c_write_byte(DP_I2C_ADDR, 0xB7U, 0x80U);
    i2c_write_byte(DP_I2C_ADDR, 0xC4U, 0xF0U);
    i2c_write_byte(DP_I2C_ADDR, 0x06U, 0xFFU);// Clear all interrupt
    i2c_write_byte(DP_I2C_ADDR, 0x07U, 0xFFU);// Clear all interrupt
    i2c_write_byte(DP_I2C_ADDR, 0x08U, 0xFFU);// Clear all interrupt
    i2c_write_byte(DP_I2C_ADDR, 0x05U, 0x00U);
    i2c_write_byte(DP_I2C_ADDR, 0x0cU, 0x08U);
    i2c_write_byte(DP_I2C_ADDR, 0x21U, 0x05U);
    i2c_write_byte(DP_I2C_ADDR, 0x3aU, 0x04U);
    i2c_write_byte(DP_I2C_ADDR, 0x5fU, 0x06U);
    i2c_write_byte(DP_I2C_ADDR, 0xc9U, 0xf5U);
    i2c_write_byte(DP_I2C_ADDR, 0xcaU, 0x4cU);
    i2c_write_byte(DP_I2C_ADDR, 0xcbU, 0x37U);
    i2c_write_byte(DP_I2C_ADDR, 0xceU, (sPInfo.ucDpBR & 0x80U));
    i2c_write_byte(DP_I2C_ADDR, 0xd3U, 0x03U);
    i2c_write_byte(DP_I2C_ADDR, 0xd4U, 0x60U);
    i2c_write_byte(DP_I2C_ADDR, 0xe8U, 0x11U);
    i2c_write_byte(DP_I2C_ADDR, 0xecU, sPInfo.ucVic);
    (void)usleep(5000);
    i2c_write_byte(DP_I2C_ADDR, 0x62U, DP_BPP);
    i2c_write_byte(DP_I2C_ADDR, 0x23U, 0x42U);
    i2c_write_byte(DP_I2C_ADDR, 0x24U, 0x07U);
    i2c_write_byte(DP_I2C_ADDR, 0x25U, 0x01U);
    i2c_write_byte(DP_I2C_ADDR, 0x26U, 0x00U);
    i2c_write_byte(DP_I2C_ADDR, 0x27U, 0x10U);
    i2c_write_byte(DP_I2C_ADDR, 0x2BU, 0x05U);
    i2c_write_byte(DP_I2C_ADDR, 0x23U, 0x40U);
    i2c_write_byte(DP_I2C_ADDR, 0x22U, (DP_AUX_PN_SWAP << 3U) | (DP_PN_SWAP << 2U) | 0x03U);
    i2c_write_byte(DP_I2C_ADDR, 0x16U, (DPTX_SSC_SETTING << 4U) | (DP_LANE_SWAP << 3U) | (sPInfo.ucDpLanes << 1U) | (sPInfo.ucDpBR & 0x01U));
    i2c_write_byte(DP_I2C_ADDR, 0x0fU, 0x01U);
    i2c_write_byte(DP_I2C_ADDR, 0x76U, 0xa7U);
    i2c_write_byte(DP_I2C_ADDR, 0x77U, 0xafU);
    #if 0
    i2c_write_byte(DP_I2C_ADDR, 0x78U, 0x7cU); // added for LG 1080PRB flick test for TCL
    i2c_write_byte(DP_I2C_ADDR, 0x79U, 0x34U); // added for LG 1080PRB flick test for TCL
    #endif
    i2c_write_byte(DP_I2C_ADDR, 0x7eU, 0x8fU);
    i2c_write_byte(DP_I2C_ADDR, 0x7fU, 0x07U);
    i2c_write_byte(DP_I2C_ADDR, 0x80U, 0xefU);
    i2c_write_byte(DP_I2C_ADDR, 0x81U, 0x5fU);
    i2c_write_byte(DP_I2C_ADDR, 0x82U, 0xefU);
    i2c_write_byte(DP_I2C_ADDR, 0x83U, 0x07U);
    i2c_write_byte(DP_I2C_ADDR, 0x88U, 0x38U);
    i2c_write_byte(DP_I2C_ADDR, 0x89U, 0x1fU);
    i2c_write_byte(DP_I2C_ADDR, 0x8aU, 0x48U);
    i2c_write_byte(DP_I2C_ADDR, 0x0fU, 0x00U);
    i2c_write_byte(DP_I2C_ADDR, 0x5cU, 0xf3U);
    i2c_write_byte(DP_I2C_ADDR, 0x17U, 0x04U);
    i2c_write_byte(DP_I2C_ADDR, 0x17U, 0x01U);
    (void)usleep(5000);
    i2c_write_byte(DP_I2C_ADDR, 0xD3U, 0x03U);
    i2c_write_byte(DP_I2C_ADDR, 0x22U, 0xC3U);
}

static void IT6151_MIPI_Init(void)
{
#if 0
    (void)printf(" IT6151_MIPI_init !!!\n");

    i2c_write_byte(MIPI_I2C_ADDR,0x05U,0x33U);
#endif
    i2c_write_byte(MIPI_I2C_ADDR, 0x05U, 0x00U);
    i2c_write_byte(MIPI_I2C_ADDR, 0x0cU, (MP_LANE_SWAP << 7U) | (MP_PN_SWAP << 6U) | (sPInfo.ucMpLanes << 4U));
    i2c_write_byte(MIPI_I2C_ADDR, 0x11U, MP_MCLK_INV | 0x10U);
    i2c_write_byte(MIPI_I2C_ADDR, 0x19U, (MP_CONTINUOUS_CLK << 1U) | MP_LANE_DESKEW);
    i2c_write_byte(MIPI_I2C_ADDR, 0x4BU, (sPInfo.ucMpReSync & 0x18U));
    i2c_write_byte(MIPI_I2C_ADDR, 0x4CU, MIPI_FFRdStg & 0xFFU);
    i2c_write_byte(MIPI_I2C_ADDR, 0x4DU, (MIPI_FFRdStg >> 8U) & 0x01U);
    i2c_write_byte(MIPI_I2C_ADDR, 0x4EU, ((sPInfo.ucMpReSync & 0x03U) << 2U) | (sPInfo.ucMpVPol << 1U) | (sPInfo.ucMpHPol));
    i2c_write_byte(MIPI_I2C_ADDR, 0x4FU, 0x01U);
    i2c_write_byte(MIPI_I2C_ADDR, 0x72U, 0x01U);
    i2c_write_byte(MIPI_I2C_ADDR, 0x73U, 0x03U);
    i2c_write_byte(MIPI_I2C_ADDR, 0x80U, sPInfo.ucMpClkDiv);

    i2c_write_byte(MIPI_I2C_ADDR, 0xC0U, (sPInfo.ucHighPclk << 4U) | 0x07U);//0x1F);//0x13);
#if (AUTO_OUTPUT == 1)
    {
        i2c_write_byte(MIPI_I2C_ADDR, 0xC1U, 0x01U);
    }

#else
    i2c_write_byte(MIPI_I2C_ADDR, 0xC1U, 0x71U);
#endif

    i2c_write_byte(MIPI_I2C_ADDR, 0xC2U, REG_C2);//0x25); //0x47
    i2c_write_byte(MIPI_I2C_ADDR, 0xC3U, 0x67U);//0x37);//0x67);
    i2c_write_byte(MIPI_I2C_ADDR, 0xC4U, 0x03U);//0x03);//0x04);
    i2c_write_byte(MIPI_I2C_ADDR, 0xCBU, (LVDS_PN_SWAP << 5U) | (LVDS_LANE_SWAP << 4U) | (LVDS_6BIT << 2U) | (LVDS_DC_BALANCE << 1U) | VESA_MAP);
#if (MIPI_EVENT_MODE == 1)
    i2c_write_byte(MIPI_I2C_ADDR, 0x33U, 0x80U | MIPI_HSYNC_W >> 8U);
    i2c_write_byte(MIPI_I2C_ADDR, 0x32U, MIPI_HSYNC_W);
    i2c_write_byte(MIPI_I2C_ADDR, 0x3DU, 0x80U | MIPI_VSYNC_W >> 8U);
    i2c_write_byte(MIPI_I2C_ADDR, 0x3CU, MIPI_VSYNC_W);
#endif

    i2c_write_byte(MIPI_I2C_ADDR, 0x06U, 0xFFU);
    i2c_write_byte(MIPI_I2C_ADDR, 0x07U, 0xFFU);

    i2c_write_byte(MIPI_I2C_ADDR, 0x09U, sPInfo.ucIntMask);
    i2c_write_byte(MIPI_I2C_ADDR, 0x0AU, 0xC0U);

    i2c_write_byte(MIPI_I2C_ADDR, 0x92U, 0x14U);
    i2c_write_byte(MIPI_I2C_ADDR, 0x90U, 0x01U);

    i2c_write_byte(MIPI_I2C_ADDR, 0x07U, 0xFFU);
    i2c_write_byte(MIPI_I2C_ADDR, 0x08U, 0xFFU);
    i2c_write_byte(MIPI_I2C_ADDR, 0x0AU, 0x3FU);
    i2c_write_byte(MIPI_I2C_ADDR, 0x0BU, 0x7FU);

    i2c_write_byte(MIPI_I2C_ADDR, 0x05U, 0x32U);
    i2c_write_byte(MIPI_I2C_ADDR, 0x05U, 0x30U);
    i2c_write_byte(MIPI_I2C_ADDR, 0x05U, 0x00U);
}

static int IT6151_init(void)
{
    uint8_t VenID[2], DevID[2], RevID;

    i2c_read_byte(DP_I2C_ADDR, 0x00U, &VenID[0]);
    i2c_read_byte(DP_I2C_ADDR, 0x01U, &VenID[1]);
    i2c_read_byte(DP_I2C_ADDR, 0x02U, &DevID[0]);
    i2c_read_byte(DP_I2C_ADDR, 0x03U, &DevID[1]);
    i2c_read_byte(DP_I2C_ADDR, 0x04U, &RevID);

    #if 0
    (void)printf("Current DPDevID=%02X%02X\n", DevID[1], DevID[0]);
    (void)printf("Current DPVenID=%02X%02X\n", VenID[1], VenID[0]);
    (void)printf("Current DPRevID=%02X\n\n", RevID);
    (void)printf(" IT6151_init !!!\n");
    #endif

    if ((VenID[0] == 0x54U) && (VenID[1] == 0x49U) && (DevID[0] == 0x51U) &&
        (DevID[1] == 0x61U))
    {
    #if 0
        (void)printf(" Test 1 DP_I2C_ADDR=0x%x, MIPI_I2C_ADDR=0x%x\n", DP_I2C_ADDR, MIPI_I2C_ADDR);
    #endif

        i2c_write_byte(DP_I2C_ADDR, 0x05U, 0x04U); // DP SW Reset
        i2c_write_byte(DP_I2C_ADDR, 0xfdU, (MIPI_I2C_ADDR << 1U) | 1U);
        i2c_write_byte(MIPI_I2C_ADDR, 0x05U, 0x00U);
        i2c_write_byte(MIPI_I2C_ADDR, 0x0cU,
                       (MP_LANE_SWAP << 7U) | (MP_PN_SWAP << 6U) | (sPInfo.ucMpLanes << 4U) | sPInfo.ucUFO);
        i2c_write_byte(MIPI_I2C_ADDR, 0x11U, MP_MCLK_INV);
        if (RevID == 0xA1U)
        {
            i2c_write_byte(MIPI_I2C_ADDR, 0x19U, MP_LANE_DESKEW);
        }
        else
        {
            i2c_write_byte(MIPI_I2C_ADDR, 0x19U,
                           (MP_CONTINUOUS_CLK << 1U) | MP_LANE_DESKEW);
        }

        i2c_write_byte(MIPI_I2C_ADDR, 0x27U, sPInfo.ucMpFmt);
        i2c_write_byte(MIPI_I2C_ADDR, 0x28U, (uint8_t)((((sPInfo.usPWidth / 4U) - 1U) >> 2U) & 0xC0U));
        i2c_write_byte(MIPI_I2C_ADDR, 0x29U, (uint8_t)(((sPInfo.usPWidth / 4U) - 1U) & 0xFFU));

        i2c_write_byte(MIPI_I2C_ADDR, 0x2eU, 0x34U);
        i2c_write_byte(MIPI_I2C_ADDR, 0x2fU, 0x01U);

        i2c_write_byte(MIPI_I2C_ADDR, 0x4eU, ((sPInfo.ucDpReSync & 0x03U) << 2U) | (sPInfo.ucMpVPol << 1U) | (sPInfo.ucMpHPol));
        i2c_write_byte(MIPI_I2C_ADDR, 0x80U, (sPInfo.ucUFO << 5U) | sPInfo.ucMpClkDiv);

        i2c_write_byte(MIPI_I2C_ADDR, 0x09U, MIPI_RECOVER);
        i2c_write_byte(MIPI_I2C_ADDR, 0x92U, TIMER_CNT);
#if (MIPI_EVENT_MODE == 1)
        i2c_write_byte(MIPI_I2C_ADDR, 0x33U, 0x80U | MIPI_HSYNC_W >> 8U);
        i2c_write_byte(MIPI_I2C_ADDR, 0x32U, MIPI_HSYNC_W & 0xFFU);
        i2c_write_byte(MIPI_I2C_ADDR, 0x3DU, 0x80U | MIPI_VSYNC_W >> 8U);
        i2c_write_byte(MIPI_I2C_ADDR, 0x3CU, MIPI_VSYNC_W & 0xFFU);
#endif
        i2c_write_byte(MIPI_I2C_ADDR, 0x18U, 0x20U | sPInfo.ucHSSetNum); //SkipStg 0x30

        IT6151_DPTX_init();

#if 0
        i2c_write_byte(DP_I2C_ADDR, 0x96, 0xD1);
        it6151_i2c_write_byte(DP_I2C_ADDR, 0x96, 0x13);   //For Test:20220111
#endif
        return 0;
    }

#if 0
    (void)printf(" Test 2 DP_I2C_ADDR=0x%x, MIPI_I2C_ADDR=0x%x\n", DP_I2C_ADDR, MIPI_I2C_ADDR);
#endif

    i2c_read_byte(MIPI_I2C_ADDR, 0x00U, &VenID[0]);
    i2c_read_byte(MIPI_I2C_ADDR, 0x01U, &VenID[1]);
    i2c_read_byte(MIPI_I2C_ADDR, 0x02U, &DevID[0]);
    i2c_read_byte(MIPI_I2C_ADDR, 0x03U, &DevID[1]);
    i2c_read_byte(MIPI_I2C_ADDR, 0x04U, &RevID);

#if 0
    (void)printf("Current MPDevID=%02X%02X\n", DevID[1], DevID[0]);
    (void)printf("Current MPVenID=%02X%02X\n", VenID[1], VenID[0]);
    (void)printf("Current MPRevID=%02X\n\n", RevID);
#endif

    if ((VenID[0] == 0x54U) && (VenID[1] == 0x49U) && (DevID[0] == 0x21U) &&
        (DevID[1] == 0x61U))
    {
        IT6151_MIPI_Init();
        return 1;
    }
    return -1;
}

static void IT6151_Debug (void)
{
    static bool frist;
    uint8_t Reg;
    uint32_t      i = 0U;

    i2c_read_byte(MIPI_I2C_ADDR, 0xDU, &Reg);
    if (Reg == 0x31U && frist)
    {
        return;
    }
    frist = true;
#if 0
    (void)printf("--------------Read MIPI RX--------------------\n");
#endif
    for (i = 0U; i < 0xFFU; i++)
    {
        i2c_read_byte(DP_I2C_ADDR, i, &Reg);
#if 0
        (void)printf("Read DP TX read_data[0x%X] = 0x%X\n", i, Reg);
#endif
    }
    for (i = 0U; i < 0xFFU; i++)
    {
        i2c_read_byte(MIPI_I2C_ADDR, i, &Reg);
#if 0
        (void)printf("Read MIPI RX read_data[0x%X] = 0x%X\n", i, Reg);
#endif
    }
}

#if 0
static int IT6151_ESD_Check (void)
{
    static uint8_t ucIsIT6151 = 0xFFU;
    uint8_t        ucReg, ucStat;

    if (ucIsIT6151 == 0xFFU)
    {
        uint8_t VenID[2], DevID[2];

        #if 0
        (void)printf("\nIT6151 1st IRQ !!!\n");
        #endif
        i2c_read_byte(DP_I2C_ADDR, 0x00, &VenID[0]);
        i2c_read_byte(DP_I2C_ADDR, 0x01, &VenID[1]);
        i2c_read_byte(DP_I2C_ADDR, 0x02, &DevID[0]);
        i2c_read_byte(DP_I2C_ADDR, 0x03, &DevID[1]);

        #if 0
        (void)printf("Current DevID=%02X%02X\n", (int)DevID[1], (int)DevID[0]);
        (void)printf("Current VenID=%02X%02X\n", (int)VenID[1], (int)VenID[0]);
        #endif

        if (VenID[0] == 0x54 && VenID[1] == 0x49 && DevID[0] == 0x51 && DevID[1] == 0x61)
        {
            ucIsIT6151 = 1;
        }
        else
        {
            ucIsIT6151 = 0;
        }
    }

    if (ucIsIT6151 == 1)
    {
        i2c_read_byte(DP_I2C_ADDR, 0x05, &ucReg);
        if (ucReg & 0x01)
        {
            return 1;
        }
        i2c_read_byte(DP_I2C_ADDR, 0x0D, &ucReg);

        if (ucReg & 0x89)
        {
            (void)printf("\nIT6151 Reg0x0D=0x%x !!!\n", ucReg);
            i2c_read_byte(DP_I2C_ADDR, 0x06, &ucReg);
            (void)printf("\nIT6151 DReg0x06=0x%x !!!\n", ucReg);
            i2c_read_byte(DP_I2C_ADDR, 0x07, &ucReg);
            (void)printf("\nIT6151 DReg0x07=0x%x !!!\n", ucReg);
            i2c_read_byte(DP_I2C_ADDR, 0x08, &ucReg);
            (void)printf("\nIT6151 DReg0x08=0x%x !!!\n", ucReg);

            i2c_read_byte(DP_I2C_ADDR, 0xb6, &ucReg);
            (void)printf("\nIT6151 DReg0xb6=0x%x !!!\n", ucReg);
            i2c_read_byte(DP_I2C_ADDR, 0xc4, &ucReg);
            (void)printf("\nIT6151 DReg0xc4=0x%x !!!\n", ucReg);
            i2c_read_byte(DP_I2C_ADDR, 0xb7, &ucReg);
            (void)printf("\nIT6151 DReg0xb7=0x%x !!!\n", ucReg);
            i2c_read_byte(DP_I2C_ADDR, 0x21, &ucReg);
            (void)printf("\nIT6151 DReg0x21=0x%x !!!\n", ucReg);
        }

        i2c_read_byte(DP_I2C_ADDR, 0x0D, &ucReg);
        #if 0
        (void)printf("\nIT6151 Reg0x0D=0x%x !!!\n", ucReg);
        #endif

        if (ucReg & 0x80)
        {
            // MIPI_IRQ
            i2c_read_byte(MIPI_I2C_ADDR, 0x06, &ucStat);
            if (ucStat & 0x01)
            {
                i2c_write_byte(MIPI_I2C_ADDR, 0x06, ucStat);

                i2c_read_byte(MIPI_I2C_ADDR, 0x0D, &ucStat);
                if (ucStat & 0x10)
                {
                    // disable timer
                    i2c_write_byte(MIPI_I2C_ADDR, 0x0B, 0x00);
                    i2c_write_byte(MIPI_I2C_ADDR, 0x08, 0x40);
                }
                else
                {
                    // enable timer
                    i2c_write_byte(MIPI_I2C_ADDR, 0x0B, 0x40);
                }
            }
            i2c_read_byte(MIPI_I2C_ADDR, 0x08, &ucStat);
            if (ucStat & 0x40)
            {
                if (ucStat & 0x20)
                {
                    // disable timer
                    i2c_write_byte(MIPI_I2C_ADDR, 0x0B, 0x00);
                    i2c_write_byte(MIPI_I2C_ADDR, 0x08, 0x40);
                }
                else
                {
                    return 1;
                }
            }
        }

        if (ucReg & 0x01)
        {   // DP_IRQ
            i2c_read_byte(DP_I2C_ADDR, 0x21, &ucStat);
            if (ucStat & 0x02)
            {
                i2c_write_byte(DP_I2C_ADDR, 0x21, ucStat);
            }
            i2c_read_byte(DP_I2C_ADDR, 0x06, &ucReg);
            i2c_read_byte(DP_I2C_ADDR, 0x0D, &ucStat);
            if (ucReg & 0x03)
            {
                if (ucStat & 0x02)
                {
                    return 1;
                }
            }
        }
    }
    else if (ucIsIT6151 == 0)
    {
        i2c_read_byte(MIPI_I2C_ADDR, 0x05, &ucReg);
        if (ucReg & 0x33)
        {
            IT6151_MIPI_Init();
        }
        else
        {
            i2c_read_byte(MIPI_I2C_ADDR, 0x06, &ucReg);
            if ((ucReg & 0x11) == 0x11)
            {
        #if 0
                (void)printf("\nIT6151 Reg0x06=0x%x !!!\n", ucReg);
        #endif
                i2c_write_byte(MIPI_I2C_ADDR, 0x06, ucReg);
                i2c_read_byte(MIPI_I2C_ADDR, 0x0D, &ucReg);
                if ((ucReg & 0x38) != 0x30)
                {
        #if 1
                    IT6151_MIPI_Init();
        #else
                    i2c_read_byte(MIPI_I2C_ADDR, 0x05, &ucReg);
                    i2c_write_byte(MIPI_I2C_ADDR, 0x05, ucReg | 0x20);
                    i2c_read_byte(MIPI_I2C_ADDR, 0x37, &ucReg);
                    i2c_write_byte(MIPI_I2C_ADDR, 0x37, ucReg & 0x7F);
        #endif
                }
                else
                {
                    unsigned short usTemp;

                    i2c_read_byte(MIPI_I2C_ADDR, 0x04, &ucReg);
                    if (ucReg == 0xA0)
                    {
                        i2c_read_byte(MIPI_I2C_ADDR, 0x37, &ucReg);
                        if (ucReg & 0x80)
                        {
                            i2c_write_byte(MIPI_I2C_ADDR, 0x37, ucReg & 0x7F);
                            i2c_read_byte(MIPI_I2C_ADDR, 0x37, &ucReg);
                        }

                        usTemp = ucReg;
                        usTemp <<= 8;
                        i2c_read_byte(MIPI_I2C_ADDR, 0x36, &ucReg);
                        usTemp += ucReg;
                        usTemp++;

                        i2c_write_byte(MIPI_I2C_ADDR, 0x36, usTemp & 0xFF);
                        i2c_write_byte(MIPI_I2C_ADDR, 0x37, 0x80 | (usTemp >> 8));
                    }
                    i2c_read_byte(MIPI_I2C_ADDR, 0x05, &ucReg);
                    i2c_write_byte(MIPI_I2C_ADDR, 0x05, ucReg & 0xDF);
                }
            }
        }
    }
    return 0;
}

static void IT6151_ESD_Recover(void)
{
    uint8_t ucStat;

    i2c_read_byte(MIPI_I2C_ADDR, 0x08, &ucStat);
    if (ucStat & 0x40){
        i2c_write_byte(MIPI_I2C_ADDR, 0x0B, 0x00);
        i2c_write_byte(MIPI_I2C_ADDR, 0x08, 0x40);
        IT6151_init();
    }
    else{
        IT6151_DPTX_init();
    }
}
#endif

static void * IT6151_check_Task (void * arg)
{
    for (;;)
    {
        uint8_t Reg, RxReg;
#if 0
        int result = 0;
#endif

        {
            i2c_read_byte(DP_I2C_ADDR, 0xDU, &Reg);
            i2c_read_byte(MIPI_I2C_ADDR, 0xDU, &RxReg);

#if 0
            result = IT6151_ESD_Check();
#endif
            if ((((Reg & 0x89U) != 0U) && ((RxReg & 0x30U) != 0x30U)) || ((RxReg & 0x8U) != 0U) || (RxReg == 0U))
            {
                #if 0
                (void)printf("main Read DP TX read_data[0xD] = 0x%X\n", Reg);
                (void)printf("main Read MIPI RX read_data[0xD] = 0x%X\n", RxReg);
                #endif
                if (errorSum > 0)
                {

                    uint32_t lcdBaseA = ithLcdGetBaseAddrA();
                    uint32_t lcdBaseB = ithLcdGetBaseAddrB();
    #if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
                    uint32_t lcdBaseC = ithLcdGetBaseAddrC();
    #endif
    #if 0
                    (void)printf("main+++++++++++++++++++++++++++++++result:%d\n", result);
                     IT6151_Debug();
    #endif

                    ithCmdQLock(ITH_CMDQ0_OFFSET);
                    // wait cmdq and 2D idle
                    while (true)
                    {
                        if (0U != (ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_SR1_REG + ITH_CMDQ0_OFFSET) &
                            (0x1UL << ITH_CMDQ_ALLIDLE_BIT)))
                        {
                            break;
                        }

                        (void)usleep(100);
                    }

                    while (0U != (ithReadRegA(0xB07000C4U) & (1U << 0U)))
                    {
                        (void)usleep(100);
                    }

                    ithLcdReset();

                    ioctl(ITP_DEVICE_SCREEN, ITP_IOCTL_RESET, NULL);

                    (void)usleep(1000);
                    ithLcdEnableHwFlip();
                    (void)usleep(1000);
    #if 0
                    (void)memset((void*)&__lcd_base_a, 0, CFG_LCD_PITCH * CFG_LCD_HEIGHT);
                    (void)memset((void*)&__lcd_base_b, 0, CFG_LCD_PITCH * CFG_LCD_HEIGHT);
                    (void)memset((void*)&__lcd_base_c, 0, CFG_LCD_PITCH * CFG_LCD_HEIGHT);
    #endif

                    ithLcdSetBaseAddrA(lcdBaseA);
                    ithLcdSetBaseAddrB(lcdBaseB);
    #if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
                    ithLcdSetBaseAddrC(lcdBaseC);
    #endif
                    ioctl(ITP_DEVICE_SCREEN, ITP_IOCTL_POST_RESET, NULL);

                    (void)usleep(200000);
    #if 0
                    IT6151_ESD_Recover();
    #endif
                    IT6151_init();

                    ithCmdQUnlock(ITH_CMDQ0_OFFSET);

                    errorSum = 0;
                }
                else
                {
                    errorSum++;
                }
            }
        }
        (void)usleep(1000000);
    }

    return NULL;
}
#endif

#if defined(GM8775)

#define GM8775C (0x5AU >> 1U)
static void GM8775C_Sys_ini(void)
{
    //HS:44, HBP:108, HFP:148, VS:5, VBP:36, VFP:4
    i2c_write_byte(GM8775C, 0x00U, 0xAAU); //PASS WORD
    i2c_write_byte(GM8775C, 0x48U, 0x02U);
    i2c_write_byte(GM8775C, 0xB6U, 0x20U); //Enable HPD_IRQ,HPD_CHG,VIDSTABLE
    i2c_write_byte(GM8775C, 0x01U, 0x80U); //HACTIVE[7:0] 1920 = 0X780
    i2c_write_byte(GM8775C, 0x02U, 0xD0U); //VACTIVE[7:0] 720 = 0X2D0
    i2c_write_byte(GM8775C, 0x03U, 0x27U); //VACTIVE[11:8] HACTIVE[11:8]
    i2c_write_byte(GM8775C, 0x04U, 0x58U); //HFP[7:0]
    i2c_write_byte(GM8775C, 0x05U, 0x2CU); //HSW_WIDTH
    i2c_write_byte(GM8775C, 0x06U, 0x94U); //HBP_WIDTH[7:0]
    i2c_write_byte(GM8775C, 0x07U, 0x00U); //{VS_POL,HS_POL,HFP_WIDTH[9:8],HSW_WIDTH[9:8],HBP_WIDTH[9:8]]
    i2c_write_byte(GM8775C, 0x08U, 0x04U); //VFP_WIDTH[7:0]
    i2c_write_byte(GM8775C, 0x09U, 0x05U); //VSW
    i2c_write_byte(GM8775C, 0x0AU, 0x24U); //VBP
    i2c_write_byte(GM8775C, 0x0BU, 0x82U); //PLL
    i2c_write_byte(GM8775C, 0x0CU, 0x13U); //PLL INT//MIPI CLK 540(0x11)-600M(0x10),480M(0x13)
    i2c_write_byte(GM8775C, 0x0DU, 0x01U);
    i2c_write_byte(GM8775C, 0x0EU, 0x80U);
    i2c_write_byte(GM8775C, 0x0FU, 0x20U); //MIN HSW
    i2c_write_byte(GM8775C, 0x10U, 0x20U); //MIN HFP
    i2c_write_byte(GM8775C, 0x11U, 0x03U);
    i2c_write_byte(GM8775C, 0x12U, 0x1BU);
    i2c_write_byte(GM8775C, 0x13U, 0x53U); //63
    i2c_write_byte(GM8775C, 0x14U, 0x01U);
    i2c_write_byte(GM8775C, 0x15U, 0x23U);
    i2c_write_byte(GM8775C, 0x16U, 0x40U);
    i2c_write_byte(GM8775C, 0x17U, 0x1FU);
    i2c_write_byte(GM8775C, 0x18U, 0x01U);
    i2c_write_byte(GM8775C, 0x19U, 0x23U);
    i2c_write_byte(GM8775C, 0x1AU, 0x40U);
    i2c_write_byte(GM8775C, 0x1BU, 0x1FU);
    i2c_write_byte(GM8775C, 0x1EU, 0x46U);

    i2c_write_byte(GM8775C, 0x51U, 0x30U);
    i2c_write_byte(GM8775C, 0x1FU, 0x10U);
    i2c_write_byte(GM8775C, 0x2AU, 0x01U); //0x4D test mode
}

static void GM8775C_DumpReg(void)
{
    uint8_t i = 0U;
    uint8_t value;

    (void)ithPrintf("--------- GM8775C ---------\r\n");
    (void)ithPrintf("HMPRX_R Reg\r\n");
    for (i = 0U; i < 0xFFU; i++)
    {

        if ((i % 0x10U) == 0x00U)
        {
            (void)printf("\r\n%02x|\t", i);
        }
        i2c_read_byte(GM8775C, i, &value);
        (void)ithPrintf("%02x ", value);
    }
#if 0
    i2c_read_byte(GM8775C, 0xFF, &value);
#endif
    (void)ithPrintf("%02x\r\n", value);
}
#endif

#if defined(IT6122)
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//for IT6122 API
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ithLcdInitIT6122(void)
{
    //IT6122 MIPI to LVDS
    return IT6122_Sys_ini();
}

void ithLcdDumpRegIT6122(void)
{
    IT6122_DumpReg();
}

void ithLcdIT6122_ShowMParam(void)
{
    uint8_t  u8Temp;
    uint16_t u16MHSW, u16MHFP, u16MHBP, u16MHDEW, u16MHVR2nd, u16MHBlank, u16MVSW, u16MVFP, u16MVBP, u16MVDEW, u16MVFP2nd, u16MVTotal;

    i2c_read_byte(MPRxDevAddr, 0x52U, &u8Temp);
    u16MHSW = u8Temp;
    i2c_read_byte(MPRxDevAddr, 0x53U, &u8Temp);
    u16MHSW += (u8Temp & 0x3FU) << 8U;

    i2c_read_byte(MPRxDevAddr, 0x50U, &u8Temp);
    u16MHFP = u8Temp;
    i2c_read_byte(MPRxDevAddr, 0x51U, &u8Temp);
    u16MHFP += (((uint16_t)u8Temp & 0x3FU)) << 8U;


    i2c_read_byte(MPRxDevAddr, 0x54U, &u8Temp);
    u16MHBP = u8Temp;
    i2c_read_byte(MPRxDevAddr, 0x55U, &u8Temp);
    u16MHBP += (((uint16_t)u8Temp & 0x3FU)) << 8U;

    i2c_read_byte(MPRxDevAddr, 0x56U, &u8Temp);
    u16MHDEW = u8Temp;
    i2c_read_byte(MPRxDevAddr, 0x57U, &u8Temp);
    u16MHDEW += (((uint16_t)u8Temp & 0x3FU)) << 8U;


    i2c_read_byte(MPRxDevAddr, 0x58U, &u8Temp);
    u16MHVR2nd = u8Temp;
    i2c_read_byte(MPRxDevAddr, 0x59U, &u8Temp);
    u16MHVR2nd += (((uint16_t)u8Temp & 0x3FU)) << 8U;

    u16MHBlank = u16MHFP + u16MHSW + u16MHBP;

    i2c_read_byte(MPRxDevAddr, 0x5CU, &u8Temp);
    u16MVSW = u8Temp;
    i2c_read_byte(MPRxDevAddr, 0x5DU, &u8Temp);
    u16MVSW += (((uint16_t)u8Temp & 0x3FU)) << 8U;

    i2c_read_byte(MPRxDevAddr, 0x5AU, &u8Temp);
    u16MVFP = u8Temp;
    i2c_read_byte(MPRxDevAddr, 0x5BU, &u8Temp);
    u16MVFP += (((uint16_t)u8Temp & 0x3FU)) << 8U;

    i2c_read_byte(MPRxDevAddr, 0x5EU, &u8Temp);
    u16MVBP = u8Temp;
    i2c_read_byte(MPRxDevAddr, 0x5FU, &u8Temp);
    u16MVBP += (((uint16_t)u8Temp & 0x3FU)) << 8U;

    i2c_read_byte(MPRxDevAddr, 0x60U, &u8Temp);
    u16MVDEW = u8Temp;
    i2c_read_byte(MPRxDevAddr, 0x61U, &u8Temp);
    u16MVDEW += (((uint16_t)u8Temp & 0x3FU)) << 8U;


    i2c_read_byte(MPRxDevAddr, 0x62U, &u8Temp);
    u16MVFP2nd = u8Temp;
    i2c_read_byte(MPRxDevAddr, 0x63U, &u8Temp);
    u16MVFP2nd += (((uint16_t)u8Temp & 0x3FU)) << 8U;

    u16MVTotal = u16MVFP + u16MVSW + u16MVBP + u16MVDEW;

    (void)ithPrintf("u16MHFP    = %d\r\n ", u16MHFP);
    (void)ithPrintf("u16MHSW    = %d\r\n ", u16MHSW);
    (void)ithPrintf("u16MHBP    = %d\r\n ", u16MHBP);
    (void)ithPrintf("u16MHDEW   = %d\r\n ", u16MHDEW);
    (void)ithPrintf("u16MHVR2nd = %d\r\n ", u16MHVR2nd);
    (void)ithPrintf("u16MHBlank  = %d\r\n ", u16MHBlank);

    (void)ithPrintf("u16MVFP    = %d\r\n ", u16MVFP);
    (void)ithPrintf("u16MVSW    = %d\r\n ", u16MVSW);
    (void)ithPrintf("u16MVBP   = %d\r\n ", u16MVBP);
    (void)ithPrintf("u16MVDEW   = %d\r\n ", u16MVDEW);
    (void)ithPrintf("u16MVFP2nd   = %d\r\n ", u16MVFP2nd);
    (void)ithPrintf("u16MVTotal = %d\r\n ", u16MVTotal);


    (void)ithPrintf("\r\n ");
}

void ithLcdInitCheckTaskIT6122(void)
{
    pthread_t       task;
    pthread_create(&task, NULL, &IT6122_check_Task, NULL);
}

void ithLcdCheckIT6122Interrupt(void)
{
    #if 0
    static uint16_t gu16LVHTotal = 0U;
    #endif

    uint32_t timeout;

    timeout = 500U; //1s

    do
    {
    #if 0
        (void)printf("ithLcdCheckIT6122Interrupt\n");
    #endif
        uint8_t u8RegF7;

        i2c_read_byte(MPRxDevAddr, 0xF7U, &u8RegF7);
        i2c_write_byte(MPRxDevAddr, 0xF7U, u8RegF7);

        if (0U != (u8RegF7 & 0x02U))
        {
    #if 0
            uint8_t value, value1;
            uint16_t u16LVHTotal;
    #endif

            (void)printf("LVDSTx VidRec parameters change Interrupt !!!\n");
    #if 0
            (void)printf("MIPI error status 0x38:0x%x !!\n", ithReadRegA(0xd0c00038U));
            (void)printf("LCD error status 0x160:0x%x !!\n", ithReadRegA(0xd0000160U));
    #endif
            (void)printf("interrupt_count = %d\n", interrupt_count);
            (void)printf("[Timestamp III]\n");

            if (interrupt_count > 5)
            {
                uint32_t lcdBaseA = ithLcdGetBaseAddrA();
                uint32_t lcdBaseB = ithLcdGetBaseAddrB();
#if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
                uint32_t lcdBaseC = ithLcdGetBaseAddrC();
#endif
                (void)printf("IT6122 Parameter Change Reset LCD Start+++++++++++++++++++++++++++++++\n");
                (void)printf("[Timestamp V]\n");

                //wait cmdq and 2D idle
                ithCmdQLock(ITH_CMDQ0_OFFSET);
                while (true)
                {
                    if (0U != (ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_SR1_REG + ITH_CMDQ0_OFFSET) & (0x1UL << ITH_CMDQ_ALLIDLE_BIT)))
                    {
                        break;
                    }

                    (void)usleep(100);
                }

                while (0U != (ithReadRegA(0xB07000C4U) & (1U << 0U)))
                {
                    (void)usleep(100);
                }

                ithLcdReset();

                ioctl(ITP_DEVICE_SCREEN, ITP_IOCTL_RESET, NULL);

                (void)usleep(1000);
                ithLcdEnableHwFlip();
                (void)usleep(1000);

                ithLcdSetBaseAddrA(lcdBaseA);
                ithLcdSetBaseAddrB(lcdBaseB);
#if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
                ithLcdSetBaseAddrC(lcdBaseC);
#endif
                ioctl(ITP_DEVICE_SCREEN, ITP_IOCTL_POST_RESET, NULL);

                ithCmdQUnlock(ITH_CMDQ0_OFFSET);
                interrupt_count = 0;
                timeout= 500U; //re-check
            }
            else
            {
                interrupt_count++;
            }
        }

    } while (--timeout != 0U);

    if (timeout == 0U)
    {
        (void)printf("ithLcdCheckIT6122Interrupt timeout+++++++++++++++++++++++++++++++\n");
    }
}
#endif

#if defined(IT6151)
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//for IT6151 API
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ithLcdInitIT6151(void)
{
    //IT6151 MIPI to EDP
    IT6151_init();
}

void ithLcdDumpRegIT6151(void)
{
    IT6151_Debug();
}

void ithLcdInitCheckTaskIT6151(void)
{
    pthread_t       task;
    pthread_create(&task, NULL, &IT6151_check_Task, NULL);
}
#endif

#if defined(GM8775)
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//for GM8775 API
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ithLcdInitGM8775C(void)
{
    (void)printf("%s\n", __FUNCTION__);
    GM8775C_Sys_ini();
}

void ithLcdDumpRegGM8775C(void)
{
    #if 0
    (void)printf("%s\n", __FUNCTION__);
    #endif
    GM8775C_DumpReg();
}
#endif
