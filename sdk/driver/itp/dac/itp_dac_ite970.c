#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "ite/ith.h"
#include "ite/itp.h"

#include "ite970/IPGD_table.inc"
#include "ite970/IPGA_table1.inc" // [-27dB(0x0)~-9dB(0x12)]
// #include "ite970/IPGA_table2.inc"   //[-27dB(0x0)~ 0dB(0x1B)]
// #include "ite970/IPGA_table.inc"  //[-27dB(0x0)~36dB(0x3F)]

// #difine FARADAY_DEBUG_PROGRAM_REGVALUE
// #define DEBUG_PRINT (void)printf
#define DEBUG_PRINT(...)

#define I2S_REG_BASE   0xD0100000U

/* ************************************************************************** */
#define MAX_OUT_VOLUME 0x3F
#define MIN_OUT_VOLUME 0x10
static uint8_t current_volstep      = MAX_OUT_VOLUME;
static uint8_t current_volperc      = 99U;
static uint8_t current_volstepRL[2] = {MAX_OUT_VOLUME, MAX_OUT_VOLUME};
static uint8_t current_volpercRL[2] = {99U, 99U};

#define MAX_IN_VOLUME 0x12
#define MIN_IN_VOLUME 0x00
static uint8_t current_micstep      = MAX_IN_VOLUME;
static uint8_t current_micperc      = 99U;
static uint8_t current_micstepRL[2] = {MAX_IN_VOLUME, MAX_IN_VOLUME};
static uint8_t current_micpercRL[2] = {99U, 99U};

// static pthread_mutex_t FARADAY_MUTEX         = PTHREAD_MUTEX_INITIALIZER;

static uint8_t faraday_in_         = -1;
static uint8_t faraday_out_        = -1;
static bool    faraday_DA_running_ = 0;
static bool    faraday_AD_running_ = 0;
static int32_t faraday_sampleRate_ = 48000;

typedef enum _OutPath
{
    HPRpathOnly = 0,
    HPLpathOnly,
    HPRLpathBoth,
    SKPpath
} OutPath;

typedef enum _InPath
{
    MicRpathOnly = 0,
    MicLpathOnly,
    MicRLpathBoth,
} InPath;

/* ************************************************************************** */

static void
faraday_write_reg (
    uint8_t  reg_addr,
    uint32_t value)
{
#ifdef FARADAY_DEBUG_PROGRAM_REGVALUE
    (void)printf("FARADAY# write reg[0x%02x] = 0x%08X\n", reg_addr, value);
#endif
    ithWriteRegA(I2S_REG_BASE | reg_addr, value);
}

static uint32_t
faraday_read_reg (
    uint8_t reg_addr)
{
    uint32_t value = ithReadRegA(I2S_REG_BASE | reg_addr);
#ifdef FARADAY_DEBUG_PROGRAM_REGVALUE
    (void)printf("FARADAY# read reg[0x%02x] = 0x%08X\n", reg_addr, value);
#endif
    return value;
}

#if 0

static void
dump_all_reg_ (
    void)
{
    (void)printf("FARADAY# read reg[0x%02x] = 0x%08X\n", 0xD0, ithReadRegA(I2S_REG_BASE | 0xD0U));
    (void)printf("FARADAY# read reg[0x%02x] = 0x%08X\n", 0xD4, ithReadRegA(I2S_REG_BASE | 0xD4U));
    (void)printf("FARADAY# read reg[0x%02x] = 0x%08X\n", 0xD8, ithReadRegA(I2S_REG_BASE | 0xD8U));
    (void)printf("FARADAY# read reg[0x%02x] = 0x%08X\n", 0xDC, ithReadRegA(I2S_REG_BASE | 0xDCU));
    (void)printf("FARADAY# read reg[0x%02x] = 0x%08X\n", 0xE0, ithReadRegA(I2S_REG_BASE | 0xE0U));
    (void)printf("FARADAY# read reg[0x%02x] = 0x%08X\n", 0xE4, ithReadRegA(I2S_REG_BASE | 0xE4U));
    (void)printf("FARADAY# read reg[0x%02x] = 0x%08X\n", 0xE8, ithReadRegA(I2S_REG_BASE | 0xE8U));
    (void)printf("FARADAY# read reg[0x%02x] = 0x%08X\n", 0xEC, ithReadRegA(I2S_REG_BASE | 0xECU));
    (void)printf("FARADAY# read reg[0x%02x] = 0x%08X\n", 0xF0, ithReadRegA(I2S_REG_BASE | 0xF0U));
    (void)printf("FARADAY# read reg[0x%02x] = 0x%08X\n", 0xF4, ithReadRegA(I2S_REG_BASE | 0xF4U));
    (void)printf("_faraday_micin   = %d -1:NULL\n", faraday_in_);
    (void)printf("_faraday_spkout  = %d -1:NULL\n", faraday_out_);
}
#endif

