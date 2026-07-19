#include <sys/ioctl.h>
#include <sys/socket.h>
#include <time.h>
#include "ite/itp.h"
#include "SDL/SDL.h"
#include "iniparser/iniparser.h"
#include "ctrlboard.h"
#include "network_config.h"

static int createTcpServerThread(void);
#ifdef CFG_BUILD_NIMBLE
#include "bd/bt_main.h"
/* 只在全部服務穩定後才記錄 DC，避免開機過程的斷線也被計入 */
extern bool s_stable_baseline;
#define WIFI_DISCONNECT_RECORD(r) do { if (s_stable_baseline) nmgw_disconnect_record(r); } while(0)
#else
#define WIFI_DISCONNECT_RECORD(r) do {} while(0)
#endif

/* Eason Refined in Mar. 2020 */


/* WIFI Static Varibles */
static struct timeval       tvStart     = {0, 0}, tvEnd     = {0, 0},
                            tvStartWifi = {0, 0}, tvEndWifi = {0, 0};
static bool                 networkWifiIsReady, process_set  = false;
static int                  wifimgr_init = WIFIMGR_ECODE_NOT_INIT;

static int                  gWifiSwitch = 0;

/* WiFi retry: 連線失敗後自動重試 */
static int                  wifi_retry_count = 0;
static int                  wifi_retry_delay = 0;
#define WIFI_MAX_RETRIES    20
#define WIFI_RETRY_INTERVAL 10  /* 秒 */

static bool wifi_try_ssid2 = false;  // true=下一輪嘗試 SSID2
char g_current_ssid[64] = "";         // 目前連線目標的 SSID，供 scene.c 顯示

/* TEMP_DISCONNECT debounce: 等 3 秒確認真的沒恢復才算斷線 */
static uint32_t s_temp_disc_time = 0;   // TEMP_DISCONNECT 發生的時間 (SDL_GetTicks)
static bool     s_temp_disc_pending = false;

/* TEMP_DISCONNECT backoff: 連續 TEMP 斷線時自動拉長重試間隔，避免頻繁重連壓力 */
static int      temp_backoff_count = 0;
#define TEMP_BACKOFF_MAX    3    // max 2^3 = 8x baseline → 80 秒

/* TEMP_DISCONNECT AP switching: 連續 3 次 TEMP 斷線 → 切換到備用 SSID */
static int      temp_strike = 0;
static int      temp_stable_seconds = 0;  // 無 TEMP 穩定秒數，達 300s 歸零 strike
#define TEMP_STRIKE_LIMIT   3
#define TEMP_STRIKE_RESET_SEC 300  // 5 分鐘無 TEMP → 歸零

/* DC counting: 只在所有服務 (WiFi/BLE/MQTT/OTA) 都上線後才開始計算 */
bool s_stable_baseline = false;

/* Post-connect scan: WiFi 連線完成、driver 閒置後，scan 檢查是否有更好的白名單 AP */
static bool     s_post_scan_done = false;
static bool     s_post_scan_pending = false;
#define POST_SCAN_MAX_AP 64

/* Scan result cache for LCD display */
#define SCAN_DISPLAY_MAX 20
char   g_scan_ssid[SCAN_DISPLAY_MAX][33];
int    g_scan_rssi[SCAN_DISPLAY_MAX];
int    g_scan_count = 0;

/* WIFI Globel Varibles */
WIFI_MGR_SETTING            gWifiSetting;
//extern int gShowTextMode;
//extern int gTextMode;

/* WIFI Extern Functions */
extern void                 WifiFirstPowerOn(void);

// 0 : softap start
// 1:  handphone connect to device
// 2: switch to sta mode, connect to ap
static int gTestFWFlow =0;

#define TEST_SSID  "LKK"
#define TEST_PASSWORD  "tony24571478"




/* ======================================================================================= */

/**
  *
  *Static Network Functions
  *
  */

/* ======================================================================================= */

