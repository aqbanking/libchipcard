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
#include <gwenhywfar/gui.h>
#include <gwenhywfar/inetsocket.h>
#include <gwenhywfar/iolayer.h>
#include <gwenhywfar/io_socket.h>
#include <gwenhywfar/io_buffered.h>
#include <gwenhywfar/io_http.h>
#include <gwenhywfar/io_tls.h>
#include <gwenhywfar/iomanager.h>
#include <gwenhywfar/url.h>

#include <unistd.h>

#define I18N(msg) msg

/* to make it easier to fing it later in the code */
#define DEFAULT_GUI_ID 0


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

  xcl->initFn=LC_Client_SetInitFn(cl, LC_ClientLcc_V_Init);
  xcl->finiFn=LC_Client_SetFiniFn(cl, LC_ClientLcc_V_Fini);
  LC_Client_SetStartFn(cl, LC_ClientLcc_V_Start);
  LC_Client_SetStopFn(cl, LC_ClientLcc_V_Stop);
  LC_Client_SetGetNextCardFn(cl, LC_ClientLcc_V_GetNextCard);
  LC_Client_SetReleaseCardFn(cl, LC_ClientLcc_V_ReleaseCard);
  LC_Client_SetExecApduFn(cl, LC_ClientLcc_V_ExecApdu);
  LC_Client_SetSetNotifyFn(cl, LC_ClientLcc_V_SetNotify);

  GWEN_IpcManager_SetClientDownFn(xcl->ipcManager, LC_Client_IpcClientDown, cl);

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

  /* read servers */
  gr=GWEN_DB_GetFirstGroup(db);
  if (gr) {
    int rv;

    if (strcasecmp(GWEN_DB_GroupName(gr), "server")==0) {
      rv=LC_ClientLcc__CreateServer(cl, gr);
      if (rv) {
	DBG_ERROR(LC_LOGDOMAIN, "Error in server group");
	GWEN_DB_Dump(gr, stderr, 2);
        return LC_Client_ResultCfgError;
      }
    } /* if "server" */
  } /* while */

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



