#include <sys/ioctl.h>
#include <assert.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "ecmMgr.h"

#define STACK_SIZE      (32* 1024)
#define DIAL_OK         0


static sem_t        Switch_Netif_Start, Switch_Netif_Done;
static sem_t        Detect_Net_Start, Detect_Net_Done;

static char         Netif_Name[2] = {0}, Det_Name[2] = {0};
extern struct netif *netif_default;

extern int ecm_dial(void);
extern int dial_check();

static int
_ECM_Switch_Netif_Init(void)
{
    int rc = -1;

    /* Set Semaphore */
    rc = sem_init(&Switch_Netif_Start, 0, 0);
    if (rc < 0) {
        ECM_ERROR("Switch_Netif_Start sem_init() fail!\r\n");
        rc = ECM_STATUS_ERROR;
        return rc;
    }

    rc = sem_init(&Switch_Netif_Done, 0, 0);
    if (rc < 0) {
        ECM_ERROR("Switch_Netif_Done sem_init() fail!\r\n");
        rc = ECM_STATUS_ERROR;
        return rc;
    }
}

static inline void
_ECM_DUAL_MAC_PRIO_GW(uint8_t prio[])
{
    /* ==== 4G ==== */
    if (prio[0] > 0) {
        if (prio[1] == 0) {
            Switch_Ecm_Netif_Easy();
            return ;
        }

        if (dial_check() != DIAL_OK) {
            ECM_INFO(" 4G is not AVAIL, change to netif of WIFI[%c]!! \n", ECM_Get_Default_Netif_Name());
            ioctl(ITP_DEVICE_ECM, ITP_IOCTL_DISABLE, NULL); //ECM use middle ware of Eth
            Switch_Wifi_Netif_Easy();
        } else {
            /* 4G priority is higher than WIFI */
            if (prio[1] > prio[0])
                Switch_Ecm_Netif_Easy();
        }
    }

    /* ==== WIFI ==== */
    if (prio[1] > 0) {
        if (prio[0] == 0) {
            Switch_Wifi_Netif_Easy();
            return ;
        }

        if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_IS_CONNECTED, NULL) < 0) {
            ECM_INFO(" WIFI is not AVAIL, change to netif of 4G[%c]!! \n", ECM_Get_Default_Netif_Name());
            Switch_Ecm_Netif_Easy();
        } else {
            /* WIFI priority is higher than 4G */
            if (prio[0] > prio[1])
                Switch_Wifi_Netif_Easy();
        }
    }
}

static inline void
_ECM_DUAL_MAC_PRIO_WE(uint8_t prio[])
{
    ECM_TRACE("==== W[%d] E[%d] ====\n", prio[1], prio[2]);
    /* ==== WIFI ==== */
    if (prio[1] > 0) {
        if (prio[2] == 0) {
            Switch_Wifi_Netif_Easy();
            return ;
        }

        if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_IS_CONNECTED, NULL) < 0) {
            ECM_INFO(" WIFI is not AVAIL, change to netif of Eth!! \n");
            Switch_Eth_Netif_Easy();
        } else {
            /* WIFI priority is higher than Eth */
            if (prio[2] > prio[1])
                Switch_Wifi_Netif_Easy();
        }
    }

    /* ==== Eth ==== */
    if (prio[2] > 0) {
        if (prio[1] == 0) {
            Switch_Eth_Netif_Easy();
            return ;
        }

        if (!ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_IS_CONNECTED, NULL)) {
            ECM_INFO(" Eth is not AVAIL, change to netif of WIFI!! \n");
            ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_DISABLE, NULL);
            Switch_Wifi_Netif_Easy();
        } else {
            /* Eth priority is higher than WIFI */
            if (prio[1] > prio[2])
                Switch_Eth_Netif_Easy();
        }
    }
}

static inline void
_ECM_DUAL_MAC_PRIO_GE(uint8_t prio[])
{
    ECM_TRACE("==== G[%d] E[%d] ====\n", prio[0], prio[2]);
    /* ==== 4G ==== */
    if (prio[0] > 0) {
        if (prio[2] == 0) {
            Switch_Ecm_Netif_Easy();
            return ;
        }

        if (dial_check() != DIAL_OK) {
            ECM_INFO(" 4G is not AVAIL, change to netif of Eth!! \n");
            ioctl(ITP_DEVICE_ECM, ITP_IOCTL_DISABLE, NULL);
            Switch_Eth_Netif_Easy();
        } else {
            /* 4G priority is higher than Eth */
            if (prio[2] > prio[0])
                Switch_Ecm_Netif_Easy();
        }
    }

    /* ==== Eth ==== */
    if (prio[2] > 0) {
        if (prio[0] == 0) {
            Switch_Eth_Netif_Easy();
            return ;
        }

        if (!ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_IS_CONNECTED, NULL)) {
            ECM_INFO(" Eth is not AVAIL, change to netif of 4G!! \n");
            Switch_Ecm_Netif_Easy();
        } else {
            /* Eth priority is higher than 4G */
            if (prio[0] > prio[2])
                Switch_Eth_Netif_Easy();
        }
    }
}


