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

#include "ddvcard_p.h"

#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <chipcard2-client/cards/ddvcard.h>
#include <chipcard2-client/cards/processorcard.h>


GWEN_INHERIT(GWEN_CRYPTTOKEN, LC_CT_DDV)
GWEN_INHERIT(GWEN_PLUGIN, LC_CT_PLUGIN_DDV)



GWEN_PLUGIN *crypttoken_ddv_factory(GWEN_PLUGIN_MANAGER *pm,
                                    const char *modName,
				    const char *fileName) {
  GWEN_PLUGIN *pl;

  pl=LC_CryptTokenDDV_Plugin_new(pm, modName, fileName);
  assert(pl);

  return pl;
}



GWEN_PLUGIN *LC_CryptTokenDDV_Plugin_new(GWEN_PLUGIN_MANAGER *pm,
					 const char *modName,
					 const char *fileName) {
  GWEN_PLUGIN *pl;
  LC_CT_PLUGIN_DDV *cpl;

  pl=GWEN_CryptToken_Plugin_new(pm,
				GWEN_CryptToken_Device_Card,
				modName,
				fileName);

  GWEN_NEW_OBJECT(LC_CT_PLUGIN_DDV, cpl);
  GWEN_INHERIT_SETDATA(GWEN_PLUGIN, LC_CT_PLUGIN_DDV, pl, cpl,
		       LC_CryptTokenDDV_Plugin_FreeData);
  cpl->client=LC_Client_new("LC_CryptTokenDDV", VERSION, 0);
  if (LC_Client_ReadConfigFile(cpl->client, 0)) {
    DBG_ERROR(LC_LOGDOMAIN,
	      "Error reading libchipcard2 client configuration.");
    GWEN_Plugin_free(pl);
    return 0;
  }

  /* set virtual functions */
  GWEN_CryptToken_Plugin_SetCreateTokenFn(pl,
					  LC_CryptTokenDDV_Plugin_CreateToken);
  GWEN_CryptToken_Plugin_SetCheckTokenFn(pl,
                                         LC_CryptTokenDDV_Plugin_CheckToken);

  return pl;
}



void LC_CryptTokenDDV_Plugin_FreeData(void *bp, void *p) {
  LC_CT_PLUGIN_DDV *cpl;

  cpl=(LC_CT_PLUGIN_DDV*)p;
  LC_Client_free(cpl->client);
  GWEN_FREE_OBJECT(cpl);
}



GWEN_CRYPTTOKEN *LC_CryptTokenDDV_Plugin_CreateToken(GWEN_PLUGIN *pl,
						     const char *subTypeName,
						     const char *name) {
  GWEN_PLUGIN_MANAGER *pm;
  GWEN_CRYPTTOKEN *ct;
  LC_CT_PLUGIN_DDV *cpl;

  assert(pl);
  cpl=GWEN_INHERIT_GETDATA(GWEN_PLUGIN, LC_CT_PLUGIN_DDV, pl);
  assert(cpl);

  pm=GWEN_Plugin_GetManager(pl);
  assert(pm);

  ct=LC_CryptTokenDDV_new(pm, cpl->client, name);
  assert(ct);

  return ct;
}



