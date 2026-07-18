/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * HAL Host functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include "ith_cfg.h"

static uint16_t hostRegs[(ITH_HOST_BUS_CTRL18_REG - ITH_HOST_BUS_CTRL1_REG + 2) / 2];

void ithHostSuspend(void)
{
    uint16_t i = 0U;

    for (i = 0U; i < ITH_COUNT_OF(hostRegs); i++)
    {
        hostRegs[i] = ithReadRegH(ITH_HOST_BUS_CTRL1_REG + i * 2);
    }
    // TODO: SUSPEND MEMORY
}

void ithHostResume(void)
{
    uint16_t i = 0U;

    for (i = 0U; i < ITH_COUNT_OF(hostRegs); i++)
    {
        ithWriteRegH(ITH_HOST_BUS_CTRL1_REG + i * 2, hostRegs[i]);
    }
}
