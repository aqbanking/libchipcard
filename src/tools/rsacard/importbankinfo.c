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
#include <gwenhywfar/keyspec.h>


int doImportBankInfo(LC_CLIENT *cl,
                     LC_CARD *card,
                     GWEN_DB_NODE *dbArgs) {
  LC_CLIENT_RESULT res;
  int rv;
  int v;
  int account;
  const char *fname;
  GWEN_DB_NODE *dbData;

  fname=GWEN_DB_GetCharValue(dbArgs, "file", 0, 0);
  if (!fname) {
    fprintf(stderr,
            "No output file name given.\n"
            "Please use option \"-f ARG\" to specify the file name.\n");
    return RETURNVALUE_PARAM;
  }

  account=GWEN_DB_GetIntValue(dbArgs, "account", 0, 1);
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


  /* read data file */
  if (v>0)
    fprintf(stderr, "Reading data file\n");
  dbData=GWEN_DB_Group_new("bankData");
  if (GWEN_DB_ReadFile(dbData, fname,
                       GWEN_DB_FLAGS_DEFAULT|
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not write key file.\n");
    GWEN_DB_Group_free(dbData);
    return RETURNVALUE_WORK;
  }

  /* check data */
  if (!GWEN_DB_VariableExists(dbData, "bankCode") ||
      !GWEN_DB_VariableExists(dbData, "userId")) {
    fprintf(stderr, "ERROR: Bank code and user id are needed.\n");
    GWEN_DB_Group_free(dbData);
    return RETURNVALUE_WORK;
  }

  /* preset */
  if (!GWEN_DB_VariableExists(dbData, "country"))
    GWEN_DB_SetIntValue(dbData, GWEN_DB_FLAGS_DEFAULT, "country", 280);
  if (!GWEN_DB_VariableExists(dbData, "comService"))
    GWEN_DB_SetIntValue(dbData, GWEN_DB_FLAGS_DEFAULT, "comService", 2);
  if (!GWEN_DB_VariableExists(dbData, "comAddress"))
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_DEFAULT, "comAddress", "");
  if (!GWEN_DB_VariableExists(dbData, "systemId"))
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_DEFAULT, "systemId", "");

  /* write bank info */
  if (v>0)
    fprintf(stderr, "Writing bank info to the card\n");
  res=LC_Starcos_WriteInstituteData(card, account, dbData);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "WriteInstituteData");
    GWEN_DB_Group_free(dbData);
    return RETURNVALUE_WORK;
  }

  GWEN_DB_Group_free(dbData);

  return 0;
}



int importBankInfo(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs) {
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

  rv=doImportBankInfo(cl, card, dbArgs);

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
            "Bank info succesfully imported.\n");
  }
  return rv;
}


