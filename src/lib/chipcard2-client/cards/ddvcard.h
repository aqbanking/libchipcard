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


#ifndef CHIPCARD_CARD_DDVCARD_H
#define CHIPCARD_CARD_DDVCARD_H

#include <chipcard2-client/client/card.h>

#ifdef __cplusplus
extern "C" {
#endif


CHIPCARD_API int LC_DDVCard_ExtendCard(LC_CARD *card);
CHIPCARD_API int LC_DDVCard_UnextendCard(LC_CARD *card);

CHIPCARD_API LC_CLIENT_RESULT LC_DDVCard_Reopen(LC_CARD *card);

CHIPCARD_API LC_CLIENT_RESULT LC_DDVCard_VerifyPin(LC_CARD *card,
                                                   const char *pin);
CHIPCARD_API LC_CLIENT_RESULT LC_DDVCard_SecureVerifyPin(LC_CARD *card);
CHIPCARD_API LC_CLIENT_RESULT LC_DDVCard_GetChallenge(LC_CARD *card,
                                                      GWEN_BUFFER *mbuf);

CHIPCARD_API LC_CLIENT_RESULT LC_DDVCard_CryptBlock(LC_CARD *card,
                                                    GWEN_BUFFER *ibuf,
                                                    GWEN_BUFFER *obuf);
LC_CLIENT_RESULT LC_DDVCard_CryptCharBlock(LC_CARD *card,
                                           const char *data,
                                           unsigned int dlen,
                                           GWEN_BUFFER *obuf);

CHIPCARD_API LC_CLIENT_RESULT LC_DDVCard_SignHash(LC_CARD *card,
                                                  GWEN_BUFFER *hbuf,
                                                  GWEN_BUFFER *obuf);

CHIPCARD_API GWEN_DB_NODE *LC_DDVCard_GetCardDataAsDb(const LC_CARD *card);

/**
 * Does not relinquish ownership.
 */
CHIPCARD_API GWEN_BUFFER *LC_DDVCard_GetCardDataAsBuffer(const LC_CARD *card);

CHIPCARD_API
  LC_CLIENT_RESULT LC_DDVCard_ReadInstituteData(LC_CARD *card,
                                                int idx,
                                                GWEN_DB_NODE *dbData);

CHIPCARD_API 
  LC_CLIENT_RESULT LC_DDVCard_WriteInstituteData(LC_CARD *card,
                                                 int idx,
                                                 GWEN_DB_NODE *dbData);

CHIPCARD_API int LC_DDVCard_GetSignKeyVersion(LC_CARD *card);
CHIPCARD_API int LC_DDVCard_GetSignKeyNumber(LC_CARD *card);
CHIPCARD_API int LC_DDVCard_GetCryptKeyVersion(LC_CARD *card);
CHIPCARD_API int LC_DDVCard_GetCryptKeyNumber(LC_CARD *card);


#ifdef __cplusplus
}
#endif


#endif /* CHIPCARD_CARD_DDVCARD_H */


