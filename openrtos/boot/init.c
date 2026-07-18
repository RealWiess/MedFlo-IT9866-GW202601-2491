/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * Initialize functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include <stdio.h>
#include <sys/ioctl.h>
#include "openrtos/FreeRTOS.h"
#include "openrtos/semphr.h"
#include "ite/itp.h"
#include "alt_cpu/alt_cpu_device.h"
#include "alt_cpu/swUart/swUart.h"

// FIXME: remove inline the tlb.c here. (Kuoping)
// I don't know why it cannot compile the tlb.c standalone. It must
// include inlined here, else the linker do not link the tlb.o.

extern uint32_t __mem_end;
extern void init_retarget_locks(void);

#define VMEM_START  (64) // keep 0 for NULL

#ifdef __SM32__
int int_init(void);
int int_enable(unsigned long vect);
#endif

#if (CFG_CHIP_FAMILY == 970 || CFG_CHIP_FAMILY == 9860)
static void _IdleForClkAdjust(void) __attribute__ ((naked)) __attribute__((__optimize__ ("-fno-stack-protector")));
static void _IdleForClkAdjust(void)
{    
    int idleLoop;
    for (idleLoop = 0; idleLoop < 1024; idleLoop++)
    {
        __asm__ __volatile__("nop");
    }
    __asm__ __volatile__ ("mov pc, lr");
}
#endif

#if (CFG_CHIP_FAMILY == 9860)
    #if defined(CFG_NOR_DTRMODE_FREQ) && (CFG_NOR_DTRMODE_FREQ > 0)
static float _SetSCLKClock(void)
{
    float DTR_FREQ = CFG_NOR_DTRMODE_FREQ;
    float PLL1_N1 = 0, PLL2_N1 = 0, PLL2_N2 = 0;
    uint32_t PLL_numerator = 0U;
    uint32_t PLL_pre_divider = 0U;
    uint32_t PLL_post_divider = 0U;
    uint32_t reg_Val = 0U;
    uint32_t CLK_base = 12U;
    uint32_t sclk_divider_p11 = 0U;
    uint32_t sclk_divider_p21 = 0U;
    uint32_t sclk_divider_p22 = 0U;
    uint32_t sclk_src_divider = 0U;

    reg_Val = ithReadRegA(0xD8000100U);
    PLL_numerator = (reg_Val & 0x7F0000U) >> 16U;
    PLL_pre_divider = reg_Val & 0x1FU;
    PLL_post_divider = (reg_Val & 0x7F00U) >> 8U;
    PLL1_N1 = (float)CLK_base * 1 / PLL_pre_divider * PLL_numerator * 1 / PLL_post_divider;
    for (sclk_divider_p11 = 1U; sclk_divider_p11 < 300U; sclk_divider_p11++)
    {
        if (DTR_FREQ > PLL1_N1 / (float)sclk_divider_p11 / 4)
        {
            PLL1_N1 = PLL1_N1 / (float)sclk_divider_p11 / 4;
            break;
        }
    }

    reg_Val = ithReadRegA(0xD8000110U);
    PLL_numerator = (reg_Val & 0x7F0000U) >> 16U;
    PLL_pre_divider = reg_Val & 0x1FU;
    PLL_post_divider = (reg_Val & 0x7F00U) >> 8U;
    PLL2_N1 = (float)CLK_base * 1 / PLL_pre_divider * PLL_numerator * 1 / PLL_post_divider;
    for (sclk_divider_p21 = 1U; sclk_divider_p21 < 300U; sclk_divider_p21++)
    {
        if (DTR_FREQ > PLL2_N1 / (float)sclk_divider_p21 / 4)
        {
            PLL2_N1 = PLL2_N1 / (float)sclk_divider_p21 / 4;
            break;
        }
    }

    reg_Val = ithReadRegA(0xD8000118U);
    PLL_numerator = (reg_Val & 0x7F0000U) >> 16U;
    PLL_pre_divider = reg_Val & 0x1FU;
    PLL_post_divider = (reg_Val & 0x7F00U) >> 8U;
    PLL2_N2 = (float)CLK_base * 1 / PLL_pre_divider * PLL_numerator * 1 / PLL_post_divider;
    for (sclk_divider_p22 = 1U; sclk_divider_p22 < 300U; sclk_divider_p22++)
    {
        if (DTR_FREQ > PLL2_N2 / (float)sclk_divider_p22 / 4)
        {
            PLL2_N2 = PLL2_N2 / (float)sclk_divider_p22 / 4;
            break;
        }
    }

    if (PLL2_N2 > PLL2_N1)
    {
        if (PLL1_N1 > PLL2_N2)
        {
            sclk_src_divider = (0x000a8800U | ((0x0UL & 0x7UL) << 12U));
            sclk_src_divider = sclk_src_divider | (sclk_divider_p11 & 0x3ffU);
            ithWriteRegA(0xD800006CU, sclk_src_divider);
            return PLL1_N1;
        }
        else
        {
            sclk_src_divider = (0x000a8800U | ((0x3UL & 0x7UL) << 12U));
            sclk_src_divider = sclk_src_divider | (sclk_divider_p22 & 0x3ffU);
            ithWriteRegA(0xD800006CU, sclk_src_divider);
            return PLL2_N2;
        }
    }
    else
    {
        if (PLL1_N1 > PLL2_N1)
        {
            sclk_src_divider = (0x000a8800U | ((0x0UL & 0x7UL) << 12U));
            sclk_src_divider = sclk_src_divider | (sclk_divider_p11 & 0x3ffU);
            ithWriteRegA(0xD800006CU, sclk_src_divider);
            return PLL1_N1;
        }
        else
        {
            sclk_src_divider = (0x000a8800U | ((0x2UL & 0x7UL) << 12U));
            sclk_src_divider = sclk_src_divider | (sclk_divider_p21 & 0x3ffU);
            ithWriteRegA(0xD800006CU, sclk_src_divider);
            return PLL2_N1;
        }
    }
}
    #endif
