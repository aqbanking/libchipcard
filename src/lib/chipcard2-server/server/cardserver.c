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

#include <chipcard2-server/chipcard2.h>
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



LC_CARDSERVER *LC_CardServer_new(const char *dataDir){
  LC_CARDSERVER *cs;
  GWEN_STRINGLIST *sl;

  if (!dataDir)
    dataDir=LC_DEFAULT_DATADIR;

  GWEN_NEW_OBJECT(LC_CARDSERVER, cs);
  cs->ipcManager=GWEN_IPCManager_new();
  cs->drivers=LC_Driver_List_new();
  cs->readers=LC_Reader_List_new();
  cs->clients=LC_Client_List_new();
  cs->activeCards=LC_Card_List_new();
  cs->freeCards=LC_Card_List_new();
  cs->requests=LC_Request_List_new();
  cs->services=LC_Service_List_new();

  cs->readerStartTimeout=LC_CARDSERVER_DEFAULT_READERSTARTTIMEOUT;
  cs->driverStartTimeout=LC_CARDSERVER_DEFAULT_DRIVERSTARTTIMEOUT;
  cs->readerIdleTimeout=LC_CARDSERVER_DEFAULT_READERIDLETIMEOUT;
  cs->driverIdleTimeout=LC_CARDSERVER_DEFAULT_DRIVERIDLETIMEOUT;

  cs->dataDir=strdup(dataDir);

  cs->dbDrivers=GWEN_DB_Group_new("drivers");
  cs->dbConfigDrivers=GWEN_DB_Group_new("configDrivers");

  cs->usbTtyMonitor=LC_USBTTYMonitor_new();
  cs->usbMonitor=LC_USBMonitor_new();

  /* create card manager */
  sl=GWEN_StringList_new();
  LC_CardServer_SampleDirs(cs, sl);
  cs->cardManager=LC_CardMgr_new(sl);

  /* sample drivers and readers from XML files */
  LC_CardServer_SampleDrivers(cs, sl, cs->dbDrivers, 1);
  GWEN_StringList_free(sl);

  return cs;
}



void LC_CardServer_free(LC_CARDSERVER *cs){
  if (cs) {
    LC_USBTTYMonitor_free(cs->usbTtyMonitor);
    LC_USBMonitor_free(cs->usbMonitor);
    LC_CardMgr_free(cs->cardManager);
    free(cs->dataDir);
    free(cs->typeForDrivers);
    free(cs->addrForDrivers);
    GWEN_DB_Group_free(cs->dbConfigDrivers);
    GWEN_DB_Group_free(cs->dbDrivers);
    LC_Request_List_free(cs->requests);
    LC_Reader_List_free(cs->readers);
    LC_Driver_List_free(cs->drivers);
    LC_Client_List_free(cs->clients);
    LC_Card_List_free(cs->activeCards);
    LC_Card_List_free(cs->freeCards);
    LC_Service_List_free(cs->services);
    GWEN_IPCManager_free(cs->ipcManager);
    GWEN_FREE_OBJECT(cs);
  }
}



LC_REQUEST *LC_CardServer_GetRequest(const LC_CARDSERVER *cs,
                                     GWEN_TYPE_UINT32 rid) {
  LC_REQUEST *rq;

  rq=LC_Request_List_First(cs->requests);
  while(rq) {
    if (LC_Request_GetRequestId(rq)==rid)
      break;
    rq=LC_Request_List_Next(rq);
  } /* while */

  return rq;
}



int LC_CardServer_CheckClient(LC_CARDSERVER *cs, LC_CLIENT *cl) {
  GWEN_NETCONNECTION *conn;
  int isDown;

  conn=GWEN_IPCManager_GetConnection(cs->ipcManager,
                                     LC_Client_GetClientId(cl));
  if (conn==0) isDown=1;
  else
    isDown=(GWEN_NetConnection_GetStatus(conn)!=
            GWEN_NetTransportStatusLConnected);
  return isDown!=0;
}



int LC_CardServer_CheckClients(LC_CARDSERVER *cs) {
  LC_CLIENT *cl;
  int done;

  done=0;
  cl=LC_Client_List_First(cs->clients);
  while(cl) {
    LC_CLIENT *next;

    next=LC_Client_List_Next(cl);
    if (LC_CardServer_CheckClient(cs, cl)) {
      DBG_NOTICE(0, "Client \"%08x\" down",
                 LC_Client_GetClientId(cl));
      LC_CardServer_ClientDown(cs, cl);
      done++;
    }
    cl=next;
  } /* while */

  return done?0:1;
}



void LC_CardServer_ClientDown(LC_CARDSERVER *cs, LC_CLIENT *cl) {
  LC_CARD *card;
  LC_READER *r;
  LC_SERVICE *as;
  LC_REQUEST *rq;
  int rv;

  /* remove all requests of this client */
  rq=LC_Request_List_First(cs->requests);
  while(rq) {
    LC_REQUEST *next;

    next=LC_Request_List_Next(rq);
    if (LC_Request_GetClient(rq)==cl) {
      LC_Request_List_Del(rq);
      LC_Request_free(rq);
    }
    rq=next;
  } /* while */

  /* release all cards used by this client */
  card=LC_Card_List_First(cs->activeCards);
  while(card) {
    if (LC_Card_GetClient(card)==cl) {
      LC_DRIVER *d;
      GWEN_TYPE_UINT32 rid;
      GWEN_DB_NODE *dbReq;

      /* card in use by client, reset it */
      DBG_NOTICE(0,
                 "Card \"%08x\" still used by client \"%08x\", "
                 "releasing it",
                 LC_Card_GetCardId(card), LC_Client_GetClientId(cl));
      LC_Card_SetContext(card, 0);
      r=LC_Card_GetReader(card);
      assert(r);
      d=LC_Reader_GetDriver(r);
      assert(d);

      /* Reset card */
      dbReq=GWEN_DB_Group_new("ResetCard");

      rid=LC_CardServer_SendResetCard(cs, card);
      if (rid==0) {
        DBG_ERROR(0, "Could not send card reset request");
        LC_Reader_SetStatus(r, LC_ReaderStatusAborted);
	LC_CardServer_SendReaderNotification(cs, 0,
					     LC_NOTIFY_CODE_READER_ERROR,
                                             r,
					     "Could not send card reset "
					     "request");
      }

      /* release card from clients handle */
      LC_Card_SetClient(card, 0);

      /* move card from activeCards to freeCards */
      LC_Card_List_Del(card);
      LC_Card_List_Add(card, cs->freeCards);

      /* detach card from reader */
      if (LC_Card_GetStatus(card)!=LC_CardStatusRemoved) {
        DBG_DEBUG(0, "Calling LC_Reader_DecUsageCount");
        LC_Reader_DecUsageCount(r);
      }
    }
    card=LC_Card_List_Next(card);
  } /* while card */

  /* release all readers currently in use by the client */
  r=LC_Reader_List_First(cs->readers);
  while(r) {
    /* remove all requests of the given client */
    LC_Reader_DelClientRequests(r, cl);
    if (LC_Client_HasReader(cl, LC_Reader_GetReaderId(r))) {
      DBG_NOTICE(0, "Reader \"%s\" in use by client \"%08x\"",
                 LC_Reader_GetReaderName(r), LC_Client_GetClientId(cl));
      DBG_DEBUG(0, "Calling LC_Reader_DecUsageCount");
      LC_Reader_DecUsageCount(r);
    }
    r=LC_Reader_List_Next(r);
  } /* while */

  /* release all services currently in use by the client */
  as=LC_Service_List_First(cs->services);
  while(as) {
    if (LC_Client_HasService(cl, LC_Service_GetServiceId(as))) {
      DBG_NOTICE(0, "Service \"%s\" in use by client \"%08x\"",
                 LC_Service_GetServiceName(as), LC_Client_GetClientId(cl));
      DBG_DEBUG(0, "Calling LC_Reader_DecUsageCount");
      LC_Service_DecActiveClientsCount(as);
    }
    as=LC_Service_List_Next(as);
  } /* while */

  /* remove the IPC node of this client */
  rv=GWEN_IPCManager_RemoveClient(cs->ipcManager,
                                  LC_Client_GetClientId(cl));
  if (rv) {
    DBG_WARN(0, "Error removing IPC node of client \"%08x\"",
	     LC_Client_GetClientId(cl));
  }

  LC_Client_List_Del(cl);
  LC_Client_free(cl);
}



int LC_CardServer_ReplaceVar(const char *path,
                             const char *var,
                             const char *value,
                             GWEN_BUFFER *nbuf) {
  unsigned int vlen;

  vlen=strlen(var);

  while(*path) {
    int handled;

    handled=0;
    if (*path=='@') {
      if (strncmp(path+1, var, vlen)==0) {
        if (path[vlen+1]=='@') {
          /* found variable, replace it */
          GWEN_Buffer_AppendString(nbuf, value);
          path+=vlen+2;
          handled=1;
        }
      }
    }
    if (!handled) {
      GWEN_Buffer_AppendByte(nbuf, *path);
      path++;
    }
  } /* while */

  return 0;
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
    LC_Driver_SetStatus(d, LC_DriverStatusDisabled);
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
    LC_Driver_SetStatus(d, LC_DriverStatusAborted);
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
  int rv;

  assert(cs);
  assert(d);

  done=0;


  nid=LC_Driver_GetIpcId(d);

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
    GWEN_PROCESS_STATE pst;

    p=LC_Driver_GetProcess(d);
    if (!p) {
      // DEBUG
      DBG_ERROR(0, "No process for this driver:");
      LC_Driver_Dump(d, stderr, 2);
      abort();
    }
    assert(p);

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
        LC_Driver_SetStatus(d, LC_DriverStatusAborted);
        if (nid) {
          rv=GWEN_IPCManager_RemoveClient(cs->ipcManager, nid);
          if (rv) {
            DBG_WARN(0, "Error removing IPC node of driver \"%08x\"",
                     LC_Driver_GetDriverId(d));
          }
          LC_Driver_SetIpcId(d, nid);
        }
	LC_CardServer_SendDriverNotification(cs, 0,
					     LC_NOTIFY_CODE_DRIVER_ERROR,
					     d,
					     "Driver still running, "
					     "killing it");
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
      LC_Driver_SetStatus(d, LC_DriverStatusDown);
      if (nid) {
        rv=GWEN_IPCManager_RemoveClient(cs->ipcManager, nid);
        if (rv) {
          DBG_WARN(0, "Error removing IPC node of driver \"%08x\"",
                   LC_Driver_GetDriverId(d));
        }
        LC_Driver_SetIpcId(d, nid);
      }
      LC_CardServer_SendDriverNotification(cs, 0,
					   LC_NOTIFY_CODE_DRIVER_DOWN,
                                           d, "Driver terminated normally");
      return 0;
    }
    else if (pst==GWEN_ProcessStateAborted) {
      DBG_WARN(0, "Driver terminated abnormally");
      LC_Driver_SetProcess(d, 0);
      LC_Driver_SetStatus(d, LC_DriverStatusAborted);
      if (nid) {
        rv=GWEN_IPCManager_RemoveClient(cs->ipcManager, nid);
        if (rv) {
          DBG_WARN(0, "Error removing IPC node of driver \"%08x\"",
                   LC_Driver_GetDriverId(d));
        }
        LC_Driver_SetIpcId(d, nid);
      }
      LC_CardServer_SendDriverNotification(cs, 0,
					   LC_NOTIFY_CODE_DRIVER_ERROR,
                                           d, "Driver terminated abnormally");
      return 0;
    }
    else if (pst==GWEN_ProcessStateStopped) {
      DBG_WARN(0, "Driver has been stopped, killing it");
      if (GWEN_Process_Terminate(p)) {
        DBG_ERROR(0, "Could not kill process");
      }
      LC_Driver_SetProcess(d, 0);
      LC_Driver_SetStatus(d, LC_DriverStatusAborted);
      if (nid) {
        rv=GWEN_IPCManager_RemoveClient(cs->ipcManager, nid);
        if (rv) {
          DBG_WARN(0, "Error removing IPC node of driver \"%08x\"",
                   LC_Driver_GetDriverId(d));
        }
        LC_Driver_SetIpcId(d, nid);
      }
      LC_CardServer_SendDriverNotification(cs, 0,
					   LC_NOTIFY_CODE_DRIVER_ERROR,
					   d,
					   "Driver has been stopped, "
					   "killing it");
      return 0;
    }
    else {
      DBG_ERROR(0, "Unknown process status %d, killing", pst);
      if (GWEN_Process_Terminate(p)) {
        DBG_ERROR(0, "Could not kill process");
      }
      LC_Driver_SetProcess(d, 0);
      LC_Driver_SetStatus(d, LC_DriverStatusAborted);
      if (nid) {
        rv=GWEN_IPCManager_RemoveClient(cs->ipcManager, nid);
        if (rv) {
          DBG_WARN(0, "Error removing IPC node of driver \"%08x\"",
                   LC_Driver_GetDriverId(d));
        }
        LC_Driver_SetIpcId(d, nid);
      }
      LC_CardServer_SendDriverNotification(cs, 0,
					   LC_NOTIFY_CODE_DRIVER_ERROR,
					   d,
					   "Unknown process status, "
					   "killing");
      return 0;
    }
  } /* if stopping */


  if (LC_Driver_GetStatus(d)==LC_DriverStatusStarted) {
    /* driver started, check timeout */
    if (cs->driverStartTimeout &&
        difftime(time(0), LC_Driver_GetLastStatusChangeTime(d))>=
        cs->driverStartTimeout) {
      GWEN_PROCESS *p;
      GWEN_PROCESS_STATE pst;

      DBG_WARN(0, "Driver \"%s\" timed out", LC_Driver_GetDriverName(d));
      p=LC_Driver_GetProcess(d);
      assert(p);

      pst=GWEN_Process_CheckState(p);
      if (pst==GWEN_ProcessStateRunning) {
        DBG_WARN(0,
                 "Driver is running but did not signal readyness, "
                 "killing it");
        if (GWEN_Process_Terminate(p)) {
          DBG_ERROR(0, "Could not kill process");
        }
        LC_Driver_SetProcess(d, 0);
        LC_Driver_SetStatus(d, LC_DriverStatusAborted);
        if (nid) {
          rv=GWEN_IPCManager_RemoveClient(cs->ipcManager, nid);
          if (rv) {
            DBG_WARN(0, "Error removing IPC node of driver \"%08x\"",
                     LC_Driver_GetDriverId(d));
          }
          LC_Driver_SetIpcId(d, nid);
        }
	LC_CardServer_SendDriverNotification(cs, 0,
					     LC_NOTIFY_CODE_DRIVER_ERROR,
					     d,
					     "Driver is running but did not "
					     "signal readyness, "
					     "killing it");
        return -1;
      }
      else if (pst==GWEN_ProcessStateExited) {
        DBG_WARN(0, "Driver terminated normally");
        LC_Driver_SetProcess(d, 0);
        LC_Driver_SetStatus(d, LC_DriverStatusDown);
        if (nid) {
          rv=GWEN_IPCManager_RemoveClient(cs->ipcManager, nid);
          if (rv) {
            DBG_WARN(0, "Error removing IPC node of driver \"%08x\"",
                     LC_Driver_GetDriverId(d));
          }
          LC_Driver_SetIpcId(d, nid);
        }
	LC_CardServer_SendDriverNotification(cs, 0,
					     LC_NOTIFY_CODE_DRIVER_DOWN,
                                             d, "Driver terminated normally");
        return 0;
      }
      else if (pst==GWEN_ProcessStateAborted) {
        DBG_WARN(0, "Driver terminated abnormally");
        LC_Driver_SetProcess(d, 0);
        LC_Driver_SetStatus(d, LC_DriverStatusAborted);
        if (nid) {
          rv=GWEN_IPCManager_RemoveClient(cs->ipcManager, nid);
          if (rv) {
            DBG_WARN(0, "Error removing IPC node of driver \"%08x\"",
                     LC_Driver_GetDriverId(d));
          }
          LC_Driver_SetIpcId(d, nid);
        }
	LC_CardServer_SendDriverNotification(cs, 0,
					     LC_NOTIFY_CODE_DRIVER_ERROR,
					     d,
					     "Driver terminated abnormally");
        return -1;
      }
      else if (pst==GWEN_ProcessStateStopped) {
        DBG_WARN(0, "Driver has been stopped, killing it");
        if (GWEN_Process_Terminate(p)) {
          DBG_ERROR(0, "Could not kill process");
        }
        LC_Driver_SetProcess(d, 0);
        LC_Driver_SetStatus(d, LC_DriverStatusAborted);
        if (nid) {
          rv=GWEN_IPCManager_RemoveClient(cs->ipcManager, nid);
          if (rv) {
            DBG_WARN(0, "Error removing IPC node of driver \"%08x\"",
                     LC_Driver_GetDriverId(d));
          }
          LC_Driver_SetIpcId(d, nid);
        }
	LC_CardServer_SendDriverNotification(cs, 0,
					     LC_NOTIFY_CODE_DRIVER_ERROR,
					     d,
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
        LC_Driver_SetStatus(d, LC_DriverStatusAborted);
        if (nid) {
          rv=GWEN_IPCManager_RemoveClient(cs->ipcManager, nid);
          if (rv) {
            DBG_WARN(0, "Error removing IPC node of driver \"%08x\"",
                     LC_Driver_GetDriverId(d));
          }
          LC_Driver_SetIpcId(d, nid);
        }
	LC_CardServer_SendDriverNotification(cs, 0,
					     LC_NOTIFY_CODE_DRIVER_ERROR,
					     d,
					     "Unknown process status, "
					     "killing");
	return -1;
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
    assert(p);
    pst=GWEN_Process_CheckState(p);
    if (pst!=GWEN_ProcessStateRunning) {
      DBG_ERROR(0, "Driver is not running anymore");
      GWEN_Process_Terminate(p);
      LC_Driver_SetProcess(d, 0);
      LC_Driver_SetStatus(d, LC_DriverStatusAborted);
      if (nid) {
        rv=GWEN_IPCManager_RemoveClient(cs->ipcManager, nid);
        if (rv) {
          DBG_WARN(0, "Error removing IPC node of driver \"%08x\"",
                   LC_Driver_GetDriverId(d));
        }
        LC_Driver_SetIpcId(d, nid);
      }
      LC_CardServer_SendDriverNotification(cs, 0,
					   LC_NOTIFY_CODE_DRIVER_ERROR,
					   d,
					   "Driver is not running anymore");
      return -1;
    }
    else {
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
  }

  return 1;
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
    LC_Reader_SetStatus(r, LC_ReaderStatusAborted);
    LC_CardServer_SendReaderNotification(cs, 0,
					 LC_NOTIFY_CODE_READER_ERROR,
                                         r,
                                         "Could not send StopReader command");
    LC_Driver_DecActiveReadersCount(d);
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

  if (st==LC_ReaderStatusAborted ||
      st==LC_ReaderStatusDisabled) {
    GWEN_TYPE_UINT32 rid;

    handled=1;
    rid=LC_Reader_GetCurrentRequestId(r);
    if (rid) {
      GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
      LC_Reader_SetCurrentRequestId(r, 0);
    }

    /* remove requests, remove cards for reader */
    LC_Reader_ClearRequests(r); /* TODO: Send messages for every request */
    LC_CardServer_RemoveCardsForReader(cs, r);

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
	LC_Reader_free(r);
        return 0;
      }
    }
  }

  if (LC_Reader_GetStatus(r)==LC_ReaderStatusDown) {
    handled=1;
    if (LC_Reader_IsAvailable(r)) {
      LC_CardServer_RemoveCardsForReader(cs, r);
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
        LC_Reader_SetStatus(r, LC_ReaderStatusAborted);
	LC_CardServer_SendReaderNotification(cs, 0,
					     LC_NOTIFY_CODE_READER_ERROR,
                                             r,
                                             "Could not send StartReader "
                                             "command");
        LC_Driver_DecActiveReadersCount(d);
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
        LC_Reader_SetStatus(r, LC_ReaderStatusAborted);
	LC_CardServer_SendReaderNotification(cs, 0,
					     LC_NOTIFY_CODE_READER_ERROR,
                                             r, "Reader timed out");
        LC_Driver_DecActiveReadersCount(d);
        return -1;
      } /* if timeout */

      /* otherwise the reader has some time left */
      return 1;
    }
    else {
      /* bad status, abort reader */
      DBG_WARN(0, "Reader \"%s\" aborted due to driver status (%d)",
               LC_Reader_GetReaderName(r), dst);
      LC_Reader_SetStatus(r, LC_ReaderStatusAborted);
      LC_CardServer_SendReaderNotification(cs, 0,
					   LC_NOTIFY_CODE_READER_ERROR,
                                           r,
                                           "Reader aborted (driver status)");
      LC_Driver_DecActiveReadersCount(d);
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
      LC_Reader_SetStatus(r, LC_ReaderStatusAborted);
      LC_CardServer_SendReaderNotification(cs, 0,
					   LC_NOTIFY_CODE_READER_ERROR,
                                           r,
                                           "Reader aborted (driver status)");
      LC_Driver_DecActiveReadersCount(d);
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
        LC_Reader_SetStatus(r, LC_ReaderStatusAborted);
	ebuf=GWEN_Buffer_new(0, 256, 0, 1);
	if (e) {
	  GWEN_Buffer_AppendString(ebuf, "Reader error (");
	  GWEN_Buffer_AppendString(ebuf, e);
	  GWEN_Buffer_AppendString(ebuf, ")");
	}
	else {
	  GWEN_Buffer_AppendString(ebuf, "Reader error (startup)");
	}
	LC_CardServer_SendReaderNotification(cs, 0,
					     LC_NOTIFY_CODE_READER_ERROR,
					     r,
					     GWEN_Buffer_GetStart(ebuf));
	GWEN_Buffer_free(ebuf);
	LC_Driver_DecActiveReadersCount(d);
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
          LC_Reader_SetStatus(r, LC_ReaderStatusAborted);
          ebuf=GWEN_Buffer_new(0, 256, 0, 1);
          if (e) {
            GWEN_Buffer_AppendString(ebuf, "Reader error (");
            GWEN_Buffer_AppendString(ebuf, e);
            GWEN_Buffer_AppendString(ebuf, ")");
          }
          else {
            GWEN_Buffer_AppendString(ebuf, "Reader error (startup)");
          }
          LC_CardServer_SendReaderNotification(cs, 0,
                                               LC_NOTIFY_CODE_READER_ERROR,
                                               r,
                                               GWEN_Buffer_GetStart(ebuf));
          GWEN_Buffer_free(ebuf);
          LC_Driver_DecActiveReadersCount(d);
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
        LC_Reader_SetStatus(r, LC_ReaderStatusAborted);
	LC_CardServer_SendReaderNotification(cs, 0,
					     LC_NOTIFY_CODE_READER_ERROR,
                                             r, "Reader error (timeout)");
        LC_Driver_DecActiveReadersCount(d);
        GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 1);
        LC_Reader_SetCurrentRequestId(r, 0);
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
      LC_Reader_SetStatus(r, LC_ReaderStatusAborted);
      LC_CardServer_StopReader(cs, r);
      return -1;
    }

    /* check for driver */
    dst=LC_Driver_GetStatus(d);
    if (dst!=LC_DriverStatusUp) {
      DBG_ERROR(0, "Bad driver status, aborting reader command");
      LC_Reader_SetStatus(r, LC_ReaderStatusAborted);
      LC_CardServer_SendReaderNotification(cs, 0,
					   LC_NOTIFY_CODE_READER_ERROR,
                                           r, "Reader error driver status)");
      LC_Driver_DecActiveReadersCount(d);
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
	  LC_Reader_SetStatus(r, LC_ReaderStatusAborted);
	  ebuf=GWEN_Buffer_new(0, 256, 0, 1);
	  GWEN_Buffer_AppendString(ebuf, "Could not send command \"");
	  GWEN_Buffer_AppendString(ebuf, cmdName);
	  GWEN_Buffer_AppendString(ebuf, "\"");
	  LC_CardServer_SendReaderNotification(cs, 0,
					       LC_NOTIFY_CODE_READER_ERROR,
					       r,
					       GWEN_Buffer_GetStart(ebuf));
	  GWEN_Buffer_free(ebuf);
	  LC_Driver_DecActiveReadersCount(d);
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
      LC_Reader_SetStatus(r, LC_ReaderStatusDown);
      LC_CardServer_SendReaderNotification(cs, 0,
					   LC_NOTIFY_CODE_READER_DOWN,
                                           r, "Reader is down (as expected)");
      LC_Driver_DecActiveReadersCount(d);
      couldDoSomething++;
    }
    else {
      if (cs->readerStartTimeout &&
          difftime(time(0), LC_Reader_GetLastStatusChangeTime(r))>=
          cs->readerStartTimeout) {
        /* reader timed out */
        DBG_WARN(0, "Reader \"%s\" timed out", LC_Reader_GetReaderName(r));
        LC_Reader_SetStatus(r, LC_ReaderStatusAborted);
        LC_CardServer_SendReaderNotification(cs, 0,
                                             LC_NOTIFY_CODE_READER_ERROR,
                                             r, "Reader error (timeout)");
        LC_Driver_DecActiveReadersCount(d);
        GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 1);
        LC_Reader_SetCurrentRequestId(r, 0);
        if (LC_CardServer_StopDriver(cs, d)) {
          DBG_WARN(0, "Could not stop driver for reader \"%s\"",
                   LC_Reader_GetReaderName(r));
        }
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
    LC_Reader_SetStatus(r, LC_ReaderStatusAborted);
    LC_CardServer_SendReaderNotification(cs, 0,
					 LC_NOTIFY_CODE_READER_ERROR,
					 r,
					 "Reader error "
					 "(unknown reader status)");
    LC_Driver_DecActiveReadersCount(d);
    return -1;
  }

  return couldDoSomething?0:1;
}



