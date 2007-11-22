/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: driverctapi_p.h 142 2005-11-29 21:40:33Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_DRIVER_SKEL2_P_H
#define CHIPCARD_DRIVER_SKEL2_P_H

#include "driverSKEL1.h"

#include <chipcard/server/driver/driver.h>


struct DRIVER_SKEL2 {
  int stopDriver;
  int nextCtn;
};


void GWENHYWFAR_CB DriverSKEL3_freeData(void *bp, void *p);

uint32_t  DriverSKEL3_SendAPDU(LCD_DRIVER *d,
                                       int toReader,
                                       LCD_READER *r,
                                       LCD_SLOT *slot,
                                       const unsigned char *apdu,
                                       unsigned int apdulen,
                                       unsigned char *buffer,
                                       int *bufferlen);

uint32_t  DriverSKEL3_ConnectSlot(LCD_DRIVER *d, LCD_SLOT *sl);
uint32_t  DriverSKEL3_ConnectReader(LCD_DRIVER *d, LCD_READER *r);

uint32_t  DriverSKEL3_DisconnectSlot(LCD_DRIVER *d, LCD_SLOT *sl);
uint32_t  DriverSKEL3_DisconnectReader(LCD_DRIVER *d, LCD_READER *r);

uint32_t  DriverSKEL3_ResetSlot(LCD_DRIVER *d, LCD_SLOT *sl);


uint32_t  DriverSKEL3_ReaderStatus(LCD_DRIVER *d, LCD_READER *r);
const char *DriverSKEL3_GetErrorText(LCD_DRIVER *d, uint32_t err);


uint32_t DriverSKEL3_ReaderInfo(LCD_DRIVER *d, LCD_READER *r,
                                        GWEN_BUFFER *buf);

int DriverSKEL3_ExtendReader(LCD_DRIVER *d, LCD_READER *r);


#endif /* CHIPCARD_DRIVER_SKEL2_P_H */



