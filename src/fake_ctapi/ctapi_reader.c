/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: ctapi.c 186 2006-06-07 13:11:19Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/



LC_CLIENT_RESULT CT__openCard(CTAPI_CONTEXT *ctx, int timeout) {
  LC_CLIENT_RESULT res;

  if (!ctx->card) {
    /* no card, wait for one */
    /* TODO: check for TLV 0x80 (waiting time in seconds) */
    res=LC_Client_GetNextCard(lc_ctapi_client,
                              &(ctx->card),
                              timeout);
    if (!ctx->card) {
      DBG_ERROR(CT_API_LOGDOMAIN, "No card");
      return LC_Client_ResultWait;
    }
  }

  DBG_ERROR(0, "Got this card:");
  LC_Card_Dump(ctx->card, stderr, 2);

  if (!ctx->isOpen) {
    const GWEN_STRINGLIST *sl;

    res=LC_Card_Open(ctx->card);
    if (res!=LC_Client_ResultOk) {
      CT__showError(ctx->card, res, "CardOpen");
      return res;
    }
    DBG_ERROR(CT_API_LOGDOMAIN, "Card is open");
    ctx->isOpen=1;

    sl=LC_Card_GetCardTypes(ctx->card);
    assert(sl);
    if (GWEN_StringList_HasString(sl, "starcos")) {
      res=LC_Card_SelectCard(ctx->card, "starcos");
      if (res!=LC_Client_ResultOk) {
        DBG_INFO(LC_LOGDOMAIN, "here");
        return res;
      }
      CTAPI_Context_SetCardType(ctx, "starcos");
    }
    else if (GWEN_StringList_HasString(sl, "ddv1")) {
      res=LC_Card_SelectCard(ctx->card, "ddv1");
      if (res!=LC_Client_ResultOk) {
        DBG_INFO(LC_LOGDOMAIN, "here");
        return res;
      }
      CTAPI_Context_SetCardType(ctx, "ddv1");
    }
    else if (GWEN_StringList_HasString(sl, "ddv0")) {
      res=LC_Card_SelectCard(ctx->card, "ddv0");
      if (res!=LC_Client_ResultOk) {
        DBG_INFO(LC_LOGDOMAIN, "here");
        return res;
      }
      CTAPI_Context_SetCardType(ctx, "ddv0");
    }
    else if (GWEN_StringList_HasString(sl, "geldkarte")) {
      res=LC_Card_SelectCard(ctx->card, "geldkarte");
      if (res!=LC_Client_ResultOk) {
        DBG_INFO(LC_LOGDOMAIN, "here");
        return res;
      }
      CTAPI_Context_SetCardType(ctx, "geldkarte");
      /* add other types here */
    }
    else {
      /* no special card found, select basic card type */
      if ((strcasecmp(LC_Card_GetCardType(ctx->card), "processor")==0) &&
          GWEN_StringList_HasString(sl, "processorCard")) {
        res=LC_Card_SelectCard(ctx->card, "ProcessorCard");
        if (res!=LC_Client_ResultOk) {
          DBG_INFO(LC_LOGDOMAIN, "here");
          return res;
        }
        CTAPI_Context_SetCardType(ctx, 0);
      }
      else if ((strcasecmp(LC_Card_GetCardType(ctx->card), "memory")==0) &&
               GWEN_StringList_HasString(sl, "MemoryCard")) {
        res=LC_Card_SelectCard(ctx->card, "MemoryCard");
        if (res!=LC_Client_ResultOk) {
          DBG_INFO(LC_LOGDOMAIN, "here");
          return res;
        }
        CTAPI_Context_SetCardType(ctx, 0);
      }
    }
  }
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT CT__closeCard(CTAPI_CONTEXT *ctx) {
  if (ctx->isOpen && ctx->card) {
    LC_CLIENT_RESULT res;

    /* close card */
    res=LC_Card_Close(ctx->card);
    if (res!=LC_Client_ResultOk) {
      CT__showError(ctx->card, res, "CardClose");
      LC_Client_ReleaseCard(lc_ctapi_client, ctx->card);
      LC_Card_free(ctx->card);
      ctx->card=0;
      ctx->isOpen=0;
      return res;
    }

    /* release card */
    res=LC_Client_ReleaseCard(lc_ctapi_client, ctx->card);
    if (res!=LC_Client_ResultOk) {
      CT__showError(ctx->card, res, "CardClose");
      LC_Card_free(ctx->card);
      ctx->card=0;
      ctx->isOpen=0;
      return res;
    }

    LC_Card_free(ctx->card);
    ctx->card=0;
    ctx->isOpen=0;
  }
  return LC_Client_ResultOk;
}






char CT__requestICC(CTAPI_CONTEXT *ctx,
                    unsigned char *dad,
                    unsigned char *sad,
                    CTAPI_APDU *apdu,
                    unsigned short *lenr,
                    unsigned char *response){
  unsigned char *p;
  const unsigned char *t;
  int i;
  int j=0;
  LC_CLIENT_RESULT res;
  const unsigned char *atr;

  DBG_ERROR(CT_API_LOGDOMAIN, "REQUEST ICC");

  if (apdu->p1!=0x00 && apdu->p1!=0x01) {
      DBG_ERROR(CT_API_LOGDOMAIN,
                "Only CT or one slot supported (%d)", apdu->p1);
    return CT_API_RV_ERR_CT;
  }

  res=CT__openCard(ctx, 30);
  if (res==LC_Client_ResultWait) {
    DBG_ERROR(CT_API_LOGDOMAIN, "No card");
    *dad=*sad;
    *sad=CT_API_AD_CT;
    response[0]=0x62; /* no card */
    response[1]=0x00;
    *lenr=2;
    return CT_API_RV_OK;
  }
  else if (res!=LC_Client_ResultOk) {
    return CT_API_RV_ERR_CT;
  }

  *sad=*dad;
  p=response;

  if ((apdu->p2 & 0xf)==0x00) {
    /* no response */
  }
  else if ((apdu->p2 & 0xf)==0x01) {
    unsigned int atrLen;

    /* return full ATR */
    atrLen=LC_Card_GetAtr(ctx->card, &atr);
    if (atrLen && atr) {
      i=apdu->rlen;
      if (i==-1)
        i=atrLen;
      if (i>(int)atrLen)
        i=atrLen;
      if (i>(*lenr)-2) {
        return CT_API_RV_ERR_INVALID;
      }
      t=atr;
      while(i--) {
        *(p++)=*(t++);
        j++;
      }
    }
  }
  else if ((apdu->p2 & 0xf)==0x02) {
    /* only return historic bytes */
    /* TODO */
  }
  *(p++)=0x90;
  j++;
  if (strcasecmp(LC_Card_GetCardType(ctx->card), "processor")==0)
    *(p++)=0x01;
  else
    *(p++)=0x00;
  j++;
  *lenr=j;
  return CT_API_RV_OK;
}



char CT__getStatusICC(CTAPI_CONTEXT *ctx,
                      unsigned char *dad,
                      unsigned char *sad,
                      CTAPI_APDU *apdu,
                      unsigned short *lenr,
                      unsigned char *response){
  LC_CLIENT_RESULT res;

  DBG_ERROR(CT_API_LOGDOMAIN, "GET STATUS ICC");

  if (apdu->p1!=0x00) {
    DBG_ERROR(CT_API_LOGDOMAIN, "Bad P1 (%d)", apdu->p1);
    return CT_API_RV_ERR_INVALID;
  }

  res=CT__openCard(ctx, 5);
  if (res==LC_Client_ResultWait) {
    DBG_ERROR(CT_API_LOGDOMAIN, "No card");
    *dad=*sad;
    *sad=CT_API_AD_CT;
    response[0]=0x62; /* no card */
    response[1]=0x00;
    *lenr=2;
    return CT_API_RV_OK;
  }
  else if (res!=LC_Client_ResultOk) {
    return CT_API_RV_ERR_CT;
  }

  *sad=*dad;

  switch(apdu->p2) {
  case 0x80:
    /* reader status */
    if (*lenr<5) {
      DBG_ERROR(CT_API_LOGDOMAIN, "Response buffer too small");
      return CT_API_RV_ERR_MEMORY;
    }

    response[0]=0x80;
    response[1]=0x01;
    response[2]=0x00;
    response[3]=0x90;
    response[4]=0x00;
    *lenr=5;

    if (ctx->card) {
      response[2]|=1;
      if (ctx->isOpen) {
        response[2]|=0x4;
      }
    }
    return CT_API_RV_OK;

  case 0x46:
    /* manufacturer info */
    /* TODO */
    DBG_ERROR(CT_API_LOGDOMAIN, "Manufacturer info not yet supported");
    break;

  default:
    break;
  }

  return CT_API_RV_ERR_INVALID;
}



char CT__ejectICC(CTAPI_CONTEXT *ctx,
                  unsigned char *dad,
                  unsigned char *sad,
                  CTAPI_APDU *apdu,
                  unsigned short *lenr,
                  unsigned char *response){
  LC_CLIENT_RESULT res;

  DBG_ERROR(CT_API_LOGDOMAIN, "EJECT ICC");
  res=CT__closeCard(ctx);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(CT_API_LOGDOMAIN, "here (%d)", res);
    return CT_API_RV_ERR_CT;
  }

  *sad=CT_API_AD_CT;
  response[0]=0x90;
  response[1]=0x00;
  *lenr=2;
  return CT_API_RV_OK;
}