static void ResetWifi(void)
{
    char buf[16], *saveptr;

    memset(&gWifiSetting.setting, 0, sizeof (ITPEthernetSetting));

    gWifiSetting.setting.index = 0;

    // dhcp
    if (Ethernet_Wifi_DualMAC == 0)
        gWifiSetting.setting.dhcp     = theConfig.dhcp;
    else
        gWifiSetting.setting.dhcp     = 1; //if eth+wifi daul work, use DHCP IP.


    // autoip
    gWifiSetting.setting.autoip = 0;

    // ipaddr
    strcpy(buf, theConfig.ipaddr);
    gWifiSetting.setting.ipaddr[0] = atoi(strtok_r(buf,  ".", &saveptr));
    gWifiSetting.setting.ipaddr[1] = atoi(strtok_r(NULL, ".", &saveptr));
    gWifiSetting.setting.ipaddr[2] = atoi(strtok_r(NULL, ".", &saveptr));
    gWifiSetting.setting.ipaddr[3] = atoi(strtok_r(NULL, " ", &saveptr));

    // netmask
    strcpy(buf, theConfig.netmask);
    gWifiSetting.setting.netmask[0] = atoi(strtok_r(buf,  ".", &saveptr));
    gWifiSetting.setting.netmask[1] = atoi(strtok_r(NULL, ".", &saveptr));
    gWifiSetting.setting.netmask[2] = atoi(strtok_r(NULL, ".", &saveptr));
    gWifiSetting.setting.netmask[3] = atoi(strtok_r(NULL, " ", &saveptr));

    // gateway
    strcpy(buf, theConfig.gw);
    gWifiSetting.setting.gw[0] = atoi(strtok_r(buf,  ".", &saveptr));
    gWifiSetting.setting.gw[1] = atoi(strtok_r(NULL, ".", &saveptr));
    gWifiSetting.setting.gw[2] = atoi(strtok_r(NULL, ".", &saveptr));
    gWifiSetting.setting.gw[3] = atoi(strtok_r(NULL, " ", &saveptr));
}


static int wifiCallbackFucntion(int nState)
{
    switch (nState)
    {
        case WIFIMGR_STATE_CALLBACK_CONNECTION_FINISH:
            printf("[Ctrlboard] WifiCallback connection finish \n");
            networkWifiIsReady = true;
            wifi_retry_count = 0;  /* 成功後重置重試計數 */
            temp_backoff_count = 0;/* 成功後重置 TEMP backoff */
            if (!s_post_scan_done) s_post_scan_pending = true;  /* driver 穩定後 scan */
            s_temp_disc_pending = false; /* 恢復了，cancel debounce */
#if TEST_WIFI_DOWNLOAD
            sleep(5);
            createHttpThread();
#endif
//            if (Ethernet_Wifi_DualMAC == 0)
//                WebServerInit();

#ifdef CFG_NET_FTP_SERVER
		    ftpd_setlogin(theConfig.user_id, theConfig.user_password);
		    ftpd_init();
#endif

#if 0//defined(CFG_NET_WIFI_SDIO)
            if (theConfig.wifi_mode == WIFIMGR_MODE_SOFTAP){
                snprintf(gWifiSetting.ssid,     WIFI_SSID_MAXLEN,       theConfig.ssid);
                snprintf(gWifiSetting.password, WIFI_PASSWORD_MAXLEN,   theConfig.password);
                gWifiSetting.secumode = WifiMgr_Secu_ITE_To_RTL(theConfig.secumode);
            }
#endif
        //gWifiSwitch = 1;
        if (WifiMgr_Get_WIFI_Mode() == WIFIMGR_MODE_CLIENT){
			//gShowTextMode= 1;
			//gTextMode = 1;
        } else {
            // Receive ssid & paaword
            createTcpServerThread();
        }

        break;

        case WIFIMGR_STATE_CALLBACK_CLIENT_MODE_DISCONNECT_30S:
            printf("[Ctrlboard] WifiCallback connection disconnect 30s \n");
            WIFI_DISCONNECT_RECORD(NMGW_DISC_REASON_30S);
            if (wifi_retry_delay == 0 && theConfig.wifi_on_off && strlen(theConfig.ssid) > 0) {
                wifi_retry_delay = WIFI_RETRY_INTERVAL;
                printf("[Ctrlboard] WiFi reconnect scheduled in %d sec (disconnected 30s)\n",
                       WIFI_RETRY_INTERVAL);
            }
        break;

        case WIFIMGR_STATE_CALLBACK_CLIENT_MODE_RECONNECTION:
            printf("[Ctrlboard] WifiCallback connection reconnection \n");
        break;

        case WIFIMGR_STATE_CALLBACK_CLIENT_MODE_TEMP_DISCONNECT:
            printf("[Ctrlboard] WifiCallback connection temp disconnect \n");
            // 不立即排程重試 — TEMP disconnect 驅動可能自行恢復。
            // 僅設 debounce，3 秒後若仍未恢復才確認斷線並排程重試。
            s_temp_disc_time = SDL_GetTicks();
            s_temp_disc_pending = true;
            temp_stable_seconds = 0;   // 穩定計時歸零
        break;

        case WIFIMGR_STATE_CALLBACK_CLIENT_MODE_CONNECTING_FAIL:
            printf("[Ctrlboard] WifiCallback connecting fail\n");
            WIFI_DISCONNECT_RECORD(NMGW_DISC_REASON_CONN_FAIL);
            WifiMgr_Sta_Disconnect();
            // 失敗時立即切換到另一個 SSID
            if (strlen(theConfig.ssid2) > 0) {
                wifi_try_ssid2 = !wifi_try_ssid2;
                printf("[Ctrlboard] Switched to SSID%d for next retry\n",
                       wifi_try_ssid2 ? 2 : 1);
            }
            if (wifi_retry_count < WIFI_MAX_RETRIES) {
                wifi_retry_delay = WIFI_RETRY_INTERVAL;
                printf("[Ctrlboard] WiFi retry scheduled in %d sec (attempt %d/%d)\n",
                       WIFI_RETRY_INTERVAL, wifi_retry_count + 1, WIFI_MAX_RETRIES);
            } else {
                printf("[Ctrlboard] WiFi max retries (%d) reached, giving up\n", WIFI_MAX_RETRIES);
            }
        break;

        case WIFIMGR_STATE_CALLBACK_CLIENT_MODE_CONNECTING_CANCEL:
            printf("[Ctrlboard] WifiCallback connecting end to sleep/cancel \n");
        break;

        case WIFIMGR_STATE_CALLBACK_CLIENT_MODE_SLEEP_SAVE_INFO:
            snprintf(gWifiSetting.ssid,     WIFI_SSID_MAXLEN,       theConfig.ssid);
            snprintf(gWifiSetting.password, WIFI_PASSWORD_MAXLEN,   theConfig.password);
#if defined(CFG_NET_WIFI_SDIO_VND_RTK)
            gWifiSetting.secumode = WifiMgr_Secu_ITE_To_RTL(theConfig.secumode);
#elif defined(CFG_NET_WIFI_SDIO_VND_MHD) || defined(CFG_NET_WIFI_SDIO_NGPL_ATBM6031)
            gWifiSetting.secumode = WifiMgr_Secu_ITE_To_MHD(theConfig.secumode);
#else
            //snprintf(gWifiSetting.secumode, WIFI_SECUMODE_MAXLEN,   theConfig.secumode);
#endif
        break;

        case WIFIMGR_STATE_CALLBACK_CLIENT_MODE_SLEEP_CLEAN_INFO:
            printf("[Ctrlboard] WifiCallback clean connecting info \n");
            snprintf(gWifiSetting.ssid,     WIFI_SSID_MAXLEN,       "");
            snprintf(gWifiSetting.password, WIFI_PASSWORD_MAXLEN,   "");
#if defined(CFG_NET_WIFI_SDIO)
            gWifiSetting.secumode = 0;
#else
            //snprintf(gWifiSetting.secumode, WIFI_SECUMODE_MAXLEN,   "");
#endif
        break;

        default:
            printf("[Ctrlboard] WifiCallback unknown %d state  \n",nState);
        break;
    }
}


