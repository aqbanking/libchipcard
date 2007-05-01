/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: driver.c 284 2006-09-22 00:53:00Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

/* included by driver.c */



GWEN_TYPE_UINT32 LCD_Driver_SendCommand(LCD_DRIVER *d,
                                        GWEN_DB_NODE *dbCommand) {
  GWEN_DB_SetCharValue(dbCommand, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driverId", LCD_Driver_GetDriverId(d));

  return GWEN_IpcManager_SendRequest(d->ipcManager,
                                     d->ipcId, dbCommand);
}



int LCD_Driver_SendResponse(LCD_DRIVER *d,
                           GWEN_TYPE_UINT32 rid,
                           GWEN_DB_NODE *dbCommand) {
  return GWEN_IpcManager_SendResponse(d->ipcManager,
                                      rid, dbCommand);
}



int LCD_Driver_SendResult(LCD_DRIVER *d,
                          GWEN_TYPE_UINT32 rid,
                          const char *name,
                          int code,
                          const char *text){
  GWEN_DB_NODE *db;

  db=GWEN_DB_Group_new(name);
  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_DEFAULT,
                      "code", code);
  if (text)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "text", text);
  return LCD_Driver_SendResponse(d, rid, db);
}



GWEN_TYPE_UINT32 LCD_Driver_GetNextInRequest(LCD_DRIVER *d) {
  assert(d);
  return GWEN_IpcManager_GetNextInRequest(d->ipcManager,
                                          LCD_DRIVER_MARK_DRIVER);
}



GWEN_DB_NODE *LCD_Driver_GetInRequestData(LCD_DRIVER *d,
                                         GWEN_TYPE_UINT32 rid) {
  assert(d);
  return GWEN_IpcManager_GetInRequestData(d->ipcManager, rid);
}


int LCD_Driver_CheckResponses(GWEN_DB_NODE *db) {
  const char *name;

  if (strcasecmp(GWEN_DB_GroupName(db), "error")==0) {
    int numCode;
    const char *txt;

    numCode=GWEN_DB_GetIntValue(db, "code", 0, LC_ERROR_GENERIC);
    txt=GWEN_DB_GetCharValue(db, "text", 0, "<empty>");
    DBG_ERROR(0, "Error %d: %s", numCode, txt);
    return numCode;
  }

  name=GWEN_DB_GetCharValue(db, "ipc/cmd", 0, 0);
  if (!name) {
    DBG_ERROR(0, "Bad IPC message (no command)");
    return -1;
  }

  if (strcasecmp(name, "Error")==0) {
    int numCode;

    numCode=GWEN_DB_GetIntValue(db, "data/code", 0, -1);
    if (numCode) {
      DBG_ERROR(0, "Error %d: %s", numCode,
                GWEN_DB_GetCharValue(db,
                                     "data/text", 0, "(empty)"));
      return -1;
    }
  }
  return 0;
}



int LCD_Driver_Connect(LCD_DRIVER *d,
                       int code, const char *text,
                       GWEN_TYPE_UINT32 dflagsValue,
                       GWEN_TYPE_UINT32 dflagsMask) {
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbRsp;
  time_t startt;
  GWEN_TYPE_UINT32 rid;

  assert(d);

  if (d->testMode) {
    DBG_INFO(0, "Testmode, will not connect");
    return 0;
  }
  startt=time(0);

  /* tell the server about our status */
  dbReq=GWEN_DB_Group_new("Driver_Ready");
  if (d->driverId)
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driverId", d->driverId);

  /* send some additional information in remote mode */
  if (d->dtype)
    GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "driverType", d->dtype);
  LC_DriverFlags_toDb(dbReq, "driverFlagsValue", dflagsValue);
  LC_DriverFlags_toDb(dbReq, "driverFlagsMask", dflagsMask);

  /* send information about every reader we already now.
   * Normally we don't know any reader by now, since the server informs us
   * about that later upon a StartReader request.
   * However, in remote mode (or later for PC/SC drivers) we in fact do have
   * some readers already, so we now inform the server about readers we can
   * offer.
   */
  if (LCD_Reader_List_GetCount(d->readers)) {
    GWEN_DB_NODE *dbReaders;
    LCD_READER *r;

    dbReaders=GWEN_DB_GetGroup(dbReq, GWEN_DB_FLAGS_OVERWRITE_GROUPS,
                               "readers");
    assert(dbReaders);

    r=LCD_Reader_List_First(d->readers);
    while(r) {
      GWEN_DB_NODE *dbReader;
      GWEN_TYPE_UINT32 flags;

      dbReader=GWEN_DB_GetGroup(dbReaders, GWEN_DB_FLAGS_OVERWRITE_GROUPS,
                                "reader");
      assert(dbReader);
      GWEN_DB_SetIntValue(dbReader, GWEN_DB_FLAGS_OVERWRITE_VARS,
                          "driversReaderId",
                          LCD_Reader_GetDriversReaderId(r));
      GWEN_DB_SetCharValue(dbReader, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "readerType", d->rtype);
      GWEN_DB_SetCharValue(dbReader, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "readerName", d->rname);
      GWEN_DB_SetIntValue(dbReader, GWEN_DB_FLAGS_OVERWRITE_VARS,
                          "port", d->rport);
      GWEN_DB_SetIntValue(dbReader, GWEN_DB_FLAGS_OVERWRITE_VARS,
                          "slots", d->rslots);

      flags=LCD_Reader_GetReaderFlags(r);
      LC_ReaderFlags_toDb(dbReader, "readerFlags", flags);

      r=LCD_Reader_List_Next(r);
    } /* while */
  } /* if readers */


  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "code", code);
  if (text)
    GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", text);

  rid=GWEN_IpcManager_SendRequest(d->ipcManager,
                                  d->ipcId,
                                  dbReq);
  if (rid==0) {
    DBG_ERROR(0, "Could not send command");
    return -1;
  }

  /* this sends the message and hopefully receives an answer */
  DBG_INFO(0, "Sending Ready Report");
  dbRsp=0;
  while (1) {
    dbRsp=GWEN_IpcManager_GetResponseData(d->ipcManager, rid);
    if (dbRsp) {
      DBG_DEBUG(0, "Command answered");
      break;
    }
    DBG_VERBOUS(0, "Working...");
    if (LCD_Driver__Work(d, 1000)) {
      DBG_ERROR(0, "Error at work");
      GWEN_IpcManager_RemoveRequest(d->ipcManager, d->ipcId, 1);
      return -1;
    }

    if (difftime(time(0), startt)>=LCD_DRIVER_STARTTIMEOUT) {
      DBG_ERROR(0, "Timeout");
      GWEN_IpcManager_RemoveRequest(d->ipcManager, d->ipcId, 1);
      return -1;
    }
  } /* while */

  DBG_DEBUG(0, "Answer received");
  if (LCD_Driver_CheckResponses(dbRsp)) {
    DBG_ERROR(0, "Error returned by server, aborting");
    GWEN_IpcManager_RemoveRequest(d->ipcManager, rid, 1);
    return -1;
  }
  GWEN_IpcManager_RemoveRequest(d->ipcManager, rid, 1);

  DBG_NOTICE(0, "Connected to server");
  return 0;
}



