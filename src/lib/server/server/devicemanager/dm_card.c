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


#include "dm_card_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/inherit.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


GWEN_INHERIT(LCCO_CARD, LCDM_CARD)


void LCDM_Card_extend(LCCO_CARD *cd,
                      LCCO_READER *r) {
  LCDM_CARD *dc;

  assert(cd);

  GWEN_NEW_OBJECT(LCDM_CARD, dc);
  dc->reader=r;
  LCCO_Reader_Attach(dc->reader);
  GWEN_INHERIT_SETDATA(LCCO_CARD, LCDM_CARD, cd, dc,
                       LCDM_Card_FreeData);
}



void LCDM_Card_unextend(LCCO_CARD *cd) {
  LCDM_CARD *dc;

  assert(cd);
  dc=GWEN_INHERIT_GETDATA(LCCO_CARD, LCDM_CARD, cd);
  assert(dc);

  GWEN_INHERIT_UNLINK(LCCO_CARD, LCDM_CARD, cd);
  LCDM_Card_FreeData(cd, dc);
}



void GWENHYWFAR_CB LCDM_Card_FreeData(void *bp, void *p) {
  LCDM_CARD *dc;

  dc=(LCDM_CARD*)p;
  LCCO_Reader_free(dc->reader);
  GWEN_FREE_OBJECT(p);
}



LCCO_READER *LCDM_Card_GetReader(const LCCO_CARD *cd) {
  LCDM_CARD *dc;

  assert(cd);
  dc=GWEN_INHERIT_GETDATA(LCCO_CARD, LCDM_CARD, cd);
  assert(dc);

  return dc->reader;
}