LC_CLIENT *LC_CardServer_FindClient(const LC_CARDSERVER *cs,
                                    GWEN_TYPE_UINT32 id){
  LC_CLIENT *cl;

  assert(cs);
  cl=LC_Client_List_First(cs->clients);
  while(cl) {
    if (LC_Client_GetClientId(cl)==id)
      return cl;
    cl=LC_Client_List_Next(cl);
  } /* while */
  return 0;
}



LC_REQUEST *LC_CardServer_FindClientCardRequest(const LC_CARDSERVER *cs,
                                                const LC_CLIENT *cl,
                                                const LC_CARD *cd,
                                                const char *name) {
  LC_REQUEST *rq;
  GWEN_TYPE_UINT32 cid;

  assert(cs);
  assert(cd);
  cid=LC_Card_GetCardId(cd);
  rq=LC_Request_List_First(cs->requests);
  while(rq) {
    if (LC_Request_GetClient(rq)==cl) {
      GWEN_DB_NODE *gr;
      const char *cmdName;

      DBG_DEBUG(0, "Found a request for this client");
      gr=LC_Request_GetInRequestData(rq);
      assert(gr);
      cmdName=GWEN_DB_GetCharValue(gr, "command/vars/cmd", 0, 0);
      assert(cmdName);
      if (strcasecmp(cmdName, name)==0) {
        GWEN_TYPE_UINT32 ccid;

        if (1==sscanf(GWEN_DB_GetCharValue(gr, "body/cardid", 0, "0"),
		      "%x", &ccid)) {
	  DBG_VERBOUS(0, "Comparing card id \"%08x\" against \"%08x\"",
                      ccid, cid);
	  if (ccid==cid)
	    return rq;
	}
      }
    } /* if client matches */
    rq=LC_Request_List_Next(rq);
  } /* while */

  return 0;
}


int LC_CardServer_CheckCards(LC_CARDSERVER *cs) {
  LC_CARD *cd;
  LC_READER *r;
  LC_READER_STATUS rst;
  int needIO;

  assert(cs);
  needIO=0;

  DBG_VERBOUS(0, "Checking active cards");
  cd=LC_Card_List_First(cs->activeCards);
  while(cd) {
    LC_CARD *next;

    next=LC_Card_List_Next(cd);

    r=LC_Card_GetReader(cd);
    assert(r);
    rst=LC_Reader_GetStatus(r);
    if (rst==LC_ReaderStatusUp || rst==LC_ReaderStatusWaitForReaderUp) {
      /* TODO: Check for busy timeout to prevent one user blocking a card */
    }
    else {
      /* reader is not up */
      if (LC_Card_GetStatus(cd)==LC_CardStatusInserted) {
        /* card becomes invalid */
        DBG_NOTICE(0, "Active card \"%08x\" became invalid",
                   LC_Card_GetCardId(cd));
        LC_Reader_DecUsageCount(r);
        LC_Card_SetStatus(cd, LC_CardStatusRemoved);
        needIO++;
      } /* if card is inserted */
    }
    cd=next;
  } /* while */

  DBG_VERBOUS(0, "Checking free cards");
  cd=LC_Card_List_First(cs->freeCards);
  while(cd) {
    LC_CARD *next;

    next=LC_Card_List_Next(cd);

    r=LC_Card_GetReader(cd);
    assert(r);
    rst=LC_Reader_GetStatus(r);
    if (rst!=LC_ReaderStatusUp &&
        rst!=LC_ReaderStatusWaitForReaderUp) {
      /* reader is not up, card is invalid */
      DBG_NOTICE(0, "Unused card \"%08x\" in reader \"%s\" became invalid, "
                 "removing it",
                 LC_Card_GetCardId(cd),
                 LC_Reader_GetReaderName(r));
      LC_Card_SetStatus(cd, LC_CardStatusRemoved);
      /* remove card from list of free cards, free it */
      LC_Card_List_Del(cd);
      LC_Card_free(cd);
      needIO++;
    }
    else {
      /* check whether this unused card has been removed */
      if (LC_Card_GetStatus(cd)==LC_CardStatusRemoved) {
        DBG_NOTICE(0,
                   "Unused card \"%08x\" has been removed, "
                   "deleting it",
                   LC_Card_GetCardId(cd));
        /* remove card from list of free cards, free it */
        LC_Card_List_Del(cd);
        LC_Card_free(cd);
        needIO++;
      }
      else {
        LC_CLIENT *cl;

        for (;;) {
          GWEN_TYPE_UINT32 cid;

          /* check whether someone is already waiting for this card */
          cid=LC_Card_GetFirstWaitingClient(cd);
          if (cid) {
            /* yes, in fact there is */
            cl=LC_CardServer_FindClient(cs, cid);
            if (cl) {
              LC_REQUEST *rq;

              rq=LC_CardServer_FindClientCardRequest(cs, cl, cd,
                                                     "TakeCard");
              if (rq) {
                GWEN_DB_NODE *dbRsp;

                DBG_DEBUG(0, "Found a TakeCard request (id=%d)",
                          LC_Request_GetInRequestId(rq));
                needIO++;
                dbRsp=GWEN_DB_Group_new("TakeCardResponse");
                GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_DEFAULT,
                                     "code", "OK");
                GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_DEFAULT,
                                     "text", "the card is yours");
                if (GWEN_IPCManager_SendResponse(cs->ipcManager,
                                                 LC_Request_GetInRequestId(rq),
                                                 dbRsp)) {
                  DBG_ERROR(0, "Could not send TakeCard response");
                  LC_Reader_SetStatus(r, LC_ReaderStatusAborted);
                  LC_CardServer_SendReaderNotification(cs, 0,
						       LC_NOTIFY_CODE_READER_ERROR,
						       r,
						       "Could not send "
						       "TakeCard response");
		  /* FIXME: Test: 2004/06/11 */
		  LC_Driver_DecActiveReadersCount(LC_Reader_GetDriver(r));
                  GWEN_IPCManager_RemoveRequest(cs->ipcManager,
                                                LC_Request_GetInRequestId(rq),
                                                0);
                }
                else {
                  /* card is now in clients hands */
                  DBG_NOTICE(0,
                             "Card \"%08x\" taken by client \"%08x\"",
                             LC_Card_GetCardId(cd),
                             LC_Client_GetClientId(cl));
                  LC_Card_SetClient(cd, cl);

                  /* move card from freeCards to activeCards */
                  LC_Card_List_Del(cd);
                  LC_Card_List_Add(cd, cs->activeCards);
                  LC_Reader_IncUsageCount(LC_Card_GetReader(cd));

                  /* remove client from waiting list */
                  LC_Card_DelWaitingClient(cd, cid);
                  /* card is sold, break the loop */
                  DBG_INFO(0, "Card is sold to client \"%08x\"", cid);
                  /* remove request from list and free it */
                  GWEN_IPCManager_RemoveRequest(cs->ipcManager,
                                                LC_Request_GetInRequestId(rq),
                                                0);
                  LC_Request_List_Del(rq);
                  LC_Request_free(rq);
                  break;
                }
              }
              else {
                /* remove client from waiting list */
                DBG_WARN(0,
                         "Did not find a TakeCard request for client \"%08x\"",
                         LC_Client_GetClientId(cl));
                LC_Card_DelWaitingClient(cd, LC_Client_GetClientId(cl));
                needIO++;
              }
            } /* if client found */
            else {
              /* client not found, remove this id from waiting list */
              DBG_WARN(0, "Did not find client \"%08x\"", cid);
              LC_Card_DelWaitingClient(cd, cid);
              needIO++;
            }
          } /* if there was a waiting client */
          else
            break;
        } /* for */

        /* reader is up, check whether this card is wanted by anyone */
        cl=LC_Client_List_First(cs->clients);
        while(cl) {
          if (LC_Client_HasReader(cl, LC_Reader_GetReaderId(r)) &&
              !LC_Client_HasCard(cl, LC_Card_GetCardId(cd))) {
            LC_DRIVER *d;
            GWEN_DB_NODE *gr;
            const char *readerType;
            const char *driverName;
            GWEN_BUFFER *atr;
            char numbuf[16];
            GWEN_TYPE_UINT32 flags;
            LC_CARD_TYPE ct;
            GWEN_TYPE_UINT32 rid;

            /* card is newly available to this client */
            needIO++;
            DBG_NOTICE(0, "Advertising card %08x to client %08x",
                       LC_Card_GetCardId(cd),
                       LC_Client_GetClientId(cl));
            d=LC_Reader_GetDriver(r);
            assert(d);
            gr=GWEN_DB_Group_new("CardAvailable");

            readerType=LC_Reader_GetReaderType(r);
            assert(readerType);
            driverName=LC_Driver_GetDriverName(d);
            atr=LC_Card_GetAtr(cd);
            snprintf(numbuf, sizeof(numbuf)-1, "%08x",
                     LC_Card_GetCardId(cd));
            numbuf[sizeof(numbuf)-1]=0;

            GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_OVERWRITE_VARS,
                                 "cardid", numbuf);
            GWEN_DB_DeleteVar(gr, "readerflags");
            flags=LC_Reader_GetFlags(r);
            if (flags & LC_READER_FLAGS_KEYPAD)
              GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_DEFAULT,
                                   "readerflags", "KEYPAD");
            if (flags & LC_READER_FLAGS_DISPLAY)
              GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_DEFAULT,
                                   "readerflags", "DISPLAY");

            ct=LC_Card_GetType(cd);
            if (ct==LC_CardTypeProcessor)
              GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_DEFAULT,
                                   "cardtype", "PROCESSOR");
            else if (ct==LC_CardTypeMemory)
              GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_DEFAULT,
                                   "cardtype", "MEMORY");
            else
              GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_DEFAULT,
                                   "cardtype", "UNKNOWN");

            if (atr) {
              if (GWEN_Buffer_GetUsedBytes(atr)) {
                GWEN_DB_SetBinValue(gr, GWEN_DB_FLAGS_OVERWRITE_VARS,
                                    "atr",
                                    GWEN_Buffer_GetStart(atr),
                                    GWEN_Buffer_GetUsedBytes(atr));
              } /* if ATR not empty */
            } /* if ATR */

            rid=GWEN_IPCManager_SendRequest(cs->ipcManager,
                                            LC_Client_GetClientId(cl),
                                            gr);
            if (rid==0) {
              DBG_ERROR(0, "Could not send \"CardAvailable\" to client");
            }
            else {
              /* client soon wil know about the card, add it to his list
               * to avoid sending a notification again */
              LC_Client_AddCard(cl, LC_Card_GetCardId(cd));
              /* remove request, we don't expect an answer */
              GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 1);
            }
          } /* if card is new to the client */
          cl=LC_Client_List_Next(cl);
        } /* while */
      } /* if card is unused but inserted */
    } /* if reader is up */
    cd=next;
  } /* while */
  DBG_VERBOUS(0, "Checking free cards done (%d)", needIO);

  return needIO?0:1;
}



int LC_CardServer_CheckServiceRsp(LC_CARDSERVER *cs,
                                  LC_REQUEST *rq,
                                  GWEN_DB_NODE *dbServiceRsp,
                                  GWEN_DB_NODE *dbRsp){
  const char *cmdName;
  const char *rspName;
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbServiceRspBody;

  assert(rq);
  dbReq=LC_Request_GetInRequestData(rq);
  assert(dbReq);
  cmdName=GWEN_DB_GetCharValue(dbReq, "body/command/name", 0, 0);
  assert(cmdName);

  DBG_VERBOUS(0, "Service response was this:");
  if (GWEN_Logger_GetLevel(0)>=GWEN_LoggerLevelVerbous)
    GWEN_DB_Dump(dbServiceRsp, stderr, 2);

  dbServiceRspBody=GWEN_DB_GetGroup(dbServiceRsp,
                                    GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                                    "body");
  assert(dbServiceRspBody);
  rspName=GWEN_DB_GetCharValue(dbServiceRsp, "command/vars/cmd",0, 0);
  assert(rspName);

  /* check for error */
  if (strcasecmp(rspName, "error")==0) {
    DBG_ERROR(0, "Service responded with error msg");
    GWEN_DB_GroupRename(dbRsp, "error");
    GWEN_DB_AddGroupChildren(dbRsp, dbServiceRspBody);
    return 0;
  }
  else if (strcasecmp(rspName, "ServiceCommandResponse")==0) {
    GWEN_DB_NODE *dbCommandRsp;
    GWEN_BUFFER *buf;

    buf=GWEN_Buffer_new(0, 64, 0, 1);
    GWEN_Buffer_AppendString(buf, cmdName);
    GWEN_Buffer_AppendString(buf, "Response");
    GWEN_DB_GroupRename(dbRsp, GWEN_Buffer_GetStart(buf));
    GWEN_Buffer_free(buf);

    dbCommandRsp=GWEN_DB_GetGroup(dbServiceRspBody,
                                  GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                                  "command");
    if (dbCommandRsp)
      GWEN_DB_AddGroupChildren(dbRsp, dbCommandRsp);
  }
  else {
    DBG_ERROR(0, "Unknown service response \"%s\"", rspName);
    GWEN_DB_GroupRename(dbRsp, "error");
    GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "code", LC_ERROR_INVALID);
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text",
                         "internal error "
                         "(unknown service response)");
    return 0;
  }

  return 0;
}