static int NetworkWifiPowerSleep(void)
{
    int  ret = WIFIMGR_ECODE_NOT_INIT;
    int  process_tv; //msec
    bool WifiNotReady = false;

#if defined(CFG_NET_WIFI_SDIO)

    /* ======================  For 8189FTV  ====================== */
    /* Confirm current status of sleep mode */
    if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_SLEEP_STATUS, NULL) == sleep_to_wakeup) {
        ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_SLEEP, (void *)default_no_sleep_or_wakeup);

        if (theConfig.wifi_mode == WIFIMGR_MODE_SOFTAP) {
            ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFIAP_ENABLE, NULL); //determine wifi softAP mode
            ret = WifiMgr_Init(WIFIMGR_MODE_SOFTAP, 0, gWifiSetting);
        } else {
            ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_ENABLE, NULL); //determine wifi client mode
            ret = WifiMgr_Init(WIFIMGR_MODE_CLIENT, 0, gWifiSetting);
        }
    }
    /* ====================================================== */

#elif defined(CFG_NET_WIFI_8188EUS) || defined(CFG_NET_WIFI_8188FTV)

    /* ======================  For 8188EUS  ====================== */
    /* Confirm current status of sleep mode */
    if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_SLEEP_STATUS, NULL) == sleep_to_wakeup) {
        while(!ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_IS_DEVICE_READY, NULL)) {
            printf("[%s] Wait wifi dongle plugin... \n", __FUNCTION__);
            usleep(200*1000);
            WifiNotReady = true;
        }

        // sleep to wakeup , wait for 5s to initialize
        gettimeofday(&tvStartWifi, NULL);
        process_tv = 0;

        do {
            usleep(1000*1000);
            gettimeofday(&tvEndWifi, NULL);
            process_tv = (int)itpTimevalDiff(&tvStartWifi, &tvEndWifi);

            if (process_tv > Network_Time_Delay) {
                printf("[%s] ready to init wifi \n", __FUNCTION__);
                break;
            }

            if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_SLEEP_STATUS, NULL) == wakeup_to_sleep) {
                printf("[%s] fast to sleep \n", __FUNCTION__);
                break;
            }
        } while (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_SLEEP_STATUS, NULL) == sleep_to_wakeup);

        if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_SLEEP_STATUS, NULL) == wakeup_to_sleep) {
            // fast sleep , do not init wifi mgr
        } else {
            ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_ADD_NETIF, NULL);
            ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_ENABLE, NULL);

            snprintf(gWifiSetting.ssid,     WIFI_SSID_MAXLEN,       theConfig.ssid);
            snprintf(gWifiSetting.password, WIFI_PASSWORD_MAXLEN,   theConfig.password);
            snprintf(gWifiSetting.secumode, WIFI_SECUMODE_MAXLEN,   theConfig.secumode);

            if (ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_WIFI_SLEEP_STATUS, NULL) == wakeup_to_sleep) {
                // fast sleep , do not init wifi mgr
            } else {
                ret = WifiMgr_Init(WIFIMGR_MODE_CLIENT, 0, gWifiSetting);
                usleep(200*1000);
                //Delay a while to ensure wifi mgr is inited and task is ready.
                ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_SLEEP, (void *)default_no_sleep_or_wakeup);
            }
        }
    }
    /* ====================================================== */
