/***************************************************************************
    begin       : Tue Jun 21 2011
    copyright   : (C) 2011 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "zkacard_p.h"

#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/ctplugin_be.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/padd.h>
#include <gwenhywfar/gui.h>
#include <gwenhywfar/i18n.h>

#include <chipcard/cards/zkacard.h>
#include <chipcard/cards/processorcard.h>
#include <chipcard/ct/ct_card.h>


#define I18N(message) GWEN_I18N_Translate("libchipcard", message)



GWEN_INHERIT(GWEN_CRYPT_TOKEN, LC_CT_ZKA)
GWEN_INHERIT(GWEN_PLUGIN, LC_CT_PLUGIN_ZKA)



GWEN_PLUGIN *ct_zkacard_factory(GWEN_PLUGIN_MANAGER *pm,
                                const char *modName,
                                const char *fileName) {
  GWEN_PLUGIN *pl;

  pl=LC_Crypt_TokenZka_Plugin_new(pm, modName, fileName);
  if (pl==NULL) {
    DBG_ERROR(LC_LOGDOMAIN, "No plugin created");
    return NULL;
  }
  return pl;
}



GWEN_PLUGIN *LC_Crypt_TokenZka_Plugin_new(GWEN_PLUGIN_MANAGER *pm,
					  const char *modName,
					  const char *fileName) {
  GWEN_PLUGIN *pl;
  LC_CT_PLUGIN_ZKA *cpl;
  LC_CLIENT_RESULT res;

  pl=GWEN_Crypt_Token_Plugin_new(pm,
				 GWEN_Crypt_Token_Device_Card,
				 modName,
				 fileName);

  GWEN_NEW_OBJECT(LC_CT_PLUGIN_ZKA, cpl);
  GWEN_INHERIT_SETDATA(GWEN_PLUGIN, LC_CT_PLUGIN_ZKA, pl, cpl,
		       LC_Crypt_TokenZka_Plugin_FreeData);
  cpl->client=LC_Client_new("LC_Crypt_TokenZka", VERSION);
  res=LC_Client_Init(cpl->client);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN,
	      "Error initialising libchipcard (%d), chipcards will not be available",
	      res);
    GWEN_Plugin_free(pl);
    return NULL;
  }

  /* set virtual functions */
  GWEN_Crypt_Token_Plugin_SetCreateTokenFn(pl, LC_Crypt_TokenZka_Plugin_CreateToken);
  GWEN_Crypt_Token_Plugin_SetCheckTokenFn(pl, LC_Crypt_TokenZka_Plugin_CheckToken);
  return pl;
}



void GWENHYWFAR_CB LC_Crypt_TokenZka_Plugin_FreeData(void *bp, void *p) {
  LC_CT_PLUGIN_ZKA *cpl;

  cpl=(LC_CT_PLUGIN_ZKA*)p;
  LC_Client_free(cpl->client);
  GWEN_FREE_OBJECT(cpl);
}



GWEN_CRYPT_TOKEN* GWENHYWFAR_CB
LC_Crypt_TokenZka_Plugin_CreateToken(GWEN_PLUGIN *pl,
				     const char *name) {
  GWEN_PLUGIN_MANAGER *pm;
  GWEN_CRYPT_TOKEN *ct;
  LC_CT_PLUGIN_ZKA *cpl;

  assert(pl);
  cpl=GWEN_INHERIT_GETDATA(GWEN_PLUGIN, LC_CT_PLUGIN_ZKA, pl);
  assert(cpl);

  pm=GWEN_Plugin_GetManager(pl);
  assert(pm);

  ct=LC_Crypt_TokenZka_new(pm, cpl->client, name);
  assert(ct);

  return ct;
}



int GWENHYWFAR_CB
LC_Crypt_TokenZka_Plugin_CheckToken(GWEN_PLUGIN *pl,
				    GWEN_BUFFER *name) {
  GWEN_PLUGIN_MANAGER *pm;
  LC_CT_PLUGIN_ZKA *cpl;
  LC_CLIENT_RESULT res;
  LC_CARD *hcard=0;
  const char *currCardNumber;
  int i;

  assert(pl);
  cpl=GWEN_INHERIT_GETDATA(GWEN_PLUGIN, LC_CT_PLUGIN_ZKA, pl);
  assert(cpl);

  pm=GWEN_Plugin_GetManager(pl);
  assert(pm);

  res=LC_Client_Start(cpl->client);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send StartWait request");
    return GWEN_ERROR_IO;
  }

  for (i=0;i<10;i++) {
    res=LC_Client_GetNextCard(cpl->client, &hcard, i==0?5:10);
    if (res==LC_Client_ResultOk)
      break;
    else {
      if (res==LC_Client_ResultWait) {
	int mres;

	mres=GWEN_Gui_MessageBox(GWEN_GUI_MSG_FLAGS_SEVERITY_NORMAL |
				 GWEN_GUI_MSG_FLAGS_CONFIRM_B1 |
				 GWEN_GUI_MSG_FLAGS_TYPE_INFO,
				 I18N("Insert card"),
				 I18N("Please insert a chipcard into the reader "
				      "and click a button."
				      "<html>"
				      "Please insert a chipcard into the reader "
				      "and click a button."
				      "</html>"),
				 I18N("Ok"),
				 I18N("Abort"),
				 NULL,
				 0);
	if (mres!=1) {
	  DBG_ERROR(LC_LOGDOMAIN, "Error in user interaction (%d)", mres);
	  LC_Client_Stop(cpl->client);
	  return GWEN_ERROR_USER_ABORTED;
	}
      }
      else {
	GWEN_Gui_ProgressLog(0, GWEN_LoggerLevel_Error,
			     I18N("Error while waiting for card"));
	LC_Client_Stop(cpl->client);
	return GWEN_ERROR_IO;
      }
    }
  }

  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN,
	      "No card within specified timeout (%d)", res);
    LC_Client_Stop(cpl->client);
    return GWEN_ERROR_IO;
  }
  else {
    int rv;

    assert(hcard);
    /* ok, we have a card, don't wait for more */
    LC_Client_Stop(cpl->client);
    /* check card */
    rv=LC_ZkaCard_ExtendCard(hcard);
    if (rv) {
      DBG_ERROR(LC_LOGDOMAIN,
		"ZKA card not available, please check your setup (%d)", rv);
      LC_Client_ReleaseCard(cpl->client, hcard);
      LC_Card_free(hcard);
      return GWEN_ERROR_NOT_AVAILABLE;
    }

    res=LC_Card_Open(hcard);
    if (res!=LC_Client_ResultOk) {
      LC_Client_ReleaseCard(cpl->client, hcard);
      LC_Card_free(hcard);
      DBG_NOTICE(LC_LOGDOMAIN,
		 "Could not open card (%d), maybe not a ZKA card?",
		 res);
      return GWEN_ERROR_NOT_SUPPORTED;
    } /* if card not open */
    else {
      GWEN_DB_NODE *dbCardData;

        dbCardData=LC_ZkaCard_GetCardDataAsDb(hcard);
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
            LC_Client_ReleaseCard(cpl->client, hcard);
            LC_Card_free(hcard);
	    return GWEN_ERROR_BAD_NAME;
	  }
	}

        LC_Card_Close(hcard);
        LC_Client_ReleaseCard(cpl->client, hcard);
        LC_Card_free(hcard);
        hcard=0;
    } /* if card is open */
    return 0;
  } /* if there is a card */
}



