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
#include <chipcard2/sharedstuff/pininfo.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/request.h>
#include <gwenhywfar/gwentime.h>



static int LCCL_ClientManager__sendVerify(LCCL_CLIENTMANAGER *clm,
                                          GWEN_TYPE_UINT32 rid,
                                          LCCL_CLIENT *cl,
                                          LCCO_CARD *card,
                                          const LC_PININFO *pi);
static int LCCL_ClientManager__sendCommand(LCCL_CLIENTMANAGER *clm,
                                           GWEN_TYPE_UINT32 rid,
                                           LCCL_CLIENT *cl,
                                           LCCO_CARD *card,
                                           const LC_PININFO *pi);




int LCCL_ClientManager_HandleVerify(LCCL_CLIENTMANAGER *clm,
                                    GWEN_TYPE_UINT32 rid,
                                    const char *name,
                                    GWEN_DB_NODE *dbReq) {
  LCCL_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_TYPE_UINT32 cardId;
  int cmdVer;
  LCCM_CARDMANAGER *cm;
  LCCO_CARD *card;
  int rv;
  GWEN_DB_NODE *dbPinInfo;
  LCDM_DEVICEMANAGER *dm;
  LC_PININFO *pi;

  dm=LCS_Server_GetDeviceManager(clm->server);
  assert(dm);

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

  cm=LCS_FullServer_GetCardManager(clm->server);
  assert(cm);

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

  DBG_NOTICE(0, "Client %08x: Verify [%s/%s]",
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
  rv=LCCL_ClientManager_CheckClientCardAccess(clm, card, cl);
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

  dbPinInfo=GWEN_DB_GetGroup(dbReq,
                             GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                             "data/pinInfo");
  if (!dbPinInfo) {
    DBG_ERROR(0, "No pinInfo given");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 LC_ERROR_INVALID,
                                 "No PinInfo given");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }
  pi=LC_PinInfo_fromDb(dbPinInfo);
  assert(pi);

  if (LCCO_Card_GetReaderFlags(card) & LC_READER_FLAGS_DRIVER_HAS_VERIFY)
    rv=LCCL_ClientManager__sendVerify(clm, rid, cl, card, pi);
  else
    rv=LCCL_ClientManager__sendCommand(clm, rid, cl, card, pi);
  LC_PinInfo_free(pi);
  if (rv)
    return -1;
  return 0; /* handled */
}



