/***************************************************************************
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "devicemanager_p.h"
#include "server_l.h"
#include "dm_card_l.h"
#include "pciscanner_l.h"
#include "pcmciascanner_l.h"
#include "usbrawscanner_l.h"
#include "usbttyscanner_l.h"
#ifdef USE_HAL
# include "halscanner_l.h"
#endif

#include <chipcard/sharedstuff/driverinfo.h>

#include <gwenhywfar/debug.h>
#include <gwenhywfar/pathmanager.h>
#include <gwenhywfar/plugin.h>
#include <gwenhywfar/ipc.h>


#include <stdlib.h>
#include <assert.h>
#include <string.h>


#ifdef OS_WIN32
# define DIRSEP "\\"
#else
# define DIRSEP "/"
#endif


GWEN_INHERIT_FUNCTIONS(LCDM_DEVICEMANAGER)


LCDM_DEVICEMANAGER *LCDM_DeviceManager_new(LCS_SERVER *server) {
  LCDM_DEVICEMANAGER *dm;

  GWEN_NEW_OBJECT(LCDM_DEVICEMANAGER, dm);
  GWEN_INHERIT_INIT(LCDM_DEVICEMANAGER, dm);
  dm->server=server;
  dm->ipcManager=LCS_Server_GetIpcManager(server);

  dm->drivers=LCDM_Driver_List_new();
  dm->readers=LCCO_Reader_List_new();
  dm->dbDrivers=GWEN_DB_Group_new("drivers");
  dm->dbConfigDrivers=GWEN_DB_Group_new("configDrivers");
  dm->driverBlackList=GWEN_StringList_new();

  return dm;
}



void LCDM_DeviceManager_free(LCDM_DEVICEMANAGER *dm) {
  if (dm) {
    GWEN_INHERIT_FINI(LCDM_DEVICEMANAGER, dm);
    LC_DevMonitor_free(dm->deviceMonitor);
    GWEN_StringList_free(dm->driverBlackList);
    free(dm->addrTypeForDrivers);
    free(dm->addrAddrForDrivers);
    GWEN_DB_Group_free(dm->dbDrivers);
    GWEN_DB_Group_free(dm->dbConfigDrivers);
    LCCO_Reader_List_free(dm->readers);
    LCDM_Driver_List_free(dm->drivers);

    GWEN_FREE_OBJECT(dm);
  }
}



int LCDM_DeviceManager_Init(LCDM_DEVICEMANAGER *dm, GWEN_DB_NODE *dbConfig) {
  GWEN_DB_NODE *dbT;
  const char *p;
  int i;

  DBG_INFO(0, "Initialising device manager");
  assert(dm);
  GWEN_DB_ClearGroup(dm->dbDrivers, 0);

  GWEN_StringList_Clear(dm->driverBlackList);

  /* preset with reasonable values */
  dm->disableAutoConf=0;
  dm->disablePciScan=0;
  dm->disablePcmciaScan=0;
  dm->disableUsbRawScan=0;
  dm->disableUsbTtyScan=0;
  dm->driverStartDelay=LCDM_DEVICEMANAGER_DEF_DRIVER_START_DELAY;
  dm->driverStartTimeout=LCDM_DEVICEMANAGER_DEF_DRIVER_START_TIMEOUT;
  dm->driverStopTimeout=LCDM_DEVICEMANAGER_DEF_DRIVER_STOP_TIMEOUT;
  dm->driverRestartTime=LCDM_DEVICEMANAGER_DEF_DRIVER_RESTART_TIME;
  dm->driverIdleTimeout=LCDM_DEVICEMANAGER_DEF_DRIVER_IDLE_TIMEOUT;

  dm->readerStartDelay=LCDM_DEVICEMANAGER_DEF_READER_START_DELAY;
  dm->readerStartTimeout=LCDM_DEVICEMANAGER_DEF_READER_START_TIMEOUT;
  dm->readerRestartTime=LCDM_DEVICEMANAGER_DEF_READER_RESTART_TIME;
  dm->readerIdleTimeout=LCDM_DEVICEMANAGER_DEF_READER_IDLE_TIMEOUT;
  dm->readerCommandTimeout=LCDM_DEVICEMANAGER_DEF_READER_COMMAND_TIMEOUT;

  dm->hardwareScanInterval=LCDM_DEVICEMANAGER_DEF_HARDWARE_SCAN_INTERVAL;
  dm->hardwareScanTriggerIntervals=
    LCDM_DEVICEMANAGER_DEF_HARDWARE_SCAN_TRIGGERS;

  /* read configuration file */
  dbT=GWEN_DB_GetGroup(dbConfig, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                       "DeviceManager");
  if (dbT==0)
    dbT=dbConfig;

  /* read driver black list */
  for (i=0; ; i++) {
    p=GWEN_DB_GetCharValue(dbT, "driverBlackList", i, 0);
    if (!p)
      break;
    GWEN_StringList_AppendString(dm->driverBlackList, p, 0, 1);
  }

  if (dbT) {
    int defval;

    dm->disableAutoConf=GWEN_DB_GetIntValue(dbT, "disableAutoConf", 0,
                                            dm->disableAutoConf);
    defval=dm->disableAutoConf;
    dm->disablePciScan=GWEN_DB_GetIntValue(dbT, "disablePciScan", 0, defval);
    dm->disablePcmciaScan=GWEN_DB_GetIntValue(dbT, "disablePcmciaScan", 0,
                                              defval);
    dm->disableUsbRawScan=GWEN_DB_GetIntValue(dbT, "disableUsbRawScan", 0,
                                              defval);
    dm->disableUsbTtyScan=GWEN_DB_GetIntValue(dbT, "disableUsbTtyScan", 0,
                                              defval);

    /* read some timeout values */
#define LCDM_DM_INIT_INT(s) \
  dm->s=GWEN_DB_GetIntValue(dbT, __STRING(s), 0, dm->s)
    LCDM_DM_INIT_INT(driverStartDelay);
    LCDM_DM_INIT_INT(driverStartTimeout);
    LCDM_DM_INIT_INT(driverStopTimeout);
    LCDM_DM_INIT_INT(driverRestartTime);
    LCDM_DM_INIT_INT(driverIdleTimeout);
    LCDM_DM_INIT_INT(readerStartDelay);
    LCDM_DM_INIT_INT(readerStartTimeout);
    LCDM_DM_INIT_INT(readerRestartTime);
    LCDM_DM_INIT_INT(readerIdleTimeout);
    LCDM_DM_INIT_INT(readerCommandTimeout);
    LCDM_DM_INIT_INT(hardwareScanInterval);
    LCDM_DM_INIT_INT(hardwareScanTriggerIntervals);
#undef LCDM_DM_INIT_INT
  }

  /* ensure some minimum values */
  if ((dm->hardwareScanInterval!=0) &&
      (dm->hardwareScanInterval<LCDM_DEVICEMANAGER_MIN_HARDWARE_SCAN_INTERVAL))
    dm->hardwareScanInterval=LCDM_DEVICEMANAGER_MIN_HARDWARE_SCAN_INTERVAL;

  /* find config of server to be used for drivers */
  dbT=GWEN_DB_FindFirstGroup(dbConfig, "server");
  while(dbT) {
    if (GWEN_DB_GetIntValue(dbT, "useForDrivers", 0, 0)!=0)
      break;
    dbT=GWEN_DB_FindNextGroup(dbT, "server");
  }
  if (!dbT)
    dbT=GWEN_DB_FindFirstGroup(dbConfig, "server");

  assert(dbT);

  p=GWEN_DB_GetCharValue(dbT, "typ", 0, "local");
  assert(p);
  dm->addrTypeForDrivers=strdup(p);

  p=GWEN_DB_GetCharValue(dbT, "addr", 0, 0);
  assert(p);
  dm->addrAddrForDrivers=strdup(p);

  dm->addrPortForDrivers=GWEN_DB_GetIntValue(dbT, "port", 0, 0);

  /* read drivers */
  dbT=GWEN_DB_GetGroup(dbConfig, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                       "DeviceManager");
  if (dbT==0)
    dbT=dbConfig;


  dbT=GWEN_DB_FindFirstGroup(dbT, "driver");
  while(dbT) {
    LCDM_DRIVER *d;
    GWEN_DB_NODE *dbR;

    /* driver section found, add it to global list and create a driver */
    GWEN_DB_AddGroup(dm->dbDrivers,
                     GWEN_DB_Group_dup(dbT));
    GWEN_DB_AddGroup(dm->dbConfigDrivers,
                     GWEN_DB_Group_dup(dbT));

    d=LCDM_Driver_fromDb(dbT);
    assert(d);

    if (!LCDM_Driver_GetLogFile(d)) {
      LCDM_DeviceManager_SetDriverLogFile(dm, d);
    }
    LCDM_Driver_AddDriverFlags(d, LC_DRIVER_FLAGS_CONFIG);

    DBG_INFO(0, "Adding driver \"%s\"", LCDM_Driver_GetDriverName(d));
    LCDM_Driver_List_Add(d, dm->drivers);

    dbR=GWEN_DB_GetFirstGroup(dbT);
    while(dbR) {
      if (strcasecmp(GWEN_DB_GroupName(dbR), "reader")==0) {
        LCCO_READER *r;

        /* reader section found */
        r=LCDM_Reader_fromDb(d, dbR);
	assert(r);
	/* readers from config are always expected to be active */
	LCCO_Reader_SetIsAvailable(r, 1);
        LCDM_Driver_IncAssignedReadersCount(d);
        DBG_INFO(0, "Adding reader \"%s\"", LCCO_Reader_GetReaderName(r));
        LCS_Server_NewReader(dm->server, r);
        LCCO_Reader_List_Add(r, dm->readers);
      } /* if reader */
      dbR=GWEN_DB_GetNextGroup(dbR);
    } /* while */

    dbT=GWEN_DB_FindNextGroup(dbT, "driver");
  }

  if (dm->disableAutoConf==0) {
    LC_DEVSCANNER *scanner;
    int scanners=0;

    DBG_INFO(0, "Autoconfiguration enabled");
    dm->deviceMonitor=LC_DevMonitor_new();
#ifdef USE_HAL
    DBG_INFO(0, "Adding HAL scanner");
    scanner=LC_HalScanner_new();
    LC_DevMonitor_AddScanner(dm->deviceMonitor, scanner);
    scanners++;
#else
    if (dm->disablePciScan==0) {
      DBG_INFO(0, "Adding PCI bus scanner");
      scanner=LC_PciScanner_new();
      LC_DevMonitor_AddScanner(dm->deviceMonitor, scanner);
      scanners++;
    }
    if (dm->disablePcmciaScan==0) {
      DBG_INFO(0, "Adding PCMCIA bus scanner");
      scanner=LC_PcmciaScanner_new();
      LC_DevMonitor_AddScanner(dm->deviceMonitor, scanner);
      scanners++;
    }
    if (dm->disableUsbRawScan==0) {
      DBG_INFO(0, "Adding USB bus scanner");
      scanner=LC_UsbRawScanner_new();
      LC_DevMonitor_AddScanner(dm->deviceMonitor, scanner);
      scanners++;
    }
    if (dm->disableUsbTtyScan==0) {
      DBG_INFO(0, "Adding USB TTY bus scanner");
      scanner=LC_UsbTtyScanner_new();
      LC_DevMonitor_AddScanner(dm->deviceMonitor, scanner);
      scanners++;
    }
#endif
    dm->lastHardwareScan=0;
    if (scanners==0) {
      DBG_WARN(0,
               "All hardware scanner modules disabled, "
               "disabling autoconfig");
      LC_DevMonitor_free(dm->deviceMonitor);
      dm->deviceMonitor=0;
      dm->disableAutoConf=1;
    }
  }

  LCDM_DeviceManager_ReloadDrivers(dm);

  return 0;
}



int LCDM_DeviceManager_Fini(LCDM_DEVICEMANAGER *dm) {

  DBG_INFO(0, "Deinitializing device manager");

  LC_DevMonitor_free(dm->deviceMonitor);
  dm->deviceMonitor=0;

  GWEN_StringList_Clear(dm->driverBlackList);
  LCCO_Reader_List_Clear(dm->readers);
  LCDM_Driver_List_Clear(dm->drivers);
  free(dm->addrTypeForDrivers);
  dm->addrTypeForDrivers=0;
  free(dm->addrAddrForDrivers);
  dm->addrAddrForDrivers=0;
  GWEN_DB_ClearGroup(dm->dbConfigDrivers, 0);
  GWEN_DB_ClearGroup(dm->dbDrivers, 0);

  return 0;
}



void LCDM_DeviceManager_BeginUseReaders(LCDM_DEVICEMANAGER *dm) {
  LCCO_READER *r;

  assert(dm);
  dm->readerUsage++;
  r=LCCO_Reader_List_First(dm->readers);
  while(r) {
    LCDM_Reader_IncUsageCount(r, 1);
    r=LCCO_Reader_List_Next(r);
  }
}



void LCDM_DeviceManager_EndUseReaders(LCDM_DEVICEMANAGER *dm, int count) {
  LCCO_READER *r;

  assert(dm);
  assert(dm->readerUsage>=count);
  dm->readerUsage-=count;
  r=LCCO_Reader_List_First(dm->readers);
  while(r) {
    LCDM_Reader_DecUsageCount(r, count);
    r=LCCO_Reader_List_Next(r);
  }
}






void LCDM_DeviceManager_BeginUseReader(LCDM_DEVICEMANAGER *dm,
                                       uint32_t rid) {
  LCCO_READER *r;

  assert(dm);
  r=LCCO_Reader_List_First(dm->readers);
  while(r) {
    if (LCCO_Reader_GetReaderId(r)==rid)
      break;
    r=LCCO_Reader_List_Next(r);
  }

  assert(r);
  LCDM_Reader_IncUsageCount(r, 1);
}



void LCDM_DeviceManager_EndUseReader(LCDM_DEVICEMANAGER *dm,
                                     uint32_t rid) {
  LCCO_READER *r;

  assert(dm);
  r=LCCO_Reader_List_First(dm->readers);
  while(r) {
    if (LCCO_Reader_GetReaderId(r)==rid)
      break;
    r=LCCO_Reader_List_Next(r);
  }

  if (r) {
    LCDM_Reader_DecUsageCount(r, 1);
  }
  else {
    DBG_WARN(0, "Reader \"%0x8\" not found", rid);
  }
}



void LCDM_DeviceManager_BeginUseCard(LCDM_DEVICEMANAGER *dm, LCCO_CARD *cd) {
  LCCO_READER *r;

  assert(dm);
  assert(cd);
  r=LCDM_Card_GetReader(cd);
  assert(r);
  LCDM_DeviceManager_BeginUseReader(dm, LCCO_Reader_GetReaderId(r));
}



void LCDM_DeviceManager_EndUseCard(LCDM_DEVICEMANAGER *dm, LCCO_CARD *cd) {
  LCCO_READER *r;

  assert(dm);
  assert(cd);
  r=LCDM_Card_GetReader(cd);
  assert(r);
  LCDM_DeviceManager_EndUseReader(dm, LCCO_Reader_GetReaderId(r));
}



