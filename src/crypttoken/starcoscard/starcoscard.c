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

#include "starcoscard_p.h"

#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <chipcard2-client/cards/starcos.h>
#include <chipcard2-client/cards/processorcard.h>


GWEN_INHERIT(GWEN_CRYPTTOKEN, LC_CT_STARCOS)
GWEN_INHERIT(GWEN_PLUGIN, LC_CT_PLUGIN_STARCOS)



GWEN_PLUGIN *crypttoken_starcoscard_factory(GWEN_PLUGIN_MANAGER *pm,
                                            const char *modName,
                                            const char *fileName) {
  GWEN_PLUGIN *pl;

  pl=LC_CryptTokenSTARCOS_Plugin_new(pm, modName, fileName);
  assert(pl);

  return pl;
}



GWEN_PLUGIN *LC_CryptTokenSTARCOS_Plugin_new(GWEN_PLUGIN_MANAGER *pm,
					 const char *modName,
					 const char *fileName) {
  GWEN_PLUGIN *pl;
  LC_CT_PLUGIN_STARCOS *cpl;

  pl=GWEN_CryptToken_Plugin_new(pm,
				GWEN_CryptToken_Device_Card,
				modName,
				fileName);

  GWEN_NEW_OBJECT(LC_CT_PLUGIN_STARCOS, cpl);
  GWEN_INHERIT_SETDATA(GWEN_PLUGIN, LC_CT_PLUGIN_STARCOS, pl, cpl,
		       LC_CryptTokenSTARCOS_Plugin_FreeData);
  cpl->client=LC_Client_new("LC_CryptTokenSTARCOS", VERSION, 0);
  if (LC_Client_ReadConfigFile(cpl->client, 0)) {
    DBG_ERROR(LC_LOGDOMAIN,
	      "Error reading libchipcard2 client configuration.");
    GWEN_Plugin_free(pl);
    return 0;
  }

  /* set virtual functions */
  GWEN_CryptToken_Plugin_SetCreateTokenFn(pl,
					  LC_CryptTokenSTARCOS_Plugin_CreateToken);
  GWEN_CryptToken_Plugin_SetCheckTokenFn(pl,
                                         LC_CryptTokenSTARCOS_Plugin_CheckToken);

  return pl;
}



void LC_CryptTokenSTARCOS_Plugin_FreeData(void *bp, void *p) {
  LC_CT_PLUGIN_STARCOS *cpl;

  cpl=(LC_CT_PLUGIN_STARCOS*)p;
  LC_Client_free(cpl->client);
  GWEN_FREE_OBJECT(cpl);
}



GWEN_CRYPTTOKEN*
LC_CryptTokenSTARCOS_Plugin_CreateToken(GWEN_PLUGIN *pl,
                                        const char *subTypeName,
                                        const char *name) {
  GWEN_PLUGIN_MANAGER *pm;
  GWEN_CRYPTTOKEN *ct;
  LC_CT_PLUGIN_STARCOS *cpl;

  assert(pl);
  cpl=GWEN_INHERIT_GETDATA(GWEN_PLUGIN, LC_CT_PLUGIN_STARCOS, pl);
  assert(cpl);

  pm=GWEN_Plugin_GetManager(pl);
  assert(pm);

  ct=LC_CryptTokenSTARCOS_new(pm, cpl->client, name);
  assert(ct);

  return ct;
}



int LC_CryptTokenSTARCOS_Plugin_CheckToken(GWEN_PLUGIN *pl,
                                           GWEN_BUFFER *subTypeName,
                                           GWEN_BUFFER *name) {
  GWEN_PLUGIN_MANAGER *pm;
  LC_CT_PLUGIN_STARCOS *cpl;
  LC_CLIENT_RESULT res;
  LC_CARD *hcard=0;
  const char *currCardNumber;

  assert(pl);
  cpl=GWEN_INHERIT_GETDATA(GWEN_PLUGIN, LC_CT_PLUGIN_STARCOS, pl);
  assert(cpl);

  pm=GWEN_Plugin_GetManager(pl);
  assert(pm);

  res=LC_Client_StartWait(cpl->client, 0, 0);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send StartWait request");
    return GWEN_ERROR_CT_IO_ERROR;
  }

  hcard=LC_Client_WaitForNextCard(cpl->client, 5);
  if (!hcard) {
    DBG_ERROR(LC_LOGDOMAIN,
	      "No card within specified timeout");
    LC_Client_StopWait(cpl->client);
    return GWEN_ERROR_CT_IO_ERROR;
  }
  else {
    int rv;

    /* ok, we have a card, don't wait for more */
    LC_Client_StopWait(cpl->client);
    /* check card */
    rv=LC_Starcos_ExtendCard(hcard);
    if (rv) {
      DBG_ERROR(LC_LOGDOMAIN,
		"STARCOS card not available, please check your setup (%d)", rv);
      LC_Card_free(hcard);
      return GWEN_ERROR_NOT_AVAILABLE;
    }

    res=LC_Card_Open(hcard);
    if (res!=LC_Client_ResultOk) {
      LC_Card_free(hcard);
      DBG_NOTICE(LC_LOGDOMAIN,
		 "Could not open card (%d), maybe not a STARCOS card?",
		 res);
      return GWEN_ERROR_CT_NOT_SUPPORTED;
    } /* if card not open */
    else {
      GWEN_DB_NODE *dbCardData;

        dbCardData=LC_Starcos_GetCardDataAsDb(hcard);
	assert(dbCardData);

        currCardNumber=GWEN_DB_GetCharValue(dbCardData,
                                            "ICCSN/cardNumber",
                                            0,
                                            0);
	if (!currCardNumber) {
          DBG_ERROR(LC_LOGDOMAIN, "INTERNAL: No card number in card data.");
          abort();
        }

        DBG_NOTICE(LC_LOGDOMAIN, "Card number: %s", currCardNumber);

	if (GWEN_Buffer_GetUsedBytes(name)==0) {
	  DBG_NOTICE(LC_LOGDOMAIN, "No or empty token name");
	  GWEN_Buffer_AppendString(name, currCardNumber);
	}
	else {
	  if (strcasecmp(GWEN_Buffer_GetStart(name), currCardNumber)!=0) {
	    DBG_ERROR(LC_LOGDOMAIN, "Card supported, but bad name");
	    LC_Card_Close(hcard);
	    LC_Card_free(hcard);
	    return GWEN_ERROR_CT_BAD_NAME;
	  }
	}

        LC_Card_Close(hcard);
	LC_Card_free(hcard);
    } /* if card is open */
    return 0;
  } /* if there is a card */

}









