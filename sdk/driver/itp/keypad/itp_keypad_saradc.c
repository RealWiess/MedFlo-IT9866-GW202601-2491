/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * Castor3 keypad module.
 *
 * @author EL
 * @version 1.0
 */
#include <stdlib.h>
#include <string.h>
#include "../itp_cfg.h"
#include "saradc/saradc.h"

#define KEY_NUM 5

typedef enum KEYPAD_KEY_TAG
{
    KEY_0 = 0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    NOKEY,
    DEFAULT     // default key value
} KEYPAD_KEY;

#ifdef CFG_KEYPAD_MULTIPLE
typedef enum KEYPAD2_KEY_TAG
{
    KEY_5 = 5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    NOKEY2,
    DEFAULT2     // default2 key value
} KEYPAD2_KEY;
#endif

typedef struct _KEY_INFO {
    KEYPAD_KEY  key;
    int         count;
} KEY_INFO;

int                 SAMPLE_COUNT = 4;
int                 channel = 0;
static KEYPAD_KEY   pre_key = NOKEY;
#ifdef CFG_KEYPAD_MULTIPLE
int                  channel2 = 0;
static KEYPAD2_KEY   pre_key2 = NOKEY2;
#endif

void
itpKeypadStandby (
    void)
{
    // Enter Standby mode
    (void)printf("[%s]\n", __FUNCTION__);
    SAMPLE_COUNT = 2;
}

void
itpKeypadResume (
    void)
{
    // Leave out Standby mode
    (void)printf("[%s]\n", __FUNCTION__);
    SAMPLE_COUNT = 4;
}

static int
XAINToChannel (
    uint32_t XAINValue)
{
    int channel = 0;

    if (XAINValue & 0x1)
    {
        channel = 0;
    }
    else if (XAINValue & 0x2)
    {
        channel = 1;
    }
    else if (XAINValue & 0x4)
    {
        channel = 2;
    }
    else if (XAINValue & 0x8)
    {
        channel = 3;
    }
    else if (XAINValue & 0x10)
    {
        channel = 4;
    }
    else if (XAINValue & 0x20)
    {
        channel = 5;
    }
    else if (XAINValue & 0x40)
    {
        channel = 6;
    }
    else if (XAINValue & 0x80)
    {
        channel = 7;
    }

    return channel;
}

#ifdef CFG_KEYPAD_MULTIPLE
static int
XAINToChannel2 (
    uint32_t XAINValue)
{
    int channel = 0;

    if (XAINValue & 0x80)
    {
        channel = 7;
    }
    else if (XAINValue & 0x40)
    {
        channel = 6;
    }
    else if (XAINValue & 0x20)
    {
        channel = 5;
    }
    else if (XAINValue & 0x10)
    {
        channel = 4;
    }
    else if (XAINValue & 0x8)
    {
        channel = 3;
    }
    else if (XAINValue & 0x4)
    {
        channel = 2;
    }
    else if (XAINValue & 0x2)
    {
        channel = 1;
    }
    else if (XAINValue & 0x1)
    {
        channel = 0;
    }

    return channel;
}
#endif

int
itpKeypadProbe (
    void)
{
    SARADC_RESULT   result = SARADC_SUCCESS;
    uint16_t        writeBuffer_len = 512;
    uint16_t        calibrationOutput = 0;
    int             count = 1;
    int             i = 0;
    int             tmp_key = 0;
    KEY_INFO        Record[10];
    bool            flag = false;
    int             KeyKing = DEFAULT, CountKing = 0;

    // Init Key_Info struct
    for (i = 0; i < 10; i++)
    {
        Record[i].key   = DEFAULT;
        Record[i].count = 0;
    }

    while (count <= SAMPLE_COUNT)
    {
        // Get SarADC raw data
        if (result = mmpSARConvert(channel, writeBuffer_len, &calibrationOutput))
        {
            (void)printf("mmpSARConvert() error (0x%x) !!\n", result);
            return -1;
        }

        // Key depend on saradc raw data
        if ((calibrationOutput > 3458) && (calibrationOutput < 4002))
        {
            tmp_key = KEY_0;
        }
        else if ((calibrationOutput > 2777) && (calibrationOutput < 3458))
        {
            tmp_key = KEY_3;
        }
        else if ((calibrationOutput > 2118) && (calibrationOutput < 2777))
        {
            tmp_key = KEY_2;
        }
        else if ((calibrationOutput > 1187) && (calibrationOutput < 2118))
        {
            tmp_key = KEY_1;
        }
        else if ((calibrationOutput > 346) && (calibrationOutput < 1187))
        {
            tmp_key = KEY_4;
        }
        else
        {
            tmp_key = NOKEY;
        }

        // (void)printf("[%s] Key=%d PreKey=%d Calibration=%x\n", __FUNCTION__, tmp_key, pre_key, calibrationOutput);

        // Fill up KEY_INFO struct
        i = 0;
        while ((Record[i].key != DEFAULT) && (i < SAMPLE_COUNT))
        {
            if (Record[i].key == tmp_key)
            {
                Record[i].count = Record[i].count + 1;
                flag            = true;
            }
            i++;
        }

        // IF KEY_INFO don't have tmp_key member
        if (flag == false)
        {
            Record[i].key   = tmp_key;
            Record[i].count = Record[i].count + 1;
        }

        if (count == SAMPLE_COUNT)
        {
            // Find the largest count of key
            i = 0;
            while (Record[i].key != DEFAULT && i < SAMPLE_COUNT)
            {
                if (Record[i].count >= CountKing)
                {
                    KeyKing     = Record[i].key;
                    CountKing   = Record[i].count;
                }
                i++;
            }

#if 0
            if (KeyKing != NOKEY)
            {
                (void)printf("KeyKing=%d\n", KeyKing);
                for (i = 0; i < 10; i++)
                {
                    (void)printf("K=%d W=%d\n", Record[i].key, Record[i].count);
                }
            }
#endif

            if ((KeyKing != NOKEY))
            {
                // (void)printf("[%s] Key=%d PreKey=%d Calibration=%x\n", __FUNCTION__, tmp_key, pre_key, calibrationOutput);
                pre_key = KeyKing;

                return KeyKing;
            }

            // Re-init Key_Info struct
            for (i = 0; i < 10; i++)
            {
                Record[i].key   = DEFAULT;
                Record[i].count = 0;
            }

            pre_key = KeyKing;
        }

        count++;
        flag = false;
    }

    return -1;
}

