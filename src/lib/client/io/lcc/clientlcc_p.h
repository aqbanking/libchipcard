/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id: client_p.h 137 2005-11-03 13:07:50Z aquamaniac $
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef CHIPCARD_CLIENT_CLIENTLCC_P_H
#define CHIPCARD_CLIENT_CLIENTLCC_P_H

#include "clientlcc_l.h"
#include "request_l.h"
#include "server_l.h"

#include <chipcard3/client/card_imp.h>

#include <gwenhywfar/logger.h>
#include <gwenhywfar/ipc.h>
#include <gwenhywfar/nl_ssl.h>
#include <gwenhywfar/nl_socket.h>
#include <gwenhywfar/nl_http.h>


#define LC_CLIENT_MARK 1


typedef struct LC_CLIENT_LCC LC_CLIENT_LCC;
struct LC_CLIENT_LCC {
  LC_SERVER_LIST *servers;
  LC_REQUEST_LIST *waitingRequests;
  LC_REQUEST_LIST *workingRequests;
  GWEN_IPCMANAGER *ipcManager;
  LC_CARD_LIST *cards;

  int startedWait;

  GWEN_NL_SSL_ASKADDCERT_RESULT askAddCertResult;
  LC_CLIENTLCC_HANDLE_REQUEST_FN handleRequestFn;

  LC_CLIENT_INIT_FN initFn;
  LC_CLIENT_FINI_FN finiFn;
};

void GWENHYWFAR_CB LC_ClientLcc_FreeData(void *bp, void *p);

int LC_ClientLcc_StartConnect(LC_CLIENT *cl, LC_SERVER *sv);
int LC_ClientLcc__CreateServer(LC_CLIENT *cl, GWEN_DB_NODE *gr,
                               const char *globalOwnCertFile);

int LC_ClientLcc_ServerDown(LC_CLIENT *cl, LC_SERVER *sv);
int LC_ClientLcc_CheckServer(LC_CLIENT *cl, LC_SERVER *sv);

GWEN_NL_SSL_ASKADDCERT_RESULT
  LC_ClientLcc_AskAddCert(GWEN_NETLAYER *nl,
                          const GWEN_SSLCERTDESCR *cert,
                          void *user_data);

LC_REQUEST *LC_ClientLcc_PeekNextRequest(LC_CLIENT *cl,
                                         GWEN_TYPE_UINT32 serverId);
LC_REQUEST *LC_ClientLcc_GetNextRequest(LC_CLIENT *cl,
                                        GWEN_TYPE_UINT32 serverId);
int LC_ClientLcc_HandleCardAvailable(LC_CLIENT *cl, GWEN_DB_NODE *dbReq);
int LC_ClientLcc_HandleNotification(LC_CLIENT *cl, GWEN_DB_NODE *dbReq);



int LC_ClientLcc_Walk(LC_CLIENT *cl);
int LC_ClientLcc__Work(LC_CLIENT *cl, int maxmsg);
int LC_ClientLcc_Work(LC_CLIENT *cl, int maxmsg);



/* @name Working with Requests
 */
/*@{*/

LC_REQUEST *LC_ClientLcc_FindWaitingRequest(LC_CLIENT *cl,
                                            GWEN_TYPE_UINT32 requestId);
LC_REQUEST *LC_ClientLcc_FindWorkingRequest(LC_CLIENT *cl,
                                            GWEN_TYPE_UINT32 requestId);

LC_REQUEST *LC_ClientLcc_FindRequest(LC_CLIENT *cl,
                                     GWEN_TYPE_UINT32 requestId);


/*@}*/







static LC_CLIENT_RESULT CHIPCARD_CB
  LC_ClientLcc_V_Init(LC_CLIENT *cl, GWEN_DB_NODE *db);

static LC_CLIENT_RESULT CHIPCARD_CB
  LC_ClientLcc_V_Fini(LC_CLIENT *cl);

static LC_CLIENT_RESULT CHIPCARD_CB
  LC_ClientLcc_V_Start(LC_CLIENT *cl);

static LC_CLIENT_RESULT CHIPCARD_CB
  LC_ClientLcc_V_Stop(LC_CLIENT *cl);

static LC_CLIENT_RESULT CHIPCARD_CB
  LC_ClientLcc_V_GetNextCard(LC_CLIENT *cl,
                             LC_CARD **pCard,
                             int timeout);

static LC_CLIENT_RESULT CHIPCARD_CB
  LC_ClientLcc_V_ReleaseCard(LC_CLIENT *cl,
                             LC_CARD *card);

static LC_CLIENT_RESULT CHIPCARD_CB
  LC_ClientLcc_V_ExecApdu(LC_CLIENT *cl,
                          LC_CARD *card,
                          const char *apdu,
                          unsigned int len,
                          GWEN_BUFFER *rbuf,
                          LC_CLIENT_CMDTARGET t,
                          int timeout);

LC_CLIENT_RESULT CHIPCARD_CB
  LC_ClientLcc_V_SetNotify(LC_CLIENT *cl,
                           GWEN_TYPE_UINT32 flags);


LC_CLIENT_RESULT LC_ClientLcc_ErrorToResult(int code);

LC_CLIENT_RESULT LC_ClientLcc__GetNextCard(LC_CLIENT *cl,
                                           LC_CARD **pCard,
                                           int timeout);

#endif /* CHIPCARD_CLIENT_CLIENTLCC_P_H */