int LC_CryptTokenDDV_Plugin_CheckToken(GWEN_PLUGIN *pl,
				       GWEN_BUFFER *subTypeName,
				       GWEN_BUFFER *name) {
  GWEN_PLUGIN_MANAGER *pm;
  LC_CT_PLUGIN_DDV *cpl;
  LC_CLIENT_RESULT res;
  LC_CARD *hcard=0;
  const char *currCardNumber;

  assert(pl);
  cpl=GWEN_INHERIT_GETDATA(GWEN_PLUGIN, LC_CT_PLUGIN_DDV, pl);
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
    rv=LC_DDVCard_ExtendCard(hcard);
    if (rv) {
      DBG_ERROR(LC_LOGDOMAIN,
		"DDV card not available, please check your setup (%d)", rv);
      LC_Card_free(hcard);
      return GWEN_ERROR_NOT_AVAILABLE;
    }

    res=LC_Card_Open(hcard);
    if (res!=LC_Client_ResultOk) {
      LC_Card_free(hcard);
      DBG_NOTICE(LC_LOGDOMAIN,
		 "Could not open card (%d), maybe not a DDV card?",
		 res);
      return GWEN_ERROR_CT_NOT_SUPPORTED;
    } /* if card not open */
    else {
      GWEN_DB_NODE *dbCardData;

        dbCardData=LC_DDVCard_GetCardDataAsDb(hcard);
	assert(dbCardData);

        currCardNumber=GWEN_DB_GetCharValue(dbCardData,
                                            "cardNumber",
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









GWEN_CRYPTTOKEN *LC_CryptTokenDDV_new(GWEN_PLUGIN_MANAGER *pm,
                                      LC_CLIENT *lc,
                                      const char *name) {
  LC_CT_DDV *lct;
  GWEN_CRYPTTOKEN *ct;

  DBG_ERROR(0, "Creating crypttoken (DDV)");

  /* create crypt token */
  ct=GWEN_CryptToken_new(pm,
                         GWEN_CryptToken_Device_Card,
                         "ddvcard", 0, name);

  /* inherit CryptToken: Set our own data */
  GWEN_NEW_OBJECT(LC_CT_DDV, lct);
  GWEN_INHERIT_SETDATA(GWEN_CRYPTTOKEN, LC_CT_DDV, ct, lct,
                       LC_CryptTokenDDV_FreeData);
  lct->pluginManager=pm;
  lct->client=lc;

  /* set virtual functions */
  GWEN_CryptToken_SetOpenFn(ct, LC_CryptTokenDDV_Open);
  GWEN_CryptToken_SetCreateFn(ct, LC_CryptTokenDDV_Create);
  GWEN_CryptToken_SetCloseFn(ct, LC_CryptTokenDDV_Close);
  GWEN_CryptToken_SetSignFn(ct, LC_CryptTokenDDV_Sign);
  GWEN_CryptToken_SetVerifyFn(ct, LC_CryptTokenDDV_Verify);
  GWEN_CryptToken_SetEncryptFn(ct, LC_CryptTokenDDV_Encrypt);
  GWEN_CryptToken_SetDecryptFn(ct, LC_CryptTokenDDV_Decrypt);
  GWEN_CryptToken_SetGetSignSeqFn(ct, LC_CryptTokenDDV_GetSignSeq);
  GWEN_CryptToken_SetReadKeySpecFn(ct, LC_CryptTokenDDV_ReadKeySpec);
  GWEN_CryptToken_SetFillUserListFn(ct, LC_CryptTokenDDV_FillUserList);
  GWEN_CryptToken_SetGetTokenIdDataFn(ct, LC_CryptTokenDDV_GetTokenIdData);
  return ct;
}



void LC_CryptTokenDDV_FreeData(void *bp, void *p) {
  LC_CT_DDV *lct;

  lct=(LC_CT_DDV*)p;
  if (lct->card)
    LC_Card_free(lct->card);
  GWEN_FREE_OBJECT(lct);
}



int LC_CryptTokenDDV__EnterPin(GWEN_CRYPTTOKEN *ct,
                               LC_CARD *hcard,
                               GWEN_CRYPTTOKEN_PINTYPE pt) {
  LC_CT_DDV *lct;
  LC_CLIENT_RESULT res;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_DDV, ct);
  assert(lct);

  assert(hcard);

  if (LC_Card_GetReaderFlags(hcard) & LC_CARD_READERFLAGS_KEYPAD) {
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

    res=LC_DDVCard_SecureVerifyPin(hcard);
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

    DBG_INFO(LC_LOGDOMAIN, "No keypad (or disabled), will ask for PIN");
    memset(pinBuffer, 0, sizeof(pinBuffer));

    mres=GWEN_CryptManager_GetPin(lct->pluginManager,
                                  ct,
				  pt,
				  GWEN_CryptToken_PinEncoding_ASCII,
                                  GWEN_CRYPTTOKEN_GETPIN_FLAGS_NUMERIC,
                                  pinBuffer,
                                  4, 10, &pinLength);
    if (mres) {
      DBG_ERROR(LC_LOGDOMAIN, "Error asking for PIN, aborting");
      memset(pinBuffer, 0, sizeof(pinBuffer));
      return mres;
    }

    DBG_INFO(LC_LOGDOMAIN, "Verifying the PIN");

    res=LC_DDVCard_VerifyPin(hcard, pinBuffer);
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



int LC_CryptTokenDDV__GetCard(GWEN_CRYPTTOKEN *ct, int manage) {
  LC_CT_DDV *lct;
  LC_CLIENT_RESULT res;
  LC_CARD *hcard=0;
  int first;
  int havepin;
  const char *currCardNumber;
  const char *name;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_DDV, ct);
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
      rv=LC_DDVCard_ExtendCard(hcard);
      if (rv) {
        DBG_ERROR(LC_LOGDOMAIN,
                  "DDV card not available, please check your setup (%d)", rv);
        LC_Card_free(hcard);
        LC_Client_StopWait(lct->client);
        return GWEN_ERROR_NOT_AVAILABLE;
      }

      res=LC_Card_Open(hcard);
      if (res!=LC_Client_ResultOk) {
        LC_Card_free(hcard);
        DBG_NOTICE(LC_LOGDOMAIN,
                   "Could not open card (%d), maybe not a DDV card?",
                   res);
      } /* if card not open */
      else {
        GWEN_DB_NODE *dbCardData;

        dbCardData=LC_DDVCard_GetCardDataAsDb(hcard);
	assert(dbCardData);

        currCardNumber=GWEN_DB_GetCharValue(dbCardData,
                                            "cardNumber",
                                            0,
                                            0);
	if (!currCardNumber) {
          DBG_ERROR(LC_LOGDOMAIN, "INTERNAL: No card number in card data.");
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

  /* now get the access pin */
  havepin=0;
  while(!havepin) {
    int rv;

    /* enter pin */
    rv=LC_CryptTokenDDV__EnterPin(ct, hcard,
                                  GWEN_CryptToken_PinType_Access);
    if (rv) {
      LC_Card_Close(hcard);
      LC_Card_free(hcard);
      DBG_ERROR(LC_LOGDOMAIN, "Error in PIN input");
      return GWEN_ERROR_CT_IO_ERROR;
    }
    else
      havepin=1;
  } /* while !havepin */


  lct->card=hcard;
  return 0;
}



int LC_CryptTokenDDV_Open(GWEN_CRYPTTOKEN *ct, int manage) {
  LC_CT_DDV *lct;
  int rv;
  GWEN_XMLNODE *node;
  GWEN_XMLNODE *nct;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_DDV, ct);
  assert(lct);

  /* get card */
  rv=LC_CryptTokenDDV__GetCard(ct, manage);
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



int LC_CryptTokenDDV_Create(GWEN_CRYPTTOKEN *ct) {
  return 0;
}



int LC_CryptTokenDDV_Close(GWEN_CRYPTTOKEN *ct) {
  LC_CT_DDV *lct;
  LC_CLIENT_RESULT res;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_DDV, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

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




int LC_CryptTokenDDV_Sign(GWEN_CRYPTTOKEN *ct,
			  const GWEN_CRYPTTOKEN_CONTEXT *ctx,
                          GWEN_BUFFER *src,
                          GWEN_BUFFER *dst) {
  LC_CT_DDV *lct;
  const GWEN_CRYPTTOKEN_KEYINFO *ki;
  const GWEN_CRYPTTOKEN_SIGNINFO *si;
  int rv;
  GWEN_BUFFER *hbuf;
  LC_CLIENT_RESULT res;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_DDV, ct);
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
  if (GWEN_CryptToken_KeyInfo_GetKeyId(ki)!=1) {
    DBG_ERROR(LC_LOGDOMAIN, "Invalid key id");
    return GWEN_ERROR_INVALID;
  }
  if (!(GWEN_CryptToken_KeyInfo_GetKeyFlags(ki) &
	GWEN_CRYPTTOKEN_KEYINFO_FLAGS_CAN_SIGN)) {
    DBG_ERROR(LC_LOGDOMAIN, "Key can not be used for signing");
    return GWEN_ERROR_INVALID;
  }
  if (GWEN_CryptToken_KeyInfo_GetCryptAlgo(ki)!=
      GWEN_CryptToken_CryptAlgo_DES_3K) {
    DBG_ERROR(LC_LOGDOMAIN, "Invalid crypt algo");
    return GWEN_ERROR_INVALID;
  }

  /* hash data */
  hbuf=GWEN_Buffer_new(0, 32, 0, 1);
  rv=GWEN_CryptToken_Hash(GWEN_CryptToken_SignInfo_GetHashAlgo(si),
			  GWEN_Buffer_GetStart(src),
			  GWEN_Buffer_GetUsedBytes(src),
			  hbuf);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    GWEN_Buffer_free(hbuf);
    return rv;
  }

  /* padd hash */
  GWEN_Buffer_Rewind(hbuf);
  rv=GWEN_CryptToken_Padd(GWEN_CryptToken_SignInfo_GetPaddAlgo(si),
			  20,
			  hbuf);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    GWEN_Buffer_free(hbuf);
    return rv;
  }
  if (GWEN_Buffer_GetUsedBytes(hbuf)!=20) {
    DBG_ERROR(LC_LOGDOMAIN, "Bad padding (result!= 20 bytes)");
    GWEN_Buffer_free(hbuf);
    return GWEN_ERROR_INVALID;
  }

  /* sign padded hash */
  GWEN_Buffer_Rewind(hbuf);
  res=LC_DDVCard_SignHash(lct->card, hbuf, dst);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error signing hash (%d)", res);
    GWEN_Buffer_free(hbuf);
    return GWEN_ERROR_CT_IO_ERROR;
  }
  GWEN_Buffer_free(hbuf);

  rv=LC_CryptTokenDDV__IncSignSeq(ct, GWEN_CryptToken_KeyInfo_GetKeyId(ki));
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  /* done */
  return 0;
}



int LC_CryptTokenDDV_Verify(GWEN_CRYPTTOKEN *ct,
			    const GWEN_CRYPTTOKEN_CONTEXT *ctx,
			    GWEN_BUFFER *src,
			    GWEN_BUFFER *signature) {
  LC_CT_DDV *lct;
  const GWEN_CRYPTTOKEN_KEYINFO *ki;
  const GWEN_CRYPTTOKEN_SIGNINFO *si;
  int rv;
  GWEN_BUFFER *hbuf;
  GWEN_BUFFER *tmpsigbuf;
  LC_CLIENT_RESULT res;
  const char *p1, *p2;
  unsigned int bsize;
  unsigned int i;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_DDV, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  /* get sign info */
  si=GWEN_CryptToken_Context_GetSignInfo(ctx);
  assert(si);

  /* get keyinfo and perform some checks */
  ki=GWEN_CryptToken_Context_GetVerifyKeyInfo(ctx);
  assert(ki);
  if (GWEN_CryptToken_KeyInfo_GetKeyId(ki)!=1) {
    DBG_ERROR(LC_LOGDOMAIN, "Invalid key id");
    return GWEN_ERROR_INVALID;
  }
  if (!(GWEN_CryptToken_KeyInfo_GetKeyFlags(ki) &
	GWEN_CRYPTTOKEN_KEYINFO_FLAGS_CAN_VERIFY)) {
    DBG_ERROR(LC_LOGDOMAIN, "Key can not be used for verification");
    return GWEN_ERROR_INVALID;
  }
  if (GWEN_CryptToken_KeyInfo_GetCryptAlgo(ki)!=
      GWEN_CryptToken_CryptAlgo_DES_3K) {
    DBG_ERROR(LC_LOGDOMAIN, "Invalid crypt algo");
    return GWEN_ERROR_INVALID;
  }

  /* hash data */
  hbuf=GWEN_Buffer_new(0, 32, 0, 1);
  rv=GWEN_CryptToken_Hash(GWEN_CryptToken_SignInfo_GetHashAlgo(si),
			  GWEN_Buffer_GetStart(src),
			  GWEN_Buffer_GetUsedBytes(src),
			  hbuf);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    GWEN_Buffer_free(hbuf);
    return rv;
  }

  /* padd hash */
  GWEN_Buffer_Rewind(hbuf);
  rv=GWEN_CryptToken_Padd(GWEN_CryptToken_SignInfo_GetPaddAlgo(si),
			  20,
			  hbuf);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    GWEN_Buffer_free(hbuf);
    return rv;
  }

  /* sign padded hash */
  GWEN_Buffer_Rewind(hbuf);
  tmpsigbuf=GWEN_Buffer_new(0, 32, 0, 1);
  res=LC_DDVCard_SignHash(lct->card, hbuf, tmpsigbuf);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error signing hash (%d)", res);
    GWEN_Buffer_free(tmpsigbuf);
    GWEN_Buffer_free(hbuf);
    return GWEN_ERROR_CT_IO_ERROR;
  }

  /* compare signatures */
  p1=GWEN_Buffer_GetStart(signature);
  p2=GWEN_Buffer_GetStart(tmpsigbuf);
  bsize=GWEN_Buffer_GetUsedBytes(signature);
  if (bsize!=GWEN_Buffer_GetUsedBytes(tmpsigbuf)) {
    DBG_ERROR(LC_LOGDOMAIN, "Invalid signature (1) [%d!=%d]",
	      bsize, GWEN_Buffer_GetUsedBytes(tmpsigbuf));
    GWEN_Buffer_free(hbuf);
    GWEN_Buffer_free(tmpsigbuf);
    return GWEN_ERROR_VERIFY;
  }

  /* compare signatures */
  for (i=0; i<bsize; i++) {
    if (*p1++!=*p2++) {
      DBG_ERROR(LC_LOGDOMAIN, "Invalid signature (2)");
      GWEN_Buffer_free(hbuf);
      GWEN_Buffer_free(tmpsigbuf);
      return GWEN_ERROR_VERIFY;
    }
  } /* for */

  /* done */
  GWEN_Buffer_free(tmpsigbuf);
  GWEN_Buffer_free(hbuf);
  DBG_INFO(LC_LOGDOMAIN, "Signature is valid");
  return 0;
}



