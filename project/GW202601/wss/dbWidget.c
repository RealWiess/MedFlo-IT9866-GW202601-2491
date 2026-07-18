#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include "ite/itu.h"
#include "dbWidget.h"
#include "util.h"

DBWIGET createDigitalNumberWidget(const char* name,
                                    DigitalNumberConfig_t *config,
                                    DigitalNumberElements_t *element)
{
    DigitalNumberWidget_t* widget = (DigitalNumberWidget_t*)malloc(sizeof(DigitalNumberWidget_t));
    widget->name = (char*)malloc(strlen(name)+1);
    strcpy(widget->name, name);

    widget->config = (DigitalNumberConfig_t*)malloc(sizeof(DigitalNumberConfig_t));
    memcpy(widget->config, config, sizeof(DigitalNumberConfig_t));
    widget->element = (DigitalNumberElements_t*)malloc(sizeof(DigitalNumberElements_t));
    memcpy(widget->element, element, sizeof(DigitalNumberElements_t));

    return ((void*)widget);
}

DBWIGET createTimeWidget(const char* name, TimeElements_t *element)
{
    TimeWidget_t* widget = (TimeWidget_t*)malloc(sizeof(TimeWidget_t));
    widget->name = (char*)malloc(strlen(name)+1);
    widget->element = element;

    return ((void*)widget);
}

DBWIGET createMileCalcWidget(const char* name)
{
    MileCalcWidget_t* widget = (MileCalcWidget_t*)malloc(sizeof(MileCalcWidget_t));
    widget->name = (char*)malloc(strlen(name)+1);
    widget->lastTick = 0;
    widget->nowTick = 0;
    widget->remainingCentimeter = 0;

    return ((void*)widget);
}

static void DigitalNumberDisplayImpl(DigitalNumberConfig_t *config,
                                         DigitalNumberElements_t *element,
                                         uint32_t value)
{
    int i;
    HundredInfo info;

    if (!element->numberSrcIcon) {
        printf("(%d)%s widget is not ready!\n", __LINE__, __func__);
        return;
    }

    if (value > config->maximumValue) {
        printf("(%d)%s given value %d is exceeded(%d)!\n", __LINE__, __func__, value, config->maximumValue);
        return;
    }

    info.num = value;
    splitHundred(&info);

    if (config->fixedNumberDigits >= 1)
        info.visible.ones = true;
    if (config->fixedNumberDigits >= 2)
        info.visible.tens = true;
    if (config->fixedNumberDigits >= 3)
        info.visible.hunds = true;
    if (config->fixedNumberDigits >= 4)
        info.visible.thuns = true;
    if (config->fixedNumberDigits >= 5)
        info.visible.tenthuns = true;
    if (config->fixedNumberDigits >= 6)
        info.visible.hundredthuns = true;

    if (config->numberOfDigits >= 2) {
        //assert(element->numberIdx1Icon);
        //assert(element->numberIdx0Icon);
        ituWidgetSetVisible(element->numberIdx1Icon, info.visible.tens);
        ituWidgetSetVisible(element->numberIdx0Icon, info.visible.ones);
        ituIconLinkSurface(element->numberIdx1Icon, element->numberSrcIcon[info.digit.tens]);
        ituIconLinkSurface(element->numberIdx0Icon, element->numberSrcIcon[info.digit.ones]);
    } else {
        ituWidgetSetVisible(element->numberIdx0Icon, info.visible.ones);
        ituIconLinkSurface(element->numberIdx0Icon, element->numberSrcIcon[info.digit.ones]);
    }

    if (config->numberOfDigits >= 3) {
        //assert(element->numberIdx2Icon);
        ituWidgetSetVisible(element->numberIdx2Icon, info.visible.hunds);
        if (!config->alignRightDigits)
            ituIconLinkSurface(element->numberIdx2Icon, element->numberSrcIcon[info.digit.hunds]);
    }

    if (config->numberOfDigits >= 4) {
        //assert(element->numberIdx3Icon);
        ituWidgetSetVisible(element->numberIdx3Icon, info.visible.thuns);
        ituIconLinkSurface(element->numberIdx3Icon, element->numberSrcIcon[info.digit.thuns]);
    }

    if (config->numberOfDigits >= 5) {
        //assert(element->numberIdx4Icon);
        ituWidgetSetVisible(element->numberIdx4Icon, info.visible.tenthuns);
        ituIconLinkSurface(element->numberIdx4Icon, element->numberSrcIcon[info.digit.tenthuns]);
    }

    if (config->numberOfDigits >= 6) {
        //assert(element->numberIdx5Icon);
        ituWidgetSetVisible(element->numberIdx5Icon, info.visible.hundredthuns);
        ituIconLinkSurface(element->numberIdx5Icon, element->numberSrcIcon[info.digit.hundredthuns]);
    }
}