GWEN_CRYPTTOKEN *LC_CryptTokenSTARCOS_new(GWEN_PLUGIN_MANAGER *pm,
                                          LC_CLIENT *lc,
                                          const char *name) {
  LC_CT_STARCOS *lct;
  GWEN_CRYPTTOKEN *ct;

  DBG_ERROR(0, "Creating crypttoken (STARCOS)");

  /* create crypt token */
  ct=GWEN_CryptToken_new(pm,
                         GWEN_CryptToken_Device_Card,
                         "starcoscard", 0, name);

  /* inherit CryptToken: Set our own data */
  GWEN_NEW_OBJECT(LC_CT_STARCOS, lct);
  GWEN_INHERIT_SETDATA(GWEN_CRYPTTOKEN, LC_CT_STARCOS, ct, lct,
                       LC_CryptTokenSTARCOS_FreeData);
  lct->pluginManager=pm;
  lct->client=lc;

  /* set virtual functions */
  GWEN_CryptToken_SetOpenFn(ct, LC_CryptTokenSTARCOS_Open);
  GWEN_CryptToken_SetCreateFn(ct, LC_CryptTokenSTARCOS_Create);
  GWEN_CryptToken_SetCloseFn(ct, LC_CryptTokenSTARCOS_Close);
  GWEN_CryptToken_SetSignFn(ct, LC_CryptTokenSTARCOS_Sign);
  GWEN_CryptToken_SetVerifyFn(ct, LC_CryptTokenSTARCOS_Verify);
  GWEN_CryptToken_SetEncryptFn(ct, LC_CryptTokenSTARCOS_Encrypt);
  GWEN_CryptToken_SetDecryptFn(ct, LC_CryptTokenSTARCOS_Decrypt);
  GWEN_CryptToken_SetGetSignSeqFn(ct, LC_CryptTokenSTARCOS_GetSignSeq);
  GWEN_CryptToken_SetReadKeySpecFn(ct, LC_CryptTokenSTARCOS_ReadKeySpec);
  GWEN_CryptToken_SetWriteKeySpecFn(ct, LC_CryptTokenSTARCOS_WriteKeySpec);
  GWEN_CryptToken_SetReadKeyFn(ct, LC_CryptTokenSTARCOS_ReadKey);
  GWEN_CryptToken_SetWriteKeyFn(ct, LC_CryptTokenSTARCOS_WriteKey);
  GWEN_CryptToken_SetGenerateKeyFn(ct, LC_CryptTokenSTARCOS_GenerateKey);
  GWEN_CryptToken_SetFillUserListFn(ct, LC_CryptTokenSTARCOS_FillUserList);
  return ct;
}



void LC_CryptTokenSTARCOS_FreeData(void *bp, void *p) {
  LC_CT_STARCOS *lct;

  lct=(LC_CT_STARCOS*)p;
  if (lct->card)
    LC_Card_free(lct->card);
  GWEN_FREE_OBJECT(lct);
}



int LC_CryptTokenSTARCOS__EnterPin(GWEN_CRYPTTOKEN *ct,
                                   LC_CARD *hcard,
                                   GWEN_CRYPTTOKEN_PINTYPE pt,
                                   int pid) {
  LC_CT_STARCOS *lct;
  LC_CLIENT_RESULT res;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_STARCOS, ct);
  assert(lct);

  assert(hcard);

  if ((pt!=GWEN_CryptToken_PinType_Manage) &&
      (LC_Card_GetReaderFlags(hcard) & LC_CARD_READERFLAGS_KEYPAD)) {
    int mres;

    DBG_INFO(LC_LOGDOMAIN,"Terminal has a keypad, will ask for pin.");
    /* tell the user about pin verification */
    mres=GWEN_CryptManager_BeginEnterPin(lct->pluginManager,
                                         ct,
                                         pt);
    if (mres) {
      DBG_ERROR(LC_LOGDOMAIN, "Error in user interaction");
      return mres;
    }

    res=LC_Starcos_SecureVerifyPin(hcard, pid);
    if (res!=LC_Client_ResultOk) {
      /* tell the user about end of pin verification */
      GWEN_CryptManager_EndEnterPin(lct->pluginManager,
                                    ct,
                                    pt, 0);
      DBG_ERROR(LC_LOGDOMAIN, "sw1=%02x sw2=%02x (%s)",
                LC_Card_GetLastSW1(hcard),
                LC_Card_GetLastSW2(hcard),
                LC_Card_GetLastText(hcard));

      if (LC_Card_GetLastSW1(hcard)==0x63) {
	switch (LC_Card_GetLastSW2(hcard)) {
        case 0xc0: /* no error left */
          return GWEN_ERROR_CT_BAD_PIN_0_LEFT;
        case 0xc1: /* one left */
          return GWEN_ERROR_CT_BAD_PIN_1_LEFT;
        case 0xc2: /* two left */
          return GWEN_ERROR_CT_BAD_PIN_2_LEFT;
        default:   /* unknown error */
          return GWEN_ERROR_CT_BAD_PIN;
        } // switch
      }
      else if (LC_Card_GetLastSW1(hcard)==0x69 &&
               LC_Card_GetLastSW2(hcard)==0x83) {
        DBG_ERROR(LC_LOGDOMAIN, "Card unusable");
        return GWEN_ERROR_CT_IO_ERROR;
      }
      else if (LC_Card_GetLastSW1(hcard)==0x64 &&
               LC_Card_GetLastSW2(hcard)==0x01) {
        DBG_ERROR(LC_LOGDOMAIN, "Aborted by user");
        return GWEN_ERROR_USER_ABORTED;
      }
      else {
        return GWEN_ERROR_CT_IO_ERROR;
      }
    } /* if not ok */
    else {
      /* PIN ok */
      DBG_INFO(LC_LOGDOMAIN, "Pin ok");
      GWEN_CryptManager_EndEnterPin(lct->pluginManager,
                                    ct,
                                    pt, 1);
    }
  } /* if hasKeyPad */
  else {
    char pinBuffer[64];
    int mres;
    int pinLength;
    GWEN_TYPE_UINT32 pflags=0;

    DBG_INFO(LC_LOGDOMAIN, "No keypad (or disabled), will ask for PIN");
    memset(pinBuffer, 0, sizeof(pinBuffer));

    if (pt==GWEN_CryptToken_PinType_Manage)
      pflags|=GWEN_CRYPTTOKEN_GETPIN_FLAGS_ALLOW_DEFAULT;
    pflags|=GWEN_CRYPTTOKEN_GETPIN_FLAGS_NUMERIC;
    mres=GWEN_CryptManager_GetPin(lct->pluginManager,
                                  ct,
				  pt,
				  GWEN_CryptToken_PinEncoding_ASCII,
				  pflags,
                                  pinBuffer,
				  4, 10, &pinLength);
    if (mres==GWEN_ERROR_CT_DEFAULT_PIN) {
      DBG_INFO(LC_LOGDOMAIN, "Verifying the default PIN");
      res=LC_Starcos_VerifyInitialPin(hcard, pid);
    }
    else if (mres==0) {
      DBG_INFO(LC_LOGDOMAIN, "Verifying the PIN");
      res=LC_Starcos_VerifyPin(hcard, pid, pinBuffer);
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "Error asking for PIN, aborting");
      memset(pinBuffer, 0, sizeof(pinBuffer));
      return mres;
    }

    if (res!=LC_Client_ResultOk) {
      DBG_ERROR(LC_LOGDOMAIN, "sw1=%02x sw2=%02x (%s)",
                LC_Card_GetLastSW1(hcard),
                LC_Card_GetLastSW2(hcard),
                LC_Card_GetLastText(hcard));

      if (LC_Card_GetLastSW1(hcard)==0x63) {
        /* TODO: Set Pin status */
        switch (LC_Card_GetLastSW2(hcard)) {
        case 0xc0: /* no error left */
          return GWEN_ERROR_CT_BAD_PIN_0_LEFT;
        case 0xc1: /* one left */
          return GWEN_ERROR_CT_BAD_PIN_1_LEFT;
        case 0xc2: /* two left */
          return GWEN_ERROR_CT_BAD_PIN_2_LEFT;
        default:
          return GWEN_ERROR_CT_BAD_PIN;
        } // switch
      }
      else if (LC_Card_GetLastSW1(hcard)==0x69 &&
               LC_Card_GetLastSW2(hcard)==0x83) {
        /* TODO: Set Pin status */
        DBG_ERROR(LC_LOGDOMAIN, "Card unusable");
        return GWEN_ERROR_CT_IO_ERROR;
      }
      else if (LC_Card_GetLastSW1(hcard)==0x64 &&
               LC_Card_GetLastSW2(hcard)==0x01) {
        return GWEN_ERROR_USER_ABORTED;
      }
      else {
        DBG_ERROR(LC_LOGDOMAIN, "Unknown error");
        return GWEN_ERROR_CT_IO_ERROR;
      }
    } // if not ok
    else {
      DBG_INFO(LC_LOGDOMAIN, "PIN ok");
      /* TODO: Set Pin Status */
    }
  } // if no keyPad

  return 0;
}



