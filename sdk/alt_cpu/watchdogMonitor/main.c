#include <stdarg.h>

#include "alt_cpu/watchdogMonitor/watchdogMonitor.h"
#include "alt_cpu/alt_cpu_utility.h"

#define ENDIAN_SWAP16(x) \
        (((x & 0x00FF) << 8) | ((x & 0xFF00) >> 8))

#define ENDIAN_SWAP32(x) \
        (((x & 0x000000FF) << 24) | \
        ((x & 0x0000FF00) <<  8) | \
        ((x & 0x00FF0000) >>  8) | \
        ((x & 0xFF000000) >> 24))

typedef struct
{
    uint32_t bRun;
} WATCHDOG_MONITOR_HANDLE;

static WATCHDOG_MONITOR_HANDLE gtMonitorHandle = { 0 };

static void watchdogMonitorProcessRunCmd(void)
{
    gtMonitorHandle.bRun = 1;
}

static void watchdogMonitorProcessStopCmd(void)
{
    gtMonitorHandle.bRun = 0;
}

static void monitorWatchdogStatus(void)
{
    //Check if watchdog interrupt is triggered.
    if (ithReadRegA(0xD1A00010))
    {
        int i = 0;
        for (i = 0; i < 1024 * 1024; i++)
        {
            asm("");
        }
        if (ithReadRegA(0xD1A00010) && (ithReadRegA(0xD1A0000C) != 0x3))
        {
            //Reset AXI SPI
            ithWriteRegA(0xD800006C, 0x80000000);
            for (i = 0; i < 1024; i++)
            {
                asm("");
            }
            //Trigger watchdog reboot.
            ithWriteRegA(0xD1A0000C, 0x3);
            ithWriteRegA(0xD1A00004, 0x0);
            ithWriteRegA(0xD1A00008, 0x5AB9);
        }
    }
}

int main(int argc, char **argv)
{
    //Set GPIO and Clock Setting

    while(1)
    {
        int inputCmd = ALT_CPU_COMMAND_REG_READ(REQUEST_CMD_REG);
        int i;
        if (inputCmd && ALT_CPU_COMMAND_REG_READ(RESPONSE_CMD_REG) == 0)
        {
            switch(inputCmd)
            {
                case RUN_CMD_ID:
                    watchdogMonitorProcessRunCmd();
                    break;
                case STOP_CMD_ID:
                    watchdogMonitorProcessStopCmd();
                    break;
                default:
                    break;
            }
            ALT_CPU_COMMAND_REG_WRITE(RESPONSE_CMD_REG, (uint16_t) inputCmd);
        }
        
        if (gtMonitorHandle.bRun)
        {
            monitorWatchdogStatus();
        }
        
        for (i = 0; i < 1024 * 10; i++)
            asm("");
    }
}
