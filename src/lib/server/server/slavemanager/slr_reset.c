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



int LCSL_SlaveManager_HandleCardReset(LCSL_SLAVEMANAGER *slm,
                                      GWEN_TYPE_UINT32 rid,
                                      const char *name,
                                      GWEN_DB_NODE *dbReq) {
  GWEN_TYPE_UINT32 readerId;
  const char *s;
  unsigned int x;
  LCDM_DEVICEMANAGER *dm;
  GWEN_TYPE_UINT32 cardId;
  LCCM_CARDMANAGER *cm;
  LCCO_CARD *card;
  GWEN_TYPE_UINT32 outRid;
  GWEN_DB_NODE *dbOutReq;
  LCCO_READER_LIST2_ITERATOR *it;
  LCCO_READER *sr=0;

  DBG_INFO(0, "Master: CardReset");

  dm=LCS_Server_GetDeviceManager(slm->server);
  assert(dm);

  cm=LCS_Server_GetCardManager(slm->server);
  assert(cm);

  /* get reader id */
  readerId=0;
  s=GWEN_DB_GetCharValue(dbReq, "data/driversReaderId", 0, 0);
  if (s && 1==sscanf(s, "%x", &x))
    readerId=x;
  if (readerId==0) {
    DBG_ERROR(0, "Invalid reader id");
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* get card id */
  cardId=GWEN_DB_GetIntValue(dbReq, "data/cardNum", 0, 0);
  if (cardId==0) {
    DBG_ERROR(0, "Missing card id");
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* get slave reader object */
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
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }
  if (!sr) {
    DBG_ERROR(0, "Reader \"%08x\" not found", readerId);
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* check whether the reader is available */
  if (!LCCO_Reader_IsAvailable(sr)) {
    DBG_ERROR(0, "Reader \"%08x\" unplugged", readerId);
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* check whether the reader has been started */
  if (!(LCSL_Reader_GetFlags(sr) & LCSL_READER_FLAGS_STARTED)) {
    DBG_ERROR(0, "Reader \"%08x\" has not been started", readerId);
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    if (LCSL_Reader_GetFlags(sr) & LCSL_READER_FLAGS_STARTED) {
      LCDM_DeviceManager_EndUseReader(dm, LCCO_Reader_GetReaderId(sr));
      LCSL_Reader_DelFlags(sr, LCSL_READER_FLAGS_STARTED);
    }
    return -1;
  }

  /* get referenced card */
  card=LCCM_CardManager_FindCard(cm, cardId);
  if (!card) {
    DBG_ERROR(0, "Card not found");
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* check whether card is still inserted */
  if (LCCO_Card_GetStatus(card)!=LC_CardStatusInserted) {
    DBG_ERROR(0, "Card has been removed");
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* create outbound request */
  dbOutReq=GWEN_DB_Group_new("Driver_ResetCard");
  outRid=LCDM_DeviceManager_SendCardCommand(dm, card, dbOutReq);
  if (outRid==0) {
    DBG_ERROR(0, "Could not send command to reader");
    if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }
  if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, outRid, 1)) {
    DBG_ERROR(0, "Could not remove request");
    abort();
  }
  if (GWEN_IpcManager_RemoveRequest(slm->ipcManager, rid, 0)) {
    DBG_ERROR(0, "Could not remove request");
    abort();
  }

  return 0; /* handled */
}



