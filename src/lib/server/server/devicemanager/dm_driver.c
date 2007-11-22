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


#include "dm_driver_p.h"
#include <gwenhywfar/debug.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_LIST_FUNCTIONS(LCDM_DRIVER, LCDM_Driver)


static uint32_t LCDM_Driver_LastId=0;




LCDM_DRIVER *LCDM_Driver_new(){
  LCDM_DRIVER *d;

  GWEN_NEW_OBJECT(LCDM_DRIVER, d);
  DBG_MEM_INC("LCDM_DRIVER", 0);
  GWEN_LIST_INIT(LCDM_DRIVER, d);

  d->idleSince=time(0);
  d->driverVars=GWEN_DB_Group_new("vars");

  /* assign unique id */
  if (LCDM_Driver_LastId==0)
    LCDM_Driver_LastId=time(0);
  d->driverId=++LCDM_Driver_LastId;
  d->maxReaders=1;

  return d;
}



LCDM_DRIVER *LCDM_Driver_fromDb(GWEN_DB_NODE *db){
  LCDM_DRIVER *d;
  const char *p;
  GWEN_DB_NODE *dbT;

  GWEN_NEW_OBJECT(LCDM_DRIVER, d);
  DBG_MEM_INC("LCDM_DRIVER", 0);
  GWEN_LIST_INIT(LCDM_DRIVER, d);

  /* assign unique id */
  if (LCDM_Driver_LastId==0)
    LCDM_Driver_LastId=time(0);
  d->driverId=++LCDM_Driver_LastId;

  /* get driver vars */
  dbT=GWEN_DB_GetGroup(db, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "vars");
  if (dbT) {
    d->driverVars=GWEN_DB_Group_dup(dbT);
  }
  else
    d->driverVars=GWEN_DB_Group_new("vars");

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

  d->driverFlags=(LC_DriverFlags_fromDb(db, "flags") &
                  ~LC_DRIVER_FLAGS_RUNTIME_MASK);


  d->idleSince=time(0);

  return d;
}



void LCDM_Driver_free(LCDM_DRIVER *d){
  if (d) {
    DBG_MEM_DEC("LCDM_DRIVER");
    GWEN_LIST_FINI(LCDM_DRIVER, d);
    GWEN_DB_Group_free(d->driverVars);
    free(d->driverType);
    free(d->driverName);
    free(d->driverDataDir);
    free(d->logFile);
    free(d->customerId);
    free(d->libraryFile);
    GWEN_Process_free(d->process);
    GWEN_FREE_OBJECT(d);
  }
}



void LCDM_Driver_toDb(const LCDM_DRIVER *d, GWEN_DB_NODE *db){
  assert(d);
  assert(db);

  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "maxReaders", d->maxReaders);

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

  /* store driver flags */
  LC_DriverFlags_toDb(db, "flags",
                      d->driverFlags & ~LC_DRIVER_FLAGS_RUNTIME_MASK);

}



int LCDM_Driver_GetMaxReaders(const LCDM_DRIVER *d){
  assert(d);
  return d->maxReaders;
}



void LCDM_Driver_SetMaxReaders(LCDM_DRIVER *d, int maxReaders){
  assert(d);
  d->maxReaders=maxReaders;
}



const char *LCDM_Driver_GetDriverType(const LCDM_DRIVER *d){
  assert(d);
  return d->driverType;
}



void LCDM_Driver_SetDriverType(LCDM_DRIVER *d, const char *s){
  assert(d);
  assert(s);
  free(d->driverType);
  d->driverType=strdup(s);
}



const char *LCDM_Driver_GetDriverName(const LCDM_DRIVER *d){
  assert(d);
  return d->driverName;
}



void LCDM_Driver_SetDriverName(LCDM_DRIVER *d, const char *s){
  assert(d);
  assert(s);
  free(d->driverName);
  d->driverName=strdup(s);
}



const char *LCDM_Driver_GetDriverDataDir(const LCDM_DRIVER *d){
  assert(d);
  return d->driverDataDir;
}



