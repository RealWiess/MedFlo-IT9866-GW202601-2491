add_library(dns_sd)
target_compile_definitions(dns_sd
PRIVATE
    HAVE_IPV6=0
    NOT_HAVE_IF_NAMETOINDEX
    NOT_HAVE_SA_LEN
    PLATFORM_NO_RLIMIT
    USE_TCP_LOOPBACK
    _PLATFORM_HAS_STRONG_PRNG_=0
)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    target_compile_definitions(dns_sd
    PRIVATE
        MDNS_DEBUGMSGS=2
    )
else()
    target_compile_definitions(dns_sd
    PRIVATE
        MDNS_DEBUGMSGS=1
    )
endif()

if (DEFINED CFG_GCC_LTO)
    target_compile_options(dns_sd PRIVATE "-fno-lto")
endif ()

target_include_directories(dns_sd
PRIVATE
    mDNSCore
    mDNSShared
    mDNSPosix
)

target_sources(dns_sd
PRIVATE
    #Clients/ClientCommon.c
    #Clients/ClientCommon.h
    #Clients/dns-sd.c
    mDNSCore/DNSCommon.c
    mDNSCore/DNSCommon.h
    mDNSCore/DNSDigest.c
    mDNSCore/mDNS.c
    mDNSCore/mDNSDebug.h
    mDNSCore/mDNSEmbeddedAPI.h
    mDNSCore/uDNS.c
    mDNSCore/uDNS.h
    mDNSPosix/mDNSPosix.c
    mDNSPosix/mDNSPosix.h
    mDNSPosix/mDNSUNP.c
    mDNSPosix/mDNSUNP.h
    mDNSShared/CommonServices.h
    mDNSShared/DebugServices.c
    mDNSShared/DebugServices.h
    mDNSShared/GenLinkedList.c
    mDNSShared/GenLinkedList.h
    mDNSShared/mDNSDebug.c
    mDNSShared/PlatformCommon.c
    mDNSShared/PlatformCommon.h
)

if (DEFINED CFG_DNS_SD_RESPONDER)
    target_sources(dns_sd
    PRIVATE
        mDNSPosix/Responder.c
    )
elseif (DEFINED CFG_DNS_SD_DAEMON)
    target_sources(dns_sd
    PRIVATE
        mDNSShared/dnssd_clientlib.c
        mDNSShared/dnssd_clientstub.c
        mDNSShared/dnssd_ipc.c
        mDNSShared/dnssd_ipc.h
        mDNSShared/dns_sd.h
        mDNSShared/uds_daemon.c
        mDNSShared/uds_daemon.h
        mDNSPosix/PosixDaemon.c
    )
endif()
