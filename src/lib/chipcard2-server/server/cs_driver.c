/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id$
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "cardserver_p.h"
#include "serverconn_l.h"
#include "card_l.h"
#include <gwenhywfar/version.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/ipc.h>
#include <gwenhywfar/nettransportssl.h>
#include <gwenhywfar/nettransportsock.h>
#include <gwenhywfar/net.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/directory.h>

#include <chipcard2/chipcard2.h>
#include <chipcard2-server/server/usbmonitor.h>
#include <chipcard2-server/server/usbttymonitor.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>


#ifdef OS_WIN32
# define DIRSEP "\\"
# define DIRSEPC '\\'
#else
# define DIRSEP "/"
# define DIRSEPC '/'
#endif



void LC_CardServer_ReaderDown(LC_CARDSERVER *cs, LC_READER *r,
                              LC_READER_STATUS newReaderStatus,
                              const char *reason) {
  LC_READER_STATUS rst;
  LC_CARD *card;
  LC_REQUEST *rq;

  rst=LC_Reader_GetStatus(r);

  /* remove all pending incoming requests for the reader */
  while((rq=LC_Reader_GetNextRequest(r))) {
    LC_CardServer_SendErrorResponse(cs,
                                    LC_Request_GetInRequestId(rq),
                                    LC_ERROR_CARD_REMOVED,
                                    "Reader is down");
    /* needed because the following fn unlinks it */
    DBG_INFO(0, "Aborting readers' request %08x",
             LC_Request_GetRequestId(rq));
    LC_Request_List_Insert(rq, cs->requests);
    LC_CardServer_RemoveRequest(cs, rq);
  } /* while */

  /* mark all cards for this reader as removed */
  card=LC_Card_List_First(cs->activeCards);
  while(card) {
    if (LC_Card_GetReader(card)==r) {
      LC_CardServer_CardDown(cs, card, LC_CardStatusRemoved,
                             "Reader is down");
    }
    card=LC_Card_List_Next(card);
  }
  card=LC_Card_List_First(cs->freeCards);
  while(card) {
    if (LC_Card_GetReader(card)==r) {
      LC_CardServer_CardDown(cs, card, LC_CardStatusRemoved,
                             "Reader down");
    }
    card=LC_Card_List_Next(card);
  }

  if (rst!=newReaderStatus) {
    const char *code;

    switch(newReaderStatus) {
    case LC_ReaderStatusDown:
      code=LC_NOTIFY_CODE_READER_DOWN;
      break;
  
    case LC_ReaderStatusAborted:
    case LC_ReaderStatusDisabled:
      code=LC_NOTIFY_CODE_READER_ERROR;
      break;
  
    default:
      code=LC_NOTIFY_CODE_READER_ERROR;
    }

    LC_CardServer_SendReaderNotification(cs, 0,
                                         code,
                                         r,
                                         reason);
    LC_Reader_SetStatus(r, newReaderStatus);
  }
}



void LC_CardServer_DriverDown(LC_CARDSERVER *cs, LC_DRIVER *d,
                              LC_DRIVER_STATUS newDriverStatus,
                              const char *reason) {
  GWEN_TYPE_UINT32 nid;
  LC_READER *r;

  assert(cs);
  assert(d);

  nid=LC_Driver_GetIpcId(d);

  /* adjust status of all readers of this driver */
  r=LC_Reader_List_First(cs->readers);
  while(r) {
    LC_READER *next;

    next=LC_Reader_List_Next(r);

    if (LC_Reader_GetDriver(r)==d) {
      LC_READER_STATUS rst;
      LC_READER_STATUS nrst;

      rst=LC_Reader_GetStatus(r);
      switch(rst) {
      case LC_ReaderStatusDown:
      case LC_ReaderStatusAborted:
      case LC_ReaderStatusDisabled:
        nrst=rst;
        break;

      case LC_ReaderStatusWaitForDriver:
      case LC_ReaderStatusWaitForReaderUp:
      case LC_ReaderStatusWaitForReaderDown:
      case LC_ReaderStatusUp:
      default:
        nrst=LC_ReaderStatusAborted;
      }
      LC_CardServer_ReaderDown(cs, r, nrst,
                               "Driver is down, aborting reader");

      if (LC_Driver_GetDriverFlags(d) & LC_DRIVER_FLAGS_REMOTE) {
        DBG_INFO(0, "Removing reader \"%s\"",
                 LC_Reader_GetReaderName(r));
        LC_Driver_DecAssignedReadersCount(LC_Reader_GetDriver(r));
        LC_Reader_List_Del(r);
        LC_Reader_free(r);
      }
    }
    r=next;
  } /* while */

  if (nid) {
    int rv;

    rv=GWEN_IPCManager_RemoveClient(cs->ipcManager, nid);
    if (rv) {
      DBG_WARN(0, "Error removing IPC node of driver \"%08x\"",
               LC_Driver_GetDriverId(d));
    }
    LC_Driver_SetIpcId(d, 0);
  }
  LC_CardServer_SendDriverNotification(cs, 0,
                                       LC_NOTIFY_CODE_DRIVER_DOWN,
                                       d,
                                       reason);
  LC_Driver_SetStatus(d, newDriverStatus);
}




int LC_CardServer_StartReader(LC_CARDSERVER *cs, LC_READER *r) {
  LC_READER_STATUS st;
  LC_DRIVER *d;
  LC_DRIVER_STATUS dst;

  assert(cs);
  assert(r);

  if (!LC_Reader_IsAvailable(r)) {
    DBG_INFO(0, "Not starting reader \"%s\", not available",
             LC_Reader_GetReaderName(r));
    return -1;
  }
  DBG_INFO(0, "Starting reader \"%s\"", LC_Reader_GetReaderName(r));
  st=LC_Reader_GetStatus(r);

  /* check for reader status */
  if (st==LC_ReaderStatusWaitForDriver ||
      st==LC_ReaderStatusWaitForReaderUp ||
      st==LC_ReaderStatusUp) {
    DBG_INFO(0, "Reader \"%s\" already started",
             LC_Reader_GetReaderName(r));
    return 0;
  }
  else if (st==LC_ReaderStatusWaitForReaderDown) {
    DBG_ERROR(0,
              "Reader \"%s\" is in transition, "
              "postponing start",
              LC_Reader_GetReaderName(r));
    LC_Reader_SetWantRestart(r, 1);
    return 0;
  }
  else if (st!=LC_ReaderStatusDown) {
    DBG_ERROR(0, "Bad reader status (%d)", st);
    return -1;
  }

  /* check for driver status */
  d=LC_Reader_GetDriver(r);
  assert(d);
  dst=LC_Driver_GetStatus(d);

  if (dst==LC_DriverStatusDown) {
    /* driver is down, start it */
    if (LC_CardServer_StartDriver(cs, d)) {
      DBG_ERROR(0, "Could not start driver for reader \"%s\"",
                LC_Reader_GetReaderName(r));
      return -1;
    }
  }
  else if (dst==LC_DriverStatusStopping) {
    DBG_ERROR(0,
              "Driver for reader \"%s\" is in transition (%d), "
              "postponing start",
              LC_Reader_GetReaderName(r), dst);
    LC_Reader_SetWantRestart(r, 1);
    return 0;
  }
  else if (dst!=LC_DriverStatusUp &&
           dst!=LC_DriverStatusStarted) {
    DBG_ERROR(0,
              "Driver for reader \"%s\" has a bad status (%d), "
              "not starting reader",
              LC_Reader_GetReaderName(r),
              dst);
    return -1;
  }

  /* attach to driver */
  LC_Driver_IncActiveReadersCount(d);
  LC_Reader_SetStatus(r, LC_ReaderStatusWaitForDriver);
  LC_CardServer_SendReaderNotification(cs, 0,
                                       LC_NOTIFY_CODE_READER_START,
                                       r, "Reader started");
  return 0;
}



int LC_CardServer_StopReader(LC_CARDSERVER *cs, LC_READER *r) {
  GWEN_TYPE_UINT32 rid;
  LC_DRIVER *d;

  assert(r);
  d=LC_Reader_GetDriver(r);
  assert(d);
  if (LC_Driver_GetStatus(d)!=LC_DriverStatusUp) {
    DBG_INFO(0, "Driver not up, so there is no reader to be stopped");
    return 0;
  }
  rid=LC_CardServer_SendStopReader(cs, r);
  if (!rid) {
    DBG_ERROR(0, "Could not send StopReader command for reader \"%s\"",
              LC_Reader_GetReaderName(r));
    LC_Driver_DecActiveReadersCount(LC_Reader_GetDriver(r));
    LC_CardServer_ReaderDown(cs, r, LC_ReaderStatusAborted,
                             "Could not send StopReader command");
    return -1;
  }
  DBG_DEBUG(0, "Sent StopReader request for reader \"%s\"",
            LC_Reader_GetReaderName(r));
  LC_Reader_SetCurrentRequestId(r, rid);
  LC_Reader_SetStatus(r, LC_ReaderStatusWaitForReaderDown);
  return 0;
}