GWEN_CRYPT_TOKEN *LC_Crypt_TokenZka_new(GWEN_PLUGIN_MANAGER *pm,
					LC_CLIENT *lc,
					const char *name) {
  LC_CT_ZKA *lct;
  GWEN_CRYPT_TOKEN *ct;

  DBG_INFO(LC_LOGDOMAIN, "Creating crypttoken (Zka)");

  /* create crypt token */
  ct=GWEN_Crypt_Token_new(GWEN_Crypt_Token_Device_Card,
			  "zkacard", name);

  /* inherit CryptToken: Set our own data */
  GWEN_NEW_OBJECT(LC_CT_ZKA, lct);
  GWEN_INHERIT_SETDATA(GWEN_CRYPT_TOKEN, LC_CT_ZKA, ct, lct,
                       LC_Crypt_TokenZka_FreeData);
  lct->pluginManager=pm;
  lct->client=lc;

  /* set virtual functions */
  GWEN_Crypt_Token_SetOpenFn(ct, LC_Crypt_TokenZka_Open);
  GWEN_Crypt_Token_SetCloseFn(ct, LC_Crypt_TokenZka_Close);

  GWEN_Crypt_Token_SetGetKeyIdListFn(ct, LC_Crypt_TokenZka_GetKeyIdList);
  GWEN_Crypt_Token_SetGetKeyInfoFn(ct, LC_Crypt_TokenZka_GetKeyInfo);
  GWEN_Crypt_Token_SetSetKeyInfoFn(ct, LC_Crypt_TokenZka_SetKeyInfo);
  GWEN_Crypt_Token_SetGetContextIdListFn(ct, LC_Crypt_TokenZka_GetContextIdList);
  GWEN_Crypt_Token_SetGetContextFn(ct, LC_Crypt_TokenZka_GetContext);
  GWEN_Crypt_Token_SetSetContextFn(ct, LC_Crypt_TokenZka_SetContext);

  GWEN_Crypt_Token_SetSignFn(ct, LC_Crypt_TokenZka_Sign);
  GWEN_Crypt_Token_SetVerifyFn(ct, LC_Crypt_TokenZka_Verify);
  GWEN_Crypt_Token_SetEncipherFn(ct, LC_Crypt_TokenZka_Encipher);
  GWEN_Crypt_Token_SetDecipherFn(ct, LC_Crypt_TokenZka_Decipher);

  GWEN_Crypt_Token_SetGenerateKeyFn(ct, LC_Crypt_TokenZka_GenerateKey);

  return ct;
}



void GWENHYWFAR_CB LC_Crypt_TokenZka_FreeData(void *bp, void *p) {
  LC_CT_ZKA *lct;

  lct=(LC_CT_ZKA*)p;
  if (lct->card) {
    LC_Client_ReleaseCard(lct->client, lct->card);
    LC_Card_free(lct->card);
  }
  GWEN_FREE_OBJECT(lct);
}



