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




int LCD_Driver_HandleCardCommand(LCD_DRIVER *d,
                                GWEN_TYPE_UINT32 rid,
                                GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 readerId;
  GWEN_DB_NODE *dbRsp;
  LCD_READER *r;
  char numbuf[16];
  unsigned char rspbuffer[300];
  const unsigned char *apdu;
  unsigned int apdulen;
  int rsplen;
  int slotNum;
  int cardNum;
  LCD_SLOT *slot;
  char retval;
  const char *target;
  int toReader;
  int readerError;

  assert(d);
  assert(dbReq);

  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "data/readerId", 0, "0"),
                "%x",
                &readerId)) {
    DBG_ERROR(0, "Bad readerId");
    /* TODO: send error result */
    return -1;
  }

  apdu=GWEN_DB_GetBinValue(dbReq, "data/data", 0, 0, 0, &apdulen);
  if (!apdu || apdulen<4) {
    DBG_ERROR(0, "APDU too small");
    /* send error result */
    LCD_Driver_SendResult(d,
                          rid,
                          "Driver_CardCommandResponse",
                          LC_ERROR_INVALID, "APDU too small");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return 0;
  }

  slotNum=GWEN_DB_GetIntValue(dbReq, "data/slotnum", 0, -1);
  if (slotNum==-1) {
    DBG_ERROR(0, "Bad slot number");
    /* send error result */
    LCD_Driver_SendResult(d, rid, "Driver_CardCommandResponse",
                          LC_ERROR_INVALID, "Bad slot number");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  cardNum=GWEN_DB_GetIntValue(dbReq, "data/cardnum", 0, -1);
  if (cardNum==-1) {
    DBG_ERROR(0, "Bad card number");
    /* send error result */
    LCD_Driver_SendResult(d, rid, "Driver_CardCommandResponse",
                          LC_ERROR_INVALID, "Bad card number");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  target=GWEN_DB_GetCharValue(dbReq, "data/target", 0, 0);
  if (!target) {
    /* send error result */
    LCD_Driver_SendResult(d, rid, "Driver_CardCommandResponse",
                          LC_ERROR_INVALID, "No target");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }
  if (strcasecmp(target, "reader")==0)
    toReader=-1;
  else if (strcasecmp(target, "card")==0)
    toReader=0;
  else {
    if (1!=sscanf(target, "%d", &toReader)) {
      DBG_ERROR(0, "Bad target \"%s\"", target);
      /* send error result */
      LCD_Driver_SendResult(d, rid, "Driver_CardCommandResponse",
                            LC_ERROR_INVALID, "Bad target");
      LCD_Driver_RemoveCommand(d, rid, 0);
    }
    return 0;
  }

  /* check whether we have a reader of that id */
  r=LCD_Driver_FindReaderById(d, readerId);
  if (!r) {
    DBG_ERROR(0, "A reader with id \"%08x\" does not exists", readerId);
    /* send error result */
    LCD_Driver_SendResult(d, rid, "Driver_CardCommandResponse",
                          LC_ERROR_INVALID, "Reader not found");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  /* get the referenced slot */
  slot=LCD_Reader_FindSlot(r, slotNum);
  if (!slot) {
    DBG_ERROR(LCD_Reader_GetLogger(r), "Slot \"%d\" not found", slotNum);
    /* send error result */
    LCD_Driver_SendResult(d, rid, "Driver_CardCommandResponse",
                          LC_ERROR_INVALID, "Slot not found");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  if (LCD_Slot_GetStatus(slot) & LCD_SLOT_STATUS_DISABLED) {
    DBG_ERROR(LCD_Reader_GetLogger(r), "Slot \"%d\" disabled",
              LCD_Slot_GetSlotNum(slot));
    /* send error result */
    LCD_Driver_SendResult(d, rid, "Driver_CardCommandResponse",
                          LC_ERROR_INVALID, "Slot diabled");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return 0;
  }

  /* check card number and reader status */
  if ((LCD_Slot_GetCardNum(slot)!=cardNum) ||
      !(LCD_Slot_GetStatus(slot) & LCD_SLOT_STATUS_CARD_CONNECTED)) {
    DBG_ERROR(LCD_Reader_GetLogger(r), "Card \"%d\" has been removed", cardNum);
    /* send error result */
    LCD_Driver_SendResult(d, rid, "Driver_CardCommandResponse",
                          LC_ERROR_CARD_REMOVED, "Card has been removed");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return 0;
  }

  DBG_DEBUG(LCD_Reader_GetLogger(r), "Executing command");
  GWEN_Text_LogString((const char*)apdu, apdulen, 0, GWEN_LoggerLevelDebug);
  dbRsp=GWEN_DB_Group_new("Driver_CardCommandResponse");
  rsplen=sizeof(rspbuffer)-1;
  retval=LCD_Driver_SendAPDU(d, toReader, r, slot, apdu, apdulen,
                              rspbuffer, &rsplen);
  if (retval!=0) {
    DBG_ERROR(LCD_Reader_GetLogger(r), "Error executing APDU (%08x)", retval);
    GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "code", -retval);
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", LCD_Driver_GetErrorText(d, retval));
    readerError=retval;
  }
  else {
    if (rsplen<2) {
      GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                          "code", LC_ERROR_INVALID);
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text", "Answer is too short");
      readerError=-1;
    }
    else {
      /* init ok */
      DBG_DEBUG(LCD_Reader_GetLogger(r), "Command succeeded");
      GWEN_Text_LogString((const char*)rspbuffer, rsplen, 0, GWEN_LoggerLevelDebug);

      GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                          "code", 0);
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text", "Command executed");
      assert(rsplen);
      GWEN_DB_SetBinValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                          "data", rspbuffer, rsplen);
      readerError=0;
    }
  }

  /* create response */
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", readerId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);
  GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "slotnum", slotNum);
  GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cardnum", cardNum);
  if (LCD_Driver_SendResponse(d, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }
  LCD_Driver_RemoveCommand(d, rid, 0);
  DBG_DEBUG(0, "Response send");

  if (readerError) {
    DBG_NOTICE(LCD_Reader_GetLogger(r),
               "Reader \"%s\" had an error, shutting down",
               LCD_Reader_GetName(r));
    LCD_Driver_SendReaderErrorNotification(d, r,
                                           LCD_Driver_GetErrorText(d,
                                                                   readerError));
    LCD_Reader_List_Del(r);
    LCD_Reader_free(r);
  }

  return 0;
}