int LC_CardServer_CheckReader(LC_CARDSERVER *cs, LC_READER *r) {
  LC_READER_STATUS st;
  LC_DRIVER *d;
  int handled;
  int i;
  int couldDoSomething;

  assert(cs);
  assert(r);

  handled=0;
  couldDoSomething=0;
  d=LC_Reader_GetDriver(r);
  st=LC_Reader_GetStatus(r);
  DBG_DEBUG(0, "Reader Status is %d", st);

  /* remove reader if AUTO and not available */
  if (!LC_Reader_IsAvailable(r) &&
      (LC_Reader_GetFlags(r) & LC_READER_FLAGS_AUTO)) {
    DBG_INFO(0, "Removing reader \"%s\"",
             LC_Reader_GetReaderName(r));
    LC_CardServer_ReaderDown(cs, r, LC_ReaderStatusAborted,
                             "Reader unplugged");
    LC_Driver_DecAssignedReadersCount(LC_Reader_GetDriver(r));
    LC_Reader_List_Del(r);
    LC_Reader_free(r);
    return 0;
  }

  if (st==LC_ReaderStatusAborted ||
      st==LC_ReaderStatusDisabled) {
    GWEN_TYPE_UINT32 rid;

    handled=1;
    rid=LC_Reader_GetCurrentRequestId(r);
    if (rid) {
      GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
      LC_Reader_SetCurrentRequestId(r, 0);
    }

    /* check whether we may reenable the reader */
    if (LC_Reader_IsAvailable(r)) {
      if (cs->readerRestartTime &&
          difftime(time(0), LC_Reader_GetLastStatusChangeTime(r))
          >
          cs->readerRestartTime) {
        DBG_NOTICE(0, "Reenabling reader \"%s\"",
                   LC_Reader_GetReaderName(r));
        LC_Reader_SetStatus(r, LC_ReaderStatusDown);
        couldDoSomething++;
      }
    } /* if isAvailable */
    else {
      if (LC_Reader_GetUsageCount(r)==0) {
	DBG_NOTICE(0, "Removing unused and unavailable reader \"%s\"",
		   LC_Reader_GetReaderName(r));
        LC_Reader_List_Del(r);
        LC_Driver_DecAssignedReadersCount(d);
	LC_Reader_free(r);
        return 0;
      }
    }
  }

  if (LC_Reader_GetStatus(r)==LC_ReaderStatusDown) {
    handled=1;
    if (LC_Reader_IsAvailable(r)) {
      if (LC_Reader_GetUsageCount(r)>0) {
        DBG_INFO(0, "Trying to start reader \"%s\" (is now in use)",
                 LC_Reader_GetReaderName(r));
        if (LC_CardServer_StartReader(cs, r)) {
          DBG_ERROR(0, "Could not start reader \"%s\"",
                    LC_Reader_GetReaderName(r));
          return -1;
        }
        couldDoSomething++;
      }
    }
    /* check for delayed start */
    if (LC_Reader_GetStatus(r)==LC_ReaderStatusDown &&
        LC_Reader_IsAvailable(r) &&
        LC_Reader_GetWantRestart(r)) {
      int rv;

        DBG_ERROR(0, "Delayed start of reader \"%s\"",
                  LC_Reader_GetReaderName(r));
        rv=LC_CardServer_StartReader(cs, r);
        if (rv) {
          DBG_INFO(0, "here");
          return -1;
        }
        LC_Reader_SetWantRestart(r, 0);
        couldDoSomething++;
    } /* if wantRestart */
  } /* if status is DOWN */

  if (LC_Reader_GetStatus(r)==LC_ReaderStatusWaitForDriver) {
    LC_DRIVER_STATUS dst;

    handled=1;
    /* check for driver */
    dst=LC_Driver_GetStatus(d);
    if (dst==LC_DriverStatusUp) {
      GWEN_TYPE_UINT32 rid;

      /* send StartReader command */
      rid=LC_CardServer_SendStartReader(cs, r);
      if (!rid) {
        DBG_ERROR(0,
                  "Could not send StartReader command "
                  "for reader \"%s\"",
                  LC_Reader_GetReaderName(r));
        LC_Driver_DecActiveReadersCount(LC_Reader_GetDriver(r));
        LC_CardServer_ReaderDown(cs, r, LC_ReaderStatusAborted,
                                 "Could not send StartReader "
                                 "command");
        return -1;
      }
      DBG_DEBUG(0, "Sent StartReader request for reader \"%s\"",
                LC_Reader_GetReaderName(r));
      LC_Reader_SetCurrentRequestId(r, rid);
      LC_Reader_SetStatus(r, LC_ReaderStatusWaitForReaderUp);
      couldDoSomething++;
    }
    else if (dst==LC_DriverStatusStarted) {
      /* driver started, check timeout */
      if (cs->readerStartTimeout &&
          difftime(time(0), LC_Reader_GetLastStatusChangeTime(r))>=
          cs->readerStartTimeout) {
        /* reader timed out */
        DBG_WARN(0, "Reader \"%s\" timed out", LC_Reader_GetReaderName(r));
        LC_Driver_DecActiveReadersCount(LC_Reader_GetDriver(r));
        LC_CardServer_ReaderDown(cs, r, LC_ReaderStatusAborted,
                                 "Reader timed out");
        return -1;
      } /* if timeout */

      /* otherwise the reader has some time left */
      return 1;
    }
    else {
      /* bad status, abort reader */
      DBG_WARN(0, "Reader \"%s\" aborted due to driver status (%d)",
               LC_Reader_GetReaderName(r), dst);
        LC_Driver_DecActiveReadersCount(LC_Reader_GetDriver(r));
      LC_CardServer_ReaderDown(cs, r, LC_ReaderStatusAborted,
                               "Reader aborted (driver status)");
      return -1;
    }
  } /* if WaitForDriver */

  if (LC_Reader_GetStatus(r)==LC_ReaderStatusWaitForReaderUp) {
    LC_DRIVER_STATUS dst;
    GWEN_TYPE_UINT32 rid;
    GWEN_DB_NODE *dbRsp;

    handled=1;
    /* check for driver */
    dst=LC_Driver_GetStatus(d);
    if (dst!=LC_DriverStatusUp) {
      DBG_ERROR(0, "Bad driver status, aborting reader startup");
      LC_Driver_DecActiveReadersCount(LC_Reader_GetDriver(r));
      LC_CardServer_ReaderDown(cs, r, LC_ReaderStatusAborted,
                               "Reader aborted (driver status)");
      return -1;
    }

    DBG_DEBUG(0, "Checking for command status");
    rid=LC_Reader_GetCurrentRequestId(r);
    assert(rid);
    dbRsp=GWEN_IPCManager_GetResponseData(cs->ipcManager, rid);
    if (dbRsp) {
      if (strcasecmp(GWEN_DB_GroupName(dbRsp), "error")==0) {
	GWEN_BUFFER *ebuf;
        const char *e;

	e=GWEN_DB_GetCharValue(dbRsp, "text", 0, 0);
	DBG_ERROR(0,
                  "Driver reported error on reader startup: %d (%s)",
		  GWEN_DB_GetIntValue(dbRsp, "code", 0, 0),
		  e?e:"<none>");
	ebuf=GWEN_Buffer_new(0, 256, 0, 1);
	if (e) {
	  GWEN_Buffer_AppendString(ebuf, "Reader error (");
	  GWEN_Buffer_AppendString(ebuf, e);
	  GWEN_Buffer_AppendString(ebuf, ")");
	}
	else {
	  GWEN_Buffer_AppendString(ebuf, "Reader error (startup)");
	}
        LC_Driver_DecActiveReadersCount(LC_Reader_GetDriver(r));
        LC_CardServer_ReaderDown(cs, r, LC_ReaderStatusAborted,
                                 GWEN_Buffer_GetStart(ebuf));
	GWEN_Buffer_free(ebuf);
        GWEN_DB_Group_free(dbRsp);
        GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 1);
        return -1;
      }
      else {
        const char *s;
        const char *e;

        s=GWEN_DB_GetCharValue(dbRsp, "body/code", 0, 0);
        assert(s);
        e=GWEN_DB_GetCharValue(dbRsp, "body/text", 0, 0);
        if (strcasecmp(s, "error")==0) {
          GWEN_BUFFER *ebuf;

          DBG_ERROR(0,
                    "Driver reported error on startup of reader \"%s\": %s",
                    LC_Reader_GetReaderName(r),
                    e?e:"(none)");
          ebuf=GWEN_Buffer_new(0, 256, 0, 1);
          if (e) {
            GWEN_Buffer_AppendString(ebuf, "Reader error (");
            GWEN_Buffer_AppendString(ebuf, e);
            GWEN_Buffer_AppendString(ebuf, ")");
          }
          else {
            GWEN_Buffer_AppendString(ebuf, "Reader error (startup)");
          }
          LC_Driver_DecActiveReadersCount(LC_Reader_GetDriver(r));
          LC_CardServer_ReaderDown(cs, r, LC_ReaderStatusAborted,
                                   GWEN_Buffer_GetStart(ebuf));
          GWEN_Buffer_free(ebuf);
          GWEN_DB_Group_free(dbRsp);
          GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 1);
          return -1;
        }
        else {
          const char *readerInfo;

          readerInfo=GWEN_DB_GetCharValue(dbRsp, "body/info", 0, 0);
          if (readerInfo) {
            DBG_NOTICE(0, "Reader \"%s\" is up (%s), info: %s",
                       LC_Reader_GetReaderName(r),
                       e?e:"no result text", readerInfo);
            LC_Reader_SetReaderInfo(r, readerInfo);
          }
          else {
            DBG_NOTICE(0, "Reader \"%s\" is up (%s), no info",
                       LC_Reader_GetReaderName(r),
                       e?e:"no result text");
          }
          LC_Reader_SetStatus(r, LC_ReaderStatusUp);
          LC_CardServer_SendReaderNotification(cs, 0,
                                               LC_NOTIFY_CODE_READER_UP,
                                               r,
                                               e?e:"Reader is up");
          GWEN_DB_Group_free(dbRsp);
          GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 1);
          LC_Reader_SetCurrentRequestId(r, 0);
        }
        couldDoSomething++;
      }
    }
    else {
      if (cs->readerStartTimeout &&
          difftime(time(0), LC_Reader_GetLastStatusChangeTime(r))>=
          cs->readerStartTimeout) {
        /* reader timed out */
        DBG_WARN(0, "Reader \"%s\" timed out", LC_Reader_GetReaderName(r));
        GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 1);
        LC_Reader_SetCurrentRequestId(r, 0);
        LC_Driver_DecActiveReadersCount(LC_Reader_GetDriver(r));
        LC_CardServer_ReaderDown(cs, r, LC_ReaderStatusAborted,
                                 "Reader error (timeout)");
        return -1;
      } /* if timeout */
      DBG_DEBUG(0, "Still some time left");
      return 1;
    }
  } /* if waitForReader */

  if (LC_Reader_GetStatus(r)==LC_ReaderStatusUp) {
    LC_DRIVER_STATUS dst;

    handled=1;
    if (!LC_Reader_IsAvailable(r)) {
      DBG_NOTICE(0, "Reader became unavailable, shutting it down");
      LC_Driver_DecActiveReadersCount(LC_Reader_GetDriver(r));
      LC_CardServer_ReaderDown(cs, r, LC_ReaderStatusAborted,
                               "Reader became unavailable");
      LC_CardServer_StopReader(cs, r);
      return -1;
    }

    /* check for driver */
    dst=LC_Driver_GetStatus(d);
    if (dst!=LC_DriverStatusUp) {
      DBG_ERROR(0, "Bad driver status, aborting reader command");
      LC_Driver_DecActiveReadersCount(LC_Reader_GetDriver(r));
      LC_CardServer_ReaderDown(cs, r, LC_ReaderStatusAborted,
                               "Reader error driver status)");
      return -1;
    }

    i=LC_Driver_GetPendingCommandCount(d);
    if (i>=LC_CARDSERVER_DRVER_MAX_PENDING_COMMANDS) {
      DBG_ERROR(0, "Too many pending commands(%d), waiting", i);
    }

    for (i=LC_Driver_GetPendingCommandCount(d);
         i<LC_CARDSERVER_DRVER_MAX_PENDING_COMMANDS;) {
      LC_REQUEST *rq;

      /* get the next command from queue */
      rq=LC_Reader_GetNextRequest(r);
      if (rq) {
	GWEN_TYPE_UINT32 rid;
        GWEN_DB_NODE *dbReq;
	const char *cmdName;

        /* found a request */
	LC_Request_List_Add(rq, cs->requests);
        dbReq=LC_Request_GetOutRequestData(rq);
        assert(dbReq);
        cmdName=GWEN_DB_GroupName(dbReq);
        assert(cmdName);
	rid=GWEN_IPCManager_SendRequest(cs->ipcManager,
					LC_Driver_GetIpcId(d),
                                        GWEN_DB_Group_dup(dbReq));
	if (!rid) {
	  GWEN_BUFFER *ebuf;

	  DBG_ERROR(0, "Could not send command \"%s\" for reader \"%s\"",
                    cmdName,
		    LC_Reader_GetReaderName(r));
	  ebuf=GWEN_Buffer_new(0, 256, 0, 1);
	  GWEN_Buffer_AppendString(ebuf, "Could not send command \"");
	  GWEN_Buffer_AppendString(ebuf, cmdName);
	  GWEN_Buffer_AppendString(ebuf, "\"");
          LC_Driver_DecActiveReadersCount(LC_Reader_GetDriver(r));
          LC_CardServer_ReaderDown(cs, r, LC_ReaderStatusAborted,
                                   GWEN_Buffer_GetStart(ebuf));
	  GWEN_Buffer_free(ebuf);
	  return -1;
	}
        DBG_DEBUG(0, "Sent request \"%s\" for reader \"%s\" (%08x)",
                  cmdName,
                  LC_Reader_GetReaderName(r),
                  rid);
        LC_Request_SetOutRequestId(rq, rid);
        LC_Reader_SetCurrentRequestId(r, LC_Request_GetRequestId(rq));
        LC_Driver_IncPendingCommandCount(d);
        couldDoSomething++;
      } /* if there was a next request */
      else
        break;
    } /* for */

    if (LC_Reader_GetUsageCount(r)==0 && cs->readerIdleTimeout) {
      time_t t;

      /* check for idle timeout */
      t=LC_Reader_GetIdleSince(r);
      assert(t);

      if (cs->readerIdleTimeout &&
          difftime(time(0), t)>cs->readerIdleTimeout) {
        if (LC_CardServer_StopReader(cs, r)) {
	  DBG_INFO(0, "Could not stop reader \"%s\"",
		   LC_Reader_GetReaderName(r));
	  return -1;
	}
	couldDoSomething++;
      } /* if timeout */
      /* otherwise reader is not idle */
      return 1;
    }
  }
  if (LC_Reader_GetStatus(r)==LC_ReaderStatusWaitForReaderDown) {
    GWEN_DB_NODE *dbResp;
    GWEN_TYPE_UINT32 rid;

    handled=1;
    rid=LC_Reader_GetCurrentRequestId(r);
    assert(rid);
    dbResp=GWEN_IPCManager_GetResponseData(cs->ipcManager, rid);
    if (dbResp) {
      /* TODO: Check for result */
      GWEN_DB_Group_free(dbResp);
      GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 1);
      DBG_NOTICE(0, "Reader \"%s\" is down as expected",
                 LC_Reader_GetReaderName(r));
      LC_Driver_DecActiveReadersCount(LC_Reader_GetDriver(r));
      LC_CardServer_ReaderDown(cs, r, LC_ReaderStatusDown,
                               "Reader is down (as expected)");
      couldDoSomething++;
      return 0;
    }
    else {
      if (cs->readerStartTimeout &&
          difftime(time(0), LC_Reader_GetLastStatusChangeTime(r))>=
          cs->readerStartTimeout) {
        /* reader timed out */
        DBG_WARN(0, "Reader \"%s\" timed out", LC_Reader_GetReaderName(r));
        GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 1);
        LC_Reader_SetCurrentRequestId(r, 0);
        if (LC_CardServer_StopDriver(cs, d)) {
          DBG_WARN(0, "Could not stop driver for reader \"%s\"",
                   LC_Reader_GetReaderName(r));
        }
        LC_Driver_DecActiveReadersCount(LC_Reader_GetDriver(r));
        LC_CardServer_ReaderDown(cs, r, LC_ReaderStatusAborted,
                                 "Reader error (timeout)");
        return -1;
      }
      else {
        /* still some time left */
        return 1;
      }
    }
  } /* if reader waiting for down command response */

  if (!handled) {
    DBG_ERROR(0, "Unknown reader status %d for reader \"%s\"", st,
              LC_Reader_GetReaderName(r));
    LC_Driver_DecActiveReadersCount(LC_Reader_GetDriver(r));
    LC_CardServer_ReaderDown(cs, r, LC_ReaderStatusAborted,
                             "Reader error "
                             "(unknown reader status)");
    return -1;
  }

  return couldDoSomething?0:1;
}



