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


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_LIST_FUNCTIONS(LC_CARD, LC_Card);


static GWEN_TYPE_UINT32 LC_Card_LastId=0;



LC_CARD *LC_Card_new(LC_READER *r, unsigned int slot,
                     GWEN_TYPE_UINT32 readersCardId,
                     LC_CARD_TYPE ct, GWEN_BUFFER *atr){
  LC_CARD *cd;

  assert(r);
  assert(readersCardId);
  GWEN_NEW_OBJECT(LC_CARD, cd);
  DBG_MEM_INC("LC_CARD", 0);
  GWEN_LIST_INIT(LC_CARD, cd);
  cd->type=ct;
  cd->reader=r;
  cd->slot=slot;
  cd->readersCardId=readersCardId;
  cd->atr=atr;
  if (LC_Card_LastId==0)
    LC_Card_LastId=time(0);
  cd->cardId=++LC_Card_LastId;
  cd->waitingClients=GWEN_IdList_new();

  return cd;
}



void LC_Card_free(LC_CARD *cd){
  if (cd) {
    DBG_MEM_DEC("LC_CARD");
    GWEN_LIST_FINI(LC_CARD, cd);
    GWEN_Buffer_free(cd->atr);
    GWEN_IdList_free(cd->waitingClients);
    LC_CardContext_free(cd->cardContext);
    GWEN_FREE_OBJECT(cd);
  }
}



GWEN_TYPE_UINT32 LC_Card_GetCardId(const LC_CARD *cd){
  assert(cd);
  return cd->cardId;
}



LC_CARD_STATUS LC_Card_GetStatus(const LC_CARD *cd){
  assert(cd);
  return cd->status;
}



void LC_Card_SetStatus(LC_CARD *cd, LC_CARD_STATUS st){
  assert(cd);
  cd->status=st;
}



LC_READER *LC_Card_GetReader(const LC_CARD *cd){
  assert(cd);
  return cd->reader;
}



LC_CARD_TYPE LC_Card_GetType(const LC_CARD *cd){
  assert(cd);
  return cd->type;
}



unsigned int LC_Card_GetSlot(const LC_CARD *cd){
  assert(cd);
  return cd->slot;
}



GWEN_TYPE_UINT32 LC_Card_GetReadersCardId(const LC_CARD *cd){
  assert(cd);
  return cd->readersCardId;
}



LC_CLIENT *LC_Card_GetClient(const LC_CARD *cd){
  assert(cd);
  return cd->client;
}



void LC_Card_SetClient(LC_CARD *cd, LC_CLIENT *cl){
  assert(cd);
  if (cd->client!=cl) {
    cd->busySince=time(0);
    cd->client=cl;
  }
}



GWEN_BUFFER *LC_Card_GetAtr(const LC_CARD *cd){
  assert(cd);
  return cd->atr;
}



time_t LC_Card_GetBusySince(const LC_CARD *cd){
  assert(cd);
  return cd->busySince;
}



GWEN_TYPE_UINT32 LC_Card_GetFirstWaitingClient(const LC_CARD *cd){
  assert(cd);
  return GWEN_IdList_GetFirstId(cd->waitingClients);
}



void LC_Card_AddWaitingClient(LC_CARD *cd, GWEN_TYPE_UINT32 id){
  assert(cd);
  GWEN_IdList_AddId(cd->waitingClients, id);
}



void LC_Card_DelWaitingClient(LC_CARD *cd, GWEN_TYPE_UINT32 id){
  assert(cd);
  GWEN_IdList_DelId(cd->waitingClients, id);
}



LC_CARDCONTEXT *LC_Card_GetContext(const LC_CARD *cd){
  assert(cd);
  return cd->cardContext;
}



void LC_Card_SetContext(LC_CARD *cd, LC_CARDCONTEXT *ctx){
  assert(cd);
  LC_CardContext_free(cd->cardContext);
  cd->cardContext=ctx;
}













