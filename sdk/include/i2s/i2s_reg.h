/* sy.chuang, 2012-0423, ITE Tech. */

#ifndef I2S_REG_H
#define I2S_REG_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ************************************************************************** */
#define I2S_REG_BASE              0x1600U

#define I2S_REG_TX_BUF_1_ADDR_L   (I2S_REG_BASE | 0x00U)
#define I2S_REG_TX_BUF_1_ADDR_H   (I2S_REG_BASE | 0x02U)

#define I2S_REG_TX_BUF_2_ADDR_L   (I2S_REG_BASE | 0x04U)
#define I2S_REG_TX_BUF_2_ADDR_H   (I2S_REG_BASE | 0x06U)

#define I2S_REG_TX_BUF_3_ADDR_L   (I2S_REG_BASE | 0x08U)
#define I2S_REG_TX_BUF_3_ADDR_H   (I2S_REG_BASE | 0x0AU)

#define I2S_REG_TX_BUF_4_ADDR_L   (I2S_REG_BASE | 0x0CU)
#define I2S_REG_TX_BUF_4_ADDR_H   (I2S_REG_BASE | 0x0EU)

#define I2S_REG_TX_BUF_5_ADDR_L   (I2S_REG_BASE | 0x10U)
#define I2S_REG_TX_BUF_5_ADDR_H   (I2S_REG_BASE | 0x12U)

#define I2S_REG_TX_BUF_6_ADDR_L   (I2S_REG_BASE | 0x20U)
#define I2S_REG_TX_BUF_6_ADDR_H   (I2S_REG_BASE | 0x22U)

#define I2S_REG_TX_BUF_SIZE_L     (I2S_REG_BASE | 0x14U)
#define I2S_REG_TX_BUF_SIZE_H     (I2S_REG_BASE | 0x16U)

#define I2S_REG_TX_BUF_W_PTR_L    (I2S_REG_BASE | 0x18U)
#define I2S_REG_TX_BUF_W_PTR_H    (I2S_REG_BASE | 0x1AU)

#define I2S_REG_TX_BUF_R_PTR_L    (I2S_REG_BASE | 0x1CU)
#define I2S_REG_TX_BUF_R_PTR_H    (I2S_REG_BASE | 0x1EU)

#define I2S_REG_RX_CTRL           (I2S_REG_BASE | 0x40U)
#define I2S_REG_TX_CTRL           (I2S_REG_BASE | 0x42U)

#define I2S_REG_ADDA_PCM          (I2S_REG_BASE | 0x44U)

#define I2S_REG_RX_BUF_1_ADDR_L   (I2S_REG_BASE | 0x46U)
#define I2S_REG_RX_BUF_1_ADDR_H   (I2S_REG_BASE | 0x48U)

#define I2S_REG_RX_BUF_SIZE       (I2S_REG_BASE | 0x4AU)
#define I2S_REG_RX_BUF_R_PTR      (I2S_REG_BASE | 0x4CU)
#define I2S_REG_RX_BUF_W_PTR      (I2S_REG_BASE | 0x5AU)

#define I2S_REG_RX_CTRL1          (I2S_REG_BASE | 0x4EU)
#define I2S_REG_RX_CTRL2          (I2S_REG_BASE | 0x50U)

#define I2S_REG_TX_L_WATER_MARK_L (I2S_REG_BASE | 0x52U)
#define I2S_REG_TX_L_WATER_MARK_H (I2S_REG_BASE | 0x54U)

#define I2S_REG_TX_CTRL1          (I2S_REG_BASE | 0x56U)
#define I2S_REG_TX_CTRL2          (I2S_REG_BASE | 0x58U)

#define I2S_REG_RX_STATUS         (I2S_REG_BASE | 0x5CU)
#define I2S_REG_TX_STATUS1        (I2S_REG_BASE | 0x5EU)
#define I2S_REG_TX_STATUS2        (I2S_REG_BASE | 0x60U)

#define I2S_REG_THOLD_CROSS_LO    (I2S_REG_BASE | 0x62U)
#define I2S_REG_THOLD_CROSS_HI    (I2S_REG_BASE | 0x64U)

#define I2S_REG_FADE_IN_OUT_CTRL  (I2S_REG_BASE | 0x66U)

#define I2S_REG_DIG_OUT_VOL       (I2S_REG_BASE | 0x68U)
#define I2S_REG_SPDIF_VOL         (I2S_REG_BASE | 0x6CU)

#define I2S_REG_BITS_CTL          (I2S_REG_BASE | 0x6EU)
#define I2S_REG_DAC_CTL           (I2S_REG_BASE | 0x70U)

#define I2S_REG_AMP_CTL           (I2S_REG_BASE | 0x72U)
#define I2S_REG_AMP_VOL           (I2S_REG_BASE | 0x74U)

#define I2S_REG_HDMI_SYNC_LEFT    (I2S_REG_BASE | 0x76U)
#define I2S_REG_HDMI_SYNC_RIGHT   (I2S_REG_BASE | 0x78U)

#define I2S_REG_HDMI_CTL          (I2S_REG_BASE | 0x7AU)

/* ************************************************************************** */
/* PCM read ptr at REG risc user defined */
// #define I2S_PCM_RDPTR           0x16AC

#define MMP_AUDIO_CLOCK_REG_3C    0x003CU
#define MMP_AUDIO_CLOCK_REG_3E    0x003EU

/* ************************************************************************** */
#ifdef __cplusplus
}
#endif

#endif // I2S_REG_H
