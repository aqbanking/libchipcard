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





int LCD_Driver_HandleStopReader(LCD_DRIVER *d,
                               GWEN_TYPE_UINT32 rid,
                               GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 readerId;
  GWEN_DB_NODE *dbRsp;
  LCD_READER *r;
  char numbuf[16];
  GWEN_TYPE_UINT32 retval;

  assert(d);
  assert(dbReq);
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "data/readerId", 0, "0"),
                "%x",
                &readerId)) {
    DBG_ERROR(0, "Bad readerId");
    /* TODO: send error result */
    return -1;
  }

  /* check whether we have a reader of that id */
  r=LCD_Driver_FindReaderById(d, readerId);
  if (!r) {
    DBG_ERROR(0, "A reader with id \"%08x\" does not exists", readerId);
    /* TODO: send error result */
    return -1;
  }

  /* deinit reader */
  DBG_NOTICE(LCD_Reader_GetLogger(r), "Disconnecting reader");
  dbRsp=GWEN_DB_Group_new("Driver_StopReaderResponse");
  retval=LCD_Driver_DisconnectReader(d, r);
  if (retval!=0) {
    DBG_INFO(LCD_Reader_GetLogger(r), "Could not disconnect reader");
    GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "code", -retval);
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text",
                         LCD_Driver_GetErrorText(d, retval));
  }
  else {
    /* init ok */
    DBG_NOTICE(LCD_Reader_GetLogger(r), "Deinit succeeded");
    GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "code", 0);
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", "Reader down as requested");
  }

  /* create response */
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", readerId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);
  if (LCD_Driver_SendResponse(d, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }
  LCD_Driver_RemoveCommand(d, rid, 0);

  DBG_NOTICE(0, "Reader down");
  LCD_Driver_DelReader(d, r);
  LCD_Reader_free(r);
  return 0;
}