int LC_CryptTokenSTARCOS_VerifyPin(GWEN_CRYPTTOKEN *ct,
                                   GWEN_CRYPTTOKEN_PINTYPE pt) {
  LC_CT_STARCOS *lct;
  int pid;
  LC_CLIENT_RESULT res;
  int maxErrors;
  int currentErrors;
  int force;
  int rv;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_STARCOS, ct);
  assert(lct);

  force=GWEN_CryptToken_GetFlags(ct) & GWEN_CRYPTTOKEN_FLAGS_FORCE_PIN_ENTRY;
  if (pt==GWEN_CryptToken_PinType_Access) {
    if (lct->haveChPin)
      return 0;
    pid=LC_Starcos_GetChPinId(lct->card);
  }
  else if (pt==GWEN_CryptToken_PinType_Manage) {
    if (lct->haveEgPin)
      return 0;
    pid=LC_Starcos_GetEgPinId(lct->card);
  }
  else {
    DBG_ERROR(LC_LOGDOMAIN, "Unknown pin type \"%s\"",
              GWEN_CryptToken_PinType_toString(pt));
    return GWEN_ERROR_INVALID;
  }

  /* enter pin */
  res=LC_Starcos_GetPinStatus(lct->card,
                              pid,
                              &maxErrors,
                              &currentErrors);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Unable to read status of pin %x (%d)", pid, res);
    return LC_CryptTokenSTARCOS__ResultToError(res);
  }

  if ((currentErrors!=maxErrors) && !force) {
    DBG_ERROR(LC_LOGDOMAIN,
              "Bad pin entered at least once before, aborting");
    return GWEN_ERROR_ABORTED;
  }

  rv=LC_CryptTokenSTARCOS__EnterPin(ct, lct->card, pt, pid);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "Error in pin input");
    return rv;
  }

  if (pt==GWEN_CryptToken_PinType_Access)
    lct->haveChPin=1;
  else if (pt==GWEN_CryptToken_PinType_Manage)
    lct->haveEgPin=1;
  return 0;
}



