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
#include <gwenhywfar/inherit.h>
#include <chipcard2/chipcard2.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>



GWEN_LIST_FUNCTIONS(LC_READER, LC_Reader);
GWEN_INHERIT_FUNCTIONS(LC_READER);


LC_READER *LC_Reader_new(GWEN_TYPE_UINT32 readerId,
                         const char *name,
                         int port,
                         unsigned int slots,
                         GWEN_TYPE_UINT32 flags){
  LC_READER *r;
  unsigned int i;

  GWEN_NEW_OBJECT(LC_READER, r);
  GWEN_LIST_INIT(LC_READER, r);
  GWEN_INHERIT_INIT(LC_READER, r);
  r->readerId=readerId;
  if (name)
    r->name=strdup(name);
  r->port=port;
  r->readerFlags=flags;
  r->slots=LC_Slot_List_new();
  /* create slots */
  for (i=0; i<slots; i++) {
    LC_SLOT *sl;

    sl=LC_Slot_new(r, i);
    LC_Slot_List_Add(sl, r->slots);
  } /* for */

  return r;
}



void LC_Reader_free(LC_READER *r){
  if (r) {
    GWEN_INHERIT_FINI(LC_READER, r);
    GWEN_LIST_FINI(LC_READER, r);
    free(r->readerType);
    free(r->name);
    LC_Slot_List_free(r->slots);
    free(r->logger);

    GWEN_FREE_OBJECT(r);
  }
}



GWEN_TYPE_UINT32 LC_Reader_GetReaderId(const LC_READER *r){
  assert(r);
  return r->readerId;
}



void LC_Reader_SetReaderId(LC_READER *r, GWEN_TYPE_UINT32 id){
  assert(r);
  r->readerId=id;
}



GWEN_TYPE_UINT32 LC_Reader_GetDriversReaderId(const LC_READER *r){
  assert(r);
  return r->driversReaderId;
}



void LC_Reader_SetDriversReaderId(LC_READER *r, GWEN_TYPE_UINT32 id){
  assert(r);
  r->driversReaderId=id;
}



const char *LC_Reader_GetName(const LC_READER *r){
  assert(r);
  return r->name;
}



const char *LC_Reader_GetReaderType(const LC_READER *r){
  assert(r);
  return r->readerType;
}



void LC_Reader_SetReaderType(LC_READER *r, const char *s){
  assert(r);
  free(r->readerType);
  if (s) r->readerType=strdup(s);
  else r->readerType=0;
}



int LC_Reader_GetPort(const LC_READER *r){
  assert(r);
  return r->port;
}



GWEN_TYPE_UINT32 LC_Reader_GetReaderFlags(const LC_READER *r){
  assert(r);
  return r->readerFlags;
}



GWEN_TYPE_UINT32 LC_Reader_GetCardNum(const LC_READER *r){
  assert(r);
  return r->cardNum;
}



GWEN_TYPE_UINT32 LC_Reader_GetStatus(const LC_READER *r){
  assert(r);
  return r->status;
}



void LC_Reader_SetStatus(LC_READER *r, GWEN_TYPE_UINT32 s){
  assert(r);
  r->status=s;
}



void LC_Reader_AddStatus(LC_READER *r, GWEN_TYPE_UINT32 s){
  assert(r);
  r->status|=s;
}



void LC_Reader_SubStatus(LC_READER *r, GWEN_TYPE_UINT32 s){
  assert(r);
  r->status&=~s;
}



LC_SLOT_LIST *LC_Reader_GetSlots(const LC_READER *r){
  assert(r);
  return r->slots;
}



LC_SLOT *LC_Reader_FindSlot(const LC_READER *r, unsigned int slotnum){
  LC_SLOT *sl;

  assert(r);
  sl=LC_Slot_List_First(r->slots);
  while(sl) {
    if (LC_Slot_GetSlotNum(sl)==slotnum)
      return sl;
    sl=LC_Slot_List_Next(sl);
  } /* while */

  return 0;
}



const char *LC_Reader_GetLogger(const LC_READER *r){
  assert(r);
  return r->logger;
}



void LC_Reader_SetLogger(LC_READER *r, const char *logDomain){
  assert(r);
  free(r->logger);
  if (logDomain) r->logger=strdup(logDomain);
  else r->logger=0;
}



GWEN_TYPE_UINT32 LC_Reader_GetDriverFlags(const LC_READER *r){
  assert(r);
  return r->driverFlags;
}



void LC_Reader_SetDriverFlags(LC_READER *r, GWEN_TYPE_UINT32 s){
  assert(r);
  r->driverFlags=s;
}



void LC_Reader_AddDriverFlags(LC_READER *r, GWEN_TYPE_UINT32 s){
  assert(r);
  r->driverFlags|=s;
}



void LC_Reader_SubDriverFlags(LC_READER *r, GWEN_TYPE_UINT32 s){
  assert(r);
  r->driverFlags&=~s;
}










