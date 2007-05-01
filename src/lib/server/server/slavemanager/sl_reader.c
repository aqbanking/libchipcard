/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: cl_request_p.h 211 2006-09-07 23:57:04Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "sl_reader_p.h"
#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>


GWEN_INHERIT(LCCO_READER, LCSL_READER)



void LCSL_Reader_Extend(LCCO_READER *r) {
  LCSL_READER *xr;

  GWEN_NEW_OBJECT(LCSL_READER, xr);
  GWEN_INHERIT_SETDATA(LCCO_READER, LCSL_READER, r, xr,
                       LCSL_Reader_FreeData);
  xr->insertedCards=LCCO_Card_List2_new();
  xr->removedCards=LCCO_Card_List2_new();
}



void GWENHYWFAR_CB LCSL_Reader_FreeData(void *bp, void *p) {
  LCSL_READER *xr;

  xr=(LCSL_READER*) p;
  LCCO_Card_List2_freeAll(xr->removedCards);
  LCCO_Card_List2_freeAll(xr->insertedCards);
  GWEN_FREE_OBJECT(xr);
}



LCCO_CARD *LCSL_Reader_GetNextInsertedCard(LCCO_READER *r) {
  LCSL_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCSL_READER, r);
  assert(xr);

  if (LCCO_Card_List2_GetSize(xr->insertedCards)) {
    LCCO_CARD *card;

    card=LCCO_Card_List2_GetFront(xr->insertedCards);
    LCCO_Card_List2_PopFront(xr->insertedCards);
    return card;
  }
  return 0;
}



void LCSL_Reader_AddInsertedCard(LCCO_READER *r, LCCO_CARD *card) {
  LCSL_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCSL_READER, r);
  assert(xr);

  assert(card);
  /* attach to card so it won't be free'd before we allow it */
  LCCO_Card_Attach(card);
  LCCO_Card_List2_PushBack(xr->insertedCards, card);
}



LCCO_CARD *LCSL_Reader_GetNextRemovedCard(LCCO_READER *r) {
  LCSL_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCSL_READER, r);
  assert(xr);

  if (LCCO_Card_List2_GetSize(xr->removedCards)) {
    LCCO_CARD *card;

    card=LCCO_Card_List2_GetFront(xr->removedCards);
    LCCO_Card_List2_PopFront(xr->removedCards);
    return card;
  }
  return 0;
}



void LCSL_Reader_AddRemovedCard(LCCO_READER *r, LCCO_CARD *card) {
  LCCO_CARD_LIST2_ITERATOR *it;
  LCSL_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCSL_READER, r);
  assert(xr);


  assert(card);

  /* check whether this card is already in the list of inserted cards */
  it=LCCO_Card_List2_First(xr->insertedCards);
  if (it) {
    LCCO_CARD *c;

    c=LCCO_Card_List2Iterator_Data(it);
    assert(c);
    while(c) {
      if (c==card) {
        /* as a matter of fact, it is. Remove it from that list and don't add
         * it to the list of removed cards, because the master doesn't know
         * about the card and he would be confused if he receives a report
         * about an unknown card being removed */
        LCCO_Card_List2_Erase(xr->insertedCards, it);
        /* we need to call LCCO_Card_free() here because when adding this
         * card to the list of inserted cards we called LCCO_Card_Attach().
         * If there still is someone else attached to the card then it won't
         * be removed quite yet.
         */
        LCCO_Card_free(c);
        LCCO_Card_List2Iterator_free(it);
        return;
      }
      c=LCCO_Card_List2Iterator_Next(it);
    }
    LCCO_Card_List2Iterator_free(it);
  }

  /* the card wasn't in the list of inserted cards, so the master already
   * knows about this card (otherwise it would still be in the list of
   * inserted cards). Attach to the card so it won't be free'd before we
   * allow it and add the card to the list of removed cards */
  LCCO_Card_Attach(card);
  LCCO_Card_List2_PushBack(xr->removedCards, card);
}



GWEN_TYPE_UINT32 LCSL_Reader_GetFlags(const LCCO_READER *r) {
  LCSL_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCSL_READER, r);
  assert(xr);

  return xr->flags;
}



void LCSL_Reader_SetFlags(LCCO_READER *r, GWEN_TYPE_UINT32 fl) {
  LCSL_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCSL_READER, r);
  assert(xr);

  xr->flags=fl;
}



void LCSL_Reader_AddFlags(LCCO_READER *r, GWEN_TYPE_UINT32 fl) {
  LCSL_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCSL_READER, r);
  assert(xr);

  xr->flags|=fl;
}



void LCSL_Reader_DelFlags(LCCO_READER *r, GWEN_TYPE_UINT32 fl) {
  LCSL_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCSL_READER, r);
  assert(xr);

  xr->flags&=~fl;
}



GWEN_TYPE_UINT32 LCSL_Reader_GetMasterReaderId(const LCCO_READER *r) {
  LCSL_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCSL_READER, r);
  assert(xr);

  return xr->masterReaderId;
}



void LCSL_Reader_SetMasterReaderId(LCCO_READER *r, GWEN_TYPE_UINT32 i) {
  LCSL_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCSL_READER, r);
  assert(xr);

  xr->masterReaderId=i;
}



GWEN_TYPE_UINT32 LCSL_Reader_GetSlaveReaderId(const LCCO_READER *r) {
  LCSL_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCSL_READER, r);
  assert(xr);

  return xr->slaveReaderId;
}



void LCSL_Reader_SetSlaveReaderId(LCCO_READER *r, GWEN_TYPE_UINT32 i) {
  LCSL_READER *xr;

  assert(r);
  xr=GWEN_INHERIT_GETDATA(LCCO_READER, LCSL_READER, r);
  assert(xr);

  xr->slaveReaderId=i;
}







