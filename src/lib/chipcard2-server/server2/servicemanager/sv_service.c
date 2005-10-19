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


#include "sv_service_p.h"
#include <gwenhywfar/debug.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>


GWEN_LIST_FUNCTIONS(LCSV_SERVICE, LCSV_Service)


static GWEN_TYPE_UINT32 LCSV_Service__LastId=0;


LCSV_SERVICE *LCSV_Service_new() {
  LCSV_SERVICE *sv;

  GWEN_NEW_OBJECT(LCSV_SERVICE, sv);
  DBG_MEM_INC("LCSV_SERVICE", 0);
  GWEN_LIST_INIT(LCSV_SERVICE, sv);

  sv->idleSince=time(0);

  /* assign unique id */
  if (LCSV_Service__LastId==0)
    LCSV_Service__LastId=time(0);
  sv->serviceId=++LCSV_Service__LastId;

  return sv;
}



void LCSV_Service_free(LCSV_SERVICE *sv) {
  if (sv) {
    GWEN_LIST_FINI(LCSV_SERVICE, sv);
    free(sv->serviceType);
    free(sv->serviceName);
    free(sv->dataDir);
    free(sv->logFile);
    GWEN_Process_free(sv->process);
    DBG_MEM_DEC("LCSV_SERVICE");
    GWEN_FREE_OBJECT(sv);
  }
}



LCSV_SERVICE *LCSV_Service_fromDb(GWEN_DB_NODE *db) {
  LCSV_SERVICE *sv;
  const char *p;

  GWEN_NEW_OBJECT(LCSV_SERVICE, sv);
  DBG_MEM_INC("LCSV_SERVICE", 0);
  GWEN_LIST_INIT(LCSV_SERVICE, sv);

  /* assign unique id */
  if (LCSV_Service__LastId==0)
    LCSV_Service__LastId=time(0);
  sv->serviceId=++LCSV_Service__LastId;

  p=GWEN_DB_GetCharValue(db, "serviceType", 0, 0);
  if (p)
    sv->serviceType=strdup(p);

  p=GWEN_DB_GetCharValue(db, "serviceName", 0, 0);
  if (p)
    sv->serviceName=strdup(p);

  p=GWEN_DB_GetCharValue(db, "dataDir", 0, 0);
  if (p)
    sv->dataDir=strdup(p);

  p=GWEN_DB_GetCharValue(db, "logFile", 0, 0);
  if (p)
    sv->logFile=strdup(p);

  sv->flags=(LC_ServiceFlags_fromDb(db, "flags") &
             ~LC_SERVICE_FLAGS_RUNTIME_MASK);

  sv->idleSince=time(0);

  return sv;
}



void LCSV_Service_toDb(const LCSV_SERVICE *sv, GWEN_DB_NODE *db) {
  assert(sv);
  assert(db);

  if (sv->serviceType)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "serviceType", sv->serviceType);

  if (sv->serviceName)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "serviceName", sv->serviceName);

  if (sv->dataDir)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "dataDir", sv->dataDir);

  if (sv->logFile)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "logFile", sv->logFile);

  /* store driver flags */
  LC_ServiceFlags_toDb(db, "flags",
                       sv->flags & ~LC_SERVICE_FLAGS_RUNTIME_MASK);

}



const char *LCSV_Service_GetServiceType(const LCSV_SERVICE *sv) {
  assert(sv);
  return sv->serviceType;
}



void LCSV_Service_SetServiceType(LCSV_SERVICE *sv, const char *s) {
  assert(sv);
  free(sv->serviceType);
  if (s) sv->serviceType=strdup(s);
  else sv->serviceType=0;
}



const char *LCSV_Service_GetServiceName(const LCSV_SERVICE *sv) {
  assert(sv);
  return sv->serviceName;
}



void LCSV_Service_SetServiceName(LCSV_SERVICE *sv, const char *s) {
  assert(sv);
  free(sv->serviceName);
  if (s) sv->serviceName=strdup(s);
  else sv->serviceName=0;
}



const char *LCSV_Service_GetLogFile(const LCSV_SERVICE *sv) {
  assert(sv);
  return sv->logFile;
}



void LCSV_Service_SetLogFile(LCSV_SERVICE *sv, const char *s) {
  assert(sv);
  free(sv->logFile);
  if (s) sv->logFile=strdup(s);
  else sv->logFile=0;
}



const char *LCSV_Service_GetDataDir(const LCSV_SERVICE *sv) {
  assert(sv);
  return sv->dataDir;
}



