/****************************************************************************
**
**  Name:          btapp.c
**
**  Description:   Contains btapp top file
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bta_platform.h"
#include "bte_glue.h"
#include "gki_int.h" //for mutex, should fix



extern int BootEntry(boot_cplt_cb_t cb);
SemaphoreHandle_t bt_stack_sync_sema =NULL;
uint8_t    btapp_inited = 0;

int8_t btapp_init(int argc, const char **argv)
{
    int stack_up = 0;

    UNUSED(argc);
    UNUSED(argv);
    

    if(btapp_inited == 0)
    {
        BRCM_PLATFORM_TRACE("btapp_init free heap size:%d\n", xPortGetFreeHeapSize());

        gki_openrtos_init_semaphore(&bt_stack_sync_sema);
        if(bt_stack_sync_sema == NULL)
        {
            BRCM_PLATFORM_TRACE("btapp_init No resource to create the sema"LINE_ENDING);
            return -1;
        }

        BootEntry(btapp_dm_post_reset);

        stack_up = gki_openrtos_take_semaphore(&bt_stack_sync_sema, 8000);

        if(stack_up == 0)
        {
            vTaskDelay(2000);                //Since the bt_stack_sync_sema posted, the stack don't bring-up enough.
            BRCM_PLATFORM_TRACE("btapp_init stack bring-up success!"LINE_ENDING);
            btapp_inited = 1;

            //BR,EDR
            btapp_discoverity_enable(0, NULL);

            //BLE
            btapp_gatt_server_start(0, NULL);

        }
        else
        {
            BRCM_PLATFORM_TRACE("btapp_init stack bring-up timeout!"LINE_ENDING);
        }
    }
    else
    {
        BRCM_PLATFORM_TRACE("btapp_init already!"LINE_ENDING);
    }

    return 0;
}

int8_t btapp_deinit(int argc, const char **argv)
{
    int stack_down;

    UNUSED(argc);
    UNUSED(argv);

    if(btapp_inited == 1)
    {
        //BRCM_PLATFORM_TRACE("btapp_deinit free heap size:%d"LINE_ENDING, xPortGetFreeHeapSize());

        btapp_dm_disable_bt();

        stack_down = gki_openrtos_take_semaphore(&bt_stack_sync_sema,3000);

        if(stack_down)
        {
            BRCM_PLATFORM_TRACE("btapp_deinit success!"LINE_ENDING);
        }
        else
        {
            BRCM_PLATFORM_TRACE("btapp_deinit timeout!"LINE_ENDING);
        }

        GKI_shut_down();
        gki_openrtos_deinit_semaphore(&bt_stack_sync_sema);
        btapp_inited = 0;

        //BRCM_PLATFORM_TRACE("btapp_deinit free heap size:%d"LINE_ENDING, xPortGetFreeHeapSize());
    }
    else
    {
        BRCM_PLATFORM_TRACE("btapp_deinit already!"LINE_ENDING);
    }

    return 0;
}

int8_t btapp_ble_start_adv(int argc, const char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    VERIFY_BT_STACK_ENABLED();

    btapp_gatts_adv_start();

    return 0;
}

int8_t btapp_ble_stop_adv(int argc, const char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    VERIFY_BT_STACK_ENABLED();

    btapp_gatts_adv_stop();

    return 0;
}
//MSH_CMD_EXPORT(btapp_ble_stop_adv, btapp_ble_stop_adv);

int8_t btapp_ble_start_scan(int argc, const char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    VERIFY_BT_STACK_ENABLED();

    btapp_ble_observer_start();

    return 0;
}
//MSH_CMD_EXPORT(btapp_ble_start_scan, btapp_ble_start_scan);

int8_t btapp_ble_stop_scan(int argc, const char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    VERIFY_BT_STACK_ENABLED();

    btapp_ble_observer_stop();

    return 0;
}
//MSH_CMD_EXPORT(btapp_ble_stop_scan, btapp_ble_stop_scan);

int8_t btapp_discoverity_enable(int argc, const char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    VERIFY_BT_STACK_ENABLED();

    btapp_dm_set_visibility(1, 0);

    return 0;
}
//MSH_CMD_EXPORT(btapp_discoverity_enable, btapp_discoverity_enable);

int8_t btapp_discoverity_disable(int argc, const char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    VERIFY_BT_STACK_ENABLED();

    btapp_dm_set_visibility(0, 0);

    return 0;
}
//MSH_CMD_EXPORT(btapp_discoverity_disable, btapp_discoverity_disable);

int8_t btapp_gatt_server_start(int argc, const char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    VERIFY_BT_STACK_ENABLED();

    btapp_gatts_start();

    return 0;
}
//MSH_CMD_EXPORT(btapp_gatt_server_start, btapp_gatt_server_start);

int8_t btapp_gatt_server_stop(int argc, const char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    VERIFY_BT_STACK_ENABLED();

    btapp_gatts_stop();

    return 0;
}
//MSH_CMD_EXPORT(btapp_gatt_server_stop, btapp_gatt_server_stop);

int8_t btapp_gatt_test_send(int argc, const char **argv)
{
    uint8_t send_buf[20];
    uint8_t* p_parm = (uint8_t*)argv[1];
    uint16_t pos = 0, idx = 0;

    VERIFY_BT_STACK_ENABLED();

    while(pos < strlen(p_parm) && idx < sizeof(send_buf))
    {
        unsigned int val = 0;
        if (sscanf(p_parm + pos, "%02x", &val) != 1)
        {
            BRCM_PLATFORM_TRACE("btapp_gatt_test_send Invalid format"LINE_ENDING);
            return -1;
        }
        send_buf[idx++] = val;
        pos += 3;
    }

    btapp_gatts_send_2_peer(send_buf, idx);

    return 0;
}

int8_t btapp_ble_ibeacon_start(int argc, const char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    VERIFY_BT_STACK_ENABLED();

    btapp_ble_ibeacon_start_ibeacon();

    return 0;
}

int8_t btapp_ble_ibeacon_stop(int argc, const char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    VERIFY_BT_STACK_ENABLED();

    btapp_ble_ibeacon_stop_ibeacon();

    return 0;
}

int8_t btapp_ble_ibeacon_adv(int argc, const char **argv)
{
    int enable = 0;

    VERIFY_BT_STACK_ENABLED();

    enable = atoi(argv[1]);

    if(enable == 0)
    {
        btapp_ble_ibeacon_stop_adv();
    }
    else if(enable == 1)
    {
        btapp_ble_ibeacon_start_adv(0);
    }
    else
    {
        BRCM_PLATFORM_TRACE("btapp_ble_ibeacon_adv not support params"LINE_ENDING);
    }

    return 0;
}

int8_t btapp_gatt_client_start(int argc, const char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    VERIFY_BT_STACK_ENABLED();

    btapp_gattc_init();

    return 0;
}

int8_t btapp_gatt_client_stop(int argc, const char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    VERIFY_BT_STACK_ENABLED();

    btapp_gattc_deinit();

    return 0;
}

int8_t btapp_gatt_client_connect(int argc, const char **argv)
{
    BD_ADDR* connect_bda = NULL;

    VERIFY_BT_STACK_ENABLED();

    connect_bda = btapp_utl_str_to_bda(argv[1]);

    if(connect_bda != NULL)
    {
        btapp_gattc_open(*connect_bda, 1);
    }
    else
    {
        BRCM_PLATFORM_TRACE("btapp_gatt_client_connect invalid bda params"LINE_ENDING);
    }

    return 0;
}

int8_t btapp_gatt_client_disconnect(int argc, const char **argv)
{
    BD_ADDR* disconnect_bda = NULL;

    VERIFY_BT_STACK_ENABLED();

    disconnect_bda = btapp_utl_str_to_bda(argv[1]);

    if(disconnect_bda != NULL)
    {
        btapp_gattc_close(*disconnect_bda);
    }
    else
    {
        BRCM_PLATFORM_TRACE("btapp_gatt_client_disconnect invalid bda params"LINE_ENDING);
    }

    return 0;
}

int8_t btapp_spp_register_idx(int argc, const char **argv)
{
    int idx = 0;

    UNUSED(argc);

    VERIFY_BT_STACK_ENABLED();

    idx = atoi(argv[1]);

    btapp_dg_register_entry(idx);

    return 0;
}

int8_t btapp_spp_register_nums(int argc, const char **argv)
{
    int nums = 0;

    UNUSED(argc);

    VERIFY_BT_STACK_ENABLED();

    nums = atoi(argv[1]);

    btapp_dg_register_entry_nums(nums);

    return 0;
}

int8_t btapp_spp_deregister_all(int argc, const char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    VERIFY_BT_STACK_ENABLED();

    btapp_dg_deregister_all();

    return 0;
}

int8_t btapp_spp_test_send(int argc, const char **argv)
{
    int sel_idx= 0;
    uint8_t send_buf[20];
    uint8_t* p_parm = (uint8_t*)argv[2];
    uint16_t pos = 0, idx = 0;

    UNUSED(argc);

    VERIFY_BT_STACK_ENABLED();

    sel_idx = atoi(argv[1]);

    while(pos < strlen(p_parm) && idx < sizeof(send_buf))
    {
        unsigned int val = 0;
        if (sscanf(p_parm + pos, "%02x", &val) != 1)
        {
            BRCM_PLATFORM_TRACE("btapp_gatt_test_send Invalid format"LINE_ENDING);
            return -1;
        }
        send_buf[idx++] = val;
        pos += 3;
    }

    btapp_dg_spp_send_data(sel_idx, send_buf, idx);

    return 0;
}

int8_t btapp_spp_test_send_tht(int argc, const char **argv)
{
    int sel_idx= 0;
    uint16_t test_cnt = 0;

    UNUSED(argc);

    VERIFY_BT_STACK_ENABLED();

    sel_idx = atoi(argv[1]);
    test_cnt = atoi(argv[2]);

    btapp_dg_spp_send_tht(sel_idx, test_cnt);

    return 0;
}


int8_t btapp_debug_print_gki_free(int argc, const char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    VERIFY_BT_STACK_ENABLED();

    GKI_print_free();

    return 0;
}

int8_t btapp_debug_print_gki_alloced(int argc, const char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    VERIFY_BT_STACK_ENABLED();

    GKI_print_alloced_all();

    return 0;
}

int8_t btapp_debug_lpm_mode_set(int argc, const char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    int lpm_mode = atoi(argv[1]);

    bte_hci_lpm_set(lpm_mode);

    return 0;
}

int8_t btapp_stack_stress_test(int argc, const char **argv)
{
    int i = 0;

    int test_cnt = atoi(argv[1]);

    for(i = 0; i < test_cnt; i ++)
    {
        BRCM_PLATFORM_TRACE(LINE_ENDING"btapp_stack_stress_test running count:%d"LINE_ENDING, i);
        btapp_init(0, NULL);

        btapp_deinit(0, NULL);
    }

    return 0;
}

int8_t btapp_trace_ctrl(int argc, const char **argv)
{
    int trace_enabled = 0;
    int trace_layers  = 0;
    UNUSED(argc);

    if (argc != 3)
    {
        printf("Please input 'cmb_test <DIVBYZERO|UNALIGNED>' \n");
        return 0;
    }

    APPL_TRACE_API4("btapp_trace_ctrl: enable %s(%d), layers %s(%d)",
        argv[1], atoi(argv[1]), argv[2], atoi(argv[2]));

    trace_enabled = atoi(argv[1]);
    trace_layers  = atoi(argv[2]);

    btapp_cfg_trace_enable(trace_enabled);
    btapp_cfg_trace_layer(trace_layers);

    return 0;
}

void btapp_task(UINT32 params)
{
    UINT16 event;
    BT_HDR* p_msg;
    UINT8  need_exit = FALSE;

    APPL_TRACE_API0("btapp_task up");

    while(1)
    {
        event = GKI_wait(0xFFFF, 0);
        if(event & TASK_MBOX_0_EVT_MASK)
        {
            while((p_msg = GKI_read_mbox(TASK_MBOX_0)) != NULL)
            {
                switch(p_msg->event)
                {
                case BTAPP_START_TIMER:
                    GKI_start_timer (TIMER_0, GKI_SECS_TO_TICKS (1), TRUE);
                    break;

                case BTAPP_STOP_TIMER:
                    /* if timer list is empty stop periodic GKI timer */
                    if (btapp_cb.timer_queue.p_first == NULL)
                    {
                        GKI_stop_timer(TIMER_0);
                    }
                    break;
                case BTAPP_TASK_SHUTDOWN:
                    need_exit = TRUE;
                    break;
                }
                GKI_freebuf(p_msg);
            }
        }

        if (event & TIMER_0_EVT_MASK)
        {
            TIMER_LIST_ENT  *p_tle;

            GKI_update_timer_list (&btapp_cb.timer_queue, 1);

            while ((btapp_cb.timer_queue.p_first) && (!btapp_cb.timer_queue.p_first->ticks))
            {
                p_tle = btapp_cb.timer_queue.p_first;
                GKI_remove_from_timer_list (&btapp_cb.timer_queue, p_tle);

                if(p_tle->p_cback)
                {
                    (*p_tle->p_cback)(p_tle);
                }
            }

            /* if timer list is empty stop periodic GKI timer */
            if (btapp_cb.timer_queue.p_first == NULL)
            {
                GKI_stop_timer(TIMER_0);
            }
        }

        if(need_exit == TRUE)
        {
            break;
        }
    }

    APPL_TRACE_DEBUG0("btapp_task shutdown");
    GKI_exit_task(BTAPPL_TASK);
}

void btapp_task_shutdown(void)
{
    BT_HDR* p_msg;
    if ((p_msg = (BT_HDR *)GKI_getbuf(BT_HDR_SIZE)) != NULL)
    {
        p_msg->event = BTAPP_TASK_SHUTDOWN;
        GKI_send_msg (BTAPPL_TASK, TASK_MBOX_0, p_msg);
    }
    else
    {
        GKI_TRACE_0("btapp_task_shutdown failed!");
    }
}

