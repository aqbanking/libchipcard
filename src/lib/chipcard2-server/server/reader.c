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


#include "reader_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_LIST_FUNCTIONS(LC_READER, LC_Reader);


static GWEN_TYPE_UINT32 LC_Reader_LastId=0;



LC_READER *LC_Reader_new(LC_DRIVER *d){
  LC_READER *r;

  assert(d);

  GWEN_NEW_OBJECT(LC_READER, r);
  DBG_MEM_INC("LC_READER", 0);
  GWEN_LIST_INIT(LC_READER, r);

  r->driver=d;
  r->requests=LC_Request_List_new();

  /* assign unique id */
  if (LC_Reader_LastId==0)
    LC_Reader_LastId=time(0);
  r->readerId=++LC_Reader_LastId;

  r->idleSince=time(0);

  return r;
}



LC_READER *LC_Reader_Instantiate(LC_DRIVER *d, LC_READER *r){
  LC_READER *rnew;

  rnew=LC_Reader_new(d);
  if (r->readerType)
    rnew->readerType=strdup(r->readerType);
  if (r->readerName) {
    char numbuf[16];
    GWEN_BUFFER *nbuf;

    nbuf=GWEN_Buffer_new(0, 256, 0, 1);
    snprintf(numbuf, sizeof(nbuf), "%d", LC_Reader_GetNextCount(r));
    GWEN_Buffer_AppendString(nbuf, r->readerName);
    GWEN_Buffer_AppendString(nbuf, numbuf);
    rnew->readerName=strdup(GWEN_Buffer_GetStart(nbuf));
    GWEN_Buffer_free(nbuf);
  }
  rnew->slots=r->slots;
  rnew->port=r->port;
  rnew->flags=r->flags;
  if (r->comType)
    rnew->comType=strdup(r->comType);
  if (r->shortDescr)
    rnew->shortDescr=strdup(r->shortDescr);
  rnew->vendorId=r->vendorId;
  rnew->productId=r->productId;

  return rnew;
}



LC_READER *LC_Reader_FromDb(LC_DRIVER *d, GWEN_DB_NODE *db){
  LC_READER *r;
  const char *p;
  unsigned int i;

  assert(d);

  GWEN_NEW_OBJECT(LC_READER, r);
  DBG_MEM_INC("LC_READER", 0);
  GWEN_LIST_INIT(LC_READER, r);

  r->driver=d;
  r->requests=LC_Request_List_new();

  /* assign unique id */
  if (LC_Reader_LastId==0)
    LC_Reader_LastId=time(0);
  r->readerId=++LC_Reader_LastId;

  p=GWEN_DB_GetCharValue(db, "readerType", 0, 0);
  if (p)
    r->readerType=strdup(p);

  p=GWEN_DB_GetCharValue(db, "comType", 0, "serial");
  assert(p);
  r->comType=strdup(p);

  p=GWEN_DB_GetCharValue(db, "readerName", 0, 0);
  if (p)
    r->readerName=strdup(p);

  p=GWEN_DB_GetCharValue(db, "shortName", 0, 0);
  if (p)
    r->shortDescr=strdup(p);

  r->slots=GWEN_DB_GetIntValue(db, "slots", 0, 1);
  r->port=GWEN_DB_GetIntValue(db, "port", 0, 0);

  r->vendorId=GWEN_DB_GetIntValue(db, "vendorId", 0, 0);
  r->productId=GWEN_DB_GetIntValue(db, "productId", 0, 0);

  for (i=0; ; i++) {
    p=GWEN_DB_GetCharValue(db, "flags", i, 0);
    if (!p)
      break;
    if (strcasecmp(p, "keypad")==0)
      r->flags|=LC_READER_FLAGS_KEYPAD;
    else if (strcasecmp(p, "display")==0)
      r->flags|=LC_READER_FLAGS_DISPLAY;
    else if (strcasecmp(p, "noinfo")==0)
      r->flags|=LC_READER_FLAGS_NOINFO;
    else if (strcasecmp(p, "remote")==0)
      r->flags|=LC_READER_FLAGS_REMOTE;
    else {
      DBG_WARN(0, "Unknown flag \"%s\", ignoring", p);
    }
  } /* for */

  r->idleSince=time(0);

  return r;
}



void LC_Reader_free(LC_READER *r){
  if (r) {
    DBG_MEM_DEC("LC_READER");
    GWEN_LIST_FINI(LC_READER, r);
    LC_Request_List_free(r->requests);
    free(r->readerType);
    free(r->readerName);
    free(r->comType);
    free(r->shortDescr);
    free(r->readerInfo);
    GWEN_FREE_OBJECT(r);
  }
}



