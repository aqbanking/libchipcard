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


#ifndef CHIPCARD_SERVER_READER_P_H
#define CHIPCARD_SERVER_READER_P_H


#include <chipcard2-server/server/reader.h>
#include <chipcard2-server/server/request.h>


struct LC_READER {
  GWEN_LIST_ELEMENT(LC_READER);

  /* variables from config file */
  char *readerType;
  char *readerName;
  char *shortDescr;
  unsigned int slots;
  unsigned int port;
  GWEN_TYPE_UINT32 flags;

  char *comType;
  GWEN_TYPE_UINT32 vendorId;
  GWEN_TYPE_UINT32 productId;


  /* runtime variables */
  GWEN_TYPE_UINT32 readerId;
  LC_DRIVER *driver;
  LC_READER_STATUS status;
  int wantRestart;
  time_t lastStatusChangeTime;
  time_t idleSince;
  time_t commandTime;
  /** increment when attached to user or to ACTIVE cards (not when attached
   * to inactive cards!) */
  GWEN_TYPE_UINT32 usageCount;

  GWEN_TYPE_UINT32 currentRequestId;
  GWEN_TYPE_UINT32 count;
  int isAvailable;
  GWEN_TYPE_UINT32 busId;
  GWEN_TYPE_UINT32 deviceId;

  LC_REQUEST_LIST *requests;
};


GWEN_TYPE_UINT32 LC_Reader_GetNextCount(LC_READER *r);



#endif /* CHIPCARD_SERVER_READER_P_H */


