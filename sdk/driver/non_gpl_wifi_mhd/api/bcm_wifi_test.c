#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include "openrtos/FreeRTOS.h"
#include "openrtos/task.h"
#include "lwip/tcpip.h"
#include "ith/ith_gpio.h"
#include "ite/ite_sd.h"
#include "wifi_nvram_image.h"
#include "mhd_api.h"

char *wifi_nvram_ptr = (char *)wifi_nvram_image;
int wifi_nvram_size = (int)sizeof(wifi_nvram_image);

//#define MHD_FW_DEBUG

/*=====================WIFI=====================*/
#define WIFI_OOB_PIN_NUM         (CFG_GPIO_SD_WIFI_WAKEUP_PIN < 0) ? (0) : CFG_GPIO_SD_WIFI_WAKEUP_PIN //WL_HOST_WAKE
#define WIFI_WLREG_ON_PIN_NUM    (CFG_GPIO_SD_WIFI_WL_REG_PIN < 0) ? (0) : CFG_GPIO_SD_WIFI_WL_REG_PIN //WIFI_EN_GPIO

extern int wifi_api_test(int argc, char **argv);
extern const char *get_security_string_by_index(uint8_t security);
extern int lwip_ping(int argc, char **argv);
extern void lwip_iperf_tcp(int argc, char **argv);
extern void lwip_iperf_udp(int argc, char **argv);
extern int mhd_gpio_regon_register();
extern int mhd_gpio_oob_register();
#ifdef MHD_FW_DEBUG
extern int mhd_config_fwlog( uint8_t enable, uint32_t interval );
#endif

//TEST_MODE
#define SOFTAP_TEST_MODE			0
#define STA_TEST_MODE				1
#define STA_LOOP_TEST_MODE			2
#define STA_IPERF_TEST_MODE			3
#define STA_SCAN_TEST_MODE			4
#define MODULE_INIT_EXIT_STA_LOOP_TEST_MODE			5

//IPERF_TEST_MODE
#define IPERF_TCP_CLIENT_TEST		0
#define IPERF_TCP_SERVER_TEST		1
#define IPERF_UDP_CLIENT_TEST		2
#define IPERF_UDP_SERVER_TEST		3

#define TEST_MODE					STA_IPERF_TEST_MODE //SOFTAP_TEST_MODE
#define IPERF_TEST_MODE				IPERF_TCP_SERVER_TEST	

#if (TEST_MODE == SOFTAP_TEST_MODE)	
//Softap settings
char softap_ssid[] = "iteAP";
char softap_passwd[] = "12345678";
char *softap_security = "2";		//0-open, 1-wpa_psk_aes, 2-wpa2_psk_aes
char *softap_channel = "12";
char client_IP_Addr[] = "192.168.1.2";	
#else
//Sta settings
char sta_ssid[] = "TOTOLINK";
char sta_passwd[] = "12345678";
char *sta_security = "2";			//0-open, 1-wpa_psk_aes, 2-wpa2_psk_aes
char *sta_channel = "11";
int max_test_count = 300000;				//For STA_LOOP_TEST_MODE, STA_SCAN_TEST_MODE, MODULE_INIT_EXIT_STA_LOOP_TEST_MODE
#endif
	
char iperf_PC_IP_Addr[] = "192.168.11.4";
int argc;
char *p;

static int nMHD_P2P = 0;

void osal_host_rtos_get_mhdtask_settings( uint32_t *priority, uint32_t *stack_size )
{
#if defined(CFG_IPERF_ENABLE)
    *priority = 2;
#else
    *priority = 2; 
#endif
    
    *stack_size = (80000);
}