int LC_CryptTokenSTARCOS__GetCard(GWEN_CRYPTTOKEN *ct, int manage) {
  LC_CT_STARCOS *lct;
  LC_CLIENT_RESULT res;
  LC_CARD *hcard=0;
  int first;
  const char *currCardNumber;
  const char *name;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_STARCOS, ct);
  assert(lct);

  name=GWEN_CryptToken_GetTokenName(ct);

  res=LC_Client_StartWait(lct->client, 0, 0);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send StartWait request");
    return GWEN_ERROR_CT_IO_ERROR;
  }

  first=1;
  for (;;) {
    int timeout;

    /* determine timeout value */
    if (first)
      timeout=3;
    else
      timeout=5;

    hcard=LC_Client_WaitForNextCard(lct->client, timeout);
    if (!hcard) {
      int mres;

      mres=GWEN_CryptManager_InsertToken(lct->pluginManager, ct);
      if (mres) {
        DBG_ERROR(LC_LOGDOMAIN, "Error in user interaction (%d)", mres);
        LC_Client_StopWait(lct->client);
        return GWEN_ERROR_USER_ABORTED;
      }
    }
    else {
      int rv;

      /* ok, we have a card, now check it */
      rv=LC_Starcos_ExtendCard(hcard);
      if (rv) {
        DBG_ERROR(LC_LOGDOMAIN,
                  "STARCOS card not available, please check your setup (%d)", rv);
        LC_Card_free(hcard);
        LC_Client_StopWait(lct->client);
        return GWEN_ERROR_NOT_AVAILABLE;
      }

      res=LC_Card_Open(hcard);
      if (res!=LC_Client_ResultOk) {
        LC_Card_free(hcard);
        DBG_NOTICE(LC_LOGDOMAIN,
                   "Could not open card (%d), maybe not a STARCOS card?",
                   res);
      } /* if card not open */
      else {
        GWEN_DB_NODE *dbCardData;

        dbCardData=LC_Starcos_GetCardDataAsDb(hcard);
	assert(dbCardData);

        currCardNumber=GWEN_DB_GetCharValue(dbCardData,
                                            "ICCSN/cardNumber",
                                            0,
                                            0);
	if (!currCardNumber) {
          DBG_ERROR(LC_LOGDOMAIN, "INTERNAL: No card number in card data.");
          GWEN_DB_Dump(dbCardData, stderr, 2);
          abort();
        }

        DBG_NOTICE(LC_LOGDOMAIN, "Card number: %s", currCardNumber);

        if (!name || !*name) {
          DBG_NOTICE(LC_LOGDOMAIN, "No or empty token name");
          GWEN_CryptToken_SetTokenName(ct, currCardNumber);
          name=GWEN_CryptToken_GetTokenName(ct);
          break;
        }

        if (strcasecmp(name, currCardNumber)==0) {
          DBG_NOTICE(LC_LOGDOMAIN, "Card number equals");
          break;
        }

        LC_Card_Close(hcard);
	LC_Card_free(hcard);

        hcard=LC_Client_PeekNextCard(lct->client);
        if (!hcard) {
          int mres;

          mres=GWEN_CryptManager_InsertCorrectToken(lct->pluginManager, ct);
          if (mres) {
            DBG_ERROR(LC_LOGDOMAIN, "Error in user interaction (%d)", mres);
            LC_Client_StopWait(lct->client);
            return GWEN_ERROR_USER_ABORTED;
          }
        } /* if there is no other card waiting */
        else {
          /* otherwise there already is another card in another reader,
           * so no need to bother the user. This allows to insert all
           * cards in all readers and let me choose the card ;-) */
        } /* if there is another card waiting */
      } /* if card open */
    } /* if there is a card */

    first=0;
  } /* for */

  /* ok, now we have the card we wanted to have, now ask for the pin */
  LC_Client_StopWait(lct->client);

  lct->card=hcard;
  return 0;
}



int LC_CryptTokenSTARCOS_Open(GWEN_CRYPTTOKEN *ct, int manage) {
  LC_CT_STARCOS *lct;
  int rv;
  GWEN_XMLNODE *node;
  GWEN_XMLNODE *nct;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_STARCOS, ct);
  assert(lct);

  lct->haveChPin=0;
  lct->haveEgPin=0;

  /* get card */
  rv=LC_CryptTokenSTARCOS__GetCard(ct, manage);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  /* get CryptToken info */
  node=LC_Card_GetAppInfo(lct->card);
  assert(node);
  nct=GWEN_XMLNode_FindFirstTag(node, "crypttoken", 0, 0);
  if (!nct) {
    DBG_ERROR(LC_LOGDOMAIN,
              "Card application data does not contain a crypttoken");
    GWEN_XMLNode_free(node);
    return GWEN_ERROR_CT_IO_ERROR;
  }

  /* read cryptToken data into CryptToken */
  rv=GWEN_CryptToken_ReadXml(ct, nct);
  if (rv) {
    DBG_ERROR(LC_LOGDOMAIN,
              "Error reading CryptToken data from XML (%d)",
              rv);
    GWEN_XMLNode_free(node);
    return rv;
  }
  GWEN_XMLNode_free(node);

  return 0;
}



int LC_CryptTokenSTARCOS_Create(GWEN_CRYPTTOKEN *ct) {
  return 0;
}



int LC_CryptTokenSTARCOS_Close(GWEN_CRYPTTOKEN *ct) {
  LC_CT_STARCOS *lct;
  LC_CLIENT_RESULT res;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_STARCOS, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  lct->haveChPin=0;
  lct->haveEgPin=0;

  res=LC_Card_Close(lct->card);
  if (res!=LC_Client_ResultOk) {
    LC_Card_free(lct->card);
    lct->card=0;
    return GWEN_ERROR_CT_IO_ERROR;
  }

  LC_Card_free(lct->card);
  lct->card=0;
  return 0;
}



int LC_CryptTokenSTARCOS__TransformAlgo(int tmpl,
                                        GWEN_CRYPTTOKEN_HASHALGO hashAlgo,
                                        GWEN_CRYPTTOKEN_PADDALGO paddAlgo) {
  int pa=-1;

  /*
    0xb6   ! 0x11 ! SHA-1      ! ISO 9796/2 with random 
           ! 0x12 ! SHA-1      ! PKCS#1 Block Type 01
           ! 0x21 ! RIPEMD-160 ! ISO 9796/2 with random 
           ! 0x22 ! RIPEMD-160 ! PKCS#1 Block Type 01
           ! 0x25 ! RIPEMD-160 ! ISO9796/1 incl. app A4 (->HBCI)
           ! 0x26 ! RIPEMD-160 ! '00 ... 00 | Hash'
           ! 0x32 ! MD-5       ! PKCS#1 Block Type 01
           !  -   !     -      ! ISO9796/1 without app A4
    0xb8   ! 0x02 !     -      ! PKCS#1 Block Type 02
           ! 0x03 !     -      ! '00 ... 00 | Plaintext'
           !  -   !     -      ! '00 ... 00 | Plaintext'
  */

  if (tmpl==0xb6) {
    switch(paddAlgo) {
    case GWEN_CryptToken_PaddAlgo_ISO9796_1:
      pa=0;
      break;
    case GWEN_CryptToken_PaddAlgo_ISO9796_1A4:
      if (hashAlgo==GWEN_CryptToken_HashAlgo_RMD160)
        pa=0x25;
      break;
    case GWEN_CryptToken_PaddAlgo_ISO9796_2:
      if (hashAlgo==GWEN_CryptToken_HashAlgo_RMD160)
        pa=0x21;
      else if (hashAlgo==GWEN_CryptToken_HashAlgo_SHA1)
        pa=0x11;
      break;
    case GWEN_CryptToken_PaddAlgo_PKCS1_1:
      if (hashAlgo==GWEN_CryptToken_HashAlgo_RMD160)
        pa=0x22;
      else if (hashAlgo==GWEN_CryptToken_HashAlgo_SHA1)
        pa=0x12;
      else if (hashAlgo==GWEN_CryptToken_HashAlgo_MD5)
        pa=0x32;
      break;
    case GWEN_CryptToken_PaddAlgo_LeftZero:
      if (hashAlgo==GWEN_CryptToken_HashAlgo_RMD160)
        pa=0x26;
      break;
    default:
      break;
    }
  }
  else if (tmpl==0xb8) {
    switch(paddAlgo) {

    case GWEN_CryptToken_PaddAlgo_PKCS1_2:
      pa=0x02;
      break;

    case GWEN_CryptToken_PaddAlgo_LeftZero:
      pa=0x03;
      break;

    default:
      break;
    }
  }

  return pa;
}



