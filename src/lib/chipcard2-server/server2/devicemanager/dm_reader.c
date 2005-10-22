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


#include "dm_reader_p.h"
#include "common/driverinfo.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_LIST_FUNCTIONS(LCDM_READER, LCDM_Reader);


static GWEN_TYPE_UINT32 LCDM_Reader_LastId=0;



LCDM_READER *LCDM_Reader_new(LCDM_DRIVER *d){
  LCDM_READER *r;

  assert(d);

  GWEN_NEW_OBJECT(LCDM_READER, r);
  DBG_MEM_INC("LCDM_READER", 0);
  GWEN_LIST_INIT(LCDM_READER, r);
  r->refCount=1;

  r->driver=d;

  /* assign unique id */
  if (LCDM_Reader_LastId==0)
    LCDM_Reader_LastId=time(0);
  r->readerId=++LCDM_Reader_LastId;

  r->idleSince=time(0);

  return r;
}



LCDM_READER *LCDM_Reader_fromDb(LCDM_DRIVER *d, GWEN_DB_NODE *db){
  LCDM_READER *r;
  const char *p;

  assert(d);

  GWEN_NEW_OBJECT(LCDM_READER, r);
  DBG_MEM_INC("LCDM_READER", 0);
  GWEN_LIST_INIT(LCDM_READER, r);
  r->refCount=1;

  r->driver=d;

  /* assign unique id */
  if (LCDM_Reader_LastId==0)
    LCDM_Reader_LastId=time(0);
  r->readerId=++LCDM_Reader_LastId;

  r->driversReaderId=GWEN_DB_GetIntValue(db, "driversReaderId", 0, 0);

  p=GWEN_DB_GetCharValue(db, "readerType", 0, 0);
  if (p)
    r->readerType=strdup(p);

  p=GWEN_DB_GetCharValue(db, "readerName", 0, 0);
  if (p)
    r->readerName=strdup(p);

  p=GWEN_DB_GetCharValue(db, "shortName", 0, 0);
  if (p)
    r->shortDescr=strdup(p);

  r->slots=GWEN_DB_GetIntValue(db, "slots", 0, 1);
  r->port=GWEN_DB_GetIntValue(db, "port", 0, 0);
  r->ctn=GWEN_DB_GetIntValue(db, "ctn", 0, 0);

  p=GWEN_DB_GetCharValue(db, "busType", 0, 0);
  if (p)
    r->busType=LC_Device_BusType_fromString(p);
  r->busId=GWEN_DB_GetIntValue(db, "busId", 0, 0);
  r->vendorId=GWEN_DB_GetIntValue(db, "vendorId", 0, 0);
  r->productId=GWEN_DB_GetIntValue(db, "productId", 0, 0);

  r->flags=LC_ReaderFlags_fromDb(db, "flags");

  r->idleSince=time(0);

  return r;
}



void LCDM_Reader_free(LCDM_READER *r){
  if (r) {
    assert(r->refCount);
    if (r->refCount==1) {
      DBG_MEM_DEC("LCDM_READER");
      GWEN_LIST_FINI(LCDM_READER, r);
      free(r->readerType);
      free(r->readerName);
      free(r->shortDescr);
      free(r->readerInfo);
      GWEN_FREE_OBJECT(r);
    }
    else
      r->refCount--;
  }
}



void LCDM_Reader_Attach(LCDM_READER *r) {
  assert(r);
  assert(r->refCount);
  r->refCount++;
}



int LCDM_Reader_IsAvailable(const LCDM_READER *r){
  assert(r);
  return r->isAvailable;
}



void LCDM_Reader_SetIsAvailable(LCDM_READER *r, int i){
  assert(r);
  r->isAvailable=i;
}



void LCDM_Reader_toDb(const LCDM_READER *r, GWEN_DB_NODE *db){
  assert(r);
  assert(db);

  if (r->driversReaderId)
    GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
			"driversReaderId",
			r->driversReaderId);

  if (r->readerType)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "readerType", r->readerType);

  if (r->readerName)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "readerName", r->readerName);

  if (r->shortDescr)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
			 "shortName", r->shortDescr);

  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "slots", r->slots);

  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "port", r->port);

  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "ctn", r->ctn);

  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "busType",
                       LC_Device_BusType_toString(r->busType));
  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "busId", r->busId);
  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "vendorId", r->vendorId);
  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "productId", r->productId);

  /* store flags */
  LC_ReaderFlags_toDb(db, "flags", r->flags);
}



