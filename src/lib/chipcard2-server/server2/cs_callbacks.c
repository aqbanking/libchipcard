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
#include <gwenhywfar/directory.h>


void LCS_Server__CallbackStatusChg(GWEN_NETLAYER *nl,
                                   GWEN_NETLAYER_STATUS nst) {
  if (LCS_Connection_IsOfType(nl)) {
    LCS_SERVER *server;

    if (nst==GWEN_NetLayerStatus_Disconnected) {
      DBG_NOTICE(0, "One of my connections is down");
      server=LCS_Connection_GetServer(nl);
      LCS_Server_ConnectionDown(server, nl);
    }
  }
  else {
    DBG_ERROR(0, "Hmm, not my connection...");
  }
}



void LCS_Server_DriverChg(LCS_SERVER *cs,
                          GWEN_TYPE_UINT32 did,
                          const char *driverType,
                          const char *driverName,
                          const char *libraryFile,
                          LC_DRIVER_STATUS newSt,
                          const char *reason) {
  assert(cs);
  if (cs->driverChgFn)
    cs->driverChgFn(cs, did, driverType, driverName, libraryFile,
                    newSt, reason);
}



void LCS_Server_ReaderChg(LCS_SERVER *cs,
                          GWEN_TYPE_UINT32 did,
                          GWEN_TYPE_UINT32 rid,
                          const char *readerType,
                          const char *readerName,
                          LC_READER_STATUS newSt,
                          const char *reason) {
  assert(cs);
  if (cs->readerChgFn)
    cs->readerChgFn(cs, did, rid, readerType, readerName,
                    newSt, reason);
}



void LCS_Server_NewCard(LCS_SERVER *cs, LCCO_CARD *card) {
  assert(cs);
  assert(card);

  DBG_ERROR(0, "Found this card:");
  LCCO_Card_Dump(card, stderr, 2);

  if (cs->newCardFn)
    cs->newCardFn(cs, card);
}



void LCS_Server_CardRemoved(LCS_SERVER *cs,
                            GWEN_TYPE_UINT32 rid,
                            int slotNum,
                            GWEN_TYPE_UINT32 cardNum) {
  assert(cs);
  if (cs->cardRemovedFn)
    cs->cardRemovedFn(cs, rid, slotNum, cardNum);
}



void LCS_Server__ConnectionDown(LCS_SERVER *cs, GWEN_NETLAYER *conn) {
  assert(cs);

  /* check for service connection */
  if (LCS_Connection_GetType(conn)==LCS_Connection_Type_Driver) {
    GWEN_TYPE_UINT32 ipcId;

    ipcId=GWEN_IpcManager_GetClientForNetLayer(cs->ipcManager, conn);
    if (ipcId==0) {
      DBG_ERROR(0, "IPC id for broken connection not found");
      return;
    }
    LCDM_DeviceManager_DriverIpcDown(cs->deviceManager, ipcId);
  }

}



void LCS_Server_ConnectionDown(LCS_SERVER *cs, GWEN_NETLAYER *conn) {
  assert(cs);
  if (cs->connectionDownFn)
    cs->connectionDownFn(cs, conn);
}



void LCS_Server_ServiceChg(LCS_SERVER *cs,
                           GWEN_TYPE_UINT32 sid,
                           const char *serviceType,
                           const char *serviceName,
                           LC_SERVICE_STATUS newSt,
                           const char *reason) {
  assert(cs);
  if (cs->serviceChgFn)
    cs->serviceChgFn(cs, sid, serviceType, serviceName,
                     newSt, reason);
}
















