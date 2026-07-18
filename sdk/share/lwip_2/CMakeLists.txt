add_library(lwip_2 STATIC)

if (TARGET non_gpl_wifi_atbm)
    target_link_libraries(lwip_2 PRIVATE non_gpl_wifi_atbm)
endif()
if (TARGET non_gpl_wifi_8189f)
    target_link_libraries(lwip_2 PRIVATE non_gpl_wifi_8189f)
endif()
if (TARGET non_gpl_wifi_8723d)
    target_link_libraries(lwip_2 PRIVATE non_gpl_wifi_8723d)
endif()
if (TARGET non_gpl_wifi_8733b)
    target_link_libraries(lwip_2 PRIVATE non_gpl_wifi_8733b)
endif()
if (TARGET non_gpl_wifi_8821c)
    target_link_libraries(lwip_2 PRIVATE non_gpl_wifi_8821c)
endif()

if (DEFINED CFG_DBG_STATS_TCPIP_SNMP)
    target_link_libraries(lwip_2 PUBLIC mbedtls)
endif()

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    target_compile_definitions(lwip_2
    PRIVATE
        LWIP_DEBUG
    )
endif()

target_include_directories(lwip_2
PRIVATE
    $ENV{CFG_PLATFORM}
    ${PROJECT_SOURCE_DIR}/sdk/include/lwip
    ${PROJECT_SOURCE_DIR}/sdk/include/netif

    #lwip/apps
    ${PROJECT_SOURCE_DIR}/sdk/include/lwip/apps/snmp

    #lwip/compat
    #${PROJECT_SOURCE_DIR}/sdk/include/lwip/compat/posix
    #${PROJECT_SOURCE_DIR}/sdk/include/lwip/compat/posix/arpa
    #${PROJECT_SOURCE_DIR}/sdk/include/lwip/compat/posix/sys
    #${PROJECT_SOURCE_DIR}/sdk/include/lwip/compat/stdc/errno

    #lwip/priv
    ${PROJECT_SOURCE_DIR}/sdk/include/lwip/priv

    #lwip/prot
    ${PROJECT_SOURCE_DIR}/sdk/include/lwip/prot

    #netif/ppp
    ${PROJECT_SOURCE_DIR}/sdk/include/netif/ppp
    ${PROJECT_SOURCE_DIR}/sdk/include/netif/ppp/polarssl

if (DEFINED CFG_DBG_STATS_TCPIP_SNMP)
    ${PROJECT_SOURCE_DIR}/sdk/include/mbedtls/
endif()
)

target_sources(lwip_2
PRIVATE
    #api/
    api/api_lib.c
    api/api_msg.c
    api/err.c
    api/if_api.c
    api/netbuf.c
    api/netdb.c
    api/netifapi.c
    api/sockets.c
    api/tcpip.c

    #core/ipv4/
    core/ipv4/acd.c
    core/ipv4/autoip.c
    core/ipv4/dhcp.c
    core/ipv4/etharp.c
    core/ipv4/icmp.c
    core/ipv4/igmp.c
    core/ipv4/ip4.c
    core/ipv4/ip4_addr.c
    core/ipv4/ip4_frag.c

    #core/ipv6/
    core/ipv6/dhcp6.c
    core/ipv6/ethip6.c
    core/ipv6/icmp6.c
    core/ipv6/inet6.c
    core/ipv6/ip6.c
    core/ipv6/ip6_addr.c
    core/ipv6/ip6_frag.c
    core/ipv6/mld6.c
    core/ipv6/nd6.c

    #core/
    core/altcp.c
    core/altcp_alloc.c
    core/altcp_tcp.c
    core/def.c
    core/dns.c
    core/inet_chksum.c
    core/init.c
    core/ip.c
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
    core/timeouts.c
    core/udp.c

    #netif/
    netif/bridgeif.c
    netif/bridgeif_fdb.c
    netif/ethernet.c
    netif/slipif.c
    netif/zepif.c

    #netif/ppp
    netif/ppp/auth.c
    netif/ppp/ccp.c
    netif/ppp/chap_ms.c
    netif/ppp/chap-md5.c
    netif/ppp/chap-new.c
    netif/ppp/demand.c
    netif/ppp/eap.c
    netif/ppp/ecp.c
    netif/ppp/eui64.c
    netif/ppp/fsm.c
    netif/ppp/ipcp.c
    #netif/ppp/ipv6cp.c
    netif/ppp/lcp.c
    netif/ppp/magic.c
    netif/ppp/mppe.c
    netif/ppp/multilink.c
    netif/ppp/polarssl
    netif/ppp/ppp.c
    netif/ppp/pppapi.c
    netif/ppp/pppcrypt.c
    netif/ppp/pppoe.c
    #netif/ppp/pppol2tp.c
    #netif/ppp/pppos.c
    netif/ppp/upap.c
    netif/ppp/utils.c
    netif/ppp/vj.c

    #netif/ppp/polarssl
    netif/ppp/polarssl/arc4.c
    netif/ppp/polarssl/des.c
    netif/ppp/polarssl/md4.c
    netif/ppp/polarssl/md5.c
    netif/ppp/polarssl/sha1.c
)


