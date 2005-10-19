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


#ifndef CHIPCARD_SERVER_SV_SERVICE_P_H
#define CHIPCARD_SERVER_SV_SERVICE_P_H


#include "sv_service_l.h"



struct LCSV_SERVICE {
  GWEN_LIST_ELEMENT(LCSV_SERVICE);

  /* variables from config file */
  char *serviceType;
  char *serviceName;
  char *logFile;
  char *dataDir;

  /* runtime variables */
  GWEN_TYPE_UINT32 serviceId;
  GWEN_TYPE_UINT32 flags;
  GWEN_PROCESS *process;
  LC_SERVICE_STATUS status;
  time_t lastStatusChangeTime;
  time_t idleSince;

  GWEN_TYPE_UINT32 currentRequestId;

  GWEN_TYPE_UINT32 ipcId;

  GWEN_TYPE_UINT32 interestedClients;
  GWEN_TYPE_UINT32 activeClients;

  time_t timeout;

};



#endif /* CHIPCARD_SERVER_SV_SERVICE_P_H */

