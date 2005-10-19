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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "cmd_card_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_INHERIT(LCCO_CARD, LCCMD_CARD)



void LCCMD_Card_extend(LCCO_CARD *cd) {
  LCCMD_CARD *dc;

  assert(cd);

  GWEN_NEW_OBJECT(LCCMD_CARD, dc);

  GWEN_INHERIT_SETDATA(LCCO_CARD, LCCMD_CARD, cd, dc,
                       LCCMD_Card_FreeData);
}



void LCCMD_Card_unextend(LCCO_CARD *cd) {
  LCCMD_CARD *dc;

  assert(cd);
  dc=GWEN_INHERIT_GETDATA(LCCO_CARD, LCCMD_CARD, cd);
  assert(dc);

  GWEN_INHERIT_UNLINK(LCCO_CARD, LCCMD_CARD, cd);
  LCCMD_Card_FreeData(cd, dc);
}



void LCCMD_Card_FreeData(void *bp, void *p) {
  LCCMD_CARD *dc;

  dc=(LCCMD_CARD*)p;

  GWEN_FREE_OBJECT(p);
}



GWEN_XMLNODE *LCCMD_Card_GetCardNode(const LCCO_CARD *cd) {
  LCCMD_CARD *dc;

  assert(cd);
  dc=GWEN_INHERIT_GETDATA(LCCO_CARD, LCCMD_CARD, cd);
  assert(dc);

  return dc->cardNode;
}



void LCCMD_Card_SetCardNode(LCCO_CARD *cd, GWEN_XMLNODE *n) {
  LCCMD_CARD *dc;

  assert(cd);
  dc=GWEN_INHERIT_GETDATA(LCCO_CARD, LCCMD_CARD, cd);
  assert(dc);

  dc->cardNode=n;
}