int LC_CryptTokenDDV_Encrypt(GWEN_CRYPTTOKEN *ct,
			     const GWEN_CRYPTTOKEN_CONTEXT *ctx,
			     GWEN_BUFFER *src,
			     GWEN_BUFFER *dst) {
  LC_CT_DDV *lct;
  const GWEN_CRYPTTOKEN_KEYINFO *ki;
  const GWEN_CRYPTTOKEN_CRYPTINFO *ci;
  int rv;
  GWEN_BUFFER *hbuf;
  unsigned int i;
  const char *p;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_DDV, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  /* get crypt info */
  ci=GWEN_CryptToken_Context_GetCryptInfo(ctx);
  assert(ci);

  /* get keyinfo and perform some checks */
  ki=GWEN_CryptToken_Context_GetEncryptKeyInfo(ctx);
  assert(ki);
  if (GWEN_CryptToken_KeyInfo_GetKeyId(ki)!=2) {
    DBG_ERROR(LC_LOGDOMAIN, "Invalid key id");
    return GWEN_ERROR_INVALID;
  }
  if (!(GWEN_CryptToken_KeyInfo_GetKeyFlags(ki) &
	GWEN_CRYPTTOKEN_KEYINFO_FLAGS_CAN_ENCRYPT)) {
    DBG_ERROR(LC_LOGDOMAIN, "Key can not be used for signing");
    return GWEN_ERROR_INVALID;
  }
  if (GWEN_CryptToken_KeyInfo_GetCryptAlgo(ki)!=
      GWEN_CryptToken_CryptAlgo_DES_3K) {
    DBG_ERROR(LC_LOGDOMAIN, "Invalid crypt algo");
    return GWEN_ERROR_INVALID;
  }

  /* copy data */
  hbuf=GWEN_Buffer_new(0, GWEN_Buffer_GetUsedBytes(src), 0, 1);
  GWEN_Buffer_AppendBuffer(hbuf, src);

  /* padd data */
  GWEN_Buffer_Rewind(hbuf);
  rv=GWEN_CryptToken_Padd(GWEN_CryptToken_CryptInfo_GetPaddAlgo(ci),
			  GWEN_CryptToken_KeyInfo_GetChunkSize(ki),
			  hbuf);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    GWEN_Buffer_free(hbuf);
    return rv;
  }

  /* encrypt data */
  p=GWEN_Buffer_GetStart(hbuf);
  i=GWEN_Buffer_GetUsedBytes(hbuf)/GWEN_CryptToken_KeyInfo_GetChunkSize(ki);
  while(i--) {
    LC_CLIENT_RESULT res;

    res=LC_DDVCard_CryptCharBlock(lct->card, p,
				  GWEN_CryptToken_KeyInfo_GetChunkSize(ki),
				  dst);
    if (res!=LC_Client_ResultOk) {
      DBG_ERROR(LC_LOGDOMAIN, "Error encrypting hash (%d)", res);
      GWEN_Buffer_free(hbuf);
      return GWEN_ERROR_CT_IO_ERROR;
    }

    p+=GWEN_CryptToken_KeyInfo_GetChunkSize(ki);
  }

  /* done */
  GWEN_Buffer_free(hbuf);
  return 0;
}



