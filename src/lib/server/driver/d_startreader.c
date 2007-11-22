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





int LCD_Driver_HandleStartReader(LCD_DRIVER *d,
                                 uint32_t rid,
                                 GWEN_DB_NODE *dbReq){
  uint32_t readerId;
  uint32_t driversReaderId;
  const char *name;
  int port;
  int slots;
  const char *devicePath;
  uint32_t flags;
  LCD_READER *r;
  char numbuf[16];
  uint32_t retval;
  GWEN_DB_NODE *dbRsp;

  assert(d);
  assert(dbReq);
  DBG_NOTICE(0, "Command: Start reader");

  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "data/readerId", 0, "0"),
                "%x",
                &readerId)) {
    DBG_ERROR(0, "Bad readerId");
    /* TODO: send error result */
    return -1;
  }

  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "data/driversReaderId", 0, "0"),
                "%x",
                &driversReaderId)) {
    DBG_ERROR(0, "Bad driversReaderId");
    /* TODO: send error result */
    return -1;
  }

  name=GWEN_DB_GetCharValue(dbReq, "data/name", 0, "noname");
  port=GWEN_DB_GetIntValue(dbReq, "data/port", 0, 0);
  flags=GWEN_DB_GetIntValue(dbReq, "data/flags", 0, 0);
  devicePath=GWEN_DB_GetCharValue(dbReq, "data/devicePath", 0, 0);
  slots=GWEN_DB_GetIntValue(dbReq, "data/slots", 0, 0);
  if (!slots || slots>16) {
    DBG_ERROR(0, "Bad number of slots (%d)", slots);
    /* TODO: send error result */
    return -1;
  }

  /* prepare response */
  dbRsp=GWEN_DB_Group_new("Driver_StartReaderResponse");
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", readerId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);

  /* check whether we have a reader of that id */
  r=LCD_Driver_FindReaderById(d, readerId);
  if (r) {
    DBG_WARN(0, "A reader with id \"%08x\" already exists", readerId);

    DBG_NOTICE(LCD_Reader_GetLogger(r), "Restarting reader");

    retval=LCD_Driver_DisconnectReader(d, r);
    if (retval==0)
      retval=LCD_Driver_ConnectReader(d, r);

    if (retval) {
      DBG_ERROR(0, "Could not restart reader");
      GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                          "code", -retval);
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text",
                           LCD_Driver_GetErrorText(d, retval));
    }
    else {
      LCD_Reader_ResetErrorCount(r);
      if (LCD_Reader_GetReaderFlags(r) & LC_READER_FLAGS_NOINFO) {
        DBG_WARN(0, "ReaderInfo disabled");
      }
      else {
        GWEN_BUFFER *ibuf;
        uint32_t rv;

        ibuf=GWEN_Buffer_new(0, 256, 0, 1);
        rv=LCD_Driver_ReaderInfo(d, r, ibuf);
        if (rv) {
          DBG_WARN(0, "ReaderInfo not available (%s)",
                   LCD_Driver_GetErrorText(d, rv));
        }
        else {
          GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                               "info",
                               GWEN_Buffer_GetStart(ibuf));
        }
        GWEN_Buffer_free(ibuf);
      }

      GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                          "code", 0);
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text", "Reader up and waiting");
    }

    if (LCD_Driver_SendResponse(d, rid, dbRsp)) {
      DBG_ERROR(0, "Could not send response");
      LCD_Driver_RemoveCommand(d, rid, 0);
      return -1;
    }
  } /* if reader found */
  else {
    /* search by driversReaderId */
    if (driversReaderId) {
      r=LCD_Driver_FindReaderByDriversId(d, driversReaderId);
      if (r) {
        if (LCD_Reader_GetReaderId(r)==0) {
          /* The reader exists but has no reader id. So this is the first time
           * the reader has been accessed. Assign the reader id from the
           * server so that the next calls will find it. */
          LCD_Reader_SetReaderId(r, readerId);
        }
        else {
          DBG_ERROR(0, "Uups, reader already has an id ?");
          /* send error result */
          GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                              "code", LC_ERROR_INVALID);
          GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                               "text",
                               "Internal error (reader already has an id)");

          LCD_Driver_SendResponse(d, rid, dbRsp);
          LCD_Driver_RemoveCommand(d, rid, 0);
          return -1;
        }
      }
      else {
        DBG_ERROR(0, "Reader not found");
      }
    }
    else {
      DBG_ERROR(0, "No DriversReaderId");
    }

    if (!r) {
      /* check whether we have a reader at that port */
      r=LCD_Driver_FindReaderByPort(d, port);
      if (r) {
        DBG_ERROR(0, "A reader with port \"%08x\" already exists", port);
        /* send error result */
        GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                            "code", LC_ERROR_INVALID);
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text",
                             "There already is a reader at the given port");
  
        LCD_Driver_SendResponse(d, rid, dbRsp);
        LCD_Driver_RemoveCommand(d, rid, 0);
        return -1;
      }
      /* if not found it is ok to create the reader */
      r=LCD_Driver_CreateReader(d, readerId, name, port,
                                devicePath,
                                slots, flags);
      assert(r);
      LCD_Reader_ResetErrorCount(r);
      LCD_Driver_AddReader(d, r);
    }

    /* setup logging for the reader */
    if (d->readerLogFile) {
      GWEN_BUFFER *mbuf;

      mbuf=GWEN_Buffer_new(0, 256, 0, 1);
      LCD_Driver_ReplaceVar(d->readerLogFile, "reader", name, mbuf);
      if (GWEN_Directory_GetPath(GWEN_Buffer_GetStart(mbuf),
                                 GWEN_PATH_FLAGS_VARIABLE)) {
        DBG_ERROR(0, "Could not create log file for reader \"%s\"", name);
      }
      else {
        if (GWEN_Logger_Open(name,
                             name,
                             GWEN_Buffer_GetStart(mbuf),
			     GWEN_LoggerType_File,
                             GWEN_LoggerFacility_Daemon)) {
          DBG_ERROR(0, "Could not open logger for reader \"%s\"", name);
        }
        else {
          DBG_NOTICE(0, "Reader \"%s\" logs to \"%s\"", name,
                     GWEN_Buffer_GetStart(mbuf));
          LCD_Reader_SetLogger(r, name);
        }
        GWEN_Buffer_free(mbuf);
      }
    } /* if reader log file */
    else {
      if (GWEN_Logger_Open(name,
                           name,
                           0,
                           GWEN_LoggerType_Console,
                           GWEN_LoggerFacility_Daemon)) {
        DBG_ERROR(0, "Could not open logger for reader \"%s\"", name);
      }
    }
    GWEN_Logger_SetLevel(name, d->logLevel);

    /* init reader */
    DBG_NOTICE(LCD_Reader_GetLogger(r),
               "Init reader %s", LCD_Reader_GetName(r));
    retval=LCD_Driver_ConnectReader(d, r);
    if (retval) {
      DBG_ERROR(LCD_Reader_GetLogger(r),
                "Could not connect reader %s (%d: %s)",
                LCD_Reader_GetName(r),
                retval,
                LCD_Driver_GetErrorText(d, retval));
      GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                          "code", LC_ERROR_GENERIC);
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text",
                           LCD_Driver_GetErrorText(d, retval));
    }
    else {
      GWEN_BUFFER *ibuf;
      uint32_t rv;

      ibuf=GWEN_Buffer_new(0, 256, 0, 1);
      rv=LCD_Driver_ReaderInfo(d, r, ibuf);
      if (rv) {
        DBG_WARN(0, "ReaderInfo not available (%s)",
                 LCD_Driver_GetErrorText(d, rv));
      }
      else {
        DBG_NOTICE(LCD_Reader_GetLogger(r), "ReaderInfo: %s",
                   GWEN_Buffer_GetStart(ibuf));
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "info",
                             GWEN_Buffer_GetStart(ibuf));
      }
      GWEN_Buffer_free(ibuf);

      DBG_NOTICE(LCD_Reader_GetLogger(r), "Reader up and waiting");
      GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                          "code", 0);
      GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "text", "Reader up and waiting");
    }

    if (LCD_Driver_SendResponse(d, rid, dbRsp)) {
      DBG_ERROR(0, "Could not send response");
      LCD_Driver_RemoveCommand(d, rid, 0);
      return -1;
    }
    DBG_NOTICE(LCD_Reader_GetLogger(r), "Reader start handled");
  }
  LCD_Driver_RemoveCommand(d, rid, 0);

  return 0;
}
