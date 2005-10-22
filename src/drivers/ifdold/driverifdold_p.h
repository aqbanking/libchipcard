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


#ifndef CHIPCARD_DRIVER_IFDOLD_P_H
#define CHIPCARD_DRIVER_IFDOLD_P_H

#include "driverifdold.h"

#include <gwenhywfar/libloader.h>
#include "driver_l.h"


#define IFD_POWER_UP           500
#define IFD_POWER_DOWN         501
#define IFD_RESET              502

#define DRIVER_IFDOLD_ERROR_BAD_RESPONSE          0x80000001
#define DRIVER_IFDOLD_ERROR_NO_SLOTS_CONNECTED    0x80000002
#define DRIVER_IFDOLD_ERROR_NO_SLOTS_DISCONNECTED 0x80000003
#define DRIVER_IFDOLD_ERROR_NO_SLOTS_AVAILABLE    0x80000004
#define DRIVER_IFDOLD_ERROR_NOT_SUPPORTED         0x80000005

#define IFD_ERROR_NOT_SUPPORTED	606
#define IFD_ERROR_POWER_ACTION  608
#define IFD_NOT_SUPPORTED       614
#define IFD_ICC_PRESENT         615
#define IFD_ICC_NOT_PRESENT     616

typedef struct _SCARD_IO_HEADER {
  GWEN_TYPE_UINT32 protocol;
  GWEN_TYPE_UINT32 length;
} SCARD_IO_HEADER;


typedef long (*IFDCREATECHANNEL_PTR)(GWEN_TYPE_UINT32 lun,
                                    GWEN_TYPE_UINT32 channel);
typedef long (*IFDCLOSECHANNEL_PTR)(GWEN_TYPE_UINT32 lun);

typedef long (*IFDPOWERICC_PTR)(GWEN_TYPE_UINT32 lun,
                                GWEN_TYPE_UINT32 action,
                                unsigned char *atr,
                                GWEN_TYPE_UINT32 *atrlen);

typedef long (*IFDTRANSMIT_PTR)(GWEN_TYPE_UINT32 lun,
                                SCARD_IO_HEADER TxPci,
                                const unsigned char *TxBuffer,
                                GWEN_TYPE_UINT32 TxBufferLen,
                                unsigned char *RxBuffer,
                                GWEN_TYPE_UINT32 *RxBufferLen,
                                SCARD_IO_HEADER *RxPci);

typedef long (*IFDCONTROL_PTR)(GWEN_TYPE_UINT32 lun,
                               const unsigned char *TxBuffer,
                               GWEN_TYPE_UINT32 TxBufferLen,
                               unsigned char *RxBuffer,
                               GWEN_TYPE_UINT32 *RxBufferLen);

typedef long (*IFDPRESENCE_PTR)(GWEN_TYPE_UINT32 lun);

typedef long (*IFDGETCAPS_PTR)(GWEN_TYPE_UINT32 lun,
                               GWEN_TYPE_UINT32 tag,
                               GWEN_TYPE_UINT32 *plen,
                               unsigned char *pvalue);


struct DRIVER_IFDOLD {
  GWEN_LIBLOADER *libLoader;

  IFDCREATECHANNEL_PTR createChannelFn;
  IFDCLOSECHANNEL_PTR closeChannelFn;
  IFDPOWERICC_PTR powerIccFn;
  IFDTRANSMIT_PTR transmitFn;
  IFDCONTROL_PTR controlFn;
  IFDPRESENCE_PTR presenceFn;
  IFDGETCAPS_PTR getCapsFn;
};


void DriverIFDOld_freeData(void *bp, void *p);

int DriverIFDOld_ExtractProtocolInfo(unsigned char *atr,
                                  unsigned int atrlen);


GWEN_TYPE_UINT32 DriverIFDOld_SendAPDU(LCD_DRIVER *d,
                                    int toReader,
                                    LCD_READER *r,
                                    LCD_SLOT *slot,
                                    const unsigned char *apdu,
                                    unsigned int apdulen,
                                    unsigned char *buffer,
                                    int *bufferlen);
GWEN_TYPE_UINT32 DriverIFDOld_ConnectSlot(LCD_DRIVER *d, LCD_SLOT *sl);
GWEN_TYPE_UINT32 DriverIFDOld_ConnectReader(LCD_DRIVER *d, LCD_READER *r);

GWEN_TYPE_UINT32 DriverIFDOld_DisconnectSlot(LCD_DRIVER *d, LCD_SLOT *sl);
GWEN_TYPE_UINT32 DriverIFDOld_DisconnectReader(LCD_DRIVER *d, LCD_READER *r);

GWEN_TYPE_UINT32 DriverIFDOld_ResetSlot(LCD_DRIVER *d, LCD_SLOT *sl);

GWEN_TYPE_UINT32 DriverIFDOld_ReaderStatus(LCD_DRIVER *d, LCD_READER *r);

GWEN_TYPE_UINT32 DriverIFDOld_ReaderInfo(LCD_DRIVER *d, LCD_READER *r,
                                      GWEN_BUFFER *buf);

const char *DriverIFDOld_GetErrorText(LCD_DRIVER *d, GWEN_TYPE_UINT32 err);



#endif /* CHIPCARD_DRIVER_IFDOLD_P_H */



