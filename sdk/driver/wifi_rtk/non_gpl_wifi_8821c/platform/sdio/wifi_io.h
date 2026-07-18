#ifndef _WIFI_IO_H
#define _WIFI_IO_H

#include "core/sdio_irq.h"
#include <pthread.h>

/*struct sdio_driver {
	int fake;
};*/
enum _CHIP_TYPE {

	NULL_CHIP_TYPE,
	RTL8188C_8192C,
	RTL8192D,
	RTL8723A,
	RTL8188E,
	RTL8189F,
	RTL8192E,
	RTL8812,
	RTL8821, //RTL8811
	RTL8723B,
	MAX_CHIP_TYPE
};

 /*
  * type related
  */
typedef unsigned char      u8;
typedef unsigned char      __u8;
typedef unsigned short      u16;
typedef unsigned short      __u16;
typedef unsigned short      __le16;
typedef unsigned short      __be16;
typedef unsigned int      u32;
typedef unsigned int      __u32;
typedef unsigned int      __le32;
typedef unsigned int      __be32;
typedef signed char         s8;
typedef signed short      s16;
typedef signed int         s32;
typedef signed long long    s64;
typedef unsigned long long   u64;
typedef unsigned int      uint;
typedef signed int         sint;
int rtw_printf(const char *format, ...);
#define printk            rtw_printf

#ifndef BIT
#define BIT(_x)	(1 << (_x))
#endif
int wifi_read(struct sdio_func *func, u32 addr, u32 cnt, void *pdata);
int wifi_write(struct sdio_func *func, u32 addr, u32 cnt, void *pdata);
u8 wifi_readb(struct sdio_func *func, u32 addr);
u16 wifi_readw(struct sdio_func *func, u32 addr);
u32 wifi_readl(struct sdio_func *func, u32 addr);
void wifi_writeb(struct sdio_func *func, u32 addr, u8 val);
void wifi_writew(struct sdio_func *func, u32 addr, u16 v);
void wifi_writel(struct sdio_func *func, u32 addr, u32 v);
u8 wifi_readb_local(struct sdio_func *func, u32 addr);
void wifi_writeb_local(struct sdio_func *func, u32 addr, u8 val);

//typedef unsigned long UINT32;
typedef struct _SDIO_BUS_OPS{
	int (*bus_probe)(void);
	int (*bus_remove)(void);
	int (*enable)(struct sdio_func*func);	/*enables a SDIO function for usage*/
	int (*disable)(struct sdio_func *func);
	int (*reg_driver)(struct sdio_driver*); /*register sdio function device driver callback*/
	void (*unreg_driver)(struct sdio_driver *);
	int (*claim_irq)(struct sdio_func *func, void(*handler)(struct sdio_func *));
	int (*release_irq)(struct sdio_func*func);
	void (*claim_host)(struct sdio_func*func);	/*exclusively claim a bus for a certain SDIO function*/
	void (*release_host)(struct sdio_func *func);
	uint8_t (*readb)(struct sdio_func *func, uint32_t addr, int *err_ret);/*read a single byte from a SDIO function*/
	uint16_t (*readw)(struct sdio_func *func, uint32_t addr, int *err_ret);	/*read a 16 bit integer from a SDIO function*/
	uint32_t (*readl)(struct sdio_func *func, uint32_t addr, int *err_ret); /*read a 32 bit integer from a SDIO function*/
	void (*writeb)(struct sdio_func *func, uint8_t b,uint32_t addr, int *err_ret);	/*write a single byte to a SDIO function*/
	void (*writew)(struct sdio_func *func, uint16_t b,uint32_t addr, int *err_ret);	/*write a 16 bit integer to a SDIO function*/
	void (*writel)(struct sdio_func *func, uint32_t b,uint32_t addr, int *err_ret); /*write a 32 bit integer to a SDIO function*/
	int (*memcpy_fromio)(struct sdio_func *func, void *dst,uint32_t addr, int count);/*read a chunk of memory from a SDIO functio*/
	int (*memcpy_toio)(struct sdio_func *func, uint32_t addr,void *src, int count);  /*write a chunk of memory to a SDIO function*/
}SDIO_BUS_OPS;
extern SDIO_BUS_OPS rtw_sdio_bus_ops;

extern struct sdio_func *wifi_sdio_func;
//extern int sdio_bus_probe(void);
//extern int sdio_bus_remove(void);
#endif

