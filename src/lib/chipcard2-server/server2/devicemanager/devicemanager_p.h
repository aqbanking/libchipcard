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


#ifndef CHIPCARD_SERVER_DEVICEMANAGER_P_H
#define CHIPCARD_SERVER_DEVICEMANAGER_P_H


#include "devicemanager_l.h"
#include "dm_driver_l.h"
#include "dm_reader_l.h"

#include <gwenhywfar/db.h>


#define LCDM_DEVICEMANAGER_DEF_DRIVER_START_DELAY     1
#define LCDM_DEVICEMANAGER_DEF_DRIVER_START_TIMEOUT   30
#define LCDM_DEVICEMANAGER_DEF_DRIVER_STOP_TIMEOUT    10
#define LCDM_DEVICEMANAGER_DEF_DRIVER_IDLE_TIMEOUT    30
#define LCDM_DEVICEMANAGER_DEF_DRIVER_RESTART_TIME    10

#define LCDM_DEVICEMANAGER_DEF_READER_START_DELAY      2
#define LCDM_DEVICEMANAGER_DEF_READER_START_TIMEOUT   30
#define LCDM_DEVICEMANAGER_DEF_READER_IDLE_TIMEOUT    30
#define LCDM_DEVICEMANAGER_DEF_READER_RESTART_TIME    30
#define LCDM_DEVICEMANAGER_DEF_READER_COMMAND_TIMEOUT 60


struct LCDM_DEVICEMANAGER {
  GWEN_INHERIT_ELEMENT(LCDM_DEVICEMANAGER)
  LCS_SERVER *server;
  GWEN_IPCMANAGER *ipcManager;

  /* data from config file */
  unsigned int driverStartDelay;
  unsigned int driverStartTimeout;
  unsigned int driverStopTimeout;
  unsigned int readerStartDelay;
  unsigned int readerStartTimeout;

  unsigned int driverIdleTimeout;
  unsigned int driverRestartTime;
  unsigned int readerIdleTimeout;
  unsigned int readerCommandTimeout;
  unsigned int readerRestartTime;

  char *addrTypeForDrivers;
  char *addrAddrForDrivers;
  int addrPortForDrivers;

  int allowRemote;

  /* runtime vars */
  GWEN_DB_NODE *dbDrivers;
  GWEN_DB_NODE *dbConfigDrivers;

  LCDM_DRIVER_LIST *drivers;
  LCDM_READER_LIST *readers;

  int nextNewPort;

  int readerUsage;
};



int LCDM_DeviceManager_ReloadDrivers(LCDM_DEVICEMANAGER *dm);

void LCDM_DeviceManager_DriverChg(LCDM_DEVICEMANAGER *dm,
                                  GWEN_TYPE_UINT32 did,
                                  const char *driverType,
                                  const char *driverName,
                                  const char *libraryFile,
                                  LC_DRIVER_STATUS newSt,
                                  const char *reason);


void LCDM_DeviceManager_ReaderChg(LCDM_DEVICEMANAGER *dm,
                                  GWEN_TYPE_UINT32 rid,
                                  const char *readerType,
                                  const char *readerName,
                                  LC_READER_STATUS newSt,
                                  const char *reason);

void LCDM_DeviceManager_CardChg(LCDM_DEVICEMANAGER *dm,
                                LCCO_CARD *card,
                                int inserted);

void LCDM_DeviceManager_AbandonDriver(LCDM_DEVICEMANAGER *dm,
                                      LCDM_DRIVER *d,
                                      LC_DRIVER_STATUS newSt,
                                      const char *reason);

void LCDM_DeviceManager_AbandonReader(LCDM_DEVICEMANAGER *dm,
                                      LCDM_READER *r,
                                      LC_READER_STATUS newSt,
                                      const char *reason);


GWEN_TYPE_UINT32 LCDM_DeviceManager_SendStartReader(LCDM_DEVICEMANAGER *dm,
                                                    const LCDM_READER *r);
GWEN_TYPE_UINT32 LCDM_DeviceManager_SendStopReader(LCDM_DEVICEMANAGER *dm,
                                                   const LCDM_READER *r);
GWEN_TYPE_UINT32 LCDM_DeviceManager_SendStopDriver(LCDM_DEVICEMANAGER *dm,
                                                   const LCDM_DRIVER *d);


int LCDM_DeviceManager_CheckDriver(LCDM_DEVICEMANAGER *dm, LCDM_DRIVER *d);
int LCDM_DeviceManager_CheckReader(LCDM_DEVICEMANAGER *dm, LCDM_READER *r);

int LCDM_DeviceManager_CheckDrivers(LCDM_DEVICEMANAGER *dm);
int LCDM_DeviceManager__Work(LCDM_DEVICEMANAGER *dm);


int LCDM_DeviceManager_HandleDriverReady(LCDM_DEVICEMANAGER *dm,
                                         GWEN_TYPE_UINT32 rid,
                                         GWEN_DB_NODE *dbReq);

int LCDM_DeviceManager_HandleCardInserted(LCDM_DEVICEMANAGER *dm,
                                          GWEN_TYPE_UINT32 rid,
                                          GWEN_DB_NODE *dbReq);
int LCDM_DeviceManager_HandleCardRemoved(LCDM_DEVICEMANAGER *dm,
                                         GWEN_TYPE_UINT32 rid,
                                         GWEN_DB_NODE *dbReq);


#endif /* CHIPCARD_SERVER_DEVICEMANAGER_P_H */



