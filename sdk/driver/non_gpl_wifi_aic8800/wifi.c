/*
 * Copyright (C) 2018-2020 AICSemi Ltd.
 *
 * All Rights Reserved
 */

#include "wifi.h"
#include <porting.h>
#include <log.h>
#include <sys/ioctl.h>
#include "ite/itp.h"
#include "dhcps.h"

#include "rwnx_defs.h"
#include "rtos_al.h"
#include "rwnx_main.h"
#include "cli_cmd.h"
#include "lmac_mac.h"
#include "wlan_if.h"

#define CONFIG_TEST_MAIN_EN 0
#define CONFIG_RTOS_AL_TEST_EN    0

#define AIC_WIFI_EVENT_ENABLE

#define AICWF_RELEASE_VERSION       "2023_1212_1547"

extern const char *aic_version;
extern const char *aic_date;
extern const char *rlsversion;

#ifdef AIC_WIFI_EVENT_ENABLE
static int aic_wifi_scan_status = 0;
#endif

aic_wifi_event_cb g_aic_wifi_event_cb = NULL;
//wifi_drv_event_cbk sp_aic_wifi_event_cb = NULL;
int dev_mode = WIFI_MODE_UNKNOWN;
int g_wifi_init = 0;

/*Static IP ADDRESS*/
#define IP_ADDR0   192
#define IP_ADDR1   168
#define IP_ADDR2   1
#define IP_ADDR3   80

/*NETMASK*/
#define NETMASK_ADDR0   255
#define NETMASK_ADDR1   255
#define NETMASK_ADDR2   255
#define NETMASK_ADDR3   0

/*Gateway Address*/
#define GW_ADDR0   192
#define GW_ADDR1   168
#define GW_ADDR2   1
#define GW_ADDR3   1

#if (CONFIG_RTOS_AL_TEST_EN)
rtos_semaphore sema = NULL;

rtos_task_handle test_task_handle = NULL;
#define TEST_TASK_ID 10
#define TEST_TASK_STACK_DEPTH 512
#define TEST_TASK_PRIO 1
rtos_semaphore task_sema = NULL;

#define TEST_QUEUE_ELT_CNT 5
struct test_queue_msg {
    uint32_t id;
    uint32_t param;
};
rtos_queue test_queue = NULL;

void my_timer_func(void *param)
{
    AIC_LOG_PRINTF("my_timer_func, now: %d, param: %x\n", rtos_now(false), (uint32_t)param);
    rtos_semaphore_signal(sema, 0);
}

void my_task_func(void *param)
{
    int ret = 0;
    AIC_LOG_PRINTF("%s enter, param: %x\n", __func__, (uint32_t)param);

    while (1) {
        ret = rtos_semaphore_wait(task_sema, 3000);
        if ((ret == 0))
            AIC_LOG_PRINTF("semaphore success\n");
        if ((ret == 1))
            AIC_LOG_PRINTF("semaphore timeout\n");
        else
            AIC_LOG_PRINTF("semaphore error\n");

        break;
    }
}

