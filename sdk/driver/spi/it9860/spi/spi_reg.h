#ifndef SPI_FTSSP_H
#define SPI_FTSSP_H

#include "ith/ith_defs.h"

#ifdef _WIN32
    #define REG_SPI_PORT_OFFSET 0x400U
#else
    #define REG_SPI_PORT_OFFSET 0x100000U
#endif

#define SPI0_BASE                           (0xD1700000U)
#define SPI1_BASE                           (SPI0_BASE + REG_SPI_PORT_OFFSET)

#define REG_SPI_CONTROL_0                   (0x00U)
#define REG_SPI_CONTROL_1                   (0x04U)
#define REG_SPI_CONTROL_2                   (0x08U)
#define REG_SPI_STATUS                      (0x0CU)
#define REG_SPI_INTR_CONTROL                (0x10U)
#define REG_SPI_INTR_STATUS                 (0x14U)
#define REG_SPI_DATA                        (0x18U)
#define REG_SPI_FREERUN                     (0x1CU)
#define REG_SPI_REVERSION                   (0x7CU)
#define REG_SPI_FEATURE                     (0x78U)
#define REG_SPI_MISC                        (0x74U)
#define REG_CLKSRC_OFFSET                   30U
#define REG_CLKSRC_MASK                     (((uint32_t)1U) << 30U)

/*SPI Control Register 0*/
#define REG_CR0_TXENDIAN_LITTLE             (((uint32_t)0U) << 17U)
#define REG_CR0_TXENDIAN_BIG                (((uint32_t)1U) << 17U)
#define REG_CR0_TXENDIAN_MASK               0x00020000U
#define REG_CR0_RXENDIAN_LITTLE             0U
#define REG_CR0_RXENDIAN_BIG                1U
#define REG_CR0_RXENDIAN_MASK               0x00010000U
#define REG_CR0_FFMT_SPI                    (((uint32_t)1U) << 12U)
#define REG_CR0_FFMT_MASK                   0x00007000U
#define REG_CR0_FSPO_HIGH                   0
#define REG_CR0_FSPO_LOW                    (((uint32_t)1U) << 5U)
#define REG_CR0_FSPO_MASK                   0x00000020U
#define REG_CR0_OPM_SLAVE                   0x00
#define REG_CR0_OPM_MASTER                  (((uint32_t)0x3U) << 2U)
#define REG_CR0_OPM_MASK                    0x0000000CU
#define REG_CR0_SCLKPO_LOW                  0
#define REG_CR0_SCLKPO_HIGH                 (((uint32_t)1U) << 1U)
#define REG_CR0_SCLKPO_MASK                 0x00000002U
#define REG_CR0_SCLKPH_DISABLE              0
#define REG_CR0_SCLKPH_ENABLE               (((uint32_t)1U) << 0U)
#define REG_CR0_SCLKPH_MASK                 0x00000001U

/*SPI Control Register 1*/
#define REG_CR1_SDL_MAX_VALUE               31U
#define REG_CR1_SDL_OFFSET                  16U
#define REG_CR1_SDL_MASK                    0x001F0000U
#define REG_CR1_SCLKDIV_MAX_VALUE           65535U
#define REG_CR1_SCLKDIV_MASK                0x0000FFFFU

/*SPI Control Register 2*/
#define REG_CR2_TXFCLR_ENABLE               (((uint32_t)1U) <<3)
#define REG_CR2_TXFCLR_NO_EFFECT            0U
#define REG_CR2_TXFCLR_MASK                 0x00000008U
#define REG_CR2_RXFCLR_ENABLE               (((uint32_t)1U) <<2)
#define REG_CR2_RXFCLR_NO_EFFECT            0U
#define REG_CR2_RXFCLR_MASK                 0x00000004U
#define REG_CR2_TXDOE_ENABLE                (((uint32_t)1U) <<1)
#define REG_CR2_TXDOE_DISABLE               0U
#define REG_CR2_TXDOE_MASK                  0x00000002U
#define REG_CR2_SPI_ENABLE                  1U
#define REG_CR2_SPI_DISABLE                 0U
#define REG_CR2_SPI_MASK                    0x00000001U

/*SPI Status Register */
#define REG_STR_TFVE_OFFSET                 12U
#define REG_STR_TFVE_MASK                   0x0007F000U
#define REG_STR_RFVE_OFFSET                 4U
#define REG_STR_RFVE_MASK                   0x000007F0U
#define REG_STR_BUSY_OFFSET                 3U
#define REG_STR_BUSY_MASK                   0x00000008U
#define REG_STR_TFNF_OFFSET                 1U
#define REG_STR_TFNF_MASK                   0x00000002U
#define REG_STR_RFF_MASK                    0x00000001U