void LCDM_DeviceManager_AbandonReader(LCDM_DEVICEMANAGER *dm,
                                      LCCO_READER *r,
                                      LC_READER_STATUS newSt,
                                      const char *reason) {
  LCDM_DRIVER *d;
  LC_READER_STATUS oldSt;

  d=LCDM_Reader_GetDriver(r);
  assert(d);

  oldSt=LCCO_Reader_GetStatus(r);
  if (oldSt!=newSt) {
    if (oldSt!=LC_ReaderStatusDown &&
	oldSt!=LC_ReaderStatusDisabled &&
        oldSt!=LC_ReaderStatusAborted &&
        oldSt!=LC_ReaderStatusHwDel) {
      DBG_ERROR(0, "Decrementing active reader count %s (%08x) [%d->%d]",
                LCCO_Reader_GetReaderName(r),
                LCCO_Reader_GetReaderId(r),
                oldSt, newSt);
      LCDM_Driver_DecActiveReadersCount(d, 1);
    }

    if (newSt==LC_ReaderStatusHwDel)
      LCCO_Reader_SetIsAvailable(r, 0);
    LCCO_Reader_SetStatus(r, newSt);
    if (newSt==LC_ReaderStatusAborted)
      LCDM_Reader_SetTimeout(r, dm->readerRestartTime);
    else
      LCDM_Reader_SetTimeout(r, 0);

    LCS_Server_ReaderChg(dm->server,
                         LCDM_Driver_GetDriverId(d),
                         r, newSt, reason);
  }
}



void LCDM_DeviceManager_AbandonDriver(LCDM_DEVICEMANAGER *dm,
                                      LCDM_DRIVER *d,
                                      LC_DRIVER_STATUS newSt,
                                      const char *reason) {
  uint32_t ipcId;

  DBG_INFO(0, "Abandoning driver %08x", LCDM_Driver_GetDriverId(d));
  ipcId=LCDM_Driver_GetIpcId(d);
  if (ipcId!=0) {
    /* remove IPC node */
    LCDM_Driver_SetIpcId(d, 0);
    GWEN_IpcManager_RemoveClient(dm->ipcManager, ipcId);
  }

  if (LCDM_Driver_GetStatus(d)!= newSt) {
    LCCO_READER *r;

    LCDM_Driver_SetStatus(d, newSt);
    LCS_Server_DriverChg(dm->server,
                         LCDM_Driver_GetDriverId(d),
                         LCDM_Driver_GetDriverType(d),
                         LCDM_Driver_GetDriverName(d),
                         LCDM_Driver_GetLibraryFile(d),
                         newSt, reason);
    if (newSt==LC_DriverStatusAborted)
      LCDM_Driver_SetTimeout(d, dm->driverRestartTime);
    else
      LCDM_Driver_SetTimeout(d, 0);

    r=LCCO_Reader_List_First(dm->readers);
    while(r) {
      if (LCDM_Reader_GetDriver(r)==d) {
        LC_READER_STATUS rst;

        rst=LCCO_Reader_GetStatus(r);
        if (rst!=LC_ReaderStatusDown &&
            rst!=LC_ReaderStatusDisabled &&
	    rst!=LC_ReaderStatusAborted &&
	    rst!=LC_ReaderStatusHwDel) {
          DBG_INFO(0, "Abandoning reader");
          LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
                                           reason);
        }
      }
      r=LCCO_Reader_List_Next(r);
    }
  }
}



int LCDM_DeviceManager_ReloadDrivers(LCDM_DEVICEMANAGER *dm) {
  GWEN_STRINGLIST *sl;
  GWEN_STRINGLISTENTRY *se;
  GWEN_DB_NODE *dbDrivers;
  GWEN_DB_NODE *dbT;

  DBG_INFO(0, "Reloading driver info files");

  sl=GWEN_PathManager_GetPaths(LCS_PATH_DESTLIB,
                               LCS_PATH_DRIVER_INFODIR);
  assert(sl);

  dbDrivers=GWEN_DB_Group_new("drivers");

  se=GWEN_StringList_FirstEntry(sl);
  while(se) {
    const char *s;
    int rv;

    s=GWEN_StringListEntry_Data(se);
    assert(s);
    /* read all available drivers from folder */
    rv=LC_DriverInfo_ReadDrivers(s, dbDrivers, 1, 0);
    if (rv) {
      DBG_INFO(0, "No driver info in folder \"%s\"", s);
    }
    se=GWEN_StringListEntry_Next(se);
  }
  GWEN_StringList_free(sl);

  GWEN_DB_Group_free(dm->dbDrivers);
  dm->dbDrivers=GWEN_DB_Group_dup(dm->dbConfigDrivers);

  /* remove all groups of drivers which are blacklisted, add all other */
  while( (dbT=GWEN_DB_GetFirstGroup(dbDrivers)) ) {
    const char *n;

    GWEN_DB_UnlinkGroup(dbT);
    n=GWEN_DB_GetCharValue(dbT, "driverName", 0, 0);
    assert(n);
    if (GWEN_StringList_HasString(dm->driverBlackList, n)) {
      DBG_ERROR(0, "Removing driver \"%s\" (blacklisted)", n);
      GWEN_DB_Group_free(dbT);
    }
    else
      GWEN_DB_AddGroup(dm->dbDrivers, dbT);
  }

  GWEN_DB_Group_free(dbDrivers);

  return 0;
}



uint32_t LCDM_DeviceManager_SendStartReader(LCDM_DEVICEMANAGER *dm,
					    const LCCO_READER *r) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  LCDM_DRIVER *d;
  int port;
  const char *p;
  uint32_t rid;

  assert(dm);
  assert(r);
  d=LCDM_Reader_GetDriver(r);
  assert(d);
  dbReq=GWEN_DB_Group_new("Driver_StartReader");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
	      LCCO_Reader_GetReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LCCO_Reader_GetDriversReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "driversReaderId", numbuf);

  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "name", LCCO_Reader_GetReaderName(r));
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
		      "slots", LCCO_Reader_GetSlots(r));
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
		      "flags", LCCO_Reader_GetFlags(r));
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "ctn", LCCO_Reader_GetCtn(r));
  port=LCCO_Reader_GetPort(r);
  if (port==-1) {
    port=0;
    DBG_INFO(0, "Assigning default port 0 to reader \"%s\"",
             LCCO_Reader_GetReaderName(r));
  }
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "port", port);

  p=LCCO_Reader_GetDevicePath(r);
  if (p)
    GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "devicePath", p);

  rv=GWEN_IpcManager_SendRequest(dm->ipcManager,
				 LCDM_Driver_GetIpcId(d),
				 dbReq,
				 &rid);
  if (rv<0) {
    DBG_INFO(0, "here (%d)", rv);
    return 0;
  }

  return rid;
}



uint32_t LCDM_DeviceManager_SendStopReader(LCDM_DEVICEMANAGER *dm,
					   const LCCO_READER *r) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  const char *p;
  LCDM_DRIVER *d;
  uint32_t rid;

  d=LCDM_Reader_GetDriver(r);
  assert(d);
  dbReq=GWEN_DB_Group_new("Driver_StopReader");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
	      LCCO_Reader_GetReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "readerId", numbuf);

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
	      LCCO_Reader_GetDriversReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driversReaderId", numbuf);

  p=LCCO_Reader_GetReaderName(r);
  if (p)
    GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                         "name", p);

  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
		      "port",  LCCO_Reader_GetPort(r));
  GWEN_DB_SetIntValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "slots", LCCO_Reader_GetSlots(r));

  rv=GWEN_IpcManager_SendRequest(dm->ipcManager,
				 LCDM_Driver_GetIpcId(d),
				 dbReq,
				 &rid);
  if (rv<0) {
    DBG_INFO(0, "here (%d)", rv);
    return 0;
  }

  return rid;
}



uint32_t LCDM_DeviceManager_SendStopDriver(LCDM_DEVICEMANAGER *dm,
					   const LCDM_DRIVER *d) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  uint32_t rid;

  assert(d);
  dbReq=GWEN_DB_Group_new("Driver_StopDriver");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
              LCDM_Driver_GetDriverId(d));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driverId", numbuf);

  rv=GWEN_IpcManager_SendRequest(dm->ipcManager,
				 LCDM_Driver_GetIpcId(d),
				 dbReq,
				 &rid);
  if (rv<0) {
    DBG_INFO(0, "here (%d)", rv);
    return 0;
  }

  return rid;
}



uint32_t LCDM_DeviceManager_SendSuspendCheck(LCDM_DEVICEMANAGER *dm,
					     const LCCO_READER *r) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  LCDM_DRIVER *d;
  uint32_t rid;

  assert(dm);
  assert(r);
  d=LCDM_Reader_GetDriver(r);
  assert(d);
  dbReq=GWEN_DB_Group_new("Driver_SuspendCheck");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
	      LCCO_Reader_GetReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
	      LCCO_Reader_GetDriversReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "driversReaderId", numbuf);

  rv=GWEN_IpcManager_SendRequest(dm->ipcManager,
				 LCDM_Driver_GetIpcId(d),
				 dbReq,
				 &rid);
  if (rv<0) {
    DBG_INFO(0, "here (%d)", rv);
    return 0;
  }

  return rid;
}



uint32_t LCDM_DeviceManager_SendResumeCheck(LCDM_DEVICEMANAGER *dm,
					    const LCCO_READER *r) {
  GWEN_DB_NODE *dbReq;
  char numbuf[16];
  int rv;
  LCDM_DRIVER *d;
  uint32_t rid;

  assert(dm);
  assert(r);
  d=LCDM_Reader_GetDriver(r);
  assert(d);
  dbReq=GWEN_DB_Group_new("Driver_ResumeCheck");

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
	      LCCO_Reader_GetReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x",
	      LCCO_Reader_GetDriversReaderId(r));
  assert(rv>0 && rv<sizeof(numbuf)-1);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbReq, GWEN_DB_FLAGS_OVERWRITE_VARS,
		       "driversReaderId", numbuf);

  rv=GWEN_IpcManager_SendRequest(dm->ipcManager,
				 LCDM_Driver_GetIpcId(d),
				 dbReq,
				 &rid);
  if (rv<0) {
    DBG_INFO(0, "here (%d)", rv);
    return 0;
  }

  return rid;
}