static inline void
faraday_writeRegMask (
    uint8_t  addr,
    uint32_t data,
    uint32_t mask)
{
    faraday_write_reg(addr, (faraday_read_reg(addr) & ~mask) | (data & mask));
}

#if 0

static void
reset_all_reg_ (
    void)
{
    faraday_write_reg(0xD0U, 0x00000CCFU);
    faraday_write_reg(0xD4U, 0x31181004U);
    faraday_write_reg(0xD8U, 0x00000005U);
    faraday_write_reg(0xDCU, 0x00000000U);
    faraday_write_reg(0xE0U, 0x00003939U);
    faraday_write_reg(0xE4U, 0x61611B1BU);
    faraday_write_reg(0xE8U, 0x00003F3FU);
    faraday_write_reg(0xECU, 0x000000E0U);
    faraday_write_reg(0xF0U, 0x3F3F1000U);
    faraday_write_reg(0xF4U, 0x00002323U);
}
#endif

static void
amp_ctrol (
    bool mute)
{
#if 0
    if (mute)
    {
        ioctl(ITP_DEVICE_AMPLIFIER, ITP_IOCTL_MUTE,   NULL);    // AMP disable
    }
    else
    {
        ioctl(ITP_DEVICE_AMPLIFIER, ITP_IOCTL_UNMUTE, NULL);    // Amp enable
    }
#endif
}

static void
output_path_mute_ (
    bool mute)
{
    amp_ctrol(mute);
    if (mute)
    {
        faraday_writeRegMask(0xD4U, ((uint32_t)0x3U) << 19U, ((uint32_t)0x3U) << 19U);
        return;
    }

    switch (faraday_out_)
    {
        case HPRpathOnly:
        {       /* HP R only */
            faraday_writeRegMask(0xD4U, ((uint32_t)0x1U) << 19U, ((uint32_t)0x3U) << 19U);
            break;
        }

        case HPLpathOnly:
        {       /* HP L only */
            faraday_writeRegMask(0xD4U, ((uint32_t)0x2U) << 19U, ((uint32_t)0x3U) << 19U);
            break;
        }

        case HPRLpathBoth:
        {       /* HP RL both */
            faraday_writeRegMask(0xD4U, ((uint32_t)0x0U) << 19U, ((uint32_t)0x3U) << 19U);
            break;
        }

        case SKPpath:
        {       /* Speaker out*/
            break;
        }

        default:
        {       /* HP RL both */
            faraday_writeRegMask(0xD4U, ((uint32_t)0x0U) << 19U, ((uint32_t)0x3U) << 19U);
            break;
        }
    }
}

static void
input_path_mute_ (
    bool mute)
{
    if (mute)
    {
        faraday_writeRegMask(0xD4U, ((uint32_t)0x3U) << 21U, ((uint32_t)0x3U) << 21U);
        return;
    }

    switch (faraday_in_)
    {
        case MicRpathOnly:
        {     /* mic R only */
            // faraday_writeRegMask(0xE4, 0x6B<<24, 0x3F<<24);//0x6B(+10dB)
            faraday_writeRegMask(0xD4U, ((uint32_t)0x1U) << 21U, ((uint32_t)0x3U) << 21U);
            break;
        }

        case MicLpathOnly:
        {     /* mic L only */
            // faraday_writeRegMask(0xE4, 0x6B<<24, 0x3F<<24);//0x6B(+10dB)
            faraday_writeRegMask(0xD4U, ((uint32_t)0x2U) << 21U, ((uint32_t)0x3U) << 21U);
            break;
        }

        case MicRLpathBoth:
        default:
        {       /* mic RL Both */
            faraday_writeRegMask(0xD4U, ((uint32_t)0x0U) << 21U, ((uint32_t)0x3U) << 21U);
            break;
        }
    }
}

