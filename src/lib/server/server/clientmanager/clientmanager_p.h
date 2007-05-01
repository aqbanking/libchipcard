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



#ifndef CHIPCARD_SERVER_CLIENTMGR_P_H
#define CHIPCARD_SERVER_CLIENTMGR_P_H

#define LCCL_CLIENTMANAGER_DEF_MAX_CLIENT_LOCKTIME      300
#define LCCL_CLIENTMANAGER_DEF_MAX_CLIENT_LOCKS         6
#define LCCL_CLIENTMANAGER_DEF_TAKE_CARD_EXPIRE_TIMEOUT 120
#define LCCL_CLIENTMANAGER_DEF_COMMAND_TIMEOUT          60

#include "clientmanager_l.h"


struct LCCL_CLIENTMANAGER {
  LCS_SERVER *server;
  GWEN_IPCMANAGER *ipcManager;
  LCCL_CLIENT_LIST *clients;

  LCCL_CLIENT *listingClient;

  int maxClientLockTime;
  int maxClientLocks;
  int takeCardExpireTimeout;
  int commandTimeout;

};


static
int LCCL_ClientManager_HandleClientReady(LCCL_CLIENTMANAGER *clm,
                                         GWEN_TYPE_UINT32 rid,
                                         const char *name,
                                         GWEN_DB_NODE *dbReq);

static
GWEN_TYPE_UINT32 LCCL_ClientManager_GetNotificationMask(const char *ntype,
                                                        const char *ncode);

static
int LCCL_ClientManager__SendNotification(LCCL_CLIENTMANAGER *clm,
                                         const LCCL_CLIENT *cl,
                                         const char *ntype,
                                         const char *ncode,
                                         GWEN_DB_NODE *dbData);

static
int LCCL_ClientManager_SendNotification(LCCL_CLIENTMANAGER *clm,
                                        const LCCL_CLIENT *cl,
                                        const char *ntype,
                                        const char *ncode,
                                        GWEN_DB_NODE *dbData);

static
int LCCL_ClientManager_SendDriverNotification(LCCL_CLIENTMANAGER *clm,
                                              const LCCL_CLIENT *cl,
                                              GWEN_TYPE_UINT32 did,
                                              const char *driverType,
                                              const char *driverName,
                                              const char *libraryFile,
                                              LC_DRIVER_STATUS dst,
                                              const char *reason);

static
int LCCL_ClientManager_SendReaderNotification(LCCL_CLIENTMANAGER *clm,
                                              const LCCL_CLIENT *cl,
                                              GWEN_TYPE_UINT32 did,
                                              LCCO_READER *r,
                                              LC_READER_STATUS rst,
                                              const char *reason);

static
int LCCL_ClientManager_SendServiceNotification(LCCL_CLIENTMANAGER *clm,
                                               const LCCL_CLIENT *cl,
                                               GWEN_TYPE_UINT32 did,
                                               const char *serviceType,
                                               const char *serviceName,
                                               LC_SERVICE_STATUS st,
                                               const char *reason);

static
int LCCL_ClientManager_SendCardNotification(LCCL_CLIENTMANAGER *clm,
                                            const LCCL_CLIENT *cl,
                                            GWEN_TYPE_UINT32 rid,
                                            int slotNum,
                                            GWEN_TYPE_UINT32 cardNum,
                                            LC_CARD_STATUS status,
                                            const char *reason);


static
int LCCL_ClientManager_HandleSetNotify(LCCL_CLIENTMANAGER *clm,
                                       GWEN_TYPE_UINT32 rid,
                                       const char *name,
                                       GWEN_DB_NODE *dbReq);

static
int LCCL_ClientManager_HandleStartWait(LCCL_CLIENTMANAGER *clm,
                                       GWEN_TYPE_UINT32 rid,
                                       const char *name,
                                       GWEN_DB_NODE *dbReq);

static
int LCCL_ClientManager_SendCardAvailable(LCCL_CLIENTMANAGER *clm,
                                         LCCL_CLIENT *cl,
                                         LCCO_CARD *card);

static
int LCCL_ClientManager__SendInitialCards(LCCL_CLIENTMANAGER *clm,
                                         LCCL_CLIENT *cl);

static
int LCCL_ClientManager_HandleStopWait(LCCL_CLIENTMANAGER *clm,
                                      GWEN_TYPE_UINT32 rid,
                                      const char *name,
                                      GWEN_DB_NODE *dbReq);

static
int LCCL_ClientManager_HandleTakeCard(LCCL_CLIENTMANAGER *clm,
                                      GWEN_TYPE_UINT32 rid,
                                      const char *name,
                                      GWEN_DB_NODE *dbReq);

static
int LCCL_ClientManager_WorkTakeCard(GWEN_IPC_REQUEST *st);


static
int LCCL_ClientManager_HandleReleaseCard(LCCL_CLIENTMANAGER *clm,
                                         GWEN_TYPE_UINT32 rid,
                                         const char *name,
                                         GWEN_DB_NODE *dbReq);


static
int LCCL_ClientManager_HandleExecApdu(LCCL_CLIENTMANAGER *clm,
                                      GWEN_TYPE_UINT32 rid,
                                      const char *name,
                                      GWEN_DB_NODE *dbReq);

static
int LCCL_ClientManager_WorkExecApdu(GWEN_IPC_REQUEST *req);

static
int LCCL_ClientManager_WorkCardCommand(GWEN_IPC_REQUEST *req);


static
GWEN_TYPE_UINT32 LCCL_ClientManager_SendResetCard(LCCL_CLIENTMANAGER *clm,
                                                  LCCO_CARD *card);

static
int LCCL_ClientManager_HandleGetDriverVar(LCCL_CLIENTMANAGER *clm,
                                          GWEN_TYPE_UINT32 rid,
                                          const char *name,
                                          GWEN_DB_NODE *dbReq);

static
int LCCL_ClientManager_HandleLockReader(LCCL_CLIENTMANAGER *clm,
                                        GWEN_TYPE_UINT32 rid,
                                        const char *name,
                                        GWEN_DB_NODE *dbReq);

static
int LCCL_ClientManager_WorkLockReader(GWEN_IPC_REQUEST *req);


static
int LCCL_ClientManager_HandleUnlockReader(LCCL_CLIENTMANAGER *clm,
                                          GWEN_TYPE_UINT32 rid,
                                          const char *name,
                                          GWEN_DB_NODE *dbReq);


static
int LCCL_ClientManager_HandleReaderCmd(LCCL_CLIENTMANAGER *clm,
                                       GWEN_TYPE_UINT32 rid,
                                       const char *name,
                                       GWEN_DB_NODE *dbReq);
static
int LCCL_ClientManager_WorkClientReaderCmd(GWEN_IPC_REQUEST *req);

static
int LCCL_ClientManager_WorkDriverReaderCmd(GWEN_IPC_REQUEST *req);


static
void LCCL_ClientManager_CheckClient(LCCL_CLIENTMANAGER *clm,
                                    LCCL_CLIENT *cl);
static
void LCCL_ClientManager_CheckClients(LCCL_CLIENTMANAGER *clm);

static
void LCCL_ClientManager_RemoveClientRequests(LCCL_CLIENTMANAGER *clm,
					     LCCL_CLIENT *cl,
					     GWEN_IPC_REQUEST_LIST *rql);


#endif /* CHIPCARD_SERVER_CL_CLIENTMGR_P_H */
