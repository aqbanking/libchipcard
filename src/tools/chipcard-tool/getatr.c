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


#include "global.h"
#include <time.h>
#include <assert.h>
#include <chipcard2-client/mon/monitor.h>
#include <gwenhywfar/debug.h>



int getAtr(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs){
  LC_CLIENT_RESULT res;
  int timeOut;
  LC_CARD *card;

  timeOut=GWEN_DB_GetIntValue(dbArgs, "timeout", 0, CARD_TIMEOUT);

  res=LC_Client_StartWait(cl, 0, 0);
  if (res!=LC_Client_ResultOk) {
    showError(0, res, "StartWait");
    return 2;
  }

  card=LC_Client_WaitForNextCard(cl, timeOut);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return 2;
  }
  fprintf(stderr, "INFO: We got this card:\n");
  LC_Card_Dump(card, stderr, 2);

  res=LC_Client_StopWait(cl);
  if (res!=LC_Client_ResultOk) {
    showError(0, res, "StopWait");
    return 2;
  }

  return 0;
}


