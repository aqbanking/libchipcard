/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id$
    begin       : Sun Jun 13 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

/** @file examplecard.h
 *  @short Pubic header file
 *
 * This file may be included by whoever wants to. It defines the public
 * functions of this type.
 */


#ifndef CHIPCARD_CARD_KVKSCARD_H
#define CHIPCARD_CARD_KVKSCARD_H

#include <chipcard/client/card_imp.h>
#include <chipcard/client/cards/memorycard.h>

#include <gwenhywfar/buffer.h>


typedef enum {
  KVKSStatus_Error=-1,
  KVKSStatus_New=0,
  KVKSStatus_Opening,
  KVKSStatus_ReadingHeader,
  KVKSStatus_ReadingData,
  KVKSStatus_Releasing,
  KVKSStatus_Done
} KVKS_STATUS;


int KVKSCard_ExtendCard(LC_CARD *card);
int KVKSCard_UnextendCard(LC_CARD *card);

LC_CLIENT_RESULT KVKSCard_Reopen(LC_CARD *card);

GWEN_BUFFER *KVKSCard_GetBuffer(const LC_CARD *card);

KVKS_STATUS KVKSCard_GetStatus(const LC_CARD *card);
void KVKSCard_SetStatus(LC_CARD *card, KVKS_STATUS st);


GWEN_TYPE_UINT32 KVKSCard_GetCurrentRequest(const LC_CARD *card);
void KVKSCard_SetCurrentRequest(LC_CARD *card, GWEN_TYPE_UINT32 i);

GWEN_DB_NODE *KVKSCard_GetDbCardData(const LC_CARD *card);
/* takes over ownership */
void KVKSCard_SetDbCardData(LC_CARD *card, GWEN_DB_NODE *db);


int KVKSCard_GetCheckSumOk(const LC_CARD *card);
void KVKSCard_SetCheckSumOk(LC_CARD *card, int b);


#endif /* CHIPCARD_CARD_KVKSCARD_P_H */




