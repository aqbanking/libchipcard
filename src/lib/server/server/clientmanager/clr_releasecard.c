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
#include "server_l.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/request.h>
#include <gwenhywfar/gwentime.h>


int LCCL_ClientManager_HandleReleaseCard(LCCL_CLIENTMANAGER *clm,
                                         GWEN_TYPE_UINT32 rid,
                                         const char *name,
                                         GWEN_DB_NODE *dbReq) {
  LCCL_CLIENT *cl;
  GWEN_TYPE_UINT32 clientId;
  GWEN_TYPE_UINT32 cardId;
  int cmdVer;
  LCCM_CARDMANAGER *cm;
  LCCO_CARD *card;
  int rv;
  GWEN_DB_NODE *dbRsp;
  GWEN_TYPE_UINT32 trid;
  LCDM_DEVICEMANAGER *dm;
  LCS_LOCKMANAGER *lm;
  GWEN_TYPE_UINT32 lrId;

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

  cm=LCS_Server_GetCardManager(clm->server);
  assert(cm);

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

  DBG_NOTICE(0, "Client %08x: ReleaseCard [%s/%s]",
             clientId,
             LCCL_Client_GetApplicationName(cl),
             LCCL_Client_GetUserName(cl));

  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "data/cardid", 0, "0"),
                "%x", &cardId)) {
    DBG_ERROR(0, "Missing card id");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Missing card id");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* get referenced card */
  card=LCCM_CardManager_FindCard(cm, cardId);
  if (!card) {
    DBG_ERROR(0, "Card not found");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Card not found");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* check whether we have a lock on this card */
  rv=LCCL_ClientManager_CheckClientCardAccess(clm, card, cl);
  if (rv) {
    DBG_ERROR(0, "Card not locked by this client");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 -rv,
                                 "Card not locked by this client");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* reset this card */
  trid=LCCL_ClientManager_SendResetCard(clm, card);
  if (trid) {
    /* immediately remove this request, we don't expect an answer */
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, trid, 1)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
  }
  else {
    DBG_WARN(0, "Could not reset card \"%08x\"",
             LCCO_Card_GetCardId(card));
  }

  /* unlock slot */
  lm=LCDM_DeviceManager_GetLockManager(dm,
                                       LCCO_Card_GetReaderId(card),
                                       LCCO_Card_GetSlotNum(card));
  assert(dm);
  lrId=LCCO_Card_GetLockId(card);
  assert(lrId);
  rv=LCS_LockManager_Unlock(lm, lrId);
  LCCO_Card_SetLockId(card, 0);
  if (rv) {
    DBG_WARN(0, "Could not unlock slot (%d)", rv);
  }

  /* unlock this card */
  rv=LCCM_CardManager_UnlockCard(cm, card, clientId);
  if (rv) {
    DBG_ERROR(0, "Could not unlock card");
    LCS_Server_SendErrorResponse(clm->server, rid,
                                 -rv,
                                 "Could not unlock card");
    if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* send response */
  dbRsp=GWEN_DB_Group_new("Client_ReleaseCardResponse");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "code", "OK");
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "text", "Card released");
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



