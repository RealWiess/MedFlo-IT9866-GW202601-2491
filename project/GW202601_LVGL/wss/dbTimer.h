#ifndef DB_TIMER_H
#define DB_TIMER_H

#include "ite/itp.h"

typedef void* DBTIMER;
typedef void* LIBTIMER;
typedef void* DBCOUNTER;

typedef enum _CounterCommand {
    CC_INCR,
    CC_DECR,
    CC_RAND,
    CC_PINGPONG,
} CounterCommand_e;

typedef enum _CounterDirection {
    CD_UP,
    CD_DOWN,
} CounterDirection_e;

typedef struct _CounterConfig {
    CounterDirection_e dir;
    uint32_t min;
    uint32_t max;
    uint32_t cnt;
    CounterCommand_e cmd;
} CounterConfig_t;

typedef struct _DashboardTimer {
    char* name;
    uint8_t div;
    uint32_t tick;
} DashboardTimer_t;

typedef struct _LibTimer {
    char* name;
    timer_t timer;
} LibTimer_t;

typedef struct _TimerSpec {
    timer_t sec;
    uint32_t nsec;
    bool repeat;
    VOIDFUNCPTR func;
    int arg;
} TimerSpec_t;

typedef struct _DashboardCounter {
    char* name;
    CounterConfig_t* config;
} DashboardCounter_t;


DBTIMER createDbTimer(const char* name, uint8_t timeout);
bool isDbTimerTimesUp(DBTIMER timer);
void printDbTimer(DBTIMER timer);
void deleteDbTimer(DBTIMER timer);

LIBTIMER createLibTimer(const char* name);
void setLibTimer(DBTIMER timer, TimerSpec_t* timerSpec);
void deleteLibTimer(DBTIMER timer);

DBCOUNTER createDbCounter(const char* name, CounterConfig_t* config);
uint32_t getValueFromCounter(DBCOUNTER counter);
void printDbCounter(DBCOUNTER counter);
void deleteDbCounter(DBCOUNTER counter);


#endif