#endif

    return ret;
}




/* ======================================================================================= */

/**
  *
  *Network Functions
  *
  */

/* ======================================================================================= */

void NetworkWifiPreSetting(void)
{
    networkWifiIsReady = false;

    snprintf(gWifiSetting.ssid,     WIFI_SSID_MAXLEN,       theConfig.ssid);
    snprintf(gWifiSetting.password, WIFI_PASSWORD_MAXLEN,   theConfig.password);

    snprintf(gWifiSetting.ap_ssid,     WIFI_SSID_MAXLEN,       theConfig.ap_ssid);
    snprintf(gWifiSetting.ap_password, WIFI_PASSWORD_MAXLEN,   theConfig.ap_password);

#if defined(CFG_NET_WIFI_SDIO_VND_RTK)
    gWifiSetting.secumode = WifiMgr_Secu_ITE_To_RTL(theConfig.secumode);
#else
    //snprintf(gWifiSetting.secumode, WIFI_SECUMODE_MAXLEN,   theConfig.secumode);
#endif
#ifdef CFG_BTA_ENABLE
    theConfig.wifi_mode = 0; // set WiFi into client mode
#endif

    gWifiSetting.wifiCallback = wifiCallbackFucntion;

    /* Reset WIFI IP*/
    ResetWifi();

    gettimeofday(&tvStart, NULL);
}

#if 0
int softapTcpServer()

{
    //socket???إߊ    char inputBuffer[256] = {};
    char message[] = {"Hi,this is server.\n"};
    int sockfd = 0,forClientSockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
printf("softapTcpServer -------------------------------------------------------\n");
    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

    //socket???s?u
    struct sockaddr_in serverInfo,clientInfo;
    int addrlen = sizeof(clientInfo);
    bzero(&serverInfo,sizeof(serverInfo));

    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(8090);
    bind(sockfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo));
    listen(sockfd,5);

    while(1){
        forClientSockfd = accept(sockfd,(struct sockaddr*) &clientInfo, &addrlen);
        send(forClientSockfd,message,sizeof(message),0);
        recv(forClientSockfd,inputBuffer,sizeof(inputBuffer),0);
        printf("Get:%s\n",inputBuffer);
    }
    return 0;
}
#else
#define SERV_PORT 8090

#define MAXNAME 1024

char ssid[WIFI_SSID_MAXLEN];
char password[WIFI_PASSWORD_MAXLEN];

int gGetInfo = 0;

#define BUFSIZ 1024

