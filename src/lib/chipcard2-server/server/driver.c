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


#include "driver_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_LIST_FUNCTIONS(LC_DRIVER, LC_Driver);


static GWEN_TYPE_UINT32 LC_Driver_LastId=0;



LC_DRIVER *LC_Driver_new(){
  LC_DRIVER *d;

  GWEN_NEW_OBJECT(LC_DRIVER, d);
  GWEN_LIST_INIT(LC_DRIVER, d);

  d->idleSince=time(0);
  d->driverVars=GWEN_DB_Group_new("vars");

  /* assign unique id */
  if (LC_Driver_LastId==0)
    LC_Driver_LastId=time(0);
  d->driverId=++LC_Driver_LastId;
  d->maxReaders=1;

  d->firstNewPort=100;

  return d;
}



LC_DRIVER *LC_Driver_FromDb(GWEN_DB_NODE *db){
  LC_DRIVER *d;
  const char *p;
  GWEN_DB_NODE *dbT;

  GWEN_NEW_OBJECT(LC_DRIVER, d);
  GWEN_LIST_INIT(LC_DRIVER, d);

  /* assign unique id */
  if (LC_Driver_LastId==0)
    LC_Driver_LastId=time(0);
  d->driverId=++LC_Driver_LastId;

  /* get driver vars */
  dbT=GWEN_DB_GetGroup(db, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "vars");
  if (dbT) {
    d->driverVars=GWEN_DB_Group_dup(dbT);
  }
  else
    d->driverVars=GWEN_DB_Group_new("vars");

  d->firstNewPort=GWEN_DB_GetIntValue(d->driverVars, "firstNewPort", 0, 1000);
  d->autoPortOffset=GWEN_DB_GetIntValue(d->driverVars,
                                        "autoPortOffset", 0, -1);

  d->maxReaders=GWEN_DB_GetIntValue(db, "maxReaders", 0, 1);

  p=GWEN_DB_GetCharValue(db, "driverType", 0, 0);
  if (p)
    d->driverType=strdup(p);

  p=GWEN_DB_GetCharValue(db, "driverName", 0, 0);
  if (p)
    d->driverName=strdup(p);

  p=GWEN_DB_GetCharValue(db, "driverDataDir", 0, 0);
  if (p)
    d->driverDataDir=strdup(p);

  p=GWEN_DB_GetCharValue(db, "customerId", 0, 0);
  if (p)
    d->customerId=strdup(p);

  p=GWEN_DB_GetCharValue(db, "libraryFile", 0, 0);
  if (p)
    d->libraryFile=strdup(p);

  p=GWEN_DB_GetCharValue(db, "logFile", 0, 0);
  if (p)
    d->logFile=strdup(p);

  d->idleSince=time(0);

  return d;
}



void LC_Driver_free(LC_DRIVER *d){
  if (d) {
    GWEN_LIST_FINI(LC_DRIVER, d);
    GWEN_DB_Group_free(d->driverVars);
    free(d->driverType);
    free(d->driverName);
    free(d->driverDataDir);
    free(d->customerId);
    free(d->libraryFile);
    GWEN_FREE_OBJECT(d);
  }
}



void LC_Driver_ToDb(const LC_DRIVER *d, GWEN_DB_NODE *db){
  assert(d);
  assert(db);

  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "maxReaders", d->maxReaders);
  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "firstNewPort", d->firstNewPort);
  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "autoPortOffset", d->autoPortOffset);

  if (d->driverType)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "driverType", d->driverType);

  if (d->driverName)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "driverName", d->driverName);

  if (d->driverDataDir)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "driverDataDir", d->driverDataDir);

  if (d->customerId)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "customerId", d->customerId);

  if (d->libraryFile)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "libraryFile", d->libraryFile);
  if (d->logFile)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "logFile", d->logFile);
  /* set driver vars */
  if (d->driverVars) {
    GWEN_DB_NODE *dbT;

    dbT=GWEN_DB_GetGroup(db, GWEN_DB_FLAGS_OVERWRITE_GROUPS, "vars");
    assert(dbT);
    GWEN_DB_AddGroupChildren(dbT, d->driverVars);
  }
}



int LC_Driver_GetMaxReaders(const LC_DRIVER *d){
  assert(d);
  return d->maxReaders;
}



void LC_Driver_SetMaxReaders(LC_DRIVER *d, int maxReaders){
  assert(d);
  d->maxReaders=maxReaders;
}



int LC_Driver_GetFirstNewPort(const LC_DRIVER *d){
  assert(d);
  return d->firstNewPort;
}



int LC_Driver_GetAutoPortOffset(const LC_DRIVER *d){
  assert(d);
  return d->autoPortOffset;
}



const char *LC_Driver_GetDriverType(const LC_DRIVER *d){
  assert(d);
  return d->driverType;
}



void LC_Driver_SetDriverType(LC_DRIVER *d, const char *s){
  assert(d);
  assert(s);
  free(d->driverType);
  d->driverType=strdup(s);
}



const char *LC_Driver_GetDriverName(const LC_DRIVER *d){
  assert(d);
  return d->driverName;
}



void LC_Driver_SetDriverName(LC_DRIVER *d, const char *s){
  assert(d);
  assert(s);
  free(d->driverName);
  d->driverName=strdup(s);
}



const char *LC_Driver_GetDriverDataDir(const LC_DRIVER *d){
  assert(d);
  return d->driverDataDir;
}