static void
internal_DAC_reset_ (
    void)
{
    uint32_t data;
    DEBUG_PRINT("faraday# %s\n", __func__);

    if (faraday_DA_running_)
    {
        DEBUG_PRINT("faraday# DAC is running, skip re-init process !\n");
        return;
    }
    if (faraday_AD_running_)
    {
        DEBUG_PRINT("faraday# ADC is running, skip re-init process !\n");
        return;
    }

    // reset_all_reg_();

    /*SYSTEM control*/
    data = 0x0U | (0x0U << 14U)                 // Data in slect                   [14:15]:LINEin(0x3) MICin  (0x0)
           | (0x0U << 12U)                      // ?                               [12:16]:
           | (0x3U << 10U)                      // HPOUT Enable(LR)                [10:11]:Enable(0x3) Disable(0x0) (0x1)OnlyHPL (0x2)OnlyHPR
           | (0x0U << 8U)                       // ADC front-end power control(LR) [ 8: 9]:Enable(0x0) Disable(0x3) (0x1)OnlyR   (0x2)OnlyL
           | (0x0U << 6U)                       // DAC power-down control(LR)      [ 6: 7]:Enable(0x0) DIsable(0x3) (0x1)OnlyR   (0x2)OnlyL
           | (0x0U << 4U)                       // ADC power-down control(LR)      [ 4: 5]:Enable(0x0) DIsable(0x3) (0x1)OnlyR   (0x2)OnlyL
           | (0xFU << 0U);                      // MCLK freequency :              [ 0: 3]
    faraday_write_reg(0xD0U, data);

    /*AUDIO control */
    data = 0x0U | (((uint32_t)1U) << 31U)       // ALC zero-crocess                   :(0x1) on
           | (((uint32_t)0x3U) << 29U)          // ADC digital modulator gain  [29:30]:0x0 +1dB,0x1 +1.5dB ,0x2 +2dB ,0x3 +4dB
           | (((uint32_t)0x1U) << 28U)          // ADC moudlator DWA control          :Enable(0x1) Disable(0x0)
           | (((uint32_t)0x0U) << 27U)          // ADC moudlator DEM control          :Enable(0x1) Disable(0x0)
           | (((uint32_t)0x1U) << 26U)          // Rmic  boost                        :(0x1) +20dB ,(0x0) +0dB
           | (((uint32_t)0x1U) << 25U)          // Lmic  boost                        :(0x1) +20dB ,(0x0) +0dB
           | (((uint32_t)0x0U) << 24U)          // speaker driver power               :down(0x1) normal(0x0)
           | (((uint32_t)0x0U) << 23U)          // mic     driver power               :down(0x1) normal(0x0)
           | (((uint32_t)0x0U) << 21U)          // ADC mute(LR)                [21:22]:mute(0x3) unmute(0x0)
           | (((uint32_t)0x3U) << 19U)          // DAC mute(LR)                [19:20]:mute(0x3) unmute(0x0)
           | (((uint32_t)0x0U) << 18U)          // DAC mix control RL channels        :Enable(0x1) DIsable(0x0)
           | (((uint32_t)0x0U) << 16U)          // DAC mix invert              [16:17]:0x00 0x01 0x2 0x3
           | (((uint32_t)0x1U) << 15U)          // ADC HPF                            :Enable(0x1) Disable(0x0)
           | (((uint32_t)0x0U) << 14U)          // ALC function                       :Enable(0x1) Disable(0x0)
           | (((uint32_t)0x1U) << 12U)          // DA IN mode slect            [12:13]:0x00 0x01(I2S) 0x02 0x03
           | (((uint32_t)0x0U) << 10U)          // DAC feeback to ADC          [10:11]:0x0;
           | (((uint32_t)0x0U) << 8U)           // de emphasis                   [8:9]:0x0;
           | (((uint32_t)0x0U) << 7U)           // DAC anti pop                       :enable (0x0) Disable(0x1);
           | (((uint32_t)0x0U) << 5U)           // LINEIN bypass to HPout(LR)         :Enable(0x3) DIsable(0x0)
           | (((uint32_t)0x1U) << 4U)           // hp VCM driver power                :down(0x1) normal(0x0)
           | (((uint32_t)0x0U) << 3U)           // Depop(internal test only)          :
           | (((uint32_t)0x4U) << 0U);          // reference current control     [0:2]:0x4
    faraday_write_reg(0xD4U, data);

    /*ALC control 1 main set*/
    data = 0x0 | (0x3 << 19)                    // ALC hold time               [19:22]:(0x0)0us ,(0x1)250us,(0x2) 500us,(0x3) 1000us
           | (0x0 << 16)                        // ALC noise gate threshold    [16:18]:(0x7) -36dB ~ (0x0)  -78dB  [6dB decay]
           | (0x7 << 11)                        // minimum PGA gain            [11:13]:(0x7) - 6dB ~ (0x0)  -27dB  [3dB decay]
           | (0x0 << 8)                         // maximun PGA gain            [ 8:10]:(0x7)  36dB ~ (0x0)   -6dB  [6dB decay]
           | (0xF << 4)                         // ALC target level            [ 4: 7]:(0xF)-4dBFS ~ (0x0)-34dBFS  [2db decay]
           | (0x0 << 3)                         // DAC DWA mode control               : Enable(0x1) DIsable(0x0)
           | (0x1 << 2)                         // DAC DEM mode control               : Enable(0x1) DIsable(0x0)
           | (0x1 << 0);                        // DAC SDMGAIN                 [ 0: 1]: (0x0)-1db (0x1)-2db (0x2)-3db (0x3)-6db
    faraday_write_reg(0xD8, data);

    /*ALC control 2 sub main set*/
    data = 0x0 | (0x1 << 8)                     // ALC attack time setting     [8:11]:(0x0)62.25 us ,double when add 1 bit
           | (0x1 << 0);                        // ALC decay time              [ 0:3]:(0x0)104   us ,double when add 1 bit
    faraday_write_reg(0xDC, data);

    /*volume gain*/
    data =
        0x0 |
        (0x39 << 16)                            // SPK   analog gain control          [16:21]:(0x3F) +6dB ~ (0x11) -40dB [1dB decay] (0x39) +0dB
        | (0x39 << 8)                           // HP(R) analog gain control          [ 8:13]:(0x3F) +6dB ~ (0x11) -40dB [1dB decay] (0x39) +0dB
        | (0x39 << 0);                          // HP(L) analog gain control          [ 0: 5]:(0x3F) +6dB ~ (0x11) -40dB [1dB decay] (0x39) +0dB

    faraday_write_reg(0xE0, data);

    /*ADC control gain*/
    data = 0x0 | (0x7F << 24)                   // ADC(R) digital gain control [24:30]:(0x7F) +30dB ~ (0x11) -50dB [1dB decay] (0x61) +0dB
           | (0x7F << 16)                       // ADC(L) digital gain control [16:22]:(0x7F) +30dB ~ (0x11) -50dB [1dB decay] (0x61) +0dB
           | (0x1B << 8)                        // ADC(R) analog  gain control [ 8:13]:(0x3F) +36dB ~ (0x00) -27dB [1dB decay] (0x1B) +0dB
           | (0x1B << 0);                       // ADC(L) analog  gain control [ 0: 5]:(0x3F) +36dB ~ (0x00) -27dB [1dB decay] (0x1B) +0dB
    faraday_write_reg(0xE4, data);

    /*DAC control gain*/                        // 0x00003F3F :0x1B1B3F3F?
    data = 0x0 | ((((uint32_t)0x3FU) << 8U))    // DAC(R) digital gain control [ 8:13]:(0x3F) +0dB ~ (0x11) -40dB [1dB decay] (0x3F) +0dB
           | (0x3FU << 0U);                     // DAC(L) digital gain control [ 0: 5]:(0x3F) +0dB ~ (0x11) -40dB [1dB decay] (0x3F) +0dB
    faraday_write_reg(0xE8, data);

    /*AUDIO control*/
    data = 0x0U
           | (((uint32_t)0x0U) << 11U)          // chopper stabilize chontrol(L) :(0x0) off
           | (((uint32_t)0x0U) << 10U)          // chopper stabilize chontrol(R) :(0x0) off
           | (((uint32_t)0x1U) << 9U)           // LINE out power(L/R)           :(0x1) off
           | (((uint32_t)0x1U) << 8U)           // LINE mute (L/R)               :(0x1) mute
           | (CFG_SINGLE_END_MODE << 7U)        // mic mode(R)   :(0x1)single-end ,(0x0)differential
           | (CFG_SINGLE_END_MODE << 6U)        // mic mode(L)   :(0x1)single-end ,(0x0)differential
           | (0x4U << 3U)                       // IRSEL_HP?                [3:5]:(0x4)default
           | (0x0U << 2U)                       // bias mode control             :(0x1) disable
           | (0x0U << 1U)                       // HP(R) power                   : 0x1 off
           | (0x0U << 0U);                      // HP(L) power                   : 0x1 off
    faraday_write_reg(0xECU, data);

    /*SOFT Mute control control 1*/
    data = 0x0U
        | (((uint32_t)0x3F) << 24U)             // DAC soft-mute MAX digital control(R) [16:21]:(0x3F) 0dB ~ (0x23) -40dB (1dB decay)
        | (((uint32_t)0x3F) << 16U)             // DAC soft-mute MAX digital control(L) [16:21]:(0x3F) 0dB ~ (0x23) -40dB (1dB decay)
        | (((uint32_t)0x0U) << 11U)             // DAC soft-mute gain step              [11:12]:(0x0) 1dB ~(0x3) 8dB
        | (((uint32_t)0x0U) << 8U)              // DAC soft mute time setp              [ 8:10]:(0x0)1ms ~ 0x7(128ms) (double change value*2)
        | (((uint32_t)0x0U) << 2U)              // DAC soft-mute zero cross                    :(0x1)zero crocess on
        | (((uint32_t)0x0U) << 1U)              // DAC fading (in/out)                         :(0x1)setp down to mute (0x0) step up to current digital
        | (((uint32_t)0x0U) << 0U);             // DAC soft-mute                               :(0x1)mute
    faraday_write_reg(0x0F0U, data);

    /*SOFT Mute control control 2*/
    data = 0x0U
        | ((uint32_t)0x0U << 8U)                // DAC soft-mute min digital control(R) [8:13]:(0x3F) 0dB ~ (0x23) -40dB (1dB decay)
        | ((uint32_t)0x0U << 0U);               // DAC soft-mute min digital control(L) [0: 5]:(0x3F) 0dB ~ (0x23) -40dB (1dB decay)
    faraday_write_reg(0xF4U, data);

    // #ifdef CFG_CHIP_PKG_IT976
    // faraday_writeRegMask(0xEC,0x3 << 6,0x3 << 6); //set input as single-end
    // #endif

    if (!CFG_SINGLE_GROUND_MODE)
    {
        // mix RL & invert L(0x1)
        // R(0x2)[17:16]
        faraday_writeRegMask(0xD4U,
                             (((uint32_t)0x1U) << 18U) | (((uint32_t)0x1U) << 16U),
                             (((uint32_t)0x1U) << 18U) | (((uint32_t)0x3U) << 16U));
    }
}