int LC_CardServer_StartDriver(LC_CARDSERVER *cs, LC_DRIVER *d) {
  LC_DRIVER_STATUS st;
  GWEN_PROCESS *p;
  GWEN_BUFFER *pbuf;
  GWEN_BUFFER *abuf;
  const char *s;
  char numbuf[32];
  int rv;
  GWEN_PROCESS_STATE pst;

  assert(cs);
  assert(d);

  DBG_INFO(0, "Starting driver \"%s\"", LC_Driver_GetDriverName(d));
  st=LC_Driver_GetStatus(d);
  if (st!=LC_DriverStatusDown) {
    DBG_ERROR(0, "Bad driver status (%d)", st);
    return -1;
  }

  abuf=GWEN_Buffer_new(0, 128, 0, 1);

  s=LC_Driver_GetDriverDataDir(d);
  if (s) {
    GWEN_Buffer_AppendString(abuf, "-d ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  s=LC_Driver_GetLibraryFile(d);
  if (s) {
    GWEN_Buffer_AppendString(abuf, " -l ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  s=LC_Driver_GetLogFile(d);
  if (s) {
    GWEN_Buffer_AppendString(abuf, " --logtype file");
    GWEN_Buffer_AppendString(abuf, " --logfile ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  GWEN_Buffer_AppendString(abuf, " --loglevel info");

  if (cs->typeForDrivers) {
    GWEN_Buffer_AppendString(abuf, " -t ");
    GWEN_Buffer_AppendString(abuf, cs->typeForDrivers);
  }

  if (cs->addrForDrivers) {
    GWEN_Buffer_AppendString(abuf, " -a ");
    GWEN_Buffer_AppendString(abuf, cs->addrForDrivers);
  }

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%d", cs->portForDrivers);
  assert(rv>0 && rv<sizeof(numbuf));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_Buffer_AppendString(abuf, " -p ");
  GWEN_Buffer_AppendString(abuf, numbuf);

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x", LC_Driver_GetDriverId(d));
  assert(rv>0 && rv<sizeof(numbuf));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_Buffer_AppendString(abuf, " -i ");
  GWEN_Buffer_AppendString(abuf, numbuf);

  s=LC_Driver_GetDriverType(d);
  if (!s) {
    DBG_ERROR(0, "No driver type");
    LC_CardServer_DriverDown(cs, d, LC_DriverStatusDisabled,
                             "No driver type");
    GWEN_Buffer_free(abuf);
    return -1;
  }

  pbuf=GWEN_Buffer_new(0, 128, 0, 1);
  GWEN_Buffer_AppendString(pbuf, LC_DEVICEDRIVER_PATH);
  GWEN_Buffer_AppendByte(pbuf, '/');
  while(*s) {
    GWEN_Buffer_AppendByte(pbuf, tolower(*s));
    s++;
  } /* while */

  p=GWEN_Process_new();
  DBG_INFO(0, "Starting driver process for driver \"%s\" (%s)",
           LC_Driver_GetDriverName(d), GWEN_Buffer_GetStart(pbuf));
  DBG_INFO(0, "Arguments are: \"%s\"", GWEN_Buffer_GetStart(abuf));

  pst=GWEN_Process_Start(p,
                         GWEN_Buffer_GetStart(pbuf),
                         GWEN_Buffer_GetStart(abuf));
  if (pst!=GWEN_ProcessStateRunning) {
    DBG_ERROR(0, "Unable to execute \"%s %s\"",
              GWEN_Buffer_GetStart(pbuf),
              GWEN_Buffer_GetStart(abuf));
    GWEN_Process_free(p);
    GWEN_Buffer_free(pbuf);
    GWEN_Buffer_free(abuf);
    return -1;
  }
  DBG_INFO(0, "Process started");
  LC_CardServer_SendDriverNotification(cs, 0,
				       LC_NOTIFY_CODE_DRIVER_START,
                                       d, "Driver started");
  GWEN_Buffer_free(pbuf);
  GWEN_Buffer_free(abuf);
  LC_Driver_SetProcess(d, p);
  LC_Driver_SetStatus(d, LC_DriverStatusStarted);
  return 0;
}




int LC_CardServer_StopDriver(LC_CARDSERVER *cs, LC_DRIVER *d) {
  GWEN_TYPE_UINT32 rid;

  assert(d);
  rid=LC_CardServer_SendStopDriver(cs, d);
  if (!rid) {
    DBG_ERROR(0, "Could not send StopDriver command for driver \"%08x\"",
              LC_Driver_GetDriverId(d));
    LC_CardServer_DriverDown(cs, d, LC_DriverStatusAborted,
                             "Could not send StopDriver command");
    return -1;
  }
  DBG_DEBUG(0, "Sent StopDriver request for driver \"%08x\"",
            LC_Driver_GetDriverId(d));
  LC_Driver_SetStatus(d, LC_DriverStatusStopping);
  return 0;
}



int LC_CardServer_CheckDriver(LC_CARDSERVER *cs, LC_DRIVER *d) {
  int done;
  GWEN_TYPE_UINT32 nid;

  assert(cs);
  assert(d);

  done=0;


  nid=LC_Driver_GetIpcId(d);

  if (
      (LC_Driver_GetDriverFlags(d) & LC_DRIVER_FLAGS_REMOTE) ||
      (
       (LC_Driver_GetDriverFlags(d) & LC_DRIVER_FLAGS_AUTO) &&
       (LC_Driver_GetAssignedReadersCount(d)==0)
      )
     ){
    DBG_NOTICE(0, "Driver \"%08x\" is unused, removing it",
               LC_Driver_GetDriverId(d));
    LC_Driver_List_Del(d);
    LC_Driver_free(d);
    return 0;
  }

  if (LC_Driver_GetStatus(d)==LC_DriverStatusAborted) {
    if (cs->driverRestartTime &&
        difftime(time(0), LC_Driver_GetLastStatusChangeTime(d))>=
        cs->driverRestartTime) {
      DBG_NOTICE(0, "Reenabling driver \"%08x\"",
                 LC_Driver_GetDriverId(d));
      LC_Driver_SetStatus(d, LC_DriverStatusDown);
    }
  }

  if (LC_Driver_GetStatus(d)==LC_DriverStatusStopping) {
    GWEN_PROCESS *p;

    p=LC_Driver_GetProcess(d);
    if (p) {
      GWEN_PROCESS_STATE pst;

      pst=GWEN_Process_CheckState(p);
      if (pst==GWEN_ProcessStateRunning) {
        if (cs->driverStopTimeout &&
            difftime(time(0), LC_Driver_GetLastStatusChangeTime(d))>=
            cs->driverStopTimeout) {
          DBG_WARN(0, "Driver is still running, killing it");
          if (GWEN_Process_Terminate(p)) {
            DBG_ERROR(0, "Could not kill process");
          }
          LC_Driver_SetProcess(d, 0);
          LC_CardServer_DriverDown(cs, d, LC_DriverStatusAborted,
                                   "Driver still running, killing it");
          return -1;
        }
        else {
          /* otherwise give the process a little bit time ... */
          DBG_DEBUG(0, "still waiting for driver to go down");
          return 1;
        }
      }
      else if (pst==GWEN_ProcessStateExited) {
        DBG_WARN(0, "Driver terminated normally");
        LC_Driver_SetProcess(d, 0);
        LC_CardServer_DriverDown(cs, d, LC_DriverStatusDown,
                                 "Driver terminated normally");
        return 0;
      }
      else if (pst==GWEN_ProcessStateAborted) {
        DBG_WARN(0, "Driver terminated abnormally");
        LC_Driver_SetProcess(d, 0);
        LC_CardServer_DriverDown(cs, d, LC_DriverStatusAborted,
                                 "Driver terminated abnormally");
        return 0;
      }
      else if (pst==GWEN_ProcessStateStopped) {
        DBG_WARN(0, "Driver has been stopped, killing it");
        if (GWEN_Process_Terminate(p)) {
          DBG_ERROR(0, "Could not kill process");
        }
        LC_Driver_SetProcess(d, 0);
        LC_CardServer_DriverDown(cs, d, LC_DriverStatusAborted,
                                 "Driver has been stopped, killing it");
        return 0;
      }
      else {
        DBG_ERROR(0, "Unknown process status %d, killing", pst);
        if (GWEN_Process_Terminate(p)) {
          DBG_ERROR(0, "Could not kill process");
        }
        LC_Driver_SetProcess(d, 0);
        LC_CardServer_DriverDown(cs, d, LC_DriverStatusAborted,
                                 "Unknown process status, "
                                 "killing");
        return 0;
      }
    } /* if process */
    else {
      if (!(LC_Driver_GetDriverFlags(d) & LC_DRIVER_FLAGS_REMOTE)) {
        DBG_ERROR(0, "No process for local driver:");
        LC_Driver_Dump(d, stderr, 2);
        abort();
      }
    }
  } /* if stopping */


  if (LC_Driver_GetStatus(d)==LC_DriverStatusStarted) {
    /* driver started, check timeout */
    if (cs->driverStartTimeout &&
        difftime(time(0), LC_Driver_GetLastStatusChangeTime(d))>=
        cs->driverStartTimeout) {
      GWEN_PROCESS *p;

      DBG_WARN(0, "Driver \"%s\" timed out", LC_Driver_GetDriverName(d));
      p=LC_Driver_GetProcess(d);
      if (p) {
        GWEN_PROCESS_STATE pst;

        pst=GWEN_Process_CheckState(p);
        if (pst==GWEN_ProcessStateRunning) {
          DBG_WARN(0,
                   "Driver is running but did not signal readyness, "
                   "killing it");
          if (GWEN_Process_Terminate(p)) {
            DBG_ERROR(0, "Could not kill process");
          }
          LC_Driver_SetProcess(d, 0);
          LC_CardServer_DriverDown(cs, d, LC_DriverStatusAborted,
                                   "Driver is running but did not "
                                   "signal readyness, "
                                   "killing it");
          return -1;
        }
        else if (pst==GWEN_ProcessStateExited) {
          DBG_WARN(0, "Driver terminated normally");
          LC_Driver_SetProcess(d, 0);
          LC_CardServer_DriverDown(cs, d, LC_DriverStatusAborted,
                                   "Driver terminated normally");
          return 0;
        }
        else if (pst==GWEN_ProcessStateAborted) {
          DBG_WARN(0, "Driver terminated abnormally");
          LC_Driver_SetProcess(d, 0);
          LC_CardServer_DriverDown(cs, d, LC_DriverStatusAborted,
                                   "Driver terminated abnormally");
          return -1;
        }
        else if (pst==GWEN_ProcessStateStopped) {
          DBG_WARN(0, "Driver has been stopped, killing it");
          if (GWEN_Process_Terminate(p)) {
            DBG_ERROR(0, "Could not kill process");
          }
          LC_Driver_SetProcess(d, 0);
          LC_CardServer_DriverDown(cs, d, LC_DriverStatusAborted,
                                   "Driver has been stopped, "
                                   "killing it");
          return -1;
        }
        else {
          DBG_ERROR(0, "Unknown process status %d, killing", pst);
          if (GWEN_Process_Terminate(p)) {
            DBG_ERROR(0, "Could not kill process");
          }
          LC_Driver_SetProcess(d, 0);
          LC_CardServer_DriverDown(cs, d, LC_DriverStatusAborted,
                                   "Unknown process status, "
                                   "killing");
          return -1;
        }
      } /* if process */
      else {
        if (!(LC_Driver_GetDriverFlags(d) & LC_DRIVER_FLAGS_REMOTE)) {
          DBG_ERROR(0, "No process for local driver:");
          LC_Driver_Dump(d, stderr, 2);
          abort();
        }
      }
    }
    else {
      /* otherwise give the process a little bit time ... */
      DBG_DEBUG(0, "still waiting for driver start");
      return 1;
    }
  }

  if (LC_Driver_GetStatus(d)==LC_DriverStatusUp) {
    GWEN_PROCESS *p;
    GWEN_PROCESS_STATE pst;

    /* check whether the driver really is still up and running */
    p=LC_Driver_GetProcess(d);
    if (p) {
      pst=GWEN_Process_CheckState(p);
      if (pst!=GWEN_ProcessStateRunning) {
        DBG_ERROR(0, "Driver is not running anymore");
        LC_Driver_Dump(d, stderr, 2);
        GWEN_Process_Terminate(p);
        LC_Driver_SetProcess(d, 0);
        LC_CardServer_DriverDown(cs, d, LC_DriverStatusAborted,
                                 "Driver is not running anymore");
        return -1;
      }
    } /* if process */
    else {
      if (!(LC_Driver_GetDriverFlags(d) & LC_DRIVER_FLAGS_REMOTE)) {
        DBG_ERROR(0, "No process for local driver:");
        LC_Driver_Dump(d, stderr, 2);
        abort();
      }
    }
    DBG_DEBUG(0, "Driver still running");
    if (LC_Driver_GetActiveReadersCount(d)==0 &&
        cs->driverIdleTimeout) {
      time_t t;

      /* check for idle timeout */
      t=LC_Driver_GetIdleSince(d);
      assert(t);

      if (cs->driverIdleTimeout &&
          difftime(time(0), t)>cs->driverIdleTimeout) {
        DBG_NOTICE(0, "Driver \"%08x\" is too long idle, stopping it",
                   LC_Driver_GetDriverId(d));
        if (LC_CardServer_StopDriver(cs, d)) {
          DBG_INFO(0, "Could not stop driver \"%08x\"",
                   LC_Driver_GetDriverId(d));
          return -1;
        }
        return 0;
      } /* if timeout */
      /* otherwise reader is not idle */
    }
    return 1;
  }

  if (LC_Driver_GetStatus(d)==LC_DriverStatusDown) {

  }

  return 1;
}



int LC_CardServer_HandleDriverReady(LC_CARDSERVER *cs,
				    GWEN_TYPE_UINT32 rid,
				    GWEN_DB_NODE *dbReq){
  GWEN_DB_NODE *dbRsp;
  GWEN_TYPE_UINT32 driverId;
  GWEN_TYPE_UINT32 nodeId;
  LC_DRIVER *d;
  const char *code;
  const char *text;
  int i;
  GWEN_NETCONNECTION *conn;

  assert(dbReq);

  nodeId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeId", 0, 0);
  if (!nodeId) {
    DBG_ERROR(0, "Invalid node id");
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
    }
    return -1;
  }

  DBG_NOTICE(0, "Driver %08x: DriverReady", nodeId);

  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/driverId", 0, "0"),
                "%x", &i)) {
    DBG_ERROR(0, "Invalid driver id");
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
    }
    return -1;
  }
  driverId=i;
  if (driverId==0) {
    DBG_ERROR(0, "Invalid driver id");
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
    }
    return -1;
  }

  /* driver ready */
  /* find driver */
  d=LC_Driver_List_First(cs->drivers);
  while(d) {
    if (LC_Driver_GetDriverId(d)==driverId)
      break;
    d=LC_Driver_List_Next(d);
  } /* while */
  if (!d) {
    DBG_ERROR(0, "Driver \"%08x\" not found", driverId);
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Driver not found");
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
    }
    return -1;
  }

  /* store node id */
  LC_Driver_SetIpcId(d, nodeId);
  conn=GWEN_IPCManager_GetConnection(cs->ipcManager, nodeId);
  LC_ServerConn_TakeOver(conn);
  LC_ServerConn_SetCardServer(conn, cs);
  LC_ServerConn_SetDriver(conn, d);
  LC_ServerConn_SetType(conn, LC_ServerConn_TypeDriver);

  /* check code */
  code=GWEN_DB_GetCharValue(dbReq, "body/code", 0, "<none>");
  text=GWEN_DB_GetCharValue(dbReq, "body/text", 0, "<none>");
  if (strcasecmp(code, "OK")!=0) {
    GWEN_BUFFER *ebuf;

    DBG_ERROR(0, "Error in driver \"%08x\": %s",
              driverId, text);
    ebuf=GWEN_Buffer_new(0, 256, 0, 1);
    GWEN_Buffer_AppendString(ebuf, "Driver error (");
    GWEN_Buffer_AppendString(ebuf, text);
    GWEN_Buffer_AppendString(ebuf, ")");
    LC_CardServer_DriverDown(cs, d, LC_DriverStatusAborted,
                             GWEN_Buffer_GetStart(ebuf));
    GWEN_Buffer_free(ebuf);
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
    }
    return -1;
  }
  DBG_NOTICE(0, "Driver \"%08x\" is up (%s)", driverId, text);
  LC_Driver_SetStatus(d, LC_DriverStatusUp);
  LC_CardServer_SendDriverNotification(cs, 0,
				       LC_NOTIFY_CODE_DRIVER_UP,
                                       d, "Driver up");

  /* TODO: Parse list of readers if available */

  dbRsp=GWEN_DB_Group_new("DriverReadyResponse");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "code", "OK");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "text", "Driver registered");
  if (GWEN_IPCManager_SendResponse(cs->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response");
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
    }
    return -1;
  }
  if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
    DBG_WARN(0, "Could not remove request");
  }

  return 0;
}



