/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
//#include <FreeRTOS.h>
//#include <device.h>
//#include <stdbool.h>
//#include <errno.h>
//#include <stddef.h>
//#include <mmc/sdio.h>
#include "stdio.h"
#include "stdlib.h"
#include "lwip/tcpip.h"
#include "mhd_api.h"
#include "host_constants.h"
//#include "dhcps.h"

unsigned int service_state = 0;

#define state_sta_connect			0x01
#define state_softap_start			0x02

extern void dhcps_init(void);

const char *get_security_string_by_index(uint8_t security)
{
    return ( security == MHD_SECURE_OPEN ) ? "OPEN":
           ( security == MHD_WPA_PSK_AES ) ? "WPA PSK TKIP":
           ( security == MHD_WPA2_PSK_AES ) ? "WPA2 PSK AES":
           ( security == MHD_WEP_OPEN ) ? "WEP OPEN":
           ( security == MHD_WEP_SHARED ) ? "WEP_SHARED":
           ( security == MHD_WPA_PSK_TKIP ) ? "WPA PSK TKIP":
           ( security == MHD_WPA_PSK_MIXED ) ? "WPA PSK MIXED":
           ( security == MHD_WPA2_PSK_TKIP ) ? "WPA2 PSK TKIP":
           ( security == MHD_WPA2_PSK_MIXED ) ? "WPA2 PSK MIXED":
           ( security == MHD_WPA2_PSK_SHA256 ) ? "WPA2 PSK SHA256":
		   ( security == MHD_WPA3_PSK_SAE ) ? "STR WPA3 PSK SAE":
           /*( security == MHD_WPA3_WPA2_PSK ) ? "WPA3 WPA2 PSK":*/
           ( security == MHD_IBSS_OPEN ) ? "ADHOC" :
           ( security == MHD_WPS_OPEN ) ? "ADHOC" :
           ( security == MHD_WPS_AES ) ? "WPS AES" :
           ( security == MHD_WPA_ENT_TKIP ) ? "WPA ENT TKIP" :
           ( security == MHD_WPA_ENT_AES ) ? "WPA ENT AES" :
           ( security == MHD_WPA_ENT_MIXED ) ? "WPA ENT MIXED" :
           ( security == MHD_WPA2_ENT_AES ) ? "WPA2 ENT AES" :
           ( security == MHD_WPA2_ENT_TKIP ) ? "WPA2 ENT TKIP" :
           ( security == MHD_WPA2_ENT_MIXED ) ? "WPA2 ENT MIXED" :
           "UNKNOWN";
}

const char *get_security_string_by_index_for_softap(uint8_t security)
{
    return ( security == MHD_AP_OPEN ) ? "OPEN":
           ( security == MHD_AP_WPA_AES_PSK ) ? "WPA PSK TKIP":
           ( security == MHD_AP_WPA2_AES_PSK ) ? "WPA2 PSK AES":
           ( security == MHD_AP_WEP_OPEN ) ? "WEP OPEN":
           ( security == MHD_AP_WEP_SHARED ) ? "WEP_SHARED":
           ( security == MHD_AP_WPA_TKIP_PSK ) ? "WPA PSK TKIP":
           ( security == MHD_AP_WPA_MIXED_PSK ) ? "WPA PSK MIXED":
           ( security == MHD_AP_WPA2_TKIP_PSK ) ? "WPA2 PSK TKIP":
           ( security == MHD_AP_WPA2_MIXED_PSK ) ? "WPA2 PSK MIXED":
		   ( security == MHD_AP_WPA3_SAE ) ? "STR WPA3 PSK SAE":
           ( security == MHD_AP_WPA3_WPA2_PSK ) ? "WPA3 WPA2 PSK":
           ( security == MHD_AP_WPS_OPEN ) ? "WPS OPEN" :
           ( security == MHD_AP_WPS_AES ) ? "WPS AES" :
           "UNKNOWN";
}

