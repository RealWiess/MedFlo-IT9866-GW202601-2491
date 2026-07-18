#ifndef	GMAC_CONFIG_H
#define	GMAC_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif


//=============================================================================
//                              Compile Option
//=============================================================================
//#define RECEIVE_BAD_RX_TO_UPLAYER
#if defined(CFG_NET_ETH_HW_VLAN)
    #define SUPPORT_MAC_VLAN
#endif

/**
 * for debug
 */
#define SHOW_HW_INFO    0
#define E_DBG       0
#define E_ERR       1
#define E_INFO      0
#define E_WARN      1

//=============================================================================
//                              Constant Definition
//=============================================================================


//=============================================================================
//                              LOG definition
//=============================================================================

#define LOG_MAC_ERR     1
#define LOG_MAC_INFO    1
#define LOG_MAC_WARN    0
#define LOG_MAC_ENTER   0
#define LOG_MAC_LEAVE   0

#if LOG_MAC_ERR
    #define mac_err(string, ...)                                       \
        do                                                                  \
        {                                                                   \
            ithPrintf("[mac][err]"); ithPrintf(string, ##__VA_ARGS__);  \
        } while (false)
#else
    #define mac_err(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#if LOG_MAC_INFO
    #define mac_info(string, ...)                                       \
        do                                                                  \
        {                                                                   \
            ithPrintf("[mac][info]"); ithPrintf(string, ##__VA_ARGS__);  \
        } while (false)
#else
    #define mac_info(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#if LOG_MAC_WARN
    #define mac_warn(string, ...)                                       \
        do                                                                  \
        {                                                                   \
            ithPrintf("[mac][warn]"); ithPrintf(string, ##__VA_ARGS__);  \
        } while (false)
#else
    #define mac_warn(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#if LOG_MAC_ENTER
    #define mac_enter(string, ...)                                       \
        do                                                                  \
        {                                                                   \
            ithPrintf("[mac][enter]"); ithPrintf(string, ##__VA_ARGS__);  \
        } while (false)
#else
    #define mac_enter(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif

#if LOG_MAC_LEAVE
    #define mac_leave(string, ...)                                       \
        do                                                                  \
        {                                                                   \
            ithPrintf("[mac][leave]"); ithPrintf(string, ##__VA_ARGS__);  \
        } while (false)
#else
    #define mac_leave(fmt, ...)                                              \
        do                                                                     \
        {                                                                      \
        } while (false)
#endif


#define check_result(rc) do { if (rc) mac_err("[%s] res = %d(0x%08X) \n", __FUNCTION__, rc, rc); } while (false)
#if 1
#define _mac_func_enter
#define _mac_func_leave
#else
#define _mac_func_enter			do{ mac_enter("%s \n", __FUNCTION__); } while (false)
#define _mac_func_leave			do{ mac_leave("%s \n", __FUNCTION__); } while (false)
#endif


#ifdef __cplusplus
}
#endif

#endif
