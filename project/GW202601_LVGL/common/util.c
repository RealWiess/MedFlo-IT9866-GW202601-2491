#include "SDL/SDL.h"
#include "ite/itp.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

void swap(uint8_t* front, uint8_t* back)
{
    uint8_t tmp = *front;

    *front = *back;
    *back = tmp;
}

void reverseData2(uint8_t* src, uint8_t len)
{
    for (int i = 0; i < len >> 1; i++) {
        swap(&src[i], &src[len - i - 1]);
    }
}

void reverseData(unsigned char* src, unsigned char* des, unsigned char len)
{
    int i;

    for (i = 0; i < len; i++) {
        des[len - i - 1] = src[i];
    }
}

// Transfer little endian(Intel) to big endian(Mortorola)
uint32_t byteOrderReverse(const uint32_t* buf, size_t size)
{
    int i;
    uint32_t ret = 0;

    if (size > sizeof(uint32_t))
        printf("size is larger than 4\n");
    else {
        ret = (*buf & 0xFF000000) >> 24;
        ret |= (*buf & 0x00FF0000) >> 8;
        ret |= (*buf & 0x0000FF00) << 8;
        ret |= (*buf & 0x000000FF) << 24;
        ret >>= ((sizeof(uint32_t) - size) * 8);
    }
    return ret;
}

int splitHundred(HundredInfo *data)
{
    if (data == NULL)
        return 1;

    if (data->num > 999999)
        return 1;

    data->visible.hundredthuns = 0;
    data->visible.tenthuns = 0;
    data->visible.thuns = 0;
    data->visible.hunds = 0;
    data->visible.tens = 0;
    data->visible.ones = 0;

    data->digit.hundredthuns = 0;
    data->digit.tenthuns = 0;
    data->digit.thuns = 0;
    data->digit.hunds = 0;
    data->digit.tens = 0;
    data->digit.ones = 0;

    if (data->num > 0)
        data->pos = 1;
    else
    {
        data->pos = 0;
        data->num = -data->num;
    }

    if (data->num < 1000000 && data->num >= 100000)
    {
        data->digit.hundredthuns = data->num / 100000;
        data->digit.hundredthuns %= 10;
        data->visible.hundredthuns = 1;
    }
    if (data->num >= 10000)
    {
        data->digit.tenthuns = (data->num % 100000) / 10000;
        data->digit.tenthuns %= 10;
        data->visible.tenthuns = 1;
    }
    if (data->num >= 1000)
    {
        data->digit.thuns = (data->num % 10000) / 1000;
        data->digit.thuns %= 10;
        data->visible.thuns = 1;
    }
    if (data->num >= 100)
    {
        data->digit.hunds = (data->num % 1000) / 100;
        data->digit.hunds %= 10;
        data->visible.hunds = 1;
    }
    if (data->num >= 10)
    {
        data->digit.tens = (data->num % 100) / 10;
        data->digit.tens %= 10;
        data->visible.tens = 1;
    }
    {
        data->digit.ones = data->num % 10;
        data->digit.ones %= 10;
        data->visible.ones = 1;
    }

    return 0;
}

