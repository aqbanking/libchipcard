/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: client_p.h 137 2005-11-03 13:07:50Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CLIENT_CLIENTPCSC_L_H
#define CHIPCARD_CLIENT_CLIENTPCSC_L_H

#include "clientpcsc.h"


#include <inttypes.h>

#define MAX_ATR_SIZE  33


/* define some types */
#ifndef OS_WIN32

#ifndef BYTE
typedef unsigned char BYTE;
#endif

/* basic types */
typedef unsigned char UCHAR;
typedef unsigned char *PUCHAR;
typedef unsigned short USHORT;
typedef unsigned long ULONG;
typedef void *LPVOID;
typedef short BOOL;
typedef unsigned long *PULONG;
typedef const void *LPCVOID;
typedef unsigned long DWORD;
typedef unsigned long *PDWORD;
typedef unsigned short WORD;
typedef long LONG;
typedef long RESPONSECODE;
typedef const char *LPCSTR;
typedef BYTE *LPBYTE;
typedef DWORD *LPDWORD;
typedef char *LPSTR;

typedef char *LPTSTR;
typedef const char *LPCTSTR;

#else
# include <windows.h>
# include <wtypes.h>
#endif

/* common basic types */
typedef const BYTE *LPCBYTE;


/* pc/sc types */
typedef ULONG SCARDCONTEXT;
typedef SCARDCONTEXT *PSCARDCONTEXT;
typedef SCARDCONTEXT *LPSCARDCONTEXT;
typedef ULONG SCARDHANDLE;
typedef SCARDHANDLE *PSCARDHANDLE;
typedef SCARDHANDLE *LPSCARDHANDLE;

typedef struct _SCARD_IO_REQUEST {
  unsigned long dwProtocol;
  unsigned long cbPciLength;
} SCARD_IO_REQUEST, *PSCARD_IO_REQUEST, *LPSCARD_IO_REQUEST;

typedef const SCARD_IO_REQUEST *LPCSCARD_IO_REQUEST;


#define SCARD_S_SUCCESS			0x00000000
#define SCARD_E_TIMEOUT			0x8010000A
#define SCARD_E_NO_SMARTCARD		0x8010000C
#define SCARD_E_UNKNOWN_CARD		0x8010000D
#define SCARD_E_PROTO_MISMATCH		0x8010000F
#define SCARD_E_NOT_READY		0x80100010
#define SCARD_E_READER_UNAVAILABLE	0x80100017

#define SCARD_W_UNSUPPORTED_CARD	0x80100065
#define SCARD_W_UNRESPONSIVE_CARD	0x80100066
#define SCARD_W_UNPOWERED_CARD		0x80100067
#define SCARD_W_RESET_CARD		0x80100068
#define SCARD_W_REMOVED_CARD		0x80100069

#define SCARD_E_READER_UNSUPPORTED	0x8010001A
#define SCARD_E_CARD_UNSUPPORTED	0x8010001C

#define SCARD_SCOPE_SYSTEM		0x0002
#define SCARD_PROTOCOL_T0		0x0001
#define SCARD_PROTOCOL_T1		0x0002

#define SCARD_SHARE_EXCLUSIVE		0x0001

#define SCARD_RESET_CARD		0x0001


/* from PC/SC */
#define SCARD_CTL_CODE(code) (0x42000000 + (code))
#define CM_IOCTL_GET_FEATURE_REQUEST SCARD_CTL_CODE(3400)

#define FEATURE_VERIFY_PIN_START  0x01 /* OMNIKEY Proposal */
#define FEATURE_VERIFY_PIN_FINISH 0x02 /* OMNIKEY Proposal */
#define FEATURE_MODIFY_PIN_START  0x03 /* OMNIKEY Proposal */
#define FEATURE_MODIFY_PIN_FINISH 0x04 /* OMNIKEY Proposal */
#define FEATURE_GET_KEY_PRESSED   0x05 /* OMNIKEY Proposal */
#define FEATURE_VERIFY_PIN_DIRECT 0x06 /* USB CCID PIN Verify */
#define FEATURE_MODIFY_PIN_DIRECT 0x07 /* USB CCID PIN Modify */
#define FEATURE_MCT_READERDIRECT  0x08 /* KOBIL Proposal */
#define FEATURE_MCT_UNIVERSAL     0x09 /* KOBIL Proposal */
#define FEATURE_IFD_PIN_PROP      0x0A /* Gemplus Proposal */
#define FEATURE_ABORT             0x0B /* SCM Proposal */


/* Set structure elements aligment on bytes
 * http://gcc.gnu.org/onlinedocs/gcc/Structure_002dPacking-Pragmas.html */
#ifdef __APPLE__
#pragma pack(1)
#else
#pragma pack(push, 1)
#endif

/* the structure must be 6-bytes long */
typedef struct {
  uint8_t tag;
  uint8_t length;
  uint32_t value;
} PCSC_TLV_STRUCTURE;

#ifdef __APPLE__
#pragma pack()
#else
#pragma pack(pop)
#endif





#endif /* CHIPCARD_CLIENT_CLIENTPCSC_L_H */