int LC_CryptTokenDDV_Decrypt(GWEN_CRYPTTOKEN *ct,
			     const GWEN_CRYPTTOKEN_CONTEXT *ctx,
			     GWEN_BUFFER *src,
			     GWEN_BUFFER *dst) {
  LC_CT_DDV *lct;
  const GWEN_CRYPTTOKEN_KEYINFO *ki;
  const GWEN_CRYPTTOKEN_CRYPTINFO *ci;
  int rv;
  GWEN_BUFFER *hbuf;
  unsigned int i;
  const char *p;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_DDV, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  /* get crypt info */
  ci=GWEN_CryptToken_Context_GetCryptInfo(ctx);
  assert(ci);

  /* get keyinfo and perform some checks */
  ki=GWEN_CryptToken_Context_GetDecryptKeyInfo(ctx);
  assert(ki);
  if (GWEN_CryptToken_KeyInfo_GetKeyId(ki)!=2) {
    DBG_ERROR(LC_LOGDOMAIN, "Invalid key id");
    return GWEN_ERROR_INVALID;
  }
  if (!(GWEN_CryptToken_KeyInfo_GetKeyFlags(ki) &
	GWEN_CRYPTTOKEN_KEYINFO_FLAGS_CAN_ENCRYPT)) {
    DBG_ERROR(LC_LOGDOMAIN, "Key can not be used for signing");
    return GWEN_ERROR_INVALID;
  }
  if (GWEN_CryptToken_KeyInfo_GetCryptAlgo(ki)!=
      GWEN_CryptToken_CryptAlgo_DES_3K) {
    DBG_ERROR(LC_LOGDOMAIN, "Invalid crypt algo");
    return GWEN_ERROR_INVALID;
  }

  /* decrypt data */
  p=GWEN_Buffer_GetStart(src);
  i=GWEN_Buffer_GetUsedBytes(src)/GWEN_CryptToken_KeyInfo_GetChunkSize(ki);
  hbuf=GWEN_Buffer_new(0, GWEN_Buffer_GetUsedBytes(src), 0, 1);
  while(i--) {
    LC_CLIENT_RESULT res;

    res=LC_DDVCard_CryptCharBlock(lct->card, p,
				  GWEN_CryptToken_KeyInfo_GetChunkSize(ki),
				  hbuf);
    if (res!=LC_Client_ResultOk) {
      DBG_ERROR(LC_LOGDOMAIN, "Error encrypting hash (%d)", res);
      GWEN_Buffer_free(hbuf);
      return GWEN_ERROR_CT_IO_ERROR;
    }

    p+=GWEN_CryptToken_KeyInfo_GetChunkSize(ki);
  }

  /* unpadd data */
  GWEN_Buffer_Rewind(hbuf);
  rv=GWEN_CryptToken_Unpadd(GWEN_CryptToken_CryptInfo_GetPaddAlgo(ci),
			    hbuf);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    GWEN_Buffer_free(hbuf);
    return rv;
  }

  /* copy data to user buffer */
  GWEN_Buffer_AppendBuffer(dst, hbuf);

  /* done */
  GWEN_Buffer_free(hbuf);
  return 0;
}



