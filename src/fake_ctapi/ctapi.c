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
  if (res==LC_Client_ResultCmdError) {
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

    /* init libchipcard2-client */
    lc_ctapi_client=LC_Client_new("fake-ctapi", "1.0", 0);
    /* read default file */
    rv=LC_Client_ReadConfigFile(lc_ctapi_client, 0);
    if (rv) {
      DBG_INFO(CT_API_LOGDOMAIN, "here");
      LC_Client_free(lc_ctapi_client);
      lc_ctapi_client=0;
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
  int handled=0;

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

  if (command[0]==0x20) {
    /* terminal commands */
    switch(command[1]) {
    case 0x11: /* RESET ICC */
      handled=1;
      break;
    case 0x12: /* REQUEST ICC */
      handled=1;
      break;
    case 0x13: /* GET STATUS */
      handled=1;
      break;
    case 0x15: /* EJECT ICC */
      handled=1;
      break;
    default:
      break;
    }
  }

  if (!handled) {
    /* normal APDU, send it to the card if we have any */
  }

  return CT_API_RV_OK;
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








