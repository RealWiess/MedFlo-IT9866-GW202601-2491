add_library(lwip)

if (TARGET non_gpl_wifi_8189f)
    target_link_libraries(lwip PRIVATE non_gpl_wifi_8189f)
endif()
if (TARGET non_gpl_wifi_8723d)
    target_link_libraries(lwip PRIVATE non_gpl_wifi_8723d)
endif()
if (TARGET non_gpl_wifi_8733b)
    target_link_libraries(lwip PRIVATE non_gpl_wifi_8733b)
endif()
if (TARGET non_gpl_wifi_8821c)
    target_link_libraries(lwip PRIVATE non_gpl_wifi_8821c)
endif()

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    target_compile_definitions(lwip
    PRIVATE
        LWIP_DEBUG
    )
endif()

if (DEFINED CFG_DLM_ENABLE)
    target_compile_options(lwip
    PRIVATE
        -fno-lto
    )
endif()

target_include_directories(lwip
PRIVATE
    $ENV{CFG_PLATFORM}
)

target_sources(lwip
PRIVATE
    api/api_lib.c
    api/api_msg.c
    api/err.c
    api/netbuf.c
    api/netdb.c
    api/netifapi.c
    api/sockets.c
    api/tcpip.c
    apps/lwiperf/lwiperf.c
    #apps/mqtt/mqtt.c
    #apps/mdns/mdns.c
    #apps/mdns/mdns_domain.c
    #apps/mdns/mdns_out.c
    #apps/netbiosns/netbiosns.c
    #apps/smtp/smtp.c
    core/ipv4/autoip.c
    core/ipv4/icmp.c
    core/ipv4/igmp.c
    core/ipv4/ip.c
    core/ipv4/inet.c
    core/ipv4/inet_chksum.c
    core/ipv4/ip_addr.c
    core/ipv4/ip_frag.c
    core/snmp/asn1_dec.c
    core/snmp/asn1_enc.c
    core/snmp/mib_structs.c
    core/snmp/mib2.c
    core/snmp/msg_in.c
    core/snmp/msg_out.c
    core/def.c
    core/dhcp.c
    core/dns.c
    core/init.c
    core/mem.c
    core/memp.c
    core/netif.c
    core/pbuf.c
    core/raw.c
    core/stats.c
    core/sys.c
    core/tcp.c
    core/tcp_in.c
    core/tcp_out.c
    core/timers.c
    core/udp.c
    netif/etharp.c
    netif/ethernetif.c
    netif/slipif.c
)

target_sources(lwip
PRIVATE
    $ENV{CFG_PLATFORM}/wifipktif.c
    $ENV{CFG_PLATFORM}/sys_arch.c
)

if ($ENV{CFG_PLATFORM} STREQUAL openrtos)

    if (DEFINED CFG_NET_ETHERNET AND NOT DEFINED CFG_NET_AMEBA_SDIO)
        target_sources(lwip
        PRIVATE
            $ENV{CFG_PLATFORM}/pktif.c
        )
    endif()

    if (DEFINED CFG_NET_AMEBA_SDIO)
        target_sources(lwip
        PRIVATE
            $ENV{CFG_PLATFORM}/ameba_sdio_if.c
        )
    endif()

    if (DEFINED CFG_USB_ECM OR DEFINED CFG_USB_ECM_EX)
        target_sources(lwip
        PRIVATE
            $ENV{CFG_PLATFORM}/ecmif.c
        )
    endif()

    if (DEFINED CFG_USBH_CD_CDCECM_HCC)
        target_sources(lwip
        PRIVATE
            $ENV{CFG_PLATFORM}/usbh_hccecmif.c
        )
    endif()
    
    if (DEFINED CFG_USBD_ECM)
        target_sources(lwip
        PRIVATE
            $ENV{CFG_PLATFORM}/usbd_ecmif.c
    )
    endif()

    if (DEFINED CFG_USBD_NCM)
        target_sources(lwip
        PRIVATE
            $ENV{CFG_PLATFORM}/usbd_ncmif.c
        )
    endif()

    if (DEFINED CFG_USBH_CD_RNDIS)
        target_sources(lwip
        PRIVATE
            $ENV{CFG_PLATFORM}/usbh_rndisif.c
        )
    endif()
else()

    target_sources(lwip
    PRIVATE
        $ENV{CFG_PLATFORM}/pktdrv.c
        $ENV{CFG_PLATFORM}/pktif.c
    )

endif()
