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
#include <gwenhywfar/keyspec.h>


int doExportBankInfo(LC_CLIENT *cl,
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

  dbData=GWEN_DB_Group_new("bankData");
  res=LC_Starcos_ReadInstituteData(card, account, dbData);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "ReadInstituteData");
    GWEN_DB_Group_free(dbData);
    return RETURNVALUE_WORK;
  }

  if (GWEN_DB_WriteFile(dbData, fname, GWEN_DB_FLAGS_DEFAULT)) {
    fprintf(stderr, "ERROR: Could not write data file.\n");
    return RETURNVALUE_WORK;
  }
  GWEN_DB_Group_free(dbData);

  return 0;
}



int exportBankInfo(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs) {
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

  rv=doExportBankInfo(cl, card, dbArgs);

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
            "Bank info succesfully exported.\n");
  }
  return rv;
}


