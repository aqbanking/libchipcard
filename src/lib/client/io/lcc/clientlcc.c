/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: card.c 187 2006-06-15 16:13:23Z martin $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "clientlcc_p.h"
#include "cardlcc_l.h"
#include <chipcard/client/card.h>
#include <chipcard/client/mon/monitor.h>

#include <gwenhywfar/gwenhywfar.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/version.h>
#include <gwenhywfar/directory.h>
#include <gwenhywfar/net2.h>
#include <gwenhywfar/waitcallback.h>
#include <gwenhywfar/inetsocket.h>

#include <unistd.h>

#define I18N(msg) msg


GWEN_INHERIT(LC_CLIENT, LC_CLIENT_LCC)



LC_CLIENT *LC_ClientLcc_new(const char *programName,
                            const char *programVersion) {
  LC_CLIENT *cl;
  LC_CLIENT_LCC *xcl;

  cl=LC_BaseClient_new(LC_CLIENT_LCC_NAME, programName, programVersion);
  if (cl==0)
    return cl;

  GWEN_NEW_OBJECT(LC_CLIENT_LCC, xcl);
  GWEN_INHERIT_SETDATA(LC_CLIENT, LC_CLIENT_LCC, cl, xcl,
                       LC_ClientLcc_FreeData);

  xcl->servers=LC_Server_List_new();
  xcl->waitingRequests=LC_Request_List_new();
  xcl->workingRequests=LC_Request_List_new();
  xcl->cards=LC_Card_List_new();
  xcl->ipcManager=GWEN_IpcManager_new();
  xcl->askAddCertResult=GWEN_NetLayerSsl_AskAddCertResult_Incoming;

  xcl->initFn=LC_Client_SetInitFn(cl, LC_ClientLcc_V_Init);
  xcl->finiFn=LC_Client_SetFiniFn(cl, LC_ClientLcc_V_Fini);
  LC_Client_SetStartFn(cl, LC_ClientLcc_V_Start);
  LC_Client_SetStopFn(cl, LC_ClientLcc_V_Stop);
  LC_Client_SetGetNextCardFn(cl, LC_ClientLcc_V_GetNextCard);
  LC_Client_SetReleaseCardFn(cl, LC_ClientLcc_V_ReleaseCard);
  LC_Client_SetExecApduFn(cl, LC_ClientLcc_V_ExecApdu);
  LC_Client_SetSetNotifyFn(cl, LC_ClientLcc_V_SetNotify);

  return cl;
}



void GWENHYWFAR_CB LC_ClientLcc_FreeData(void *bp, void *p) {
  LC_CLIENT_LCC *xcl;

  xcl=(LC_CLIENT_LCC*)p;
  LC_Card_List_free(xcl->cards);
  LC_Request_List_free(xcl->waitingRequests);
  LC_Request_List_free(xcl->workingRequests);
  LC_Server_List_free(xcl->servers);
  GWEN_IpcManager_free(xcl->ipcManager);
}



LC_CLIENTLCC_HANDLE_REQUEST_FN
LC_ClientLcc_SetHandleRequestFn(LC_CLIENT *cl,
                                LC_CLIENTLCC_HANDLE_REQUEST_FN fn) {
  LC_CLIENT_LCC *xcl;
  LC_CLIENTLCC_HANDLE_REQUEST_FN oldFn;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  oldFn=xcl->handleRequestFn;
  xcl->handleRequestFn=fn;
  return oldFn;
}