int LC_CryptTokenSTARCOS_Sign(GWEN_CRYPTTOKEN *ct,
                              const GWEN_CRYPTTOKEN_CONTEXT *ctx,
                              const char *ptr,
                              unsigned int len,
                              GWEN_BUFFER *dst) {
  LC_CT_STARCOS *lct;
  const GWEN_CRYPTTOKEN_KEYINFO *ki;
  const GWEN_CRYPTTOKEN_SIGNINFO *si;
  int rv;
  GWEN_BUFFER *hbuf;
  LC_CLIENT_RESULT res;
  int pa;
  int kid;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_STARCOS, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  /* get sign info */
  si=GWEN_CryptToken_Context_GetSignInfo(ctx);
  assert(si);

  /* get keyinfo and perform some checks */
  ki=GWEN_CryptToken_Context_GetSignKeyInfo(ctx);
  assert(ki);

  kid=GWEN_CryptToken_KeyInfo_GetKeyId(ki);
  if (kid<0x81 || kid>0x85) {
    DBG_ERROR(LC_LOGDOMAIN,
              "Signing only allowed with kid 0x81-0x85 (is: %02x)",
              kid);
    return GWEN_ERROR_INVALID;
  }

  if (!(GWEN_CryptToken_KeyInfo_GetKeyFlags(ki) &
	GWEN_CRYPTTOKEN_KEYINFO_FLAGS_CAN_SIGN)) {
    DBG_ERROR(LC_LOGDOMAIN, "Key can not be used for signing");
    return GWEN_ERROR_INVALID;
  }
  if (GWEN_CryptToken_KeyInfo_GetCryptAlgo(ki)!=
      GWEN_CryptToken_CryptAlgo_RSA) {
    DBG_ERROR(LC_LOGDOMAIN, "Invalid crypt algo");
    return GWEN_ERROR_INVALID;
  }

  /* transform algo */
  pa=GWEN_CryptToken_SignInfo_GetId(si);
  assert(pa);

  /* hash data */
  hbuf=GWEN_Buffer_new(0, 32, 0, 1);
  rv=GWEN_CryptToken_Hash(GWEN_CryptToken_SignInfo_GetHashAlgo(si),
                          ptr, len,
			  hbuf);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    GWEN_Buffer_free(hbuf);
    return rv;
  }

  /* set security status */
  rv=LC_CryptTokenSTARCOS_VerifyPin(ct, GWEN_CryptToken_PinType_Access);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
    GWEN_Buffer_free(hbuf);
    return rv;
  }

  /* set security environment */
  res=LC_Card_IsoManageSe(lct->card, 0xb6,
                          kid & 0xff,
                          kid & 0xff,
                          pa);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error preparing signing (%d)", res);
    GWEN_Buffer_free(hbuf);
    return GWEN_ERROR_CT_IO_ERROR;
  }

  /* sign unpadded hash */
  GWEN_Buffer_Rewind(hbuf);

  res=LC_Card_IsoSign(lct->card,
                      GWEN_Buffer_GetStart(hbuf),
                      GWEN_Buffer_GetUsedBytes(hbuf),
                      dst);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error signing hash (%d)", res);
    GWEN_Buffer_free(hbuf);
    return GWEN_ERROR_CT_IO_ERROR;
  }
  GWEN_Buffer_free(hbuf);

  /* done */
  return 0;
}



int LC_CryptTokenSTARCOS_Verify(GWEN_CRYPTTOKEN *ct,
                                const GWEN_CRYPTTOKEN_CONTEXT *ctx,
                                const char *ptr,
                                unsigned int len,
                                const char *sigptr,
                                unsigned int siglen) {
  LC_CT_STARCOS *lct;
  const GWEN_CRYPTTOKEN_KEYINFO *ki;
  const GWEN_CRYPTTOKEN_SIGNINFO *si;
  int rv;
  GWEN_BUFFER *hbuf;
  LC_CLIENT_RESULT res;
  int pa;
  int kid;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_STARCOS, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  /* get sign info */
  si=GWEN_CryptToken_Context_GetSignInfo(ctx);
  assert(si);

  pa=GWEN_CryptToken_SignInfo_GetId(si);
  assert(pa);

  /* get keyinfo and perform some checks */
  ki=GWEN_CryptToken_Context_GetVerifyKeyInfo(ctx);
  assert(ki);

  kid=GWEN_CryptToken_KeyInfo_GetKeyId(ki);
  if (!(
        (kid>=0x81 && kid<=0x85) ||
        (kid>=0x91 && kid<=0x95)
       )
     ){
    DBG_ERROR(LC_LOGDOMAIN,
              "Expected KID 0x81-0x85 or 0x91-0x95 (is: %02x)",
              kid);
    return GWEN_ERROR_INVALID;
  }

  if (!(GWEN_CryptToken_KeyInfo_GetKeyFlags(ki) &
	GWEN_CRYPTTOKEN_KEYINFO_FLAGS_CAN_VERIFY)) {
    DBG_ERROR(LC_LOGDOMAIN, "Key can not be used for verification");
    return GWEN_ERROR_INVALID;
  }
  if (GWEN_CryptToken_KeyInfo_GetCryptAlgo(ki)!=
      GWEN_CryptToken_CryptAlgo_RSA) {
    DBG_ERROR(LC_LOGDOMAIN, "Invalid crypt algo");
    return GWEN_ERROR_INVALID;
  }

  /* hash data */
  hbuf=GWEN_Buffer_new(0, 32, 0, 1);
  rv=GWEN_CryptToken_Hash(GWEN_CryptToken_SignInfo_GetHashAlgo(si),
                          ptr, len,
			  hbuf);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    GWEN_Buffer_free(hbuf);
    return rv;
  }

  /* set security status */
  rv=LC_CryptTokenSTARCOS_VerifyPin(ct, GWEN_CryptToken_PinType_Access);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
    GWEN_Buffer_free(hbuf);
    return rv;
  }

  /* set security environment */
  res=LC_Card_IsoManageSe(lct->card, 0xb6,
                          0,
                          kid & 0xff,
                          pa);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error preparing verification (%d)", res);
    GWEN_Buffer_free(hbuf);
    return GWEN_ERROR_CT_IO_ERROR;
  }

  /* verify padded hash */
  GWEN_Buffer_Rewind(hbuf);
  res=LC_Card_IsoVerify(lct->card,
                        GWEN_Buffer_GetStart(hbuf),
                        GWEN_Buffer_GetUsedBytes(hbuf),
                        sigptr, siglen);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error signing hash (%d)", res);
    GWEN_Buffer_free(hbuf);
    return GWEN_ERROR_CT_IO_ERROR;
  }

  GWEN_Buffer_free(hbuf);
  DBG_INFO(LC_LOGDOMAIN, "Signature is valid");
  return 0;
}



