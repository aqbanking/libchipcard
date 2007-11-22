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



int LCD_Driver_WaitForNextResponse(LCD_DRIVER *d,
				   uint32_t rqid,
				   GWEN_DB_NODE **pDbRsp,
				   int timeout) {
  time_t startt;
  GWEN_IO_LAYER_WORKRESULT res;

  startt=time(0);

  for (;;) {
    int rv;
    int done=0;
    GWEN_DB_NODE *dbRsp;

    /* let io manager work until there is nothing to do */
    while((res=GWEN_Io_Manager_Work())==GWEN_Io_Layer_WorkResultOk)
      done=1;
    if (res==GWEN_Io_Layer_WorkResultError) {
      DBG_ERROR(0, "Error working on io layers");
      return GWEN_ERROR_IO;
    }

    /* let ipc manager work until there is nothing to do */
    while((rv=GWEN_IpcManager_Work(d->ipcManager))==0)
      done=1;
    if (rv<0) {
      DBG_ERROR(0, "Error working on ipc");
      return rv;
    }

    /* get next response */
    dbRsp=GWEN_IpcManager_GetResponseData(d->ipcManager, rqid);
    if (dbRsp) {
      DBG_VERBOUS(LC_LOGDOMAIN, "Got a response to request \"%08x\"", rqid);
      *pDbRsp=dbRsp;
      return 0;
    }

    if (done==0) {
      int d;
      int secs;

      /* check timeout */
      d=(int)difftime(time(0), startt);
      if (timeout!=GWEN_TIMEOUT_FOREVER) {
	if (timeout==GWEN_TIMEOUT_NONE ||
	    d>timeout) {
	  DBG_INFO(GWEN_LOGDOMAIN,
		   "Timeout (%d) while waiting, giving up",
		   timeout);
	  return GWEN_ERROR_TRY_AGAIN;
	}
      }

      /* calculate waiting time */
      if (timeout==GWEN_TIMEOUT_FOREVER)
	secs=(timeout/1000)-d;
      else
	secs=5;

      /* actually wait for changes in io */
      GWEN_Io_Manager_Wait(secs*1000, 0);
    }
  } /* for */
}



int LCD_Driver_CheckStatusChanges(LCD_DRIVER *d) {
  LCD_READER *r;

  r=LCD_Reader_List_First(LCD_Driver_GetReaders(d));
  while(r) {
    LCD_READER *rnext;
    uint32_t retval;

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
          uint32_t newStatus, oldStatus;
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

  assert(d);

  lastStatusCheckTime=(time_t)0;
  while(!d->stopDriver) {
    GWEN_IO_LAYER_WORKRESULT res;
    uint32_t rid;

    t1=time(0);
    if (difftime(t1, lastStatusCheckTime)>=1) {
      /* Do some hardware work */
      DBG_VERBOUS(0, "Checking for status changes");
      LCD_Driver_CheckStatusChanges(d);
      lastStatusCheckTime=t1;
    }

    while(!d->stopDriver) {
      int rv;
      int done=0;
  
      /* let io manager work until there is nothing to do */
      while((res=GWEN_Io_Manager_Work())==GWEN_Io_Layer_WorkResultOk)
	done=1;
      if (res==GWEN_Io_Layer_WorkResultError) {
	DBG_ERROR(LC_LOGDOMAIN, "Error working on io layers");
	return -1;
      }
  
      /* let ipc manager work until there is nothing to do */
      while((rv=GWEN_IpcManager_Work(d->ipcManager))==0)
	done=1;
      if (rv<0) {
	DBG_ERROR(LC_LOGDOMAIN, "Error working on ipc");
	return rv;
      }

      t1=time(0);
      if (difftime(t1, lastStatusCheckTime)>=1) {
	/* Do some hardware work */
	DBG_VERBOUS(0, "Checking for status changes");
	LCD_Driver_CheckStatusChanges(d);
	lastStatusCheckTime=t1;
        done=1;
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
	else if (rv<0) {
	  DBG_ERROR(0, "Error while handling request, going down");
	  return rv;
	}
	else
	  /* something has been done */
	  done=1;
      } /* if incoming request */

      if (done==0) {
	/* actually wait for changes in io */
	GWEN_Io_Manager_Wait(750, 0);
      }
    } /* for */
  } /* while driver is not to be stopped */

  return 0;
}



