/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stddef.h>
#include "os/os.h"
#include "sysinit/sysinit.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "transport/uart/ble_hci_uart.h"
#include "hci_if_task.h"
#include "host/src/ble_att_priv.h"
#if NIMBLE_CFG_CONTROLLER
//#include "controller/ble_ll.h"
#endif

static struct ble_npl_eventq g_eventq_dflt;

extern void os_msys_init(void);

static struct ble_hs_stop_listener bhst_listener;
static void
bhst_stop_cb(int status, void *arg)
{
}

void
nimble_port_init(void)
{
    /* Initialize default event queue */
    ble_npl_eventq_init(&g_eventq_dflt);
    /* Initialize the global memory pool */
    os_msys_init();
    /* Initialize the host */
    ble_hs_init();
	ble_hci_uart_init();
#if NIMBLE_CFG_CONTROLLER
    ble_ll_init();
#endif
}

void
nimble_port_run(void)
{
    struct ble_npl_event *ev;

    while (1) {
        ev = ble_npl_eventq_get(&g_eventq_dflt, BLE_NPL_TIME_FOREVER);
        ble_npl_event_run(ev);
    }
}

int
nimble_port_suspend(void)
{
	int rc;

	if (!hci_if_check_opened())
	{
		printf("interface is already off, return\n");
		return 1;
	}
	ble_att_svr_reset();
	rc = ble_hs_stop(&bhst_listener, bhst_stop_cb, NULL);
	if (rc)
		printf("ble_gap_adv_stop failed: %d\n", rc);
	ble_hci_uart_deconfig();
	return rc;
}

int
nimble_port_resume(void)
{
	int rc;

	if (hci_if_check_opened())
	{
		printf("interface is already on, return\n");
		return 1;
	}

	ble_hci_uart_config();

	rc = ble_att_svr_init();
	if (rc)
	{
		printf("ble_att_svr_init failed: %d\n", rc);
		goto done;
	}

	rc = ble_gap_adv_active();
	if (rc)
	{
		printf("ble_gap_adv_active failed: %d\n", rc);
		goto done;
	}
	rc = ble_gatts_resume();
	if (rc)
	{
		printf("ble_gatts_resume failed: %d\n", rc);
		goto done;
	}
	rc = ble_hs_start();
	if (rc)
	{
		printf("ble_hs_start failed: %d\n", rc);
		goto done;
	}

done:
	return rc;
}

struct ble_npl_eventq *
nimble_port_get_dflt_eventq(void)
{
    return &g_eventq_dflt;
}

#if NIMBLE_CFG_CONTROLLER
void
nimble_port_ll_task_func(void *arg)
{
    extern void ble_ll_task(void *);

    ble_ll_task(arg);
}
#endif
