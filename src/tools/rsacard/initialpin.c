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

#include <gwenhywfar/debug.h>


int doInitialPin(LC_CLIENT *cl,
		 LC_CARD *card,
		 GWEN_DB_NODE *dbArgs) {
  LC_CLIENT_RESULT res;
  int maxErrors;
  int currentErrors;
  const char *pin;

  pin=GWEN_DB_GetCharValue(dbArgs, "cardHolderPin", 0, 0);
  if (!pin) {
    fprintf(stderr, "ERROR: New cardholder PIN missing. Use \"-p ARG\"\n");
    return RETURNVALUE_PARAM;
  }

  if (!*pin) {
    fprintf(stderr, "ERROR: Empty cardholder PIN. Use \"-p ARG\"\n");
    return RETURNVALUE_PARAM;
  }

  res=LC_Starcos_GetPinStatus(card, 0x90, &maxErrors, &currentErrors);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "GetPinStatus");
    return RETURNVALUE_WORK;
  }

  if (currentErrors!=maxErrors) {
    if (!GWEN_DB_VariableExists(dbArgs, "forcepin")) {
      fprintf(stderr,
              "\n"
              "ERROR: The error counter for the pin has not its inital\n"
              "value. Therefore I will not verify this pin unless the option\n"
              "\"-F\" is given.\n"
              "This is to protect your card against blocking.\n"
              "Please only use the \"-F\" option if you are sure that the\n"
              "pin is correct.\n");
      return RETURNVALUE_WORK;
    }
  }

  res=LC_Starcos_ModifyInitialPin(card, 0x90, pin);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "ModifyInitialPin");
    return RETURNVALUE_WORK;
  }
  else {
    fprintf(stderr, "PIN changed.\n");
  }

  return 0;
}



int initialPin(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs) {
  LC_CARD *card=0;
  LC_CLIENT_RESULT res;
  int rv;
  int v;

  v=GWEN_DB_GetIntValue(dbArgs, "verbosity", 0, 0);

  if (GWEN_DB_VariableExists(dbArgs, "keypad") &&
      !GWEN_DB_VariableExists(dbArgs, "cardHolderPin")) {
    fprintf(stderr,
            "\n"
            "ERROR: When setting the initial PIN the keypad can't be used.\n"
            "Please give the new PIN via the option \"-p ARG\"\n");
    return RETURNVALUE_PARAM;
  }

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

  rv=doInitialPin(cl, card, dbArgs);

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
	    "Initial PIN successfully changed.\n");
  }
  return rv;
}