#endif

void __init BootInit(void)
{
#if (CFG_CHIP_FAMILY == 970 || CFG_CHIP_FAMILY == 9860)
    uint32_t targetClkRegVal = ithReadRegA(0xD800027C);
    if (targetClkRegVal)
    {
        //to ensure idle instructions into I-Cache before clk adjustment.
        _IdleForClkAdjust();
        //adjust Arm CPU clock to target clock speed.
        ithWriteRegA(0xD800000C, targetClkRegVal);
        //Process idle routine to prevent clk adjust glitch to cause system crash issue.
        _IdleForClkAdjust();
        ithWriteRegA(0xD800027C, 0x0);
        ithWriteRegA(0xD1A0000C, 0x00000000);  //Disable Watchdog Setting from script.
    }
#endif

#ifdef CFG_DBG_STACK_PROTECTOR
    extern uintptr_t __stack_chk_guard;
    //Init stack protector canary value at first and this setup should be same value of __guard_setup.
    //The reason is current system init flow __guard_setup is called later.
    ((unsigned char *)&__stack_chk_guard)[0] = 0;
    ((unsigned char *)&__stack_chk_guard)[1] = 0;
    ((unsigned char *)&__stack_chk_guard)[2] = '\n';
    ((unsigned char *)&__stack_chk_guard)[3] = 255;
#endif

    init_retarget_locks();

#if (CFG_CHIP_FAMILY == 9860)
    #if defined(CFG_NOR_DTRMODE_FREQ) && (CFG_NOR_DTRMODE_FREQ > 0)
    float sclk = _SetSCLKClock();
    #endif
#endif

#if (CFG_CHIP_FAMILY == 9860)
    #if (CFG_WT_SIZE > 0)
    static ITHVmem vmem;
    #endif
    #ifdef CFG_M2D_RESERVE_MEM_ENABLE
    static ITHVmem vmem_m2d;
    #endif
#endif

#if (CFG_CHIP_FAMILY == 970)
    // workaround SD/NOR co-bus issue
    #if !defined(CFG_SD0_ENABLE) && !defined(CFG_SD1_ENABLE)
    ithWriteRegMaskA(0xD800006CU, 0x00000005U, 0x000001FFU);
    #endif
    // workaround AX1 JPEG bug
    if (ithGetRevisionId() == 0)
    {
        ithWriteRegMaskA(0xD8000038U, 0x0000080AU, 0x0000080FU);
    }
#endif

#ifdef CFG_BUILD_STL
    stl_startup();
#endif

    uint32_t bootTime = ithTimerGetTime(portTIMER);

#if defined(CFG_DBG_TRACE_ANALYZER) && defined(CFG_DBG_TRACE)
    vTraceEnable(TRC_START);
#endif

#ifdef CFG_OPENRTOS_MEMPOOL_ENABLE
    // init openrtos memory pool
    vPortInitialiseBlocks();
#endif

#if (CFG_CHIP_FAMILY == 9860)
    // init video memory management for write-back memory
    #if (CFG_WT_SIZE > 0)
    vmem.startAddr      = VMEM_START;
    vmem.totalSize      = CFG_WT_SIZE;
    vmem.mutex          = xSemaphoreCreateMutex();
    vmem.usedMcbCount = vmem.freeSize = 0;

    ithVmemInit(&vmem);
    #endif // #if (CFG_WT_SIZE > 0)

    #ifdef CFG_M2D_RESERVE_MEM_ENABLE
	vmem_m2d.startAddr      = VMEM_START;
    vmem_m2d.totalSize      = CFG_M2D_RESERVE_MEM_SIZE;
    vmem_m2d.mutex          = xSemaphoreCreateMutex();
    vmem_m2d.usedMcbCount = vmem_m2d.freeSize = 0;

    ithVmemM2DInit(&vmem_m2d);
    #endif
#endif // #if (CFG_CHIP_FAMILY == 9860)

#ifdef __SM32__
    int_init();
    int_enable(3);

#endif // __SM32__

    // init hal module
    ithInit();

#if defined(CFG_DBG_PRINTBUF)
    // init print buffer device
    itpRegisterDevice(ITP_DEVICE_STD, &itpDevicePrintBuf);
    itpRegisterDevice(ITP_DEVICE_PRINTBUF, &itpDevicePrintBuf);
    ioctl(ITP_DEVICE_PRINTBUF, ITP_IOCTL_INIT, NULL);

//#elif defined(CFG_DBG_SWUART)
    // init sw uart device
//    itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceSwUart);
//    itpRegisterDevice(ITP_DEVICE_SWUART, &itpDeviceSwUart);
//    ioctl(ITP_DEVICE_SWUART, ITP_IOCTL_INIT, (void*)CFG_SWUART_BAUDRATE);

#elif defined(CFG_DBG_UART0)
    // init uart device
    itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceUart0);
    itpRegisterDevice(ITP_DEVICE_UART0, &itpDeviceUart0);
    ioctl(ITP_DEVICE_UART0, ITP_IOCTL_INIT, NULL);