int LC_Crypt_TokenZka__GetCard(GWEN_CRYPT_TOKEN *ct, uint32_t guiid) {
  LC_CT_ZKA *lct;
  LC_CLIENT_RESULT res;
  LC_CARD *hcard=0;
  int first;
  const char *currCardNumber;
  const char *name;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, LC_CT_ZKA, ct);
  assert(lct);

  name=GWEN_Crypt_Token_GetTokenName(ct);

  res=LC_Client_Start(lct->client);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send Start request");
    return GWEN_ERROR_IO;
  }

  first=1;
  hcard=0;
  for (;;) {
    int timeout;

    /* determine timeout value */
    if (first)
      timeout=3;
    else
      timeout=5;

    if (hcard==0) {
      res=LC_Client_GetNextCard(lct->client, &hcard, timeout);
      if (res!=LC_Client_ResultOk &&
	  res!=LC_Client_ResultWait) {
	DBG_ERROR(LC_LOGDOMAIN, "Error while waiting for card (%d)", res);
	return GWEN_ERROR_IO;
      }
    }
    if (!hcard) {
      int mres;

      mres=GWEN_Crypt_Token_InsertToken(ct, guiid);
      if (mres) {
        DBG_ERROR(LC_LOGDOMAIN, "Error in user interaction (%d)", mres);
        LC_Client_Stop(lct->client);
        return GWEN_ERROR_USER_ABORTED;
      }
    }
    else {
      int rv;

      /* ok, we have a card, now check it */
      rv=LC_ZkaCard_ExtendCard(hcard);
      if (rv) {
        DBG_ERROR(LC_LOGDOMAIN,
                  "Zka card not available, please check your setup (%d)", rv);
        LC_Client_ReleaseCard(lct->client, hcard);
        LC_Card_free(hcard);
	LC_Client_Stop(lct->client);
        return GWEN_ERROR_NOT_AVAILABLE;
      }

      res=LC_Card_Open(hcard);
      if (res!=LC_Client_ResultOk) {
        LC_Client_ReleaseCard(lct->client, hcard);
        LC_Card_free(hcard);
        hcard=0;
        DBG_NOTICE(LC_LOGDOMAIN,
                   "Could not open card (%d), maybe not a Zka card?",
                   res);
      } /* if card not open */
      else {
        GWEN_DB_NODE *dbCardData;

        dbCardData=LC_ZkaCard_GetCardDataAsDb(hcard);
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
          GWEN_Crypt_Token_SetTokenName(ct, currCardNumber);
          name=GWEN_Crypt_Token_GetTokenName(ct);
          break;
        }

        if (strcasecmp(name, currCardNumber)==0) {
          DBG_NOTICE(LC_LOGDOMAIN, "Card number equals");
          break;
        }

        LC_Card_Close(hcard);
        LC_Client_ReleaseCard(lct->client, hcard);
        LC_Card_free(hcard);
        hcard=0;

	res=LC_Client_GetNextCard(lct->client, &hcard, GWEN_TIMEOUT_NONE);
	if (res!=LC_Client_ResultOk) {
	  int mres;

	  if (res!=LC_Client_ResultWait) {
	    DBG_ERROR(LC_LOGDOMAIN,
		      "Communication error (%d)", res);
	    LC_Client_Stop(lct->client);
	    return GWEN_ERROR_IO;
	  }

	  mres=GWEN_Crypt_Token_InsertCorrectToken(ct, guiid);
	  if (mres) {
	    DBG_ERROR(LC_LOGDOMAIN, "Error in user interaction (%d)", mres);
	    LC_Client_Stop(lct->client);
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
  LC_Client_Stop(lct->client);

  lct->card=hcard;
  return 0;
}



int LC_Crypt_TokenZka__EnsureAccessPin(GWEN_CRYPT_TOKEN *ct, uint32_t guiid) {
  LC_CT_ZKA *lct;
  const LC_PININFO *pi;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, LC_CT_ZKA, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  pi=LC_ZkaCard_GetPinInfo(lct->card);

  while(!lct->haveAccessPin) {
    int rv;

    /* enter pin */
    if (pi)
      rv=LC_Crypt_Token_VerifyPinWithPinInfo(ct, lct->card,
                                             GWEN_Crypt_PinType_Access,
                                             pi,
                                             guiid);
    else
      rv=LC_Crypt_Token_VerifyPin(ct, lct->card,
                                  GWEN_Crypt_PinType_Access,
                                  guiid);
    if (rv) {
      DBG_ERROR(LC_LOGDOMAIN, "Error in PIN input");
      return rv;
    }
    else
      lct->haveAccessPin=1;
  } /* while !havepin */

  return 0;
}



int GWENHYWFAR_CB
LC_Crypt_TokenZka_Open(GWEN_CRYPT_TOKEN *ct, int manage, uint32_t guiid) {
  LC_CT_ZKA *lct;
  int rv;
  int i;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, LC_CT_ZKA, ct);
  assert(lct);

  /* reset pin status */
  lct->haveAccessPin=0;
  lct->haveAdminPin=0;

  /* reset key info */
  for (i=0; i<LC_CT_ZKA_NUM_KEY; i++) {
    GWEN_Crypt_Token_KeyInfo_free(lct->keyInfos[i]);
    lct->keyInfos[i]=NULL;
  }

  /* reset context info */
  for (i=0; i<LC_CT_ZKA_NUM_CONTEXT; i++) {
    GWEN_Crypt_Token_Context_free(lct->contexts[i]);
    lct->contexts[i]=NULL;
  }
  lct->contextListIsValid=0;

  /* get card */
  rv=LC_Crypt_TokenZka__GetCard(ct, guiid);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  return 0;
}



int GWENHYWFAR_CB
LC_Crypt_TokenZka_Close(GWEN_CRYPT_TOKEN *ct,
			int abandon,
			uint32_t guiid) {
  LC_CT_ZKA *lct;
  LC_CLIENT_RESULT res;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, LC_CT_ZKA, ct);
  assert(lct);

  if (lct->card==0) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  res=LC_Card_Close(lct->card);
  if (res!=LC_Client_ResultOk) {
    LC_Client_ReleaseCard(lct->client, lct->card);
    LC_Card_free(lct->card);
    lct->card=0;
    return GWEN_ERROR_IO;
  }

  res=LC_Client_ReleaseCard(lct->client, lct->card);
  LC_Card_free(lct->card);
  lct->card=0;
  if (res!=LC_Client_ResultOk)
    return GWEN_ERROR_IO;

  return 0;
}



int GWENHYWFAR_CB
LC_Crypt_TokenZka_GetKeyIdList(GWEN_CRYPT_TOKEN *ct,
				   uint32_t *pIdList,
				   uint32_t *pCount,
				   uint32_t gid) {
  LC_CT_ZKA *lct;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, LC_CT_ZKA, ct);
  assert(lct);

  assert(pCount);

  if (!(lct->contextListIsValid)) {
    int rv;

    /* read the context list */
    rv=LC_Crypt_TokenZka__ReadContextList(ct, gid);
    if (rv<0) {
      DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
      return rv;
    }
  }

  if (pIdList) {
    int i;
    int cnt=0;

    for (i=0; i<LC_CT_ZKA_NUM_KEY; i++) {
      if (lct->keyInfos[i]!=NULL) {
        if (cnt<*pCount)
	  pIdList[cnt]=GWEN_Crypt_Token_KeyInfo_GetKeyId(lct->keyInfos[i]);
        else {
          DBG_ERROR(LC_LOGDOMAIN, "Id buffer too small (at %d)", cnt);
          return GWEN_ERROR_BUFFER_OVERFLOW;
        }
        cnt++;
      }
    }
    *pCount=cnt;
  }
  else {
    int i;
    int cnt=0;

    for (i=0; i<LC_CT_ZKA_NUM_KEY; i++) {
      if (lct->keyInfos[i]!=NULL)
        cnt++;
    }
    *pCount=cnt;
  }

  return 0;
}



