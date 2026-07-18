/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   gl_dependency.h
 * \brief  List the os-dependent structure/API that need to implement
 * to align with common part
 *
 * not to modify *.c while put current used linux-type strucutre/API here
 * For porting to new OS, the API listed in this file needs to be
 * implemented
 */

#ifndef _GL_DEPENDENT_H
#define _GL_DEPENDENT_H

/* some arch's have a small-data section that can be accessed register-relative
 * but that can only take up to, say, 4-byte variables. jiffies being part of
 * an 8-byte variable may not be correctly accessed unless we force the issue
 * #define __jiffy_data  __attribute__((section(".data")))
 *
 * The 64-bit value is not atomic - you MUST NOT read it
 * without sampling the sequence number in jiffies_lock.

 * extern u64 __jiffy_data jiffies_64;
 * extern unsigned long volatile __jiffy_data jiffies;
 * TODO: no idea how to implement jiffies here
 */
#ifndef HZ
#define HZ (1000)
#endif
#define jiffies (0)

/*
 * defined in linux/time64.h
 * in cnm_timer.h
 * we do (?!), we may just add undef NSEC_PER_MSEC then define ours
 * #undef MSEC_PER_SEC
 * #define MSEC_PER_SEC            1000
 * #undef USEC_PER_MSEC
 * #define USEC_PER_MSEC           1000
 * #undef USEC_PER_SEC
 * #define USEC_PER_SEC            1000000
 */
#define NSEC_PER_MSEC	1000000L
#define NSEC_PER_USEC	1000L


#define IP4ADDR_STRLEN_MAX  16

/* needed by mgmt/scan.c */
int kal_test_bit(unsigned long bit, unsigned long *p);

#define unlikely(x) (x)

/* From MTK/dhcpd/inc/dhcpd.h */
/** @brief This structure defines the DHCPD configuration structure. For more information, please refer to #dhcpd_start() */
typedef struct
{
    char dhcpd_server_address[IP4ADDR_STRLEN_MAX];  /**< Specify server IP for AP. */
    char dhcpd_gateway[IP4ADDR_STRLEN_MAX];         /**< Specify gateway IP for AP. */
    char dhcpd_netmask[IP4ADDR_STRLEN_MAX];         /**< Specify netmask for AP. */
    char dhcpd_primary_dns[IP4ADDR_STRLEN_MAX];     /**< Specify primary DNS IP for AP. */
    char dhcpd_secondary_dns[IP4ADDR_STRLEN_MAX];   /**< Specify secondary DNS IP for AP. */
    char dhcpd_ip_pool_start[IP4ADDR_STRLEN_MAX];   /**< Specify starting IP for IP pool. */
    char dhcpd_ip_pool_end[IP4ADDR_STRLEN_MAX];     /**< Specify ending IP for IP pool. */
} dhcpd_settings_t;

#endif