int LCDM_DeviceManager_StartDriver(LCDM_DEVICEMANAGER *dm, LCDM_DRIVER *d) {
  GWEN_PROCESS *p=0;
  GWEN_BUFFER *pbuf=0;
  GWEN_BUFFER *abuf=0;
  const char *s;
  char numbuf[32];
  int rv;
  GWEN_PROCESS_STATE pst;
  GWEN_PLUGIN_MANAGER *pm=0;

  assert(dm);
  assert(d);

  DBG_INFO(0, "Starting driver \"%s\"", LCDM_Driver_GetDriverName(d));

  abuf=GWEN_Buffer_new(0, 128, 0, 1);

  s=LCDM_Driver_GetDriverDataDir(d);
  if (s) {
    GWEN_Buffer_AppendString(abuf, "-d ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  s=LCDM_Driver_GetLibraryFile(d);
  if (s) {
    GWEN_Buffer_AppendString(abuf, " -l ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  s=LCDM_Driver_GetLogFile(d);
  if (s) {
    GWEN_Buffer_AppendString(abuf, " --logtype file");
    GWEN_Buffer_AppendString(abuf, " --logfile ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  s=getenv("LCDM_DRIVER_LOGLEVEL");
  if (s) {
    GWEN_Buffer_AppendString(abuf, " --loglevel ");
    GWEN_Buffer_AppendString(abuf, s);
  }

  if (dm->addrTypeForDrivers) {
    GWEN_Buffer_AppendString(abuf, " -t ");
    GWEN_Buffer_AppendString(abuf, dm->addrTypeForDrivers);
  }

  if (dm->addrAddrForDrivers) {
    GWEN_Buffer_AppendString(abuf, " -a ");
    GWEN_Buffer_AppendString(abuf, dm->addrAddrForDrivers);
  }

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%d", dm->addrPortForDrivers);
  assert(rv>0 && rv<sizeof(numbuf));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_Buffer_AppendString(abuf, " -p ");
  GWEN_Buffer_AppendString(abuf, numbuf);

  rv=snprintf(numbuf, sizeof(numbuf)-1, "%08x", LCDM_Driver_GetDriverId(d));
  assert(rv>0 && rv<sizeof(numbuf));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_Buffer_AppendString(abuf, " -i ");
  GWEN_Buffer_AppendString(abuf, numbuf);

  s=LCDM_Driver_GetDriverType(d);
  if (!s) {
    DBG_ERROR(0, "No driver type");
    LCDM_DeviceManager_AbandonDriver(dm, d,
                                     LC_DriverStatusAborted,
                                     "No driver type");
    GWEN_Buffer_free(abuf);
    return -1;
  }

  /* get driver path by loading its plugin description */
  pm=GWEN_PluginManager_FindPluginManager(LCS_PLUGIN_DRIVER);
  if (!pm) {
    DBG_ERROR(0, "Plugin manager \"%s\" not found",
              LCS_PLUGIN_DRIVER);
    GWEN_Buffer_free(abuf);
    abort();
  }
  else {
    GWEN_PLUGIN_DESCRIPTION *pd;
    const char *p;

    pd=GWEN_PluginManager_GetPluginDescr(pm, s);
    if (!pd) {
      DBG_ERROR(0, "Plugin description for driver \"%s\" not found", s);
      LCDM_DeviceManager_AbandonDriver(dm, d,
                                       LC_DriverStatusAborted,
                                       "No plugin description for driver");
      GWEN_Buffer_free(abuf);
      return -1;
    }
    p=GWEN_PluginDescription_GetPath(pd);
    assert(p);
    pbuf=GWEN_Buffer_new(0, 256, 0, 1);
    GWEN_Buffer_AppendString(pbuf, p);
    GWEN_Buffer_AppendString(pbuf, DIRSEP);
    GWEN_Buffer_AppendString(pbuf, s);
    GWEN_PluginDescription_free(pd);
  }

  p=GWEN_Process_new();
  DBG_INFO(0, "Starting driver process for driver \"%s\" (%s)",
           LCDM_Driver_GetDriverName(d), GWEN_Buffer_GetStart(pbuf));
  DBG_INFO(0, "Arguments are: \"%s\"", GWEN_Buffer_GetStart(abuf));

  pst=GWEN_Process_Start(p,
                         GWEN_Buffer_GetStart(pbuf),
                         GWEN_Buffer_GetStart(abuf));
  if (pst!=GWEN_ProcessStateRunning) {
    DBG_ERROR(0, "Unable to execute \"%s %s\"",
              GWEN_Buffer_GetStart(pbuf),
              GWEN_Buffer_GetStart(abuf));
    GWEN_Process_free(p);
    GWEN_Buffer_free(pbuf);
    GWEN_Buffer_free(abuf);
    LCDM_DeviceManager_AbandonDriver(dm, d,
                                     LC_DriverStatusAborted,
                                     "Unable to execute driver");
    return -1;
  }

  /* process started */
  DBG_INFO(0, "Process started");
  GWEN_Buffer_free(pbuf);
  GWEN_Buffer_free(abuf);
  LCDM_Driver_SetProcess(d, p);
  LCDM_Driver_SetStatus(d, LC_DriverStatusStarted);
  /* notify callback */
  LCS_Server_DriverChg(dm->server,
                       LCDM_Driver_GetDriverId(d),
                       LCDM_Driver_GetDriverType(d),
                       LCDM_Driver_GetDriverName(d),
                       LCDM_Driver_GetLibraryFile(d),
                       LC_DriverStatusStarted,
                       "Driver started");
  /* done */
  return 0;
}



void LCDM_DeviceManager_DeleteDriver(LCDM_DEVICEMANAGER *dm,
                                     LCDM_DRIVER *d) {
  LCCO_READER *r;

  assert(dm);
  assert(d);

  /* abandon all readers */
  r=LCCO_Reader_List_First(dm->readers);
  while(r) {
    LCCO_READER *rNext;
  
    rNext=LCCO_Reader_List_Next(r);
    if (LCDM_Reader_GetDriver(r)==d) {
      DBG_INFO(0, "Abandoning reader");
      LCDM_DeviceManager_AbandonReader(dm, r,
                                       LC_ReaderStatusDisabled,
                                       "Driver disappeared");
      DBG_INFO(0, "Removing reader \"%s\"",
               LCCO_Reader_GetReaderName(r));
      /* TODO: Watch out, check that nobody references this reader! */
      LCCO_Reader_List_Del(r);
      LCCO_Reader_SetIsAvailable(r, 0);
      LCCO_Reader_free(r);
    }
    r=rNext;
  }

  LCDM_Driver_List_Del(d);
  LCDM_Driver_free(d);
}



int LCDM_DeviceManager_CheckDriver(LCDM_DEVICEMANAGER *dm, LCDM_DRIVER *d) {
  int done=0;
  uint32_t nid;
  LC_DRIVER_STATUS dst;
  uint32_t dflags;

  assert(dm);
  assert(d);

  nid=LCDM_Driver_GetIpcId(d);
  dst=LCDM_Driver_GetStatus(d);

  DBG_VERBOUS(0, "Checking driver %s (%d)",
              LCDM_Driver_GetDriverName(d), dst);

  dflags=LCDM_Driver_GetDriverFlags(d);
  /* under the following circumstances a driver is removed automatically:
   * 1) it is a local, automatically created driver which is unused and the
   * timeout of the driver elapsed
   * 2) it is a remote driver which has been disconnected: In this case there
   * is no way to reach the driver so we remove it
   */
  if (!(dflags & LC_DRIVER_FLAGS_REMOTE) &&
      (dflags & LC_DRIVER_FLAGS_AUTO)) {
    uint32_t ipcId;

    /* local auto reader */
    ipcId=LCDM_Driver_GetIpcId(d);
    if (LCDM_Driver_GetAssignedReadersCount(d)==0){
      DBG_NOTICE(0, "Driver \"%s\" is unused, removing it",
                 LCDM_Driver_GetDriverName(d));
      if (ipcId!=0)
        /* remove IPC node */
        GWEN_IpcManager_RemoveClient(dm->ipcManager, ipcId);

      LCDM_DeviceManager_DeleteDriver(dm, d);
      return 1;
    }
  } /* if local idle driver timed out*/

  if (dst==LC_DriverStatusAborted) {
    if (!(LCDM_Driver_GetDriverFlags(d) & LC_DRIVER_FLAGS_REMOTE) &&
        LCDM_Driver_CheckTimeout(d)) {
      DBG_NOTICE(0, "Reenabling driver \"%s\"",
                 LCDM_Driver_GetDriverName(d));
      dst=LC_DriverStatusDown;
      LCDM_Driver_SetStatus(d, dst);
      done++;
    }
  } /* if aborted */

  if (dst==LC_DriverStatusWaitForStart) {
    if (LCDM_Driver_CheckTimeout(d)) {
      int rv;

      rv=LCDM_DeviceManager_StartDriver(dm, d);
      if (rv) {
        DBG_INFO(0, "here (%d)", rv);
        return 1;
      }
      dst=LCDM_Driver_GetStatus(d);
      LCDM_Driver_SetTimeout(d, dm->driverStartTimeout);
      done++;
    }
  }

  if (dst==LC_DriverStatusStopping) {
    GWEN_PROCESS *p;

    p=LCDM_Driver_GetProcess(d);
    if (p) {
      GWEN_PROCESS_STATE pst;

      pst=GWEN_Process_CheckState(p);
      if (pst==GWEN_ProcessStateRunning) {
        if (LCDM_Driver_CheckTimeout(d)==1) {
          DBG_WARN(0, "Driver is still running, killing it");
          if (GWEN_Process_Terminate(p)) {
            DBG_ERROR(0, "Could not kill process");
          }
          LCDM_Driver_SetProcess(d, 0);
          dst=LC_DriverStatusAborted;
          LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                           "Driver still running, "
                                           "killing it");
          done++;
        }
        else {
          /* otherwise give the process a little bit time ... */
          DBG_DEBUG(0, "still waiting for driver to go down");
        }
      }
      else if (pst==GWEN_ProcessStateExited) {
        DBG_WARN(0, "Driver terminated normally");
        LCDM_Driver_SetProcess(d, 0);
        dst=LC_DriverStatusDown;
        LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                         "Driver terminated normally");
        done++;
      }
      else if (pst==GWEN_ProcessStateAborted) {
        DBG_WARN(0, "Driver terminated abnormally");
        LCDM_Driver_SetProcess(d, 0);
        dst=LC_DriverStatusAborted;
        LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                         "Driver terminated abnormally");
        done++;;
      }
      else if (pst==GWEN_ProcessStateStopped) {
        DBG_WARN(0, "Driver has been stopped, killing it");
        if (GWEN_Process_Terminate(p)) {
          DBG_ERROR(0, "Could not kill process");
        }
        LCDM_Driver_SetProcess(d, 0);
        dst=LC_DriverStatusAborted;
        LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                         "Driver has been stopped,"
                                         "killing it");
        done++;
      }
      else {
        DBG_ERROR(0, "Unknown process status %d, killing", pst);
        if (GWEN_Process_Terminate(p)) {
          DBG_ERROR(0, "Could not kill process");
        }
        LCDM_Driver_SetProcess(d, 0);
        dst=LC_DriverStatusAborted;
        LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                         "Unknown process status, "
                                         "killing it");
        done++;
      }
    } /* if process */
    else {
      if (!(LCDM_Driver_GetDriverFlags(d) & LC_DRIVER_FLAGS_REMOTE)) {
        DBG_ERROR(0, "No process for local driver:");
        LCDM_Driver_Dump(d, stderr, 2);
        abort();
      }
    }
  } /* if stopping */

  if (dst==LC_DriverStatusStarted) {
    /* driver started, check timeout */
    if (LCDM_Driver_CheckTimeout(d)==1) {
      GWEN_PROCESS *p;

      DBG_WARN(0, "Driver \"%s\" timed out", LCDM_Driver_GetDriverName(d));
      p=LCDM_Driver_GetProcess(d);
      if (p) {
        GWEN_PROCESS_STATE pst;

        pst=GWEN_Process_CheckState(p);
        if (pst==GWEN_ProcessStateRunning) {
          DBG_WARN(0,
                   "Driver is running but did not signal readyness, "
                   "killing it");
          if (GWEN_Process_Terminate(p)) {
            DBG_ERROR(0, "Could not kill process");
          }
          LCDM_Driver_SetProcess(d, 0);
          dst=LC_DriverStatusAborted;
          LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                           "Driver is running but did not "
                                           "signal readyness, "
                                           "killing it");
          done++;
        }
        else if (pst==GWEN_ProcessStateExited) {
          DBG_WARN(0, "Driver terminated without signalling readyness");
          LCDM_Driver_SetProcess(d, 0);
          dst=LC_DriverStatusAborted;
          LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                           "Driver terminated "
                                           "without signalling readyness");
          done++;
        }
        else if (pst==GWEN_ProcessStateAborted) {
          DBG_WARN(0, "Driver terminated abnormally");
          LCDM_Driver_SetProcess(d, 0);
          dst=LC_DriverStatusAborted;
          LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                           "Driver terminated abnormally");
          done++;
        }
        else if (pst==GWEN_ProcessStateStopped) {
          DBG_WARN(0, "Driver has been stopped, killing it");
          if (GWEN_Process_Terminate(p)) {
            DBG_ERROR(0, "Could not kill process");
          }
          LCDM_Driver_SetProcess(d, 0);
          dst=LC_DriverStatusAborted;
          LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                           "Driver has been stopped, "
                                           "killing it");
          done++;
        }
        else {
          DBG_ERROR(0, "Unknown process status %d, killing", pst);
          if (GWEN_Process_Terminate(p)) {
            DBG_ERROR(0, "Could not kill process");
          }
          LCDM_Driver_SetProcess(d, 0);
          dst=LC_DriverStatusAborted;
          LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                           "Unknown process status, "
                                           "killing");
          done++;
        }
      } /* if process */
      else {
        if (!(LCDM_Driver_GetDriverFlags(d) & LC_DRIVER_FLAGS_REMOTE)) {
          DBG_ERROR(0, "No process for local driver:");
          LCDM_Driver_Dump(d, stderr, 2);
          abort();
        }
      }
    }
    else {
      /* otherwise give the process a little bit time ... */
      DBG_DEBUG(0, "still waiting for driver start");
    }
  }

  if (dst==LC_DriverStatusUp) {
    GWEN_PROCESS *p;
    GWEN_PROCESS_STATE pst;

    /* check whether the driver really is still up and running */
    p=LCDM_Driver_GetProcess(d);
    if (p) {
      pst=GWEN_Process_CheckState(p);
      if (pst!=GWEN_ProcessStateRunning) {
        DBG_ERROR(0, "Driver is not running anymore");
        LCDM_Driver_Dump(d, stderr, 2);
        GWEN_Process_Terminate(p);
        LCDM_Driver_SetProcess(d, 0);
        dst=LC_DriverStatusAborted;
        LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                         "Driver is not running anymore");
	done++;
        //return 1;
      }
    } /* if process */
    else {
      if (!(LCDM_Driver_GetDriverFlags(d) & LC_DRIVER_FLAGS_REMOTE)) {
        DBG_ERROR(0, "No process for local driver:");
        LCDM_Driver_Dump(d, stderr, 2);
        abort();
      }
    }

    if (LCDM_Driver_GetIpcId(d)==0) {
      DBG_ERROR(0, "Driver connection is down");
      p=LCDM_Driver_GetProcess(d);
      if (p) {
        GWEN_Process_Terminate(p);
      }
      LCDM_Driver_SetProcess(d, 0);
      dst=LC_DriverStatusAborted;
      LCDM_DeviceManager_AbandonDriver(dm, d, dst, "Driver connection broken");
      done++;
    }

    DBG_VERBOUS(0, "Driver still running");
    if (LCDM_Driver_GetActiveReadersCount(d)==0 &&
        dm->driverIdleTimeout) {
      time_t t;

      /* check for idle timeout */
      t=LCDM_Driver_GetIdleSince(d);
      assert(t);

      if (!(LCDM_Driver_GetDriverFlags(d) & LC_DRIVER_FLAGS_REMOTE) &&
          dm->driverIdleTimeout &&
          (difftime(time(0), t)>dm->driverIdleTimeout)) {
        uint32_t rid;

        DBG_NOTICE(0, "Driver \"%s\" is too long idle, stopping it",
                   LCDM_Driver_GetDriverName(d));

        assert(d);
        rid=LCDM_DeviceManager_SendStopDriver(dm, d);
        if (!rid) {
          DBG_ERROR(0, "Could not send StopDriver command for driver \"%s\"",
                    LCDM_Driver_GetDriverName(d));
          dst=LC_DriverStatusAborted;
          LCDM_DeviceManager_AbandonDriver(dm, d, dst,
                                           "Could not send StopDriver "
                                           "command");
          done++;
        }
        else {
          DBG_DEBUG(0, "Sent StopDriver request for driver \"%s\"",
                    LCDM_Driver_GetDriverName(d));
          dst=LC_DriverStatusStopping;
          LCDM_Driver_SetStatus(d, dst);
          LCS_Server_DriverChg(dm->server,
                               LCDM_Driver_GetDriverId(d),
                               LCDM_Driver_GetDriverType(d),
                               LCDM_Driver_GetDriverName(d),
                               LCDM_Driver_GetLibraryFile(d),
                               dst,
                               "Stopping driver");
          LCDM_Driver_SetTimeout(d, dm->driverStopTimeout);
          done++;
        }
      } /* if timeout */
      /* otherwise reader is not idle */
    }
  }

  if (dst==LC_DriverStatusDown) {
    if (LCDM_Driver_GetActiveReadersCount(d)) {
      if (LCDM_Driver_GetDriverFlags(d) & LC_DRIVER_FLAGS_REMOTE)
        LCDM_Driver_SetTimeout(d, 0);
      else
        LCDM_Driver_SetTimeout(d, dm->driverStartDelay);
      dst=LC_DriverStatusWaitForStart;
      LCDM_Driver_SetStatus(d, dst);
      LCS_Server_DriverChg(dm->server,
                           LCDM_Driver_GetDriverId(d),
                           LCDM_Driver_GetDriverType(d),
                           LCDM_Driver_GetDriverName(d),
                           LCDM_Driver_GetLibraryFile(d),
                           dst,
                           "Initiating driver start");
      done++;
    }
  }

  if (dflags & LC_DRIVER_FLAGS_REMOTE) {
    uint32_t ipcId;

    ipcId=LCDM_Driver_GetIpcId(d);
    if (ipcId==0){
      DBG_NOTICE(0, "Remote driver \"%s\" disappeared, removing it",
                 LCDM_Driver_GetDriverName(d));
      LCDM_DeviceManager_DeleteDriver(dm, d);
      return 1;
    }
  } /* remote driver disconnected */

  if (done)
    return 1;

  return 0;
}



