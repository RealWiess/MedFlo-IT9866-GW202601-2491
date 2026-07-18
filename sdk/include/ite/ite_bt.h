/*
 * Copyright (c) 2012 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * Bluetooth Driver API header file.
 *
 * @author James Lin
 * @version 1.0
 */
#ifndef ITE_BT_H
#define ITE_BT_H


#ifdef __cplusplus
extern "C" {
#endif

#include "wifi/ite_port.h"
#ifndef WIN32
#include "openrtos/portmacro.h"
#endif

extern int mmpMtkBtDriverRegister(void);

//@}

#ifdef __cplusplus
}
#endif

#endif /* ITE_BT_H */

