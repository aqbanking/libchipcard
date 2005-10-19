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


#include "fullserver_p.h"
#include "connection_l.h"
#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/directory.h>


GWEN_INHERIT(LCS_SERVER, LCS_FULLSERVER)


LCS_SERVER *LCS_FullServer_new() {
  LCS_FULLSERVER *fs;
  LCS_SERVER *cs;

  cs=LCS_Server_new();
  GWEN_NEW_OBJECT(LCS_FULLSERVER, fs)
  GWEN_INHERIT_SETDATA(LCS_SERVER, LCS_FULLSERVER, cs, fs,
                       LCS_FullServer_FreeData)

  fs->cardManager=LCCM_CardManager_new(cs);
  fs->clientManager=LCCL_ClientManager_new(cs);
  fs->commandManager=LCCMD_CommandManager_new();
  fs->serviceManager=LCSV_ServiceManager_new(cs);

  fs->driverChgFn=
    LCS_Server_SetDriverChgFn(cs, LCS_FullServer_DriverChg);
  fs->readerChgFn=
    LCS_Server_SetReaderChgFn(cs, LCS_FullServer_ReaderChg);
  fs->newCardFn=
    LCS_Server_SetNewCardFn(cs, LCS_FullServer_NewCard);
  fs->cardRemovedFn=
    LCS_Server_SetCardRemovedFn(cs, LCS_FullServer_CardRemoved);
  fs->handleRequestFn=
    LCS_Server_SetHandleRequestFn(cs, LCS_FullServer_HandleRequest);
  fs->connectionDownFn=
    LCS_Server_SetConnectionDownFn(cs, LCS_FullServer_ConnectionDown);
  fs->serviceChgFn=
    LCS_Server_SetServiceChgFn(cs, LCS_FullServer_ServiceChg);

  return cs;
}



void LCS_FullServer_FreeData(void *bp, void *p) {
  LCS_FULLSERVER *fs;

  fs=(LCS_FULLSERVER*)p;

  LCSV_ServiceManager_free(fs->serviceManager);
  LCCMD_CommandManager_free(fs->commandManager);
  LCCL_ClientManager_free(fs->clientManager);
  LCCM_CardManager_free(fs->cardManager);

  GWEN_FREE_OBJECT(fs);
}



int LCS_FullServer_Init(LCS_SERVER *cs, GWEN_DB_NODE *db) {
  LCS_FULLSERVER *fs;
  int rv;

  assert(cs);
  fs=GWEN_INHERIT_GETDATA(LCS_SERVER, LCS_FULLSERVER, cs);
  assert(fs);

  rv=LCS_Server_Init(cs, db);
  if (rv) {
    DBG_INFO(0, "here (%d)", rv);
    return rv;
  }

  DBG_INFO(0, "Initializing card manager");
  rv=LCCM_CardManager_Init(fs->cardManager, db);
  if (rv) {
    DBG_INFO(0, "here (%d)", rv);
    return rv;
  }

  DBG_INFO(0, "Initializing command manager");
  rv=LCCMD_CommandManager_Init(fs->commandManager, db);
  if (rv) {
    DBG_INFO(0, "here (%d)", rv);
    return rv;
  }

  DBG_INFO(0, "Initializing client manager");
  rv=LCCL_ClientManager_Init(fs->clientManager, db);
  if (rv) {
    DBG_INFO(0, "here (%d)", rv);
    return rv;
  }

  DBG_INFO(0, "Initializing service manager");
  rv=LCSV_ServiceManager_Init(fs->serviceManager, db);
  if (rv) {
    DBG_INFO(0, "here (%d)", rv);
    return rv;
  }

  return 0;
}



int LCS_FullServer_Fini(LCS_SERVER *cs, GWEN_DB_NODE *db) {
  LCS_FULLSERVER *fs;
  int firstError=0;
  int rv;

  assert(cs);
  fs=GWEN_INHERIT_GETDATA(LCS_SERVER, LCS_FULLSERVER, cs);
  assert(fs);

  DBG_INFO(0, "Deinitializing service manager");
  rv=LCSV_ServiceManager_Fini(fs->serviceManager, db);
  if (rv && firstError==0)
    firstError=rv;

  DBG_INFO(0, "Deinitializing client manager");
  rv=LCCL_ClientManager_Fini(fs->clientManager, db);
  if (rv && firstError==0)
    firstError=rv;

  DBG_INFO(0, "Deinitializing command manager");
  rv=LCCMD_CommandManager_Fini(fs->commandManager, db);
  if (rv && firstError==0)
    firstError=rv;

  DBG_INFO(0, "Deinitializing card manager");
  rv=LCCM_CardManager_Fini(fs->cardManager, db);
  if (rv && firstError==0)
    firstError=rv;


  rv=LCS_Server_Fini(cs, db);
  if (rv && firstError==0)
    firstError=rv;

  return firstError;
}



