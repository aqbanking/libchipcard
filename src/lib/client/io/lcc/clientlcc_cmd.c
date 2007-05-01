/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: card.c 187 2006-06-15 16:13:23Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/



GWEN_TYPE_UINT32 LC_ClientLcc_SendStartWait(LC_CLIENT *cl,
                                            GWEN_TYPE_UINT32 rflags,
                                            GWEN_TYPE_UINT32 rmask){
  GWEN_DB_NODE *db;
  GWEN_TYPE_UINT32 rqid;

  db=GWEN_DB_Group_new("Client_StartWait");

  /* set rflags */
  if (rflags & LC_READER_FLAGS_KEYPAD)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "flags", "KEYPAD");
  if (rflags & LC_READER_FLAGS_DISPLAY)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "flags", "DISPLAY");
  if (rflags & LC_READER_FLAGS_NOINFO)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "flags", "NOINFO");
  if (rflags & LC_READER_FLAGS_REMOTE)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "flags", "REMOTE");
  if (rflags & LC_READER_FLAGS_AUTO)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "flags", "AUTO");

  /* set rmask */
  if (rmask & LC_READER_FLAGS_KEYPAD)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "mask", "KEYPAD");
  if (rmask & LC_READER_FLAGS_DISPLAY)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "mask", "DISPLAY");
  if (rmask & LC_READER_FLAGS_NOINFO)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "mask", "NOINFO");
  if (rmask & LC_READER_FLAGS_REMOTE)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "mask", "REMOTE");
  if (rmask & LC_READER_FLAGS_AUTO)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "mask", "AUTO");


  /* send request */
  rqid=LC_ClientLcc_SendRequest(cl, 0, 0, db);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    return 0;
  }

  return rqid;
}



LC_CLIENT_RESULT
LC_ClientLcc_CheckStartWait(LC_CLIENT *cl,
                            GWEN_TYPE_UINT32 rid){
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbRsp;

  res=LC_ClientLcc_CheckResponse(cl, rid);
  if (res!=LC_Client_ResultOk)
    return res;

  dbRsp=LC_ClientLcc_GetNextResponse(cl, rid);
  assert(dbRsp);

  res=LC_ClientLcc_CheckForError(dbRsp);
  if (res!=LC_Client_ResultOk) {
    GWEN_DB_Group_free(dbRsp);
    return res;
  }

  GWEN_DB_Group_free(dbRsp);
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_ClientLcc_StartWait(LC_CLIENT *cl,
                                        GWEN_TYPE_UINT32 rflags,
                                        GWEN_TYPE_UINT32 rmask) {
  GWEN_TYPE_UINT32 rqid;
  LC_CLIENT_RESULT res;

  rqid=LC_ClientLcc_SendStartWait(cl, rflags, rmask);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"startWait\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_ClientLcc_CheckResponse_Wait(cl, rqid,
                                      LC_Client_GetShortTimeout(cl));
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
      LC_ClientLcc_DeleteRequest(cl, rqid);
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"startWait\"");
    }
    return res;
  }
  res=LC_ClientLcc_CheckStartWait(cl, rqid);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"startWait\"");
    LC_ClientLcc_DeleteRequest(cl, rqid);
    return res;
  }

  LC_ClientLcc_DeleteRequest(cl, rqid);

  return LC_Client_ResultOk;
}






GWEN_TYPE_UINT32 LC_ClientLcc_SendStopWait(LC_CLIENT *cl) {
  GWEN_DB_NODE *dbReq;
  GWEN_TYPE_UINT32 rqid;

  dbReq=GWEN_DB_Group_new("Client_StopWait");

  /* send request */
  rqid=LC_ClientLcc_SendRequest(cl, 0, 0, dbReq);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    return 0;
  }

  return rqid;
}



LC_CLIENT_RESULT
LC_ClientLcc_CheckStopWait(LC_CLIENT *cl,
                           GWEN_TYPE_UINT32 rid){
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbRsp;

  res=LC_ClientLcc_CheckResponse(cl, rid);
  if (res!=LC_Client_ResultOk)
    return res;

  dbRsp=LC_ClientLcc_GetNextResponse(cl, rid);
  assert(dbRsp);

  res=LC_ClientLcc_CheckForError(dbRsp);
  if (res!=LC_Client_ResultOk) {
    GWEN_DB_Group_free(dbRsp);
    return res;
  }

  GWEN_DB_Group_free(dbRsp);
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_ClientLcc_StopWait(LC_CLIENT *cl) {
  GWEN_TYPE_UINT32 rqid;
  LC_CLIENT_RESULT res;

  rqid=LC_ClientLcc_SendStopWait(cl);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"stopWait\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_ClientLcc_CheckResponse_Wait(cl, rqid,
                                      LC_Client_GetShortTimeout(cl));
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
      LC_ClientLcc_DeleteRequest(cl, rqid);
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"stopWait\"");
    }
    return res;
  }
  res=LC_ClientLcc_CheckStopWait(cl, rqid);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"stopWait\"");
    LC_ClientLcc_DeleteRequest(cl, rqid);
    return res;
  }

  LC_ClientLcc_DeleteRequest(cl, rqid);

  return LC_Client_ResultOk;
}






