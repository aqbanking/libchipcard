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


#ifndef CHIPCARD_DRIVER_CCID_P_H
#define CHIPCARD_DRIVER_CCID_P_H

#include "driverccid.h"

#include <gwenhywfar/libloader.h>
#include "driver_l.h"


#define CCID_POWER_UP           500
#define CCID_POWER_DOWN         501
#define CCID_RESET              502

#define DRIVER_CCID_ERROR_BAD_RESPONSE          0x80000001
#define DRIVER_CCID_ERROR_NO_SLOTS_CONNECTED    0x80000002
#define DRIVER_CCID_ERROR_NO_SLOTS_DISCONNECTED 0x80000003
#define DRIVER_CCID_ERROR_NO_SLOTS_AVAILABLE    0x80000004
#define DRIVER_CCID_ERROR_NOT_SUPPORTED         0x80000005

#define CCID_ERROR_NOT_SUPPORTED	606
#define CCID_ERROR_POWER_ACTION  608
#define CCID_NOT_SUPPORTED       614
#define CCID_ICC_PRESENT         615
#define CCID_ICC_NOT_PRESENT     616

typedef struct _SCARD_IO_HEADER {
  GWEN_TYPE_UINT32 protocol;
  GWEN_TYPE_UINT32 length;
} SCARD_IO_HEADER;


typedef long (*CCIDCREATECHANNEL_PTR)(GWEN_TYPE_UINT32 lun,
                                    GWEN_TYPE_UINT32 channel);
typedef long (*CCIDCLOSECHANNEL_PTR)(GWEN_TYPE_UINT32 lun);

typedef long (*CCIDPOWERICC_PTR)(GWEN_TYPE_UINT32 lun,
                                GWEN_TYPE_UINT32 action,
                                unsigned char *atr,
                                GWEN_TYPE_UINT32 *atrlen);

typedef long (*CCIDTRANSMIT_PTR)(GWEN_TYPE_UINT32 lun,
                                SCARD_IO_HEADER TxPci,
                                const unsigned char *TxBuffer,
                                GWEN_TYPE_UINT32 TxBufferLen,
                                unsigned char *RxBuffer,
                                GWEN_TYPE_UINT32 *RxBufferLen,
                                SCARD_IO_HEADER *RxPci);

typedef long (*CCIDCONTROL_PTR)(GWEN_TYPE_UINT32 lun,
                               GWEN_TYPE_UINT32 controlCode,
                               const unsigned char *TxBuffer,
                               GWEN_TYPE_UINT32 TxBufferLen,
                               unsigned char *RxBuffer,
                               GWEN_TYPE_UINT32 RxBufferLen,
                               GWEN_TYPE_UINT32 *pdwBytesReturned);

typedef long (*CCIDPRESENCE_PTR)(GWEN_TYPE_UINT32 lun);

typedef long (*CCIDGETCAPS_PTR)(GWEN_TYPE_UINT32 lun,
                               GWEN_TYPE_UINT32 tag,
                               GWEN_TYPE_UINT32 *plen,
                               unsigned char *pvalue);


struct DRIVER_CCID {
  GWEN_LIBLOADER *libLoader;

  CCIDCREATECHANNEL_PTR createChannelFn;
  CCIDCLOSECHANNEL_PTR closeChannelFn;
  CCIDPOWERICC_PTR powerIccFn;
  CCIDTRANSMIT_PTR transmitFn;
  CCIDCONTROL_PTR controlFn;
  CCIDPRESENCE_PTR presenceFn;
  CCIDGETCAPS_PTR getCapsFn;
};


void DriverCCID_freeData(void *bp, void *p);

int DriverCCID_ExtractProtocolInfo(unsigned char *atr,
                                  unsigned int atrlen);


GWEN_TYPE_UINT32 DriverCCID_SendAPDU(LCD_DRIVER *d,
                                    int toReader,
                                    LCD_READER *r,
                                    LCD_SLOT *slot,
                                    const unsigned char *apdu,
                                    unsigned int apdulen,
                                    unsigned char *buffer,
                                    int *bufferlen);
GWEN_TYPE_UINT32 DriverCCID_ConnectSlot(LCD_DRIVER *d, LCD_SLOT *sl);
GWEN_TYPE_UINT32 DriverCCID_ConnectReader(LCD_DRIVER *d, LCD_READER *r);

GWEN_TYPE_UINT32 DriverCCID_DisconnectSlot(LCD_DRIVER *d, LCD_SLOT *sl);
GWEN_TYPE_UINT32 DriverCCID_DisconnectReader(LCD_DRIVER *d, LCD_READER *r);

GWEN_TYPE_UINT32 DriverCCID_ResetSlot(LCD_DRIVER *d, LCD_SLOT *sl);

GWEN_TYPE_UINT32 DriverCCID_ReaderStatus(LCD_DRIVER *d, LCD_READER *r);

GWEN_TYPE_UINT32 DriverCCID_ReaderInfo(LCD_DRIVER *d, LCD_READER *r,
                                      GWEN_BUFFER *buf);

GWEN_TYPE_UINT32 DriverCCID_PerformVerification(LCD_DRIVER *d,
                                                LCD_READER *r,
                                                LCD_SLOT *slot,
                                                const LC_PININFO *pi,
                                                int *triesLeft);

GWEN_TYPE_UINT32 DriverCCID_PerformModification(LCD_DRIVER *d,
                                                LCD_READER *r,
                                                LCD_SLOT *slot,
                                                const LC_PININFO *pi,
                                                int *triesLeft);

const char *DriverCCID_GetErrorText(LCD_DRIVER *d, GWEN_TYPE_UINT32 err);



/**
 * These functions are needed by some drivers.
 */
/*@{*/
void log_msg(const int priority, const char *fmt, ...);
char *pcsc_stringify_error(long x);
/*@}*/


#endif /* CHIPCARD_DRIVER_CCID_P_H */



