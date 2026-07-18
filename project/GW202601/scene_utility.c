#include "ite/itp.h"
#include "scene.h"
#include <stdlib.h>
#include <string.h>
#include "scene_utility.h"

#if 0
static CustomEventValue customev = {0};

void getCustomEventValue(CustomEventValue *st) {
    memcpy(st, &customev, sizeof(customev));
}
#endif
/**
 * This function must inside main thread
 * Inject custom event with parameter (value, max to 255) into layer
 */
int sceneInjectCustomEvent(int targetCustomEvent, int value) {
    char sceneParam[5] = {0};

    if (value == 0)
        ituSceneSendEvent(&theScene, targetCustomEvent, NULL);
    else if (value < 0 || value > 255)
        return 0;

    if (targetCustomEvent < 0 || targetCustomEvent >= EVENT_CUSTOM_MAX) {
        return 0;
    }
    //printf("[%s]value: %d\n", __func__, value);
    snprintf(sceneParam, sizeof(sceneParam), "%d", value);
    ituSceneSendEvent(&theScene, targetCustomEvent, (char*)sceneParam);
    ituFlushActionQueue(theScene.actionQueue, &theScene.actionQueueLen);
    return 1;
}

/**
 * Convert scene parameter into value.
 * This implement including some fault detection.
 *
 * return 0 as fail, other, return value bigger than 0
 */
unsigned int sceneParameter2Value(char *param) {
    char value[3] = {0};
    int after = 0;
    // charactor ASCII value 0 ~ 9
    for (int i = 0; i < strlen(param); i++) {
        value[i] = param[i];
        if (value[i] < 0x30 || value[i] > 0x39) {
            return 0;
        }
    }
    after = atoi(value);
    if (after < 0) {
        return 0;
    }
    return after;
}

#if 0
static uint32_t lasttick = 0;
static uint32_t timeCounter = 0,timeLastCounter=0;
static uint32_t battery_sen=0;
static uint32_t ther_det=0;
static uint8_t  bat_lcd_cnt=0;
static uint8_t  ther_alert_cnt=0;
static uint8_t  lcd_15sec_cnt = 0;
static uint8_t  lcd_15sec_en = 1;
static uint8_t  bat_lcd_en=1;
static uint8_t  ther_alert_en=1;



#define TimeUnit  100
static uint32_t timeThd = TimeUnit;
#define GPIO_BL_EN   61
enum {
    BATTERY_SEN=1,
    THER_DET=2,
    THER_ADC_LOW=280,
    BATTERY_ADC_LOW_LCD_OFF=1500,
    BATTERY_ADC_LOW_LCD_ON=1590,
    BATTERY_ADC_HIGH_LCD_ON=2740,
    BATTERY_ADC_HIGH_LCD_OFF=2850,
}testDataEnum;

static void sceneUpdateCounter(uint32_t tick)
{
    uint32_t tickgap = 0;
    uint8_t counterGo = 0;

    tickgap = tick - lasttick;
    if (abs(tickgap > TimeUnit) || (!lasttick)) {
        lasttick = tick;
    }
    //tickgap = tick - lasttick;

    if (tickgap >= timeThd){
        counterGo = 1;
        timeThd -= (tickgap - TimeUnit);
        if(timeThd> TimeUnit)
            timeThd = TimeUnit;
    }


    if (counterGo) {
        timeCounter++;
        //printf("%d ms(%d/%d)\n", (timeCounter * TimeUnit), timeThd, tickgap);
        lasttick = tick;
    }
}

static uint32_t getTestData(uint8_t data_id)
{
    if(data_id == BATTERY_SEN)
        return battery_sen;
    else if(data_id == THER_DET)
        return ther_det;
    else{
        return 0;
        printf("%s_unknow_id(%d)\n",__func__,data_id);
    }
}

static uint32_t getTimeCounter(void)
{
    return timeCounter;
}

