if (DEFINED CFG_NET_ETHERNET OR DEFINED CFG_NET_ETHERNET_4G)
    add_library(eth_mgr STATIC)

    target_include_directories(eth_mgr
    PRIVATE
        ${PROJECT_SOURCE_DIR}/sdk/share/eth_mgr/
        ${PROJECT_SOURCE_DIR}/sdk/share/eth_mgr/ecm_dial
    )

    target_sources(eth_mgr
    PRIVATE
        ecmMgr.c
    )

    if (DEFINED CFG_NET_N720_4G)
        target_sources(eth_mgr
        PRIVATE
            ecm_dial/dial_N720.c
        )
    elseif (DEFINED CFG_NET_L718_4G)
        target_sources(eth_mgr
        PRIVATE
            ecm_dial/dial_L718.c
        )
    elseif (DEFINED CFG_NET_L610_4G)
        target_sources(eth_mgr
        PRIVATE
            ecm_dial/dial_L610.c
        )
    elseif (DEFINED CFG_NET_AIR720SL_4G)
        target_sources(eth_mgr
        PRIVATE
            ecm_dial/dial_Air720SL.c
        )
    elseif (DEFINED CFG_NET_M8910DL_4G)
        target_sources(eth_mgr
        PRIVATE
            ecm_dial/dial_M8910DL.c
        )
    elseif (DEFINED CFG_NET_N58_4G)
        target_sources(eth_mgr
        PRIVATE
            ecm_dial/dial_N58.c
        )
    endif()

endif()
