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


#include "client_p.h"
#include "mon/monitor_l.h"
#include "apps/cardmgr_l.h"
#include <gwenhywfar/gwenhywfar.h>
#include <gwenhywfar/version.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/net.h>
#include <gwenhywfar/nettransportsock.h>
#include <gwenhywfar/nettransportssl.h>
#include <gwenhywfar/netconnectionhttp.h>
#include <gwenhywfar/waitcallback.h>
#include <gwenhywfar/directory.h>
#include <chipcard2/chipcard2.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <time.h>

#ifdef OS_WIN32
# define DIRSEP "\\"
#else
# define DIRSEP "/"
#endif


GWEN_INHERIT_FUNCTIONS(LC_CLIENT)



LC_CLIENT *LC_Client_new(const char *programName,
                         const char *programVersion,
                         const char *dataDir) {
  LC_CLIENT *cl;
  GWEN_STRINGLIST *paths;
  GWEN_BUFFER *tbuf;
  GWEN_ERRORCODE err;

  err=GWEN_Init();
  if (!GWEN_Error_IsOk(err)) {
    DBG_ERROR_ERR(LC_LOGDOMAIN, err);
    abort();
  }

  if (!GWEN_Logger_Exists(LC_LOGDOMAIN)) {
    const char *s;

    /* only set our logger if not not already has been */
    GWEN_Logger_Open(LC_LOGDOMAIN, "chipcard2-client", 0,
		     GWEN_LoggerTypeConsole,
		     GWEN_LoggerFacilityUser);
    GWEN_Logger_SetLevel(LC_LOGDOMAIN, GWEN_LoggerLevelWarning);

    s=getenv("LC_LOGLEVEL");
    if (s) {
      GWEN_LOGGER_LEVEL ll;

      ll=GWEN_Logger_Name2Level(s);
      if (ll!=GWEN_LoggerLevelUnknown) {
	GWEN_Logger_SetLevel(LC_LOGDOMAIN, ll);
	DBG_WARN(0,
		 "Overriding loglevel for Lichipcard-Client with \"%s\"",
		 s);
      }
      else {
	DBG_ERROR(0, "Unknown loglevel \"%s\"",
		  s);
      }
    }
    else {
      GWEN_Logger_SetLevel(LC_LOGDOMAIN, GWEN_LoggerLevelWarning);
    }
  }

  GWEN_NEW_OBJECT(LC_CLIENT, cl);
  GWEN_INHERIT_INIT(LC_CLIENT, cl);
  cl->servers=LC_Server_List_new();
  cl->waitingRequests=LC_Request_List_new();
  cl->workingRequests=LC_Request_List_new();
  cl->cards=LC_Card_List_new();
  cl->monitor=LCM_Monitor_new();
  if (programName)
    cl->programName=strdup(programName);
  if (programVersion)
    cl->programVersion=strdup(programVersion);
  if (dataDir)
    cl->dataDir=strdup(dataDir);
  else {
    GWEN_BUFFER *dirbuf;
    char homeDir[256];
    int rv;

    rv=GWEN_Directory_GetHomeDirectory(homeDir, sizeof(homeDir));
    assert(rv==0);
    dirbuf=GWEN_Buffer_new(0, 256, 0, 1);
    GWEN_Buffer_AppendString(dirbuf, homeDir);
#ifdef OS_WIN32
    GWEN_Buffer_AppendByte(dirbuf, '\\');
#else
    GWEN_Buffer_AppendByte(dirbuf, '/');
#endif
    GWEN_Buffer_AppendString(dirbuf, LC_CLIENT_DATADIR);
    cl->dataDir=strdup(GWEN_Buffer_GetStart(dirbuf));
    GWEN_Buffer_free(dirbuf);
  }

  cl->ipcManager=GWEN_IPCManager_new();

  paths=GWEN_StringList_new();

  /* always append system-wide data dir */
  tbuf=GWEN_Buffer_new(0, 256, 0, 1);
  GWEN_Directory_OsifyPath(LC_DEFAULT_DATADIR, tbuf, 1);
  GWEN_StringList_AppendString(paths,
                               GWEN_Buffer_GetStart(tbuf),
                               0, 1);
  GWEN_Buffer_free(tbuf);

  /* append local data dir */
  GWEN_StringList_AppendString(paths, cl->dataDir, 0, 1);
  cl->cardMgr=LC_CardMgr_new(paths);
  GWEN_StringList_free(paths);

  return cl;
}



void LC_Client_free(LC_CLIENT *cl) {
  if (cl) {
    GWEN_ERRORCODE err;

    GWEN_INHERIT_FINI(LC_CLIENT, cl);
    free(cl->programName);
    free(cl->programVersion);
    free(cl->dataDir);
    LCM_Monitor_free(cl->monitor);
    LC_CardMgr_free(cl->cardMgr);
    LC_Card_List_free(cl->cards);
    LC_Request_List_free(cl->waitingRequests);
    LC_Request_List_free(cl->workingRequests);
    LC_Server_List_free(cl->servers);
    GWEN_IPCManager_free(cl->ipcManager);
    GWEN_FREE_OBJECT(cl);

    err=GWEN_Fini();
    if (!GWEN_Error_IsOk(err)) {
      DBG_ERROR_ERR(LC_LOGDOMAIN, err);
    }
  }
}



LCM_MONITOR *LC_Client_GetMonitor(const LC_CLIENT *cl){
  assert(cl);
  return cl->monitor;
}



int LC_Client_GetShortTimeout(const LC_CLIENT *cl) {
  assert(cl);
  return cl->shortTimeout;
}



int LC_Client_GetLongTimeout(const LC_CLIENT *cl) {
  assert(cl);
  return cl->longTimeout;
}



int LC_Client_GetVeryLongTimeout(const LC_CLIENT *cl) {
  assert(cl);
  return cl->veryLongTimeout;
}



int LC_Client_CheckForError(GWEN_DB_NODE *db) {
  const char *name;

  name=GWEN_DB_GetCharValue(db, "command/vars/cmd", 0, 0);
  assert(name);
  if (strcasecmp(name, "Error")==0) {
    int code;
    const char *text;

    code=GWEN_DB_GetIntValue(db, "body/code", 0, 0);
    text=GWEN_DB_GetCharValue(db, "body/text", 0, "(empty)");
    if (code) {
      DBG_ERROR(LC_LOGDOMAIN, "Error %d: %s", code, text);
    }
    else {
      if (text) {
        DBG_INFO(LC_LOGDOMAIN, "Info: %s", text);
      }
    }
    return code;
  }

  return 0;
}



int LC_Client_ServerDown(LC_CLIENT *cl, LC_SERVER *sv) {
  LC_CARD *cd;
  LC_REQUEST *rq;

  assert(cl);
  assert(sv);

  if (cl->serverDownFn)
    cl->serverDownFn(cl, LC_Server_GetServerId(sv));

  cd=LC_Card_List_First(cl->cards);
  while(cd) {
    LC_CARD *next;

    next=LC_Card_List_Next(cd);
    if (LC_Card_GetServerId(cd)==LC_Server_GetServerId(sv)) {
      LC_Card_ResetCardId(cd);
      LC_Card_List_Del(cd);
      LC_Card_free(cd);
    }
    cd=next;
  } /* while */

  /* TODO: remove cards from monitor */

  /* check every request in the working list */
  rq=LC_Request_List_First(cl->workingRequests);
  while(rq) {
    if (!LC_Request_GetIsAborted(rq)) {
      if (LC_Request_GetRequestId(rq)==LC_Server_GetServerId(sv)) {
        GWEN_IPCManager_RemoveRequest(cl->ipcManager,
                                      LC_Request_GetIpcRequestId(rq),
                                      1);
        LC_Request_SetIpcRequestId(rq, 0);
        /* mark request as aborted */
        LC_Request_SetIsAborted(rq, 1);
      } /* if request id matches */
    }
    rq=LC_Request_List_Next(rq);
  } /* while */

  LC_Server_SetStatus(sv, LC_ServerStatusUnconnected);
  LC_Server_SetCurrentCommand(sv, 0);
  GWEN_IPCManager_Disconnect(cl->ipcManager, LC_Server_GetServerId(sv));

  return 0;
}



int LC_Client_CheckServer(LC_CLIENT *cl, LC_SERVER *sv) {
  GWEN_TYPE_UINT32 rid;
  LC_REQUEST *rq;
  int handled;
  int done;

  assert(cl);
  assert(sv);
  handled=0;
  done=0;

  if (LC_Server_GetStatus(sv)==LC_ServerStatusUnconnected) {
    handled=1;
    rq=LC_Client_PeekNextRequest(cl, LC_Server_GetServerId(sv));
    if (rq) {
      DBG_INFO(LC_LOGDOMAIN, "Starting to connect");
      if (LC_Client_StartConnect(cl, sv)) {
        DBG_ERROR(LC_LOGDOMAIN, "Could not start connecting to server");
        return -1;
      }
      done++;
    }
    else {
      DBG_VERBOUS(LC_LOGDOMAIN, "No request");
    }
  }

  if (LC_Server_GetStatus(sv)==LC_ServerStatusWaitReady) {
    handled=1;
    rid=LC_Server_GetCurrentCommand(sv);
    if (rid==0) {
      DBG_ERROR(LC_LOGDOMAIN, "No current command in WaitReady mode?");
      LC_Server_SetStatus(sv, LC_ServerStatusAborted);
      return -1;
    }
    else {
      GWEN_DB_NODE *dbReq;

      dbReq=GWEN_IPCManager_GetResponseData(cl->ipcManager, rid);
      if (dbReq==0) {
        /* TODO: check for timeout */
        DBG_DEBUG(LC_LOGDOMAIN, "No response yet");
      }
      else {
        int errCode;

        errCode=LC_Client_CheckForError(dbReq);
        if (errCode) {
          DBG_INFO(LC_LOGDOMAIN, "Error connecting (%08x)", errCode);
          LC_Server_SetStatus(sv, LC_ServerStatusAborted);
          LC_Server_SetCurrentCommand(sv, 0);
          GWEN_DB_Group_free(dbReq);
          GWEN_IPCManager_RemoveRequest(cl->ipcManager, rid, 1);
          return -1;
        }
        LC_Server_SetCurrentCommand(sv, 0);
        LC_Server_SetStatus(sv, LC_ServerStatusConnected);
        DBG_INFO(LC_LOGDOMAIN, "Connected");
        done++;
      }
    }
  }

  if (LC_Server_GetStatus(sv)==LC_ServerStatusConnected) {
    LC_REQUEST *rq;

    handled=1;
    /* check for incoming command */
    DBG_DEBUG(LC_LOGDOMAIN, "Checking for incoming requests");
    rid=GWEN_IPCManager_GetNextInRequest(cl->ipcManager,
                                         LC_CLIENT_MARK);
    if (rid) {
      GWEN_DB_NODE *dbReq;
      const char *name;
      int rv;

      /* there is an incoming request */
      DBG_DEBUG(LC_LOGDOMAIN, "Got an incoming request");
      done++;
      dbReq=GWEN_IPCManager_GetInRequestData(cl->ipcManager, rid);
      assert(dbReq);
      name=GWEN_DB_GetCharValue(dbReq, "command/vars/cmd", 0, 0);
      rv=LC_Client_HandleInRequest(cl, rid, dbReq);
      if (rv==1) {
        if (strcasecmp(name, "CardAvailable")==0) {
          DBG_DEBUG(LC_LOGDOMAIN, "A card seems to be available");
          if (LC_Client_HandleCardAvailable(cl, dbReq)) {
            DBG_WARN(LC_LOGDOMAIN, "Error handling CardAvailable message");
          }
        }
        else if (strcasecmp(name, "Notification")==0) {
          DBG_INFO(LC_LOGDOMAIN, "Notification received");
          if (LC_Client_HandleNotification(cl, dbReq)) {
            DBG_WARN(LC_LOGDOMAIN, "Error handling Notification message");
          }
        }
        else {
          DBG_NOTICE(LC_LOGDOMAIN, "Unhandled incoming request:");
          if (GWEN_Logger_GetLevel(0)>=GWEN_LoggerLevelNotice) {
            GWEN_DB_Dump(dbReq, stderr, 2);
          }
        }
        DBG_DEBUG(LC_LOGDOMAIN, "Removing incoming request");
        GWEN_IPCManager_RemoveRequest(cl->ipcManager, rid, 0);
      } /* if not externally handled */
    } /* if incoming request */

    /* check for outbound requests */
    rq=LC_Client_PeekNextRequest(cl, LC_Server_GetServerId(sv));
    if (rq) {
      GWEN_DB_NODE *dbReq;
      GWEN_NETTRANSPORT_STATUS nst;

      /* we have a waiting request */

      /* check for server status */
      nst=GWEN_IPCManager_CheckConnection(cl->ipcManager,
                                          LC_Server_GetServerId(sv));
      if (nst!=GWEN_NetTransportStatusLConnected) {
        /* server is no longer connected */
        DBG_INFO(LC_LOGDOMAIN, "Server is down");
        LC_Client_ServerDown(cl, sv);
        done++;
      }
      else {
        dbReq=LC_Request_GetRequestData(rq);
        assert(dbReq);
        DBG_DEBUG(LC_LOGDOMAIN, "Sending waiting request %08x",
                  LC_Request_GetRequestId(rq));
        if (GWEN_Logger_GetLevel(0)>=GWEN_LoggerLevelDebug)
          GWEN_DB_Dump(dbReq, stderr, 4);
        done++;
        rid=GWEN_IPCManager_SendRequest(cl->ipcManager,
                                        LC_Server_GetServerId(sv),
                                        GWEN_DB_Group_dup(dbReq));
        if (rid==0) {
          DBG_ERROR(LC_LOGDOMAIN, "Could not send request");
          LC_Server_SetStatus(sv, LC_ServerStatusAborted);
          return -1;
        }
        LC_Request_SetIpcRequestId(rq, rid);
        /* above we only called the peek function, now we remove the
         * request */
        rq=LC_Client_GetNextRequest(cl, LC_Server_GetServerId(sv));
      }
    } /* if there is a request */
  } /* if connected */

  if (LC_Server_GetStatus(sv)==LC_ServerStatusAborted) {
    handled=1;
    /* TODO: Check for timeout, set to Unconnected */
  }

  if (!handled) {
    DBG_ERROR(LC_LOGDOMAIN, "Unknown status %d", LC_Server_GetStatus(sv));
    return -1;
  }

  return done?0:1;
}