LC_CLIENT_RESULT CHIPCARD_CB LC_ClientLcc_V_GetNextCard(LC_CLIENT *cl,
							LC_CARD **pCard,
							int timeout) {
  LC_CLIENT_LCC *xcl;
  LC_CARD *card;
  LC_CLIENT_RESULT res;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  res=LC_ClientLcc_WaitForNextCard(cl, &card, timeout);
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
                                          uint32_t flags) {
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
    DBG_INFO(LC_LOGDOMAIN, "Error %d: %s", numCode, txt);
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
      DBG_INFO(LC_LOGDOMAIN, "Error %d: %s", code, text);
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
  uint32_t rid;
  LC_SERVER_STATUS st;
  int rv;

  assert(cl);
  assert(sv);

  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  st=LC_Server_GetStatus(sv);
  if (st!=LC_ServerStatusUnconnected) {
    DBG_ERROR(LC_LOGDOMAIN, "Bad server status (%d)", st);
    return GWEN_ERROR_INVALID;
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
  rv=GWEN_IpcManager_SendRequest(xcl->ipcManager,
				 LC_Server_GetServerId(sv),
				 dbReq,
				 &rid);
  if (rv<0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not send command (%d)", rv);
    return rv;
  }

  LC_Server_SetCurrentCommand(sv, rid);
  LC_Server_SetStatus(sv, LC_ServerStatusWaitReady);
  DBG_INFO(LC_LOGDOMAIN, "Started to connect");
  return 0;
}



int LC_ClientLcc__CreateServer(LC_CLIENT *cl, GWEN_DB_NODE *gr) {
  LC_CLIENT_LCC *xcl;
  const char *typ;
  const char *address;
  int port;
  GWEN_SOCKET *sk;
  GWEN_INETADDRESS *addr;
  uint32_t sid;
  GWEN_IO_LAYER *ioBase;
  GWEN_IO_LAYER *io;
  GWEN_URL *url;
  LC_SERVER *sv;
  GWEN_DB_NODE *db;

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
    ioBase=GWEN_Io_LayerSocket_new(sk);
    GWEN_Io_LayerSocket_SetPeerAddr(ioBase, addr);
  }
  else if (strcasecmp(typ, "public")==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Public mode not supported for security reasons.");
    return GWEN_ERROR_BAD_DATA;
  }
  else {
    const char *ownCertFile;
    const char *ownKeyFile;
    const char *caFile;

    ownCertFile=GWEN_DB_GetCharValue(gr, "certfile", 0, NULL);
    ownKeyFile=GWEN_DB_GetCharValue(gr, "keyfile", 0, NULL);
    caFile=GWEN_DB_GetCharValue(gr, "cafile", 0, NULL);
    addr=GWEN_InetAddr_new(GWEN_AddressFamilyIP);
    GWEN_InetAddr_SetAddress(addr, GWEN_Url_GetServer(url));
    GWEN_InetAddr_SetPort(addr, port);
    sk=GWEN_Socket_new(GWEN_SocketTypeTCP);

    ioBase=GWEN_Io_LayerSocket_new(sk);
    GWEN_Io_LayerSocket_SetPeerAddr(ioBase, addr);

    io=GWEN_Io_LayerTls_new(ioBase);
    ioBase=io;
    GWEN_Io_Layer_AddFlags(ioBase, GWEN_IO_LAYER_TLS_FLAGS_FORCE_SSL_V3);

    if (ownCertFile) {
      GWEN_Io_LayerTls_SetLocalCertFile(ioBase, ownCertFile);
    }
    if (ownKeyFile) {
      GWEN_Io_LayerTls_SetLocalKeyFile(ioBase, ownKeyFile);
    }
    if (caFile) {
      GWEN_Io_LayerTls_SetLocalTrustFile(ioBase, caFile);
    }

    if (strcasecmp(typ, "private")==0) {
      /* HTTP over SSL */
    }
    else if (strcasecmp(typ, "secure")==0) {
      /* HTTP over SSL with mandatory certificates from both sides */
      GWEN_Io_Layer_AddFlags(ioBase, GWEN_IO_LAYER_TLS_FLAGS_NEED_PEER_CERT);
    }
    else {
      DBG_ERROR(LC_LOGDOMAIN, "Unknown ipc mode \"%s\"", typ);
      GWEN_InetAddr_free(addr);
      GWEN_Url_free(url);
      return -1;
    }
  }
  GWEN_InetAddr_free(addr);

  io=GWEN_Io_LayerBuffered_new(ioBase);
  ioBase=io;

  io=GWEN_Io_LayerHttp_new(ioBase);
  GWEN_Io_Layer_AddFlags(io, GWEN_IO_LAYER_HTTP_FLAGS_IPC);

  db=GWEN_Io_LayerHttp_GetDbCommandOut(io);
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "command", "POST");
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "protocol", "HTTP/1.1");
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "url", GWEN_Url_GetPath(url));

  db=GWEN_Io_LayerHttp_GetDbHeaderOut(io);
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "Host", GWEN_Url_GetServer(url));
  GWEN_DB_SetCharValue(db, GWEN_DB_FLAGS_OVERWRITE_VARS, "Connection", "keep alive");

  ioBase=io;

  /* TODO: shouldn't it be addServer? */
  sid=GWEN_IpcManager_AddClient(xcl->ipcManager, io, LC_CLIENT_MARK);
  if (sid==0) {
    DBG_ERROR(LC_LOGDOMAIN, "Could not add IPC client");
    return GWEN_ERROR_INVALID;
  }

  GWEN_Url_free(url);

  sv=LC_Server_new(sid);
  LC_Server_List_Add(sv, xcl->servers);
  DBG_INFO(LC_LOGDOMAIN, "Added server");
  return 0;
}






