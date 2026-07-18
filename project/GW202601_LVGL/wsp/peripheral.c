#include <sys/ioctl.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "ite/itp.h"
#include "ite/ith.h"
#include "scene.h"
#include "saradc/saradc.h"
#include "peripheral.h"
#include "../scene_utility.h"
#include "ite/ite_idb.h"
#include "ite/ite_usbex.h"
#include "ctrlboard.h"
#include "bd/bt_log.h"
#include "wifiMgr.h"

//#define VIDEO_SENSOR_DEBUG

#define PERI_MAX_QUEUE_SIZE 2
//#define OUTSIDE_TEMP_CHN     SARADC_5
#define LIGHT_SENSOR_CHN   SARADC_6
#define DONTCARE 0
#define SARADC_BUF_LEN    512

struct peripheralInfo{
    uint32_t adc_temp;
    uint32_t als_light_sensor;
    uint32_t result_ok_eeprom;
    uint32_t result_ng_eeprom;
} ;

static struct peripheralInfo periMonitor = {0};

//Add by Hardy: for PC Tool USB IDB Upgrade Service
#include <pthread.h>
#include <string.h>
static pthread_t usbdAcmTask;
#define ACM_BUF_LEN (8*1024)

#include <stdarg.h>

#ifndef _WIN32
#include "openrtos/FreeRTOS.h"
#include "openrtos/task.h"
#include "ith/ith_wd.h"
#endif

#undef printf
#define printf app_printf

#define LOG_RING_BUF_SIZE (32 * 1024)
static char s_log_ring_buf[LOG_RING_BUF_SIZE];
static int s_log_ring_head = 0;
static int s_log_ring_tail = 0;

void log_ring_buffer_push(const char *str, int len) {
#ifndef _WIN32
    portENTER_CRITICAL();
#endif
    for (int i = 0; i < len; i++) {
        s_log_ring_buf[s_log_ring_head] = str[i];
        s_log_ring_head = (s_log_ring_head + 1) % LOG_RING_BUF_SIZE;
        if (s_log_ring_head == s_log_ring_tail) {
            s_log_ring_tail = (s_log_ring_tail + 1) % LOG_RING_BUF_SIZE;
        }
    }
#ifndef _WIN32
    portEXIT_CRITICAL();
#endif
}

int log_ring_buffer_pop(char *dest, int max_len) {
#ifndef _WIN32
    portENTER_CRITICAL();
#endif
    int count = 0;
    while (s_log_ring_tail != s_log_ring_head && count < max_len - 1) {
        dest[count++] = s_log_ring_buf[s_log_ring_tail];
        s_log_ring_tail = (s_log_ring_tail + 1) % LOG_RING_BUF_SIZE;
    }
    dest[count] = '\0';
#ifndef _WIN32
    portEXIT_CRITICAL();
#endif
    return count;
}

void app_printf(const char *format, ...) {
    va_list args;
    va_list args_copy;
    va_start(args, format);
    va_copy(args_copy, args);
    
    // 1. Output to UART
    vprintf(format, args);
    
    // 2. Output to Ring Buffer
    char buf[256];
    int len = vsnprintf(buf, sizeof(buf), format, args_copy);
    if (len > 0) {
        if (len >= (int)sizeof(buf)) {
            len = (int)sizeof(buf) - 1;
        }
        log_ring_buffer_push(buf, len);
    }
    
    va_end(args_copy);
    va_end(args);
}

int escape_json_string(char *dest, int max_len, const char *src) {
    int d_idx = 0;
    for (int s_idx = 0; src[s_idx] != '\0' && d_idx < max_len - 5; s_idx++) {
        char c = src[s_idx];
        if (c == '\\' || c == '"') {
            dest[d_idx++] = '\\';
            dest[d_idx++] = c;
        } else if (c == '\n') {
            dest[d_idx++] = '\\';
            dest[d_idx++] = 'n';
        } else if (c == '\r') {
            dest[d_idx++] = '\\';
            dest[d_idx++] = 'r';
        } else if (c == '\t') {
            dest[d_idx++] = '\\';
            dest[d_idx++] = 't';
        } else if ((unsigned char)c < 32) {
            dest[d_idx++] = '.';
        } else {
            dest[d_idx++] = c;
        }
    }
    dest[d_idx] = '\0';
    return d_idx;
}

