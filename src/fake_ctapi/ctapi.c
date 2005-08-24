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


#include "ctapi_p.h"

#include <gwenhywfar/debug.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/tlv.h>
#include <stdlib.h>


GWEN_LIST_FUNCTIONS(CTAPI_CONTEXT, CTAPI_Context)




static CTAPI_CONTEXT_LIST *lc_ctapi_contexts=0;
static LC_CLIENT *lc_ctapi_client=0;
static int lc_ctapi_initcount=0;


CTAPI_CONTEXT *CTAPI_Context_new(unsigned short ctn, unsigned short port){
  CTAPI_CONTEXT *ctx;

  GWEN_NEW_OBJECT(CTAPI_CONTEXT, ctx);
  GWEN_LIST_INIT(CTAPI_CONTEXT, ctx);
  ctx->ctn=ctn;
  ctx->port=port;

  return ctx;
}



void CTAPI_Context_free(CTAPI_CONTEXT *ctx){
  if (ctx) {
    LC_Card_free(ctx->card);
    GWEN_LIST_INIT(CTAPI_CONTEXT, ctx);
    GWEN_FREE_OBJECT(ctx);
  }
}



void CTAPI_Context_SetCardType(CTAPI_CONTEXT *ctx, const char *ct){
  assert(ctx);
  free(ctx->cardType);
  if (ct) ctx->cardType=strdup(ct);
  else ctx->cardType=0;
}



CTAPI_CONTEXT *CTAPI_Context_FindByCtn(unsigned short ctn){
  CTAPI_CONTEXT *ctx;

  if (lc_ctapi_contexts==0)
    return 0;
  ctx=CTAPI_Context_List_First(lc_ctapi_contexts);
  while(ctx) {
    if (ctx->ctn==ctn)
      break;
    ctx=CTAPI_Context_List_Next(ctx);
  } /* while */

  return ctx;
}



CTAPI_CONTEXT *CTAPI_Context_FindByPort(unsigned short port){
  CTAPI_CONTEXT *ctx;

  if (lc_ctapi_contexts==0)
    return 0;
  ctx=CTAPI_Context_List_First(lc_ctapi_contexts);
  while(ctx) {
    if (ctx->port==port)
      break;
    ctx=CTAPI_Context_List_Next(ctx);
  } /* while */

  return ctx;
}



CTAPI_APDU *CTAPI_APDU_new(unsigned char *cmd, int len){
  CTAPI_APDU *apdu;

  if (len<4) {
    DBG_ERROR(CT_API_LOGDOMAIN, "Invalid APDU, too few bytes");
    return 0;
  }
  GWEN_NEW_OBJECT(CTAPI_APDU, apdu);
  apdu->cla=*(cmd++); len--;
  apdu->ins=*(cmd++); len--;
  apdu->p1=*(cmd++); len--;
  apdu->p2=*(cmd++); len--;
  if (len) {
    if (len>1) {
      /* data follows */
      apdu->dlen=*(cmd++); len--;
      apdu->data=(unsigned char*)malloc(apdu->dlen);
      memmove(apdu->data, cmd, apdu->dlen);
      cmd+=apdu->dlen;
      len-=apdu->dlen;
    }
    if (len) {
      apdu->rlen=*(cmd++); len--;
      if (apdu->rlen==0)
        apdu->rlen=-1;
    }
    if (len) {
      /* still bytes ? */
      DBG_ERROR(CT_API_LOGDOMAIN, "Invalid APDU, too many bytes");
      CTAPI_APDU_free(apdu);
      return 0;
    }
  }

  return apdu;
}



void CTAPI_APDU_free(CTAPI_APDU *apdu){
  if (apdu) {
    free(apdu->data);
    GWEN_FREE_OBJECT(apdu);
  }
}



void CTAPI_APDU_Dump(CTAPI_APDU *apdu){
  fprintf(stderr, "APDU: CLA=%02x, INS=%02x, P1=%02x, P2=%02x\n",
          apdu->cla, apdu->ins, apdu->p1, apdu->p2);
  if (apdu->dlen) {
    GWEN_BUFFER *tbuf;

    tbuf=GWEN_Buffer_new(0, 256, 0, 1);
    fprintf(stderr, "Data (%d bytes):\n", apdu->dlen);
    GWEN_Text_DumpString2Buffer((const char*)apdu->data, apdu->dlen, tbuf, 2);
    GWEN_Buffer_free(tbuf);
  }
}