int LC_CryptTokenSTARCOS_Encrypt(GWEN_CRYPTTOKEN *ct,
                                 const GWEN_CRYPTTOKEN_CONTEXT *ctx,
                                 const char *ptr,
                                 unsigned int len,
                                 GWEN_BUFFER *dst) {
  LC_CT_STARCOS *lct;
  const GWEN_CRYPTTOKEN_KEYINFO *ki;
  const GWEN_CRYPTTOKEN_CRYPTINFO *ci;
  LC_CLIENT_RESULT res;
  int pa;
  int kid;
  int rv;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_STARCOS, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  /* get crypt info */
  ci=GWEN_CryptToken_Context_GetCryptInfo(ctx);
  assert(ci);

  /* transform algo */
  pa=GWEN_CryptToken_CryptInfo_GetId(ci);
  assert(pa);

  /* get keyinfo and perform some checks */
  ki=GWEN_CryptToken_Context_GetEncryptKeyInfo(ctx);
  assert(ki);
  kid=GWEN_CryptToken_KeyInfo_GetKeyId(ki);
  if (!(
        (kid>=0x86 && kid<=0x8a) ||
        (kid>=0x96 && kid<=0x9a)
       )
     ){
    DBG_ERROR(LC_LOGDOMAIN,
              "Expected KID 0x86-0x8a or 0x96-0x9a (is: %02x)",
              kid);
    return GWEN_ERROR_INVALID;
  }

  if (!(GWEN_CryptToken_KeyInfo_GetKeyFlags(ki) &
	GWEN_CRYPTTOKEN_KEYINFO_FLAGS_CAN_ENCRYPT)) {
    DBG_ERROR(LC_LOGDOMAIN, "Key can not be used for encryption");
    return GWEN_ERROR_INVALID;
  }
  if (GWEN_CryptToken_KeyInfo_GetCryptAlgo(ki)!=
      GWEN_CryptToken_CryptAlgo_RSA) {
    DBG_ERROR(LC_LOGDOMAIN, "Invalid crypt algo");
    return GWEN_ERROR_INVALID;
  }

  /* set security status */
  rv=LC_CryptTokenSTARCOS_VerifyPin(ct, GWEN_CryptToken_PinType_Access);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  /* set security environment */
  res=LC_Card_IsoManageSe(lct->card, 0xb8,
                          0,
                          kid & 0xff,
                          pa);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error preparing encryption (%d)", res);
    return GWEN_ERROR_CT_IO_ERROR;
  }

  /* encrypt data */
  res=LC_Card_IsoEncipher(lct->card, ptr, len, dst);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error encrypting hash (%d)", res);
    return GWEN_ERROR_CT_IO_ERROR;
  }

  /* done */
  return 0;
}



int LC_CryptTokenSTARCOS_Decrypt(GWEN_CRYPTTOKEN *ct,
                                 const GWEN_CRYPTTOKEN_CONTEXT *ctx,
                                 const char *ptr,
                                 unsigned int len,
                                 GWEN_BUFFER *dst) {
  LC_CT_STARCOS *lct;
  const GWEN_CRYPTTOKEN_KEYINFO *ki;
  const GWEN_CRYPTTOKEN_CRYPTINFO *ci;
  LC_CLIENT_RESULT res;
  int kid;
  int pa;
  int rv;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_STARCOS, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  /* get crypt info */
  ci=GWEN_CryptToken_Context_GetCryptInfo(ctx);
  assert(ci);

  /* transform algo */
  pa=GWEN_CryptToken_CryptInfo_GetId(ci);
  assert(pa);

  /* get keyinfo and perform some checks */
  ki=GWEN_CryptToken_Context_GetDecryptKeyInfo(ctx);
  assert(ki);

  kid=GWEN_CryptToken_KeyInfo_GetKeyId(ki);
  if (!(kid>=0x86 && kid<=0x8a)){
    DBG_ERROR(LC_LOGDOMAIN,
              "Expected KID 0x86-0x8a or 0x96-0x9a (is: %02x)",
              kid);
    return GWEN_ERROR_INVALID;
  }

  if (!(GWEN_CryptToken_KeyInfo_GetKeyFlags(ki) &
	GWEN_CRYPTTOKEN_KEYINFO_FLAGS_CAN_ENCRYPT)) {
    DBG_ERROR(LC_LOGDOMAIN, "Key can not be used for decryption");
    return GWEN_ERROR_INVALID;
  }
  if (GWEN_CryptToken_KeyInfo_GetCryptAlgo(ki)!=
      GWEN_CryptToken_CryptAlgo_RSA) {
    DBG_ERROR(LC_LOGDOMAIN, "Invalid crypt algo");
    return GWEN_ERROR_INVALID;
  }

  /* set security status */
  rv=LC_CryptTokenSTARCOS_VerifyPin(ct, GWEN_CryptToken_PinType_Access);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  /* set security environment */
  res=LC_Card_IsoManageSe(lct->card, 0xb8,
                          kid & 0xff,
                          kid & 0xff,
                          pa);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error preparing encryption (%d)", res);
    return GWEN_ERROR_CT_IO_ERROR;
  }

  /* decrypt data */
  res=LC_Card_IsoDecipher(lct->card, ptr, len, dst);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error decrypting hash (%d)", res);
    return GWEN_ERROR_CT_IO_ERROR;
  }

  /* done */
  return 0;
}



int LC_CryptTokenSTARCOS_GetSignSeq(GWEN_CRYPTTOKEN *ct,
                                    GWEN_TYPE_UINT32 kid,
                                    GWEN_TYPE_UINT32 *signSeq) {
  LC_CT_STARCOS *lct;
  int seq;
  int rv;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_STARCOS, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  if ((kid & 0xff)<0x81 || (kid & 0xff)>0x85) {
    DBG_ERROR(LC_LOGDOMAIN,
              "Signing only allowed with kid 0x81-0x85 (is: %02x)",
              kid);
    return GWEN_ERROR_INVALID;
  }

  /* set security status */
  rv=LC_CryptTokenSTARCOS_VerifyPin(ct, GWEN_CryptToken_PinType_Access);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  /* read signature sequence counter from card */
  seq=LC_Starcos_ReadSigCounter(lct->card, kid);
  if (seq==-1) {
    DBG_ERROR(LC_LOGDOMAIN, "Bad signature sequence counter");
    return GWEN_ERROR_CT_IO_ERROR;
  }

  *signSeq=seq;

  return 0;
}