const char *LCDM_Reader_GetReaderType(const LCDM_READER *r){
  assert(r);
  return r->readerType;
}



void LCDM_Reader_SetReaderType(LCDM_READER *r, const char *s){
  assert(r);
  assert(s);
  free(r->readerType);
  r->readerType=strdup(s);
}



const char *LCDM_Reader_GetReaderName(const LCDM_READER *r){
  assert(r);
  return r->readerName;
}



void LCDM_Reader_SetReaderName(LCDM_READER *r, const char *s){
  assert(r);
  assert(s);
  free(r->readerName);
  r->readerName=strdup(s);
}



const char *LCDM_Reader_GetReaderInfo(const LCDM_READER *r){
  assert(r);
  return r->readerInfo;
}



void LCDM_Reader_SetReaderInfo(LCDM_READER *r, const char *s){
  assert(r);
  free(r->readerInfo);
  if (s)
    r->readerInfo=strdup(s);
  else
    r->readerInfo=0;
}




GWEN_TYPE_UINT32 LCDM_Reader_GetReaderId(const LCDM_READER *r){
  assert(r);
  return r->readerId;
}



GWEN_TYPE_UINT32 LCDM_Reader_GetDriversReaderId(const LCDM_READER *r){
  assert(r);
  return r->driversReaderId;
}



void LCDM_Reader_SetDriversReaderId(LCDM_READER *r, GWEN_TYPE_UINT32 id){
  assert(r);
  r->driversReaderId=id;
}



LC_READER_STATUS LCDM_Reader_GetStatus(const LCDM_READER *r){
  assert(r);
  return r->status;
}



void LCDM_Reader_SetStatus(LCDM_READER *r, LC_READER_STATUS st){
  assert(r);
  if (r->status!=st) {
    DBG_VERBOUS(0, "Changing status of reader \"%s\" from %d to %d",
                r->readerName, r->status, st);
    r->status=st;
    r->lastStatusChangeTime=time(0);
  }
}



time_t LCDM_Reader_GetLastStatusChangeTime(const LCDM_READER *r){
  assert(r);
  return r->lastStatusChangeTime;
}



time_t LCDM_Reader_GetIdleSince(const LCDM_READER *r){
  assert(r);
  return r->idleSince;
}



GWEN_TYPE_UINT32 LCDM_Reader_GetUsageCount(const LCDM_READER *r){
  assert(r);
  return r->usageCount;
}



void LCDM_Reader_IncUsageCount(LCDM_READER *r, int count){
  assert(r);
  assert(count);
  r->usageCount+=count;
  r->idleSince=(time_t)0;
  DBG_VERBOUS(0, "Incremented Usage count of reader \"%s\" to %d",
              r->readerName, r->usageCount);
}



void LCDM_Reader_DecUsageCount(LCDM_READER *r, int count){
  assert(r);
  assert(count);
  assert(r->usageCount);
  assert(r->usageCount>=count);
  if ((r->usageCount-=count)==0) {
    r->idleSince=time(0);
    DBG_VERBOUS(0, "Reader \"%s\" became idle",
                r->readerName);
  }
}



unsigned int LCDM_Reader_GetSlots(const LCDM_READER *r){
  assert(r);
  return r->slots;
}



void LCDM_Reader_SetSlots(LCDM_READER *r, unsigned int i){
  assert(r);
  r->slots=i;
}



unsigned int LCDM_Reader_GetPort(const LCDM_READER *r){
  assert(r);
  return r->port;
}



void LCDM_Reader_SetPort(LCDM_READER *r, unsigned int i){
  assert(r);
  r->port=i;
}



unsigned int LCDM_Reader_GetCtn(const LCDM_READER *r) {
  assert(r);
  return r->ctn;
}



void LCDM_Reader_SetCtn(LCDM_READER *r, unsigned int i) {
  assert(r);
  r->ctn=i;
}



