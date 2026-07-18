/*
 * Copyright (c) 2016 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * HAL LCD functions.
 *
 * @author Irene Wang
 * @version 1.0
 */
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>
#include <sys/ioctl.h>
#include <ith/ith_cmdq.h>
#include "ith_cfg.h"
#include "ith_lcd.h"
#include "ite/itp.h"
#include "ith/ith_lcd.h"

//#define RGB_GAIN_COMPENSATION

#define CMD_DELAY           0xFFFFFFFFU
#define CLAMP(value, low, high) ( ((value) > (high)) ? (high) : ((value) < (low)) ? (low) : (value) )

#define MIPI_RELOAD_SCRIPT

#ifdef MIPI_RELOAD_SCRIPT
static uint32_t lcdA;
static uint32_t lcdB;
static uint32_t lcdC;
#endif

static const uint32_t * lcdScript;
static unsigned int     lcdScriptCount, lcdScriptIndex;
static uint32_t         mipiHSTable[16];
static uint32_t         pinShareTable[8];
static uint32_t         mipiDPHYInfo[8];

static int              gContrast;
static int              gBrightness;
static ITHLcdPanelType  gPanelType = ITH_LCD_RGB;

///////////////////////////////////////////////////////////
/// Color Conversion Algorithm
///////////////////////////////////////////////////////////

/** (Local Function) This is matrix multiplication of size (3x3)
*
*  @param M_in1 (3x3 matrix, input):  This is input matrix for left-hand side in matrix multiplication.
*  @param M_in2 (3x3 matrix, input):  This is input matrix for right-hand side in matrix multiplication.
*  @param M_out (3x3 matrix, output): This stores result after matrix multiplication.
*/
static void
matrix_multiply_33_(
    float M_in1[3][3],
    float M_in2[3][3],
    float M_out[3][3])
{
    int16_t i, j, k;

    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            M_out[i][j] = 0.0f;
            for (k = 0; k < 3; k++)
            {
                M_out[i][j] += M_in1[i][k] * M_in2[k][j];
            }
        }
    }
}

/** This is to achieve Hue rotation by using single 3x3 matrix multiplication.
*
* NOTE: There is no color model conversion, only hue rotation.
*
* @param angle [unit: degree]: Hue rotation angle.
*                              If positive, red is turning into green. Vise-versa.
*
* @return 3x4 Matrix coefficients to put into hardware, where the last column is delta/constant.
*          In `EV_MATRIX`, `M_color_correction` is 3x3 matrix at the left of 3x4 matrix;
*          `V_color_correction` is the last column in 3x4 matrix.
*/
static EV_MATRIX color_conversion_(signed short angle)
{
    float a, b;
    float M_hsv_angle[3][3] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
    float M_ycrb2rgb[3][3];
    float M_rgb2ycrb[3][3];

    float M_temp1[3][3];
    EV_MATRIX CC_MatrixCurrect;

    /// Get hue rotation matrix (in YCrCb domain)
    a = (float)cos((float)angle / 180.0 * 3.1516159);
    b = (float)-sin((float)angle / 180.0 * 3.1516159);

    M_hsv_angle[1][1] = a;
    M_hsv_angle[1][2] = -b;
    M_hsv_angle[2][1] = b;
    M_hsv_angle[2][2] = a;

    /// Color conversion from/to YCrCb to/from RGB
    {
        M_ycrb2rgb[0][0] = 1.0f;
        M_ycrb2rgb[0][1] = 1.402f;
        M_ycrb2rgb[0][2] = 0.0f;
        M_ycrb2rgb[1][0] = 1.0f;
        M_ycrb2rgb[1][1] = -0.714136f;
        M_ycrb2rgb[1][2] = -0.344136f;
        M_ycrb2rgb[2][0] = 1.0f;
        M_ycrb2rgb[2][1] = 0.0f;
        M_ycrb2rgb[2][2] = 1.772f;

        M_rgb2ycrb[0][0] = 0.299f;
        M_rgb2ycrb[0][1] = 0.587f;
        M_rgb2ycrb[0][2] = 0.114f;
        M_rgb2ycrb[1][0] = 0.5f;
        M_rgb2ycrb[1][1] = -0.418688f;
        M_rgb2ycrb[1][2] = -0.081312f;
        M_rgb2ycrb[2][0] = -0.168736f;
        M_rgb2ycrb[2][1] = -0.331264f;
        M_rgb2ycrb[2][2] = 0.5f;
    }

    /// Get final R/G/B color conversion matrix
    matrix_multiply_33_(M_ycrb2rgb, M_hsv_angle, M_temp1);
    matrix_multiply_33_(M_temp1, M_rgb2ycrb, CC_MatrixCurrect.M_color_correction);

    /// Ensure that delta is 0
    for (int i = 0; i < 3; i++)
    {
        CC_MatrixCurrect.V_color_correction[i][0] = 0.0f;
    }

    return CC_MatrixCurrect;
}

#define POW2_INT(i)       ((int) pow(2,i))
/** This is to convert floating point number (double) into fix point representation using 2's complement notation.
*      Output format is `SIGN_BIT`:`INT_D`:`FRAC_D`
*
*  @param value:    This value is to be converted to fix-point representation.
*  @param SIGN_BIT [valid value - 0/1]: How many sign bit does the value contains. Now only 1 sign bit is supported.
*  @param INT_D:    Number of integer bits.
*  @param FRAC_D:   Number of fractional bits.
*/
static inline uint16_t double2fix_2s_comp_(
    double value,
    int SIGN_BIT,
    int INT_D,
    int FRAC_D)
{
    int value_uint = (int)((fabs(value) * pow(2.0f, FRAC_D)) + 0.5f);            ///Turn to fix point (with rounding)
    uint16_t value_uint16 = CLAMP(value_uint, 0, POW2_INT(FRAC_D + INT_D) - 1);   ///Clamp to allowed value range
    uint16_t value_masked;

    /// Start Sign bit process (sign-magnitude)
    if (SIGN_BIT > 0)
    {
        if (value < 0.0f)
        {
            value_uint16 = (~value_uint16) + 1U;
        }
    }
    else if (value < 0.0f)
    {
        value_uint16 = 0;
    }
    else
    {
        /* do nothing */
    }
    /// End Sign bit process (sign-magnitude)

    value_masked = value_uint16 & ((uint16_t)(POW2_INT(FRAC_D + INT_D + SIGN_BIT) - 1));

    return value_masked;
}

/** This is to check if parameter `value` is within valid range.
*
*  @param value: Input value to check if it is valid or not.
*  @param row: Row inside 3x4 matrix.
*  @param column: Row inside 3x4 matrix.
*  @param SIGN_BIT [valid value - 0/1]: How many sign bit does the value contains. Now only 1 sign bit is supported.
*  @param INT_D:    Number of integer bits.
*  @param FRAC_D:   Number of fractional bits.
*/
static inline void check_range_(
    double value, int row, int column,
    int SIGN_BIT,
    int INT_D,
    int FRAC_D)
{
    if (fabs(value) >= ((float)POW2_INT(INT_D)))
    {
        (void)printf("[ERROR] Coefficient at [%d,%d] is larger than allowed range, with value %f\n", row, column, value);
#if 0
        exit(130);
#endif
    }
}
#undef POW2_INT

static uint16_t double_to_s1_8_(double value_in)
{
    return double2fix_2s_comp_(value_in, 1, 1, 8);
}

static void check_range_s1_8_(double value, int row, int column)
{
    check_range_(value, row, column, 1, 1, 8);
}

static uint16_t double_to_s8_0_(double value_in)
{
    return double2fix_2s_comp_(value_in, 1, 8, 0);
}

static void check_range_s8_0_(double value, int row, int column)
{
    check_range_(value, row, column, 1, 8, 0);
}

