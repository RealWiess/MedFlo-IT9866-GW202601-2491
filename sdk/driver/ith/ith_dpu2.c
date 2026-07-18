/*
 * Copyright (c) 2012 ITE Tech. Inc. All Rights Reserved.
 */
#include <stdio.h>
#include "ite/ith.h"
#include "ite/itp.h"
#include "ith_cfg.h"

//=============================================================================
//                              MACRO definition
//=============================================================================
#define MAX_DPU_MODE_NUM   (20)

#define DPU_INTR_ENABLE         0x00000001
#define DPU_INTR_DISABLE        0x00000000
#define DPU_INTR_ENABLE_MASK    0x00000001

#define DPU_INTR_ENCLEAR       0x00000002
#define DPU_INTR_DISCLEAR      0x00000000
#define DPU_INTR_CLEAR_MASK    0x00000002

#define DPU_SET_ENCODE                0x00000080
#define DPU_SET_DECODE                0x00000000
#define DPU_SET_ENCODE_DECODE_MASK    0x00000080

#define DPU_FIRE_BIT    0x00000002

#define DPU_SET_ENDIAN_MASK    0x0030000C

#define DPU_BURST_WIDTH_32_BIT       0x20000000
#define DPU_BURST_SIZE_8             0x02000000
#define DPU_SET_RAND_KEY_FREERUN     0x00000800
#define CRC_PROC_DATA_FROM_DATAIN    0x00010000
#define DPU_SET_LITTLE_ENDIAN        0x00000000
#define DPU_SET_VECTOR_SRC1          0x00004000
#define DPU_SET_KEY_SRC1             0x00001000
#define DPU_SET_AES_MODE             0x00000010
#define DPU_SET_ECB_MODE             0x00000000
#define DPU_SET_CBC_MODE             0x00000100
#define DPU_SET_OFB_MODE             0x00000200
#define DPU_SET_CFB_MODE             0x00000300
#define DPU_SET_CTR_MODE             0x00000400

#define AES_ECB_CTRL_INIT_VALUE    (DPU_BURST_WIDTH_32_BIT | DPU_BURST_SIZE_8 |\
DPU_SET_RAND_KEY_FREERUN | CRC_PROC_DATA_FROM_DATAIN | DPU_SET_LITTLE_ENDIAN |\
DPU_SET_VECTOR_SRC1 | DPU_SET_KEY_SRC1 | DPU_SET_AES_MODE | DPU_SET_ECB_MODE)
#define AES_CBC_CTRL_INIT_VALUE    (DPU_BURST_WIDTH_32_BIT | DPU_BURST_SIZE_8 |\
DPU_SET_RAND_KEY_FREERUN | CRC_PROC_DATA_FROM_DATAIN | DPU_SET_LITTLE_ENDIAN |\
DPU_SET_VECTOR_SRC1 | DPU_SET_KEY_SRC1 | DPU_SET_AES_MODE | DPU_SET_CBC_MODE)
#define AES_OFB_CTRL_INIT_VALUE    (DPU_BURST_WIDTH_32_BIT | DPU_BURST_SIZE_8 |\
DPU_SET_RAND_KEY_FREERUN | CRC_PROC_DATA_FROM_DATAIN | DPU_SET_LITTLE_ENDIAN |\
DPU_SET_VECTOR_SRC1 | DPU_SET_KEY_SRC1 | DPU_SET_AES_MODE | DPU_SET_OFB_MODE)
#define AES_CFB_CTRL_INIT_VALUE    (DPU_BURST_WIDTH_32_BIT | DPU_BURST_SIZE_8 |\
DPU_SET_RAND_KEY_FREERUN | CRC_PROC_DATA_FROM_DATAIN | DPU_SET_LITTLE_ENDIAN |\
DPU_SET_VECTOR_SRC1 | DPU_SET_KEY_SRC1 | DPU_SET_AES_MODE | DPU_SET_CFB_MODE)
#define AES_CTR_CTRL_INIT_VALUE    (DPU_BURST_WIDTH_32_BIT | DPU_BURST_SIZE_8 |\
DPU_SET_RAND_KEY_FREERUN | CRC_PROC_DATA_FROM_DATAIN | DPU_SET_LITTLE_ENDIAN |\
DPU_SET_VECTOR_SRC1 | DPU_SET_KEY_SRC1 | DPU_SET_AES_MODE | DPU_SET_CTR_MODE)

