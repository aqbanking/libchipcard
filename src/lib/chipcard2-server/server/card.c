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
  cd->types=GWEN_StringList_new();
  return cd;
}



void LC_Card_free(LC_CARD *cd){
  if (cd) {
    DBG_MEM_DEC("LC_CARD");
    while(cd->waitingClientCount--) {
      LC_Reader_DecUsageCount(cd->reader);
    }
    GWEN_LIST_FINI(LC_CARD, cd);
    GWEN_StringList_free(cd->types);
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



const GWEN_STRINGLIST *LC_Card_GetTypes(const LC_CARD *cd){
  assert(cd);
  return cd->types;
}



int LC_Card_AddType(LC_CARD *cd, const char *s) {
  assert(cd);
  return GWEN_StringList_AppendString(cd->types, s, 0, 1);
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
    if (cl)
      LC_Reader_IncUsageCount(cd->reader);
    if (cd->client)
      LC_Reader_DecUsageCount(cd->reader);
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



int LC_Card_AddWaitingClient(LC_CARD *cd, GWEN_TYPE_UINT32 id){
  assert(cd);
  if (!GWEN_IdList_AddId(cd->waitingClients, id)) {
    cd->waitingClientCount++;
    LC_Reader_IncUsageCount(cd->reader);
    return 0;
  }
  return -1;
}



int LC_Card_DelWaitingClient(LC_CARD *cd, GWEN_TYPE_UINT32 id){
  assert(cd);
  if (!GWEN_IdList_DelId(cd->waitingClients, id)) {
    cd->waitingClientCount--;
    LC_Reader_DecUsageCount(cd->reader);
    return 0;
  }
  return -1;
}



void LC_Card_ClearWaitingClients(LC_CARD *cd){
  assert(cd);
  GWEN_IdList_Clear(cd->waitingClients);
  while(cd->waitingClientCount--) {
    LC_Reader_DecUsageCount(cd->reader);
  }
  cd->waitingClientCount=0;
}



int LC_Card_WaitingClientCount(const LC_CARD *cd){
  assert(cd);
  return cd->waitingClientCount;
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



void LC_Card_Dump(const LC_CARD *cd, FILE *f, int indent) {
  int i;

  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "--------------------------\n");
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Card\n");
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Id : %08x\n", cd->cardId);
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Type : ");
  switch(cd->type) {
  case LC_CardTypeProcessor: fprintf(f, "processor\n"); break;
  case LC_CardTypeMemory:    fprintf(f, "memory\n"); break;
  default:                   fprintf(f, "unknown (%d)\n", cd->type);
  }

  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Status : ");
  switch(cd->status) {
  case LC_CardStatusInserted: fprintf(f, "inserted\n"); break;
  case LC_CardStatusRemoved:  fprintf(f, "removed\n"); break;
  case LC_CardStatusOrphaned: fprintf(f, "orphaned\n"); break;
  default:                    fprintf(f, "unknown (%d)\n", cd->status);
  }

  if (cd->client) {
    for (i=0; i<indent; i++)
      fprintf(f, " ");
    fprintf(f, "UsedBy : %08x\n",
            LC_Client_GetClientId(cd->client));
  }
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Reader : %s (%08x)\n",
          LC_Reader_GetReaderName(cd->reader),
          LC_Reader_GetReaderId(cd->reader));
  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Slot : %d\n", cd->slot);

  for (i=0; i<indent; i++)
    fprintf(f, " ");
  fprintf(f, "Waiting Clients : %d\n", cd->waitingClientCount);
}