char CT__resetICC(CTAPI_CONTEXT *ctx,
                  unsigned char *dad,
                  unsigned char *sad,
                  CTAPI_APDU *apdu,
                  unsigned short *lenr,
                  unsigned char *response){
  LC_CLIENT_RESULT res;
  unsigned char *p;
  int j=0;

  DBG_ERROR(CT_API_LOGDOMAIN, "RESET ICC");

  if (apdu->p1==0x00) {
    /* reset CT */
    *sad=CT_API_AD_CT;
    response[0]=0x90;
    response[1]=0x00;
    *lenr=2;
    return CT_API_RV_OK;
  }
  else if (apdu->p1!=0x01) {
    DBG_ERROR(CT_API_LOGDOMAIN, "Only one slot supported (%d)", apdu->p1);
    return CT_API_RV_ERR_CT;
  }

  res=CT__openCard(ctx, 30);
  if (res==LC_Client_ResultWait) {
    DBG_ERROR(CT_API_LOGDOMAIN, "No card");
    *dad=*sad;
    *sad=CT_API_AD_CT;
    response[0]=0x62; /* no card */
    response[1]=0x00;
    *lenr=2;
    return CT_API_RV_OK;
  }
  else if (res!=LC_Client_ResultOk) {
    return CT_API_RV_ERR_CT;
  }

  p=response;

  if (apdu->p2==0x00) {
    /* no response */
  }
  else if (apdu->p2==0x01) {
    unsigned int atrLen;
    const unsigned char *atr;

    /* return full ATR */
    atrLen=LC_Card_GetAtr(ctx->card, &atr);
    if (atrLen && atr) {
      int i;
      const unsigned char *t;

      i=apdu->rlen;
      if (i==-1)
        i=atrLen;
      if (i>(int)atrLen)
        i=atrLen;
      if (i>(*lenr)-2) {
        return CT_API_RV_ERR_INVALID;
      }
      t=atr;
      while(i--) {
        *(p++)=*(t++);
        j++;
      }
    }
  }
  else if (apdu->p2==0x02) {
    /* only return historic bytes */
    /* TODO */
  }
  *(p++)=0x90;
  j++;
  if (strcasecmp(LC_Card_GetCardType(ctx->card), "processor")==0)
    *(p++)=0x01;
  else
    *(p++)=0x00;
  j++;
  *lenr=j;

  *dad=*sad;
  *sad=CT_API_AD_CT;
  return CT_API_RV_OK;
}







