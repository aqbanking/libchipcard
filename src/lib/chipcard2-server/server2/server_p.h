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



#ifndef CHIPCARD_SERVER2_SERVER_P_H
#define CHIPCARD_SERVER2_SERVER_P_H

#define LCS_MARK_SERVER 1
#define LCS_MARK_DRIVER 2
#define LCS_MARK_CLIENT 3


#include "server_l.h"



struct LCS_SERVER {
  GWEN_INHERIT_ELEMENT(LCS_SERVER)

  /* vars from config file */
  int disableAutoconf;

  GWEN_DB_NODE *dbConfig;
  GWEN_IPCMANAGER *ipcManager;
  GWEN_IPC_REQUEST_MANAGER *requestManager;
  LCDM_DEVICEMANAGER *deviceManager;

  LCS_SERVER_DRIVER_CHG_FN driverChgFn;
  LCS_SERVER_READER_CHG_FN readerChgFn;
  LCS_SERVER_NEWCARD_FN newCardFn;
  LCS_SERVER_CARDREMOVED_FN cardRemovedFn;
  LCS_SERVER_CONNECTION_DOWN_FN connectionDownFn;
  LCS_SERVER_SERVICE_CHG_FN serviceChgFn;

  LCS_SERVER_HANDLEREQUEST_FN handleRequestFn;

  /* runtime vars */
  GWEN_PLUGIN_MANAGER *driverPluginManager;
  GWEN_PLUGIN_MANAGER *servicePluginManager;
};


int LCS_Server__InitListener(LCS_SERVER *cs, GWEN_DB_NODE *gr);
int LCS_Server__InitListeners(LCS_SERVER *cs, GWEN_DB_NODE *db);

int LCS_Server__InitPaths(LCS_SERVER *cs, GWEN_DB_NODE *gr);


/**
 * GWEN_NetConnection callbacks.
 */
/*@{*/
void LCS_Server__CallbackUp(GWEN_NETCONNECTION *conn);
void LCS_Server__CallbackDown(GWEN_NETCONNECTION *conn);
/*@}*/


/**
 * LCDM_DeviceManager callbacks.
 */
/*@{*/
/*@}*/



int LCS_Server_HandleNextCommand(LCS_SERVER *cs);


int LCS_Server__HandleRequest(LCS_SERVER *cs,
                              GWEN_TYPE_UINT32 rid,
                              const char *name,
                              GWEN_DB_NODE *dbReq);

int LCS_Server_HandleRequest(LCS_SERVER *cs,
                             GWEN_TYPE_UINT32 rid,
                             const char *name,
                             GWEN_DB_NODE *dbReq);

void LCS_Server__ConnectionDown(LCS_SERVER *cs, GWEN_NETCONNECTION *conn);


#endif /* CHIPCARD_SERVER2_SERVER_P_H */

