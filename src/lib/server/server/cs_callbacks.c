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
      DBG_NOTICE(0,
                 "One of my connections is down. "
                 "Don't worry just yet, this might be ok...");
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

  if (cs->role==LCS_Server_RoleSlave) {
    // TODO
  }
  else {
    LCCL_ClientManager_DriverChg(cs->clientManager,
                                 did, driverType, driverName, libraryFile,
                                 newSt, reason);
  }
}



void LCS_Server_ReaderChg(LCS_SERVER *cs,
                          GWEN_TYPE_UINT32 did,
                          LCCO_READER *r,
                          LC_READER_STATUS newSt,
                          const char *reason) {
  assert(cs);

  if (newSt==LC_ReaderStatusDown ||
      newSt==LC_ReaderStatusAborted ||
      newSt==LC_ReaderStatusDisabled ||
      newSt==LC_ReaderStatusHwDel) {
    LCCM_CardManager_ReaderDown(cs->cardManager,
                                LCCO_Reader_GetReaderId(r));
  }

  if (cs->role==LCS_Server_RoleSlave) {
    LCSL_SlaveManager_ReaderChg(cs->slaveManager,
                                did, r,
                                newSt, reason);
  }
  else {
    LCCL_ClientManager_ReaderChg(cs->clientManager,
                                 did, r,
                                 newSt, reason);
  }
}



void LCS_Server_ServiceChg(LCS_SERVER *cs,
                               GWEN_TYPE_UINT32 sid,
                               const char *serviceType,
                               const char *serviceName,
                               LC_SERVICE_STATUS newSt,
                               const char *reason) {
  assert(cs);

  if (cs->role==LCS_Server_RoleSlave) {
    // TODO
  }
  else {
    LCCL_ClientManager_ServiceChg(cs->clientManager,
                                  sid, serviceType, serviceName,
                                  newSt, reason);
  }
}



void LCS_Server_NewReader(LCS_SERVER *cs, LCCO_READER *r) {
  assert(cs);

  if (cs->role==LCS_Server_RoleSlave) {
    LCSL_SlaveManager_NewReader(cs->slaveManager, r);
  }
}



void LCS_Server_NewCard(LCS_SERVER *cs, LCCO_CARD *card) {
  assert(cs);

  LCCM_CardManager_NewCard(cs->cardManager, card);
  if (cs->role==LCS_Server_RoleSlave) {
    LCSL_SlaveManager_NewCard(cs->slaveManager, card);
  }
  else {
    LCCL_ClientManager_NewCard(cs->clientManager, card);
  }
}



void LCS_Server_CardRemoved(LCS_SERVER *cs, LCCO_CARD *card) {
  assert(cs);

  if (cs->role==LCS_Server_RoleSlave) {
    LCSL_SlaveManager_CardRemoved(cs->slaveManager, card);
  }
  else {
    LCCL_ClientManager_CardRemoved(cs->clientManager, card);
  }
  LCCM_CardManager_CardRemoved(cs->cardManager, card);
}



void LCS_Server_ConnectionDown(LCS_SERVER *cs, GWEN_NETLAYER *conn) {
  assert(cs);

  if (LCS_Connection_IsOfType(conn)) {
    /* check for driver connection */
    if (LCS_Connection_GetType(conn)==LCS_Connection_Type_Driver) {
      GWEN_TYPE_UINT32 ipcId;

      ipcId=GWEN_IpcManager_GetClientForNetLayer(cs->ipcManager, conn);
      if (ipcId==0) {
        DBG_ERROR(0, "IPC id for broken connection not found");
        return;
      }
      LCDM_DeviceManager_DriverIpcDown(cs->deviceManager, ipcId);
    }
    else {
      if (cs->role==LCS_Server_RoleSlave) {
        if (LCS_Connection_GetType(conn)==LCS_Connection_Type_Master) {
          LCSL_SlaveManager_ConnectionDown(cs->slaveManager, conn);
        }
      }
      else {
        /* check for service connection */
	if (LCS_Connection_GetType(conn)==LCS_Connection_Type_Service ||
	    LCS_Connection_GetType(conn)==LCS_Connection_Type_Client) {
	  GWEN_TYPE_UINT32 clientId;

	  /* client is down, tell this to card manager and client manager */
	  clientId=
	    GWEN_IpcManager_GetClientForNetLayer(cs->ipcManager,
						 conn);
	  if (clientId==0) {
	    DBG_WARN(0, "Client for connection not found");
	    return;
	  }

	  /* we must notify the service because it is basically just a client,
	   * and maybe it is exactly this client here that pulled down a
	   * service, too
	   */
	  LCSV_ServiceManager_ConnectionDown(cs->serviceManager, clientId);
	  LCCL_ClientManager_ClientDown(cs->clientManager, clientId);
	  LCDM_DeviceManager_ClientDown(cs->deviceManager, clientId);
	}
      }
    }
  }
}



GWEN_NL_SSL_ASKADDCERT_RESULT
LCS_Server_AskAddCert(GWEN_NETLAYER *nl,
                      const GWEN_SSLCERTDESCR *cert,
                      void *user_data) {
  LCS_SERVER *cs;

  cs=(LCS_SERVER*)user_data;
  assert(cs);
  if (cs->askAddCertFn)
    return cs->askAddCertFn(cs, nl, cert);
  return GWEN_NetLayerSsl_AskAddCertResult_No;
}
