LC_REQUEST *LC_ClientLcc_PeekNextRequest(LC_CLIENT *cl,
                                         uint32_t serverId){
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



void LC_ClientLcc_AbortServerRequests(LC_CLIENT *cl, uint32_t serverId, int errorCode){
  LC_CLIENT_LCC *xcl;
  LC_REQUEST *rq;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  rq=LC_Request_List_First(xcl->waitingRequests);
  while(rq) {
    if (serverId==0 || serverId==LC_Request_GetServerId(rq)) {
      DBG_DEBUG(LC_LOGDOMAIN, "Aborting request %08x", LC_Request_GetRequestId(rq));
      LC_Request_SetIsAborted(rq, errorCode);
    }
    rq=LC_Request_List_Next(rq);
  } /* while */
}



LC_REQUEST *LC_ClientLcc_GetNextRequest(LC_CLIENT *cl, uint32_t serverId){
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



int LC_ClientLcc_HandleCardAvailable(LC_CLIENT *cl, GWEN_DB_NODE *dbReq){
  LC_CLIENT_LCC *xcl;
  LC_CARD *card;
  uint32_t cardId;
  uint32_t serverId;
  const char *cardType;
  uint32_t rflags;
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
  GWEN_IO_LAYER_WORKRESULT res;
  uint32_t progressId;
  uint64_t to;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  startt=time(0);
  assert(cl);

  if (timeout==GWEN_TIMEOUT_NONE ||
      timeout==GWEN_TIMEOUT_FOREVER)
    to=0;
  else
    to=timeout*1000;

  progressId=GWEN_Gui_ProgressStart(GWEN_GUI_PROGRESS_DELAY |
				    GWEN_GUI_PROGRESS_ALLOW_EMBED |
				    GWEN_GUI_PROGRESS_SHOW_PROGRESS |
				    GWEN_GUI_PROGRESS_SHOW_ABORT,
				    I18N("Waiting for card to be inserted"),
				    NULL,
				    to,
				    DEFAULT_GUI_ID);

  for (;;) {
    int err;
    int rv;
    int done=0;

    /* let io manager work until there is nothing to do */
    while((res=GWEN_Io_Manager_Work())==GWEN_Io_Layer_WorkResultOk)
      done=1;
    if (res==GWEN_Io_Layer_WorkResultError) {
      DBG_ERROR(LC_LOGDOMAIN, "Error working on io layers");
      GWEN_Gui_ProgressEnd(progressId);
      return LC_Client_ResultIpcError;
    }

    /* let ipc manager work until there is nothing to do */
    while((rv=GWEN_IpcManager_Work(xcl->ipcManager))==0)
      done=1;
    if (rv<0) {
      DBG_ERROR(LC_LOGDOMAIN, "Error working on ipc");
      GWEN_Gui_ProgressEnd(progressId);
      return LC_Client_ResultIpcError;
    }

    /* let client work until there is nothing to do */
    while( (rv=LC_ClientLcc_Walk(cl))==0)
      done=1;
    if (rv<0) {
      DBG_ERROR(LC_LOGDOMAIN, "Error working on client");
      GWEN_Gui_ProgressEnd(progressId);
      return LC_Client_ResultIpcError;
    }

    /* try to get the next card (if any) */
    card=LC_ClientLcc_GetNextCard(cl);
    if (card) {
      DBG_DEBUG(LC_LOGDOMAIN, "Got a card");
      GWEN_Gui_ProgressEnd(progressId);
      *pCard=card;
      return LC_Client_ResultOk;
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
	  GWEN_Gui_ProgressEnd(progressId);
	  return LC_Client_ResultWait;
	}
      }

      /* advance progress bar */
      err=GWEN_Gui_ProgressAdvance(progressId, (uint64_t)(d*1000));
      if (err==GWEN_ERROR_USER_ABORTED) {
	DBG_ERROR(GWEN_LOGDOMAIN, "User aborted");
	GWEN_Gui_ProgressEnd(progressId);
	return LC_Client_ResultAborted;
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
  GWEN_Gui_ProgressEnd(progressId);

  return LC_Client_ResultWait;
}



int LC_ClientLcc_CheckServer(LC_CLIENT *cl, LC_SERVER *sv) {
  LC_CLIENT_LCC *xcl;
  uint32_t rid;
  LC_REQUEST *rq;
  int handled;
  int done;
  int rv;

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
      rv=LC_ClientLcc_StartConnect(cl, sv);
      if (rv<0) {
	DBG_ERROR(LC_LOGDOMAIN, "Could not start connecting to server");
	return rv;
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
      return GWEN_ERROR_INVALID;
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
          if (GWEN_Logger_GetLevel(0)>=GWEN_LoggerLevel_Notice) {
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
      int rv;

      /* we have a waiting request */
      dbReq=LC_Request_GetRequestData(rq);
      assert(dbReq);
      DBG_DEBUG(LC_LOGDOMAIN, "Sending waiting request %08x",
                LC_Request_GetRequestId(rq));
      if (GWEN_Logger_GetLevel(0)>=GWEN_LoggerLevel_Debug)
        GWEN_DB_Dump(dbReq, stderr, 4);
      done++;
      rv=GWEN_IpcManager_SendRequest(xcl->ipcManager,
				     LC_Server_GetServerId(sv),
				     GWEN_DB_Group_dup(dbReq),
				     &rid);
      if (rv<0) {
	DBG_ERROR(LC_LOGDOMAIN, "Could not send request (%d)", rv);
	LC_Server_SetStatus(sv, LC_ServerStatusAborted);
	return rv;
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
  if (sv==NULL) {
    DBG_ERROR(0, "No servers");
  }
  else {
    while(sv) {
      rv=LC_ClientLcc_CheckServer(cl, sv);
      if (rv==0) {
	okCount++;
	done++;
      }
      else if (rv<0) {
	DBG_INFO(LC_LOGDOMAIN, "Error checking server (%d)", rv);
      }
      else if (rv==1) {
	/* blocking but ok */
	okCount++;
      }
      sv=LC_Server_List_Next(sv);
    } /* while */
  }

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
  GWEN_IO_LAYER_WORKRESULT res;
  uint32_t progressId;
  uint64_t to;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  startt=time(0);
  assert(cl);

  if (timeout==GWEN_TIMEOUT_NONE || timeout==GWEN_TIMEOUT_FOREVER)
    to=0;
  else
    to=timeout*1000;

  progressId=GWEN_Gui_ProgressStart(GWEN_GUI_PROGRESS_DELAY |
				    GWEN_GUI_PROGRESS_ALLOW_EMBED |
				    GWEN_GUI_PROGRESS_SHOW_PROGRESS |
				    GWEN_GUI_PROGRESS_SHOW_ABORT,
				    I18N("Libchipcard is working..."),
				    NULL,
				    to,
				    DEFAULT_GUI_ID);

  for (;;) {
    int err;
    int rv;
    int done=0;
    int lcDone=0;

    /* let io manager work until there is nothing to do */
    while((res=GWEN_Io_Manager_Work())==GWEN_Io_Layer_WorkResultOk)
      done=1;
    if (res==GWEN_Io_Layer_WorkResultError) {
      DBG_ERROR(LC_LOGDOMAIN, "Error working on io layers");
      GWEN_Gui_ProgressEnd(progressId);
      return LC_Client_ResultIpcError;
    }

    /* let ipc manager work until there is nothing to do */
    while((rv=GWEN_IpcManager_Work(xcl->ipcManager))==0)
      done=1;
    if (rv<0) {
      DBG_ERROR(LC_LOGDOMAIN, "Error working on ipc");
      GWEN_Gui_ProgressEnd(progressId);
      return LC_Client_ResultIpcError;
    }

    /* let client work until there is nothing more to do */
    while( (rv=LC_ClientLcc_Walk(cl)==0))
      lcDone=1;
    if (rv<0) {
      DBG_ERROR(LC_LOGDOMAIN, "Error working on client");
      GWEN_Gui_ProgressEnd(progressId);
      return LC_Client_ResultIpcError;
    }
    if (lcDone) {
      /* client did something, return */
      GWEN_Gui_ProgressEnd(progressId);
      return LC_Client_ResultOk;
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
	  GWEN_Gui_ProgressEnd(progressId);
	  return LC_Client_ResultWait;
	}
      }

      /* advance progress bar */
      err=GWEN_Gui_ProgressAdvance(progressId, (uint64_t)(d*1000));
      if (err==GWEN_ERROR_USER_ABORTED) {
	DBG_ERROR(GWEN_LOGDOMAIN, "User aborted");
	GWEN_Gui_ProgressEnd(progressId);
	return LC_Client_ResultAborted;
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











uint32_t LC_ClientLcc_SendRequest(LC_CLIENT *cl,
				  LC_CARD *card,
				  uint32_t serverId,
				  GWEN_DB_NODE *dbReq){
  LC_CLIENT_LCC *xcl;
  uint32_t rid;
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



GWEN_DB_NODE *LC_ClientLcc_GetNextResponse(LC_CLIENT *cl, uint32_t rqid) {
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



LC_CLIENT_RESULT LC_ClientLcc_WaitForNextResponse(LC_CLIENT *cl,
						  uint32_t rqid,
						  GWEN_DB_NODE **pDbRsp,
						  int timeout) {
  LC_CLIENT_LCC *xcl;
  time_t startt;
  GWEN_IO_LAYER_WORKRESULT res;
  uint32_t progressId;
  uint64_t to;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  startt=time(0);

  if (timeout==GWEN_TIMEOUT_NONE ||
      timeout==GWEN_TIMEOUT_FOREVER)
    to=0;
  else
    to=timeout*1000;

  progressId=GWEN_Gui_ProgressStart(GWEN_GUI_PROGRESS_DELAY |
				    GWEN_GUI_PROGRESS_ALLOW_EMBED |
				    GWEN_GUI_PROGRESS_SHOW_PROGRESS |
				    GWEN_GUI_PROGRESS_SHOW_ABORT,
				    I18N("Waiting for server response..."),
				    NULL,
				    to,
				    DEFAULT_GUI_ID);

  for (;;) {
    int err;
    int rv;
    int done=0;
    GWEN_DB_NODE *dbRsp;

    /* let io manager work until there is nothing to do */
    while((res=GWEN_Io_Manager_Work())==GWEN_Io_Layer_WorkResultOk)
      done=1;
    if (res==GWEN_Io_Layer_WorkResultError) {
      DBG_ERROR(LC_LOGDOMAIN, "Error working on io layers");
      GWEN_Gui_ProgressEnd(progressId);
      return LC_Client_ResultIpcError;
    }

    /* let ipc manager work until there is nothing to do */
    while((rv=GWEN_IpcManager_Work(xcl->ipcManager))==0)
      done=1;
    if (rv<0) {
      DBG_ERROR(LC_LOGDOMAIN, "Error working on ipc");
      GWEN_Gui_ProgressEnd(progressId);
      return LC_Client_ResultIpcError;
    }

    /* let client work until there is nothing to do */
    while( (rv=LC_ClientLcc_Walk(cl))==0)
      done=1;
    if (rv<0) {
      DBG_ERROR(LC_LOGDOMAIN, "Error working on client");
      GWEN_Gui_ProgressEnd(progressId);
      return LC_Client_ResultIpcError;
    }

    /* try to get the next response (if any) */
    if (pDbRsp==NULL) {
      LC_CLIENT_RESULT res;

      res=LC_ClientLcc_CheckResponse(cl, rqid);
      if (res!=LC_Client_ResultOk && res!=LC_Client_ResultWait) {
	DBG_ERROR(LC_LOGDOMAIN, "Error checking response");
	GWEN_Gui_ProgressEnd(progressId);
	return res;
      }
      if (res!=LC_Client_ResultWait) {
	GWEN_Gui_ProgressEnd(progressId);
	return res;
      }
    }
    else {
      dbRsp=LC_ClientLcc_GetNextResponse(cl, rqid);
      if (dbRsp) {
	DBG_VERBOUS(LC_LOGDOMAIN, "Got a response to request \"%08x\"", rqid);
	GWEN_Gui_ProgressEnd(progressId);
	*pDbRsp=dbRsp;
	return LC_Client_ResultOk;
      }
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
	  GWEN_Gui_ProgressEnd(progressId);
	  return LC_Client_ResultWait;
	}
      }

      /* advance progress bar */
      err=GWEN_Gui_ProgressAdvance(progressId, (uint64_t)(d*1000));
      if (err==GWEN_ERROR_USER_ABORTED) {
	DBG_ERROR(GWEN_LOGDOMAIN, "User aborted");
	GWEN_Gui_ProgressEnd(progressId);
	return LC_Client_ResultAborted;
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



int LC_ClientLcc_DeleteRequest(LC_CLIENT *cl, uint32_t rqid) {
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



int LC_ClientLcc_DeleteInRequest(LC_CLIENT *cl, uint32_t rid) {
  LC_CLIENT_LCC *xcl;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  return GWEN_IpcManager_RemoveRequest(xcl->ipcManager, rid, 0);
}



int LC_ClientLcc_SendResponse(LC_CLIENT *cl,
                              uint32_t rid,
                              GWEN_DB_NODE *rsp) {
  LC_CLIENT_LCC *xcl;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  return GWEN_IpcManager_SendResponse(xcl->ipcManager, rid, rsp);
}



LC_REQUEST *LC_ClientLcc_FindWaitingRequest(LC_CLIENT *cl,
                                            uint32_t requestId){
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
                                            uint32_t requestId){
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
                                     uint32_t requestId){
  LC_REQUEST *rq;

  assert(cl);
  rq=LC_ClientLcc_FindWorkingRequest(cl, requestId);
  if (!rq)
    rq=LC_ClientLcc_FindWaitingRequest(cl, requestId);

  return rq;
}



LC_CLIENT_RESULT
LC_ClientLcc_CheckResponse(LC_CLIENT *cl, uint32_t rid){
  LC_CLIENT_LCC *xcl;
  LC_REQUEST *rq;
  uint32_t count;

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
	DBG_DEBUG(LC_LOGDOMAIN, "Got a response to request %08x", rid);
        return LC_Client_ResultOk;
      }
      else {
	if (LC_Request_GetIsAborted(rq)) {
	  DBG_ERROR(LC_LOGDOMAIN, "Request was aborted (server down?)");
	  return LC_Request_GetIsAborted(rq);
	}
      }
    } /* if request id matches */
    rq=LC_Request_List_Next(rq);
  } /* while */

  if (!count) {
    /* request not found, check whether it exists at all */
    rq=LC_ClientLcc_FindWaitingRequest(cl, rid);
    if (rq==NULL) {
      DBG_ERROR(LC_LOGDOMAIN, "Request not found");
      return LC_Client_ResultIpcError;
    }
    else {
      LC_CLIENT_RESULT res;

      res=LC_Request_GetIsAborted(rq);
      if (res) {
	DBG_ERROR(LC_LOGDOMAIN, "Request was aborted (server down?) [%d]", res);
	return res;
      }
    }
  }
  return LC_Client_ResultWait;
}



void LC_Client_IpcClientDown(GWEN_IPCMANAGER *mgr,
			     uint32_t id,
			     GWEN_IO_LAYER *io,
			     void *user_data) {
  LC_CLIENT *cl;
  LC_CLIENT_LCC *xcl;
  LC_SERVER *sv;

  cl=(LC_CLIENT*) user_data;

  assert(cl);
  xcl=GWEN_INHERIT_GETDATA(LC_CLIENT, LC_CLIENT_LCC, cl);
  assert(xcl);

  /* check all servers */
  sv=LC_Server_List_First(xcl->servers);
  while(sv) {
    if (LC_Server_GetServerId(sv)==id)
      break;
    sv=LC_Server_List_Next(sv);
  } /* while */
  if (sv==NULL) {
    DBG_WARN(LC_LOGDOMAIN, "IPC client %08x is not mine", id);
  }
  else {
    DBG_INFO(LC_LOGDOMAIN, "Server %08x is down", id);
    LC_ClientLcc_AbortServerRequests(cl, id, LC_Client_ResultIoError);
    LC_Server_SetStatus(sv, LC_ServerStatusAborted);
  }
}





#include "clientlcc_cmd.c"
#include "clientlcc_reader.c"
#include "clientlcc_notify.c"


