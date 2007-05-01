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




int LCD_Driver_HandleStopDriver(LCD_DRIVER *d,
                               GWEN_TYPE_UINT32 rid,
                               GWEN_DB_NODE *dbReq){
  LCD_READER_LIST *rl;
  LCD_READER *r;

  assert(d);
  assert(dbReq);

  rl=LCD_Driver_GetReaders(d);
  assert(rl);
  r=LCD_Reader_List_First(rl);
  while(r) {
    LCD_READER *nr;

    nr=LCD_Reader_List_Next(r);
    /* deinit reader */
    DBG_INFO(LCD_Reader_GetLogger(r),
             "Disconnecting reader \"%s\"", LCD_Reader_GetName(r));
    if (LCD_Driver_DisconnectReader(d, r)) {
      DBG_WARN(LCD_Reader_GetLogger(r), "Could not disconnect reader");
    }
    else {
      DBG_INFO(LCD_Reader_GetLogger(r), "Reader \"%s\" disconnected", LCD_Reader_GetName(r));
    }
    LCD_Reader_List_Del(r);
    LCD_Reader_free(r);
    r=nr;
  } /* while r */

  LCD_Driver_RemoveCommand(d, rid, 0);

  DBG_NOTICE(0, "Driver down");
  d->stopDriver=1;
  return 0;
}