void* softapTcpServer(void* arg)
{
    int socket_fd;      /* file description into transport */
    int recfd;     /* file descriptor to accept        */
    int length;     /* length of address structure      */
    int nbytes;     /* the number of read **/
    char buf[BUFSIZ];
    char buf2[BUFSIZ];
    struct sockaddr_in myaddr; /* address of this service */
    struct sockaddr_in client_addr; /* address of client    */
    char *ptr = buf;
    /*
    *      Get a socket into TCP/IP
    */
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) <0) {
        printf ("socket failed");
        exit(1);
    }
    printf("softapTcpServer -------------------------------------------------------\n");

    /*
    *    Set up our address
    */
    bzero ((char *)&myaddr, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(SERV_PORT);

    /*
    *     Bind to the address to which the service will be offered
    */
    if (bind(socket_fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) <0) {
        printf ("bind failed");
        exit(1);
    }

    /*
    * Set up the socket for listening, with a queue length of 5
    */
    if (listen(socket_fd, 20) <0) {
        printf ("listen failed");
        exit(1);
    }
    /*
    * Loop continuously, waiting for connection requests
    * and performing the service
    */
    length = sizeof(client_addr);
    printf("Server is ready to receive !!\n");

    while (1) {
        if ((recfd = accept(socket_fd,(struct sockaddr *)&client_addr, &length)) <0) {
            printf ("could not accept call-----------------------------------------");
            while(1);
            exit(1);
        }

        if ((nbytes = recv(recfd, &buf, BUFSIZ,0)) < 0) {
            printf("read of data error nbytes ! -----------------------------------------------");
            while(1);
            exit (1);
        }

        printf("Create socket #%d form %s : %d , length %d , nbytes %d \n", recfd,inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port),length,nbytes);
        ptr = strchr(buf, ',');
        if (!ptr){
            printf ("could not found ssid and password \n");
            while(1);
        }
        memset(buf2,0,sizeof(buf2));
        memcpy(buf2,buf,nbytes);
        printf("Receive : \n");
        printf("%s\n", &buf2);

        memset(ssid,0,sizeof(ssid));
        memset(password,0,sizeof(password));

        memcpy(ssid,buf,ptr-buf);
        memcpy(password,buf+(ptr-buf+1),nbytes-(ptr-buf+1));
        printf("ssid(%d):%s\n",ptr-buf,ssid);
        printf("password(%d):%s\n",nbytes-(ptr-buf+1),password);
        // get data and exit this while loop
        break;

        /* return to client */
        if (write(recfd, &buf, nbytes) == -1) {
            printf ("write to client error ------------------------------------------------------");
            while(1);
            exit(1);
        }
        //close(recfd);
        //printf("Can Strike Crtl-c to stop Server >>\n");
    }
    gGetInfo = 1;

    return NULL;
}

#endif

int createTcpServerThread(void)
{
    pthread_t       task;
    pthread_attr_t  attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, (255 * 1024));
    pthread_create(&task, &attr, softapTcpServer, NULL);
    return 0;
}

int gQRCode_SoftAP = 0;
void QRCode_SoftAP()
{

    if (gQRCode_SoftAP==0){
        gQRCode_SoftAP =1;
        snprintf(gWifiSetting.ssid,     WIFI_SSID_MAXLEN,       theConfig.ap_ssid);
        snprintf(gWifiSetting.password, WIFI_PASSWORD_MAXLEN,   theConfig.ap_password);
#ifdef CFG_NET_WIFI_SDIO_VND_RTK
        gWifiSetting.secumode = WifiMgr_Secu_ITE_To_RTL(theConfig.secumode);
#endif
    if (theConfig.wifi_mode == WIFIMGR_MODE_CLIENT){
        snprintf(gWifiSetting.ssid,     WIFI_SSID_MAXLEN,       theConfig.ap_ssid);
        snprintf(gWifiSetting.password, WIFI_PASSWORD_MAXLEN,   theConfig.ap_password);
        printf("QRCode_SoftAP %s %s \n",gWifiSetting.ssid,gWifiSetting.password);
        NetworkWifiModeSwitch();
    }
//        WifiMgr_HostAP_First_Start();
//        wifimgr_init = WifiMgr_Init(WIFIMGR_MODE_SOFTAP, 0, gWifiSetting);
        printf("QRCode_SoftAP end \n");
        gQRCode_SoftAP = 0;
    }


}


/* For WIFI main task in network_main.c  */
/**
 * 嘗試連線 WiFi（不掃描，直接連）。
 * 優先連 SSID1，若 SSID2 有設定則在 SSID1 失敗後嘗試 SSID2。
 */
static void wifi_try_connect(const char* caller)
{
    const char* target_ssid;
    const char* target_pass;
    const char* target_secu;

    if (wifi_try_ssid2 && strlen(theConfig.ssid2) > 0) {
        target_ssid = theConfig.ssid2;
        target_pass = theConfig.password2;
        target_secu = theConfig.secumode2;
    } else {
        target_ssid = theConfig.ssid;
        target_pass = theConfig.password;
        target_secu = theConfig.secumode;
    }

    printf("[Network Wifi] %s: connecting to SSID: %s (secu=%s)\n",
           caller, target_ssid, target_secu);
    strncpy(g_current_ssid, target_ssid, sizeof(g_current_ssid) - 1);
    WifiMgr_Sta_Connect(target_ssid, target_pass, target_secu);
}

/*
 * Post-connect scan: WiFi 已連線穩定後，掃描白名單是否有更好的 AP。
 * 只在找到不同且 RSSI 明顯更強 (>10dB) 的 AP 時才切換，避免頻繁跳動。
 */
