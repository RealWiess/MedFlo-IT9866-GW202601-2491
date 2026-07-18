#ifndef ITH_DEFS_DMA_AXI_H
#define ITH_DEFS_DMA_AXI_H

#define ITH_DMA_INT_REG                 0x00
#define ITH_DMA_INT_TC_REG              0x04
#define ITH_DMA_INT_TC_CLR_REG          0x08

#define ITH_DMA_INT_ERRABT_REG          0x0C
#define ITH_DMA_INT_ERRABT_CLR_REG      0x10

#define ITH_DMA_TC_REG                  0x14
#define ITH_DMA_ERRABT_REG              0x18
#define ITH_DMA_CH_EN_REG               0x1C
#define ITH_DMA_SYNC_REG                0x20
#define ITH_DMA_LDM_REG                 0x24
#define ITH_DMA_WDT_REG                 0x28
#define ITH_DMA_GE_REG                  0x2C
#define ITH_DMA_PSE_REG                 0x30
#define ITH_DMA_REVISION_REG            0x34
#define ITH_DMA_FEATURE_REG             0x38

#define ITH_DMA_WO_VALUE_REG            0x50

#define ITH_DMA_CTRL_CH(x)              (0x100+(x)*0x20)
#define ITH_DMA_CFG_CH(x)               (0x104+(x)*0x20)
#define ITH_DMA_SRC_CH(x)               (0x108+(x)*0x20)
#define ITH_DMA_DST_CH(x)               (0x10C+(x)*0x20)
#define ITH_DMA_LLP_CH(x)               (0x110+(x)*0x20)
#define ITH_DMA_SIZE_CH(x)              (0x114+(x)*0x20)
#define ITH_DMA_STRIDE_CH(x)            (0x118+(x)*0x20)

/*
 * Error/abort interrupt status/clear register
 * Error/abort status register
 */
#define ITH_DMA_EA_ERR_CH(x)            (1U<<(x))
#define ITH_DMA_EA_WDT_CH(x)            (1U<<((x)+8))
#define ITH_DMA_EA_ABT_CH(x)            (1U<<((x)+16))

/*
* Control register
*/
#define ITH_DMA_CTRL_WE(x)              ((1U << (x)) & 0xff)
#define ITH_DMA_CTRL_WSYNC              (1U << 8)
#define ITH_DMA_CTRL_SE(x)              (((x) & 0x7) << 9)
#define ITH_DMA_CTRL_SE_ENABLE          (1U << 12)
#define ITH_DMA_CTRL_WE_ENABLE          (1U << 13)
#define ITH_DMA_CTRL_2D                 (1U << 14)
#define ITH_DMA_CTRL_EXP                (1U << 15)
#define ITH_DMA_CTRL_ENABLE             (1U << 16)
#define ITH_DMA_CTRL_WDT_ENABLE         (1U << 17)
#define ITH_DMA_CTRL_DST_INC            (0x0U << 18)
#define ITH_DMA_CTRL_DST_FIXED          (0x2U << 18)
#define ITH_DMA_CTRL_SRC_INC            (0x0U << 20)
#define ITH_DMA_CTRL_SRC_FIXED          (0x2U << 20)
#define ITH_DMA_CTRL_DST_BURST_CTRL(x)  (((x) & 0x3) << 18)
#define ITH_DMA_CTRL_SRC_BURST_CTRL(x)  (((x) & 0x3) << 20)
#define ITH_DMA_CTRL_DST_WIDTH(x)       (((x) & 0x7) << 22)
#define ITH_DMA_CTRL_SRC_WIDTH(x)       (((x) & 0x7) << 25)
#define ITH_DMA_CTRL_MASK_TC            (1U << 28)
#define ITH_DMA_CTRL_SrcTcnt(x)         (((x) & 0x7) << 29)

/*
* Configuration register
*/
#define ITH_DMA_CFG_MASK_TCI            (1U << 0)	/* mask tc interrupt */
#define ITH_DMA_CFG_MASK_EI             (1U << 1)	/* mask error interrupt */
#define ITH_DMA_CFG_MASK_AI             (1U << 2)	/* mask abort interrupt */
#define ITH_DMA_CFG_SRC_HANDSHAKE(x)    (((x) & 0xf) << 3)
#define ITH_DMA_CFG_SRC_HANDSHAKE_EN(x) ((x) << 7)
#define ITH_DMA_CFG_DST_HANDSHAKE(x)    (((x+8) & 0xf) << 9)
#define ITH_DMA_CFG_DST_HANDSHAKE_EN(x) ((x) << 13)
#define ITH_DMA_CFG_LLP_CNT(cfg)        (((cfg) >> 16) & 0xf)
#define ITH_DMA_CFG_GW(x)               (((x) & 0xff) << 20)
#define ITH_DMA_CFG_HIGH_PRIO           (1U << 28)
#define ITH_DMA_CFG_WO_MODE             (1U << 30)
#define ITH_DMA_CFG_UNALIGN_MODE        (1U << 31)

