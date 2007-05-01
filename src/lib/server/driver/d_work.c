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




int LCD_Driver__Work(LCD_DRIVER *d, int timeout){
  GWEN_NETLAYER_RESULT res;
  int rv;

  if (!GWEN_Net_HasActiveConnections()) {
    DBG_ERROR(0, "No active connections, stopping");
    return -1;
  }

  res=GWEN_Net_HeartBeat(timeout);
  if (res==GWEN_NetLayerResult_Error) {
    DBG_ERROR(0, "Network error");
    return -1;
  }
  else if (res==GWEN_NetLayerResult_Idle) {
    DBG_VERBOUS(0, "No activity");
  }

  while(1) {
    DBG_DEBUG(0, "Driver: Working");
    /* activity detected, work with it */
    rv=GWEN_IpcManager_Work(d->ipcManager);
    if (rv==-1) {
      DBG_ERROR(0, "Error while working with IPC");
      return -1;
    }
    else if (rv==1)
      break;
  }

  return 0;
}



int LCD_Driver_CheckStatusChanges(LCD_DRIVER *d) {
  LCD_READER *r;

  r=LCD_Reader_List_First(LCD_Driver_GetReaders(d));
  while(r) {
    LCD_READER *rnext;
    GWEN_TYPE_UINT32 retval;

    rnext=LCD_Reader_List_Next(r);

    if (LCD_Reader_GetStatus(r) & LCD_READER_STATUS_UP &&
        !(LCD_Reader_GetReaderFlags(r) & LC_READER_FLAGS_SUSPENDED_CHECKS)) {
      retval=LCD_Driver_ReaderStatus(d, r);
      if (retval) {
        DBG_ERROR(LCD_Reader_GetLogger(r), "Error getting reader status");
        LCD_Driver_SendReaderErrorNotification(d, r,
                                              LCD_Driver_GetErrorText(d, retval));
        DBG_NOTICE(LCD_Reader_GetLogger(r),
                   "Reader \"%s\" had an error, shutting down",
                   LCD_Reader_GetName(r));
        LCD_Reader_List_Del(r);
        LCD_Reader_free(r);
      }
      else {
        LCD_SLOT_LIST *slList;
        LCD_SLOT *sl;
  
        slList=LCD_Reader_GetSlots(r);
        sl=LCD_Slot_List_First(slList);
        while(sl) {
          int isInserted;
          GWEN_TYPE_UINT32 newStatus, oldStatus;
          int cardNum;
  
          newStatus=LCD_Slot_GetStatus(sl);
          if (!(newStatus & LCD_SLOT_STATUS_DISABLED)) {
            oldStatus=LCD_Slot_GetLastStatus(sl);
    
            if (((newStatus^oldStatus) & LCD_SLOT_STATUS_CARD_INSERTED) &&
                /*!(newStatus & LCD_SLOT_STATUS_CARD_CONNECTED) && */
                (newStatus & LCD_SLOT_STATUS_CARD_INSERTED)){
              /* card has just been inserted, try to connect it */
              DBG_NOTICE(LCD_Reader_GetLogger(r),
                         "Card inserted, trying to connect it");
              if (LCD_Driver_ConnectSlot(d, sl)) {
                DBG_ERROR(0, "Card inserted, but I can't connect to it");
              }
              newStatus=LCD_Slot_GetStatus(sl);
            }
    
            isInserted=(newStatus & LCD_SLOT_STATUS_CARD_CONNECTED);
            if ((newStatus^oldStatus) &
                (LCD_SLOT_STATUS_CARD_CONNECTED)){
              DBG_NOTICE(LCD_Reader_GetLogger(r),
                         "Status changed on slot %d (%08x->%08x) (cardnum %d)",
                         LCD_Slot_GetSlotNum(sl),
                         oldStatus, newStatus,
                         LCD_Slot_GetCardNum(sl));
              if (isInserted) {
                DBG_INFO(LCD_Reader_GetLogger(r), "Card is now connected");
                cardNum=++LCD_Driver__LastCardNum;
                LCD_Slot_SetCardNum(sl, cardNum);
              }
              else {
                DBG_INFO(LCD_Reader_GetLogger(r), "Card is not connected");
                cardNum=LCD_Slot_GetCardNum(sl);
              }
  
              DBG_INFO(LCD_Reader_GetLogger(r), "Card number is %d", cardNum);
    
              if (LCD_Driver_SendStatusChangeNotification(d,
                                                         sl)) {
                DBG_ERROR(0, "Error sending status change notification");
              }
              else {
                DBG_INFO(0, "Server informed about status change");
              }
              LCD_Slot_SetLastStatus(sl, newStatus);
            }
            else {
              DBG_DEBUG(LCD_Reader_GetLogger(r), "Status on slot %d unchanged",
                         LCD_Slot_GetSlotNum(sl));
            }
          }
          sl=LCD_Slot_List_Next(sl);
        } /* while slots */
      } /* if getting reader status worked */
    } /* if reader is up */
    r=rnext;
  } /* while reader */

  return 0;
}