int LC_Client_Walk(LC_CLIENT *cl) {
  LC_SERVER *sv;
  int rv;
  int done;

  assert(cl);

  /* check all servers */
  done=0;
  sv=LC_Server_List_First(cl->servers);
  while(sv) {
    rv=LC_Client_CheckServer(cl, sv);
    if (rv==-1) {
      DBG_INFO(LC_LOGDOMAIN, "Error checking server");
    }
    else if (rv==0)
      done++;
    sv=LC_Server_List_Next(sv);
  } /* while */

  /* TODO: Check requests for timeouts */

  return done?0:1;
}



int LC_Client__Work(LC_CLIENT *cl, int maxmsg){
  int done;
  int rv;

  assert(cl);

  done=0;

  while(1) {
    rv=LC_Client_Walk(cl);
    if (rv==-1) {
      DBG_INFO(LC_LOGDOMAIN, "Error on Walk");
      return -1;
    }
    else if (rv==0)
      done++;
    else
      break;
    if (done>256) {
      DBG_ERROR(LC_LOGDOMAIN,
                "EMERGENCY BRAKE !!! Exiting from endless loop");
      break;
    }
  }

  //if (done)
  //  return 0;

  while(1) {
    rv=GWEN_IPCManager_Work(cl->ipcManager, maxmsg);
    if (rv==-1) {
      DBG_INFO(LC_LOGDOMAIN, "Error on WorkIO");
      return -1;
    }
    else if (rv==0)
      done++;
    else
      break;
    if (done>256) {
      DBG_ERROR(LC_LOGDOMAIN,
                "EMERGENCY BRAKE !!! Exiting from endless loop");
      break;
    }
  }

  return done?0:1;
}



int LC_Client_Work(LC_CLIENT *cl, int maxmsg){
  for (;;) {
    int rv;

    rv=LC_Client__Work(cl, maxmsg);
    if (rv!=0) {
      DBG_VERBOUS(LC_LOGDOMAIN, "Nothing done");
      return rv;
    }
    DBG_VERBOUS(LC_LOGDOMAIN, "Something done");
  }
}



LC_CLIENT_RESULT LC_Client_Work_Wait(LC_CLIENT *cl, int timeout) {
  time_t startt;
  int distance;
  GWEN_NETCONNECTION_WORKRESULT res;

  startt=time(0);
  assert(cl);

  /* check every request with the given id in the working list */
  if (timeout==GWEN_NETCONNECTION_TIMEOUT_NONE)
    distance=GWEN_NETCONNECTION_TIMEOUT_NONE;
  else if (timeout==GWEN_NETCONNECTION_TIMEOUT_FOREVER)
    distance=GWEN_NETCONNECTION_TIMEOUT_FOREVER;
  else {
    distance=GWEN_WaitCallback_GetDistance(0);
    if (distance)
      if ((distance/1000)>timeout)
        distance=timeout*1000;
    if (!distance)
      distance=750;
  }

  GWEN_WaitCallback_Enter(LC_CLIENT_CBID_IO_WAITRSP);
  for (;;) {
    int didSomething;

    didSomething=0;
    while(1) {
      int rv;

      rv=LC_Client__Work(cl, 0);
      if (rv==-1) {
        DBG_ERROR(LC_LOGDOMAIN, "Error working on client");
        GWEN_WaitCallback_Leave();
        return LC_Client_ResultGeneric;
      }
      else if (rv==1) {
        DBG_VERBOUS(0, "Nothing done");
        break;
      }
      else if (rv==0) {
        DBG_VERBOUS(0, "Something done");
        didSomething=1;
        break;
      }
    }
    if (didSomething) {
      GWEN_WaitCallback_Leave();
      return LC_Client_ResultOk;
    }

    for (;;) {
      if (GWEN_WaitCallback()==GWEN_WaitCallbackResult_Abort) {
        DBG_ERROR(LC_LOGDOMAIN, "User aborted via waitcallback");
        GWEN_WaitCallback_Leave();
        return LC_Client_ResultAborted;
      }

      res=GWEN_Net_HeartBeat(distance);
      if (res==GWEN_NetConnectionWorkResult_Error) {
        DBG_ERROR(LC_LOGDOMAIN, "Error while working (%d)", res);
        GWEN_WaitCallback_Leave();
        return LC_Client_ResultIpcError;
      }
      else if (res==GWEN_NetConnectionWorkResult_Change) {
        DBG_VERBOUS(LC_LOGDOMAIN, "Changed");
        break;
      }

      /* check timeout */
      if (timeout!=GWEN_NETCONNECTION_TIMEOUT_FOREVER) {
        if (timeout==GWEN_NETCONNECTION_TIMEOUT_NONE ||
            difftime(time(0), startt)>timeout) {
          DBG_INFO(LC_LOGDOMAIN,
                   "Could not read within %d seconds, giving up",
                   timeout);
          GWEN_WaitCallback_Leave();
          return LC_Client_ResultWait;
        }
      }
    } /* for */
  } /* for */
  GWEN_WaitCallback_Leave();

  return LC_Client_ResultOk;
}





int LC_Client_StartConnect(LC_CLIENT *cl, LC_SERVER *sv) {
  GWEN_DB_NODE *dbReq;
  GWEN_TYPE_UINT32 rid;
  LC_SERVER_STATUS st;

  assert(cl);
  assert(sv);

  st=LC_Server_GetStatus(sv);
  if (st!=LC_ServerStatusUnconnected) {
    DBG_ERROR(LC_LOGDOMAIN, "Bad server status (%d)", st);
    return -1;
  }

  /* tell the server about our status */
  dbReq=GWEN_DB_Group_new("ClientReady");
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "Application", cl->programName);
  GWEN_DB_SetCharValue(dbReq,
		       GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "GwenVersion", GWENHYWFAR_VERSION_FULL_STRING);
  GWEN_DB_SetCharValue(dbReq,
		       GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "ChipcardVersion", CHIPCARD_VERSION_FULL_STRING);
  GWEN_DB_SetCharValue(dbReq,
                       GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "System", ""); /* TODO: Get system string */

  /* bypass request list, send directly */
  rid=GWEN_IPCManager_SendRequest(cl->ipcManager,
                                  LC_Server_GetServerId(sv),
                                  dbReq);
  if (rid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send command");
    return -1;
  }

  /* TODO: Add error response to currently selected request */
  LC_Server_SetCurrentCommand(sv, rid);
  LC_Server_SetStatus(sv, LC_ServerStatusWaitReady);
  DBG_INFO(LC_LOGDOMAIN, "Started to connect");
  return 0;
}



