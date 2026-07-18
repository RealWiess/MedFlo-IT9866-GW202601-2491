#include <stdio.h>
#include <stdlib.h>
#include "ite/itp.h"

typedef struct _tickParam
{
    uint32_t start_t;
    uint32_t curr_t;
    uint32_t interval_t;
    uint32_t timeout_t;
    uint32_t size_cnt;
    int      num_cnt;
    
} tickParam;

void Tickinit(tickParam *gT);
void Tickreset(tickParam *gT);
uint32_t Tickduration(tickParam *gT);
uint32_t Tickinterval(tickParam *gT);
bool Ticknumcount(tickParam *gT,int N);
bool Ticktimeout(tickParam *gT,uint32_t ms);
int Ticksizecount(tickParam *gT,int bsize,uint32_t Nsize);
void Tickuninit(tickParam *gT);
