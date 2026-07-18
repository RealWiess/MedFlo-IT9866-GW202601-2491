/****************************************************************************
**
**  Name:          btapp_cli.h
**
**  Description:   Contains btapp trigger commands' action header file
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_CLI_H
#define BTAPP_CLI_H

typedef int8_t (*btapp_cli_func_t)(int32_t argc, const char **argv);

typedef struct
{
    char*            str_fmt;
    int32_t          argc;
    btapp_cli_func_t handler;
}btapp_cli_t;

void btapp_cli_init(void);

#endif
