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


#ifndef CHIPCARD_SERVER_SV_SERVICEMANAGER_L_H
#define CHIPCARD_SERVER_SV_SERVICEMANAGER_L_H

#include <gwenhywfar/types.h>
#include <gwenhywfar/process.h>
#include <gwenhywfar/misc.h>


typedef struct LCSV_SERVICEMANAGER LCSV_SERVICEMANAGER;


#include "sv_service_l.h"
#include "server_l.h"


LCSV_SERVICEMANAGER *LCSV_ServiceManager_new(LCS_SERVER *server);
void LCSV_ServiceManager_free(LCSV_SERVICEMANAGER *svm);

int LCSV_ServiceManager_Init(LCSV_SERVICEMANAGER *svm, GWEN_DB_NODE *db);
int LCSV_ServiceManager_Fini(LCSV_SERVICEMANAGER *svm, GWEN_DB_NODE *db);

int LCSV_ServiceManager_Work(LCSV_SERVICEMANAGER *svm);

int LCSV_ServiceManager_ListServices(LCSV_SERVICEMANAGER *svm);

int LCSV_ServiceManager_HandleRequest(LCSV_SERVICEMANAGER *svm,
                                      uint32_t rid,
                                      const char *name,
                                      GWEN_DB_NODE *dbReq);

void LCSV_ServiceManager_ConnectionDown(LCSV_SERVICEMANAGER *svm,
                                        uint32_t ipcId);

uint32_t LCSV_ServiceManager_SendCommand(LCSV_SERVICEMANAGER *svm,
                                                 uint32_t serviceId,
                                                 GWEN_DB_NODE *dbCmd);

/**
 * For every matching service a GWEN_DB_NODE is added to the given
 * node describing the service.
 * This function is to be used by the ClientManager to let a client
 * choose a service.
 */
int LCSV_ServiceManager_GetMatchingServices(LCSV_SERVICEMANAGER *svm,
                                            const char *serviceType,
                                            const char *serviceName,
                                            GWEN_DB_NODE *dbData);

#endif /* CHIPCARD_SERVER_SV_SERVICEMANAGER_L_H */