LCCM_CARDMANAGER *LCS_FullServer_GetCardManager(const LCS_SERVER *cs) {
  LCS_FULLSERVER *fs;

  assert(cs);
  fs=GWEN_INHERIT_GETDATA(LCS_SERVER, LCS_FULLSERVER, cs);
  assert(fs);

  return fs->cardManager;
}



void LCS_FullServer_SetCardManager(LCS_SERVER *cs, LCCM_CARDMANAGER *cm) {
  LCS_FULLSERVER *fs;

  assert(cs);
  fs=GWEN_INHERIT_GETDATA(LCS_SERVER, LCS_FULLSERVER, cs);
  assert(fs);

  fs->cardManager=cm;
}



LCCMD_COMMANDMANAGER *LCS_FullServer_GetCommandManager(const LCS_SERVER *cs) {
  LCS_FULLSERVER *fs;

  assert(cs);
  fs=GWEN_INHERIT_GETDATA(LCS_SERVER, LCS_FULLSERVER, cs);
  assert(fs);

  return fs->commandManager;
}



void LCS_FullServer_SetCommandManager(LCS_SERVER *cs,
                                      LCCMD_COMMANDMANAGER *cm) {
  LCS_FULLSERVER *fs;

  assert(cs);
  fs=GWEN_INHERIT_GETDATA(LCS_SERVER, LCS_FULLSERVER, cs);
  assert(fs);

  fs->commandManager=cm;
}



LCSV_SERVICEMANAGER *LCS_FullServer_GetServiceManager(const LCS_SERVER *cs){
  LCS_FULLSERVER *fs;

  assert(cs);
  fs=GWEN_INHERIT_GETDATA(LCS_SERVER, LCS_FULLSERVER, cs);
  assert(fs);

  return fs->serviceManager;
}



void LCS_FullServer_SetServiceManager(LCS_SERVER *cs,
                                      LCSV_SERVICEMANAGER *svm) {
  LCS_FULLSERVER *fs;

  assert(cs);
  fs=GWEN_INHERIT_GETDATA(LCS_SERVER, LCS_FULLSERVER, cs);
  assert(fs);

  fs->serviceManager=svm;
}



void LCS_FullServer_DriverChg(LCS_SERVER *cs,
                              GWEN_TYPE_UINT32 did,
                              const char *driverType,
                              const char *driverName,
                              const char *libraryFile,
                              LC_DRIVER_STATUS newSt,
                              const char *reason) {
  LCS_FULLSERVER *fs;

  assert(cs);
  fs=GWEN_INHERIT_GETDATA(LCS_SERVER, LCS_FULLSERVER, cs);
  assert(fs);

  /* call previous function */
  if (fs->driverChgFn)
    fs->driverChgFn(cs, did, driverType, driverName, libraryFile,
		    newSt, reason);

  LCCL_ClientManager_DriverChg(fs->clientManager,
                               did, driverType, driverName, libraryFile,
                               newSt, reason);
}



void LCS_FullServer_ReaderChg(LCS_SERVER *cs,
                              GWEN_TYPE_UINT32 did,
                              GWEN_TYPE_UINT32 rid,
                              const char *readerType,
                              const char *readerName,
                              LC_READER_STATUS newSt,
                              const char *reason) {
  LCS_FULLSERVER *fs;

  assert(cs);
  fs=GWEN_INHERIT_GETDATA(LCS_SERVER, LCS_FULLSERVER, cs);
  assert(fs);

  /* call previous function */
  if (fs->readerChgFn)
    fs->readerChgFn(cs, did, rid, readerType, readerName, newSt, reason);

  if (newSt==LC_ReaderStatusDown ||
      newSt==LC_ReaderStatusAborted ||
      newSt==LC_ReaderStatusDisabled) {
    LCCM_CardManager_ReaderDown(fs->cardManager, rid);
  }

  LCCL_ClientManager_ReaderChg(fs->clientManager,
                               did, rid,
                               readerType, readerName,
                               newSt, reason);
}



void LCS_FullServer_NewCard(LCS_SERVER *cs, LCCO_CARD *card) {
  LCS_FULLSERVER *fs;

  assert(cs);
  fs=GWEN_INHERIT_GETDATA(LCS_SERVER, LCS_FULLSERVER, cs);
  assert(fs);

  assert(card);
  /* call previous function */
  if (fs->newCardFn)
    fs->newCardFn(cs, card);

  LCCM_CardManager_NewCard(fs->cardManager, card);
  LCCL_ClientManager_NewCard(fs->clientManager, card);
  LCCMD_CommandManager_NewCard(fs->commandManager, card);
}