/** This function is to write 3x4 color conversion coefficients into LCD's color conversion register.
*
*  ------------------------
*  Input/Output Arguments
*  ------------------------
*
*  @param out_cfg_path [const char*]: File to write register-write commands to.
*  @param color_conv_mat [`EV_MATRIX`]: This is the 3x4 color conversion matrix.
*/
static void soc_write__color_conversion_(EV_MATRIX color_conv_mat)
{
#if 0
    FILE* fp = fopen(out_cfg_path, "w");
#endif

    // Write coefficients into registers
    for (int i = 0; i < 3; i++)
    {
        uint32_t reg_addr_base = ITH_LCD_BASE + ITH_LCD_RGB2YUV11_REG + ((uint32_t)i * 8U);
        uint32_t reg_value;

        check_range_s1_8_(color_conv_mat.M_color_correction[i][0], i, 0);
        check_range_s1_8_(color_conv_mat.M_color_correction[i][1], i, 1);
        check_range_s1_8_(color_conv_mat.M_color_correction[i][2], i, 2);
        check_range_s8_0_(color_conv_mat.V_color_correction[i][0], i, 3);

        reg_value = (uint32_t)double_to_s1_8_(color_conv_mat.M_color_correction[i][0]) + ((uint32_t)double_to_s1_8_(color_conv_mat.M_color_correction[i][1]) << 16);
        ithWriteRegA(reg_addr_base, reg_value);

#if 0
        fprintf(fp, "WRITE(0x%08X, 0x%08X);\n", reg_addr_base + 0, reg_value);
#endif
        reg_value = (uint32_t)double_to_s1_8_(color_conv_mat.M_color_correction[i][2]) + ((uint32_t)double_to_s8_0_(color_conv_mat.V_color_correction[i][0]) << 16);
        ithWriteRegA(reg_addr_base + 4U, reg_value);

#if 0
        fprintf(fp, "WRITE(0x%08X, 0x%08X);\n", reg_addr_base + 4, reg_value);
#endif
    }

#if 0
    // Enable color conversion
    fprintf(fp, "WRITE(0xD0000050, 0x00000001);\n");
    fprintf(fp, "WRITE(0xD000001C, 0x80000003);\n");    /// This is due to the script is after LCD activating sequence.
#endif
    ithSetRegBitA(ITH_LCD_BASE + ITH_LCD_RGB2YUV_REG, ITH_LCD_RGB2YUV_EN_BIT);
    ithSetRegBitA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG, ITH_LCD_LAYER1UPDATE_BIT);
    while (0UL != (ithReadRegA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG) & (0x1UL << ITH_LCD_LAYER1UPDATE_BIT)))
    {
        ithDelay(1000);
    }
#if 0
    fclose(fp);
#endif
}

static int color_conversion_main_(int hue)
{
    EV_MATRIX color_conv_mat = color_conversion_((signed short)hue);

    soc_write__color_conversion_(color_conv_mat);

    return 0;
}

/** This function is to generate LUT for gamma function, to achieve
 *  - Contrast (by setting variable `contrast` and `midpoint`)
 *  - Brightness (by setting variable `brightness`)
 *
 * [Math equation: out_value = in_value + (in_value - `midpoint`) * `contrast` /
 * 128.0f + `brigtness`]
 *
 * ------------------------
 * Input/Output Arguments
 * ------------------------
 *
 * @param contrast [-64 ~ 63]   : This is to set how much contrast is enhance or
 * reduced.
 * @param midpoint [0 ~ 255]    : This is to set mid-point for contrast.
 * @param brightness [-64 ~ 63] : This is to set brightness (curve offset, but
 * not exactly for now).
 */
static GAMMA_FUNC_CURVE gamma_curve__contrast_brightness_(int contrast, int midpoint, int brightness)
{
    float contrast_f = (float)contrast / 128.0f;
    float brightness_f = (float)brightness;

    GAMMA_FUNC_CURVE curve = {
        0.0f};           ///This is Y axis in LUT

    for (float idx = 0.0f; idx < (float)GAMMA_CURVE_LUT_SIZE; idx++)
    {
        float x = 256.0f / (float)(GAMMA_CURVE_LUT_SIZE - 1) * idx;
        float y = x + ((x - (float)midpoint) * contrast_f) + brightness_f;

        curve.x[(int)idx] = x;

        /// Since y is currently always > 0, directly add 0.5 to perform rounding.
        curve.y[(int)idx] = CLAMP(y, 0.0f, (256.0f - 1.0f));
    }

    return curve;
}

/** This function is to write gamma curve into LCD's gamma function register,
 * where the curve is used to tune gray-level intensity, such that only one
 * curve is given.
 *
 *  ------------------------
 *  Input/Output Arguments
 *  ------------------------
 *
 * @param out_cfg_path [const char*]: File to write register-write commands to.
 * @param curve_gray [struct gamma_func_curve]: Gamma Curve to write (one curve
 * for all R/G/B tables).
 * @param enable [bool]: If set False, Gamma Function will be disabled.
 */
static void soc_write__LCD_gamma_intensity_(GAMMA_FUNC_CURVE curve_gray, bool enable)
{
#if 0
    FILE *fp = fopen(out_cfg_path, "w");
#endif
    for (uint32_t channel = 0U; channel < 3U; channel++) ///Channel: R/G/B color
    {
        uint32_t reg_addr;
        uint32_t reg_value;

        /// R/G/B LUT write locations are starting from `0xD0000040` to `0xD0000048`
        reg_addr = ITH_LCD_BASE + ITH_LCD_GAMMA_R_PTR_REG + (4U * channel); //0xD0000040

        for (int idx = 0; idx < GAMMA_CURVE_LUT_SIZE; idx++)
        {
            /// `curve_x_int` is LUT's address in hardware
            uint32_t curve_x_int = (uint32_t)idx;

            /// `curve_y_int` is LUT's value in given address, with format U8.2
            uint32_t curve_y_int = (curve_gray.y[idx] * pow(2.0f, 2.0f)) + 0.5f;

            if (!enable)
            {
                reg_value = 0; /// Disable Gamma Function
            }
            else
            {
                reg_value =
                    (uint32_t)1
                    << ITH_LCD_GAMMA_FUN_EN_BIT; /// Enabel Gamma Function
            }

            reg_value += ((uint32_t)((curve_y_int << 16) + curve_x_int));
            ithWriteRegA(reg_addr, reg_value);
#if 0
            fprintf(fp, "WRITE(0x%X, 0x%X);\n", reg_addr, reg_value);
#endif
        }
        ithWriteRegA(reg_addr, reg_value);
#if 0
        fprintf(fp, "WRITE(0x%X, 0x%X);\n", reg_addr, reg_value);
#endif
    }

    ithSetRegBitA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG, ITH_LCD_LAYER1UPDATE_BIT);
    while (0U != (ithReadRegA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG) & (0x1UL << ITH_LCD_LAYER1UPDATE_BIT)))
    {
        ithDelay(1000);
    }
#if 0
    fclose(fp);
#endif
}

static int gamma_function_api_(int contrast, int brightness)
{
    GAMMA_FUNC_CURVE curve = gamma_curve__contrast_brightness_(contrast, 128, brightness);

    /// Write Register-Write command file
    {
        bool enable_gamma = true;

        /// Disable Gamma Function if contrast==brightness==0
        if ((contrast == 0) && (brightness == 0))
        {
            enable_gamma = false;
        }
        else
        {
            enable_gamma = true;
        }

        soc_write__LCD_gamma_intensity_(curve, enable_gamma);
    }

    return 0;
}