int LC_CryptTokenDDV__IncSignSeq(GWEN_CRYPTTOKEN *ct,
                                 GWEN_TYPE_UINT32 kid) {
  LC_CT_DDV *lct;
  LC_CLIENT_RESULT res;
  GWEN_BUFFER *mbuf;
  GWEN_DB_NODE *dbRecord;
  int seq;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_DDV, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  /* get keyinfo and perform some checks */
  if (kid!=1) {
    DBG_ERROR(LC_LOGDOMAIN, "Invalid key id");
    return GWEN_ERROR_INVALID;
  }

  /* read signature sequence counter from card */
  res=LC_ProcessorCard_SelectEF(lct->card, "EF_SEQ");
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "here");
    return GWEN_ERROR_CT_IO_ERROR;
  }

  mbuf=GWEN_Buffer_new(0, 4, 0, 1);
  res=LC_ProcessorCard_ReadRecord(lct->card, 1, mbuf);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "here");
    GWEN_Buffer_free(mbuf);
    return GWEN_ERROR_CT_IO_ERROR;
  }
  GWEN_Buffer_Rewind(mbuf);
  dbRecord=GWEN_DB_Group_new("seq");
  if (LC_Card_ParseRecord(lct->card, 1, mbuf, dbRecord)) {
    DBG_ERROR(LC_LOGDOMAIN, "Error parsing record");
    GWEN_DB_Group_free(dbRecord);
    GWEN_Buffer_free(mbuf);
    return GWEN_ERROR_CT_IO_ERROR;
  }

  seq=GWEN_DB_GetIntValue(dbRecord, "seq", 0, -1);
  if (seq==-1) {
    DBG_ERROR(LC_LOGDOMAIN, "Bad record data in EF_SEQ");
    GWEN_DB_Group_free(dbRecord);
    GWEN_Buffer_free(mbuf);
    return GWEN_ERROR_CT_IO_ERROR;
  }

  seq++;
  GWEN_DB_SetIntValue(dbRecord, GWEN_DB_FLAGS_OVERWRITE_VARS,
		      "seq", seq);
  GWEN_Buffer_Reset(mbuf);
  if (LC_Card_CreateRecord(lct->card, 1, mbuf, dbRecord)) {
    DBG_ERROR(LC_LOGDOMAIN, "Error creating record");
    GWEN_DB_Group_free(dbRecord);
    GWEN_Buffer_free(mbuf);
    return GWEN_ERROR_CT_IO_ERROR;
  }
  GWEN_Buffer_Rewind(mbuf);
  res=LC_ProcessorCard_WriteRecord(lct->card, 1, mbuf);
  GWEN_DB_Group_free(dbRecord);
  GWEN_Buffer_free(mbuf);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    return GWEN_ERROR_CT_IO_ERROR;
  }

  return 0;
}