static void print_scan_results( mhd_ap_info_t *results, int num )
{
    int k;
    mhd_ap_info_t *record;

    record = &results[0];
    for ( k = 0; k < num; k++, record++ )
    {
        /* Print SSID */
        printf("\n[%03d]\n", k+1 );
#if       defined(CFG_NET_WIFI_SDIO_NGPL_SYN4345x) || defined(CFG_NET_WIFI_SDIO_NGPL_SYN4343x)
        printf("       SSID          : %s\n", record->ssid.value );
#else
        printf("       SSID          : %s\n", record->ssid );
#endif
        /* Print other network characteristics */
        printf("       BSSID         : %02X:%02X:%02X:%02X:%02X:%02X\n",
#if       defined(CFG_NET_WIFI_SDIO_NGPL_SYN4345x) || defined(CFG_NET_WIFI_SDIO_NGPL_SYN4343x)
                                                  (uint8_t)record->bssid.octet[0], (uint8_t)record->bssid.octet[1], (uint8_t)record->bssid.octet[2],
                                                  (uint8_t)record->bssid.octet[3], (uint8_t)record->bssid.octet[4], (uint8_t)record->bssid.octet[5] );
#else
                                                  (uint8_t)record->bssid[0], (uint8_t)record->bssid[1], (uint8_t)record->bssid[2],
                                                  (uint8_t)record->bssid[3], (uint8_t)record->bssid[4], (uint8_t)record->bssid[5] );
#endif
        printf("       RSSI          : %ddBm\n", (int)record->rssi );
		printf("       Security value: %d\n", record->security);
        printf("       Security      : %s\n", get_security_string_by_index(record->security));
        printf("       Channel       : %d\n", (int)record->channel );
    }
}

#define FORMAT_IPADDR(x) ((unsigned char *)&x)[3], ((unsigned char *)&x)[2], ((unsigned char *)&x)[1], ((unsigned char *)&x)[0]