int LC_Client_ReadConfig(LC_CLIENT *cl, GWEN_DB_NODE *db) {
  GWEN_DB_NODE *gr;
  const char *globalOwnCertFile;
  GWEN_BUFFER *cfbuf;

  assert(cl);
  assert(db);

  cfbuf=0;
  globalOwnCertFile=GWEN_DB_GetCharValue(db, "certfile", 0, 0);
  if (!globalOwnCertFile) {
    FILE *f;

    cfbuf=GWEN_Buffer_new(0, 256, 0, 1);
    GWEN_Buffer_AppendString(cfbuf, cl->dataDir);
#ifdef OS_WIN32
    GWEN_Buffer_AppendByte(cfbuf, '\\');
#else
    GWEN_Buffer_AppendByte(cfbuf, '/');
#endif
    GWEN_Buffer_AppendString(cfbuf, "user.crt");
    f=fopen(GWEN_Buffer_GetStart(cfbuf), "r");
    if (f) {
      fclose(f);
      DBG_INFO(LC_LOGDOMAIN, "Default certificate is \"%s\"",
                 GWEN_Buffer_GetStart(cfbuf));
      globalOwnCertFile=GWEN_Buffer_GetStart(cfbuf);
    }
  }

  cl->shortTimeout=
    GWEN_DB_GetIntValue(db, "shortTimeout", 0, LC_CLIENT_SHORT_TIMEOUT);
  cl->longTimeout=
    GWEN_DB_GetIntValue(db, "longTimeout", 0, LC_CLIENT_LONG_TIMEOUT);
  cl->veryLongTimeout=
    GWEN_DB_GetIntValue(db, "veryLongTimeout", 0, LC_CLIENT_VERYLONG_TIMEOUT);

  /* read servers */
  gr=GWEN_DB_GetFirstGroup(db);
  while(gr) {
    LC_SERVER *sv;
    GWEN_NETCONNECTION *conn;

    if (strcasecmp(GWEN_DB_GroupName(gr), "server")==0) {
      const char *typ;
      GWEN_NETTRANSPORT *tr;
      GWEN_SOCKET *sk;
      GWEN_INETADDRESS *addr;
      GWEN_TYPE_UINT32 sid;
      const char *userName;
      const char *password;

      userName=GWEN_DB_GetCharValue(gr, "userName", 0, 0);
      password=GWEN_DB_GetCharValue(gr, "password", 0, 0);
      typ=GWEN_DB_GetCharValue(gr, "typ", 0, "local");
      if (strcasecmp(typ, "local")==0) {
        /* HTTP over UDS */
        sk=GWEN_Socket_new(GWEN_SocketTypeUnix);
        addr=GWEN_InetAddr_new(GWEN_AddressFamilyUnix);
        GWEN_InetAddr_SetAddress(addr,
                                 GWEN_DB_GetCharValue
                                 (gr,
                                  "addr", 0,
                                  LC_DEFAULT_UDS_SOCK
                                 )
                                );
        tr=GWEN_NetTransportSocket_new(sk, 1);
        GWEN_NetTransport_AddFlags(tr, GWEN_NETTRANSPORT_FLAGS_RESTARTABLE);
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
        GWEN_NetTransport_AddFlags(tr, GWEN_NETTRANSPORT_FLAGS_RESTARTABLE);
      }
      else {
        const char *certDir;
        const char *newCertDir;
        const char *ownCertFile;
        const char *ciphers;
        GWEN_BUFFER *tmpbuf1;
        GWEN_BUFFER *tmpbuf2;

        tmpbuf1=GWEN_Buffer_new(0, 256, 0, 1);
        GWEN_Buffer_AppendString(tmpbuf1, cl->dataDir);
        GWEN_Buffer_AppendString(tmpbuf1, DIRSEP LC_CLIENT_CERTDIR);
        tmpbuf2=GWEN_Buffer_new(0, 256, 0, 1);
        GWEN_Buffer_AppendBuffer(tmpbuf2, tmpbuf1);

        GWEN_Buffer_AppendString(tmpbuf1, DIRSEP "valid");
        GWEN_Buffer_AppendString(tmpbuf2, DIRSEP "new");

        certDir=GWEN_DB_GetCharValue(gr, "certdir", 0,
                                     GWEN_Buffer_GetStart(tmpbuf1));
        newCertDir=GWEN_DB_GetCharValue(gr, "newCertdir", 0,
                                        GWEN_Buffer_GetStart(tmpbuf2));
        /* create path if necessary */
        if (GWEN_Directory_GetPath(certDir,
                                   GWEN_PATH_FLAGS_CHECKROOT)) {
          DBG_ERROR(LC_LOGDOMAIN, "Could not access path \"%s\"",
                    certDir);
          GWEN_Buffer_free(tmpbuf2);
          GWEN_Buffer_free(tmpbuf1);
          GWEN_Buffer_free(cfbuf);
          return -1;
        }

        ownCertFile=GWEN_DB_GetCharValue(gr, "certfile", 0,
                                         globalOwnCertFile);
        ciphers=GWEN_DB_GetCharValue(gr, "ciphers", 0, 0);
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
          DBG_INFO(LC_LOGDOMAIN, "Using private socket");
          sk=GWEN_Socket_new(GWEN_SocketTypeTCP);
          tr=GWEN_NetTransportSSL_new(sk,
                                      certDir,
                                      newCertDir,
                                      ownCertFile,
                                      0,
                                      0,
                                      1);
          GWEN_NetTransport_AddFlags(tr, GWEN_NETTRANSPORT_FLAGS_RESTARTABLE);
        }
        else if (strcasecmp(typ, "secure")==0) {
          /* HTTP over SSL with certificates */
          sk=GWEN_Socket_new(GWEN_SocketTypeTCP);
          tr=GWEN_NetTransportSSL_new(sk,
                                      certDir,
                                      newCertDir,
                                      ownCertFile,
                                      0,
                                      1,
                                      1);
          GWEN_NetTransport_AddFlags(tr, GWEN_NETTRANSPORT_FLAGS_RESTARTABLE);
        }
        else {
          DBG_ERROR(LC_LOGDOMAIN, "Unknown mode \"%s\"", typ);
          GWEN_InetAddr_free(addr);
          GWEN_Buffer_free(tmpbuf2);
          GWEN_Buffer_free(tmpbuf1);
          GWEN_Buffer_free(cfbuf);
          return -1;
        }
        GWEN_Buffer_free(tmpbuf2);
        GWEN_Buffer_free(tmpbuf1);

        if (ciphers)
          GWEN_NetTransportSSL_SetCipherList(tr, ciphers);
      }

      GWEN_NetTransport_SetPeerAddr(tr, addr);
      GWEN_InetAddr_free(addr);
      sid=GWEN_IPCManager_AddClient(cl->ipcManager,
                                    tr,
                                    userName,
                                    password,
                                    LC_CLIENT_MARK);
      if (sid==0) {
        DBG_ERROR(LC_LOGDOMAIN, "Could not add server");
        GWEN_DB_Dump(gr, stderr, 2);
        GWEN_Buffer_free(cfbuf);
        return -1;
      }

      conn=GWEN_IPCManager_GetConnection(cl->ipcManager, sid);
      assert(conn);
      GWEN_NetConnectionHTTP_SetDefaultURL(conn, "/libchipcard2/server");

      sv=LC_Server_new(sid);
      LC_Server_List_Add(sv, cl->servers);
      DBG_INFO(LC_LOGDOMAIN, "Added server");
    } /* if "server" */

    gr=GWEN_DB_GetNextGroup(gr);
  } /* while */

  GWEN_Buffer_free(cfbuf);
  return 0;
}



int LC_Client_ReadConfigFile(LC_CLIENT *cl, const char *fname){
  GWEN_BUFFER *buf;
  FILE *f;
  int found;
  GWEN_DB_NODE *dbConfig;
  int rv;

  found=0;
  buf=GWEN_Buffer_new(0, 256, 0, 1);
  if (!fname) {
    GWEN_Buffer_Reset(buf);
    GWEN_Buffer_AppendString(buf, cl->dataDir);
    GWEN_Buffer_AppendString(buf, DIRSEP "chipcardc2.conf");
    f=fopen(GWEN_Buffer_GetStart(buf), "r");
    if (f) {
      fclose(f);
      found=1;
    }

    if (!found) {
      GWEN_Buffer_Reset(buf);
      GWEN_Directory_OsifyPath(LC_DEFAULT_DATADIR, buf, 1);
      GWEN_Buffer_AppendString(buf, DIRSEP "chipcardc2.conf");
      f=fopen(GWEN_Buffer_GetStart(buf), "r");
      if (f) {
        fclose(f);
        found=1;
      }
    }
  } /* if no name given */
  else {
    GWEN_Buffer_Reset(buf);
    GWEN_Buffer_AppendString(buf, fname);
    f=fopen(GWEN_Buffer_GetStart(buf), "r");
    if (f) {
      fclose(f);
      found=1;
    }
  } /* if name given */

  if (!found) {
    DBG_ERROR(LC_LOGDOMAIN, "No configuration file found");
    GWEN_Buffer_free(buf);
    return -1;
  }

  dbConfig=GWEN_DB_Group_new("config");
  DBG_INFO(LC_LOGDOMAIN, "Reading configuration from \"%s\"",
             GWEN_Buffer_GetStart(buf));
  if (GWEN_DB_ReadFile(dbConfig,
                       GWEN_Buffer_GetStart(buf),
                       GWEN_DB_FLAGS_DEFAULT |
                       GWEN_PATH_FLAGS_CREATE_GROUP)) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not read config file \"%s\"",
              GWEN_Buffer_GetStart(buf));
    GWEN_DB_Group_free(dbConfig);
    GWEN_Buffer_free(buf);
    return -1;
  }

  rv=LC_Client_ReadConfig(cl, dbConfig);
  GWEN_DB_Group_free(dbConfig);
  GWEN_Buffer_free(buf);
  if (rv) {
    DBG_INFO(LC_LOGDOMAIN, "here");
  }
  return rv;
}



int LC_Client_SelectApp(LC_CLIENT *cl,
                        LC_CARD *cd,
                        const char *appName){
  LC_CARDCONTEXT *ctx;

  ctx=LC_CardMgr_SelectApp(cl->cardMgr, appName);
  if (!ctx) {
    DBG_ERROR(LC_LOGDOMAIN, "Unknown application \"%s\"", appName);
    return -1;
  }
  LC_Card_SetContext(cd, ctx);

  return 0;
}




LC_REQUEST *LC_Client_PeekNextRequest(LC_CLIENT *cl,
                                      GWEN_TYPE_UINT32 serverId){
  LC_REQUEST *rq;

  assert(cl);

  rq=LC_Request_List_First(cl->waitingRequests);
  while(rq) {
    if (serverId==0 || serverId==LC_Request_GetServerId(rq)) {
      return rq;
    }
    rq=LC_Request_List_Next(rq);
  } /* while */

  return 0;
}



LC_REQUEST *LC_Client_GetNextRequest(LC_CLIENT *cl,
				     GWEN_TYPE_UINT32 serverId){
  LC_REQUEST *rq;

  assert(cl);

  rq=LC_Client_PeekNextRequest(cl, serverId);
  if (rq) {
    LC_Request_List_Del(rq);
    LC_Request_List_Add(rq, cl->workingRequests);
    return rq;
  }

  return 0;
}



LC_SERVER *LC_Client_FindServer(LC_CLIENT *cl,
                                GWEN_TYPE_UINT32 serverId){
  LC_SERVER *sv;

  sv=LC_Server_List_First(cl->servers);
  while(sv) {
    if (serverId==LC_Server_GetServerId(sv))
      return sv;
    sv=LC_Server_List_Next(sv);
  } /* while */

  return 0;
}



LC_REQUEST *LC_Client_FindWaitingRequest(LC_CLIENT *cl,
                                         GWEN_TYPE_UINT32 requestId){
  LC_REQUEST *rq;

  assert(cl);

  rq=LC_Request_List_First(cl->waitingRequests);
  while(rq) {
    if (requestId==LC_Request_GetRequestId(rq)) {
      return rq;
    }
    rq=LC_Request_List_Next(rq);
  } /* while */

  return 0;
}



LC_REQUEST *LC_Client_FindWorkingRequest(LC_CLIENT *cl,
                                         GWEN_TYPE_UINT32 requestId){
  LC_REQUEST *rq;

  assert(cl);

  rq=LC_Request_List_First(cl->workingRequests);
  while(rq) {
    if (requestId==LC_Request_GetRequestId(rq)) {
      return rq;
    }
    rq=LC_Request_List_Next(rq);
  } /* while */

  return 0;
}



LC_REQUEST *LC_Client_FindRequest(LC_CLIENT *cl,
                                  GWEN_TYPE_UINT32 requestId){
  LC_REQUEST *rq;

  assert(cl);
  rq=LC_Client_FindWorkingRequest(cl, requestId);
  if (!rq)
    rq=LC_Client_FindWaitingRequest(cl, requestId);

  return rq;
}



GWEN_TYPE_UINT32 LC_Client_SendRequest(LC_CLIENT *cl,
                                       LC_CARD *card,
                                       GWEN_TYPE_UINT32 serverId,
                                       GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 rid;
  LC_REQUEST *rq;

  assert(cl);
  if (serverId==0) {
    LC_SERVER *sv;

    sv=LC_Server_List_First(cl->servers);
    rid=0;
    while(sv) {
      DBG_DEBUG(LC_LOGDOMAIN, "Creating request for server \"%08x\"",
                LC_Server_GetServerId(sv));
      rq=LC_Request_new(card, GWEN_DB_Group_dup(dbReq),
                        LC_Server_GetServerId(sv), 0);
      if (!rid)
        rid=LC_Request_GetRequestId(rq);
      else
        LC_Request_SetRequestId(rq, rid);
      LC_Request_List_Add(rq, cl->waitingRequests);
      sv=LC_Server_List_Next(sv);
    } /* while */
    GWEN_DB_Group_free(dbReq);
    if (!rid) {
      DBG_ERROR(LC_LOGDOMAIN, "No request created");
    }
    return rid;
  }
  else {
    rq=LC_Request_new(card, dbReq, serverId, 0);
    rid=LC_Request_GetRequestId(rq);
    LC_Request_List_Add(rq, cl->waitingRequests);
    return rid;
  }
}



