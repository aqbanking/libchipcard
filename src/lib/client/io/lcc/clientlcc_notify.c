

uint32_t LC_ClientLcc_SendSetNotify(LC_CLIENT *cl,
                                            uint32_t flags){
  GWEN_DB_NODE *db;
  uint32_t rqid;

  db=GWEN_DB_Group_new("Client_SetNotify");

  /* driver flags */
  if (flags & LC_NOTIFY_FLAGS_DRIVER_START)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_DRIVER":"LC_NOTIFY_CODE_DRIVER_START);
  if (flags & LC_NOTIFY_FLAGS_DRIVER_UP)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_DRIVER":"LC_NOTIFY_CODE_DRIVER_UP);
  if (flags & LC_NOTIFY_FLAGS_DRIVER_DOWN)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_DRIVER":"LC_NOTIFY_CODE_DRIVER_DOWN);
  if (flags & LC_NOTIFY_FLAGS_DRIVER_ERROR)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_DRIVER":"LC_NOTIFY_CODE_DRIVER_ERROR);
  if (flags & LC_NOTIFY_FLAGS_DRIVER_ADD)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
                         LC_NOTIFY_TYPE_DRIVER":"LC_NOTIFY_CODE_DRIVER_ADD);
  if (flags & LC_NOTIFY_FLAGS_DRIVER_DEL)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
                         LC_NOTIFY_TYPE_DRIVER":"LC_NOTIFY_CODE_DRIVER_DEL);

  /* reader flags */
  if (flags & LC_NOTIFY_FLAGS_READER_START)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_READER":"LC_NOTIFY_CODE_READER_START);
  if (flags & LC_NOTIFY_FLAGS_READER_UP)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_READER":"LC_NOTIFY_CODE_READER_UP);
  if (flags & LC_NOTIFY_FLAGS_READER_DOWN)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_READER":"LC_NOTIFY_CODE_READER_DOWN);
  if (flags & LC_NOTIFY_FLAGS_READER_ERROR)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_READER":"LC_NOTIFY_CODE_READER_ERROR);
  if (flags & LC_NOTIFY_FLAGS_READER_ADD)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_READER":"LC_NOTIFY_CODE_READER_ADD);
  if (flags & LC_NOTIFY_FLAGS_READER_DEL)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_READER":"LC_NOTIFY_CODE_READER_DEL);

  /* service flags */
  if (flags & LC_NOTIFY_FLAGS_SERVICE_START)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_SERVICE":"LC_NOTIFY_CODE_SERVICE_START);
  if (flags & LC_NOTIFY_FLAGS_SERVICE_UP)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_SERVICE":"LC_NOTIFY_CODE_SERVICE_UP);
  if (flags & LC_NOTIFY_FLAGS_SERVICE_DOWN)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_SERVICE":"LC_NOTIFY_CODE_SERVICE_DOWN);
  if (flags & LC_NOTIFY_FLAGS_SERVICE_ERROR)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_SERVICE":"LC_NOTIFY_CODE_SERVICE_ERROR);

  /* card flags */
  if (flags & LC_NOTIFY_FLAGS_CARD_INSERTED)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_CARD":"LC_NOTIFY_CODE_CARD_INSERTED);
  if (flags & LC_NOTIFY_FLAGS_CARD_REMOVED)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_CARD":"LC_NOTIFY_CODE_CARD_REMOVED);

  /* client flags */
  if (flags & LC_NOTIFY_FLAGS_CLIENT_UP)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_CLIENT":"LC_NOTIFY_CODE_CLIENT_UP);
  if (flags & LC_NOTIFY_FLAGS_CLIENT_DOWN)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_CLIENT":"LC_NOTIFY_CODE_CLIENT_DOWN);
  if (flags & LC_NOTIFY_FLAGS_CLIENT_STARTWAIT)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_CLIENT":"LC_NOTIFY_CODE_CLIENT_STARTWAIT);
  if (flags & LC_NOTIFY_FLAGS_CLIENT_STOPWAIT)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_CLIENT":"LC_NOTIFY_CODE_CLIENT_STOPWAIT);
  if (flags & LC_NOTIFY_FLAGS_CLIENT_TAKECARD)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_CLIENT":"
			 LC_NOTIFY_CODE_CLIENT_TAKECARD);
  if (flags & LC_NOTIFY_FLAGS_CLIENT_GOTCARD)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_CLIENT":"
			 LC_NOTIFY_CODE_CLIENT_GOTCARD);
  if (flags & LC_NOTIFY_FLAGS_CLIENT_CMDSEND)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_CLIENT":"
			 LC_NOTIFY_CODE_CLIENT_CMDSEND);
  if (flags & LC_NOTIFY_FLAGS_CLIENT_CMDRECV)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_CLIENT":"
			 LC_NOTIFY_CODE_CLIENT_CMDRECV);


  /* send request */
  rqid=LC_ClientLcc_SendRequest(cl, 0, 0, db);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    return 0;
  }

  return rqid;
}