/**
* Save MIPI HS information settings
*/
static void ithLcdSaveMipiHSInfo(void)
{
    // save MIPI HS registers
    mipiHSTable[0]  = ithReadRegA(0xd0c00004U);
    mipiHSTable[1]  = ithReadRegA(0xd0c00008U);
    mipiHSTable[2]  = ithReadRegA(0xd0c00010U);
    mipiHSTable[3]  = ithReadRegA(0xd0c00014U);
    mipiHSTable[4]  = ithReadRegA(0xd0c00018U);
    mipiHSTable[5]  = ithReadRegA(0xd0c0001cU);
    mipiHSTable[6]  = ithReadRegA(0xd0c00020U);
    mipiHSTable[7]  = ithReadRegA(0xd0c00028U);
    mipiHSTable[8]  = ithReadRegA(0xd0c00048U);
    mipiHSTable[9]  = ithReadRegA(0xd0c00054U);
    mipiHSTable[10] = ithReadRegA(0xd0c00058U);
    mipiHSTable[11] = ithReadRegA(0xd0c00080U);
    mipiHSTable[12] = ithReadRegA(0xd0c00084U);
    mipiHSTable[13] = ithReadRegA(0xd0c00088U);
    mipiHSTable[14] = ithReadRegA(0xd0c0008cU);
    mipiHSTable[15] = ithReadRegA(0xd0c00050U);
}

#ifndef MIPI_RELOAD_SCRIPT
/**
 * Reset MIPI HS information settings
 */
static void ithLcdResetMipiHSInfo(void)
{
    // resetting MIPI HS registers
    ithWriteRegA(0xd0c00004U, mipiHSTable[0]); 
    ithWriteRegA(0xd0c00008U, mipiHSTable[1]);
    ithWriteRegA(0xd0c00010U, mipiHSTable[2]);
    ithWriteRegA(0xd0c00014U, mipiHSTable[3]); 
    ithWriteRegA(0xd0c00018U, mipiHSTable[4]);
    ithWriteRegA(0xd0c0001cU, mipiHSTable[5]);
    ithWriteRegA(0xd0c00020U, mipiHSTable[6]);
    ithWriteRegA(0xd0c00028U, mipiHSTable[7]);
    ithWriteRegA(0xd0c00048U, mipiHSTable[8]);
    ithWriteRegA(0xd0c00054U, mipiHSTable[9]); 
    ithWriteRegA(0xd0c00058U, mipiHSTable[10]);
    ithWriteRegA(0xd0c00080U, mipiHSTable[11]); 
    ithWriteRegA(0xd0c00084U, mipiHSTable[12]); 
    ithWriteRegA(0xd0c00088U, mipiHSTable[13]); 
    ithWriteRegA(0xd0c0008cU, mipiHSTable[14]); 

    ithWriteRegA(0xd0c00050U, mipiHSTable[15]); 
    (void)usleep(150);
}
#endif
/**
 * Save RGB pin share information settings
 */
static void ithLcdSavePinShareInfo(void)
{
    // save LCD pin share registers
#if 0
    pinShareTable[0] = ithReadRegA(0xD10000E4U);
#endif
    pinShareTable[1] = ithReadRegA(0xD10000E8U);
    pinShareTable[2] = ithReadRegA(0xD10000ECU);
    pinShareTable[3] = ithReadRegA(0xD1000160U);
    pinShareTable[4] = ithReadRegA(0xD1000164U);
#if 0
    pinShareTable[5] = ithReadRegA(0xD1000168U);
    pinShareTable[6] = ithReadRegA(0xD100016CU);
#endif


#if 0
    ithWriteRegA(0xD10000E4U, 0U);
#endif
    ithWriteRegA(0xD10000E8U, 0U);
    ithWriteRegA(0xD10000ECU, 0U);
    ithWriteRegA(0xD1000160U, 0U);
    ithWriteRegA(0xD1000164U, 0U);
#if 0
    ithWriteRegA(0xD1000168U, 0U);
    ithWriteRegA(0xD100016CU, 0U);
#endif
}

/**
* Reset RGB pin share information settings
*/
static void ithLcdResetPinShareInfo(void)
{
    //resetting LCD pin share registers
    #if 0
    ithWriteRegA(0xD10000E4U, pinShareTable[0]);
    #endif
    ithWriteRegA(0xD10000E8U, pinShareTable[1]);
    ithWriteRegA(0xD10000ECU, pinShareTable[2]);
    ithWriteRegA(0xD1000160U, pinShareTable[3]);
    ithWriteRegA(0xD1000164U, pinShareTable[4]);
    #if 0
    ithWriteRegA(0xD1000168U, pinShareTable[5]);
    ithWriteRegA(0xD100016CU, pinShareTable[6]);
    #endif
}

/**
 * Save MIPI DPHY information settings
 */
static void ithLcdSaveMipiDPHYInfo (void)
{
    mipiDPHYInfo[0] = ithReadRegA(0xD0D00000U);
    mipiDPHYInfo[1] = ithReadRegA(0xD0D0001CU);

    mipiDPHYInfo[2] = ithReadRegA(0xD0D00004U);
    mipiDPHYInfo[3] = ithReadRegA(0xD0D00008U);
    mipiDPHYInfo[4] = ithReadRegA(0xD0D0000CU);
    mipiDPHYInfo[5] = ithReadRegA(0xD0D00010U);
    mipiDPHYInfo[6] = ithReadRegA(0xD0D00014U);
    mipiDPHYInfo[7] = ithReadRegA(0xD0D00018U);
}

#ifndef MIPI_RELOAD_SCRIPT
/**
 * Reset MIPI DPHY information settings
 */
static void ithLcdResetMipiDPHYInfo (void)
{
    /* ************************************************* */
    /*          MIPI DPHY reg base: 0xD0D00000           */
    /* ************************************************* */
    ithWriteRegA(0xD0D00000U,
                 (mipiDPHYInfo[0] & ~(0x1U << 24U)) | (0U & (0x1U << 24U))); // PLLNS=48, Pad Type=MIPI, [21:17]P/N SWAP
    ithWriteRegA(0xD0D00004U, mipiDPHYInfo[2]); // PLLMS=1, PLLF=1/8 (First, datarateclk change to slow)
    ithWriteRegA(0xD0D0001CU, mipiDPHYInfo[1]); // ESCCLK = BYTECLK/3
    ithWriteRegA(0xD0D00000U, mipiDPHYInfo[0]); // PLL ENABLE
    (void)usleep(200);

    ithWriteRegA(0xD0D00008U, mipiDPHYInfo[3]);
    ithWriteRegA(0xD0D0000CU, mipiDPHYInfo[4]);
    ithWriteRegA(0xD0D00010U, mipiDPHYInfo[5]);
    ithWriteRegA(0xD0D00014U, mipiDPHYInfo[6]);
    ithWriteRegA(0xD0D00018U, mipiDPHYInfo[7]);

    ithWriteRegA(0xD0D00004U, 0x055e8001U); // CLKEN,DATAEN
    ithWriteRegA(0xD0D00004U, 0x055f8001U); // RESET
    (void)usleep(1);
    ithWriteRegA(0xD0D00004U, 0x055e8041U); // normal
    (void)usleep(200);
}
#endif

