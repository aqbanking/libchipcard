

uint32_t LC_ClientLcc_SendLockReader(LC_CLIENT *cl,
                                             uint32_t serverId,
                                             uint32_t readerId){
  GWEN_DB_NODE *dbReq;
  uint32_t rqid;
  char numbuf[16];

  dbReq=GWEN_DB_Group_new("Client_LockReader");
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", readerId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerid", numbuf);

  /* send request */
  rqid=LC_ClientLcc_SendRequest(cl, 0, serverId, dbReq);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    return 0;
  }

  return rqid;
}



LC_CLIENT_RESULT LC_ClientLcc_CheckLockReader(LC_CLIENT *cl,
                                              uint32_t rid,
                                              uint32_t *lockId){
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbRsp;
  int err;
  const char *code;
  const char *text;

  assert(cl);
  assert(lockId);
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

  code=GWEN_DB_GetCharValue(dbRsp, "data/code", 0, "ERROR");
  text=GWEN_DB_GetCharValue(dbRsp, "data/text", 0, "(none)");
  DBG_NOTICE(LC_LOGDOMAIN, "LockReader result: %s (%s)", code, text);
  if (strcasecmp(code, "OK")==0) {
    int i;

    if (1!=sscanf(GWEN_DB_GetCharValue(dbRsp, "data/lockid", 0, "0"),
                  "%x", &i)) {
      DBG_ERROR(LC_LOGDOMAIN, "Bad server message");
      res=LC_Client_ResultCmdError;
    }
    else {
      assert(i);
      *lockId=i;
      res=LC_Client_ResultOk;
    }
  }
  else
    res=LC_Client_ResultGeneric;
  GWEN_DB_Group_free(dbRsp);
  return res;
}




LC_CLIENT_RESULT LC_ClientLcc_LockReader(LC_CLIENT *cl,
                                         uint32_t serverId,
                                         uint32_t readerId,
                                         uint32_t *lockId) {
  uint32_t rqid;
  LC_CLIENT_RESULT res;

  rqid=LC_ClientLcc_SendLockReader(cl, serverId, readerId);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"lockReader\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_ClientLcc_WaitForNextResponse(cl, rqid, NULL, LC_Client_GetVeryLongTimeout(cl));
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"lockReader\"");
    }
    LC_ClientLcc_DeleteRequest(cl, rqid);
    return res;
  }
  res=LC_ClientLcc_CheckLockReader(cl, rqid, lockId);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"lockReader\"");
    LC_ClientLcc_DeleteRequest(cl, rqid);
    return res;
  }

  LC_ClientLcc_DeleteRequest(cl, rqid);

  return LC_Client_ResultOk;
}





uint32_t LC_ClientLcc_SendUnlockReader(LC_CLIENT *cl,
				       uint32_t serverId,
				       uint32_t readerId,
				       uint32_t lockId){
  GWEN_DB_NODE *dbReq;
  uint32_t rqid;
  char numbuf[16];

  dbReq=GWEN_DB_Group_new("Client_UnlockReader");
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", readerId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerid", numbuf);
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", lockId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "lockid", numbuf);

  /* send request */
  rqid=LC_ClientLcc_SendRequest(cl, 0, serverId, dbReq);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    return 0;
  }

  return rqid;
}



LC_CLIENT_RESULT LC_ClientLcc_CheckUnlockReader(LC_CLIENT *cl,
                                                uint32_t rid){
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbRsp;
  int err;
  const char *code;
  const char *text;

  assert(cl);
  assert(rid);
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

  code=GWEN_DB_GetCharValue(dbRsp, "data/code", 0, "ERROR");
  text=GWEN_DB_GetCharValue(dbRsp, "data/text", 0, "(none)");
  DBG_DEBUG(LC_LOGDOMAIN, "UnlockReader result: %s (%s)", code, text);
  if (strcasecmp(code, "OK")==0) {
    res=LC_Client_ResultOk;
  }
  else
    res=LC_Client_ResultGeneric;
  GWEN_DB_Group_free(dbRsp);
  return res;
}