void LCSV_Service_SetDataDir(LCSV_SERVICE *sv, const char *s) {
  assert(sv);
  free(sv->dataDir);
  if (s) sv->dataDir=strdup(s);
  else sv->dataDir=0;
}




GWEN_TYPE_UINT32 LCSV_Service_GetServiceId(const LCSV_SERVICE *sv) {
  assert(sv);
  return sv->serviceId;
}



GWEN_TYPE_UINT32 LCSV_Service_GetFlags(const LCSV_SERVICE *sv) {
  assert(sv);
  return sv->flags;
}



void LCSV_Service_SetFlags(LCSV_SERVICE *sv, GWEN_TYPE_UINT32 fl) {
  assert(sv);
  sv->flags=fl;
}



void LCSV_Service_AddFlags(LCSV_SERVICE *sv, GWEN_TYPE_UINT32 fl) {
  assert(sv);
  sv->flags|=fl;
}



void LCSV_Service_SubFlags(LCSV_SERVICE *sv, GWEN_TYPE_UINT32 fl) {
  assert(sv);
  sv->flags&=~fl;
}




GWEN_PROCESS *LCSV_Service_GetProcess(const LCSV_SERVICE *sv) {
  assert(sv);
  return sv->process;
}



void LCSV_Service_SetProcess(LCSV_SERVICE *sv, GWEN_PROCESS *p) {
  assert(sv);
  GWEN_Process_free(sv->process);
  sv->process=p;
}



LC_SERVICE_STATUS LCSV_Service_GetStatus(const LCSV_SERVICE *sv) {
  assert(sv);
  return sv->status;
}



void LCSV_Service_SetStatus(LCSV_SERVICE *sv, LC_SERVICE_STATUS st) {
  assert(sv);
  if (sv->status!=st) {
    sv->lastStatusChangeTime=time(0);
    sv->status=st;
  }
}



GWEN_TYPE_UINT32 LCSV_Service_GetIpcId(const LCSV_SERVICE *sv) {
  assert(sv);
  return sv->ipcId;
}



void LCSV_Service_SetIpcId(LCSV_SERVICE *sv, GWEN_TYPE_UINT32 id) {
  assert(sv);
  sv->ipcId=id;
}



GWEN_TYPE_UINT32 LCSV_Service_GetInterestedClients(const LCSV_SERVICE *sv) {
  assert(sv);
  return sv->interestedClients;
}



void LCSV_Service_IncInterestedClients(LCSV_SERVICE *sv) {
  assert(sv);
  sv->interestedClients++;
  sv->idleSince=(time_t)0;
}



void LCSV_Service_DecInterestedClients(LCSV_SERVICE *sv) {
  assert(sv);
  assert(sv->interestedClients);
  sv->interestedClients--;
  if (sv->interestedClients==0)
    sv->idleSince=time(0);
}



GWEN_TYPE_UINT32 LCSV_Service_GetActiveClients(const LCSV_SERVICE *sv) {
  assert(sv);
  return sv->activeClients;
}



void LCSV_Service_IncActiveClients(LCSV_SERVICE *sv) {
  assert(sv);
  sv->activeClients++;
}



void LCSV_Service_DecActiveClients(LCSV_SERVICE *sv) {
  assert(sv);
  assert(sv->activeClients);
  sv->activeClients--;
}



time_t LCSV_Service_GetLastStatusChangeTime(const LCSV_SERVICE *sv){
  assert(sv);
  return sv->lastStatusChangeTime;
}



time_t LCSV_Service_GetIdleSince(const LCSV_SERVICE *sv){
  assert(sv);
  return sv->idleSince;
}



GWEN_TYPE_UINT32 LCSV_Service_GetCurrentRequestId(const LCSV_SERVICE *sv){
  assert(sv);
  return sv->currentRequestId;
}



void LCSV_Service_SetCurrentRequestId(LCSV_SERVICE *sv, GWEN_TYPE_UINT32 rid){
  assert(sv);
  sv->currentRequestId=rid;
}



void LCSV_Service_SetTimeout(LCSV_SERVICE *sv, int secs) {
  assert(sv);
  DBG_ERROR(0, "Setting service timeout to %d", secs);
  if (secs==0)
    sv->timeout=0;
  else {
    time_t t;

    t=time(0);
    t+=secs;
    sv->timeout=t;
  }
}



int LCSV_Service_CheckTimeout(const LCSV_SERVICE *sv) {
  assert(sv);
  if (sv->timeout==0)
    return -1;
  else {
    time_t t;

    t=time(0);
    return (difftime(t, sv->timeout)>0);
  }
}