static void ithLcdMIPIEnterSleppMode (void)
{
    /* ************************************************* */
    /*                LP  to Dispaly                     */
    /* ************************************************* */
    // Select MIPI disable
    ithWriteRegA(0xD0000230U, 0x00000016U); // [0]:Select MIPI disable
    (void)usleep(1);

    /* ************************************************* */
    /*    Clear  0xB0000234[15:8]                        */
    /* ************************************************* */
    ithWriteRegA(0xD00000ecU, 0x00020000U); // Force CSN=0
    (void)usleep(1);
    ithWriteRegA(0xD00000ecU, 0x00030000U); // Force CSN=1
    (void)usleep(1);

    /* ************************************************* */
    /*    Panel Setting (CPUIF FOR DBI), 0xB000_0000     */
    /* ************************************************* */
    ithWriteRegA(0xD0000004U, 0x0F7F0410U); // CPUIF
    (void)usleep(200000);

    // Select MIPI enable
    ithWriteRegA(0xD0000230U, 0x00000017U); // [0]:Select MIPI enable
    (void)usleep(1);

    /* ************************************************* */
    /*     Reset  MIPI Controller                        */
    /* ************************************************* */
    ithWriteRegA(0xD800004CU, 0xC002C801U); // KDSICLK
    (void)usleep(10);

    /* ************************************************* */
    /*     Enable MIPI controller                        */
    /* ************************************************* */
    ithWriteRegA(0xD800004CU, 0x0002C001U); // MIPI controller normal
    (void)usleep(200);

    (void)usleep(200000);

    /* ************************************************* */
    /*    MIPI reg base: 0xD0c00000 (LP)                 */
    /* ************************************************* */
    // ----------LP----------- //
    ithWriteRegA(0xD0C00004U, 0x000F020FU); // 0x6[7]=BLLP, +0x04[0]=EOTPGE
    ithWriteRegA(0xD0C00010U, 0x000F0000U);
    ithWriteRegA(0xD0C00014U, 0x0000001BU);
    (void)usleep(200000);

    // -------_MIPI LP END-------- //

    /* ************************************************* */
    /*    0x11 : exit sleep  0x29 :display on            */
    /*    0x28: display off  0x10: enter sleep           */
    /* ************************************************* */
    ithWriteRegA(0xD00000ECU, 0x00020000U); // Force CSN=0
    (void)usleep(1);
    ithWriteRegA(0xD0000230U, 0x00050017U); // ct=05
    (void)usleep(1);
    ithWriteRegA(0xD00000F0U, 0x0000A028U); // cmd 0x28
    (void)usleep(1);
    ithWriteRegA(0xD00000ECU, 0x00030000U); // Force CSN=1
    (void)usleep(1);

    ithWriteRegA(0xD00000ECU, 0x00020000U); // Force CSN=0
    (void)usleep(1);
    ithWriteRegA(0xD0000230U, 0x00050017U); // ct=05
    (void)usleep(1);
    ithWriteRegA(0xD00000F0U, 0x0000A010U); // cmd 0x10
    (void)usleep(1);
    ithWriteRegA(0xD00000ECU, 0x00030000U); // Force CSN=1
    (void)usleep(1);
}

void ithLcdReset(void)
{
    ithWriteRegMaskA(ITH_HOST_BASE + ITH_LCD_CLK1_REG, 0xFFFFFFFFU, (0x1UL << ITH_LCD_REG_RST_BIT) | (0x1UL << ITH_LCD_RST_BIT));
    ithWriteRegMaskA(ITH_HOST_BASE + ITH_LCD_CLK1_REG, 0x0U, (0x1UL << ITH_LCD_REG_RST_BIT) | (0x1UL << ITH_LCD_RST_BIT));
}

void ithLcdEnable(void)
{
    // enable clock
    if (gPanelType != ITH_LCD_RGB)
    {
        ithSetRegBitA(ITH_HOST_BASE + ITH_LCD_CLK1_REG, ITH_EN_DCLK_BIT);
    }
    ithSetRegBitA(ITH_HOST_BASE + ITH_LCD_CLK1_REG, ITH_EN_M3CLK_BIT);
    ithSetRegBitA(ITH_HOST_BASE + ITH_LCD_CLK1_REG, ITH_EN_W12CLK_BIT);

#if (CFG_GPIO_LCD_PWR_EN > 0)
    #ifdef CFG_GPIO_LCD_PWR_EN_ACTIVE_LOW
    ithGpioClear(CFG_GPIO_LCD_PWR_EN);
    #else
    ithGpioSet(CFG_GPIO_LCD_PWR_EN);
    #endif
#endif


    if (gPanelType == ITH_LCD_MIPI)
    {
#ifdef MIPI_RELOAD_SCRIPT
        ithLcdReset();
        (void)usleep(1000);

        ioctl(ITP_DEVICE_SCREEN, ITP_IOCTL_RESET, NULL);

        ithLcdEnableHwFlip();
        (void)usleep(2000); // avoid MIPI sync error

        ithLcdSetBaseAddrA(lcdA);
        ithLcdSetBaseAddrB(lcdB);
#if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
        ithLcdSetBaseAddrC(lcdC);
#endif
        ioctl(ITP_DEVICE_SCREEN, ITP_IOCTL_POST_RESET, NULL);

#else
        // MIPI PHY normal
        ithWriteRegA(0xD8000044U, 0x00280001U); //[31] DPHY PORn rst normal
        (void)usleep(200);                      // 200us

        // reset mipi DPHY
        ithLcdResetMipiDPHYInfo();

        // MIPI controller
        ithWriteRegA(0xD800004CU, 0x0002C001U); // MIPI controller normal
        (void)usleep(200);

        // LCD Setting (CPUIF FOR DBI), CPUIF mode
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_SET_MODE_REG, 0x0F7F0410U); // CPUIF

        // MIPI reg base: 0xd0c00000 (LP)
        ithWriteRegA(0xD0C00004U, 0x004F028FU); // $6[7]=BLLP, +$04[0]=EOTPGE
        ithWriteRegA(0xD0C00010U, 0x000F0000U);
        ithWriteRegA(0xD0C00014U, 0x0000001BU);
        (void)usleep(200000); // 200ms

        // LCD Setting, normal RGB mode
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_SET_MODE_REG,
                     0x0F7F0A60U); // SRC:RGB565, dst 24-bits
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_TEST_COLOR_SET_REG,
                     0x00000000U); // test color mode=0, None

        // CTG Setting
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_TCON_CTG_REG,
                     0x00010300U); // ctg_reset_on
        (void)usleep(1);           // 1 μs
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_TCON_CTG_REG,
                     0x00000307U); // enable ctg 0 1 2

        // reset HS
        ithLcdResetMipiHSInfo();

        // Enable LCD
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG, 0x00000001U);
        (void)usleep(10);
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG, 0x00000003U);
        (void)usleep(10000);
#endif
    }
    else if (gPanelType == ITH_LCD_LVDS)
    {
        // ithWriteRegMaskA(ITH_MIPI_DPHY_BASE, 0x0 << 23 | 0x1 << 22, 0x1 << 23
        // | 0x1 << 22); // // [23:22]Pad Type=TTL (0x01)
        ithClearRegBitA(ITH_MIPI_DPHY_BASE, 23);
        ithSetRegBitA(ITH_MIPI_DPHY_BASE, 22);
        ithClearRegBitA(ITH_MIPI_DPHY_BASE, 8); // [8] LDOPD
        (void)usleep(100);

        // LVDS Enable
        ithSetRegBitA(ITH_MIPI_DPHY_BASE, 24); // [24]PLL enable
        (void)usleep(200);
        ithSetRegBitA(ITH_LCD_BASE + ITH_LCD_LVDS_SET1_REG, 0U); // [0] Disable LVDS

        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_TEST_COLOR_SET_REG,
                     0x00000000U); // test color mode=0, None

        // CTG Setting
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_TCON_CTG_REG,
                     0x00010300U); // ctg_reset_on
        (void)usleep(1);           // 1 μs
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_TCON_CTG_REG,
                     0x00000307U); // enable ctg 0 1 2

        // Enable LCD
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG, 0x00000001U);
        (void)usleep(10);
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG, 0x00000003U);
        (void)usleep(10000);
    }
    else if (gPanelType == ITH_LCD_CPU)
    {
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG, 0x00000001U);
        usleep(10);
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG, 0x00000003U);
        usleep(10000);

        ithLcdResetPinShareInfo();
    }
	else
	{
        //gPanelType == ITH_LCD_RGB
    
        // CTG Setting
        (void)usleep(100);
        // ctg_reset_on
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_TCON_CTG_REG, 0x00010300U);
        (void)usleep(1);
        // enable ctg 0 1 2
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_TCON_CTG_REG, 0x00000307U);

        (void)usleep(10);
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG, 0x00000001U);
        (void)usleep(10);
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG, 0x00000003U);
        (void)usleep(100);

        ithLcdResetPinShareInfo();   
	}

    // set reg 0x0020 as 0x80000000 for leave test mode
    //  wait for 0x80000000 become 0x00000000
    ithWriteRegA(ITH_LCD_BASE + ITH_LCD_TEST_COLOR_SET_REG, 0x80000000U);
    while (0U != (ithReadRegA(ITH_LCD_BASE + ITH_LCD_TEST_COLOR_SET_REG) & 0x80000000U))
    {
        ithDelay(1000);
    }

}