#ifdef CFG_KEYPAD_MULTIPLE
int
itpKeypad2Probe (
    void)
{
    SARADC_RESULT   result = SARADC_SUCCESS;
    uint16_t        writeBuffer_len = 512;
    uint16_t        calibrationOutput = 0;
    int             count = 1;
    int             i = 0;
    int             tmp_key = 0;
    KEY_INFO        Record[10];
    bool            flag = false;
    int             KeyKing = DEFAULT2, CountKing = 0;

    // Init Key_Info struct
    for (i = 0; i < 10; i++)
    {
        Record[i].key   = DEFAULT2;
        Record[i].count = 0;
    }

    while (count <= SAMPLE_COUNT)
    {
        // Get SarADC raw data
        if (result = mmpSARConvert(channel2, writeBuffer_len, &calibrationOutput))
        {
            (void)printf("mmpSARConvert() error (0x%x) !!\n", result);
            return -1;
        }

        // Key depend on saradc raw data
        if ((calibrationOutput > 3458) && (calibrationOutput < 4002))
        {
            tmp_key = KEY_5;
        }
        else if ((calibrationOutput > 2777) && (calibrationOutput < 3458))
        {
            tmp_key = KEY_8;
        }
        else if ((calibrationOutput > 2118) && (calibrationOutput < 2777))
        {
            tmp_key = KEY_7;
        }
        else if ((calibrationOutput > 1187) && (calibrationOutput < 2118))
        {
            tmp_key = KEY_6;
        }
        else if ((calibrationOutput > 346) && (calibrationOutput < 1187))
        {
            tmp_key = KEY_9;
        }
        else
        {
            tmp_key = NOKEY2;
        }

        // (void)printf("[%s] Key=%d PreKey=%d Calibration=%x\n", __FUNCTION__, tmp_key, pre_key, calibrationOutput);

        // Fill up KEY_INFO struct
        i = 0;
        while ((Record[i].key != DEFAULT2) && (i < SAMPLE_COUNT))
        {
            if (Record[i].key == tmp_key)
            {
                Record[i].count = Record[i].count + 1;
                flag            = true;
            }
            i++;
        }

        // IF KEY_INFO don't have tmp_key member
        if (flag == false)
        {
            Record[i].key   = tmp_key;
            Record[i].count = Record[i].count + 1;
        }

        if (count == SAMPLE_COUNT)
        {
            // Find the largest count of key
            i = 0;
            while (Record[i].key != DEFAULT2 && i < SAMPLE_COUNT)
            {
                if (Record[i].count >= CountKing)
                {
                    KeyKing     = Record[i].key;
                    CountKing   = Record[i].count;
                }
                i++;
            }

#if 0
            if (KeyKing != NOKEY)
            {
                (void)printf("KeyKing=%d\n", KeyKing);
                for (i = 0; i < 10; i++)
                {
                    (void)printf("K=%d W=%d\n", Record[i].key, Record[i].count);
                }
            }
#endif

            if ((KeyKing != NOKEY2))
            {
                // (void)printf("[%s] Key=%d PreKey=%d Calibration=%x\n", __FUNCTION__, tmp_key, pre_key, calibrationOutput);
                pre_key2 = KeyKing;

                return KeyKing;
            }

            // Re-init Key_Info struct
            for (i = 0; i < 10; i++)
            {
                Record[i].key   = DEFAULT2;
                Record[i].count = 0;
            }

            pre_key2 = KeyKing;
        }

        count++;
        flag = false;
    }

    return -1;
}
#endif

void
itpKeypadInit (
    void)
{
    SARADC_RESULT   result = SARADC_SUCCESS;
    uint16_t        range_min = 0, range_max = 0;

    result = mmpSARInitialize(SARADC_MODE_AVG_ENABLE, SARADC_MODE_STORE_RAW_ENABLE, SARADC_AMPLIFY_1X, SARADC_CLK_DIV_9);
    if (result)
    {
        (void)printf("mmpSARInitialize() error (0x%x) !!\n", result);
        return;
    }

    channel = XAINToChannel(CFG_SARADC_VALID_XAIN);
#ifdef CFG_KEYPAD_MULTIPLE
    channel2 = XAINToChannel2(CFG_SARADC_VALID_XAIN);
#endif
}

int
itpKeypadGetMaxLevel (
    void)
{
    return KEY_NUM;
}
