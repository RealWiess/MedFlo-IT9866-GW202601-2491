/* OPTIONAL: Pools to replace heap allocation
 * Optional: Pools can be used instead of the heap for mem_malloc. If
 * so, these should be defined here, in increasing order according to
 * the pool element size.
 *
 * LWIP_MALLOC_MEMPOOL(number_elements, element_size)
 */
#if MEM_USE_POOLS
LWIP_MALLOC_MEMPOOL_START
LWIP_MALLOC_MEMPOOL(32, 128)
LWIP_MALLOC_MEMPOOL(32, 256) //100
LWIP_MALLOC_MEMPOOL(50, 512)
LWIP_MALLOC_MEMPOOL(20, 1024)
#if defined (CFG_IPTV_TX)
LWIP_MALLOC_MEMPOOL(2560, 1536)
#elif defined(CFG_NET_LWIP_OPT_MEM_HIGH)
    #if defined(CFG_NET_WIFI_SDIO_VND_AIC)
        LWIP_MALLOC_MEMPOOL(400, 1536) //200
    #else
        LWIP_MALLOC_MEMPOOL(320, 1536) //200
    #endif
#else
LWIP_MALLOC_MEMPOOL(32, 1536) //200
#endif
#if defined (CFG_NET_WIFI_SDIO_VND_MTK)
LWIP_MALLOC_MEMPOOL(360, 2536)
#endif
LWIP_MALLOC_MEMPOOL_END
#endif /* MEM_USE_POOLS */

/* Optional: Your custom pools can go here if you would like to use
 * lwIP's memory pools for anything else.
 */
LWIP_MEMPOOL(SYS_MBOX, 22, 100, "SYS_MBOX")