void LCDM_Driver_SetDriverDataDir(LCDM_DRIVER *d, const char *s){
  assert(d);
  assert(s);
  free(d->driverDataDir);
  d->driverDataDir=strdup(s);
}



const char *LCDM_Driver_GetCustomerId(const LCDM_DRIVER *d){
  assert(d);
  return d->customerId;
}



void LCDM_Driver_SetCustomerId(LCDM_DRIVER *d, const char *s){
  assert(d);
  assert(s);
  free(d->customerId);
  d->customerId=strdup(s);
}



const char *LCDM_Driver_GetLibraryFile(const LCDM_DRIVER *d){
  assert(d);
  return d->libraryFile;
}



void LCDM_Driver_SetLibraryFile(LCDM_DRIVER *d, const char *s){
  assert(d);
  assert(s);
  free(d->libraryFile);
  d->libraryFile=strdup(s);
}



const char *LCDM_Driver_GetLogFile(const LCDM_DRIVER *d){
  assert(d);
  return d->logFile;
}



void LCDM_Driver_SetLogFile(LCDM_DRIVER *d, const char *s){
  assert(d);
  assert(s);
  free(d->logFile);
  d->logFile=strdup(s);
}



uint32_t LCDM_Driver_GetDriverId(const LCDM_DRIVER *d){
  assert(d);
  return d->driverId;
}



GWEN_PROCESS *LCDM_Driver_GetProcess(const LCDM_DRIVER *d){
  assert(d);
  return d->process;
}



void LCDM_Driver_SetProcess(LCDM_DRIVER *d, GWEN_PROCESS *p){
  assert(d);
  GWEN_Process_free(d->process);
  d->process=p;
}



LC_DRIVER_STATUS LCDM_Driver_GetStatus(const LCDM_DRIVER *d){
  assert(d);
  return d->status;
}



void LCDM_Driver_SetStatus(LCDM_DRIVER *d, LC_DRIVER_STATUS st){
  assert(d);
  if (d->status!=st) {
    d->status=st;
    d->lastStatusChangeTime=time(0);
    DBG_VERBOUS(0, "Changing status of driver \"%s\" from %d to %d",
                d->driverName, d->status, st);

  }
}



time_t LCDM_Driver_GetLastStatusChangeTime(const LCDM_DRIVER *d){
  assert(d);
  return d->lastStatusChangeTime;
}



time_t LCDM_Driver_GetIdleSince(const LCDM_DRIVER *d){
  assert(d);
  return d->idleSince;
}



uint32_t LCDM_Driver_GetActiveReadersCount(const LCDM_DRIVER *d){
  assert(d);
  return d->activeReadersCount;
}



void LCDM_Driver_IncActiveReadersCount(LCDM_DRIVER *d, int count){
  assert(d);
  DBG_ERROR(0,
            "Incrementing active readers count of driver %s [%08x](%d->%d)",
            d->driverName,
            d->driverId,
            d->activeReadersCount,
            d->activeReadersCount+count);

  if (d->activeReadersCount==0) {
    DBG_INFO(0, "Some readers active, driver leaving idle mode");
  }
  d->activeReadersCount+=count;
  d->idleSince=(time_t)0;
}



void LCDM_Driver_DecActiveReadersCount(LCDM_DRIVER *d, int count){
  assert(d);
  DBG_ERROR(0,
            "Decrementing active readers count of driver %s [%08x](%d->%d)",
            d->driverName,
            d->driverId,
            d->activeReadersCount,
            d->activeReadersCount-count);
  assert(d->activeReadersCount>=count);
  d->activeReadersCount-=count;
  if (d->activeReadersCount==0) {
    DBG_INFO(0, "No readers active, driver entering idle mode");
    d->idleSince=time(0);
  }
}



uint32_t LCDM_Driver_GetAssignedReadersCount(const LCDM_DRIVER *d){
  assert(d);
  return d->assignedReaders;
}



void LCDM_Driver_ResetAssignedReadersCount(LCDM_DRIVER *d){
  assert(d);
  d->assignedReaders=0;
}



