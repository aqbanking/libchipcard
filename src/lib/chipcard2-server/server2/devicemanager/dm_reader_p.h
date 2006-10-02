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


#ifndef CHIPCARD_SERVER_DM_READER_P_H
#define CHIPCARD_SERVER_DM_READER_P_H


#include "dm_reader_l.h"

#include "common/devmonitor.h"



struct LCDM_READER {
  GWEN_LIST_ELEMENT(LCDM_READER);

  /* variables from config file */
  char *readerType;
  char *readerName;
  char *driverName;
  char *shortDescr;
  unsigned int slots;
  unsigned int ctn;
  unsigned int port;
  GWEN_TYPE_UINT32 flags;

  LC_DEVICE_BUSTYPE busType;
  GWEN_TYPE_UINT32 vendorId;
  GWEN_TYPE_UINT32 productId;

  /* runtime variables */
  int isAvailable;
  GWEN_TYPE_UINT32 busId;
  GWEN_TYPE_UINT32 deviceId;

  char *readerInfo;
  GWEN_TYPE_UINT32 readerId;
  GWEN_TYPE_UINT32 driversReaderId;
  LCDM_DRIVER *driver;
  LC_READER_STATUS status;
  int wantRestart;
  time_t lastStatusChangeTime;
  time_t idleSince;

  time_t timeout;

  GWEN_TYPE_UINT32 currentRequestId;

  /** increment when attached to user or to ACTIVE cards (not when attached
   * to inactive cards!) */
  GWEN_TYPE_UINT32 usageCount;

  GWEN_TYPE_UINT32 refCount;

  LCDM_SLOT_LIST *slotList;
};


GWEN_TYPE_UINT32 LCDM_Reader_GetNextCount(LCDM_READER *r);


#endif /* CHIPCARD_SERVER_DM_READER_P_H */