void LCD_Driver_Disconnect(LCD_DRIVER *d){
  assert(d);
  if (d->testMode) {
    DBG_INFO(0, "Testmode, will not disconnect (since I'm not connected)");
    return;
  }

  if (GWEN_IpcManager_Disconnect(d->ipcManager, d->ipcId)) {
    DBG_ERROR(0, "Error while disconnecting");
  }
}



int LCD_Driver_SendStatusChangeNotification(LCD_DRIVER *d,
                                            LCD_SLOT *sl) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  int slot;
  int cardnum;
  GWEN_BUFFER *atr;
  int isInserted;
  LCD_READER *r;
  GWEN_TYPE_UINT32 rid;

  r=LCD_Slot_GetReader(sl);
  slot=LCD_Slot_GetSlotNum(sl);
  cardnum=LCD_Slot_GetCardNum(sl);
  atr=LCD_Slot_GetAtr(sl);
  isInserted=(LCD_Slot_GetStatus(sl) & LCD_SLOT_STATUS_CARD_CONNECTED);

  dbReq=GWEN_DB_Group_new(isInserted?
                          "Driver_CardInserted":
                          "Driver_CardRemoved");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x", LCD_Reader_GetReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LCD_Reader_GetDriversReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driversReaderId", numbuf);

  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "slotnum", slot);

  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cardnum", cardnum);

  if (isInserted) {
    if (atr)
      if (GWEN_Buffer_GetUsedBytes(atr))
        GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                            "atr",
                            GWEN_Buffer_GetStart(atr),
                            GWEN_Buffer_GetUsedBytes(atr));
    if (LCD_Slot_GetFlags(sl) & LCD_SLOT_FLAGS_PROCESSORCARD)
      GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "cardType", "PROCESSOR");
    else
      GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "cardType", "MEMORY");
  } /* if inserted */

  rid=LCD_Driver_SendCommand(d, dbReq);
  if (rid==0) {
    DBG_ERROR(0, "Could not send command");
    return -1;
  }

  GWEN_IpcManager_RemoveRequest(d->ipcManager, rid, 1);
  DBG_DEBUG(0, "Command sent");
  return 0;
}



int LCD_Driver_SendReaderErrorNotification(LCD_DRIVER *d,
                                           LCD_READER *r,
                                           const char *text) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  GWEN_TYPE_UINT32 rid;

  assert(d);
  dbReq=GWEN_DB_Group_new("Driver_ReaderError");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x", LCD_Reader_GetReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LCD_Reader_GetDriversReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driversReaderId", numbuf);

  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "text", text);

  rid=LCD_Driver_SendCommand(d, dbReq);
  if (rid==0) {
    DBG_ERROR(0, "Could not send command");
    return -1;
  }

  GWEN_IpcManager_RemoveRequest(d->ipcManager, rid, 1);
  DBG_DEBUG(0, "Command sent");

  return 0;
}



int LCD_Driver_RemoveCommand(LCD_DRIVER *d,
                            GWEN_TYPE_UINT32 rid,
                            int outbound){
  assert(d);
  return GWEN_IpcManager_RemoveRequest(d->ipcManager, rid, outbound);
}