GWEN_TYPE_UINT32 LC_ClientLcc_SendTakeCard(LC_CLIENT *cl, LC_CARD *cd) {
  GWEN_DB_NODE *dbReq;
  GWEN_TYPE_UINT32 rqid;
  char numbuf[16];

  dbReq=GWEN_DB_Group_new("Client_TakeCard");
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", LC_Card_GetCardId(cd));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "cardid", numbuf);

  /* send request */
  DBG_DEBUG(LC_LOGDOMAIN, "Sending take card request to %08x",
            LC_CardLcc_GetServerId(cd));
  rqid=LC_ClientLcc_SendRequest(cl, cd, LC_CardLcc_GetServerId(cd), dbReq);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    return 0;
  }

  return rqid;
}



LC_CLIENT_RESULT
LC_ClientLcc_CheckTakeCard(LC_CLIENT *cl,
                           GWEN_TYPE_UINT32 rid){
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbRsp;
  LC_REQUEST *rq;
  LC_CARD *card;

  rq=LC_ClientLcc_FindWorkingRequest(cl, rid);
  if (!rq) {
    if (LC_ClientLcc_FindWaitingRequest(cl, rid)) {
      DBG_INFO(LC_LOGDOMAIN, "Request not yet sent");
      return LC_Client_ResultWait;
    }
    DBG_ERROR(LC_LOGDOMAIN, "Request not found");
    return LC_Client_ResultIpcError;
  }
  card=LC_Request_GetCard(rq);
  assert(card);

  res=LC_ClientLcc_CheckResponse(cl, rid);
  if (res!=LC_Client_ResultOk)
    return res;

  dbRsp=LC_ClientLcc_GetNextResponse(cl, rid);
  assert(dbRsp);

  res=LC_ClientLcc_CheckForError(dbRsp);
  if (res!=LC_Client_ResultOk) {
    GWEN_DB_Group_free(dbRsp);
    return res;
  }

  GWEN_DB_Group_free(dbRsp);
  LC_CardLcc_SetConnected(card, 1);
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_ClientLcc_TakeCard(LC_CLIENT *cl, LC_CARD *cd) {
  GWEN_TYPE_UINT32 rqid;
  LC_CLIENT_RESULT res;

  rqid=LC_ClientLcc_SendTakeCard(cl, cd);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"takeCard\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_ClientLcc_CheckResponse_Wait(cl, rqid,
                                      LC_Client_GetVeryLongTimeout(cl));
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
      LC_ClientLcc_DeleteRequest(cl, rqid);
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"takeCard\"");
    }
    return res;
  }
  res=LC_ClientLcc_CheckTakeCard(cl, rqid);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"takeCard\"");
    LC_ClientLcc_DeleteRequest(cl, rqid);
    return res;
  }

  LC_ClientLcc_DeleteRequest(cl, rqid);

  return LC_Client_ResultOk;
}






GWEN_TYPE_UINT32 LC_ClientLcc_SendReleaseCard(LC_CLIENT *cl, LC_CARD *cd){
  GWEN_DB_NODE *dbReq;
  GWEN_TYPE_UINT32 rqid;
  char numbuf[16];

  dbReq=GWEN_DB_Group_new("Client_ReleaseCard");
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", LC_Card_GetCardId(cd));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "cardid", numbuf);

  /* send request */
  rqid=LC_ClientLcc_SendRequest(cl, cd, LC_CardLcc_GetServerId(cd), dbReq);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    return 0;
  }

  LC_CardLcc_SetConnected(cd, 0);
  return rqid;
}



LC_CLIENT_RESULT
LC_ClientLcc_CheckReleaseCard(LC_CLIENT *cl,
                              GWEN_TYPE_UINT32 rid){
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbRsp;

  res=LC_ClientLcc_CheckResponse(cl, rid);
  if (res!=LC_Client_ResultOk)
    return res;

  dbRsp=LC_ClientLcc_GetNextResponse(cl, rid);
  assert(dbRsp);

  res=LC_ClientLcc_CheckForError(dbRsp);
  if (res!=LC_Client_ResultOk) {
    GWEN_DB_Group_free(dbRsp);
    return res;
  }

  GWEN_DB_Group_free(dbRsp);
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_ClientLcc_ReleaseCard(LC_CLIENT *cl, LC_CARD *cd) {
  GWEN_TYPE_UINT32 rqid;
  LC_CLIENT_RESULT res;

  rqid=LC_ClientLcc_SendReleaseCard(cl, cd);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"releaseCard\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_ClientLcc_CheckResponse_Wait(cl, rqid,
                                      LC_Client_GetVeryLongTimeout(cl));
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
      LC_ClientLcc_DeleteRequest(cl, rqid);
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"releaseCard\"");
    }
    return res;
  }
  res=LC_ClientLcc_CheckReleaseCard(cl, rqid);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"releaseCard\"");
    LC_ClientLcc_DeleteRequest(cl, rqid);
    return res;
  }

  LC_ClientLcc_DeleteRequest(cl, rqid);

  return LC_Client_ResultOk;
}