int LC_CryptTokenSTARCOS_FillUserList(GWEN_CRYPTTOKEN *ct,
                                      GWEN_CRYPTTOKEN_USER_LIST *ul) {
  LC_CT_STARCOS *lct;
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbData;
  GWEN_DB_NODE *dbT;
  int i;
  int rv;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_STARCOS, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  /* set security status */
  rv=LC_CryptTokenSTARCOS_VerifyPin(ct, GWEN_CryptToken_PinType_Access);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  dbData=GWEN_DB_Group_new("contexts");
  res=LC_Starcos_ReadInstituteData(lct->card, 0, dbData);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "No context available");
    GWEN_DB_Group_free(dbData);
    return GWEN_ERROR_NO_DATA;
  }

  dbT=GWEN_DB_FindFirstGroup(dbData, "context");
  i=1;
  while(dbT) {
    GWEN_CRYPTTOKEN_USER *u;
    const char *s;
    int j;

    u=GWEN_CryptToken_User_new();
    GWEN_CryptToken_User_SetId(u, i);
    s=GWEN_DB_GetCharValue(dbT, "userId", 0, 0);
    GWEN_CryptToken_User_SetUserId(u, s);
    GWEN_CryptToken_User_SetPeerId(u, s); /* same id for peer */
    s=GWEN_DB_GetCharValue(dbT, "bankName", 0, 0);
    GWEN_CryptToken_User_SetPeerName(u, s);
    s=GWEN_DB_GetCharValue(dbT, "bankCode", 0, 0);
    GWEN_CryptToken_User_SetServiceId(u, s);
    s=GWEN_DB_GetCharValue(dbT, "comAddress", 0, 0);
    GWEN_CryptToken_User_SetAddress(u, s);
    j=GWEN_DB_GetIntValue(dbT, "comService", 0, 2);
    switch(j) {
    case 0:
    case 1:
      break;
    case 2:
      GWEN_CryptToken_User_SetPort(u, 3000);
      break;
    case 3:
      GWEN_CryptToken_User_SetPort(u, 443);
      break;
    default:
      break;
    }

    GWEN_CryptToken_User_SetContextId(u, i);

    GWEN_CryptToken_User_List_Add(u, ul);
    i++;
    dbT=GWEN_DB_FindNextGroup(dbT, "context");
  }

  GWEN_DB_Group_free(dbData);
  return 0;
}



int LC_CryptTokenSTARCOS_ReadKey(GWEN_CRYPTTOKEN *ct,
                                 GWEN_TYPE_UINT32 kid,
                                 GWEN_CRYPTKEY **pkey) {
  LC_CT_STARCOS *lct;
  GWEN_CRYPTKEY *key;
  GWEN_KEYSPEC *ks;
  int rv;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_STARCOS, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  /* set security status */
  rv=LC_CryptTokenSTARCOS_VerifyPin(ct, GWEN_CryptToken_PinType_Access);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  key=LC_Starcos_ReadPublicKey(lct->card, kid);
  if (!key) {
    DBG_INFO(LC_LOGDOMAIN, "Could not read key 0x%x", kid);
    return GWEN_ERROR_CT_NO_KEY;
  }

  ks=LC_Starcos_GetKeySpec(lct->card, kid);
  if (!ks) {
    DBG_WARN(LC_LOGDOMAIN, "Could not read keyspec 0x%x", kid);
  }
  else {
    int x;

    x=GWEN_KeySpec_GetStatus(ks);
    GWEN_KeySpec_SetStatus(ks, LC_CryptTokenSTARCOS_Status_toCtStatus(x));
    GWEN_CryptKey_SetKeySpec(key, ks);
  }
  GWEN_KeySpec_free(ks);

  *pkey=key;

  return 0;
}



int LC_CryptTokenSTARCOS__ResultToError(LC_CLIENT_RESULT res) {
  int rv;

  switch(res) {
  case LC_Client_ResultOk:
    rv=0;
    break;
  case LC_Client_ResultWait:
    rv=GWEN_ERROR_TIMEOUT;
    break;
  case LC_Client_ResultIpcError:
  case LC_Client_ResultCmdError:
  case LC_Client_ResultDataError:
    rv=GWEN_ERROR_CT_IO_ERROR;
    break;

  case LC_Client_ResultAborted:
    rv=GWEN_ERROR_USER_ABORTED;
    break;

  case LC_Client_ResultInvalid:
    rv=GWEN_ERROR_INVALID;
    break;

  case LC_Client_ResultNoData:
    rv=GWEN_ERROR_NO_DATA;
    break;

  case LC_Client_ResultCardRemoved:
    rv=GWEN_ERROR_CT_REMOVED;
    break;

  case LC_Client_ResultNotSupported:
    rv=GWEN_ERROR_CT_NOT_SUPPORTED;
    break;

  case LC_Client_ResultInternal:
  case LC_Client_ResultGeneric:
  default:
    rv=GWEN_ERROR_GENERIC;
    break;
  }

  return rv;
}



int LC_CryptTokenSTARCOS_Status_toCtStatus(int i) {
  switch(i) {
  case LC_STARCOS_KEY_STATUS_INACTIVE_FREE:
    return GWEN_CRYPTTOKEN_KEYSTATUS_FREE;
  case LC_STARCOS_KEY_STATUS_ACTIVE_NEW:
    return GWEN_CRYPTTOKEN_KEYSTATUS_NEW;
  case LC_STARCOS_KEY_STATUS_ACTIVE:
    return GWEN_CRYPTTOKEN_KEYSTATUS_ACTIVE;
  default:
    return LC_STARCOS_KEY_STATUS_ACTIVE;
  }
}



int LC_CryptTokenSTARCOS_Status_fromCtStatus(int i) {
  switch(i) {
  case GWEN_CRYPTTOKEN_KEYSTATUS_FREE:
    return LC_STARCOS_KEY_STATUS_INACTIVE_FREE;
  case GWEN_CRYPTTOKEN_KEYSTATUS_NEW:
    return LC_STARCOS_KEY_STATUS_ACTIVE_NEW;
  case GWEN_CRYPTTOKEN_KEYSTATUS_ACTIVE:
    return LC_STARCOS_KEY_STATUS_ACTIVE;
  default:
    return GWEN_CRYPTTOKEN_KEYSTATUS_UNKNOWN;
  }
}



