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


#ifndef CHIPCARD_SERVER_SERVICE_H
#define CHIPCARD_SERVER_SERVICE_H

/** service provided by a client */
#define LC_SERVICE_FLAGS_CLIENT   0x00000001
#define LC_SERVICE_FLAGS_AUTOLOAD 0x00000002
#define LC_SERVICE_FLAGS_SILENT   0x00000004


typedef struct LC_SERVICE LC_SERVICE;

#include <chipcard2-server/server/request.h>

#include <gwenhywfar/process.h>
#include <gwenhywfar/db.h>
#include <gwenhywfar/misc.h>

#include <time.h>


GWEN_LIST_FUNCTION_DEFS(LC_SERVICE, LC_Service);


typedef enum {
  LC_ServiceStatusDown=0,
  LC_ServiceStatusStarted,
  LC_ServiceStatusUp,
  LC_ServiceStatusSilentRunning,
  LC_ServiceStatusStopping,
  LC_ServiceStatusAborted,
  LC_ServiceStatusDisabled,
  LC_ServiceStatusUnknown=999
} LC_SERVICE_STATUS;



LC_SERVICE *LC_Service_new();
LC_SERVICE *LC_Service_FromDb(GWEN_DB_NODE *db);
void LC_Service_free(LC_SERVICE *as);

void LC_Service_ToDb(const LC_SERVICE *as, GWEN_DB_NODE *db);

const char *LC_Service_GetServiceDataDir(const LC_SERVICE *as);
void LC_Service_SetServiceDataDir(LC_SERVICE *as, const char *s);

const char *LC_Service_GetServiceName(const LC_SERVICE *as);
void LC_Service_SetServiceName(LC_SERVICE *as, const char *s);

const char *LC_Service_GetLogFile(const LC_SERVICE *as);
void LC_Service_SetLogFile(LC_SERVICE *as, const char *s);

GWEN_TYPE_UINT32 LC_Service_GetServiceId(const LC_SERVICE *as);

GWEN_PROCESS *LC_Service_GetProcess(const LC_SERVICE *as);
void LC_Service_SetProcess(LC_SERVICE *as, GWEN_PROCESS *p);

LC_SERVICE_STATUS LC_Service_GetStatus(const LC_SERVICE *as);
void LC_Service_SetStatus(LC_SERVICE *as, LC_SERVICE_STATUS st);

time_t LC_Service_GetLastStatusChangeTime(const LC_SERVICE *as);
time_t LC_Service_GetIdleSince(const LC_SERVICE *as);
time_t LC_Service_GetCommandTime(const LC_SERVICE *as);

GWEN_TYPE_UINT32 LC_Service_GetActiveClientsCount(const LC_SERVICE *as);
void LC_Service_IncActiveClientsCount(LC_SERVICE *as);
void LC_Service_DecActiveClientsCount(LC_SERVICE *as);

GWEN_TYPE_UINT32 LC_Service_GetCurrentRequestId(const LC_SERVICE *as);
void LC_Service_SetCurrentRequestId(LC_SERVICE *as,
                                    GWEN_TYPE_UINT32 rid);


GWEN_TYPE_UINT32 LC_Service_GetIpcId(const LC_SERVICE *as);
void LC_Service_SetIpcId(LC_SERVICE *as, GWEN_TYPE_UINT32 id);

int LC_Service_HasNextRequest(const LC_SERVICE *as);
LC_REQUEST *LC_Service_GetNextRequest(LC_SERVICE *as);
void LC_Service_AddRequest(LC_SERVICE *as, LC_REQUEST *rq);


GWEN_TYPE_UINT32 LC_Service_GetFlags(const LC_SERVICE *as);
void LC_Service_SetFlags(LC_SERVICE *as, GWEN_TYPE_UINT32 f);
void LC_Service_AddFlags(LC_SERVICE *as, GWEN_TYPE_UINT32 f);
void LC_Service_DelFlags(LC_SERVICE *as, GWEN_TYPE_UINT32 f);


#endif /* CHIPCARD_SERVER_SERVICE_H */


