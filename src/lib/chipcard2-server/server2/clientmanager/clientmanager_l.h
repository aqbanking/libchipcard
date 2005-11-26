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



#ifndef CHIPCARD_SERVER_CL_CLIENTMGR_L_H
#define CHIPCARD_SERVER_CL_CLIENTMGR_L_H

typedef struct LCCL_CLIENTMANAGER  LCCL_CLIENTMANAGER;

#include "cl_client_l.h"
#include "server_l.h"
#include "lockrequest_l.h"



LCCL_CLIENTMANAGER *LCCL_ClientManager_new(LCS_SERVER *server);
void LCCL_ClientManager_free(LCCL_CLIENTMANAGER *clm);

int LCCL_ClientManager_HandleRequest(LCCL_CLIENTMANAGER *clm,
                                     GWEN_TYPE_UINT32 rid,
                                     const char *name,
                                     GWEN_DB_NODE *dbReq);

int LCCL_ClientManager_Init(LCCL_CLIENTMANAGER *clm, GWEN_DB_NODE *db);
int LCCL_ClientManager_Fini(LCCL_CLIENTMANAGER *clm, GWEN_DB_NODE *db);

void LCCL_ClientManager_ClientDown(LCCL_CLIENTMANAGER *clm,
                                   GWEN_TYPE_UINT32 clid);

int LCCL_ClientManager_Work(LCCL_CLIENTMANAGER *clm);


/** @name Notification-related Functions
 * These functions send notifications to all interested clients.
 */
/*@{*/
void LCCL_ClientManager_DriverChg(LCCL_CLIENTMANAGER *clm,
                                  GWEN_TYPE_UINT32 did,
                                  const char *driverType,
                                  const char *driverName,
                                  const char *libraryFile,
                                  LC_DRIVER_STATUS newSt,
                                  const char *reason);

void LCCL_ClientManager_ReaderChg(LCCL_CLIENTMANAGER *clm,
                                  GWEN_TYPE_UINT32 did,
                                  GWEN_TYPE_UINT32 rid,
                                  const char *readerType,
                                  const char *readerName,
                                  const char *readerInfo,
                                  LC_READER_STATUS newSt,
                                  const char *reason);

void LCCL_ClientManager_NewCard(LCCL_CLIENTMANAGER *clm, LCCO_CARD *card);

void LCCL_ClientManager_CardRemoved(LCCL_CLIENTMANAGER *clm,
                                    GWEN_TYPE_UINT32 rid,
                                    int slotNum,
                                    GWEN_TYPE_UINT32 cardNum);

void LCCL_ClientManager_ServiceChg(LCCL_CLIENTMANAGER *clm,
                                   GWEN_TYPE_UINT32 sid,
                                   const char *serviceType,
                                   const char *serviceName,
                                   LC_SERVICE_STATUS newSt,
                                   const char *reason);

int LCCL_ClientManager_GetClientCount(const LCCL_CLIENTMANAGER *clm);

int LCCL_ClientManager_CheckClientCardAccess(LCCL_CLIENTMANAGER *clm,
                                             LCCO_CARD *card,
                                             LCCL_CLIENT *cl);

void LCCL_ClientManager_DumpState(const LCCL_CLIENTMANAGER *clm);

/*@}*/


#endif /* CHIPCARD_SERVER_CL_CLIENTMGR_L_H */