LC_CLIENT_RESULT CHIPCARD_CB LC_ClientLcc_V_Init(LC_CLIENT *cl, GWEN_DB_NODE *db) {
  LC_CLIENT_LCC *xcl;
  GWEN_DB_NODE *gr;
  const char *globalOwnCertFile;
  GWEN_BUFFER *cfbuf;

  assert(cl);
  assert(db);

  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  if (xcl->initFn) {
    LC_CLIENT_RESULT res;

    res=xcl->initFn(cl, db);
    if (res!=LC_Client_ResultOk)
      return res;
  }

  cfbuf=0;
  globalOwnCertFile=GWEN_DB_GetCharValue(db, "certfile", 0, 0);
  if (!globalOwnCertFile) {
    const char *s;
    FILE *f;

    cfbuf=GWEN_Buffer_new(0, 256, 0, 1);
    s=LC_CLIENT_DEF_CERT_FILE;
    if (*s=='~') {
      char hbuf[256];

      s++;
      if (*s=='/' || *s=='\\')
        s++;
      if (GWEN_Directory_GetHomeDirectory(hbuf, sizeof(hbuf))) {
        DBG_ERROR(LC_LOGDOMAIN, "Could not get home folder");
        GWEN_Buffer_free(cfbuf);
        return LC_Client_ResultCfgError;
      }
      GWEN_Buffer_AppendString(cfbuf, hbuf);
      GWEN_Buffer_AppendString(cfbuf, s);
    }
    else
      GWEN_Buffer_AppendString(cfbuf, s);

    f=fopen(GWEN_Buffer_GetStart(cfbuf), "r");
    if (f) {
      fclose(f);
      DBG_INFO(LC_LOGDOMAIN, "Default certificate is \"%s\"",
               GWEN_Buffer_GetStart(cfbuf));
      globalOwnCertFile=GWEN_Buffer_GetStart(cfbuf);
    }
  }

  /* read servers */
  gr=GWEN_DB_GetFirstGroup(db);
  while(gr) {
    int rv;

    if (strcasecmp(GWEN_DB_GroupName(gr), "server")==0) {
      rv=LC_ClientLcc__CreateServer(cl, gr, globalOwnCertFile);
      if (rv) {
        DBG_ERROR(LC_LOGDOMAIN, "Error in server group");
        GWEN_DB_Dump(gr, stderr, 2);
        GWEN_Buffer_free(cfbuf);
        return LC_Client_ResultCfgError;
      }
    } /* if "server" */

    gr=GWEN_DB_GetNextGroup(gr);
  } /* while */

  GWEN_Buffer_free(cfbuf);
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT CHIPCARD_CB LC_ClientLcc_V_Fini(LC_CLIENT *cl) {
  LC_CLIENT_LCC *xcl;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  /* TODO: deinit IPC, clear list of servers and requests */

  if (xcl->finiFn) {
    LC_CLIENT_RESULT res;

    res=xcl->finiFn(cl);
    if (res!=LC_Client_ResultOk)
      return res;
  }

  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT CHIPCARD_CB LC_ClientLcc_V_Start(LC_CLIENT *cl) {
  LC_CLIENT_LCC *xcl;
  LC_CLIENT_RESULT res;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  res=LC_ClientLcc_StartWait(cl, 0, 0);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
    return res;
  }
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT CHIPCARD_CB LC_ClientLcc_V_Stop(LC_CLIENT *cl) {
  LC_CLIENT_LCC *xcl;
  LC_CLIENT_RESULT res;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  res=LC_ClientLcc_StopWait(cl);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
    return res;
  }
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT LC_ClientLcc__GetNextCard(LC_CLIENT *cl,
                                           LC_CARD **pCard,
                                           int timeout) {
  LC_CLIENT_LCC *xcl;
  LC_CARD *card;
  LC_CLIENT_RESULT res;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  /* get the next available card */
  if (timeout==LC_CLIENT_TIMEOUT_NONE) {
    /* return next card if there is any */
    card=LC_ClientLcc_GetNextCard(cl);
    if (card) {
      *pCard=card;
      return LC_Client_ResultOk;
    }
   
    /* work on current IPC requests/notifications */
    if (LC_ClientLcc_Work(cl, 0)==-1) {
      DBG_ERROR(LC_LOGDOMAIN, "Error working on client");
      GWEN_WaitCallback_Leave();
      return LC_Client_ResultIpcError;
    }

    /* return next card if there is one now */
    card=LC_ClientLcc_GetNextCard(cl);
    if (card) {
      *pCard=card;
      res=LC_Client_ResultOk;
    }
    else
      res=LC_Client_ResultWait;
  }
  else if (timeout==LC_CLIENT_TIMEOUT_FOREVER) {
    res=LC_ClientLcc_WaitForNextCard(cl, &card,
                                     GWEN_NET2_TIMEOUT_FOREVER);
  }
  else
    res=LC_ClientLcc_WaitForNextCard(cl, &card, timeout);

  if (res!=LC_Client_ResultOk)
    return res;

  *pCard=card;
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT CHIPCARD_CB LC_ClientLcc_V_GetNextCard(LC_CLIENT *cl,
                                            LC_CARD **pCard,
                                            int timeout) {
  LC_CLIENT_LCC *xcl;
  LC_CARD *card;
  LC_CLIENT_RESULT res;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  res=LC_ClientLcc__GetNextCard(cl, &card, timeout);
  if (res!=LC_Client_ResultOk)
    return res;
  DBG_INFO(LC_LOGDOMAIN, "Got a card, taking it");

  /* take the card */
  res=LC_ClientLcc_TakeCard(cl, card);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
    LC_Card_free(card);
    return res;
  }
  DBG_INFO(LC_LOGDOMAIN, "Card taken");

  /* return the card */
  *pCard=card;
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT CHIPCARD_CB LC_ClientLcc_V_ReleaseCard(LC_CLIENT *cl, LC_CARD *card) {
  LC_CLIENT_LCC *xcl;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  if (LC_CardLcc_IsConnected(card)) {
    LC_CLIENT_RESULT res;

    res=LC_ClientLcc_ReleaseCard(cl, card);
    if (res!=LC_Client_ResultOk) {
      DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
      return res;
    }
  }
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT CHIPCARD_CB LC_ClientLcc_V_ExecApdu(LC_CLIENT *cl,
                                         LC_CARD *card,
                                         const char *apdu,
                                         unsigned int len,
                                         GWEN_BUFFER *rbuf,
                                         LC_CLIENT_CMDTARGET t,
                                         int timeout) {
  LC_CLIENT_LCC *xcl;
  LC_CLIENT_RESULT res;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  res=LC_ClientLcc_CommandCard(cl, card, apdu, len, rbuf, t, timeout);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
    return res;
  }
  return LC_Client_ResultOk;
}



LC_CLIENT_RESULT CHIPCARD_CB LC_ClientLcc_V_SetNotify(LC_CLIENT *cl,
                                          GWEN_TYPE_UINT32 flags) {
  LC_CLIENT_LCC *xcl;
  LC_CLIENT_RESULT res;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  res=LC_ClientLcc_SetNotify(cl, flags);
  if (res!=LC_Client_ResultOk) {
    DBG_INFO(LC_LOGDOMAIN, "here (%d)", res);
    return res;
  }
  return LC_Client_ResultOk;

}


LC_CLIENT_RESULT LC_ClientLcc_ErrorToResult(int code) {
  LC_CLIENT_RESULT res;

  switch(code) {
  case LC_ERROR_NONE:             res=LC_Client_ResultOk; break;
  case LC_ERROR_GENERIC:          res=LC_Client_ResultGeneric; break;
  case LC_ERROR_INVALID:          res=LC_Client_ResultInvalid; break;
  case LC_ERROR_CARD_REMOVED:     res=LC_Client_ResultCardRemoved; break;
  case LC_ERROR_CARD_NOT_OWNED:   res=LC_Client_ResultInvalid; break;
  case LC_ERROR_NOT_SUPPORTED:    res=LC_Client_ResultNotSupported; break;
  case LC_ERROR_SETUP:            res=LC_Client_ResultInternal; break;
  case LC_ERROR_NO_DATA:          res=LC_Client_ResultNoData; break;
  case LC_ERROR_LOCKED_BY_OTHER:  res=LC_Client_ResultInvalid; break;
  case LC_ERROR_NOT_LOCKED:       res=LC_Client_ResultInvalid; break;

  case LC_ERROR_BAD_RESPONSE:          res=LC_Client_ResultNoData; break;
  case LC_ERROR_NO_SLOTS_CONNECTED:
  case LC_ERROR_NO_SLOTS_DISCONNECTED:
  case LC_ERROR_NO_SLOTS_AVAILABLE:    res=LC_Client_ResultIoError; break;
  case LC_ERROR_BAD_PIN:               res=LC_Client_ResultBadPin; break;
  case LC_ERROR_USER_ABORTED:          res=LC_Client_ResultAborted; break;
  case LC_ERROR_CARD_DESTROYED:        res=LC_Client_ResultGeneric; break;
  case LC_ERROR_READER_REMOVED:        res=LC_Client_ResultCardRemoved; break;
  case LC_ERROR_TIMEOUT:               
  default:                             res=LC_Client_ResultGeneric; break;
  }

  DBG_NOTICE(0, "Transformed code %d to result %d", code, res);
  return res;
}





LC_CLIENT_RESULT LC_ClientLcc_CheckForError(GWEN_DB_NODE *db) {
  const char *name;

  if (strcasecmp(GWEN_DB_GroupName(db), "error")==0) {
    int numCode;
    const char *txt;

    numCode=GWEN_DB_GetIntValue(db, "code", 0, LC_ERROR_GENERIC);
    txt=GWEN_DB_GetCharValue(db, "text", 0, "<empty>");
    DBG_ERROR(0, "Error %d: %s", numCode, txt);
    return LC_ClientLcc_ErrorToResult(numCode);
  }

  name=GWEN_DB_GetCharValue(db, "ipc/cmd", 0, 0);
  assert(name);
  if (strcasecmp(name, "Error")==0) {
    int code;
    const char *text;

    code=GWEN_DB_GetIntValue(db, "data/code", 0, 0);
    text=GWEN_DB_GetCharValue(db, "data/text", 0, "(empty)");
    if (code) {
      DBG_ERROR(LC_LOGDOMAIN, "Error %d: %s", code, text);
    }
    else {
      if (text) {
        DBG_INFO(LC_LOGDOMAIN, "Info: %s", text);
      }
    }
    return LC_ClientLcc_ErrorToResult(code);
  }

  return 0;
}



int LC_ClientLcc_StartConnect(LC_CLIENT *cl, LC_SERVER *sv) {
  LC_CLIENT_LCC *xcl;
  GWEN_DB_NODE *dbReq;
  GWEN_TYPE_UINT32 rid;
  LC_SERVER_STATUS st;

  assert(cl);
  assert(sv);

  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  st=LC_Server_GetStatus(sv);
  if (st!=LC_ServerStatusUnconnected) {
    DBG_ERROR(LC_LOGDOMAIN, "Bad server status (%d)", st);
    return -1;
  }

  /* tell the server about our status */
  dbReq=GWEN_DB_Group_new("Client_Ready");
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "Application",
                       LC_Client_GetProgramName(cl));
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
  rid=GWEN_IpcManager_SendRequest(xcl->ipcManager,
                                  LC_Server_GetServerId(sv),
                                  dbReq);
  if (rid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send command");
    return -1;
  }

  LC_Server_SetCurrentCommand(sv, rid);
  LC_Server_SetStatus(sv, LC_ServerStatusWaitReady);
  DBG_INFO(LC_LOGDOMAIN, "Started to connect");
  return 0;
}



int LC_ClientLcc__CreateServer(LC_CLIENT *cl, GWEN_DB_NODE *gr,
                               const char *globalOwnCertFile) {
  LC_CLIENT_LCC *xcl;
  const char *typ;
  const char *address;
  int port;
  GWEN_SOCKET *sk;
  GWEN_INETADDRESS *addr;
  GWEN_TYPE_UINT32 sid;
  GWEN_NETLAYER *nl;
  GWEN_NETLAYER *nlBase;
  GWEN_URL *url;
  LC_SERVER *sv;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  typ=GWEN_DB_GetCharValue(gr, "typ", 0, "local");
  address=GWEN_DB_GetCharValue(gr,
                               "addr", 0,
                               "0.0.0.0");
  url=GWEN_Url_fromString(address);
  port=GWEN_DB_GetIntValue(gr,
                           "port", 0,
                           LC_DEFAULT_PORT);

  if (strcasecmp(typ, "local")==0) {
    /* HTTP over UDS */
    sk=GWEN_Socket_new(GWEN_SocketTypeUnix);
    addr=GWEN_InetAddr_new(GWEN_AddressFamilyUnix);
    GWEN_InetAddr_SetAddress(addr, address);
    nlBase=GWEN_NetLayerSocket_new(sk, 1);
    GWEN_NetLayer_SetPeerAddr(nlBase, addr);
  }
  else if (strcasecmp(typ, "public")==0) {
    /* HTTP over TCP */
    sk=GWEN_Socket_new(GWEN_SocketTypeTCP);
    addr=GWEN_InetAddr_new(GWEN_AddressFamilyIP);
    GWEN_InetAddr_SetAddress(addr, GWEN_Url_GetServer(url));
    GWEN_InetAddr_SetPort(addr, port);
    nlBase=GWEN_NetLayerSocket_new(sk, 1);
    GWEN_NetLayer_SetPeerAddr(nlBase, addr);
  }
  else {
    const char *certDir;
    const char *newCertDir;
    const char *ownCertFile;
    const char *ciphers;
    GWEN_BUFFER *tmpbuf1;
    GWEN_BUFFER *tmpbuf2;
    const char *s;

    /* get valid cert folder */
    tmpbuf1=GWEN_Buffer_new(0, 256, 0, 1);
    s=LC_CLIENT_DEF_CERT_DIR;
    if (*s=='~') {
      char hbuf[256];

      s++;
      if (*s=='/' || *s=='\\')
        s++;
      if (GWEN_Directory_GetHomeDirectory(hbuf, sizeof(hbuf))) {
        DBG_ERROR(LC_LOGDOMAIN, "Could not get home folder");
        GWEN_Buffer_free(tmpbuf1);
        return -1;
      }
      GWEN_Buffer_AppendString(tmpbuf1, hbuf);
      GWEN_Buffer_AppendString(tmpbuf1, s);
    }
    else
      GWEN_Buffer_AppendString(tmpbuf1, s);

    /* get new cert folder */
    tmpbuf2=GWEN_Buffer_new(0, 256, 0, 1);
    s=LC_CLIENT_DEF_NEWCERT_DIR;
    if (*s=='~') {
      char hbuf[256];

      s++;
      if (*s=='/' || *s=='\\')
        s++;
      if (GWEN_Directory_GetHomeDirectory(hbuf, sizeof(hbuf))) {
        DBG_ERROR(LC_LOGDOMAIN, "Could not get home folder");
        GWEN_Buffer_free(tmpbuf2);
        GWEN_Buffer_free(tmpbuf1);
        return -1;
      }
      GWEN_Buffer_AppendString(tmpbuf2, hbuf);
      GWEN_Buffer_AppendString(tmpbuf2, s);
    }
    else
      GWEN_Buffer_AppendString(tmpbuf2, s);

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
      return -1;
    }
  
    ownCertFile=GWEN_DB_GetCharValue(gr, "certfile", 0,
                                     globalOwnCertFile);
    ciphers=GWEN_DB_GetCharValue(gr, "ciphers", 0, 0);
    addr=GWEN_InetAddr_new(GWEN_AddressFamilyIP);
    GWEN_InetAddr_SetAddress(addr, GWEN_Url_GetServer(url));
    GWEN_InetAddr_SetPort(addr, port);
    sk=GWEN_Socket_new(GWEN_SocketTypeTCP);
    nlBase=GWEN_NetLayerSocket_new(sk, 1);
    GWEN_NetLayer_SetPeerAddr(nlBase, addr);

    if (strcasecmp(typ, "private")==0) {
      /* HTTP over SSL */
      nl=GWEN_NetLayerSsl_new(nlBase,
                              certDir,
                              newCertDir,
                              ownCertFile,
                              0,
                              0);
      GWEN_NetLayer_free(nlBase);
      GWEN_NetLayerSsl_SetAskAddCertFn(nl, LC_ClientLcc_AskAddCert,
                                       (void*)cl);
      nlBase=nl;
    }
    else if (strcasecmp(typ, "secure")==0) {
      /* HTTP over SSL with certificates */
      nl=GWEN_NetLayerSsl_new(nlBase,
                              certDir,
                              newCertDir,
                              ownCertFile,
                              0,
                              1);
      GWEN_NetLayer_free(nlBase);
      nlBase=nl;
    }
    else {
      DBG_ERROR(0, "Unknown mode \"%s\"", typ);
      GWEN_InetAddr_free(addr);
      GWEN_Url_free(url);
      return -1;
    }
  }
  GWEN_InetAddr_free(addr);

  nl=GWEN_NetLayerHttp_new(nlBase);
  GWEN_NetLayer_free(nlBase);
  GWEN_NetLayerHttp_SetOutCommand(nl, "POST", url);
  GWEN_Url_free(url);

  sid=GWEN_IpcManager_AddClient(xcl->ipcManager,
                                nl,
                                LC_CLIENT_MARK);
  if (sid==0) {
    DBG_ERROR(0, "Could not add IPC client");
    return -1;
  }

  sv=LC_Server_new(sid);
  LC_Server_List_Add(sv, xcl->servers);
  DBG_INFO(LC_LOGDOMAIN, "Added server");
  return 0;
}






LC_REQUEST *LC_ClientLcc_PeekNextRequest(LC_CLIENT *cl,
                                         GWEN_TYPE_UINT32 serverId){
  LC_CLIENT_LCC *xcl;
  LC_REQUEST *rq;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  rq=LC_Request_List_First(xcl->waitingRequests);
  while(rq) {
    if (serverId==0 || serverId==LC_Request_GetServerId(rq)) {
      return rq;
    }
    rq=LC_Request_List_Next(rq);
  } /* while */

  return 0;
}



LC_REQUEST *LC_ClientLcc_GetNextRequest(LC_CLIENT *cl,
                                        GWEN_TYPE_UINT32 serverId){
  LC_CLIENT_LCC *xcl;
  LC_REQUEST *rq;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  rq=LC_ClientLcc_PeekNextRequest(cl, serverId);
  if (rq) {
    LC_Request_List_Del(rq);
    LC_Request_List_Add(rq, xcl->workingRequests);
    return rq;
  }

  return 0;
}







int LC_ClientLcc_ServerDown(LC_CLIENT *cl, LC_SERVER *sv) {
  LC_CLIENT_LCC *xcl;
  LC_CARD *cd;
  LC_REQUEST *rq;

  assert(cl);
  assert(sv);

  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  cd=LC_Card_List_First(xcl->cards);
  while(cd) {
    LC_CARD *next;

    next=LC_Card_List_Next(cd);
    if (LC_CardLcc_GetServerId(cd)==LC_Server_GetServerId(sv)) {
      LC_Card_ResetCardId(cd);
      LC_Card_List_Del(cd);
      LC_Card_free(cd);
    }
    cd=next;
  } /* while */

  /* TODO: remove cards from monitor */

  /* check every request in the working list */
  rq=LC_Request_List_First(xcl->workingRequests);
  while(rq) {
    if (!LC_Request_GetIsAborted(rq)) {
      if (LC_Request_GetRequestId(rq)==LC_Server_GetServerId(sv)) {
        GWEN_IpcManager_RemoveRequest(xcl->ipcManager,
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
  GWEN_IpcManager_Disconnect(xcl->ipcManager, LC_Server_GetServerId(sv));

  return 0;
}



int LC_ClientLcc_HandleCardAvailable(LC_CLIENT *cl, GWEN_DB_NODE *dbReq){
  LC_CLIENT_LCC *xcl;
  LC_CARD *card;
  GWEN_TYPE_UINT32 cardId;
  GWEN_TYPE_UINT32 serverId;
  const char *cardType;
  GWEN_TYPE_UINT32 rflags;
  const void *p;
  unsigned int bsize;
  unsigned int i;
  const char *s;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  serverId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "data/cardid", 0, "0"),
                "%x", &cardId)) {
    DBG_ERROR(LC_LOGDOMAIN, "Bad server message");
    return -1;
  }
  cardType=GWEN_DB_GetCharValue(dbReq, "data/cardtype", 0, 0);
  assert(cardType);
  p=GWEN_DB_GetBinValue(dbReq, "data/atr", 0, 0, 0, &bsize);

  rflags=LC_ReaderFlags_fromDb(dbReq, "data/readerflags");

  card=LC_CardLcc_new(cl,
                      cardId,
                      serverId,
                      cardType,
                      rflags,
                      p, bsize);

  for (i=0;; i++) {
    s=GWEN_DB_GetCharValue(dbReq, "data/cardTypes", i, 0);
    if (!s)
      break;
    LC_Card_AddCardType(card, s);
  }

  /* get driver type */
  s=GWEN_DB_GetCharValue(dbReq, "data/driverType", 0, 0);
  if (s)
    LC_Card_SetDriverType(card, s);

  /* get reader type */
  s=GWEN_DB_GetCharValue(dbReq, "data/readerType", 0, 0);
  if (s)
    LC_Card_SetReaderType(card, s);

  LC_Card_List_Add(card, xcl->cards);
  DBG_INFO(LC_LOGDOMAIN, "Card added");
  return 0;
}



LC_CARD *LC_ClientLcc_PeekNextCard(LC_CLIENT *cl){
  LC_CLIENT_LCC *xcl;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);
  return LC_Card_List_First(xcl->cards);
}



LC_CARD *LC_ClientLcc_GetNextCard(LC_CLIENT *cl){
  LC_CLIENT_LCC *xcl;
  LC_CARD *card;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  card=LC_Card_List_First(xcl->cards);
  if (card) {
    LC_Card_List_Del(card);
  }
  return card;
}



LC_CLIENT_RESULT LC_ClientLcc_WaitForNextCard(LC_CLIENT *cl,
                                              LC_CARD **pCard,
                                              int timeout) {
  LC_CLIENT_LCC *xcl;
  LC_CARD *card;
  time_t startt;
  int distance;
  GWEN_NETLAYER_RESULT res;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  startt=time(0);
  assert(cl);

  if (timeout==GWEN_NET2_TIMEOUT_NONE)
    distance=GWEN_NET2_TIMEOUT_NONE;
  else if (timeout==GWEN_NET2_TIMEOUT_FOREVER)
    distance=GWEN_NET2_TIMEOUT_FOREVER;
  else {
    distance=GWEN_WaitCallback_GetDistance(0);
    if (distance)
      if ((distance/1000)>timeout)
        distance=timeout*1000;
    if (!distance)
      distance=750;
  }

  GWEN_WaitCallback_EnterWithText(GWEN_WAITCALLBACK_ID_SIMPLE_PROGRESS,
                                  I18N("Waiting for card to be inserted"),
                                  0, 0);
  GWEN_WaitCallback_SetProgressTotal(GWEN_WAITCALLBACK_PROGRESS_NONE);
  for (;;) {
    DBG_VERBOUS(LC_LOGDOMAIN, "Working");
    if (LC_ClientLcc_Work(cl, 0)==-1) {
      DBG_ERROR(LC_LOGDOMAIN, "Error working on client");
      GWEN_WaitCallback_Leave();
      return LC_Client_ResultIpcError;
    }

    card=LC_ClientLcc_GetNextCard(cl);
    if (card) {
      DBG_DEBUG(LC_LOGDOMAIN, "Got a card");
      GWEN_WaitCallback_Leave();
      *pCard=card;
      return LC_Client_ResultOk;
    }

    for (;;) {
      if (LC_ClientLcc_Work(cl, 0)==-1) {
        DBG_ERROR(LC_LOGDOMAIN, "Error working on client");
        GWEN_WaitCallback_Leave();
        return LC_Client_ResultIpcError;
      }


      if (GWEN_WaitCallback()==GWEN_WaitCallbackResult_Abort) {
        DBG_ERROR(LC_LOGDOMAIN, "User aborted via waitcallback");
        GWEN_WaitCallback_Leave();
        return LC_Client_ResultAborted;
      }

      res=GWEN_Net_HeartBeat(distance);
      if (res==GWEN_NetLayerResult_Error) {
        DBG_ERROR(LC_LOGDOMAIN, "Error while working (%d)", res);
        GWEN_WaitCallback_Leave();
        return LC_Client_ResultIpcError;
      }
      else if (res==GWEN_NetLayerResult_Changed)
        break;
      else if (res!=GWEN_NetLayerResult_WouldBlock)
        break;

      /* check timeout */
      if (timeout!=GWEN_NET2_TIMEOUT_FOREVER) {
        if (timeout==GWEN_NET2_TIMEOUT_NONE ||
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

  return LC_Client_ResultWait;
}



int LC_ClientLcc_CheckServer(LC_CLIENT *cl, LC_SERVER *sv) {
  LC_CLIENT_LCC *xcl;
  GWEN_TYPE_UINT32 rid;
  LC_REQUEST *rq;
  int handled;
  int done;

  assert(cl);
  assert(sv);

  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  handled=0;
  done=0;

  if (LC_Server_GetStatus(sv)==LC_ServerStatusUnconnected) {
    handled=1;
    rq=LC_ClientLcc_PeekNextRequest(cl, LC_Server_GetServerId(sv));
    if (rq) {
      DBG_INFO(LC_LOGDOMAIN, "Starting to connect");
      if (LC_ClientLcc_StartConnect(cl, sv)) {
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

      dbReq=GWEN_IpcManager_GetResponseData(xcl->ipcManager, rid);
      if (dbReq==0) {
        /* TODO: check for timeout */
        DBG_DEBUG(LC_LOGDOMAIN, "No response yet");
      }
      else {
        int errCode;

        errCode=LC_ClientLcc_CheckForError(dbReq);
        if (errCode) {
          DBG_INFO(LC_LOGDOMAIN, "Error connecting (%08x)", errCode);
          LC_Server_SetStatus(sv, LC_ServerStatusAborted);
          LC_Server_SetCurrentCommand(sv, 0);
          GWEN_DB_Group_free(dbReq);
          GWEN_IpcManager_RemoveRequest(xcl->ipcManager, rid, 1);
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
    DBG_VERBOUS(LC_LOGDOMAIN, "Checking for incoming requests");
    rid=GWEN_IpcManager_GetNextInRequest(xcl->ipcManager,
                                         LC_CLIENT_MARK);
    if (rid) {
      GWEN_DB_NODE *dbReq;
      const char *name;
      int rqHandled=0;

      /* there is an incoming request */
      DBG_DEBUG(LC_LOGDOMAIN, "Got an incoming request");
      done++;
      dbReq=GWEN_IpcManager_GetInRequestData(xcl->ipcManager, rid);
      assert(dbReq);
      name=GWEN_DB_GetCharValue(dbReq, "ipc/cmd", 0, 0);
      if (xcl->handleRequestFn) {
        int rv;

        rv=xcl->handleRequestFn(cl, rid, name, dbReq);
        if (rv!=1)
          rqHandled=1;
      }
      if (!rqHandled) {
        if (strcasecmp(name, "CardAvailable")==0) {
          DBG_DEBUG(LC_LOGDOMAIN, "A card seems to be available");
          if (LC_ClientLcc_HandleCardAvailable(cl, dbReq)) {
            DBG_WARN(LC_LOGDOMAIN, "Error handling CardAvailable message");
          }
        }
        else if (strcasecmp(name, "Notification")==0) {
          DBG_INFO(LC_LOGDOMAIN, "Notification received");
          if (LC_ClientLcc_HandleNotification(cl, dbReq)) {
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
        GWEN_IpcManager_RemoveRequest(xcl->ipcManager, rid, 0);
      }
    } /* if incoming request */

    /* check for outbound requests */
    rq=LC_ClientLcc_PeekNextRequest(cl, LC_Server_GetServerId(sv));
    if (rq) {
      GWEN_DB_NODE *dbReq;

      /* we have a waiting request */
      dbReq=LC_Request_GetRequestData(rq);
      assert(dbReq);
      DBG_DEBUG(LC_LOGDOMAIN, "Sending waiting request %08x",
                LC_Request_GetRequestId(rq));
      if (GWEN_Logger_GetLevel(0)>=GWEN_LoggerLevelDebug)
        GWEN_DB_Dump(dbReq, stderr, 4);
      done++;
      rid=GWEN_IpcManager_SendRequest(xcl->ipcManager,
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
      rq=LC_ClientLcc_GetNextRequest(cl, LC_Server_GetServerId(sv));
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



GWEN_NL_SSL_ASKADDCERT_RESULT
LC_ClientLcc_AskAddCert(GWEN_NETLAYER *nl,
                        const GWEN_SSLCERTDESCR *cert,
                        void *user_data) {
  LC_CLIENT *cl;
  LC_CLIENT_LCC *xcl;

  cl=(LC_CLIENT*)user_data;
  if (!cl) {
    DBG_ERROR(0, "No user data in AskAddCert function");
    return GWEN_NetLayerSsl_AskAddCertResult_No;
  }
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  return xcl->askAddCertResult;
}



int LC_ClientLcc_Walk(LC_CLIENT *cl) {
  LC_CLIENT_LCC *xcl;
  LC_SERVER *sv;
  int rv;
  int done;
  int okCount;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  /* check all servers */
  done=0;
  okCount=0;
  sv=LC_Server_List_First(xcl->servers);
  while(sv) {
    rv=LC_ClientLcc_CheckServer(cl, sv);
    if (rv==-1) {
      DBG_INFO(LC_LOGDOMAIN, "Error checking server");
    }
    else {
      if (rv==0)
        done++;
      okCount++;
    }
    sv=LC_Server_List_Next(sv);
  } /* while */

  /* TODO: Check requests for timeouts */

  /* check whether any server succeeded */
  if (okCount==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Errors on all servers");
    return -1;
  }
  return done?0:1;
}



int LC_ClientLcc__Work(LC_CLIENT *cl, int maxmsg){
  LC_CLIENT_LCC *xcl;
  int done;
  int rv;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  done=0;

  while(1) {
    rv=LC_ClientLcc_Walk(cl);
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
    rv=GWEN_IpcManager_Work(xcl->ipcManager);
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
      GWEN_Socket_Select(0, 0, 0, 1000);
      break;
    }
  }

  return done?0:1;
}



int LC_ClientLcc_Work(LC_CLIENT *cl, int maxmsg){
  for (;;) {
    int rv;

    rv=LC_ClientLcc__Work(cl, maxmsg);
    if (rv!=0) {
      if (rv==-1) {
        DBG_INFO(LC_LOGDOMAIN, "here");
      }
      else {
        DBG_VERBOUS(LC_LOGDOMAIN, "Nothing done");
      }
      return rv;
    }
    DBG_VERBOUS(LC_LOGDOMAIN, "Something done");
  }
}



LC_CLIENT_RESULT LC_ClientLcc_Work_Wait(LC_CLIENT *cl, int timeout) {
  LC_CLIENT_LCC *xcl;
  time_t startt;
  int distance;
  GWEN_NETLAYER_RESULT res;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);


  startt=time(0);
  assert(cl);

  /* check every request with the given id in the working list */
  if (timeout==GWEN_NET2_TIMEOUT_NONE)
    distance=GWEN_NET2_TIMEOUT_NONE;
  else if (timeout==GWEN_NET2_TIMEOUT_FOREVER)
    distance=GWEN_NET2_TIMEOUT_FOREVER;
  else {
    distance=GWEN_WaitCallback_GetDistance(0);
    if (distance)
      if ((distance/1000)>timeout)
        distance=timeout*1000;
    if (!distance)
      distance=750;
  }

  GWEN_WaitCallback_EnterWithText(GWEN_WAITCALLBACK_ID_FAST,
                                  I18N("Libchipcard is working..."),
                                  0, 0);
  GWEN_WaitCallback_SetProgressTotal(GWEN_WAITCALLBACK_PROGRESS_NONE);
  for (;;) {
    int didSomething;

    didSomething=0;
    while(1) {
      int rv;

      rv=LC_ClientLcc__Work(cl, 0);
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
      if (res==GWEN_NetLayerResult_Error) {
        DBG_ERROR(LC_LOGDOMAIN, "Error while working (%d)", res);
        GWEN_WaitCallback_Leave();
        return LC_Client_ResultIpcError;
      }
      else if (res==GWEN_NetLayerResult_Changed) {
        DBG_VERBOUS(LC_LOGDOMAIN, "Changed");
        break;
      }

      /* check timeout */
      if (timeout!=GWEN_NET2_TIMEOUT_FOREVER) {
        if (timeout==GWEN_NET2_TIMEOUT_NONE ||
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











GWEN_TYPE_UINT32 LC_ClientLcc_SendRequest(LC_CLIENT *cl,
                                          LC_CARD *card,
                                          GWEN_TYPE_UINT32 serverId,
                                          GWEN_DB_NODE *dbReq){
  LC_CLIENT_LCC *xcl;
  GWEN_TYPE_UINT32 rid;
  LC_REQUEST *rq;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  if (serverId==0) {
    LC_SERVER *sv;

    sv=LC_Server_List_First(xcl->servers);
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
      LC_Request_List_Add(rq, xcl->waitingRequests);
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
    LC_Request_List_Add(rq, xcl->waitingRequests);
    return rid;
  }
}



GWEN_DB_NODE *LC_ClientLcc_GetNextResponse(LC_CLIENT *cl,
                                           GWEN_TYPE_UINT32 rqid) {
  LC_CLIENT_LCC *xcl;
  LC_REQUEST *rq;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  /* check every request with the given id in the working list */
  rq=LC_Request_List_First(xcl->workingRequests);
  while(rq) {
    if (LC_Request_GetRequestId(rq)==rqid) {
      GWEN_DB_NODE *dbRsp;

      dbRsp=
        GWEN_IpcManager_GetResponseData(xcl->ipcManager,
                                        LC_Request_GetIpcRequestId(rq));
      if (dbRsp) {
        DBG_VERBOUS(LC_LOGDOMAIN, "Got a response to request %08x", rqid);
        return dbRsp;
      }
    } /* if request id matches */
    rq=LC_Request_List_Next(rq);
  } /* while */

  return 0;
}



GWEN_DB_NODE *LC_ClientLcc_WaitForNextResponse(LC_CLIENT *cl,
                                               GWEN_TYPE_UINT32 rqid,
                                               int timeout) {
  LC_CLIENT_LCC *xcl;
  time_t startt;
  int distance;
  GWEN_NETLAYER_RESULT res;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  startt=time(0);

  /* check every request with the given id in the working list */
  if (timeout==GWEN_NET2_TIMEOUT_NONE)
    distance=GWEN_NET2_TIMEOUT_NONE;
  else if (timeout==GWEN_NET2_TIMEOUT_FOREVER)
    distance=GWEN_NET2_TIMEOUT_FOREVER;
  else {
    distance=GWEN_WaitCallback_GetDistance(0);
    if (distance)
      if ((distance/1000)>timeout)
        distance=timeout*1000;
    if (!distance)
      distance=750;
  }

  GWEN_WaitCallback_EnterWithText(GWEN_WAITCALLBACK_ID_FAST,
                                  I18N("Waiting for server response..."),
                                  0, 0);
  GWEN_WaitCallback_SetProgressTotal(GWEN_WAITCALLBACK_PROGRESS_NONE);
  for (;;) {
    GWEN_DB_NODE *dbRsp;

    while(1) {
      int rv;

      rv=LC_ClientLcc_Work(cl, 0);
      if (rv==-1) {
        DBG_ERROR(LC_LOGDOMAIN, "Error working on client");
        GWEN_WaitCallback_Leave();
        return 0;
      }
      else if (rv==1)
        break;
    }

    dbRsp=LC_ClientLcc_GetNextResponse(cl, rqid);
    if (dbRsp) {
      DBG_VERBOUS(LC_LOGDOMAIN, "Got a response to request \"%08x\"", rqid);
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
      if (res==GWEN_NetLayerResult_Error) {
        DBG_ERROR(LC_LOGDOMAIN, "Error while working (%d)", res);
        GWEN_WaitCallback_Leave();
        return 0;
      }
      else if (res==GWEN_NetLayerResult_Changed) {
        DBG_VERBOUS(LC_LOGDOMAIN, "Changed");
        break;
      }
      else if (res==GWEN_NetLayerResult_Idle) {
        DBG_VERBOUS(LC_LOGDOMAIN, "Nothing to do");
        break;
      }

      /* check timeout */
      if (timeout!=GWEN_NET2_TIMEOUT_FOREVER) {
        if (timeout==GWEN_NET2_TIMEOUT_NONE ||
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



int LC_ClientLcc_DeleteRequest(LC_CLIENT *cl, GWEN_TYPE_UINT32 rqid) {
  LC_CLIENT_LCC *xcl;
  LC_REQUEST *rq;
  int rqs;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  rqs=0;

  /* check every request with the given id in the working list */
  rq=LC_Request_List_First(xcl->workingRequests);
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
  rq=LC_Request_List_First(xcl->waitingRequests);
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



int LC_ClientLcc_DeleteInRequest(LC_CLIENT *cl, GWEN_TYPE_UINT32 rid) {
  LC_CLIENT_LCC *xcl;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  return GWEN_IpcManager_RemoveRequest(xcl->ipcManager, rid, 0);
}



int LC_ClientLcc_SendResponse(LC_CLIENT *cl,
                              GWEN_TYPE_UINT32 rid,
                              GWEN_DB_NODE *rsp) {
  LC_CLIENT_LCC *xcl;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  return GWEN_IpcManager_SendResponse(xcl->ipcManager, rid, rsp);
}



LC_REQUEST *LC_ClientLcc_FindWaitingRequest(LC_CLIENT *cl,
                                            GWEN_TYPE_UINT32 requestId){
  LC_CLIENT_LCC *xcl;
  LC_REQUEST *rq;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  rq=LC_Request_List_First(xcl->waitingRequests);
  while(rq) {
    if (requestId==LC_Request_GetRequestId(rq)) {
      return rq;
    }
    rq=LC_Request_List_Next(rq);
  } /* while */

  return 0;
}



LC_REQUEST *LC_ClientLcc_FindWorkingRequest(LC_CLIENT *cl,
                                            GWEN_TYPE_UINT32 requestId){
  LC_CLIENT_LCC *xcl;
  LC_REQUEST *rq;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  rq=LC_Request_List_First(xcl->workingRequests);
  while(rq) {
    if (requestId==LC_Request_GetRequestId(rq)) {
      return rq;
    }
    rq=LC_Request_List_Next(rq);
  } /* while */

  return 0;
}



LC_REQUEST *LC_ClientLcc_FindRequest(LC_CLIENT *cl,
                                     GWEN_TYPE_UINT32 requestId){
  LC_REQUEST *rq;

  assert(cl);
  rq=LC_ClientLcc_FindWorkingRequest(cl, requestId);
  if (!rq)
    rq=LC_ClientLcc_FindWaitingRequest(cl, requestId);

  return rq;
}



LC_CLIENT_RESULT
LC_ClientLcc_CheckResponse(LC_CLIENT *cl, GWEN_TYPE_UINT32 rid){
  LC_CLIENT_LCC *xcl;
  LC_REQUEST *rq;
  GWEN_TYPE_UINT32 count;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  count=0;
  /* check every request with the given id in the working list */
  rq=LC_Request_List_First(xcl->workingRequests);
  while(rq) {
    if (LC_Request_GetRequestId(rq)==rid) {
      GWEN_DB_NODE *dbRsp;

      count++;
      dbRsp=
        GWEN_IpcManager_PeekResponseData(xcl->ipcManager,
                                         LC_Request_GetIpcRequestId(rq));
      if (dbRsp) {
        DBG_VERBOUS(LC_LOGDOMAIN, "Got a response to request %08x", rid);
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
    if (!LC_ClientLcc_FindWaitingRequest(cl, rid)) {
      DBG_ERROR(LC_LOGDOMAIN, "Request not found");
      return LC_Client_ResultIpcError;
    }
  }
  return LC_Client_ResultWait;
}



LC_CLIENT_RESULT
LC_ClientLcc_CheckResponse_Wait(LC_CLIENT *cl, GWEN_TYPE_UINT32 rid,
                                int timeout){
  LC_CLIENT_LCC *xcl;
  time_t startt;
  int distance;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  startt=time(0);
  assert(cl);

  /* check every request with the given id in the working list */
  if (timeout==GWEN_NET2_TIMEOUT_NONE)
    distance=GWEN_NET2_TIMEOUT_NONE;
  else if (timeout==GWEN_NET2_TIMEOUT_FOREVER)
    distance=GWEN_NET2_TIMEOUT_FOREVER;
  else {
    distance=GWEN_WaitCallback_GetDistance(0);
    if (distance)
      if ((distance/1000)>timeout)
        distance=timeout*1000;
    if (!distance)
      distance=750;
  }

  GWEN_WaitCallback_EnterWithText(GWEN_WAITCALLBACK_ID_FAST,
                                  I18N("Waiting for server response..."),
                                  0, 0);
  GWEN_WaitCallback_SetProgressTotal(GWEN_WAITCALLBACK_PROGRESS_NONE);
  for (;;) {
    GWEN_NETLAYER_RESULT nres;
    LC_CLIENT_RESULT cres;

    if (LC_ClientLcc_Work(cl, 0)==-1) {
      DBG_ERROR(LC_LOGDOMAIN, "Error working on client");
      GWEN_WaitCallback_Leave();
      return LC_Client_ResultIpcError;
    }

    cres=LC_ClientLcc_CheckResponse(cl, rid);
    if (cres==LC_Client_ResultOk) {
      DBG_VERBOUS(LC_LOGDOMAIN, "Got a response to request \"%08x\"", rid);
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
      if (nres==GWEN_NetLayerResult_Error) {
        DBG_ERROR(LC_LOGDOMAIN, "Error while working (%d)", nres);
        GWEN_WaitCallback_Leave();
        return LC_Client_ResultIpcError;
      }
      else if (nres==GWEN_NetLayerResult_Changed) {
        DBG_VERBOUS(LC_LOGDOMAIN, "Changed");
        break;
      }
      else if (nres!=GWEN_NetLayerResult_WouldBlock) {
        DBG_VERBOUS(LC_LOGDOMAIN, "Not wouldBlock");
        break;
      }

      /* check timeout */
      if (timeout!=GWEN_NET2_TIMEOUT_FOREVER) {
        if (timeout==GWEN_NET2_TIMEOUT_NONE ||
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








#include "clientlcc_cmd.c"
#include "clientlcc_reader.c"
#include "clientlcc_notify.c"


