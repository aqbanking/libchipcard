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
//#include "fullserver_l.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>


int LCCL_ClientManager_HandleStopWait(LCCL_CLIENTMANAGER *clm,
                                      GWEN_TYPE_UINT32 rid,
                                      const char *name,
                                      GWEN_DB_NODE *dbReq) {
  LCCL_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  int cmdVer;

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

  cmdVer=GWEN_DB_GetIntValue(dbReq, "data/cmdver", 0, 0);

  cl=LCCL_Client_List_First(clm->clients);
  while(cl) {
    if (LCCL_Client_GetClientId(cl)==clientId)
      break;
    cl=LCCL_Client_List_Next(cl);
  } /* while */
  if (!cl) {
    DBG_ERROR(0, "Client \"%08x\" not found", clientId);
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Unknown client id");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  DBG_NOTICE(0, "Client %08x: StopWait [%s/%s]",
             clientId,
             LCCL_Client_GetApplicationName(cl),
             LCCL_Client_GetUserName(cl));

  if (LCCL_Client_GetWaitRequestCount(cl)==0) {
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Not waiting for cards");
  }
  else {
    GWEN_DB_NODE *dbRsp;

    LCCL_Client_SubWaitRequestCount(cl);
    if (LCCL_Client_GetWaitRequestCount(cl)==0)
      LCS_Server_EndUseReaders(clm->server, 1);

    /* send all cards we already know, create response */
    dbRsp=GWEN_DB_Group_new("Client_StopWaitResponse");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "code", "OK");
    GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "text", "Command executed.");
    if (GWEN_IpcManager_SendResponse(clm->ipcManager, rid, dbRsp)) {
      DBG_ERROR(0, "Could not send response to client");
      if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
        DBG_ERROR(0, "Could not remove request");
        abort();
      }
      return -1;
    }
  }

  if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
    DBG_ERROR(0, "Could not remove request");
    abort();
  }

  return 0; /* handled */
}