GWEN_DB_NODE *LC_Client_GetNextResponse(LC_CLIENT *cl,
                                        GWEN_TYPE_UINT32 rqid) {
  LC_REQUEST *rq;

  assert(cl);

  /* check every request with the given id in the working list */
  rq=LC_Request_List_First(cl->workingRequests);
  while(rq) {
    if (LC_Request_GetRequestId(rq)==rqid) {
      GWEN_DB_NODE *dbRsp;

      dbRsp=
        GWEN_IPCManager_GetResponseData(cl->ipcManager,
                                        LC_Request_GetIpcRequestId(rq));
      if (dbRsp) {
        DBG_DEBUG(LC_LOGDOMAIN, "Got a response to request %08x", rqid);
        return dbRsp;
      }
    } /* if request id matches */
    rq=LC_Request_List_Next(rq);
  } /* while */

  return 0;
}



GWEN_DB_NODE *LC_Client_WaitForNextResponse(LC_CLIENT *cl,
                                            GWEN_TYPE_UINT32 rqid,
                                            int timeout) {
  time_t startt;
  int distance;
  GWEN_NETCONNECTION_WORKRESULT res;

  startt=time(0);
  assert(cl);

  /* check every request with the given id in the working list */
  if (timeout==GWEN_NETCONNECTION_TIMEOUT_NONE)
    distance=GWEN_NETCONNECTION_TIMEOUT_NONE;
  else if (timeout==GWEN_NETCONNECTION_TIMEOUT_FOREVER)
    distance=GWEN_NETCONNECTION_TIMEOUT_FOREVER;
  else {
    distance=GWEN_WaitCallback_GetDistance(0);
    if (distance)
      if ((distance/1000)>timeout)
        distance=timeout*1000;
    if (!distance)
      distance=750;
  }

  GWEN_WaitCallback_Enter(LC_CLIENT_CBID_IO_WAITRSP);
  for (;;) {
    GWEN_DB_NODE *dbRsp;

    while(1) {
      int rv;

      rv=LC_Client_Work(cl, 0);
      if (rv==-1) {
        DBG_ERROR(LC_LOGDOMAIN, "Error working on client");
        GWEN_WaitCallback_Leave();
        return 0;
      }
      else if (rv==1)
        break;
    }

    dbRsp=LC_Client_GetNextResponse(cl, rqid);
    if (dbRsp) {
      DBG_DEBUG(LC_LOGDOMAIN, "Got a response to request \"%08x\"", rqid);
      GWEN_WaitCallback_Leave();
      return dbRsp;
    }

    for (;;) {
      if (GWEN_WaitCallback()==GWEN_WaitCallbackResult_Abort) {
        DBG_ERROR(LC_LOGDOMAIN, "User aborted via waitcallback");
        GWEN_WaitCallback_Leave();
        return 0;
      }

      res=GWEN_Net_HeartBeat(distance);
      if (res==GWEN_NetConnectionWorkResult_Error) {
        DBG_ERROR(LC_LOGDOMAIN, "Error while working (%d)", res);
        GWEN_WaitCallback_Leave();
        return 0;
      }
      else if (res==GWEN_NetConnectionWorkResult_Change) {
        DBG_VERBOUS(LC_LOGDOMAIN, "Changed");
        break;
      }

      /* check timeout */
      if (timeout!=GWEN_NETCONNECTION_TIMEOUT_FOREVER) {
        if (timeout==GWEN_NETCONNECTION_TIMEOUT_NONE ||
            difftime(time(0), startt)>timeout) {
          DBG_INFO(LC_LOGDOMAIN, "Could not read within %d seconds, giving up",
                   timeout);
          GWEN_WaitCallback_Leave();
          return 0;
        }
      }
    } /* for */
  } /* for */
  GWEN_WaitCallback_Leave();

  return 0;
}



int LC_Client_DeleteRequest(LC_CLIENT *cl, GWEN_TYPE_UINT32 rqid) {
  LC_REQUEST *rq;
  int rqs;

  assert(cl);

  rqs=0;

  /* check every request with the given id in the working list */
  rq=LC_Request_List_First(cl->workingRequests);
  while(rq) {
    LC_REQUEST *next;

    next=LC_Request_List_Next(rq);

    if (LC_Request_GetRequestId(rq)==rqid) {
      LC_Request_List_Del(rq);
      LC_Request_free(rq);
      rqs++;
    } /* if request id matches */
    rq=next;
  } /* while */

  /* check every request with the given id in the waiting list */
  rq=LC_Request_List_First(cl->waitingRequests);
  while(rq) {
    LC_REQUEST *next;

    next=LC_Request_List_Next(rq);

    if (LC_Request_GetRequestId(rq)==rqid) {
      LC_Request_List_Del(rq);
      LC_Request_free(rq);
      rqs++;
    } /* if request id matches */
    rq=next;
  } /* while */

  return (rqs==0);
}



GWEN_TYPE_UINT32 LC_Client_SendStartWait(LC_CLIENT *cl,
                                         GWEN_TYPE_UINT32 rflags,
                                         GWEN_TYPE_UINT32 rmask){
  GWEN_DB_NODE *db;
  GWEN_TYPE_UINT32 rqid;

  db=GWEN_DB_Group_new("StartWait");

  /* set rflags */
  if (rflags & LC_CARD_READERFLAGS_KEYPAD)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "flags", "KEYPAD");
  if (rflags & LC_CARD_READERFLAGS_DISPLAY)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "flags", "DISPLAY");
  if (rflags & LC_CARD_READERFLAGS_NOINFO)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "flags", "NOINFO");
  if (rflags & LC_CARD_READERFLAGS_REMOTE)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "flags", "REMOTE");
  if (rflags & LC_CARD_READERFLAGS_AUTO)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "flags", "AUTO");

  /* set rmask */
  if (rmask & LC_CARD_READERFLAGS_KEYPAD)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "mask", "KEYPAD");
  if (rmask & LC_CARD_READERFLAGS_DISPLAY)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "mask", "DISPLAY");
  if (rmask & LC_CARD_READERFLAGS_NOINFO)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "mask", "NOINFO");
  if (rmask & LC_CARD_READERFLAGS_REMOTE)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "mask", "REMOTE");
  if (rmask & LC_CARD_READERFLAGS_AUTO)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT,
                         "mask", "AUTO");


  /* send request */
  rqid=LC_Client_SendRequest(cl, 0, 0, db);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    return 0;
  }

  return rqid;
}



LC_CARD *LC_Client_PeekNextCard(LC_CLIENT *cl){
  assert(cl);
  return LC_Card_List_First(cl->cards);
}



LC_CARD *LC_Client_GetNextCard(LC_CLIENT *cl){
  LC_CARD *card;

  assert(cl);
  card=LC_Card_List_First(cl->cards);
  if (card) {
    LC_Card_List_Del(card);
  }
  return card;
}



LC_CARD *LC_Client_WaitForNextCard(LC_CLIENT *cl, int timeout) {
  LC_CARD *card;
  time_t startt;
  int distance;
  GWEN_NETCONNECTION_WORKRESULT res;

  startt=time(0);
  assert(cl);

  if (timeout==GWEN_NETCONNECTION_TIMEOUT_NONE)
    distance=GWEN_NETCONNECTION_TIMEOUT_NONE;
  else if (timeout==GWEN_NETCONNECTION_TIMEOUT_FOREVER)
    distance=GWEN_NETCONNECTION_TIMEOUT_FOREVER;
  else {
    distance=GWEN_WaitCallback_GetDistance(0);
    if (distance)
      if ((distance/1000)>timeout)
        distance=timeout*1000;
    if (!distance)
      distance=750;
  }

  GWEN_WaitCallback_Enter(LC_CLIENT_CBID_IO_WAITCARD);
  for (;;) {
    DBG_VERBOUS(LC_LOGDOMAIN, "Working");
    if (LC_Client_Work(cl, 0)==-1) {
      DBG_ERROR(LC_LOGDOMAIN, "Error working on client");
      GWEN_WaitCallback_Leave();
      return 0;
    }

    card=LC_Client_GetNextCard(cl);
    if (card) {
      DBG_DEBUG(LC_LOGDOMAIN, "Got a card");
      GWEN_WaitCallback_Leave();
      return card;
    }

    for (;;) {
      if (LC_Client_Work(cl, 0)==-1) {
        DBG_ERROR(LC_LOGDOMAIN, "Error working on client");
        GWEN_WaitCallback_Leave();
        return 0;
      }


      if (GWEN_WaitCallback()==GWEN_WaitCallbackResult_Abort) {
        DBG_ERROR(LC_LOGDOMAIN, "User aborted via waitcallback");
        GWEN_WaitCallback_Leave();
        return 0;
      }

      res=GWEN_Net_HeartBeat(distance);
      if (res==GWEN_NetConnectionWorkResult_Error) {
        DBG_ERROR(LC_LOGDOMAIN, "Error while working (%d)", res);
        GWEN_WaitCallback_Leave();
        return 0;
      }
      else if (res==GWEN_NetConnectionWorkResult_Change)
        break;

      /* check timeout */
      if (timeout!=GWEN_NETCONNECTION_TIMEOUT_FOREVER) {
        if (timeout==GWEN_NETCONNECTION_TIMEOUT_NONE ||
            difftime(time(0), startt)>timeout) {
          DBG_INFO(LC_LOGDOMAIN, "Could not read within %d seconds, giving up",
                   timeout);
          GWEN_WaitCallback_Leave();
          return 0;
        }
      }
    } /* for */
  } /* for */
  GWEN_WaitCallback_Leave();

  return 0;
}



int LC_Client_HandleCardAvailable(LC_CLIENT *cl, GWEN_DB_NODE *dbReq){
  LC_CARD *card;
  GWEN_TYPE_UINT32 cardId;
  GWEN_TYPE_UINT32 serverId;
  const char *cardType;
  GWEN_TYPE_UINT32 rflags;
  GWEN_BUFFER *atr;
  const void *p;
  unsigned int bsize;
  unsigned int i;

  assert(cl);

  serverId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "body/cardid", 0, "0"),
                "%x", &cardId)) {
    DBG_ERROR(LC_LOGDOMAIN, "Bad server message");
    return -1;
  }
  cardType=GWEN_DB_GetCharValue(dbReq, "body/cardtype", 0, 0);
  assert(cardType);
  p=GWEN_DB_GetBinValue(dbReq, "body/atr", 0, 0, 0, &bsize);
  if (p && bsize) {
    atr=GWEN_Buffer_new(0, bsize+1, 0, 1);
    GWEN_Buffer_AppendBytes(atr, p, bsize);
  }
  else
    atr=0;

  rflags=0;
  for (i=0;; i++) {
    const char *s;

    s=GWEN_DB_GetCharValue(dbReq, "body/readerflags", i, 0);
    if (!s)
      break;
    if (strcasecmp(s, "KEYPAD")==0)
      rflags|=LC_CARD_READERFLAGS_KEYPAD;
    else if (strcasecmp(s, "DISPLAY")==0)
      rflags|=LC_CARD_READERFLAGS_DISPLAY;
    else if (strcasecmp(s, "NOINFO")==0)
      rflags|=LC_CARD_READERFLAGS_NOINFO;
    else if (strcasecmp(s, "REMOTE")==0)
      rflags|=LC_CARD_READERFLAGS_REMOTE;
    else if (strcasecmp(s, "AUTO")==0)
      rflags|=LC_CARD_READERFLAGS_AUTO;
    else {
      DBG_WARN(LC_LOGDOMAIN, "Unknown reader flag \"%s\"", s);
    }
  } /* for */

  card=LC_Card_new(cl,
                   cardId,
                   serverId,
                   cardType,
                   rflags,
                   atr);

  for (i=0;; i++) {
    const char *s;

    s=GWEN_DB_GetCharValue(dbReq, "body/cardTypes", i, 0);
    if (!s)
      break;
    LC_Card_AddCardType(card, s);
  }

  LC_Card_List_Add(card, cl->cards);
  DBG_INFO(LC_LOGDOMAIN, "Card added");
  return 0;
}