int LC_CardServer_HandleGetDriverVar(LC_CARDSERVER *cs,
				     GWEN_TYPE_UINT32 rid,
				     GWEN_DB_NODE *dbReq){
  GWEN_DB_NODE *dbRsp;
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  const char *varName;
  const char *varValue;
  LC_CARD *card;
  GWEN_TYPE_UINT32 cardId;
  LC_READER *r;
  LC_DRIVER *d;
  GWEN_DB_NODE *dbVars;

  assert(dbReq);

  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeId", 0, 0);
  if (!clientId) {
    DBG_ERROR(0, "Invalid node id");
    return -1;
  }

  /* find client */
  cl=LC_Client_List_First(cs->clients);
  while(cl) {
    if (LC_Client_GetClientId(cl)==clientId)
      break;
    cl=LC_Client_List_Next(cl);
  } /* while */
  if (!cl) {
    DBG_ERROR(0, "Client \"%08x\" not found", clientId);
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
				    "Unknown client id");
    return -1;
  }

  DBG_NOTICE(0, "Client %08x: GetDriverVar", clientId);
  varName=GWEN_DB_GetCharValue(dbReq, "body/varName", 0, 0);
  if (varName==0) {
    DBG_ERROR(0, "Missing variable name");
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
				    "Missing variable name");
    return -1;
  }

  /* get card, get driver */
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/cardid", 0, "0"),
		"%x", &cardId)) {
    DBG_ERROR(0, "Bad server message");
    return -1;
  }

  /* search for card in active list */
  card=LC_Card_List_First(cs->activeCards);
  while(card) {
    if (LC_Card_GetCardId(card)==cardId) {
      /* card found */
      if (LC_Card_GetClient(card)==cl) {
        break;
      }
      else {
	DBG_ERROR(0, "Card \"%08x\" not owned by client \"%08x\"",
		  cardId, LC_Client_GetClientId(cl));
	LC_CardServer_SendErrorResponse(cs, rid,
                                        LC_ERROR_CARD_NOT_OWNED,
					"Card is not owned by you");
	return -1;
      }
    }
    card=LC_Card_List_Next(card);
  } /* while */

  if (!card) {
    DBG_ERROR(0, "No card with id \"%08x\" found", cardId);
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Card not found");
    return -1;
  }

  if (LC_Card_GetStatus(card)==LC_CardStatusRemoved) {
    DBG_ERROR(0, "Card has been removed");
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_CARD_REMOVED,
                                    "Card has been removed");
    return -1;
  }

  /* get driver */
  r=LC_Card_GetReader(card);
  assert(r);
  d=LC_Reader_GetDriver(r);
  assert(d);

  /* get variable */
  dbVars=LC_Driver_GetDriverVars(d);
  assert(dbVars);
  varValue=GWEN_DB_GetCharValue(dbVars, varName, 0, "");

  DBG_ERROR(0, "Returning variable: %s=%s",
	    varName, varValue);

  /* send response */
  dbRsp=GWEN_DB_Group_new("GetDriverVarResponse");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "varName", varName);
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "varValue", varValue);
  if (GWEN_IPCManager_SendResponse(cs->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response");
    return -1;
  }

  /* remove request */
  if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
    DBG_WARN(0, "Could not remove request");
  }

  return 0;
}