GWEN_TYPE_UINT32 LCDM_Reader_GetFlags(const LCDM_READER *r){
  assert(r);
  return r->flags;
}



void LCDM_Reader_SetFlags(LCDM_READER *r, GWEN_TYPE_UINT32 f){
  assert(r);
  r->flags=f;
}



void LCDM_Reader_AddFlags(LCDM_READER *r, GWEN_TYPE_UINT32 f){
  assert(r);
  r->flags|=f;
}



void LCDM_Reader_SubFlags(LCDM_READER *r, GWEN_TYPE_UINT32 f){
  assert(r);
  r->flags&=~f;
}



LCDM_DRIVER *LCDM_Reader_GetDriver(const LCDM_READER *r){
  assert(r);
  return r->driver;
}



GWEN_TYPE_UINT32 LCDM_Reader_GetVendorId(const LCDM_READER *r){
  assert(r);
  return r->vendorId;
}



void LCDM_Reader_SetVendorId(LCDM_READER *r, GWEN_TYPE_UINT32 i){
  assert(r);
  r->vendorId=i;
}



GWEN_TYPE_UINT32 LCDM_Reader_GetProductId(const LCDM_READER *r){
  assert(r);
  return r->productId;
}



void LCDM_Reader_SetProductId(LCDM_READER *r, GWEN_TYPE_UINT32 i){
  assert(r);
  r->productId=i;
}



GWEN_TYPE_UINT32 LCDM_Reader_GetBusId(const LCDM_READER *r){
  assert(r);
  return r->busId;
}



LC_DEVICE_BUSTYPE LCDM_Reader_GetBusType(const LCDM_READER *r){
  assert(r);
  return r->busType;
}



void LCDM_Reader_SetBusType(LCDM_READER *r, LC_DEVICE_BUSTYPE i){
  assert(r);
  r->busType=i;
}



void LCDM_Reader_SetBusId(LCDM_READER *r, GWEN_TYPE_UINT32 i){
  assert(r);
  r->busId=i;
}



GWEN_TYPE_UINT32 LCDM_Reader_GetDeviceId(const LCDM_READER *r){
  assert(r);
  return r->deviceId;
}



void LCDM_Reader_SetDeviceId(LCDM_READER *r, GWEN_TYPE_UINT32 i){
  assert(r);
  r->deviceId=i;
}



const char *LCDM_Reader_GetShortDescr(const LCDM_READER *r){
  assert(r);
  return r->shortDescr;
}



void LCDM_Reader_SetShortDescr(LCDM_READER *r, const char *s){
  assert(r);
  free(r->shortDescr);
  if (s)
    r->shortDescr=strdup(s);
  else
    r->shortDescr=0;
}



void LCDM_Reader_SetTimeout(LCDM_READER *r, int secs) {
  if (secs==0)
    r->timeout=0;
  else {
    time_t t;
    assert(r);

    t=time(0);
    t+=secs;
    r->timeout=t;
  }
}



int LCDM_Reader_CheckTimeout(const LCDM_READER *r) {
  if (r->timeout==0)
    return -1;
  else {
    time_t t;

    assert(r);
    t=time(0);
    return (difftime(t, r->timeout)>0);
  }
}



GWEN_TYPE_UINT32 LCDM_Reader_GetCurrentRequestId(const LCDM_READER *r) {
  assert(r);
  return r->currentRequestId;
}



void LCDM_Reader_SetCurrentRequestId(LCDM_READER *r, GWEN_TYPE_UINT32 rid) {
  assert(r);
  r->currentRequestId=rid;
}



void LCDM_Reader_Dump(const LCDM_READER *r, FILE *f, int indent) {
  int i;
  GWEN_DB_NODE *dbT;

  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "--------------------------\n");
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Reader\n");
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Name   : %s\n", r->readerName);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Type   : %s\n", r->readerType);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Slots  : %d\n", r->slots);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Usage  : %d\n", r->usageCount);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Status : %s (%d)\n",
          LC_ReaderStatus_toString(r->status),
          r->status);

  dbT=GWEN_DB_Group_new("flags");
  LC_ReaderFlags_toDb(dbT, "flags", r->flags);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Flags  : ");
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