void LC_Driver_SetDriverDataDir(LC_DRIVER *d, const char *s){
  assert(d);
  assert(s);
  free(d->driverDataDir);
  d->driverDataDir=strdup(s);
}



const char *LC_Driver_GetCustomerId(const LC_DRIVER *d){
  assert(d);
  return d->customerId;
}



void LC_Driver_SetCustomerId(LC_DRIVER *d, const char *s){
  assert(d);
  assert(s);
  free(d->customerId);
  d->customerId=strdup(s);
}



const char *LC_Driver_GetLibraryFile(const LC_DRIVER *d){
  assert(d);
  return d->libraryFile;
}



void LC_Driver_SetLibraryFile(LC_DRIVER *d, const char *s){
  assert(d);
  assert(s);
  free(d->libraryFile);
  d->libraryFile=strdup(s);
}



const char *LC_Driver_GetLogFile(const LC_DRIVER *d){
  assert(d);
  return d->logFile;
}



void LC_Driver_SetLogFile(LC_DRIVER *d, const char *s){
  assert(d);
  assert(s);
  free(d->logFile);
  d->logFile=strdup(s);
}



GWEN_TYPE_UINT32 LC_Driver_GetDriverId(const LC_DRIVER *d){
  assert(d);
  return d->driverId;
}



GWEN_PROCESS *LC_Driver_GetProcess(const LC_DRIVER *d){
  assert(d);
  return d->process;
}



void LC_Driver_SetProcess(LC_DRIVER *d, GWEN_PROCESS *p){
  assert(d);
  GWEN_Process_free(d->process);
  d->process=p;
}



LC_DRIVER_STATUS LC_Driver_GetStatus(const LC_DRIVER *d){
  assert(d);
  return d->status;
}



void LC_Driver_SetStatus(LC_DRIVER *d, LC_DRIVER_STATUS st){
  assert(d);
  if (d->status!=st) {
    d->status=st;
    d->lastStatusChangeTime=time(0);
    if (d->status!=LC_DriverStatusUp)
      d->pendingCommandCount=0;
  }
}



time_t LC_Driver_GetLastStatusChangeTime(const LC_DRIVER *d){
  assert(d);
  return d->lastStatusChangeTime;
}



time_t LC_Driver_GetIdleSince(const LC_DRIVER *d){
  assert(d);
  return d->idleSince;
}



GWEN_TYPE_UINT32 LC_Driver_GetActiveReadersCount(const LC_DRIVER *d){
  assert(d);
  return d->activeReadersCount;
}



void LC_Driver_IncActiveReadersCount(LC_DRIVER *d){
  assert(d);
  d->activeReadersCount++;
  d->idleSince=(time_t)0;
}



void LC_Driver_DecActiveReadersCount(LC_DRIVER *d){
  assert(d);
  if (d->activeReadersCount) {
    if (--(d->activeReadersCount)==0) {
      d->idleSince=time(0);
    }
  }
}



GWEN_TYPE_UINT32 LC_Driver_GetIpcId(const LC_DRIVER *d){
  assert(d);
  return d->ipcId;
}



void LC_Driver_SetIpcId(LC_DRIVER *d, GWEN_TYPE_UINT32 id){
  assert(d);
  d->ipcId=id;
}



void LC_Driver_ResetActiveReadersCount(LC_DRIVER *d){
  assert(d);
  d->activeReadersCount=0;
}



time_t LC_Driver_GetPingTime(const LC_DRIVER *d){
  assert(d);
  return d->pingTime;
}



void LC_Driver_SetPingTime(LC_DRIVER *d, time_t t){
  assert(d);
  d->pingTime=t;
}



time_t LC_Driver_GetPongTime(const LC_DRIVER *d){
  assert(d);
  return d->pongTime;
}



void LC_Driver_SetPongTime(LC_DRIVER *d, time_t t){
  assert(d);
  d->pongTime=t;
}



GWEN_DB_NODE *LC_Driver_GetDriverVars(const LC_DRIVER *d){
  assert(d);
  return d->driverVars;
}



int LC_Driver_GetPendingCommandCount(const LC_DRIVER *d){
  assert(d);
  return d->pendingCommandCount;
}



void LC_Driver_IncPendingCommandCount(LC_DRIVER *d){
  assert(d);
  d->pendingCommandCount++;
}



void LC_Driver_DecPendingCommandCount(LC_DRIVER *d){
  assert(d);
  if (d->pendingCommandCount)
    d->pendingCommandCount--;
  else {
    DBG_WARN(0, "Pending command counter already at ZERO");
  }
}



void LC_Driver_Dump(const LC_DRIVER *d, FILE *f, int indent) {
  int i;

  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Driver\n");
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "--------------------------\n");
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Type : %s\n", d->driverType);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Name : %s\n", d->driverName);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "DataDir : %s\n", d->driverDataDir);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "LogFile : %s\n", d->logFile);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Library : %s\n", d->libraryFile);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Customer : %s\n", d->customerId);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "MaxReaders : %d\n", d->maxReaders);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Vars:\n");
  GWEN_DB_Dump(d->driverVars, f, indent+2);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Id : %04x\n", d->driverId);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "IPC-Id : %04x\n", d->ipcId);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Status : %d\n", d->status);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "ActiveReaders : %d\n", d->activeReadersCount);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "PendingCommands : %d\n", d->pendingCommandCount);
}














