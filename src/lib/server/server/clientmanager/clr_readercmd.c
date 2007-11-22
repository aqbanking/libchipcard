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
#include "server_l.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/request.h>
#include <gwenhywfar/gwentime.h>


int LCCL_ClientManager_HandleReaderCmd(LCCL_CLIENTMANAGER *clm,
                                       uint32_t rid,
                                       const char *name,
                                       GWEN_DB_NODE *dbReq) {
  LCCL_CLIENT *cl;
  uint32_t clientId;
  uint32_t readerId;
  uint32_t lockId;
  int cmdVer;
  GWEN_IPC_REQUEST *req;
  GWEN_IPC_REQUEST *dreq;
  GWEN_TIME *ti;
  int rv;
  uint32_t outRid;
  GWEN_DB_NODE *dbOutReq;
  const void *p;
  unsigned int bs;
  const char *target;
  LCDM_DEVICEMANAGER *dm;

  dm=LCS_Server_GetDeviceManager(clm->server);
  assert(dm);

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  if (clientId==0) {
    DBG_ERROR(0, "No client id");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  cmdVer=GWEN_DB_GetIntValue(dbReq, "data/cmdver", 0, 0);

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

  DBG_NOTICE(0, "Client %08x: Client_ReaderCommand [%s/%s]",
             clientId,
             LCCL_Client_GetApplicationName(cl),
             LCCL_Client_GetUserName(cl));

  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "data/readerid", 0, "0"),
                "%x", &readerId)) {
    DBG_ERROR(0, "Missing reader id");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Missing reader id");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "data/lockid", 0, "0"),
                "%x", &lockId)) {
    DBG_ERROR(0, "Missing lock id");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Missing lock id");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* check whether we have a lock on this card */
  rv=LCDM_DeviceManager_CheckLockReaderAccess(dm, readerId, lockId);
  if (rv) {
    DBG_ERROR(0, "Reader not locked by this client");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 -rv,
                                 "Reader not locked by this client");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  p=GWEN_DB_GetBinValue(dbReq, "data/data", 0, 0, 0, &bs);
  if (!p || !bs) {
    DBG_ERROR(0, "No data give");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 LC_ERROR_INVALID,
                                 "No data given");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  target=GWEN_DB_GetCharValue(dbReq, "data/target", 0, 0);
  if (!target) {
    DBG_ERROR(0, "No target given");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 LC_ERROR_INVALID,
                                 "No target given");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* check whether we have a lock on this reader */
  rv=LCDM_DeviceManager_CheckLockReaderAccess(dm, readerId, lockId);
  if (rv) {
    DBG_ERROR(0, "Reader not locked by this client");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 -rv,
                                 "Reader not locked by this client");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* create subrequest, send it to the reader */
  dbOutReq=GWEN_DB_Group_new("Driver_ReaderCommand");
  GWEN_DB_SetBinValue(dbOutReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "data", p, bs);
  GWEN_DB_SetCharValue(dbOutReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "target", target);
  outRid=LCDM_DeviceManager_SendReaderCommand(dm, readerId, dbOutReq);
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
  ti=GWEN_CurrentTime();
  assert(ti);
  GWEN_Time_AddSeconds(ti, clm->commandTimeout);
  GWEN_IpcRequest_SetExpires(req, ti);
  GWEN_Time_free(ti);
  LCCL_Request_SetClientManager(req, clm);
  LCCL_Request_SetClient(req, cl);
  GWEN_IpcRequest_SetWorkFn(req, LCCL_ClientManager_WorkClientReaderCmd);
  GWEN_IpcRequest_SetStatus(req, GWEN_IpcRequest_StatusPartial);

  /* create subrequest, add it to request */
  dreq=LCCL_Request_new();
  GWEN_IpcRequest_SetId(dreq, outRid);
  GWEN_IpcRequest_SetName(dreq, "Driver_ReaderCommand");
  ti=GWEN_CurrentTime();
  assert(ti);
  GWEN_Time_AddSeconds(ti, clm->commandTimeout);
  GWEN_IpcRequest_SetExpires(dreq, ti);
  GWEN_Time_free(ti);
  LCCL_Request_SetClientManager(dreq, clm);
  LCCL_Request_SetClient(dreq, cl);
  GWEN_IpcRequest_SetWorkFn(dreq, LCCL_ClientManager_WorkDriverReaderCmd);
  GWEN_IpcRequest_SetStatus(dreq, GWEN_IpcRequest_StatusSent);
  GWEN_IpcRequest_List_Add(dreq, GWEN_IpcRequest_GetSubRequests(req));

  /* add incoming request */
  GWEN_IpcRequestManager_AddRequest(LCS_Server_GetRequestManager(clm->server),
				    req);

  /* do not remove the request now, since it is enqueued */
  DBG_NOTICE(0,
             "Enqueued Driver_ReaderCommand request for reader \"%08x\" "
             "and client \"%08x\" (lockId: %0x8)",
             readerId, clientId, lockId);
  return 0; /* handled */
}



int LCCL_ClientManager_WorkClientReaderCmd(GWEN_IPC_REQUEST *req) {
  LCCL_CLIENTMANAGER *clm;
  LCCL_CLIENT *cl;
  uint32_t rid;
  uint32_t drid;
  GWEN_IPC_REQUEST *dreq;
  GWEN_DB_NODE *dbDriverResponse;

  DBG_ERROR(0, "Working on Client_ReaderCommand request");

  rid=GWEN_IpcRequest_GetId(req);
  assert(rid);

  clm=LCCL_Request_GetClientManager(req);
  assert(clm);

  cl=LCCL_Request_GetClient(req);
  assert(clm);

  dreq=GWEN_IpcRequest_List_First(GWEN_IpcRequest_GetSubRequests(req));
  assert(dreq);
  drid=GWEN_IpcRequest_GetId(dreq);

  dbDriverResponse=GWEN_IpcManager_GetResponseData(clm->ipcManager, drid);
  if (dbDriverResponse) {
    GWEN_DB_NODE *dbClientResponse;
    const char *p;
    const void *bp;
    unsigned int bs;
    int code;

    DBG_DEBUG(0, "Sending response to Client_ReaderCommand");
    dbClientResponse=GWEN_DB_Group_new("Client_ReaderCommandResponse");
    code=GWEN_DB_GetIntValue(dbDriverResponse,
                             "data/code", 0, -1);
    GWEN_DB_SetIntValue(dbClientResponse,
                        GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "code", code);
    p=GWEN_DB_GetCharValue(dbDriverResponse,
                           "data/text", 0, 0);
    if (p)
      GWEN_DB_SetCharValue(dbClientResponse,
			   GWEN_DB_FLAGS_OVERWRITE_VARS,
			   "text", p);

    bp=GWEN_DB_GetBinValue(dbDriverResponse,
			   "data/data", 0, 0, 0, &bs);
    if (bp && bs)
      GWEN_DB_SetBinValue(dbClientResponse,
			  GWEN_DB_FLAGS_OVERWRITE_VARS,
			  "data", bp, bs);
    GWEN_DB_Group_free(dbDriverResponse);

    /* send response */
    if (GWEN_IpcManager_SendResponse(clm->ipcManager,
                                     rid,
				     dbClientResponse)) {
      DBG_ERROR(0, "Could not send ReaderCommand response");
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



int LCCL_ClientManager_WorkDriverReaderCmd(GWEN_IPC_REQUEST *req) {
  DBG_ERROR(0, "Working on Driver_ReaderCommand request");
  return 1; /* nothing done */
}




