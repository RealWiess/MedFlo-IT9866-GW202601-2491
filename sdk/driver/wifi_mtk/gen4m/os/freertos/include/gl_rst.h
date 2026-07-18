/*******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
/*
 * Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/os/linux/include
 *     /gl_rst.h#1
 */

/*! \file   gl_rst.h
 *    \brief  Declaration of functions and finite state machine for
 *	    MT6620 Whole-Chip Reset Mechanism
 */

#ifndef _GL_RST_H
#define _GL_RST_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "gl_typedef.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
/* If there's any modification in these macros, then sync to apcAction[] in
 * glResetTriggerCommon().
 */
#define RST_FLAG_DO_CORE_DUMP              BIT(0)
#define RST_FLAG_PREVENT_POWER_OFF         BIT(1)
#define RST_FLAG_DO_WHOLE_RESET            BIT(2)
#define RST_FLAG_DO_L0P5_RESET             BIT(3)
#define RST_FLAG_DO_L1_RESET               BIT(4)

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
enum ENUM_RESET_STATUS {
	RESET_FAIL,
	RESET_SUCCESS
};

/* If there's any modification in this enum, then sync to apcReason[] in
 * glResetTriggerCommon().
 */
enum _ENUM_CHIP_RESET_REASON_TYPE_T {
	RST_UNKNOWN = 0,
	RST_PROCESS_ABNORMAL_INT,
	RST_DRV_OWN_FAIL,
	RST_FW_ASSERT_DONE,
	RST_FW_ASSERT_TIMEOUT,
	RST_BT_TRIGGER,
	RST_OID_TIMEOUT,
	RST_CMD_TRIGGER,
	RST_CR_ACCESS_FAIL,
	RST_CMD_EVT_FAIL,
	RST_GROUP4_NULL,
	RST_TX_ERROR,
	RST_RX_ERROR,
	RST_WDT,
	RST_SER_L1_FAIL,    /* TODO */
	RST_SER_L0P5_FAIL,  /* TODO */
	RST_REASON_MAX
};

/* Copy from os/linux/include/gl_rst.h */
/* L0.5 reset state */
enum ENUM_WFSYS_RESET_STATE_TYPE_T {
	WFSYS_RESET_STATE_IDLE = 0,
	WFSYS_RESET_STATE_DETECT,
	WFSYS_RESET_STATE_RESET,
	WFSYS_RESET_STATE_REINIT,
	WFSYS_RESET_STATE_POSTPONE,
	WFSYS_RESET_STATE_MAX
};

enum ENUM_WMTRSTMSG_TYPE {
	WMTRSTMSG_RESET_START = 0x0,  /*whole chip reset (include other radio)*/
	WMTRSTMSG_RESET_END = 0x1,
	WMTRSTMSG_RESET_END_FAIL = 0x2,
	WMTRSTMSG_0P5RESET_START = 0x3, /*wfsys reset ( wifi only )*/
	WMTRSTMSG_RESET_MAX,
	WMTRSTMSG_RESET_INVALID = 0xff
};

/*******************************************************************************
 *                    E X T E R N A L   F U N C T I O N S
 *******************************************************************************
 */
u_int8_t kalIsResetting(void);
void glSetRstReason(enum _ENUM_CHIP_RESET_REASON_TYPE_T     eReason);
int glGetRstReason(void);
u_int8_t kalIsRstPreventFwOwn(void);

#if CFG_CHIP_RESET_SUPPORT
void wlan_reset_thread_main(void *data);
void kalSetRstEvent(void);
void glResetInit(struct GLUE_INFO *prGlueInfo);
void glResetUninit(void);
u_int8_t glResetTrigger(struct ADAPTER *prAdapter,
		uint32_t u4RstFlag, const uint8_t *pucFile, uint32_t u4Line);
void glSetWfsysResetState(struct ADAPTER *prAdapter,
			  enum ENUM_WFSYS_RESET_STATE_TYPE_T state);
u_int8_t glResetMsgHandler(enum ENUM_WMTRSTMSG_TYPE MsgBody);
#endif

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
#if CFG_CHIP_RESET_SUPPORT
extern EventGroupHandle_t g_rst_init_wait;
extern EventGroupHandle_t g_event_wlan_reset_thread;
extern EventGroupHandle_t g_rst_init_wait;
extern u_int8_t fgIsResetting;
extern SemaphoreHandle_t g_wait_wifi_main_down;

