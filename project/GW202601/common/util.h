#if !defined (_UTIL_H) || defined(CFG_HW_TEST)

#ifndef _UTIL_H
#define _UTIL_H
#endif

typedef struct _HundredDigit
{
    unsigned char ones;
    unsigned char tens;
    unsigned char hunds;
    unsigned char thuns;
    unsigned char tenthuns;
    unsigned char hundredthuns;
} HundredDigit;

typedef struct _HundredInfo
{
    long num;
    long pos;
    HundredDigit visible;
    HundredDigit digit;
} HundredInfo;

void reverseData(unsigned char* src, unsigned char* des, unsigned char len);
uint32_t byteOrderReverse(const uint32_t* buf, size_t size);
int splitHundred(HundredInfo *data);

#endif

