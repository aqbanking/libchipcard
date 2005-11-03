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



int LCCL_ClientManager_HandleExecCommand(LCCL_CLIENTMANAGER *clm,
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
  GWEN_DB_NODE *dbOutReq;
  const char *target;
  LCDM_DEVICEMANAGER *dm;
  LCCMD_COMMANDMANAGER *cmdm;
  const char *cmdName;
  GWEN_DB_NODE *dbCmd;
  GWEN_BUFFER *apdu;
  GWEN_TYPE_UINT32 cmdReqId=0;

  dm=LCS_Server_GetDeviceManager(clm->server);
  assert(dm);

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

  cm=LCS_FullServer_GetCardManager(clm->server);
  assert(cm);

  cmdm=LCS_FullServer_GetCommandManager(clm->server);
  assert(cmdm);

  cmdVer=GWEN_DB_GetIntValue(dbReq, "data/cmdver", 0, 0);

  /* find client */
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
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  DBG_NOTICE(0, "Client %08x: ExecCommand [%s/%s]",
             clientId,
             LCCL_Client_GetApplicationName(cl),
             LCCL_Client_GetUserName(cl));

  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "data/cardid", 0, "0"),
                "%x", &cardId)) {
    DBG_ERROR(0, "Missing card id");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Missing card id");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
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
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
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
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
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
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* now handle command */
  dbCmd=GWEN_DB_GetGroup(dbReq, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                         "data/command");
  if (!dbCmd) {
    DBG_ERROR(0, "Command group");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Command group missing");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }
  cmdName=GWEN_DB_GetCharValue(dbReq, "data/command/name", 0, 0);
  if (!cmdName) {
    DBG_ERROR(0, "Command name missing");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Command name missing");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* transform command to APDU */
  apdu=GWEN_Buffer_new(0, 256, 0, 1);
  rv=LCCMD_CommandManager_BuildCommand(cmdm,
                                       card,
                                       cmdName,
                                       dbCmd,
                                       apdu,
                                       &target,
                                       &cmdReqId);
  if (rv) {
    const char *s;

    DBG_INFO(0, "here (%d)", rv);
    if (GWEN_Buffer_GetUsedBytes(apdu))
      s=GWEN_Buffer_GetStart(apdu);
    else
      s="Error building command";

    LCS_Server_SendErrorResponse(clm->server, rid, -rv, s);
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    GWEN_Buffer_free(apdu);
    return -1;
  }
  assert(cmdReqId);
  assert(target);

  /* create CardCommand for reader */
  dbOutReq=GWEN_DB_Group_new("CardCommand");
  GWEN_DB_SetBinValue(dbOutReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "data",
                      GWEN_Buffer_GetStart(apdu),
                      GWEN_Buffer_GetUsedBytes(apdu));
  GWEN_Buffer_free(apdu);
  GWEN_DB_SetCharValue(dbOutReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "target", target);

  /* send CardCommand */
  outRid=LCDM_DeviceManager_SendCardCommand(dm, card, dbOutReq);
  if (outRid==0) {
    DBG_ERROR(0, "Could not send command to reader");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 -rv,
                                 "Could not send command to reader");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
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
  LCCL_Request_SetUint32Data(req, cmdReqId);
  ti=GWEN_CurrentTime();
  assert(ti);
  GWEN_Time_AddSeconds(ti, clm->commandTimeout);
  GWEN_IpcRequest_SetExpires(req, ti);
  GWEN_Time_free(ti);
  LCCL_Request_SetClientManager(req, clm);
  LCCL_Request_SetClient(req, cl);
  LCCL_Request_SetCard(req, card);
  GWEN_IpcRequest_SetWorkFn(req, LCCL_ClientManager_WorkExecCommand);
  GWEN_IpcRequest_SetStatus(req, GWEN_IpcRequest_StatusPartial);

  /* create subrequest, add it to request */
  dreq=LCCL_Request_new();
  GWEN_IpcRequest_SetId(dreq, outRid);
  GWEN_IpcRequest_SetName(dreq, "CardCommand");
  ti=GWEN_CurrentTime();
  assert(ti);
  GWEN_Time_AddSeconds(ti, clm->commandTimeout);
  GWEN_IpcRequest_SetExpires(dreq, ti);
  GWEN_Time_free(ti);
  LCCL_Request_SetClientManager(dreq, clm);
  LCCL_Request_SetClient(dreq, cl);
  LCCL_Request_SetCard(dreq, card);
  GWEN_IpcRequest_SetWorkFn(dreq, LCCL_ClientManager_WorkCardCommand);
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



int LCCL_ClientManager_WorkExecCommand(GWEN_IPC_REQUEST *req) {
  LCCL_CLIENTMANAGER *clm;
  LCCL_CLIENT *cl;
  LCCO_CARD *card;
  GWEN_TYPE_UINT32 rid;
  GWEN_TYPE_UINT32 drid;
  GWEN_IPC_REQUEST *dreq;
  GWEN_DB_NODE *dbDriverResponse;
  LCCMD_COMMANDMANAGER *cmdm;

  DBG_ERROR(0, "Working on ExecCommand request");

  rid=GWEN_IpcRequest_GetId(req);
  assert(rid);

  clm=LCCL_Request_GetClientManager(req);
  assert(clm);

  cmdm=LCS_FullServer_GetCommandManager(clm->server);
  assert(cmdm);

  cl=LCCL_Request_GetClient(req);
  assert(clm);

  card=LCCL_Request_GetCard(req);
  assert(card);

  dreq=GWEN_IpcRequest_List_First(GWEN_IpcRequest_GetSubRequests(req));
  assert(dreq);
  drid=GWEN_IpcRequest_GetId(dreq);

  dbDriverResponse=GWEN_IpcManager_GetResponseData(clm->ipcManager, drid);
  if (dbDriverResponse) {
    GWEN_DB_NODE *dbClientResponse;
    const char *p;
    const void *bp;
    unsigned int bs;
    const char *code;
    GWEN_TYPE_UINT32 cmdReqId;

    DBG_DEBUG(0, "Sending response to CommandCard");

    cmdReqId=LCCL_Request_GetUint32Data(req);
    assert(cmdReqId);

    code=GWEN_DB_GetCharValue(dbDriverResponse,
			      "data/code", 0, "ERROR");
    assert(code);
    if (strcasecmp(code, "ok")!=0) {
      LCS_Server_SendErrorResponse(clm->server, rid,
                                   LC_ERROR_GENERIC,
                                   GWEN_DB_GetCharValue(dbDriverResponse,
                                                        "data/text", 0,
                                                        "Driver error"));
      if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, drid, 1)) {
        DBG_ERROR(0, "Could not remove request");
        abort();
      }
      if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
        DBG_ERROR(0, "Could not remove request");
        abort();
      }
      GWEN_IpcRequest_List_Del(req);
      GWEN_IpcRequest_free(req);
      GWEN_DB_Group_free(dbDriverResponse);
      return -1;
    }

    /* create response for client */
    dbClientResponse=GWEN_DB_Group_new("CommandCardResponse");
    GWEN_DB_SetCharValue(dbClientResponse,
			 GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "code", "Ok");
    p=GWEN_DB_GetCharValue(dbDriverResponse,
			   "data/text", 0, 0);
    if (p)
      GWEN_DB_SetCharValue(dbClientResponse,
			   GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text", p);

    bp=GWEN_DB_GetBinValue(dbDriverResponse,
			   "data/data", 0, 0, 0, &bs);
    if (bp && bs) {
      GWEN_DB_NODE *dbCmdRsp;
      GWEN_BUFFER *dbuf;
      int rv;

      dbCmdRsp=GWEN_DB_GetGroup(dbClientResponse,
                                GWEN_DB_FLAGS_OVERWRITE_GROUPS,
                                "command");
      dbuf=GWEN_Buffer_new(0, bs, 0, 1);
      GWEN_Buffer_AppendBytes(dbuf, bp, bs);
      rv=LCCMD_CommandManager_ParseAnswer(cmdm, card, cmdReqId,
                                          dbuf, dbCmdRsp);
      if (rv==0 && GWEN_Buffer_GetUsedBytes(dbuf)) {
        GWEN_DB_SetBinValue(dbClientResponse,
                            GWEN_DB_FLAGS_OVERWRITE_VARS,
                            "data",
                            GWEN_Buffer_GetStart(dbuf),
                            GWEN_Buffer_GetUsedBytes(dbuf));
      }
      GWEN_Buffer_free(dbuf);

      if (rv) {
        DBG_ERROR(0, "Error parsing answer");
        LCS_Server_SendErrorResponse(clm->server, rid,
                                     -rv,
                                     "Error parsing answer");
        if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, drid, 1)) {
          DBG_ERROR(0, "Could not remove request");
          abort();
        }
        if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
          DBG_ERROR(0, "Could not remove request");
          abort();
        }
        GWEN_IpcRequest_List_Del(req);
        GWEN_IpcRequest_free(req);
        GWEN_DB_Group_free(dbDriverResponse);
        return -1;
      }
    }
    GWEN_DB_Group_free(dbDriverResponse);

    /* send response */
    if (GWEN_IpcManager_SendResponse(clm->ipcManager,
                                     rid,
				     dbClientResponse)) {
      DBG_ERROR(0, "Could not send CommandCard response");
      if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, drid, 1)) {
        DBG_ERROR(0, "Could not remove request");
        abort();
      }
      if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
        DBG_ERROR(0, "Could not remove request");
        abort();
      }
      GWEN_IpcRequest_List_Del(req);
      GWEN_IpcRequest_free(req);
      return -1;
    }

    /* remove IpcManager requests and IpcRequests */
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, drid, 1)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    GWEN_IpcRequest_List_Del(req);
    GWEN_IpcRequest_free(req);
    return 0;
  }

  return 1; /* nothing done */
}