int LCDM_DeviceManager_CheckReader(LCDM_DEVICEMANAGER *dm, LCCO_READER *r) {
  LC_READER_STATUS rst;
  int didSomething=0;
  LCDM_DRIVER *d;

  rst=LCCO_Reader_GetStatus(r);
  d=LCDM_Reader_GetDriver(r);

  DBG_VERBOUS(0, "Checking reader %s (%d)",
              LCCO_Reader_GetReaderName(r), rst);

  if (rst==LC_ReaderStatusHwAdd) {
    rst=LC_ReaderStatusDown;
    LCCO_Reader_SetStatus(r, rst);
    didSomething++;
  }

  if (rst==LC_ReaderStatusHwDel) {
    LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusDisabled,
				     "Reader no longer connected");
    rst=LC_ReaderStatusDisabled;
    LCCO_Reader_SetStatus(r, rst);
    didSomething++;
  }

  /* TODO: needs to be more clean here */
  if (!LCCO_Reader_IsAvailable(r)) {
    if (LCCO_Reader_GetStatus(r)!=LC_ReaderStatusDisabled) {
      LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusDisabled,
				       "Reader no longer connected");
    }
  }

  if (rst==LC_ReaderStatusWaitForStart) {
    if (LCDM_Reader_CheckTimeout(r)) {
      rst=LC_ReaderStatusWaitForDriver;
      LCCO_Reader_SetStatus(r, rst);
      LCDM_Reader_SetTimeout(r, dm->readerStartTimeout);
      didSomething++;
    }
  }

  if (rst==LC_ReaderStatusWaitForDriver) {
    if (LCDM_Reader_CheckTimeout(r)) {
      DBG_ERROR(0, "Reader %s time out", LCCO_Reader_GetReaderName(r));
      LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
				       "Reader timed out");
      return 1;
    }
    else {
      LC_DRIVER_STATUS dst;

      dst=LCDM_Driver_GetStatus(d);
      if (dst==LC_DriverStatusUp) {
	uint32_t rid;

	rid=LCDM_DeviceManager_SendStartReader(dm, r);
	if (rid==0) {
	  DBG_ERROR(0, "Reader %s: Could not send start request",
		    LCCO_Reader_GetReaderName(r));
	  LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
					   "IPC error");
	  return 1;
	}
	rst=LC_ReaderStatusWaitForReaderUp;
	LCCO_Reader_SetStatus(r, rst);
        LCDM_Reader_SetCurrentRequestId(r, rid);
        didSomething++;

        /* inform clients */
        LCS_Server_ReaderChg(dm->server,
                             LCDM_Driver_GetDriverId(d),
                             r,
                             rst,
                             "Reader started");
      }
      else if (dst!=LC_DriverStatusStarted &&
               dst!=LC_DriverStatusWaitForStart) {
        DBG_ERROR(0, "Bad status of driver for reader %s (%d)",
                  LCCO_Reader_GetReaderName(r), dst);
	LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
					 "Bad driver status");
	return 1;
      }
    }
  }

  if (rst==LC_ReaderStatusWaitForReaderUp) {
    uint32_t rid;
    GWEN_DB_NODE *dbRsp;

    rid=LCDM_Reader_GetCurrentRequestId(r);
    assert(rid);
    dbRsp=GWEN_IpcManager_GetResponseData(dm->ipcManager, rid);
    if (dbRsp) {
      if (strcasecmp(GWEN_DB_GroupName(dbRsp), "error")==0) {
	GWEN_BUFFER *ebuf;
	const char *e;

	e=GWEN_DB_GetCharValue(dbRsp, "text", 0, 0);
	DBG_ERROR(0,
                  "Driver reported error on reader startup: %d (%s)",
		  GWEN_DB_GetIntValue(dbRsp, "code", 0, 0),
		  e?e:"<none>");
	ebuf=GWEN_Buffer_new(0, 256, 0, 1);
	if (e) {
	  GWEN_Buffer_AppendString(ebuf, "Reader error (");
	  GWEN_Buffer_AppendString(ebuf, e);
	  GWEN_Buffer_AppendString(ebuf, ")");
	}
	else {
	  GWEN_Buffer_AppendString(ebuf, "Reader error (startup)");
	}
	LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
                                         GWEN_Buffer_GetStart(ebuf));

	GWEN_Buffer_free(ebuf);
	GWEN_DB_Group_free(dbRsp);
        if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 1)) {
          DBG_ERROR(0, "Request not found");
          abort();
        }
	LCDM_Reader_SetCurrentRequestId(r, 0);
       return 1;
      }
      else {
        int code;
        const char *e;

        code=GWEN_DB_GetIntValue(dbRsp, "data/code", 0, -1);
        if (code==-1) {
          DBG_ERROR(0, "Driver did not send a result code:");
          GWEN_DB_Dump(dbRsp, stderr, 2);
          code=LC_ERROR_GENERIC;
        }
        e=GWEN_DB_GetCharValue(dbRsp, "data/text", 0, 0);
        if (code!=0) {
          GWEN_BUFFER *ebuf;

	  DBG_ERROR(0,
                    "Driver reported error %d on startup of "
                    "reader \"%s\": %s",
                    code,
                    LCCO_Reader_GetReaderName(r),
                    e?e:"(none)");
          ebuf=GWEN_Buffer_new(0, 256, 0, 1);
          if (e) {
            GWEN_Buffer_AppendString(ebuf, "Reader error (");
            GWEN_Buffer_AppendString(ebuf, e);
            GWEN_Buffer_AppendString(ebuf, ")");
          }
          else {
            GWEN_Buffer_AppendString(ebuf, "Reader error (startup)");
          }
	  LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
					   GWEN_Buffer_GetStart(ebuf));
          GWEN_Buffer_free(ebuf);
          GWEN_DB_Group_free(dbRsp);
          if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 1)) {
            DBG_ERROR(0, "Request not found");
            abort();
          }

          LCDM_Reader_SetCurrentRequestId(r, 0);
          return -1;
        }
        else {
          const char *readerInfo;

          readerInfo=GWEN_DB_GetCharValue(dbRsp, "data/info", 0, 0);
          if (readerInfo) {
            DBG_NOTICE(0, "Reader \"%s\" is up (%s)",
		       LCCO_Reader_GetReaderName(r),
                       e?e:"no result text");
	    LCCO_Reader_SetReaderInfo(r, readerInfo);
	  }
	  else {
	    DBG_NOTICE(0, "Reader \"%s\" is up (%s), no info",
		       LCCO_Reader_GetReaderName(r),
		       e?e:"no result text");
	  }
	  rst=LC_ReaderStatusUp;
	  LCCO_Reader_SetStatus(r, rst);

	  LCS_Server_ReaderChg(dm->server,
                               LCDM_Driver_GetDriverId(d),
                               r,
                               LC_ReaderStatusUp,
                               e?e:"Reader is up");
          GWEN_DB_Group_free(dbRsp);
          if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 1)) {
            DBG_ERROR(0, "Request not found");
            abort();
          }
          LCDM_Reader_SetCurrentRequestId(r, 0);
          if (LCCO_Reader_GetFlags(r) & LC_READER_FLAGS_SUSPENDED_CHECKS) {
            uint32_t rqid;

            /* checks have been suspended, tell this to the reader */
            rqid=LCDM_DeviceManager_SendSuspendCheck(dm, r);
            if (rqid==0) {
              DBG_INFO(0, "here");
              LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
                                               "Could not send ResumeCheck "
                                               "command");
              return -1;
            }
        
            /* immediately remove this request, we don't expect an answer */
            if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rqid, 1)) {
              DBG_ERROR(0, "Could not remove request");
              abort();
            }
          }
        }
	didSomething++;
      }
    }
    else {
      if (LCDM_Reader_CheckTimeout(r)) {
	/* reader timed out */
	DBG_WARN(0, "Reader \"%s\" timed out", LCCO_Reader_GetReaderName(r));
        if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 1)) {
          DBG_ERROR(0, "Request not found");
          abort();
        }
	LCDM_Reader_SetCurrentRequestId(r, 0);
	LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
                                         "Reader timed out (command)");
	return -1;
      } /* if timeout */
      DBG_DEBUG(0, "Still some time left");
      return 0;
    }
  }

  if (rst==LC_ReaderStatusUp) {
    if (!(LCCO_Reader_GetFlags(r) & LC_READER_FLAGS_KEEP_RUNNING)) {
      if (dm->readerIdleTimeout) {
	time_t t0;

	t0=LCDM_Reader_GetIdleSince(r);
	if (t0) {
	  time_t t1;
  
	  t1=time(0);
	  if (difftime(t1, t0)>=dm->readerIdleTimeout) {
	    LC_DRIVER_STATUS dst;
  
	    /* stop reader */
	    DBG_NOTICE(0, "Reader \"%s\" too long idle, stopping",
		       LCCO_Reader_GetReaderName(r));
	    dst=LCDM_Driver_GetStatus(d);
	    if (dst==LC_DriverStatusUp) {
	      uint32_t rid;
  
	      rid=LCDM_DeviceManager_SendStopReader(dm, r);
	      if (rid==0) {
		DBG_ERROR(0, "Reader %s: Could not send stop request",
			  LCCO_Reader_GetReaderName(r));
		LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
						 "IPC error");
		return 1;
	      }
	      rst=LC_ReaderStatusWaitForReaderDown;
	      LCCO_Reader_SetStatus(r, rst);
	      LCDM_Reader_SetCurrentRequestId(r, rid);
	      LCS_Server_ReaderChg(dm->server,
				   LCDM_Driver_GetDriverId(d),
				   r,
				   rst,
				   "Stopping idle reader");
	      didSomething++;
	    }
	    else {
	      DBG_ERROR(0, "Bad status of driver for reader %s",
			LCCO_Reader_GetReaderName(r));
	      LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
					       "Bad driver status");
	      return 1;
	    }
	  }
	} /* if idle */
      } /* if a reader timeout is specified for the driver */
    } /* if not flag "keepRunning" */
  } /* if readerStatusUp */

  if (rst==LC_ReaderStatusWaitForReaderDown) {
    uint32_t rid;
    GWEN_DB_NODE *dbRsp;

    rid=LCDM_Reader_GetCurrentRequestId(r);
    assert(rid);
    dbRsp=GWEN_IpcManager_GetResponseData(dm->ipcManager, rid);
    if (dbRsp) {
      if (strcasecmp(GWEN_DB_GroupName(dbRsp), "error")==0) {
	GWEN_BUFFER *ebuf;
	const char *e;

	e=GWEN_DB_GetCharValue(dbRsp, "text", 0, 0);
	DBG_ERROR(0,
		  "Driver reported error on reader down: %d (%s)",
		  GWEN_DB_GetIntValue(dbRsp, "code", 0, 0),
		  e?e:"<none>");
	ebuf=GWEN_Buffer_new(0, 256, 0, 1);
	if (e) {
	  GWEN_Buffer_AppendString(ebuf, "Reader error (");
	  GWEN_Buffer_AppendString(ebuf, e);
	  GWEN_Buffer_AppendString(ebuf, ")");
	}
	else {
	  GWEN_Buffer_AppendString(ebuf, "Reader error (down)");
	}
	LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
                                         GWEN_Buffer_GetStart(ebuf));
	GWEN_Buffer_free(ebuf);
	GWEN_DB_Group_free(dbRsp);
        if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 1)) {
          DBG_ERROR(0, "Request not found");
          abort();
        }
	LCDM_Reader_SetCurrentRequestId(r, 0);
       return 1;
      } /* if error message */
      else {
        int code;
        const char *e;

        code=GWEN_DB_GetIntValue(dbRsp, "data/code", 0, -1);
        if (code==-1) {
          DBG_ERROR(0, "Driver did not send a result code:");
          GWEN_DB_Dump(dbRsp, stderr, 2);
          code=LC_ERROR_GENERIC;
        }
        e=GWEN_DB_GetCharValue(dbRsp, "data/text", 0, 0);
        if (code!=0) {
          GWEN_BUFFER *ebuf;

          DBG_ERROR(0,
                    "Driver reported error %d on stopping "
                    "of reader \"%s\": %s",
                    code,
                    LCCO_Reader_GetReaderName(r),
                    e?e:"(none)");
          ebuf=GWEN_Buffer_new(0, 256, 0, 1);
          if (e) {
            GWEN_Buffer_AppendString(ebuf, "Reader error (");
            GWEN_Buffer_AppendString(ebuf, e);
            GWEN_Buffer_AppendString(ebuf, ")");
          }
          else {
	    GWEN_Buffer_AppendString(ebuf, "Reader error (stopping)");
          }
	  LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
					   GWEN_Buffer_GetStart(ebuf));
          GWEN_Buffer_free(ebuf);
          GWEN_DB_Group_free(dbRsp);
          if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 1)) {
            DBG_ERROR(0, "Request not found");
            abort();
          }
          LCDM_Reader_SetCurrentRequestId(r, 0);
	  return -1;
	} /* if error code returned */
        else {
	  DBG_NOTICE(0, "Reader \"%s\" is down as expected",
		     LCCO_Reader_GetReaderName(r));
	  rst=LC_ReaderStatusDown;
	  LCCO_Reader_SetStatus(r, rst);
	  LCS_Server_ReaderChg(dm->server,
                               LCDM_Driver_GetDriverId(d),
                               r,
                               LC_ReaderStatusDown,
                               "Reader is down as expected");
	  GWEN_DB_Group_free(dbRsp);
          if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 1)) {
            DBG_ERROR(0, "Request not found");
            abort();
          }
	  LCDM_Reader_SetCurrentRequestId(r, 0);
          LCDM_Reader_SetTimeout(r, 0);
          DBG_ERROR(0, "Decrementing active reader count");
          LCDM_Driver_DecActiveReadersCount(d, 1);
        }
	didSomething++;
      } /* if not error message */
    }
  } /* if readerStatusWaitForReaderDown */

  if (rst==LC_ReaderStatusAborted) {
    if (LCDM_Reader_CheckTimeout(r)) {
      rst=LC_ReaderStatusDown;
      LCCO_Reader_SetStatus(r, rst);

      LCS_Server_ReaderChg(dm->server,
                           LCDM_Driver_GetDriverId(d),
                           r,
                           LC_ReaderStatusDown,
                           "Reenabling reader");
      didSomething++;
    }
  }

  if (rst==LC_ReaderStatusDown) {
    if (LCDM_Reader_GetUsageCount(r)) {
      if (LCCO_Reader_GetFlags(r) & LC_READER_FLAGS_REMOTE)
        /* no delay for remote readers, because the remote driver
         * is a chipcardd daemon running in slave mode, so that server
         * will guard its own startup times */
        LCDM_Reader_SetTimeout(r, 0);
      else
        LCDM_Reader_SetTimeout(r, dm->readerStartDelay);
      LCCO_Reader_SetStatus(r, LC_ReaderStatusWaitForStart);
      DBG_ERROR(0, "Incrementing active reader count %s (%08x) [%d]",
                LCCO_Reader_GetReaderName(r),
                LCCO_Reader_GetReaderId(r),
		LCCO_Reader_GetStatus(r));
      LCDM_Driver_IncActiveReadersCount(d, 1);
      LCS_Server_ReaderChg(dm->server,
                           LCDM_Driver_GetDriverId(d),
                           r,
                           LC_ReaderStatusWaitForStart,
                           "Starting reader");
      didSomething++;
    }
  } /* if readerStatusDown */

  if (rst==LC_ReaderStatusDisabled) {
    if ((LCCO_Reader_GetFlags(r) & LC_READER_FLAGS_AUTO) &&
	LCDM_Reader_GetUsageCount(r)==0) {
      DBG_NOTICE(0, "Reader \"%s\" is no longer active, removing it",
                 LCCO_Reader_GetReaderName(r));
      LCDM_Driver_DecAssignedReadersCount(LCDM_Reader_GetDriver(r));

      LCCO_Reader_List_Del(r);
      LCCO_Reader_SetIsAvailable(r, 0);
      LCCO_Reader_free(r);
      return 1; /* we did something */
    }
  }

  if (didSomething)
    return 1;
  return 0;
}