int LC_CardServer_HandleReaderError(LC_CARDSERVER *cs,
				    GWEN_TYPE_UINT32 rid,
				    GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 readerId;
  GWEN_TYPE_UINT32 nodeId;
  LC_READER *r;
  const char *txt;
  GWEN_BUFFER *ebuf;

  nodeId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeId", 0, 0);
  if (!nodeId) {
    DBG_ERROR(0, "Invalid node id");
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
    }
    return -1;
  }

  DBG_NOTICE(0, "Driver %08x: Reader error", nodeId);

  /* reader error */
  if (sscanf(GWEN_DB_GetCharValue(dbReq, "body/readerId", 0, "0"),
	     "%x",
	     &readerId)!=1) {
    DBG_ERROR(0, "Bad reader id in command \"%s\"",
	      GWEN_DB_GroupName(dbReq));
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }

  /* find reader */
  r=LC_Reader_List_First(cs->readers);
  while(r) {
    if (LC_Reader_GetReaderId(r)==readerId)
      break;
    r=LC_Reader_List_Next(r);
  } /* while */
  if (!r) {
    DBG_ERROR(0, "Reader \"%08x\" not found", readerId);
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }

  txt=GWEN_DB_GetCharValue(dbReq, "body/text", 0, "- no text -");
  DBG_NOTICE(0, "Reader \"%s\" is down due to an error (%s)",
             LC_Reader_GetReaderName(r), txt);
  ebuf=GWEN_Buffer_new(0, 256, 0, 1);
  GWEN_Buffer_AppendString(ebuf, "Reader error (");
  GWEN_Buffer_AppendString(ebuf, txt);
  GWEN_Buffer_AppendString(ebuf, ")");
  LC_Driver_DecActiveReadersCount(LC_Reader_GetDriver(r));
  LC_CardServer_ReaderDown(cs, r, LC_ReaderStatusAborted,
                           GWEN_Buffer_GetStart(ebuf));
  GWEN_Buffer_free(ebuf);

  GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
  return 0;
}



