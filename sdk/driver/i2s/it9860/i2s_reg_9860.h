/* sy.chuang, 2012-0423, ITE Tech. */

#ifndef I2S_REG_H
#define I2S_REG_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ************************************************************************** */
#define I2S_REG_BASE           0xD0100000U

#define I2S_REG_CODEC_SET      (I2S_REG_BASE | 0x00U)

#define I2S_REG_ADC_SRATE_SET  (I2S_REG_BASE | 0x04U)
#define I2S_REG_DAC_SRATE_SET  (I2S_REG_BASE | 0x08U)

#define I2S_REG_DATA_LOOPBACK  (I2S_REG_BASE | 0x0CU)

#define I2S_REG_IN1_BASE1      (I2S_REG_BASE | 0x10U)
#define I2S_REG_IN2_BASE1      (I2S_REG_BASE | 0x20U)
#define I2S_REG_IN2_BASE2      (I2S_REG_BASE | 0x24U)
#define I2S_REG_IN2_BASE3      (I2S_REG_BASE | 0x28U)
#define I2S_REG_IN2_BASE4      (I2S_REG_BASE | 0x2CU)
#define I2S_REG_IN_LEN         (I2S_REG_BASE | 0x30U)
#define I2S_REG_IN_RdWrGAP     (I2S_REG_BASE | 0x34U)
#define I2S_REG_IN_VOLUME      (I2S_REG_BASE | 0x38U)
#define I2S_REG_IN_CTRL        (I2S_REG_BASE | 0x40U)

#define I2S_REG_IN1_STATUS     (I2S_REG_BASE | 0x44U)
// #define I2S_REG_IN2_STATUS          (I2S_REG_BASE | 0x48U)
#define I2S_REG_IN1_RDPTR      (I2S_REG_BASE | 0x50U)
#define I2S_REG_IN1_WRPTR      (I2S_REG_BASE | 0x54U)
#define I2S_REG_IN2_RDPTR      (I2S_REG_BASE | 0x58U)
#define I2S_REG_IN2_WRPTR      (I2S_REG_BASE | 0x5CU)
#define I2S_REG_CODEC_PCM_SET  (I2S_REG_BASE | 0x60U)

#define I2S_REG_OUT_BASE1      (I2S_REG_BASE | 0x70U)
// #define I2S_REG_OUT_BASE2           (I2S_REG_BASE | 0x74U)
// #define I2S_REG_OUT_BASE3           (I2S_REG_BASE | 0x78U)
// #define I2S_REG_OUT_BASE4           (I2S_REG_BASE | 0x7CU)
// #define I2S_REG_OUT_BASE5           (I2S_REG_BASE | 0x80U)
#define I2S_REG_OUT_LEN        (I2S_REG_BASE | 0x90U)
#define I2S_REG_OUT_RdWrGAP    (I2S_REG_BASE | 0x94U)
#define I2S_REG_OUT_BIST       (I2S_REG_BASE | 0x98U)

//
#define I2S_REG_OUT_CTRL       (I2S_REG_BASE | 0xA0U)
//

#define I2S_REG_OUT_CROSSTH    (I2S_REG_BASE | 0xA4U)
#define I2S_REG_OUT_FADE_SET   (I2S_REG_BASE | 0xA8U)
#define I2S_REG_OUT_VOLUME     (I2S_REG_BASE | 0xACU)
#define I2S_REG_SPDIF_VOLUME   (I2S_REG_BASE | 0xB0U)
#define I2S_REG_OUT_STATUS1    (I2S_REG_BASE | 0xB8U)
#define I2S_REG_OUT_STATUS2    (I2S_REG_BASE | 0xBCU)
#define I2S_REG_OUT_WRPTR      (I2S_REG_BASE | 0xC0U)
#define I2S_REG_OUT_RDPTR      (I2S_REG_BASE | 0xC4U)

/* ************************************************************************** */

#define MMP_AUDIO_CLOCK_REG_3C 0xD800003CU
#define MMP_AUDIO_CLOCK_REG_40 0xD8000040U

/* ************************************************************************** */

#ifdef __cplusplus
}
#endif

#endif // I2S_REG_H
