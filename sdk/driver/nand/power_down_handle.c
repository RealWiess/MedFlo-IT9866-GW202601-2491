#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#ifndef WIN32
    #include "openrtos/FreeRTOS.h"
#endif
#include "ite/itp.h"

#include "inc/power_down_handle.h"

/*********************************************************************

*********************************************************************/
// #define  ENABLE_POWER_DOWN_HANDLE_DBG_MSG
// #define  ENABLE_REACTIVATE_POWER_DOWN_INTR
/********************************************
 * global variables
 ********************************************/
static bool     gPowerDownIntr      = false;
static uint32_t gCurrPwrHdlStatus   = 0;
uint8_t         hdPin               = (uint8_t)GPIO_POWER_HANDLE_PIN;

#ifdef  CFG_NAND_POWER_DOWN_PROTECT
uint8_t EnablePowerDetectFunction   = 1;
#else
uint8_t EnablePowerDetectFunction   = 0;
#endif

/**************************************
** private functions                 **
***************************************/
static void
_pd_isr (
    void * data)
{
    unsigned int    regValue;
    int             pdPin = (int)GPIO_POWER_DETECT_PIN;

#ifdef  ENABLE_POWER_DOWN_HANDLE_DBG_MSG
    // ithPrintf("$in\n");
#endif

    if (ithGpioGet(pdPin) == 0)
    {
        gPowerDownIntr = true;
        ithGpioDisableIntr(pdPin);
        if (hdPin != 255)
        {
            ithGpioClear(GPIO_POWER_HANDLE_PIN);
        }
    }

    ithGpioClearIntr(pdPin);

#ifdef  ENABLE_POWER_DOWN_HANDLE_DBG_MSG
    // ithPrintf("$out(%x)\n",*gpRaSpec->pTouchDownIntr);
#endif
}

void
_initPwrDwnIntr (
    void)
{
    int     pdPin               = (int)GPIO_POWER_DETECT_PIN;
    char    pdGpioTriggerType   = 1;
    char    pdGpioActiveState   = 0;

#ifdef  ENABLE_TOUCH_PANEL_DBG_MSG
    (void)printf("PD isr init in\n");
#endif

    ithEnterCritical();

    ithGpioClearIntr(pdPin);
    ithGpioRegisterIntrHandler(pdPin, (ITHGpioIntrHandler)_pd_isr, NULL);

    if (pdGpioTriggerType == PD_INT_LEVLE_TRIGGER)
    {
        ithGpioCtrlEnable(pdPin, ITH_GPIO_INTR_LEVELTRIGGER);
    }
    else
    {
        ithGpioCtrlDisable(pdPin, ITH_GPIO_INTR_LEVELTRIGGER);
    }

    if (pdGpioTriggerType == PD_INT_EDGE_TRIGGER)                   // if edge trigger
    {
        ithGpioCtrlDisable(pdPin, ITH_GPIO_INTR_BOTHEDGE);          // set as single edge
    }
    if (pdGpioActiveState == PD_ACTIVE_HIGH)
    {
        ithGpioCtrlDisable(pdPin, ITH_GPIO_INTR_TRIGGERFALLING);    // set as rising edge
    }
    else
    {
        ithGpioCtrlEnable(pdPin, ITH_GPIO_INTR_TRIGGERFALLING);     // set as falling edge
    }
    ithIntrEnableIrq(ITH_INTR_GPIO);
    ithGpioEnableIntr(pdPin);

    ithExitCritical();

#ifdef  ENABLE_TOUCH_PANEL_DBG_MSG
    (void)printf("TP init out\n");
#endif
}

void
_initGpioPin (
    void)
{
    int     pwrPin      = (int)GPIO_POWER_DETECT_PIN;
    int     handlePin   = (int)GPIO_POWER_HANDLE_PIN;
    char    s           = 0; // determine the PULL UP/DOWN

    // set power pin as input & pull up
    ithGpioSetMode(pwrPin, ITH_GPIO_MODE0);
    ithGpioSetIn(pwrPin);

    if (1)
    {
        char s = 0;// set GPIO PULL-UP

        if (s)
        {
            ithGpioCtrlDisable(pwrPin, ITH_GPIO_PULL_UP);
        }
        else
        {
            ithGpioCtrlEnable(pwrPin, ITH_GPIO_PULL_UP);
        }

        ithGpioCtrlEnable(pwrPin, ITH_GPIO_PULL_ENABLE);
    }
    ithGpioEnable(pwrPin);

    // 0.set "handlePin" as output-high
    if (hdPin != 255)
    {
        ithGpioSetMode(handlePin, ITH_GPIO_MODE0);
        ithGpioClear(handlePin);
        // ithGpioSet(handlePin);
        ithGpioSetOut(handlePin);
        ithGpioEnable(handlePin);
    }
}

/*
This function is Just for test
*/
void
_reActivatePwrDwnIntr (
    void)
{
#ifdef  ENABLE_REACTIVATE_POWER_DOWN_INTR
    int pdPin = (int)GPIO_POWER_DETECT_PIN;

    (void)printf("ReActivate Power Detect\n");

    if (ithGpioGet(pdPin) != 0)
    {
        (void)printf("ReActivate Power Detect2: r=%x\n", ithGpioGet(pdPin));
        gPowerDownIntr = false;
        ithGpioClearIntr(pdPin);
        ithGpioEnableIntr(pdPin);
    }
#endif
}

bool
_chkIfPwrDwn (
    void)
{
    int     pwrPin      = (int)GPIO_POWER_DETECT_PIN;
    int     handlePin   = (int)GPIO_POWER_HANDLE_PIN;
    bool    pd          = 0;

    // readGpioPin
    pd = gPowerDownIntr;
    // pd = ithGpioGet(pwrPin);

    // (void)printf("pdReg = %X\n",pd);
    if (pd == true)
    {
        // power-down
        if (ithGpioGet(pwrPin))
        {
            (void)printf("	GotPwrPin UP\n");
            _reActivatePwrDwnIntr();
        }

        if (gCurrPwrHdlStatus == 0)
        {
            if (hdPin != 255)
            {
                ithGpioClear(handlePin);
            }
            gCurrPwrHdlStatus = 1;// power-down handle start
            // (void)printf("   @@@ Set GPIO 14 as LOW @@@\n");
        }
        return true;
    }
    else
    {
        // power on status
        if (gCurrPwrHdlStatus)
        {
            (void)printf("	@@@ clear current status @@@\n");
            gCurrPwrHdlStatus = 0;
        }
        return false;
    }
}

/**************************************
** public functions                  **
***************************************/
void
doPowerDownProtectNand (
    int tag)
{
    if (EnablePowerDetectFunction)
    {
        while (_chkIfPwrDwn() == true)
        {
            if (hdPin != 255)
            {
                ithGpioSet(GPIO_POWER_HANDLE_PIN);                 // pull-up Power-Handle Pin
            }
            (void)printf("STOP R/W test For Power-Down:%x\n", tag);
            usleep(10 * 1000);
        }
    }
}

/*
return: 0:OK,  others:FAIL
*/
int
init_power_detect (
    void)
{
    int         result  = 0;
    int         fd;
    uint32_t    nfCnt   = 0;
    int         pwrPin  = (int)GPIO_POWER_DETECT_PIN;

    if (EnablePowerDetectFunction)
    {
        (void)printf("	##### init_power_detect: pwPin=%d, hdPin=%d #####\n", pwrPin, hdPin);

        _initGpioPin();

        _initPwrDwnIntr();

        (void)printf("	##### init_power_detect: finished #####\n");
    }

    return 0;
}
