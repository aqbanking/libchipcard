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


#ifndef CHIPCARD_SERVER_SERVICE_P_H
#define CHIPCARD_SERVER_SERVICE_P_H


#include <chipcard2-server/server/service.h>


struct LC_SERVICE {
  GWEN_LIST_ELEMENT(LC_SERVICE);

  /* variables from config file */
  char *serviceName;
  char *serviceDataDir;
  char *logFile;

  /* runtime variables */
  GWEN_TYPE_UINT32 serviceId;
  GWEN_TYPE_UINT32 flags;
  GWEN_PROCESS *process;
  LC_SERVICE_STATUS status;
  time_t lastStatusChangeTime;
  time_t idleSince;
  GWEN_TYPE_UINT32 activeClientsCount;
  LC_REQUEST_LIST *requests;

  GWEN_TYPE_UINT32 currentRequestId;
  time_t commandTime;

  GWEN_TYPE_UINT32 ipcId;
};



#endif /* CHIPCARD_SERVER_SERVICE_P_H */