void CT__showError(LC_CARD *card,
                   LC_CLIENT_RESULT res,
                   const char *failedCommand) {
  const char *s;

  switch(res) {
  case LC_Client_ResultOk:
    s="Ok.";
    break;
  case LC_Client_ResultWait:
    s="Timeout.";
    break;
  case LC_Client_ResultIpcError:
    s="IPC error.";
    break;
  case LC_Client_ResultCmdError:
    s="Command error.";
    break;
  case LC_Client_ResultDataError:
    s="Data error.";
    break;
  case LC_Client_ResultAborted:
    s="Aborted.";
    break;
  case LC_Client_ResultInvalid:
    s="Invalid argument to command.";
    break;
  case LC_Client_ResultInternal:
    s="Internal error.";
    break;
  case LC_Client_ResultGeneric:
    s="Generic error.";
    break;
  default:
    s="Unknown error.";
    break;
  }

  DBG_ERROR(CT_API_LOGDOMAIN, "Error in \"%s\": %s\n", failedCommand, s);
  if (res==LC_Client_ResultCmdError && card) {
    int sw1;
    int sw2;

    sw1=LC_Card_GetLastSW1(card);
    sw2=LC_Card_GetLastSW2(card);
    DBG_ERROR(CT_API_LOGDOMAIN, "  Last card command result:\n");
    if (sw1!=-1 && sw2!=-1) {
      DBG_ERROR(CT_API_LOGDOMAIN, "   SW1=%02x, SW2=%02x\n", sw1, sw2);
    }
    s=LC_Card_GetLastResult(card);
    if (s) {
      DBG_ERROR(CT_API_LOGDOMAIN, "   Result: %s\n", s);
    }
    s=LC_Card_GetLastText(card);
    if (s) {
      DBG_ERROR(CT_API_LOGDOMAIN, "   Text  : %s\n", s);
    }
  }
}




char CT_init(unsigned short ctn, unsigned short pn){
  CTAPI_CONTEXT *ctx;

  /* first find by ctn */
  ctx=CTAPI_Context_FindByCtn(ctn);
  if (ctx) {
    DBG_ERROR(CT_API_LOGDOMAIN, "Ctn %d already in use", ctn);
    return CT_API_RV_ERR_INVALID;
  }

  /* then find by port */
  ctx=CTAPI_Context_FindByPort(pn);
  if (ctx) {
    DBG_ERROR(CT_API_LOGDOMAIN, "Port %d already in use", pn);
    return CT_API_RV_ERR_INVALID;
  }

  if (lc_ctapi_initcount==0) {
    int rv;
    LC_CLIENT_RESULT res;
    const char *s;
    GWEN_LOGGER_LEVEL lv;

    /* get and set log level */
    s=getenv("LC_CTAPI_LOGLEVEL");
    if (!s)
      s="critical";
    lv=GWEN_Logger_Name2Level(s);
    GWEN_Logger_SetLevel(CT_API_LOGDOMAIN, lv);

    /* init libchipcard2-client */
    lc_ctapi_client=LC_Client_new("fake-ctapi", "1.0", 0);
    /* read default file */
    rv=LC_Client_ReadConfigFile(lc_ctapi_client, 0);
    if (rv) {
      DBG_INFO(CT_API_LOGDOMAIN, "here");
      LC_Client_free(lc_ctapi_client);
      lc_ctapi_client=0;
      return CT_API_RV_ERR_HOST;
    }
    res=LC_Client_StartWait(lc_ctapi_client, 0, 0);
    if (res!=LC_Client_ResultOk) {
      CT__showError(0, res, "StartWait");
      DBG_INFO(CT_API_LOGDOMAIN, "here");
      LC_Client_free(lc_ctapi_client);
      lc_ctapi_client=0;
      return CT_API_RV_ERR_HOST;
    }
    lc_ctapi_contexts=CTAPI_Context_List_new();
  }
  ctx=CTAPI_Context_new(ctn, pn);
  CTAPI_Context_List_Add(ctx, lc_ctapi_contexts);
  lc_ctapi_initcount++;

  return CT_API_RV_OK;
}



