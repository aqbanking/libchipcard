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


int doImportKey(LC_CLIENT *cl,
                LC_CARD *card,
                GWEN_DB_NODE *dbArgs) {
  LC_CLIENT_RESULT res;
  int rv;
  int v;
  int kid;
  GWEN_KEYSPEC *ks;
  const GWEN_KEYSPEC *cks;
  GWEN_CRYPTKEY *key;
  const char *fname;
  GWEN_DB_NODE *dbKey;
  const char *s;

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

  /* verify EG pin */
  if (v>0)
    fprintf(stderr, "Verifying device pin\n");
  rv=verifyPin(card, dbArgs, 0x91);
  if (rv) {
    return rv;
  }
  if (v>0)
    fprintf(stderr, "Device PIN ok.\n");

  if (v>0)
    fprintf(stderr, "Reading key information\n");
  ks=LC_Starcos_GetKeySpec(card, kid);
  if (!ks) {
    showError(card, LC_Client_ResultCmdError, "GetKeySpec");
    return RETURNVALUE_WORK;
  }

  /* check key status */
  if (GWEN_KeySpec_GetStatus(ks)!=0x08) {
    fprintf(stderr,
            "ERROR: Key already exists, please remove it first by using the\n"
            "command \"deactivatekey\"\n");
    GWEN_KeySpec_free(ks);
    return RETURNVALUE_WORK;
  }

  /* read key from file */
  if (v>0)
    fprintf(stderr, "Reading key file\n");
  dbKey=GWEN_DB_Group_new("key");
  if (GWEN_DB_ReadFile(dbKey, fname,
                       GWEN_DB_FLAGS_DEFAULT|
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    fprintf(stderr, "ERROR: Could not write key file.\n");
    GWEN_DB_Group_free(dbKey);
    GWEN_KeySpec_free(ks);
    return RETURNVALUE_WORK;
  }
  GWEN_KeySpec_free(ks);

  key=GWEN_CryptKey_FromDb(dbKey);
  if (!key) {
    fprintf(stderr, "ERROR: Could not create key from file data.\n");
    GWEN_DB_Group_free(dbKey);
    return RETURNVALUE_WORK;
  }
  GWEN_DB_Group_free(dbKey);

  /* check/set key name */
  s=GWEN_CryptKey_GetKeyName(key);
  if (s) {
    if (strcasecmp(s, "V")==0) {
      if (!GWEN_DB_VariableExists(dbArgs, "cryptkey")) {
        fprintf(stderr, "ERROR: Cannot write a crypt key as a sign key\n");
        GWEN_CryptKey_free(key);
        return RETURNVALUE_WORK;
      }
    }
    else if (strcasecmp(s, "S")==0) {
      if (GWEN_DB_VariableExists(dbArgs, "cryptkey")) {
        fprintf(stderr, "ERROR: Cannot write a sign key as a crypt key\n");
        GWEN_CryptKey_free(key);
        return RETURNVALUE_WORK;
      }
    }
    else {
      fprintf(stderr,
              "ERROR: Unknown key name "
              "(use either \"S\" or \"V\" as keyname)\n");
      GWEN_CryptKey_free(key);
      return RETURNVALUE_WORK;
    }
  }
  else {
    if (GWEN_DB_VariableExists(dbArgs, "cryptkey"))
      GWEN_CryptKey_SetKeyName(key, "V");
    else
      GWEN_CryptKey_SetKeyName(key, "S");
  }

  /* write key spec */
  if (v>0)
    fprintf(stderr, "Updating key information\n");
  GWEN_CryptKey_SetStatus(key, 0x10);
  cks=GWEN_CryptKey_GetKeySpec(key);
  assert(cks);
  res=LC_Starcos_SetKeySpec(card, kid, cks);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "SetKeySpec");
    GWEN_CryptKey_free(key);
    return RETURNVALUE_WORK;
  }

  /* write public key */
  if (v>0)
    fprintf(stderr, "Writing key to card\n");
  res=LC_Starcos_WritePublicKey(card, kid, key);
  if (res!=LC_Client_ResultOk) {
    showError(card, res, "WritePublicKey");
    GWEN_CryptKey_free(key);
    return RETURNVALUE_WORK;
  }
  GWEN_CryptKey_free(key);

  return 0;
}



int importKey(LC_CLIENT *cl, GWEN_DB_NODE *dbArgs) {
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

  rv=doImportKey(cl, card, dbArgs);

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
            "Key succesfully imported.\n");
  }
  return rv;
}