int LC_CryptTokenSTARCOS_WriteKey(GWEN_CRYPTTOKEN *ct,
                                  GWEN_TYPE_UINT32 kid,
                                  const GWEN_CRYPTKEY *key) {
  LC_CT_STARCOS *lct;
  LC_CLIENT_RESULT res;
  int rv;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_STARCOS, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  if (kid<0x91 || kid>0x9a) {
    DBG_ERROR(LC_LOGDOMAIN, "Can only write bank keys (%x)", kid);
    return GWEN_ERROR_INVALID;
  }

  /* set security status */
  rv=LC_CryptTokenSTARCOS_VerifyPin(ct, GWEN_CryptToken_PinType_Access);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  /* set manager security status */
  rv=LC_CryptTokenSTARCOS_VerifyPin(ct, GWEN_CryptToken_PinType_Manage);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  if (key) {
    GWEN_KEYSPEC *ks;
    int x;

    res=LC_Starcos_WritePublicKey(lct->card, kid, key);
    if (res!=LC_Client_ResultOk) {
      DBG_ERROR(LC_LOGDOMAIN, "Unable to write public key %x (%d)",
		kid, res);
      return LC_CryptTokenSTARCOS__ResultToError(res);
    }

    ks=GWEN_KeySpec_dup(GWEN_CryptKey_GetKeySpec(key));
    x=GWEN_KeySpec_GetStatus(ks);
    GWEN_KeySpec_SetStatus(ks, LC_CryptTokenSTARCOS_Status_fromCtStatus(x));

    res=LC_Starcos_SetKeySpec(lct->card, kid, ks);
    GWEN_KeySpec_free(ks);
    rv=LC_CryptTokenSTARCOS__ResultToError(res);
  }
  else {
    GWEN_KEYSPEC *ks;

    ks=GWEN_KeySpec_new();
    if (kid>0x90 && kid<0x96)
      GWEN_KeySpec_SetKeyName(ks, "S");
    else
      GWEN_KeySpec_SetKeyName(ks, "V");
    GWEN_KeySpec_SetStatus(ks, LC_STARCOS_KEY_STATUS_INACTIVE_FREE);
    res=LC_Starcos_SetKeySpec(lct->card, kid, ks);
    GWEN_KeySpec_free(ks);
    rv=LC_CryptTokenSTARCOS__ResultToError(res);
  }

  return rv;
}



int LC_CryptTokenSTARCOS_ReadKeySpec(GWEN_CRYPTTOKEN *ct,
                                     GWEN_TYPE_UINT32 kid,
                                     GWEN_KEYSPEC **pks) {
  LC_CT_STARCOS *lct;
  GWEN_KEYSPEC *ks;
  int rv;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_STARCOS, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  /* set security status */
  rv=LC_CryptTokenSTARCOS_VerifyPin(ct, GWEN_CryptToken_PinType_Access);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  ks=LC_Starcos_GetKeySpec(lct->card, kid);
  if (!ks) {
    DBG_INFO(LC_LOGDOMAIN, "Could not read keyspec 0x%x", kid);
    return GWEN_ERROR_CT_NO_KEY;
  }
  else {
    int x;

    x=GWEN_KeySpec_GetStatus(ks);
    GWEN_KeySpec_SetStatus(ks, LC_CryptTokenSTARCOS_Status_toCtStatus(x));
  }

  *pks=ks;

  return 0;
}



int LC_CryptTokenSTARCOS_WriteKeySpec(GWEN_CRYPTTOKEN *ct,
                                      GWEN_TYPE_UINT32 kid,
                                      const GWEN_KEYSPEC *ks) {
  LC_CT_STARCOS *lct;
  LC_CLIENT_RESULT res;
  int rv;
  int x;
  GWEN_KEYSPEC *ks2;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_STARCOS, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  /* set security status */
  rv=LC_CryptTokenSTARCOS_VerifyPin(ct, GWEN_CryptToken_PinType_Access);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  /* set security status */
  rv=LC_CryptTokenSTARCOS_VerifyPin(ct, GWEN_CryptToken_PinType_Manage);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  ks2=GWEN_KeySpec_dup(ks);
  x=GWEN_KeySpec_GetStatus(ks2);
  GWEN_KeySpec_SetStatus(ks2, LC_CryptTokenSTARCOS_Status_fromCtStatus(x));

  res=LC_Starcos_SetKeySpec(lct->card, kid, ks2);
  GWEN_KeySpec_free(ks2);

  rv=LC_CryptTokenSTARCOS__ResultToError(res);
  return rv;
}



int LC_CryptTokenSTARCOS_GenerateKey(GWEN_CRYPTTOKEN *ct,
                                     const GWEN_CRYPTTOKEN_KEYINFO *ki,
                                     GWEN_CRYPTKEY **key) {
  LC_CT_STARCOS *lct;
  LC_CLIENT_RESULT res;
  int rv;
  int srcKid;
  int dstKid;
  int bits;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_STARCOS, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  dstKid=GWEN_CryptToken_KeyInfo_GetKeyId(ki);
  if (dstKid>0x80 && dstKid<0x86)
    srcKid=0x8f;
  else
    srcKid=0x8e;
  bits=GWEN_CryptToken_KeyInfo_GetKeySize(ki);

  /* set security status */
  rv=LC_CryptTokenSTARCOS_VerifyPin(ct, GWEN_CryptToken_PinType_Access);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  /* set manager security status */
  rv=LC_CryptTokenSTARCOS_VerifyPin(ct, GWEN_CryptToken_PinType_Manage);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  res=LC_Starcos_GenerateKeyPair(lct->card, srcKid, bits);
  if (res) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not generate key %x (%x)",
              dstKid, srcKid);
  }
  else {
    GWEN_KEYSPEC *ks;

    ks=GWEN_KeySpec_new();
    GWEN_KeySpec_SetKeyType(ks, "rsa");
    if (srcKid==0x8e)
      GWEN_KeySpec_SetKeyName(ks, "V");
    else
      GWEN_KeySpec_SetKeyName(ks, "S");
    GWEN_KeySpec_SetNumber(ks, 1);
    GWEN_KeySpec_SetVersion(ks, 1);
    GWEN_KeySpec_SetStatus(ks, LC_STARCOS_KEY_STATUS_ACTIVE);
    res=LC_Starcos_ActivateKeyPair(lct->card, srcKid, dstKid, ks);
    GWEN_KeySpec_free(ks);
  }
  rv=LC_CryptTokenSTARCOS__ResultToError(res);

  return rv;
}






