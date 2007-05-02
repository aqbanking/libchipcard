/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: egkcard_p.h 325 2006-10-09 15:29:02Z martin $
    begin       : Tue Feb 20 2007
    copyright   : (C) 2007 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CARD_EGKCARD_P_H
#define CHIPCARD_CARD_EGKCARD_P_H


#include <chipcard3/client/card_imp.h>
#include <chipcard3/client/cards/egkcard.h>


typedef struct LC_EGKCARD LC_EGKCARD;


struct LC_EGKCARD {

  LC_CARD_OPEN_FN openFn;
  LC_CARD_CLOSE_FN closeFn;
};


static void GWENHYWFAR_CB LC_EgkCard_freeData(void *bp, void *p);


static LC_CLIENT_RESULT CHIPCARD_CB LC_EgkCard_Open(LC_CARD *card);
static LC_CLIENT_RESULT CHIPCARD_CB LC_EgkCard_Close(LC_CARD *card);


#endif /* CHIPCARD_CARD_EGKCARD_P_H */