GWEN_TYPE_UINT32 LC_CardServer_SendStartReader(LC_CARDSERVER *cs,
                                               const LC_READER *r) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  LC_DRIVER *d;
  int port;

  assert(cs);
  assert(r);
  d=LC_Reader_GetDriver(r);
  assert(d);
  dbReq=GWEN_DB_Group_new("StartReader");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LC_Reader_GetReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "name", LC_Reader_GetReaderName(r));
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "slots", LC_Reader_GetSlots(r));
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
		      "flags", LC_Reader_GetFlags(r));
  port=LC_Reader_GetPort(r);
  if (port==-1) {
    port=LC_Driver_GetFirstNewPort(d)+(++(cs->nextNewPort));
    DBG_INFO(0, "Assigning port %d to reader \"%s\"",
             port, LC_Reader_GetReaderName(r));
  }
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "port", port);

  return GWEN_IPCManager_SendRequest(cs->ipcManager,
                                     LC_Driver_GetIpcId(d),
                                     dbReq);
}



GWEN_TYPE_UINT32 LC_CardServer_SendStopReader(LC_CARDSERVER *cs,
                                              const LC_READER *r) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  const char *p;
  LC_DRIVER *d;

  d=LC_Reader_GetDriver(r);
  assert(d);
  dbReq=GWEN_DB_Group_new("StopReader");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LC_Reader_GetReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);

  p=LC_Reader_GetReaderName(r);
  if (p)
    GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "name", p);

  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "port",  LC_Reader_GetPort(r));
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "slots", LC_Reader_GetSlots(r));

  return GWEN_IPCManager_SendRequest(cs->ipcManager,
                                     LC_Driver_GetIpcId(d),
                                     dbReq);
}



GWEN_TYPE_UINT32 LC_CardServer_SendStopDriver(LC_CARDSERVER *cs,
                                              const LC_DRIVER *d) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;

  assert(d);
  dbReq=GWEN_DB_Group_new("StopDriver");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LC_Driver_GetDriverId(d));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driverId", numbuf);

  return GWEN_IPCManager_SendRequest(cs->ipcManager,
                                     LC_Driver_GetIpcId(d),
                                     dbReq);
}



GWEN_TYPE_UINT32 LC_CardServer_CheckConnForDriver(LC_CARDSERVER *cs,
                                                  LC_DRIVER *d){
  LC_READER *r;

  assert(cs);
  r=LC_Reader_List_First(cs->readers);
  while(r) {
    if (LC_Reader_GetDriver(r)==d) {
      if (LC_Reader_HasNextRequest(r)) {
        return GWEN_NETCONNECTION_CHECK_WANTWRITE;
      }
    }
    r=LC_Reader_List_Next(r);
  } /* while */

  return 0;
}