int LC_CardServer_CheckRequest(LC_CARDSERVER *cs, LC_REQUEST *rq) {
  const char *name;
  LC_CARD *card;
  LC_READER *r;
  LC_DRIVER *d;

  card=LC_Request_GetCard(rq);
  assert(card);
  r=LC_Card_GetReader(card);
  assert(r);
  d=LC_Reader_GetDriver(r);
  assert(d);

  name=GWEN_DB_GetCharValue(LC_Request_GetInRequestData(rq),
                            "command/vars/cmd", 0, 0);
  assert(name);
  DBG_DEBUG(0, "Request: %s", name);

  if (strcasecmp(name, "CommandCard")==0) {
    GWEN_DB_NODE *dbDriverResponse;
    GWEN_TYPE_UINT32 ridOut;

    DBG_NOTICE(0, "Handling CommandCard request");
    ridOut=LC_Request_GetOutRequestId(rq);
    if (!ridOut) {
      DBG_ERROR(0, "INTERNAL: No out request ??");
      return -1;
    }
    DBG_DEBUG(0, "Checking for driver response");
    dbDriverResponse=GWEN_IPCManager_GetResponseData(cs->ipcManager,
                                                     ridOut);
    if (dbDriverResponse) {
      GWEN_DB_NODE *dbClientResponse;
      const char *p;
      const void *bp;
      unsigned int bs;
      const char *code;

      LC_Driver_DecPendingCommandCount(d);
      DBG_DEBUG(0, "Sending response to CommandCard");
      dbClientResponse=GWEN_DB_Group_new("CommandCardResponse");
      code=GWEN_DB_GetCharValue(dbDriverResponse,
				"body/code", 0, "ERROR");
      assert(code);
      GWEN_DB_SetCharValue(dbClientResponse,
			   GWEN_DB_FLAGS_OVERWRITE_VARS,
			   "code", code);
      p=GWEN_DB_GetCharValue(dbDriverResponse,
			     "body/text", 0, 0);
      if (p)
        GWEN_DB_SetCharValue(dbClientResponse,
                             GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text", p);

      bp=GWEN_DB_GetBinValue(dbDriverResponse,
                             "body/data", 0, 0, 0, &bs);
      if (bp && bs)
        GWEN_DB_SetBinValue(dbClientResponse,
                            GWEN_DB_FLAGS_OVERWRITE_VARS,
                            "data", bp, bs);

      if (GWEN_IPCManager_SendResponse(cs->ipcManager,
                                       LC_Request_GetInRequestId(rq),
                                       dbClientResponse)) {
        DBG_ERROR(0, "Could not send CommandCard response");
        return -1;
      }
      return 0;
    }
    else {
      DBG_DEBUG(0, "No response yet");
    }
  } /* CommandCard */
  else if (strcasecmp(name, "TakeCard")==0) {
    DBG_DEBUG(0, "TakeCard");
  }
  else if (strcasecmp(name, "ExecCommand")==0) {
    GWEN_DB_NODE *dbDriverResponse;
    GWEN_TYPE_UINT32 ridOut;

    DBG_DEBUG(0, "Handling ExecCommand request");
    ridOut=LC_Request_GetOutRequestId(rq);
    if (!ridOut) {
      DBG_ERROR(0, "INTERNAL: No out request ??");
      return -1;
    }
    DBG_DEBUG(0, "Checking for driver response (%08x)", ridOut);
    dbDriverResponse=GWEN_IPCManager_GetResponseData(cs->ipcManager,
                                                     ridOut);
    if (dbDriverResponse) {
      GWEN_DB_NODE *dbClientResponse;
      GWEN_DB_NODE *dbRsp;
      LC_CARDMGR_RESULT res;

      LC_Driver_DecPendingCommandCount(d);
      DBG_DEBUG(0, "Sending response to ExecCommand");
      dbClientResponse=GWEN_DB_Group_new("ExecCommandResponse");
      dbRsp=GWEN_DB_Group_new("command");
      res=LC_CardMgr_CheckResponse(cs->cardManager,
                                   rq,
                                   dbDriverResponse,
                                   dbRsp);
      if (res==LC_CardMgr_ResultOk) {
        GWEN_DB_SetIntValue(dbClientResponse,
                            GWEN_DB_FLAGS_OVERWRITE_VARS,
                            "code", LC_ERROR_NONE);
        GWEN_DB_SetCharValue(dbClientResponse,
                             GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text", "Command executed");
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "name",
                             GWEN_DB_GroupName(dbRsp));
        GWEN_DB_GroupRename(dbRsp, "command");
        GWEN_DB_AddGroup(dbClientResponse, dbRsp);
        if (GWEN_IPCManager_SendResponse(cs->ipcManager,
                                         LC_Request_GetInRequestId(rq),
                                         dbClientResponse)) {
          DBG_ERROR(0, "Could not send ExecCommand response");
          return -1;
        }
        return 0;
      } /* if ok */
      else if (res==LC_CardMgr_ResultNeedMore) {
        GWEN_TYPE_UINT32 rqid;

        /* TODO */
        GWEN_DB_Group_free(dbRsp);
        rqid=LC_Request_GetInRequestId(rq);
        DBG_ERROR(0, "NeedMore not yet supported");
        if (LC_CardServer_SendErrorResponse
            (cs, rqid,
             LC_ERROR_NOT_SUPPORTED,
             "NeedMore not yet supported")
           ) {
          DBG_ERROR(0, "Could not send ExecCommand response");
          return -1;
        }
        return 0;
      } /* if need more */
      else if (res==LC_CardMgr_ResultError ||
               res==LC_CardMgr_ResultCmdError) {
        int code;
        const char *text;
        GWEN_TYPE_UINT32 rqid;

        rqid=LC_Request_GetInRequestId(rq);
        code=GWEN_DB_GetIntValue(dbRsp, "code", 0, 0);
        text=GWEN_DB_GetCharValue(dbRsp, "text", 0, 0);
        DBG_NOTICE(0, "Error in command (%d: %s)", code, text);
        if (LC_CardServer_SendErrorResponse(cs, rqid, code, text)) {
          DBG_ERROR(0, "Could not send ExecCommand response");
          GWEN_DB_Group_free(dbRsp);
          return -1;
        }
        GWEN_DB_Group_free(dbRsp);
        return 0;
      } /* if error */
    } /* if driverResponse */
  } /* ExecCommand */
  else if (strcasecmp(name, "ServiceOpen")==0) {
    GWEN_DB_NODE *dbServiceResponse;
    GWEN_TYPE_UINT32 ridOut;

    DBG_DEBUG(0, "Handling ServiceOpen request");
    ridOut=LC_Request_GetOutRequestId(rq);
    if (!ridOut) {
      DBG_ERROR(0, "INTERNAL: No out request ??");
      abort();
    }
    DBG_DEBUG(0, "Checking for service response (%08x)", ridOut);
    dbServiceResponse=GWEN_IPCManager_GetResponseData(cs->ipcManager,
                                                      ridOut);
    if (dbServiceResponse) {
      GWEN_DB_NODE *dbClientResponse;
      GWEN_DB_NODE *dbInReq;
      LC_SERVICE *sv;
      GWEN_TYPE_UINT32 serviceId;
      char numbuf[16];
      const char *rspName;
      GWEN_DB_NODE *dbServiceRspBody;
      GWEN_DB_NODE *dbRsp;

      dbInReq=LC_Request_GetInRequestData(rq);
      if (1!=sscanf(GWEN_DB_GetCharValue(dbInReq, "body/serviceId", 0, "0"),
                    "%x", &serviceId)) {
        DBG_ERROR(0, "Uups, no/bad service id, should not happen here...");
        abort();
      }

      /* search for service */
      sv=LC_Service_List_First(cs->services);
      while(sv) {
        if (LC_Service_GetServiceId(sv)==serviceId)
          break;
        sv=LC_Service_List_Next(sv);
      }
      if (!sv) {
        DBG_ERROR(0, "Service \"%08x\" not found", serviceId);
        abort();
      }

      DBG_DEBUG(0, "Sending response to ServiceOpen");
      dbClientResponse=GWEN_DB_Group_new("ServiceOpenResponse");

      dbServiceRspBody=GWEN_DB_GetGroup(dbServiceResponse,
                                        GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                                        "body");
      assert(dbServiceRspBody);
      rspName=GWEN_DB_GetCharValue(dbServiceResponse,
                                   "command/vars/cmd",0, 0);
      assert(rspName);
    
      /* check for error */
      if (strcasecmp(rspName, "error")==0) {
        GWEN_DB_SetIntValue(dbClientResponse,
                            GWEN_DB_FLAGS_OVERWRITE_VARS,
                            "code", LC_ERROR_INVALID);
        GWEN_DB_SetCharValue(dbClientResponse,
                             GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text", "Error flagged by service.");

        /* remove service from client */
        LC_Client_DelService(LC_Request_GetClient(rq),
                             LC_Service_GetServiceId(sv));
        LC_Service_DecActiveClientsCount(sv);
      }
      else if (strcasecmp(rspName, "ServiceOpenResponse")==0) {
        GWEN_TYPE_UINT32 contextId;

        /* get context id */
        if (1!=sscanf(GWEN_DB_GetCharValue(dbServiceRspBody,
                                           "contextId", 0, "0"),
                      "%x", &contextId)) {
          DBG_ERROR(0, "No/bad context id from service");
          GWEN_DB_SetIntValue(dbClientResponse,
                              GWEN_DB_FLAGS_OVERWRITE_VARS,
                              "code", LC_ERROR_INVALID);
          GWEN_DB_SetCharValue(dbClientResponse,
                               GWEN_DB_FLAGS_OVERWRITE_VARS,
                               "text", "No context id received.");
          /* remove service from client */
          LC_Client_DelService(LC_Request_GetClient(rq),
                               LC_Service_GetServiceId(sv));
          LC_Service_DecActiveClientsCount(sv);
        }
        else {
          GWEN_DB_SetIntValue(dbClientResponse,
                              GWEN_DB_FLAGS_OVERWRITE_VARS,
                              "code", LC_ERROR_NONE);
          GWEN_DB_SetCharValue(dbClientResponse,
                               GWEN_DB_FLAGS_OVERWRITE_VARS,
                               "text", "Service open.");
          /* send context id */
          snprintf(numbuf, sizeof(numbuf)-1, "%08x", contextId);
          numbuf[sizeof(numbuf)-1]=0;
          GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                               "contextId", numbuf);
        }
      }
      else {
        DBG_ERROR(0, "Unknown service response to ServiceOpen: %s",
                  rspName);
        GWEN_DB_SetIntValue(dbClientResponse,
                            GWEN_DB_FLAGS_OVERWRITE_VARS,
                            "code", LC_ERROR_INVALID);
        GWEN_DB_SetCharValue(dbClientResponse,
                             GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text", "Invalid service response.");
        /* remove service from client */
        LC_Client_DelService(LC_Request_GetClient(rq),
                             LC_Service_GetServiceId(sv));
        LC_Service_DecActiveClientsCount(sv);
      }

      /* send service id */
      snprintf(numbuf, sizeof(numbuf)-1, "%08x",
               LC_Service_GetServiceId(sv));
      numbuf[sizeof(numbuf)-1]=0;
      GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "serviceId", numbuf);
      /* send service name */
      GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "serviceName", LC_Service_GetServiceName(sv));
      /* send service flags */
      if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_AUTOLOAD)
        GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_DEFAULT,
                             "serviceFlags", "autoload");
      if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_SILENT)
        GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_DEFAULT,
                             "serviceFlags", "silent");
      if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_CLIENT)
        GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_DEFAULT,
                             "serviceFlags", "client");
      dbRsp=GWEN_DB_GetGroup(dbClientResponse,
                             GWEN_DB_FLAGS_DEFAULT,
                             "service");
      GWEN_DB_AddGroupChildren(dbRsp, dbServiceRspBody);

      /* send response */
      if (GWEN_IPCManager_SendResponse(cs->ipcManager,
                                       LC_Request_GetInRequestId(rq),
                                       dbClientResponse)) {
        DBG_ERROR(0, "Could not send ServiceCommand response");
        return -1;
      }
      return 0;
    } /* if serviceResponse */
  } /* ServiceOpen */
  else if (strcasecmp(name, "ServiceClose")==0) {
    GWEN_DB_NODE *dbServiceResponse;
    GWEN_TYPE_UINT32 ridOut;

    DBG_DEBUG(0, "Handling ServiceClose request");
    ridOut=LC_Request_GetOutRequestId(rq);
    if (!ridOut) {
      DBG_ERROR(0, "INTERNAL: No out request ??");
      abort();
    }
    DBG_DEBUG(0, "Checking for service response (%08x)", ridOut);
    dbServiceResponse=GWEN_IPCManager_GetResponseData(cs->ipcManager,
                                                      ridOut);
    if (dbServiceResponse) {
      GWEN_DB_NODE *dbClientResponse;
      GWEN_DB_NODE *dbInReq;
      LC_SERVICE *sv;
      GWEN_TYPE_UINT32 serviceId;
      char numbuf[16];
      const char *rspName;
      GWEN_DB_NODE *dbServiceRspBody;
      GWEN_DB_NODE *dbRsp;

      dbInReq=LC_Request_GetInRequestData(rq);
      if (1!=sscanf(GWEN_DB_GetCharValue(dbInReq, "body/serviceId", 0, "0"),
                    "%x", &serviceId)) {
        DBG_ERROR(0, "Uups, no/bad service id, should not happen here...");
        abort();
      }

      /* search for service */
      sv=LC_Service_List_First(cs->services);
      while(sv) {
        if (LC_Service_GetServiceId(sv)==serviceId)
          break;
        sv=LC_Service_List_Next(sv);
      }
      if (!sv) {
        DBG_ERROR(0, "Service \"%08x\" not found", serviceId);
        abort();
      }

      DBG_DEBUG(0, "Sending response to ServiceClose");
      dbClientResponse=GWEN_DB_Group_new("ServiceCloseResponse");

      dbServiceRspBody=GWEN_DB_GetGroup(dbServiceResponse,
                                        GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                                        "body");
      assert(dbServiceRspBody);
      rspName=GWEN_DB_GetCharValue(dbServiceResponse,
                                   "command/vars/cmd",0, 0);
      assert(rspName);
    
      /* check for error */
      if (strcasecmp(rspName, "error")==0) {
        GWEN_DB_SetIntValue(dbClientResponse,
                            GWEN_DB_FLAGS_OVERWRITE_VARS,
                            "code", LC_ERROR_INVALID);
        GWEN_DB_SetCharValue(dbClientResponse,
                             GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text", "Error flagged by service.");
      }
      else if (strcasecmp(rspName, "ServiceCloseResponse")==0) {
        GWEN_TYPE_UINT32 contextId;

        /* get context id */
        if (1!=sscanf(GWEN_DB_GetCharValue(dbServiceRspBody,
                                           "contextId", 0, "0"),
                      "%x", &contextId)) {
          DBG_ERROR(0, "No/bad context id from service");
          GWEN_DB_SetIntValue(dbClientResponse,
                              GWEN_DB_FLAGS_OVERWRITE_VARS,
                              "code", LC_ERROR_INVALID);
          GWEN_DB_SetCharValue(dbClientResponse,
                               GWEN_DB_FLAGS_OVERWRITE_VARS,
                               "text", "No context id received.");
        }
        else {
          GWEN_DB_SetIntValue(dbClientResponse,
                              GWEN_DB_FLAGS_OVERWRITE_VARS,
                              "code", LC_ERROR_NONE);
          GWEN_DB_SetCharValue(dbClientResponse,
                               GWEN_DB_FLAGS_OVERWRITE_VARS,
                               "text", "Service closed.");
          /* send context id */
          snprintf(numbuf, sizeof(numbuf)-1, "%08x", contextId);
          numbuf[sizeof(numbuf)-1]=0;
          GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                               "contextId", numbuf);
        }
      }
      else {
        DBG_ERROR(0, "Unknown service response to ServiceClose: %s",
                  rspName);
        GWEN_DB_SetIntValue(dbClientResponse,
                            GWEN_DB_FLAGS_OVERWRITE_VARS,
                            "code", LC_ERROR_INVALID);
        GWEN_DB_SetCharValue(dbClientResponse,
                             GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text", "Invalid service response.");
      }

      /* send service id */
      snprintf(numbuf, sizeof(numbuf)-1, "%08x",
               LC_Service_GetServiceId(sv));
      numbuf[sizeof(numbuf)-1]=0;
      GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "serviceId", numbuf);
      /* send service name */
      GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "serviceName", LC_Service_GetServiceName(sv));
      /* send service flags */
      if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_AUTOLOAD)
        GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_DEFAULT,
                             "serviceFlags", "autoload");
      if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_SILENT)
        GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_DEFAULT,
                             "serviceFlags", "silent");
      if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_CLIENT)
        GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_DEFAULT,
                             "serviceFlags", "client");
      dbRsp=GWEN_DB_GetGroup(dbClientResponse,
                             GWEN_DB_FLAGS_DEFAULT,
                             "service");
      GWEN_DB_AddGroupChildren(dbRsp, dbServiceRspBody);

      /* send response */
      if (GWEN_IPCManager_SendResponse(cs->ipcManager,
                                       LC_Request_GetInRequestId(rq),
                                       dbClientResponse)) {
        DBG_ERROR(0, "Could not send ServiceCommand response");
        /* remove service from client */
        LC_Client_DelService(LC_Request_GetClient(rq),
                             LC_Service_GetServiceId(sv));
        LC_Service_DecActiveClientsCount(sv);
        return -1;
      }

      /* remove service from client */
      LC_Client_DelService(LC_Request_GetClient(rq),
                           LC_Service_GetServiceId(sv));
      LC_Service_DecActiveClientsCount(sv);
      return 0;
    } /* if serviceResponse */
  } /* ServiceClose */
  else if (strcasecmp(name, "ServiceCommand")==0) {
    GWEN_DB_NODE *dbServiceResponse;
    GWEN_TYPE_UINT32 ridOut;

    DBG_DEBUG(0, "Handling ServiceCommand request");
    ridOut=LC_Request_GetOutRequestId(rq);
    if (!ridOut) {
      DBG_ERROR(0, "INTERNAL: No out request ??");
      abort();
    }
    DBG_DEBUG(0, "Checking for service response (%08x)", ridOut);
    dbServiceResponse=GWEN_IPCManager_GetResponseData(cs->ipcManager,
                                                      ridOut);
    if (dbServiceResponse) {
      GWEN_DB_NODE *dbClientResponse;
      GWEN_DB_NODE *dbRsp;
      int res;
      GWEN_DB_NODE *dbInReq;
      LC_SERVICE *sv;
      GWEN_TYPE_UINT32 serviceId;
      GWEN_TYPE_UINT32 contextId;
      char numbuf[16];
  
      DBG_DEBUG(0, "Handling ServiceCommand request");
      dbInReq=LC_Request_GetInRequestData(rq);
      if (1!=sscanf(GWEN_DB_GetCharValue(dbInReq, "body/serviceId", 0, "0"),
                    "%x", &serviceId)) {
        DBG_ERROR(0, "Uups, no/bad service id, should not happen here...");
        abort();
      }

      /* get context id */
      if (1!=sscanf(GWEN_DB_GetCharValue(dbInReq, "body/contextId", 0, "0"),
                    "%x", &contextId)) {
        DBG_ERROR(0, "Uups, no/bad context id, should not happen here...");
        abort();
      }

      /* search for service */
      sv=LC_Service_List_First(cs->services);
      while(sv) {
        if (LC_Service_GetServiceId(sv)==serviceId)
          break;
        sv=LC_Service_List_Next(sv);
      }
      if (!sv) {
        DBG_ERROR(0, "Service \"%08x\" not found", serviceId);
        abort();
      }

      DBG_DEBUG(0, "Sending response to ServiceCommand");
      dbClientResponse=GWEN_DB_Group_new("ServiceCommandResponse");

      dbRsp=GWEN_DB_Group_new("command");
      res=LC_CardServer_CheckServiceRsp(cs,
                                        rq,
                                        dbServiceResponse,
                                        dbRsp);
      if (res==0) {
        GWEN_DB_SetIntValue(dbClientResponse,
                            GWEN_DB_FLAGS_OVERWRITE_VARS,
                            "code", LC_ERROR_NONE);
        GWEN_DB_SetCharValue(dbClientResponse,
                             GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text", "Service command executed");
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "name",
                             GWEN_DB_GroupName(dbRsp));
        GWEN_DB_GroupRename(dbRsp, "command");
        GWEN_DB_AddGroup(dbClientResponse, dbRsp);
      }
      else {
        GWEN_DB_SetIntValue(dbClientResponse,
                            GWEN_DB_FLAGS_OVERWRITE_VARS,
                            "code", LC_ERROR_INVALID);
        GWEN_DB_SetCharValue(dbClientResponse,
                             GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "text", "bad service response");
        GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                             "name",
                             "error");
        GWEN_DB_Group_free(dbRsp);
      }

      /* send service id */
      snprintf(numbuf, sizeof(numbuf)-1, "%08x",
               LC_Service_GetServiceId(sv));
      numbuf[sizeof(numbuf)-1]=0;
      GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "serviceId", numbuf);
      /* send service name */
      GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "serviceName", LC_Service_GetServiceName(sv));
      /* send context id */
      snprintf(numbuf, sizeof(numbuf)-1, "%08x", contextId);
      numbuf[sizeof(numbuf)-1]=0;
      GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_OVERWRITE_VARS,
                           "contextId", numbuf);
      /* send service flags */
      if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_AUTOLOAD)
        GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_DEFAULT,
                             "serviceFlags", "autoload");
      if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_SILENT)
        GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_DEFAULT,
                             "serviceFlags", "silent");
      if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_CLIENT)
        GWEN_DB_SetCharValue(dbClientResponse, GWEN_DB_FLAGS_DEFAULT,
                             "serviceFlags", "client");

      /* send response */
      if (GWEN_IPCManager_SendResponse(cs->ipcManager,
                                       LC_Request_GetInRequestId(rq),
                                       dbClientResponse)) {
        DBG_ERROR(0, "Could not send ExecCommand response");
        return -1;
      }
      return 0;
    } /* if serviceResponse */
  } /* ServiceCommand */
  else {
    DBG_ERROR(0, "Unknown client request \"%s\"", name);
    return -1;
  }

  return 1;
}


void LC_CardServer_RemoveRequest(LC_CARDSERVER *cs, LC_REQUEST *rq){
  GWEN_TYPE_UINT32 rid;

  DBG_DEBUG(0, "Removing request");
  LC_Request_List_Del(rq);
  rid=LC_Request_GetOutRequestId(rq);
  if (rid) {
    DBG_DEBUG(0, "- outRequest is %08x", rid);
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 1);
  }
  rid=LC_Request_GetInRequestId(rq);
  if (rid) {
    DBG_DEBUG(0, "- inRequest is %08x", rid);
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
  }
  LC_Request_free(rq);
}



int LC_CardServer_CheckRequests(LC_CARDSERVER *cs){
  LC_REQUEST *rq;
  int needIO;

  assert(cs);
  needIO=0;
  rq=LC_Request_List_First(cs->requests);
  while(rq) {
    int rv;
    LC_REQUEST *next;

    DBG_INFO(0, "Checking request %s",
             GWEN_DB_GetCharValue(LC_Request_GetInRequestData(rq),
                                  "command/vars/cmd", 0, 0));
    next=LC_Request_List_Next(rq);
    rv=LC_CardServer_CheckRequest(cs, rq);
    if (rv!=1) {
      DBG_DEBUG(0, "Removing request");
      LC_CardServer_RemoveRequest(cs, rq);
      needIO++;
    }
    rq=next;
  } /* while */

  return needIO?0:1;
}



void LC_CardServer__Up(GWEN_NETCONNECTION *conn){
}



void LC_CardServer__Down(GWEN_NETCONNECTION *conn){
  DBG_NOTICE(0, "Connection down");
}



int LC_CardServer_ReadConfig(LC_CARDSERVER *cs, GWEN_DB_NODE *db) {
  GWEN_DB_NODE *gr;
  int drivers;
  int servers;
  int permissions;

  assert(cs);
  assert(db);

  permissions=0666;
  drivers=0;
  servers=0;
  cs->disableAutoconf=
      GWEN_DB_GetIntValue(db, "disableAutoconf", 0, 0);
  if (cs->disableAutoconf) {
    DBG_WARN(0, "USB Autoconfiguration disabled");
  }
  else {
    DBG_NOTICE(0, "USB Autoconfiguration enabled");
  }

  cs->driverStartTimeout=
    GWEN_DB_GetIntValue(db,
                        "driverStartTimeout", 0,
                        LC_CARDSERVER_DEFAULT_DRIVERSTARTTIMEOUT);
  cs->driverStopTimeout=
    GWEN_DB_GetIntValue(db,
                        "driverStopTimeout", 0,
                        LC_CARDSERVER_DEFAULT_DRIVERSTOPTIMEOUT);
  cs->readerStartTimeout=
    GWEN_DB_GetIntValue(db,
                        "readerStartTimeout", 0,
                        LC_CARDSERVER_DEFAULT_READERSTARTTIMEOUT);

  cs->driverIdleTimeout=
    GWEN_DB_GetIntValue(db,
                        "driverIdleTimeout", 0,
                        LC_CARDSERVER_DEFAULT_DRIVERIDLETIMEOUT);

  cs->driverRestartTime=
    GWEN_DB_GetIntValue(db,
                        "driverRestartTime", 0,
                        LC_CARDSERVER_DEFAULT_DRIVERRSTTIME);

  cs->readerIdleTimeout=
    GWEN_DB_GetIntValue(db,
                        "readerIdleTimeout", 0,
                        LC_CARDSERVER_DEFAULT_READERIDLETIMEOUT);

  cs->readerCommandTimeout=
    GWEN_DB_GetIntValue(db,
                        "readerCommandTimeout", 0,
                        LC_CARDSERVER_DEFAULT_READERCMDTIMEOUT);

  cs->readerRestartTime=
    GWEN_DB_GetIntValue(db,
                        "readerRestartTime", 0,
                        LC_CARDSERVER_DEFAULT_READERRSTTIME);

  cs->usbScanInterval=
    GWEN_DB_GetIntValue(db,
			"usbScanInterval", 0,
                        LC_CARDSERVER_DEFAULT_USB_SCAN_INTERVAL);
  if (cs->usbScanInterval==0) {
    DBG_WARN(0, "Will only scan once for raw USB devices");
  }
  cs->usbTtyScanInterval=
    GWEN_DB_GetIntValue(db,
			"usbTtyScanInterval", 0,
			LC_CARDSERVER_DEFAULT_USBTTY_SCAN_INTERVAL);
  if (cs->usbTtyScanInterval==0) {
    DBG_WARN(0, "Will only scan once for ttyUSB devices");
  }

  gr=GWEN_DB_GetFirstGroup(db);
  while(gr) {
    if (strcasecmp(GWEN_DB_GroupName(gr), "driver")==0) {
      LC_DRIVER *d;
      GWEN_DB_NODE *dbR;

      /* driver section found, add it to global list and create a driver */
      GWEN_DB_InsertGroup(cs->dbDrivers,
			  GWEN_DB_Group_dup(gr));
      GWEN_DB_InsertGroup(cs->dbConfigDrivers,
			  GWEN_DB_Group_dup(gr));

      d=LC_Driver_FromDb(gr);
      assert(d);

      if (!LC_Driver_GetLogFile(d)) {
        GWEN_BUFFER *lbuf;

        lbuf=GWEN_Buffer_new(0, 256, 0, 1);
        LC_CardServer_ReplaceVar(LC_DEFAULT_LOGDIR
                                 "/drivers/@driver@"
                                 "/@reader@"
                                 ".log",
                                 "driver",
                                 LC_Driver_GetDriverName(d),
                                 lbuf);
        LC_Driver_SetLogFile(d, GWEN_Buffer_GetStart(lbuf));
        GWEN_Buffer_free(lbuf);
      }

      DBG_INFO(0, "Adding driver \"%s\"", LC_Driver_GetDriverName(d));
      LC_Driver_List_Add(d, cs->drivers);
      drivers++;

      dbR=GWEN_DB_GetFirstGroup(gr);
      while(dbR) {
        if (strcasecmp(GWEN_DB_GroupName(dbR), "reader")==0) {
          LC_READER *r;

          /* reader section found */
          r=LC_Reader_FromDb(d, dbR);
          assert(r);
          DBG_INFO(0, "Adding reader \"%s\"", LC_Reader_GetReaderName(r));
          /* readers from config file are always assumed available */
          LC_Reader_SetIsAvailable(r, 1);
          LC_Reader_List_Add(r, cs->readers);
        } /* if reader */
        dbR=GWEN_DB_GetNextGroup(dbR);
      } /* while */
    } /* if driver */

    else if (strcasecmp(GWEN_DB_GroupName(gr), "service")==0) {
      LC_SERVICE *as;

      /* service section found */
      as=LC_Service_FromDb(gr);
      assert(as);
      DBG_INFO(0, "Adding service \"%s\"", LC_Service_GetServiceName(as));
      LC_Service_List_Add(as, cs->services);
    }
    else if (strcasecmp(GWEN_DB_GroupName(gr), "server")==0) {
      const char *typ;
      GWEN_NETTRANSPORT *tr;
      GWEN_SOCKET *sk;
      GWEN_INETADDRESS *addr;
      GWEN_TYPE_UINT32 sid;
      const char *address;

      typ=GWEN_DB_GetCharValue(gr, "typ", 0, "local");
      address=GWEN_DB_GetCharValue(gr,
                                   "addr", 0, 0);
      if (!address) {
        DBG_ERROR(0, "No address given");
        return -1;
      }

      if (strcasecmp(typ, "local")==0) {
        char *taddr, *p;

        /* HTTP over UDS */
        taddr=strdup(address);
        p=strrchr(taddr, '/');
        if (p)
          *p=0;
        if (GWEN_Directory_GetPath(taddr, GWEN_PATH_FLAGS_CHECKROOT)) {
          DBG_ERROR(0, "Could not create path \"%s\"", taddr);
          free(taddr);
          return -1;
        }
        if (chmod(taddr, 0777)) {
          DBG_ERROR(0, "Could not set permissions for path \"%s\"", taddr);
          free(taddr);
          return -1;
        }
        free(taddr);
        unlink(address);
        permissions=GWEN_DB_GetIntValue(gr, "rights", 0, 0666);
        sk=GWEN_Socket_new(GWEN_SocketTypeUnix);
        addr=GWEN_InetAddr_new(GWEN_AddressFamilyUnix);
        GWEN_InetAddr_SetAddress(addr, address);
	tr=GWEN_NetTransportSocket_new(sk, 1);
      }
      else if (strcasecmp(typ, "public")==0) {
	/* HTTP over TCP */
	sk=GWEN_Socket_new(GWEN_SocketTypeTCP);
	addr=GWEN_InetAddr_new(GWEN_AddressFamilyIP);
	GWEN_InetAddr_SetAddress(addr,
				 GWEN_DB_GetCharValue
				 (gr,
				  "addr", 0,
				  "0.0.0.0"
				 )
				);
	GWEN_InetAddr_SetPort(addr,
			      GWEN_DB_GetIntValue(gr,
						  "port", 0,
						  LC_DEFAULT_PORT));
	tr=GWEN_NetTransportSocket_new(sk, 1);
      }
      else {
	const char *certDir;
        const char *newCertDir;
        const char *ownCertFile;
        const char *ciphers;
        GWEN_BUFFER *cfbuf;

        cfbuf=0;
        certDir=GWEN_DB_GetCharValue(gr, "certdir", 0,
                                     LC_DEFAULT_DATADIR"/"
                                     "certificates/valid");
        newCertDir=GWEN_DB_GetCharValue(gr, "newcertdir", 0,
                                        LC_DEFAULT_DATADIR"/"
                                        "certificates/new");
        ciphers=GWEN_DB_GetCharValue(gr, "ciphers", 0, 0);
        ownCertFile=GWEN_DB_GetCharValue(gr, "certfile", 0, 0);
        if (ownCertFile) {
          if (*ownCertFile!=DIRSEPC) {
            FILE *f;

            /* check whether the file exists */
            f=fopen(ownCertFile, "r");
            if (f)
              fclose(f);
            else {
              cfbuf=GWEN_Buffer_new(0, 256, 0, 1);
              GWEN_Buffer_AppendString(cfbuf, LC_DEFAULT_DATADIR);
              GWEN_Buffer_AppendByte(cfbuf, DIRSEPC);
              GWEN_Buffer_AppendString(cfbuf, ownCertFile);
              ownCertFile=GWEN_Buffer_GetStart(cfbuf);
            }
          }
        }
	addr=GWEN_InetAddr_new(GWEN_AddressFamilyIP);
	GWEN_InetAddr_SetAddress(addr,
				 GWEN_DB_GetCharValue(gr,
						      "addr", 0,
						      "0.0.0.0"));
	GWEN_InetAddr_SetPort(addr,
			      GWEN_DB_GetIntValue(gr,
						  "port", 0,
						  LC_DEFAULT_PORT));
	if (strcasecmp(typ, "private")==0) {
	  /* HTTP over SSL */
	  sk=GWEN_Socket_new(GWEN_SocketTypeTCP);
	  tr=GWEN_NetTransportSSL_new(sk,
                                      certDir,
                                      newCertDir,
                                      ownCertFile,
                                      LC_DEFAULT_DATADIR DIRSEP
                                      LC_DEFAULT_DHFILE,
				      0,
				      1);
	}
	else if (strcasecmp(typ, "secure")==0) {
	  /* HTTP over SSL with certificates */
	  sk=GWEN_Socket_new(GWEN_SocketTypeTCP);
	  tr=GWEN_NetTransportSSL_new(sk,
                                      certDir,
                                      newCertDir,
				      ownCertFile,
				      LC_DEFAULT_DHFILE,
				      1,
				      1);
	}
	else {
	  DBG_ERROR(0, "Unknown mode \"%s\"", typ);
          GWEN_InetAddr_free(addr);
          GWEN_Buffer_free(cfbuf);
	  return -1;
	}
        GWEN_Buffer_free(cfbuf);

        if (ciphers)
          GWEN_NetTransportSSL_SetCipherList(tr, ciphers);
      }

      GWEN_NetTransport_SetLocalAddr(tr, addr);
      GWEN_InetAddr_free(addr);
      sid=GWEN_IPCManager_AddServer(cs->ipcManager,
				    tr,
				    LC_CARDSERVER_MARK_SERVER);
      if (sid==0) {
	DBG_ERROR(0, "Could not add service");
	GWEN_DB_Dump(gr, stderr, 2);
        return -1;
      }

      if (strcasecmp(typ, "local")==0) {
        if (chmod(GWEN_DB_GetCharValue(gr,
                                       "addr", 0,
                                       LC_DEFAULT_UDS_SOCK),
                  permissions)) {
          DBG_ERROR(0, "Could not change permissions for service");
          GWEN_DB_Dump(gr, stderr, 2);
          return -1;
        }
      }
      GWEN_IPCManager_SetUpFn(cs->ipcManager, sid, LC_CardServer__Up);
      GWEN_IPCManager_SetDownFn(cs->ipcManager, sid,
				LC_CardServer__Down);

      DBG_INFO(0, "Service added");
      servers++;
      if (cs->typeForDrivers==0) {
        cs->typeForDrivers=strdup(GWEN_DB_GetCharValue(gr,
                                                       "typ", 0,
                                                       "local"));
        free(cs->addrForDrivers);
        cs->addrForDrivers=strdup(GWEN_DB_GetCharValue(gr,
                                                       "addr", 0,
                                                       "0.0.0.0"));
        cs->portForDrivers=GWEN_DB_GetIntValue(gr,
                                               "port", 0,
                                               LC_DEFAULT_PORT);
      }
    }

    gr=GWEN_DB_GetNextGroup(gr);
  } /* while */

  /*
  if (!drivers) {
    DBG_ERROR(0, "No drivers in this configuration");
    return -1;
  }
  */
  if (!servers) {
    DBG_ERROR(0, "No servers in this configuration");
    return -1;
  }
  return 0;
}



