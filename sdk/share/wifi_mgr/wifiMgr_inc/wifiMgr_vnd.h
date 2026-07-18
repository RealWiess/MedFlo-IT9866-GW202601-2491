#ifndef WIFIMGR_VND_H
#define WIFIMGR_VND_H

#if defined(CFG_NET_WIFI_SDIO_VND_RTK)
#define _WIFIMGR_API_RTK_
#endif

#if defined(CFG_NET_WIFI_SDIO_VND_MHD)
#define _WIFIMGR_API_BCM_
#endif

#if defined(CFG_NET_WIFI_SDIO_NGPL_ATBM6031)
#define _WIFIMGR_API_ATBM_
#endif

#if defined(CFG_NET_WIFI_SDIO_VND_AIC)
#define _WIFIMGR_API_AIC_
#endif

#if defined(CFG_NET_WIFI_SDIO_VND_MTK)
#define _WIFIMGR_API_MTK_
#endif

#endif