int LC_Reader_IsAvailable(const LC_READER *r){
  assert(r);
  return r->isAvailable;
}



void LC_Reader_SetIsAvailable(LC_READER *r, int i){
  assert(r);
  r->isAvailable=i;
}



void LC_Reader_ToDb(const LC_READER *r, GWEN_DB_NODE *db){
  assert(r);
  assert(db);

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

  if (r->comType)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "comType", r->comType);

  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "vendorId", r->vendorId);
  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "productId", r->productId);

  /* store flags */
  GWEN_DB_DeleteVar(db, "flags");
  if (r->flags & LC_READER_FLAGS_KEYPAD)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "flags", "keypad");
  if (r->flags & LC_READER_FLAGS_DISPLAY)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "flags", "display");

}



const char *LC_Reader_GetReaderType(const LC_READER *r){
  assert(r);
  return r->readerType;
}



void LC_Reader_SetReaderType(LC_READER *r, const char *s){
  assert(r);
  assert(s);
  free(r->readerType);
  r->readerType=strdup(s);
}



const char *LC_Reader_GetReaderName(const LC_READER *r){
  assert(r);
  return r->readerName;
}



void LC_Reader_SetReaderName(LC_READER *r, const char *s){
  assert(r);
  assert(s);
  free(r->readerName);
  r->readerName=strdup(s);
}



const char *LC_Reader_GetReaderInfo(const LC_READER *r){
  assert(r);
  return r->readerInfo;
}



void LC_Reader_SetReaderInfo(LC_READER *r, const char *s){
  assert(r);
  free(r->readerInfo);
  if (s)
    r->readerInfo=strdup(s);
  else
    r->readerInfo=0;
}




GWEN_TYPE_UINT32 LC_Reader_GetReaderId(const LC_READER *r){
  assert(r);
  return r->readerId;
}



LC_READER_STATUS LC_Reader_GetStatus(const LC_READER *r){
  assert(r);
  return r->status;
}



void LC_Reader_SetStatus(LC_READER *r, LC_READER_STATUS st){
  assert(r);
  if (r->status!=st) {
    DBG_VERBOUS(0, "Changing status of reader \"%s\" from %d to %d",
                r->readerName, r->status, st);
    r->status=st;
    r->lastStatusChangeTime=time(0);
  }
}



time_t LC_Reader_GetLastStatusChangeTime(const LC_READER *r){
  assert(r);
  return r->lastStatusChangeTime;
}



time_t LC_Reader_GetIdleSince(const LC_READER *r){
  assert(r);
  return r->idleSince;
}



GWEN_TYPE_UINT32 LC_Reader_GetUsageCount(const LC_READER *r){
  assert(r);
  return r->usageCount;
}



void LC_Reader_IncUsageCount(LC_READER *r){
  assert(r);
  r->usageCount++;
  r->idleSince=(time_t)0;
  DBG_VERBOUS(0, "Incremented Usage count of reader \"%s\" to %d",
              r->readerName, r->usageCount);
}



void LC_Reader_DecUsageCount(LC_READER *r){
  assert(r);
  assert(r->usageCount);
  if (r->usageCount) {
    if (--(r->usageCount)==0) {
      r->idleSince=time(0);
      DBG_VERBOUS(0, "Decremented Usage count of reader \"%s\" to %d",
                  r->readerName, r->usageCount);
    }
  }
}



unsigned int LC_Reader_GetSlots(const LC_READER *r){
  assert(r);
  return r->slots;
}



void LC_Reader_SetSlots(LC_READER *r, unsigned int i){
  assert(r);
  r->slots=i;
}



unsigned int LC_Reader_GetPort(const LC_READER *r){
  assert(r);
  return r->port;
}



void LC_Reader_SetPort(LC_READER *r, unsigned int i){
  assert(r);
  r->port=i;
}



GWEN_TYPE_UINT32 LC_Reader_GetFlags(const LC_READER *r){
  assert(r);
  return r->flags;
}



void LC_Reader_SetFlags(LC_READER *r, GWEN_TYPE_UINT32 f){
  assert(r);
  r->flags=f;
}



void LC_Reader_AddFlags(LC_READER *r, GWEN_TYPE_UINT32 f){
  assert(r);
  r->flags|=f;
}