#undef printf
#define printf app_printf

static pthread_t adcTask;
static pthread_t i2cTask;

// WiFi Scan 緩衝區（由 NetworkWifiProcess 填入，USB ACM task 讀取）
char  s_wifi_scan_result[1024];
int   s_wifi_scan_result_len = 0;
bool  s_wifi_scan_ready = false;
pthread_mutex_t s_wifi_scan_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile bool g_wifi_scan_requested = false;
//static pthread_t eepromTestTask;
static mqd_t inQueue = -1;
static volatile bool extQuit = false;
static int fd;
uint8_t lightSensorSlaveAddr = 0x53;

//static bool isEEPROMEnable = false;

static bool isCreatePthread = false;
#define CHECK_BIT(var, pos) ((var) & (1 << (pos)))
#define WAITTING_TIME 3


//Add by Hardy: for PC Tool USB IDB Upgrade Service
static void* UsbdAcmProcessTask(void* arg);

static void* SarAdcTask(void* arg);
static void* I2cAlsTask(void* arg);



uint8_t i2cRead(int fd, uint8_t address, uint8_t regAddr) {
    int result;
    uint8_t dbuf[2];
    uint8_t* pdbuf = dbuf;
    ITPI2cInfo evt;

    dbuf[0] = (uint8_t)(regAddr);
    pdbuf++;

    evt.slaveAddress = address;
    evt.cmdBuffer = dbuf;
    evt.cmdBufferSize = 1;
    evt.dataBuffer = pdbuf;
    evt.dataBufferSize = 1;

    result = read(fd, &evt, 1);
    if (result != 0) {
        ithPrintf("read i2c address 0x%02x error!\n", regAddr);
    }
    return dbuf[1];

}

uint32_t i2cWrite(int fd, uint8_t address, uint8_t RegAddr, uint8_t data) {
    int result = 0;
    uint8_t dbuf[4];
    uint8_t* pdbuf = dbuf;
    ITPI2cInfo evt;

    *pdbuf++ = (uint8_t)(RegAddr & 0xff);
    *pdbuf = (uint8_t)(data);

    evt.slaveAddress = address;
    evt.cmdBuffer = dbuf;
    evt.cmdBufferSize = 2;

    if (0 != (result = write(fd, &evt, 1))) {
        printf("WriteI2c_Byte Write Error, reg=%02x val=%02x\n", RegAddr, data);
    }
    return result;
}

#ifdef CFG_SARADC_ENABLE
static void* SarAdcTask(void* arg)
{
    SARADC_RESULT result = SARADC_SUCCESS;
    uint16_t writeBuffer_len = SARADC_BUF_LEN;
    uint16_t calibrationOutputOT = 0;
    PeriCmd lightSensorCmd;

    lightSensorCmd.id = PERICMD_SET_SARADC_LIGHT_SENSOR;
    while (!extQuit) {
#ifndef _WIN32
        result = mmpSARConvert(LIGHT_SENSOR_CHN, writeBuffer_len, &calibrationOutputOT);
        if ( result != SARADC_SUCCESS) {
            printf("mmpSARConvert() error (0x%x) !!\n", result);
        } else
#endif
        {
            lightSensorCmd.arg1 = calibrationOutputOT;
            PeripheralSend(&lightSensorCmd);
        }

        usleep(100 * 1000);
    }

#ifndef _WIN32
    result = mmpSARTerminate();
    if (result)
        printf("mmpSARTerminate() error (0x%x) !!\n", result);
#endif
}
#endif

#if 0
static void BrightlightInit()
{
    backlightWithAls = false;
    ithPwmInit(ITH_PWM1, 8000, 5);
    // the third parameter has no side effect for 986x chip.
    ithPwmReset(ITH_PWM1, 62, DONTCARE);
    ithPwmEnable(ITH_PWM1, 62, DONTCARE);
}


static void searchBacklight(unsigned int dutyLevel, bool isEncouraged)
{
    uint8_t trans = (isEncouraged ? 0x3f : 0xFF);
    uint8_t dutyTable[] = {82, 80, 79, 76, 75,
                           74, 70, 64, 60, 56,
                           51, 45, 40, 35, 30,
                           22, 14, 12, 10, 8, 6};

    printf("dutyLevel is: %u, the duty is: %u\n", dutyLevel, dutyTable[dutyLevel]);
    printf("duty after calculation is: %u\n", dutyTable[dutyLevel] & trans);

    if ((dutyTable[dutyLevel] & trans) < dutyTable[dutyLevel]) {
        ithPwmSetDutyCycle(ITH_PWM1, 60 - (dutyTable[dutyLevel] & trans));
        return;
    }

    ithPwmSetDutyCycle(ITH_PWM1, dutyTable[dutyLevel]);
}



