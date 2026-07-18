/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * HAL USB functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include <unistd.h>
#include "ith_cfg.h"

#if (CFG_CHIP_FAMILY == 9860)
#define ITH_USB_STS_REG     0x2C
#define ITH_USB_PHY_REG     0x3C
#define ITH_USB_GLB1_REG    0x34
#define ITH_USB_GLB2_REG    0x38
#else // 9830
#define ITH_USB_STS_REG     0x50
#define ITH_USB_PHY_REG     0x58
#define ITH_USB_GLB_REG     0x54
#endif

void ithUsbSuspend(ITHUsbModule usb)
{
    ithWriteRegMaskA(usb + ITH_USB_PHY_REG, 0x0, 0x14);
    ithSetRegBitA(usb + ITH_USB_HC_MISC_REG, ITH_USB_HOSTPHY_SUSPEND_BIT);
}

void ithUsbResume(ITHUsbModule usb)
{
    ithWriteRegMaskA(usb + ITH_USB_PHY_REG, 0x10, 0x14);
    ithClearRegBitA(usb + ITH_USB_HC_MISC_REG, ITH_USB_HOSTPHY_SUSPEND_BIT);
}

void ithUsbEnableClock(void)
{
#if (CFG_CHIP_FAMILY == 9860)
    ithWriteRegMaskA(ITH_HOST_BASE + ITH_USB_CLK_REG, 0xA, 0xA);
#else // 9830
    ithSetRegBitA(ITH_HOST_BASE + ITH_USB_CLK_REG, 25); // Enable A5CLK (AXI clock for USB)
    ithSetRegBitA(ITH_HOST_BASE + ITH_USB_CLK_REG, 23); // Enable W26CLK (APB clock for USBPHY)
#endif
}

void ithUsbDisableClock(void)
{
#if (CFG_CHIP_FAMILY == 9860)
    ithWriteRegMaskA(ITH_HOST_BASE + ITH_USB_CLK_REG, 0x0, 0xA);
#else
    ithClearRegBitA(ITH_HOST_BASE + ITH_USB_CLK_REG, 25); // A5CLK (AXI clock for USB)
    ithClearRegBitA(ITH_HOST_BASE + ITH_USB_CLK_REG, 23); // W26CLK (APB clock for USBPHY)
#endif
}

void ithUsbReset(void)
{
#if (CFG_CHIP_FAMILY == 9860)
    ithWriteRegA(ITH_HOST_BASE + ITH_USB_CLK_REG, 0xC0A80000);
    (void)usleep(5*1000);
    ithWriteRegA(ITH_HOST_BASE + ITH_USB_CLK_REG, 0x00A80000);
    (void)usleep(5*1000);
#else  // for 9830
    ithWriteRegA(ITH_HOST_BASE + ITH_USB_CLK_REG, 0xC2800000);
    (void)usleep(5*1000);
    ithWriteRegA(ITH_HOST_BASE + ITH_USB_CLK_REG, 0x02800000);
    (void)usleep(5*1000);
#endif
}

void ithUsbInterfaceSel(ITHUsbInterface intf)
{
#if (CFG_CHIP_FAMILY == 9860)
    //uint32_t reg = 0x422A0800;
    uint32_t reg = 0x442A0800;
    if (intf == ITH_USB_AMBA)
        reg |= 0x1;
    else
        reg &= ~0x1;
    ithWriteRegA(ITH_USB0_BASE + ITH_USB_GLB1_REG, reg);
#else // for 9830
    if (intf == ITH_USB_AMBA)
        ithClearRegBitA(ITH_USB0_BASE + ITH_USB_GLB_REG, 17);
    else
        ithSetRegBitA(ITH_USB0_BASE + ITH_USB_GLB_REG, 17);
#endif
}

void ithUsbPhyPowerOn(ITHUsbModule usb)
{
#if (CFG_CHIP_FAMILY == 9860)
    ithWriteRegA(usb + ITH_USB_PHY_REG, 0x00031907);
#else
    //ithWriteRegA(usb + 0x58, 0x00031307); // for 9830 FPGA
    ithWriteRegA(usb + ITH_USB_PHY_REG, 0x00011907); // for test chip
#endif
}

