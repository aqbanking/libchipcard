/***************************************************************************
    begin       : Mon Mar 01 2004
    copyright   : (C) 2021 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CARD_GELDKARTE_P_H
#define CHIPCARD_CARD_GELDKARTE_P_H


#include <libchipcard/base/card_imp.h>
#include <libchipcard/cards/geldkarte/geldkarte.h>


typedef struct LC_GELDKARTE LC_GELDKARTE;


struct LC_GELDKARTE {
  GWEN_BUFFER *bin_ef_id_1;
  GWEN_DB_NODE *db_ef_id_1;
  GWEN_BUFFER *bin_ef_boerse_1;
  GWEN_DB_NODE *db_ef_boerse_1;
  LC_CARD_OPEN_FN openFn;
  LC_CARD_CLOSE_FN closeFn;
};


void GWENHYWFAR_CB LC_GeldKarte_freeData(void *bp, void *p);


int CHIPCARD_CB LC_GeldKarte_Open(LC_CARD *card);
int CHIPCARD_CB LC_GeldKarte_Close(LC_CARD *card);

int LC_GeldKarte__ReadValues(LC_CARD *card,
                             GWEN_DB_NODE *dbData);

int LC_GeldKarte__ReadBLog(LC_CARD *card,
                           int idx,
                           GWEN_DB_NODE *dbData);
int LC_GeldKarte__ReadLLog(LC_CARD *card,
                           int idx,
                           GWEN_DB_NODE *dbData);

#endif /* CHIPCARD_CARD_GELDKARTE_P_H */