GWEN_TYPE_UINT32 LC_ClientLcc_SendCommandCard(LC_CLIENT *cl,
                                              LC_CARD *cd,
                                              const char *apdu,
                                              unsigned int len,
                                              LC_CLIENT_CMDTARGET t) {
  GWEN_DB_NODE *dbReq;
  GWEN_TYPE_UINT32 rqid;
  char numbuf[16];
  const char *s;

  assert(apdu);
  assert(len);
  dbReq=GWEN_DB_Group_new("Client_CommandCard");
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", LC_Card_GetCardId(cd));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "cardid", numbuf);

  GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "data", apdu, len);

  switch(t) {
  case LC_Client_CmdTargetReader: s="reader"; break;
  case LC_Client_CmdTargetCard:   s="card"; break;
  default:
    DBG_ERROR(LC_LOGDOMAIN, "Unknown command target %d", t);
    return 0;
  }
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "target", s);

  /* send request */
  rqid=LC_ClientLcc_SendRequest(cl, cd, LC_CardLcc_GetServerId(cd), dbReq);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    return 0;
  }

  return rqid;
}



LC_CLIENT_RESULT
LC_ClientLcc_CheckCommandCard(LC_CLIENT *cl,
                              GWEN_TYPE_UINT32 rid,
                              GWEN_BUFFER *data){
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbRsp;
  const void *bp;
  unsigned int bs;
  const char *txt;
  LC_REQUEST *rq;
  LC_CARD *card;
  int code;

  rq=LC_ClientLcc_FindWorkingRequest(cl, rid);
  if (!rq) {
    if (LC_ClientLcc_FindWaitingRequest(cl, rid)) {
      DBG_INFO(LC_LOGDOMAIN, "Request not yet sent");
      return LC_Client_ResultWait;
    }
    DBG_ERROR(LC_LOGDOMAIN, "Request not found");
    return LC_Client_ResultIpcError;
  }
  card=LC_Request_GetCard(rq);
  assert(card);

  res=LC_ClientLcc_CheckResponse(cl, rid);
  if (res!=LC_Client_ResultOk)
    return res;

  dbRsp=LC_ClientLcc_GetNextResponse(cl, rid);
  assert(dbRsp);

  res=LC_ClientLcc_CheckForError(dbRsp);
  if (res!=LC_Client_ResultOk) {
    GWEN_DB_Group_free(dbRsp);
    return res;
  }

  txt=GWEN_DB_GetCharValue(dbRsp, "data/text", 0, "");
  code=GWEN_DB_GetIntValue(dbRsp, "data/code", 0, -1);
  if (code!=0) {
    DBG_ERROR(LC_LOGDOMAIN, "Command error %d (%s)", code, txt);
    LC_Card_SetLastResult(card, "error", txt, -1, -1);
    GWEN_DB_Group_free(dbRsp);
    return LC_Client_ResultCmdError;
  }

  bp=GWEN_DB_GetBinValue(dbRsp, "data/data", 0, 0, 0, &bs);
  if (bp && bs>1) {
    LC_Card_SetLastResult(card, "ok",
                          txt,
                          ((const unsigned char*)bp)[bs-2],
                          ((const unsigned char*)bp)[bs-1]);
    GWEN_Buffer_AppendBytes(data, bp, bs);
  }
  GWEN_DB_Group_free(dbRsp);
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_ClientLcc_CommandCard(LC_CLIENT *cl,
                                          LC_CARD *card,
                                          const char *apdu,
                                          unsigned int len,
                                          GWEN_BUFFER *rbuf,
                                          LC_CLIENT_CMDTARGET t,
                                          int timeout){
  GWEN_TYPE_UINT32 rqid;
  LC_CLIENT_RESULT res;

  rqid=LC_ClientLcc_SendCommandCard(cl, card, apdu, len, t);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"commandCard\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_ClientLcc_CheckResponse_Wait(cl, rqid, timeout);
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
      LC_ClientLcc_DeleteRequest(cl, rqid);
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"commandCard\"");
      LC_ClientLcc_DeleteRequest(cl, rqid);
    }
    return res;
  }
  res=LC_ClientLcc_CheckCommandCard(cl, rqid, rbuf);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"commandCard\"");
    LC_ClientLcc_DeleteRequest(cl, rqid);
    return LC_Client_ResultCmdError;
  }
  LC_ClientLcc_DeleteRequest(cl, rqid);

  return LC_Client_ResultOk;
}






