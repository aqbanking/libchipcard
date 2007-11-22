/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id$
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_DRIVER_IFD_P_H
#define CHIPCARD_DRIVER_IFD_P_H

#include "driverifd.h"

#include <gwenhywfar/libloader.h>
#include <inttypes.h>

#include <chipcard/server/driver/driver.h>


#define CCID_POWER_UP             500
#define CCID_POWER_DOWN           501
#define CCID_RESET                502

#define CCID_ERROR_NOT_SUPPORTED  606
#define CCID_ERROR_POWER_ACTION   608
#define CCID_COMMUNICATION_ERROR  612
#define CCID_NOT_SUPPORTED        614
#define CCID_ICC_PRESENT          615
#define CCID_ICC_NOT_PRESENT      616


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

/* The follwoing definition is NOT correct for 64 bit architectures but
 * unfortunately PC/SC defines them this way. So in order to be able to work
 * with IFD drivers on 64 bit archs as well we need to propagete this error
 * here :-(
 */
typedef unsigned long DWORD;

/* the structure must be 6-bytes long */
typedef struct {
  uint8_t tag;
  uint8_t length;
  uint32_t value;
} PCSC_TLV_STRUCTURE;


typedef struct {
  uint8_t bTimerOut;	/* timeout is seconds (00 means use default timeout) */
  uint8_t bTimerOut2; /* timeout in seconds after first key stroke */
  uint8_t bmFormatString; /* formatting options */
  uint8_t bmPINBlockString; /* bits 7-4 bit size of PIN length in APDU,
  * bits 3-0 PIN block size in bytes after
  * justification and formatting */
  uint8_t bmPINLengthFormat; /* bits 7-5 RFU,
  * bit 4 set if system units are bytes, clear if
  * system units are bits,
  * bits 3-0 PIN length position in system units */
  uint16_t wPINMaxExtraDigit; /* 0xXXYY where XX is minimum PIN size in digits,
  and YY is maximum PIN size in digits */
  uint8_t bEntryValidationCondition; /* Conditions under which PIN entry should
  * be considered complete */
  uint8_t bNumberMessage; /* Number of messages to display for PIN verification */
  uint16_t wLangId; /* Language for messages */
  uint8_t bMsgIndex; /* Message index (should be 00) */
  uint8_t bTeoPrologue[3]; /* T=1 block prologue field to use (fill with 00) */
  uint32_t ulDataLength; /* length of Data to be sent to the ICC */
  uint8_t abData[1]; /* Data to send to the ICC */
} PIN_VERIFY_STRUCTURE;


typedef struct _SCARD_IO_HEADER {
  DWORD protocol;
  DWORD length;
} SCARD_IO_HEADER;

#ifdef __APPLE__
#pragma pack()
#else
#pragma pack(pop)
#endif


#ifdef WORDS_BIGENDIAN
# define HOST_TO_CCID_16(x) \
  ((((x) >> 8) & 0xff) + \
  ((x & 0xff) << 8))
# define HOST_TO_CCID_32(x) \
  (((((x) >> 24) & 0xff) + \
  (((x) >> 8) & 0xff00) + \
  ((x & 0xff00) << 8) + \
  (((x) & 0xff) << 24)))
#else
# define HOST_TO_CCID_16(x) (x)
# define HOST_TO_CCID_32(x) (x)
#endif

typedef long (*CCIDCREATECHANNEL_PTR)(DWORD lun,
                                      DWORD channel);
typedef long (*CCIDCREATECHANNELBYNAME_PTR)(DWORD lun,
                                            const char *name);
typedef long (*CCIDCLOSECHANNEL_PTR)(DWORD lun);

typedef long (*CCIDPOWERICC_PTR)(DWORD lun,
                                 DWORD action,
                                 unsigned char *atr,
                                 DWORD *atrlen);