const GWEN_CRYPT_TOKEN_KEYINFO* GWENHYWFAR_CB
LC_Crypt_TokenZka_GetKeyInfo(GWEN_CRYPT_TOKEN *ct,
			     uint32_t kid,
			     uint32_t flags,
			     uint32_t gid) {
  LC_CT_ZKA *lct;
  int i;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, LC_CT_ZKA, ct);
  assert(lct);

  if (!(lct->contextListIsValid)) {
    int rv;

    /* read the context list */
    rv=LC_Crypt_TokenZka__ReadContextList(ct, gid);
    if (rv<0) {
      DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
      return NULL;
    }
  }

  for (i=0; i<LC_CT_ZKA_NUM_KEY; i++) {
    if (lct->keyInfos[i]!=NULL &&
	GWEN_Crypt_Token_KeyInfo_GetKeyId(lct->keyInfos[i])==kid)
      return lct->keyInfos[i];
  }

  return NULL;
}



int LC_Crypt_TokenZka_SetKeyInfo(GWEN_CRYPT_TOKEN *ct,
                                 uint32_t id,
                                 const GWEN_CRYPT_TOKEN_KEYINFO *ki,
                                 uint32_t gid) {
  return GWEN_ERROR_NOT_IMPLEMENTED;
}



int GWENHYWFAR_CB
LC_Crypt_TokenZka_GetContextIdList(GWEN_CRYPT_TOKEN *ct,
				   uint32_t *pIdList,
				   uint32_t *pCount,
				   uint32_t gid) {
  LC_CT_ZKA *lct;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, LC_CT_ZKA, ct);
  assert(lct);

  assert(pCount);

  if (!(lct->contextListIsValid)) {
    int rv;

    /* read the context list */
    rv=LC_Crypt_TokenZka__ReadContextList(ct, gid);
    if (rv<0) {
      DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
      return rv;
    }
  }

  if (pIdList) {
    int i;
    int cnt=0;

    for (i=0; i<LC_CT_ZKA_NUM_CONTEXT; i++) {
      if (lct->contexts[i]!=NULL) {
        if (cnt<*pCount)
          pIdList[cnt]=GWEN_Crypt_Token_Context_GetId(lct->contexts[i]);
        else {
          DBG_ERROR(LC_LOGDOMAIN, "Id buffer too small (at %d)", cnt);
          return GWEN_ERROR_BUFFER_OVERFLOW;
        }
        cnt++;
      }
    }
    *pCount=cnt;
  }
  else {
    int i;
    int cnt=0;

    for (i=0; i<LC_CT_ZKA_NUM_CONTEXT; i++) {
      if (lct->contexts[i]!=NULL)
        cnt++;
    }
    *pCount=cnt;
  }

  return 0;
}



const GWEN_CRYPT_TOKEN_CONTEXT* GWENHYWFAR_CB
LC_Crypt_TokenZka_GetContext(GWEN_CRYPT_TOKEN *ct,
			     uint32_t id,
			     uint32_t gid) {
  LC_CT_ZKA *lct;
  int i;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, LC_CT_ZKA, ct);
  assert(lct);

  if (!(lct->contextListIsValid)) {
    int rv;

    /* read the context list */
    rv=LC_Crypt_TokenZka__ReadContextList(ct, gid);
    if (rv<0) {
      DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
      return NULL;
    }
  }

  for (i=0; i<LC_CT_ZKA_NUM_CONTEXT; i++) {
    if (lct->contexts[i]!=NULL &&
	GWEN_Crypt_Token_Context_GetId(lct->contexts[i])==id)
      return lct->contexts[i];
  }

  return NULL;
}



int GWENHYWFAR_CB
LC_Crypt_TokenZka_SetContext(GWEN_CRYPT_TOKEN *ct,
			     uint32_t id,
			     const GWEN_CRYPT_TOKEN_CONTEXT *ctx,
			     uint32_t gid) {
  return GWEN_ERROR_NOT_IMPLEMENTED;
}



int GWENHYWFAR_CB
LC_Crypt_TokenZka_GenerateKey(GWEN_CRYPT_TOKEN *ct,
				  uint32_t kid,
				  const GWEN_CRYPT_CRYPTALGO *a,
				  uint32_t gid) {
  return GWEN_ERROR_NOT_IMPLEMENTED;
}



int GWENHYWFAR_CB
LC_Crypt_TokenZka_Sign(GWEN_CRYPT_TOKEN *ct,
			   uint32_t kid,
			   GWEN_CRYPT_PADDALGO *a,
			   const uint8_t *pInData,
			   uint32_t inLen,
			   uint8_t *pSignatureData,
			   uint32_t *pSignatureLen,
			   uint32_t *pSeqCounter,
			   uint32_t gid) {
  LC_CT_ZKA *lct;
  LC_CLIENT_RESULT res;
  GWEN_BUFFER *tbuf;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, LC_CT_ZKA, ct);
  assert(lct);

  res=LC_Card_SelectMf(lct->card);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error selecting MF (%d)", res);
    return GWEN_ERROR_NOT_OPEN;
  }

  res=LC_Card_SelectDf(lct->card, "DF_SIG");
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error selecting DF_SIG (%d)", res);
    return GWEN_ERROR_NOT_OPEN;
  }

  tbuf=GWEN_Buffer_new(0, 256, 0, 1);
  res=LC_ZkaCard_Sign(lct->card, 1, 2, -1, pInData, inLen, tbuf);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
    GWEN_Buffer_free(tbuf);
    return GWEN_ERROR_NOT_OPEN;
  }


  return GWEN_ERROR_NOT_IMPLEMENTED;
}