// 0: client mode , 1 : ap mode
int nWifiMode = 0;
void wifi_drv_on(int nSD)
{
	int ret;

    printf("--------- [wifi_drv_on] start\n");
    tcpip_init(NULL, NULL);
#if       !defined(CFG_NET_WIFI_SDIO_NGPL_SYN4345x) && !defined(CFG_NET_WIFI_SDIO_NGPL_SYN4343x)
    mhd_gpio_regon_register(WIFI_WLREG_ON_PIN_NUM); //Useless with daughter board of ITE(Always pull high in HW)
#endif
    //mhd_gpio_oob_register(WIFI_OOB_PIN_NUM, ITH_GPIO_PULL_UP);
    //trigger_mode: 0, LEVEL TRIGGER; 1,EDGE TRIGGER
    //mhd_gpio_oob_register(WIFI_OOB_PIN_NUM, trigger_mode);
#if       defined(CFG_NET_WIFI_SDIO_NGPL_SYN4345x) || defined(CFG_NET_WIFI_SDIO_NGPL_SYN4343x)
    mhd_gpio_oob_register(WIFI_OOB_PIN_NUM, ITH_GPIO_INTR_LEVELTRIGGER);
#else
    mhd_gpio_oob_register(WIFI_OOB_PIN_NUM, 0);
#endif
    #ifdef CFG_NET_WIFI_SDIO_NGPL_AP6256
        mhd_set_country_code(MK_CNTRY( 'C', 'N', 38 )); 
    #endif

    #if defined (CFG_NET_WIFI_SDIO_NGPL_AP6203)  || (CFG_NET_WIFI_SDIO_NGPL_AP6212)
        mhd_set_country_code(MK_CNTRY( 'C', 'N', 0 )); 
    #endif

#if       defined(CFG_NET_WIFI_SDIO_NGPL_SYN4345x) || defined(CFG_NET_WIFI_SDIO_NGPL_SYN4343x)
    mhd_set_country_code(MK_CNTRY( 'C', 'N', 38 ));
#endif

    //mhd_set_country_code(MK_CNTRY( 'C', 'N', 0 ));
    //mhd_set_country_code(MK_CNTRY( 'J', 'P', 3 ));
    if (nSD == 0)
        mhd_sdio_controller_index_register( SD_0 );
    else if(nSD == 1)
        mhd_sdio_controller_index_register( SD_1 );

    if (nMHD_P2P == 1){
        ret = mhd_module_init(MHD_P2P_MODE);
    } else if (nMHD_P2P == 0) {
        ret = mhd_module_init(MHD_STA_AP_MODE);
    }

#ifdef MHD_FW_DEBUG   
    mhd_config_fwlog( 1, 500 );
#endif
    printf("------------------ BUILD : %s %s SD%d WL_PIN[%d] ------------------\n",
        __DATE__, __TIME__, nSD, WIFI_OOB_PIN_NUM);

    if(ret)
        return;

    sleep(1);   

    #ifdef CFG_NET_WIFI_SDIO_NGPL_AP6256
	mhd_get_ampdu_hostreorder_support();
	mhd_set_ampdu_hostreorder_support(false);
    #endif

    #ifdef CFG_NET_WIFI_SDIO_NGPL_AP6212
	mhd_get_ampdu_hostreorder_support();
	mhd_set_ampdu_hostreorder_support(false);
    #endif

    
    #ifdef CFG_NET_WIFI_SDIO_NGPL_AP6203
	mhd_get_ampdu_hostreorder_support();
	mhd_set_ampdu_hostreorder_support(false);        
    #else
        //mhd_set_wifi_11n_support(false);
    #endif


    #if defined (CFG_NET_LWIP_2) && (CFG_NET_WIFI_SDIO_MHD_P2P)
    if (nMHD_P2P == 1){

        printf("====== start p2p service =====\n");
        //====== start p2p service ======

        mhd_get_valid_channels();
        printf("====== mhd_get_valid_channels =====\n");    

        //Step1: Set channel for p2p service
        //void mhd_set_channel_for_P2P( uint channel );
        mhd_set_channel_for_P2P( 36 );
        printf("====== mhd_set_channel_for_P2P =====\n");        


        //Step2: Set device name/ ssid / password for p2p
        //int mhd_start_P2P_service( char *device_name, char *ssid, char *password );
        ret = mhd_start_P2P_service( "dev_mhd_p2p", "DIRECT-MHD-P2P", "87654321" );
        printf("====== mhd_start_P2P_service =====\n");        

        if(ret)
        return;

        //Step3: 
        //int mhd_p2p_network_up( uint32_t ip, uint32_t gateway, uint32_t netmask )
        ret = mhd_p2p_network_up(0xc0a80101, 0xc0a80101, 0xffffff00);   

        printf("====== mhd_p2p_network_up =====\n");

        if(ret)
        return;

        //Step4: 
        //start dhcpd server
        dhcps_init();
    }
    
    #endif

    
}