/* ************************************************************************** */

/* === common DAC/ADC ops === */
void
itp_codec_depop (
    void)
{
    ithWriteRegA(0xD800003CU, 0x02002804U);     //
    ithWriteRegA(0xD01000D0U, 0x0000000FU);     // DAC depop (for DAC faraday)
}

void
itp_codec_wake_up (
    void)
{
    amp_ctrol(false);
    if (!faraday_DA_running_)
    {
        ithWriteRegA(0xD800003CU, 0x02A88800U);     // start clock
        ithWriteRegA(0xD8000040U, 0x0002a801U);     // start clock
    }
}

void
itp_codec_standby (
    void)
{
    ithWriteRegA(0xD800003cU, 0x02008800U);                                                                             // enable W15CLK
    faraday_writeRegMask(0xE0U, (((uint32_t)0x3FU) << 8U) | (0x3FU << 0U), (((uint32_t)0x3FU) << 8U) | (0x3FU << 0U));  // DAC analog(R|L) HP out
    faraday_writeRegMask(0xE4U, (((uint32_t)0x00U) << 8U) | (0x00U << 0U), (((uint32_t)0x3FU) << 8U) | (0x3FU << 0U));  // ADC analog gain(R|L)
    faraday_write_reg(
        0xD0U,
        0x00000FFFU);                                                                                                   // HP out off[10:11] ADC front-end power control off [8:9] DAC power off [6:7] ADC power off [4:5]
    faraday_write_reg(0xD4U, 0x61F81010U);                                                                              // reference current control [0:2]
    // faraday_write_reg(0xEC,0x000003C3); //HP power off[0:1]
    faraday_DA_running_ = false;
    faraday_AD_running_ = false;
    ithWriteRegA(0xD800003CU, 0x00008800U);                                                                             // reset clock
    ithWriteRegA(0xD8000040U, 0x0000a801U);                                                                             // reset clock
    amp_ctrol(true);
}

