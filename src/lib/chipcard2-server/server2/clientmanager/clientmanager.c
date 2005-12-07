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
#include "fullserver_l.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


LCCL_CLIENTMANAGER *LCCL_ClientManager_new(LCS_SERVER *server) {
  LCCL_CLIENTMANAGER *clm;

  assert(server);
  GWEN_NEW_OBJECT(LCCL_CLIENTMANAGER, clm);
  clm->server=server;
  clm->ipcManager=LCS_Server_GetIpcManager(server);
  clm->clients=LCCL_Client_List_new();

  return clm;
}



void LCCL_ClientManager_free(LCCL_CLIENTMANAGER *clm) {
  if (clm) {
    LCCL_Client_List_free(clm->clients);
    GWEN_FREE_OBJECT(clm);
  }
}



int LCCL_ClientManager_Init(LCCL_CLIENTMANAGER *clm, GWEN_DB_NODE *dbConfig) {
  GWEN_DB_NODE *dbT;

  DBG_INFO(0, "Initialising client manager");
  assert(clm);

  clm->maxClientLockTime=LCCL_CLIENTMANAGER_DEF_MAX_CLIENT_LOCKTIME;
  clm->maxClientLocks=LCCL_CLIENTMANAGER_DEF_MAX_CLIENT_LOCKS;
  clm->takeCardExpireTimeout=LCCL_CLIENTMANAGER_DEF_TAKE_CARD_EXPIRE_TIMEOUT;
  clm->commandTimeout=LCCL_CLIENTMANAGER_DEF_COMMAND_TIMEOUT;

  dbT=GWEN_DB_GetGroup(dbConfig, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                       "ClientManager");
  if (dbT) {
    /* read some values */
#define LCCL_CLM_INIT_VAL(s) \
  clm->s=GWEN_DB_GetIntValue(dbT, __STRING(s), 0, clm->s);
    LCCL_CLM_INIT_VAL(maxClientLockTime)
    LCCL_CLM_INIT_VAL(maxClientLocks)
    LCCL_CLM_INIT_VAL(takeCardExpireTimeout)
    LCCL_CLM_INIT_VAL(commandTimeout)
#undef LCDM_DM_INIT_VAL
  }
  return 0;
}



int LCCL_ClientManager_Fini(LCCL_CLIENTMANAGER *clm, GWEN_DB_NODE *db) {
  return 0;
}



int LCCL_ClientManager_Work(LCCL_CLIENTMANAGER *clm) {
  LCCL_ClientManager_CheckClients(clm);
  return 0;
}



int LCCL_ClientManager_HandleRequest(LCCL_CLIENTMANAGER *clm,
                                     GWEN_TYPE_UINT32 rid,
                                     const char *name,
                                     GWEN_DB_NODE *dbReq) {
  int rv;

  assert(clm);
  assert(name);

  if (strcasecmp(name, "Client_Ready")==0) {
    rv=LCCL_ClientManager_HandleClientReady(clm, rid, name, dbReq);
  }
  else if (strcasecmp(name, "Client_SetNotify")==0) {
    rv=LCCL_ClientManager_HandleSetNotify(clm, rid, name, dbReq);
  }
  else if (strcasecmp(name, "Client_StartWait")==0) {
    rv=LCCL_ClientManager_HandleStartWait(clm, rid, name, dbReq);
  }
  else if (strcasecmp(name, "Client_StopWait")==0) {
    rv=LCCL_ClientManager_HandleStopWait(clm, rid, name, dbReq);
  }
  else if (strcasecmp(name, "Client_TakeCard")==0) {
    rv=LCCL_ClientManager_HandleTakeCard(clm, rid, name, dbReq);
  }
  else if (strcasecmp(name, "Client_ReleaseCard")==0) {
    rv=LCCL_ClientManager_HandleReleaseCard(clm, rid, name, dbReq);
  }
  else if (strcasecmp(name, "Client_CommandCard")==0) {
    rv=LCCL_ClientManager_HandleExecApdu(clm, rid, name, dbReq);
  }
  else if (strcasecmp(name, "Client_ExecCommand")==0) {
    rv=LCCL_ClientManager_HandleExecCommand(clm, rid, name, dbReq);
  }
  else if (strcasecmp(name, "Client_CardReset")==0) {
    rv=LCCL_ClientManager_HandleCardReset(clm, rid, name, dbReq);
  }
  else if (strcasecmp(name, "Client_SelectCard")==0) {
    rv=LCCL_ClientManager_HandleSelectCard(clm, rid, name, dbReq);
  }
  else if (strcasecmp(name, "Client_GetDriverVar")==0) {
    rv=LCCL_ClientManager_HandleGetDriverVar(clm, rid, name, dbReq);
  }
  else if (strcasecmp(name, "Client_LockReader")==0) {
    rv=LCCL_ClientManager_HandleLockReader(clm, rid, name, dbReq);
  }
  else if (strcasecmp(name, "Client_UnlockReader")==0) {
    rv=LCCL_ClientManager_HandleUnlockReader(clm, rid, name, dbReq);
  }
  else if (strcasecmp(name, "Client_ReaderCommand")==0) {
    rv=LCCL_ClientManager_HandleReaderCmd(clm, rid, name, dbReq);
  }
  else if (strcasecmp(name, "Client_Verify")==0) {
    rv=LCCL_ClientManager_HandleVerify(clm, rid, name, dbReq);
  }
  /* Insert more handlers here */
  else {
    DBG_INFO(0, "Command \"%s\" not handled by client manager",
             name);
    rv=1; /* not handled */
  }

  return rv;
}