void wifi_on(void)
{
    if (nMHD_P2P == 1){
        mhd_module_init(MHD_P2P_MODE);
    } else if (nMHD_P2P == 0) {
        mhd_module_init(MHD_STA_AP_MODE);
    }
}

void wifi_off(void)
{

  if(nWifiMode == 0) 
  {
      mhd_sta_disconnect( 1 );
      mhd_sta_network_down(); 
      sleep(1);

  } else if(nWifiMode ==1) {
      //mhd_softap_stop_dhcpd();
      mhd_softap_stop( 1 );   
      sleep(1);      
  }
  mhd_module_exit();

}
void wifi_switch(void)
{

  if(nWifiMode == 0) 
  {
      mhd_sta_disconnect( 1 );
      mhd_sta_network_down(); 
      sleep(1);

  } else if(nWifiMode ==1) {
      //mhd_softap_stop_dhcpd();
      mhd_softap_stop( 1 );   
      sleep(1);      
  }
}

// 1: enable 0: disable , default disable
void wifi_enable_p2p(int nP2p)
{
    printf("wifi_enable_p2p %d \n",nMHD_P2P);

    if (nP2p ==1){
        nMHD_P2P = 1;
    } else if (nP2p == 0) {
        nMHD_P2P = 0;   
    }


}

// 0: client mode , 1 : ap mode
void wifi_set_mode(int nMode)
{
    if (nMode ==0)
        nWifiMode = nMode;
    else if (nMode ==1)
        nWifiMode = nMode;

}