static void _post_connect_scan(void)
{
    WIFI_MGR_SCANAP_LIST apList[POST_SCAN_MAX_AP];
    memset(apList, 0, sizeof(apList));

    printf("[Network Wifi] Post-connect scan...\n");
    int count = WifiMgr_Get_Scan_AP_Info(apList);
    printf("[Network Wifi] Post-connect scan: found %d APs\n", count);

    /* Cache for LCD display */
    int n = (count < SCAN_DISPLAY_MAX) ? count : SCAN_DISPLAY_MAX;
    for (int i = 0; i < n; i++) {
        strncpy(g_scan_ssid[i], (char*)apList[i].ssidName, 32);
        g_scan_ssid[i][32] = '\0';
        g_scan_rssi[i] = apList[i].rfQualityRSSI;
    }
    g_scan_count = n;

    if (count <= 0) return;

    const char* wl_ssid[2];
    const char* wl_pass[2];
    const char* wl_secu[2];
    int wl_count = 0;
    wl_ssid[wl_count] = theConfig.ssid;
    wl_pass[wl_count] = theConfig.password;
    wl_secu[wl_count] = theConfig.secumode;
    wl_count++;
    if (strlen(theConfig.ssid2) > 0) {
        wl_ssid[wl_count] = theConfig.ssid2;
        wl_pass[wl_count] = theConfig.password2;
        wl_secu[wl_count] = theConfig.secumode2;
        wl_count++;
    }

    /* 找最佳 RSSI */
    int best_wl = -1, best_rssi = -999;
    for (int i = 0; i < count; i++) {
        for (int w = 0; w < wl_count; w++) {
            if (strcmp((char*)apList[i].ssidName, wl_ssid[w]) == 0) {
                if ((int)apList[i].rfQualityRSSI > best_rssi) {
                    best_rssi = apList[i].rfQualityRSSI;
                    best_wl = w;
                }
                break;
            }
        }
    }
    if (best_wl < 0) return;

    /* 如果是不同 SSID 且 RSSI 明顯更強，或目前連的 SSID 不在白名單中，才切換 */
    if (strcmp(g_current_ssid, wl_ssid[best_wl]) != 0) {
        printf("[Network Wifi] Post-connect scan: switching to %s (RSSI=%d)\n",
               wl_ssid[best_wl], best_rssi);
        strncpy(g_current_ssid, wl_ssid[best_wl], sizeof(g_current_ssid) - 1);
        WifiMgr_Sta_Connect((char*)wl_ssid[best_wl],
                            (char*)wl_pass[best_wl],
                            (char*)wl_secu[best_wl]);
    } else {
        printf("[Network Wifi] Post-connect scan: already on best AP (%s)\n",
               g_current_ssid);
    }
}

void NetworkWifiProcess(void)
{
    int process_tv; //msec
	int i;

    // TEMP strike stable timer: 連線穩定 N 秒後歸零 strike counter
    if (networkWifiIsReady && !s_temp_disc_pending && temp_strike > 0) {
        temp_stable_seconds++;
        if (temp_stable_seconds >= TEMP_STRIKE_RESET_SEC) {
            temp_strike = 0;
            temp_stable_seconds = 0;
            printf("[Network Wifi] TEMP strike reset (stable %d sec)\n",
                   TEMP_STRIKE_RESET_SEC);
        }
    }

    // TEMP_DISCONNECT debounce: 等 3 秒，若 WiFi 仍未恢復才計為斷線並排程重試
    if (s_temp_disc_pending) {
        if (networkWifiIsReady) {
            s_temp_disc_pending = false;  // 已恢復，不算
            printf("[Network Wifi] TEMP_DISCONNECT recovered within debounce window\n");
        } else if (SDL_GetTicks() - s_temp_disc_time >= 3000) {
            WIFI_DISCONNECT_RECORD(NMGW_DISC_REASON_TEMP);
            s_temp_disc_pending = false;
            printf("[Network Wifi] TEMP_DISCONNECT confirmed after 3s debounce\n");
            // 連續 TEMP_STRIKE_LIMIT 次確認斷線 → 切換備用 SSID
            if (strlen(theConfig.ssid2) > 0) {
                temp_strike++;
                printf("[Network Wifi] TEMP strike %d/%d\n", temp_strike, TEMP_STRIKE_LIMIT);
                if (temp_strike >= TEMP_STRIKE_LIMIT) {
                    temp_strike = 0;
                    wifi_try_ssid2 = !wifi_try_ssid2;
                    printf("[Network Wifi] Switching to SSID%d\n",
                           wifi_try_ssid2 ? 2 : 1);
                }
            }
            // 確認斷線後才排程重試（含 backoff）
            if (theConfig.wifi_on_off && strlen(theConfig.ssid) > 0
                && wifi_retry_delay == 0 && wifi_retry_count < WIFI_MAX_RETRIES) {
                int delay = WIFI_RETRY_INTERVAL;  // baseline 10s
                if (temp_backoff_count > 0) {
                    int shift = (temp_backoff_count < TEMP_BACKOFF_MAX)
                                ? temp_backoff_count : TEMP_BACKOFF_MAX;
                    delay = WIFI_RETRY_INTERVAL * (1 << shift);
                }
                wifi_retry_delay = delay;
                temp_backoff_count++;
                printf("[Network Wifi] TEMP retry scheduled in %d sec (backoff lv=%d)\n",
                       delay, temp_backoff_count);
            }
        }
    }

    gettimeofday(&tvEnd, NULL);

    process_tv = (int)itpTimevalDiff(&tvStart, &tvEnd);

    if (process_tv > Network_Time_Delay && process_set == false) {
#if !defined(CFG_NET_WIFI_SDIO)
        while(!ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_IS_DEVICE_READY, NULL)) {
            usleep(100*1000);
            printf("Wait WIFI device(USB) get ready... \n");
        }
#endif

        WifiMgr_Sta_Switch(theConfig.wifi_on_off);
        printf("[%s] WIFI mode: %s mode, WIFI(%s) \n", __FUNCTION__,  theConfig.wifi_mode ? "Soft AP":"Station", theConfig.wifi_on_off ? "ON":"OFF");
        WifiMgr_Disable_Auto_Reconnect(1);

        if (theConfig.wifi_mode == WIFIMGR_MODE_SOFTAP){
#if defined(CFG_NET_WIFI_SDIO)
            WifiMgr_HostAP_First_Start();
#endif
            wifimgr_init = WifiMgr_Init(WIFIMGR_MODE_SOFTAP, 0, gWifiSetting);
        } else {
#if defined(CFG_NET_WIFI_SDIO_VND_RTK)
            ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_ENABLE, NULL); //determine wifi client mode
#endif

            WifiMgr_Sta_Switch(theConfig.wifi_on_off);
            wifimgr_init = WifiMgr_Init(WIFIMGR_MODE_CLIENT, 0, gWifiSetting);
        }

        /* WiFi driver 初始化完成，通知 BLE thread 可以開始使用 UART0 */