void configDigitalNumberWidget(DBWIGET widget,
                               DigitalNumberConfig_t *config,
                               DigitalNumberElements_t *element)
{
    DigitalNumberWidget_t* self = (DigitalNumberWidget_t*)widget;

    memcpy(self->config, config, sizeof(DigitalNumberConfig_t));
    memcpy(self->element, element, sizeof(DigitalNumberElements_t));
}

void displayDigitalNumberWidget(DBWIGET widget, uint32_t value)
{
    DigitalNumberElements_t element;
    DigitalNumberConfig_t config;

    if (!widget) {
        printf("[%s]error %d\n", __func__, __LINE__);
        return;
    }

    memcpy(&element, ((DigitalNumberWidget_t*)widget)->element, sizeof(element));
    memcpy(&config, ((DigitalNumberWidget_t*)widget)->config, sizeof(config));

    DigitalNumberDisplayImpl(&config, &element, value);

}

void getDigitalClock(uint8_t* hour,   uint8_t* min,
                     uint8_t* day, uint8_t* month, uint32_t* year)
{
    struct timeval tp;
    struct tm* ptm;

    gettimeofday(&tp, NULL);
    ptm = localtime(&tp.tv_sec);
    //printf("y-m:%d-%d\n", ptm->tm_year, ptm->tm_mon);
    *hour = ptm->tm_hour;
    *min = ptm->tm_min;
    *year = ptm->tm_year + 1900;
    *month = ptm->tm_mon + 1;
    *day = ptm->tm_mday;
}

void printDigitalClock()
{
    //printf("%s:%s\n", *hstring, *mstring);
}

void displayDigitalTime(DBWIGET hourWidget, DBWIGET minuteWidget,
                DBWIGET dayWidget, DBWIGET monthWidget, DBWIGET yearWidget)
{
    uint8_t hour;
    uint8_t min;
    uint8_t day;
    uint8_t month;
    uint32_t year;

    getDigitalClock(&hour, &min, &day, &month, &year);
    if (hourWidget)
        displayDigitalNumberWidget(hourWidget, hour);
    if (minuteWidget)
        displayDigitalNumberWidget(minuteWidget, min);
    if (dayWidget)
        displayDigitalNumberWidget(dayWidget, day);
    if (monthWidget)
        displayDigitalNumberWidget(monthWidget, month);
    if (yearWidget)
        displayDigitalNumberWidget(yearWidget, year);
}

void printDigitalNumberWidget(DBWIGET widget)
{
    DigitalNumberWidget_t * dnWidget = (DigitalNumberWidget_t*)widget;
    DigitalNumberConfig_t *config = ((DigitalNumberWidget_t*)widget)->config;

    printf("<<<< Display Digital Number Widget Information >>>> \n");
    if (widget) {
        printf("Name:%s, alignRightDigits:%d, fixedNumberDigits:%d, numberOfDigits:%d, maximumValue:%d\n",
            dnWidget->name, config->alignRightDigits,
            config->fixedNumberDigits, config->numberOfDigits,
            config->maximumValue);
    }
}

uint32_t mileCalculator(DBWIGET widget, uint32_t speed)
{
    MileCalcWidget_t* calculator = (MileCalcWidget_t*)widget;
    uint32_t second;
    uint32_t centimeter;
    uint32_t meter = 0;

    if (!calculator) {
        printf("[%s]error %d\n", __func__, __LINE__);
        return;
    }

    if (calculator->lastTick == 0) {
        printf("first speed input!\n", __LINE__);
        calculator->lastTick = SDL_GetTicks();
        return 0;
    }

    calculator->nowTick = SDL_GetTicks();
    //printf("%d-%d\n", calculator->lastTick, calculator->nowTick);

    if (calculator->nowTick - calculator->lastTick > 2000) {
        // calculate odo
        second = (calculator->nowTick - calculator->lastTick) / 1000;

        // 278 cm per 10 seconds
        centimeter = second * 278 * speed;
        centimeter /= 10;
        centimeter += calculator->remainingCentimeter;

        meter = centimeter / 100;
        calculator->remainingCentimeter = centimeter % 100;
        //printf("%d-%d(%d)\n", centimeter, meter, second);

        calculator->lastTick = calculator->nowTick;
    }
    return meter;
}

void deleteDigitalNumberWidget(DBWIGET widget)
{

    if (widget) {
        free(((DigitalNumberWidget_t*)widget)->name);
        free(((DigitalNumberWidget_t*)widget)->config);
        free(((DigitalNumberWidget_t*)widget)->element);
        free(widget);
    }

}

void deleteMileCalcWidget(DBWIGET widget)
{
    printf("%s\n", __func__);

    if (widget) {
        free(((DigitalNumberWidget_t*)widget)->name);
        free(widget);
    }
}

