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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "service_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_LIST_FUNCTIONS(LC_SERVICE, LC_Service);


static GWEN_TYPE_UINT32 LC_Service_LastId=0;



LC_SERVICE *LC_Service_new(){
  LC_SERVICE *as;

  GWEN_NEW_OBJECT(LC_SERVICE, as);
  DBG_MEM_INC("LC_SERVICE", 0);
  GWEN_LIST_INIT(LC_SERVICE, as);

  as->idleSince=time(0);
  as->requests=LC_Request_List_new();

  /* assign unique id */
  if (LC_Service_LastId==0)
    LC_Service_LastId=time(0);
  as->serviceId=++LC_Service_LastId;

  return as;
}



LC_SERVICE *LC_Service_FromDb(GWEN_DB_NODE *db){
  LC_SERVICE *as;
  const char *p;
  GWEN_TYPE_UINT32 flags;
  int i;

  GWEN_NEW_OBJECT(LC_SERVICE, as);
  DBG_MEM_INC("LC_SERVICE", 0);
  GWEN_LIST_INIT(LC_SERVICE, as);

  /* assign unique id */
  if (LC_Service_LastId==0)
    LC_Service_LastId=time(0);
  as->serviceId=++LC_Service_LastId;
  as->requests=LC_Request_List_new();

  p=GWEN_DB_GetCharValue(db, "serviceName", 0, 0);
  if (p)
    as->serviceName=strdup(p);

  p=GWEN_DB_GetCharValue(db, "serviceDataDir", 0, 0);
  if (p)
    as->serviceDataDir=strdup(p);

  p=GWEN_DB_GetCharValue(db, "logFile", 0, 0);
  if (p)
    as->logFile=strdup(p);

  as->idleSince=time(0);

  flags=0;
  for (i=0; ; i++) {
    p=GWEN_DB_GetCharValue(db, "flags", i, 0);
    if (!p)
      break;
    if (strcasecmp(p, "autoload")==0)
      flags|=LC_SERVICE_FLAGS_AUTOLOAD;
    else if (strcasecmp(p, "silent")==0)
      flags|=LC_SERVICE_FLAGS_SILENT;
    else {
      DBG_ERROR(0, "Unknown service flag \"%s\"", p);
    }
  } /* for */
  as->flags=flags;

  return as;
}



void LC_Service_free(LC_SERVICE *as){
  if (as) {
    DBG_MEM_DEC("LC_SERVICE");
    GWEN_LIST_FINI(LC_SERVICE, as);
    LC_Request_List_free(as->requests);
    free(as->serviceName);
    free(as->serviceDataDir);
    free(as->logFile);
    GWEN_FREE_OBJECT(as);
  }
}



void LC_Service_ToDb(const LC_SERVICE *as, GWEN_DB_NODE *db){
  assert(as);
  assert(db);

  if (as->serviceName)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "serviceName", as->serviceName);

  if (as->serviceDataDir)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "serviceDataDir", as->serviceDataDir);

  GWEN_DB_DeleteVar(db, "flags");
  if (as->flags & LC_SERVICE_FLAGS_AUTOLOAD)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "flags", "autoload");
  if (as->flags & LC_SERVICE_FLAGS_SILENT)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "flags", "silent");
}



const char *LC_Service_GetServiceName(const LC_SERVICE *as){
  assert(as);
  return as->serviceName;
}



void LC_Service_SetServiceName(LC_SERVICE *as, const char *s){
  assert(as);
  assert(s);
  free(as->serviceName);
  as->serviceName=strdup(s);
}



const char *LC_Service_GetServiceDataDir(const LC_SERVICE *as){
  assert(as);
  return as->serviceDataDir;
}



void LC_Service_SetServiceDataDir(LC_SERVICE *as, const char *s){
  assert(as);
  assert(s);
  free(as->serviceDataDir);
  as->serviceDataDir=strdup(s);
}



GWEN_TYPE_UINT32 LC_Service_GetServiceId(const LC_SERVICE *as){
  assert(as);
  return as->serviceId;
}



