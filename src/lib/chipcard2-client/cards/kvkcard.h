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


#ifndef CHIPCARD_CARD_KVKCARD_H
#define CHIPCARD_CARD_KVKCARD_H

#include <chipcard2-client/client/card.h>


CHIPCARD_API
int LC_KVKCard_ExtendCard(LC_CARD *card);
CHIPCARD_API
int LC_KVKCard_UnextendCard(LC_CARD *card);

CHIPCARD_API
GWEN_DB_NODE *LC_KVKCard_GetCardData(const LC_CARD *card);

CHIPCARD_API
LC_CLIENT_RESULT LC_KVKCard_Reopen(LC_CARD *card);


#endif /* CHIPCARD_CARD_KVKCARD_P_H */