static inline void
_ECM_TRI_MAC_PRIO(uint8_t prio[])
{
    ECM_TRACE("==== G[%d] W[%d] E[%d] ====\n", prio[0], prio[1], prio[2]);
#if defined (CFG_NET_ETHERNET_4G)
    if (prio[0] > 0) {
        if (dial_check() != DIAL_OK) {
            ECM_INFO(" 4G is not AVAIL, change another netif!! \n");
            ioctl(ITP_DEVICE_ECM, ITP_IOCTL_DISABLE, NULL);

            if (prio[1] > 0 && prio[2] > 0) {
                if (prio[1] > prio[2])
                    Switch_Eth_Netif_Easy();
                else if (prio[1] < prio[2])
                    Switch_Wifi_Netif_Easy();
            } else if (prio[1] > 0 && prio[2] == 0) {
                Switch_Wifi_Netif_Easy();
            } else if (prio[2] > 0 && prio[1] == 0) {
                Switch_Eth_Netif_Easy();
            }
        } else {
            ioctl(ITP_DEVICE_ECM, ITP_IOCTL_ENABLE, NULL);

            if (prio[0] == 1)
                Switch_Ecm_Netif_Easy();
        }
    }
#endif

#if defined (CFG_NET_WIFI)
    if (prio[1] > 0) {
        if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_IS_CONNECTED, NULL) < 0) {
            ECM_INFO(" WIFI is not AVAIL, change another netif!! \n");
            //netif flag is set in WIFI driver

            if (prio[0] > 0 && prio[2] > 0) {
                if (prio[0] > prio[2])
                    Switch_Eth_Netif_Easy();
                else if (prio[0] < prio[2])
                    Switch_Ecm_Netif_Easy();
            } else if (prio[0] > 0 && prio[2] == 0) {
                Switch_Ecm_Netif_Easy();
            } else if (prio[2] > 0 && prio[0] == 0) {
                Switch_Eth_Netif_Easy();
            }
        } else {
            if (prio[1] == 1)
                Switch_Wifi_Netif_Easy();
        }
    }
#endif

#if defined (CFG_NET_ETHERNET)
    if (prio[2] > 0) {
        if (!ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_IS_AVAIL, NULL)) {
            ECM_INFO(" Eth is not AVAIL, change another netif!! \n");
            ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_DISABLE, NULL);

            if (prio[0] > 0 && prio[1] > 0) {
                if (prio[0] > prio[1])
                    Switch_Wifi_Netif_Easy();
                else if (prio[0] < prio[1])
                    Switch_Ecm_Netif_Easy();
            } else if (prio[0] > 0 && prio[1] == 0) {
                Switch_Ecm_Netif_Easy();
            } else if (prio[1] > 0 && prio[0] == 0) {
                Switch_Wifi_Netif_Easy();
            }
        } else {
            ioctl(ITP_DEVICE_ECM, ITP_IOCTL_ENABLE, NULL);

            if (prio[2] == 1)
                Switch_Eth_Netif_Easy();
        }
    }
#endif

}

static void*
_ECM_Switch_Netif_Thread(void* arg)
{
    unsigned int SW_CNT = 0;

    _ECM_Switch_Netif_Init();

    for (;;) {

        sem_wait(&Switch_Netif_Start);

        /*ECM_INFO("================ [%d] SW START (netif %s Now, want to set %s) ================>\n",
            SW_CNT++, netif_default->name, Netif_Name);*/


        switch (Netif_Name[0])
        {
            case 'e':
                //ECM_TRACE("Set Eth netif\n");
                ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_RESET_DEFAULT, NULL);
                break;

            case 'w':
                //ECM_TRACE("Set WIFI netif\n");
                ioctl(ITP_DEVICE_WIFI,     ITP_IOCTL_RESET_DEFAULT, NULL);
                break;

            case 'g':
                //ECM_TRACE("Set 4G netif\n");
                ioctl(ITP_DEVICE_ECM,      ITP_IOCTL_RESET_DEFAULT, NULL);
                break;
        }


        /* Wait connection set done */
        sem_wait(&Switch_Netif_Done); /* Switch Done need post */
        /*ECM_INFO("<================ SW DONE (%s netif Now) ================\n\n\n",
            netif_default->name);*/
    }

    return NULL;
}