int GWENHYWFAR_CB
LC_Crypt_TokenZka_Verify(GWEN_CRYPT_TOKEN *ct,
			     uint32_t kid,
			     GWEN_CRYPT_PADDALGO *a,
			     const uint8_t *pInData,
			     uint32_t inLen,
			     const uint8_t *pSignatureData,
			     uint32_t signatureLen,
			     uint32_t seqCounter,
			     uint32_t gid) {
  return GWEN_ERROR_NOT_IMPLEMENTED;
}



int GWENHYWFAR_CB
LC_Crypt_TokenZka_Encipher(GWEN_CRYPT_TOKEN *ct,
			       uint32_t kid,
			       GWEN_CRYPT_PADDALGO *a,
			       const uint8_t *pInData,
			       uint32_t inLen,
			       uint8_t *pOutData,
			       uint32_t *pOutLen,
			       uint32_t gid) {
  return GWEN_ERROR_NOT_IMPLEMENTED;
}



int GWENHYWFAR_CB
LC_Crypt_TokenZka_Decipher(GWEN_CRYPT_TOKEN *ct,
			       uint32_t kid,
			       GWEN_CRYPT_PADDALGO *a,
			       const uint8_t *pInData,
			       uint32_t inLen,
			       uint8_t *pOutData,
			       uint32_t *pOutLen,
			       uint32_t gid) {
  return GWEN_ERROR_NOT_IMPLEMENTED;
}




int LC_Crypt_TokenZka__ReadNotePad(GWEN_CRYPT_TOKEN *ct, GWEN_DB_NODE *dbNotePad, uint32_t guiid) {
  LC_CT_ZKA *lct;
  int i;
  LC_CLIENT_RESULT res;
  GWEN_BUFFER *mbuf;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, LC_CT_ZKA, ct);
  assert(lct);

  res=LC_Card_SelectMf(lct->card);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error selecting MF (%d)", res);
    return GWEN_ERROR_NOT_OPEN;
  }

  res=LC_Card_SelectDf(lct->card, "DF_NOTEPAD");
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error selecting DF_NOTEPAD (%d)", res);
    return GWEN_ERROR_NOT_OPEN;
  }

  res=LC_Card_SelectEf(lct->card, "EF_NOTEPAD");
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error selecting EF_NOTEPAD (%d)", res);
    return GWEN_ERROR_NOT_OPEN;
  }


  mbuf=GWEN_Buffer_new(0, 256, 0, 1);
  for (i=1; i<31; i++) {
    /* read record */
    DBG_INFO(LC_LOGDOMAIN, "Reading entry %d", i);
    res=LC_Card_IsoReadRecord(lct->card, LC_CARD_ISO_FLAGS_RECSEL_GIVEN, i, mbuf);
    if (res!=LC_Client_ResultOk) {
      if (LC_Card_GetLastSW1(lct->card)==0x6a &&
          LC_Card_GetLastSW2(lct->card)==0x83) {
        DBG_INFO(LC_LOGDOMAIN, "All records read (%d)", i-1);
        break;
      }
      else {
        DBG_ERROR(LC_LOGDOMAIN, "Error reading record %d of EF_NOTEPAD (%d)", i, res);
        if (i>1)
          break;
        GWEN_Buffer_free(mbuf);
        return GWEN_ERROR_IO;
      }
    }
    else {
      GWEN_DB_NODE *dbEntry;

      /* parse record */
      if (GWEN_Buffer_GetBytesLeft(mbuf)) {
        DBG_INFO(LC_LOGDOMAIN, "Parsing entry %d", i);
        dbEntry=GWEN_DB_Group_new("entry");
        GWEN_Buffer_Rewind(mbuf);
        res=LC_Card_ParseRecord(lct->card, i, mbuf, dbEntry);
        if (res!=LC_Client_ResultOk) {
          DBG_ERROR(LC_LOGDOMAIN, "Error parsing record %d of EF_NOTEPAD (%d)", i, res);
          if (i>1)
            break;
          GWEN_DB_Group_free(dbEntry);
          GWEN_Buffer_free(mbuf);
          return GWEN_ERROR_IO;
        }

        /* add new entry */
        DBG_INFO(LC_LOGDOMAIN, "Adding entry %d", i);
        GWEN_DB_AddGroup(dbNotePad, dbEntry);
      }
      else {
        DBG_INFO(LC_LOGDOMAIN, "Entry %d is empty", i);
      }
    }
    GWEN_Buffer_Reset(mbuf);
  }
  GWEN_Buffer_free(mbuf);

  return 0;
}



GWEN_CRYPT_TOKEN_KEYINFO *LC_Crypt_TokenZka__FindKeyInfo(GWEN_CRYPT_TOKEN *ct, uint32_t id) {
  LC_CT_ZKA *lct;
  int i;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, LC_CT_ZKA, ct);
  assert(lct);

  for (i=0; i<LC_CT_ZKA_NUM_KEY; i++) {
    if (lct->keyInfos[i]!=NULL && GWEN_Crypt_Token_KeyInfo_GetKeyId(lct->keyInfos[i])==id)
      return lct->keyInfos[i];
  }

  return NULL;
}