int LCD_Driver_Work(LCD_DRIVER *d) {
  time_t lastStatusCheckTime;
  time_t t1;

  lastStatusCheckTime=(time_t)0;
  while(!d->stopDriver) {
    GWEN_NETLAYER_RESULT res;
    GWEN_TYPE_UINT32 rid;
    int needHeartbeat;

    t1=time(0);
    if (difftime(t1, lastStatusCheckTime)>=1) {
      /* Do some hardware work */
      DBG_VERBOUS(0, "Checking for status changes");
      LCD_Driver_CheckStatusChanges(d);
      lastStatusCheckTime=t1;
    }

    needHeartbeat=0;
    while(!needHeartbeat) {
      int j;

      t1=time(0);
      if (difftime(t1, lastStatusCheckTime)>=1) {
        /* Do some hardware work */
        DBG_VERBOUS(0, "Checking for status changes");
        LCD_Driver_CheckStatusChanges(d);
        lastStatusCheckTime=t1;
      }
      for(j=0; ; j++) {
        int rv;

        if (j>LCD_DRIVER_IPC_MAXWORK) {
          DBG_ERROR(0, "IPC running wild, aborting driver");
          return -1;
        }
        t1=time(0);
        if (difftime(t1, lastStatusCheckTime)>=1) {
          /* Do some hardware work */
          DBG_VERBOUS(0, "Checking for status changes");
          LCD_Driver_CheckStatusChanges(d);
          lastStatusCheckTime=t1;
        }
        /* work as long as possible */
        rv=GWEN_IpcManager_Work(d->ipcManager);
        if (rv==-1) {
          DBG_ERROR(0, "Error while working with IPC");
          return -1;
        }
        else if (rv==1)
          break;
      }

      rid=LCD_Driver_GetNextInRequest(d);
      if (rid) {
        GWEN_DB_NODE *dbReq;
        int rv;
        const char *name;

        dbReq=LCD_Driver_GetInRequestData(d, rid);
        assert(dbReq);

        /* we have an incoming message */
        name=GWEN_DB_GetCharValue(dbReq, "ipc/cmd", 0, 0);
        if (!name) {
          DBG_ERROR(0, "Bad IPC command (no command name), discarding");
          LCD_Driver_RemoveCommand(d, rid, 0);
        }
        rv=LCD_Driver_HandleRequest(d, rid, name, dbReq);
        if (rv==1) {
          DBG_WARN(0, "Unknown command \"%s\", discarding", name);
          if (GWEN_IpcManager_RemoveRequest(d->ipcManager, rid, 0)) {
            DBG_ERROR(0, "Could not remove request");
            abort();
          }
        }
        else if (rv==-1) {
          DBG_ERROR(0, "Error while handling request, going down");
          return -1;
        }
        else {
          for(j=0; ; j++) {
            int rv;

            if (j>LCD_DRIVER_IPC_MAXWORK) {
              DBG_ERROR(0, "IPC running wild, aborting driver");
              return -1;
            }

            /* work as long as possible (flush responses) */
            rv=GWEN_IpcManager_Work(d->ipcManager);
            if (rv==-1) {
              DBG_ERROR(0, "Error while working with IPC");
              return -1;
            }
            else if (rv==1)
              break;
          }
        } /* if something done */
      } /* if incoming request */
      else
        needHeartbeat=1;
    } /* while !needHeartbeat */

    res=GWEN_Net_HeartBeat(750);
    if (res==GWEN_NetLayerResult_Error) {
      DBG_ERROR(0, "Network error");
      return -1;
    }
    else if (res==GWEN_NetLayerResult_Idle) {
      DBG_VERBOUS(0, "No activity");
    }
  } /* while driver is not to be stopped */
  return 0;
}