static void*
_ECM_Main_Thread(void* arg)
{
#if defined (CFG_NET_PRIO_MULTI_MAC)
    struct netif *mac_netif;
    uint8_t      prio[] = { CFG_NET_PRIO_MULTI_MAC };
#endif

    struct timeval tvDHCP1 = {0, 0};
    struct timeval tvDHCP2 = {0, 0};

    // Start DHCP count
    gettimeofday(&tvDHCP1, NULL);

    for (;;) {
        if (ECM_Get_Network_Available() == ECM_NETWORK_LINK_UP) {
            gettimeofday(&tvDHCP2, NULL);
            if (itpTimevalDiff(&tvDHCP1, &tvDHCP2) > ECM_DHCP_RENEW_TIME * ECM_TIME_MSEC_TO_MIN) {
                ECM_INFO("====>Send DHCP Discover!!!!!\n");
                ECM_INFO("DHCP renew in %ld min \n", itpTimevalDiff(&tvDHCP1, &tvDHCP2)/60/1000);
                ioctl(ITP_DEVICE_ECM, ITP_IOCTL_RENEW_DHCP, NULL);

                //Reset timer
                gettimeofday(&tvDHCP1, NULL);
                gettimeofday(&tvDHCP2, NULL);
            }
        }

#if defined (CFG_NET_PRIO_MULTI_MAC)

        /* 4G + WIFI */
#if defined (CFG_NET_WIFI_ECM_EX)
        _ECM_DUAL_MAC_PRIO_GW(prio);
#endif

        /* WIFI + ETH */
#if defined (CFG_NET_WIFI_ETH_EX)
        _ECM_DUAL_MAC_PRIO_WE(prio);
#endif

        /* 4G + ETH */
#if defined (CFG_USB_ECM_EX) || defined (CFG_USBH_CD_CDCECM_EX)
        _ECM_DUAL_MAC_PRIO_GE(prio);
#endif

#if (defined (CFG_USB_ECM_EX) || defined (CFG_USBH_CD_CDCECM_EX)) && defined (CFG_NET_WIFI_ECM_EX) && defined (CFG_NET_WIFI_ETH_EX)
        _ECM_TRI_MAC_PRIO(prio);
#endif

#endif
        sleep(1);
    }

    return NULL;
}

ECM_Network_t
ECM_Get_Network_Available(void)
{
    int rc;

    /* Netif have an legal IP */
    rc = ioctl(ITP_DEVICE_ECM, ITP_IOCTL_IS_AVAIL, NULL);
    return rc;
}

void
Switch_Wifi_Netif_Easy(void)
{
    struct netif *netif = netif_default;
    char name[] = "w";

#if defined (CFG_NET_WIFI)
    if (strncmp(netif->name, name, sizeof(char)) == 0)
#endif
        return;

    ECM_Switch_Netif_Start("w0");
    usleep(100*1000);
    ECM_Switch_Netif_Done("w0");
}

void
Switch_Ecm_Netif_Easy(void)
{
    struct netif *netif = netif_default;
    char name[] = "g";

#if defined (CFG_NET_ETHERNET_4G)
    if (strncmp(netif->name, name, sizeof(char)) == 0)
#endif
        return;

    ECM_Switch_Netif_Start("g0");
    usleep(100*1000);
    ECM_Switch_Netif_Done("g0");
}

void
Switch_Eth_Netif_Easy(void)
{
    struct netif *netif = netif_default;
    char name[] = "e";

#if defined (CFG_NET_ETHERNET)
    if (strncmp(netif->name, name, sizeof(char)) == 0)
#endif
        return;

    ECM_Switch_Netif_Start("e0");
    usleep(100*1000);
    ECM_Switch_Netif_Done("e0");
}


void
ECM_Switch_Netif_Start(const char* name)
{
    strcpy(Netif_Name, name);
    sem_post(&Switch_Netif_Start);
}

void
ECM_Switch_Netif_Done(const char* name)
{
    sem_post(&Switch_Netif_Done);
}

char
ECM_Get_Default_Netif_Name(void)
{
    struct netif *netif = netif_default;

    //printf("==== name[%c], name[%c] ====\n", netif->name[0], netif->name[1]);

    return netif->name[0];
}


int
ECM_Init(void)
{
    pthread_t task, rx_task;
    pthread_attr_t attr;
    int rc;

    ECM_INFO("==== ECM_Init ====\n");
#if defined(CFG_USB_OPTION) || defined(CFG_USBH_CD_CDCACM)
    rc = ecm_dial();
    if (rc) {
        printf("Dial fail! \n");

        return rc;
    }
#endif

    //pthread_create(&rx_task, NULL, _ECM_RCV_CMD, NULL);

    {
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&attr, STACK_SIZE);
        pthread_create(&task, &attr, _ECM_Main_Thread, NULL);
    }

#if defined(CFG_NET_PRIO_MULTI_MAC)
    {
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&attr, STACK_SIZE);
        pthread_create(&task, &attr, _ECM_Switch_Netif_Thread, NULL);
    }
#endif

    return rc;
}