#if CFG_CHIP_RESET_L0_SUPPORT
extern u_int8_t g_IsWfsysBusHang;
#endif
#endif
extern u_int8_t fgSimplifyResetFlow;
/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
#define RST_FLAG_CHIP_RESET        0
#define RST_FLAG_DO_CORE_DUMP      BIT(0)
#define RST_FLAG_PREVENT_POWER_OFF BIT(1)
#define RST_FLAG_DO_WHOLE_RESET    BIT(2)

#if 0 //Implemented from os/linux/include/gl_rst.h 
#if CFG_CHIP_RESET_SUPPORT
#define GL_RESET_TRIGGER(_prAdapter, _u4Flags) \
	glResetTrigger(_prAdapter, (_u4Flags), \
	(const uint8_t *)__FILE__, __LINE__)
#else
#define GL_RESET_TRIGGER(_prAdapter, _u4Flags) \
	DBGLOG(INIT, INFO, "DO NOT support chip reset\n")
#endif
#else
#if CFG_CHIP_RESET_SUPPORT
#if (CFG_WMT_RESET_API_SUPPORT == 1) || (CFG_SUPPORT_CONNINFRA == 1)
#define glResetSelectAction(_prAdapter)    glResetSelectActionMobile(_prAdapter)
#else
#define glResetSelectAction(_prAdapter)    glResetSelectActionCe(_prAdapter)
#endif /* CFG_WMT_RESET_API_SUPPORT || CFG_SUPPORT_CONNINFRA */

/* Each reset trigger reason has corresponding default reset action, which is
 * defined in glResetSelectAction(). You can use this macro to trigger default
 * action.
 */
#ifndef MT7921_PORTING_TMP_MASK 
#define GL_DEFAULT_RESET_TRIGGER(_prAdapter, _eReason)		\
{    \
	uint32_t u4RstFlag; \
	glSetRstReason(_eReason);   \
	u4RstFlag = glResetSelectAction(_prAdapter); \
	glResetTrigger(_prAdapter, u4RstFlag,	\
		       (const uint8_t *)__FILE__, __LINE__);     \
}
#else
#define GL_DEFAULT_RESET_TRIGGER(_prAdapter, _eReason)		\
    DBGLOG(INIT, INFO, "DO NOT support chip reset\n")
#endif
/* You can use this macro to trigger user defined reset actions instead of the
 * default ones.
 */
#define GL_USER_DEFINE_RESET_TRIGGER(_prAdapter, _eReason, _u4Flags)    \
{    \
	glSetRstReason(_eReason);    \
	glResetTrigger(_prAdapter, _u4Flags,	\
		       (const uint8_t *)__FILE__, __LINE__);     \
}

#define GL_SET_WFSYS_RESET_POSTPONE(_prAdapter, _fgPostpone)    \
	(_prAdapter->fgWfsysResetPostpone = _fgPostpone)

#define GL_GET_WFSYS_RESET_POSTPONE(_prAdapter)    \
	(_prAdapter->fgWfsysResetPostpone)
#else
#define glResetSelectAction(_prAdapter)    \
{    \
	DBGLOG(INIT, INFO, "DO NOT support chip reset\n");    \
}

#define GL_DEFAULT_RESET_TRIGGER(_prAdapter, _eReason) \
{    \
	DBGLOG(INIT, INFO, "DO NOT support chip reset\n");    \
}

#define GL_USER_DEFINE_RESET_TRIGGER(_prAdapter, _eReason, _u4Flags)    \
{    \
	DBGLOG(INIT, INFO, "DO NOT support chip reset\n");    \
}

#define GL_SET_WFSYS_RESET_POSTPONE(_prAdapter, _fgPostpone)    \
{    \
	DBGLOG(INIT, INFO, "DO NOT support chip reset\n");    \
}

#define GL_GET_WFSYS_RESET_POSTPONE(_prAdapter)    \
{    \
	DBGLOG(INIT, INFO, "DO NOT support chip reset\n");    \
}
#endif /* CFG_CHIP_RESET_SUPPORT */
#endif
/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif /* _GL_RST_H */
