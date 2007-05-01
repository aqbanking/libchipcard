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

#include "server_l.h"



struct LCS_SERVER {
  GWEN_INHERIT_ELEMENT(LCS_SERVER)

  /* vars from config file */
  int disableAutoconf;
  LCS_SERVER_ROLE role;

  GWEN_DB_NODE *dbConfig;
  GWEN_IPCMANAGER *ipcManager;
  GWEN_IPC_REQUEST_MANAGER *requestManager;
  LCDM_DEVICEMANAGER *deviceManager;
  LCCL_CLIENTMANAGER *clientManager;
  LCCM_CARDMANAGER *cardManager;
  LCSV_SERVICEMANAGER *serviceManager;
  LCSL_SLAVEMANAGER *slaveManager;

  LCS_SERVER_ASKADDCERT_FN askAddCertFn;



  /* runtime vars */
  GWEN_PLUGIN_MANAGER *driverPluginManager;
  GWEN_PLUGIN_MANAGER *servicePluginManager;
};


static
int LCS_Server__InitListener(LCS_SERVER *cs, GWEN_DB_NODE *gr);

static
int LCS_Server__InitListeners(LCS_SERVER *cs, GWEN_DB_NODE *db);

static
int LCS_Server__InitPaths(LCS_SERVER *cs, GWEN_DB_NODE *gr);


/**
 * GWEN_NetConnection callbacks.
 */
/*@{*/
static
void LCS_Server__CallbackStatusChg(GWEN_NETLAYER *nl,
                                   GWEN_NETLAYER_STATUS st);
/*@}*/


/**
 * LCDM_DeviceManager callbacks.
 */
/*@{*/
/*@}*/



static
int LCS_Server_HandleNextCommand(LCS_SERVER *cs);


static
int LCS_Server_HandleRequest(LCS_SERVER *cs,
                             GWEN_TYPE_UINT32 rid,
                             const char *name,
                             GWEN_DB_NODE *dbReq);

#endif /* CHIPCARD_SERVER2_SERVER_P_H */