GWEN_TYPE_UINT32 LC_Client_SendStopWait(LC_CLIENT *cl) {
  GWEN_DB_NODE *dbReq;
  GWEN_TYPE_UINT32 rqid;

  dbReq=GWEN_DB_Group_new("StopWait");

  /* send request */
  rqid=LC_Client_SendRequest(cl, 0, 0, dbReq);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    return 0;
  }

  return rqid;
}



GWEN_TYPE_UINT32 LC_Client_SendTakeCard(LC_CLIENT *cl, LC_CARD *cd) {
  GWEN_DB_NODE *dbReq;
  GWEN_TYPE_UINT32 rqid;
  char numbuf[16];

  dbReq=GWEN_DB_Group_new("TakeCard");
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", LC_Card_GetCardId(cd));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "cardid", numbuf);

  /* send request */
  DBG_DEBUG(LC_LOGDOMAIN, "Sending take card request to %08x",
            LC_Card_GetServerId(cd));
  rqid=LC_Client_SendRequest(cl, cd, LC_Card_GetServerId(cd), dbReq);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    return 0;
  }

  return rqid;
}



GWEN_TYPE_UINT32 LC_Client_SendReleaseCard(LC_CLIENT *cl, LC_CARD *cd){
  GWEN_DB_NODE *dbReq;
  GWEN_TYPE_UINT32 rqid;
  char numbuf[16];

  dbReq=GWEN_DB_Group_new("ReleaseCard");
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", LC_Card_GetCardId(cd));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "cardid", numbuf);

  /* send request */
  rqid=LC_Client_SendRequest(cl, cd, LC_Card_GetServerId(cd), dbReq);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    return 0;
  }

  return rqid;
}



GWEN_TYPE_UINT32 LC_Client_SendCommandCard(LC_CLIENT *cl,
                                           LC_CARD *cd,
                                           const char *apdu,
                                           unsigned int len,
                                           LC_CLIENT_CMDTARGET t) {
  GWEN_DB_NODE *dbReq;
  GWEN_TYPE_UINT32 rqid;
  char numbuf[16];
  const char *s;

  assert(apdu);
  assert(len);
  dbReq=GWEN_DB_Group_new("CommandCard");
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", LC_Card_GetCardId(cd));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "cardid", numbuf);

  GWEN_DB_SetBinValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "data", apdu, len);

  switch(t) {
  case LC_Client_CmdTargetReader: s="reader"; break;
  case LC_Client_CmdTargetCard:   s="card"; break;
  default:
    DBG_ERROR(LC_LOGDOMAIN, "Unknown command target %d", t);
    return 0;
  }
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "target", s);

  /* send request */
  rqid=LC_Client_SendRequest(cl, cd, LC_Card_GetServerId(cd), dbReq);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    return 0;
  }

  return rqid;
}



GWEN_TYPE_UINT32 LC_Client_SendSelectCardApp(LC_CLIENT *cl,
                                             LC_CARD *cd,
                                             const char *cardName,
                                             const char *appName){
  GWEN_DB_NODE *dbReq;
  GWEN_TYPE_UINT32 rqid;
  char numbuf[16];
  LC_CARDCONTEXT *ctx;

  ctx=LC_CardMgr_SelectApp(cl->cardMgr, appName);
  if (!ctx) {
    DBG_ERROR(LC_LOGDOMAIN, "Unknown application \"%s\"", appName);
    return 0;
  }
  LC_Card_SetContext(cd, ctx);

  dbReq=GWEN_DB_Group_new("SelectCard");
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", LC_Card_GetCardId(cd));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "cardid", numbuf);

  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "cardName", cardName);
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "appName", appName);

  /* send request */
  rqid=LC_Client_SendRequest(cl, cd, LC_Card_GetServerId(cd), dbReq);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    LC_Card_SetContext(cd, 0);
    return 0;
  }

  return rqid;
}











LC_CLIENT_RESULT
LC_Client_CheckResponse(LC_CLIENT *cl, GWEN_TYPE_UINT32 rid){
  LC_REQUEST *rq;
  GWEN_TYPE_UINT32 count;

  assert(cl);

  count=0;
  /* check every request with the given id in the working list */
  rq=LC_Request_List_First(cl->workingRequests);
  while(rq) {
    if (LC_Request_GetRequestId(rq)==rid) {
      GWEN_DB_NODE *dbRsp;

      count++;
      dbRsp=
        GWEN_IPCManager_PeekResponseData(cl->ipcManager,
                                         LC_Request_GetIpcRequestId(rq));
      if (dbRsp) {
        DBG_DEBUG(LC_LOGDOMAIN, "Got a response to request %08x", rid);
        return LC_Client_ResultOk;
      }
      else {
        if (LC_Request_GetIsAborted(rq)) {
          DBG_ERROR(LC_LOGDOMAIN, "Request was aborted (server down?)");
          return LC_Client_ResultIpcError;
        }
      }
    } /* if request id matches */
    rq=LC_Request_List_Next(rq);
  } /* while */

  if (!count) {
    /* request not found, check whether it exists at all */
    if (!LC_Client_FindWaitingRequest(cl, rid)) {
      DBG_ERROR(LC_LOGDOMAIN, "Request not found");
      return LC_Client_ResultIpcError;
    }
  }
  return LC_Client_ResultWait;
}



LC_CLIENT_RESULT
LC_Client_CheckResponse_Wait(LC_CLIENT *cl, GWEN_TYPE_UINT32 rid,
                             int timeout){
  time_t startt;
  int distance;

  startt=time(0);
  assert(cl);

  /* check every request with the given id in the working list */
  if (timeout==GWEN_NETCONNECTION_TIMEOUT_NONE)
    distance=GWEN_NETCONNECTION_TIMEOUT_NONE;
  else if (timeout==GWEN_NETCONNECTION_TIMEOUT_FOREVER)
    distance=GWEN_NETCONNECTION_TIMEOUT_FOREVER;
  else {
    distance=GWEN_WaitCallback_GetDistance(0);
    if (distance)
      if ((distance/1000)>timeout)
        distance=timeout*1000;
    if (!distance)
      distance=750;
  }

  GWEN_WaitCallback_Enter(LC_CLIENT_CBID_IO_WAITRSP);
  for (;;) {
    GWEN_NETCONNECTION_WORKRESULT nres;
    LC_CLIENT_RESULT cres;

    if (LC_Client_Work(cl, 0)==-1) {
      DBG_ERROR(LC_LOGDOMAIN, "Error working on client");
      GWEN_WaitCallback_Leave();
      return LC_Client_ResultIpcError;
    }

    cres=LC_Client_CheckResponse(cl, rid);
    if (cres==LC_Client_ResultOk) {
      DBG_DEBUG(LC_LOGDOMAIN, "Got a response to request \"%08x\"", rid);
      GWEN_WaitCallback_Leave();
      return cres;
    }
    if (cres!=LC_Client_ResultWait){
      DBG_DEBUG(LC_LOGDOMAIN, "Leaving due to result %d", cres);
      GWEN_WaitCallback_Leave();
      return cres;
    }

    for (;;) {
      if (GWEN_WaitCallback()==GWEN_WaitCallbackResult_Abort) {
        DBG_ERROR(LC_LOGDOMAIN, "User aborted via waitcallback");
        GWEN_WaitCallback_Leave();
        return LC_Client_ResultAborted;
      }

      nres=GWEN_Net_HeartBeat(distance);
      if (nres==GWEN_NetConnectionWorkResult_Error) {
        DBG_ERROR(LC_LOGDOMAIN, "Error while working (%d)", nres);
        GWEN_WaitCallback_Leave();
        return LC_Client_ResultIpcError;
      }
      else if (nres==GWEN_NetConnectionWorkResult_Change) {
        DBG_VERBOUS(LC_LOGDOMAIN, "Changed");
        break;
      }

      /* check timeout */
      if (timeout!=GWEN_NETCONNECTION_TIMEOUT_FOREVER) {
        if (timeout==GWEN_NETCONNECTION_TIMEOUT_NONE ||
            difftime(time(0), startt)>timeout) {
          DBG_INFO(LC_LOGDOMAIN, "Could not read within %d seconds, giving up",
                   timeout);
          GWEN_WaitCallback_Leave();
          return LC_Client_ResultWait;
        }
      }
    } /* for */
  } /* for */
  GWEN_WaitCallback_Leave();

  return LC_Client_ResultIpcError;
}