int ithUsbPhyClockReady(ITHUsbModule usb)
{
    return (ithReadRegA(usb + ITH_USB_STS_REG) & 0x1);
}

int ithUsbDevicePresent(ITHUsbModule usb)
{
    return ithReadRegA(usb + 0x30) & 0x1;
}

void ithUsbForceDeviceMode(ITHUsbModule usb, bool enable)
{
#if (CFG_CHIP_FAMILY == 9860)
    if (enable)
        ithSetRegBitA(usb + ITH_USB_GLB2_REG, 9);
    else
        ithClearRegBitA(usb + ITH_USB_GLB2_REG, 9);
#else
    if (enable)
        ithSetRegBitA(usb + ITH_USB_GLB_REG, 8);
    else
        ithClearRegBitA(usb + ITH_USB_GLB_REG, 8);
#endif
}

void ithUsbSetIdPin(ITHUsbModule usb, uint32_t id_pin)
{
    ithGpioSetMode(id_pin, ITH_GPIO_MODE0);
    ithGpioSetIn(id_pin);

#if (CFG_CHIP_FAMILY == 9860)
    ithWriteRegMaskA(usb + ITH_USB_GLB2_REG, id_pin, 0x7F);
#else // 9830
    ithWriteRegMaskA(usb + ITH_USB_GLB_REG, id_pin, 0x7F);
#endif
}

void ithUsbSetRemoteWakeup(ITHUsbModule usb)
{
#if (CFG_CHIP_FAMILY == 9860)
    ithSetRegBitA(usb + ITH_USB_GLB2_REG, 10);
    (void)usleep(2 * 1000);
    ithClearRegBitA(usb + ITH_USB_GLB2_REG, 10);
#else // 9830
    ithSetRegBitA(usb + ITH_USB_GLB_REG, 9);
    (void)usleep(2 * 1000);
    ithClearRegBitA(usb + ITH_USB_GLB_REG, 9);
#endif
}

bool ithUsbIsDeviceMode(ITHUsbModule usb)
{
    if (ithReadRegA(usb + 0x80) & (0x1 << 21))
        return true;
    else
        return false;
}

#define USB_WRAP_REG						0x34
#define USB_WRAP_FLUSH_EN				(0x1 << 22)
#define USB_WRAP_FLUSH_FIRE_END (0x1 << 23)

void ithUsbWrapFlush(void)
{
#if (CFG_CHIP_FAMILY == 9860)
    uint32_t reg = ithReadRegA(ITH_USB0_BASE + 0x34);
    /* only for wrap path */
    if (!(reg & 0x1)) {
        int timeout = 100;

        ithEnterCritical();

        /* flush usb's ahb wrap */
        ithWriteRegMaskA(ITH_USB0_BASE + USB_WRAP_REG, USB_WRAP_FLUSH_EN, USB_WRAP_FLUSH_EN);
        ithWriteRegMaskA(ITH_USB0_BASE + USB_WRAP_REG, USB_WRAP_FLUSH_FIRE_END, USB_WRAP_FLUSH_FIRE_END);

        // wait AHB Wrap flush finish!
        #if 1 // workaround some IC flush will not end
        while (((ithReadRegA(ITH_USB0_BASE + USB_WRAP_REG) & USB_WRAP_FLUSH_FIRE_END)) && timeout--);
        if (timeout <= 0)
            (void)ithPrintf("!\n");
        #else
        while ((ithReadRegA(ITH_USB0_BASE + USB_WRAP_REG) & USB_WRAP_FLUSH_FIRE_END));
        #endif

        ithWriteRegMaskA(ITH_USB0_BASE + USB_WRAP_REG, 0x0, USB_WRAP_FLUSH_EN);

        /* flush axi2 wrap */
        if(ithMemWrapFlush(ITH_MEM_AXI2WRAP))
            (void)ithPrintf("+\n");

        ithExitCritical();
    }
#else
    return;
#endif
}