typedef long (*CCIDTRANSMIT_PTR)(DWORD lun,
                                 SCARD_IO_HEADER TxPci,
                                 const unsigned char *TxBuffer,
                                 DWORD TxBufferLen,
                                 unsigned char *RxBuffer,
                                 DWORD *RxBufferLen,
                                 SCARD_IO_HEADER *RxPci);

typedef long (*CCIDCONTROL3_PTR)(DWORD lun,
                                 DWORD controlCode,
                                 const unsigned char *TxBuffer,
                                 DWORD TxBufferLen,
                                 unsigned char *RxBuffer,
                                 DWORD RxBufferLen,
                                 DWORD *pdwBytesReturned);

typedef long (*CCIDCONTROL2_PTR)(DWORD lun,
                                 const unsigned char *TxBuffer,
                                 DWORD TxBufferLen,
                                 unsigned char *RxBuffer,
                                 DWORD *RxBufferLen);

typedef long (*CCIDPRESENCE_PTR)(DWORD lun);

typedef long (*CCIDGETCAPS_PTR)(DWORD lun,
                                DWORD tag,
                                DWORD *plen,
                                unsigned char *pvalue);

typedef long (*CCIDSETPROTO_PTR)(DWORD Lun, DWORD Protocol,
				 unsigned char Flags,
				 unsigned char PTS1,
				 unsigned char PTS2,
				 unsigned char PTS3);


struct DRIVER_IFD {
  GWEN_LIBLOADER *libLoader;

  int ifdVersion;

  CCIDCREATECHANNEL_PTR createChannelFn;
  CCIDCREATECHANNELBYNAME_PTR createChannelByNameFn;
  CCIDCLOSECHANNEL_PTR closeChannelFn;
  CCIDPOWERICC_PTR powerIccFn;
  CCIDTRANSMIT_PTR transmitFn;
  CCIDCONTROL3_PTR control3Fn;
  CCIDCONTROL2_PTR control2Fn;
  CCIDPRESENCE_PTR presenceFn;
  CCIDGETCAPS_PTR getCapsFn;
  CCIDSETPROTO_PTR setProtoFn;
};


void GWENHYWFAR_CB DriverIFD_freeData(void *bp, void *p);

int DriverIFD_ExtractProtocolInfo(unsigned char *atr,
				  unsigned int atrlen);


uint32_t DriverIFD_SendAPDU(LCD_DRIVER *d,
			    int toReader,
			    LCD_READER *r,
			    LCD_SLOT *slot,
			    const unsigned char *apdu,
			    unsigned int apdulen,
			    unsigned char *buffer,
			    int *bufferlen);
uint32_t DriverIFD_ConnectSlot(LCD_DRIVER *d, LCD_SLOT *sl);
uint32_t DriverIFD_ConnectReader(LCD_DRIVER *d, LCD_READER *r);

uint32_t DriverIFD_DisconnectSlot(LCD_DRIVER *d, LCD_SLOT *sl);
uint32_t DriverIFD_DisconnectReader(LCD_DRIVER *d, LCD_READER *r);

uint32_t DriverIFD_ResetSlot(LCD_DRIVER *d, LCD_SLOT *sl);

uint32_t DriverIFD_ReaderStatus(LCD_DRIVER *d, LCD_READER *r);

uint32_t DriverIFD_ReaderInfo(LCD_DRIVER *d, LCD_READER *r,
                                       GWEN_BUFFER *buf);

int DriverIFD_ExtendReader(LCD_DRIVER *d, LCD_READER *r);

const char *DriverIFD_GetErrorText(LCD_DRIVER *d, uint32_t err);



/**
 * These functions are needed by some drivers.
 */
/*@{*/
void log_msg(const int priority, const char *fmt, ...);
char *pcsc_stringify_error(long x);
/*@}*/


void DriverIFD__checkMsg(int priority, const char *msg);


#endif /* CHIPCARD_DRIVER_IFD_P_H */



