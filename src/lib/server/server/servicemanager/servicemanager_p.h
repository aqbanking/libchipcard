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


#ifndef CHIPCARD_SERVER_SV_SERVICEMANAGER_P_H
#define CHIPCARD_SERVER_SV_SERVICEMANAGER_P_H

#include <gwenhywfar/types.h>
#include <gwenhywfar/process.h>
#include <gwenhywfar/misc.h>

#include "servicemanager_l.h"


#define LCSV_SERVICEMANAGER_DEF_SERVICE_START_DELAY     1
#define LCSV_SERVICEMANAGER_DEF_SERVICE_START_TIMEOUT   30
#define LCSV_SERVICEMANAGER_DEF_SERVICE_STOP_TIMEOUT    10
#define LCSV_SERVICEMANAGER_DEF_SERVICE_IDLE_TIMEOUT    30
#define LCSV_SERVICEMANAGER_DEF_SERVICE_RESTART_TIME    10


struct LCSV_SERVICEMANAGER {
  LCS_SERVER *server;
  GWEN_IPCMANAGER *ipcManager;

  unsigned int serviceStartDelay;
  unsigned int serviceStartTimeout;
  unsigned int serviceStopTimeout;

  unsigned int serviceIdleTimeout;
  unsigned int serviceRestartTime;

  char *addrTypeForServices;
  char *addrAddrForServices;
  int addrPortForServices;

  int allowClientService;

  LCSV_SERVICE_LIST *services;
};


static
void LCSV_ServiceManager_AbandonService(LCSV_SERVICEMANAGER *svm,
                                        LCSV_SERVICE *sv,
                                        LC_SERVICE_STATUS newSt,
                                        const char *reason);

static
GWEN_TYPE_UINT32 LCSV_ServiceManager_SendStopService(LCSV_SERVICEMANAGER *svm,
                                                     const LCSV_SERVICE *sv);

static
int LCSV_ServiceManager_StartService(LCSV_SERVICEMANAGER *svm,
                                     LCSV_SERVICE *sv);

static
int LCSV_ServiceManager_CheckService(LCSV_SERVICEMANAGER *svm,
                                     LCSV_SERVICE *sv);

static
int LCSV_ServiceManager_CheckServices(LCSV_SERVICEMANAGER *sv);


static
int LCSV_ServiceManager_HandleServiceReady(LCSV_SERVICEMANAGER *svm,
                                           GWEN_TYPE_UINT32 rid,
                                           GWEN_DB_NODE *dbReq);



#endif /* CHIPCARD_SERVER_SV_SERVICEMANAGER_P_H */