LC_CLIENT_RESULT
LC_Client_CheckStartWait(LC_CLIENT *cl,
                         GWEN_TYPE_UINT32 rid){
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbRsp;
  int err;

  res=LC_Client_CheckResponse(cl, rid);
  if (res!=LC_Client_ResultOk)
    return res;

  dbRsp=LC_Client_GetNextResponse(cl, rid);
  assert(dbRsp);

  err=LC_Client_CheckForError(dbRsp);
  if (err) {
    if ((unsigned int)err>GWEN_IPC_ERROR_CODES) {
      DBG_ERROR(LC_LOGDOMAIN, "IPC error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultIpcError;
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "Command error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultCmdError;
    }
  }

  GWEN_DB_Group_free(dbRsp);
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT
LC_Client_CheckStopWait(LC_CLIENT *cl,
                        GWEN_TYPE_UINT32 rid){
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbRsp;
  int err;

  res=LC_Client_CheckResponse(cl, rid);
  if (res!=LC_Client_ResultOk)
    return res;

  dbRsp=LC_Client_GetNextResponse(cl, rid);
  assert(dbRsp);

  err=LC_Client_CheckForError(dbRsp);
  if (err) {
    if ((unsigned int)err>GWEN_IPC_ERROR_CODES) {
      DBG_ERROR(LC_LOGDOMAIN, "IPC error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultIpcError;
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "Command error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultCmdError;
    }
  }

  GWEN_DB_Group_free(dbRsp);
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT
LC_Client_CheckTakeCard(LC_CLIENT *cl,
                        GWEN_TYPE_UINT32 rid){
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbRsp;
  int err;

  res=LC_Client_CheckResponse(cl, rid);
  if (res!=LC_Client_ResultOk)
    return res;

  dbRsp=LC_Client_GetNextResponse(cl, rid);
  assert(dbRsp);

  err=LC_Client_CheckForError(dbRsp);
  if (err) {
    if (err>(int)GWEN_IPC_ERROR_CODES) {
      DBG_ERROR(LC_LOGDOMAIN, "IPC error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultIpcError;
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "Command error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultCmdError;
    }
  }

  GWEN_DB_Group_free(dbRsp);
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT
LC_Client_CheckReleaseCard(LC_CLIENT *cl,
                           GWEN_TYPE_UINT32 rid){
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbRsp;
  int err;

  res=LC_Client_CheckResponse(cl, rid);
  if (res!=LC_Client_ResultOk)
    return res;

  dbRsp=LC_Client_GetNextResponse(cl, rid);
  assert(dbRsp);

  err=LC_Client_CheckForError(dbRsp);
  if (err) {
    if (err>(int)GWEN_IPC_ERROR_CODES) {
      DBG_ERROR(LC_LOGDOMAIN, "IPC error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultIpcError;
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "Command error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultCmdError;
    }
  }

  GWEN_DB_Group_free(dbRsp);
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT
LC_Client_CheckCommandCard(LC_CLIENT *cl,
                           GWEN_TYPE_UINT32 rid,
                           GWEN_BUFFER *data){
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbRsp;
  int err;
  const void *bp;
  unsigned int bs;
  const char *s;
  const char *txt;
  LC_REQUEST *rq;
  LC_CARD *card;

  rq=LC_Client_FindWorkingRequest(cl, rid);
  if (!rq) {
    if (LC_Client_FindWaitingRequest(cl, rid)) {
      DBG_INFO(LC_LOGDOMAIN, "Request not yet sent");
      return LC_Client_ResultWait;
    }
    DBG_ERROR(LC_LOGDOMAIN, "Request not found");
    return LC_Client_ResultIpcError;
  }
  card=LC_Request_GetCard(rq);
  assert(card);

  res=LC_Client_CheckResponse(cl, rid);
  if (res!=LC_Client_ResultOk)
    return res;

  dbRsp=LC_Client_GetNextResponse(cl, rid);
  assert(dbRsp);

  err=LC_Client_CheckForError(dbRsp);
  if (err) {
    if (err>(int)GWEN_IPC_ERROR_CODES) {
      DBG_ERROR(LC_LOGDOMAIN, "IPC error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultIpcError;
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "Command error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultCmdError;
    }
  }

  txt=GWEN_DB_GetCharValue(dbRsp, "body/text", 0, "");
  s=GWEN_DB_GetCharValue(dbRsp, "body/code", 0, "ERROR");
  if (strcasecmp(s, "OK")!=0) {
    DBG_ERROR(LC_LOGDOMAIN, "Command error (%s)", txt);
    LC_Card_SetLastResult(card, "error", txt, -1, -1);
    GWEN_DB_Group_free(dbRsp);
    return LC_Client_ResultCmdError;
  }

  bp=GWEN_DB_GetBinValue(dbRsp, "body/data", 0, 0, 0, &bs);
  if (bp && bs>1) {
    LC_Card_SetLastResult(card, "ok",
                          txt,
                          ((const unsigned char*)bp)[bs-2],
                          ((const unsigned char*)bp)[bs-1]);
    GWEN_Buffer_AppendBytes(data, bp, bs);
  }
  GWEN_DB_Group_free(dbRsp);
  return LC_Client_ResultOk;
}



GWEN_TYPE_UINT32 LC_Client_SendExecCommand(LC_CLIENT *cl,
                                           LC_CARD *cd,
                                           GWEN_DB_NODE *dbCmd){
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbExecCommand;
  GWEN_TYPE_UINT32 rqid;
  char numbuf[16];
  LC_CARDCONTEXT *ctx;
  LC_CARDMGR_RESULT res;

  ctx=LC_Card_GetContext(cd);
  if (!ctx) {
    DBG_ERROR(LC_LOGDOMAIN, "No application selected");
    return 0;
  }
  dbReq=GWEN_DB_Group_new("ExecCommand");
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", LC_Card_GetCardId(cd));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "cardid", numbuf);
  dbExecCommand=GWEN_DB_GetGroup(dbReq, GWEN_DB_FLAGS_DEFAULT,
                                 "command");
  assert(dbExecCommand);
  GWEN_DB_AddGroupChildren(dbExecCommand, dbCmd);
  GWEN_DB_SetCharValue(dbExecCommand, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "name", GWEN_DB_GroupName(dbCmd));

  /* translate command */
  res=LC_CardContext_Translate(ctx, dbExecCommand);
  if (res!=LC_CardMgr_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    GWEN_DB_Group_free(dbReq);
    return 0;
  }

  /* send request */
  rqid=LC_Client_SendRequest(cl, cd, LC_Card_GetServerId(cd), dbReq);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    return 0;
  }

  return rqid;
}



LC_CLIENT_RESULT
LC_Client_CheckExecCommand(LC_CLIENT *cl,
                           GWEN_TYPE_UINT32 rid,
                           GWEN_DB_NODE *dbAnswer){
  LC_CLIENT_RESULT res;
  LC_REQUEST *rq;
  GWEN_DB_NODE *dbReq;
  GWEN_DB_NODE *dbRsp;
  GWEN_DB_NODE *dbCmdRsp;
  int err;
  LC_CARDMGR_RESULT ccres;
  LC_CARD *card;
  const char *resultText;
  const char *resultTyp;
  int sw1, sw2;

  rq=LC_Client_FindWorkingRequest(cl, rid);
  if (!rq) {
    if (LC_Client_FindWaitingRequest(cl, rid)) {
      DBG_INFO(LC_LOGDOMAIN, "Request not yet sent");
      return LC_Client_ResultWait;
    }
    DBG_ERROR(LC_LOGDOMAIN, "Request not found");
    return LC_Client_ResultIpcError;
  }
  card=LC_Request_GetCard(rq);
  assert(card);
  dbReq=LC_Request_GetRequestData(rq);
  assert(dbReq);

  dbReq=GWEN_DB_GetGroup(dbReq, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "command");
  assert(dbReq);

  res=LC_Client_CheckResponse(cl, rid);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "here");
    return res;
  }

  dbRsp=LC_Client_GetNextResponse(cl, rid);
  assert(dbRsp);

  err=LC_Client_CheckForError(dbRsp);
  if (err) {
    if (err>(int)GWEN_IPC_ERROR_CODES) {
      DBG_ERROR(LC_LOGDOMAIN, "IPC error %08x", err);
      LC_Card_SetLastResult(card, "error", "IPC error", -1, -1);
      return LC_Client_ResultIpcError;
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "Command error %08x", err);
      LC_Card_SetLastResult(card, "error", "Command error", -1, -1);
      return LC_Client_ResultCmdError;
    }
  }

  dbCmdRsp=GWEN_DB_GetGroup(dbRsp, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                            "body/command");
  if (!dbCmdRsp) {
    DBG_ERROR(LC_LOGDOMAIN, "Bad response");
    LC_Card_SetLastResult(card, "error", "Bad response", -1, -1);
    return LC_Client_ResultCmdError;
  }

  /* check response */
  ccres=LC_CardContext_CheckResponse(LC_Card_GetContext(card),
                                     dbReq,
                                     dbCmdRsp);
  if (ccres!=LC_CardMgr_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here");
    LC_Card_SetLastResult(card, "error", "Error in response", -1, -1);
    return LC_Client_ResultCmdError;
  }

  resultTyp=GWEN_DB_GetCharValue(dbCmdRsp, "result/type", 0, "error");
  resultText=GWEN_DB_GetCharValue(dbCmdRsp, "result/text", 0, 0);
  sw1=GWEN_DB_GetIntValue(dbCmdRsp, "result/sw1", 0, -1);
  sw2=GWEN_DB_GetIntValue(dbCmdRsp, "result/sw2", 0, -1);
  LC_Card_SetLastResult(card, resultTyp, resultText, sw1, sw2);

  if (strcasecmp(resultTyp, "success")==0) {
    DBG_INFO(LC_LOGDOMAIN, "Result: %s (%s)", resultTyp, resultText);
  }
  else {
    if (sw1!=-1 && sw2!=-1) {
      DBG_INFO(LC_LOGDOMAIN, "Result: %s, SW1=%02x, SW2=%02x (%s)",
               resultTyp,
               sw1, sw2,
               resultText);
    }
    return LC_Client_ResultCmdError;
  }

  GWEN_DB_UnlinkGroup(dbCmdRsp);
  GWEN_DB_Group_free(dbRsp);
  GWEN_DB_AddGroup(dbAnswer, dbCmdRsp);
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT
LC_Client_CheckSelectCardApp(LC_CLIENT *cl,
                             GWEN_TYPE_UINT32 rid){
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbRsp;
  int err;

  res=LC_Client_CheckResponse(cl, rid);
  if (res!=LC_Client_ResultOk)
    return res;

  dbRsp=LC_Client_GetNextResponse(cl, rid);
  assert(dbRsp);

  err=LC_Client_CheckForError(dbRsp);
  if (err) {
    if (err>(int)GWEN_IPC_ERROR_CODES) {
      DBG_ERROR(LC_LOGDOMAIN, "IPC error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultIpcError;
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "Command error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultCmdError;
    }
  }

  GWEN_DB_Group_free(dbRsp);
  return LC_Client_ResultOk;
}





LC_CLIENT_RESULT LC_Client_StartWait(LC_CLIENT *cl,
                                     GWEN_TYPE_UINT32 rflags,
                                     GWEN_TYPE_UINT32 rmask) {
  GWEN_TYPE_UINT32 rqid;
  LC_CLIENT_RESULT res;

  rqid=LC_Client_SendStartWait(cl, rflags, rmask);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"startWait\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, cl->shortTimeout);
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
      LC_Client_DeleteRequest(cl, rqid);
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"startWait\"");
    }
    return res;
  }
  res=LC_Client_CheckStartWait(cl, rqid);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"startWait\"");
    return LC_Client_ResultCmdError;
  }

  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_Client_StopWait(LC_CLIENT *cl) {
  GWEN_TYPE_UINT32 rqid;
  LC_CLIENT_RESULT res;

  rqid=LC_Client_SendStopWait(cl);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"stopWait\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, cl->shortTimeout);
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
      LC_Client_DeleteRequest(cl, rqid);
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"stopWait\"");
    }
    return res;
  }
  res=LC_Client_CheckStopWait(cl, rqid);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"stopWait\"");
    return LC_Client_ResultCmdError;
  }

  return LC_Client_ResultOk;
}



