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



#ifndef CHIPCARD_SERVER_CARD_H
#define CHIPCARD_SERVER_CARD_H


typedef struct LC_CARD LC_CARD;


#include <gwenhywfar/buffer.h>
#include <gwenhywfar/misc.h>
#include <chipcard2-server/server/reader.h>
#include <chipcard2-server/server/client.h>

#include <time.h>
#include <stdio.h>


typedef enum {
  LC_CardStatusInserted=0,
  LC_CardStatusRemoved,
  LC_CardStatusOrphaned,

  LC_CardStatusUnknown=999
} LC_CARD_STATUS;


typedef enum {
  LC_CardTypeUnknown=0,
  LC_CardTypeProcessor,
  LC_CardTypeMemory
} LC_CARD_TYPE;


GWEN_LIST_FUNCTION_DEFS(LC_CARD, LC_Card);


LC_CARD *LC_Card_new(LC_READER *r,
                     unsigned int slot,
                     GWEN_TYPE_UINT32 readersCardId,
                     LC_CARD_TYPE ct,
                     GWEN_BUFFER *atr);
void LC_Card_free(LC_CARD *cd);


GWEN_TYPE_UINT32 LC_Card_GetCardId(const LC_CARD *cd);

LC_CARD_STATUS LC_Card_GetStatus(const LC_CARD *cd);
void LC_Card_SetStatus(LC_CARD *cd, LC_CARD_STATUS st);

LC_READER *LC_Card_GetReader(const LC_CARD *cd);
unsigned int LC_Card_GetSlot(const LC_CARD *cd);
GWEN_TYPE_UINT32 LC_Card_GetReadersCardId(const LC_CARD *cd);

LC_CLIENT *LC_Card_GetClient(const LC_CARD *cd);
void LC_Card_SetClient(LC_CARD *cd, LC_CLIENT *cl);

LC_CLIENT *LC_Card_GetRealClient(const LC_CARD *cd);
void LC_Card_SetRealClient(LC_CARD *cd, LC_CLIENT *cl);

time_t LC_Card_GetBusySince(const LC_CARD *cd);

GWEN_BUFFER *LC_Card_GetAtr(const LC_CARD *cd);

LC_CARD_TYPE LC_Card_GetType(const LC_CARD *cd);

GWEN_TYPE_UINT32 LC_Card_GetFirstWaitingClient(const LC_CARD *cd);
int LC_Card_AddWaitingClient(LC_CARD *cd, GWEN_TYPE_UINT32 id);
int LC_Card_DelWaitingClient(LC_CARD *cd, GWEN_TYPE_UINT32 id);
void LC_Card_ClearWaitingClients(LC_CARD *cd);
int LC_Card_WaitingClientCount(const LC_CARD *cd);


void LC_Card_Dump(const LC_CARD *cd, FILE *f, int indent);


#endif /* CHIPCARD_SERVER_CARD_H */





