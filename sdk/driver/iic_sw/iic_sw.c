/*
   This file is part of XXX

   Information here.
 */
/**
 * @file iic_sw.c
 * @brief
 * @author
 * @author
 */

/********************************************
* Include Header
********************************************/
#include <stdio.h>
#include <pthread.h>
#include "iic_sw.h"

/********************************************
* MACRO defination
********************************************/
#define IIC_SW_VERSION_2

#define SCLK_High() {GpioSetHigh_(IicDevice.gpioCLK); IIC_Delay_(IicDevice.delayTime);}
#define SCLK_Low()  {GpioSetLow_(IicDevice.gpioCLK);  IIC_Delay_(IicDevice.delayTime);}
#define SDA_High()  {GpioSetHigh_(IicDevice.gpioDATA); IIC_Delay_(IicDevice.delayTime);}
#define SDA_Low()   {GpioSetLow_(IicDevice.gpioDATA); IIC_Delay_(IicDevice.delayTime);}

/********************************************
* Definition
********************************************/
typedef enum _PIN_STATE
{
    PIN_LOW,
    PIN_HIGH
} PIN_STATE;

typedef struct tagIIC_DEVICE
{
    int32_t         refCount;
    pthread_mutex_t iicMutex;
    uint8_t         gpioCLK;
    uint8_t         gpioDATA;
    uint32_t        delayTime;
} IIC_DEVICE;

/********************************************
* Global variable
********************************************/
static IIC_DEVICE IicDevice = {0, PTHREAD_MUTEX_INITIALIZER};

/********************************************
* Prinvate function declaration
********************************************/

/********************************************
* Prinvate function definition
********************************************/

/**
 * Function Brief Here
 *
 * @param XXX XXX
 * @return XXX
 */
static void
GpioSetHigh_ (
    uint32_t pin)
{
    ithGpioSetIn(pin);
}

static void
GpioSetLow_ (
    uint32_t pin)
{
    ithGpioClear(pin);
    ithGpioSetOut(pin);
}

static PIN_STATE
SCLK_State ()
{
    uint32_t value = 0;

    value = ithGpioGet(IicDevice.gpioCLK);
    if (value)
    {
        return PIN_HIGH;
    }
    return PIN_LOW;
}

static PIN_STATE
SDA_State ()
{
    uint32_t value = 0;

    value = ithGpioGet(IicDevice.gpioDATA);
    if (value)
    {
        return PIN_HIGH;
    }
    return PIN_LOW;
}

static void
WaitSCLKState (
    PIN_STATE state)
{
    if (state == PIN_HIGH)
    {
        while (SCLK_State() == PIN_LOW)
        {
        }
    }
    else
    {
        while (SCLK_State() == PIN_HIGH)
        {
        }
    }
}

static void
WaitSDAState (
    PIN_STATE state)
{
    if (state == PIN_HIGH)
    {
        while (SDA_State() == PIN_LOW)
        {
        }
    }
    else
    {
        while (SDA_State() == PIN_HIGH)
        {
        }
    }
}

static void
IIC_Delay_ (
    uint32_t loop)
{
#if 1
    ithDelay(loop);
#else
    uint32_t _loop = loop;

    while (_loop)
    {
        _loop--;
    }
#endif
}

#ifdef IIC_SW_VERSION_2

static IIC_RESULT
IIC_WaitAck_ ()
{
    uint32_t ack = 0xFFFFFFFF;

    IIC_Delay_(IicDevice.delayTime);    // Delay 7

    // SCLK pull high
    GpioSetHigh_(IicDevice.gpioCLK);
    IIC_Delay_(IicDevice.delayTime);    // Delay 7

    // Get SDA data
    ack = ithGpioGet(IicDevice.gpioDATA);

    // SCLK pull low
    GpioSetLow_(IicDevice.gpioCLK);
    IIC_Delay_(IicDevice.delayTime);    // Delay 8

    if (ack == 0)
    {
        return IIC_RESULT_OK;
    }

    return IIC_RESULT_ERROR;
}

#else