LC_CLIENT_RESULT LC_ClientLcc_CheckSetNotify(LC_CLIENT *cl,
                                             uint32_t rid){
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbRsp;
  int err;

  res=LC_ClientLcc_CheckResponse(cl, rid);
  if (res!=LC_Client_ResultOk)
    return res;

  dbRsp=LC_ClientLcc_GetNextResponse(cl, rid);
  assert(dbRsp);

  err=LC_ClientLcc_CheckForError(dbRsp);
  if (err) {
    if (err>(int)GWEN_IPC_ERROR_CODES) {
      DBG_ERROR(LC_LOGDOMAIN, "IPC error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultIpcError;
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "Command error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultCmdError;
    }
  }

  GWEN_DB_Group_free(dbRsp);
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_ClientLcc_SetNotify(LC_CLIENT *cl, uint32_t flags){
  uint32_t rqid;
  LC_CLIENT_RESULT res;

  rqid=LC_ClientLcc_SendSetNotify(cl, flags);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"setNotify\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_ClientLcc_WaitForNextResponse(cl, rqid, NULL, LC_Client_GetShortTimeout(cl));
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"setNotify\"");
    }
    LC_ClientLcc_DeleteRequest(cl, rqid);
    return res;
  }
  res=LC_ClientLcc_CheckSetNotify(cl, rqid);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"setNotify\"");
    LC_ClientLcc_DeleteRequest(cl, rqid);
    return res;
  }

  LC_ClientLcc_DeleteRequest(cl, rqid);

  return LC_Client_ResultOk;
}



int LC_ClientLcc_HandleNotification(LC_CLIENT *cl, GWEN_DB_NODE *dbReq){
  uint32_t serverId;
  const char *clientId;
  const char *ntype;
  const char *ncode;
  GWEN_DB_NODE *dbData;
  LC_NOTIFICATION *n;

  assert(cl);

  serverId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  clientId=GWEN_DB_GetCharValue(dbReq, "data/clientid", 0, "0");

  ntype=GWEN_DB_GetCharValue(dbReq, "data/ntype", 0, 0);
  ncode=GWEN_DB_GetCharValue(dbReq, "data/ncode", 0, 0);
  dbData=GWEN_DB_GetGroup(dbReq, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "data/data");
  if (!ntype) {
    DBG_ERROR(0, "Bad server message (no ntype)");
    return -1;
  }
  if (!ncode) {
    DBG_ERROR(0, "Bad server message (no ncode)");
    return -1;
  }

  n=LC_Notification_new(serverId, clientId, ntype, ncode, dbData);
  assert(n);
  if (LC_Client_HandleNotification(cl, n)) {
    DBG_INFO(LC_LOGDOMAIN, "Error handling notification");
  }
  LC_Notification_free(n);
  return 0;
}