int LC_CryptTokenDDV_GetSignSeq(GWEN_CRYPTTOKEN *ct,
				GWEN_TYPE_UINT32 kid,
				GWEN_TYPE_UINT32 *signSeq) {
  LC_CT_DDV *lct;
  LC_CLIENT_RESULT res;
  GWEN_BUFFER *mbuf;
  GWEN_DB_NODE *dbRecord;
  int seq;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_DDV, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  /* get keyinfo and perform some checks */
  if (kid!=1) {
    DBG_ERROR(LC_LOGDOMAIN, "Invalid key id");
    return GWEN_ERROR_INVALID;
  }

  /* read signature sequence counter from card */
  res=LC_ProcessorCard_SelectEF(lct->card, "EF_SEQ");
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "here");
    return GWEN_ERROR_CT_IO_ERROR;
  }

  mbuf=GWEN_Buffer_new(0, 4, 0, 1);
  res=LC_ProcessorCard_ReadRecord(lct->card, 1, mbuf);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "here");
    GWEN_Buffer_free(mbuf);
    return GWEN_ERROR_CT_IO_ERROR;
  }
  GWEN_Buffer_Rewind(mbuf);
  dbRecord=GWEN_DB_Group_new("seq");
  if (LC_Card_ParseRecord(lct->card, 1, mbuf, dbRecord)) {
    DBG_ERROR(LC_LOGDOMAIN, "Error parsing record");
    GWEN_DB_Group_free(dbRecord);
    GWEN_Buffer_free(mbuf);
    return GWEN_ERROR_CT_IO_ERROR;
  }

  seq=GWEN_DB_GetIntValue(dbRecord, "seq", 0, -1);
  if (seq==-1) {
    DBG_ERROR(LC_LOGDOMAIN, "Bad record data in EF_SEQ");
    GWEN_DB_Group_free(dbRecord);
    GWEN_Buffer_free(mbuf);
    return GWEN_ERROR_CT_IO_ERROR;
  }

  *signSeq=seq;

  return 0;
}