/*SSP Interrupt Control Register */
#define REG_ICR_TFTHOD_MAX                  63U
#define REG_ICR_TFTHOD_OFFSET               16U
#define REG_ICR_TFTHOD_MASK                 0x003F0000U
#define REG_ICR_RFTHOD_MAX                  63U
#define REG_ICR_RFTHOD_OFFSET               8U
#define REG_ICR_RFTHOD_MASK                 0x00003F00U
#define REG_ICR_TX_DMA_ENABLE               (((uint32_t)1U) <<5)
#define REG_ICR_TX_DMA_DISABLE              0U
#define REG_ICR_TX_DMA_MASK                 0x00000020U
#define REG_ICR_RX_DMA_ENABLE               (((uint32_t)1U) <<4)
#define REG_ICR_RX_DMA_DISABLE              0U
#define REG_ICR_RX_DMA_MASK                 0x00000010U
#define REG_ICR_TX_THRESHOLD_INTR_ENABLE    (((uint32_t)1U) <<3)
#define REG_ICR_TX_THRESHOLD_INTR_DISABLE   0U
#define REG_ICR_TX_THRESHOLD_INTR_MASK      0x00000008U
#define REG_ICR_RX_THRESHOLD_INTR_ENABLE    (((uint32_t)1U) <<2)
#define REG_ICR_RX_THRESHOLD_INTR_DISABLE   0U
#define REG_ICR_RX_THRESHOLD_INTR_MASK      0x00000004U
#define REG_ICR_TX_UNDERRUN_INTR_ENABLE     (((uint32_t)1U) <<1)
#define REG_ICR_TX_UNDERRUN_INTR_DISABLE    0U
#define REG_ICR_TX_UNDERRUN_INTR_MASK       0x00000002U
#define REG_ICR_RX_OVERRUN_INTR_ENABLE      (((uint32_t)1U) <<0)
#define REG_ICR_RX_OVERRUN_INTR_DISABLE     0U
#define REG_ICR_RX_OVERRUN_INTR_MASK        0x00000001U

/*SSP Interrupt Status Register */
#define REG_ISR_TX_THRESHOLD_INTR_MASK      0x00000008U
#define REG_ISR_RX_THRESHOLD_INTR_MASK      0x00000004U
#define REG_ISR_TX_UNDERRUN_INTR_MASK       0x00000002U
#define REG_ISR_RX_OVERRUN_INTR_MASK        0x00000001U

/*TX FIFO Free Run Register*/
#define REG_TXFR_FRUN_COUNT_MAX_VALUE       65535U
#define REG_TXFR_FRUN_COUNT_OFFSET          16U
#define REG_TXFR_FRUN_COUNT_MASK            0xFFFF0000U

#define REG_TXFR_FRUN_FILTER_LENG_MAX_VALUE 255U
#define REG_TXFR_FRUN_FILTER_LENG_OFFSET    8U
#define REG_TXFR_FRUN_FILTER_LENG_MASK      0x0000FF00U
#define REG_TXFR_FRUN_FILTER_ENABLE         (((uint32_t)1U) << 4U)
#define REG_TXFR_FRUN_FILTER_DISABLE        0U
#define REG_TXFR_FRUN_FILTER_MASK           0x00000010U

#define REG_TXFR_CSNCLR_NORMAL              0U
#define REG_TXFR_CSNCLR_ALWAYS_ZERO         (((uint32_t)1U) << 2U)
#define REG_TXFR_CSNCLR_MASK                0x00000004U

#define REG_TXFR_FRUN_RXLOCK_0              0U
#define REG_TXFR_FRUN_RXLOCK_1              (((uint32_t)1U) << 1U)
#define REG_TXFR_FRUN_RXLOCK_MASK           0x00000002U

#define REG_TXFR_FRUN_FIRE_ENABLE           ((uint32_t)1U)
#define REG_TXFR_FRUN_FIRE_DISABLE          ((uint32_t)0U)
#define REG_TXFR_FRUN_FIRE_MASK             0x00000001U

/*SPI Revision Register */
#define REG_REV_MAJOR_REV_OFFSET            16U
#define REG_REV_MAJOR_REV_MASK              0x00FF0000U
#define REG_REV_MINOR_REV_OFFSET            8U
#define REG_REV_MINOR_REV_MASK              0x0000FF00U
#define REG_REV_RELEASE_REV_MASK            0x000000FFU

/*SPI Misc Register */
#define REG_CLKOUT_INV_ENABLE               ((uint32_t)1U)
#define REG_CLKOUT_INV_DISABLE              ((uint32_t)0U)
#define REG_CLKOUT_INV_OFFSET               25U
#define REG_CLKOUT_INV_MASK                 (((uint32_t)1U) << 25U)
#define REG_CLKOUT_BYPASS_ENABLE            ((uint32_t)1U)
#define REG_CLKOUT_BYPASS_DISABLE           ((uint32_t)0U)
#define REG_CLKOUT_BYPASS_OFFSET            24U
#define REG_CLKOUT_BYPASS_MASK              (((uint32_t)1U) <<24U)
#define REG_CLKOUT_DALAY_MASK               (((uint32_t)0xFFU) << 16U)
#define REG_CLKOUT_DALAY_OFFSET             16U
#endif