/* DAC */
void
itp_codec_playback_init (
    uint32_t output)
{
    (void)printf("faraday# %s _faraday_out = %ud \n", __func__, output);

    internal_DAC_reset_();

    faraday_out_ = output;
    output_path_mute_(false);
    faraday_writeRegMask(0xE0U, (current_volstep << 8U) | (current_volstep << 0U),
                         (((uint32_t)0x3FU) << 8U) | (0x3FU << 0U)); // DAC analog(R|L) HP out
    current_volstepRL[0] = current_volstep;
    current_volstepRL[1] = current_volstep;
    current_volpercRL[0] = current_volperc;
    current_volpercRL[1] = current_volperc;
    faraday_DA_running_  = true;
    amp_ctrol(true);
    // dump_all_reg_();
}

void
itp_codec_playback_deinit (
    void)
{
    if (faraday_DA_running_)
    {
        faraday_writeRegMask(0xE0U, (MIN_OUT_VOLUME << 8U) | (MIN_OUT_VOLUME << 0U),
                             (((uint32_t)0x3FU) << 8U) | (0x3FU << 0U)); // DAC analog(R|L) HP out
        faraday_DA_running_ = false;
    }
    amp_ctrol(false);
}

void
itp_codec_playback_set_direct_vol (
    uint32_t volstep)
{ // set volstep to register;
    if (volstep > MAX_OUT_VOLUME)
    {
        (void)printf("ERROR# invalid target volume step: 0x%08x\n", volstep);
        return;
    }
    if (volstep == current_volstep)
    {
        return;
    }
    current_volstep = volstep;

    if (faraday_DA_running_)
    {
        /* IPGA_table.inc*/
        faraday_writeRegMask(0xE0U, (volstep << 8U) | (volstep << 0U),
                             (((uint32_t)0x3FU) << 8U) | (((uint32_t)0x3FU) << 0U)); // DAC analog(R|L) HP out
        //    faraday_writeRegMask(0xE0, volstep<<16, 0x3F<<16);//DAC analog spk out
    }
}

