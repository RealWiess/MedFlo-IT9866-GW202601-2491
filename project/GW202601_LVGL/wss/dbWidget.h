#ifndef DB_WIDGET_H
#define DB_WIDGET_H

#include "ite/itu.h"

typedef void* DBWIGET;

typedef struct _DigitalNumberConfig {
    bool alignRightDigits;
    uint8_t fixedNumberDigits;
    uint8_t numberOfDigits;
    uint32_t maximumValue;
} DigitalNumberConfig_t;

typedef struct _DigitalNumberElements {
    ITUIcon* alignRightHundredIcon;
    ITUIcon* numberIdx5Icon;
    ITUIcon* numberIdx4Icon;
    ITUIcon* numberIdx3Icon;
    ITUIcon* numberIdx2Icon;
    ITUIcon* numberIdx1Icon;
    ITUIcon* numberIdx0Icon;
    ITUIcon** numberSrcIcon;
    ITUSprite* numberUnitSprite;
} DigitalNumberElements_t;

typedef struct _TimeElements {
    /* 3 digits */
    ITUIcon** timeNumShowIcon;
    ITUIcon** timeNumFakeIcon;

    ITUIcon* timeColonIcon;
    ITUSprite* timeMiddaySprite;
} TimeElements_t;


typedef struct _DigitalNumberWidget {
    char* name;
    DigitalNumberConfig_t *config;
    DigitalNumberElements_t *element;
} DigitalNumberWidget_t;

typedef struct _TimeWidget
{
    char* name;
    TimeElements_t *element;
} TimeWidget_t;

typedef struct _MileCalcWidget
{
    char* name;
    uint32_t lastTick;
    uint32_t nowTick;
    uint32_t remainingCentimeter;
} MileCalcWidget_t;

DBWIGET createDigitalNumberWidget(const char* name,
                                    DigitalNumberConfig_t *config,
                                    DigitalNumberElements_t *element);
void configDigitalNumberWidget(DBWIGET widget,
                                    DigitalNumberConfig_t *config,
                                    DigitalNumberElements_t *element);
void displayDigitalNumberWidget(DBWIGET widget, uint32_t value);

void getDigitalClock(uint8_t* hour,   uint8_t* min,
                     uint8_t* day, uint8_t* month, uint32_t* year);
void displayDigitalTime(DBWIGET hourWidget, DBWIGET minuteWidget,
                DBWIGET dayWidget, DBWIGET monthWidget, DBWIGET yearWidget);


DBWIGET createTimeWidget(const char* name, TimeElements_t *element);
DBWIGET createMileCalcWidget(const char* name);

void printDigitalNumberWidget(DBWIGET widget);
void deleteDigitalNumberWidget(DBWIGET widget);
uint32_t mileCalculator(DBWIGET widget, uint32_t speed);
void deleteMileCalcWidget(DBWIGET widget);


#endif