GWEN_TYPE_UINT32 LC_Client_SendSetNotify(LC_CLIENT *cl,
					 GWEN_TYPE_UINT32 flags){
  GWEN_DB_NODE *db;
  GWEN_TYPE_UINT32 rqid;

  db=GWEN_DB_Group_new("SetNotify");

  /* driver flags */
  if (flags & LC_NOTIFY_FLAGS_DRIVER_START)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_DRIVER":"LC_NOTIFY_CODE_DRIVER_START);
  if (flags & LC_NOTIFY_FLAGS_DRIVER_UP)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_DRIVER":"LC_NOTIFY_CODE_DRIVER_UP);
  if (flags & LC_NOTIFY_FLAGS_DRIVER_DOWN)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_DRIVER":"LC_NOTIFY_CODE_DRIVER_DOWN);
  if (flags & LC_NOTIFY_FLAGS_DRIVER_ERROR)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_DRIVER":"LC_NOTIFY_CODE_DRIVER_ERROR);

  /* reader flags */
  if (flags & LC_NOTIFY_FLAGS_READER_START)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_READER":"LC_NOTIFY_CODE_READER_START);
  if (flags & LC_NOTIFY_FLAGS_READER_UP)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_READER":"LC_NOTIFY_CODE_READER_UP);
  if (flags & LC_NOTIFY_FLAGS_READER_DOWN)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_READER":"LC_NOTIFY_CODE_READER_DOWN);
  if (flags & LC_NOTIFY_FLAGS_READER_ERROR)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_READER":"LC_NOTIFY_CODE_READER_ERROR);

  /* service flags */
  if (flags & LC_NOTIFY_FLAGS_SERVICE_START)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_SERVICE":"LC_NOTIFY_CODE_SERVICE_START);
  if (flags & LC_NOTIFY_FLAGS_SERVICE_UP)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_SERVICE":"LC_NOTIFY_CODE_SERVICE_UP);
  if (flags & LC_NOTIFY_FLAGS_SERVICE_DOWN)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_SERVICE":"LC_NOTIFY_CODE_SERVICE_DOWN);
  if (flags & LC_NOTIFY_FLAGS_SERVICE_ERROR)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_SERVICE":"LC_NOTIFY_CODE_SERVICE_ERROR);

  /* card flags */
  if (flags & LC_NOTIFY_FLAGS_CARD_INSERTED)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_CARD":"LC_NOTIFY_CODE_CARD_INSERTED);
  if (flags & LC_NOTIFY_FLAGS_CARD_REMOVED)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_CARD":"LC_NOTIFY_CODE_CARD_REMOVED);

  /* client flags */
  if (flags & LC_NOTIFY_FLAGS_CLIENT_UP)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_CLIENT":"LC_NOTIFY_CODE_CLIENT_UP);
  if (flags & LC_NOTIFY_FLAGS_CLIENT_DOWN)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_CLIENT":"LC_NOTIFY_CODE_CLIENT_DOWN);
  if (flags & LC_NOTIFY_FLAGS_CLIENT_STARTWAIT)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_CLIENT":"LC_NOTIFY_CODE_CLIENT_STARTWAIT);
  if (flags & LC_NOTIFY_FLAGS_CLIENT_STOPWAIT)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_CLIENT":"LC_NOTIFY_CODE_CLIENT_STOPWAIT);
  if (flags & LC_NOTIFY_FLAGS_CLIENT_TAKECARD)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_CLIENT":"
			 LC_NOTIFY_CODE_CLIENT_TAKECARD);
  if (flags & LC_NOTIFY_FLAGS_CLIENT_GOTCARD)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_CLIENT":"
			 LC_NOTIFY_CODE_CLIENT_GOTCARD);
  if (flags & LC_NOTIFY_FLAGS_CLIENT_CMDSEND)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_CLIENT":"
			 LC_NOTIFY_CODE_CLIENT_CMDSEND);
  if (flags & LC_NOTIFY_FLAGS_CLIENT_CMDRECV)
    GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_DEFAULT, "flag",
			 LC_NOTIFY_TYPE_CLIENT":"
			 LC_NOTIFY_CODE_CLIENT_CMDRECV);


  /* send request */
  rqid=LC_Client_SendRequest(cl, 0, 0, db);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    return 0;
  }

  return rqid;

}



LC_CLIENT_RESULT LC_Client_CheckSetNotify(LC_CLIENT *cl,
					  GWEN_TYPE_UINT32 rid){
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbRsp;
  int err;

  res=LC_Client_CheckResponse(cl, rid);
  if (res!=LC_Client_ResultOk)
    return res;

  dbRsp=LC_Client_GetNextResponse(cl, rid);
  assert(dbRsp);

  err=LC_Client_CheckForError(dbRsp);
  if (err) {
    if (err>(int)GWEN_IPC_ERROR_CODES) {
      DBG_ERROR(LC_LOGDOMAIN, "IPC error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultIpcError;
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "Command error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultCmdError;
    }
  }

  GWEN_DB_Group_free(dbRsp);
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_Client_SetNotify(LC_CLIENT *cl, GWEN_TYPE_UINT32 flags){
  GWEN_TYPE_UINT32 rqid;
  LC_CLIENT_RESULT res;

  rqid=LC_Client_SendSetNotify(cl, flags);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"setNotify\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, cl->shortTimeout);
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
      LC_Client_DeleteRequest(cl, rqid);
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"setNotify\"");
    }
    return res;
  }
  res=LC_Client_CheckSetNotify(cl, rqid);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"setNotify\"");
    return LC_Client_ResultCmdError;
  }

  return LC_Client_ResultOk;
}



int LC_Client_HandleNotification(LC_CLIENT *cl, GWEN_DB_NODE *dbReq){
  GWEN_TYPE_UINT32 serverId;
  const char *clientId;
  const char *ntype;
  const char *ncode;
  GWEN_DB_NODE *dbData;
  LC_NOTIFICATION *n;

  assert(cl);

  serverId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  clientId=GWEN_DB_GetCharValue(dbReq, "body/clientid", 0, "0");

  ntype=GWEN_DB_GetCharValue(dbReq, "body/ntype", 0, 0);
  ncode=GWEN_DB_GetCharValue(dbReq, "body/ncode", 0, 0);
  dbData=GWEN_DB_GetGroup(dbReq, GWEN_PATH_FLAGS_NAMEMUSTEXIST, "body/data");
  if (!ntype) {
    DBG_ERROR(0, "Bad server message (no ntype)");
    return -1;
  }
  if (!ncode) {
    DBG_ERROR(0, "Bad server message (no ncode)");
    return -1;
  }

  n=LC_Notification_new(serverId, clientId, ntype, ncode, dbData);
  assert(n);
  if (LCM_Monitor_HandleNotification(cl->monitor, n)) {
    DBG_INFO(LC_LOGDOMAIN, "Error handling notification");
  }
  LC_Notification_free(n);
  return 0;
}





GWEN_TYPE_UINT32 LC_Client_SendGetDriverVar(LC_CLIENT *cl,
                                            LC_CARD *cd,
                                            const char *vname) {
  GWEN_DB_NODE *dbReq;
  GWEN_TYPE_UINT32 rqid;
  char numbuf[16];
  LC_CARDCONTEXT *ctx;

  ctx=LC_Card_GetContext(cd);
  if (!ctx) {
    DBG_ERROR(LC_LOGDOMAIN, "No application selected");
    return 0;
  }
  dbReq=GWEN_DB_Group_new("GetDriverVar");
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", LC_Card_GetCardId(cd));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "cardid", numbuf);
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "varName", vname);

  /* send request */
  rqid=LC_Client_SendRequest(cl, cd, LC_Card_GetServerId(cd), dbReq);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    return 0;
  }

  return rqid;
}



LC_CLIENT_RESULT
LC_Client_CheckGetDriverVar(LC_CLIENT *cl,
                            GWEN_TYPE_UINT32 rid,
                            GWEN_BUFFER *vbuf){
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbRsp;
  int err;
  const char *value;

  res=LC_Client_CheckResponse(cl, rid);
  if (res!=LC_Client_ResultOk)
    return res;

  dbRsp=LC_Client_GetNextResponse(cl, rid);
  assert(dbRsp);

  err=LC_Client_CheckForError(dbRsp);
  if (err) {
    if (err>(int)GWEN_IPC_ERROR_CODES) {
      DBG_ERROR(LC_LOGDOMAIN, "IPC error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultIpcError;
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "Command error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultCmdError;
    }
  }

  value=GWEN_DB_GetCharValue(dbRsp, "body/varValue", 0, 0);
  if (value) {
    DBG_DEBUG(LC_LOGDOMAIN, "Got value: %s", value);
    GWEN_Buffer_AppendString(vbuf, value);
  }

  GWEN_DB_Group_free(dbRsp);
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_Client_GetDriverVar(LC_CLIENT *cl,
                                        LC_CARD *card,
                                        const char *vname,
                                        GWEN_BUFFER *vbuf){
  GWEN_TYPE_UINT32 rqid;
  LC_CLIENT_RESULT res;

  rqid=LC_Client_SendGetDriverVar(cl, card, vname);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"GetDriverVar\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, cl->shortTimeout);
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
      LC_Client_DeleteRequest(cl, rqid);
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"GetDriverVar\"");
    }
    return res;
  }
  res=LC_Client_CheckGetDriverVar(cl, rqid, vbuf);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"GetDriverVar\"");
    return LC_Client_ResultCmdError;
  }

  return LC_Client_ResultOk;
}



GWEN_TYPE_UINT32 LC_Client_SendCardCheck(LC_CLIENT *cl,
                                         LC_CARD *cd){
  GWEN_DB_NODE *dbReq;
  GWEN_TYPE_UINT32 rqid;
  char numbuf[16];

  dbReq=GWEN_DB_Group_new("CardCheck");
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", LC_Card_GetCardId(cd));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "cardid", numbuf);

  /* send request */
  rqid=LC_Client_SendRequest(cl, cd, LC_Card_GetServerId(cd), dbReq);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    return 0;
  }

  return rqid;
}



LC_CLIENT_RESULT LC_Client_CheckCardCheck(LC_CLIENT *cl,
                                          GWEN_TYPE_UINT32 rid){
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbRsp;
  int err;
  const char *code;
  const char *text;

  res=LC_Client_CheckResponse(cl, rid);
  if (res!=LC_Client_ResultOk)
    return res;

  dbRsp=LC_Client_GetNextResponse(cl, rid);
  assert(dbRsp);

  err=LC_Client_CheckForError(dbRsp);
  if (err) {
    if (err>(int)GWEN_IPC_ERROR_CODES) {
      DBG_ERROR(LC_LOGDOMAIN, "IPC error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultIpcError;
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "Command error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultCmdError;
    }
  }

  code=GWEN_DB_GetCharValue(dbRsp, "body/code", 0, "ERROR");
  text=GWEN_DB_GetCharValue(dbRsp, "body/text", 0, "(none)");
  DBG_DEBUG(LC_LOGDOMAIN, "CardCheck result: %s (%s)", code, text);
  if (strcasecmp(code, "OK"))
    res=LC_Client_ResultOk;
  else if (strcasecmp(code, "REMOVED")){
    res=LC_Client_ResultCardRemoved;
  }
  else {
    res=LC_Client_ResultGeneric;
  }
  GWEN_DB_Group_free(dbRsp);
  return res;
}



LC_CLIENT_RESULT LC_Client_CardCheck(LC_CLIENT *cl, LC_CARD *card){
  GWEN_TYPE_UINT32 rqid;
  LC_CLIENT_RESULT res;

  rqid=LC_Client_SendCardCheck(cl, card);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"CardCheck\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, cl->shortTimeout);
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
      LC_Client_DeleteRequest(cl, rqid);
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"CardCheck\"");
    }
    return res;
  }
  res=LC_Client_CheckCardCheck(cl, rqid);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"CardCheck\"");
    return LC_Client_ResultCmdError;
  }

  return LC_Client_ResultOk;
}



GWEN_TYPE_UINT32 LC_Client_SendCardReset(LC_CLIENT *cl, LC_CARD *cd){
  GWEN_DB_NODE *dbReq;
  GWEN_TYPE_UINT32 rqid;
  char numbuf[16];

  dbReq=GWEN_DB_Group_new("CardReset");
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", LC_Card_GetCardId(cd));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "cardid", numbuf);

  /* send request */
  rqid=LC_Client_SendRequest(cl, cd, LC_Card_GetServerId(cd), dbReq);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    return 0;
  }

  return rqid;
}