#ifdef CFG_BUILD_NIMBLE
        nimbleNotifyWifiReady();
        if (wifimgr_init == WIFIMGR_ECODE_OK) {
            nmgw_status_set_flag(NMGW_FLAG_WIFI_READY);
            printf("[Network Wifi] Driver init OK, WiFi hardware ready\n");
        } else {
            printf("[Network Wifi] Driver init FAILED (ecode=%d)\n", wifimgr_init);
        }
#endif

#if TEST_PING_WIFI
        while(!networkWifiIsReady){
            usleep(100*1000);
        }
        ping_main();
#endif

        process_set = true;
    } else if (process_set == true) {
        networkWifiIsReady = (bool)WifiMgr_Sta_Is_Available(&process_tv); //Return: NGPL(SSID Len)

        // 全部服務上線後才開始記錄 DC（排除開機過程的正常斷線）
        if (!s_stable_baseline) {
            uint8_t f = nmgw_status_get_flags();
            if ((f & NMGW_FLAG_WIFI_OK) && (f & NMGW_FLAG_BLE_OK) &&
                (f & NMGW_FLAG_MQTT_OK) && (f & NMGW_FLAG_OTA_OK)) {
                s_stable_baseline = true;
                printf("[Network Wifi] All services stable, DC counting enabled\n");
            }
        }
        if (gWifiSwitch == 1){
                NetworkWifiModeSwitch();

                gWifiSwitch = 0;
        }

        // 開機延遲後連線
        static int boot_connect_delay = 10;
        if (boot_connect_delay > 0) {
            boot_connect_delay--;
            if (boot_connect_delay == 0) {
                if (theConfig.wifi_on_off) {
                    wifi_try_connect("boot");
                }
            }
        }

        // Post-connect scan: WiFi 已連線 + BLE 已啟動 + driver 閒置 → 安全 scan
        // 檢查白名單是否有訊號更強的 AP，有的話自動切換
        if (s_post_scan_pending && !s_post_scan_done) {
            extern bool g_ble_started;
            if (g_ble_started && networkWifiIsReady) {
                static int scan_delay = 0;
                if (scan_delay < 1) { scan_delay++; }  // 等 1s
                else {
                    scan_delay = 0;
                    s_post_scan_pending = false;
                    s_post_scan_done = true;
                    _post_connect_scan();
                }
            }
        }

        // WiFi 連線失敗自動重試（USB 連線中時有限次數延後，避免永久阻塞）
        if (wifi_retry_delay > 0) {
            wifi_retry_delay--;
            if (wifi_retry_delay == 0 && !networkWifiIsReady
                && theConfig.wifi_on_off
                && wifi_retry_count < WIFI_MAX_RETRIES) {
                // WiFi driver bug: runtime WifiMgr_Sta_Connect() 會干擾 USB。
                // 最多延後 6 次 (60s)，之後強制重試（尤其需切換備用 SSID 時）
                if (ioctl(ITP_DEVICE_USBDACM, ITP_IOCTL_IS_CONNECTED, NULL)) {
                    static int usb_defer_count = 0;
                    usb_defer_count++;
                    if (usb_defer_count <= 6) {
                        printf("[Network Wifi] USB active, defer %d/6 (retry %d/%d)\n",
                               usb_defer_count, wifi_retry_count + 1, WIFI_MAX_RETRIES);
                        wifi_retry_delay = WIFI_RETRY_INTERVAL;
                    } else {
                        usb_defer_count = 0;
                        wifi_retry_count++;
                        printf("[Network Wifi] USB defer limit reached, forcing retry %d/%d\n",
                               wifi_retry_count, WIFI_MAX_RETRIES);
                        wifi_try_connect("retry");
                    }
                } else {
                    wifi_retry_count++;
                    wifi_try_connect("retry");
                }
            }
        }

        // Failsafe watchdog: if WiFi stays disconnected with no retry in progress,
        // force a reconnection after 60 seconds (catches missed disconnect callbacks)
        static int failsafe_down_timer = 0;
        if (!networkWifiIsReady && wifi_retry_delay == 0
            && theConfig.wifi_on_off
            && wifi_retry_count < WIFI_MAX_RETRIES) {
            failsafe_down_timer++;
            if (failsafe_down_timer >= 60) {
                printf("[Network Wifi] Failsafe: WiFi down %d sec, forcing reconnect...\n",
                       failsafe_down_timer);
                wifi_retry_delay = WIFI_RETRY_INTERVAL;
                failsafe_down_timer = 0;
            }
        } else {
            failsafe_down_timer = 0;  // Reset when WiFi is up or retry is pending
        }

#if 0
		// check if handphone connetct to device
		if (gTestFWFlow!= 2 && WifiMgr_Get_HostAP_Device_Number() > 0){
			// handphone connetct to device
			printf("[Network Wifi] handphone connetct to device \n");

			//gShowTextMode= 1;
			//gTextMode = 0;
			//gTestFWFlow  = 2;
			// switch to sta mode
			sleep(1);
			NetworkWifiModeSwitch();
			sleep(2);
			for (i=0;i<10;i++){
				printf("wait %d / 10 \n",i);
				sleep(1);
			}

			printf("[Network Wifi] device connecting ...........................................................\n");

			WifiMgr_Sta_Connect(TEST_SSID,TEST_PASSWORD,NULL);
		}
#else
        // PC App WiFi setting (via USB SET_WIFI command)
        // WiFi driver has a known bug where runtime WifiMgr_Sta_Connect()
        // interferes with USB controller. To avoid USB disconnection,
        // we only save the config here. New AP takes effect after reboot.
        if (gGetInfo){
            gGetInfo = 0;
            printf("[Network Wifi] PC App set new AP (saved): SSID=%s (effective after reboot)\n", ssid);
        }

#endif

#if defined(CFG_POWER_SLEEP)
        wifimgr_init = NetworkWifiPowerSleep();
#endif

#if TEST_CHANGE_AP
        if (networkWifiIsReady)
            link_differnet_ap();
#endif

    }
}


