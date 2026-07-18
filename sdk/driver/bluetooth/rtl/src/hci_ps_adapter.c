/**
 * Copyright (C) 2017 Realtek Semiconductor Corporation
 * Author: Thomas_li <thomas_li@realsil.com.cn>
 */
#include "hci_ps.h"
#include "hci_board_if.h"
#include "osif_tp.h"
#include "hci_tp_dbg.h"


extern T_HCI_PROCESS_INFO *hci_process_info;
uint8_t g_hci_step = 0;

bool hci_pos_process_init(void)
{
    g_hci_step = 0; //init the step
    return true;
}

bool hci_ps_start_next_process(void)
{
    uint8_t ret = 0xff;
    do
    {
        if (hci_process_table[g_hci_step].start_pro != NULL)
        {
            ret = hci_process_table[g_hci_step].start_pro();
        }
        else
        {
            // has complete no support to here;
            hci_tp_err("total step  is %x, not support to here", g_hci_step);
            tp_osif_delay(2000);
            return true;
        }
        if (ret == HCI_TP_CHECK_AGAIN)
        {
            return true;
        }
        else if (ret == HCI_TP_CONFIG_FAIL)
        {
            //ABORT THE PROCESS
            return false;
        }
        else if (ret == HCI_TP_CHECK_OK)
        {
            //send ok not ++
            return true;
        }
        g_hci_step++;
    }
    while (ret == HCI_TP_NOT_SEND);

    if (ret == HCI_TP_CHECK_OK)
    {
        //normal start
        return true;
    }
    else
    {
        hci_tp_err("ret = %x, g_hci_ps_step = %x", ret, g_hci_step);
        return false;
    }
}



//capute

bool hci_pos_process_deinit(void)
{
    g_hci_step = 0; //init the step
    if (hci_process_info->baudrate_timer_handle != NULL)
    {
        tp_osif_timer_stop(&hci_process_info->baudrate_timer_handle);
        tp_osif_timer_delete(&hci_process_info->baudrate_timer_handle);
    }

    return true;
}

bool hci_ps_check_process(uint8_t *p_buf, uint16_t len)
{
    (void)len;
    uint8_t evt_code;
    uint16_t opcode;
    uint8_t  status;
    uint8_t  p_buf_len;
    uint16_t idx = 0;
#ifdef USE_HCI_H4
    //if h4 may be here
    uint8_t pkt_type;
    pkt_type = p_buf[idx++];
    if (pkt_type != HCI_EVT_PKT)
    {
        hci_tp_err("packet type is error: %x", pkt_type);
        //do nothing dont check
        return false;
    }
#endif
    evt_code = p_buf[idx++];
    p_buf_len = p_buf[idx++];
    //util_hexdump('R', p_buf, len);
    if (evt_code != HCI_COMMAND_COMPLETE)
    {
        hci_tp_err("packet type is error: %x packet is\r\n", evt_code);
        hci_ps_util_hexdump('E', p_buf, len);
        //do nothing dont check
        return false;
    }
    idx++; /*skip num of complete */
    opcode = p_buf[idx] | (p_buf[idx + 1] << 8);
    idx += 2;

    if (opcode == hci_process_table[g_hci_step].opcode)//check the opcode
    {
        status = p_buf[idx++];
        if (status != 0)
        {
            hci_tp_err("wrong status is %x, opcode is 0x%x\n", status, opcode);
            return false;
        }

        if (hci_process_table[g_hci_step].check_func != NULL)
        {
            uint8_t ret;
            ret = hci_process_table[g_hci_step].check_func(p_buf_len, p_buf + idx);
            switch (ret)
            {
            case HCI_TP_CHECK_OK:
                goto hci_tp_config_ok;
            //break;
            case HCI_TP_CHECK_AGAIN:
                goto hci_tp_config_again;
            //break;
            case HCI_TP_CONFIG_FAIL:
                goto hci_tp_config_fail;
            //break;
            case HCI_TP_CONFIG_END:
                goto hci_tp_config_end;
            //break;
            case HCI_TP_CHECK_WAIT_TIMEOUT:
                goto hci_tp_config_wait_timeout;
            //break;
            case HCI_TP_CHECK_ERROR:
                goto hci_tp_config_fail;
            //break;
            default:
                hci_tp_err("\r\n%s:unexpect status is %x\n", __FUNCTION__, ret);
                break;
            }
        }
        else
        {
            if (p_buf_len == 4)
            {
                // hci_tp_warn("no need check is %x, opcode %x\n", g_hci_step, opcode);
            }
            hci_tp_info("no need check step is %x, opcode %x\n", g_hci_step, opcode);
        }
hci_tp_config_ok:
        //current step is ok
        g_hci_step++;
        if (hci_process_table[g_hci_step].opcode == 0)
        {
            //the process end
            tp_osif_printf("\r\nBT INIT success %x\n", g_hci_step);
            if (rtk_hci_board_complete() == true)
            {
                //FREE
                hci_pos_process_deinit();
                rtk_hci_ps_deinit();
                goto hci_tp_config_end;
            }
            else
            {
                goto hci_tp_config_fail;
            }
        }
        else
        {
            hci_ps_start_next_process();
        }
    }
    else
    {
        hci_tp_err("wrong opcode is %x, current step %x,opcode is 0x%x\n", opcode, g_hci_step,
                   hci_process_table[g_hci_step].opcode);
    }
    return false;

hci_tp_config_fail:
    hci_ps_complete(false);
    return false;

hci_tp_config_end:
    hci_ps_complete(true);
    return true;

hci_tp_config_again:
    hci_process_table[g_hci_step].start_pro();
    return true;

hci_tp_config_wait_timeout:
    g_hci_step++;
    //hci_process_table[g_hci_step].start_pro();
    return true;
}






