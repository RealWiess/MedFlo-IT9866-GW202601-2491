//#include "include/common.h"
#if defined(CFG_MMC_ENABLE)
#include "ite/ite_sdio.h"
#else
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio.h>
#endif
#include "wifi_io.h"
#include "wifi_constants.h"
#include "rtwlan_bsp.h"
#include "rtwlan_config.h"
#include "openrtos/portmacro.h"

#define BUILD_SDIO

struct sdio_func *wifi_sdio_func = NULL;

static const struct sdio_device_id sdio_ids[] = {
	{ SDIO_DEVICE(0x024C, 0xB733),.driver_data = (void *)RTL8733B}
};

/*Declaration*/
static int drv_init(struct sdio_func *func, const struct sdio_device_id *id);
static void dev_remove(struct sdio_func *func);
static void wifi_fake_driver_probe_rtlwifi(struct sdio_func *func);
static int wifi_fake_driver_probe_8189es(struct sdio_func *func);

/* Extern*/
extern void cmd_wifi_scan(int argc, char **argv);
extern int wifi_on(rtw_mode_t mode);
extern int wifi_off(void);
extern void vTaskDelay(const TickType_t xTicksToDelay );

struct sdio_drv_priv {
	struct sdio_driver wifi_drv;
	int drv_registered;
};

static struct sdio_drv_priv sdio_drvpriv = {
	.wifi_drv.probe = drv_init,
	.wifi_drv.remove = dev_remove,
	.wifi_drv.name = (char*)DRV_NAME,
	.wifi_drv.id_table = sdio_ids,
};

static int drv_init(struct sdio_func *func, const struct sdio_device_id *id)
{
    printk("====>drv_init: hook sdio_func!! vendor=0x%04x device=0x%04x class=0x%02x\n",
    	func->vendor, func->device, func->class);
    wifi_sdio_func = NULL;
    wifi_sdio_func = func;
    return 0;
}

static void dev_remove(struct sdio_func *func)
{
    printk("====>dev_remove()\n");
    return;
}

int mmpRtlWifiDriverRegister(void)
{
	int ret;

	printk("====>Do mmpRtlWifiDriverRegister....\n");
	ret = sdio_register_driver(&sdio_drvpriv.wifi_drv);

	return ret;
}

int mmpRtlWifiDriverCmdTest(void)
{
    int nRet = -1, rc;
    int Default_Mode = 0, try_count = 0;

    /* Enable function & Get IO ready */
    sdio_claim_host(wifi_sdio_func);
    rc = sdio_enable_func(wifi_sdio_func);
    sdio_release_host(wifi_sdio_func);

    printk("====>[mmpRtlWifiDriver][SDIO %s] We call entry function or CMD test there.... \n", (rc == 0) ? "enabled":"disabled");

    if (rc != 0)
        return RTW_NODEVICE;

    //wifi_fake_driver_probe_rtlwifi(wifi_sdio_func);
    usleep(100*1000);

retry_init:

    if (Default_Mode == 0){
        nRet = wifi_on(RTW_MODE_STA);
    } else if (Default_Mode ==1){
        vTaskDelay(20);
        nRet = wifi_on(RTW_MODE_AP);
    } else if (Default_Mode ==2){
        wifi_off();
        vTaskDelay(20);
        nRet = wifi_on(RTW_MODE_STA_AP);
    } else {
        printk("Select Unknown mode before calling wifi_on\n");
    }

    printk("[mmpRtlWifiDriver] wifi on: %s\n", (nRet == RTW_SUCCESS) ? "success":"failed");

    if (nRet < 0 && try_count < ITE_WIFI_ON_RETRY) {
        printk("\n====>Oops!! wifi_on was failed, try init again\n\n");
        try_count++;

        wifi_off();
        usleep(100*1000);

        goto retry_init;
    } else if (try_count >= ITE_WIFI_ON_RETRY) {
        printk("====Warning!! wifi_on was failed over %d times\n", try_count);
    }

    while(nRet < 0){
        printk("====>wait sdio wifi driver ready...\n");
        usleep(100*1000);
    }
    //start_interactive_mode();

    //cmd_wifi_scan(0, NULL);

    //cmd_wifi_info(0,NULL);

    // wifi_off();

    return nRet;
}

void testBuildFail()
{
    Set_WLAN_Power_On();
    //netif_post_sleep_processing();
	//wifi_on(1);
}
#ifdef BUILD_SDIO