bool changeBacklight(unsigned int dutyLevel,int diff)
{
    bool isEncouraged = false;
    if (diff < -1250)
        isEncouraged = true;
    else if (diff > 1550)
        dutyLevel--;

    searchBacklight(dutyLevel, isEncouraged);
    return true;
}

#endif

static void* I2cAlsTask(void* arg)
{
#if 0
    uint32_t lsTemp = 0;
    uint32_t alsData = 0;
    uint32_t oldAlsData = -1;
    uint32_t lux = 0;
    PeriCmd lightSensorCmd;

    lightSensorCmd.id = PERICMD_SET_SARADC_LIGHT_SENSOR;

    while (!extQuit)
    {
#ifndef _WIN32
        //printf("[I2cAlsTask]INT: 0x%X  ", i2cRead(fd, lightSensorSlaveAddr, 0x04));
        //printf("[I2cAlsTask]GAIN:0x%X\n", i2cRead(fd, lightSensorSlaveAddr, 0x05));

        lsTemp = 0;
        for (int addr = 0x0F; addr >= 0x0D ; addr--)
        {
            lsTemp |= i2cRead(fd, lightSensorSlaveAddr, addr);
            lsTemp <<= 8;
        }
#endif
        lux = (6 * lsTemp) / (18 * 4);
        lux /= 10;
        alsData = (lux < 10000) ? lux : 9999;
        if(backlightWithAls)
            changeBacklight(alsData / 550, alsData - oldAlsData);
        if (oldAlsData != alsData) {
            lightSensorCmd.arg1 = alsData;
            PeripheralSend(&lightSensorCmd);
            oldAlsData = alsData;
        }

        usleep(500000);
    }
#endif
}


#if 0
static void OutsideTempInit(void)
{
    SARADC_RESULT result = SARADC_SUCCESS;

    result = mmpSARInitialize(SARADC_MODE_AVG_ENABLE, SARADC_MODE_STORE_AVG_ENABLE,
                    SARADC_AMPLIFY_1X, SARADC_CLK_DIV_9);
    if (result)
        printf("mmpSARInitialize() error (0x%x) !!\n", result);
}
#endif

static void LightSensorInit(void)
{
#ifdef CFG_SARADC_ENABLE
    SARADC_RESULT result = SARADC_SUCCESS;

    result = mmpSARInitialize(
        SARADC_MODE_AVG_ENABLE,
        SARADC_MODE_STORE_AVG_ENABLE,
        SARADC_AMPLIFY_1X,
        SARADC_CLK_DIV_9
    );

    if (result) {
        printf("mmpSARTerminate() error (0x%x) !!\n", result);
    }

#endif

}

#if 0
static void* EepromTestTask(void* arg)
{
    PeriCmd eepromResCmd;
    int result;
    static int result_ok = 0;
    static int result_ng = 0;
    uint32_t tick1, tick2;
    uint32_t delaySec = 15;

    do {
        tick1 = SDL_GetTicks();
        result = EepromTest();

        if (result) {
            result_ng++;
            eepromResCmd.id = PERICMD_SET_EEPROM_NG_RESULT;
            eepromResCmd.arg1 = result_ng;
        } else {
            result_ok++;
            eepromResCmd.id = PERICMD_SET_EEPROM_OK_RESULT;
            eepromResCmd.arg1 = result_ok;
        }
        PeripheralSend(&eepromResCmd);
        tick2 = SDL_GetTicks();
        printf("Eeprom Test time: %dms, delay %ds\n", tick2 - tick1, delaySec);
        sleep(delaySec);
    }while (1);
}
#endif

extern char ssid[WIFI_SSID_MAXLEN];
extern char password[WIFI_PASSWORD_MAXLEN];
extern int gGetInfo;
extern void bt_wifi_get_ip(char *ip, int ip_len);