#elif defined(CFG_DBG_UART1)
    itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceUart1);
    itpRegisterDevice(ITP_DEVICE_UART1, &itpDeviceUart1);
    ioctl(ITP_DEVICE_UART1, ITP_IOCTL_INIT, NULL);
#elif defined(CFG_DBG_UART2)
    // init uart device
    itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceUart2);
    itpRegisterDevice(ITP_DEVICE_UART2, &itpDeviceUart2);
    ioctl(ITP_DEVICE_UART2, ITP_IOCTL_INIT, NULL);
#elif defined(CFG_DBG_UART3)
    itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceUart3);
    itpRegisterDevice(ITP_DEVICE_UART3, &itpDeviceUart3);
    ioctl(ITP_DEVICE_UART3, ITP_IOCTL_INIT, NULL);
#elif defined(CFG_DBG_UART4)
    // init uart device
    itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceUart4);
    itpRegisterDevice(ITP_DEVICE_UART4, &itpDeviceUart4);
    ioctl(ITP_DEVICE_UART4, ITP_IOCTL_INIT, NULL);
#elif defined(CFG_DBG_UART5)
    itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceUart5);
    itpRegisterDevice(ITP_DEVICE_UART5, &itpDeviceUart5);
    ioctl(ITP_DEVICE_UART5, ITP_IOCTL_INIT, NULL);
