/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: dm_reader.c 282 2006-09-21 16:52:04Z martin $
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
#include "common/driverinfo.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_LIST_FUNCTIONS(LCCO_READER, LCCO_Reader);
GWEN_LIST2_FUNCTIONS(LCCO_READER, LCCO_Reader);
GWEN_INHERIT_FUNCTIONS(LCCO_READER);



LCCO_READER_ADDRESS LCCO_ReaderAddress_fromString(const char *s) {
  assert(s);
  if (strcasecmp(s, "port")==0)
    return LCCO_Reader_AddressPort;
  else if (strcasecmp(s, "devicePath")==0)
    return LCCO_Reader_AddressDevicePath;
  else
    return LCCO_Reader_AddressUnknown;
}



const char *LCCO_ReaderAddress_toString(LCCO_READER_ADDRESS at) {
  switch(at) {
  case LCCO_Reader_AddressPort:       return "port";
  case LCCO_Reader_AddressDevicePath: return "devicePath";
  case LCCO_Reader_AddressUnknown:    return "unknown";
  }
  return "unknown";
}







LCCO_READER *LCCO_Reader_new(){
  LCCO_READER *r;

  GWEN_NEW_OBJECT(LCCO_READER, r);
  GWEN_INHERIT_INIT(LCCO_READER, r);
  GWEN_LIST_INIT(LCCO_READER, r);
  r->refCount=1;

  return r;
}



LCCO_READER *LCCO_Reader_dup(const LCCO_READER *r){
  LCCO_READER *nr;

  nr=LCCO_Reader_new();
#define COPY_STRING(x) if (r->x) nr->x=strdup(r->x)
  COPY_STRING(readerType);
  COPY_STRING(readerName);
  COPY_STRING(driverName);
  COPY_STRING(shortDescr);
  COPY_STRING(readerInfo);
  COPY_STRING(devicePath);
  COPY_STRING(devicePathTmpl);
#undef COPY_STRING
  nr->addressType=r->addressType;
  nr->slots=r->slots;
  nr->ctn=r->ctn;
  nr->port=r->port;
  nr->flags=r->flags;
  nr->busType=r->busType;
  nr->vendorId=r->vendorId;
  nr->productId=r->productId;
  nr->isAvailable=r->isAvailable;
  nr->busId=r->busId;
  nr->deviceId=r->deviceId;
  nr->readerId=r->readerId;
  nr->driversReaderId=r->driversReaderId;
  nr->status=r->status;
  nr->lastStatusChangeTime=r->lastStatusChangeTime;

  return nr;
}



LCCO_READER *LCCO_Reader_fromDb(GWEN_DB_NODE *db){
  LCCO_READER *r;
  const char *p;

  r=LCCO_Reader_new();

  p=GWEN_DB_GetCharValue(db, "driversReaderId", 0, 0);
  if (p) {
    unsigned int rid;

    if (1==sscanf(p, "%x", &rid))
      r->driversReaderId=rid;
  }

  p=GWEN_DB_GetCharValue(db, "readerType", 0, 0);
  if (p)
    r->readerType=strdup(p);

  p=GWEN_DB_GetCharValue(db, "readerName", 0, 0);
  if (p)
    r->readerName=strdup(p);

  p=GWEN_DB_GetCharValue(db, "driverName", 0, 0);
  if (p)
    r->driverName=strdup(p);

  p=GWEN_DB_GetCharValue(db, "shortName", 0, 0);
  if (p)
    r->shortDescr=strdup(p);

  p=GWEN_DB_GetCharValue(db, "devicePath", 0, 0);
  if (p)
    r->devicePath=strdup(p);
  p=GWEN_DB_GetCharValue(db, "devicePathTmpl", 0, 0);
  if (p)
    r->devicePathTmpl=strdup(p);

  p=GWEN_DB_GetCharValue(db, "addressType", 0, "port");
  r->addressType=LCCO_ReaderAddress_fromString(p);
  if (r->addressType==LCCO_Reader_AddressUnknown) {
    DBG_WARN(0, "Unknown address type \"%s\"", p);
    r->addressType=LCCO_Reader_AddressPort;
  }

  r->slots=GWEN_DB_GetIntValue(db, "slots", 0, 1);
  if (r->slots<1) {
    DBG_WARN(0, "Invalid number of slots (%d)", r->slots);
  }
  r->port=GWEN_DB_GetIntValue(db, "port", 0, 0);
  r->ctn=GWEN_DB_GetIntValue(db, "ctn", 0, 0);

  p=GWEN_DB_GetCharValue(db, "busType", 0, 0);
  if (p)
    r->busType=LC_Device_BusType_fromString(p);
  r->busId=GWEN_DB_GetIntValue(db, "busId", 0, 0);
  r->vendorId=GWEN_DB_GetIntValue(db, "vendorId", 0, 0);
  r->productId=GWEN_DB_GetIntValue(db, "productId", 0, 0);

  r->flags=LC_ReaderFlags_fromDb(db, "flags");

  return r;
}



