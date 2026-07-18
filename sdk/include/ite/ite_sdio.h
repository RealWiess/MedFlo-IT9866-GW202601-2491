/** @file
 * ITE SDIO Driver API header file.
 *
 * @author Irene Lin
 * @copyright ITE Tech.Inc.All Rights Reserved.
 */

#ifndef ITE_SDIO_H
#define ITE_SDIO_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif


/** @defgroup driver_sdio SDIO
 *  @{
 */

#define SDIO_ANY_ID     UINT32_MAX

struct sdio_device_id {
    uint8_t	    class;			/* SDIO standard interface code or SDIO_ANY_ID */
    uint16_t	    vendor;			/* Vendor or SDIO_ANY_ID */
    uint16_t	    device;			/* Device ID or SDIO_ANY_ID */
    void*           driver_data;	/* private data to the driver */
};

struct sd_card;
struct sdio_func;

typedef void (*sdio_irq_handler)(struct sdio_func *);

struct sdio_func {
    struct sd_card      *card;
    sdio_irq_handler    irq_handler; /* IRQ callback */
    uint32_t    num;
    uint32_t    class;
    uint32_t    vendor;
    uint32_t    device;
    uint32_t    max_blksize;    /* maximum block size */
    uint32_t    cur_blksize;    /* current block size */
    uint32_t    enable_timeout;
    uint8_t     tmpbuf[4];
    void        *drv_data;
    struct sdio_driver *drv;
    uint32_t    type;
};

/*
 * SDIO function device driver
 */
struct sdio_driver {
    char *name;
    const struct sdio_device_id *id_table;

    int(*probe)(struct sdio_func *, const struct sdio_device_id *);
    void(*remove)(struct sdio_func *);
};

/**
 * Describe a specific SDIO device
 * @vend: the 16 bit manufacturer code
 * @dev: the 16 bit function id
 */
#define SDIO_DEVICE(vend,dev) \
	.class = (uint8_t)SDIO_ANY_ID, \
	.vendor = (vend), .device = (dev)

/**
 * Register a SDIO function driver.
 * @drv: the registerd function driver
 *
 * @return: 0 if success.
 */
int iteSdioRegisterDriver(struct sdio_driver *drv);

/**
 * Claim a bus for a set of operations.
 * @func: SDIO function that will be accessd
 */
void iteSdioClaimHost(struct sdio_func *func);

/**
 * Release a bus for a certain SDIO function.
 * @func: SDIO function that will be accessd
 */
void iteSdioReleaseHost(struct sdio_func *func);

/**
 *	Claim the IRQ for a SDIO function
 *	@func: SDIO function
 *	@handler: IRQ handler callback
 *
 *	Claim and activate the IRQ for the given SDIO function.
 */
int iteSdioClaimIrq(struct sdio_func *func, sdio_irq_handler handler);

/**
 *	Release the IRQ for a SDIO function
 *	@func: SDIO function
 *
 *	Disable and release the IRQ for the given SDIO function.
 */
int iteSdioReleaseIrq(struct sdio_func *func);

/**
 *	Enables a SDIO function
 *	@func: SDIO function to enable
 *
 *	Powers up and activates a SDIO function so that register access is possible.
 */
int iteSdioEnableFunc(struct sdio_func *func);

/**
 *	Disable a SDIO function
 *	@func: SDIO function to disable
 *
 *	Powers down and deactivates a SDIO function. Register access
 *	to this function will fail until the function is reenabled.
 */
int iteSdioDisableFunc(struct sdio_func *func);

/**
 * Set the block size of an SDIO function.
 * @func: SDIO function to change
 * @block_size: new block size
 */
int iteSdioSetBlockSize(struct sdio_func *func, uint32_t block_size);

/**
 *	Read a single byte from a SDIO function
 *	@func: SDIO function to access
 *	@addr: address to read
 *
 *	Reads a single byte from the address space of a given SDIO
 *	function. If there is a problem reading the address, 0xff
 *	is returned.
 */
uint8_t iteSdioRead8(struct sdio_func *func, uint32_t addr, int *err);

/**
 *	Write a single byte to a SDIO function
 *	@func: SDIO function to access
 *	@data: byte to write
 *	@addr: address to write to
 */
void iteSdioWrite8(struct sdio_func *func, uint8_t data, uint32_t addr, int *err);

