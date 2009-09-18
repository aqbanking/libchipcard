/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: card.h 282 2006-09-21 16:52:04Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/



#ifndef CHIPCARD_SERVER_COMMON_READER_P_H
#define CHIPCARD_SERVER_COMMON_READER_P_H

#include "reader.h"


struct LCCO_READER {
  GWEN_INHERIT_ELEMENT(LCDM_READER);
  GWEN_LIST_ELEMENT(LCDM_READER);

  /* variables from config file */
  char *readerType;
  char *readerName;
  char *driverName;
  char *shortDescr;
  char *readerInfo;

  unsigned int slots;
  unsigned int ctn;
  LCCO_READER_ADDRESS addressType;
  unsigned int port;
  char *devicePath;
  char *devicePathTmpl;
  uint32_t flags;

  LC_DEVICE_BUSTYPE busType;
  uint32_t vendorId;
  uint32_t productId;

  int usbClass;

  /* runtime variables */
  int isAvailable;
  uint32_t busId;
  uint32_t deviceId;

  uint32_t readerId;
  uint32_t driversReaderId;
  LC_READER_STATUS status;

  time_t lastStatusChangeTime;

  uint32_t refCount;

};


#endif /* CHIPCARD_SERVER_COMMON_READER_P_H */