static IIC_RESULT
IIC_WaitAck_ ()
{
    uint32_t ack = 0;

    // IIC_Delay_(IicDevice.delayTime);

    GpioSetHigh_(IicDevice.gpioDATA);
    IIC_Delay_(IicDevice.delayTime);

    // SCLK pull high
    GpioSetHigh_(IicDevice.gpioCLK);
    IIC_Delay_(IicDevice.delayTime);

    // Get SDA data
    ack = ithGpioGet(IicDevice.gpioDATA);

    // SCLK pull low
    GpioSetLow_(IicDevice.gpioCLK);
    IIC_Delay_(IicDevice.delayTime);

    (void)printf("ack = 0x%08X\n", ack);

    if ((ack & (1 << IicDevice.gpioDATA)) == 0)
    {
        return IIC_RESULT_OK;
    }

    return IIC_RESULT_ERROR;

    // Set to input mode
    ithGpioSetIn(IicDevice.gpioDATA);

    #if 1
    IIC_Delay_(IicDevice.delayTime);
    ack = ithGpioGet(IicDevice.gpioDATA);
    #else
    ithGpioSet(IicDevice.gpioDATA);
    #endif
    IIC_Delay_(IicDevice.delayTime);

    ithGpioClear(IicDevice.gpioCLK);
    IIC_Delay_(IicDevice.delayTime);

    ithGpioSet(IicDevice.gpioCLK);
    IIC_Delay_(IicDevice.delayTime);

    ack = ithGpioGet(IicDevice.gpioDATA);
    IIC_Delay_(IicDevice.delayTime);

    ithGpioClear(IicDevice.gpioCLK);

    // Set to input mode
    ithGpioSetIn(IicDevice.gpioDATA);

    if ((ack & IicDevice.gpioDATA) == 0)
    {
        return IIC_RESULT_OK;
    }

    return IIC_RESULT_ERROR;
}

#endif

static IIC_RESULT
IIC_SendAck_ (
    bool sendAck)
{
    GpioSetLow_(IicDevice.gpioCLK);
    IIC_Delay_(IicDevice.delayTime);

    if (sendAck)
    {
        GpioSetLow_(IicDevice.gpioDATA);
        IIC_Delay_(IicDevice.delayTime);
    }
    else // NACK
    {
        GpioSetHigh_(IicDevice.gpioDATA);
        IIC_Delay_(IicDevice.delayTime);
    }

    GpioSetHigh_(IicDevice.gpioCLK);
    IIC_Delay_(IicDevice.delayTime);

    GpioSetLow_(IicDevice.gpioCLK);
    IIC_Delay_(IicDevice.delayTime);

    return IIC_RESULT_OK;
}

static void
IIC_Start_ ()
{
    // Set GPIO to Mode 0
    ithGpioSetMode(IicDevice.gpioCLK,  ITH_GPIO_MODE0);
    ithGpioSetMode(IicDevice.gpioDATA, ITH_GPIO_MODE0);
    // Set GPIO data to 0
    ithGpioSetDataOut(IicDevice.gpioCLK);
    ithGpioSetDataOut(IicDevice.gpioDATA);

    // SCLK/DATA pull high
    GpioSetHigh_(IicDevice.gpioCLK);
    GpioSetHigh_(IicDevice.gpioDATA);
    IIC_Delay_(IicDevice.delayTime);

    ithGpioSetOut(IicDevice.gpioCLK);
    ithGpioSetOut(IicDevice.gpioDATA);

    // DATA pull low
    GpioSetLow_(IicDevice.gpioDATA);
    IIC_Delay_(IicDevice.delayTime);

    // SCLK pull low
    GpioSetLow_(IicDevice.gpioCLK);
    IIC_Delay_(IicDevice.delayTime);
}

static void
_IIC_Stop ()
{
#if 1
    IIC_Delay_(IicDevice.delayTime);
    GpioSetLow_(IicDevice.gpioDATA);
    IIC_Delay_(IicDevice.delayTime);
    GpioSetHigh_(IicDevice.gpioCLK);
    IIC_Delay_(IicDevice.delayTime);
    GpioSetHigh_(IicDevice.gpioDATA);
    IIC_Delay_(IicDevice.delayTime);

    WaitSCLKState(PIN_HIGH);
    WaitSDAState(PIN_HIGH);
#else
    // Set pins output
    // ithGpioSetOut(IicDevice.gpioCLK);
    // ithGpioSetOut(IicDevice.gpioDATA);

    // SCLK/DATA pull low
    GpioSetLow_(IicDevice.gpioCLK);
    GpioSetLow_(IicDevice.gpioDATA);
    IIC_Delay_(IicDevice.delayTime);

    // SCLK pull high
    GpioSetHigh_(IicDevice.gpioCLK);
    IIC_Delay_(IicDevice.delayTime);

    // DATA pull high
    GpioSetHigh_(IicDevice.gpioDATA);
    IIC_Delay_(IicDevice.delayTime);
#endif
}

#ifdef IIC_SW_VERSION_2

