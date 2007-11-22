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


#include "server_p.h"
#include "connection_l.h"
#include <gwenhywfar/misc.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/buffer.h>



int LCS_Server_HandleNextCommand(LCS_SERVER *cs) {
  const char *name;
  int rv;
  uint32_t ridNext;
  GWEN_DB_NODE *dbReq;

  assert(cs);

  ridNext=GWEN_IpcManager_GetNextInRequest(cs->ipcManager, 0);
  if (!ridNext) {
    DBG_VERBOUS(0, "No incoming request");
    return 1;
  }
  dbReq=GWEN_IpcManager_GetInRequestData(cs->ipcManager, ridNext);
  assert(dbReq);
  name=GWEN_DB_GetCharValue(dbReq, "ipc/cmd", 0, 0);
  if (!name) {
    DBG_ERROR(0, "Bad IPC command (no command name), discarding");
    if (GWEN_IpcManager_RemoveRequest(cs->ipcManager, ridNext, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    rv=-1;
  }
  else {
    /* let managers handle this request */
    rv=LCS_Server_HandleRequest(cs, ridNext, name, dbReq);

    if (rv==1) {
      DBG_ERROR(0, "Request \"%s\" not handled.", name);
      LCS_Server_SendErrorResponse(cs, ridNext, LC_ERROR_NOT_SUPPORTED,
                                   "Unknown command");
      if (GWEN_IpcManager_RemoveRequest(cs->ipcManager, ridNext, 0)) {
        DBG_ERROR(0, "Could not remove request");
        abort();
      }
    }
  }

  return rv;
}



int LCS_Server__Work(LCS_SERVER *cs) {
  int done=0;
  int rv;

  for (;;) {
    DBG_VERBOUS(0, "Letting IPC manager work");
    rv=GWEN_IpcManager_Work(cs->ipcManager);
    if (rv==0) {
      DBG_VERBOUS(0, "change reported");
      done++;
    }
    else
      break;
  }

  for (;;) {
    DBG_VERBOUS(0, "Handling incoming commands");
    rv=LCS_Server_HandleNextCommand(cs);
    if (rv==0) {
      DBG_VERBOUS(0, "change reported");
      done++;
    }
    else
      break;
  }

  DBG_VERBOUS(0, "Letting request manager work");
  rv=GWEN_IpcRequestManager_Work(cs->requestManager);
  if (rv!=1) { /* "1" is correct here! */
    DBG_VERBOUS(0, "change reported");
    done++;
  }

  if (done) {
    DBG_VERBOUS(0, "Reporting change.");
    return 1;
  }
  return 0;
}



int LCS_Server_Work(LCS_SERVER *cs) {
  int rv;
  int done=0;

  assert(cs);

  /* let IPC work */
  rv=LCS_Server__Work(cs);
  if (rv==1)
    done++;

  /* device manager work */
  DBG_VERBOUS(0, "Letting device manager work");
  rv=LCDM_DeviceManager_Work(cs->deviceManager);
  if (rv!=0) {
    DBG_VERBOUS(0, "change reported");
    done++;
  }

  /* let card manager work */
  DBG_VERBOUS(0, "Letting card manager work");
  rv=LCCM_CardManager_Work(cs->cardManager);
  if (rv!=0)
    done++;

  if (cs->role==LCS_Server_RoleSlave) {
    DBG_VERBOUS(0, "Letting slave manager work");
    rv=LCSL_SlaveManager_Work(cs->slaveManager);
    if (rv==LCS_WORKRESULT_RESTART) {
      DBG_INFO(0, "SlaveManager wants to restart the server");
      return rv;
    }
    if (rv!=0) {
      DBG_VERBOUS(0, "change reported");
      done++;
    }
  }
  else {
    /* let client manager work */
    DBG_VERBOUS(0, "Letting client manager work");
    rv=LCCL_ClientManager_Work(cs->clientManager);
    if (rv!=0) {
      DBG_VERBOUS(0, "change reported");
      done++;
    }

    /* let service manager work */
    DBG_VERBOUS(0, "Letting service manager work");
    rv=LCSV_ServiceManager_Work(cs->serviceManager);
    if (rv!=0) {
      DBG_VERBOUS(0, "change reported");
      done++;
    }
  }

  if (done)
    return LCS_WORKRESULT_UNCHANGED;

  return 0;
}



int LCS_Server_HandleRequest(LCS_SERVER *cs,
                             uint32_t rid,
                             const char *name,
                             GWEN_DB_NODE *dbReq) {
  int rv;

  assert(cs);

  /* let my managers handle it */
  if (cs->deviceManager) {
    rv=LCDM_DeviceManager_HandleRequest(cs->deviceManager,
                                        rid,
                                        name,
                                        dbReq);
    if (rv!=1)
      return rv;
  }

  if (LCS_Server_GetRole(cs)==LCS_Server_RoleSlave) {
    if (cs->slaveManager) {
      rv=LCSL_SlaveManager_HandleRequest(cs->slaveManager, rid, name, dbReq);
      if (rv!=1)
        return rv;
    }
  }
  else {
    if (cs->clientManager) {
      rv=LCCL_ClientManager_HandleRequest(cs->clientManager, rid, name, dbReq);
      if (rv!=1)
        return rv;
    }

    if (cs->serviceManager) {
      rv=LCSV_ServiceManager_HandleRequest(cs->serviceManager,
                                           rid, name, dbReq);
      if (rv!=1)
        return rv;
    }
  }

  DBG_WARN(0, "Command \"%s\" not handled", name);

  return 1;
}



int LCS_Server_GetClientCount(LCS_SERVER *cs) {
  assert(cs);

  assert(cs->clientManager);
  return LCCL_ClientManager_GetClientCount(cs->clientManager);
}




void LCS_Server_BeginUseReaders(LCS_SERVER *cs) {
  assert(cs);
  assert(cs->deviceManager);
  LCDM_DeviceManager_BeginUseReaders(cs->deviceManager);
}



void LCS_Server_EndUseReaders(LCS_SERVER *cs, int count) {
  assert(cs);
  assert(cs->deviceManager);
  LCDM_DeviceManager_EndUseReaders(cs->deviceManager, count);
}



void LCS_Server_UseConnectionFor(LCS_SERVER *cs,
				 GWEN_IO_LAYER *conn,
                                 LCS_CONNECTION_TYPE t,
                                 uint32_t ipcId) {
  LCS_Connection_TakeOver(conn);
  LCS_Connection_SetType(conn, t);
  LCS_Connection_SetServer(conn, cs);
}