void rtos_al_test(void)
{
    AIC_LOG_PRINTF("rtos_al_test start\n");

    int ret = 0;
    unsigned int i = 0;

#if defined PLATFORM_GX_ECOS || PLATFORM_SUNPLUS_ECOS
    cyg_resolution_t resolution = cyg_clock_get_resolution(cyg_real_time_clock());
    AIC_LOG_PRINTF("dividend:%u divisor:%u\n", resolution.dividend, resolution.divisor);
#endif:

    // 1.rtos_now/rtos_msleep
    AIC_LOG_PRINTF("now: %d\n", rtos_now(false));
    rtos_msleep(20);
    AIC_LOG_PRINTF("now: %d\n", rtos_now(false));

#ifdef PLATFORM_GX_ECOS
    aic_dbg("nowms:%u\n", cyg_time_get_ms());
    aic_dbg("nowus:%u\n", cyg_time_get_us());
    rtos_msleep(1);
    aic_dbg("nowms:%u\n", cyg_time_get_ms());
    aic_dbg("nowus:%u\n", cyg_time_get_us());
    rtos_msleep(2);
    aic_dbg("nowms:%u\n", cyg_time_get_ms());
    aic_dbg("nowus:%u\n", cyg_time_get_us());
    rtos_msleep(3);
    aic_dbg("nowms:%u\n", cyg_time_get_ms());
    aic_dbg("nowus:%u\n", cyg_time_get_us());

    for (i = 0; i < 100; i++) {
        uint32_t now = rtos_now(false);
        uint32_t now_ms = cyg_time_get_ms();
        uint32_t now_us = cyg_time_get_us();
        aic_dbg("now:%u/%u/%u\n", now, now_ms, now_us);
        int j;
        for (j = 0; j < 1000; j++);
    }
#endif

    // 2.rtos_malloc/rtos_free/rtos_memcpy/rtos_memset
    char *ptr = NULL;
    ptr = (char *)rtos_malloc(16);
    if (ptr == NULL)
        AIC_LOG_PRINTF("rtos_malloc failed\n");
    else
        AIC_LOG_PRINTF("rtos_malloc successfully, addr: 0x%lx\n", (unsigned long)ptr);

    rtos_memset(ptr, 0, 16);
    //rwnx_data_dump("rtos_memset", ptr, 16);
    AIC_LOG_PRINTF("rtos_memset:\n");
    for (i = 0; i < 16; i++) {
        aic_dbg("%02x ", ptr[i]);
    }
    aic_dbg("\n");

    char a[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    rtos_memcpy(ptr, a, 16);
    //rwnx_data_dump("rtos_memset", ptr, 16);
    AIC_LOG_PRINTF("rtos_memcpy:\n");
    for (i = 0; i < 16; i++) {
        aic_dbg("%02x ", ptr[i]);
    }
    aic_dbg("\n");

    rtos_free(ptr);

    // 3.rtos_entercritical/rtos_exitcritical

    // 4.task
    ret = rtos_semaphore_create(&task_sema, "task_sema", 1, 0);
    if (ret != 0)
        AIC_LOG_PRINTF("rtos_semaphore_create failed\n");
    else
        AIC_LOG_PRINTF("rtos_semaphore_create successfully\n");

    ret = rtos_mutex_create(&task_mutex);
    if (ret != 0)
        AIC_LOG_PRINTF("rtos_mutex_create failed\n");
    else
        AIC_LOG_PRINTF("rtos_mutex_create successfully\n");

    AIC_LOG_PRINTF("test_task_handle ptr: %x\n", (uint32_t)&test_task_handle);
    ret = rtos_task_create(my_task_func, "Test", TEST_TASK_ID, TEST_TASK_STACK_DEPTH, &test_task_handle, TEST_TASK_PRIO, &test_task_handle);
    if (ret != 0)
        AIC_LOG_PRINTF("rtos_task_create failed\n");
    else
        AIC_LOG_PRINTF("rtos_task_create successfully\n");
    AIC_LOG_PRINTF("test_task_priority:%x\n" ,rtos_task_get_priority(test_task_handle));


    AIC_LOG_PRINTF("%s task_mutex lock start@%u\n", __func__, rtos_now(false));
    ret = rtos_mutex_lock(task_mutex, -1);
    AIC_LOG_PRINTF("%s task_mutex lock end@%u, ret=%d\n", __func__, rtos_now(false), ret);

#if 1
    AIC_LOG_PRINTF("test_task2_handle ptr: %x\n", (uint32_t)&test_task2_handle);
    ret = rtos_task_create(my_task2_func, "Test2", TEST_TASK2_ID, TEST_TASK2_STACK_DEPTH, &test_task2_handle, TEST_TASK2_PRIO, &test_task2_handle);
    if (ret != 0)
        AIC_LOG_PRINTF("rtos_task_create failed\n");
    else
        AIC_LOG_PRINTF("rtos_task_create successfully\n");
    AIC_LOG_PRINTF("test_task2_priority:%x\n" ,rtos_task_get_priority(test_task2_handle));
#endif

    rtos_msleep(10);
    rtos_semaphore_signal(task_sema, 0);
    rtos_msleep(150);

    AIC_LOG_PRINTF("%s task_mutex unlock@%u\n", __func__, rtos_now(false));
    rtos_mutex_unlock(task_mutex);

    AIC_LOG_PRINTF("test_task delete start\n");
    rtos_task_delete(test_task_handle);
    AIC_LOG_PRINTF("test_task delete end\n");
#if 1
    AIC_LOG_PRINTF("test_task2 delete start\n");
    rtos_task_delete(test_task2_handle);
    AIC_LOG_PRINTF("test_task2 delete end\n");
#endif
    rtos_semaphore_delete(task_sema);
    rtos_mutex_delete(task_mutex);

    // 5.queue
    struct test_queue_msg msg;
    ret = rtos_queue_create(sizeof(struct test_queue_msg), TEST_QUEUE_ELT_CNT, &test_queue, "test_queue");
    if (ret != 0)
        AIC_LOG_PRINTF("rtos_queue_create failed\n");
    else
        AIC_LOG_PRINTF("rtos_queue_create successfully\n");
    AIC_LOG_PRINTF("test_queue:%X\n", (uint32_t)test_queue);
    AIC_LOG_PRINTF("rtos_queue cnt:%d full:%d empty:%d\n", rtos_queue_cnt(test_queue), rtos_queue_is_full(test_queue), rtos_queue_is_empty(test_queue));
    AIC_LOG_PRINTF("rtos_queue_read empty queue start@:%d\n", rtos_now(false));
    ret = rtos_queue_read(test_queue, &msg, 10, 0);
    AIC_LOG_PRINTF("rtos_queue_read empty queue end @%d ret:%d\n", rtos_now(false), ret);

    msg.id = 1;
    msg.param = 1;
    ret = rtos_queue_write(test_queue, &msg, 0, 0);
    AIC_LOG_PRINTF("rtos_queue_write ret:%d id:%d param:%d\n", ret, msg.id, msg.param);
    AIC_LOG_PRINTF("rtos_queue cnt:%d full:%d empty:%d\n", rtos_queue_cnt(test_queue), rtos_queue_is_full(test_queue), rtos_queue_is_empty(test_queue));

    msg.id = 2;
    msg.param = 2;
    ret = rtos_queue_write(test_queue, &msg, 0, 0);
    AIC_LOG_PRINTF("rtos_queue_write ret:%d id:%d param:%d\n", ret, msg.id, msg.param);
    AIC_LOG_PRINTF("rtos_queue cnt:%d full:%d empty:%d\n", rtos_queue_cnt(test_queue), rtos_queue_is_full(test_queue), rtos_queue_is_empty(test_queue));

    memset(&msg, 0, sizeof(struct test_queue_msg));
    ret = rtos_queue_read(test_queue, &msg, 10, 0);
    AIC_LOG_PRINTF("rtos_queue_read ret:%d id:%d param:%d\n", ret, msg.id, msg.param);
    AIC_LOG_PRINTF("rtos_queue cnt:%d full:%d empty:%d\n", rtos_queue_cnt(test_queue), rtos_queue_is_full(test_queue), rtos_queue_is_empty(test_queue));

    memset(&msg, 0, sizeof(struct test_queue_msg));
    ret = rtos_queue_read(test_queue, &msg, 10, 0);
    AIC_LOG_PRINTF("rtos_queue_read ret:%d id:%d param:%d\n", ret, msg.id, msg.param);
    AIC_LOG_PRINTF("rtos_queue cnt:%d full:%d empty:%d\n", rtos_queue_cnt(test_queue), rtos_queue_is_full(test_queue), rtos_queue_is_empty(test_queue));

    rtos_queue_delete(test_queue);

    // 6.semaphore
    ret = rtos_semaphore_create(&sema, "sema", 1, 0);
    if (ret != 0)
        AIC_LOG_PRINTF("rtos_semaphore_create failed\n");
    else
        AIC_LOG_PRINTF("rtos_semaphore_create successfully\n");

    // 7.timer
    rtos_timer test_timer;
    AIC_LOG_PRINTF("test_timer ptr:%x\n", (uint32_t)&test_timer);
    ret = rtos_timer_create("test_timer", &test_timer, 10, 0, &test_timer, my_timer_func);
    if (ret != 0)
        AIC_LOG_PRINTF("rtos_timer_create failed\n");
    else
        AIC_LOG_PRINTF("rtos_timer_create successfully\n");

    AIC_LOG_PRINTF("test_timer start@%d\n", rtos_now(false));
    rtos_timer_start(test_timer, 0, 0);

    ret = rtos_semaphore_wait(sema, 30);
    if (ret == 0)
        AIC_LOG_PRINTF("get semaphore singal successfully\n");
    else if (ret == 1)
        AIC_LOG_PRINTF("get semaphore singal timeout\n");
    else
        AIC_LOG_PRINTF("get semaphore singal failed\n");

    ret = rtos_timer_delete(test_timer, 0);
    if ((ret != 0))
        AIC_LOG_PRINTF("rtos_timer_delete failed\n");
    else
        AIC_LOG_PRINTF("rtos_timer_delete successfully\n");

    ret = rtos_timer_create("test_timer", &test_timer, 10, 1, &test_timer, my_timer_func);
    if (ret != 0)
        AIC_LOG_PRINTF("rtos_timer_create failed\n");
    else
        AIC_LOG_PRINTF("rtos_timer_create successfully\n");

    AIC_LOG_PRINTF("test_timer start@%d\n", rtos_now(false));
    rtos_timer_start(test_timer, 20, 0);

    rtos_msleep(100);

    ret = rtos_timer_delete(test_timer, 0);
    if ((ret != 0))
        AIC_LOG_PRINTF("rtos_timer_delete failed\n");
    else
        AIC_LOG_PRINTF("rtos_timer_delete successfully\n");

    uint32_t idx = 0;
    for (idx = 0; idx < 20; idx++) {
        AIC_LOG_PRINTF("timer_sema count:%u\n", rtos_semaphore_get_count(timer_sema));
        ret = rtos_semaphore_wait(timer_sema, 10);
        AIC_LOG_PRINTF("timer_sema wait, idx:%d, ret:%d\n", idx, ret);
    }
    rtos_semaphore_delete(timer_sema);

    uint32_t sec = 0, usec = 0;
    aic_time_get(0, &sec, &usec);
    AIC_LOG_PRINTF("now:%u sec:%u usec:%u\n", rtos_now(false), sec, usec);

    AIC_LOG_PRINTF("sleep 10\n");
    rtos_msleep(10);

    aic_time_get(0, &sec, &usec);
    AIC_LOG_PRINTF("now:%u sec:%u usec:%u\n", rtos_now(false), sec, usec);

    AIC_LOG_PRINTF("rtos_al_test end\n");
}
#endif

void temp_isr(void)
{
    //AIC_LOG_PRINTF("temp_isr\r\n");
}


struct rwnx_hw *g_rwnx_hw = NULL;

void aicwf_get_chipid(void)
{
    struct rwnx_hw *rwnx_hw = g_rwnx_hw;
#if defined(CONFIG_AIC8801)
    rwnx_hw->chipid = PRODUCT_ID_AIC8801;
    AIC_LOG_PRINTF("aicwf chipid: USE AIC8801\r\n");
#elif defined(CONFIG_AIC8800DC)
    rwnx_hw->chipid = PRODUCT_ID_AIC8800DC;
    AIC_LOG_PRINTF("aicwf chipid: USE AIC8800DC\r\n");
#elif defined(CONFIG_AIC8800DW)
    rwnx_hw->chipid = PRODUCT_ID_AIC8800DW;
    AIC_LOG_PRINTF("aicwf chipid: USE AIC8800DW\r\n");
#elif defined(CONFIG_AIC8800D80)
    rwnx_hw->chipid = PRODUCT_ID_AIC8800D80;
    AIC_LOG_PRINTF("aicwf chipid: USE AIC8800D80\r\n");
#else
    AIC_LOG_PRINTF("aicwf chipid: no aic product\r\n");
#endif
}

unsigned int aicwf_is_5g_enable(void)
{
#ifdef USE_5G
    return 1;
#else
    return 0;
#endif
}

/**
 * @brief initializing wifi
 * @author
 * @date
 * @param [in] mode  wifi mode
 * @param [in] param a pointer to ap/sta cfg
 * @return int
 * @retval   0  initializing sucessful
 * @retval  -1 initializing fail
 */
static int aic_wifi_open(int mode, void *param, u16 chip_id)
{
    struct rwnx_hw *rwnx_hw = NULL;
    static uint8_t g_wifi_opened = 0;
    int ret = 0;
    //AIC_LOG_PRINTF("Wifilib version:%s\r\n", aic_wifi_get_version());
    AIC_LOG_PRINTF("aic_wifi_open: %d\n", mode);

    if (g_wifi_opened == 0)
    {
        // pwrkey en
        platform_pwr_en_pin_init();
        platform_pwr_en_pin_set(0);
        rtos_task_suspend(10);
        platform_pwr_en_pin_set(1);
        // alloc structs
        rwnx_hw = rtos_malloc(sizeof(struct rwnx_hw));
        if (rwnx_hw == NULL) {
            AIC_LOG_PRINTF("rwnx_hw alloc failed\r\n");
            return -1;
        }
        g_rwnx_hw = rwnx_hw;
        rwnx_hw->mode = mode;

        #ifdef CONFIG_SDIO_SUPPORT
        aicwf_get_chipid();
        sdio_host_init(rwnx_hw, temp_isr);
        #endif
        #ifdef CONFIG_USB_SUPPORT
        rwnx_hw->chipid = chip_id;
        aic_usb_host_init(rwnx_hw);
        #endif
        fhost_init(rwnx_hw);
        rwnx_cmd_mgr_init(&rwnx_hw->cmd_mgr);
        ret = rwnx_fdrv_init(rwnx_hw);
        if(ret) return ret;
        aic_cli_cmd_init(rwnx_hw);

        g_wifi_opened = 1;
    } else {
        rwnx_hw = g_rwnx_hw;
    }

    if (mode == WIFI_MODE_AP) {
		dhcps_init();
        #define AP_SSID_STRING  "AIC-AP-ITE"
        #define AP_PASS_STRING  "kkkkkkkk"
        struct aic_ap_cfg user_ap_cfg = {
            .aic_ap_ssid = {
                strlen(AP_SSID_STRING),
                AP_SSID_STRING
            },
            .aic_ap_passwd = {
                strlen(AP_PASS_STRING),
                AP_PASS_STRING
            },
            .band = 1,
            .type = PHY_CHNL_BW_20,
            .channel = 149,
            .hidden_ssid = 0,
            .max_inactivity = 60,
            .enable_he = 1,
            .enable_acs = 0,
            .bcn_interval = 100,
            .sta_num = 10,
        };
        #undef AP_SSID_STRING
        #undef AP_PASS_STRING
        rwnx_hw->net_id = wlan_start_ap(&user_ap_cfg);

		#ifdef CFG_NET_LWIP_2
		ip_addr_t ipaddr;
		ip_addr_t netmask;
		ip_addr_t gw;
		IP4_ADDR(ip_2_ip4(&ipaddr), GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
		IP4_ADDR(ip_2_ip4(&netmask), NETMASK_ADDR0, NETMASK_ADDR1 , NETMASK_ADDR2, NETMASK_ADDR3);
		IP4_ADDR(ip_2_ip4(&gw), GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
		netif_set_addr(fhost_to_net_if(0), ip_2_ip4(&ipaddr), ip_2_ip4(&netmask),ip_2_ip4(&gw));
		#else
		struct ip_addr ipaddr;
		struct ip_addr netmask;
		struct ip_addr gw;
		IP4_ADDR(&ipaddr, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
		IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1 , NETMASK_ADDR2, NETMASK_ADDR3);
		IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
		netif_set_addr(fhost_to_net_if(0), &ipaddr, &netmask, &gw);
		#endif

    } else if (mode == WIFI_MODE_STA) {
		#if 0
		wlan_if_scan();
		wifi_ap_list_t* scan_ap_list = (wifi_ap_list_t*)rtos_malloc(sizeof(wifi_ap_list_t));
   		if (scan_ap_list == NULL) {
        	aic_dbg("%s: scan_ap_list allocation failed\n", __func__);
        	return NULL;
    	}
    	memset(scan_ap_list, 0, sizeof(wifi_ap_list_t));
		wlan_if_getscan(scan_ap_list);

		rtos_free(scan_ap_list);
		//rwnx_hw->net_id = wlan_start_sta("Empty_SSID", "Empty_Password", -1);      
        #endif

    } else if (mode == WIFI_MODE_P2P) {
		dhcps_init();
		rwnx_hw->net_id = user_p2p_start(1);//user_p2p_start(0);
		#ifdef CFG_NET_LWIP_2
		ip_addr_t ipaddr;
		ip_addr_t netmask;
		ip_addr_t gw;
		IP4_ADDR(ip_2_ip4(&ipaddr), GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
		IP4_ADDR(ip_2_ip4(&netmask), NETMASK_ADDR0, NETMASK_ADDR1 , NETMASK_ADDR2, NETMASK_ADDR3);
		IP4_ADDR(ip_2_ip4(&gw), GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
		netif_set_addr(fhost_to_net_if(0), ip_2_ip4(&ipaddr), ip_2_ip4(&netmask),ip_2_ip4(&gw));
		#else
		struct ip_addr ipaddr;
		struct ip_addr netmask;
		struct ip_addr gw;
		IP4_ADDR(&ipaddr, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
		IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1 , NETMASK_ADDR2, NETMASK_ADDR3);
		IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
		netif_set_addr(fhost_to_net_if(0), &ipaddr, &netmask, &gw);
		#endif
	}

    g_wifi_init = 1;
    AIC_LOG_PRINTF("wifi open ok\r\n");
    return 0;
}

static int aic_wifi_close(int mode)
{
    int ret = 0;
    struct rwnx_hw *rwnx_hw = g_rwnx_hw;

    AIC_LOG_PRINTF("aic_wifi_deinit_mode: %d\n", mode);

    if (g_wifi_init == 0)
    {
        AIC_LOG_PRINTF("aic_wifi_deinit already deinit\r\n");
        return 0;
    }

    if (mode == WIFI_MODE_AP) {
        ret = wlan_stop_ap();
        if (ret) {
            AIC_LOG_PRINTF("wlan_stop_ap failed: %d\n", ret);
            return -1;
        } else {
            AIC_LOG_PRINTF("wlan_stop_ap success: %d\n", ret);
        }
        rwnx_hw->net_id = 0;
    }


#if 0
    aic_cli_cmd_deinit(rwnx_hw);
    rwnx_fdrv_deinit(rwnx_hw);
    rwnx_cmd_mgr_deinit(&rwnx_hw->cmd_mgr);
    fhost_deinit(rwnx_hw);
    sdio_host_deinit();

    rtos_free(rwnx_hw);
#endif

    AIC_LOG_PRINTF("aic_wifi_close success\r\n");
    return ret;
}

AIC_WIFI_MODE aic_wifi_get_mode(void)
{
    return dev_mode;
}

void aic_wifi_set_mode(AIC_WIFI_MODE mode)
{
    dev_mode = mode;
}


void aic_wifi_event_register(aic_wifi_event_cb cb)
{
    g_aic_wifi_event_cb = (aic_wifi_event_cb)cb;
}
#if 0
int wifi_drv_event_set_cbk(wifi_drv_event_cbk cbk)
{
    sp_aic_wifi_event_cb = cbk;
    AIC_LOG_PRINTF("%s is called, sp_aic_wifi_event_cb: %p\r\n", __func__, sp_aic_wifi_event_cb);

    return 1;
}

void wifi_drv_event_reset_cbk(void) {
    sp_aic_wifi_event_cb = NULL;

    aic_wifi_deinit(WIFI_MODE_AP);
}
#endif
static wifi_event_handle m_scan_result_handle = NULL;
static wifi_event_handle m_scan_done_handle = NULL;
static wifi_event_handle m_join_success_handle = NULL;
static wifi_event_handle m_join_fail_handle = NULL;
static wifi_event_handle m_leave_handle = NULL;

static unsigned int aic_p2p_dev_port = 7236; //default port number

void aic_wifi_event_callback(AIC_WIFI_EVENT enEvent, aic_wifi_event_data *enData)
{
    switch(enEvent)
    {
        case SCAN_RESULT_EVENT:
            {
                //MLOGE("func:%s, SCAN_RESULT_EVENT received\n",__FUNCTION__);
                if (m_scan_result_handle)
                {
                    m_scan_result_handle(enData);
                }
                break;
            }
        case SCAN_DONE_EVENT:
            {
                if (m_scan_done_handle)
                {
                    m_scan_done_handle(enData);
                }
                #if 0
                if (1)
                {
                    wlan_event_msg_t event;
                    //make a fake wlan event
                    event.event_type = WLAN_E_SCAN_COMPLETE;
                    WLAN_SYS_StatusCallback(&event);
                }
                #endif
                break;
            }
        case JOIN_SUCCESS_EVENT:
            {
                if (m_join_success_handle)
                {
                    m_join_success_handle(enData);
                }
                #if 0
                if (1)
                {
                    wlan_event_msg_t event;
                    //make a fake wlan event
                    event.event_type = WLAN_E_LINK;
                    event.flags = 1;
                    WLAN_SYS_StatusCallback(&event);
                }
                #endif
                break;
            }
        case JOIN_FAIL_EVENT:
            {
                if (m_join_fail_handle)
                {
                    m_join_fail_handle(enData);
                }
                #if 0
                if (1)
                {
                    struct resp_evt_result *join_res = (struct resp_evt_result *)enData;
                    wlan_event_msg_t event;
                    event.event_type = WLAN_E_LINK;
                    event.flags = 0;
                    event.reason = join_res->u.join.status_code;
                    WLAN_SYS_StatusCallback(&event);
                }
                #endif
                break;
            }
        case LEAVE_RESULT_EVENT:
            {
                if (m_leave_handle)
                {
                    m_leave_handle(enData);
                }
                #if 0
                if (1)
                {
                    struct resp_evt_result *leave_res = (struct resp_evt_result *)enData;
                    wlan_event_msg_t event;
                    event.event_type = WLAN_E_LINK;
                    event.flags = 0;
                    event.reason = leave_res->u.leave.reason_code;
                    WLAN_SYS_StatusCallback(&event);
                }

                #endif
                break;
            }
        case PRO_DISC_REQ_EVENT:
            {
                uint32_t *mac_addr = enData->data.auth_deauth_data.reserved;
                aic_dbg("PRO_DISC_REQ_EVENT mac_addr = %02x:%02x:%02x:%02x:%02x:%02x\r\n"
                       , mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
                fhost_wpa_execute_cmd(0, NULL, NULL, 300, "WPS_PBC");
                /*
                if (sp_aic_wifi_event_cb) {
                    wifi_drv_event drv_event;
                    struct wifi_p2p_event p2p_event;
                    p2p_event.event_type = WIFI_P2P_EVENT_GOT_PRO_DISC_REQ_AFTER_GONEGO_OK;
                    p2p_event.peer_dev_mac_addr[0] = (unsigned char)enData->data.auth_deauth_data.reserved[0];
                    p2p_event.peer_dev_mac_addr[1] = (unsigned char)enData->data.auth_deauth_data.reserved[1];
                    p2p_event.peer_dev_mac_addr[2] = (unsigned char)enData->data.auth_deauth_data.reserved[2];
                    p2p_event.peer_dev_mac_addr[3] = (unsigned char)enData->data.auth_deauth_data.reserved[3];
                    p2p_event.peer_dev_mac_addr[4] = (unsigned char)enData->data.auth_deauth_data.reserved[4];
                    p2p_event.peer_dev_mac_addr[5] = (unsigned char)enData->data.auth_deauth_data.reserved[5];
                    drv_event.type = WIFI_DRV_EVENT_P2P;
                    drv_event.node.p2p_event = p2p_event;

                    aic_p2p_dev_port = enData->p2p_dev_port_num;
                    sp_aic_wifi_event_cb(&drv_event);
                } */
                break;
            }
        case EAPOL_STA_FIN_EVENT:
            {
                uint32_t *mac_addr = enData->data.auth_deauth_data.reserved;
                aic_dbg("EAPOL_STA_FIN_EVENT mac_addr = %02x:%02x:%02x:%02x:%02x:%02x\r\n"
                       , mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
		/*
                if (sp_aic_wifi_event_cb) {
                    wifi_drv_event drv_event;
                    struct wifi_ap_event ap_event;
                    ap_event.event_type = WIFI_AP_EVENT_ON_ASSOC;
                    drv_event.type = WIFI_DRV_EVENT_AP;
                    ap_event.peer_dev_mac_addr[0] = (unsigned char)enData->data.auth_deauth_data.reserved[0];
                    ap_event.peer_dev_mac_addr[1] = (unsigned char)enData->data.auth_deauth_data.reserved[1];
                    ap_event.peer_dev_mac_addr[2] = (unsigned char)enData->data.auth_deauth_data.reserved[2];
                    ap_event.peer_dev_mac_addr[3] = (unsigned char)enData->data.auth_deauth_data.reserved[3];
                    ap_event.peer_dev_mac_addr[4] = (unsigned char)enData->data.auth_deauth_data.reserved[4];
                    ap_event.peer_dev_mac_addr[5] = (unsigned char)enData->data.auth_deauth_data.reserved[5];
                    drv_event.node.ap_event = ap_event;

                    //diag_dump_buf(ap_event.peer_dev_mac_addr, 6);
                    sp_aic_wifi_event_cb(&drv_event);
                } */
                break;
            }
        case EAPOL_P2P_FIN_EVENT:
            {
                uint32_t *mac_addr = enData->data.auth_deauth_data.reserved;
                aic_dbg("EAPOL_P2P_FIN_EVENT mac_addr = %02x:%02x:%02x:%02x:%02x:%02x\r\n"
                       , mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
				/*
                if (sp_aic_wifi_event_cb) {
                    wifi_drv_event drv_event;
                    struct wifi_p2p_event p2p_event;
                    p2p_event.event_type = WIFI_P2P_EVENT_ON_ASSOC_REQ;
                    p2p_event.peer_dev_port = aic_p2p_dev_port;
                    p2p_event.peer_dev_mac_addr[0] = (unsigned char)enData->data.auth_deauth_data.reserved[0];
                    p2p_event.peer_dev_mac_addr[1] = (unsigned char)enData->data.auth_deauth_data.reserved[1];
                    p2p_event.peer_dev_mac_addr[2] = (unsigned char)enData->data.auth_deauth_data.reserved[2];
                    p2p_event.peer_dev_mac_addr[3] = (unsigned char)enData->data.auth_deauth_data.reserved[3];
                    p2p_event.peer_dev_mac_addr[4] = (unsigned char)enData->data.auth_deauth_data.reserved[4];
                    p2p_event.peer_dev_mac_addr[5] = (unsigned char)enData->data.auth_deauth_data.reserved[5];
                    drv_event.type = WIFI_DRV_EVENT_P2P;
                    drv_event.node.p2p_event = p2p_event;

                    sp_aic_wifi_event_cb(&drv_event);
                }
                break;
            }
        case ASSOC_IND_EVENT:
            {
                uint32_t *mac_addr = enData->data.auth_deauth_data.reserved;
                aic_dbg("ASSOC_IND_EVENT mac_addr = %02x:%02x:%02x:%02x:%02x:%02x\r\n"
                       , mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
                #if 0
                if (1)
                {
                    wlan_event_msg_t event;
                    //make a fake wlan event
                    event.event_type = WLAN_E_ASSOC_IND;
                    event.addr.mac[0] = enData->data.auth_deauth_data.reserved[0];
                    event.addr.mac[1] = enData->data.auth_deauth_data.reserved[1];
                    event.addr.mac[2] = enData->data.auth_deauth_data.reserved[2];
                    event.addr.mac[3] = enData->data.auth_deauth_data.reserved[3];
                    event.addr.mac[4] = enData->data.auth_deauth_data.reserved[4];
                    event.addr.mac[5] = enData->data.auth_deauth_data.reserved[5];
                    WLAN_SYS_StatusCallback(&event);
                }
                #endif
                break;
            }
        case STA_DISCONNECT_EVENT:
            {
                AIC_WIFI_MODE mode = aic_wifi_get_mode();
                aic_dbg("STA_DISCONNECT_EVENT, current mode:%d\r\n", mode);
                if (sp_aic_wifi_event_cb &&  mode == WIFI_MODE_STA) {
                    wifi_drv_event drv_event;
                    struct wifi_sta_event dev_event;
                    dev_event.event_type = WIFI_STA_EVENT_ON_DISASSOC;
                    drv_event.type = WIFI_DRV_EVENT_STA;
                    drv_event.node.sta_event = dev_event;
                    sp_aic_wifi_event_cb(&drv_event);
                }
                break;
            }
        case DISASSOC_STA_IND_EVENT:
            {
                uint32_t *mac_addr = enData->data.auth_deauth_data.reserved;
                aic_dbg("DISASSOC_STA_IND_EVENT mac_addr = %02x:%02x:%02x:%02x:%02x:%02x\r\n"
                       , mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
                #if 0
                if (1)
                {
                    wlan_event_msg_t event;
                    //make a fake wlan event
                    event.event_type = WLAN_E_DISASSOC_IND;
                    event.addr.mac[0] = enData->data.auth_deauth_data.reserved[0];
                    event.addr.mac[1] = enData->data.auth_deauth_data.reserved[1];
                    event.addr.mac[2] = enData->data.auth_deauth_data.reserved[2];
                    event.addr.mac[3] = enData->data.auth_deauth_data.reserved[3];
                    event.addr.mac[4] = enData->data.auth_deauth_data.reserved[4];
                    event.addr.mac[5] = enData->data.auth_deauth_data.reserved[5];
                    WLAN_SYS_StatusCallback(&event);
                }
                #endif
		/*
                if(sp_aic_wifi_event_cb) {
                    wifi_drv_event drv_event;
                    struct wifi_ap_event ap_event;
                    ap_event.event_type = WIFI_AP_EVENT_ON_DISASSOC;
                    ap_event.peer_dev_mac_addr[0] = (unsigned char)enData->data.auth_deauth_data.reserved[0];
                    ap_event.peer_dev_mac_addr[1] = (unsigned char)enData->data.auth_deauth_data.reserved[1];
                    ap_event.peer_dev_mac_addr[2] = (unsigned char)enData->data.auth_deauth_data.reserved[2];
                    ap_event.peer_dev_mac_addr[3] = (unsigned char)enData->data.auth_deauth_data.reserved[3];
                    ap_event.peer_dev_mac_addr[4] = (unsigned char)enData->data.auth_deauth_data.reserved[4];
                    ap_event.peer_dev_mac_addr[5] = (unsigned char)enData->data.auth_deauth_data.reserved[5];
                    drv_event.type = WIFI_DRV_EVENT_AP;
                    drv_event.node.ap_event = ap_event;

                    sp_aic_wifi_event_cb(&drv_event);
                }*/
                break;
            }
        case DISASSOC_P2P_IND_EVENT:
            {
                uint32_t *mac_addr = enData->data.auth_deauth_data.reserved;
                aic_dbg("DISASSOC_P2P_IND_EVENT mac_addr = %02x:%02x:%02x:%02x:%02x:%02x\r\n"
                       , mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
                #if 0
                if (1)
                {
                    wlan_event_msg_t event;
                    //make a fake wlan event
                    event.event_type = WLAN_E_DISASSOC_IND;
                    event.addr.mac[0] = enData->data.auth_deauth_data.reserved[0];
                    event.addr.mac[1] = enData->data.auth_deauth_data.reserved[1];
                    event.addr.mac[2] = enData->data.auth_deauth_data.reserved[2];
                    event.addr.mac[3] = enData->data.auth_deauth_data.reserved[3];
                    event.addr.mac[4] = enData->data.auth_deauth_data.reserved[4];
                    event.addr.mac[5] = enData->data.auth_deauth_data.reserved[5];
                    WLAN_SYS_StatusCallback(&event);
                }
                #endif
		/*
                if(sp_aic_wifi_event_cb) {
                    wifi_drv_event drv_event;
                    struct wifi_p2p_event p2p_event;
                    p2p_event.event_type = WIFI_P2P_EVENT_ON_DISASSOC;
                    p2p_event.peer_dev_mac_addr[0] = (unsigned char)enData->data.auth_deauth_data.reserved[0];
                    p2p_event.peer_dev_mac_addr[1] = (unsigned char)enData->data.auth_deauth_data.reserved[1];
                    p2p_event.peer_dev_mac_addr[2] = (unsigned char)enData->data.auth_deauth_data.reserved[2];
                    p2p_event.peer_dev_mac_addr[3] = (unsigned char)enData->data.auth_deauth_data.reserved[3];
                    p2p_event.peer_dev_mac_addr[4] = (unsigned char)enData->data.auth_deauth_data.reserved[4];
                    p2p_event.peer_dev_mac_addr[5] = (unsigned char)enData->data.auth_deauth_data.reserved[5];
                    drv_event.type = WIFI_DRV_EVENT_P2P;
                    drv_event.node.p2p_event = p2p_event;

                    sp_aic_wifi_event_cb(&drv_event);
                } */
                break;
            }
        default:
            {
                break;
            }
    }
}

int aic_wifi_init_mac()
{
    int ret = 0;

    unsigned char mac_addr[6] = {0x88, 0x00, 0x33, 0x77, 0x69, 0x22};
    unsigned int mac_local[6] = {0};

    /* use chip info as MAC in now(By Sunplus),
     * TODO: the MAC value read from efuse will be modified later(By AIC company).
     */

    set_mac_address(mac_addr);
       return 0;
}

int aic_wifi_init(int mode, int chip_id, void *param)
{
    int ret = 0;
    unsigned char mac_addr[6] = {0x88, 0x00, 0x33, 0x77, 0x69, 0x22};
    unsigned int mac_local[6] = {0};

    AIC_LOG_PRINTF("aic_wifi_init, mode=%d\r\n", mode);
    AIC_LOG_PRINTF("release version:%s\r\n", AICWF_RELEASE_VERSION);

    #if (CONFIG_RTOS_AL_TEST_EN)
    rtos_al_test();
    #endif

#if 0
    if (g_wifi_init == 1)
    {
        AIC_LOG_PRINTF("aic_wifi_init already init\r\n");
        return g_rwnx_hw->net_id;
    }
#endif

    //set_mac_address(mac_addr);
    ret = aic_wifi_open(mode, param, chip_id);
    if (ret) {
        AIC_LOG_PRINTF("wifi_open fail, ret=%d\n", ret);
        return -1;
    }

    aic_wifi_event_register(aic_wifi_event_callback);

    #if (CONFIG_TEST_MAIN_EN)
    test_main_entry();
    #endif
    AIC_LOG_PRINTF("aic_wifi_init ok\r\n");
/*
retry:
    if(sp_aic_wifi_event_cb) {
        wifi_drv_event drv_event;
        struct wifi_dev_event dev_event;
        dev_event.drv_status = WIFI_DEVICE_DRIVER_LOADED;
        dev_event.local_mac_addr[0] = mac_addr[0];
        dev_event.local_mac_addr[1] = mac_addr[1];
        dev_event.local_mac_addr[2] = mac_addr[2];
        dev_event.local_mac_addr[3] = mac_addr[3];
        dev_event.local_mac_addr[4] = mac_addr[4];
        dev_event.local_mac_addr[5] = mac_addr[5];
        drv_event.type = WIFI_DRV_EVENT_NET_DEVICE;
        drv_event.node.dev_event = dev_event;
        sp_aic_wifi_event_cb(&drv_event);
    } else {
        rtos_msleep(5);
        goto retry;
    } */
    return g_rwnx_hw->net_id;
}

int wifi_off() {
	return 0;
}

int wifi_on() {
	int fhost_vif_idx = 0;
    printf("AIC wifi_on\n");
	ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_ADD_NETIF, fhost_to_net_if(fhost_vif_idx));

	return 0;
}

void aic_wifi_deinit(int mode)
{
    AIC_LOG_PRINTF("aic_wifi_deinit, mode=%d\r\n", mode);

    if (g_wifi_init == 1)
    {
       aic_wifi_close(mode);

       aic_wifi_event_register(NULL);
    }
    /*
    if(sp_aic_wifi_event_cb) {
        wifi_drv_event drv_event;
        struct wifi_dev_event dev_event;
        dev_event.drv_status = WIFI_DEVICE_DRIVER_UNLOAD;
        drv_event.type = WIFI_DRV_EVENT_NET_DEVICE;
        drv_event.node.dev_event = dev_event;
        sp_aic_wifi_event_cb(&drv_event);
    } */
    g_wifi_init = 0;

    AIC_LOG_PRINTF("aic_wifi_deinit ok\r\n");
}

int hostapd_user_wps_button_pushed(void* handle, const unsigned char *p2p_dev_addr)
{
    int fhost_vif_idx = 0;
    fhost_wpa_execute_cmd(fhost_vif_idx, NULL, NULL, 300, "WPS_PBC");
    return 0;
}

extern uint8_t p2p_started;
int user_p2p_start(int cmd_flag)
{

    #define P2P_SSID_STRING  "DIRECT-AIC-P2P"
    #define AP_PASS_STRING   "12345678"
    struct aic_p2p_cfg user_p2p_cfg = {
        .aic_p2p_ssid = {
              strlen(P2P_SSID_STRING),
                P2P_SSID_STRING
        },
        .aic_ap_passwd = {
                strlen(AP_PASS_STRING),
                AP_PASS_STRING
         },
        .band = 1,
        .type = PHY_CHNL_BW_20,
        //.channel = 11,
        .channel = 36,
        .enable_he = 1,
        .enable_acs = 0,
    };
    if (!p2p_started) {
        g_rwnx_hw->net_id = wlan_start_p2p(&user_p2p_cfg);
    }

//#if 0
    if (cmd_flag) {
        int fhost_vif_idx = 0;
        char set_p2p_go_cmd[64];
        strcpy(set_p2p_go_cmd, "P2P_GROUP_ADD he ");
        strcat(set_p2p_go_cmd, "pass=");
        strcat(set_p2p_go_cmd, AP_PASS_STRING);
        strcat(set_p2p_go_cmd, " freq=");
        char freq[5];
        uint16_t prim20_freq;
        memset(freq, 0, sizeof(freq));
        if ((user_p2p_cfg.band == 0) && (user_p2p_cfg.channel == 0)) {
            prim20_freq = phy_channel_to_freq(PHY_BAND_2G4, 11);
        } else {
            prim20_freq = phy_channel_to_freq(user_p2p_cfg.band, user_p2p_cfg.channel);
        }
        sprintf(freq, "%d", prim20_freq);
        strcat(set_p2p_go_cmd, freq);
        strcat(set_p2p_go_cmd," persistent");
        printf("set_p2p_go_cmd as %s\n", set_p2p_go_cmd);

        fhost_wpa_execute_cmd(fhost_vif_idx, NULL, NULL, 300, "WFD_SUBELEM_SET 0 000600111C440032");
        int res = (fhost_wpa_execute_cmd(fhost_vif_idx, NULL, NULL, -1, set_p2p_go_cmd) |
            fhost_wpa_enable_network(fhost_vif_idx, 20000));
        }else {
            printf("No need to start p2p go for now\n");
        }
//#endif

    aic_wifi_set_mode(WIFI_MODE_P2P);
    return g_rwnx_hw->net_id;
}

int user_p2p_setDN(const char* device_name)
{
    int fhost_vif_idx = 0;
    char set_p2p_dev_name_cmd[64];
    char set_ap_dev_name_cmd[64];
    strcpy(set_p2p_dev_name_cmd, "SET p2p_dev_name ");
    strcpy(set_ap_dev_name_cmd, "P2P_SET ssid_postfix ");
    strcat(set_p2p_dev_name_cmd, device_name);
    strcat(set_ap_dev_name_cmd, device_name);
    fhost_wpa_execute_cmd(fhost_vif_idx, NULL, NULL, 300, set_p2p_dev_name_cmd);
    fhost_wpa_execute_cmd(fhost_vif_idx, NULL, NULL, 300, set_ap_dev_name_cmd);
    return 0;
}

int user_set_wfd_type(int wfd_device_type)
{
    int fhost_vif_idx = 0;
    if (wfd_device_type)
        fhost_wpa_execute_cmd(fhost_vif_idx, NULL, NULL, 300, "WFD_SUBELEM_SET 0 000600111C440032");
    else
        fhost_wpa_execute_cmd(fhost_vif_idx, NULL, NULL, 300, "WFD_SUBELEM_SET 0 000600101C440032");
    return 0;
}

int user_set_wfd_enable(int wfd_device_enable)
{
    int fhost_vif_idx = 0;
    if (wfd_device_enable)
        fhost_wpa_execute_cmd(fhost_vif_idx, NULL, NULL, 300, "SET wifi_display 1");
    else
        fhost_wpa_execute_cmd(fhost_vif_idx, NULL, NULL, 300, "SET wifi_display 0");
    return 0;
}

int wpa_sta_disconnect()
{
    int ret = 0;
    ret = wlan_disconnect_sta(0);
    if(ret < 0)
        printf("disconnect sta fail\n");

    return ret;
}

int wpa_sta_connect(const char *ssid, const char *passwd, int timeout_ms)
{
    return wlan_start_sta(ssid, passwd, timeout_ms);
}

int wpa_stop_ap()
{
    int ret = 0;
    ret = wlan_stop_ap();
    if(ret < 0)
        printf("stop ap fail\n");

    return ret;
}

void aic_wifi_switch (unsigned int mode) {
    if (mode == 1) {         //AP Mode
        printf("aic wifi mode switch to STA\n");
        wpa_stop_ap();        
        sleep(2);
    } else if (mode == 0) { //STA Mode     
        printf("aic wifi mode switch to AP\n");
        wpa_sta_disconnect();
        sleep(2);
    } else {
        printf("Unknown mode to switch\n");
    }
}

