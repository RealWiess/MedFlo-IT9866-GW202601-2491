/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * CLI system related functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include "cli_cfg.h"
#include "openrtos/FreeRTOS.h"
#include "openrtos/task.h"
#include "openrtos/FreeRTOS_CLI.h"
#include <stdlib.h>

static portBASE_TYPE prvTaskRebootCommand (char *       pcWriteBuffer,
                                           size_t       xWriteBufferLen,
                                           const char * pcCommandString)
{
    int i;
    (void)pcCommandString;
    configASSERT(pcWriteBuffer);

    (void)xWriteBufferLen;

    CLI_LOG_INFO("reboot countdown:\n");
    for (i = 3; i > 0; --i)
    {
        CLI_LOG_INFO("%d..\n", i);
        sleep(1);
    }

    exit(0);

    return pdFALSE;
}

// clang-format off
static const xCommandLineInput xTaskRebootCommand = {
    (const char * const)"reboot",
    (const char * const)"reboot: Reboot this device\r\n",
    prvTaskRebootCommand,
    0
};
// clang-format on

void cliSystemInit (void)
{
    FreeRTOS_CLIRegisterCommand(&xTaskRebootCommand);
}
