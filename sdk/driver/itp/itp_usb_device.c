
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/ioctl.h>
#include "itp_cfg.h"
#include "ite/itp.h"
#include "ite/ite_acm.h"
#include "ite/ite_usbex.h"

static bool connected = false;

#if defined(CFG_USB_DEVICE_DISKS)
static const int diskTable[] = { CFG_USB_DEVICE_DISKS , -1 };
#endif


extern int iteUsbRegisterGadget(void);
extern int iteUsbUnRegisterGadget(void);

#if defined(CFG_USBD_HID)
extern pthread_mutex_t gUsbdHidMutex;
#endif
static void heapStatHandler(void)
{
#if 0
extern uint8_t __heap_start__[];
extern uint8_t __heap_end__[];

    struct mallinfo mi = mallinfo();
    unsigned long total = (unsigned int)__heap_end__ - (unsigned int)__heap_start__;

    // heap usage
    printf("HEAP newlib: usage=%d/%d(%d%%),addr=0x%X\n",
        mi.uordblks,
        total,
        (int)(100.0f * mi.uordblks / total),
        __heap_start__);
#endif

#ifdef CFG_DBG_RMALLOC
    extern void  Rmalloc_stat(const char *file);
    Rmalloc_stat(__FILE__);
#endif

}

#if defined(CFG_DBG_USB)
static void OriStandardIo(void)
{
#if defined(CFG_DBG_PRINTBUF)
	// init print buffer device
	itpRegisterDevice(ITP_DEVICE_STD, &itpDevicePrintBuf);
	ioctl(ITP_DEVICE_PRINTBUF, ITP_IOCTL_INIT, NULL);
#elif defined(CFG_DBG_UART0)
	// init uart device
	itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceUart0);
	ioctl(ITP_DEVICE_UART0, ITP_IOCTL_INIT, NULL);
#elif defined(CFG_DBG_UART1)
	itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceUart1);
	ioctl(ITP_DEVICE_UART1, ITP_IOCTL_INIT, NULL);
#elif defined(CFG_DBG_UART2)
	itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceUart2);
	ioctl(ITP_DEVICE_UART2, ITP_IOCTL_INIT, NULL);
#elif defined(CFG_DBG_UART3)
	itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceUart3);
	ioctl(ITP_DEVICE_UART3, ITP_IOCTL_INIT, NULL);
#elif defined(CFG_DBG_UART4)
	itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceUart4);
	ioctl(ITP_DEVICE_UART4, ITP_IOCTL_INIT, NULL);
#elif defined(CFG_DBG_UART5)
	itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceUart5);
	ioctl(ITP_DEVICE_UART5, ITP_IOCTL_INIT, NULL);
#elif defined(CFG_DBG_SWUART_CODEC)
	itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceSwUartCodecDbg);
	ioctl(ITP_DEVICE_SWUARTDBG, ITP_IOCTL_INIT, NULL);
#endif // defined(CFG_DBG_PRINTBUF)
}
#endif

