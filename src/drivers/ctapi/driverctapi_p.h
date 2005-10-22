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


#ifndef CHIPCARD_DRIVER_CTAPI_P_H
#define CHIPCARD_DRIVER_CTAPI_P_H

#include "driverctapi.h"

#include <gwenhywfar/libloader.h>
#include "driver_l.h"


#define LCD_DRIVERCTAPI_DAD_CT   1

#define LCD_DRIVERCTAPI_SAD_HOST 2

#define DRIVER_CTAPI_ERROR_BAD_RESPONSE          1
#define DRIVER_CTAPI_ERROR_NO_SLOTS_CONNECTED    2
#define DRIVER_CTAPI_ERROR_NO_SLOTS_DISCONNECTED 3
#define DRIVER_CTAPI_ERROR_NO_SLOTS_AVAILABLE    4
#define DRIVER_CTAPI_ERROR_READER_INIT           5
#define DRIVER_CTAPI_ERROR_GENERIC               6


typedef char (*CTAPIINITPTR) (unsigned short Ctn, unsigned short pn);
typedef char (*CTAPICLOSEPTR)(unsigned short Ctn);
typedef char (*CTAPIDATAPTR) (unsigned short Ctn,
                              unsigned char  *dad,
                              unsigned char  *sad,
                              unsigned short lc,
                              unsigned char  *cmd,
                              unsigned short *lr,
                              unsigned char  *rsp);




struct DRIVER_CTAPI {
  GWEN_LIBLOADER *libLoader;
  int stopDriver;
  int nextCtn;

  CTAPIINITPTR initFn;
  CTAPICLOSEPTR closeFn;
  CTAPIDATAPTR dataFn;
};


void DriverCTAPI_freeData(void *bp, void *p);


int DriverCTAPI_TransformDAD(int i);
GWEN_TYPE_UINT32  DriverCTAPI_SendAPDU(LCD_DRIVER *d,
                                       int toReader,
                                       LCD_READER *r,
                                       LCD_SLOT *slot,
                                       const unsigned char *apdu,
                                       unsigned int apdulen,
                                       unsigned char *buffer,
                                       int *bufferlen);
GWEN_TYPE_UINT32  DriverCTAPI_ConnectSlot(LCD_DRIVER *d, LCD_SLOT *sl);
GWEN_TYPE_UINT32  DriverCTAPI_ConnectReader(LCD_DRIVER *d, LCD_READER *r);

GWEN_TYPE_UINT32  DriverCTAPI_DisconnectSlot(LCD_DRIVER *d, LCD_SLOT *sl);
GWEN_TYPE_UINT32  DriverCTAPI_DisconnectReader(LCD_DRIVER *d, LCD_READER *r);

GWEN_TYPE_UINT32  DriverCTAPI_ResetSlot(LCD_DRIVER *d, LCD_SLOT *sl);


GWEN_TYPE_UINT32  DriverCTAPI_ReaderStatus(LCD_DRIVER *d, LCD_READER *r);
const char *DriverCTAPI_GetErrorText(LCD_DRIVER *d, GWEN_TYPE_UINT32 err);

GWEN_TYPE_UINT32 DriverCTAPI_ReaderInfo(LCD_DRIVER *d, LCD_READER *r,
                                        GWEN_BUFFER *buf);

LCD_READER *DriverCTAPI_CreateReader(LCD_DRIVER *d,
                                    GWEN_TYPE_UINT32 readerId,
                                    const char *name,
                                    int port,
                                    unsigned int slots,
                                    GWEN_TYPE_UINT32 flags);


#endif /* CHIPCARD_DRIVER_CTAPI_P_H */



