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


#ifndef RSACARD_GLOBAL_H
#define RSACARD_GLOBAL_H


#include <chipcard2/chipcard2.h>
#include <chipcard2-client/client/client.h>
#include <chipcard2-client/cards/starcos.h>

#include <gwenhywfar/logger.h>


#define RETURNVALUE_PARAM   1
#define RETURNVALUE_SETUP   2
#define RETURNVALUE_WORK    3
#define RETURNVALUE_DEINIT  4


void showError(LC_CARD *card, LC_CLIENT_RESULT res, const char *x);
int verifyPin(LC_CARD *card, GWEN_DB_NODE *dbArgs, int pid);

int generateKey(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs);
int activateKey(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs);
int deactivateKey(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs);
int exportKey(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs);
int importKey(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs);
int exportBankInfo(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs);
int importBankInfo(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs);

int initialPin(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs);




#endif /* RSACARD_GLOBAL_H */