GWEN_DB_NODE *LC_CardServer_DriverDbFromXml(GWEN_XMLNODE *node) {
  GWEN_DB_NODE *db;
  GWEN_XMLNODE *n;
  GWEN_XMLNODE *nLib;
  const char *p;
  const char *dname;
  GWEN_STRINGLIST *slDirs;
  GWEN_STRINGLIST *slNames;
  GWEN_BUFFER *nbuf;

  db=GWEN_DB_Group_new("driver");
  dname=GWEN_XMLNode_GetProperty(node, "name", 0);
  if (!dname) {
    DBG_ERROR(0, "Driver in XML file has no name");
    GWEN_DB_Group_free(db);
    return 0;
  }
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
		       "driverName", dname);

  n=GWEN_XMLNode_FindFirstTag(node, "manufacturer", 0, 0);
  if (n) {
    p=GWEN_XMLNode_GetCharValue(n, "name", 0);
    if (p)
      GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                           "manufacturer", p);
    p=GWEN_XMLNode_GetCharValue(n, "url", 0);
    if (p)
      GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                           "url", p);
  }

  /* read variables */
  n=GWEN_XMLNode_FindFirstTag(node, "vars", 0, 0);
  if (n) {
    GWEN_DB_NODE *dbVars;
    GWEN_XMLNODE *nn;

    dbVars=GWEN_DB_GetGroup(db, GWEN_DB_FLAGS_OVERWRITE_GROUPS, "vars");
    assert(dbVars);
    nn=GWEN_XMLNode_FindFirstTag(n, "var", 0, 0);
    while(nn) {
      const char *name;
      const char *value;
      GWEN_XMLNODE *nd;

      name=GWEN_XMLNode_GetProperty(nn, "name", 0);
      assert(name);

      nd=GWEN_XMLNode_GetFirstData(nn);
      if (nd)
	value=GWEN_XMLNode_GetData(nd);
      else
	value="";
      GWEN_DB_SetCharValue(dbVars, GWEN_DB_FLAGS_DEFAULT,
			   name, value);

      nn=GWEN_XMLNode_FindNextTag(nn, "var", 0, 0);
    } /* while */
  }


  p=GWEN_XMLNode_GetProperty(node, "type", 0);
  if (!p) {
    DBG_ERROR(0, "Driver \"%s\" in XML file has no type",
	      dname);
    GWEN_DB_Group_free(db);
    return 0;
  }
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
		       "driverType", p);

  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_DEFAULT,
		      "maxReaders",
		      atoi(GWEN_XMLNode_GetProperty(node,
						    "maxReaders",
						    "1")));

  p=GWEN_XMLNode_GetCharValue(node, "short", 0);
  if (p)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "short", p);

  nLib=GWEN_XMLNode_FindNode(node, GWEN_XMLNodeTypeTag, "lib");
  if (!nLib) {
    DBG_ERROR(0, "No <lib> tag for driver \"%s\"", dname);
    GWEN_DB_Group_free(db);
    return 0;
  }

  /* fetch dirs */
  n=GWEN_XMLNode_FindNode(nLib, GWEN_XMLNodeTypeTag, "locations");
  if (!n) {
    DBG_ERROR(0, "No locations given for driver \"%s\"", dname);
    GWEN_DB_Group_free(db);
    return 0;
  }

  slDirs=GWEN_StringList_new();
  /* always add common lowlevel driver path to search list */
  GWEN_StringList_AppendString(slDirs, LC_LOWLEVELDRIVER_PATH, 0, 1);

  n=GWEN_XMLNode_FindFirstTag(n, "loc", 0, 0);
  while(n) {
    GWEN_XMLNODE *nData;

    nData=GWEN_XMLNode_GetFirstData(n);
    if (n) {
      p=GWEN_XMLNode_GetData(nData);
      if (p)
	GWEN_StringList_AppendString(slDirs,
				     p, 0, 1);
    }
    n=GWEN_XMLNode_FindNextTag(n, "loc", 0, 0);
  } /* while */

  /* fetch names */
  n=GWEN_XMLNode_FindNode(nLib, GWEN_XMLNodeTypeTag, "names");
  if (!n) {
    DBG_ERROR(0, "No names given for driver \"%s\"", dname);
    GWEN_StringList_free(slDirs);
    GWEN_DB_Group_free(db);
    return 0;
  }

  slNames=GWEN_StringList_new();
  n=GWEN_XMLNode_FindFirstTag(n, "name", 0, 0);
  while(n) {
    GWEN_XMLNODE *nData;

    nData=GWEN_XMLNode_GetFirstData(n);
    if (n) {
      p=GWEN_XMLNode_GetData(nData);
      if (p)
	GWEN_StringList_AppendString(slNames,
				     p, 0, 1);
    }
    n=GWEN_XMLNode_FindNextTag(n, "loc", 0, 0);
  } /* while */

  nbuf=GWEN_Buffer_new(0, 256, 0, 1);
  if (!LC_CardServer_FindFile(slDirs, slNames, nbuf)) {
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "libraryFile", GWEN_Buffer_GetStart(nbuf));
  }
  GWEN_Buffer_free(nbuf);
  GWEN_StringList_free(slNames);
  GWEN_StringList_free(slDirs);

  return db;
}



GWEN_DB_NODE *LC_CardServer_ReaderDbFromXml(GWEN_XMLNODE *node) {
  GWEN_DB_NODE *db;
  GWEN_XMLNODE *n;
  const char *rtype;
  const char *p;
  int i;

  db=GWEN_DB_Group_new("reader");
  rtype=GWEN_XMLNode_GetProperty(node, "name", 0);
  if (!rtype) {
    DBG_ERROR(0, "Reader in XML file has no name");
    GWEN_DB_Group_free(db);
    return 0;
  }
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
		       "readerType", rtype);

  p=GWEN_XMLNode_GetCharValue(node, "short", rtype);
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                       "shortName", p);

  p=GWEN_XMLNode_GetProperty(node, "com", "serial");
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
		       "comType", p);

  GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_DEFAULT,
		      "slots",
		      atoi(GWEN_XMLNode_GetProperty(node, "slots", "1")));

  if (1==sscanf(GWEN_XMLNode_GetProperty(node, "vendor", "-1"),
                "%i", &i))
    if (i!=-1)
      GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_DEFAULT,
                          "vendorId", i);
  if (1==sscanf(GWEN_XMLNode_GetProperty(node, "product", "-1"),
                "%i", &i))
    if (i!=-1)
      GWEN_DB_SetIntValue(db, GWEN_DB_FLAGS_DEFAULT,
                          "productId", i);

  /* read flags */
  n=GWEN_XMLNode_FindNode(node, GWEN_XMLNodeTypeTag, "flags");
  if (n) {
    n=GWEN_XMLNode_FindFirstTag(n, "flag", 0, 0);
    while(n) {
      GWEN_XMLNODE *nData;

      nData=GWEN_XMLNode_GetFirstData(n);
      if (n) {
	p=GWEN_XMLNode_GetData(nData);
	if (p)
	  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
			       "flags", p);
      }
      n=GWEN_XMLNode_FindNextTag(n, "flag", 0, 0);
    } /* while */
  }

  /* read ports */
  n=GWEN_XMLNode_FindNode(node, GWEN_XMLNodeTypeTag, "ports");
  if (n) {
    GWEN_DB_NODE *dbPorts;

    dbPorts=GWEN_DB_GetGroup(db, GWEN_DB_FLAGS_OVERWRITE_GROUPS, "ports");
    n=GWEN_XMLNode_FindFirstTag(n, "port", 0, 0);
    while(n) {
      const char *vp;
      int i;

      vp=GWEN_XMLNode_GetProperty(n, "value", "0");
      if (1!=sscanf(vp, "%i", &i)) {
	DBG_ERROR(0, "Bad port value (%s), ignoring", vp);
      }
      else {
	GWEN_XMLNODE *nData;

	nData=GWEN_XMLNode_GetFirstData(n);
	if (nData) {
	  p=GWEN_XMLNode_GetData(nData);
	  if (p)
	    GWEN_DB_SetIntValue(dbPorts,
				GWEN_DB_FLAGS_DEFAULT,
				p, i);
	}
	else {
	  DBG_WARN(0, "No port name for value %d, ignoring", i);
	}
      }
      n=GWEN_XMLNode_FindNextTag(n, "port", 0, 0);
    } /* while */
  }

  return db;
}