uint8_t getTherDetAlert(void)
{
    return ther_alert_en;
}


void setTestData(uint8_t data_id,uint32_t data)
{
    if(data_id == BATTERY_SEN)
        battery_sen=data;
    else if(data_id == THER_DET)
        ther_det=data;
    else
        printf("%s_unknow id(%d)=%d\n",__func__,data_id,data);
}

void wiessSceneRunHandler(uint32_t tick)
{
    #ifdef HW_TEST
    uint8_t last_lcd_en = lcd_15sec_en & bat_lcd_en;
    #else
    uint8_t last_lcd_en = bat_lcd_en;
    #endif
    uint8_t last_alert_en=ther_alert_en;
    uint8_t new_lcd_en = 0;
    //uint8_t lcd_en = lcd_15sec_en;
    sceneUpdateCounter(tick);

    if(getTimeCounter()!=timeLastCounter){
        timeLastCounter=getTimeCounter();

        uint32_t bat_val=getTestData(BATTERY_SEN);
        uint32_t ther_val=getTestData(THER_DET);
        #ifdef HW_TEST
        if ((timeLastCounter % 10)==0) {
            lcd_15sec_cnt++;
            if (lcd_15sec_en) {
                if (lcd_15sec_cnt >= 15){
                    lcd_15sec_en = 0;
                    lcd_15sec_cnt = 0;
                }
            }
            else {
                if (lcd_15sec_cnt >= 1) {
                   lcd_15sec_en = 1;
                   lcd_15sec_cnt = 0;
                }
            }
        }
        #endif

        if((bat_val<BATTERY_ADC_LOW_LCD_OFF)||(bat_val>BATTERY_ADC_HIGH_LCD_OFF)){
            if ((bat_lcd_en)&&(bat_val)){
                bat_lcd_cnt++;
            }
            if (bat_lcd_cnt == 10) {
                bat_lcd_en = 0;
                bat_lcd_cnt = 0;
            }
            //printf("BATTERY_LCD_OFF(%d)=%d\n",bat_lcd_cnt,ther_val);
        }else if((bat_val>BATTERY_ADC_LOW_LCD_ON)||(bat_val<BATTERY_ADC_HIGH_LCD_ON)){
            if (!bat_lcd_en) {
                bat_lcd_cnt++;
            }
            if(bat_lcd_cnt==10){
                bat_lcd_en=1;
                bat_lcd_cnt = 0;
            }
            //printf("BATTERY_LCD_ON(%d)=%d\n",bat_lcd_cnt,ther_val);
        } else {
            bat_lcd_cnt = 0;
        }

        if((ther_val<THER_ADC_LOW)&&(ther_val)) {
            ther_alert_en=1;
            //printf("THER_DET HIGH(%d)=%d\n",ther_alert_cnt,ther_val);
        }else{
            ther_alert_en = 0;
            ther_alert_cnt=0;
        }

        #ifdef HW_TEST
        new_lcd_en=bat_lcd_en&lcd_15sec_en;
        #else
        new_lcd_en=bat_lcd_en;
        #endif
        if(last_lcd_en!=new_lcd_en){
            if (new_lcd_en) {
            #ifndef _WIN32
                ithGpioSet(CFG_GPIO_LCD_PWR_EN);
                #ifndef HW_TEST
                ithGpioSet(GPIO_BL_EN);
                #endif
            #endif
                printf("lcd_on(%d)\n",bat_val);
            } else {
            #ifndef _WIN32
                ithGpioClear(CFG_GPIO_LCD_PWR_EN);
                #ifndef HW_TEST
                ithGpioClear(GPIO_BL_EN);
                #endif
            #endif
                printf("lcd_off(%d)\n",bat_val);
            }
        }

        if(last_alert_en != ther_alert_en){
            if(ther_alert_en)
                printf("ther_det alert on(%d)\n",ther_val);
            else
                printf("ther_det alert off(%d)\n",ther_val);
        }


    }
}
#endif
