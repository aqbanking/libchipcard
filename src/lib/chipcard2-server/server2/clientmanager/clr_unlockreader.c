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
#include "cl_request_l.h"
#include "fullserver_l.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/request.h>
#include <gwenhywfar/gwentime.h>


int LCCL_ClientManager_HandleUnlockReader(LCCL_CLIENTMANAGER *clm,
                                          GWEN_TYPE_UINT32 rid,
                                          const char *name,
                                          GWEN_DB_NODE *dbReq) {
  LCCL_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_TYPE_UINT32 readerId;
  GWEN_TYPE_UINT32 lrId;
  int cmdVer;
  int rv;
  GWEN_DB_NODE *dbRsp;
  LCDM_DEVICEMANAGER *dm;

  assert(dbReq);
  clientId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeid", 0, 0);
  assert(clientId);

  dm=LCS_Server_GetDeviceManager(clm->server);
  assert(dm);

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

  DBG_NOTICE(0, "Client %08x: UnlockReader [%s/%s]",
             clientId,
             LCCL_Client_GetApplicationName(cl),
             LCCL_Client_GetUserName(cl));

  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "data/readerid", 0, "0"),
                "%x", &readerId)) {
    DBG_ERROR(0, "Missing reader id");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Missing reader id");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "data/lockid", 0, "0"),
                "%x", &lrId)) {
    DBG_ERROR(0, "Missing lock id");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Missing lock id");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  if (lrId==0) {
    DBG_ERROR(0, "Invalid lock id");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Invalid lock id");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* check whether we have a lock on this reader */
  rv=LCDM_DeviceManager_CheckLockReaderAccess(dm, readerId, lrId);
  if (rv) {
    DBG_ERROR(0, "Reader not locked by this client");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 -rv,
                                 "Reader not locked by this client");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* unlock this reader */
  rv=LCDM_DeviceManager_UnlockReader(dm, readerId, lrId);
  if (rv) {
    DBG_ERROR(0, "Could not unlock reader");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 -rv,
                                 "Could not unlock reader");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  LCDM_DeviceManager_ResumeReaderCheck(dm, readerId);
  LCDM_DeviceManager_EndUseReader(dm, readerId);
  LCCL_Client_DelReader(cl, readerId);

  /* send response */
  dbRsp=GWEN_DB_Group_new("UnlockReaderResponse");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "code", "OK");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "text", "Reader unlocked");
  if (GWEN_IpcManager_SendResponse(clm->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response to client");
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

  return 0; /* handled */
}