void ithLcdDisable(void)
{
    // set reg 0x0020 as 0x81000000 for test mode
    //  wait for 0x81000000 become 0x01000000
    if ((ithReadRegA(ITH_LCD_BASE + ITH_LCD_TEST_COLOR_SET_REG) &
         0x01000000U) == 0x0U)
    {
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_TEST_COLOR_SET_REG, 0x81000000U);
        while (0U != (ithReadRegA(ITH_LCD_BASE + ITH_LCD_TEST_COLOR_SET_REG) &
               0x80000000U))
        {
            ithDelay(1000);
        }
    }

    gPanelType = ithLcdGetPanelType();


    if (gPanelType == ITH_LCD_MIPI)
    {
        // save HS and DPHY Info
        ithLcdSaveMipiHSInfo();
        ithLcdSaveMipiDPHYInfo();

        // LCD controller disable, CTG reset
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_TCON_CTG_REG,
                     0x00010307U); // for Hsync=1, Vsync=1
        (void)usleep(1);

        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG, 0x00000002U);
        (void)usleep(10);
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG, 0x00000000U);

#ifdef MIPI_RELOAD_SCRIPT
        ithLcdMIPIEnterSleppMode();
#endif

        // MIPI PHY PORn
        ithWriteRegA(0xD8000044U,
                     0x80280001U); //[31]=1 DPHY PORn, En_W20CLK(mipi
                                   //ctrl),En_W21CLK(mipi phy)
        (void)usleep(1);

        // MIPI controller reset
        ithWriteRegA(0xD800004CU,
                     0xC002C001U); //[31]:Reset MIPI power controller,[30]:Reset
                                   //MIPI system controller,  KDSICLK
    }
    else if (gPanelType == ITH_LCD_LVDS)
    {
        // LCD disable
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_TCON_CTG_REG,
                     0x00010307U); // for Hsync=1, Vsync=1
        (void)usleep(1);

        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG,
                     0x00000002U); // Display disable
        (void)usleep(10);
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG,
                     0x00000000U); // Sync disable
        (void)usleep(100);

        // LVDS disable
        ithClearRegBitA(ITH_MIPI_DPHY_BASE, 24); // [24]PLL disable
        ithClearRegBitA(ITH_LCD_BASE + ITH_LCD_LVDS_SET1_REG,
                        0);                      // [0] Disable LVDS

        // ithWriteRegMaskA(ITH_MIPI_DPHY_BASE, 0x1 << 23 | 0x0 << 22, 0x1 << 23
        // | 0x1 << 22); // // [23:22]Pad Type=TTL (0x10)
        ithSetRegBitA(ITH_MIPI_DPHY_BASE, 23);
        ithClearRegBitA(ITH_MIPI_DPHY_BASE, 22);
        ithSetRegBitA(ITH_MIPI_DPHY_BASE, 8); // [8] LDOPD
    }
    else if (gPanelType == ITH_LCD_CPU)
    {
        ithLcdSavePinShareInfo();

        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG, 0x00000002U);
        usleep(10);
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG, 0x00000000U);
        usleep(100);
    }
	else
	{
        //gPanelType == ITH_LCD_RGB
    
        ithLcdSavePinShareInfo();

        (void)usleep(100);
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_TCON_CTG_REG,
                     0x00010307U); // for Hsync=1, Vsync=1
        (void)usleep(10);
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG, 0x00000002U);
        (void)usleep(10);
        ithWriteRegA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG, 0x00000000U);
        (void)usleep(100);    
	}

#ifdef MIPI_RELOAD_SCRIPT
    if (gPanelType == ITH_LCD_MIPI)
    {
    	lcdA = ithLcdGetBaseAddrA();
    	lcdB = ithLcdGetBaseAddrB();
#if defined(CFG_VIDEO_ENABLE) || defined(CFG_LCD_TRIPLE_BUFFER)
	    lcdC = ithLcdGetBaseAddrC();
#endif
    	ithLcdReset();
	}
	else
#endif
	{
    	// reset LCD controller
    	ithWriteRegMaskA(ITH_HOST_BASE + ITH_LCD_CLK1_REG, 0x1U << ITH_LCD_RST_BIT,
                     0x1U << ITH_LCD_RST_BIT);
    	(void)usleep(2);
    	ithWriteRegMaskA(ITH_HOST_BASE + ITH_LCD_CLK1_REG, 0x0U,
                     0x1U << ITH_LCD_RST_BIT);
	}

    // disable clock
    ithClearRegBitA(ITH_HOST_BASE + ITH_LCD_CLK1_REG, ITH_EN_M3CLK_BIT);
    ithClearRegBitA(ITH_HOST_BASE + ITH_LCD_CLK1_REG, ITH_EN_W12CLK_BIT);
    if (gPanelType != ITH_LCD_RGB)
    {
        ithClearRegBitA(ITH_HOST_BASE + ITH_LCD_CLK1_REG, ITH_EN_DCLK_BIT);
    }

#if (CFG_GPIO_LCD_PWR_EN > 0)
    #ifdef CFG_GPIO_LCD_PWR_EN_ACTIVE_LOW
    ithGpioSet(CFG_GPIO_LCD_PWR_EN);
    #else
    ithGpioClear(CFG_GPIO_LCD_PWR_EN);
    #endif
#endif
}

void ithLcdSetBaseAddrA(uint32_t addr)
{
    ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_BASEA_REG, addr, ITH_LCD_BASEA_MASK);
}

void ithLcdSetBaseAddrB(uint32_t addr)
{
    ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_BASEB_REG, addr, ITH_LCD_BASEB_MASK);
}

uint32_t ithLcdGetBaseAddrB(void)
{
    return (ithReadRegA(ITH_LCD_BASE + ITH_LCD_BASEB_REG) & ITH_LCD_BASEB_MASK);
}

void ithLcdSetBaseAddrC(uint32_t addr)
{
    ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_BASEC_REG, addr, ITH_LCD_BASEC_MASK);
}

uint32_t ithLcdGetBaseAddrC(void)
{
    return (ithReadRegA(ITH_LCD_BASE + ITH_LCD_BASEC_REG) & ITH_LCD_BASEC_MASK);
}

void ithLcdEnableHwFlip(void)
{
    ithSetRegBitA(ITH_LCD_BASE + ITH_LCD_SET1_REG, ITH_LCD_HW_FLIP_BIT);
}

void ithLcdDisableHwFlip(void)
{
    ithClearRegBitA(ITH_LCD_BASE + ITH_LCD_SET1_REG, ITH_LCD_HW_FLIP_BIT);
}

void ithLcdEnableVideoFlip(void)
{
    ithLcdDisableHwFlip();
    ithSetRegBitA(ITH_LCD_BASE + ITH_LCD_SET1_REG, ITH_LCD_VIDEO_FLIP_EN_BIT);
}

void ithLcdDisableVideoFlip(void)
{
    ithLcdDisableHwFlip();
    ithClearRegBitA(ITH_LCD_BASE + ITH_LCD_SET1_REG, ITH_LCD_VIDEO_FLIP_EN_BIT);
}

void ithLcdLoadScriptFirst(const uint32_t* script, unsigned int count)
{
    unsigned int i;

    lcdScript      = script;
    lcdScriptCount = count;

    // Run script until fire
    for (i = 0U; i < count; i += 2U)
    {
        unsigned int reg = script[i];
        unsigned int val = script[i + 1U];

        if (reg == CMD_DELAY)
        {
            ithDelay(val);
        }
        else
        {
            if ((reg == (ITH_LCD_BASE + ITH_LCD_UPDATE_REG)) &&
                (0U != (val & ITH_LCD_SYNCFIRE_MASK)))
            {
                break;
            }
            if ((reg == (ITH_GPIO_BASE + ITH_GPIO1_PINDIR_REG)) || (reg == (ITH_GPIO_BASE + ITH_GPIO2_PINDIR_REG)) || (reg == (ITH_GPIO_BASE + ITH_GPIO3_PINDIR_REG)) || (reg == (ITH_GPIO_BASE + ITH_GPIO4_PINDIR_REG)))
            {
                ithWriteRegMaskA(reg, val, val);
            }
            else
            {
                ithWriteRegA(reg, val);
            }
        }
    }
    lcdScriptIndex = i;
}

