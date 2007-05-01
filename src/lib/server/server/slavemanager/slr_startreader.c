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


int LCSL_SlaveManager_HandleStartReader(LCSL_SLAVEMANAGER *slm,
                                        GWEN_TYPE_UINT32 rid,
                                        const char *name,
                                        GWEN_DB_NODE *dbReq) {
  GWEN_IPC_REQUEST *req;
  GWEN_TIME *ti;
  GWEN_TYPE_UINT32 readerId;
  GWEN_TYPE_UINT32 masterReaderId;
  const char *s;
  unsigned int x;
  LCDM_DEVICEMANAGER *dm;
  LCCO_READER_LIST2_ITERATOR *it;
  LCCO_READER *sr=0;

  DBG_INFO(0, "Master: StartReader");

  dm=LCS_Server_GetDeviceManager(slm->server);
  assert(dm);

  readerId=0;
  s=GWEN_DB_GetCharValue(dbReq, "data/driversReaderId", 0, 0);
  if (s && 1==sscanf(s, "%x", &x))
    readerId=x;

  if (readerId==0) {
    DBG_ERROR(0, "Invalid device's reader id");
    LCS_Server_SendErrorResponse(slm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Invalid device's reader id");
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  masterReaderId=0;
  s=GWEN_DB_GetCharValue(dbReq, "data/readerId", 0, 0);
  if (s && 1==sscanf(s, "%x", &x))
    masterReaderId=x;

  if (masterReaderId==0) {
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

  /* get reader from slave pool */
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

  /* store reader id assigned by the master */
  LCSL_Reader_SetMasterReaderId(sr, masterReaderId);

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

  LCSL_Reader_AddFlags(sr, LCSL_READER_FLAGS_STARTED);
  LCSL_Reader_DelFlags(sr, LCSL_READER_FLAGS_STOPPED);
  LCDM_DeviceManager_BeginUseReader(dm, LCCO_Reader_GetReaderId(sr));

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
  GWEN_IpcRequest_SetWorkFn(req, LCSL_SlaveManager_WorkStartReader);
  GWEN_IpcRequest_SetStatus(req, GWEN_IpcRequest_StatusPartial);

  /* add incoming request */
  GWEN_IpcRequestManager_AddRequest(LCS_Server_GetRequestManager(slm->server),
                                    req);

  /* do not remove the request now, since it is enqueued */
  DBG_DEBUG(0, "Enqueued StartReader request");
  return 0; /* handled */
}



int LCSL_SlaveManager_WorkStartReader(GWEN_IPC_REQUEST *req) {
  LCSL_SLAVEMANAGER *slm;
  LCDM_DEVICEMANAGER *dm;
  GWEN_TYPE_UINT32 rid;
  LC_READER_STATUS rst;
  GWEN_TYPE_UINT32 readerId;
  LCCO_READER *sr;

  rid=GWEN_IpcRequest_GetId(req);
  assert(rid);

  slm=LCSL_Request_GetSlaveManager(req);
  assert(slm);

  dm=LCS_Server_GetDeviceManager(slm->server);
  assert(dm);

  readerId=LCSL_Request_GetUint32Data(req);
  assert(readerId);

  sr=LCSL_Request_GetReader(req);
  assert(sr);

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

  rst=LCCO_Reader_GetStatus(sr);
  if (rst==LC_ReaderStatusUp) {
    GWEN_DB_NODE *dbClientResponse;

    DBG_DEBUG(0, "Sending response to StartReader");
    dbClientResponse=GWEN_DB_Group_new("Driver_StartReaderResponse");
    GWEN_DB_SetIntValue(dbClientResponse,
                        GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "code", 0);
    GWEN_DB_SetCharValue(dbClientResponse,
                         GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", "Reader up");

    /* send response */
    if (GWEN_IpcManager_SendResponse(slm->ipcManager,
                                     rid,
                                     dbClientResponse)) {
      DBG_ERROR(0, "Could not send CommandCard response");
      if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
        DBG_ERROR(0, "Could not remove request");
        abort();
      }
      LCDM_DeviceManager_EndUseReader(dm, LCCO_Reader_GetReaderId(sr));
      LCSL_Reader_DelFlags(sr, LCSL_READER_FLAGS_STARTED);
      GWEN_IpcRequest_List_Del(req);
      GWEN_IpcRequest_free(req);
      return -1;
    }

    /* remove IpcManager requests and IpcRequests */
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    GWEN_IpcRequest_List_Del(req);
    GWEN_IpcRequest_free(req);
    return 0; /* something done */
  }
  else {
    GWEN_TIME *ti;
    const GWEN_TIME *tiExp;

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
      LCDM_DeviceManager_EndUseReader(dm, LCCO_Reader_GetReaderId(sr));
      LCSL_Reader_DelFlags(sr, LCSL_READER_FLAGS_STARTED);
      GWEN_IpcRequest_List_Del(req);
      GWEN_IpcRequest_free(req);
      return 0; /* something done */
    }
    else
      GWEN_Time_free(ti);
  }

  return 1; /* nothing done */
}




