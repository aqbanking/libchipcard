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

#include <gwenhywfar/nl_ssl.h>
#include <gwenhywfar/version.h>



int LCCL_ClientManager_HandleClientReady(LCCL_CLIENTMANAGER *clm,
                                         GWEN_TYPE_UINT32 rid,
                                         const char *name,
                                         GWEN_DB_NODE *dbReq) {
  LCCL_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_DB_NODE *dbRsp;
  GWEN_NETLAYER *conn;
  GWEN_NETLAYER *nlSSL;
  const char *p;

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  if (clientId==0) {
    DBG_ERROR(0, "No client id");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  cl=LCCL_Client_List_First(clm->clients);
  while(cl) {
    if (LCCL_Client_GetClientId(cl)==clientId)
      break;
    cl=LCCL_Client_List_Next(cl);
  } /* while */
  if (cl) {
    DBG_ERROR(0, "Client already started");
    /* TODO: Send SegResult */
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  cl=LCCL_Client_new(clientId);
  LCCL_Client_SetMaxClientLockTime(cl, clm->maxClientLockTime);
  LCCL_Client_SetMaxClientLocks(cl, clm->maxClientLocks);
  LCCL_Client_List_Add(cl, clm->clients);

  conn=GWEN_IpcManager_GetNetLayer(clm->ipcManager, clientId);
  assert(conn);
  LCS_Server_UseConnectionFor(clm->server, conn,
                              LCS_Connection_Type_Client,
                              clientId);

  LCCL_Client_SetUserName(cl, "nobody");

  nlSSL=GWEN_NetLayer_FindBaseLayer(conn, GWEN_NL_SSL_NAME);
  if (nlSSL){
    DBG_INFO(0, "Got an SSL connection, checking...");
    if (GWEN_NetLayerSsl_GetIsSecure(nlSSL)) {
      const GWEN_SSLCERTDESCR *cert;

      DBG_INFO(0, "Got a secure SSL connection");
      cert=GWEN_NetLayerSsl_GetPeerCertificate(nlSSL);
      if (cert) {
        const char *p;

        p=GWEN_SslCertDescr_GetCommonName(cert);
        if (p) {
          DBG_NOTICE(0, "Verified peer is \"%s\"", p);
          LCCL_Client_SetUserName(cl, p);
        }
      }
    }
  }

  p=GWEN_DB_GetCharValue(dbReq, "data/application", 0, "");
  DBG_NOTICE(0, "Client \"%08x\" started (%s, Gwen %s, ChipCard %s)",
             clientId,
             p,
	     GWEN_DB_GetCharValue(dbReq, "data/GwenVersion", 0, ""),
	     GWEN_DB_GetCharValue(dbReq, "data/ChipcardVersion", 0, ""));

  LCCL_Client_SetApplicationName(cl, p);

  /* create response for client */
  dbRsp=GWEN_DB_Group_new("Client_ReadyResponse");
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
  if (GWEN_IpcManager_SendResponse(clm->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response to ClientReady");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* remove request */
  if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
    DBG_ERROR(0, "Could not remove request");
    abort();
  }

  DBG_DEBUG(0, "Response sent.");
  return 0;
}