void ithLcdLoadScriptNext(void)
{
    unsigned int i, lcdwidth;

    for (i = lcdScriptIndex; i < lcdScriptCount; i += 2U)
    {
        unsigned int reg = lcdScript[i];
        unsigned int val = lcdScript[i + 1U];

        if (reg != CMD_DELAY)
        {
            ithWriteRegA(reg, val);
        }
    }
}

void ithLcdLoadScriptMIPIRead(const uint32_t* script, unsigned int count)
{
    unsigned int i;

    lcdScript      = script;
    lcdScriptCount = count;

    // Run script until fire
    for (i = 0U; i < count; i += 2U)
    {
        unsigned int reg = script[i];
        unsigned int val = script[i + 1U];

        if (reg == CMD_DELAY)
        {
            ithDelay(val);
        }
        else
        {
            if ((reg == 0xD00000F0U) && (val == 0x0000A011U))
            {
                break;
            }
            ithWriteRegA(reg, val);
        }
    }

    lcdScriptIndex = i;
}

uint32_t ithLcdMIPIReadData(uint8_t command)
{
    uint16_t readCommand = 0U;

    /* ************************************************* */
    /*                LP  to Dispaly                     */
    /* ************************************************* */
    // Select MIPI disable
    ithWriteRegA(0xD0000230U, 0x00000016U);        // [0]:Select MIPI disable
    (void)usleep(1);

    /* ************************************************* */
    /*    Clear  0xB0000234[15:8]                        */
    /* ************************************************* */
    ithWriteRegA(0xD00000ecU, 0x00020000U);     //Force CSN=0
    (void)usleep(1);
    ithWriteRegA(0xD00000ecU, 0x00030000U);     //Force CSN=1
    (void)usleep(1);

    /* ************************************************* */
    /*    Panel Setting (CPUIF FOR DBI), 0xB000_0000     */
    /* ************************************************* */
    ithWriteRegA(0xD0000004U, 0x0F7F0410U);        // CPUIF
    (void)usleep(200000);

    // Select MIPI enable
    ithWriteRegA(0xD0000230U, 0x00000017U);        // [0]:Select MIPI enable
    (void)usleep(1);

    /* ************************************************* */
    /*     Reset  MIPI Controller                        */
    /* ************************************************* */
    ithWriteRegA(0xD800004CU, 0xC002C801U);        // KDSICLK
    (void)usleep(10);

    /* ************************************************* */
    /*     Enable MIPI controller                        */
    /* ************************************************* */
    ithWriteRegA(0xD800004CU, 0x0002C001U);        // MIPI controller normal
    (void)usleep(200);

    (void)usleep(200000);

    /* ************************************************* */
    /*    MIPI reg base: 0xD0c00000 (LP)                 */
    /* ************************************************* */
    // ----------LP----------- //
    ithWriteRegA(0xD0C00004U, 0x000F020FU);        // 0x6[7]=BLLP, +0x04[0]=EOTPGE
    ithWriteRegA(0xD0C00010U, 0x000F0000U);
    ithWriteRegA(0xD0C00014U, 0x0000001BU);
    (void)usleep(200000);

    // -------_MIPI LP END-------- //
    /* ************************************************* */
    /*                   CPUIF Setting                   */
    /* ************************************************* */
    ithWriteRegA(0xD00000F4U, 0x60514242U);       // CSN,DCN,WRN,RDN


    /* ************************************************* */
    /*    MIPI PHY Turn around Phase delay               */
    /* ************************************************* */
    ithWriteRegA(0xD0D00000U, 0x413E10E4U);        // PLL ENABLE

    ithWriteRegA(0xD0D00014U, 0x0503F004U);        //Turn around ctrl

    /* ************************************************* */
    /* ST7703                                            */
    /* RDID1 : 0xDA =0x38h                               */
    /* RDID2 : 0xDB =0x21h                               */
    /* RDID3 : 0xDC =0x1Fh                               */
    /* ************************************************* */

    ////LP CMD////
    ithWriteRegA(0xD0000104U, 0x00040000U);     //[20:16]read data cnt,[8:0]: CPUIF read data valid bit [40:30]
    (void)usleep(1);
    ithWriteRegA(0xD0000230U, 0x10060017U);     //[24]ge,[21:16]ct=06 DSI Read
    (void)usleep(1);

    ithWriteRegA(0xD00000ecU, 0x00020000U);     //Force CSN=0
    (void)usleep(1);

    readCommand = 0x0000A000U | ((uint16_t)command);
    (void)printf("readCommand:0x%x\n", readCommand);
    ithWriteRegA(0xD00000f0U, readCommand);     //[7:0]cmd  <= *** User MODIFY***
    (void)usleep(1);


    ithWriteRegA(0xD00000ecU, 0x08020000U);     //[27]:cpurden=1,Force CSN=0
    (void)usleep(1);
    ithWriteRegA(0xD00000f0U, 0x0000B000U);     //Force DCN=1, fire
    (void)usleep(10);
    ithWriteRegA(0xD00000ecU, 0x00030000U);     //27]:cpurden=0,Force CSN=1
    (void)usleep(1);

    (void)usleep(1000);


    if (0U != ithReadRegA(0xD0C00030U))
    {
        (void)printf("MIPI Read fail\n");
        return 0;
    }
    else
    {
#if 0
        (void)printf("Read MIPI valid count:%d\n", (ithReadRegA(0xD0000234) & 0xFF00) >> 8);
#endif
        return ithReadRegA(0xD0000238U);
    }
    //READ status
    // 0xD0C00030
    // 0xD0000234 ; //[15:8] read valid count
    // 0xD0000238 ; //[21:16]read Data select ; [15:0] DBI read data

}

void ithLcdLoadScriptMIPINext(void)
{
    unsigned int i, lcdwidth;

    ithWriteRegA(0xD00000ECU, 0x00020000U);    // Force CSN=0
    (void)usleep(1);
    ithWriteRegA(0xD0000230U, 0x10050017U);    // ct=05
    (void)usleep(1);
    #if 0
    ithWriteRegA(0xD00000F0U, 0x0000A011U);    // cmd 0x11, Sleep Out
    (void)usleep(1);
    ithWriteRegA(0xD00000ECU, 0x00030000U);    // Force CSN=1
    #endif

    for (i = lcdScriptIndex; i < lcdScriptCount; i += 2U)
    {
        unsigned int reg = lcdScript[i];
        unsigned int val = lcdScript[i + 1U];
#if 0
        (void)printf("Write MIPI next:0x%x 0x%x\n", reg, val);
#endif
        if (reg != CMD_DELAY)
        {
            ithWriteRegA(reg, val);
        }
    }

    //check LCD width alignment
    lcdwidth = ithLcdGetWidth();
    if (0U != (lcdwidth % 4U))
    {
        lcdwidth = ((lcdwidth >> 2U) << 2U) + 4U;
        ithLcdSetPitch(lcdwidth * 2U);
        (void)printf("check alignment ithLcdGetPitch():%d\n", ithLcdGetPitch());
    }
}

void ithLcdCursorSetBaseAddr(uint32_t addr)
{
    ithWriteRegA(ITH_LCD_BASE + ITH_LCD_HWC_BASE_REG, addr);
}