void
itp_codec_playback_set_direct_volperc (
    uint32_t target_volperc)
{ // precent to step :ex  82% -> 0x90
    uint32_t volstep;

    if (target_volperc >= 100U)
    {
        target_volperc = 99U;
    }

    volstep              = it970DA_perc_to_reg_table[target_volperc];
    itp_codec_playback_set_direct_vol(volstep);
    current_volperc      = target_volperc;
    current_volpercRL[0] = target_volperc;
    current_volpercRL[1] = target_volperc;
    current_volstepRL[0] = current_volstep;
    current_volstepRL[1] = current_volstep;
    DEBUG_PRINT("FARADAY# %s target_volperc = %d(0x%02x) \n", __func__, target_volperc, volstep);
}

void
itp_codec_playback_get_currvol (
    uint32_t * currvolperc)
{
    *currvolperc = current_volperc;
}

void
itp_codec_playback_set_direct_RLvol (
    uint32_t volstep,
    uint8_t  RL)
{
    current_volstepRL[RL] = volstep;
    if (faraday_DA_running_)
    {
        switch (RL)
        {
            case 0:
                faraday_writeRegMask(0xE0U, volstep << 8U, ((uint32_t)0x3FU) << 8U);
                break;

            case 1:
                faraday_writeRegMask(0xE0U, volstep << 0U, ((uint32_t)0x3FU) << 0U);
                break;

            default:
                faraday_writeRegMask(0xE0U,
                                     (volstep << 8U) | (volstep << 0U),
                                     (((uint32_t)0x3FU) << 8U) | (((uint32_t)0x3FU) << 0U));
                break;
        }
    }
}

void
itp_codec_playback_set_direct_RLvolperc (
    uint32_t target_volperc,
    uint8_t  RL)
{ // precent to step :ex  82% -> 0x90
    uint32_t volstep;
    if ((RL != 0U) && (RL != 1U))
    {
        (void)printf("error vol RL(%d) set\n", RL);
        return;
    }
    if (target_volperc >= 100U)
    {
        target_volperc = 99U;
    }

    volstep               = it970DA_perc_to_reg_table[target_volperc];
    itp_codec_playback_set_direct_RLvol(volstep, RL);
    current_volpercRL[RL] = target_volperc;
    if (target_volperc != 0U)
    {
        current_volperc = target_volperc;
        current_volstep = current_volstepRL[RL];
    }
    if (current_volpercRL[0] == 0U)
    {
        faraday_out_ = 1;       // mute R:
    }
    if (current_volpercRL[1] == 0U)
    {
        faraday_out_ = 0;       // mute L
    }
    if ((current_volpercRL[0] == 0U) && (current_volpercRL[1] == 0U))
    {
        current_volperc = 0U;
        current_volstep = MIN_OUT_VOLUME;
        faraday_out_    = 2;    // mute RL
    }
    itp_codec_playback_unmute();

    (void)printf("FARADAY# target_volperc[%d] = %ud(0x%02x) out:%ud\n", RL, target_volperc, volstep, faraday_out_);
}

void
itp_codec_playback_get_RLcurrvol (
    uint32_t * currvolperc,
    uint8_t  RL)
{
    *currvolperc = current_volpercRL[RL];
}