int LCDM_DeviceManager_CheckDrivers(LCDM_DEVICEMANAGER *dm) {
  int done=0;
  LCDM_DRIVER *d;

  assert(dm);
  d=LCDM_Driver_List_First(dm->drivers);
  while(d) {
    int rv;
    LCDM_DRIVER *dNext;

    dNext=LCDM_Driver_List_Next(d);
    rv=LCDM_DeviceManager_CheckDriver(dm, d);
    if (rv!=0)
      done++;
    d=dNext;
  }

  if (done)
    return 1;
  return 0;
}



int LCDM_DeviceManager_CheckReaders(LCDM_DEVICEMANAGER *dm) {
  int done=0;
  LCCO_READER *r;

  assert(dm);
  r=LCCO_Reader_List_First(dm->readers);
  while(r) {
    LCCO_READER *rnext;
    int rv;

    rnext=LCCO_Reader_List_Next(r);
    rv=LCDM_DeviceManager_CheckReader(dm, r);
    if (rv!=0)
      done++;
    r=rnext;
  }

  if (done)
    return 1;
  return 0;
}



int LCDM_DeviceManager__Work(LCDM_DEVICEMANAGER *dm) {
  int done=0;
  int rv;

  DBG_VERBOUS(0, "Maybe scanning for hardware changes");
  rv=LCDM_DeviceManager_HardwareScan(dm);
  if (rv!=0)
    done++;

  DBG_VERBOUS(0, "Checking drivers.");
  rv=LCDM_DeviceManager_CheckDrivers(dm);
  if (rv!=0) {
    DBG_VERBOUS(0, "Could work on drivers");
    done++;
  }
  DBG_VERBOUS(0, "Checking readers.");
  rv=LCDM_DeviceManager_CheckReaders(dm);
  if (rv!=0) {
    DBG_VERBOUS(0, "Could work on readers");
    done++;
  }

  if (done)
    return 1;
  return 0;
}



int LCDM_DeviceManager_Work(LCDM_DEVICEMANAGER *dm) {
  int rv;
  int done=0;

  for (;;) {
    rv=LCDM_DeviceManager__Work(dm);
    if (rv!=0)
      done++;
    else
      break;
  }
  if (done)
    return 1;
  return 0;
}



void LCDM_DeviceManager_DriverIpcDown(LCDM_DEVICEMANAGER *dm,
                                      uint32_t ipcId) {
  LCDM_DRIVER *d;

  d=LCDM_Driver_List_First(dm->drivers);
  while(d) {
    if (LCDM_Driver_GetIpcId(d)==ipcId)
      break;
    d=LCDM_Driver_List_Next(d);
  } /* while */

  if (d) {
    DBG_NOTICE(0, "Connection of driver \"%s\" (%08x) just went down",
               LCDM_Driver_GetDriverName(d),
               LCDM_Driver_GetDriverId(d));
    LCDM_Driver_SetIpcId(d, 0);

    /*LCDM_DeviceManager_AbandonDriver(dm, d, LC_DriverStatusDown,
				     "Driver connection went down");*/
  }
}




int LCDM_DeviceManager_HandleRequest(LCDM_DEVICEMANAGER *dm,
                                     uint32_t rid,
                                     const char *name,
                                     GWEN_DB_NODE *dbReq) {
  int rv;

  assert(dm);
  assert(name);

  if (strcasecmp(name, "Driver_Ready")==0) {
    rv=LCDM_DeviceManager_HandleDriverReady(dm, rid, dbReq);
  }
  else if (strcasecmp(name, "Driver_CardInserted")==0) {
    rv=LCDM_DeviceManager_HandleCardInserted(dm, rid, dbReq);
  }
  else if (strcasecmp(name, "Driver_CardRemoved")==0) {
    rv=LCDM_DeviceManager_HandleCardRemoved(dm, rid, dbReq);
  }
  else if (strcasecmp(name, "Driver_ReaderError")==0) {
    rv=LCDM_DeviceManager_HandleReaderError(dm, rid, dbReq);
  }
  else if (strcasecmp(name, "Driver_ReaderAdd")==0) {
    rv=LCDM_DeviceManager_HandleReaderAdd(dm, rid, dbReq);
  }
  else if (strcasecmp(name, "Driver_ReaderDel")==0) {
    rv=LCDM_DeviceManager_HandleReaderDel(dm, rid, dbReq);
  }
  /* Insert more handlers here */
  else {
    DBG_DEBUG(0, "Command \"%s\" not handled by device manager", name);
    rv=1; /* not handled */
  }

  return rv;
}



int LCDM_DeviceManager_HandleDriverReady(LCDM_DEVICEMANAGER *dm,
                                         uint32_t rid,
                                         GWEN_DB_NODE *dbReq) {
  GWEN_DB_NODE *dbRsp;
  uint32_t driverId;
  uint32_t nodeId;
  uint32_t driverFlagsValue;
  uint32_t driverFlagsMask;
  LCDM_DRIVER *d;
  int code;
  const char *text;
  int i;
  GWEN_IO_LAYER *conn;

  assert(dbReq);

  nodeId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeId", 0, 0);
  if (!nodeId) {
    DBG_ERROR(0, "Invalid node id");
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  DBG_DEBUG(0, "Driver %08x: DriverReady", nodeId);

  if (1!=sscanf(GWEN_DB_GetCharValue(dbReq, "data/driverId", 0, "0"),
                "%x", &i)) {
    DBG_ERROR(0, "Invalid driver id (%s)",
              GWEN_DB_GetCharValue(dbReq, "data/driverId", 0, "0"));
    LCS_Server_SendErrorResponse(dm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Invalid driver id");
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }
  driverId=i;
  if (driverId==0 && LCS_Server_GetRole(dm->server)!=LCS_Server_RoleMaster) {
    DBG_ERROR(0, "Invalid driver id, server not in master mode");
    LCS_Server_SendErrorResponse(dm->server, rid,
                                 LC_ERROR_INVALID,
                                 "Invalid driver id, "
                                 "remote drivers not allowed");
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* driver ready */
  /* find driver */
  if (driverId) {
    d=LCDM_Driver_List_First(dm->drivers);
    while(d) {
      if (LCDM_Driver_GetDriverId(d)==driverId)
        break;
      d=LCDM_Driver_List_Next(d);
    } /* while */

    if (!d) {
      DBG_ERROR(0, "Driver \"%08x\" not found", driverId);
      LCS_Server_SendErrorResponse(dm->server, rid,
                                   LC_ERROR_INVALID,
                                   "Driver not found");
      if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
        DBG_WARN(0, "Could not remove request");
        abort();
      }
      return -1;
    }
  }
  else {
    const char *dtype;
    const char *dname;

    /* driver with id=0, must be a remote driver */
    dname=GWEN_DB_GetCharValue(dbReq, "data/driverName", 0, 0);
    dtype=GWEN_DB_GetCharValue(dbReq, "data/driverType", 0, 0);
    if (!dtype) {
      DBG_ERROR(0, "No driver type given in remote driver");
      LCS_Server_SendErrorResponse(dm->server, rid,
                                   LC_ERROR_INVALID,
                                   "No driver type");
      if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
        DBG_WARN(0, "Could not remove request");
        abort();
      }
      return -1;
    }

    if (strcasecmp(dtype, "slave")!=0) {
      DBG_ERROR(0, "Bad remote driver (dtype!=\"slave\")");
      LCS_Server_SendErrorResponse(dm->server, rid,
                                   LC_ERROR_INVALID,
                                   "Bad driver type");
      if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
        DBG_WARN(0, "Could not remove request");
        abort();
      }
      return -1;
    }

    /* create slave driver */
    d=LCDM_Driver_new();
    assert(d);
    LCDM_Driver_SetDriverType(d, "slave");
    LCDM_Driver_SetDriverName(d, dname);
    LCDM_Driver_SetIpcId(d, nodeId);
    driverId=LCDM_Driver_GetDriverId(d);
    LCDM_Driver_AddDriverFlags(d, LC_DRIVER_FLAGS_REMOTE);
    LCDM_Driver_AddDriverFlags(d, LC_DRIVER_FLAGS_AUTO);

    /* add driver to list */
    DBG_INFO(0, "Adding remote driver \"%s\"", dtype);
    LCDM_Driver_List_Add(d, dm->drivers);
  } /* if driver does not exist */

  driverFlagsValue=LC_DriverFlags_fromDb(dbReq, "data/driverFlagsValue");
  driverFlagsMask=LC_DriverFlags_fromDb(dbReq, "data/driverFlagsMask");
  if (driverFlagsMask) {
    uint32_t x;

    /* don't change runtime flags */
    x=(((LCDM_Driver_GetDriverFlags(d) ^ driverFlagsValue) &
        (driverFlagsMask & ~LC_DRIVER_FLAGS_RUNTIME_MASK))) ^
      LCDM_Driver_GetDriverFlags(d);
    LCDM_Driver_SetDriverFlags(d, x);
  }

  /* store node id */
  LCDM_Driver_SetIpcId(d, nodeId);

  /* check code */
  code=GWEN_DB_GetIntValue(dbReq, "data/code", 0, -1);
  text=GWEN_DB_GetCharValue(dbReq, "data/text", 0, "<none>");
  if (code!=0) {
    GWEN_BUFFER *ebuf;

    DBG_ERROR(0, "Error in driver \"%08x\": %s (%d)",
              driverId, text, code);
    ebuf=GWEN_Buffer_new(0, 256, 0, 1);
    GWEN_Buffer_AppendString(ebuf, "Driver error (");
    GWEN_Buffer_AppendString(ebuf, text);
    GWEN_Buffer_AppendString(ebuf, ")");

    /* remove request before removing IPC client */
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }

    LCDM_DeviceManager_AbandonDriver(dm, d,
				     LC_DriverStatusAborted,
				     GWEN_Buffer_GetStart(ebuf));
    GWEN_Buffer_free(ebuf);

    return -1;
  }

  conn=GWEN_IpcManager_GetIoLayer(dm->ipcManager, nodeId);
  assert(conn);
  LCS_Server_UseConnectionFor(dm->server, conn,
			      LCS_Connection_Type_Driver,
			      nodeId);

  DBG_NOTICE(0, "Driver \"%08x\" (%s) is up (%s)",
             driverId, LCDM_Driver_GetDriverName(d), text);
  LCDM_Driver_SetStatus(d, LC_DriverStatusUp);
  LCS_Server_DriverChg(dm->server,
                       LCDM_Driver_GetDriverId(d),
                       LCDM_Driver_GetDriverType(d),
                       LCDM_Driver_GetDriverName(d),
                       LCDM_Driver_GetLibraryFile(d),
                       LC_DriverStatusUp,
                       "Driver up");

  dbRsp=GWEN_DB_Group_new("Driver_ReadyResponse");
  GWEN_DB_SetIntValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "code", 0);
  GWEN_DB_SetCharValue(dbRsp, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "text", "Driver registered");
  if (GWEN_IpcManager_SendResponse(dm->ipcManager, rid, dbRsp)) {
    DBG_ERROR(0, "Could not send response");
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }
  if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
    DBG_WARN(0, "Could not remove request");
    abort();
  }

  return 0;
}