int LC_CardServer__SampleDrivers(GWEN_STRINGLIST *sl,
                                 GWEN_DB_NODE *dbDrivers,
                                 int availOnly) {
  GWEN_STRINGLISTENTRY *e;
  GWEN_BUFFER *nbuf;

  e=GWEN_StringList_FirstEntry(sl);
  nbuf=GWEN_Buffer_new(0, 256, 0, 1);
  while(e) {
    GWEN_XMLNODE *nFile;

    GWEN_Buffer_Reset(nbuf);
    GWEN_Buffer_AppendString(nbuf, GWEN_StringListEntry_Data(e));
    GWEN_Buffer_AppendString(nbuf, "/driver.xml");
    nFile=GWEN_XMLNode_new(GWEN_XMLNodeTypeTag, "drivers");
    if (GWEN_XML_ReadFile(nFile, GWEN_Buffer_GetStart(nbuf),
			  GWEN_XML_FLAGS_DEFAULT)) {
      DBG_INFO(0, "Could not read file \"%s\"", GWEN_Buffer_GetStart(nbuf));
    }
    else {
      GWEN_XMLNODE *nDriver;

      nDriver=GWEN_XMLNode_FindNode(nFile, GWEN_XMLNodeTypeTag, "driver");
      if (!nDriver) {
	DBG_WARN(0, "XML file \"%s\" does not contain a driver",
		 GWEN_Buffer_GetStart(nbuf));
      }
      else {
	GWEN_DB_NODE *dbDriver;

	dbDriver=LC_CardServer_DriverDbFromXml(nDriver);
	if (!dbDriver) {
	  DBG_INFO(0, "Could not create driver from file \"%s\"",
		   GWEN_Buffer_GetStart(nbuf));
	}
	else {
          if (GWEN_DB_GetCharValue(dbDriver, "libraryFile", 0, 0) ||
              !availOnly) {
            GWEN_XMLNODE *nReader;
  
            nReader=GWEN_XMLNode_FindNode(nDriver, GWEN_XMLNodeTypeTag,
                                          "readers");
            if (!nReader) {
              DBG_WARN(0, "XML file \"%s\" contains no <readers> tag",
                       GWEN_Buffer_GetStart(nbuf));
            }
            else {
              int readers;
  
              readers=0;
              nReader=GWEN_XMLNode_FindFirstTag(nReader, "reader", 0, 0);
              while(nReader) {
                GWEN_DB_NODE *dbReader;
  
                dbReader=LC_CardServer_ReaderDbFromXml(nReader);
                if (dbReader) {
                  GWEN_DB_AddGroup(dbDriver, dbReader);
                  readers++;
                }
                nReader=GWEN_XMLNode_FindNextTag(nReader, "reader", 0, 0);
              } /* while */
              if (!readers) {
                DBG_WARN(0, "XML file \"%s\" contains no readers",
                         GWEN_Buffer_GetStart(nbuf));
              }
            }
            GWEN_DB_AddGroup(dbDrivers, dbDriver);
          }
          else {
            GWEN_DB_Group_free(dbDriver);
          }
	}
      }
    }
    GWEN_XMLNode_free(nFile);
    e=GWEN_StringListEntry_Next(e);
  } /* while eDirs */
  GWEN_Buffer_free(nbuf);

  return 0;
}



int LC_CardServer_SampleDrivers(LC_CARDSERVER *cs,
                                GWEN_STRINGLIST *sl,
                                GWEN_DB_NODE *dbDrivers,
                                int availOnly) {
  return LC_CardServer__SampleDrivers(sl, dbDrivers, availOnly);
}



int LC_CardServer_ReadDrivers(const char *dataDir,
                              GWEN_DB_NODE *dbDrivers,
                              int availOnly) {
  GWEN_STRINGLIST *sl;
  int rv;

  sl=GWEN_StringList_new();
  LC_CardServer__SampleDirs(dataDir, sl);
  rv=LC_CardServer__SampleDrivers(sl, dbDrivers, availOnly);
  GWEN_StringList_free(sl);
  return rv;
}








int LC_CardServer_Reader_Up(LC_CARDSERVER *cs, LC_READER *r) {
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 readerFlags;

  assert(cs);
  assert(r);

  readerFlags=LC_Reader_GetFlags(r);
  cl=LC_Client_List_First(cs->clients);
  while(cl) {
    GWEN_TYPE_UINT32 clientId;

    clientId=LC_Client_GetClientId(cl);
    if (!((readerFlags ^ LC_Client_GetWaitReaderFlags(cl)) &
	  LC_Client_GetWaitReaderMask(cl))) {

      /* add the reader */
      DBG_INFO(0, "Adding reader \"%s\" for client %08x",
	       LC_Reader_GetReaderName(r), clientId);
      DBG_DEBUG(0, "Calling LC_Reader_IncUsageCount");
      LC_Reader_IncUsageCount(r);
      LC_Client_AddReader(cl, LC_Reader_GetReaderId(r));

      /* is reader down ? */
      if (LC_Reader_GetStatus(r)==LC_ReaderStatusDown) {
	DBG_NOTICE(0, "Starting reader \"%s\" on account of client %08x",
		   LC_Reader_GetReaderName(r), clientId);
	if (LC_CardServer_StartReader(cs, r)) {
	  DBG_ERROR(0, "Could not start reader \"%s\"",
		    LC_Reader_GetReaderName(r));
	  LC_Client_DelReader(cl, LC_Reader_GetReaderId(r));
	}
      } /* if reader down */
      break;
    }
    cl=LC_Client_List_Next(cl);
  } /* while */

  return 0;
}



int LC_CardServer_SendDriverNotification(LC_CARDSERVER *cs,
					 LC_CLIENT *cl,
                                         const char *ncode,
                                         const LC_DRIVER *d,
                                         const char *info){
  GWEN_DB_NODE *dbData;
  const char *s;
  char numbuf[16];
  int rv;

  assert(cs);
  assert(ncode);
  assert(d);
  dbData=GWEN_DB_Group_new("driverData");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LC_Driver_GetDriverId(d));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driverId", numbuf);

  s=LC_Driver_GetDriverType(d);
  if (s)
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "driverType", s);
  s=LC_Driver_GetDriverName(d);
  if (s)
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "driverName", s);
  s=LC_Driver_GetLibraryFile(d);
  if (s)
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "libraryFile", s);
  if (info)
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "info", info);
  rv=LC_CardServer_SendNotification(cs, cl,
                                    LC_NOTIFY_TYPE_DRIVER,
                                    ncode, dbData);
  GWEN_DB_Group_free(dbData);
  if (rv) {
    DBG_INFO(0, "here");
    return rv;
  }
  return 0;
}



int LC_CardServer_SendReaderNotification(LC_CARDSERVER *cs,
					 LC_CLIENT *cl,
                                         const char *ncode,
                                         const LC_READER *r,
                                         const char *info){
  GWEN_DB_NODE *dbData;
  const char *s;
  char numbuf[16];
  int rv;
  const LC_DRIVER *d;
  GWEN_TYPE_UINT32 flags;

  assert(cs);
  assert(ncode);
  assert(r);
  dbData=GWEN_DB_Group_new("readerData");

  d=LC_Reader_GetDriver(r);
  assert(d);
  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LC_Reader_GetReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);
  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LC_Driver_GetDriverId(d));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driverId", numbuf);
  s=LC_Reader_GetReaderType(r);
  if (s)
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "readerType", s);
  s=LC_Reader_GetReaderName(r);
  if (s)
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "readerName", s);

  s=LC_Reader_GetShortDescr(r);
  if (s)
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "shortDescr", s);

  GWEN_DB_SetIntValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "readerPort", LC_Reader_GetPort(r));

  flags=LC_Reader_GetFlags(r);
  if (flags & LC_READER_FLAGS_KEYPAD)
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_DEFAULT,
                         "readerflags", "KEYPAD");
  if (flags & LC_READER_FLAGS_DISPLAY)
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_DEFAULT,
                         "readerflags", "DISPLAY");
  if (flags & LC_READER_FLAGS_NOINFO)
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_DEFAULT,
			 "readerflags", "NOINFO");
  if (flags & LC_READER_FLAGS_REMOTE)
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_DEFAULT,
                         "readerflags", "REMOTE");
  if (flags & LC_READER_FLAGS_AUTO)
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_DEFAULT,
                         "readerflags", "AUTO");

  s=LC_Reader_GetReaderInfo(r);
  if (s)
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "readerInfo", s);

  if (info)
    GWEN_DB_SetCharValue(dbData, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "info", info);
  rv=LC_CardServer_SendNotification(cs, cl,
                                    LC_NOTIFY_TYPE_READER,
                                    ncode, dbData);
  GWEN_DB_Group_free(dbData);
  if (rv) {
    DBG_INFO(0, "here");
    return rv;
  }
  return 0;
}



