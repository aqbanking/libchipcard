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


#include "ctapi_p.h"

#include <gwenhywfar/debug.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/tlv.h>
#include <stdlib.h>


GWEN_LIST_FUNCTIONS(CTAPI_CONTEXT, CTAPI_Context)




static CTAPI_CONTEXT_LIST *lc_ctapi_contexts=0;
static LC_CLIENT *lc_ctapi_client=0;
static int lc_ctapi_initcount=0;




void CT__dumpString2Buffer(const unsigned char *s,
                           unsigned int l,
                           GWEN_BUFFER *mbuf) {
  unsigned int i;
  unsigned int j;
  char numbuf[32];

  if (l>32)
    j=32;
  else
    j=l;

  /* show hex dump */
  for (i=0; i<j; i++) {
    snprintf(numbuf, sizeof(numbuf),"%02x ", (unsigned char)s[i]);
    GWEN_Buffer_AppendString(mbuf, numbuf);
  }

  if (j!=l)
    GWEN_Buffer_AppendString(mbuf, " [...]");

  GWEN_Buffer_AppendString(mbuf, " (");
  snprintf(numbuf, sizeof(numbuf), "%d", l);
  GWEN_Buffer_AppendString(mbuf, numbuf);
  GWEN_Buffer_AppendString(mbuf, " bytes)");
}





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
  case LC_Client_ResultIoError:
    s="IO error.";
    break;
  default:
    s="Unknown error.";
    break;
  }

  DBG_ERROR(CT_API_LOGDOMAIN, "Error in \"%s\": %s (%d)\n",
            failedCommand, s, res);
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






CHIPCARD_EXPORT char CT_init(unsigned short ctn, unsigned short pn){
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
    LC_CLIENT_RESULT res;

    if (!GWEN_Logger_Exists(CT_API_LOGDOMAIN)) {
      const char *s;
      GWEN_LOGGER_LEVEL lv;

      /* only set our logger if it not already has been */
      GWEN_Logger_Open(CT_API_LOGDOMAIN, "ctapi", 0,
		       GWEN_LoggerType_Console,
                       GWEN_LoggerFacility_User);
      GWEN_Logger_SetLevel(CT_API_LOGDOMAIN, GWEN_LoggerLevel_Warning);

      /* get and set log level */
      s=getenv("LC_CTAPI_LOGLEVEL");
      if (!s)
        s="critical";
      lv=GWEN_Logger_Name2Level(s);
      GWEN_Logger_SetLevel(CT_API_LOGDOMAIN, lv);
    }

    /* init libchipcard3-client */
    lc_ctapi_client=LC_Client_new("fake-ctapi", "1.0");
    /* init libchipcard3-client */
    res=LC_Client_Init(lc_ctapi_client);
    if (res!=LC_Client_ResultOk) {
      DBG_INFO(CT_API_LOGDOMAIN, "here (%d)", res);
      LC_Client_free(lc_ctapi_client);
      lc_ctapi_client=0;
      return CT_API_RV_ERR_HOST;
    }

    /* start working with chip cards */
    res=LC_Client_Start(lc_ctapi_client);
    if (res!=LC_Client_ResultOk) {
      CT__showError(0, res, "Start");
      DBG_INFO(CT_API_LOGDOMAIN, "here");
      LC_Client_free(lc_ctapi_client);
      lc_ctapi_client=0;
      return CT_API_RV_ERR_HOST;
    }

    /* create context lists */
    lc_ctapi_contexts=CTAPI_Context_List_new();
  }
  lc_ctapi_initcount++;

  /* create and add CTAPI context */
  ctx=CTAPI_Context_new(ctn, pn);
  CTAPI_Context_List_Add(ctx, lc_ctapi_contexts);

  return CT_API_RV_OK;
}



CHIPCARD_EXPORT char CT_data(unsigned short ctn,
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

  if (GWEN_Logger_GetLevel(CT_API_LOGDOMAIN)>=GWEN_LoggerLevel_Debug){
    GWEN_BUFFER *mbuf;

    mbuf=GWEN_Buffer_new(0, 256, 0, 1);
    CT__dumpString2Buffer(command, lenc, mbuf);
    DBG_DEBUG(CT_API_LOGDOMAIN, "APDU: %s", GWEN_Buffer_GetStart(mbuf));
    GWEN_Buffer_free(mbuf);
  }

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
      res=LC_Card_ExecApdu(ctx->card,
                           (const char*)command, lenc,
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
      /* IO error, remove card */
      if (res==LC_Client_ResultIoError ||
          res==LC_Client_ResultCardRemoved) {
        DBG_NOTICE(LC_LOGDOMAIN, "Card has been removed");
        CT__closeCard(ctx);
      }
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
  if (GWEN_Logger_GetLevel(CT_API_LOGDOMAIN)>=GWEN_LoggerLevel_Debug){
    if (result==CT_API_RV_OK) {
      GWEN_BUFFER *mbuf;

      mbuf=GWEN_Buffer_new(0, 256, 0, 1);
      CT__dumpString2Buffer(response, *lenr, mbuf);
      DBG_DEBUG(CT_API_LOGDOMAIN, "SW  : %s", GWEN_Buffer_GetStart(mbuf));
      GWEN_Buffer_free(mbuf);
    }
    else {
      DBG_DEBUG(CT_API_LOGDOMAIN, "RET : %d", result)
    }
  }

  return result;
}



CHIPCARD_EXPORT char CT_close(unsigned short ctn){
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
    LC_Client_ReleaseCard(lc_ctapi_client, ctx->card);
  }
  CTAPI_Context_List_Del(ctx);
  CTAPI_Context_free(ctx);
  lc_ctapi_initcount--;

  if (lc_ctapi_initcount==0) {
    CTAPI_Context_List_free(lc_ctapi_contexts);
    lc_ctapi_contexts=0;
    LC_Client_Fini(lc_ctapi_client);
    LC_Client_free(lc_ctapi_client);
    lc_ctapi_client=0;
    GWEN_Logger_Close(CT_API_LOGDOMAIN);
  }

  return rv;
}





#include "ctapi_reader.c"
#include "ctapi_secure.c"