void LC_Reader_SubFlags(LC_READER *r, GWEN_TYPE_UINT32 f){
  assert(r);
  r->flags&=~f;
}



LC_DRIVER *LC_Reader_GetDriver(const LC_READER *r){
  assert(r);
  return r->driver;
}



GWEN_TYPE_UINT32 LC_Reader_GetCurrentRequestId(const LC_READER *r){
  assert(r);
  return r->currentRequestId;
}



void LC_Reader_SetCurrentRequestId(LC_READER *r, GWEN_TYPE_UINT32 rid){
  assert(r);
  r->currentRequestId=rid;
  r->commandTime=time(0);
}



LC_REQUEST *LC_Reader_GetNextRequest(LC_READER *r){
  LC_REQUEST *rq;

  assert(r);
  rq=LC_Request_List_First(r->requests);
  if (rq) {
    LC_Request_List_Del(rq);
    return rq;
  }
  return 0;
}



void LC_Reader_AddRequest(LC_READER *r, LC_REQUEST *rq){
  assert(r);
  assert(rq);
  LC_Request_List_Add(rq, r->requests);
}



void LC_Reader_ClearRequests(LC_READER *r){
  assert(r);
  LC_Request_List_Clear(r->requests);
}



time_t LC_Reader_GetCommandTime(const LC_READER *r){
  return r->commandTime;
}



int LC_Reader_HasNextRequest(const LC_READER *r){
  assert(r);
  return (LC_Request_List_First(r->requests)!=0);
}



GWEN_TYPE_UINT32 LC_Reader_GetVendorId(const LC_READER *r){
  assert(r);
  return r->vendorId;
}



void LC_Reader_SetVendorId(LC_READER *r, GWEN_TYPE_UINT32 i){
  assert(r);
  r->vendorId=i;
}



GWEN_TYPE_UINT32 LC_Reader_GetProductId(const LC_READER *r){
  assert(r);
  return r->productId;
}



void LC_Reader_SetProductId(LC_READER *r, GWEN_TYPE_UINT32 i){
  assert(r);
  r->productId=i;
}



GWEN_TYPE_UINT32 LC_Reader_GetBusId(const LC_READER *r){
  assert(r);
  return r->busId;
}



void LC_Reader_SetBusId(LC_READER *r, GWEN_TYPE_UINT32 i){
  assert(r);
  r->busId=i;
}



GWEN_TYPE_UINT32 LC_Reader_GetDeviceId(const LC_READER *r){
  assert(r);
  return r->deviceId;
}



void LC_Reader_SetDeviceId(LC_READER *r, GWEN_TYPE_UINT32 i){
  assert(r);
  r->deviceId=i;
}



const char *LC_Reader_GetComType(const LC_READER *r){
  assert(r);
  return r->comType;
}



void LC_Reader_SetComType(LC_READER *r, const char *s){
  assert(r);
  free(r->comType);
  if (s)
    r->comType=strdup(s);
  else
    r->comType=0;
}



const char *LC_Reader_GetShortDescr(const LC_READER *r){
  assert(r);
  return r->shortDescr;
}



void LC_Reader_SetShortDescr(LC_READER *r, const char *s){
  assert(r);
  free(r->shortDescr);
  if (s)
    r->shortDescr=strdup(s);
  else
    r->shortDescr=0;
}



GWEN_TYPE_UINT32 LC_Reader_GetNextCount(LC_READER *r){
  assert(r);
  return r->count++;
}



int LC_Reader_GetWantRestart(const LC_READER *r){
  assert(r);
  return r->wantRestart;
}



void LC_Reader_SetWantRestart(LC_READER *r, int wantRestart){
  assert(r);
  r->wantRestart=wantRestart;
}



void LC_Reader_Dump(const LC_READER *r, FILE *f, int indent) {
  int i;

  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Reader\n");
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "--------------------------\n");
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Name : %s\n", r->readerName);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Type : %s\n", r->readerType);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Slots : %d\n", r->slots);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Usage : %d\n", r->usageCount);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Status : %d\n", r->status);
}



void LC_Reader_DelClientRequests(LC_READER *r, LC_CLIENT *cl){
  LC_REQUEST *rq;

  /* remove all requests of the given client */
  rq=LC_Request_List_First(r->requests);
  while(rq) {
    LC_REQUEST *next;

    next=LC_Request_List_Next(rq);
    if (LC_Request_GetClient(rq)==cl) {
      LC_Request_List_Del(rq);
      LC_Request_free(rq);
    }
    rq=next;
  } /* while */
}