char CT_data(unsigned short ctn,
             unsigned char *dad,
             unsigned char *sad,
             unsigned short lenc,
             unsigned char *command,
             unsigned short *lenr,
             unsigned char *response){
  CTAPI_CONTEXT *ctx;
  CTAPI_APDU *apdu;
  int handled=0;
  char result;

  if (lc_ctapi_initcount<1) {
    DBG_ERROR(CT_API_LOGDOMAIN, "You MUST call CT_open before CT_data");
    return CT_API_RV_ERR_INVALID;
  }

  ctx=CTAPI_Context_FindByCtn(ctn);
  if (!ctx) {
    DBG_ERROR(CT_API_LOGDOMAIN, "Ctn %d not found", ctn);
    return CT_API_RV_ERR_INVALID;
  }

  /* generic parameter checks */
  if (!dad || !sad || !command || !lenr || !response) {
    DBG_ERROR(CT_API_LOGDOMAIN,
              "Null pointer given");
    return CT_API_RV_ERR_INVALID;
  }

  if (*lenr<2) {
    DBG_ERROR(CT_API_LOGDOMAIN,
              "Response buffer too small (%d, need at least 2 bytes)",
              *lenr);
    return CT_API_RV_ERR_INVALID;
  }

  if (lenc<4) {
    DBG_ERROR(CT_API_LOGDOMAIN,
              "Too few bytes in APDU (%d, need at least 4 bytes)", lenc);
    return CT_API_RV_ERR_INVALID;
  }

  DBG_ERROR(CT_API_LOGDOMAIN, "Sending APDU:");
  GWEN_Text_LogString((const char*)command, lenc,
                      CT_API_LOGDOMAIN, GWEN_LoggerLevelError);
  apdu=CTAPI_APDU_new(command, lenc);
  if (!apdu) {
    DBG_ERROR(CT_API_LOGDOMAIN,
              "Invalid APDU");
    return CT_API_RV_ERR_INVALID;
  }
  result=CT_API_RV_OK;
  if (command[0]==0x20) {
    /* terminal commands */
    switch(command[1]) {
    case 0x11: /* RESET ICC */
      result=CT__resetICC(ctx, dad, sad, apdu, lenr, response);
      handled=1;
      break;
    case 0x12: /* REQUEST ICC */
      result=CT__requestICC(ctx, dad, sad, apdu, lenr, response);
      handled=1;
      break;
    case 0x13: /* GET STATUS */
      result=CT__getStatusICC(ctx, dad, sad, apdu, lenr, response);
      handled=1;
      break;
    case 0x14: /* DEACTIVATE ICC */
      result=CT__ejectICC(ctx, dad, sad, apdu, lenr, response);
      handled=1;
      break;
    case 0x15: /* EJECT ICC */
      result=CT__ejectICC(ctx, dad, sad, apdu, lenr, response);
      handled=1;
      break;
    case 0x18: /* Perform Verification */
      result=CT__secureVerify(ctx, dad, sad, apdu, lenr, response);
      handled=1;
      break;
    case 0x19: /* Modify Verification Data */
      result=CT__secureModify(ctx, dad, sad, apdu, lenr, response);
      handled=1;
      break;
    default:
      break;
    }
  }

  if (!handled) {
    /* normal APDU, send it to the card if we have any */
    if (ctx->isOpen && ctx->card) {
      LC_CLIENT_RESULT res;
      GWEN_BUFFER *rbuf;
      int i;

      rbuf=GWEN_Buffer_new(0, 300, 0, 1);
      res=LC_Card_ExecAPDU(ctx->card,
                           command, lenc,
                           rbuf,
                           (*dad==CT)?LC_Client_CmdTargetReader:
                           LC_Client_CmdTargetCard,
                           60);
      i=GWEN_Buffer_GetUsedBytes(rbuf);
      if (i) {
        if (i>*lenr) {
          DBG_ERROR(CT_API_LOGDOMAIN, "Buffer too small");
          result=CT_API_RV_ERR_MEMORY;
        }
        else {
          DBG_INFO(CT_API_LOGDOMAIN, "Response received: ");
          if (GWEN_Logger_GetLevel(CT_API_LOGDOMAIN)>=GWEN_LoggerLevelInfo)
            GWEN_Buffer_Dump(rbuf, stderr, 4);
          *sad=*dad;
          memmove(response, GWEN_Buffer_GetStart(rbuf), i);
          *lenr=i;
          result=CT_API_RV_OK;
        }
      }
      else {
        if (res!=LC_Client_ResultOk) {
          CT__showError(ctx->card, res, "LC_Card_ExecAPDU");
          result=CT_API_RV_ERR_HOST;
        }
        else {
          /* nothing returned */
          DBG_ERROR(CT_API_LOGDOMAIN, "Nothing returned");
          result=CT_API_RV_ERR_HOST;
        }
      } /* if buffer filled */
      GWEN_Buffer_free(rbuf);
    } /* if open */
    else {
      *sad=CT_API_AD_CT;
      response[0]=0x62; /* no card */
      response[1]=0x00;
      *lenr=2;
      result=CT_API_RV_OK;
    }
  }

  CTAPI_APDU_free(apdu);
  DBG_ERROR(CT_API_LOGDOMAIN, "CTAPI-Result: %d", result);
  if (result==CT_API_RV_OK) {
    GWEN_Text_LogString((const char*)response, *lenr,
                        CT_API_LOGDOMAIN, GWEN_LoggerLevelError);
  }

  return result;
}