#define ITH_DPU_REG_DCR        (ITH_DPU_BASE + 0x00)
#define ITH_DPU_REG_SAR        (ITH_DPU_BASE + 0x04)
#define ITH_DPU_REG_DAR        (ITH_DPU_BASE + 0x08)
#define ITH_DPU_REG_SSR        (ITH_DPU_BASE + 0x0C)
#define ITH_DPU_REG_INTCT      (ITH_DPU_BASE + 0x10)
#define ITH_DPU_REG_DSR        (ITH_DPU_BASE + 0x14)
#define ITH_DPU_REG_CRC0       (ITH_DPU_BASE + 0x18)
#define ITH_DPU_REG_CRC1       (ITH_DPU_BASE + 0x1C)
#define ITH_DPU_REG_CRCSD      (ITH_DPU_BASE + 0x20)
#define ITH_DPU_REG_RANKEY     (ITH_DPU_BASE + 0x24)
#define ITH_DPU_REG_RANKS      (ITH_DPU_BASE + 0x28)
#define ITH_DPU_REG_KEY0       (ITH_DPU_BASE + 0x2C)
#define ITH_DPU_REG_KEY1       (ITH_DPU_BASE + 0x30)
#define ITH_DPU_REG_KEY2       (ITH_DPU_BASE + 0x34)
#define ITH_DPU_REG_KEY3       (ITH_DPU_BASE + 0x38)
#define ITH_DPU_REG_KEY4       (ITH_DPU_BASE + 0x3C)
#define ITH_DPU_REG_KEY5       (ITH_DPU_BASE + 0x40)
#define ITH_DPU_REG_VECTOR0    (ITH_DPU_BASE + 0x44)
#define ITH_DPU_REG_VECTOR1    (ITH_DPU_BASE + 0x48)
#define ITH_DPU_REG_VECTOR2    (ITH_DPU_BASE + 0x4C)
#define ITH_DPU_REG_VECTOR3    (ITH_DPU_BASE + 0x50)

#define DPU_DPCLK_EN     17
#define DPU_M14CLK_EN    19
#define DPU_W19CLK_EN    21

#define ITH_DPU_CLOCK    (ITH_HOST_BASE + 0x2C)

//=============================================================================
//                              Global definition
//=============================================================================

static uint32_t DpuDsrInitArray[MAX_DPU_MODE_NUM] =
{
    AES_ECB_CTRL_INIT_VALUE,
    AES_CBC_CTRL_INIT_VALUE,
    AES_OFB_CTRL_INIT_VALUE,
    AES_CFB_CTRL_INIT_VALUE,
    AES_CTR_CTRL_INIT_VALUE
};

static uint32_t DpuRegs[(ITH_DPU_REG_VECTOR3 - ITH_DPU_REG_DCR + 4) / 4];

//=============================================================================
//                              Function definition
//=============================================================================
void ithDpuClearCtrl(void)
{
    ithWriteRegA(ITH_DPU_REG_DCR, 0x0);
}

void ithDpuClearIntr(void)
{
    ithWriteRegMaskA(ITH_DPU_REG_INTCT, DPU_INTR_ENCLEAR, DPU_INTR_CLEAR_MASK);
    ithWriteRegMaskA(ITH_DPU_REG_INTCT, DPU_INTR_DISCLEAR, DPU_INTR_CLEAR_MASK);
}

void ithDpuInitCtrl(ITH_DPU_MODE DpuMode)
{
    uint32_t CtrlRegMask = 0x773BFFFF;

    ithWriteRegMaskA(ITH_DPU_REG_DCR, DpuDsrInitArray[DpuMode], CtrlRegMask);
}

void ithDpuEnableIntr(void)
{
    ithWriteRegMaskA(ITH_DPU_REG_INTCT, DPU_INTR_ENABLE, DPU_INTR_ENABLE_MASK);
}

void ithDpuDisableIntr(void)
{
    ithWriteRegMaskA(ITH_DPU_REG_INTCT, DPU_INTR_DISABLE, DPU_INTR_ENABLE_MASK);
}

void ithDpuSetSrcAddr(unsigned int SrcAddr)
{
    ithWriteRegA(ITH_DPU_REG_SAR, SrcAddr);
}

void ithDpuGetStatus(unsigned int *Reg32)
{
    *Reg32 = ithReadRegA(ITH_DPU_REG_DSR);
}

void ithDpuSetDstAddr(unsigned int DstAddr)
{
    ithWriteRegA(ITH_DPU_REG_DAR, DstAddr);
}

void ithDpuSetEncrypt(void)
{
    ithWriteRegMaskA(ITH_DPU_REG_DCR, DPU_SET_ENCODE, DPU_SET_ENCODE_DECODE_MASK);
}

