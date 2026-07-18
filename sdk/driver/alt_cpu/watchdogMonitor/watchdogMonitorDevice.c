#include <string.h>
#include "alt_cpu/watchdogMonitor/watchdogMonitor.h"

static uint8_t gpWatchdogMonitorImage[] =
{
#include "watchdogMonitor.hex"
};

static void
watchdogMonitorProcessCommand (
    int cmdId)
{
    int i = 0;
    ALT_CPU_COMMAND_REG_WRITE(REQUEST_CMD_REG, cmdId);
    for (;;)
    {
        if (ALT_CPU_COMMAND_REG_READ(RESPONSE_CMD_REG) != cmdId)
        {
            // Waiting ALT CPU response
            continue;
        }
        else
        {
            break;
        }
    }
    ALT_CPU_COMMAND_REG_WRITE(REQUEST_CMD_REG, 0);
    for (i = 0; i < 1024; i++)
    {
        asm ("");
    }
    ALT_CPU_COMMAND_REG_WRITE(RESPONSE_CMD_REG, 0);
}

static int
watchdogMonitorIoctl (
    int             file,
    unsigned long   request,
    void            * ptr,
    void            * info)
{
    uint8_t * pWriteAddress = (uint8_t *) (iteRiscGetTargetMemAddress(ALT_CPU_IMAGE_MEM_TARGET) + CMD_DATA_BUFFER_OFFSET);
    switch (request)
    {
        case ITP_IOCTL_INIT:
        {
            // Stop ALT CPU
            iteRiscResetCpu(ALT_CPU);

            // Clear Commuication Engine and command buffer
            memset(pWriteAddress, 0x0, MAX_CMD_DATA_BUFFER_SIZE);
            ALT_CPU_COMMAND_REG_WRITE(REQUEST_CMD_REG, 0);
            ALT_CPU_COMMAND_REG_WRITE(RESPONSE_CMD_REG, 0);

            // Load Engine First
            iteRiscLoadData(ALT_CPU_IMAGE_MEM_TARGET, gpWatchdogMonitorImage, sizeof(gpWatchdogMonitorImage));
            (void)printf("sizeof gpWatchdogMonitorImage: %d bytes\n", (int)sizeof(gpWatchdogMonitorImage));
            // Fire Alt CPU
            iteRiscFireCpu(ALT_CPU);
            break;
        }

        case ITP_IOCTL_WATCHDOG_MONITOR_RUN:
        {
            (void)printf("watchdog monitor start run...\n");
            watchdogMonitorProcessCommand(RUN_CMD_ID);
            break;
        }

        case ITP_IOCTL_WATCHDOG_MONITOR_STOP:
        {
            watchdogMonitorProcessCommand(STOP_CMD_ID);
            break;
        }

        default:
            break;
    }
    return 0;
}

const ITPDevice itpDeviceWatchdogMonitor =
{
    ":watchdogMonitor",
    itpOpenDefault,
    itpCloseDefault,
    itpReadDefault,
    itpWriteDefault,
    itpLseekDefault,
    watchdogMonitorIoctl,
    NULL
};