GWEN_PROCESS *LC_Service_GetProcess(const LC_SERVICE *as){
  assert(as);
  return as->process;
}



void LC_Service_SetProcess(LC_SERVICE *as, GWEN_PROCESS *p){
  assert(as);
  GWEN_Process_free(as->process);
  as->process=p;
}



LC_SERVICE_STATUS LC_Service_GetStatus(const LC_SERVICE *as){
  assert(as);
  return as->status;
}



void LC_Service_SetStatus(LC_SERVICE *as, LC_SERVICE_STATUS st){
  assert(as);
  if (as->status!=st) {
    as->status=st;
    as->lastStatusChangeTime=time(0);
  }
}



time_t LC_Service_GetLastStatusChangeTime(const LC_SERVICE *as){
  assert(as);
  return as->lastStatusChangeTime;
}



time_t LC_Service_GetIdleSince(const LC_SERVICE *as){
  assert(as);
  return as->idleSince;
}



GWEN_TYPE_UINT32 LC_Service_GetActiveClientsCount(const LC_SERVICE *as){
  assert(as);
  return as->activeClientsCount;
}



void LC_Service_IncActiveClientsCount(LC_SERVICE *as){
  assert(as);
  as->activeClientsCount++;
  as->idleSince=(time_t)0;
}



void LC_Service_DecActiveClientsCount(LC_SERVICE *as){
  assert(as);
  if (as->activeClientsCount) {
    if (--(as->activeClientsCount)==0) {
      as->idleSince=time(0);
    }
  }
}



GWEN_TYPE_UINT32 LC_Service_GetIpcId(const LC_SERVICE *as){
  assert(as);
  return as->ipcId;
}



void LC_Service_SetIpcId(LC_SERVICE *as, GWEN_TYPE_UINT32 id){
  assert(as);
  as->ipcId=id;
}



LC_REQUEST *LC_Service_GetNextRequest(LC_SERVICE *as){
  LC_REQUEST *rq;

  assert(as);
  rq=LC_Request_List_First(as->requests);
  if (rq) {
    LC_Request_List_Del(rq);
    return rq;
  }
  return 0;
}



void LC_Service_AddRequest(LC_SERVICE *as, LC_REQUEST *rq){
  assert(as);
  assert(rq);
  LC_Request_List_Add(rq, as->requests);
}



int LC_Service_HasNextRequest(const LC_SERVICE *as){
  assert(as);
  return (LC_Request_List_First(as->requests)!=0);
}



const char *LC_Service_GetLogFile(const LC_SERVICE *as){
  assert(as);
  return as->logFile;
}



void LC_Service_SetLogFile(LC_SERVICE *as, const char *s){
  assert(as);
  free(as->logFile);
  if (s) as->logFile=strdup(s);
  else as->logFile=0;
}



GWEN_TYPE_UINT32 LC_Service_GetCurrentRequestId(const LC_SERVICE *as){
  assert(as);
  return as->currentRequestId;
}



void LC_Service_SetCurrentRequestId(LC_SERVICE *as, GWEN_TYPE_UINT32 rid){
  assert(as);
  if (rid!=as->currentRequestId) {
    as->currentRequestId=rid;
    if (rid!=0)
      as->commandTime=time(0);
  }
}



time_t LC_Service_GetCommandTime(const LC_SERVICE *as){
  assert(as);
  return as->commandTime;
}



GWEN_TYPE_UINT32 LC_Service_GetFlags(const LC_SERVICE *as){
  assert(as);
  return as->flags;
}



void LC_Service_SetFlags(LC_SERVICE *as, GWEN_TYPE_UINT32 f){
  assert(as);
  as->flags=f;
}



void LC_Service_AddFlags(LC_SERVICE *as, GWEN_TYPE_UINT32 f){
  assert(as);
  as->flags|=f;
}



void LC_Service_DelFlags(LC_SERVICE *as, GWEN_TYPE_UINT32 f){
  assert(as);
  as->flags&=~f;
}