void LCS_FullServer_CardRemoved(LCS_SERVER *cs,
                                GWEN_TYPE_UINT32 rid,
                                int slotNum,
                                GWEN_TYPE_UINT32 cardNum) {
  LCS_FULLSERVER *fs;

  assert(cs);
  fs=GWEN_INHERIT_GETDATA(LCS_SERVER, LCS_FULLSERVER, cs);
  assert(fs);

  /* call previous function */
  if (fs->cardRemovedFn)
    fs->cardRemovedFn(cs, rid, slotNum, cardNum);

  LCCL_ClientManager_CardRemoved(fs->clientManager, rid, slotNum, cardNum);
  LCCM_CardManager_CardRemoved(fs->cardManager, rid, slotNum, cardNum);
}



void LCS_FullServer_ConnectionDown(LCS_SERVER *cs, GWEN_NETCONNECTION *conn) {
  LCS_FULLSERVER *fs;

  assert(cs);
  fs=GWEN_INHERIT_GETDATA(LCS_SERVER, LCS_FULLSERVER, cs);
  assert(fs);

  /* call previous function (which checks for driver connections) */
  if (fs->connectionDownFn)
    fs->connectionDownFn(cs, conn);

  /* check for client connection */
  if (LCS_Connection_GetType(conn)==LCS_Connection_TypeClient) {
    GWEN_TYPE_UINT32 clientId;

    /* client is down, tell this to card manager and client manager */
    clientId=
      GWEN_IPCManager_GetClientForConnection(LCS_Server_GetIpcManager(cs),
                                             conn);
    if (clientId==0) {
      DBG_WARN(0, "Client for connection not found");
      return;
    }

    /* we must notify the service because it is basically just a client, and
     * maybe it is exactly this client here that pulled down a service, too
     */
    LCSV_ServiceManager_ConnectionDown(fs->serviceManager, clientId);
    LCCL_ClientManager_ClientDown(fs->clientManager, clientId);
  }

  /* check for service connection */
  else if (LCS_Connection_GetType(conn)==LCS_Connection_TypeService) {
    GWEN_TYPE_UINT32 ipcId;

    ipcId=GWEN_IPCManager_GetClientForConnection(LCS_Server_GetIpcManager(cs),
                                                 conn);
    if (ipcId==0) {
      DBG_ERROR(0, "IPC id for broken connection not found");
      return;
    }
    //LCSM_ServiceManager_DriverIpcDown(cs->serviceManager, ipcId);
  }
}



void LCS_FullServer_ServiceChg(LCS_SERVER *cs,
                               GWEN_TYPE_UINT32 sid,
                               const char *serviceType,
                               const char *serviceName,
                               LC_SERVICE_STATUS newSt,
                               const char *reason) {
  LCS_FULLSERVER *fs;

  assert(cs);
  fs=GWEN_INHERIT_GETDATA(LCS_SERVER, LCS_FULLSERVER, cs);
  assert(fs);

  /* call previous function */
  if (fs->serviceChgFn)
    fs->serviceChgFn(cs, sid, serviceType, serviceName,
                     newSt, reason);

  LCCL_ClientManager_ServiceChg(fs->clientManager,
                                sid, serviceType, serviceName,
                                newSt, reason);
}



int LCS_FullServer_Work(LCS_SERVER *cs) {
  LCS_FULLSERVER *fs;
  int rv;
  int done=0;

  assert(cs);
  fs=GWEN_INHERIT_GETDATA(LCS_SERVER, LCS_FULLSERVER, cs);
  assert(fs);

  /* let base class work */
  rv=LCS_Server_Work(cs);
  if (rv==1)
    done++;

  /* let card manager work */
  DBG_INFO(0, "Letting card manager work");
  rv=LCCM_CardManager_Work(fs->cardManager);
  if (rv!=0)
    done++;

  /* let client manager work */
  DBG_INFO(0, "Letting client manager work");
  rv=LCCL_ClientManager_Work(fs->clientManager);
  if (rv!=0)
    done++;

  /* let service manager work */
  DBG_INFO(0, "Letting service manager work");
  rv=LCSV_ServiceManager_Work(fs->serviceManager);
  if (rv!=0)
    done++;

  if (done)
    return 1;

  return 0;
}



int LCS_FullServer_HandleRequest(LCS_SERVER *cs,
                                 GWEN_TYPE_UINT32 rid,
                                 const char *name,
                                 GWEN_DB_NODE *dbReq) {
  LCS_FULLSERVER *fs;
  int rv;

  assert(cs);
  fs=GWEN_INHERIT_GETDATA(LCS_SERVER, LCS_FULLSERVER, cs);
  assert(fs);

  /* let my managers handle it */
  if (fs->clientManager) {
    rv=LCCL_ClientManager_HandleRequest(fs->clientManager, rid, name, dbReq);
    if (rv!=1)
      return rv;
  }

  if (fs->serviceManager) {
    rv=LCSV_ServiceManager_HandleRequest(fs->serviceManager,
                                         rid, name, dbReq);
    if (rv!=1)
      return rv;
  }

  /* otherwise let base class handle it */
  if (fs->handleRequestFn) {
    rv=fs->handleRequestFn(cs, rid, name, dbReq);
    if (rv!=1)
      return rv;
  }

  return 1;
}