void
itp_codec_playback_mute (
    void)
{
    DEBUG_PRINT("FARADAY# %s \n", __func__);
    if (faraday_DA_running_)
    {
        output_path_mute_(true);
    }
}

void
itp_codec_playback_unmute (
    void)
{
    DEBUG_PRINT("FARADAY# %s \n", __func__);

    if (current_volstep == MIN_OUT_VOLUME)
    { // vol = min (mute volume)
        output_path_mute_(true);
        return;
    }

    if (faraday_DA_running_)
    {
        output_path_mute_(false);
    }
}

/* ADC */
void
itp_codec_rec_init (
    uint32_t input_source)
{
    (void)printf("faraday# %s _faraday_in = %ud \n", __func__, input_source);

    internal_DAC_reset_();

    faraday_in_ = input_source;
    input_path_mute_(false);
    faraday_writeRegMask(0xE4U, (current_micstep << 8U) | (current_micstep << 0U),
                         (((uint32_t)0x3FU) << 8U) | (((uint32_t)0x3FU) << 0U)); // ADC analog gain(R|L)
    current_micstepRL[0] = current_micstep;
    current_micstepRL[1] = current_micstep;
    current_micpercRL[0] = current_micperc;
    current_micpercRL[1] = current_micperc;
    faraday_AD_running_  = true;

    // dump_all_reg_();
}

void
itp_codec_rec_deinit (
    void)
{
    DEBUG_PRINT("FARADAY# %s \n", __func__);
    if (faraday_AD_running_)
    {
        faraday_writeRegMask(0xE4U, (MIN_IN_VOLUME << 8U) | (MIN_IN_VOLUME << 0U),
                             (((uint32_t)0x3FU) << 8U) | (0x3FU << 0U)); // ADC analog gain(R|L)
        faraday_AD_running_ = 0;
    }
}

void
itp_codec_rec_set_direct_vol (
    uint32_t micstep)
{
    // set to register;
    if (micstep > MAX_IN_VOLUME)
    {
        (void)printf("ERROR# invalid target volume step: 0x%08x\n", micstep);
        return;
    }

    if (micstep == current_micstep)
    {
        return;
    }

    current_micstep = micstep;

    if (faraday_AD_running_)
    {
        /*IPGA_table.inc*/
        faraday_writeRegMask(0xE4U, (micstep << 8U) | (micstep << 0U),
                             (((uint32_t)0x3FU) << 8U) | (0x3FU << 0U)); // ADC analog gain(R|L)
    }
}

void
itp_codec_rec_set_direct_volperc (
    uint32_t target_micperc)
{ // precent to step :ex  82% -> 0x90
    uint32_t micstep;
    if (target_micperc >= 100U)
    {
        target_micperc = 99U;
    }

    micstep              = it970AD_perc_to_reg_table[target_micperc];
    itp_codec_rec_set_direct_vol(micstep);
    current_micperc      = target_micperc;
    current_micpercRL[0] = current_micpercRL[1] = current_micperc;
    current_micstepRL[0] = current_micstepRL[1] = current_micstep;
    DEBUG_PRINT("FARADAY# %s target_micperc = %d(0x%02x) \n", __func__, target_micperc, micstep);
}

void
itp_codec_rec_get_currvol (
    uint32_t * currvol)
{
    *currvol = current_micperc;
}

void
itp_codec_rec_set_direct_RLvol (
    uint32_t micstep,
    uint8_t  RL)
{ // set to register;
    if (micstep == current_micstepRL[RL])
    {
        return;
    }

    current_micstepRL[RL] = micstep;

    if (faraday_AD_running_)
    {
        switch (RL)
        {
            case 0:
                faraday_writeRegMask(0xE4U, micstep << 8U, (((uint32_t)0x3FU) << 8U));
                break;

            case 1:
                faraday_writeRegMask(0xE4U, micstep << 0U, ((uint32_t)0x3FU) << 0U);
                break;

            default:
                faraday_writeRegMask(0xE4U, (micstep << 8U) | (micstep << 0U),
                                     (((uint32_t)0x3FU) << 8U) | (0x3FU << 0U));
                break;
        }
    }
}

