/***************************************************************************
    begin       : Sat Nov 13 2010
    copyright   : (C) 2010 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CARD_ZKACARD_H
#define CHIPCARD_CARD_ZKACARD_H

#include <chipcard/card.h>


CHIPCARD_API
int LC_ZkaCard_ExtendCard(LC_CARD *card);
CHIPCARD_API
int LC_ZkaCard_UnextendCard(LC_CARD *card);
CHIPCARD_API
LC_CLIENT_RESULT LC_ZkaCard_Reopen(LC_CARD *card);

/** @name General Card Data
 *
 */
/*@{*/
/**
 * Returns the card data (EF_ID) parsed into a GWEN_DB.
 */
CHIPCARD_API GWEN_DB_NODE *LC_ZkaCard_GetCardDataAsDb(const LC_CARD *card);

/**
 * Returns the raw card data (content of EF_ID). The card object remains the
 * owner of the object returned (if any), so you must not manipulate or free
 * it.
 */
CHIPCARD_API GWEN_BUFFER *LC_ZkaCard_GetCardDataAsBuffer(const LC_CARD *card);
/*@}*/


#endif