void LC_CardServer_CollectCommands(LC_CARDSERVER *cs) {
  //TODO
}



int LC_CardServer_RemoveCardsForReader(LC_CARDSERVER *cs, LC_READER *r) {
  LC_CARD *card;

  /* find cards in active ones */
  card=LC_Card_List_First(cs->activeCards);
  while(card) {
    LC_CARD *next;

    next=LC_Card_List_Next(card);
    if (LC_Card_GetReader(card)==r) {
      if (LC_Card_GetStatus(card)!=LC_CardStatusRemoved) {
        DBG_ERROR(0, "Found an active card for this reader ! (%04x)",
                  LC_Card_GetCardId(card));
        LC_Card_SetStatus(card, LC_CardStatusOrphaned);
        LC_Reader_DecUsageCount(r);
      }
    }
    card=next;
  } /* while */

  /* find cards in free ones */
  card=LC_Card_List_First(cs->activeCards);
  while(card) {
    LC_CARD *next;

    next=LC_Card_List_Next(card);
    if (LC_Card_GetReader(card)==r) {
      DBG_INFO(0, "Removing free card \"%08x\"",
               LC_Card_GetCardId(card));
      LC_Card_SetStatus(card, LC_CardStatusOrphaned);
      LC_Card_List_Del(card);
      LC_Card_free(card);
    }
    card=next;
  } /* while */

  return 0;
}



int LC_CardServer_RemoveCardsAt(LC_CARDSERVER *cs,
                                LC_READER *r,
                                unsigned int slotNum) {
  LC_CARD *card;

  /* find card in active ones */
  card=LC_Card_List_First(cs->activeCards);
  while(card) {
    if (LC_Card_GetReader(card)==r &&
        LC_Card_GetSlot(card)==slotNum) {
      /* we got a card which was at the place of the new card */
      if (LC_Card_GetStatus(card)!=LC_CardStatusRemoved) {
        DBG_WARN(0, "Active card \"%08x\" removed",
                 LC_Card_GetCardId(card));
        LC_Card_SetStatus(card, LC_CardStatusRemoved);
        LC_Reader_DecUsageCount(r);
      }
    }
    card=LC_Card_List_Next(card);
  } /* while */

  /* find card in free ones */
  card=LC_Card_List_First(cs->freeCards);
  while(card) {
    LC_CARD *next;

    next=LC_Card_List_Next(card);
    if (LC_Card_GetReader(card)==r &&
        LC_Card_GetSlot(card)==slotNum) {
      /* we got a card which was at the place of the new card */
      if (LC_Card_GetStatus(card)!=LC_CardStatusRemoved) {
        DBG_WARN(0, "Unused card \"%08x\" removed", LC_Card_GetCardId(card));
        LC_Card_SetStatus(card, LC_CardStatusRemoved);
        LC_Card_List_Del(card);
        LC_Card_free(card);
      }
    }
    card=next;
  } /* while */

  return 0;
}



int LC_CardServer_SendErrorResponse(LC_CARDSERVER *cs,
                                    GWEN_TYPE_UINT32 rid,
                                    int code,
				    const char *text) {
  GWEN_DB_NODE *gr;

  gr=GWEN_DB_Group_new("Error");

  GWEN_DB_SetIntValue(gr, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "code", code);
  if (text)
    GWEN_DB_SetCharValue(gr, GWEN_DB_FLAGS_OVERWRITE_VARS,
			 "text", text);
  if (GWEN_IPCManager_SendResponse(cs->ipcManager, rid, gr)) {
    DBG_ERROR(0, "Could not send command");
    return -1;
  }

  return 0;
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
    LC_Driver_SetStatus(d, LC_DriverStatusAborted);
    ebuf=GWEN_Buffer_new(0, 256, 0, 1);
    GWEN_Buffer_AppendString(ebuf, "Driver error (");
    GWEN_Buffer_AppendString(ebuf, text);
    GWEN_Buffer_AppendString(ebuf, ")");
    LC_CardServer_SendDriverNotification(cs, 0,
					 LC_NOTIFY_CODE_DRIVER_ERROR,
					 d,
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



int LC_CardServer_HandleServiceReady(LC_CARDSERVER *cs,
                                     GWEN_TYPE_UINT32 rid,
                                     GWEN_DB_NODE *dbReq){
  GWEN_DB_NODE *dbRsp;
  GWEN_TYPE_UINT32 serviceId;
  GWEN_TYPE_UINT32 nodeId;
  LC_SERVICE *as;
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

  DBG_NOTICE(0, "Service %08x: ServiceReady", nodeId);

  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/serviceId", 0, "0"),
                "%x", &i)) {
    DBG_ERROR(0, "Invalid service id");
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
    }
    return -1;
  }
  serviceId=i;
  if (serviceId==0) {
    DBG_ERROR(0, "Invalid service id");
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
    }
    return -1;
  }

  /* find service */
  as=LC_Service_List_First(cs->services);
  while(as) {
    if (LC_Service_GetServiceId(as)==serviceId)
      break;
    as=LC_Service_List_Next(as);
  } /* while */
  if (!as) {
    DBG_ERROR(0, "Service \"%08x\" not found", serviceId);
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Service not found");
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
    }
    return -1;
  }

  /* store node id */
  LC_Service_SetIpcId(as, nodeId);

  /* take over connection */
  conn=GWEN_IPCManager_GetConnection(cs->ipcManager, nodeId);
  LC_ServerConn_TakeOver(conn);
  LC_ServerConn_SetCardServer(conn, cs);
  LC_ServerConn_SetService(conn, as);
  LC_ServerConn_SetType(conn, LC_ServerConn_TypeService);

  /* check code */
  code=GWEN_DB_GetCharValue(dbReq, "body/code", 0, "<none>");
  text=GWEN_DB_GetCharValue(dbReq, "body/text", 0, "<none>");
  if (strcasecmp(code, "OK")!=0) {
    DBG_ERROR(0, "Error in service \"%08x\": %s",
              serviceId, text);
    LC_Service_SetStatus(as, LC_ServiceStatusAborted);
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
    }
    return -1;
  }
  DBG_NOTICE(0, "Service \"%08x\" is up (%s)", serviceId, text);
  LC_Service_SetStatus(as, LC_ServiceStatusUp);

  dbRsp=GWEN_DB_Group_new("ServiceReadyResponse");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "code", "OK");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "text", "Service registered");
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



int LC_CardServer_HandleOfferService(LC_CARDSERVER *cs,
                                     GWEN_TYPE_UINT32 rid,
                                     GWEN_DB_NODE *dbReq){
  GWEN_DB_NODE *dbRsp;
  GWEN_TYPE_UINT32 clientId;
  GWEN_TYPE_UINT32 serviceId=0;
  const char *serviceName;
  LC_SERVICE *sv;
  const char *text=0;

  assert(dbReq);

  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeId", 0, 0);
  if (!clientId) {
    DBG_ERROR(0, "Invalid node id");
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
    }
    return -1;
  }

  DBG_NOTICE(0, "Client %08x: OfferService", clientId);
  serviceName=GWEN_DB_GetCharValue(dbReq, "body/serviceName", 0, 0);
  if (serviceName==0) {
    DBG_ERROR(0, "Missing service name");
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Missing service name");
    return -1;
  }

  /* search for service */
  sv=LC_Service_List_First(cs->services);
  while(sv) {
    const char *sn;

    sn=LC_Service_GetServiceName(sv);
    if (sn)
      if (strcasecmp(sn, serviceName)==0)
        break;
    sv=LC_Service_List_Next(sv);
  }
  if (sv) {
    DBG_ERROR(0, "Service \"%s\" already exists", serviceName);
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Service already exists");
    return -1;
  }

  /* create new client-service, add it */
  sv=LC_Service_new();
  LC_Service_SetServiceName(sv, serviceName);
  LC_Service_AddFlags(sv, LC_SERVICE_FLAGS_CLIENT);
  LC_Service_SetIpcId(sv, clientId);
  LC_Service_List_Add(sv, cs->services);

  DBG_NOTICE(0, "Service \"%08x\" is up (%s)", serviceId, text);
  LC_Service_SetStatus(sv, LC_ServiceStatusUp);

  /* send response */
  dbRsp=GWEN_DB_Group_new("OfferServiceResponse");
  GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "code", LC_ERROR_NONE);
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "text", "Service registered");
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



int LC_CardServer_HandleCardInserted(LC_CARDSERVER *cs,
				     GWEN_TYPE_UINT32 rid,
				     GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 readerId;
  GWEN_TYPE_UINT32 nodeId;
  LC_READER *r;
  int slotNum;
  GWEN_TYPE_UINT32 cardNum;
  LC_CARD *card;
  GWEN_BUFFER *atr;
  const void *p;
  unsigned int bs;
  const char *cardType;
  LC_CARD_TYPE ct;

  nodeId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeId", 0, 0);
  if (!nodeId) {
    DBG_ERROR(0, "Invalid node id");
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
    }
    return -1;
  }

  DBG_NOTICE(0, "Driver %08x: Card inserted", nodeId);

  /* driver ready */
  if (sscanf(GWEN_DB_GetCharValue(dbReq, "body/readerId", 0, "0"),
             "%x",
	     &readerId)!=1) {
    DBG_ERROR(0, "Bad reader id in command \"%s\"",
              GWEN_DB_GroupName(dbReq));
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }

  slotNum=GWEN_DB_GetIntValue(dbReq, "body/slotNum", 0, 0),
  cardNum=GWEN_DB_GetIntValue(dbReq, "body/cardNum", 0, 0),
  cardType=GWEN_DB_GetCharValue(dbReq, "body/cardType", 0, "unknown");
  if (strcasecmp(cardType, "processor")==0)
    ct=LC_CardTypeProcessor;
  else if (strcasecmp(cardType, "memory")==0)
    ct=LC_CardTypeMemory;
  else {
    DBG_WARN(0, "Unknown card type \"%s\"", cardType);
    ct=LC_CardTypeUnknown;
  }
  atr=0;
  p=GWEN_DB_GetBinValue(dbReq, "body/atr", 0, 0, 0, &bs);
  if (p && bs) {
    atr=GWEN_Buffer_new(0, bs, 0, 1);
    GWEN_Buffer_AppendBytes(atr, p, bs);
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
    GWEN_Buffer_free(atr);
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }

  /* TODO: Check for reader status */

  LC_CardServer_RemoveCardsAt(cs, r, slotNum);

  card=LC_Card_new(r, slotNum, cardNum, ct, atr);
  LC_Card_SetStatus(card, LC_CardStatusInserted);
  LC_Card_List_Add(card, cs->freeCards);
  DBG_NOTICE(0, "Free card added with id \"%08x\" in reader \"%s\"(%08x)",
             LC_Card_GetCardId(card),
             LC_Reader_GetReaderName(r),
             readerId);
  GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
  return 0;
}



int LC_CardServer_HandleCardRemoved(LC_CARDSERVER *cs,
				    GWEN_TYPE_UINT32 rid,
				    GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 readerId;
  GWEN_TYPE_UINT32 nodeId;
  LC_READER *r;
  int slotNum;
  GWEN_TYPE_UINT32 cardNum;

  nodeId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeId", 0, 0);
  if (!nodeId) {
    DBG_ERROR(0, "Invalid node id");
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
    }
    return -1;
  }

  DBG_NOTICE(0, "Driver %08x: Card removed", nodeId);

  /* driver ready */
  if (sscanf(GWEN_DB_GetCharValue(dbReq, "body/readerId", 0, "0"),
             "%x",
             &readerId)!=1) {
    DBG_ERROR(0, "Bad reader id in command \"%s\"",
	      GWEN_DB_GroupName(dbReq));
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }

  slotNum=GWEN_DB_GetIntValue(dbReq, "body/slotNum", 0, 0),
  cardNum=GWEN_DB_GetIntValue(dbReq, "body/cardNum", 0, 0),

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

  /* TODO: Check for reader status */

  LC_CardServer_RemoveCardsAt(cs, r, slotNum);

  GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
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
  LC_Reader_SetStatus(r, LC_ReaderStatusAborted);
  ebuf=GWEN_Buffer_new(0, 256, 0, 1);
  GWEN_Buffer_AppendString(ebuf, "Reader error (");
  GWEN_Buffer_AppendString(ebuf, txt);
  GWEN_Buffer_AppendString(ebuf, ")");
  LC_CardServer_SendReaderNotification(cs, 0,
				       LC_NOTIFY_CODE_READER_ERROR,
				       r,
				       GWEN_Buffer_GetStart(ebuf));
  GWEN_Buffer_free(ebuf);

  /* FIXME: Test: 2004/06/11 */
  LC_Driver_DecActiveReadersCount(LC_Reader_GetDriver(r));
  GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
  return 0;
}



GWEN_TYPE_UINT32 LC_CardServer_GetFlags(GWEN_DB_NODE *db,
                                        const char *vname){
  unsigned int i;
  GWEN_TYPE_UINT32 f;

  f=0;
  for (i=0; ; i++) {
    const char *p;

    p=GWEN_DB_GetCharValue(db, vname, i, 0);
    if (!p)
      break;
    if (strcasecmp(p, "KEYPAD")==0)
      f|=LC_READER_FLAGS_KEYPAD;
    else if (strcasecmp(p, "DISPLAY")==0)
      f|=LC_READER_FLAGS_DISPLAY;
  } /* for */

  return f;
}



int LC_CardServer_HandleClientReady(LC_CARDSERVER *cs,
				    GWEN_TYPE_UINT32 rid,
				    GWEN_DB_NODE *dbReq){
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_DB_NODE *dbRsp;
  GWEN_NETCONNECTION *conn;
  GWEN_NETTRANSPORT *tr;
  const char *p;

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

  cl=LC_Client_List_First(cs->clients);
  while(cl) {
    if (LC_Client_GetClientId(cl)==clientId)
      break;
    cl=LC_Client_List_Next(cl);
  } /* while */
  if (cl) {
    DBG_ERROR(0, "Client already started");
    /* TODO: Send SegResult */
    return -1;
  }

  cl=LC_Client_new(clientId);
  LC_Client_List_Add(cl, cs->clients);

  conn=GWEN_IPCManager_GetConnection(cs->ipcManager, clientId);
  assert(conn);
  tr=GWEN_NetConnection_GetTransportLayer(conn);
  assert(tr);
  LC_ServerConn_TakeOver(conn);
  LC_ServerConn_SetCardServer(conn, cs);
  LC_ServerConn_SetType(conn, LC_ServerConn_TypeClient);

  LC_Client_SetUserName(cl, "nobody");
  if (GWEN_NetTransportSSL_IsOfType(tr)){
    DBG_INFO(0, "Got an SSL connection, checking...");
    if (GWEN_NetTransportSSL_IsSecure(tr)) {
      GWEN_DB_NODE *dbCert;
      const char *p;

      DBG_INFO(0, "Got a secure SSL connection");
      dbCert=GWEN_NetTransportSSL_GetPeerCertificate(tr);
      assert(dbCert);

      p=GWEN_DB_GetCharValue(dbCert, "commonName", 0, "nobody");
      DBG_NOTICE(0, "Verified peer is \"%s\"", p);
      LC_Client_SetUserName(cl, p);
    }
  }

  DBG_NOTICE(0, "Client %08x: ClientReady [%s/%s]",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  p=GWEN_DB_GetCharValue(dbReq, "body/application", 0, "");
  DBG_NOTICE(0, "Client \"%08x\" started (%s, Gwen %s, ChipCard %s)",
             clientId,
             p,
	     GWEN_DB_GetCharValue(dbReq, "body/GwenVersion", 0, ""),
	     GWEN_DB_GetCharValue(dbReq, "body/ChipcardVersion", 0, ""));

  LC_Client_SetApplicationName(cl, p);

  /* create response for client */
  dbRsp=GWEN_DB_Group_new("ClientReadyResponse");
  GWEN_DB_SetCharValue(dbRsp,
		       GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "GwenVersion", GWENHYWFAR_VERSION_FULL_STRING);
  GWEN_DB_SetCharValue(dbRsp,
		       GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "ChipcardVersion", CHIPCARD_VERSION_FULL_STRING);
  GWEN_DB_SetCharValue(dbRsp,
		       GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "System", ""); /* TODO: Get system string */

  /* send response */
  if (GWEN_IPCManager_SendResponse(cs->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response to ClientReady");
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }

  /* remove request */
  if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0)) {
    DBG_WARN(0, "Could not remove ClientReady request");
  }

  DBG_DEBUG(0, "Response sent.");
  return 0;
}



