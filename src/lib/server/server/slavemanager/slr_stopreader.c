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



int LCSL_SlaveManager_HandleStopReader(LCSL_SLAVEMANAGER *slm,
                                       GWEN_TYPE_UINT32 rid,
                                       const char *name,
                                       GWEN_DB_NODE *dbReq) {
  GWEN_TYPE_UINT32 readerId;
  const char *s;
  unsigned int x;
  LCDM_DEVICEMANAGER *dm;
  GWEN_DB_NODE *dbClientResponse;
  LCCO_READER_LIST2_ITERATOR *it;
  LCCO_READER *sr=0;

  DBG_INFO(0, "Master: StopReader");

  dm=LCS_Server_GetDeviceManager(slm->server);
  assert(dm);

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

  DBG_NOTICE(0, "Sending response to StopReader");
  dbClientResponse=GWEN_DB_Group_new("Driver_StopReaderResponse");
  if (LCSL_Reader_GetFlags(sr) & LCSL_READER_FLAGS_STARTED) {
    LCDM_DeviceManager_EndUseReader(dm, LCCO_Reader_GetReaderId(sr));
    LCSL_Reader_DelFlags(sr, LCSL_READER_FLAGS_STARTED);
    LCSL_Reader_AddFlags(sr, LCSL_READER_FLAGS_STOPPED);
    GWEN_DB_SetIntValue(dbClientResponse,
                        GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "code", 0);
    GWEN_DB_SetCharValue(dbClientResponse,
                         GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", "Reader down as commanded");
  }
  else {
    GWEN_DB_SetIntValue(dbClientResponse,
                        GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "code", LC_ERROR_INVALID);
    GWEN_DB_SetCharValue(dbClientResponse,
                         GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", "Reader has not been started");
  }

  /* send response */
  if (GWEN_IpcManager_SendResponse(slm->ipcManager,
                                   rid,
                                   dbClientResponse)) {
    DBG_ERROR(0, "Could not send StopReader response");
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  return 0; /* handled */
}