int LC_CryptTokenDDV_ReadKeySpec(GWEN_CRYPTTOKEN *ct,
				 GWEN_TYPE_UINT32 kid,
				 GWEN_KEYSPEC **pks) {
  LC_CT_DDV *lct;
  GWEN_KEYSPEC *ks;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_DDV, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  /* get keyinfo and perform some checks */
  if (kid!=1 && kid!=2) {
    DBG_ERROR(LC_LOGDOMAIN, "Invalid key id");
    return GWEN_ERROR_INVALID;
  }

  /* read and create key spec */
  ks=GWEN_KeySpec_new();
  GWEN_KeySpec_SetKeyType(ks, "des");
  if (kid==1) {
    int i;

    i=LC_DDVCard_GetSignKeyNumber(lct->card);
    if (i==-1) {
      DBG_WARN(LC_LOGDOMAIN,
	       "Could not get the sign key number, assuming 1");
      i=1;
    }
    GWEN_KeySpec_SetNumber(ks, i);

    i=LC_DDVCard_GetSignKeyVersion(lct->card);
    if (i==-1) {
      DBG_WARN(LC_LOGDOMAIN,
	       "Could not get the sign key version, assuming 1");
      i=1;
    }
    GWEN_KeySpec_SetVersion(ks, i);
  }
  else {
    int i;

    i=LC_DDVCard_GetCryptKeyNumber(lct->card);
    if (i==-1) {
      DBG_WARN(LC_LOGDOMAIN,
	       "Could not get the crypt key number, assuming 1");
      i=1;
    }
    GWEN_KeySpec_SetNumber(ks, i);

    i=LC_DDVCard_GetCryptKeyVersion(lct->card);
    if (i==-1) {
      DBG_WARN(LC_LOGDOMAIN,
	       "Could not get the crypt key version, assuming 1");
      i=1;
    }
    GWEN_KeySpec_SetVersion(ks, i);
  }

  *pks=ks;
  return 0;
}