void LCCO_Reader_free(LCCO_READER *r){
  if (r) {
    assert(r->refCount);
    if (r->refCount==1) {
      DBG_DEBUG(0, "Deleting reader \"%s\"", r->readerName);
      GWEN_INHERIT_FINI(LCCO_READER, r);
      free(r->readerType);
      free(r->readerName);
      free(r->driverName);
      free(r->shortDescr);
      free(r->readerInfo);
      free(r->devicePath);
      free(r->devicePathTmpl);
      GWEN_LIST_FINI(LCCO_READER, r);
      GWEN_FREE_OBJECT(r);
    }
    else
      r->refCount--;
  }
}



void LCCO_Reader_Attach(LCCO_READER *r) {
  assert(r);
  assert(r->refCount);
  r->refCount++;
}



void LCCO_Reader_List2_freeAll(LCCO_READER_LIST2 *rl) {
  if (rl) {
    LCCO_READER_LIST2_ITERATOR *it;

    it=LCCO_Reader_List2_First(rl);
    if (it) {
      LCCO_READER *r;

      r=LCCO_Reader_List2Iterator_Data(it);
      assert(r);
      while(r) {
        LCCO_Reader_free(r);
        r=LCCO_Reader_List2Iterator_Next(it);
      }
      LCCO_Reader_List2Iterator_free(it);
    }
    LCCO_Reader_List2_free(rl);
  }
}



void LCCO_Reader_toDb(const LCCO_READER *r, GWEN_DB_NODE *db){
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

  if (r->driverName)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "driverName", r->driverName);

  if (r->shortDescr)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
			 "shortName", r->shortDescr);
  if (r->devicePath)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "devicePath", r->devicePath);
  if (r->devicePathTmpl)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "devicePathTmpl", r->devicePathTmpl);

  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "addressType",
                       LCCO_ReaderAddress_toString(r->addressType));

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



int LCCO_Reader_IsAvailable(const LCCO_READER *r){
  assert(r);
  return r->isAvailable;
}



void LCCO_Reader_SetIsAvailable(LCCO_READER *r, int i){
  assert(r);
  r->isAvailable=i;
}



const char *LCCO_Reader_GetReaderType(const LCCO_READER *r){
  assert(r);
  return r->readerType;
}



void LCCO_Reader_SetReaderType(LCCO_READER *r, const char *s){
  assert(r);
  assert(s);
  free(r->readerType);
  r->readerType=strdup(s);
}



const char *LCCO_Reader_GetReaderName(const LCCO_READER *r){
  assert(r);
  return r->readerName;
}



void LCCO_Reader_SetReaderName(LCCO_READER *r, const char *s){
  assert(r);
  assert(s);
  free(r->readerName);
  r->readerName=strdup(s);
}



const char *LCCO_Reader_GetDriverName(const LCCO_READER *r){
  assert(r);
  return r->driverName;
}



void LCCO_Reader_SetDriverName(LCCO_READER *r, const char *s){
  assert(r);
  assert(s);
  free(r->driverName);
  r->driverName=strdup(s);
}



const char *LCCO_Reader_GetReaderInfo(const LCCO_READER *r){
  assert(r);
  return r->readerInfo;
}



void LCCO_Reader_SetReaderInfo(LCCO_READER *r, const char *s){
  assert(r);
  free(r->readerInfo);
  if (s)
    r->readerInfo=strdup(s);
  else
    r->readerInfo=0;
}




GWEN_TYPE_UINT32 LCCO_Reader_GetReaderId(const LCCO_READER *r){
  assert(r);
  return r->readerId;
}



void LCCO_Reader_SetReaderId(LCCO_READER *r, GWEN_TYPE_UINT32 id) {
  assert(r);
  r->readerId=id;
}



GWEN_TYPE_UINT32 LCCO_Reader_GetDriversReaderId(const LCCO_READER *r){
  assert(r);
  return r->driversReaderId;
}



void LCCO_Reader_SetDriversReaderId(LCCO_READER *r, GWEN_TYPE_UINT32 id){
  assert(r);
  r->driversReaderId=id;
}



LC_READER_STATUS LCCO_Reader_GetStatus(const LCCO_READER *r){
  assert(r);
  return r->status;
}



void LCCO_Reader_SetStatus(LCCO_READER *r, LC_READER_STATUS st){
  assert(r);
  if (r->status!=st) {
    DBG_VERBOUS(0, "Changing status of reader \"%s\" from %d to %d",
                r->readerName, r->status, st);
    r->status=st;
    r->lastStatusChangeTime=time(0);
  }
}