void ithLcdCursorSetColorWeight(ITHLcdCursorColor color, uint8_t value)
{
    switch (color)
    {
    case ITH_LCD_CURSOR_DEF_COLOR:
        ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_HWC_INVCOLORWEI_REG, ((uint32_t)value) << ITH_LCD_HWC_INVCOLORWEI_BIT, ITH_LCD_HWC_INVCOLORWEI_MASK);
        break;

    case ITH_LCD_CURSOR_FG_COLOR:
        ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_HWC_FORECOLORWEI_REG, ((uint32_t)value) << ITH_LCD_HWC_FORECOLORWEI_BIT, ITH_LCD_HWC_FORECOLORWEI_MASK);
        break;

    case ITH_LCD_CURSOR_BG_COLOR:
        ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_HWC_BACKCOLORWEI_REG, ((uint32_t)value) << ITH_LCD_HWC_BACKCOLORWEI_BIT, ITH_LCD_HWC_BACKCOLORWEI_MASK);
        break;

    default:
        /* do nothing */
        break;
    }
}

ITHLcdFormat ithLcdGetFormat(void)
{
    return (ITHLcdFormat)((ithReadRegA(ITH_LCD_BASE + ITH_LCD_SRCFMT_REG) & ITH_LCD_SRCFMT_MASK) >> ITH_LCD_SRCFMT_BIT);
}

void ithLcdSetWidth(uint32_t width)
{
    ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_WIDTH_REG, width << ITH_LCD_WIDTH_BIT, ITH_LCD_WIDTH_MASK);
}


void ithLcdSetHeight(uint32_t height)
{
    ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_HEIGHT_REG, height << ITH_LCD_HEIGHT_BIT, ITH_LCD_HEIGHT_MASK);
}

void ithLcdSetPitch(uint32_t pitch)
{
    ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_PITCH_REG, pitch << ITH_LCD_PITCH_BIT, ITH_LCD_PITCH_MASK);
}

unsigned int ithLcdGetXCounter(void)
{
    return (ithReadRegA(ITH_LCD_BASE + ITH_LCD_CTGH_CNT_REG) & ITH_LCD_CTGH_CNT_MASK) >> ITH_LCD_CTGH_CNT_BIT;
}

unsigned int ithLcdGetYCounter(void)
{
    return (ithReadRegA(ITH_LCD_BASE + ITH_LCD_CTGH_CNT_REG) & ITH_LCD_CTGV_CNT_MASK) >> ITH_LCD_CTGV_CNT_BIT;
}

void ithLcdSyncFire(void)
{
    ithSetRegBitA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG, ITH_LCD_SYNCFIRE_BIT);
}

 bool ithLcdIsSyncFired(void)
{
     return (ithReadRegA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG) & ITH_LCD_SYNCFIRE_MASK);
}

bool ithLcdIsEnabled (void)
{
     return ((ithReadRegA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG) & (ITH_LCD_DISPEN_MASK | ITH_LCD_SYNCFIRE_MASK)) ==
             (ITH_LCD_DISPEN_MASK | ITH_LCD_SYNCFIRE_MASK))
                ? true
                : false;
}

unsigned int ithLcdGetFlip (void)
{
     return (ithReadRegA(ITH_LCD_BASE + ITH_LCD_READ_STATUS1_REG) & ITH_LCD_FLIP_NUM_MASK) >> ITH_LCD_FLIP_NUM_BIT;
}

unsigned int ithLcdGetMaxLcdBufCount(void)
{
#ifdef CFG_VIDEO_ENABLE
    return 3U;
#else
    return 2U;
#endif
}

void ithLcdSwFlip(unsigned int index)
{
    ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_SWFLIPNUM_REG, index << ITH_LCD_SWFLIPNUM_BIT, ITH_LCD_SWFLIPNUM_MASK);
    ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG, 1UL << ITH_LCD_LAYER1UPDATE_BIT, ITH_LCD_LAYER1UPDATE_MASK);
}

void ithLcdCursorEnable(void)
{
    ithSetRegBitA(ITH_LCD_BASE + ITH_LCD_HWC_EN_REG, ITH_LCD_HWC_EN_BIT);
}

void ithLcdCursorDisable(void)
{
    ithClearRegBitA(ITH_LCD_BASE + ITH_LCD_HWC_EN_REG, ITH_LCD_HWC_EN_BIT);
}

void ithLcdCursorCtrlEnable(ITHLcdCursorCtrl ctrl)
{
    switch(ctrl)
    {
    case ITH_LCD_CURSOR_ALPHABLEND_ENABLE:
        ithSetRegBitA(ITH_LCD_BASE + ITH_LCD_HWC_CR_REG, ITH_LCD_HWC_ABLDEN_BIT);
        break;
    case ITH_LCD_CURSOR_DEFDEST_ENABLE:
        ithSetRegBitA(ITH_LCD_BASE + ITH_LCD_HWC_CR_REG, ITH_LCD_HWC_DEFDSTEN_BIT);
        break;
    case ITH_LCD_CURSOR_INVDEST_ENABLE:
        ithSetRegBitA(ITH_LCD_BASE + ITH_LCD_HWC_CR_REG, ITH_LCD_HWC_DEFINVDST_BIT);
        break;
    default:
        /* do nothing */
        break;
    }
}

void ithLcdCursorCtrlDisable(ITHLcdCursorCtrl ctrl)
{
    switch(ctrl)
    {
    case ITH_LCD_CURSOR_ALPHABLEND_ENABLE:
        ithClearRegBitA(ITH_LCD_BASE + ITH_LCD_HWC_CR_REG, ITH_LCD_HWC_ABLDEN_BIT);
        break;
    case ITH_LCD_CURSOR_DEFDEST_ENABLE:
        ithClearRegBitA(ITH_LCD_BASE + ITH_LCD_HWC_CR_REG, ITH_LCD_HWC_DEFDSTEN_BIT);
        break;
    case ITH_LCD_CURSOR_INVDEST_ENABLE:
        ithClearRegBitA(ITH_LCD_BASE + ITH_LCD_HWC_CR_REG, ITH_LCD_HWC_DEFINVDST_BIT);
        break;
    default:
        /* do nothing */
        break;
    }
}

void ithLcdCursorSetWidth(unsigned int width)
{
    ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_HWC_WIDTH_REG, width << ITH_LCD_HWC_WIDTH_BIT, ITH_LCD_HWC_WIDTH_MASK);
}

void ithLcdCursorSetHeight(unsigned int height)
{
    ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_HWC_HEIGHT_REG, height << ITH_LCD_HWC_HEIGHT_BIT, ITH_LCD_HWC_HEIGHT_MASK);
}

void ithLcdCursorSetPitch(unsigned int pitch)
{
    ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_HWC_PITCH_REG, pitch << ITH_LCD_HWC_PITCH_BIT, ITH_LCD_HWC_PITCH_MASK);
}

void ithLcdCursorSetX(unsigned int x)
{
    ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_HWC_POSX_REG, x << ITH_LCD_HWC_POSX_BIT, ITH_LCD_HWC_POSX_MASK);
}

void ithLcdCursorSetY(unsigned int y)
{
    ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_HWC_POSY_REG, y << ITH_LCD_HWC_POSY_BIT, ITH_LCD_HWC_POSY_MASK);
}

void ithLcdCursorSetColor(ITHLcdCursorColor color, uint16_t value)
{
    switch(color)
    {
        case ITH_LCD_CURSOR_DEF_COLOR:
            ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_HWC_DEFCOLOR_REG, ((uint32_t)value) << ITH_LCD_HWC_DEFCOLOR_BIT, ITH_LCD_HWC_DEFCOLOR_MASK);
            break;
        case ITH_LCD_CURSOR_FG_COLOR:
            ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_HWC_FORECOLOR_REG, ((uint32_t)value) << ITH_LCD_HWC_FORECOLOR_BIT, ITH_LCD_HWC_FORECOLOR_MASK);
            break;
        case ITH_LCD_CURSOR_BG_COLOR:
            ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_HWC_BACKCOLOR_REG, ((uint32_t)value) << ITH_LCD_HWC_BACKCOLOR_BIT, ITH_LCD_HWC_BACKCOLOR_MASK);
            break;
        default:
            /* do nothing */
            break;
    }
}

