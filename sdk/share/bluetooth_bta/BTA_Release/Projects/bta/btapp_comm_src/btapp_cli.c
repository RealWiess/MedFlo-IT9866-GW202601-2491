/****************************************************************************
**
**  Name:          btapp_cli.c
**
**  Description:   Contains btapp trigger commands' action file
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

//#include "fsl_debug_console.h"
//#include "fsl_shell.h"

#include "bta_platform.h"
#include "bte_glue.h"

#define BT_STACK_ENABLE_FMT_STR             "enable"
#define BT_STACK_DISABLE_FMT_STR            "disable"
#define BT_BLE_ADV_START_FMT_STR            "ble_start_adv"
#define BT_BLE_ADV_STOP_FMT_STR             "ble_stop_adv"
#define BT_BLE_SCAN_START_FMT_STR           "ble_start_scan"
#define BT_BLE_SCAN_STOP_FMT_STR            "ble_stop_scan"
#define BT_DISCOVERY_ENABLE_FMT_STR         "bt_discover_enable"
#define BT_DISCOVERY_DISABLE_FMT_STR        "bt_discover_disable"

#define BT_GATT_SERVER_START_FMT_STR        "gatt_server_start"
#define BT_GATT_SERVER_STOP_FMT_STR         "gatt_server_stop"
#define BT_GATT_SERVER_TEST_SEND_FMT_STR    "gatt_server_test_send"

#define BT_GATT_IBEACON_START_FMT_STR       "ble_ibeacon_start"
#define BT_GATT_IBEACON_STOP_FMT_STR        "ble_ibeacon_stop"
#define BT_GATT_IBEACON_ADV_FMT_STR         "ble_ibeacon_adv"

#define BT_GATT_CLIENT_START_FMT_STR        "gatt_client_start"
#define BT_GATT_CLIENT_STOP_FMT_STR         "gatt_client_stop"
#define BT_GATT_CLIENT_CONNECT_FMT_STR      "gatt_client_connect"
#define BT_GATT_CLIENT_DISCONNECT_FMT_STR   "gatt_client_disconect"

#define BT_SPP_REGISTER_IDX_FMT_STR         "bt_spp_register_idx"
#define BT_SPP_REGISTER_NUM_FMT_STR         "bt_spp_register_num"
#define BT_SPP_DEREGISTER_ALL_FMT_STR       "bt_spp_deregister"
#define BT_SPP_TEST_SEND_FMT_STR            "bt_spp_test_send"
#define BT_SPP_TEST_SEND_THT_FMT_STR        "bt_spp_test_send_tht"

#define BT_AVK_FMT_STR                      ""

#define BT_DEBUG_GET_GKI_FREE_FMT_STR       "gki_free"
#define BT_DEBUG_PRINT_GKI_FMT_STR          "gki_alloced"
#define BT_DEBUG_LPM_MODE_FMT_STR           "lpm_mode"
#define BT_DEBUG_STACK_STRESS_TEST_FMT_STR  "stack_stress"
#define BT_DEBUG_TRACE_CONTROL_FMT_STR      "trace_ctrl"

#define BT_MFG_INIT_FMT_STR                 "init"
#define BT_MFG_DEINIT_FMT_STR               "deinit"
#define BT_MFG_RESET_FMT_STR                "reset"
#define BT_MFG_LE_TX_FMT_STR                "ble_tx"
#define BT_MFG_LE_ENHANCED_TX_FMT_STR       "ble_enhanced_tx"
#define BT_MFG_LE_ENHANCED_RX_FMT_STR       "ble_enhanced_rx"
#define BT_MFG_LE_TEST_END_FMT_STR          "ble_test_end"
#define BT_MFG_CLASSIC_TX_FMT_STR           "bt_tx"
#define BT_MFG_CLASSIC_RX_FMT_STR           "bt_rx"
#define BT_MFG_CLASSIC_TX_END_FMT_STR       "bt_test_end"
#define BT_MFG_HCI_SEND_ANY_FMT_STR         "hci_send_any"

btapp_cli_t brcm_bt_cli_tbl[] = {
                                {BT_STACK_ENABLE_FMT_STR,           0,      btapp_init},                  //brcm_bt enable
                                {BT_STACK_DISABLE_FMT_STR,          0,      btapp_deinit},                //brcm_bt disable
                                {BT_BLE_ADV_START_FMT_STR,          0,      btapp_ble_start_adv},
                                {BT_BLE_ADV_STOP_FMT_STR,           0,      btapp_ble_stop_adv},
                                {BT_BLE_SCAN_START_FMT_STR,         0,      btapp_ble_start_scan},
                                {BT_BLE_SCAN_STOP_FMT_STR,          0,      btapp_ble_stop_scan},
                                {BT_DISCOVERY_ENABLE_FMT_STR,       0,      btapp_discoverity_enable},
                                {BT_DISCOVERY_DISABLE_FMT_STR,      0,      btapp_discoverity_disable},

                                {BT_GATT_SERVER_START_FMT_STR,      0,      btapp_gatt_server_start},
                                {BT_GATT_SERVER_STOP_FMT_STR,       0,      btapp_gatt_server_stop},
                                {BT_GATT_SERVER_TEST_SEND_FMT_STR,  1,      btapp_gatt_test_send},

                                {BT_GATT_IBEACON_START_FMT_STR,     0,      btapp_ble_ibeacon_start},
                                {BT_GATT_IBEACON_STOP_FMT_STR,      0,      btapp_ble_ibeacon_stop},
                                {BT_GATT_IBEACON_ADV_FMT_STR,       1,      btapp_ble_ibeacon_adv},

                                {BT_GATT_CLIENT_START_FMT_STR,      0,      btapp_gatt_client_start},
                                {BT_GATT_CLIENT_STOP_FMT_STR,       0,      btapp_gatt_client_stop},
                                {BT_GATT_CLIENT_CONNECT_FMT_STR,    1,      btapp_gatt_client_connect},
                                {BT_GATT_CLIENT_DISCONNECT_FMT_STR, 1,      btapp_gatt_client_disconnect},

                                {BT_SPP_REGISTER_IDX_FMT_STR,       1,      btapp_spp_register_idx},
                                {BT_SPP_REGISTER_NUM_FMT_STR,       1,      btapp_spp_register_nums},
                                {BT_SPP_DEREGISTER_ALL_FMT_STR,     0,      btapp_spp_deregister_all},
                                {BT_SPP_TEST_SEND_FMT_STR,          2,      btapp_spp_test_send},
                                {BT_SPP_TEST_SEND_THT_FMT_STR,      2,      btapp_spp_test_send_tht},

                                {BT_DEBUG_GET_GKI_FREE_FMT_STR,     0,      btapp_debug_print_gki_free},
                                {BT_DEBUG_PRINT_GKI_FMT_STR,        0,      btapp_debug_print_gki_alloced},
                                {BT_DEBUG_LPM_MODE_FMT_STR,         1,      btapp_debug_lpm_mode_set},
                                {BT_DEBUG_STACK_STRESS_TEST_FMT_STR,1,      btapp_stack_stress_test},
                                {BT_DEBUG_TRACE_CONTROL_FMT_STR,    2,      btapp_trace_ctrl},

                                };

btapp_cli_t bt_mfg_cli_tbl[] = {
                                {BT_MFG_INIT_FMT_STR,               0,      bt_mfg_init},            //bt_mfg init
                                {BT_MFG_DEINIT_FMT_STR,             0,      bt_mfg_deinit},          //bt_mfg deinit
                                {BT_MFG_RESET_FMT_STR,              0,      bt_hci_reset},           //bt_mfg reset
                                {BT_MFG_LE_TX_FMT_STR,              0xFF,   bt_le_tx_test},          //bt_mfg le_transmitter_test
                                {BT_MFG_LE_ENHANCED_TX_FMT_STR,     0xFF,   bt_le_enhanced_tx_test}, //bt_mfg le_enhanced_transmitter_test
                                {BT_MFG_LE_ENHANCED_RX_FMT_STR,     0xFF,   bt_le_enhanced_rx_test}, //bt_mfg le_enhanced_receiver_test
                                {BT_MFG_LE_TEST_END_FMT_STR,        0xFF,   bt_le_test_end},         //bt_mfg le_test_end
                                {BT_MFG_CLASSIC_TX_FMT_STR,         0xFF,   bt_radio_tx_test},       //bt_mfg radio_tx_test
                                {BT_MFG_CLASSIC_RX_FMT_STR,         0xFF,   bt_radio_rx_test},       //bt_mfg radio_rx_test
                                {BT_MFG_CLASSIC_TX_END_FMT_STR,     0,      bt_hci_reset},           //bt_mfg radio_tx_test_end
                                {BT_MFG_HCI_SEND_ANY_FMT_STR,       1,      bt_mfg_hci_send_any},    //bt_mfg hci_send_any
                                };

#if (SEL_PLATFORM == PLATFORM_RT595_EVK || SEL_PLATFORM == PLATFORM_RT685_EVK)
static shell_status_t brcm_bt_cli_handler(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t bt_mfg_cli_handler(shell_handle_t shellHandle, int32_t argc, char **argv);

SHELL_COMMAND_DEFINE(brcm_bt,
                     LINE_ENDING
                     "\"brcm_bt args ...\":"
                     LINE_ENDING
                     "Usage:"
                     LINE_ENDING,
                     brcm_bt_cli_handler,
                     SHELL_IGNORE_PARAMETER_COUNT
                    );

SHELL_COMMAND_DEFINE(bt_mfg,
                     LINE_ENDING
                     "\"bt_mfg args ...\":"
                     LINE_ENDING
                     "Usage:"
                     LINE_ENDING,
                     bt_mfg_cli_handler,
                     SHELL_IGNORE_PARAMETER_COUNT
                    );

static shell_status_t brcm_bt_cli_handler(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    uint8_t idx = 0;

    for(idx = 0; idx < ARRAY_COUNT(brcm_bt_cli_tbl); idx++)
    {
        if(strcmp(argv[1],brcm_bt_cli_tbl[idx].str_fmt) == 0 && (brcm_bt_cli_tbl[idx].argc == 0xFF || ((argc - 2) == brcm_bt_cli_tbl[idx].argc)))
        {
            if(brcm_bt_cli_tbl[idx].handler != NULL)
            {
                brcm_bt_cli_tbl[idx].handler((argc - 2), (const char**)&argv[2]);
                break;
            }
        }
    }

    if(idx == ARRAY_COUNT(brcm_bt_cli_tbl))
    {
        BRCM_PLATFORM_TRACE("%s, argc:%d not find command:%s to handle"LINE_ENDING, __FUNCTION__, argc, argv[0]);
    }

    return kStatus_SHELL_Success;
}

static shell_status_t bt_mfg_cli_handler(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    uint8_t idx = 0;

    for(idx = 0; idx < ARRAY_COUNT(bt_mfg_cli_tbl); idx++)
    {
        if(strcmp(argv[1],bt_mfg_cli_tbl[idx].str_fmt) == 0 && (bt_mfg_cli_tbl[idx].argc == 0xFF || ((argc - 2) == bt_mfg_cli_tbl[idx].argc)))
        {
            if(bt_mfg_cli_tbl[idx].handler != NULL)
            {
                bt_mfg_cli_tbl[idx].handler((argc - 2), (const char**)&argv[2]);
                break;
            }
        }
    }

    if(idx == ARRAY_COUNT(bt_mfg_cli_tbl))
    {
        BRCM_PLATFORM_TRACE("%s, argc:%d not find command:%s to handle"LINE_ENDING, __FUNCTION__, argc, argv);
    }

    return kStatus_SHELL_Success;
}

void btapp_cli_init(void)
{
    SHELL_RegisterCommand(app_shell_handle, SHELL_COMMAND(brcm_bt));
    SHELL_RegisterCommand(app_shell_handle, SHELL_COMMAND(bt_mfg));
}
#elif (SEL_PLATFORM == PLATFORM_AMZ_IVD)
int brcm_bt_cli_handler(int32_t argc, char **argv)
{
    uint8_t idx = 0;

    for(idx = 0; idx < ARRAY_COUNT(brcm_bt_cli_tbl); idx++)
    {
        if(strcmp(argv[0], brcm_bt_cli_tbl[idx].str_fmt) == 0 && (brcm_bt_cli_tbl[idx].argc == 0xFF || ((argc - 1) == brcm_bt_cli_tbl[idx].argc)))
        {
            if(brcm_bt_cli_tbl[idx].handler != NULL)
            {
                brcm_bt_cli_tbl[idx].handler((argc - 1), (const char**)&argv[1]);
                break;
            }
        }
    }

    if(idx == ARRAY_COUNT(brcm_bt_cli_tbl))
    {
        BRCM_PLATFORM_TRACE("%s, argc:%d not find command:%s to handle"LINE_ENDING, __FUNCTION__, argc, argv);
    }

    return 0;
}

int bt_mfg_cli_handler(int32_t argc, char **argv)
{
    uint8_t idx = 0;

    for(idx = 0; idx < ARRAY_COUNT(bt_mfg_cli_tbl); idx++)
    {
        if(strcmp(argv[0],bt_mfg_cli_tbl[idx].str_fmt) == 0 && (bt_mfg_cli_tbl[idx].argc == 0xFF || ((argc - 1) == bt_mfg_cli_tbl[idx].argc)))
        {
            if(bt_mfg_cli_tbl[idx].handler != NULL)
            {
                bt_mfg_cli_tbl[idx].handler((argc - 1), (const char**)&argv[1]);
                break;
            }
        }
    }

    if(idx == ARRAY_COUNT(bt_mfg_cli_tbl))
    {
        BRCM_PLATFORM_TRACE("%s, argc:%d not find command:%s to handle"LINE_ENDING, __FUNCTION__, argc, argv);
    }

    return 0;
}

#endif
