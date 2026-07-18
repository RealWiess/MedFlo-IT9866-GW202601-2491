
/***************************************************************************
 *
 *            Copyright (c) 2018 by HCC Embedded
 *
 * This software is copyrighted by and is the sole property of
 * HCC.  All rights, title, ownership, or other interests
 * in the software remain the property of HCC.  This
 * software may only be used in accordance with the corresponding
 * license agreement.  Any unauthorized use, duplication, transmission,
 * distribution, or disclosure of this software is expressly forbidden.
 *
 * This Copyright notice may not be removed or modified without prior
 * written consent of HCC.
 *
 * HCC reserves the right to modify this software without notice.
 *
 * HCC Embedded
 * Budapest 1133
 * Vaci ut 76
 * Hungary
 *
 * Tel:  +36 (1) 450 1302
 * Fax:  +36 (1) 450 1303
 * http: www.hcc-embedded.com
 * email: info@hcc-embedded.com
 *
 ***************************************************************************/
#ifndef _API_PCSC_H
#define _API_PCSC_H

#include "../psp/include/psp_types.h"
#include "../version/ver_usbh_ccid.h"
#if VER_USBH_CCID_MAJOR != 1 || VER_USBH_CCID_MINOR != 6
 #error Incompatible CCID version number!
#endif