int LC_CardServer_HandleStartWait(LC_CARDSERVER *cs,
				  GWEN_TYPE_UINT32 rid,
				  GWEN_DB_NODE *dbReq){
  LC_READER *r;
  GWEN_TYPE_UINT32 flags;
  GWEN_TYPE_UINT32 mask;
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_TYPE_UINT32 waitReqId;
  GWEN_DB_NODE *dbRsp;
  unsigned int readers;

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

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
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }

  DBG_NOTICE(0, "Client %08x: StartWait [%s/%s]",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* allow all cards to be seen */
  LC_Client_DelAllCards(cl);

  flags=LC_CardServer_GetFlags(dbReq, "body/flags");
  mask=LC_CardServer_GetFlags(dbReq, "body/mask");

  readers=0;
  r=LC_Reader_List_First(cs->readers);
  while(r) {
    if (!((LC_Reader_GetFlags(r) ^ flags) & mask)) {
      /* reader matches flags, does the client already use it ? */
      if (LC_Client_HasReader(cl, LC_Reader_GetReaderId(r))) {
        /* yes, simply do nothing about it */
        DBG_INFO(0, "Reader \"%s\" is already known to client %08x",
                 LC_Reader_GetReaderName(r), clientId);
      }
      else {
	if (LC_Reader_IsAvailable(r)) {
	  /* add the reader */
	  DBG_INFO(0, "Adding reader \"%s\" for client %08x",
		   LC_Reader_GetReaderName(r), clientId);
	  DBG_DEBUG(0, "Calling LC_Reader_IncUsageCount");
	  LC_Reader_IncUsageCount(r);
	  LC_Client_AddReader(cl, LC_Reader_GetReaderId(r));
	  readers++;
	  /* is reader down ? */
	  if (LC_Reader_GetStatus(r)==LC_ReaderStatusDown) {
	    DBG_NOTICE(0, "Starting reader \"%s\" on account of client %08x",
		       LC_Reader_GetReaderName(r), clientId);
	    if (LC_CardServer_StartReader(cs, r)) {
	      DBG_ERROR(0, "Could not start reader \"%s\"",
			LC_Reader_GetReaderName(r));
	      LC_Client_DelReader(cl, LC_Reader_GetReaderId(r));
	      readers--;
	    }
	  } /* if reader down */
	} /* if reader is available */
      } /* if reader is new to the client */
    } /* if reader matches the given flags */
    r=LC_Reader_List_Next(r);
  } /* while */

  /* create response for client */
  dbRsp=GWEN_DB_Group_new("StartWaitResponse");
  if (readers) {
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
			 "code", "OK");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
			 "text", "Waiting for cards");
  }
  else {
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
			 "code", "ERROR");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
			 "text", "No matching reader");
  }
  /* send response */
  if (GWEN_IPCManager_SendResponse(cs->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response to client");
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }
  DBG_DEBUG(0, "Response sent.");
  if (readers) {
    LC_Client_AddWaitRequestCount(cl);
    LC_Client_AddWaitReaderState(cl, flags, mask);
  }

  /* remove old StartWaitRequest (if any) */
  waitReqId=LC_Client_GetLastWaitRequestId(cl);
  if (waitReqId) {
    /* remove previous wait request, store new one */
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, waitReqId, 0);
    LC_Client_SubWaitRequestCount(cl);
  }
  LC_Client_SetLastWaitRequestId(cl, rid);

  return 0;
}



int LC_CardServer_HandleStopWait(LC_CARDSERVER *cs,
				 GWEN_TYPE_UINT32 rid,
				 GWEN_DB_NODE *dbReq){
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_TYPE_UINT32 waitReqId;
  GWEN_DB_NODE *dbRsp;

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

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
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }

  DBG_NOTICE(0, "Client %08x: StopWait [%s/%s]",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* remove WaitRequest (if any) */
  waitReqId=LC_Client_GetLastWaitRequestId(cl);
  if (waitReqId) {
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, waitReqId, 0);
    LC_Client_SetLastWaitRequestId(cl, 0);
  }

  dbRsp=GWEN_DB_Group_new("StopWaitResponse");
  if (LC_Client_GetWaitRequestCount(cl)) {
    LC_Client_SubWaitRequestCount(cl);
    if (LC_Client_GetWaitRequestCount(cl)==0) {
      LC_READER *r;
  
      r=LC_Reader_List_First(cs->readers);
      while(r) {
	LC_READER *next;

	next=LC_Reader_List_Next(r);
	if (LC_Client_HasReader(cl, LC_Reader_GetReaderId(r))) {
          LC_Client_DelReader(cl,LC_Reader_GetReaderId(r));
          DBG_VERBOUS(0, "Calling LC_Reader_DecUsageCount");
          LC_Reader_DecUsageCount(r);
        }
	r=next;
      } /* while */
    } /* if no wait request left */
  
    /* create response for client */
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
			 "code", "OK");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
			 "text", "No longer waiting for cards");
  }
  else {
    /* create response for client */
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
			 "code", "ERROR");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
			 "text", "Not waiting for cards");
  }
  /* send response */
  if (GWEN_IPCManager_SendResponse(cs->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response to client");
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }
  DBG_DEBUG(0, "Response sent.");

  GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
  return 0;
}



int LC_CardServer_HandleTakeCard(LC_CARDSERVER *cs,
				 GWEN_TYPE_UINT32 rid,
				 GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 cardId;
  LC_CARD *card;
  LC_REQUEST *rq;
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

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

  DBG_NOTICE(0, "Client %08x: TakeCard [%s/%s]",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* get card id */
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/cardid", 0, "0"),
		"%x", &cardId)) {
    DBG_ERROR(0, "Bad server message");
    return -1;
  }

  /* search for card in free list */
  card=LC_Card_List_First(cs->freeCards);
  while(card) {
    if (LC_Card_GetCardId(card)==cardId) {
      /* card found */
      break;
    }
    card=LC_Card_List_Next(card);
  } /* while */

  if (!card) {
    /* search for card in active list */
    card=LC_Card_List_First(cs->activeCards);
    while(card) {
      if (LC_Card_GetCardId(card)==cardId) {
	/* card found */
        break;
      }
      card=LC_Card_List_Next(card);
    } /* while */
  }

  if (!card) {
    DBG_ERROR(0, "No card with id \"%08x\" found", cardId);
    return -1;
  }

  if (LC_Card_GetStatus(card)==LC_CardStatusRemoved) {
    DBG_ERROR(0, "Card has been removed");
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_CARD_REMOVED,
                                    "Card has been removed");
    return -1;
  }

  LC_Card_AddWaitingClient(card, LC_Client_GetClientId(cl));
  rq=LC_Request_new(cl,
		    GWEN_DB_Group_dup(dbReq),
                    0, card);
  LC_Request_SetInRequestId(rq, rid);
  LC_Request_List_Add(rq, cs->requests);
  return 0;
}



int LC_CardServer_HandleReleaseCard(LC_CARDSERVER *cs,
				    GWEN_TYPE_UINT32 rid,
				    GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 cardId;
  LC_CARD *card;
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

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

  DBG_NOTICE(0, "Client %08x: ReleaseCard [%s/%s]",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* get card id */
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
        GWEN_DB_NODE *dbRsp;
        GWEN_TYPE_UINT32 ridReset;

	LC_Card_SetClient(card, 0);
	LC_Card_List_Del(card);
	LC_Card_List_Add(card, cs->freeCards);

	/* detach card from reader */
	if (LC_Card_GetStatus(card)!=LC_CardStatusRemoved) {
	  DBG_DEBUG(0, "Calling LC_Reader_DecUsageCount");
	  LC_Reader_DecUsageCount(LC_Card_GetReader(card));
	}

        ridReset=LC_CardServer_SendResetCard(cs, card);
        if (ridReset==0) {
          DBG_ERROR(0, "Could not send card reset request");
        }
        else
          /* we don't expect an answer, delete request */
          GWEN_IPCManager_RemoveRequest(cs->ipcManager, ridReset, 1);

	/* create response for client */
	dbRsp=GWEN_DB_Group_new("ReleaseCardResponse");
	GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
			     "code", "OK");
	GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
			     "text", "Card released");

	/* send response */
	if (GWEN_IPCManager_SendResponse(cs->ipcManager, rid, dbRsp)) {
	  DBG_ERROR(0, "Could not send response to client");
	  return -1;
	}
	DBG_DEBUG(0, "Response sent.");
	GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
	return 0;
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

  DBG_ERROR(0, "No card with id \"%08x\" found", cardId);
  LC_CardServer_SendErrorResponse(cs, rid,
                                  LC_ERROR_INVALID,
				  "Unknown card id");
  return -1;
}



int LC_CardServer_HandleCommandCard(LC_CARDSERVER *cs,
				    GWEN_TYPE_UINT32 rid,
				    GWEN_DB_NODE *dbReq){
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_TYPE_UINT32 cardId;
  LC_CARD *card;
  LC_READER *r;
  LC_DRIVER *d;
  LC_REQUEST *rq;
  const void *p;
  unsigned int bs;
  const char *target;
  char numbuf[16];
  GWEN_DB_NODE *dbDriverReq;

  DBG_DEBUG(0, "Command card request %08x", rid);
  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

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

  DBG_NOTICE(0, "Client %08x: CommandCard [%s/%s]",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* get card id */
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/cardid", 0, "0"),
		"%x", &cardId)) {
    DBG_ERROR(0, "Bad server message");
    return -1;
  }

  p=GWEN_DB_GetBinValue(dbReq, "body/data", 0, 0, 0, &bs);
  if (!p || !bs) {
    DBG_ERROR(0, "No data give");
    return -1;
  }

  target=GWEN_DB_GetCharValue(dbReq, "body/target", 0, 0);
  if (!target) {
    DBG_ERROR(0, "No target given");
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

  /* create outbound command */
  r=LC_Card_GetReader(card);
  assert(r);
  d=LC_Reader_GetDriver(r);
  assert(d);
  dbDriverReq=GWEN_DB_Group_new("CardCommand");

  snprintf(numbuf, sizeof(numbuf)-1, "%08x", LC_Reader_GetReaderId(r));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbDriverReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "readerId", numbuf);
  GWEN_DB_SetIntValue(dbDriverReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "slotnum", LC_Card_GetSlot(card));
  GWEN_DB_SetIntValue(dbDriverReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
		      "cardnum", LC_Card_GetReadersCardId(card));
  GWEN_DB_SetBinValue(dbDriverReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "data", p, bs);
  GWEN_DB_SetCharValue(dbDriverReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "target", target);

  /* store request */
  rq=LC_Request_new(cl,
                    GWEN_DB_Group_dup(dbReq),
                    dbDriverReq,
                    card);
  LC_Request_SetInRequestId(rq, rid);
  LC_Reader_AddRequest(r, rq);

  DBG_INFO(0, "Request enqueued");
  return 0;
}



int LC_CardServer_HandleExecCommand(LC_CARDSERVER *cs,
                                    GWEN_TYPE_UINT32 rid,
                                    GWEN_DB_NODE *dbReq){
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_TYPE_UINT32 cardId;
  LC_CARD *card;
  LC_CARDMGR_RESULT res;
  GWEN_DB_NODE *dbRsp;

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

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

  DBG_NOTICE(0, "Client %08x: ExecCommand (%s) [%s/%s]",
             clientId,
             GWEN_DB_GetCharValue(dbReq,
                                  "body/command/name", 0, "no command"),
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* get card id */
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

  /* create outbound command */
  dbRsp=GWEN_DB_Group_new("ExecCommandResponse");
  res=LC_CardMgr_HandleCommand(cs->cardManager,
                               card,
                               rid,
                               dbReq,
                               dbRsp);
  if (res==LC_CardMgr_ResultOk) {
    DBG_DEBUG(0, "Command enqueued");
  }
  else if (res==LC_CardMgr_ResultImmediateResponse) {
    GWEN_DB_SetIntValue(dbRsp,
                        GWEN_DB_FLAGS_OVERWRITE_VARS,
                        "code", LC_ERROR_NONE);
    GWEN_DB_SetCharValue(dbRsp,
                         GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", "Command executed");
    if (GWEN_IPCManager_SendResponse(cs->ipcManager,
                                     rid,
                                     dbRsp)) {
      DBG_ERROR(0, "Could not send ExecCommand response");
      return -1;
    }
  }
  else {
    if (strcasecmp(GWEN_DB_GroupName(dbRsp), "error")==0) {
      int code;
      const char *text;

      code=GWEN_DB_GetIntValue(dbRsp, "code", 0, LC_ERROR_GENERIC);
      text=GWEN_DB_GetCharValue(dbRsp, "text", 0, 0);
      DBG_ERROR(0, "Bad result %d (%d: %s)", res, code, text);
      GWEN_DB_Dump(dbRsp, stderr, 2);
      LC_CardServer_SendErrorResponse(cs, rid,
                                      code,
                                      text?text:"Error handling command");
    }
    else {
      DBG_ERROR(0, "Bad result %d (generic error)", res);
      LC_CardServer_SendErrorResponse(cs, rid,
                                      LC_ERROR_GENERIC,
                                      "Error handling command");
    }
    GWEN_DB_Group_free(dbRsp);
    return -1;
  }

  return 0;
}



int LC_CardServer_HandleSelectCard(LC_CARDSERVER *cs,
                                   GWEN_TYPE_UINT32 rid,
                                   GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 cardId;
  LC_CARD *card;
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  const char *cardName;
  GWEN_DB_NODE *dbRsp;

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

  cardName=GWEN_DB_GetCharValue(dbReq, "body/cardName", 0, 0);
  if (!cardName) {
    if (LC_CardServer_SendErrorResponse(cs, rid,
					LC_ERROR_INVALID,
					"Command incomplete")) {
      DBG_ERROR(0, "Could not send response");
    }
    return -1;
  }

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

  DBG_NOTICE(0, "Client %08x: SelectCard (%s) [%s/%s]",
             clientId,
             cardName,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* get card id */
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
      if (LC_Card_GetClient(card)==cl)
        break;
      DBG_ERROR(0, "Card \"%08x\" not owned by client \"%08x\"",
                cardId, LC_Client_GetClientId(cl));
      if (LC_CardServer_SendErrorResponse(cs, rid,
                                          LC_ERROR_CARD_NOT_OWNED,
                                          "Card is not owned by you")){
        DBG_ERROR(0, "Could not send response");
      }
      return -1;
    }
    card=LC_Card_List_Next(card);
  } /* while */

  if (!card) {
    DBG_ERROR(0, "No card with id \"%08x\" found", cardId);
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Unknown card id");
    return -1;
  }

  if (LC_Card_GetStatus(card)==LC_CardStatusRemoved) {
    DBG_ERROR(0, "Card has been removed");
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_CARD_REMOVED,
                                    "Card has been removed");
    return -1;
  }

  if (LC_CardMgr_SelectCard(cs->cardManager,
                            card,
                            cardName)) {
    if (LC_CardServer_SendErrorResponse(cs, rid,
                                        LC_ERROR_INVALID,
                                        "Could not select card/app")){
      DBG_ERROR(0, "Could not send response");
    }
    return -1;
  }

  /* create response for client */
  dbRsp=GWEN_DB_Group_new("SelectCardResponse");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "code", "OK");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "text", "Card and application selected");

  /* send response */
  if (GWEN_IPCManager_SendResponse(cs->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response to client");
    return -1;
  }
  DBG_DEBUG(0, "Response sent.");
  GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
  return 0;
}



int LC_CardServer_HandleSetNotify(LC_CARDSERVER *cs,
                                  GWEN_TYPE_UINT32 rid,
                                  GWEN_DB_NODE *dbReq){
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_DB_NODE *dbRsp;
  GWEN_TYPE_UINT32 flags;
  int i;
  GWEN_BUFFER *ebuf;
  int err;

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

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
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }

  DBG_NOTICE(0, "Client %08x: SetNotify [%s/%s]",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  flags=0;
  err=0;
  ebuf=GWEN_Buffer_new(0, 256, 0, 1);
  for (i=0; ; i++) {
    const char *s;

    s=GWEN_DB_GetCharValue(dbReq, "body/flag", i, 0);
    if (!s)
      break;
    else {
      const char *p;

      p=strchr(s, ':');
      if (p) {
        char ntype[32];
        int len;
        GWEN_TYPE_UINT32 lflags;

        len=p-s;
        if (len>=sizeof(ntype)) {
          DBG_ERROR(0, "Notification type too long (%s)", s);
          GWEN_Buffer_AppendString(ebuf,
                                   "Notification type/code too long (");
          GWEN_Buffer_AppendString(ebuf, s);
          GWEN_Buffer_AppendString(ebuf, ")");
          err++;
          break;
        }
        strncpy(ntype, s, len);
        ntype[len]=0;
        p++;
        lflags=LC_CardServer_GetNotificationMask(ntype, p);
        if (!lflags) {
          DBG_ERROR(0, "Unknown notification type/code (%s)", s);
          GWEN_Buffer_AppendString(ebuf,
                                   "Bad Notification type/code (");
          GWEN_Buffer_AppendString(ebuf, s);
          GWEN_Buffer_AppendString(ebuf, ")");
          err++;
          break;
        }
        if (lflags & ~LC_Client_GetNotifyMask(cl)) {
          DBG_ERROR(0, "Notification type/code not allowed (%s)", s);
          GWEN_Buffer_AppendString(ebuf,
                                   "Notification type/code not allowed (");
          GWEN_Buffer_AppendString(ebuf, s);
          GWEN_Buffer_AppendString(ebuf, ")");
          err++;
          break;
        }
        else {
          flags|=lflags;
        }
      } /* if ":" found */
      else {
        DBG_ERROR(0, "Bad notification type/code (%s)", s);
        GWEN_Buffer_AppendString(ebuf,
                                 "Bad Notification type/code (");
        GWEN_Buffer_AppendString(ebuf, s);
        GWEN_Buffer_AppendString(ebuf, ")");
        err++;
        break;
      }
    } /* if type/code pair found */
  } /* for */

  /* create response for client */
  dbRsp=GWEN_DB_Group_new("SetNotifyResponse");
  if (err) {
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "code", "ERROR");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", GWEN_Buffer_GetStart(ebuf));
  }
  else {
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "code", "OK");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", "Notification types/codes ok");
  }
  GWEN_Buffer_free(ebuf);

  DBG_NOTICE(0, "Setting notify flags %08x", flags);
  LC_Client_SetNotifyFlags(cl, flags);

  /* send initial notifications to this client */
  if (flags & LC_NOTIFY_FLAGS_CLIENT_MASK) {
    /* TODO */
  }

  if (flags & LC_NOTIFY_FLAGS_SERVICE_MASK) {
    /* TODO */
  }

  if (flags & LC_NOTIFY_FLAGS_DRIVER_MASK) {
    LC_DRIVER *d;

    d=LC_Driver_List_First(cs->drivers);
    while(d) {
      switch(LC_Driver_GetStatus(d)) {
      case LC_DriverStatusStarted:
	if (flags & LC_NOTIFY_FLAGS_DRIVER_START) {
	  LC_CardServer_SendDriverNotification(cs, cl,
					       LC_NOTIFY_CODE_DRIVER_START,
					       d,
					       "Driver started");
	}
	break;
      case LC_DriverStatusStopping:
      case LC_DriverStatusUp:
	if (flags & LC_NOTIFY_FLAGS_DRIVER_UP) {
	  LC_CardServer_SendDriverNotification(cs, cl,
					       LC_NOTIFY_CODE_DRIVER_UP,
					       d,
					       "Driver is up");
	}
	break;
      case LC_DriverStatusDown:
	if (flags & LC_NOTIFY_FLAGS_DRIVER_DOWN) {
	  LC_CardServer_SendDriverNotification(cs, cl,
					       LC_NOTIFY_CODE_DRIVER_UP,
					       d,
					       "Driver is down");
	}
	break;
      case LC_DriverStatusAborted:
	if (flags & LC_NOTIFY_FLAGS_DRIVER_ERROR) {
	  LC_CardServer_SendDriverNotification(cs, cl,
					       LC_NOTIFY_CODE_DRIVER_UP,
					       d,
					       "Driver aborted");
	}
	break;
      case LC_DriverStatusDisabled:
	if (flags & LC_NOTIFY_FLAGS_DRIVER_ERROR) {
	  LC_CardServer_SendDriverNotification(cs, cl,
					       LC_NOTIFY_CODE_DRIVER_UP,
					       d,
					       "Driver disabled");
	}
	break;
      default:
        break;
      } /* switch */
      d=LC_Driver_List_Next(d);
    }
  } /* if driver mask */

  if (flags & LC_NOTIFY_FLAGS_READER_MASK) {
    LC_READER *r;

    r=LC_Reader_List_First(cs->readers);
    while(r) {
      switch(LC_Reader_GetStatus(r)) {
      case LC_ReaderStatusWaitForDriver:
      case LC_ReaderStatusWaitForReaderUp:
	if (flags & LC_NOTIFY_FLAGS_READER_START) {
	  LC_CardServer_SendReaderNotification(cs, cl,
					       LC_NOTIFY_CODE_READER_START,
					       r,
					       "Reader started");
	}
	break;
      case LC_ReaderStatusWaitForReaderDown:
      case LC_ReaderStatusUp:
	if (flags & LC_NOTIFY_FLAGS_READER_UP) {
	  LC_CardServer_SendReaderNotification(cs, cl,
					       LC_NOTIFY_CODE_READER_UP,
					       r,
					       "Reader is up");
	}
	break;
      case LC_ReaderStatusDown:
	if (flags & LC_NOTIFY_FLAGS_READER_DOWN) {
	  LC_CardServer_SendReaderNotification(cs, cl,
					       LC_NOTIFY_CODE_READER_UP,
					       r,
					       "Reader is down");
	}
	break;
      case LC_ReaderStatusAborted:
	if (flags & LC_NOTIFY_FLAGS_READER_ERROR) {
	  LC_CardServer_SendReaderNotification(cs, cl,
					       LC_NOTIFY_CODE_READER_UP,
					       r,
					       "Reader aborted");
	}
	break;
      case LC_ReaderStatusDisabled:
	if (flags & LC_NOTIFY_FLAGS_READER_ERROR) {
	  LC_CardServer_SendReaderNotification(cs, cl,
					       LC_NOTIFY_CODE_READER_UP,
					       r,
					       "Reader disabled");
	}
	break;
      default:
        break;
      } /* switch */
      r=LC_Reader_List_Next(r);
    }
  }

  /* send response */
  if (GWEN_IPCManager_SendResponse(cs->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response to client");
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
    return -1;
  }
  GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
  DBG_NOTICE(0, "Response sent.");

  return 0;
}




/* __________________________________________________________________________
 * AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
 *                            Service Commands
 * YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY
 */