void ithDpuSetDescrypt(void)
{
    ithWriteRegMaskA(ITH_DPU_REG_DCR, DPU_SET_DECODE, DPU_SET_ENCODE_DECODE_MASK);
}

void ithDpuFire(void)
{
    ithWriteRegMaskA(ITH_DPU_REG_DCR, DPU_FIRE_BIT, DPU_FIRE_BIT);
}

void ithDpuSetSize(unsigned int size)
{
    ithWriteRegA(ITH_DPU_REG_SSR, size);
}

void ithDpuSetKey(uint32_t *key, uint8_t KeyNum)
{
    unsigned char KeyIndex = 0;
    uint32_t *pKey = (uint32_t *)key;

    for (KeyIndex = 0; KeyIndex < KeyNum; KeyIndex++)
    {
        ithWriteRegA(((ITH_DPU_REG_KEY0) + (KeyIndex * 4)), pKey[KeyIndex]);
    }
}

void ithDpuSetVector(uint32_t *vector, uint8_t len)
{
    unsigned char vi = 0;
    uint32_t *pIV=(uint32_t *)vector;

    for (vi = 0; vi < len; vi++)
    {
        ithWriteRegA(((ITH_DPU_REG_VECTOR0) + (vi * 4)), pIV[vi]);
    }
}

bool ithDpuWait(void)
{
    unsigned int StatusReg32 = 0, TimeOutCnt = 1000000;
    unsigned char Result = 1;

    StatusReg32 = ithReadRegA(ITH_DPU_REG_DSR);
    while ((StatusReg32 & 0x1) == 0x0 && TimeOutCnt > 0)
    {
        (void)usleep(1);
        StatusReg32 = ithReadRegA(ITH_DPU_REG_DSR);
        TimeOutCnt--;
    }

    if (TimeOutCnt == 0)
    {
        (void)printf("ith DPU timeOut-970, dsr_base=%x, %x!!\n",ITH_DPU_REG_DSR, ithReadRegA(ITH_DPU_REG_DSR));
        Result = 0;
    }

    ithWriteRegMaskA(ITH_DPU_REG_DCR, 0x0, DPU_FIRE_BIT);

    return Result;
}

void ithDpuSuspend(void)
{
    unsigned int StatusReg32 = 0, TimeOutCnt = 1000000;
    int i = 0;

    StatusReg32 = ithReadRegA(ITH_DPU_REG_DSR);
    while ((StatusReg32 & 0x2) == 0x2 && TimeOutCnt > 0)
    {
        (void)usleep(1);
        StatusReg32 = ithReadRegA(ITH_DPU_REG_DSR);
        TimeOutCnt--;
    }

    if (TimeOutCnt == 0)
    {
        (void)printf("[dpu][warning]: TimeOutCnt,reg=%x\n", StatusReg32);
    }

    for (i = ITH_DPU_REG_DCR; i < ITH_DPU_REG_VECTOR3; i += 4)
    {
        switch (i)
        {
        case ITH_DPU_REG_INTCT:
            DpuRegs[(i - ITH_DPU_REG_DCR) / 4] = ithReadRegA(ITH_DPU_REG_INTCT);
            break;

        default:
            break;
        }
    }
}

void ithDpuResume(void)
{
    int i = 0;

    for (i = ITH_DPU_REG_DCR; i < ITH_DPU_REG_VECTOR3; i += 4)
    {
        switch (i)
        {
        case ITH_DPU_REG_INTCT:
            ithWriteRegA(ITH_DPU_REG_INTCT, DpuRegs[(i - ITH_DPU_REG_DCR) / 4]);
            break;

        default:
            break;
        }
    }
}

void ithDpuSetDpuEndian(unsigned int EndianIndex)
{
    ithWriteRegMaskA(ITH_DPU_REG_DCR, EndianIndex, DPU_SET_ENDIAN_MASK);
}

void ithDpuEnableClock(void)
{
    ithSetRegBitA(ITH_DPU_CLOCK, DPU_DPCLK_EN);
    ithSetRegBitA(ITH_DPU_CLOCK, DPU_M14CLK_EN);
    ithSetRegBitA(ITH_DPU_CLOCK, DPU_W19CLK_EN);
}

void ithDpuDisableClock(void)
{
    ithClearRegBitA(ITH_DPU_CLOCK, DPU_DPCLK_EN);
    ithClearRegBitA(ITH_DPU_CLOCK, DPU_M14CLK_EN);
    ithClearRegBitA(ITH_DPU_CLOCK, DPU_W19CLK_EN);
}