static IIC_RESULT
IIC_SendByte_ (
    uint8_t data)
{
    static uint32_t ccc = 0;
    uint8_t         i   = 0;

    ++ccc;

    for (i = 0; i < 8; i++)
    {
        if (data & 0x80)
        {
            GpioSetHigh_(IicDevice.gpioDATA);
        }
        else
        {
            GpioSetLow_(IicDevice.gpioDATA);
        }
        GpioSetHigh_(IicDevice.gpioCLK);
        IIC_Delay_(IicDevice.delayTime);    // Delay 4

        IIC_Delay_(IicDevice.delayTime);    // Delay 5
        if (i == 0)
        {
            WaitSCLKState(PIN_HIGH);
        }

        GpioSetLow_(IicDevice.gpioCLK);
        if (i == 7)
        {
            GpioSetHigh_(IicDevice.gpioDATA);
        }
        IIC_Delay_(IicDevice.delayTime);    // Delay 6

        data <<= 1;
    }

    if (IIC_WaitAck_() != IIC_RESULT_OK)
    {
        (void)printf("Write to device error(NO ACK)\n");
        return IIC_RESULT_ERROR;
    }
    return IIC_RESULT_OK;
}

#else

static IIC_RESULT
IIC_SendByte_ (
    uint8_t data)
{
    uint8_t i = 0;

    (void)printf("IIC_SendByte_(), data = 0x%X\n", data);

    IIC_Delay_(IicDevice.delayTime);    // Delay 4
    // send address
    for (i = 0; i < 8; i++)
    {
        if (data & 0x80)
        {
            GpioSetHigh_(IicDevice.gpioDATA);
            (void)printf("1");
        }
        else
        {
            GpioSetLow_(IicDevice.gpioDATA);
            (void)printf("0");
        }
        IIC_Delay_(IicDevice.delayTime);    // Delay 5

        // CLK pull low, DATA prepare
        GpioSetLow_(IicDevice.gpioCLK);
        IIC_Delay_(IicDevice.delayTime);    // Delay 6

        // CLK pull high, DATA transfer
        GpioSetHigh_(IicDevice.gpioCLK);
        IIC_Delay_(IicDevice.delayTime);    // Delay 7

        if (i == 7)
        {
            // Set SDA output
            GpioSetHigh_(IicDevice.gpioDATA);
            ithGpioSetIn(IicDevice.gpioDATA);
        }

        // CLK pull low, DATA ready
        GpioSetLow_(IicDevice.gpioCLK);
        IIC_Delay_(IicDevice.delayTime);    // Delay 8

        data <<= 1;
    }

    if (IIC_WaitAck_() != IIC_RESULT_OK)
    {
        IIC_LOG_MSG("Write to device error(NO ACK)\n");

        ithGpioSetOut(IicDevice.gpioDATA);
        return IIC_RESULT_ERROR;
    }

    ithGpioSetOut(IicDevice.gpioDATA);
    return IIC_RESULT_OK;
}

#endif

static IIC_RESULT
IIC_ReceiveByte_ (
    uint8_t * data,
    bool    sendAck)
{
#if 1
    int8_t      i           = 0;
    uint32_t    readBack    = 0x0000;
    uint32_t    receiveData = 0;

    GpioSetHigh_(IicDevice.gpioDATA);
    // ithGpioSetIn(IicDevice.gpioDATA);
    for (i = 7; i >= 0; i--)
    {
        // CLK pull high
        GpioSetHigh_(IicDevice.gpioCLK);
        IIC_Delay_(IicDevice.delayTime);
        if (i == 7)
        {
            WaitSCLKState(PIN_HIGH);
        }

        readBack = ithGpioGet(IicDevice.gpioDATA);
        IIC_Delay_(IicDevice.delayTime);

        // CLK pull low
        GpioSetLow_(IicDevice.gpioCLK);
        IIC_Delay_(IicDevice.delayTime);

        if (readBack)
        {
            receiveData |= (1 << i);
        }
    }

    *data = receiveData;
    // ithGpioSetOut(IicDevice.gpioDATA);
    IIC_SendAck_(sendAck);
    return IIC_RESULT_OK;
#else
    uint8_t     i           = 0;
    uint32_t    readBack    = 0x0000;
    // uint32_t receiveData = 0;

    for (i = 0; i < 8; i++)
    {
        GpioSetHigh_(IicDevice.gpioDATA);
        // CLK pull high
        GpioSetHigh_(IicDevice.gpioCLK);
        IIC_Delay_(IicDevice.delayTime);

        readBack = ithGpioGet(IicDevice.gpioDATA);
        // IIC_Delay_(IicDevice.delayTime);

        // CLK pull low
        GpioSetLow_(IicDevice.gpioCLK);
        IIC_Delay_(IicDevice.delayTime);

        if (readBack & (1 << IicDevice.gpioDATA))
        {
            (*data) |= 1;
            // receiveData |= (1 << i);
            (void)printf("*1");
        }
        else
        {
            (void)printf("*0");
        }

        (*data) <<= 1;
    }
    (*data) >>= 1;
    // (void)printf("IIC_ReceiveByte_() get 0x%X\n", receiveData);
    // *data = receiveData;

    IIC_SendAck_(sendAck);
    return IIC_RESULT_OK;
#endif
}

