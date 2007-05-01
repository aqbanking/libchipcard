/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: client_p.h 137 2005-11-03 13:07:50Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "cardlcc_p.h"
#include <gwenhywfar/gwenhywfar.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>


GWEN_INHERIT(LC_CARD, LC_CARD_LCC)



LC_CARD *LC_CardLcc_new(LC_CLIENT *cl,
                        GWEN_TYPE_UINT32 cardId,
                        GWEN_TYPE_UINT32 serverId,
                        const char *cardType,
                        GWEN_TYPE_UINT32 rflags,
                        const unsigned char *atrBuf,
                        unsigned int atrLen) {
  LC_CARD *card;
  LC_CARD_LCC *xcard;

  card=LC_Card_new(cl, cardId, cardType, rflags, atrBuf, atrLen);
  GWEN_NEW_OBJECT(LC_CARD_LCC, xcard);
  GWEN_INHERIT_SETDATA(LC_CARD, LC_CARD_LCC, card, xcard,
                       LC_CardLcc_FreeData);
  xcard->serverId=serverId;

  return card;
}




void GWENHYWFAR_CB LC_CardLcc_FreeData(void *bp, void *p) {
  LC_CARD_LCC *xcard;

  xcard=(LC_CARD_LCC*) p;

  GWEN_FREE_OBJECT(xcard);
}



GWEN_TYPE_UINT32 LC_CardLcc_GetServerId(const LC_CARD *card) {
  LC_CARD_LCC *xcard;

  assert(card);
  xcard=GWEN_INHERIT_GETDATA(LC_CARD, LC_CARD_LCC, card);
  assert(xcard);

  return xcard->serverId;
}



void LC_CardLcc_SetConnected(LC_CARD *card, int b) {
  LC_CARD_LCC *xcard;

  assert(card);
  xcard=GWEN_INHERIT_GETDATA(LC_CARD, LC_CARD_LCC, card);
  assert(xcard);

  xcard->connected=b;
}



int LC_CardLcc_IsConnected(const LC_CARD *card) {
  LC_CARD_LCC *xcard;

  assert(card);
  xcard=GWEN_INHERIT_GETDATA(LC_CARD, LC_CARD_LCC, card);
  assert(xcard);

  return xcard->connected;
}









