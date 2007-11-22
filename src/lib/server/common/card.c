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


#include "card_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/inherit.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_LIST_FUNCTIONS(LCCO_CARD, LCCO_Card)
GWEN_LIST2_FUNCTIONS(LCCO_CARD, LCCO_Card)
GWEN_INHERIT_FUNCTIONS(LCCO_CARD)




LCCO_CARD *LCCO_Card_new(uint32_t cardId){
  LCCO_CARD *cd;

  /* create and init */
  GWEN_NEW_OBJECT(LCCO_CARD, cd);
  DBG_MEM_INC("LCCO_CARD", 0);
  GWEN_LIST_INIT(LCCO_CARD, cd);
  GWEN_INHERIT_INIT(LCCO_CARD, cd);
  cd->usage=1;

  /* preset reasonable defaults, prepare structs etc */
  cd->cardType=LC_CardTypeUnknown;
  cd->atr=GWEN_Buffer_new(0, 256, 0, 1);
  cd->cardTypes=GWEN_StringList_new();

  return cd;
}



void LCCO_Card_free(LCCO_CARD *cd){
  if (cd) {
    assert(cd->usage);
    if (--(cd->usage)==0) {
      DBG_MEM_DEC("LCCO_CARD");
      GWEN_INHERIT_FINI(LCCO_CARD, cd);
      GWEN_LIST_FINI(LCCO_CARD, cd);
      GWEN_StringList_free(cd->cardTypes);
      free(cd->readerType);
      free(cd->driverType);
      GWEN_Buffer_free(cd->atr);
      GWEN_FREE_OBJECT(cd);
    }
  }
}



void LCCO_Card_Attach(LCCO_CARD *cd){
  assert(cd->usage);
  cd->usage++;
}



void LCCO_Card_List2_freeAll(LCCO_CARD_LIST2 *cl) {
  if (cl) {
    LCCO_CARD_LIST2_ITERATOR *it;

    it=LCCO_Card_List2_First(cl);
    if (it) {
      LCCO_CARD *card;

      card=LCCO_Card_List2Iterator_Data(it);
      assert(card);
      while(card) {
        LCCO_Card_free(card);
        card=LCCO_Card_List2Iterator_Next(it);
      }
      LCCO_Card_List2Iterator_free(it);
    }
    LCCO_Card_List2_free(cl);
  }
}



uint32_t LCCO_Card_GetCardId(const LCCO_CARD *cd){
  assert(cd);
  return cd->cardId;
}



void LCCO_Card_SetCardId(LCCO_CARD *cd, uint32_t id) {
  assert(cd);
  cd->cardId=id;
}



uint32_t LCCO_Card_GetReaderId(const LCCO_CARD *cd) {
  assert(cd);
  return cd->readerId;
}



void LCCO_Card_SetReaderId(LCCO_CARD *cd, uint32_t id) {
  assert(cd);
  cd->readerId=id;
}



uint32_t LCCO_Card_GetDriversReaderId(const LCCO_CARD *cd) {
  assert(cd);
  return cd->driversReaderId;
}



void LCCO_Card_SetDriversReaderId(LCCO_CARD *cd, uint32_t id) {
  assert(cd);
  cd->driversReaderId=id;
}



uint32_t LCCO_Card_GetReadersCardId(const LCCO_CARD *cd) {
  assert(cd);
  return cd->readersCardId;
}



void LCCO_Card_SetReadersCardId(LCCO_CARD *cd, uint32_t id) {
  assert(cd);
  cd->readersCardId=id;
}



int LCCO_Card_GetSlotNum(const LCCO_CARD *cd) {
  assert(cd);
  return cd->slotNum;
}



void LCCO_Card_SetSlotNum(LCCO_CARD *cd, int i) {
  assert(cd);
  cd->slotNum=i;
}



uint32_t LCCO_Card_GetCardNum(const LCCO_CARD *cd) {
  assert(cd);
  return cd->cardNum;
}



void LCCO_Card_SetCardNum(LCCO_CARD *cd, uint32_t i) {
  assert(cd);
  cd->cardNum=i;
}



LC_CARD_STATUS LCCO_Card_GetStatus(const LCCO_CARD *cd){
  assert(cd);
  return cd->status;
}



void LCCO_Card_SetStatus(LCCO_CARD *cd, LC_CARD_STATUS st){
  assert(cd);
  if (cd->status!=st) {
    DBG_DEBUG(0, "Changing status of card in reader \"%s\" "
              "(%08x) from %d to %d",
              cd->readerType, cd->readerId, cd->status, st);
  }
  cd->status=st;
}



