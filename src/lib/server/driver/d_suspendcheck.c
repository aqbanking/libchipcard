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




int LCD_Driver_HandleSuspendCheck(LCD_DRIVER *d,
                                  GWEN_TYPE_UINT32 rid,
                                  GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 readerId;
  LCD_READER *r;

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
    DBG_ERROR(0, "A reader with id \"%08x\" does not exist", readerId);
    /* TODO: send error result */
    return -1;
  }

  DBG_NOTICE(LCD_Reader_GetLogger(r), "Suspending checks");
  LCD_Reader_AddReaderFlags(r, LC_READER_FLAGS_SUSPENDED_CHECKS);
  LCD_Driver_RemoveCommand(d, rid, 0);
  return 0;
}



