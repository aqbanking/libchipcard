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



int LCS_Server__HandleRequest(LCS_SERVER *cs,
                              GWEN_TYPE_UINT32 rid,
                              const char *name,
                              GWEN_DB_NODE *dbReq) {
  int rv;

  /* let managers handle this request (DeviceManager only for now) */
  rv=LCDM_DeviceManager_HandleRequest(cs->deviceManager,
                                      rid,
                                      name,
                                      dbReq);
  return rv;
}



int LCS_Server_HandleRequest(LCS_SERVER *cs,
                             GWEN_TYPE_UINT32 rid,
                             const char *name,
                             GWEN_DB_NODE *dbReq) {
  DBG_INFO(0, "Incoming request \"%s\"", name);
  assert(cs);
  if (cs->handleRequestFn)
    return cs->handleRequestFn(cs, rid, name, dbReq);
  return 1;
}



int LCS_Server_HandleNextCommand(LCS_SERVER *cs) {
  const char *name;
  int rv;
  GWEN_TYPE_UINT32 ridNext;
  GWEN_DB_NODE *dbReq;

  assert(cs);

  ridNext=GWEN_IPCManager_GetNextInRequest(cs->ipcManager, 0);
  if (!ridNext) {
    DBG_VERBOUS(0, "No incoming request");
    return 1;
  }
  dbReq=GWEN_IPCManager_GetInRequestData(cs->ipcManager, ridNext);
  assert(dbReq);
  name=GWEN_DB_GetCharValue(dbReq, "command/vars/cmd", 0, 0);
  if (!name) {
    DBG_ERROR(0, "Bad IPC command (no command name), discarding");
    if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, ridNext, 0)) {
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
      if (GWEN_IPCManager_RemoveRequest(cs->ipcManager, ridNext, 0)) {
        DBG_ERROR(0, "Could not remove request");
        abort();
      }
    }
  }

  return rv;
}



int LCS_Server_Work(LCS_SERVER *cs) {
  int done=0;
  int rv;

  for (;;) {
    DBG_VERBOUS(0, "Letting IPC manager work");
    rv=GWEN_IPCManager_Work(cs->ipcManager, 10);
    if (rv==0)
      done++;
    else
      break;
  }

  for (;;) {
    DBG_VERBOUS(0, "Handling incoming commands");
    rv=LCS_Server_HandleNextCommand(cs);
    if (rv==0)
      done++;
    else
      break;
  }

  DBG_VERBOUS(0, "Letting device manager work");
  rv=LCDM_DeviceManager_Work(cs->deviceManager);
  if (rv!=0)
    done++;

  DBG_VERBOUS(0, "Letting request manager work");
  rv=GWEN_IpcRequestManager_Work(cs->requestManager);
  if (rv!=1) /* "1" is correct here! */
    done++;

  if (done)
    return 1;
  return 0;
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
                                 GWEN_NETCONNECTION *conn,
                                 LCS_CONNECTION_TYPE t,
                                 GWEN_TYPE_UINT32 ipcId) {
  LCS_Connection_TakeOver(conn);
  LCS_Connection_SetType(conn, t);
  LCS_Connection_SetServer(conn, cs);

  GWEN_IPCManager_SetDownFn(cs->ipcManager, ipcId, LCS_Server__CallbackDown);
}








