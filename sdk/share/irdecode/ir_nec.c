#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <inttypes.h>
#include "irdecode/ir_nec.h"

#define ERR_RANGE_CHECK(sig, target)                                                                                   \
    ((sig) > ((target) * (100 - ERR_RANGE) / 100) && (sig) < ((target) * (100 + ERR_RANGE) / 100))

// #define DBG_MSG
#ifdef DBG_MSG
    #define nec_debug_(string, ...)                                                                                    \
        do                                                                                                             \
        {                                                                                                              \
            (void)printf("[NEC DEOCDER] ");                                                                            \
            (void)printf(string, __VA_ARGS__);                                                                         \
        } while (false)
#else
    #define nec_debug_(string, ...)
#endif

typedef void (*pfState)(uint32_t, uint32_t *);

static void     state_wait_sig_header (uint32_t sig, uint32_t * code);
static void     state_check_sig_header (uint32_t sig, uint32_t * code);
static void     state_wait_sig_bit (uint32_t sig, uint32_t * code);
static void     state_check_sig_bit (uint32_t sig, uint32_t * code);

static pfState  state_func = &state_wait_sig_header;
static uint32_t sigCode = 0U, sigCount = 0U;

static int      nec_process_bit (bool bit)
{
    sigCode = (sigCode << 1U) | bit;

    if (++sigCount == NEC_TOTAL_BIT)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static void nec_bit_check (uint32_t * code, uint8_t * addr, uint8_t * cmd)
{
    uint8_t bAddr, bAddrComp, bCmd, bCmdComp, err = 0U;
    bCmdComp  = (uint8_t)(~*code & (0xFFU));
    bCmd      = (uint8_t)((*code >> 8U) & 0xFFU);
    bAddrComp = (uint8_t)((~*code >> 16U) & 0xFFU);
    bAddr     = (uint8_t)((*code >> 24U) & 0xFFU);

    if (bCmdComp != bCmd)
    {
        nec_debug_("CMD bit check error: bCmd: 0x%X, bCmdComp: 0x%X\n", bCmd, bCmdComp);
        err = (err << 1U) | 0x1U;
    }
    if (bAddrComp != bAddr)
    {
        nec_debug_("ADDR bit check error: bAddr: 0x%X, bAddrComp: 0x%X\n", bAddr, bAddrComp);
        err = (err << 1U) | 0x1U;
    }
    if (err != 0U)
    {
        nec_debug_("code check error: 0x%X\n", *code);
        *code = 0U;
    }
    else
    {
        *addr = bAddr;
        *cmd  = bCmd;
    }
}

static void state_wait_sig_header (uint32_t sig, uint32_t * code)
{
    if (ERR_RANGE_CHECK(sig, NEC_START_HIGH))
    {
        nec_debug_("sig high get! %" PRIu32 "\n", sig);
        state_func = &state_check_sig_header;
    }
}

static void state_check_sig_header (uint32_t sig, uint32_t * code)
{
    if (ERR_RANGE_CHECK(sig, NEC_START_LOW))
    {
        nec_debug_("header low get! %" PRIu32 "\n", sig);
        sigCode    = 0U;
        sigCount   = 0U;
        state_func = &state_wait_sig_bit;
    }
    else if (ERR_RANGE_CHECK(sig, NEC_RPT_LOW))
    {
        nec_debug_("repeat low get! %d\n", sig);
        *code      = sigCode;
        state_func = &state_check_sig_header;
    }
    else
    {
        nec_debug_("error unknow sig sequence! %d\n", sig);
        state_func = &state_check_sig_header;
    }
}

static void state_wait_sig_bit (uint32_t sig, uint32_t * code)
{
    if (ERR_RANGE_CHECK(sig, NEC_BIT_HIGH))
    {
        nec_debug_("bit high get! %d\n", sig);
        state_func = &state_check_sig_bit;
    }
    else
    {
        nec_debug_("error unknow sig sequence! %d\n", sig);
        state_func = &state_wait_sig_header;
    }
}

static void state_check_sig_bit (uint32_t sig, uint32_t * code)
{
    int recv_stat = 0;
    if (ERR_RANGE_CHECK(sig, NEC_BIT0_LOW))
    {
        nec_debug_("bit0 low get! %d\n", sig);
        recv_stat = nec_process_bit(0);
    }
    else if (ERR_RANGE_CHECK(sig, NEC_BIT1_LOW))
    {
        nec_debug_("bit1 low get! %d\n", sig);
        recv_stat = nec_process_bit(1);
    }
    else
    {
        nec_debug_("error unknow sig sequence! %d\n", sig);
        state_func = &state_wait_sig_header;
        return;
    }
    if (recv_stat)
    {
        nec_debug_("all NEC data received! %d\n", sig);
        *code      = sigCode;
        state_func = &state_wait_sig_header;
    }
    else
    {
        state_func = &state_wait_sig_bit;
    }
}

uint32_t ir_NEC_decoder (uint32_t * data, int len, uint8_t * addr, uint8_t * cmd)
{
    int      i;
    uint32_t code = 0U;

    for (i = 0; i < len; i++)
    {
        nec_debug_("data[%d]: %d\n", i, *(data + i));
        state_func(*(data + i), &code);
    }
    if (code != 0U)
    {
        nec_bit_check(&code, addr, cmd);
    }
    return code;
}