int LC_CardServer_HandleServiceOpen(LC_CARDSERVER *cs,
                                    GWEN_TYPE_UINT32 rid,
                                    GWEN_DB_NODE *dbReq){
  LC_REQUEST *rq;
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  LC_SERVICE *sv;
  const char *serviceName;
  GWEN_DB_NODE *dbServiceReq;
  GWEN_DB_NODE *dbT;
  char numbuf[32];

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

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

  DBG_NOTICE(0, "Client %08x: ServiceOpen [%s/%s]",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* get service name */
  serviceName=GWEN_DB_GetCharValue(dbReq, "body/serviceName", 0, 0);
  if (!serviceName) {
    DBG_ERROR(0, "No service name");
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "No service name");
    return -1;
  }

  /* search for service */
  sv=LC_Service_List_First(cs->services);
  while(sv) {
    const char *sn;

    sn=LC_Service_GetServiceName(sv);
    if (sn)
      if (strcasecmp(sn, serviceName)==0)
        break;
    sv=LC_Service_List_Next(sv);
  }
  if (!sv) {
    DBG_ERROR(0, "Service \"%s\" not found", serviceName);
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Unknown service name");
    return -1;
  }

  /* create outbound command */
  dbServiceReq=GWEN_DB_Group_new("ServiceOpen");

  snprintf(numbuf, sizeof(numbuf)-1, "%08x",
           LC_Service_GetServiceId(sv));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "serviceId", numbuf);
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", clientId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "clientId", numbuf);
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "appName", LC_Client_GetApplicationName(cl));
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "userName", LC_Client_GetUserName(cl));

  dbT=GWEN_DB_GetGroup(dbReq, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "command");
  if (dbT) {
    GWEN_DB_NODE *dbTarget;

    dbTarget=GWEN_DB_GetGroup(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_GROUPS,
                              "command");
    assert(dbTarget);
    GWEN_DB_AddGroupChildren(dbTarget, dbT);
  }

  /* store request */
  rq=LC_Request_new(cl,
                    GWEN_DB_Group_dup(dbReq),
                    dbServiceReq,
                    0); /* no card */
  LC_Request_SetInRequestId(rq, rid);
  LC_Service_AddRequest(sv, rq);

  LC_Service_IncActiveClientsCount(sv);
  LC_Client_AddService(cl, LC_Service_GetServiceId(sv));

  DBG_INFO(0, "Request enqueued");
  return 0;
}



int LC_CardServer_HandleServiceClose(LC_CARDSERVER *cs,
                                     GWEN_TYPE_UINT32 rid,
                                     GWEN_DB_NODE *dbReq){
  LC_REQUEST *rq;
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_TYPE_UINT32 serviceId;
  GWEN_TYPE_UINT32 contextId;
  LC_SERVICE *sv;
  const char *serviceName=0;
  GWEN_DB_NODE *dbServiceReq;
  GWEN_DB_NODE *dbT;
  char numbuf[32];

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

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

  DBG_NOTICE(0, "Client %08x: ServiceClose [%s/%s]",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* get service id */
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/serviceId", 0, "0"),
                "%x", &serviceId)) {
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Missing service id");
    return -1;
  }

  /* search for service */
  sv=LC_Service_List_First(cs->services);
  while(sv) {
    if (LC_Service_GetServiceId(sv)==serviceId)
      break;
    sv=LC_Service_List_Next(sv);
  }
  if (!sv) {
    DBG_ERROR(0, "Service \"%08x\" not found", serviceId);
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Unknown service id");
    return -1;
  }

  /* get context id */
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/contextId", 0, "0"),
                "%x", &contextId)) {
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Missing context id");
    return -1;
  }

  /* search for service */
  sv=LC_Service_List_First(cs->services);
  while(sv) {
    const char *sn;

    sn=LC_Service_GetServiceName(sv);
    if (sn)
      if (strcasecmp(sn, serviceName)==0)
        break;
    sv=LC_Service_List_Next(sv);
  }
  if (!sv) {
    DBG_ERROR(0, "Service \"%s\" not found", serviceName);
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Unknown service name");
    return -1;
  }

  /* create outbound command */
  dbServiceReq=GWEN_DB_Group_new("ServiceClose");

  snprintf(numbuf, sizeof(numbuf)-1, "%08x",
           LC_Service_GetServiceId(sv));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "serviceId", numbuf);
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", clientId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "clientId", numbuf);
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", contextId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "contextId", numbuf);
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "appName", LC_Client_GetApplicationName(cl));
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "userName", LC_Client_GetUserName(cl));

  dbT=GWEN_DB_GetGroup(dbReq, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "command");
  if (dbT) {
    GWEN_DB_NODE *dbTarget;

    dbTarget=GWEN_DB_GetGroup(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_GROUPS,
                              "command");
    assert(dbTarget);
    GWEN_DB_AddGroupChildren(dbTarget, dbT);
  }

  /* store request */
  rq=LC_Request_new(cl,
                    GWEN_DB_Group_dup(dbReq),
                    dbServiceReq,
                    0); /* no card */
  LC_Request_SetInRequestId(rq, rid);
  LC_Service_AddRequest(sv, rq);

  LC_Service_DecActiveClientsCount(sv);
  LC_Client_DelService(cl, LC_Service_GetServiceId(sv));

  DBG_INFO(0, "Request enqueued");
  return 0;
}



int LC_CardServer_HandleServiceCommand(LC_CARDSERVER *cs,
                                       GWEN_TYPE_UINT32 rid,
                                       GWEN_DB_NODE *dbReq){
  LC_REQUEST *rq;
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_TYPE_UINT32 contextId;
  GWEN_TYPE_UINT32 serviceId;
  LC_SERVICE *sv;
  GWEN_DB_NODE *dbServiceReq;
  GWEN_DB_NODE *dbT;
  char numbuf[32];

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

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

  DBG_NOTICE(0, "Client %08x: ServiceCommand [%s/%s]",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* get service id */
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/serviceId", 0, "0"),
                "%x", &serviceId)) {
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Missing service id");
    return -1;
  }

  /* search for service */
  sv=LC_Service_List_First(cs->services);
  while(sv) {
    if (LC_Service_GetServiceId(sv)==serviceId)
      break;
    sv=LC_Service_List_Next(sv);
  }
  if (!sv) {
    DBG_ERROR(0, "Service \"%08x\" not found", serviceId);
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Unknown service id");
    return -1;
  }

  /* get context id */
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/contextId", 0, "0"),
                "%x", &contextId)) {
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Missing context id");
    return -1;
  }

  /* create outbound command */
  dbServiceReq=GWEN_DB_Group_new("ServiceCommand");

  snprintf(numbuf, sizeof(numbuf)-1, "%08x",
           LC_Service_GetServiceId(sv));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "serviceId", numbuf);
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", clientId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "clientId", numbuf);
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", contextId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "contextId", numbuf);
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "appName", LC_Client_GetApplicationName(cl));
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "userName", LC_Client_GetUserName(cl));

  dbT=GWEN_DB_GetGroup(dbReq, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "command");
  if (dbT) {
    GWEN_DB_NODE *dbTarget;

    dbTarget=GWEN_DB_GetGroup(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_GROUPS,
                              "command");
    assert(dbTarget);
    GWEN_DB_AddGroupChildren(dbTarget, dbT);
  }

  /* store request */
  rq=LC_Request_new(cl,
                    GWEN_DB_Group_dup(dbReq),
                    dbServiceReq,
                    0); /* no card */
  LC_Request_SetInRequestId(rq, rid);
  LC_Service_AddRequest(sv, rq);

  DBG_INFO(0, "Request enqueued");
  return 0;
}



int LC_CardServer_HandleServiceNotification(LC_CARDSERVER *cs,
                                            GWEN_TYPE_UINT32 rid,
                                            GWEN_DB_NODE *dbReq){
  LC_CLIENT *cl;
  GWEN_TYPE_UINT32 serviceId;
  GWEN_TYPE_UINT32 clientId;
  GWEN_TYPE_UINT32 contextId;
  LC_SERVICE *sv;
  GWEN_DB_NODE *dbServiceReq;
  GWEN_DB_NODE *dbT;
  char numbuf[32];
  GWEN_TYPE_UINT32 ridOut;

  assert(dbReq);
  serviceId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(serviceId);

  /* search for service */
  sv=LC_Service_List_First(cs->services);
  while(sv) {
    if (LC_Service_GetServiceId(sv)==serviceId)
      break;
    sv=LC_Service_List_Next(sv);
  }
  if (!sv) {
    DBG_ERROR(0, "Service \"%08x\" not found", serviceId);
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Unknown service id");
    return -1;
  }

  /* get client id */
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/clientId", 0, "0"),
                "%x", &clientId)) {
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Missing client id");
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

  DBG_NOTICE(0, "Client %08x: ServiceNotification for %s/%s",
             clientId,
             LC_Client_GetApplicationName(cl),
             LC_Client_GetUserName(cl));

  /* get context id */
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/contextId", 0, "0"),
                "%x", &contextId)) {
    /* Send SegResult */
    LC_CardServer_SendErrorResponse(cs, rid,
                                    LC_ERROR_INVALID,
                                    "Missing context id");
    return -1;
  }

  /* create outbound command */
  dbServiceReq=GWEN_DB_Group_new("ServiceNotification");

  /* send service id */
  snprintf(numbuf, sizeof(numbuf)-1, "%08x",
           LC_Service_GetServiceId(sv));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "serviceId", numbuf);
  /* send client id */
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", clientId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "clientId", numbuf);
  /* send context id */
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", contextId);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "contextId", numbuf);
  /* send some info about the sending service */
  GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "serviceName",
                       LC_Service_GetServiceName(sv));
  /* send service flags */
  if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_AUTOLOAD)
    GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_DEFAULT,
                         "serviceFlags", "autoload");
  if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_SILENT)
    GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_DEFAULT,
                         "serviceFlags", "silent");
  if (LC_Service_GetFlags(sv) & LC_SERVICE_FLAGS_CLIENT)
    GWEN_DB_SetCharValue(dbServiceReq, GWEN_DB_FLAGS_DEFAULT,
                         "serviceFlags", "client");

  dbT=GWEN_DB_GetGroup(dbReq, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "command");
  if (dbT) {
    GWEN_DB_NODE *dbTarget;

    dbTarget=GWEN_DB_GetGroup(dbServiceReq, GWEN_DB_FLAGS_OVERWRITE_GROUPS,
                              "command");
    assert(dbTarget);
    GWEN_DB_AddGroupChildren(dbTarget, dbT);
  }

  ridOut=GWEN_IPCManager_SendRequest(cs->ipcManager,
                                     LC_Client_GetClientId(cl),
                                     dbServiceReq);
  if (ridOut==0) {
    DBG_ERROR(0, "Could not send \"ServiceNotification\" to client");
  }
  else {
    /* remove request, we don't expect an answer */
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, ridOut, 1);
  }

  /* remove incoming request */
  GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 0);
  DBG_INFO(0, "Request forwarded");
  return 0;
}





int LC_CardServer_HandleNextCommand(LC_CARDSERVER *cs) {
  const char *name;
  int rv;
  GWEN_TYPE_UINT32 ridNext;
  GWEN_DB_NODE *dbReq;

  assert(cs);

  ridNext=GWEN_IPCManager_GetNextInRequest(cs->ipcManager, 0);
  if (!ridNext) {
    DBG_VERBOUS(0, "No incoming request");
    return 1;
  }
  dbReq=GWEN_IPCManager_GetInRequestData(cs->ipcManager, ridNext);
  assert(dbReq);
  name=GWEN_DB_GetCharValue(dbReq, "command/vars/cmd", 0, 0);
  if (!name) {
    DBG_ERROR(0, "Bad IPC command (no command name), discarding");
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, ridNext, 0);
    rv=-1;
  }
  else {
    DBG_INFO(0, "Command \"%s\"", name);
    if (strcasecmp(name, "DriverReady")==0) {
      rv=LC_CardServer_HandleDriverReady(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "ServiceReady")==0) {
      rv=LC_CardServer_HandleServiceReady(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "OfferService")==0) {
      rv=LC_CardServer_HandleOfferService(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "ServiceNotification")==0) {
      rv=LC_CardServer_HandleServiceNotification(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "ClientReady")==0) {
      rv=LC_CardServer_HandleClientReady(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "CardInserted")==0) {
      rv=LC_CardServer_HandleCardInserted(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "ReaderError")==0) {
      rv=LC_CardServer_HandleReaderError(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "CardRemoved")==0) {
      rv=LC_CardServer_HandleCardRemoved(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "StartWait")==0) {
      rv=LC_CardServer_HandleStartWait(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "StopWait")==0) {
      rv=LC_CardServer_HandleStopWait(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "TakeCard")==0) {
      rv=LC_CardServer_HandleTakeCard(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "ReleaseCard")==0) {
      rv=LC_CardServer_HandleReleaseCard(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "CommandCard")==0) {
      rv=LC_CardServer_HandleCommandCard(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "ExecCommand")==0) {
      rv=LC_CardServer_HandleExecCommand(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "SelectCard")==0) {
      rv=LC_CardServer_HandleSelectCard(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "SetNotify")==0) {
      rv=LC_CardServer_HandleSetNotify(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "GetDriverVar")==0) {
      rv=LC_CardServer_HandleGetDriverVar(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "ServiceOpen")==0) {
      rv=LC_CardServer_HandleServiceOpen(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "ServiceClose")==0) {
      rv=LC_CardServer_HandleServiceClose(cs, ridNext, dbReq);
    }
    else if (strcasecmp(name, "ServiceCommand")==0) {
      rv=LC_CardServer_HandleServiceCommand(cs, ridNext, dbReq);
    }

    /* Insert more handlers here */
    else {
      DBG_ERROR(0, "Unknown command \"%s\", discarding", name);
      rv=-1;
    }
  }

  if (rv<0) {
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, ridNext, 0);
  }

  return rv;
}



int LC_CardServer_Work(LC_CARDSERVER *cs) {
  LC_DRIVER *d;
  LC_SERVICE *as;
  LC_READER *r;
  int rv;
  int somethingDone;

  somethingDone=0;

  rv=GWEN_IPCManager_Work(cs->ipcManager, 10);
  if (rv==-1) {
    DBG_ERROR(0, "Error in IPC manager");
    return -1;
  }
  else if (rv==0) {
    somethingDone++;
  }

  DBG_DEBUG(0, "Handling commands");
  rv=LC_CardServer_HandleNextCommand(cs);
  if (rv==0) {
    somethingDone++;
  }

  DBG_DEBUG(0, "Checking clients");
  rv=LC_CardServer_CheckClients(cs);
  if (rv==0) {
    somethingDone++;
  }

  DBG_DEBUG(0, "Checking drivers");
  d=LC_Driver_List_First(cs->drivers);
  while(d) {
    rv=LC_CardServer_CheckDriver(cs, d);
    if (rv==0) {
      somethingDone++;
    }
    d=LC_Driver_List_Next(d);
  } /* while */

  DBG_DEBUG(0, "Checking services");
  as=LC_Service_List_First(cs->services);
  while(as) {
    rv=LC_CardServer_CheckService(cs, as);
    if (rv==0) {
      somethingDone++;
    }
    as=LC_Service_List_Next(as);
  } /* while */

  if (!cs->disableAutoconf) {
    DBG_DEBUG(0, "Scanning USB");
    if (LC_CardServer_ScanUSB(cs)) {
      DBG_INFO(0, "here");
    }
  }

  DBG_DEBUG(0, "Checking readers");
  r=LC_Reader_List_First(cs->readers);
  while(r) {
    LC_READER *rnext;

    rnext=LC_Reader_List_Next(r);
    rv=LC_CardServer_CheckReader(cs, r);
    if (rv==0) {
      somethingDone++;
    }
    r=rnext;
  } /* while */

  DBG_DEBUG(0, "Checking cards");
  rv=LC_CardServer_CheckCards(cs);
  if (rv==0) {
    somethingDone++;
  }

  DBG_DEBUG(0, "Checking requests");
  rv=LC_CardServer_CheckRequests(cs);
  if (rv==0) {
    somethingDone++;
  }

  rv=GWEN_IPCManager_Work(cs->ipcManager, 10);
  if (rv==-1) {
    DBG_ERROR(0, "Error in IPC manager");
    return -1;
  }
  else if (rv==0) {
    somethingDone++;
  }

  return somethingDone?0:1;
}



GWEN_TYPE_UINT32 LC_CardServer_GetIpcId(const LC_CARDSERVER *cs){
  assert(cs);
  return cs->ipcId;
}



void LC_CardServer_SetIpcId(LC_CARDSERVER *cs, GWEN_TYPE_UINT32 id){
  assert(cs);
  cs->ipcId=id;
}



GWEN_TYPE_UINT32 LC_CardServer_SendResetCard(LC_CARDSERVER *cs,
                                             const LC_CARD *card){
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  LC_READER *r;
  LC_DRIVER *d;
  unsigned int slot;
  unsigned int cardNum;

  DBG_NOTICE(0, "Resetting card \"%08x\"", LC_Card_GetCardId(card));
  r=LC_Card_GetReader(card);
  assert(r);
  d=LC_Reader_GetDriver(r);
  assert(d);
  slot=LC_Card_GetSlot(card);
  cardNum=LC_Card_GetReadersCardId(card);

  dbReq=GWEN_DB_Group_new("ResetCard");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LC_Reader_GetReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "slotnum", slot);
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cardnum", cardNum);

  return GWEN_IPCManager_SendRequest(cs->ipcManager,
                                     LC_Driver_GetIpcId(d),
                                     dbReq);
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



GWEN_TYPE_UINT32 LC_CardServer_SendStopService(LC_CARDSERVER *cs,
                                               const LC_SERVICE *as) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;

  assert(as);
  dbReq=GWEN_DB_Group_new("StopService");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LC_Service_GetServiceId(as));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "serviceId", numbuf);

  return GWEN_IPCManager_SendRequest(cs->ipcManager,
                                     LC_Service_GetIpcId(as),
                                     dbReq);
}



LC_CARD *LC_CardServer_FindActiveCard(const LC_CARDSERVER *cs,
                                      GWEN_TYPE_UINT32 cid){
  LC_CARD *card;

  card=LC_Card_List_First(cs->activeCards);
  while(card) {
    if (LC_Card_GetCardId(card)==cid)
      break;
    card=LC_Card_List_Next(card);
  } /* while */

  return 0;
}



void LC_CardServer__SampleDirs(const char *dataDir, GWEN_STRINGLIST *sl) {
  GWEN_BUFFER *buf;
  GWEN_DIRECTORYDATA *d;
  unsigned int dpos;

  buf=GWEN_Buffer_new(0, 256, 0, 1);

  /* always append data dir */
  GWEN_StringList_AppendString(sl, dataDir, 0, 1);

  d=GWEN_Directory_new();
  GWEN_Buffer_AppendString(buf, dataDir);
  GWEN_Buffer_AppendByte(buf, '/');
  GWEN_Buffer_AppendString(buf, "drivers");
  dpos=GWEN_Buffer_GetPos(buf);
  if (!GWEN_Directory_Open(d, GWEN_Buffer_GetStart(buf))) {
    char buffer[256];

    while (!GWEN_Directory_Read(d, buffer, sizeof(buffer))){
      struct stat st;

      GWEN_Buffer_Crop(buf, 0, dpos);
      GWEN_Buffer_SetPos(buf, dpos);
      GWEN_Buffer_AppendByte(buf, '/');
      GWEN_Buffer_AppendString(buf, buffer);
      if (stat(GWEN_Buffer_GetStart(buf), &st)) {
        DBG_ERROR(0, "stat(%s): %s",
                  GWEN_Buffer_GetStart(buf),
                  strerror(errno));
      }
      else {
        if (S_ISDIR(st.st_mode)) {
          if (strcasecmp(buffer, "..")!=0 &&
              strcasecmp(buffer, ".")!=0) {
            DBG_DEBUG(0, "Adding driver dir \"%s\"",
                      GWEN_Buffer_GetStart(buf));
            GWEN_StringList_AppendString(sl,
                                         GWEN_Buffer_GetStart(buf),
                                         0, 1);
          } /* if real folder name */
        } /* if it is not a folder */
      } /* if stat succeeded */
    } /* while */
  } /* if open succeeded */
  else {
    DBG_ERROR(0, "Could not open folder %s", GWEN_Buffer_GetStart(buf));
  }
  GWEN_Directory_Close(d);
  GWEN_Directory_free(d);
  GWEN_Buffer_free(buf);
}



