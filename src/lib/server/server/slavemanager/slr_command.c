/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: clr_execapdu.c 211 2006-09-07 23:57:04Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

/* included by slavemanager.c */


int LCSL_SlaveManager_HandleCardCommand(LCSL_SLAVEMANAGER *slm,
                                        GWEN_TYPE_UINT32 rid,
                                        const char *name,
                                        GWEN_DB_NODE *dbReq) {
  GWEN_TYPE_UINT32 readerId;
  const char *s;
  unsigned int x;
  LCDM_DEVICEMANAGER *dm;
  GWEN_TYPE_UINT32 cardId;
  LCCM_CARDMANAGER *cm;
  LCCO_CARD *card;
  GWEN_IPC_REQUEST *req;
  GWEN_IPC_REQUEST *dreq;
  GWEN_TIME *ti;
  GWEN_TYPE_UINT32 outRid;
  GWEN_DB_NODE *dbOutReq;
  const void *p;
  unsigned int bs;
  const char *target;
  LCCO_READER_LIST2_ITERATOR *it;
  LCCO_READER *sr=0;

  DBG_INFO(0, "Master: CardCommand");

  dm=LCS_Server_GetDeviceManager(slm->server);
  assert(dm);

  cm=LCS_Server_GetCardManager(slm->server);
  assert(cm);

  /* get reader id */
  readerId=0;
  s=GWEN_DB_GetCharValue(dbReq, "data/driversReaderId", 0, 0);
  if (s && 1==sscanf(s, "%x", &x))
    readerId=x;
  if (readerId==0) {
    DBG_ERROR(0, "Invalid reader id");
    LCS_Server_SendErrorResponse(slm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Invalid reader id");
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* get card id */
  cardId=GWEN_DB_GetIntValue(dbReq, "data/cardNum", 0, 0);
  if (cardId==0) {
    DBG_ERROR(0, "Missing card id");
    LCS_Server_SendErrorResponse(slm->server, rid,
				 LC_ERROR_INVALID,
				 "Missing card id");
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* get slave reader object */
  it=LCCO_Reader_List2_First(slm->slaveReaders);
  if (it) {
    sr=LCCO_Reader_List2Iterator_Data(it);
    assert(sr);
    while(sr) {
      if (LCSL_Reader_GetSlaveReaderId(sr)==readerId)
        break;
      sr=LCCO_Reader_List2Iterator_Next(it);
    }
    LCCO_Reader_List2Iterator_free(it);
  }
  if (!sr) {
    DBG_ERROR(0, "Reader \"%08x\" not found", readerId);
    LCS_Server_SendErrorResponse(slm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Reader not found");
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }
  if (!sr) {
    DBG_ERROR(0, "Reader \"%08x\" not found", readerId);
    LCS_Server_SendErrorResponse(slm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Reader not found");
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* check whether the reader is available */
  if (!LCCO_Reader_IsAvailable(sr)) {
    DBG_ERROR(0, "Reader \"%08x\" unplugged", readerId);
    LCS_Server_SendErrorResponse(slm->server, rid,
                                 LC_ERROR_READER_REMOVED,
                                 "Reader unplugged");
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* check whether the reader has been started */
  if (!(LCSL_Reader_GetFlags(sr) & LCSL_READER_FLAGS_STARTED)) {
    DBG_ERROR(0, "Reader \"%08x\" has not been started", readerId);
    LCS_Server_SendErrorResponse(slm->server, rid,
                                 LC_ERROR_READER_REMOVED,
                                 "Reader has not been started");
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    if (LCSL_Reader_GetFlags(sr) & LCSL_READER_FLAGS_STARTED) {
      LCDM_DeviceManager_EndUseReader(dm, LCCO_Reader_GetReaderId(sr));
      LCSL_Reader_DelFlags(sr, LCSL_READER_FLAGS_STARTED);
    }
    return -1;
  }

  /* get referenced card */
  DBG_DEBUG(0, "Looking for card \"%08x\"", cardId);
  card=LCCM_CardManager_FindCard(cm, cardId);
  if (!card) {
    DBG_ERROR(0, "Card \"%08x\" not found", cardId);
    LCS_Server_SendErrorResponse(slm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Card not found");
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* check whether card is still inserted */
  if (LCCO_Card_GetStatus(card)!=LC_CardStatusInserted) {
    DBG_ERROR(0, "Card has been removed");
    LCS_Server_SendErrorResponse(slm->server, rid,
                                 LC_ERROR_CARD_REMOVED,
                                 "Card has been removed");
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* get APDU */
  p=GWEN_DB_GetBinValue(dbReq, "data/data", 0, 0, 0, &bs);
  if (!p || !bs) {
    DBG_ERROR(0, "No data given");
    LCS_Server_SendErrorResponse(slm->server, rid,
                                 LC_ERROR_INVALID,
                                 "No data given");
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* get target */
  target=GWEN_DB_GetCharValue(dbReq, "data/target", 0, 0);
  if (!target) {
    DBG_ERROR(0, "No target given");
    LCS_Server_SendErrorResponse(slm->server, rid,
                                 LC_ERROR_INVALID,
                                 "No target given");
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* create outbound request */
  dbOutReq=GWEN_DB_Group_new("Driver_CardCommand");
  GWEN_DB_SetBinValue(dbOutReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "data", p, bs);
  GWEN_DB_SetCharValue(dbOutReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "target", target);
  outRid=LCDM_DeviceManager_SendCardCommand(dm, card, dbOutReq);
  if (outRid==0) {
    DBG_ERROR(0, "Could not send command to reader");
    LCS_Server_SendErrorResponse(slm->server, rid,
				 LC_ERROR_GENERIC,
				 "Could not send command to reader");
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* create request */
  req=LCSL_Request_new();
  assert(req);
  GWEN_IpcRequest_SetId(req, rid);
  GWEN_IpcRequest_SetIpcId(req, slm->ipcId);
  GWEN_IpcRequest_SetName(req, name);
  LCSL_Request_SetUint32Data(req, readerId);
  LCSL_Request_SetReader(req, sr);
  ti=GWEN_CurrentTime();
  assert(ti);
  GWEN_Time_AddSeconds(ti, slm->commandTimeout);
  GWEN_IpcRequest_SetExpires(req, ti);
  GWEN_Time_free(ti);
  LCSL_Request_SetSlaveManager(req, slm);
  LCSL_Request_SetCard(req, card);
  GWEN_IpcRequest_SetWorkFn(req, LCSL_SlaveManager_WorkCardCommand);
  GWEN_IpcRequest_SetStatus(req, GWEN_IpcRequest_StatusPartial);

  /* create subrequest, add it to request */
  dreq=LCSL_Request_new();
  GWEN_IpcRequest_SetId(dreq, outRid);
  GWEN_IpcRequest_SetName(dreq, "Driver_CardCommand");
  LCSL_Request_SetUint32Data(req, readerId);
  ti=GWEN_CurrentTime();
  assert(ti);
  GWEN_Time_AddSeconds(ti, slm->commandTimeout);
  GWEN_IpcRequest_SetExpires(dreq, ti);
  GWEN_Time_free(ti);
  LCSL_Request_SetSlaveManager(dreq, slm);
  LCSL_Request_SetCard(dreq, card);
  GWEN_IpcRequest_SetStatus(dreq, GWEN_IpcRequest_StatusSent);
  GWEN_IpcRequest_List_Add(dreq, GWEN_IpcRequest_GetSubRequests(req));

  /* add incoming request */
  GWEN_IpcRequestManager_AddRequest(LCS_Server_GetRequestManager(slm->server),
				    req);

  /* do not remove the request now, since it is enqueued */
  DBG_DEBUG(0,
            "Enqueued CardCommand request for card \"%08x\"", cardId);
  return 0; /* handled */
}



int LCSL_SlaveManager_WorkCardCommand(GWEN_IPC_REQUEST *req) {
  LCSL_SLAVEMANAGER *slm;
  LCDM_DEVICEMANAGER *dm;
  GWEN_TYPE_UINT32 rid;
  LCCO_CARD *card;
  GWEN_TYPE_UINT32 drid;
  GWEN_IPC_REQUEST *dreq;
  GWEN_DB_NODE *dbDriverResponse;
  GWEN_TYPE_UINT32 readerId;

  rid=GWEN_IpcRequest_GetId(req);
  assert(rid);

  slm=LCSL_Request_GetSlaveManager(req);
  assert(slm);

  dm=LCS_Server_GetDeviceManager(slm->server);
  assert(dm);

  card=LCSL_Request_GetCard(req);
  assert(card);

  readerId=LCSL_Request_GetUint32Data(req);
  assert(readerId);

  dreq=GWEN_IpcRequest_List_First(GWEN_IpcRequest_GetSubRequests(req));
  assert(dreq);
  drid=GWEN_IpcRequest_GetId(dreq);

  dbDriverResponse=GWEN_IpcManager_GetResponseData(slm->ipcManager, drid);
  if (dbDriverResponse) {
    GWEN_DB_NODE *dbClientResponse;
    const char *p;
    const void *bp;
    unsigned int bs;
    int code;

    DBG_DEBUG(0, "Sending response to CommandCard");
    dbClientResponse=GWEN_DB_Group_new("Driver_CommandCardResponse");
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
    if (GWEN_IpcManager_SendResponse(slm->ipcManager,
                                     rid,
				     dbClientResponse)) {
      DBG_ERROR(0, "Could not send CommandCard response");
      if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, drid, 1)) {
        DBG_ERROR(0, "Could not remove request");
        abort();
      }
      if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
        DBG_ERROR(0, "Could not remove request");
        abort();
      }
      GWEN_IpcRequest_List_Del(req);
      GWEN_IpcRequest_free(req);
      return -1;
    }

    /* remove IpcManager requests and IpcRequests */
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, drid, 1)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    GWEN_IpcRequest_List_Del(req);
    GWEN_IpcRequest_free(req);
    return 0;
  }
  else {
    LCCO_READER *sr;
    GWEN_TIME *ti;
    const GWEN_TIME *tiExp;

    /* no response, check whether a response is possible at all */
    /* check card status */
    if (LCCO_Card_GetStatus(card)!=LC_CardStatusInserted) {
      DBG_ERROR(0, "Card has been removed");
      LCS_Server_SendErrorResponse(slm->server, rid,
                                   LC_ERROR_CARD_REMOVED,
                                   "Card has been removed");
      if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
        DBG_WARN(0, "Could not remove request");
        abort();
      }
      GWEN_IpcRequest_List_Del(req);
      GWEN_IpcRequest_free(req);
      return -1;
    }

    /* check reader status */
    sr=LCSL_Request_GetReader(req);
    assert(sr);

    /* reader still available? */
    if (!LCCO_Reader_IsAvailable(sr)) {
      DBG_ERROR(0, "Reader \"%08x\" unplugged", readerId);
      LCS_Server_SendErrorResponse(slm->server, rid,
                                   LC_ERROR_READER_REMOVED,
                                   "Reader unplugged");
      if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
        DBG_WARN(0, "Could not remove request");
        abort();
      }
      if (LCSL_Reader_GetFlags(sr) & LCSL_READER_FLAGS_STARTED) {
        LCDM_DeviceManager_EndUseReader(dm, LCCO_Reader_GetReaderId(sr));
        LCSL_Reader_DelFlags(sr, LCSL_READER_FLAGS_STARTED);
      }
      GWEN_IpcRequest_List_Del(req);
      GWEN_IpcRequest_free(req);
      return -1;
    }

    ti=GWEN_CurrentTime();
    assert(ti);
    tiExp=GWEN_IpcRequest_GetExpires(req);
    if (GWEN_Time_DiffSeconds(ti, tiExp)>0) {
      DBG_INFO(0, "StartReader request timed out");
      GWEN_Time_free(ti);

      LCS_Server_SendErrorResponse(slm->server, rid,
                                   LC_ERROR_TIMEOUT,
                                   "Request timed out");

      /* remove IpcManager requests and IpcRequests */
      if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
        DBG_ERROR(0, "Could not remove request");
        abort();
      }
      GWEN_IpcRequest_List_Del(req);
      GWEN_IpcRequest_free(req);
      return 0; /* something done */
    }
    else
      GWEN_Time_free(ti);

  }

  return 1; /* nothing done */
}






