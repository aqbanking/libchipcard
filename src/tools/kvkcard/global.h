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


#include <chipcard/chipcard.h>
#include <chipcard/client/client.h>
#include <chipcard/client/cards/kvkcard.h>

#include <gwenhywfar/logger.h>
#include <gwenhywfar/process.h>


#define RETURNVALUE_PARAM   1
#define RETURNVALUE_SETUP   2
#define RETURNVALUE_WORK    3
#define RETURNVALUE_DEINIT  4


void usage(const char *name, const char *ustr);
void showError(LC_CARD *card, LC_CLIENT_RESULT res, const char *x);

void okBeep();
void errorBeep();


int kvkRead(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs);
int kvkDaemon(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs);




#endif /* RSACARD_GLOBAL_H */