bool NetworkWifiIsReady(void)
{
    return networkWifiIsReady;
}

int NetworkWifiGetRetryCount(void)
{
    return wifi_retry_count;
}


void NetworkWifiModeSwitch(void)
{
	int ret;
    if (theConfig.wifi_mode == WIFIMGR_MODE_SOFTAP){
        snprintf(gWifiSetting.ssid,     WIFI_SSID_MAXLEN,       theConfig.ap_ssid);
        snprintf(gWifiSetting.password, WIFI_PASSWORD_MAXLEN,   theConfig.ap_password);
    }
//	memset(&gWifiSetting, 0, sizeof(gWifiSetting));
    gWifiSetting.wifiCallback = wifiCallbackFucntion;
#if defined(CFG_NET_WIFI_SDIO_VND_MHD) || defined(CFG_NET_WIFI_SDIO_NGPL_8821CS)
    WifiMgr_Set_APMode_5G(1);
#endif
	ret = WifiMgr_Sta_HostAP_Switch(gWifiSetting);
}

/* ======================================================================================= */




/* Useless!! For only build WIFI in indoor project */
#if !defined(CFG_NET_ETHERNET)
bool NetworkIsReady(void)
{
    return false;
}

bool NetworkServerIsReady(void)
{
    return false;
}

void NetworkExit(void)
{
    //Not implement in network_wifi.c
}

void NetworkSntpUpdate(void)
{
    //Not implement in network_wifi.c
}
#endif