int LCDM_DeviceManager_HandleCardInserted(LCDM_DEVICEMANAGER *dm,
                                          uint32_t rid,
                                          GWEN_DB_NODE *dbReq){
  uint32_t readerId=0;
  uint32_t driversReaderId=0;
  uint32_t x;
  uint32_t nodeId;
  LCCO_READER *r;
  LCDM_DRIVER *d;
  int slotNum;
  uint32_t cardNum;
  LCCO_CARD *card;
  GWEN_BUFFER *atr;
  const void *p;
  unsigned int bs;
  const char *cardType;
  LC_CARD_TYPE ct;
  const char *s;

  nodeId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeId", 0, 0);
  if (!nodeId) {
    DBG_ERROR(0, "Invalid node id");
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  DBG_DEBUG(0, "Driver %08x: Card inserted", nodeId);

  d=LCDM_Driver_List_First(dm->drivers);
  while(d) {
    if (LCDM_Driver_GetIpcId(d)==nodeId)
      break;
    d=LCDM_Driver_List_Next(d);
  }

  if (!d) {
    DBG_ERROR(0, "Driver not found");
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  if (sscanf(GWEN_DB_GetCharValue(dbReq, "data/readerId", 0, "0"),
             "%x",
             &x)==1)
    readerId=x;
  if (sscanf(GWEN_DB_GetCharValue(dbReq, "data/driversReaderId", 0, "0"),
             "%x",
             &x)==1)
    driversReaderId=x;

  if (readerId==0 && driversReaderId==0) {
    DBG_ERROR(0, "Bad reader id");
    GWEN_DB_Dump(dbReq, stderr, 2);
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  slotNum=GWEN_DB_GetIntValue(dbReq, "data/slotNum", 0, 0),
  cardNum=GWEN_DB_GetIntValue(dbReq, "data/cardNum", 0, 0),
  cardType=GWEN_DB_GetCharValue(dbReq, "data/cardType", 0, "unknown");
  if (strcasecmp(cardType, "processor")==0)
    ct=LC_CardTypeProcessor;
  else if (strcasecmp(cardType, "memory")==0)
    ct=LC_CardTypeMemory;
  else {
    DBG_WARN(0, "Unknown card type \"%s\"", cardType);
    ct=LC_CardTypeUnknown;
  }
  atr=0;
  p=GWEN_DB_GetBinValue(dbReq, "data/atr", 0, 0, 0, &bs);
  if (p && bs) {
    atr=GWEN_Buffer_new(0, bs, 0, 1);
    GWEN_Buffer_AppendBytes(atr, p, bs);
  }

  /* find reader */
  if (readerId) {
    r=LCCO_Reader_List_First(dm->readers);
    while(r) {
      if (LCCO_Reader_GetReaderId(r)==readerId)
        break;
      r=LCCO_Reader_List_Next(r);
    } /* while */
  }
  else {
    /* no reader id, this is the case with slave readers */
    r=LCCO_Reader_List_First(dm->readers);
    while(r) {
      if (LCDM_Reader_GetDriver(r)==d &&
          LCCO_Reader_GetDriversReaderId(r)==driversReaderId)
        break;
      r=LCCO_Reader_List_Next(r);
    } /* while */
  }
  if (!r) {
    DBG_ERROR(0, "Reader \"%08x/%08x\" not found",
              readerId, driversReaderId);
    GWEN_Buffer_free(atr);
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  card=LCCO_Card_new();
  LCDM_Card_extend(card, r);
  LCCO_Card_SetReaderId(card, LCCO_Reader_GetReaderId(r));
  LCCO_Card_SetDriversReaderId(card, LCCO_Reader_GetDriversReaderId(r));
  LCCO_Card_SetSlotNum(card, slotNum);
  LCCO_Card_SetCardNum(card, cardNum);
  LCCO_Card_SetReadersCardId(card, cardNum);
  LCCO_Card_SetCardType(card, ct);
  s=LCCO_Reader_GetDriverName(r);
  if (!s)
    /* TODO: No fallback! */
    s=LCDM_Driver_GetDriverName(d);
  LCCO_Card_SetDriverTypeName(card, s);
  LCCO_Card_SetReaderTypeName(card, LCCO_Reader_GetReaderType(r));
  LCCO_Card_SetReaderFlags(card, LCCO_Reader_GetFlags(r));
  if (LCDM_Driver_GetDriverFlags(d) & LC_DRIVER_FLAGS_HAS_VERIFY_FN) {
    LCCO_Card_AddReaderFlags(card, LC_READER_FLAGS_DRIVER_HAS_VERIFY);
  }
  if (atr) {
    LCCO_Card_SetAtr(card,
                     GWEN_Buffer_GetStart(atr),
                     GWEN_Buffer_GetUsedBytes(atr));
    GWEN_Buffer_free(atr);
    atr=0;
  }
  LCCO_Card_SetStatus(card, LC_CardStatusInserted);

  /* make new card known to server */
  DBG_NOTICE(0, "New card \"%08x\" found in reader \"%s\"(%08x)",
             LCCO_Card_GetCardId(card),
             LCCO_Reader_GetReaderName(r),
             LCCO_Reader_GetReaderId(r));
  LCS_Server_NewCard(dm->server, card);
  /* the card is freed here because the device manager doesn't need it.
   * However, other managers which have been informed by LCS_Server_NewCard
   * might very well be interested in the card, in which case they attached
   * themselves accordingly so the following LCCO_Card_free() will not really
   * free the card just yet.
   */
  LCCO_Card_free(card);

  if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
    DBG_ERROR(0, "Could not remove request");
    abort();
  }

  return 0;
}



int LCDM_DeviceManager_HandleCardRemoved(LCDM_DEVICEMANAGER *dm,
                                         uint32_t rid,
                                         GWEN_DB_NODE *dbReq){
  uint32_t nodeId;
  uint32_t x;
  uint32_t readerId=0;
  LCCO_READER *r;
  int slotNum;
  uint32_t cardNum;
  LCDM_DRIVER *d;
  LCCM_CARDMANAGER *cm;
  LCCO_CARD *card;

  nodeId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeId", 0, 0);
  if (!nodeId) {
    DBG_ERROR(0, "Invalid node id");
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  DBG_DEBUG(0, "Driver %08x: Card removed", nodeId);

  cm=LCS_Server_GetCardManager(dm->server);
  assert(cm);

  d=LCDM_Driver_List_First(dm->drivers);
  while(d) {
    if (LCDM_Driver_GetIpcId(d)==nodeId)
      break;
    d=LCDM_Driver_List_Next(d);
  }

  if (!d) {
    DBG_ERROR(0, "Driver not found");
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  if (sscanf(GWEN_DB_GetCharValue(dbReq, "data/readerId", 0, "0"),
             "%x",
             &x)==1)
    readerId=x;
  if (readerId==0) {
    DBG_ERROR(0, "Bad reader id in command \"%s\"",
              GWEN_DB_GroupName(dbReq));
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  slotNum=GWEN_DB_GetIntValue(dbReq, "data/slotNum", 0, 0);
  cardNum=GWEN_DB_GetIntValue(dbReq, "data/cardNum", 0, 0);

  /* find reader */
  if (readerId) {
    r=LCCO_Reader_List_First(dm->readers);
    while(r) {
      if (LCCO_Reader_GetReaderId(r)==readerId)
        break;
      r=LCCO_Reader_List_Next(r);
    } /* while */
  }
  if (!r) {
    DBG_ERROR(0, "Reader \"%08x\" not found", readerId);
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }
  readerId=LCCO_Reader_GetReaderId(r);
  /* find card */
  card=LCCM_CardManager_GetFirstCard(cm);
  while(card) {
    if (LCCO_Card_GetReaderId(card)==readerId &&
        LCCO_Card_GetSlotNum(card)==slotNum &&
        LCCO_Card_GetCardNum(card)==cardNum)
      break;
    card=LCCM_CardManager_GetNextCard(cm, card);
  }

  if (card) {
    DBG_NOTICE(0, "Card \"%08x\" in reader \"%s\" (%08x) removed",
               LCCO_Card_GetCardId(card),
               LCCO_Reader_GetReaderName(r),
               LCCO_Reader_GetReaderId(r));
    LCCO_Card_SetStatus(card, LC_CardStatusRemoved);
    LCS_Server_CardRemoved(dm->server, card);
  }
  else {
    DBG_ERROR(0, "Card %08x/%d/%08x not found",
              readerId, slotNum, cardNum);
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
    DBG_WARN(0, "Could not remove request");
    abort();
  }

  return 0;
}



int LCDM_DeviceManager_HandleReaderError(LCDM_DEVICEMANAGER *dm,
                                         uint32_t rid,
                                         GWEN_DB_NODE *dbReq){
  uint32_t nodeId;
  uint32_t x;
  uint32_t driversReaderId=0;
  LCDM_DRIVER *d;
  LCCO_READER *r;
  const char *text;

  nodeId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeId", 0, 0);
  if (!nodeId) {
    DBG_ERROR(0, "Invalid node id");
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  d=LCDM_Driver_List_First(dm->drivers);
  while(d) {
    if (LCDM_Driver_GetIpcId(d)==nodeId)
      break;
    d=LCDM_Driver_List_Next(d);
  }

  /* find driver */
  if (!d) {
    DBG_ERROR(0, "Driver not found");
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  if (sscanf(GWEN_DB_GetCharValue(dbReq, "data/driversReaderId", 0, "0"),
             "%x",
             &x)==1)
    driversReaderId=x;

  if (driversReaderId==0) {
    DBG_ERROR(0, "Bad reader id in command \"%s\"",
              GWEN_DB_GroupName(dbReq));
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  DBG_NOTICE(0, "Driver %s: Reader error", LCDM_Driver_GetDriverName(d));

  /* find reader */
  r=LCCO_Reader_List_First(dm->readers);
  while(r) {
    if (LCDM_Reader_GetDriver(r)==d &&
        LCCO_Reader_GetDriversReaderId(r)==driversReaderId)
      break;
    r=LCCO_Reader_List_Next(r);
  } /* while */
  if (!r) {
    DBG_ERROR(0, "Reader \"%08x\" not found", driversReaderId);
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  text=GWEN_DB_GetCharValue(dbReq, "data/text", 0, 0);
  if (!text || !*text)
    text="Reader error flagged by driver";

  /* notify all instances */
  LCS_Server_ReaderChg(dm->server,
                       LCDM_Driver_GetDriverId(d),
                       r,
                       LC_ReaderStatusAborted,
                       text);

  if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
    DBG_WARN(0, "Could not remove request");
    abort();
  }

  return 0;
}



int LCDM_DeviceManager_HandleReaderAdd(LCDM_DEVICEMANAGER *dm,
                                       uint32_t rid,
                                       GWEN_DB_NODE *dbReq){
  uint32_t nodeId;
  LCCO_READER *r;
  LCDM_DRIVER *d;
  GWEN_DB_NODE *dbReader;
  GWEN_BUFFER *nbuf;
  char numbuf[32];
  const char *s;

  nodeId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeId", 0, 0);
  if (!nodeId) {
    DBG_ERROR(0, "Invalid node id");
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  DBG_INFO(0, "Driver %08x: Reader added", nodeId);

  /* find driver */
  d=LCDM_Driver_List_First(dm->drivers);
  while(d) {
    if (LCDM_Driver_GetIpcId(d)==nodeId)
      break;
    d=LCDM_Driver_List_Next(d);
  } /* while */

  if (!d) {
    DBG_ERROR(0, "Driver for IPC id \"%08x\" not found", nodeId);
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  dbReader=GWEN_DB_GetGroup(dbReq, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                            "data/reader");
  if (dbReader==0) {
    DBG_ERROR(0, "Bad request (no reader data)");
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* create reader */
  r=LCDM_Reader_fromDb(d, dbReader);
  if (!r) {
    DBG_ERROR(0, "Bad request (bad reader data)");
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }
  nbuf=GWEN_Buffer_new(0, 256, 0, 1);
  snprintf(numbuf, sizeof(numbuf), "%d", ++(dm->lastAutoReader));
  GWEN_Buffer_AppendString(nbuf, "auto");
  GWEN_Buffer_AppendString(nbuf, numbuf);
  GWEN_Buffer_AppendByte(nbuf, '-');
  s=LCCO_Reader_GetReaderType(r);
  if (s)
    GWEN_Buffer_AppendString(nbuf, s);
  else
    GWEN_Buffer_AppendString(nbuf, "slave");
  DBG_NOTICE(0, "Remote reader \"%s\" becomes \"%s\"",
             LCCO_Reader_GetReaderName(r), GWEN_Buffer_GetStart(nbuf));
  LCCO_Reader_SetReaderName(r, GWEN_Buffer_GetStart(nbuf));
  GWEN_Buffer_free(nbuf);

  LCDM_Driver_IncAssignedReadersCount(d);
  if (LCDM_Driver_GetDriverFlags(d) & LC_DRIVER_FLAGS_REMOTE)
    /* if the driver is remote, so is the reader */
    LCCO_Reader_AddFlags(r, LC_READER_FLAGS_REMOTE);
  /* reader has been created automatically */
  LCCO_Reader_AddFlags(r, LC_READER_FLAGS_AUTO);
  /* reader is available */
  LCCO_Reader_SetIsAvailable(r, 1);
  /* let the CheckReader function automatically start this reader
   * if necessary */
  if (dm->readerUsage)
    LCDM_Reader_IncUsageCount(r, dm->readerUsage);
  /* add reader to list */
  LCS_Server_NewReader(dm->server, r);
  LCCO_Reader_List_Add(r, dm->readers);

  /* announce the reader
   * The checkReader function will later change this code to "down" */
  LCS_Server_ReaderChg(dm->server,
                       LCDM_Driver_GetDriverId(d),
                       r,
                       LC_ReaderStatusHwAdd,
                       "Reader plugged-in");

  if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
    DBG_WARN(0, "Could not remove request");
    abort();
  }

  return 0;
}



int LCDM_DeviceManager_HandleReaderDel(LCDM_DEVICEMANAGER *dm,
                                       uint32_t rid,
                                       GWEN_DB_NODE *dbReq) {
  uint32_t readerId;
  uint32_t nodeId;
  LCDM_DRIVER *d;
  LCCO_READER *r;

  nodeId=GWEN_DB_GetIntValue(dbReq, "ipc/nodeId", 0, 0);
  if (!nodeId) {
    DBG_ERROR(0, "Invalid node id");
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* find driver */
  d=LCDM_Driver_List_First(dm->drivers);
  while(d) {
    if (LCDM_Driver_GetIpcId(d)==nodeId)
      break;
    d=LCDM_Driver_List_Next(d);
  } /* while */
  if (!d) {
    DBG_ERROR(0, "Unknown driver id \"%08x\"", nodeId);
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  DBG_NOTICE(0, "Driver %s: Reader unplugged", LCDM_Driver_GetDriverName(d));

  /* get reader id */
  if (sscanf(GWEN_DB_GetCharValue(dbReq, "data/driversReaderId", 0, "0"),
             "%x",
             &readerId)!=1) {
    DBG_ERROR(0, "Bad reader id in command \"%s\"",
              GWEN_DB_GroupName(dbReq));
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  /* find reader */
  r=LCCO_Reader_List_First(dm->readers);
  while(r) {
    if (LCDM_Reader_GetDriver(r)==d &&
        LCCO_Reader_GetDriversReaderId(r)==readerId)
      break;
    r=LCCO_Reader_List_Next(r);
  } /* while */
  if (!r) {
    DBG_ERROR(0, "Reader \"%08x\" not found", readerId);
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
      DBG_WARN(0, "Could not remove request");
      abort();
    }
    return -1;
  }

  LCDM_DeviceManager_AbandonReader(dm, r,
                                   LC_ReaderStatusHwDel,
                                   "Reader unplugged");

  if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rid, 0)) {
    DBG_WARN(0, "Could not remove request");
    abort();
  }

  return 0;
}



int LCDM_DeviceManager_ListDrivers(LCDM_DEVICEMANAGER *dm) {
  LCDM_DRIVER *d;

  assert(dm);
  d=LCDM_Driver_List_First(dm->drivers);
  while(d) {
    LCS_Server_DriverChg(dm->server,
                         LCDM_Driver_GetDriverId(d),
                         LCDM_Driver_GetDriverType(d),
                         LCDM_Driver_GetDriverName(d),
                         LCDM_Driver_GetLibraryFile(d),
                         LCDM_Driver_GetStatus(d),
                         "Driver listing");
    d=LCDM_Driver_List_Next(d);
  }

  return 0;
}



int LCDM_DeviceManager_ListReaders(LCDM_DEVICEMANAGER *dm) {
  LCCO_READER *r;

  assert(dm);
  r=LCCO_Reader_List_First(dm->readers);
  while(r) {
    LCDM_DRIVER *d;

    d=LCDM_Reader_GetDriver(r);
    assert(d);

    LCS_Server_ReaderChg(dm->server,
                         LCDM_Driver_GetDriverId(d),
                         r,
                         LCCO_Reader_GetStatus(r),
                         "Reader listing");
    r=LCCO_Reader_List_Next(r);
  }

  return 0;
}



uint32_t LCDM_DeviceManager_SendCardCommand(LCDM_DEVICEMANAGER *dm,
                                                    LCCO_CARD *card,
                                                    GWEN_DB_NODE *dbCmd) {
  LCCO_READER *r;
  LCDM_DRIVER *d;
  char numbuf[16];
  uint32_t rid;
  int rv;

  assert(dm);
  assert(card);

  r=LCDM_Card_GetReader(card);
  assert(r);

  d=LCDM_Reader_GetDriver(r);
  assert(d);

  if (LCDM_Driver_GetStatus(d)!=LC_DriverStatusUp) {
    DBG_ERROR(0, "Bad driver status (%d)", LCDM_Driver_GetStatus(d));
    GWEN_DB_Group_free(dbCmd);
    return 0;
  }

  if (LCCO_Reader_GetStatus(r)!=LC_ReaderStatusUp) {
    DBG_ERROR(0, "Bad reader status (%d)", LCCO_Reader_GetStatus(r));
    GWEN_DB_Group_free(dbCmd);
    return 0;
  }

  snprintf(numbuf, sizeof(numbuf)-1, "%08x", LCCO_Reader_GetReaderId(r));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbCmd, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);

  snprintf(numbuf, sizeof(numbuf)-1, "%08x",
           LCCO_Reader_GetDriversReaderId(r));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbCmd, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driversReaderId", numbuf);

  GWEN_DB_SetIntValue(dbCmd, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "slotnum", LCCO_Card_GetSlotNum(card));
  GWEN_DB_SetIntValue(dbCmd, GWEN_DB_FLAGS_OVERWRITE_VARS,
                      "cardnum", LCCO_Card_GetReadersCardId(card));

  rv=GWEN_IpcManager_SendRequest(dm->ipcManager,
				 LCDM_Driver_GetIpcId(d),
				 dbCmd,
				 &rid);
  if (rv<0) {
    DBG_ERROR(0, "Could not send request (%d)", rv);
    return 0;
  }

  return rid;
}



uint32_t
LCDM_DeviceManager_SendReaderCommand(LCDM_DEVICEMANAGER *dm,
                                     uint32_t readerId,
                                     GWEN_DB_NODE *dbCmd) {
  LCCO_READER *r;
  LCDM_DRIVER *d;
  char numbuf[16];
  uint32_t rid;
  int rv;

  assert(dm);
  assert(readerId);

  r=LCCO_Reader_List_First(dm->readers);
  while(r) {
    if (LCCO_Reader_GetReaderId(r)==readerId)
      break;
    r=LCCO_Reader_List_Next(r);
  }

  if (!r) {
    DBG_ERROR(0, "Unknown reader \"%08x\"", readerId);
    GWEN_DB_Group_free(dbCmd);
    return 0;
  }

  d=LCDM_Reader_GetDriver(r);
  assert(d);

  if (LCDM_Driver_GetStatus(d)!=LC_DriverStatusUp) {
    DBG_ERROR(0, "Bad driver status (%d)", LCDM_Driver_GetStatus(d));
    GWEN_DB_Group_free(dbCmd);
    return 0;
  }

  if (LCCO_Reader_GetStatus(r)!=LC_ReaderStatusUp) {
    DBG_ERROR(0, "Bad reader status (%d)", LCCO_Reader_GetStatus(r));
    GWEN_DB_Group_free(dbCmd);
    return 0;
  }

  snprintf(numbuf, sizeof(numbuf)-1, "%08x", LCCO_Reader_GetReaderId(r));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbCmd, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "readerId", numbuf);

  snprintf(numbuf, sizeof(numbuf)-1, "%08x",
           LCCO_Reader_GetDriversReaderId(r));
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_DB_SetCharValue(dbCmd, GWEN_DB_FLAGS_OVERWRITE_VARS,
                       "driversReaderId", numbuf);

  rv=GWEN_IpcManager_SendRequest(dm->ipcManager,
				 LCDM_Driver_GetIpcId(d),
				 dbCmd,
				 &rid);
  if (rv<0) {
    DBG_ERROR(0, "Could not send request (%d)", rv);
    return 0;
  }

  return rid;
}




/* __________________________________________________________________________
 * AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
 *                      Determining port values from device
 * YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY
 */


int LCDM_DeviceManager__GetAutoPortByDeviceId(GWEN_DB_NODE *dbReader,
					      const LC_DEVICE *dev) {
  GWEN_DB_NODE *dbAutoPort;
  int offset;
  int port;

  dbAutoPort=GWEN_DB_GetGroup(dbReader, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
                              "autoport");
  assert(dbAutoPort);
  offset=GWEN_DB_GetIntValue(dbAutoPort, "offset", 0, 0);

  port=LC_Device_GetDeviceId(dev);
  port+=offset;

  return port;
}



int LCDM_DeviceManager_IsSameDevice(const LC_DEVICE *dev1,
                                    const LC_DEVICE *dev2) {
  if (LC_Device_GetBusType(dev1)==LC_Device_GetBusType(dev2) &&
      LC_Device_GetBusId(dev1)==LC_Device_GetBusId(dev2) &&
      LC_Device_GetDeviceId(dev1)==LC_Device_GetDeviceId(dev2) &&
      LC_Device_GetProductId(dev1)==LC_Device_GetProductId(dev2)) {
    const char *s1, *s2;

    s1=LC_Device_GetPath(dev1);
    s2=LC_Device_GetPath(dev2);
    if (s1==s2 || (s1 && s2 && strcasecmp(s1, s2)==0))
      return 1;
  }
  return 0;
}



int LCDM_DeviceManager__GetAutoPortByPos(GWEN_DB_NODE *dbReader,
					 const LC_DEVICE *dev,
					 const LC_DEVICE_LIST *deviceList) {
  GWEN_DB_NODE *dbAutoPort;
  int pos=0;
  int i;
  LC_DEVICE_BUSTYPE busType;
  int foundDev=0;
  int offset;

  dbAutoPort=GWEN_DB_GetGroup(dbReader, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
			      "autoport");
  assert(dbAutoPort);
  offset=GWEN_DB_GetIntValue(dbAutoPort, "offset", 0, 0);

  for (i=0; ; i++) {
    const char *bt;

    bt=GWEN_DB_GetCharValue(dbAutoPort, "busorder", i, 0);
    if (!bt)
      break;
    else {
      busType=LC_Device_BusType_fromString(bt);
      if (busType!=LC_Device_BusType_Unknown) {
	const char *sortKey;

        sortKey=GWEN_DB_GetCharValue(dbAutoPort, "sortKey", i, "vendorId");
        if (strcasecmp(sortKey, "vendorId")==0) {
	  LC_DEVICE *tdev;

	  tdev=LC_Device_List_First(deviceList);
          while(tdev) {
            if (LCDM_DeviceManager_IsSameDevice(tdev, dev)) {
              foundDev=1;
              break;
            }
            if (LC_Device_GetBusType(tdev)==busType &&
                LC_Device_GetVendorId(tdev)==LC_Device_GetVendorId(dev)) {
              pos++;
            }
            tdev=LC_Device_List_Next(tdev);
	  }
	} /* if sorted by vendor */
	else if (strcasecmp(sortKey, "productId")==0) {
	  LC_DEVICE *tdev;

	  tdev=LC_Device_List_First(deviceList);
	  while(tdev) {
            if (LCDM_DeviceManager_IsSameDevice(tdev, dev)) {
	      foundDev=1;
	      break;
	    }
	    if (LC_Device_GetBusType(tdev)==busType &&
		LC_Device_GetVendorId(tdev)==LC_Device_GetVendorId(dev) &&
		LC_Device_GetProductId(tdev)==LC_Device_GetProductId(dev)) {
	      pos++;
	    }
	    tdev=LC_Device_List_Next(tdev);
	  }
	} /* if sorted by vendor and product */
        else if (strcasecmp(sortKey, "BusPos")==0) {
	  LC_DEVICE *tdev;

	  tdev=LC_Device_List_First(deviceList);
	  while(tdev) {
            if (LCDM_DeviceManager_IsSameDevice(tdev, dev)) {
              foundDev=1;
              break;
            }
	    if (LC_Device_GetBusType(tdev)==LC_Device_GetBusType(dev))
	      pos++;
	    tdev=LC_Device_List_Next(tdev);
	  }
	} /* if sorted by position */
        else if (strcasecmp(sortKey, "DriverType")==0) {
          LC_DEVICE *tdev;

	  tdev=LC_Device_List_First(deviceList);
          while(tdev) {
            const char *s1, *s2;

            if (LCDM_DeviceManager_IsSameDevice(tdev, dev)) {
	      foundDev=1;
	      break;
            }
            s1=LC_Device_GetDriverType(dev);
            s2=LC_Device_GetDriverType(tdev);
            if (s1 && s2 && strcasecmp(s1, s2)==0)
              pos++;
	    tdev=LC_Device_List_Next(tdev);
	  }
	} /* if sorted by driverType */
        else if (strcasecmp(sortKey, "ReaderType")==0) {
          LC_DEVICE *tdev;

	  tdev=LC_Device_List_First(deviceList);
          while(tdev) {
            const char *s1, *s2;

            if (LCDM_DeviceManager_IsSameDevice(tdev, dev)) {
	      foundDev=1;
	      break;
            }
            s1=LC_Device_GetReaderType(dev);
            s2=LC_Device_GetReaderType(tdev);
            if (s1 && s2 && strcasecmp(s1, s2)==0)
              pos++;
	    tdev=LC_Device_List_Next(tdev);
	  }
	} /* if sorted by readerType */
	else {
	  DBG_ERROR(0, "Unknown sort key \"%s\"", sortKey);
          return -1;
        }
      }
      else {
        DBG_WARN(0, "Unknown bus type \"%s\"", bt);
      }
    }
    if (foundDev)
      break;
  } /* for busorder elements */

  i=pos+offset;
  return i;
}



int LCDM_DeviceManager__GetAutoPortByFixed(GWEN_DB_NODE *dbReader,
					   const LC_DEVICE *dev) {
  GWEN_DB_NODE *dbAutoPort;
  GWEN_BUFFER *tbuf;
  LC_DEVICE_BUSTYPE busType;
  int port;

  dbAutoPort=GWEN_DB_GetGroup(dbReader, GWEN_PATH_FLAGS_NAMEMUSTEXIST,
			      "autoport");
  assert(dbAutoPort);
  tbuf=GWEN_Buffer_new(0, 256, 0, 1);
  GWEN_Buffer_AppendString(tbuf, "ports/");
  busType=LC_Device_GetBusType(dev);
  GWEN_Buffer_AppendString(tbuf, LC_Device_BusType_toString(busType));
  GWEN_Buffer_AppendString(tbuf, "-fixed");
  port=GWEN_DB_GetIntValue(dbReader, GWEN_Buffer_GetStart(tbuf), 0, 0);
  GWEN_Buffer_free(tbuf);

  return port;
}



int LCDM_DeviceManager_GetAutoPort(LCDM_DEVICEMANAGER *dm,
				   GWEN_DB_NODE *dbReader,
				   const LC_DEVICE *dev,
				   const LC_DEVICE_LIST *deviceList) {
  const char *s;
  int port;

  s=GWEN_DB_GetCharValue(dbReader, "autoport/mode", 0, 0);
  if (!s) {
    DBG_ERROR(0, "No autoport mode");
    return -1;
  }

  if (strcasecmp(s, "deviceId")==0)
    port=LCDM_DeviceManager__GetAutoPortByDeviceId(dbReader, dev);
  else if (strcasecmp(s, "pos")==0)
    port=LCDM_DeviceManager__GetAutoPortByPos(dbReader,
                                              dev, deviceList);
  else if (strcasecmp(s, "fixed")==0)
    port=LCDM_DeviceManager__GetAutoPortByFixed(dbReader, dev);
  else {
    DBG_ERROR(0, "Invalid autoport mode \"%s\"", s);
    return -1;
  }

  return port;
}



void LCDM_DeviceManager_SetDriverLogFile(LCDM_DEVICEMANAGER *dm,
                                         LCDM_DRIVER *d) {
  GWEN_BUFFER *lbuf;
  GWEN_STRINGLIST *sl;
  const char *s;

  sl=GWEN_PathManager_GetPaths(LCS_PATH_DESTLIB,
                               LCS_PATH_SERVER_LOGDIR);
  assert(sl);
  s=GWEN_StringList_FirstString(sl);
  assert(s);
  lbuf=GWEN_Buffer_new(0, 256, 0, 1);

  GWEN_Buffer_AppendString(lbuf, s);
  LCS_Server_ReplaceVar(DIRSEP"drivers"DIRSEP"@driver@"
                        DIRSEP"@reader@"
                        ".log",
                        "driver",
                        LCDM_Driver_GetDriverName(d),
                        lbuf);
  LCDM_Driver_SetLogFile(d, GWEN_Buffer_GetStart(lbuf));
  GWEN_Buffer_free(lbuf);
  GWEN_StringList_free(sl);
}



int LCDM_DeviceManager_DeviceUp(LCDM_DEVICEMANAGER *dm,
                                LC_DEVICE *ud,
				const LC_DEVICE_LIST *deviceList) {
  GWEN_DB_NODE *dbDriver;
  GWEN_DB_NODE *dbReader=0;
  const char *busType;

  DBG_DEBUG(0, "Device %s/%04x/%04x up",
            LC_Device_BusType_toString(LC_Device_GetBusType(ud)),
            LC_Device_GetVendorId(ud),
            LC_Device_GetProductId(ud));

  busType=LC_Device_BusType_toString(LC_Device_GetBusType(ud));

  /* find driver and reader configuration */
  dbDriver=GWEN_DB_FindFirstGroup(dm->dbDrivers, "driver");
  while(dbDriver) {
    dbReader=GWEN_DB_FindFirstGroup(dbDriver, "reader");
    while(dbReader) {
      if (strcasecmp(GWEN_DB_GetCharValue(dbReader,
                                          "busType", 0,
                                          "serial"),
		     busType)==0) {
	if ((GWEN_DB_GetIntValue(dbReader, "vendorId", 0, 0)==
	     (int)LC_Device_GetVendorId(ud)) &&
	    (GWEN_DB_GetIntValue(dbReader, "productId", 0, 0)==
	     (int)LC_Device_GetProductId(ud))) {
	  /* reader found */
	  break;
	}
      }
      dbReader=GWEN_DB_FindNextGroup(dbReader, "reader");
    } /* while */

    if (dbReader)
      break;
    dbDriver=GWEN_DB_FindNextGroup(dbDriver, "driver");
  } /* while */

  if (dbDriver && dbReader) {
    LCDM_DRIVER *d;
    const char *dname;
    const char *rtype;
    GWEN_BUFFER *nbuf;
    LCCO_READER *r;
    char numbuf[32];

    /* found reader and driver */
    dname=GWEN_DB_GetCharValue(dbDriver, "driverName", 0, 0);
    assert(dname);
    LC_Device_SetDriverType(ud, dname);

    rtype=GWEN_DB_GetCharValue(dbReader, "readerType", 0, 0);
    assert(rtype);
    LC_Device_SetReaderType(ud, rtype);

    d=LCDM_Driver_List_First(dm->drivers);
    while(d) {
      if (strcasecmp(LCDM_Driver_GetDriverName(d), dname)==0) {
	if (LCDM_Driver_GetMaxReaders(d)>
	    LCDM_Driver_GetAssignedReadersCount(d)) {
	  DBG_NOTICE(0,
		     "Reusing driver %s: MaxReaders: %d, Active Readers: %d",
		     LCDM_Driver_GetDriverName(d),
                     LCDM_Driver_GetMaxReaders(d),
                     LCDM_Driver_GetAssignedReadersCount(d));
	  break;
	}
      }
      d=LCDM_Driver_List_Next(d);
    } /* while */

    if (!d) {
      /* no driver exists, create one */
      d=LCDM_Driver_fromDb(dbDriver);
      assert(d);
      if (!LCDM_Driver_GetLogFile(d)) {
        LCDM_DeviceManager_SetDriverLogFile(dm, d);
      }
      LCDM_Driver_AddDriverFlags(d, LC_DRIVER_FLAGS_AUTO);
      LCDM_Driver_List_Add(d, dm->drivers);
    }

    /* create reader */
    r=LCDM_Reader_fromDb(d, dbReader);
    assert(r);
    LCDM_Driver_IncAssignedReadersCount(d);
    LCCO_Reader_AddFlags(r, LC_READER_FLAGS_AUTO);

    nbuf=GWEN_Buffer_new(0, 256, 0, 1);
    snprintf(numbuf, sizeof(numbuf), "%d", ++(dm->lastAutoReader));
    GWEN_Buffer_AppendString(nbuf, "auto");
    GWEN_Buffer_AppendString(nbuf, numbuf);
    GWEN_Buffer_AppendByte(nbuf, '-');
    GWEN_Buffer_AppendString(nbuf, rtype);
    LCCO_Reader_SetReaderName(r, GWEN_Buffer_GetStart(nbuf));
    DBG_NOTICE(0, "AUTOCONFIG: Created new reader \"%s\" (%s/%04x/%04x)",
               GWEN_Buffer_GetStart(nbuf),
               LC_Device_BusType_toString(LC_Device_GetBusType(ud)),
	       LC_Device_GetVendorId(ud),
	       LC_Device_GetProductId(ud));
    GWEN_Buffer_free(nbuf);

    /* set device port if necessary */
    if (LCCO_Reader_GetAddressType(r)==LCCO_Reader_AddressPort) {
      int port;

      port=LCDM_DeviceManager_GetAutoPort(dm, dbReader, ud, deviceList);
      if (port<0) {
        DBG_WARN(0, "Could not automatically assign port, assuming 1");
        port=1;
      }
      else {
        DBG_NOTICE(0, "Automatically assigned port %d to reader \"%s\"",
                   port, LCCO_Reader_GetReaderName(r));
      }
      LCCO_Reader_SetPort(r, port);
    }

    /* set device path if necessary */
    else if (LCCO_Reader_GetAddressType(r)==LCCO_Reader_AddressDevicePath) {
      const char *s;

      s=LCCO_Reader_GetDevicePathTmpl(r);
      if (s) {
        GWEN_BUFFER *pbuf;

        pbuf=GWEN_Buffer_new(0, 256, 0, 1);
        if (LC_Device_ReplaceVars(ud, s, pbuf)) {
          DBG_INFO(0, "here");
          GWEN_Buffer_free(pbuf);
          LCCO_Reader_free(r);
          return 0;
        }
        LCCO_Reader_SetDevicePath(r, GWEN_Buffer_GetStart(pbuf));
        DBG_NOTICE(0, "Assigned device path \"%s\" to reader \"%s\"",
                   GWEN_Buffer_GetStart(pbuf), LCCO_Reader_GetReaderName(r));
        GWEN_Buffer_free(pbuf);
      }
      else {
        s=LCCO_Reader_GetDevicePath(r);
        if (!s) {
          DBG_ERROR(0, "Device has address mode devicePath but "
                    "neither a template or a path");
          LCCO_Reader_free(r);
          return 0;
        }
      }
    }

    LCCO_Reader_SetDriverName(r, LCDM_Driver_GetDriverName(d));
    LCCO_Reader_SetBusType(r, LC_Device_GetBusType(ud));
    LCCO_Reader_SetBusId(r, LC_Device_GetBusId(ud));
    LCCO_Reader_SetDeviceId(r, LC_Device_GetDeviceId(ud));
    LCCO_Reader_SetIsAvailable(r, 1);
    LCCO_Reader_SetStatus(r, LC_ReaderStatusDown);
    if (dm->readerUsage)
      LCDM_Reader_IncUsageCount(r, dm->readerUsage);
    LCS_Server_NewReader(dm->server, r);
    LCCO_Reader_List_Add(r, dm->readers);
    LCS_Server_ReaderChg(dm->server,
                         LCDM_Driver_GetDriverId(d),
                         r,
                         LC_ReaderStatusHwAdd, "New reader detected");
  }
  else {
    DBG_INFO(0, "Device %s/%04x/%04x is not a known reader",
             LC_Device_BusType_toString(LC_Device_GetBusType(ud)),
             LC_Device_GetVendorId(ud),
             LC_Device_GetProductId(ud));
  }

  return 0;
}



int LCDM_DeviceManager_DeviceDown(LCDM_DEVICEMANAGER *dm,
				  const LC_DEVICE *ud) {
  LCCO_READER *r;

  assert(dm);
  assert(ud);

  r=LCCO_Reader_List_First(dm->readers);
  while(r) {
    if (LCCO_Reader_GetBusType(r)==LC_Device_GetBusType(ud) &&
	LCCO_Reader_GetBusId(r)==LC_Device_GetBusId(ud) &&
	LCCO_Reader_GetDeviceId(r)==LC_Device_GetDeviceId(ud)) {
      LCDM_DRIVER *d;

      DBG_NOTICE(0, "Reader \"%s\" disconnected",
                 LCCO_Reader_GetReaderName(r));
      d=LCDM_Reader_GetDriver(r);

      LCDM_DeviceManager_AbandonReader(dm, r,
				       LC_ReaderStatusHwDel,
				       "Reader unplugged");
      /*
      LCCO_Reader_SetIsAvailable(r, 0);
      LCS_Server_ReaderChg(dm->server,
                           LCDM_Driver_GetDriverId(d),
                           r,
			   LC_ReaderStatusHwDel, "Reader removed");
      */
      /* the rest will be handled in the CheckReader function */
      break;
    }
    r=LCCO_Reader_List_Next(r);
  }

  return 0;
}



int LCDM_DeviceManager_HardwareScan(LCDM_DEVICEMANAGER *dm) {
  time_t t1;
  int reloadedDrivers=0;
  int scanInterval;

  if (dm->disableAutoConf)
    return 0; /* nothing done */

  if (dm->hardwareScanInterval==0 &&
      dm->triggeredScan==0)
    return 0; /* nothing done */

  scanInterval=dm->hardwareScanInterval;
  if (scanInterval==0)
    scanInterval=LCDM_DEVICEMANAGER_DEF_HARDWARE_SCAN_INTERVAL;

  t1=time(0);
  if (dm->lastHardwareScan==0 ||
      difftime(t1, dm->lastHardwareScan)>scanInterval) {
    int rv;

    rv=LC_DevMonitor_Scan(dm->deviceMonitor);
    dm->lastHardwareScan=t1;

    if (rv==1) {
      /* If a scan has been triggered by a SIGNAL then the reason was that
       * a device has become active or inactive.
       * But if there is no change in the scan, we did not catch this change
       * right now, so we just decrement the trigger counter (it is set to
       * a reasonable retry-value by the SIGNAL).
       * Effectively this makes the scanner scan for some time until it
       * finally gives up.
       */
      if (dm->triggeredScan)
	dm->triggeredScan--;
      return 0; /* no changes */
    }
    else if (rv==0) {
      LC_DEVICE_LIST *devFullList;
      LC_DEVICE_LIST *devList;
      LC_DEVICE *dev;
      int changes=0;

      DBG_NOTICE(0, "Changes in hardware list");
      if (dm->triggeredScan)
	dm->triggeredScan--;
      devFullList=LC_DevMonitor_GetCurrentDevices(dm->deviceMonitor);

      /* first handle devices which went down */
      devList=LC_DevMonitor_GetLostDevices(dm->deviceMonitor);
      dev=LC_Device_List_First(devList);
      while(dev) {
        LCDM_DeviceManager_DeviceDown(dm, dev);
        changes++;
        dev=LC_Device_List_Next(dev);
      }

      /* then handle devices which are up */
      devList=LC_DevMonitor_GetNewDevices(dm->deviceMonitor);
      dev=LC_Device_List_First(devList);
      while(dev) {
        if (reloadedDrivers==0) {
          LCDM_DeviceManager_ReloadDrivers(dm);
          reloadedDrivers=1;
        }

        LCDM_DeviceManager_DeviceUp(dm, dev, devFullList);
        changes++;
        dev=LC_Device_List_Next(dev);
      }

      if (changes)
        return 1;
    }
    else {
      DBG_ERROR(0, "Error scanning");
    }
  }

  return 0; /* nothing done */
}



const char *LCDM_DeviceManager_GetDriverVar(LCDM_DEVICEMANAGER *dm,
                                            LCCO_CARD *card,
                                            const char *vname) {
  LCCO_READER *r;
  LCDM_DRIVER *d;
  GWEN_DB_NODE *dbT;

  assert(dm);
  r=LCDM_Card_GetReader(card);
  assert(r);
  d=LCDM_Reader_GetDriver(r);
  assert(d);

  dbT=LCDM_Driver_GetDriverVars(d);
  assert(dbT);

  return GWEN_DB_GetCharValue(dbT, vname, 0, 0);
}



void LCDM_DeviceManager_DumpState(const LCDM_DEVICEMANAGER *dm) {
  if (!dm) {
    fprintf(stderr, "No device manager.\n");
    return;
  }
  else {
    LCDM_DRIVER *d;
    LCCO_READER *r;

    fprintf(stderr, "DeviceManager\n");
    fprintf(stderr, "=====================================\n");
    d=LCDM_Driver_List_First(dm->drivers);
    while(d) {
      LCDM_Driver_Dump(d, stderr, 2);
      d=LCDM_Driver_List_Next(d);
    }

    r=LCCO_Reader_List_First(dm->readers);
    while(r) {
      LCCO_Reader_Dump(r, stderr, 2);
      r=LCCO_Reader_List_Next(r);
    }
  }
}



LCS_LOCKMANAGER*
LCDM_DeviceManager_GetLockManager(const LCDM_DEVICEMANAGER *dm,
                                  uint32_t rid,
                                  int slot) {
  LCCO_READER *r;

  assert(dm);
  r=LCCO_Reader_List_First(dm->readers);
  while(r) {
    if (LCCO_Reader_GetReaderId(r)==rid)
      break;
    r=LCCO_Reader_List_Next(r);
  }

  if (r)
    return LCDM_Reader_GetLockManager(r, slot);
  else {
    DBG_WARN(0, "Reader \"%08x\" not found", rid);
    return 0;
  }

}



void LCDM_DeviceManager_ClientDown(LCDM_DEVICEMANAGER *dm,
                                   uint32_t clid) {
  LCCO_READER *r;

  if (clid==0) {
    DBG_ERROR(0, "Bad client id");
  }
  else {
    assert(dm);
    r=LCCO_Reader_List_First(dm->readers);
    while(r) {
      int slots;
      int i;

      slots=LCCO_Reader_GetSlots(r);
      for (i=0; i<slots; i++) {
	LCS_LOCKMANAGER *lm;

	lm=LCDM_Reader_GetLockManager(r, i);
	assert(lm);
	LCS_LockManager_RemoveAllClientRequests(lm, clid);
      }
      r=LCCO_Reader_List_Next(r);
    }
  }
}



uint32_t LCDM_DeviceManager_LockReader(LCDM_DEVICEMANAGER *dm,
                                               uint32_t rid,
                                               uint32_t clid,
                                               int maxLockTime,
                                               int maxLockCount) {
  LCCO_READER *r;

  assert(dm);
  r=LCCO_Reader_List_First(dm->readers);
  while(r) {
    if (LCCO_Reader_GetReaderId(r)==rid)
      break;
    r=LCCO_Reader_List_Next(r);
  }

  assert(r);
  return LCDM_Reader_LockReader(r, clid, maxLockTime, maxLockCount);
}



int LCDM_DeviceManager_CheckLockReaderRequest(LCDM_DEVICEMANAGER *dm,
                                              uint32_t rid,
                                              uint32_t rqid) {
  LCCO_READER *r;

  assert(dm);
  r=LCCO_Reader_List_First(dm->readers);
  while(r) {
    if (LCCO_Reader_GetReaderId(r)==rid)
      break;
    r=LCCO_Reader_List_Next(r);
  }

  assert(r);
  return LCDM_Reader_CheckLockRequest(r, rqid);
}



int LCDM_DeviceManager_CheckLockReaderAccess(LCDM_DEVICEMANAGER *dm,
                                             uint32_t rid,
                                             uint32_t rqid) {
  LCCO_READER *r;

  assert(dm);
  r=LCCO_Reader_List_First(dm->readers);
  while(r) {
    if (LCCO_Reader_GetReaderId(r)==rid)
      break;
    r=LCCO_Reader_List_Next(r);
  }

  assert(r);
  return LCDM_Reader_CheckLockAccess(r, rqid);
}



int LCDM_DeviceManager_RemoveLockReaderRequest(LCDM_DEVICEMANAGER *dm,
                                               uint32_t rid,
                                               uint32_t rqid) {
  LCCO_READER *r;

  assert(dm);
  r=LCCO_Reader_List_First(dm->readers);
  while(r) {
    if (LCCO_Reader_GetReaderId(r)==rid)
      break;
    r=LCCO_Reader_List_Next(r);
  }

  assert(r);
  return LCDM_Reader_RemoveLockRequest(r, rqid);
}



int LCDM_DeviceManager_UnlockReader(LCDM_DEVICEMANAGER *dm,
                                    uint32_t rid,
                                    uint32_t rqid) {
  LCCO_READER *r;

  assert(dm);
  r=LCCO_Reader_List_First(dm->readers);
  while(r) {
    if (LCCO_Reader_GetReaderId(r)==rid)
      break;
    r=LCCO_Reader_List_Next(r);
  }

  // TODO: don't assert here
  assert(r);
  return LCDM_Reader_Unlock(r, rqid);
}



LC_READER_STATUS LCDM_DeviceManager_GetReaderStatus(LCDM_DEVICEMANAGER *dm,
                                                    uint32_t rid) {
  LCCO_READER *r;

  assert(dm);
  r=LCCO_Reader_List_First(dm->readers);
  while(r) {
    if (LCCO_Reader_GetReaderId(r)==rid)
      break;
    r=LCCO_Reader_List_Next(r);
  }

  if (r)
    return LCCO_Reader_GetStatus(r);
  return LC_ReaderStatusUnknown;
}



int LCDM_DeviceManager_SuspendReaderCheck(LCDM_DEVICEMANAGER *dm,
                                          uint32_t rid) {
  LCCO_READER *r;

  assert(dm);
  r=LCCO_Reader_List_First(dm->readers);
  while(r) {
    if (LCCO_Reader_GetReaderId(r)==rid)
      break;
    r=LCCO_Reader_List_Next(r);
  }

  assert(r);
  LCCO_Reader_AddFlags(r, LC_READER_FLAGS_SUSPENDED_CHECKS);
  if (LCCO_Reader_GetStatus(r)==LC_ReaderStatusUp) {
    uint32_t rqid;

    rqid=LCDM_DeviceManager_SendSuspendCheck(dm, r);
    if (rqid==0) {
      DBG_INFO(0, "here");
      LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
                                       "Could not send SuspendCheck "
                                       "command");
      return -1;
    }

    /* immediately remove this request, we don't expect an answer */
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rqid, 1)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
  }

  return 0;
}



void LCDM_DeviceManager_ResumeReaderCheck(LCDM_DEVICEMANAGER *dm,
                                          uint32_t rid) {
  LCCO_READER *r;

  assert(dm);
  r=LCCO_Reader_List_First(dm->readers);
  while(r) {
    if (LCCO_Reader_GetReaderId(r)==rid)
      break;
    r=LCCO_Reader_List_Next(r);
  }

  assert(r);
  if (LCCO_Reader_GetStatus(r)==LC_ReaderStatusUp) {
    uint32_t rqid;

    rqid=LCDM_DeviceManager_SendResumeCheck(dm, r);
    if (rqid==0) {
      DBG_INFO(0, "here");
      LCDM_DeviceManager_AbandonReader(dm, r, LC_ReaderStatusAborted,
                                       "Could not send ResumeCheck "
                                       "command");
      return;
    }

    /* immediately remove this request, we don't expect an answer */
    if (GWEN_IpcManager_RemoveRequest(dm->ipcManager, rqid, 1)) {
      DBG_ERROR(0, "Could not remove request");
      abort();
    }
  }
  LCCO_Reader_SubFlags(r, LC_READER_FLAGS_SUSPENDED_CHECKS);
}



void LCDM_DeviceManager_TriggerHwScan(LCDM_DEVICEMANAGER *dm) {
  assert(dm);
  dm->triggeredScan=dm->hardwareScanTriggerIntervals;
}