static void* UsbDeviceDetectHandler(void* arg)
{
#if defined(CFG_DBG_USB)
    int acm_dbg = 0;
#endif

    while (1) {
        #if defined(CFG_USBD_HID)
        pthread_mutex_lock(&gUsbdHidMutex);
        #endif
        if (iteOtgIsDeviceMode() == true) {
            if (connected == false) {
				heapStatHandler();
				printf(" Enter Device Mode ~~~~~~~~~~~~~~~ \n");

                #if defined(CFG_USBD_MASS_STORAGE)
                #if defined(CFG_FS_FAT)
                ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_DISABLE, NULL);
                ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_UNMOUNT, (void*)diskTable);
                #endif
                #endif // #if defined(CFG_USBD_MASS_STORAGE)

                iteUsbRegisterGadget();

                #if defined(CFG_USBD_MASS_STORAGE)
                ioctl(ITP_DEVICE_USBDFSG, ITP_IOCTL_ENABLE, NULL);
                #endif

                #if defined(CFG_USBD_ACM)
                ioctl(ITP_DEVICE_USBDACM, ITP_IOCTL_ENABLE, NULL);
                #endif // #if defined(CFG_USBD_ACM)

                #if defined(CFG_USBD_ECM)
                while (!ioctl(ITP_DEVICE_USBDECM, ITP_IOCTL_IS_CONNECTED, (void*)1)) {
                    sleep(1);
                    putchar('$');
                    fflush(stdout);
                }

                ioctl(ITP_DEVICE_USBDECM, ITP_IOCTL_NETIF_ADD, NULL);
                #endif // #if defined(CFG_USBD_ECM)

                #if defined(CFG_USBD_NCM)
                while (!ioctl(ITP_DEVICE_USBDNCM, ITP_IOCTL_IS_CONNECTED, (void *)1))
                {
                    sleep(1);
                    putchar('$');
                    fflush(stdout);
                }

                ioctl(ITP_DEVICE_USBDNCM, ITP_IOCTL_NETIF_ADD, NULL);
                #endif // #if defined(CFG_USBD_NCM)

                #if defined(CFG_USBD_HID)
                ioctl(ITP_DEVICE_USBDHID, ITP_IOCTL_ENABLE, NULL);
                #endif

                connected = true;
            }
        }
        else {
            if (connected == true) {
				#if defined(CFG_DBG_USB)
				if (acm_dbg == 1) {
					OriStandardIo();
					printf("\n original STD I/O.... \n");
					acm_dbg = 0;
				}
				#endif

				printf(" Leave Device Mode ~~~~~~~~~~~~~~~ \n");

				#if defined(CFG_USBD_ACM)
                ioctl(ITP_DEVICE_USBDACM, ITP_IOCTL_DISABLE, NULL);
				#endif // #if defined(CFG_USBD_ACM)

                #if defined(CFG_USBD_ECM)
                ioctl(ITP_DEVICE_USBDECM, ITP_IOCTL_NETIF_REMOVE, NULL); /* netif down & remove & call ncm_close */
                #endif // #if defined(CFG_USBD_ECM)

                #if defined(CFG_USBD_NCM)
                ioctl(ITP_DEVICE_USBDNCM, ITP_IOCTL_NETIF_REMOVE, NULL); /* netif down & remove & call ncm_close */
                #endif // #if defined(CFG_USBD_NCM)

                #if defined(CFG_USBD_HID)
                ioctl(ITP_DEVICE_USBDHID, ITP_IOCTL_DISABLE, NULL);
                #endif

				iteUsbUnRegisterGadget();

                #if defined(CFG_USBD_MASS_STORAGE)
                ioctl(ITP_DEVICE_USBDFSG, ITP_IOCTL_DISABLE, NULL);
                #if defined(CFG_FS_FAT)
                ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_MOUNT, (void*)diskTable);
                ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_ENABLE, NULL);
                #endif // #if defined(CFG_FS_FAT)
                #endif // #if defined(CFG_USBD_MASS_STORAGE)
                connected = false;

				heapStatHandler();
            }
        }
        #if defined(CFG_USBD_HID)
	    pthread_mutex_unlock(&gUsbdHidMutex);
        #endif

        usleep(30 * 1000);
		#if defined(CFG_DBG_USB)
		if (acm_dbg == 0) {
			if (ioctl(ITP_DEVICE_USBDACM, ITP_IOCTL_IS_CONNECTED, NULL)) {
				printf("\n use usb console.... \n");
				itpRegisterDevice(ITP_DEVICE_STD, &itpDeviceUsbdConsole);
				itpRegisterDevice(ITP_DEVICE_USBDCONSOLE, &itpDeviceUsbdConsole);
                ioctl(ITP_DEVICE_USBDCONSOLE, ITP_IOCTL_INIT, NULL);
				acm_dbg = 1;
			}
		}
		if (acm_dbg == 1) {
			if (ioctl(ITP_DEVICE_USBDACM, ITP_IOCTL_IS_CONNECTED, NULL) == 0) {
				OriStandardIo();
				printf("\n back to print buffer.... \n");
				acm_dbg = 0;
			}
		}
		#endif
    }
}


static int UsbdInit(void)
{
    int res;
	pthread_t task;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	res = pthread_create(&task, &attr, UsbDeviceDetectHandler, NULL);
	if (res)
		ITP_LOG_ERR(" create usb device detect thread fail! 0x%08X \n", res );

	return res;
}

static int UsbdIoctl(int file, unsigned long request, void* ptr, void* info)
{
    int res;

    switch (request)
    {
    case ITP_IOCTL_INIT:
        res = UsbdInit();
        if(res)
        {
            errno = (ITP_DEVICE_USBD << ITP_DEVICE_ERRNO_BIT) | res;
            return -1;
        }
        break;

    default:
        errno = (ITP_DEVICE_USBD << ITP_DEVICE_ERRNO_BIT) | 1;
        return -1;
    }
    return 0;
}

const ITPDevice itpDeviceUsbd =
{
    ":usbd",
    itpOpenDefault,
    itpCloseDefault,
    itpReadDefault,
    itpWriteDefault,
    itpLseekDefault,
    UsbdIoctl,
    NULL
};