#define ITH_DMA_INT_MASK                0x7   // D[2]:abort, D[1]:error, D[0]:tc

/*
* Transfer size register
*/
#define ITH_DMA_CYC_MASK                0x3fffff
#define ITH_DMA_CYC_TOTAL(x)            ((x) & ITH_DMA_CYC_MASK)
#define ITH_DMA_CYC_2D(x, y)            (((x) & 0xffffU) | (((y) & 0xffffU) << 16))

/*
* Stride register
*/
#define ITH_DMA_STRIDE_SRC(x)           ((x) & 0xffff)
#define ITH_DMA_STRIDE_DST(x)           (((x) & 0xffff) << 16)

/*
* Hardware handshaking source select
*/
#define ITH_DMA_HW_SRC_SEL_S_REG(ch)      ((ch < 4) ? 0x60 : 0x64)
#define ITH_DMA_HW_SRC_SEL_D_REG(ch)      ((ch < 4) ? 0x68 : 0x6C)
#if (CFG_CHIP_FAMILY == 970)
#define ITH_DMA_HW_SRC_SEL_MSK(ch)        (0x1FU << ((ch % 4)*8))
#else
#define ITH_DMA_HW_SRC_SEL_MSK(ch)        (0x3FU << ((ch % 4)*8))
#endif
#define ITH_DMA_HW_SRC_SEL_VAL(ch, val)   ((val) << ((ch % 4)*8))

/* dma burst */
#define ITH_DMA_BURST_1     0U
#define ITH_DMA_BURST_2     1U
#define ITH_DMA_BURST_4     2U
#define ITH_DMA_BURST_8     3U
#define ITH_DMA_BURST_16    4U
#define ITH_DMA_BURST_32    5U
#define ITH_DMA_BURST_64    6U
#define ITH_DMA_BURST_128   7U

typedef uint8_t ITHDmaBurst;

/* dma priority */
#define ITH_DMA_CH_PRIO_LOW  0U
#define ITH_DMA_CH_PRIO_HIGH ITH_DMA_CFG_HIGH_PRIO

typedef uint32_t ITHDmaPriority;

/* dma width */
#define ITH_DMA_WIDTH_8     0U
#define ITH_DMA_WIDTH_16    1U
#define ITH_DMA_WIDTH_32    2U
#define ITH_DMA_WIDTH_64    3U

typedef uint8_t ITHDmaWidth;

/* dma address control */
#define ITH_DMA_CTRL_INC    0U
#define ITH_DMA_CTRL_FIX    2U

typedef uint8_t ITHDmaAddrCtl;

/* dma mode */
#define ITH_DMA_NORMAL_MODE         0U
#define ITH_DMA_HW_HANDSHAKE_MODE   1U

typedef uint8_t ITHDmaMode;

/* dma request */
#define ITH_DMA_MEM          9999U
#define ITH_DMA_IIC0_TX      0U
#define ITH_DMA_IIC0_RX      1U
#define ITH_DMA_IIC1_TX      2U
#define ITH_DMA_IIC1_RX      3U
#define ITH_DMA_IIC2_TX      4U
#define ITH_DMA_IIC2_RX      5U
#define ITH_DMA_IIC3_TX      6U
#define ITH_DMA_IIC3_RX      7U
#define ITH_DMA_IR_CAP0_RX   8U
#define ITH_DMA_IR_CAP0_TX   9U
#define ITH_DMA_AXISPI_TX_RX 10U
#define ITH_DMA_UART0_TX     11U
#define ITH_DMA_UART0_RX     12U
#define ITH_DMA_UART1_TX     13U
#define ITH_DMA_UART1_RX     14U
#define ITH_DMA_UART2_TX     15U
#define ITH_DMA_UART2_RX     16U
#define ITH_DMA_UART3_TX     17U
#define ITH_DMA_UART3_RX     18U
#define ITH_DMA_UART4_TX     19U
#define ITH_DMA_UART4_RX     20U
#define ITH_DMA_UART5_TX     21U
#define ITH_DMA_UART5_RX     22U
#define ITH_DMA_SPI0_TX      23U
#define ITH_DMA_SPI0_RX      24U
#define ITH_DMA_SPI1_TX      25U
#define ITH_DMA_SPI1_RX      26U
#define ITH_DMA_SPDIF_TIMECNT 27U
#define ITH_DMA_IR_CAP1_RX   28U
#define ITH_DMA_IR_CAP1_TX   29U
#define ITH_DMA_IR_CAP2_RX   30U
#define ITH_DMA_IR_CAP2_TX   31U
#define ITH_DMA_IR_CAP3_RX   32U
#define ITH_DMA_IR_CAP3_TX   33U

typedef uint32_t ITHDmaRequest;

#endif // ITH_DEFS_DMA_AXI_H
