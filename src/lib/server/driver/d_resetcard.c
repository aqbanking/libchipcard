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


int LCD_Driver_HandleResetCard(LCD_DRIVER *d,
                               uint32_t rid,
                               GWEN_DB_NODE *dbReq){
  uint32_t readerId;
  LCD_READER *r;
  int slotNum;
  int cardNum;
  LCD_SLOT *slot;
  char retval;

  assert(d);
  assert(dbReq);
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "data/readerId", 0, "0"),
                "%x",
                &readerId)) {
    DBG_ERROR(0, "Bad readerId");
    /* TODO: send error result */
    return -1;
  }

  slotNum=GWEN_DB_GetIntValue(dbReq, "data/slotnum", 0, -1);
  if (slotNum==-1) {
    DBG_ERROR(0, "Bad slot number");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  cardNum=GWEN_DB_GetIntValue(dbReq, "data/cardnum", 0, -1);
  if (cardNum==-1) {
    DBG_ERROR(0, "Bad card number");
    LCD_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  /* check whether we have a reader of that id */
  r=LCD_Driver_FindReaderById(d, readerId);
  if (!r) {
    DBG_ERROR(0, "A reader with id \"%08x\" does not exists", readerId);
    LCD_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  /* get the referenced slot */
  slot=LCD_Reader_FindSlot(r, slotNum);
  if (!slot) {
    DBG_ERROR(LCD_Reader_GetLogger(r), "Slot \"%d\" not found", slotNum);
    LCD_Driver_RemoveCommand(d, rid, 0);
    return -1;
  }

  if (LCD_Slot_GetStatus(slot) & LCD_SLOT_STATUS_DISABLED) {
    DBG_ERROR(LCD_Reader_GetLogger(r), "Slot \"%d\" disabled",
              LCD_Slot_GetSlotNum(slot));
    LCD_Driver_RemoveCommand(d, rid, 0);
    return 0;
  }

  /* check card number and reader status */
  if ((LCD_Slot_GetCardNum(slot)!=cardNum) ||
      !(LCD_Slot_GetStatus(slot) & LCD_SLOT_STATUS_CARD_CONNECTED)) {
    DBG_ERROR(0, "Card \"%d\" has been removed", cardNum);
    LCD_Driver_RemoveCommand(d, rid, 0);
    return 0;
  }

  DBG_NOTICE(LCD_Reader_GetLogger(r), "Resetting card");
  retval=LCD_Driver_ResetSlot(d, slot);
  if (retval!=0) {
    DBG_ERROR(LCD_Reader_GetLogger(r), "Error resetting slot (%d: %s)",
              retval,
              LCD_Driver_GetErrorText(d, retval));
    LCD_Driver_SendReaderErrorNotification(d, r,
                                          LCD_Driver_GetErrorText(d, retval));
    DBG_NOTICE(LCD_Reader_GetLogger(r),
               "Reader \"%s\" had an error, shutting down",
               LCD_Reader_GetName(r));
    LCD_Reader_List_Del(r);
    LCD_Reader_free(r);
  }
  else {
    /* reset ok */
    DBG_INFO(LCD_Reader_GetLogger(r), "Reset succeeded");
  }

  LCD_Driver_RemoveCommand(d, rid, 0);

  return 0;
}