#ifdef __cplusplus
extern "C" {
#endif


#define     SCARD_S_SUCCESS                 ( (uint32_t)0x00000000 )
#define     SCARD_F_INTERNAL_ERROR          ( (uint32_t)0x80100001 )
#define     SCARD_E_CANCELLED               ( (uint32_t)0x80100002 )
#define     SCARD_E_INVALID_HANDLE          ( (uint32_t)0x80100003 )
#define     SCARD_E_INVALID_PARAMETER       ( (uint32_t)0x80100004 )
#define     SCARD_E_INVALID_TARGET          ( (uint32_t)0x80100005 )
#define     SCARD_E_NO_MEMORY               ( (uint32_t)0x80100006 )
#define     SCARD_F_WAITED_TOO_LONG         ( (uint32_t)0x80100007 )
#define     SCARD_E_INSUFFICIENT_BUFFER     ( (uint32_t)0x80100008 )
#define     SCARD_E_UNKNOWN_READER          ( (uint32_t)0x80100009 )
#define     SCARD_E_TIMEOUT                 ( (uint32_t)0x8010000A )
#define     SCARD_E_SHARING_VIOLATION       ( (uint32_t)0x8010000B )
#define     SCARD_E_NO_SMARTCARD            ( (uint32_t)0x8010000C )
#define     SCARD_E_UNKNOWN_CARD            ( (uint32_t)0x8010000D )
#define     SCARD_E_CANT_DISPOSE            ( (uint32_t)0x8010000E )
#define     SCARD_E_PROTO_MISMATCH          ( (uint32_t)0x8010000F )
#define     SCARD_E_NOT_READY               ( (uint32_t)0x80100010 )
#define     SCARD_E_INVALID_VALUE           ( (uint32_t)0x80100011 )
#define     SCARD_E_SYSTEM_CANCELLED        ( (uint32_t)0x80100012 )
#define     SCARD_F_COMM_ERROR              ( (uint32_t)0x80100013 )
#define     SCARD_F_UNKNOWN_ERROR           ( (uint32_t)0x80100014 )
#define     SCARD_E_INVALID_ATR             ( (uint32_t)0x80100015 )
#define     SCARD_E_NOT_TRANSACTED          ( (uint32_t)0x80100016 )
#define     SCARD_E_READER_UNAVAILABLE      ( (uint32_t)0x80100017 )
#define     SCARD_P_SHUTDOWN                ( (uint32_t)0x80100018 )
#define     SCARD_E_PCI_TOO_SMALL           ( (uint32_t)0x80100019 )
#define     SCARD_E_READER_UNSUPPORTED      ( (uint32_t)0x8010001A )
#define     SCARD_E_DUPLICATE_READER        ( (uint32_t)0x8010001B )
#define     SCARD_E_CARD_UNSUPPORTED        ( (uint32_t)0x8010001C )
#define     SCARD_E_NO_SERVICE              ( (uint32_t)0x8010001D )
#define     SCARD_E_SERVICE_STOPPED         ( (uint32_t)0x8010001E )
#define     SCARD_E_UNEXPECTED              ( (uint32_t)0x8010001F )
#define     SCARD_E_UNSUPPORTED_FEATURE     ( (uint32_t)0x8010001F )
#define     SCARD_E_ICC_INSTALLATION        ( (uint32_t)0x80100020 )
#define     SCARD_E_ICC_CREATEORDER         ( (uint32_t)0x80100021 )
#define     SCARD_E_DIR_NOT_FOUND           ( (uint32_t)0x80100023 )
#define     SCARD_E_FILE_NOT_FOUND          ( (uint32_t)0x80100024 )
#define     SCARD_E_NO_DIR                  ( (uint32_t)0x80100025 )
#define     SCARD_E_NO_FILE                 ( (uint32_t)0x80100026 )
#define     SCARD_E_NO_ACCESS               ( (uint32_t)0x80100027 )
#define     SCARD_E_WRITE_TOO_MANY          ( (uint32_t)0x80100028 )
#define     SCARD_E_BAD_SEEK                ( (uint32_t)0x80100029 )
#define     SCARD_E_INVALID_CHV             ( (uint32_t)0x8010002A )
#define     SCARD_E_UNKNOWN_RES_MNG         ( (uint32_t)0x8010002B )
#define     SCARD_E_NO_SUCH_CERTIFICATE     ( (uint32_t)0x8010002C )
#define     SCARD_E_CERTIFICATE_UNAVAILABLE ( (uint32_t)0x8010002D )
#define     SCARD_E_NO_READERS_AVAILABLE    ( (uint32_t)0x8010002E )
#define     SCARD_E_COMM_DATA_LOST          ( (uint32_t)0x8010002F )
#define     SCARD_E_NO_KEY_CONTAINER        ( (uint32_t)0x80100030 )
#define     SCARD_E_SERVER_TOO_BUSY         ( (uint32_t)0x80100031 )
#define     SCARD_W_UNSUPPORTED_CARD        ( (uint32_t)0x80100065 )
#define     SCARD_W_UNRESPONSIVE_CARD       ( (uint32_t)0x80100066 )
#define     SCARD_W_UNPOWERED_CARD          ( (uint32_t)0x80100067 )
#define     SCARD_W_RESET_CARD              ( (uint32_t)0x80100068 )
#define     SCARD_W_REMOVED_CARD            ( (uint32_t)0x80100069 )
#define     SCARD_W_SECURITY_VIOLATION      ( (uint32_t)0x8010006A )
#define     SCARD_W_WRONG_CHV               ( (uint32_t)0x8010006B )
#define     SCARD_W_CHV_BLOCKED             ( (uint32_t)0x8010006C )
#define     SCARD_W_EOF                     ( (uint32_t)0x8010006D )
#define     SCARD_W_CANCELLED_BY_USER       ( (uint32_t)0x8010006E )
#define     SCARD_W_CARD_NOT_AUTHENTICATED  ( (uint32_t)0x8010006F )

#define     SCARD_SCOPE_USER                0x0000
#define     SCARD_SCOPE_TERMINAL            0x0001
#define     SCARD_SCOPE_SYSTEM              0x0002
#define     SCARD_SCOPE_GLOBAL              0x0003
#define     SCARD_PROTOCOL_UNDEFINED        0x0000
#define     SCARD_PROTOCOL_UNSET            SCARD_PROTOCOL_UNDEFINED /* backward compat */
#define     SCARD_PROTOCOL_T0               0x0001
#define     SCARD_PROTOCOL_T1               0x0002
#define     SCARD_PROTOCOL_RAW              0x0004
#define     SCARD_PROTOCOL_T15              0x0008
#define     SCARD_PROTOCOL_ANY              ( SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1 )
#define     SCARD_SHARE_EXCLUSIVE           0x0001
#define     SCARD_SHARE_SHARED              0x0002
#define     SCARD_SHARE_DIRECT              0x0003
#define     SCARD_LEAVE_CARD                0x0000
#define     SCARD_RESET_CARD                0x0001
#define     SCARD_UNPOWER_CARD              0x0002
#define     SCARD_EJECT_CARD                0x0003
#define     SCARD_UNKNOWN                   0x0001
#define     SCARD_ABSENT                    0x0002
#define     SCARD_PRESENT                   0x0004
#define     SCARD_SWALLOWED                 0x0008
#define     SCARD_POWERED                   0x0010
#define     SCARD_NEGOTIABLE                0x0020
#define     SCARD_SPECIFIC                  0x0040
#define     SCARD_STATE_UNAWARE             0x0000
#define     SCARD_STATE_IGNORE              0x0001
#define     SCARD_STATE_CHANGED             0x0002
#define     SCARD_STATE_UNKNOWN             0x0004
#define     SCARD_STATE_UNAVAILABLE         0x0008
#define     SCARD_STATE_EMPTY               0x0010
#define     SCARD_STATE_PRESENT             0x0020
#define     SCARD_STATE_ATRMATCH            0x0040
#define     SCARD_STATE_EXCLUSIVE           0x0080
#define     SCARD_STATE_INUSE               0x0100
#define     SCARD_STATE_MUTE                0x0200
#define     SCARD_STATE_UNPOWERED           0x0400

#define     SCARD_AUTOALLOCATE              (DWORD)( -1 )

#define MAX_ATR_SIZE                        33u /* maximum ATR size */
#define INFINITE                            0xFFFFFFFF


typedef uint8_t        BYTE;
typedef int32_t        LONG;
typedef uint32_t       DWORD;

typedef LONG           SCARDCONTEXT;
typedef SCARDCONTEXT * PSCARDCONTEXT;
typedef SCARDCONTEXT * LPSCARDCONTEXT;

typedef LONG           SCARDHANDLE;
typedef SCARDHANDLE  * LPSCARDHANDLE;

typedef void         * LPVOID;
typedef const void   * LPCVOID;
typedef char         * LPSTR;
typedef const char   * LPCSTR;
typedef BYTE         * LPBYTE;
typedef const BYTE   * LPCBYTE;
typedef DWORD        * LPDWORD;


typedef struct
{
  unsigned long  dwProtocol;  /**< Protocol identifier */
  unsigned long  cbPciLength; /**< Protocol Control Inf Length */
}
SCARD_IO_REQUEST, * PSCARD_IO_REQUEST, * LPSCARD_IO_REQUEST;

typedef struct
{
  const char   * szReader;
  void         * pvUserData;
  DWORD          dwCurrentState;
  DWORD          dwEventState;
  DWORD          cbAtr;
  unsigned char  rgbAtr[MAX_ATR_SIZE];
}
SCARD_READERSTATE, * LPSCARD_READERSTATE;


int pcsc_init ( void );
int pcsc_delete ( void );
LONG    SCardEstablishContext ( DWORD dwScope, LPCVOID pvReserved1, LPCVOID pvReserved2, LPSCARDCONTEXT phContext );
LONG    SCardReleaseContext ( SCARDCONTEXT hContext );
LONG    SCardConnect ( SCARDCONTEXT hContext, LPCSTR szReader, DWORD dwShareMode, DWORD dwPreferredProtocols, LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol );
LONG    SCardReconnect ( SCARDHANDLE hCard, DWORD dwShareMode, DWORD dwPreferredProtocols, DWORD dwInitialization, LPDWORD pdwActiveProtocol );
LONG    SCardDisconnect ( SCARDHANDLE hCard, DWORD dwDisposition );
LONG    SCardBeginTransaction ( SCARDHANDLE hCard );
LONG    SCardEndTransaction ( SCARDHANDLE hCard, DWORD dwDisposition );
LONG    SCardStatus ( SCARDHANDLE hCard, LPSTR szReaderName, LPDWORD pcchReaderLen, LPDWORD pdwState, LPDWORD pdwProtocol, LPBYTE pbAtr, LPDWORD pcbAtrLen );
LONG    SCardGetStatusChange ( SCARDCONTEXT hContext, DWORD dwTimeout, SCARD_READERSTATE * rgReaderStates, DWORD cReaders );
LONG    SCardControl ( SCARDHANDLE hCard,
                      DWORD dwControlCode,
                      LPCVOID pbSendBuffer,
                      DWORD cbSendLength,
                      LPVOID pbRecvBuffer,
                      DWORD cbRecvLength,
                      LPDWORD lpBytesReturned );
LONG    SCardGetAttrib ( SCARDHANDLE hCard, DWORD dwAttrId, LPBYTE pbAttr, LPDWORD pcbAttrLen );
LONG    SCardSetAttrib ( SCARDHANDLE hCard, DWORD dwAttrId, LPCBYTE pbAttr, DWORD cbAttrLen );
LONG    SCardTransmit ( SCARDHANDLE hCard,
                       const SCARD_IO_REQUEST * pioSendPci,
                       LPCBYTE pbSendBuffer,
                       DWORD cbSendLength,
                       SCARD_IO_REQUEST * pioRecvPci,
                       LPBYTE pbRecvBuffer,
                       LPDWORD pcbRecvLength );
LONG    SCardListReaders ( SCARDCONTEXT hContext, LPCSTR mszGroups, LPSTR mszReaders, LPDWORD pcchReaders );
LONG    SCardFreeMemory ( SCARDCONTEXT hContext, LPCVOID pvMem );
LONG    SCardListReaderGroups ( SCARDCONTEXT hContext, LPSTR mszGroups, LPDWORD pcchGroups );
LONG    SCardCancel ( SCARDCONTEXT hContext );
LONG    SCardIsValidContext ( SCARDCONTEXT hContext );



#ifdef __cplusplus
}
#endif


#endif /* ifndef _API_PCSC_H */