int LC_Crypt_TokenZka__AddKeyInfo(GWEN_CRYPT_TOKEN *ct, GWEN_CRYPT_TOKEN_KEYINFO *ki) {
  LC_CT_ZKA *lct;
  int i;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, LC_CT_ZKA, ct);
  assert(lct);

  for (i=0; i<LC_CT_ZKA_NUM_KEY; i++) {
    if (lct->keyInfos[i]==NULL) {
      lct->keyInfos[i]=ki;
      return 0;
    }
  }

  return GWEN_ERROR_BUFFER_OVERFLOW;
}



int LC_Crypt_TokenZka__ReadContextList(GWEN_CRYPT_TOKEN *ct, uint32_t guiid) {
  LC_CT_ZKA *lct;
  GWEN_DB_NODE *dbNotePad;
  GWEN_DB_NODE *dbEntry;
  int rv;
  int i;
  int cnt;

  assert(ct);
  lct=GWEN_INHERIT_GETDATA(GWEN_CRYPT_TOKEN, LC_CT_ZKA, ct);
  assert(lct);

  if (lct->card==NULL) {
    DBG_ERROR(LC_LOGDOMAIN, "No card.");
    return GWEN_ERROR_NOT_OPEN;
  }

  /* ensure access pin, because this is needed for reading the notepad */
  rv=LC_Crypt_TokenZka__EnsureAccessPin(ct, guiid);
  if (rv<0) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }

  dbNotePad=GWEN_DB_Group_new("notepad");
  rv=LC_Crypt_TokenZka__ReadNotePad(ct, dbNotePad, guiid);
  DBG_ERROR(0, "Notepad data:");
  GWEN_DB_Dump(dbNotePad, 2);
  if (rv<0) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
    GWEN_DB_Group_free(dbNotePad);
    return rv;
  }

  /* reset context info */
  for (i=0; i<LC_CT_ZKA_NUM_CONTEXT; i++) {
    GWEN_Crypt_Token_Context_free(lct->contexts[i]);
    lct->contexts[i]=NULL;
  }
  lct->contextListIsValid=0;

  cnt=0;
  dbEntry=GWEN_DB_GetFirstGroup(dbNotePad);
  while(dbEntry && cnt<LC_CT_ZKA_NUM_CONTEXT) {
    GWEN_DB_NODE *dbCtx;

    dbCtx=GWEN_DB_GetGroup(dbEntry, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "entry/hbciData");
    if (dbCtx) {
      GWEN_CRYPT_TOKEN_CONTEXT *ctx;
      GWEN_DB_NODE *dbT;
      const char *s;
      GWEN_CRYPT_TOKEN_KEYINFO *ki;
      uint32_t kid;
      int j;

      ctx=GWEN_Crypt_Token_Context_new();
      GWEN_Crypt_Token_Context_SetId(ctx, cnt+1);

      /* preset/precreate keys */

      /* peer crypt key */
      kid=((cnt+1)<<8)+LC_CT_ZKA_PEER_CRYPT_KEY;
      GWEN_Crypt_Token_Context_SetEncipherKeyId(ctx, kid);
      ki=LC_Crypt_TokenZka__FindKeyInfo(ct, kid);
      if (ki==NULL) {
	/* assume key size of 256, this will later be adjusted */
	ki=GWEN_Crypt_Token_KeyInfo_new(kid, GWEN_Crypt_CryptAlgoId_Rsa, 256);
	GWEN_Crypt_Token_KeyInfo_SetKeyDescr(ki, I18N("Peer Crypt Key"));
	rv=LC_Crypt_TokenZka__AddKeyInfo(ct, ki);
	if (rv<0) {
	  DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
	  GWEN_Crypt_Token_Context_free(ctx);
	  GWEN_DB_Group_free(dbNotePad);
	  return rv;
	}
      }
      GWEN_Crypt_Token_KeyInfo_AddFlags(ki,
					GWEN_CRYPT_TOKEN_KEYFLAGS_HASACTIONFLAGS |
					GWEN_CRYPT_TOKEN_KEYFLAGS_CANENCIPHER);

      /* peer sign key */
      kid=((cnt+1)<<8)+LC_CT_ZKA_PEER_SIGN_KEY;
      GWEN_Crypt_Token_Context_SetVerifyKeyId(ctx, kid);
      ki=LC_Crypt_TokenZka__FindKeyInfo(ct, kid);
      if (ki==NULL) {
	/* assume key size of 256, this will later be adjusted */
	ki=GWEN_Crypt_Token_KeyInfo_new(kid, GWEN_Crypt_CryptAlgoId_Rsa, 256);
	GWEN_Crypt_Token_KeyInfo_SetKeyDescr(ki, I18N("Peer Sign Key"));
	rv=LC_Crypt_TokenZka__AddKeyInfo(ct, ki);
	if (rv<0) {
	  DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
	  GWEN_Crypt_Token_Context_free(ctx);
	  GWEN_DB_Group_free(dbNotePad);
	  return rv;
	}
      }
      GWEN_Crypt_Token_KeyInfo_AddFlags(ki,
					GWEN_CRYPT_TOKEN_KEYFLAGS_HASACTIONFLAGS |
					GWEN_CRYPT_TOKEN_KEYFLAGS_CANVERIFY);

      /* peer auth key */
      kid=((cnt+1)<<8)+LC_CT_ZKA_PEER_AUTH_KEY;
      GWEN_Crypt_Token_Context_SetAuthVerifyKeyId(ctx, kid);
      ki=LC_Crypt_TokenZka__FindKeyInfo(ct, kid);
      if (ki==NULL) {
	/* assume key size of 256, this will later be adjusted */
	ki=GWEN_Crypt_Token_KeyInfo_new(kid, GWEN_Crypt_CryptAlgoId_Rsa, 256);
	GWEN_Crypt_Token_KeyInfo_SetKeyDescr(ki, I18N("Peer Auth Key"));
	rv=LC_Crypt_TokenZka__AddKeyInfo(ct, ki);
	if (rv<0) {
	  DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
	  GWEN_Crypt_Token_Context_free(ctx);
	  GWEN_DB_Group_free(dbNotePad);
	  return rv;
	}
      }
      GWEN_Crypt_Token_KeyInfo_AddFlags(ki,
					GWEN_CRYPT_TOKEN_KEYFLAGS_HASACTIONFLAGS |
					GWEN_CRYPT_TOKEN_KEYFLAGS_CANVERIFY);

      /* user sign key */
      kid=((cnt+1)<<8)+LC_CT_ZKA_USER_SIGN_KEY;
      GWEN_Crypt_Token_Context_SetSignKeyId(ctx, kid);
      ki=LC_Crypt_TokenZka__FindKeyInfo(ct, kid);
      if (ki==NULL) {
	/* assume key size of 256, this will later be adjusted */
	ki=GWEN_Crypt_Token_KeyInfo_new(kid, GWEN_Crypt_CryptAlgoId_Rsa, 256);
	GWEN_Crypt_Token_KeyInfo_SetKeyDescr(ki, I18N("User Sign Key"));
	rv=LC_Crypt_TokenZka__AddKeyInfo(ct, ki);
	if (rv<0) {
	  DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
	  GWEN_Crypt_Token_Context_free(ctx);
	  GWEN_DB_Group_free(dbNotePad);
	  return rv;
	}
      }
      GWEN_Crypt_Token_KeyInfo_AddFlags(ki,
					GWEN_CRYPT_TOKEN_KEYFLAGS_HASACTIONFLAGS |
					GWEN_CRYPT_TOKEN_KEYFLAGS_CANSIGN |
					GWEN_CRYPT_TOKEN_KEYFLAGS_CANVERIFY);

      /* user crypt key */
      kid=((cnt+1)<<8)+LC_CT_ZKA_USER_CRYPT_KEY;
      GWEN_Crypt_Token_Context_SetDecipherKeyId(ctx, kid);
      ki=LC_Crypt_TokenZka__FindKeyInfo(ct, kid);
      if (ki==NULL) {
	/* assume key size of 256, this will later be adjusted */
	ki=GWEN_Crypt_Token_KeyInfo_new(kid, GWEN_Crypt_CryptAlgoId_Rsa, 256);
	GWEN_Crypt_Token_KeyInfo_SetKeyDescr(ki, I18N("User Crypt Key"));
	rv=LC_Crypt_TokenZka__AddKeyInfo(ct, ki);
	if (rv<0) {
	  DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
	  GWEN_Crypt_Token_Context_free(ctx);
	  GWEN_DB_Group_free(dbNotePad);
	  return rv;
	}
      }
      GWEN_Crypt_Token_KeyInfo_AddFlags(ki,
					GWEN_CRYPT_TOKEN_KEYFLAGS_HASACTIONFLAGS |
					GWEN_CRYPT_TOKEN_KEYFLAGS_CANENCIPHER |
					GWEN_CRYPT_TOKEN_KEYFLAGS_CANDECIPHER);

      /* user auth key */
      kid=((cnt+1)<<8)+LC_CT_ZKA_USER_AUTH_KEY;
      GWEN_Crypt_Token_Context_SetAuthSignKeyId(ctx, kid);
      ki=LC_Crypt_TokenZka__FindKeyInfo(ct, kid);
      if (ki==NULL) {
	/* assume key size of 256, this will later be adjusted */
	ki=GWEN_Crypt_Token_KeyInfo_new(kid, GWEN_Crypt_CryptAlgoId_Rsa, 256);
	GWEN_Crypt_Token_KeyInfo_SetKeyDescr(ki, I18N("User Auth Key"));
	rv=LC_Crypt_TokenZka__AddKeyInfo(ct, ki);
	if (rv<0) {
	  DBG_INFO(LC_LOGDOMAIN, "here (%d)", rv);
	  GWEN_Crypt_Token_Context_free(ctx);
	  GWEN_DB_Group_free(dbNotePad);
	  return rv;
	}
      }
      GWEN_Crypt_Token_KeyInfo_AddFlags(ki,
					GWEN_CRYPT_TOKEN_KEYFLAGS_HASACTIONFLAGS |
					GWEN_CRYPT_TOKEN_KEYFLAGS_CANSIGN |
					GWEN_CRYPT_TOKEN_KEYFLAGS_CANVERIFY);


      /* read institute info */
      dbT=GWEN_DB_GetGroup(dbCtx, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "institute");
      if (dbT) {
        s=GWEN_DB_GetCharValue(dbT, "bankName", 0, NULL);
        if (s)
          GWEN_Crypt_Token_Context_SetPeerName(ctx, s);
        s=GWEN_DB_GetCharValue(dbT, "bankCode", 0, NULL);
        if (s) {
          GWEN_Crypt_Token_Context_SetPeerId(ctx, s);
          GWEN_Crypt_Token_Context_SetServiceId(ctx, s);
	}

	/* update sign key info */
	kid=((cnt+1)<<8)+LC_CT_ZKA_PEER_SIGN_KEY;
	GWEN_Crypt_Token_Context_SetVerifyKeyId(ctx, kid);
	ki=LC_Crypt_TokenZka__FindKeyInfo(ct, kid);
	assert(ki);

	s=GWEN_DB_GetCharValue(dbT, "keyNum", 0, NULL);
	if (s && 1==sscanf(s, "%d", &j)) {
	  GWEN_Crypt_Token_KeyInfo_SetKeyNumber(ki, j);
	  GWEN_Crypt_Token_KeyInfo_AddFlags(ki, GWEN_CRYPT_TOKEN_KEYFLAGS_HASKEYNUMBER);
	}

	s=GWEN_DB_GetCharValue(dbT, "keyVer", 0, NULL);
	if (s && 1==sscanf(s, "%d", &j)) {
	  GWEN_Crypt_Token_KeyInfo_SetKeyVersion(ki, j);
	  GWEN_Crypt_Token_KeyInfo_AddFlags(ki, GWEN_CRYPT_TOKEN_KEYFLAGS_HASKEYVERSION);
	}
      }

      dbT=GWEN_DB_GetGroup(dbCtx, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "commData");
      if (dbT) {
        s=GWEN_DB_GetCharValue(dbT, "address", 0, NULL);
        if (s)
          GWEN_Crypt_Token_Context_SetAddress(ctx, s);
      }

      dbT=GWEN_DB_GetGroup(dbCtx, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "user");
      if (dbT) {
        s=GWEN_DB_GetCharValue(dbT, "userId", 0, NULL);
        if (s)
          GWEN_Crypt_Token_Context_SetUserId(ctx, s);
        s=GWEN_DB_GetCharValue(dbT, "customerId", 0, NULL);
        if (s)
          GWEN_Crypt_Token_Context_SetCustomerId(ctx, s);

	/* update sign key info */
	kid=((cnt+1)<<8)+LC_CT_ZKA_USER_SIGN_KEY;
	GWEN_Crypt_Token_Context_SetSignKeyId(ctx, kid);
	ki=LC_Crypt_TokenZka__FindKeyInfo(ct, kid);
	assert(ki);

	s=GWEN_DB_GetCharValue(dbT, "signKeyNum", 0, NULL);
	if (s && 1==sscanf(s, "%d", &j)) {
	  GWEN_Crypt_Token_KeyInfo_SetKeyNumber(ki, j);
	  GWEN_Crypt_Token_KeyInfo_AddFlags(ki, GWEN_CRYPT_TOKEN_KEYFLAGS_HASKEYNUMBER);
	}

	s=GWEN_DB_GetCharValue(dbT, "signKeyVer", 0, NULL);
	if (s && 1==sscanf(s, "%d", &j)) {
	  GWEN_Crypt_Token_KeyInfo_SetKeyVersion(ki, j);
	  GWEN_Crypt_Token_KeyInfo_AddFlags(ki, GWEN_CRYPT_TOKEN_KEYFLAGS_HASKEYVERSION);
	}

	GWEN_Crypt_Token_KeyInfo_AddFlags(ki,
					  GWEN_CRYPT_TOKEN_KEYFLAGS_HASACTIONFLAGS |
					  GWEN_CRYPT_TOKEN_KEYFLAGS_CANSIGN |
					  GWEN_CRYPT_TOKEN_KEYFLAGS_CANVERIFY);

	/* update crypt key info */
	kid=((cnt+1)<<8)+LC_CT_ZKA_USER_CRYPT_KEY;
	GWEN_Crypt_Token_Context_SetDecipherKeyId(ctx, kid);
	ki=LC_Crypt_TokenZka__FindKeyInfo(ct, kid);
	assert(ki);

	s=GWEN_DB_GetCharValue(dbT, "cryptKeyNum", 0, NULL);
	if (s && 1==sscanf(s, "%d", &j)) {
	  GWEN_Crypt_Token_KeyInfo_SetKeyNumber(ki, j);
	  GWEN_Crypt_Token_KeyInfo_AddFlags(ki, GWEN_CRYPT_TOKEN_KEYFLAGS_HASKEYNUMBER);
	}

	s=GWEN_DB_GetCharValue(dbT, "cryptKeyVer", 0, NULL);
	if (s && 1==sscanf(s, "%d", &j)) {
	  GWEN_Crypt_Token_KeyInfo_SetKeyVersion(ki, j);
	  GWEN_Crypt_Token_KeyInfo_AddFlags(ki, GWEN_CRYPT_TOKEN_KEYFLAGS_HASKEYVERSION);
	}
	GWEN_Crypt_Token_KeyInfo_AddFlags(ki,
					  GWEN_CRYPT_TOKEN_KEYFLAGS_HASACTIONFLAGS |
					  GWEN_CRYPT_TOKEN_KEYFLAGS_CANENCIPHER |
					  GWEN_CRYPT_TOKEN_KEYFLAGS_CANDECIPHER);

	/* update auth key info */
	kid=((cnt+1)<<8)+LC_CT_ZKA_USER_AUTH_KEY;
	GWEN_Crypt_Token_Context_SetAuthSignKeyId(ctx, kid);
	ki=LC_Crypt_TokenZka__FindKeyInfo(ct, kid);
	assert(ki);

	s=GWEN_DB_GetCharValue(dbT, "authKeyNum", 0, NULL);
	if (s && 1==sscanf(s, "%d", &j)) {
	  GWEN_Crypt_Token_KeyInfo_SetKeyNumber(ki, j);
	  GWEN_Crypt_Token_KeyInfo_AddFlags(ki, GWEN_CRYPT_TOKEN_KEYFLAGS_HASKEYNUMBER);
	}

	s=GWEN_DB_GetCharValue(dbT, "authKeyVer", 0, NULL);
	if (s && 1==sscanf(s, "%d", &j)) {
	  GWEN_Crypt_Token_KeyInfo_SetKeyVersion(ki, j);
	  GWEN_Crypt_Token_KeyInfo_AddFlags(ki, GWEN_CRYPT_TOKEN_KEYFLAGS_HASKEYVERSION);
	}

	GWEN_Crypt_Token_KeyInfo_AddFlags(ki,
					  GWEN_CRYPT_TOKEN_KEYFLAGS_HASACTIONFLAGS |
					  GWEN_CRYPT_TOKEN_KEYFLAGS_CANSIGN |
					  GWEN_CRYPT_TOKEN_KEYFLAGS_CANVERIFY);
      }

      lct->contexts[cnt]=ctx;
      cnt++;
    }
    dbEntry=GWEN_DB_GetNextGroup(dbEntry);
  }

  lct->contextListIsValid=1;

  GWEN_DB_Group_free(dbNotePad);
  return 0;
}