void bcm_drv_init(void) 
{
	int ret;
	tcpip_init(NULL, NULL);
	mhd_gpio_regon_register(WIFI_WLREG_ON_PIN_NUM);
	mhd_gpio_oob_register(WIFI_OOB_PIN_NUM, ITH_GPIO_PULL_UP);
	//mhd_set_country_code(MK_CNTRY( 'C', 'N', 0 ));
	mhd_set_country_code(MK_CNTRY( 'J', 'P', 3 ));
	
	mhd_sdio_controller_index_register( SD_0 );
	
	ret = mhd_module_init(MHD_STA_AP_MODE);  
	printf("------------------ BUILD : %s %s ------------------\n", __DATE__, __TIME__);
	
	if(ret)
		return;

	sleep(1);	

#if (TEST_MODE == STA_SCAN_TEST_MODE)
	for(int i=0; i<max_test_count; i++) 
	{
		argc = 2;	
		char *argv[] = {"api", "2"};
		wifi_api_test(argc, argv);	
		sleep(1);
	}
#else
#if (TEST_MODE == SOFTAP_TEST_MODE)	
	//--------- softap mode testing  --------- 		
	int max_delay = 60;
	argc = 6;
	//softap start
	char *argv[] = {"api", "6", softap_ssid, softap_passwd, softap_security, softap_channel};
	printf("SSDI = %s\nPassword = %s\nSecurity = %s\nChannel = %s\n", argv[2], argv[3], get_security_string_by_index(strtol(argv[4], &p, 10)), argv[5]);
	wifi_api_test(argc, argv);

	for(int i=0; i<max_delay; i++)
	{
		printf("delay %d sec (%d)\n", i+1, max_delay);
		sleep(1);
	}
	//ping test
	char *ping_argv[] = {"ping", client_IP_Addr};
	lwip_ping(argc, ping_argv);

	//get mac list
	argv[1] = "8";		
	wifi_api_test(argc, argv);
	
	sleep(1);	
	//softap stop
	argv[1] = "7";		
	wifi_api_test(argc, argv);	

	//------- softap mode testing end -------- 
#else 
	//---------   sta mode testing   --------- 
	argc = 6;	
	char *argv[] = {"api", "0", sta_ssid, sta_passwd, sta_security, sta_channel};
	printf("SSDI = %s\nPassword = %s\nSecurity = %s\nChannel = %s\n\n", argv[2], argv[3], get_security_string_by_index(strtol(argv[4], &p, 10)), argv[5]);
	
#if (TEST_MODE == STA_TEST_MODE) || (TEST_MODE == STA_IPERF_TEST_MODE)
		//connect
		wifi_api_test(argc, argv);
		
		//get rssi
		argv[1] = "3";
		wifi_api_test(argc, argv);		
#else	
	for(int i=0; i<max_test_count; i++) 
	{
		printf("----- sta loop test %d (Max: %d) -----\n", i+1, max_test_count);
		argc = 6;
		argv[1] = "0";
		wifi_api_test(argc, argv);

		argc = 2;
		//ping test
		char *ping_argv[] = {"ping", iperf_PC_IP_Addr};
		lwip_ping(argc, ping_argv);
		//get rssi
		argv[1] = "3";
		wifi_api_test(argc, argv);
		//get rate
		argv[1] = "4";
		wifi_api_test(argc, argv);
		//get mac addr
		argv[1] = "5";
		wifi_api_test(argc, argv);
		//scan
		argv[1] = "2";
		wifi_api_test(argc, argv);
		//ping test
		lwip_ping(argc, ping_argv);
#if (TEST_MODE == STA_LOOP_TEST_MODE)
		//disconnect
		argv[1] = "1";
		wifi_api_test(argc, argv);
		sleep(1);
#else
	/* MODULE_INIT_EXIT_STA_LOOP_TEST_MODE */
		//module exit
		argv[1] = "9";
		wifi_api_test(argc, argv);
		
		//module init
		argv[1] = "10";
		ret = wifi_api_test(argc, argv);		
		if(ret)
			return;
		
		sleep(1);	
#endif
	}		
#endif	//#if (TEST_MODE == STA_TEST_MODE) || (TEST_MODE == STA_IPERF_TEST_MODE)
	//--------- sta mode testing end --------- 
#endif

#if (TEST_MODE == STA_IPERF_TEST_MODE)
	//---------    iperf testing     --------- 
#if (IPERF_TEST_MODE == IPERF_TCP_CLIENT_TEST) || (IPERF_TEST_MODE == IPERF_TCP_SERVER_TEST)
	/*	iperf TCP testing  */
#if (IPERF_TEST_MODE == IPERF_TCP_CLIENT_TEST)			
	/*	iperf TCP client testing  */
	char *iperf_argv[] = {"iperf_tcp", "-c", iperf_PC_IP_Addr, "-t", "60", "-i", "60"};
	argc = 7;
	lwip_iperf_tcp(argc, iperf_argv);
#else
	/*	iperf TCP server testing  */
	char *iperf_argv[] = {"iperf_tcp", "-s", "-i", "5"};
	argc = 4;
	lwip_iperf_tcp(argc, iperf_argv);
#endif
#else
	/*	iperf UDP testing  */		
#if (IPERF_TEST_MODE == IPERF_UDP_CLIENT_TEST)	
	/*	iperf UDP client testing  */
	char *iperf_argv[] = {"iperf_udp", "-c", iperf_PC_IP_Addr, "-t", "60", "-i", "60", "-b", "40M"};
	argc = 9;
	lwip_iperf_udp(argc, iperf_argv);
#else
	/*	iperf UDP server testing  */
	char *iperf_argv[] = {"iperf_udp", "-s", "-i", "5"};
	argc = 4;
	lwip_iperf_udp(argc, iperf_argv);
#endif	
#endif
	//---------   iperf testing end  --------- 
#endif	//#if (TEST_MODE == STA_IPERF_TEST_MODE)
#endif	//#if (TEST_MODE == STA_SCAN_TEST_MODE)
	printf(">>>>>>>> ok\n");
	while(1)
	{
		sleep(1);
	}	
}