int LCCL_ClientManager__sendVerify(LCCL_CLIENTMANAGER *clm,
                                   GWEN_TYPE_UINT32 rid,
                                   LCCL_CLIENT *cl,
                                   LCCO_CARD *card,
                                   const LC_PININFO *pi) {
  GWEN_IPC_REQUEST *req;
  GWEN_IPC_REQUEST *dreq;
  GWEN_TIME *ti;
  GWEN_TYPE_UINT32 outRid;
  GWEN_DB_NODE *dbT;
  GWEN_DB_NODE *dbOutReq;
  LCDM_DEVICEMANAGER *dm;

  dm=LCS_Server_GetDeviceManager(clm->server);
  assert(dm);

  dbOutReq=GWEN_DB_Group_new("Driver_Verify");
  dbT=GWEN_DB_GetGroup(dbOutReq,
                       GWEN_DB_FLAGS_DEFAULT,
                       "PinInfo");
  assert(dbT);
  LC_PinInfo_toDb(pi, dbT);

  outRid=LCDM_DeviceManager_SendCardCommand(dm, card, dbOutReq);
  if (outRid==0) {
    DBG_ERROR(0, "Could not send command to reader");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 -1,
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
  GWEN_IpcRequest_SetIpcId(req, LCCL_Client_GetClientId(cl));
  GWEN_IpcRequest_SetName(req, "Client_Verify");
  LCCL_Request_SetUint32Data2(req, 0);
  ti=GWEN_CurrentTime();
  assert(ti);
  GWEN_Time_AddSeconds(ti, clm->commandTimeout);
  GWEN_IpcRequest_SetExpires(req, ti);
  GWEN_Time_free(ti);
  LCCL_Request_SetClientManager(req, clm);
  LCCL_Request_SetClient(req, cl);
  LCCL_Request_SetCard(req, card);
  GWEN_IpcRequest_SetWorkFn(req, LCCL_ClientManager_WorkVerify);
  GWEN_IpcRequest_SetStatus(req, GWEN_IpcRequest_StatusPartial);

  /* create subrequest, add it to request */
  dreq=LCCL_Request_new();
  GWEN_IpcRequest_SetId(dreq, outRid);
  GWEN_IpcRequest_SetName(dreq, "Driver_Verify");
  ti=GWEN_CurrentTime();
  assert(ti);
  GWEN_Time_AddSeconds(ti, clm->commandTimeout);
  GWEN_IpcRequest_SetExpires(dreq, ti);
  GWEN_Time_free(ti);
  LCCL_Request_SetClientManager(dreq, clm);
  LCCL_Request_SetClient(dreq, cl);
  LCCL_Request_SetCard(dreq, card);
  GWEN_IpcRequest_SetWorkFn(dreq, LCCL_ClientManager_WorkDriverVerify);
  GWEN_IpcRequest_SetStatus(dreq, GWEN_IpcRequest_StatusSent);
  GWEN_IpcRequest_List_Add(dreq, GWEN_IpcRequest_GetSubRequests(req));

  /* add incoming request */
  GWEN_IpcRequestManager_AddRequest(LCS_Server_GetRequestManager(clm->server),
				    req);

  /* do not remove the request now, since it is enqueued */
  DBG_NOTICE(0,
             "Enqueued Verify request for card \"%08x\" "
             "and client \"%08x\"",
             LCCO_Card_GetCardId(card),
             LCCL_Client_GetClientId(cl));

  return 0;
}



int LCCL_ClientManager__sendCommand(LCCL_CLIENTMANAGER *clm,
                                    GWEN_TYPE_UINT32 rid,
                                    LCCL_CLIENT *cl,
                                    LCCO_CARD *card,
                                    const LC_PININFO *pi) {
  LCCM_CARDMANAGER *cm;
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

  cm=LCS_FullServer_GetCardManager(clm->server);
  assert(cm);

  cmdm=LCS_FullServer_GetCommandManager(clm->server);
  assert(cmdm);

  switch(LC_PinInfo_GetEncoding(pi)) {
  case LC_PinInfo_EncodingBin:
    cmdName="IsoPerformVerification_Bin";
    break;
  case LC_PinInfo_EncodingBcd:
    cmdName="IsoPerformVerification_Bcd";
    break;
  case LC_PinInfo_EncodingAscii:
    cmdName="IsoPerformVerification_Ascii";
    break;
  case LC_PinInfo_EncodingFpin2:
    cmdName="IsoPerformVerification_Fpin2";
    break;
  default:
    cmdName=0;
    break;
  }

  if (!cmdName) {
    LCS_Server_SendErrorResponse(clm->server, rid, -1,
                                 "Unsupported pin encoding");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
  }

  /* now handle command */
  dbCmd=GWEN_DB_Group_new("command");
  GWEN_DB_SetIntValue(dbCmd, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "pid", LC_PinInfo_GetId(pi));

  /* transform command to APDU */
  apdu=GWEN_Buffer_new(0, 256, 0, 1);
  rv=LCCMD_CommandManager_BuildCommand(cmdm,
                                       card,
                                       cmdName,
                                       dbCmd,
                                       apdu,
                                       &target,
                                       &cmdReqId);
  GWEN_DB_Group_free(dbCmd);
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
  dbOutReq=GWEN_DB_Group_new("Driver_CardCommand");
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
  GWEN_IpcRequest_SetIpcId(req, LCCL_Client_GetClientId(cl));
  GWEN_IpcRequest_SetName(req, "Client_Verify");
  LCCL_Request_SetUint32Data(req, cmdReqId);
  LCCL_Request_SetUint32Data2(req, 1);
  ti=GWEN_CurrentTime();
  assert(ti);
  GWEN_Time_AddSeconds(ti, clm->commandTimeout);
  GWEN_IpcRequest_SetExpires(req, ti);
  GWEN_Time_free(ti);
  LCCL_Request_SetClientManager(req, clm);
  LCCL_Request_SetClient(req, cl);
  LCCL_Request_SetCard(req, card);
  GWEN_IpcRequest_SetWorkFn(req, LCCL_ClientManager_WorkVerify);
  GWEN_IpcRequest_SetStatus(req, GWEN_IpcRequest_StatusPartial);

  /* create subrequest, add it to request */
  dreq=LCCL_Request_new();
  GWEN_IpcRequest_SetId(dreq, outRid);
  GWEN_IpcRequest_SetName(dreq, "Driver_CardCommand");
  ti=GWEN_CurrentTime();
  assert(ti);
  GWEN_Time_AddSeconds(ti, clm->commandTimeout);
  GWEN_IpcRequest_SetExpires(dreq, ti);
  GWEN_Time_free(ti);
  LCCL_Request_SetClientManager(dreq, clm);
  LCCL_Request_SetClient(dreq, cl);
  LCCL_Request_SetCard(dreq, card);
  GWEN_IpcRequest_SetWorkFn(dreq, LCCL_ClientManager_WorkDriverVerify);
  GWEN_IpcRequest_SetStatus(dreq, GWEN_IpcRequest_StatusSent);
  GWEN_IpcRequest_List_Add(dreq, GWEN_IpcRequest_GetSubRequests(req));

  /* add incoming request */
  GWEN_IpcRequestManager_AddRequest(LCS_Server_GetRequestManager(clm->server),
				    req);

  /* do not remove the request now, since it is enqueued */
  DBG_NOTICE(0,
             "Enqueued ExecApdu request for card \"%08x\" "
             "and client \"%08x\"",
             LCCO_Card_GetCardId(card),
             LCCL_Client_GetClientId(cl));
  return 0; /* handled */
}






int LCCL_ClientManager_WorkVerify(GWEN_IPC_REQUEST *req) {
  LCCL_CLIENTMANAGER *clm;
  LCCL_CLIENT *cl;
  LCCO_CARD *card;
  GWEN_TYPE_UINT32 rid;
  GWEN_TYPE_UINT32 drid;
  GWEN_IPC_REQUEST *dreq;
  GWEN_DB_NODE *dbDriverResponse;

  DBG_ERROR(0, "Working on Verify request");

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

  dbDriverResponse=GWEN_IpcManager_GetResponseData(clm->ipcManager, drid);
  if (dbDriverResponse) {
    GWEN_DB_NODE *dbClientResponse;
    const char *p;
    int triesLeft;
    const char *code;

    DBG_DEBUG(0, "Sending response to Verify");
    dbClientResponse=GWEN_DB_Group_new("Client_VerifyResponse");
    code=GWEN_DB_GetCharValue(dbDriverResponse,
                              "data/code", 0, "ERROR");
    assert(code);
    GWEN_DB_SetCharValue(dbClientResponse,
                         GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "code", code);
    p=GWEN_DB_GetCharValue(dbDriverResponse,
                           "data/text", 0, 0);
    if (p)
      GWEN_DB_SetCharValue(dbClientResponse,
                           GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text", p);
    if (LCCL_Request_GetUint32Data2(req)==0) {
      /* response to Driver_Verify */
      triesLeft=GWEN_DB_GetIntValue(dbDriverResponse,
                                    "data/triesLeft", 0, 0);
      GWEN_DB_SetIntValue(dbClientResponse,
                          GWEN_DB_FLAGS_OVERWRITE_VARS,
                          "triesLeft", triesLeft);
    }
    else {
      const void *bp;
      unsigned int bs;

      /* response to Driver_CommandCard */
      bp=GWEN_DB_GetBinValue(dbDriverResponse,
                             "data/data", 0, 0, 0, &bs);
      if (bp && bs>1) {
        const unsigned char *p;

        p=(const unsigned char*)bp;
        if (p[bs-2]!=0x90) {
          if (p[bs-2]==0x63) {
            GWEN_DB_SetIntValue(dbClientResponse,
                                GWEN_DB_FLAGS_OVERWRITE_VARS,
                                "triesLeft", (p[bs-1] & 0x3f));
          }
          GWEN_DB_SetCharValue(dbClientResponse,
                               GWEN_DB_FLAGS_OVERWRITE_VARS,
                               "code", "ERROR");
          GWEN_DB_SetCharValue(dbClientResponse,
                               GWEN_DB_FLAGS_OVERWRITE_VARS,
                               "text", "APDU result: error");
        }
      }
      else {
        GWEN_DB_SetCharValue(dbClientResponse,
                             GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "code", "ERROR");
        GWEN_DB_SetCharValue(dbClientResponse,
                             GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text", "Too small APDU response");
      }
    }
    GWEN_DB_Group_free(dbDriverResponse);

    /* send response */
    if (GWEN_IpcManager_SendResponse(clm->ipcManager,
                                     rid,
                                     dbClientResponse)) {
      DBG_ERROR(0, "Could not send Verify response");
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



int LCCL_ClientManager_WorkDriverVerify(GWEN_IPC_REQUEST *req) {
  DBG_ERROR(0, "Working on Verify request");
  return 1; /* nothing done */
}


