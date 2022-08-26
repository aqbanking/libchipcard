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
#include <libchipcard/base/client.h>
#include <libchipcard/cards/memorycard/memorycard.h>



#define RETURNVALUE_PARAM   1
#define RETURNVALUE_SETUP   2
#define RETURNVALUE_WORK    3
#define RETURNVALUE_DEINIT  4


void showError(LC_CARD *card, int res, const char *x);

int memRead(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs);
int memWrite(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs);




#endif /* RSACARD_GLOBAL_H */