/* test wifi driver */
#define ADDR_MASK 0x10000
#define LOCAL_ADDR_MASK 0x00000
#ifndef BIT
#define BIT(_x)	(1 << (_x))
#endif

int wifi_read(struct sdio_func *func, u32 addr, u32 cnt, void *pdata)
{
	int err;

	sdio_claim_host(func);

	err = sdio_memcpy_fromio(func, pdata, addr, cnt);
	if (err) {
		printk("%s: FAIL(%d)! ADDR=%#x Size=%d\n", __func__, err, addr, cnt);
	}

	sdio_release_host(func);

	return err;
}

int wifi_write(struct sdio_func *func, u32 addr, u32 cnt, void *pdata)
{
	int err;
	u32 size;

	sdio_claim_host(func);

	size = cnt;
	err = sdio_memcpy_toio(func, addr, pdata, size);
	if (err) {
		printk("%s: FAIL(%d)! ADDR=%#x Size=%d(%d)\n", __func__, err, addr, cnt, size);
	}

	sdio_release_host(func);

	return err;
}

u8 wifi_readb(struct sdio_func *func, u32 addr)
{
	int err;
	u8 ret = 0;

	sdio_claim_host(func);
	ret = sdio_readb(func, ADDR_MASK | addr, &err);
	sdio_release_host(func);

	if (err)
		printk("%s: FAIL!(%d) addr=0x%05x\n", __func__, err, addr);

	return ret;
}

u16 wifi_readw(struct sdio_func *func, u32 addr)
{
	int err;
	u16 v;

	sdio_claim_host(func);
	v = sdio_readw(func, ADDR_MASK | addr, &err);
	sdio_release_host(func);
	if (err)
		printk("%s: FAIL!(%d) addr=0x%05x\n", __func__, err, addr);

	return  v;
}

u32 wifi_readl(struct sdio_func *func, u32 addr)
{
	int err;
	u32 v;

	sdio_claim_host(func);
	v = sdio_readl(func, ADDR_MASK | addr, &err);
	sdio_release_host(func);

	return  v;
}

void wifi_writeb(struct sdio_func *func, u32 addr, u8 val)
{
	int err;

	sdio_claim_host(func);
	sdio_writeb(func, val, ADDR_MASK | addr, &err);
	sdio_release_host(func);
	if (err)
		printk("%s: FAIL!(%d) addr=0x%05x val=0x%02x\n", __func__, err, addr, val);
}

void wifi_writew(struct sdio_func *func, u32 addr, u16 v)
{
	int err;

	sdio_claim_host(func);
	sdio_writew(func, v, ADDR_MASK | addr, &err);
	sdio_release_host(func);
	if (err)
		printk("%s: FAIL!(%d) addr=0x%05x val=0x%04x\n", __func__, err, addr, v);
}

void wifi_writel(struct sdio_func *func, u32 addr, u32 v)
{
	int err;

	sdio_claim_host(func);
	sdio_writel(func, v, ADDR_MASK | addr, &err);
	sdio_release_host(func);
}

u8 wifi_readb_local(struct sdio_func *func, u32 addr)
{
	int err;
	u8 ret = 0;
    sdio_claim_host(func);
	ret = sdio_readb(func, LOCAL_ADDR_MASK | addr, &err);
    sdio_release_host(func);

	return ret;
}

void wifi_writeb_local(struct sdio_func *func, u32 addr, u8 val)
{
	int err;
    sdio_claim_host(func);
	sdio_writeb(func, val, LOCAL_ADDR_MASK | addr, &err);
    sdio_release_host(func);
}
extern int rtw_fake_driver_probe(struct sdio_func *func);
void wifi_fake_driver_probe_rtlwifi(struct sdio_func *func)
{
	rtw_fake_driver_probe(func);//todo1
}

//extern int sdio_bus_probe(void);
//extern int sdio_bus_remove(void);
extern int sdio_register_driver(struct sdio_driver *);
SDIO_BUS_OPS rtw_sdio_bus_ops = {
	//sdio_bus_probe,
	//sdio_bus_remove,
	NULL,
	NULL,
	sdio_enable_func,
	sdio_disable_func,
	sdio_register_driver,
	NULL,
	sdio_claim_irq,
	sdio_release_irq,
	sdio_claim_host,
	sdio_release_host,
	sdio_readb,
	sdio_readw,
	sdio_readl,
	sdio_writeb,
	sdio_writew,
	sdio_writel,
	sdio_memcpy_fromio,
	sdio_memcpy_toio
};
#endif