//Add by Hardy: for PC Tool USB IDB Upgrade Service
static void* UsbdAcmProcessTask(void* arg)
{
    int rlen, wlen;
    static char rbuf[ACM_BUF_LEN];
    static char wbuf[32 * 1024]; // 32KB write buffer for logs

    printf("[USBD ACM] Task Started.\n");

    while (!extQuit) 
    {
        // 檢查 USB 是否已經連接
        if (ioctl(ITP_DEVICE_USBDACM, ITP_IOCTL_IS_CONNECTED, NULL)) 
        {
            memset(rbuf, 0, ACM_BUF_LEN);
            rlen = read(ITP_DEVICE_USBDACM, rbuf, ACM_BUF_LEN);
            
            if (rlen > 0) 
            {
                // Ensure null termination for safe string ops
                if (rlen < ACM_BUF_LEN) {
                    rbuf[rlen] = '\0';
                } else {
                    rbuf[ACM_BUF_LEN - 1] = '\0';
                }
                
                printf("[USBD ACM] read: %d bytes (%s)\n", rlen, rbuf);
                
                // 解析 PC 端命令
                if (strncmp(rbuf, "GET_STATUS", 10) == 0) 
                {
                    char ip[16] = "0.0.0.0";
                    bt_wifi_get_ip(ip, sizeof(ip));
                    bool wifi_connected = (strcmp(ip, "0.0.0.0") != 0);
                    
                    static char raw_log[4 * 1024];
                    static char escaped_log[8 * 1024];
                    log_ring_buffer_pop(raw_log, sizeof(raw_log));
                    escape_json_string(escaped_log, sizeof(escaped_log), raw_log);
                    
                    int len = bt_log_get_status_json(wbuf, sizeof(wbuf), wifi_connected, theConfig.ssid, ip, escaped_log);
                    
                    // 加上換行符以便 PC 端按行讀取
                    if (len < sizeof(wbuf) - 2) {
                        wbuf[len++] = '\n';
                        wbuf[len] = '\0';
                    }
                    wlen = write(ITP_DEVICE_USBDACM, wbuf, len);
                } 
                else if (strncmp(rbuf, "GET_LOGS", 8) == 0) 
                {
                    int len = bt_log_get_all_json(wbuf, sizeof(wbuf));
                    
                    if (len < sizeof(wbuf) - 2) {
                        wbuf[len++] = '\n';
                        wbuf[len] = '\0';
                    }
                    wlen = write(ITP_DEVICE_USBDACM, wbuf, len);
                } 
                else if (strncmp(rbuf, "SCAN_WIFI", 9) == 0)
                {
                    // 交由 NetworkWifiProcess 在自己的 thread 中執行掃描（thread-safe）
                    extern volatile bool g_wifi_scan_requested;
                    g_wifi_scan_requested = true;
                    printf("[USBD ACM] WiFi scan requested, NetworkWifiProcess will handle it\n");
                    int pos = snprintf(wbuf, sizeof(wbuf), "{\"cmd\":\"SCAN_WIFI\",\"status\":\"SCANNING\"}\n");
                    wlen = write(ITP_DEVICE_USBDACM, wbuf, pos);
                }
                else if (strncmp(rbuf, "SET_WIFI:", 9) == 0) 
                {
                    // 格式: SET_WIFI:<ssid>,<password>
                    char *params = rbuf + 9;
                    char *comma = strchr(params, ',');
                    if (comma) {
                        *comma = '\0';
                        char *pwd_end = strchr(comma + 1, '\r');
                        if (!pwd_end) pwd_end = strchr(comma + 1, '\n');
                        if (pwd_end) *pwd_end = '\0';
                        
                        strncpy(ssid, params, WIFI_SSID_MAXLEN - 1);
                        strncpy(password, comma + 1, WIFI_PASSWORD_MAXLEN - 1);
                        
                        printf("[USBD ACM] Setting WiFi SSID: %s, PWD: %s\n", ssid, password);

                        // 將目前的 primary AP 移到 backup，再儲存新的 primary
                        strncpy(theConfig.ap_ssid, theConfig.ssid, sizeof(theConfig.ap_ssid) - 1);
                        strncpy(theConfig.ap_password, theConfig.password, sizeof(theConfig.ap_password) - 1);
                        strncpy(theConfig.ssid, ssid, sizeof(theConfig.ssid) - 1);
                        strncpy(theConfig.password, password, sizeof(theConfig.password) - 1);
                        ConfigSave();
                        usleep(500000);  // 等 ConfigSave 的 async NOR write 完成

                        gGetInfo = 1; // 觸發 network_wifi 執行緒進行 AP 切換與連線
                        
                        snprintf(wbuf, sizeof(wbuf), "{\"cmd\":\"SET_WIFI\",\"status\":\"OK\"}\n");
                    } else {
                        snprintf(wbuf, sizeof(wbuf), "{\"cmd\":\"SET_WIFI\",\"status\":\"ERROR_INVALID_FORMAT\"}\n");
                    }
                    wlen = write(ITP_DEVICE_USBDACM, wbuf, strlen(wbuf));
                }
                else if (strncmp(rbuf, "SET_WIFI2:", 10) == 0)
                {
                    char *params = rbuf + 10;
                    char *comma = strchr(params, ',');
                    if (comma) {
                        *comma = '\0';
                        char *pwd_end = strchr(comma + 1, '\r');
                        if (!pwd_end) pwd_end = strchr(comma + 1, '\n');
                        if (pwd_end) *pwd_end = '\0';
                        strncpy(theConfig.ssid2, params, WIFI_SSID_MAXLEN - 1);
                        strncpy(theConfig.password2, comma + 1, WIFI_PASSWORD_MAXLEN - 1);
                        printf("[USBD ACM] WiFi2 set: %s\n", theConfig.ssid2);
                        ConfigSave();
                        usleep(500000);
                        snprintf(wbuf, sizeof(wbuf), "{\"cmd\":\"SET_WIFI2\",\"status\":\"OK\"}\n");
                    } else {
                        snprintf(wbuf, sizeof(wbuf), "{\"cmd\":\"SET_WIFI2\",\"status\":\"ERROR_INVALID_FORMAT\"}\n");
                    }
                    wlen = write(ITP_DEVICE_USBDACM, wbuf, strlen(wbuf));
                }
                else if (strncmp(rbuf, "SET_TIME:", 9) == 0)
                {
                    // 格式: SET_TIME:2026-07-05T10:04:19
                    int year, month, day, hour, minute, second;
                    if (sscanf(rbuf + 9, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6)
                    {
                        struct tm tm_time;
                        memset(&tm_time, 0, sizeof(struct tm));
                        tm_time.tm_year = year - 1900;
                        tm_time.tm_mon  = month - 1;
                        tm_time.tm_mday = day;
                        tm_time.tm_hour = hour;
                        tm_time.tm_min  = minute;
                        tm_time.tm_sec  = second;
                        
                        time_t t = mktime(&tm_time);
                        
                        // 使用 ITE RTC API 設定系統時間（比 POSIX settimeofday 更可靠）
                        itpRtcSetTime((long)t, 0);
                        
                        // 同步 POSIX clock (tzset 後讓 time(NULL) 也能讀到正確時間)
                        {
                            struct timeval tv;
                            tv.tv_sec  = t;
                            tv.tv_usec = 0;
                            settimeofday(&tv, NULL);
                        }
                        
                        printf("[USBD ACM] Setting Time via RTC: %04d-%02d-%02d %02d:%02d:%02d (unix=%ld)\n",
                               year, month, day, hour, minute, second, (long)t);
                        snprintf(wbuf, sizeof(wbuf), "{\"cmd\":\"SET_TIME\",\"status\":\"OK\"}\n");
                    }
                    else
                    {
                        snprintf(wbuf, sizeof(wbuf), "{\"cmd\":\"SET_TIME\",\"status\":\"ERROR_INVALID_FORMAT\"}\n");
                    }
                    wlen = write(ITP_DEVICE_USBDACM, wbuf, strlen(wbuf));
                }
                else if (strncmp(rbuf, "REBOOT", 6) == 0 && (rbuf[6] == '\r' || rbuf[6] == '\n'))
                {
                    printf("[USBD ACM] REBOOT command received, restarting...\n");
                    snprintf(wbuf, sizeof(wbuf), "{\"cmd\":\"REBOOT\",\"status\":\"OK\"}\n");
                    wlen = write(ITP_DEVICE_USBDACM, wbuf, strlen(wbuf));
                    // Wait for pending ConfigSave to complete (async thread writes to NOR)
                    usleep(3000000);  // 3 seconds
                    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_EXIT, NULL);
                    usleep(100000);
                    exit(0);
                }
                else if (strncmp(rbuf, "OTA:", 4) == 0)
                {
                    // OTA firmware update via USB: OTA:http://host/ROM
                    extern char g_ota_url[256];
                    extern volatile bool g_ota_pending;
                    char *url = rbuf + 4;
                    char *end = strchr(url, '\r');
                    if (!end) end = strchr(url, '\n');
                    if (end) *end = '\0';
                    strncpy(g_ota_url, url, sizeof(g_ota_url) - 1);
                    g_ota_url[sizeof(g_ota_url) - 1] = '\0';
                    g_ota_pending = true;
                    printf("[USBD ACM] OTA URL: %s\n", g_ota_url);
                    snprintf(wbuf, sizeof(wbuf), "{\"cmd\":\"OTA\",\"status\":\"ACCEPTED\"}\n");
                    wlen = write(ITP_DEVICE_USBDACM, wbuf, strlen(wbuf));
                }
                else if (strncmp(rbuf, "REBOOT_TO_BOOTLOADER", 20) == 0)
                {
                    printf("[USBD ACM] Command REBOOT_TO_BOOTLOADER received! Resetting via Watchdog...\n");
                    snprintf(wbuf, sizeof(wbuf), "{\"cmd\":\"REBOOT_TO_BOOTLOADER\",\"status\":\"OK\"}\n");
                    wlen = write(ITP_DEVICE_USBDACM, wbuf, strlen(wbuf));
                    
                    // Delay 100ms to allow USBD ACM Tx to complete flushing
                    usleep(100000);
                    
                    #ifndef _WIN32
                    ioctl(ITP_DEVICE_WIFI, ITP_IOCTL_EXIT, NULL);
                    usleep(100000);
                    exit(0);
                    #endif
                    while (1);
                }
                else 
                {
                    // 未知命令，原樣回傳
                    wlen = write(ITP_DEVICE_USBDACM, rbuf, rlen);
                }
            }

            // 非同步 WiFi 掃描結果：若掃描完成，立即透過 USB 回傳結果
            {
                pthread_mutex_lock(&s_wifi_scan_mutex);
                if (s_wifi_scan_ready) {
                    wlen = write(ITP_DEVICE_USBDACM, s_wifi_scan_result, s_wifi_scan_result_len);
                    s_wifi_scan_ready = false;
                    printf("[USBD ACM] WiFi scan result sent to PC.\n");
                }
                pthread_mutex_unlock(&s_wifi_scan_mutex);
            }

        } 
        else 
        {
            // 若未連接，稍微等待避免咬死 CPU
            sleep(1);
        }
    }
    
    printf("[USBD ACM] Task Exited.\n");
    return NULL;
}