if (DEFINED CFG_DBG_STATS_TCPIP_SNMP)
target_sources(lwip_2
PRIVATE
    apps/snmp/snmp_asn1.c
    apps/snmp/snmp_core.c
    apps/snmp/snmp_mib2.c
    apps/snmp/snmp_mib2_icmp.c
    apps/snmp/snmp_mib2_interfaces.c
    apps/snmp/snmp_mib2_ip.c
    apps/snmp/snmp_mib2_snmp.c
    apps/snmp/snmp_mib2_system.c
    apps/snmp/snmp_mib2_tcp.c
    apps/snmp/snmp_mib2_udp.c
    apps/snmp/snmp_msg.c
    apps/snmp/snmp_netconn.c
    apps/snmp/snmp_pbuf_stream.c
    apps/snmp/snmp_raw.c
    apps/snmp/snmp_scalar.c
    apps/snmp/snmp_snmpv2_framework.c
    apps/snmp/snmp_snmpv2_usm.c
    apps/snmp/snmp_table.c
    apps/snmp/snmp_threadsync.c
    apps/snmp/snmp_traps.c
    apps/snmp/snmpv3.c
    apps/snmp/snmpv3_mbedtls.c
    
    #apps/snmp/snmp_v3
    apps/snmp/snmp_v3/snmpv3_dummy.c
    
    #apps/snmp/snmp_private_mib
    apps/snmp/snmp_private_mib/lwip_prvmib.c
)
endif()

target_sources(lwip_2
PRIVATE
    $ENV{CFG_PLATFORM}/wifipktif.c
    $ENV{CFG_PLATFORM}/sys_arch.c
)

if ($ENV{CFG_PLATFORM} STREQUAL openrtos)

    if (DEFINED CFG_NET_ETHERNET AND NOT DEFINED CFG_NET_AMEBA_SDIO)
        if (DEFINED CFG_NET_ETH_HW_VLAN)
		        target_sources(lwip_2
		        PRIVATE
		            $ENV{CFG_PLATFORM}/pktif_vlan.c
		        )
        else()
		        target_sources(lwip_2
		        PRIVATE
		            $ENV{CFG_PLATFORM}/pktif.c
		        )
        endif()
    endif()

    if (DEFINED CFG_NET_AMEBA_SDIO)
        target_sources(lwip_2
        PRIVATE
            $ENV{CFG_PLATFORM}/ameba_sdio_if.c
        )
    endif()

    if (DEFINED CFG_USB_ECM OR DEFINED CFG_USB_ECM_EX)
        target_sources(lwip_2
        PRIVATE
            $ENV{CFG_PLATFORM}/ecmif.c
        )
    endif()

    if (DEFINED CFG_USBH_CD_CDCECM_HCC)
        target_sources(lwip_2
        PRIVATE
            $ENV{CFG_PLATFORM}/usbh_hccecmif.c
        )
    endif()

    if (DEFINED CFG_USBD_ECM)
        target_sources(lwip_2
        PRIVATE
            $ENV{CFG_PLATFORM}/usbd_ecmif.c
    )
    endif()

    if (DEFINED CFG_USBD_CD_CDCECM)
        target_sources(lwip_2
        PRIVATE
            $ENV{CFG_PLATFORM}/usbd_hccecmif.c
        )
    endif()
    
    if (DEFINED CFG_USBD_NCM)
        target_sources(lwip_2
        PRIVATE
            $ENV{CFG_PLATFORM}/usbd_ncmif.c
        )
    endif()

    if (DEFINED CFG_USBD_CD_CDCNCM)
        target_sources(lwip_2
        PRIVATE
            $ENV{CFG_PLATFORM}/usbd_hccncmif.c
        )
    endif()
	
    if (DEFINED CFG_USBH_CD_RNDIS)
        target_sources(lwip_2
        PRIVATE
            $ENV{CFG_PLATFORM}/usbh_rndisif.c
        )
    endif()
else()

    target_sources(lwip_2
    PRIVATE
        $ENV{CFG_PLATFORM}/pktdrv.c
        $ENV{CFG_PLATFORM}/pktif.c
    )

endif()