LC_CLIENT_RESULT LC_Client_UnlockReader(LC_CLIENT *cl,
                                        uint32_t serverId,
                                        uint32_t readerId,
                                        uint32_t lockId){
  uint32_t rqid;
  LC_CLIENT_RESULT res;

  rqid=LC_ClientLcc_SendUnlockReader(cl, serverId, readerId, lockId);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"UnlockReader\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_ClientLcc_WaitForNextResponse(cl, rqid, NULL, LC_Client_GetShortTimeout(cl));
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"UnlockReader\"");
    }
    LC_ClientLcc_DeleteRequest(cl, rqid);
    return res;
  }
  res=LC_ClientLcc_CheckUnlockReader(cl, rqid);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"UnlockReader\"");
    LC_ClientLcc_DeleteRequest(cl, rqid);
    return res;
  }

  LC_ClientLcc_DeleteRequest(cl, rqid);

  return LC_Client_ResultOk;
}







uint32_t LC_ClientLcc_SendReaderCommand(LC_CLIENT *cl,
					uint32_t serverId,
					uint32_t readerId,
					uint32_t lockId,
					GWEN_DB_NODE *dbData){
  GWEN_DB_NODE *dbReq;
  uint32_t rqid;
  char numbuf[16];

  assert(cl);
  assert(serverId);
  assert(readerId);
  assert(lockId);
  dbReq=GWEN_DB_Group_new("Client_ReaderCommand");
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", readerId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "readerId", numbuf);
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", lockId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "lockId", numbuf);

  if (dbData) {
    GWEN_DB_NODE *dbCommandCommand;

    dbCommandCommand=GWEN_DB_GetGroup(dbReq, GWEN_DB_FLAGS_DEFAULT,
                                   "command");
    GWEN_DB_AddGroupChildren(dbCommandCommand, dbData);
  }

  /* send request */
  rqid=LC_ClientLcc_SendRequest(cl, 0, serverId, dbReq);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    return 0;
  }

  return rqid;
}



LC_CLIENT_RESULT LC_ClientLcc_CheckReaderCommand(LC_CLIENT *cl,
                                                 uint32_t rid,
                                                 GWEN_DB_NODE *dbCmdResp){
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
  else {
    if (dbCmdResp) {
      GWEN_DB_NODE *dbAnswer;

      dbAnswer=GWEN_DB_GetGroup(dbRsp, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                                "data/command");
      if (dbAnswer)
        GWEN_DB_AddGroupChildren(dbCmdResp, dbAnswer);
    }
  }

  GWEN_DB_Group_free(dbRsp);
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_ClientLcc_ReaderCommand(LC_CLIENT *cl,
                                            uint32_t serverId,
                                            uint32_t readerId,
                                            uint32_t lockId,
                                            GWEN_DB_NODE *dbData,
                                            GWEN_DB_NODE *dbCmdResp) {
  uint32_t rqid;
  LC_CLIENT_RESULT res;

  assert(cl);
  assert(serverId);
  assert(readerId);
  assert(lockId);
  rqid=LC_ClientLcc_SendReaderCommand(cl, serverId, readerId, lockId, dbData);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"ReaderCommand\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_ClientLcc_WaitForNextResponse(cl, rqid, NULL, LC_Client_GetLongTimeout(cl));
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"ReaderCommand\"");
    }
    LC_ClientLcc_DeleteRequest(cl, rqid);
    return res;
  }
  res=LC_ClientLcc_CheckReaderCommand(cl, rqid, dbCmdResp);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"ReaderCommand\"");
    LC_ClientLcc_DeleteRequest(cl, rqid);
    return res;
  }

  LC_ClientLcc_DeleteRequest(cl, rqid);
  return LC_Client_ResultOk;
}






