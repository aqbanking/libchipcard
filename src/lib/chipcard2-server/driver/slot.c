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


#include "slot_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>
#include <chipcard2/chipcard2.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>




GWEN_LIST_FUNCTIONS(LC_SLOT, LC_Slot);


LC_SLOT *LC_Slot_new(LC_READER *r, unsigned int slotNum){
  LC_SLOT *sl;

  GWEN_NEW_OBJECT(LC_SLOT, sl);
  GWEN_LIST_INIT(LC_SLOT, sl);
  sl->reader=r;
  sl->slotNum=slotNum;

  return sl;
}



void LC_Slot_free(LC_SLOT *sl){
  if (sl) {
    GWEN_LIST_FINI(LC_SLOT, sl);
    GWEN_Buffer_free(sl->atr);

    GWEN_FREE_OBJECT(sl);
  }
}



GWEN_TYPE_UINT32 LC_Slot_GetStatus(const LC_SLOT *sl){
  assert(sl);
  return sl->status;
}



void LC_Slot_SetStatus(LC_SLOT *sl, GWEN_TYPE_UINT32 s){
  assert(sl);
  if (sl->status!=s) {
    sl->status=s;
    sl->lastStatusChange=time(0);
  }
}



void LC_Slot_AddStatus(LC_SLOT *sl, GWEN_TYPE_UINT32 s){
  GWEN_TYPE_UINT32 nst;

  assert(sl);
  nst=sl->status|s;
  if (sl->status!=nst) {
    sl->status=nst;
    sl->lastStatusChange=time(0);
  }
}



void LC_Slot_SubStatus(LC_SLOT *sl, GWEN_TYPE_UINT32 s){
  GWEN_TYPE_UINT32 nst;

  assert(sl);
  nst=sl->status&~s;
  if (sl->status!=nst) {
    sl->status=nst;
    sl->lastStatusChange=time(0);
  }
}



LC_READER *LC_Slot_GetReader(const LC_SLOT *sl){
  assert(sl);
  return sl->reader;
}



unsigned int LC_Slot_GetSlotNum(const LC_SLOT *sl){
  assert(sl);
  return sl->slotNum;
}



GWEN_TYPE_UINT32 LC_Slot_GetCardNum(const LC_SLOT *sl){
  assert(sl);
  return sl->cardNum;
}



void LC_Slot_SetCardNum(LC_SLOT *sl, GWEN_TYPE_UINT32 i){
  assert(sl);
  sl->cardNum=i;
}




GWEN_BUFFER *LC_Slot_GetAtr(const LC_SLOT *sl){
  assert(sl);
  return sl->atr;
}



void LC_Slot_SetAtr(LC_SLOT *sl, GWEN_BUFFER *atr){
  assert(sl);
  GWEN_Buffer_free(sl->atr);
  sl->atr=atr;
}



GWEN_TYPE_UINT32 LC_Slot_GetLastStatus(const LC_SLOT *sl){
  assert(sl);
  return sl->lastStatus;
}



void LC_Slot_SetLastStatus(LC_SLOT *sl, GWEN_TYPE_UINT32 s){
  assert(sl);
  sl->lastStatus=s;
}



time_t LC_Slot_GetLastStatusChange(const LC_SLOT *sl){
  assert(sl);
  return sl->lastStatusChange;
}



GWEN_TYPE_UINT32 LC_Slot_GetFlags(const LC_SLOT *sl){
  assert(sl);
  return sl->flags;
}



void LC_Slot_SetFlags(LC_SLOT *sl, GWEN_TYPE_UINT32 s){
  assert(sl);
  sl->flags=s;
}



void LC_Slot_AddFlags(LC_SLOT *sl, GWEN_TYPE_UINT32 s){
  assert(sl);
  sl->flags|=s;
}



void LC_Slot_SubFlags(LC_SLOT *sl, GWEN_TYPE_UINT32 s){
  assert(sl);
  sl->flags&=~s;
}



GWEN_TYPE_UINT32 LC_Slot_GetProtocolInfo(const LC_SLOT *sl){
  assert(sl);
  return sl->protocolInfo;
}



void LC_Slot_SetProtocolInfo(LC_SLOT *sl, GWEN_TYPE_UINT32 i){
  assert(sl);
  sl->protocolInfo=i;
}



















