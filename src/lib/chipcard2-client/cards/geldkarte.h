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


#ifndef CHIPCARD_CARD_GELDKARTE_H
#define CHIPCARD_CARD_GELDKARTE_H

#include <chipcard2-client/client/card.h>
#include <chipcard2-client/cards/geldkarte_blog.h>
#include <chipcard2-client/cards/geldkarte_llog.h>
#include <chipcard2-client/cards/geldkarte_values.h>


#ifdef __cplusplus
extern "C" {
#endif


CHIPCARD_API int LC_GeldKarte_ExtendCard(LC_CARD *card);
CHIPCARD_API int LC_GeldKarte_UnextendCard(LC_CARD *card);

CHIPCARD_API LC_CLIENT_RESULT LC_GeldKarte_Reopen(LC_CARD *card);

CHIPCARD_API LC_CLIENT_RESULT LC_GeldKarte_VerifyPin(LC_CARD *card,
                                                     const char *pin);
CHIPCARD_API LC_CLIENT_RESULT LC_GeldKarte_SecureVerifyPin(LC_CARD *card);

CHIPCARD_API GWEN_DB_NODE *LC_GeldKarte_GetCardDataAsDb(const LC_CARD *card);
CHIPCARD_API
  GWEN_BUFFER *LC_GeldKarte_GetCardDataAsBuffer(const LC_CARD *card);

CHIPCARD_API
  GWEN_DB_NODE *LC_GeldKarte_GetAccountDataAsDb(const LC_CARD *card);
CHIPCARD_API
  GWEN_BUFFER *LC_GeldKarte_GetAccountDataAsBuffer(const LC_CARD *card);


CHIPCARD_API
  LC_CLIENT_RESULT LC_GeldKarte_ReadValues(LC_CARD *card,
                                           LC_GELDKARTE_VALUES *val);

CHIPCARD_API
  LC_CLIENT_RESULT LC_GeldKarte_ReadBLogs(LC_CARD *card,
                                          LC_GELDKARTE_BLOG_LIST2 *bll);

CHIPCARD_API
  LC_CLIENT_RESULT LC_GeldKarte_ReadLLogs(LC_CARD *card,
                                          LC_GELDKARTE_LLOG_LIST2 *bll);


#ifdef __cplusplus
}
#endif


#endif /* CHIPCARD_CARD_GELDKARTE_H */