char CT_close(unsigned short ctn){
  CTAPI_CONTEXT *ctx;
  char rv=CT_API_RV_OK;

  if (lc_ctapi_initcount<1) {
    DBG_ERROR(CT_API_LOGDOMAIN, "You MUST call CT_open before CT_close");
    return CT_API_RV_ERR_INVALID;
  }

  ctx=CTAPI_Context_FindByCtn(ctn);
  if (!ctx) {
    DBG_ERROR(CT_API_LOGDOMAIN, "Ctn not found");
    return CT_API_RV_ERR_INVALID;
  }

  if (ctx->isOpen && ctx->card) {
    LC_CLIENT_RESULT res;

    res=LC_Card_Close(ctx->card);
    if (res) {
      CT__showError(ctx->card, res, "LC_Card_Close");
      rv=CT_API_RV_ERR_HOST;
    }
  }
  CTAPI_Context_List_Del(ctx);
  CTAPI_Context_free(ctx);
  lc_ctapi_initcount--;

  if (lc_ctapi_initcount==0) {
    CTAPI_Context_List_free(lc_ctapi_contexts);
    lc_ctapi_contexts=0;
    LC_Client_free(lc_ctapi_client);
    lc_ctapi_client=0;
  }

  return rv;
}


LC_CLIENT_RESULT CT__openCard(CTAPI_CONTEXT *ctx, int timeout) {

  if (!ctx->card) {
    /* no card, wait for one */
    /* TODO: check for TLV 0x80 (waiting time in seconds) */
    ctx->card=LC_Client_WaitForNextCard(lc_ctapi_client, timeout);
    if (!ctx->card) {
      DBG_ERROR(CT_API_LOGDOMAIN, "No card");
      return LC_Client_ResultWait;
    }
  }

  if (!ctx->isOpen) {
    LC_CLIENT_RESULT res;
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
    if (GWEN_StringList_HasString(sl, "rsacard")) {
      res=LC_Card_SelectCardAndApp(ctx->card, "rsacard", "rsacard");
      if (res!=LC_Client_ResultOk) {
        DBG_INFO(LC_LOGDOMAIN, "here");
        return res;
      }
      CTAPI_Context_SetCardType(ctx, "rsacard");
    }
    else if (GWEN_StringList_HasString(sl, "ddv1")) {
      res=LC_Card_SelectCardAndApp(ctx->card, "ddv1", "ddv");
      if (res!=LC_Client_ResultOk) {
        DBG_INFO(LC_LOGDOMAIN, "here");
        return res;
      }
      CTAPI_Context_SetCardType(ctx, "ddv1");
    }
    else if (GWEN_StringList_HasString(sl, "ddv0")) {
      res=LC_Card_SelectCardAndApp(ctx->card, "ddv0", "ddv");
      if (res!=LC_Client_ResultOk) {
        DBG_INFO(LC_LOGDOMAIN, "here");
        return res;
      }
      CTAPI_Context_SetCardType(ctx, "ddv0");
    }
    else if (GWEN_StringList_HasString(sl, "geldkarte")) {
      res=LC_Card_SelectCardAndApp(ctx->card, "geldkarte", "geldkarte");
      if (res!=LC_Client_ResultOk) {
        DBG_INFO(LC_LOGDOMAIN, "here");
        return res;
      }
      CTAPI_Context_SetCardType(ctx, "geldkarte");
      /* add other types here */
    }
    else {
      if ((strcasecmp(LC_Card_GetCardType(ctx->card), "processor")==0) &&
          GWEN_StringList_HasString(sl, "processorCard")) {
        res=LC_Card_SelectCardAndApp(ctx->card, "ProcessorCard", "ProcessorCard");
        if (res!=LC_Client_ResultOk) {
          DBG_INFO(LC_LOGDOMAIN, "here");
          return res;
        }
        CTAPI_Context_SetCardType(ctx, 0);
      }
      else if ((strcasecmp(LC_Card_GetCardType(ctx->card), "memory")==0) &&
               GWEN_StringList_HasString(sl, "MemoryCard")) {
        res=LC_Card_SelectCardAndApp(ctx->card, "MemoryCard", "MemoryCard");
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



char CT__requestICC(CTAPI_CONTEXT *ctx,
                    unsigned char *dad,
                    unsigned char *sad,
                    CTAPI_APDU *apdu,
                    unsigned short *lenr,
                    unsigned char *response){
  GWEN_BUFFER *atr;
  unsigned char *p;
  unsigned char *t;
  int i;
  int j=0;
  LC_CLIENT_RESULT res;

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
    /* return full ATR */
    atr=LC_Card_GetAtr(ctx->card);
    if (atr) {
      i=apdu->rlen;
      if (i==-1)
        i=GWEN_Buffer_GetUsedBytes(atr);
      if (i>(int)GWEN_Buffer_GetUsedBytes(atr))
        i=GWEN_Buffer_GetUsedBytes(atr);
      if (i>(*lenr)-2) {
        return CT_API_RV_ERR_INVALID;
      }
      t=(unsigned char*)GWEN_Buffer_GetStart(atr);
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
  else if (res==LC_Client_ResultOk) {
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

  DBG_ERROR(CT_API_LOGDOMAIN, "EJECT ICC");

  if (ctx->isOpen && ctx->card) {
    LC_CLIENT_RESULT res;

    res=LC_Card_Close(ctx->card);
    if (res!=LC_Client_ResultOk) {
      CT__showError(ctx->card, res, "CardClose");
      LC_Card_free(ctx->card);
      ctx->card=0;
      ctx->isOpen=0;
      return CT_API_RV_ERR_CT;
    }
    LC_Card_free(ctx->card);
    ctx->card=0;
    ctx->isOpen=0;
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
    GWEN_BUFFER *atr;
    unsigned char *t;
    int i;

    /* return full ATR */
    atr=LC_Card_GetAtr(ctx->card);
    if (atr) {
      i=apdu->rlen;
      if (i==-1)
        i=GWEN_Buffer_GetUsedBytes(atr);
      if (i>(int)GWEN_Buffer_GetUsedBytes(atr))
        i=GWEN_Buffer_GetUsedBytes(atr);
      if (i>(*lenr)-2) {
        return CT_API_RV_ERR_INVALID;
      }
      t=(unsigned char*)GWEN_Buffer_GetStart(atr);
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
  *p=0x90;
  j++;
  if (strcasecmp(LC_Card_GetCardType(ctx->card), "processor")==0)
    *p=0x01;
  else
    *p=0x00;
  j++;
  *lenr=j;

  *dad=*sad;
  *sad=CT_API_AD_CT;
  return CT_API_RV_OK;
}



int CT__getPinId(CTAPI_APDU *apdu) {
  GWEN_BUFFER *dbuf;
  GWEN_TLV *tlv=0;
  const unsigned char *p;
  int i;

  if (apdu->dlen<8) {
    DBG_ERROR(CT_API_LOGDOMAIN,
              "Bad APDU: Too few data bytes in APDU (only %d bytes)",
              apdu->dlen);
    return -1;
  }

  dbuf=GWEN_Buffer_new(0, 256, 0, 1);
  GWEN_Buffer_AppendBytes(dbuf, apdu->data, apdu->dlen);
  GWEN_Buffer_Rewind(dbuf);

  /* find tag 0x52 */
  while(GWEN_Buffer_GetBytesLeft(dbuf)) {
    tlv=GWEN_TLV_fromBuffer(dbuf, 0);
    if (!tlv)
      break;
    if (GWEN_TLV_GetTagType(tlv)==0x52)
      break;
  } /* while */

  if (!tlv) {
    DBG_ERROR(CT_API_LOGDOMAIN, "TLV 0x52 not found");
    return -1;
  }

  if (GWEN_TLV_GetTagLength(tlv)<6) {
    DBG_ERROR(CT_API_LOGDOMAIN, "TLV 0x52 too small");
    GWEN_TLV_free(tlv);
    return -1;
  }
  p=(const unsigned char*)GWEN_TLV_GetTagData(tlv);

  i=(int)p[5];
  GWEN_TLV_free(tlv);
  return i;
}



char CT__secureVerify(CTAPI_CONTEXT *ctx,
                      unsigned char *dad,
                      unsigned char *sad,
                      CTAPI_APDU *apdu,
                      unsigned short *lenr,
                      unsigned char *response){
  LC_CLIENT_RESULT res;
  unsigned char *p;
  const unsigned char *t;
  unsigned int bs;
  int i;
  int j=0;
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  unsigned char uc;
  unsigned char sw1, sw2;

  DBG_ERROR(CT_API_LOGDOMAIN, "SecureVerify");

  uc=*sad;
  *sad=*dad;
  *dad=uc;

  if (ctx->cardType==0 ||
      !(LC_Card_GetReaderFlags(ctx->card) & LC_CARD_READERFLAGS_KEYPAD)) {
    DBG_ERROR(CT_API_LOGDOMAIN,
              "SecureVerify not available");
    response[0]=0x6d; /* not supported */
    response[1]=0x00;
    *lenr=2;
    return CT_API_RV_OK;
  }

  i=CT__getPinId(apdu);
  if (i<0)
    return CT_API_RV_ERR_INVALID;

  dbReq=GWEN_DB_Group_new("SecureVerifyPin");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS, "pid", i);
  res=LC_Card_ExecCommand(ctx->card, dbReq, dbResp,
                          LC_Client_GetShortTimeout(lc_ctapi_client));
  GWEN_DB_Group_free(dbReq);
  p=response;
  t=GWEN_DB_GetBinValue(dbResp,
                        "command/response/data",
                        0,
                        0, 0,
                        &bs);
  if (t && bs) {
    i=apdu->rlen;
    if (i==-1)
      i=bs;
    if (i>(int)bs)
      i=bs;
    if (i>(*lenr)-2) {
      GWEN_DB_Group_free(dbResp);
      DBG_ERROR(CT_API_LOGDOMAIN, "Buffer too small");
      return CT_API_RV_ERR_INVALID;
    }
    while(i--) {
      *(p++)=*(t++);
      j++;
    }
  }
  GWEN_DB_Group_free(dbResp);

  switch(res) {
  case LC_Client_ResultOk:
    sw1=0x90;
    sw2=0x00;
    break;
  case LC_Client_ResultWait:
    sw1=0x64;
    sw2=0x00;
    break;
  case LC_Client_ResultIpcError:
    GWEN_DB_Group_free(dbResp);
    DBG_ERROR(CT_API_LOGDOMAIN, "IPC error");
    return CT_API_RV_ERR_HOST;
  case LC_Client_ResultCmdError:
    sw1=LC_Card_GetLastSW1(ctx->card);
    sw2=LC_Card_GetLastSW2(ctx->card);
    break;
  case LC_Client_ResultAborted:
    sw1=0x64;
    sw2=0x01;
    break;
  default:
    GWEN_DB_Group_free(dbResp);
    DBG_ERROR(CT_API_LOGDOMAIN, "CT error");
    return CT_API_RV_ERR_CT;
  }
  *dad=*sad;
  *sad=CT_API_AD_CT;
  *(p++)=sw1;
  j++;
  *(p++)=sw2;
  j++;
  *lenr=j;
  return CT_API_RV_OK;
}



char CT__secureModify(CTAPI_CONTEXT *ctx,
                      unsigned char *dad,
                      unsigned char *sad,
                      CTAPI_APDU *apdu,
                      unsigned short *lenr,
                      unsigned char *response){
  LC_CLIENT_RESULT res;
  unsigned char *p;
  const unsigned char *t;
  unsigned int bs;
  int i;
  int j=0;
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbResp;
  unsigned char uc;
  unsigned char sw1, sw2;

  DBG_ERROR(CT_API_LOGDOMAIN, "SecureVerify");

  uc=*sad;
  *sad=*dad;
  *dad=uc;

  if (ctx->cardType==0 ||
      !(LC_Card_GetReaderFlags(ctx->card) & LC_CARD_READERFLAGS_KEYPAD)) {
    DBG_ERROR(CT_API_LOGDOMAIN,
              "SecureModify not available");
    response[0]=0x6d; /* not supported */
    response[1]=0x00;
    *lenr=2;
    return CT_API_RV_OK;
  }

  i=CT__getPinId(apdu);
  if (i<0)
    return CT_API_RV_ERR_INVALID;

  dbReq=GWEN_DB_Group_new("SecureModifyPin");
  dbResp=GWEN_DB_Group_new("response");
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS, "pid", i);
  res=LC_Card_ExecCommand(ctx->card, dbReq, dbResp,
                          LC_Client_GetShortTimeout(lc_ctapi_client));
  GWEN_DB_Group_free(dbReq);
  p=response;
  t=GWEN_DB_GetBinValue(dbResp,
                        "command/response/data",
                        0,
                        0, 0,
                        &bs);
  if (t && bs) {
    i=apdu->rlen;
    if (i==-1)
      i=bs;
    if (i>(int)bs)
      i=bs;
    if (i>(*lenr)-2) {
      GWEN_DB_Group_free(dbResp);
      DBG_ERROR(CT_API_LOGDOMAIN, "Buffer too small");
      return CT_API_RV_ERR_INVALID;
    }
    while(i--) {
      *(p++)=*(t++);
      j++;
    }
  }
  GWEN_DB_Group_free(dbResp);

  switch(res) {
  case LC_Client_ResultOk:
    sw1=0x90;
    sw2=0x00;
    break;
  case LC_Client_ResultWait:
    sw1=0x64;
    sw2=0x00;
    break;
  case LC_Client_ResultIpcError:
    GWEN_DB_Group_free(dbResp);
    DBG_ERROR(CT_API_LOGDOMAIN, "IPC error");
    return CT_API_RV_ERR_HOST;
  case LC_Client_ResultCmdError:
    sw1=LC_Card_GetLastSW1(ctx->card);
    sw2=LC_Card_GetLastSW2(ctx->card);
    break;
  case LC_Client_ResultAborted:
    sw1=0x64;
    sw2=0x01;
    break;
  default:
    GWEN_DB_Group_free(dbResp);
    DBG_ERROR(CT_API_LOGDOMAIN, "CT error");
    return CT_API_RV_ERR_CT;
  }
  *dad=*sad;
  *sad=CT_API_AD_CT;
  *(p++)=sw1;
  j++;
  *(p++)=sw2;
  j++;
  *lenr=j;
  return CT_API_RV_OK;
}







