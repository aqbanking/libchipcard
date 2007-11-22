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

#include "cardpcsc_p.h"
#include <gwenhywfar/gwenhywfar.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>


GWEN_INHERIT(LC_CARD, LC_CARD_PCSC)



LC_CARD *LC_CardPcsc_new(LC_CLIENT *cl,
                         uint32_t cardId,
                         SCARDHANDLE scardHandle,
                         const char *readerName,
                         DWORD protocol,
                         const char *cardType,
                         uint32_t rflags,
                         const unsigned char *atrBuf,
                         unsigned int atrLen) {
  LC_CARD *card;
  LC_CARD_PCSC *xcard;

  assert(readerName);
  card=LC_Card_new(cl, cardId, cardType, rflags, atrBuf, atrLen);
  GWEN_NEW_OBJECT(LC_CARD_PCSC, xcard);
  GWEN_INHERIT_SETDATA(LC_CARD, LC_CARD_PCSC, card, xcard,
                       LC_CardPcsc_FreeData);
  xcard->readerName=strdup(readerName);
  xcard->scardHandle=scardHandle;
  xcard->protocol=protocol;

  return card;
}



void GWENHYWFAR_CB LC_CardPcsc_FreeData(void *bp, void *p) {
  LC_CARD_PCSC *xcard;

  xcard=(LC_CARD_PCSC*)p;
  free(xcard->readerName);
  GWEN_FREE_OBJECT(xcard);
}



SCARDHANDLE LC_CardPcsc_GetScardHandle(const LC_CARD *card) {
  LC_CARD_PCSC *xcard;

  assert(card);
  xcard=GWEN_INHERIT_GETDATA(LC_CARD, LC_CARD_PCSC, card);
  assert(xcard);

  return xcard->scardHandle;
}



const char *LC_CardPcsc_GetReaderName(const LC_CARD *card) {
  LC_CARD_PCSC *xcard;

  assert(card);
  xcard=GWEN_INHERIT_GETDATA(LC_CARD, LC_CARD_PCSC, card);
  assert(xcard);

  return xcard->readerName;
}



DWORD LC_CardPcsc_GetProtocol(const LC_CARD *card) {
  LC_CARD_PCSC *xcard;

  assert(card);
  xcard=GWEN_INHERIT_GETDATA(LC_CARD, LC_CARD_PCSC, card);
  assert(xcard);

  return xcard->protocol;
}