void LCCL_ClientManager_DriverChg(LCCL_CLIENTMANAGER *clm,
                                  GWEN_TYPE_UINT32 did,
                                  const char *driverType,
                                  const char *driverName,
                                  const char *libraryFile,
                                  LC_DRIVER_STATUS newSt,
                                  const char *reason) {
  LCCL_ClientManager_SendDriverNotification(clm,
                                            /* if there currently is a
                                             * setNotify request in work
                                             * then listingClient would be
                                             * set, otherwise it is 0 which
                                             * means to send the notification
                                             * to all interested clients */
                                            clm->listingClient,
                                            did,
                                            driverType,
                                            driverName,
                                            libraryFile,
                                            newSt,
                                            reason);
}



void LCCL_ClientManager_ReaderChg(LCCL_CLIENTMANAGER *clm,
                                  GWEN_TYPE_UINT32 did,
                                  GWEN_TYPE_UINT32 rid,
                                  const char *readerType,
                                  const char *readerName,
                                  const char *readerInfo,
                                  LC_READER_STATUS newSt,
                                  const char *reason) {
  LCCL_ClientManager_SendReaderNotification(clm,
                                            /* if there currently is a
                                             * setNotify request in work
                                             * then listingClient would be
                                             * set, otherwise it is 0 which
                                             * means to send the notification
                                             * to all interested clients */
                                            clm->listingClient,
                                            did, rid,
                                            readerType,
                                            readerName,
                                            readerInfo,
                                            newSt,
                                            reason);
}



void LCCL_ClientManager_NewCard(LCCL_CLIENTMANAGER *clm, LCCO_CARD *card){
  if (LCCO_Card_GetStatus(card)==LC_CardStatusInserted) {
    LCCL_CLIENT *cl;

    DBG_WARN(0, "Advertising card \"%08x\" to all interested clients",
             LCCO_Card_GetCardId(card));
    cl=LCCL_Client_List_First(clm->clients);
    while(cl) {
      if (LCCL_Client_GetWaitRequestCount(cl))
        LCCL_ClientManager_SendCardAvailable(clm, cl, card);
      cl=LCCL_Client_List_Next(cl);
    } /* while */
  }
  else {
    DBG_WARN(0, "Card \"%08x\" not inserted, will not handle it",
             LCCO_Card_GetCardId(card));
  }
}



void LCCL_ClientManager_CardRemoved(LCCL_CLIENTMANAGER *clm,
                                    GWEN_TYPE_UINT32 rid,
                                    int slotNum,
                                    GWEN_TYPE_UINT32 cardNum) {
  // TODO
}



void LCCL_ClientManager_ClientDown(LCCL_CLIENTMANAGER *clm,
                                   GWEN_TYPE_UINT32 clid) {
  LCCL_CLIENT *cl;
  LCCM_CARDMANAGER *cm;
  LCCO_CARD *card;
  LCDM_DEVICEMANAGER *dm;
  GWEN_TYPE_UINT32 rid;

  cl=LCCL_Client_List_First(clm->clients);
  while(cl) {
    if (LCCL_Client_GetClientId(cl)==clid)
      break;
    cl=LCCL_Client_List_Next(cl);
  } /* while */
  if (!cl) {
    DBG_ERROR(0, "Client \"%08x\" not found", clid);
    return;
  }

  /* unregister from every card in the card manager,
   * unlock and reset all locked cards */
  cm=LCS_FullServer_GetCardManager(clm->server);
  assert(cm);

  card=LCCM_CardManager_GetFirstCard(cm);
  while(card) {
    if (!LCCM_CardManager_CheckAccess(cm, card, LCCL_Client_GetClientId(cl))){
      GWEN_TYPE_UINT32 rid;
      int rv;

      /* we own this card, so unlock it */
      rid=LCCL_ClientManager_SendResetCard(clm, card);
      if (rid) {
	/* immediately remove this request, we don't expect an answer */
	if (GWEN_IpcManager_RemoveRequest(clm->ipcManager, rid, 1)) {
	  DBG_ERROR(0, "Could not remove request");
	  abort();
        }
      }
      else {
        DBG_WARN(0, "Could not reset card \"%08x\"",
                 LCCO_Card_GetCardId(card));
      }
      rv=LCCM_CardManager_UnlockCard(cm, card, LCCL_Client_GetClientId(cl));
      if (rv) {
        DBG_WARN(0, "Could not unlock card \"%08x\"",
                 LCCO_Card_GetCardId(card));
      }
      else {
        DBG_INFO(0, "Card \"%08x\" unlocked from client",
                 LCCO_Card_GetCardId(card));
      }
    }
    card=LCCM_CardManager_GetNextCard(cm, card);
  }

  LCCM_CardManager_ClientDown(cm, LCCL_Client_GetClientId(cl));

  /* unregister from usage of every reader the client may have locked */
  dm=LCS_Server_GetDeviceManager(clm->server);
  assert(dm);

  rid=LCCL_Client_GetFirstReader(cl);
  while(rid) {
    LCDM_DeviceManager_ResumeReaderCheck(dm, rid);
    LCDM_DeviceManager_EndUseReader(dm, rid);
    rid=LCCL_Client_GetNextReader(cl);
  }

  /* unregister from general usage of readers if necessary */
  if (LCCL_Client_GetWaitRequestCount(cl)) {
    LCS_Server_EndUseReaders(clm->server, 1);
    LCCL_Client_ResetRequestCount(cl);
  }

  LCCL_Client_SetWantDestroy(cl, 1);
}



