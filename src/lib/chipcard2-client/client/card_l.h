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


#ifndef CHIPCARD_CLIENT_CARD_L_H
#define CHIPCARD_CLIENT_CARD_L_H


#include <chipcard2-client/client/card.h>
#include "apps/cardcontext_l.h"


GWEN_LIST_FUNCTION_DEFS(LC_CARD, LC_Card)

LC_CARD *LC_Card_new(LC_CLIENT *cl,
                     GWEN_TYPE_UINT32 cardId,
                     GWEN_TYPE_UINT32 serverId,
                     const char *cardType,
                     GWEN_TYPE_UINT32 rflags,
                     GWEN_BUFFER *atr);

LC_CARDCONTEXT *LC_Card_GetContext(const LC_CARD *cd);
void LC_Card_SetContext(LC_CARD *cd, LC_CARDCONTEXT *ctx);

void LC_Card_ResetCardId(LC_CARD *cd);

GWEN_TYPE_UINT32 LC_Card_GetServerId(const LC_CARD *cd);
void LC_Card_SetCardType(LC_CARD *cd, const char *ct);

void LC_Card_SetLastResult(LC_CARD *cd,
                           const char *result,
                           const char *text,
                           int sw1, int sw2);

int LC_Card_AddCardType(LC_CARD *cd, const char *s);


#endif /* CHIPCARD_CLIENT_CARD_L_H */