void PeripheralInit(void)
{
    struct mq_attr attr;

    attr.mq_flags = 0;
    attr.mq_maxmsg = PERI_MAX_QUEUE_SIZE;
    attr.mq_msgsize = sizeof(PeriCmd);

    printf(">>> %s\n", __func__);
    inQueue = mq_open("peripheral", O_CREAT | O_NONBLOCK, 0644, &attr);
    assert(inQueue != -1);

    extQuit = false;

#ifndef _WIN32
    //OutsideTempInit();
    LightSensorInit();
    //EepromInit(&isEEPROMEnable);
    //BrightlightInit();
#if CFG_HW_TEST
    //pthread_create(&eepromTestTask, NULL, EepromTestTask, NULL);
#endif
#endif

//Add by Hardy: for PC Tool USB IDB Upgrade Service
#if defined(CFG_UPGRADE_USB_DEVICE) || defined(CFG_USBD_CD_CDCACM)
    printf("[Peripheral] >>> Initializing USB Device Services <<<\n");
    
    // Force device mode since there is no VBUS detection GPIO
#ifdef CFG_USBHCC
    ithUsbForceDeviceMode(ITH_USB0, true);
#else
    iteUsbExForceDeviceMode(true);
#endif

    itpRegisterDevice(ITP_DEVICE_USBD, &itpDeviceUsbd);
    ioctl(ITP_DEVICE_USBD, ITP_IOCTL_INIT, NULL);

    itpRegisterDevice(ITP_DEVICE_USBDACM, &itpDeviceUsbdAcm);
    ioctl(ITP_DEVICE_USBDACM, ITP_IOCTL_INIT, NULL);
    printf("[Peripheral] USB Device Services are READY.\n");
#endif

#if defined(CFG_UPGRADE_USB_DEVICE)
    printf("[Peripheral] >>> Activating PC Tool USB IDB Upgrade Service <<<\n");
    ioctl(ITP_DEVICE_USBDIDB, ITP_IOCTL_INIT, NULL); 
    ioctl(ITP_DEVICE_USBDIDB, ITP_IOCTL_ENABLE, NULL); 
    printf("[Peripheral] PC Tool USB IDB Upgrade Service is READY.\n");
#endif

#ifdef CFG_SARADC_ENABLE
    pthread_create(&adcTask, NULL, SarAdcTask, NULL);
#endif

    pthread_create(&i2cTask, NULL, I2cAlsTask, NULL);

#if defined(CFG_USBD_CD_CDCACM)
    {
        pthread_attr_t acm_attr;
        pthread_attr_init(&acm_attr);
        pthread_attr_setdetachstate(&acm_attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&acm_attr, 64 * 1024); // 64KB stack for USB ACM task
        pthread_create(&usbdAcmTask, &acm_attr, UsbdAcmProcessTask, NULL);
        pthread_attr_destroy(&acm_attr);
    }
#endif

}