void ithLcdCursorUpdate(void)
{
    ithSetRegBitA(ITH_LCD_BASE + ITH_LCD_HWC_UPDATE_REG, ITH_LCD_HWC_UPDATE_BIT);
}

 bool ithLcdCursorIsUpdateDone(void)
{
     return ithReadRegA(ITH_LCD_BASE + ITH_LCD_HWC_UPDATE_REG) & (0x1UL << ITH_LCD_HWC_UPDATE_BIT);
}

void ithLcdIntrCtrlEnable (ITHLcdIntrCtrl ctrl)
{
     switch (ctrl)
     {
        case ITH_LCD_INTR_ENABLE:
            ithSetRegBitA(ITH_LCD_BASE + ITH_LCD_INT_CTRL_REG,
                          ITH_LCD_INT_EN_BIT);
            break;
        case ITH_LCD_INTR_FIELDMODE1:
            ithSetRegBitA(ITH_LCD_BASE + ITH_LCD_INT_CTRL_REG,
                          ITH_LCD_INT_FIELDMODE1_BIT);
            break;

        case ITH_LCD_INTR_OUTPUT2:
        case ITH_LCD_INTR_FIELDMODE2:
        case ITH_LCD_INTR_OUTPUT1:
        default:
            /* do nothing */
            break;
     }
}

void ithLcdIntrCtrlDisable (ITHLcdIntrCtrl ctrl)
{
     switch (ctrl)
     {
        case ITH_LCD_INTR_ENABLE:
            ithClearRegBitA(ITH_LCD_BASE + ITH_LCD_INT_CTRL_REG,
                            ITH_LCD_INT_EN_BIT);
            break;

        case ITH_LCD_INTR_FIELDMODE1:
            ithClearRegBitA(ITH_LCD_BASE + ITH_LCD_INT_CTRL_REG,
                            ITH_LCD_INT_FIELDMODE1_BIT);
            break;

        case ITH_LCD_INTR_OUTPUT2:
        case ITH_LCD_INTR_FIELDMODE2:
        case ITH_LCD_INTR_OUTPUT1:
        default:
            /* do nothing */
            break;
     }
}

void ithLcdIntrEnable(void)
{
    ithLcdIntrCtrlEnable(ITH_LCD_INTR_ENABLE);
}

void ithLcdIntrDisable(void)
{
    ithLcdIntrCtrlDisable(ITH_LCD_INTR_ENABLE);
}

void ithLcdIntrClear(void)
{
    ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_INT_CLR_REG, 0x1UL << ITH_LCD_INT_CLR_BIT, ITH_LCD_INT_CLR_MASK);
}

void ithLcdIntrSetScanLine1(unsigned int line)
{
    ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_INT_LINE1_REG, line << ITH_LCD_INT_LINE1_BIT, ITH_LCD_INT_LINE1_MASK);
}

void ithLcdIntrSetScanLine2(unsigned int line)
{

}

void ithLcdSetRotMode(ITHLcdScanType type, ITHLcdRotMode mode)
{
    ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_SET1_REG, type << ITH_LCD_SCAN_TYPE_BIT, 0x1UL << ITH_LCD_SCAN_TYPE_BIT);
    ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_SET1_REG, mode << ITH_LCD_ROT_MODE_BIT, ITH_LCD_ROT_MODE_MASK);

    ithWriteRegMaskA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG, 1UL << ITH_LCD_LAYER1UPDATE_BIT, ITH_LCD_LAYER1UPDATE_MASK);

    while (0U != (ithReadRegA(ITH_LCD_BASE + ITH_LCD_UPDATE_REG) & (0x1UL << ITH_LCD_LAYER1UPDATE_BIT)))
    {
        ithDelay(1000);
    }
}

ITHLcdPanelType ithLcdGetPanelType (void)
{
    if (0U != (ithReadRegA(ITH_LCD_BASE + ITH_LCD_MIPI_SET1_REG) & 0x1U))
    {
        gPanelType = ITH_LCD_MIPI;
    }
    else if (0U != (ithReadRegA(ITH_LCD_BASE + ITH_LCD_LVDS_SET1_REG) & 0x1U))
    {
        gPanelType = ITH_LCD_LVDS;
    }
    else if (0U != (ithReadRegA(ITH_LCD_BASE + ITH_LCD_CPUSERIAL_SET4_REG) & 0x1U))
    {
        gPanelType = ITH_LCD_CPU;
    }
    else
    {
        gPanelType = ITH_LCD_RGB;
    }

    return gPanelType;
}

/**
* Setting RGB gain API for White Balance tunning.
*
* @param gain's range is 0~255.
*/
int ithLcdSetWhiteBalance(uint8_t gain_r, uint8_t gain_g, uint8_t gain_b)
{
    EV_MATRIX config;
    int  result = 0;

    // Convert gain range (0~255 → 0.0~1.99)
    // 3×3 gain matrix (diagonal matrix)
    config.M_color_correction[0][0] = (gain_r > 0) ? gain_r / 128.0f : 0.01f;
    config.M_color_correction[0][1] = 0.0f;;
    config.M_color_correction[0][2] = 0.0f;;

    config.M_color_correction[1][0] = 0.0f;;
    config.M_color_correction[1][1] = (gain_g > 0) ? gain_g / 128.0f : 0.01f;
    config.M_color_correction[1][2] = 0.0f;;

    config.M_color_correction[2][0] = 0.0f;;
    config.M_color_correction[2][1] = 0.0f;;
    config.M_color_correction[2][2] = (gain_b > 0) ? gain_b / 128.0f : 0.01f;

    // 3×1 offset matrix (based on gain compensation)
    // Compute offset compensation based on gain deviation from 1.0
    // Offset formula: O_x = ((gain_x - 128) / 255) * 128
#ifdef RGB_GAIN_COMPENSATION
    config.V_color_correction[0][0] = ((float)(gain_r - 128) / 255.0f) * 128.0f;  // Red offset
    config.V_color_correction[1][0] = ((float)(gain_g - 128) / 255.0f) * 128.0f;  // Green offset
    config.V_color_correction[2][0] = ((float)(gain_b - 128) / 255.0f) * 128.0f;  // Blue offset
#else
    config.V_color_correction[0][0] = 0.0f;;
    config.V_color_correction[1][0] = 0.0f;;
    config.V_color_correction[2][0] = 0.0f;;
#endif   
    
    soc_write__color_conversion_(config);

    return result;
}


/**
* Setting hue API in LCD color conversion registers.
*
* @param hue (Hue)'s range is 0~359.
*/
int ithLcdSetHue(int hue)
{
    static int gHue;
    int        result;

    if ((hue < 0) || (hue > 359))
    {
        (void)printf("[ERROR] Angle (Hue)'s range is 0~359, but %d is given.\n'", hue);
        return 1;
    }

    gHue   = hue;
    result = color_conversion_main_(gHue);

    return result;
}

/**
* Setting contrast API in LCD gamma function registers.
*
* @param contrast Contrast's range is -64~63.
*/
int ithLcdSetContrast(int contrast)
{
    int result;

    if ((contrast < -64) || (contrast > 63))
    {
        (void)printf("[ERROR] (Contrast)'s range is -64~63, but (%d) is given.\n'", contrast);
        return 1;
    }

    gContrast = contrast;
    result = gamma_function_api_(gContrast, gBrightness);

    return result;
}

/**
* Setting brightness API in LCD gamma function registers.
*
* @param brightness Brightness's range is -64~63.
*/
int ithLcdSetBrightness(int brightness)
{
    int result;

    if ((brightness < -64) || (brightness > 63))
    {
        (void)printf("[ERROR] (Brightness)'s range is -64~63, but (%d) is given.\n'", brightness);
        return 1;
    }

    gBrightness = brightness;
    result = gamma_function_api_(gContrast, gBrightness);

    return result;
}
