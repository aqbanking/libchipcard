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
#undef BUILDING_LIBCHIPCARD2_DLL

#include "global.h"

#include <gwenhywfar/debug.h>



int doGenerateKey(LC_CLIENT *cl,
                  LC_CARD *card,
                  GWEN_DB_NODE *dbArgs) {
  LC_CLIENT_RESULT res;
  int kid;
  int rv;
  int v;

  if (GWEN_DB_VariableExists(dbArgs, "cryptkey"))
    kid=0x8e;
  else
    kid=0x8f;

  v=GWEN_DB_GetIntValue(dbArgs, "verbosity", 0, 0);

  /* verify cardholder PIN */
  if (v>0)
    fprintf(stderr, "Verifying cardholder pin\n");
  rv=verifyPin(card, dbArgs, 0x90);
  if (rv) {
    return rv;
  }
  if (v>0)
    fprintf(stderr, "Cardholder PIN ok.\n");

  /* verify EG pin */
  if (v>0)
    fprintf(stderr, "Verifying device pin\n");
  rv=verifyPin(card, dbArgs, 0x91);
  if (rv) {
    return rv;
  }
  if (v>0)
    fprintf(stderr, "Device PIN ok.\n");

  DBG_NOTICE(0, "Generating key %02x", kid);

  if (v>0)
    fprintf(stderr, "Generating key, please wait...\n");
  res=LC_Starcos_GenerateKeyPair(card, kid, 768);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "GenerateKeyPair");
    return RETURNVALUE_WORK;
  }
  if (v>0)
    fprintf(stderr, "Key generated.\n");

  return 0;
}




int generateKey(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs) {
  LC_CARD *card=0;
  LC_CLIENT_RESULT res;
  int rv;
  int v;

  v=GWEN_DB_GetIntValue(dbArgs, "verbosity", 0, 0);

  if (v>1)
    fprintf(stderr, "Connecting to server.\n");
  res=LC_Client_StartWait(cl, 0, 0);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "StartWait");
    return RETURNVALUE_WORK;
  }
  if (v>1)
    fprintf(stderr, "Connected.\n");

  if (v>0)
    fprintf(stderr, "Waiting for card...\n");
  card=LC_Client_WaitForNextCard(cl, 20);
  if (!card) {
    fprintf(stderr, "ERROR: No card found.\n");
    return RETURNVALUE_WORK;
  }
  if (v>0)
    fprintf(stderr, "Found a card.\n");

  if (LC_Starcos_ExtendCard(card)) {
    fprintf(stderr, "Could not extend card as STARCOS card\n");
    return RETURNVALUE_WORK;
  }

  if (v>0)
    fprintf(stderr, "Opening card.\n");
  res=LC_Card_Open(card);
  if (res!=LC_Client_ResultOk) {
    fprintf(stderr,
            "ERROR: Error executing command CardOpen (%d).\n",
            res);
    return RETURNVALUE_WORK;
  }
  if (v>0)
    fprintf(stderr, "Card is a STARCOS card as expected.\n");

  if (v>1)
    fprintf(stderr, "Telling the server that we need no more cards.\n");
  res=LC_Client_StopWait(cl);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "StopWait");
    return RETURNVALUE_WORK;
  }

  rv=doGenerateKey(cl, card, dbArgs);

  if (v>0)
    fprintf(stderr, "Closing card.\n");
  res=LC_Card_Close(card);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "CardClose");
    return RETURNVALUE_WORK;
  }
  if (v>0)
    fprintf(stderr, "Card closed.\n");

  if (rv==0) {
    fprintf(stderr,
            "Key succesfully created.\n"
            "Activate it using the command \"activatekey\".\n");
  }
  return rv;
}