void LC_CardServer_SampleDirs(LC_CARDSERVER *cs,
                              GWEN_STRINGLIST *sl) {
  return LC_CardServer__SampleDirs(cs->dataDir, sl);
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



int LC_CardServer_StartService(LC_CARDSERVER *cs, LC_SERVICE *as) {
  LC_SERVICE_STATUS st;
  GWEN_PROCESS *p;
  GWEN_BUFFER *pbuf;
  GWEN_BUFFER *abuf;
  const char *s;
  char numbuf[32];
  int rv;
  GWEN_PROCESS_STATE pst;

  assert(cs);
  assert(as);

  DBG_INFO(0, "Starting service \"%s\"", LC_Service_GetServiceName(as));
  st=LC_Service_GetStatus(as);
  if (st!=LC_ServiceStatusDown) {
    DBG_ERROR(0, "Bad service status (%d)", st);
    return -1;
  }

  abuf=GWEN_Buffer_new(0, 128, 0, 1);

  s=LC_Service_GetServiceDataDir(as);
  if (s) {
    GWEN_Buffer_AppendString(abuf, "-d ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  s=LC_Service_GetLogFile(as);
  if (s) {
    GWEN_Buffer_AppendString(abuf, " --logtype file");
    GWEN_Buffer_AppendString(abuf, " --logfile ");
    GWEN_Buffer_AppendString(abuf, s);
  }

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

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x", LC_Service_GetServiceId(as));
  assert(rv>0 && rv<sizeof(numbuf));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_Buffer_AppendString(abuf, " -i ");
  GWEN_Buffer_AppendString(abuf, numbuf);

  s=LC_Service_GetServiceName(as);
  if (!s) {
    DBG_ERROR(0, "No service type");
    LC_Service_SetStatus(as, LC_ServiceStatusDisabled);
    GWEN_Buffer_free(abuf);
    return -1;
  }

  pbuf=GWEN_Buffer_new(0, 128, 0, 1);
  GWEN_Buffer_AppendString(pbuf, LC_SERVICE_PATH);
  GWEN_Buffer_AppendByte(pbuf, '/');
  while(*s) {
    GWEN_Buffer_AppendByte(pbuf, tolower(*s));
    s++;
  } /* while */

  p=GWEN_Process_new();
  DBG_NOTICE(0, "Starting service process for service \"%s\" (%s)",
             LC_Service_GetServiceName(as), GWEN_Buffer_GetStart(pbuf));
  DBG_NOTICE(0, "Arguments are: \"%s\"", GWEN_Buffer_GetStart(abuf));

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
  GWEN_Buffer_free(pbuf);
  GWEN_Buffer_free(abuf);
  LC_Service_SetProcess(as, p);
  LC_Service_SetStatus(as, LC_ServiceStatusStarted);
  return 0;
}



int LC_CardServer_StopService(LC_CARDSERVER *cs, LC_SERVICE *as) {
  GWEN_TYPE_UINT32 rid;

  assert(as);
  rid=LC_Service_GetCurrentRequestId(as);
  if (rid) {
    GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 1);
    LC_Service_SetCurrentRequestId(as, 0);
  }
  rid=LC_CardServer_SendStopService(cs, as);
  if (!rid) {
    DBG_ERROR(0, "Could not send StopService command for service \"%s\"",
              LC_Service_GetServiceName(as));
    LC_Service_SetStatus(as, LC_ServiceStatusAborted);
    return -1;
  }
  DBG_DEBUG(0, "Sent StopService command for service \"%s\"",
            LC_Service_GetServiceName(as));
  LC_Service_SetStatus(as, LC_ServiceStatusStopping);
  return 0;
}



int LC_CardServer_CheckService(LC_CARDSERVER *cs, LC_SERVICE *as) {
  int done;

  assert(cs);
  assert(as);

  done=0;
  if (LC_Service_GetStatus(as)==LC_ServiceStatusAborted) {
    if (cs->driverRestartTime &&
        difftime(time(0), LC_Service_GetLastStatusChangeTime(as))>=
        cs->driverRestartTime) {
      LC_Service_SetStatus(as, LC_ServiceStatusDown);
    }
  }

  if (LC_Service_GetStatus(as)==LC_ServiceStatusStarted) {
    /* service started, check timeout */
    if (cs->driverStartTimeout &&
        difftime(time(0), LC_Service_GetLastStatusChangeTime(as))>=
        cs->driverStartTimeout) {
      GWEN_PROCESS *p;
      GWEN_PROCESS_STATE pst;

      DBG_WARN(0, "Service \"%s\" timed out", LC_Service_GetServiceName(as));
      p=LC_Service_GetProcess(as);
      assert(p);

      pst=GWEN_Process_CheckState(p);
      if (pst==GWEN_ProcessStateRunning) {
        if (LC_Service_GetFlags(as) & LC_SERVICE_FLAGS_SILENT) {
          LC_Service_SetStatus(as, LC_ServiceStatusSilentRunning);
          DBG_NOTICE(0, "Silent service \"%s\" is running",
                     LC_Service_GetServiceName(as));
          return 1;
        }
        else {
          DBG_WARN(0,
                   "Service is running but did not signal readyness, "
                   "killing it");
          if (GWEN_Process_Terminate(p)) {
            DBG_ERROR(0, "Could not kill process");
          }
          LC_Service_SetProcess(as, 0);
          LC_Service_SetStatus(as, LC_ServiceStatusAborted);
          return -1;
        }
      }
      else if (pst==GWEN_ProcessStateExited) {
        DBG_WARN(0, "Service terminated normally");
        LC_Service_SetProcess(as, 0);
        LC_Service_SetStatus(as, LC_ServiceStatusDown);
        done++;
      }
      else if (pst==GWEN_ProcessStateAborted) {
        DBG_WARN(0, "Service terminated abnormally");
        LC_Service_SetProcess(as, 0);
        LC_Service_SetStatus(as, LC_ServiceStatusAborted);
        return -1;
      }
      else if (pst==GWEN_ProcessStateStopped) {
        DBG_WARN(0, "Service has been stopped, killing it");
        if (GWEN_Process_Terminate(p)) {
          DBG_ERROR(0, "Could not kill process");
        }
        LC_Service_SetProcess(as, 0);
        LC_Service_SetStatus(as, LC_ServiceStatusAborted);
        return -1;
      }
      else {
        DBG_ERROR(0, "Unknown process status %d, killing", pst);
        if (GWEN_Process_Terminate(p)) {
          DBG_ERROR(0, "Could not kill process");
        }
        LC_Service_SetProcess(as, 0);
        LC_Service_SetStatus(as, LC_ServiceStatusAborted);
        return -1;
      }
    }
    else {
      /* otherwise give the process a little bit time ... */
      DBG_DEBUG(0, "still waiting for service start");
      return 1;
    }
  }

  if (LC_Service_GetStatus(as)==LC_ServiceStatusStopping) {
    GWEN_PROCESS *p;
    GWEN_PROCESS_STATE pst;

    p=LC_Service_GetProcess(as);
    assert(p);

    pst=GWEN_Process_CheckState(p);
    if (pst==GWEN_ProcessStateRunning) {
      if (cs->serviceStopTimeout &&
          difftime(time(0), LC_Service_GetLastStatusChangeTime(as))>=
          cs->serviceStopTimeout) {
        DBG_WARN(0, "Service is still running, killing it");
        if (GWEN_Process_Terminate(p)) {
          DBG_ERROR(0, "Could not kill process");
        }
        LC_Service_SetProcess(as, 0);
        LC_Service_SetStatus(as, LC_ServiceStatusAborted);
        return -1;
      }
      else {
        /* otherwise give the process a little bit time ... */
        DBG_DEBUG(0, "still waiting for service to go down");
        return 1;
      }
    }
    else if (pst==GWEN_ProcessStateExited) {
      DBG_WARN(0, "Service terminated normally");
      LC_Service_SetProcess(as, 0);
      LC_Service_SetStatus(as, LC_ServiceStatusDown);
      done++;
      /* no return here, we want to continue below */
    }
    else if (pst==GWEN_ProcessStateAborted) {
      DBG_WARN(0, "Service terminated abnormally");
      LC_Service_SetProcess(as, 0);
      LC_Service_SetStatus(as, LC_ServiceStatusAborted);
      return 0;
    }
    else if (pst==GWEN_ProcessStateStopped) {
      DBG_WARN(0, "Service has been stopped, killing it");
      if (GWEN_Process_Terminate(p)) {
        DBG_ERROR(0, "Could not kill process");
      }
      LC_Service_SetProcess(as, 0);
      LC_Service_SetStatus(as, LC_ServiceStatusAborted);
      return 0;
    }
    else {
      DBG_ERROR(0, "Unknown process status %d, killing", pst);
      if (GWEN_Process_Terminate(p)) {
        DBG_ERROR(0, "Could not kill process");
      }
      LC_Service_SetProcess(as, 0);
      LC_Service_SetStatus(as, LC_ServiceStatusAborted);
      return 0;
    }
  } /* if stopping */

  if (LC_Service_GetStatus(as)==LC_ServiceStatusDown) {
    if (LC_Service_GetFlags(as) & LC_SERVICE_FLAGS_AUTOLOAD) {
      DBG_NOTICE(0, "Loading service \"%s\"", LC_Service_GetServiceName(as));
      if (LC_CardServer_StartService(cs, as)) {
        DBG_ERROR(0, "Could not start service \"%08x\"",
                  LC_Service_GetServiceId(as));
        LC_Service_SetStatus(as, LC_ServiceStatusAborted);
      }
      else {
        DBG_NOTICE(0, "Started service \"%08x\"",
                   LC_Service_GetServiceId(as));
      }
    }
  }

  if (LC_Service_GetStatus(as)==LC_ServiceStatusUp ||
      LC_Service_GetStatus(as)==LC_ServiceStatusSilentRunning) {
    GWEN_TYPE_UINT32 rid;

    if (!(LC_Service_GetFlags(as) & LC_SERVICE_FLAGS_CLIENT)) {
      GWEN_PROCESS *p;
      GWEN_PROCESS_STATE pst;

      /* check whether the service really is still up and running */
      p=LC_Service_GetProcess(as);
      assert(p);
      pst=GWEN_Process_CheckState(p);
      if (pst!=GWEN_ProcessStateRunning) {
        DBG_ERROR(0, "Service is not running anymore");
        GWEN_Process_Terminate(p);
        LC_Service_SetProcess(as, 0);
        LC_Service_SetStatus(as, LC_ServiceStatusAborted);
        /* TODO: send error responses for all listed requests */

        return -1;
      }
    }

    /* check for requests to send */
    DBG_DEBUG(0, "Service still running");
    rid=LC_Service_GetCurrentRequestId(as);
    if (rid) {
      LC_REQUEST *rq;
      int rv;

      /* there is a current request, check whether is is answered */
      rq=LC_CardServer_GetRequest(cs, rid);
      assert(rq);
      rv=LC_CardServer_CheckRequest(cs, rq);
      if (rv==1) {
        if (cs->serviceCommandTimeout &&
            difftime(time(0), LC_Service_GetCommandTime(as))>=
            cs->serviceCommandTimeout) {
          /* service timed out */
          DBG_WARN(0, "Service \"%s\" timed out",
                   LC_Service_GetServiceName(as));
          LC_Service_SetStatus(as, LC_ServiceStatusAborted);
          GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 1);
          LC_Service_SetCurrentRequestId(as, 0);
          return -1;
        } /* if timeout */
        DBG_DEBUG(0, "Still some time left");
        return 1;
      }
      else {
        DBG_DEBUG(0, "Removing request");
        LC_CardServer_RemoveRequest(cs, rq);
        LC_Service_SetCurrentRequestId(as, 0);
        rid=0;
        done++;
      }
    } /* if rid */

    if (rid==0) {
      LC_REQUEST *rq;

      /* no current command, get the next one from queue */
      rq=LC_Service_GetNextRequest(as);
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
                                        LC_Service_GetIpcId(as),
                                        GWEN_DB_Group_dup(dbReq));
        if (!rid) {
          DBG_ERROR(0, "Could not send command \"%s\" for service \"%s\"",
                    cmdName,
                    LC_Service_GetServiceName(as));
          LC_Service_SetStatus(as, LC_ServiceStatusAborted);
          return -1;
        }
        DBG_DEBUG(0, "Sent request \"%s\" for service \"%s\" (%08x)",
                  cmdName,
                  LC_Service_GetServiceName(as),
                  rid);
        LC_Request_SetOutRequestId(rq, rid);
        LC_Service_SetCurrentRequestId(as, LC_Request_GetRequestId(rq));
        done++;
      } /* if there was a next request */
    } /* if there was no current command */

    if (!(LC_Service_GetFlags(as) & LC_SERVICE_FLAGS_CLIENT)) {
      if (LC_Service_GetActiveClientsCount(as)==0 &&
          cs->serviceIdleTimeout){
        time_t t;

        /* check for idle timeout */
        t=LC_Service_GetIdleSince(as);
        assert(t);

        if (cs->serviceIdleTimeout &&
            difftime(time(0), t)>cs->serviceIdleTimeout) {
          if (!LC_CardServer_StopService(cs, as)) {
            DBG_INFO(0, "Could not stop service \"%s\"",
                     LC_Service_GetServiceName(as));
            return -1;
          }
          done++;
        } /* if timeout */
        else {
          /* otherwise service is not idle */
          return 1;
        }
      }
    } /* if not a client-service */
  }

  return done?0:1;
}