void LCDM_Driver_IncAssignedReadersCount(LCDM_DRIVER *d){
  assert(d);
  d->assignedReaders++;
}



void LCDM_Driver_DecAssignedReadersCount(LCDM_DRIVER *d){
  assert(d);
  if (d->assignedReaders)
    d->assignedReaders--;
}



uint32_t LCDM_Driver_GetIpcId(const LCDM_DRIVER *d){
  assert(d);
  return d->ipcId;
}



void LCDM_Driver_SetIpcId(LCDM_DRIVER *d, uint32_t id){
  assert(d);
  d->ipcId=id;
}



void LCDM_Driver_ResetActiveReadersCount(LCDM_DRIVER *d){
  assert(d);
  d->activeReadersCount=0;
}



time_t LCDM_Driver_GetPingTime(const LCDM_DRIVER *d){
  assert(d);
  return d->pingTime;
}



void LCDM_Driver_SetPingTime(LCDM_DRIVER *d, time_t t){
  assert(d);
  d->pingTime=t;
}



time_t LCDM_Driver_GetPongTime(const LCDM_DRIVER *d){
  assert(d);
  return d->pongTime;
}



void LCDM_Driver_SetPongTime(LCDM_DRIVER *d, time_t t){
  assert(d);
  d->pongTime=t;
}



GWEN_DB_NODE *LCDM_Driver_GetDriverVars(const LCDM_DRIVER *d){
  assert(d);
  return d->driverVars;
}



uint32_t LCDM_Driver_GetDriverFlags(const LCDM_DRIVER *d){
  assert(d);
  return d->driverFlags;
}



void LCDM_Driver_SetDriverFlags(LCDM_DRIVER *d, uint32_t fl){
  assert(d);
  d->driverFlags=fl;
}



void LCDM_Driver_AddDriverFlags(LCDM_DRIVER *d, uint32_t fl){
  assert(d);
  d->driverFlags|=fl;
}



void LCDM_Driver_SubDriverFlags(LCDM_DRIVER *d, uint32_t fl){
  assert(d);
  d->driverFlags&=~fl;
}



void LCDM_Driver_SetTimeout(LCDM_DRIVER *d, int secs) {
  if (secs==0)
    d->timeout=0;
  else {
    time_t t;
    assert(d);

    t=time(0);
    t+=secs;
    d->timeout=t;
  }
}



int LCDM_Driver_CheckTimeout(const LCDM_DRIVER *d) {
  if (d->timeout==0)
    return -1;
  else {
    time_t t;

    assert(d);
    t=time(0);
    return (difftime(t, d->timeout)>0);
  }
}



void LCDM_Driver_Dump(const LCDM_DRIVER *d, FILE *f, int indent) {
  int i;
  GWEN_DB_NODE *dbT;

  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "--------------------------\n");
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Driver\n");
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Type            : %s\n", d->driverType);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Name            : %s\n", d->driverName);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "DataDir         : %s\n", d->driverDataDir);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "LogFile         : %s\n", d->logFile);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Library         : %s\n", d->libraryFile);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Customer        : %s\n", d->customerId);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "MaxReaders      : %d\n", d->maxReaders);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Vars:\n");
  GWEN_DB_Dump(d->driverVars, f, indent+2);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Id              : %04x\n", d->driverId);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "IPC-Id          : %04x\n", d->ipcId);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Status          : %s (%d)\n",
          LC_DriverStatus_toString(d->status),
          d->status);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "ActiveReaders   : %d\n", d->activeReadersCount);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "AssignedReaders : %d\n", d->assignedReaders);
  for (i=0; i<indent; i++)
    fprintf(f, " ");

  dbT=GWEN_DB_Group_new("flags");
  LC_DriverFlags_toDb(dbT, "flags", d->driverFlags);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Driver flags : ");
  for (i=0; ; i++) {
    const char *s;

    s=GWEN_DB_GetCharValue(dbT, "flags", i, 0);
    if (!s)
      break;
    if (i)
      fprintf(f, ", ");
    fprintf(f, "%s", s);
  }
  GWEN_DB_Group_free(dbT);
  fprintf(stderr, "\n");
}