void LCCL_ClientManager_ServiceChg(LCCL_CLIENTMANAGER *clm,
                                   GWEN_TYPE_UINT32 sid,
                                   const char *serviceType,
                                   const char *serviceName,
                                   LC_SERVICE_STATUS newSt,
                                   const char *reason) {
  LCCL_ClientManager_SendServiceNotification(clm,
                                             /* if there currently is a
                                              * setNotify request in work
                                              * then listingClient would be
                                              * set, otherwise it is 0 which
                                              * means to send the notification
                                              * to all interested clients */
                                             clm->listingClient,
                                             sid,
                                             serviceType,
                                             serviceName,
                                             newSt,
                                             reason);
}



void LCCL_ClientManager_CheckClient(LCCL_CLIENTMANAGER *clm,
                                    LCCL_CLIENT *cl) {
  assert(cl);
  if (LCCL_Client_GetWantDestroy(cl)) {
    /* remove IPC node */
    GWEN_IpcManager_RemoveClient(clm->ipcManager,
                                 LCCL_Client_GetClientId(cl));

    /* finally destroy client */
    DBG_NOTICE(0, "Removing client %08x [%s/%s]",
               LCCL_Client_GetClientId(cl),
               LCCL_Client_GetApplicationName(cl),
               LCCL_Client_GetUserName(cl));
    LCCL_Client_List_Del(cl);
    LCCL_Client_free(cl);
  }
}



void LCCL_ClientManager_CheckClients(LCCL_CLIENTMANAGER *clm) {
  LCCL_CLIENT *cl;

  assert(clm);
  cl=LCCL_Client_List_First(clm->clients);
  while(cl) {
    LCCL_CLIENT *clNext;

    clNext=LCCL_Client_List_Next(cl);
    LCCL_ClientManager_CheckClient(clm, cl);
    cl=clNext;
  }
}



int LCCL_ClientManager_GetClientCount(const LCCL_CLIENTMANAGER *clm) {
  assert(clm);
  return LCCL_Client_List_GetCount(clm->clients);
}



int LCCL_ClientManager_CheckClientCardAccess(LCCL_CLIENTMANAGER *clm,
                                             LCCO_CARD *card,
                                             LCCL_CLIENT *cl) {
  LCDM_DEVICEMANAGER *dm;
  LCCM_CARDMANAGER *cm;
  LCS_LOCKMANAGER *lm;
  int rv;

  assert(clm);
  cm=LCS_FullServer_GetCardManager(clm->server);
  assert(cm);
  rv=LCCM_CardManager_CheckAccess(cm, card, LCCL_Client_GetClientId(cl));
  if (rv!=0)
    return rv;
  dm=LCS_Server_GetDeviceManager(clm->server);
  assert(dm);

  lm=LCDM_DeviceManager_GetLockManager(dm,
                                       LCCO_Card_GetReaderId(card),
                                       LCCO_Card_GetSlotNum(card));
  assert(lm);
  return LCS_LockManager_CheckAccess(lm, LCCO_Card_GetLockId(card));
}



void LCCL_ClientManager_DumpState(const LCCL_CLIENTMANAGER *clm) {
  if (!clm) {
    fprintf(stderr, "No client manager.\n");
    return;
  }
  else {
    LCCL_CLIENT *client;

    fprintf(stderr, "ClientManager\n");
    fprintf(stderr, "=====================================\n");
    fprintf(stderr, "Clients: %d\n", LCCL_Client_List_GetCount(clm->clients));
    client=LCCL_Client_List_First(clm->clients);
    while(client) {
      LCCL_Client_Dump(client, stderr, 2);
      client=LCCL_Client_List_Next(client);
    }
  }
}

















