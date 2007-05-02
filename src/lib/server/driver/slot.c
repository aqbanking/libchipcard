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
#include <chipcard/chipcard.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>




GWEN_LIST_FUNCTIONS(LCD_SLOT, LCD_Slot);


LCD_SLOT *LCD_Slot_new(LCD_READER *r, unsigned int slotNum){
  LCD_SLOT *sl;

  GWEN_NEW_OBJECT(LCD_SLOT, sl);
  GWEN_LIST_INIT(LCD_SLOT, sl);
  sl->reader=r;
  sl->slotNum=slotNum;

  return sl;
}



void LCD_Slot_free(LCD_SLOT *sl){
  if (sl) {
    GWEN_LIST_FINI(LCD_SLOT, sl);
    GWEN_Buffer_free(sl->atr);

    GWEN_FREE_OBJECT(sl);
  }
}



GWEN_TYPE_UINT32 LCD_Slot_GetStatus(const LCD_SLOT *sl){
  assert(sl);
  return sl->status;
}



void LCD_Slot_SetStatus(LCD_SLOT *sl, GWEN_TYPE_UINT32 s){
  assert(sl);
  if (sl->status!=s) {
    sl->status=s;
    sl->lastStatusChange=time(0);
  }
}



void LCD_Slot_AddStatus(LCD_SLOT *sl, GWEN_TYPE_UINT32 s){
  GWEN_TYPE_UINT32 nst;

  assert(sl);
  nst=sl->status|s;
  if (sl->status!=nst) {
    sl->status=nst;
    sl->lastStatusChange=time(0);
  }
}



void LCD_Slot_SubStatus(LCD_SLOT *sl, GWEN_TYPE_UINT32 s){
  GWEN_TYPE_UINT32 nst;

  assert(sl);
  nst=sl->status&~s;
  if (sl->status!=nst) {
    sl->status=nst;
    sl->lastStatusChange=time(0);
  }
}



LCD_READER *LCD_Slot_GetReader(const LCD_SLOT *sl){
  assert(sl);
  return sl->reader;
}



unsigned int LCD_Slot_GetSlotNum(const LCD_SLOT *sl){
  assert(sl);
  return sl->slotNum;
}



GWEN_TYPE_UINT32 LCD_Slot_GetCardNum(const LCD_SLOT *sl){
  assert(sl);
  return sl->cardNum;
}



void LCD_Slot_SetCardNum(LCD_SLOT *sl, GWEN_TYPE_UINT32 i){
  assert(sl);
  sl->cardNum=i;
}




GWEN_BUFFER *LCD_Slot_GetAtr(const LCD_SLOT *sl){
  assert(sl);
  return sl->atr;
}



void LCD_Slot_SetAtr(LCD_SLOT *sl, GWEN_BUFFER *atr){
  assert(sl);
  if (sl->atr!=atr) {
    GWEN_Buffer_free(sl->atr);
    sl->atr=atr;
  }
}



GWEN_TYPE_UINT32 LCD_Slot_GetLastStatus(const LCD_SLOT *sl){
  assert(sl);
  return sl->lastStatus;
}



void LCD_Slot_SetLastStatus(LCD_SLOT *sl, GWEN_TYPE_UINT32 s){
  assert(sl);
  sl->lastStatus=s;
}



time_t LCD_Slot_GetLastStatusChange(const LCD_SLOT *sl){
  assert(sl);
  return sl->lastStatusChange;
}



GWEN_TYPE_UINT32 LCD_Slot_GetFlags(const LCD_SLOT *sl){
  assert(sl);
  return sl->flags;
}



void LCD_Slot_SetFlags(LCD_SLOT *sl, GWEN_TYPE_UINT32 s){
  assert(sl);
  sl->flags=s;
}



void LCD_Slot_AddFlags(LCD_SLOT *sl, GWEN_TYPE_UINT32 s){
  assert(sl);
  sl->flags|=s;
}



void LCD_Slot_SubFlags(LCD_SLOT *sl, GWEN_TYPE_UINT32 s){
  assert(sl);
  sl->flags&=~s;
}



GWEN_TYPE_UINT32 LCD_Slot_GetProtocolInfo(const LCD_SLOT *sl){
  assert(sl);
  return sl->protocolInfo;
}



void LCD_Slot_SetProtocolInfo(LCD_SLOT *sl, GWEN_TYPE_UINT32 i){
  assert(sl);
  sl->protocolInfo=i;
}



