void PeripheralExit(void)
{
    printf(">>> %s\n", __func__);
    extQuit = true;
    mq_close(inQueue);
    inQueue = -1;
}

int PeripheralReceive(PeriCmd *cmd)
{
    assert(cmd);

    if (extQuit)
        return 0;

    if (mq_receive(inQueue, (char*)cmd, sizeof(PeriCmd), 0) > 0)
        return 1;

    return 0;
}

int PeripheralSend(PeriCmd *cmd)
{
    assert(cmd);

    if (extQuit)
        return -1;

    return mq_send(inQueue, (char*)cmd, sizeof(PeriCmd), 0);
}

bool updatePeripheral(void)
{
    PeriCmd peri_cmd;

    while (PeripheralReceive(&peri_cmd)) {
        switch (peri_cmd.id) {
    #if 0
        case PERICMD_SET_ADC_TEMP:
            periMonitor.adc_temp = peri_cmd.arg1;
            //printf("(1)PERICMD_SET_ADC_TEMP:%d\n", periMonitor.adc_temp);
            sceneInjectCustomEvent(EVENT_CUSTOM_PERIPHERAL, PERI_ADC);
            break;
    #endif
        case PERICMD_SET_SARADC_LIGHT_SENSOR:
            periMonitor.als_light_sensor = peri_cmd.arg1;
            break;
    #if 0
        case PERICMD_SET_EEPROM_OK_RESULT:
            //printf("(1)PERICMD_SET_EEPROM_OK_RESULT\n");
            periMonitor.result_ok_eeprom = peri_cmd.arg1;
            sceneInjectCustomEvent(EVENT_CUSTOM_PERIPHERAL, PERI_EEPROM_OK);
            break;
        case PERICMD_SET_EEPROM_NG_RESULT:
            //printf("(1)PERICMD_SET_EEPROM_NG_RESULT\n");
            periMonitor.result_ng_eeprom = peri_cmd.arg1;
            sceneInjectCustomEvent(EVENT_CUSTOM_PERIPHERAL, PERICMD_SET_EEPROM_NG_RESULT);
            break;
    #endif
        }
    }
}

uint32_t getPeriInfo(SOC_PERIPHERAL id)
{
    switch (id)
    {
    #if 0
    case PERI_ADC:
        //printf("(2)PERI_ADC:%d\n", periMonitor.adc_temp);
        return periMonitor.adc_temp;
    #endif
    case PERI_ALS:
        //printf("(2)PERI_ALS:%d\n", periMonitor.als_light_sensor);
        return periMonitor.als_light_sensor;
    #if 0
    case PERI_EEPROM_OK:
        //printf("(2)PERI_EEPROM_OK:%d\n", periMonitor.result_ok_eeprom);
        return periMonitor.result_ok_eeprom;
    case PERI_EEPROM_NG:
        //printf("(2)PERI_EEPROM_NG:%d\n", periMonitor.result_ng_eeprom);
        return periMonitor.result_ng_eeprom;
    #endif
    default:
        break;
    }
}

