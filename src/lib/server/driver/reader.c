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
#include <chipcard3/chipcard3.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>



GWEN_LIST_FUNCTIONS(LCD_READER, LCD_Reader);
GWEN_INHERIT_FUNCTIONS(LCD_READER);


LCD_READER *LCD_Reader_new(GWEN_TYPE_UINT32 readerId,
                           const char *name,
                           int port,
                           unsigned int slots,
                           GWEN_TYPE_UINT32 flags){
  LCD_READER *r;
  unsigned int i;

  GWEN_NEW_OBJECT(LCD_READER, r);
  GWEN_LIST_INIT(LCD_READER, r);
  GWEN_INHERIT_INIT(LCD_READER, r);
  r->readerId=readerId;
  if (name)
    r->name=strdup(name);
  r->port=port;
  r->readerFlags=flags;
  r->slots=LCD_Slot_List_new();
  /* create slots */
  for (i=0; i<slots; i++) {
    LCD_SLOT *sl;

    sl=LCD_Slot_new(r, i);
    LCD_Slot_List_Add(sl, r->slots);
  } /* for */

  return r;
}



void LCD_Reader_free(LCD_READER *r){
  if (r) {
    GWEN_INHERIT_FINI(LCD_READER, r);
    GWEN_LIST_FINI(LCD_READER, r);
    free(r->readerType);
    free(r->name);
    free(r->devicePath);
    LCD_Slot_List_free(r->slots);
    free(r->logger);

    GWEN_FREE_OBJECT(r);
  }
}



GWEN_TYPE_UINT32 LCD_Reader_GetReaderId(const LCD_READER *r){
  assert(r);
  return r->readerId;
}



void LCD_Reader_SetReaderId(LCD_READER *r, GWEN_TYPE_UINT32 id){
  assert(r);
  r->readerId=id;
}



GWEN_TYPE_UINT32 LCD_Reader_GetDriversReaderId(const LCD_READER *r){
  assert(r);
  return r->driversReaderId;
}



void LCD_Reader_SetDriversReaderId(LCD_READER *r, GWEN_TYPE_UINT32 id){
  assert(r);
  r->driversReaderId=id;
}



const char *LCD_Reader_GetName(const LCD_READER *r){
  assert(r);
  return r->name;
}



const char *LCD_Reader_GetReaderType(const LCD_READER *r){
  assert(r);
  return r->readerType;
}



void LCD_Reader_SetReaderType(LCD_READER *r, const char *s){
  assert(r);
  free(r->readerType);
  if (s) r->readerType=strdup(s);
  else r->readerType=0;
}



int LCD_Reader_GetPort(const LCD_READER *r){
  assert(r);
  return r->port;
}



GWEN_TYPE_UINT32 LCD_Reader_GetCardNum(const LCD_READER *r){
  assert(r);
  return r->cardNum;
}



GWEN_TYPE_UINT32 LCD_Reader_GetStatus(const LCD_READER *r){
  assert(r);
  return r->status;
}



void LCD_Reader_SetStatus(LCD_READER *r, GWEN_TYPE_UINT32 s){
  assert(r);
  r->status=s;
}



void LCD_Reader_AddStatus(LCD_READER *r, GWEN_TYPE_UINT32 s){
  assert(r);
  r->status|=s;
}



void LCD_Reader_SubStatus(LCD_READER *r, GWEN_TYPE_UINT32 s){
  assert(r);
  r->status&=~s;
}



LCD_SLOT_LIST *LCD_Reader_GetSlots(const LCD_READER *r){
  assert(r);
  return r->slots;
}



LCD_SLOT *LCD_Reader_FindSlot(const LCD_READER *r, unsigned int slotnum){
  LCD_SLOT *sl;

  assert(r);
  sl=LCD_Slot_List_First(r->slots);
  while(sl) {
    if (LCD_Slot_GetSlotNum(sl)==slotnum)
      return sl;
    sl=LCD_Slot_List_Next(sl);
  } /* while */

  return 0;
}



const char *LCD_Reader_GetLogger(const LCD_READER *r){
  assert(r);
  return r->logger;
}



void LCD_Reader_SetLogger(LCD_READER *r, const char *logDomain){
  assert(r);
  free(r->logger);
  if (logDomain) r->logger=strdup(logDomain);
  else r->logger=0;
}



GWEN_TYPE_UINT32 LCD_Reader_GetDriverFlags(const LCD_READER *r){
  assert(r);
  return r->driverFlags;
}



void LCD_Reader_SetDriverFlags(LCD_READER *r, GWEN_TYPE_UINT32 s){
  assert(r);
  r->driverFlags=s;
}



void LCD_Reader_AddDriverFlags(LCD_READER *r, GWEN_TYPE_UINT32 s){
  assert(r);
  r->driverFlags|=s;
}



void LCD_Reader_SubDriverFlags(LCD_READER *r, GWEN_TYPE_UINT32 s){
  assert(r);
  r->driverFlags&=~s;
}



GWEN_TYPE_UINT32 LCD_Reader_GetReaderFlags(const LCD_READER *r){
  assert(r);
  return r->readerFlags;
}



void LCD_Reader_SetReaderFlags(LCD_READER *r, GWEN_TYPE_UINT32 s){
  assert(r);
  r->readerFlags=s;
}



void LCD_Reader_AddReaderFlags(LCD_READER *r, GWEN_TYPE_UINT32 s){
  assert(r);
  r->readerFlags|=s;
}



void LCD_Reader_SubReaderFlags(LCD_READER *r, GWEN_TYPE_UINT32 s){
  assert(r);
  r->readerFlags&=~s;
}



const char *LCD_Reader_GetDevicePath(const LCD_READER *r) {
  assert(r);
  return r->devicePath;
}



void LCD_Reader_SetDevicePath(LCD_READER *r, const char *s) {
  assert(r);
  free(r->devicePath);
  if (s) r->devicePath=strdup(s);
  else r->devicePath=0;
}