int LC_CardServer_FindFile(GWEN_STRINGLIST *slDirs,
			   GWEN_STRINGLIST *slNames,
			   GWEN_BUFFER *nbuf) {
  GWEN_STRINGLISTENTRY *eDirs;

  eDirs=GWEN_StringList_FirstEntry(slDirs);
  while(eDirs) {
    GWEN_TYPE_UINT32 pos;
    GWEN_STRINGLISTENTRY *eNames;

    GWEN_Buffer_Reset(nbuf);
    GWEN_Buffer_AppendString(nbuf, GWEN_StringListEntry_Data(eDirs));
    GWEN_Buffer_AppendByte(nbuf, '/');
    pos=GWEN_Buffer_GetPos(nbuf);

    eNames=GWEN_StringList_FirstEntry(slNames);

    while(eNames) {
      GWEN_DIRECTORYDATA *dDir;

      dDir=GWEN_Directory_new();
      if (!GWEN_Directory_Open(dDir, GWEN_StringListEntry_Data(eDirs))) {
        char nameBuf[256];

        /* search for name in this folder */
        while(!GWEN_Directory_Read(dDir, nameBuf, sizeof(nameBuf))) {
          if (strcmp(nameBuf, ".")!=0 &&
              strcmp(nameBuf, "..")!=0) {
            if (-1!=GWEN_Text_ComparePattern(nameBuf,
                                             GWEN_StringListEntry_Data(eNames),
                                             0)) {
              struct stat st;

              /* found name, add it to the buffer */
              GWEN_Buffer_Crop(nbuf, 0, pos);
              GWEN_Buffer_SetPos(nbuf, pos);
              GWEN_Buffer_AppendString(nbuf, nameBuf);
              if (stat(GWEN_Buffer_GetStart(nbuf), &st)) {
                /* error */
                DBG_WARN(0, "stat(%s): %s",
                         GWEN_Buffer_GetStart(nbuf),
                         strerror(errno));
              }
              else {
                /* check for regular file */
                if (S_ISREG(st.st_mode)) {
                  GWEN_Directory_Close(dDir);
                  GWEN_Directory_free(dDir);
                  DBG_INFO(0, "File found: %s", GWEN_Buffer_GetStart(nbuf));
                  return 0;
                }
                else {
                  DBG_INFO(0, "Entry \"%s\" is not a regular file",
                           GWEN_Buffer_GetStart(nbuf));
                }
              }
            } /* if name pattern matches */
          } /* if not a special entry */
        } /* while still entries */
        GWEN_Directory_Close(dDir);
      }
      GWEN_Directory_free(dDir);

      eNames=GWEN_StringListEntry_Next(eNames);
    } /* while eNames */

    eDirs=GWEN_StringListEntry_Next(eDirs);
  } /* while eDirs */

  DBG_DEBUG(0, "File not found in search paths");
  return -1;
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





int LC_CardServer_USBDevice_Up(LC_CARDSERVER *cs, LC_USBDEVICE *ud) {
  GWEN_DB_NODE *dbDriver;
  GWEN_DB_NODE *dbReader;

  dbDriver=GWEN_DB_GetFirstGroup(cs->dbDrivers);
  while(dbDriver) {
    dbReader=0;
    if (strcasecmp(GWEN_DB_GroupName(dbDriver), "driver")==0) {
      dbReader=GWEN_DB_GetFirstGroup(dbDriver);
      while(dbReader) {
	if (strcasecmp(GWEN_DB_GroupName(dbReader), "reader")==0) {
	  if (strcasecmp(GWEN_DB_GetCharValue(dbReader,
					      "comType", 0, "serial"),
			 "USB")==0) {
	    if ((GWEN_DB_GetIntValue(dbReader, "vendorId", 0, 0)==
		 (int)LC_USBDevice_GetVendorId(ud)) &&
		(GWEN_DB_GetIntValue(dbReader, "productId", 0, 0)==
		 (int)LC_USBDevice_GetProductId(ud))) {
	      /* reader found */
	      break;
	    }
	  }
	}
	dbReader=GWEN_DB_GetNextGroup(dbReader);
      } /* while */
    }
    if (dbReader)
      break;
    dbDriver=GWEN_DB_GetNextGroup(dbDriver);
  } /* while */

  if (dbDriver && dbReader) {
    LC_DRIVER *d;
    const char *dname;
    const char *rtype;
    GWEN_BUFFER *nbuf;
    LC_READER *r;
    char numbuf[32];
    int port;
    int defPort;

    /* found reader and driver */
    dname=GWEN_DB_GetCharValue(dbDriver, "driverName", 0, 0);
    assert(dname);

    rtype=GWEN_DB_GetCharValue(dbReader, "readerType", 0, 0);
    assert(rtype);

    d=LC_Driver_List_First(cs->drivers);
    while(d) {
      if (strcasecmp(LC_Driver_GetDriverName(d), dname)==0) {
	if (LC_Driver_GetMaxReaders(d)>LC_Driver_GetActiveReadersCount(d))
	  break;
      }
      d=LC_Driver_List_Next(d);
    } /* while */

    if (!d) {
      GWEN_BUFFER *lbuf;

      /* no driver exists, create one */
      d=LC_Driver_FromDb(dbDriver);
      assert(d);
      lbuf=GWEN_Buffer_new(0, 256, 0, 1);
      LC_CardServer_ReplaceVar(LC_DEFAULT_LOGDIR
                               "/drivers/@driver@"
                               "/@reader@"
                               ".log",
                               "driver", dname, lbuf);
      DBG_DEBUG(0, "Logfile is \"%s\"",
                GWEN_Buffer_GetStart(lbuf));
      LC_Driver_SetLogFile(d, GWEN_Buffer_GetStart(lbuf));
      GWEN_Buffer_free(lbuf);
      LC_Driver_List_Add(d, cs->drivers);
    }

    /* create reader */
    r=LC_Reader_FromDb(d, dbReader);
    assert(r);
    LC_Reader_List_Add(r, cs->readers);

    nbuf=GWEN_Buffer_new(0, 256, 0, 1);
    snprintf(numbuf, sizeof(numbuf), "%d", ++(cs->lastAutoReader));
    GWEN_Buffer_AppendString(nbuf, "auto");
    GWEN_Buffer_AppendString(nbuf, numbuf);
    GWEN_Buffer_AppendByte(nbuf, '-');
    GWEN_Buffer_AppendString(nbuf, rtype);
    LC_Reader_SetReaderName(r, GWEN_Buffer_GetStart(nbuf));
    DBG_NOTICE(0, "AUTOCONFIG: Created new reader \"%s\" (USB: %04x/%04x)",
               GWEN_Buffer_GetStart(nbuf),
               LC_USBDevice_GetVendorId(ud),
               LC_USBDevice_GetProductId(ud));
    GWEN_Buffer_free(nbuf);

    defPort=GWEN_DB_GetIntValue(dbReader, "ports/default", 0,
				LC_USBDevice_GetDeviceId(ud));
    snprintf(numbuf, sizeof(numbuf), "ports/USB%d",
	     LC_USBDevice_GetDeviceId(ud));

    if (!GWEN_DB_VariableExists(dbReader, numbuf) &&
        !GWEN_DB_VariableExists(dbReader, "ports/default")) {
      LC_USBDEVICE_LIST *usbDevices;
      LC_USBDEVICE *tud;
      int count;
      int autoPortOffset;

      count=0;
      autoPortOffset=LC_Driver_GetAutoPortOffset(d);
      if (autoPortOffset==-1)
        autoPortOffset=0;

      /* neither port nor default port available, just count the devices */
      usbDevices=LC_USBMonitor_GetCurrentDevices(cs->usbMonitor);
      tud=LC_USBDevice_List_First(usbDevices);
      assert(tud);
      while(tud) {
        GWEN_DB_NODE *dbT;

        dbT=GWEN_DB_GetFirstGroup(dbDriver);
        while(dbT) {
          if (strcasecmp(GWEN_DB_GroupName(dbT), "reader")==0) {
            if (strcasecmp(GWEN_DB_GetCharValue(dbT,
                                                "comType", 0, "serial"),
                           "USB")==0) {
              if ((GWEN_DB_GetIntValue(dbT, "vendorId", 0, 0)==
                   (int)LC_USBDevice_GetVendorId(tud)) &&
                  (GWEN_DB_GetIntValue(dbT, "productId", 0, 0)==
                   (int)LC_USBDevice_GetProductId(tud))) {
                /* reader found */
                break;
              }
            }
          }
          dbT=GWEN_DB_GetNextGroup(dbT);
        } /* while */
        if (dbT) {
          count++;
        }
        if (LC_USBDevice_GetDevicePos(tud)==
            LC_USBDevice_GetDevicePos(ud))
          break;

        tud=LC_USBDevice_List_Next(tud);
      } /* while tud */
      assert(tud);
      assert(count);

      port=count-1+autoPortOffset;
      DBG_NOTICE(0, "Assigning port value %d to reader \"%s\"",
                 port, LC_Reader_GetReaderName(r));
    }
    else {
      port=GWEN_DB_GetIntValue(dbReader, numbuf, 0, defPort);
    }
    LC_Reader_SetPort(r, port);
    LC_Reader_SetBusId(r, LC_USBDevice_GetBusId(ud));
    LC_Reader_SetDeviceId(r, LC_USBDevice_GetDeviceId(ud));
    LC_Reader_SetIsAvailable(r, 1);

    if (LC_CardServer_Reader_Up(cs, r)) {
      DBG_INFO(0, "here");
    }
  }
  else {
    DBG_INFO(0, "Device %04x/%04x is not a known non-ttyUSB reader",
	     LC_USBDevice_GetVendorId(ud),
	     LC_USBDevice_GetProductId(ud));
  }

  return 0;
}



int LC_CardServer_USBDevice_Down(LC_CARDSERVER *cs, LC_USBDEVICE *ud) {
  LC_READER *r;

  assert(cs);
  assert(ud);

  r=LC_Reader_List_First(cs->readers);
  while(r) {
    if (strcasecmp(LC_Reader_GetComType(r), "USB")==0) {
      if ((LC_Reader_GetBusId(r)==
	   (int)LC_USBDevice_GetBusId(ud)) &&
	  (LC_Reader_GetDeviceId(r)==
	   (int)LC_USBDevice_GetDeviceId(ud))) {
	/* reader found */
	break;
      }
    }
    r=LC_Reader_List_Next(r);
  } /* while */

  if (!r) {
    DBG_DEBUG(0, "Unknown device %04x/%04x",
	      LC_USBDevice_GetBusId(ud),
	      LC_USBDevice_GetDeviceId(ud));
    return 0;
  }

  DBG_NOTICE(0, "Reader \"%s\" unplugged",
             LC_Reader_GetReaderName(r));
  if (LC_CardServer_StopReader(cs, r)) {
    DBG_INFO(0, "here");
  }
  LC_Reader_SetIsAvailable(r, 0);
  return 0;
}



int LC_CardServer_USBTTYDevice_Up(LC_CARDSERVER *cs, LC_USBTTYDEVICE *ud) {
  GWEN_DB_NODE *dbDriver;
  GWEN_DB_NODE *dbReader;

  dbDriver=GWEN_DB_GetFirstGroup(cs->dbDrivers);
  while(dbDriver) {
    dbReader=0;
    if (strcasecmp(GWEN_DB_GroupName(dbDriver), "driver")==0) {
      dbReader=GWEN_DB_GetFirstGroup(dbDriver);
      while(dbReader) {
	if (strcasecmp(GWEN_DB_GroupName(dbReader), "reader")==0) {
	  if (strcasecmp(GWEN_DB_GetCharValue(dbReader,
					      "comType", 0, "serial"),
			 "USBSerial")==0) {
	    if ((GWEN_DB_GetIntValue(dbReader, "vendorId", 0, 0)==
		 (int)LC_USBTTYDevice_GetVendorId(ud)) &&
		(GWEN_DB_GetIntValue(dbReader, "productId", 0, 0)==
		 (int)LC_USBTTYDevice_GetProductId(ud))) {
	      /* reader found */
	      break;
	    }
	  }
	}
	dbReader=GWEN_DB_GetNextGroup(dbReader);
      } /* while */
    }
    if (dbReader)
      break;
    dbDriver=GWEN_DB_GetNextGroup(dbDriver);
  } /* while */

  if (dbDriver && dbReader) {
    LC_DRIVER *d;
    const char *dname;
    const char *rtype;
    GWEN_BUFFER *nbuf;
    LC_READER *r;
    char numbuf[32];
    int port;

    /* found reader and driver */
    dname=GWEN_DB_GetCharValue(dbDriver, "driverName", 0, 0);
    assert(dname);

    rtype=GWEN_DB_GetCharValue(dbReader, "readerType", 0, 0);
    assert(rtype);

    d=LC_Driver_List_First(cs->drivers);
    while(d) {
      if (strcasecmp(LC_Driver_GetDriverName(d), dname)==0) {
	if (LC_Driver_GetMaxReaders(d)>LC_Driver_GetActiveReadersCount(d))
	  break;
      }
      d=LC_Driver_List_Next(d);
    } /* while */

    if (!d) {
      GWEN_BUFFER *lbuf;

      /* no driver exists, create one */
      d=LC_Driver_FromDb(dbDriver);
      assert(d);
      lbuf=GWEN_Buffer_new(0, 256, 0, 1);
      LC_CardServer_ReplaceVar(LC_DEFAULT_LOGDIR
                               "/drivers/@driver@"
                               "/@reader@"
                               ".log",
                               "driver", dname, lbuf);
      DBG_DEBUG(0, "Logfile is \"%s\"",
                GWEN_Buffer_GetStart(lbuf));
      LC_Driver_SetLogFile(d, GWEN_Buffer_GetStart(lbuf));
      GWEN_Buffer_free(lbuf);
      LC_Driver_List_Add(d, cs->drivers);
    }

    /* create reader */
    r=LC_Reader_FromDb(d, dbReader);
    assert(r);
    LC_Reader_List_Add(r, cs->readers);

    nbuf=GWEN_Buffer_new(0, 256, 0, 1);
    snprintf(numbuf, sizeof(numbuf), "%d", ++(cs->lastAutoReader));
    GWEN_Buffer_AppendString(nbuf, "auto");
    GWEN_Buffer_AppendString(nbuf, numbuf);
    GWEN_Buffer_AppendByte(nbuf, '-');
    GWEN_Buffer_AppendString(nbuf, rtype);
    LC_Reader_SetReaderName(r, GWEN_Buffer_GetStart(nbuf));
    DBG_NOTICE(0, "AUTOCONFIG: Created new reader \"%s\" (USBTTY: %04x/%04x)",
	       GWEN_Buffer_GetStart(nbuf),
	       LC_USBTTYDevice_GetVendorId(ud),
	       LC_USBTTYDevice_GetProductId(ud));
    GWEN_Buffer_free(nbuf);

    snprintf(numbuf, sizeof(numbuf), "ports/USB%d",
	     LC_USBTTYDevice_GetPort(ud));
    port=GWEN_DB_GetIntValue(dbReader, numbuf, 0,
			     LC_USBTTYDevice_GetPort(ud));
    DBG_DEBUG(0, "Looked up value for port \"%s\" (%d)", numbuf, port);
    LC_Reader_SetPort(r, port);
    LC_Reader_SetDeviceId(r, LC_USBTTYDevice_GetPort(ud));
    LC_Reader_SetIsAvailable(r, 1);

    if (LC_CardServer_Reader_Up(cs, r)) {
      DBG_INFO(0, "here");
    }
  }
  else {
    DBG_INFO(0, "Device %04x/%04x is not a known ttyUSB reader",
	     LC_USBTTYDevice_GetVendorId(ud),
	     LC_USBTTYDevice_GetProductId(ud));
  }

  return 0;
}



int LC_CardServer_USBTTYDevice_Down(LC_CARDSERVER *cs, LC_USBTTYDEVICE *ud) {
  LC_READER *r;

  assert(cs);
  assert(ud);

  r=LC_Reader_List_First(cs->readers);
  while(r) {
    if (strcasecmp(LC_Reader_GetComType(r), "USBSerial")==0) {
      if (LC_Reader_GetDeviceId(r)==(int)LC_USBTTYDevice_GetPort(ud)) {
	/* reader found */
	break;
      }
    }
    r=LC_Reader_List_Next(r);
  } /* while */

  if (!r) {
    DBG_DEBUG(0, "Unknown device %02x",
	      LC_USBTTYDevice_GetPort(ud));
    return 0;
  }

  DBG_NOTICE(0, "Reader \"%s\" unplugged",
	     LC_Reader_GetReaderName(r));
  if (LC_CardServer_StopReader(cs, r)) {
    DBG_INFO(0, "here");
  }

  LC_Reader_SetIsAvailable(r, 0);
  return 0;
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



int LC_CardServer_ScanUSB(LC_CARDSERVER *cs) {
  int rv;
  int reloaded;

  reloaded=0;
  if (cs->lastUsbScan==0 ||
      (cs->usbScanInterval &&
       difftime(time(0), cs->lastUsbScan)>=cs->usbScanInterval)){
    rv=LC_USBMonitor_Scan(cs->usbMonitor);
    if (rv==-1) {
      DBG_INFO(0, "Error scanning USB bus");
    }
    else if (rv==1) {
      DBG_VERBOUS(0, "No changes on USB bus");
    }
    else {
      LC_USBDEVICE_LIST *newDevices;
      LC_USBDEVICE_LIST *lostDevices;
      LC_USBDEVICE *ud;
  
      lostDevices=LC_USBMonitor_GetLostDevices(cs->usbMonitor);
      assert(lostDevices);
      ud=LC_USBDevice_List_First(lostDevices);
      while(ud) {
	DBG_DEBUG(0, "Device %02x/%02x down",
		  LC_USBDevice_GetVendorId(ud),
		  LC_USBDevice_GetProductId(ud));
	if (LC_CardServer_USBDevice_Down(cs, ud)) {
	  DBG_INFO(0, "here");
	}
	ud=LC_USBDevice_List_Next(ud);
      } /* while */

      newDevices=LC_USBMonitor_GetNewDevices(cs->usbMonitor);
      assert(newDevices);
      ud=LC_USBDevice_List_First(newDevices);
      while(ud) {
        DBG_DEBUG(0, "Device %02x/%02x up",
		  LC_USBDevice_GetVendorId(ud),
		  LC_USBDevice_GetProductId(ud));
	if (!reloaded) {
	  /* reload driver list */
	  GWEN_DB_Group_free(cs->dbDrivers);
	  cs->dbDrivers=GWEN_DB_Group_dup(cs->dbConfigDrivers);
	  reloaded=!LC_CardServer_ReadDrivers(cs->dataDir, cs->dbDrivers, 1);
        }

	if (LC_CardServer_USBDevice_Up(cs, ud)) {
	  DBG_INFO(0, "here");
	}
	ud=LC_USBDevice_List_Next(ud);
      } /* while */
    }
    cs->lastUsbScan=time(0);
  }

  if (cs->lastUsbTtyScan==0 ||
      (cs->usbTtyScanInterval &&
       difftime(time(0), cs->lastUsbTtyScan)>=cs->usbTtyScanInterval)){
    if (LC_USBTTYMonitor_Scan(cs->usbTtyMonitor)) {
      DBG_DEBUG(0, "Error scanning USB bus (serial)");
    }
    else {
      LC_USBTTYDEVICE_LIST *newDevices;
      LC_USBTTYDEVICE_LIST *lostDevices;
      LC_USBTTYDEVICE *ud;

      lostDevices=LC_USBTTYMonitor_GetLostDevices(cs->usbTtyMonitor);
      assert(lostDevices);
      ud=LC_USBTTYDevice_List_First(lostDevices);
      while(ud) {
	DBG_DEBUG(0, "Device %02x/%02x down",
		  LC_USBTTYDevice_GetVendorId(ud),
		  LC_USBTTYDevice_GetProductId(ud));
	if (LC_CardServer_USBTTYDevice_Down(cs, ud)) {
	  DBG_INFO(0, "here");
	}
	ud=LC_USBTTYDevice_List_Next(ud);
      } /* while */

      newDevices=LC_USBTTYMonitor_GetNewDevices(cs->usbTtyMonitor);
      assert(newDevices);
      ud=LC_USBTTYDevice_List_First(newDevices);
      while(ud) {
	DBG_DEBUG(0, "Device %02x/%02x up",
		  LC_USBTTYDevice_GetVendorId(ud),
		  LC_USBTTYDevice_GetProductId(ud));
	if (!reloaded) {
	  /* reload driver list */
	  GWEN_DB_Group_free(cs->dbDrivers);
	  cs->dbDrivers=GWEN_DB_Group_dup(cs->dbConfigDrivers);
	  reloaded=!LC_CardServer_ReadDrivers(cs->dataDir, cs->dbDrivers, 1);
	}
	if (LC_CardServer_USBTTYDevice_Up(cs, ud)) {
	  DBG_INFO(0, "here");
	}
	ud=LC_USBTTYDevice_List_Next(ud);
      } /* while */
    }
    cs->lastUsbTtyScan=time(0);
  }

  return 0;
}




int LC_CardServer__USBDeviceToDB(LC_USBDEVICE *ud,
                                 GWEN_DB_NODE *dbDrivers,
                                 GWEN_DB_NODE *dbDriverStore,
                                 GWEN_DB_NODE *dbReaderStore) {
  GWEN_DB_NODE *dbDriver;
  GWEN_DB_NODE *dbReader;

  dbDriver=GWEN_DB_GetFirstGroup(dbDrivers);
  while(dbDriver) {
    dbReader=0;
    if (strcasecmp(GWEN_DB_GroupName(dbDriver), "driver")==0) {
      dbReader=GWEN_DB_GetFirstGroup(dbDriver);
      while(dbReader) {
        if (strcasecmp(GWEN_DB_GroupName(dbReader), "reader")==0) {
          if (strcasecmp(GWEN_DB_GetCharValue(dbReader,
                                              "comType", 0, "serial"),
                         "USB")==0) {
            if ((GWEN_DB_GetIntValue(dbReader, "vendorId", 0, 0)==
                 (int)LC_USBDevice_GetVendorId(ud)) &&
                (GWEN_DB_GetIntValue(dbReader, "productId", 0, 0)==
                 (int)LC_USBDevice_GetProductId(ud))) {
              /* reader found */
              break;
            }
          }
        }
        dbReader=GWEN_DB_GetNextGroup(dbReader);
      } /* while */
    }
    if (dbReader)
      break;
    dbDriver=GWEN_DB_GetNextGroup(dbDriver);
  } /* while */

  if (dbDriver && dbReader) {
    /* found reader and driver */

    /* delete all reader sections here */
    GWEN_DB_ClearGroup(dbDriverStore, 0);
    GWEN_DB_AddGroupChildren(dbDriverStore, dbDriver);
    while(!GWEN_DB_DeleteGroup(dbDriverStore, "reader"));
    GWEN_DB_AddGroupChildren(dbReaderStore, dbReader);
  }
  else {
    DBG_INFO(0, "Device %02x/%02x is not a known reader",
             LC_USBDevice_GetVendorId(ud),
             LC_USBDevice_GetProductId(ud));
    return -1;
  }

  return 0;
}



int LC_CardServer__USBTTYDeviceToDB(LC_USBTTYDEVICE *ud,
                                    GWEN_DB_NODE *dbDrivers,
                                    GWEN_DB_NODE *dbDriverStore,
                                    GWEN_DB_NODE *dbReaderStore) {
  GWEN_DB_NODE *dbDriver;
  GWEN_DB_NODE *dbReader;

  dbDriver=GWEN_DB_GetFirstGroup(dbDrivers);
  while(dbDriver) {
    dbReader=0;
    if (strcasecmp(GWEN_DB_GroupName(dbDriver), "driver")==0) {
      dbReader=GWEN_DB_GetFirstGroup(dbDriver);
      while(dbReader) {
	if (strcasecmp(GWEN_DB_GroupName(dbReader), "reader")==0) {
	  if (strcasecmp(GWEN_DB_GetCharValue(dbReader,
					      "comType", 0, "serial"),
			 "USBSerial")==0) {
	    if ((GWEN_DB_GetIntValue(dbReader, "vendorId", 0, 0)==
		 (int)LC_USBTTYDevice_GetVendorId(ud)) &&
		(GWEN_DB_GetIntValue(dbReader, "productId", 0, 0)==
		 (int)LC_USBTTYDevice_GetProductId(ud))) {
	      /* reader found */
	      break;
	    }
	  }
	}
	dbReader=GWEN_DB_GetNextGroup(dbReader);
      } /* while */
    }
    if (dbReader)
      break;
    dbDriver=GWEN_DB_GetNextGroup(dbDriver);
  } /* while */

  if (dbDriver && dbReader) {
    /* found reader and driver */

    /* delete all reader sections */
    GWEN_DB_ClearGroup(dbDriverStore, 0);
    GWEN_DB_AddGroupChildren(dbDriverStore, dbDriver);
    while(!GWEN_DB_DeleteGroup(dbDriverStore, "reader"));
    GWEN_DB_AddGroupChildren(dbReaderStore, dbReader);
  }
  else {
    DBG_INFO(0, "Device %02x/%02x is not a known reader",
	     LC_USBTTYDevice_GetVendorId(ud),
             LC_USBTTYDevice_GetProductId(ud));
    return -1;
  }

  return 0;
}




int LC_CardServer_GetUSBDevices(GWEN_DB_NODE *dbKnownDrivers,
                                GWEN_DB_NODE *dbReaders) {
  int rv;
  LC_USBMONITOR *usbMonitor;
  LC_USBTTYMONITOR *usbTtyMonitor;

  usbTtyMonitor=LC_USBTTYMonitor_new();
  usbMonitor=LC_USBMonitor_new();

  rv=LC_USBMonitor_Scan(usbMonitor);
  if (rv==-1) {
    DBG_INFO(0, "Error scanning USB bus");
  }
  else if (rv==1) {
    DBG_VERBOUS(0, "No changes on USB bus");
  }
  else {
    LC_USBDEVICE_LIST *newDevices;
    LC_USBDEVICE *ud;

    newDevices=LC_USBMonitor_GetNewDevices(usbMonitor);
    assert(newDevices);
    ud=LC_USBDevice_List_First(newDevices);
    while(ud) {
      GWEN_DB_NODE *dbReaderStore;
      GWEN_DB_NODE *dbDriverStore;

      DBG_DEBUG(0, "Device %02x/%02x found",
                LC_USBDevice_GetVendorId(ud),
                LC_USBDevice_GetProductId(ud));
      dbReaderStore=GWEN_DB_Group_new("readerStore");
      dbDriverStore=GWEN_DB_Group_new("driverStore");

      if (LC_CardServer__USBDeviceToDB(ud,
                                       dbKnownDrivers,
                                       dbDriverStore,
                                       dbReaderStore)) {
        DBG_INFO(0, "here");
      }
      else {
        GWEN_DB_NODE *dbReader;
        const char *p;

        dbReader=GWEN_DB_Group_new("reader");
        p=GWEN_DB_GetCharValue(dbDriverStore, "manufacturer", 0,
                               "(unknown)");
        GWEN_DB_SetCharValue(dbReader, GWEN_DB_FLAGS_DEFAULT,
                             "manufacturer", p);
        p=GWEN_DB_GetCharValue(dbReaderStore, "shortName", 0,
                               "(unknown)");
        GWEN_DB_SetCharValue(dbReader, GWEN_DB_FLAGS_DEFAULT,
                             "name", p);
        GWEN_DB_SetCharValue(dbReader, GWEN_DB_FLAGS_DEFAULT,
                             "type", "USB");
        GWEN_DB_AddGroup(dbReaders, dbReader);
      }
      ud=LC_USBDevice_List_Next(ud);
    } /* while */
  }

  if (LC_USBTTYMonitor_Scan(usbTtyMonitor)) {
    DBG_INFO(0, "Error scanning USB bus (serial)");
  }
  else {
    LC_USBTTYDEVICE_LIST *newDevices;
    LC_USBTTYDEVICE *ud;

    newDevices=LC_USBTTYMonitor_GetNewDevices(usbTtyMonitor);
    assert(newDevices);
    ud=LC_USBTTYDevice_List_First(newDevices);
    while(ud) {
      GWEN_DB_NODE *dbReaderStore=0;
      GWEN_DB_NODE *dbDriverStore=0;

      DBG_DEBUG(0, "Device %02x/%02x found",
                LC_USBTTYDevice_GetVendorId(ud),
                LC_USBTTYDevice_GetProductId(ud));
      if (LC_CardServer__USBTTYDeviceToDB(ud,
                                          dbKnownDrivers,
                                          dbDriverStore,
                                          dbReaderStore)) {
        DBG_INFO(0, "here");
      }
      else {
        GWEN_DB_NODE *dbReader;
        const char *p;

        dbReader=GWEN_DB_Group_new("reader");
        p=GWEN_DB_GetCharValue(dbDriverStore, "manufacturer", 0,
                               "(unknown)");
        GWEN_DB_SetCharValue(dbReader, GWEN_DB_FLAGS_DEFAULT,
                             "manufacturer", p);
        p=GWEN_DB_GetCharValue(dbReaderStore, "shortName", 0,
                               "(unknown)");
        GWEN_DB_SetCharValue(dbReader, GWEN_DB_FLAGS_DEFAULT,
                             "name", p);
        GWEN_DB_SetCharValue(dbReader, GWEN_DB_FLAGS_DEFAULT,
                             "type", "USB-Serial");
        GWEN_DB_AddGroup(dbReaders, dbReader);
      }
      ud=LC_USBTTYDevice_List_Next(ud);
    } /* while */
  }

  LC_USBMonitor_free(usbMonitor);
  LC_USBTTYMonitor_free(usbTtyMonitor);
  return 0;
}



GWEN_TYPE_UINT32 LC_CardServer_GetNotificationMask(const char *ntype,
                                                   const char *ncode) {
  GWEN_TYPE_UINT32 res;

  assert(ntype);
  assert(ncode);

  res=0;

  if (strcasecmp(ntype, LC_NOTIFY_TYPE_DRIVER)==0) {
    if (strcasecmp(ncode, LC_NOTIFY_CODE_DRIVER_START)==0)
      res=LC_NOTIFY_FLAGS_DRIVER_START;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_DRIVER_UP)==0)
      res=LC_NOTIFY_FLAGS_DRIVER_UP;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_DRIVER_DOWN)==0)
      res=LC_NOTIFY_FLAGS_DRIVER_DOWN;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_DRIVER_ERROR)==0)
      res=LC_NOTIFY_FLAGS_DRIVER_ERROR;
  }
  else if (strcasecmp(ntype, LC_NOTIFY_TYPE_READER)==0) {
    if (strcasecmp(ncode, LC_NOTIFY_CODE_READER_START)==0)
      res=LC_NOTIFY_FLAGS_READER_START;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_READER_UP)==0)
      res=LC_NOTIFY_FLAGS_READER_UP;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_READER_DOWN)==0)
      res=LC_NOTIFY_FLAGS_READER_DOWN;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_READER_ERROR)==0)
      res=LC_NOTIFY_FLAGS_READER_ERROR;
  }
  else if (strcasecmp(ntype, LC_NOTIFY_TYPE_SERVICE)==0) {
    if (strcasecmp(ncode, LC_NOTIFY_CODE_SERVICE_START)==0)
      res=LC_NOTIFY_FLAGS_SERVICE_START;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_SERVICE_UP)==0)
      res=LC_NOTIFY_FLAGS_SERVICE_UP;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_SERVICE_DOWN)==0)
      res=LC_NOTIFY_FLAGS_SERVICE_DOWN;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_SERVICE_ERROR)==0)
      res=LC_NOTIFY_FLAGS_SERVICE_ERROR;
  }
  else if (strcasecmp(ntype, LC_NOTIFY_TYPE_CARD)==0) {
    if (strcasecmp(ncode, LC_NOTIFY_CODE_CARD_INSERTED)==0)
      res=LC_NOTIFY_FLAGS_CARD_INSERTED;
    else if (strcasecmp(ncode, LC_NOTIFY_CODE_CARD_REMOVED)==0)
      res=LC_NOTIFY_FLAGS_CARD_REMOVED;
  }
  else if (strcasecmp(ntype, LC_NOTIFY_TYPE_CLIENT)==0) {
  }

  if (res==0) {
    DBG_ERROR(0, "Could not translate \"%s:%s\" into a mask",
              ntype, ncode);
    return 0;
  }
  return res;
}



int LC_CardServer__SendNotification(LC_CARDSERVER *cs,
                                    const LC_CLIENT *cl,
                                    const char *ntype,
                                    const char *ncode,
                                    GWEN_DB_NODE *dbData){
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  GWEN_TYPE_UINT32 rid;

  assert(ntype);
  assert(ncode);
  DBG_NOTICE(0, "Sending notification to client \"%08x\"",
             LC_Client_GetClientId(cl));

  dbReq=GWEN_DB_Group_new("Notification");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LC_Client_GetClientId(cl));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "clientId", numbuf);
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "ntype", ntype);
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "ncode", ncode);
  if (dbData) {
    GWEN_DB_NODE *dbT;

    dbT=GWEN_DB_GetGroup(dbReq, GWEN_DB_FLAGS_DEFAULT, "data");
    assert(dbT);
    GWEN_DB_AddGroupChildren(dbT, dbData);
  }

  /* send request (fire and forget) */
  rid=GWEN_IPCManager_SendRequest(cs->ipcManager,
                                  LC_Client_GetClientId(cl),
                                  dbReq);
  if (rid==0) {
    DBG_INFO(0, "here");
    return -1;
  }
  GWEN_IPCManager_RemoveRequest(cs->ipcManager, rid, 1);

  /* done */
  return 0;
}



int LC_CardServer_SendNotification(LC_CARDSERVER *cs,
                                   LC_CLIENT *cl,
                                   const char *ntype,
                                   const char *ncode,
                                   GWEN_DB_NODE *dbData){
  int rv;
  GWEN_TYPE_UINT32 mask;
  int err;
  int clients;

  assert(ntype);
  assert(ncode);

  DBG_INFO(0, "Sending notification %s:%s to all clients",
           ntype, ncode);

  mask=LC_CardServer_GetNotificationMask(ntype, ncode);
  if (!mask) {
    DBG_ERROR(0, "here");
    return -1;
  }

  DBG_DEBUG(0, "Mask for %s:%s is %08x", ntype, ncode, mask);

  err=0;
  clients=0;
  if (cl!=0) {
    if (LC_Client_GetNotifyFlags(cl) & mask) {
      clients++;
      rv=LC_CardServer__SendNotification(cs, cl, ntype, ncode, dbData);
      if (rv) {
	DBG_ERROR(0, "Error sending request to client \"%08x\"",
		  LC_Client_GetClientId(cl));
	err++;
      }
    }
    else {
      DBG_DEBUG(0, "Client \"%08x\" is not interested in %s:%s",
		LC_Client_GetClientId(cl), ntype, ncode);
    }
  }
  else {
    cl=LC_Client_List_First(cs->clients);
    while(cl) {
      if (LC_Client_GetNotifyFlags(cl) & mask) {
	clients++;
	rv=LC_CardServer__SendNotification(cs, cl, ntype, ncode, dbData);
	if (rv) {
	  DBG_ERROR(0, "Error sending request to client \"%08x\"",
		    LC_Client_GetClientId(cl));
	  err++;
	}
      }
      else {
	DBG_DEBUG(0, "Client \"%08x\" is not interested in %s:%s",
		  LC_Client_GetClientId(cl), ntype, ncode);
      }
      cl=LC_Client_List_Next(cl);
    }
  }

  if (clients && (clients==err)) {
    DBG_ERROR(0, "Could not send notification to any client");
    return -1;
  }

  /* done */
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



void LC_CardServer_DumpState(LC_CARDSERVER *cs) {
  LC_DRIVER *d;
  LC_READER *r;

  fprintf(stderr, "Card-Server Status:\n");
  fprintf(stderr, "  Drivers:\n");
  d=LC_Driver_List_First(cs->drivers);
  while(d) {
    LC_Driver_Dump(d, stderr, 4);
    d=LC_Driver_List_Next(d);
  }

  fprintf(stderr, "  Readers:\n");
  r=LC_Reader_List_First(cs->readers);
  while(r) {
    LC_Reader_Dump(r, stderr, 4);
    r=LC_Reader_List_Next(r);
  }


}



int LC_CardServer_GetClientCount(const LC_CARDSERVER *cs){
  assert(cs);
  return LC_Client_List_GetCount(cs->clients);
}




