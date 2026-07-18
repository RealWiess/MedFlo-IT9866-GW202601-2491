/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * HAL utility functions.
 *
 * @author Jim Tan
 * @version 1.0
 */

#include <string.h>
#include <ith/ith_defs.h>
#include <ith/ith_utility.h>
#include <ith/ith_vram.h>
#include <ith/ith_reg.h>
#include "ith_cfg.h"

char * ithU32tob (char * s, uint32_t i)
{
    char *        cp      = s;
    uint32_t bitMask = ~0U - (~0U >> 1U);

    do
    {
        *cp = ((i & bitMask) != 0U) ? '1' : '0';
        cp++;
        bitMask >>= 1U;
    } while (bitMask != 0U);

    *cp = '\0';
    return s;
}

void ithPrintRegH (uint16_t addr, unsigned int size)
{
    uint16_t     reg, p;
    unsigned int i, j, count;

    if (size == 0U)
    {
        return;
    }

    reg   = ITH_ALIGN_DOWN(addr, sizeof(uint16_t));
    count = size / sizeof(uint16_t);

    j     = 0U;
    p     = reg;
    for (i = 0U; i < count; ++i)
    {
        uint16_t value = ithReadRegH(p);

        if (j == 0U)
        {
            PRINTF("0x%04X:", p);
        }

        PRINTF(" %04X", value);

        if (j >= 7U)
        {
            PRINTF("\r\n");
            j = 0U;
        }
        else
        {
            j++;
        }

        p += sizeof(uint16_t);
    }

    if (j > 0U)
    {
        PRINTF("\r\n");
    }
}

void ithPrintRegA (uint32_t addr, unsigned int size)
{
    uint32_t     reg, p;
    unsigned int i, j, count;

    if (size == 0U)
    {
        return;
    }

    reg   = ITH_ALIGN_DOWN(addr, sizeof(uint32_t));
    count = size / sizeof(uint32_t);

    j     = 0U;
    p     = reg;
    for (i = 0U; i < count; ++i)
    {
        uint32_t value = ithReadRegA(p);

        if (j == 0U)
        {
            PRINTF("0x%08X:", p);
        }

        PRINTF(" %08X", value);

        if (j >= 3U)
        {
            PRINTF("\r\n");
            j = 0U;
        }
        else
        {
            j++;
        }

        p += sizeof(uint32_t);
    }

    if (j > 0U)
    {
        PRINTF("\r\n");
    }
}

void ithPrintVram8 (uint32_t addr, unsigned int size)
{
    uint8_t *    base, *mem;
    unsigned int i, j;

    if (size == 0U)
    {
        return;
    }

    base = (uint8_t *)ithMapVram(addr, size, ITH_VRAM_READ);
    mem  = base;

    j    = 0U;
    for (i = 0U; i < size; ++i)
    {
        if (j == 0U)
        {
            PRINTF("0x%08X:", addr);
        }

        PRINTF(" %02X", *mem);

        if (j >= 15U)
        {
            PRINTF("\r\n");
            j = 0U;
        }
        else
        {
            j++;
        }

        addr++;
        mem++;
    }

    ithUnmapVram(base, size);

    if (j > 0U)
    {
        PRINTF("\r\n");
    }
}

void ithPrintVram16 (uint32_t addr, unsigned int size)
{
    uint16_t *   base, *mem;
    unsigned int i, j, count;

    if (size == 0U)
    {
        return;
    }

    base  = (uint16_t *)ithMapVram(addr, size, ITH_VRAM_READ);
    mem   = base;
    count = size / sizeof(uint16_t);

    j     = 0U;
    for (i = 0U; i < count; ++i)
    {
        if (j == 0U)
        {
            PRINTF("0x%08X:", addr);
        }

        PRINTF(" %04X", *mem);

        if (j >= 7U)
        {
            PRINTF("\r\n");
            j = 0U;
        }
        else
        {
            j++;
        }

        addr += sizeof(uint16_t);
        mem++;
    }

    ithUnmapVram(base, size);

    if (j > 0U)
    {
        PRINTF("\r\n");
    }
}

void ithPrintVram32 (uint32_t addr, unsigned int size)
{
    uint32_t *   base, *mem;
    unsigned int i, j, count;

    if (size == 0U)
    {
        return;
    }

    base  = (uint32_t *)ithMapVram(addr, size, ITH_VRAM_READ);
    mem   = base;
    count = size / sizeof(uint32_t);

    j     = 0U;
    for (i = 0U; i < count; ++i)
    {
        if (j == 0U)
        {
            PRINTF("0x%08X:", addr);
        }

        PRINTF(" %08X", *mem);

        if (j >= 3U)
        {
            PRINTF("\r\n");
            j = 0U;
        }
        else
        {
            j++;
        }

        addr += sizeof(uint32_t);
        mem++;
    }

    ithUnmapVram(base, size);

    if (j > 0U)
    {
        PRINTF("\r\n");
    }
}