int wifi_api_test(int argc, char **argv)
{
    uint32_t index;
    char *p;
    uint8_t security = 2;
	mhd_mac_t mac;
    int ret, value, maxTestCount;
    mhd_mac_t mac_list[8];
    uint32_t count = 8;
	uint32_t ip, gateway, netmask;	

    if (argc == 1)
    {
        printf("Please input: api <index>\n");
    } else {
		index = strtol(argv[1], &p, 10);
        switch(index) 
        {
          case 0:	//sta connect
			if (argc == 6) {
				//CMD: api 0 [SSID] [Password] [Security] [Channel]
				if(mhd_sta_connect(argv[2], NULL, strtol(argv[4], &p, 10), argv[3], strtol(argv[5], &p, 10) )) 
				{
					return -1;
				}	  		
			} else if (argc == 4) {
				//CMD: api 0 [SSID] [Password]
				if(mhd_sta_connect(argv[2], NULL, security, argv[3], 4 )) 
				{
					return -1;
				}	  		
			} else if (argc == 3) {
				security = 0;
				//CMD: api 0 [SSID]
				if(mhd_sta_connect(argv[2], NULL, security, NULL, 4 )) 
				{
					return -1;
				}	  		
			} else {				
				//CMD: api 0
				if(mhd_sta_connect("Buffalo-G-43D0", NULL, security, "3w8xmxu3sryyv", 4 )) 
				{
					return -1;
				}	  
			}
			mhd_sta_network_up(0, 0, 0);
			
			ip = htonl(mhd_sta_ipv4_ipaddr());
			printf("Ip addr: %u.%u.%u.%u\n", FORMAT_IPADDR(ip));
			netmask = htonl(mhd_sta_ipv4_netmask());
			printf("netmask: %u.%u.%u.%u\n", FORMAT_IPADDR(netmask));
			gateway = htonl(mhd_sta_ipv4_gateway());
			printf("gateway: %u.%u.%u.%u\n", FORMAT_IPADDR(gateway));				
			service_state |= state_sta_connect;
            break;
          case 1:	//sta disconnect  
			//CMD: api 1
			mhd_sta_disconnect( 1 );
			mhd_sta_network_down();
			service_state &= ~state_sta_connect;
            break;
		  case 2:  	//scan
			//CMD: api 2
			mhd_start_scan();

			mhd_ap_info_t results[50];
			int num = sizeof(results)/sizeof(mhd_ap_info_t);
			mhd_get_scan_results(results, &num);
			print_scan_results(results, num);			
            break;	
          case 3:	//get rssi
			//CMD: api 3
			value = mhd_sta_get_rssi();
			printf("RSSI = %d\n", value);
            break;
          case 4:	//get rate
			//CMD: api 4
			value = mhd_sta_get_rate();
			printf("rate = %d\n", value);
            break;					
		  case 5:	//get mac address
#if       !defined(CFG_NET_WIFI_SDIO_NGPL_SYN4345x) && !defined(CFG_NET_WIFI_SDIO_NGPL_SYN4343x)
			//CMD: api 5
	        if ( mhd_sta_get_mac_address(&mac) )
    	    {
				printf("failed to get mac address\n");
				return -1;
			}
			printf("[WiFi] MAC Address : %02X:%02X:%02X:%02X:%02X:%02X\r\n",
					mac.octet[0],mac.octet[1],mac.octet[2], mac.octet[3],mac.octet[4],mac.octet[5]);
#endif
			break;
          case 6:	//softap start
			if (argc == 4) {
				//CMD: api 6 [SSID] [Password]
				mhd_softap_start(argv[2], argv[3], security, 6);
			} else if (argc == 6) {
				//CMD: api 6 [SSID] [Password] [security] [Channel]
				mhd_softap_start(argv[2], argv[3], strtol(argv[4], &p, 10), strtol(argv[5], &p, 10));				
			} else {
				//CMD: api 6
				mhd_softap_start("3516a_softap", "12345678", security, 6);
			}
#if       !defined(CFG_NET_WIFI_SDIO_NGPL_SYN4345x) && !defined(CFG_NET_WIFI_SDIO_NGPL_SYN4343x)
			if ( mhd_softap_get_mac_address(&mac) )
			{
				printf("failed to get softap LAN mac address\n");
				return -1;
			}
			printf("Softap LAN MAC Address : %02X:%02X:%02X:%02X:%02X:%02X\r\n", mac.octet[0],mac.octet[1],mac.octet[2], mac.octet[3],mac.octet[4],mac.octet[5]);
#endif
			mhd_softap_start_dhcpd(0xc0a80101);			
			//dhcps_init();
			service_state |= state_softap_start;
            break;		
          case 7:	//softap stop
			//CMD: api 7
			mhd_softap_stop_dhcpd();
			mhd_softap_stop( 1 );
			service_state &= ~state_softap_start;
            break;				
          case 8:	//get mac list
			//CMD: api 8
			ret = mhd_softap_get_mac_list( (mhd_mac_t *)mac_list, &count );
			if ( ret == 0 ) 
			{
				for (int i=0; i<count; i++)
				{
					printf("MAC[%d]: %02X:%02X:%02X:%02X:%02X:%02X\n", i,
                          mac_list[i].octet[0], mac_list[i].octet[1], mac_list[i].octet[2],
                          mac_list[i].octet[3], mac_list[i].octet[4], mac_list[i].octet[5]);
				}
			}			
            break;
          case 9:	//module exit
			//CMD: api 9
			if(service_state & state_sta_connect) 
			{
				mhd_sta_disconnect( 1 );
				mhd_sta_network_down();	
				service_state &= ~state_sta_connect;
			} else if(service_state & state_softap_start) {
				mhd_softap_stop_dhcpd();
				mhd_softap_stop( 1 );	
				service_state &= ~state_softap_start;				
			}
			mhd_module_exit();
            break;
          case 10:
			//CMD: api 10
			return mhd_module_init(MHD_STA_AP_MODE);
            break;			
          default:
            printf("api function index error>\n");
            break;
        }        
    }
    return 0;
}

#define max_argv	10
void wifi_WL_cmd_string(char *str)
{
	int wl_argc = 0;
	char *wl_argv[max_argv];
	const char s[2] = " ";
	char *token;
	printf("---------------------------------------\n");
	printf("Input wl cmd: %s\n", str);
	
	for(int i=0; i<max_argv; i++)
		wl_argv[i] = NULL;
	
	/* get the first token */
	token = strtok(str, s);
	if(strcmp(token, "wl"))
		return;
	
	token = strtok(NULL, s);
	/* walk through other tokens */
	while( token != NULL ) 
	{
		wl_argv[wl_argc] = token;
		wl_argc++;		
    
		token = strtok(NULL, s);
	}
#if 0
	printf("wl_argc = %d\n", wl_argc);
	for(int i=0; i<wl_argc; i++)
		printf("wl_argv[%d] = %s\n", i, wl_argv[i]);
	printf("\n");
#endif
#if 0
	mhd_wl_cmd(wl_argc, wl_argv);	
#endif
}