int LC_CryptTokenDDV_FillUserList(GWEN_CRYPTTOKEN *ct,
				  GWEN_CRYPTTOKEN_USER_LIST *ul) {
  LC_CT_DDV *lct;
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbData;
  GWEN_DB_NODE *dbT;
  int i;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_DDV, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  dbData=GWEN_DB_Group_new("contexts");
  res=LC_DDVCard_ReadInstituteData(lct->card, 0, dbData);
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
    GWEN_CryptToken_User_SetId(u, i++);
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

    /* all users use the only available context */
    GWEN_CryptToken_User_SetContextId(u, 1);

    GWEN_CryptToken_User_List_Add(u, ul);
    dbT=GWEN_DB_FindNextGroup(dbT, "context");
  }

  GWEN_DB_Group_free(dbData);
  return 0;
}



int LC_CryptTokenDDV_GetTokenIdData(GWEN_CRYPTTOKEN *ct, GWEN_BUFFER *buf){
  LC_CT_DDV *lct;
  GWEN_BUFFER *dbuf;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPTTOKEN, LC_CT_DDV, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  dbuf=LC_DDVCard_GetCardDataAsBuffer(lct->card);
  if (dbuf==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card data");
    return GWEN_ERROR_CT_IO_ERROR;
  }

  GWEN_Buffer_AppendBuffer(buf, dbuf);

  return 0;
}