/**
 *	Read a chunk of memory from a SDIO function
 *	@func: SDIO function to access
 *	@dst: buffer to store the data
 *	@addr: address to begin reading from
 *	@size: number of bytes to read
 *
 *	Return 0 if success.
 */
int iteSdioMemcpyFromIo(struct sdio_func *func, void *buf, uint32_t addr, int size);

/**
 *	Write a chunk of memory to a SDIO function
 *	@func: SDIO function to access
 *	@addr: address to start writing to
 *	@src: buffer that contains the data to write
 *	@size: number of bytes to write
 *
 *	Return 0 if success.
 */
int iteSdioMemcpyToIo(struct sdio_func *func, uint32_t addr, void *src, int size);

/**
*	Read a 16 bit data from a SDIO function
*	@func: SDIO function to access
*	@addr: address to read
*/
uint16_t iteSdioRead16(struct sdio_func *func, uint32_t addr, int *err);

/**
*	Write a 16 bit data to a SDIO function
*	@func: SDIO function to access
*	@data: data to write
*	@addr: address to write to
*/
void iteSdioWrite16(struct sdio_func *func, uint16_t data, uint32_t addr, int *err);

/**
*	Read a 32 bit integer from a SDIO function
*	@func: SDIO function to access
*	@addr: address to read
*	@err: optional status value
*/
uint32_t iteSdioRead32(struct sdio_func *func, uint32_t addr, int *err);

/**
 *	write a 32 bit integer to a SDIO function
 *	@func: SDIO function to access
 *	@data: data to write
 *	@addr: address to write to
 */
void iteSdioWrite32(struct sdio_func *func, uint32_t data, uint32_t addr, int *err);

/**
 *	Read a single byte from SDIO function 0
 *	@func: an SDIO function of the card
 *	@addr: address to read
 *
 *	Reads a single byte from the address space of SDIO function 0.
 */
uint8_t iteSdioF0Read8(struct sdio_func *func, uint32_t addr, int *err);

/**
 *	Write a single byte to SDIO function 0
 *	@func: an SDIO function of the card
 *	@data: byte to write
 *	@addr: address to write to
 *
 *	Writes a single byte to the address space of SDIO function 0.
 */
void iteSdioF0Write8(struct sdio_func *func, uint8_t data, uint32_t addr, int *err);

/**
 *	Read from a FIFO on a SDIO function
 *	@func: SDIO function to access
 *	@buf: buffer to store the data
 *	@addr: address of (single byte) FIFO
 *	@size: number of bytes to read
 *
 *	Reads from the specified FIFO of a given SDIO function.
 */
int iteSdioMemcpyFromFifo(struct sdio_func *func, void *buf, uint32_t addr, int size);

/**
 *	Write to a FIFO of a SDIO function
 *	@func: SDIO function to access
 *	@addr: address of (single byte) FIFO
 *	@src: buffer that contains the data to write
 *	@size: number of bytes to write
 *
 *	Writes to the specified FIFO of a given SDIO function.
 */
int iteSdioMemcpyToFifo(struct sdio_func *func, uint32_t addr, void *src, int size);


#define sdio_get_drvdata(f)	    (f->drv_data)
#define sdio_set_drvdata(f,d)	do { f->drv_data = d; } while (false)

#define sdio_register_driver        iteSdioRegisterDriver
#define sdio_claim_host             iteSdioClaimHost
#define sdio_release_host           iteSdioReleaseHost
#define sdio_claim_irq              iteSdioClaimIrq
#define sdio_release_irq            iteSdioReleaseIrq
#define sdio_enable_func            iteSdioEnableFunc
#define sdio_disable_func           iteSdioDisableFunc
#define sdio_set_block_size         iteSdioSetBlockSize
#define sdio_readb                  iteSdioRead8
#define sdio_writeb                 iteSdioWrite8
#define sdio_memcpy_fromio          iteSdioMemcpyFromIo
#define sdio_memcpy_toio            iteSdioMemcpyToIo
#define sdio_readw                  iteSdioRead16
#define sdio_writew                 iteSdioWrite16
#define sdio_readl                  iteSdioRead32
#define sdio_writel                 iteSdioWrite32
#define sdio_f0_readb               iteSdioF0Read8
#define sdio_f0_writeb              iteSdioF0Write8
#define sdio_readsb                 iteSdioMemcpyFromFifo
#define sdio_writesb                iteSdioMemcpyToFifo

/** @} */ // end of driver_sdio


#ifdef __cplusplus
}
#endif

#endif /* ITE_SDIO_H */