#elif defined(CFG_DBG_SWUART_CODEC)
    #if defined(CFG_SW_UART) && !defined(CFG_RS485_4_ENABLE)
    // init Risc
    iteRiscInit();

    // init alt cpu
    itpRegisterDevice(ITP_DEVICE_ALT_CPU, &itpDeviceAltCpu);

    {
        // NOTICE: alt_cpu only for chip 9850
        int               altCpuEngineType = ALT_CPU_SW_UART;
        SW_UART_INIT_DATA tUartInitData    = {0};

        itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceAltCpu);

        ioctl(ITP_DEVICE_ALT_CPU, ITP_IOCTL_ALT_CPU_SWITCH_ENG, &altCpuEngineType);
        ioctl(ITP_DEVICE_ALT_CPU, ITP_IOCTL_INIT, NULL);
        ioctl(ITP_DEVICE_ALT_CPU, ITP_IOCTL_INIT_DBG_UART, NULL);

        tUartInitData.cpuClock   = ithGetRiscCpuClock();
        tUartInitData.baudrate   = CFG_SWUARTDBGPRINTF_BAUDRATE;
        tUartInitData.uartRxGpio = -1;
        tUartInitData.uartTxGpio = CFG_SWUARTDBGPRINTF_GPIO;
        tUartInitData.parity     = NONE;
        ioctl(ITP_DEVICE_ALT_CPU, ITP_IOCTL_INIT_UART_PARAM, &tUartInitData);
    }
    #else
    int swuart_gpio;
    int swuart_baudrate;
        #if 0
    int swuart_parity;
        #endif
    iteRiscInit();
    iteRiscOpenEngine(1, 1);
    {
        int i;
        for (i = 0; i < 1000000; i++)
        {
            asm("");
        }
    }

    itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceSwUartCodecDbg);
    itpRegisterDevice(ITP_DEVICE_SWUARTDBG, &itpDeviceSwUartCodecDbg);

    swuart_gpio = CFG_SWUARTDBGPRINTF_GPIO;
    ioctl(ITP_DEVICE_SWUARTDBG, ITP_IOCTL_SET_GPIO_PIN, &swuart_gpio);
    swuart_baudrate = CFG_SWUARTDBGPRINTF_BAUDRATE;
    ioctl(ITP_DEVICE_SWUARTDBG, ITP_IOCTL_SET_BAUDRATE, &swuart_baudrate);
        #if 0
    swuart_parity = ITP_SWUART_NONE;
    ioctl(ITP_DEVICE_SWUARTDBG, ITP_IOCTL_SET_PARITY, &swuart_parity);
        #endif
    ioctl(ITP_DEVICE_SWUARTDBG, ITP_IOCTL_INIT, NULL);
    #endif
#endif // defined(CFG_DBG_PRINTBUF)

    (void)ithPrintf(CFG_SYSTEM_NAME "/" CFG_PROJECT_NAME " ver " CFG_VERSION_MAJOR_STR "." CFG_VERSION_MINOR_STR "." CFG_VERSION_PATCH_STR "." CFG_VERSION_CUSTOM_STR "." CFG_VERSION_TWEAK_STR "\n");

    // booting time
    (void)ithPrintf("booting time: %ums\r\n", bootTime / 1000);

#if (CFG_CHIP_FAMILY == 9860)
    // Host ID using AW1/AW2 for IT9860
    if ((ithReadRegA(ITH_HOST_BASE + 0x274U) & 0x1U) == 0x0U)
    {
        (void)ithPrintf("WARNNING: AW1 is ES sample, recommend to use MP revision.\n");
    }
#endif

#if (CFG_CHIP_FAMILY == 9860)
    #if defined(CFG_NOR_DTRMODE_FREQ) && (CFG_NOR_DTRMODE_FREQ > 0)
    (void)printf("NOR DTR Frequency: %.2f MHz\r\n", sclk);
    #endif
#endif
}
