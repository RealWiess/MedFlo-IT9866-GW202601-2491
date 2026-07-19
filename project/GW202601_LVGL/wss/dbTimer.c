#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "SDL/SDL.h"
#include "ite/itu.h"
#include "dbTimer.h"

#define TICK  (((DashboardTimer_t*)timer)->tick)
#define CNT_CMD (((DashboardCounter_t*)counter)->config->cmd)
#define CNT_MAX (((DashboardCounter_t*)counter)->config->max)
#define CNT_MIN (((DashboardCounter_t*)counter)->config->min)
#define CNT_DIR (((DashboardCounter_t*)counter)->config->dir)
#define COUNTER  (((DashboardCounter_t*)counter)->config->cnt)

DBTIMER createDbTimer(const char* name, uint8_t div)
{
    DashboardTimer_t* timer = (DashboardTimer_t*)malloc(sizeof(DashboardTimer_t));
    timer->name = (char*)malloc(strlen(name)+1);
    strcpy(timer->name, name);
    timer->div = div;
    timer->tick = 0;

    return ((void*)timer);
}

static void countTickDbTimer(DBTIMER timer)
{
    //_timer_ticks = (_timer_ticks + 1) % (((DashboardTimer_t*)timer)->div);

    TICK = ((TICK) + 1) % (((DashboardTimer_t*)timer)->div);

    //printf("%d\n", TICK);
}

bool isDbTimerTimesUp(DBTIMER timer)
{
    if (!timer) {
        printf("error %d\n", __LINE__);
        return false;
    }

    countTickDbTimer(timer);

    if (TICK == 0) {
        return true;
    }
    return false;
}

void printDbTimer(DBTIMER timer)
{
    DashboardTimer_t* dbTimer = (DashboardTimer_t*)timer;

    printf("<<<< Display Timer Information >>>> \n");
    if (timer) {
        printf("Name:%s div:%d\n", dbTimer->name, dbTimer->div);
    }
}

void deleteDbTimer(DBTIMER timer)
{
    printf("%s\n", __func__);

    if (timer) {
        free(((DashboardTimer_t*)timer)->name);
        free(timer);
    }
}

LIBTIMER createLibTimer(const char* name)
{
    LibTimer_t* timer = (LibTimer_t*)malloc(sizeof(LibTimer_t));
    timer->name = (char*)malloc(strlen(name)+1);
    strcpy(timer->name, name);

    timer_create(CLOCK_REALTIME, NULL, &timer->timer);

    return ((void*)timer);
}

void setLibTimer(DBTIMER timer, TimerSpec_t* timerSpec)
{
    LibTimer_t* libTimer = (LibTimer_t*)timer;
    struct itimerspec value;
    int ret;

    timer_connect(libTimer->timer, timerSpec->func, timerSpec->arg);
    if (!timerSpec->repeat) {
        value.it_interval.tv_sec = 0;
        value.it_interval.tv_nsec = 0;                     // one shot
        value.it_value.tv_sec = timerSpec->sec;
        value.it_value.tv_nsec = timerSpec->nsec;
    } else {
        value.it_interval.tv_sec = value.it_value.tv_sec = timerSpec->sec;
        value.it_interval.tv_nsec = value.it_value.tv_nsec = timerSpec->nsec;
    }

    ret = timer_settime(libTimer->timer, 0, &value, NULL);
}

void deleteLibTimer(DBTIMER timer)
{
    printf("%s\n", __func__);

    LibTimer_t* libTimer = (LibTimer_t*)timer;
    timer_delete(libTimer->timer);
    if (timer) {
        free(((DashboardTimer_t*)timer)->name);
        free(timer);
    }
}


DBCOUNTER createDbCounter(const char* name, CounterConfig_t* config)
{
    DashboardCounter_t* counter = (DashboardCounter_t*)malloc(sizeof(DashboardCounter_t));
    counter->name = (char*)malloc(strlen(name)+1);
    strcpy(counter->name, name);

    counter->config = (CounterConfig_t*)malloc(sizeof(CounterConfig_t));
    memcpy(counter->config, config, sizeof(CounterConfig_t));

    if (counter->config->cmd == CC_DECR)
        counter->config->cnt = counter->config->max;
    else if (counter->config->cmd == CC_INCR)
        counter->config->cnt = counter->config->min;
    else if (counter->config->cmd == CC_RAND)
         srand(SDL_GetTicks() + 1);
    else if (counter->config->cmd == CC_PINGPONG) {
        if (counter->config->dir == CD_UP)
            counter->config->cnt = counter->config->min;
        else
            counter->config->cnt = counter->config->max;
    }

    return ((void*)counter);
}

uint32_t getValueFromCounter(DBCOUNTER counter)
{
    CounterConfig_t config;

    if (!counter) {
        printf("error %d\n", __LINE__);
        return 0;
    }

    switch (CNT_CMD)
    {
    case CC_INCR:
        if (COUNTER == CNT_MAX)
            COUNTER = CNT_MIN;
        else
             COUNTER++;
        break;
    case CC_DECR:
        if (COUNTER == CNT_MIN)
            COUNTER = CNT_MAX;
        else
            COUNTER--;
        break;
    case CC_RAND:
        COUNTER =  rand() % CNT_MAX;
        break;
    case CC_PINGPONG:
        if (CNT_DIR == CD_UP)
            COUNTER++;
        else
            COUNTER--;

       if (COUNTER == CNT_MAX || COUNTER == CNT_MIN)
            CNT_DIR ^= true;

        break;
    }

    //printf("%d:%d:%d\n", CNT_MAX, CNT_MIN, CNT_CMD);
    //printf(">>%d\n", COUNTER);

    return COUNTER;
}

void printDbCounter(DBCOUNTER counter)
{
    DashboardCounter_t * dbCounter = (DashboardCounter_t*)counter;
    CounterConfig_t* config = ((DashboardCounter_t*)counter)->config;

    printf("<<<< Display Counter Information >>>> \n");
    if (counter) {
        printf("Name:%s, min:%d, max:%d, cmd:%d\n", dbCounter->name,
            config->min, config->max, config->cmd);
    }
}

void deleteDbCounter(DBCOUNTER counter)
{
    printf("%s\n", __func__);

    if (counter) {
        free(((DashboardCounter_t*)counter)->name);
        free(((DashboardCounter_t*)counter)->config);
        free(counter);
    }
}

