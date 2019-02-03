/***************************************************************************
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004-2010 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef RSACARD_GLOBAL_H
#define RSACARD_GLOBAL_H


#include <gwenhywfar/logger.h>
#include <gwenhywfar/db.h>
#include <gwenhywfar/ct.h>
#include <gwenhywfar/debug.h>
#include <chipcard/client.h>
#include <chipcard/ct/ct_card.h>


#define I18N(msg) msg

#define LISTREADERS_TIMEOUT 5
#define CARD_TIMEOUT 30

#define RETURNVALUE_PARAM   1
#define RETURNVALUE_SETUP   2
#define RETURNVALUE_WORK    3
#define RETURNVALUE_DEINIT  4

#define ZKACARDTOOL_PROGRAM_VERSION "0.9"

int getPublicKey(GWEN_DB_NODE *dbArgs, int argc, char **argv);
int showNotepad(GWEN_DB_NODE *dbArgs, int argc, char **argv);
void showError(LC_CARD *card, LC_CLIENT_RESULT res, const char *x);


#endif /* RSACARD_GLOBAL_H */

