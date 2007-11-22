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
#include <gwenhywfar/stringlist.h>


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

#define LCDM_DEVICEMANAGER_DEF_HARDWARE_SCAN_INTERVAL  2
#define LCDM_DEVICEMANAGER_MIN_HARDWARE_SCAN_INTERVAL  2
#define LCDM_DEVICEMANAGER_DEF_HARDWARE_SCAN_TRIGGERS  4


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

  int disableAutoConf;
  int disablePciScan;
  int disablePcmciaScan;
  int disableUsbRawScan;
  int disableUsbTtyScan;
  unsigned int hardwareScanInterval;
  unsigned int hardwareScanTriggerIntervals;

  time_t lastHardwareScan;
  int triggeredScan;

  char *addrTypeForDrivers;
  char *addrAddrForDrivers;
  int addrPortForDrivers;

  /* runtime vars */
  GWEN_DB_NODE *dbDrivers;
  GWEN_DB_NODE *dbConfigDrivers;

  GWEN_STRINGLIST *driverBlackList;

  LCDM_DRIVER_LIST *drivers;
  LCCO_READER_LIST *readers;

  LC_DEVMONITOR *deviceMonitor;

  int nextNewPort;
  int lastAutoReader;

  uint32_t lastReaderId;

  int readerUsage;
};



static
int LCDM_DeviceManager_ReloadDrivers(LCDM_DEVICEMANAGER *dm);


static
void LCDM_DeviceManager_AbandonDriver(LCDM_DEVICEMANAGER *dm,
                                      LCDM_DRIVER *d,
                                      LC_DRIVER_STATUS newSt,
                                      const char *reason);

static
void LCDM_DeviceManager_AbandonReader(LCDM_DEVICEMANAGER *dm,
                                      LCCO_READER *r,
                                      LC_READER_STATUS newSt,
                                      const char *reason);


static
uint32_t LCDM_DeviceManager_SendStartReader(LCDM_DEVICEMANAGER *dm,
                                                    const LCCO_READER *r);

static
uint32_t LCDM_DeviceManager_SendStopReader(LCDM_DEVICEMANAGER *dm,
                                                   const LCCO_READER *r);

static
uint32_t LCDM_DeviceManager_SendStopDriver(LCDM_DEVICEMANAGER *dm,
                                                   const LCDM_DRIVER *d);

static
uint32_t LCDM_DeviceManager_SendSuspendCheck(LCDM_DEVICEMANAGER *dm,
                                                     const LCCO_READER *r);

static
uint32_t LCDM_DeviceManager_SendResumeCheck(LCDM_DEVICEMANAGER *dm,
                                                    const LCCO_READER *r);


static
int LCDM_DeviceManager_CheckDriver(LCDM_DEVICEMANAGER *dm, LCDM_DRIVER *d);

static
int LCDM_DeviceManager_CheckReader(LCDM_DEVICEMANAGER *dm, LCCO_READER *r);

static
int LCDM_DeviceManager_CheckDrivers(LCDM_DEVICEMANAGER *dm);

static
int LCDM_DeviceManager__Work(LCDM_DEVICEMANAGER *dm);


static
int LCDM_DeviceManager_HandleDriverReady(LCDM_DEVICEMANAGER *dm,
                                         uint32_t rid,
                                         GWEN_DB_NODE *dbReq);

static
int LCDM_DeviceManager_HandleCardInserted(LCDM_DEVICEMANAGER *dm,
                                          uint32_t rid,
                                          GWEN_DB_NODE *dbReq);
static
int LCDM_DeviceManager_HandleCardRemoved(LCDM_DEVICEMANAGER *dm,
                                         uint32_t rid,
                                         GWEN_DB_NODE *dbReq);

static
int LCDM_DeviceManager_HandleReaderError(LCDM_DEVICEMANAGER *dm,
                                         uint32_t rid,
                                         GWEN_DB_NODE *dbReq);

static
int LCDM_DeviceManager_HandleReaderAdd(LCDM_DEVICEMANAGER *dm,
                                       uint32_t rid,
                                       GWEN_DB_NODE *dbReq);

static
int LCDM_DeviceManager_HandleReaderDel(LCDM_DEVICEMANAGER *dm,
                                       uint32_t rid,
                                       GWEN_DB_NODE *dbReq);


static
int LCDM_DeviceManager__GetAutoPortByDeviceId(GWEN_DB_NODE *dbReader,
                                              const LC_DEVICE *dev);

static
int LCDM_DeviceManager__GetAutoPortByPos(GWEN_DB_NODE *dbReader,
                                         const LC_DEVICE *dev,
                                         const LC_DEVICE_LIST *deviceList);

static
int LCDM_DeviceManager__GetAutoPortByFixed(GWEN_DB_NODE *dbReader,
                                           const LC_DEVICE *dev);

static
int LCDM_DeviceManager_GetAutoPort(LCDM_DEVICEMANAGER *dm,
                                   GWEN_DB_NODE *dbReader,
                                   const LC_DEVICE *dev,
                                   const LC_DEVICE_LIST *deviceList);


static
int LCDM_DeviceManager_DeviceUp(LCDM_DEVICEMANAGER *dm,
                                LC_DEVICE *ud,
                                const LC_DEVICE_LIST *deviceList);

static
int LCDM_DeviceManager_DeviceDown(LCDM_DEVICEMANAGER *dm,
                                  const LC_DEVICE *ud);

static
int LCDM_DeviceManager_HardwareScan(LCDM_DEVICEMANAGER *dm);

static
void LCDM_DeviceManager_SetDriverLogFile(LCDM_DEVICEMANAGER *dm,
                                         LCDM_DRIVER *d);


static
void LCDM_DeviceManager_DeleteDriver(LCDM_DEVICEMANAGER *dm,
                                     LCDM_DRIVER *d);


static
int LCDM_DeviceManager_IsSameDevice(const LC_DEVICE *dev1,
                                    const LC_DEVICE *dev2);

#endif /* CHIPCARD_SERVER_DEVICEMANAGER_P_H */



