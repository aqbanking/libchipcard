/***************************************************************************
    begin       : Wed Nov 26 2008
    copyright   : (C) 2008 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CARD_ZKACARD_H
#define CHIPCARD_CARD_ZKACARD_H

#include <chipcard/client/card.h>


CHIPCARD_API
int LC_ZkaCard_ExtendCard(LC_CARD *card);
CHIPCARD_API
int LC_ZkaCard_UnextendCard(LC_CARD *card);
CHIPCARD_API
LC_CLIENT_RESULT LC_ZkaCard_Reopen(LC_CARD *card);


#endif