LC_CLIENT_RESULT LC_Client_CheckCardReset(LC_CLIENT *cl,
                                          GWEN_TYPE_UINT32 rid){
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbRsp;
  int err;
  const char *code;
  const char *text;

  res=LC_Client_CheckResponse(cl, rid);
  if (res!=LC_Client_ResultOk)
    return res;

  dbRsp=LC_Client_GetNextResponse(cl, rid);
  assert(dbRsp);

  err=LC_Client_CheckForError(dbRsp);
  if (err) {
    if (err>(int)GWEN_IPC_ERROR_CODES) {
      DBG_ERROR(LC_LOGDOMAIN, "IPC error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultIpcError;
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "Command error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultCmdError;
    }
  }

  code=GWEN_DB_GetCharValue(dbRsp, "body/code", 0, "ERROR");
  text=GWEN_DB_GetCharValue(dbRsp, "body/text", 0, "(none)");
  DBG_DEBUG(LC_LOGDOMAIN, "CardReset result: %s (%s)", code, text);
  if (strcasecmp(code, "OK"))
    res=LC_Client_ResultOk;
  else
    res=LC_Client_ResultGeneric;
  GWEN_DB_Group_free(dbRsp);
  return res;
}



LC_CLIENT_RESULT LC_Client_CardReset(LC_CLIENT *cl,
				     LC_CARD *card){
  GWEN_TYPE_UINT32 rqid;
  LC_CLIENT_RESULT res;

  rqid=LC_Client_SendCardReset(cl, card);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"CardReset\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, cl->shortTimeout);
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
      LC_Client_DeleteRequest(cl, rqid);
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"CardReset\"");
    }
    return res;
  }
  res=LC_Client_CheckCardReset(cl, rqid);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"CardReset\"");
    return LC_Client_ResultCmdError;
  }

  return LC_Client_ResultOk;
}







GWEN_TYPE_UINT32 LC_Client_SendOpenService(LC_CLIENT *cl,
                                           GWEN_TYPE_UINT32 serverId,
                                           GWEN_TYPE_UINT32 svid,
                                           GWEN_DB_NODE *dbData){
  GWEN_DB_NODE *dbReq;
  GWEN_TYPE_UINT32 rqid;
  char numbuf[16];

  assert(cl);
  assert(serverId);
  assert(svid);
  dbReq=GWEN_DB_Group_new("OpenService");
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", svid);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "serviceId", numbuf);
  if (dbData) {
    GWEN_DB_NODE *dbOpenCommand;

    dbOpenCommand=GWEN_DB_GetGroup(dbReq, GWEN_DB_FLAGS_DEFAULT,
                                   "command");
    GWEN_DB_AddGroupChildren(dbOpenCommand, dbData);
  }

  /* send request */
  rqid=LC_Client_SendRequest(cl, 0, serverId, dbReq);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    return 0;
  }

  return rqid;
}



LC_CLIENT_RESULT
LC_Client_CheckOpenService(LC_CLIENT *cl,
                           GWEN_TYPE_UINT32 rid){
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbRsp;
  int err;

  res=LC_Client_CheckResponse(cl, rid);
  if (res!=LC_Client_ResultOk)
    return res;

  dbRsp=LC_Client_GetNextResponse(cl, rid);
  assert(dbRsp);

  err=LC_Client_CheckForError(dbRsp);
  if (err) {
    if (err>(int)GWEN_IPC_ERROR_CODES) {
      DBG_ERROR(LC_LOGDOMAIN, "IPC error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultIpcError;
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "Command error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultCmdError;
    }
  }

  GWEN_DB_Group_free(dbRsp);
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT
LC_Client_OpenService(LC_CLIENT *cl,
                      GWEN_TYPE_UINT32 serverId,
                      GWEN_TYPE_UINT32 svid,
                      GWEN_DB_NODE *dbData){
  GWEN_TYPE_UINT32 rqid;
  LC_CLIENT_RESULT res;

  assert(cl);
  assert(serverId);
  assert(svid);
  rqid=LC_Client_SendOpenService(cl, serverId, svid, dbData);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"OpenService\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, cl->shortTimeout);
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
      LC_Client_DeleteRequest(cl, rqid);
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"OpenService\"");
      LC_Client_DeleteRequest(cl, rqid);
    }
    return res;
  }
  res=LC_Client_CheckOpenService(cl, rqid);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"OpenService\"");
    LC_Client_DeleteRequest(cl, rqid);
    return LC_Client_ResultCmdError;
  }

  LC_Client_DeleteRequest(cl, rqid);
  return LC_Client_ResultOk;
}



GWEN_TYPE_UINT32 LC_Client_SendCloseService(LC_CLIENT *cl,
                                            GWEN_TYPE_UINT32 serverId,
                                            GWEN_TYPE_UINT32 svid,
                                            GWEN_DB_NODE *dbData){
  GWEN_DB_NODE *dbReq;
  GWEN_TYPE_UINT32 rqid;
  char numbuf[16];

  assert(cl);
  assert(serverId);
  assert(svid);
  dbReq=GWEN_DB_Group_new("CloseService");
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", svid);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "serviceId", numbuf);
  if (dbData) {
    GWEN_DB_NODE *dbCloseCommand;

    dbCloseCommand=GWEN_DB_GetGroup(dbReq, GWEN_DB_FLAGS_DEFAULT,
                                   "command");
    GWEN_DB_AddGroupChildren(dbCloseCommand, dbData);
  }

  /* send request */
  rqid=LC_Client_SendRequest(cl, 0, serverId, dbReq);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    return 0;
  }

  return rqid;
}



LC_CLIENT_RESULT LC_Client_CheckCloseService(LC_CLIENT *cl,
                                             GWEN_TYPE_UINT32 rid){
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbRsp;
  int err;

  res=LC_Client_CheckResponse(cl, rid);
  if (res!=LC_Client_ResultOk)
    return res;

  dbRsp=LC_Client_GetNextResponse(cl, rid);
  assert(dbRsp);

  err=LC_Client_CheckForError(dbRsp);
  if (err) {
    if (err>(int)GWEN_IPC_ERROR_CODES) {
      DBG_ERROR(LC_LOGDOMAIN, "IPC error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultIpcError;
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "Command error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultCmdError;
    }
  }

  GWEN_DB_Group_free(dbRsp);
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_Client_CloseService(LC_CLIENT *cl,
                                        GWEN_TYPE_UINT32 serverId,
                                        GWEN_TYPE_UINT32 svid,
                                        GWEN_DB_NODE *dbData){
  GWEN_TYPE_UINT32 rqid;
  LC_CLIENT_RESULT res;

  assert(cl);
  assert(serverId);
  assert(svid);
  rqid=LC_Client_SendCloseService(cl, serverId, svid, dbData);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"CloseService\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, cl->shortTimeout);
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
      LC_Client_DeleteRequest(cl, rqid);
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"CloseService\"");
      LC_Client_DeleteRequest(cl, rqid);
    }
    return res;
  }
  res=LC_Client_CheckCloseService(cl, rqid);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"CloseService\"");
    LC_Client_DeleteRequest(cl, rqid);
    return LC_Client_ResultCmdError;
  }

  LC_Client_DeleteRequest(cl, rqid);
  return LC_Client_ResultOk;
}



GWEN_TYPE_UINT32 LC_Client_SendServiceCommand(LC_CLIENT *cl,
                                              GWEN_TYPE_UINT32 serverId,
                                              GWEN_TYPE_UINT32 svid,
                                              GWEN_DB_NODE *dbData){
  GWEN_DB_NODE *dbReq;
  GWEN_TYPE_UINT32 rqid;
  char numbuf[16];

  assert(cl);
  assert(serverId);
  assert(svid);
  dbReq=GWEN_DB_Group_new("ServiceCommand");
  snprintf(numbuf, sizeof(numbuf)-1, "%08x", svid);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "serviceId", numbuf);
  if (dbData) {
    GWEN_DB_NODE *dbCommandCommand;

    dbCommandCommand=GWEN_DB_GetGroup(dbReq, GWEN_DB_FLAGS_DEFAULT,
                                   "command");
    GWEN_DB_AddGroupChildren(dbCommandCommand, dbData);
  }

  /* send request */
  rqid=LC_Client_SendRequest(cl, 0, serverId, dbReq);
  if (rqid==0) {
    DBG_INFO(LC_LOGDOMAIN, "Error sending request");
    return 0;
  }

  return rqid;
}



LC_CLIENT_RESULT LC_Client_CheckServiceCommand(LC_CLIENT *cl,
                                               GWEN_TYPE_UINT32 rid,
                                               GWEN_DB_NODE *dbCmdResp){
  LC_CLIENT_RESULT res;
  GWEN_DB_NODE *dbRsp;
  int err;

  res=LC_Client_CheckResponse(cl, rid);
  if (res!=LC_Client_ResultOk)
    return res;

  dbRsp=LC_Client_GetNextResponse(cl, rid);
  assert(dbRsp);

  err=LC_Client_CheckForError(dbRsp);
  if (err) {
    if (err>(int)GWEN_IPC_ERROR_CODES) {
      DBG_ERROR(LC_LOGDOMAIN, "IPC error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultIpcError;
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "Command error %08x", err);
      GWEN_DB_Group_free(dbRsp);
      return LC_Client_ResultCmdError;
    }
  }
  else {
    if (dbCmdResp) {
      GWEN_DB_NODE *dbAnswer;

      dbAnswer=GWEN_DB_GetGroup(dbRsp, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                                "body/command");
      if (dbAnswer)
        GWEN_DB_AddGroupChildren(dbCmdResp, dbAnswer);
    }
  }

  GWEN_DB_Group_free(dbRsp);
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_Client_ServiceCommand(LC_CLIENT *cl,
                                          GWEN_TYPE_UINT32 serverId,
                                          GWEN_TYPE_UINT32 svid,
                                          GWEN_DB_NODE *dbData,
                                          GWEN_DB_NODE *dbCmdResp) {
  GWEN_TYPE_UINT32 rqid;
  LC_CLIENT_RESULT res;

  assert(cl);
  assert(serverId);
  assert(svid);
  rqid=LC_Client_SendServiceCommand(cl, serverId, svid, dbData);
  if (rqid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send request \"ServiceCommand\"");
    return LC_Client_ResultIpcError;
  }
  res=LC_Client_CheckResponse_Wait(cl, rqid, cl->shortTimeout);
  if (res!=LC_Client_ResultOk) {
    if (res==LC_Client_ResultAborted) {
      DBG_ERROR(LC_LOGDOMAIN, "User aborted");
      LC_Client_DeleteRequest(cl, rqid);
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "No response for request \"ServiceCommand\"");
      LC_Client_DeleteRequest(cl, rqid);
    }
    return res;
  }
  res=LC_Client_CheckServiceCommand(cl, rqid, dbCmdResp);
  if (res!=LC_Client_ResultOk) {
    DBG_ERROR(LC_LOGDOMAIN, "Error response for request \"ServiceCommand\"");
    LC_Client_DeleteRequest(cl, rqid);
    return LC_Client_ResultCmdError;
  }

  LC_Client_DeleteRequest(cl, rqid);
  return LC_Client_ResultOk;
}



void LC_Client_SetHandleInRequestFn(LC_CLIENT *cl,
                                    LC_CLIENT_HANDLE_INREQUEST_FN fn){
  assert(cl);
  cl->handleInRequestFn=fn;
}



void LC_Client_SetServerDownFn(LC_CLIENT *cl,
                               LC_CLIENT_SERVER_DOWN_FN fn){
  assert(cl);
  cl->serverDownFn=fn;
}



int LC_Client_HandleInRequest(LC_CLIENT *cl,
                              GWEN_TYPE_UINT32 rid,
                              GWEN_DB_NODE *dbReq){
  assert(cl);
  if (cl->handleInRequestFn)
    return cl->handleInRequestFn(cl, rid, dbReq);
  else {
    /* not handled */
    return 1;
  }
}



int LC_Client_SendResponse(LC_CLIENT *cl,
                           GWEN_TYPE_UINT32 rid,
                           GWEN_DB_NODE *dbCommand) {
  assert(cl);
  return GWEN_IPCManager_SendResponse(cl->ipcManager,
                                      rid,
                                      dbCommand);
}



void LC_Client_RemoveInRequest(LC_CLIENT *cl, GWEN_TYPE_UINT32 rid){
  assert(cl);
  GWEN_IPCManager_RemoveRequest(cl->ipcManager, rid, 0);
}




