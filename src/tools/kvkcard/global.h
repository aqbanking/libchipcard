/***************************************************************************
    begin       : Mon Mar 01 2004
    copyright   : (C) 2021 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef RSACARD_GLOBAL_H
#define RSACARD_GLOBAL_H


#include <libchipcard/chipcard.h>
#include <libchipcard/cards/kvkcard/kvkcard.h>
#include <libchipcard/cards/egkcard/egkcard.h>
#include <libchipcard/cards/hipersonaldata.h>
#include <libchipcard/cards/hiinsurancedata.h>



#define RETURNVALUE_PARAM          1
#define RETURNVALUE_SETUP          2
#define RETURNVALUE_WORK           3
#define RETURNVALUE_DEINIT         4
#define RETURNVALUE_CARD_NOT_SUPP  5


void usage(const char *name, const char *ustr);
void showError(LC_CARD *card, int res, const char *x);

void okBeep();
void errorBeep();


int kvkRead(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs);

int kvkRead2(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs);

int rdvd(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs);
int rdpd(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs);


int psvd(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs);


#endif /* RSACARD_GLOBAL_H */

