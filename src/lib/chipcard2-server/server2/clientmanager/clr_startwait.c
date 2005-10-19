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


#include "clientmanager_p.h"
#include "fullserver_l.h"
#include "cardmanager/cardmanager_l.h"
#include "connection_l.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>


int LCCL_ClientManager_HandleStartWait(LCCL_CLIENTMANAGER *clm,
                                       GWEN_TYPE_UINT32 rid,
                                       const char *name,
                                       GWEN_DB_NODE *dbReq) {
  LCCL_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_DB_NODE *dbRsp;
  int cmdVer;
  int rv;

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

  cmdVer=GWEN_DB_GetIntValue(dbReq, "body/cmdver", 0, 0);

  cl=LCCL_Client_List_First(clm->clients);
  while(cl) {
    if (LCCL_Client_GetClientId(cl)==clientId)
      break;
    cl=LCCL_Client_List_Next(cl);
  } /* while */
  if (!cl) {
    DBG_ERROR(0, "Client \"%08x\" not found", clientId);
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Unknown client id");
    if (GWEN_IPCManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  DBG_NOTICE(0, "Client %08x: StartWait [%s/%s]",
             clientId,
             LCCL_Client_GetApplicationName(cl),
             LCCL_Client_GetUserName(cl));

  if (LCCL_Client_GetWaitRequestCount(cl)==0)
    LCS_Server_BeginUseReaders(clm->server);
  LCCL_Client_AddWaitRequestCount(cl);

  /* send all cards we already know, create response */
  dbRsp=GWEN_DB_Group_new("StartWaitResponse");
  rv=LCCL_ClientManager__SendInitialCards(clm, cl);
  if (rv) {
    DBG_ERROR(0, "Could not send card inserted messages");
    LCS_Server_SendErrorResponse(clm->server,
                                 rid,
                                 -rv,
                                 "Error sending card available messages");
  }
  else {
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "code", "OK");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", "Waiting for cards");
    if (GWEN_IPCManager_SendResponse(clm->ipcManager, rid, dbRsp)) {
      DBG_ERROR(0, "Could not send response to client");
      if (GWEN_IPCManager_RemoveRequest(clm->ipcManager, rid, 0)) {
        DBG_ERROR(0, "Could not remove request");
        abort();
      }
      return -1;
    }
  }

  if (GWEN_IPCManager_RemoveRequest(clm->ipcManager, rid, 0)) {
    DBG_ERROR(0, "Could not remove request");
    abort();
  }

  return 0; /* handled */
}



int LCCL_ClientManager_SendCardAvailable(LCCL_CLIENTMANAGER *clm,
                                         LCCL_CLIENT *cl,
                                         LCCO_CARD *card) {
  GWEN_DB_NODE *gr;
  const char *readerType;
  const char *driverName;
  const char *atr;
  unsigned int atrLen;
  char numbuf[16];
  GWEN_TYPE_UINT32 flags;
  LC_CARD_TYPE ct;
  GWEN_TYPE_UINT32 rid;
  GWEN_STRINGLISTENTRY *se;
  LCCM_CARDMANAGER *cm;

  cm=LCS_FullServer_GetCardManager(clm->server);
  assert(cm);

  gr=GWEN_DB_Group_new("CardAvailable");

  DBG_NOTICE(0, "Advertising card \"%08x\" to client \"%08x\" [%s/%s]",
             LCCO_Card_GetCardId(card),
             LCCL_Client_GetClientId(cl),
             LCCL_Client_GetApplicationName(cl),
             LCCL_Client_GetUserName(cl));

  readerType=LCCO_Card_GetReaderTypeName(card);
  assert(readerType);
  driverName=LCCO_Card_GetDriverTypeName(card);
  assert(driverName);
  atr=LCCO_Card_GetAtr(card, &atrLen);

  snprintf(numbuf, sizeof(numbuf)-1, "%08x",
           LCCO_Card_GetCardId(card));
  numbuf[sizeof(numbuf)-1]=0;

  GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "cardId", numbuf);
  GWEN_DB_DeleteVar(gr, "readerflags");
  flags=LCCO_Card_GetReaderFlags(card);
  LC_ReaderFlags_toDb(gr, "readerflags", flags);

  /* add card types */
  se=GWEN_StringList_FirstEntry(LCCO_Card_GetTypes(card));
  while (se) {
    const char *s;
  
    s=GWEN_StringListEntry_Data(se);
    assert(s);
    GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_DEFAULT,
                         "cardtypes", s);
    se=GWEN_StringListEntry_Next(se);
  }

  /* add cards' base type */
  ct=LCCO_Card_GetCardType(card);
  if (ct==LC_CardTypeProcessor) {
    GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_DEFAULT,
                         "cardtypes", "ProcessorCard");
    GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_DEFAULT,
                         "cardtype", "PROCESSOR");
  }
  else if (ct==LC_CardTypeMemory) {
    GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_DEFAULT,
                         "cardtypes", "MemoryCard");
    GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_DEFAULT,
                         "cardtype", "MEMORY");
  }
  else
    GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_DEFAULT,
                         "cardtype", "UNKNOWN");
  
  if (atr && atrLen) {
    GWEN_DB_SetBinValue(gr, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "atr", atr, atrLen);
  } /* if ATR */

  rid=GWEN_IPCManager_SendRequest(clm->ipcManager,
                                  LCCL_Client_GetClientId(cl),
                                  gr);
  if (rid==0) {
    DBG_ERROR(0, "Could not send \"CardAvailable\" to client");
    return -LC_ERROR_GENERIC;
  }
  else {
    /* remove request, we don't expect an answer */
    if (GWEN_IPCManager_RemoveRequest(clm->ipcManager, rid, 1)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
  }

  LCCM_CardManager_SetCardAdTime(cm, card, time(0));

  return 0;
}



int LCCL_ClientManager__SendInitialCards(LCCL_CLIENTMANAGER *clm,
                                         LCCL_CLIENT *cl) {
  LCCM_CARDMANAGER *cm;
  LCCO_CARD *card;

  cm=LCS_FullServer_GetCardManager(clm->server);
  assert(cm);

  card=LCCM_CardManager_GetFirstCard(cm);
  while(card) {
    if (LCCO_Card_GetStatus(card)==LC_CardStatusInserted) {
      int rv;

      rv=LCCL_ClientManager_SendCardAvailable(clm, cl, card);
      if (rv)
        return rv;
    }
    card=LCCM_CardManager_GetNextCard(cm, card);
  }

  return 0;
}








