#include "include/tickapi.h"

void Tickinit (tickParam * gT)
{ // init measure param
    gT->start_t    = itpGetTickCount();
    gT->curr_t     = gT->start_t;
    gT->interval_t = gT->start_t;
    gT->timeout_t  = gT->start_t;
    gT->size_cnt   = 0;
    gT->num_cnt    = 0;
}

void Tickreset (tickParam * gT)
{
    gT->interval_t = itpGetTickCount();
    gT->timeout_t  = itpGetTickCount();
    gT->size_cnt   = 0;
    gT->num_cnt    = 0;
}

uint32_t Tickduration (tickParam * gT)
{ // curr_t-start_t(ms)
    gT->curr_t = itpGetTickDuration(gT->start_t);
    return gT->curr_t;
}

uint32_t Tickinterval (tickParam * gT)
{
    uint32_t interval;
    interval       = itpGetTickDuration(gT->interval_t);
    gT->interval_t = itpGetTickCount();
    return interval;
}

bool Ticktimeout (tickParam * gT, uint32_t ms)
{
    bool     ret = false;
    uint32_t interval;
    interval = itpGetTickDuration(gT->timeout_t);
    if (interval >= ms)
    {
        gT->timeout_t = itpGetTickCount();
        ret           = true;
    }
    return ret;
}

bool Ticknumcount (tickParam * gT, int Ncnt)
{
    bool ret = false;
    if (gT->num_cnt++ >= Ncnt)
    {
        gT->num_cnt = 0;
        ret         = true;
    }
    return ret;
}

int Ticksizecount (tickParam * gT, int bsize, uint32_t Nsize)
{
    int ret = 0;
    gT->size_cnt += bsize;
    ret = gT->size_cnt - Nsize;
    if (gT->size_cnt >= Nsize)
    {
        gT->size_cnt = 0;
    }
    return ret;
}

void Tickuninit (tickParam * gT)
{
    gT->start_t    = 0;
    gT->curr_t     = 0;
    gT->interval_t = 0;
    gT->timeout_t  = 0;
    gT->num_cnt    = 0;
}