/********************************************
* Public function
********************************************/

/**
 * Function Brief Here
 *
 * @param XXX XXX
 * @return XXX
 */
IIC_RESULT
IIC_Initial (
    uint8_t     gpioCLK,
    uint8_t     gpioDATA,
    uint32_t    delay)
{
    IIC_RESULT iicResult = IIC_RESULT_ERROR;

    pthread_mutex_lock(&IicDevice.iicMutex);
    if (IicDevice.refCount == 0)
    {
        IicDevice.gpioCLK   = gpioCLK;
        IicDevice.gpioDATA  = gpioDATA;
        IicDevice.delayTime = delay;

        (void)printf("IIC SCLK pin = %u\n",  IicDevice.gpioCLK);
        (void)printf("IIC SDA pin  = %u\n",  IicDevice.gpioDATA);
        (void)printf("IIC Delay    = %u\n",  IicDevice.delayTime);

        // Set IicDevice.gpioCLK to mode 0
        ithGpioSetMode(IicDevice.gpioCLK,  ITH_GPIO_MODE0);
        // Set IicDevice.gpioDATA to mode 0
        ithGpioSetMode(IicDevice.gpioDATA, ITH_GPIO_MODE0);

        // Set ouput mode
        ithGpioSetOut(IicDevice.gpioCLK);
        ithGpioSetOut(IicDevice.gpioDATA);
        // Set ouput 0
        ithGpioSetDataOut(IicDevice.gpioCLK);
        ithGpioSetDataOut(IicDevice.gpioDATA);
        // Set input mode
        ithGpioSetIn(IicDevice.gpioCLK);
        ithGpioSetIn(IicDevice.gpioDATA);

        iicResult = IIC_RESULT_OK;
    }
    else
    {
        IIC_LOG_MSG("IIC already inited! reference count = %d\n", IicDevice.refCount + 1);
    }
    IicDevice.refCount++;
    pthread_mutex_unlock(&IicDevice.iicMutex);

    return iicResult;
}

IIC_RESULT
IIC_Terminate ()
{
    IIC_RESULT iicResult = IIC_RESULT_ERROR;

    pthread_mutex_lock(&IicDevice.iicMutex);
    IicDevice.refCount--;
    if (IicDevice.refCount == 0)
    {
        IicDevice.gpioCLK   = 0;
        IicDevice.gpioDATA  = 0;
        IicDevice.delayTime = 0;

        iicResult           = IIC_RESULT_OK;
    }
    else if (IicDevice.refCount < 0)
    {
        IIC_LOG_MSG("Abnormal reference count, reference count = %d\n", IicDevice.refCount);
    }
    else
    {
        IIC_LOG_MSG("Reference count(%d) > 0, skip termination!\n", IicDevice.refCount);
    }
    pthread_mutex_unlock(&IicDevice.iicMutex);

    return iicResult;
}

IIC_RESULT
IIC_Start ()
{
    IIC_RESULT iicResult = IIC_RESULT_OK;

    pthread_mutex_lock(&IicDevice.iicMutex);
    IIC_Start_();
    pthread_mutex_unlock(&IicDevice.iicMutex);

    return iicResult;
}

IIC_RESULT
IIC_Stop ()
{
    IIC_RESULT iicResult = IIC_RESULT_OK;

    pthread_mutex_lock(&IicDevice.iicMutex);
    _IIC_Stop();
    pthread_mutex_unlock(&IicDevice.iicMutex);

    return iicResult;
}

IIC_RESULT
IIC_SendByte (
    uint8_t data)
{
    IIC_RESULT iicResult = IIC_RESULT_OK;

    pthread_mutex_lock(&IicDevice.iicMutex);
    iicResult = IIC_SendByte_(data);
    pthread_mutex_unlock(&IicDevice.iicMutex);

    return iicResult;
}

IIC_RESULT
IIC_RecieveByte (
    uint8_t * data,
    bool    sendAck)
{
    IIC_RESULT iicResult = IIC_RESULT_OK;

    pthread_mutex_lock(&IicDevice.iicMutex);
    iicResult = IIC_ReceiveByte_(data, sendAck);
    pthread_mutex_unlock(&IicDevice.iicMutex);

    return iicResult;
}