void
itp_codec_rec_set_direct_RLvolperc (
    uint32_t target_micperc,
    uint8_t  RL)
{ // precent to step :ex  82% -> 0x90
    uint32_t micstep;
    if ((RL != 0U) && (RL != 1U))
    {
        (void)printf("error rec RL(%d) set\n", RL);
        return;
    }
    if (target_micperc >= 100U)
    {
        target_micperc = 99U;
    }

    micstep               = it970AD_perc_to_reg_table[target_micperc];
    itp_codec_rec_set_direct_RLvol(micstep, RL);
    current_micpercRL[RL] = target_micperc;
    if (target_micperc != 0)
    {
        current_micperc = target_micperc;
        current_micstep = current_micstepRL[RL];
    }
    if (current_micpercRL[0] == 0)
    {
        faraday_in_ = 1;        // mute R:
    }
    if (current_micpercRL[1] == 0)
    {
        faraday_in_ = 0;        // mute L
    }
    if ((current_micpercRL[0] == 0) && (current_micpercRL[1] == 0))
    {
        current_micperc = 0;
        current_micstep = MIN_IN_VOLUME;
        faraday_in_     = 2;    // mute RL
    }

    itp_codec_rec_unmute();

    (void)printf("FARADAY# target_micperc[%d] = %ud(0x%02x) in:%ud\n", RL, target_micperc, micstep, faraday_in_);
}

void
itp_codec_rec_get_RLcurrvol (
    uint32_t * currvol,
    uint8_t  RL)
{
    *currvol = current_micpercRL[RL];
}

void
itp_codec_rec_get_vol_range (
    uint32_t * max,
    uint32_t * regular_0db,
    uint32_t * min)
{
    *max         = MAX_IN_VOLUME;
    *regular_0db = current_micstep;    // current step;
    *min         = MIN_IN_VOLUME;
}

void
itp_codec_rec_mute (
    void)
{
    DEBUG_PRINT("FARADAY# %s \n", __func__);
    if (faraday_AD_running_)
    {
        input_path_mute_(true);
    }
}

void
itp_codec_rec_unmute (
    void)
{
    DEBUG_PRINT("FARADAY# %s \n", __func__);
    if (current_micstep == MIN_IN_VOLUME)
    { // vol = min (mute volume)
        input_path_mute_(true);
        return;
    }

    if (faraday_AD_running_)
    {
        input_path_mute_(false);
    }
}

void
itp_codec_power_on (
    void)
{
    DEBUG_PRINT("FARADAY# %s \n", __func__);
    faraday_writeRegMask(0xD0U, 0x0U << 4U, 0xFU << 4U);    // DA(R/L) AD(R/L) power on
    faraday_writeRegMask(0xECU, 0x0U << 0U, 0x3U << 0U);    // HP(R/L) power on
}

void
itp_codec_power_off (
    void)
{
    DEBUG_PRINT("FARADAY# %s \n", __func__);
    faraday_writeRegMask(0xD0U, 0xFU << 4U, 0xFU << 4U);    // DA(R/L) AD(R/L) power down
    faraday_writeRegMask(0xECU, 0x3U << 0U, 0x3U << 0U);    // HP(R/L) power down
}

void
itp_codec_set_i2s_sample_rate (
    uint32_t samplerate)
{
    switch (samplerate)
    {
        case 48000:
        case 44100:
            faraday_writeRegMask(0xD0U, 0xFU << 0U, 0xFU << 0U);
            break;  // 256x

        case 32000:
            faraday_writeRegMask(0xD0U, 0x7U << 0U, 0xFU << 0U);
            break;  // 384x

        case 24000:
            faraday_writeRegMask(0xD0U, 0x5U << 0U, 0xFU << 0U);
            break;  // 500x

        case 22050:
            faraday_writeRegMask(0xD0U, 0x8U << 0U, 0xFU << 0U);
            break;  // 544x

        case 16000:
            faraday_writeRegMask(0xD0U, 0x1U << 0U, 0xFU << 0U);
            break;  // 750x

        case 12000:
            faraday_writeRegMask(0xD0U, 0xBU << 0U, 0xFU << 0U);
            break;  // 1000x

        case 11025:
            faraday_writeRegMask(0xD0U, 0x9U << 0U, 0xFU << 0U);
            break;  // 1088x

        case 8000:
            faraday_writeRegMask(0xD0U, 0x4U << 0U, 0xFU << 0U);
            break;  // 1500x

        default:
            faraday_writeRegMask(0xD0U, 0x6U << 0U, 0xFU << 0U);
            break;  // 250x
    }
    faraday_sampleRate_ = samplerate;
}

void
itp_codec_get_i2s_sample_rate (
    uint32_t * samplerate)
{
    *samplerate = faraday_sampleRate_;
}

bool
itp_codec_get_DA_running (
    void)
{
    return faraday_DA_running_;
}

bool
itp_codec_get_AD_running (
    void)
{
    return faraday_AD_running_;
}