const char *LCCO_Card_GetAtr(const LCCO_CARD *cd, unsigned int *len) {
  assert(cd);
  *len=GWEN_Buffer_GetUsedBytes(cd->atr);
  if (*len==0)
    return 0;
  return GWEN_Buffer_GetStart(cd->atr);
}



void LCCO_Card_SetAtr(LCCO_CARD *cd,
                      const char *s, unsigned int len) {
  assert(cd);
  GWEN_Buffer_Reset(cd->atr);
  if (s && len)
    GWEN_Buffer_AppendBytes(cd->atr, s, len);
}



const char *LCCO_Card_GetDriverTypeName(const LCCO_CARD *cd) {
  assert(cd);
  return cd->driverType;
}



void LCCO_Card_SetDriverTypeName(LCCO_CARD *cd, const char *s) {
  assert(cd);
  free(cd->driverType);
  if (s) cd->driverType=strdup(s);
  else cd->driverType=0;
}



const char *LCCO_Card_GetReaderTypeName(const LCCO_CARD *cd) {
  assert(cd);
  return cd->readerType;
}



void LCCO_Card_SetReaderTypeName(LCCO_CARD *cd, const char *s) {
  assert(cd);
  free(cd->readerType);
  if (s) cd->readerType=strdup(s);
  else cd->readerType=0;
}



uint32_t LCCO_Card_GetReaderFlags(const LCCO_CARD *cd) {
  assert(cd);
  return cd->readerFlags;
}



void LCCO_Card_SetReaderFlags(LCCO_CARD *cd, uint32_t fl) {
  assert(cd);
  cd->readerFlags=fl;
}



void LCCO_Card_AddReaderFlags(LCCO_CARD *cd, uint32_t fl) {
  assert(cd);
  cd->readerFlags|=fl;
}



void LCCO_Card_SubReaderFlags(LCCO_CARD *cd, uint32_t fl) {
  assert(cd);
  cd->readerFlags&=~fl;
}



LC_CARD_TYPE LCCO_Card_GetCardType(const LCCO_CARD *cd) {
  assert(cd);
  return cd->cardType;
}



void LCCO_Card_SetCardType(LCCO_CARD *cd, LC_CARD_TYPE ct) {
  assert(cd);
  cd->cardType=ct;
}



const GWEN_STRINGLIST *LCCO_Card_GetTypes(const LCCO_CARD *cd) {
  assert(cd);
  return cd->cardTypes;
}



int LCCO_Card_AddType(LCCO_CARD *cd, const char *s) {
  assert(cd);
  return GWEN_StringList_AppendString(cd->cardTypes, s, 0, 1);
}



uint32_t LCCO_Card_GetLockId(const LCCO_CARD *cd) {
  assert(cd);
  return cd->lockId;
}



void LCCO_Card_SetLockId(LCCO_CARD *cd, uint32_t lid) {
  assert(cd);
  cd->lockId=lid;
}



void LCCO_Card_Dump(const LCCO_CARD *cd, FILE *f, int indent) {
  int i;
  GWEN_STRINGLISTENTRY *se;

  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "--------------------------\n");
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Card\n");
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Id        : %08x\n", cd->cardId);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Card Num  : %08x\n", cd->readersCardId);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Type      : ");
  switch(cd->cardType) {
  case LC_CardTypeProcessor: fprintf(f, "processor\n"); break;
  case LC_CardTypeMemory:    fprintf(f, "memory\n"); break;
  default:                   fprintf(f, "unknown (%d)\n", cd->cardType);
  }

  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Types     :");
  se=GWEN_StringList_FirstEntry(cd->cardTypes);
  while(se) {
    fprintf(f, " %s", GWEN_StringListEntry_Data(se));
    se=GWEN_StringListEntry_Next(se);
  }
  fprintf(f, "\n");

  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Status    : ");
  switch(cd->status) {
  case LC_CardStatusInserted: fprintf(f, "inserted\n"); break;
  case LC_CardStatusRemoved:  fprintf(f, "removed\n"); break;
  case LC_CardStatusOrphaned: fprintf(f, "orphaned\n"); break;
  default:                    fprintf(f, "unknown (%d)\n", cd->status);
  }

  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Reader Id : %08x\n", cd->readerId);

  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Reader    : %s \n", cd->readerType);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Driver    : %s \n", cd->driverType);
}






