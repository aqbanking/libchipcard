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
#include "cl_request_l.h"
#include "fullserver_l.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/request.h>
#include <gwenhywfar/gwentime.h>



GWEN_TYPE_UINT32 LCCL_ClientManager_SendResetCard(LCCL_CLIENTMANAGER *clm,
                                                  LCCO_CARD *card) {
  GWEN_DB_NODE *dbOutReq;
  LCDM_DEVICEMANAGER *dm;
  GWEN_TYPE_UINT32 outRid;

  assert(clm);

  dm=LCS_Server_GetDeviceManager(clm->server);
  assert(dm);

  DBG_NOTICE(0, "Resetting card \"%08x\"", LCCO_Card_GetCardId(card));

  dbOutReq=GWEN_DB_Group_new("ResetCard");
  outRid=LCDM_DeviceManager_SendCardCommand(dm, card, dbOutReq);

  return outRid;
}



int LCCL_ClientManager_HandleCardReset(LCCL_CLIENTMANAGER *clm,
                                       GWEN_TYPE_UINT32 rid,
                                       const char *name,
                                       GWEN_DB_NODE *dbReq) {
  LCCL_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_TYPE_UINT32 cardId;
  int cmdVer;
  LCCM_CARDMANAGER *cm;
  LCCO_CARD *card;
  GWEN_IPC_REQUEST *req;
  GWEN_IPC_REQUEST *dreq;
  GWEN_TIME *ti;
  int rv;
  GWEN_TYPE_UINT32 outRid;

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

  cm=LCS_FullServer_GetCardManager(clm->server);
  assert(cm);

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

  DBG_NOTICE(0, "Client %08x: CardReset [%s/%s]",
             clientId,
             LCCL_Client_GetApplicationName(cl),
             LCCL_Client_GetUserName(cl));

  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/cardid", 0, "0"),
                "%x", &cardId)) {
    DBG_ERROR(0, "Missing card id");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Missing card id");
    if (GWEN_IPCManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* get referenced card */
  card=LCCM_CardManager_FindCard(cm, cardId);
  if (!card) {
    DBG_ERROR(0, "Card not found");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Card not found");
    if (GWEN_IPCManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* check whether we have a lock on this card */
  rv=LCCM_CardManager_CheckAccess(cm, card, clientId);
  if (rv) {
    DBG_ERROR(0, "Card not locked by this client");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 -rv,
                                 "Card not locked by this client");
    if (GWEN_IPCManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* check whether card is still inserted */
  if (LCCO_Card_GetStatus(card)!=LC_CardStatusInserted) {
    DBG_ERROR(0, "Card has been removed");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 LC_ERROR_CARD_REMOVED,
                                 "Card has been removed");
    if (GWEN_IPCManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* send the reset command */
  outRid=LCCL_ClientManager_SendResetCard(clm, card);
  if (outRid==0) {
    DBG_ERROR(0, "Could not send command to reader");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 -rv,
                                 "Could not send command to reader");
    if (GWEN_IPCManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* create request */
  req=LCCL_Request_new();
  assert(req);
  GWEN_IpcRequest_SetId(req, rid);
  GWEN_IpcRequest_SetIpcId(req, clientId);
  GWEN_IpcRequest_SetName(req, name);
  ti=GWEN_CurrentTime();
  assert(ti);
  GWEN_Time_AddSeconds(ti, clm->commandTimeout);
  GWEN_IpcRequest_SetExpires(req, ti);
  GWEN_Time_free(ti);
  LCCL_Request_SetClientManager(req, clm);
  LCCL_Request_SetClient(req, cl);
  LCCL_Request_SetCard(req, card);
  GWEN_IpcRequest_SetWorkFn(req, LCCL_ClientManager_WorkExecApdu);
  GWEN_IpcRequest_SetStatus(req, GWEN_IpcRequest_StatusPartial);

  /* create subrequest, add it to request */
  dreq=LCCL_Request_new();
  GWEN_IpcRequest_SetId(dreq, outRid);
  GWEN_IpcRequest_SetName(dreq, "ResetCard");
  ti=GWEN_CurrentTime();
  assert(ti);
  GWEN_Time_AddSeconds(ti, clm->commandTimeout);
  GWEN_IpcRequest_SetExpires(dreq, ti);
  GWEN_Time_free(ti);
  LCCL_Request_SetClientManager(dreq, clm);
  LCCL_Request_SetClient(dreq, cl);
  LCCL_Request_SetCard(dreq, card);
  GWEN_IpcRequest_SetWorkFn(dreq, LCCL_ClientManager_WorkCardReset);
  GWEN_IpcRequest_SetStatus(dreq, GWEN_IpcRequest_StatusSent);
  GWEN_IpcRequest_List_Add(dreq, GWEN_IpcRequest_GetSubRequests(req));

  /* add incoming request */
  GWEN_IpcRequestManager_AddRequest(LCS_Server_GetRequestManager(clm->server),
				    req);

  /* do not remove the request now, since it is enqueued */
  DBG_NOTICE(0,
             "Enqueued ExecApdu request for card \"%08x\" "
             "and client \"%08x\"",
             cardId,
             clientId);
  return 0; /* handled */
}



int LCCL_ClientManager_WorkCardReset(GWEN_IPC_REQUEST *req) {
  LCCL_CLIENTMANAGER *clm;
  LCCL_CLIENT *cl;
  LCCO_CARD *card;
  GWEN_TYPE_UINT32 rid;
  GWEN_TYPE_UINT32 drid;
  GWEN_IPC_REQUEST *dreq;
  GWEN_DB_NODE *dbDriverResponse;

  DBG_ERROR(0, "Working on ExecApdu request");

  rid=GWEN_IpcRequest_GetId(req);
  assert(rid);

  clm=LCCL_Request_GetClientManager(req);
  assert(clm);

  cl=LCCL_Request_GetClient(req);
  assert(clm);

  card=LCCL_Request_GetCard(req);
  assert(card);

  dreq=GWEN_IpcRequest_List_First(GWEN_IpcRequest_GetSubRequests(req));
  assert(dreq);
  drid=GWEN_IpcRequest_GetId(dreq);

  dbDriverResponse=GWEN_IPCManager_GetResponseData(clm->ipcManager, drid);
  if (dbDriverResponse) {
    GWEN_DB_NODE *dbClientResponse;
    const char *p;
    const char *code;

    DBG_DEBUG(0, "Sending response to CardReset");
    dbClientResponse=GWEN_DB_Group_new("CardResetResponse");
    code=GWEN_DB_GetCharValue(dbDriverResponse,
                              "body/code", 0, "ERROR");
    assert(code);
    GWEN_DB_SetCharValue(dbClientResponse,
                         GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "code", code);
    p=GWEN_DB_GetCharValue(dbDriverResponse,
                           "body/text", 0, 0);
    if (p)
      GWEN_DB_SetCharValue(dbClientResponse,
			   GWEN_DB_FLAGS_OVERWRITE_VARS,
			   "text", p);
    GWEN_DB_Group_free(dbDriverResponse);

    /* send response */
    if (GWEN_IPCManager_SendResponse(clm->ipcManager,
                                     rid,
                                     dbClientResponse)) {
      DBG_ERROR(0, "Could not send CardReset response");
      if (GWEN_IPCManager_RemoveRequest(clm->ipcManager, drid, 1)) {
        DBG_ERROR(0, "Could not remove request");
        abort();
      }
      if (GWEN_IPCManager_RemoveRequest(clm->ipcManager, rid, 0)) {
        DBG_ERROR(0, "Could not remove request");
        abort();
      }
      GWEN_IpcRequest_List_Del(req);
      GWEN_IpcRequest_free(req);
      return -1;
    }

    /* remove IpcManager requests and IpcRequests */
    if (GWEN_IPCManager_RemoveRequest(clm->ipcManager, drid, 1)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    if (GWEN_IPCManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    GWEN_IpcRequest_List_Del(req);
    GWEN_IpcRequest_free(req);
    return 0;
  }

  return 1; /* nothing done */
}


