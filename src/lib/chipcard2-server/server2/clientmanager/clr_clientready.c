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


#include "clientmanager_p.h"
#include "connection_l.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>

#include <gwenhywfar/netconnectionhttp.h>
#include <gwenhywfar/nettransportssl.h>
#include <gwenhywfar/nettransportsock.h>
#include <gwenhywfar/version.h>



int LCCL_ClientManager_HandleClientReady(LCCL_CLIENTMANAGER *clm,
                                         GWEN_TYPE_UINT32 rid,
                                         const char *name,
                                         GWEN_DB_NODE *dbReq) {
  LCCL_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_DB_NODE *dbRsp;
  GWEN_NETCONNECTION *conn;
  GWEN_NETTRANSPORT *tr;
  const char *p;

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

  cl=LCCL_Client_List_First(clm->clients);
  while(cl) {
    if (LCCL_Client_GetClientId(cl)==clientId)
      break;
    cl=LCCL_Client_List_Next(cl);
  } /* while */
  if (cl) {
    DBG_ERROR(0, "Client already started");
    /* TODO: Send SegResult */
    return -1;
  }

  cl=LCCL_Client_new(clientId);
  LCCL_Client_SetMaxClientLockTime(cl, clm->maxClientLockTime);
  LCCL_Client_SetMaxClientLocks(cl, clm->maxClientLocks);
  LCCL_Client_List_Add(cl, clm->clients);

  conn=GWEN_IPCManager_GetConnection(clm->ipcManager, clientId);
  assert(conn);
  LCS_Server_UseConnectionFor(clm->server, conn,
                              LCS_Connection_TypeClient,
                              clientId);
  tr=GWEN_NetConnection_GetTransportLayer(conn);
  assert(tr);

  LCCL_Client_SetUserName(cl, "nobody");
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
      LCCL_Client_SetUserName(cl, p);
    }
  }

  p=GWEN_DB_GetCharValue(dbReq, "body/application", 0, "");
  DBG_NOTICE(0, "Client \"%08x\" started (%s, Gwen %s, ChipCard %s)",
             clientId,
             p,
	     GWEN_DB_GetCharValue(dbReq, "body/GwenVersion", 0, ""),
	     GWEN_DB_GetCharValue(dbReq, "body/ChipcardVersion", 0, ""));

  LCCL_Client_SetApplicationName(cl, p);

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
  if (GWEN_IPCManager_SendResponse(clm->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response to ClientReady");
    if (GWEN_IPCManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* remove request */
  if (GWEN_IPCManager_RemoveRequest(clm->ipcManager, rid, 0)) {
    DBG_ERROR(0, "Could not remove request");
    abort();
  }

  DBG_DEBUG(0, "Response sent.");
  return 0;
}



