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


#ifndef CHIPCARD_CLIENT_CLIENT_P_H
#define CHIPCARD_CLIENT_CLIENT_P_H

#define LC_CLIENT_STARTTIMEOUT 20

#define LC_CLIENT_SHORT_TIMEOUT 20
#define LC_CLIENT_LONG_TIMEOUT  60
#define LC_CLIENT_VERYLONG_TIMEOUT  (60*5)

#define LC_CLIENT_MARK 1

#ifdef OS_WIN32
# define LC_CLIENT_DATADIR "chipcard2"
#else
# define LC_CLIENT_DATADIR ".chipcard2"
#endif
#define LC_CLIENT_CERTDIR "certificates"

#include <gwenhywfar/logger.h>
#include <gwenhywfar/ipc.h>
#include <gwenhywfar/nl_ssl.h>
#include <gwenhywfar/nl_socket.h>
#include <gwenhywfar/nl_http.h>
#include <chipcard2-client/client/notifications.h>
#include <chipcard2-client/client/lowlevel/server.h>
#include <chipcard2-client/client/lowlevel/request.h>
#include "client_l.h"
#include "card_l.h"
#include "notifications_l.h"
#include "apps/cardmgr_l.h"


struct LC_CLIENT {
  GWEN_INHERIT_ELEMENT(LC_CLIENT)
  char *programName;
  char *programVersion;
  char *dataDir;
  LC_SERVER_LIST *servers;
  LC_REQUEST_LIST *waitingRequests;
  LC_REQUEST_LIST *workingRequests;
  GWEN_IPCMANAGER *ipcManager;
  LC_CARD_LIST *cards;
  LC_CARDMGR *cardMgr;
  int shortTimeout;
  int longTimeout;
  int veryLongTimeout;

  GWEN_NL_SSL_ASKADDCERT_RESULT askAddCertResult;

  LCM_MONITOR *monitor;

  LC_CLIENT_HANDLE_INREQUEST_FN handleInRequestFn;
  LC_CLIENT_SERVER_DOWN_FN serverDownFn;
};


LC_REQUEST *LC_Client_FindWaitingRequest(LC_CLIENT *cl,
                                         GWEN_TYPE_UINT32 requestId);
LC_REQUEST *LC_Client_FindWorkingRequest(LC_CLIENT *cl,
                                         GWEN_TYPE_UINT32 requestId);
LC_REQUEST *LC_Client_FindRequest(LC_CLIENT *cl, GWEN_TYPE_UINT32 requestId);

LC_SERVER *LC_Client_FindServer(LC_CLIENT *cl, GWEN_TYPE_UINT32 serverId);

int LC_Client_StartConnect(LC_CLIENT *cl, LC_SERVER *sv);


int LC_Client_HandleCardAvailable(LC_CLIENT *cl, GWEN_DB_NODE *dbReq);
int LC_Client_HandleNotification(LC_CLIENT *cl, GWEN_DB_NODE *dbReq);

LC_REQUEST *LC_Client_PeekNextRequest(LC_CLIENT *cl,
                                      GWEN_TYPE_UINT32 serverId);
LC_REQUEST *LC_Client_GetNextRequest(LC_CLIENT *cl,
                                     GWEN_TYPE_UINT32 serverId);

int LC_Client_StartConnect(LC_CLIENT *cl, LC_SERVER *sv);

int LC_Client_Walk(LC_CLIENT *cl);


int LC_Client__GetPassword(GWEN_NETLAYER *nl,
                           char *buffer, int num,
                           int rwflag);

int LC_Client_HandleInRequest(LC_CLIENT *cl,
                              GWEN_TYPE_UINT32 rid,
                              GWEN_DB_NODE *dbReq);
int LC_Client_ServerDown(LC_CLIENT *cl, LC_SERVER *sv);


int LC_Client__CreateServer(LC_CLIENT *cl, GWEN_DB_NODE *gr,
                            const char *globalOwnCertFile);

GWEN_NL_SSL_ASKADDCERT_RESULT
  LC_Client_AskAddCert(GWEN_NETLAYER *nl,
                       const GWEN_SSLCERTDESCR *cert,
                       void *user_data);


#endif /* CHIPCARD_CLIENT_CLIENT_P_H */



