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


int doExportKey(LC_CLIENT *cl,
                LC_CARD *card,
                GWEN_DB_NODE *dbArgs) {
  int rv;
  int v;
  int kid;
  GWEN_KEYSPEC *ks;
  GWEN_CRYPTKEY *key;
  const char *fname;
  GWEN_DB_NODE *dbKey;
  GWEN_ERRORCODE err;
  int kst;

  fname=GWEN_DB_GetCharValue(dbArgs, "file", 0, 0);
  if (!fname) {
    fprintf(stderr,
            "No key file name given.\n"
            "Please use option \"-f ARG\" to specify the file name.\n");
    return RETURNVALUE_PARAM;
  }
  kid=0x80;
  if (GWEN_DB_VariableExists(dbArgs, "bankkey"))
    kid+=0x10;
  if (GWEN_DB_VariableExists(dbArgs, "cryptkey"))
    kid+=5;
  kid+=GWEN_DB_GetIntValue(dbArgs, "account", 0, 1);

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

  ks=LC_Starcos_GetKeySpec(card, kid);
  if (!ks) {
    showError(card, LC_Client_ResultCmdError, "GetKeySpec");
    return RETURNVALUE_WORK;
  }

  /* check key status */
  kst=GWEN_KeySpec_GetStatus(ks);
  if (kst==0x08 || kst==0x0a || kst==0x07) {
    fprintf(stderr, "ERROR: Key is not active.\n");
    GWEN_KeySpec_free(ks);
    return RETURNVALUE_PARAM;
  }

  /* read public key */
  key=LC_Starcos_ReadPublicKey(card, kid);
  if (!key) {
    showError(card, LC_Client_ResultCmdError, "ReadPublicKey");
    GWEN_KeySpec_free(ks);
    return RETURNVALUE_WORK;
  }

  /* complete key */
  GWEN_CryptKey_SetStatus(key, GWEN_KeySpec_GetStatus(ks));
  GWEN_CryptKey_SetNumber(key, GWEN_KeySpec_GetNumber(ks));
  GWEN_CryptKey_SetVersion(key, GWEN_KeySpec_GetVersion(ks));

  dbKey=GWEN_DB_Group_new("key");
  err=GWEN_CryptKey_ToDb(key, dbKey, 1);
  if (!GWEN_Error_IsOk(err)) {
    DBG_ERROR_ERR(0, err);
    GWEN_DB_Group_free(dbKey);
    GWEN_CryptKey_free(key);
    return RETURNVALUE_WORK;
  }

  if (GWEN_DB_WriteFile(dbKey, fname, GWEN_DB_FLAGS_DEFAULT)) {
    fprintf(stderr, "ERROR: Could not write key file.\n");
    GWEN_DB_Group_free(dbKey);
    GWEN_CryptKey_free(key);
    return RETURNVALUE_WORK;
  }
  GWEN_DB_Group_free(dbKey);
  GWEN_CryptKey_free(key);

  return 0;
}



int exportKey(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs) {
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

  rv=doExportKey(cl, card, dbArgs);

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
            "Key succesfully exported.\n");
  }
  return rv;
}


