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



#ifndef CHIPCARD_SERVER2_SERVER_L_H
#define CHIPCARD_SERVER2_SERVER_L_H

#include <gwenhywfar/plugin.h>

#define LCS_MARK_SERVER 1
#define LCS_MARK_DRIVER 2
#define LCS_MARK_CLIENT 3
#define LCS_MARK_SLAVE  4


/** defaults */
/*@{*/
#define LCS_DEFAULT_DHFILE   "chipcardd-dh.pem"
#define LCS_DEFAULT_CERTFILE "chipcardd-cert.pem"

#define LCS_IPC_URL "/libchipcard2/server"
/*@}*/

/** paths */
/*@{*/
#define LCS_PATH_DESTLIB                "libchipcard2"
#define LCS_PATH_DRIVER_INFODIR         "DriverInfoDir"
#define LCS_PATH_DRIVER_EXECDIR         "DriverExecDir"

#define LCS_PATH_SERVICE_EXECDIR        "ServiceExecDir"

#define LCS_PATH_SERVER_DATADIR         "DataDir"
#define LCS_PATH_SERVER_NEWCERTDIR      "NewCertDir"
#define LCS_PATH_SERVER_TRUSTEDCERTDIR  "TrustedCertDir"
#define LCS_PATH_SERVER_LOGDIR          "LogDir"

#define LCS_REGKEY_BASE                 "Software\\Libchipcard2\\Server\\Paths"

/*@}*/

/** If this name is changed it must be changed in all plugin description
 * files of the drivers! */
#define LCS_PLUGIN_DRIVER "Libchipcard2_Driver"

/** If this name is changed it must be changed in all plugin description
 * files of the services! */
#define LCS_PLUGIN_SERVICE "Libchipcard2_Service"


#define LCS_WORKRESULT_CHANGED    0
#define LCS_WORKRESULT_UNCHANGED  1
#define LCS_WORKRESULT_ERROR     (-1)
#define LCS_WORKRESULT_RESTART   (-2)

#define LCS_INITRESULT_OK       0
#define LCS_INITRESULT_ERROR   (-1)
#define LCS_INITRESULT_RESTART  1


#include <gwenhywfar/ipc.h>
#include <gwenhywfar/requestmgr.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/db.h>
#include <gwenhywfar/buffer.h>
#include <gwenhywfar/iolayer.h>


typedef struct LCS_SERVER LCS_SERVER;
GWEN_INHERIT_FUNCTION_DEFS(LCS_SERVER)

typedef enum {
  LCS_Server_RoleStandAlone=0,
  LCS_Server_RoleMaster,
  LCS_Server_RoleSlave
} LCS_SERVER_ROLE;


#include <chipcard/chipcard.h>
#include "common/card.h"
#include "common/reader.h"
#include "connection_l.h"
#include "devicemanager/devicemanager_l.h"
#include "clientmanager/clientmanager_l.h"
#include "cardmanager/cardmanager_l.h"
#include "servicemanager/servicemanager_l.h"
#include "slavemanager/slavemanager_l.h"



LCS_SERVER *LCS_Server_new();
void LCS_Server_free(LCS_SERVER *cs);

int LCS_Server_Init(LCS_SERVER *cs, GWEN_DB_NODE *db);
int LCS_Server_Fini(LCS_SERVER *cs, GWEN_DB_NODE *db);

/** @return 1 if something could be done */
int LCS_Server_Work(LCS_SERVER *cs);


/**
 * This function is used when a client sends a WaitForCard-request.
 */
void LCS_Server_BeginUseReaders(LCS_SERVER *cs);
void LCS_Server_EndUseReaders(LCS_SERVER *cs, int count);



/**
 * This tells the server to use this connection. Basically this just sets
 * the connectionUp callback to call an internal server function which in
 * turn calls LCS_Server_ConnectionDown.
 */
void LCS_Server_UseConnectionFor(LCS_SERVER *cs,
				 GWEN_IO_LAYER *conn,
                                 LCS_CONNECTION_TYPE t,
                                 uint32_t ipcId);


/** @name Functions Called by other managers
 *
 */
/*@{*/
void LCS_Server_DriverChg(LCS_SERVER *cs,
                          uint32_t did,
                          const char *driverType,
                          const char *driverName,
                          const char *libraryFile,
                          LC_DRIVER_STATUS newSt,
                          const char *reason);

void LCS_Server_ReaderChg(LCS_SERVER *cs,
                          uint32_t did,
                          LCCO_READER *r,
                          LC_READER_STATUS newSt,
                          const char *reason);

/**
 * Make a new reader known to the system.
 */
void LCS_Server_NewReader(LCS_SERVER *cs, LCCO_READER *r);

/**
 * Makes the given new card known to the system. Right after this
 * function is called by the LCDM_DeviceManager it is free'd, so the
 * handler in this function MUST call @ref LCCO_Card_Attach if it wants
 * to use this card afterwards.
 */
void LCS_Server_NewCard(LCS_SERVER *cs, LCCO_CARD *card);

void LCS_Server_CardRemoved(LCS_SERVER *cs, LCCO_CARD *card);

void LCS_Server_ConnectionDown(LCS_SERVER *cs, uint32_t id, GWEN_IO_LAYER *conn);

void LCS_Server_ServiceChg(LCS_SERVER *cs,
                           uint32_t sid,
                           const char *serviceType,
                           const char *serviceName,
                           LC_SERVICE_STATUS newSt,
                           const char *reason);

/*@}*/



GWEN_IPCMANAGER *LCS_Server_GetIpcManager(const LCS_SERVER *cs);
GWEN_IPC_REQUEST_MANAGER *LCS_Server_GetRequestManager(const LCS_SERVER *cs);

LCDM_DEVICEMANAGER *LCS_Server_GetDeviceManager(const LCS_SERVER *cs);

LCCM_CARDMANAGER *LCS_Server_GetCardManager(const LCS_SERVER *cs);

LCSV_SERVICEMANAGER *LCS_Server_GetServiceManager(const LCS_SERVER *cs);

LCSL_SLAVEMANAGER *LCS_Server_GetSlaveManager(const LCS_SERVER *cs);


int LCS_Server_ReplaceVar(const char *path,
                          const char *var,
                          const char *value,
                          GWEN_BUFFER *nbuf);


int LCS_Server_SendErrorResponse(LCS_SERVER *cs,
                                 uint32_t rid,
                                 int code,
                                 const char *text);

int LCS_Server_CheckIpcResponse(GWEN_DB_NODE *db);


LCS_SERVER_ROLE LCS_Server_GetRole(const LCS_SERVER *cs);

void LCS_Server_DumpState(const LCS_SERVER *cs);

int LCS_Server_GetClientCount(LCS_SERVER *cs);

void LCS_Server_TriggerHwScan(LCS_SERVER *cs);


#endif /* CHIPCARD_SERVER2_SERVER_L_H */



