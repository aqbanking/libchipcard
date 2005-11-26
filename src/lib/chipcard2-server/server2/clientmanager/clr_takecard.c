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


#define LCCL_REQ_TAKECARD_STEP1 1
#define LCCL_REQ_TAKECARD_STEP2 2



int LCCL_ClientManager_HandleTakeCard(LCCL_CLIENTMANAGER *clm,
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
  GWEN_TIME *ti;
  int rv;

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

  DBG_NOTICE(0, "Client %08x: TakeCard [%s/%s]",
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

  /* request a lock on this card */
  rv=LCCM_CardManager_RequestLockCard(cm, card, clientId,
                                      LCCL_Client_GetMaxClientLockTime(cl),
                                      LCCL_Client_GetMaxClientLocks(cl));
  if (rv) {
    DBG_ERROR(0, "Could not lock card");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 -rv,
                                 "Could not lock card");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  req=LCCL_Request_new();
  assert(req);
  GWEN_IpcRequest_SetId(req, rid);
  GWEN_IpcRequest_SetIpcId(req, clientId);
  GWEN_IpcRequest_SetName(req, name);
  ti=GWEN_CurrentTime();
  assert(ti);
  GWEN_Time_AddSeconds(ti, clm->takeCardExpireTimeout);
  LCCL_Request_SetClientManager(req, clm);
  LCCL_Request_SetClient(req, cl);
  LCCL_Request_SetCard(req, card);
  LCCL_Request_SetUint32Data(req, LCCL_REQ_TAKECARD_STEP1);
  GWEN_IpcRequest_SetWorkFn(req, LCCL_ClientManager_WorkTakeCard);
  GWEN_IpcRequest_SetStatus(req, GWEN_IpcRequest_StatusPartial);
  GWEN_IpcRequestManager_AddRequest(LCS_Server_GetRequestManager(clm->server),
                                    req);
  /* do not remove the request now, since it is enqueued */
  DBG_NOTICE(0,
             "Enqueued TakeCard request for card \"%08x\" "
             "and client \"%08x\"",
             cardId,
             clientId);
  return 0; /* handled */
}



int LCCL_ClientManager_WorkTakeCard(GWEN_IPC_REQUEST *req) {
  LCCL_CLIENTMANAGER *clm;
  LCCL_CLIENT *cl;
  LCCO_CARD *card;
  LCCM_CARDMANAGER *cm;
  LCDM_DEVICEMANAGER *dm;
  int rv;
  GWEN_TYPE_UINT32 rid;
  GWEN_TYPE_UINT32 step;

  DBG_ERROR(0, "Working on TakeCard request");

  rid=GWEN_IpcRequest_GetId(req);
  assert(rid);

  clm=LCCL_Request_GetClientManager(req);
  assert(clm);

  cl=LCCL_Request_GetClient(req);
  assert(clm);

  card=LCCL_Request_GetCard(req);
  assert(card);

  cm=LCS_FullServer_GetCardManager(clm->server);
  assert(cm);

  dm=LCS_Server_GetDeviceManager(clm->server);
  assert(dm);


  step=LCCL_Request_GetUint32Data(req);
  if (step==LCCL_REQ_TAKECARD_STEP1) {
    DBG_INFO(0, "Step 1");
    rv=LCCM_CardManager_CheckLockCardRequest(cm, card,
                                             LCCL_Client_GetClientId(cl));
    if (rv==1) {
      // TODO: Check timeout
      return rv;
    }
    else {
      if (rv==0) {
        LCS_LOCKMANAGER *lm;
        GWEN_TYPE_UINT32 lrId;

        DBG_INFO(0, "Card lock acquired, now locking slot");
        LCCL_Request_SetUint32Data(req, LCCL_REQ_TAKECARD_STEP2);

        lm=LCDM_DeviceManager_GetLockManager(dm,
                                             LCCO_Card_GetReaderId(card),
                                             LCCO_Card_GetSlotNum(card));
        assert(dm);
        lrId=LCS_LockManager_RequestLock(lm, LCCL_Client_GetClientId(cl),
                                         LCCL_Client_GetMaxClientLockTime(cl),
                                         LCCL_Client_GetMaxClientLocks(cl));
        assert(lrId);
        LCCO_Card_SetLockId(card, lrId);
      }
      else {
        /* error (most likely a timeout) */
        GWEN_IpcRequest_List_Del(req);
        LCS_Server_SendErrorResponse(clm->server,
                                     rid,
                                     -rv,
                                     "Could not acquire lock on card");
        /* remove request from IPC manager */
        if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
          DBG_ERROR(0, "Could not remove request");
          abort();
        }

        /* free request (already removed from RequestManager) */
        GWEN_IpcRequest_free(req);
        req=0;
      }
    }
  } /* if step one */
  else if (step==LCCL_REQ_TAKECARD_STEP2) {
    LCS_LOCKMANAGER *lm;
    GWEN_TYPE_UINT32 lrId;

    DBG_INFO(0, "Step 2");
    lm=LCDM_DeviceManager_GetLockManager(dm,
                                         LCCO_Card_GetReaderId(card),
                                         LCCO_Card_GetSlotNum(card));
    assert(dm);
    lrId=LCCO_Card_GetLockId(card);
    assert(lrId);

    rv=LCS_LockManager_CheckRequest(lm, lrId);
    if (rv==1) {
      // TODO: Check timeout
      return 1; /* nothing done */
    }
    else {
      if (rv==0) {
        GWEN_DB_NODE *dbRsp;
  
        /* succeeded */
        GWEN_IpcRequest_List_Del(req);
  
        /* send response */
        dbRsp=GWEN_DB_Group_new("TakeCardResponse");
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_DEFAULT,
                             "code", "OK");
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_DEFAULT,
                             "text", "The card is yours");
        if (GWEN_IpcManager_SendResponse(clm->ipcManager, rid, dbRsp)) {
          DBG_ERROR(0, "Could not send response to client");
          if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
            DBG_ERROR(0, "Could not remove request");
            abort();
          }
          GWEN_IpcRequest_free(req);
          LCCM_CardManager_UnlockCard(cm, card, LCCL_Client_GetClientId(cl));
          return -1;
        }
      }
      else {
        /* error (most likely a timeout) */
        LCCM_CardManager_UnlockCard(cm, card, LCCL_Client_GetClientId(cl));
        GWEN_IpcRequest_List_Del(req);
        LCS_Server_SendErrorResponse(clm->server,
                                     rid,
                                     -rv,
                                     "Could not acquire lock on slot");
      }
  
      /* remove request from IPC manager */
      if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
        DBG_ERROR(0, "Could not remove request");
        abort();
      }
  
      /* free request (already removed from RequestManager) */
      GWEN_IpcRequest_free(req);
      req=0;
    }
  }
  else {
    DBG_ERROR(0, "Invalid step \"%d\" for TakeCard request", step);
    abort();
  }

  return 0;
}