time_t LCCO_Reader_GetLastStatusChangeTime(const LCCO_READER *r){
  assert(r);
  return r->lastStatusChangeTime;
}



unsigned int LCCO_Reader_GetSlots(const LCCO_READER *r){
  assert(r);
  return r->slots;
}



void LCCO_Reader_SetSlots(LCCO_READER *r, unsigned int i){
  assert(r);
  r->slots=i;
}



unsigned int LCCO_Reader_GetPort(const LCCO_READER *r){
  assert(r);
  return r->port;
}



void LCCO_Reader_SetPort(LCCO_READER *r, unsigned int i){
  assert(r);
  r->port=i;
}



unsigned int LCCO_Reader_GetCtn(const LCCO_READER *r) {
  assert(r);
  return r->ctn;
}



void LCCO_Reader_SetCtn(LCCO_READER *r, unsigned int i) {
  assert(r);
  r->ctn=i;
}



GWEN_TYPE_UINT32 LCCO_Reader_GetFlags(const LCCO_READER *r){
  assert(r);
  return r->flags;
}



void LCCO_Reader_SetFlags(LCCO_READER *r, GWEN_TYPE_UINT32 f){
  assert(r);
  r->flags=f;
}



void LCCO_Reader_AddFlags(LCCO_READER *r, GWEN_TYPE_UINT32 f){
  assert(r);
  r->flags|=f;
}



void LCCO_Reader_SubFlags(LCCO_READER *r, GWEN_TYPE_UINT32 f){
  assert(r);
  r->flags&=~f;
}



GWEN_TYPE_UINT32 LCCO_Reader_GetVendorId(const LCCO_READER *r){
  assert(r);
  return r->vendorId;
}



void LCCO_Reader_SetVendorId(LCCO_READER *r, GWEN_TYPE_UINT32 i){
  assert(r);
  r->vendorId=i;
}



GWEN_TYPE_UINT32 LCCO_Reader_GetProductId(const LCCO_READER *r){
  assert(r);
  return r->productId;
}



void LCCO_Reader_SetProductId(LCCO_READER *r, GWEN_TYPE_UINT32 i){
  assert(r);
  r->productId=i;
}



GWEN_TYPE_UINT32 LCCO_Reader_GetBusId(const LCCO_READER *r){
  assert(r);
  return r->busId;
}



LC_DEVICE_BUSTYPE LCCO_Reader_GetBusType(const LCCO_READER *r){
  assert(r);
  return r->busType;
}



void LCCO_Reader_SetBusType(LCCO_READER *r, LC_DEVICE_BUSTYPE i){
  assert(r);
  r->busType=i;
}



void LCCO_Reader_SetBusId(LCCO_READER *r, GWEN_TYPE_UINT32 i){
  assert(r);
  r->busId=i;
}



GWEN_TYPE_UINT32 LCCO_Reader_GetDeviceId(const LCCO_READER *r){
  assert(r);
  return r->deviceId;
}



void LCCO_Reader_SetDeviceId(LCCO_READER *r, GWEN_TYPE_UINT32 i){
  assert(r);
  r->deviceId=i;
}



const char *LCCO_Reader_GetShortDescr(const LCCO_READER *r){
  assert(r);
  return r->shortDescr;
}



void LCCO_Reader_SetShortDescr(LCCO_READER *r, const char *s){
  assert(r);
  free(r->shortDescr);
  if (s)
    r->shortDescr=strdup(s);
  else
    r->shortDescr=0;
}



LCCO_READER_ADDRESS LCCO_Reader_GetAddressType(const LCCO_READER *r) {
  assert(r);
  return r->addressType;
}



void LCCO_Reader_SetAddressType(LCCO_READER *r, LCCO_READER_ADDRESS a) {
  assert(r);
  r->addressType=a;
}



const char *LCCO_Reader_GetDevicePath(const LCCO_READER *r) {
  assert(r);
  return r->devicePath;
}



void LCCO_Reader_SetDevicePath(LCCO_READER *r, const char *s) {
  assert(r);
  free(r->devicePath);
  if (s) r->devicePath=strdup(s);
  else r->devicePath=0;
}



const char *LCCO_Reader_GetDevicePathTmpl(const LCCO_READER *r) {
  assert(r);
  return r->devicePathTmpl;
}



void LCCO_Reader_SetDevicePathTmpl(LCCO_READER *r, const char *s) {
  assert(r);
  free(r->devicePathTmpl);
  if (s) r->devicePathTmpl=strdup(s);
  else r->devicePathTmpl=0;
}



void LCCO_Reader_Dump(const LCCO_READER *r, FILE *f, int indent) {
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



